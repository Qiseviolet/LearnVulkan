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
    
    createUniformBuffers();
    createLight();

    std::vector<VkDescriptorPoolSize> poolSizes = loadDescriptorPoolSizes();
    vDescriptorPool.createDescriptorPool(&vDevice,static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 100);

    // 创建阴影描述符集布局
    VkDescriptorSetLayoutBinding lightSpaceUboLayoutBinding{};
    lightSpaceUboLayoutBinding.binding = 0;
    lightSpaceUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightSpaceUboLayoutBinding.descriptorCount = 1;
    lightSpaceUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    lightSpaceUboLayoutBinding.pImmutableSamplers = nullptr;
    std::vector<VkDescriptorSetLayoutBinding> shadowBindings = { lightSpaceUboLayoutBinding };
    shadowDescriptorSetLayout.createDescriptorSetLayout(&vDevice, shadowBindings.data(), static_cast<uint32_t>(shadowBindings.size()));
    
    createShadowResources();

    std::vector<VkDescriptorSetLayout> shadowLayouts(MAX_FRAMES_IN_FLIGHT, shadowDescriptorSetLayout.descriptorSetLayout);
    shadowDescriptorSets = vDescriptorPool.allocateDescriptorSets(&vDevice, shadowLayouts.data(), static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));
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
    loadObjects();
    commandBuffers = vCommandPool.allocateCommandBuffers(vDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
    shadowCommandBuffers = vCommandPool.allocateCommandBuffers(vDevice, VK_COMMAND_BUFFER_LEVEL_PRIMARY, MAX_FRAMES_IN_FLIGHT);
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
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadowMapImage.imageView, shadowMapSampler);
    objects[1].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)),
        "./Texture/dark-wood-stain_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadowMapImage.imageView, shadowMapSampler);
    objects[2].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        "./Texture/stylized-cave-wall1_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadowMapImage.imageView, shadowMapSampler);
    objects[3].loadObject(vPhysicalDevice, vDevice, vCommandPool, sphereObj,
        glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f)),
        "./Texture/houndstooth-fabric-weave_albedo.png", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadowMapImage.imageView, shadowMapSampler);
    static const std::string planeObj = "./Model/cube.obj";
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 50.0f, 0.1f));
    objects[4].loadObject(vPhysicalDevice, vDevice, vCommandPool, planeObj,
        modelMatrix,
        "./Texture/wood_diff.jpg", vDescriptorPool, vDescriptorSetLayout,
        cameraUniformBuffers, lightUniformBuffers, lightSpaceUniformBuffers, shadowMapImage.imageView, shadowMapSampler); 
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
    
    vkDestroyFramebuffer(vDevice.device, shadowMapFramebuffer, nullptr);
    vkDestroySampler(vDevice.device, shadowMapSampler, nullptr);
    shadowMapImage.ReleaseImage(vDevice);
    shadowGraphicsPipeline.destroyGraphicsPipeline(vDevice);
    shadowRenderPass.destroyRenderPass(vDevice);
    shadowDescriptorSetLayout.destroyDescriptorSetLayout(&vDevice);
    
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

    updateLightSpaceMatrix(currentFrame);
    updateCameraUniformBuffer(currentFrame);

    vkResetFences(vDevice.device, 1, &inFlightFences[currentFrame].fence);
    recordShadowCommandBuffer(shadowCommandBuffers[currentFrame], currentFrame);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame].semaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    std::array<VkCommandBuffer, 2> submitCommandBuffers = { shadowCommandBuffers[currentFrame], commandBuffers[currentFrame] };
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

void Sence::createShadowResources()
{
    createShadowRenderPass();
    createShadowMap();
    createShadowPipeline();
}

void Sence::createShadowRenderPass()
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

void Sence::createShadowMap()
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

void Sence::createShadowPipeline()
{
    VertexInputDescription vertexInputDescription = loadVertexInputDescription();
    
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

void Sence::updateLightSpaceMatrix(uint32_t currentImage)
{
    glm::mat4 lightProj = glm::orthoZO(-50.0f, 50.0f, -50.0f, 50.0f, 0.0f, 100.0f);
    LightSpaceMatrix lightSpace{};
    lightSpace.lightView = camera->GetViewMatrix();
    lightSpace.lightProj = lightProj;
    lightSpace.lightProj[1][1] *= -1;
    lightSpaceUniformBuffers[currentImage].updateUniformBuffer(lightSpace);
}

void Sence::recordShadowCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame) const
{
    vkResetCommandBuffer(commandBuffer, 0);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
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

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicsPipeline.graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { SHADOW_MAP_SIZE, SHADOW_MAP_SIZE };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // 绑定光源空间矩阵的uniform buffer
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowGraphicsPipeline.pipelineLayout, 0, 1, &shadowDescriptorSets[currentFrame], 0, nullptr);

    for (const auto& object : objects)
    {
        VkBuffer vertexBuffers[] = { object.mesh.vertexBuffer.buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, object.mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        
        ObjectModelMatrix objectModel{};
        objectModel.model = object.mesh.modelMatrix;
        vkCmdPushConstants(commandBuffer, shadowGraphicsPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ObjectModelMatrix), &objectModel);
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.mesh.indexCount), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

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
    
    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);


    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record shadow command buffer!");
    }
}