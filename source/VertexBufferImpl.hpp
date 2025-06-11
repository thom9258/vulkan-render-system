#pragma once

#include <VulkanRenderer/VertexBuffer.hpp>
#include <VulkanRenderer/Utils.hpp>
#include "ContextImpl.hpp"

struct VertexBuffer::Impl
{
	Impl(Render::Context::Impl* context,
		 const void* vertices,
		 size_t vertices_length,
		 size_t vertex_memory_size);
	
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory memory;
	size_t length;
};
