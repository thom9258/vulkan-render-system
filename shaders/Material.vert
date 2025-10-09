#version 450

#include "Material.shared"

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 vertex_color;
layout(location = 3) in vec2 vertex_texcoord;

layout(location = 0) out vec2 out_texcoord;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_fragpos;
layout(location = 3) out vec3 out_view_position;
layout(location = 4) out vec4 out_dirshadowcaster_lightspace_fragpos;
layout(location = 5) out vec4 out_spotshadowcaster_lightspace_fragpos;

layout( push_constant )
uniform constants
{
	mat4 model;
} push;

layout (set = 0, binding = 0)
uniform GlobalBindings
{
	mat4 view;
	mat4 proj;
	vec4 camera_position;
} global;

layout (set = 0, binding = 5)
uniform DirectionalShadowCasterUniform 
{
	DirectionalLight light;
	mat4 viewproj_matrix;
	bool exists;
} directional_shadowcaster;

layout (set = 0, binding = 6)
uniform SpotShadowCasterUniform 
{
	SpotLight light;
	mat4 viewproj_matrix;
	bool exists;
} spot_shadowcaster;



void main()
{
     mat4 transform = global.proj * global.view * push.model;
	 gl_Position = transform * vec4(vertex_position, 1.0);

	 out_texcoord = vertex_texcoord;

	 // world space vertex normal from model space vertex normal
	 out_normal = mat3(transpose(inverse(push.model))) * vertex_normal;   
	 out_fragpos = vec3(push.model * vec4(vertex_position, 1.0));
 	 out_view_position = vec3(global.camera_position);

	 out_dirshadowcaster_lightspace_fragpos =
	     directional_shadowcaster.viewproj_matrix * vec4(out_fragpos, 1.0);
		 
	 out_spotshadowcaster_lightspace_fragpos =
	     spot_shadowcaster.viewproj_matrix * vec4(out_fragpos, 1.0);
}
