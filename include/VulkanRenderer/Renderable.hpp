#pragma once

#include "Animation.hpp"
#include "Mesh.hpp"
#include "MeshCache.hpp"
#include "TextureSamplerCache.hpp"
#include "glm.hpp"

#include <concepts>
#include <map>
#include <optional>
#include <string>
#include <variant>

struct NormColorRenderable {
  std::optional<SimpleMeshRef> mesh;
  glm::mat4 model{glm::mat4(1.0f)};
};

struct WireframeRenderable {
  std::optional<SimpleMeshRef> mesh;
  glm::mat4 model{glm::mat4(1.0f)};
  glm::vec4 basecolor{1.0f, 0.0f, 0.0f, 1.0f};
};

struct MaterialRenderable {
  std::optional<SimpleMeshRef> mesh;
  std::optional<TextureSamplerRef> ambient;
  std::optional<TextureSamplerRef> diffuse;
  std::optional<TextureSamplerRef> specular;
  std::optional<TextureSamplerRef> normal;
  glm::mat4 model{glm::mat4(1.0f)};
    bool has_shadow{false};
};

struct AnimatedRenderable {
  std::optional<AnimatedMeshRef> mesh;
  std::optional<TextureSamplerRef> ambient;
  std::optional<TextureSamplerRef> diffuse;
  std::optional<TextureSamplerRef> specular;
  std::optional<TextureSamplerRef> normal;
  glm::mat4 model{glm::mat4(1.0f)};
    bool has_shadow{false};
    Animator *animator{nullptr};
};

struct RenderableNode {
  struct SimpleModel {
    std::optional<SimpleMeshRef> mesh;
    std::optional<TextureSamplerRef> ambient;
    std::optional<TextureSamplerRef> diffuse;
    std::optional<TextureSamplerRef> specular;
    std::optional<TextureSamplerRef> normal;
    bool has_shadow{false};
  };

  struct AnimatedModel {
    std::optional<AnimatedMeshRef> mesh;
    std::optional<TextureSamplerRef> ambient;
    std::optional<TextureSamplerRef> diffuse;
    std::optional<TextureSamplerRef> specular;
    std::optional<TextureSamplerRef> normal;
    bool has_shadow{false};
    Animator *animator{nullptr};
  };

  using Model = std::variant<SimpleModel, AnimatedModel>;

  std::optional<std::string> name;
  glm::mat4 model_matrix{glm::mat4(1.0f)};
  std::vector<Model> models;

  using NodePtr = std::shared_ptr<RenderableNode>;
  std::vector<NodePtr> children;
};

using RenderableNodePtr = RenderableNode::NodePtr;

template <typename TFn>
constexpr void foreach_node(TFn &&fn, RenderableNodePtr &node)
  requires std::invocable<TFn, RenderableNodePtr &>
{
  std::invoke(fn, node);
  for (RenderableNodePtr &child : node->children) {
    foreach_node(std::forward<decltype(fn)>(fn), child);
  }
}

using Renderable =
    std::variant<NormColorRenderable, WireframeRenderable, MaterialRenderable,
                 AnimatedRenderable, RenderableNodePtr>;

using ShadowRenderable = std::variant<MaterialRenderable, AnimatedRenderable>;
