#pragma once

#include <VulkanRenderer/StrongType.hpp>

#include "ShaderTextureImpl.hpp"
#include "FlightFrames.hpp"

#include <vulkan/vulkan.hpp>

#include <map>
#include <utility>

using BindingIndex = StrongType<uint32_t, struct BindingIndexTag>;
using TotalDescriptorCount = StrongType<uint32_t, struct TotalDescriptorCountTag>;
using FragmentPath = StrongType<std::filesystem::path, struct FragmentPathTag>;
using VertexPath = StrongType<std::filesystem::path, struct VertexPathTag>;

using DescriptorSetIndex = StrongType<uint32_t, struct DescriptorSetIndexTag>;
using DescriptorSetBindingIndex = StrongType<uint32_t, struct DescriptorSetBindingIndexTag>;


auto create_texture_descriptorset(vk::Device device,
								  vk::DescriptorSetLayout descriptorset_layout,
								  vk::DescriptorPool descriptor_pool,
								  TextureSamplerReadOnly& texture)
	-> FlightFramesArray<vk::UniqueDescriptorSet>;

struct ShaderStageInfos
{
	struct {
		vk::UniqueShaderModule vertex;
		vk::UniqueShaderModule fragment;
	} modules;

	std::array<vk::PipelineShaderStageCreateInfo, 2> create_info;
};

auto create_shaderstage_infos(vk::Device device,
							  VertexPath const vertex_path,
							  FragmentPath const fragment_path)
	noexcept -> std::optional<ShaderStageInfos>;


template<typename TData>
struct UniformMemoryDirectWrite
{
	using Data = std::remove_cvref_t<TData>;
	size_t m_count;
	AllocatedMemory m_memory;
	
	UniformMemoryDirectWrite() = default;

	UniformMemoryDirectWrite(const UniformMemoryDirectWrite& rhs) = delete;
	UniformMemoryDirectWrite(UniformMemoryDirectWrite&& rhs)
	{
		std::swap(m_memory, rhs.m_memory);
		std::swap(m_count, rhs.m_count);
	}

	UniformMemoryDirectWrite& operator=(const UniformMemoryDirectWrite&) = delete;
	UniformMemoryDirectWrite& operator=(UniformMemoryDirectWrite&& rhs)
	{
		std::swap(m_memory, rhs.m_memory);
		std::swap(m_count, rhs.m_count);
		return *this;
	}

	explicit UniformMemoryDirectWrite(vk::PhysicalDevice physical_device,
									  vk::Device device,
									  size_t count)
	{
		m_count = (count < 1) ? 1 : count;
		m_memory = allocate_memory(physical_device,
								   device,
								   sizeof(Data) * m_count,
								   vk::BufferUsageFlagBits::eTransferSrc
								   | vk::BufferUsageFlagBits::eUniformBuffer,
								   // Host Visible and Coherent allows direct
								   // writes into the buffers without sync issues.
								   // but it can be slower overall
								   vk::MemoryPropertyFlagBits::eHostVisible
								   | vk::MemoryPropertyFlagBits::eHostCoherent);
	}
	
	void write(vk::Device device, Data* data, size_t length)
	{
		if (length == 0) return;
		if (length > m_count) length = m_count;
		copy_to_allocated_memory(device,
								 m_memory,
								 reinterpret_cast<void*>(data),
								 sizeof(Data) * length);
	}
	
	vk::DescriptorBufferInfo& buffer_info() const
	{
		static auto info = vk::DescriptorBufferInfo{}
			.setBuffer(m_memory.buffer.get())
			.setOffset(0)
			.setRange(sizeof(Data) * m_count);
		return info;
	}
	
};


template<typename Data>
struct UniformBuffer
{
	using UniformDataType = std::remove_cvref_t<Data>;

	AllocatedMemory m_memory;
	vk::UniqueDescriptorSet m_set;
	
	UniformBuffer() = default;

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
		
		copy_to_allocated_memory(device,
								 m_memory,
								 reinterpret_cast<void*>(init),
								 sizeof(UniformDataType));
		
		const std::array<vk::WriteDescriptorSet, 1> writes{
			vk::WriteDescriptorSet{}
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDstSet(m_set.get())
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setBufferInfo(buffer_info),
		};
		
		device.updateDescriptorSets(writes.size(),
									writes.data(),
									0,
									nullptr);
		
		logger.info(std::source_location::current(),
					"Created UniformBuffer");
		
	}
};

template <DescriptorSetIndex t_set_index,
		  typename TUniformBufferLayout>
struct Uniform {
	using BufferLayoutType = std::remove_cvref_t<TUniformBufferLayout>;
	using BufferType = UniformBuffer<BufferLayoutType>;
	DescriptorSetIndex static constexpr set_index = t_set_index;
	size_t static constexpr data_memsize = sizeof(BufferLayoutType);
	BufferLayoutType data;
	vk::UniqueDescriptorSetLayout set_layout;
	FlightFramesArray<BufferType> buffers;
};

template <DescriptorSetIndex t_set_index>
struct TextureDescriptor
{
	DescriptorSetIndex static constexpr set_index = t_set_index;
	TextureSamplerReadOnly default_texture;
	vk::UniqueDescriptorSetLayout layout;
	std::map<TextureSamplerReadOnly*, FlightFramesArray<vk::UniqueDescriptorSet>> sets;
};

struct TextureMaterial
{
	TextureSamplerReadOnly* ambient;
	TextureSamplerReadOnly* diffuse;
	TextureSamplerReadOnly* specular;
	TextureSamplerReadOnly* normal;
	
	bool operator==(TextureMaterial const& rhs) const
	{
		return ambient == rhs.ambient
			&& diffuse == rhs.diffuse
			&& specular == rhs.specular
			&& normal == rhs.normal;
	}
};

template <DescriptorSetIndex t_set_index>
struct TextureMaterialDescriptorSet
{
	using MaterialDescriptorSet = std::pair<
		TextureMaterial,
		FlightFramesArray<vk::UniqueDescriptorSet>>;

	DescriptorSetIndex static constexpr set_index = t_set_index;
	TextureMaterial default_material;
	vk::UniqueDescriptorSetLayout layout;
	std::vector<MaterialDescriptorSet> sets;
	
	auto get_set(TextureMaterial const& material, CurrentFlightFrame flightframe)
		-> std::optional<vk::DescriptorSet>
	{
		for (MaterialDescriptorSet const& set: sets) {
			if (set.first == material)
				return set.second[*flightframe].get();
		}
		
		return std::nullopt;
	}
};

