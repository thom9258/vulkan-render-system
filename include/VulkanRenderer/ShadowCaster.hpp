#pragma once

#include "glm.hpp"
#include "StrongType.hpp"
#include "Light.hpp"

#include <optional>
#include <vector>

using OrthographicProjection = StrongType<glm::mat4, struct OrthographicProjectionTag>;
using PerspectiveProjection = StrongType<glm::mat4, struct PerspectiveProjectionTag>;
using PositionVector = StrongType<glm::vec3, struct PositionVectorTag>;
using UpVector = StrongType<glm::vec3, struct UpVectorTag>;

struct DirectionalShadowCaster
{
	DirectionalShadowCaster(OrthographicProjection projection,
							DirectionalLight Light,
							PositionVector position,
							UpVector up) noexcept;

	std::optional<glm::mat4> view() const noexcept;
	std::optional<glm::mat4> model() const noexcept;
	OrthographicProjection projection() const noexcept;
	DirectionalLight light() const noexcept;

	OrthographicProjection m_projection;
	DirectionalLight m_light{};
	PositionVector m_position{glm::vec3(0.0f, 5.0f, 0.0f)};
	UpVector m_up{glm::vec3(0.0f, 1.0f, 0.0f)};
};

struct SpotShadowCaster
{
	SpotShadowCaster(PerspectiveProjection projection,
					 SpotLight Light,
					 UpVector up) noexcept;

	std::optional<glm::mat4> view() const noexcept;
	std::optional<glm::mat4> model() const noexcept;

	PerspectiveProjection projection;
	SpotLight light;
	UpVector up;
};

struct ShadowCasters
{
	std::optional<DirectionalShadowCaster> directional_caster;
	std::vector<SpotShadowCaster> spot_casters;
};
