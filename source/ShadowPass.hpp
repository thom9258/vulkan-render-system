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

enum class ShadowPassTextureState { Readable, Writeable };

class ShadowPassTexture
{
public:
	~ShadowPassTexture() = default;

	ShadowPassTexture() = default;
	ShadowPassTexture(ShadowPassTexture& rhs) = delete;
	ShadowPassTexture(ShadowPassTexture&& rhs);

	ShadowPassTexture& operator=(ShadowPassTexture& rhs) = delete;
	ShadowPassTexture& operator=(ShadowPassTexture&& rhs);

	ShadowPassTexture(Render::Context::Impl* context,
					  U32Extent extent);
	
	void transition_readable(vk::CommandBuffer& commandbuffer);
	void transition_writeable(vk::CommandBuffer& commandbuffer);
	
	Texture2D texture;
	vk::UniqueSampler sampler;
	ShadowPassTextureState state = ShadowPassTextureState::Writeable;

	static const vk::ImageLayout readable_layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	static const vk::ImageLayout writeable_layout = vk::ImageLayout::eTransferDstOptimal;
};


class GenericShadowPass
{
public:
	~GenericShadowPass() = default;

	GenericShadowPass() = default;
	GenericShadowPass(GenericShadowPass&& rhs);
	GenericShadowPass(Logger& logger,
					  Render::Context::Impl* context,
					  Presenter::Impl* presenter,
					  U32Extent extent,
					  VertexPath vertex_path,
					  FragmentPath fragment_path,
					  const bool debug_print);

	GenericShadowPass& operator=(GenericShadowPass&& rhs);

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
		//Texture2D colorbuffer;
		ShadowPassTexture colorbuffer;
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
	OrthographicShadowPass(OrthographicShadowPass&& rhs) = default;

	OrthographicShadowPass(Logger& logger,
						   Render::Context::Impl* context,
						   Presenter::Impl* presenter,
						   U32Extent extent,
						   std::filesystem::path shader_root_path,
						   const bool debug_print);

	OrthographicShadowPass& operator=(OrthographicShadowPass&& rhs) = default;
	
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
	PerspectiveShadowPass(PerspectiveShadowPass&& rhs) = default;
	PerspectiveShadowPass(Logger& logger,
						  Render::Context::Impl* context,
						  Presenter::Impl* presenter,
						  U32Extent extent,
						  std::filesystem::path shader_root_path,
						  const bool debug_print);
	
	PerspectiveShadowPass& operator=(PerspectiveShadowPass&& rhs) = default;

	void record(Logger* logger,
				vk::Device& device,
				CurrentFlightFrame current_flightframe,
				vk::CommandBuffer& commandbuffer,
				CameraUniformData camera_data,
				std::vector<MaterialRenderable>& renderables);
};

