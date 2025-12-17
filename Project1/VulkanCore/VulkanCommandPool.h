#pragma once
#include "VulkanDescriptorPool.h"

class VulkanCommandPool
{
public:
    VkCommandPool commandPool = VK_NULL_HANDLE;

    void createCommandPool(const VulkanPhysicalDevice& vPhysicalDevice, const VulkanDevice& vDevice, const VulkanSurface& vSurface)
    {
        QueueFamilyIndices queueFamilyIndices = VulkanPhysicalDevice::findQueueFamilies(vPhysicalDevice.physicalDevice, vSurface.surface);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(vDevice.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void destroyCommandPool(const VulkanDevice& vDevice) const
    {
        if (commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(vDevice.device, commandPool, nullptr);
        }
    }

    VkCommandBuffer allocateCommandBuffer(const VulkanDevice& vDevice, VkCommandBufferLevel level) const
    {
        VkCommandBuffer commandBuffer;
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = level;
        allocInfo.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(vDevice.device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffer!");
        }
        return commandBuffer;
    }

    std::vector<VkCommandBuffer> allocateCommandBuffers(const VulkanDevice& vDevice,
        VkCommandBufferLevel level, uint32_t commandBufferCount) const
    {
        std::vector<VkCommandBuffer> commandBuffers;
        commandBuffers.resize(commandBufferCount);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = level;
        allocInfo.commandBufferCount = commandBufferCount;
        if (vkAllocateCommandBuffers(vDevice.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
        return commandBuffers;
    }
    
    VkCommandBuffer beginSingleTimeCommands(const VulkanDevice& vDevice, VkCommandBufferLevel level) const
    {
        VkCommandBuffer commandBuffer = allocateCommandBuffer(vDevice, level);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void endSingleTimeCommands(const VulkanDevice& vDevice, const VkCommandBuffer& commandBuffer, const VkQueue& queue) const
    {
        vkEndCommandBuffer(commandBuffer);
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
        vkFreeCommandBuffers(vDevice.device, commandPool, 1, &commandBuffer);
    }
};
