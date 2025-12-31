#version 450
layout(binding = 0) uniform CameraMatrixUbo{
    vec3 pos;
    mat4 view;
    mat4 proj;
} cameraUBO;

layout(binding = 3) uniform LightSpaceMatrixUbo {
    mat4 lightView;
    mat4 lightProj;
} lightSpaceUBO;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;
layout(location = 5) in vec3 inBitangent;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragPosition;
layout(location = 4) out vec4 fragLightSpacePos;
layout(location = 5) out VS_OUT_NORMAL{
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

layout(push_constant) uniform PushConstant{
    mat4 model;
} pushConstant;

void main() {
    mat3 normalMatrix = transpose(inverse(mat3(pushConstant.model)));
    fragNormal = normalMatrix * inNormal;
    fragPosition = vec3(pushConstant.model * vec4(inPosition, 1.0));
    gl_Position = cameraUBO.proj * cameraUBO.view * pushConstant.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    
    // 计算光源空间位置
    vec4 worldPos = pushConstant.model * vec4(inPosition, 1.0);
    fragLightSpacePos = lightSpaceUBO.lightProj * lightSpaceUBO.lightView * worldPos;
    
    // 计算切线空间
    vec3 T = normalize(normalMatrix * inTangent);
    vec3 B = normalize(normalMatrix * inBitangent);
    vec3 N = normalize(fragNormal);
    mat3 TBN = transpose(mat3(T, B, N));
    vs_out.TangentLightPos = TBN * cameraUBO.pos;
    vs_out.TangentViewPos = TBN * cameraUBO.pos;
    vs_out.TangentFragPos = TBN * fragPosition;
}