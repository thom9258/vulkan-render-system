#pragma once

#include "stb_image.h"

#include <VulkanRenderer/Exception.hpp>

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

struct BitmapLoadError : public Exception
{
	BitmapLoadError(const char* what) : Exception(what) {}
};

struct BitmapInvalidNativePixels : public Exception
{
	BitmapInvalidNativePixels() : Exception("Invalid Native Pixels") {}
};

using LoadBitmapResult = std::variant<LoadedBitmap2D,
									  InvalidPath,
									  BitmapInvalidNativePixels,
									  BitmapLoadError>;

template <typename F>
decltype(auto) operator|(LoadBitmapResult&& result, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(result));
}

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

auto throw_on_bitmap_error()
	-> std::function<LoadBitmapResult(LoadBitmapResult&& result)>;

auto get_bitmap()
	-> std::function<LoadedBitmap2D(LoadBitmapResult&& result)>;
