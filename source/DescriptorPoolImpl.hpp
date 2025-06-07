#pragma once
#ifndef _DESCRIPTOR_POOL_IMPL_
#define _DESCRIPTOR_POOL_IMPL_

#include <VulkanRenderer/DescriptorPool.hpp>
#include "VulkanRendererImpl.hpp"

class DescriptorPool::Impl 
{
public:
    explicit Impl(DescriptorPoolCreateInfo const& create_info,
				  PresentationContext::Impl* presentation_context);
    ~Impl();
	
	vk::UniqueDescriptorPool descriptor_pool;
};

#endif //_DESCRIPTOR_POOL_IMPL_
