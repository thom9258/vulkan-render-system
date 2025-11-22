#pragma once

#include "ShaderTexture.hpp"

#include <map>
#include <filesystem>

class TextureSamplerRef
{
public:
	TextureSamplerRef(std::uint64_t id);
	std::uint64_t id() const;

private:
	std::uint64_t m_id;
};


bool operator==(const TextureSamplerRef lhs, const TextureSamplerRef rhs) noexcept; 
bool operator<(const TextureSamplerRef lhs, const TextureSamplerRef rhs) noexcept; 

class TextureSamplerCache
{
public:
	struct TextureInfo
	{
		TextureSamplerReadOnly texture;
		InterpolationType interpolation;
		std::string name;
		std::optional<std::filesystem::path> path;
	};

	[[nodiscard]]
	auto add_texture(Render::Context* context,
					 InterpolationType interpolation,
					 std::string_view name,
					 std::filesystem::path path,
					 Texture2D&& texture)
		-> TextureSamplerRef;

	TextureInfo* get_texture(TextureSamplerRef ref);
	std::optional<TextureSamplerRef> get_ref_from_path(std::filesystem::path path);

private:        
	std::map<TextureSamplerRef, TextureInfo> m_cache;
	std::uint64_t m_id_counter{0};
};   
