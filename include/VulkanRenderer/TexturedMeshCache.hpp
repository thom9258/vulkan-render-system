#pragma once

#include "Mesh.hpp"

#include <cstdint>
#include <map>

class TexturedMeshRef {
public:
  TexturedMeshRef(std::uint64_t id);
  std::uint64_t id() const;


bool operator==(const TexturedMeshRef rhs) const noexcept;
bool operator<(const TexturedMeshRef rhs) const noexcept;

private:
  std::uint64_t m_id;
};


class TexturedMeshCache {
public:

  [[nodiscard]]
  auto add(Render::Context &context, TexturedMesh&& vertex_buffer)
      -> TexturedMeshRef;

  TexturedMesh *get(TexturedMeshRef ref);

private:
  std::map<TexturedMeshRef, TexturedMesh> m_cache;
  std::uint64_t m_id_counter{0};
};
