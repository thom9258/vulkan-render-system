#pragma once 

#include <filesystem>

#include <VulkanRenderer/Renderable.hpp>

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

using DescriptorSetIndex = StrongType<uint32_t, struct DescriptorSetIndexTag>;

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
	
	/* The Camera descriptor loads persistent perspective
	 * data, and should happen as a single descriptor load
	 * every frame.
	 */
	struct GlobalBinding
	{
		glm::mat4 view;
		glm::mat4 proj;
	};
	
	void render(GlobalBinding* global_binding,
				Logger& logger,
				vk::Device& device,
				vk::DescriptorPool descriptor_pool,
				vk::CommandBuffer& commandbuffer,
				CurrentFrameInFlight const current_flightframe,
				TotalFramesInFlight const max_frames_in_flight,
				std::vector<MaterialRenderable>& renderables);

	MaterialPipeline(MaterialPipeline&& rhs) noexcept;
	MaterialPipeline& operator=(MaterialPipeline&& rhs) noexcept;
	
private:
	struct PushConstants {
		glm::mat4 model;
		glm::vec4 basecolor;
	};
	
	vk::UniquePipelineLayout m_layout;
    vk::UniquePipeline m_pipeline;

	struct {
		vk::UniqueDescriptorSetLayout layout;
		std::vector<UniformBuffer<GlobalBinding>> buffers;
	} m_globalbinding;

	template <DescriptorSetIndex t_set_index>
	struct TextureDescriptor
	{
		DescriptorSetIndex static constexpr set_index = t_set_index;
		TextureSamplerReadOnly default_texture;
		vk::UniqueDescriptorSetLayout layout;
		std::map<TextureSamplerReadOnly*, std::vector<vk::UniqueDescriptorSet>> sets;
	};
	
	TextureDescriptor<DescriptorSetIndex{1}> m_diffuse;
	TextureDescriptor<DescriptorSetIndex{2}> m_normal;
	TextureDescriptor<DescriptorSetIndex{3}> m_specular;
};
