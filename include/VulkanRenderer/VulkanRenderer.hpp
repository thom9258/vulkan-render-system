#pragma once 
#ifndef _VULKANRENDERER_VULKANRENDERER_
#define _VULKANRENDERER_VULKANRENDERER_

#include <variant>
#include <optional>
#include <functional>
#include <string>
#include <memory>

#include "Context.hpp"
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

class PresentationContext 
{
public:
    explicit PresentationContext(Render::Context* context);
    ~PresentationContext();

	void with_presentation(FrameProducer& next_frame_producer);

	class Impl;
	std::unique_ptr<Impl> impl;
};

#endif //_VULKANRENDERER_VULKANRENDERER_
