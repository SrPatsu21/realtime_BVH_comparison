#!/usr/bin/env python3

from pathlib import Path
import math
import re

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parent
OUTPUT_DIR = ROOT / "comparacao_nsys"
ALGORITHM_FOLDERS = ["bsah", "bvh8", "default", "lbvh"]

# Implementações que serão consideradas como referência para os cálculos relativos.
REFERENCE = "default"

# Métricas explicitamente indisponíveis no Nsight Systems utilizado.
UNAVAILABLE_METRICS = {
    "Ray Tracing Throughput",
    "Shader Execution",
    "Warp Divergence",
}

# Métricas principais para a comparação.
MAIN_METRICS = [
    "BLAS Build - CPU",
    "BLAS Upload - CPU",
    "TLAS Build - CPU",
    "TLAS Upload",
    "GPU Frame Duration (Present Delta)",
    "CPU Frame Duration (Acquire Delta)",
    "Vulkan Synchronization APIs (Wait/Fences/Semaphores)",
    "Vulkan Memory Allocation Size",
    "GPU Context Switch Count",
    "Power",
    "Temperature",
    "SMs Active [Throughput %]",
    "SM Issue [Throughput %]",
    "DRAM Read Bandwidth [Throughput %]",
    "DRAM Write Bandwidth [Throughput %]",
    "PCIe RX Throughput [Throughput %]",
    "PCIe TX Throughput [Throughput %]",
]

# Métricas auxiliares que podem ser analisadas.
AUXILIARY_METRICS = [
    "Compute Warps in Flight [Avg Warps per Cycle]",
    "Compute Warps in Flight [Avg]",
    "Compute Warps in Flight [Throughput %]",
    "Pixel Warps in Flight [Avg Warps per Cycle]",
    "Pixel Warps in Flight [Avg]",
    "Pixel Warps in Flight [Throughput %]",
    "Vertex/Tess/Geometry Warps in Flight [Avg Warps per Cycle]",
    "Vertex/Tess/Geometry Warps in Flight [Avg]",
    "Vertex/Tess/Geometry Warps in Flight [Throughput %]",
    "Async Copy Engine Active 0 [Cycles Active]",
    "Async Copy Engine Active 0 [Throughput %]",
    "Sync Copy Engine Active [Cycles Active]",
    "Sync Copy Engine Active [Throughput %]",
    "Tensor Active [Throughput %]",
    "GPC Clock Frequency [MHz]",
    "SYS Clock Frequency [MHz]",
    "PCIe Read Requests to BAR1 [Requests]",
    "PCIe Write Requests to BAR1 [Requests]",
]

# Chamadas Vulkan que são úteis individualmente para comparação.
VULKAN_IMPORTANT_APIS = [
    "vkAcquireNextImageKHR",
    "vkAllocateCommandBuffers",
    "vkAllocateDescriptorSets",
    "vkAllocateMemory",
    "vkBeginCommandBuffer",
    "vkBindBufferMemory",
    "vkBindImageMemory",
    "vkCmdBindDescriptorSets",
    "vkCmdBindPipeline",
    "vkCreateBuffer",
    "vkCreateCommandPool",
    "vkCreateDevice",
    "vkCreateFence",
    "vkCreateFramebuffer",
    "vkCreateGraphicsPipelines",
    "vkCreateImage",
    "vkCreateImageView",
    "vkCreateSemaphore",
    "vkCreateShaderModule",
    "vkCreateSwapchainKHR",
    "vkDeviceWaitIdle",
    "vkEndCommandBuffer",
    "vkFreeMemory",
    "vkMapMemory",
    "vkQueuePresentKHR",
    "vkQueueSubmit",
    "vkUnmapMemory",
    "vkUpdateDescriptorSets",
    "vkWaitForFences",
]

# Métricas onde menor valor representa, em geral, melhor desempenho.
LOWER_IS_BETTER = {
    "BLAS Build - CPU",
    "BLAS Upload - CPU",
    "TLAS Build - CPU",
    "TLAS Upload",
    "GPU Frame Duration (Present Delta)",
    "CPU Frame Duration (Acquire Delta)",
    "Vulkan Synchronization APIs (Wait/Fences/Semaphores)",
    "GPU Context Switch Count",
    "Power",
    "Temperature",
    "Vulkan Memory Allocation Size",
    "vkAcquireNextImageKHR",
    "vkAllocateCommandBuffers",
    "vkAllocateDescriptorSets",
    "vkAllocateMemory",
    "vkBeginCommandBuffer",
    "vkBindBufferMemory",
    "vkBindImageMemory",
    "vkCmdBindDescriptorSets",
    "vkCmdBindPipeline",
    "vkCreateBuffer",
    "vkCreateCommandPool",
    "vkCreateDevice",
    "vkCreateFence",
    "vkCreateFramebuffer",
    "vkCreateGraphicsPipelines",
    "vkCreateImage",
    "vkCreateImageView",
    "vkCreateSemaphore",
    "vkCreateShaderModule",
    "vkCreateSwapchainKHR",
    "vkDeviceWaitIdle",
    "vkEndCommandBuffer",
    "vkFreeMemory",
    "vkMapMemory",
    "vkQueuePresentKHR",
    "vkQueueSubmit",
    "vkUnmapMemory",
    "vkUpdateDescriptorSets",
    "vkWaitForFences",
}


def normalize_name(value):
    if pd.isna(value):
        return ""

    value = str(value).strip()

    # Remove espaços duplicados.
    value = re.sub(r"\s+", " ", value)

    return value


def safe_float(value):
    try:
        if pd.isna(value):
            return np.nan

        value = str(value).strip()

        if not value:
            return np.nan

        return float(value)

    except (ValueError, TypeError):
        return np.nan


def find_report_files():
    """
    Procura recursivamente arquivos *_report_analise.xlsx,
    ignorando a pasta de saída.
    """

    files = []

    for path in ROOT.rglob("*_report_analise.xlsx"):
        if OUTPUT_DIR in path.parents:
            continue

        if path.is_file():
            files.append(path)

    return sorted(files)


def implementation_name(path):
    """
    Usa a primeira pasta abaixo da raiz como nome da implementação.

    Exemplo:
        root/bvh8/foo_report_analise.xlsx
        -> bvh8
    """

    try:
        relative = path.relative_to(ROOT)

        if len(relative.parts) >= 2:
            return relative.parts[0]

    except ValueError:
        pass

    return path.parent.name


def read_report(path):
    """
    Lê o Excel procurando automaticamente a tabela que contém
    tipo_metrica e nome_metrica.
    """

    sheets = pd.read_excel(path, sheet_name=None)

    selected = None

    for sheet_name, df in sheets.items():

        columns = {
            normalize_name(column).lower()
            for column in df.columns
        }

        if "tipo_metrica" in columns and "nome_metrica" in columns:
            selected = df.copy()
            break

    if selected is None:
        raise ValueError(
            f"Nenhuma tabela de métricas encontrada em {path.name}"
        )

    selected.columns = [
        normalize_name(column)
        for column in selected.columns
    ]

    selected["tipo_metrica"] = selected["tipo_metrica"].map(normalize_name)
    selected["nome_metrica"] = selected["nome_metrica"].map(normalize_name)
    selected["unidade"] = selected["unidade"].map(normalize_name)

    numeric_columns = [
        "quantidade_amostras",
        "soma",
        "media",
        "mediana",
        "moda",
        "desvio_padrao",
        "minimo",
        "maximo",
        "quantidade_amostras_sem_outliers",
        "soma_sem_outliers",
        "media_sem_outliers",
        "mediana_sem_outliers",
        "moda_sem_outliers",
        "desvio_padrao_sem_outliers",
        "minimo_sem_outliers",
        "maximo_sem_outliers",
        "quantidade_outliers",
        "percentual_outliers",
        "limite_inferior_iqr",
        "limite_superior_iqr",
    ]

    for column in numeric_columns:
        if column in selected.columns:
            selected[column] = selected[column].map(safe_float)

    return selected


def normalize_units(df):
    """
    Normaliza algumas unidades somente para apresentação.

    Os valores originais permanecem disponíveis nas colunas originais.
    """

    df = df.copy()

    def convert_frequency(row):
        name = row["nome_metrica"]
        value = row["valor"]

        if pd.isna(value):
            return value, row["unidade"]

        if name in {
            "GPC Clock Frequency [MHz]",
            "SYS Clock Frequency [MHz]",
        }:
            # Os arquivos informam Hz apesar do nome conter [MHz].
            if abs(value) > 10_000_000:
                return value / 1_000_000, "MHz"

        return value, row["unidade"]

    converted = df.apply(convert_frequency, axis=1)

    df["valor_normalizado"] = [item[0] for item in converted]
    df["unidade_normalizada"] = [item[1] for item in converted]

    return df


def choose_value(row):
    """
    Escolhe a média como estatística principal da comparação.

    A coluna ``valor`` é usada pelos gráficos e pelas comparações,
    portanto ela deve receber o campo ``media`` do relatório.
    A média sem outliers continua disponível separadamente.
    """

    mean = safe_float(row.get("media"))

    if not pd.isna(mean):
        return mean, "media"

    return np.nan, "indisponível"

def prepare_data(reports):
    """
    Converte todos os relatórios para uma estrutura única.
    """

    rows = []

    for implementation, path, df in reports:

        for _, row in df.iterrows():

            metric = normalize_name(row["nome_metrica"])

            if not metric:
                continue

            value, statistic = choose_value(row)

            rows.append({
                "implementacao": implementation,
                "tipo_metrica": row["tipo_metrica"],
                "nome_metrica": metric,
                "unidade": row["unidade"],
                "quantidade_amostras": row.get(
                    "quantidade_amostras", np.nan
                ),
                "media": row.get("media", np.nan),
                "mediana": row.get("mediana", np.nan),
                "desvio_padrao": row.get("desvio_padrao", np.nan),
                "minimo": row.get("minimo", np.nan),
                "maximo": row.get("maximo", np.nan),
                "media_sem_outliers": row.get(
                    "media_sem_outliers", np.nan
                ),
                "mediana_sem_outliers": row.get(
                    "mediana_sem_outliers", np.nan
                ),
                "desvio_padrao_sem_outliers": row.get(
                    "desvio_padrao_sem_outliers", np.nan
                ),
                "minimo_sem_outliers": row.get(
                    "minimo_sem_outliers", np.nan
                ),
                "maximo_sem_outliers": row.get(
                    "maximo_sem_outliers", np.nan
                ),
                "quantidade_outliers": row.get(
                    "quantidade_outliers", np.nan
                ),
                "percentual_outliers": row.get(
                    "percentual_outliers", np.nan
                ),
                "limite_inferior_iqr": row.get(
                    "limite_inferior_iqr", np.nan
                ),
                "limite_superior_iqr": row.get(
                    "limite_superior_iqr", np.nan
                ),
                "valor": value,
                "estatistica_utilizada": statistic,
            })

    result = pd.DataFrame(rows)

    if result.empty:
        return result

    result = normalize_units(result)

    return result


def add_vulkan_total(data):
    """
    Calcula o tempo total das APIs Vulkan observadas.

    É mantido como uma métrica derivada separada.
    """

    rows = []

    for implementation in data["implementacao"].unique():

        subset = data[
            (data["implementacao"] == implementation)
            & (data["tipo_metrica"] == "Vulkan_API_Duration")
            & (data["nome_metrica"].isin(VULKAN_IMPORTANT_APIS))
        ]

        if subset.empty:
            continue

        total = subset["media"].sum()

        rows.append({
            "implementacao": implementation,
            "tipo_metrica": "Vulkan_API_Duration",
            "nome_metrica": "Vulkan API Total Time [derivada]",
            "unidade": "ms",
            "quantidade_amostras": np.nan,
            "media": total,
            "mediana": np.nan,
            "desvio_padrao": np.nan,
            "minimo": np.nan,
            "maximo": np.nan,
            "media_sem_outliers": np.nan,
            "mediana_sem_outliers": np.nan,
            "desvio_padrao_sem_outliers": np.nan,
            "minimo_sem_outliers": np.nan,
            "maximo_sem_outliers": np.nan,
            "quantidade_outliers": np.nan,
            "percentual_outliers": np.nan,
            "limite_inferior_iqr": np.nan,
            "limite_superior_iqr": np.nan,
            "valor": total,
            "estatistica_utilizada": "soma das médias das APIs",
            "valor_normalizado": total,
            "unidade_normalizada": "ms",
        })

    if rows:
        data = pd.concat(
            [data, pd.DataFrame(rows)],
            ignore_index=True
        )

    return data


def make_pivot(data, value_column="valor"):
    pivot = data.pivot_table(
        index=["tipo_metrica", "nome_metrica", "unidade_normalizada"],
        columns="implementacao",
        values=value_column,
        aggfunc="first",
    )

    return pivot.reset_index()


def relative_comparison(data, reference=REFERENCE):
    """
    Calcula percentual relativo à referência.

    Para todas as métricas:
        ((valor - referência) / referência) * 100
    """

    pivot = data.pivot_table(
        index="nome_metrica",
        columns="implementacao",
        values="valor",
        aggfunc="first",
    )

    if reference not in pivot.columns:
        return pd.DataFrame()

    rows = []

    for metric, row in pivot.iterrows():

        reference_value = row.get(reference, np.nan)

        if pd.isna(reference_value) or reference_value == 0:
            continue

        result = {
            "nome_metrica": metric,
            f"{reference}_referencia": reference_value,
        }

        for implementation in pivot.columns:

            if implementation == reference:
                continue

            value = row.get(implementation, np.nan)

            if pd.isna(value):
                result[implementation] = np.nan
            else:
                result[implementation] = (
                    (value - reference_value)
                    / abs(reference_value)
                ) * 100.0

        rows.append(result)

    return pd.DataFrame(rows)


def find_best(row, implementations, metric):
    values = {}

    for implementation in implementations:

        value = row.get(implementation, np.nan)

        if pd.isna(value):
            continue

        values[implementation] = value

    if not values:
        return None, np.nan

    if metric in LOWER_IS_BETTER:
        best = min(values, key=values.get)
    else:
        best = max(values, key=values.get)

    return best, values[best]


def build_summary(data, implementations):
    """
    Cria resumo com melhor implementação e diferença em relação ao default.
    """

    pivot = data.pivot_table(
        index="nome_metrica",
        columns="implementacao",
        values="valor",
        aggfunc="first",
    )

    rows = []

    for metric, row in pivot.iterrows():

        best, best_value = find_best(
            row,
            implementations,
            metric
        )

        result = {
            "nome_metrica": metric,
            "melhor_implementacao": best,
            "melhor_valor": best_value,
        }

        reference_value = row.get(
            REFERENCE,
            np.nan
        )

        if not pd.isna(reference_value):

            result["referencia"] = reference_value

            if reference_value != 0 and best is not None:
                result["melhor_vs_referencia_%"] = (
                    (best_value - reference_value)
                    / abs(reference_value)
                ) * 100
            else:
                result["melhor_vs_referencia_%"] = np.nan

        else:
            result["referencia"] = np.nan
            result["melhor_vs_referencia_%"] = np.nan

        for implementation in implementations:
            result[implementation] = row.get(
                implementation,
                np.nan
            )

        rows.append(result)

    return pd.DataFrame(rows)


def format_axis_unit(unit):
    if pd.isna(unit):
        return ""

    return str(unit)


def metric_slug(name):
    name = str(name)

    name = (
        name.replace("%", "percent")
        .replace("/", "_")
        .replace("\\", "_")
    )

    name = re.sub(r"[^\w\s-]", "", name)
    name = re.sub(r"\s+", "_", name)

    return name.lower()[:100]


def save_metric_chart(
    data,
    metric,
    output_path,
    statistic="valor",
    chart_type="bar",
):
    subset = data[
        data["nome_metrica"] == metric
    ].copy()

    if subset.empty:
        return False

    subset = subset.dropna(subset=[statistic])

    if subset.empty:
        return False

    subset = subset.sort_values("implementacao")

    implementations = subset["implementacao"].tolist()
    values = subset[statistic].astype(float).tolist()

    unit = ""

    if "unidade_normalizada" in subset:
        units = subset["unidade_normalizada"].dropna().unique()

        if len(units):
            unit = str(units[0])

    plt.figure(figsize=(10, 6))

    x = np.arange(len(implementations))

    if chart_type == "bar":

        bars = plt.bar(
            x,
            values,
        )

        plt.xticks(
            x,
            implementations,
            rotation=0,
        )

        for bar, value in zip(bars, values):
            if not math.isfinite(value):
                continue

            plt.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height(),
                f"{value:.4g}",
                ha="center",
                va="bottom",
                fontsize=9,
            )

    else:

        plt.plot(
            x,
            values,
            marker="o",
        )

        plt.xticks(
            x,
            implementations,
        )

    plt.title(metric)

    if unit:
        plt.ylabel(unit)

    plt.grid(
        axis="y",
        alpha=0.25,
    )

    plt.tight_layout()

    output_path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    plt.savefig(
        output_path,
        dpi=180,
        bbox_inches="tight",
    )

    plt.close()

    return True


def save_statistical_chart(
    data,
    metric,
    output_path,
):
    """
    Gráfico com média, mediana, média sem outliers e
    intervalo mínimo/máximo.
    """

    subset = data[
        data["nome_metrica"] == metric
    ].copy()

    if subset.empty:
        return False

    subset = subset.sort_values("implementacao")

    if subset["media"].notna().sum() == 0:
        return False

    implementations = subset["implementacao"].tolist()
    x = np.arange(len(implementations))

    mean = subset["media"].astype(float).to_numpy()
    median = subset["mediana"].astype(float).to_numpy()
    mean_clean = subset["media_sem_outliers"].astype(float).to_numpy()
    minimum = subset["minimo"].astype(float).to_numpy()
    maximum = subset["maximo"].astype(float).to_numpy()

    unit = ""

    units = subset["unidade_normalizada"].dropna().unique()

    if len(units):
        unit = str(units[0])

    plt.figure(figsize=(11, 6))

    valid_range = (
        np.isfinite(minimum)
        & np.isfinite(maximum)
    )

    if valid_range.any():
        plt.vlines(
            x[valid_range],
            minimum[valid_range],
            maximum[valid_range],
            linewidth=2,
            alpha=0.35,
            label="Mínimo / Máximo",
        )

    valid_mean = np.isfinite(mean)

    if valid_mean.any():
        plt.scatter(
            x[valid_mean],
            mean[valid_mean],
            marker="o",
            label="Média",
        )

    valid_median = np.isfinite(median)

    if valid_median.any():
        plt.scatter(
            x[valid_median],
            median[valid_median],
            marker="D",
            label="Mediana",
        )

    valid_clean = np.isfinite(mean_clean)

    if valid_clean.any():
        plt.scatter(
            x[valid_clean],
            mean_clean[valid_clean],
            marker="s",
            label="Média sem outliers",
        )

    plt.xticks(
        x,
        implementations,
    )

    plt.title(metric)

    if unit:
        plt.ylabel(unit)

    plt.grid(
        axis="y",
        alpha=0.25,
    )

    plt.legend()

    plt.tight_layout()

    output_path.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    plt.savefig(
        output_path,
        dpi=180,
        bbox_inches="tight",
    )

    plt.close()

    return True


def save_boxplot_from_statistics(
    data,
    metric,
    output_path,
):
    """
    Não gera boxplot falso.

    O Excel atual não possui as amostras individuais nem Q1/Q3
    diretamente. Portanto esta função não tenta reconstruir
    um boxplot a partir de média/desvio padrão.

    Retorna False para que o relatório estatístico seja usado.
    """

    return False


def generate_metric_graphs(data, metrics):
    graph_dir = OUTPUT_DIR / "graficos"

    generated = 0

    for metric in metrics:

        slug = metric_slug(metric)

        path = graph_dir / f"{slug}.png"

        if save_statistical_chart(
            data,
            metric,
            path,
        ):
            generated += 1

    return generated


def generate_individual_value_graphs(data):
    """
    Gera gráficos individuais para todas as métricas encontradas.
    """

    graph_dir = OUTPUT_DIR / "graficos" / "todas_metricas"

    generated = 0

    metrics = sorted(
        data["nome_metrica"].dropna().unique()
    )

    for metric in metrics:

        path = graph_dir / f"{metric_slug(metric)}.png"

        if save_metric_chart(
            data,
            metric,
            path,
        ):
            generated += 1

    return generated


def generate_category_graphs(data):
    categories = {
        "build": [
            "BLAS Build - CPU",
            "BLAS Upload - CPU",
            "TLAS Build - CPU",
            "TLAS Upload",
        ],
        "frame": [
            "GPU Frame Duration (Present Delta)",
            "CPU Frame Duration (Acquire Delta)",
        ],
        "memoria": [
            "Vulkan Memory Allocation Size",
        ],
        "sistema": [
            "Power",
            "Temperature",
            "GPU Context Switch Count",
        ],
        "gpu": [
            "SMs Active [Throughput %]",
            "SM Issue [Throughput %]",
            "DRAM Read Bandwidth [Throughput %]",
            "DRAM Write Bandwidth [Throughput %]",
            "PCIe RX Throughput [Throughput %]",
            "PCIe TX Throughput [Throughput %]",
        ],
        "vulkan": [
            "Vulkan Synchronization APIs (Wait/Fences/Semaphores)",
            "Vulkan API Total Time [derivada]",
        ],
    }

    generated = 0

    for category, metrics in categories.items():

        directory = OUTPUT_DIR / "graficos" / category

        for metric in metrics:

            path = directory / f"{metric_slug(metric)}.png"

            if save_statistical_chart(
                data,
                metric,
                path,
            ):
                generated += 1

    return generated


def create_comparison_excel(
    data,
    summary,
    relative,
):
    output = OUTPUT_DIR / "comparacao_nsys.xlsx"

    with pd.ExcelWriter(
        output,
        engine="openpyxl",
    ) as writer:

        # Dados completos.
        data.to_excel(
            writer,
            sheet_name="Dados",
            index=False,
        )

        # Comparação principal.
        main = data[
            data["nome_metrica"].isin(MAIN_METRICS)
        ].copy()

        main.to_excel(
            writer,
            sheet_name="Metricas_Principais",
            index=False,
        )

        # Métricas auxiliares.
        aux = data[
            data["nome_metrica"].isin(AUXILIARY_METRICS)
        ].copy()

        aux.to_excel(
            writer,
            sheet_name="Metricas_Auxiliares",
            index=False,
        )

        # Todas as métricas em formato comparativo.
        pivot = make_pivot(
            data,
            "valor",
        )

        pivot.to_excel(
            writer,
            sheet_name="Comparacao",
            index=False,
        )

        # Média.
        mean_pivot = make_pivot(
            data,
            "media",
        )

        mean_pivot.to_excel(
            writer,
            sheet_name="Medias",
            index=False,
        )

        # Mediana.
        median_pivot = make_pivot(
            data,
            "mediana",
        )

        median_pivot.to_excel(
            writer,
            sheet_name="Medianas",
            index=False,
        )

        # Desvio padrão.
        std_pivot = make_pivot(
            data,
            "desvio_padrao",
        )

        std_pivot.to_excel(
            writer,
            sheet_name="Desvio_Padrao",
            index=False,
        )

        # Resumo.
        summary.to_excel(
            writer,
            sheet_name="Resumo",
            index=False,
        )

        # Comparação relativa.
        if not relative.empty:
            relative.to_excel(
                writer,
                sheet_name="Relativo_Default",
                index=False,
            )

        # Métricas indisponíveis.
        unavailable = pd.DataFrame({
            "Metrica": sorted(UNAVAILABLE_METRICS),
            "Status": [
                "Não coletada / não disponível no Nsight Systems"
                for _ in UNAVAILABLE_METRICS
            ],
        })

        unavailable.to_excel(
            writer,
            sheet_name="Nao_Disponiveis",
            index=False,
        )

        # APIs Vulkan.
        vulkan = data[
            data["nome_metrica"].isin(
                VULKAN_IMPORTANT_APIS
            )
        ].copy()

        vulkan.to_excel(
            writer,
            sheet_name="Vulkan_APIs",
            index=False,
        )

    return output


def print_metric_comparison(
    data,
    metric,
    implementations,
):
    subset = data[
        data["nome_metrica"] == metric
    ]

    if subset.empty:
        return

    print()
    print("-" * 70)
    print(metric)

    for implementation in implementations:

        row = subset[
            subset["implementacao"] == implementation
        ]

        if row.empty:
            print(f"  {implementation:<15}: N/D")
            continue

        value = row.iloc[0]["valor"]

        unit = row.iloc[0].get(
            "unidade_normalizada",
            "",
        )

        statistic = row.iloc[0].get(
            "estatistica_utilizada",
            "",
        )

        if pd.isna(value):
            print(f"  {implementation:<15}: N/D")
        else:
            print(
                f"  {implementation:<15}: "
                f"{value:.6g} {unit} "
                f"({statistic})"
            )


def print_summary(
    data,
    implementations,
):
    print()
    print("=" * 70)
    print("COMPARAÇÃO NSIGHT SYSTEMS")
    print("=" * 70)

    print()
    print("Implementações encontradas:")

    for implementation in implementations:
        print(f"  {implementation}")

    print()
    print("Métricas principais:")

    for metric in MAIN_METRICS:
        print_metric_comparison(
            data,
            metric,
            implementations,
        )

    print()
    print("=" * 70)
    print("MÉTRICAS NÃO DISPONÍVEIS")
    print("=" * 70)

    for metric in sorted(UNAVAILABLE_METRICS):
        print(f"  {metric}")

    if REFERENCE in implementations:

        print()
        print("=" * 70)
        print(f"COMPARAÇÃO RELATIVA A {REFERENCE}")
        print("=" * 70)

        pivot = data.pivot_table(
            index="nome_metrica",
            columns="implementacao",
            values="valor",
            aggfunc="first",
        )

        for metric in MAIN_METRICS:

            if metric not in pivot.index:
                continue

            reference = pivot.loc[
                metric
            ].get(
                REFERENCE,
                np.nan,
            )

            if pd.isna(reference) or reference == 0:
                continue

            print()
            print(metric)

            for implementation in implementations:

                if implementation == REFERENCE:
                    continue

                value = pivot.loc[
                    metric
                ].get(
                    implementation,
                    np.nan,
                )

                if pd.isna(value):
                    continue

                relative = (
                    (value - reference)
                    / abs(reference)
                ) * 100

                print(
                    f"  {implementation:<15}: "
                    f"{relative:+.2f}%"
                )


def validate_reports(reports):
    """
    Verifica se os relatórios possuem as mesmas métricas.
    """

    if len(reports) <= 1:
        return

    reference_name = reports[0][0]
    reference_df = reports[0][2]

    reference_metrics = set(
        reference_df["nome_metrica"].dropna().map(
            normalize_name
        )
    )

    print()
    print("=" * 70)
    print("VALIDAÇÃO DOS RELATÓRIOS")
    print("=" * 70)

    all_equal = True

    for implementation, path, df in reports:

        metrics = set(
            df["nome_metrica"].dropna().map(
                normalize_name
            )
        )

        missing = reference_metrics - metrics
        extra = metrics - reference_metrics

        if missing or extra:

            all_equal = False

            print()
            print(
                f"{implementation}: "
                "estrutura diferente"
            )

            if missing:
                print(
                    "  Ausentes:",
                    ", ".join(sorted(missing)),
                )

            if extra:
                print(
                    "  Extras:",
                    ", ".join(sorted(extra)),
                )

    if all_equal:
        print(
            "Todos os relatórios possuem "
            "as mesmas métricas."
        )

def parse_bvh_size_logs():
    """
    Lê os arquivos bvh_size_log.log de cada pasta de algoritmo e extrai
    o total de bytes para BLAS e TLAS.
    """
    data_rows = []

    for folder_name in ALGORITHM_FOLDERS:
        folder_path = ROOT / folder_name
        log_path = folder_path / "bvh_size_log.log"

        if not log_path.is_file():
            continue

        current_type = None
        blas_total_sum = 0
        tlas_total_sum = 0

        with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                line_str = line.strip()
                if line_str.startswith("BLAS:"):
                    current_type = "BLAS"
                elif line_str.startswith("TLAS:"):
                    current_type = "TLAS"
                
                if line_str.startswith("Total:"):
                    match = re.search(r"Total:\s*([\d.]+)\s*bytes", line_str)
                    if match:
                        val = float(match.group(1))
                        if current_type == "BLAS":
                            blas_total_sum += val
                        elif current_type == "TLAS":
                            tlas_total_sum += val

        if blas_total_sum > 0:
            data_rows.append({
                "implementacao": folder_name,
                "tipo_metrica": "BVH_Size",
                "nome_metrica": "BLAS Total Size",
                "unidade": "bytes",
                "quantidade_amostras": 1,
                "media": blas_total_sum,
                "mediana": blas_total_sum,
                "desvio_padrao": 0.0,
                "minimo": blas_total_sum,
                "maximo": blas_total_sum,
                "valor": blas_total_sum,
                "estatistica_utilizada": "soma",
                "unidade_normalizada": "bytes",
                "valor_normalizado": blas_total_sum,
            })

        if tlas_total_sum > 0:
            data_rows.append({
                "implementacao": folder_name,
                "tipo_metrica": "BVH_Size",
                "nome_metrica": "TLAS Total Size",
                "unidade": "bytes",
                "quantidade_amostras": 1,
                "media": tlas_total_sum,
                "mediana": tlas_total_sum,
                "desvio_padrao": 0.0,
                "minimo": tlas_total_sum,
                "maximo": tlas_total_sum,
                "valor": tlas_total_sum,
                "estatistica_utilizada": "soma",
                "unidade_normalizada": "bytes",
                "valor_normalizado": tlas_total_sum,
            })

    return pd.DataFrame(data_rows)


def generate_bvh_size_bar_charts(data):
    """
    Gera dois gráficos de colunas separados (um para BLAS e outro para TLAS)
    com 4 colunas (uma para cada tipo de algoritmo) salvos na pasta de saída.
    """
    bvh_data = data[data["tipo_metrica"] == "BVH_Size"]
    if bvh_data.empty:
        return 0

    graph_dir = OUTPUT_DIR / "graficos" / "bvh_sizes"
    graph_dir.mkdir(parents=True, exist_ok=True)

    generated = 0
    metrics_to_plot = [
        ("BLAS Total Size", "Tamanho Total BLAS (bytes)", "blas_total_size.png"),
        ("TLAS Total Size", "Tamanho Total TLAS (bytes)", "tlas_total_size.png"),
    ]

    for metric_name, y_label, filename in metrics_to_plot:
        subset = bvh_data[bvh_data["nome_metrica"] == metric_name].copy()
        if subset.empty:
            continue

        subset["implementacao"] = pd.Categorical(
            subset["implementacao"], 
            categories=ALGORITHM_FOLDERS, 
            ordered=True
        )
        subset = subset.sort_values("implementacao")

        implementations = subset["implementacao"].astype(str).tolist()
        values = subset["valor"].astype(float).tolist()

        plt.figure(figsize=(8, 5))
        x = np.arange(len(implementations))

        bars = plt.bar(x, values, color=["#4C72B0", "#55A868", "#C44E52", "#8172B3"])
        plt.xticks(x, implementations)
        plt.title(metric_name)
        plt.ylabel(y_label)

        for bar, value in zip(bars, values):
            if not math.isfinite(value):
                continue
            plt.text(
                bar.get_x() + bar.get_width() / 2,
                bar.get_height(),
                f"{value:,.0f}",
                ha="center",
                va="bottom",
                fontsize=9,
            )

        plt.grid(axis="y", alpha=0.25)
        plt.tight_layout()

        output_path = graph_dir / filename
        plt.savefig(output_path, dpi=180, bbox_inches="tight")
        plt.close()
        generated += 1

    return generated


def main():

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    print("=" * 70)
    print("BUSCANDO *_report_analise.xlsx")
    print("=" * 70)

    files = find_report_files()

    if not files:

        print(
            "Nenhum arquivo *_report_analise.xlsx encontrado."
        )

        return 1

    reports = []

    for path in files:

        implementation = implementation_name(path)

        print(
            f"  {implementation:<15} -> "
            f"{path.relative_to(ROOT)}"
        )

        try:

            df = read_report(path)

            reports.append(
                (
                    implementation,
                    path,
                    df,
                )
            )

        except Exception as error:

            print(
                f"  ERRO ao ler {path}: {error}"
            )

    if not reports:
        print(
            "Nenhum relatório pôde ser carregado."
        )

        return 1

    validate_reports(reports)

    data = prepare_data(reports)

    # Adiciona os dados dos logs de BVH na estrutura geral
    bvh_df = parse_bvh_size_logs()
    if not bvh_df.empty:
        data = pd.concat([data, bvh_df], ignore_index=True)

    if data.empty:
        print(
            "Nenhum dado encontrado nos relatórios."
        )

        return 1

    data = add_vulkan_total(data)

    implementations = [
        implementation
        for implementation, _, _ in reports
    ]

    # Remove possíveis duplicações de implementação caso existam
    # múltiplos arquivos na mesma pasta.
    implementations = list(
        dict.fromkeys(implementations)
    )

    # ==============================================================
    # RESUMO
    # ==============================================================

    summary = build_summary(
        data,
        implementations,
    )

    relative = relative_comparison(
        data,
        REFERENCE,
    )

    # ==============================================================
    # EXCEL
    # ==============================================================

    excel_path = create_comparison_excel(
        data,
        summary,
        relative,
    )

    # ==============================================================
    # GRÁFICOS
    # ==============================================================

    print()
    print("=" * 70)
    print("GERANDO GRÁFICOS")
    print("=" * 70)

    generated_main = generate_metric_graphs(
        data,
        MAIN_METRICS,
    )

    generated_categories = generate_category_graphs(
        data,
    )

    generated_all = generate_individual_value_graphs(
        data,
    )

    generated_bvh = generate_bvh_size_bar_charts(data)

    # ==============================================================
    # CSV
    # ==============================================================

    data.to_csv(
        OUTPUT_DIR / "metricas_consolidadas.csv",
        index=False,
        encoding="utf-8-sig",
    )

    summary.to_csv(
        OUTPUT_DIR / "resumo_comparacao.csv",
        index=False,
        encoding="utf-8-sig",
    )

    if not relative.empty:
        relative.to_csv(
            OUTPUT_DIR / "comparacao_relativa.csv",
            index=False,
            encoding="utf-8-sig",
        )

    # ==============================================================
    # TERMINAL
    # ==============================================================

    print_summary(
        data,
        implementations,
    )

    print()
    print("=" * 70)
    print("RESULTADO")
    print("=" * 70)

    print(
        f"Excel: {excel_path}"
    )

    print(
        f"Gráficos principais: {generated_main}"
    )

    print(
        f"Gráficos por categoria: {generated_categories}"
    )

    print(
        f"Gráficos de todas as métricas: {generated_all}"
    )

    print(
        f"Gráficos de tamanho BVH: {generated_bvh}"
    )

    print(
        f"Saída: {OUTPUT_DIR}"
    )

    print()
    print(
        "Boxplots reais não foram gerados porque os "
        "relatórios atuais contêm estatísticas agregadas, "
        "mas não as amostras individuais."
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
