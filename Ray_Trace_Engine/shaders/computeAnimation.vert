#version 450 		// Use GLSL 4.5

const int MAX_BONE_INFLUENCE = 4;

struct Vertex {
    vec4 pos;
    vec4 col;
    vec4 tex;
    vec4 norm;
    ivec4 mBoneIDs;             // Only a single ivec4, not an array
    vec4 mWeights;  // Only the weights remain as an array
};

layout(std430, binding = 1) buffer CurrentFrameBuffer {
    Vertex vertices[];
}ssbo;

layout(set = 1, binding = 3) uniform UBO {
    mat4 projection;
    mat4 view;
    vec4 lightPosition;
    vec4 viewPos;
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

void main() {

    uint vertexId = gl_VertexIndex; 

    if(vertexId >= ssbo.vertices.length()) {
        return; 
    }

    // Fetch vertex attributes directly from the buffer
    vec4 vertexPos = ssbo.vertices[vertexId].pos;
    vec4 vertexCol = ssbo.vertices[vertexId].col;
    vec4 vertexTex = ssbo.vertices[vertexId].tex;
    vec4 vertexNorm = ssbo.vertices[vertexId].norm;

    fragPos = normalize(pushModel.model * vertexPos);
    fragColor = vertexCol;
    texCoords = vec2(vertexTex);
    normal = normalize(vec4(transpose(inverse(mat3(pushModel.model))) * vec3(vertexNorm), 1.0f));
    viewPosition = ubo.viewPos;

    gl_Position = ubo.projection * ubo.view * fragPos;

}
