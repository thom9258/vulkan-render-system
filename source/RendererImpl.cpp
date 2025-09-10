#include "RendererImpl.hpp"

#include "MaterialPipeline.hpp"

auto create_texture_view(vk::Device& device,
						 Texture2D& texture,
						 const vk::ImageAspectFlags aspect)
	-> vk::UniqueImageView
{
	const auto subresourceRange = vk::ImageSubresourceRange{}
		.setAspectMask(aspect)
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

	return device.createImageViewUnique(imageViewCreateInfo);
}

void sort_renderable(Logger* logger,
					 SortedRenderables* sorted,
					 Renderable renderable)
{
	if (auto p = std::get_if<NormColorRenderable>(&renderable))
		sorted->normcolors.push_back(*p);
	else if (auto p = std::get_if<WireframeRenderable>(&renderable))
		sorted->wireframes.push_back(*p);
	else if (auto p = std::get_if<BaseTextureRenderable>(&renderable))
		sorted->basetextures.push_back(*p);
	else if (auto p = std::get_if<MaterialRenderable>(&renderable))
		sorted->materialrenderables.push_back(*p);
	else {
		logger->warn(std::source_location::current(),
					 "Found unknown Renderable that can not be sorted and drawn");
	}
}

ShadowPass::ShadowPass(Logger& logger,
					   Render::Context::Impl* context,
					   Presenter::Impl* presenter,
					   U32Extent extent,
					   const std::filesystem::path shader_root_path,
					   const bool debug_print)
	: m_extent{extent}
{
	auto constexpr color_format = vk::Format::eR32Sfloat;
	auto constexpr depth_format = vk::Format::eD32Sfloat;
    auto constexpr colorComponentFlags(vk::ColorComponentFlagBits::eR);
	const std::string pipeline_name = "ShadowPass";
	const std::string vertexshader_name = "OrthographicDepth.vert.spv";
	const std::string fragmentshader_name = "OrthographicDepth.frag.spv";
	auto const frames_in_flight = MaxFlightFrames{presenter->max_frames_in_flight};

    const auto color_attachment = vk::AttachmentDescription{}
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(color_format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		// NOTE these are important, as they determine the layout of the image before and after
		// the renderpass
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eTransferSrcOptimal);

    const auto depth_attachment = vk::AttachmentDescription{}
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(depth_format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		// NOTE these are important, as they determine the layout of the image before and after
		// the renderpass
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	const auto color_reference = vk::AttachmentReference{}
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	const auto depth_reference = vk::AttachmentReference{}
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	
    auto subpass = vk::SubpassDescription{}
		.setFlags(vk::SubpassDescriptionFlags())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setInputAttachments({})
		.setResolveAttachments({})
		.setColorAttachments(color_reference)
		.setPDepthStencilAttachment(&depth_reference);
	
	auto color_depth_dependency = vk::SubpassDependency{}
		.setSrcSubpass(vk::SubpassExternal)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
						 | vk::PipelineStageFlagBits::eEarlyFragmentTests)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
						 | vk::PipelineStageFlagBits::eEarlyFragmentTests)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite
						  | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

	
	std::array<vk::AttachmentDescription, 2> attachments {
		color_attachment,
		depth_attachment
	};
	std::array<vk::SubpassDependency, 1> dependencies {
		color_depth_dependency
	};
    auto renderPassCreateInfo = vk::RenderPassCreateInfo{}
		.setFlags(vk::RenderPassCreateFlags())
		.setAttachments(attachments)
		.setDependencies(dependencies)
		.setSubpasses(subpass);

	m_renderpass = context->device.get().createRenderPassUnique(renderPassCreateInfo);
	context->logger.info(std::source_location::current(),
						 "Created Shadowmap Render Pass!");

	for (FrameTextures& textures: m_framestextures) {
		/* Setup the rendertarget and its view for the render pass
		 */
		textures.colorbuffer = Texture2D(std::make_unique<Texture2D::Impl>(RenderTargetTexture,
														 context,
														 m_extent,
														 vkformat_to_textureformat(color_format)));
		
		auto transition_to_transfer_src = [&] (vk::CommandBuffer& commandbuffer)
		{
			textures.colorbuffer.impl->layout =
				transition_image_for_color_override(textures.colorbuffer.impl->allocated.image.get(),
													commandbuffer);
		};
		
		with_buffer_submit(context->device.get(),
						   context->commandpool.get(),
						   context->graphics_queue(),
						   transition_to_transfer_src);
		
		textures.colorbuffer_view
			= textures.colorbuffer.impl->create_view(context,
													 vk::ImageAspectFlagBits::eColor);

		/* Setup the depthbuffer and its view for the render pass
		 */
		textures.depthbuffer = Texture2D(std::make_unique<Texture2D::Impl>(DepthBufferTexture,
																		   context,
																		   m_extent));
		/* Setup the depthbuffer view
		 */
		textures.depthbuffer_view =
			textures.depthbuffer.impl->create_view(context,
												   vk::ImageAspectFlagBits::eDepth);
		
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
	
	const auto vertex_path = VertexPath{shader_root_path / vertexshader_name};
	const auto fragment_path = FragmentPath{shader_root_path / fragmentshader_name};
	auto shaderstage_infos = create_shaderstage_infos(context->device.get(),
													  vertex_path,
													  fragment_path);

	if (!shaderstage_infos) {
		std::string const msg = std::format("{} could not load vertex/fragment sources {} / {}",
											pipeline_name,
											vertexshader_name,
											fragmentshader_name);
		logger.fatal(std::source_location::current(), msg);
		throw std::runtime_error(msg);
	}

    std::array<vk::DynamicState, 2> dynamic_states{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	auto pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
		.setFlags(vk::PipelineDynamicStateCreateFlags())
		.setDynamicStates(dynamic_states);
	
	const auto bindingDescriptions = binding_descriptions(VertexPosNormColorUV{});
	const auto attributeDescriptions = attribute_descriptions(VertexPosNormColorUV{});
	
	auto pipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{}
		.setFlags(vk::PipelineVertexInputStateCreateFlags())
		.setVertexBindingDescriptions(bindingDescriptions)
		.setVertexAttributeDescriptions(attributeDescriptions);

    auto pipelineInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{}
		.setFlags(vk::PipelineInputAssemblyStateCreateFlags())
		.setPrimitiveRestartEnable(vk::False)
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	
	 const auto initial_viewport = vk::Viewport{}
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(m_extent.width()))
		.setHeight(static_cast<float>(m_extent.height()))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	
	auto initial_scissor = vk::Rect2D{}
		.setOffset(vk::Offset2D{}.setX(0.0f)
				                 .setY(0.0f));

	
    auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
		.setFlags(vk::PipelineViewportStateCreateFlags())
		.setViewports(initial_viewport)
		.setScissors(initial_scissor);
	
    auto pipelineRasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{}
		.setFlags(vk::PipelineRasterizationStateCreateFlags())
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f)
		.setLineWidth(1.0f);

    auto pipelineMultisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo{}
		.setFlags(vk::PipelineMultisampleStateCreateFlags())
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	auto pipelineColorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{}
		.setBlendEnable(false)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(colorComponentFlags);
	
	auto pipelineColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo{}
		.setFlags(vk::PipelineColorBlendStateCreateFlags())
		.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eNoOp)
		.setAttachments(pipelineColorBlendAttachmentState)
		.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });

	const auto push_constant_range = vk::PushConstantRange{}
		.setOffset(0)
		.setSize(sizeof(RenderPipeline::PushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	if (sizeof(RenderPipeline::PushConstants) > 128) {
		logger.warn(std::source_location::current(), 
					std::format("PushConstant size={} is larger than minimum supported (128)"
								"This can cause compatability issues on some devices",
								sizeof(RenderPipeline::PushConstants)));
	}

	const auto layout_binding = vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer);
	
	const auto set_info = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindingCount(1)
		.setBindings(layout_binding);
	
	m_pipeline.descriptor_layout =
		context->device.get().createDescriptorSetLayoutUnique(set_info, nullptr);
	
    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(m_pipeline.descriptor_layout.get())
		.setPushConstantRanges(push_constant_range);

	m_pipeline.layout = 
		context->device.get().createPipelineLayoutUnique(pipelineLayoutCreateInfo);
	
	logger.info(std::source_location::current(), "Created Pipeline Layout");
	
	auto depth_stencil_state_info = vk::PipelineDepthStencilStateCreateInfo{}
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(false)
		.setMinDepthBounds(0.0f)
		.setMaxDepthBounds(1.0f)
		.setStencilTestEnable(false);
	
	auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
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
		.setLayout(m_pipeline.layout.get())
		.setRenderPass(m_renderpass.get());

	vk::ResultValue<vk::UniquePipeline> result =
		context->device.get().createGraphicsPipelineUnique(nullptr,
											graphicsPipelineCreateInfo);
	
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
	
	m_pipeline.pipeline = std::move(result.value);
	logger.info(std::source_location::current(),
				"Created Pipeline");
	
	std::array<vk::DescriptorPoolSize, 1> sizes {
		vk::DescriptorPoolSize{}
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(10),
	};
	
	const auto pool_info = vk::DescriptorPoolCreateInfo{}
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(10)
		.setPoolSizes(sizes);
	
	m_pipeline.descriptor_pool = context->device.get().createDescriptorPoolUnique(pool_info, nullptr);
	logger.info(std::source_location::current(),
				"Created Descriptor Pool");

	for (uint32_t i = 0; i < frames_in_flight.get(); i++) {
		m_pipeline.descriptor_memories
			.push_back(allocate_memory(context->physical_device,
									   context->device.get(),
									   sizeof(CameraUniformData),
									   vk::BufferUsageFlagBits::eTransferSrc
									   | vk::BufferUsageFlagBits::eUniformBuffer,
									   // Host Visible and Coherent allows direct
									   // writes into the buffers without sync issues.
									   vk::MemoryPropertyFlagBits::eHostVisible
									   | vk::MemoryPropertyFlagBits::eHostCoherent));

		const auto allocate_info = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(m_pipeline.descriptor_pool.get())
			.setDescriptorSetCount(1)
			.setSetLayouts(m_pipeline.descriptor_layout.get());
		
		auto sets = context->device.get().allocateDescriptorSetsUnique(allocate_info);
		if (sets.size() != 1) {
			logger.error(std::source_location::current(),
						 "This system only allows handling 1 set per frame in flight"
						 "\n if you want more sets find another way to store them..");
		}
		
		m_pipeline.descriptor_sets.push_back(std::move(sets[0]));

		const auto buffer_info = vk::DescriptorBufferInfo{}
			.setBuffer(m_pipeline.descriptor_memories[i].buffer.get())
			.setOffset(0)
			.setRange(sizeof(CameraUniformData));
		
		const auto write_descriptor = vk::WriteDescriptorSet{}
			.setDstBinding(0)
			.setDstSet(m_pipeline.descriptor_sets[i].get())
			.setDstArrayElement(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			// here images can be set aswell
			.setBufferInfo(buffer_info);
		
		const uint32_t write_count = 1;
		const uint32_t copy_count = 0;
		context->device.get().updateDescriptorSets(write_count,
												   &write_descriptor,
												   copy_count,
												   nullptr);
	}

	logger.info(std::source_location::current(),
				std::format("Allocated {} descriptor sets for {} frames in flight",
							 m_pipeline.descriptor_sets.size(),
							 frames_in_flight.get()));

	logger.info(std::source_location::current(),
				"Created Shadowpass RenderPipeline!");
}

void ShadowPass::record(Logger* logger,
						vk::Device& device,
						CurrentFlightFrame current_flightframe,
						vk::CommandBuffer& commandbuffer,
						CameraUniformData camera_data,
						std::vector<MaterialRenderable>& renderables)
{
 	const auto render_area = vk::Rect2D{}
		.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
		.setExtent(vk::Extent2D{m_extent.width(), m_extent.height()});

	std::array<vk::ClearValue, 2> clearvalues{
		vk::ClearValue{}.setColor({1.0f, 1.0f, 1.0f, 1.0f}),
		vk::ClearValue{}.setDepthStencil({1.0f, 0}),
	};
	
	const auto renderPassInfo = vk::RenderPassBeginInfo{}
		.setRenderPass(m_renderpass.get())
		.setFramebuffer(m_framestextures[current_flightframe.get()].framebuffer.get())
		.setRenderArea(render_area)
		.setClearValues(clearvalues);
	
	commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
	
	std::array<vk::Viewport, 1> const viewports{
		vk::Viewport{}
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(m_extent.width())
		.setHeight(m_extent.height())
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f)
	};
	uint32_t const viewport_start = 0;
	commandbuffer.setViewport(viewport_start, viewports);
	
	std::array<vk::Rect2D, 1> const scissors{
		vk::Rect2D{}
		.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
		.setExtent(vk::Extent2D{m_extent.width(), m_extent.height()}),
	};
	const uint32_t scissor_start = 0;
	commandbuffer.setScissor(scissor_start, scissors);
	

	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
							   m_pipeline.pipeline.get());

	copy_to_allocated_memory(device,
							 m_pipeline.descriptor_memories[current_flightframe.get()],
							 reinterpret_cast<void*>(&camera_data),
							 sizeof(camera_data));

	const uint32_t first_set = 0;
	const uint32_t descriptor_set_count = 1;
	auto descriptor_sets = &(m_pipeline.descriptor_sets[current_flightframe.get()].get());
	const uint32_t dynamic_offset_count = 0;
	const uint32_t* dynamic_offsets = nullptr;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									 m_pipeline.layout.get(),
									 first_set,
									 descriptor_set_count,
									 descriptor_sets,
									 dynamic_offset_count,
									 dynamic_offsets);


	for (auto renderable: renderables) {
		RenderPipeline::PushConstants push{};
		push.model = renderable.model;
		const uint32_t push_offset = 0;
		commandbuffer.pushConstants(m_pipeline.layout.get(),
									vk::ShaderStageFlagBits::eVertex,
									push_offset,
									sizeof(push),
									&push);
		
		const uint32_t firstBinding = 0;
		const uint32_t bindingCount = 1;
		std::array<vk::DeviceSize, bindingCount> offsets = {0};
		std::array<vk::Buffer, bindingCount> buffers {
			renderable.mesh->vertexbuffer.impl->buffer.get(),
		};
		commandbuffer.bindVertexBuffers(firstBinding,
										bindingCount,
										buffers.data(),
										offsets.data());
		
		
		const uint32_t instanceCount = 1;
		const uint32_t firstVertex = 0;
		const uint32_t firstInstance = 0;
		commandbuffer.draw(renderable.mesh->vertexbuffer.impl->length,
						   instanceCount,
						   firstVertex,
						   firstInstance);
	}

	commandbuffer.endRenderPass();
}

auto create_geometry_pass(Render::Context::Impl* context,
						  vk::Extent2D render_extent,
						  const uint32_t frames_in_flight,
						  const bool debug_print)
	-> GeometryPass
{
	constexpr auto render_format = vk::Format::eR8G8B8A8Srgb;
	constexpr auto depth_format = vk::Format::eD32Sfloat;

	GeometryPass pass{};
	pass.extent = render_extent;

	/* Setup the renderpass
	 */
    const auto color_attachment = vk::AttachmentDescription{}
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(render_format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		// NOTE these are important, as they determine the layout of the image before and after
		// the renderpass
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eTransferSrcOptimal);

    const auto depth_attachment = vk::AttachmentDescription{}
		.setFlags(vk::AttachmentDescriptionFlags())
		.setFormat(depth_format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		// NOTE these are important, as they determine the layout of the image before and after
		// the renderpass
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	const auto color_reference = vk::AttachmentReference{}
		.setAttachment(0)
		.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	const auto depth_reference = vk::AttachmentReference{}
		.setAttachment(1)
		.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
	
    auto subpass = vk::SubpassDescription{}
		.setFlags(vk::SubpassDescriptionFlags())
		.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
		.setInputAttachments({})
		.setResolveAttachments({})
		.setColorAttachments(color_reference)
		.setPDepthStencilAttachment(&depth_reference);
	
	// @note we could also specify color and depth dependencies seperately
	//       and put them together in the renderpass as an array
	auto color_depth_dependency = vk::SubpassDependency{}
		.setSrcSubpass(vk::SubpassExternal)
		.setDstSubpass(0)
		.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
						 | vk::PipelineStageFlagBits::eEarlyFragmentTests)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
						 | vk::PipelineStageFlagBits::eEarlyFragmentTests)
		.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite
						  | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	
	std::array<vk::AttachmentDescription, 2> attachments {color_attachment, depth_attachment};
	std::array<vk::SubpassDependency, 1> dependencies {color_depth_dependency};
    auto renderPassCreateInfo = vk::RenderPassCreateInfo{}
		.setFlags(vk::RenderPassCreateFlags())
		.setAttachments(attachments)
		.setDependencies(dependencies)
		.setSubpasses(subpass);

    pass.renderpass = context->device.get().createRenderPassUnique(renderPassCreateInfo);
	context->logger.info(std::source_location::current(),
						 "Created Render Pass!");
	
	U32Extent texture_extent {
		render_extent.width,
		render_extent.height
	};
	
	for (size_t i = 0; i < frames_in_flight; i++) {
		/* Setup the rendertarget for the render pass
		 */

		pass.colorbuffers.push_back(Texture2D::Impl(RenderTargetTexture,
													context,
													texture_extent,
													vkformat_to_textureformat(render_format)));
		
		auto transition_to_transfer_src = [&] (vk::CommandBuffer& commandbuffer)
		{
			/* Setup the rendertarget
			 */
			pass.colorbuffers.back().layout =
				transition_image_for_color_override(pass.colorbuffers.back().allocated.image.get(),
													commandbuffer);
		};
		
		with_buffer_submit(context->device.get(),
						   context->commandpool.get(),
						   context->graphics_queue(),
						   transition_to_transfer_src);
		
		/* Setup the rendertarget view
		 */
		pass.colorbuffer_views
			.push_back(pass.colorbuffers.back()
					   .create_view(context,
									vk::ImageAspectFlagBits::eColor));

		/* Setup the Depthbuffers
		 */
		pass.depthbuffers.push_back(Texture2D::Impl(DepthBufferTexture,
													context,
													texture_extent));
														 
		/* Setup the depthbuffer view
		 */
		pass.depthbuffer_views
			.push_back(pass.depthbuffers.back()
					   .create_view(context,
									vk::ImageAspectFlagBits::eDepth));
		
		/* Setup the FrameBuffers
		 */
		std::array<vk::ImageView, 2> attachments{
			pass.colorbuffer_views.back().get(),
			pass.depthbuffer_views.back().get(),
		};
		auto framebufferCreateInfo = vk::FramebufferCreateInfo{}
			.setFlags(vk::FramebufferCreateFlags())
			.setAttachments(attachments)
			.setWidth(render_extent.width)
			.setHeight(render_extent.height)
			.setRenderPass(pass.renderpass.get())
			.setLayers(1);
		pass.framebuffers
			.push_back(context->device.get()
					   .createFramebufferUnique(framebufferCreateInfo));
	}

	context->logger.info(std::source_location::current(),
						 "Created FramePasses!");

	return pass;
}

auto render_geometry_pass(GeometryPass& pass,
						  ShadowPass& shadow_pass,
						  // TODO: Pipelines are captured as a ptr because bind_front
						  //       does not want to capture a reference for it...
						  GeometryPipelines* pipelines,
						  Logger* logger,
						  const uint32_t current_frame_in_flight,
						  const uint32_t max_frames_in_flight,
						  const uint64_t total_frames,
						  vk::Device& device,
						  vk::DescriptorPool descriptor_pool,
						  vk::CommandPool& command_pool,
						  vk::Queue& queue,
						  const WorldRenderInfo& world_info,
						  std::vector<Renderable>& renderables,
						  std::vector<Light>& lights)
	-> Texture2D::Impl*
{
	//TODO: Pull clearvalues out!
	const float flash = std::abs(std::sin(total_frames / 120.f));
	std::array<vk::ClearValue, 2> clearvalues{
		vk::ClearValue{}.setColor({0.0f, 0.0f, flash, 1.0f}),
		vk::ClearValue{}.setDepthStencil({1.0f, 0}),
	};
	
	SortedRenderables sorted{};
	auto sort = std::bind_front(sort_renderable, logger, &sorted);
	std::ranges::for_each(renderables, sort);
	


	auto generate_shadow_pass = [&] (vk::CommandBuffer& commandbuffer) 
	{
		ShadowPass::CameraUniformData camera_data;
		
		//TODO: Depth textures is much further away than expected!
		//      maybe ortho only works for directional lights, and if we want 
		//      depth for spots we do perspective with normalized projection?
        glm::mat4 lightProjection;
        float near_plane = 1.0f, far_plane = 7.5f;
        lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		//TODO: maybe we flip like for perspective lights?
		lightProjection[1][1] *= -1;

		camera_data.view = world_info.view;
		camera_data.proj = lightProjection;

		shadow_pass.record(logger,
						   device,
						   CurrentFlightFrame{current_frame_in_flight},
						   commandbuffer,
						   camera_data,
						   sorted.materialrenderables);
	};

	//TODO: shadow and geometry passes should be in same commandbuffer with proper image barrier
	with_buffer_submit(device,
					   command_pool,
					   queue,
					   generate_shadow_pass);
	
	auto generate_frame = [&] (vk::CommandBuffer& commandbuffer) 
	{
		const auto render_area = vk::Rect2D{}
			.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
			.setExtent(pass.extent);
		
		const auto renderPassInfo = vk::RenderPassBeginInfo{}
			.setRenderPass(pass.renderpass.get())
			.setFramebuffer(pass.framebuffers[current_frame_in_flight].get())
			.setRenderArea(render_area)
			.setClearValues(clearvalues);

		commandbuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		const std::vector<vk::Viewport> viewports{
			vk::Viewport{}
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(pass.extent.width)
			.setHeight(pass.extent.height)
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f),
		};
		const uint32_t viewport_start = 0;
		commandbuffer.setViewport(viewport_start, viewports);

		const std::vector<vk::Rect2D> scissors{
			vk::Rect2D{}
			.setOffset(vk::Offset2D{}.setX(0.0f).setY(0.0f))
			.setExtent(pass.extent),
		};
		const uint32_t scissor_start = 0;
		commandbuffer.setScissor(scissor_start, scissors);
		
		NormColorRenderInfo normcolor_info{};
		normcolor_info.view = world_info.view;
		normcolor_info.proj = world_info.projection;

		draw_normcolors(device,
						pipelines->normcolor,
						commandbuffer,
						current_frame_in_flight,
						normcolor_info,
						sorted.normcolors);

		WireframeRenderInfo wireframe_info{};
		wireframe_info.viewproj = world_info.projection * world_info.view;

		draw_wireframes(pipelines->wireframe,
						commandbuffer,
						wireframe_info,
						sorted.wireframes);
		
		BaseTextureRenderInfo texture_info{};
		texture_info.view = world_info.view;
		texture_info.proj = world_info.projection;
		draw_base_texture_renderables(pipelines->basetexture,
									  *logger,
									  device,
									  descriptor_pool,
									  commandbuffer,
									  current_frame_in_flight,
									  max_frames_in_flight,
									  texture_info,
									  sorted.basetextures);

		
		MaterialPipeline::FrameInfo material_frame_info{};
		material_frame_info.view = world_info.view;
		material_frame_info.proj = world_info.projection;
		material_frame_info.camera_position = world_info.camera_position;
		pipelines->material.render(material_frame_info,
								   *logger,
								   device,
								   descriptor_pool,
								   commandbuffer,
								   CurrentFlightFrame{current_frame_in_flight},
								   MaxFlightFrames{max_frames_in_flight},
								   sorted.materialrenderables,
								   lights);

		commandbuffer.endRenderPass();
	};

	with_buffer_submit(device,
					   command_pool,
					   queue,
					   generate_frame);

	return &pass.colorbuffers[current_frame_in_flight];
}

Renderer::Impl::Impl(Render::Context::Impl* context,
					 Presenter::Impl* presenter,
					 Logger logger,
					 DescriptorPool::Impl* descriptor_pool,
					 std::filesystem::path shaders_root)
	: shaders_root(shaders_root)
	, context(context)
	, presenter(presenter)
	, logger(logger)
	, descriptor_pool(descriptor_pool)
{
	//TODO: Allow extent to be set externally
	//TODO: Allow debug print to be set externally
	vk::Extent2D const render_extent = context->get_window_extent();
	bool const debug_print = true;
	shadow_pass = ShadowPass(logger,
							 context,
							 presenter,
							 U32Extent{1024, 1024},
							 shaders_root,
							 debug_print);

	geometry_pass = create_geometry_pass(context,
										 render_extent,
										 presenter->max_frames_in_flight,
										 debug_print);
	
	geometry_pipelines.material = MaterialPipeline(logger,
												   context,
												   presenter,
												   descriptor_pool,
												   geometry_pass.renderpass.get(),
												   shaders_root);
	context->logger.info(std::source_location::current(),
						 "Created Material Pipeline");
	
	geometry_pipelines.basetexture = create_base_texture_pipeline(context->logger,
																  context,
																  presenter,
																  descriptor_pool,
																  geometry_pass.renderpass.get(),
																  presenter->max_frames_in_flight,
																  render_extent,
																  shaders_root,
																  debug_print);
	context->logger.info(std::source_location::current(),
						 "Created BaseTexture Pipeline");

	geometry_pipelines.normcolor = create_norm_render_pipeline(context->logger,
															   context->physical_device,
															   context->device.get(),
															   geometry_pass.renderpass.get(),
															   presenter->max_frames_in_flight,
															   render_extent,
															   shaders_root,
															   debug_print);
	context->logger.info(std::source_location::current(),
						 "Created NormColor Pipeline");

	geometry_pipelines.wireframe = create_wireframe_render_pipeline(context->logger,
																	context->device.get(),
																	geometry_pass.renderpass.get(),
																	render_extent,
																	shaders_root,
																	debug_print);
	context->logger.info(std::source_location::current(),
						 "Created Wireframe Pipeline");
}

Renderer::Impl::~Impl()
{
}

auto Renderer::Impl::render(const uint32_t current_frame_in_flight,
							const uint64_t total_frames,
							const WorldRenderInfo& world_info,
							std::vector<Renderable>& renderables,
							std::vector<Light>& lights)
		-> Texture2D::Impl*
{
	return render_geometry_pass(geometry_pass,
								shadow_pass,
								&geometry_pipelines,
								&logger,
								current_frame_in_flight,
								presenter->max_frames_in_flight,
								total_frames,
								context->device.get(),
								descriptor_pool->descriptor_pool.get(),
								presenter->command_pool(),
								context->graphics_queue(),
								world_info,
								renderables,
								lights);
}

auto Renderer::render(const uint32_t current_frame_in_flight,
					  const uint64_t total_frames,
					  const WorldRenderInfo& world_info,
					  std::vector<Renderable>& renderables,
					  std::vector<Light>& lights)
		-> Texture2D::Impl*
{
	return impl->render(current_frame_in_flight,
						total_frames,
						world_info,
						renderables,
						lights);
}

Renderer::Renderer(Render::Context& context,
				   Presenter& presenter,
				   Logger logger,
				   DescriptorPool& descriptor_pool,
				   const std::filesystem::path shaders_root)
	: impl(std::make_unique<Impl>(context.impl.get(),
								  presenter.impl.get(),
								  logger,
								  descriptor_pool.impl.get(),
								  shaders_root))
{
}

Renderer::~Renderer()
{
}
