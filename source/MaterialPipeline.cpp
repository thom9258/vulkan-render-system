#include "MaterialPipeline.hpp"
#include "VertexBufferImpl.hpp"

#include <format>

MaterialPipeline::MaterialPipeline(Logger& logger,
								   Render::Context::Impl* context,
								   Presenter::Impl* presenter,
								   DescriptorPool::Impl* descriptor_pool,
								   vk::RenderPass& renderpass,
								   std::filesystem::path const shader_root_path)
{
	std::string const pipeline_name = "MaterialPipeline";
	std::string const vertexshader_name = "Material.vert.spv";
	std::string const fragmentshader_name = "Material.frag.spv";
	logger.info(std::source_location::current(),
				std::format("Creating Pipeline {}",
							pipeline_name));
	logger.info(std::source_location::current(),
				std::format("  Vertex Shader {}",
							vertexshader_name));
	logger.info(std::source_location::current(),
				std::format("  Fragment Shader {}",
							fragmentshader_name));
	
	auto const frames_in_flight = TotalFramesInFlight{presenter->max_frames_in_flight};
	vk::Extent2D const render_extent = context->get_window_extent();
	
	const auto vertex_path = VertexPath{shader_root_path / vertexshader_name};
	const auto fragment_path = FragmentPath{shader_root_path / fragmentshader_name};
	auto shaderstage_infos = create_shaderstage_infos(context->device.get(),
													  vertex_path,
													  fragment_path);

	if (!shaderstage_infos) {
		std::string const msg = std::format("{} could not load vertex/fragment sources {} / {}",
											pipeline_name,
											vertexshader_name,
											fragmentshader_name);
		logger.fatal(std::source_location::current(), msg);
		throw std::runtime_error(msg);
	}

    std::array<vk::DynamicState, 2> const dynamicStates{
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	auto pipelineDynamicStateCreateInfo = vk::PipelineDynamicStateCreateInfo{}
		.setFlags(vk::PipelineDynamicStateCreateFlags())
		.setDynamicStates(dynamicStates);
	
	const auto bindingDescriptions = binding_descriptions(VertexPosNormColorUV{});
	const auto attributeDescriptions = attribute_descriptions(VertexPosNormColorUV{});
	
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

    auto pipelineMultisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo{}
		.setFlags(vk::PipelineMultisampleStateCreateFlags())
		.setSampleShadingEnable(false)
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::ColorComponentFlags const colorComponentFlags(vk::ColorComponentFlagBits::eR 
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
		.setSize(sizeof(PushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	if (sizeof(PushConstants) > 128) {
		logger.warn(std::source_location::current(), 
					std::format("PushConstant size={} is larger than minimum supported (128)"
								"This can cause compatability issues on some devices",
								sizeof(PushConstants)));
	}

	std::array<vk::DescriptorSetLayoutBinding, 1> globalbindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	};

	const auto globalbindings_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(globalbindings);
	

	logger.info(std::source_location::current(), "Created globalbindings setinfo");
	
	m_globalbinding.layout =
		context->device.get().createDescriptorSetLayoutUnique(globalbindings_setinfo,
															  nullptr);
	
	logger.info(std::source_location::current(), "Created globalbindings layout");
	
	GlobalBinding global_binding_init{};	
	global_binding_init.view = glm::mat4(1.0f);
	global_binding_init.proj = glm::mat4(1.0f);

	/*Allocate Camera Descriptor Sets*/
	for (uint32_t i = 0; i < *frames_in_flight; i++) {
		m_globalbinding.buffers
			.push_back(UniformBuffer<GlobalBinding>(&global_binding_init,
													logger,
													context->physical_device,
													context->device.get(),
													descriptor_pool->descriptor_pool.get(),
													m_globalbinding.layout.get()));
	}

	logger.info(std::source_location::current(),
				"created global descriptor sets");

	auto constexpr diffuse_binding_index = BindingIndex{0}; 
	auto constexpr total_descriptor_count = TotalDescriptorCount{1}; 
	
	vk::UniqueDescriptorSetLayout diffuse_layout = 
		create_texture_descriptorset_layout(context->device.get(),
											diffuse_binding_index,
											total_descriptor_count);

	logger.info(std::source_location::current(), "Created diffuse layout");

	std::array<vk::DescriptorSetLayout, 2> const descriptorset_layouts{
		m_globalbinding.layout.get(),
		diffuse_layout.get(),
	};
	
    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(descriptorset_layouts)
		.setPushConstantRanges(push_constant_range);

	m_layout = 
		context->device.get().createPipelineLayoutUnique(pipelineLayoutCreateInfo);

	logger.info(std::source_location::current(), "Created Pipeline Layout");
	

	Pixel8bitRGBA const diffuse_blue{0, 0, 170, 255};
	Pixel8bitRGBA const diffuse_red{170, 0, 0, 255};
	m_default_textures.diffuse = 
		create_canvas(diffuse_blue, CanvasExtent{64, 64})
		| canvas_draw_checkerboard(diffuse_red, CheckerSquareSize{4})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);

	logger.info(std::source_location::current(), "Created diffuse default texture");
	m_descriptor_binders.diffuse =
		CachedTextureDescriptorBinder(context->device.get(),
									  std::move(diffuse_layout),
									  descriptor_pool->descriptor_pool.get(),
									  frames_in_flight,
									  &m_default_textures.diffuse);

	logger.info(std::source_location::current(), "Created diffuse descriptor cache");
	
#if 0
	Pixel8bitRGBA const normal_default(128, 128, 255, 255);
	TextureSamplerReadOnly normal = 
		create_canvas(normal_default, CanvasExtent{64, 64})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);
	m_descriptor_binders.normal =
		CachedTextureDescriptorBinder(context->device.get(),
									  m_layout.get(),
									  2,
									  descriptor_pool->descriptor_pool.get(),
									  frames_in_flight,
									  std::move(normal));


	
	Pixel8bitRGBA const specular_default(128, 128, 128, 255);
	TextureSamplerReadOnly specular = 
		create_canvas(specular_default, CanvasExtent{64, 64})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);
	m_descriptor_binders.specular =
		CachedTextureDescriptorBinder(context->device.get(),
									  m_layout.get(),
									  3,
									  descriptor_pool->descriptor_pool.get(),
									  frames_in_flight,
									  std::move(specular));
#endif


	logger.info(std::source_location::current(),
				"Created caching texture descriptor binders");
	
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
		.setStages(shaderstage_infos.value().create_info)
		.setPVertexInputState(&pipelineVertexInputStateCreateInfo)
		.setPInputAssemblyState(&pipelineInputAssemblyStateCreateInfo)
		.setPTessellationState(nullptr)
		.setPViewportState(&pipelineViewportStateCreateInfo)
		.setPRasterizationState(&pipelineRasterizationStateCreateInfo)
		.setPMultisampleState(&pipelineMultisampleStateCreateInfo)
		.setPDepthStencilState(&depth_stencil_state_info)
		.setPColorBlendState(&pipelineColorBlendStateCreateInfo)
		.setPDynamicState(&pipelineDynamicStateCreateInfo)
		.setLayout(m_layout.get())
		.setRenderPass(renderpass);

	vk::ResultValue<vk::UniquePipeline> result =
		context->device.get().createGraphicsPipelineUnique(nullptr,
														   graphicsPipelineCreateInfo);
	
    switch (result.result) {
	case vk::Result::eSuccess:
		break;
	case vk::Result::ePipelineCompileRequiredEXT:
		logger.error(std::source_location::current(),
					 "Creating pipeline error: PipelineCompileRequiredEXT");
	default: 
		logger.error(std::source_location::current(),
					 "Creating pipeline error: Unknown invalid Result state");
    }
	
	m_pipeline = std::move(result.value);

	logger.info(std::source_location::current(), "Created Pipeline");
}


MaterialPipeline::MaterialPipeline(MaterialPipeline&& rhs) noexcept
{
	std::swap(m_layout, rhs.m_layout);
	std::swap(m_pipeline, rhs.m_pipeline);
	std::swap(m_globalbinding, rhs.m_globalbinding);
	std::swap(m_descriptor_binders, rhs.m_descriptor_binders);
}

MaterialPipeline& MaterialPipeline::operator=(MaterialPipeline&& rhs) noexcept
{
	std::swap(m_layout, rhs.m_layout);
	std::swap(m_pipeline, rhs.m_pipeline);
	std::swap(m_globalbinding, rhs.m_globalbinding);
	std::swap(m_default_textures, rhs.m_default_textures);
	std::swap(m_descriptor_binders, rhs.m_descriptor_binders);
	return *this;
}

MaterialPipeline::~MaterialPipeline()
{

}


void MaterialPipeline::render(MaterialPipeline::GlobalBinding* global_binding,
							  Logger& logger,
							  vk::Device& device,
							  vk::DescriptorPool descriptor_pool,
							  vk::CommandBuffer& commandbuffer,
							  CurrentFrameInFlight const current_flightframe,
							  TotalFramesInFlight const max_frames_in_flight,
							  std::vector<MaterialRenderable>& renderables)
{
	copy_to_allocated_memory(device,
							 m_globalbinding.buffers[*current_flightframe].m_memory,
							 reinterpret_cast<void*>(global_binding),
							 sizeof(GlobalBinding));
	
	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
							   m_pipeline.get());
	
	auto diffuse_init = 
		m_descriptor_binders.diffuse.get_default_descriptor(current_flightframe);
	if (!diffuse_init) {
		logger.error(std::source_location::current(),
					 "Could not find initializer texture for diffuse");
		return;
	}
		
	
	/*Bind GlobalBinding Get Once before rendering*/
	std::array<vk::DescriptorSet, 2> init_sets{
		m_globalbinding.buffers[*current_flightframe].m_set.get(),
		*diffuse_init,
	};

	const uint32_t first_set = 0;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									 m_layout.get(),
									 first_set,
									 init_sets.size(),
									 init_sets.data(),
									 0,
									 nullptr);

	int i = 0;
	for (MaterialRenderable& renderable: renderables) {

		m_descriptor_binders.diffuse
			.bind_texture_descriptor(logger,
									 device,
									 m_layout.get(),
									 descriptor_pool,
									 commandbuffer,
									 max_frames_in_flight,
									 current_flightframe,
									 renderable.texture.diffuse);
		
		PushConstants push{};
		push.model = renderable.model;
		push.basecolor = renderable.basecolor;
		const uint32_t push_offset = 0;
		commandbuffer.pushConstants(m_layout.get(),
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
