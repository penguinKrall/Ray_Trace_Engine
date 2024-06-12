#version 450 		// Use GLSL 4.5
#define GLM_SWIZZLE

const int MAX_BONES = 15;
const int MAX_BONE_INFLUENCE = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 norm;
layout(location = 4) in ivec4 boneIds;
layout(location = 5) in vec4 weights;

//note the first descriptor set, vpUniformBuffer, in VulkanRenderer.cpp. 
//see record commands, create descriptor set, (create uniform buffer - utilities.hpp)
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

//push constant from record commands, described in createPushConstants and pushConstantRanges
layout(push_constant) uniform PushModel {
		mat4 model;
} pushModel;


layout(location = 0) out vec2 texCoords;

void main() {
    //values for the animation and GL_position
	  vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    vec4 localPosition = vec4(0.0f);

    //loop through bones
    for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++) {

        if(boneIds[i] == -1) {
            continue;
        }

        if(boneIds[i] >= MAX_BONES) {
            totalPosition = pos;
            break;
        }

        //calculate new position with transform matrices and model position
        localPosition = ubo.finalBonesMatrices[boneIds[i]] * pos;      
        totalPosition += localPosition * weights[i];

   }

    texCoords = vec2(tex);

   if(ubo.rtValues.x == 1.0f){
    gl_Position =  ubo.projection * ubo.view * pushModel.model * totalPosition;
   }

   else{
    gl_Position =  ubo.lightProjection * ubo.lightView * pushModel.model * totalPosition;
   }
}