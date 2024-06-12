#version 450
//in from vertex shader
//layout(location = 0) in vec4 fragPos;
//layout(location = 1) in vec4 normal;

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoords;
layout (location = 2) in vec3 color;
layout (location = 3) in vec3 fragPos;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gBrightness;

layout(set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	vec4 viewPosition;
	vec4 lightPosition;
	float nearPlane;
	float farPlane;
	float pad1;
	float pad2;
	vec4 ssaoDebug;
	vec4 ssaoDebug2;
} ubo;

layout(set = 1, binding = 2) uniform sampler2DMS rendererColor;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - z * (ubo.farPlane - ubo.nearPlane));	
}
 
void main() { 
		gPosition = vec4(fragPos, linearDepth(gl_FragCoord.z));
		gNormal = vec4(normal, linearDepth(gl_FragCoord.z));
    gAlbedo = vec4(1.0f);

		float strength = 1.0f;
    float threshold = 0.1f;

    float brightness = dot(gAlbedo.rgb, vec3(0.2126, 0.7152, 0.0722));
    
    if (brightness > threshold) {
    gBrightness = vec4(vec3(gAlbedo * strength), 1.0f);
    } 

    else {
      gBrightness = vec4(0.0f);
    }
}
