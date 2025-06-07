#pragma once

#include <vulkan/vulkan.hpp>
#include "glm.hpp"

#include <vector>
#include <array>

struct VertexPosNormColor {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
};

auto binding_descriptions(const VertexPosNormColor&)
	-> std::array<vk::VertexInputBindingDescription, 1>;

auto attribute_descriptions(const VertexPosNormColor&)
	-> std::array<vk::VertexInputAttributeDescription, 3>;
