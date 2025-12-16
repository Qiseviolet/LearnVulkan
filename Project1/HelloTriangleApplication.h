#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <string>
#include <chrono>
#include "InputManager.h"
#include "VulkanCore/VulkanInstance.h"
#include "VulkanCore/VulkanSurface.h"
#include "VulkanCore/VulkanDevice.h"
#include "Model.h"
#include "Mesh.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "VulkanCore/VulkanDescriptorPool.h"
#include "VulkanCore/VulkanDescriptorSetLayout.h"
#include "VulkanCore/VulkanGraphicsPipeline.h"
#include "VulkanCore/VulkanRenderPass.h"
#include "VulkanCore/VulkanSwapChain.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "./Model/viking_room.obj";
const std::string TEXTURE_PATH = "./Texture/viking_room.png";
const std::string VERTEX_SHADER_PATH = "./Shader/vert.spv";
const std::string FRAGMENT_SHADER_PATH = "./Shader/frag.spv";

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class HelloTriangleApplication {
public:
    void run();
private:
    GLFWwindow* window;
    VulkanInstance vInstance;
    VulkanSurface vSurface;
    VulkanPhysicalDevice vPhysicalDevice;
    VulkanDevice vDevice;
    Model vModel;
    Mesh vMesh;
    Texture texture;
    InputManager* inputManager;
    CameraBase* camera;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    float mainLoopLastTime = 0.0f;
    std::vector<UniformBuffer> uniformBuffers;
    VkFormat depthFormat;
    VulkanSwapChain vSwapChain;
    bool framebufferResized = false;
    VulkanRenderPass vRenderPass;
    VulkanGraphicsPipeline vGraphicsPipeline;
    VulkanDescriptorPool vDescriptorPool;
    VulkanDescriptorSetLayout vDescriptorSetLayout;
    
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);
    
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }


    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void drawFrame();


    struct PushConstantData {
        glm::mat4 model;
    };

    std::vector<VkDescriptorSet> descriptorSets;
    void createDescriptorSets();
};
 