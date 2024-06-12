#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable

struct RayPayload {
	vec3 color;
	float distance;
	vec3 normal;
	float reflector;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

hitAttributeEXT vec2 attribs;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
	int vertexSize;
} ubo;
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;

struct Vertex
{
  vec4 pos;
  vec4 normal;
  vec2 uv;
  vec4 _pad0; 
  vec4 _pad1;
  vec4 color;
	vec4 joint0;
	vec4 weight0;
	vec4 tangent;
};

Vertex unpack(uint index)
{
    // Calculate the multiplier based on the size of the vertex structure in terms of vec4
    const int m = ubo.vertexSize / 16;

    // Load the vec4 components from the SSBO
    vec4 d0 = vertices.v[m * index + 0];
    vec4 d1 = vertices.v[m * index + 1];
    vec4 d2 = vertices.v[m * index + 2];
    vec4 d3 = vertices.v[m * index + 3];
    vec4 d4 = vertices.v[m * index + 4];
    vec4 d5 = vertices.v[m * index + 5];
    vec4 d6 = vertices.v[m * index + 6];

    // Create a Vertex instance and assign the values
    Vertex v;
    v.pos = d0;
    v.normal = d1;
    v.uv = vec2(d2.x, d2.y);
    v._pad0 = d3;
    v._pad1 = d4;
    v.color = d3;
    v.joint0 = d4;
    v.weight0 = d5;
    v.tangent = d6;

    return v;
}

void main()
{
	ivec3 index = ivec3(indices.i[3 * gl_PrimitiveID], indices.i[3 * gl_PrimitiveID + 1], indices.i[3 * gl_PrimitiveID + 2]);

	Vertex v0 = unpack(index.x);
	Vertex v1 = unpack(index.y);
	Vertex v2 = unpack(index.z);

	// Interpolate normal
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 normal = normalize(v0.normal.xyz * barycentricCoords.x + v1.normal.xyz * barycentricCoords.y + v2.normal.xyz * barycentricCoords.z);

	// Basic lighting
	vec3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(lightVector, normal), 0.6);
	rayPayload.color = v0.color.rgb * vec3(dot_product);
	rayPayload.distance = gl_RayTmaxEXT;
	rayPayload.normal = normal;

	// Objects with full white vertex color are treated as reflectors
	rayPayload.reflector = ((v0.color.r == 1.0f) && (v0.color.g == 1.0f) && (v0.color.b == 1.0f)) ? 1.0f : 0.0f; 
}
