#pragma once

#include <VulkanRenderer/Presenter.hpp>
#include "ContextImpl.hpp"
#include <VulkanRenderer/Utils.hpp>
#include <VulkanRenderer/Texture.hpp>

class Presenter::Impl 
{
public:
    explicit Impl(Render::Context::Impl* context);
    ~Impl();

	void with_presentation(FrameProducer& f);
	vk::CommandPool& command_pool();

	Render::Context::Impl* context;
	const bool per_frame_debug_print{false};
	//TODO we should have a smart little object to handle this logic...
	const int max_frames_in_flight = 2;
	uint32_t current_frame_in_flight{0};
	uint64_t total_frames{0};

	vk::SurfaceFormatKHR swapchain_format;
	vk::UniqueSwapchainKHR swapchain;
	vk::UniqueCommandPool commandpool;

	/*Per swapchain image*/
	std::vector<vk::Image> swapchain_images;
	std::vector<vk::UniqueImageView> swapchain_imageviews;
	std::vector<Texture2D::Impl> rendertargets;


	/*Per Frame-in-Flight*/
	std::vector<vk::UniqueCommandBuffer> commandbuffers;
	std::vector<vk::UniqueSemaphore> imageAvailableSemaphores;
	std::vector<vk::UniqueSemaphore> renderFinishedSemaphores;
	std::vector<vk::UniqueFence> inFlightFences;
	
private:
	void CreateSwapChain();
	void CreateCommandpool();
	void CreateCommandbuffers();
	void CreateRenderTargets();
	void CreateSyncObjects();

	void RecordBlitTextureToSwapchain(vk::CommandBuffer& commandbuffer,
									  vk::Image& swapchain_image,
									  Texture2D::Impl* texture);

};
