#pragma once

#include "glm.hpp"
#include "StrongType.hpp"
#include "Light.hpp"

#include <optional>
#include <vector>

using OrthographicProjection = StrongType<glm::mat4, struct OrthographicProjectionTag>;
using PerspectiveProjection = StrongType<glm::mat4, struct PerspectiveProjectionTag>;

struct DirectionalShadowCaster
{
	DirectionalShadowCaster(OrthographicProjection projection,
							DirectionalLight Light,
							glm::vec3 position,
							glm::vec3 up) noexcept;

	std::optional<glm::mat4> view() const noexcept;
	std::optional<glm::mat4> model() const noexcept;

	OrthographicProjection projection;
	DirectionalLight light;
	glm::vec3 position;
	glm::vec3 up;
};

struct SpotShadowCaster
{
	SpotShadowCaster(PerspectiveProjection projection,
					 SpotLight Light,
					 glm::vec3 up) noexcept;

	std::optional<glm::mat4> view() const noexcept;
	std::optional<glm::mat4> model() const noexcept;

	PerspectiveProjection projection;
	SpotLight light;
	glm::vec3 up;
};

struct ShadowCasters
{
	std::optional<DirectionalShadowCaster> directional_caster;
	std::vector<SpotShadowCaster> spot_casters;
};
