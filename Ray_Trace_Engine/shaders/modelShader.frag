#version 450
//in from vertex shader
layout(location = 0) in vec4 fragPos;	//vertex position (x, y, z)
layout(location = 1) in vec4 texCoords;	//tex u v
layout(location = 2) in vec4 normal;
layout(location = 3) in vec4 viewPos;

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
//layout(set = 0, binding = 8) uniform sampler2DMS shadowImage;
layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 2, binding = 0) uniform sampler2D specularTexture;


//fragment color
layout(location = 0) out vec4 outColor;

vec3 CalDirLight(vec4 norm, vec3 hdrColor, vec4 fragpos, vec3 viewDir, vec2 tex);
//vec2 ParallaxMapping(vec2 tex, vec3 viewDir);

void main() {

    //calculate view
    vec3 viewDir = normalize(viewPos - fragPos).rgb;

    //vec3 result = CalDirLight(normal, fragPos, viewDir, texCoords); 
    vec3 result = vec3(0.0f);

    vec4 fPos = normalize(fragPos);

    result = CalDirLight(normal, result.rgb, fPos, viewDir, texCoords.xy);

    outColor = vec4(result, 1.0f);
}

vec3 CalDirLight(vec4 norm, vec3 hdrColor, vec4 fragpos, vec3 viewDir, vec2 tex){

    //light direction
    vec3 lightDir = vec3(normalize(ubo.lightPos - fragPos));
    
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(norm), reflectDir), 0.0f), ubo.lighting.lightingValues.x);

    // - Attenuation math
    float Distance = length(vec3(ubo.lightPos) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance , 2));

    vec3 tempColor = texture(diffuseTexture, texCoords.xy).rgb;

    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * tempColor;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * diff * tempColor;
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * vec3(texture(specularTexture, texCoords.xy));
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;

}
