#pragma once

#include "glm.hpp"
#include "TextureSamplerCache.hpp"
#include "MeshCache.hpp"
#include "Mesh.hpp"

#include <variant>
#include <map>
#include <optional>
#include <string>

struct NormColorRenderable
{
	std::optional<SimpleMeshRef> mesh;
	glm::mat4 model;
};

struct WireframeRenderable
{
	std::optional<SimpleMeshRef> mesh;
	glm::mat4 model;
	glm::vec4 basecolor;
};

struct MaterialRenderable
{
	std::optional<SimpleMeshRef> mesh;
	std::optional<TextureSamplerRef> ambient;
	std::optional<TextureSamplerRef> diffuse;
	std::optional<TextureSamplerRef> specular;
	std::optional<TextureSamplerRef> normal;
	glm::mat4 model;
	bool has_shadow;
};

struct AnimatedRenderable
{
	std::optional<AnimatedMeshRef> mesh;
	std::optional<TextureSamplerRef> ambient;
	std::optional<TextureSamplerRef> diffuse;
	std::optional<TextureSamplerRef> specular;
	std::optional<TextureSamplerRef> normal;
	glm::mat4 model;
	bool has_shadow;
};

struct RenderableNode
{
	struct SimpleModel
	{
		std::optional<SimpleMeshRef> mesh;
		std::optional<TextureSamplerRef> ambient;
		std::optional<TextureSamplerRef> diffuse;
		std::optional<TextureSamplerRef> specular;
		std::optional<TextureSamplerRef> normal;
		bool has_shadow;
	};
	
	struct AnimatedModel
	{
		std::optional<AnimatedMeshRef> mesh;
		std::optional<TextureSamplerRef> ambient;
		std::optional<TextureSamplerRef> diffuse;
		std::optional<TextureSamplerRef> specular;
		std::optional<TextureSamplerRef> normal;
		bool has_shadow;
	};

	using Model = std::variant<SimpleModel, AnimatedModel>;

	std::optional<std::string> name;
	glm::mat4 model_matrix;
	std::vector<Model> models;
	
	using NodePtr = std::shared_ptr<RenderableNode>;
	std::vector<NodePtr> children;
};

using RenderableNodePtr = RenderableNode::NodePtr;

using Renderable = std::variant<NormColorRenderable,
								WireframeRenderable,
								MaterialRenderable,
								AnimatedRenderable,
								RenderableNodePtr>;
