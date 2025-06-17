#pragma once

#include <VulkanRenderer/DescriptorPool.hpp>
#include "ContextImpl.hpp"

class DescriptorPool::Impl 
{
public:
    explicit Impl(DescriptorPoolCreateInfo const& create_info,
				  Render::Context::Impl* context);
    ~Impl();
	
	vk::UniqueDescriptorPool descriptor_pool;
};
