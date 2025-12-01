#pragma once

#include <VulkanRenderer/Bitmap.hpp>
#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/TextureSamplerCache.hpp>
#include <VulkanRenderer/TexturedMeshCache.hpp>
#include <VulkanRenderer/Vertex.hpp>

#define SIMPLE_GEOMETRY_IMPLEMENTATION
#include <simple_geometry.h>

#include <filesystem>

struct Resources {
  Resources(Resources &&) = delete;
  Resources(const Resources &) = delete;
  Resources &operator=(Resources &&) = delete;
  Resources &operator=(const Resources &) = delete;

  Resources(Render::Context &context, TexturedMeshCache &mesh_cache,
            TextureSamplerCache &texture_cache,
            std::filesystem::path assets_root);

  struct {
    std::optional<TexturedMeshRef> mesh;
  } monkey;

  struct {
    std::optional<TexturedMeshRef> mesh;
    std::optional<TexturedMeshRef> textured_mesh;
  } cube;

  struct {
    std::optional<TexturedMeshRef> mesh;
  } gizmo_cone;

  struct {
    std::optional<TexturedMeshRef> mesh;
  } gizmo_sphere;

  struct {
    std::optional<TexturedMeshRef> mesh;
    std::optional<TexturedMeshRef> textured_mesh;
    std::optional<TextureSamplerRef> diffuse;
  } chest;

  struct {
    std::optional<TexturedMeshRef> mesh;
    std::optional<TextureSamplerRef> diffuse;
  } transformship;

  struct {
    std::optional<TexturedMeshRef> textured_mesh;
    std::optional<TextureSamplerRef> diffuse;
    std::optional<TextureSamplerRef> specular;
    std::optional<TextureSamplerRef> normal;
    std::optional<TextureSamplerRef> glossiness;
  } smg;

  struct {
    std::optional<TextureSamplerRef> diffuse;
    std::optional<TextureSamplerRef> specular;
  } box;

  struct {
    std::optional<TextureSamplerRef> diffuse;
    std::optional<TextureSamplerRef> specular;
    std::optional<TextureSamplerRef> normal;
  } brickwall;

  struct {
    std::optional<TextureSamplerRef> lulu;
    std::optional<TextureSamplerRef> statue;
    std::optional<TextureSamplerRef> pixelart;
  } textures;
};

auto get_textured_cube_vertices() -> std::vector<VertexPosNormColorUV> {
  sg_status status;
  size_t vertices_length{0};

  sg_cube_info cube_info{};
  cube_info.width = 0.5f;
  cube_info.height = 0.5f;
  cube_info.depth = 0.5f;

  status =
      sg_cube_vertices(&cube_info, &vertices_length, nullptr, nullptr, nullptr);
  if (status != SG_OK_RETURNED_LENGTH)
    throw std::runtime_error("Could not get positions size");

  std::vector<sg_position> positions(vertices_length);
  std::vector<sg_normal> normals(vertices_length);
  std::vector<sg_texcoord> texcoords(vertices_length);
  status = sg_cube_vertices(&cube_info, &vertices_length, positions.data(),
                            normals.data(), texcoords.data());
  if (status != SG_OK_RETURNED_BUFFER)
    throw std::runtime_error("Could not get vertices");

  std::vector<VertexPosNormColorUV> vertices(positions.size());
  for (size_t i = 0; i < vertices.size(); i++) {
    vertices[i].pos = {positions[i].x, positions[i].y, positions[i].z};
    vertices[i].norm = {normals[i].x, normals[i].y, normals[i].z};
    vertices[i].color = {1.0f, 1.0f, 1.0f};
    vertices[i].uv = {texcoords[i].u, texcoords[i].v};
  }

  return vertices;
}

auto get_gizmo_cone_vertices() -> std::vector<VertexPosNormColorUV> {
  sg_status status;
  size_t vertices_length{0};

  sg_gizmo_cone_info info{};
  info.height = 1.0f;
  info.radius = 0.5f;

  status = sg_gizmo_cone_vertices(&info, &vertices_length, nullptr);
  if (status != SG_OK_RETURNED_LENGTH)
    throw std::runtime_error("Could not get positions size");

  std::vector<sg_position> positions(vertices_length);
  status = sg_gizmo_cone_vertices(&info, &vertices_length, positions.data());

  if (status != SG_OK_RETURNED_BUFFER)
    throw std::runtime_error("Could not get vertices");

  std::vector<VertexPosNormColorUV> vertices(positions.size());
  for (size_t i = 0; i < vertices.size(); i++) {
    vertices[i].pos = {positions[i].x, positions[i].y, positions[i].z};
    vertices[i].color = {1.0f, 1.0f, 1.0f};
  }

  return vertices;
}

auto get_gizmo_sphere_vertices() -> std::vector<VertexPosNormColorUV> {
  sg_status status;
  size_t vertices_length{0};
  sg_gizmo_sphere_info info{};
  info.radius = 1.0f;

  status = sg_gizmo_sphere_vertices(&info, &vertices_length, nullptr);
  if (status != SG_OK_RETURNED_LENGTH)
    throw std::runtime_error("Could not get positions size");

  std::vector<sg_position> positions(vertices_length);
  status = sg_gizmo_sphere_vertices(&info, &vertices_length, positions.data());

  if (status != SG_OK_RETURNED_BUFFER)
    throw std::runtime_error("Could not get vertices");

  std::vector<VertexPosNormColorUV> vertices(positions.size());
  for (size_t i = 0; i < vertices.size(); i++) {
    vertices[i].pos = {positions[i].x, positions[i].y, positions[i].z};
    vertices[i].color = {1.0f, 1.0f, 1.0f};
  }

  return vertices;
}

Resources::Resources(Render::Context &context, TexturedMeshCache &mesh_cache,
                     TextureSamplerCache &texture_cache,
                     std::filesystem::path assets_root) {
  std::filesystem::path models_root = assets_root / "models/";
  std::filesystem::path textures_root = assets_root / "textures/";

  cube.textured_mesh =
      mesh_cache.add(context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(
          context, get_textured_cube_vertices())});

  gizmo_sphere.mesh =
      mesh_cache.add(context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(
          context, get_gizmo_sphere_vertices())});

  gizmo_cone.mesh =
      mesh_cache.add(context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(
          context, get_gizmo_cone_vertices())});

  auto loaded_monkey_mesh =
      load_obj_with_texcoords(context, assets_root, "models/monkey/monkey_flat.obj");

  if (std::holds_alternative<TexturedMesh>(loaded_monkey_mesh)) {
    monkey.mesh = mesh_cache.add(context, std::move(std::get<TexturedMesh>(loaded_monkey_mesh)));
  } else if (std::holds_alternative<TexturedMeshWithWarning>(loaded_monkey_mesh)) {
    std::cout << "TinyOBJ Warning: "
              << std::get<TexturedMeshWithWarning>(loaded_monkey_mesh).warning
              << std::endl;
    monkey.mesh =
        mesh_cache.add(context, std::move(std::get<TexturedMeshWithWarning>(loaded_monkey_mesh).mesh));
  } else if (std::holds_alternative<MeshInvalidPath>(loaded_monkey_mesh)) {
    auto error = std::get<MeshInvalidPath>(loaded_monkey_mesh);
    throw std::runtime_error(std::string("InvalidPath: ") +
                             (error.path / error.filename).string());
  } else if (std::holds_alternative<MeshLoadError>(loaded_monkey_mesh)) {
    throw std::runtime_error(std::string("TinyOBJ error: ") +
                             std::get<MeshLoadError>(loaded_monkey_mesh).msg);
  }

  std::cout << "loading textured chest model!" << std::endl;
  auto loaded_textured_chest = load_obj_with_texcoords(
      context, assets_root, "models/ChestWowStyle/Chest.obj");
  if (auto p = std::get_if<TexturedMesh>(&loaded_textured_chest)) {
    chest.textured_mesh = mesh_cache.add(context, std::move(*p));
  } else if (auto p =
                 std::get_if<TexturedMeshWithWarning>(&loaded_textured_chest)) {
    std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
    chest.textured_mesh = mesh_cache.add(context, std::move(p->mesh));
  } else if (auto p = std::get_if<MeshLoadError>(&loaded_textured_chest)) {
    throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
  }

  // https://opengameart.org/art-search-advanced?keys=&field_art_type_tid%5B0%5D=10&sort_by=count&sort_order=DESC&page=3
  std::cout << "loading chest texture!" << std::endl;

  chest.diffuse =
	  texture_cache.load_from_path(&context,
								   "ChestDiffuse",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::Yes,
								   BitmapPixelFormat::RGBA,
								   models_root / "ChestWowStyle/diffuse.tga");

  std::cout << "loading TransformShip model!" << std::endl;
  auto loaded_transformship = load_obj_with_texcoords(
      context, assets_root, "models/TransformShip/TransformSpaceship.obj");
  if (auto p = std::get_if<TexturedMesh>(&loaded_transformship)) {
    transformship.mesh = mesh_cache.add(context, std::move(*p));
  } else if (auto p =
                 std::get_if<TexturedMeshWithWarning>(&loaded_transformship)) {
    std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
    transformship.mesh = mesh_cache.add(context, std::move(p->mesh));
  } else if (auto p = std::get_if<MeshLoadError>(&loaded_transformship)) {
    throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
  }

  std::cout << "loading chest texture!" << std::endl;
  transformship.diffuse =
	  texture_cache.load_from_path(&context,
								   "TransformShipDiffuse",
								   InterpolationType::Point,
								   VerticalFlipOnLoad::Yes,
								   BitmapPixelFormat::RGBA,
								   models_root / "TransformShip/TransformShipTexture.png");
												   
  auto loaded_textured_smg =
      load_obj_with_texcoords(context, models_root, "smg/smg.obj");
  if (auto p = std::get_if<TexturedMesh>(&loaded_textured_smg)) {
    smg.textured_mesh = mesh_cache.add(context, std::move(*p));
  } else if (auto p =
                 std::get_if<TexturedMeshWithWarning>(&loaded_textured_smg)) {
    std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
    smg.textured_mesh = mesh_cache.add(context, std::move(p->mesh));
  } else if (auto p = std::get_if<MeshLoadError>(&loaded_textured_smg)) {
    throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
  }

  std::cout << "loading smg textures!" << std::endl;
  smg.diffuse =
	  texture_cache.load_from_path(&context,
								   "smgDiffuse",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::Yes,
								   BitmapPixelFormat::RGBA,
								   models_root / "smg/D.tga");
  
  smg.specular =
	  texture_cache.load_from_path(&context,
								   "smgSpecular",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::Yes,
								   BitmapPixelFormat::RGBA,
								   models_root / "smg/S.tga");
 
  smg.normal =
	  texture_cache.load_from_path(&context,
								   "smgNormal",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::Yes,
								   BitmapPixelFormat::RGBA,
								   models_root / "smg/N.tga");
 
  smg.glossiness =
	  texture_cache.load_from_path(&context,
								   "smgGlossiness",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::Yes,
								   BitmapPixelFormat::RGBA,
								   models_root / "smg/G.tga");

  // TODO: this is a glb model so textures are embedded! see
  // ModelLoader.cpp for more info and provide a fix!

  std::cout << "loading statue jpg!" << std::endl;
  textures.statue =
		  texture_cache.load_from_path(&context,
								   "statue",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::No,
								   BitmapPixelFormat::RGBA,
								   textures_root / "texture.jpg");
  
  std::cout << "loading lulu jpg!" << std::endl;
  textures.lulu =
		  texture_cache.load_from_path(&context,
								   "lulu",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::No,
								   BitmapPixelFormat::RGBA,
								   textures_root / "lulu.jpg");

  box.diffuse =
		  texture_cache.load_from_path(&context,
								   "boxDiffuse",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::No,
								   BitmapPixelFormat::RGBA,
								   textures_root / "box/diffuse.png");

  box.specular =
		  texture_cache.load_from_path(&context,
								   "boxSpecular",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::No,
								   BitmapPixelFormat::RGBA,
								   textures_root / "box/specular.png");

  brickwall.diffuse =
		  texture_cache.load_from_path(&context,
									   "brickWallDiffuse",
									   InterpolationType::Linear,
									   VerticalFlipOnLoad::No,
									   BitmapPixelFormat::RGBA,
									   textures_root / "brick_wall/brick_wall2-diff-2048.tga");

  brickwall.specular =
	  texture_cache.load_from_path(&context,
									   "brickWallSpecular",
									   InterpolationType::Linear,
									   VerticalFlipOnLoad::No,
									   BitmapPixelFormat::RGBA,
									   textures_root / "brick_wall/brick_wall2-spec-2048.tga");

  brickwall.normal =
	  texture_cache.load_from_path(&context,
								   "brickWallNormal",
								   InterpolationType::Linear,
								   VerticalFlipOnLoad::No,
								   BitmapPixelFormat::RGBA,
								   textures_root / "brick_wall/brick_wall2-nor-2048.tga");
 
  textures.pixelart =
	  texture_cache.load_from_path(&context,
								   "brickWallNormal",
								   InterpolationType::Point,
								   VerticalFlipOnLoad::No,
								   BitmapPixelFormat::RGBA,
								   textures_root / "cat_pixelart.png");
 }
