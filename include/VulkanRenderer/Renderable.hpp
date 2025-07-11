#pragma once

#include "glm.hpp"
#include "Mesh.hpp"
#include "ShaderTexture.hpp"

#include <variant>
#include <optional>

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

struct MaterialRenderable
{
	TexturedMesh* mesh;
	struct {
		TextureSamplerReadOnly* ambient;
		TextureSamplerReadOnly* diffuse;
		TextureSamplerReadOnly* specular;
		TextureSamplerReadOnly* normal;
	} texture;
	glm::mat4 model;
	bool casts_shadow;
};



using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								BaseTextureRenderable,
								MaterialRenderable>;
