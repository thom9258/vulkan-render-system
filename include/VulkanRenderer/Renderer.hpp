#pragma once

#include "Context.hpp"
#include "DescriptorPool.hpp"
#include "Light.hpp"
#include "Renderable.hpp"
#include "ShadowCaster.hpp"

#include <filesystem>

struct CurrentFrameInfo
{
	uint64_t total_frame_count;
	uint64_t current_flight_frame_index;
};

struct WorldRenderInfo {
  glm::mat4 view;
  glm::mat4 projection;
  glm::vec3 camera_position;
};

struct RenderInfo {
  TextureSamplerCache *texturecache;
  MeshCache *meshcache;
  WorldRenderInfo world;
  std::vector<Renderable> renderables;
  std::vector<Light> lights;
  ShadowCasters shadowcasters;
};


struct RenderedFrameStats {
};

using RenderInfoCreator = std::function<RenderInfo(CurrentFrameInfo)>;

class Renderer {
public:
  Renderer(Render::Context &context, Logger logger,
           DescriptorPool &descriptor_pool,
           const std::filesystem::path shaders_root);

  ~Renderer();

  RenderedFrameStats with_render(Render::Context *context, RenderInfoCreator render_info_creator);

  class Impl;
  std::unique_ptr<Impl> impl;
};
