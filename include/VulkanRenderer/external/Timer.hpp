#pragma once

#include <chrono>
#include <functional>

template <typename F, typename... Args>
std::chrono::duration<double> with_time_measurement(F &&f, Args &&...args) {
  using Clock = std::chrono::high_resolution_clock;
  auto start = Clock::now();
  std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  auto end = Clock::now();
  return end - start;
}
