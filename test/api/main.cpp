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

#define SIMPLE_GEOMETRY_IMPLEMENTATION
#include <simple_geometry.h>

std::filesystem::path root = "../../../";
std::filesystem::path shaders_root = root / "compiled_shaders/";
std::filesystem::path assets_root = root / "assets/";
std::filesystem::path models_root = assets_root / "models/";
std::filesystem::path textures_root = assets_root / "textures/";

auto textured_cube_vertices()
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
	
	WorldRenderInfo world_info{};
    world_info.view = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -4.f));
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

	const std::vector<VertexPosNormColorUV> cube_vertices = textured_cube_vertices();
	TexturedMesh cube_mesh{
		VertexBuffer::create<VertexPosNormColorUV>(context, cube_vertices)
	};

	auto loaded_monkey_mesh = load_obj(context,
									   assets_root,
									   "models/monkey/monkey_flat.obj");
	
	Mesh monkey_mesh;
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
	auto loaded_chest = load_obj_with_texcoords(context,
												assets_root,
												"models/ChestWowStyle/Chest.obj");
	TexturedMesh chest_mesh;
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
		load_bitmap(models_root / "ChestWowStyle/diffuse.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes);

	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_chest_texture)) {
		chest_texture = std::move(*p) 
			| move_bitmap_to_gpu(&context)
			| make_shader_readonly(&context, InterpolationType::Linear);
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
	
	std::cout << "loading smg model!" << std::endl;
	auto loaded_smg = load_obj_with_texcoords(context,
											  models_root,
											  "smg/smg.obj");
	TexturedMesh smg_mesh;
	if (auto p = std::get_if<TexturedMesh>(&loaded_smg)) {
		smg_mesh = std::move(*p);
	}
	else if (auto p = std::get_if<TexturedMeshWithWarning>(&loaded_smg)) {
		std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
		smg_mesh = std::move(p->mesh);
	}
	else if (auto p = std::get_if<MeshLoadError>(&loaded_smg)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
	}

	std::cout << "loading smg texture!" << std::endl;
	TextureSamplerReadOnly smg_diffuse;
	auto loaded_smg_diffuse = 
		load_bitmap(models_root / "smg/D.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes);

	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_smg_diffuse)) {
		smg_diffuse = std::move(*p) 
			| move_bitmap_to_gpu(&context)
			| make_shader_readonly(&context, InterpolationType::Linear);
	}
	else if (auto p = std::get_if<InvalidPath>(&loaded_smg_diffuse)) {
		std::cout << "Invalid texture path: " << p->path << std::endl;
	}
	else if (auto p = std::get_if<LoadError>(&loaded_smg_diffuse)) {
		std::cout << "Texture Load Error: " << p->why << std::endl;
	}
	else if (std::get_if<InvalidNativePixels>(&loaded_smg_diffuse)) {
		std::cout << "Texture has invalid native pixels" << std::endl;
	}



	std::cout << "loading statue jpg!" << std::endl;
	TextureSamplerReadOnly statue;
	auto loaded_statue =
		load_bitmap(textures_root / "texture.jpg",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No);
	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_statue)) {
		statue = std::move(*p) 
			| move_bitmap_to_gpu(&context)
			| make_shader_readonly(&context, InterpolationType::Point);
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
	auto loaded_lulu = load_bitmap(textures_root / "lulu.jpg",
								   BitmapPixelFormat::RGBA,
								   VerticalFlipOnLoad::No);
	if (auto p = std::get_if<LoadedBitmap2D>(&loaded_lulu)) {
		lulu = std::move(*p) 
			| move_bitmap_to_gpu(&context)
			| make_shader_readonly(&context, InterpolationType::Linear);
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
			-> std::optional<Texture2D::Impl*>
			{
				std::vector<Renderable> renderables{};
#if 0			
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
				d3.model = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.5f, 0.0f));
				d3.model = glm::rotate(d3.model,
									   glm::radians(frameInfo.total_frame_count * 1.0f),
									   glm::vec3(-0.7, 0.7, 0));
				renderables.push_back(d3);
#endif				
				
#if 1
				BaseTextureRenderable smg{};
				smg.mesh = &smg_mesh;
				smg.texture = &smg_diffuse;
				smg.model = glm::mat4(1.0f);
				smg.model = glm::translate(smg.model, glm::vec3(0.0f, 0.0f, 0.0f));
				smg.model = glm::scale(smg.model, glm::vec3(0.4f));
				smg.model = glm::rotate(smg.model,
										glm::radians(frameInfo.total_frame_count * 1.0f),
										glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(smg);
#else
				MaterialRenderable smg{};
				smg.mesh = &smg_mesh;
				smg.casts_shadow = true;
				smg.texture.diffuse = &smg_diffuse;
				smg.model = glm::mat4(1.0f);
				smg.model = glm::translate(smg.model, glm::vec3(0.0f, 0.0f, 0.0f));
				smg.model = glm::scale(smg.model, glm::vec3(0.4f));
				smg.model = glm::rotate(smg.model,
										glm::radians(frameInfo.total_frame_count * 1.0f),
										glm::normalize(glm::vec3(0, 1, 0)));
				renderables.push_back(smg);
#endif

				std::vector<Light> lights;
				PointLight pl;
				pl.position = glm::vec3(1.0f, 0.0f, 1.0f);
				lights.push_back(pl);

				AreaLight al;
				al.direction = glm::vec3(-1.0f, 0.0f, -1.0f);
				lights.push_back(al);

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
