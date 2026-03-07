#pragma once

#include <algorithm>
#include <chrono>
#include <deque>

class FPSCounter {
public:
  constexpr auto next_frame(std::chrono::duration<double> deltatime)
      -> std::size_t {

    m_times.push_back(deltatime);
    while (should_pop_time()) {
      m_times.pop_front();
    }

    return m_times.size();
  }

private:
  constexpr auto should_pop_time() -> bool {
    auto sum = std::ranges::fold_left_first(m_times, std::plus{});
    if (!sum) {
      return false;
    }

    if (*sum > std::chrono::seconds(1)) {
      return true;
    }

    return false;
  }

  std::deque<std::chrono::duration<double>> m_times;
};
