#version 450

//layout(location = 0) in vec4 pos;
//layout(location = 1) in vec4 col;
//layout(location = 2) in vec4 tex;
//layout(location = 3) in vec4 nor;

layout(location = 0) out vec2 texCoords;

void main(){

		vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;

		gl_Position = vec4(position, 0.0f, 1.0f);

		texCoords = vec2(position * 0.5f + 0.5f);

		}