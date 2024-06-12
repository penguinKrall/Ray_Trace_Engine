#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

//layout(set = 1, binding = 4) uniform sampler2D ssaoBlurredDownsample;
layout(set = 0, binding = 9) uniform sampler2D ssaoBlurredDownsample;

void main(){

  float filterRadius = 0.005f;
	//float filterRadius = ubo.hdrBlur.w;

	// The filter kernel is applied with a radius, specified in texture
	// coordinates, so that the radius will vary across mip resolutions.
	float x = filterRadius;
	float y = filterRadius;
	
	// Take 9 samples around current texel:
	// a - b - c
	// d - e - f
	// g - h - i
	// === ('e' is the current texel) ===
	vec3 a = texture(ssaoBlurredDownsample, vec2(texCoords.x - x, texCoords.y + y)).rgb;
	vec3 b = texture(ssaoBlurredDownsample, vec2(texCoords.x,     texCoords.y + y)).rgb;
	vec3 c = texture(ssaoBlurredDownsample, vec2(texCoords.x + x, texCoords.y + y)).rgb;
	
	vec3 d = texture(ssaoBlurredDownsample, vec2(texCoords.x - x, texCoords.y)).rgb;
	vec3 e = texture(ssaoBlurredDownsample, vec2(texCoords.x,     texCoords.y)).rgb;
	vec3 f = texture(ssaoBlurredDownsample, vec2(texCoords.x + x, texCoords.y)).rgb;
	
	vec3 g = texture(ssaoBlurredDownsample, vec2(texCoords.x - x, texCoords.y - y)).rgb;
	vec3 h = texture(ssaoBlurredDownsample, vec2(texCoords.x,     texCoords.y - y)).rgb;
	vec3 i = texture(ssaoBlurredDownsample, vec2(texCoords.x + x, texCoords.y - y)).rgb;
	
	// Apply weighted distribution, by using a 3x3 tent filter:
	//  1   | 1 2 1 |
	// -- * | 2 4 2 |
	// 16   | 1 2 1 |
	vec3 upsample = vec3(0.0f);

	upsample = e*4.0;
	upsample += (b+d+f+h)*2.0;
	upsample += (a+c+g+i);
	upsample *= 1.0 / 16.0;

	outColor = vec4(upsample, 1.0f);
}