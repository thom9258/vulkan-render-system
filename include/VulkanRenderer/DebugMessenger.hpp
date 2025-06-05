#pragma once

#include <vulkan/vulkan.hpp>


VKAPI_ATTR VkBool32 VKAPI_CALL
debugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
							VkDebugUtilsMessageTypeFlagsEXT messageTypes,
							const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
							void* /*pUserData*/ )
{
	constexpr uint32_t validation_warning = 0x822806fa;
	constexpr uint32_t validation_performance_warning = 0xe8d1a9fe;
	switch (static_cast<uint32_t>(pCallbackData->messageIdNumber)) {
	case 0:
		// Validation Warning: Override layer has override paths set to C:/VulkanSDK/<version>/Bin
		return vk::False;
	case validation_warning:
		// Validation Warning: vkCreateInstance(): to enable extension VK_EXT_debug_utils,
		//but this extension is intended to support use by applications when
		// debugging and it is strongly recommended that it be otherwise avoided.
		return vk::False;
	case validation_performance_warning:
		// Validation Performance Warning: Using debug builds of the
		//validation layers *will* adversely affect performance.
		return vk::False;
	}

	std::cerr << vk::to_string(vk::DebugUtilsMessageSeverityFlagBitsEXT(messageSeverity)) 
			  << ": " << vk::to_string(vk::DebugUtilsMessageTypeFlagsEXT(messageTypes)) 
			  << ":\n";

	std::cerr << std::string( "\t" ) << "messageIDName   = <" 
			  << pCallbackData->pMessageIdName << ">\n";
	std::cerr << std::string( "\t" ) << "messageIdNumber = " 
			  << pCallbackData->messageIdNumber << "\n";
	std::cerr << std::string( "\t" ) << "message         = <" 
			  << pCallbackData->pMessage << ">\n";
	if ( 0 < pCallbackData->queueLabelCount ) {
		std::cerr << std::string( "\t" ) << "Queue Labels:\n";
		for ( uint32_t i = 0; i < pCallbackData->queueLabelCount; i++ ) {
			std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
		}
	}
	if ( 0 < pCallbackData->cmdBufLabelCount ) {
		std::cerr << std::string( "\t" ) << "CommandBuffer Labels:\n";
		for ( uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++ ) {
			std::cerr << std::string( "\t\t" ) << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
		}
	}
	if ( 0 < pCallbackData->objectCount ) {
		std::cerr << std::string( "\t" ) << "Objects:\n";
		for ( uint32_t i = 0; i < pCallbackData->objectCount; i++ ) {
			std::cerr << std::string( "\t\t" ) << "Object " << i << "\n";
			std::cerr << std::string( "\t\t\t" ) << "objectType   = " 
					  << vk::to_string( vk::ObjectType(pCallbackData->pObjects[i].objectType))
					  << "\n";
			std::cerr << std::string( "\t\t\t" ) << "objectHandle = " 
					  << pCallbackData->pObjects[i].objectHandle << "\n";
			if ( pCallbackData->pObjects[i].pObjectName ) {
				std::cerr << std::string( "\t\t\t" ) << "objectName   = <" 
						  << pCallbackData->pObjects[i].pObjectName << ">\n";
			}
		}
	}
	return vk::False;
}
