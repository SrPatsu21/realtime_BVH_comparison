#include "Render.hpp"
#include <chrono>

TextureImage::DefaultTextures Render::defaultTextures =
{
    nullptr,
    nullptr,
    nullptr
};

Render::Render(){};

int Render::run(){
    //GLFW things
    initWindow();

    // Basic all vulkan setup
    initVulkan();

    // The 3D objects
    initInstances();


    //main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    //free memory (secure)
    cleanup();
    return 0;
};

void Render::initWindow(){
    if (!glfwInit()) {
        throw std::runtime_error("Failed to init GLFW");
    }

    // no OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // block resize
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // screen config
    this->window = glfwCreateWindow(this->width, this->height, "Apotheosis", nullptr, nullptr);
    glfwSetWindowUserPointer(this->window, this);
    glfwSetFramebufferSizeCallback(this->window, framebufferResizeCallback);
};

void Render::initVulkan(){
    //* Core Vulkan
    //Create Vulkan
    coreVulkan = new CoreVulkan(
        window,
        {},
        {},
        {},
        {}
    );

    bufferManager = new BufferManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        coreVulkan->getGraphicsQueue(),
        coreVulkan->getGraphicsQueueFamilyIndices().graphicsFamily.value()
    );

    // Create swapchain
    swapchainManager = new SwapchainManager(
        coreVulkan->getDevice(),
        coreVulkan->getGraphicsQueueFamilyIndices(),
        coreVulkan->getSwapchainSupportDetails(),
        coreVulkan->getSurface(),
        window,
        {}
    );

    // Create render pass
    renderPass = new RenderPass(
        coreVulkan->getDevice(),
        swapchainManager->getImageFormat(),
        coreVulkan->getMsaaSamples(),
        coreVulkan->getDepthFormat(),
        {}
    );

    // Create camera buff with uniformBuffer
    iCameraProvider = new CameraBufferManager::DefaultCameraProvider();

    cameraBufferManager = new CameraBufferManager(
        coreVulkan->getDevice(),
        bufferManager,
        Render::MAX_FRAMES_IN_FLIGHT
    );

    //Multisampling implementation
    imageColor = new ImageColor(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        swapchainManager->getImageFormat(),
        swapchainManager->getExtent(),
        coreVulkan->getMsaaSamples()
    );

    samplerManagerForStaticTextures = new SamplerManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice()
    );


    Render::defaultTextures.white = TextureFactory::createSolidRGBA8(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        bufferManager,
        samplerManagerForStaticTextures,
        VK_FORMAT_R8G8B8A8_SRGB,
        255, 255, 255, 255
    );

    Render::defaultTextures.normal = TextureFactory::createSolidRGBA8(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        bufferManager,
        samplerManagerForStaticTextures,
        VK_FORMAT_R8G8B8A8_UNORM,
        128, 128, 255, 255
    );

    Render::defaultTextures.metallic = TextureFactory::createSolidRGBA8(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        bufferManager,
        samplerManagerForStaticTextures,
        VK_FORMAT_R8G8B8A8_UNORM,
        0, 255, 0, 255
    );

    //Create DepthResources
    depthBufferManager = new DepthBufferManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        swapchainManager->getExtent(),
        coreVulkan->getMsaaSamples(),
        coreVulkan->getDepthFormat(),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    //Create framebuffers
    framebufferManager = new FramebufferManager(
        coreVulkan->getDevice(),
        renderPass->get(),
        swapchainManager->getImageViews(),
        imageColor->getColorImageView(),
        depthBufferManager->getDepthImageView(),
        swapchainManager->getExtent(),
        (coreVulkan->getMsaaSamples() != VK_SAMPLE_COUNT_1_BIT)
    );

    // create semaphore and fence
    createSyncObjects();
    initImagesInFlight(
        this->swapchainManager->getImages().size()
    );

    // Create command
    commandManager = new CommandManager(
        coreVulkan->getDevice(),
        coreVulkan->getGraphicsQueueFamilyIndices().graphicsFamily.value(),
        this->framebufferManager->getFramebuffers()
    );

    // Create descript
    globalDescriptorManager = new GlobalDescriptorManager(
        coreVulkan->getDevice(),
        this->cameraBufferManager,
        Render::MAX_FRAMES_IN_FLIGHT
    );

    materialDescriptorManager = new MaterialDescriptorManager(
        coreVulkan->getDevice(),
        maxMaterials,
        {}
    );

    instanceDescriptorManager = new InstanceDescriptorManager(
        coreVulkan->getDevice(),
        bufferManager,
        coreVulkan->getAtomSize(),
        Render::MAX_FRAMES_IN_FLIGHT,
        maxInstances
    );

    particleInstanceDescriptorManager = new ParticleInstanceDescriptorManager(
        coreVulkan->getDevice(),
        bufferManager,
        coreVulkan->getAtomSize(),
        Render::MAX_FRAMES_IN_FLIGHT,
        maxInstances
    );

    // Create graphics pipeline
    graphicsPipeline = new GraphicsPipeline(
        coreVulkan->getDevice(),
        swapchainManager->getExtent(),
        renderPass->get(),
        globalDescriptorManager->getLayout(),
        materialDescriptorManager->getLayout(),
        instanceDescriptorManager->getLayout(),
        particleInstanceDescriptorManager->getLayout(),
        coreVulkan->getMsaaSamples(),
        coreVulkan->getSupportedFeatures12()
    );

    #ifndef NDEBUG
        // VkPhysicalDeviceProperties deviceProperties;
        // vkGetPhysicalDeviceProperties(CoreVulkan::getPhysicalDevice(), &deviceProperties);

        // std::cout << "Push Constant Max Size: " << deviceProperties.limits.maxPushConstantsSize << " bytes\n";
    #endif

    vkDeviceWaitIdle(coreVulkan->getDevice());
};

void Render::initInstances(){

    resourceManager = new ResourceManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        bufferManager,
        materialDescriptorManager
    );

    renderInstanceManager = new RenderInstanceManager(resourceManager);

    renderInstance = new RenderInstance();
    renderInstanceManager->addInstance(
        resourceManager->getMesh("models/Maxwell/Untitled.gltf"),
        renderInstance
    );
    renderInstance->scale = glm::vec3(0.2f);
    renderInstance->position += glm::vec3(1.5f, 0, 0);

}

void Render::drawFrame(){
    double time = glfwGetTime();
    // double deltaTime = time - lastTime;
    // double lastTime = currentTime;

    // Wait for this frame to be free
    vkWaitForFences(coreVulkan->getDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult next_img_result = vkAcquireNextImageKHR(coreVulkan->getDevice(), this->swapchainManager->getSwapchain(),
            UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (next_img_result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (next_img_result != VK_SUCCESS && next_img_result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // If this swapchain image is already in flight, wait for the fence that owns it
    if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(coreVulkan->getDevice(), 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark this image as now owned by the current frame's fence
    this->imagesInFlight[imageIndex] = this->inFlightFences[this->currentFrame];

    // Reset the fence for the current frame
    vkResetFences(coreVulkan->getDevice(), 1, &this->inFlightFences[this->currentFrame]);

    // Update UBOs for this frame
    UniformBufferGlobal ubg{};
    iCameraProvider->fill(
        ubg,
        time,
        swapchainManager->getExtent()
    );
    this->cameraBufferManager->update(currentFrame, ubg);
    renderInstance->rotation = glm::vec3(
        0.5* time,
        0.3,
        0.6
    );
    renderInstance->updateModelMatrix();

    // platicles
    float timetester = (time * 5);
    float phaseA = sin(timetester);
    float phaseB = sin(timetester + 2.094395f);  // 120°
    float phaseC = sin(timetester + 4.18879f);   // 240°

    ParticleData particle{};
    ParticleData particle1{};

    particle.positionSize = glm::vec4(
        0.6f * phaseA,
        0.6f  * phaseB,
        0.6f  * phaseC,
        60 // (timetester*60) + 10.0f
    );
    particle1.positionSize = glm::vec4(
        -0.6f * phaseA,
        -0.6f  * phaseB,
        -0.6f  * phaseC,
        60 // (timetester*60) + 10.0f
    );

    particle.color = glm::vec4(
        (phaseA + 1.0f) * 0.5f,
        (phaseB + 1.0f) * 0.5f,
        (phaseC + 1.0f) * 0.5f,
        1.0f
    );
    particle1.color = glm::vec4(
        (phaseA + 1.0f) * 0.5f,
        (phaseB + 1.0f) * 0.5f,
        (phaseC + 1.0f) * 0.5f,
        1.0f
    );

    // Reset + record only the command buffer for this swapchain image
    VkCommandBuffer cmd = this->commandManager->getCommandBuffers()[imageIndex];
    vkResetCommandBuffer(cmd, 0);
    this->commandManager->recordCommandBuffer(
        imageIndex,
        currentFrame,
        this->renderPass->get(),
        this->graphicsPipeline,
        this->framebufferManager->getFramebuffers(),
        this->swapchainManager->getExtent(),
        globalDescriptorManager,
        instanceDescriptorManager,
        particleInstanceDescriptorManager,
        renderInstanceManager,
        {particle, particle1},
        // {},
        {},
        {},
        {},
        {}
    );

    // --- Submit work ---
    VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[this->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(coreVulkan->getGraphicsQueue(), 1, &submitInfo, this->inFlightFences[this->currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // Present image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = { this->swapchainManager->getSwapchain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(coreVulkan->getPresentQueue(), &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR  || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Advance to next frame slot
    this->currentFrame = (this->currentFrame + 1) % Render::MAX_FRAMES_IN_FLIGHT;
}

void Render::cleanup(){
    if (coreVulkan)
    {
        // 1) Stop the GPU first so nothing is in flight.
        if (coreVulkan->getDevice() != VK_NULL_HANDLE) {
            vkQueueWaitIdle(coreVulkan->getPresentQueue());
            vkDeviceWaitIdle(coreVulkan->getDevice());
        }

        // 2) Per-frame sync primitives.
        for (VkSemaphore s : this->renderFinishedSemaphores)
            if (s != VK_NULL_HANDLE) vkDestroySemaphore(coreVulkan->getDevice(), s, nullptr);
        this->renderFinishedSemaphores.clear(); this->renderFinishedSemaphores.shrink_to_fit();

        for (VkSemaphore s : this->imageAvailableSemaphores)
            if (s != VK_NULL_HANDLE) vkDestroySemaphore(coreVulkan->getDevice(), s, nullptr);
        this->imageAvailableSemaphores.clear(); this->imageAvailableSemaphores.shrink_to_fit();

        for (VkFence f : this->inFlightFences)
            if (f != VK_NULL_HANDLE) vkDestroyFence(coreVulkan->getDevice(), f, nullptr);
        this->inFlightFences.clear(); this->inFlightFences.shrink_to_fit();

        // 3) Managers: destroy in strict reverse-creation order.
        //    (Everything that depends on the swapchain must go BEFORE swapchain.)
        //    Delete pointers and null them to avoid accidental double free later.
        if (renderInstance ){ delete renderInstance; renderInstance = nullptr; }
        if (renderInstanceManager){ delete renderInstanceManager; renderInstanceManager = nullptr; }
        if (samplerManagerForStaticTextures) { delete samplerManagerForStaticTextures; samplerManagerForStaticTextures = nullptr; }
        if (defaultTextures.metallic)
        {
            defaultTextures.metallic.reset();
            defaultTextures.normal.reset();
            defaultTextures.white.reset();
        }
        if ( resourceManager ){ delete resourceManager; resourceManager = nullptr; }
        if (this->commandManager){ delete this->commandManager; this->commandManager = nullptr; }
        if (this->framebufferManager){ delete this->framebufferManager; this->framebufferManager = nullptr; }
        if (this->imageColor){ delete this->imageColor; this->imageColor = nullptr; }
        if (this->depthBufferManager){ delete this->depthBufferManager; this->depthBufferManager = nullptr; }
        if (this->graphicsPipeline){ delete this->graphicsPipeline; this->graphicsPipeline = nullptr; }
        if (globalDescriptorManager){ delete globalDescriptorManager; globalDescriptorManager = nullptr; }
        if (materialDescriptorManager){ delete materialDescriptorManager; materialDescriptorManager = nullptr; }
        if (instanceDescriptorManager){ delete instanceDescriptorManager; instanceDescriptorManager = nullptr; }
        if (particleInstanceDescriptorManager){ delete particleInstanceDescriptorManager; particleInstanceDescriptorManager = nullptr; }
        if (iCameraProvider){ delete iCameraProvider; iCameraProvider = nullptr; }
        if (this->cameraBufferManager){ delete this->cameraBufferManager; this->cameraBufferManager = nullptr; }
        if (this->renderPass){ delete this->renderPass; this->renderPass = nullptr; }
        if ( bufferManager ){ delete bufferManager; bufferManager = nullptr; }

        // Swapchain and resources that own VkSwapchainKHR should be last among managers.
        if (this->swapchainManager)
        {
            delete this->swapchainManager;
            this->swapchainManager = nullptr;
        }

        // 4) Vulkan core teardown (device, surface, instance, debug messenger, etc.).
        //    Ensure CoreVulkan::destroy() destroys in the order:
        //    - vkDeviceWaitIdle (if not already) -> vkDestroyDevice
        //    - vkDestroySurfaceKHR
        //    - vkDestroyInstance
        //    - Destroy debug messenger (if you use one) before vkDestroyInstance.
        if (this->coreVulkan) {
            delete coreVulkan;
            this->coreVulkan = nullptr;
        }
    }

    // 5) Windowing. Destroy the window AFTER you've destroyed the VkSurfaceKHR.
    if (this->window) {
        glfwDestroyWindow(this->window);
        this->window = nullptr;
    }
    glfwTerminate();
}

void Render::createSyncObjects() {
    this->imageAvailableSemaphores.resize(Render::MAX_FRAMES_IN_FLIGHT);
    this->renderFinishedSemaphores.resize(Render::MAX_FRAMES_IN_FLIGHT);
    this->inFlightFences.resize(Render::MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{ };
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{ };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame doesn't block

    VkDevice device = coreVulkan->getDevice();
    for (uint32_t i = 0; i < Render::MAX_FRAMES_IN_FLIGHT; ++i) {
        if (
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS
        ) {
            throw std::runtime_error("failed to create per-frame sync objects!");
        }
    }
}

void Render::initImagesInFlight(uint32_t swapchainImageCount) {
    this->imagesInFlight.assign(swapchainImageCount, VK_NULL_HANDLE);
}

void Render::cleanupSwapChain() {

    // Command Buffers
    vkFreeCommandBuffers(
        coreVulkan->getDevice(),
        commandManager->getCommandPool(),
        static_cast<uint32_t>(commandManager->getCommandBuffers().size()),
        commandManager->getCommandBuffers().data()
    );

    if (this->framebufferManager){ delete this->framebufferManager; this->framebufferManager = nullptr; }
    if (this->imageColor){ delete this->imageColor; this->imageColor = nullptr; }
    if (this->depthBufferManager){ delete this->depthBufferManager; this->depthBufferManager = nullptr; }
    if (this->graphicsPipeline){ delete this->graphicsPipeline; this->graphicsPipeline = nullptr; }
    if (this->renderPass){ delete this->renderPass; this->renderPass = nullptr; }
}

void Render::recreateSwapChain() {
    #ifndef NDEBUG
    std::cout << "swap chain recreated" << std::endl;
    #endif
    vkDeviceWaitIdle(coreVulkan->getDevice());

    int width = 0, height = 0;
    glfwGetFramebufferSize(this->window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(this->window, &width, &height);
        glfwWaitEvents();
    }

    // 1. Destroy old swapchain + dependents
    cleanupSwapChain();

    // 2. Recreate swapchain
    coreVulkan->updateSwapchainDetails();

    this->swapchainManager->recreate(
        coreVulkan->getGraphicsQueueFamilyIndices(),
        coreVulkan->getSwapchainSupportDetails(),
        coreVulkan->getSurface(),
        this->window,
        {}
    );

    // 3. Recreate render pass (might depend on swapchain format)
    this->renderPass = new RenderPass(
        coreVulkan->getDevice(),
        swapchainManager->getImageFormat(),
        coreVulkan->getMsaaSamples(),
        coreVulkan->getDepthFormat(),
        {}
    );

    // 4. Recreate pipeline (depends on render pass + extent)
    graphicsPipeline = new GraphicsPipeline(
        coreVulkan->getDevice(),
        swapchainManager->getExtent(),
        renderPass->get(),
        globalDescriptorManager->getLayout(),
        materialDescriptorManager->getLayout(),
        instanceDescriptorManager->getLayout(),
        particleInstanceDescriptorManager->getLayout(),
        coreVulkan->getMsaaSamples(),
        coreVulkan->getSupportedFeatures12()
    );

    // 5. Recreate Multisampling
    imageColor = new ImageColor(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        swapchainManager->getImageFormat(),
        swapchainManager->getExtent(),
        coreVulkan->getMsaaSamples()
    );

    // 6. Recreate depth buffer
    depthBufferManager = new DepthBufferManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        swapchainManager->getExtent(),
        coreVulkan->getMsaaSamples(),
        coreVulkan->getDepthFormat(),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    // 7. Recreate framebuffers
    framebufferManager = new FramebufferManager(
        coreVulkan->getDevice(),
        renderPass->get(),
        swapchainManager->getImageViews(),
        imageColor->getColorImageView(),
        depthBufferManager->getDepthImageView(),
        swapchainManager->getExtent(),
        (coreVulkan->getMsaaSamples() != VK_SAMPLE_COUNT_1_BIT)
    );

    // 8. Recreate command buffers
    this->commandManager->allocateCommandBuffers(framebufferManager->getFramebuffers());

    // 9. Resize imagesInFlight vector to match new swapchain image count
    initImagesInFlight(this->swapchainManager->getImages().size());

}

void Render::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Render*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

Render::~Render() {
    cleanup();
}