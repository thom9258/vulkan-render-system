#version 450

#include "Material.shared"

layout(location = 0) in vec2 in_texcoord;
layout(location = 1) in vec3 in_vertex_normal;
layout(location = 2) in vec3 in_frag_position;
layout(location = 3) in vec3 in_view_position;
layout(location = 4) in vec4 in_dirshadowcaster_lightspace_fragpos;
layout(location = 5) in vec4 in_spotshadowcaster_lightspace_fragpos;

layout(location = 0) out vec4 final_color;

layout (set = 0, binding = 1)
uniform PointLightUniform { PointLight pointlight[MAX_POINTLIGHTS]; };

layout (set = 0, binding = 2)
uniform SpotLightUniform { SpotLight spotlight[MAX_SPOTLIGHTS]; };

layout (set = 0, binding = 3)
uniform DirectionalLightUniform { DirectionalLight directionallight[MAX_DIRECTIONALLIGHTS]; };

layout (set = 0, binding = 4)
uniform LightLengthsUniform { 
	ivec3 light_length;
	// light_length.x = pointlight length
	// light_length.y = spotlight length
	// light_length.z = directionallight length
};

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


layout(set = 1, binding = 0) 
uniform sampler2D ambient;

layout(set = 2, binding = 0) 
uniform sampler2D diffuse;

layout(set = 3, binding = 0) 
uniform sampler2D specular;

layout(set = 4, binding = 0) 
uniform sampler2D normal;

layout(set = 5, binding = 0)
uniform sampler2D directional_shadowmap;

layout(set = 6, binding = 0)
uniform sampler2D spot_shadowmap;


vec3 calculate_point_light(PointLight light);
vec3 calculate_directional_light(DirectionalLight light);
vec3 calculate_spot_light(SpotLight light);
bool is_in_directional_shadow(vec4 fragpos_lightspace);
bool is_in_spot_shadow(vec4 fragpos_lightspace);

void main() 
{
	const int pointlight_length = light_length.x;
	const int spotlight_length = light_length.y;
	const int directionallight_length = light_length.z;

	vec3 total_lighting = vec3(0.0);

	for (int i = 0; i < pointlight_length; i++)
		total_lighting += calculate_point_light(pointlight[i]);

	for (int i = 0; i < spotlight_length; i++)
		total_lighting += calculate_spot_light(spotlight[i]);

	for (int i = 0; i < directionallight_length; i++) {
		total_lighting += calculate_directional_light(directionallight[i]);
	}

	vec3 in_light = vec3(1.0, 1.0, 1.0);
	vec3 in_shadow = vec3(0.0, 0.0, 0.0);
	if (directional_shadowcaster.exists) {
#if 0
	   if (!is_in_directional_shadow(in_dirshadowcaster_lightspace_fragpos)) {
	   		final_color = vec4(in_light, 1.0);
	   }
	   else {
	   		final_color = vec4(in_shadow, 1.0);
	   }

#else		
	   if (!is_in_directional_shadow(in_dirshadowcaster_lightspace_fragpos)) {
		  total_lighting += calculate_directional_light(directional_shadowcaster.light);
	   }
	}
	
	if (spot_shadowcaster.exists) {
	   if (!is_in_spot_shadow(in_spotshadowcaster_lightspace_fragpos)) {
		  total_lighting += calculate_spot_light(spot_shadowcaster.light);
	   }
	}

	final_color = vec4(total_lighting, 1.0);
#endif
}

#define SHADOW_BIAS 0.005 
#define SHININESS 32

bool is_in_directional_shadow(vec4 fragpos_lightspace)
{
	vec3 projection_coords = fragpos_lightspace.xyz / fragpos_lightspace.w;
	if (projection_coords.z > 1.0)
	   return false;

    // in vulkan only xy needs to be converted as z is already in [0,1]
	vec2 tex_coords = projection_coords.xy * 0.5 + 0.5;
	float closest_depth = texture(directional_shadowmap, tex_coords).r;
	float current_depth = projection_coords.z;
	return (current_depth - SHADOW_BIAS) > closest_depth;
}

bool is_in_spot_shadow(vec4 fragpos_lightspace)
{
	vec3 projection_coords = fragpos_lightspace.xyz / fragpos_lightspace.w;
	if (projection_coords.z > 1.0)
	   return false;

    // in vulkan only xy needs to be converted as z is already in [0,1]
	vec2 tex_coords = projection_coords.xy * 0.5 + 0.5;
	float closest_depth = texture(spot_shadowmap, tex_coords).r;
	float current_depth = projection_coords.z;
	return (current_depth - SHADOW_BIAS) > closest_depth;
}

vec3 calculate_point_light(PointLight light)
{
    vec3 lightDir = normalize(light.position - in_frag_position);
    vec3 viewDir = normalize(in_view_position - in_frag_position);

    // diffuse shading
    vec3 normal = normalize(in_vertex_normal);
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);

    // attenuation
    float distance = length(light.position - in_frag_position);
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));    

    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse, in_texcoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse, in_texcoord));
    vec3 specular = light.specular * spec * vec3(texture(specular, in_texcoord));
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
	
	float normal_dot_length = dot(normal, lightDir);
	if (normal_dot_length > 0.0) 
		return (ambient + diffuse + specular);
	else
		return ambient;
}

vec3 calculate_directional_light(DirectionalLight light)
{
    vec3 lightDir = normalize(-light.direction);
    vec3 viewDir = normalize(in_view_position - in_frag_position);

    // diffuse shading
    vec3 normal = normalize(in_vertex_normal);
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);

    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse, in_texcoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse, in_texcoord));
    vec3 specular = light.specular * spec * vec3(texture(specular, in_texcoord));
	

	float normal_dot_length = dot(normal, lightDir);
	if (normal_dot_length > 0.0) 
		return (ambient + diffuse + specular);
	else
		return ambient;
}

vec3 calculate_spot_light(SpotLight light)
{
    vec3 lightDir = normalize(light.position - in_frag_position);
    vec3 viewDir = normalize(in_view_position - in_frag_position);

    // diffuse shading
    vec3 normal = normalize(in_vertex_normal);
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), SHININESS);

    // attenuation
    float distance = length(light.position - in_frag_position);
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));    
	
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
	const float inner_cutoff = light.cutoff.x;
	const float outer_cutoff = light.cutoff.y;
    float epsilon = inner_cutoff - outer_cutoff;
    float intensity = clamp((theta - outer_cutoff) / epsilon, 0.0, 1.0);

    // combine results
    vec3 ambient = light.ambient * vec3(texture(diffuse, in_texcoord));
    vec3 diffuse = light.diffuse * diff * vec3(texture(diffuse, in_texcoord));
    vec3 specular = light.specular * spec * vec3(texture(specular, in_texcoord));
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
	
	float normal_dot_length = dot(normal, lightDir);
	if (normal_dot_length > 0.0) 
		return (ambient + diffuse + specular);
	else
		return ambient;
}
