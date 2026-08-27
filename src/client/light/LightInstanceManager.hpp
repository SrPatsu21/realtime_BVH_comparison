#pragma once

#include <vector>
#include <cstddef>

#include "LightInstance.hpp"
#include "LightData.hpp"
#include "LightInstanceRegistration.hpp"
#include "LightDescriptorManager.hpp"

class LightInstanceManager
{
private:

    std::vector<LightData> lightsData;
    std::vector<LightInstanceRegistration*> registrations;

    LightDescriptorManager* descriptorManager = nullptr;

    std::vector<bool> needUpdate;

public:

    LightInstanceManager(
        VkDevice device,
        BufferManager *bufferManager,
        VkDeviceSize nonCoherentAtomSize,
        uint32_t maxFramesInFlight,
        uint32_t maxLights
    );

    LightInstance* createLight(
        const LightData& data
    );

    bool removeLight(
        LightInstance* light
    );

    void update(
        uint32_t frameIndex
    );

    void markDirty();

    LightData* getLightData( LightInstanceRegistration* registration ) { return &lightsData[registration->indexInVector]; };
    const LightData* getLightData( LightInstanceRegistration* registration ) const { return &lightsData[registration->indexInVector]; };
    std::vector<LightData>& getLightData() { return lightsData; }
    const std::vector<LightData>& getLightData() const { return lightsData; }

    ~LightInstanceManager();
};