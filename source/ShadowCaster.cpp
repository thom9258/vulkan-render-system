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
	std::cout
		<< std::format("Constructor pos {} {} {} dir {} {} {}",
					   m_position.get()[0],
					   m_position.get()[1],
					   m_position.get()[2],
					   m_light.direction[0],
					   m_light.direction[1],
					   m_light.direction[2])
		<< std::endl;
}

std::optional<glm::mat4> DirectionalShadowCaster::view() const noexcept
{
	std::cout
		<< std::format("view() pos {} {} {} dir {} {} {}",
					   m_position.get()[0],
					   m_position.get()[1],
					   m_position.get()[2],
					   m_light.direction[0],
					   m_light.direction[1],
					   m_light.direction[2])
		<< std::endl;

	glm::mat4 view =
		glm::lookAt(m_position.get(), m_position.get() + m_light.direction, m_up.get());

	view[1] *= -1;
	return view;
}

std::optional<glm::mat4> DirectionalShadowCaster::model() const noexcept
{
	glm::mat4 view =
		glm::lookAt(m_position.get(), m_position.get() + m_light.direction, m_up.get());

	return glm::inverse(view);
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


