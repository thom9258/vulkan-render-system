#pragma once 
#ifndef _VULKANRENDERER_VULKANRENDERER_
#define _VULKANRENDERER_VULKANRENDERER_

#include <variant>
#include <optional>
#include <functional>

#include "Texture.hpp"
#include "Extent.hpp"

struct CurrentFrameInfo
{
	uint64_t total_frame_count;
	uint64_t current_flight_frame_index;
};

struct ReUseLastFrame {};
using GeneratedFrame = std::variant<Texture2D*,
									ReUseLastFrame>;

using FrameProducer = std::function<std::optional<Texture2D*>(CurrentFrameInfo)>;

struct Logger
{
	using FileName = const char*;
	using LineNumber = uint32_t;
	enum class Type { Info, Warn, Error }; 	

	std::function<void(FileName, LineNumber, Type, std::string)> log;
};

struct WindowConfig
{
	std::string name = "unnamed rendering window";
	U32Extent size{1200, 800};
	std::optional<I32Extent> position;
	std::optional<std::int32_t> fps;
};


class PresentationContext 
{
public:
    explicit PresentationContext(WindowConfig const& config, Logger logger);
    ~PresentationContext();

	U32Extent get_window_extent() const noexcept;
	U32Extent window_resize_event_triggered() noexcept;
	
	void wait_until_idle() noexcept;
	void with_presentation(FrameProducer& next_frame_producer);

	class Impl;
	std::unique_ptr<Impl> impl;
};

#endif //_VULKANRENDERER_VULKANRENDERER_
