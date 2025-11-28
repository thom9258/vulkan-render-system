#pragma once

#include "Renderable.hpp"

#include <filesystem>


RenderableNodePtr load_model(Render::Context& context,
							 TexturedMeshCache& texturedmesh_cache,
							 TextureSamplerCache& texture_cache,
							 std::filesystem::path path);
