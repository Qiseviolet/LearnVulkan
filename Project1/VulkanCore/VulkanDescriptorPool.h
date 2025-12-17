#pragma once
#include "VulkanDevice.h"

class VulkanDescriptorPool
{
public:
    VkDescriptorPool descriptorPool;

    void createDescriptorPool(const VulkanDevice* device, uint32_t poolSizeCount, const VkDescriptorPoolSize* poolSize, uint32_t maxSets)
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = poolSizeCount;
        descriptorPoolInfo.pPoolSizes = poolSize;
        descriptorPoolInfo.maxSets = maxSets;
        if (vkCreateDescriptorPool(device->device, &descriptorPoolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void destroyDescriptorPool(const VulkanDevice* device) const
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device->device, descriptorPool, nullptr);
        }
    }

    std::vector<VkDescriptorSet> allocateDescriptorSets(const VulkanDevice* device, const VkDescriptorSetLayout* descriptorSetLayout, uint32_t descriptorSetCount) const
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = descriptorSetCount;
        allocInfo.pSetLayouts = descriptorSetLayout;

        std::vector<VkDescriptorSet> descriptorSet;
        descriptorSet.resize(descriptorSetCount);
        if (vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSet.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }
        return descriptorSet;
    }
};
