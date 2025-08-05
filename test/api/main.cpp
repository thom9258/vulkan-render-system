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

std::filesystem::path root = "../../../";
std::filesystem::path shaders_root = root / "compiled_shaders/";
std::filesystem::path assets_root = root / "assets/";
std::filesystem::path models_root = assets_root / "models/";
std::filesystem::path textures_root = assets_root / "textures/";

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


struct Scene
{

	std::vector<Renderable> renderables;
	std::vector<Light> lights;
};

auto normcolor_scene(Resources& resources,
					 CurrentFrameInfo frameInfo)
	-> Scene
{
	Scene scene;
	
	NormColorRenderable box{};
	box.mesh = &resources.cube.mesh;
	box.model = glm::mat4(1.0f);
	box.model = glm::translate(box.model, glm::vec3(1.5f, 0.8f, 0.0f));
	box.model = glm::rotate(box.model,
							glm::radians(frameInfo.total_frame_count * 1.0f),
							glm::normalize(glm::vec3(0, 1, 0)));
	scene.renderables.push_back(box);
	
	NormColorRenderable floor{};
	floor.mesh = &resources.cube.mesh;
	floor.model = glm::mat4(1.0f);
	floor.model = glm::translate(floor.model, glm::vec3(0.0f, -1.0f, 0.0f));
	floor.model = glm::scale(floor.model, glm::vec3(10.0f, 0.1f, 10.0f));
	scene.renderables.push_back(floor);
		
	NormColorRenderable smg{};
	smg.mesh = &resources.smg.mesh;
	smg.model = glm::mat4(1.0f);
	smg.model = glm::translate(smg.model, glm::vec3(-1.5f, 0.0f, 0.0f));
	smg.model = glm::scale(smg.model, glm::vec3(0.2f));
	smg.model = glm::rotate(smg.model,
						   glm::radians(frameInfo.total_frame_count * 1.0f),
						   glm::normalize(glm::vec3(0, 1, 0)));
	scene.renderables.push_back(smg);
	return scene;
}

auto mixed_light_scene(Resources& resources,
					   CurrentFrameInfo frameInfo,
					   glm::vec3 campos,
					   glm::vec3 camfront,
					   bool spotlight_enabled)
	-> Scene
{
	Scene scene;
	
	//{
	//PointLight pl;
	//pl.position = glm::vec3(-2.0f, 1.0f, 0.7f);
	//pl.diffuse = glm::vec3(3.0f, 1.0f, 1.0f);
	//pl.ambient = pl.diffuse * 0.05f;
	//pl.specular = glm::vec3(1.0f) * 0.1f;
	//pl.attenuation.constant = 1.0f;
	//pl.attenuation.linear = 0.09f;
	//pl.attenuation.quadratic = 0.032f;
	//scene.lights.push_back(pl);
	//WireframeRenderable plbox{};
	//plbox.basecolor = glm::vec4(glm::normalize(pl.diffuse), 1.0f);
	//plbox.mesh = &resources.cube.mesh;
	//plbox.model = glm::translate(glm::mat4(1.0f), pl.position);
	//plbox.model = glm::scale(plbox.model, glm::vec3(0.05f));
	//scene.renderables.push_back(plbox);
//}
	{
		PointLight pl;
		pl.position = glm::vec3(-2.0f, 1.0f, -0.7f);
		pl.diffuse = glm::vec3(3.0f, 3.0f, 1.0f);
		pl.ambient = pl.diffuse * 0.05f;
		pl.specular = glm::vec3(1.0f) * 0.1f;
		pl.attenuation.constant = 1.0f;
		pl.attenuation.linear = 0.09f;
		pl.attenuation.quadratic = 0.032f;
		scene.lights.push_back(pl);
		WireframeRenderable plbox{};
		plbox.basecolor = glm::vec4(glm::normalize(pl.diffuse), 1.0f);
		plbox.mesh = &resources.cube.mesh;
		plbox.model = glm::translate(glm::mat4(1.0f), pl.position);
		plbox.model = glm::scale(plbox.model, glm::vec3(0.05f));
		scene.renderables.push_back(plbox);
	}
	
	DirectionalLight dl;
	dl.direction = glm::vec3(-0.1f, -0.2f, -0.0f);
	dl.diffuse = glm::vec3(1.0f, 1.0f, 0.4f);
	dl.ambient = dl.diffuse * 0.01f;
	dl.specular = glm::vec3(0.1f);
	scene.lights.push_back(dl);
	
	if (spotlight_enabled) {
		SpotLight s1;
		s1.position = glm::vec3(-1.0f, 0.0f, -1.0f);
		s1.direction = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));
		s1.ambient = glm::vec3(0.0f);
		s1.diffuse = glm::vec3(2.5f, 2.5f, 0.8f);
		s1.specular = glm::vec3(1.0f);
		s1.attenuation.constant = 1.0f;
		s1.attenuation.linear = 0.09f;
		s1.attenuation.quadratic = 0.032f;
		s1.cutoff.inner = glm::cos(glm::radians(30.0f));
		s1.cutoff.outer = glm::cos(glm::radians(35.0f));
		scene.lights.push_back(s1);
		
		WireframeRenderable spotpos{};
		spotpos.basecolor = glm::vec4(glm::normalize(s1.diffuse), 1.0f);
		spotpos.mesh = &resources.cube.mesh;
		spotpos.model = glm::translate(glm::mat4(1.0f), s1.position);
		spotpos.model = glm::scale(spotpos.model, glm::vec3(0.1f));
		scene.renderables.push_back(spotpos);
		WireframeRenderable spotdir{};
		spotdir.basecolor = glm::vec4(glm::normalize(s1.diffuse), 1.0f);
		spotdir.mesh = &resources.cube.mesh;
		spotdir.model = glm::translate(glm::mat4(1.0f), s1.position + s1.direction);
		spotdir.model = glm::scale(spotdir.model, glm::vec3(0.4f));
		scene.renderables.push_back(spotdir);
	}
	
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
	scene.renderables.push_back(chest);
	

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
	scene.renderables.push_back(smg);
	
	MaterialRenderable box{};
	box.mesh = &resources.cube.textured_mesh;
	box.casts_shadow = true;
	box.texture.ambient = nullptr;
	box.texture.diffuse = &resources.box.diffuse;
	box.texture.specular = &resources.box.specular;
	box.model = glm::mat4(1.0f);
	box.model = glm::translate(box.model, glm::vec3(1.5f, 0.8f, 0.0f));
	box.model = glm::rotate(box.model,
							glm::radians(frameInfo.total_frame_count * 1.0f),
							glm::normalize(glm::vec3(0, 1, 0)));
	scene.renderables.push_back(box);
	
	MaterialRenderable floor{};
	floor.mesh = &resources.cube.textured_mesh;
	floor.casts_shadow = true;
	floor.texture.diffuse = &resources.brickwall.diffuse;
	floor.texture.specular = &resources.brickwall.specular;
	floor.texture.normal = &resources.brickwall.normal;
	floor.model = glm::mat4(1.0f);
	floor.model = glm::translate(floor.model, glm::vec3(0.0f, -1.0f, 0.0f));
	floor.model = glm::scale(floor.model, glm::vec3(10.0f, 0.1f, 10.0f));
	scene.renderables.push_back(floor);

	return scene;
}



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
	
	struct {
		glm::vec3 position;
		glm::mat3 rotation;
		
		glm::mat4 view() 
		{ 
			return glm::translate(glm::inverse(glm::mat4(rotation)), position); 
		}
	} camera;

	camera.position = -camera_init_position;
	camera.rotation = glm::mat3(glm::inverse(glm::lookAt(camera_init_position,
														 camera_init_target,
														 camera_init_up)));
	
	WorldRenderInfo world_info{};
	world_info.camera_position = camera.position;
    world_info.view = camera.view();
    world_info.projection = glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.0f);
    world_info.projection[1][1] *= -1;

	DescriptorPoolCreateInfo descriptor_pool_info;
	descriptor_pool_info.uniform_buffer_count = 5000;
	descriptor_pool_info.combined_image_sampler_count = 5000;

	DescriptorPool descriptor_pool(descriptor_pool_info, context);

	Renderer renderer(context,
					  presenter,
					  logger,
					  descriptor_pool,
					  shaders_root);
	
	Resources resources{context, assets_root};



	std::cout << "STARTING DRAW LOOP" << std::endl;
	/** ************************************************************************
	 * Frame Loop
	 */
	SDL_Event event{};
	bool exit = false;
	uint64_t framecount = 0;
	
	bool spotlight_enabled = true;

	while (!exit) {
		/** ************************************************************************
		 * Handle Inputs
		 */
		glm::vec3 constexpr world_right = glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 constexpr world_up = glm::vec3(0.0f, 1.0f, 0.0f);
		glm::vec3 constexpr world_forward = glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 const camera_right = camera.rotation[0];
		glm::vec3 const camera_up = camera.rotation[1];
		glm::vec3 const camera_forward = camera.rotation[2];
		float constexpr move_speed = 0.5f;
		float constexpr rotate_speed = 3.0f;

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
					
				case SDLK_t:
					spotlight_enabled = !spotlight_enabled;
					break;
				case SDLK_w:
					camera.position += camera_forward * move_speed;
					break;
				case SDLK_s:
					camera.position += camera_forward * -move_speed;
					break;
				case SDLK_d:
					camera.position += camera_right * -move_speed;
					break;
				case SDLK_a:
					camera.position += camera_right * move_speed;
					break;
				case SDLK_e:
					camera.position += camera_up * -move_speed;
					break;
				case SDLK_q:
					camera.position += camera_up * move_speed;
					break;
				case SDLK_LEFT:
					camera.rotation = glm::mat3(glm::rotate(glm::mat4(camera.rotation),
															glm::radians(rotate_speed),
															world_up));
					break;
				case SDLK_RIGHT:
					camera.rotation = glm::mat3(glm::rotate(glm::mat4(camera.rotation),
															glm::radians(-rotate_speed),
															world_up));
					break;
				case SDLK_UP:
					camera.rotation = glm::mat3(glm::rotate(glm::mat4(camera.rotation),
															glm::radians(rotate_speed),
															world_right));
					break;
				case SDLK_DOWN:
					camera.rotation = glm::mat3(glm::rotate(glm::mat4(camera.rotation),
															glm::radians(-rotate_speed),
															world_right));
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

		world_info.camera_position = camera.position;
		world_info.view = camera.view();
#if 0		
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
#endif		
			
		/** ************************************************************************
		 * Render Loop
		 */
		FrameProducer frameGenerator = [&] (CurrentFrameInfo frameInfo)
			-> std::optional<Texture2D::Impl*>
			{
				Scene scene;
#if 1 
				scene = mixed_light_scene(resources,
										  frameInfo,
										  camera.position,
										  camera.position + camera_forward,
										  spotlight_enabled);
#else
				scene = normcolor_scene(resources, frameInfo);
#endif

				auto* textureptr = renderer.render(frameInfo.current_flight_frame_index,
												   frameInfo.total_frame_count,
												   world_info,
												   scene.renderables,
												   scene.lights);
			
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
