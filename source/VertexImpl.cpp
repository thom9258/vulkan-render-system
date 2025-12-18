#include "VertexImpl.hpp"

auto binding_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputBindingDescription, 1>
{
	return std::array<vk::VertexInputBindingDescription, 1>{
		vk::VertexInputBindingDescription{}
		.setBinding(0)
		.setStride(sizeof(VertexPosNormColorUV))
		.setInputRate(vk::VertexInputRate::eVertex),
	};
}

auto attribute_descriptions(const VertexPosNormColorUV&)
	-> std::array<vk::VertexInputAttributeDescription, 4>
{
	return std::array<vk::VertexInputAttributeDescription, 4>{
		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosNormColorUV, pos)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(1)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosNormColorUV, norm)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(2)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexPosNormColorUV, color)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(3)
		.setFormat(vk::Format::eR32G32Sfloat)
		.setOffset(offsetof(VertexPosNormColorUV, uv)),
	};
}

auto binding_descriptions(const VertexAnimatedPosNormColorUV&)
	-> std::array<vk::VertexInputBindingDescription, 1>
{
	return std::array<vk::VertexInputBindingDescription, 1>{
		vk::VertexInputBindingDescription{}
		.setBinding(0)
		.setStride(sizeof(VertexAnimatedPosNormColorUV))
		.setInputRate(vk::VertexInputRate::eVertex),
	};
}

auto attribute_descriptions(const VertexAnimatedPosNormColorUV&)
	-> std::array<vk::VertexInputAttributeDescription, 6>
{
	return std::array<vk::VertexInputAttributeDescription, 6>{
		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(0)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexAnimatedPosNormColorUV, pos)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(1)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexAnimatedPosNormColorUV, norm)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(2)
		.setFormat(vk::Format::eR32G32B32Sfloat)
		.setOffset(offsetof(VertexAnimatedPosNormColorUV, color)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(3)
		.setFormat(vk::Format::eR32G32Sfloat)
		.setOffset(offsetof(VertexAnimatedPosNormColorUV, uv)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(4)
		.setFormat(vk::Format::eR32G32B32Sint)
		.setOffset(offsetof(VertexAnimatedPosNormColorUV, bone_ids)),

		vk::VertexInputAttributeDescription{}
		.setBinding(0)
		.setLocation(5)
		.setFormat(vk::Format::eR32G32B32A32Sfloat)
		.setOffset(offsetof(VertexAnimatedPosNormColorUV, weights)),
	};
}
