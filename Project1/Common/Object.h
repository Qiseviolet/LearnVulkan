#pragma once
#include <iostream>
#include <vector>
#include "Mesh.h"
#include "Texture.h"
#include "../Camera/CameraData.h"
#include "../Light/DirectionalLight.h"
#include "../Light/LightSpaceMatrix.h"
#include "../VulkanCore/VulkanDescriptorSetLayout.h"
#include "../VulkanCore/VulkanUniformBuffer.h"

class Object
{
public:
    std::vector<VkDescriptorSet> descriptorSets;
    Mesh mesh;
    Texture texture;

    void loadObject(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device, const VulkanCommandPool& commandPool,
        const std::string& modelPath, const glm::mat4& modelMatrix, const std::string& texturePath,
        const VulkanDescriptorPool& descriptorPool, const VulkanDescriptorSetLayout& descriptorSetLayout,
        const std::vector<VulkanUniformBuffer>& cameraUniformBuffers,  const std::vector<VulkanUniformBuffer>& lightUniformBuffers,
        const std::vector<VulkanUniformBuffer>& lightSpaceUniformBuffers, VkImageView shadowMapImageView, VkSampler shadowMapSampler)
    {
        loadMesh(physicalDevice, device, commandPool, modelPath, modelMatrix);
        loadTexture(physicalDevice, device, commandPool, texturePath);
        updateDescriptorSet(device, descriptorPool, cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadowMapImageView, shadowMapSampler, descriptorSetLayout);
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

    void updateDescriptorSet(const VulkanDevice& device, const VulkanDescriptorPool& descriptorPool,
        const std::vector<VulkanUniformBuffer>& cameraUniform, const std::vector<VulkanUniformBuffer>& lightUniform,
        const std::vector<VulkanUniformBuffer>& lightSpaceUniform, VkImageView shadowMapImageView, VkSampler shadowMapSampler,
        VulkanDescriptorSetLayout descriptorSetLayout)
    {
        size_t setCount = cameraUniform.size();
        std::vector<VkDescriptorSetLayout> layouts(setCount, descriptorSetLayout.descriptorSetLayout);
        descriptorSets = descriptorPool.allocateDescriptorSets(&device, layouts.data(), static_cast<uint32_t>(setCount));
        for (size_t i = 0; i < descriptorSets.size(); ++i)
        {
            std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
            
            VkDescriptorBufferInfo cameraBufferInfo{};
            cameraBufferInfo.buffer = cameraUniform[i].vBuffer.buffer;
            cameraBufferInfo.offset = 0;
            cameraBufferInfo.range = sizeof(CameraData);

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &cameraBufferInfo;

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.image.imageView;
            imageInfo.sampler = texture.sampler;
            
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            VkDescriptorBufferInfo lightBufferInfo{};
            lightBufferInfo.buffer = lightUniform[i].vBuffer.buffer;
            lightBufferInfo.offset = 0;
            lightBufferInfo.range = sizeof(DirectionalLight);
            
            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &lightBufferInfo;

            VkDescriptorBufferInfo lightSpaceBufferInfo{};
            lightSpaceBufferInfo.buffer = lightSpaceUniform[i].vBuffer.buffer;
            lightSpaceBufferInfo.offset = 0;
            lightSpaceBufferInfo.range = sizeof(LightSpaceMatrix);
            
            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo = &lightSpaceBufferInfo;

            VkDescriptorImageInfo shadowMapInfo{};
            shadowMapInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            shadowMapInfo.imageView = shadowMapImageView;
            shadowMapInfo.sampler = shadowMapSampler;
            
            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = descriptorSets[i];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pImageInfo = &shadowMapInfo;
            
            vkUpdateDescriptorSets(device.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
};
