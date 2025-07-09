#version 450

layout(location = 0) in vec4 baseColor;
layout(location = 1) in vec2 texcoord;

layout(set = 1, binding = 0) uniform sampler2D diffuse;
layout(set = 2, binding = 0) uniform sampler2D normal;
layout(set = 3, binding = 0) uniform sampler2D specular;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 diffuse_color = texture(diffuse, texcoord);
    vec4 normal_color = texture(normal, texcoord);
    vec4 specular_color = texture(specular, texcoord);
	outColor = diffuse_color * baseColor;
}
