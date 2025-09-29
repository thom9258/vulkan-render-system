#pragma once

#include <VulkanRenderer/Light.hpp>
#include <VulkanRenderer/ShadowCaster.hpp>


struct DirectionalLightUniformData
{
	glm::vec3 direction;
	float _padding1{1.0f};
	glm::vec3 ambient;
	float _padding2{1.0f};
	glm::vec3 diffuse;
	float _padding3{1.0f};
	glm::vec3 specular;
	float _padding4{1.0f};
	
	DirectionalLightUniformData() = default;
	DirectionalLightUniformData(DirectionalLight light);
	DirectionalLightUniformData& operator=(DirectionalLight light);
};

struct PointLightUniformData
{
	glm::vec3 position;
	float _padding1{1.0f};
	glm::vec3 ambient;
	float _padding2{1.0f};
	glm::vec3 diffuse;
	float _padding3{1.0f};
	glm::vec3 specular;
	float _padding4{1.0f};
	float attenuation_constant;
	float attenuation_linear;
	float attenuation_quadratic;
	float _padding5{1.0f};
	
	PointLightUniformData() = default;
	PointLightUniformData(PointLight light);
	PointLightUniformData& operator=(PointLight light);
};

struct SpotLightUniformData
{
	glm::vec3 position;
	float _padding1{1.0f};
	glm::vec3 direction;
	float _padding2{1.0f};
	glm::vec3 ambient;
	float _padding3{1.0f};
	glm::vec3 diffuse;
	float _padding4{1.0f};
	glm::vec3 specular;
	float _padding5{1.0f};
	float attenuation_constant;
	float attenuation_linear;
	float attenuation_quadratic;
	float _padding6{1.0f};
	float cutoff_inner;
	float cutoff_outer;
	float _padding7[2]{1.0f, 1.0f};
	
	SpotLightUniformData() = default;
	SpotLightUniformData(SpotLight light);
	SpotLightUniformData& operator=(SpotLight light);
};

struct DirectionalShadowCasterUniformData
{
	glm::vec3 direction;
	float _padding1{1.0f};
	glm::vec3 ambient;
	float _padding2{1.0f};
	glm::vec3 diffuse;
	float _padding3{1.0f};
	glm::vec3 specular;
	float _padding4{1.0f};
	glm::mat4 viewproj_matrix;
	int exists{false};
	float _padding5[3];
	
	DirectionalShadowCasterUniformData() = default;
	DirectionalShadowCasterUniformData(DirectionalShadowCaster caster);
};


