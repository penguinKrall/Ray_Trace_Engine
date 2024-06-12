#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2DMS sceneColor;

layout(set = 1, binding = 0) uniform UBO {
    vec4 hdr;
    vec4 hdrTonemap;
    vec4 horizontalBlur;
    vec4 verticalBlur;
    vec4 bloomMix;
    vec4 bloomTonemap;
} ubo;

const float weights[5] = {0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162};

vec3 toneMapReinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 gaussianBlur(sampler2DMS image, vec2 texCoords, int sampleIndex, float blurRadius) {
    vec3 result = texelFetch(image, ivec2(texCoords * vec2(textureSize(image))), sampleIndex).rgb * weights[0];
    vec2 texOffset = 1.0 / vec2(textureSize(image));

    for(int i = 1; i < blurRadius; ++i) {
        vec2 offsetH = vec2(texOffset.x * float(i), 0.0);
        vec2 offsetV = vec2(0.0, texOffset.y * float(i));

        result += texelFetch(image, ivec2((texCoords + offsetH) * vec2(textureSize(image))), sampleIndex).rgb * weights[i];
        result += texelFetch(image, ivec2((texCoords - offsetH) * vec2(textureSize(image))), sampleIndex).rgb * weights[i];

        result += texelFetch(image, ivec2((texCoords + offsetV) * vec2(textureSize(image))), sampleIndex).rgb * weights[i];
        result += texelFetch(image, ivec2((texCoords - offsetV) * vec2(textureSize(image))), sampleIndex).rgb * weights[i];
    }

    return result;
}

void main() {
    float strength = ubo.hdr.x;
    float threshold = ubo.hdr.y;

    vec4 accumulatedColor = vec4(0.0);
    for (int i = 0; i < 8; ++i) {
       // accumulatedColor += vec4(gaussianBlur(sceneColor, texCoords, i, blurRadius), 1.0);

        accumulatedColor += texelFetch(sceneColor, ivec2(texCoords * textureSize(sceneColor)), i);
    }
    outColor = accumulatedColor / 8.0f;

    float brightness = dot(outColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    if (brightness > threshold) {
    outColor *= strength;
    } 

    else {
      outColor = vec4(0.0f);
    }
}

