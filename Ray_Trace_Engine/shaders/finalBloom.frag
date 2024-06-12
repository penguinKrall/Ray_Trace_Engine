#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2DMS sceneColor;
layout(set = 0, binding = 3) uniform sampler2DMS blurredColor;

const float bloomIntensity = 0.8; // Adjust as needed to achieve desired bloom effect
const int num_samples = 8;        // Adjust based on how many samples you are using in your MSAA

vec4 resolveMS(sampler2DMS image, vec2 coords) {
    vec4 accumulatedColor = vec4(0.0);
    for (int i = 0; i < num_samples; ++i) {
        accumulatedColor += texelFetch(image, ivec2(coords * textureSize(image)), i);
    }
    return accumulatedColor / float(num_samples);
}

void main(){

    vec4 scene = resolveMS(sceneColor, texCoords);
    vec4 bloom = resolveMS(blurredColor, texCoords);

    outColor = mix(scene, scene + bloom * bloomIntensity, bloomIntensity);

}
