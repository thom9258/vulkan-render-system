#pragma once 

#include <filesystem>

#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/Light.hpp>

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

struct SortedLights
{
	std::vector<DirectionalLight> directionals;
	std::vector<PointLight> points;
	std::vector<SpotLight> spots;
};



void sort_light(Logger* logger,
				SortedLights* sorted,
				Light light);


struct DirectionalLightUniformLayout
{
	glm::vec3 direction;
	float _padding1{1.0f};
	glm::vec3 ambient;
	float _padding2{1.0f};
	glm::vec3 diffuse;
	float _padding3{1.0f};
	glm::vec3 specular;
	float _padding4{1.0f};
};

struct PointLightUniformLayout
{
	glm::vec3 position;
	float _padding1{1.0f};
	glm::vec3 ambient;
	float _padding2{1.0f};
	glm::vec3 diffuse;
	float _padding3{1.0f};
	glm::vec3 specular;
	float _padding4{1.0f};
	struct {
		float constant;
		float linear;
		float quadratic;
	}attenuation;
	float _padding5{1.0f};
};

struct SpotLightUniformLayout
{
	glm::vec3 position;
	float _padding1{1.0f};
	glm::vec3 direction;
	float _padding2{1.0f};
	glm::vec3 ambient;
	float _padding3{1.0f};
	glm::vec3 diffuse;
	float _padding4{1.0f};
	glm::vec3 specular;
	float _padding5{1.0f};
	struct {
		float constant;
		float linear;
		float quadratic;
	}attenuation;
	float _padding6{1.0f};
	struct {
		float inner;
		float outer;
	}cutoff;
	float _padding7[2]{1.0f, 1.0f};
};



struct MaterialPipeline2
{
	MaterialPipeline2() = default;
	explicit MaterialPipeline2(Logger& logger,
							  Render::Context::Impl* context,
							  Presenter::Impl* presenter,
							  DescriptorPool::Impl* descriptor_pool,
							  vk::RenderPass& renderpass,
							  std::filesystem::path const shader_root_path);

	~MaterialPipeline2();
	
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

	MaterialPipeline2(MaterialPipeline2&& rhs) noexcept;
	MaterialPipeline2& operator=(MaterialPipeline2&& rhs) noexcept;
	
private:
	struct PushConstants {
		glm::mat4 model;
	};
	
	vk::UniquePipelineLayout m_layout;
    vk::UniquePipeline m_pipeline;
	
	struct FrameUniformLayout
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 camera_position;
		float _padding1{1.0f};
	};

	Uniform<DescriptorSetIndex{0}, FrameUniformLayout> m_frame_uniform;
	TextureDescriptor<DescriptorSetIndex{1}> m_ambient;
	TextureDescriptor<DescriptorSetIndex{2}> m_diffuse;
	TextureDescriptor<DescriptorSetIndex{3}> m_specular;
	TextureDescriptor<DescriptorSetIndex{4}> m_normal;
	Uniform<DescriptorSetIndex{5}, PointLightUniformLayout> m_pointlight_uniform;
	Uniform<DescriptorSetIndex{6}, SpotLightUniformLayout> m_spotlight_uniform;
	Uniform<DescriptorSetIndex{7}, DirectionalLightUniformLayout> m_directionallight_uniform;
};
