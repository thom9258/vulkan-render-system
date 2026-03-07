#pragma once

#include <memory>
#include <vector>

namespace scene_system {

template <class T_UpdateInfoType> class Scene {
public:
  using UpdateInfoType = std::remove_cvref_t<T_UpdateInfoType>;
  virtual void init();
  virtual void destroy();
  virtual void update(UpdateInfoType update_info);
};

template <class T_UpdateInfoType> class SceneStack {
public:
  using UpdateInfoType = std::remove_cvref_t<T_UpdateInfoType>;
  using SceneType = Scene<UpdateInfoType>;

  constexpr SceneStack() = default;

  constexpr ~SceneStack() {
    while (std::shared_ptr<SceneType> active = get_active()) {
      active->destroy();
      m_stack.pop_back();
    }
  }

  constexpr std::size_t size() const { return m_stack.size(); }

  constexpr std::shared_ptr<SceneType> get_active() {
    if (size() > 0)
      return m_stack.back();
    return nullptr;
  }

  constexpr void put(std::shared_ptr<SceneType> scene) {
    m_stack.push_back(scene);
    scene->init();
  }

  constexpr std::shared_ptr<SceneType> pop() {
    std::shared_ptr<SceneType> active = get_active();
    if (active) {
      active->destroy();
      m_stack.pop_back();
    }

    return active;
  }

  constexpr void update_active(UpdateInfoType update_info) {
    std::shared_ptr<SceneType> active = get_active();
    if (active)
      active->update(update_info);
  }

private:
  std::vector<std::shared_ptr<SceneType>> m_stack;
};

} // namespace scene_system
