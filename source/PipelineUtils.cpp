#include "PipelineUtils.hpp"
#include "ContextImpl.hpp"

auto create_shaderstage_infos(vk::Device device,
							  std::filesystem::path const vertex_path,
							  std::filesystem::path const fragment_path)
	noexcept -> std::optional<std::array<vk::PipelineShaderStageCreateInfo, 2>>
{
	auto vert = read_binary_file(vertex_path.string().c_str());
	if (!vert)
		return std::nullopt;

	auto vertexShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*vert);

	vk::UniqueShaderModule vertex_module =
		device.createShaderModuleUnique(vertexShaderModuleCreateInfo);
	
	auto frag = read_binary_file(fragment_path.string().c_str());
	if (!frag)
		return std::nullopt;
	
	auto fragmentShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*frag);

	vk::UniqueShaderModule fragment_module =
		device.createShaderModuleUnique(fragmentShaderModuleCreateInfo);
	
	return std::array<vk::PipelineShaderStageCreateInfo, 2>{
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*vertex_module)
		.setPName("main"),
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*fragment_module)
		.setPName("main")
	};
}

auto create_texture_descriptorset(vk::Device device,
								  vk::DescriptorSetLayout descriptorset_layout,
								  vk::DescriptorPool descriptor_pool,
								  size_t frames_in_flight,
								  TextureSamplerReadOnly& texture)
	-> std::vector<vk::UniqueDescriptorSet>
{
	std::vector<vk::UniqueDescriptorSet> sets;

	for (uint32_t i = 0; i < frames_in_flight; i++) {
		const auto allocate_info = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(descriptor_pool)
			.setDescriptorSetCount(1)
			.setSetLayouts(descriptorset_layout);
		
		auto createdsets = device.allocateDescriptorSetsUnique(allocate_info);
		sets.push_back(std::move(createdsets[0]));
		
		const auto image_info = vk::DescriptorImageInfo{}
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(texture.impl->view.get())
			.setSampler(texture.impl->sampler.get());
		
		const std::array<vk::WriteDescriptorSet, 1> writes{
			vk::WriteDescriptorSet{}
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDstSet(sets.back().get())
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setImageInfo(image_info),
		};

		device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
	}

	return sets;
}

auto create_texture_descriptorset_layout(vk::Device device,
										 uint32_t binding_index)
	-> vk::UniqueDescriptorSetLayout
{
	std::array<vk::DescriptorSetLayoutBinding, 1> const binding{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(binding_index)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
	};

	auto const setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(binding);
	
	return device.createDescriptorSetLayoutUnique(setinfo, nullptr);
}

CachedTextureDescriptorBinder::CachedTextureDescriptorBinder(
    vk::Device device,
    vk::UniqueDescriptorSetLayout&& descriptorset_layout,
    vk::DescriptorPool descriptor_pool,
    size_t frames_in_flight,
    TextureSamplerReadOnly&& default_texture)
	: m_default{std::move(default_texture)}
	, m_layout{std::move(descriptorset_layout)}
{
	m_sets.insert({&m_default, create_texture_descriptorset(device,
															m_layout.get(),
															descriptor_pool,
															frames_in_flight,
															m_default)});
}
	
void CachedTextureDescriptorBinder::bind_texture_descriptor(vk::Device device,
							 vk::PipelineLayout pipeline_layout,
							 uint32_t binding_index,
							 vk::DescriptorPool descriptor_pool,
							 size_t frames_in_flight,
							 vk::CommandBuffer& commandbuffer,
							 size_t frame_in_flight,
							 TextureSamplerReadOnly* texture)
{
	if (!texture) 
		texture = &m_default;

	if (texture == m_last_bound)
		return;

	if (!m_sets.contains(texture)) {
		m_sets.insert({texture, create_texture_descriptorset(device,
															 m_layout.get(),
															 descriptor_pool,
															 frames_in_flight,
															 *texture)});
	}

	uint32_t const first_set = 1;
	std::array<vk::DescriptorSet, 1> descriptorset{
		m_sets[texture][frame_in_flight].get()
	};
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									 pipeline_layout,
									 first_set,
									 descriptorset.size(),
									 descriptorset.data(),
									 0,
									 nullptr);
	m_last_bound = texture;
}


#if 0
auto CachedTextureDescriptorBinder::layout()
	const noexcept -> vk::DescriptorSetLayout
{
	return m_layout.get();
}

void CachedTextureDescriptorBinder::flush_cache(vk::Device device,
												vk::DescriptorPool descriptor_pool,
												size_t frames_in_flight);
{
	m_sets.clear();
	m_sets.insert({&base, create_texture_descriptorset(device,
													   m_layout.get(),
													   descriptor_pool,
													   frames_in_flight,
													   m_base)});
}
#endif
