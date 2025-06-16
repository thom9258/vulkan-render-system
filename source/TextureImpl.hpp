#pragma once
#include <VulkanRenderer/Texture.hpp>
#include <VulkanRenderer/Utils.hpp>

#include "ContextImpl.hpp"

struct Texture2D::Impl
{
	explicit Impl(GeneralTextureType, 
				  Render::Context::Impl* context,
				  const U32Extent extent,
				  const TextureFormat format);

	explicit Impl(DepthBufferTextureType, 
				  Render::Context::Impl* context,
				  const U32Extent extent);

	explicit Impl(RenderTargetTextureType, 
				  Render::Context::Impl* context,
				  const U32Extent extent,
				  const TextureFormat format);

	~Impl();
	
	Impl(Impl&& rhs) noexcept;

	auto operator=(Impl&& rhs) 
		noexcept -> Impl&;

	auto image()
	-> vk::Image&;

	auto create_view(Render::Context::Impl* context,
					 const vk::ImageAspectFlags aspect)
	-> vk::UniqueImageView;

	AllocatedImage allocated;
	vk::Extent3D extent;
	vk::Format format;
	vk::ImageLayout layout;
};

auto move_canvas_to_gpu(Render::Context::Impl* context,
						Canvas8bitRGBA&& canvas)
	-> Texture2D;

auto move_canvas_to_gpu(Render::Context::Impl* context)
	-> std::function<Texture2D(Canvas8bitRGBA&& canvas)>;

auto move_bitmap_to_gpu(Render::Context::Impl* context,
						LoadedBitmap2D&& bitmap)
	-> Texture2D;

auto move_bitmap_to_gpu(Render::Context::Impl* context)
	-> std::function<Texture2D(LoadedBitmap2D&& bitmap)>;



#if 0
auto texture_create_depthbuffer(Render::Context::Impl* context,
								const U32Extent extent)
	-> Texture2D;

auto texture_create_rendertarget(Render::Context::Impl* context,
								 const U32Extent extent,
								 const TextureFormat format)
	-> Texture2D;
auto texture_create_general(Render::Context::Impl* context,
							const U32Extent extent,
							const TextureFormat format)
	-> Texture2D;
#endif

auto vkformat_to_textureformat(vk::Format format)
	-> TextureFormat;

auto textureformat_to_vkformat(TextureFormat format)
	-> vk::Format;
