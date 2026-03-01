#pragma once

#include "player.hpp"
#include "input.hpp"

struct MovePlayerPC {

  RawInputKey key_w;
  RawInputKey key_a;
  RawInputKey key_s;
  RawInputKey key_d;

  void operator()(Player &player, double delta_time, std::span<SDL_Event> events) {
    glm::vec3 constexpr forward(0.0f, 0.0f, 1.0f);
    glm::vec3 constexpr right(1.0f, 0.0f, 0.0f);

    glm::vec3 raw_translation(0.0f);
    for (SDL_Event event : events) {
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_w:
          key_w.pressed_down();
          break;
        case SDLK_s:
          key_s.pressed_down();
          break;
        case SDLK_a:
          key_a.pressed_down();
          break;
        case SDLK_d:
          key_d.pressed_down();
          break;
        }
      }

      if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_w:
          key_w.pressed_up();
          break;
        case SDLK_s:
          key_s.pressed_up();
          break;
        case SDLK_a:
          key_a.pressed_up();
          break;
        case SDLK_d:
          key_d.pressed_up();
          break;
        }
      }
    }

    if (key_w.is_down())
      raw_translation += forward;
    if (key_s.is_down())
      raw_translation -= forward;
    if (key_a.is_down())
      raw_translation += right;
    if (key_d.is_down())
      raw_translation -= right;

    if (glm::length(raw_translation) < 0.01f)
      return;

    glm::vec3 translation =
        glm::normalize(raw_translation) * glm::vec3(player.move_speed * delta_time);

    player.translate(translation);
  }
};
