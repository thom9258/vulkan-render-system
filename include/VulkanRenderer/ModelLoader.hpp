#pragma once

#include "Renderable.hpp"

#include <filesystem>


std::optional<RenderableTree> load_model(Render::Context& context, std::filesystem::path path);
