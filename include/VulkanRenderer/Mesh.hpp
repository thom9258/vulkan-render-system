#pragma once
#ifndef _VULKANRENDERER_MESH_
#define _VULKANRENDERER_MESH_

#include <filesystem>
#include <variant>

#include "VertexPosNormColor.hpp"
#include "VertexPosNormColorUV.hpp"
#include "VertexBuffer.hpp"

struct Mesh
{
	VertexBuffer<VertexPosNormColor> vertexbuffer;
	std::string warning{};
};

struct MeshWithWarning
{
	Mesh mesh;
	std::string warning;
};

struct TexturedMesh
{
	VertexBuffer<VertexPosNormColorUV> vertexbuffer;
	std::string warning{};
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

auto load_obj(const std::filesystem::path& path,
			  const std::string& filename,
			  vk::PhysicalDevice& physical_device,
			  vk::Device& device)
	-> LoadMeshResult;

auto load_obj_with_texcoords(const std::filesystem::path& path,
							 const std::string& filename,
							 vk::PhysicalDevice& physical_device,
							 vk::Device& device)
	-> LoadTexturedMeshResult;

#endif //_VULKANRENDERER_MESH_
