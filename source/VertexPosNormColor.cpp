#include <VulkanRenderer/VertexPosNormColor.hpp>

auto binding_descriptions(const VertexPosNormColor&)
	-> std::array<vk::VertexInputBindingDescription, 1>
{
	return std::array<vk::VertexInputBindingDescription, 1>{
		vk::VertexInputBindingDescription{}
		.setBinding(0)
		.setStride(sizeof(VertexPosNormColor))
		.setInputRate(vk::VertexInputRate::eVertex),
	};
}

	

auto attribute_descriptions(const VertexPosNormColor&)
	-> std::array<vk::VertexInputAttributeDescription, 3>
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
