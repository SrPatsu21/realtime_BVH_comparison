#pragma once

#ifndef GLFW_INCLUDE_VULKAN
    #define GLFW_INCLUDE_VULKAN
#endif

#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>


/**
 * @struct SwapchainSupportDetails
 * @brief Holds the capabilities and supported formats/present modes of a surface.
 *
 * Contains the surface capabilities, supported image formats, and
 * supported present modes, as queried from the physical device.
 */
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
