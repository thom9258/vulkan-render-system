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
  glm::mat4 lookat_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 20.0f));

  glm::mat4 position_near_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -1.8f));

  glm::mat4 position_far_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -3.0f));

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

    mesh = mesh_cache.add(
        context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(
                     context, get_textured_cube_vertices())});

    diffuse = texture_cache.load_from_path(
        &context, "greenbox_texture", InterpolationType::Linear,
        VerticalFlipOnLoad::No, BitmapPixelFormat::RGBA,
        "../assets/GreyboxTextures/greybox_green_grid.png");

    gun_diffuse = texture_cache.load_from_path(
        &context, "redbox_texture", InterpolationType::Linear,
        VerticalFlipOnLoad::No, BitmapPixelFormat::RGBA,
        "../assets/GreyboxTextures/greybox_red_grid.png");
  }
  ~Player() = default;

  void translate(glm::vec3 offset) {
    m_transform = glm::translate(m_transform, offset);
  }

  glm::vec3 translation() { return m_transform[3]; }

  glm::mat4 origin() { return m_transform; }

  void rotate(glm::vec3 rotation) {
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::length(rotation),
                                         glm::normalize(rotation));
    m_transform = m_transform * rotation_mat;
  }

  std::vector<Renderable> renderables(CameraRig &camera_rig) {
    std::vector<Renderable> renderables;
    MaterialRenderable player;
    player.model = glm::scale(m_transform, glm::vec3(0.6f, 2.0f, 0.6f));
    player.mesh = mesh;
    player.diffuse = diffuse;
    player.has_shadow = true;
    renderables.push_back(player);

    if (is_aiming) {
      glm::mat4 gun_scale =
          glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.6f));
      glm::mat4 gun_transform =
          m_transform * m_gun_offset * camera_rig.origin_rotation();
      MaterialRenderable gun;
      gun.model = gun_transform * gun_scale;
      gun.mesh = mesh;
      gun.diffuse = gun_diffuse;
      gun.has_shadow = true;
      renderables.push_back(gun);
    }

    return renderables;
  }

  double move_speed = 0.5f;
  double horizontal_rotate_speed = 0.7f;
  double vertical_rotate_speed = horizontal_rotate_speed * 0.6;

  bool is_aiming{false};

  // glm::mat4 m_camera_position_offset;
  glm::mat4 m_camera_current;

private:
  glm::mat4 m_transform;
  glm::mat4 m_gun_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-0.4f, 0.8f, 0.3f));

  std::optional<SimpleMeshRef> mesh;
  std::optional<TextureSamplerRef> diffuse;
  std::optional<TextureSamplerRef> gun_diffuse;
};

struct CameraPlayerFollow {

  glm::mat4 current_position;
  double step_percentage = 0.7f;

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
