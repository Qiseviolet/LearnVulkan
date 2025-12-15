#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>
#include <limits>
#include <fstream>
#include <string>
#include <chrono>
#include "InputManager.h"
#include "VulkanDevice.h"
#include "Model.h"
#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"
#include "UniformBuffer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "./Model/viking_room.obj";
const std::string TEXTURE_PATH = "./Texture/viking_room.png";

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
    VkInstance instance;
    VkSurfaceKHR surface;
    VulkanPhysicalDevice vPhysicalDevice;
    VulkanDevice vDevice;
    Model vModel;
    Mesh vMesh;
    Texture texture;
    InputManager* inputManager;
    CameraBase* camera;
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkFormat depthFormat;
    VulkanImage _depthImage;
    VulkanImage _colorImage;
    float mainLoopLastTime = 0.0f;
    std::vector<UniformBuffer> uniformBuffers;

    void createInstance();
    void createSurface();
    std::vector<const char*> getRequiredExtensions();
    VkFormat findDepthFormat() const;
    void createDepthResources();
    void createColorResources();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

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

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();

    void createImageViews();

    
    void createGraphicsPipeline();

    void createRenderPass();

    void createFramebuffers();

    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void createSyncObjects();

    void drawFrame();

    void recreateSwapChain();

    void cleanupSwapChain();

    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    struct PushConstantData {
        glm::mat4 model;
    };


    VkDescriptorSetLayout descriptorSetLayout;
    void createDescriptorSetLayout();
    
    
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    
    VkDescriptorPool descriptorPool;
    void createDescriptorPool();
    std::vector<VkDescriptorSet> descriptorSets;
    void createDescriptorSets();

    
};
 