#pragma once

#include "../CoreVulkan.hpp"

/**
@brief Manages Vulkan shader modules for a graphics pipeline.

Loads vertex and fragment shaders from SPIR-V files,
creates the corresponding shader modules and destroys them
automatically when this object goes out of scope.
*/
class ShaderLoader {
private:
    VkDevice device;
    /**
    @brief Vulkan shader module handle for the vertex shader.

    Created from the provided SPIR-V vertex shader file in the constructor.
    Automatically destroyed in the destructor.
    */
    VkShaderModule vertModule = VK_NULL_HANDLE;
    /**
    @brief Vulkan shader module handle for the fragment shader.

    Created from the provided SPIR-V fragment shader file in the constructor.
    Automatically destroyed in the destructor.
    */
    VkShaderModule fragModule = VK_NULL_HANDLE;

    /**
    @brief Reads the contents of a binary file into a byte buffer.

    Utility method used to read SPIR-V shader files from disk.

    @param filename Path to the file to read.
    @return A vector of bytes containing the file contents.

    @throws std::runtime_error If the file cannot be opened or read.
    */
    std::vector<char> readFile(const std::string& filename);

    /**
    @brief Creates a Vulkan shader module from SPIR-V bytecode.

    Wraps `vkCreateShaderModule` and returns the created shader module handle.

    @param code The SPIR-V bytecode as a vector of bytes.
    @return The created VkShaderModule.

    @throws std::runtime_error If the shader module cannot be created.
    */
    VkShaderModule createShaderModule(const std::vector<char>& code);
public:
    /**
    Creates shader modules from SPIR-V files.

    @param vertPath Path to the vertex shader SPIR-V file.
    @param fragPath Path to the fragment shader SPIR-V file.
    */
    ShaderLoader(VkDevice device, const std::string& vertPath, const std::string& fragPath);

    /**
    Cleans up the shader modules.
    */
    ~ShaderLoader();

    VkShaderModule getVertModule() const { return vertModule; }
    VkShaderModule getFragModule() const { return fragModule; }
};