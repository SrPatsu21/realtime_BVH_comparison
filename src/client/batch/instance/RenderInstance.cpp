#include "RenderInstance.hpp"
#include "../RenderInstanceManager.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include "../RenderBatch.hpp"

RenderInstance::RenderInstance(
    const glm::vec3& position,
    const glm::quat& rotation,
    const glm::vec3& scale
) :
    position(position),
    rotation(rotation),
    scale(scale)
{
}

void RenderInstance::addRegistration(
    RenderBatch* batch,
    size_t index
)
{
    registrations.push_back({ batch, index});
}

void RenderInstance::clearRegistrations()
{
    registrations.clear();
}

void RenderInstance::updateModelMatrix()
{
    model =
        glm::translate(glm::mat4(1.0f), position) *
        glm::mat4_cast(rotation) *
        glm::scale(glm::mat4(1.0f), scale);

    for (auto& reg : registrations)
    {
        reg.renderBatch->getinstancesData()[reg.indexInBatch] = model;
    }
}

RenderInstance::~RenderInstance()
{
    for (auto& reg : registrations)
    {
        if (reg.renderBatch)
            reg.renderBatch->removeInstance(this, reg.indexInBatch);
    }
}