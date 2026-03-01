#pragma once

#include "Mesh.hpp"

#include <cstdint>
#include <map>

class AnimatedMeshRef {
public:
  AnimatedMeshRef(std::uint64_t id);
  std::uint64_t id() const;


bool operator==(const AnimatedMeshRef rhs) const noexcept;
bool operator<(const AnimatedMeshRef rhs) const noexcept;

private:
  std::uint64_t m_id;
};

class SimpleMeshRef {
public:
  SimpleMeshRef(std::uint64_t id);
  std::uint64_t id() const;


bool operator==(const SimpleMeshRef rhs) const noexcept;
bool operator<(const SimpleMeshRef rhs) const noexcept;

private:
  std::uint64_t m_id;
};


class MeshCache {
public:

  [[nodiscard]]
  auto add(Render::Context &context, TexturedMesh&& vertex_buffer)
      -> SimpleMeshRef;

  TexturedMesh *get(SimpleMeshRef ref);
  void remove(SimpleMeshRef ref);
	
  [[nodiscard]]
  auto add(Render::Context &context, AnimatedMesh&& vertex_buffer)
      -> AnimatedMeshRef;

  AnimatedMesh *get(AnimatedMeshRef ref);
  void remove(AnimatedMeshRef ref);


private:
  std::map<SimpleMeshRef, TexturedMesh> m_mesh_cache;
  std::uint64_t m_mesh_counter{0};

  std::map<AnimatedMeshRef, AnimatedMesh> m_animated_mesh_cache;
  std::uint64_t m_animated_mesh_counter{0};
};
