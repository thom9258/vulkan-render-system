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

std::array<vk::VertexInputBindingDescription, 1>
binding_descriptions(const VertexPosNormColor&)
{
	return std::array<vk::VertexInputBindingDescription, 1>{
		vk::VertexInputBindingDescription{}
		.setBinding(0)
		.setStride(sizeof(VertexPosNormColor))
		.setInputRate(vk::VertexInputRate::eVertex),
	};
}

	
std::array<vk::VertexInputAttributeDescription, 3>
attribute_descriptions(const VertexPosNormColor&)
{
	return std::array<vk::VertexInputAttributeDescription, 3>{
		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosNormColor, pos)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(1)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosNormColor, norm)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(2)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosNormColor, color)),

	};
}


