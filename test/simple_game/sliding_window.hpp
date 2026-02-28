#pragma once

#include <deque>
#include <algorithm>

template <typename T> class sliding_window {
public:
  sliding_window(std::size_t capacity) : capacity{capacity} {}

  void put(const T &t) {
    maybe_pop_on_full();
    queue.push_back(t);
  }

  void put(T &&t) {
    maybe_pop_on_full();
    queue.push_back(std::move(t));
  }

  T average() {
    T sum = 0;
    for (T value : queue)
      sum += value;
    return sum / capacity;
  }

private:
  void maybe_pop_on_full() {
    if (queue.size() > capacity)
      queue.pop_front();
  }

  std::size_t capacity;
  std::deque<T> queue;
};
