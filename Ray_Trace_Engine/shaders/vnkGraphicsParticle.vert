#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVelocity;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushValues {
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 viewPosition;
} pushValues;

void main() {

    //gl_PointSize = 10.0;
    //gl_Position = vec4(inPosition.xy, 0.5f, 1.0f);
    //fragColor = inColor.rgb;
		    // Apply 3D transformation
    vec4 worldPosition = pushValues.model * inPosition;

    // Apply view and projection transformations
    gl_Position = pushValues.projection * pushValues.view * worldPosition;
    //gl_Position = vec4(mat3(pushValues.projection) * mat3(pushValues.view) * vec3(worldPosition),1.0f);
    // Set point size
    gl_PointSize = 3.0;

    // Pass color to fragment shader
    fragColor = inColor.rgb;
}