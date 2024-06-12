#version 450

layout(location = 0) in vec2 texCoords;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 viewPosition;
layout(location = 4) in vec3 reflectNormals;
layout(location = 5) in vec3 inPos;

layout (location = 0) out vec4 outColor;


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
		float cubemap;
} pushModel;

layout(set = 1, binding = 0) uniform sampler2D golbTex;
layout(set = 2, binding = 0) uniform sampler2D golbTexSpecular;
layout(set = 3, binding = 0) uniform samplerCube cubemapTexture;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * 0.1f * 200.0f) / (0.1f + 200.0f - z * (0.1f - 200.0f));	
}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 specularTex, vec3 fragpos, vec3 viewDir, vec2 tex);

void main() {

    vec3 viewDir = normalize(fragPos - viewPosition).rgb;
    vec4 texColor = texture(golbTex, texCoords);

    vec3 I = normalize((mat3(pushModel.model) * inPos) - viewPosition);
    vec3 R = reflect(I, normalize(reflectNormals));
    vec4 reflectColor = vec4(texture(cubemapTexture, R).rgb, 1.0);

    if(pushModel.cubemap == 1.0f){
        vec4 cubemapTex = texture(cubemapTexture, inPos);
      outColor = cubemapTex;
    }

    else{
        outColor = vec4(CalDirLight(normal.rgb,
        texColor.rgb, reflectColor.rgb, fragPos.xyz, viewDir, texCoords.xy), 1.0f);
        
        //outColor = vec4(texColor.rgb, 1.0f);
    }

}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 specularTex, vec3 fragpos, vec3 viewDir, vec2 tex){

    //light direction
    vec3 lightDir = normalize(ubo.lighting.lightPosition.xyz - fragpos.xyz);
    
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
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * specularTex;
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
    //return (ambient + (1.0 + shadow) * (diffuse + specular));   
}
