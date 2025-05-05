#version 450

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 texcoord;

layout(set = 1, binding = 0) uniform sampler2D tex1;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = texture(tex1, texcoord).xyz;
	outColor = vec4(color, 1.0f);
}
