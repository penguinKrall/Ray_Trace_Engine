#version 450
//in from vertex shader
layout(location = 0) in vec4 fragPos;	//vertex position (x, y, z)
layout(location = 1) in vec4 fragColor;	//vertex color (r, g, b)
layout(location = 2) in vec2 texCoords;	//tex u v
layout(location = 3) in vec4 normal;
layout(location = 4) in vec4 viewPos;
layout(location = 5) in vec4 lightPos;
layout(location = 6) in vec4 inFragPosLightSpace;

	struct Lighting {
		vec4 ambientColor;
		vec4 diffuseColor;
		vec4 specularColor;
		vec4 lightPosition;
		vec4 lightingValues;
	};

layout(set = 0, binding = 4) uniform UBO {

		vec4 shadowFactorToneMap;
		vec4 lightPos;
		vec4 shadowValues;
		vec2 minMax;
		vec2 shadowsOnOff;
		vec4 visualDebugValues;
    vec4 parallaxMap;
    mat4 lightMVP;
    Lighting lighting;
} ubo;

//in from descriptor set in record commands -  this needs refactoring to have a single descriptor set
//and multiple bindings
layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 1) uniform sampler2D depthMap;

//fragment color
layout(location = 0) out vec4 outColor;

vec3 CalDirLight(vec4 norm, vec3 hdrColor, vec4 fragpos, vec3 viewDir, vec2 tex, float shadow);
//vec2 ParallaxMapping(vec2 tex, vec3 viewDir);

void main() {
    
    float shadow = 0.0f;

    //calculate
    vec3 viewDir = normalize(viewPos - fragPos).rgb;

    //vec3 result = CalDirLight(normal, fragPos, viewDir, texCoords); 
    vec4 result = texture(diffuseTexture, texCoords);

    if(ubo.shadowsOnOff.y == 1.0f) {

    float bias = 0.05f * ubo.shadowValues.x; 

    bias = clamp(bias, 0.0, 0.05);
    
    // Transform the fragment position to light space
    vec3 projCoords = normalize(inFragPosLightSpace.xyz / inFragPosLightSpace.w);
    projCoords = projCoords * 0.5 + 0.5; // Convert from [-1, 1] to [0, 1]
    
    // Fetch the depth from the shadow map for this fragment's position
    float shadowDepth = texture(depthMap, projCoords.xy).r * ubo.shadowValues.y;
    
    // Determine if this fragment is in shadow or not
    shadow = (projCoords.z - bias > shadowDepth) ? 0.0 : 1.0; // 0.0 is in shadow, 1.0 is lit
    
    if(projCoords.z > 1.0){
        shadow = 0.0;
    }

    }

    result = vec4(CalDirLight(normal, result.rgb, fragPos, viewDir, texCoords, shadow), 1.0f);

    ////tonemap
    vec3 hdrColor = result.rgb;
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    hdrColor = pow(mapped, vec3(1.0 / 2.2));
    
    outColor = vec4(hdrColor, 1.0f);

}

vec3 CalDirLight(vec4 norm, vec3 hdrColor, vec4 fragpos, vec3 viewDir, vec2 tex, float shadow){

    //light direction
    vec3 lightDir = vec3(normalize(lightPos - fragPos));
    
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x * 0.1f);

    // - Attenuation math
    float Distance = length(vec3(lightPos) - vec3(fragpos));
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
