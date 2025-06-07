#include "GeometryPassImpl.hpp"

void sort_renderable(SortedRenderables* sorted,
					 Renderable renderable)
{
	if (auto p = std::get_if<NormColorRenderable>(&renderable))
		sorted->normcolors.push_back(*p);
	else if (auto p = std::get_if<WireframeRenderable>(&renderable))
		sorted->wireframes.push_back(*p);
	else if (auto p = std::get_if<BaseTextureRenderable>(&renderable))
		sorted->basetextures.push_back(*p);
	else
		throw std::runtime_error("Not sortable");
};


auto create_pipelines(vk::PhysicalDevice& physical_device,
					  vk::Device& device,
					  vk::CommandPool& command_pool,
					  vk::Queue& graphics_queue,
					  vk::DescriptorPool& descriptor_pool,
					  vk::RenderPass& renderpass,
					  const uint32_t frames_in_flight,
					  const vk::Extent2D render_extent,
					  const std::filesystem::path shader_root_path,
					  bool debug_print) 
	-> Pipelines
{
	Pipelines pipelines;
	pipelines.basetexture = create_base_texture_pipeline(physical_device,
														 device,
														 command_pool,
														 graphics_queue,
														 descriptor_pool,
														 renderpass,
														 frames_in_flight,
														 render_extent,
														 shader_root_path,
														 debug_print);

	pipelines.normcolor = create_norm_render_pipeline(physical_device,
													  device,
													  renderpass,
													  frames_in_flight,
													  render_extent,
													  shader_root_path,
													  debug_print);

	pipelines.wireframe = create_wireframe_render_pipeline(device,
														   renderpass,
														   render_extent,
														   shader_root_path,
														   debug_print);
	
	return pipelines;
}

auto create_geometry_pass(vk::PhysicalDevice& physical_device,
						  vk::Device& device,
						  vk::CommandPool& command_pool,
						  vk::Queue& graphics_queue,
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

    pass.renderpass = device.createRenderPassUnique(renderPassCreateInfo);
	if (debug_print)
		std::cout << "> GeometryPass > created Render Pass!" << std::endl;
	
	const auto texture_extent = vk::Extent3D{}
		.setWidth(render_extent.width)
		.setHeight(render_extent.height)
		.setDepth(1);

	for (size_t i = 0; i < frames_in_flight; i++) {
		/* Setup the rendertarget for the render pass
		 */
		pass.colorbuffers
			.push_back(create_empty_rendertarget_texture(physical_device,
														 device,
														 render_format,
														 texture_extent,
														 vk::ImageTiling::eOptimal,
														 vk::MemoryPropertyFlagBits::eDeviceLocal));
		
		auto transition_to_transfer_src = [&] (vk::CommandBuffer& commandbuffer)
		{
			/* Setup the rendertarget
			 */
			pass.colorbuffers.back().layout =
				transition_image_for_color_override(get_image(pass.colorbuffers.back()),
													commandbuffer);
		};
		
		with_buffer_submit(device,
						   command_pool,
						   graphics_queue,
						   transition_to_transfer_src);
		
		/* Setup the rendertarget view
		 */
		pass.colorbuffer_views.push_back(create_texture_view(device,
															 pass.colorbuffers.back(),
															 vk::ImageAspectFlagBits::eColor));

		/* Setup the Depthbuffers
		 */
		pass.depthbuffers.push_back(create_empty_texture(physical_device,
														 device,
														 depth_format,
														 texture_extent,
														 vk::ImageTiling::eOptimal,
														 vk::MemoryPropertyFlagBits::eDeviceLocal,
														 vk::ImageUsageFlagBits::eDepthStencilAttachment));
														 
		/* Setup the depthbuffer view
		 */
		pass.depthbuffer_views.push_back(create_texture_view(device,
															 pass.depthbuffers.back(),
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
		pass.framebuffers.push_back(device.createFramebufferUnique(framebufferCreateInfo));
	}

	if (debug_print)
		std::cout << "> GeometryPass > Created FramePasses" << std::endl;

	return pass;
}

auto render_geometry_pass(GeometryPass& pass,
						  // TODO: Pipelines are captured as a ptr because bind_front
						  //       does not want to capture a reference for it...
						  Pipelines* pipelines,
						  const uint32_t current_frame_in_flight,
						  const uint32_t max_frames_in_flight,
						  const uint64_t total_frames,
						  vk::Device& device,
						  vk::DescriptorPool descriptor_pool,
						  vk::CommandPool& command_pool,
						  vk::Queue& queue,
						  const WorldRenderInfo& world_info,
						  std::vector<Renderable>& renderables)
	-> Texture2D*
{
	//TODO: Pull clearvalues out!
	const float flash = std::abs(std::sin(total_frames / 120.f));
	std::array<vk::ClearValue, 2> clearvalues{
		vk::ClearValue{}.setColor({0.0f, 0.0f, flash, 1.0f}),
		vk::ClearValue{}.setDepthStencil({1.0f, 0}),
	};
	
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
		
		SortedRenderables sorted{};
		auto sort = std::bind_front(sort_renderable, &sorted);
		std::ranges::for_each(renderables, sort);
	
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
									  device,
									  descriptor_pool,
									  commandbuffer,
									  current_frame_in_flight,
									  max_frames_in_flight,
									  texture_info,
									  sorted.basetextures);

		commandbuffer.endRenderPass();
	};

	with_buffer_submit(device,
					   command_pool,
					   queue,
					   generate_frame);

	return &pass.colorbuffers[current_frame_in_flight];
}

Renderer::Impl::Impl(PresentationContext::Impl* presentation_context,
					 DescriptorPool::Impl* descriptor_pool,
					 std::filesystem::path shaders_root)
	: presentation_context(presentation_context)
	, descriptor_pool(descriptor_pool)
	, shaders_root(shaders_root)
{
	geometry_pass = create_geometry_pass(presentation_context->physical_device,
										 presentation_context->device.get(),
										 presentation_context->command_pool(),
										 presentation_context->graphics_queue(),
										 //TODO: Allow extent to be set externally
										 presentation_context->get_window_extent(),
										 presentation_context->maxFramesInFlight_,
										 //TODO: Allow debug print to be set externally
										 true
										 );
	
	pipelines = create_pipelines(presentation_context->physical_device,
								 presentation_context->device.get(),
								 presentation_context->command_pool(),
								 presentation_context->graphics_queue(),
								 descriptor_pool->descriptor_pool.get(),
								 geometry_pass.renderpass.get(),
								 presentation_context->maxFramesInFlight_,
								 presentation_context->get_window_extent(),
								 shaders_root,
								 true);
}

Renderer::Impl::~Impl()
{
}

auto Renderer::Impl::render(const uint32_t current_frame_in_flight,
							const uint64_t total_frames,
							const WorldRenderInfo& world_info,
							std::vector<Renderable>& renderables)
		-> Texture2D*
{
	return render_geometry_pass(geometry_pass,
								&pipelines,
								current_frame_in_flight,
								presentation_context->maxFramesInFlight_,
								total_frames,
								presentation_context->device.get(),
								descriptor_pool->descriptor_pool.get(),
								presentation_context->command_pool(),
								presentation_context->graphics_queue(),
								world_info,
								renderables);
}

auto Renderer::render(const uint32_t current_frame_in_flight,
					  const uint64_t total_frames,
					  const WorldRenderInfo& world_info,
					  std::vector<Renderable>& renderables)
		-> Texture2D*
{
	return impl->render(current_frame_in_flight,
						total_frames,
						world_info,
						renderables);
}

Renderer::Renderer(PresentationContext& presentation_context,
				   DescriptorPool& descriptor_pool,
				   const std::filesystem::path shaders_root)
	: impl(std::make_unique<Impl>(presentation_context.impl.get(),
								  descriptor_pool.impl.get(),
								  shaders_root))
{
}

Renderer::~Renderer()
{
}
