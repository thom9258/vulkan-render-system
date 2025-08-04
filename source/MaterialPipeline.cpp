#include "MaterialPipeline.hpp"
#include "VertexBufferImpl.hpp"

#include <format>

void sort_light(Logger* logger,
				SortedLights* sorted,
				Light light)
{
	if (auto p = std::get_if<DirectionalLight>(&light))
		sorted->directionals.push_back(*p);
	else if (auto p = std::get_if<PointLight>(&light))
		sorted->points.push_back(*p);
	else if (auto p = std::get_if<SpotLight>(&light))
		sorted->spots.push_back(*p);
	else {
		logger->warn(std::source_location::current(),
					 "Found unknown Light that can not be sorted and used for drawing");
	}
}

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
	
	auto const frames_in_flight = MaxFlightFrames{presenter->max_frames_in_flight};
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

    vk::ColorComponentFlags constexpr colorComponentFlags(vk::ColorComponentFlagBits::eR 
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
	
	std::array<vk::DescriptorSetLayoutBinding, 1> frame_uniform_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
	};

	//std::array<vk::DescriptorSetLayoutBinding, 4> frame_uniform_bindings{
	//vk::DescriptorSetLayoutBinding{}
	//.setStageFlags(vk::ShaderStageFlagBits::eVertex)
	//.setBinding(0)
	//.setDescriptorCount(1)
	//.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	//vk::DescriptorSetLayoutBinding{}
	//.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	//.setBinding(1)
	//.setDescriptorCount(1)
	//.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	//vk::DescriptorSetLayoutBinding{}
	//.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	//.setBinding(2)
	//.setDescriptorCount(1)
	//.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	//vk::DescriptorSetLayoutBinding{}
	//.setStageFlags(vk::ShaderStageFlagBits::eFragment)
	//.setBinding(3)
	//.setDescriptorCount(1)
	//.setDescriptorType(vk::DescriptorType::eUniformBuffer),
//};

	const auto frame_uniform_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(frame_uniform_bindings);
	
	m_frame_uniform.set_layout =
		context->device.get().createDescriptorSetLayoutUnique(frame_uniform_setinfo,
															  nullptr);
	
	logger.info(std::source_location::current(), "Created frame uniform layout");
	
	// create pointlight uniform
	std::array<vk::DescriptorSetLayoutBinding, 1> pointlight_uniform_layout_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	};

	const auto pointlight_uniform_bindings_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(pointlight_uniform_layout_bindings);
	
	m_pointlight_uniform.set_layout =
		context->device.get().createDescriptorSetLayoutUnique(pointlight_uniform_bindings_setinfo,
															  nullptr);
	
	logger.info(std::source_location::current(), "Created point light uniform layout");
	
	// create directionallight uniform
	std::array<vk::DescriptorSetLayoutBinding, 1> directionallight_uniform_layout_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	};

	const auto directionallight_uniform_bindings_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(directionallight_uniform_layout_bindings);
	
	m_directionallight_uniform.set_layout =
		context->device.get().createDescriptorSetLayoutUnique(directionallight_uniform_bindings_setinfo,
															  nullptr);
	
	logger.info(std::source_location::current(), "Created directional light uniform layout");
	
	
	// create spotlight uniform
	std::array<vk::DescriptorSetLayoutBinding, 1> spotlight_uniform_layout_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	};

	const auto spotlight_uniform_bindings_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(spotlight_uniform_layout_bindings);
	
	m_spotlight_uniform.set_layout =
		context->device.get().createDescriptorSetLayoutUnique(spotlight_uniform_bindings_setinfo,
															  nullptr);
	
	logger.info(std::source_location::current(), "Created spotlight light uniform layout");
	
	std::array<vk::DescriptorSetLayoutBinding, 1> ambient_texture_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
	};
	auto ambient_texture_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(ambient_texture_bindings);
	
	m_ambient.layout =
		context->device.get().createDescriptorSetLayoutUnique(ambient_texture_setinfo,
															  nullptr);

	std::array<vk::DescriptorSetLayoutBinding, 1> diffuse_texture_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
	};
	auto diffuse_texture_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(diffuse_texture_bindings);
	
	m_diffuse.layout =
		context->device.get().createDescriptorSetLayoutUnique(diffuse_texture_setinfo,
															  nullptr);
	
	std::array<vk::DescriptorSetLayoutBinding, 1> normal_texture_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
	};
	auto normal_texture_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(normal_texture_bindings);
	
	m_normal.layout =
		context->device.get().createDescriptorSetLayoutUnique(normal_texture_setinfo,
															  nullptr);
	
	std::array<vk::DescriptorSetLayoutBinding, 1> specular_texture_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
	};
	auto specular_texture_setinfo = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(specular_texture_bindings);
	
	m_specular.layout =
		context->device.get().createDescriptorSetLayoutUnique(specular_texture_setinfo,
															  nullptr);
	
	logger.info(std::source_location::current(), "Created texture descriptorset layouts");

	//NOTE: thsese MUST match the indices of each individual set
	std::array<vk::DescriptorSetLayout, 8> const descriptorset_layouts{
		m_frame_uniform.set_layout.get(),
		m_pointlight_uniform.set_layout.get(),
		m_spotlight_uniform.set_layout.get(),
		m_directionallight_uniform.set_layout.get(),
		m_ambient.layout.get(),
		m_diffuse.layout.get(),
		m_specular.layout.get(),
		m_normal.layout.get(),
	};

    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(descriptorset_layouts)
		.setPushConstantRanges(push_constant_range);

	m_layout = 
		context->device.get().createPipelineLayoutUnique(pipelineLayoutCreateInfo);

	logger.info(std::source_location::current(), "Created Pipeline Layout");
	

	Pixel8bitRGBA::PixelType constexpr ambient_color = 51;
	Pixel8bitRGBA constexpr ambient_basic{ambient_color, ambient_color, ambient_color, 255};
	Pixel8bitRGBA constexpr diffuse_blue{0, 0, 170, 255};
	Pixel8bitRGBA constexpr diffuse_red{170, 0, 0, 255};
	Pixel8bitRGBA constexpr normal_default(128, 128, 255, 255);
	Pixel8bitRGBA constexpr specular_default(128, 128, 128, 255);
	
	m_ambient.default_texture = 
		create_canvas(ambient_basic, CanvasExtent{64, 64})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);

	m_diffuse.default_texture = 
		create_canvas(diffuse_blue, CanvasExtent{64, 64})
		| canvas_draw_checkerboard(diffuse_red, CheckerSquareSize{4})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);
	
	m_specular.default_texture = 
		create_canvas(specular_default, CanvasExtent{64, 64})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);
	
	m_normal.default_texture = 
		create_canvas(normal_default, CanvasExtent{64, 64})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);
	
	logger.info(std::source_location::current(), "Created default textures");

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

	/*Allocate Per-Frame Uniform Descriptor Sets*/
	m_frame_uniform.data.view = glm::mat4(1.0f);
	m_frame_uniform.data.proj = glm::mat4(1.0f);
	m_frame_uniform.data.position = glm::vec3(1.0f);

	for (uint32_t i = 0; i < m_frame_uniform.buffers.size(); i++) {
		m_frame_uniform.buffers[i] =
			decltype(m_frame_uniform)::BufferType(&m_frame_uniform.data,
												  logger,
												  context->physical_device,
												  context->device.get(),
												  descriptor_pool->descriptor_pool.get(),
												  m_frame_uniform.set_layout.get());
	}

	logger.info(std::source_location::current(),
				"created frame uniform descriptor sets");
	
	for (uint32_t i = 0; i < m_pointlight_uniform.buffers.size(); i++) {
		m_pointlight_uniform.buffers[i] =
			decltype(m_pointlight_uniform)::BufferType(&m_pointlight_uniform.data,
													   logger,
													   context->physical_device,
													   context->device.get(),
													   descriptor_pool->descriptor_pool.get(),
													   m_pointlight_uniform.set_layout.get());
	}

	logger.info(std::source_location::current(),
				"created pointlight uniform descriptor sets");

	for (uint32_t i = 0; i < m_directionallight_uniform.buffers.size(); i++) {
		m_directionallight_uniform.buffers[i] =
			decltype(m_directionallight_uniform)::BufferType(&m_directionallight_uniform.data,
															 logger,
															 context->physical_device,
															 context->device.get(),
															 descriptor_pool->descriptor_pool.get(),
															 m_directionallight_uniform.set_layout.get());
	}

	logger.info(std::source_location::current(),
				"created directionallight uniform descriptor sets");
	
	for (uint32_t i = 0; i < m_spotlight_uniform.buffers.size(); i++) {
		m_spotlight_uniform.buffers[i] =
			decltype(m_spotlight_uniform)::BufferType(&m_spotlight_uniform.data,
													  logger,
													  context->physical_device,
													  context->device.get(),
													  descriptor_pool->descriptor_pool.get(),
													  m_spotlight_uniform.set_layout.get());
	}

	logger.info(std::source_location::current(),
				"created spotlight uniform descriptor sets");
}


MaterialPipeline::MaterialPipeline(MaterialPipeline&& rhs) noexcept
{
	std::swap(m_layout, rhs.m_layout);
	std::swap(m_pipeline, rhs.m_pipeline);
	std::swap(m_frame_uniform, rhs.m_frame_uniform);
	std::swap(m_ambient, rhs.m_ambient);
	std::swap(m_diffuse, rhs.m_diffuse);
	std::swap(m_specular, rhs.m_specular);
	std::swap(m_normal, rhs.m_normal);
	std::swap(m_pointlight_uniform, rhs.m_pointlight_uniform);
	std::swap(m_directionallight_uniform, rhs.m_directionallight_uniform);
	std::swap(m_spotlight_uniform, rhs.m_spotlight_uniform);
}

MaterialPipeline& MaterialPipeline::operator=(MaterialPipeline&& rhs) noexcept
{
	std::swap(m_layout, rhs.m_layout);
	std::swap(m_pipeline, rhs.m_pipeline);
	std::swap(m_frame_uniform, rhs.m_frame_uniform);
	std::swap(m_ambient, rhs.m_ambient);
	std::swap(m_diffuse, rhs.m_diffuse);
	std::swap(m_specular, rhs.m_specular);
	std::swap(m_normal, rhs.m_normal);
	std::swap(m_pointlight_uniform, rhs.m_pointlight_uniform);
	std::swap(m_directionallight_uniform, rhs.m_directionallight_uniform);
	std::swap(m_spotlight_uniform, rhs.m_spotlight_uniform);
	return *this;
}

MaterialPipeline::~MaterialPipeline()
{
}

void MaterialPipeline::render(MaterialPipeline::FrameInfo& frame_info,
							  Logger& logger,
							  vk::Device& device,
							  vk::DescriptorPool descriptor_pool,
							  vk::CommandBuffer& commandbuffer,
							  CurrentFlightFrame const current_flightframe,
							  MaxFlightFrames const max_frames_in_flight,
							  std::vector<MaterialRenderable>& renderables,
							  std::vector<Light>& lights)
{
	if (!m_ambient.sets.contains(&m_ambient.default_texture)) {
		m_ambient.sets.insert({&m_ambient.default_texture,
							  create_texture_descriptorset(device,
														   m_ambient.layout.get(),
														   descriptor_pool,
														   m_ambient.default_texture)});
		logger.info(std::source_location::current(), "Created ambient default");
	}
	
	if (!m_diffuse.sets.contains(&m_diffuse.default_texture)) {
		m_diffuse.sets.insert({&m_diffuse.default_texture,
							  create_texture_descriptorset(device,
														   m_diffuse.layout.get(),
														   descriptor_pool,
														   m_diffuse.default_texture)});
		logger.info(std::source_location::current(), "Created diffuse default");
	}
			
	if (!m_specular.sets.contains(&m_specular.default_texture)) {
		m_specular.sets.insert({&m_specular.default_texture,
							  create_texture_descriptorset(device,
														   m_specular.layout.get(),
														   descriptor_pool,
														   m_specular.default_texture)});
		logger.info(std::source_location::current(), "Created specular default");
	}
	
	if (!m_normal.sets.contains(&m_normal.default_texture)) {
		m_normal.sets.insert({&m_normal.default_texture,
							  create_texture_descriptorset(device,
														   m_normal.layout.get(),
														   descriptor_pool,
														   m_normal.default_texture)});
		logger.info(std::source_location::current(), "Created normal default");
	}
	
	m_frame_uniform.data.view = frame_info.view;
	m_frame_uniform.data.proj = frame_info.proj;
	m_frame_uniform.data.position = frame_info.camera_position;
	copy_to_allocated_memory(device,
							 m_frame_uniform.buffers[*current_flightframe].m_memory,
							 reinterpret_cast<void*>(&m_frame_uniform.data),
							 m_frame_uniform.data_memsize);
	
	SortedLights sorted_lights;
	std::ranges::for_each(lights, std::bind_front(sort_light, &logger, &sorted_lights));
	
	if (!sorted_lights.points.empty()) {
		PointLight& p = sorted_lights.points.front();
		m_pointlight_uniform.data = p;
	}
	
	copy_to_allocated_memory(device,
							 m_pointlight_uniform.buffers[*current_flightframe].m_memory,
							 reinterpret_cast<void*>(&m_pointlight_uniform.data),
							 m_pointlight_uniform.data_memsize);
	
	if (!sorted_lights.directionals.empty()) {
		DirectionalLight& p = sorted_lights.directionals.front();
		m_directionallight_uniform.data = p;
	}
	
	copy_to_allocated_memory(device,
							 m_directionallight_uniform.buffers[*current_flightframe].m_memory,
							 reinterpret_cast<void*>(&m_directionallight_uniform.data),
							 m_directionallight_uniform.data_memsize);
	
	if (sorted_lights.spots.empty())
		logger.warn(std::source_location::current(),
					"no spotlights are provided");

	if (!sorted_lights.spots.empty()) {
		SpotLight& p = sorted_lights.spots.front();
		m_spotlight_uniform.data = p;
	}
	
	copy_to_allocated_memory(device,
							 m_spotlight_uniform.buffers[*current_flightframe].m_memory,
							 reinterpret_cast<void*>(&m_spotlight_uniform.data),
							 m_spotlight_uniform.data_memsize);
	
	
	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
							   m_pipeline.get());
	
	//NOTE: thsese MUST match the indices of each individual set
	std::array<vk::DescriptorSet, 8> init_sets{
		m_frame_uniform.buffers[*current_flightframe].m_set.get(),
		m_pointlight_uniform.buffers[*current_flightframe].m_set.get(),
		m_spotlight_uniform.buffers[*current_flightframe].m_set.get(),
		m_directionallight_uniform.buffers[*current_flightframe].m_set.get(),
		m_ambient.sets[&m_ambient.default_texture][*current_flightframe].get(),
		m_diffuse.sets[&m_diffuse.default_texture][*current_flightframe].get(),
		m_specular.sets[&m_specular.default_texture][*current_flightframe].get(),
		m_normal.sets[&m_normal.default_texture][*current_flightframe].get(),
	};

	const uint32_t first_set = 0;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									 m_layout.get(),
									 first_set,
									 init_sets.size(),
									 init_sets.data(),
									 0,
									 nullptr);
	
	TextureSamplerReadOnly* last_ambient_texture = &m_ambient.default_texture;
	TextureSamplerReadOnly* last_diffuse_texture = &m_diffuse.default_texture;
	TextureSamplerReadOnly* last_specular_texture = &m_specular.default_texture;
	TextureSamplerReadOnly* last_normal_texture = &m_normal.default_texture;

	int i = 0;
	for (MaterialRenderable& renderable: renderables) {
		
		TextureSamplerReadOnly* ambient_texture = renderable.texture.ambient 
			? renderable.texture.ambient 
			: &m_ambient.default_texture;
		
		if (ambient_texture != last_ambient_texture) {
			if (!m_ambient.sets.contains(ambient_texture)) {
				m_ambient.sets.insert({ambient_texture,
									  create_texture_descriptorset(device,
																   m_ambient.layout.get(),
																   descriptor_pool,
																   *ambient_texture)});

				logger.info(std::source_location::current(),
							"Added new ambient texture to cache");
			}	

			std::array<vk::DescriptorSet, 1> descriptorset{
				m_ambient.sets[ambient_texture][*current_flightframe].get()
			};
			commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
											 m_layout.get(),
											 *m_ambient.set_index,
											 descriptorset.size(),
											 descriptorset.data(),
											 0,
											 nullptr);
			last_ambient_texture = ambient_texture;
		}

		TextureSamplerReadOnly* diffuse_texture = renderable.texture.diffuse 
			? renderable.texture.diffuse 
			: &m_diffuse.default_texture;
		
		if (diffuse_texture != last_diffuse_texture) {
			if (!m_diffuse.sets.contains(diffuse_texture)) {
				m_diffuse.sets.insert({diffuse_texture,
									  create_texture_descriptorset(device,
																   m_diffuse.layout.get(),
																   descriptor_pool,
																   *diffuse_texture)});

				logger.info(std::source_location::current(),
							"Added new diffuse texture to cache");
			}	

			std::array<vk::DescriptorSet, 1> descriptorset{
				m_diffuse.sets[diffuse_texture][*current_flightframe].get()
			};
			commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
											 m_layout.get(),
											 *m_diffuse.set_index,
											 descriptorset.size(),
											 descriptorset.data(),
											 0,
											 nullptr);
			last_diffuse_texture = diffuse_texture;
		}
	

		TextureSamplerReadOnly* specular_texture = renderable.texture.specular 
			? renderable.texture.specular 
			: &m_specular.default_texture;
		
		if (specular_texture != last_specular_texture) {
			if (!m_specular.sets.contains(specular_texture)) {
				m_specular.sets.insert({specular_texture,
									  create_texture_descriptorset(device,
																   m_specular.layout.get(),
																   descriptor_pool,
																   *specular_texture)});

				logger.info(std::source_location::current(),
							"Added new specular texture to cache");
			}	

			std::array<vk::DescriptorSet, 1> descriptorset{
				m_specular.sets[specular_texture][*current_flightframe].get()
			};
			commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
											 m_layout.get(),
											 *m_specular.set_index,
											 descriptorset.size(),
											 descriptorset.data(),
											 0,
											 nullptr);
			last_specular_texture = specular_texture;
		}
		
		TextureSamplerReadOnly* normal_texture = renderable.texture.normal 
			? renderable.texture.normal 
			: &m_normal.default_texture;
		
		if (normal_texture != last_normal_texture) {
			if (!m_normal.sets.contains(normal_texture)) {
				m_normal.sets.insert({normal_texture,
									  create_texture_descriptorset(device,
																   m_normal.layout.get(),
																   descriptor_pool,
																   *normal_texture)});

				logger.info(std::source_location::current(),
							"Added new normal texture to cache");
			}	

			std::array<vk::DescriptorSet, 1> descriptorset{
				m_normal.sets[normal_texture][*current_flightframe].get()
			};
			commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
											 m_layout.get(),
											 *m_normal.set_index,
											 descriptorset.size(),
											 descriptorset.data(),
											 0,
											 nullptr);
			last_normal_texture = normal_texture;
		}
	
		PushConstants push{};
		push.model = renderable.model;
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
