#pragma once

#include <memory>
#include <optional>

#include "VulkanRenderer.hpp"

struct DescriptorPoolCreateInfo
{
	std::optional<uint32_t> uniform_buffer_count{std::nullopt};
	std::optional<uint32_t> combined_image_sampler_count{std::nullopt};
};

class DescriptorPool
{
public:
	DescriptorPool(DescriptorPoolCreateInfo const& create_info,
				   PresentationContext& presentation_context);
	~DescriptorPool();

	class Impl;
	std::unique_ptr<Impl> impl;
}; 

