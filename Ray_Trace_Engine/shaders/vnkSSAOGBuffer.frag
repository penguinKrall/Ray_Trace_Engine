#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 viewPos;
layout(location = 5) in vec3 albedoFragPos;
layout(location = 6) in vec3 inPos;
layout(location = 7) in vec3 reflectNormals;
layout(location = 8) in mat3 TBN;

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
		float cubePass;
		float spherePass;
		float quadPass;
		float animationPass;
		float cubemapPass;
    float bias;
		float range;
		float isShadows;
} pushModel;

layout(set = 0, binding = 3) uniform sampler2D shadowDepthMap;
layout(set = 1, binding = 0) uniform sampler2D golbTex;
layout(set = 1, binding = 1) uniform sampler2D golbTexSpecular;
layout(set = 1, binding = 2) uniform sampler2D floorColor;
layout(set = 1, binding = 3) uniform sampler2D floorSpecular;
layout(set = 1, binding = 4) uniform sampler2D floorHeight;
layout(set = 1, binding = 5) uniform sampler2D floorNormal;
layout(set = 1, binding = 6) uniform sampler2D golbColor;
layout(set = 1, binding = 7) uniform sampler2D golbSpecular;
layout(set = 1, binding = 8) uniform sampler2D golbHeight;
layout(set = 1, binding = 9) uniform sampler2D golbNormal;
layout(set = 2, binding = 0) uniform samplerCube cubemapTexture;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * 0.1f * 200.0f) / (0.1f + 200.0f - z * (0.1f - 200.0f));	
}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 specularTex, vec3 fragpos, vec3 viewDir, vec2 tex);
vec3 CalDirLightShadow(vec3 norm, vec3 hdrColor, vec3 specularColor, vec3 fragpos, vec3 viewDir, vec2 tex, float shadow);
vec2 ParallaxMapping(vec2 tex, vec3 viewDir);
vec3 ParallaxMapLighting();

void main() {

    float shadow = 0.0f;
    vec3 viewDir = normalize(fragPos - viewPos).xyz;
    //vec3 viewDir = normalize(viewPos - fragPos).xyz;
    vec4 texColor = texture(golbTex, texCoords);
    
    if(pushModel.quadPass == 1.0f){

        if(pushModel.isShadows == 1.0f){
            //get shadow per fragment
            // vec4 lightViewProj = lvpUBO.lightProjection * lvpUBO.lightView * fragPosTBN;
            //vec4 lightViewProj = lvpUBO.lightProjection * lvpUBO.lightView * vec4(fragPos, 1.0f);
            float bias = pushModel.bias; 

            vec4 lightViewProj = lvpUBO.lightProjection * lvpUBO.lightView * vec4(fragPos, 1.0f);
            vec3 projCoords = lightViewProj.xyz / lightViewProj.w;
            projCoords = projCoords * 0.5 + 0.5; // Convert from [-1, 1] to [0, 1]
            //vec4 shadowMap = texture(shadowDepthMap, projCoords.xy) * 2.0f - 1.0f;
            vec4 shadowMap = texture(shadowDepthMap, projCoords.xy);
            float shadowDepth = shadowMap.r;
            
            // Determine if this fragment is in shadow or not
            shadow = (projCoords.z - bias > shadowDepth) ? 0.0 : 1.0; // 0.0 is in shadow, 1.0 is lit
           
           if(projCoords.z > pushModel.range){
                shadow = 0.0;
            }
            
        }
               //normal mapping
               vec4 aNormal = texture(floorNormal, texCoords);
               vec4 tempNormal = vec4(normalize(vec3(aNormal.xyz * 0.5 + 0.5) * normal), 1.0f);
               aNormal = tempNormal;


               //parallax mapping
               //get new texcoords
               //vec2 aTexCoords = ParallaxMapping(texCoords, normalize(viewPos - fragPos));
               //get new normals
               //vec4 aNormal = normalize(texture(floorNormal, aTexCoords));

                //fill gbuffer
		            gPosition = normalize(vec4(fragPos , 1.0f));


                //gNormal = vec4(normalize(normal), 1.0f); //this is how it works w/o normal mapp etc
                gNormal = aNormal;


                texColor = texture(floorColor, texCoords);
                gAlbedo = vec4(CalDirLightShadow(gNormal.rgb, vec3(texColor), vec3(texture(floorSpecular, texCoords)),
                fragPos, viewDir, texCoords, shadow), 1.0f);
                //gAlbedo = vec4(ParallaxMapLighting(),1.0f);
                gBrightness = vec4(vec3(0.0f), 1.0f);

                return;
    }

    //cubemap
    if(pushModel.cubemapPass == 1.0f){

        gPosition = normalize(vec4(fragPos, 1.0f));
		    gNormal = vec4(normalize(normal.xyz * 0.5 + 0.5), 1.0f);

        vec4 cubemapTex = texture(cubemapTexture, inPos);
        vec4 tempColor = vec4(1.0) - exp(-cubemapTex * 0.75f);

        const float gamma = 0.75;
        
        vec4 finalTempColor = pow(tempColor, vec4(vec3(1.0 / gamma), 1.0f));

        gAlbedo = finalTempColor;
        gBrightness = vec4(0.0f);

        return;
    
    }

    //cube
    if(pushModel.spherePass != 1.0f){

        //normal mapping
        vec4 aNormal = texture(golbNormal, texCoords);
        vec4 tempNormal = vec4(normalize(vec3(aNormal.xyz * 0.5 + 0.5) * normal), 1.0f);
        aNormal = tempNormal;

        vec3 I = normalize((mat3(pushModel.model) * inPos) - viewPos);
        vec3 R = reflect(I, normalize(reflectNormals * aNormal.rgb));
        vec4 reflectColor = vec4(texture(cubemapTexture, R).rgb, 1.0) * vec4(texture(golbSpecular, texCoords).rgb,1.0f);


        texColor = texture(golbColor, texCoords);

        gPosition = normalize(vec4(fragPos, 1.0f));
		    gNormal = vec4(normalize(normal.xyz * 0.5 + 0.5), 1.0f);
        gAlbedo = vec4(CalDirLight(normalize(aNormal.rgb), texColor.rgb, reflectColor.rgb, albedoFragPos.xyz,
        normalize(albedoFragPos - viewPos), texCoords.xy), 1.0f);

        //bloom geometry
        gBrightness = vec4(vec3(0.0f), 1.0f);

    }

    //sphere
    if(pushModel.spherePass == 1.0f){
        gPosition = normalize(vec4(vec3(0.0f), 1.0f));
		    gNormal = vec4(0.0f);
        gAlbedo = vec4(vec3((texColor * (ubo.lighting.ambientColor +
        ubo.lighting.diffuseColor + ubo.lighting.specularColor)) / 10), 1.0f);
        gBrightness = gAlbedo * 50;
    }

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
    * Distance + ubo.lighting.lightingValues.w * pow(Distance , 1));

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

vec3 CalDirLightShadow(vec3 norm, vec3 hdrColor, vec3 specularColor, vec3 fragpos, vec3 viewDir, vec2 tex, float shadow){
    //light direction
    vec3 lightDir = normalize( fragpos - ubo.lighting.lightPosition.xyz);
    // - Diffuse Shading
    float diff = max(dot(vec3(norm), vec3(-lightDir)),0.0f);
    //  - Specular Shading
    vec3 reflectDir = reflect(vec3(-lightDir), vec3(norm));
    float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x);
    // - Attenuation math
    float Distance = length(vec3(ubo.lighting.lightPosition) - vec3(fragpos));
    float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    * Distance + ubo.lighting.lightingValues.w * pow(Distance, 1));
    //color
    vec3 ambient = vec3(ubo.lighting.ambientColor) * hdrColor;
    vec3 diffuse = vec3(ubo.lighting.diffuseColor) * diff * hdrColor;
    vec3 specular = vec3(ubo.lighting.specularColor) * spec * specularColor;
    
    // -  Attenuate results
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    //return ambient + diffuse + specular;
    return (ambient + (1.0 + shadow) * (diffuse + specular));   
}


vec3 ParallaxMapLighting(){
    //vec3 viewDir = normalize(vec3(fragPos) - (TBN * viewPos));
    //vec3 viewDir = normalize((TBN * fragPos) - (TBN * viewPos));
    //vec2 pTexCoords = ParallaxMapping(texCoords, viewDir);       

    //if(pTexCoords.x > 1.0 || pTexCoords.y > 1.0 || pTexCoords.x < 0.0 || pTexCoords.y < 0.0)
    //    discard;

    // obtain normal from normal map
    // vec4 aNormal = texture(floorNormal, texCoords);
    // vec4 tempNormal = vec4(normalize(vec3(aNormal.xyz * 0.5 + 0.5) * normal), 1.0f);
    // aNormal = tempNormal;  
    //
    //// get diffuse color
    //vec3 color = texture(floorColor, texCoords).rgb;
    //
    //// ambient
    //vec3 ambient = vec3(ubo.lighting.ambientColor) * color;
    //
    //// diffuse
    //vec3 diffViewDir = normalize(TBN * fragPos - viewPos).xyz;
    //vec3 diffLightDir = vec3(normalize(fragPos - ubo.lighting.lightPosition.xyz));
    //
    //vec3 lightDir = normalize(lvpUBO.lightPosition.xyz - fragPos);
    //float diff = max(dot(vec3(aNormal), vec3(diffLightDir)),0.0f);
    //vec3 diffuse = diff * color;
    //
    ////  - Specular Shading
    //vec3 reflectDir = reflect(vec3(-lightDir), vec3(aNormal));
    //float spec = pow(max(dot(vec3(diffViewDir), reflectDir), 0.0f), ubo.lighting.lightingValues.x);
    ////float spec = pow(max(dot(vec3(normalize(fragPos - viewPos).xyz), reflectDir), 0.0f), ubo.lighting.lightingValues.x);
    //
    //vec3 specular = vec3(ubo.lighting.specularColor) * spec * vec3(texture(floorSpecular, texCoords));
    ////vec3 specular = vec3(0.2) * spec;
    //
    ////attenuation
    //float Distance = length(vec3(ubo.lighting.lightPosition) - vec3(fragPos));
    //float attenuation = 1.0f / (ubo.lighting.lightingValues.z + ubo.lighting.lightingValues.y 
    //* Distance + ubo.lighting.lightingValues.w * pow(Distance, 1));
    //
    //ambient *= attenuation;
    //diffuse *= attenuation;
    //specular *= attenuation;
    //
    //vec3 returnColor = vec3(ambient + diffuse + specular);

    //vec3 color = texture(floorColor, pTexCoords).rgb;
    return vec3(0.0f);

}

//vec2 ParallaxMapping(vec2 tex, vec3 viewDir) {
//    const float minLayers = 8.0;
//    const float maxLayers = 32.0;
//    
//    float layerDepth = 1.0 / maxLayers;
//    float currentDepth = texture(floorHeight, tex).r;
//
//    // Initial parallax mapping offset
//    vec2 texOffset = viewDir.xy * layerDepth / viewDir.z;
//    
//    // Initial texture coordinates
//    vec2 currentTexCoords = tex;
//    
//    for (float i = 0.0; i < maxLayers; ++i) {
//        currentTexCoords -= texOffset;
//        
//        float depth = texture(floorHeight, currentTexCoords).r;
//
//        if (depth <= currentDepth) {
//            // Linearly interpolate between the current and previous layer
//            float weight = (currentDepth - depth) / layerDepth;
//            return mix(currentTexCoords + texOffset, currentTexCoords, weight);
//        }
//        
//        currentDepth += layerDepth;
//
//        // Adjust texOffset based on the view direction
//        texOffset = normalize(viewDir.xy) * layerDepth / viewDir.z;
//        // texOffset = normalize(viewDir.xy) * layerDepth;
//    }
//
//    return tex;
//}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8.0f;
    const float maxLayers = 32.0f;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  

    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;

    // depth of current layer
    float currentLayerDepth = 0.0;

    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * 1.0f; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(floorHeight, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue) {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;

        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(floorHeight, currentTexCoords).r;  

        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    
    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;

    float beforeDepth = texture(floorHeight, prevTexCoords).r - currentLayerDepth + layerDepth;
     
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);

    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    
    return finalTexCoords; 
}
