#pragma once

#include <vector>
#include <cstddef>

#include "LightInstance.hpp"
#include "LightData.hpp"
#include "LightInstanceRegistration.hpp"

class LightInstanceManager
{
private:

    std::vector<LightData> lightsData;
    std::vector<LightInstanceRegistration*> registrations;

public:

    LightInstanceManager() = default;

    LightInstance* createLight(
        const LightData& data
    );

    bool removeLight(
        LightInstance* light
    );

    LightData* getLightData( LightInstanceRegistration* registration ) { return &lightsData[registration->indexInVector]; };
    const LightData* getLightData( LightInstanceRegistration* registration ) const { return &lightsData[registration->indexInVector]; };
    std::vector<LightData>& getLightData() { return lightsData; }
    const std::vector<LightData>& getLightData() const { return lightsData; }

    ~LightInstanceManager();
};