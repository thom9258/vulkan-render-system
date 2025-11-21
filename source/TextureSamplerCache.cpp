#include <VulkanRenderer/TextureSamplerCache.hpp>


TextureSamplerRef::TextureSamplerRef(std::uint64_t id)
	: m_id{id}
{
}


std::uint64_t TextureSamplerRef::id() const
{
	return m_id;
}

bool operator==(const TextureSamplerRef lhs, const TextureSamplerRef rhs) noexcept
{
	return lhs.id() == rhs.id();
}

bool operator<(const TextureSamplerRef lhs, const TextureSamplerRef rhs) noexcept
{
	return lhs.id() < rhs.id();
}

auto TextureSamplerCache::add_texture(Render::Context* context,
									  InterpolationType interpolation,
									  std::string_view name,
									  std::filesystem::path path,
									  Texture2D&& texture)
	-> TextureSamplerRef
{
	TextureSamplerRef ref(m_id_counter);
	m_id_counter++;
	auto found = m_cache.find(ref);
	if (found != m_cache.end()) {
		throw std::runtime_error("fatal! texture ref could not be generated!");
	}

        m_cache.insert({ref, TextureSamplerCache::TextureInfo{
                                 make_shader_readonly(context, interpolation,
                                                      std::move(texture)),
						interpolation, std::string(name), path}});
		
	return ref;
}

auto TextureSamplerCache::get_texture(TextureSamplerRef ref) 
	-> TextureSamplerCache::TextureInfo*
{
 	auto found = m_cache.find(ref);
	if (found == m_cache.end()) {
		return nullptr;
	}
	
	return &(found->second);
}    
