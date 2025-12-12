#pragma once
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"
#include "VulkanPhyscialDevice.h"
#include <stdexcept>

class VulkanBuffer
{
public:
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;

    VulkanBuffer() = default;

    void createBuffer(const VulkanPhysicalDevice* physicalDevice, const VulkanDevice* device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device->device, buffer, &memRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = physicalDevice->findMemoryType(memRequirements.memoryTypeBits, properties);
        if (vkAllocateMemory(device->device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }
        vkBindBufferMemory(device->device, buffer, memory, 0);
    }

    void releaseBuffer(const VulkanDevice* device) const
    {
        if (buffer)
            vkDestroyBuffer(device->device, buffer, nullptr);
        if (memory)
            vkFreeMemory(device->device, memory, nullptr);
    }

    void copyBuffer(const VulkanDevice* device, VkCommandPool commandPool, VkBuffer srcBuffer, VkDeviceSize size) const
    {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands(commandPool);
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, buffer, 1, &copyRegion);
        device->endSingleTimeCommands(commandPool, commandBuffer, device->graphicsQueue);
    }
};

