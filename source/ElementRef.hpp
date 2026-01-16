#pragma once

#if 0
template <typename T> class element_ref {
public:
  using value_t = std::remove_const<T>::type;
  using pointer_t = value_t *;
  using const_pointer_t = const pointer_t;
  using reference_t = value_t &;

  explicit constexpr element_ref(std::nullptr_t) = delete;

  explicit constexpr element_ref(pointer_t ptr)
    requires std::is_trivially_constructible_v<T>
      : m_ptr{ptr} {}

  explicit constexpr element_ref(pointer_t ptr)
    requires std::is_default_constructible_v<T>
      : m_ptr{ptr} {
    m_ptr->T();
  }

  template <typename... Args>
  explicit constexpr element_ref(pointer_t ptr, Args &&...args)
    requires std::is_constructible_v<T, Args...>
      : m_ptr{ptr} {
    m_ptr->T(std::forward<Args>(args)...);
  }

  element_ref(const element_ref &) = delete;
  element_ref &operator=(const element_ref &) = delete;
  explicit constexpr element_ref(element_ref &&) noexcept = default;
  constexpr element_ref &operator=(element_ref &&) noexcept = default;

  ~element_ref()
    requires std::is_trivially_destructible_v<T>
  {}

  ~element_ref()
    requires std::destructible<T>
  {
    if (m_ptr != nullptr)
      m_ptr->~T();
  }

  void destruct()
    requires std::is_trivially_destructible_v<T>
  {}

  void destruct()
    requires std::destructible<T>
  {
    if (m_ptr != nullptr)
      m_ptr->~T();
  }

  auto get() noexcept -> pointer_t { return m_ptr; };
  // auto get() const noexcept -> const_pointer_t { return m_ptr; };
  auto operator*() noexcept -> reference_t { return m_ptr; };
  auto ref() noexcept -> reference_t { return m_ptr; };

private:
  pointer_t m_ptr;
};

#endif
