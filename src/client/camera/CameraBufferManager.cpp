#include "CameraBufferManager.hpp"

#define GLM_FORCE_RADIANS

// =============================================================================
// Camera direction
// =============================================================================

glm::vec3 CameraBufferManager::getForward() const
{
    glm::vec3 forward;

    forward.x =
        cos(glm::radians(yaw)) *
        cos(glm::radians(pitch));

    forward.y =
        sin(glm::radians(pitch));

    forward.z =
        sin(glm::radians(yaw)) *
        cos(glm::radians(pitch));

    return glm::normalize(forward);
}

glm::vec3 CameraBufferManager::getRight() const
{
    return glm::normalize(
        glm::cross(
            getForward(),
            glm::vec3(0.0f, 1.0f, 0.0f)
        )
    );
}

// =============================================================================
// Mouse input
// =============================================================================

void CameraBufferManager::processMouse()
{
    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );

    // First frame after creating/enabling the camera.
    // Prevents the camera from jumping to the current cursor position.
    if (firstMouse)
    {
        lastMouseX = mouseX;
        lastMouseY = mouseY;

        firstMouse = false;

        return;
    }

    double xOffset = mouseX - lastMouseX;
    double yOffset = mouseY - lastMouseY;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    yaw += static_cast<float>(
        xOffset * mouseSensitivity
    );

    pitch -= static_cast<float>(
        yOffset * mouseSensitivity
    );

    // Prevent camera from flipping upside down.
    if (pitch > 89.0f)
        pitch = 89.0f;

    if (pitch < -89.0f)
        pitch = -89.0f;
}

// =============================================================================
// Keyboard input
// =============================================================================

void CameraBufferManager::processKeyboard(float deltaTime)
{
    glm::vec3 forward = getForward();

    // -------------------------------------------------------------------------
    // Important:
    //
    // WASD movement is restricted to the XZ plane.
    //
    // This means looking up/down with the mouse does NOT make W/S move
    // vertically.
    // -------------------------------------------------------------------------

    glm::vec3 flatForward(
        forward.x,
        0.0f,
        forward.z
    );

    // Avoid normalize(0,0,0) if looking exactly straight up/down.
    if (glm::length(flatForward) > 0.0001f)
    {
        flatForward = glm::normalize(flatForward);
    }
    else
    {
        flatForward = glm::vec3(
            0.0f,
            0.0f,
            -1.0f
        );
    }

    glm::vec3 right = getRight();

    glm::vec3 movement(0.0f);

    // -------------------------------------------------------------------------
    // XZ movement
    // -------------------------------------------------------------------------

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        movement += flatForward;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        movement -= flatForward;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        movement -= right;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        movement += right;
    }

    // -------------------------------------------------------------------------
    // Vertical movement
    // -------------------------------------------------------------------------

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        movement.y += 1.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        movement.y -= 1.0f;
    }

    // -------------------------------------------------------------------------
    // Normalize movement
    //
    // Without this, pressing W+D would move faster than pressing only W.
    // -------------------------------------------------------------------------

    if (glm::length(movement) > 0.0f)
    {
        movement = glm::normalize(movement);

        position +=
            movement *
            movementSpeed *
            deltaTime;
    }
}

// =============================================================================
// Input
// =============================================================================

void CameraBufferManager::processInput(float deltaTime)
{
    processMouse();
    processKeyboard(deltaTime);
}

// =============================================================================
// Camera update
// =============================================================================

void CameraBufferManager::updateCamera(
    uint32_t currentFrame,
    float deltaTime,
    const VkExtent2D& extent
)
{
    processInput(deltaTime);

    UniformBufferGlobal ubg{};

    // -------------------------------------------------------------------------
    // View matrix
    // -------------------------------------------------------------------------

    glm::vec3 forward = getForward();

    ubg.view = glm::lookAt(
        position,
        position + forward,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // -------------------------------------------------------------------------
    // Projection matrix
    // -------------------------------------------------------------------------

    float aspect =
        extent.width /
        static_cast<float>(extent.height);

    ubg.proj = glm::perspective(
        glm::radians(45.0f),
        aspect,
        0.1f,
        100.0f
    );

    // Vulkan Y coordinate correction.
    ubg.proj[1][1] *= -1.0f;

    // -------------------------------------------------------------------------
    // Upload to GPU
    // -------------------------------------------------------------------------

    update(
        currentFrame,
        ubg
    );
}

// =============================================================================
// Constructor
// =============================================================================

CameraBufferManager::CameraBufferManager(
    VkDevice device,
    BufferManager* bufferManager,
    int max_frames_in_flight,
    GLFWwindow* window
)
    : device(device),
      window(window)
{
    if (window == nullptr)
    {
        throw std::runtime_error(
            "CameraBufferManager received a null GLFWwindow"
        );
    }

    if (bufferManager == nullptr)
    {
        throw std::runtime_error(
            "CameraBufferManager received a null BufferManager"
        );
    }

    // Force cursor to be captured by the window.
    //
    // GLFW_CURSOR_DISABLED:
    // - hides the cursor
    // - locks it to the window
    // - allows infinite mouse movement
    glfwSetInputMode(
        window,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    // Prevent the first mouse update from causing a camera jump.
    double mouseX;
    double mouseY;

    glfwGetCursorPos(
        window,
        &mouseX,
        &mouseY
    );

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    // -------------------------------------------------------------------------
    // Uniform buffers
    // -------------------------------------------------------------------------

    constexpr VkDeviceSize bufferSize =
        sizeof(UniformBufferGlobal);

    uniformBuffers.resize(
        max_frames_in_flight
    );

    uniformBuffersMemory.resize(
        max_frames_in_flight
    );

    uniformBuffersMapped.resize(
        max_frames_in_flight,
        nullptr
    );

    for (size_t i = 0; i < uniformBuffers.size(); ++i)
    {
        bufferManager->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            uniformBuffers[i]
        );

        bufferManager->allocateBufferMemory(
            uniformBuffers[i],
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            uniformBuffersMemory[i]
        );

        VkResult result = vkBindBufferMemory(
            device,
            uniformBuffers[i],
            uniformBuffersMemory[i],
            0
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(
                "Failed to bind camera uniform buffer memory"
            );
        }

        result = vkMapMemory(
            device,
            uniformBuffersMemory[i],
            0,
            bufferSize,
            0,
            &uniformBuffersMapped[i]
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error(
                "Failed to map camera uniform buffer memory"
            );
        }
    }
}

// =============================================================================
// Update UBO
// =============================================================================

void CameraBufferManager::update(
    uint32_t currentFrame,
    const UniformBufferGlobal& ubg
)
{
    memcpy(
        uniformBuffersMapped[currentFrame],
        &ubg,
        sizeof(UniformBufferGlobal)
    );
}

// =============================================================================
// Destructor
// =============================================================================

CameraBufferManager::~CameraBufferManager()
{
    for (size_t i = 0; i < uniformBuffers.size(); ++i)
    {
        if (uniformBuffersMapped[i] != nullptr)
        {
            vkUnmapMemory(
                device,
                uniformBuffersMemory[i]
            );

            uniformBuffersMapped[i] = nullptr;
        }

        if (uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(
                device,
                uniformBuffers[i],
                nullptr
            );

            uniformBuffers[i] = VK_NULL_HANDLE;
        }

        if (uniformBuffersMemory[i] != VK_NULL_HANDLE)
        {
            vkFreeMemory(
                device,
                uniformBuffersMemory[i],
                nullptr
            );

            uniformBuffersMemory[i] = VK_NULL_HANDLE;
        }
    }
}