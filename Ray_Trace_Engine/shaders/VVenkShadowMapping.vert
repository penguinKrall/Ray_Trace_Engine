#version 450 		// Use GLSL 4.5
#define GLM_SWIZZLE

//for the love of fucking god make sure this is FIFTEEN and FOUR in every shader using this bullshit
//no wonder nothing worked right
const int max_bones = 15;
const int max_bone_influence = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 nor;

layout(set = 0, binding = 0) uniform UBO {
		mat4 lightProjection;
		mat4 lightView;
		vec4 lightPosition;
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
} pushModel;

//layout(location = 0) out vec4 outColor;

void main() {

 //outColor = col;

 gl_Position =  (ubo.lightProjection * ubo.lightView * pushModel.model) * pos;


	}