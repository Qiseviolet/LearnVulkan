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
layout(binding = 5) uniform sampler2D normalMap;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) in vec4 fragLightSpacePos;
layout(location = 5) in VS_OUT_NORMAL{
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

layout(location = 0) out vec4 outColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    vec3 texCoords = projCoords * 0.5 + 0.5;
    if(texCoords.x < 0.0 || texCoords.x > 1.0 || texCoords.y < 0.0 || texCoords.y > 1.0)
        return 0.0;
    float shadow = 0.0;
    float bias = 0.005;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            float pcfDepth = texture(shadowMap, texCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

void main() {

    vec3 normal = texture(normalMap, fragTexCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    //vec3 lightDir = normalize(lightUBO.direction);
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    //vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightUBO.diffuse * diff;
    
    //vec3 viewDir = normalize(cameraUBO.pos - fragPosition);
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), lightUBO.shininess);
    vec3 specular = lightUBO.specular * spec;
    vec3 ambient = lightUBO.ambient;
    
    float shadow = ShadowCalculation(fragLightSpacePos);
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
    outColor = vec4(texColor.rgb * lighting, texColor.a);
    //outColor = vec4(specular.xyz, 1.0);
}