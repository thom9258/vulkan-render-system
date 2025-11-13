#pragma once

#include "glm.hpp"
#include "Mesh.hpp"
#include "ShaderTexture.hpp"

#include <variant>
#include <optional>
#include <map>
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

struct TextureMaterial
{
	TextureSamplerReadOnly ambient;
	TextureSamplerReadOnly diffuse;
	TextureSamplerReadOnly specular;
	TextureSamplerReadOnly normal;
};

struct RenderableTree
{
	struct Node
	{
		struct MaterialMesh
		{
			TexturedMesh mesh;
			std::string material_name;
		};

		std::string name;
		glm::mat4 model;
		std::vector<MaterialMesh> meshes;
		std::vector<std::shared_ptr<Node>> children;
	};
	using MaterialMap = std::map<std::string, std::shared_ptr<TextureMaterial>>;
	MaterialMap materials;
	std::shared_ptr<Node> root{nullptr};
};

using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								BaseTextureRenderable,
								MaterialRenderable,
								RenderableTree>;
