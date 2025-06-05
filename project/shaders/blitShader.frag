#version 450

#define SUNNY_16
//#define INDOOR

// Input texture (from your framebuffer)
layout(binding = 0) uniform sampler2D inputTexture;

// Texture coordinates from vertex shader
layout(location = 0) in vec2 texCoord;

// Final output
layout(location = 0) out vec4 outColor;

//layout(push_constant) uniform PushConstants
//{
//    float aperture;
//    float shutterTime;
//    float ISO;
//} cameraEV;

vec3 Uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

float CalculateEV100FromPhysicalCamera(in float aperture,in float shutterTime, in float ISO)
{
    return log2(pow(aperture,2) / shutterTime * 100 / ISO);
}

float ConvertEV100ToExposure(in float EV100)
{
    const float maxLuminance = 1.2f * pow(2.f, EV100);
    return 1.f / max(maxLuminance, 0.0001f);
}

float CalculateEV100FromAverageLuminance(in float averageLuminance)
{
    const float K = 12.f;
    return log2((averageLuminance * 100.0f) / K);
}

void main()
{
    vec4 color = texture(inputTexture, texCoord);

    #ifdef SUNNY_16
        float aperture = 5.f;
        float ISO = 100.f;
        float shutterSpeed = 1.f / 200.f;
    #endif

    #ifdef INDOOR
        float aperture = 1.4f;
        float ISO = 1600.0f;
        float shutterSpeed = 1.f / 60.f;
    #endif

    const float EV100_HardCoded = 1.f;
    const float EV100_PhysicalCamera = CalculateEV100FromPhysicalCamera(aperture,shutterSpeed, ISO);

    float exposure = ConvertEV100ToExposure(EV100_PhysicalCamera);

    // Apply exposure here
    vec3 exposedColor = color.rgb * exposure;

    vec3 tonemapped = Uncharted2Tonemap(exposedColor);

    outColor = vec4(tonemapped, 1);
    //outColor = vec4(color.rgb,1);
}