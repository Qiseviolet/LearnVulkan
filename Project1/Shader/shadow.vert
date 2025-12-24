#version 450

layout(binding = 0) uniform LightSpaceMatrixUbo {
    mat4 lightView;
    mat4 lightProj;
} lightSpaceUBO;

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstant {
    mat4 model;
} pushConstant;

void main() {
    gl_Position = lightSpaceUBO.lightProj * lightSpaceUBO.lightView * pushConstant.model * vec4(inPosition, 1.0);
}
