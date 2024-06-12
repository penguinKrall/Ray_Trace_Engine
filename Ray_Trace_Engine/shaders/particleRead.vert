#version 450

const int MAX_BONES = 15;
const int MAX_BONE_INFLUENCE = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 norm;

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out flat ivec2 texelCoord;

void main(){

  //vec4 shadowPosition =  ubo.lightProjection * ubo.lightView * pushModel.model * pos;

  //gl_Position =  shadowPosition;

	//gl_Position = vec4(vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f, 0.0f, 1.0f);
    vec2 position = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0f - 1.0f;

    gl_Position = vec4(position, 0.0f, 1.0f);

    fragTexCoord = position * 0.5f + 0.5f;

    texelCoord = ivec2(fragTexCoord * vec2(1920.0f, 1024.0f));
}