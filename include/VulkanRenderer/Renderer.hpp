#pragma once

#include "Renderable.hpp"
#include "Light.hpp"
#include "ShadowCaster.hpp"
#include "Context.hpp"
#include "Presenter.hpp"
#include "DescriptorPool.hpp"

#include <algorithm>
#include <filesystem>



struct WorldRenderInfo
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::vec3 camera_position;
};

class Renderer
{
public:
	Renderer(Render::Context& context,
			 Presenter& presenter,
			 Logger logger,
			 DescriptorPool& descriptor_pool,
			 const std::filesystem::path shaders_root);

	~Renderer();
	
	auto render(const uint32_t current_frame_in_flight,
				const uint64_t total_frames,
				const WorldRenderInfo& world_info,
				std::vector<Renderable>& renderables,
				std::vector<Light>& lights,
				ShadowCasters& shadowcasters)
		-> Texture2D::Impl*;

	class Impl;
	std::unique_ptr<Impl> impl;
}; 

