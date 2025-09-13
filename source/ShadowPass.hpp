#pragma once

#include <VulkanRenderer/Renderable.hpp>

#include "FlightFrames.hpp"
#include "VertexImpl.hpp"
#include "VertexBufferImpl.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "ShaderTexture.hpp"
#include "ContextImpl.hpp"
#include "PresenterImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "TextureImpl.hpp"
#include "ShaderTextureImpl.hpp"
#include "PipelineUtils.hpp"

class GenericShadowPass
{
public:
	GenericShadowPass() = default;
	GenericShadowPass(Logger& logger,
					  Render::Context::Impl* context,
					  Presenter::Impl* presenter,
					  U32Extent extent,
					  VertexPath vertex_path,
					  FragmentPath fragment_path,
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


class OrthographicShadowPass 
	: public GenericShadowPass
{
public:
	OrthographicShadowPass() = default;
	OrthographicShadowPass(Logger& logger,
						   Render::Context::Impl* context,
						   Presenter::Impl* presenter,
						   U32Extent extent,
						   std::filesystem::path shader_root_path,
						   const bool debug_print);
	
	void record(Logger* logger,
				vk::Device& device,
				CurrentFlightFrame current_flightframe,
				vk::CommandBuffer& commandbuffer,
				CameraUniformData camera_data,
				std::vector<MaterialRenderable>& renderables);
};

class PerspectiveShadowPass 
	: public GenericShadowPass
{
public:
	PerspectiveShadowPass() = default;
	PerspectiveShadowPass(Logger& logger,
						  Render::Context::Impl* context,
						  Presenter::Impl* presenter,
						  U32Extent extent,
						  std::filesystem::path shader_root_path,
						  const bool debug_print);
	
	void record(Logger* logger,
				vk::Device& device,
				CurrentFlightFrame current_flightframe,
				vk::CommandBuffer& commandbuffer,
				CameraUniformData camera_data,
				std::vector<MaterialRenderable>& renderables);
};

