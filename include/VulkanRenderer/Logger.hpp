#pragma once

#include <functional>
#include <print>
#include <source_location>
#include <string_view>
#include <utility>

namespace render::log {

enum class Type { Info, Warn, Error, Fatal };

constexpr auto to_string_view(Type t) -> std::string_view {
  switch (t) {
    using enum Type;
  case Info:
    return "Info";
  case Warn:
    return "Warn";
  case Error:
    return "Error";
  case Fatal:
    return "Fatal";
  }
  std::unreachable();
}

using LogFunction =
    std::function<void(std::source_location, Type, std::string_view)>;

void default_log_function(std::source_location loc, Type type,
                          std::string_view msg) {
  std::println("[{}:{}] ({}) {}", loc.file_name(), loc.line(),
               to_string_view(type), msg);
}

namespace global {
static LogFunction log_function = default_log_function;
}

template <typename... Args>
void log(std::source_location loc, Type type, std::format_string<Args...> fmt,
         Args &&...args) {
  global::log_function(loc, type,
                       std::format(fmt, std::forward<Args>(args)...));
}

} // namespace render::log
