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
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 2) rayPayloadEXT bool shadowed;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;

layout(binding = 2, set = 0) uniform UBO {
    mat4 viewInverse;
    mat4 projInverse;
    vec4 lightPos;
} ubo;

struct GeometryNode {
    uint64_t vertexBufferDeviceAddress;
    uint64_t indexBufferDeviceAddress;
    int textureIndexBaseColor;
    int textureIndexOcclusion;
};

struct GeometryIndex {
    int offset;
};

layout(binding = 3, set = 0) buffer G_Nodes_Buffer { GeometryNode nodes[]; } g_nodes_buffer;
layout(binding = 4, set = 0) buffer G_Nodes_Index { GeometryIndex indices[]; } g_nodes_indices;
layout(binding = 5, set = 0) uniform sampler2D glassTexture;
layout(binding = 6, set = 0) uniform sampler2D textures[];

#include "build_scene_bufferreferences.glsl"
#include "build_scene_geometrytypes.glsl"

//void main() {
//    uint instanceCustomIndex = gl_InstanceCustomIndexEXT;
//    GeometryNode geometryNode = g_nodes_buffer.nodes[gl_GeometryIndexEXT + g_nodes_indices.indices[instanceCustomIndex].offset];
//
//    Triangle tri = unpackTriangle2(gl_PrimitiveID, 112);
//
//    vec4 color = vec4(1.0f); // Default color with full opacity
//
//    if (geometryNode.textureIndexBaseColor > -1) {
//        color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], attribs);
//    }
//    else {
//        color = tri.color; // Default color with full opacity
//    }
//
//    if (gl_InstanceCustomIndexEXT == 2) { // Check for the third model with semi-transparent texture
//        if (color.a < 1.0f) { // Only adjust alpha if the texture is semi-transparent
//            color.a *= 0.7f; // Adjust alpha to 70%
//        }
//    }
//
//    if (gl_InstanceCustomIndexEXT > 1) {
//        color = texture(glassTexture, attribs);
//    }
//
//    if (geometryNode.textureIndexOcclusion > -1) {
//        float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], attribs).a;
//        color.a *= occlusion; // Adjust alpha based on occlusion texture
//    }
//
//    rayPayload.color = color.rgb;
//    rayPayload.accumulatedColor.rgb = mix(rayPayload.accumulatedColor.rgb, color.rgb, color.a); // Blend colors based on alpha
//    rayPayload.accumulatedAlpha = mix(rayPayload.accumulatedAlpha, 1.0, color.a); // Update accumulated alpha
//
//    rayPayload.distance = (color.a < 0.001) ? -1.0 : gl_RayTmaxEXT; // Set distance to negative if fully transparent
//
//    // Basic lighting
//    vec3 lightVector = normalize(gl_WorldRayOriginEXT - ubo.lightPos.xyz);
//    float dot_product = max(dot(lightVector, gl_WorldRayDirectionEXT), 0.95);
//    rayPayload.color *= dot_product;
//
//    // Shadow casting
//    float tmin = 0.001;
//    float tmax = 10000.0;
//    float epsilon = 0.001;
//    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + gl_WorldRayDirectionEXT * epsilon;
//    shadowed = true;
//
//    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
//    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
//        0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
//
//    if (shadowed) {
//        rayPayload.color *= 0.7;
//    }
//
//    // Objects with full white vertex color are treated as reflectors
//    rayPayload.reflector = (geometryNode.textureIndexBaseColor > -1) ? 0.0f :
//                           ((tri.vertices[0].color.r == 1.0f) && (tri.vertices[0].color.g == 1.0f) && (tri.vertices[0].color.b == 1.0f)) ? 1.0f : 0.0f;
//
//    rayPayload.index = gl_InstanceCustomIndexEXT;
//}

void main() {
    uint instanceCustomIndex = gl_InstanceCustomIndexEXT;
    Triangle tri = unpackTriangle2(gl_PrimitiveID, 112);
    GeometryNode geometryNode = g_nodes_buffer.nodes[gl_GeometryIndexEXT + g_nodes_indices.indices[instanceCustomIndex].offset];

    vec4 color = vec4(0.0f); // Default color

    if (geometryNode.textureIndexBaseColor > -1) {
        color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], tri.uv);
    } 
    
    else {
        color = vec4(tri.color.rgb, 1.0f); // Use vertex color if no texture
    }

    // Apply transparency only to the third model (custom index 2)
    if (instanceCustomIndex == 2) {
        float transparency = 0.2; // Example transparency value
        color *= transparency;
    }

    if (geometryNode.textureIndexOcclusion > -1) {
        float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], tri.uv).r;
        color *= occlusion;
    }

    rayPayload.distance = gl_RayTmaxEXT;
    rayPayload.normal = normalize(tri.normal.xyz);
    rayPayload.color = color.rgb;

    // Basic lighting
    vec3 lightVector = normalize(tri.position.xyz - ubo.lightPos.xyz);
    float dot_product = max(dot(lightVector, tri.normal.xyz), 0.95);
    rayPayload.color *= dot_product;

    // Shadow casting
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + tri.normal.xyz * epsilon;
    shadowed = true;

    // Objects with full white vertex color are treated as reflectors
    rayPayload.reflector = (geometryNode.textureIndexBaseColor > -1) ? 0.0f :
                           ((tri.vertices[0].color.r == 1.0f) && (tri.vertices[0].color.g == 1.0f) && (tri.vertices[0].color.b == 1.0f)) ? 1.0f : 0.0f;

    rayPayload.index = float(gl_InstanceCustomIndexEXT);

    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);

    if (shadowed) {
        rayPayload.color *= 0.7;
    }

}



//#version 460
//
//#extension GL_EXT_ray_tracing : require
//#extension GL_GOOGLE_include_directive : require
//#extension GL_EXT_nonuniform_qualifier : require
//#extension GL_EXT_buffer_reference2 : require
//#extension GL_EXT_scalar_block_layout : require
//#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
//
//struct RayPayload {
//    vec3 color;
//    float distance;
//    vec3 normal;
//    float reflector;
//    vec4 accumulatedColor;
//    float accumulatedAlpha;
//    float index;
//};
//
//layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
//layout(location = 2) rayPayloadEXT bool shadowed;
//hitAttributeEXT vec2 attribs;
//
//layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
//
//layout(binding = 2, set = 0) uniform UBO {
//    mat4 viewInverse;
//    mat4 projInverse;
//    vec4 lightPos;
//} ubo;
//
//struct GeometryNode {
//    uint64_t vertexBufferDeviceAddress;
//    uint64_t indexBufferDeviceAddress;
//    int textureIndexBaseColor;
//    int textureIndexOcclusion;
//};
//
//struct GeometryIndex {
//    int offset;
//};
//
//layout(binding = 3, set = 0) buffer G_Nodes_Buffer { GeometryNode nodes[]; } g_nodes_buffer;
//
//layout(binding = 4, set = 0) buffer G_Nodes_Index { GeometryIndex indices[]; } g_nodes_indices;
//
//layout(binding = 5, set = 0) uniform sampler2D glassTexture;
//
//layout(binding = 6, set = 0) uniform sampler2D textures[];
//
//#include "build_scene_bufferreferences.glsl"
//#include "build_scene_geometrytypes.glsl"
//





