#include "RenderBatch.hpp"

RenderBatch::RenderBatch(
    BatchKey batchKey
)
    : batchKey(batchKey)
{}

RenderBatch::RenderBatch(
    RenderBatch&& other
) noexcept :
    batchKey(std::move(other.batchKey)),
    batchRegistrations(std::move(other.batchRegistrations)),
    instancesData(std::move(other.instancesData))
{}

RenderBatch&
RenderBatch::operator=(
    RenderBatch&& other
) noexcept
{
    if (this != &other)
    {
        batchKey = std::move(other.batchKey);
        batchRegistrations = std::move(other.batchRegistrations);
        instancesData = std::move(other.instancesData);
    }
    return *this;
}

RenderBatch::~RenderBatch() = default;

void RenderBatch::addInstance(
    RenderInstance* instance
)
{
    size_t index = instancesData.size();
    instancesData.emplace_back();
    instance->addRegistration(this, index);

    batchRegistrations.push_back(&instance->registrations.back());
}

void RenderBatch::removeInstance(
    RenderInstance* instance,
    size_t intregistrationsIndex
) {

    std::cout << "removeInstance fault 1" << std::endl;

    auto& reg = instance->registrations[intregistrationsIndex];

    size_t index = reg.indexInBatch;
    size_t lastIndex = batchRegistrations.size() - 1;

    std::cout << "removeInstance fault 2" << std::endl;
    if (index != lastIndex)
    {
        batchRegistrations[index] = batchRegistrations[lastIndex];
        batchRegistrations[index]->indexInBatch = index;

        instancesData[index] = instancesData[lastIndex];
    }

    std::cout << "removeInstance fault 3" << std::endl;
    batchRegistrations.pop_back();
    instancesData.pop_back();

    reg.renderBatch = nullptr;
}

bool RenderBatch::empty()
{
    return batchRegistrations.empty();
}
