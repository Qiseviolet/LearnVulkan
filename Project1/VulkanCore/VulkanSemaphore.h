#pragma once
#include "VulkanDevice.h"

class VulkanSemaphore
{
public:
    VkSemaphore semaphore = VK_NULL_HANDLE;

    void createSemaphore(const VulkanDevice& device)
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        if (vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphore!");
        }
    }

    void destroySemaphore(const VulkanDevice& device) const
    {
        vkDestroySemaphore(device.device, semaphore, nullptr);
    }
};
