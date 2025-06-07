#pragma once
#ifndef _VULKANRENDERERIMPL_RENDERER_IMPL_
#define _VULKANRENDERERIMPL_RENDERER_IMPL_

#include <VulkanRenderer/GeometryPass.hpp>
#include "VulkanRendererImpl.hpp"
#include "DescriptorPoolImpl.hpp"

struct GeometryPass
{
	vk::Extent2D extent;
	vk::UniqueRenderPass renderpass;
	std::vector<Texture2D> colorbuffers;
	std::vector<vk::UniqueImageView> colorbuffer_views;
	std::vector<Texture2D> depthbuffers;
	std::vector<vk::UniqueImageView> depthbuffer_views;
	std::vector<vk::UniqueFramebuffer> framebuffers;
};

struct Pipelines
{
	BaseTexturePipeline basetexture;
	NormRenderPipeline normcolor;
	WireframePipeline wireframe;
};

void sort_renderable(SortedRenderables* sorted,
					 Renderable renderable);

auto create_pipelines(vk::PhysicalDevice& physical_device,
					  vk::Device& device,
					  vk::CommandPool& command_pool,
					  vk::Queue& graphics_queue,
					  vk::DescriptorPool& descriptor_pool,
					  vk::RenderPass& renderpass,
					  const uint32_t frames_in_flight,
					  const vk::Extent2D render_extent,
					  const std::filesystem::path shader_root_path,
					  bool debug_print) 
	-> Pipelines;

auto create_geometry_pass(vk::PhysicalDevice& physical_device,
						  vk::Device& device,
						  vk::CommandPool& command_pool,
						  vk::Queue& graphics_queue,
						  vk::Extent2D render_extent,
						  const uint32_t frames_in_flight,
						  const bool debug_print)
	-> GeometryPass;


auto render_geometry_pass(GeometryPass& pass,
						  // TODO: Pipelines are captured as a ptr because bind_front
						  //       does not want to capture a reference for it...
						  Pipelines* pipelines,
						  const uint32_t current_frame_in_flight,
						  const uint32_t max_frames_in_flight,
						  const uint64_t total_frames,
						  vk::Device& device,
						  vk::DescriptorPool descriptor_pool,
						  vk::CommandPool& command_pool,
						  vk::Queue& queue,
						  const WorldRenderInfo& world_info,
						  std::vector<Renderable>& renderables)
	-> Texture2D*;


class Renderer::Impl 
{
public:
    explicit Impl(PresentationContext::Impl* presentation_context,
				  DescriptorPool::Impl* descriptor_pool,
				  const std::filesystem::path shaders_root);
    ~Impl();
	
	auto render(const uint32_t current_frame_in_flight,
				const uint64_t total_frames,
				const WorldRenderInfo& world_info,
				std::vector<Renderable>& renderables)
		-> Texture2D*;
	
	std::filesystem::path shaders_root;

	PresentationContext::Impl* presentation_context{nullptr};
	DescriptorPool::Impl* descriptor_pool;

	GeometryPass geometry_pass;
	Pipelines pipelines;
};

#endif //_VULKANRENDERERIMPL_RENDERER_IMPL_
