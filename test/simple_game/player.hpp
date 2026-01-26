#pragma once

#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/MeshCache.hpp>
#include <VulkanRenderer/ModelLoader.hpp>
#include <VulkanRenderer/Renderable.hpp>
#include <VulkanRenderer/TextureSamplerCache.hpp>
#include <VulkanRenderer/VertexBuffer.hpp>
#include <glm/geometric.hpp>

#include "generate_textured_cube.hpp"

#include <deque>
#include <print>

class Camera {
public:
  Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up,
         glm::mat4 projection) {
    lookat(position, target, up);
    m_projection = projection;
  }

  glm::mat4 view() {
    return glm::translate(glm::inverse(glm::mat4(m_rotation)), m_position);
  }
  glm::mat4 projection() { return m_projection; }
  glm::vec3 position() { return -m_position; }

  void lookat(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
    m_position = -position;
    m_rotation = glm::mat3(glm::inverse(glm::lookAt(position, target, up)));
  }

private:
  glm::vec3 m_position;
  glm::mat3 m_rotation;
  glm::mat4 m_projection;
};

class Player {
public:
  Player(Render::Context &context, MeshCache &mesh_cache,
         TextureSamplerCache &texture_cache)
      : m_transform(glm::mat4(1.0f)) {

    m_camera_center_offset = glm::mat4(1.0f);
    m_gun_offset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 1.0f, 0.0f));

    m_camera_position_offset =
        glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -3.0f));

    m_camera_lookat_offset =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 20.0f));

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

  void rotate(glm::vec3 rotation) {
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::length(rotation),
                                         glm::normalize(rotation));
    m_transform = m_transform * rotation_mat;
  }

  void rotate_camera_center(glm::vec3 rotation) {
    glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::length(rotation),
                                         glm::normalize(rotation));
    m_camera_center_offset = m_camera_center_offset * rotation_mat;
  }

  glm::mat4 camera_offset() {
    return m_transform * m_camera_center_offset * m_camera_position_offset;
  }

  glm::mat4 camera_lookat_offset() {
    return m_transform * m_camera_center_offset * m_camera_lookat_offset;
  }

  std::vector<Renderable> renderables() {
    std::vector<Renderable> renderables;
    MaterialRenderable player;
    player.model = glm::scale(m_transform, glm::vec3(0.6f, 2.0f, 0.6f));
    player.mesh = mesh;
    player.diffuse = diffuse;
    player.has_shadow = true;
    renderables.push_back(player);

    if (is_aiming) {
		glm::mat4 gun_scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.6f));
		glm::mat4 gun_transform = m_transform * m_camera_center_offset * m_gun_offset;
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

  static constexpr glm::mat4 m_camera_position_near_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -1.8f));
  static constexpr glm::mat4 m_camera_position_far_offset =
      glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, -3.0f));

  glm::mat4 m_camera_position_offset;

private:
  glm::mat4 m_transform;
  glm::mat4 m_gun_offset;
  glm::mat4 m_camera_center_offset;
  glm::mat4 m_camera_lookat_offset;
  std::optional<SimpleMeshRef> mesh;
  std::optional<TextureSamplerRef> diffuse;
  std::optional<TextureSamplerRef> gun_diffuse;
};

void camera_follow_player(Camera &camera, Player &player) {
  glm::vec3 constexpr up(0.0f, 1.0f, 0.0f);
  glm::vec3 position = player.camera_offset()[3];
  glm::vec3 target = player.camera_lookat_offset()[3];
  camera.lookat(position, target, up);
}
