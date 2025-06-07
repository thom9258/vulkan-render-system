#pragma once

#include "VulkanRenderer.hpp"
#include "DescriptorPool.hpp"

#include "NormRenderPipeline.hpp"
#include "WireframePipeline.hpp"
#include "BaseTexturePipeline.hpp"
#include "Texture.hpp"

#include <variant>
#include <algorithm>
#include <filesystem>

using Renderable = std::variant<NormColorRenderable,
								BaseTextureRenderable,
								WireframeRenderable>;


struct SortedRenderables
{
	std::vector<BaseTextureRenderable> basetextures;
	std::vector<NormColorRenderable> normcolors;
	std::vector<WireframeRenderable> wireframes;
};

void sort_renderable(SortedRenderables* sorted,
					 Renderable renderable);

struct WorldRenderInfo
{
	glm::mat4 view;
	glm::mat4 projection;
};

class Renderer
{
public:
	Renderer(PresentationContext& presentation_context,
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

