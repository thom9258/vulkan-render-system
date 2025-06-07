#pragma once

#include <vulkan/vulkan.hpp>
#include "glm.hpp"

#include <vector>
#include <array>

struct VertexPosNormColorUV {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
    glm::vec2 uv;
};


auto binding_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputBindingDescription, 1>;

auto attribute_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputAttributeDescription, 4>;
