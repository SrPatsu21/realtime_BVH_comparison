#include "TextureAsset.hpp"

TextureAsset::TextureAsset(
    const std::string& path,
    VkPhysicalDevice physicalDevice
)
{
    loadFromFile(path);
    transcodeIfNeeded(physicalDevice);
    extractAllSubresources();
}

TextureAsset::~TextureAsset()
{
    if (texture)
        ktxTexture_Destroy(ktxTexture(texture));
}

void TextureAsset::loadFromFile(const std::string& path)
{
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(
        path.c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &texture
    );

    if (result != KTX_SUCCESS || !texture)
        throw std::runtime_error("Failed to load KTX2 texture");

    width     = texture->baseWidth;
    height    = texture->baseHeight;
    depth     = texture->baseDepth;
    mipLevels = texture->numLevels;
    layers    = texture->numLayers;
    faces     = texture->numFaces;

    cubemap      = (faces == 6);
    arrayTexture = (layers > 1);
}

void TextureAsset::transcodeIfNeeded(VkPhysicalDevice physicalDevice)
{
    if (!ktxTexture2_NeedsTranscoding(texture))
    {
        vkFormat = static_cast<VkFormat>(texture->vkFormat);
        return;
    }

    KTX_error_code result = ktxTexture2_TranscodeBasis(
        texture,
        KTX_TTF_BC7_RGBA,
        0
    );

    if (result != KTX_SUCCESS)
        throw std::runtime_error("Failed to transcode texture");

    vkFormat = static_cast<VkFormat>(texture->vkFormat);

    if (vkFormat == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Transcode succeeded but vkFormat is undefined");
}

void TextureAsset::extractAllSubresources()
{
    mipData.resize(mipLevels);

    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        MipLevelData& mip = mipData[level];

        mip.width  = std::max(1u, width  >> level);
        mip.height = std::max(1u, height >> level);

        uint32_t subresourceCount = layers * faces;
        mip.subresources.resize(subresourceCount);

        for (uint32_t layer = 0; layer < layers; ++layer)
        {
            for (uint32_t face = 0; face < faces; ++face)
            {
                ktx_size_t offset;

                KTX_error_code result = ktxTexture_GetImageOffset(
                    ktxTexture(texture),
                    level,
                    layer,
                    face,
                    &offset
                );

                if (result != KTX_SUCCESS)
                    throw std::runtime_error("Failed to get subresource offset");

                size_t imageSize =
                    ktxTexture_GetImageSize(ktxTexture(texture), level);

                uint32_t index = layer * faces + face;

                mip.subresources[index].data =
                    texture->pData + offset;

                mip.subresources[index].size =
                    imageSize;
            }
        }
    }
}

SamplerManager::SamplerDesc
TextureAsset::getRecommendedSamplerDesc() const
{
    SamplerManager::SamplerDesc desc{};

    // Cubemap geralmente não deve repetir
    if (cubemap)
    {
        desc.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        desc.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        desc.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    // Textura array normalmente também clamp
    if (arrayTexture)
    {
        desc.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        desc.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        desc.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    }

    // Se só tem 1 mip, evita overkill
    if (mipLevels <= 1)
    {
        desc.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        desc.maxLod = 0.0f;
    }
    else
    {
        desc.maxLod = static_cast<float>(mipLevels);
    }

    return desc;
}