# Plano de implementação — Transparência, Deferred Lighting e Composite

## Objetivo

Evoluir o pipeline atual do renderer para suportar materiais opacos, mascarados e transparentes utilizando:

```text
Materials
    |
    +--------------------+
    |                    |
    v                    v
GBuffer          TransparentGBuffer
    |                    |
    +---------+----------+
              |
              v
      Deferred Lighting
              |
              v
          Composite
              |
              v
             HDR
```

A implementação deve ser feita de forma incremental, mantendo o GBuffer tradicional para superfícies opacas/masked e criando um caminho separado para superfícies transparentes.

---

## 1. Ajustar os materiais

Antes de alterar os pipelines, os materiais precisam informar **qual tipo de pipeline deve ser utilizado**.

Não devemos determinar o pipeline somente olhando o `alpha` da textura.

O material precisa possuir um `AlphaMode`:

```cpp
enum class AlphaMode
{
    OPAQUE,
    MASK,
    BLEND
};
```

E o material deve possuir informações como:

```cpp
struct Material
{
    TextureHandle albedo;
    TextureHandle normal;
    TextureHandle metallic;

    AlphaMode alphaMode = AlphaMode::OPAQUE;

    float alphaCutoff = 0.5f;
};
```

### Comportamento

#### OPAQUE

* Utiliza o pipeline opaco.
* Escreve no GBuffer.
* Depth test habilitado.
* Depth write habilitado.
* Não utiliza blending.
* Alpha da textura não determina transparência.

#### MASK

* Utiliza o pipeline opaco/masked.
* Escreve no GBuffer.
* Depth test habilitado.
* Depth write habilitado.
* Não utiliza blending.
* O shader realiza alpha test:

```glsl
if (albedo.a < alphaCutoff)
    discard;
```

O objetivo é permitir materiais como:

* folhas;
* grades;
* cercas;
* cabelos;
* superfícies recortadas.

#### BLEND

* Utiliza o pipeline transparente.
* Não escreve no GBuffer opaco.
* Será armazenado no TransparentGBuffer.
* Depth test habilitado.
* Depth write desabilitado.
* Utiliza alpha blending.

Importante:

`BLEND` significa transparência visual. Isso não significa automaticamente que o material possui transmissão física de luz.

---

## 2. Criar o TransparentGBuffer

Além do GBuffer existente, será criado um GBuffer específico para superfícies transparentes.

O GBuffer tradicional continuará representando a superfície opaca visível:

```text
GBuffer
├── Position
├── Normal
├── Albedo
└── Material
```

O novo TransparentGBuffer inicialmente terá apenas **uma camada transparente por pixel**:

```text
TransparentGBuffer
├── Position
├── Normal
├── Albedo
├── Material
└── Depth
```

A primeira versão deve armazenar a superfície transparente necessária para o Deferred Lighting naquele pixel.

A possibilidade de múltiplas camadas transparentes será avaliada posteriormente.

## Objetivo

O TransparentGBuffer deve permitir que o Deferred Lighting receba todas as informações necessárias para iluminar uma superfície transparente sem precisar descobrir novamente sua geometria durante cada teste de iluminação.

A ideia é:

```text
Geometry
    |
    +--> GBuffer
    |
    +--> TransparentGBuffer
```

---

## 3. Criar o pipeline transparente

O pipeline de geometria atual deverá ser separado conceitualmente em:

```text
Opaque/Masked Pipeline
Transparent Pipeline
```

### Opaque/Masked

```text
Depth Test  = ON
Depth Write = ON
Blending    = OFF
```

Escreve:

```text
GBuffer
Depth
```

### Transparent

```text
Depth Test  = ON
Depth Write = OFF
Blending    = ON
```

Escreve:

```text
TransparentGBuffer
```

O pipeline transparente deve possuir estados apropriados para alpha blending.

A configuração inicial esperada é:

```cpp
blendEnable = VK_TRUE;

srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

colorBlendOp = VK_BLEND_OP_ADD;
```

A configuração exata do alpha channel será definida de acordo com a utilização do attachment.

---

## 4. Alterar o Deferred Lighting

O Deferred Lighting atualmente trabalha com o GBuffer opaco.

Ele deverá passar a receber:

```text
GBuffer
+
TransparentGBuffer
```

A estrutura lógica será:

```text
Deferred Lighting
├── GBuffer
├── TransparentGBuffer
├── Lights
└── Shadow Pass
```

Para cada pixel, o shader deverá verificar se existe uma superfície transparente.

Conceitualmente:

```glsl
opaque = loadGBuffer(pixel);
transparent = loadTransparentGBuffer(pixel);

if (opaque.valid)
{
    // iluminação da superfície opaca
}

if (transparent.valid)
{
    // iluminação da superfície transparente
}
```

Caso não exista entrada no TransparentGBuffer:

```text
TransparentGBuffer
    |
    +--> vazio
          |
          +--> não realiza iluminação transparente
```

Isso é importante porque testes de iluminação e principalmente testes de sombra/ray tracing podem ser caros.

O objetivo é evitar executar trabalho de transparência quando o pixel não possui superfície transparente.

---

## 5. Shadow Pass

O Shadow Pass continuará sendo utilizado pelo Deferred Lighting.

Inicialmente, a prioridade é manter o comportamento existente para:

```text
OPAQUE
MASK
```

Para materiais transparentes, o comportamento deverá ser definido separadamente.

No futuro, materiais com transmissão poderão permitir que o shadow ray atravesse determinados objetos transparentes em vez de tratar o primeiro hit como um bloqueador absoluto.

Essa parte não precisa ser completamente implementada na primeira etapa do TransparentGBuffer.

---

## 7. Criar o Composite

Atualmente o Composite ainda não existe.

Ele será responsável por juntar os resultados produzidos pelo Deferred Lighting.

Fluxo:

```text
GBuffer
     |
     v
TransparentGBuffer
     |
     v
Deferred Lighting
     |
     +------------------+
     |                  |
     v                  v
Opaque Lighting   Transparent Lighting
     |                  |
     +---------+--------+
               |
               v
           Composite
               |
               v
              HDR
```

O Composite não deve recalcular iluminação.

Sua responsabilidade é realizar a composição dos resultados.

Conceitualmente:

```text
FinalColor =
    TransparentLighting
    composited over
    OpaqueLighting
```

Para alpha blending:

```text
result =
    transparentColor * transparentAlpha +
    opaqueColor * (1.0 - transparentAlpha);
```

A implementação final dependerá de como os attachments de iluminação forem organizados.

---

# Estado desejado ao final

A arquitetura final inicial deverá ser:

```text
                                      Geometry
                                         |
                               +---------+---------+
                               |                   |
                               v                   v
                            GBuffer       TransparentGBuffer
                               |                   |
                       +-------+-------------------+------+
                       |                                  |
                       v                                  v
              Deferred Lighting                      Ray Tracing
                       |                                  |
                       v                                  v
          Opaque Light + Transparent Light        Reflection Color
                       |                                  |
                       |                                  |
                       +-----------------+----------------+
                                         |
                                         v
                                     Composite
                                         |
                                         v
                                        HDR
```
