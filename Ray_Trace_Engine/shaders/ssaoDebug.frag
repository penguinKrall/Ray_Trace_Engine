#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 1) in vec4 viewPos;
layout(location = 2) in vec4 lightPos;
layout(location = 3) in mat4 view;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 view;
	vec4 viewPosition;
	vec4 lightPosition;
	float nearPlane;
	float farPlane;
	float pad1;
	float pad2;
	vec4 ssaoDebug;
	vec4 ssaoDebug2;
} ubo;

struct Lighting {
		vec4 ambientColor;
		vec4 diffuseColor;
		vec4 specularColor;
		vec4 lightPosition;
		vec4 lightingValues;
	};

layout(set = 1, binding = 4) uniform RendererUBO {

		vec4 shadowFactorToneMap;
		vec4 lightPos;
		vec4 shadowValues;
		vec2 minMax;
		vec2 shadowsOnOff;
		vec4 visualDebugValues;
    vec4 parallaxMap;
    mat4 lightMVP;
    Lighting lighting;
} rendererUBO;

layout(set = 0, binding = 4) uniform sampler2D positionImage;
layout(set = 0, binding = 5) uniform sampler2D normalImage;
layout(set = 0, binding = 6) uniform sampler2D albedoImage;
layout(set = 0, binding = 7) uniform sampler2D depthImage;
layout(set = 0, binding = 8) uniform sampler2D ssaoColorImage;
layout(set = 0, binding = 9) uniform sampler2D ssaoDownscaleBlurImage;
layout(set = 0, binding = 9) uniform sampler2D ssaoUpscaleBlurImage;
layout(set = 0, binding = 12) uniform sampler2D sceneColorImage;

float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - z * (ubo.farPlane - ubo.nearPlane));	
}

void main(){
    
		vec4 posColor = vec4(0.0f);
		vec4 normColor = vec4(0.0f);
		vec4 albedoColor = vec4(0.0f);
		vec4 depthColor = vec4(0.0f);
		vec4 ssaoColor = vec4(0.0f);
		vec4 blurColor = vec4(0.0f);
		vec4 sceneColor = vec4(0.0f);

		if(ubo.ssaoDebug.x == 1.0f){
		posColor = texture(positionImage, texCoords);
		outColor = posColor;
		}

		if(ubo.ssaoDebug.y == 1.0f){
		normColor = texture(normalImage, texCoords);
		outColor = normColor;
		}

		if(ubo.ssaoDebug.z == 1.0f){
		albedoColor = texture(albedoImage, texCoords);
		outColor = albedoColor;
		}

		if(ubo.ssaoDebug.w == 1.0f){
		depthColor = texture(depthImage, texCoords);
		outColor = vec4(depthColor.r);
		}

		if(ubo.ssaoDebug2.x == 1.0f){
		ssaoColor = texture(ssaoColorImage, texCoords);
		outColor = ssaoColor;
		}

		if(ubo.ssaoDebug2.y == 1.0f){
		blurColor = texture(ssaoDownscaleBlurImage, texCoords);
		outColor = blurColor;
		}

		if(ubo.ssaoDebug2.z == 1.0f){
		sceneColor = texture(ssaoUpscaleBlurImage, texCoords);
		outColor = sceneColor;
		}
		//if(ubo.ssaoDebug2.w == 1.0f){
		//    // retrieve data from gbuffer
    //vec3 FragPos = texture(positionImage, texCoords).xyz;
    ////vec3 Normal = normalize(mat3(view) * vec3(texture(normalImage, texCoords)));
		//vec3 Normal = vec3(texture(normalImage, texCoords));
		//
		////vec3 LIGHTPOSITION = vec3(view * lightPos);
		////vec3 LIGHTPOSITION = vec3(mat3(view) * vec3(lightPos));
		//
    ////vec3 Diffuse = texture(albedoImage, texCoords).rgb;
		//vec3 Diffuse = texture(sceneColorImage, texCoords).rgb;
    //float AmbientOcclusion = texture(ssaoBlurImage, texCoords).r;
    //vec3 viewDir  = normalize(-FragPos); // viewpos is (0.0.0)
    ////vec3 viewDir = normalize(viewPos.xyz - FragPos).rgb;
    //// then calculate lighting as usual
		//vec3 ambient = vec3(0.3 * Diffuse * AmbientOcclusion);
		//
    //vec3 lighting  = ambient; 
		//
    //// diffuse
    //vec3 lightDir = normalize(vec3(lightPos) - FragPos);
    //vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * rendererUBO.lighting.ambientColor.xyz;
		//
    //// specular
		//// vec3 reflectDir = reflect(vec3(-lightDir), vec3(Normal));
    ////float spec = pow(max(dot(vec3(viewDir), reflectDir), 0.0f), rendererUBO.lighting.lightingValues.x * 0.1f);
		//
    //vec3 halfwayDir = normalize(lightDir + viewDir);  
    //float spec = pow(max(dot(Normal, halfwayDir), 0.0), rendererUBO.lighting.lightingValues.x);
    //vec3 specular = vec3(rendererUBO.lighting.specularColor) * spec;
		//
    //// attenuation
    //float Distance = length(vec3(lightPos) - FragPos);
   ////float attenuation = 1.0f / (rendererUBO.lighting.lightingValues.z + rendererUBO.lighting.lightingValues.y 
   //// * Distance + rendererUBO.lighting.lightingValues.w * pow(Distance ,2));
		//    float attenuation = 1.0 / (1.0 + rendererUBO.lighting.lightingValues.y
		//		* Distance + rendererUBO.lighting.lightingValues.z * Distance * Distance);
		//
    //diffuse *= attenuation;
    //specular *= attenuation;
    //lighting += diffuse + specular;
		//vec4 mixedColor = vec4(Diffuse * lighting.r, 1.0f);
	  //outColor = mix(mixedColor, vec4(Diffuse, 1.0f), 0.95);
    ////outColor = vec4(lighting, 1.0);
		//}
		//experiment

		if(ubo.ssaoDebug2.w == 1.0f){
	
	outColor = vec4(1.0f);
	
	vec3 fragPos = texture(positionImage, texCoords).rgb;
	vec3 normal = texture(normalImage, texCoords).rgb;
	vec4 albedo = texture(albedoImage, texCoords);
	 
	//float ssao =  texture(ssaoDownscaleBlurImage, texCoords).r;
		float ssao =  texture(ssaoUpscaleBlurImage, texCoords).r;

	//vec3 lightPos = vec3(0.0);
	vec3 L = normalize(lightPos.xyz - fragPos);
	//vec3 L = normalize(ubo.lightPosition.xyz - fragPos);
	float NdotL = max(0.5, dot(normal, L));
	
		outColor.rgb = ssao.rrr;
	
		vec3 baseColor = albedo.rgb * NdotL;
	
				outColor.rgb *= baseColor;
				
				vec4 finalColor = outColor;

				if(outColor.r > 0.4){
				vec4 mixedColor = texture(sceneColorImage, texCoords) * vec4(outColor.rrr, 1.0f);
				outColor = mixedColor;
				}

				else {
				outColor = texture(sceneColorImage, texCoords);
				}

				}

}