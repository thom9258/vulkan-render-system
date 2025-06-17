#pragma once

#include <VulkanRenderer/Vertex.hpp>

#include <vulkan/vulkan.hpp>

auto binding_descriptions(const VertexPosNormColor&)
	-> std::array<vk::VertexInputBindingDescription, 1>;

auto attribute_descriptions(const VertexPosNormColor&)
	-> std::array<vk::VertexInputAttributeDescription, 3>;

auto binding_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputBindingDescription, 1>;

auto attribute_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputAttributeDescription, 4>;

