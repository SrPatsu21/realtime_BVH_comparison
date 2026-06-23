#include "MaterialLayoutBuilder.hpp"

MaterialLayoutBuilder::MaterialLayoutBuilder(
    uint32_t reservedStart,
    uint32_t reservedEnd
) :
    reservedStart(reservedStart),
    reservedEnd(reservedEnd)
{}

void MaterialLayoutBuilder::addEngineBinding(
    uint32_t binding,
    VkDescriptorType type,
    uint32_t count,
    VkShaderStageFlags stages
) {
    if (usedBindings.count(binding))
        throw std::runtime_error("Engine binding already defined");

    BindingInfo info{};
    info.binding = binding;
    info.type = type;
    info.count = count;
    info.stages = stages;

    bindings.push_back(info);
    usedBindings.insert(binding);

    descriptorCountByType[type] += count;
}

void MaterialLayoutBuilder::addBinding(
    uint32_t binding,
    VkDescriptorType type,
    uint32_t count,
    VkShaderStageFlags stages
) {
    if (binding >= reservedStart && binding <= reservedEnd)
        throw std::runtime_error("Binding is reserved for engine core");

    if (usedBindings.count(binding))
        throw std::runtime_error("Binding already defined");

    BindingInfo info{};
    info.binding = binding;
    info.type = type;
    info.count = count;
    info.stages = stages;

    bindings.push_back(info);
    usedBindings.insert(binding);

    descriptorCountByType[type] += count;
}