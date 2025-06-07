#pragma once
#ifndef _VULKANRENDERER_UTILS_
#define _VULKANRENDERER_UTILS_

//#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <filesystem>
#include <functional>
#include <fstream>
#include <variant>
#include <array>
#include <optional>

[[nodiscard]]
const std::string
MemoryType_string(const vk::MemoryType& type);

[[nodiscard]]
const std::string
PhysicalDeviceType_string(const vk::PhysicalDeviceType& type);

[[nodiscard]]
const std::string
PhysicalDevice_string(const vk::PhysicalDevice& physicaldevice);

[[nodiscard]]
std::vector<const char*>
get_sdl2_instance_extensions(SDL_Window* window);

[[nodiscard]]
std::vector<uint32_t>
get_all_graphics_queue_family_indices(const vk::PhysicalDevice& device);

[[nodiscard]]
std::vector<uint32_t>
get_all_present_queue_family_indices(const vk::SurfaceKHR& surface,
									 const vk::PhysicalDevice& device);

struct SharedGraphicsPresentIndex {
	uint32_t shared;
};

struct SplitGraphicsPresentIndices {
	uint32_t graphics;
	uint32_t present;
};

using GraphicsPresentIndices = std::variant<SharedGraphicsPresentIndex,
											SplitGraphicsPresentIndices>;

[[nodiscard]]
uint32_t
present_index(GraphicsPresentIndices& graphics_present);

[[nodiscard]]
uint32_t
graphics_index(GraphicsPresentIndices& graphics_present);

[[nodiscard]]
std::optional<uint32_t>
find_matching_queue_family_indices(const std::vector<uint32_t> a,
								   const std::vector<uint32_t> b);


[[nodiscard]]
std::optional<GraphicsPresentIndices>
get_graphics_present_indices(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

struct SharedIndexQueue {
	vk::Queue shared;
};

struct SplitIndexQueues {
	vk::Queue graphics;
	vk::Queue present;
};

using IndexQueues = std::variant<SharedIndexQueue,
								 SplitIndexQueues>;

[[nodiscard]]
IndexQueues
get_index_queues(const vk::Device& device, const GraphicsPresentIndices& indices);

[[nodiscard]]
vk::Queue&
present_queue(IndexQueues& queues);


[[nodiscard]]
vk::Queue&
graphics_queue(IndexQueues& queues);


[[nodiscard]]
vk::SurfaceFormatKHR
get_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& availables);

[[nodiscard]]
std::optional<std::vector<uint32_t>>
read_binary_file(const std::string& filename);

#if 0
[[nodiscard]]
std::optional<uint32_t>
find_memory_type(const vk::PhysicalDeviceMemoryProperties& memoryProperties,
				 uint32_t typeBits,
				 const vk::MemoryPropertyFlags requirementsMask)
{
	uint32_t typeIndex = uint32_t( ~0 );
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		const bool has_requirements =
			((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask);

		if ((typeBits & 1) && has_requirements) {
			typeIndex = i;
			break;
		}
		typeBits >>= 1;
	}

	if (typeIndex == uint32_t( ~0 ) )
		return {};
	return typeIndex;
}

uint32_t findMemoryType(vk::BufferUsageFlags usage,
						vk::MemoryPropertyFlags properties,
						vk::PhysicalDeviceMemoryProperties memProperties)
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}
#endif

uint32_t 
findMemoryType(vk::PhysicalDeviceMemoryProperties const & memoryProperties,
			   uint32_t typeBits,
			   vk::MemoryPropertyFlags requirementsMask);

struct AllocatedMemory
{
	vk::UniqueBuffer buffer; 
	vk::UniqueDeviceMemory memory;
};

AllocatedMemory
allocate_memory(vk::PhysicalDevice& physical_device,
				vk::Device& device,
				const vk::DeviceSize size,
				const vk::BufferUsageFlags usage,
				const vk::MemoryPropertyFlags properties);

void
copy_to_allocated_memory(vk::Device& device,
						 AllocatedMemory& allocated_memory,
						 void const* data,
						 const size_t size);

AllocatedMemory
create_staging_buffer(vk::PhysicalDevice& physical_device,
					  vk::Device& device,
					  void const* data,
					  const vk::DeviceSize size);

struct AllocatedImage
{
	vk::UniqueImage image;
	vk::UniqueDeviceMemory memory;
};

vk::Image&
get_image(AllocatedImage& allocatedImage);

AllocatedImage
allocate_image(vk::PhysicalDevice physical_device,
			   vk::Device device,
			   const vk::Extent3D extent,
			   const vk::Format format,
			   const vk::ImageTiling tiling,
			   const vk::MemoryPropertyFlags propertyFlags,
			   const vk::ImageUsageFlags usage) noexcept;

vk::UniqueCommandBuffer
beginSingleTimeCommands(vk::Device& device,
						vk::CommandPool& command_pool);

void
endSingleTimeCommands(vk::Queue& queue,
					  vk::CommandBuffer& command_buffer);

void
with_buffer_submit(vk::Device& device,
				   vk::CommandPool& command_pool,
				   vk::Queue& queue,
				   std::function<void(vk::CommandBuffer&)>&& f);

void
copy_buffer(vk::Device& device,
			vk::CommandPool& command_pool,
			vk::Queue& queue,
			vk::Buffer& src,
			vk::Buffer& dst,
			vk::DeviceSize size);

void
transition_image_layout(vk::Image& image,
						const vk::ImageLayout old_layout,
						const vk::ImageLayout new_layout,
						vk::CommandBuffer& commandbuffer);

vk::ImageSubresourceRange 
image_subresource_range(const vk::ImageAspectFlags aspect_mask);

/**
* https://vkguide.dev/docs/chapter-5/loading_images/
*/
vk::ImageLayout 
transition_image_for_color_override(vk::Image& image,
									vk::CommandBuffer& commandbuffer);

void
copy_buffer_to_image(vk::Buffer& buffer,
					 vk::Image& image,
					 const uint32_t width,
					 const uint32_t height,
					 vk::CommandBuffer& commandbuffer);

#endif //_VULKANRENDERER_UTILS_
