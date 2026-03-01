#include "StaticDepthPipeline.hpp"

StaticDepthPipeline &StaticDepthPipeline::operator=(StaticDepthPipeline &&rhs) {
  std::swap(m_layout, rhs.m_layout);
  std::swap(m_pipeline, rhs.m_pipeline);
  std::swap(m_descriptor_layout, rhs.m_descriptor_layout);
  std::swap(m_descriptor_pool, rhs.m_descriptor_pool);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_camera_uniforms, rhs.m_camera_uniforms);
  return *this;
}

StaticDepthPipeline::StaticDepthPipeline(StaticDepthPipeline &&rhs) {
  std::swap(m_layout, rhs.m_layout);
  std::swap(m_pipeline, rhs.m_pipeline);
  std::swap(m_descriptor_layout, rhs.m_descriptor_layout);
  std::swap(m_descriptor_pool, rhs.m_descriptor_pool);
  std::swap(m_extent, rhs.m_extent);
  std::swap(m_camera_uniforms, rhs.m_camera_uniforms);
}

StaticDepthPipeline::StaticDepthPipeline(std::string_view name,
    Logger &logger, Render::Context::Impl *context, Presenter& presenter,
    vk::RenderPass &renderpass, StaticVertexPath vertex_path,
    StaticFragmentPath fragment_path, U32Extent extent, const bool debug_print)
    : m_extent{extent}
    , m_name{std::string(name)} {
  auto shaderstage_infos = create_shaderstage_infos(
      context->device.get(), VertexPath{vertex_path.get()},
      FragmentPath{fragment_path.get()});

  if (!shaderstage_infos) {
    std::string const msg = std::format(
        "[{}] could not load vertex/fragment sources {} / {}", m_name,
        vertex_path.get().string(), fragment_path.get().string());
    logger.fatal(std::source_location::current(), msg);
    throw std::runtime_error(msg);
  }

  std::array<vk::DynamicState, 2> dynamic_states{vk::DynamicState::eViewport,
                                                 vk::DynamicState::eScissor};

  auto pipelineDynamicStateCreateInfo =
      vk::PipelineDynamicStateCreateInfo{}
          .setFlags(vk::PipelineDynamicStateCreateFlags())
          .setDynamicStates(dynamic_states);

  const auto bindingDescriptions = binding_descriptions(VertexPosNormColorUV{});
  const auto attributeDescriptions =
      attribute_descriptions(VertexPosNormColorUV{});

  auto pipelineVertexInputStateCreateInfo =
      vk::PipelineVertexInputStateCreateInfo{}
          .setFlags(vk::PipelineVertexInputStateCreateFlags())
          .setVertexBindingDescriptions(bindingDescriptions)
          .setVertexAttributeDescriptions(attributeDescriptions);

  auto pipelineInputAssemblyStateCreateInfo =
      vk::PipelineInputAssemblyStateCreateInfo{}
          .setFlags(vk::PipelineInputAssemblyStateCreateFlags())
          .setPrimitiveRestartEnable(vk::False)
          .setTopology(vk::PrimitiveTopology::eTriangleList);

  const auto initial_viewport =
      vk::Viewport{}
          .setX(0.0f)
          .setY(0.0f)
          .setWidth(static_cast<float>(extent.width()))
          .setHeight(static_cast<float>(extent.height()))
          .setMinDepth(0.0f)
          .setMaxDepth(1.0f);

  auto initial_scissor =
      vk::Rect2D{}.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f));

  auto pipelineViewportStateCreateInfo =
      vk::PipelineViewportStateCreateInfo{}
          .setFlags(vk::PipelineViewportStateCreateFlags())
          .setViewports(initial_viewport)
          .setScissors(initial_scissor);

  auto pipelineRasterizationStateCreateInfo =
      vk::PipelineRasterizationStateCreateInfo{}
          .setFlags(vk::PipelineRasterizationStateCreateFlags())
          .setDepthClampEnable(false)
          .setRasterizerDiscardEnable(false)
          .setPolygonMode(vk::PolygonMode::eFill)
          // NOTE we cull front faces so we draw backfaces to the shadow depth.
          //      this helps with peter panning where shadows makes objects seem
          //      to float.
          .setCullMode(vk::CullModeFlagBits::eFront)
          .setFrontFace(vk::FrontFace::eCounterClockwise)
          .setDepthBiasEnable(false)
          .setDepthBiasConstantFactor(0.0f)
          .setDepthBiasClamp(0.0f)
          .setDepthBiasSlopeFactor(0.0f)
          .setLineWidth(1.0f);

  auto pipelineMultisampleStateCreateInfo =
      vk::PipelineMultisampleStateCreateInfo{}
          .setFlags(vk::PipelineMultisampleStateCreateFlags())
          .setSampleShadingEnable(false)
          .setRasterizationSamples(vk::SampleCountFlagBits::e1);

  auto pipelineColorBlendAttachmentState =
      vk::PipelineColorBlendAttachmentState{}
          .setBlendEnable(false)
          .setSrcColorBlendFactor(vk::BlendFactor::eOne)
          .setDstColorBlendFactor(vk::BlendFactor::eZero)
          .setColorBlendOp(vk::BlendOp::eAdd)
          .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
          .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
          .setAlphaBlendOp(vk::BlendOp::eAdd)
          .setColorWriteMask(
              vk::ColorComponentFlagBits::eR); // We only write to R channel

  auto pipelineColorBlendStateCreateInfo =
      vk::PipelineColorBlendStateCreateInfo{}
          .setFlags(vk::PipelineColorBlendStateCreateFlags())
          .setLogicOpEnable(false)
          .setLogicOp(vk::LogicOp::eNoOp)
          .setAttachments(pipelineColorBlendAttachmentState)
          .setBlendConstants({1.0f, 1.0f, 1.0f, 1.0f});

  const auto push_constant_range =
      vk::PushConstantRange{}
          .setOffset(0)
          .setSize(sizeof(PushConstants))
          .setStageFlags(vk::ShaderStageFlagBits::eVertex);

  if (sizeof(PushConstants) > 128) {
    logger.warn(
        std::source_location::current(),
        std::format(
            "PushConstant size={} is larger than minimum supported (128)"
            "This can cause compatability issues on some devices",
            sizeof(PushConstants)));
  }

  const auto layout_binding =
      vk::DescriptorSetLayoutBinding{}
          .setStageFlags(vk::ShaderStageFlagBits::eVertex)
          .setBinding(0)
          .setDescriptorCount(1)
          .setDescriptorType(vk::DescriptorType::eUniformBuffer);

  const auto set_info = vk::DescriptorSetLayoutCreateInfo{}
                            .setFlags(vk::DescriptorSetLayoutCreateFlags())
                            .setBindingCount(1)
                            .setBindings(layout_binding);

  m_descriptor_layout =
      context->device.get().createDescriptorSetLayoutUnique(set_info, nullptr);

  auto pipelineLayoutCreateInfo =
      vk::PipelineLayoutCreateInfo{}
          .setFlags(vk::PipelineLayoutCreateFlags())
          .setSetLayouts(m_descriptor_layout.get())
          .setPushConstantRanges(push_constant_range);

  m_layout = context->device.get().createPipelineLayoutUnique(
      pipelineLayoutCreateInfo);

  logger.info(std::source_location::current(), "Created Pipeline Layout");

  auto depth_stencil_state_info = vk::PipelineDepthStencilStateCreateInfo{}
                                      .setDepthTestEnable(true)
                                      .setDepthWriteEnable(true)
                                      .setDepthCompareOp(vk::CompareOp::eLess)
                                      .setDepthBoundsTestEnable(false)
                                      .setMinDepthBounds(0.0f)
                                      .setMaxDepthBounds(1.0f)
                                      .setStencilTestEnable(false);

  auto graphicsPipelineCreateInfo =
      vk::GraphicsPipelineCreateInfo{}
          .setFlags(vk::PipelineCreateFlags())
          .setStages(shaderstage_infos.value().create_info)
          .setPVertexInputState(&pipelineVertexInputStateCreateInfo)
          .setPInputAssemblyState(&pipelineInputAssemblyStateCreateInfo)
          .setPTessellationState(nullptr)
          .setPViewportState(&pipelineViewportStateCreateInfo)
          .setPRasterizationState(&pipelineRasterizationStateCreateInfo)
          .setPMultisampleState(&pipelineMultisampleStateCreateInfo)
          .setPDepthStencilState(&depth_stencil_state_info)
          .setPColorBlendState(&pipelineColorBlendStateCreateInfo)
          .setPDynamicState(&pipelineDynamicStateCreateInfo)
          .setLayout(m_layout.get())
          .setRenderPass(renderpass);

  vk::ResultValue<vk::UniquePipeline> result =
      context->device.get().createGraphicsPipelineUnique(
          nullptr, graphicsPipelineCreateInfo);

  switch (result.result) {
  case vk::Result::eSuccess:
    break;
  case vk::Result::ePipelineCompileRequiredEXT:
    logger.error(std::source_location::current(),
                 "Creating pipeline error: PipelineCompileRequiredEXT");
  default:
    logger.error(std::source_location::current(),
                 "Creating pipeline error: Unknown invalid Result state");
  }

  m_pipeline = std::move(result.value);
  logger.info(std::source_location::current(), "Created Pipeline");

  // TODO: 10 is just random, we need to calculate the exact number of sets
  std::array<vk::DescriptorPoolSize, 1> sizes{
      vk::DescriptorPoolSize{}
          .setType(vk::DescriptorType::eUniformBuffer)
          .setDescriptorCount(10),
  };

  const auto pool_info =
      vk::DescriptorPoolCreateInfo{}
          .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
          .setMaxSets(10)
          .setPoolSizes(sizes);

  m_descriptor_pool =
      context->device.get().createDescriptorPoolUnique(pool_info, nullptr);
  logger.info(std::source_location::current(), "Created Descriptor Pool");

  for (CameraUniform &camera_uniform : m_camera_uniforms) {
    camera_uniform.uniform = UniformMemoryDirectWrite<CameraUniformData>(
        context->physical_device, context->device.get(), 1);

    const auto allocate_info = vk::DescriptorSetAllocateInfo{}
                                   .setDescriptorPool(m_descriptor_pool.get())
                                   .setDescriptorSetCount(1)
                                   .setSetLayouts(m_descriptor_layout.get());

    auto sets =
        context->device.get().allocateDescriptorSetsUnique(allocate_info);
    if (sets.size() != 1) {
      logger.error(std::source_location::current(),
                   "This system only allows handling 1 set per frame in flight"
                   "\n if you want more sets find another way to store them..");
    }

    camera_uniform.set = std::move(sets[0]);

    const auto write_descriptor =
        vk::WriteDescriptorSet{}
            .setDstBinding(0)
            .setDstSet(camera_uniform.set.get())
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            // here images can be set aswell
            .setBufferInfo(camera_uniform.uniform.buffer_info());

    const uint32_t write_count = 1;
    const uint32_t copy_count = 0;
    context->device.get().updateDescriptorSets(write_count, &write_descriptor,
                                               copy_count, nullptr);
  }

  logger.info(std::source_location::current(),
              "Created OrthoGraphic Depth Pipeline!");
}

void StaticDepthPipeline::record(Logger *logger, vk::Device &device,
                                 MeshCache &mesh_cache,
                                 CurrentFlightFrame current_flightframe,
                                 vk::CommandBuffer &commandbuffer,
                                 CameraUniformData camera_data,
                                 std::vector<ShadowRenderable> &renderables) {
  commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             m_pipeline.get());

  m_camera_uniforms[*current_flightframe].uniform.write(device, &camera_data);

  std::array<vk::DescriptorSet, 1> uniform_sets{
      m_camera_uniforms[*current_flightframe].set.get()};

  commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                   m_layout.get(), 0, uniform_sets.size(),
                                   uniform_sets.data(), 0, nullptr);

  for (auto renderable : renderables) {

    if (auto *material = std::get_if<MaterialRenderable>(&renderable)) {
      if (!material->has_shadow)
        continue;

      PushConstants push{};
      push.model = material->model;
      const uint32_t push_offset = 0;
      commandbuffer.pushConstants(m_layout.get(),
                                  vk::ShaderStageFlagBits::eVertex, push_offset,
                                  sizeof(push), &push);

      const uint32_t firstBinding = 0;
      const uint32_t bindingCount = 1;
      std::array<vk::DeviceSize, bindingCount> offsets = {0};

      TexturedMesh *mesh = mesh_cache.get(material->mesh.value());
      uint32_t vertex_length = mesh->vertexbuffer.impl->length;
      std::array<vk::Buffer, bindingCount> buffers{
          mesh->vertexbuffer.impl->buffer.get()};

      commandbuffer.bindVertexBuffers(firstBinding, bindingCount,
                                      buffers.data(), offsets.data());

      const uint32_t instanceCount = 1;
      const uint32_t firstVertex = 0;
      const uint32_t firstInstance = 0;
      commandbuffer.draw(vertex_length, instanceCount, firstVertex,
                         firstInstance);
    }
  }
}
