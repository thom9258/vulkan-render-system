#pragma once

#include "glm.hpp"
#include "TextureSamplerCache.hpp"
#include "TexturedMeshCache.hpp"
#include "Mesh.hpp"

#include <variant>
#include <map>
#include <optional>
#include <string>

struct NormColorRenderable
{
	std::optional<TexturedMeshRef> mesh;
	glm::mat4 model;
};

struct WireframeRenderable
{
	std::optional<TexturedMeshRef> mesh;
	glm::mat4 model;
	glm::vec4 basecolor;
};

struct MaterialRenderable
{
	std::optional<TexturedMeshRef> mesh;
	std::optional<TextureSamplerRef> ambient;
	std::optional<TextureSamplerRef> diffuse;
	std::optional<TextureSamplerRef> specular;
	std::optional<TextureSamplerRef> normal;
	glm::mat4 model;
	bool has_shadow;
};

struct RenderableNode
{
	struct MaterialMesh
	{
		std::optional<TexturedMeshRef> mesh;
		std::optional<TextureSamplerRef> ambient;
		std::optional<TextureSamplerRef> diffuse;
		std::optional<TextureSamplerRef> specular;
		std::optional<TextureSamplerRef> normal;
		bool has_shadow;
	};

	std::optional<std::string> name;
	glm::mat4 model;
	std::vector<MaterialMesh> meshes;
	
	using NodePtr = std::shared_ptr<RenderableNode>;
	std::vector<NodePtr> children;
};

using RenderableNodePtr = RenderableNode::NodePtr;

using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								MaterialRenderable,
								RenderableNodePtr>;
