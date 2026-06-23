#include "CameraBufferManager.hpp"
#define GLM_FORCE_RADIANS

void CameraBufferManager::DefaultCameraProvider::fill(
    UniformBufferGlobal& ubg,
    float time,
    const VkExtent2D& extent
) {

    ubg.view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    float aspect = extent.width / float(extent.height);
    ubg.proj = glm::perspective(
        glm::radians(45.0f),
        aspect,
        0.1f,
        10.0f
    );
    ubg.proj[1][1] *= -1;
}

CameraBufferManager::CameraBufferManager(
    VkDevice device,
    BufferManager* bufferManager,
    int max_frames_in_flight
)
: device(device)
{
    //constexpr force definition on compile time
    constexpr VkDeviceSize bufferSize = sizeof(UniformBufferGlobal);

    uniformBuffers.resize(max_frames_in_flight);
    uniformBuffersMemory.resize(max_frames_in_flight);
    uniformBuffersMapped.resize(max_frames_in_flight);

    for (size_t i = 0; i < max_frames_in_flight; i++) {
        bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            uniformBuffers[i]
        );

        bufferManager->allocateBufferMemory(
            uniformBuffers[i],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffersMemory[i]
        );

        vkBindBufferMemory(
            device,
            uniformBuffers[i],
            uniformBuffersMemory[i],
            0
        );

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void CameraBufferManager::update(
    uint32_t currentFrame,
    const UniformBufferGlobal& ubg
) {
    memcpy(uniformBuffersMapped[currentFrame], &ubg, sizeof(UniformBufferGlobal));
}

CameraBufferManager::~CameraBufferManager()
{
    for (size_t i = 0; i < uniformBuffers.size(); ++i)
    {
        if (uniformBuffersMapped[i] != nullptr)
        {
            vkUnmapMemory(device, uniformBuffersMemory[i]);
        }

        if (uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        }

        if (uniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
    }
}
