#pragma once

#include "Context.hpp"

//TODO MAKE THIS TYPESAFE SO VERTEXBUFFERS CAN BE SPECIFIED FROM VERTEX TYPE

struct VertexBuffer
{
	VertexBuffer(Render::Context& context,
				 void* vertices,
				 size_t vertices_length,
				 size_t vertex_memory_size);
	
	template<typename Vertex>
	VertexBuffer(Render::Context& context,
				 std::vector<Vertex>& vertices)
		: VertexBuffer(context,
					   vertices.data(),
					   vertices.size(),
					   sizeof(vertices[0]))
	{
	}

	class Impl;
	std::unique_ptr<Impl> impl;
};
