#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inVertexColor;
layout(location = 3) in vec2 inTexcoord;

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out vec2 texcoord;

layout( push_constant )
uniform constants
{
	mat4 model;
	vec4 basecolor;
} push;

layout (set = 0, binding = 0)
uniform GlobalBindings
{
	mat4 view;
	mat4 proj;
} global;

void main() {
    mat4 transform = global.proj * global.view * push.model;
    gl_Position = transform * vec4(inPosition, 1.0);
    outBaseColor = push.basecolor;
    texcoord = inTexcoord;
}
