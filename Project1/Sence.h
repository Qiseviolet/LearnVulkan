#pragma once
#include "vulkan/vulkan.h"
#include <GLFW/glfw3.h>
#include "InputManager.h"
#include "Common/Model.h"
#include "Common/Mesh.h"
#include "Common/Texture.h"
#include "Camera/CameraFPS.h"
#include "VulkanCore/VulkanInstance.h"
#include "VulkanCore/VulkanSurface.h"
#include "VulkanCore/VulkanDevice.h"
#include "VulkanCore/VulkanUniformBuffer.h"
#include "VulkanCore/VulkanCommandPool.h"
#include "VulkanCore/VulkanDescriptorPool.h"
#include "VulkanCore/VulkanDescriptorSetLayout.h"
#include "VulkanCore/VulkanFence.h"
#include "VulkanCore/VulkanGraphicsPipeline.h"
#include "VulkanCore/VulkanRenderPass.h"
#include "VulkanCore/VulkanSemaphore.h"
#include "VulkanCore/VulkanSwapChain.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string TEXTURE_PATH = "./Texture/wood_diff.jpg";
const std::string VERTEX_SHADER_PATH = "./Shader/vert.spv";
const std::string FRAGMENT_SHADER_PATH = "./Shader/frag.spv";

struct PushConstantData {
    glm::mat4 model;
};

class Sence {
private:
    GLFWwindow* window;
    VulkanInstance vInstance;
    VulkanSurface vSurface;
    VulkanPhysicalDevice vPhysicalDevice;
    VulkanDevice vDevice;
    std::vector<Mesh> vMeshes;
    Texture texture;
    InputManager* inputManager;
    CameraBase* camera;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    float mainLoopLastTime = 0.0f;
    VkFormat depthFormat;
    VulkanSwapChain vSwapChain;
    VulkanRenderPass vRenderPass;
    
    VulkanDescriptorPool vDescriptorPool;
    std::vector<VkDescriptorPoolSize> loadDescriptorPoolSizes() const;
    
    VulkanGraphicsPipeline vGraphicsPipeline;
    VertexInputDescription loadVertexInputDescription() const;
    GraphicsPipelineConfig loadGraphicsPipelineConfig() const;
    
    VulkanDescriptorSetLayout vDescriptorSetLayout;
    std::vector<VkDescriptorSetLayoutBinding> createDescriptorSetLayoutBinding() const;
    
    std::vector<VulkanUniformBuffer> uniformBuffers;
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    bool framebufferResized = false;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<Sence*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    std::vector<VkDescriptorSet> descriptorSets;
    void updateDescriptorSets();
    
    std::vector<VkCommandBuffer> commandBuffers;
    VulkanCommandPool vCommandPool;
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;

    std::vector<VulkanSemaphore> imageAvailableSemaphores;
    std::vector<VulkanSemaphore> renderFinishedSemaphores;
    std::vector<VulkanFence> inFlightFences;
    void createSyncObjects();
    
    uint32_t currentFrame = 0;
    void drawFrame();

    void drawMesh(const VkCommandBuffer& commandBuffer, const Mesh& mesh) const;

    void loadModel();
    
public:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
};
 