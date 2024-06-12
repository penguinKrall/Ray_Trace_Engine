#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 texCoords;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 viewPos;

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

layout(set = 0, binding = 2) uniform LVPUBO {
		mat4 lightProjection;
		mat4 lightView;
		vec4 lightPosition;
} lvpUBO;

layout(push_constant) uniform PushModel {
		mat4 model;
		float depth;
} pushModel;

layout(set = 1, binding = 0) uniform sampler3D noiseMap;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * 0.1f * 200.0f) / (0.1f + 200.0f - z * (0.1f - 200.0f));	
}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 specularTex, vec3 fragpos, vec3 viewDir, vec2 tex);
vec3 CalDirLightShadow(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex, float shadow);

void main() {

    float shadow = 0.0f;
    vec3 viewDir = normalize(fragPos - viewPos).xyz;
    vec4 texColor = vec4(vec3(texture(noiseMap, texCoords).r), 1.0f);

    //fill gbuffer
		gPosition = vec4(fragPos , 1.0f);

		gNormal = vec4(normalize(normal) * 0.5 + 0.5, 1.0f);

    gAlbedo = texColor;

    gBrightness = vec4(vec3(0.0f), 1.0f);

}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 specularTex, vec3 fragpos, vec3 viewDir, vec2 tex){

    //light direction
    vec3 lightDir = normalize(ubo.lighting.lightPosition.xyz - fragpos.xyz);
    
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(-viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x);

    // - Attenuation math
    float Distance = length(vec3(ubo.lighting.lightPosition) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance ,2));

    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * hdrColor;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * diff * hdrColor;
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * specularTex;
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
    //return (ambient + (1.0 + shadow) * (diffuse + specular));   
}

vec3 CalDirLightShadow(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex, float shadow){
    //light direction
    vec3 lightDir = normalize(ubo.lighting.lightPosition.xyz - fragpos);
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x * 0.1f);
    // - Attenuation math
    float Distance = length(vec3(ubo.lighting.lightPosition) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance ,2));
    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * hdrColor;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * hdrColor;
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * hdrColor;
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    //return ambient + diffuse + specular;
    return (ambient + (1.0 + shadow) * (diffuse + specular));   
}
