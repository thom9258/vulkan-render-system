#pragma once

#include <filesystem>
#include <variant>
#include "VertexBuffer.hpp"

struct TexturedMesh
{
	//TOOD: enforce typed vertexbuffers to ensure compile errors on bugs
	//VertexBuffer<VertexPosNormColorUV> vertexbuffer;
	VertexBuffer vertexbuffer;
};

struct AnimatedMesh
{
	//TOOD: enforce typed vertexbuffers to ensure compile errors on bugs
	//VertexBuffer<VertexAnimatedPosNormColorUV> vertexbuffer;
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
