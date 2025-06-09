#pragma once

#include <iostream>

struct WireframePipeline
{
	vk::UniquePipelineLayout layout;
    vk::UniquePipeline pipeline;

	struct PushConstants
	{
		glm::vec4 color;
		glm::mat4 modelviewproj;
	};
};

struct WireframeRenderInfo
{
	glm::mat4 viewproj;
};

WireframePipeline
create_wireframe_render_pipeline(vk::Device& device,
								 vk::RenderPass& renderpass,
								 const vk::Extent2D render_extent,
								 const std::filesystem::path shader_root_path,
								 bool debug_print) noexcept
{
	const std::string pipeline_name = "Wireframe";
	const std::filesystem::path vertexshader_name = "Wireframe.vert.spv";
	const std::filesystem::path fragmentshader_name = "Wireframe.frag.spv";

	if (debug_print) {
		std::cout << "> " << pipeline_name << "\n"
				  << "  -> vertexshader   " << vertexshader_name << "\n"
				  << "  -> fragmentshader " << fragmentshader_name
				  << std::endl;
	}

	WireframePipeline pipeline;

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
		.setPolygonMode(vk::PolygonMode::eLine)
		.setCullMode(vk::CullModeFlagBits::eNone)
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
		.setSize(sizeof(WireframePipeline::PushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	if (debug_print)
		std::cout << "> PushConstant size: " << sizeof(WireframePipeline::PushConstants) 
				  << std::endl;

	if (sizeof(WireframePipeline::PushConstants) > 128) {
		std::cout << "> WARNING: PushConstant size is larger than minimum supported!\n"
				  << "  -> This can cause compatability issues..."
				  << std::endl;
	}
	
    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(nullptr)
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
		std::cout << "> created wireframe Graphics Pipeline!" << std::endl;
	
	return pipeline;
}

void draw_wireframes(WireframePipeline& pipeline,
					 vk::CommandBuffer& commandbuffer,
					 const WireframeRenderInfo& info,
					 std::vector<WireframeRenderable> renderables)
{
	/* Wireframe Drawing uses a setup where the proj*view*model
	 * is multiplied beforehand and a single color value is provided
	 * directly in the push constant.
	 */
	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
							   pipeline.pipeline.get());

	for (auto renderable: renderables) {
		WireframePipeline::PushConstants push{};
		push.color = renderable.basecolor;	
		push.modelviewproj = info.viewproj * renderable.model;
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
			renderable.mesh->vertexbuffer.buffer.get(),
		};
		commandbuffer.bindVertexBuffers(firstBinding,
										bindingCount,
										buffers.data(),
										offsets.data());

		const uint32_t instanceCount = 1;
		const uint32_t firstVertex = 0;
		const uint32_t firstInstance = 0;
		commandbuffer.draw(renderable.mesh->vertexbuffer.length,
						   instanceCount,
						   firstVertex,
						   firstInstance);
	}
}
