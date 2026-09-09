#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>

#include "../../utils/accelerationStructureConfig.hpp"

#include "../primitives/PrimitiveRef.hpp"
#include "../primitives/TLASBuildInput.hpp"
#include "../node/TLASInstance.hpp"

class TLASInstanceBuilder
{
public:

    using NodeType = DefaultTLASNode;
    using BuilderType = DefaultTLASBuilder;

    static void build(
        const std::vector<TLASBuildInput>& inputs,
        const std::vector<uint32_t>& blasIndices,
        std::vector<PrimitiveRef>& primitives,
        std::vector<NodeType>& nodes,
        std::vector<TLASInstance>& instances
    );

private:

    static void createPrimitives(
        const std::vector<TLASBuildInput>& inputs,
        std::vector<PrimitiveRef>& primitives
    );

    static void createInstances(
        const std::vector<TLASBuildInput>& inputs,
        const std::vector<uint32_t>& blasIndices,
        const std::vector<PrimitiveRef>& primitives,
        const std::vector<NodeType>& nodes,
        std::vector<TLASInstance>& instances
    );
};