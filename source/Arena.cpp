#include "Arena.hpp"
#include <atomic>

namespace memory {

constexpr arena_checkpoint::arena_checkpoint(pointer_t ptr) noexcept
    : m_top{ptr} {}

constexpr auto arena_checkpoint::value() const noexcept -> pointer_t {
  return m_top;
}

atomic_arena::atomic_arena(memoryspan_t memory) noexcept
    : m_memory{memory} {
  m_top.store(m_memory.data());
}

auto atomic_arena::allocate_bytes(alignment alignment, memory_size_t n) noexcept
    -> std::optional<memoryspan_t> {

  if (n < 1) {
    return std::nullopt;
  }

  memory_pointer_t original_top{nullptr};
  memory_pointer_t allocation_start{nullptr};
  memory_pointer_t allocation_end{nullptr};
  do {
    original_top = m_top.load();
    if (original_top == nullptr) {
      return std::nullopt;
    }

    allocation_start = alignforward<memory_pointer_t>(alignment, original_top);
	if (allocation_start + n >= m_memory.data() + m_memory.size())
		return std::nullopt;

    allocation_end = allocation_start + n;
  } while (!m_top.compare_exchange_strong(original_top, allocation_end));

  return memoryspan_t(allocation_start, n);
}

auto atomic_arena::allocate_bytes(no_alignment_t, memory_size_t n) noexcept
    -> std::optional<memoryspan_t> {
  return allocate_bytes(alignment{0}, n);
}

auto atomic_arena::allocate_bytes(default_alignment_t, memory_size_t n) noexcept
    -> std::optional<memoryspan_t> {
  return allocate_bytes(alignment{trait<byte_t>::alignment}, n);
}

void atomic_arena::reset() { m_top.store(m_memory.data()); }

auto atomic_arena::total_memory() const noexcept -> memory_size_t {
  return m_memory.size();
}

auto atomic_arena::used_memory() const noexcept -> memory_size_t {
  return m_top.load() - m_memory.data();
}

auto atomic_arena::available_memory() const noexcept -> memory_size_t {
  return total_memory() - used_memory();
}

auto atomic_arena::revert(arena_checkpoint checkpoint) noexcept -> bool {

  if (checkpoint.value() > m_memory.data() &&
      checkpoint.value() < m_memory.data() + m_memory.size()) {
    m_top.store(checkpoint.value());
    return true;
  }

  return false;
}

auto atomic_arena::top_ptr() const noexcept -> memory_pointer_t {
  return m_top.load();
}

} // namespace memory
