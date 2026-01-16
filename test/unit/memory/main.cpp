#include "../../../source/Arena.hpp"

#include <array>
#include <memory>
#include <print>
#include <vector>

#define LT_MAX_TESTS 1024
#include <libtester/libtester.h>

namespace m = memory;

void print_memory_usage(m::atomic_arena &arena) {
  std::println("available memory: {}", arena.available_memory());
  std::println("used memory: {}", arena.used_memory());
}

void test_simple_allocations_increases_usage() {
  constexpr m::alignment alignment{16};
  constexpr std::size_t memory_size = 1024;
  std::array<std::uint8_t, memory_size> memory;
  m::atomic_arena arena(memory);
  print_memory_usage(arena);

  TEST(arena.total_memory() == memory_size);
  TEST(arena.available_memory() == memory_size);
  {
    auto a = arena.allocate_bytes(alignment, 10);
    print_memory_usage(arena);
    TEST(a.has_value());
    if (!a.has_value())
      return;
    TEST(a.value().size() == 10);
    TEST(alignment.value() > a.value().size());
    TEST(arena.used_memory() == 10);
  }
  {
    auto a = arena.allocate_bytes(alignment, 500);
    print_memory_usage(arena);
    TEST(a.has_value());
    if (!a.has_value())
      return;
    TEST(a.value().size() == 500);
    TEST(arena.used_memory() == alignment.value() + 500);
  }
}

struct Foo {
  std::uint8_t bytes[17];
};

void test_element_allocations_increases_usage() {
  constexpr m::alignment alignment{16};
  constexpr std::size_t memory_size = 1024;
  std::array<std::uint8_t, memory_size> memory;
  m::atomic_arena arena(memory);
  print_memory_usage(arena);
  std::println("size of Foo: {}", m::trait<Foo>::size);
  std::println("m::alignment of Foo: {}", m::trait<Foo>::alignment);
  std::println("aligned size of Foo: {}", m::trait<Foo>::alignedsize);

  TEST(arena.total_memory() == memory_size);
  TEST(arena.available_memory() == memory_size);
  {
    auto a = arena.allocate_elements<Foo>(alignment, 1);
    print_memory_usage(arena);
    TEST(a.has_value());
    if (!a.has_value())
      return;
    TEST(a.value().size() == 1);
    TEST(arena.used_memory() == m::trait<Foo>::size);
  }
  {
    auto a = arena.allocate_elements<Foo>(alignment, 5);
    print_memory_usage(arena);
    TEST(a.has_value());
    if (!a.has_value())
      return;
    TEST(a.value().size() == 5);
    const std::size_t expected =
        m::alignforward(alignment, m::trait<Foo>::size) + m::trait<Foo>::size * 5;
    std::println("expected used memory: {}", expected);
    std::println("actual used memory: {}", arena.used_memory());
    TEST(arena.used_memory() == expected);
  }
}

constexpr std::intptr_t verbose_alignforward(m::alignment alignment,
                                             std::intptr_t ptr) {
  std::intptr_t aligned = alignforward(alignment, ptr);
  std::println("ptr {} aligned forward with {} -> {}", ptr, alignment.value(),
               aligned);
  return aligned;
}

void test_alignforward() {
  TEST(verbose_alignforward(m::alignment{0}, 28) == 28);
  TEST(verbose_alignforward(m::alignment{2}, 27) == 28);
  TEST(verbose_alignforward(m::alignment{2}, 28) == 28);
  TEST(verbose_alignforward(m::alignment{10}, 28) == 28);
  TEST(verbose_alignforward(m::alignment{32}, 28) == 32);
  TEST(verbose_alignforward(m::alignment{32}, 114) == 128);
  TEST(verbose_alignforward(m::alignment{4}, 777) == 780);
  TEST(780 % 4 == 0);
}

void test_allocation_alignment() {
  constexpr std::size_t memory_size = 1024;
  std::vector<std::uint8_t> memory(memory_size);
  m::atomic_arena arena(memory);
  print_memory_usage(arena);
  std::println("size of Foo: {}", m::trait<Foo>::size);
  std::println("m::alignment of Foo: {}", m::trait<Foo>::alignment);
  std::println("aligned size of Foo: {}", m::trait<Foo>::alignedsize);
  std::println("size of uint8_t: {}", m::trait<std::uint8_t>::size);
  std::println("m::alignment of uint8_t: {}", m::trait<std::uint8_t>::alignment);
  std::println("aligned size of uint8_t: {}", m::trait<std::uint8_t>::alignedsize);

  TEST(arena.total_memory() == memory_size);
  TEST(arena.available_memory() == memory_size);
  {
    auto a = arena.allocate_bytes(m::default_alignment, 100);
    print_memory_usage(arena);
    TEST(a.has_value());
    TEST(arena.used_memory() == 100);
  }
  {
	  auto old_top = arena.top_ptr();

    auto a = arena.allocate_elements<Foo>(m::alignment{16}, 5);
    print_memory_usage(arena);
    TEST(a.has_value());
    if (!a.has_value())
      return;
    TEST(a.value().size() == 5);
    TEST(a.value().size_bytes() == m::trait<Foo>::size * 5);
    auto expected_top =
        old_top + m::alignforward(m::alignment{16}, m::trait<Foo>::size * 5);
    std::println("expected top ptr: {}", (void*)expected_top);
    std::println("actual top pointer: {}", (void*)arena.top_ptr());
    TEST(arena.top_ptr() == expected_top);
  }
}

int main(int argc, char **argv) {
  ltcontext_begin(argc, argv);

  TEST_UNIT(test_simple_allocations_increases_usage());
  TEST_UNIT(test_element_allocations_increases_usage());
  TEST_UNIT(test_alignforward());
  TEST_UNIT(test_allocation_alignment());

  ltcontext_end();
}
