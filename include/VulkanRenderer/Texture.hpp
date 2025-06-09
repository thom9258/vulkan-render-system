#pragma once
#ifndef _VULKANRENDERER_TEXTURE_
#define _VULKANRENDERER_TEXTURE_

#include "Utils.hpp"
#include "Bitmap.hpp"
#include "Canvas.hpp"
#include "Context.hpp"

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

auto create_empty_texture(Render::Context::Impl* context,
						  const vk::Format format,
						  const vk::Extent3D extent,
						  const vk::ImageTiling tiling,
						  const vk::MemoryPropertyFlags propertyFlags,
						  const vk::ImageUsageFlags usageFlags)
	-> Texture2D;


auto create_empty_general_texture(Render::Context::Impl* context,
								  const vk::Format format,
								  const vk::Extent3D extent,
								  const vk::ImageTiling tiling,
								  const vk::MemoryPropertyFlags propertyFlags)
	-> Texture2D;



auto create_empty_rendertarget_texture(Render::Context::Impl* context,
									   const vk::Format format,
									   const vk::Extent3D extent,
									   const vk::ImageTiling tiling,
									   const vk::MemoryPropertyFlags propertyFlags)
	-> Texture2D;

auto copy_bitmap_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						const LoadedBitmap2D& bitmap)
	-> Texture2D;



auto copy_canvas_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						Canvas8bitRGBA& canvas)
	-> Texture2D;

// @note move_to_gpu is just a copy_to_gpu, but it consumes a canvas 
//       as an rvalue-reference, thus the canvas is destroyed after 
//       the operation is completed.
//       this allows move_to_gpu to be used as a pipe.
[[nodiscard]]
auto move_canvas_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						Canvas8bitRGBA&& canvas)
	-> Texture2D;

auto move_canvas_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags)
	-> std::function<Texture2D(Canvas8bitRGBA&&)>;

[[nodiscard]]
auto move_bitmap_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags,
						LoadedBitmap2D&& bitmap)
	-> Texture2D;

auto move_bitmap_to_gpu(Render::Context::Impl* context,
						const vk::MemoryPropertyFlags propertyFlags)
	-> std::function<Texture2D(LoadedBitmap2D&&)>;

#endif //_VULKANRENDERER_TEXTURE_
