#include "ASManager.hpp"

template<
    typename BuilderType,
    typename NodeType
>
uint32_t ASManager<
    BuilderType,
    NodeType
>::createBLAS(
    const std::vector<Triangle>& triangles
)
{
    AS<NodeType> blas;

    std::vector<Triangle> localTriangles =
        triangles;

    BuilderType::Build(
        blas.nodes,
        localTriangles
    );

    blas.rootNode = 0;

    m_blas.push_back(
        std::move(blas)
    );

    return static_cast<uint32_t>(
        m_blas.size() - 1
    );
}

template<
    typename BuilderType,
    typename NodeType
>
void ASManager<
    BuilderType,
    NodeType
>::buildTLAS(
    const std::vector<BLASInstance>& instances
)
{
    m_tlas.nodes.clear();

    std::vector<BLASInstance> localInstances =
        instances;

    BuilderType::Build(
        m_tlas.nodes,
        localInstances
    );

    m_tlas.rootNode = 0;
}

template< typename BuilderType, typename NodeType >
const AS<NodeType>& ASManager< BuilderType, NodeType >::getBLAS(uint32_t index )
const {
    return m_blas[index];
}

template< typename BuilderType, typename NodeType >
const AS<NodeType>& ASManager< BuilderType, NodeType >::getTLAS() const
{
    return m_tlas;
}