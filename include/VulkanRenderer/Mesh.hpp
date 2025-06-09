#pragma once
#ifndef _VULKANRENDERER_MESH_
#define _VULKANRENDERER_MESH_

#include <filesystem>
#include <variant>
#include "VertexBuffer.hpp"

struct Mesh
{
	//VertexBuffer<VertexPosNormColor> vertexbuffer;
	VertexBuffer vertexbuffer;
};

struct MeshWithWarning
{
	Mesh mesh;
	std::string warning;
};

struct TexturedMesh
{
	//VertexBuffer<VertexPosNormColorUV> vertexbuffer;
	VertexBuffer vertexbuffer;
};

struct TexturedMeshWithWarning
{
	TexturedMesh mesh;
	std::string warning;
};

struct MeshLoadError
{
	std::string msg{};
};

struct MeshInvalidPath
{
	std::filesystem::path path;
	std::string filename;
};

using LoadMeshResult = std::variant<
	Mesh,
	MeshWithWarning,
	MeshLoadError,
	MeshInvalidPath>;

using LoadTexturedMeshResult = std::variant<
	TexturedMesh,
	TexturedMeshWithWarning,
	MeshLoadError,
	MeshInvalidPath>;

auto load_obj(Render::Context& context,
			  const std::filesystem::path& path,
			  const std::string& filename)
	-> LoadMeshResult;

auto load_obj_with_texcoords(Render::Context& context,
							 const std::filesystem::path& path,
							 const std::string& filename)
	-> LoadTexturedMeshResult;

#endif //_VULKANRENDERER_MESH_
