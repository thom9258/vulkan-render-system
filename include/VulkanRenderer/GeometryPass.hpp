#pragma once

#include "Renderable.hpp"
#include "Context.hpp"
#include "VulkanRenderer.hpp"
#include "DescriptorPool.hpp"

#include <algorithm>
#include <filesystem>



struct WorldRenderInfo
{
	glm::mat4 view;
	glm::mat4 projection;
};

class Renderer
{
public:
	Renderer(Render::Context& context,
			 PresentationContext& presentor,
			 DescriptorPool& descriptor_pool,
			 const std::filesystem::path shaders_root);

	~Renderer();
	
	auto render(const uint32_t current_frame_in_flight,
				const uint64_t total_frames,
				const WorldRenderInfo& world_info,
				std::vector<Renderable>& renderables)
		-> Texture2D*;

	class Impl;
	std::unique_ptr<Impl> impl;
}; 

