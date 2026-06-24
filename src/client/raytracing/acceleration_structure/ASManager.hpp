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

class ASManager
{
public:

    uint32_t createBLAS(
        const std::vector<Triangle>& triangles
    );

    void buildTLAS();

private:

    std::vector<AS<BVHNode>> m_blas;
    TLAS<BVHNode> m_tlas;
};