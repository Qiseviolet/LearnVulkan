#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraMatrix
{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
