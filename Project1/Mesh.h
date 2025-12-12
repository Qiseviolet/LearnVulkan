#pragma once
#include "vulkan/vulkan.h"
#include <glm/glm.hpp>
#include "Model.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"

struct Mesh
{
	VulkanBuffer vertexBuffer;
	VulkanBuffer indexBuffer;

	uint64_t indexCount;
	uint64_t vertexCount;
	glm::mat4 modelMatrix;
 
	void createVertexBuffer(const VulkanPhysicalDevice* vPhysicalDevice, const VulkanDevice* vDevice, const Model& model, VkCommandPool commandPool) {
		VkDeviceSize bufferSize = sizeof(model.vertices[0]) * model.vertices.size();

		VulkanBuffer stagingBuffer;
		stagingBuffer.createBuffer(vPhysicalDevice, vDevice, bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(vDevice->device, stagingBuffer.memory, 0, bufferSize, 0, &data);
		memcpy(data, model.vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(vDevice->device, stagingBuffer.memory);

		vertexBuffer.createBuffer(vPhysicalDevice, vDevice, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vertexBuffer.copyBuffer(vDevice, commandPool, stagingBuffer.buffer, bufferSize);
		vertexCount = model.vertices.size();
		stagingBuffer.releaseBuffer(vDevice);
	}

	void createIndexBuffer(const VulkanPhysicalDevice* vPhysicalDevice, const VulkanDevice* vDevice, const Model& model, VkCommandPool commandPool) {
		VkDeviceSize bufferSize = sizeof(model.indices[0]) * model.indices.size();

		VulkanBuffer stagingBuffer;
		stagingBuffer.createBuffer(vPhysicalDevice, vDevice, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(vDevice->device, stagingBuffer.memory, 0, bufferSize, 0, &data);
		memcpy(data, model.indices.data(), (size_t)bufferSize);
		vkUnmapMemory(vDevice->device, stagingBuffer.memory);

		indexBuffer.createBuffer(vPhysicalDevice, vDevice, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		indexBuffer.copyBuffer(vDevice, commandPool, stagingBuffer.buffer, bufferSize);
		indexCount = model.indices.size();
		stagingBuffer.releaseBuffer(vDevice);
	}

	void destroyMesh(const VulkanDevice* vDevice) const
	{
		vertexBuffer.releaseBuffer(vDevice);
		indexBuffer.releaseBuffer(vDevice);
	}
};
