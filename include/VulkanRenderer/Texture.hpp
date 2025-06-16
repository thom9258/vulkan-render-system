#pragma once

#include "Bitmap.hpp"
#include "Canvas.hpp"
#include "Context.hpp"
#include "Extent.hpp"

#include <iostream>

enum class TextureFormat
{
	R8G8B8A8Srgb,
	R32G32B32Sfloat,
	R32G32Sfloat,
};

struct GeneralTextureType { const int _ignore; };
static constexpr GeneralTextureType GeneralTexture {2501};

struct DepthBufferTextureType { const int _ignore; };
static constexpr DepthBufferTextureType DepthBufferTexture {2501};

struct RenderTargetTextureType { const int _ignore; };
static constexpr RenderTargetTextureType RenderTargetTexture {2501};

struct Texture2D
{
	Texture2D();
	~Texture2D();

	explicit Texture2D(GeneralTextureType, 
					   Render::Context* context,
					   const U32Extent extent,
					   const TextureFormat format);

	explicit Texture2D(DepthBufferTextureType, 
					   Render::Context* context,
					   const U32Extent extent);

	explicit Texture2D(RenderTargetTextureType, 
					   Render::Context* context,
					   const U32Extent extent,
					   const TextureFormat format);
	

	Texture2D(Texture2D&& rhs) noexcept;
	Texture2D& operator=(Texture2D&& rhs) noexcept;
	
	auto extent()
		const noexcept -> U32Extent;

	auto format()
		const noexcept -> TextureFormat;

	struct Impl;
	std::unique_ptr<Impl> impl {nullptr};
	
	Texture2D(std::unique_ptr<Impl>&& impl);
};

template <typename F>
decltype(auto) operator|(Texture2D&& texture, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(texture));
}

auto copy_bitmap_to_gpu(Render::Context* context,
						const LoadedBitmap2D& bitmap)
	-> Texture2D;


auto copy_canvas_to_gpu(Render::Context* context,
						Canvas8bitRGBA& canvas)
	-> Texture2D;

// @note move_to_gpu is just a copy_to_gpu, but it consumes a canvas 
//       as an rvalue-reference, thus the canvas is destroyed after 
//       the operation is completed.
//       this allows move_to_gpu to be used as a pipe.
[[nodiscard]]
auto move_canvas_to_gpu(Render::Context* context,
						Canvas8bitRGBA&& canvas)
	-> Texture2D;

auto move_canvas_to_gpu(Render::Context* context)
	-> std::function<Texture2D(Canvas8bitRGBA&&)>;

[[nodiscard]]
auto move_bitmap_to_gpu(Render::Context* context,
						LoadedBitmap2D&& bitmap)
	-> Texture2D;

auto move_bitmap_to_gpu(Render::Context* context)
	-> std::function<Texture2D(LoadedBitmap2D&&)>;
