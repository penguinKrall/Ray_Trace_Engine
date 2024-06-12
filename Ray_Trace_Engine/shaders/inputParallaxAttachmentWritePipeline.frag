#version 450

// Inputs from vertex shader
layout(location = 0) in vec4 fragPos;	
layout(location = 1) in vec4 fragColor;	
layout(location = 2) in vec2 fragTexCoords;	
layout(location = 3) in vec4 fragNormal;
layout(location = 4) in vec4 fragViewPosition;
layout(location = 5) in vec4 fragLightPos;
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

layout(set = 1, binding = 0) uniform sampler2D diffuseTexture;
layout(set = 1, binding = 1) uniform sampler2D specularTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D heightTexture;
layout(set = 0, binding = 8) uniform sampler2DMS shadowImage;
layout(set = 2, binding = 1) uniform sampler2D depthMap;


layout(location = 0) out vec4 outColor;

vec3 CalDirLight(vec4 norm, vec3 hdrColor, vec4 fragpos, vec3 viewDir, vec2 tex, float shadow);
vec2 ParallaxMapping(vec2 tex, vec3 viewDir);


void main() {
    float shadow = 0.0f;
    //vec4 v_FragPosLightSpace = ubo.lightMVP * fragPos;

    vec3 result;

    vec4 aNormal = fragNormal;
    
    //calculate view
    vec3 viewDir = normalize(fragViewPosition - fragPos).rgb;

    //get new tex coords
    vec2 aTexCoords = ParallaxMapping(fragTexCoords,  viewDir);   
    if(aTexCoords.x > 1.0 || aTexCoords.y > 1.0 || aTexCoords.x < 0.0 || aTexCoords.y < 0.0)
    discard;

    //get new normals
    aNormal = texture(normalTexture, aTexCoords);
    aNormal = normalize(aNormal * 2.0 - 1.0);

    ////tonemap
    vec3 hdrColor = texture(diffuseTexture, aTexCoords).rgb;

    //shadow map
    if(ubo.shadowsOnOff.y == 1.0f) {

       float bias = 0.05f; 

    bias = clamp(bias, 0.0, 0.05);
    
    // Transform the fragment position to light space
    vec3 projCoords = inFragPosLightSpace.xyz / inFragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Convert from [-1, 1] to [0, 1]
    
    // Fetch the depth from the shadow map for this fragment's position
    float shadowDepth = texture(depthMap, projCoords.xy).r;
    
    // Determine if this fragment is in shadow or not
    shadow = (projCoords.z - bias > shadowDepth) ? 0.0 : 1.0; // 0.0 is in shadow, 1.0 is lit
    
    if(projCoords.z > 1.0){
        shadow = 0.0;
    }

    }

    //apply lighting
    result = CalDirLight(aNormal, hdrColor, fragPos, viewDir, aTexCoords, shadow); 

    outColor = vec4(result, 1.0f);

}


vec3 CalDirLight(vec4 norm, vec3 hdrColor, vec4 fragpos, vec3 viewDir, vec2 tex, float shadow){

    //light direction
    vec3 lightDir = vec3(normalize(fragLightPos - fragPos));
    
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(lightDir)),0.0f);
    
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x * 0.1f);

    // - Attenuation math
    float Distance = length(vec3(fragLightPos) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance ,2));

    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * hdrColor;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * hdrColor;
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * vec3(texture(specularTexture, tex));
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    //return ambient + diffuse + specular;
    return (ambient + (1.0 + shadow) * (diffuse + specular));   
}

vec2 ParallaxMapping(vec2 tex, vec3 viewDir) {  

    // number of depth layers
    const float minLayers = ubo.parallaxMap.y;
    const float maxLayers = ubo.parallaxMap.z;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  

    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;

    // depth of current layer
    float currentLayerDepth = 0.0;

    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * ubo.parallaxMap.x; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = tex;
    float currentDepthMapValue = texture(heightTexture, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue) {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;

        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(heightTexture, currentTexCoords).r;  

        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    
    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;

    float beforeDepth = texture(heightTexture, prevTexCoords).r - currentLayerDepth + layerDepth;
     
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);

    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords; 

}