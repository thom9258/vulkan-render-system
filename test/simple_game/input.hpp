#pragma once

#include <SDL_events.h>
#include <SDL_gamecontroller.h>


#include <VulkanRenderer/StrongType.hpp>

#include <memory>
#include <span>

using Deadzone = StrongType<int, struct DeadzoneTag>;

struct RawInputKey {
  void pressed_down() { m_is_down = true; }
  void pressed_up() { m_is_down = false; }

  bool is_down() const { return m_is_down; }

private:
  bool m_is_down = false;
};

template <typename T> struct RawInputState {
  RawInputState(T init) : m_value{init} {};

  void changed(T v) { m_value = v; }
  T value() { return m_value; }

private:
  T m_value;
};

class ControllerJoystickAxis {
public:
  static int constexpr deadzone = 5000;

  ControllerJoystickAxis(SDL_JoystickID id, std::uint8_t axis)
      : m_id{id}, m_axis{axis} {}

  void update(std::span<SDL_Event> events) {
    for (SDL_Event &event : events) {
      if (event.type == SDL_JOYAXISMOTION) {
        if (event.jaxis.which == m_id) {
          const auto axis_value = static_cast<int>(event.jaxis.value);
          const bool inside_deadzone =
              axis_value < deadzone && axis_value > -deadzone;

          if (event.jaxis.axis == m_axis) {
            if (inside_deadzone) {
              m_state.changed(0);
            } else {
              m_state.changed(static_cast<float>(axis_value) /
                              std::numeric_limits<Sint16>::max());
            }
          }
        }
      }
    }
  }

  float value() { return m_state.value(); }

private:
  SDL_JoystickID m_id;
  std::uint8_t m_axis;
  RawInputState<float> m_state{0.0f};
};

class ControllerButton {
public:
  ControllerButton(int button) : m_button{button} {}

  void update(std::span<SDL_Event> events) {
    for (SDL_Event &event : events) {
      if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == m_button) {
          m_key.pressed_down();
        }
      }

      if (event.type == SDL_CONTROLLERBUTTONUP) {
        if (event.cbutton.button == m_button) {
          m_key.pressed_up();
        }
      }
    }
  }

  bool is_down() { return m_key.is_down(); }

  bool pressed_this_frame(std::span<SDL_Event> events) const {
    for (SDL_Event &event : events) {
      if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == m_button) {
          return true;
        }
      }
    }

    return false;
  }

  bool released_this_frame(std::span<SDL_Event> events) const {
    for (SDL_Event &event : events) {
      if (event.type == SDL_CONTROLLERBUTTONUP) {
        if (event.cbutton.button == m_button) {
          return true;
        }
      }
    }

    return false;
  }

private:
  RawInputKey m_key;
  int m_button;
};
