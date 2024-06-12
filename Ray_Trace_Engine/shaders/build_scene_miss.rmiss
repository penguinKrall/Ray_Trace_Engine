#version 460

#extension GL_EXT_ray_tracing : enable

struct RayPayload {
    vec3 color;
    float distance;
    vec3 normal;
    float reflector;
    //vec4 rgbaColorData;
    vec4 accumulatedColor;
    float accumulatedAlpha;
    float index;
    vec3 bgTest;
};

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {
    // View-independent background gradient to simulate a basic sky background
    const vec3 gradientStart = vec3(0.2f, 0.2f, 0.2f);
    const vec3 gradientEnd = vec3(1.0f);
    vec3 unitDir = normalize(gl_WorldRayDirectionEXT);
    float t = 0.5 * (unitDir.y + 1.0f);
    vec3 backgroundColor = (1.0f - t) * gradientStart + t * gradientEnd;

    // Set the background color without alpha
    rayPayload.color = backgroundColor;
    rayPayload.bgTest = backgroundColor;
    //rayPayload.accumulatedColor = vec4(backgroundColor, 1.0f);
    rayPayload.distance = -1.0f;
    rayPayload.normal = vec3(1.0f);
    rayPayload.reflector = 0.0f;
    //rayPayload.index = gl_InstanceCustomIndexEXT;
}

