#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 7) uniform sampler2D fullResColorAttachment;

void main() {
    float strength = 1.0f;
    float threshold = 0.1f;

    outColor = texture(fullResColorAttachment, texCoords);

    float brightness = dot(outColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    if (brightness > threshold) {
    outColor *= strength;
    } 

    else {
      outColor = vec4(0.0f);
    }
}