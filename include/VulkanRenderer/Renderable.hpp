#pragma once

#include "glm.hpp"
#include "Context.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "ShaderTexture.hpp"

#include <variant>

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

using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								BaseTextureRenderable>;
