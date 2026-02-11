#pragma once

#include "config_parser.hpp"
#include "input.hpp"
#include "interpolation.hpp"
#include "player.hpp"
#include <SDL_events.h>
#include <SDL_gamecontroller.h>

#include <algorithm>
#include <ostream>
#include <ranges>

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

  glm::vec3 last_player_translation{0.0f};

  double max_player_walk_speed = 0.7;
  double max_player_run_speed = 1.4;

  std::size_t backwalk_animation = 0;
  std::size_t idle_animation = 0;
  std::size_t leftstrafe_animation = 0;
  std::size_t rightstrafe_animation = 0;
  std::size_t walk_animation = 0;

  PlayerController() {
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

    } else {
      std::println("Config was not readable: {}", config.error());
    }
  }

  void operator()(Player &player, CameraRig &camera_rig, double delta_time,
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

    {

      glm::vec3 player_translation(-joystick_left_x.value(), 0.0f,
                                   -joystick_left_y.value());

      if (glm::length(player_translation) < 0.05f) {
        player_translation = glm::vec3(0.0f);
      } else {

        if (joystick_l2.value() > 0.5f) {
          player_translation *= max_player_run_speed;
        } else {
          player_translation *= max_player_walk_speed;
        }
      }

      if (glm::length(player_translation) > 0.01f) {

        player.translate(player_translation *
                         glm::vec3(player.move_speed * delta_time));
      }

     //if (last_player_translation == glm::vec3(0.0f) &&
     //    player_translation != glm::vec3(0.0f)) {
      if (player_translation != glm::vec3(0.0f)) {

        double x_length = glm::length(player_translation.x);
        double z_length = glm::length(player_translation.z);
        if (x_length < z_length) {
          if (player_translation.z > 0.0f) {
            player.play_animation(walk_animation);
          } else {
            player.play_animation(backwalk_animation);
          }
        } else {
          if (player_translation.x > 0.0f) {
            player.play_animation(leftstrafe_animation);
          } else {
            player.play_animation(rightstrafe_animation);
          }
        }

      } else if (last_player_translation != glm::vec3(0.0f) &&
                 player_translation == glm::vec3(0.0f)) {
        player.play_animation(idle_animation);
      }

      last_player_translation = player_translation;
    }

    {
      glm::vec3 player_rotation(0.0f, -joystick_right_x.value(), 0.0f);
      if (glm::length(player_rotation) > 0.01f) {
        player.rotate(player_rotation *
                      glm::vec3(player.horizontal_rotate_speed * delta_time));
      }
    }

    {
      glm::vec3 camera_center_offset_rotation(-joystick_right_y.value(), 0.0f,
                                              0.0f);
      if (glm::length(camera_center_offset_rotation) > 0.01f) {
        camera_rig.rotate_camera_center(
            camera_center_offset_rotation *
            glm::vec3(player.vertical_rotate_speed * delta_time));
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
