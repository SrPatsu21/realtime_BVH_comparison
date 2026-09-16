#include "TextureManager.hpp"

#include <stdexcept>

TextureManager::TextureManager(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    BufferManager* bufferManager,
    SamplerManager* samplerManager,
    uint32_t maxTextures
) :
    physicalDevice(physicalDevice),
    device(device),
    bufferManager(bufferManager),
    samplerManager(samplerManager),
    maxTextures(maxTextures)
{
    #ifndef NDEBUG
    if (!bufferManager)
        throw std::invalid_argument("TextureManager: bufferManager is null");

    if (!samplerManager)
        throw std::invalid_argument("TextureManager: samplerManager is null");

    if (maxTextures == 0)
        throw std::invalid_argument("TextureManager: maxTextures must be greater than zero");
    #endif

    textures.reserve(maxTextures);

    defaultWhite =
        TextureFactory::createSolidRGBA8(
            physicalDevice,
            device,
            bufferManager,
            samplerManager,
            VK_FORMAT_R8G8B8A8_SRGB,
            255,
            255,
            255,
            255
        );

    defaultNormal =
        TextureFactory::createSolidRGBA8(
            physicalDevice,
            device,
            bufferManager,
            samplerManager,
            VK_FORMAT_R8G8B8A8_UNORM,
            128,
            128,
            255,
            255
        );

    defaultMetallic =
        TextureFactory::createSolidRGBA8(
            physicalDevice,
            device,
            bufferManager,
            samplerManager,
            VK_FORMAT_R8G8B8A8_UNORM,
            0,
            255,
            0,
            255
        );

    textures.push_back(
        defaultWhite
    );
}

TextureManager::~TextureManager()
{
    textures.clear();
    textureIndices.clear();
}

uint32_t TextureManager::createTexture(
    const std::string& path
)
{
    auto found = textureIndices.find(path);

    if (found != textureIndices.end())
        return found->second;


    if (textures.size() >= maxTextures)
        throw std::runtime_error("TextureManager: maximum number of textures reached");

    TextureAsset asset(path, physicalDevice);

    SamplerManager::SamplerDesc samplerDesc = asset.getRecommendedSamplerDesc();

    VkSampler sampler = samplerManager->getSampler(samplerDesc);

    uint32_t textureIndex = static_cast<uint32_t>(textures.size());

    std::shared_ptr<TextureImage> texture =
        std::make_shared<TextureImage>(
            physicalDevice,
            device,
            bufferManager,
            sampler,
            asset,
            textureIndex,
            &TextureImage::DefaultImageTransitionPolicy::instance()
        );

    textures.push_back(
        std::move(texture)
    );

    textureIndices.emplace(
        path,
        textureIndex
    );

    return textureIndex;
}

const std::shared_ptr<TextureImage>&
TextureManager::getTexture(
    uint32_t textureIndex
) const
{
    return textures[textureIndex];
}

VkImageView TextureManager::getImageView(
    uint32_t textureIndex
) const
{
    return textures[textureIndex]->getImageView();
}