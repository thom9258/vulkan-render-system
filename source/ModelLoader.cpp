#include <VulkanRenderer/ModelLoader.hpp>

#include "ShaderTexture.hpp"
#include "MeshCache.hpp"
#include "Vertex.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>

// TODO: this is kinda dirty and a waste of memory, we should add proper support
//       for indexed vertice buffers instead..
auto unindex_vertices(std::vector<VertexPosNormColorUV> vertices,
                      std::vector<std::uint32_t> indices)
    -> std::vector<VertexPosNormColorUV> {
  std::vector<VertexPosNormColorUV> unindexed;
  unindexed.reserve(indices.size());

  for (std::uint32_t index : indices)
    unindexed.push_back(vertices.at(index));
  return unindexed;
}

void print_mesh_material_info(std::string_view prefix,
                              std::filesystem::path const &base_directory,
                              aiMesh *mesh, const aiScene *scene,
                              aiTextureType type) {
  aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
  aiString name = material->GetName();
  std::cout << std::format("Model has {} material called {}", prefix,
                           name.C_Str())
            << std::endl;

  int const count = material->GetTextureCount(type);
  for (int i = 0; i < count; i++) {
    aiString pathstring;
    material->GetTexture(type, i, &pathstring);

    std::filesystem::path path =
        base_directory / std::filesystem::path(pathstring.C_Str());

#if 0
    if (!std::filesystem::exists(path) ||
        !std::filesystem::is_regular_file(path)) {
      std::cout << std::format(
                       " with diffuse texture count {}, at INVALID path", i)
                << std::endl;
    } else {
      std::cout << std::format(" with diffuse texture {}, at path {}", i,
                               path.string())
                << std::endl;
    }
#endif

  }
};

auto process_mesh(Render::Context &context, TextureSamplerCache &texture_cache,
				  MeshCache& mesh_cache,
                  std::filesystem::path const &base_directory, aiMesh *mesh,
                  const aiScene *scene) -> RenderableNode::MaterialMesh {
  std::vector<VertexPosNormColorUV> vertices{};
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    VertexPosNormColorUV vertex;
    vertex.pos[0] = mesh->mVertices[i].x;
    vertex.pos[1] = mesh->mVertices[i].y;
    vertex.pos[2] = mesh->mVertices[i].z;
    vertex.norm[0] = mesh->mNormals[i].x;
    vertex.norm[1] = mesh->mNormals[i].y;
    vertex.norm[2] = mesh->mNormals[i].z;
    vertex.color = glm::vec3(1.0f);

    if (mesh->mTextureCoords[0]) {
      // texcoords have multiple dimensions we only care about the first
      vertex.uv[0] = mesh->mTextureCoords[0][i].x;
      vertex.uv[1] = mesh->mTextureCoords[0][i].y;
    } else {
      vertex.uv = glm::vec2(0.0f);
    }

    vertices.push_back(vertex);
  }

  std::vector<std::uint32_t> indices;
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  std::vector<VertexPosNormColorUV> unindexed =
      unindex_vertices(vertices, indices);

  RenderableNode::MaterialMesh drawable_mesh;
  drawable_mesh.mesh = mesh_cache.add(
      context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(context,
																	   unindexed)});

  drawable_mesh.ambient = std::nullopt;
  drawable_mesh.diffuse = std::nullopt;
  drawable_mesh.specular = std::nullopt;
  drawable_mesh.normal = std::nullopt;
  drawable_mesh.has_shadow = true;

  if (mesh->mMaterialIndex >= 0) {
    auto get_texture_path =
        [&](aiMaterial *material, aiTextureType type,
            int index) -> std::optional<std::filesystem::path> {
      aiString aipathstring;
      material->GetTexture(type, index, &aipathstring);
      std::string pathstring = aipathstring.C_Str();
      // https://gamedev.stackexchange.com/questions/212749/assimp-texture-path-is-bad-for-glb-exported-from-blender
      if (pathstring.starts_with("*")) {
        std::cout << std::format("TODO: Model has EMBEDDED texture path that "
                                 "is NOT supported yet: {}",
                                 pathstring)
                  << std::endl;

        return std::nullopt;
      }

      std::filesystem::path path =
          base_directory / std::filesystem::path(pathstring);

      if (!std::filesystem::exists(path) ||
          !std::filesystem::is_regular_file(path))
        return std::nullopt;

      return path;
    };

    int constexpr first_texture{0};
    aiMaterial *first_material = scene->mMaterials[mesh->mMaterialIndex];

    // ------------------------------------------
    // Diffuse texture
    std::string const diffuse_name = first_material->GetName().C_Str();
    std::optional<std::filesystem::path> diffuse_path =
        get_texture_path(first_material, aiTextureType_DIFFUSE, first_texture);

    if (diffuse_path.has_value()) {
      std::optional<TextureSamplerRef> cached =
          texture_cache.get_ref_from_path(diffuse_path.value());

      if (cached.has_value()) {
        drawable_mesh.diffuse = cached.value();
      } else {
        Texture2D texture =
            load_bitmap(diffuse_path.value(), BitmapPixelFormat::RGBA,
                        VerticalFlipOnLoad::No) |
            throw_on_bitmap_error() | get_bitmap() |
            move_bitmap_to_gpu(&context);

        TextureSamplerRef ref = texture_cache.add_texture(
            &context, InterpolationType::Linear, diffuse_name,
            diffuse_path.value(), std::move(texture));

        drawable_mesh.diffuse = ref;
      }
    }

    // ------------------------------------------
    // Specular texture
    std::string const specular_name = first_material->GetName().C_Str();
    std::optional<std::filesystem::path> specular_path =
        get_texture_path(first_material, aiTextureType_SPECULAR, first_texture);

    if (specular_path.has_value()) {
      std::optional<TextureSamplerRef> cached =
          texture_cache.get_ref_from_path(specular_path.value());

      if (cached.has_value()) {
        drawable_mesh.specular = cached.value();
      } else {
        Texture2D texture =
            load_bitmap(specular_path.value(), BitmapPixelFormat::RGBA,
                        VerticalFlipOnLoad::No) |
            throw_on_bitmap_error() | get_bitmap() |
            move_bitmap_to_gpu(&context);

        TextureSamplerRef ref = texture_cache.add_texture(
            &context, InterpolationType::Linear, specular_name,
            specular_path.value(), std::move(texture));

        drawable_mesh.specular = ref;
      }
    }

    // ------------------------------------------
    // Ambient texture
    std::string const ambient_name = first_material->GetName().C_Str();
    std::optional<std::filesystem::path> ambient_path =
        get_texture_path(first_material, aiTextureType_AMBIENT, first_texture);

    if (ambient_path.has_value()) {
      std::optional<TextureSamplerRef> cached =
          texture_cache.get_ref_from_path(ambient_path.value());

      if (cached.has_value()) {
        drawable_mesh.ambient = cached.value();
      } else {

        Texture2D texture =
            load_bitmap(ambient_path.value(), BitmapPixelFormat::RGBA,
                        VerticalFlipOnLoad::No) |
            throw_on_bitmap_error() | get_bitmap() |
            move_bitmap_to_gpu(&context);

        TextureSamplerRef ref = texture_cache.add_texture(
            &context, InterpolationType::Linear, ambient_name,
            ambient_path.value(), std::move(texture));

        drawable_mesh.ambient = ref;
      }
    }
  }

      return drawable_mesh;
}

glm::mat4 glm_matrix(aiMatrix4x4 other) {
  glm::mat4 matrix;
  for (size_t r = 0; r < 4; r++)
    for (size_t c = 0; c < 4; c++)
      matrix[r][c] = other[r][c];
  return matrix;
}

[[nodiscard]]
auto process_node(Render::Context &context, TextureSamplerCache &texture_cache,
                  MeshCache &mesh_cache,
                  std::filesystem::path const &base_directory, aiNode *node,
                  const aiScene *scene) -> RenderableNodePtr {
  if (!node || !scene)
    return nullptr;
  auto drawable_node = std::make_shared<RenderableNodePtr::element_type>();
  drawable_node->name = node->mName.C_Str();
  drawable_node->model = glm_matrix(node->mTransformation);

  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    if (!mesh)
      continue;
    drawable_node->meshes.push_back(
        process_mesh(context, texture_cache, mesh_cache, base_directory, mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    RenderableNodePtr child = process_node(
        context, texture_cache, mesh_cache, base_directory, node->mChildren[i], scene);

    if (child != nullptr)
      drawable_node->children.push_back(std::move(child));
  }

  return drawable_node;
}

RenderableNodePtr load_model(Render::Context &context,
							 MeshCache &mesh_cache,
                             TextureSamplerCache &texture_cache,
                             std::filesystem::path path) {
  if (!std::filesystem::exists(path) &&
      std::filesystem::is_regular_file(path)) {
    std::cout << "FAILED Loading Model at path: " << path.string() << std::endl;
    return nullptr;
  }

  Assimp::Importer importer;
  // https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/postprocessing.html
  std::uint32_t constexpr flags =
      aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs;

  const aiScene *scene = importer.ReadFile(path, flags);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
    return nullptr;
  }

  std::filesystem::path base_directory = path.parent_path();
  RenderableNodePtr root = process_node(context, texture_cache, mesh_cache, base_directory,
                                        scene->mRootNode, scene);
  return root;
}
