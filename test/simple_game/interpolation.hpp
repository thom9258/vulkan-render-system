#pragma once

#include <VulkanRenderer/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace interpolation {

struct Transform {
  Transform() = default;
  Transform(glm::mat4 m) {
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(m, scale, rotation, translation, skew, perspective);
  };

  glm::vec3 scale;
  glm::quat rotation;
  glm::vec3 translation;

  glm::mat4 as_mat4() {
	 glm::mat4 mtranslation = glm::translate(glm::mat4(1.0f), translation); 
	 glm::mat4 mrotation = glm::toMat4(rotation); 
	 glm::mat4 mscale = glm::scale(glm::mat4(1.0f), scale); 
	 return mtranslation * mrotation * mscale;
  }
};

Transform interpolate(Transform a, Transform b, float delta) {
  Transform out;
  out.translation = glm::mix(a.translation, b.translation, delta);
  out.rotation = glm::slerp(a.rotation, b.rotation, delta);
  out.scale = glm::mix(a.scale, b.scale, delta);
  return out;
}

glm::mat4 interpolate_position(glm::mat4 a, glm::mat4 b, float delta) {
  glm::vec3 pos = glm::mix(a[3], b[3], delta);
  return glm::translate(glm::mat4(1.0f), pos);
}

glm::mat4 interpolate_rotation(glm::mat4 a, glm::mat4 b, float delta) {
  glm::quat rot = glm::slerp(glm::toQuat(a), glm::toQuat(b), delta);
  return glm::toMat4(rot);
}

glm::mat4 interpolate_scale(glm::mat4 a, glm::mat4 b, float delta) {
// glm::vec3 scale = glm::mix(a[3], b[3], delta);
// return glm::translate(glm::mat4(1.0f), scale);
}

glm::mat4 interpolate(glm::mat4 a, glm::mat4 b, float delta) {
  glm::mat4 translation = interpolate_position(a, b, delta);
  glm::mat4 rotation = interpolate_rotation(a, b, delta);
 // glm::mat4 scale = interpolate_scale(a, b, delta);
  return translation * rotation /** scale*/;
}

} // namespace interpolation
