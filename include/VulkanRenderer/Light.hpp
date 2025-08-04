#pragma once

#include "glm.hpp"

#include <variant>

struct DirectionalLight
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
};

struct Attenuation
{
	float constant;
	float linear;
	float quadratic;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	Attenuation attenuation;
};

struct Cutoff {
	float inner;
	float outer;
};

struct SpotLight
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;
	Attenuation attenuation;
	Cutoff cutoff;
};

using Light = std::variant<DirectionalLight,
						   PointLight,
						   SpotLight>;
