#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <sstream>
#include <streambuf>
#include <thread>
#include <variant>

#include <VulkanRenderer/Bitmap.hpp>
#include <VulkanRenderer/Canvas.hpp>
#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/DescriptorPool.hpp>
#include <VulkanRenderer/Light.hpp>
#include <VulkanRenderer/ModelLoader.hpp>
#include <VulkanRenderer/Presenter.hpp>
#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/Renderer.hpp>
#include <VulkanRenderer/ShaderTexture.hpp>
#include <VulkanRenderer/ShadowCaster.hpp>
#include <VulkanRenderer/TextureSamplerCache.hpp>
#include <VulkanRenderer/Transform.hpp>
#include <VulkanRenderer/Utils.hpp>
#include <VulkanRenderer/Vertex.hpp>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "PlayerController.hpp"
#include "generate_textured_cube.hpp"
#include "player.hpp"

void insert_animator(Animator *animator, RenderableNodePtr &renderable) {
  for (RenderableNode::Model &model : renderable->models) {
    if (auto *p = std::get_if<RenderableNode::AnimatedModel>(&model)) {
      p->animator = animator;
    }
  }
}

std::filesystem::path root = "../../../";
std::filesystem::path shaders_root = root / "compiled_shaders/";
// std::filesystem::path scenes_root = "../scenes/";
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
std::chrono::duration<double> with_time_measurement(F &&f, Args &&...args) {
  using Clock = std::chrono::high_resolution_clock;
  auto start = Clock::now();
  std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  auto end = Clock::now();
  return end - start;
}

int main(int argc, char **argv) {

  WindowConfig window_config;
  // RenderConfig render_config;
  // render_config.window_name = "Test Renderer";
  // render_config.window_extent = U32Extent{1200, 800};
  // render_config.render_extent = U32Extent{1200, 800};
  // render_config.shadow_extent = U32Extent{256, 256};

  Logger logger;

  std::ofstream logfile;
  logfile.open("./Game.log");

  logger.log = [&](std::source_location loc, Logger::Type type,
                   std::string msg) {
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

    const auto log =
        std::format("[{} L{}] ({}) {}\n", filename, loc.line(), typestr, msg);

    logfile << log;

    std::vector const serious_types{Logger::Type::Info, Logger::Type::Warn,
                                    Logger::Type::Error, Logger::Type::Fatal};

    bool const is_serious =
        std::ranges::find(serious_types, type) != serious_types.end();
    if (is_serious) {
      std::cout << log;
    }
  };

  Render::Context context(window_config, logger);
  Presenter presenter(&context, logger);

  const auto window = context.get_window_extent();
  const auto aspect =
      static_cast<float>(window.width()) / static_cast<float>(window.height());

  DescriptorPoolCreateInfo descriptor_pool_info;
  descriptor_pool_info.uniform_buffer_count = 5000;
  descriptor_pool_info.combined_image_sampler_count = 5000;

  DescriptorPool descriptor_pool(descriptor_pool_info, context);

  TextureSamplerCache texture_cache;
  MeshCache mesh_cache;
  Renderer renderer(context, presenter, logger, descriptor_pool, shaders_root);

  std::optional<SimpleMeshRef> textured_cube = mesh_cache.add(
      context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(
                   context, get_textured_cube_vertices())});

  TextureSamplerRef greybox_texture = texture_cache.load_from_path(
      &context, "greybox_texture", InterpolationType::Linear,
      VerticalFlipOnLoad::No, BitmapPixelFormat::RGBA,
      "../assets/GreyboxTextures/greybox_light_grid.png");

  TextureSamplerRef bluebox_texture = texture_cache.load_from_path(
      &context, "bluebox_texture", InterpolationType::Linear,
      VerticalFlipOnLoad::No, BitmapPixelFormat::RGBA,
      "../assets/GreyboxTextures/greybox_blue_grid.png");

  glm::mat4 projection =
      glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.0f);
  projection[1][1] *= -1;

  Camera camera(camera_init_position, camera_init_up, camera_init_target,
                projection);

  Player player(context, mesh_cache, texture_cache);
  player.translate(glm::vec3(-2.0f, 1.0f, 0.0f));

  PlayerController player_controller;

  bool reload_scene = false;
  bool exit = false;
  uint64_t framecount = 0;
  double delta_time = 0;
  double total_time = 0;

  auto poll_all_events = []() -> std::vector<SDL_Event> {
    std::vector<SDL_Event> events;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      events.push_back(event);
    }

    return events;
  };

  while (!exit) {
    auto duration_delta_time = with_time_measurement([&]() {
      /** ************************************************************************
       * Handle Inputs
       */

      std::vector<SDL_Event> events = poll_all_events();

      for (SDL_Event event : events) {
        switch (event.type) {
        case SDL_QUIT:
          exit = true;
          break;

        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            exit = true;
            break;
          case SDLK_r:
            reload_scene = true;
            break;
          }
          break;

        case SDL_WINDOWEVENT: {
          switch (event.window.event) {
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_SIZE_CHANGED:
            context.window_resize_event_triggered();
            // TODO: DO RESIZE
            break;
          case SDL_WINDOWEVENT_CLOSE:
            exit = true;
            break;
          }
        } break;
        }
      }

      /** ************************************************************************
       * Update
       */
      player_controller(player, delta_time / 100, events);
      camera_follow_player(camera, player);
      /** ************************************************************************
       * Render
       */
      std::vector<Renderable> renderables;

      MaterialRenderable floor;
      floor.mesh = textured_cube;
      floor.diffuse = greybox_texture;
      floor.model = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 0.2f, 10.0f));
	  floor.has_shadow = true;
      renderables.push_back(floor);

      MaterialRenderable pillar;
      pillar.mesh = textured_cube;
      pillar.diffuse = bluebox_texture;
      pillar.model = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 5.0f, 1.0f));
	  pillar.has_shadow = true;
      renderables.push_back(pillar);

	  for (auto renderable: player.renderables())
		  renderables.push_back(renderable);

      std::vector<Light> lights;

	  DirectionalLight base_light;
	  base_light.ambient = glm::vec3(0.01f);
	  lights.push_back(base_light);

      ShadowCasters shadowcasters;

      DirectionalLight sunlight;
      sunlight.direction = glm::vec3(-0.4f, -1.0f, -0.4f);
      sunlight.diffuse = glm::vec3(0.6f);
      sunlight.specular = glm::vec3(0.0f);
      sunlight.ambient = glm::vec3(0.0f);

      const float ortho_size = 15.0f;
      const float near_plane = 0.1f;
      const float far_plane = ortho_size * 2;
      const glm::vec3 position = glm::vec3(0.0f, 10.0f, 0.0f);

      DirectionalShadowCaster sunlight_caster{
          OrthographicProjection{glm::ortho(-ortho_size, ortho_size,
                                            -ortho_size, ortho_size, near_plane,
                                            far_plane)},
          sunlight, PositionVector{position}, UpVector{world_up}};

      shadowcasters.directional_caster = sunlight_caster;

      WorldRenderInfo world_info{};
      world_info.camera_position = camera.position();
      world_info.view = camera.view();
      world_info.projection = camera.projection();

      FrameProducer frameGenerator =
          [&](CurrentFrameInfo frameInfo) -> std::optional<Texture2D::Impl *> {
        auto *textureptr = renderer.render(
            &context, texture_cache, mesh_cache,
            frameInfo.current_flight_frame_index, frameInfo.total_frame_count,
            world_info, renderables, lights, shadowcasters);

        if (textureptr == nullptr)
          return std::nullopt;
        return textureptr;
      };

      auto render_time = with_time_measurement(
          [&]() { presenter.with_presentation(frameGenerator); });

      framecount++;
    });

    delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                     duration_delta_time)
                     .count();

    total_time += delta_time;
  }

  context.wait_until_idle();
  logfile.close();
  return 0;
}
