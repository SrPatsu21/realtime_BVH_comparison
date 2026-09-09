#pragma once

#include "../CoreVulkan.hpp"
#include "../swapchain&framebuffer/SwapchainManager.hpp"
#include "UniformBufferGlobal.hpp"
#include "../BufferManager.hpp"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstring>
#include <vector>

/**
 * @brief Manages camera state and per-frame camera uniform buffers.
 *
 * CameraBufferManager owns:
 * - Camera position
 * - Camera rotation (yaw / pitch)
 * - Mouse input
 * - Keyboard input
 * - View matrix
 * - Projection matrix
 * - Per-frame uniform buffers
 *
 * Controls:
 * - W: Move forward
 * - S: Move backward
 * - A: Move left
 * - D: Move right
 * - Space: Move up
 * - Left Ctrl: Move down
 * - Mouse: Rotate camera
 */
class CameraBufferManager
{
private:
    VkDevice device;
    GLFWwindow* window;

    // One uniform buffer per frame-in-flight
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    // Persistently mapped pointers for fast CPU updates
    std::vector<void*> uniformBuffersMapped;

    // -------------------------------------------------------------------------
    // Camera state
    // -------------------------------------------------------------------------

    glm::vec3 position = glm::vec3(2.0f, 2.0f, 2.0f);

    // Rotation in degrees
    float yaw = -135.0f;
    float pitch = -25.0f;

    // Camera movement speed in world units per second
    float movementSpeed = 5.0f;

    // Mouse sensitivity
    float mouseSensitivity = 0.1f;

    // -------------------------------------------------------------------------
    // Mouse state
    // -------------------------------------------------------------------------

    bool firstMouse = true;

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

private:
    /**
     * @brief Returns the camera's forward direction.
     */
    glm::vec3 getForward() const;

    /**
     * @brief Returns the camera's right direction.
     */
    glm::vec3 getRight() const;

    /**
     * @brief Processes mouse input and updates camera rotation.
     */
    void processMouse();

    /**
     * @brief Processes keyboard input and updates camera position.
     *
     * @param deltaTime Time elapsed since the previous frame.
     */
    void processKeyboard(float deltaTime);

    /**
     * @brief Updates the camera state from GLFW input.
     */
    void processInput(float deltaTime);

public:
    /**
     * @brief Creates uniform buffers and initializes the camera.
     *
     * @param device Logical Vulkan device.
     * @param bufferManager BufferManager used for buffer creation/allocation.
     * @param max_frames_in_flight Number of concurrent frames.
     * @param window GLFW window used for camera input.
     */
    CameraBufferManager(
        VkDevice device,
        BufferManager* bufferManager,
        int max_frames_in_flight,
        GLFWwindow* window
    );

    /**
     * @brief Releases all uniform buffers and associated memory.
     */
    ~CameraBufferManager();

    /**
     * @brief Updates camera state and generates the camera UBO.
     *
     * This function:
     * - Reads mouse input
     * - Reads WASD / Space / Ctrl
     * - Updates camera position
     * - Updates camera rotation
     * - Generates view matrix
     * - Generates projection matrix
     * - Uploads the UBO for the current frame
     *
     * @param currentFrame Current frame-in-flight index.
     * @param deltaTime Time elapsed since the previous frame.
     * @param extent Current swapchain extent.
     */
    void updateCamera(
        uint32_t currentFrame,
        float deltaTime,
        const VkExtent2D& extent
    );

    /**
     * @brief Updates the uniform buffer for the given frame.
     *
     * @param currentFrame Frame index.
     * @param ubg Uniform buffer data to upload.
     */
    void update(
        uint32_t currentFrame,
        const UniformBufferGlobal& ubg
    );

    const std::vector<VkBuffer>& getUniformBuffers() const
    {
        return uniformBuffers;
    }

    std::vector<VkDeviceMemory> getUniformBufferMemorys() const
    {
        return uniformBuffersMemory;
    }

    std::vector<void*> getUniformBuffersMapped() const
    {
        return uniformBuffersMapped;
    }

    /**
     * @brief Returns the current camera position.
     */
    const glm::vec3& getPosition() const
    {
        return position;
    }

    /**
     * @brief Returns the current camera yaw.
     */
    float getYaw() const
    {
        return yaw;
    }

    /**
     * @brief Returns the current camera pitch.
     */
    float getPitch() const
    {
        return pitch;
    }

    /**
     * @brief Sets camera movement speed.
     */
    void setMovementSpeed(float speed)
    {
        movementSpeed = speed;
    }

    /**
     * @brief Sets mouse sensitivity.
     */
    void setMouseSensitivity(float sensitivity)
    {
        mouseSensitivity = sensitivity;
    }
};