#pragma once

#include <VulkanRenderer/FlightFrames.hpp>
#include <VulkanRenderer/Renderer.hpp>

#include "ContextImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "Presenter.hpp"

#include "AnimatedPipeline.hpp"
#include "MaterialPipeline.hpp"
#include "NormRenderPipeline.hpp"
#include "ShadowPass.hpp"
#include "WireframePipeline.hpp"

struct GeometryPass {
  vk::Extent2D extent;
  vk::UniqueRenderPass renderpass;
  std::vector<Texture2D::Impl> colorbuffers;
  std::vector<vk::UniqueImageView> colorbuffer_views;
  std::vector<Texture2D::Impl> depthbuffers;
  std::vector<vk::UniqueImageView> depthbuffer_views;
  std::vector<vk::UniqueFramebuffer> framebuffers;
};

struct GeometryPipelines {
  NormRenderPipeline normcolor;
  WireframePipeline wireframe;
  MaterialPipeline material;
  AnimatedPipeline animated;
};

struct SortedRenderables {
  std::vector<NormColorRenderable> normcolors;
  std::vector<WireframeRenderable> wireframes;
  std::vector<MaterialRenderable> materialrenderables;
  std::vector<AnimatedRenderable> animated_renderables;
  std::vector<RenderableNodePtr> renderablenodes;
};

class Renderer::Impl {
public:
  explicit Impl(Render::Context *context, Logger logger,
                DescriptorPool::Impl *descriptor_pool,
                const std::filesystem::path shaders_root);

  ~Impl();

  RenderedFrameStats with_render(Render::Context *context, RenderInfoCreator render_info_creator);

  auto render(Render::Context::Impl *context,
              TextureSamplerCache &texture_cache, MeshCache &mesh_cache,
              const uint32_t current_frame_in_flight,
              const uint64_t total_frames, const WorldRenderInfo &world_info,
              std::vector<Renderable> &renderables, std::vector<Light> &lights,
              ShadowCasters &shadowcasters) -> Texture2D::Impl *;

  Logger logger;
  std::filesystem::path shaders_root;

  Render::Context *context{nullptr};
  Presenter presenter;
  DescriptorPool::Impl *descriptor_pool;

  struct ShadowPasses {
    OrthographicShadowPass orthographic;
    PerspectiveShadowPass perspective;
  };

  ShadowPasses shadow_passes;
  GeometryPass geometry_pass;
  GeometryPipelines geometry_pipelines;
};

void sort_renderable(Logger *logger, SortedRenderables *sorted,
                     Renderable renderable);

auto create_geometry_pass(vk::PhysicalDevice &physical_device,
                          vk::Device &device, vk::CommandPool &command_pool,
                          vk::Queue &graphics_queue, vk::Extent2D render_extent,
                          const uint32_t frames_in_flight,
                          const bool debug_print) -> GeometryPass;

auto render_geometry_pass(
    Render::Context::Impl *context, GeometryPass &pass,
    Renderer::Impl::ShadowPasses &shadow_passes,
    // TODO: Pipelines are captured as a ptr because bind_front
    //       does not want to capture a reference for it...
    GeometryPipelines *pipelines, Logger *logger,
    const uint32_t current_frame_in_flight, const uint32_t max_frames_in_flight,
    const uint64_t total_frames, vk::Device &device,
    vk::DescriptorPool descriptor_pool, vk::CommandPool &command_pool,
    vk::Queue &queue, const WorldRenderInfo &world_info,
    std::vector<Renderable> &renderables, ShadowCasters &shadowcasters)

    -> Texture2D::Impl *;
