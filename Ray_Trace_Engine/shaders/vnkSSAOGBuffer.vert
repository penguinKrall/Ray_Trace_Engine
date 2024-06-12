#version 450 		// Use GLSL 4.5
#define GLM_SWIZZLE

const int max_bones = 15;
const int max_bone_influence = 4;

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;
layout(location = 2) in vec4 tex;
layout(location = 3) in vec4 nor;
layout(location = 4) in vec4 tangent;
layout(location = 5) in vec4 bittangent;

layout(set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	vec4 viewPosition;
	mat4 finalBonesMatrices[max_bones];
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
		float cubePass;
		float spherePass;
		float quadPass;
		float animationPass;
		float cubemapPass;
} pushModel;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 texCoords;
layout(location = 3) out vec3 normal;
layout(location = 4) out vec3 viewPosition;
layout(location = 5) out vec3 albedoFragPos;
layout(location = 6) out vec3 inPos;
layout(location = 7) out vec3 reflectNormals;
layout(location = 8) out mat3 TBN;

void main() {

		if(pushModel.quadPass == 1.0f){
		// Transform vertex position
		    fragPos = mat3(pushModel.model) * pos.xyz;
		    
		    fragColor = col.rgb;
				
		    texCoords = tex.xy;
		    
		    normal = vec3(nor);
				
		    viewPosition = ubo.viewPosition.xyz;
				
		    gl_Position = ubo.projection * ubo.view * pushModel.model * pos;
				
				return;
				
				//gl_Position      = ubo.projection * ubo.view * pushModel.model * pos;
				//mat3 normalMatrix = transpose(inverse(mat3(pushModel.model)));
				//
        //vec3 T = normalize(normalMatrix * vec3(tangent));
        //vec3 N = normalize(normalMatrix * vec3(normalize(nor)));
				//vec3 B = cross(N, T);
				//TBN = transpose(mat3(T, B, N));
				//
				//fragPos				   = vec3(pushModel.model * vec4(vec3(pos), 1.0));
				////fragPos *= TBN;
				//texCoords				 = tex.xy;  
				//normal = vec3(nor);
				//
				////vs_out.TangentLightPos = TBN * lightPos;
				////vs_out.TangentViewPos  = TBN * viewPos;
				////vs_out.TangentFragPos  = TBN * vs_out.FragPos;
				//
				//return;
		}

		fragPos = vec3(ubo.view * pushModel.model * pos);
		albedoFragPos = vec3(pushModel.model * pos);
		fragColor = col.rgb;

		texCoords = tex.xy;

		//*max bones for brightness.animation looks good cus of this value
		normal = normalize(transpose(inverse(mat3(pushModel.model))) * (nor.xyz * max_bones)); 

		viewPosition = ubo.viewPosition.xyz;

		inPos = pos.xyz;

		reflectNormals = mat3(transpose(inverse(pushModel.model))) * nor.xyz;

		if(pushModel.cubemapPass == 1.0f){
		gl_Position = ubo.projection * mat4(mat3(ubo.view)) * pushModel.model * pos;
		}

		else{
		gl_Position = ubo.projection * ubo.view * pushModel.model * pos;
		}

	}