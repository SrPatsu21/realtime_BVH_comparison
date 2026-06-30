#include "CoreVulkan.hpp"
#include <set>

CoreVulkan::CoreVulkan(
    GLFWwindow* window,
    const std::vector<IInstanceConfigProvider*>& instanceProviders,
    const std::vector<IPhysicalDeviceSelector*>& physicalDeviceSelectors,
    const std::vector<IDeviceConfigProvider*>& deviceProviders,
    const std::vector<IDepthFormatProvider*>& depthProviders
){
    // test debug mode
    #ifndef NDEBUG
        if (!checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
    #endif

    // instance and surface
    createInstance(instanceProviders);
    createSurface(window);

    // device extensions
    pickPhysicalDevice(physicalDeviceSelectors);
    msaaSamples = findMaxLimitedUsableSampleCount(VK_SAMPLE_COUNT_8_BIT, physicalDevice);
    atomSize = takeAtomSize(physicalDevice);
    #ifndef NDEBUG
        std::cout << "Sample Count: " << msaaSamples << std::endl;
    #endif
    graphicsQueueFamilyIndices = findQueueFamilies(physicalDevice);
    if (!graphicsQueueFamilyIndices.isComplete()) {
        throw std::runtime_error("failed to find a graphics queue family!");
    }
    updateSwapchainDetails();
    createLogicalDevice(deviceProviders);
    vkGetDeviceQueue(device, graphicsQueueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, graphicsQueueFamilyIndices.presentFamily.value(), 0, &presentQueue);

    // find depth requirements
    DepthFormatRequirements req{};
    for (auto* d : depthProviders) {
        d->contribute(req);
    }
    std::vector<VkFormat> candidates;
    if (req.requireStencil) {
        candidates = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
    } else if (req.preferHighPrecision) {
        candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
    } else {
        candidates = {
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT
        };
    }

    depthFormat = findSupportedFormat(
        candidates,
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

CoreVulkan::CoreVulkan(CoreVulkan&& other) noexcept
{
    instance = other.instance;
    surface = other.surface;
    graphicsQueueFamilyIndices = std::move(other.graphicsQueueFamilyIndices);
    physicalDevice = other.physicalDevice;
    swapchainSupportDetails = std::move(other.swapchainSupportDetails);
    msaaSamples = other.msaaSamples;
    device = other.device;
    presentQueue = other.presentQueue;
    graphicsQueue = other.graphicsQueue;
    depthFormat = other.depthFormat;

    other.instance = VK_NULL_HANDLE;
    other.surface = VK_NULL_HANDLE;
    other.physicalDevice = VK_NULL_HANDLE;
    other.device = VK_NULL_HANDLE;
    other.presentQueue = VK_NULL_HANDLE;
    other.graphicsQueue = VK_NULL_HANDLE;
    other.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    other.depthFormat = VK_FORMAT_UNDEFINED;
    other.graphicsQueueFamilyIndices = {};
    other.swapchainSupportDetails = {};
}

CoreVulkan& CoreVulkan::operator=(CoreVulkan&& other) noexcept
{
    if (this != &other)
    {
        cleanup(); //avoid zombie memory

        instance = other.instance;
        surface = other.surface;
        physicalDevice = other.physicalDevice;
        device = other.device;
        presentQueue = other.presentQueue;
        graphicsQueue = other.graphicsQueue;
        msaaSamples = other.msaaSamples;
        depthFormat = other.depthFormat;
        graphicsQueueFamilyIndices = std::move(other.graphicsQueueFamilyIndices);
        swapchainSupportDetails = std::move(other.swapchainSupportDetails);

        other.instance = VK_NULL_HANDLE;
        other.surface = VK_NULL_HANDLE;
        other.device = VK_NULL_HANDLE;
        other.physicalDevice = VK_NULL_HANDLE;
        other.presentQueue = VK_NULL_HANDLE;
        other.graphicsQueue = VK_NULL_HANDLE;
        other.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        other.depthFormat = VK_FORMAT_UNDEFINED;
        other.graphicsQueueFamilyIndices =  {};
        other.swapchainSupportDetails = {};
    }
    return *this;
}

bool CoreVulkan::checkValidationLayerSupport()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* requiredLayer : validationLayers) {
        bool found = false;

        for (const VkLayerProperties& layer : availableLayers) {
            if (std::strcmp(requiredLayer, layer.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

void CoreVulkan::createInstance(
    const std::vector<IInstanceConfigProvider*>& providers
) {
    // base config
    InstanceConfig config{};
    config.apiVersion = VK_API_VERSION_1_3;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
        config.extensions.push_back(glfwExtensions[i]);
    }
    #ifndef NDEBUG
        config.layers.insert(
            config.layers.end(),
            validationLayers.begin(),
            validationLayers.end()
        );

        config.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    // mods
    for (auto* provider : providers) {
        provider->contribute(config);
    }

    // build VkInstanceCreateInfo
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Apotheosis";
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = config.apiVersion;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(config.extensions.size());
    createInfo.ppEnabledExtensionNames = config.extensions.data();

    createInfo.enabledLayerCount = static_cast<uint32_t>(config.layers.size());
    createInfo.ppEnabledLayerNames = config.layers.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance\n";
    }


    // Create Debug Messenger
    #ifndef NDEBUG
        CreateDebugCallback();
    #endif
    // Checking for extension support and print
    #ifndef NDEBUG
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "available extensions:\n";
        for (const auto& extension : extensions) {
            std::cout << extension.extensionName << '\t' << '\t';
        }
        std::cout << std::endl;
    #endif
}

void CoreVulkan::createSurface(
    GLFWwindow* window
) {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
};

SwapchainSupportDetails CoreVulkan::querySwapchainSupport(
    VkPhysicalDevice physicalDevice
) {
    SwapchainSupportDetails swapchainSupportDetails;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainSupportDetails.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        swapchainSupportDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapchainSupportDetails.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        swapchainSupportDetails.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, swapchainSupportDetails.presentModes.data());
    }
    return swapchainSupportDetails;
}

QueueFamilyIndices CoreVulkan::findQueueFamilies(
    VkPhysicalDevice physicalDevice
) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        const auto& queueFamily = queueFamilies[i];

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(
            physicalDevice,
            i,
            surface,
            &presentSupport
        );

        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if (indices.isComplete()) {
            return indices;
        }
    }

    return indices;
}

bool CoreVulkan::isDeviceSuitable(
    VkPhysicalDevice physicalDevice,
    const PhysicalDeviceRequirements& reqs,
    const std::vector<IPhysicalDeviceSelector*>& selectors
) {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    if (!indices.isComplete())
        return false;

    // check Device Extension Support
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &extensionCount,
        nullptr
    );
    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        physicalDevice,
        nullptr,
        &extensionCount,
        available.data()
    );

    for (auto* required : reqs.requiredExtensions) {
        bool found = false;
        for (auto& avail : available) {
            if (strcmp(required, avail.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    // features
    VkPhysicalDeviceFeatures supported{};
    vkGetPhysicalDeviceFeatures(physicalDevice, &supported);

    if ((supported.samplerAnisotropy & reqs.requiredFeatures.samplerAnisotropy) != reqs.requiredFeatures.samplerAnisotropy)
        return false;

    if ((supported.geometryShader & reqs.requiredFeatures.geometryShader) != reqs.requiredFeatures.geometryShader)
        return false;

    // swapchain
    SwapchainSupportDetails swapchainSupportDetails = querySwapchainSupport(physicalDevice);
    if (swapchainSupportDetails.formats.empty() || swapchainSupportDetails.presentModes.empty())
        return false;

    // mods
    for (auto* sel : selectors) {
        if (!sel->isDeviceCompatible(physicalDevice, reqs))
            return false;
    }

    return true;
}

int CoreVulkan::rateDeviceSuitability(
    VkPhysicalDevice physicalDevice,
    const PhysicalDeviceRequirements& reqs,
    const std::vector<IPhysicalDeviceSelector*>& selectors
) {
    if (!isDeviceSuitable(physicalDevice, reqs, selectors))
        return 0;

    int score = 0;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // mods
    for (auto* sel : selectors) {
        sel->scoreDevice(physicalDevice, score);
    }

    return score;
}

VkSampleCountFlagBits CoreVulkan::findMaxLimitedUsableSampleCount(
    VkSampleCountFlagBits maxDesiredSamples,
    VkPhysicalDevice physicalDevice
) {
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);

    VkSampleCountFlags supported =
        props.limits.framebufferColorSampleCounts &
        props.limits.framebufferDepthSampleCounts &
        maxDesiredSamples;

    if (supported == 0) {
        return VK_SAMPLE_COUNT_1_BIT;
    }

    return static_cast<VkSampleCountFlagBits>(
        1u << (31 - __builtin_clz(supported)) // get the max desired
    );
}

VkDeviceSize CoreVulkan::takeAtomSize(
    VkPhysicalDevice physicalDevice
) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    return properties.limits.nonCoherentAtomSize;
}

bool CoreVulkan::isMemoryCoherent(
    VkPhysicalDevice physicalDevice,
    uint32_t memoryTypeIndex
) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    if (memoryTypeIndex >= memProperties.memoryTypeCount)
        throw std::runtime_error("Invalid memoryTypeIndex");

    VkMemoryPropertyFlags flags = memProperties.memoryTypes[memoryTypeIndex].propertyFlags;

    return (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
}

void CoreVulkan::pickPhysicalDevice(
    const std::vector<IPhysicalDeviceSelector*>& selectors
) {
    PhysicalDeviceRequirements reqs{};
    reqs.requiredExtensions = DEVICE_EXTENSIONS;
    reqs.requiredFeatures.samplerAnisotropy = VK_TRUE;
    reqs.requiredFeatures.geometryShader = VK_TRUE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
    int bestScore = 0;

    for (const auto& device : devices) {
        int score = rateDeviceSuitability(device, reqs, selectors);
        if (score > bestScore) {
            bestScore = score;
            bestDevice = device;
        }
    }

    // Check if the best candidate is suitable at all
    if (bestDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    physicalDevice = bestDevice;

    // debug
    #ifndef NDEBUG
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        std::cout << "GPU name: " << deviceProperties.deviceName << std::endl;
    #endif
}

void CoreVulkan::updateSwapchainDetails()
{
    swapchainSupportDetails = querySwapchainSupport(physicalDevice);
};

void CoreVulkan::createLogicalDevice(
    const std::vector<IDeviceConfigProvider*>& providers
) {
    // set queue info
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        graphicsQueueFamilyIndices.graphicsFamily.value(),
        graphicsQueueFamilyIndices.presentFamily.value()
    };

    float queuePriority = 1.0f;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // base config
    DeviceConfig config{};
    config.extensions = DEVICE_EXTENSIONS;

    config.requiredFeatures.samplerAnisotropy = VK_TRUE;
    config.optionalFeatures.sampleRateShading = VK_TRUE;
    config.optionalFeatures.wideLines = VK_TRUE;

    for (auto* p : providers) {
        p->contribute(config);
    }

    // resolve feature support
    supportedFeatures12 = {};
    supportedFeatures12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceFeatures2 supportedFeatures2{};
    supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    supportedFeatures2.pNext = &supportedFeatures12;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &supportedFeatures2);

    // enable features
    VkPhysicalDeviceVulkan12Features enabled12{};
    enabled12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    if (!supportedFeatures12.bufferDeviceAddress)
    {
        throw std::runtime_error(
            "GPU does not support buffer device address."
        );
    }

    enabled12.bufferDeviceAddress = VK_TRUE;

    if (supportedFeatures12.descriptorIndexing) {
        enabled12.descriptorIndexing = VK_TRUE;
        enabled12.runtimeDescriptorArray = supportedFeatures12.runtimeDescriptorArray;
        enabled12.descriptorBindingPartiallyBound = supportedFeatures12.descriptorBindingPartiallyBound;
        enabled12.descriptorBindingVariableDescriptorCount = supportedFeatures12.descriptorBindingVariableDescriptorCount;
        enabled12.shaderSampledImageArrayNonUniformIndexing = supportedFeatures12.shaderSampledImageArrayNonUniformIndexing;
        enabled12.shaderStorageBufferArrayNonUniformIndexing = supportedFeatures12.shaderStorageBufferArrayNonUniformIndexing;
        enabled12.descriptorBindingSampledImageUpdateAfterBind = supportedFeatures12.descriptorBindingSampledImageUpdateAfterBind;
    }

    VkPhysicalDeviceFeatures2 enabledFeatures2{};
    enabledFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enabledFeatures2.features.samplerAnisotropy = config.requiredFeatures.samplerAnisotropy;
    enabledFeatures2.features.sampleRateShading = config.optionalFeatures.sampleRateShading;
    enabledFeatures2.features.wideLines = config.optionalFeatures.wideLines;
    enabledFeatures2.pNext = &enabled12;

    // device create info
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = nullptr;
    createInfo.pNext = &enabledFeatures2;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(config.extensions.size());
    createInfo.ppEnabledExtensionNames = config.extensions.data();

    // #ifndef NDEBUG
    //     createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    //     createInfo.ppEnabledLayerNames = validationLayers.data();
    // #endif

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device");
}

VkFormat CoreVulkan::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

void CoreVulkan::cleanup()
{
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
    #ifndef NDEBUG
        DestroyDebugCallback();
    #endif
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}

CoreVulkan::~CoreVulkan(){
    cleanup();
}

uint32_t CoreVulkan::findMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags required,
    VkMemoryPropertyFlags preferred
) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t fallback = UINT32_MAX;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (!(typeFilter & (1u << i))) continue;

        VkMemoryPropertyFlags flags = memProperties.memoryTypes[i].propertyFlags;

        if ((flags & required) == required) {
            if ((flags & preferred) == preferred) {
                return i;
            }
            if (fallback == UINT32_MAX)
                fallback = i;
        }
    }

    if (fallback != UINT32_MAX) return fallback;
    throw std::runtime_error("no suitable memory type found");
}

bool CoreVulkan::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

bool CoreVulkan::hasDepthComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT ||
        format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT;
}

#ifndef NDEBUG

void CoreVulkan::CreateDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        // VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;

    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
    );

    if (!func) {
        throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT");
    }

    VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug messenger");
    }

    std::cout << "Debug messenger active\n";
}

void CoreVulkan::DestroyDebugCallback()
{
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
    );

    if (func) {
        func(instance, debugMessenger, nullptr);
    }
}
#endif
