#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 3) uniform sampler2D mip3;

//layout(set = 2, binding = 0) uniform UBO {
//    vec4 hdrBlur;
//    vec4 hdrTonemap;
//    vec4 horizontalBlur;
//    vec4 verticalBlur;
//    vec4 bloomMix;
//    vec4 bloomTonemap;
//} ubo;

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
	vec3 a = texture(mip3, vec2(texCoords.x - x, texCoords.y + y)).rgb;
	vec3 b = texture(mip3, vec2(texCoords.x,     texCoords.y + y)).rgb;
	vec3 c = texture(mip3, vec2(texCoords.x + x, texCoords.y + y)).rgb;
	
	vec3 d = texture(mip3, vec2(texCoords.x - x, texCoords.y)).rgb;
	vec3 e = texture(mip3, vec2(texCoords.x,     texCoords.y)).rgb;
	vec3 f = texture(mip3, vec2(texCoords.x + x, texCoords.y)).rgb;
	
	vec3 g = texture(mip3, vec2(texCoords.x - x, texCoords.y - y)).rgb;
	vec3 h = texture(mip3, vec2(texCoords.x,     texCoords.y - y)).rgb;
	vec3 i = texture(mip3, vec2(texCoords.x + x, texCoords.y - y)).rgb;
	
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