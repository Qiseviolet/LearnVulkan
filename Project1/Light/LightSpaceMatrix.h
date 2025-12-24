#pragma once
#include <glm/glm.hpp>

struct LightSpaceMatrix
{
    alignas(16) glm::mat4 lightView;
    alignas(16) glm::mat4 lightProj;
};
