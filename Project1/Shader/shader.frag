#version 450
layout(binding = 0) uniform CameraMatrixUbo{
    vec3 pos;
    mat4 view;
    mat4 proj;
} cameraUBO;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform lightBufferObject {
    vec3 direction; 
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} lightUBO;

layout(binding = 4) uniform sampler2D shadowMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) in vec4 fragLightSpacePos;

layout(location = 0) out vec4 outColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    vec3 texCoords = projCoords * 0.5 + 0.5;
    if(texCoords.x < 0.0 || texCoords.x > 1.0 || texCoords.y < 0.0 || texCoords.y > 1.0)
        return 0.0;
    float closestDepth = texture(shadowMap, texCoords.xy).r;
    float bias = 0.005;
    float shadow = projCoords.z - bias> closestDepth ? 1.0 : 0.0;
    return shadow;
}

void main() {
    vec3 lightDir = normalize(lightUBO.direction);
    vec3 norm = normalize(fragNormal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightUBO.diffuse * diff;
    vec3 viewDir = normalize(cameraUBO.pos - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), lightUBO.shininess);
    vec3 specular = lightUBO.specular * spec;
    vec3 ambient = lightUBO.ambient;
    
    float shadow = ShadowCalculation(fragLightSpacePos);
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    outColor = vec4(texColor.rgb * lighting, texColor.a);
    //outColor = vec4(shadow, shadow, shadow, 1.0);
}