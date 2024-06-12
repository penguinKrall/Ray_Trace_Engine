#version 450

layout(location = 0) out vec2 texCoords;

void main(){

		vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;

		gl_Position = vec4(position, 0.0f, 1.0f);

		texCoords = vec2(position * 0.5f + 0.5f);

		}