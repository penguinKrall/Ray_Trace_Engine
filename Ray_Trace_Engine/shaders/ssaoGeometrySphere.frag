#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 viewPos;
layout(location = 5) in vec3 albedoFragPos;

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;
layout (location = 3) out vec4 gBrightness;

struct Lighting {
		vec4 ambientColor;
		vec4 diffuseColor;
		vec4 specularColor;
		vec4 lightPosition;
		vec4 lightingValues;
 };

layout(set = 0, binding = 1) uniform UBO {
    Lighting lighting;
} ubo;

layout(push_constant) uniform PushModel {
		mat4 model;
		vec4 renderOptions;
} pushModel;

layout(set = 1, binding = 0) uniform sampler2D golbTex;
layout(set = 1, binding = 1) uniform sampler2D golbTexSpecular;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * 0.1f * 200.0f) / (0.1f + 200.0f - z * (0.1f - 200.0f));	
}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex);

void main() {

    vec3 viewDir = normalize(fragPos - viewPos).rgb;

    //gPosition = normalize(vec4(fragPos, 1.0f));

		//gNormal = vec4(normalize(normal.xyz * 0.5 + 0.5), 1.0f);

    //gAlbedo = ubo.lighting.ambientColor + ubo.lighting.diffuseColor + ubo.lighting.specularColor * vec4(fragColor, 1.0f);

    gAlbedo = vec4(CalDirLight(normalize(normal), vec3(1.0f), fragPos, viewDir, texCoords.xy), 1.0f);

    gBrightness = gAlbedo;

}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex){
    
    //light direction
    vec3 lightDir = normalize(ubo.lighting.lightPosition.xyz - fragPos.xyz);
    
    // - Diffuse
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    
    //  - Specular
    vec3 reflectDir = reflect(vec3(lightDir), vec3(-norm));
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x * 5.0f);

    // - Attenuation
    float Distance = length(vec3(ubo.lighting.lightPosition) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance , 1));

    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * hdrColor;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * hdrColor;
    vec3 specular = vec3(ubo.lighting.specularColor) * hdrColor;
    
    // -  Attenuate results
   // ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;

}
