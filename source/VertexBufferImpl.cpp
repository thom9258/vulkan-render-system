#include "VertexBufferImpl.hpp"

#if 0
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
#endif


VertexBuffer::Impl::Impl(Render::Context::Impl* context,
		 void* vertices,
		 size_t vertices_length,
		 size_t vertex_memory_size)
{
	auto physical_device = context->physical_device;
	auto device = context->device.get();

	length = vertices_length;

	const auto buffer_info = vk::BufferCreateInfo{}
		.setSize(vertices_length * vertex_memory_size)
		.setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
		.setSharingMode(vk::SharingMode::eExclusive);
	buffer = device.createBufferUnique(buffer_info);
	
	const auto memory_requirements =
		device.getBufferMemoryRequirements(buffer.get());
	const auto memory_properties = physical_device.getMemoryProperties();
	
	auto memorytype = findMemoryType(memory_properties,
									 memory_requirements.memoryTypeBits,
									 vk::MemoryPropertyFlagBits::eHostVisible
									 | vk::MemoryPropertyFlagBits::eHostCoherent);

	const auto allocate_info = vk::MemoryAllocateInfo{}
		.setAllocationSize(memory_requirements.size)
		.setMemoryTypeIndex(memorytype);
	
	memory = device.allocateMemoryUnique(allocate_info, nullptr);
	device.bindBufferMemory(buffer.get(), memory.get(), 0);
	
	void* data{nullptr};
	vk::Result result = device.mapMemory(memory.get(),
										 0,
										 memory_requirements.size,
										 vk::MemoryMapFlags(),
										 &data);

	//TODO: return a invalid vertex buffer type!
	if (result != vk::Result::eSuccess)
		throw std::runtime_error("could not map memory!");

	memcpy(data, vertices, static_cast<size_t>(memory_requirements.size));
	device.unmapMemory(memory.get());
}



VertexBuffer::VertexBuffer(Render::Context& context,
						   void* vertices,
						   size_t vertices_length,
						   size_t vertex_memory_size)
	: impl(std::make_unique<Impl>(context.impl.get(),
								  vertices,
								  vertices_length,
								  vertex_memory_size))
{
}
	
