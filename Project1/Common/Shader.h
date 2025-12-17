#pragma once
#include <stdexcept>
#include <vector>
#include <fstream>
#include "../VulkanCore/VulkanDevice.h"

class Shader
{
private:
    const VulkanDevice* vDevice = nullptr;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    
public:
    Shader(const VulkanDevice* device) : vDevice(device){}
    
    ~Shader()
    {
        if (shaderModule) vkDestroyShaderModule(vDevice->device, shaderModule, nullptr);
    }
    
    void createShaderStageInfo(const std::string& filename, VkShaderStageFlagBits stage)
    {
        createShaderModule(filename);
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = stage;
        shaderStageInfo.module = shaderModule;
        shaderStageInfo.pName = "main";
    }

    const VkPipelineShaderStageCreateInfo& getShaderStageInfo() const {
        return shaderStageInfo;
    }

private:
    void createShaderModule(const std::string& filename)
    {
        std::vector<char> code = readFile(filename);
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        if (vkCreateShaderModule(vDevice->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        std::cout << "Shader module created from " << filename << std::endl;
    }
    
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }
};
