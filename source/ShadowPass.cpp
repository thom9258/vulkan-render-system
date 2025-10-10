#include "ShadowPass.hpp"

#include "DescriptorPoolImpl.hpp"

ShadowPassTexture::ShadowPassTexture(Render::Context::Impl* context,
									 DescriptorPool::Impl* descriptor_pool,
									 U32Extent extent)

{
	texture =
		Texture2D(std::make_unique<Texture2D::Impl>(RenderTargetTexture,
													context,
													extent,
													TextureFormat::R32Sfloat));

	auto transition_to_transfer_src = [&] (vk::CommandBuffer& commandbuffer)
	{
		transition_image_for_color_override(texture.impl->allocated.image.get(),
											commandbuffer);
	};
	
	with_buffer_submit(context->device.get(),
					   context->commandpool.get(),
					   context->graphics_queue(),
					   transition_to_transfer_src);
		
	const auto subresourceRange = vk::ImageSubresourceRange{}
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
	const auto max_anisotropy = has_anisotropy
		? std::min(4.0f, properties.limits.maxSamplerAnisotropy)
		: 1.0f;
	
	vk::Filter const filter = vk::Filter::eLinear;
	vk::SamplerMipmapMode const mipmap_filter = vk::SamplerMipmapMode::eLinear;

	const auto sampler_info = vk::SamplerCreateInfo{}
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
	
	descriptorset_layout =
		context->device.get().createDescriptorSetLayoutUnique(layout_createinfo,
															  nullptr);

	const auto descriptorset_allocate_info = vk::DescriptorSetAllocateInfo{}
		.setDescriptorPool(descriptor_pool->descriptor_pool.get())
		.setDescriptorSetCount(1)
		.setSetLayouts(descriptorset_layout.get());
	
	auto createdsets =
		context->device.get().allocateDescriptorSetsUnique(descriptorset_allocate_info);
	descriptorset = std::move(createdsets[0]);
	

	const auto descriptorimage_info = vk::DescriptorImageInfo{}
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
	
	context->device.get().updateDescriptorSets(descriptorwrite.size(),
											   descriptorwrite.data(),
											   0,
											   nullptr);
}

ShadowPassTexture::ShadowPassTexture(ShadowPassTexture&& rhs)
{
	std::swap(texture, rhs.texture);
	std::swap(sampler, rhs.sampler);
	std::swap(state, rhs.state);
	std::swap(view, rhs.view);
	std::swap(descriptorset, rhs.descriptorset);
}

ShadowPassTexture& ShadowPassTexture::operator=(ShadowPassTexture&& rhs)
{
	std::swap(texture, rhs.texture);
	std::swap(sampler, rhs.sampler);
	std::swap(state, rhs.state);
	std::swap(view, rhs.view);
	std::swap(descriptorset, rhs.descriptorset);
	return *this;
}

OrthographicShadowPass::OrthographicShadowPass(Logger& logger,
											   Render::Context::Impl* context,
											   Presenter::Impl* presenter,
											   DescriptorPool::Impl* descriptor_pool,
											   U32Extent extent,
											   std::filesystem::path shader_root_path,
											   const bool debug_print)
	: GenericShadowPass(logger,
						context,
						presenter,
						descriptor_pool,
						extent,
						VertexPath{shader_root_path / "OrthographicDepth.vert.spv"},
						FragmentPath{shader_root_path / "OrthographicDepth.frag.spv"},
						debug_print)
{
}

void OrthographicShadowPass::record(Logger* logger,
									vk::Device& device,
									CurrentFlightFrame current_flightframe,
									vk::CommandBuffer& commandbuffer,
									std::optional<CameraUniformData> camera_data,
									std::vector<MaterialRenderable>& renderables)
{
	GenericShadowPass::record(logger,
							  device,
							  current_flightframe,
							  commandbuffer,
							  camera_data,
							  renderables);
}

auto OrthographicShadowPass::get_shadowtexture(CurrentFlightFrame current_flightframe)
	-> ShadowPassTexture&
{
	return GenericShadowPass::get_shadowtexture(current_flightframe);
}


PerspectiveShadowPass::PerspectiveShadowPass(Logger& logger,
											 Render::Context::Impl* context,
											 Presenter::Impl* presenter,
											 DescriptorPool::Impl* descriptor_pool,
											 U32Extent extent,
											 std::filesystem::path shader_root_path,
											 const bool debug_print)
	: GenericShadowPass(logger,
						context,
						presenter,
						descriptor_pool,
						extent,
						VertexPath{shader_root_path / "PerspectiveDepth.vert.spv"},
						FragmentPath{shader_root_path / "PerspectiveDepth.frag.spv"},
						debug_print)
{
}

void PerspectiveShadowPass::record(Logger* logger,
								   vk::Device& device,
								   CurrentFlightFrame current_flightframe,
								   vk::CommandBuffer& commandbuffer,
								   std::optional<CameraUniformData> camera_data,
								   std::vector<MaterialRenderable>& renderables)
{
	GenericShadowPass::record(logger,
							  device,
							  current_flightframe,
							  commandbuffer,
							  camera_data,
							  renderables);
}

auto PerspectiveShadowPass::get_shadowtexture(CurrentFlightFrame current_flightframe)
	-> ShadowPassTexture&
{
	return GenericShadowPass::get_shadowtexture(current_flightframe);
}



GenericShadowPass& GenericShadowPass::operator=(GenericShadowPass&& rhs)
{
	std::swap(m_extent, rhs.m_extent);
	std::swap(m_renderpass, rhs.m_renderpass);
	std::swap(m_framestextures, rhs.m_framestextures);
	std::swap(m_pipeline, rhs.m_pipeline);
	return *this;
}

GenericShadowPass::GenericShadowPass(GenericShadowPass&& rhs)
{
	std::swap(m_extent, rhs.m_extent);
	std::swap(m_renderpass, rhs.m_renderpass);
	std::swap(m_framestextures, rhs.m_framestextures);
	std::swap(m_pipeline, rhs.m_pipeline);
}


GenericShadowPass::GenericShadowPass(Logger& logger,
									 Render::Context::Impl* context,
									 Presenter::Impl* presenter,
									 DescriptorPool::Impl* descriptor_pool,
									 U32Extent extent,
									 VertexPath vertex_path,
									 FragmentPath fragment_path,
									 const bool debug_print)
	: m_extent{extent}
{
	auto constexpr color_format = vk::Format::eR32Sfloat;
	auto constexpr depth_format = vk::Format::eD32Sfloat;
    auto constexpr colorComponentFlags(vk::ColorComponentFlagBits::eR);
	const std::string pipeline_name = "ShadowPass";
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
		.setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

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
		textures.colorbuffer = ShadowPassTexture(context, descriptor_pool, m_extent);

		//textures.colorbuffer = Texture2D(std::make_unique<Texture2D::Impl>(RenderTargetTexture,
		//context,
		//m_extent,
		//vkformat_to_textureformat(color_format)));
		//
		//auto transition_to_transfer_src = [&] (vk::CommandBuffer& commandbuffer)
		//{
		//textures.colorbuffer.impl->layout =
		//transition_image_for_color_override(textures.colorbuffer.impl->allocated.image.get(),
		//commandbuffer);
	//};
		//
		//with_buffer_submit(context->device.get(),
		//context->commandpool.get(),
		//context->graphics_queue(),
		//transition_to_transfer_src);
		
		textures.colorbuffer_view
			= textures.colorbuffer.texture.impl->create_view(context,
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
	
	auto shaderstage_infos = create_shaderstage_infos(context->device.get(),
													  vertex_path,
													  fragment_path);

	if (!shaderstage_infos) {
		std::string const msg = std::format("{} could not load vertex/fragment sources {} / {}",
											pipeline_name,
											vertex_path.get().string(),
											fragment_path.get().string());
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
		//NOTE we cull front faces so we draw backfaces to the shadow depth.
		//     this helps with peter panning where shadows makes objects seem to float.
		.setCullMode(vk::CullModeFlagBits::eFront)
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

void GenericShadowPass::record(Logger* logger,
							   vk::Device& device,
							   CurrentFlightFrame current_flightframe,
							   vk::CommandBuffer& commandbuffer,
							   std::optional<CameraUniformData> camera_data,
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
	

	// if there is no shadow camera we end the renderpass immediately
	// we still need to begin/end it so that the implicit shadow texture
	// commands are applied.
	// commands are applied.
	if (!camera_data.has_value()) {
		commandbuffer.endRenderPass();
		return;
	}
	
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
	
	CameraUniformData camera_uniform_data = camera_data.value();
	copy_to_allocated_memory(device,
							 m_pipeline.descriptor_memories[current_flightframe.get()],
							 reinterpret_cast<void*>(&camera_uniform_data),
							 sizeof(camera_uniform_data));
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
		if (!renderable.has_shadow) 
			continue;
		
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

auto GenericShadowPass::get_shadowtexture(CurrentFlightFrame current_flightframe)
	-> ShadowPassTexture&
{
	return m_framestextures[current_flightframe.get()].colorbuffer;
}
