#version 450

#include "Animation.shared"

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 vertex_color;
layout(location = 3) in vec2 vertex_texcoord;
layout(location = 4) in ivec4 vertex_bone_ids; 
layout(location = 5) in vec4 vertex_weights;

layout (set = 0, binding = 0)
uniform Camera
{
	mat4 view;
	mat4 proj;
} camera;

layout (set = 1, binding = 0)
uniform ModelInfo
{
	mat4 model;
	mat4 bone_matrices[MAX_BONES];
} model_info;


void main()
{
	mat4 bone_matrices[MAX_BONE_INFLUENCES];
	for (int i = 0; i < MAX_BONE_INFLUENCES; i++) {
		if (vertex_bone_ids[i] == -1) {
		   continue;
		}

		bone_matrices[i] = model_info.bone_matrices[vertex_bone_ids[i]];
	}

	vec4 animated_vertex = animate_vertex(
		 vertex_position,
		 vertex_bone_ids,
		 vertex_weights,
		 bone_matrices);

	mat4 transform = camera.proj * camera.view * model_info.model;
	gl_Position = transform * animated_vertex;
}
