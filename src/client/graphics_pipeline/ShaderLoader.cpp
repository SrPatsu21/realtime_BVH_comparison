#include "ShaderLoader.hpp"
#include <fstream>

std::vector<char> ShaderLoader::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open shader file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

VkShaderModule ShaderLoader::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

ShaderLoader::ShaderLoader(
    VkDevice device,
    const std::string& vertPath,
    const std::string& fragPath
) :
    device(device)
{
    auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);

    this->vertModule = createShaderModule(vertCode);
    this->fragModule = createShaderModule(fragCode);
}

ShaderLoader::~ShaderLoader() {
    if (this->vertModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, this->vertModule, nullptr);
    }
    if (this->fragModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, this->fragModule, nullptr);
    }
}