#pragma once
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <limits>
#include <fstream>
#include <string>
#include <array>
#include <chrono>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string MODEL_PATH = "./Model/viking_room.obj";
const std::string TEXTURE_PATH = "./Texture/viking_room.png";

//设备扩展列表
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    
    //顶点绑定描述了整个顶点从内存加载数据的速率
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    //属性描述结构描述了如何从来自绑定描述的顶点数据块中提取顶点属性
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

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

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

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

    //创建用于初始化Vulkan库的实例
    void createInstance();
    //查看所需的全局拓展
    std::vector<const char*> getRequiredExtensions();

    //选择所需功能的显卡
    void pickPhysicalDevice();
    //设备评估
    bool isDeviceSuitable(VkPhysicalDevice device);
    //查找队列族：
    // 1.队列用于将命令提交到队列
    // 2.不同类型的队列来自不同队列族
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    //创建逻辑设备，用于与物理设备交互
    void createLogicalDevice();

    //创建窗口表面
    // 1.窗口表面建立Vulkan与窗口系统的链接，
    // 2.用于呈现渲染的图像
    void createSurface();

    //Vulkan没有默认帧缓冲的概念，需要交换链来拥有渲染的缓冲
    //交换链本质上是一个等待呈现到屏幕的图像队列
    
    //检查所有必须的拓展
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    //查询交换链支持的详细信息
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    //选择交换链的正确设置
    // 1.表面格式
    // 2.呈现模式
    // 3.交换范围
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    //创建交换链
    void createSwapChain();

    //创建图像视图
    // 1.在渲染管线中使用任何VkImage，必须创建一个VkImageView对象
    // 2.描述了如何访问图像以及访问图像的那一部分
    void createImageViews();

    //读取着色器文件
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
    //创建渲染管线
    void createGraphicsPipeline();
    //创建着色器模块
    VkShaderModule createShaderModule(const std::vector<char>& code);

    //创建渲染过程
    // 1.指定颜色和深度缓冲区
    // 2.缓冲区的采样方式
    void createRenderPass();

    //创建帧缓冲
    void createFramebuffers();

    //创建命令缓冲
    //Vulkan中命令不是通过函数调用直接执行的，必须要将所有要执行的操作记录在命令缓冲区对象中
    //命令缓冲区中所有命令会一起提交

    //创建命令池
    //命令池管理用于存储缓冲区的内存，命令缓冲区从其中分配
    void createCommandPool();
    //分配命令缓冲区
    void createCommandBuffers();
    //将要执行的命令写入命令缓冲区
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //创建同步对象
    // 1.一个信号量来表示已从交换链获取图像并准备好渲染
    // 2.一个信号量来表示渲染已完成并且可以进行呈现
    // 3.一个栅栏来确保一次只渲染一帧
    void createSyncObjects();

    void drawFrame();

    //窗口大小发生改变时，窗口表面发生变化，交换链不再兼容
    //需要重新创建交换链
    void recreateSwapChain();

    void cleanupSwapChain();

    //标记是否发生了调整大小的操作
    bool framebufferResized = false;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    /*const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };*/

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    
    void createVertexBuffer();

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    //最佳内存具有VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT标志
    //创建两个顶点缓冲区
    // 一个是CPU可访问内存的暂存缓冲，用于将数据从顶点数据上传到该缓冲
    // 另外一个是位于设备本地内存中的顶点缓冲
    void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags, VkBuffer&, VkDeviceMemory&);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    /*const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };*/

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    void createIndexBuffer();

    //资源描述符：着色器自由访问缓冲区和图像等资源的一种方式
    // 1.在管线期间创建指定描述符集布局
    // 2.从描述符池中分配描述符集
    // 3.在渲染期间绑定描述符集
    VkDescriptorSetLayout descriptorSetLayout;
    //为管线创建中使用的每个描述符绑定提供详细信息
    void createDescriptorSetLayout();
    //包含着色器UBO数据的缓冲区
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);

    VkDescriptorPool descriptorPool;
    //描述符集不能直接创建，必须像命令缓冲区一样从池中分配
    void createDescriptorPool();
    std::vector<VkDescriptorSet> descriptorSets;
    //分配描述符集
    void createDescriptorSets();

    uint32_t mipLevels;

    //加载图像并上传到Vulkan图像对象中
    void createTextureImage();
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

    //辅助函数
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    //布局转换
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayOut, uint32_t mipLevels);
    //缓冲区复制到图像
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    VkImageView textureImageView;
    void createTextureImageView();
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
    VkSampler textureSampler;
    void createTextureSampler();

    //深度图像和视图
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    void createDepthResources();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    void loadModel();

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    //多重采样
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlagBits getMaxUsableSampleCount();
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    void createColorResources();
};
 