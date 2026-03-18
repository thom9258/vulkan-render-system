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
#include <ranges>

void print_skeleton_with_boneids(animation::BoneInfos &bone_infos,
                                 animation::Skeleton &skeleton, int depth) {
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

glm::vec3 rotate(glm::quat quat, glm::vec3 vec) {
  glm::mat4 rotation = glm::toMat4(quat);

  glm::vec4 rotated_vec = rotation * glm::vec4(vec, 1.0f);
  return glm::vec3(rotated_vec);
}

struct Bias2D {
  animation::Bias x;
  animation::Bias y;
};

Bias2D direction_percentage(glm::vec2 v) {
  double const abs_sum = std::abs(v.x) + std::abs(v.y);
  return Bias2D{animation::Bias(std::abs(v.x) / abs_sum),
                animation::Bias(std::abs(v.y) / abs_sum)};
}

struct PlayerController {
  SDL_GameController *m_controller{nullptr};

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

  std::unique_ptr<btCapsuleShape> capsule_collider;
  std::unique_ptr<btRigidBody> rigidbody;

  glm::vec3 last_joystick_translation;

  glm::vec3 spawn_position{glm::vec3(0.0f)};

  static constexpr float const player_height = 1.6f;
  static constexpr float const collider_height = player_height * 0.5f;
  static constexpr float const ray_height = player_height - collider_height;

#if 0
  struct {
    Animator idle;
    struct {
      animation::BlendAnimator forward_right;
      animation::BlendAnimator forward_left;
      animation::BlendAnimator backward_right;
      animation::BlendAnimator backward_left;
    } walk;

  } animators;
#endif

  animation::FinalAnimationState final_animation_state{100};

  auto get_position() const -> glm::vec3 {
    btVector3 pos = rigidbody->getCenterOfMassPosition();
    return glm::vec3(pos.x(), pos.y(), pos.z());
  }

  void respawn() {
    rigidbody->setLinearVelocity(btVector3(0, 0, 0));
    rigidbody->setAngularVelocity(btVector3(0, 0, 0));
    btTransform transform = btTransform::getIdentity();
    transform.setOrigin(
        btVector3(spawn_position.x, spawn_position.y, spawn_position.z));
    rigidbody->setCenterOfMassTransform(transform);
  }

  PlayerController(Player &player, physics::Physics &physics,
                   glm::vec3 position)
      : spawn_position{position} {
    // TODO::This is crucial because for some reason this is not enabled inside
    // SDL_INIT_EVERYTHING
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) != 1)
      SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);

    m_controller = try_find_controller();
    if (!m_controller) {
      std::println("No controller could be found from beginning!");
    } else {
      std::println("Controller is connected from beginning!");
    }

    auto config = config::parse_config("../player.config");
    if (config.has_value()) {
      max_player_walk_speed =
          config::assoc("walk_speed", config.value().f32s).value_or(0.7f);
      max_player_run_speed =
          config::assoc("run_speed", config.value().f32s).value_or(1.4f);
      player_rotate_speed =
          config::assoc("player_rotate_speed", config.value().f32s)
              .value_or(1.4f);
      camera_rotate_speed =
          config::assoc("camera_rotate_speed", config.value().f32s)
              .value_or(1.4f);

      player_model_scale =
          config::assoc("player_model_scale", config.value().f32s)
              .value_or(1.4f);
      player_model_yoffset =
          config::assoc("player_model_yoffset", config.value().f32s)
              .value_or(1.4f);

      backwalk_animation =
          config::assoc("backwalk_animation", config.value().i32s).value_or(0);
      walk_animation =
          config::assoc("walk_animation", config.value().i32s).value_or(0);
      idle_animation =
          config::assoc("idle_animation", config.value().i32s).value_or(0);
      leftstrafe_animation =
          config::assoc("leftstrafe_animation", config.value().i32s)
              .value_or(0);
      rightstrafe_animation =
          config::assoc("rightstrafe_animation", config.value().i32s)
              .value_or(0);

      ground_pushback_factor =
          config::assoc("ground_pushback_factor", config.value().f32s)
              .value_or(5.0f);
      gravity = config::assoc("gravity", config.value().f32s).value_or(-8.0f);

    } else {
      std::println("Config was not readable: {}", config.error());
    }

#if 0    
    animators.idle.PlayAnimation(&player.animations()[idle_animation]);
    player.set_animation_state(animators.idle.GetFinalBoneMatrices());

    animators.walk.forward.PlayAnimation(&player.animations()[walk_animation]);
    animators.walk.back.PlayAnimation(&player.animations()[backwalk_animation]);
    animators.walk.left.PlayAnimation(
        &player.animations()[leftstrafe_animation]);
    animators.walk.right.PlayAnimation(
        &player.animations()[rightstrafe_animation]);

    animators.walk.forward_right =
        animation::BlendAnimator(&player.animations()[walk_animation],
                                 &player.animations()[rightstrafe_animation]);

    animators.walk.forward_left =
        animation::BlendAnimator(&player.animations()[walk_animation],
                                 &player.animations()[leftstrafe_animation]);

    animators.walk.backward_right =
        animation::BlendAnimator(&player.animations()[backwalk_animation],
                                 &player.animations()[rightstrafe_animation]);

    animators.walk.backward_left =
        animation::BlendAnimator(&player.animations()[backwalk_animation],
                                 &player.animations()[leftstrafe_animation]);
#endif

    capsule_collider = std::make_unique<btCapsuleShape>(
        btScalar(0.5f), btScalar(player_height / 2));

    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(btVector3(position.x, position.y, position.z));
    btScalar mass(1.f);
    btVector3 localInertia(0, 0, 0);
    capsule_collider->calculateLocalInertia(mass, localInertia);

    // TODO: does these allocations leak?
    btDefaultMotionState *myMotionState =
        new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        mass, myMotionState, capsule_collider.get(), localInertia);

    rigidbody = std::make_unique<btRigidBody>(rbInfo);
    rigidbody->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
    rigidbody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
    rigidbody->setActivationState(DISABLE_DEACTIVATION);
    physics.dynamicsWorld->addRigidBody(rigidbody.get());

    btVector3 rigidbody_position = rigidbody->getWorldTransform().getOrigin();
    player.set_translation(glm::vec3(rigidbody_position.x(),
                                     (rigidbody_position.y()),
                                     rigidbody_position.z()));
  }

  void operator()(Player &player, CameraRig &camera_rig,
                  physics::Physics &physics, double delta_time,
                  std::span<SDL_Event> events) {

    // TODO: seemingly a controller is found but cant be re-added after it is
    // added again
    maybe_rediscover_controller(events);

    if (!m_controller) {
      m_controller = try_find_controller();
      if (!m_controller) {
        std::println("No Controller!");
        return;
      }
    }

    SDL_JoystickID joystick_id =
        SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(m_controller));

    auto has_joystick_id = [](SDL_JoystickID id, SDL_Event &event) -> bool {
      return event.cdevice.which == id;
    };

    std::vector<SDL_Event> joystick_events =
        events |
        std::ranges::views::filter(
            std::bind_front(has_joystick_id, joystick_id)) |
        std::ranges::to<std::vector>();

    joystick_left_x.update(joystick_events);
    joystick_left_y.update(joystick_events);
    joystick_right_x.update(joystick_events);
    joystick_right_y.update(joystick_events);
    button_x.update(joystick_events);
    button_a.update(joystick_events);
    button_b.update(joystick_events);
    button_y.update(joystick_events);
    left_stick.update(joystick_events);
    right_stick.update(joystick_events);

    button_l1.update(joystick_events);
    button_r1.update(joystick_events);
    joystick_l2.update(joystick_events);
    joystick_r2.update(joystick_events);

    struct GroundInfo {
      double distance = 0.0;
      glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    };
    std::optional<GroundInfo> ground_info;

    {
      // https://github.com/bulletphysics/bullet3/blob/master/examples/Raycast/RaytestDemo.cpp
      btVector3 collider_center = rigidbody->getCenterOfMassPosition();
      btVector3 from = collider_center - btVector3(0, collider_height, 0);
      btVector3 to = from - btVector3(0, collider_height + ray_height, 0);
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
        ground_info.emplace();
        ground_info.value().distance = closestResults.m_closestHitFraction;
        ground_info.value().normal =
            glm::vec3(closestResults.m_hitNormalWorld.x(),
                      closestResults.m_hitNormalWorld.y(),
                      closestResults.m_hitNormalWorld.z());
      }
    }
    {
      glm::vec3 joystick_translation(-joystick_left_x.value(), 0.0f,
                                     -joystick_left_y.value());

      if (glm::length(joystick_translation) < 0.05f) {
        joystick_translation = glm::vec3(0.0f);
      } else {

        if (joystick_l2.value() > 0.5f) {
          joystick_translation *= max_player_run_speed;
        } else {
          joystick_translation *= max_player_walk_speed;
        }
      }

      float y_velocity = 0;
      if (ground_info.has_value()) {
        float const ground_distance_to_decired =
            ray_height - ground_info.value().distance;

        y_velocity = ground_distance_to_decired * ground_pushback_factor;
      } else {
        y_velocity = gravity;
      }

      if (glm::length(joystick_translation) > 0.01f) {

        glm::vec3 translation = joystick_translation *
                                glm::vec3(max_player_walk_speed * delta_time);

        btQuaternion bt_player_rotation = rigidbody->getOrientation();
        glm::quat player_rotation(
            bt_player_rotation.w(), bt_player_rotation.x(),
            bt_player_rotation.y(), bt_player_rotation.z());

        translation = rotate(player_rotation, translation);

        rigidbody->setLinearVelocity(
            btVector3(translation.x, y_velocity, translation.z));

        btVector3 collider_center = rigidbody->getCenterOfMassPosition();
        btVector3 from = collider_center;
        btVector3 to = from + btVector3(translation.x, 0, translation.z) * 0.5f;
        physics.dynamicsWorld->getDebugDrawer()->drawLine(
            from, to, btVector4(0, 1, 0, 1));

      } else {
        rigidbody->setLinearVelocity(btVector3(0, y_velocity, 0));
      }

      double const animation_deltatime = delta_time / 10;
      animation::Animation &idle_anim = player.animations()[idle_animation];
      animation::Animation &front_anim = player.animations()[walk_animation];
      animation::Animation &back_anim = player.animations()[backwalk_animation];
      animation::Animation &left_anim =
          player.animations()[leftstrafe_animation];
      animation::Animation &right_anim =
          player.animations()[rightstrafe_animation];

      if (joystick_translation != glm::vec3(0.0f)) {
        walk_animation_time += animation_deltatime;
        idle_animation_time = 0;

        double x_length = joystick_translation.x;
        double z_length = joystick_translation.z;
        // TODO: we might have to use mix_bias.y for some of these, we can see
        // that once i fixed the stupid animations to be mixable
        Bias2D direction_bias =
            direction_percentage(glm::vec2(x_length, z_length));

        glm::vec2 norm_direction =
            glm::normalize(glm::vec2(x_length, z_length));

        if (norm_direction.x >= 0 && norm_direction.y >= 0) {

          std::optional<animation::Skeleton> front_skeleton =
              front_anim.animate(walk_animation_time);
          std::optional<animation::Skeleton> left_skeleton =
              left_anim.animate(walk_animation_time);

          if (front_skeleton.has_value() && left_skeleton.has_value()) {
            std::optional<animation::Skeleton> mix = animation::blend_skeletons(
                front_skeleton.value(), left_skeleton.value(),
                direction_bias.x);

            if (mix.has_value()) {
              auto state = calculate_final_animation_state(
                  idle_anim.bone_infos(), mix.value());
              if (state.has_value()) {
                final_animation_state = std::move(state.value());
              }
            }
          }

        } else if (norm_direction.x >= 0 && norm_direction.y <= 0) {

          std::optional<animation::Skeleton> back_skeleton =
              back_anim.animate(walk_animation_time);
          std::optional<animation::Skeleton> left_skeleton =
              left_anim.animate(walk_animation_time);

          if (back_skeleton.has_value() && left_skeleton.has_value()) {
            std::optional<animation::Skeleton> mix = animation::blend_skeletons(
                back_skeleton.value(), left_skeleton.value(), direction_bias.x);

            if (mix.has_value()) {
              auto state = calculate_final_animation_state(
                  idle_anim.bone_infos(), mix.value());
              if (state.has_value()) {
                final_animation_state = std::move(state.value());
              }
            }
          }

        } else if (norm_direction.x <= 0 && norm_direction.y >= 0) {

          std::optional<animation::Skeleton> forward_skeleton =
              front_anim.animate(walk_animation_time);
          std::optional<animation::Skeleton> right_skeleton =
              right_anim.animate(walk_animation_time);

          if (forward_skeleton.has_value() && right_skeleton.has_value()) {
            std::optional<animation::Skeleton> mix = animation::blend_skeletons(
                forward_skeleton.value(), right_skeleton.value(),
                direction_bias.x);

            if (mix.has_value()) {
              auto state = calculate_final_animation_state(
                  idle_anim.bone_infos(), mix.value());
              if (state.has_value()) {
                final_animation_state = std::move(state.value());
              }
            }
          }

        } else if (norm_direction.x <= 0 && norm_direction.y <= 0) {
          std::optional<animation::Skeleton> back_skeleton =
              back_anim.animate(walk_animation_time);
          std::optional<animation::Skeleton> right_skeleton =
              right_anim.animate(walk_animation_time);

          if (back_skeleton.has_value() && right_skeleton.has_value()) {
            std::optional<animation::Skeleton> mix = animation::blend_skeletons(
                back_skeleton.value(), right_skeleton.value(),
                direction_bias.x);

            if (mix.has_value()) {
              auto state = calculate_final_animation_state(
                  idle_anim.bone_infos(), mix.value());
              if (state.has_value()) {
                final_animation_state = std::move(state.value());
              }
            }
          }
        }

        player.set_animation_state(final_animation_state.matrices());
      } else {
        idle_animation_time += animation_deltatime;
        walk_animation_time = 0;

        std::optional<animation::Skeleton> idle_skeleton =
            idle_anim.animate(idle_animation_time);

        if (idle_skeleton.has_value()) {
          auto state = calculate_final_animation_state(idle_anim.bone_infos(),
                                                       idle_skeleton.value());

          if (state.has_value()) {
            final_animation_state = std::move(state.value());
          }
        }

        player.set_animation_state(final_animation_state.matrices());
      }

      last_joystick_translation = joystick_translation;
    }

    {
      double joystick_rotation = -joystick_right_x.value();
      if (glm::length(joystick_rotation) > 0.01f) {
        double player_rotation_velocity =
            joystick_rotation * player_rotate_speed * delta_time;

        rigidbody->setAngularVelocity(
            btVector3(0.0f, player_rotation_velocity, 0.0f));

      } else {
        rigidbody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
      }
    }

    {
      glm::vec3 camera_center_offset_rotation(-joystick_right_y.value(), 0.0f,
                                              0.0f);

      if (glm::length(camera_center_offset_rotation) > 0.01f) {
        camera_rig.rotate_camera_center(
            camera_center_offset_rotation *
            glm::vec3(camera_rotate_speed * delta_time));
      }
    }

    {
      if (button_l1.is_down()) {
        player.is_aiming = true;
      } else {
        player.is_aiming = false;
      }

      if (button_l1.is_down() && joystick_r2.value() > 0.5f)
        std::println("Shooting");
    }

    btTransform rigidbody_transform = rigidbody->getWorldTransform();
    glm::vec3 player_position(rigidbody_transform.getOrigin().x(),
                              rigidbody_transform.getOrigin().y() -
                                  collider_height,
                              rigidbody_transform.getOrigin().z());

    glm::quat player_rotation(rigidbody_transform.getRotation().w(),
                              rigidbody_transform.getRotation().x(),
                              rigidbody_transform.getRotation().y(),
                              rigidbody_transform.getRotation().z());

    player.set_translation(player_position);
    player.set_rotation(player_rotation);
  }

private:
  static SDL_GameController *try_find_controller() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
      if (SDL_IsGameController(i)) {
        return SDL_GameControllerOpen(i);
      }
    }
    return nullptr;
  }

  void maybe_rediscover_controller(std::span<SDL_Event> events) {

    for (SDL_Event event : events) {
      if (event.type == SDL_CONTROLLERDEVICEADDED) {
        std::println("Controller device is added!");
        if (m_controller == nullptr) {
          m_controller = SDL_GameControllerOpen(event.cdevice.which);
          if (!m_controller) {
            std::println(
                "Controller device could not be found after it was added!");
          }
        }
      }
      if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (m_controller &&
            event.cdevice.which ==
                SDL_JoystickInstanceID(
                    SDL_GameControllerGetJoystick(m_controller))) {
          std::println("Controller device is removed!");
          SDL_GameControllerClose(m_controller);
          m_controller = try_find_controller();
        }
      }
    }
  }
};
