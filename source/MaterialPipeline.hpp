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
	
	void render(FrameInfo& frame_info,
				Logger& logger,
				vk::Device& device,
				vk::DescriptorPool descriptor_pool,
				vk::CommandBuffer& commandbuffer,
				CurrentFlightFrame const current_flightframe,
				MaxFlightFrames const max_frames_in_flight,
				std::vector<MaterialRenderable>& renderables,
				std::vector<Light>& lights);

	MaterialPipeline(MaterialPipeline&& rhs) noexcept;
	MaterialPipeline& operator=(MaterialPipeline&& rhs) noexcept;
	
private:
	struct PushConstants {
		glm::mat4 model;
	};
	
	vk::UniquePipelineLayout m_layout;
    vk::UniquePipeline m_pipeline;
	
	struct CameraUniformLayout
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 position;
		float _padding1{1.0f};
	};
	

	//struct FrameUniform 
	//{
	//DescriptorSetIndex static constexpr set_index = DescriptorSetIndex{0};
	//vk::UniqueDescriptorSetLayout set_layout;
	//FlightFramesArray<UniformBuffer<CameraUniformLayout>> camera_binding; 
	//FlightFramesArray<UniformBuffer<PointLightUniformLayout>> pointlight_binding; 
	//FlightFramesArray<UniformBuffer<SpotLightUniformLayout>> spotlight_binding; 
	//FlightFramesArray<UniformBuffer<DirectionalLightUniformLayout>> directionallight_binding; 
//};
	//
	//FrameUniform m_frame_uniform;



	Uniform<DescriptorSetIndex{0}, CameraUniformLayout> m_frame_uniform;
	Uniform<DescriptorSetIndex{1}, PointLightUniformLayout> m_pointlight_uniform;
	Uniform<DescriptorSetIndex{2}, SpotLightUniformLayout> m_spotlight_uniform;
	Uniform<DescriptorSetIndex{3}, DirectionalLightUniformLayout> m_directionallight_uniform;
	TextureDescriptor<DescriptorSetIndex{4}> m_ambient;
	TextureDescriptor<DescriptorSetIndex{5}> m_diffuse;
	TextureDescriptor<DescriptorSetIndex{6}> m_specular;
	TextureDescriptor<DescriptorSetIndex{7}> m_normal;
};

