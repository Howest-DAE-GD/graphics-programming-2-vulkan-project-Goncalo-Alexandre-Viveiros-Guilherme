#version 450

layout(push_constant) uniform PushConstants {
    mat4 modelMatrix;
    uint materialIndex;
} pushConstants;

layout(binding = 0) uniform UniformBufferObject {
    mat4 sceneMatrix;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;

void main() 
{
    gl_Position = ubo.proj * ubo.view * ubo.sceneMatrix * pushConstants.modelMatrix * vec4(inPosition, 1.0);
}