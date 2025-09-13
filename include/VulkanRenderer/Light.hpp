#pragma once

#include "glm.hpp"

#include <variant>
#include <algorithm>

struct DirectionalLight
{
	glm::vec3 direction{0.0f, -1.0f, 0.2f};
	glm::vec3 ambient{0.1f};
	glm::vec3 diffuse{1.0f};
	glm::vec3 specular{1.0f};
};

struct Attenuation
{
	float constant;
	float linear;
	float quadratic;

	constexpr auto approximate_distance(float intensity)
		const noexcept -> float
	{
		float F = std::clamp(intensity, 0.0f, 1.0f);
		return ((-constant * F) - (4 * F * quadratic) + 1) / F * linear;
	}
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
