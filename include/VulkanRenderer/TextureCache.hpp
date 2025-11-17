#pragma once

#include "ShaderTexture.hpp"

#include <map>
#include <filesystem>

class TextureSamplerRef
{
public:
	TextureSamplerRef(std::uint64_t id);

	bool operator==(TextureSamplerRef& other) noexcept; 
	bool operator<(TextureSamplerRef& other) noexcept; 

private:
	std::uint64_t m_id;
};

class TextureSamplerCache
{
public:
	struct TextureSamplerInfo
	{
		TextureSamplerReadOnly texture;
		std::string name;
		std::optional<std::filesystem::path> path;
	};

	TextureSamplerInfo& get_texture(TextureSamplerRef ref);

private:        
	std::map<TextureSamplerRef, TextureSamplerInfo> m_cache;
	std::uint64_t m_id_counter{0};
};   
