#include <VulkanRenderer/Camera.hpp>


Camera::Camera(glm::vec3 position, glm::vec3 look_direction, glm::vec3 up) noexcept
	: m_position{position}
	, m_up{up}
{
	m_forward = glm::normalize(look_direction);
	m_right = glm::normalize(glm::cross(up, m_forward));
}

auto Camera::view()
	const noexcept -> glm::mat4
{
	return glm::lookAt(m_position, m_position + m_forward, m_up);
}

auto Camera::position() 
	const noexcept -> glm::vec3
{
	return m_position;
}

void Camera::translate(Direction direction, float distance) noexcept
{
	switch (direction) {
		case Direction::Forward:
			m_position += distance * glm::normalize(m_forward);
		case Direction::Right:
			m_position += distance * glm::normalize(m_right);
			//m_position += distance * glm::normalize(glm::cross(glm::normalize(m_forward), glm::normalize(m_up)));
		//case Direction::Up:
			//m_position += glm::normalize(glm::cross(m_forward, m_right)) * distance;
	}
}
