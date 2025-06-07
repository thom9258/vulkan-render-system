#pragma once

#include <vulkan/vulkan.hpp>

VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							VkDebugUtilsMessageTypeFlagsEXT messageTypes,
							const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
							void* /*pUserData*/ );
