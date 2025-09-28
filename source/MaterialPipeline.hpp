#pragma once 

#include <filesystem>

#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/ShadowCaster.hpp>

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

#include "LightUniforms.hpp"
#include "PipelineUtils.hpp"

#include <algorithm>
#include <map>

// TODO: We dont want a combined image sampler, we want them seperated to be able to swap them
// https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html

using DescriptorSetIndex = StrongType<uint32_t, struct DescriptorSetIndexTag>;

struct SortedLights
{
	std::vector<DirectionalLight> directionals;
	std::vector<PointLight> points;
	std::vector<SpotLight> spots;
};

void sort_light(Logger* logger,
				SortedLights* sorted,
				Light light);


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
	struct FrameInfo
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 camera_position;
	};
	
	struct MaterialShadowCasters
	{
		struct DirectionalShadowCasterTexture
		{
			vk::DescriptorSet descriptorset;
			DirectionalShadowCaster caster;
		};
		std::optional<DirectionalShadowCasterTexture> directional;
		

		struct SpotShadowCasterTexture
		{
			vk::DescriptorSet descriptorset;
			SpotShadowCaster caster;
		};
		std::vector<SpotShadowCasterTexture> spots;
	};
	
	void render(FrameInfo& frame_info,
				Logger& logger,
				vk::Device& device,
				vk::DescriptorPool descriptor_pool,
				vk::CommandBuffer& commandbuffer,
				CurrentFlightFrame const current_flightframe,
				MaxFlightFrames const max_frames_in_flight,
				std::vector<MaterialRenderable>& renderables,
				std::vector<Light>& lights,
				MaterialShadowCasters shadowcasters);

	MaterialPipeline(MaterialPipeline&& rhs) noexcept;
	MaterialPipeline& operator=(MaterialPipeline&& rhs) noexcept;
	
private:
	struct PushConstants {
		glm::mat4 model;
	};
	
	vk::UniquePipelineLayout m_layout;
    vk::UniquePipeline m_pipeline;
	
	struct CameraUniformData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 position;
		float _padding1{1.0f};
	};
	
	struct LightArrayLengthsUniformData
	{
		int point_length = 0;
		int spot_length = 0;
		int directional_length = 0;
		int _padding1{0};
	};

	static constexpr size_t camera_uniform_count = 1;
	static constexpr size_t lightarray_lengths_count = 1;
	static constexpr size_t directional_shadowcasters_count = 1;
	static constexpr uint32_t directional_shadowcaster_set_index = 5;

	static constexpr size_t max_pointlights = 10;
	static constexpr size_t max_spotlights = 10;
	static constexpr size_t max_directionallights = 10;

	struct GlobalSetUniform
	{
		vk::UniqueDescriptorSet set;
		UniformMemoryDirectWrite<CameraUniformData> camera; 
		UniformMemoryDirectWrite<PointLightUniformData> pointlight; 
		UniformMemoryDirectWrite<SpotLightUniformData> spotlight; 
		UniformMemoryDirectWrite<DirectionalLightUniformData> directionallight; 
		UniformMemoryDirectWrite<LightArrayLengthsUniformData> lightarray_lengths; 
		UniformMemoryDirectWrite<DirectionalShadowCasterUniformData> directional_shadowcaster; 
	};
	
	vk::UniqueDescriptorSetLayout m_global_set_layout;
	FlightFramesArray<GlobalSetUniform> m_global_set_uniforms;

	//TODO make all the samplers part of a single sampler uniform set
	TextureDescriptor<DescriptorSetIndex{1}> m_ambient;
	TextureDescriptor<DescriptorSetIndex{2}> m_diffuse;
	TextureDescriptor<DescriptorSetIndex{3}> m_specular;
	TextureDescriptor<DescriptorSetIndex{4}> m_normal;

	vk::UniqueDescriptorSetLayout m_directional_shadowmap_layout;
};

