#pragma once
#include "vulkan/vulkan.h"
#include "VulkanPhyscialDevice.h"
#include "VulkanDevice.h"

class VulkanImage
{
public:
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;

    VulkanImage() = default;

    void createImage(const VulkanPhysicalDevice* physicalDevice, const VulkanDevice* device,
        uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
        VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.usage = usage;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        if (vkCreateImage(device->device, &imageInfo, nullptr, &image)) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirments;
        vkGetImageMemoryRequirements(device->device, image, &memRequirments);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirments.size;
        allocInfo.memoryTypeIndex = physicalDevice->findMemoryType(memRequirments.memoryTypeBits, properties);
        if (vkAllocateMemory(device->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }
        vkBindImageMemory(device->device, image, imageMemory, 0);
    }

    VkImageView createImageView(const VulkanDevice* device, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        VkImageView result;
        if (vkCreateImageView(device->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
        return result;
    }

    void ReleaseImage(const VulkanDevice* device) const
    {
        if (imageView)
            vkDestroyImageView(device->device, imageView, nullptr);
        if (image)
            vkDestroyImage(device->device, image, nullptr);
        if (imageMemory)
            vkFreeMemory(device->device, imageMemory, nullptr);
    }
};
