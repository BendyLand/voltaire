/**************************************************************************/
/*  test_gltf.h                                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "tests/test_macros.h"

#ifdef TOOLS_ENABLED

#include "../gltf_document.h"
#include "modules/gltf/gltf_document.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "modules/gltf/gltf_document.h"
#include "scene/resources/image_texture.h"
#include "scene/main/node.h"
#include "tests/test_utils.h"

namespace TestGltf {

static Node *gltf_import(const String &p_file) {
	// Setting up importers.
	Ref<ResourceImporterScene> import_scene;
	import_scene.instantiate("PackedScene");
	ResourceFormatImporter::add_importer(import_scene);
	Ref<EditorSceneFormatImporterGLTF> import_gltf;
	import_gltf.instantiate();
	ResourceImporterScene::add_scene_importer(import_gltf);

	// Support processing png files in editor import.
	Ref<ResourceImporterTexture> import_texture;
	import_texture.instantiate(true);
	ResourceFormatImporter::add_importer(import_texture);

	// Once editor import convert pngs to ctex, we will need to load it as ctex resource.
	Ref<ResourceFormatLoaderCompressedTexture> resource_loader_stream_texture;
	if constexpr (GD_IS_CLASS_ENABLED(CompressedTexture2D)) {
		resource_loader_stream_texture.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_stream_texture);
	}

	HashMap<String, Variant> options(21);
	options["nodes/root_type"] = "";
	options["nodes/root_name"] = "";
	options["nodes/root_script"] = Variant();
	options["nodes/apply_root_scale"] = true;
	options["nodes/root_scale"] = 1.0;
	options["meshes/ensure_tangents"] = true;
	options["meshes/generate_lods"] = false;
	options["meshes/create_shadow_meshes"] = true;
	options["meshes/light_baking"] = 1;
	options["meshes/lightmap_texel_size"] = 0.2;
	options["meshes/force_disable_compression"] = false;
	options["skins/use_named_skins"] = true;
	options["animation/import"] = true;
	options["animation/fps"] = 30;
	options["animation/trimming"] = false;
	options["animation/remove_immutable_tracks"] = true;
	options["import_script/path"] = "";
	options["extract_path"] = "res://";
	options["_subresources"] = Dictionary();
	options["gltf/naming_version"] = 1;

	// Process gltf file, note that this generates `.scn` resource from the 2nd argument.
	String scene_file = "res://" + p_file.get_file().get_basename();
	Error err = import_scene->import(0, p_file, scene_file, options, nullptr, nullptr, nullptr);
	CHECK_MESSAGE(err == OK, "GLTF import failed.");

	Ref<PackedScene> packed_scene = ResourceLoader::load(scene_file + ".scn", "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);
	CHECK_MESSAGE(err == OK, "Loading scene failed.");
	Node *p_scene = packed_scene->instantiate();

	ResourceImporterScene::remove_scene_importer(import_gltf);
	ResourceFormatImporter::remove_importer(import_texture);
	if constexpr (GD_IS_CLASS_ENABLED(CompressedTexture2D)) {
		ResourceLoader::remove_resource_format_loader(resource_loader_stream_texture);
		resource_loader_stream_texture.unref();
	}
	return p_scene;
}

static Node *gltf_export_then_import(Node *p_root, const String &p_test_name) {
	String tempfile = TestUtils::get_temp_path(p_test_name);

	Ref<GLTFDocument> doc;
	doc.instantiate();
	Ref<GLTFState> state;
	state.instantiate();
	Error err = doc->append_from_scene(p_root, state, EditorSceneFormatImporter::IMPORT_USE_NAMED_SKIN_BINDS);
	CHECK_MESSAGE(err == OK, "GLTF state generation failed.");

	err = doc->write_to_filesystem(state, tempfile + ".gltf");
	CHECK_MESSAGE(err == OK, "Writing GLTF to cache dir failed.");

	return gltf_import(tempfile + ".gltf");
}

void init(const String &p_test, const String &p_copy_target = String()) {
	Error err;

	// Setup project settings with `res://` set to a temporary path.
	String project_folder = TestUtils::get_temp_path(p_test.get_file().get_basename());
	TestProjectSettingsInternalsAccessor::resource_path() = project_folder;
	err = ProjectSettings::setup(project_folder, String(), true);

	// Create the imported files folder as the editor import process expects it to exist.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	da->make_dir_recursive(ProjectSettings::globalize_path(ProjectSettings::get_imported_files_path()));

	if (p_copy_target.is_empty()) {
		return;
	}

	// Copy all the necessary test data files to the res:// directory.
	String test_data = String("modules/gltf/tests/data/").path_join(p_test);
	da = DirAccess::open(test_data);
	CHECK_MESSAGE(da.is_valid(), "Unable to open folder.");
	da->list_dir_begin();
	for (String item = da->get_next(); !item.is_empty(); item = da->get_next()) {
		if (!FileAccess::exists(test_data.path_join(item))) {
			continue;
		}
		Ref<FileAccess> output = FileAccess::open(p_copy_target.path_join(item), FileAccess::WRITE, &err);
		CHECK_MESSAGE(err == OK, "Unable to open output file.");
		output->store_buffer(FileAccess::get_file_as_bytes(test_data.path_join(item)));
		output->close();
	}
	da->list_dir_end();
}

} //namespace TestGltf

#endif // TOOLS_ENABLED

// --- Folded from modules/gltf/tests/test_gltf_emissive.h ---


#ifdef TOOLS_ENABLED

namespace TestGltf {

TEST_CASE("[SceneTree][Node] GLTF emissiveTexture without emissiveFactor uses white emission") {
	init("gltf_emissive_no_factor", "res://");

	Node *loaded = gltf_import("res://emissive_no_factor.gltf");
	CHECK_MESSAGE(loaded != nullptr, "Failed to load GLB.");

	MeshInstance3D *mesh = dynamic_cast<MeshInstance3D *>(loaded->find_child("Cube", true, true));
	CHECK_MESSAGE(mesh != nullptr, "Mesh not found.");

	Ref<StandardMaterial3D> mat = mesh->get_active_material(0);
	CHECK_MESSAGE(mat.is_valid(), "Material not found.");

	// Emission should be enabled.
	CHECK(mat->get_feature(StandardMaterial3D::FEATURE_EMISSION));

	// Emission operator should be MULTIPLY per glTF spec.
	CHECK(mat->get_emission_operator() == StandardMaterial3D::EMISSION_OP_MULTIPLY);

	// Without emissiveFactor, emission color should be WHITE, not BLACK.
	Color c = mat->get_emission();
	CHECK_MESSAGE(c.r > 0.9f, "Emission red should be ~1.0 when emissiveFactor is absent.");
	CHECK_MESSAGE(c.g > 0.9f, "Emission green should be ~1.0 when emissiveFactor is absent.");
	CHECK_MESSAGE(c.b > 0.9f, "Emission blue should be ~1.0 when emissiveFactor is absent.");

	memdelete(loaded);
}

} // namespace TestGltf

#endif // TOOLS_ENABLED
// --- End modules/gltf/tests/test_gltf_emissive.h ---

// --- Folded from modules/gltf/tests/test_gltf_extras.h ---


#ifdef TOOLS_ENABLED

#include "modules/gltf/gltf_document.h"

#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "tests/test_macros.h"

namespace TestGltf {

TEST_CASE("[SceneTree][Node] GLTF test mesh and material meta export and import") {
	init("gltf_mesh_material_extras");
	// Setup scene.
	Ref<StandardMaterial3D> original_material = memnew(StandardMaterial3D);
	original_material->set_albedo(Color(1.0, .0, .0));
	original_material->set_name("material");
	Dictionary material_dict;
	material_dict["node_type"] = "material";
	original_material->set_meta("extras", material_dict);

	Ref<PrimitiveMesh> original_meshdata = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
	original_meshdata->set_name("planemesh");
	Dictionary meshdata_dict;
	meshdata_dict["node_type"] = "planemesh";
	original_meshdata->set_meta("extras", meshdata_dict);
	original_meshdata->surface_set_material(0, original_material);

	MeshInstance3D *original_mesh_instance = memnew(MeshInstance3D);
	original_mesh_instance->set_mesh(original_meshdata);
	original_mesh_instance->set_name("mesh_instance_3d");
	Dictionary mesh_instance_dict;
	mesh_instance_dict["node_type"] = "mesh_instance_3d";
	original_mesh_instance->set_meta("extras", mesh_instance_dict);

	Node3D *original = memnew(Node3D);
	SceneTree::get_root()->add_child(original);
	original->add_child(original_mesh_instance);
	original->set_name("node3d");
	Dictionary node_dict;
	node_dict["node_type"] = "node3d";
	original->set_meta("extras", node_dict);
	original->set_meta("meta_not_nested_under_extras", "should not propagate");

	original->set_owner(SceneTree::get_root());
	original_mesh_instance->set_owner(SceneTree::get_root());

	// Convert to GLFT and back.
	Node *loaded = gltf_export_then_import(original, "gltf_extras");

	// Compare the results.
	CHECK(loaded->get_name() == "node3d");
	CHECK(Dictionary(loaded->get_meta("extras")).size() == 1);
	CHECK(Dictionary(loaded->get_meta("extras"))["node_type"] == "node3d");
	CHECK_FALSE(loaded->has_meta("meta_not_nested_under_extras"));
	CHECK_FALSE(Dictionary(loaded->get_meta("extras")).has("meta_not_nested_under_extras"));

	MeshInstance3D *mesh_instance_3d = Object::cast_to<MeshInstance3D>(loaded->find_child("mesh_instance_3d", false, true));
	CHECK(mesh_instance_3d->get_name() == "mesh_instance_3d");
	CHECK(Dictionary(mesh_instance_3d->get_meta("extras"))["node_type"] == "mesh_instance_3d");

	Ref<Mesh> mesh = mesh_instance_3d->get_mesh();
	CHECK(Dictionary(mesh->get_meta("extras"))["node_type"] == "planemesh");

	Ref<Material> material = mesh->surface_get_material(0);
	CHECK(material->get_name() == "material");
	CHECK(Dictionary(material->get_meta("extras"))["node_type"] == "material");

	memdelete(original_mesh_instance);
	memdelete(original);
	memdelete(loaded);
}

TEST_CASE("[SceneTree][Node] GLTF test skeleton and bone export and import") {
	init("gltf_skeleton_extras");
	// Setup scene.
	Skeleton3D *skeleton = memnew(Skeleton3D);
	skeleton->set_name("skeleton");
	Dictionary skeleton_extras;
	skeleton_extras["node_type"] = "skeleton";
	skeleton->set_meta("extras", skeleton_extras);

	skeleton->add_bone("parent");
	skeleton->set_bone_rest(0, Transform3D());
	Dictionary parent_bone_extras;
	parent_bone_extras["bone"] = "i_am_parent_bone";
	skeleton->set_bone_meta(0, "extras", parent_bone_extras);

	skeleton->add_bone("child");
	skeleton->set_bone_rest(1, Transform3D());
	skeleton->set_bone_parent(1, 0);
	Dictionary child_bone_extras;
	child_bone_extras["bone"] = "i_am_child_bone";
	skeleton->set_bone_meta(1, "extras", child_bone_extras);

	// We have to have a mesh to link with skeleton or it will not get imported.
	Ref<PrimitiveMesh> meshdata = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
	meshdata->set_name("planemesh");

	MeshInstance3D *mesh = memnew(MeshInstance3D);
	mesh->set_mesh(meshdata);
	mesh->set_name("mesh_instance_3d");

	Node3D *original = memnew(Node3D);
	SceneTree::get_root()->add_child(original);
	original->add_child(skeleton);
	original->add_child(mesh);
	original->set_name("node3d");

	// Now that both skeleton and mesh are part of scene, link them.
	mesh->set_skeleton_path(mesh->get_path_to(skeleton));

	mesh->set_owner(SceneTree::get_root());
	original->set_owner(SceneTree::get_root());

	// Convert to GLFT and back.
	Node *loaded = gltf_export_then_import(original, "gltf_bone_extras");

	// Compare the results.
	CHECK(loaded->get_name() == "node3d");
	Skeleton3D *result = Object::cast_to<Skeleton3D>(loaded->find_child("Skeleton3D", false, true));
	CHECK(result->get_bone_name(0) == "parent");
	CHECK(Dictionary(result->get_bone_meta(0, "extras"))["bone"] == "i_am_parent_bone");
	CHECK(result->get_bone_name(1) == "child");
	CHECK(Dictionary(result->get_bone_meta(1, "extras"))["bone"] == "i_am_child_bone");

	memdelete(skeleton);
	memdelete(mesh);
	memdelete(original);
	memdelete(loaded);
}
} //namespace TestGltf

#endif // TOOLS_ENABLED
// --- End modules/gltf/tests/test_gltf_extras.h ---

// --- Folded from modules/gltf/tests/test_gltf_images.h ---
#ifdef TOOLS_ENABLED



#include "editor/settings/editor_settings.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "scene/resources/image_texture.h"

namespace TestGltf {
Ref<Texture2D> _check_texture(Node *p_node) {
	MeshInstance3D *mesh_instance_3d = Object::cast_to<MeshInstance3D>(p_node->find_child("mesh_instance_3d", true, true));
	Ref<StandardMaterial3D> material = mesh_instance_3d->get_active_material(0);
	Ref<Texture2D> texture = material->get_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO);

	CHECK_MESSAGE(texture->get_size().x == 2, "Texture width not correct.");
	CHECK_MESSAGE(texture->get_size().y == 2, "Texture height not correct.");

	// Check if the loaded texture pixels are exactly as we expect.
	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			Color c = texture->get_image()->get_pixel(x, y);
			CHECK_MESSAGE(c == Color(x, y, y), "Texture content is incorrect.");
		}
	}
	return texture;
}

TEST_CASE("[SceneTree][Node] Export GLTF with external texture and import") {
	init("gltf_images_external_export_import");
	// Setup scene.
	Ref<ImageTexture> original_texture;
	original_texture.instantiate();
	Ref<Image> image;
	image.instantiate();
	image->initialize_data(2, 2, false, Image::FORMAT_RGBA8);
	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			image->set_pixel(x, y, Color(x, y, y));
		}
	}

	original_texture->set_image(image);

	Ref<StandardMaterial3D> original_material;
	original_material.instantiate();
	original_material->set_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO, original_texture);
	original_material->set_name("material");

	Ref<PrimitiveMesh> original_meshdata;
	original_meshdata.instantiate(PrimitiveMesh::MESH_PLANE);
	original_meshdata->set_name("planemesh");
	original_meshdata->surface_set_material(0, original_material);

	MeshInstance3D *original_mesh_instance = memnew(MeshInstance3D);
	original_mesh_instance->set_mesh(original_meshdata);
	original_mesh_instance->set_name("mesh_instance_3d");

	Node3D *original = memnew(Node3D);
	SceneTree::get_root()->add_child(original);
	original->add_child(original_mesh_instance);
	original->set_owner(SceneTree::get_root());
	original_mesh_instance->set_owner(SceneTree::get_root());

	// Convert to GLFT and back.
	Node *loaded = gltf_export_then_import(original, "gltf_images");
	_check_texture(loaded);

	memdelete(original_mesh_instance);
	memdelete(original);
	memdelete(loaded);
}

TEST_CASE("[SceneTree][Node][Editor] Import GLTF from .godot/imported folder with external texture") {
	init("gltf_placed_in_dot_godot_imported", "res://.godot/imported");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://.godot/imported/gltf_placed_in_dot_godot_imported.gltf");
	Ref<Texture2D> texture = _check_texture(loaded);
	ERR_PRINT_ON

	// In-editor imports of gltf and texture from .godot/imported folder should end up in res:// if extract_path is defined.
	CHECK_MESSAGE(texture->get_path() == "res://gltf_placed_in_dot_godot_imported_material_albedo000.png", "Texture not parsed as resource.");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

TEST_CASE("[SceneTree][Node][Editor] Import GLTF with texture outside of res:// directory") {
	init("gltf_pointing_to_texture_outside_of_res_folder", "res://");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	// Copy texture to the parent folder of res:// - i.e. to res://.. where we can't import from.
	String oneup = TestUtils::get_temp_path("texture.png");
	Error err;
	Ref<FileAccess> output = FileAccess::open(oneup, FileAccess::WRITE, &err);
	CHECK_MESSAGE(err == OK, "Unable to open texture file.");
	output->store_buffer(FileAccess::get_file_as_bytes("res://texture_source.png"));
	output->close();

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://gltf_pointing_to_texture_outside_of_res_folder.gltf");
	Ref<Texture2D> texture = _check_texture(loaded);
	ERR_PRINT_ON

	// Imports of gltf with texture from outside of res:// folder should end up being copied to res://
	CHECK_MESSAGE(texture->get_path() == "res://gltf_pointing_to_texture_outside_of_res_folder_material_albedo000.png", "Texture not parsed as resource.");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

TEST_CASE("[SceneTree][Node][Editor] Import GLTF with embedded texture, check how it got extracted") {
	init("gltf_embedded_texture", "res://");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://embedded_texture.gltf");
	Ref<Texture2D> texture = _check_texture(loaded);
	ERR_PRINT_ON

	// In-editor imports of texture embedded in file should end up with a resource.
	CHECK_MESSAGE(texture->get_path() == "res://embedded_texture_material_albedo000.png", "Texture not parsed as resource.");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

} //namespace TestGltf

#endif // TOOLS_ENABLED
// --- End modules/gltf/tests/test_gltf_images.h ---
