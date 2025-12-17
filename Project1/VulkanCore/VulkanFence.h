#pragma once
#include "VulkanDevice.h"

class VulkanFence
{
public:
    VkFence fence = VK_NULL_HANDLE;

    void createFence(const VulkanDevice& device, bool signaled = false)
    {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        if (vkCreateFence(device.device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create fence!");
        }
    }

    void destroyFence(const VulkanDevice& device) const
    {
        if (fence != VK_NULL_HANDLE)
        {
            vkDestroyFence(device.device, fence, nullptr);
        }
    }
};
