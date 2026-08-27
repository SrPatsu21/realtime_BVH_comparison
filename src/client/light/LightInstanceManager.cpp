#include "LightInstanceManager.hpp"

#include <utility>

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

    return true;
}

LightInstanceManager::~LightInstanceManager()
{
    for (LightInstanceRegistration* registration : registrations)
    {
        delete registration;
    }

    registrations.clear();
    lightsData.clear();
}