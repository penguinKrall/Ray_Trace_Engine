#version 460

#extension GL_EXT_ray_tracing : enable

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

layout(binding = 6, set = 0) uniform sampler2D glassTexture;

layout(binding = 7, set = 0) uniform samplerCube cubemapTexture;

layout(binding = 8, set = 0) uniform sampler2D textures[];

void main() {

    //// View-independent background gradient to simulate a basic sky background
    //const vec3 gradientStart = vec3(0.4f, 0.2f, 0.6f);
    //const vec3 gradientEnd = vec3(1.0f);
    //vec3 unitDir = normalize(gl_WorldRayDirectionEXT);
    //float t = 0.5 * (unitDir.y + 1.0f);
    //vec3 backgroundColor = (1.0f - t) * gradientStart + t * gradientEnd;
    //
    //// Set the background color without alpha
    //rayPayload.color = backgroundColor;
    //rayPayload.bgTest = backgroundColor;
    ////rayPayload.accumulatedColor = vec4(backgroundColor, 1.0f);
    //rayPayload.distance = -1.0f;
    //rayPayload.normal = vec3(1.0f);
    //rayPayload.reflector = 1.0f;
    ////rayPayload.index = gl_InstanceCustomIndexEXT;

    // Normalize the incoming ray direction
    vec3 unitDir = normalize(gl_WorldRayDirectionEXT);

    // Sample the cubemap texture using the normalized ray direction
    vec3 cubemapColor = texture(cubemapTexture, unitDir).rgb;

    // Set the background color to the sampled cubemap color
    rayPayload.color = cubemapColor;
    rayPayload.bgTest = cubemapColor;
    rayPayload.distance = -100.0f;
    rayPayload.normal = vec3(1.0f);
    rayPayload.reflector = 1.0f;
    rayPayload.colorID = vec4(0.0f);
}

