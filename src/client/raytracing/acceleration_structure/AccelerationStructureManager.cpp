#include "AccelerationStructureManager.hpp"

template<
    typename TLBuilderType,
    typename BLBuilderType
>
uint32_t AccelerationStructureManager<
    TLBuilderType,
    BLBuilderType
>::createBLAS(
    const std::vector<Triangle>& triangles
)
{
    using BLNodeType =
    typename AccelerationStructureManager<
        TLBuilderType,
        BLBuilderType
    >::BLNodeType;

    AccelerationStructure<BLNodeType> blas;

    std::vector<Triangle> localTriangles = triangles;

    BLBuilderType::Build(
        blas.nodes,
        localTriangles
    );

    m_blas.push_back(
        std::move(blas)
    );

    return static_cast<uint32_t>(
        m_blas.size() - 1
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
    m_tlas.nodes.clear();

    std::vector<BLASInstance> localInstances =
        instances;

    TLBuilderType::Build(
        m_tlas.nodes,
        localInstances
    );
}

template<
    typename TLBuilderType,
    typename BLBuilderType
>
const AccelerationStructure<
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
    return m_blas[index];
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
    return m_tlas;
}