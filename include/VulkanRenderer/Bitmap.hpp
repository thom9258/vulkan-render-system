#pragma once

#include "stb_image.h"

#include <filesystem>
#include <functional>
#include <variant>

enum class BitmapPixelFormat 
{
	RGBA,
};

struct LoadedBitmap2D
{
	LoadedBitmap2D() = default;
	~LoadedBitmap2D();
	LoadedBitmap2D(const LoadedBitmap2D&) = delete;
	LoadedBitmap2D& operator=(const LoadedBitmap2D&) = delete;
	LoadedBitmap2D(LoadedBitmap2D&& rhs);
	LoadedBitmap2D& operator=(LoadedBitmap2D&& rhs);

	size_t memory_size() const noexcept;
	
	BitmapPixelFormat format;
	int width{0};
	int height{0};
	int channels{0};
	stbi_uc* pixels{nullptr};
};

template <typename F>
decltype(auto) operator|(LoadedBitmap2D&& bitmap, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(bitmap));
}

struct InvalidPath
{
	std::filesystem::path path;
};

struct LoadError
{
	const char* why{nullptr};
};

struct InvalidNativePixels
{
};

using LoadBitmapError = std::variant<InvalidPath,
									 InvalidNativePixels,
									 LoadError>;

using LoadBitmapResult = std::variant<LoadedBitmap2D,
									  InvalidPath,
									  InvalidNativePixels,
									  LoadError>;

enum class VerticalFlipOnLoad
{
	Yes,
	No
};

auto load_bitmap(const std::filesystem::path path, 
				 BitmapPixelFormat format, 
				 VerticalFlipOnLoad flip) 
	noexcept -> LoadBitmapResult;


auto get_pixels(LoadedBitmap2D const& bitmap)
	-> uint8_t*;


auto get_bitmap_or_throw(LoadBitmapResult&& result)
	-> LoadedBitmap2D;
