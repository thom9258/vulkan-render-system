#pragma once

#include "Physics.hpp"
#include "config_parser.hpp"
#include "input.hpp"
#include "interpolation.hpp"
#include "player.hpp"
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btVector3.h>
#include <SDL_events.h>
#include <SDL_gamecontroller.h>

#include <VulkanRenderer/Animation2.hpp>
#include <VulkanRenderer/glm.hpp>
#include <algorithm>
#include <ostream>
#include <print>
#include <ranges>

constexpr auto try_find_controller() -> SDL_GameController * {
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      return SDL_GameControllerOpen(i);
    }
  }

  return nullptr;
}

struct Bias2D {
  animation::Bias x;
  animation::Bias y;
};

constexpr auto direction_percentage(glm::vec2 v) -> Bias2D {
  double const abs_sum = std::abs(v.x) + std::abs(v.y);
  return Bias2D{animation::Bias(std::abs(v.x) / abs_sum),
                animation::Bias(std::abs(v.y) / abs_sum)};
}

constexpr auto print_skeleton_with_boneids(animation::BoneInfos &bone_infos,
                                           animation::Skeleton &skeleton,
                                           int depth) -> void {
  std::string spacer = " ";
  for (int i = 0; i < depth; i++) {
    spacer += " ";
  }

  auto id = bone_infos.find_bone_id(skeleton.name());
  if (id.has_value()) {
    std::println("{}{} [id:{}] {}", spacer, skeleton.name(), id.value(),
                 glm::to_string(skeleton.model_matrix()));
  } else {
    std::println("{}{} [id:?] {}", spacer, skeleton.name(),
                 glm::to_string(skeleton.model_matrix()));
  }

  for (animation::Skeleton &child : skeleton.children()) {
    print_skeleton_with_boneids(bone_infos, child, depth + 1);
  }
};

struct PlayerControllerData {
  SDL_GameController *controller{nullptr};

  ControllerButton button_x{SDL_CONTROLLER_BUTTON_X};
  ControllerButton button_a{SDL_CONTROLLER_BUTTON_A};
  ControllerButton button_b{SDL_CONTROLLER_BUTTON_B};
  ControllerButton button_y{SDL_CONTROLLER_BUTTON_Y};

  ControllerButton button_l1{SDL_CONTROLLER_BUTTON_LEFTSHOULDER};
  ControllerButton button_r1{SDL_CONTROLLER_BUTTON_RIGHTSHOULDER};
  ControllerButton left_stick{SDL_CONTROLLER_BUTTON_LEFTSTICK};
  ControllerButton right_stick{SDL_CONTROLLER_BUTTON_RIGHTSTICK};

  ControllerJoystickAxis joystick_left_x{0, 0};
  ControllerJoystickAxis joystick_left_y{0, 1};
  ControllerJoystickAxis joystick_right_x{0, 3};
  ControllerJoystickAxis joystick_right_y{0, 4};

  ControllerJoystickAxis joystick_l2{0, 2};
  ControllerJoystickAxis joystick_r2{0, 5};

  double max_player_walk_speed = 0.7;
  double max_player_run_speed = 1.4;
  double player_rotate_speed = 1.4;
  double camera_rotate_speed = 1.4;

  double player_model_scale = 1;
  double player_model_yoffset = 1;
  float ground_pushback_factor = 5.0;
  float gravity = -8.0f;

  double idle_animation_time = 0.0;
  double walk_animation_time = 0.0;

  double full_idle_to_walk_transition_time = 0.5;
  std::optional<double> idle_to_walk_transition_time;

  std::size_t backwalk_animation = 0;
  std::size_t idle_animation = 0;
  std::size_t leftstrafe_animation = 0;
  std::size_t rightstrafe_animation = 0;
  std::size_t walk_animation = 0;

  animation::FinalAnimationState final_animation_state{100};

  std::unique_ptr<btCapsuleShape> capsule_collider;
  std::unique_ptr<btRigidBody> rigidbody;

  glm::vec3 last_joystick_translation;

  glm::vec3 spawn_position{glm::vec3(-2.0f, 5.0f, 0.0f)};

  static constexpr float const player_height = 1.6f;
  static constexpr float const collider_height = player_height * 0.5f;
  static constexpr float const ray_height = player_height - collider_height;

  std::optional<glm::vec2> move_direction;

  struct GroundInfo {
    double distance = 0.0;
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
  };
  std::optional<GroundInfo> ground_info;

  float y_velocity = 0;

  auto get_position() const -> glm::vec3 {
    btVector3 pos = rigidbody->getCenterOfMassPosition();
    return glm::vec3(pos.x(), pos.y(), pos.z());
  }
};

namespace playersystems {

constexpr auto init_with_config(PlayerControllerData &player,
                                std::filesystem::path config_path) -> void {
  auto config = config::parse_config(config_path);
  if (config.has_value()) {
    player.max_player_walk_speed =
        config::assoc("walk_speed", config.value().f32s).value_or(0.7f);
    player.max_player_run_speed =
        config::assoc("run_speed", config.value().f32s).value_or(1.4f);
    player.player_rotate_speed =
        config::assoc("player_rotate_speed", config.value().f32s)
            .value_or(1.4f);
    player.camera_rotate_speed =
        config::assoc("camera_rotate_speed", config.value().f32s)
            .value_or(1.4f);

    player.player_model_scale =
        config::assoc("player_model_scale", config.value().f32s).value_or(1.4f);
    player.player_model_yoffset =
        config::assoc("player_model_yoffset", config.value().f32s)
            .value_or(1.4f);

    player.backwalk_animation =
        config::assoc("backwalk_animation", config.value().i32s).value_or(0);
    player.walk_animation =
        config::assoc("walk_animation", config.value().i32s).value_or(0);
    player.idle_animation =
        config::assoc("idle_animation", config.value().i32s).value_or(0);
    player.leftstrafe_animation =
        config::assoc("leftstrafe_animation", config.value().i32s).value_or(0);
    player.rightstrafe_animation =
        config::assoc("rightstrafe_animation", config.value().i32s).value_or(0);

    player.ground_pushback_factor =
        config::assoc("ground_pushback_factor", config.value().f32s)
            .value_or(5.0f);
    player.gravity =
        config::assoc("gravity", config.value().f32s).value_or(-8.0f);
  }
}

constexpr auto init_colliders(PlayerControllerData &player_controller,
                              Player &player, physics::Physics &physics)
    -> void {

  player_controller.capsule_collider = std::make_unique<btCapsuleShape>(
      btScalar(0.5f), btScalar(player_controller.player_height / 2));

  btTransform startTransform;
  startTransform.setIdentity();
  startTransform.setOrigin(btVector3(player_controller.spawn_position.x,
                                     player_controller.spawn_position.y,
                                     player_controller.spawn_position.z));
  btScalar mass(1.f);
  btVector3 localInertia(0, 0, 0);
  player_controller.capsule_collider->calculateLocalInertia(mass, localInertia);

  // TODO: does these allocations leak?
  btDefaultMotionState *myMotionState =
      new btDefaultMotionState(startTransform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(
      mass, myMotionState, player_controller.capsule_collider.get(),
      localInertia);

  player_controller.rigidbody = std::make_unique<btRigidBody>(rbInfo);
  player_controller.rigidbody->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
  player_controller.rigidbody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
  player_controller.rigidbody->setActivationState(DISABLE_DEACTIVATION);
  physics.dynamicsWorld->addRigidBody(player_controller.rigidbody.get());

  btVector3 rigidbody_position =
      player_controller.rigidbody->getWorldTransform().getOrigin();
  player.set_translation(glm::vec3(rigidbody_position.x(),
                                   (rigidbody_position.y()),
                                   rigidbody_position.z()));
}

constexpr auto controller_input(PlayerControllerData &player,
                                std::span<SDL_Event> events) -> void {
  if (player.controller == nullptr) {
    player.controller = try_find_controller();
    if (!player.controller) {
      std::println("No controller could be found!");
    } else {
      std::println("Controller is connected!");
    }
  }

  for (SDL_Event event : events) {
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
      std::println("Controller device is added!");
      if (player.controller == nullptr) {
        player.controller = SDL_GameControllerOpen(event.cdevice.which);
        if (!player.controller) {
          std::println(
              "Controller device could not be found after it was added!");
        }
      }
    }
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
      if (player.controller &&
          event.cdevice.which ==
              SDL_JoystickInstanceID(
                  SDL_GameControllerGetJoystick(player.controller))) {
        std::println("Controller device is removed!");
        SDL_GameControllerClose(player.controller);
        player.controller = try_find_controller();
      }
    }
  }

  SDL_JoystickID joystick_id =
      SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(player.controller));

  auto has_joystick_id = [](SDL_JoystickID id, SDL_Event &event) -> bool {
    return event.cdevice.which == id;
  };

  std::vector<SDL_Event> joystick_events =
      events |
      std::ranges::views::filter(
          std::bind_front(has_joystick_id, joystick_id)) |
      std::ranges::to<std::vector>();

  player.joystick_left_x.update(joystick_events);
  player.joystick_left_y.update(joystick_events);
  player.joystick_right_x.update(joystick_events);
  player.joystick_right_y.update(joystick_events);
  player.button_x.update(joystick_events);
  player.button_a.update(joystick_events);
  player.button_b.update(joystick_events);
  player.button_y.update(joystick_events);
  player.left_stick.update(joystick_events);
  player.right_stick.update(joystick_events);
  player.button_l1.update(joystick_events);
  player.button_r1.update(joystick_events);
  player.joystick_l2.update(joystick_events);
  player.joystick_r2.update(joystick_events);
}

constexpr auto respawn(PlayerControllerData &player) -> void {

  if (player.get_position().y < -10.0f) {

    player.rigidbody->setLinearVelocity(btVector3(0, 0, 0));
    player.rigidbody->setAngularVelocity(btVector3(0, 0, 0));
    btTransform transform = btTransform::getIdentity();

    btVector3 spawn_position(player.spawn_position.x, player.spawn_position.y,
                             player.spawn_position.z);

    transform.setOrigin(spawn_position);
    player.rigidbody->setCenterOfMassTransform(transform);
  }
}

constexpr auto ground_detect(PlayerControllerData &player,
                             physics::Physics &physics) -> void {
  // https://github.com/bulletphysics/bullet3/blob/master/examples/Raycast/RaytestDemo.cpp
  btVector3 collider_center = player.rigidbody->getCenterOfMassPosition();
  btVector3 from = collider_center - btVector3(0, player.collider_height, 0);
  btVector3 to =
      from - btVector3(0, player.collider_height + player.ray_height, 0);
  btVector3 const red(1, 0, 0);
  btVector3 const blue(0, 0, 1);
  physics.dynamicsWorld->getDebugDrawer()->drawLine(from, to,
                                                    btVector4(0, 1, 0, 1));

  btCollisionWorld::ClosestRayResultCallback closestResults(from, to);
  physics.dynamicsWorld->rayTest(from, to, closestResults);

  if (closestResults.hasHit()) {
    btVector3 p = closestResults.m_hitPointWorld;
    physics.dynamicsWorld->getDebugDrawer()->drawLine(
        p, p + closestResults.m_hitNormalWorld, blue);

    PlayerControllerData::GroundInfo ground_info;
    ground_info.distance = closestResults.m_closestHitFraction;
    ground_info.normal = glm::vec3(closestResults.m_hitNormalWorld.x(),
                                   closestResults.m_hitNormalWorld.y(),
                                   closestResults.m_hitNormalWorld.z());

    player.ground_info = ground_info;
  } else {
    player.ground_info = std::nullopt;
  }
}

constexpr auto determine_move_direction(PlayerControllerData &player) -> void {
  glm::vec2 joystick_translation(-player.joystick_left_x.value(),
                                 -player.joystick_left_y.value());

  if (glm::length(joystick_translation) < 0.05f) {
    player.move_direction = std::nullopt;
  } else {
    player.move_direction = joystick_translation;
    //std::println("Move dir: {}", glm::to_string(player.move_direction.value()));
  }
}

constexpr auto rotate(glm::quat quat, glm::vec3 vec) -> glm::vec3 {
  glm::mat4 rotation = glm::toMat4(quat);

  glm::vec4 rotated_vec = rotation * glm::vec4(vec, 1.0f);
  return glm::vec3(rotated_vec);
}

constexpr auto apply_translation(PlayerControllerData &player,
                                 Player &playermodel, physics::Physics &physics,
                                 double deltatime) -> void {

  deltatime *= 3;

  float const y_velocity = std::invoke([&]() {
    if (player.ground_info.has_value()) {
      float const ground_distance_to_decired =
          player.ray_height - player.ground_info.value().distance;
      return ground_distance_to_decired * player.ground_pushback_factor;
    }
    return player.gravity;
  });

  if (player.move_direction.has_value()) {
    glm::vec3 translation_direction =
        glm::normalize(glm::vec3(player.move_direction.value().x, 0.0f, player.move_direction.value().y)) *
        glm::vec3(player.max_player_walk_speed * deltatime);

    btQuaternion bt_player_rotation = player.rigidbody->getOrientation();
    glm::quat player_rotation(bt_player_rotation.w(), bt_player_rotation.x(),
                              bt_player_rotation.y(), bt_player_rotation.z());

    glm::vec3 translation = rotate(player_rotation, translation_direction);
    btVector3 bt_translation(translation.x, y_velocity, translation.z);

    player.rigidbody->setLinearVelocity(bt_translation);

    btVector3 collider_center = player.rigidbody->getCenterOfMassPosition();
    btVector3 from = collider_center;
    btVector3 to = from + bt_translation * 0.5f;
    physics.dynamicsWorld->getDebugDrawer()->drawLine(from, to,
                                                      btVector4(0, 1, 0, 1));

  } else {

    player.rigidbody->setLinearVelocity(btVector3(0, y_velocity, 0));
  }

  btTransform rigidbody_transform = player.rigidbody->getWorldTransform();
  glm::vec3 player_position(rigidbody_transform.getOrigin().x(),
                            rigidbody_transform.getOrigin().y() -
                                player.collider_height,
                            rigidbody_transform.getOrigin().z());

  playermodel.set_translation(player_position);
}

constexpr auto apply_rotation(PlayerControllerData &player, Player &playermodel,
                              physics::Physics &physics, double deltatime)
    -> void {
  double joystick_rotation = -player.joystick_right_x.value();
  if (glm::length(joystick_rotation) > 0.01f) {
    double player_rotation_velocity =
        joystick_rotation * player.player_rotate_speed * deltatime;

    player.rigidbody->setAngularVelocity(
        btVector3(0.0f, player_rotation_velocity, 0.0f));

  } else {
    player.rigidbody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
  }

  btTransform rigidbody_transform = player.rigidbody->getWorldTransform();
  glm::quat player_rotation(rigidbody_transform.getRotation().w(),
                            rigidbody_transform.getRotation().x(),
                            rigidbody_transform.getRotation().y(),
                            rigidbody_transform.getRotation().z());

  playermodel.set_rotation(player_rotation);
}

constexpr auto apply_camera_rotation(PlayerControllerData &player,
                                     CameraRig &camera_rig, double deltatime)
    -> void {
  glm::vec3 camera_center_offset_rotation(-player.joystick_right_y.value(),
                                          0.0f, 0.0f);

  if (glm::length(camera_center_offset_rotation) > 0.01f) {
    camera_rig.rotate_camera_center(
        camera_center_offset_rotation *
        glm::vec3(player.camera_rotate_speed * deltatime));
  }
}

constexpr auto calculate_animation(PlayerControllerData &player,
                                   Player &playermodel, double deltatime)
    -> void {

  deltatime *= 0.1;

  animation::Animation &idle_anim =
      playermodel.animations()[player.idle_animation];
  animation::Animation &front_anim =
      playermodel.animations()[player.walk_animation];
  animation::Animation &back_anim =
      playermodel.animations()[player.backwalk_animation];
  animation::Animation &left_anim =
      playermodel.animations()[player.leftstrafe_animation];
  animation::Animation &right_anim =
      playermodel.animations()[player.rightstrafe_animation];

  if (!player.move_direction.has_value()) {
    player.idle_animation_time += deltatime;
    player.walk_animation_time = 0;

    std::optional<animation::Skeleton> idle_skeleton =
        idle_anim.animate(player.idle_animation_time);

    if (idle_skeleton.has_value()) {
      auto state = calculate_final_animation_state(idle_anim.bone_infos(),
                                                   idle_skeleton.value());

      if (state.has_value()) {
        player.final_animation_state = std::move(state.value());
      }
    }
  } else {
    player.walk_animation_time += deltatime;
    player.idle_animation_time = 0;

    Bias2D direction_bias = direction_percentage(player.move_direction.value());

    glm::vec2 norm_direction = glm::normalize(player.move_direction.value());

    if (norm_direction.x >= 0 && norm_direction.y >= 0) {

      std::optional<animation::Skeleton> front_skeleton =
          front_anim.animate(player.walk_animation_time);
      std::optional<animation::Skeleton> left_skeleton =
          left_anim.animate(player.walk_animation_time);

      if (front_skeleton.has_value() && left_skeleton.has_value()) {
        std::optional<animation::Skeleton> mix = animation::blend_skeletons(
            front_skeleton.value(), left_skeleton.value(), direction_bias.x);

        if (mix.has_value()) {
          auto state = calculate_final_animation_state(idle_anim.bone_infos(),
                                                       mix.value());
          if (state.has_value()) {
            player.final_animation_state = std::move(state.value());
          }
        }
      }

    } else if (norm_direction.x >= 0 && norm_direction.y <= 0) {

      std::optional<animation::Skeleton> back_skeleton =
          back_anim.animate(player.walk_animation_time);
      std::optional<animation::Skeleton> left_skeleton =
          left_anim.animate(player.walk_animation_time);

      if (back_skeleton.has_value() && left_skeleton.has_value()) {
        std::optional<animation::Skeleton> mix = animation::blend_skeletons(
            back_skeleton.value(), left_skeleton.value(), direction_bias.x);

        if (mix.has_value()) {
          auto state = calculate_final_animation_state(idle_anim.bone_infos(),
                                                       mix.value());
          if (state.has_value()) {
            player.final_animation_state = std::move(state.value());
          }
        }
      }

    } else if (norm_direction.x <= 0 && norm_direction.y >= 0) {

      std::optional<animation::Skeleton> forward_skeleton =
          front_anim.animate(player.walk_animation_time);
      std::optional<animation::Skeleton> right_skeleton =
          right_anim.animate(player.walk_animation_time);

      if (forward_skeleton.has_value() && right_skeleton.has_value()) {
        std::optional<animation::Skeleton> mix = animation::blend_skeletons(
            forward_skeleton.value(), right_skeleton.value(), direction_bias.x);

        if (mix.has_value()) {
          auto state = calculate_final_animation_state(idle_anim.bone_infos(),
                                                       mix.value());
          if (state.has_value()) {
            player.final_animation_state = std::move(state.value());
          }
        }
      }
    } else if (norm_direction.x <= 0 && norm_direction.y <= 0) {
      std::optional<animation::Skeleton> back_skeleton =
          back_anim.animate(player.walk_animation_time);
      std::optional<animation::Skeleton> right_skeleton =
          right_anim.animate(player.walk_animation_time);

      if (back_skeleton.has_value() && right_skeleton.has_value()) {
        std::optional<animation::Skeleton> mix = animation::blend_skeletons(
            back_skeleton.value(), right_skeleton.value(), direction_bias.x);

        if (mix.has_value()) {
          auto state = calculate_final_animation_state(idle_anim.bone_infos(),
                                                       mix.value());
          if (state.has_value()) {
            player.final_animation_state = std::move(state.value());
          }
        }
      }
    }
  }

  playermodel.set_animation_state(player.final_animation_state.matrices());
}

} // namespace playersystems

class PlayerController {
public:
  PlayerController(std::filesystem::path config_path, physics::Physics &physics,
                   Player &playermodel) {
    playersystems::init_with_config(data, config_path);
    playersystems::init_colliders(data, playermodel, physics);
  }

  void update(Player &playermodel, CameraRig &camera_rig,
              physics::Physics &physics, double deltatime,
              std::span<SDL_Event> events) {

    playersystems::controller_input(data, events);
    playersystems::respawn(data);
    playersystems::ground_detect(data, physics);
    playersystems::determine_move_direction(data);
    playersystems::apply_translation(data, playermodel, physics, deltatime);
    playersystems::apply_rotation(data, playermodel, physics, deltatime);
    playersystems::apply_camera_rotation(data, camera_rig, deltatime);
    playersystems::calculate_animation(data, playermodel, deltatime);
  }

private:
  PlayerControllerData data;
};
