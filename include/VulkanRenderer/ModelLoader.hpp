#pragma once

#include "Renderable.hpp"
#include "Animation.hpp"

#include <filesystem>
#include <format>
#include <expected>

RenderableNodePtr load_model(Render::Context &context, MeshCache &mesh_cache,
                             TextureSamplerCache &texture_cache,
                             std::filesystem::path path);

struct LoadedAnimatedModel
{
	RenderableNodePtr renderable;
	std::vector<Animation> animations;
	BoneInfos bone_infos;
	std::filesystem::path base_directory;
};

auto load_animated_model(Render::Context &context, MeshCache &mesh_cache,
                         TextureSamplerCache &texture_cache,
                         std::filesystem::path path)
    -> std::expected<LoadedAnimatedModel, std::string>;
