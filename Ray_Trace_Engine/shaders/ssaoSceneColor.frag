#version 450

layout(location = 0) in vec2 texCoords;

layout(set = 0, binding = 2) uniform sampler2DMS sceneColor;

layout(location = 0) out vec4 outColor;


void main(){

		vec4 accumulatedColor = vec4(0.0);

    for (int i = 0; i < 8; ++i) {
       // accumulatedColor += vec4(gaussianBlur(sceneColor, texCoords, i, blurRadius), 1.0);
		
        accumulatedColor += texelFetch(sceneColor, ivec2(texCoords * textureSize(sceneColor)), i);
    }
		
    outColor = accumulatedColor / 8.0f;

		}