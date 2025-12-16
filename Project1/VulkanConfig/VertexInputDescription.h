#pragma once
#include "vulkan/vulkan.h"
#include <vector>

struct VertexInputDescription
{
    VkVertexInputBindingDescription bindingDescription;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};
