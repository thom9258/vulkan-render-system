#pragma once

#include <VulkanRenderer/Vertex.hpp>

#include <vulkan/vulkan.hpp>

auto binding_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputBindingDescription, 1>;

auto attribute_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputAttributeDescription, 4>;

auto binding_descriptions(const VertexAnimatedPosNormColorUV&)
	-> std::array<vk::VertexInputBindingDescription, 1>;

auto attribute_descriptions(const VertexAnimatedPosNormColorUV&)
	-> std::array<vk::VertexInputAttributeDescription, 6>;

