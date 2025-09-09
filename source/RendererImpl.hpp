#pragma once

#include <VulkanRenderer/Renderer.hpp>

#include "PresenterImpl.hpp"
#include "ContextImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "FlightFrames.hpp"

#include "NormRenderPipeline.hpp"
#include "WireframePipeline.hpp"
#include "BaseTexturePipeline.hpp"
#include "MaterialPipeline.hpp"

class ShadowPass
{
public:
	ShadowPass() = default;
	ShadowPass(Logger& logger,
			   Render::Context::Impl* context,
			   Presenter::Impl* presenter,
			   U32Extent extent,
			   const std::filesystem::path shader_root_path,
			   const bool debug_print);

	struct CameraUniformData
	{
		glm::mat4 view;
		glm::mat4 proj;
	};
	
	void record(Logger* logger,
				vk::Device& device,
				CurrentFlightFrame current_flightframe,
				vk::CommandBuffer& commandbuffer,
				CameraUniformData camera_data,
				std::vector<MaterialRenderable>& renderables);

private:
	U32Extent m_extent;
	vk::UniqueRenderPass m_renderpass;

	struct FrameTextures {
		Texture2D colorbuffer;
		vk::UniqueImageView colorbuffer_view;
		Texture2D depthbuffer;
		vk::UniqueImageView depthbuffer_view;
		vk::UniqueFramebuffer framebuffer;
	};

	FlightFramesArray<FrameTextures> m_framestextures;
	
	struct RenderPipeline 
	{
		vk::UniquePipelineLayout layout;
		vk::UniquePipeline pipeline;
		
		vk::UniqueDescriptorSetLayout descriptor_layout;
		vk::UniqueDescriptorPool descriptor_pool;
		
		struct PushConstants
		{
			glm::mat4 model;
		};
		
		std::vector<AllocatedMemory> descriptor_memories;
		std::vector<vk::UniqueDescriptorSet> descriptor_sets;
	};
	
	RenderPipeline m_pipeline;
};

struct GeometryPass
{
	vk::Extent2D extent;
	vk::UniqueRenderPass renderpass;
	std::vector<Texture2D::Impl> colorbuffers;
	std::vector<vk::UniqueImageView> colorbuffer_views;
	std::vector<Texture2D::Impl> depthbuffers;
	std::vector<vk::UniqueImageView> depthbuffer_views;
	std::vector<vk::UniqueFramebuffer> framebuffers;
};

struct GeometryPipelines
{
	BaseTexturePipeline basetexture;
	NormRenderPipeline normcolor;
	WireframePipeline wireframe;
	MaterialPipeline material;
};

struct SortedRenderables
{
	std::vector<BaseTextureRenderable> basetextures;
	std::vector<NormColorRenderable> normcolors;
	std::vector<WireframeRenderable> wireframes;
	std::vector<MaterialRenderable> materialrenderables;
};

void sort_renderable(Logger* logger,
					 SortedRenderables* sorted,
					 Renderable renderable);

auto create_geometry_pass(vk::PhysicalDevice& physical_device,
						  vk::Device& device,
						  vk::CommandPool& command_pool,
						  vk::Queue& graphics_queue,
						  vk::Extent2D render_extent,
						  const uint32_t frames_in_flight,
						  const bool debug_print)
	-> GeometryPass;

auto render_geometry_pass(GeometryPass& pass,
						  ShadowPass& shadow_pass,
						  // TODO: Pipelines are captured as a ptr because bind_front
						  //       does not want to capture a reference for it...
						  GeometryPipelines* pipelines,
						  Logger* logger,
						  const uint32_t current_frame_in_flight,
						  const uint32_t max_frames_in_flight,
						  const uint64_t total_frames,
						  vk::Device& device,
						  vk::DescriptorPool descriptor_pool,
						  vk::CommandPool& command_pool,
						  vk::Queue& queue,
						  const WorldRenderInfo& world_info,
						  std::vector<Renderable>& renderables)
	-> Texture2D::Impl*;


class Renderer::Impl 
{
public:
    explicit Impl(Render::Context::Impl* context,
				  Presenter::Impl* presenter,
				  Logger logger,
				  DescriptorPool::Impl* descriptor_pool,
				  const std::filesystem::path shaders_root);
    ~Impl();
	
	auto render(const uint32_t current_frame_in_flight,
				const uint64_t total_frames,
				const WorldRenderInfo& world_info,
				std::vector<Renderable>& renderables,
				std::vector<Light>& lights)
		-> Texture2D::Impl*;
	
	Logger logger;
	std::filesystem::path shaders_root;

	Render::Context::Impl* context{nullptr};
	Presenter::Impl* presenter{nullptr};
	DescriptorPool::Impl* descriptor_pool;

	ShadowPass shadow_pass;
	GeometryPass geometry_pass;
	GeometryPipelines geometry_pipelines;
};
