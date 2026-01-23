#pragma once

#include "glm.hpp"

#include <memory>

namespace animation {

static constexpr std::size_t max_bone_matrices = 100;
static constexpr std::size_t max_bone_weights = 4;
static constexpr size_t drawable_models_per_frame = 10;

void initialize_mat4(glm::mat4 &m);

void initialize_bone_matrices(glm::mat4 *bone_matrices,
                              size_t bone_matrices_count);

} // namespace animation
