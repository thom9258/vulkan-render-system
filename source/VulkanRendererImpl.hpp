#pragma once
#ifndef _VULKANRENDERERIMPL_PRESENTATIONCONTEXT_IMPL_
#define _VULKANRENDERERIMPL_PRESENTATIONCONTEXT_IMPL_

#include <VulkanRenderer/VulkanRenderer.hpp>

//#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include "Utils.hpp"
#include "DebugMessenger.hpp"

class PresentationContext::Impl 
{
public:
    explicit Impl(WindowConfig const& config, Logger logger);
    ~Impl(); 
	
	Logger logger;

	vk::Extent2D get_window_extent() const noexcept;
	vk::Extent2D window_resize_event_triggered() noexcept;
	void with_presentation(FrameProducer& f);
	
	void wait_until_idle() noexcept;

	vk::CommandPool& command_pool();
	vk::Queue& graphics_queue();
	
	const bool per_frame_debug_print{false};

	const int maxFramesInFlight_ = 2;
	uint32_t current_frame_in_flight_{0};
	uint64_t total_frames_{0};

	SDL_Window* window_;
	vk::UniqueInstance instance_;
	//TODO: I cannot seem to load the function pointers for debugutils messenger..
	//vk::UniqueDebugUtilsMessengerEXT debug_messenger_;
	vk::PhysicalDevice physical_device;
	//TODO: get some automatic destructon onto this surface
	VkSurfaceKHR raw_window_surface_;
	GraphicsPresentIndices graphics_present_indices_;
	vk::UniqueDevice device;
	IndexQueues index_queues_;

	vk::SurfaceFormatKHR swapchain_format_;
	vk::UniqueSwapchainKHR swapchain_;
	vk::UniqueCommandPool commandpool_;

	/*Per swapchain image*/
	std::vector<vk::Image> swapchain_images_;
	std::vector<vk::UniqueImageView> swapchain_imageviews_;
	std::vector<Texture2D> rendertargets_;


	/*Per Frame-in-Flight*/
	std::vector<vk::UniqueCommandBuffer> commandbuffers_;
	std::vector<vk::UniqueSemaphore> imageAvailableSemaphores_;
	std::vector<vk::UniqueSemaphore> renderFinishedSemaphores_;
	std::vector<vk::UniqueFence> inFlightFences_;
	
private:
	void CreateContext();
	void CreateWindow(WindowConfig const& config);
	void CreateInstance();
	void CreateWindowSurface();
	void CreateDebugMessenger();
	void GetPhysicalDevice();
	void GetQueueFamilyIndices();
	void CreateDevice();
	void CreateIndexQueues();
	void CreateSwapChain();
	void CreateCommandpool();
	void CreateCommandbuffers();
	void CreateRenderTargets();
	void CreateSyncObjects();

	void RecordBlitTextureToSwapchain(vk::CommandBuffer& commandbuffer,
									  vk::Image& swapchain_image,
									  Texture2D* texture);

};

#endif //_VULKANRENDERERIMPL_PRESENTATIONCONTEXT_IMPL
