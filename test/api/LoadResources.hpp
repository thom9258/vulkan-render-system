#pragma once

#include <VulkanRenderer/Context.hpp>
#include <VulkanRenderer/Vertex.hpp>

#include <filesystem>

struct Resources
{
	Resources(Resources&&) = delete;
	Resources(const Resources&) = delete;
	Resources& operator=(Resources&&) = delete;
	Resources& operator=(const Resources&) = delete;
	

	Resources(Render::Context& context,
			  std::filesystem::path assets_root);

	struct {
		Mesh mesh;
	} monkey;
	
	struct {
		TexturedMesh mesh;
		TextureSamplerReadOnly diffuse;
	} chest;
	
	struct {
		TexturedMesh textured_mesh;
		Mesh mesh;

		TextureSamplerReadOnly diffuse;
		TextureSamplerReadOnly specular;
		TextureSamplerReadOnly normal;
		TextureSamplerReadOnly glossiness;
	} smg;
	
	struct {
		TextureSamplerReadOnly diffuse;
		TextureSamplerReadOnly specular;
	} box;
	
	struct {
		TextureSamplerReadOnly diffuse;
		TextureSamplerReadOnly specular;
		TextureSamplerReadOnly normal;
	} brickwall;

	struct {
		TextureSamplerReadOnly lulu;
		TextureSamplerReadOnly statue;
	} textures;
};


Resources::Resources(Render::Context& context,
					 std::filesystem::path assets_root)
{
	std::filesystem::path models_root = assets_root / "models/";
	std::filesystem::path textures_root = assets_root / "textures/";

	auto loaded_monkey_mesh = load_obj(context,
									   assets_root,
									   "models/monkey/monkey_flat.obj");
	
	if (std::holds_alternative<Mesh>(loaded_monkey_mesh)) {
		monkey.mesh = std::move(std::get<Mesh>(loaded_monkey_mesh));
	}
	else if (std::holds_alternative<MeshWithWarning>(loaded_monkey_mesh)) {
		std::cout << "TinyOBJ Warning: " 
				  << std::get<MeshWithWarning>(loaded_monkey_mesh).warning
				  << std::endl;
		monkey.mesh = std::move(std::get<MeshWithWarning>(loaded_monkey_mesh).mesh);
	}
	else if (std::holds_alternative<MeshInvalidPath>(loaded_monkey_mesh)) {
		auto error = std::get<MeshInvalidPath>(loaded_monkey_mesh);
		throw std::runtime_error(std::string("InvalidPath: ")
								 + (error.path / error.filename).string());
	}
	else if (std::holds_alternative<MeshLoadError>(loaded_monkey_mesh)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") +
								 std::get<MeshLoadError>(loaded_monkey_mesh).msg);
	}
	
	std::cout << "loading chest model!" << std::endl;
	auto loaded_chest = load_obj_with_texcoords(context,
												assets_root,
												"models/ChestWowStyle/Chest.obj");
	if (auto p = std::get_if<TexturedMesh>(&loaded_chest)) {
		chest.mesh = std::move(*p);
	}
	else if (auto p = std::get_if<TexturedMeshWithWarning>(&loaded_chest)) {
		std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
		chest.mesh = std::move(p->mesh);
	}
	else if (auto p = std::get_if<MeshLoadError>(&loaded_chest)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
	}

	//https://opengameart.org/art-search-advanced?keys=&field_art_type_tid%5B0%5D=10&sort_by=count&sort_order=DESC&page=3
	std::cout << "loading chest texture!" << std::endl;
	chest.diffuse =
		load_bitmap(models_root / "ChestWowStyle/diffuse.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	std::cout << "loading smg model!" << std::endl;
	auto loaded_smg = load_obj(context,
							   models_root,
							   "smg/smg.obj");
	if (auto p = std::get_if<Mesh>(&loaded_smg)) {
		smg.mesh = std::move(*p);
	}
	else if (auto p = std::get_if<MeshWithWarning>(&loaded_smg)) {
		std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
		smg.mesh = std::move(p->mesh);
	}
	else if (auto p = std::get_if<MeshLoadError>(&loaded_smg)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
	}
	
	auto loaded_textured_smg = load_obj_with_texcoords(context,
													   models_root,
													   "smg/smg.obj");
	if (auto p = std::get_if<TexturedMesh>(&loaded_textured_smg)) {
		smg.textured_mesh = std::move(*p);
	}
	else if (auto p = std::get_if<TexturedMeshWithWarning>(&loaded_textured_smg)) {
		std::cout << "TinyOBJ Warning: " << p->warning << std::endl;
		smg.textured_mesh = std::move(p->mesh);
	}
	else if (auto p = std::get_if<MeshLoadError>(&loaded_textured_smg)) {
		throw std::runtime_error(std::string("TinyOBJ error: ") + p->msg);
	}

	
	std::cout << "loading smg textures!" << std::endl;

	smg.diffuse =
		load_bitmap(models_root / "smg/D.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);
	
	smg.specular =
		load_bitmap(models_root / "smg/S.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	smg.normal =
		load_bitmap(models_root / "smg/N.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	smg.glossiness =
		load_bitmap(models_root / "smg/G.tga", 
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::Yes)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	
	std::cout << "loading statue jpg!" << std::endl;
	textures.statue = 
		load_bitmap(textures_root / "texture.jpg",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Point);
	
	std::cout << "loading lulu jpg!" << std::endl;
	textures.lulu = 
		load_bitmap(textures_root / "lulu.jpg",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);
	
	box.diffuse = 
		load_bitmap(textures_root / "box/diffuse.png",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	box.specular = 
		load_bitmap(textures_root / "box/specular.png",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	
	brickwall.diffuse = 
		load_bitmap(textures_root / "brick_wall/brick_wall2-diff-2048.tga",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	brickwall.specular = 
		load_bitmap(textures_root / "brick_wall/brick_wall2-spec-2048.tga",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);

	brickwall.normal = 
		load_bitmap(textures_root / "brick_wall/brick_wall2-nor-2048.tga",
					BitmapPixelFormat::RGBA,
					VerticalFlipOnLoad::No)
		| throw_on_bitmap_error()
		| get_bitmap()
		| move_bitmap_to_gpu(&context)
		| make_shader_readonly(&context, InterpolationType::Linear);
}
