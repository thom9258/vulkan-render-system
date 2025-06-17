#include <VulkanRenderer/Mesh.hpp>

#include "VertexImpl.hpp"
#include "VertexBufferImpl.hpp"
#include "ContextImpl.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <VulkanRenderer/tiny_obj_loader.hpp>

auto load_obj(Render::Context& context,
			  const std::filesystem::path& path,
			  const std::string& filename)
	-> LoadMeshResult
{
    //attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib;
    //shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes;
    //materials contains the information about the material of each shape, but we won't use it.
    std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!std::filesystem::exists(path / filename)) {
		return MeshInvalidPath{path, filename};
	}

	tinyobj::LoadObj(&attrib,
					 &shapes,
					 &materials,
					 &warn,
					 &err,
					 (path / filename).string().c_str(),
					 path.string().c_str());

	if (!err.empty())
		return MeshLoadError{err};

	std::vector<VertexPosNormColor> vertices{};

	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            //hardcode loading to triangles
			size_t fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                //vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                //vertex normal
            	tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                //copy it into our vertex
				VertexPosNormColor vertex{};
				vertex.pos.x = vx;
				vertex.pos.y = vy;
				vertex.pos.z = vz;
				vertex.norm.x = nx;
				vertex.norm.y = ny;
                vertex.norm.z = nz;
                vertex.color = vertex.norm;
				vertices.push_back(vertex);
			}
			index_offset += fv;
		}
	}
	
	auto buffer = VertexBuffer::create<VertexPosNormColor>(context, vertices);
	Mesh mesh{std::move(buffer)};
	if (!warn.empty()) {
		return MeshWithWarning{std::move(mesh), warn};
	}
	
	return mesh;
}


auto load_obj_with_texcoords(Render::Context& context,
							 const std::filesystem::path& path,
							 const std::string& filename)
	-> LoadTexturedMeshResult
{
    //attrib will contain the vertex arrays of the file
	tinyobj::attrib_t attrib;
    //shapes contains the info for each separate object in the file
	std::vector<tinyobj::shape_t> shapes;
    //materials contains the information about the material of each shape, but we won't use it.
    std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;
	if (!std::filesystem::exists(path / filename)) {
		return MeshInvalidPath{path, filename};
	}

	tinyobj::LoadObj(&attrib,
					 &shapes,
					 &materials,
					 &warn,
					 &err,
					 (path / filename).string().c_str(),
					 path.string().c_str());

	if (!err.empty())
		return MeshLoadError{err};

	std::vector<VertexPosNormColorUV> vertices{};

	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            //hardcode loading to triangles
			size_t fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                //vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                //vertex normal
            	tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

				tinyobj::real_t ux = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t uy = attrib.texcoords[2 * idx.texcoord_index + 1];

                //copy it into our vertex
				VertexPosNormColorUV vertex{};
				vertex.pos.x = vx;
				vertex.pos.y = vy;
				vertex.pos.z = vz;
				vertex.norm.x = nx;
				vertex.norm.y = ny;
                vertex.norm.z = nz;
				vertex.uv.x = ux;
				vertex.uv.y = uy;
                vertex.color = vertex.norm;
				vertices.push_back(vertex);
			}
			index_offset += fv;
		}
	}
	
	auto buffer = VertexBuffer::create<VertexPosNormColorUV>(context, vertices);
	TexturedMesh mesh{std::move(buffer)};
	if (!warn.empty()) {
		return TexturedMeshWithWarning{std::move(mesh), warn};
	}
	
	return mesh;
}
