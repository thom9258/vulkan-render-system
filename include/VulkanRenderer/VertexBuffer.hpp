#pragma once

#include "Utils.hpp"
#include "Context.hpp"

//TODO MAKE THIS PIMPL
//TODO MAKE THIS TYPESAFE SO VERTEXBUFFERS CAN BE SPECIFIED FROM VERTEX TYPE

struct VertexBuffer
{
	vk::UniqueBuffer buffer;
	vk::UniqueDeviceMemory memory;
	size_t length;
};

auto create_vertex_buffer(Render::Context& context,
						  void* vertices,
						  size_t vertices_length,
						  size_t vertex_memory_size)
	-> VertexBuffer;

template<typename Vertex>
auto create_vertex_buffer(Render::Context& context,
						  std::vector<Vertex>& vertices)
	-> VertexBuffer
{
	return create_vertex_buffer(context,
								vertices.data(),
								vertices.size(),
								sizeof(vertices[0]));
}
