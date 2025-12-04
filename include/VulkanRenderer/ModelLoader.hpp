#pragma once

#include "Renderable.hpp"

#include <filesystem>


RenderableNodePtr load_model(Render::Context& context,
							 MeshCache& mesh_cache,
							 TextureSamplerCache& texture_cache,
							 std::filesystem::path path);
