/**************************************************************************/
/*  resource_importer_scene.cpp                                           */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/error/error_macros.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/import/3d/scene_import_settings.h"
#include "editor/settings/editor_settings.h"
#include "resource_importer_scene.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/navigation/navigation_region_3d.h"
#include "scene/3d/occluder_instance_3d.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/3d/physics/vehicle_body_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/3d/separation_ray_shape_3d.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/3d/world_boundary_shape_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/bone_map.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/physics_material.h"
#include "scene/resources/resource_format_text.h"

void EditorSceneFormatImporter::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

/////////////////////////////////
void EditorScenePostImport::_bind_methods() {}

String EditorScenePostImport::get_source_file() const { return source_file; }

void EditorScenePostImport::init(const String& p_source_file) { source_file = p_source_file; }

///////////////////////////////////////////////////////

void EditorScenePostImportPlugin::get_internal_import_options(
	InternalImportCategory p_category, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

void EditorScenePostImportPlugin::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

/////////////////////////////////////////////////////////

const String ResourceImporterScene::material_extension[3] = {".tres", ".res", ".material"};

String ResourceImporterScene::get_importer_name() const
{
	// For compatibility with 4.2 and earlier we need to keep the "scene" and "animation_library"
	// names. However this is arbitrary so for new import types we can use any string.
	if (_scene_import_type == "PackedScene") {
		return "scene";
	}
	else if (_scene_import_type == "AnimationLibrary") {
		return "animation_library";
	}
	return _scene_import_type;
}

String ResourceImporterScene::get_visible_name() const
{
	// This is displayed on the UI. Friendly names here are nice but not vital, so fall back to the
	// type.
	if (_scene_import_type == "PackedScene") {
		return "Scene";
	}
	else if (_scene_import_type == "ArrayMesh") {
		return "Single Mesh";
	}
	return _scene_import_type.capitalize();
}

void ResourceImporterScene::get_recognized_extensions(List<String>* p_extensions) const
{
	get_scene_importer_extensions(p_extensions);
}

String ResourceImporterScene::get_save_extension() const
{
	if (_scene_import_type == "PackedScene") {
		return "scn";
	}
	return "res";
}

String ResourceImporterScene::get_resource_type() const { return _scene_import_type; }

int ResourceImporterScene::get_format_version() const { return 1; }

int ResourceImporterScene::get_preset_count() const { return 0; }

String ResourceImporterScene::get_preset_name(int p_idx) const { return String(); }

static bool _teststr(const String& p_what, const String& p_str)
{
	String what = p_what;

	// Remove trailing spaces and numbers, some apps like blender add ".number" to duplicates
	// (dot is replaced with _ as invalid character) so also compensate for this.
	while (what.length() && (is_digit(what[what.length() - 1]) || what[what.length() - 1] <= 32 ||
								what[what.length() - 1] == '_')) {
		what = what.substr(0, what.length() - 1);
	}

	if (what.containsn("$" + p_str)) { // Blender and other stuff.
		return true;
	}
	if (what.to_lower().ends_with(
			"-" + p_str)) { // collada only supports "_" and "-" besides letters
		return true;
	}
	if (what.to_lower().ends_with(
			"_" + p_str)) { // collada only supports "_" and "-" besides letters
		return true;
	}
	return false;
}

static String _fixstr(const String& p_what, const String& p_str)
{
	String what = p_what;

	// Remove trailing spaces and numbers, some apps like blender add ".number" to duplicates
	// (dot is replaced with _ as invalid character) so also compensate for this.
	while (what.length() && (is_digit(what[what.length() - 1]) || what[what.length() - 1] <= 32 ||
								what[what.length() - 1] == '_')) {
		what = what.substr(0, what.length() - 1);
	}

	String end = p_what.substr(what.length());

	if (what.containsn("$" + p_str)) { // Blender and other stuff.
		return what.replace("$" + p_str, "") + end;
	}
	if (what.to_lower().ends_with(
			"-" + p_str)) { // collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	if (what.to_lower().ends_with(
			"_" + p_str)) { // collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	return what;
}

static void _pre_gen_shape_list(
	Ref<ImporterMesh>& mesh, Vector<Ref<Shape3D>>& r_shape_list, bool p_convex)
{
	ERR_FAIL_COND_MSG(mesh.is_null(), "Cannot generate shape list with null mesh value.");
	if (!p_convex) {
		Ref<ConcavePolygonShape3D> shape = mesh->create_trimesh_shape();
		r_shape_list.push_back(shape);
	}
	else {
		Vector<Ref<Shape3D>> cd;
		cd.push_back(mesh->create_convex_shape(true,
			/*Passing false, otherwise VHACD will be used to simplify (Decompose) the Mesh.*/
			false));
		if (cd.size()) {
			for (int i = 0; i < cd.size(); i++) {
				r_shape_list.push_back(cd[i]);
			}
		}
	}
}

struct ScalableNodeCollection
{
	HashSet<Node3D*> node_3ds;
	HashSet<Ref<ImporterMesh>> importer_meshes;
	HashSet<Ref<Skin>> skins;
	HashSet<Ref<Animation>> animations;
};

void _rescale_skin(Vector3 p_scale, Ref<Skin> p_skin)
{
	// MESH and SKIN data divide, to compensate for object position multiplying.
	for (int i = 0; i < p_skin->get_bind_count(); i++) {
		Transform3D transform = p_skin->get_bind_pose(i);
		p_skin->set_bind_pose(i, Transform3D(transform.basis, p_scale * transform.origin));
	}
}

Ref<Animation> ResourceImporterScene::_save_animation_to_file(Ref<Animation> anim,
	bool p_save_to_file, const String& p_save_to_path, bool p_keep_custom_tracks)
{
	String res_path = ResourceUID::ensure_path(p_save_to_path);
	if (!p_save_to_file || !res_path.is_resource_file()) {
		return anim;
	}

	if (FileAccess::exists(res_path) && p_keep_custom_tracks) {
		// Copy custom animation tracks from previously imported files.
		Ref<Animation> old_anim =
			ResourceLoader::load(res_path, "Animation", ResourceFormatLoader::CACHE_MODE_IGNORE);
		if (old_anim.is_valid()) {
			for (int i = 0; i < old_anim->get_track_count(); i++) {
				if (!old_anim->track_is_imported(i)) {
					old_anim->copy_track(i, anim);
				}
			}
			anim->set_loop_mode(old_anim->get_loop_mode());
		}
	}

	if (ResourceCache::has(res_path)) {
		Ref<Animation> old_anim = ResourceCache::get_ref(res_path);
		if (old_anim.is_valid()) {
			old_anim->copy_from(anim);
			anim = old_anim;
		}
	}
	anim->set_path(res_path, true); // Set path to save externally.
	Error err = ResourceSaver::save(anim.ptr(), res_path, ResourceSaver::FLAG_CHANGE_PATH);

	ERR_FAIL_COND_V_MSG(err != OK, anim, "Saving of animation failed: " + res_path);
	if (p_save_to_path.begins_with("uid://")) {
		// slow
		ResourceSaver::set_uid(res_path, ResourceUID::get_singleton()->text_to_id(p_save_to_path));
	}
	return anim;
}

void ResourceImporterScene::_optimize_animations(
	AnimationPlayer* anim, float p_max_vel_error, float p_max_ang_error, int p_prc_error)
{
	LocalVector<StringName> anim_names;
	anim->get_animation_list(&anim_names);
	for (const StringName& E : anim_names) {
		Ref<Animation> a = anim->get_animation(E);
		a->optimize(p_max_vel_error, p_max_ang_error, p_prc_error);
	}
}

void ResourceImporterScene::_compress_animations(AnimationPlayer* anim, int p_page_size_kb)
{
	LocalVector<StringName> anim_names;
	anim->get_animation_list(&anim_names);
	for (const StringName& E : anim_names) {
		Ref<Animation> a = anim->get_animation(E);
		a->compress(p_page_size_kb * 1024);
	}
}

void ResourceImporterScene::_replace_owner(Node* p_node, Node* p_scene, Node* p_new_owner)
{
	if (p_node != p_new_owner && p_node->get_owner() == p_scene) {
		p_node->set_owner(p_new_owner);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* n = p_node->get_child(i);
		_replace_owner(n, p_scene, p_new_owner);
	}
}

void ResourceImporterScene::_add_shapes(Node* p_node, const Vector<Ref<Shape3D>>& p_shapes)
{
	for (const Ref<Shape3D>& E : p_shapes) {
		CollisionShape3D* cshape = memnew(CollisionShape3D);
		cshape->set_shape(E);
		p_node->add_child(cshape, true);

		cshape->set_owner(p_node->get_owner());
	}
}

Vector<Ref<EditorSceneFormatImporter>> ResourceImporterScene::scene_importers;
Vector<Ref<EditorScenePostImportPlugin>> ResourceImporterScene::post_importer_plugins;

bool ResourceImporterScene::has_advanced_options() const { return true; }

void ResourceImporterScene::show_advanced_options(const String& p_path)
{
	SceneImportSettingsDialog::get_singleton()->open_settings(p_path, _scene_import_type);
}

ResourceImporterScene::ResourceImporterScene(const String& p_scene_import_type)
{
	_scene_import_type = p_scene_import_type;
}

void ResourceImporterScene::add_scene_importer(
	Ref<EditorSceneFormatImporter> p_importer, bool p_first_priority)
{
	ERR_FAIL_COND(p_importer.is_null());
	if (p_first_priority) {
		scene_importers.insert(0, p_importer);
	}
	else {
		scene_importers.push_back(p_importer);
	}
}

void ResourceImporterScene::remove_post_importer_plugin(
	const Ref<EditorScenePostImportPlugin>& p_plugin)
{
	post_importer_plugins.erase(p_plugin);
}

void ResourceImporterScene::add_post_importer_plugin(
	const Ref<EditorScenePostImportPlugin>& p_plugin, bool p_first_priority)
{
	ERR_FAIL_COND(p_plugin.is_null());
	if (p_first_priority) {
		post_importer_plugins.insert(0, p_plugin);
	}
	else {
		post_importer_plugins.push_back(p_plugin);
	}
}

void ResourceImporterScene::remove_scene_importer(Ref<EditorSceneFormatImporter> p_importer)
{
	scene_importers.erase(p_importer);
}

void ResourceImporterScene::clean_up_importer_plugins()
{
	scene_importers.clear();
	post_importer_plugins.clear();
}

void ResourceImporterScene::get_scene_importer_extensions(List<String>* p_extensions)
{
	for (Ref<EditorSceneFormatImporter> importer_elem : scene_importers) {
		importer_elem->get_extensions(p_extensions);
	}
}

///////////////////////////////////////

void EditorSceneFormatImporterESCN::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("escn");
}

Node* EditorScenePostImport::post_import(Node* p_scene) { return p_scene; }


