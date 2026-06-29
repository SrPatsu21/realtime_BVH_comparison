#include "AccelerationStructureManager.hpp"

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
    const std::vector<Primitive>& primitives
)
{
    using BLNodeType =
        typename AccelerationStructureManager<
            TLBuilderType,
            BLBuilderType
        >::BLNodeType;

    BLAS<BLNodeType> blas;

    blas.mesh = mesh;

    BLBuilderType::Build(
        blas.accelerationStructure.nodes,
        primitives
    );

    // VulkanBLAS build

    this->blas.emplace_back(
        std::move(blas)
    );

    return static_cast<uint32_t>(
        this->blas.size() - 1
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

    std::vector<BLASInstance> localInstances =
        instances;

    TLBuilderType::Build(
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
    return blas[index];
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
const AccelerationStructure<
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::TLNodeType
>&
AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::getTLAS(
) const
{
    return tlas;
}