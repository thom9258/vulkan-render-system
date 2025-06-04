#pragma once

#include "Bitmap.hpp"

struct Pixel8bitRGBA
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
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

struct CheckerSquareSize
{
	uint32_t xy;
};

struct Canvas8bitRGBA
{
	Canvas8bitRGBA() = default;
	~Canvas8bitRGBA() = default;
	Canvas8bitRGBA(const Canvas8bitRGBA&) = delete;
	Canvas8bitRGBA& operator=(const Canvas8bitRGBA&) = delete;
	Canvas8bitRGBA(Canvas8bitRGBA&& rhs);
	Canvas8bitRGBA& operator=(Canvas8bitRGBA&& rhs);

	size_t memory_size() const noexcept;

	std::optional<std::reference_wrapper<Pixel8bitRGBA>>
	at(const CanvasOffset offset) noexcept;

	std::optional<std::reference_wrapper<Pixel8bitRGBA>>
	at(const uint32_t x, const uint32_t y) noexcept;

	CanvasExtent extent{0, 0};
	std::vector<Pixel8bitRGBA> pixels{};
};

template <typename F>
decltype(auto) operator|(Canvas8bitRGBA&& canvas, F&& f)
{
	return std::invoke(std::forward<F>(f), std::move(canvas));
}

[[nodiscard]]
Canvas8bitRGBA as_canvas(LoadedBitmap2D&& bitmap)
{
	if (bitmap.format != BitmapPixelFormat::RGBA)
		throw std::runtime_error("Only RGBA format can be currently converted to canvas...");
	
	Canvas8bitRGBA canvas;
	canvas.extent = CanvasExtent{static_cast<uint32_t>(bitmap.width),
		                         static_cast<uint32_t>(bitmap.height)};
	canvas.pixels.resize(bitmap.width * bitmap.height);
	for (size_t i = 0; i < canvas.pixels.size(); i++) {
		canvas.pixels[i] = Pixel8bitRGBA{bitmap.pixels[i*4+0],
			                             bitmap.pixels[i*4+1],
										 bitmap.pixels[i*4+2],
										 bitmap.pixels[i*4+3]};
	}

	return canvas;
}

Canvas8bitRGBA
draw_rectangle(const Pixel8bitRGBA color,
			   const CanvasOffset offset,
			   const CanvasExtent extent,
			   Canvas8bitRGBA&& canvas)
{
	for (uint32_t dw = 0; dw < extent.width; dw++) {
		for (uint32_t dh = 0; dh < extent.height; dh++) {
			auto accessor = canvas.at(offset.x + dw, offset.y + dh);
			if (accessor.has_value()) 
				accessor.value().get() = color;
		}
	}
	
	return canvas;
}

decltype(auto) 
draw_rectangle(const Pixel8bitRGBA color,
			   const CanvasOffset offset,
			   const CanvasExtent extent)
{
	return [=] (Canvas8bitRGBA&& canvas) 
	{
		return draw_rectangle(color, offset, extent, std::move(canvas));
	};
}

Canvas8bitRGBA
draw_checkerboard(const Pixel8bitRGBA color,
				  const CheckerSquareSize size,
				  Canvas8bitRGBA&& canvas)
{
	Canvas8bitRGBA board = std::move(canvas);

	bool apply_width_offset = false;
	for (uint32_t h = 0; h < board.extent.height; h += size.xy) {
		for (uint32_t w = 0; w < board.extent.width; w += size.xy*2) {
			const auto offset = (apply_width_offset) 
				? CanvasOffset{w + size.xy, h}
				: CanvasOffset{w, h};
			const auto extent = CanvasExtent{size.xy, size.xy};
			board = std::move(board) | draw_rectangle(color, offset, extent);
		}
		apply_width_offset = !apply_width_offset;
	}
	return board;
}

decltype(auto)
draw_checkerboard(const Pixel8bitRGBA color,
				  const CheckerSquareSize size)
{
	return [=] (Canvas8bitRGBA&& canvas) 
	{
		return draw_checkerboard(color, size, std::move(canvas));
	};
}

Canvas8bitRGBA
draw_coordinate_system(const CanvasExtent arrow, Canvas8bitRGBA&& canvas)
{
	const auto green = Pixel8bitRGBA{0, 170, 0, 255};
	const auto red = Pixel8bitRGBA{170, 0, 0, 255};
	const auto blue = Pixel8bitRGBA{0, 0, 170, 255};
	const auto canvas_extent = canvas.extent;

	return std::move(canvas)
		| draw_rectangle(green,
						 CanvasOffset{arrow.width, 0},
						 CanvasExtent{arrow.height, arrow.width})
		| draw_rectangle(red,
						 CanvasOffset{0, arrow.width},
						 CanvasExtent{arrow.width, arrow.height})
		| draw_rectangle(blue,
						 CanvasOffset{0, 0},
						 CanvasExtent{arrow.width, arrow.width})
		| draw_rectangle(green,
						 CanvasOffset{canvas_extent.width - arrow.width, 0},
						 CanvasExtent{arrow.width, arrow.width})
		| draw_rectangle(red,
						 CanvasOffset{0, canvas_extent.height - arrow.width},
						 CanvasExtent{arrow.width, arrow.width})
		| draw_rectangle(blue,
						 CanvasOffset{canvas_extent.width - arrow.width,
							          canvas_extent.height - arrow.width},
						 CanvasExtent{arrow.width, arrow.width});
}

decltype(auto)
draw_coordinate_system(const CanvasExtent arrow)
{
	return [=] (Canvas8bitRGBA&& canvas)
	{
		return draw_coordinate_system(arrow, std::move(canvas));
	};
}
