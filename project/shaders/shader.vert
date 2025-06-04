#version 450

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint albedoMapIndex;
    uint aoMapIndex;
    uint normalMapIndex;
    uint metallicRoughnessMapIndex; 

} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    mat4 sceneMatrix;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent; 
layout(location = 5) in vec3 inBiTangent; 

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out mat3 fragTBN;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.sceneMatrix * pushConstants.modelMatrix * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    vec3 N = normalize(mat3(ubo.sceneMatrix * pushConstants.modelMatrix) * inNormal);
    vec3 T = normalize(mat3(ubo.sceneMatrix * pushConstants.modelMatrix) * inTangent);
    vec3 B = normalize(inBiTangent);



    fragTBN = mat3(T, B, N);
}