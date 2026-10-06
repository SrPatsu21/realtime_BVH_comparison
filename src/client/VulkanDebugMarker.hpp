#pragma once

#include <vulkan/vulkan.h>

#ifndef NDEBUG
    #include <nvtx3/nvtx3.hpp>

    static inline VkCommandBuffer g_actualCommandBuffer = VK_NULL_HANDLE;
    static inline PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT = nullptr;
    static inline PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT = nullptr;

    inline void initVulkanDebugMarkers(VkDevice device)
    {
        pfnCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
            vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));

        pfnCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
            vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
    }

    inline void setActualCommandBuffer(VkCommandBuffer cmd)
    {
        g_actualCommandBuffer = cmd;
    }

    // ============================================================
    // CPU Marker (RAII Wrapper para NVTX)
    // ============================================================
    class NGFXCPUMarker {
    public:
        explicit NGFXCPUMarker(const char* name)
            : m_range(name) {}

    private:
        nvtx3::scoped_range m_range;
    };

    // ============================================================
    // GPU / Vulkan Marker (RAII Scope Guard)
    // ============================================================
    class NGFXGPUMarker {
    public:
        explicit NGFXGPUMarker(const char* name)
        {
            if (g_actualCommandBuffer != VK_NULL_HANDLE && pfnCmdBeginDebugUtilsLabelEXT != nullptr)
            {
                VkDebugUtilsLabelEXT label{};
                label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                label.pLabelName = name;
                label.color[0] = 1.0f;
                label.color[1] = 1.0f;
                label.color[2] = 1.0f;
                label.color[3] = 1.0f;

                pfnCmdBeginDebugUtilsLabelEXT(g_actualCommandBuffer, &label);
                m_active = true;
            }
        }

        ~NGFXGPUMarker()
        {
            if (m_active && g_actualCommandBuffer != VK_NULL_HANDLE && pfnCmdEndDebugUtilsLabelEXT != nullptr)
            {
                pfnCmdEndDebugUtilsLabelEXT(g_actualCommandBuffer);
            }
        }

        NGFXGPUMarker(const NGFXGPUMarker&) = delete;
        NGFXGPUMarker& operator=(const NGFXGPUMarker&) = delete;

    private:
        bool m_active = false;
    };

#else
    inline void initVulkanDebugMarkers(VkDevice) {}
    inline void setActualCommandBuffer(VkCommandBuffer) {}

    class NGFXCPUMarker {
    public:
        explicit NGFXCPUMarker(const char*) {}
    };

    class NGFXGPUMarker {
    public:
        explicit NGFXGPUMarker(const char*) {}
    };
#endif