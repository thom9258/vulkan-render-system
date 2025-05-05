#pragma once

#include "Utils.hpp"

template<typename T>
struct VertexBuffer
{
	using VertexType = std::remove_cvref_t<T>;
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory memory;
	size_t length;
};

template<typename Vertex>
VertexBuffer<Vertex>
create_vertex_buffer(vk::PhysicalDevice& physical_device,
					 vk::Device& device,
					 std::vector<Vertex>& vertices)
{
	const auto buffer_info = vk::BufferCreateInfo{}
		.setSize(sizeof(vertices[0]) * vertices.size())
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
		.setSharingMode(vk::SharingMode::eExclusive);

	VertexBuffer<Vertex> vertexbuffer{};
	vertexbuffer.length = vertices.size();
	vertexbuffer.buffer = device.createBufferUnique(buffer_info);
	
	const auto memory_requirements =
		device.getBufferMemoryRequirements(vertexbuffer.buffer.get());
	const auto memory_properties = physical_device.getMemoryProperties();
	
	auto memorytype = findMemoryType(memory_properties,
									 memory_requirements.memoryTypeBits,
									 vk::MemoryPropertyFlagBits::eHostVisible
									 | vk::MemoryPropertyFlagBits::eHostCoherent);

	const auto allocate_info = vk::MemoryAllocateInfo{}
		.setAllocationSize(memory_requirements.size)
		.setMemoryTypeIndex(memorytype);
	
	vertexbuffer.memory = device.allocateMemoryUnique(allocate_info, nullptr);
	device.bindBufferMemory(vertexbuffer.buffer.get(), vertexbuffer.memory.get(), 0);
	
	void* data{nullptr};
	vk::Result result = device.mapMemory(vertexbuffer.memory.get(),
										 0,
										 memory_requirements.size,
										 vk::MemoryMapFlags(),
										 &data);

	//TODO: return a invalid vertex buffer type!
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("could not map memory!");

	memcpy(data, vertices.data(), static_cast<size_t>(memory_requirements.size));
	device.unmapMemory(vertexbuffer.memory.get());
	
	return vertexbuffer;
}
