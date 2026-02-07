#pragma once

#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/MeshCache.hpp>
#include <VulkanRenderer/ModelLoader.hpp>
#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/TextureSamplerCache.hpp>
#include <VulkanRenderer/VertexBuffer.hpp>
#include <glm/geometric.hpp>

#include "Camera.hpp"
#include "generate_textured_cube.hpp"
#include "interpolation.hpp"

#include <deque>
#include <print>

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
      : m_transform(glm::mat4(1.0f)) {

    std::expected<LoadedAnimatedModel, std::string> loaded_model =
        load_animated_model(
            context, mesh_cache, texture_cache,
            "../assets/lowpoly_scifi_girl/lowpoly_scifi_girl.gltf");

    if (loaded_model.has_value()) {
      model = loaded_model.value();
    } else {
      std::println("Could NOT Load player model, error: {}",
                   loaded_model.error());
    }

    auto insert_animator = [](Animator *animator,
                              RenderableNodePtr &renderable) {
      for (RenderableNode::Model &model : renderable->models) {
        if (auto *p = std::get_if<RenderableNode::AnimatedModel>(&model)) {
          p->animator = animator;
        }
      }
    };

    foreach_node(std::bind_front(insert_animator, &animator), model.renderable);
    animator.PlayAnimation(&model.animations.at(0));
  }

  ~Player() = default;

  void translate(glm::vec3 offset) {
    m_transform = glm::translate(m_transform, offset);
  }

  void play_walk_animation() {
    if (m_current_animation != m_walk_animation) {
      animator.PlayAnimation(&model.animations.at(m_walk_animation));
	  m_current_animation = m_walk_animation;
    }
  }

  void play_idle_animation() {
    if (m_current_animation != m_idle_animation) {
      animator.PlayAnimation(&model.animations.at(m_idle_animation));
	  m_current_animation = m_idle_animation;
    }
  }

  glm::vec3 translation() { return m_transform[3]; }

  glm::mat4 origin() { return m_transform; }

  void rotate(glm::vec3 rotation) {
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::length(rotation),
                                         glm::normalize(rotation));
    m_transform = m_transform * rotation_mat;
  }

  void update(double delta_time) {
    animator.UpdateAnimation(delta_time);
  }

  std::vector<Renderable> renderables(CameraRig &camera_rig) {
    std::vector<Renderable> renderables;

    model.renderable->model_matrix =
        glm::scale(glm::translate(m_transform, glm::vec3(0.0f, -1.0f, 0.0f)),
                   glm::vec3(0.7f));
    renderables.push_back(model.renderable);

    return renderables;
  }
  double move_speed = 0.5f;
  double horizontal_rotate_speed = 0.7f;
  double vertical_rotate_speed = horizontal_rotate_speed * 0.6;

  bool is_aiming{false};

  size_t const m_idle_animation = 0;
  size_t const m_walk_animation = 1;
  size_t m_current_animation = m_idle_animation;
  glm::mat4 m_camera_current;

private:
  glm::mat4 m_transform;
  glm::mat4 m_gun_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-0.4f, 0.8f, 0.3f));

  LoadedAnimatedModel model;
  Animator animator;
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
