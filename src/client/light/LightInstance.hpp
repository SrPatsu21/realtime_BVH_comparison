#pragma once

#include "LightInstanceRegistration.hpp"

class LightInstance
{
private:
    LightInstanceRegistration* registration = nullptr;
public:

    explicit LightInstance(
        LightInstanceRegistration* registration
    )
        : registration(registration)
    {
    }

    LightInstanceRegistration* getRegistration() { return registration; }
    const LightInstanceRegistration* getRegistration() const { return registration; }

};