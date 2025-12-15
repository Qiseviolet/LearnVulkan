#pragma once
#include "VulkanBuffer.h"

class UniformBuffer
{
public:
    VulkanBuffer vBuffer;
    void* mapped = nullptr;
    
    void createUniformBuffer(const VulkanPhysicalDevice* physicalDevice, const VulkanDevice* device,
        VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        vBuffer.createBuffer(physicalDevice, device, size, usage, properties);
        vkMapMemory(device->device, vBuffer.memory, 0, size, 0, &mapped);
    }

    void destroyUniformBuffer(const VulkanDevice* device)
    {
        vkUnmapMemory(device->device, vBuffer.memory);
        mapped = nullptr;
        vBuffer.releaseBuffer(device);
    }

    template<typename T>
    void updateUniformBuffer(const T& data) {
        memcpy(mapped, &data, sizeof(T));
    }
};
