#include <vector>
#include <cstdint>
#include "BVHNode.hpp"
#include "primitives/BLASInstance.hpp"
#include "primitives/Triangle.hpp"
#include "AS.hpp"

enum class BVHType
{
    BVH,
    LBVH,
    BVH8,
    BSAH
};

template<
    typename BuilderType,
    typename NodeType = BVHNode
>
class ASManager
{
public:

    uint32_t createBLAS(
        const std::vector<Triangle>& triangles
    );

    void buildTLAS(
        const std::vector<BLASInstance>& instances
    );

    const AS<NodeType>& getBLAS(
        uint32_t index
    ) const;

    const AS<NodeType>& getTLAS() const;

private:

    std::vector<AS<NodeType>> m_blas;
    AS<NodeType> m_tlas;
};