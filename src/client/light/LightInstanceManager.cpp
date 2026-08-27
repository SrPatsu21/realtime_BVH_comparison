#include "LightInstanceManager.hpp"

#include <utility>

LightInstanceManager::LightInstanceManager(
    VkDevice device,
    BufferManager *bufferManager,
    VkDeviceSize nonCoherentAtomSize,
    uint32_t maxFramesInFlight,
    uint32_t maxLights
)
{
    descriptorManager = new LightDescriptorManager(
        device,
        bufferManager,
        nonCoherentAtomSize,
        maxFramesInFlight,
        maxLights
    );

    needUpdate.resize(maxFramesInFlight);
    for (size_t i = 0; i < needUpdate.size(); i++)
        needUpdate[i] = true;

}

LightInstance*
LightInstanceManager::createLight(
    const LightData& data
)
{
    const uint32_t index = static_cast<uint32_t>(lightsData.size());

    LightInstanceRegistration* registration =
        new LightInstanceRegistration{
            index
        };

    lightsData.emplace_back(data);
    registrations.emplace_back(registration);

    markDirty();

    return new LightInstance(registration);
}

bool LightInstanceManager::removeLight(
    LightInstance* light
)
{
    if (!light)
        return false;

    LightInstanceRegistration* registration = light->getRegistration();

    if (!registration)
        return false;

    const size_t index = registration->indexInVector;

    if (index >= lightsData.size())
        return false;

    const size_t last = lightsData.size() - 1;

    if (index != last)
    {
        lightsData[index] = std::move(lightsData[last]);
        registrations[index] = registrations[last];
        registrations[index]->indexInVector = static_cast<uint32_t>(index);
    }

    delete registrations[last];

    registrations.pop_back();
    lightsData.pop_back();

    delete light;

    markDirty();

    return true;
}

void LightInstanceManager::update(
    uint32_t frameIndex
)
{
#ifndef NDEBUG
    if (frameIndex >= needUpdate.size())
        throw std::runtime_error("update fail! frameIndex >= needUpdate.size()");
#endif

    if (!needUpdate[frameIndex])
        return;

    descriptorManager->update(
        frameIndex,
        lightsData
    );

    needUpdate[frameIndex] = false;
}

void LightInstanceManager::markDirty()
{
    for (size_t i = 0; i < needUpdate.size(); i++)
        needUpdate[i] = true;
}

LightInstanceManager::~LightInstanceManager()
{
    for (LightInstanceRegistration* registration : registrations)
    {
        delete registration;
    }

    registrations.clear();
    lightsData.clear();
    needUpdate.clear();

    delete descriptorManager;
}