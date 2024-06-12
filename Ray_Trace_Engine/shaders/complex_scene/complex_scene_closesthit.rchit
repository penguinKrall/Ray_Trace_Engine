#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
  //
struct RayPayload {
    vec3 color;
    float distance;
    vec3 normal;
    float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 2, set = 0) uniform UBO {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 lightPos;
} ubo;

layout(binding = 3, set = 0) uniform sampler2D image;

struct GeometryNode {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    int textureIndexBaseColor;
    int textureIndexOcclusion;
};
layout(binding = 4, set = 0) buffer GeometryNodes { GeometryNode nodes[]; } geometryNodes;

layout(binding = 5, set = 0) uniform sampler2D textures[];

#include "complex_scene_bufferreferences.glsl"
#include "complex_scene_geometrytypes.glsl"

void main()
{
    Triangle tri = unpackTriangle(gl_PrimitiveID, 112);
    rayPayload.color = tri.normal.xyz;

    GeometryNode geometryNode = geometryNodes.nodes[gl_GeometryIndexEXT];

    vec3 color = vec3(1.0f);

    // Check if the texture index is valid
    if (geometryNode.textureIndexBaseColor > -1) {
            color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], tri.uv.xy).rgb;
    } 

    else {
        color = tri.color.rgb; // Use vertex color if no texture
    }

    if (geometryNode.textureIndexOcclusion > -1) {
        float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], tri.uv).r;
        color *= occlusion;
    }

    rayPayload.color = vec3(color.r);

    rayPayload.distance = gl_RayTmaxEXT;
    rayPayload.normal = normalize(tri.normal.xyz);

    
    // Shadow casting
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + tri.normal.xyz * epsilon;

    shadowed = true;  

    // Basic lighting
    vec3 lightVector = normalize(tri.position.xyz - ubo.lightPos.xyz);
    float dot_product = max(dot(lightVector, tri.normal.xyz), 0.95);
    
    rayPayload.color = vec3(color * dot_product);

    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsNoneEXT | gl_RayFlagsSkipClosestHitShaderEXT,
    0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
    
    if (shadowed) {
        rayPayload.color *= 0.7;
    }

     // Objects with full white vertex color are treated as reflectors
    if(geometryNode.textureIndexBaseColor > -1) {
        rayPayload.reflector = 0.0f;
    }

    else{
        rayPayload.reflector = ((tri.vertices[0].color.r == 1.0f) && (tri.vertices[0].color.g == 1.0f) && (tri.vertices[0].color.b == 1.0f)) ? 1.0f : 0.0f;
    }
}

