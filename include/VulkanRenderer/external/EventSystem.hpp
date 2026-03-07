#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace event_system {

template <class T_EventType> class EventDispatch {
public:
  using EventType = std::remove_cvref_t<T_EventType>;
  using EventCallbackType = std::function<void(EventType)>;

  constexpr std::size_t dispatch_events() {
    for (EventType &event : m_events) {
      for (auto &[name, callback] : m_callbacks) {
        callback(event);
      }
    }

    m_events.clear();
  }

  constexpr std::size_t add_event(EventType event) {
    m_events.push_back(event);
    return m_events.size();
  }

  constexpr bool add_callback(std::string_view name,
                              EventCallbackType callback) {
    if (m_callbacks.contains(name)) {
      return false;
    }

    m_callbacks.insert(std::string(name), callback);
    return true;
  }

  constexpr void remove_callback(std::string_view name) {
    auto found = m_callbacks.find(name);
    if (found == m_callbacks.end()) {
      return;
    }

    m_callbacks.erase(found);
  }

private:
  std::vector<EventType> m_events;
  std::map<std::string, EventCallbackType> m_callbacks;
};

} // namespace event_system
