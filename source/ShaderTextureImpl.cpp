#include "ShaderTextureImpl.hpp"

auto TextureSamplerReadOnly::Impl::image()
	-> vk::Image&
{
	return allocated.image.get();
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

	auto sampler_impl = std::make_unique<TextureSamplerReadOnly::Impl>();
	std::swap(sampler_impl->allocated, texture.impl->allocated);
	sampler_impl->format = texture.impl->format;
	
	const auto view_subresource_range = vk::ImageSubresourceRange{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);
	const auto view_info = vk::ImageViewCreateInfo{}
		.setImage(sampler_impl->image())
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(sampler_impl->format)
		.setSubresourceRange(view_subresource_range);
	sampler_impl->view = context->device.get().createImageViewUnique(view_info);

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
	sampler_impl->sampler = context->device.get().createSamplerUnique(sampler_info);
	return TextureSamplerReadOnly(std::move(sampler_impl));
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

TextureSamplerReadOnly::TextureSamplerReadOnly(std::unique_ptr<TextureSamplerReadOnly::Impl>&& impl)
	: impl(std::move(impl))
{
}

TextureSamplerReadOnly::TextureSamplerReadOnly()
{
}

TextureSamplerReadOnly::~TextureSamplerReadOnly()
{
}

TextureSamplerReadOnly::TextureSamplerReadOnly(TextureSamplerReadOnly&& rhs) noexcept
{
	std::swap(impl, rhs.impl);
}

TextureSamplerReadOnly& TextureSamplerReadOnly::operator=(TextureSamplerReadOnly&& rhs) noexcept
{
	std::swap(impl, rhs.impl);
	return *this;
}
