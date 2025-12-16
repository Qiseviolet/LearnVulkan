#pragma once
#include "VulkanInstance.h"

class VulkanSurface
{
private:
    VulkanInstance* instance = nullptr;
    GLFWwindow* window = nullptr;
public:
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VulkanSurface() = default;
    
    VulkanSurface(VulkanInstance* instance, GLFWwindow* window): instance(instance), window(window), surface(VK_NULL_HANDLE){}

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance->instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void destroySurface()
    {
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance->instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
    }
    
};
