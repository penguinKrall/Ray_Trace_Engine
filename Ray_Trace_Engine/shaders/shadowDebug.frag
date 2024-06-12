#version 450

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D depthMap;

void main(){
		
	vec4 depthColor = texture(depthMap, texCoords); 

  outColor = depthColor + vec4(0.0f, depthColor.r, depthColor.r, 1.0f); 

}
