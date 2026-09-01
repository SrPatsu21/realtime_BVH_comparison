#pragma once

#include <vector>

#include "../node/BVHNode.hpp"
#include "../primitives/PrimitiveRef.hpp"
#include "../node/TLASInstance.hpp"
#include "TLASBuildInput.hpp"

class TLASInstanceBuilder
{
public:

    static void build(
        const std::vector<TLASBuildInput>& inputs,
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
        const std::vector<PrimitiveRef>& primitives,
        const std::vector<BVHNode>& nodes,
        std::vector<TLASInstance>& instances
    );
};