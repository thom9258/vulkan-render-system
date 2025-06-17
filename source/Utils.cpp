#include "Utils.hpp"

#include <polymorph/polymorph.hpp>

[[nodiscard]]
const std::string
MemoryType_string(const vk::MemoryType& type) {
	std::stringstream ss{};
	size_t bitcount = 0;

	const auto add_propertyflag = [&ss, &bitcount] (const char* bitstr) {
		if (bitcount > 0)
			ss << " | " << bitstr;
		else
			ss << bitstr;
		bitcount++;
	};
	
	ss << "[HeapIndex: " << type.heapIndex << "]  ";
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
		add_propertyflag("MemoryPropertyFlagBits::eDeviceLocal");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
		add_propertyflag("MemoryPropertyFlagBits::eHostVisible");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCached)
		add_propertyflag("MemoryPropertyFlagBits::eHostCached");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
		add_propertyflag("MemoryPropertyFlagBits::eHostCoherent");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eLazilyAllocated)
		add_propertyflag("MemoryPropertyFlagBits::eLazilyAllocated");
	if (type.propertyFlags & vk::MemoryPropertyFlagBits::eProtected)
		add_propertyflag("MemoryPropertyFlagBits::eProtected");
	return ss.str();
}

[[nodiscard]]
const std::string
PhysicalDeviceType_string(const vk::PhysicalDeviceType& type) {
	if (type == vk::PhysicalDeviceType::eCpu)
		return "PhysicalDeviceType::eCpu";
	if (type == vk::PhysicalDeviceType::eDiscreteGpu)
		return "PhysicalDeviceType::eDiscreteGpu";
	if (type == vk::PhysicalDeviceType::eIntegratedGpu)
		return "PhysicalDeviceType::eIntegratedGpu";
	if (type == vk::PhysicalDeviceType::eVirtualGpu)
		return "PhysicalDeviceType::eVirtualGpu";
	return "PhysicalDeviceType::eOther";
}

[[nodiscard]]
const std::string
PhysicalDevice_string(const vk::PhysicalDevice& physicaldevice)
{
	std::stringstream ss{};
	ss << physicaldevice.getProperties().deviceName << "\n"
	   << "> Type: "
	   << PhysicalDeviceType_string(physicaldevice.getProperties().deviceType) << std::endl;
	
	auto memory_properties = physicaldevice.getMemoryProperties();
	for (size_t i = 0; i < memory_properties.memoryHeapCount; i++) {
		const auto memorytype_str = MemoryType_string(memory_properties.memoryTypes[i]);
		ss << "> Propertyflags: " << memorytype_str << std::endl;
		ss << "> Heap size: " << memory_properties.memoryHeaps[i].size << std::endl;
	}
	
	auto properties = physicaldevice.getProperties();
	auto features = physicaldevice.getFeatures();
	std::cout << "> wireframe mode supported: " 
			  << std::boolalpha << features.fillModeNonSolid 
			  << "> max Anisotrophy supported: " 
			  << properties.limits.maxSamplerAnisotropy
			  << std::endl;
	
	return ss.str();
}

[[nodiscard]]
std::vector<const char*>
get_sdl2_instance_extensions(SDL_Window* window)
{
    uint32_t length;
    SDL_Vulkan_GetInstanceExtensions(window, &length, nullptr);
	std::vector<const char*> extensions(length);
    SDL_Vulkan_GetInstanceExtensions(window, &length, extensions.data());
	return extensions;
}

[[nodiscard]]
std::vector<uint32_t>
get_all_graphics_queue_family_indices(const vk::PhysicalDevice& device)
{
	using enumerated_properties = polymorph::enumerated<vk::QueueFamilyProperties>;
	const auto has_graphics_index = [] (const enumerated_properties& eps)
	{ 
		return static_cast<bool>(eps.value().queueFlags & vk::QueueFlagBits::eGraphics);
	};
	const auto take_index = [] (const enumerated_properties& eps)
	{ 
		return static_cast<uint32_t>(eps.index());
	};

	return device.getQueueFamilyProperties() 
		>>= polymorph::enumerate()
		>>= polymorph::filter(has_graphics_index)
		>>= polymorph::transform(take_index);
}
[[nodiscard]]
std::vector<uint32_t>
get_all_present_queue_family_indices(const vk::SurfaceKHR& surface,
									 const vk::PhysicalDevice& device)
{
	using enumerated_properties = polymorph::enumerated<vk::QueueFamilyProperties>;
	const auto has_present_index = [&] (const enumerated_properties& eps)
	{ 
		return device.getSurfaceSupportKHR(eps.index(), surface);
	};
	const auto take_index = [] (const enumerated_properties& eps)
	{ 
		return static_cast<uint32_t>(eps.index());
	};

	return device.getQueueFamilyProperties()
		>>= polymorph::enumerate()
		>>= polymorph::filter(has_present_index)
		>>= polymorph::transform(take_index);
}

[[nodiscard]]
uint32_t
present_index(GraphicsPresentIndices& graphics_present)
{
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present)) {
		return std::get<SharedGraphicsPresentIndex>(graphics_present).shared;
	}
	return std::get<SplitGraphicsPresentIndices>(graphics_present).present;
}

[[nodiscard]]
uint32_t
graphics_index(GraphicsPresentIndices& graphics_present)
{
	if (std::holds_alternative<SharedGraphicsPresentIndex>(graphics_present)) {
		return std::get<SharedGraphicsPresentIndex>(graphics_present).shared;
	}

	return std::get<SplitGraphicsPresentIndices>(graphics_present).graphics;
}



[[nodiscard]]
std::optional<uint32_t>
find_matching_queue_family_indices(const std::vector<uint32_t> a,
								   const std::vector<uint32_t> b)
{
	for (const auto index: a) {
		const auto matching = std::ranges::find(b, index);
		if (matching != b.end())
			return *matching;
	}

	return {};
}

[[nodiscard]]
std::optional<GraphicsPresentIndices>
get_graphics_present_indices(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface)
{
	const auto graphics_indices = get_all_graphics_queue_family_indices(physical_device);
	std::cout << "> graphics flags: ";
	graphics_indices >>= polymorph::stream(std::cout, " ", "\n");
	const auto present_indices = get_all_present_queue_family_indices(surface, physical_device);
	std::cout << "> present flags: ";
	present_indices >>= polymorph::stream(std::cout, " ", "\n");
	
	if (graphics_indices.empty() || present_indices.empty())
		return std::nullopt;
	
	const auto matching = find_matching_queue_family_indices(graphics_indices,
															 present_indices);
	
	if (matching)
		return SharedGraphicsPresentIndex{*matching};
	return SplitGraphicsPresentIndices{graphics_indices[0], present_indices[0]};
}

[[nodiscard]]
IndexQueues
get_index_queues(const vk::Device& device, const GraphicsPresentIndices& indices)
{
	if (std::holds_alternative<SharedGraphicsPresentIndex>(indices)) {
		const auto index = std::get<SharedGraphicsPresentIndex>(indices).shared;
		return SharedIndexQueue{device.getQueue(index, 0)};
	}

	const auto graphics = std::get<SplitGraphicsPresentIndices>(indices).graphics;
	const auto present = std::get<SplitGraphicsPresentIndices>(indices).present;
	return SplitIndexQueues{device.getQueue(graphics, 0),
		                    device.getQueue(present, 1)};
}

[[nodiscard]]
vk::Queue&
present_queue(IndexQueues& queues)
{
	if (std::holds_alternative<SharedIndexQueue>(queues)) {
		return std::get<SharedIndexQueue>(queues).shared;
	}
	return std::get<SplitIndexQueues>(queues).present;
}

[[nodiscard]]
vk::Queue&
graphics_queue(IndexQueues& queues)
{
	if (std::holds_alternative<SharedIndexQueue>(queues)) {
		return std::get<SharedIndexQueue>(queues).shared;
	}
	return std::get<SplitIndexQueues>(queues).graphics;
}

[[nodiscard]]
vk::SurfaceFormatKHR
get_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& availables) {
    for (const auto& available : availables) {
        if (available.format == vk::Format::eB8G8R8A8Srgb 
		 && available.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available;
        }
    }

    return availables.front();
}

[[nodiscard]]
std::optional<std::vector<uint32_t>>
read_binary_file(const std::string& filename) 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        return std::nullopt;

    size_t bytecount = static_cast<size_t>(file.tellg());
	constexpr size_t scaling_factor = sizeof(uint32_t) / sizeof(char);
    size_t read_times = bytecount / scaling_factor;
    std::vector<uint32_t> buffer;
	buffer.resize(read_times);
	//std::cout << "Byte Count: " << bytecount 
	//<< "\nBuffer Length: " << buffer.size() 
	//<< "\nBuffer Length * 4: " << buffer.size() * 4
	//<< "\nScaling Factor: " << scaling_factor
	//<< std::endl;
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), sizeof(buffer[0]) * buffer.size());
    file.close();
    return buffer;
}


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
			   vk::MemoryPropertyFlags requirementsMask)
{
	uint32_t typeIndex = uint32_t( ~0 );
	for ( uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++ ) {
		if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)) {
			typeIndex = i;
			break;
		}
		typeBits >>= 1;
	}
	assert( typeIndex != uint32_t( ~0 ) );
	return typeIndex;
}

AllocatedMemory
allocate_memory(vk::PhysicalDevice& physical_device,
				vk::Device& device,
				const vk::DeviceSize size,
				const vk::BufferUsageFlags usage,
				const vk::MemoryPropertyFlags properties)
{
	const auto bufferInfo = vk::BufferCreateInfo{}
		.setSize(size)
		.setUsage(usage)
		.setSharingMode(vk::SharingMode::eExclusive);

	AllocatedMemory buffer_and_memory{};
	buffer_and_memory.buffer = device.createBufferUnique(bufferInfo, nullptr);

	vk::PhysicalDeviceMemoryProperties memProperties = physical_device.getMemoryProperties();
    vk::MemoryRequirements memRequirements =
		device.getBufferMemoryRequirements(*(buffer_and_memory.buffer));

	const auto memoryTypeIndex = findMemoryType(memProperties,
												memRequirements.memoryTypeBits,
												properties);

    auto allocInfo = vk::MemoryAllocateInfo{}
		.setAllocationSize(memRequirements.size)
		.setMemoryTypeIndex(memoryTypeIndex);

	buffer_and_memory.memory = device.allocateMemoryUnique(allocInfo, nullptr);
	device.bindBufferMemory(*(buffer_and_memory.buffer),
							*(buffer_and_memory.memory),
							0);

	return buffer_and_memory;
}

void
copy_to_allocated_memory(vk::Device& device,
						 AllocatedMemory& allocated_memory,
						 void const* data,
						 const size_t size)
{
	void* staging_ptr = device.mapMemory(allocated_memory.memory.get(),
										 0,
										 size,
										 vk::MemoryMapFlags());
	memcpy(staging_ptr, data, size);
	device.unmapMemory(allocated_memory.memory.get());
}


AllocatedMemory
create_staging_buffer(vk::PhysicalDevice& physical_device,
					  vk::Device& device,
					  void const* data,
					  const vk::DeviceSize size)
{
	AllocatedMemory staging =
		allocate_memory(physical_device,
							  device, 
							  size,
							  vk::BufferUsageFlagBits::eTransferSrc,
							  vk::MemoryPropertyFlagBits::eHostVisible
							  | vk::MemoryPropertyFlagBits::eHostCoherent);

	copy_to_allocated_memory(device,
							 staging,
							 data,
							 size);
	return staging;
}

vk::Image&
get_image(AllocatedImage& allocatedImage)
{
	return allocatedImage.image.get();
}

AllocatedImage
allocate_image(vk::PhysicalDevice physical_device,
			   vk::Device device,
			   const vk::Extent3D extent,
			   const vk::Format format,
			   const vk::ImageTiling tiling,
			   const vk::MemoryPropertyFlags propertyFlags,
			   const vk::ImageUsageFlags usage) noexcept
{
	const auto imageCreateInfo = vk::ImageCreateInfo{}
		.setImageType(vk::ImageType::e2D)
		.setFormat(format)
		.setExtent(extent)
		.setMipLevels(1)
		.setArrayLayers(1)
		.setTiling(tiling)
		.setUsage(usage) 
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSamples(vk::SampleCountFlagBits::e1);
	
	AllocatedImage out{};
	out.image = device.createImageUnique(imageCreateInfo);
	
	vk::PhysicalDeviceMemoryProperties memProperties = physical_device.getMemoryProperties();
    vk::MemoryRequirements memRequirements =
		device.getImageMemoryRequirements(out.image.get());

	const auto memoryTypeIndex = findMemoryType(memProperties,
												memRequirements.memoryTypeBits,
												propertyFlags);

    auto allocInfo = vk::MemoryAllocateInfo{}
		.setAllocationSize(memRequirements.size)
		.setMemoryTypeIndex(memoryTypeIndex);
	out.memory = device.allocateMemoryUnique(allocInfo, nullptr);

	device.bindImageMemory(out.image.get(), out.memory.get(), 0);
	return out;
}

vk::UniqueCommandBuffer
beginSingleTimeCommands(vk::Device& device,
						vk::CommandPool& command_pool) 
{
    auto allocInfo = vk::CommandBufferAllocateInfo{}
		.setLevel(vk::CommandBufferLevel::ePrimary)
		.setCommandPool(command_pool)
		.setCommandBufferCount(1);

    vk::UniqueCommandBuffer commandBuffer =
		std::move(device.allocateCommandBuffersUnique(allocInfo).front());

    auto beginInfo = vk::CommandBufferBeginInfo{}
		.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer->begin(beginInfo);
    return commandBuffer;
}


void
endSingleTimeCommands(vk::Queue& queue,
					  vk::CommandBuffer& command_buffer) 
{
	command_buffer.end();
	auto submitinfo = vk::SubmitInfo{}
		.setCommandBuffers(command_buffer);
	queue.submit(submitinfo);
	queue.waitIdle();
}

void
with_buffer_submit(vk::Device& device,
				   vk::CommandPool& command_pool,
				   vk::Queue& queue,
				   std::function<void(vk::CommandBuffer&)>&& f)
{
	auto buffer = beginSingleTimeCommands(device, command_pool);
	f(buffer.get());
	endSingleTimeCommands(queue, buffer.get());
}

void
copy_buffer(vk::Device& device,
			vk::CommandPool& command_pool,
			vk::Queue& queue,
			vk::Buffer& src,
			vk::Buffer& dst,
			vk::DeviceSize size)
{
	with_buffer_submit(device, command_pool, queue,
						[&] (vk::CommandBuffer& commandbuffer)
						{
							auto region = vk::BufferCopy{}
								.setSize(size);
							commandbuffer.copyBuffer(src,dst, region);
						});
}

void
transition_image_layout(vk::Image& image,
						const vk::ImageLayout old_layout,
						const vk::ImageLayout new_layout,
						vk::CommandBuffer& commandbuffer)
{
	auto range = vk::ImageSubresourceRange{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);
	
	auto barrier = vk::ImageMemoryBarrier{}
		.setOldLayout(old_layout)
		.setNewLayout(new_layout)
		.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
		.setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
		.setImage(image)
		.setSubresourceRange(range);

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (old_layout == vk::ImageLayout::eUndefined 
		&& new_layout == vk::ImageLayout::eTransferDstOptimal) {
		barrier
			.setSrcAccessMask(vk::AccessFlags())
			.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (old_layout == vk::ImageLayout::eTransferDstOptimal
			 && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {

		barrier
			.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	commandbuffer.pipelineBarrier(sourceStage,
								  destinationStage,
								  vk::DependencyFlags(), 
								  nullptr,
								  nullptr,
								  barrier);
};

vk::ImageSubresourceRange 
image_subresource_range(const vk::ImageAspectFlags aspect_mask)
{
    return vk::ImageSubresourceRange{}
		.setAspectMask(aspect_mask)
		.setBaseMipLevel(0)
		.setLevelCount(VK_REMAINING_MIP_LEVELS)
		.setBaseArrayLayer(0)
		.setLayerCount(VK_REMAINING_ARRAY_LAYERS);
}

/**
* https://vkguide.dev/docs/chapter-5/loading_images/
*/
vk::ImageLayout 
transition_image_for_color_override(vk::Image& image,
									vk::CommandBuffer& commandbuffer)
{
    auto range = vk::ImageSubresourceRange{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setBaseMipLevel(0)
		.setLevelCount(1)
		.setBaseArrayLayer(0)
		.setLayerCount(1);

	auto barrier = vk::ImageMemoryBarrier{}
		.setOldLayout(vk::ImageLayout::eUndefined)
		.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
		.setImage(image)
		.setSubresourceRange(range)
		.setSrcAccessMask(vk::AccessFlags())
		.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

    commandbuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
								  vk::PipelineStageFlagBits::eTransfer,
								  vk::DependencyFlags(),
								  nullptr,
								  nullptr,
								  barrier);

	return vk::ImageLayout::eTransferDstOptimal;
}

void
copy_buffer_to_image(vk::Buffer& buffer,
					 vk::Image& image,
					 const uint32_t width,
					 const uint32_t height,
					 vk::CommandBuffer& commandbuffer)
{
	auto subresource = vk::ImageSubresourceLayers{}
		.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setMipLevel(0)
		.setBaseArrayLayer(0)
		.setLayerCount(1);
	
	auto offset = vk::Offset3D{}
		.setX(0)
		.setY(0)
		.setZ(0);
	
	auto extent = vk::Extent3D{}
		.setWidth(width)
		.setHeight(height)
		.setDepth(1);
	
	auto region = vk::BufferImageCopy{}
		.setBufferOffset(0)
		.setBufferRowLength(0)
		.setBufferImageHeight(0)
		.setImageSubresource(subresource)
		.setImageOffset(offset)
		.setImageExtent(extent);
	
	commandbuffer.copyBufferToImage(buffer,
									image,
									vk::ImageLayout::eTransferDstOptimal,
									region);
}



