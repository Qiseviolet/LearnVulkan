#pragma once
#include "VulkanInstance.h"
#include <GLFW/glfw3.h>

class VulkanSurface
{
public:
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    
    void createSurface(const VulkanInstance& instance, GLFWwindow* win)
    {
        if (glfwCreateWindowSurface(instance.instance, win, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void destroySurface(const VulkanInstance& instance)
    {
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance.instance, surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
    }
    
};
