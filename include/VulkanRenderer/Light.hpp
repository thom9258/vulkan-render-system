#pragma once

#include "glm.hpp"
//#include "ShaderTexture.hpp"

#include <variant>

struct AreaLight
{
	glm::vec3 direction;
	glm::vec4 color;
};

struct PointLight
{
	glm::vec3 position;
	glm::vec4 color;
};

struct SpotLight
{
	glm::vec3 position;
	glm::vec3 direction;
	glm::vec4 color;
};

using Light = std::variant<AreaLight,
						   PointLight,
						   SpotLight>;
