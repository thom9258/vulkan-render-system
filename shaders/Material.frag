#version 450

layout(set = 1, binding = 0) uniform sampler2D ambient;
layout(set = 2, binding = 0) uniform sampler2D diffuse;
layout(set = 3, binding = 0) uniform sampler2D specular;
layout(set = 4, binding = 0) uniform sampler2D normal;

struct PointLight
{
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 attenuation;
	// attenuation.x = constant
	// attenuation.y = linear
	// attenuation.z = quadratic
};	

layout (set = 5, binding = 0)
uniform PointLightUniform { PointLight pointlight; };


layout(location = 0) in vec2 texcoord;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 frag_position;
layout(location = 3) in vec3 view_position;

layout(location = 0) out vec4 final_color;

void main() 
{
	 vec3 total_lighting = vec3(0.0);
	 
	 float distance = length(pointlight.position - frag_position);
	 float attenuation = 1.0 / (pointlight.attenuation.x + pointlight.attenuation.y * distance + 
    		    pointlight.attenuation.z * (distance * distance)); 
	 
	 vec3 light_direction = normalize(pointlight.position - frag_position);  
	 vec3 normalized_normal = normalize(vertex_normal);
	 float diffuse_impact = max(dot(normalized_normal, light_direction), 0.0);
	 vec3 diffuse_texture = texture(diffuse, texcoord).rgb;
	 vec3 diffuse_impact_color = pointlight.diffuse * diffuse_impact * diffuse_texture;
	 total_lighting += diffuse_impact_color * attenuation;
	 
	 vec3 ambient_color = diffuse_texture;
	 vec3 ambient_impact_color = pointlight.ambient * ambient_color;
	 total_lighting += ambient_impact_color * attenuation;
	 
	 vec3 view_direction = normalize(view_position - frag_position);
	 float normal_dot_length = dot(vertex_normal, light_direction);
	 if (normal_dot_length > 0.0) 
	 {
		vec3 reflect_direction = reflect(-light_direction, normalized_normal);  

		float SHININESS = 1.0;
		float specular_impact = pow(max(dot(view_direction, reflect_direction), 0.0), 32 * SHININESS);
	  	vec3 specular_texture = texture(specular, texcoord).rgb;
		vec3 specular_impact_color = pointlight.specular * specular_impact * specular_texture;

		float SPECULAR_INTENSITY = 1.0;
		total_lighting += specular_impact_color * SPECULAR_INTENSITY * attenuation;
	 }
	 
	 final_color = vec4(total_lighting, 1.0);
}
