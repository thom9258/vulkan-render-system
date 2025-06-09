#pragma once

#include <memory>
#include <optional>

#include "Context.hpp"

struct DescriptorPoolCreateInfo
{
	std::optional<uint32_t> uniform_buffer_count{std::nullopt};
	std::optional<uint32_t> combined_image_sampler_count{std::nullopt};
};

class DescriptorPool
{
public:
	DescriptorPool(DescriptorPoolCreateInfo const& create_info,
				   Render::Context& context);
	~DescriptorPool();

	class Impl;
	std::unique_ptr<Impl> impl;
}; 

