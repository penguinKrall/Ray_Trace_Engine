#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

hitAttributeEXT vec2 attribs;

struct RayPayload {
    vec3 color;
    float distance;
    vec3 normal;
    float reflector;
    vec4 accumulatedColor;
    float accumulatedAlpha;
    float index;
    vec3 bgTest;
    int semiTransparentFlag;
    vec4 colorID;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 2) rayPayloadEXT bool shadowed;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 3, set = 0) uniform UBO {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 lightPos;
} ubo;

struct GeometryNode {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    int textureIndexBaseColor;
    int textureIndexOcclusion;
    int textureIndexMetallicRoughness;
    int textureIndexNormal;
    int textureIndexEmissive;
    int semiTransparentFlag;
    float objectColorID;
};

struct GeometryIndex {
    int offset;
};

layout(binding = 4, set = 0) buffer G_Nodes_Buffer { GeometryNode nodes[]; } g_nodes_buffer;
layout(binding = 5, set = 0) buffer G_Nodes_Index { GeometryIndex indices[]; } g_nodes_indices;
layout(binding = 6, set = 0) uniform sampler2D glassTexture;
layout(binding = 8, set = 0) uniform sampler2D textures[];

#include "main_renderer_bufferreferences.glsl"
#include "main_renderer_geometrytypes.glsl"

void main() {

    //get geometry node index
    uint instanceCustomIndex = gl_InstanceCustomIndexEXT;

    //unpack triangles
    Triangle tri = unpackTriangle2(gl_PrimitiveID, 112);

    //get geometry node data from buffer
    GeometryNode geometryNode = g_nodes_buffer.nodes[gl_GeometryIndexEXT + g_nodes_indices.indices[instanceCustomIndex].offset];

    //set ray payload semi transparent flag
    rayPayload.semiTransparentFlag = geometryNode.semiTransparentFlag;

    //set ray payload color ID data
    rayPayload.colorID = vec4(vec3(geometryNode.objectColorID), 1.0f);

    //default color
    vec4 color = vec4(1.0f);

    //assign texture color if geometry node has texture
    if (geometryNode.textureIndexBaseColor > -1) {

        color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], tri.uv);

        //set semi transparency flag according to base color alpha
        if(color.a < 1.0f){
            rayPayload.semiTransparentFlag = 1;
        }
    } 
    
    //assign vertex color to color if no texture
    else {
        color = vec4(tri.color.rgb, 1.0f); // Use vertex color if no texture
    }

    //assign occlusion color map if model has one -- currently unused?
    if (geometryNode.textureIndexOcclusion > -1) {
        float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], tri.uv).r;
        color *= occlusion;
    }

    //distance used by reflection
    rayPayload.distance = gl_RayTmaxEXT;

    //assign ray payload normal from tri - can rewrite this to use normal map texture if available
    rayPayload.normal = normalize(tri.normal.xyz);

    //assign ray payload color
    rayPayload.color = color.rgb;

    //assign ray payload index - referred to in ray gen?
    rayPayload.index = float(gl_InstanceCustomIndexEXT);

    // Basic lighting
    vec3 lightVector = normalize(ubo.lightPos.xyz);
    float dot_product = max(dot(lightVector, tri.normal.xyz), 0.95);
    rayPayload.color *= dot_product;

    // Shadow casting
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + tri.normal.xyz * epsilon;
    shadowed = true;

    // Objects with full white vertex color are treated as reflectors
    rayPayload.reflector = ((color.r == 1.0f) && (color.g == 1.0f) && (color.b == 1.0f)) ? 1.0f : 0.0f;

    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
    
    //blend shadow
    if (shadowed) {
        rayPayload.color *= 0.7;
    }

}