#version 450 core

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D mip0;

layout(set = 2, binding = 0) uniform UBO {
    vec4 hdrBlur;
    vec4 hdrTonemap;
    vec4 horizontalBlur;
    vec4 verticalBlur;
    vec4 bloomMix;
    vec4 bloomTonemap;
} ubo;

vec3 PowVec3(vec3 v, float p) {
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float invGamma = 1.0 / 2.2;
vec3 ToSRGB(vec3 v)   { return PowVec3(v, invGamma); }

float sRGBToLuma(vec3 col) {
    return dot(col, vec3(0.299f, 0.587f, 0.114f));
}

float KarisAverage(vec3 col) {
    float luma = sRGBToLuma(ToSRGB(col)) * 0.25f;
    return 1.0f / (1.0f + luma);
}

void main() {
	vec2 srcTexelSize = 1.0 / textureSize(mip0, 0);
	float x = srcTexelSize.x;
	float y = srcTexelSize.y;

	//ivec2 baseCoord = ivec2(texCoords * textureSize(mip0));  

	// Fetch the surrounding samples
	vec3 a = texture(mip0, texCoords + vec2(-2*x,  2*y)).rgb;
	vec3 b = texture(mip0, texCoords + vec2( 0,    2*y)).rgb;
	vec3 c = texture(mip0, texCoords + vec2( 2*x,  2*y)).rgb;
	
	vec3 d = texture(mip0, texCoords + vec2(-2*x,  0)).rgb;
	vec3 e = texture(mip0, texCoords).rgb;
	vec3 f = texture(mip0, texCoords + vec2( 2*x,  0)).rgb;
	
	vec3 g = texture(mip0, texCoords + vec2(-2*x, -2*y)).rgb;
	vec3 h = texture(mip0, texCoords + vec2( 0,   -2*y)).rgb;
	vec3 i = texture(mip0, texCoords + vec2( 2*x, -2*y)).rgb;
	
	vec3 j = texture(mip0, texCoords + vec2(-x,    y)).rgb;
	vec3 k = texture(mip0, texCoords + vec2( x,    y)).rgb;
	vec3 l = texture(mip0, texCoords + vec2(-x,   -y)).rgb;
	vec3 m = texture(mip0, texCoords + vec2( x,   -y)).rgb;

	// Apply weighted distribution:
	// 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
	// a,b,d,e * 0.125
	// b,c,e,f * 0.125
	// d,e,g,h * 0.125
	// e,f,h,i * 0.125
	// j,k,l,m * 0.5
	// This shows 5 square areas that are being sampled. But some of them overlap,
	// so to have an energy preserving downsample we need to make some adjustments.
	// The weights are the distributed, so that the sum of j,k,l,m (e.g.)
	// contribute 0.5 to the final color output. The code below is written
	// to effectively yield this sum. We get:
	// 0.125*5 + 0.03125*4 + 0.0625*4 = 1
	
	// Check if we need to perform Karis average on each block of 4 samples
	int mipLevel = 0;

	vec3 groups[5];
	vec3 downsample = vec3(0.0f);

	switch (mipLevel) {
	case 0:
	  // We are writing to mip 0, so we need to apply Karis average to each block
	  // of 4 samples to prevent fireflies (very bright subpixels, leads to pulsating
	  // artifacts).
	  groups[0] = (a+b+d+e) * (0.125f/4.0f);
	  groups[1] = (b+c+e+f) * (0.125f/4.0f);
	  groups[2] = (d+e+g+h) * (0.125f/4.0f);
	  groups[3] = (e+f+h+i) * (0.125f/4.0f);
	  groups[4] = (j+k+l+m) * (0.5f/4.0f);
	  groups[0] *= KarisAverage(groups[0]);
	  groups[1] *= KarisAverage(groups[1]);
	  groups[2] *= KarisAverage(groups[2]);
	  groups[3] *= KarisAverage(groups[3]);
	  groups[4] *= KarisAverage(groups[4]);
	  downsample = groups[0]+groups[1]+groups[2]+groups[3]+groups[4];
	  downsample = max(downsample, 0.0001f);
		outColor = vec4(downsample, 1.0f);
	  break;
	default:
	  downsample = e*0.125;                // ok
	  downsample += (a+c+g+i)*0.03125;     // ok
	  downsample += (b+d+f+h)*0.0625;      // ok
	  downsample += (j+k+l+m)*0.125;       // ok
		outColor = vec4(downsample, 1.0f);
	  break;
	}
}