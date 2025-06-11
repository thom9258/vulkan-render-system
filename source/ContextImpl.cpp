#include "ContextImpl.hpp"

#include <iostream>

namespace Render
{
	
Context::Impl::~Impl()
{
	instance->destroySurfaceKHR(raw_window_surface, nullptr);
	SDL_DestroyWindow(window);
	SDL_Vulkan_UnloadLibrary();
	SDL_Quit();
}

Context::Impl::Impl(WindowConfig const& config, Logger logger)
	: logger(logger)
{
	InitSDL();
	CreateWindow(config);
	CreateInstance();
	CreateWindowSurface();
	GetPhysicalDevice();
	GetQueueFamilyIndices();
	CreateDebugMessenger();
	CreateDevice();
	CreateIndexQueues();
	CreateCommandpool();
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
	

void Context::Impl::InitSDL()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
		logger.fatal(std::source_location::current(), 
					 "Could not initialize SDL");
		throw std::runtime_error("Could not init sdl!");
	}

    SDL_Vulkan_LoadLibrary(nullptr);
}
	
void Context::Impl::CreateInstance()
{
	std::vector<const char*> sdl_instance_extensions = get_sdl2_instance_extensions(window);
	auto instance_extensions = sdl_instance_extensions;
	instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	
	logger.info(std::source_location::current(), 
				"Instance Extensions:");
	for (const auto& extension: instance_extensions) {
		logger.info(std::source_location::current(), 
					std::format("\t{}", extension));
	}

	//NOTE Only do validation layers for debugging
	std::vector<const char*> validation_layers{
		"VK_LAYER_KHRONOS_validation"
	};
	
	logger.info(std::source_location::current(), 
				"Validation Layers:");
	for (auto wanted: validation_layers) {
		logger.info(std::source_location::current(), 
					std::format("\t{}", wanted));

		if (!is_validation_layer_available(wanted)) {
			logger.fatal(std::source_location::current(), 
						 std::format("Validation layer not available: {}",
									 wanted));
			throw std::runtime_error("Validation layer not available");
		}
	}
	
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
	
    instance = vk::createInstanceUnique(instanceCreateInfo);
	logger.info(std::source_location::current(), "Created Vulkan Instance!");
}
	
void Context::Impl::CreateWindow(WindowConfig const& config)
{
	int pos_x = -1;
	int pos_y = -1;
	if (config.position.has_value()) {
		pos_x = config.position.value().width();
		pos_y = config.position.value().height();
	}

	window = SDL_CreateWindow(config.name.c_str(),
							  pos_x,
							  pos_y,
							  config.size.width(),
							  config.size.height(),
							  0 | SDL_WINDOW_VULKAN);
}
	
void Context::Impl::GetPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> devices = instance->enumeratePhysicalDevices();
	physical_device = devices.front();
	logger.warn(std::source_location::current(),
				"TODO: A rating system for devices is not implemented yet.");
	logger.info(std::source_location::current(), 
				std::format("Vulkan Device: {}",
							PhysicalDevice_string(physical_device)));
}
	
void Context::Impl::CreateWindowSurface()
{
	SDL_Vulkan_CreateSurface(window, *instance, &raw_window_surface);
}

void Context::Impl::CreateDebugMessenger()
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
	logger.warn(std::source_location::current(), 
				"TODO: Cannot Create Debug Messenger!");
}
	
void Context::Impl::GetQueueFamilyIndices()
{
	auto graphics_present = get_graphics_present_indices(physical_device,
														 raw_window_surface);

	if (!graphics_present) {
		const auto msg = "could not get graphics and present indices";
		logger.fatal(std::source_location::current(), msg);
		throw std::runtime_error(msg);
	}

	graphics_present_indices = *graphics_present;
	
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present_indices)) {
		logger.info(std::source_location::current(),
					"found SHARED graphics present indices");
	}
	else {
		logger.warn(std::source_location::current(),
					"found SPLIT graphics present indices");
	}
}
	
void Context::Impl::CreateIndexQueues()
{
	index_queues = get_index_queues(*device,
									graphics_present_indices);

	if (std::holds_alternative<SharedIndexQueue>(index_queues)) {
		logger.info(std::source_location::current(),
					"created SHARED index queue");
	}
	else {
		logger.info(std::source_location::current(),
					"created SPLIT index queues");
	}
}

void Context::Impl::CreateDevice()
{
	float queuePriority = 1.0f;
	uint32_t device_index = present_index(graphics_present_indices);

	auto deviceQueueCreateInfo = vk::DeviceQueueCreateInfo{}
		.setFlags({})
		.setQueueFamilyIndex(device_index)
		.setPQueuePriorities(&queuePriority)
		.setQueueCount(1);

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	
	logger.info(std::source_location::current(), "requred device extensions:");
	for (auto extension: device_extensions) {
		logger.info(std::source_location::current(), 
					std::format("\t{}", extension));
	}
	
	// TODO: We actually need to check if these are available,
	//       if not, we can:
	//       1. discard the device alltogether
	//       2. dont make use of the functionality in our code
	logger.warn(std::source_location::current(), 
				"TODO: We need to check device features are available"
				" before trying to enable them");
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
	logger.info(std::source_location::current(), "Created Logical Device!");
}	

void Context::Impl::CreateCommandpool()
{
    auto commandPoolCreateInfo = vk::CommandPoolCreateInfo{}
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
		.setQueueFamilyIndex(graphics_index(graphics_present_indices));
	commandpool = device->createCommandPoolUnique(commandPoolCreateInfo, nullptr);
	logger.info(std::source_location::current(), "Created Graphics Command pool!");
}

vk::Extent2D
Context::Impl::get_window_extent() const noexcept
{
	int width;
	int height;
	SDL_GetWindowSize(window, &width, &height);
	return vk::Extent2D{}.setWidth(width).setHeight(height);
}

vk::Extent2D 
Context::Impl::window_resize_event_triggered() noexcept
{
	logger.warn(std::source_location::current(), 
				"TODO: resize event handling is not handled yet..");
	return get_window_extent();
}

void Context::Impl::wait_until_idle() noexcept
{
	device.get().waitIdle();
}
	
vk::Queue& Context::Impl::graphics_queue()
{
	return ::graphics_queue(index_queues);
}

auto Context::Impl::window_surface_capabilities()
	-> vk::SurfaceCapabilitiesKHR
{
	return physical_device.getSurfaceCapabilitiesKHR(raw_window_surface);
}

auto Context::Impl::window_surface_formats()
	-> std::vector<vk::SurfaceFormatKHR>
{
	return physical_device.getSurfaceFormatsKHR(raw_window_surface);
}


Context::~Context()
{
}

Context::Context(WindowConfig const& config, Logger logger)
  : impl(std::make_unique<Impl>(config, logger))
{
}

U32Extent Context::get_window_extent() const noexcept
{
	vk::Extent2D extent = impl->get_window_extent();
	return U32Extent{extent.width, extent.height};
}

U32Extent Context::window_resize_event_triggered() noexcept
{
	vk::Extent2D extent = impl->window_resize_event_triggered();
	return U32Extent{extent.width, extent.height};
}

void Context::wait_until_idle() noexcept
{
	impl->wait_until_idle();
}

}
