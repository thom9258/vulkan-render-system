#include <VulkanRenderer/VulkanRenderer.hpp>


vk::CommandPool& PresentationContext::command_pool()
{
	return commandpool_.get();
}

vk::Queue& PresentationContext::graphics_queue()
{
	return ::graphics_queue(index_queues_);
}

PresentationContext::~PresentationContext()
{
	// TODO: Port over the ResourceWrapperRuntime so we can automatically destroy all this stuff..
	// Note we need to destroy the swapchain manually so it happens before the surface...
	swapchain_.reset();
	instance_->destroySurfaceKHR(raw_window_surface_, nullptr);
	SDL_DestroyWindow(window_);
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
}

PresentationContext::PresentationContext(WindowConfig const& config,
										 Logger logger)
	: maxFramesInFlight_(2)
	, logger(logger)
{
	CreateContext();
	CreateWindow(config);
	CreateInstance();
	CreateWindowSurface();
	CreateDebugMessenger();
	GetPhysicalDevice();
	GetQueueFamilyIndices();
	CreateDevice();
	CreateIndexQueues();
	CreateSwapChain();
	CreateCommandpool();
	CreateCommandbuffers();
	CreateSyncObjects();

	CreateRenderTargets();
}

void PresentationContext::CreateContext()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0)
		throw std::runtime_error("Could not init sdl!");
    SDL_Vulkan_LoadLibrary(nullptr);
}

void PresentationContext::CreateWindow(WindowConfig const& config)
{
	int pos_x = -1;
	int pos_y = -1;
	if (config.position.has_value()) {
		pos_x = config.position.value().width();
		pos_y = config.position.value().height();
	}

	window_ = SDL_CreateWindow(config.name.c_str(),
							   pos_x,
							   pos_y,
							   config.size.width(),
							   config.size.height(),
							   0 | SDL_WINDOW_VULKAN);
}

[[nodiscard]]
bool
is_validation_layer_available(const char* layer)
{
	std::string str_layer = layer;
	std::vector<vk::LayerProperties> properties = vk::enumerateInstanceLayerProperties();
	for (vk::LayerProperties& property: properties) {
		std::string str_available = property.layerName;
		if (str_layer == str_available)
			return true;
	}

	return false;
}

void PresentationContext::CreateInstance()
{
	std::vector<const char*> sdl_instance_extensions = get_sdl2_instance_extensions(window_);
	auto instance_extensions = sdl_instance_extensions;
	instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	//instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	
	std::cout << "Instance Extensions:\n";
	for (const auto& extension: instance_extensions)
		std::cout << "-> " << extension << "\n";
	std::cout << std::endl;

	//NOTE Only do validation layers for debugging
	std::vector<const char*> validation_layers{
		"VK_LAYER_KHRONOS_validation"
	};
	
	for (auto wanted: validation_layers)
		if (!is_validation_layer_available(wanted))
			throw std::runtime_error("Validation layer not available");
	
	auto applicationInfo = vk::ApplicationInfo{}
		.setPApplicationName("app")
		.setPEngineName("engine")
		.setApplicationVersion(VK_MAKE_VERSION(1, 1, 0))
		.setEngineVersion(VK_MAKE_VERSION(1, 1, 0))
		.setApiVersion(VK_MAKE_VERSION(1, 1, 0));

	auto instanceCreateInfo = vk::InstanceCreateInfo{}
		.setPApplicationInfo(&applicationInfo)
		.setPEnabledLayerNames(validation_layers)
		.setPEnabledExtensionNames(instance_extensions);
	
    instance_ = vk::createInstanceUnique(instanceCreateInfo);
	std::cout << "Created Vulkan Instance!" << std::endl;
}

void PresentationContext::CreateWindowSurface()
{
	SDL_Vulkan_CreateSurface(window_, *instance_, &raw_window_surface_);
}

void PresentationContext::CreateDebugMessenger()
{
#if 0	
	auto debugMessengerCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT{}
	.setFlags(vk::DebugUtilsMessengerCreateFlagsEXT())
	.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError 
					  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
					  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
					  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)
	.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral 
				  | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation 
				  | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
	.setPfnUserCallback(&debugUtilsMessengerCallback);

	debug_messenger_ = instance_->createDebugUtilsMessengerEXTUnique(debugMessengerCreateInfo);
	std::cout << "Created Debug Messenger!" << std::endl;
#endif
	std::cout << "TODO: Cannot Create Debug Messenger!" << std::endl;
}

void PresentationContext::GetPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = instance_->enumeratePhysicalDevices();
	physical_device = devices.front();
	std::cout << PhysicalDevice_string(physical_device);
}

void PresentationContext::GetQueueFamilyIndices()
{
	auto graphics_present = get_graphics_present_indices(physical_device,
														 raw_window_surface_);

	if (!graphics_present)
		throw std::runtime_error("could not get graphics and present indices");

	graphics_present_indices_ = *graphics_present;
	
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present_indices_)) {
		std::cout << "> found SHARED graphics present indices" << std::endl;
	}
	else {
		std::cout << "> found SPLIT graphics present indices" << std::endl;
	}
}

void PresentationContext::CreateDevice()
{
	float queuePriority = 1.0f;
	uint32_t device_index = present_index(graphics_present_indices_);

	auto deviceQueueCreateInfo = vk::DeviceQueueCreateInfo{}
		.setFlags({})
		.setQueueFamilyIndex(device_index)
		.setPQueuePriorities(&queuePriority)
		.setQueueCount(1);

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	
	// TODO: We actually need to check if these are available,
	//       if not, we can:
	//       1. discard the device alltogether
	//       2. dont make use of the functionality in our code
	const auto features = vk::PhysicalDeviceFeatures{}
		.setFillModeNonSolid(true)
		.setSamplerAnisotropy(true);
	
	auto deviceCreateInfo = vk::DeviceCreateInfo{}
		.setQueueCreateInfoCount(1)
		.setQueueCreateInfos(deviceQueueCreateInfo)
		.setPEnabledFeatures(&features)
		.setPpEnabledExtensionNames(device_extensions.data())
		.setEnabledExtensionCount(device_extensions.size());
	
	device = physical_device.createDeviceUnique(deviceCreateInfo);
	std::cout << "Created Logical Device!" << std::endl;
}	

void PresentationContext::CreateIndexQueues()
{
	index_queues_ = get_index_queues(*device,
									 graphics_present_indices_);

	if (std::holds_alternative<SharedIndexQueue>(index_queues_)) {
		std::cout << "> created SHARED index queue" << std::endl;
	}
	else {
		std::cout << "> created SPLIT index queues" << std::endl;
	}
}


vk::Extent2D
PresentationContext::get_window_extent() const noexcept
{
	int width;
	int height;
	SDL_GetWindowSize(window_, &width, &height);
	return vk::Extent2D{}.setWidth(width).setHeight(height);
}

vk::Extent2D 
PresentationContext::window_resize_event_triggered() noexcept
{
	//TODO: Implement resize of swapchain
	return get_window_extent();
}

void PresentationContext::CreateSwapChain()
{
	const auto surface_capabilities =
		physical_device.getSurfaceCapabilitiesKHR(raw_window_surface_);

	const auto window_extent = get_window_extent();
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

	const std::vector<vk::SurfaceFormatKHR> formats =
		physical_device.getSurfaceFormatsKHR(raw_window_surface_);
	if (formats.empty())
		throw std::runtime_error("Swapchain has no formats!");
	
	swapchain_format_ = get_swapchain_surface_format(formats);

	std::cout << "> Swapchain format: " << vk::to_string(swapchain_format_.format) << std::endl;

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
		.setSurface(raw_window_surface_)
		.setMinImageCount(surface_image_count)
		.setImageFormat(swapchain_format_.format)
		.setImageColorSpace(swapchain_format_.colorSpace)
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
		graphics_index(graphics_present_indices_),
		present_index(graphics_present_indices_)
	};

	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present_indices_)) {
		swapChainCreateInfo
			.setImageSharingMode(vk::SharingMode::eExclusive);
	} 
	else {
		swapChainCreateInfo
			.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setQueueFamilyIndices(splitIndices);
	}

	swapchain_ = device->createSwapchainKHRUnique(swapChainCreateInfo, nullptr);

	swapchain_images_ = device->getSwapchainImagesKHR(*swapchain_);
	std::cout << "> SwapChain image count: " << swapchain_images_.size() << std::endl;
	
	swapchain_imageviews_.reserve(swapchain_images_.size());
	
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
		.setFormat(swapchain_format_.format)
		.setComponents(componentMapping);
	
	for (auto& image : swapchain_images_) {
		imageViewCreateInfo.setImage(image);
		swapchain_imageviews_.push_back(device->createImageViewUnique(imageViewCreateInfo));
	}

	std::cout << "> created SwapChain!" << std::endl;
}

void PresentationContext::CreateRenderTargets()
{
	for (size_t i = 0; i < swapchain_images_.size(); i++) {
		const auto window_extent = get_window_extent();
		const auto extent = vk::Extent3D{}
			.setWidth(window_extent.width)
			.setHeight(window_extent.height)
			.setDepth(1);
		
		Texture2D texture =
			create_empty_rendertarget_texture(physical_device,
											  device.get(),
											  vk::Format::eR8G8B8A8Srgb,
											  extent,
											  vk::ImageTiling::eOptimal,
											  vk::MemoryPropertyFlagBits::eDeviceLocal);

		with_buffer_submit(device.get(),
						   command_pool(),
						   graphics_queue(),
						   [&] (vk::CommandBuffer& commandbuffer)
						   {
							   texture.layout =
								  transition_image_for_color_override(get_image(texture),
																  commandbuffer);

						   });
		
		rendertargets_.push_back(std::move(texture));
	}
	std::cout << "> Created Render Targets" << std::endl;
}

void PresentationContext::CreateCommandpool()
{
    auto commandPoolCreateInfo = vk::CommandPoolCreateInfo{}
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(graphics_index(graphics_present_indices_));
	commandpool_ = device->createCommandPoolUnique(commandPoolCreateInfo, nullptr);

	std::cout << "> Created Command pool" << std::endl;
}

void PresentationContext::CreateCommandbuffers()
{
	// allocate a CommandBuffer from the CommandPool
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo
		.setCommandPool(*commandpool_)
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandBufferCount(maxFramesInFlight_);

	commandbuffers_ = device->allocateCommandBuffersUnique(commandBufferAllocateInfo);
	std::cout << "> Created " << commandbuffers_.size() << " Command buffers" << std::endl;
}

void PresentationContext::CreateSyncObjects()
{
	const auto semaphoreCreateInfo = vk::SemaphoreCreateInfo{};
	const auto fenceCreateInfo = vk::FenceCreateInfo{}
		.setFlags(vk::FenceCreateFlagBits::eSignaled);
	// Signaled means that the fence is engaged at creation, this
	// simplifies the first time we want to wait for it to not need
	// extra logic

	for (int i = 0; i < maxFramesInFlight_; i++) {
		imageAvailableSemaphores_.push_back(device->createSemaphoreUnique(semaphoreCreateInfo,
																		   nullptr));
		renderFinishedSemaphores_.push_back(device->createSemaphoreUnique(semaphoreCreateInfo,
																		   nullptr));
		inFlightFences_.push_back(device->createFenceUnique(fenceCreateInfo, nullptr));
	}

	std::cout << "> Created Sync Objects" << std::endl;
}

void
PresentationContext::RecordBlitTextureToSwapchain(vk::CommandBuffer& commandbuffer,
											 vk::Image& swapchain_image,
											 Texture2D* texture)
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
		const auto window_extent = get_window_extent();
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
		
		commandbuffer.blitImage(get_image(*texture),
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
	

void PresentationContext::with_presentation(FrameProducer& currentFrameGenerator)
{

	const auto maxTimeout = std::numeric_limits<unsigned int>::max();
	auto waitresult = device->waitForFences(*(inFlightFences_[current_frame_in_flight_]),
										 true,
										 maxTimeout);
	device->resetFences(*(inFlightFences_[current_frame_in_flight_]));

	if (waitresult != vk::Result::eSuccess)
		throw std::runtime_error("Could not wait for inFlightFence");
	
	auto [result, swapchain_index] =
		device->acquireNextImageKHR(*swapchain_,
									 maxTimeout,
									 *(imageAvailableSemaphores_[current_frame_in_flight_]));

	assert(result == vk::Result::eSuccess);
	assert(swapchain_index < swapchain_imageviews_.size());

	if (per_frame_debug_print) {
		const std::string line = ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>";
		std::cout << line 
				  << "\n> present flight index:    " << current_frame_in_flight_
				  << "\n> present swapchain index: " << swapchain_index 
				  << "\n> present total frames:    " << total_frames_ 
				  << std::endl;
	}
	
	CurrentFrameInfo currentFrameInfo;
	currentFrameInfo.current_flight_frame_index = current_frame_in_flight_;
	currentFrameInfo.total_frame_count = total_frames_;

	std::optional<Texture2D*> frameToPresent = std::invoke(currentFrameGenerator,
														   currentFrameInfo);
	if (!frameToPresent.has_value())
		throw std::runtime_error("SwapChain has not implemented a way to present the old"
								 " swapchain image if generator returns nullopt");
	
	RecordBlitTextureToSwapchain(commandbuffers_[current_frame_in_flight_].get(),
								 swapchain_images_[swapchain_index],
								 frameToPresent.value());

	const std::vector<vk::Semaphore> waitSemaphores{
		*(imageAvailableSemaphores_[current_frame_in_flight_]),
	};
	const std::vector<vk::Semaphore> signalSemaphores{
		*(renderFinishedSemaphores_[current_frame_in_flight_]),
	};
	
	const std::vector<vk::PipelineStageFlags> waitStages{
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
	};

	auto submitInfo = vk::SubmitInfo{}
		.setWaitSemaphores(waitSemaphores)
		.setWaitDstStageMask(waitStages)
		.setCommandBuffers(*(commandbuffers_[current_frame_in_flight_]))
		.setSignalSemaphores(signalSemaphores);
	
	::present_queue(index_queues_).submit(submitInfo,
										  *(inFlightFences_[current_frame_in_flight_]));

	const std::vector<vk::SwapchainKHR> swapchains = {*swapchain_};
	const std::vector<uint32_t> imageIndices = {swapchain_index};
	auto presentInfo = vk::PresentInfoKHR{}
		.setSwapchains(swapchains)
		.setImageIndices(imageIndices)
		.setWaitSemaphores(signalSemaphores);
	
	auto presentResult = present_queue(index_queues_).presentKHR(presentInfo);
	if (presentResult != vk::Result::eSuccess)
		throw std::runtime_error("Could not wait for inFlightFence");

    current_frame_in_flight_ = (current_frame_in_flight_ + 1) % maxFramesInFlight_;
	total_frames_++;
}

