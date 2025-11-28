#include <VulkanRenderer/TexturedMeshCache.hpp>

TexturedMeshRef::TexturedMeshRef(std::uint64_t id)
	: m_id{id}
{
}


std::uint64_t TexturedMeshRef::id() const
{
	return m_id;
}

bool TexturedMeshRef::operator==(const TexturedMeshRef rhs) const noexcept
{
	return id() == rhs.id();
}

bool TexturedMeshRef::operator<(const TexturedMeshRef rhs) const noexcept
{
	return id() < rhs.id();
}

auto TexturedMeshCache::add(Render::Context& context,
							TexturedMesh&& vertex_buffer)
	-> TexturedMeshRef
{
	TexturedMeshRef ref(m_id_counter);
	m_id_counter++;
	auto found = m_cache.find(ref);
	if (found != m_cache.end()) {
		throw std::runtime_error("fatal! texture ref could not be generated!");
	}

	m_cache.insert({ref, std::move(vertex_buffer)});
	return ref;
}

auto TexturedMeshCache::get(TexturedMeshRef ref) 
	-> TexturedMesh*
{
 	auto found = m_cache.find(ref);
	if (found == m_cache.end()) {
		return nullptr;
	}
	
	return &(found->second);
}
