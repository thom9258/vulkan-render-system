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

struct AnimatedPipeline
{
	AnimatedPipeline() = default;
	explicit AnimatedPipeline(Logger& logger,
							  Render::Context::Impl* context,
							  Presenter::Impl* presenter,
							  DescriptorPool::Impl* descriptor_pool,
							  vk::RenderPass& renderpass,
							  std::filesystem::path const shader_root_path);

	~AnimatedPipeline();
	
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
			std::optional<DirectionalShadowCaster> caster;
		};
		DirectionalShadowCasterTexture directional;

		struct SpotShadowCasterTexture
		{
			vk::DescriptorSet descriptorset;
			std::optional<SpotShadowCaster> caster;
		};
		SpotShadowCasterTexture spot;
	};
	
	void render(FrameInfo& frame_info,
				Logger& logger,
				vk::Device& device,
				vk::DescriptorPool descriptor_pool,
				MeshCache& mesh_cache,
				TextureSamplerCache& texturesampler_cache,
				vk::CommandBuffer& commandbuffer,
				CurrentFlightFrame const current_flightframe,
				MaxFlightFrames const max_frames_in_flight,
				std::vector<AnimatedRenderable>& renderables,
				std::vector<Light>& lights,
				MaterialShadowCasters shadowcasters);

	AnimatedPipeline(AnimatedPipeline&& rhs) noexcept;
	AnimatedPipeline& operator=(AnimatedPipeline&& rhs) noexcept;
	
	
private:
	static constexpr std::size_t max_bone_matrices = 100;

	struct ModelInfoUniformData {
		glm::mat4 model_matrix;
		glm::mat4 bone_matrices[max_bone_matrices];
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
	static constexpr size_t spot_shadowcasters_count = 1;
	static constexpr size_t model_infos_count = 1;

	static constexpr uint32_t directional_shadowcaster_set_index = 5;
	static constexpr uint32_t spot_shadowcaster_set_index = 6;

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
		UniformMemoryDirectWrite<SpotShadowCasterUniformData> spot_shadowcaster; 
		UniformMemoryDirectWrite<ModelInfoUniformData> model_info; 
	};
	
	vk::UniqueDescriptorSetLayout m_global_set_layout;
	FlightFramesArray<GlobalSetUniform> m_global_set_uniforms;

	struct ModelInfoUniform
	{
		vk::UniqueDescriptorSet set;
		UniformMemoryDirectWrite<ModelInfoUniformData> uniform; 
	};
	static constexpr uint32_t model_info_uniform_count = 1;
	static constexpr uint32_t model_info_set_index = 7;
	static constexpr size_t model_info_uniforms_per_frame = 100;
	using ModelInfoUniformPool = std::array<ModelInfoUniform, model_info_uniforms_per_frame>;
	vk::UniqueDescriptorSetLayout m_model_info_layout;
	FlightFramesArray<ModelInfoUniformPool> m_model_info_uniform_pools;

	//TODO make all the samplers part of a single sampler uniform set
	TextureDescriptor<DescriptorSetIndex{1}> m_ambient;
	TextureDescriptor<DescriptorSetIndex{2}> m_diffuse;
	TextureDescriptor<DescriptorSetIndex{3}> m_specular;
	TextureDescriptor<DescriptorSetIndex{4}> m_normal;

	vk::UniqueDescriptorSetLayout m_directional_shadowmap_layout;
	vk::UniqueDescriptorSetLayout m_spot_shadowmap_layout;
};
