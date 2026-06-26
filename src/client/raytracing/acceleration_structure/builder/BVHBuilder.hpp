#pragma once

#include <vector>
#include <cstdint>

template<typename NodeType>
class BVHBuilder
{
public:

    static constexpr uint32_t LEAF_SIZE = 4;

    template<typename PrimitiveType>
    static void build(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives
    );

private:

    template<typename PrimitiveType>
    static uint32_t buildRecursive(
        std::vector<NodeType>& nodes,
        std::vector<PrimitiveType>& primitives,
        uint32_t begin,
        uint32_t end
    );
};