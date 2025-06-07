#pragma once
#ifndef _VULKANRENDERER_TEXTURE_
#define _VULKANRENDERER_TEXTURE_

#include <vulkan/vulkan.hpp>

#include "Utils.hpp"
#include "Bitmap.hpp"
#include "Canvas.hpp"

#include <iostream>
	
struct Texture2D
{
	explicit Texture2D() = default;
	~Texture2D() = default;

	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(const Texture2D&) = delete;

	Texture2D(Texture2D&& texture) noexcept;
	Texture2D& operator=(Texture2D&& texture) noexcept;

	AllocatedImage allocated;
	vk::Extent3D extent;
	vk::Format format;
	vk::ImageLayout layout;
};

template <typename F>
decltype(auto) operator|(Texture2D&& texture, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(texture));
}


auto get_image(Texture2D& texture)
	-> vk::Image&;


auto create_empty_texture(vk::PhysicalDevice& physical_device,
						  vk::Device& device,
						  const vk::Format format,
						  const vk::Extent3D extent,
						  const vk::ImageTiling tiling,
						  const vk::MemoryPropertyFlags propertyFlags,
						  const vk::ImageUsageFlags usageFlags)
	-> Texture2D;


auto create_empty_general_texture(vk::PhysicalDevice& physical_device,
								  vk::Device& device,
								  const vk::Format format,
								  const vk::Extent3D extent,
								  const vk::ImageTiling tiling,
								  const vk::MemoryPropertyFlags propertyFlags)
	-> Texture2D;



auto create_empty_rendertarget_texture(vk::PhysicalDevice& physical_device,
								  vk::Device& device,
								  const vk::Format format,
								  const vk::Extent3D extent,
								  const vk::ImageTiling tiling,
								  const vk::MemoryPropertyFlags propertyFlags)
	-> Texture2D;

constexpr
auto BitmapPixelFormatToVulkanFormat(const BitmapPixelFormat format) 
	noexcept -> vk::Format;


auto copy_bitmap_to_gpu(vk::PhysicalDevice& physical_device,
						vk::Device& device,
						vk::CommandPool& command_pool,
						vk::Queue& queue,
						const vk::MemoryPropertyFlags propertyFlags,
						const LoadedBitmap2D& bitmap)
	-> Texture2D;


#if 0
Texture2D
copy_to_gpu_as_shader_readonly(vk::PhysicalDevice& physical_device,
							   vk::Device& device,
							   vk::CommandPool& command_pool,
							   vk::Queue& queue,
							   const LoadedBitmap2D& bitmap)
{
	AllocatedMemory staging = create_staging_buffer(physical_device,
													device,
													get_pixels(bitmap),
													bitmap.memory_size());
	const auto extent = vk::Extent3D{}
		.setWidth(bitmap.width)
		.setHeight(bitmap.height)
		.setDepth(1);

	const auto format = BitmapPixelFormatToVulkanFormat(bitmap.format);
	Texture2D texture = create_empty_general_texture(physical_device,
													 device,
													 format,
													 extent,
													 vk::ImageTiling::eOptimal,
													 vk::MemoryPropertyFlagBits::eDeviceLocal);

	with_buffer_submit(device, command_pool, queue,
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
	return texture;
}


Texture2D
copy_to_gpu_as_shader_readonly(vk::PhysicalDevice& physical_device,
							   vk::Device& device,
							   vk::CommandPool& command_pool,
							   vk::Queue& queue,
							   const Canvas8bitRGBA& canvas)
{
	AllocatedMemory staging = create_staging_buffer(physical_device,
													device,
													get_pixels(canvas),
													canvas.memory_size());
	const auto extent = vk::Extent3D{}
		.setWidth(canvas.extent.width)
		.setHeight(canvas.extent.height)
		.setDepth(1);

	Texture2D texture = create_empty_general_texture(physical_device,
													 device,
													 vk::Format::eR8G8B8A8Srgb,
													 extent,
													 vk::ImageTiling::eOptimal,
													 vk::MemoryPropertyFlagBits::eDeviceLocal);

	with_buffer_submit(device, command_pool, queue,
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

	return texture;
}
#endif


auto copy_canvas_to_gpu(vk::PhysicalDevice& physical_device,
						vk::Device& device,
						vk::CommandPool& command_pool,
						vk::Queue& queue,
						const vk::MemoryPropertyFlags propertyFlags,
						Canvas8bitRGBA& canvas)
	-> Texture2D;

// @note move_to_gpu is just a copy_to_gpu, but it consumes a canvas 
//       as an rvalue-reference, thus the canvas is destroyed after 
//       the operation is completed.
//       this allows move_to_gpu to be used as a pipe.
auto move_canvas_to_gpu(vk::PhysicalDevice physical_device,
						vk::Device device,
						vk::CommandPool command_pool,
						vk::Queue queue,
						const vk::MemoryPropertyFlags propertyFlags,
						Canvas8bitRGBA&& canvas)
	-> Texture2D;

auto move_canvas_to_gpu(vk::PhysicalDevice physical_device,
						vk::Device device,
						vk::CommandPool command_pool,
						vk::Queue queue,
						const vk::MemoryPropertyFlags propertyFlags)
	-> std::function<Texture2D(Canvas8bitRGBA&& canvas)>;

auto move_bitmap_to_gpu(vk::PhysicalDevice physical_device,
						vk::Device device,
						vk::CommandPool command_pool,
						vk::Queue queue,
						const vk::MemoryPropertyFlags propertyFlags,
						LoadedBitmap2D&& bitmap)
	-> Texture2D;

auto move_bitmap_to_gpu(vk::PhysicalDevice physical_device,
						vk::Device device,
						vk::CommandPool command_pool,
						vk::Queue queue,
						const vk::MemoryPropertyFlags propertyFlags)
	-> std::function<Texture2D(LoadedBitmap2D&& bitmap)>;


auto create_texture_view(vk::Device& device,
						 Texture2D& texture,
						 const vk::ImageAspectFlags aspect)
	-> vk::UniqueImageView;

#endif //_VULKANRENDERER_TEXTURE_
