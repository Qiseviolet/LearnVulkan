#include "HelloTriangleApplication.h"
#include "CameraFPS.h"

void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    static CameraFPS fpsCamera(glm::vec3(0.0f, 5.0f, 5.0f), 0.0, 45.0f, 45.0f);
    camera = &fpsCamera;
    static InputManager input(window, camera);
    inputManager = &input;

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void HelloTriangleApplication::initVulkan() {
    vInstance = VulkanInstance();
    vInstance.createInstance("Vulkan");
    vSurface = VulkanSurface(&vInstance, window);
    vSurface.createSurface();
    vPhysicalDevice = VulkanPhysicalDevice(vInstance.instance, vSurface.surface);
    vPhysicalDevice.pickPhysicalDevice();
    vDevice = VulkanDevice(vPhysicalDevice);
    vDevice.createLogicalDevice();
    vSwapChain.createSwapChain(vPhysicalDevice, vDevice, window, vSurface.surface);
    msaaSamples = vPhysicalDevice.getMaxUsableSampleCount();
    depthFormat = vPhysicalDevice.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    vRenderPass.createRenderPass(vDevice, vSwapChain.swapChainImageFormat, depthFormat, msaaSamples);

    vDescriptorSetLayout = VulkanDescriptorSetLayout(&vDevice);
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    vDescriptorSetLayout.createDescriptorSetLayout(bindings.data(), static_cast<uint32_t>(bindings.size()));
    
    Shader vertShader(&vDevice);
    vertShader.createShaderStageInfo(VERTEX_SHADER_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    Shader fragShader(&vDevice);
    fragShader.createShaderStageInfo(FRAGMENT_SHADER_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);
    vGraphicsPipeline.shaderStages = { vertShader.getShaderStageInfo(), fragShader.getShaderStageInfo() };
    
    VertexInputDescription vertexInputDescription;
    vertexInputDescription.bindingDescription = Vertex::getBindingDescription();
    vertexInputDescription.attributeDescriptions = Vertex::getAttributeDescriptions();
    GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineConfig.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineConfig.depthTest = VK_TRUE;
    pipelineConfig.depthWrite = VK_TRUE;
    vGraphicsPipeline.createGraphicsPipeline(vDevice, vRenderPass, vDescriptorSetLayout.descriptorSetLayout, vertexInputDescription, pipelineConfig, msaaSamples);

    vSwapChain.createColorResources(vPhysicalDevice, vDevice, msaaSamples);
    vSwapChain.createDepthResources(vPhysicalDevice, vDevice, msaaSamples);
    vSwapChain.createFramebuffers(vDevice, vRenderPass.renderPass);
    
    createCommandPool();

    texture.loadTextureImage(vPhysicalDevice, vDevice, commandPool, TEXTURE_PATH, VK_FORMAT_R8G8B8A8_SRGB);
    texture.createTextureSampler(vPhysicalDevice, vDevice);
    vModel.loadModel(MODEL_PATH);
    vMesh.createVertexBuffer(vPhysicalDevice, vDevice, vModel, commandPool);
    vMesh.createIndexBuffer(vPhysicalDevice, vDevice, vModel, commandPool);
    vMesh.modelMatrix = glm::rotate(glm::mat4(1.0f), /*time * */glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    createUniformBuffers();

    vDescriptorPool = VulkanDescriptorPool(&vDevice);
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    vDescriptorPool.createDescriptorPool(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), MAX_FRAMES_IN_FLIGHT);
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApplication::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float current = glfwGetTime();
        float deltaTime = current - mainLoopLastTime;
        inputManager->Update(deltaTime);
        drawFrame();
        mainLoopLastTime = current;
    }

    vkDeviceWaitIdle(vDevice.device);
}

void HelloTriangleApplication::cleanup() {
    vSwapChain.cleanSwapChain(vDevice);
    texture.destroyTexture(vDevice);
    vGraphicsPipeline.destroyGraphicsPipeline(vDevice);
    vRenderPass.destroyRenderPass(vDevice);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i].destroyUniformBuffer(vDevice);
    }

    vDescriptorPool.destroyDescriptorPool();
    vDescriptorSetLayout.destroyDescriptorSetLayout();

    vMesh.destroyMesh(vDevice);
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(vDevice.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vDevice.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(vDevice.device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vDevice.device, commandPool, nullptr);

    vDevice.destroyDevice();
    vInstance.destroyInstance();
    vSurface.destroySurface();
    vkDestroyInstance(vInstance.instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
}


void HelloTriangleApplication::createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = vPhysicalDevice.findQueueFamilies(vPhysicalDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(vDevice.device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void HelloTriangleApplication::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(vDevice.device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vRenderPass.renderPass;
    renderPassInfo.framebuffer = vSwapChain.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vSwapChain.swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vGraphicsPipeline.graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vSwapChain.swapChainExtent.width);
    viewport.height = static_cast<float>(vSwapChain.swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vSwapChain.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vMesh.vertexBuffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, vMesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    PushConstantData pc{};
    pc.model = vMesh.modelMatrix;
    vkCmdPushConstants(commandBuffer, vGraphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pc);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vGraphicsPipeline.pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(vModel.indices.size()), 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void HelloTriangleApplication::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(vDevice.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vDevice.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vDevice.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void HelloTriangleApplication::drawFrame() {
    vkWaitForFences(vDevice.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vDevice.device, vSwapChain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vSwapChain.recreateSwapChain(vPhysicalDevice, vDevice, window, vSurface.surface, vRenderPass.renderPass, msaaSamples);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame);

    vkResetFences(vDevice.device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vDevice.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { vSwapChain.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(vDevice.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        vSwapChain.recreateSwapChain(vPhysicalDevice, vDevice, window, vSurface.surface, vRenderPass.renderPass, msaaSamples);

    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i].createUniformBuffer(vPhysicalDevice, vDevice, bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

void HelloTriangleApplication::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->GetViewMatrix();
    ubo.proj = glm::perspective(glm::radians(camera->GetFovy()),
        static_cast<float>(vSwapChain.swapChainExtent.width) / static_cast<float>(vSwapChain.swapChainExtent.height),
        0.1f, 100.0f);
    ubo.proj[1][1] *= -1;
    uniformBuffers[currentImage].updateUniformBuffer(ubo);
}

void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, vDescriptorSetLayout.descriptorSetLayout);
    descriptorSets = vDescriptorPool.allocateDescriptorSets(layouts.data(), static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i].vBuffer.buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

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

        vkUpdateDescriptorSets(vDevice.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
