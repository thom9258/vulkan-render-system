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
								  TextureSamplerReadOnly& texture)
	-> FlightFramesArray<vk::UniqueDescriptorSet>
{
	FlightFramesArray<vk::UniqueDescriptorSet> sets;

	for (uint32_t i = 0; i < sets.size(); i++) {
		const auto allocate_info = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(descriptor_pool)
			.setDescriptorSetCount(1)
			.setSetLayouts(descriptorset_layout);
		
		auto createdsets = device.allocateDescriptorSetsUnique(allocate_info);
		sets[i] = std::move(createdsets[0]);
		
		const auto image_info = vk::DescriptorImageInfo{}
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
			.setImageView(texture.impl->view.get())
			.setSampler(texture.impl->sampler.get());
		
		const std::array<vk::WriteDescriptorSet, 1> writes{
			vk::WriteDescriptorSet{}
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDstSet(sets[i].get())
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setImageInfo(image_info),
		};

		device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
	}

	return sets;
}
