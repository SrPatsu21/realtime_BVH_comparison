#include "Render.hpp"
#include "graphics_pipeline/pipelines/IPipelineProvider.hpp"
#include "raytracing/render_pass/GeometryGBufferRenderPassProvider.hpp"
#include "raytracing/render_pass/TransparentGBufferRenderPassProvider.hpp"
#include "raytracing/render_pass/DeferredLightingRenderPassProvider.hpp"
#include "raytracing/render_pass/CompositeRenderPassProvider.hpp"
#include "raytracing/frame_buffer/GBufferFramebufferProvider.hpp"
#include "raytracing/frame_buffer/DeferredLightingFramebufferProvider.hpp"
#include "raytracing/frame_buffer/CompositeFramebufferProvider.hpp"

#include <chrono>

TextureImage::DefaultTextures Render::defaultTextures =
{
    nullptr,
    nullptr,
    nullptr
};

Render::Render(){

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

    const double targetFPS = 120.0;
    const double targetFrameTime = 1.0 / targetFPS;

    double lastFrameTime = glfwGetTime();
    double fpsTimer = lastFrameTime;

    int frameCount = 0;
    double fps = 0.0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double frameStart = glfwGetTime();
        double elapsed = frameStart - lastFrameTime;

        if (elapsed < targetFrameTime) {
            std::this_thread::sleep_for(
                std::chrono::duration<double>(targetFrameTime - elapsed)
            );
        }

        double currentTime = glfwGetTime();
        elapsed = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        updateInstances(
            currentTime,
            elapsed
        );

        drawFrame();

        frameCount++;

        if (currentTime - fpsTimer >= 1.0) {
            fps = frameCount / (currentTime - fpsTimer);

            std::cout
                << "time: " << currentTime
                << " | Delta: " << elapsed << " s"
                << " | FPS: " << fps
                << std::endl;

            frameCount = 0;
            fpsTimer = currentTime;
        }
    }

    cleanup();
    return 0;
}


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
    cameraBufferManager = new CameraBufferManager(
        coreVulkan->getDevice(),
        bufferManager,
        Render::MAX_FRAMES_IN_FLIGHT,
        window
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

    materialDescriptorManager = new MaterialDescriptorManager(
        coreVulkan->getDevice(),
        maxMaterials,
        {}
    );

    resourceManager = new ResourceManager(
        coreVulkan->getPhysicalDevice(),
        coreVulkan->getDevice(),
        bufferManager,
        materialDescriptorManager
    );

    auto accelerationStructureManager = resourceManager->getAccelerationStructureManager();
    globalDescriptorManager = new GlobalDescriptorManager(
        coreVulkan->getDevice(),
        this->cameraBufferManager,
        accelerationStructureManager->getBLASBuffer(),
        accelerationStructureManager->getBLASInstanceBuffer(),
        accelerationStructureManager->getTLASGPU(),
        accelerationStructureManager->getTLASInstanceGPU(),
        Render::MAX_FRAMES_IN_FLIGHT
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
    lightInstanceManager = new LightInstanceManager(
        coreVulkan->getDevice(),
        bufferManager,
        coreVulkan->getAtomSize(),
        Render::MAX_FRAMES_IN_FLIGHT,
        maxLightInstances
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
        this->gBufferFramebufferManager->getFramebuffers(),
        this->swapchainManager->getImages().size(),
        4
    );
}

void Render::createRenderPasses(){
    RenderPassManager::Description description1, description2, description3;

    GeometryGBufferRenderPassProvider::build(
        description1,
        coreVulkan->getMsaaSamples(),
        coreVulkan->getDepthFormat()
    );
    gBufferRenderPassManager = new RenderPassManager(
        coreVulkan->getDevice(),
        std::move(description1)
    );

    TransparentGBufferRenderPassProvider::build(
        description2,
        coreVulkan->getMsaaSamples(),
        coreVulkan->getDepthFormat()
    );
    transparentRenderPassManager = new RenderPassManager(
        coreVulkan->getDevice(),
        std::move(description2)
    );

    DeferredLightingRenderPassProvider::build(
        description3,
        coreVulkan->getMsaaSamples()
    );
    deferredLightRenderPassManager = new RenderPassManager(
        coreVulkan->getDevice(),
        std::move(description3)
    );

    RenderPassManager::Description compositeDescription;
    CompositeRenderPassProvider::build(
        compositeDescription,
        swapchainManager->getImageFormat()
    );
    compositeRenderPassManager = new RenderPassManager(
        coreVulkan->getDevice(),
        std::move(compositeDescription)
    );
}

void Render::createSwapchainDependentResources(){
    std::vector<std::vector<VkImageView>> attachmentsVector;

    // ==========================
    // GBuffer
    // ==========================
    gBuffer = new GBuffer;
    gBuffer->create(
        coreVulkan->getDevice(),
        coreVulkan->getPhysicalDevice(),
        swapchainManager->getExtent(),
        coreVulkan->getDepthFormat(),
        coreVulkan->getMsaaSamples()
    );

    gBufferDescriptorManager = new GBufferDescriptorManager(
        coreVulkan->getDevice(),
        gBuffer
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

    gBufferFramebufferManager = new FramebufferManager(
        coreVulkan->getDevice(),
        gBufferRenderPassManager->get(),
        swapchainManager->getImageViews().size(),
        swapchainManager->getExtent(),
        attachmentsVector
    );

    // ==========================
    // Transparent GBuffer
    // ==========================
    attachmentsVector.clear();
    transparentGBuffer = new TransparentGBuffer;
    transparentGBuffer->create(
        coreVulkan->getDevice(),
        coreVulkan->getPhysicalDevice(),
        swapchainManager->getExtent(),
        coreVulkan->getMsaaSamples()
    );

    transparentGBufferDescriptorManager = new TransparentGBufferDescriptorManager(
        coreVulkan->getDevice(),
        transparentGBuffer
    );

    GBufferFramebufferProvider::GBufferAttachments transparentGBufferAttachments{
        .position = transparentGBuffer->getView(TransparentGBuffer::Attachment::Position),
        .albedo = transparentGBuffer->getView(TransparentGBuffer::Attachment::Albedo),
        .normal = transparentGBuffer->getView(TransparentGBuffer::Attachment::Normal),
        .material = transparentGBuffer->getView(TransparentGBuffer::Attachment::Material),

        .depth = gBuffer->getView(GBuffer::Attachment::Depth)
    };
    GBufferFramebufferProvider::build(
        transparentGBufferAttachments,
        swapchainManager->getImageViews().size(),
        attachmentsVector
    );

    transparentFramebufferManager = new FramebufferManager(
        coreVulkan->getDevice(),
        transparentRenderPassManager->get(),
        swapchainManager->getImageViews().size(),
        swapchainManager->getExtent(),
        attachmentsVector
    );

    // ==========================
    // Lighting
    // ==========================
    deferredLightingBuffer = new DeferredLightingBuffer;

    deferredLightingBuffer->create(
        coreVulkan->getDevice(),
        coreVulkan->getPhysicalDevice(),
        swapchainManager->getExtent(),
        coreVulkan->getMsaaSamples()
    );

    deferredLightingDescriptorManager =
        new DeferredLightingDescriptorManager(
            coreVulkan->getDevice(),
            deferredLightingBuffer
        );

    attachmentsVector.clear();

    DeferredLightingFramebufferProvider::build(
        deferredLightingBuffer->getView(),
        swapchainManager->getImageViews().size(),
        attachmentsVector
    );

    deferredLightingFramebufferManager =
        new FramebufferManager(
            coreVulkan->getDevice(),
            deferredLightRenderPassManager->get(),
            swapchainManager->getImageViews().size(),
            swapchainManager->getExtent(),
            attachmentsVector
        );

    // ==========================
    // Composite
    // ==========================
    attachmentsVector.clear();
    CompositeFramebufferProvider::build(
        swapchainManager->getImageViews(),
        swapchainManager->getImageViews().size(),
        attachmentsVector
    );

    compositeFramebufferManager = new FramebufferManager(
        coreVulkan->getDevice(),
        compositeRenderPassManager->get(),
        swapchainManager->getImageViews().size(),
        swapchainManager->getExtent(),
        attachmentsVector
    );
}

void Render::createGraphicsPipelineObjects(){
    PipelineCreationContext pipelineContext{
        .device = coreVulkan->getDevice(),
        .gBufferRenderPass = gBufferRenderPassManager->get(),
        .msaa = coreVulkan->getMsaaSamples(),
        .supportedFeatures12 = coreVulkan->getSupportedFeatures12(),
        .config = &config
    };
    pipelineContext.globalLayout = globalDescriptorManager->getLayout();
    pipelineContext.materialLayout = materialDescriptorManager->getLayout();
    pipelineContext.instanceLayout = instanceDescriptorManager->getLayout();
    pipelineContext.particleLayout = particleInstanceDescriptorManager->getLayout();

    pipelineContext.lightRenderPass = deferredLightRenderPassManager->get();
    pipelineContext.compositeRenderPass = compositeRenderPassManager->get();

    pipelineContext.gBufferLayout = gBufferDescriptorManager->getLayout();
    pipelineContext.transparentGBufferLayout = transparentGBufferDescriptorManager->getLayout();
    pipelineContext.lightingLayout = lightInstanceManager->getLightDescriptorManager()->getDescriptorSetLayout();
    pipelineContext.deferredLightingLayout = deferredLightingDescriptorManager->getLayout();

    // Create graphics pipeline
    graphicsPipeline = new GraphicsPipelineManager(
        coreVulkan->getDevice(),
        swapchainManager->getExtent(),
        pipelineContext,
        config
    );
}

void Render::initInstances(){

    renderInstanceManager = new RenderInstanceManager(
        resourceManager
    );


    //* floor
    RenderInstanceRegistration* floor = renderInstanceManager->createRenderInstance(
        resourceManager->getMesh("models/floor/Untitled.gltf")
    );

    RenderInstance * renderInstance0 = renderInstanceManager->getRenderInstance(floor->indexInVector);
    renderInstance0->scale = glm::vec3(10.0f);
    renderInstance0->position += glm::vec3(0, 3, 0);
    renderInstance0->updateModelMatrix();

    renderInstanceRegistration = renderInstanceManager->createRenderInstance(
        resourceManager->getMesh("models/floor/Untitled.gltf")
    );

    RenderInstance * renderInstance4 = renderInstanceManager->getRenderInstance(renderInstanceRegistration->indexInVector);
    renderInstance4->scale = glm::vec3(0.2f);
    renderInstance4->position += glm::vec3(0, 4, 0);
    renderInstance4->updateModelMatrix();

    //* cat
    renderInstanceRegistration = renderInstanceManager->createRenderInstance(
        resourceManager->getMesh("models/Maxwell/Untitled.gltf")
    );

    RenderInstance * renderInstance1 = renderInstanceManager->getRenderInstance(renderInstanceRegistration->indexInVector);
    renderInstance1->scale = glm::vec3(0.2f);
    renderInstance1->position += glm::vec3(0, 1, 0);
    renderInstance1->updateModelMatrix();

    renderInstanceRegistration = renderInstanceManager->createRenderInstance(
        resourceManager->getMesh("models/Maxwell/Untitled.gltf")
    );

    RenderInstance * renderInstance2 = renderInstanceManager->getRenderInstance(renderInstanceRegistration->indexInVector);
    renderInstance2->scale = glm::vec3(0.2f);
    renderInstance2->position += glm::vec3(2, 5, 0);
    renderInstance2->updateModelMatrix();

    //* light
    lightInstanceManager->createLight({
        .position = glm::vec3(0.0f, 100.0f, 0.0f),
        .intensity = 20000.0f,
        .color = glm::vec3(1.0f, 1.0f, 1.0f),
        .radius = 10.0f,
        .type = Config::LightType::Point,
        .range = 1000.0f,
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
        cameraBufferManager->updateCamera(
            currentFrame,
            static_cast<float>(deltaTime),
            swapchainManager->getExtent()
        );
    }

    // update render instances
    {
        std::vector<RenderInstance>& renderInstances = renderInstanceManager->getRenderInstances();
        std::size_t renderInstancesSize = renderInstances.size();
        for (size_t i = 1; i < renderInstancesSize; i++)
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

    //TLAS
    renderInstanceManager->rebuildTLAS();
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

    this->commandManager->recordCommandBuffer(
        imageIndex,
        currentFrame,
        gBufferRenderPassManager->get(),
        transparentRenderPassManager->get(),
        deferredLightRenderPassManager->get(),
        compositeRenderPassManager->get(),
        graphicsPipeline,
        gBufferFramebufferManager->getFramebuffers(),
        transparentFramebufferManager->getFramebuffers(),
        deferredLightingFramebufferManager->getFramebuffers(),
        compositeFramebufferManager->getFramebuffers(),
        swapchainManager->getExtent(),
        globalDescriptorManager,
        instanceDescriptorManager,
        particleInstanceDescriptorManager,
        renderInstanceManager,
        gBufferDescriptorManager,
        transparentGBufferDescriptorManager,
        lightInstanceManager,
        deferredLightingDescriptorManager,
        particlesData,
        {},
        {},
        {},
        {},
        config
    );

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

    if (this->gBufferFramebufferManager){ delete this->gBufferFramebufferManager; this->gBufferFramebufferManager = nullptr; }
    if (this->deferredLightingFramebufferManager){ delete this->deferredLightingFramebufferManager; this->deferredLightingFramebufferManager = nullptr; }
    if (this->transparentFramebufferManager) { delete this->transparentFramebufferManager; this->transparentFramebufferManager = nullptr; }
    if (this->compositeFramebufferManager) { delete this->compositeFramebufferManager; this->compositeFramebufferManager = nullptr; }
    if (this->graphicsPipeline){ delete this->graphicsPipeline; this->graphicsPipeline = nullptr; }

    if (this->gBufferDescriptorManager) { delete this->gBufferDescriptorManager; this->gBufferDescriptorManager = nullptr; }
    if (this->gBuffer) { gBuffer->destroy(coreVulkan->getDevice()); delete this->gBuffer; this->gBuffer = nullptr;}
    if (this->transparentGBufferDescriptorManager) { delete this->transparentGBufferDescriptorManager; this->transparentGBufferDescriptorManager = nullptr; }
    if (this->transparentGBuffer) { this->transparentGBuffer->destroy(coreVulkan->getDevice()); delete this->transparentGBuffer; this->transparentGBuffer = nullptr; }
    if (this->deferredLightingDescriptorManager) { delete this->deferredLightingDescriptorManager; this->deferredLightingDescriptorManager = nullptr; }
    if (this->deferredLightingBuffer) { this->deferredLightingBuffer->destroy(coreVulkan->getDevice()); delete this->deferredLightingBuffer; this->deferredLightingBuffer = nullptr; }

    if (this->gBufferRenderPassManager){ delete this->gBufferRenderPassManager; this->gBufferRenderPassManager = nullptr; }
    if (this->transparentRenderPassManager){ delete this->transparentRenderPassManager; this->transparentRenderPassManager = nullptr; }
    if (this->deferredLightRenderPassManager){ delete this->deferredLightRenderPassManager; this->deferredLightRenderPassManager = nullptr; }
    if (this->compositeRenderPassManager){ delete this->compositeRenderPassManager; this->compositeRenderPassManager = nullptr; }
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
        gBufferFramebufferManager->getFramebuffers()
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