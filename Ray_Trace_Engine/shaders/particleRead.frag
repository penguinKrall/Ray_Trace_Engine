#version 450

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInputMS inputColor;

layout (location = 0) out vec4 outColor;

void main() {

		vec4 resultColor = vec4(0.0);

    for (int i = 0; i < 4; ++i) {
        resultColor += subpassLoad(inputColor, i);
    }

    outColor = resultColor / float(4);

	}
