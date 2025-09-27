#pragma once 

#include <filesystem>

#include "VertexImpl.hpp"
#include "VertexBuffer.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "ShaderTexture.hpp"

#include "ContextImpl.hpp"
#include "PresenterImpl.hpp"
#include "DescriptorPoolImpl.hpp"
#include "TextureImpl.hpp"
#include "ShaderTextureImpl.hpp"

#include <algorithm>
#include <map>

// TODO: We dont want a combined image sampler, we want them seperated to be able to swap them
// https://docs.vulkan.org/samples/latest/samples/api/separate_image_sampler/README.html

struct BaseTexturePipeline
{
	vk::UniquePipelineLayout layout;
    vk::UniquePipeline pipeline;

	/* The Camera descriptor loads persistent perspective
	 * data, and should happen as a single descriptor load
	 * every frame.
	 */
	struct Camera
	{
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct {
		vk::UniqueDescriptorSetLayout layout;
		std::vector<AllocatedMemory> memories;
		std::vector<vk::UniqueDescriptorSet> sets;
	} camera_descriptor;
	
	TextureSamplerReadOnly base_texture;
	struct {
		vk::UniqueDescriptorSetLayout layout;
		std::map<TextureSamplerReadOnly*, std::vector<vk::UniqueDescriptorSet>> sets;
	} texture_descriptor;

	struct PushConstants
	{
		glm::mat4 model;
	};
};

struct BaseTextureRenderInfo
{
	glm::mat4 view;
	glm::mat4 proj;
};

BaseTexturePipeline
create_base_texture_pipeline(Logger& logger,
							 Render::Context::Impl* context,
							 Presenter::Impl* presenter,
							 DescriptorPool::Impl* descriptor_pool,
							 vk::RenderPass& renderpass,
							 uint32_t frames_in_flight,
							 const vk::Extent2D render_extent,
							 const std::filesystem::path shader_root_path,
							 bool debug_print)
{
	const std::string pipeline_name = "BaseTexture";
	const std::filesystem::path vertexshader_name = "Diffuse.vert.spv";
	const std::filesystem::path fragmentshader_name = "Diffuse.frag.spv";
	logger.info(std::source_location::current(),
				std::format("Creating Pipeline {}",
							pipeline_name));
	logger.info(std::source_location::current(),
				std::format("  Vertex Shader {}",
							vertexshader_name.string()));
	logger.info(std::source_location::current(),
				std::format("  Fragment Shader {}",
							fragmentshader_name.string()));

	BaseTexturePipeline pipeline;
	const auto vert =
		read_binary_file((shader_root_path / vertexshader_name).string().c_str());
	if (!vert) {
		const auto msg = "COULD NOT LOAD VERTEX SHADER BINARY";
		logger.fatal(std::source_location::current(), msg);
		throw std::runtime_error(msg);
	}

	auto vertexShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*vert);

	vk::UniqueShaderModule vertex_module =
		context->device.get().createShaderModuleUnique(vertexShaderModuleCreateInfo);
	
	const auto frag =
		read_binary_file((shader_root_path / fragmentshader_name).string().c_str());
	if (!frag) {
		const auto msg = "COULD NOT LOAD FRAGMENT SHADER BINARY";
		logger.fatal(std::source_location::current(), msg);
		throw std::runtime_error(msg);
	}
	
	auto fragmentShaderModuleCreateInfo = vk::ShaderModuleCreateInfo{}
		.setFlags(vk::ShaderModuleCreateFlags())
		.setCode(*frag);

	vk::UniqueShaderModule fragment_module =
		context->device.get().createShaderModuleUnique(fragmentShaderModuleCreateInfo);
	
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

    std::array<vk::DynamicState, 2> dynamicStates{
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
		.setSize(sizeof(BaseTexturePipeline::PushConstants))
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	
	if (sizeof(BaseTexturePipeline::PushConstants) > 128) {
		logger.warn(std::source_location::current(), 
					std::format("PushConstant size={} is larger than minimum supported (128)"
								"This can cause compatability issues on some devices",
								sizeof(BaseTexturePipeline::PushConstants)));
	}

	std::array<vk::DescriptorSetLayoutBinding, 1> camera_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eVertex)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer),
	};

	const auto camera_set_info = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(camera_bindings);
	
	pipeline.camera_descriptor.layout =
		context->device.get().createDescriptorSetLayoutUnique(camera_set_info, nullptr);

	std::array<vk::DescriptorSetLayoutBinding, 1> base_texture_bindings{
		vk::DescriptorSetLayoutBinding{}
		.setStageFlags(vk::ShaderStageFlagBits::eFragment)
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eCombinedImageSampler),
	};

	const auto base_texture_set_info = vk::DescriptorSetLayoutCreateInfo{}
		.setFlags(vk::DescriptorSetLayoutCreateFlags())
		.setBindings(base_texture_bindings);
	
	pipeline.texture_descriptor.layout =
		context->device.get().createDescriptorSetLayoutUnique(base_texture_set_info,
															  nullptr);

	std::array<vk::DescriptorSetLayout, 2> descriptorset_layouts{
		pipeline.camera_descriptor.layout.get(),
		pipeline.texture_descriptor.layout.get(),
	};
	
    auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
		.setFlags(vk::PipelineLayoutCreateFlags())
		.setSetLayouts(descriptorset_layouts)
		.setPushConstantRanges(push_constant_range);

	pipeline.layout = 
		context->device.get().createPipelineLayoutUnique(pipelineLayoutCreateInfo);
	
	logger.info(std::source_location::current(), "Created Pipeline Layout");
	
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
	
	pipeline.pipeline = std::move(result.value);

	/*Allocate Camera Descriptor Sets*/
	for (uint32_t i = 0; i < frames_in_flight; i++) {
		pipeline.camera_descriptor.memories
			.push_back(allocate_memory(context->physical_device,
									   context->device.get(),
									   sizeof(BaseTexturePipeline::Camera),
									   vk::BufferUsageFlagBits::eTransferSrc
									   | vk::BufferUsageFlagBits::eUniformBuffer,
									   // Host Visible and Coherent allows direct
									   // writes into the buffers without sync issues.
									   vk::MemoryPropertyFlagBits::eHostVisible
									   | vk::MemoryPropertyFlagBits::eHostCoherent));
		
		const auto allocate_info = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(descriptor_pool->descriptor_pool.get())
			.setDescriptorSetCount(1)
			.setSetLayouts(pipeline.camera_descriptor.layout.get());
		
		auto sets = context->device.get().allocateDescriptorSetsUnique(allocate_info);
		if (sets.size() != 1) {
			logger.error(std::source_location::current(),
						 "This system only allows handling 1 set per frame in flight"
						 "\n if you want more sets find another way to store them..");
		}
		
		pipeline.camera_descriptor.sets.push_back(std::move(sets[0]));
		
		const auto buffer_info = vk::DescriptorBufferInfo{}
			.setBuffer(pipeline.camera_descriptor.memories[i].buffer.get())
			.setOffset(0)
			.setRange(sizeof(BaseTexturePipeline::Camera));
	
		const std::array<vk::WriteDescriptorSet, 1> camera_write{
			vk::WriteDescriptorSet{}
			.setDstBinding(0)
			.setDstArrayElement(0)
			.setDstSet(pipeline.camera_descriptor.sets[i].get())
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setBufferInfo(buffer_info),
		};
		
		BaseTexturePipeline::Camera camera{};	
		camera.view = glm::mat4(1.0f);
		camera.proj = glm::mat4(1.0f);
		
		copy_to_allocated_memory(context->device.get(),
								 pipeline.camera_descriptor.memories[i],
								 reinterpret_cast<void*>(&camera),
								 sizeof(camera));
		
		context->device.get().updateDescriptorSets(camera_write.size(),
												   camera_write.data(),
												   0,
												   nullptr);
	}

	logger.info(std::source_location::current(),
				"created camera descriptor sets");

	/*Setup base_texture*/
	const auto purple = Pixel8bitRGBA{170, 0, 170, 255};
	const auto yellow = Pixel8bitRGBA{170, 170, 0, 255};

	pipeline.base_texture = 
		create_canvas(purple, CanvasExtent{64, 64})
		| canvas_draw_checkerboard(yellow, CheckerSquareSize{4})
		| move_canvas_to_gpu(context)
		| make_shader_readonly(context, InterpolationType::Point);
	
	logger.info(std::source_location::current(),
				"created default invalid texture");

	return pipeline;
}

bool texture_changed(TextureSamplerReadOnly* lhs, TextureSamplerReadOnly* rhs)
{
	return lhs != rhs;
}

void draw_base_texture_renderables(BaseTexturePipeline& pipeline,
								   Logger& logger,
								   vk::Device& device,
								   vk::DescriptorPool descriptor_pool,
								   vk::CommandBuffer& commandbuffer,
								   const uint32_t frame_in_flight,
								   const uint32_t max_frames_in_flight,
								   const BaseTextureRenderInfo& info,
								   std::vector<BaseTextureRenderable> renderables)
{
	// ensure base texture is available
	if (!pipeline.texture_descriptor.sets.contains(&pipeline.base_texture)) {
		logger.info(std::source_location::current(),
					"texture descriptor sets does not exist in descriptor cache yet,"
					" it will be created and added cached.");

		pipeline.texture_descriptor.sets.insert({
				&pipeline.base_texture,
				create_descriptorset_for_texture(device,
												 pipeline.texture_descriptor.layout.get(),
												 descriptor_pool,
												 max_frames_in_flight,
												 pipeline.base_texture)});
	}

	/* Norm Direction Drawing uses a setup where the model is provided
	 * in the push constant, and the view and proj is provided
	 * in a camera descriptor set.
	 * The view and proj can be written to directly.
	 */
	
	/* Bind the pipeline and the descriptor sets
	 */
	BaseTexturePipeline::Camera camera{};	
	camera.view = info.view;
	camera.proj = info.proj;
	
	copy_to_allocated_memory(device,
							 pipeline.camera_descriptor.memories[frame_in_flight],
							 reinterpret_cast<void*>(&camera),
							 sizeof(camera));
	

	commandbuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
							   pipeline.pipeline.get());
	
	std::array<vk::DescriptorSet, 2> init_sets{
		pipeline.camera_descriptor.sets[frame_in_flight].get(),
		pipeline.texture_descriptor.sets[&pipeline.base_texture][frame_in_flight].get()
	};
	const uint32_t first_set = 0;
	commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
									 pipeline.layout.get(),
									 first_set,
									 init_sets.size(),
									 init_sets.data(),
									 0,
									 nullptr);

	TextureSamplerReadOnly* last_bound_texture = &pipeline.base_texture;

	for (BaseTextureRenderable& renderable: renderables) {
		TextureSamplerReadOnly* texture = 
			renderable.texture ? renderable.texture : &pipeline.base_texture;
		
		if (texture_changed(texture, last_bound_texture)) {
			if (!pipeline.texture_descriptor.sets.contains(texture)) {
				pipeline.texture_descriptor.sets.insert({
						texture,
						create_descriptorset_for_texture(device,
														 pipeline.texture_descriptor.layout.get(),
														 descriptor_pool,
														 max_frames_in_flight,
														 *texture)});
			}

			const uint32_t first_set = 1;
			std::array<vk::DescriptorSet, 1> texture_descriptorset{
				pipeline.texture_descriptor.sets[texture][frame_in_flight].get()
			};
			commandbuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
											 pipeline.layout.get(),
											 first_set,
											 texture_descriptorset.size(),
											 texture_descriptorset.data(),
											 0,
											 nullptr);
			last_bound_texture = texture;
		}

		BaseTexturePipeline::PushConstants push{};
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
