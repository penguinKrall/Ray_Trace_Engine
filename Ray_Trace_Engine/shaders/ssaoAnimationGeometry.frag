#version 450

layout(location = 0) in vec4 fragPos;
layout(location = 1) in vec4 fragColor;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec4 normal;
layout(location = 4) in vec4 viewPos;
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

//in from descriptor set in record commands
layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D specularTexture;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * 0.1f * 200.0f) / (0.1f + 200.0f - z * (0.1f - 200.0f));	
}

vec3 CalDirLight(vec4 norm, vec4 fragpos, vec3 diffuseCol, vec3 specularCol, vec3 viewDir);

void main() {

    vec3 diffCol = vec3(texture(diffuseTexture,texCoords));
    vec3 specCol = vec3(texture(specularTexture, texCoords));

    vec3 viewDir = normalize(viewPos - fragPos).xyz;

		//gPosition = vec4(fragPos.xy , linearDepth(gl_FragCoord.z), 1.0f);
    //gPosition = vec4(fragPos.xyz , linearDepth(gl_FragCoord.z));
    gPosition = normalize(vec4(fragPos.rgb, 1.0f));

		gNormal = vec4(normalize(normal.xyz * 0.5 + 0.5), 1.0f);

    gAlbedo = vec4(CalDirLight(normal, vec4(albedoFragPos, 1.0f), diffCol, specCol, viewDir), 1.0f);
    
    gBrightness = vec4(vec3(0.0f), 1.0f);

    //float strength = 1.0f;
    //float threshold = 0.1f;
    //
    //float brightness = dot(gAlbedo.rgb, vec3(0.2126, 0.7152, 0.0722));
    //
    //if (brightness > threshold) {
    //gBrightness = vec4(vec3(gAlbedo * strength), 1.0f);
    //} 
    //
    //else {
    //  gBrightness = vec4(0.0f);
    //}

}

vec3 CalDirLight(vec4 norm, vec4 fragpos, vec3 diffuseCol, vec3 specularCol, vec3 viewDir){

    //light direction
    //vec3 lightDir = vec3(normalize(fragPos - ubo.lighting.lightPosition));
    vec3 lightDir = normalize(ubo.lighting.lightPosition - fragpos).xyz;

    // - Diffuse Shading
    float diff = max(dot(vec3(lightDir), vec3(norm)),0.0f);
    
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
     float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x);

    // - Attenuation math
    float Distance = length(vec3(ubo.lighting.lightPosition) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance , 1));

    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * diffuseCol.rgb;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * diff * diffuseCol.rgb;
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * specularCol.rgb;
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;

}
