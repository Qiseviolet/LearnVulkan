#include "Sence.h"
#include "Camera/CameraData.h"
#include "Light/DirectionalLight.h"
#include "Light/LightSpaceMatrix.h"
#include "Common/Shader.h"

void Sence::initWindow() {
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

void Sence::initVulkan() {
    vInstance.createInstance("Vulkan");

    vSurface.createSurface(vInstance, window);

    vPhysicalDevice.pickPhysicalDevice(vInstance, vSurface);

    vDevice.createLogicalDevice(vPhysicalDevice, vSurface);

    vSwapChain.createSwapChain(vPhysicalDevice, vDevice, window, vSurface);

    msaaSamples = vPhysicalDevice.getMaxUsableSampleCount();
    depthFormat = vPhysicalDevice.findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    vRenderPass.createRenderPass(vDevice, vSwapChain.swapChainImageFormat, depthFormat, msaaSamples);

    std::vector<VkDescriptorSetLayoutBinding> bindings = createDescriptorSetLayoutBinding();
    vDescriptorSetLayout.createDescriptorSetLayout(&vDevice, bindings.data(), static_cast<uint32_t>(bindings.size()));

    VertexInputDescription vertexInputDescription = loadVertexInputDescription();
    GraphicsPipelineConfig pipelineConfig = loadGraphicsPipelineConfig();
    Shader vertShader(&vDevice);
    vertShader.createShaderStageInfo(VERTEX_SHADER_PATH, VK_SHADER_STAGE_VERTEX_BIT);
    Shader fragShader(&vDevice);
    fragShader.createShaderStageInfo(FRAGMENT_SHADER_PATH, VK_SHADER_STAGE_FRAGMENT_BIT);
    std::vector shaderStages = { vertShader.getShaderStageInfo(), fragShader.getShaderStageInfo() };
    vGraphicsPipeline.createGraphicsPipeline(vDevice, vRenderPass, vDescriptorSetLayout.descriptorSetLayout, vertexInputDescription, pipelineConfig, shaderStages, msaaSamples);

    vSwapChain.createColorResources(vPhysicalDevice, vDevice, msaaSamples);
    vSwapChain.createDepthResources(vPhysicalDevice, vDevice, msaaSamples);
    vSwapChain.createFramebuffers(vDevice, vRenderPass.renderPass);
    vCommandPool.createCommandPool(vPhysicalDevice, vDevice, vSurface);
    std::vector<VkDescriptorPoolSize> poolSizes = loadDescriptorPoolSizes();
    vDescriptorPool.createDescriptorPool(&vDevice,static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 100);
    createUniformBuffers();
    createLight();
    shadow.createShadowResources(vPhysicalDevice, vDevice, vCommandPool, depthFormat, vertexInputDescription,
        vDescriptorPool, lightSpaceUniformBuffers, MAX_FRAMES_IN_FLIGHT);
    loadObjects();
    commandBuffers = vCommandPool.allocateCommandBuffers(vDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
    createSyncObjects();
}

void Sence::createLight()
{
    DirectionalLight directionalLight;
    directionalLight.direction = glm::vec3(1.0f, 1.0f, 1.0f);
    directionalLight.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    directionalLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    directionalLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    directionalLight.shininess = 32.0;
    for (size_t i = 0; i < lightUniformBuffers.size(); ++i)
    {
        lightUniformBuffers[i].updateUniformBuffer(directionalLight);
    }
}

void Sence::loadObjects()
{
    static const std::string sphereObj = "./Model/sphere.obj";
    objects.resize(5);
    objects[0].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        (glm::translate(glm::mat4(1.0f), glm::vec3(-3.0f, 0.0f, 0.0f))),
        "./Texture/lightgold_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadow.shadowMapImage.imageView, shadow.shadowMapSampler);
    objects[1].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
        "./Texture/dark-wood-stain_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadow.shadowMapImage.imageView, shadow.shadowMapSampler);
    objects[2].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        "./Texture/stylized-cave-wall1_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadow.shadowMapImage.imageView, shadow.shadowMapSampler);
    objects[3].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f)),
        "./Texture/houndstooth-fabric-weave_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadow.shadowMapImage.imageView, shadow.shadowMapSampler);
    static const std::string planeObj = "./Model/cube.obj";
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 0.1f));
    objects[4].loadObject(vPhysicalDevice, vDevice, vCommandPool, planeObj,
        modelMatrix,
        "./Texture/wood_diff.jpg", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadow.shadowMapImage.imageView, shadow.shadowMapSampler); 
}


void Sence::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        float current = static_cast<float>(glfwGetTime());
        float deltaTime = current - mainLoopLastTime;
        inputManager->Update(deltaTime);
        drawFrame();
        mainLoopLastTime = current;
    }
    vkDeviceWaitIdle(vDevice.device);
}

void Sence::cleanup() {
    vSwapChain.cleanSwapChain(vDevice);
    for (auto& object : objects)
    {
        object.mesh.destroyMesh(vDevice);
        object.texture.destroyTexture(vDevice);
    }
    shadow.destroyShadowResources(vDevice);
    vGraphicsPipeline.destroyGraphicsPipeline(vDevice);
    vRenderPass.destroyRenderPass(vDevice);
    for (auto& uniformBuffer : cameraUniformBuffers)
        uniformBuffer.destroyUniformBuffer(vDevice);
    for (auto& uniformBuffer : lightUniformBuffers)
        uniformBuffer.destroyUniformBuffer(vDevice);
    for (auto& uniformBuffer : lightSpaceUniformBuffers)
        uniformBuffer.destroyUniformBuffer(vDevice);
    vDescriptorPool.destroyDescriptorPool(&vDevice);
    vDescriptorSetLayout.destroyDescriptorSetLayout(&vDevice);
    for (auto& semaphore : imageAvailableSemaphores)
        semaphore.destroySemaphore(vDevice);
    for (auto& semaphore : renderFinishedSemaphores)
        semaphore.destroySemaphore(vDevice);
    for (auto& fence : inFlightFences)
        fence.destroyFence(vDevice);
    vCommandPool.destroyCommandPool(vDevice);
    vDevice.destroyDevice();
    vSurface.destroySurface(vInstance);
    vInstance.destroyInstance();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void Sence::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
{
    vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    
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

    for (auto& object : objects)
    {
        drawObject(commandBuffer, object);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Sence::drawObject(const VkCommandBuffer& commandBuffer, const Object& object) const
{
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vGraphicsPipeline.pipelineLayout, 0, 1, &object.descriptorSets[currentFrame], 0, nullptr);
    VkBuffer vertexBuffers[] = { object.mesh.vertexBuffer.buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, object.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    ObjectModelMatrix objectModel{};
    objectModel.model = object.mesh.modelMatrix;
    vkCmdPushConstants(commandBuffer, vGraphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectModelMatrix), &objectModel);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.mesh.indexCount), 1, 0, 0, 0);
}

void Sence::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        imageAvailableSemaphores[i].createSemaphore(vDevice);
        renderFinishedSemaphores[i].createSemaphore(vDevice);
        inFlightFences[i].createFence(vDevice, true);
    }
}

void Sence::drawFrame() {
    vkWaitForFences(vDevice.device, 1, &inFlightFences[currentFrame].fence, VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vDevice.device, vSwapChain.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame].semaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        vSwapChain.recreateSwapChain(vPhysicalDevice, vDevice, window, vSurface, vRenderPass.renderPass, msaaSamples);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    shadow.updateLightSpaceMatrix(currentFrame, camera->GetViewMatrix(), lightSpaceUniformBuffers);
    updateCameraUniformBuffer(currentFrame);

    vkResetFences(vDevice.device, 1, &inFlightFences[currentFrame].fence);
    shadow.recordShadowCommandBuffer(currentFrame, objects);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame].semaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    std::array<VkCommandBuffer, 2> submitCommandBuffers = { shadow.shadowCommandBuffers[currentFrame], commandBuffers[currentFrame] };
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame].semaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vDevice.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame].fence) != VK_SUCCESS) {
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
        vSwapChain.recreateSwapChain(vPhysicalDevice, vDevice, window, vSurface, vRenderPass.renderPass, msaaSamples);

    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Sence::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(CameraData);
    cameraUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        cameraUniformBuffers[i].createUniformBuffer(vPhysicalDevice, vDevice, bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    VkDeviceSize lightBufferSize = sizeof(DirectionalLight);
    lightUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        lightUniformBuffers[i].createUniformBuffer(vPhysicalDevice, vDevice, lightBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    VkDeviceSize lightSpaceBufferSize = sizeof(LightSpaceMatrix);
    lightSpaceUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        lightSpaceUniformBuffers[i].createUniformBuffer(vPhysicalDevice, vDevice, lightSpaceBufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

void Sence::updateCameraUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    CameraData cameraUbo{};
    cameraUbo.position = camera->GetPosition();
    cameraUbo.view = camera->GetViewMatrix();
    cameraUbo.proj = glm::perspective(glm::radians(camera->GetFovy()),
        static_cast<float>(vSwapChain.swapChainExtent.width) / static_cast<float>(vSwapChain.swapChainExtent.height),
        0.1f, 100.0f);
    cameraUbo.proj[1][1] *= -1;
    cameraUniformBuffers[currentImage].updateUniformBuffer(cameraUbo);
}

std::vector<VkDescriptorSetLayoutBinding> Sence::createDescriptorSetLayoutBinding() const
{
    VkDescriptorSetLayoutBinding cameraUboLayoutBinding{};
    cameraUboLayoutBinding.binding = 0;
    cameraUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraUboLayoutBinding.descriptorCount = 1;
    cameraUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    cameraUboLayoutBinding.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutBinding lightUboLayout{};
    lightUboLayout.binding = 2;
    lightUboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightUboLayout.descriptorCount = 1;
    lightUboLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightUboLayout.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutBinding lightSpaceUboLayout{};
    lightSpaceUboLayout.binding = 3;
    lightSpaceUboLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightSpaceUboLayout.descriptorCount = 1;
    lightSpaceUboLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lightSpaceUboLayout.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutBinding shadowMapLayoutBinding{};
    shadowMapLayoutBinding.binding = 4;
    shadowMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMapLayoutBinding.descriptorCount = 1;
    shadowMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowMapLayoutBinding.pImmutableSamplers = nullptr;
    return { cameraUboLayoutBinding, samplerLayoutBinding, lightUboLayout, lightSpaceUboLayout, shadowMapLayoutBinding };
}

VertexInputDescription Sence::loadVertexInputDescription() const
{
    VertexInputDescription vertexInputDescription;
    vertexInputDescription.bindingDescription = Vertex::getBindingDescription();
    vertexInputDescription.attributeDescriptions = Vertex::getAttributeDescriptions();
    return vertexInputDescription;
}

GraphicsPipelineConfig Sence::loadGraphicsPipelineConfig() const
{
    GraphicsPipelineConfig pipelineConfig;
    pipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineConfig.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineConfig.depthTest = VK_TRUE;
    pipelineConfig.depthWrite = VK_TRUE;
    return pipelineConfig;
}

std::vector<VkDescriptorPoolSize> Sence::loadDescriptorPoolSizes() const
{
    std::vector<VkDescriptorPoolSize> poolSizes{};

    VkDescriptorPoolSize uniformBufferPoolSize{};
    uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 3); // camera, light, lightSpace
    poolSizes.push_back(uniformBufferPoolSize);

    VkDescriptorPoolSize samplerPoolSize{};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2); // texture, shadowMap
    poolSizes.push_back(samplerPoolSize);

    return poolSizes;
}
