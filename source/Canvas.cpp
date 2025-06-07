#include <VulkanRenderer/Canvas.hpp>

Canvas8bitRGBA::Canvas8bitRGBA(Canvas8bitRGBA&& rhs) 
{
	std::swap(extent, rhs.extent);
	std::swap(pixels, rhs.pixels);
}

Canvas8bitRGBA copy(const Canvas8bitRGBA& other)
{
	Canvas8bitRGBA copy;
	copy.extent = other.extent;
	copy.pixels = other.pixels;
	return copy;
}

Canvas8bitRGBA& Canvas8bitRGBA::operator=(Canvas8bitRGBA&& rhs) 
{
	std::swap(extent, rhs.extent);
	std::swap(pixels, rhs.pixels);
	return *this;
}

auto Canvas8bitRGBA::memory_size() 
	const noexcept -> size_t
{
    return extent.width * extent.height * sizeof(decltype(pixels)::value_type);
}


auto Canvas8bitRGBA::at(const CanvasOffset offset) 
	noexcept -> std::optional<std::reference_wrapper<Pixel8bitRGBA>>
{
	if (offset.x < extent.width && offset.y < extent.height) {
		const auto i = offset.y * extent.width + offset.x;
		return pixels.at(i);
	}

	return std::nullopt;
}


auto Canvas8bitRGBA::at(const uint32_t x, const uint32_t y)
	noexcept -> std::optional<std::reference_wrapper<Pixel8bitRGBA>>
{
	return at(CanvasOffset{x, y});
}

auto get_pixels(Canvas8bitRGBA& canvas)
	noexcept -> uint8_t*
{
	return reinterpret_cast<uint8_t*>(canvas.pixels.data());
}


auto get_pixels(const Canvas8bitRGBA& canvas)
	noexcept -> uint8_t const*
{
	return reinterpret_cast<uint8_t const*>(canvas.pixels.data());
}

auto create_canvas(const Pixel8bitRGBA color,
				   const CanvasExtent extent) 
	noexcept -> Canvas8bitRGBA
{
	Canvas8bitRGBA canvas{};
	canvas.extent = extent;
	canvas.pixels.resize(extent.width * extent.height, color);
	return canvas;
}

auto bitmap_as_canvas(LoadedBitmap2D&& bitmap)
	-> Canvas8bitRGBA
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


auto Canvas8bitRGBA::draw_rectangle(const Pixel8bitRGBA color,
									const CanvasOffset offset,
									const CanvasExtent extent)
	-> Canvas8bitRGBA&
{
	for (uint32_t dw = 0; dw < extent.width; dw++) {
		for (uint32_t dh = 0; dh < extent.height; dh++) {
			auto accessor = at(offset.x + dw, offset.y + dh);
			if (accessor.has_value()) 
				accessor.value().get() = color;
		}
	}
	
	return *this;
}

auto canvas_draw_rectangle(const Pixel8bitRGBA color,
						   const CanvasOffset offset,
						   const CanvasExtent extent,
						   Canvas8bitRGBA&& canvas)
	-> Canvas8bitRGBA
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

auto canvas_draw_rectangle(const Pixel8bitRGBA color,
						   const CanvasOffset offset,
						   const CanvasExtent extent)
	-> std::function<Canvas8bitRGBA(Canvas8bitRGBA&&)>
{
	return [=] (Canvas8bitRGBA&& canvas)
	{
		return canvas_draw_rectangle(color,
									 offset,
									 extent,
									 std::forward<Canvas8bitRGBA>(canvas));
	};
}

auto Canvas8bitRGBA::draw_checkerboard(const Pixel8bitRGBA color,
									   const CheckerSquareSize size)
	-> Canvas8bitRGBA&
{
	bool apply_width_offset = false;
	for (uint32_t h = 0; h < extent.height; h += size.get()) {
		for (uint32_t w = 0; w < extent.width; w += size.get() * 2) {
			const auto offset = (apply_width_offset) 
				? CanvasOffset{w + size.get(), h}
				: CanvasOffset{w, h};
			const auto extent = CanvasExtent{size.get(), size.get()};
			draw_rectangle(color, offset, extent);
		}
		apply_width_offset = !apply_width_offset;
	}
	return *this;
}


auto Canvas8bitRGBA::draw_coordinate_system(const CanvasExtent arrow)
	-> Canvas8bitRGBA&
{
	const auto green = Pixel8bitRGBA{0, 170, 0, 255};
	const auto red = Pixel8bitRGBA{170, 0, 0, 255};
	const auto blue = Pixel8bitRGBA{0, 0, 170, 255};

	draw_rectangle(green,
				   CanvasOffset{arrow.width, 0},
				   CanvasExtent{arrow.height, arrow.width});
	draw_rectangle(red,
				   CanvasOffset{0, arrow.width},
				   CanvasExtent{arrow.width, arrow.height});
	draw_rectangle(blue,
				   CanvasOffset{0, 0},
				   CanvasExtent{arrow.width, arrow.width});
	draw_rectangle(green,
				   CanvasOffset{extent.width - arrow.width, 0},
				   CanvasExtent{arrow.width, arrow.width});
	draw_rectangle(red,
				   CanvasOffset{0, extent.height - arrow.width},
				   CanvasExtent{arrow.width, arrow.width});
	draw_rectangle(blue,
				   CanvasOffset{
					   extent.width - arrow.width,
					   extent.height - arrow.width},
				   CanvasExtent{arrow.width, arrow.width});

	return *this;
}
