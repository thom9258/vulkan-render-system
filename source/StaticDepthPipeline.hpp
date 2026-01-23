#pragma once

#include <VulkanRenderer/Renderable.hpp>

#include "ContextImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "FlightFrames.hpp"
#include "Mesh.hpp"
#include "PipelineUtils.hpp"
#include "PresenterImpl.hpp"
#include "ShaderTexture.hpp"
#include "ShaderTextureImpl.hpp"
#include "Texture.hpp"
#include "TextureImpl.hpp"
#include "VertexBufferImpl.hpp"
#include "VertexImpl.hpp"

using StaticVertexPath =
    StrongType<std::filesystem::path, struct StaticVertexPathTag>;
using StaticFragmentPath =
    StrongType<std::filesystem::path, struct StaticFragmentPathTag>;

class StaticDepthPipeline {
public:
  ~StaticDepthPipeline() = default;

  StaticDepthPipeline() = default;
  StaticDepthPipeline(StaticDepthPipeline &&rhs);
  StaticDepthPipeline(std::string_view name, Logger &logger, Render::Context::Impl *context,
                      Presenter::Impl *presenter, vk::RenderPass &renderpass,
                      StaticVertexPath vertex_path,
                      StaticFragmentPath fragment_path, U32Extent extent,
                      const bool debug_print);

  StaticDepthPipeline &operator=(StaticDepthPipeline &&rhs);

  struct CameraUniformData {
    glm::mat4 view;
    glm::mat4 proj;
  };

  void record(Logger *logger, vk::Device &device, MeshCache &mesh_cache,
              CurrentFlightFrame current_flightframe,
              vk::CommandBuffer &commandbuffer, CameraUniformData camera_data,
              std::vector<ShadowRenderable> &renderables);

private:
  U32Extent m_extent;
	std::string m]_name;
  vk::UniquePipelineLayout m_layout;
  vk::UniquePipeline m_pipeline;
  vk::UniqueDescriptorSetLayout m_descriptor_layout;
  vk::UniqueDescriptorPool m_descriptor_pool;

  struct PushConstants {
    glm::mat4 model;
  };

  struct CameraUniform {
    UniformMemoryDirectWrite<CameraUniformData> uniform;
    vk::UniqueDescriptorSet set;
  };

  FlightFramesArray<CameraUniform> m_camera_uniforms;
};
