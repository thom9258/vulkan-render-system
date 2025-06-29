#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexcoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 texcoord;

layout( push_constant )
uniform constants
{
	mat4 model;
} push;

layout (set = 0, binding = 0)
uniform Camera
{
	mat4 view;
	mat4 proj;
} camera;


void main() {
    mat4 transform = camera.proj * camera.view * push.model;
    gl_Position = transform * vec4(inPosition, 1.0);
    fragColor = inColor;
    texcoord = inTexcoord;
}
