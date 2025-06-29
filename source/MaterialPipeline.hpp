#pragma once 

#include <filesystem>

#include "VertexImpl.hpp"
#include "VertexBuffer.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "ShaderTexture.hpp"

#include "ContextImpl.hpp"
#include "PresenterImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "TextureImpl.hpp"
#include "ShaderTextureImpl.hpp"

#include "PipelineUtils.hpp"

#include <algorithm>
#include <map>

// TODO: We dont want a combined image sampler, we want them seperated to be able to swap them
// https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html

struct MaterialPipeline
{
	MaterialPipeline() = default;
	explicit MaterialPipeline(Logger& logger,
							  Render::Context::Impl* context,
							  Presenter::Impl* presenter,
							  DescriptorPool::Impl* descriptor_pool,
							  vk::RenderPass& renderpass,
							  std::filesystem::path const shader_root_path);

	~MaterialPipeline();
	

	MaterialPipeline(MaterialPipeline&& rhs) noexcept;
	MaterialPipeline& operator=(MaterialPipeline&& rhs) noexcept;
	
private:
	struct PushConstants {
		glm::mat4 model;
		glm::vec4 basecolor;
	};
	
	vk::UniquePipelineLayout m_layout;
    vk::UniquePipeline m_pipeline;

	/* The Camera descriptor loads persistent perspective
	 * data, and should happen as a single descriptor load
	 * every frame.
	 */
	struct GlobalBinding
	{
		glm::mat4 view;
		glm::mat4 proj;
	};
	
	struct {
		vk::UniqueDescriptorSetLayout layout;
		std::vector<UniformBuffer<GlobalBinding>> buffers;
	} m_globalbinding;
	
	
	struct {
		CachedTextureDescriptorBinder diffuse;
		CachedTextureDescriptorBinder normal;
		CachedTextureDescriptorBinder specular;
	} m_descriptor_binders;
};
