#include "TextureImpl.hpp"
#include "ContextImpl.hpp"

#include <memory>

#if 0
auto create_empty_texture(Render::Context::Impl* context,
						  const vk::Format format,
						  const vk::Extent3D extent,
						  const vk::ImageTiling tiling,
						  const vk::MemoryPropertyFlags propertyFlags,
						  const vk::ImageUsageFlags usageFlags)
	-> Texture2D
{
	Texture2D texture{};
	texture.format = format;
	texture.extent = extent;
	texture.layout = vk::ImageLayout::eUndefined;
	texture.allocated = allocate_image(context->physical_device,
									   context->device.get(),
									   extent,
									   textureformat_to_vkformat(format),
									   tiling,
									   propertyFlags,
									   usageFlags);
	return texture;
}

auto create_empty_general_texture(Render::Context::Impl* context,
								  const vk::Format format,
								  const vk::Extent3D extent,
								  const vk::ImageTiling tiling,
								  const vk::MemoryPropertyFlags propertyFlags)
	-> Texture2D
{
	return create_empty_texture(context,
								format,
								extent,
								tiling,
								propertyFlags,
								vk::ImageUsageFlagBits::eTransferDst
								| vk::ImageUsageFlagBits::eTransferSrc
								| vk::ImageUsageFlagBits::eSampled);
}

auto create_empty_rendertarget_texture(Render::Context::Impl* context,
									   const vk::Format format,
									   const vk::Extent3D extent,
									   const vk::ImageTiling tiling,
									   const vk::MemoryPropertyFlags propertyFlags)
	-> Texture2D
{
	return create_empty_texture(context,
								format,
								extent,
								tiling,
								propertyFlags,
								vk::ImageUsageFlagBits::eTransferDst
								| vk::ImageUsageFlagBits::eTransferSrc
								| vk::ImageUsageFlagBits::eSampled
								| vk::ImageUsageFlagBits::eColorAttachment);
}
#endif

auto vkformat_to_textureformat(vk::Format format)
	-> TextureFormat
{
	switch (format) {
	case vk::Format::eR8G8B8A8Srgb:    return TextureFormat::R8G8B8A8Srgb;
	case vk::Format::eR32G32B32Sfloat: return TextureFormat::R32G32B32Sfloat;
	case vk::Format::eR32G32Sfloat:    return TextureFormat::R32G32Sfloat;
	case vk::Format::eR32Sfloat:       return TextureFormat::R32Sfloat;
	};
	throw std::runtime_error(std::format("unhandled format {}", vk::to_string(format)));
};

auto textureformat_to_vkformat(TextureFormat format)
	-> vk::Format
{
	switch (format) {
	case TextureFormat::R8G8B8A8Srgb:    return vk::Format::eR8G8B8A8Srgb;
	case TextureFormat::R32G32B32Sfloat: return vk::Format::eR32G32B32Sfloat;
	case TextureFormat::R32G32Sfloat:    return vk::Format::eR32G32Sfloat;
	case TextureFormat::R32Sfloat:       return vk::Format::eR32Sfloat;
	};
	throw std::runtime_error(std::format("unhandled TextureFormat!"));
};

auto Texture2D::Impl::image()
	-> vk::Image&
{
	return get_image(allocated);
}

constexpr
auto BitmapPixelFormatToVulkanFormat(const BitmapPixelFormat format)
	noexcept -> vk::Format
{
	switch (format) {	
	case BitmapPixelFormat::RGBA:
		return vk::Format::eR8G8B8A8Srgb;
	};

	return vk::Format::eR8G8B8A8Srgb;
}


auto copy_bitmap_to_gpu(Render::Context::Impl* context,
						const LoadedBitmap2D& bitmap)
	-> Texture2D
{
	AllocatedMemory staging = create_staging_buffer(context->physical_device,
													context->device.get(),
													get_pixels(bitmap),
													bitmap.memory_size());

	const auto format = BitmapPixelFormatToVulkanFormat(bitmap.format);
	auto texture_impl = 
		std::make_unique<Texture2D::Impl>(GeneralTexture,
										  context,
										  U32Extent{
											  static_cast<uint32_t>(bitmap.width),
											  static_cast<uint32_t>(bitmap.height)},
										  vkformat_to_textureformat(format));

	with_buffer_submit(context->device.get(),
					   context->commandpool.get(),
					   context->graphics_queue(),
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture_impl->layout =
							   transition_image_for_color_override(texture_impl->image(),
																   commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												texture_impl->image(),
												texture_impl->extent.width,
												texture_impl->extent.height,
												commandbuffer);
					   });
	
	return Texture2D(std::move(texture_impl));
}

auto copy_canvas_to_gpu(Render::Context::Impl* context,
						Canvas8bitRGBA& canvas)
	-> Texture2D
{
	AllocatedMemory staging = create_staging_buffer(context->physical_device,
													context->device.get(),
													get_pixels(canvas),
													canvas.memory_size());

	auto texture_impl = 
		std::make_unique<Texture2D::Impl>(GeneralTexture,
										  context,
										  U32Extent{canvas.extent.width, canvas.extent.height},
										  TextureFormat::R8G8B8A8Srgb);

	with_buffer_submit(context->device.get(),
					   context->commandpool.get(),
					   context->graphics_queue(),
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture_impl->layout =
							   transition_image_for_color_override(texture_impl->image(),
																   commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												texture_impl->image(),
												texture_impl->extent.width,
												texture_impl->extent.height,
												commandbuffer);
					   });

	return Texture2D(std::move(texture_impl));
}


auto move_canvas_to_gpu(Render::Context::Impl* context,
						Canvas8bitRGBA&& canvas)
	-> Texture2D
{
	return copy_canvas_to_gpu(context, canvas);
}

auto move_canvas_to_gpu(Render::Context* context,
						Canvas8bitRGBA&& canvas)
	-> Texture2D
{
	return move_canvas_to_gpu(context->impl.get(), std::move(canvas));
}

auto move_canvas_to_gpu(Render::Context::Impl* context)
	-> std::function<Texture2D(Canvas8bitRGBA&& canvas)>
{
	return [=] (Canvas8bitRGBA&& canvas) {

		return move_canvas_to_gpu(context, std::move(canvas));
	};
}


auto move_canvas_to_gpu(Render::Context* context)
	-> std::function<Texture2D(Canvas8bitRGBA&& canvas)>
{
	return move_canvas_to_gpu(context->impl.get());
}

auto move_bitmap_to_gpu(Render::Context::Impl* context,
						LoadedBitmap2D&& bitmap)
	-> Texture2D
{
	return copy_bitmap_to_gpu(context, bitmap);
}

auto move_bitmap_to_gpu(Render::Context::Impl* context)
	-> std::function<Texture2D(LoadedBitmap2D&& bitmap)>
{
	return [=] (LoadedBitmap2D&& bitmap) {
		return move_bitmap_to_gpu(context, std::move(bitmap));
	};
}

[[nodiscard]]
auto move_bitmap_to_gpu(Render::Context* context,
						LoadedBitmap2D&& bitmap)
	-> Texture2D
{
	return move_bitmap_to_gpu(context->impl.get(), std::move(bitmap));
}


auto move_bitmap_to_gpu(Render::Context* context)
	-> std::function<Texture2D(LoadedBitmap2D&& bitmap)>
{
	return move_bitmap_to_gpu(context->impl.get());
}

//TODO: This is part of a pimpl wrapper for textures!
// also defined in geometrypass for now, TO BE DELETED
auto Texture2D::Impl::create_view(Render::Context::Impl* context,
								  const vk::ImageAspectFlags aspect)
	-> vk::UniqueImageView
{
	const auto subresourceRange = vk::ImageSubresourceRange{}
		.setAspectMask(aspect)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	const auto componentMapping = vk::ComponentMapping{}
		.setR(vk::ComponentSwizzle::eIdentity)		
		.setG(vk::ComponentSwizzle::eIdentity)
		.setB(vk::ComponentSwizzle::eIdentity)
		.setA(vk::ComponentSwizzle::eIdentity);

	const auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
		.setImage(image())
		.setFormat(format)
		.setSubresourceRange(subresourceRange)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(componentMapping);

	return context->device->createImageViewUnique(imageViewCreateInfo);
}

Texture2D::Impl::Impl(DepthBufferTextureType,
					  Render::Context::Impl* context,
					  const U32Extent extent_)
	: extent(vk::Extent3D(extent_.w, extent_.h, 1))
	, format(vk::Format::eD32Sfloat)
	, layout(vk::ImageLayout::eUndefined)
{
	const auto usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	allocated = allocate_image(context->physical_device,
							   context->device.get(),
							   extent,
							   format,
							   vk::ImageTiling::eOptimal,
							   vk::MemoryPropertyFlagBits::eDeviceLocal,
							   usage);
}

Texture2D::Impl::Impl(RenderTargetTextureType,
					  Render::Context::Impl* context,
					  const U32Extent extent_,
					  const TextureFormat format)
	: extent(vk::Extent3D(extent_.w, extent_.h, 1))
	, format(textureformat_to_vkformat(format))
	, layout(vk::ImageLayout::eUndefined)
{
	const auto usage = 
		vk::ImageUsageFlagBits::eTransferDst
		| vk::ImageUsageFlagBits::eTransferSrc
		| vk::ImageUsageFlagBits::eSampled
		| vk::ImageUsageFlagBits::eColorAttachment;

	allocated = allocate_image(context->physical_device,
							   context->device.get(),
							   extent,
							   textureformat_to_vkformat(format),
							   vk::ImageTiling::eOptimal,
							   vk::MemoryPropertyFlagBits::eDeviceLocal,
							   usage);
}

Texture2D::Impl::Impl(Impl&& rhs) noexcept
{
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	std::swap(layout, rhs.layout);
	std::swap(allocated, rhs.allocated);
}

auto Texture2D::Impl::operator=(Impl&& rhs) 
	noexcept -> Impl&
{
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	std::swap(layout, rhs.layout);
	std::swap(allocated, rhs.allocated);
	return *this;
}

Texture2D::Impl::Impl(GeneralTextureType,
					  Render::Context::Impl* context,
					  const U32Extent extent_,
					  const TextureFormat format)
	: extent(vk::Extent3D(extent_.w, extent_.h, 1))
	, format(textureformat_to_vkformat(format))
	, layout(vk::ImageLayout::eUndefined)
{
	const auto usage = 
		vk::ImageUsageFlagBits::eTransferDst
		| vk::ImageUsageFlagBits::eTransferSrc
		| vk::ImageUsageFlagBits::eSampled;

	allocated = allocate_image(context->physical_device,
							   context->device.get(),
							   extent,
							   textureformat_to_vkformat(format),
							   vk::ImageTiling::eOptimal,
							   vk::MemoryPropertyFlagBits::eDeviceLocal,
							   usage);
}

Texture2D::Texture2D(GeneralTextureType, 
					   Render::Context* context,
					   const U32Extent extent,
					   const TextureFormat format)
	: impl(std::make_unique<Impl>(GeneralTexture,
								  context->impl.get(),
								  extent,
								  format))
{
}

Texture2D::Texture2D(DepthBufferTextureType, 
					   Render::Context* context,
					   const U32Extent extent)
	: impl(std::make_unique<Impl>(DepthBufferTexture,
								  context->impl.get(),
								  extent))
{
}

Texture2D::Texture2D(RenderTargetTextureType, 
					   Render::Context* context,
					   const U32Extent extent,
					   const TextureFormat format)
	: impl(std::make_unique<Impl>(RenderTargetTexture,
								  context->impl.get(),
								  extent,
								  format))
{
}

Texture2D::Impl::~Impl()
{
}

auto Texture2D::extent()
		const noexcept -> U32Extent
{
	return U32Extent{impl->extent.width, impl->extent.height};
}

auto Texture2D::format()
		const noexcept -> TextureFormat
{
	return vkformat_to_textureformat(impl->format);
}

Texture2D::Texture2D(Texture2D&& rhs) noexcept
{
	std::swap(impl, rhs.impl);
}

auto Texture2D::operator=(Texture2D&& rhs)
	noexcept -> Texture2D&
{
	std::swap(impl, rhs.impl);
	return *this;
}

Texture2D::Texture2D()
	: impl(nullptr)
{
}

Texture2D::Texture2D(std::unique_ptr<Impl>&& impl)
	: impl(std::move(impl))
{
}

Texture2D::~Texture2D()
{
}
