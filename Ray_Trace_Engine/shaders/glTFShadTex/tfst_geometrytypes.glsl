/* Copyright (c) 2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
	vec3 inColor;
  vec4 inJointIndices;
	vec4 inJointWeights;
};

struct Triangle {
	Vertex vertices[3];
	vec3 normal;
	vec2 uv;
};

// This function will unpack our vertex buffer data into a single triangle and calculates uv coordinates
Triangle unpackTriangle(uint index, int vertexSize) {
	Triangle tri;
	const uint triIndex = index * 3;

	GeometryNode geometryNode = geometryNodes.nodes[gl_GeometryIndexEXT];

	Indices indices   = Indices(geometryNode.indexBufferDeviceAddress);
	Vertices vertices = Vertices(geometryNode.vertexBufferDeviceAddress);

	// Unpack vertices
	// Data is packed as vec4 so we can map to the glTF vertex structure from the host side
	// We match vkglTF::Vertex: pos.xyz+normal.x, normalyz+uv.xy
	// glm::vec3 pos;
	// glm::vec3 normal;
	// glm::vec2 uv;
	// ...
	for (uint i = 0; i < 3; i++) {
		const uint offset = indices.i[triIndex + i] * 6;
		vec4 d0 = vertices.v[offset + 0]; // pos.xyz, n.x
		vec4 d1 = vertices.v[offset + 1]; // n.yz, uv.xy
		tri.vertices[i].pos = d0.xyz + vec3(2.0f);
		tri.vertices[i].normal = vec3(d0.w, d1.xy);
		tri.vertices[i].uv = d1.zw;

	}

	// Calculate values at barycentric coordinates
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
	tri.normal = tri.vertices[0].normal * barycentricCoords.x + tri.vertices[1].normal * barycentricCoords.y + tri.vertices[2].normal * barycentricCoords.z;
	return tri;
}

//Triangle unpackTriangle2(uint index, int vertexSize) {
//    Triangle tri;
//    const uint triIndex = index * 3;
//
//    GeometryNode geometryNode = geometryNodes.nodes[gl_GeometryIndexEXT];
//    TransformMatrix transformationMatrix = transformMatrices.transforms[gl_GeometryIndexEXT]; // Updated to use gl_GeometryIndexEXT
//    Indices indices   = Indices(geometryNode.indexBufferDeviceAddress);
//    Vertices vertices = Vertices(geometryNode.vertexBufferDeviceAddress);
//
//    // Unpack vertices
//    for (uint i = 0; i < 3; i++) {
//        const uint offset = indices.i[triIndex + i] * vertexSize;
//        tri.vertices[i].pos = vertices.v[offset].xyz;
//        tri.vertices[i].normal = vertices.v[offset + 1].xyz;
//        tri.vertices[i].uv = vertices.v[offset + 2].xy;
//        tri.vertices[i].inColor = vertices.v[offset + 3].xyz;
//        tri.vertices[i].inJointIndices = vertices.v[offset + 4];
//        tri.vertices[i].inJointWeights = vertices.v[offset + 5];
//        
//        // Calculate skinned matrix
//        mat4 skinMat = 
//            tri.vertices[i].inJointWeights.x * transformationMatrix.matrix +
//            tri.vertices[i].inJointWeights.y * transformationMatrix.matrix +
//            tri.vertices[i].inJointWeights.z * transformationMatrix.matrix +
//            tri.vertices[i].inJointWeights.w * transformationMatrix.matrix;
//
//        // Apply skinMat transformation
//        tri.vertices[i].pos = (skinMat * vec4(tri.vertices[i].pos, 1.0)).xyz;
//    }
//
//    // Calculate values at barycentric coordinates
//    vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
//    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
//    tri.normal = tri.vertices[0].normal * barycentricCoords.x + tri.vertices[1].normal * barycentricCoords.y + tri.vertices[2].normal * barycentricCoords.z;
//
//    return tri;
//}



///* Copyright (c) 2023, Sascha Willems
// *
// * SPDX-License-Identifier: MIT
// *
// */
//
//struct Vertex
//{
//  vec3 pos;
//  vec3 normal;
//  vec2 uv;
//	vec3 inColor;
//  vec4 inJointIndices;
//	vec4 inJointWeights;
//};
//
//struct Triangle {
//	Vertex vertices[3];
//	vec3 normal;
//	vec2 uv;
//};
//
//// This function will unpack our vertex buffer data into a single triangle and calculates uv coordinates
//// Modify the unpackTriangle function to include transformation using SSBO
//Triangle unpackTriangle(uint index, int vertexSize, mat4[] transformationMatrices) {
//    Triangle tri;
//    const uint triIndex = index * 3;
//
//    GeometryNode geometryNode = geometryNodes.nodes[gl_GeometryIndexEXT];
//
//    Indices indices   = Indices(geometryNode.indexBufferDeviceAddress);
//    Vertices vertices = Vertices(geometryNode.vertexBufferDeviceAddress);
//
//    // Unpack vertices
//    for (uint i = 0; i < 3; i++) {
//        const uint offset = indices.i[triIndex + i] * vertexSize;
//        tri.vertices[i].pos = vertices.v[offset].xyz;
//        tri.vertices[i].normal = vertices.v[offset + 1].xyz;
//        tri.vertices[i].uv = vertices.v[offset + 2].xy;
//        tri.vertices[i].inColor = vertices.v[offset + 3].xyz;
//        tri.vertices[i].inJointIndices = vertices.v[offset + 4];
//        tri.vertices[i].inJointWeights = vertices.v[offset + 5];
//        
//        // Apply transformation using SSBO
//        vec4 transformedPosition = vec4(tri.vertices[i].pos, 1.0) * transformationMatrices[i]; // Assuming each vertex has its own transformation matrix
//        tri.vertices[i].pos = transformedPosition.xyz;
//    }
//
//    // Calculate values at barycentric coordinates
//    vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
//    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
//    tri.normal = tri.vertices[0].normal * barycentricCoords.x + tri.vertices[1].normal * barycentricCoords.y + tri.vertices[2].normal * barycentricCoords.z;
//    return tri;
//}
