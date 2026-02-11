#pragma once

#include <algorithm>
#include <array>
#include <expected>
#include <filesystem>
#include <fstream>
#include <optional>
#include <vector>

namespace config {

namespace id {
static constexpr std::string_view f32 = "float";
static constexpr std::string_view f32array = "float[]";
static constexpr std::string_view i32 = "int";
static constexpr std::string_view i32array = "int[]";
static constexpr std::string_view str = "str";
static constexpr std::string_view strarray = "str[]";
} // namespace id

namespace lex {
static constexpr std::string_view whitespaces = " \t\v";

[[nodiscard]] constexpr auto trim_left(std::string_view const &data,
                                       std::string_view trimChars)
    -> std::string_view {
  std::string_view sv{data};
  sv.remove_prefix(std::min(sv.find_first_not_of(trimChars), sv.size()));
  return sv;
}

[[nodiscard]] constexpr auto whitespace_trim_left(std::string_view const &data)
    -> std::string_view {
  return trim_left(data, whitespaces);
}

[[nodiscard]] constexpr auto take_while_not(std::string_view const &data,
                                            std::string_view trimChars)
    -> std::string_view {
  return {data.data(), std::min(data.find_first_of(trimChars), data.size())};
}

[[nodiscard]] constexpr auto
take_while_not_whitespace(std::string_view const &data) -> std::string_view {
  return take_while_not(data, whitespaces);
}

} // namespace lex

template <typename T>
using AssociatedVector = std::vector<std::pair<std::string, T>>;

struct ParsedConfig {
  AssociatedVector<float> f32s;
  AssociatedVector<int> i32s;
#if 0
  AssociatedVector<std::string> strs;
  AssociatedVector<std::vector<float>> f32arrays;
  AssociatedVector<std::vector<int> i32arrays;
  AssociatedVector<std::vector<std::string>> strarrays;
#endif
};

template <typename AV, typename E = AV::value_type::second_type>
[[nodiscard]] constexpr auto assoc(std::string_view name, AV &av)
    -> std::optional<E> {
  auto found =
      std::ranges::find_if(av, [&](auto &pair) { return pair.first == name; });

  if (found == std::ranges::end(av))
    return std::nullopt;
  return found->second;
}

namespace parse {

[[nodiscard]] constexpr auto parse_f32(std::string_view line,
                                       std::size_t line_number)
    -> std::expected<std::pair<std::string, float>, std::string> {

  line.remove_prefix(id::f32.size());
  line = lex::whitespace_trim_left(line);
  std::string_view name = lex::take_while_not_whitespace(line);
  if (name.empty()) {
    return std::unexpected(
        std::format("Invalid line '{}' while parsing name", line_number));
  }
  line.remove_prefix(name.size());

  line = lex::whitespace_trim_left(line);
  std::string_view valuestr = lex::take_while_not_whitespace(line);
  if (valuestr.empty()) {
    return std::unexpected(
        std::format("Invalid line '{}' while parsing value", line_number));
  }

  float value = std::stof(std::string(valuestr));
  return std::pair<std::string, float>{std::string(name), value};
}
	
[[nodiscard]] constexpr auto parse_i32(std::string_view line,
                                       std::size_t line_number)
    -> std::expected<std::pair<std::string, std::int32_t>, std::string> {

  line.remove_prefix(id::i32.size());
  line = lex::whitespace_trim_left(line);
  std::string_view name = lex::take_while_not_whitespace(line);
  if (name.empty()) {
    return std::unexpected(
        std::format("Invalid line '{}' while parsing name", line_number));
  }
  line.remove_prefix(name.size());

  line = lex::whitespace_trim_left(line);
  std::string_view valuestr = lex::take_while_not_whitespace(line);
  if (valuestr.empty()) {
    return std::unexpected(
        std::format("Invalid line '{}' while parsing value", line_number));
  }

  std::int32_t value = std::stoi(std::string(valuestr));
  return std::pair<std::string, std::int32_t>{std::string(name), value};
}

} // namespace parse

using MaybeParsedConfig = std::expected<ParsedConfig, std::string>;
[[nodiscard]] constexpr auto parse_config(std::filesystem::path path)
    -> MaybeParsedConfig {
  if (!std::filesystem::exists(path) || !std::filesystem::is_regular_file(path))
    return std::unexpected(
        std::format("File '{}' did not exist!", path.string()));

  std::string line;
  std::fstream file;
  file.open(path.string());

  ParsedConfig config;
  std::size_t line_number = 0;
  while (std::getline(file, line)) {

    if (line.starts_with(id::f32)) {
      auto entry = parse::parse_f32(line, line_number);
      if (!entry.has_value())
        return std::unexpected(entry.error());
      config.f32s.push_back(entry.value());
	  continue;
    }

    else if (line.starts_with(id::i32)) {
      auto entry = parse::parse_i32(line, line_number);
      if (!entry.has_value())
        return std::unexpected(entry.error());
      config.i32s.push_back(entry.value());
	  continue;
    } else {
		std::println("Unknown line content '{}'", line);
    }

    // TODO: parse more types -> extract parsing into easy functions!
    line_number++;
  }

  return config;
}

} // namespace config
