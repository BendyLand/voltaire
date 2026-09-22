/**************************************************************************/
/*  editor_node.cpp                                                       */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/missing_resource.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "core/string/translation_server.h"
#include "core/version.h"
#include "editor/animation/animation_player_editor_plugin.h"
#include "editor/asset_library/asset_library_editor_plugin.h"
#include "editor/audio/audio_stream_preview.h"
#include "editor/audio/editor_audio_buses.h"
#include "editor/debugger/debugger_editor_plugin.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/doc/editor_help.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/groups_dock.h"
#include "editor/docks/history_dock.h"
#include "editor/docks/import_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/docks/signals_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_log.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/export/dedicated_server_export_plugin.h"
#include "editor/export/editor_export.h"
#include "editor/export/export_template_manager.h"
#include "editor/export/project_export.h"
#include "editor/export/project_zip_packer.h"
#include "editor/export/register_exporters.h"
#include "editor/export/shader_baker_export_plugin.h"
#include "editor/file_system/dependency_editor.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_about.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_icon_manager.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/gui/editor_title_bar.h"
#include "editor/gui/editor_toaster.h"
#include "editor/gui/progress_dialog.h"
#include "editor/gui/window_wrapper.h"
#include "editor/import/3d/editor_import_collada.h"
#include "editor/import/3d/resource_importer_obj.h"
#include "editor/import/3d/resource_importer_scene.h"
#include "editor/import/3d/scene_import_settings.h"
#include "editor/import/audio_stream_import_settings.h"
#include "editor/import/dynamic_font_import_settings.h"
#include "editor/import/fbx_importer_manager.h"
#include "editor/import/resource_importer_bitmask.h"
#include "editor/import/resource_importer_bmfont.h"
#include "editor/import/resource_importer_csv_translation.h"
#include "editor/import/resource_importer_dynamic_font.h"
#include "editor/import/resource_importer_image.h"
#include "editor/import/resource_importer_imagefont.h"
#include "editor/import/resource_importer_layered_texture.h"
#include "editor/import/resource_importer_shader_file.h"
#include "editor/import/resource_importer_svg.h"
#include "editor/import/resource_importer_texture.h"
#include "editor/import/resource_importer_texture_atlas.h"
#include "editor/import/resource_importer_wav.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_preview_plugins.h"
#include "editor/inspector/editor_properties.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/inspector/editor_resource_picker.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/plugins/editor_plugin_list.h"
#include "editor/plugins/editor_resource_conversion_plugin.h"
#include "editor/plugins/plugin_config_dialog.h"
#include "editor/project_upgrade/project_upgrade_tool.h"
#include "editor/run/editor_run.h"
#include "editor/run/editor_run_bar.h"
#include "editor/run/game_view_plugin.h"
#include "editor/scene/3d/material_3d_conversion_plugins.h"
#include "editor/scene/3d/mesh_library_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/scene/3d/root_motion_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/scene/editor_scene_tabs.h"
#include "editor/scene/material_editor_plugin.h"
#include "editor/scene/particle_process_material_editor_plugin.h"
#include "editor/script/editor_script.h"
#include "editor/script/script_text_editor.h"
#include "editor/script/text_editor.h"
#include "editor/settings/editor_build_profile.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_layouts_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/editor_settings_dialog.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/shader/editor_native_shader_source_visualizer.h"
#include "editor/shader/text_shader_editor.h"
#include "editor/themes/editor_color_map.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor/translations/editor_translation_parser.h"
#include "editor/translations/packed_scene_translation_parser_plugin.h"
#include "editor/version_control/version_control_editor_plugin.h"
#include "editor_node.h"
#include "main/main.h"
#include "scene/2d/node_2d.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/animation/animation_tree.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/menu_bar.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/property_utils.h"
#include "scene/resources/3d/mesh_library.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/theme/theme_db.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server.h"

#ifdef VULKAN_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_vulkan.h"
#endif

#ifdef D3D12_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_d3d12.h"
#endif

#ifdef METAL_ENABLED
#include "editor/shader/shader_baker/shader_baker_export_plugin_platform_metal.h"
#endif

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

#ifdef ANDROID_ENABLED
#include "editor/gui/touch_actions_panel.h"
#endif // ANDROID_ENABLED

#include <cstdlib>
#include "modules/modules_enabled.gen.h" // For gdscript, mono.

EditorNode* EditorNode::singleton = nullptr;

static const String EDITOR_NODE_CONFIG_SECTION = "EditorNode";

static const String REMOVE_ANDROID_BUILD_TEMPLATE_MESSAGE = TTRC(
	"The Android build template is already installed in this project and it won't be "
	"overwritten.\nRemove the \"%s\" directory manually before attempting this operation again.");
static const String INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE =
	TTRC("This will set up your project for gradle Android builds by installing the source "
		 "template to \"%s\".\nNote that in order to make gradle builds instead of using pre-built "
		 "APKs, the \"Use Gradle Build\" option should be enabled in the Android export preset.");

constexpr int LARGE_RESOURCE_WARNING_SIZE_THRESHOLD = 512'000; // 500 KB

void EditorNode::disambiguate_filenames(
	const Vector<String> p_full_paths, Vector<String>& r_filenames)
{
	ERR_FAIL_COND_MSG(p_full_paths.size() != r_filenames.size(),
		vformat("disambiguate_filenames requires two string vectors of same length (%d != %d).",
			p_full_paths.size(), r_filenames.size()));

	// Keep track of a list of "index sets," i.e. sets of indices
	// within disambiguated_scene_names which contain the same name.
	Vector<RBSet<int>> index_sets;
	HashMap<String, int> scene_name_to_set_index;
	for (int i = 0; i < r_filenames.size(); i++) {
		const String& scene_name = r_filenames[i];
		if (!scene_name_to_set_index.has(scene_name)) {
			index_sets.append(RBSet<int>());
			scene_name_to_set_index.insert(r_filenames[i], index_sets.size() - 1);
		}
		index_sets.write[scene_name_to_set_index[scene_name]].insert(i);
	}

	// For each index set with a size > 1, we need to disambiguate.
	for (int i = 0; i < index_sets.size(); i++) {
		RBSet<int> iset(index_sets[i]);
		while (iset.size() > 1) {
			// Append the parent folder to each scene name.
			for (const int& E : iset) {
				int set_idx = E;
				String scene_name = r_filenames[set_idx];
				String full_path = p_full_paths[set_idx];

				// Get rid of file extensions and res:// prefixes.
				scene_name = scene_name.get_basename();
				if (full_path.begins_with("res://")) {
					full_path = full_path.substr(6);
				}
				full_path = full_path.get_basename();

				// Normalize trailing slashes when normalizing directory names.
				scene_name = scene_name.trim_suffix("/");
				full_path = full_path.trim_suffix("/");

				int scene_name_size = scene_name.size();
				int full_path_size = full_path.size();
				int difference = full_path_size - scene_name_size;

				// Find just the parent folder of the current path and append it.
				// If the current name is foo.tscn, and the full path is /some/folder/foo.tscn
				// then slash_idx is the second '/', so that we select just "folder", and
				// append that to yield "folder/foo.tscn".
				if (difference > 0) {
					String parent = full_path.substr(0, difference);
					int slash_idx = parent.rfind_char('/');
					slash_idx = parent.rfind_char('/', slash_idx - 1);
					parent = (slash_idx >= 0 && parent.length() > 1) ? parent.substr(slash_idx + 1)
																	 : parent;
					r_filenames.write[set_idx] = parent + r_filenames[set_idx];
				}
			}

			// Loop back through scene names and remove non-ambiguous names.
			bool can_proceed = false;
			RBSet<int>::Element* E = iset.front();
			while (E) {
				String scene_name = r_filenames[E->get()];
				bool duplicate_found = false;
				for (const int& F : iset) {
					if (E->get() == F) {
						continue;
					}
					const String& other_scene_name = r_filenames[F];
					if (other_scene_name == scene_name) {
						duplicate_found = true;
						break;
					}
				}

				RBSet<int>::Element* to_erase = duplicate_found ? nullptr : E;

				// We need to check that we could actually append anymore names
				// if we wanted to for disambiguation. If we can't, then we have
				// to abort even with ambiguous names. We clean the full path
				// and the scene name first to remove extensions so that this
				// comparison actually works.
				String path = p_full_paths[E->get()];

				// Get rid of file extensions and res:// prefixes.
				scene_name = scene_name.get_basename();
				if (path.begins_with("res://")) {
					path = path.substr(6);
				}
				path = path.get_basename();

				// Normalize trailing slashes when normalizing directory names.
				scene_name = scene_name.trim_suffix("/");
				path = path.trim_suffix("/");

				// We can proceed if the full path is longer than the scene name,
				// meaning that there is at least one more parent folder we can
				// tack onto the name.
				can_proceed = can_proceed || (path.size() - scene_name.size()) >= 1;

				E = E->next();
				if (to_erase) {
					iset.erase(to_erase);
				}
			}

			if (!can_proceed) {
				break;
			}
		}
	}
}

void EditorNode::_version_control_menu_option(int p_idx)
{
	switch (vcs_actions_menu->get_item_id(p_idx)) {
	case VCS_SETTINGS: {
		VersionControlEditorPlugin::get_singleton()->popup_vcs_set_up_dialog(gui_base);
	} break;
	}
}

void EditorNode::_gdextensions_reloaded()
{
	// In case the developer is inspecting an object that will be changed by the reload.
	InspectorDock::get_inspector_singleton()->update_tree();

	// Reload script editor to revalidate GDScript if classes are added or removed.
	ScriptEditor::get_singleton()->reload_scripts(true);

	// Regenerate documentation without using script documentation cache since that would
	// revert doc changes during this session.
	EditorHelp::generate_doc(true, false);
}

void EditorNode::_propagate_translation_notification()
{
	pending_translation_notification = false;
	scene_root->propagate_notification(NOTIFICATION_TRANSLATION_CHANGED);
}

bool EditorNode::_is_project_data_missing()
{
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	const String project_data_dir = EditorPaths::get_singleton()->get_project_data_dir();
	if (!da->dir_exists(project_data_dir)) {
		return true;
	}

	String project_data_gdignore_file_path = project_data_dir.path_join(".gdignore");
	if (!FileAccess::exists(project_data_gdignore_file_path)) {
		Ref<FileAccess> f = FileAccess::open(project_data_gdignore_file_path, FileAccess::WRITE);
		if (f.is_valid()) {
			f->store_line("");
		}
		else {
			ERR_PRINT("Failed to create file " + project_data_gdignore_file_path.quote() + ".");
		}
	}

	String uid_cache = ResourceUID::get_singleton()->get_cache_file();
	if (!da->file_exists(uid_cache)) {
		Error err = ResourceUID::get_singleton()->save_to_cache();
		if (err != OK) {
			ERR_PRINT("Failed to create file " + uid_cache.quote() + ".");
		}
	}

	const String dirs[] = {EditorPaths::get_singleton()->get_project_settings_dir(),
		ProjectSettings::get_singleton()->get_imported_files_path()};
	for (const String& dir : dirs) {
		if (!da->dir_exists(dir)) {
			return true;
		}
	}
	return false;
}

void EditorNode::_remove_lock_file() { OS::get_singleton()->remove_lock_file(); }

void EditorNode::_reload_project_settings()
{
	ProjectSettings::get_singleton()->setup(
		ProjectSettings::get_singleton()->get_resource_path(), String(), true, true);
}

void EditorNode::_vp_resized() {}

void EditorNode::_node_renamed()
{
	if (InspectorDock::get_inspector_singleton()) {
		InspectorDock::get_inspector_singleton()->update_tree();
	}
}

int EditorNode::get_resource_count(Ref<Resource> p_res)
{
	List<Node*>* L = resource_count.getptr(p_res);
	return L ? L->size() : 0;
}

List<Node*> EditorNode::get_resource_node_list(Ref<Resource> p_res)
{
	List<Node*>* L = resource_count.getptr(p_res);
	return L == nullptr ? List<Node*>() : List<Node*>(*L);
}

void EditorNode::_dialog_display_save_error(String p_file, Error p_error)
{
	if (p_error) {
		switch (p_error) {
		case ERR_FILE_CANT_WRITE: {
			show_warning(TTR("Can't open file for writing:") + " " + p_file.get_extension());
		} break;
		case ERR_FILE_UNRECOGNIZED: {
			show_warning(TTR("Requested file format unknown:") + " " + p_file.get_extension());
		} break;
		default: {
			show_warning(TTR("Error while saving."));
		} break;
		}
	}
}

void EditorNode::_dialog_display_load_error(String p_file, Error p_error)
{
	if (p_error) {
		switch (p_error) {
		case ERR_CANT_OPEN: {
			show_warning(
				vformat(TTR("Can't open file '%s'. The file could have been moved or deleted."),
					p_file.get_file()));
		} break;
		case ERR_PARSE_ERROR: {
			show_warning(vformat(TTR("Error while parsing file '%s'."), p_file.get_file()));
		} break;
		case ERR_FILE_CORRUPT: {
			show_warning(
				vformat(TTR("Scene file '%s' appears to be invalid/corrupt."), p_file.get_file()));
		} break;
		case ERR_FILE_NOT_FOUND: {
			show_warning(
				vformat(TTR("Missing file '%s' or one of its dependencies."), p_file.get_file()));
		} break;
		case ERR_FILE_UNRECOGNIZED: {
			show_warning(
				vformat(TTR("File '%s' is saved in a format that is newer than the formats "
							"supported by this version of Godot, so it can't be opened."),
					p_file.get_file()));
		} break;
		default: {
			show_warning(vformat(TTR("Error while loading file '%s'."), p_file.get_file()));
		} break;
		}
	}
}

void EditorNode::_close_save_scene_progress()
{
	memdelete(save_scene_progress);
	save_scene_progress = nullptr;
}

bool EditorNode::_validate_scene_recursive(const String& p_filename, Node* p_node)
{
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* child = p_node->get_child(i);
		if (child->get_scene_file_path() == p_filename) {
			return true;
		}

		if (_validate_scene_recursive(p_filename, child)) {
			return true;
		}
	}

	return false;
}

bool EditorNode::is_scene_unsaved(int p_idx)
{
	const Node* scene = editor_data.get_edited_scene_root(p_idx);
	if (!scene) {
		return false;
	}

	if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(
			editor_data.get_scene_history_id(p_idx))) {
		return true;
	}

	const String& scene_path = scene->get_scene_file_path();
	if (!scene_path.is_empty()) {
		// Check if scene has unsaved changes in built-in resources.
		for (int j = 0; j < editor_data.get_editor_plugin_count(); j++) {
			if (!editor_data.get_editor_plugin(j)->get_unsaved_status(scene_path).is_empty()) {
				return true;
			}
		}
	}
	return false;
}

bool EditorNode::_is_class_editor_disabled_by_feature_profile(const StringName& p_class)
{
	Ref<EditorFeatureProfile> profile =
		EditorFeatureProfileManager::get_singleton()->get_current_profile();
	if (profile.is_null()) {
		return false;
	}

	StringName class_name = p_class;

	while (class_name != StringName()) {
		if (profile->is_class_disabled(class_name)) {
			return true;
		}
		if (profile->is_class_editor_disabled(class_name)) {
			return true;
		}
	}
	return false;
}

void EditorNode::_android_export_preset_selected(int p_index)
{
	if (p_index >= 0) {
		android_export_preset = EditorExport::get_singleton()->get_export_preset(
			choose_android_export_profile->get_item_id(p_index));
	}
	else {
		android_export_preset.unref();
	}
	install_android_build_template_message->set_text(
		vformat(TTR(INSTALL_ANDROID_BUILD_TEMPLATE_MESSAGE),
			export_template_manager->get_android_build_directory(android_export_preset)));
}

void EditorNode::_android_explore_build_templates()
{
	OS::get_singleton()->shell_show_in_file_manager(
		ProjectSettings::get_singleton()->globalize_path(
			export_template_manager->get_android_build_directory(android_export_preset)
				.get_base_dir()),
		true);
}

static String _get_unsaved_scene_dialog_text(String p_scene_filename, uint64_t p_opened_timestamp)
{
	const uint64_t scene_modified_time = FileAccess::get_modified_time(p_scene_filename);
	String unsaved_message;

	// Consider scene opening to be a point of saving, so that when you
	// close and reopen the editor, you don't get an excessively long
	// "modified X hours ago".
	const uint64_t last_modified_seconds = Time::get_singleton()->get_unix_time_from_system() -
										   MAX(p_opened_timestamp, scene_modified_time);

	String last_modified_string;
	if (last_modified_seconds < 120) {
		last_modified_string = vformat(
			TTRN("%d second ago", "%d seconds ago", last_modified_seconds), last_modified_seconds);
	}
	else if (last_modified_seconds < 7200) {
		last_modified_string =
			vformat(TTRN("%d minute ago", "%d minutes ago", last_modified_seconds / 60),
				last_modified_seconds / 60);
	}
	else {
		last_modified_string =
			vformat(TTRN("%d hour ago", "%d hours ago", last_modified_seconds / 3600),
				last_modified_seconds / 3600);
	}

	String last_action_and_time;
	if (p_opened_timestamp > scene_modified_time) {
		last_action_and_time = vformat(TTR("Scene opened: %s."), last_modified_string);
	}
	else {
		last_action_and_time = vformat(TTR("Last saved: %s."), last_modified_string);
	}

	unsaved_message = vformat(
		TTR("Scene \"%s\" has unsaved changes.\n%s"), p_scene_filename, last_action_and_time);

	return unsaved_message;
}

int EditorNode::_next_unsaved_scene(bool p_valid_filename, int p_start)
{
	for (int i = p_start; i < editor_data.get_edited_scene_count(); i++) {
		if (!editor_data.get_edited_scene_root(i)) {
			continue;
		}

		String scene_filename = editor_data.get_edited_scene_root(i)->get_scene_file_path();
		if (p_valid_filename && scene_filename.is_empty()) {
			continue;
		}

		bool unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(
			editor_data.get_scene_history_id(i));
		if (unsaved) {
			return i;
		}
		else {
			for (int j = 0; j < editor_data.get_editor_plugin_count(); j++) {
				if (!editor_data.get_editor_plugin(j)
						 ->get_unsaved_status(scene_filename)
						 .is_empty()) {
					return i;
				}
			}
		}
	}
	return -1;
}

void EditorNode::add_extension_editor_plugin(const StringName& p_class_name) {}

bool EditorNode::is_addon_plugin_enabled(const String& p_addon) const
{
	if (p_addon.begins_with("res://")) {
		return addon_name_to_plugin.has(p_addon);
	}

	return addon_name_to_plugin.has("res://addons/" + p_addon + "/plugin.cfg");
}

String EditorNode::get_preview_locale() const
{
	const Ref<TranslationDomain>& main_domain =
		TranslationServer::get_singleton()->get_main_domain();
	return main_domain->is_enabled() ? main_domain->get_locale_override() : String();
}

bool EditorNode::is_pseudolocalization_enabled() const
{
	const Ref<TranslationDomain>& main_domain =
		TranslationServer::get_singleton()->get_main_domain();
	return main_domain->is_pseudolocalization_enabled();
}

Ref<ConfigFile> EditorNode::_load_scene_config(const String& p_scene_path)
{
	const String config_file_path =
		EditorPaths::get_singleton()->get_project_settings_dir().path_join(
			p_scene_path.get_file() + "-editstate-" + p_scene_path.md5_text() + ".cfg");

	Ref<ConfigFile> editor_state_cf;
	editor_state_cf.instantiate();
	editor_state_cf->load(config_file_path);
	return editor_state_cf;
}

bool EditorNode::is_changing_scene() const { return changing_scene; }

bool EditorNode::is_scene_open(const String& p_path)
{
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == p_path) {
			return true;
		}
	}

	return false;
}

bool EditorNode::is_additional_node_in_scene(
	Node* p_edited_scene, Node* p_reimported_root, Node* p_node)
{
	if (p_node == p_reimported_root) {
		return false;
	}

	bool node_part_of_subscene =
		p_node != p_edited_scene && p_edited_scene->get_scene_inherited_state().is_valid() &&
		p_edited_scene->get_scene_inherited_state()->find_node_by_path(
			p_edited_scene->get_path_to(p_node)) >= 0 &&
		// It's important to process added nodes from the base scene in the inherited scene as
		// additional nodes to ensure they do not disappear on reload.
		// When p_reimported_root == p_edited_scene that means the edited scene
		// is the reimported scene, in that case the node is in the root base scene,
		// so it's not an addition, otherwise, the node would be added twice on reload.
		(p_node->get_owner() != p_edited_scene || p_reimported_root == p_edited_scene);

	if (node_part_of_subscene) {
		return false;
	}

	// Loop through the owners until either we reach the root node or nullptr
	Node* valid_node_owner = p_node->get_owner();
	while (valid_node_owner) {
		if (valid_node_owner == p_reimported_root) {
			break;
		}
		valid_node_owner = valid_node_owner->get_owner();
	}

	// When the owner is the imported scene and the owner is also the edited scene,
	// that means the node was added in the current edited scene.
	// We can be sure here because if the node that the node does not come from
	// the base scene because we checked just over with
	// 'get_scene_inherited_state()->find_node_by_path'.
	if (valid_node_owner == p_reimported_root && p_reimported_root != p_edited_scene) {
		return false;
	}

	return true;
}

void EditorNode::get_scene_editor_data_for_node(
	Node* p_root, Node* p_node, HashMap<NodePath, SceneEditorDataEntry>& p_table)
{
	SceneEditorDataEntry new_entry;
	new_entry.is_display_folded = p_node->is_displayed_folded();

	if (p_root != p_node) {
		new_entry.is_editable = p_root->is_editable_instance(p_node);
	}

	p_table.insert(p_root->get_path_to(p_node), new_entry);

	for (int i = 0; i < p_node->get_child_count(); i++) {
		get_scene_editor_data_for_node(p_root, p_node->get_child(i), p_table);
	}
}

void EditorNode::get_children_nodes(Node* p_node, List<Node*>& p_nodes)
{
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* child = p_node->get_child(i);
		p_nodes.push_back(child);
		get_children_nodes(child, p_nodes);
	}
}

bool EditorNode::has_previous_closed_scenes() const { return !prev_closed_scenes.is_empty(); }

bool EditorNode::is_resource_read_only(
	Ref<Resource> p_resource, bool p_foreign_resources_are_writable)
{
	ERR_FAIL_COND_V(p_resource.is_null(), false);

	String path = p_resource->get_path();
	if (!path.is_resource_file()) {
		// If the resource name contains '::', that means it is a subresource embedded in another
		// resource.
		int srpos = path.find("::");
		if (srpos != -1) {
			String base = path.substr(0, srpos);
			// If the base resource is a packed scene, we treat it as read-only if it is not the
			// currently edited scene.
			if (ResourceLoader::get_resource_type(base) == "PackedScene") {
				if (!get_tree()->get_edited_scene_root() ||
					get_tree()->get_edited_scene_root()->get_scene_file_path() != base) {
					// If we have not flagged foreign resources as writable or the base scene the
					// resource is part was imported, it can be considered read-only.
					if (!p_foreign_resources_are_writable || FileAccess::exists(base + ".import")) {
						return true;
					}
				}
			}
			else {
				// If a corresponding .import file exists for the base file, we assume it to be
				// imported and should therefore treated as read-only.
				if (FileAccess::exists(base + ".import")) {
					return true;
				}
			}
		}
	}
	else if (FileAccess::exists(path + ".import")) {
		// The resource is not a subresource, but if it has an .import file, it's imported so treat
		// it as read only.
		return true;
	}

	return false;
}

void EditorNode::_close_messages()
{
	old_split_ofs = center_split->get_split_offset();
	center_split->set_split_offset(0);
}

void EditorNode::_show_messages() { center_split->set_split_offset(old_split_ofs); }

void EditorNode::notify_all_debug_sessions_exited() { project_run_bar->stop_playing(); }

bool EditorNode::_find_scene_in_use(Node* p_node, const String& p_path) const
{
	if (p_node->get_scene_file_path() == p_path) {
		return true;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		if (_find_scene_in_use(p_node->get_child(i), p_path)) {
			return true;
		}
	}

	return false;
}

bool EditorNode::is_scene_in_use(const String& p_path)
{
	Node* es = get_edited_scene();
	if (es) {
		return _find_scene_in_use(es, p_path);
	}
	return false;
}

ProcessID EditorNode::has_child_process(ProcessID p_pid) const
{
	return project_run_bar->has_child_process(p_pid);
}

void EditorNode::stop_child_process(ProcessID p_pid) { project_run_bar->stop_child_process(p_pid); }

// Used to track the progress of tasks in the CLI output (since we don't have any other frame of
// reference).
static HashMap<String, int> progress_total_steps;

static String last_progress_task;
static String last_progress_state;
static int last_progress_step = 0;
static double last_progress_time = 0;

void EditorNode::progress_add_task_bg(const String& p_task, const String& p_label, int p_steps)
{
	singleton->progress_hb->add_task(p_task, p_label, p_steps);
}

void EditorNode::progress_task_step_bg(const String& p_task, int p_step)
{
	singleton->progress_hb->task_step(p_task, p_step);
}

void EditorNode::progress_end_task_bg(const String& p_task)
{
	singleton->progress_hb->end_task(p_task);
}

void EditorNode::_progress_dialog_visibility_changed()
{
	// Open the io errors after the progress dialog is closed.
	if (load_errors_queued_to_display && !progress_dialog->is_visible()) {
		EditorInterface::get_singleton()->popup_dialog_centered_ratio(
			singleton->load_error_dialog, 0.5);
		load_errors_queued_to_display = false;
	}
}

void EditorNode::_load_error_dialog_visibility_changed()
{
	if (!load_error_dialog->is_visible()) {
		load_errors->clear();
	}
}

Ref<Texture2D> EditorNode::_file_dialog_get_icon(const String& p_path)
{
	EditorFileSystemDirectory* efsd =
		EditorFileSystem::get_singleton()->get_filesystem_path(p_path.get_base_dir());
	if (efsd) {
		String file = p_path.get_file();
		for (int i = 0; i < efsd->get_file_count(); i++) {
			if (efsd->get_file(i) == file) {
				String type = efsd->get_file_type(i);

				if (singleton->icon_type_cache.has(type)) {
					return singleton->icon_type_cache[type];
				}
				else {
					return singleton->icon_type_cache["Object"];
				}
			}
		}
	}

	return singleton->icon_type_cache["Object"];
}

void EditorNode::_file_dialog_thumbnail_callback(const String& p_path,
	const Ref<Texture2D>& p_preview, const Ref<Texture2D>& p_small_preview,
	Ref<ImageTexture> p_texture)
{
	ERR_FAIL_COND(p_texture.is_null());
	if (p_preview.is_valid()) {
		p_texture->set_image(p_preview->get_image());
	}
}

void EditorNode::_build_icon_type_cache()
{
	List<StringName> tl;
	theme->get_icon_list(EditorStringName(EditorIcons), &tl);
	for (const StringName& E : tl) {
		icon_type_cache[E] = theme->get_icon(E, EditorStringName(EditorIcons));
	}
}

void EditorNode::_file_dialog_register(FileDialog* p_dialog)
{
	singleton->file_dialogs.insert(p_dialog);
}

void EditorNode::_file_dialog_unregister(FileDialog* p_dialog)
{
	singleton->file_dialogs.erase(p_dialog);
}

Vector<EditorNodeInitCallback> EditorNode::_init_callbacks;

void EditorNode::_begin_first_scan()
{
	if (!waiting_for_first_scan) {
		return;
	}
	requested_first_scan = true;
}

Error EditorNode::export_preset(const String& p_preset, const String& p_path, bool p_debug,
	bool p_pack_only, bool p_android_build_template, bool p_patch, const Vector<String>& p_patches)
{
	export_defer.preset = p_preset;
	export_defer.path = p_path;
	export_defer.debug = p_debug;
	export_defer.pack_only = p_pack_only;
	export_defer.android_build_template = p_android_build_template;
	export_defer.patch = p_patch;
	export_defer.patches = p_patches;
	cmdline_mode = true;
	return OK;
}

bool EditorNode::is_project_exporting() const
{
	return project_export && project_export->is_exporting();
}

void EditorNode::show_save_accept(const String& p_text, const String& p_ok_text)
{
	current_menu_option = -1;
	if (save_accept) {
		_close_save_scene_progress();
		save_accept->set_ok_button_text(p_ok_text);
		save_accept->set_text(p_text);
		save_accept->reset_size();
		EditorInterface::get_singleton()->popup_dialog_centered_clamped(save_accept, Size2i(), 0.0);
	}
}

void EditorNode::show_warning(const String& p_text, const String& p_title)
{
	if (warning) {
		_close_save_scene_progress();
		warning->set_text(p_text);
		warning->set_title(p_title);
		warning->reset_size();
		EditorInterface::get_singleton()->popup_dialog_centered_clamped(warning, Size2i(), 0.0);
	}
	else {
		WARN_PRINT(p_title + " " + p_text);
	}
}

void EditorNode::_copy_warning(const String& p_str)
{
	DisplayServer::get_singleton()->clipboard_set(warning->get_text());
}

void EditorNode::_immediate_dialog_confirmed() { immediate_dialog_confirmed = true; }

bool EditorNode::is_cmdline_mode()
{
	ERR_FAIL_NULL_V(singleton, false);
	return singleton->cmdline_mode;
}

void EditorNode::cleanup() { _init_callbacks.clear(); }

bool EditorNode::_is_closing_editor() const
{
	return tab_closing_menu_option == SCENE_QUIT ||
		   tab_closing_menu_option == PROJECT_QUIT_TO_PROJECT_MANAGER ||
		   tab_closing_menu_option == PROJECT_RELOAD_CURRENT_PROJECT;
}

void EditorNode::_cancel_close_scene_tab()
{
	if (_is_closing_editor()) {
		tab_closing_menu_option = -1;
	}
	changing_scene = false;
	tabs_to_close.clear();
}

void EditorNode::_cancel_confirmation()
{
	stop_project_confirmation = false;
	stop_download_confirmation = false;
}

void EditorNode::_prepare_save_confirmation_popup()
{
	if (save_confirmation->get_window() != get_last_exclusive_window()) {
		save_confirmation->reparent(get_last_exclusive_window());
	}
}

bool EditorNode::is_distraction_free_mode_enabled() const { return distraction_free->is_pressed(); }

void EditorNode::set_center_split_offset(int p_offset) { center_split->set_split_offset(p_offset); }

PopupMenu* EditorNode::get_export_as_menu() { return export_as_menu; }

void EditorNode::_add_dropped_files_recursive(const Vector<String>& p_files, String to_path)
{
	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND(dir.is_null());

	for (int i = 0; i < p_files.size(); i++) {
		const String& from = p_files[i];
		String to = to_path.path_join(from.get_file());

		if (dir->dir_exists(from)) {
			Vector<String> sub_files;

			Ref<DirAccess> sub_dir = DirAccess::open(from);
			ERR_FAIL_COND(sub_dir.is_null());

			sub_dir->list_dir_begin();

			String next_file = sub_dir->get_next();
			while (!next_file.is_empty()) {
				if (next_file == "." || next_file == "..") {
					next_file = sub_dir->get_next();
					continue;
				}

				sub_files.push_back(from.path_join(next_file));
				next_file = sub_dir->get_next();
			}

			if (!sub_files.is_empty()) {
				dir->make_dir(to);
				_add_dropped_files_recursive(sub_files, to);
			}

			continue;
		}

		dir->copy(from, to);
	}
}

void EditorNode::find_all_instances_inheriting_path_in_node(
	Node* p_root, Node* p_node, const String& p_instance_path, HashSet<Node*>& p_instance_list)
{
	bool valid_instance_found = false;

	// Attempt to find all the instances matching path we're going to reload.
	if (p_node->get_scene_file_path() == p_instance_path) {
		valid_instance_found = true;
	}
	else {
		Node* current_node = p_node;

		Ref<SceneState> inherited_state = current_node->get_scene_inherited_state();
		while (inherited_state.is_valid()) {
			String inherited_path = inherited_state->get_path();
			if (inherited_path == p_instance_path) {
				valid_instance_found = true;
				break;
			}

			inherited_state = inherited_state->get_base_scene_state();
		}
	}

	// Instead of adding this instance directly, if its not owned by the scene, walk its ancestors
	// and find the first node still owned by the scene. This is what we will reloading instead.
	if (valid_instance_found) {
		Node* current_node = p_node;
		while (true) {
			if (current_node->get_owner() == p_root || current_node->get_owner() == nullptr) {
				p_instance_list.insert(current_node);
				break;
			}
			current_node = current_node->get_parent();
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		find_all_instances_inheriting_path_in_node(
			p_root, p_node->get_child(i), p_instance_path, p_instance_list);
	}
}

void EditorNode::_remove_all_not_owned_children(Node* p_node, Node* p_owner)
{
	Vector<Node*> nodes_to_remove;
	if (p_node != p_owner && p_node->get_owner() != p_owner) {
		nodes_to_remove.push_back(p_node);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* child_node = p_node->get_child(i);
		_remove_all_not_owned_children(child_node, p_owner);
	}

	for (Node* node : nodes_to_remove) {
		node->get_parent()->remove_child(node);
		node->queue_free();
	}
}

int EditorNode::plugin_init_callback_count = 0;

void EditorNode::add_plugin_init_callback(EditorPluginInitializeCallback p_callback)
{
	ERR_FAIL_COND(plugin_init_callback_count == MAX_INIT_CALLBACKS);

	plugin_init_callbacks[plugin_init_callback_count++] = p_callback;
}

EditorPluginInitializeCallback EditorNode::plugin_init_callbacks[EditorNode::MAX_INIT_CALLBACKS];

int EditorNode::build_callback_count = 0;

void EditorNode::add_build_callback(EditorBuildCallback p_callback)
{
	ERR_FAIL_COND(build_callback_count == MAX_INIT_CALLBACKS);

	build_callbacks[build_callback_count++] = p_callback;
}

EditorBuildCallback EditorNode::build_callbacks[EditorNode::MAX_BUILD_CALLBACKS];

bool EditorNode::call_build()
{
	bool builds_successful = true;

	for (int i = 0; i < build_callback_count && builds_successful; i++) {
		if (!build_callbacks[i]()) {
			ERR_PRINT("A Godot Engine build callback failed.");
			builds_successful = false;
		}
	}

	if (builds_successful && !editor_data.call_build()) {
		ERR_PRINT("An EditorPlugin build callback failed.");
		builds_successful = false;
	}

	return builds_successful;
}

void EditorNode::call_run_scene(const String& p_scene, Vector<String>& r_args)
{
	for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
		EditorPlugin* plugin = editor_data.get_editor_plugin(i);
		plugin->run_scene(p_scene, r_args);
	}
}

void EditorNode::dim_editor(bool p_dimming)
{
	dimmed = p_dimming;
	gui_base->set_modulate(p_dimming ? Color(0.5, 0.5, 0.5) : Color(1, 1, 1));
}

bool EditorNode::is_editor_dimmed() const { return dimmed; }

void EditorNode::add_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin>& p_plugin)
{
	resource_conversion_plugins.push_back(p_plugin);
}

void EditorNode::remove_resource_conversion_plugin(
	const Ref<EditorResourceConversionPlugin>& p_plugin)
{
	resource_conversion_plugins.erase(p_plugin);
}

Vector<Ref<EditorResourceConversionPlugin>>
EditorNode::find_resource_conversion_plugin_for_resource(const Ref<Resource>& p_for_resource)
{
	if (p_for_resource.is_null()) {
		return Vector<Ref<EditorResourceConversionPlugin>>();
	}

	Vector<Ref<EditorResourceConversionPlugin>> ret;
	for (Ref<EditorResourceConversionPlugin> resource_conversion_plugin :
		resource_conversion_plugins) {
		if (resource_conversion_plugin.is_valid() &&
			resource_conversion_plugin->handles(p_for_resource)) {
			ret.push_back(resource_conversion_plugin);
		}
	}

	return ret;
}

Vector<Ref<EditorResourceConversionPlugin>>
EditorNode::find_resource_conversion_plugin_for_type_name(const String& p_type)
{
	Vector<Ref<EditorResourceConversionPlugin>> ret;
	return ret;
}

String EditorNode::_to_rendering_method_display_name(const String& p_rendering_method) const
{
	if (p_rendering_method == "forward_plus") {
		return TTR("Forward+");
	}
	if (p_rendering_method == "mobile") {
		return TTR("Mobile");
	}
	if (p_rendering_method == "gl_compatibility") {
		return TTR("Compatibility");
	}
	return p_rendering_method;
}

void EditorNode::_resource_loaded(Ref<Resource> p_resource, const String& p_path)
{
	singleton->editor_folding.load_resource_folding(p_resource, p_path);
}

static Node* _resource_get_edited_scene()
{
	return EditorNode::get_singleton()->get_edited_scene();
}

static void _execute_thread(void* p_ud)
{
	EditorNode::ExecuteThreadArgs* eta = (EditorNode::ExecuteThreadArgs*)p_ud;
	Error err = OS::get_singleton()->execute(
		eta->path, eta->args, &eta->output, &eta->exitcode, true, &eta->execute_output_mutex);
	print_verbose("Thread exit status: " + itos(eta->exitcode));
	if (err != OK) {
		eta->exitcode = err;
	}

	eta->done.set();
}

void EditorNode::set_unfocused_low_processor_usage_mode_enabled(bool p_enabled)
{
	unfocused_low_processor_usage_mode_enabled = p_enabled;
}

void EditorNode::_add_to_main_menu(const String& p_name, PopupMenu* p_menu)
{
	p_menu->set_name(p_name);
	main_menu_items.push_back(p_menu);
}

void EditorNode::_bottom_panel_resized()
{
	bottom_panel->set_bottom_panel_offset(center_split->get_split_offset());
}

#ifdef ANDROID_ENABLED
void EditorNode::_touch_actions_panel_mode_changed()
{
	int panel_mode = EDITOR_GET("interface/touchscreen/touch_actions_panel");
	switch (panel_mode) {
	case 1:
		if (touch_actions_panel != nullptr) {
			touch_actions_panel->queue_free();
		}
		touch_actions_panel = memnew(TouchActionsPanel);
		main_hbox->call_deferred("add_child", touch_actions_panel);
		break;
	case 2:
		if (touch_actions_panel != nullptr) {
			touch_actions_panel->queue_free();
		}
		touch_actions_panel = memnew(TouchActionsPanel);
		call_deferred("add_child", touch_actions_panel);
		break;
	case 0:
		if (touch_actions_panel != nullptr) {
			touch_actions_panel->queue_free();
			touch_actions_panel = nullptr;
		}
		break;
	}
}
#endif

#ifdef MACOS_ENABLED
extern "C" GameViewPluginBase* get_game_view_plugin();
#else
GameViewPluginBase* get_game_view_plugin() { return memnew(GameViewPlugin); }
#endif

void EditorNode::notify_settings_overrides_changed() { settings_overrides_changed = true; }

EditorNode::~EditorNode()
{
	EditorInspector::cleanup_plugins();
	EditorTranslationParser::get_singleton()->clean_parsers();
	ResourceImporterScene::clean_up_importer_plugins();
	EditorContextMenuPluginManager::cleanup();

	remove_print_handler(&print_handler);
	EditorHelp::cleanup_doc();
#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	EditorHelpHighlighter::free_singleton();
#endif
	memdelete(editor_selection);
	memdelete(editor_plugins_over);
	memdelete(editor_plugins_force_over);
	memdelete(editor_plugins_force_input_forwarding);
	memdelete(progress_hb);
	memdelete(project_upgrade_tool);
	memdelete(editor_dock_manager);

	EditorSettings::destroy();
	EditorThemeManager::finalize();

	FileDialog::register_func = nullptr;
	FileDialog::unregister_func = nullptr;

	file_dialogs.clear();

	singleton = nullptr;
}

String StandardMaterial3DConversionPlugin::converts_to() const { return ""; }

bool StandardMaterial3DConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	return false;
}

Ref<Resource> StandardMaterial3DConversionPlugin::convert(const Ref<Resource>& p_resource) const
{
	return Ref<Resource>();
}

String ORMMaterial3DConversionPlugin::converts_to() const { return ""; }

bool ORMMaterial3DConversionPlugin::handles(const Ref<Resource>& p_resource) const { return false; }

Ref<Resource> ORMMaterial3DConversionPlugin::convert(const Ref<Resource>& p_resource) const
{
	return Ref<Resource>();
}

String ProceduralSkyMaterialConversionPlugin::converts_to() const { return ""; }

bool ProceduralSkyMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	return false;
}

Ref<Resource> ProceduralSkyMaterialConversionPlugin::convert(const Ref<Resource>& p_resource) const
{
	return Ref<Resource>();
}

String PanoramaSkyMaterialConversionPlugin::converts_to() const { return ""; }

bool PanoramaSkyMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	return false;
}

Ref<Resource> PanoramaSkyMaterialConversionPlugin::convert(const Ref<Resource>& p_resource) const
{
	return Ref<Resource>();
}

String PhysicalSkyMaterialConversionPlugin::converts_to() const { return ""; }

bool PhysicalSkyMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const
{
	return false;
}

Ref<Resource> PhysicalSkyMaterialConversionPlugin::convert(const Ref<Resource>& p_resource) const
{
	return Ref<Resource>();
}

String FogMaterialConversionPlugin::converts_to() const { return ""; }

bool FogMaterialConversionPlugin::handles(const Ref<Resource>& p_resource) const { return false; }

Ref<Resource> FogMaterialConversionPlugin::convert(const Ref<Resource>& p_resource) const
{
	return Ref<Resource>();
}


