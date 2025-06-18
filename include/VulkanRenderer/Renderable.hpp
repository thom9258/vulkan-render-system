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
	std::optional<bool> casts_shadows;
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
	std::optional<bool> casts_shadows;
};

using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								BaseTextureRenderable>;
