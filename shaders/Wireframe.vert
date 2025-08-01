#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout( push_constant ) uniform constants
{
	vec4 color;
	mat4 modelviewproj;
} push;

void main() {
    gl_Position = push.modelviewproj * vec4(inPosition, 1.0);
    fragColor = vec3(push.color);
}
