#version 450
//in from vertex shader
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 viewPos;


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

layout(set = 1, binding = 0) uniform sampler2D golbTex;

//fragment color
layout(location = 0) out vec4 outColor;

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex);
//vec2 ParallaxMapping(vec2 tex, vec3 viewDir);

void main() {

    vec3 viewDir = normalize(viewPos - fragPos).rgb;

    vec4 texColor = texture(golbTex, texCoords);

    //outColor = texColor;

 
    //outColor = vec4(1.0f);

    outColor = vec4(CalDirLight(normal, vec3(texColor), fragPos, viewDir, texCoords), 1.0f);

    //outColor = fragColor;

}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex){

    //light direction
    vec3 lightDir = normalize(ubo.lighting.lightPosition.xyz - fragPos);
    
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

    return ambient + diffuse + specular;
    //return (ambient + (1.0 + shadow) * (diffuse + specular));   
}