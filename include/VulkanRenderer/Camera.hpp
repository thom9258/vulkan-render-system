#pragma once

#include "glm.hpp"

namespace World
{
	glm::vec3 static constexpr up(0.0f, 1.0f, 0.0f);
}

struct Camera
{
	Camera(glm::vec3 position, glm::vec3 look_direction, glm::vec3 up) noexcept;
	
	auto view()
		const noexcept -> glm::mat4;
	
	enum class Direction
	{
		Forward,
		Right,
		Up,
	};
	
	void translate(Direction direction, float distance) noexcept;

	auto position() 
		const noexcept -> glm::vec3;
	
	glm::vec3 m_position{0.0f};
	glm::vec3 m_forward{0.0f, 0.0f, -1.0f};
	glm::vec3 m_right{0.0f};
	glm::vec3 m_up{0.0f};
};
