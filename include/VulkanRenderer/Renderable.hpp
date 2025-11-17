#pragma once

#include "glm.hpp"
#include "TextureCache.hpp"
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
		TexturedMesh mesh;
		std::optional<TextureSamplerRef> ambient;
		std::optional<TextureSamplerRef> diffuse;
		std::optional<TextureSamplerRef> specular;
		std::optional<TextureSamplerRef> normal;
	};

	std::string name;
	glm::mat4 model;
	std::vector<MaterialMesh> meshes;
	std::vector<std::shared_ptr<RenderableNode>> children;
};

using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								BaseTextureRenderable,
								MaterialRenderable,
								RenderableNode>;
