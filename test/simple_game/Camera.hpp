#pragma once

#include <VulkanRenderer/glm.hpp>

class Camera {
public:
  Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up,
         glm::mat4 projection) {
    lookat(position, target, up);
    m_projection = projection;
  }

  glm::mat4 view() {
    return glm::translate(glm::inverse(glm::mat4(m_rotation)), m_position);
  }
  glm::mat4 projection() { return m_projection; }
  glm::vec3 position() { return -m_position; }

  void lookat(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    m_position = -position;
    m_rotation = glm::mat3(glm::inverse(glm::lookAt(position, target, up)));
  }

private:
  glm::vec3 m_position;
  glm::mat3 m_rotation;
  glm::mat4 m_projection;
};
