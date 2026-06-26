#include <vector>
#include <cstdint>
#include "BVHNode.hpp"
#include "primitives/BLASInstance.hpp"
#include "primitives/Triangle.hpp"
#include "AccelerationStructure.hpp"

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

    uint32_t createBLAS(
        const std::vector<Triangle>& triangles
    );


    const AccelerationStructure<BLNodeType>& getBLAS(
        uint32_t index
    ) const;

    const AccelerationStructure<TLNodeType>& getTLAS() const;

private:

    std::vector<AccelerationStructure<BLNodeType>> m_blas;
    AccelerationStructure<TLNodeType> m_tlas;
};