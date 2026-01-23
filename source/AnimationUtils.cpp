#include "AnimationUtils.hpp"

namespace animation {

void initialize_mat4(glm::mat4 &m) { m = glm::mat4(1.0f); }

void initialize_bone_matrices(glm::mat4 *bone_matrices,
                              size_t bone_matrices_count) {
  for (size_t i = 0; i < bone_matrices_count; i++) {
    bone_matrices[i] = glm::mat4(1.0f);
  }
}

} // namespace animation
