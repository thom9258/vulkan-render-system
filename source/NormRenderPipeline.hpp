#pragma once 

#include <iostream>
#include <filesystem>
#include "Utils.hpp"
#include "VertexBufferImpl.hpp"
#include "VertexImpl.hpp"
#include "Mesh.hpp"

#include <vector>

struct NormRenderPipeline
{
	vk::UniquePipelineLayout layout;
    vk::UniquePipeline pipeline;

	vk::UniqueDescriptorSetLayout descriptor_layout;
	vk::UniqueDescriptorPool descriptor_pool;

	struct PushConstants
	{
		glm::mat4 model;
	};

	/* The Camera descriptor loads persistent perspective
	 * data, and should happen as a single descriptor load
	 * every frame.
	 */
	struct Camera
	{
		glm::mat4 view;
		glm::mat4 proj;
	};
	std::vector<AllocatedMemory> descriptor_memories;
	std::vector<vk::UniqueDescriptorSet> descriptor_sets;
};

struct NormColorRenderInfo
{
	glm::mat4 view;
	glm::mat4 proj;
};

/**
 * @brief descripe the descriptor layout of the norm color shader.
 * this will set up a layout for NormRenderPipeline::Camera
 */
vk::UniqueDescriptorSetLayout
norm_render_pipeline_descriptorset_layout(vk::Device& device)
{
	const auto layout_binding = vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer);
	
	const auto set_info = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindingCount(1)
		.setBindings(layout_binding);
	
	return device.createDescriptorSetLayoutUnique(set_info, nullptr);
}

NormRenderPipeline
create_norm_render_pipeline(vk::PhysicalDevice& physical_device,
							vk::Device& device,
							vk::RenderPass& renderpass,
							const uint32_t frames_in_flight,
							const vk::Extent2D render_extent,
							const std::filesystem::path shader_root_path,
							bool debug_print) noexcept
{
	const std::string pipeline_name = "NormColor";
	const std::filesystem::path vertexshader_name = "NormColor.vert.spv";
	const std::filesystem::path fragmentshader_name = "NormColor.frag.spv";
	if (debug_print) {
		std::cout << "> " << pipeline_name << "\n"
				  << "  -> vertexshader   " << vertexshader_name << "\n"
				  << "  -> fragmentshader " << fragmentshader_name
				  << std::endl;
	}

	if (debug_print) {
		std::cout << "> norm render pipeline -> vertexshader   " << vertexshader_name << "\n"
				  << "                       -> fragmentshader " << fragmentshader_name
				  << std::endl;
	}

	NormRenderPipeline pipeline;

	const auto vert =
		read_binary_file((shader_root_path / vertexshader_name).string().c_str());
	if (!vert)
		throw std::runtime_error("COULD NOT LOAD VERTEX SHADER BINARY");
	
	auto vertexShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*vert);

	vk::UniqueShaderModule vertex_module =
		device.createShaderModuleUnique(vertexShaderModuleCreateInfo);
	
	const auto frag =
		read_binary_file((shader_root_path / fragmentshader_name).string().c_str());
	if (!frag)
		throw std::runtime_error("COULD NOT LOAD FRAGMENT SHADER BINARY");
	
	auto fragmentShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*frag);

	vk::UniqueShaderModule fragment_module =
		device.createShaderModuleUnique(fragmentShaderModuleCreateInfo);
	
	std::array<vk::PipelineShaderStageCreateInfo, 2> pipelineShaderStageCreateInfos{
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eVertex)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*vertex_module)
		.setPName("main"),
		
		vk::PipelineShaderStageCreateInfo{}
		.setStage(vk::ShaderStageFlagBits::eFragment)
		.setFlags(vk::PipelineShaderStageCreateFlags())
		.setModule(*fragment_module)
		.setPName("main"),
	};

	if (debug_print)
		std::cout << "> Created shaders" << std::endl;

    std::array<vk::DynamicState, 2> dynamicStates{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	auto pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
		.setFlags(vk::PipelineDynamicStateCreateFlags())
		.setDynamicStates(dynamicStates);
	
	const auto bindingDescriptions = binding_descriptions(VertexPosNormColor{});
	const auto attributeDescriptions = attribute_descriptions(VertexPosNormColor{});
	
	auto pipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{}
		.setFlags(vk::PipelineVertexInputStateCreateFlags())
		.setVertexBindingDescriptions(bindingDescriptions)
		.setVertexAttributeDescriptions(attributeDescriptions);

    auto pipelineInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{}
		.setFlags(vk::PipelineInputAssemblyStateCreateFlags())
		.setPrimitiveRestartEnable(vk::False)
		.setTopology(vk::PrimitiveTopology::eTriangleList);
	
	 const auto initial_viewport = vk::Viewport{}
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast<float>(render_extent.width))
		.setHeight(static_cast<float>(render_extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);
	
	auto initial_scissor = vk::Rect2D{}
		.setOffset(vk::Offset2D{}.setX(0.0f)
				                 .setY(0.0f));


    auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
		.setFlags(vk::PipelineViewportStateCreateFlags())
		.setViewports(initial_viewport)
		.setScissors(initial_scissor);
	
    auto pipelineRasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{}
		.setFlags(vk::PipelineRasterizationStateCreateFlags())
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setDepthBiasConstantFactor(0.0f)
		.setDepthBiasClamp(0.0f)
		.setDepthBiasSlopeFactor(0.0f)
		.setLineWidth(1.0f);

    vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo
		.setFlags(vk::PipelineMultisampleStateCreateFlags())
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    const vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR 
													| vk::ColorComponentFlagBits::eG 
													| vk::ColorComponentFlagBits::eB 
													| vk::ColorComponentFlagBits::eA);

	auto pipelineColorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{}
		.setBlendEnable(false)
		.setSrcColorBlendFactor(vk::BlendFactor::eOne)
		.setDstColorBlendFactor(vk::BlendFactor::eZero)
		.setColorBlendOp(vk::BlendOp::eAdd)
		.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
		.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		.setAlphaBlendOp(vk::BlendOp::eAdd)
		.setColorWriteMask(colorComponentFlags);
	
	auto pipelineColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo{}
		.setFlags(vk::PipelineColorBlendStateCreateFlags())
		.setLogicOpEnable(false)
		.setLogicOp(vk::LogicOp::eNoOp)
		.setAttachments(pipelineColorBlendAttachmentState)
		.setBlendConstants({ 1.0f, 1.0f, 1.0f, 1.0f });
	
	const auto push_constant_range = vk::PushConstantRange{}
		.setOffset(0)
		.setSize(sizeof(NormRenderPipeline::PushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	if (debug_print)
		std::cout << "> PushConstant size: " << sizeof(NormRenderPipeline::PushConstants) 
				  << std::endl;

	if (sizeof(NormRenderPipeline::PushConstants) > 128) {
		std::cout << "> WARNING: PushConstant size is larger than minimum supported!"
				  << std::endl;
	}
	

	pipeline.descriptor_layout = norm_render_pipeline_descriptorset_layout(device);
	
    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(pipeline.descriptor_layout.get())
		.setPushConstantRanges(push_constant_range);

	pipeline.layout = 
		device.createPipelineLayoutUnique(pipelineLayoutCreateInfo);
	
	if (debug_print)
		std::cout << "> Created Pipeline Layout" << std::endl;
	
	auto depth_stencil_state_info = vk::PipelineDepthStencilStateCreateInfo{}
		.setDepthTestEnable(true)
		.setDepthWriteEnable(true)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(false)
		.setMinDepthBounds(0.0f)
		.setMaxDepthBounds(1.0f)
		.setStencilTestEnable(false);
	
	auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo{}
		.setFlags(vk::PipelineCreateFlags())
		.setStages(pipelineShaderStageCreateInfos)
		.setPVertexInputState(&pipelineVertexInputStateCreateInfo)
		.setPInputAssemblyState(&pipelineInputAssemblyStateCreateInfo)
		.setPTessellationState(nullptr)
		.setPViewportState(&pipelineViewportStateCreateInfo)
		.setPRasterizationState(&pipelineRasterizationStateCreateInfo)
		.setPMultisampleState(&pipelineMultisampleStateCreateInfo)
		.setPDepthStencilState(&depth_stencil_state_info)
		.setPColorBlendState(&pipelineColorBlendStateCreateInfo)
		.setPDynamicState(&pipelineDynamicStateCreateInfo)
		.setLayout(pipeline.layout.get())
		.setRenderPass(renderpass);

	vk::ResultValue<vk::UniquePipeline> result =
		device.createGraphicsPipelineUnique(nullptr,
											graphicsPipelineCreateInfo);
	
    switch (result.result) {
	case vk::Result::eSuccess:
		break;
	case vk::Result::ePipelineCompileRequiredEXT:
		throw std::runtime_error("Creating pipeline error: PipelineCompileRequiredEXT");
	default: 
		throw std::runtime_error("Invalid Result state");
    }
	
	pipeline.pipeline = std::move(result.value);
	if (debug_print)
		std::cout << "> created Norm Graphics Pipeline!" << std::endl;
	
	std::array<vk::DescriptorPoolSize, 1> sizes {
		vk::DescriptorPoolSize{}
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(10),
	};
	
	const auto pool_info = vk::DescriptorPoolCreateInfo{}
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(10)
		.setPoolSizes(sizes);
	
	pipeline.descriptor_pool = device.createDescriptorPoolUnique(pool_info, nullptr);
	if (debug_print)
		std::cout << "> allocated descriptor pool!" << std::endl;
	
	for (uint32_t i = 0; i < frames_in_flight; i++) {
		pipeline.descriptor_memories
			.push_back(allocate_memory(physical_device,
									   device,
									   sizeof(NormRenderPipeline::Camera),
									   vk::BufferUsageFlagBits::eTransferSrc
									   | vk::BufferUsageFlagBits::eUniformBuffer,
									   // Host Visible and Coherent allows direct
									   // writes into the buffers without sync issues.
									   vk::MemoryPropertyFlagBits::eHostVisible
									   | vk::MemoryPropertyFlagBits::eHostCoherent));

		const auto allocate_info = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(pipeline.descriptor_pool.get())
			.setDescriptorSetCount(1)
			.setSetLayouts(pipeline.descriptor_layout.get());
		
		auto sets = device.allocateDescriptorSetsUnique(allocate_info);
		if (sets.size() != 1)
			std::cout << "> WARNING: this system only allows handling 1 set per frame in flight"
					  << "\n if you want more sets find another way to store them.."
					  << std::endl;
		
		pipeline.descriptor_sets.push_back(std::move(sets[0]));

		const auto buffer_info = vk::DescriptorBufferInfo{}
			.setBuffer(pipeline.descriptor_memories[i].buffer.get())
			.setOffset(0)
			.setRange(sizeof(NormRenderPipeline::Camera));
		
		const auto write_descriptor = vk::WriteDescriptorSet{}
			.setDstBinding(0)
			.setDstSet(pipeline.descriptor_sets[i].get())
			.setDstArrayElement(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			// here images can be set aswell
			.setBufferInfo(buffer_info);
		
		const uint32_t write_count = 1;
		const uint32_t copy_count = 0;
		device.updateDescriptorSets(write_count,
									&write_descriptor,
									copy_count,
									nullptr);
	}
	std::cout << std::format("> allocated {} descriptor sets for {} frames in flight",
							 pipeline.descriptor_sets.size(),
							 frames_in_flight) 
			  << std::endl;

	return pipeline;
}

void draw_normcolors(vk::Device& device,
					 NormRenderPipeline& pipeline,
					 vk::CommandBuffer& commandbuffer,
					 const uint32_t frame_in_flight,
					 const NormColorRenderInfo& info,
					 std::vector<NormColorRenderable> renderables)
{
	/* Norm Direction Drawing uses a setup where the model is provided
	 * in the push constant, and the view and proj is provided
	 * in a camera descriptor set.
	 */
	
	/* Bind the pipeline and the descriptor sets
	 */
	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
							   pipeline.pipeline.get());

	NormRenderPipeline::Camera camera{};	
	camera.view = info.view;
	camera.proj = info.proj;

	copy_to_allocated_memory(device,
							 pipeline.descriptor_memories[frame_in_flight],
							 reinterpret_cast<void*>(&camera),
							 sizeof(camera));

	const uint32_t first_set = 0;
	const uint32_t descriptor_set_count = 1;
	const uint32_t dynamic_offset_count = 0;
	const uint32_t* dynamic_offsets = nullptr;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									 pipeline.layout.get(),
									 first_set,
									 descriptor_set_count,
									 &(pipeline.descriptor_sets[frame_in_flight].get()),
									 dynamic_offset_count,
									 dynamic_offsets);
	

	for (auto renderable: renderables) {
		NormRenderPipeline::PushConstants push{};
		push.model = renderable.model;
		const uint32_t push_offset = 0;
		commandbuffer.pushConstants(pipeline.layout.get(),
									vk::ShaderStageFlagBits::eVertex,
									push_offset,
									sizeof(push),
									&push);
		
		
		const uint32_t firstBinding = 0;
		const uint32_t bindingCount = 1;
		std::array<vk::DeviceSize, bindingCount> offsets = {0};
		std::array<vk::Buffer, bindingCount> buffers {
			renderable.mesh->vertexbuffer.impl->buffer.get(),
		};
		commandbuffer.bindVertexBuffers(firstBinding,
										bindingCount,
										buffers.data(),
										offsets.data());
		
		
		const uint32_t instanceCount = 1;
		const uint32_t firstVertex = 0;
		const uint32_t firstInstance = 0;
		commandbuffer.draw(renderable.mesh->vertexbuffer.impl->length,
						   instanceCount,
						   firstVertex,
						   firstInstance);
		
	}
}
