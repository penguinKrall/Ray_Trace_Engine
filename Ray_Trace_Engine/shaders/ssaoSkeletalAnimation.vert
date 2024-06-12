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
layout(set = 1, binding = 3) uniform UBO {
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


layout(location = 0) out vec3 normal;
layout(location = 1) out vec2 texCoords;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec3 fragPos;

void main() {

	  vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);

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
        vec4 localPosition = ubo.finalBonesMatrices[boneIds[i]] * pos;        
        totalPosition += localPosition * weights[i];

        //calculate new normals with bone weights and model normals
        vec3 localNormal = mat3(transpose(inverse(ubo.finalBonesMatrices[boneIds[i]]))) * vec3(norm);
        totalNormal += localNormal * vec3(weights[i]);   
   }  

   	//// Vertex position in view space
		//outPos = vec3(ubo.view * pushModel.model * pos);

    fragPos =  mat3(ubo.view) * mat3(pushModel.model) * totalPosition.xyz;   
    
    fragColor = col.rgb;
    
    texCoords = vec2(tex);
    
    // Normal in view space
		//mat3 normalMatrix = transpose(inverse(mat3(ubo.view * pushModel.model)));
    mat3 normalMatrix =transpose(inverse(mat3(pushModel.model * ubo.view)));

		normal = normalMatrix * totalNormal.xyz, 1.0f;

    gl_Position =  ubo.projection * ubo.view * pushModel.model * totalPosition;

}