#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

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

layout(binding = 2, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
	//mat4 testMat;
	uint frame;
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

#include "gltfAnim2_bufferreferences.glsl"
#include "gltfAnim2_geometrytypes.glsl"

void main()
{
	Triangle tri = unpackTriangle(gl_PrimitiveID, 112);
	rayPayload.color = tri.normal.xyz;

	GeometryNode geometryNode = geometryNodes.nodes[gl_GeometryIndexEXT];

	vec3 color = texture(textures[nonuniformEXT(geometryNode.textureIndexBaseColor)], tri.uv).rgb;

	if (geometryNode.textureIndexOcclusion > -1) {
		float occlusion = texture(textures[nonuniformEXT(geometryNode.textureIndexOcclusion)], tri.uv).r;
		color *= occlusion;
	}

	rayPayload.color = color;

	// Shadow casting
	float tmin = 0.001;
	float tmax = 10000.0;
	float epsilon = 0.001;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + tri.normal.xyz * epsilon;

	shadowed = true;  

	// Basic lighting
	vec3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, tri.normal.xyz), 0.2);
	
	rayPayload.color = color.rgb * dot_product;

	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT,
	0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
	
	if (shadowed) {
		rayPayload.color *= 0.2;
	}

	rayPayload.distance = gl_RayTmaxEXT;
	rayPayload.normal = normalize(tri.normal.xyz);

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = ((color.r == 1.0f) && (color.g == 1.0f) && (color.b == 1.0f)) ? 1.0f : 0.0f; 

	//	// Shadow casting
	//float tmin = 0.001;
	//float tmax = 10000.0;
	//float epsilon = 0.001;
	//vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT + tri.normal * epsilon;
	//shadowed = true;  
	//vec3 lightVector = vec3(-5.0, -2.5, -5.0);
	//	// Trace shadow ray and offset indices to match shadow hit/miss shader group indices
	//traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 0, 0, 1, origin, tmin, lightVector, tmax, 2);
	//if (shadowed) {
	//	hitValue *= 0.7;
	//}

}
