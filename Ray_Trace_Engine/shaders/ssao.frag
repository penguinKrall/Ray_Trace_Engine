#version 450

//#define KERNEL_SIZE 4

layout(location = 0) in vec2 texCoords;

layout(set = 0, binding = 0) uniform SceneUBO {
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
	vec4 ssaoDebug3;
} sceneUBO;

layout(set = 0, binding = 1) uniform SSAOUBO {
	mat4 projection;
	bool ssao;
	bool ssaoOnly;
	bool ssaoBlur;
} ssaoUBO;

layout(set = 0, binding = 2) uniform KernelUBO {

	vec4 samples[64];

} kernelUBO;

layout(set = 0, binding = 4) uniform sampler2D positionImage;
layout(set = 0, binding = 5) uniform sampler2D normalImage;
layout(set = 0, binding = 6) uniform sampler2D albedoImage;
layout(set = 0, binding = 7) uniform sampler2D depthImage;
layout(set = 0, binding = 11) uniform sampler2D noiseTexture;

layout(location = 0) out vec4 outColor;

float linearDepth(float depth) {

	float z = depth * 2.0f - 1.0f; 

	return (2.0f * sceneUBO.nearPlane * sceneUBO.farPlane) /
  (sceneUBO.farPlane + sceneUBO.nearPlane - z *
  (sceneUBO.farPlane - sceneUBO.nearPlane));	

}

void main(){

   vec2 noiseScale = vec2(512.0 / 4.0, 512.0 /4.0); 
   float radius = sceneUBO.ssaoDebug3.x;
   float bias = sceneUBO.ssaoDebug3.y;
   float kernelSize = sceneUBO.ssaoDebug3.z;
 
   // get input for SSAO algorithm
   vec3 fragPos = texture(positionImage, texCoords).xyz;
   vec3 normal = normalize(texture(normalImage, texCoords).rgb);
   vec3 randomVec = normalize(texture(noiseTexture, texCoords * noiseScale).xyz);
 
   // create TBN change-of-basis matrix: from tangent-space to view-space
   vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
   vec3 bitangent = cross(normal, tangent);
   mat3 TBN = mat3(tangent, bitangent, normal);
 
 //  // iterate over the sample kernel and calculate occlusion factor
  float occlusion = 0.0;
  for(int i = 0; i < kernelSize; ++i) {
      // get sample position
      vec3 samplePos = TBN * kernelUBO.samples[i].xyz; // from tangent to view-space
      samplePos = fragPos + samplePos * radius; 
      
      // project sample position (to sample texture) (to get position on screen/texture)
      vec4 offset = vec4(samplePos, 1.0);
      offset = sceneUBO.projection * offset; // from view to clip-space
      offset.xyz /= offset.w; // perspective divide
      offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
      
      // get sample depth
      float sampleDepth = texture(positionImage, offset.xy).r; // get depth value of kernel sample
      
      // range check & accumulate
      float rangeCheck = smoothstep(0.0, 2.0, radius / abs(fragPos.z - sampleDepth));
      occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck; 
      
  }
 
  occlusion = 1.0 - (occlusion / kernelSize);
  
  outColor = vec4(vec3(occlusion), 1.0f);


 //experiement

//// Get G-Buffer values
//vec3 fragPos = texture(positionImage, texCoords).rgb;
//vec3 normal = normalize(texture(normalImage, texCoords).rgb ) * 2.0 - 1.0;
//
//// Get a random vector using a noise lookup
//ivec2 texDim = textureSize(depthImage, 0); 
//ivec2 noiseDim = textureSize(noiseTexture, 0);
//const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * texCoords;  
//vec3 randomVec = texture(noiseTexture, noiseUV).xyz * 2.0 - 1.0;
//
//// Create TBN matrix
//vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
//vec3 bitangent = cross(tangent, normal);
//mat3 TBN = mat3(tangent, bitangent, normal);
//
//// Calculate occlusion value
//float occlusion = 0.0f;
//
//// remove banding
//for(int i = 0; i < KERNEL_SIZE; i++) {
//
//	vec3 samplePos = TBN * kernelUBO.samples[i].xyz; 
//	samplePos = fragPos + samplePos * radius;
//
//	// project
//	vec4 offset = vec4(samplePos, 1.0f);
//	offset = sceneUBO.projection * offset; 
//	offset.xyz /= offset.w; 
//	offset.xyz = offset.xyz * 0.5f + 0.5f;
//
//	float sampleDepth = -texture(depthImage, offset.xy).r;
//
//	float rangeCheck = smoothstep(0.0f, 1.0f, radius / abs(fragPos.z - sampleDepth));
//	occlusion += (sampleDepth >= samplePos.z + bias ? 1.0f : 0.0f) * rangeCheck;           
//}
//
//occlusion = 1.0 - (occlusion / float(KERNEL_SIZE));
//
//outColor = vec4(occlusion);
}