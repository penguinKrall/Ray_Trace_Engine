#version 450
//in from vertex shader
layout(location = 0) in vec4 fragPos;	//vertex position (x, y, z)
layout(location = 1) in vec4 fragColor;	//vertex color (r, g, b)
layout(location = 2) in vec2 texCoords;	//tex u v
layout(location = 3) in vec4 normal;
layout(location = 4) in vec4 viewPos;
layout(location = 5) in vec4 lightPos;

//in from descriptor set in record commands
layout(set = 2, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 3, binding = 0) uniform sampler2D specularTexture;

	struct Lighting {
		vec4 ambientColor;
		vec4 diffuseColor;
		vec4 specularColor;
		vec4 lightPosition;
		vec4 lightingValues;
	};

layout(set = 1, binding = 4) uniform UBO {

		vec4 shadowFactorToneMap;
		vec4 lightPosition;
		vec4 shadowValues;
		vec2 minMax;
		vec2 shadowsOnOff;
		vec4 visualDebugValues;
    vec4 parallaxMap;
    mat4 lightMVP;

    Lighting lighting;

} ubo;

//fragment out color for render pass? rasterizer? figure this out so you know for sure.
layout(location = 0) out vec4 outColor;	// Interpolated color from vertex (location must match)

//the first of more functions to come. think this is written more like a point light. 
vec3 CalDirLight(vec4 norm, vec4 fragpos, vec3 viewDir);

void main() {

    //calculate view direction as per blinn - phong light tutorial on learnopengl.com
    vec3 viewDir = vec3(normalize(viewPos - fragPos));

    //result. "+=" future ambient + diffuse + specular functions for the other lights. (spotlight, actual directional light, etc.)
    vec3 result = CalDirLight(normal, fragPos, viewDir); 
    
    //fragment out color
    outColor = vec4(result, 1.0f);

}

vec3 CalDirLight(vec4 norm, vec4 fragpos, vec3 viewDir){

    //light direction
    vec3 lightDir = vec3(normalize(ubo.lightPosition - fragPos));
     //vec3 lightDir = vec3(normalize(fragPos - ubo.lightPosition));
    
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x * 0.1f);

    // - Attenuation math
    float Distance = length(vec3(ubo.lightPosition) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance ,2));

    vec3 color = vec3(1.0f, 1.0f, 1.0f);

    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * vec3(texture(diffuseTexture,texCoords));
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * diff * vec3(texture(diffuseTexture,texCoords));
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * vec3(texture(specularTexture, texCoords));

    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;

}