#include <VulkanRenderer/ModelLoader.hpp>

#include "VertexImpl.hpp"
#include "VertexBufferImpl.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


//TODO: this is kinda dirty and a waste of memory, we should add proper support
//      for indexed vertice buffers instead..
auto unindex_vertices(std::vector<VertexPosNormColorUV> vertices,
					  std::vector<std::uint32_t> indices)
	-> std::vector<VertexPosNormColorUV>
{
	std::vector<VertexPosNormColorUV> unindexed;
	unindexed.reserve(indices.size());
	
	for (std::uint32_t index: indices)
		unindexed.push_back(vertices.at(index));
	return unindexed;
}

void loadMaterialTextures(Render::Context& context,
						  RenderableTree::MaterialMap& materials,
						  aiMaterial* mat,
						  aiTextureType type,
						  string typeName)
{
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        Texture texture;
        texture.id = TextureFromFile(str.C_Str(), directory);
        texture.type = typeName;
        texture.path = str;
        textures.push_back(texture);
    }
    return textures;
}  



auto process_mesh(Render::Context& context,
						  RenderableTree::MaterialMap& materials,
						  aiMesh* mesh,
						  const aiScene* scene)
	-> RenderableTree::Node::MaterialMesh
{
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
		}
		else {
			vertex.uv = glm::vec2(0.0f);
		}

		vertices.push_back(vertex);
	}
	
	std::vector<std::uint32_t> indices;
	for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for(unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	std::vector<VertexPosNormColorUV> unindexed = unindex_vertices(vertices, indices);
	


	if (mesh->mMaterialIndex >= 0) {
		std::cout << "MODEL HAS MATERIAL" << std::endl;

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		vector<Texture> diffuseMaps = loadMaterialTextures(material, 
														   aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		vector<Texture> specularMaps = loadMaterialTextures(material, 
															aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	}  


	
	RenderableTree::Node::MaterialMesh drawable_mesh{
		VertexBuffer::create<VertexPosNormColorUV>(context, unindexed),
		""};
    return drawable_mesh;
}

glm::mat4 glm_matrix(aiMatrix4x4 other)
{
	glm::mat4 matrix;
	for (size_t r = 0; r < 4; r++)
		for (size_t c = 0; c < 4; c++)
			matrix[r][c] = other[r][c];
	return matrix;
}


[[nodiscard]]
auto process_node(Render::Context& context,
				  RenderableTree::MaterialMap& materials,
				  aiNode* node,
				  const aiScene* scene)
	-> std::shared_ptr<RenderableTree::Node>
{
	if (!node || !scene) return nullptr;
	auto drawable_node = std::make_shared<RenderableTree::Node>();
	drawable_node->name = node->mName.C_Str();
	drawable_node->model = glm_matrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		if (!mesh) continue;
        drawable_node->meshes.push_back(process_mesh(context,
													 materials,
													 mesh,
													 scene));			
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
		std::shared_ptr<RenderableTree::Node> child = process_node(context,
																   materials,
																   node->mChildren[i],
																   scene);
		if (child != nullptr)
			drawable_node->children.push_back(std::move(child));
    }
	
	return drawable_node;
}

std::optional<RenderableTree> load_model(Render::Context& context, std::filesystem::path path)
{
	if (!std::filesystem::exists(path)) {
		std::cout << "FAILED Loading Model at path: " << path.string() << std::endl;
		return std::nullopt;
	}

	Assimp::Importer importer;
	// https://the-asset-importer-lib-documentation.readthedocs.io/en/latest/usage/postprocessing.html
	std::uint32_t constexpr flags = 
		aiProcess_Triangulate 
		| aiProcess_GenNormals
		| aiProcess_FlipUVs;

	const aiScene* scene = importer.ReadFile(path, flags);
	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return std::nullopt;
    }

	RenderableTree tree;
	tree.root = process_node(context,
							 tree.materials,
							 scene->mRootNode,
							 scene);
	return tree;
}
