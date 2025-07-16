#pragma once

#include "glm.hpp"
//#include "ShaderTexture.hpp"

#include <variant>

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	struct {
		float constant;
		float linear;
		float quadratic;
	}attenuation;
};

struct SpotLight
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	struct {
		float constant;
		float linear;
		float quadratic;
	}attenuation;
	struct {
		float inner;
		float outer;
	}cutoff;
};

using Light = std::variant<DirectionalLight,
						   PointLight,
						   SpotLight>;
