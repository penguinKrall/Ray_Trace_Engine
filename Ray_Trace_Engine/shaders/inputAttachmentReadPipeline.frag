#version 450

const float LIGHT_RADIUS = 0.03f;
const int MAX_SAMPLES = 8;

layout(input_attachment_index = 0, set = 0, binding = 0) uniform subpassInputMS inputColor;
layout(input_attachment_index = 1, set = 0, binding = 1) uniform subpassInputMS inputDepth;
layout(set = 0, binding = 8) uniform sampler2DMS depthImage;

struct Lighting {
		vec4 ambientColor;
		vec4 diffuseColor;
		vec4 specularColor;
		vec4 lightPosition;
		vec4 lightingValues;
	};

layout(set = 0, binding = 4) uniform UBO {

		vec4 shadowFactorToneMap;
		vec4 lightPos;
		vec4 shadowValues;
		vec2 minMax;
		vec2 shadowsOnOff;
		vec4 visualDebugValues;
    vec4 parallaxMap;
    mat4 lightMVP;
    Lighting lighting;
} ubo;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in mat4 projectionMatrix;
layout(location = 5) in mat4 viewMatrix;
layout(location = 9) in vec4 viewPosition;
//layout(location = 10) in mat4 lightViewProjMatrix;

layout(location = 0) out vec4 outColor;

vec3 getViewPosition(float depth, vec2 uv, mat4 inverseViewMatrix);
vec3 computeNormals(float shadowFactorSum);
vec4 computeWorldPosFromDepth(vec2 tex, float depth, mat4 invProjection, mat4 invView);
float rayMarchShadow(vec4 worldPos, vec3 lightDir, vec2 screenSize,
vec4 lightPosition, mat4 inverseProjectionMatrix, mat4 inverseViewMatrix);
vec3 FilmicToneMapping(vec3 color);

void main() {
		
		vec4 sceneColor = subpassLoad(inputColor, 0);
		vec4 sceneDepth = subpassLoad(inputDepth, 0);

		for (int i = 1; i < 8; ++i) { 
				sceneColor += subpassLoad(inputColor, i);
		}
		sceneColor /= 8.0; 

		vec2 screenSize = vec2(1920.0, 1024.0);
		mat4 lightViewProjMatrix = ubo.lightMVP;
		vec4 colorFromTexture = sceneColor;		
		float depth = sceneDepth.r;
		mat4 inverseViewMatrix;
    mat4 inverseProjectionMatrix;
		vec4 worldPos;
		vec4 lightSpacePos;
		float lightDepth;

		if(ubo.visualDebugValues.x + ubo.visualDebugValues.y > 0){

		inverseViewMatrix = inverse(viewMatrix);
    inverseProjectionMatrix = inverse(projectionMatrix);

		// 1. Fetch the depth values		
		worldPos = computeWorldPosFromDepth(fragTexCoord, depth, inverseProjectionMatrix, inverseViewMatrix);

		//vec4 lightSpacePos = lightViewProjMatrix * viewPosition;
		lightSpacePos = lightViewProjMatrix * vec4(worldPos.xy, 0.5f, 1.0f);
		lightSpacePos.xyz /= lightSpacePos.w;
		//lightSpacePos /= lightSpacePos.w;
		
		lightDepth = lightSpacePos.z;
		
		}

		// 4. (Optional) Transform the view space position to world space

		if(ubo.shadowsOnOff.x == 1.0f){
		// 2. Create a clip space position for the fragment.
		//    Depth values range between 0 and 1, and NDC x/y range between -1 and 1.
		vec4 clipSpacePosition = vec4(fragTexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
		
		// 3. Transform this position from clip space to view space
		vec4 viewSpacePosition = inverseProjectionMatrix * clipSpacePosition;
		viewSpacePosition /= viewSpacePosition.w;		// Divide by w to go from homogeneous coordinates to 3D
		
				float lowerLimit = ubo.shadowValues.z; // This value will be taken when viewPosition.y is at its lowest.
				float upperLimit = ubo.shadowValues.w;  // This value will be taken when viewPosition.y is at its highest.
				
				float minY = ubo.minMax.x; // Minimum expected y value of viewPosition
				float maxY = ubo.minMax.y; // Maximum expected y value of viewPosition
				
				float t = (viewPosition.y - minY) / (maxY - minY); // Maps the y position to a [0, 1] range
				
				float dynamicValue = mix(lowerLimit, upperLimit, t);
				
				vec3 lightDir = normalize(vec3(ubo.lightPos) - worldPos.xyz * dynamicValue);
				
				float shadowFactor = rayMarchShadow(worldPos, lightDir,
				screenSize, ubo.lightPos, inverseProjectionMatrix, inverseViewMatrix);
				
				colorFromTexture.rgb = FilmicToneMapping(colorFromTexture.rgb * vec3(shadowFactor));
	
				//colorFromTexture.rgb = pow(colorFromTexture.rgb, vec3(1.0/2.2));
		}

		//visualize world position as color:
		switch(int(ubo.visualDebugValues.x + ubo.visualDebugValues.y)){

		case(0):
		outColor = sceneColor;
		break;

		case(1): //one of the debugs is active
		//world space debug
		if(int(ubo.visualDebugValues.x) == 1){	
				outColor = vec4(worldPos.xyz * 0.5 + 0.5, 1.0);  // Mapping from [-1, 1] to [0, 1]
				break;
		}
		//light space debug
		if(int(ubo.visualDebugValues.y) == 1){
				outColor = vec4(lightSpacePos.xyz * 0.5 + 0.5, 1.0);
				break;
		}
		break;

		case(2):		//both light and world space debug
		outColor = sceneColor * (vec4(lightSpacePos.xyz * 0.5 + 0.5, 1.0) + vec4(worldPos.xyz * 0.5 + 0.5, 1.0));
		break;

		default:
		break;
		}

		if(ubo.shadowsOnOff.x == 1.0f){

				vec4 finalColor = mix(outColor, colorFromTexture, ubo.shadowFactorToneMap.x);
				
        finalColor = vec4(vec3(1.0) - exp(-finalColor.rgb * ubo.shadowFactorToneMap.y), 1.0f);
				
				finalColor = vec4(pow(finalColor.rgb, vec3(1.0 / ubo.shadowFactorToneMap.z)), 1.0f);

				outColor = finalColor;
		}
}

vec3 getViewPosition(float depth, vec2 uv, mat4 inverseViewMatrix) {
		vec2 ndc = vec2(uv.x, 1.0 - uv.y) * 2.0 - 1.0;  // Invert the Y-axis
		vec4 clipSpacePosition = vec4(ndc, depth * 2.0 - 1.0, 1.0);
		vec4 viewPosition = inverseViewMatrix * clipSpacePosition;
		return viewPosition.xyz / viewPosition.w;
}

vec3 computeNormals(float shadowFactorSum){

				vec3 normal = vec3(1.0f);
				vec2 screenSize = vec2(1920.0, 1024.0);

				// Retrieve depth values
				float depthL = texelFetch(depthImage, ivec2(fragTexCoord * screenSize) + ivec2(-1, 0), 0).r;
				float depthR = texelFetch(depthImage, ivec2(fragTexCoord * screenSize) + ivec2(1, 0), 0).r;
				float depthU = texelFetch(depthImage, ivec2(fragTexCoord * screenSize) + ivec2(0, -1), 0).r;
				float depthD = texelFetch(depthImage, ivec2(fragTexCoord * screenSize) + ivec2(0, 1), 0).r;

				normal.x = (depthL - depthR) * shadowFactorSum;
				normal.y = (depthU - depthD) * shadowFactorSum;
				normal.z = 1.0;
				
				return normal;
		}

		vec4 computeWorldPosFromDepth(vec2 tex, float newDepth, mat4 invProjection, mat4 invView){

				// Convert the depth to NDC (normalized device coordinates)
				vec4 clipSpacePosition = vec4(fragTexCoord * 2.0 - 1.0, newDepth, 1.0);
				//vec4 clipSpacePosition = vec4(tex * 2.0 - 1.0, newDepth * 2.0 - 1.0, 1.0);

				// Convert the NDC position to view space
				vec4 viewSpacePosition = invProjection * clipSpacePosition;
				viewSpacePosition /= viewSpacePosition.w;

				vec4 worldPos = invView * viewSpacePosition;

				return worldPos;
		}

	float rayMarchShadow(vec4 worldPos, vec3 lightDir, vec2 screenSize,
	vec4 lightPosition, mat4 inverseProjectionMatrix, mat4 inverseViewMatrix){
		
				//// Ray marching towards the light
				const int numSteps = 200;

				float maxDistance = distance(vec3(ubo.lightPos), worldPos.xyz);

				float stepSize = maxDistance / float(numSteps);
				
				vec3 offsetStartPos = worldPos.xyz + lightDir * ubo.shadowValues.x; // Add an offset
				
				float shadowFactor = 1.0f;

				for (int i = 0; i < numSteps; ++i) {
						
						vec3 currentPos = offsetStartPos + lightDir * float(i) * stepSize;
						vec4 currentClipPos = projectionMatrix * viewMatrix * vec4(currentPos, 1.0);
						currentClipPos /= currentClipPos.w;
						
						vec2 currentTexCoord = currentClipPos.xy / currentClipPos.w * 0.5 + 0.5;				
						float aggregatedDepth = 0.0;
						
						for (int s = 0; s < MAX_SAMPLES; ++s) {
								aggregatedDepth += texelFetch(depthImage, ivec2(currentTexCoord * screenSize), s).r;
						}
						
						float averageDepth = aggregatedDepth / float(MAX_SAMPLES);

						vec4 currentViewPos = inverseProjectionMatrix * vec4(currentTexCoord * 2.0 - 1.0, averageDepth, 1.0);
						currentViewPos /= currentViewPos.w;
						
						vec4 currentWorldPos = inverseViewMatrix * currentViewPos;

						float dynamicBias = mix(0.005, 0.001, clamp(length(currentViewPos.xyz - worldPos.xyz) / maxDistance, 0.0, ubo.shadowValues.w));
						
						if (length(currentWorldPos.xyz - worldPos.xyz) > maxDistance - LIGHT_RADIUS) {
						    break;
						}
						
						if (length(currentWorldPos.xyz - worldPos.xyz) < (i * stepSize) - dynamicBias * ubo.shadowValues.y) {
						    shadowFactor = ubo.shadowFactorToneMap.w; // This point is in shadow
						    break;
						}
				}

		 return shadowFactor;

		}

vec3 FilmicToneMapping(vec3 color) {
				color = max(vec3(0.0), color - vec3(0.004));
				color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
				return color;
		}