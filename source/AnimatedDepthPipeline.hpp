#pragma once

#include <VulkanRenderer/Renderable.hpp>

#include "AnimationUtils.hpp"
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

using AnimatedVertexPath =
    StrongType<std::filesystem::path, struct AnimatedVertexPathTag>;
using AnimatedFragmentPath =
    StrongType<std::filesystem::path, struct AnimatedFragmentPathTag>;

class AnimatedDepthPipeline {
public:
  ~AnimatedDepthPipeline() = default;

  AnimatedDepthPipeline() = default;
  AnimatedDepthPipeline(AnimatedDepthPipeline &&rhs);
  AnimatedDepthPipeline(std::string_view name, Logger &logger,
                        Render::Context::Impl *context,
                        Presenter::Impl *presenter, vk::RenderPass &renderpass,
                        AnimatedVertexPath vertex_path,
                        AnimatedFragmentPath fragment_path, U32Extent extent,
                        const bool debug_print);

  AnimatedDepthPipeline &operator=(AnimatedDepthPipeline &&rhs);

  struct CameraUniformData {
    glm::mat4 view;
    glm::mat4 proj;
  };

  void record(Render::Context::Impl *context, Logger *logger,
              vk::Device &device, MeshCache &mesh_cache,
              CurrentFlightFrame current_flightframe,
              vk::CommandBuffer &commandbuffer, CameraUniformData camera_data,
              std::vector<ShadowRenderable> &renderables);

private:
  using ShaderVertexType = VertexAnimatedPosNormColorUV;

  U32Extent m_extent;
  std::string m_name;
  vk::UniquePipelineLayout m_layout;
  vk::UniquePipeline m_pipeline;
  vk::UniqueDescriptorSetLayout m_descriptor_layout;
  vk::UniqueDescriptorPool m_descriptor_pool;

  struct CameraUniform {
    UniformMemoryDirectWrite<CameraUniformData> uniform;
    vk::UniqueDescriptorSet set;
  };

  static constexpr uint32_t camera_uniform_set_index = 0;
  FlightFramesArray<CameraUniform> m_camera_uniforms;

  struct ModelInfoUniformData {
    glm::mat4 model_matrix;
    glm::mat4 bone_matrices[animation::max_bone_matrices];
  };

  struct ModelInfoUniform {
    vk::UniqueDescriptorSet set;
    UniformMemory<ModelInfoUniformData> uniform;
  };

  static constexpr uint32_t model_info_set_index = 1;
  using ModelInfoUniformPool =
      std::array<ModelInfoUniform, animation::drawable_models_per_frame>;
  vk::UniqueDescriptorSetLayout m_model_info_layout;
  FlightFramesArray<ModelInfoUniformPool> m_model_info_uniform_pools;
};
