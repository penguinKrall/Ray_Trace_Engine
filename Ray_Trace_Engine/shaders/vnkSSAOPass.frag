#version 450

//#define KERNEL_SIZE 4

layout(location = 0) in vec2 texCoords;

layout(set = 0, binding = 0) uniform SceneUBO {
	mat4 projection;
	mat4 view;
	vec4 viewPosition;
} ubo;

layout(set = 1, binding = 5) uniform KernelUBO {
	vec4 samples[64];
} kernelUBO;

layout(set = 1, binding = 0) uniform sampler2D positionImage;
layout(set = 1, binding = 1) uniform sampler2D normalImage;
layout(set = 1, binding = 2) uniform sampler2D albedoImage;
//layout(set = 1, binding = 3) uniform sampler2DMS depthImage;
layout(set = 1, binding = 4) uniform sampler2D noiseTexture;

layout(location = 0) out vec4 outColor;

float linearDepth(float depth) {

	float z = depth * 2.0f - 1.0f; 

	return (2.0f * 0.1f * 200.0f) /
  (200.0f + 0.1f - z *
  (200.0f - 0.1f));	

}

layout(push_constant) uniform PushValues {
		float bias;
		float radius;
		float kernelSize;
		float range;
} pushValues;

void main(){
 
  //vec2 noiseScale = vec2(256.0 / 4.0, 256.0 /4.0); 
  //float bias = 0.025f;
  //float radius = 0.3f;
  //float kernelSize = 64.0f;

  float bias = pushValues.bias;
  float radius = pushValues.radius;
  float kernelSize  = pushValues.kernelSize;
  float range  = pushValues.range;

 // Get G-Buffer values
	vec3 fragPos = texture(positionImage, texCoords).rgb * 2.0 - 1.0;
	vec3 normal = normalize(texture(normalImage, texCoords).rgb * 2.0 - 1.0);
	
	// Get a random vector using a noise lookup
	ivec2 texDim = textureSize(positionImage, 0); 
	ivec2 noiseDim = textureSize(noiseTexture, 0);
	const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * texCoords;  
	vec3 randomVec = texture(noiseTexture, noiseUV).xyz * 2.0 - 1.0;
	
	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);
	
	// Calculate occlusion value
	float occlusion = 0.0f;

	for(int i = 0; i < kernelSize; i++){		
		vec3 samplePos = TBN * kernelUBO.samples[i].xyz; 
		samplePos = fragPos + samplePos * radius; 
		
		// project
		vec4 offset = vec4(samplePos, 1.0f);
		//offset = vec4(transpose(inverse(mat3(ubo.projection))) * offset.xyz, 1.0f); 
    offset = ubo.view * offset; 
		offset.xyz /= offset.z; 
		offset.xyz = offset.xyz * 0.05f + 0.5f; 
		offset.y = 1.0f - offset.y;

		//float sampleDepth = texture(positionImage, texCoords).z * 2.0 - 1.0; 
	  float sampleDepth = texture(positionImage, texCoords).z * 2.0 - 1.0; 
		float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth)) * range;

		occlusion += (sampleDepth <= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck; 
    
	}

	occlusion = 1.0 - (occlusion / float(64));
	
	outColor = vec4(occlusion);

}








 	//
  //// get input for SSAO algorithm
  //vec3 fragPos = texture(positionImage, texCoords).xyz;
  //vec3 normal = normalize(texture(normalImage, texCoords).rgb);
  //vec3 randomVec = normalize(texture(noiseTexture, texCoords * noiseScale).xyz);
 	//
  //// create TBN change-of-basis matrix: from tangent-space to view-space
  //vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
  //vec3 bitangent = cross(normal, tangent);
  //mat3 TBN = mat3(tangent, bitangent, normal);
 	//
  // //  // iterate over the sample kernel and calculate occlusion factor
  //  float occlusion = 0.0;
  // for(int i = 0; i < kernelSize; ++i) {
  //   // get sample position
  //   vec3 samplePos = TBN * kernelUBO.samples[i].xyz; // from tangent to view-space
  //   samplePos = fragPos + samplePos * radius; 
  //   
  //   // project sample position (to sample texture) (to get position on screen/texture)
  //   vec4 offset = vec4(samplePos, 1.0f);
  //   offset = ubo.projection * offset; // from view to clip-space
  //   offset.xy /= offset.w; // perspective divide
  //   offset.xy = offset.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0
  //   offset.y = 1.0 - offset.y;
 	//
  //   // get sample depth
  //   float sampleDepth = texture(positionImage, offset.xy).z; // get depth value of kernel sample
  //   
  //   // range check & accumulate
  //   float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth - bias));
  //   occlusion += (sampleDepth >= samplePos.z - bias ? 1.0 : 0.0) * rangeCheck; 
  //   
  //  }
 	//
  // occlusion = 1.0 - (occlusion / kernelSize);
  //
  // //outColor = vec4(vec3(occlusion), 1.0f);
  //  outColor = vec4(occlusion);
