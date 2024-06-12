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

layout(location = 0) out vec2 texCoords;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 fragPos;
void main() {
		
	//vec4 testPosition = inputBuffer.fragPos[0].fragPosition;
	fragPos = pushModel.model * pos;   
	texCoords = vec2(tex);
  outColor = col;
	//gl_Position = ubo.projection * ubo.view * pushModel.model * pos;
  //gl_Position = ubo.lightProjection * ubo.lightView * pushModel.model * vec4(pos.x, pos.y, pos.z, 1.0f);
	//gl_Position = testPosition;
  if(ubo.rtValues.x == 1.0f){
    gl_Position =  (ubo.projection * ubo.view * pushModel.model) * pos;
  }
  
  if(ubo.rtValues.x == 0.0f){
    gl_Position =  (ubo.lightProjection * ubo.lightView * pushModel.model) * pos;
  }
}