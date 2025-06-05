#version 450
#extension GL_EXT_nonuniform_qualifier: enable

layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    uint albedoMapIndex;
    uint aoMapIndex;
    uint normalMapIndex;
    uint metallicRoughnessMapIndex; 

} pushConstants;

layout(binding = 1) uniform sampler texSampler;
layout(binding = 2) uniform texture2D textures[];

layout(location = 0) in vec3 fragColor;      
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in mat3 fragTBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMetallicRoughness;

void main()
{
    vec3 albedoColor = texture(sampler2D(textures[nonuniformEXT(pushConstants.albedoMapIndex)], texSampler), fragTexCoord).rgb;
    //float aoValue = texture(sampler2D(textures[nonuniformEXT(pushConstants.aoMapIndex)], texSampler), fragTexCoord).r;

    //outAlbedoAO = vec4(albedoColor, aoValue);
    outAlbedo = vec4(albedoColor,1);
    vec3 tangentSpaceNormal = texture(sampler2D(textures[nonuniformEXT(pushConstants.normalMapIndex)], texSampler), fragTexCoord).rgb;
    tangentSpaceNormal = tangentSpaceNormal * 2.0 - 1.0;
    vec3 worldSpaceNormal = normalize(fragTBN * tangentSpaceNormal);

    outNormal = normalize(vec4(worldSpaceNormal * 0.5 + 0.5, 1.0));

    vec4 metallicRoughnessSample = texture(sampler2D(textures[nonuniformEXT(pushConstants.metallicRoughnessMapIndex)], texSampler), fragTexCoord).rgba;

    outMetallicRoughness = vec4(metallicRoughnessSample.g, metallicRoughnessSample.b, 0.0, 0.0);

}