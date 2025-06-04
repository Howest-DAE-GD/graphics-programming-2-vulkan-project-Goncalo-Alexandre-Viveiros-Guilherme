#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec2 TexCoords;
layout(location = 0) out vec4 FragColor;

// G-Buffer inputs
layout(binding = 0) uniform sampler2D gAlbedo;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gMetallicRoughness;
layout(binding = 4) uniform sampler2D gDepth;

layout(binding = 5) uniform UniformBufferObject {
    mat4 sceneMatrix;
    mat4 view;
    mat4 proj;
    mat4 invView;
} cameraUBO;

struct Light {
    vec3 Position;
    vec3 Color;
    float Radius;
};
const int NR_LIGHTS = 32;
layout(binding = 3) uniform Lights {
    Light lights[NR_LIGHTS];
    vec3 viewPos;
} ubo;

const float PI = 3.14159265359;

// Corrected PBR Functions with proper parameter types
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ReconstructWorldPos(vec2 uv) {
    float depth = texture(gDepth, uv).r;
    vec2 ndc = vec2(
    (float(gl_FragCoord.x) / textureSize(gDepth,0).x) * 2.0 - 1.0,
    (float(gl_FragCoord.y) / textureSize(gDepth,0).y) * 2.0 - 1.0
    ) ;
    vec4 clipPos = vec4( ndc , depth, 1.0);
    vec4 viewPos = inverse(cameraUBO.proj) * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = inverse(cameraUBO.view) * viewPos;
    return worldPos.xyz;
}

void main() {
    vec2 uv = TexCoords;
    vec3 FragPos = ReconstructWorldPos(uv);

    // Retrieve G-Buffer data
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    float metallic = texture(gMetallicRoughness, TexCoords).r;
    float roughness = texture(gMetallicRoughness, TexCoords).g;

    // Normalize normal
    vec3 N = normalize(normal);
    vec3 V = normalize(ubo.viewPos - FragPos);

    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < NR_LIGHTS; ++i) {
        // Light direction and distance
        vec3 L = normalize(ubo.lights[i].Position - FragPos);
        vec3 H = normalize(V + L);
        float distance = length(ubo.lights[i].Position - FragPos);
        float attenuation = 1.0 / (distance * distance + 0.01);
        vec3 radiance = ubo.lights[i].Color * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    FragColor = vec4(color, 1.0);
}