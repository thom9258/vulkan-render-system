#pragma once

#include "Renderable.hpp"

#include <filesystem>
#include <variant>
#include <vector>
#include <map>

std::optional<RenderableTree> load_model(Render::Context& context, std::filesystem::path path);
