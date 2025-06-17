#pragma once

#include <VulkanRenderer/ShaderTexture.hpp>

#include "ContextImpl.hpp"
#include "TextureImpl.hpp"

struct TextureSamplerReadOnly::Impl
{
	explicit Impl() = default;
	~Impl() = default;

	auto image()
		-> vk::Image&;

	AllocatedImage allocated;
	vk::UniqueImageView view;
	vk::UniqueSampler sampler;
	vk::Format format;
};

auto make_shader_readonly(Render::Context::Impl* context,
						  InterpolationType interpolation,
						  Texture2D&& texture)
	-> TextureSamplerReadOnly;

auto make_shader_readonly(Render::Context::Impl* context,
						  InterpolationType interpolation)
	-> std::function<TextureSamplerReadOnly(Texture2D&&)>;
