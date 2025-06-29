#pragma once

#include "ShaderTextureImpl.hpp"
#include <vulkan/vulkan.hpp>

#include <map>

auto create_texture_descriptorset(vk::Device device,
								  vk::DescriptorSetLayout descriptorset_layout,
								  vk::DescriptorPool descriptor_pool,
								  size_t frames_in_flight,
								  TextureSamplerReadOnly& texture)
	-> std::vector<vk::UniqueDescriptorSet>;


auto create_shaderstage_infos(vk::Device device,
							  std::filesystem::path const vertex_path,
							  std::filesystem::path const fragment_path)
	noexcept -> std::optional<std::array<vk::PipelineShaderStageCreateInfo, 2>>;


auto create_texture_descriptorset_layout(vk::Device device,
										 uint32_t binding_index)
	-> vk::UniqueDescriptorSetLayout;


struct CachedTextureDescriptorBinder
{	
	TextureSamplerReadOnly m_default;
	TextureSamplerReadOnly* m_last_bound{nullptr};
	vk::UniqueDescriptorSetLayout m_layout;
	std::map<TextureSamplerReadOnly*, std::vector<vk::UniqueDescriptorSet>> m_sets;

	CachedTextureDescriptorBinder() = default;

	CachedTextureDescriptorBinder(vk::Device device,
								  vk::UniqueDescriptorSetLayout&& descriptorset_layout,
								  vk::DescriptorPool descriptor_pool,
								  size_t frames_in_flight,
								  TextureSamplerReadOnly&& default_texture);

	void bind_texture_descriptor(vk::Device device,
								 vk::PipelineLayout pipeline_layout,
								 uint32_t binding_index,
								 vk::DescriptorPool descriptor_pool,
								 size_t frames_in_flight,
								 vk::CommandBuffer& commandbuffer,
								 size_t frame_in_flight,
								 TextureSamplerReadOnly* texture);
	

#if 0
	//auto layout()
	//const noexcept -> vk::DescriptorSetLayout;
	void flush_cache(vk::Device device,
					 vk::DescriptorPool descriptor_pool,
					 size_t frames_in_flight);
#endif					 
};

template<typename Data>
struct UniformBuffer
{
	using UniformDataType = std::remove_cvref_t<Data>;

	AllocatedMemory m_memory;
	vk::UniqueDescriptorSet m_set;

	explicit UniformBuffer(UniformDataType* init,
						   Logger& logger,
						   vk::PhysicalDevice physical_device,
						   vk::Device device,
						   vk::DescriptorPool pool,
						   vk::DescriptorSetLayout layout)
	{
		m_memory = allocate_memory(physical_device,
								   device,
								   sizeof(UniformDataType),
								   vk::BufferUsageFlagBits::eTransferSrc
								   | vk::BufferUsageFlagBits::eUniformBuffer,
								   // Host Visible and Coherent allows direct
								   // writes into the buffers without sync issues.
								   // but it can be slower overall
								   vk::MemoryPropertyFlagBits::eHostVisible
								   | vk::MemoryPropertyFlagBits::eHostCoherent);

			const auto allocate_info = vk::DescriptorSetAllocateInfo{}
				.setDescriptorPool(pool)
				.setDescriptorSetCount(1)
				.setSetLayouts(layout);

			auto sets = device.allocateDescriptorSetsUnique(allocate_info);
			if (sets.size() != 1) {
				logger.error(std::source_location::current(),
							"This system only allows handling 1 set per frame in flight"
							"\n if you want more sets find another way to store them..");
			}
			m_set = std::move(sets.at(0));

			const auto buffer_info = vk::DescriptorBufferInfo{}
				.setBuffer(m_memory.buffer.get())
				.setOffset(0)
				.setRange(sizeof(UniformDataType));

			const std::array<vk::WriteDescriptorSet, 1> writes{
				vk::WriteDescriptorSet{}
				.setDstBinding(0)
				.setDstArrayElement(0)
				.setDstSet(m_set.get())
				.setDescriptorCount(1)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setBufferInfo(buffer_info),
			};

			copy_to_allocated_memory(device,
									m_memory,
									reinterpret_cast<void*>(init),
									sizeof(UniformDataType));

			device.updateDescriptorSets(writes.size(),
										writes.data(),
										0,
										nullptr);

			logger.info(std::source_location::current(),
						"Created UniformBuffer");
	}
};
