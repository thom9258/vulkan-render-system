#include <VulkanRenderer/ShadowCaster.hpp>

#include <iostream>
#include <format>

DirectionalShadowCaster::DirectionalShadowCaster(OrthographicProjection projection,
							DirectionalLight light,
							PositionVector position,
							UpVector up) noexcept
	: m_projection{projection}
	, m_light{light}
	, m_position{position}
	, m_up{up}
{
}

glm::mat4 DirectionalShadowCaster::view() const noexcept
{
	glm::mat4 view =
		glm::lookAt(m_position.get(), m_position.get() + m_light.direction, m_up.get());
	// NOTE: it seems that orthographic projections does not require flipping the z
	//       direction the same way a perspective projection needs to (in vulkan!).
	//view[1][1] *= -1;
	return view;
}

glm::mat4 DirectionalShadowCaster::model() const noexcept
{
	auto model = glm::inverse(view());
	float angle = 180.0f;
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	model = glm::rotate(model, glm::radians(angle), right);
	return model;
}


OrthographicProjection DirectionalShadowCaster::projection() const noexcept
{
	return m_projection;
}

DirectionalLight DirectionalShadowCaster::light() const noexcept
{
	return m_light;
}


































































SpotShadowCaster::SpotShadowCaster(PerspectiveProjection projection,
								   SpotLight Light,
								   UpVector up) noexcept
	: projection{projection}
	, light{light}
	, up{up}
{}

std::optional<glm::mat4> SpotShadowCaster::view() const noexcept
{
	glm::mat4 view =
		glm::lookAt(light.position, light.position + light.direction, up.get());
	view[1][1] *= -1;
	return view;
}

std::optional<glm::mat4> SpotShadowCaster::model() const noexcept
{
	glm::mat4 view =
		glm::lookAt(light.position, light.position + light.direction, up.get());
	return glm::inverse(view);
}


