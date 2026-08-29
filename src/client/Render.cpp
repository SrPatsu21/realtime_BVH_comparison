#include "Render.hpp"
#include "graphics_pipeline/pipelines/IPipelineProvider.hpp"
#include "raytracing/render_pass/GeometryGBufferRenderPassProvider.hpp"
#include "raytracing/render_pass/LightingRenderPassProvider.hpp"
#include "raytracing/frame_buffer/GBufferFramebufferProvider.hpp"
#include "raytracing/frame_buffer/LightingFramebufferProvider.hpp"
#include "forward_render/ForwardRenderPassProvider.hpp"
#include "forward_render/ForwardFramebufferProvider.hpp"
#include <chrono>

TextureImage::DefaultTextures Render::defaultTextures =
{
    nullptr,
    nullptr,
    nullptr
};

Render::Render(){
    // config.render.mode = Config::RenderMode::GeometryGBuffer;
    config.render.mode = Config::RenderMode::GeometryGBuffer;

    config.lighting.flags =
        Config::ConfigTable::Bit(Config::LightingBits::Shadows) |
        Config::ConfigTable::Bit(Config::LightingBits::RayTracing);
};

int Render::run(){
    //GLFW things
    initWindow();
    // Basic all vulkan setup
    initVulkan();
    // The 3D objects
    initInstances();

    const double targetFPS = 60.0;
    const double targetFrameTime = 1.0 / targetFPS;

    double lastFrameTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        double elapsed = currentTime - lastFrameTime;

        if (elapsed < targetFrameTime) {
            std::this_thread::sleep_for(
                std::chrono::duration<double>(targetFrameTime - elapsed)
            );
        }

        lastFrameTime = glfwGetTime();

        updateInstances(
            currentTime,
            elapsed
        );

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
    createCoreVulkan();

    // Create swapchain
    createSwapchain();

    // Create render pass
    createRenderPasses();

    // Create camera buffer, sampler e default textures
    createCameraAndSamplers();

    // Create framebuffers / gBuffer / gBufferDescriptorManager
    createSwapchainDependentResources();

    // Create sync objects + command manager
    createCommandAndSyncObjects();

    // Create descriptor managers
    createDescriptorManagers();

    // Create graphics pipeline
    createGraphicsPipelineObjects();

    #ifndef NDEBUG
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(
            coreVulkan->getPhysicalDevice(),
            &deviceProperties
        );

        std::cout << "Push Constant Max Size: " << deviceProperties.limits.maxPushConstantsSize << " bytes\n";
    #endif

    vkDeviceWaitIdle(coreVulkan->getDevice());
};

void Render::createCoreVulkan(){
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
}

void Render::createSwapchain(){
    swapchainManager = new SwapchainManager(
        coreVulkan->getDevice(),
        coreVulkan->getGraphicsQueueFamilyIndices(),
        coreVulkan->getSwapchainSupportDetails(),
        coreVulkan->getSurface(),
        window,
        {}
    );
}

void Render::createCameraAndSamplers(){
    // Create camera buff with uniformBuffer
    iCameraProvider = new CameraBufferManager::DefaultCameraProvider();

    cameraBufferManager = new CameraBufferManager(
        coreVulkan->getDevice(),
        bufferManager,
        Render::MAX_FRAMES_IN_FLIGHT
    );

    //MultiSampling implementation
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
}

void Render::createDescriptorManagers(){
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
}

void Render::createCommandAndSyncObjects(){
    // create semaphore and fence
    createSyncObjects();
    initImagesInFlight(
        this->swapchainManager->getImages().size()
    );

    // Create command
    commandManager = new CommandManager(
        coreVulkan->getDevice(),
        coreVulkan->getGraphicsQueueFamilyIndices().graphicsFamily.value(),
        this->framebufferManager->getFramebuffers(),
        this->swapchainManager->getImages().size(),
        4
    );
}

void Render::createRenderPasses(){
    RenderPassManager::Description description1, description2;
    switch (config.render.mode)
    {
        case Config::RenderMode::Forward:
                ForwardRenderPassProvider::build(
                    description1,
                    swapchainManager->getImageFormat(),
                    coreVulkan->getMsaaSamples(),
                    coreVulkan->getDepthFormat()
                );
            renderPassManager = new RenderPassManager(
                coreVulkan->getDevice(),
                std::move(description1)
            );

            lightRenderPassManager = nullptr;

            break;

        case Config::RenderMode::GeometryGBuffer:
            GeometryGBufferRenderPassProvider::build(
                description1,
                coreVulkan->getMsaaSamples(),
                coreVulkan->getDepthFormat()
            );
            renderPassManager = new RenderPassManager(
                coreVulkan->getDevice(),
                std::move(description1)
            );

            LightingRenderPassProvider::build(
                description2,
                swapchainManager->getImageFormat()
            );
            lightRenderPassManager = new RenderPassManager(
                coreVulkan->getDevice(),
                std::move(description2)
            );

            break;
    }
}

void Render::createSwapchainDependentResources(){
    std::vector<std::vector<VkImageView>> attachmentsVector;
    if (config.render.mode == Config::RenderMode::Forward)
    {
        depthBufferManager = new DepthBufferManager(
            coreVulkan->getPhysicalDevice(),
            coreVulkan->getDevice(),
            swapchainManager->getExtent(),
            coreVulkan->getMsaaSamples(),
            coreVulkan->getDepthFormat(),
            VK_IMAGE_ASPECT_DEPTH_BIT
        );

        imageColor = new ImageColor(
            coreVulkan->getPhysicalDevice(),
            coreVulkan->getDevice(),
            swapchainManager->getImageFormat(),
            swapchainManager->getExtent(),
            coreVulkan->getMsaaSamples()
        );

        gBuffer = nullptr;
        gBufferDescriptorManager = nullptr;

        ForwardFramebufferProvider::ForwardAttachments forwardAttachments{
            .color = imageColor->getColorImageView(),
            .depth = depthBufferManager->getDepthImageView()
        };

        ForwardFramebufferProvider::build(
            forwardAttachments,
            swapchainManager->getImageViews(),
            swapchainManager->getImageViews().size(),
            coreVulkan->getMsaaSamples(),
            attachmentsVector
        );

        framebufferManager = new FramebufferManager(
            coreVulkan->getDevice(),
            renderPassManager->get(),
            swapchainManager->getImageViews().size(),
            swapchainManager->getExtent(),
            attachmentsVector
        );

        lightingFramebufferManager = nullptr;

    } else if (config.render.mode == Config::RenderMode::GeometryGBuffer)
    {
        imageColor = nullptr;

        depthBufferManager = nullptr;

        gBuffer = new GBuffer;
        gBuffer->create(
            coreVulkan->getDevice(),
            coreVulkan->getPhysicalDevice(),
            swapchainManager->getExtent(),
            coreVulkan->getDepthFormat(),
            coreVulkan->getMsaaSamples()
        );

        GBufferFramebufferProvider::GBufferAttachments gBufferAttachments{
            .position = gBuffer->getView(GBuffer::Attachment::Position),
            .albedo = gBuffer->getView(GBuffer::Attachment::Albedo),
            .normal = gBuffer->getView(GBuffer::Attachment::Normal),
            .material = gBuffer->getView(GBuffer::Attachment::Material),
            .depth = gBuffer->getView(GBuffer::Attachment::Depth)
        };

        GBufferFramebufferProvider::build(
            gBufferAttachments,
            swapchainManager->getImageViews().size(),
            attachmentsVector
        );

        framebufferManager = new FramebufferManager(
            coreVulkan->getDevice(),
            renderPassManager->get(),
            swapchainManager->getImageViews().size(),
            swapchainManager->getExtent(),
            attachmentsVector
        );

        attachmentsVector.clear();
        LightingFramebufferProvider::build(
            swapchainManager->getImageViews(),
            swapchainManager->getImageViews().size(),
            attachmentsVector
        );

        lightingFramebufferManager = new FramebufferManager(
            coreVulkan->getDevice(),
            lightRenderPassManager->get(),
            swapchainManager->getImageViews().size(),
            swapchainManager->getExtent(),
            attachmentsVector
        );

        gBufferDescriptorManager = new GBufferDescriptorManager(
            coreVulkan->getDevice(),
            gBuffer
        );
    }
}

void Render::createGraphicsPipelineObjects(){
    PipelineCreationContext pipelineContext{
        .device = coreVulkan->getDevice(),
        .renderPass = renderPassManager->get(),
        .msaa = coreVulkan->getMsaaSamples(),
        .supportedFeatures12 = coreVulkan->getSupportedFeatures12(),
        .config = &config
    };
    pipelineContext.globalLayout = globalDescriptorManager->getLayout();
    pipelineContext.materialLayout = materialDescriptorManager->getLayout();
    pipelineContext.instanceLayout = instanceDescriptorManager->getLayout();
    pipelineContext.particleLayout = particleInstanceDescriptorManager->getLayout();

    switch (config.render.mode)
    {
        case Config::RenderMode::Forward:
            pipelineContext.lightRenderPass = VK_NULL_HANDLE;

            pipelineContext.gBufferLayout = VK_NULL_HANDLE;
            pipelineContext.lightingLayout = VK_NULL_HANDLE;
            break;

        case Config::RenderMode::GeometryGBuffer:
            pipelineContext.lightRenderPass = lightRenderPassManager->get();

            pipelineContext.gBufferLayout = gBufferDescriptorManager->getLayout();
            pipelineContext.lightingLayout = VK_NULL_HANDLE;
            break;

        default:
            throw std::runtime_error(
                "Unknown render mode"
            );
    }

    // Create graphics pipeline
    graphicsPipeline = new GraphicsPipelineManager(
        coreVulkan->getDevice(),
        swapchainManager->getExtent(),
        pipelineContext,
        config
    );
}

void Render::initInstances(){

    resourceManager = new ResourceManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        bufferManager,
        materialDescriptorManager
    );

    renderInstanceManager = new RenderInstanceManager(
        resourceManager
    );

    renderInstanceRegistration = renderInstanceManager->createRenderInstance(
        resourceManager->getMesh("models/Maxwell/Untitled.gltf")
    );

    #ifndef NDEBUG
        // test
        // ====================================

        RenderInstanceRegistration* renderInstanceRegistration2 = renderInstanceManager->createRenderInstance(
            resourceManager->getMesh("models/Maxwell/Untitled.gltf")
        );

        RenderInstanceRegistration* renderInstanceRegistration3 = renderInstanceManager->createRenderInstance(
            resourceManager->getMesh("models/Maxwell/Untitled.gltf")
        );

        renderInstanceManager->removeRenderInstance(renderInstanceRegistration);
        renderInstanceManager->removeRenderInstance(renderInstanceRegistration2);
        renderInstanceRegistration = renderInstanceRegistration3;

        // ====================================
        // end test
    #endif

    RenderInstance* renderInstance = renderInstanceManager->getRenderInstance(renderInstanceRegistration->indexInVector);
    renderInstance->scale = glm::vec3(0.2f);
    renderInstance->position += glm::vec3(1.5f, 0, 0);

    //* light
    lightInstanceManager = new LightInstanceManager(
        coreVulkan->getDevice(),
        bufferManager,
        coreVulkan->getAtomSize(),
        Render::MAX_FRAMES_IN_FLIGHT,
        maxInstances
    );
    lightInstanceManager->createLight({
        .position = glm::vec3(0.0f),
        .intensity = 0.5f,
        .color = glm::vec3(1.0f),
        .radius = 0.5f,
        .type = (Config::LightType::Point),
        .range = 5.0f,

        ._pad0 = 0.0f,
        ._pad1 = 0.0f
    });
}

void Render::updateInstances(
    double time,
    double deltaTime
){
    // Update UBOs for this frame
    {
        UniformBufferGlobal ubg{};
        iCameraProvider->fill(
            ubg,
            time,
            swapchainManager->getExtent()
        );
        this->cameraBufferManager->update(currentFrame, ubg);
    }

    // update render instances
    {
        std::vector<RenderInstance>& renderInstances = renderInstanceManager->getRenderInstances();
        std::size_t renderInstancesSize = renderInstances.size();
        for (size_t i = 0; i < renderInstancesSize; i++)
        {
            renderInstances[i].rotation = glm::vec3(
                0.5* time,
                0.3,
                0.6
            );
            renderInstances[i].updateModelMatrix();
        }

        uint32_t currentOffset = 0;
        renderInstanceManager->forEachBatch(
            [&](RenderBatch& batch)
            {
                instanceDescriptorManager->update(
                    currentFrame,
                    currentOffset,
                    batch.getInstancesData()
                );
                currentOffset += static_cast<uint32_t>(batch.getInstancesData().size());
            }
        );
    }

    // particlesData
    {
        float timeTester = (time * 5);
        float phaseA = sin(timeTester);
        float phaseB = sin(timeTester + 2.094395f);  // 120°
        float phaseC = sin(timeTester + 4.18879f);   // 240°

        ParticleData particle{};
        ParticleData particle1{};

        particle.positionSize = glm::vec4(
            0.6f * phaseA,
            0.6f  * phaseB,
            0.6f  * phaseC,
            60 // (timeTester*60) + 10.0f
        );
        particle1.positionSize = glm::vec4(
            -0.6f * phaseA,
            -0.6f  * phaseB,
            -0.6f  * phaseC,
            60 // (timeTester*60) + 10.0f
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

        particlesData.resize(2);
        particlesData[0] = particle;
        particlesData[1] = particle1;
        uint32_t currentOffset = 0;

        particleInstanceDescriptorManager->update(
            currentFrame,
            currentOffset,
            particlesData
        );
    }
    // light
    this->lightInstanceManager->update(currentFrame);
}

void Render::drawFrame(){
    // Wait for this frame to be free
    vkWaitForFences(coreVulkan->getDevice(), 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult next_img_result = vkAcquireNextImageKHR(
        coreVulkan->getDevice(),
        this->swapchainManager->getSwapchain(),
        UINT64_MAX,
        this->imageAvailableSemaphores[this->currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

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

    // Reset + record only the command buffer for this swapchain image
    VkCommandBuffer cmd = this->commandManager->getCommandBuffers()[imageIndex];

    vkResetCommandBuffer(cmd, 0);
    std::vector<VkFramebuffer> auxVkFramebuffers = {};
    if(config.render.mode == Config::RenderMode::Forward){
        this->commandManager->recordCommandBuffer(
            imageIndex,
            currentFrame,
            this->renderPassManager->get(),
            VK_NULL_HANDLE,
            this->graphicsPipeline,
            this->framebufferManager->getFramebuffers(),
            {},
            swapchainManager->getExtent(),
            globalDescriptorManager,
            instanceDescriptorManager,
            particleInstanceDescriptorManager,
            renderInstanceManager,
            gBufferDescriptorManager,
            particlesData,
            {},
            {},
            {},
            {},
            config
        );
    } else
    {
        this->commandManager->recordCommandBuffer(
            imageIndex,
            currentFrame,
            renderPassManager->get(),
            lightRenderPassManager->get(),
            graphicsPipeline,
            framebufferManager->getFramebuffers(),
            lightingFramebufferManager->getFramebuffers(),
            swapchainManager->getExtent(),
            globalDescriptorManager,
            instanceDescriptorManager,
            particleInstanceDescriptorManager,
            renderInstanceManager,
            gBufferDescriptorManager,
            particlesData,
            {},
            {},
            {},
            {},
            config
        );
    }

    // --- Submit work ---
    VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[this->currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[imageIndex] };

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
        if (renderInstanceManager){ delete renderInstanceManager; renderInstanceManager = nullptr; }
        if (lightInstanceManager){ delete lightInstanceManager; lightInstanceManager = nullptr; }

        if (samplerManagerForStaticTextures) { delete samplerManagerForStaticTextures; samplerManagerForStaticTextures = nullptr; }
        if (defaultTextures.metallic)
        {
            defaultTextures.metallic.reset();
            defaultTextures.normal.reset();
            defaultTextures.white.reset();
        }
        if ( resourceManager ){ delete resourceManager; resourceManager = nullptr; }
        if (this->commandManager){ delete this->commandManager; this->commandManager = nullptr; }

        // 4) Swapchain Dependents
        destroySwapchainDependentResources();

        // 5) Not Swapchain Dependents
        if (globalDescriptorManager){ delete globalDescriptorManager; globalDescriptorManager = nullptr; }
        if (materialDescriptorManager){ delete materialDescriptorManager; materialDescriptorManager = nullptr; }
        if (instanceDescriptorManager){ delete instanceDescriptorManager; instanceDescriptorManager = nullptr; }
        if (particleInstanceDescriptorManager){ delete particleInstanceDescriptorManager; particleInstanceDescriptorManager = nullptr; }
        if (iCameraProvider){ delete iCameraProvider; iCameraProvider = nullptr; }
        if (this->cameraBufferManager){ delete this->cameraBufferManager; this->cameraBufferManager = nullptr; }
        if ( bufferManager ){ delete bufferManager; bufferManager = nullptr; }

        // Swapchain and resources that own VkSwapchainKHR should be last among managers.
        if (this->swapchainManager)
        {
            delete this->swapchainManager;
            this->swapchainManager = nullptr;
        }

        // 6) Vulkan core teardown (device, surface, instance, debug messenger, etc.).
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

    // 7) Windowing. Destroy the window AFTER you've destroyed the VkSurfaceKHR.
    if (this->window) {
        glfwDestroyWindow(this->window);
        this->window = nullptr;
    }
    glfwTerminate();
}

void Render::createSyncObjects()
{
    imageAvailableSemaphores.resize(Render::MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(Render::MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(swapchainManager->getImages().size());

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = coreVulkan->getDevice();

    for (uint32_t i = 0; i < Render::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (
            vkCreateSemaphore(
                device,
                &semaphoreInfo,
                nullptr,
                &imageAvailableSemaphores[i]
            ) != VK_SUCCESS ||
            vkCreateFence(
                device,
                &fenceInfo,
                nullptr,
                &inFlightFences[i]
            ) != VK_SUCCESS
        )
        {
            throw std::runtime_error("failed to create frame sync objects!");
        }
    }

    for (uint32_t i = 0; i < renderFinishedSemaphores.size(); ++i)
    {
        if (vkCreateSemaphore(
                device,
                &semaphoreInfo,
                nullptr,
                &renderFinishedSemaphores[i]
            ) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create renderFinished semaphore!");
        }
    }
}

void Render::initImagesInFlight(uint32_t swapchainImageCount) {
    this->imagesInFlight.assign(swapchainImageCount, VK_NULL_HANDLE);
}

void Render::destroySwapchainDependentResources() {

    if (this->framebufferManager){ delete this->framebufferManager; this->framebufferManager = nullptr; }
    if (this->lightingFramebufferManager){ delete this->lightingFramebufferManager; this->lightingFramebufferManager = nullptr; }
    if (this->graphicsPipeline){ delete this->graphicsPipeline; this->graphicsPipeline = nullptr; }
    if (this->imageColor){ delete this->imageColor; this->imageColor = nullptr; }
    if (this->depthBufferManager){ delete this->depthBufferManager; this->depthBufferManager = nullptr; }
    if (this->gBufferDescriptorManager) { delete this->gBufferDescriptorManager; this->gBufferDescriptorManager = nullptr; }
    if (this->gBuffer) {
        gBuffer->destroy(coreVulkan->getDevice());
        delete this->gBuffer;
        this->gBuffer = nullptr;
    }
    if (this->renderPassManager){ delete this->renderPassManager; this->renderPassManager = nullptr; }
    if (this->lightRenderPassManager){ delete this->lightRenderPassManager; this->lightRenderPassManager = nullptr; }

}

void Render::recreateSwapChain()
{
    #ifndef NDEBUG
        std::cout << "swap chain recreated" << std::endl;
    #endif

    vkDeviceWaitIdle(coreVulkan->getDevice());

    int width = 0;
    int height = 0;

    glfwGetFramebufferSize(this->window, &width, &height);


    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(
            this->window,
            &width,
            &height
        );

        glfwWaitEvents();
    }

    // ------------------------------------------------------------
    // 1. Destroy swapchain-dependent resources
    // ------------------------------------------------------------

    vkFreeCommandBuffers(
        coreVulkan->getDevice(),
        commandManager->getCommandPool(),
        static_cast<uint32_t>(commandManager->getCommandBuffers().size()),
        commandManager->getCommandBuffers().data()
    );

    destroySwapchainDependentResources();

    // ------------------------------------------------------------
    // 2. Recreate swapchain
    // ------------------------------------------------------------

    coreVulkan->updateSwapchainDetails();

    this->swapchainManager->recreate(
        coreVulkan->getGraphicsQueueFamilyIndices(),
        coreVulkan->getSwapchainSupportDetails(),
        coreVulkan->getSurface(),
        this->window,
        {}
    );


    createRenderPasses();
    createSwapchainDependentResources();
    createGraphicsPipelineObjects();

    // ------------------------------------------------------------
    // 4. Recreate command buffers
    // ------------------------------------------------------------

    commandManager->allocateCommandBuffers(
        framebufferManager->getFramebuffers()
    );

    // ------------------------------------------------------------
    // 5. Images in flight
    // ------------------------------------------------------------

    initImagesInFlight(
        swapchainManager->getImages().size()
    );
}

void Render::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Render*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

Render::~Render() {
    cleanup();
}