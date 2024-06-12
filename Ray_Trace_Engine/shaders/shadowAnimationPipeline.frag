
#version 450

layout(location = 0) in vec2 texCoords;

//in from descriptor set in record commands
layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;

layout(location = 0) out vec4 outColor;	

void main() {

    outColor = texture(diffuseTexture, texCoords); 

}
