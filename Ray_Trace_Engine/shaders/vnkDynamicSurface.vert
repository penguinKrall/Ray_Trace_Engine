#version 450 		// Use GLSL 4.5
#define GLM_SWIZZLE

const int max_bones = 15;
const int max_bone_influence = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 nor;

layout(set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	vec4 viewPosition;
	mat4 finalBonesMatrices[max_bones];
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
		float depth;
} pushModel;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec3 texCoords;
layout(location = 3) out vec3 normal;
layout(location = 4) out vec3 viewPosition;

void main() {

		fragPos = mat3(pushModel.model) * pos.xyz;
		
		fragColor = col.rgb;
		
		texCoords = vec3(tex.xy, pushModel.depth);
		
		normal = vec3(nor);
		
		viewPosition = ubo.viewPosition.xyz;

		gl_Position = ubo.projection * ubo.view * pushModel.model * pos;

	}