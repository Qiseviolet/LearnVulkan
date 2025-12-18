#pragma once
#include <iostream>
#include <vector>
#include "Mesh.h"
#include "Texture.h"
#include "../VulkanCore/VulkanDescriptorSetLayout.h"
#include "../VulkanCore/VulkanUniformBuffer.h"

class Object
{
public:
    std::vector<VkDescriptorSet> descriptorSets;
    Mesh mesh;
    Texture texture;

    template <typename T>
    void loadObject(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device, const VulkanCommandPool& commandPool,
        const std::string& modelPath, const glm::mat4& modelMatrix, const std::string& texturePath,
        const VulkanDescriptorPool& descriptorPool, const std::vector<VulkanUniformBuffer>& uniformBuffers, const VulkanDescriptorSetLayout& descriptorSetLayout)
    {
        loadMesh(physicalDevice, device, commandPool, modelPath, modelMatrix);
        loadTexture(physicalDevice, device, commandPool, texturePath);
        updateDescriptorSet<T>(device, descriptorPool, uniformBuffers, descriptorSetLayout);
    }
    
private:
    void loadMesh(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device, const VulkanCommandPool& commandPool,
        const std::string& path, const glm::mat4& matrix)
    {
        Model model;
        model.loadModel(path);
        mesh.createVertexBuffer(physicalDevice, device, model, commandPool);
        mesh.createIndexBuffer(physicalDevice, device, model, commandPool);
        mesh.modelMatrix = matrix;
    }

    void loadTexture(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device, const VulkanCommandPool& commandPool,
        const std::string& path)
    {
        texture.loadTextureImage(physicalDevice, device, commandPool, path, VK_FORMAT_R8G8B8A8_SRGB);
        texture.createTextureSampler(physicalDevice, device);
    }

    template <typename T>
    void updateDescriptorSet(const VulkanDevice& device, const VulkanDescriptorPool& descriptorPool,
        const std::vector<VulkanUniformBuffer>& uniformBuffers, VulkanDescriptorSetLayout descriptorSetLayout)
    {
        size_t setCount = uniformBuffers.size();
        std::vector<VkDescriptorSetLayout> layouts(setCount, descriptorSetLayout.descriptorSetLayout);
        descriptorSets = descriptorPool.allocateDescriptorSets(&device, layouts.data(), static_cast<uint32_t>(setCount));
        for (size_t i = 0; i < descriptorSets.size(); ++i)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i].vBuffer.buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(T);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.image.imageView;
            imageInfo.sampler = texture.sampler;

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;
            vkUpdateDescriptorSets(device.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
};
