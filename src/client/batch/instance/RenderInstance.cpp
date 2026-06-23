#include "RenderInstance.hpp"
#include "../RenderBatchManager.hpp"
#include <glm/gtc/matrix_transform.hpp>

RenderInstance::RenderInstance(
    const glm::vec3& position,
    const glm::vec3& rotation,
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
    glm::mat4 model(1.0f);

    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);

    for (auto& reg : registrations)
    {
        reg.batch->getinstancesData()[reg.indexInBatch] = model;
    }
}

RenderInstance::~RenderInstance()
{
    for (auto& reg : registrations)
    {
        if (reg.batch)
            reg.batch->removeInstance(this, reg.indexInBatch);
    }
}