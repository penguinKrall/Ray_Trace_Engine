#version 450

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec2 texCoords;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 viewPos;
//layout(location = 5) in mat3 TBN;

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

layout(push_constant) uniform PushValues {
		mat4 model;
		float bias;
		float range;
		float isShadows;
} pushValues;

layout(set = 0, binding = 3) uniform sampler2D shadowDepthMap;
layout(set = 1, binding = 2) uniform sampler2D golbTex;
layout(set = 1, binding = 3) uniform sampler2D golbTexSpecular;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * 0.1f * 200.0f) / (0.1f + 200.0f - z * (0.1f - 200.0f));	
}

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex, float shadow);

void main() {

    float shadow = 0.0f;
    vec3 viewDir = normalize(viewPos - fragPos).rgb;
    vec4 texColor = texture(golbTex, texCoords);

   //vec4 fragLightPos = vec4(TBN * vec3(ubo.lighting.lightPosition), 1.0f);
   //vec4 fragViewPosition  = vec4(TBN * vec3(viewPos), 1.0f);
   //vec4 fragPosTBN  = vec4(TBN * vec3(fragPos), 1.0f);

    //get new tex coords
    //vec2 aTexCoords = ParallaxMapping(fragTexCoords,  viewDir);   
    //if(aTexCoords.x > 1.0 || aTexCoords.y > 1.0 || aTexCoords.x < 0.0 || aTexCoords.y < 0.0)
    //discard;

    //get new normals
    //aNormal = texture(normalTexture, aTexCoords);
    //aNormal = normalize(aNormal * 2.0 - 1.0);

    if(pushValues.isShadows == 1.0f){
        //get shadow per fragment
        // vec4 lightViewProj = lvpUBO.lightProjection * lvpUBO.lightView * fragPosTBN;
        vec4 lightViewProj = lvpUBO.lightProjection * lvpUBO.lightView * vec4(fragPos, 1.0f);
        float bias = pushValues.bias; 

        //bias = clamp(pushValues.bias, 0.0, 0.05);
        
        // Transform the fragment position to light space
        vec3 projCoords = lightViewProj.xyz / lightViewProj.w;
        projCoords = projCoords * 0.5 + 0.5; // Convert from [-1, 1] to [0, 1]
        
        // Fetch the depth from the shadow map for this fragment's position
        vec4 shadowMap = texture(shadowDepthMap, projCoords.xy) * 2.0f - 1.0f;
        float shadowDepth = shadowMap.r;
        
        // Determine if this fragment is in shadow or not
        shadow = (projCoords.z - bias > shadowDepth) ? 0.0 : 1.0; // 0.0 is in shadow, 1.0 is lit
        
        if(projCoords.z > pushValues.range){
            shadow = 0.0;
        }
    }

    //fill gbuffer
		gPosition = vec4(fragPos , linearDepth(gl_FragCoord.z));

		gNormal = vec4(normalize(normal) * 0.5 + 0.5, 1.0f);

    gAlbedo = vec4(CalDirLight(normal, vec3(texColor), fragPos, viewDir, texCoords, shadow), 1.0f);

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

vec3 CalDirLight(vec3 norm, vec3 hdrColor, vec3 fragpos, vec3 viewDir, vec2 tex, float shadow){
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
