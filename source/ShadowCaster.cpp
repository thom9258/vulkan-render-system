#include <VulkanRenderer/ShadowCaster.hpp>

DirectionalShadowCaster::DirectionalShadowCaster(OrthographicProjection projection,
							DirectionalLight Light,
							glm::vec3 position,
							glm::vec3 up) noexcept
	: projection{projection}
	, light{light}
	, position{position}
	, up{up}
{}

std::optional<glm::mat4> DirectionalShadowCaster::view() const noexcept
{
	glm::mat4 view =
		glm::lookAt(position, position + light.direction, up);
	view[1][1] *= -1;
	return view;
}

std::optional<glm::mat4> DirectionalShadowCaster::model() const noexcept
{
	glm::mat4 view =
		glm::lookAt(position, position + light.direction, up);
	return glm::inverse(view);
}

SpotShadowCaster::SpotShadowCaster(PerspectiveProjection projection,
								   SpotLight Light,
								   glm::vec3 up) noexcept
	: projection{projection}
	, light{light}
	, up{up}
{}

std::optional<glm::mat4> SpotShadowCaster::view() const noexcept
{
	glm::mat4 view =
		glm::lookAt(light.position, light.position + light.direction, up);
	view[1][1] *= -1;
	return view;
}

std::optional<glm::mat4> SpotShadowCaster::model() const noexcept
{
	glm::mat4 view =
		glm::lookAt(light.position, light.position + light.direction, up);
	return glm::inverse(view);
}


