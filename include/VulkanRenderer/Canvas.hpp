#pragma once

#include "Extent.hpp"
#include "StrongType.hpp"
#include "Bitmap.hpp"

#include <optional>
#include <functional>

struct Pixel8bitRGBA
{
	using PixelType = uint8_t;
	PixelType r;
	PixelType g;
	PixelType b;
	PixelType a;
};

struct CanvasOffset
{
	uint32_t x;
	uint32_t y;
};

struct CanvasExtent
{
	uint32_t width;
	uint32_t height;
};

using CheckerSquareSize = StrongType<uint32_t, struct CheckerSquareSizeTag>;

struct Canvas8bitRGBA
{
	Canvas8bitRGBA() = default;
	~Canvas8bitRGBA() = default;
	Canvas8bitRGBA(const Canvas8bitRGBA&) = delete;
	Canvas8bitRGBA& operator=(const Canvas8bitRGBA&) = delete;
	Canvas8bitRGBA(Canvas8bitRGBA&& rhs);
	Canvas8bitRGBA& operator=(Canvas8bitRGBA&& rhs);

	auto memory_size() 
		const noexcept -> size_t;


	auto at(const uint32_t x, const uint32_t y)
		noexcept -> std::optional<std::reference_wrapper<Pixel8bitRGBA>>;

	auto at(const CanvasOffset offset)
		noexcept -> std::optional<std::reference_wrapper<Pixel8bitRGBA>>;
	
	auto draw_rectangle(const Pixel8bitRGBA color,
						const CanvasOffset offset,
						const CanvasExtent extent)
		-> Canvas8bitRGBA&;

	auto draw_checkerboard(const Pixel8bitRGBA color,
						   const CheckerSquareSize size)
		-> Canvas8bitRGBA&;

	auto draw_coordinate_system(const CanvasExtent arrow)
		-> Canvas8bitRGBA&;

	CanvasExtent extent{0, 0};
	std::vector<Pixel8bitRGBA> pixels{};
};


auto get_pixels(Canvas8bitRGBA& canvas)
	noexcept -> uint8_t*;

auto get_pixels(const Canvas8bitRGBA& canvas)
	noexcept -> const uint8_t*;

template <typename F>
auto operator|(Canvas8bitRGBA&& canvas, F&& f)
	-> decltype(auto)
{
	return std::invoke(std::forward<F>(f), std::move(canvas));
}

[[nodiscard]]
auto create_canvas(const Pixel8bitRGBA color,
				   const CanvasExtent extent)
	noexcept -> Canvas8bitRGBA;


[[nodiscard]]
auto bitmap_as_canvas(LoadedBitmap2D&& bitmap)
	-> Canvas8bitRGBA;


[[nodiscard]]
auto canvas_draw_rectangle(const Pixel8bitRGBA color,
						   const CanvasOffset offset,
						   const CanvasExtent extent,
						   Canvas8bitRGBA&& canvas)
	-> Canvas8bitRGBA;

auto canvas_draw_rectangle(const Pixel8bitRGBA color,
						   const CanvasOffset offset,
						   const CanvasExtent extent)
	-> std::function<Canvas8bitRGBA(Canvas8bitRGBA&&)>;


[[nodiscard]]
auto canvas_draw_checkerboard(const Pixel8bitRGBA color,
							  const CheckerSquareSize size,
							  Canvas8bitRGBA&& canvas)
	-> Canvas8bitRGBA;

auto canvas_draw_checkerboard(const Pixel8bitRGBA color,
							  const CheckerSquareSize size)
	-> std::function<Canvas8bitRGBA(Canvas8bitRGBA&&)>;

[[nodiscard]]
auto canvas_draw_coordinate_system(const CanvasExtent arrow,
								   Canvas8bitRGBA&& canvas)
	-> Canvas8bitRGBA;


auto canvas_draw_coordinate_system(const CanvasExtent arrow)
	-> std::function<Canvas8bitRGBA(Canvas8bitRGBA&&)>;
