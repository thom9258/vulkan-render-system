#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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

#include "LoadResources.hpp"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

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

auto rotation_from_direction(glm::vec3 direction) -> glm::mat3 {
  glm::vec3 const rotationZ = direction;
  glm::vec3 const rotationX =
      glm::normalize(glm::cross(glm::vec3(0, 1, 0), rotationZ));
  glm::vec3 const rotationY = glm::normalize(glm::cross(rotationZ, rotationX));
  return glm::mat3(rotationX.x, rotationY.x, rotationZ.x, rotationX.y,
                   rotationY.y, rotationZ.y, rotationX.z, rotationY.z,
                   rotationZ.z);
}

constexpr bool slowframes = false;
constexpr bool printframerate = false;
constexpr size_t printframerateinterval = 100;

std::vector<VertexPosNormColorUV> triangle_vertices = {
    {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0, 0}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0, 0}},
    {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0, 0}},
};

struct Scene {
  std::vector<Renderable> renderables;
  std::vector<Light> lights;
  ShadowCasters shadowcasters;
};

auto parse_vec3(json j) -> glm::vec3 { return {j[0], j[1], j[2]}; }

auto parse_quat(json j) -> glm::quat { return {j[0], j[1], j[2], j[3]}; }

auto parse_transform(json j) -> Render::Transform {
  glm::vec3 const pos = parse_vec3(j["position"]);
  glm::quat const rot = parse_quat(j["rotation-quat-wxyz"]);
  glm::vec3 const scale = parse_vec3(j["scale"]);
  return {pos, rot, scale};
}

auto load_scene_from_path(std::filesystem::path const path,
                          Render::Context &context,
                          TextureSamplerCache &texture_cache,
                          MeshCache &mesh_cache, Resources &resources)
    -> Scene {
  std::ifstream fs(path.string());
  std::string content;
  fs.seekg(0, std::ios::end);
  content.reserve(fs.tellg());
  fs.seekg(0, std::ios::beg);

  content.assign((std::istreambuf_iterator<char>(fs)),
                 std::istreambuf_iterator<char>());

  json j = json::parse(content);

  std::map<std::string, RenderableNodePtr> loaded_assets;

  json assets = j["assets"];
  for (auto &asset : assets) {
    std::string name = asset["name"];
    std::string path = asset["path"];

    RenderableNodePtr loaded_model =
        load_model(context, mesh_cache, texture_cache, path);

    if (loaded_model) {
      loaded_assets[name] = loaded_model;
      std::cout << std::format("Loaded asset {} from path: {}", name, path)
                << std::endl;
    } else {
      std::cout << std::format("Could NOT Load asset {} from path: {}", name,
                               path)
                << std::endl;
    }
  }

  Scene scene;
  json level = j["level"];
  for (auto &prefab : level) {
    std::string name = prefab["name"];
    Render::Transform transform = parse_transform(prefab);

    if (name == "smg") {
      if (prefab["draw-mode"] == "material") {
        MaterialRenderable smg{};
        smg.mesh = resources.smg.textured_mesh;
        if (prefab["has-shadow"] == "yes") {
          smg.has_shadow = true;
        }

        // smg.texture.ambient = &resources.smg.diffuse;
        smg.ambient = std::nullopt;
        smg.diffuse = resources.smg.diffuse;
        smg.specular = resources.smg.specular;
        smg.normal = resources.smg.normal;
        smg.model = transform.as_matrix();
        scene.renderables.push_back(smg);
      } else if (prefab["draw-mode"] == "normcolor") {
        NormColorRenderable smg{};
        smg.mesh = resources.smg.textured_mesh;
        smg.model = transform.as_matrix();
        scene.renderables.push_back(smg);
      } else {
        std::cout << "Unknown draw mode for " << name << std::endl;
      }
    } else if (name == "chest") {
      if (prefab["draw-mode"] == "material") {
        MaterialRenderable chest{};
        chest.mesh = resources.chest.textured_mesh;
        if (prefab["has-shadow"] == "yes") {
          chest.has_shadow = true;
        }

        chest.ambient = resources.chest.diffuse;
        chest.diffuse = resources.chest.diffuse;
        chest.specular = resources.chest.diffuse;
        chest.model = transform.as_matrix();
        scene.renderables.push_back(chest);
      } else if (prefab["draw-mode"] == "normcolor") {
        NormColorRenderable chest{};
        chest.mesh = resources.chest.textured_mesh;
        chest.model = transform.as_matrix();
        scene.renderables.push_back(chest);
      } else if (prefab["draw-mode"] == "wireframe") {
        WireframeRenderable chest{};
        chest.mesh = resources.chest.textured_mesh;
        chest.basecolor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        chest.model = transform.as_matrix();
        scene.renderables.push_back(chest);
      } else {
        std::cout << "Unknown draw mode for " << name << std::endl;
      }
    } else if (name == "transformship") {
      MaterialRenderable ship{};
      ship.mesh = resources.transformship.mesh;
      if (prefab["has-shadow"] == "yes") {
        ship.has_shadow = true;
      }
      ship.ambient = std::nullopt;
      ship.diffuse = resources.transformship.diffuse;
      ship.specular = std::nullopt;
      ship.normal = std::nullopt;
      ship.model = transform.as_matrix();
      scene.renderables.push_back(ship);
    } else if (name == "box") {
      if (prefab["draw-mode"] == "material") {
        MaterialRenderable box{};
        box.mesh = resources.cube.textured_mesh;
        if (prefab["has-shadow"] == "yes") {
          box.has_shadow = true;
        }

        box.ambient = resources.box.diffuse;
        box.diffuse = resources.box.diffuse;
        box.specular = resources.box.specular;
        box.model = transform.as_matrix();
        scene.renderables.push_back(box);
      } else if (prefab["draw-mode"] == "normcolor") {
        NormColorRenderable box{};
        box.mesh = resources.cube.textured_mesh;
        box.model = transform.as_matrix();
        scene.renderables.push_back(box);
      } else if (prefab["draw-mode"] == "wireframe") {
        WireframeRenderable box{};
        box.mesh = resources.cube.textured_mesh;
        box.model = transform.as_matrix();
        scene.renderables.push_back(box);
      } else {
        std::cout << "Unknown draw mode for " << name << std::endl;
      }
    } else if (name == "pixelart") {
      if (prefab["draw-mode"] == "material") {
        MaterialRenderable pixelart{};
        pixelart.mesh = resources.cube.textured_mesh;
        if (prefab["has-shadow"] == "yes") {
          pixelart.has_shadow = true;
        }

        pixelart.diffuse = resources.textures.pixelart;
        pixelart.specular = std::nullopt;
        pixelart.normal = std::nullopt;
        pixelart.model = transform.as_matrix();
        scene.renderables.push_back(pixelart);
      } else {
        std::cout << "Unknown draw mode for " << name << std::endl;
      }
    } else if (name == "floor") {
      if (prefab["draw-mode"] == "material") {
        MaterialRenderable floor{};
        floor.mesh = resources.cube.textured_mesh;
        if (prefab["has-shadow"] == "yes") {
          floor.has_shadow = true;
        }

        floor.diffuse = resources.brickwall.diffuse;
        floor.specular = resources.brickwall.specular;
        floor.normal = resources.brickwall.normal;
        floor.model = transform.as_matrix();
        scene.renderables.push_back(floor);
      } else if (prefab["draw-mode"] == "wireframe") {
        WireframeRenderable floor{};
        floor.mesh = resources.cube.textured_mesh;
        floor.model = transform.as_matrix();
        scene.renderables.push_back(floor);
      } else {
        std::cout << "Unknown draw mode for " << name << std::endl;
      }
    } else {
      auto found = loaded_assets.find(name);
      if (found == loaded_assets.end()) {
        std::cout << "Unknown renderable " << name << std::endl;
        continue;
      }

      RenderableNodePtr renderable = found->second;
      renderable->model_matrix = transform.as_matrix();
      scene.renderables.push_back(renderable);
    }
  }

  json lights = j["lights"];
  for (auto &obj : lights) {
    std::string type = obj["type"];

    if (type == "directional") {
      DirectionalLight p;
      p.direction = glm::normalize(parse_vec3(obj["direction"]));
      p.ambient = parse_vec3(obj["ambient"]);
      p.specular = parse_vec3(obj["specular"]);
      p.diffuse = parse_vec3(obj["diffuse"]);

      const float ortho_size = 50.0f;
      const float near_plane = 0.1f;
      const float far_plane = ortho_size * 2;
      const glm::vec3 position = parse_vec3(obj["position"]);

      DirectionalShadowCaster caster{OrthographicProjection{glm::ortho(
                                         -ortho_size, ortho_size, -ortho_size,
                                         ortho_size, near_plane, far_plane)},
                                     p, PositionVector{position},
                                     UpVector{world_up}};

      if (obj["casts-shadow"] == "yes") {
        scene.shadowcasters.directional_caster = caster;
      } else {
        scene.lights.push_back(p);
      }

      if (obj["draw-gizmo"] == "yes") {
        MaterialRenderable ship{};
        ship.mesh = resources.transformship.mesh;
        ship.has_shadow = false;
        ship.ambient = std::nullopt;
        ship.diffuse = resources.transformship.diffuse;
        ship.specular = std::nullopt;
        ship.normal = std::nullopt;
        ship.model = caster.model();
        scene.renderables.push_back(ship);
      }
    } else if (type == "spot") {
      SpotLight p;
      p.position = parse_vec3(obj["position"]);
      p.direction = glm::normalize(parse_vec3(obj["direction"]));
      p.ambient = parse_vec3(obj["ambient"]);
      p.specular = parse_vec3(obj["specular"]);
      p.diffuse = parse_vec3(obj["diffuse"]);
      p.attenuation.constant = obj["attenuation-constant"];
      p.attenuation.linear = obj["attenuation-linear"];
      p.attenuation.quadratic = obj["attenuation-quadratic"];
      p.cutoff.inner = glm::cos(
          glm::radians(static_cast<float>(obj["cutoff-inner-degrees"])));
      p.cutoff.outer = glm::cos(
          glm::radians(static_cast<float>(obj["cutoff-outer-degrees"])));

      const float aspect = 1;
      const float near_plane = 1.0f, far_plane = 20.0f;

      SpotShadowCaster caster{
          PerspectiveProjection{glm::perspective(glm::radians(70.f), aspect,
                                                 near_plane, far_plane)},
          p, UpVector{world_up}};

      if (obj["casts-shadow"] == "yes") {
        scene.shadowcasters.spot_caster = caster;
      } else {
        scene.lights.push_back(p);
      }

      if (obj["draw-gizmo"] == "yes") {
        MaterialRenderable ship{};
        ship.mesh = resources.transformship.mesh;
        ship.has_shadow = false;
        ship.ambient = std::nullopt;
        ship.diffuse = resources.transformship.diffuse;
        ship.specular = std::nullopt;
        ship.normal = std::nullopt;
        ship.model = caster.model();
        scene.renderables.push_back(ship);
      }
    } else if (type == "point") {
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
        gizmo.mesh = resources.gizmo_sphere.mesh;
        float const scale = p.attenuation.approximate_distance(0.03f);
        gizmo.model = glm::translate(glm::mat4(1.0f), p.position) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        scene.renderables.push_back(gizmo);
        gizmo.model = glm::translate(glm::mat4(1.0f), p.position) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(scale * 0.04f));
        scene.renderables.push_back(gizmo);
      }
    } else {
      std::cout << "Unknown light " << type << std::endl;
    }
  }

  return scene;
}

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cout << std::format("Please provide a path to a scene file..")
              << std::endl;
    return 1;
  }

  std::filesystem::path scene_path{argv[1]};

  if (!std::filesystem::exists(scene_path)) {
    throw std::runtime_error("Scene file does not exist!");
  }
  if (!std::filesystem::is_regular_file(scene_path)) {
    throw std::runtime_error("Scene file is not a file!");
  }

  WindowConfig window_config;

  // RenderConfig render_config;
  // render_config.window_name = "Test Renderer";
  // render_config.window_extent = U32Extent{1200, 800};
  // render_config.render_extent = U32Extent{1200, 800};
  // render_config.shadow_extent = U32Extent{256, 256};

  Logger logger;

  std::ofstream logfile;
  logfile.open("./Engine.log");

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

  struct {
    glm::vec3 position;
    glm::mat3 rotation;

    glm::mat4 view() {
      return glm::translate(glm::inverse(glm::mat4(rotation)), position);
    }
  } camera;

  camera.position = -camera_init_position;
  camera.rotation = glm::mat3(glm::inverse(
      glm::lookAt(camera_init_position, camera_init_target, camera_init_up)));

  WorldRenderInfo world_info{};
  world_info.camera_position = camera.position;
  world_info.view = camera.view();
  world_info.projection =
      glm::perspective(glm::radians(70.f), aspect, 0.1f, 200.0f);
  world_info.projection[1][1] *= -1;

  DescriptorPoolCreateInfo descriptor_pool_info;
  descriptor_pool_info.uniform_buffer_count = 5000;
  descriptor_pool_info.combined_image_sampler_count = 5000;

  DescriptorPool descriptor_pool(descriptor_pool_info, context);

  TextureSamplerCache texture_cache;
  MeshCache mesh_cache;
  Renderer renderer(context, presenter, logger, descriptor_pool, shaders_root);
  Resources resources{context, mesh_cache, texture_cache, assets_root};

  std::cout << "STARTING DRAW LOOP" << std::endl;
  /** ************************************************************************
   * Frame Loop
   */
  SDL_Event event{};
  bool reload_scene = false;
  Scene scene = load_scene_from_path(scene_path, context, texture_cache,
                                     mesh_cache, resources);
  bool exit = false;
  uint64_t framecount = 0;

#if 0
  std::expected<LoadedAnimatedModel, std::string> animated =
      load_animated_model(context, mesh_cache, texture_cache,
                          models_root /
                              "glTF-Sample-Models/2.0/AnimatedMorphCube/glTF/"
                              "AnimatedMorphCube.gltf");

  if (!animated.has_value()) {
	  throw std::runtime_error(animated.error());
  }

#endif 
  
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
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          exit = true;
          break;

          //			case SDLK_n:
          //				scene_index++;
          //				break;
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
        case SDLK_r:
          reload_scene = true;
          break;
        case SDLK_LEFT:
          camera.rotation =
              glm::mat3(glm::rotate(glm::mat4(camera.rotation),
                                    glm::radians(rotate_speed), world_up));
          break;
        case SDLK_RIGHT:
          camera.rotation =
              glm::mat3(glm::rotate(glm::mat4(camera.rotation),
                                    glm::radians(-rotate_speed), world_up));
          break;
        case SDLK_UP:
          camera.rotation =
              glm::mat3(glm::rotate(glm::mat4(camera.rotation),
                                    glm::radians(rotate_speed), world_right));
          break;
        case SDLK_DOWN:
          camera.rotation =
              glm::mat3(glm::rotate(glm::mat4(camera.rotation),
                                    glm::radians(-rotate_speed), world_right));
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

    world_info.camera_position = camera.position;
    world_info.view = camera.view();

    /** ************************************************************************
     * Render Loop
     */
    FrameProducer frameGenerator =
        [&](CurrentFrameInfo frameInfo) -> std::optional<Texture2D::Impl *> {
      if (reload_scene) {
        scene = load_scene_from_path(scene_path, context, texture_cache,
                                     mesh_cache, resources);
        reload_scene = false;
      }

      auto *textureptr = renderer.render(
          texture_cache, mesh_cache, frameInfo.current_flight_frame_index,
          frameInfo.total_frame_count, world_info, scene.renderables,
          scene.lights, scene.shadowcasters);

      if (textureptr == nullptr)
        return std::nullopt;
      return textureptr;
    };

    auto render_time = with_time_measurement(
        [&]() { presenter.with_presentation(frameGenerator); });

    if (slowframes) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    if (printframerate) {
      if (framecount % printframerateinterval == 0) {
        const auto frame_time_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(render_time);

        std::cout << "Frame Time [ms]: " << frame_time_ms.count() << "\n"
                  << "Frame Count:     " << framecount << "\n"
                  << "=====================================" << std::endl;
      }
    }

    framecount++;
  }

  context.wait_until_idle();
  logfile.close();
  return 0;
}
