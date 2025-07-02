#include <VulkanRenderer/Bitmap.hpp>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#include <VulkanRenderer/stb_image.h>


constexpr uint32_t
BitmapPixelFormatToSTBIFormat(BitmapPixelFormat format) noexcept
{
	switch (format) {	
	case BitmapPixelFormat::RGBA:
		return STBI_rgb_alpha;
	default:
		break;
	};
	return STBI_rgb_alpha;
}

LoadedBitmap2D::LoadedBitmap2D(LoadedBitmap2D&& rhs) {
	std::swap(format, rhs.format);
	std::swap(width, rhs.width);
	std::swap(height, rhs.height);
	std::swap(channels, rhs.channels);
	std::swap(pixels, rhs.pixels);
}

LoadedBitmap2D& LoadedBitmap2D::operator=(LoadedBitmap2D&& rhs) {
	std::swap(format, rhs.format);
	std::swap(width, rhs.width);
	std::swap(height, rhs.height);
	std::swap(channels, rhs.channels);
	std::swap(pixels, rhs.pixels);
	return *this;
}

size_t
LoadedBitmap2D::memory_size() const noexcept
{
    return width * height * 4;
}

LoadedBitmap2D::~LoadedBitmap2D()
{
	stbi_image_free(pixels);
}


auto load_bitmap(const std::filesystem::path path, 
				 BitmapPixelFormat format, 
				 VerticalFlipOnLoad flip)
	noexcept -> LoadBitmapResult
{
	if (!std::filesystem::exists(path))
		return InvalidPath{path};

	stbi_set_flip_vertically_on_load(flip == VerticalFlipOnLoad::Yes);
	LoadedBitmap2D bitmap;
	bitmap.format = format;
    bitmap.pixels = stbi_load(path.string().c_str(),
							  &bitmap.width,
							  &bitmap.height,
							  &bitmap.channels,
							  BitmapPixelFormatToSTBIFormat(format));
	
    if (!bitmap.pixels)
		return BitmapLoadError{stbi_failure_reason()};

	return bitmap;
}


auto get_pixels(LoadedBitmap2D const& bitmap)
	-> uint8_t*
{
	return bitmap.pixels;
}

auto throw_on_bitmap_error()
	-> std::function<LoadBitmapResult(LoadBitmapResult&& result)>
{
	return [] (LoadBitmapResult&& result) 
		-> LoadBitmapResult
	{
		if (std::holds_alternative<LoadedBitmap2D>(result))
			return std::move(result);
		if (auto p = std::get_if<InvalidPath>(&result))
			throw *p;
		if (auto p = std::get_if<BitmapInvalidNativePixels>(&result))
			throw *p;
		if (auto p = std::get_if<BitmapLoadError>(&result))
			throw *p;
		throw Exception("Unknown variant in LoadBitmapResult");
	};
}

auto get_bitmap()
	-> std::function<LoadedBitmap2D(LoadBitmapResult&& result)>
{
	return [] (LoadBitmapResult&& result)
		-> LoadedBitmap2D
	{
		return std::move(std::get<LoadedBitmap2D>(result));
	};
}
