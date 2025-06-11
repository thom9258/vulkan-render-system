#pragma once

#include "Texture.hpp"

enum class InterpolationType {
	Linear,
	Point
};

struct TextureSamplerReadOnly
{
	explicit TextureSamplerReadOnly() = default;
	~TextureSamplerReadOnly() = default;

	TextureSamplerReadOnly(const TextureSamplerReadOnly&) = delete;
	TextureSamplerReadOnly& operator=(const TextureSamplerReadOnly&) = delete;

	TextureSamplerReadOnly(TextureSamplerReadOnly&& texture) noexcept;
	TextureSamplerReadOnly& operator=(TextureSamplerReadOnly&& texture) noexcept;

	AllocatedImage allocated;
	vk::UniqueImageView view;
	vk::UniqueSampler sampler;
	vk::Format format;
};


auto get_image(TextureSamplerReadOnly& texture)
	-> vk::Image&;

auto get_image_view(TextureSamplerReadOnly& texture)
	-> vk::ImageView&;

auto get_sampler(TextureSamplerReadOnly& texture)
	-> vk::Sampler&;


auto make_shader_readonly(Render::Context* context,
						  InterpolationType interpolation,
						  Texture2D&& texture)
	-> TextureSamplerReadOnly;

auto make_shader_readonly(Render::Context::Impl* context,
						  InterpolationType interpolation,
						  Texture2D&& texture)
	-> TextureSamplerReadOnly;

auto make_shader_readonly(Render::Context* context,
						  InterpolationType interpolation)
	-> std::function<TextureSamplerReadOnly(Texture2D&&)>;

auto make_shader_readonly(Render::Context::Impl* context,
						  InterpolationType interpolation)
	-> std::function<TextureSamplerReadOnly(Texture2D&&)>;
