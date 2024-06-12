#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2DMS sceneColor;

const int num_samples = 8;

void main() { 

    vec4 accumulatedColor = vec4(0.0);

    for (int i = 0; i < num_samples; ++i) {
        accumulatedColor += texelFetch(sceneColor, ivec2(texCoords * textureSize(sceneColor)), i);
    }

    vec4 fragColor = accumulatedColor / float(num_samples);
    
    outColor = fragColor;
}
