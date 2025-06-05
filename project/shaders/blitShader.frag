#version 450

// Input texture (from your framebuffer)
layout(binding = 0) uniform sampler2D inputTexture;

// Texture coordinates from vertex shader
layout(location = 0) in vec2 texCoord;

// Final output
layout(location = 0) out vec4 outColor;



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

void main()
{
    vec4 color = texture(inputTexture, texCoord);

    // Apply exposure here
    vec3 exposedColor = color.rgb * 1;

   vec3 tonemapped = Uncharted2Tonemap(exposedColor);

    outColor = vec4(tonemapped, 1);
    //outColor = vec4(color.rgb,1);
}