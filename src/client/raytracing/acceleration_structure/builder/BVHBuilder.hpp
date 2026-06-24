#pragma once
#include <vector>
#include <cstdint>

template<
    typename NodeType,
    typename PrimitiveType
>
class BVHBuilder
{
public:

    static void Build(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives
    );
    constexpr uint32_t LEAF_SIZE = 4;


private:

    uint32_t BuildRecursive(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );
};