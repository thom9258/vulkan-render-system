#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>
#include <ranges>
#include <thread>

#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/Presenter.hpp>
#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/Light.hpp>
#include <VulkanRenderer/Renderer.hpp>
#include <VulkanRenderer/DescriptorPool.hpp>
#include <VulkanRenderer/Vertex.hpp>
#include <VulkanRenderer/Bitmap.hpp>
#include <VulkanRenderer/Canvas.hpp>
#include <VulkanRenderer/ShaderTexture.hpp>

#include "LoadResources.hpp"

#define SIMPLE_GEOMETRY_IMPLEMENTATION
#include <simple_geometry.h>

std::filesystem::path root = "../../../";
std::filesystem::path shaders_root = root / "compiled_shaders/";
std::filesystem::path assets_root = root / "assets/";
std::filesystem::path models_root = assets_root / "models/";
std::filesystem::path textures_root = assets_root / "textures/";

auto get_textured_cube_vertices()
	-> std::vector<VertexPosNormColorUV> 
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

	return vertices;
}

auto get_cube_vertices()
	-> std::vector<VertexPosNormColor> 
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
	status = sg_cube_vertices(&cube_info,
							  &vertices_length,
							  positions.data(),
							  normals.data(), 
							  nullptr);
	if (status != SG_OK_RETURNED_BUFFER)
		throw std::runtime_error("Could not get vertices");

	std::vector<VertexPosNormColor> vertices(positions.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		vertices[i].pos = {positions[i].x, positions[i].y, positions[i].z};
		vertices[i].norm = {normals[i].x, normals[i].y, normals[i].z};
		vertices[i].color = {1.0f, 1.0f, 1.0f};
	}

	return vertices;
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
	//window_config.size = U32Extent{400, 300};

	Logger logger;
	
	std::ofstream logfile;
	logfile.open("./Engine.log");

	logger.log = [&] (std::source_location loc, Logger::Type type, std::string msg) {
		
		std::string typestr = "?";
		switch (type) {
			case Logger::Type::Info: 
				typestr = "Info";
				break;
			case Logger::Type::Warn: 
				typestr = "Warn";
				break;
			case Logger::Type::Error: 
				typestr = "Error";
				break;
			case Logger::Type::Fatal: 
				typestr = "Fatal";
				break;
		};
		
		auto filename = std::string_view(loc.file_name());
		filename.remove_prefix(filename.find_last_of("/") + 1);
		
		const auto log = std::format("[{} L{}] ({}) {}\n",
									 filename,
									 loc.line(),
									 typestr,
									 msg);
		
		logfile << log;
		
		std::vector const serious_types{
			Logger::Type::Info,
			Logger::Type::Warn,
			Logger::Type::Error,
			Logger::Type::Fatal};

		bool const is_serious = std::ranges::find(serious_types, type) != serious_types.end();
		if (is_serious) {
			std::cout << log;
		}
	};

	Render::Context context(window_config, logger);
	Presenter presenter(&context, logger);

	const auto window = context.get_window_extent();
	const auto aspect = static_cast<float>(window.width()) / static_cast<float>(window.height());
	
	glm::vec3 constexpr camera_init_position = glm::vec3(0.0f, 0.0f, -3.0f);
	glm::vec3 constexpr camera_init_target = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 constexpr camera_init_up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 camera = glm::inverse(glm::lookAt(camera_init_position,
												camera_init_target,
												camera_init_up));

	WorldRenderInfo world_info{};
	world_info.camera_position = camera[3];
    world_info.view = glm::inverse(camera);
    world_info.projection = glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.0f);
    world_info.projection[1][1] *= -1;

	DescriptorPoolCreateInfo descriptor_pool_info;
	descriptor_pool_info.uniform_buffer_count = 100;
	descriptor_pool_info.combined_image_sampler_count = 100;

	DescriptorPool descriptor_pool(descriptor_pool_info,
								   context);

	Renderer renderer(context,
					  presenter,
					  logger,
					  descriptor_pool,
					  shaders_root);
	
	Mesh triangle_mesh{VertexBuffer::create(context, triangle_vertices)};

	const std::vector<VertexPosNormColorUV> textured_cube_vertices = get_textured_cube_vertices();
	TexturedMesh textured_cube_mesh{
		VertexBuffer::create<VertexPosNormColorUV>(context, textured_cube_vertices)
	};
	
	const std::vector<VertexPosNormColor> cube_vertices = get_cube_vertices();
	Mesh cube_mesh{
		VertexBuffer::create<VertexPosNormColor>(context, cube_vertices)
	};

	Resources resources{context, assets_root};



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
		glm::vec3 constexpr world_right = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 constexpr world_up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 constexpr world_forward = glm::vec3(0.0f, 0.0f, 1.0f);
#if 0
		glm::vec3 const camera_right = glm::vec3(glm::normalize(camera[0]));
		glm::vec3 const camera_up = glm::vec3(glm::normalize(camera[1]));
		glm::vec3 const camera_forward = glm::vec3(glm::normalize(camera[2]));
#else 
		glm::vec3 const camera_right = glm::vec3(camera[0][0], 
																camera[0][1],
																camera[0][2]
																);
		glm::vec3 const camera_up = glm::vec3(camera[1][0], 
															 camera[1][1],
															 camera[1][2]
															 );
		glm::vec3 const camera_forward = glm::vec3(camera[2][0], 
																  camera[2][1],
																  camera[2][2]
																  );

#endif		

		float constexpr move_speed = 0.5f;
		float constexpr rotate_speed = 2.0f;

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
				case SDLK_w:
					camera = glm::translate(camera, camera_forward * move_speed);
					break;
				case SDLK_s:
					camera = glm::translate(camera, camera_forward * -move_speed);
					break;
				case SDLK_d:
					camera = glm::translate(camera, camera_right * -move_speed);
					break;
				case SDLK_a:
					camera = glm::translate(camera, camera_right * move_speed);
					break;
				case SDLK_e:
					camera = glm::translate(camera, camera_up * -move_speed);
					break;
				case SDLK_q:
					camera = glm::translate(camera, camera_up * move_speed);
					break;
					
				case SDLK_LEFT:
					camera = glm::rotate(camera,
										 glm::radians(rotate_speed),
										 world_up);
					break;
				case SDLK_RIGHT:
					camera = glm::rotate(camera,
										 glm::radians(-rotate_speed),
										 world_up);
					break;

				}
				break;
				
			case SDL_WINDOWEVENT: {
				switch (event.window.event) {
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
		world_info.camera_position = camera[3];
		world_info.view = glm::inverse(camera);
		
		std::cout << std::format("Camera forward  {} {} {}\n"
								 "       right    {} {} {}\n"
								 "       up       {} {} {}\n"
								 "       position {} {} {}",
								 camera_forward[0],
								 camera_forward[1],
								 camera_forward[2],
								 camera_right[0],
								 camera_right[1],
								 camera_right[2],
								 camera_up[0],
								 camera_up[1],
								 camera_up[2],
								 world_info.camera_position[0],
								 world_info.camera_position[1],
								 world_info.camera_position[2]
								 )
			<< std::endl;
		
			
		/** ************************************************************************
		 * Render Loop
		 */
		FrameProducer frameGenerator = [&] (CurrentFrameInfo frameInfo)
			-> std::optional<Texture2D::Impl*>
			{
				std::vector<Renderable> renderables{};
#if 0
				BaseTextureRenderable chest{};
				chest.mesh = &resources.chest.mesh;
				chest.texture = &resources.chest.diffuse;
				chest.model = glm::mat4(1.0f);
				chest.model = glm::translate(chest.model, glm::vec3(0.0f, -1.0f, 0.0f));
				chest.model = glm::scale(chest.model, glm::vec3(0.02f));
				chest.model = glm::rotate(chest.model,
										  glm::radians(frameInfo.total_frame_count * 1.0f),
										  glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(chest);
				
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
				d2.mesh = &resources.monkey.mesh;
				d2.model = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
				d2.model = glm::rotate(d2.model,
									   glm::radians(frameInfo.total_frame_count * 1.0f),
									   glm::vec3(0, 1, 0));
				renderables.push_back(d2);
				
				WireframeRenderable d3{};
				//d3.basecolor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				d3.basecolor = glm::vec4(1.0f);
				d2.mesh = &resources.monkey.mesh;
				d3.model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 0.0f));
				d3.model = glm::rotate(d3.model,
									   glm::radians(frameInfo.total_frame_count * 1.0f),
									   glm::vec3(-0.7, 0.7, 0));
				renderables.push_back(d3);
				
				BaseTextureRenderable basesmg{};
				basesmg.mesh = &smg.textured_mesh;
				basesmg.texture = &smg.diffuse;
				basesmg.model = glm::mat4(1.0f);
				basesmg.model = glm::translate(basesmg.model, glm::vec3(1.5f, 0.0f, 0.0f));
				basesmg.model = glm::scale(basesmg.model, glm::vec3(0.4f));
				basesmg.model = glm::rotate(basesmg.model,
											glm::radians(frameInfo.total_frame_count * 1.0f),
											glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(basesmg);

				DirectionalLight dl;
				dl.direction = glm::vec3(-1.0f, 0.0f, -1.0f);
				lights.push_back(dl);
#endif				
				
				std::vector<Light> lights;
				PointLight pl;
				pl.position = glm::vec3(0.0f, 0.0f, 0.2f);
				pl.ambient = glm::vec3(0.4f);
				pl.diffuse = glm::vec3(1.0f);
				pl.specular = glm::vec3(1.0f);
				pl.attenuation.constant = 1.0f;
				pl.attenuation.linear = 0.09f;
				pl.attenuation.quadratic = 0.032f;

				lights.push_back(pl);
				
				MaterialRenderable chest{};
				chest.mesh = &resources.chest.mesh;
				chest.casts_shadow = true;
				chest.texture.ambient = nullptr;
				chest.texture.diffuse = &resources.chest.diffuse;
				chest.model = glm::mat4(1.0f);
				chest.model = glm::translate(chest.model, glm::vec3(1.5f, -1.0f, 1.5f));
				chest.model = glm::scale(chest.model, glm::vec3(0.02f));
				chest.model = glm::rotate(chest.model,
										  //glm::radians(3.14f/2),
										  glm::radians(frameInfo.total_frame_count * 0.5f),
										  glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(chest);
	
				
				WireframeRenderable d3{};
				//d3.basecolor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				d3.basecolor = glm::vec4(1.0f);
				d3.mesh = &cube_mesh;
				d3.model = glm::translate(glm::mat4(1.0f), pl.position);
				d3.model = glm::scale(d3.model, glm::vec3(0.1f));
				renderables.push_back(d3);
				
				MaterialRenderable smg{};
				smg.mesh = &resources.smg.textured_mesh;
				smg.casts_shadow = true;
				smg.texture.ambient = nullptr;
				smg.texture.diffuse = &resources.smg.diffuse;
				smg.texture.specular = &resources.smg.specular;
				smg.texture.normal = &resources.smg.normal;
				smg.model = glm::mat4(1.0f);
				smg.model = glm::translate(smg.model, glm::vec3(-1.5f, 0.0f, 0.0f));
				smg.model = glm::scale(smg.model, glm::vec3(0.2f));
				smg.model = glm::rotate(smg.model,
										glm::radians(frameInfo.total_frame_count * 1.0f),
										glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(smg);
				
				MaterialRenderable box{};
				box.mesh = &textured_cube_mesh;
				box.casts_shadow = true;
				box.texture.ambient = nullptr;
				box.texture.diffuse = &resources.box.diffuse;
				box.texture.specular = &resources.box.specular;
				//box.texture.normal = &box_normal;
				box.model = glm::mat4(1.0f);
				box.model = glm::translate(box.model, glm::vec3(1.5f, 0.8f, 0.0f));
				box.model = glm::rotate(box.model,
										glm::radians(frameInfo.total_frame_count * 1.0f),
										glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(box);
				
				MaterialRenderable floor{};
				floor.mesh = &textured_cube_mesh;
				floor.casts_shadow = true;
				floor.texture.diffuse = &resources.brickwall.diffuse;
				floor.texture.specular = &resources.brickwall.specular;
				floor.texture.normal = &resources.brickwall.normal;
				floor.model = glm::mat4(1.0f);
				floor.model = glm::translate(floor.model, glm::vec3(0.0f, -1.0f, 0.0f));
				floor.model = glm::scale(floor.model, glm::vec3(10.0f, 0.1f, 10.0f));
				renderables.push_back(floor);
	
				
#if 0				
				NormColorRenderable d2{};
				d2.mesh = &smg.mesh;
				d2.model = glm::mat4(1.0f);
				d2.model = glm::translate(d2.model, glm::vec3(1.5f, -0.8f, 0.0f));
				d2.model = glm::scale(d2.model, glm::vec3(0.4f));
				d2.model = glm::rotate(d2.model,
									   glm::radians(frameInfo.total_frame_count * 1.0f),
									   glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(d2);
#endif	

				auto* textureptr = renderer.render(frameInfo.current_flight_frame_index,
												   frameInfo.total_frame_count,
												   world_info,
												   renderables,
												   lights);
			
				if (textureptr == nullptr)
					return std::nullopt;
				return textureptr;
			};
			
		auto render_time = with_time_measurement([&] () {
			presenter.with_presentation(frameGenerator);
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
	
	context.wait_until_idle();
	logfile.close();
	return 0;
}
