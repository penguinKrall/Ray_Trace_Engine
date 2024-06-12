#version 450 		// Use GLSL 4.5
#define GLM_SWIZZLE

const int max_bones = 15;
const int max_bone_influence = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 nor;
layout(location = 4) in ivec4 boneIds;
layout(location = 5) in vec4 weights;

layout(set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	vec4 viewPosition;
	mat4 finalBonesMatrices[max_bones];
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
} pushModel;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 texCoords;
layout(location = 3) out vec4 normal;
layout(location = 4) out vec4 viewPosition;
layout(location = 5) out vec3 albedoFragPos;

void main() {

		//values for the animation and GL_position
	  vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);

    //loop through bones
    for(int i = 0 ; i < max_bone_influence ; i++) {
        if(boneIds[i] == -1) {
            continue;
            }
        if(boneIds[i] >= max_bones) {
            totalPosition = pos;
            break;
        }

        //calculate new position with transform matrices and model position
        vec4 localPosition = ubo.finalBonesMatrices[boneIds[i]] * pos;        
        totalPosition += localPosition * weights[i];

        //calculate new normals with bone weights and model normals
        vec3 localNormal = transpose(inverse(mat3(ubo.finalBonesMatrices[boneIds[i]]))) * vec3(nor);
        totalNormal += localNormal * vec3(weights[i]);   
   }  

    fragPos = vec4(vec3(ubo.view * pushModel.model * pos), 1.0f);

    albedoFragPos = normalize(vec3(pushModel.model * pos));

		fragColor = col;

		texCoords = tex.xy;

		normal = normalize(vec4(transpose(inverse(mat3(pushModel.model))) * totalNormal, 1.0f));
    //normal = normalize(vec4(transpose(inverse(mat3(pushModel.model))) * vec3(norm), 1.0f));

		viewPosition = ubo.viewPosition;

		gl_Position =  ubo.projection * ubo.view * pushModel.model * totalPosition;

	}