#pragma once
#include "VulkanDevice.h"

class VulkanDescriptorSetLayout
{
public:
    VkDescriptorSetLayout descriptorSetLayout;

    void createDescriptorSetLayout(const VulkanDevice* device, const VkDescriptorSetLayoutBinding* bindings, uint32_t bindingCount)
    {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindingCount;
        layoutInfo.pBindings = bindings;
        if (vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void destroyDescriptorSetLayout(const VulkanDevice* device) const
    {
        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, nullptr);
        }
    }
};
