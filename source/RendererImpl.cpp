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
						  Renderer::Impl::ShadowPasses& shadow_passes,
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
						  std::vector<Light>& lights,
						  ShadowCasters& shadowcasters)
	-> Texture2D::Impl*
{
	//TODO: Pull clearvalues out!
	const float flash = std::abs(std::sin(total_frames / 120.f));
	std::array<vk::ClearValue, 2> clearvalues{
		vk::ClearValue{}.setColor({0.0f, 0.0f, flash, 1.0f}),
		vk::ClearValue{}.setDepthStencil({1.0f, 0}),
	};
	
	SortedRenderables sorted{};
	std::ranges::for_each(renderables,
						  std::bind_front(sort_renderable, logger, &sorted));
	

	auto generate_shadow_passes = [&] (vk::CommandBuffer& commandbuffer) 
	{
		//TODO: have multiple directional casters
		if (shadowcasters.directional_caster.has_value()) {
			DirectionalShadowCaster& caster = shadowcasters.directional_caster.value();
			OrthographicShadowPass::CameraUniformData caster_data;
			caster_data.view = caster.view().value_or(glm::mat4(1.0f));
			caster_data.proj = caster.projection.get();
			shadow_passes.orthographic.record(logger,
											  device,
											  CurrentFlightFrame{current_frame_in_flight},
											  commandbuffer,
											  caster_data,
											  sorted.materialrenderables);
		}
		//TODO: have multiple directional casters
		if (!shadowcasters.spot_casters.empty()) {
			SpotShadowCaster& caster = shadowcasters.spot_casters.front();
			PerspectiveShadowPass::CameraUniformData caster_data;
			caster_data.view = caster.view().value_or(glm::mat4(1.0f));
			caster_data.proj = caster.projection.get();
			shadow_passes.perspective.record(logger,
											 device,
											 CurrentFlightFrame{current_frame_in_flight},
											 commandbuffer,
											 caster_data,
											 sorted.materialrenderables);
		}

	};

	//TODO: shadow and geometry passes should be in same commandbuffer with proper image barrier
	with_buffer_submit(device,
					   command_pool,
					   queue,
					   generate_shadow_passes);
	
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
	shadow_passes.orthographic = OrthographicShadowPass(logger,
														context,
														presenter,
														U32Extent{1024, 1024},
														shaders_root,
														debug_print);

	shadow_passes.perspective = PerspectiveShadowPass(logger,
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
							std::vector<Light>& lights,
							ShadowCasters& shadowcasters)
		-> Texture2D::Impl*
{
	return render_geometry_pass(geometry_pass,
								shadow_passes,
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
								lights,
								shadowcasters);
}

auto Renderer::render(const uint32_t current_frame_in_flight,
					  const uint64_t total_frames,
					  const WorldRenderInfo& world_info,
					  std::vector<Renderable>& renderables,
					  std::vector<Light>& lights,
					  ShadowCasters& shadowcasters)
		-> Texture2D::Impl*
{
	return impl->render(current_frame_in_flight,
						total_frames,
						world_info,
						renderables,
						lights,
						shadowcasters);
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
