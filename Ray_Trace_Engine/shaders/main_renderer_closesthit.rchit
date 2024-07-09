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
    vec4 viewPos;
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
layout(binding = 7, set = 0) uniform samplerCube cubemapTexture;
layout(binding = 8, set = 0) uniform sampler2D textures[];

#include "main_renderer_bufferreferences.glsl"
#include "main_renderer_geometrytypes.glsl"

vec3 blinnPhong(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 diffuseColor, vec3 ambientColor, vec3 specularColor) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = max(dot(normal, halfDir), 0.0);
    float specular = pow(specAngle, 16.0);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * diffuseColor;
    vec3 ambient = ambientColor;
    vec3 spec = specular * specularColor;

    return ambient + diffuse + spec;
}

void main() {
    // Get geometry node index
    uint instanceCustomIndex = gl_InstanceCustomIndexEXT;

    // Unpack triangles
    Triangle tri = unpackTriangle2(gl_PrimitiveID, 112);

    // Get geometry node data from buffer
    GeometryNode geometryNode = g_nodes_buffer.nodes[gl_GeometryIndexEXT + g_nodes_indices.indices[instanceCustomIndex].offset];

    // Set ray payload semi-transparent flag
    rayPayload.semiTransparentFlag = geometryNode.semiTransparentFlag;

    // Set ray payload color ID data
    rayPayload.colorID = vec4(vec3(geometryNode.objectColorID), 1.0f);

    // Default color
    vec4 color = vec4(1.0f);

    // Assign texture color if geometry node has texture
    if (geometryNode.textureIndexBaseColor > -1) {
        color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], tri.uv);
        // Set semi-transparency flag according to base color alpha
        if (color.a < 1.0f) {
            rayPayload.semiTransparentFlag = 1;
        }
    } else {
        // Assign vertex color to color if no texture
        color = vec4(tri.color.rgb, 1.0f);
    }

    // Assign occlusion color map if model has one -- currently unused?
    if (geometryNode.textureIndexOcclusion > -1) {
        float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], tri.uv).r;
        color *= occlusion;
    }

    // Distance used by reflection
    rayPayload.distance = gl_RayTmaxEXT;

    // Use normal map texture if available
    if (geometryNode.textureIndexNormal > -1) {
        vec3 normalSample = texture(textures[nonuniformEXT(geometryNode.textureIndexNormal)], tri.uv).rgb;
        // Convert from [0, 1] to [-1, 1]
        rayPayload.normal = normalize(normalSample * 2.0f - 1.0f);
    } else {
        // Assign ray payload normal from tri
        rayPayload.normal = tri.normal.xyz;
    }

    // Assign ray payload color
    rayPayload.color = color.rgb;

    // Assign ray payload index - referred to in ray gen?
    rayPayload.index = float(gl_InstanceCustomIndexEXT);

    /* LIGHTING */
    // Sky color
    vec3 skyColor = texture(cubemapTexture, rayPayload.normal.xyz).rgb;
    vec3 tempSpecColor = mix(rayPayload.color.rgb, skyColor, 0.5f);

    vec3 lightingNormal = normalize(gl_WorldRayDirectionEXT);
    vec3 hitPoint = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;

    vec3 lightDir = normalize(ubo.lightPos.xyz - hitPoint);
    vec3 viewDir = normalize(-gl_WorldRayDirectionEXT);

    rayPayload.color = blinnPhong(lightingNormal, lightDir, viewDir, rayPayload.color.rgb, rayPayload.color.rgb, tempSpecColor);

    /* SHADOW CASTING */
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + rayPayload.normal.xyz * epsilon;
    shadowed = true;

    // Objects with full white vertex color are treated as reflectors
    rayPayload.reflector = ((color.r == 1.0f) && (color.g == 1.0f) && (color.b == 1.0f)) ? 1.0f : 0.0f;

    // Shadow trace rays light vector
    vec3 shadowLightVector = normalize(ubo.lightPos.xyz);

    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF, 0, 0, 1, origin, tmin, shadowLightVector, tmax, 2);

    // Blend shadow
    if (shadowed) {
        rayPayload.color *= 0.7;
    }
}
