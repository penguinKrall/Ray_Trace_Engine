#version 450 core

layout(location = 0) in vec2 texCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D shadowDepthMap;
//layout(set = 0, binding = 1) uniform sampler2D sceneColor;
layout(set = 1, binding = 0) uniform sampler2D gbufferPosition;
layout(set = 1, binding = 1) uniform sampler2D gbufferNormal;
layout(set = 1, binding = 2) uniform sampler2D gbufferAlbedo;
//layout(set = 1, binding = 3) uniform sampler2D gbufferDepth;
layout(set = 1, binding = 4) uniform sampler2D ssaoImage;
layout(set = 1, binding = 5) uniform sampler2D downscaleImage;
layout(set = 1, binding = 6) uniform sampler2D upscaleImage;
layout(set = 1, binding = 7) uniform sampler2D compilationImage;
//layout(set = 1, binding = 9) uniform sampler2D lightingColor;

layout(set = 2, binding = 3) uniform sampler2D bloomMip4;
layout(set = 2, binding = 5) uniform sampler2D bloomUpscale4;

// Reinhard tone mapping operator
vec3 reinhard(vec3 color, float exposure) {
    return color / (color + vec3(1.0)) * exposure;
}

layout(push_constant) uniform PushConstantValues {
				float mixStrength;
				float tonemapStrength;
				float gamma;
} pushValues;

void main(){
		//vec4 sceneColor = texture(sceneColor, texCoords);
	  //vec4 shadowColor = texture(shadowDepthMap, texCoords);

		//vec4 gbufferPositionColor = texture(gbufferPosition, texCoords);
		//vec4 gbufferNormalColor = texture(gbufferNormal, texCoords);
		//vec4 gbufferAlbedoColor = texture(gbufferAlbedo, texCoords);
		//vec4 gbufferDepthColor = texture(gbufferDepth, texCoords);
		//vec4 ssaoImageColor = texture(ssaoImage, texCoords);
		//vec4 downscaleColor = texture(downscaleImage, texCoords);
		//vec4 upscaleColor = texture(upscaleImage, texCoords);
		//vec4 bloomMip4Color = texture(bloomMip4, texCoords);
		vec4 bloomUpscale4Color = texture(bloomUpscale4, texCoords);
		vec4 compilationColor = texture(compilationImage, texCoords);

		//outColor = shadowColor;
		//outColor = gbufferPositionColor;
		//outColor = gbufferNormalColor;
		//outColor = gbufferAlbedoColor;
		//outColor = gbufferDepthColor;
		//outColor = vec4(ssaoImageColor.rgb, 1.0f);
		//outColor = downscaleColor;
		//outColor = upscaleColor;
		//outColor = bloomMip4Color;
		//outColor = bloomUpscale4Color;
		//vec4 tempColor = compilationColor + bloomUpscale4Color;
		vec4 result;
		vec4 preSult;

		result = mix(compilationColor, bloomUpscale4Color, pushValues.mixStrength);
		result += bloomUpscale4Color;

		//preSult = mix(result, lightingCol, 0.33);
		// tone mapping
    // result = vec4(1.0) - exp(-preSult * pushValues.tonemapStrength);
	  result = vec4(1.0) - exp(-result * pushValues.tonemapStrength);

    // also gamma correct while we're at it
    const float gamma = 2.2;
    result = pow(result, vec4(vec3(1.0 / pushValues.gamma), 1.0f));
    outColor = vec4(result.rgb, 1.0f);
				
		//outColor = vec4(vec3(shadowColor.r), 1.0f);
}