#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>
#include "../node/TLASInstance.hpp"
#include "../node/BVHNode.hpp"
#include "../primitives/PrimitiveRef.hpp"
#include "../primitives/TLASBuildInput.hpp"

class TLASInstanceBuilder
{
public:

    static void build(
        const std::vector<TLASBuildInput>& inputs,
        const std::vector<uint32_t>& blasIndices,
        std::vector<PrimitiveRef>& primitives,
        std::vector<BVHNode>& nodes,
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
        const std::vector<BVHNode>& nodes,
        std::vector<TLASInstance>& instances
    );
};