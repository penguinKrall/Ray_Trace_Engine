#version 450 		// Use GLSL 4.5
#define GLM_SWIZZLE

const int MAX_BONES = 15;
const int MAX_BONE_INFLUENCE = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 nor;

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

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 texCoords;
layout(location = 3) out vec4 normal;
layout(location = 4) out vec4 viewPosition;
layout(location = 5) out vec4 lightPos;
layout(location = 6) out vec4 fragPosLightSpace;
//struct ObjectData{
//	mat4 model;
//};
//
//layout(std140,set = 0, binding = 10) readonly buffer ObjectBuffer{
//	ObjectData objects[];
//} objectBuffer;

void main() {
		fragPos = pushModel.model * pos;
	//fragPos = objectBuffer.objects[0].model * pos;  

	fragColor = col;

	texCoords = vec2(tex);

  normal = vec4(mat3(transpose(inverse(pushModel.model))) * vec3(nor), 1.0f);
	//normal = vec4(mat3(transpose(inverse(objectBuffer.objects[0].model))) * vec3(nor), 1.0f);

  viewPosition = ubo.viewPos;

	lightPos = ubo.lightPosition;

	fragPosLightSpace = ubo.lightProjection * ubo.lightView * fragPos;

	gl_Position = ubo.projection * ubo.view * pushModel.model * pos;
  //gl_Position = ubo.projection * ubo.view * objectBuffer.objects[0].model * pos;

	}