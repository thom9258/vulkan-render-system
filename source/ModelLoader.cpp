#include <VulkanRenderer/ModelLoader.hpp>

#include "MeshCache.hpp"
#include "ShaderTexture.hpp"
#include "Vertex.hpp"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// TODO: this is kinda dirty and a waste of memory, we should add proper support
//       for indexed vertice buffers instead..
#if 0
auto unindex_vertices(std::vector<VertexPosNormColorUV> vertices,
                      std::vector<std::uint32_t> indices)
    -> std::vector<VertexPosNormColorUV> {
  std::vector<VertexPosNormColorUV> unindexed;
  unindexed.reserve(indices.size());

  for (std::uint32_t index : indices)
    unindexed.push_back(vertices.at(index));
  return unindexed;
}
#endif

template <typename TVertex>
auto unindex_vertices(std::span<TVertex> vertices,
                      std::span<std::uint32_t> indices)
    -> std::vector<TVertex> {
  std::vector<TVertex> unindexed;
  unindexed.reserve(indices.size());

  for (std::uint32_t index : indices)
    unindexed.push_back(vertices[index]);
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
                  MeshCache &mesh_cache,
                  std::filesystem::path const &base_directory, aiMesh *mesh,
                  const aiScene *scene) -> RenderableNode::SimpleModel {
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
      unindex_vertices<VertexPosNormColorUV>(vertices, indices);

  RenderableNode::SimpleModel drawable_mesh;
  drawable_mesh.mesh = mesh_cache.add(
      context, TexturedMesh{VertexBuffer::create<VertexPosNormColorUV>(
                   context, unindexed)});

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

namespace assimp_to_glm {

glm::mat4 matrix(aiMatrix4x4 other) {
  glm::mat4 matrix;
  for (size_t r = 0; r < 4; r++)
    for (size_t c = 0; c < 4; c++)
      matrix[r][c] = other[r][c];
  return matrix;
}

glm::vec3 vec3(aiVector3f other) {
  glm::vec3 v;
  v[0] = other.x;
  v[1] = other.y;
  v[2] = other.z;
  return v;
}

glm::quat quat(aiQuaternion other) {
  glm::quat q;
  q.w = other.w;
  q.x = other.x;
  q.y = other.y;
  q.z = other.z;
  return q;
}

} // namespace assimp_to_glm

[[nodiscard]]
auto process_node(Render::Context &context, TextureSamplerCache &texture_cache,
                  MeshCache &mesh_cache,
                  std::filesystem::path const &base_directory, aiNode *node,
                  const aiScene *scene) -> RenderableNodePtr {
  if (!node || !scene)
    return nullptr;
  auto drawable_node = std::make_shared<RenderableNodePtr::element_type>();
  drawable_node->name = node->mName.C_Str();
  drawable_node->model_matrix = assimp_to_glm::matrix(node->mTransformation);

  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    if (!mesh)
      continue;
    drawable_node->models.push_back(process_mesh(
        context, texture_cache, mesh_cache, base_directory, mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    RenderableNodePtr child =
        process_node(context, texture_cache, mesh_cache, base_directory,
                     node->mChildren[i], scene);

    if (child != nullptr)
      drawable_node->children.push_back(std::move(child));
  }

  return drawable_node;
}

RenderableNodePtr load_model(Render::Context &context, MeshCache &mesh_cache,
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
  RenderableNodePtr root =
      process_node(context, texture_cache, mesh_cache, base_directory,
                   scene->mRootNode, scene);
  return root;
}

Bone create_bone(const std::string &name, int ID, const aiNodeAnim *channel) {
  Bone bone(name, ID);

  bone.m_NumPositions = channel->mNumPositionKeys;

  for (int positionIndex = 0; positionIndex < bone.m_NumPositions;
       ++positionIndex) {
    aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
    float timeStamp = channel->mPositionKeys[positionIndex].mTime;
    KeyPosition data;
    data.position = assimp_to_glm::vec3(aiPosition);
    data.timeStamp = timeStamp;
    bone.m_Positions.push_back(data);
  }

  bone.m_NumRotations = channel->mNumRotationKeys;
  for (int rotationIndex = 0; rotationIndex < bone.m_NumRotations;
       ++rotationIndex) {
    aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
    float timeStamp = channel->mRotationKeys[rotationIndex].mTime;
    KeyRotation data;
    data.orientation = assimp_to_glm::quat(aiOrientation);
    data.timeStamp = timeStamp;
    bone.m_Rotations.push_back(data);
  }

  bone.m_NumScalings = channel->mNumScalingKeys;
  for (int keyIndex = 0; keyIndex < bone.m_NumScalings; ++keyIndex) {
    aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
    float timeStamp = channel->mScalingKeys[keyIndex].mTime;
    KeyScale data;
    data.scale = assimp_to_glm::vec3(scale);
    data.timeStamp = timeStamp;
    bone.m_Scales.push_back(data);
  }

  return bone;
}

void ReadMissingBones(Animation &animation, BoneInfos &bone_infos,
                      const aiAnimation *ai_animation) {
  // reading channels(bones engaged in an ai_animation and their keyframes)
  for (int i = 0; i < ai_animation->mNumChannels; i++) {
    auto channel = ai_animation->mChannels[i];
    std::string boneName = channel->mNodeName.data;

    if (!bone_infos.find_bone_id(boneName)) {
      // NOTE for some reason we insert invalid bones IF
      // they are mentioned but not used

      bone_infos.insert_bone(boneName, glm::mat4(1.0f));
    }

    Bone bone =
        create_bone(boneName, bone_infos.find_bone_id(boneName).value(), channel);
    animation.m_Bones.push_back(bone);
  }

  animation.m_BoneInfoMap = bone_infos.get();
}

void ReadHeirarchyData(Animation &animation, AssimpNodeData &dest,
                       const aiNode *src) {
  assert(src);

  dest.name = src->mName.data;
  dest.transformation =
      assimp_to_glm::matrix(src->mTransformation);
  dest.childrenCount = src->mNumChildren;

  for (int i = 0; i < src->mNumChildren; i++) {
    AssimpNodeData newData;
    ReadHeirarchyData(animation, newData, src->mChildren[i]);
    dest.children.push_back(newData);
  }
}

auto create_animations(const aiScene *scene, BoneInfos& bone_infos)
    -> std::vector<Animation> {
  if (!scene->HasAnimations())
    return {};

  std::vector<Animation> animations;
  for (int i = 0; i < scene->mNumAnimations; i++) {
    auto ai_animation = scene->mAnimations[i];
    Animation animation;
    animation.m_Duration = ai_animation->mDuration;
    animation.m_TicksPerSecond = ai_animation->mTicksPerSecond;
    ReadHeirarchyData(animation, animation.m_RootNode, scene->mRootNode);
    ReadMissingBones(animation, bone_infos, ai_animation);
  }

  return animations;
}

auto process_animated_mesh(Render::Context &context,
                           TextureSamplerCache &texture_cache,
                           MeshCache &mesh_cache,
                           std::filesystem::path const &base_directory,
						   BoneInfos& bone_infos, aiMesh *mesh,
                           const aiScene *scene)
    -> RenderableNode::AnimatedModel {
  std::vector<VertexAnimatedPosNormColorUV> vertices{};
  for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
    VertexAnimatedPosNormColorUV vertex;
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

  for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
    std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
    std::optional<int> boneID;
    if (!bone_infos.has_bone(boneName)) {
      boneID = bone_infos.insert_bone(
          boneName,
          assimp_to_glm::matrix(mesh->mBones[boneIndex]->mOffsetMatrix));
    } else {
      boneID = bone_infos.find_bone_id(boneName);
    }

    assert(boneID != std::nullopt);
    auto weights = mesh->mBones[boneIndex]->mWeights;
    int numWeights = mesh->mBones[boneIndex]->mNumWeights;

    for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
      int vertexId = weights[weightIndex].mVertexId;
      float weight = weights[weightIndex].mWeight;
      assert(vertexId <= vertices.size());

      vertices[vertexId].bone_ids[boneIndex] = boneID.value();
      vertices[vertexId].weights[boneIndex] = weight;
    }
  }

  std::vector<std::uint32_t> indices;
  for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
    aiFace face = mesh->mFaces[i];
    for (unsigned int j = 0; j < face.mNumIndices; j++) {
      indices.push_back(face.mIndices[j]);
    }
  }

  std::vector<VertexAnimatedPosNormColorUV> unindexed =
      unindex_vertices<VertexAnimatedPosNormColorUV>(vertices, indices);

  RenderableNode::AnimatedModel drawable_mesh;
  drawable_mesh.mesh = mesh_cache.add(
      context, AnimatedMesh{VertexBuffer::create<VertexAnimatedPosNormColorUV>(
                                context, unindexed),
                            bone_infos});

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

[[nodiscard]]
auto process_animated_node(Render::Context &context,
                           TextureSamplerCache &texture_cache,
                           MeshCache &mesh_cache,
                           std::filesystem::path const &base_directory,
                           BoneInfos& bone_infos, aiNode *node,
                           const aiScene *scene) -> RenderableNodePtr {
  if (!node || !scene)
    return nullptr;
  auto drawable_node = std::make_shared<RenderableNodePtr::element_type>();
  drawable_node->name = node->mName.C_Str();
  drawable_node->model_matrix = assimp_to_glm::matrix(node->mTransformation);

  for (unsigned int i = 0; i < node->mNumMeshes; i++) {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    if (!mesh)
      continue;
    drawable_node->models.push_back(
        process_animated_mesh(context, texture_cache, mesh_cache,
                              base_directory, bone_infos, mesh, scene));
  }

  for (unsigned int i = 0; i < node->mNumChildren; i++) {
    RenderableNodePtr child = process_animated_node(
        context, texture_cache, mesh_cache, base_directory, bone_infos,
        node->mChildren[i], scene);

    if (child != nullptr)
      drawable_node->children.push_back(std::move(child));
  }

  return drawable_node;
}

auto load_animated_model(Render::Context &context, MeshCache &mesh_cache,
                         TextureSamplerCache &texture_cache,
                         std::filesystem::path path)
    -> std::expected<LoadedAnimatedModel, std::string> {
  if (!std::filesystem::exists(path) &&
      std::filesystem::is_regular_file(path)) {
    return std::unexpected(
        std::format("FAILED Loading Model at path: {}", path.string()));
  }

  Assimp::Importer importer;
  // https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/postprocessing.html
  std::uint32_t constexpr flags =
      aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs;

  const aiScene *scene = importer.ReadFile(path, flags);
  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
      !scene->mRootNode) {
    return std::unexpected(
        std::format("ERROR::ASSIMP::{}", importer.GetErrorString()));
  }

  LoadedAnimatedModel loaded_animated_model;
  std::filesystem::path base_directory = path.parent_path();

  loaded_animated_model.renderable = process_animated_node(
      context, texture_cache, mesh_cache, base_directory,
      loaded_animated_model.bone_infos, scene->mRootNode, scene);

  loaded_animated_model.animations = create_animations(scene, loaded_animated_model.bone_infos);

  return loaded_animated_model;
}
