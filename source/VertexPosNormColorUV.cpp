#include <VulkanRenderer/VertexPosNormColorUV.hpp>

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
