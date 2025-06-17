#pragma once

#include <VulkanRenderer/Context.hpp>
#include "Utils.hpp"
#include "DebugMessenger.hpp"

namespace Render
{

class Context::Impl 
{
public:
    explicit Impl(WindowConfig const& config, Logger logger);
    ~Impl(); 

	vk::Extent2D get_window_extent() const noexcept;
	vk::Extent2D window_resize_event_triggered() noexcept;
	void wait_until_idle() noexcept;
	vk::Queue& graphics_queue();

	auto window_surface_capabilities()
		-> vk::SurfaceCapabilitiesKHR;
	auto window_surface_formats()
		-> std::vector<vk::SurfaceFormatKHR>;

	Logger logger;
	vk::UniqueInstance instance;
	SDL_Window* window;
	VkSurfaceKHR raw_window_surface;
	vk::PhysicalDevice physical_device;
	GraphicsPresentIndices graphics_present_indices;
	vk::UniqueDevice device;
	IndexQueues index_queues;
	vk::UniqueCommandPool commandpool;

private:	
	void InitSDL();
	void CreateWindow(WindowConfig const& config);
	void CreateInstance();
	void CreateWindowSurface();
	void GetPhysicalDevice();
	void GetQueueFamilyIndices();
	void CreateDebugMessenger();
	void CreateDevice();
	void CreateIndexQueues();
	void CreateCommandpool();
};

}
