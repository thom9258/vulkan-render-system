#include <VulkanRenderer/Texture.hpp>
#include "ContextImpl.hpp"

Texture2D::Texture2D(Texture2D&& rhs) noexcept
{
	std::swap(allocated, rhs.allocated);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	std::swap(layout, rhs.layout);
}

Texture2D& Texture2D::operator=(Texture2D&& rhs) noexcept
{
	std::swap(allocated, rhs.allocated);
	std::swap(extent, rhs.extent);
	std::swap(format, rhs.format);
	std::swap(layout, rhs.layout);
	return *this;
}

auto get_image(Texture2D& texture)
	-> vk::Image&
{
	return get_image(texture.allocated);
}

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
									   format,
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
						const vk::MemoryPropertyFlags propertyFlags,
						const LoadedBitmap2D& bitmap)
	-> Texture2D
{
	AllocatedMemory staging = create_staging_buffer(context->physical_device,
													context->device.get(),
													get_pixels(bitmap),
													bitmap.memory_size());
	const auto extent = vk::Extent3D{}
		.setWidth(bitmap.width)
		.setHeight(bitmap.height)
		.setDepth(1);

	const auto format = BitmapPixelFormatToVulkanFormat(bitmap.format);
	Texture2D texture = create_empty_general_texture(context,
													 format,
													 extent,
													 vk::ImageTiling::eOptimal,
													 propertyFlags);

	with_buffer_submit(context->device.get(),
					   context->commandpool.get(),
					   context->graphics_queue(),
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture.layout =
							   transition_image_for_color_override(get_image(texture),
																   commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												get_image(texture),
												texture.extent.width,
												texture.extent.height,
												commandbuffer);
					   });
	return texture;
}

auto copy_canvas_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						Canvas8bitRGBA& canvas)
	-> Texture2D
{
	AllocatedMemory staging = create_staging_buffer(context->physical_device,
													context->device.get(),
													get_pixels(canvas),
													canvas.memory_size());
	const auto extent = vk::Extent3D{}
		.setWidth(canvas.extent.width)
		.setHeight(canvas.extent.height)
		.setDepth(1);

	Texture2D texture = create_empty_general_texture(context,
													 vk::Format::eR8G8B8A8Srgb,
													 extent,
													 vk::ImageTiling::eOptimal,
													 propertyFlags);

	with_buffer_submit(context->device.get(),
					   context->commandpool.get(),
					   context->graphics_queue(),
					   [&] (vk::CommandBuffer& commandbuffer)
					   {
						   texture.layout =
							   transition_image_for_color_override(get_image(texture),
																   commandbuffer);

						   copy_buffer_to_image(staging.buffer.get(),
												get_image(texture),
												texture.extent.width,
												texture.extent.height,
												commandbuffer);
					   });
	return texture;
}


auto move_canvas_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						Canvas8bitRGBA&& canvas)
	-> Texture2D
{
	return copy_canvas_to_gpu(context,
							  propertyFlags,
							  canvas);
}

auto move_canvas_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags)
	-> std::function<Texture2D(Canvas8bitRGBA&& canvas)>
{
	return [=] (Canvas8bitRGBA&& canvas) {

		return move_canvas_to_gpu(context,
								  propertyFlags,
								  std::move(canvas));
	};
}

auto move_bitmap_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						LoadedBitmap2D&& bitmap)
	-> Texture2D
{
	return copy_bitmap_to_gpu(context,
							  propertyFlags,
							  bitmap);
}

auto move_bitmap_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags)
	-> std::function<Texture2D(LoadedBitmap2D&& bitmap)>
{
	return [=] (LoadedBitmap2D&& bitmap) {
		return move_bitmap_to_gpu(context,
								  propertyFlags,
								  std::move(bitmap));
	};
}

#if 0
//TODO: This is part of a pimpl wrapper for textures!
// also defined in geometrypass for now, TO BE DELETED
auto create_texture_view(vk::Device& device,
						 Texture2D& texture,
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
		.setImage(get_image(texture))
		.setFormat(texture.format)
		.setSubresourceRange(subresourceRange)
		.setViewType(vk::ImageViewType::e2D)
		.setComponents(componentMapping);

	return device.createImageViewUnique(imageViewCreateInfo);
}
#endif
