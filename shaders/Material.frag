#version 450

layout(location = 0) in vec2 texcoord;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 frag_position;
layout(location = 3) in vec3 view_position;

layout(location = 0) out vec4 final_color;


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

struct DirectionalLight
{
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};	

struct SpotLight
{
	vec3 position;
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	vec3 attenuation;
	// attenuation.x = constant
	// attenuation.y = linear
	// attenuation.z = quadratic
	vec2 cutoff;
	// cutoff.x = inner
	// cutoff.y = outer
};

const int max_pointlights = 10;
const int max_spotlights = 10;
const int max_directionallights = 10;

layout (set = 0, binding = 1)
uniform PointLightUniform { PointLight pointlight[max_pointlights]; };

layout (set = 0, binding = 2)
uniform SpotLightUniform { SpotLight spotlight[max_spotlights]; };

layout (set = 0, binding = 3)
uniform DirectionalLightUniform { DirectionalLight directionallight[max_directionallights]; };

layout (set = 0, binding = 4)
uniform LightLengthsUniform { 
	ivec3 light_length;

	// light_length.x = pointlight length
	// light_length.y = spotlight length
	// light_length.z = directionallight length
};

layout(set = 1, binding = 0) 
uniform sampler2D ambient;

layout(set = 2, binding = 0) 
uniform sampler2D diffuse;

layout(set = 3, binding = 0) 
uniform sampler2D specular;

layout(set = 4, binding = 0) 
uniform sampler2D normal;

vec3 calculate_point_light(PointLight light)
{
    float distance = length(light.position - frag_position);
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance)); 
    
    vec3 diffuse_texture = texture(diffuse, texcoord).rgb;
    vec3 ambient = light.ambient * diffuse_texture * attenuation;

   	vec3 light_direction = normalize(light.position - frag_position);  
    float normal_dot_length = dot(vertex_normal, light_direction);
    if (normal_dot_length > 0.0) 
    {
        vec3 normalized_normal = normalize(vertex_normal);
        float diffuse_impact = max(dot(normalized_normal, light_direction), 0.0);
        vec3 diffuse = light.diffuse * diffuse_impact * diffuse_texture * attenuation;
        
        vec3 view_direction = normalize(view_position - frag_position);
		vec3 reflect_direction = reflect(-light_direction, normalized_normal);  
		float specular_impact = pow(max(dot(view_direction, reflect_direction), 0.0), 32);
	  	vec3 specular_texture = texture(specular, texcoord).rgb;
		vec3 specular = light.specular * specular_impact * specular_texture* attenuation;
		return ambient + diffuse + specular;
    }

	return ambient;
}

vec3 calculate_directional_light(DirectionalLight light)
{
	 vec3 light_direction = normalize(-light.direction);  
     vec3 diffuse_texture = texture(diffuse, texcoord).rgb;
 	 vec3 ambient = light.ambient * diffuse_texture;

	 float normal_dot_length = dot(vertex_normal, light_direction);
	 if (normal_dot_length > 0.0) 
	 {
          vec3 normalized_normal = normalize(vertex_normal);
          float diffuse_impact = max(dot(normalized_normal, light_direction), 0.0);
          vec3 reflect_direction = reflect(-light_direction, normalized_normal);  
          vec3 view_direction = normalize(view_position - frag_position);
          float specular_impact = pow(max(dot(view_direction, reflect_direction), 0.0), 32);
          
          vec3 diffuse = light.diffuse * diffuse_impact * diffuse_texture;
          vec3 specular = light.specular * specular_impact * texture(specular, texcoord).rgb;
          return ambient + diffuse + specular;
	 }

	 return ambient;
}

vec3 calculate_spot_light(SpotLight light)
{
    float distance = length(light.position - frag_position);
    float constant = light.attenuation.x;
    float linear = light.attenuation.y;
    float quadratic = light.attenuation.z;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance)); 
	
    vec3 light_direction = normalize(light.position - frag_position);  
	float theta = dot(light_direction, normalize(-light.direction));
	float inner_cutoff = light.cutoff.x;
	float outer_cutoff = light.cutoff.y;
	float epsilon = inner_cutoff - outer_cutoff;
	float intensity = clamp((theta - inner_cutoff) / epsilon, 0.0, 1.0);

    vec3 diffuse_texture = texture(diffuse, texcoord).rgb;
    vec3 ambient = light.ambient * diffuse_texture * attenuation * intensity;

	vec3 normalized_normal = normalize(vertex_normal);
	float diffuse_impact = max(dot(normalized_normal, light_direction), 0.0);
	vec3 diffuse = light.diffuse * diffuse_impact * diffuse_texture * attenuation * intensity;
	
	vec3 reflect_direction = reflect(-light_direction, normalized_normal);  
	vec3 view_direction = normalize(view_position - frag_position);
	float specular_impact = pow(max(dot(view_direction, reflect_direction), 0.0), 32);
	vec3 specular = light.specular * specular_impact * texture(specular, texcoord).rgb * attenuation * intensity;
	
	return ambient + diffuse + specular;	
}

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

	for (int i = 0; i < directionallight_length; i++)
		total_lighting += calculate_directional_light(directionallight[i]);
	
	final_color = vec4(total_lighting, 1.0);
}
