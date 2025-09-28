#include <iostream>
#include <sstream>
#include <fstream>
#include <variant>
#include <algorithm>
#include <chrono>
#include <thread>
#include <ranges>
#include <thread>
#include <fstream>
#include <streambuf>

#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/Presenter.hpp>
#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/Light.hpp>
#include <VulkanRenderer/ShadowCaster.hpp>
#include <VulkanRenderer/Renderer.hpp>
#include <VulkanRenderer/DescriptorPool.hpp>
#include <VulkanRenderer/Vertex.hpp>
#include <VulkanRenderer/Bitmap.hpp>
#include <VulkanRenderer/Canvas.hpp>
#include <VulkanRenderer/ShaderTexture.hpp>
#include <VulkanRenderer/Utils.hpp>
#include <VulkanRenderer/Transform.hpp>

#include "LoadResources.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

std::filesystem::path root = "../../../";
std::filesystem::path shaders_root = root / "compiled_shaders/";
std::filesystem::path scenes_root = "../scenes/";
std::filesystem::path assets_root = root / "assets/";
std::filesystem::path models_root = assets_root / "models/";
std::filesystem::path textures_root = assets_root / "textures/";


glm::vec3 constexpr world_right = glm::vec3(1.0f, 0.0f, 0.0f);
glm::vec3 constexpr world_up = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 constexpr world_forward = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 constexpr camera_init_position = glm::vec3(0.0f, 1.0f, -3.0f);
glm::vec3 constexpr camera_init_target = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 constexpr camera_init_up = glm::vec3(0.0f, 1.0f, 0.0f);
	

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

		
auto rotation_from_direction(glm::vec3 direction)
	-> glm::mat3
{
	glm::vec3 const rotationZ = direction;
	glm::vec3 const rotationX = glm::normalize(glm::cross(glm::vec3(0, 1, 0), rotationZ));
	glm::vec3 const rotationY = glm::normalize(glm::cross(rotationZ, rotationX));
	return glm::mat3(rotationX.x, rotationY.x, rotationZ.x,
					 rotationX.y, rotationY.y, rotationZ.y,
					 rotationX.z, rotationY.z, rotationZ.z);
}



constexpr bool slowframes = true;
constexpr bool printframerate = true;
constexpr size_t printframerateinterval = 1;

std::vector<VertexPosNormColor> triangle_vertices = {
	{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f},  {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
};


struct Scene
{
	std::vector<Renderable> renderables;
	std::vector<Light> lights;
	ShadowCasters shadowcasters;
};

auto parse_vec3(json j)
	-> glm::vec3
{
	return {j[0], j[1], j[2]};
}

auto parse_quat(json j)
	-> glm::quat
{
	return {j[0], j[1], j[2], j[3]};
}

auto parse_transform(json j)
	-> Render::Transform
{
	glm::vec3 const pos = parse_vec3(j["position"]);
	glm::quat const rot = parse_quat(j["rotation-wxyz"]);
	glm::vec3 const scale = parse_vec3(j["scale"]);
	return {pos, rot, scale};
}

auto load_scene_from_path(std::filesystem::path const path,
						  Resources& resources)
	-> Scene
{
	std::ifstream fs(path.string());
	std::string content;
	fs.seekg(0, std::ios::end);   
	content.reserve(fs.tellg());
	fs.seekg(0, std::ios::beg);

	content.assign((std::istreambuf_iterator<char>(fs)),
				   std::istreambuf_iterator<char>());

	json j = json::parse(content);
	Scene scene;

	json prefabs = j["prefabs"];
	for (auto& prefab: prefabs) {
		std::string name = prefab["name"];
		Render::Transform transform = parse_transform(prefab);
	
		if (name == "smg") {
			if (prefab["draw-mode"] == "material") {
				MaterialRenderable smg{};
				smg.mesh = &resources.smg.textured_mesh;
				if (prefab["has-shadow"] == "yes") {
					smg.has_shadow = true;
				}

				//smg.texture.ambient = &resources.smg.diffuse;
				smg.texture.ambient = nullptr;
				smg.texture.diffuse = &resources.smg.diffuse;
				smg.texture.specular = &resources.smg.specular;
				smg.texture.normal = &resources.smg.normal;
				smg.model = transform.as_matrix();
				scene.renderables.push_back(smg);
			}
			else if (prefab["draw-mode"] == "normcolor") {
				NormColorRenderable smg{};
				smg.mesh = &resources.smg.mesh;
				smg.model = transform.as_matrix();
				scene.renderables.push_back(smg);
			}
			else {
				std::cout << "Unknown draw mode for " << name << std::endl;
			}
		}
		else if (name == "chest") {
			if (prefab["draw-mode"] == "material") {
				MaterialRenderable chest{};
				chest.mesh = &resources.chest.textured_mesh;
				if (prefab["has-shadow"] == "yes") {
					chest.has_shadow = true;
				}

				chest.texture.ambient = &resources.chest.diffuse;
				chest.texture.diffuse = &resources.chest.diffuse;
				chest.texture.specular = &resources.chest.diffuse;
				chest.model = transform.as_matrix();
				scene.renderables.push_back(chest);
			}
			else if (prefab["draw-mode"] == "normcolor") {
				NormColorRenderable chest{};
				chest.mesh = &resources.chest.mesh;
				chest.model = transform.as_matrix();
				scene.renderables.push_back(chest);
			}
			else {
				std::cout << "Unknown draw mode for " << name << std::endl;
			}
		}
		else if (name == "transformship") {
			MaterialRenderable ship{};
			ship.mesh = &resources.transformship.mesh;
			if (prefab["has-shadow"] == "yes") {
				ship.has_shadow = true;
			}
			ship.texture.ambient = nullptr;
			ship.texture.diffuse = &resources.transformship.diffuse;
			ship.texture.specular = nullptr;
			ship.texture.normal = nullptr;
			ship.model = transform.as_matrix();
			scene.renderables.push_back(ship);
		}
		else if (name == "box") {
			if (prefab["draw-mode"] == "material") {
				MaterialRenderable box{};
				box.mesh = &resources.cube.textured_mesh;
				if (prefab["has-shadow"] == "yes") {
					box.has_shadow = true;
				}

				box.texture.ambient = &resources.box.diffuse;
				box.texture.diffuse = &resources.box.diffuse;
				box.texture.specular = &resources.box.specular;
				box.model = transform.as_matrix();
				scene.renderables.push_back(box);
			}
			else if (prefab["draw-mode"] == "normcolor") {
				NormColorRenderable box{};
				box.mesh = &resources.cube.mesh;
				box.model = transform.as_matrix();
				scene.renderables.push_back(box);
			}
			else {
				std::cout << "Unknown draw mode for " << name << std::endl;
			}
		}
		else if (name == "floor") {
			if (prefab["draw-mode"] == "material") {
				MaterialRenderable floor{};
				floor.mesh = &resources.cube.textured_mesh;
				if (prefab["has-shadow"] == "yes") {
					floor.has_shadow = true;
				}

				floor.texture.diffuse = &resources.brickwall.diffuse;
				floor.texture.specular = &resources.brickwall.specular;
				floor.texture.normal = &resources.brickwall.normal;
				floor.model = transform.as_matrix();
				scene.renderables.push_back(floor);
			}
			else {
				std::cout << "Unknown draw mode for " << name << std::endl;
			}
		}
		else {
			std::cout << "Unknown renderable " << name << std::endl;
		}
	}
	
	json lights = j["lights"];
	for (auto& obj: lights) {
		std::string type = obj["type"];

		if (type == "directional") {
			DirectionalLight p;
			p.direction = parse_vec3(obj["direction"]);
			p.ambient = parse_vec3(obj["ambient"]);
			p.specular = parse_vec3(obj["specular"]);
			p.diffuse = parse_vec3(obj["diffuse"]);
			
			const float near_plane = 0.1f, far_plane = 30.0f;
			const glm::vec3 position = parse_vec3(obj["position"]);
			
			DirectionalShadowCaster caster{
				OrthographicProjection{glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f,
												  near_plane, far_plane)},
				p,
				PositionVector{position},
				UpVector{world_up}};

			if (obj["casts-shadow"] == "yes") {
				scene.shadowcasters.directional_caster = caster;
			}
				
			if (obj["draw-gizmo"] == "yes") {
				MaterialRenderable ship{};
				ship.mesh = &resources.transformship.mesh;
				ship.has_shadow = false;
				ship.texture.ambient = nullptr;
				ship.texture.diffuse = &resources.transformship.diffuse;
				ship.texture.specular = nullptr;
				ship.texture.normal = nullptr;
				ship.model = caster.model();
				scene.renderables.push_back(ship);
			}
			else {
				scene.lights.push_back(p);
			}
		}
		else if (type == "spot") {
			SpotLight p;
			p.position = parse_vec3(obj["position"]);
			p.direction = glm::normalize(parse_vec3(obj["direction"]));
			p.ambient = parse_vec3(obj["ambient"]);
			p.specular = parse_vec3(obj["specular"]);
			p.diffuse = parse_vec3(obj["diffuse"]);
			p.attenuation.constant = obj["attenuation-constant"];
			p.attenuation.linear = obj["attenuation-linear"];
			p.attenuation.quadratic = obj["attenuation-quadratic"];
			p.cutoff.inner =
				glm::cos(glm::radians(static_cast<float>(obj["cutoff-inner-degrees"])));
			p.cutoff.outer =
				glm::cos(glm::radians(static_cast<float>(obj["cutoff-outer-degrees"])));
			
			const float aspect = 1;
			const float near_plane = 1.0f, far_plane = 20.0f;
			
			SpotShadowCaster caster{
				PerspectiveProjection{glm::perspective(glm::radians(70.f),
													   aspect,
													   near_plane,
													   far_plane)},
				p,
				UpVector{world_up}};


			if (obj["casts-shadow"] == "yes") {
				scene.shadowcasters.spot_casters.push_back(caster);
				//TODO: DO NOT PUSH A NORMAL LIGHT ONCE Casters are implemented in
				// materialpass
				scene.lights.push_back(p);
			}
			else {
				scene.lights.push_back(p);
			}

			if (obj["draw-gizmo"] == "yes") {
				MaterialRenderable ship{};
				ship.mesh = &resources.transformship.mesh;
				ship.has_shadow = false;
				ship.texture.ambient = nullptr;
				ship.texture.diffuse = &resources.transformship.diffuse;
				ship.texture.specular = nullptr;
				ship.texture.normal = nullptr;
				ship.model = caster.model();
				scene.renderables.push_back(ship);
			}
		}
		else if (type == "point") {
			PointLight p;
			p.position = parse_vec3(obj["position"]);
			p.ambient = parse_vec3(obj["ambient"]);
			p.specular = parse_vec3(obj["specular"]);
			p.diffuse = parse_vec3(obj["diffuse"]);
			p.attenuation.constant = obj["attenuation-constant"];
			p.attenuation.linear = obj["attenuation-linear"];
			p.attenuation.quadratic = obj["attenuation-quadratic"];
			scene.lights.push_back(p);
			
			if (obj["draw-gizmo"] == "yes") {
				WireframeRenderable gizmo{};
				gizmo.basecolor = glm::vec4(glm::normalize(p.diffuse), 1.0f);
				gizmo.mesh = &resources.gizmo_sphere.mesh;
				float const scale = p.attenuation.approximate_distance(0.03f);
				gizmo.model = glm::translate(glm::mat4(1.0f), p.position)
					* glm::scale(glm::mat4(1.0f), glm::vec3(scale));
				scene.renderables.push_back(gizmo);
				gizmo.model = glm::translate(glm::mat4(1.0f), p.position)
					* glm::scale(glm::mat4(1.0f), glm::vec3(scale * 0.04f));
				scene.renderables.push_back(gizmo);
			}
		}
		else {
			std::cout << "Unknown light " << type << std::endl;
		}
	}

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
	std::size_t scene_index = 0;

	while (!exit) {
		/** ************************************************************************
		 * Handle Inputs
		 */
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
					
				case SDLK_n:
					scene_index++;
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
				
				std::array<std::filesystem::path, 2> const scenes {
					scenes_root / "shadowtest.json",
					scenes_root / "normaltest.json"
				};

				if (scene_index > scenes.size() - 1)
					scene_index = 0;
				
				Scene scene	= load_scene_from_path(scenes.at(scene_index),
												   resources);

				auto* textureptr = renderer.render(frameInfo.current_flight_frame_index,
												   frameInfo.total_frame_count,
												   world_info,
												   scene.renderables,
												   scene.lights,
												   scene.shadowcasters);
			
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
