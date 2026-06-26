#pragma once

#include <vector>
#include <cstdint>

template<
    typename NodeType
>
class AccelerationStructure
{
public:

    std::vector<NodeType> nodes;
};
