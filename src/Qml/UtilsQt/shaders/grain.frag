// Compile with:
//   qsb --glsl "100 es,120,150" --hlsl 50 --msl 12 -o grain.frag.qsb grain.frag

#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float intensity;   // Noise strength (0.0 - 1.0), default ~0.05
    float grainScale;  // UV multiplier for noise size, default ~400.0
    float seed;        // Seed for noise pattern variation, default 0.0
};

layout(binding = 1) uniform sampler2D source;

// Hash-based pseudo-random function for procedural noise
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec4 tex = texture(source, qt_TexCoord0);

    // Generate noise based on texture coordinates + seed offset
    float noise = rand(qt_TexCoord0 * grainScale + vec2(seed));

    // Apply noise to break color banding
    // Shift noise to [-0.5, 0.5] range, then scale by intensity
    vec3 grain = vec3((noise - 0.5) * intensity);

    // Add grain to the color
    vec3 result = tex.rgb + grain;

    fragColor = vec4(result, tex.a) * qt_Opacity;
}
