#include <vector>
#include <cstdint>
#include "BVHNode.hpp"
#include "primitives/BLASInstance.hpp"
#include "AccelerationStructure.hpp"
#include "BLAS.hpp"
#include "vulkan/VulkanBLAS.hpp"
#include "../../batch/mesh/Mesh.hpp"

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
        const std::vector<Primitive>& primitives
    );

    const BLAS<BLNodeType>& getBLAS(
        uint32_t index
    ) const;

    const AccelerationStructure<TLNodeType>& getTLAS() const;

private:

    AccelerationStructure<TLNodeType> tlas;
    std::vector<BLAS<BLNodeType>> blas;
};