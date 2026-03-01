#pragma once

#define SIMPLE_GEOMETRY_IMPLEMENTATION
#include <SimpleGeometry/simple_geometry.h>

#include <VulkanRenderer/Vertex.hpp>
#include <VulkanRenderer/MeshCache.hpp>

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
