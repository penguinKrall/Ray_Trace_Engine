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
		float cubemapPass;
} pushModel;

layout(location = 0) out vec2 texCoords;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 viewPosition;
layout(location = 4) out vec3 reflectNormals;
layout(location = 5) out vec3 inPos;

void main() {

		fragPos = vec3(pushModel.model * pos);

		texCoords = tex.xy;

		normalize(transpose(inverse(mat3(pushModel.model))) * (nor.xyz * max_bones));
		//normal = nor.xyz; 

		viewPosition = ubo.viewPosition.xyz;

		reflectNormals = mat3(transpose(inverse(pushModel.model))) * nor.xyz;

		inPos = pos.xyz;

		if(pushModel.cubemapPass == 1.0f){
		gl_Position = ubo.projection * mat4(mat3(ubo.view)) * pushModel.model * pos;
		}

		else{
		gl_Position = ubo.projection * ubo.view * pushModel.model * pos;
		}
	}