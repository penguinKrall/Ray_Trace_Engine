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

    //use normal map texture if available
    if (geometryNode.textureIndexNormal > -1) {
        vec3 normalSample = texture(textures[nonuniformEXT(geometryNode.textureIndexNormal)], tri.uv).rgb;
        // Convert from [0, 1] to [-1, 1]
        rayPayload.normal = normalize(normalSample * 2.0 - 1.0);
    }

    //assign ray payload normal from tri
    else{
    rayPayload.normal = normalize(tri.normal.xyz);
    }
    //assign ray payload color
    rayPayload.color = color.rgb;

    //assign ray payload index - referred to in ray gen?
    rayPayload.index = float(gl_InstanceCustomIndexEXT);

    /* LIGHTING */
    //sky color
    vec3 skyColor = texture(cubemapTexture, rayPayload.normal.xyz).rgb;

    //diffuse light vector
    vec3 diffuseLightVector = normalize(ubo.lightPos.xyz - tri.position.xyz);

    //view direction
    vec3 viewDir = normalize(tri.position.xyz - ubo.viewPos.xyz).xyz;

    //ambient lighting
    float dot_product = max(dot(rayPayload.normal.xyz, diffuseLightVector), 0.95);
    rayPayload.color *= dot_product;
   
    //specular lighting
    vec3 reflectDir = reflect(vec3(-diffuseLightVector), rayPayload.normal.xyz);
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), 0.05f);

    rayPayload.color  = rayPayload.color * spec;

    //rayPayload.color = mix(rayPayload.color, skyColor, 0.1);

    /* SHADOW CASTING */
    float tmin = 0.001;
    float tmax = 10000.0;
    float epsilon = 0.001;
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + rayPayload.normal.xyz * epsilon;
    shadowed = true;

    // Objects with full white vertex color are treated as reflectors
    rayPayload.reflector = ((color.r == 1.0f) && (color.g == 1.0f) && (color.b == 1.0f)) ? 1.0f : 0.0f;

    //shadow trace rays light vector
    vec3 shadowLightVector = normalize(ubo.lightPos.xyz);

    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
        0xFF, 0, 0, 1, origin, tmin, shadowLightVector, tmax, 2);
    
    //blend shadow
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
//hitAttributeEXT vec2 attribs;
//
//struct RayPayload {
//    vec3 color;
//    float distance;
//    vec3 normal;
//    float reflector;
//    vec4 accumulatedColor;
//    float accumulatedAlpha;
//    float index;
//    vec3 bgTest;
//    int semiTransparentFlag;
//    vec4 colorID;
//};
//
//layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
//layout(location = 2) rayPayloadEXT bool shadowed;
//
//layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
//
//layout(binding = 3, set = 0) uniform UBO {
//    mat4 viewInverse;
//    mat4 projInverse;
//    vec4 lightPos;
//    vec4 viewPos;
//} ubo;
//
//struct GeometryNode {
//    uint64_t vertexBufferDeviceAddress;
//    uint64_t indexBufferDeviceAddress;
//    int textureIndexBaseColor;
//    int textureIndexOcclusion;
//    int textureIndexMetallicRoughness;
//    int textureIndexNormal;
//    int textureIndexEmissive;
//    int semiTransparentFlag;
//    float objectColorID;
//};
//
//struct GeometryIndex {
//    int offset;
//};
//
//layout(binding = 4, set = 0) buffer G_Nodes_Buffer { GeometryNode nodes[]; } g_nodes_buffer;
//layout(binding = 5, set = 0) buffer G_Nodes_Index { GeometryIndex indices[]; } g_nodes_indices;
//layout(binding = 6, set = 0) uniform sampler2D glassTexture;
//layout(binding = 7, set = 0) uniform samplerCube cubemapTexture;
//layout(binding = 8, set = 0) uniform sampler2D textures[];
//
//#include "main_renderer_bufferreferences.glsl"
//#include "main_renderer_geometrytypes.glsl"
//
//void main() {
//
//    // Get geometry node index
//    uint instanceCustomIndex = gl_InstanceCustomIndexEXT;
//
//    // Unpack triangles
//    Triangle tri = unpackTriangle2(gl_PrimitiveID, 112);
//
//    // Get geometry node data from buffer
//    GeometryNode geometryNode = g_nodes_buffer.nodes[gl_GeometryIndexEXT + g_nodes_indices.indices[instanceCustomIndex].offset];
//
//    // Set ray payload semi-transparent flag
//    rayPayload.semiTransparentFlag = geometryNode.semiTransparentFlag;
//
//    // Set ray payload color ID data
//    rayPayload.colorID = vec4(vec3(geometryNode.objectColorID), 1.0f);
//
//    // Default color
//    vec4 color = vec4(1.0f);
//
//    // Assign texture color if geometry node has texture
//    if (geometryNode.textureIndexBaseColor > -1) {
//        color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], tri.uv);
//
//        // Set semi-transparency flag according to base color alpha
//        if (color.a < 1.0f) {
//            rayPayload.semiTransparentFlag = 1;
//        }
//    } else {
//        // Assign vertex color to color if no texture
//        color = vec4(tri.color.rgb, 1.0f);
//    }
//
//    // Assign occlusion color map if model has one -- currently unused?
//    if (geometryNode.textureIndexOcclusion > -1) {
//        float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], tri.uv).r;
//        color *= occlusion;
//    }
//
//    // Distance used by reflection
//    rayPayload.distance = gl_RayTmaxEXT;
//
//    // Use normal map texture if available
//    if (geometryNode.textureIndexNormal > -1) {
//        vec3 normalSample = texture(textures[nonuniformEXT(geometryNode.textureIndexNormal)], tri.uv).rgb;
//        // Convert from [0, 1] to [-1, 1]
//        rayPayload.normal = normalize(normalSample * 2.0 - 1.0);
//    } else {
//        // Assign ray payload normal from tri
//        rayPayload.normal = normalize(tri.normal.xyz);
//    }
//
//    // Assign ray payload color
//    rayPayload.color = color.rgb;
//
//    // Assign ray payload index
//    rayPayload.index = float(gl_InstanceCustomIndexEXT);
//
//    // Ray coords
//    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
//    const vec2 inUV = pixelCenter / vec2(gl_LaunchSizeEXT.xy);
//    vec2 d = inUV;
//
//    // Calculate world position
//    vec4 origin = ubo.viewInverse * vec4(0, 0, 0, 1);
//    vec4 target = ubo.projInverse * vec4(d.x, d.y, 1, 1);
//    vec4 direction = ubo.viewInverse * vec4(normalize(target.xyz), 0);
//    vec3 worldPos = origin.xyz + direction.xyz * gl_HitTEXT;
//
//    // Static shininess value
//    const float shininess = 32.0;
//
//    // Add sky color
//    vec3 skyColor = texture(cubemapTexture, rayPayload.normal).rgb;
//
//    // Basic lighting
//    vec3 lightVector = normalize(ubo.lightPos.xyz - target.xyz); // Light direction
//
//    // Diffuse lighting
//    float diffuseFactor = max(dot(rayPayload.normal, lightVector), 0.0); // Lambertian reflectance
//
//    // Specular lighting
//    vec3 viewVector = normalize(ubo.viewPos.xyz - worldPos); // View direction
//    vec3 reflectVector = reflect(-lightVector, rayPayload.normal); // Reflection vector
//    float specularFactor = pow(max(dot(viewVector, reflectVector), 0.0), shininess); // Blinn-Phong or Phong reflection model
//
//    // Light intensity and color
//    vec3 lightColor = skyColor.rgb;
//    vec3 diffuseColor = diffuseFactor * rayPayload.color * lightColor; // Diffuse component
//    vec3 specularColor = specularFactor * lightColor; // Specular component
//
//    // Combine both diffuse and specular lighting
//    rayPayload.color = diffuseColor + specularColor;
//
//    //rayPayload.color = mix(rayPayload.color, skyColor, 0.1);
//    
//    // Shadow casting
//    float tmin = 0.001;
//    float tmax = 10000.0;
//    float epsilon = 0.001;
//    vec3 shadowOrigin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + rayPayload.normal * epsilon;
//    shadowed = true;
//
//    // Objects with full white vertex color are treated as reflectors
//    rayPayload.reflector = ((color.r == 1.0f) && (color.g == 1.0f) && (color.b == 1.0f)) ? 1.0f : 0.0f;
//
//    // Trace shadow ray and offset indices to match shadow hit/miss shader group indices
//    traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
//        0xFF, 0, 0, 1, shadowOrigin, tmin, lightVector, tmax, 2);
//
//    // Blend shadow
//    if (shadowed) {
//        rayPayload.color *= 0.7;
//    }
//}