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
	Mesh* mesh;
	glm::mat4 model;
};

struct WireframeRenderable
{
	Mesh* mesh;
	glm::mat4 model;
	glm::vec4 basecolor;
};

struct BaseTextureRenderable
{
	TexturedMesh* mesh;
	TextureSamplerReadOnly* texture;
	glm::mat4 model;
};

struct TextureMaterialPtrs
{
	TextureSamplerReadOnly* ambient{nullptr};
	TextureSamplerReadOnly* diffuse{nullptr};
	TextureSamplerReadOnly* specular{nullptr};
	TextureSamplerReadOnly* normal{nullptr};
};

struct MaterialRenderable
{
	TexturedMesh* mesh;
	TextureMaterialPtrs texture;
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
								BaseTextureRenderable,
								MaterialRenderable,
								RenderableNodePtr>;



struct NoRenderable {};
struct MaterialRenderable2
{
		//TexturedMeshRef vertices;
		//std::optional<IndicesRef> indices;
		std::optional<TextureSamplerRef> ambient;
		std::optional<TextureSamplerRef> diffuse;
		std::optional<TextureSamplerRef> specular;
		std::optional<TextureSamplerRef> normal;
};

using Renderable2 = std::variant < NoRenderable,
								   MaterialRenderable2>;

struct RenderableNode2
{
	glm::mat4 model_matrix;
	std::optional<std::string> name;
	Renderable2 renderable;
	std::vector<RenderableNode2> children;
};      
