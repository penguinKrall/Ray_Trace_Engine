#version 450 		// Use GLSL 4.5

const int max_bones = 15;
const int max_bone_influence = 4;

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexColor;
layout(location = 2) in vec4 textureCoords;
layout(location = 3) in vec4 vertexNormal;
layout(location = 4) in vec4 tangent;
layout(location = 5) in vec4 bittangent;

layout(set = 0, binding = 0) uniform UBO {
		mat4 projection;
		mat4 view;
    vec4 viewPosition;
} ubo;

layout(push_constant) uniform PushModel {
	mat4 model;
  float bias;
  float range;
  float render;
} pushModel;

layout(location = 0) out vec4 fragPos;
layout(location = 1) out vec4 fragColor;
layout(location = 2) out vec2 texCoords;
layout(location = 3) out vec4 normal;
layout(location = 4) out vec4 viewPosition;
//layout(location = 5) out mat3 TBN;

void main() {

    // Transform vertex position
    fragPos = pushModel.model * vertexPosition;
    
    fragColor = vertexColor;

    texCoords = vec2(textureCoords);
    
    normal = vertexNormal;

    viewPosition = ubo.viewPosition;

    //// Construct the TBN matrix for normal mapping
    //mat3 normalMatrix = transpose(inverse(mat3(pushModel.model)));
    //vec3 T = normalize(normalMatrix * vec3(tangent));
    //vec3 N = normalize(normalMatrix * vec3(vertexNormal));
    //vec3 B = cross(N, T);
    //
    //TBN = transpose(mat3(T, B, N));

    gl_Position = ubo.projection * ubo.view * pushModel.model * vertexPosition;

}