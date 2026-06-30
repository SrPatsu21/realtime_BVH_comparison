#include "ResourceManager.hpp"

ResourceManager::ResourceManager(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    BufferManager* bufferManager,
    MaterialDescriptorManager* descriptorManager
) :
    physicalDevice(physicalDevice),
    device(device),
    bufferManager(bufferManager),
    descriptorManager(descriptorManager),
    samplerManager(physicalDevice, device)
{
    accelerationStructureManager = new AccelerationStructureManager<BVHBuilder<BVHNode>, BVHBuilder<BVHNode>>(device);
}

std::shared_ptr<Mesh> ResourceManager::getMesh(
    const std::string& meshPath
) {
    auto it = meshes.find(meshPath);

    if (it != meshes.end())
    {
        if (auto mesh = it->second.lock())
            return mesh;
    }

    auto mesh = std::make_shared<Mesh>(
        meshPath,
        device,
        bufferManager
    );
    meshes[meshPath] = mesh;

    return mesh;
}

std::vector<std::shared_ptr<Material>>
ResourceManager::getMaterialsForMesh(const Mesh& mesh)
{
    std::vector<std::shared_ptr<Material>> result;

    for (const auto& matData : mesh.getMaterials())
    {
        std::string key =
            matData.baseColorPath + "|" +
            matData.normalPath + "|" +
            matData.metallicRoughnessPath;

        std::shared_ptr<Material> material = nullptr;

        auto it = materials.find(key);
        if (it != materials.end())
        {
            material = it->second.lock();
        }

        if (!material)
        {
            std::shared_ptr<TextureImage> baseColorHandle = nullptr;
            std::shared_ptr<TextureImage> normalHandle = nullptr;
            std::shared_ptr<TextureImage> mrHandle = nullptr;

            if (!matData.baseColorPath.empty())
                baseColorHandle = getTexture(matData.baseColorPath);

            if (!matData.normalPath.empty())
                normalHandle = getTexture(matData.normalPath);

            if (!matData.metallicRoughnessPath.empty())
                mrHandle = getTexture(matData.metallicRoughnessPath);

            material = std::make_shared<Material>(
                device,
                descriptorManager,
                baseColorHandle,
                normalHandle,
                mrHandle
            );

            materials[key] = material;
        }

        result.push_back(material);
    }

    return result;
}

std::shared_ptr<Material>
ResourceManager::getMaterialForSubMesh(
    const Mesh& mesh,
    const Mesh::SubMesh& subMesh
)
{
    const auto& materialsData = mesh.getMaterials();

    if (subMesh.materialIndex >= materialsData.size())
        throw std::runtime_error("Invalid material index in SubMesh");

    const auto& matData = materialsData[subMesh.materialIndex];

    std::string key =
        matData.baseColorPath + "|" +
        matData.normalPath + "|" +
        matData.metallicRoughnessPath;

    std::shared_ptr<Material> material = nullptr;

    auto it = materials.find(key);
    if (it != materials.end())
        material = it->second.lock();

    if (!material)
    {
        std::shared_ptr<TextureImage> baseColorHandle = nullptr;
        std::shared_ptr<TextureImage> normalHandle = nullptr;
        std::shared_ptr<TextureImage> mrHandle = nullptr;

        if (!matData.baseColorPath.empty())
            baseColorHandle = getTexture(matData.baseColorPath);

        if (!matData.normalPath.empty())
            normalHandle = getTexture(matData.normalPath);

        if (!matData.metallicRoughnessPath.empty())
            mrHandle = getTexture(matData.metallicRoughnessPath);

        material = std::make_shared<Material>(
            device,
            descriptorManager,
            baseColorHandle,
            normalHandle,
            mrHandle
        );

        materials[key] = material;
    }

    return material;
}

std::shared_ptr<TextureImage>
ResourceManager::getTexture(const std::string& path)
{
    auto it = textures.find(path);

    if (it != textures.end())
    {
        if (auto handle = it->second.lock())
            return handle;
    }

    TextureAsset asset(path, physicalDevice);

    std::shared_ptr<TextureImage> textureImage = std::make_unique<TextureImage>(
        physicalDevice,
        device,
        bufferManager,
        samplerManager.getSampler(asset.getRecommendedSamplerDesc()),
        asset,
        &TextureImage::DefaultImageTransitionPolicy::instance()
    );

    textures[path] = textureImage;

    return textureImage;
}

uint32_t
ResourceManager::getAccelerationStructureIndex(
    const Mesh* mesh
)
{
    auto it = accelerationStructuresIndex.find(mesh);

    if (it != accelerationStructuresIndex.end())
    {
        return it->second;
    }
    std::vector<PrimitiveRef> primitives;

    buildPrimitiveRefs(
        *mesh,
        primitives
    );

    uint32_t accelerationStructureIndex = accelerationStructureManager->createBLAS(mesh, primitives, bufferManager);

    accelerationStructuresIndex[mesh] = accelerationStructureIndex;

    return accelerationStructureIndex;
}

void ResourceManager::buildPrimitiveRefs(
    const Mesh& mesh,
    std::vector<PrimitiveRef>& primitives
)
{
    primitives.clear();

    const auto& vertices = mesh.getVertices();
    const auto& indices = mesh.getIndices();

    primitives.reserve(indices.size() / 3);

    for (uint32_t i = 0; i < indices.size(); i += 3)
    {
        const glm::vec3& v0 = vertices[indices[i + 0]].pos;
        const glm::vec3& v1 = vertices[indices[i + 1]].pos;
        const glm::vec3& v2 = vertices[indices[i + 2]].pos;

        PrimitiveRef primitive{};

        primitive.triangleIndex = i / 3;

        primitive.bounds.reset();
        primitive.bounds.expand(v0);
        primitive.bounds.expand(v1);
        primitive.bounds.expand(v2);

        primitives.emplace_back(std::move(primitive));
    }
}

ResourceManager::~ResourceManager(){
    delete accelerationStructureManager;
}
