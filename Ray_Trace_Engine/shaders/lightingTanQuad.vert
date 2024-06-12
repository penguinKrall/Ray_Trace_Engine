#version 450 		// Use GLSL 4.5

const int MAX_BONES = 15;
const int MAX_BONE_INFLUENCE = 4;

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec4 textureCoords;
layout(location = 3) in vec4 vertexNormal;
layout(location = 4) in vec4 tangent;
layout(location = 5) in vec4 bittangent;

//layout(set = 0, binding = 0) uniform UBO {
//		mat4 lightProjection;
//		mat4 lightView;
//		vec4 lightPosition;
//} ubo;

layout(set = 0, binding = 0) uniform BonesUBO {
		mat4 projection;
		mat4 view;
    vec4 viewPosition;
		mat4 finalBonesMatrices[MAX_BONES];
} bonesUBO;

layout(push_constant) uniform PushModel {
	mat4 model;
} pushModel;

layout(location = 0) out vec2 texCoords;

void main() {

    // Transform vertex position
    vec4 fragPos = pushModel.model * vertexPosition;   

   // vec4 fragPosLightSpace = ubo.lightProjection * ubo.lightView * fragPos;

    vec4 fragColor = vertexColor;

    texCoords = vec2(textureCoords);

    vec4 fragNormal = vertexNormal;

    // Construct the TBN matrix for normal mapping
    mat3 normalMatrix = transpose(inverse(mat3(pushModel.model)));
    vec3 T = normalize(normalMatrix * vec3(tangent));
    vec3 N = normalize(normalMatrix * vec3(vertexNormal));
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));    

    //fragLightPos = vec4(TBN * vec3(ubo.lightPosition), 1.0f);
    //fragViewPosition  = vec4(TBN * vec3(ubo.viewPos), 1.0f);
    //fragPos  = vec4(TBN * vec3(fragPos), 1.0f);


    gl_Position = bonesUBO.projection * bonesUBO.view * pushModel.model * vertexPosition;
    //gl_Position = ubo.lightProjection * ubo.lightView * pushModel.model * vertexPosition;
}