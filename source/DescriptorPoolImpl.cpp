#include "DescriptorPoolImpl.hpp"

DescriptorPool::Impl::Impl(DescriptorPoolCreateInfo const& create_info,
						   PresentationContext::Impl* presentation_context)
{
	std::vector<vk::DescriptorPoolSize> sizes{};
	uint32_t max_sets = 0;

	if (create_info.uniform_buffer_count) {
		const auto count = create_info.uniform_buffer_count.value();
		const auto size = vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(count);

		sizes.push_back(size);
		max_sets += count;
	}
	
	if (create_info.combined_image_sampler_count) {
		const auto count = create_info.combined_image_sampler_count.value();
		const auto size = vk::DescriptorPoolSize{}
			.setType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(count);

		sizes.push_back(size);
		max_sets += count;
	}
	
	const auto pool_info = vk::DescriptorPoolCreateInfo{}
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(max_sets)
		.setPoolSizes(sizes);
	
	descriptor_pool =
		presentation_context->device.get().createDescriptorPoolUnique(pool_info, nullptr);
}

DescriptorPool::Impl::~Impl()
{
}


DescriptorPool::DescriptorPool(DescriptorPoolCreateInfo const& create_info,
							   PresentationContext& presentation_context)
	: impl(std::make_unique<Impl>(create_info, presentation_context.impl.get()))
{
}

DescriptorPool::~DescriptorPool()
{
}
