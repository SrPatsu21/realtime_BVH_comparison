#include <vector>
#include <cstdint>
#include "BVHNode.hpp"
#include "primitives/BLASInstance.hpp"
#include "AccelerationStructure.hpp"
#include "BLAS.hpp"
#include "vulkan/VulkanBLAS.hpp"
#include "../../batch/mesh/Mesh.hpp"
#include "vulkan/BuildVulkanBLAS.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
class AccelerationStructureManager
{
    using TLNodeType = typename TLBuilderType::NodeType;
    using BLNodeType = typename BLBuilderType::NodeType;

public:

    void buildTLAS(
        const std::vector<BLASInstance>& instances
    );

    template<typename Primitive>
    uint32_t createBLAS(
        const Mesh* mesh,
        std::vector<Primitive>& primitives,
        BufferManager* bufferManager
    );

    const BLAS<BLNodeType>& getBLAS(
        uint32_t index
    ) const;

    const AccelerationStructure<TLNodeType>& getTLAS() const;

    AccelerationStructureManager(VkDevice device)
    :
        device(device)
    {
    };

    ~AccelerationStructureManager()
    {
        for (auto& blas : blas_vector)
            blas.destroy(device);

        blas_vector.clear();
    }

private:

    VkDevice device = VK_NULL_HANDLE;
    AccelerationStructure<TLNodeType> tlas;
    std::vector<BLAS<BLNodeType>> blas_vector;
};

template<
    typename TLBuilderType,
    typename BLBuilderType
>
template<typename Primitive>
uint32_t AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createBLAS(
    const Mesh* mesh,
    std::vector<Primitive>& primitives,
    BufferManager* bufferManager
)
{
    using BLNodeType =
        typename AccelerationStructureManager<
            TLBuilderType,
            BLBuilderType
        >::BLNodeType;

    BLAS<BLNodeType> blas;

    blas.mesh = mesh;

    BLBuilderType::build(
        blas.accelerationStructure.nodes,
        primitives
    );

    buildVulkanBLAS(
        bufferManager,
        blas
    );

    this->blas_vector.emplace_back(
        std::move(blas)
    );

    return static_cast<uint32_t>(
        this->blas_vector.size() - 1
    );
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
void AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::buildTLAS(
    const std::vector<BLASInstance>& instances
)
{
    tlas.nodes.clear();

    std::vector<BLASInstance> localInstances = instances;

    TLBuilderType::build(
        tlas.nodes,
        localInstances
    );
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
const BLAS<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::BLNodeType
>&
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::getBLAS(
    uint32_t index
) const {
    return blas_vector[index];
}
