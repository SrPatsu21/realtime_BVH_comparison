#include <vector>
#include <cstdint>

template<typename NodeType>
class AS
{
public:

    std::vector<NodeType> nodes;

    uint32_t rootNode = 0;
};