#include <VulkanRenderer/ShaderTexture.hpp>

#include "ContextImpl.hpp"
#include "TextureImpl.hpp"

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

auto get_image(TextureSamplerReadOnly& texture)
	-> vk::Image&
{
	return texture.allocated.image.get();
}

auto get_image_view(TextureSamplerReadOnly& texture)
	-> vk::ImageView&
{
	return texture.view.get();
}

auto get_sampler(TextureSamplerReadOnly& texture)
	-> vk::Sampler&
{
	return texture.sampler.get();
}

auto make_shader_readonly(Render::Context::Impl* context,
						  InterpolationType interpolation,
						  Texture2D&& texture)
	-> TextureSamplerReadOnly
{
	with_buffer_submit(context->device.get(),
					   context->commandpool.get(),
					   context->graphics_queue(),
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   auto range = vk::ImageSubresourceRange{}
							   .setAspectMask(vk::ImageAspectFlagBits::eColor)
							   .setBaseMipLevel(0)
							   .setLevelCount(1)
							   .setBaseArrayLayer(0)
							   .setLayerCount(1);
						   
						   auto barrier = vk::ImageMemoryBarrier{}
							   .setOldLayout(texture.impl->layout)
							   .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
							   .setImage(texture.impl->image())
							   .setSubresourceRange(range)
							   .setSrcAccessMask(vk::AccessFlags())
							   .setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
						   
						   commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
														 vk::PipelineStageFlagBits::eTransfer,
														 vk::DependencyFlags(),
														 nullptr,
														 nullptr,
														 barrier);
						   texture.impl->layout = vk::ImageLayout::eShaderReadOnlyOptimal;
					   });

	TextureSamplerReadOnly shader_texture{};
	std::swap(shader_texture.allocated, texture.impl->allocated);
	shader_texture.format = texture.impl->format;
	
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
	shader_texture.view = context->device.get().createImageViewUnique(view_info);


	const auto properties = context->physical_device.getProperties();
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
	shader_texture.sampler = context->device.get().createSamplerUnique(sampler_info);
	return shader_texture;
}

auto make_shader_readonly(Render::Context* context,
						  InterpolationType interpolation,
						  Texture2D&& texture)
	-> TextureSamplerReadOnly
{
	return make_shader_readonly(context->impl.get(),
								interpolation,
								std::forward<Texture2D>(texture));
}

auto make_shader_readonly(Render::Context::Impl* context,
						  InterpolationType interpolation)
	-> std::function<TextureSamplerReadOnly(Texture2D&&)>
{
	return [=] (Texture2D&& texture) {
		return make_shader_readonly(context,
									interpolation,
									std::move(texture));
	};
}

auto make_shader_readonly(Render::Context* context,
						  InterpolationType interpolation)
	-> std::function<TextureSamplerReadOnly(Texture2D&&)>
{
	return make_shader_readonly(context->impl.get(), interpolation);
}

