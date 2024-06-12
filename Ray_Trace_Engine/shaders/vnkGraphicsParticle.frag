#version 450

layout(location = 0) in vec3 fragColor;
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gBrightness;

// Tonemapping function (you can use different tonemapping operators)
//vec3 tonemap(vec3 hdrColor, float exposure) {
//    return vec3(1.0) - exp(-hdrColor * exposure);
//}

void main() {
    

    // Tonemapping parameters
    const float exposure = 1.0;
    const float gamma = 2.2;

    vec2 coord = gl_PointCoord - vec2(0.5);
    gPosition = vec4(fragColor, 1.0f);
    gNormal = normalize(gPosition);
    gAlbedo = gPosition;
    gBrightness =vec4(fragColor * vec3(100.0f), 1.0f);

}