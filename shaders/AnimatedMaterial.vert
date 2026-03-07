#version 450

#include "Material.shared"
#include "Animation.shared"

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 vertex_color;
layout(location = 3) in vec2 vertex_texcoord;
layout(location = 4) in ivec4 vertex_bone_ids; 
layout(location = 5) in vec4 vertex_weights;

layout(location = 0) out vec2 out_texcoord;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_fragpos;
layout(location = 3) out vec3 out_view_position;
layout(location = 4) out vec4 out_dirshadowcaster_lightspace_fragpos;
layout(location = 5) out vec4 out_spotshadowcaster_lightspace_fragpos;

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

layout(set = 7, binding = 0)
uniform ModelInfo
{
	mat4 model;
	ivec4 bind_info; //TODO: this is redundant
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

	gl_Position = global.proj * global.view * model_info.model * animated_vertex;
    
    out_texcoord = vertex_texcoord;
    
    // world space vertex normal from model space vertex normal
    out_normal = mat3(transpose(inverse(model_info.model))) * vertex_normal;   
    out_fragpos = vec3(model_info.model * vec4(vertex_position, 1.0));
    out_view_position = vec3(global.camera_position);
    
    out_dirshadowcaster_lightspace_fragpos =
        directional_shadowcaster.viewproj_matrix * vec4(out_fragpos, 1.0);
     
    out_spotshadowcaster_lightspace_fragpos =
        spot_shadowcaster.viewproj_matrix * vec4(out_fragpos, 1.0);
}
