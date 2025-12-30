#pragma once
#include "../VulkanCore/VulkanRenderPass.h"
#include "../VulkanCore/VulkanGraphicsPipeline.h"
#include "../VulkanCore/VulkanDescriptorSetLayout.h"
#include "../VulkanCore/VulkanImage.h"
#include "../Light/LightSpaceMatrix.h"
#include "../VulkanCore/VulkanUniformBuffer.h"
#include "../Common/Object.h"
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shadow
{
public:
    std::vector<VkCommandBuffer> shadowCommandBuffers;
    VulkanImage shadowMapImage;
    VkSampler shadowMapSampler;
    
    void createShadowResources(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device,
        const VulkanCommandPool& commandPool, VkFormat depthFormat, const VertexInputDescription& vertexInputDescription,
        const VulkanDescriptorPool& descriptorPool, const std::vector<VulkanUniformBuffer>& lightSpaceUniformBuffers, size_t maxFramesInFlight)
    {
        createShadowDescriptorSetLayout(device);
        createShadowRenderPass(device, depthFormat);
        createShadowMap(physicalDevice, device, commandPool, depthFormat);
        createShadowPipeline(device, vertexInputDescription);
        updateShadowDescriptors(device, descriptorPool, lightSpaceUniformBuffers, maxFramesInFlight);
        createShadowCommandBuffers(device, commandPool, maxFramesInFlight);
    }

    void updateLightSpaceMatrix(uint32_t currentImage, const glm::mat4& lightViewMatrix, std::vector<VulkanUniformBuffer>& lightSpaceUniformBuffers) const
    {
        glm::mat4 lightProj = glm::orthoZO(-50.0f, 50.0f, -50.0f, 50.0f, 0.0f, 100.0f);
        LightSpaceMatrix lightSpace{};
        lightSpace.lightView = lightViewMatrix;
        lightSpace.lightProj = lightProj;
        lightSpace.lightProj[1][1] *= -1;
        lightSpaceUniformBuffers[currentImage].updateUniformBuffer(lightSpace);
    }

    void recordShadowCommandBuffer(uint32_t currentFrame, const std::vector<Object>& objects) const
    {
        vkResetCommandBuffer(shadowCommandBuffers[currentFrame], 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(shadowCommandBuffers[currentFrame], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording shadow command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = shadowRenderPass.renderPass;
        renderPassInfo.framebuffer = shadowMapFramebuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };

        VkClearValue clearValue{};
        clearValue.depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(shadowCommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(shadowCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicsPipeline.graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(shadowCommandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
        vkCmdSetScissor(shadowCommandBuffers[currentFrame], 0, 1, &scissor);

        // 绑定光源空间矩阵的uniform buffer
        vkCmdBindDescriptorSets(shadowCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicsPipeline.pipelineLayout, 0, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);

        for (const auto& object : objects)
        {
            VkBuffer vertexBuffers[] = { object.mesh.vertexBuffer.buffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(shadowCommandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(shadowCommandBuffers[currentFrame], object.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            
            ObjectModelMatrix objectModel{};
            objectModel.model = object.mesh.modelMatrix;
            vkCmdPushConstants(shadowCommandBuffers[currentFrame], shadowGraphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectModelMatrix), &objectModel);
            
            vkCmdDrawIndexed(shadowCommandBuffers[currentFrame], static_cast<uint32_t>(object.mesh.indexCount), 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(shadowCommandBuffers[currentFrame]);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = shadowMapImage.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(shadowCommandBuffers[currentFrame],
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);


        if (vkEndCommandBuffer(shadowCommandBuffers[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record shadow command buffer!");
        }
    }

    void destroyShadowResources(const VulkanDevice& vDevice) const
    {
        vkDestroyFramebuffer(vDevice.device, shadowMapFramebuffer, nullptr);
        vkDestroySampler(vDevice.device, shadowMapSampler, nullptr);
        shadowMapImage.ReleaseImage(vDevice);
        shadowGraphicsPipeline.destroyGraphicsPipeline(vDevice);
        shadowRenderPass.destroyRenderPass(vDevice);
        shadowDescriptorSetLayout.destroyDescriptorSetLayout(&vDevice);
    }
    
private:
    const uint32_t SHADOW_MAP_SIZE = 2048;
    const std::string SHADOW_VERTEX_SHADER_PATH = "./Shader/shadow_vert.spv";
    const std::string SHADOW_FRAGMENT_SHADER_PATH = "./Shader/shadow_frag.spv";
    
    VulkanRenderPass shadowRenderPass;
    VulkanGraphicsPipeline shadowGraphicsPipeline;
    VulkanDescriptorSetLayout shadowDescriptorSetLayout;
    VkFramebuffer shadowMapFramebuffer;
    std::vector<VkDescriptorSet> shadowDescriptorSets;

    void createShadowDescriptorSetLayout(const VulkanDevice& vDevice)
    {
        // 创建阴影描述符集布局
        VkDescriptorSetLayoutBinding lightSpaceUboLayoutBinding{};
        lightSpaceUboLayoutBinding.binding = 0;
        lightSpaceUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightSpaceUboLayoutBinding.descriptorCount = 1;
        lightSpaceUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        lightSpaceUboLayoutBinding.pImmutableSamplers = nullptr;
        std::vector<VkDescriptorSetLayoutBinding> shadowBindings = { lightSpaceUboLayoutBinding };
        shadowDescriptorSetLayout.createDescriptorSetLayout(&vDevice, shadowBindings.data(), static_cast<uint32_t>(shadowBindings.size()));
    }
    
    void createShadowRenderPass(const VulkanDevice& vDevice, VkFormat depthFormat)
    {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkSubpassDependency dependency2{};
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency2.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        std::array<VkSubpassDependency, 2> dependencies = { dependency, dependency2 };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        if (vkCreateRenderPass(vDevice.device, &renderPassInfo, nullptr, &shadowRenderPass.renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow render pass!");
        }
    }
    
    void createShadowPipeline(const VulkanDevice& vDevice, const VertexInputDescription& vertexInputDescription)
    {
        GraphicsPipelineConfig pipelineConfig;
        pipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipelineConfig.polygonMode = VK_POLYGON_MODE_FILL;
        pipelineConfig.cullMode = VK_CULL_MODE_FRONT_BIT;
        pipelineConfig.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipelineConfig.depthTest = VK_TRUE;
        pipelineConfig.depthWrite = VK_TRUE;

        Shader shadowVertShader(&vDevice);
        shadowVertShader.createShaderStageInfo(SHADOW_VERTEX_SHADER_PATH, VK_SHADER_STAGE_VERTEX_BIT);
        Shader shadowFragShader(&vDevice);
        shadowFragShader.createShaderStageInfo(SHADOW_FRAGMENT_SHADER_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);
        std::vector shaderStages = { shadowVertShader.getShaderStageInfo(), shadowFragShader.getShaderStageInfo() };
    
        shadowGraphicsPipeline.createGraphicsPipeline(vDevice, shadowRenderPass, shadowDescriptorSetLayout.descriptorSetLayout,
            vertexInputDescription, pipelineConfig, shaderStages, VK_SAMPLE_COUNT_1_BIT);
    }
    
    void createShadowMap(const VulkanPhysicalDevice& vPhysicalDevice, const VulkanDevice& vDevice, const VulkanCommandPool& vCommandPool, VkFormat depthFormat)
    {
        shadowMapImage.createImage(vPhysicalDevice, vDevice, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 1,
        VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
        shadowMapImage.createImageView(vDevice, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        VkCommandBuffer commandBuffer = vCommandPool.beginSingleTimeCommands(vDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = shadowMapImage.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
        
        vCommandPool.endSingleTimeCommands(vDevice, commandBuffer, vDevice.graphicsQueue);
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = shadowRenderPass.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &shadowMapImage.imageView;
        framebufferInfo.width = SHADOW_MAP_SIZE;
        framebufferInfo.height = SHADOW_MAP_SIZE;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vDevice.device, &framebufferInfo, nullptr, &shadowMapFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow map framebuffer!");
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(vDevice.device, &samplerInfo, nullptr, &shadowMapSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow map sampler!");
        }
    }

    void updateShadowDescriptors(const VulkanDevice& vDevice, const VulkanDescriptorPool& vDescriptorPool, const std::vector<VulkanUniformBuffer>& lightSpaceUniformBuffers, size_t maxFramesInFlight)
    {
        std::vector<VkDescriptorSetLayout> shadowLayouts(maxFramesInFlight, shadowDescriptorSetLayout.descriptorSetLayout);
        shadowDescriptorSets = vDescriptorPool.allocateDescriptorSets(&vDevice, shadowLayouts.data(), static_cast<uint32_t>(maxFramesInFlight));
        for (size_t i = 0; i < shadowDescriptorSets.size(); ++i){
            VkWriteDescriptorSet descriptorWrite{};
            VkDescriptorBufferInfo lightSpaceBufferInfo{};
            lightSpaceBufferInfo.buffer = lightSpaceUniformBuffers[i].vBuffer.buffer;
            lightSpaceBufferInfo.offset = 0;
            lightSpaceBufferInfo.range = sizeof(LightSpaceMatrix);
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = shadowDescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &lightSpaceBufferInfo;
            vkUpdateDescriptorSets(vDevice.device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void createShadowCommandBuffers(const VulkanDevice& vDevice, const VulkanCommandPool& vCommandPool, size_t maxFramesInFlight)
    {
        shadowCommandBuffers = vCommandPool.allocateCommandBuffers(vDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(maxFramesInFlight));
    }
};
