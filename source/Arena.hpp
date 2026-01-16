#pragma once

#include <atomic>
#include <concepts>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>

namespace memory {

template <typename T> struct trait {
  using notref_t = std::remove_cvref_t<T>;
  static constexpr std::size_t size = sizeof(notref_t);
  static constexpr std::size_t alignment = std::alignment_of_v<notref_t>;
  static constexpr std::size_t alignedsize = alignment + size;
};

struct alignment {
  explicit constexpr alignment(std::size_t value) : m_value(value) {}

  constexpr auto value() const -> std::size_t { return m_value; }

private:
  std::size_t m_value;
};

struct no_alignment_t {
  explicit constexpr no_alignment_t(int magic) : _magic(magic) {}

private:
  int _magic;
};

struct default_alignment_t {
  explicit constexpr default_alignment_t(int magic) : _magic(magic) {}

private:
  int _magic;
};

[[maybe_unused]] static constexpr no_alignment_t no_alignment(7);
[[maybe_unused]] static constexpr default_alignment_t default_alignment(7);

constexpr std::intptr_t alignforward(alignment alignment, std::intptr_t ptr) {
  if (alignment.value() == 0)
    return ptr;
  return ptr + (((~ptr) + 1) & (alignment.value() - 1));
}

template <typename P>
  requires std::is_pointer_v<P>
constexpr P alignforward(alignment alignment, P p) {
  return reinterpret_cast<P>(
      alignforward(alignment, reinterpret_cast<std::intptr_t>(p)));
}

class arena_checkpoint {
public:
  using pointer_t = std::uint8_t *;
  explicit constexpr arena_checkpoint(pointer_t ptr) noexcept;
  constexpr arena_checkpoint(std::nullptr_t) = delete;
  constexpr ~arena_checkpoint() = default;
  constexpr arena_checkpoint(const arena_checkpoint &) = default;
  constexpr arena_checkpoint(arena_checkpoint &&) = default;
  constexpr arena_checkpoint &operator=(const arena_checkpoint &) = default;
  constexpr arena_checkpoint &operator=(arena_checkpoint &&) = default;

  constexpr auto value() const noexcept -> pointer_t;

private:
  pointer_t m_top;
};

// TODO: you REALLY need to test this implementation before starting to use it!
class atomic_arena {
public:
  using byte_t = std::uint8_t;
  using memoryspan_t = std::span<byte_t>;
  using memory_pointer_t = memoryspan_t::pointer;
  using memory_size_t = memoryspan_t::size_type;
  using element_count_t = std::size_t;

  static_assert(std::is_same_v<arena_checkpoint::pointer_t, memory_pointer_t>);

  atomic_arena(memoryspan_t memory) noexcept;
  constexpr atomic_arena(const atomic_arena &) = delete;
  constexpr atomic_arena &operator=(atomic_arena) = delete;
  constexpr ~atomic_arena() noexcept = default;

  auto allocate_bytes(alignment alignment, memory_size_t n) noexcept
      -> std::optional<memoryspan_t>;

  auto allocate_bytes(no_alignment_t, memory_size_t n) noexcept
      -> std::optional<memoryspan_t>;

  auto allocate_bytes(default_alignment_t, memory_size_t n) noexcept
      -> std::optional<memoryspan_t>;

  template <typename Ti, typename T = trait<Ti>::notref_t>
  auto allocate_elements(alignment alignment, element_count_t n) noexcept
      -> std::optional<std::span<T>>
    requires std::is_trivially_constructible_v<T> &&
             std::is_trivially_destructible_v<T>
  {
    return element_memoryspan<T>(allocate_bytes(alignment, n * sizeof(T)), n);
  }

  template <typename Ti, typename T = trait<Ti>::notref_t>
  auto allocate_elements(no_alignment_t, element_count_t n) noexcept
      -> std::optional<std::span<T>>
    requires std::is_trivially_constructible_v<T> &&
             std::is_trivially_destructible_v<T>
  {
    return element_memoryspan<T>(allocate_bytes(no_alignment, n * sizeof(T)),
                                 n);
  }

  template <typename Ti, typename T = trait<Ti>::notref_t>
  auto allocate_elements(default_alignment_t, element_count_t n) noexcept
      -> std::optional<std::span<T>>
    requires std::is_trivially_constructible_v<T> &&
             std::is_trivially_destructible_v<T>
  {
    constexpr alignment alignment{trait<T>::alig};
    constexpr memory_size_t total_size{n * trait<T>::size};
    return element_memoryspan<T>(allocate_bytes(alignment, total_size), n);
  }

  void reset();
  auto total_memory() const noexcept -> memory_size_t;
  auto available_memory() const noexcept -> memory_size_t;
  auto used_memory() const noexcept -> memory_size_t;
  auto revert(arena_checkpoint checkpoint) noexcept -> bool;

  // DEBUGGING
  auto top_ptr() const noexcept -> memory_pointer_t;

private:
  template <typename Ti, typename T = trait<Ti>::notref_t>
  static constexpr auto element_memoryspan(std::optional<memoryspan_t> memory,
                                           element_count_t n) noexcept
      -> std::optional<std::span<T>> {
    if (!memory.has_value())
      return std::nullopt;
    return std::span<T>(reinterpret_cast<T *>(memory.value().data()), n);
  }

  memoryspan_t m_memory;
  std::atomic<memory_pointer_t> m_top;
};

} // namespace memory
