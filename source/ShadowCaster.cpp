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
	return glm::rotate(glm::inverse(view()),
					   glm::radians(180.0f),
					   glm::vec3(1.0f, 0.0f, 0.0f));
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
								   SpotLight light,
								   UpVector up) noexcept
	: m_projection{projection}
	, m_light{light}
	, m_up{up}
{}

glm::mat4 SpotShadowCaster::view() const noexcept
{
	glm::mat4 view =
		glm::lookAt(m_light.position, m_light.position + m_light.direction, m_up.get());
	return view;
}

glm::mat4 SpotShadowCaster::model() const noexcept
{
	glm::mat4 view =
		glm::lookAt(m_light.position, m_light.position + m_light.direction, m_up.get());

	return glm::rotate(glm::inverse(view),
					   glm::radians(180.0f),
					   glm::vec3(1.0f, 0.0f, 0.0f));
}


PerspectiveProjection SpotShadowCaster::projection() const noexcept
{
	return m_projection;
}

SpotLight SpotShadowCaster::light() const noexcept
{
	return m_light;
}
