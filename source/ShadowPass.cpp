#include "ShadowPass.hpp"

#include "DescriptorPoolImpl.hpp"
#include "StaticDepthPipeline.hpp"

ShadowPassTexture::ShadowPassTexture(Render::Context::Impl *context,
                                     DescriptorPool::Impl *descriptor_pool,
                                     U32Extent extent)

{
  texture = Texture2D(std::make_unique<Texture2D::Impl>(
      RenderTargetTexture, context, extent, TextureFormat::R32Sfloat));

  auto transition_to_transfer_src = [&](vk::CommandBuffer &commandbuffer) {
    transition_image_for_color_override(texture.impl->allocated.image.get(),
                                        commandbuffer);
  };

  with_buffer_submit(context->device.get(), context->commandpool.get(),
                     context->graphics_queue(), transition_to_transfer_src);

  const auto subresourceRange =
      vk::ImageSubresourceRange{}
          .setAspectMask(vk::ImageAspectFlagBits::eColor)
          .setBaseMipLevel(0)
          .setLevelCount(1)
          .setBaseArrayLayer(0)
          .setLayerCount(1);

  const auto componentMapping = vk::ComponentMapping{}
                                    .setR(vk::ComponentSwizzle::eIdentity)
                                    .setG(vk::ComponentSwizzle::eIdentity)
                                    .setB(vk::ComponentSwizzle::eIdentity)
                                    .setA(vk::ComponentSwizzle::eIdentity);

  const auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
                                       .setImage(texture.impl->image())
                                       .setFormat(texture.impl->format)
                                       .setSubresourceRange(subresourceRange)
                                       .setViewType(vk::ImageViewType::e2D)
                                       .setComponents(componentMapping);
  view = context->device.get().createImageViewUnique(imageViewCreateInfo);

  const auto properties = context->physical_device.getProperties();
  const auto has_anisotropy = true; // TODO: set this from device
  const auto max_anisotropy =
      has_anisotropy ? std::min(4.0f, properties.limits.maxSamplerAnisotropy)
                     : 1.0f;

  vk::Filter const filter = vk::Filter::eLinear;
  vk::SamplerMipmapMode const mipmap_filter = vk::SamplerMipmapMode::eLinear;

  const auto sampler_info =
      vk::SamplerCreateInfo{}
          .setMagFilter(filter)
          .setMinFilter(filter)
          .setAddressModeU(vk::SamplerAddressMode::eRepeat)
          .setAddressModeV(vk::SamplerAddressMode::eRepeat)
          .setAddressModeW(vk::SamplerAddressMode::eRepeat)
          .setAnisotropyEnable(has_anisotropy)
          .setMaxAnisotropy(max_anisotropy)
          .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
          .setUnnormalizedCoordinates(false)
          .setCompareEnable(false)
          .setCompareOp(vk::CompareOp::eAlways)
          .setMipmapMode(mipmap_filter)
          .setMipLodBias(0.0f)
          .setMinLod(0.0f)
          .setMaxLod(0.0f);
  sampler = context->device.get().createSamplerUnique(sampler_info);

  std::array<vk::DescriptorSetLayoutBinding, 1> const layout_bindings{
      vk::DescriptorSetLayoutBinding{}
          .setStageFlags(vk::ShaderStageFlagBits::eFragment)
          .setBinding(0)
          .setDescriptorCount(1)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
  };
  auto layout_createinfo = vk::DescriptorSetLayoutCreateInfo{}
                               .setFlags(vk::DescriptorSetLayoutCreateFlags())
                               .setBindings(layout_bindings);

  descriptorset_layout = context->device.get().createDescriptorSetLayoutUnique(
      layout_createinfo, nullptr);

  const auto descriptorset_allocate_info =
      vk::DescriptorSetAllocateInfo{}
          .setDescriptorPool(descriptor_pool->descriptor_pool.get())
          .setDescriptorSetCount(1)
          .setSetLayouts(descriptorset_layout.get());

  auto createdsets = context->device.get().allocateDescriptorSetsUnique(
      descriptorset_allocate_info);
  descriptorset = std::move(createdsets[0]);

  const auto descriptorimage_info =
      vk::DescriptorImageInfo{}
          .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
          .setImageView(view.get())
          .setSampler(sampler.get());

  const std::array<vk::WriteDescriptorSet, 1> descriptorwrite{
      vk::WriteDescriptorSet{}
          .setDstBinding(0)
          .setDstArrayElement(0)
          .setDstSet(descriptorset.get())
          .setDescriptorCount(1)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
          .setImageInfo(descriptorimage_info),
  };

  context->device.get().updateDescriptorSets(
      descriptorwrite.size(), descriptorwrite.data(), 0, nullptr);
}

ShadowPassTexture::ShadowPassTexture(ShadowPassTexture &&rhs) {
  std::swap(texture, rhs.texture);
  std::swap(sampler, rhs.sampler);
  std::swap(state, rhs.state);
  std::swap(view, rhs.view);
  std::swap(descriptorset, rhs.descriptorset);
}

ShadowPassTexture &ShadowPassTexture::operator=(ShadowPassTexture &&rhs) {
  std::swap(texture, rhs.texture);
  std::swap(sampler, rhs.sampler);
  std::swap(state, rhs.state);
  std::swap(view, rhs.view);
  std::swap(descriptorset, rhs.descriptorset);
  return *this;
}

OrthographicShadowPass::OrthographicShadowPass(
    Logger &logger, Render::Context::Impl *context, Presenter &presenter,
    DescriptorPool::Impl *descriptor_pool, U32Extent extent,
    StaticVertexPath static_vertex_path,
    StaticFragmentPath static_fragment_path,
    AnimatedVertexPath animated_vertex_path,
    AnimatedFragmentPath animated_fragment_path, const bool debug_print)
    : GenericShadowPass("OrthoGraphicShadowPass", context, logger, presenter,
                        descriptor_pool, extent, static_vertex_path,
                        static_fragment_path, animated_vertex_path,
                        animated_fragment_path, debug_print) {}

void OrthographicShadowPass::record(
    Render::Context::Impl *context, Logger *logger, vk::Device &device,
    MeshCache &mesh_cache, CurrentFlightFrame current_flightframe,
    vk::CommandBuffer &commandbuffer,
    std::optional<CameraUniformData> camera_data,
    std::vector<ShadowRenderable> &renderables) {
  GenericShadowPass::record(context, logger, device, mesh_cache,
                            current_flightframe, commandbuffer, camera_data,
                            renderables);
}

auto OrthographicShadowPass::get_shadowtexture(
    CurrentFlightFrame current_flightframe) -> ShadowPassTexture & {
  return GenericShadowPass::get_shadowtexture(current_flightframe);
}

PerspectiveShadowPass::PerspectiveShadowPass(
    Logger &logger, Render::Context::Impl *context, Presenter &presenter,
    DescriptorPool::Impl *descriptor_pool, U32Extent extent,
    StaticVertexPath static_vertex_path,
    StaticFragmentPath static_fragment_path,
    AnimatedVertexPath animated_vertex_path,
    AnimatedFragmentPath animated_fragment_path, const bool debug_print)
    : GenericShadowPass("PerspectiveShadowPass", context, logger, presenter,
                        descriptor_pool, extent, static_vertex_path,
                        static_fragment_path, animated_vertex_path,
                        animated_fragment_path, debug_print) {}

void PerspectiveShadowPass::record(Render::Context::Impl *context,
                                   Logger *logger, vk::Device &device,
                                   MeshCache &mesh_cache,
                                   CurrentFlightFrame current_flightframe,
                                   vk::CommandBuffer &commandbuffer,
                                   std::optional<CameraUniformData> camera_data,
                                   std::vector<ShadowRenderable> &renderables) {
  GenericShadowPass::record(context, logger, device, mesh_cache,
                            current_flightframe, commandbuffer, camera_data,
                            renderables);
}

auto PerspectiveShadowPass::get_shadowtexture(
    CurrentFlightFrame current_flightframe) -> ShadowPassTexture & {
  return GenericShadowPass::get_shadowtexture(current_flightframe);
}

GenericShadowPass &GenericShadowPass::operator=(GenericShadowPass &&rhs) {
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_name, rhs.m_name);
  std::swap(m_renderpass, rhs.m_renderpass);
  std::swap(m_framestextures, rhs.m_framestextures);
  std::swap(m_static_pipeline, rhs.m_static_pipeline);
  std::swap(m_animated_pipeline, rhs.m_animated_pipeline);
  return *this;
}

GenericShadowPass::GenericShadowPass(GenericShadowPass &&rhs) {
  *this = std::move(rhs);
}

GenericShadowPass::GenericShadowPass(
    std::string_view name, Render::Context::Impl *context, Logger &logger,
    Presenter &presenter, DescriptorPool::Impl *descriptor_pool,
    U32Extent extent, StaticVertexPath static_vertex_path,
    StaticFragmentPath static_fragment_path,
    AnimatedVertexPath animated_vertex_path,
    AnimatedFragmentPath animated_fragment_path, const bool debug_print)
    : m_name{std::string(name)}, m_extent{extent} {
  auto constexpr color_format = vk::Format::eR32Sfloat;
  auto constexpr depth_format = vk::Format::eD32Sfloat;
  auto constexpr colorComponentFlags(vk::ColorComponentFlagBits::eR);
  auto const frames_in_flight =
      MaxFlightFrames{presenter.max_frames_in_flight};

  const auto color_attachment =
      vk::AttachmentDescription{}
          .setFlags(vk::AttachmentDescriptionFlags())
          .setFormat(color_format)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eStore)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          // NOTE these are important, as they determine the layout of the image
          // before and after the renderpass
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

  const auto depth_attachment =
      vk::AttachmentDescription{}
          .setFlags(vk::AttachmentDescriptionFlags())
          .setFormat(depth_format)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          // NOTE these are important, as they determine the layout of the image
          // before and after the renderpass
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

  const auto color_reference =
      vk::AttachmentReference{}.setAttachment(0).setLayout(
          vk::ImageLayout::eColorAttachmentOptimal);

  const auto depth_reference =
      vk::AttachmentReference{}.setAttachment(1).setLayout(
          vk::ImageLayout::eDepthStencilAttachmentOptimal);

  auto subpass = vk::SubpassDescription{}
                     .setFlags(vk::SubpassDescriptionFlags())
                     .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                     .setInputAttachments({})
                     .setResolveAttachments({})
                     .setColorAttachments(color_reference)
                     .setPDepthStencilAttachment(&depth_reference);

  auto color_depth_dependency =
      vk::SubpassDependency{}
          .setSrcSubpass(vk::SubpassExternal)
          .setDstSubpass(0)
          .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                           vk::PipelineStageFlagBits::eEarlyFragmentTests)
          .setSrcAccessMask(vk::AccessFlags())
          .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                           vk::PipelineStageFlagBits::eEarlyFragmentTests)
          .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                            vk::AccessFlagBits::eDepthStencilAttachmentWrite);

  std::array<vk::AttachmentDescription, 2> attachments{color_attachment,
                                                       depth_attachment};
  std::array<vk::SubpassDependency, 1> dependencies{color_depth_dependency};
  auto renderPassCreateInfo = vk::RenderPassCreateInfo{}
                                  .setFlags(vk::RenderPassCreateFlags())
                                  .setAttachments(attachments)
                                  .setDependencies(dependencies)
                                  .setSubpasses(subpass);

  m_renderpass =
      context->device.get().createRenderPassUnique(renderPassCreateInfo);
  context->logger.info(std::source_location::current(),
                       "Created Shadowmap Render Pass!");

  for (FrameTextures &textures : m_framestextures) {
    /* Setup the rendertarget and its view for the render pass
     */
    textures.colorbuffer =
        ShadowPassTexture(context, descriptor_pool, m_extent);

    textures.colorbuffer_view = textures.colorbuffer.texture.impl->create_view(
        context, vk::ImageAspectFlagBits::eColor);

    /* Setup the depthbuffer and its view for the render pass
     */
    textures.depthbuffer = Texture2D(std::make_unique<Texture2D::Impl>(
        DepthBufferTexture, context, m_extent));
    /* Setup the depthbuffer view
     */
    textures.depthbuffer_view = textures.depthbuffer.impl->create_view(
        context, vk::ImageAspectFlagBits::eDepth);

    /* Setup the FrameBuffer
     */
    std::array<vk::ImageView, 2> attachments{
        textures.colorbuffer_view.get(),
        textures.depthbuffer_view.get(),
    };
    auto framebufferCreateInfo = vk::FramebufferCreateInfo{}
                                     .setFlags(vk::FramebufferCreateFlags())
                                     .setAttachments(attachments)
                                     .setWidth(m_extent.width())
                                     .setHeight(m_extent.height())
                                     .setRenderPass(m_renderpass.get())
                                     .setLayers(1);

    textures.framebuffer =
        context->device.get().createFramebufferUnique(framebufferCreateInfo);
  }

  context->logger.info(std::source_location::current(),
                       "Created Shadowpass FramePasses!");

  m_static_pipeline =
      StaticDepthPipeline(m_name + "::StaticDepthPipeline", logger, context,
                          presenter, m_renderpass.get(), static_vertex_path,
                          static_fragment_path, m_extent, debug_print);

  m_animated_pipeline =
      AnimatedDepthPipeline(m_name + "::AnimatedDepthPipeline", logger, context,
                            presenter, m_renderpass.get(), animated_vertex_path,
                            animated_fragment_path, m_extent, debug_print);
}

void GenericShadowPass::record(Render::Context::Impl *context, Logger *logger,
                               vk::Device &device, MeshCache &mesh_cache,
                               CurrentFlightFrame current_flightframe,
                               vk::CommandBuffer &commandbuffer,
                               std::optional<CameraUniformData> camera_data,
                               std::vector<ShadowRenderable> &renderables) {
  const auto render_area =
      vk::Rect2D{}
          .setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
          .setExtent(vk::Extent2D{m_extent.width(), m_extent.height()});

  std::array<vk::ClearValue, 2> clearvalues{
      vk::ClearValue{}.setColor({1.0f, 1.0f, 1.0f, 1.0f}),
      vk::ClearValue{}.setDepthStencil({1.0f, 0}),
  };

  const auto renderPassInfo =
      vk::RenderPassBeginInfo{}
          .setRenderPass(m_renderpass.get())
          .setFramebuffer(
              m_framestextures[current_flightframe.get()].framebuffer.get())
          .setRenderArea(render_area)
          .setClearValues(clearvalues);

  commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

  // if there is no shadow camera we end the renderpass immediately
  // we still need to begin/end it so that the implicit shadow texture
  // commands are applied, and allows us to bind it to the geometry pipeline.
  if (!camera_data.has_value()) {
    commandbuffer.endRenderPass();
    return;
  }

  std::array<vk::Viewport, 1> const viewports{vk::Viewport{}
                                                  .setX(0.0f)
                                                  .setY(0.0f)
                                                  .setWidth(m_extent.width())
                                                  .setHeight(m_extent.height())
                                                  .setMinDepth(0.0f)
                                                  .setMaxDepth(1.0f)};
  uint32_t const viewport_start = 0;
  commandbuffer.setViewport(viewport_start, viewports);

  std::array<vk::Rect2D, 1> const scissors{
      vk::Rect2D{}
          .setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
          .setExtent(vk::Extent2D{m_extent.width(), m_extent.height()}),
  };
  const uint32_t scissor_start = 0;
  commandbuffer.setScissor(scissor_start, scissors);

  StaticDepthPipeline::CameraUniformData static_depth_camera_data;
  static_depth_camera_data.view = camera_data.value().view;
  static_depth_camera_data.proj = camera_data.value().proj;
  m_static_pipeline.record(logger, device, mesh_cache, current_flightframe,
                           commandbuffer, static_depth_camera_data,
                           renderables);

  AnimatedDepthPipeline::CameraUniformData animated_depth_camera_data;
  animated_depth_camera_data.view = camera_data.value().view;
  animated_depth_camera_data.proj = camera_data.value().proj;
  m_animated_pipeline.record(context, logger, device, mesh_cache,
                             current_flightframe, commandbuffer,
                             animated_depth_camera_data, renderables);

  commandbuffer.endRenderPass();
}

auto GenericShadowPass::get_shadowtexture(
    CurrentFlightFrame current_flightframe) -> ShadowPassTexture & {
  return m_framestextures[current_flightframe.get()].colorbuffer;
}
