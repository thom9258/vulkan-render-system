#pragma once

#include <VulkanRenderer/Animator.hpp>
#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/MeshCache.hpp>
#include <VulkanRenderer/ModelLoader.hpp>
#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/TextureSamplerCache.hpp>
#include <VulkanRenderer/VertexBuffer.hpp>
#include <VulkanRenderer/glm.hpp>
#include <glm/geometric.hpp>

#include "Camera.hpp"
#include "generate_textured_cube.hpp"
#include "interpolation.hpp"

#include <deque>
#include <print>
#include <ranges>

class CameraRig {
public:
  static constexpr glm::mat4 lookat_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 20.0f));

  static constexpr glm::mat4 position_near_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -2.0f));

  static constexpr glm::mat4 position_far_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -5.0f));

  glm::mat4 origin_offset = glm::mat4(1.0f);

  void rotate_camera_center(glm::vec3 rotation) {
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::length(rotation),
                                         glm::normalize(rotation));
    origin_offset = origin_offset * rotation_mat;
  }

  glm::mat4 camera_position(glm::mat4 player_origin, bool is_aiming) {
    if (is_aiming)
      return player_origin * origin_offset * position_near_offset;
    return player_origin * origin_offset * position_far_offset;
  }

  glm::mat4 origin_rotation() {
    return glm::toMat4(glm::toQuat(origin_offset));
  }

  glm::mat4 camera_lookat(glm::mat4 player_origin) {
    return player_origin * origin_offset * lookat_offset;
  }
};


class Player {
public:
  Player(Render::Context &context, MeshCache &mesh_cache,
         TextureSamplerCache &texture_cache)
      : m_transform(Transform::identity()) {

    std::expected<LoadedAnimatedModel, std::string> loaded_model =
        load_animated_model(
            context, mesh_cache, texture_cache,
            "../assets/lowpoly_scifi_girl_2/lowpoly_scifi_girl.gltf");

    if (loaded_model.has_value()) {
      model = loaded_model.value();

      anim_matrix_locations =
          model.animations.at(0).GetFinalBoneMatrixLocations();
      std::println("Animation matrix locations:");
      for (auto [i, name] : std::views::enumerate(anim_matrix_locations)) {
        std::println("({}) {}", i, name);
      }

    } else {
      std::println("Could NOT Load player model, error: {}",
                   loaded_model.error());
    }
  }

  ~Player() = default;

  std::span<Animation> animations() { return model.animations; }
  void set_animation_state(std::span<glm::mat4> animation_state) {
    auto insert_animation_state = [](std::span<glm::mat4> animation_state,
                                     RenderableNodePtr &renderable) {
      for (RenderableNode::Model &model : renderable->models) {
        if (auto *p = std::get_if<RenderableNode::AnimatedModel>(&model)) {
          p->animation_state = animation_state;
        }
      }
    };

    foreach_node(std::bind_front(insert_animation_state, animation_state),
                 model.renderable);
  }

  void set_translation(glm::vec3 translation) {
    m_transform.translation = translation;
  }

  void set_rotation(glm::quat rotation) { m_transform.rotation = rotation; }

  glm::vec3 translation() { return m_transform.translation; }

  glm::mat4 origin() { return m_transform.as_mat4(); }
  glm::mat4 model_matrix() {
    // TODO: get this from controller somehow
    double capsule_collider_height = -1.2f;
    glm::mat4 translation = glm::translate(
        glm::mat4(1.0f), glm::vec3(0.0f, capsule_collider_height, 0.0f));
    glm::mat4 rotation = glm::mat4(1.0f);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.7f));
    glm::mat4 offset = translation * rotation * scale;
    return origin() * offset;
  }

  std::vector<Renderable> renderables() {
    std::vector<Renderable> renderables;

    model.renderable->model_matrix = model_matrix();
    renderables.push_back(model.renderable);

    return renderables;
  }

  // double horizontal_rotate_speed = 0.7f;
  // double vertical_rotate_speed = horizontal_rotate_speed * 0.6;

  bool is_aiming{false};
  glm::mat4 m_camera_current;

  std::vector<std::string> anim_matrix_locations;

private:
  Transform m_transform{Transform::identity()};
  glm::mat4 m_gun_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-0.4f, 0.8f, 0.3f));

  LoadedAnimatedModel model;
};

struct CameraPlayerFollow {

  glm::mat4 current_position{1.0f};
  double step_percentage = 0.8f;

  void operator()(Camera &camera, Player &player, CameraRig &camera_rig,
                  double delta_time) {
    glm::vec3 constexpr up(0.0f, 1.0f, 0.0f);
    glm::vec3 target = camera_rig.camera_lookat(player.origin())[3];

    glm::mat4 ideal_position =
        camera_rig.camera_position(player.origin(), player.is_aiming);
    const auto step = step_percentage * delta_time;
    current_position =
        interpolation::interpolate(current_position, ideal_position, step);

    camera.lookat(current_position[3], target, up);
  }
};
