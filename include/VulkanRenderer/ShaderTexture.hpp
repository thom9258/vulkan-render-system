#pragma once

#include <vulkan/vulkan.hpp>

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

TextureSamplerReadOnly::TextureSamplerReadOnly(TextureSamplerReadOnly&& rhs) noexcept
{
	std::swap(allocated, rhs.allocated);
	std::swap(view, rhs.view);
	std::swap(sampler, rhs.sampler);
	std::swap(format, rhs.format);
}

TextureSamplerReadOnly& TextureSamplerReadOnly::operator=(TextureSamplerReadOnly&& rhs) noexcept
{
	std::swap(allocated, rhs.allocated);
	std::swap(view, rhs.view);
	std::swap(sampler, rhs.sampler);
	std::swap(format, rhs.format);
	return *this;
}

vk::Image&
get_image(TextureSamplerReadOnly& texture)
{
	return get_image(texture.allocated);
}

vk::ImageView&
get_image_view(TextureSamplerReadOnly& texture)
{
	return texture.view.get();
}

vk::Sampler&
get_sampler(TextureSamplerReadOnly& texture)
{
	return texture.sampler.get();
}

TextureSamplerReadOnly
make_shader_readonly(vk::PhysicalDevice physical_device,
					 vk::Device device,
					 vk::Queue queue,
					 vk::CommandPool command_pool,
					 InterpolationType interpolation,
					 Texture2D&& texture)
{
	with_buffer_submit(device, command_pool, queue,
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   auto range = vk::ImageSubresourceRange{}
							   .setAspectMask(vk::ImageAspectFlagBits::eColor)
							   .setBaseMipLevel(0)
							   .setLevelCount(1)
							   .setBaseArrayLayer(0)
							   .setLayerCount(1);
						   
						   auto barrier = vk::ImageMemoryBarrier{}
							   .setOldLayout(texture.layout)
							   .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
							   .setImage(get_image(texture))
							   .setSubresourceRange(range)
							   .setSrcAccessMask(vk::AccessFlags())
							   .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
						   
						   commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
														 vk::PipelineStageFlagBits::eTransfer,
														 vk::DependencyFlags(),
														 nullptr,
														 nullptr,
														 barrier);
						   texture.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
					   });

	TextureSamplerReadOnly shader_texture{};
	std::swap(shader_texture.allocated, texture.allocated);
	shader_texture.format = texture.format;
	
	const auto view_subresource_range = vk::ImageSubresourceRange{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);
	const auto view_info = vk::ImageViewCreateInfo{}
		.setImage(get_image(shader_texture))
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(shader_texture.format)
		.setSubresourceRange(view_subresource_range);
	shader_texture.view = device.createImageViewUnique(view_info);


	const auto properties = physical_device.getProperties();
	const auto has_anisotropy = true; // TODO: set this from device
	const auto max_anisotropy = has_anisotropy
		? std::min(4.0f, properties.limits.maxSamplerAnisotropy)
		: 1.0f;
	
	vk::Filter filter = vk::Filter::eLinear;
	if (interpolation == InterpolationType::Point)
		filter = vk::Filter::eNearest;

	const auto sampler_info = vk::SamplerCreateInfo{}
		//TODO Filter should be changeable!
		.setMagFilter(filter)
		.setMinFilter(filter)
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(has_anisotropy)
		.setMaxAnisotropy(max_anisotropy)
		.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
		.setUnnormalizedCoordinates(false)
		.setCompareEnable(false)
		.setCompareOp(vk::CompareOp::eAlways)
		.setMipmapMode(vk::SamplerMipmapMode::eLinear)
		.setMipLodBias(0.0f)
		.setMinLod(0.0f)
		.setMaxLod(0.0f);
	shader_texture.sampler = device.createSamplerUnique(sampler_info);
	return shader_texture;
}

decltype(auto)
make_shader_readonly(vk::PhysicalDevice physical_device,
					 vk::Device device,
					 vk::Queue queue,
					 InterpolationType interpolation,
					 vk::CommandPool command_pool)
{
	return [=] (Texture2D&& texture) {
		return make_shader_readonly(physical_device,
									device,
									queue,
									command_pool,
									interpolation,
									std::move(texture));
	};
}
