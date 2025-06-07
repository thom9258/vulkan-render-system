#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>

#include <VulkanRenderer/VulkanRenderer.hpp>
#include <VulkanRenderer/GeometryPass.hpp>
#include <VulkanRenderer/Bitmap.hpp>
#include <VulkanRenderer/Canvas.hpp>
#include <VulkanRenderer/ShaderTexture.hpp>

#define SIMPLE_GEOMETRY_IMPLEMENTATION
#include <simple_geometry.h>

const std::string root = "../../../";
const std::string shaders_root = root + "compiled_shaders/";
const std::string assets_root = root + "assets/";


TexturedMesh 
textured_cube(vk::PhysicalDevice& physical_device,
			  vk::Device& device)
{
	sg_status status;
	size_t vertices_length{0};
	
	sg_cube_info cube_info{};
	cube_info.width = 0.5f;
	cube_info.height = 0.5f;
	cube_info.depth = 0.5f;
	
	status = sg_cube_vertices(&cube_info, &vertices_length, nullptr, nullptr, nullptr);
	if (status != SG_OK_RETURNED_LENGTH)
		throw std::runtime_error("Could not get positions size");

	std::vector<sg_position> positions(vertices_length);
	std::vector<sg_normal> normals(vertices_length);
	std::vector<sg_texcoord> texcoords(vertices_length);
	status = sg_cube_vertices(&cube_info,
							  &vertices_length,
							  positions.data(),
							  normals.data(), 
							  texcoords.data());
	if (status != SG_OK_RETURNED_BUFFER)
		throw std::runtime_error("Could not get vertices");

	std::vector<VertexPosNormColorUV> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = {positions[i].x, positions[i].y, positions[i].z};
		vertices[i].norm = {normals[i].x, normals[i].y, normals[i].z};
		vertices[i].color = {1.0f, 1.0f, 1.0f};
		vertices[i].uv = {texcoords[i].u, texcoords[i].v};
	}
	
	TexturedMesh mesh{};
	mesh.vertexbuffer = create_vertex_buffer(physical_device,
											 device,
											 vertices);
	return mesh;
}

template <typename F, typename... Args>
std::chrono::duration<double>
with_time_measurement(F&& f, Args&&... args)
{
	using Clock = std::chrono::high_resolution_clock;
	auto start = Clock::now();
	std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
	auto end = Clock::now();
	return end - start;
}

constexpr bool slowframes = false;
constexpr bool printframerate = true;
constexpr size_t printframerateinterval = 100;

std::vector<VertexPosNormColor> triangle_vertices = {
	{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
};

int main()
{

	WindowConfig window_config;
	Logger logger;
	PresentationContext context(window_config, logger);

	const auto window = context.get_window_extent();
	const auto aspect = static_cast<float>(window.width()) / static_cast<float>(window.height());
	
	WorldRenderInfo world_info{};
    world_info.view = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -4.f));
    world_info.projection = glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.0f);
    world_info.projection[1][1] *= -1;
	
	GeometryPass pass = create_geometry_pass(context.physical_device,
											 context.device.get(),
											 context.command_pool(),
											 context.graphics_queue(),
											 context.get_window_extent(),
											 2,
											 true);
	
	std::array<vk::DescriptorPoolSize, 2> sizes{
		vk::DescriptorPoolSize{}
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(100),
		vk::DescriptorPoolSize{}
		.setType(vk::DescriptorType::eCombinedImageSampler)
		.setDescriptorCount(100),
	};
	
	const auto pool_info = vk::DescriptorPoolCreateInfo{}
		.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
		.setMaxSets(200)
		.setPoolSizes(sizes);
	
	auto descriptor_pool =
		context.device.get().createDescriptorPoolUnique(pool_info, nullptr);
	
	Pipelines pipelines = create_pipelines(context,
										   descriptor_pool.get(),
										   pass.renderpass.get(),
										   context.maxFramesInFlight_,
										   context.get_window_extent(),
										   shaders_root,
										   true);

	Mesh triangle_mesh{};
	triangle_mesh.vertexbuffer = create_vertex_buffer(context.physical_device,
													  context.device.get(),
													  triangle_vertices);
	

	TexturedMesh cube_mesh = 
		textured_cube(context.physical_device, context.device.get());

	auto loaded_monkey_mesh = load_obj(assets_root,
									   "models/monkey/monkey_flat.obj",
									   context.physical_device,
									   context.device.get());
	
	Mesh monkey_mesh{};
	if (std::holds_alternative<Mesh>(loaded_monkey_mesh)) {
		monkey_mesh = std::move(std::get<Mesh>(loaded_monkey_mesh));
	}
	else if (std::holds_alternative<MeshWithWarning>(loaded_monkey_mesh)) {
		std::cout << "TinyOBJ Warning: " 
				  << std::get<MeshWithWarning>(loaded_monkey_mesh).warning
				  << std::endl;
		monkey_mesh = std::move(std::get<MeshWithWarning>(loaded_monkey_mesh).mesh);
	}
	else if (std::holds_alternative<MeshInvalidPath>(loaded_monkey_mesh)) {
		auto error = std::get<MeshInvalidPath>(loaded_monkey_mesh);
		throw std::runtime_error(std::string("InvalidPath: ")
								 + (error.path / error.filename).string());
	}
	else if (std::holds_alternative<MeshLoadError>(loaded_monkey_mesh)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") +
								 std::get<MeshLoadError>(loaded_monkey_mesh).msg);
	}
	
	//https://opengameart.org/art-search-advanced?keys=&field_art_type_tid%5B0%5D=10&sort_by=count&sort_order=DESC&page=3
	std::cout << "loading chest model!" << std::endl;
	auto loaded_chest = load_obj_with_texcoords(assets_root,
												"models/ChestWowStyle/Chest.obj",
												context.physical_device,
												context.device.get());
	TexturedMesh chest_mesh{};
	if (auto p = std::get_if<TexturedMesh>(&loaded_chest)) {
		chest_mesh = std::move(*p);
	}
	else if (auto p = std::get_if<TexturedMeshWithWarning>(&loaded_chest)) {
		std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
		chest_mesh = std::move(p->mesh);
	}
	else if (auto p = std::get_if<MeshLoadError>(&loaded_chest)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
	}
	
	std::cout << "loading chest texture!" << std::endl;
	TextureSamplerReadOnly chest_texture;
	auto loaded_chest_texture = 
		load_bitmap(assets_root + "models/ChestWowStyle/diffuse.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes);

	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_chest_texture)) {
		chest_texture = std::move(*p) 
			| move_bitmap_to_gpu(context.physical_device,
								 context.device.get(),
								 context.command_pool(),
								 context.graphics_queue(),
								 vk::MemoryPropertyFlagBits::eDeviceLocal)
			| make_shader_readonly(context.physical_device,
								   context.device.get(),
								   context.graphics_queue(),
								   InterpolationType::Linear,
								   context.command_pool());
	}
	else if (auto p = std::get_if<InvalidPath>(&loaded_chest_texture)) {
		std::cout << "Invalid texture path: " << p->path << std::endl;
	}
	else if (auto p = std::get_if<LoadError>(&loaded_chest_texture)) {
		std::cout << "Texture Load Error: " << p->why << std::endl;
	}
	else if (std::get_if<InvalidNativePixels>(&loaded_chest_texture)) {
		std::cout << "Texture has invalid native pixels" << std::endl;
	}


	
	
	std::cout << "loading statue jpg!" << std::endl;
	TextureSamplerReadOnly statue;
	auto loaded_statue =
		load_bitmap(assets_root + "textures/texture.jpg",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes);
	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_statue)) {
		statue = std::move(*p) 
			| move_bitmap_to_gpu(context.physical_device,
								 context.device.get(),
								 context.command_pool(),
								 context.graphics_queue(),
								 vk::MemoryPropertyFlagBits::eDeviceLocal)
			| make_shader_readonly(context.physical_device,
								   context.device.get(),
								   context.graphics_queue(),
								   InterpolationType::Point,
								   context.command_pool());
	}
	else if (auto p = std::get_if<InvalidPath>(&loaded_statue)) {
		std::cout << "Invalid texture path: " << p->path << std::endl;
	}
	else if (auto p = std::get_if<LoadError>(&loaded_statue)) {
		std::cout << "Texture Load Error: " << p->why << std::endl;
	}
	else if (std::get_if<InvalidNativePixels>(&loaded_statue)) {
		std::cout << "Texture has invalid native pixels" << std::endl;
	}

	std::cout << "loading lulu jpg!" << std::endl;
	TextureSamplerReadOnly lulu;
	auto loaded_lulu = load_bitmap(assets_root + "textures/lulu.jpg",
								   BitmapPixelFormat::RGBA,
								   VerticalFlipOnLoad::Yes);
	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_lulu)) {
		lulu = std::move(*p) 
			| move_bitmap_to_gpu(context.physical_device,
								 context.device.get(),
								 context.command_pool(),
								 context.graphics_queue(),
								 vk::MemoryPropertyFlagBits::eDeviceLocal)
			| make_shader_readonly(context.physical_device,
								   context.device.get(),
								   context.graphics_queue(),
								   InterpolationType::Linear,
								   context.command_pool());
	}
	else if (auto p = std::get_if<InvalidPath>(&loaded_lulu)) {
		std::cout << "Invalid texture path: " << p->path << std::endl;
	}
	else if (auto p = std::get_if<LoadError>(&loaded_lulu)) {
		std::cout << "Texture Load Error: " << p->why << std::endl;
	}
	else if (std::get_if<InvalidNativePixels>(&loaded_lulu)) {
		std::cout << "Texture has invalid native pixels" << std::endl;
	}

	std::cout << "STARTING DRAW LOOP" << std::endl;
	/** ************************************************************************
	 * Frame Loop
	 */
	SDL_Event event{};
	bool exit = false;
	uint64_t framecount = 0;

	while (!exit) {
		/** ************************************************************************
		 * Handle Inputs
		 */
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT: 
				exit = true;
				break;
				
			case SDL_KEYDOWN:
				switch( event.key.keysym.sym ) {
				case SDLK_ESCAPE:
					exit = true;
					break;
				}
				
				break;
				
			case SDL_WINDOWEVENT: {
				const SDL_WindowEvent& wev = event.window;
				switch (wev.event) {
				case SDL_WINDOWEVENT_RESIZED:
				case SDL_WINDOWEVENT_SIZE_CHANGED:
					context.window_resize_event_triggered();
					//TODO: DO RESIZE
					break;
				case SDL_WINDOWEVENT_CLOSE:
					exit = true;
					break;
				}
			} break;
			}
		}
			
		/** ************************************************************************
		 * Render Loop
		 */
		FrameProducer frameGenerator = [&] (CurrentFrameInfo frameInfo)
			-> std::optional<Texture2D*>
			{
				std::vector<Renderable> renderables{};
			
				BaseTextureRenderable chest{};
				chest.mesh = &chest_mesh;
				chest.texture = &chest_texture;
				chest.model = glm::mat4(1.0f);
				chest.model = glm::translate(chest.model, glm::vec3(0.0f, -1.0f, 0.0f));
				chest.model = glm::scale(chest.model, glm::vec3(0.02f));
				chest.model = glm::rotate(chest.model,
										  glm::radians(frameInfo.total_frame_count * 1.0f),
										  glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(chest);

#if 1
				BaseTextureRenderable t{};
				t.mesh = &cube_mesh;
				t.texture = &lulu;
				t.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.4f, 0.0f));
				t.model = glm::rotate(t.model,
									  glm::radians(frameInfo.total_frame_count * 1.0f),
									  glm::normalize(glm::vec3(0, 1, 1)));
				renderables.push_back(t);
				
				t.mesh = &cube_mesh;
				t.texture = &statue;
				t.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.4f, 0.0f));
				t.model = glm::rotate(t.model,
									  glm::radians(frameInfo.total_frame_count * 1.0f),
									  glm::normalize(glm::vec3(0, 1, 1)));
				renderables.push_back(t);
				
				NormColorRenderable d2{};
				d2.mesh = &monkey_mesh;
				d2.model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
				d2.model = glm::rotate(d2.model,
									   glm::radians(frameInfo.total_frame_count * 1.0f),
									   glm::vec3(0, 1, 0));
				renderables.push_back(d2);
				
				WireframeRenderable d3{};
				d3.basecolor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				d3.mesh = &monkey_mesh;
				d3.model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, -0.5f, 0.0f));
				d3.model = glm::rotate(d3.model,
									   glm::radians(frameInfo.total_frame_count * 1.0f),
									   glm::vec3(-0.7, 0.7, 0));
				renderables.push_back(d3);
#endif

				auto* textureptr = render(pass,
										  &pipelines,
										  frameInfo.current_flight_frame_index,
										  context.maxFramesInFlight_,
										  frameInfo.total_frame_count,
										  context.device.get(),
										  descriptor_pool.get(),
										  context.command_pool(),
										  context.graphics_queue(),
										  world_info,
										  renderables);
				
				if (textureptr == nullptr)
					return std::nullopt;
				return textureptr;
			};
			
		auto render_time = with_time_measurement([&] () {
			context.with_presentation(frameGenerator);
		});

		if (slowframes) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}	
		if (printframerate) {
			if (framecount % printframerateinterval == 0) {
				const auto frame_time_ms =
					std::chrono::duration_cast<std::chrono::milliseconds>(render_time);
				
				std::cout << "Frame Time [ms]: " << frame_time_ms.count() << "\n"
						  << "Frame Count:     " << framecount << "\n"
						  << "====================================="
						  << std::endl;
			}
		}

		framecount++;
	}
	

	context.device.get().waitIdle();
	return 0;
}
