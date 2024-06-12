#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D mip0;
layout(set = 0, binding = 5) uniform sampler2DMS hdrColor;

layout(set = 2, binding = 0) uniform UBO {
    vec4 hdrBlur;
    vec4 hdrTonemap;
    vec4 horizontalBlur;
    vec4 verticalBlur;
    vec4 bloomMix;
    vec4 bloomTonemap;
} ubo;

void main(){
    
    vec4 upsampleColor = texture(mip0, texCoords);

    vec4 accumulatedColor = vec4(0.0);

    for (int i = 0; i < 8; ++i) {
        accumulatedColor += texelFetch(hdrColor, ivec2((texCoords) * textureSize(hdrColor)), i);
    }

    outColor = accumulatedColor / 8.0f;

    outColor = mix(accumulatedColor, upsampleColor, ubo.hdrBlur.z); 
}