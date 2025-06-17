#pragma once

#include "Texture.hpp"

enum class InterpolationType {
	Linear,
	Point
};

struct TextureSamplerReadOnly
{
	struct Impl;
	std::unique_ptr<Impl> impl{nullptr};

	TextureSamplerReadOnly();
	~TextureSamplerReadOnly();

	TextureSamplerReadOnly(std::unique_ptr<Impl>&& impl);
	TextureSamplerReadOnly(const TextureSamplerReadOnly&) = delete;
	TextureSamplerReadOnly& operator=(const TextureSamplerReadOnly&) = delete;
	TextureSamplerReadOnly(TextureSamplerReadOnly&& texture) noexcept;
	TextureSamplerReadOnly& operator=(TextureSamplerReadOnly&& texture) noexcept;
};

auto make_shader_readonly(Render::Context* context,
						  InterpolationType interpolation,
						  Texture2D&& texture)
	-> TextureSamplerReadOnly;

auto make_shader_readonly(Render::Context* context,
						  InterpolationType interpolation)
	-> std::function<TextureSamplerReadOnly(Texture2D&&)>;

