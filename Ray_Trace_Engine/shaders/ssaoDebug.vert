#version 450
#define MAX_BONES 4

layout(location = 0) out vec2 texCoords;
layout(location = 1) out vec4 viewPos;
layout(location = 2) out vec4 lightView;
layout(location = 3) out mat4 view;

layout(set = 1, binding = 3) uniform RendererUBO {
		mat4 projection;
		mat4 view;
		vec4 lightPosition;
		vec4 viewPos;
		mat4 finalBonesMatrices[MAX_BONES];
		mat4 lightProjection;
		mat4 lightView;
		vec4 rtValues;
} rendererUBO;

void main(){

		vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;

		gl_Position = vec4(position, 0.0f, 1.0f);

		texCoords = vec2(position * 0.5f + 0.5f);

		viewPos = rendererUBO.viewPos;

		lightView = vec4(mat3(rendererUBO.view) * vec3(rendererUBO.lightPosition), 1.0f);

		view = rendererUBO.view;

		}