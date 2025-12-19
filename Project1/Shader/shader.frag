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

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

void main() {
    //归一化光照方向
    vec3 lightDir = normalize(lightUBO.direction);
    vec3 norm = normalize(fragNormal);
    //漫反射
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightUBO.diffuse * diff;
    //视线方向
    vec3 viewDir = normalize(cameraUBO.pos - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);
    //镜面反射
    float spec = pow(max(dot(norm, halfDir), 0.0), lightUBO.shininess);
    vec3 specular = lightUBO.specular * spec;
    //环境光
    vec3 ambient = lightUBO.ambient;
    
    vec4 lightColor = vec4(ambient + diffuse + specular, 1);
    vec4 texColor = texture(texSampler, fragTexCoord);
    outColor = vec4(texColor.rgb * lightColor.rgb, texColor.a);
}