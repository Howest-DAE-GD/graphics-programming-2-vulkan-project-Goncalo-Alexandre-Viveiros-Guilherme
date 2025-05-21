#version 450
#extension GL_EXT_nonuniform_qualifier: enable

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint materialIndex;
} pushConstants;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(textures[nonuniformEXT(pushConstants.materialIndex)], texSampler), fragTexCoord).rgba;
}