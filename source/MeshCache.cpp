#include <VulkanRenderer/MeshCache.hpp>

#include <print>

SimpleMeshRef::SimpleMeshRef(std::uint64_t id) : m_id{id} {}

std::uint64_t SimpleMeshRef::id() const { return m_id; }

bool SimpleMeshRef::operator==(const SimpleMeshRef rhs) const noexcept {
  return id() == rhs.id();
}

bool SimpleMeshRef::operator<(const SimpleMeshRef rhs) const noexcept {
  return id() < rhs.id();
}

AnimatedMeshRef::AnimatedMeshRef(std::uint64_t id) : m_id{id} {}

std::uint64_t AnimatedMeshRef::id() const { return m_id; }

bool AnimatedMeshRef::operator==(const AnimatedMeshRef rhs) const noexcept {
  return id() == rhs.id();
}

bool AnimatedMeshRef::operator<(const AnimatedMeshRef rhs) const noexcept {
  return id() < rhs.id();
}

auto MeshCache::add(Render::Context &context, TexturedMesh &&mesh)
    -> SimpleMeshRef {
  SimpleMeshRef ref(m_mesh_counter);
  m_mesh_counter++;
  auto found = m_mesh_cache.find(ref);
  if (found != m_mesh_cache.end()) {
    throw std::runtime_error("fatal! texture ref could not be generated!");
  }

  m_mesh_cache.insert({ref, std::move(mesh)});
  return ref;
}

auto MeshCache::get(SimpleMeshRef ref) -> TexturedMesh * {
  auto found = m_mesh_cache.find(ref);
  if (found == m_mesh_cache.end()) {
    return nullptr;
  }

  return &(found->second);
}

void MeshCache::remove(SimpleMeshRef ref) {
  if (m_mesh_cache.find(ref) == m_mesh_cache.end()) {
    std::println("ERROR: deleted non-existing mesh");
    return;
  }

  m_mesh_cache.erase(ref);
}

auto MeshCache::add(Render::Context &context, AnimatedMesh &&mesh)
    -> AnimatedMeshRef {
  AnimatedMeshRef ref(m_animated_mesh_counter);
  m_animated_mesh_counter++;
  auto found = m_animated_mesh_cache.find(ref);
  if (found != m_animated_mesh_cache.end()) {
    throw std::runtime_error(
        "fatal! animated texture ref could not be generated!");
  }

  m_animated_mesh_cache.insert({ref, std::move(mesh)});
  return ref;
}

auto MeshCache::get(AnimatedMeshRef ref) -> AnimatedMesh * {
  auto found = m_animated_mesh_cache.find(ref);
  if (found == m_animated_mesh_cache.end()) {
    return nullptr;
  }

  return &(found->second);
}

void MeshCache::remove(AnimatedMeshRef ref) {
  if (m_animated_mesh_cache.find(ref) == m_animated_mesh_cache.end()) {
    std::println("ERROR: deleted non-existing mesh");
    return;
  }

  m_animated_mesh_cache.erase(ref);
}
