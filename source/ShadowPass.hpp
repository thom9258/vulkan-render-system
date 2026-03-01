#pragma once

#include <VulkanRenderer/Renderable.hpp>

#include "AnimatedDepthPipeline.hpp"
#include "ContextImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "FlightFrames.hpp"
#include "Mesh.hpp"
#include "PipelineUtils.hpp"
#include "Presenter.hpp"
#include "ShaderTexture.hpp"
#include "ShaderTextureImpl.hpp"
#include "StaticDepthPipeline.hpp"
#include "Texture.hpp"
#include "TextureImpl.hpp"
#include "VertexBufferImpl.hpp"
#include "VertexImpl.hpp"

enum class ShadowPassTextureState { Readable, Writeable };

class ShadowPassTexture {
public:
  ~ShadowPassTexture() = default;

  ShadowPassTexture() = default;
  ShadowPassTexture(ShadowPassTexture &rhs) = delete;
  ShadowPassTexture(ShadowPassTexture &&rhs);

  ShadowPassTexture &operator=(ShadowPassTexture &rhs) = delete;
  ShadowPassTexture &operator=(ShadowPassTexture &&rhs);

  ShadowPassTexture(Render::Context::Impl *context,
                    DescriptorPool::Impl *descriptor_pool, U32Extent extent);

  Texture2D texture;
  vk::UniqueSampler sampler;
  vk::UniqueImageView view;
  vk::UniqueDescriptorSet descriptorset;
  vk::UniqueDescriptorSetLayout descriptorset_layout;
  ShadowPassTextureState state = ShadowPassTextureState::Writeable;

  static const vk::ImageLayout readable_layout =
      vk::ImageLayout::eShaderReadOnlyOptimal;
  static const vk::ImageLayout writeable_layout =
      vk::ImageLayout::eTransferDstOptimal;
};

class GenericShadowPass {
public:
  ~GenericShadowPass() = default;

  GenericShadowPass() = default;
  GenericShadowPass(GenericShadowPass &&rhs);
  GenericShadowPass(std::string_view name, Render::Context::Impl *context, Logger &logger,
                    Presenter &presenter,
                    DescriptorPool::Impl *descriptor_pool, U32Extent extent,
                    StaticVertexPath static_vertex_path,
                    StaticFragmentPath static_fragment_path,
                    AnimatedVertexPath animated_vertex_path,
                    AnimatedFragmentPath animated_fragment_path,
                    const bool debug_print);

  GenericShadowPass &operator=(GenericShadowPass &&rhs);

  struct CameraUniformData {
    glm::mat4 view;
    glm::mat4 proj;
  };

  void record(Render::Context::Impl *context, Logger *logger,
              vk::Device &device, MeshCache &mesh_cache,
              CurrentFlightFrame current_flightframe,
              vk::CommandBuffer &commandbuffer,
              std::optional<CameraUniformData> camera_data,
              std::vector<ShadowRenderable> &renderables);

  auto get_shadowtexture(CurrentFlightFrame current_flightframe)
      -> ShadowPassTexture &;

private:
	std::string m_name;
  U32Extent m_extent;
  vk::UniqueRenderPass m_renderpass;

  struct FrameTextures {
    ShadowPassTexture colorbuffer;
    vk::UniqueImageView colorbuffer_view;
    Texture2D depthbuffer;
    vk::UniqueImageView depthbuffer_view;
    vk::UniqueFramebuffer framebuffer;
  };

  FlightFramesArray<FrameTextures> m_framestextures;

  StaticDepthPipeline m_static_pipeline;
  AnimatedDepthPipeline m_animated_pipeline;
};

class OrthographicShadowPass : public GenericShadowPass {
public:
  OrthographicShadowPass() = default;
  OrthographicShadowPass(OrthographicShadowPass &&rhs) = default;

  OrthographicShadowPass(Logger &logger, Render::Context::Impl *context,
                         Presenter& presenter,
                         DescriptorPool::Impl *descriptor_pool,
                         U32Extent extent, StaticVertexPath static_vertex_path,
                         StaticFragmentPath static_fragment_path,
                         AnimatedVertexPath animated_vertex_path,
                         AnimatedFragmentPath animated_fragment_path,
                         const bool debug_print);

  OrthographicShadowPass &operator=(OrthographicShadowPass &&rhs) = default;

  void record(Render::Context::Impl *context, Logger *logger,
              vk::Device &device, MeshCache &mesh_cache,
              CurrentFlightFrame current_flightframe,
              vk::CommandBuffer &commandbuffer,
              std::optional<CameraUniformData> camera_data,
              std::vector<ShadowRenderable> &renderables);

  auto get_shadowtexture(CurrentFlightFrame current_flightframe)
      -> ShadowPassTexture &;
};

class PerspectiveShadowPass : public GenericShadowPass {
public:
  PerspectiveShadowPass() = default;
  PerspectiveShadowPass(PerspectiveShadowPass &&rhs) = default;
  PerspectiveShadowPass(Logger &logger, Render::Context::Impl *context,
                        Presenter& presenter,
                        DescriptorPool::Impl *descriptor_pool, U32Extent extent,
                        StaticVertexPath static_vertex_path,
                        StaticFragmentPath static_fragment_path,
                        AnimatedVertexPath animated_vertex_path,
                        AnimatedFragmentPath animated_fragment_path,
                        const bool debug_print);

  PerspectiveShadowPass &operator=(PerspectiveShadowPass &&rhs) = default;

  void record(Render::Context::Impl *context, Logger *logger,
              vk::Device &device, MeshCache &mesh_cache,
              CurrentFlightFrame current_flightframe,
              vk::CommandBuffer &commandbuffer,
              std::optional<CameraUniformData> camera_data,
              std::vector<ShadowRenderable> &renderables);

  auto get_shadowtexture(CurrentFlightFrame current_flightframe)
      -> ShadowPassTexture &;
};
