#include "PresenterImpl.hpp"
#include "TextureImpl.hpp"

#include <iostream>

vk::CommandPool& Presenter::Impl::command_pool()
{
	return commandpool.get();
}

Presenter::Impl::~Impl()
{
	// TODO: Port over the ResourceWrapperRuntime so we can automatically destroy all this stuff..
	// Note we need to destroy the swapchain manually so it happens before the surface...
	swapchain.reset();
}

Presenter::Impl::Impl(Render::Context::Impl* context)
	: max_frames_in_flight(2)
	, context(context)
{
	CreateSwapChain();
	CreateCommandpool();
	CreateCommandbuffers();
	CreateSyncObjects();
	CreateRenderTargets();
}

void Presenter::Impl::CreateSwapChain()
{
	const auto surface_capabilities = context->window_surface_capabilities();
	const auto window_extent = context->get_window_extent();
	std::cout << "> Window width:  " << window_extent.width << "\n"
			  << "         height: " << window_extent.height << std::endl;

	vk::Extent2D swapchain_extent;
	const bool surface_size_is_undefined = 
		surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max();
	if (surface_size_is_undefined) {
		// If the surface size is undefined, the size is set to the size of the images requested.
		swapchain_extent.width = std::clamp(window_extent.width,
											surface_capabilities.minImageExtent.width,
											surface_capabilities.maxImageExtent.width);
		
		swapchain_extent.height = std::clamp(window_extent.height,
											 surface_capabilities.minImageExtent.height,
											 surface_capabilities.maxImageExtent.height);
	}
	else {
		// If the surface size is defined, the swap chain size must match
		swapchain_extent = surface_capabilities.currentExtent;
	}
	
	std::cout << "> SwapChain width:  " << swapchain_extent.width << "\n"
			  << "            height: " << swapchain_extent.height << std::endl;

	const auto formats = context->window_surface_formats();
	if (formats.empty())
		throw std::runtime_error("Swapchain has no formats!");
	
	swapchain_format = get_swapchain_surface_format(formats);

	std::cout << "> Swapchain format: " << vk::to_string(swapchain_format.format) << std::endl;

	const vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
	std::cout << "> Swapchain PresentMode: " << vk::to_string(swapchainPresentMode) << std::endl;
	// vk::PresentModeKHR::eMailBox is potentially good on low power devices..

	const auto supports_identity = 
		static_cast<bool>(surface_capabilities.supportedTransforms 
						  & vk::SurfaceTransformFlagBitsKHR::eIdentity);
	
	const vk::SurfaceTransformFlagBitsKHR preTransform =
		(supports_identity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity
		                    : surface_capabilities.currentTransform;
	
	// this spaghetti determines the best compositealpha flags....
	vk::CompositeAlphaFlagBitsKHR compositeAlpha =
	( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied )    ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
	: ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied ) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
	: ( surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit )        ? vk::CompositeAlphaFlagBitsKHR::eInherit
	: vk::CompositeAlphaFlagBitsKHR::eOpaque;
	

	const auto wanted_surface_image_count = 3u;
	uint32_t surface_image_count{0};
	if (surface_capabilities.maxImageCount > 0) {
		surface_image_count = std::clamp(wanted_surface_image_count,
										 surface_capabilities.minImageCount,
										 surface_capabilities.maxImageCount);
	}
	else {
		surface_image_count = std::min(wanted_surface_image_count,
									   surface_capabilities.minImageCount);
	}

	std::cout << "> surface capabilities:\n"
	<< "  > min image count: " << surface_capabilities.minImageCount << "\n"
	<< "  > max image count: " << surface_capabilities.maxImageCount << "\n"
	<< "  > requested image count: " << surface_image_count 
	<< std::endl;
	
	auto swapChainCreateInfo = vk::SwapchainCreateInfoKHR{}
		.setFlags(vk::SwapchainCreateFlagsKHR())
		.setSurface(context->raw_window_surface)
		.setMinImageCount(surface_image_count)
		.setImageFormat(swapchain_format.format)
		.setImageColorSpace(swapchain_format.colorSpace)
		.setImageExtent(swapchain_extent)
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment
					   | vk::ImageUsageFlagBits::eTransferSrc
					   | vk::ImageUsageFlagBits::eTransferDst
					   | vk::ImageUsageFlagBits::eSampled)
		.setClipped(true)
		.setPreTransform(preTransform)
		.setCompositeAlpha(compositeAlpha)
		//.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(swapchainPresentMode);
	
	std::array<uint32_t, 2> splitIndices = {
		::graphics_index(context->graphics_present_indices),
		::present_index(context->graphics_present_indices)
	};

	if (std::holds_alternative<SharedGraphicsPresentIndex>(context->graphics_present_indices)) {
		swapChainCreateInfo
			.setImageSharingMode(vk::SharingMode::eExclusive);
	} 
	else {
		swapChainCreateInfo
			.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndices(splitIndices);
	}

	swapchain = context->device->createSwapchainKHRUnique(swapChainCreateInfo, nullptr);

	swapchain_images = context->device->getSwapchainImagesKHR(*swapchain);
	std::cout << "> SwapChain image count: " << swapchain_images.size() << std::endl;
	
	swapchain_imageviews.reserve(swapchain_images.size());
	
	auto subresourceRange = vk::ImageSubresourceRange{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	auto componentMapping = vk::ComponentMapping{}
		.setR(vk::ComponentSwizzle::eIdentity)		
		.setG(vk::ComponentSwizzle::eIdentity)
		.setB(vk::ComponentSwizzle::eIdentity)
		.setA(vk::ComponentSwizzle::eIdentity);

	auto imageViewCreateInfo = vk::ImageViewCreateInfo{}
		.setSubresourceRange(subresourceRange)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(swapchain_format.format)
		.setComponents(componentMapping);
	
	for (auto& image : swapchain_images) {
		imageViewCreateInfo.setImage(image);
		swapchain_imageviews
			.push_back(context->device->createImageViewUnique(imageViewCreateInfo));
	}

	std::cout << "> created SwapChain!" << std::endl;
}

void Presenter::Impl::CreateRenderTargets()
{
	for (size_t i = 0; i < swapchain_images.size(); i++) {
		const auto extent = context->get_window_extent();
		auto texture = Texture2D::Impl(RenderTargetTexture,
									   context,
									   U32Extent{
										   extent.width,
										   extent.height},
									   TextureFormat::R8G8B8A8Srgb);

		with_buffer_submit(context->device.get(),
						   command_pool(),
						   context->graphics_queue(),
						   [&] (vk::CommandBuffer& commandbuffer)
						   {
							   texture.layout =
								  transition_image_for_color_override(texture.image(),
																	  commandbuffer);

						   });
		
		rendertargets.push_back(std::move(texture));
	}
	std::cout << "> Created Render Targets" << std::endl;
}

void Presenter::Impl::CreateCommandpool()
{
    auto commandPoolCreateInfo = vk::CommandPoolCreateInfo{}
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(graphics_index(context->graphics_present_indices));
	commandpool = context->device->createCommandPoolUnique(commandPoolCreateInfo, nullptr);

	std::cout << "> Created Command pool" << std::endl;
}

void Presenter::Impl::CreateCommandbuffers()
{
	// allocate a CommandBuffer from the CommandPool
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo
		.setCommandPool(*commandpool)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(max_frames_in_flight);

	commandbuffers = context->device->allocateCommandBuffersUnique(commandBufferAllocateInfo);
	std::cout << "> Created " << commandbuffers.size() << " Command buffers" << std::endl;
}

void Presenter::Impl::CreateSyncObjects()
{
	const auto semaphoreCreateInfo = vk::SemaphoreCreateInfo{};
	const auto fenceCreateInfo = vk::FenceCreateInfo{}
		.setFlags(vk::FenceCreateFlagBits::eSignaled);
	// Signaled means that the fence is engaged at creation, this
	// simplifies the first time we want to wait for it to not need
	// extra logic

	for (int i = 0; i < max_frames_in_flight; i++) {
		imageAvailableSemaphores.push_back(context->device->createSemaphoreUnique(semaphoreCreateInfo,
																		   nullptr));
		renderFinishedSemaphores.push_back(context->device->createSemaphoreUnique(semaphoreCreateInfo,
																		   nullptr));
		inFlightFences.push_back(context->device->createFenceUnique(fenceCreateInfo, nullptr));
	}

	std::cout << "> Created Sync Objects" << std::endl;
}

void
Presenter::Impl::RecordBlitTextureToSwapchain(vk::CommandBuffer& commandbuffer,
											 vk::Image& swapchain_image,
											 Texture2D::Impl* texture)
{
	auto printImageBarrierTransition = [&] (const std::string name, 
											const vk::ImageMemoryBarrier& barrier)
	{
		if (!per_frame_debug_print)
			return;
		std::cout << "Transfered " << name << " from " 
				  << vk::to_string(barrier.oldLayout) << " to "
				  << vk::to_string(barrier.newLayout) << "\n"
				  << "=======================================" 
				  << std::endl;
	};

	if (per_frame_debug_print) {
		std::cout << "=======================================" << std::endl;
		std::cout << "=======================================" << std::endl;
	}
	
	commandbuffer.reset(vk::CommandBufferResetFlags());
	const auto beginInfo = vk::CommandBufferBeginInfo{};
	commandbuffer.begin(beginInfo);

	if (true) {
		auto range = vk::ImageSubresourceRange{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		
		auto barrier = vk::ImageMemoryBarrier{}
			.setImage(swapchain_image)
			.setSubresourceRange(range)
			.setOldLayout(vk::ImageLayout::eUndefined)
			.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
			.setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
			.setDstAccessMask(vk::AccessFlags())
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		
		commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
									  vk::PipelineStageFlagBits::eTransfer,
									  vk::DependencyFlags(),
									  nullptr,
									  nullptr,
									  barrier);

		printImageBarrierTransition("SwapChain image", barrier);
	}
	if (true) {
		auto src_subresource = vk::ImageSubresourceLayers{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setMipLevel(0);
		const std::array<vk::Offset3D, 2> src_offsets{ 
			vk::Offset3D(0, 0, 0),
			vk::Offset3D(texture->extent.width,
						 texture->extent.height,
						 1)
		};
		
		auto dst_subresource = vk::ImageSubresourceLayers{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setMipLevel(0);
		const auto window_extent = context->get_window_extent();
		const std::array<vk::Offset3D, 2> dst_offsets{ 
			vk::Offset3D(0, 0, 0),
			vk::Offset3D(window_extent.width, window_extent.height, 1)
		};
		
		auto image_blit = vk::ImageBlit{}
			.setSrcOffsets(src_offsets)
			.setSrcSubresource(src_subresource)
			.setDstOffsets(dst_offsets)
			.setDstSubresource(dst_subresource)
			;
		
		// TODO: This always does linear blitting!, for low res games we need to be able
		// to specify point filtering!
		commandbuffer.blitImage(texture->image(),
								vk::ImageLayout::eTransferSrcOptimal,
								swapchain_image,
								vk::ImageLayout::eTransferDstOptimal,
								image_blit,
								// Linear Interpolation is used
								vk::Filter::eLinear);
		if (per_frame_debug_print) {
			std::cout << "Blitted rendertexture to swapchain image" << std::endl;
			std::cout << "=======================================" << std::endl;
		}
	}
	if (true) {
		auto range = vk::ImageSubresourceRange{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);
		
		auto barrier = vk::ImageMemoryBarrier{}
			.setImage(swapchain_image)
			.setSubresourceRange(range)
			.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
			.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eTransferRead)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
		
		commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
									  vk::PipelineStageFlagBits::eTransfer,
									  vk::DependencyFlags(),
									  nullptr,
									  nullptr,
									  barrier);

		printImageBarrierTransition("SwapChain Image", barrier);
	}

	commandbuffer.end();
}


void Presenter::Impl::with_presentation(FrameProducer& currentFrameGenerator)
{

	const auto maxTimeout = std::numeric_limits<unsigned int>::max();
	auto waitresult = context->device->waitForFences(*(inFlightFences[current_frame_in_flight]),
													 true,
													 maxTimeout);
	context->device->resetFences(*(inFlightFences[current_frame_in_flight]));

	if (waitresult != vk::Result::eSuccess)
		throw std::runtime_error("Could not wait for inFlightFence");
	
	auto [result, swapchain_index] =
		context->device->acquireNextImageKHR(*swapchain,
											 maxTimeout,
											 *(imageAvailableSemaphores[current_frame_in_flight]));

	assert(result == vk::Result::eSuccess);
	assert(swapchain_index < swapchain_imageviews.size());

	if (per_frame_debug_print) {
		const std::string line = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
		std::cout << line 
				  << "\n> present flight index:    " << current_frame_in_flight
				  << "\n> present swapchain index: " << swapchain_index 
				  << "\n> present total frames:    " << total_frames 
				  << std::endl;
	}
	
	CurrentFrameInfo currentFrameInfo;
	currentFrameInfo.current_flight_frame_index = current_frame_in_flight;
	currentFrameInfo.total_frame_count = total_frames;

	std::optional<Texture2D::Impl*> frameToPresent = std::invoke(currentFrameGenerator,
																 currentFrameInfo);
	if (!frameToPresent.has_value())
		throw std::runtime_error("SwapChain has not implemented a way to present the old"
								 " swapchain image if generator returns nullopt");
	
	RecordBlitTextureToSwapchain(commandbuffers[current_frame_in_flight].get(),
								 swapchain_images[swapchain_index],
								 frameToPresent.value());

	const std::vector<vk::Semaphore> waitSemaphores{
		*(imageAvailableSemaphores[current_frame_in_flight]),
	};
	const std::vector<vk::Semaphore> signalSemaphores{
		*(renderFinishedSemaphores[current_frame_in_flight]),
	};
	
	const std::vector<vk::PipelineStageFlags> waitStages{
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
	};

	auto submitInfo = vk::SubmitInfo{}
		.setWaitSemaphores(waitSemaphores)
		.setWaitDstStageMask(waitStages)
		.setCommandBuffers(*(commandbuffers[current_frame_in_flight]))
		.setSignalSemaphores(signalSemaphores);
	
	::present_queue(context->index_queues).submit(submitInfo,
												  *(inFlightFences[current_frame_in_flight]));

	const std::vector<vk::SwapchainKHR> swapchains = {*swapchain};
	const std::vector<uint32_t> imageIndices = {swapchain_index};
	auto presentInfo = vk::PresentInfoKHR{}
		.setSwapchains(swapchains)
		.setImageIndices(imageIndices)
		.setWaitSemaphores(signalSemaphores);
	
	auto presentResult = present_queue(context->index_queues).presentKHR(presentInfo);
	if (presentResult != vk::Result::eSuccess)
		throw std::runtime_error("Could not wait for inFlightFence");

    current_frame_in_flight = (current_frame_in_flight + 1) % max_frames_in_flight;
	total_frames++;
}

Presenter::~Presenter()
{
}

Presenter::Presenter(Render::Context* context)
  : impl(std::make_unique<Presenter::Impl>(context->impl.get()))
{
}

void Presenter::with_presentation(FrameProducer& next_frame_producer)
{
	return impl->with_presentation(next_frame_producer);
}
