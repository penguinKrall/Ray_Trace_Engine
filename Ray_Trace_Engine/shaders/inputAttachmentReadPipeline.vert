#version 450

#define GLM_SWIZZLE

const int MAX_BONES = 15;
const int MAX_BONE_INFLUENCE = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 norm;

layout(set = 0, binding = 3) uniform UBO {
		mat4 projection;
		mat4 view;
		vec4 lightPosition;
		vec4 viewPos;
		mat4 finalBonesMatrices[MAX_BONES];
		mat4 lightProjection;
		mat4 lightView;
		vec4 rtValues;
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
} pushModel;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out mat4 projectionMatrix;
layout(location = 5) out mat4 viewMatrix;
layout(location = 9) out vec4 viewPosition;
//layout(location = 10) out mat4 lightViewProjMatrix;

void main(){

		vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;

		gl_Position = vec4(position, 0.0f, 1.0f);

		fragTexCoord = gl_Position.xy * 0.5f + 0.5f;

		projectionMatrix = ubo.projection;
		
		viewMatrix =  ubo.view;

		viewPosition = ubo.viewPos;

		//lightViewProjMatrix = ubo.lightView * ubo.lightProjection;

}