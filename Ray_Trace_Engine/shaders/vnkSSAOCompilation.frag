#version 450 core

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

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

layout(push_constant) uniform PushValues {
		float bias;
		float mixStr;
		float clampStr;
} pushValues;

//layout(set = 0, binding = 0) uniform sampler2D shadowDepthMap;
//layout(set = 0, binding = 1) uniform sampler2D sceneColor;
layout(set = 1, binding = 0) uniform sampler2D gbufferPosition;
layout(set = 1, binding = 1) uniform sampler2D gbufferNormal;
layout(set = 1, binding = 2) uniform sampler2D gbufferAlbedo;
//layout(set = 1, binding = 3) uniform sampler2D gbufferDepth;
layout(set = 1, binding = 4) uniform sampler2D ssaoImage;
layout(set = 1, binding = 5) uniform sampler2D downscaleImage;
layout(set = 1, binding = 6) uniform sampler2D upscaleImage;

void main(){

	outColor = vec4(1.0f);
	
	vec3 fragPos = normalize(texture(gbufferPosition, texCoords).rgb * 2.0 - 1.0);
	vec3 normal = normalize(texture(gbufferNormal, texCoords).rgb * 2.0 - 1.0);
	vec4 albedo = texture(gbufferAlbedo, texCoords);
	//vec4 lightingCol = texture(lightingColor, texCoords);

	float ssao =  texture(upscaleImage, texCoords).z * 2.0 - 1.0;

	vec3 L = normalize(ubo.lighting.lightPosition.xyz - fragPos);
	float NdotL = max(pushValues.clampStr, dot(normal, L));
	
		outColor.rgb = ssao.rrr;
	
		vec3 baseColor = albedo.rgb * NdotL;
	
				outColor.rgb *= baseColor;
				
				vec4 finalColor = outColor;

				if(finalColor.r < (pushValues.bias)){
						vec4 mixedColor = mix(vec4(albedo.rgb, 1.0f), outColor, pushValues.mixStr);
						outColor = mixedColor;
				}

				else {
						outColor = vec4(albedo.rgb, 1.0f);
				}

}