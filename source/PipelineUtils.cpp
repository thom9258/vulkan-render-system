#include "PipelineUtils.hpp"
#include "ContextImpl.hpp"

auto create_shaderstage_infos(vk::Device device,
							  VertexPath const vertex_path,
							  FragmentPath const fragment_path)
	noexcept -> std::optional<ShaderStageInfos>
{
	auto vert = read_binary_file(vertex_path.get().string().c_str());
	if (!vert)
		return std::nullopt;
	auto frag = read_binary_file(fragment_path.get().string().c_str());
	if (!frag)
		return std::nullopt;
	
	auto vertexShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*vert);

	auto fragmentShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*frag);

	ShaderStageInfos shaderstage;

	shaderstage.modules.vertex =
		device.createShaderModuleUnique(vertexShaderModuleCreateInfo);
	
	shaderstage.modules.fragment =
		device.createShaderModuleUnique(fragmentShaderModuleCreateInfo);
	
	shaderstage.create_info = std::array<vk::PipelineShaderStageCreateInfo, 2>{
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*shaderstage.modules.vertex)
		.setPName("main"),
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*shaderstage.modules.fragment)
		.setPName("main")
	};
	
	return shaderstage;
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
										 BindingIndex binding_index,
										 TotalDescriptorCount descriptor_count)
	-> vk::UniqueDescriptorSetLayout
{
	std::array<vk::DescriptorSetLayoutBinding, 1> const binding{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(*binding_index)
		.setDescriptorCount(*descriptor_count)
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
	TotalFramesInFlight total_flight_frames,
    TextureSamplerReadOnly* default_texture)
	: m_default{default_texture}
	, m_layout{std::move(descriptorset_layout)}
{
	if (default_texture)
		m_sets.insert({m_default, create_texture_descriptorset(device,
															   m_layout.get(),
															   descriptor_pool,
															   *total_flight_frames,
															   *m_default)});
}


auto CachedTextureDescriptorBinder::get_default_descriptor(CurrentFrameInFlight current_flightframe)
	noexcept -> std::optional<vk::DescriptorSet>
{
	auto found = m_sets.find(m_default);
	if (found == m_sets.end()) return std::nullopt;
	
	//TODO: this is technically a hack, we just happen to know that if we get
	//      the default texture we are starting a new rendering iteration,
	//      thus we require a reset of the last bound texture.
	m_last_bound = m_default;
	return found->second.at(*current_flightframe).get();
}
	
void CachedTextureDescriptorBinder::bind_texture_descriptor(
    Logger& logger,
    vk::Device device,
	vk::PipelineLayout pipeline_layout,
	vk::DescriptorPool descriptor_pool,
	vk::CommandBuffer& commandbuffer,
	TotalFramesInFlight total_flightframes,
	CurrentFrameInFlight current_flightframe,
	TextureSamplerReadOnly* texture)
{
	if (!texture) 
		texture = m_default;

	if (!m_sets.contains(texture)) {
		m_sets.insert({texture, create_texture_descriptorset(device,
															 m_layout.get(),
															 descriptor_pool,
															 *total_flightframes,
															 *texture)});

		logger.info(std::source_location::current(),
					"Created descriptorset for new texture");
	}
	
	if (texture == m_last_bound)
		return;

	uint32_t const first_set = 1;
	std::array<vk::DescriptorSet, 1> descriptorset{
		m_sets[texture][*current_flightframe].get()
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

void CachedTextureDescriptorBinder::flush_cache(vk::Device device,
												vk::DescriptorPool descriptor_pool,
												TotalFramesInFlight total_flightframes,
												TextureSamplerReadOnly* texture)
{
	m_sets.clear();
	m_sets.insert({texture, create_texture_descriptorset(device,
													   m_layout.get(),
													   descriptor_pool,
													   *total_flightframes,
													   *texture)});
	m_default = texture;
}
