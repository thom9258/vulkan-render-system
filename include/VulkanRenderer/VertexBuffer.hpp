#pragma once

#include "Context.hpp"

//TODO MAKE THIS TYPESAFE SO VERTEXBUFFERS CAN BE SPECIFIED FROM VERTEX TYPE

struct VertexBuffer
{
	VertexBuffer(Render::Context& context,
				 const void* vertices,
				 size_t vertices_length,
				 size_t vertex_memory_size);
	
	template<typename Vertex>
	static VertexBuffer create(Render::Context& context,
							   const std::vector<Vertex>& vertices)
	{
		return VertexBuffer(context,
							vertices.data(),
							vertices.size(),
							sizeof(vertices[0]));
	}
	
	explicit VertexBuffer();
	~VertexBuffer();
	VertexBuffer(VertexBuffer&& rhs);
	VertexBuffer& operator=(VertexBuffer&& rhs);

	class Impl;
	std::unique_ptr<Impl> impl{ nullptr };
};
