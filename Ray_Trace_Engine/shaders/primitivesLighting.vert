#version 450

const int MAX_BONES = 15;
const int MAX_BONE_INFLUENCE = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 nor;

layout(set = 0, binding = 0) uniform UBO {
		mat4 projection;
		mat4 view;
		vec4 viewPos;
		mat4 finalBonesMatrices[MAX_BONES];
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
} pushModel;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 texCoords;
layout(location = 3) out vec3 normal;
layout(location = 4) out vec3 viewPosition;

void main() {

	fragPos = mat3(pushModel.model) * pos.xyz;

	fragColor = col.rgb;

	texCoords = tex.xy;

  normal = mat3(transpose(inverse(pushModel.model))) * vec3(nor);

  viewPosition = ubo.viewPos.xyz;

	gl_Position = ubo.projection * ubo.view * pushModel.model * pos;

	}