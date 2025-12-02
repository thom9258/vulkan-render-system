#pragma once

#include <filesystem>
#include <variant>
#include "VertexBuffer.hpp"

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

using LoadTexturedMeshResult = std::variant<
	TexturedMesh,
	TexturedMeshWithWarning,
	MeshLoadError,
	MeshInvalidPath>;

auto load_obj_with_texcoords(Render::Context& context,
							 const std::filesystem::path& path,
							 const std::string& filename)
	-> LoadTexturedMeshResult;
