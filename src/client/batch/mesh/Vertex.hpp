#pragma once

#include "../../CoreVulkan.hpp"
#include <glm/glm.hpp>
#include <array>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 tangent; // xyz + w (handedness)
    glm::vec2 texCoord;

    Vertex(
        const glm::vec3 pos,
        glm::vec3 normal,
        glm::vec4 tangent,
        glm::vec2 texCoord
    ) :
        pos(pos),
        normal(normal),
        tangent(tangent),
        texCoord(texCoord)
    {}

    // bindingDescription.binding = 0;
    // bindingDescription.stride = sizeof(Vertex);
    // bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    static const VkVertexInputBindingDescription getBindingDescription() {
        static const VkVertexInputBindingDescription bindingDescription{
            0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX
        };
        return bindingDescription;
    }

    //     attributeDescriptions[0].binding = 0;
    //     attributeDescriptions[0].location = 0;
    //     attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    //     attributeDescriptions[0].offset = offsetof(Vertex, pos);
    static const std::array<VkVertexInputAttributeDescription, 4>& getAttributeDescriptions() {
        static const std::array<VkVertexInputAttributeDescription, 4> attributes{{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
            {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)},
            {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)}
        }};
        return attributes;
    }
};