

#pragma once

#include "glm.hpp"
#include "Utils.hpp"

namespace Render
{
	
class Transform
{
	public:
	constexpr Transform() = default;
	constexpr ~Transform() = default;
	constexpr Transform(glm::vec3 const position,
						glm::quat const rotation,
						glm::vec3 const scale)
		: _position{position}
		, _rotation{rotation}
		, _scale{scale}
	{}
	
	glm::vec3& position() { return _position; }
	glm::vec3 const& position() const { return _position; }

	glm::quat& rotation() { return _rotation; }
	glm::quat const& rotation() const { return _rotation; }

	glm::vec3& scale() { return _scale; }
	glm::vec3 const& scale() const { return _scale; }
	
	glm::mat4 as_matrix() const
	{
		return 
			glm::translate(glm::mat4(1.0f), position())
			* glm::toMat4(rotation())
			* glm::scale(glm::mat4(1.0f), scale());
	}
		
private:
	glm::vec3 _position{0.0f};
	glm::quat _rotation{0.0f, 0.0f, 0.0f, 1.0f};
	glm::vec3 _scale{1.0f};
};

auto stringify(Transform const& t)
	-> std::string
{
	return stringify(t.position())
		+ stringify(t.rotation())
		+ stringify(t.scale());
}

}
