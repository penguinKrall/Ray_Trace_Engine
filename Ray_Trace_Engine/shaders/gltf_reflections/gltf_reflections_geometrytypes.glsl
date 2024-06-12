struct Vertex
{
    vec4 pos;
    vec4 normal; // Changing vec3 to vec4
    vec2 uv;
    float paddin1;
    float paddin2;
    vec4 color;
    vec4 joints;
    vec4 weights;
    vec4 tangent;
};

struct Triangle {
    Vertex vertices[3];
    vec4 position;
    vec4 normal;
    vec2 uv;
    vec4 color;
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
    const uint offset = indices.i[triIndex + i] * 7; // Each vertex takes 7 vec4s
    vec4 d0 = vertices.v[offset]; // First vec4 (pos.xyz)
    vec4 d1 = vertices.v[offset + 1]; // Second vec4 (normal.xyz)
    vec4 d2 = vertices.v[offset + 2]; // Second vec4 (uv.xyz)
    vec4 d3 = vertices.v[offset + 3]; // Third vec4 (color)
    vec4 d4 = vertices.v[offset + 4]; // Fourth vec4 (joints)
    vec4 d5 = vertices.v[offset + 5]; // Fifth vec4 (weights)
    vec4 d6 = vertices.v[offset + 6]; // Sixth vec4 (tangent)
    
    // Assign pos from d0 directly
    tri.vertices[i].pos = d0;
    
    // Assign normal from d1 directly
    tri.vertices[i].normal = d1;
    
    // Assign uv from d2 directly
    tri.vertices[i].uv = d2.xy;
    
    // Assign color from d2 directly
    tri.vertices[i].color = d3;
    
    // Assign joints from d3 directly
    tri.vertices[i].joints = d4;
    
    // Assign weights from d4 directly
    tri.vertices[i].weights = d5;
    
    // Assign tangent from d5 directly
    tri.vertices[i].tangent = d6;
}

    // Calculate values at barycentric coordinates
    vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    tri.uv = tri.vertices[0].uv * barycentricCoords.x + tri.vertices[1].uv * barycentricCoords.y + tri.vertices[2].uv * barycentricCoords.z;
    // Calculate the centroid position of the triangle
    tri.position = (tri.vertices[0].pos + tri.vertices[1].pos + tri.vertices[2].pos) / 3.0;
    tri.normal = vec4(tri.vertices[0].normal.xyz * barycentricCoords.x + tri.vertices[1].normal.xyz *
    barycentricCoords.y + tri.vertices[2].normal.xyz * barycentricCoords.z, 1.0f);
     // Interpolate the color
    tri.color = tri.vertices[0].color * barycentricCoords.x + tri.vertices[1].color * barycentricCoords.y + tri.vertices[2].color * barycentricCoords.z;
    return tri;
}
