/**************************************************************************/
/*  editor_node.cpp                                                       */
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
#include "editor/script/script_editor_navigation_marker.h"
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
#include "servers/rendering/renderer.h"

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
		VersionControlEditorPlugin::popup_vcs_set_up_dialog(gui_base);
	} break;
	}
}

void EditorNode::_gdextensions_reloaded()
{
	// In case the developer is inspecting an object that will be changed by the reload.
	InspectorDock::get_inspector_singleton()->update_tree();

	// Reload script editor to revalidate GDScript if classes are added or removed.
	ScriptEditor::reload_scripts(true);

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
	const String project_data_dir = EditorPaths::get_project_data_dir();
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

	String uid_cache = ResourceUID::get_cache_file();
	if (!da->file_exists(uid_cache)) {
		Error err = ResourceUID::save_to_cache();
		if (err != OK) {
			ERR_PRINT("Failed to create file " + uid_cache.quote() + ".");
		}
	}

	const String dirs[] = {EditorPaths::get_project_settings_dir(),
		ProjectSettings::get_imported_files_path()};
	for (const String& dir : dirs) {
		if (!da->dir_exists(dir)) {
			return true;
		}
	}
	return false;
}

void EditorNode::_remove_lock_file() { OS::remove_lock_file(); }

void EditorNode::_reload_project_settings()
{
	ProjectSettings::setup(
		ProjectSettings::get_resource_path(), String(), true, true);
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
	return EditorNode::instance->_impl_is_scene_unsaved(p_idx);
}

bool EditorNode::_impl_is_scene_unsaved(int p_idx)
{
	const Node* scene = editor_data.get_edited_scene_root(p_idx);
	if (!scene) {
		return false;
	}

	if (EditorUndoRedoManager::is_history_unsaved(
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

bool EditorNode::_is_class_editor_disabled_by_feature_profile(const String& p_class)
{
	Ref<EditorFeatureProfile> profile =
		EditorFeatureProfileManager::get_current_profile();
	if (profile.is_null()) {
		return false;
	}

	String class_name = p_class;

	while (class_name != String()) {
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
		android_export_preset = EditorExport::get_export_preset(
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
	OS::shell_show_in_file_manager(
		ProjectSettings::globalize_path(
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
	const uint64_t last_modified_seconds =
		Time::get_unix_time_from_system() - MAX(p_opened_timestamp, scene_modified_time);

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

		bool unsaved = EditorUndoRedoManager::is_history_unsaved(
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

void EditorNode::add_extension_editor_plugin(const String& p_class_name) {}

bool EditorNode::is_addon_plugin_enabled(const String& p_addon)
{
	return EditorNode::instance->_impl_is_addon_plugin_enabled(p_addon);
}

bool EditorNode::_impl_is_addon_plugin_enabled(const String& p_addon) const
{
	if (p_addon.begins_with("res://")) {
		return addon_name_to_plugin.has(p_addon);
	}

	return addon_name_to_plugin.has("res://addons/" + p_addon + "/plugin.cfg");
}

String EditorNode::get_preview_locale() const
{
	const Ref<TranslationDomain>& main_domain =
		TranslationServer::get_main_domain();
	return main_domain->is_enabled() ? main_domain->get_locale_override() : String();
}

bool EditorNode::is_pseudolocalization_enabled() const
{
	const Ref<TranslationDomain>& main_domain =
		TranslationServer::get_main_domain();
	return main_domain->is_pseudolocalization_enabled();
}

Ref<ConfigFile> EditorNode::_load_scene_config(const String& p_scene_path)
{
	const String config_file_path =
		EditorPaths::get_project_settings_dir().path_join(
			p_scene_path.get_file() + "-editstate-" + p_scene_path.md5_text() + ".cfg");

	Ref<ConfigFile> editor_state_cf;
	editor_state_cf.instantiate();
	editor_state_cf->load(config_file_path);
	return editor_state_cf;
}

bool EditorNode::is_changing_scene()
{
	return EditorNode::instance->_impl_is_changing_scene();
}

bool EditorNode::_impl_is_changing_scene() const
{ return changing_scene; }

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

bool EditorNode::is_resource_read_only(Ref<Resource> p_resource, bool p_foreign_resources_are_writable)
{
	return EditorNode::instance->_impl_is_resource_read_only(p_resource, p_foreign_resources_are_writable);
}

bool EditorNode::_impl_is_resource_read_only(Ref<Resource> p_resource, bool p_foreign_resources_are_writable)
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
				if (!SceneTree::get_edited_scene_root() ||
					SceneTree::get_edited_scene_root()->get_scene_file_path() != base) {
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

ProcessID EditorNode::has_child_process(ProcessID p_pid)
{
	return EditorNode::instance->_impl_has_child_process(p_pid);
}

ProcessID EditorNode::_impl_has_child_process(ProcessID p_pid) const
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
	EditorNode::instance->progress_hb->add_task(p_task, p_label, p_steps);
}

void EditorNode::progress_task_step_bg(const String& p_task, int p_step)
{
	EditorNode::instance->progress_hb->task_step(p_task, p_step);
}

void EditorNode::progress_end_task_bg(const String& p_task)
{
	EditorNode::instance->progress_hb->end_task(p_task);
}

void EditorNode::_progress_dialog_visibility_changed()
{
	// Open the io errors after the progress dialog is closed.
	if (load_errors_queued_to_display && !progress_dialog->is_visible()) {
		EditorInterface::popup_dialog_centered_ratio(EditorNode::instance->load_error_dialog, 0.5);
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
		EditorFileSystem::get_filesystem_path(p_path.get_base_dir());
	if (efsd) {
		String file = p_path.get_file();
		for (int i = 0; i < efsd->get_file_count(); i++) {
			if (efsd->get_file(i) == file) {
				String type = efsd->get_file_type(i);

				if (EditorNode::instance->icon_type_cache.has(type)) {
					return EditorNode::instance->icon_type_cache[type];
				}
				else {
					return EditorNode::instance->icon_type_cache["Object"];
				}
			}
		}
	}

	return EditorNode::instance->icon_type_cache["Object"];
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
	List<String> tl;
	theme->get_icon_list(EditorString(EditorIcons), &tl);
	for (const String& E : tl) {
		icon_type_cache[E] = theme->get_icon(E, EditorString(EditorIcons));
	}
}

void EditorNode::_file_dialog_register(FileDialog* p_dialog)
{
	EditorNode::instance->file_dialogs.insert(p_dialog);
}

void EditorNode::_file_dialog_unregister(FileDialog* p_dialog)
{
	EditorNode::instance->file_dialogs.erase(p_dialog);
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
		EditorInterface::popup_dialog_centered_clamped(save_accept, Size2i(), 0.0);
	}
}

void EditorNode::show_warning(const String& p_text, const String& p_title)
{
	ERR_FAIL_NULL(EditorNode::instance);
	EditorNode::instance->_impl_show_warning(p_text, p_title);
}

void EditorNode::_impl_show_warning(const String& p_text, const String& p_title)
{
	if (warning) {
		_close_save_scene_progress();
		warning->set_text(p_text);
		warning->set_title(p_title);
		warning->reset_size();
		EditorInterface::popup_dialog_centered_clamped(warning, Size2i(), 0.0);
	}
	else {
		WARN_PRINT(p_title + " " + p_text);
	}
}

void EditorNode::_copy_warning(const String& p_str)
{
	DisplayServer::clipboard_set(warning->get_text());
}

void EditorNode::_immediate_dialog_confirmed() { immediate_dialog_confirmed = true; }

bool EditorNode::is_cmdline_mode()
{
	ERR_FAIL_NULL_V(EditorNode::instance, false);
	return EditorNode::instance->cmdline_mode;
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

bool EditorNode::is_distraction_free_mode_enabled()
{
	return EditorNode::instance->_impl_is_distraction_free_mode_enabled();
}

bool EditorNode::_impl_is_distraction_free_mode_enabled() const
{ return distraction_free->is_pressed(); }

void EditorNode::set_center_split_offset(int p_offset) { center_split->set_split_offset(p_offset); }

PopupMenu* EditorNode::get_export_as_menu()
{
	return EditorNode::instance->_impl_get_export_as_menu();
}

PopupMenu* EditorNode::_impl_get_export_as_menu()
{ return export_as_menu; }

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
	return EditorNode::instance->_impl_call_build();
}

bool EditorNode::_impl_call_build()
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
	ERR_FAIL_NULL(EditorNode::instance);
	EditorNode::instance->_impl_add_resource_conversion_plugin(p_plugin);
}

void EditorNode::_impl_add_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin>& p_plugin)
{
	resource_conversion_plugins.push_back(p_plugin);
}

void EditorNode::remove_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin>& p_plugin)
{
	ERR_FAIL_NULL(EditorNode::instance);
	EditorNode::instance->_impl_remove_resource_conversion_plugin(p_plugin);
}

void EditorNode::_impl_remove_resource_conversion_plugin(const Ref<EditorResourceConversionPlugin>& p_plugin)
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

Vector<Ref<EditorResourceConversionPlugin>> EditorNode::find_resource_conversion_plugin_for_type_name(const String& p_type)
{
	return EditorNode::instance->_impl_find_resource_conversion_plugin_for_type_name(p_type);
}

Vector<Ref<EditorResourceConversionPlugin>> EditorNode::_impl_find_resource_conversion_plugin_for_type_name(const String& p_type)
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
	EditorNode::instance->editor_folding.load_resource_folding(p_resource, p_path);
}

static Node* _resource_get_edited_scene()
{
	return EditorNode::get_edited_scene();
}

static void _execute_thread(void* p_ud)
{
	EditorNode::ExecuteThreadArgs* eta = (EditorNode::ExecuteThreadArgs*)p_ud;
	Error err = OS::execute(
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

void EditorNode::notify_settings_overrides_changed()
{
	ERR_FAIL_NULL(EditorNode::instance);
	EditorNode::instance->_impl_notify_settings_overrides_changed();
}

void EditorNode::_impl_notify_settings_overrides_changed()
{ settings_overrides_changed = true; }

EditorNode::~EditorNode()
{
	EditorInspector::cleanup_plugins();
	EditorTranslationParser::finalize();
	ResourceImporterScene::clean_up_importer_plugins();
	EditorContextMenuPluginManager::finalize();
	ScriptEditorNavigationMarker::finalize();
	EditorVCSInterface::finalize();
	EditorInterface::finalize();

	remove_print_handler(&print_handler);
	EditorHelp::cleanup_doc();
#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
	EditorHelpHighlighter::finalize();
#endif
	memdelete(editor_selection);
	memdelete(editor_plugins_over);
	memdelete(editor_plugins_force_over);
	memdelete(editor_plugins_force_input_forwarding);
	memdelete(progress_hb);
	memdelete(project_upgrade_tool);
	EditorDockManager::finalize();

	EditorSettings::finalize();
	EditorThemeManager::finalize();
	EditorStrings::finalize();

	FileDialog::register_func = nullptr;
	FileDialog::unregister_func = nullptr;

	file_dialogs.clear();

	EditorNode::instance = nullptr;
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

/**************************************************************************/
/*  EditorUI — Voltaire Data-Oriented Project Editor & Runtime Preview    */
/**************************************************************************/

#include "core/io/dir_access.h"
#include "editor/editor_ui.h"
#include "editor/project_manager/project_manager.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

namespace
{
void _eui_draw_bordered_rect(RID p_ci, const Rect2& p_rect, const Color& p_fill,
	const Color& p_border, float p_border_w = 1.0f)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_fill);
	const float bw = MIN(p_border_w, MIN(p_rect.size.x * 0.5f, p_rect.size.y * 0.5f));
	if (bw > 0.0f) {
		Renderer::canvas_item_add_rect(
			p_ci, Rect2(p_rect.position.x, p_rect.position.y, p_rect.size.x, bw), p_border);
		Renderer::canvas_item_add_rect(p_ci,
			Rect2(p_rect.position.x, p_rect.position.y + p_rect.size.y - bw, p_rect.size.x, bw),
			p_border);
		Renderer::canvas_item_add_rect(
			p_ci, Rect2(p_rect.position.x, p_rect.position.y, bw, p_rect.size.y), p_border);
		Renderer::canvas_item_add_rect(p_ci,
			Rect2(p_rect.position.x + p_rect.size.x - bw, p_rect.position.y, bw, p_rect.size.y),
			p_border);
	}
}

float _eui_baseline_y(const Ref<Font>& p_font, const Rect2& p_rect, int p_font_size)
{
	if (p_font.is_valid()) {
		float ascent = p_font->get_ascent(p_font_size);
		float descent = p_font->get_descent(p_font_size);
		if (ascent > 0.0f) {
			return Math::round(p_rect.position.y + (p_rect.size.y + ascent - descent) * 0.5f);
		}
	}
	return Math::round(p_rect.position.y + p_rect.size.y * 0.65f);
}

void _eui_draw_button(
	RID p_ci, const Ref<Font>& p_font, const EditorUI::UIButton& p_btn, int p_font_size = 13)
{
	if (p_btn.rect.size.x <= 2.0f || p_btn.rect.size.y <= 2.0f) {
		return;
	}
	const bool enabled = (p_btn.is_enabled == nullptr) || p_btn.is_enabled();

	Color bg(0.19f, 0.23f, 0.29f, 1.0f);
	Color accent(0.0f, 0.75f, 0.95f, 1.0f);
	Color text_col(0.92f, 0.94f, 0.96f, 1.0f);

	if (!enabled) {
		bg = Color(0.13f, 0.15f, 0.18f, 1.0f);
		accent = Color(0.20f, 0.24f, 0.29f, 1.0f);
		text_col = Color(0.45f, 0.49f, 0.55f, 1.0f);
	}
	else if (p_btn.primary) {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.0f, 0.50f, 0.68f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.06f, 0.68f, 0.88f, 1.0f);
		}
		else {
			bg = Color(0.02f, 0.58f, 0.78f, 1.0f);
		}
		accent = Color(0.45f, 0.92f, 1.0f, 1.0f);
		text_col = Color(0.98f, 0.99f, 1.0f, 1.0f);
	}
	else if (p_btn.active_tab) {
		bg = p_btn.hovered ? Color(0.24f, 0.34f, 0.44f, 1.0f) : Color(0.20f, 0.30f, 0.40f, 1.0f);
		accent = Color(0.0f, 0.85f, 1.0f, 1.0f);
	}
	else {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.15f, 0.20f, 0.27f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.27f, 0.34f, 0.44f, 1.0f);
		}
	}

	Renderer::canvas_item_add_rect(p_ci, p_btn.rect, bg);
	Renderer::canvas_item_add_rect(p_ci,
		Rect2(p_btn.rect.position.x,
			p_btn.rect.position.y + p_btn.rect.size.y - (p_btn.active_tab ? 3.0f : 2.0f),
			p_btn.rect.size.x, p_btn.active_tab ? 3.0f : 2.0f),
		accent);

	if (p_font.is_valid() && p_btn.rect.size.x > 14.0f) {
		const float pad_x = 6.0f;
		const float avail_w = p_btn.rect.size.x - pad_x * 2.0f;
		const float base_y = _eui_baseline_y(p_font, p_btn.rect, p_font_size);
		p_font->draw_string(p_ci, Point2(p_btn.rect.position.x + pad_x, base_y), p_btn.label,
			HORIZONTAL_ALIGNMENT_CENTER, avail_w, p_font_size, text_col);
	}
}

// Cohen-Sutherland 2D line segment clipping against p_bounds
bool _eui_clip_segment(const Rect2& p_bounds, Point2& r_a, Point2& r_b)
{
	const float xmin = p_bounds.position.x;
	const float ymin = p_bounds.position.y;
	const float xmax = p_bounds.position.x + p_bounds.size.x;
	const float ymax = p_bounds.position.y + p_bounds.size.y;

	auto outcode = [&](const Point2& p) -> int {
		int code = 0;
		if (p.x < xmin) {
			code |= 1;
		}
		else if (p.x > xmax) {
			code |= 2;
		}
		if (p.y < ymin) {
			code |= 4;
		}
		else if (p.y > ymax) {
			code |= 8;
		}
		return code;
	};

	int c0 = outcode(r_a);
	int c1 = outcode(r_b);
	for (int iter = 0; iter < 12; iter++) {
		if (!(c0 | c1)) {
			return true;
		}
		if (c0 & c1) {
			return false;
		}
		int cout = c0 ? c0 : c1;
		Point2 p;
		if (cout & 8) {
			p.x = r_a.x + (r_b.x - r_a.x) * (ymax - r_a.y) / (r_b.y - r_a.y);
			p.y = ymax;
		}
		else if (cout & 4) {
			p.x = r_a.x + (r_b.x - r_a.x) * (ymin - r_a.y) / (r_b.y - r_a.y);
			p.y = ymin;
		}
		else if (cout & 2) {
			p.y = r_a.y + (r_b.y - r_a.y) * (xmax - r_a.x) / (r_b.x - r_a.x);
			p.x = xmax;
		}
		else {
			p.y = r_a.y + (r_b.y - r_a.y) * (xmin - r_a.x) / (r_b.x - r_a.x);
			p.x = xmin;
		}
		if (cout == c0) {
			r_a = p;
			c0 = outcode(r_a);
		}
		else {
			r_b = p;
			c1 = outcode(r_b);
		}
	}
	return false;
}

void _eui_draw_clipped_line(RID p_ci, const Rect2& p_clip, Point2 p_a, Point2 p_b,
	const Color& p_col, float p_width = 1.0f)
{
	if (_eui_clip_segment(p_clip, p_a, p_b)) {
		Renderer::canvas_item_add_line(p_ci, p_a, p_b, p_col, p_width, true);
	}
}

bool _eui_project_3d(const Vector3& p_pt, float p_yaw, float p_pitch, const Rect2& p_vp,
	Point2& r_screen)
{
	const float cy = Math::cos(p_yaw);
	const float sy = Math::sin(p_yaw);
	const float cp = Math::cos(p_pitch);
	const float sp = Math::sin(p_pitch);

	// Orbit around origin at distance 9.0
	float x1 = p_pt.x * cy - p_pt.z * sy;
	float z1 = p_pt.x * sy + p_pt.z * cy;
	float y2 = p_pt.y * cp - z1 * sp;
	float z2 = p_pt.y * sp + z1 * cp + 9.0f;

	if (z2 <= 0.4f) {
		return false;
	}
	const float fov_scale = MIN(p_vp.size.x, p_vp.size.y) * 0.85f;
	r_screen.x = p_vp.position.x + p_vp.size.x * 0.5f + (x1 / z2) * fov_scale;
	r_screen.y = p_vp.position.y + p_vp.size.y * 0.5f - (y2 / z2) * fov_scale;
	return true;
}
} // namespace

bool EditorUI::_can_modify_selected_node()
{
	return data && data->selected_entity_idx >= 0 &&
		   data->selected_entity_idx < (int)data->entities.size();
}

bool EditorUI::_can_delete_selected_node()
{
	// Keep index 0 (Root) so scene always has a root node
	return data && data->selected_entity_idx > 0 &&
		   data->selected_entity_idx < (int)data->entities.size();
}

void EditorUI::add_log(const String& p_msg)
{
	if (!data) {
		return;
	}
	data->logs.push_back(p_msg);
	if (data->logs.size() > 64) {
		data->logs.remove_at(0);
	}
	data->needs_redraw = true;
}

void EditorUI::scan_project_files()
{
	if (!data || data->project_path.is_empty()) {
		return;
	}
	data->files.clear();
	Ref<DirAccess> da = DirAccess::open(data->project_path);
	if (da.is_null()) {
		return;
	}

	LocalVector<FileEntry> dirs;
	LocalVector<FileEntry> regular;
	da->list_dir_begin();
	String item = da->get_next();
	while (!item.is_empty()) {
		if (item != "." && item != ".." && !item.begins_with(".")) {
			FileEntry fe;
			fe.name = item;
			fe.full_path = data->project_path.path_join(item);
			fe.is_dir = da->current_is_dir();
			if (fe.is_dir) {
				dirs.push_back(fe);
			}
			else {
				regular.push_back(fe);
			}
		}
		item = da->get_next();
	}
	da->list_dir_end();

	for (uint32_t i = 0; i < dirs.size(); i++) {
		data->files.push_back(dirs[i]);
	}
	for (uint32_t i = 0; i < regular.size(); i++) {
		data->files.push_back(regular[i]);
	}
}

void EditorUI::load_scene()
{
	if (!data) {
		return;
	}
	data->entities.clear();

	String scene_path = data->project_path.path_join("main.vscn");
	if (FileAccess::exists(scene_path)) {
		Ref<FileAccess> fa = FileAccess::open(scene_path, FileAccess::READ);
		if (fa.is_valid()) {
			while (!fa->eof_reached()) {
				String line = fa->get_line().strip_edges();
				if (line.is_empty() || line.begins_with("#") || line.begins_with(";")) {
					continue;
				}
				Vector<String> parts = line.split("|");
				if (parts.size() >= 9) {
					SceneEntity ent;
					ent.name = parts[0];
					ent.type = parts[1];
					ent.position = Vector2(parts[2].to_float(), parts[3].to_float());
					ent.size = Size2(MAX(16.0f, parts[4].to_float()), MAX(16.0f, parts[5].to_float()));
					ent.color =
						Color(parts[6].to_float(), parts[7].to_float(), parts[8].to_float(), 1.0f);
					ent.runtime_vel = Vector2(130.0f + 25.0f * (data->entities.size() % 3),
						95.0f + 20.0f * (data->entities.size() % 2));
					data->entities.push_back(ent);
				}
			}
		}
	}

	if (data->entities.is_empty()) {
		SceneEntity root;
		root.name = "MainScene";
		root.type = "Node2D";
		root.position = Vector2(0.0f, 0.0f);
		root.size = Size2(48.0f, 48.0f);
		root.color = Color(0.55f, 0.65f, 0.78f, 1.0f);
		data->entities.push_back(root);

		SceneEntity player;
		player.name = "Player";
		player.type = "Sprite2D";
		player.position = Vector2(220.0f, 180.0f);
		player.size = Size2(96.0f, 96.0f);
		player.color = Color(0.0f, 0.75f, 0.95f, 1.0f);
		player.runtime_vel = Vector2(170.0f, 125.0f);
		data->entities.push_back(player);

		SceneEntity platform;
		platform.name = "GroundPlatform";
		platform.type = "StaticBody2D";
		platform.position = Vector2(160.0f, 380.0f);
		platform.size = Size2(360.0f, 44.0f);
		platform.color = Color(0.22f, 0.78f, 0.55f, 1.0f);
		platform.runtime_vel = Vector2(-95.0f, 65.0f);
		data->entities.push_back(platform);
	}

	data->selected_entity_idx = data->entities.size() > 1 ? 1 : 0;
	data->next_entity_serial = (int)data->entities.size();
	data->scene_dirty = false;
}

void EditorUI::save_scene()
{
	if (!data || data->project_path.is_empty()) {
		return;
	}
	String scene_path = data->project_path.path_join("main.vscn");
	Ref<FileAccess> fa = FileAccess::open(scene_path, FileAccess::WRITE);
	if (fa.is_null()) {
		add_log("ERROR: Could not write scene to " + scene_path);
		return;
	}
	fa->store_line("# Voltaire Scene v1");
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		const SceneEntity& e = data->entities[i];
		fa->store_line(vformat("%s|%s|%.1f|%.1f|%.1f|%.1f|%.3f|%.3f|%.3f",
			e.name.replace("|", "_"), e.type.replace("|", "_"), e.position.x, e.position.y,
			e.size.x, e.size.y, e.color.r, e.color.g, e.color.b));
	}
	fa->close();
	data->scene_dirty = false;
	scan_project_files();
	add_log("Saved scene: res://main.vscn (" + itos(data->entities.size()) + " nodes)");
}

void EditorUI::open_text_file(const String& p_path)
{
	if (!data || p_path.is_empty() || !FileAccess::exists(p_path)) {
		return;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	if (fa.is_null()) {
		add_log("Could not open file: " + p_path.get_file());
		return;
	}
	data->script_lines.clear();
	while (!fa->eof_reached()) {
		data->script_lines.push_back(fa->get_line().replace("\t", "    "));
	}
	if (data->script_lines.is_empty()) {
		data->script_lines.push_back("");
	}
	data->open_file_path = p_path;
	data->open_file_name = p_path.get_file();
	data->caret_line = 0;
	data->script_scroll_line = 0;
	data->script_dirty = false;
	data->needs_redraw = true;
}

void EditorUI::save_open_text_file()
{
	if (!data || data->open_file_path.is_empty()) {
		return;
	}
	Ref<FileAccess> fa = FileAccess::open(data->open_file_path, FileAccess::WRITE);
	if (fa.is_null()) {
		add_log("ERROR: Failed to save " + data->open_file_name);
		return;
	}
	for (int i = 0; i < data->script_lines.size(); i++) {
		fa->store_line(data->script_lines[i]);
	}
	fa->close();
	data->script_dirty = false;
	add_log("Saved file: res://" + data->open_file_name);
}

void EditorUI::initialize(RID p_viewport_rid, Size2i p_size, const String& p_project_path,
	const String& p_project_name, bool p_start_playing)
{
	if (data) {
		return;
	}
	data = memnew(Data);
	data->window_size = (p_size.x > 0 && p_size.y > 0)
							? p_size
							: Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
	data->project_path = p_project_path.simplify_path();
	if (data->project_path.begins_with("res://")) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			data->project_path = da->get_current_dir();
		}
	}
	data->project_name = p_project_name;
	if (data->project_name.is_empty()) {
		String cfg_path = data->project_path.path_join("project.godot");
		Ref<FileAccess> cfg_fa = FileAccess::open(cfg_path, FileAccess::READ);
		if (cfg_fa.is_valid()) {
			while (!cfg_fa->eof_reached()) {
				String ln = cfg_fa->get_line().strip_edges();
				if (ln.begins_with("config/name=")) {
					String val = ln.substr(12).strip_edges();
					if (val.length() >= 2 && val.begins_with("\"") && val.ends_with("\"")) {
						val = val.substr(1, val.length() - 2);
					}
					data->project_name = val;
					break;
				}
			}
		}
	}
	if (data->project_name.is_empty()) {
		data->project_name = data->project_path.get_file();
	}
	if (data->project_name.is_empty()) {
		data->project_name = "Voltaire Project";
	}

	if (p_viewport_rid.is_valid()) {
		data->viewport_rid = p_viewport_rid;
		data->owns_viewport = false;
	}
	else {
		data->viewport_rid = Renderer::viewport_create();
		data->owns_viewport = true;
	}

	data->canvas_rid = Renderer::canvas_create();
	Renderer::viewport_attach_canvas(data->viewport_rid, data->canvas_rid);
	Renderer::viewport_set_size(data->viewport_rid, data->window_size.x, data->window_size.y);
	Renderer::viewport_set_active(data->viewport_rid, true);
	Renderer::viewport_set_update_mode(data->viewport_rid, RSE::VIEWPORT_UPDATE_ALWAYS);
	Renderer::viewport_attach_to_screen(data->viewport_rid,
		Rect2(0, 0, data->window_size.x, data->window_size.y), DisplayServerEnums::MAIN_WINDOW_ID);

	data->canvas_item_rid = Renderer::canvas_item_create();
	Renderer::canvas_item_set_parent(data->canvas_item_rid, data->canvas_rid);
	Renderer::canvas_item_set_visible(data->canvas_item_rid, true);

	if (ThemeDB::is_initialized()) {
		data->ui_font = ThemeDB::get_fallback_font();
	}

	// Top toolbar buttons: [0]=< Project List, [1]=2D, [2]=3D, [3]=Script, [4]=+ Add Node, [5]=Save, [6]=Play
	data->top_buttons.push_back(
		{"< Project List", Rect2(), &EditorUI::_back_to_project_list, nullptr, false, false, false, false});
	data->top_buttons.push_back(
		{"2D", Rect2(), &EditorUI::_switch_view_2d, nullptr, false, false, false, true});
	data->top_buttons.push_back(
		{"3D", Rect2(), &EditorUI::_switch_view_3d, nullptr, false, false, false, false});
	data->top_buttons.push_back(
		{"Script", Rect2(), &EditorUI::_switch_view_script, nullptr, false, false, false, false});
	data->top_buttons.push_back(
		{"+ Add Node", Rect2(), &EditorUI::_add_node, nullptr, false, false, false, false});
	data->top_buttons.push_back(
		{"Save", Rect2(), &EditorUI::_save_all, nullptr, false, false, false, false});
	data->top_buttons.push_back(
		{"Play", Rect2(), &EditorUI::_toggle_play, nullptr, false, false, true, false});

	// Dock & viewport utility buttons: [0]=+ Node, [1]=+ Script, [2]=Refresh, [3]=Reset View, [4]=Clear Output
	data->dock_buttons.push_back(
		{"+ Node", Rect2(), &EditorUI::_add_node, nullptr, false, false, false, false});
	data->dock_buttons.push_back(
		{"+ Script", Rect2(), &EditorUI::_new_script_file, nullptr, false, false, false, false});
	data->dock_buttons.push_back(
		{"Refresh", Rect2(), &EditorUI::_refresh_files, nullptr, false, false, false, false});
	data->dock_buttons.push_back(
		{"Reset View", Rect2(), &EditorUI::_reset_camera_view, nullptr, false, false, false, false});
	data->dock_buttons.push_back(
		{"Clear", Rect2(), &EditorUI::_clear_output, nullptr, false, false, false, false});

	// Inspector buttons: [0..7]=pos/size nudges, [8]=Duplicate, [9]=Delete Node
	data->inspector_buttons.push_back({"-10", Rect2(), &EditorUI::_nudge_pos_x_neg,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"+10", Rect2(), &EditorUI::_nudge_pos_x_pos,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"-10", Rect2(), &EditorUI::_nudge_pos_y_neg,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"+10", Rect2(), &EditorUI::_nudge_pos_y_pos,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"-10", Rect2(), &EditorUI::_nudge_size_w_neg,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"+10", Rect2(), &EditorUI::_nudge_size_w_pos,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"-10", Rect2(), &EditorUI::_nudge_size_h_neg,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"+10", Rect2(), &EditorUI::_nudge_size_h_pos,
		&EditorUI::_can_modify_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"Duplicate", Rect2(), &EditorUI::_duplicate_selected_node,
		&EditorUI::_can_delete_selected_node, false, false, false, false});
	data->inspector_buttons.push_back({"Delete Node", Rect2(), &EditorUI::_delete_selected_node,
		&EditorUI::_can_delete_selected_node, false, false, false, false});

	// Inspector color swatches
	data->swatches.push_back({Color(0.0f, 0.75f, 0.95f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.95f, 0.40f, 0.42f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.95f, 0.75f, 0.22f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.22f, 0.80f, 0.55f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.68f, 0.48f, 0.96f, 1.0f), Rect2(), false});

	// Runtime preview toolbar buttons: [0]=Reset Sim, [1]=Open in Editor, [2]=Stop
	data->runtime_buttons.push_back(
		{"Reset Sim", Rect2(), &EditorUI::_reset_simulation, nullptr, false, false, false, false});
	data->runtime_buttons.push_back(
		{"Open in Editor", Rect2(), &EditorUI::_stop_and_edit, nullptr, false, false, false, false});
	data->runtime_buttons.push_back(
		{"Stop", Rect2(), &EditorUI::_toggle_play, nullptr, false, false, true, false});

	if (Input::is_initialized() && Input::data) {
		data->prev_input_dispatch = Input::data->event_dispatch_function;
		Input::set_event_dispatch_function(&EditorUI::handle_input_event);
	}

	load_scene();
	scan_project_files();
	String proj_godot = data->project_path.path_join("project.godot");
	if (FileAccess::exists(proj_godot)) {
		open_text_file(proj_godot);
	}

	add_log(vformat("Opened project '%s' (%s)", data->project_name, data->project_path));

	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + (p_start_playing ? " (Running) - Voltaire" : " - Voltaire Editor"),
			DisplayServerEnums::MAIN_WINDOW_ID);
	}

	_initialized = true;
	if (p_start_playing) {
		data->launched_directly_to_play = true;
		start_play();
	}
	data->needs_redraw = false;
	process_and_draw();
}

void EditorUI::start_play()
{
	if (!data) {
		return;
	}
	data->playing = true;
	data->play_time = 0.0;
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		data->entities[i].runtime_pos = data->entities[i].position;
		if (data->entities[i].runtime_vel.length_squared() < 1.0f) {
			data->entities[i].runtime_vel = Vector2(140.0f, 100.0f);
		}
	}
	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + " (Running) - Voltaire", DisplayServerEnums::MAIN_WINDOW_ID);
	}
	add_log("Started runtime preview.");
	data->needs_redraw = true;
}

void EditorUI::stop_play()
{
	if (!data) {
		return;
	}
	if (data->launched_directly_to_play) {
		data->pending_return_to_pm = true;
		return;
	}
	data->playing = false;
	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + " - Voltaire Editor", DisplayServerEnums::MAIN_WINDOW_ID);
	}
	add_log("Stopped runtime preview.");
	data->needs_redraw = true;
}

void EditorUI::handle_input_event(const Ref<InputEvent>& p_event)
{
	if (!data || p_event.is_null()) {
		return;
	}
	if (data->prev_input_dispatch && data->prev_input_dispatch != &EditorUI::handle_input_event) {
		data->prev_input_dispatch(p_event);
	}

	auto update_btn_hover = [&](LocalVector<UIButton>& r_list) -> bool {
		bool ch = false;
		for (uint32_t i = 0; i < r_list.size(); i++) {
			bool h = r_list[i].rect.has_point(data->mouse_pos);
			if (r_list[i].hovered != h) {
				r_list[i].hovered = h;
				ch = true;
			}
		}
		return ch;
	};

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Point2 prev_mouse = data->mouse_pos;
		data->mouse_pos = mm->get_position();
		Vector2 delta = data->mouse_pos - prev_mouse;

		if (data->playing) {
			if (update_btn_hover(data->runtime_buttons)) {
				data->needs_redraw = true;
			}
			return;
		}

		if (data->dragging_node_2d && _can_modify_selected_node()) {
			Vector2 vp_origin = data->center_viewport_rect.position + Vector2(24.0f, 24.0f) + data->pan_2d;
			SceneEntity& ent = data->entities[data->selected_entity_idx];
			ent.position = (data->mouse_pos - vp_origin - data->drag_node_offset).round();
			data->scene_dirty = true;
			data->needs_redraw = true;
			return;
		}

		if (data->panning_viewport) {
			if (data->view_mode == VIEW_2D) {
				data->pan_2d += delta;
				data->needs_redraw = true;
			}
			else if (data->view_mode == VIEW_3D) {
				data->orbit_yaw += delta.x * 0.01f;
				data->orbit_pitch = CLAMP(data->orbit_pitch + delta.y * 0.01f, -1.35f, 1.35f);
				data->needs_redraw = true;
			}
			return;
		}

		bool changed = false;
		changed |= update_btn_hover(data->top_buttons);
		changed |= update_btn_hover(data->dock_buttons);
		changed |= update_btn_hover(data->inspector_buttons);

		for (uint32_t i = 0; i < data->swatches.size(); i++) {
			bool h = data->swatches[i].rect.has_point(data->mouse_pos);
			if (data->swatches[i].hovered != h) {
				data->swatches[i].hovered = h;
				changed = true;
			}
		}
		for (uint32_t i = 0; i < data->entities.size(); i++) {
			bool h = data->entities[i].tree_row_rect.has_point(data->mouse_pos);
			if (data->entities[i].tree_hovered != h) {
				data->entities[i].tree_hovered = h;
				changed = true;
			}
		}
		for (uint32_t i = 0; i < data->files.size(); i++) {
			bool h = data->files[i].row_rect.has_point(data->mouse_pos);
			if (data->files[i].hovered != h) {
				data->files[i].hovered = h;
				changed = true;
			}
		}
		if (changed) {
			data->needs_redraw = true;
		}
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		data->mouse_pos = mb->get_position();

		// Scroll wheel in Script view
		if (mb->is_pressed() && !data->playing && data->view_mode == VIEW_SCRIPT &&
			data->center_viewport_rect.has_point(data->mouse_pos)) {
			if (mb->get_button_index() == MouseButton::WHEEL_UP && data->script_scroll_line > 0) {
				data->script_scroll_line = MAX(0, data->script_scroll_line - 3);
				data->needs_redraw = true;
				return;
			}
			if (mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				int max_sc = MAX(0, (int)data->script_lines.size() - 5);
				data->script_scroll_line = MIN(max_sc, data->script_scroll_line + 3);
				data->needs_redraw = true;
				return;
			}
		}

		// Right or middle mouse drag pans 2D / orbits 3D
		if (mb->get_button_index() == MouseButton::RIGHT ||
			mb->get_button_index() == MouseButton::MIDDLE) {
			if (!data->playing && mb->is_pressed() &&
				data->center_viewport_rect.has_point(data->mouse_pos)) {
				data->panning_viewport = true;
			}
			else if (!mb->is_pressed()) {
				data->panning_viewport = false;
			}
			return;
		}

		if (mb->get_button_index() != MouseButton::LEFT) {
			return;
		}

		if (data->playing) {
			if (mb->is_pressed()) {
				bool hit_btn = false;
				for (uint32_t i = 0; i < data->runtime_buttons.size(); i++) {
					UIButton& b = data->runtime_buttons[i];
					b.hovered = b.rect.has_point(data->mouse_pos);
					const bool en = (b.is_enabled == nullptr) || b.is_enabled();
					b.pressed = b.hovered && en;
					hit_btn |= b.pressed;
				}
				// Clicking in the runtime arena gives moving entities an interactive bounce impulse
				if (!hit_btn && data->center_viewport_rect.has_point(data->mouse_pos)) {
					for (uint32_t i = 1; i < data->entities.size(); i++) {
						data->entities[i].runtime_vel = Vector2(
							-data->entities[i].runtime_vel.y * 1.05f,
							data->entities[i].runtime_vel.x * 1.05f);
					}
				}
				data->needs_redraw = true;
			}
			else {
				void (*to_call)() = nullptr;
				for (uint32_t i = 0; i < data->runtime_buttons.size(); i++) {
					UIButton& b = data->runtime_buttons[i];
					bool was = b.pressed;
					b.pressed = false;
					b.hovered = b.rect.has_point(data->mouse_pos);
					const bool en = (b.is_enabled == nullptr) || b.is_enabled();
					if (was && b.hovered && en && b.on_click) {
						to_call = b.on_click;
					}
				}
				data->needs_redraw = true;
				if (to_call) {
					to_call();
				}
			}
			return;
		}

		// Editor Mode Left Click
		if (mb->is_pressed()) {
			auto press_buttons = [&](LocalVector<UIButton>& r_list) -> bool {
				bool any = false;
				for (uint32_t i = 0; i < r_list.size(); i++) {
					UIButton& b = r_list[i];
					b.hovered = b.rect.has_point(data->mouse_pos);
					const bool en = (b.is_enabled == nullptr) || b.is_enabled();
					b.pressed = b.hovered && en;
					any |= b.pressed;
				}
				return any;
			};

			bool hit_btn = false;
			hit_btn |= press_buttons(data->top_buttons);
			hit_btn |= press_buttons(data->dock_buttons);
			hit_btn |= press_buttons(data->inspector_buttons);
			if (hit_btn) {
				data->needs_redraw = true;
				return;
			}

			// Inspector node name box focus
			if (data->inspector_name_rect.has_point(data->mouse_pos) && _can_modify_selected_node()) {
				data->focus_field = FOCUS_NODE_NAME;
				data->needs_redraw = true;
				return;
			}

			// Color swatches
			if (_can_modify_selected_node()) {
				for (uint32_t i = 0; i < data->swatches.size(); i++) {
					if (data->swatches[i].rect.has_point(data->mouse_pos)) {
						data->entities[data->selected_entity_idx].color = data->swatches[i].color;
						data->scene_dirty = true;
						data->needs_redraw = true;
						return;
					}
				}
			}

			// Scene Tree rows
			for (uint32_t i = 0; i < data->entities.size(); i++) {
				if (data->entities[i].tree_row_rect.has_point(data->mouse_pos)) {
					data->selected_entity_idx = (int)i;
					data->focus_field = FOCUS_NONE;
					data->needs_redraw = true;
					return;
				}
			}

			// FileSystem rows
			for (uint32_t i = 0; i < data->files.size(); i++) {
				if (data->files[i].row_rect.has_point(data->mouse_pos)) {
					data->selected_file_idx = (int)i;
					if (!data->files[i].is_dir) {
						open_text_file(data->files[i].full_path);
						_switch_view_script();
						add_log("Opened res://" + data->files[i].name);
					}
					else {
						add_log("Folder: res://" + data->files[i].name + "/");
					}
					data->needs_redraw = true;
					return;
				}
			}

			// Center Viewport click
			if (data->center_viewport_rect.has_point(data->mouse_pos)) {
				if (data->view_mode == VIEW_SCRIPT) {
					data->focus_field = FOCUS_SCRIPT_EDITOR;
					int clicked_row =
						data->script_scroll_line +
						(int)((data->mouse_pos.y - (data->center_viewport_rect.position.y + 38.0f)) /
							  22.0f);
					if (!data->script_lines.is_empty()) {
						data->caret_line =
							CLAMP(clicked_row, 0, (int)data->script_lines.size() - 1);
					}
					data->needs_redraw = true;
					return;
				}
				if (data->view_mode == VIEW_2D) {
					data->focus_field = FOCUS_NONE;
					Vector2 vp_origin =
						data->center_viewport_rect.position + Vector2(24.0f, 24.0f) + data->pan_2d;
					// Hit-test entities front-to-back (reverse order)
					for (int i = (int)data->entities.size() - 1; i >= 1; i--) {
						Rect2 world_r(vp_origin + data->entities[i].position, data->entities[i].size);
						if (world_r.has_point(data->mouse_pos)) {
							data->selected_entity_idx = i;
							data->dragging_node_2d = true;
							data->drag_node_offset = data->mouse_pos - world_r.position;
							data->needs_redraw = true;
							return;
						}
					}
					// Empty space in 2D viewport pans
					data->panning_viewport = true;
					data->needs_redraw = true;
					return;
				}
				if (data->view_mode == VIEW_3D) {
					data->focus_field = FOCUS_NONE;
					data->panning_viewport = true;
					data->needs_redraw = true;
					return;
				}
			}

			data->focus_field = FOCUS_NONE;
			data->needs_redraw = true;
		}
		else {
			data->dragging_node_2d = false;
			data->panning_viewport = false;

			void (*to_call)() = nullptr;
			auto release_buttons = [&](LocalVector<UIButton>& r_list) {
				for (uint32_t i = 0; i < r_list.size(); i++) {
					UIButton& b = r_list[i];
					bool was = b.pressed;
					b.pressed = false;
					b.hovered = b.rect.has_point(data->mouse_pos);
					const bool en = (b.is_enabled == nullptr) || b.is_enabled();
					if (was && b.hovered && en && b.on_click) {
						to_call = b.on_click;
					}
				}
			};
			release_buttons(data->top_buttons);
			release_buttons(data->dock_buttons);
			release_buttons(data->inspector_buttons);
			data->needs_redraw = true;
			if (to_call) {
				to_call();
			}
		}
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed()) {
		const Key code = k->get_keycode();

		if (code == Key::F5 && !data->playing) {
			start_play();
			return;
		}
		if ((code == Key::F8 || code == Key::ESCAPE) && data->playing) {
			stop_play();
			return;
		}
		if (k->is_ctrl_pressed() && code == Key::S && !data->playing) {
			_save_all();
			return;
		}
		if (code == Key::ESCAPE) {
			data->focus_field = FOCUS_NONE;
			data->needs_redraw = true;
			return;
		}

		// Inspector Node Name editing
		if (data->focus_field == FOCUS_NODE_NAME && _can_modify_selected_node()) {
			String& nm = data->entities[data->selected_entity_idx].name;
			if (code == Key::ENTER || code == Key::KP_ENTER) {
				if (nm.strip_edges().is_empty()) {
					nm = "Node";
				}
				data->focus_field = FOCUS_NONE;
				data->needs_redraw = true;
				return;
			}
			if (code == Key::BACKSPACE) {
				if (!nm.is_empty()) {
					nm = nm.substr(0, nm.length() - 1);
					data->scene_dirty = true;
					data->needs_redraw = true;
				}
				return;
			}
			char32_t ch = k->get_unicode();
			if (ch >= 32 && ch != 127 && ch != '|' && !k->is_ctrl_pressed()) {
				nm += String::chr(ch);
				data->scene_dirty = true;
				data->needs_redraw = true;
				return;
			}
		}

		// Script Editor keyboard editing
		if (data->focus_field == FOCUS_SCRIPT_EDITOR && data->view_mode == VIEW_SCRIPT &&
			!data->script_lines.is_empty()) {
			data->caret_line = CLAMP(data->caret_line, 0, (int)data->script_lines.size() - 1);
			if (code == Key::UP && data->caret_line > 0) {
				data->caret_line--;
				if (data->caret_line < data->script_scroll_line) {
					data->script_scroll_line = data->caret_line;
				}
				data->needs_redraw = true;
				return;
			}
			if (code == Key::DOWN && data->caret_line + 1 < data->script_lines.size()) {
				data->caret_line++;
				data->needs_redraw = true;
				return;
			}
			if (code == Key::ENTER || code == Key::KP_ENTER) {
				data->script_lines.insert(data->caret_line + 1, "");
				data->caret_line++;
				data->script_dirty = true;
				data->needs_redraw = true;
				return;
			}
			if (code == Key::BACKSPACE) {
				String cur = data->script_lines[data->caret_line];
				if (!cur.is_empty()) {
					data->script_lines.write[data->caret_line] = cur.substr(0, cur.length() - 1);
				}
				else if (data->caret_line > 0) {
					data->script_lines.remove_at(data->caret_line);
					data->caret_line--;
				}
				data->script_dirty = true;
				data->needs_redraw = true;
				return;
			}
			if (code == Key::TAB) {
				data->script_lines.write[data->caret_line] += "    ";
				data->script_dirty = true;
				data->needs_redraw = true;
				return;
			}
			char32_t ch = k->get_unicode();
			if (ch >= 32 && ch != 127 && !k->is_ctrl_pressed() && !k->is_meta_pressed()) {
				data->script_lines.write[data->caret_line] += String::chr(ch);
				data->script_dirty = true;
				data->needs_redraw = true;
				return;
			}
		}

		// Arrow-key nudging in 2D view when no text field is focused
		if (data->focus_field == FOCUS_NONE && data->view_mode == VIEW_2D &&
			_can_modify_selected_node()) {
			float step = k->is_shift_pressed() ? 10.0f : 2.0f;
			SceneEntity& ent = data->entities[data->selected_entity_idx];
			if (code == Key::LEFT) {
				ent.position.x -= step;
				data->scene_dirty = true;
				data->needs_redraw = true;
			}
			else if (code == Key::RIGHT) {
				ent.position.x += step;
				data->scene_dirty = true;
				data->needs_redraw = true;
			}
			else if (code == Key::UP) {
				ent.position.y -= step;
				data->scene_dirty = true;
				data->needs_redraw = true;
			}
			else if (code == Key::DOWN) {
				ent.position.y += step;
				data->scene_dirty = true;
				data->needs_redraw = true;
			}
			else if (code == Key::KEY_DELETE && _can_delete_selected_node()) {
				_delete_selected_node();
			}
		}
	}
}

void EditorUI::poll(double p_delta)
{
	if (!data) {
		return;
	}

	if (Input::is_initialized()) {
		Input::flush_buffered_events();
	}

	if (data->pending_return_to_pm) {
		Size2i win_size = data->window_size;
		String last_proj = data->project_path;
		finalize();
		if (DisplayServer::is_initialized()) {
			DisplayServer::window_set_title(
				"Voltaire - Project Manager", DisplayServerEnums::MAIN_WINDOW_ID);
		}
		ProjectManager::initialize(RID(), win_size);
		if (ProjectManager::data && !last_proj.is_empty()) {
			for (uint32_t i = 0; i < ProjectManager::data->projects.size(); i++) {
				if (ProjectManager::data->projects[i].path == last_proj) {
					ProjectManager::data->selected_project_idx = (int)i;
					ProjectManager::rebuild_filtered_list();
					ProjectManager::data->needs_redraw = true;
					break;
				}
			}
		}
		return;
	}

	if (DisplayServer::is_initialized()) {
		Size2i ds_size = DisplayServer::window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
		if (ds_size.x > 0 && ds_size.y > 0 && ds_size != data->window_size) {
			data->window_size = ds_size;
			Renderer::viewport_set_size(
				data->viewport_rid, data->window_size.x, data->window_size.y);
			Renderer::viewport_attach_to_screen(data->viewport_rid,
				Rect2(0, 0, data->window_size.x, data->window_size.y),
				DisplayServerEnums::MAIN_WINDOW_ID);
			data->needs_redraw = true;
		}
	}

	if (data->playing) {
		double dt = CLAMP(p_delta, 0.001, 0.05);
		data->play_time += dt;
		data->fps_accum += dt;
		data->fps_frames++;
		if (data->fps_accum >= 0.5) {
			data->display_fps = MAX(1, (int)Math::round(data->fps_frames / data->fps_accum));
			data->fps_accum = 0.0;
			data->fps_frames = 0;
		}

		const float arena_w = MAX(160.0f, data->center_viewport_rect.size.x - 32.0f);
		const float arena_h = MAX(120.0f, data->center_viewport_rect.size.y - 32.0f);
		for (uint32_t i = 1; i < data->entities.size(); i++) {
			SceneEntity& e = data->entities[i];
			e.runtime_pos += e.runtime_vel * (float)dt;
			if (e.runtime_pos.x < 0.0f) {
				e.runtime_pos.x = 0.0f;
				e.runtime_vel.x = Math::abs(e.runtime_vel.x);
			}
			else if (e.runtime_pos.x + e.size.x > arena_w) {
				e.runtime_pos.x = MAX(0.0f, arena_w - e.size.x);
				e.runtime_vel.x = -Math::abs(e.runtime_vel.x);
			}
			if (e.runtime_pos.y < 0.0f) {
				e.runtime_pos.y = 0.0f;
				e.runtime_vel.y = Math::abs(e.runtime_vel.y);
			}
			else if (e.runtime_pos.y + e.size.y > arena_h) {
				e.runtime_pos.y = MAX(0.0f, arena_h - e.size.y);
				e.runtime_vel.y = -Math::abs(e.runtime_vel.y);
			}
		}
		data->needs_redraw = true;
	}

	if (data->needs_redraw) {
		data->needs_redraw = false;
		process_and_draw();
	}
}

void EditorUI::process_and_draw()
{
	if (!data || !data->canvas_item_rid.is_valid()) {
		return;
	}

	RID ci = data->canvas_item_rid;
	Renderer::canvas_item_clear(ci);

	const float W = (float)data->window_size.x;
	const float H = (float)data->window_size.y;
	if (W <= 8.0f || H <= 8.0f) {
		return;
	}

	const Color col_bg(0.08f, 0.09f, 0.11f, 1.0f);
	const Color col_header(0.12f, 0.14f, 0.17f, 1.0f);
	const Color col_dock(0.11f, 0.13f, 0.16f, 1.0f);
	const Color col_dock_hdr(0.14f, 0.17f, 0.21f, 1.0f);
	const Color col_vp_bg(0.06f, 0.07f, 0.09f, 1.0f);
	const Color col_accent(0.0f, 0.75f, 0.95f, 1.0f);
	const Color col_text(0.92f, 0.94f, 0.96f, 1.0f);
	const Color col_subtext(0.56f, 0.62f, 0.70f, 1.0f);
	const Color col_border(0.19f, 0.23f, 0.28f, 1.0f);

	Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), col_bg);

	const float pad = 12.0f;
	const float content_w = MAX(0.0f, W - pad * 2.0f);

	// =========================================================================
	// RUNTIME PREVIEW MODE
	// =========================================================================
	if (data->playing) {
		const float bar_h = 46.0f;
		Rect2 bar_rect(pad, pad, content_w, bar_h);
		Renderer::canvas_item_add_rect(ci, bar_rect, col_header);
		Renderer::canvas_item_add_rect(
			ci, Rect2(pad, pad, MIN(4.0f, content_w), bar_h), Color(0.22f, 0.85f, 0.52f, 1.0f));

		// Right-aligned runtime control buttons
		float rx = W - pad - 10.0f;
		for (int i = (int)data->runtime_buttons.size() - 1; i >= 0; i--) {
			UIButton& rb = data->runtime_buttons[i];
			// Show 'Open in Editor' only if launched directly to Play mode
			if (rb.on_click == &EditorUI::_stop_and_edit && !data->launched_directly_to_play) {
				rb.rect = Rect2();
				continue;
			}
			float bw = (rb.on_click == &EditorUI::_stop_and_edit) ? 124.0f : 96.0f;
			rx -= bw;
			rb.rect = Rect2(rx, pad + (bar_h - 32.0f) * 0.5f, bw, 32.0f);
			_eui_draw_button(ci, data->ui_font, rb, 13);
			rx -= 8.0f;
		}

		if (data->ui_font.is_valid()) {
			float avail = MAX(0.0f, rx - (pad + 20.0f));
			if (avail > 30.0f) {
				String run_title = vformat("RUNNING // %s   |   %d FPS   |   %.1fs",
					data->project_name, data->display_fps, data->play_time);
				data->ui_font->draw_string(ci,
					Point2(pad + 16.0f, _eui_baseline_y(data->ui_font, bar_rect, 15)), run_title,
					HORIZONTAL_ALIGNMENT_LEFT, avail, 15, col_text);
			}
		}

		float vp_y = pad + bar_h + 10.0f;
		float vp_h = MAX(0.0f, H - vp_y - pad);
		data->center_viewport_rect = Rect2(pad, vp_y, content_w, vp_h);
		_eui_draw_bordered_rect(ci, data->center_viewport_rect, col_vp_bg, col_border, 1.0f);

		Vector2 arena_origin = data->center_viewport_rect.position + Vector2(16.0f, 16.0f);
		for (uint32_t i = 1; i < data->entities.size(); i++) {
			const SceneEntity& e = data->entities[i];
			Rect2 r = Rect2(arena_origin + e.runtime_pos, e.size)
						  .intersection(data->center_viewport_rect);
			if (r.size.x > 1.0f && r.size.y > 1.0f) {
				Renderer::canvas_item_add_rect(ci, r, e.color);
				if (data->ui_font.is_valid() && r.size.x > 24.0f && r.size.y >= 20.0f) {
					data->ui_font->draw_string(ci,
						Point2(r.position.x + 6.0f, _eui_baseline_y(data->ui_font, r, 13)), e.name,
						HORIZONTAL_ALIGNMENT_CENTER, r.size.x - 12.0f, 13,
						Color(0.06f, 0.08f, 0.10f, 1.0f));
				}
			}
		}

		if (data->ui_font.is_valid() && vp_h > 40.0f && content_w > 80.0f) {
			data->ui_font->draw_string(ci,
				Point2(pad + 16.0f, vp_y + vp_h - 14.0f),
				"Click inside viewport to impulse entities  |  Press Esc or Stop to exit runtime",
				HORIZONTAL_ALIGNMENT_LEFT, content_w - 32.0f, 12, col_subtext);
		}
		return;
	}

	// =========================================================================
	// EDITOR WORKSPACE MODE
	// =========================================================================
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		data->entities[i].tree_row_rect = Rect2();
	}
	for (uint32_t i = 0; i < data->files.size(); i++) {
		data->files[i].row_rect = Rect2();
	}
	for (uint32_t i = 0; i < data->dock_buttons.size(); i++) {
		data->dock_buttons[i].rect = Rect2();
	}
	for (uint32_t i = 0; i < data->inspector_buttons.size(); i++) {
		data->inspector_buttons[i].rect = Rect2();
	}
	for (uint32_t i = 0; i < data->swatches.size(); i++) {
		data->swatches[i].rect = Rect2();
	}
	data->inspector_name_rect = Rect2();

	// 1. Top Editor Bar
	const float top_h = 46.0f;
	Rect2 top_rect(pad, pad, content_w, top_h);
	Renderer::canvas_item_add_rect(ci, top_rect, col_header);
	Renderer::canvas_item_add_rect(ci, Rect2(pad, pad, MIN(4.0f, content_w), top_h), col_accent);

	data->top_buttons[1].active_tab = (data->view_mode == VIEW_2D);
	data->top_buttons[2].active_tab = (data->view_mode == VIEW_3D);
	data->top_buttons[3].active_tab = (data->view_mode == VIEW_SCRIPT);

	const float tbtn_h = 32.0f;
	const float tbtn_y = pad + (top_h - tbtn_h) * 0.5f;

	// Left '< Project List' button
	float left_cur = pad + 10.0f;
	const float back_w = MIN(118.0f, MAX(0.0f, content_w * 0.22f));
	data->top_buttons[0].rect = Rect2(left_cur, tbtn_y, back_w, tbtn_h);
	_eui_draw_button(ci, data->ui_font, data->top_buttons[0], 13);
	left_cur += back_w + 14.0f;

	// Right action buttons: [+ Add Node] [Save] [Play]
	float right_cur = W - pad - 10.0f;
	const float right_widths[3] = {100.0f, 76.0f, 84.0f}; // indices 4, 5, 6
	for (int idx = 6; idx >= 4; idx--) {
		float bw = right_widths[idx - 4];
		if (right_cur - bw > left_cur + 40.0f) {
			right_cur -= bw;
			data->top_buttons[idx].rect = Rect2(right_cur, tbtn_y, bw, tbtn_h);
			_eui_draw_button(ci, data->ui_font, data->top_buttons[idx], 13);
			right_cur -= 8.0f;
		}
		else {
			data->top_buttons[idx].rect = Rect2();
		}
	}

	// Center workspace switcher tabs: [2D] [3D] [Script]
	const float tab_w = 64.0f;
	const float tab_gap = 6.0f;
	const float tabs_total = tab_w * 3.0f + tab_gap * 2.0f;
	float tabs_x = Math::floor((W - tabs_total) * 0.5f);
	if (tabs_x < left_cur + 8.0f) {
		tabs_x = left_cur + 8.0f;
	}
	if (tabs_x + tabs_total <= right_cur) {
		for (int t = 0; t < 3; t++) {
			data->top_buttons[1 + t].rect =
				Rect2(tabs_x + t * (tab_w + tab_gap), tbtn_y, tab_w, tbtn_h);
			_eui_draw_button(ci, data->ui_font, data->top_buttons[1 + t], 13);
		}
	}
	else {
		tabs_x = right_cur;
		for (int t = 0; t < 3; t++) {
			data->top_buttons[1 + t].rect = Rect2();
		}
	}

	// Project Title between '< Project List' and center tabs
	if (data->ui_font.is_valid()) {
		float title_w = MAX(0.0f, tabs_x - left_cur - 12.0f);
		if (title_w > 24.0f) {
			String dirty_mark = (data->scene_dirty || data->script_dirty) ? " *" : "";
			String title = "VOLTAIRE // " + data->project_name + dirty_mark;
			data->ui_font->draw_string(ci,
				Point2(left_cur, _eui_baseline_y(data->ui_font, top_rect, 15)), title,
				HORIZONTAL_ALIGNMENT_LEFT, title_w, 15, col_text);
		}
	}

	// 2. Main Body + Bottom Output Layout
	const float body_y = pad + top_h + 10.0f;
	const float avail_h = MAX(0.0f, H - body_y - pad);
	if (avail_h <= 20.0f) {
		return;
	}

	const bool show_output = avail_h >= 240.0f;
	const float output_gap = show_output ? 10.0f : 0.0f;
	const float output_h = show_output ? MIN(124.0f, avail_h * 0.24f) : 0.0f;
	const float main_h = MAX(0.0f, avail_h - output_h - output_gap);

	const bool show_left = content_w >= 620.0f;
	const bool show_right = content_w >= 820.0f;
	const float col_gap = 10.0f;
	const float left_w = show_left ? MIN(230.0f, content_w * 0.23f) : 0.0f;
	const float right_w = show_right ? MIN(250.0f, content_w * 0.25f) : 0.0f;
	const float center_x = pad + (show_left ? left_w + col_gap : 0.0f);
	const float center_w = MAX(0.0f,
		content_w - (show_left ? left_w + col_gap : 0.0f) -
			(show_right ? right_w + col_gap : 0.0f));

	// 3. Left Column: SCENE Dock (top) + FILESYSTEM Dock (bottom)
	if (show_left && main_h > 40.0f) {
		const float split_gap = 10.0f;
		const float scene_h = Math::floor((main_h - split_gap) * 0.52f);
		const float fs_h = MAX(0.0f, main_h - scene_h - split_gap);

		// SCENE Dock
		Rect2 scene_rect(pad, body_y, left_w, scene_h);
		Renderer::canvas_item_add_rect(ci, scene_rect, col_dock);
		Rect2 scene_hdr(pad, body_y, left_w, 30.0f);
		Renderer::canvas_item_add_rect(ci, scene_hdr, col_dock_hdr);
		if (data->ui_font.is_valid()) {
			data->ui_font->draw_string(ci,
				Point2(pad + 10.0f, _eui_baseline_y(data->ui_font, scene_hdr, 12)), "SCENE",
				HORIZONTAL_ALIGNMENT_LEFT, MAX(0.0f, left_w - 76.0f), 12, col_subtext);
		}
		data->dock_buttons[0].rect = Rect2(pad + left_w - 62.0f, body_y + 4.0f, 56.0f, 22.0f);
		_eui_draw_button(ci, data->ui_font, data->dock_buttons[0], 11);

		float ry = body_y + 36.0f;
		const float row_h = 26.0f;
		for (uint32_t i = 0; i < data->entities.size(); i++) {
			if (ry + row_h > body_y + scene_h - 6.0f) {
				break;
			}
			SceneEntity& ent = data->entities[i];
			const float indent = (i == 0) ? 8.0f : 22.0f;
			Rect2 rrect(pad + 6.0f, ry, left_w - 12.0f, row_h);
			ent.tree_row_rect = rrect;

			const bool is_sel = ((int)i == data->selected_entity_idx);
			if (is_sel || ent.tree_hovered) {
				Renderer::canvas_item_add_rect(ci, rrect,
					is_sel ? Color(0.18f, 0.28f, 0.38f, 1.0f) : Color(0.16f, 0.20f, 0.26f, 1.0f));
			}
			Renderer::canvas_item_add_rect(
				ci, Rect2(pad + indent, ry + 7.0f, 10.0f, 12.0f), ent.color);
			if (data->ui_font.is_valid() && left_w > indent + 28.0f) {
				String row_lbl = ent.name + "  (" + ent.type + ")";
				data->ui_font->draw_string(ci,
					Point2(pad + indent + 16.0f, _eui_baseline_y(data->ui_font, rrect, 12)),
					row_lbl, HORIZONTAL_ALIGNMENT_LEFT, left_w - indent - 26.0f, 12,
					is_sel ? col_text : Color(0.80f, 0.84f, 0.90f, 1.0f));
			}
			ry += row_h + 2.0f;
		}

		// FILESYSTEM Dock
		const float fs_y = body_y + scene_h + split_gap;
		if (fs_h > 34.0f) {
			Rect2 fs_rect(pad, fs_y, left_w, fs_h);
			Renderer::canvas_item_add_rect(ci, fs_rect, col_dock);
			Rect2 fs_hdr(pad, fs_y, left_w, 30.0f);
			Renderer::canvas_item_add_rect(ci, fs_hdr, col_dock_hdr);
			if (data->ui_font.is_valid()) {
				data->ui_font->draw_string(ci,
					Point2(pad + 10.0f, _eui_baseline_y(data->ui_font, fs_hdr, 12)), "FILESYSTEM",
					HORIZONTAL_ALIGNMENT_LEFT, MAX(0.0f, left_w - 130.0f), 12, col_subtext);
			}
			data->dock_buttons[1].rect =
				Rect2(pad + left_w - 122.0f, fs_y + 4.0f, 58.0f, 22.0f);
			data->dock_buttons[2].rect =
				Rect2(pad + left_w - 60.0f, fs_y + 4.0f, 54.0f, 22.0f);
			_eui_draw_button(ci, data->ui_font, data->dock_buttons[1], 11);
			_eui_draw_button(ci, data->ui_font, data->dock_buttons[2], 11);

			float fy = fs_y + 36.0f;
			for (uint32_t i = 0; i < data->files.size(); i++) {
				if (fy + 24.0f > fs_y + fs_h - 6.0f) {
					break;
				}
				FileEntry& fe = data->files[i];
				Rect2 frect(pad + 6.0f, fy, left_w - 12.0f, 24.0f);
				fe.row_rect = frect;
				const bool fsel = ((int)i == data->selected_file_idx) ||
								  (!fe.is_dir && fe.full_path == data->open_file_path);
				if (fsel || fe.hovered) {
					Renderer::canvas_item_add_rect(ci, frect,
						fsel ? Color(0.17f, 0.26f, 0.35f, 1.0f)
							 : Color(0.15f, 0.19f, 0.24f, 1.0f));
				}
				if (data->ui_font.is_valid() && left_w > 28.0f) {
					String disp = fe.is_dir ? ("[DIR] " + fe.name) : fe.name;
					data->ui_font->draw_string(ci,
						Point2(pad + 12.0f, _eui_baseline_y(data->ui_font, frect, 12)), disp,
						HORIZONTAL_ALIGNMENT_LEFT, left_w - 24.0f, 12,
						fe.is_dir ? col_accent : col_text);
				}
				fy += 25.0f;
			}
		}
	}

	// 4. Center Main Viewport (2D / 3D / Script)
	data->center_viewport_rect = Rect2(center_x, body_y, center_w, main_h);
	_eui_draw_bordered_rect(ci, data->center_viewport_rect, col_vp_bg, col_border, 1.0f);

	if (center_w > 40.0f && main_h > 40.0f) {
		const Rect2 vp = data->center_viewport_rect;

		if (data->view_mode == VIEW_2D) {
			const Vector2 origin = vp.position + Vector2(24.0f, 24.0f) + data->pan_2d;
			const float step = 48.0f;
			const Color grid_col(0.11f, 0.13f, 0.16f, 1.0f);

			float start_gx = vp.position.x + Math::fposmod(origin.x - vp.position.x, step);
			for (float gx = start_gx; gx < vp.position.x + vp.size.x; gx += step) {
				_eui_draw_clipped_line(ci, vp, Point2(gx, vp.position.y + 1.0f),
					Point2(gx, vp.position.y + vp.size.y - 1.0f), grid_col, 1.0f);
			}
			float start_gy = vp.position.y + Math::fposmod(origin.y - vp.position.y, step);
			for (float gy = start_gy; gy < vp.position.y + vp.size.y; gy += step) {
				_eui_draw_clipped_line(ci, vp, Point2(vp.position.x + 1.0f, gy),
					Point2(vp.position.x + vp.size.x - 1.0f, gy), grid_col, 1.0f);
			}

			// X & Y Origin Axes
			_eui_draw_clipped_line(ci, vp, Point2(vp.position.x + 1.0f, origin.y),
				Point2(vp.position.x + vp.size.x - 1.0f, origin.y),
				Color(0.55f, 0.22f, 0.26f, 1.0f), 1.5f);
			_eui_draw_clipped_line(ci, vp, Point2(origin.x, vp.position.y + 1.0f),
				Point2(origin.x, vp.position.y + vp.size.y - 1.0f),
				Color(0.20f, 0.52f, 0.34f, 1.0f), 1.5f);

			// Reference camera frame (640x360 preview frame)
			const Vector2 cam_sz(640.0f, 360.0f);
			const Color cam_col(0.28f, 0.36f, 0.52f, 1.0f);
			_eui_draw_clipped_line(
				ci, vp, origin, origin + Vector2(cam_sz.x, 0.0f), cam_col, 1.0f);
			_eui_draw_clipped_line(ci, vp, origin + Vector2(cam_sz.x, 0.0f), origin + cam_sz,
				cam_col, 1.0f);
			_eui_draw_clipped_line(
				ci, vp, origin + cam_sz, origin + Vector2(0.0f, cam_sz.y), cam_col, 1.0f);
			_eui_draw_clipped_line(
				ci, vp, origin + Vector2(0.0f, cam_sz.y), origin, cam_col, 1.0f);

			// Draw 2D scene entities
			for (uint32_t i = 1; i < data->entities.size(); i++) {
				const SceneEntity& e = data->entities[i];
				Rect2 world_r(origin + e.position, e.size);
				Rect2 clipped = world_r.intersection(vp);
				if (clipped.size.x > 1.0f && clipped.size.y > 1.0f) {
					const bool is_sel = ((int)i == data->selected_entity_idx);
					_eui_draw_bordered_rect(ci, clipped, e.color,
						is_sel ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.05f, 0.07f, 0.09f, 0.8f),
						is_sel ? 2.0f : 1.0f);
					if (data->ui_font.is_valid() && clipped.size.x > 24.0f &&
						clipped.size.y >= 18.0f) {
						data->ui_font->draw_string(ci,
							Point2(clipped.position.x + 4.0f,
								_eui_baseline_y(data->ui_font, clipped, 12)),
							e.name, HORIZONTAL_ALIGNMENT_CENTER, clipped.size.x - 8.0f, 12,
							Color(0.06f, 0.08f, 0.10f, 1.0f));
					}
				}
			}

			// Overlay badge & Reset View button
			data->dock_buttons[3].label = "Reset View";
			data->dock_buttons[3].rect =
				Rect2(vp.position.x + vp.size.x - 92.0f, vp.position.y + 8.0f, 84.0f, 26.0f);
			_eui_draw_button(ci, data->ui_font, data->dock_buttons[3], 11);
			if (data->ui_font.is_valid() && vp.size.x > 160.0f) {
				data->ui_font->draw_string(ci, Point2(vp.position.x + 12.0f, vp.position.y + 24.0f),
					"2D VIEWPORT  —  Drag nodes to move, Right-drag to pan",
					HORIZONTAL_ALIGNMENT_LEFT, vp.size.x - 120.0f, 12, col_subtext);
			}
		}
		else if (data->view_mode == VIEW_3D) {
			// Ground Grid (-5..5 on XZ plane)
			const Color gcol(0.14f, 0.17f, 0.22f, 1.0f);
			for (int g = -5; g <= 5; g++) {
				Point2 a, b;
				if (_eui_project_3d(
						Vector3((float)g, 0.0f, -5.0f), data->orbit_yaw, data->orbit_pitch, vp, a) &&
					_eui_project_3d(
						Vector3((float)g, 0.0f, 5.0f), data->orbit_yaw, data->orbit_pitch, vp, b)) {
					_eui_draw_clipped_line(ci, vp, a, b,
						g == 0 ? Color(0.24f, 0.42f, 0.78f, 1.0f) : gcol, g == 0 ? 2.0f : 1.0f);
				}
				if (_eui_project_3d(
						Vector3(-5.0f, 0.0f, (float)g), data->orbit_yaw, data->orbit_pitch, vp, a) &&
					_eui_project_3d(
						Vector3(5.0f, 0.0f, (float)g), data->orbit_yaw, data->orbit_pitch, vp, b)) {
					_eui_draw_clipped_line(ci, vp, a, b,
						g == 0 ? Color(0.78f, 0.26f, 0.30f, 1.0f) : gcol, g == 0 ? 2.0f : 1.0f);
				}
			}
			// Y-up axis
			{
				Point2 o, up;
				if (_eui_project_3d(
						Vector3(0, 0, 0), data->orbit_yaw, data->orbit_pitch, vp, o) &&
					_eui_project_3d(
						Vector3(0, 2.5f, 0), data->orbit_yaw, data->orbit_pitch, vp, up)) {
					_eui_draw_clipped_line(
						ci, vp, o, up, Color(0.25f, 0.80f, 0.45f, 1.0f), 2.0f);
				}
			}

			// Project scene entities as 3D wireframe boxes
			for (uint32_t i = 1; i < data->entities.size(); i++) {
				const SceneEntity& e = data->entities[i];
				const float cx = (e.position.x - 240.0f) / 80.0f;
				const float cz = (e.position.y - 200.0f) / 80.0f;
				const float hw = MAX(0.25f, e.size.x / 180.0f);
				const float hh = MAX(0.25f, e.size.y / 120.0f);
				Vector3 corners[8] = {
					Vector3(cx - hw, 0.0f, cz - hw),
					Vector3(cx + hw, 0.0f, cz - hw),
					Vector3(cx + hw, 0.0f, cz + hw),
					Vector3(cx - hw, 0.0f, cz + hw),
					Vector3(cx - hw, hh * 2.0f, cz - hw),
					Vector3(cx + hw, hh * 2.0f, cz - hw),
					Vector3(cx + hw, hh * 2.0f, cz + hw),
					Vector3(cx - hw, hh * 2.0f, cz + hw),
				};
				Point2 pts[8];
				bool ok = true;
				for (int c = 0; c < 8; c++) {
					if (!_eui_project_3d(
							corners[c], data->orbit_yaw, data->orbit_pitch, vp, pts[c])) {
						ok = false;
						break;
					}
				}
				if (ok) {
					const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
						{6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
					const bool is_sel = ((int)i == data->selected_entity_idx);
					Color wire_col = is_sel ? Color(1.0f, 1.0f, 1.0f, 1.0f) : e.color;
					for (int ed = 0; ed < 12; ed++) {
						_eui_draw_clipped_line(ci, vp, pts[edges[ed][0]], pts[edges[ed][1]],
							wire_col, is_sel ? 2.0f : 1.4f);
					}
				}
			}

			data->dock_buttons[3].label = "Reset Orbit";
			data->dock_buttons[3].rect =
				Rect2(vp.position.x + vp.size.x - 96.0f, vp.position.y + 8.0f, 88.0f, 26.0f);
			_eui_draw_button(ci, data->ui_font, data->dock_buttons[3], 11);
			if (data->ui_font.is_valid() && vp.size.x > 160.0f) {
				data->ui_font->draw_string(ci, Point2(vp.position.x + 12.0f, vp.position.y + 24.0f),
					"3D PERSPECTIVE VIEWPORT  —  Drag anywhere to orbit camera",
					HORIZONTAL_ALIGNMENT_LEFT, vp.size.x - 120.0f, 12, col_subtext);
			}
		}
		else {
			// SCRIPT / TEXT EDITOR VIEW
			Rect2 shdr(vp.position.x + 1.0f, vp.position.y + 1.0f, vp.size.x - 2.0f, 30.0f);
			Renderer::canvas_item_add_rect(ci, shdr, col_dock_hdr);
			if (data->ui_font.is_valid()) {
				String fname = data->open_file_name.is_empty() ? "No file open"
															   : ("res://" + data->open_file_name);
				if (data->script_dirty) {
					fname += " *";
				}
				data->ui_font->draw_string(ci,
					Point2(shdr.position.x + 12.0f, _eui_baseline_y(data->ui_font, shdr, 13)),
					fname + "   (Click below to type, Ctrl+S or Save to write to disk)",
					HORIZONTAL_ALIGNMENT_LEFT, MAX(0.0f, shdr.size.x - 24.0f), 13, col_text);
			}

			const float gutter_w = 44.0f;
			Renderer::canvas_item_add_rect(ci,
				Rect2(vp.position.x + 1.0f, vp.position.y + 31.0f, gutter_w, vp.size.y - 32.0f),
				Color(0.08f, 0.09f, 0.12f, 1.0f));

			const float line_h = 22.0f;
			float ly = vp.position.y + 36.0f;
			const int max_vis = MAX(1, (int)((vp.size.y - 42.0f) / line_h));
			for (int idx = data->script_scroll_line;
				 idx < data->script_lines.size() && (idx - data->script_scroll_line) < max_vis;
				 idx++) {
				const bool is_caret =
					(data->focus_field == FOCUS_SCRIPT_EDITOR && idx == data->caret_line);
				Rect2 lrect(vp.position.x + gutter_w + 2.0f, ly, vp.size.x - gutter_w - 6.0f, line_h);
				if (is_caret) {
					Renderer::canvas_item_add_rect(
						ci, lrect, Color(0.13f, 0.18f, 0.25f, 1.0f));
				}
				if (data->ui_font.is_valid()) {
					float by = _eui_baseline_y(data->ui_font, lrect, 13);
					data->ui_font->draw_string(ci, Point2(vp.position.x + 6.0f, by),
						itos(idx + 1), HORIZONTAL_ALIGNMENT_RIGHT, gutter_w - 12.0f, 12,
						col_subtext);
					String code_txt =
						is_caret ? (data->script_lines[idx] + "|") : data->script_lines[idx];
					Color line_col = data->script_lines[idx].strip_edges().begins_with("#") ||
											 data->script_lines[idx].strip_edges().begins_with(";")
										 ? Color(0.45f, 0.62f, 0.50f, 1.0f)
										 : col_text;
					if (lrect.size.x > 16.0f) {
						data->ui_font->draw_string(ci, Point2(lrect.position.x + 8.0f, by),
							code_txt, HORIZONTAL_ALIGNMENT_LEFT, lrect.size.x - 16.0f, 13,
							line_col);
					}
				}
				ly += line_h;
			}
		}
	}

	// 5. Right Column: INSPECTOR Dock
	if (show_right && main_h > 40.0f) {
		const float ix = center_x + center_w + col_gap;
		Rect2 insp_rect(ix, body_y, right_w, main_h);
		Renderer::canvas_item_add_rect(ci, insp_rect, col_dock);
		Rect2 insp_hdr(ix, body_y, right_w, 30.0f);
		Renderer::canvas_item_add_rect(ci, insp_hdr, col_dock_hdr);
		if (data->ui_font.is_valid()) {
			data->ui_font->draw_string(ci,
				Point2(ix + 10.0f, _eui_baseline_y(data->ui_font, insp_hdr, 12)), "INSPECTOR",
				HORIZONTAL_ALIGNMENT_LEFT, right_w - 20.0f, 12, col_subtext);
		}

		if (_can_modify_selected_node() && right_w > 60.0f) {
			const SceneEntity& sel = data->entities[data->selected_entity_idx];
			float iy = body_y + 40.0f;
			const float inner_w = right_w - 20.0f;

			if (data->ui_font.is_valid()) {
				data->ui_font->draw_string(ci, Point2(ix + 10.0f, iy + 12.0f),
					"Node Name (" + sel.type + "):", HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12,
					col_subtext);
			}
			iy += 18.0f;
			data->inspector_name_rect = Rect2(ix + 10.0f, iy, inner_w, 30.0f);
			const bool nfoc = (data->focus_field == FOCUS_NODE_NAME);
			_eui_draw_bordered_rect(ci, data->inspector_name_rect, col_vp_bg,
				nfoc ? col_accent : col_border, nfoc ? 2.0f : 1.0f);
			if (data->ui_font.is_valid()) {
				String ntxt = nfoc ? (sel.name + "|") : sel.name;
				data->ui_font->draw_string(ci,
					Point2(ix + 18.0f,
						_eui_baseline_y(data->ui_font, data->inspector_name_rect, 13)),
					ntxt, HORIZONTAL_ALIGNMENT_LEFT, inner_w - 16.0f, 13, col_text);
			}
			iy += 42.0f;

			auto draw_prop_stepper = [&](const String& p_lbl, float p_val, int p_btn_neg,
										 int p_btn_pos) {
				if (iy + 46.0f > body_y + main_h - 48.0f) {
					return;
				}
				if (data->ui_font.is_valid()) {
					data->ui_font->draw_string(ci, Point2(ix + 10.0f, iy + 11.0f), p_lbl,
						HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
				}
				iy += 16.0f;
				const float step_bw = 42.0f;
				data->inspector_buttons[p_btn_neg].rect = Rect2(ix + 10.0f, iy, step_bw, 26.0f);
				data->inspector_buttons[p_btn_pos].rect =
					Rect2(ix + 10.0f + inner_w - step_bw, iy, step_bw, 26.0f);
				_eui_draw_button(ci, data->ui_font, data->inspector_buttons[p_btn_neg], 12);
				_eui_draw_button(ci, data->ui_font, data->inspector_buttons[p_btn_pos], 12);

				Rect2 val_r(ix + 10.0f + step_bw + 6.0f, iy, inner_w - step_bw * 2.0f - 12.0f, 26.0f);
				Renderer::canvas_item_add_rect(ci, val_r, col_vp_bg);
				if (data->ui_font.is_valid() && val_r.size.x > 12.0f) {
					data->ui_font->draw_string(ci,
						Point2(val_r.position.x, _eui_baseline_y(data->ui_font, val_r, 13)),
						vformat("%.0f", p_val), HORIZONTAL_ALIGNMENT_CENTER, val_r.size.x, 13,
						col_text);
				}
				iy += 34.0f;
			};

			draw_prop_stepper("Position X", sel.position.x, 0, 1);
			draw_prop_stepper("Position Y", sel.position.y, 2, 3);
			draw_prop_stepper("Width", sel.size.x, 4, 5);
			draw_prop_stepper("Height", sel.size.y, 6, 7);

			// Color Swatches
			if (iy + 46.0f <= body_y + main_h - 48.0f) {
				if (data->ui_font.is_valid()) {
					data->ui_font->draw_string(ci, Point2(ix + 10.0f, iy + 12.0f), "Tint Color:",
						HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
				}
				iy += 18.0f;
				const float sw_gap = 8.0f;
				const float sw_w =
					(inner_w - sw_gap * (data->swatches.size() - 1)) / (float)data->swatches.size();
				for (uint32_t s = 0; s < data->swatches.size(); s++) {
					Rect2 sr(ix + 10.0f + s * (sw_w + sw_gap), iy, sw_w, 24.0f);
					data->swatches[s].rect = sr;
					bool match = sel.color.is_equal_approx(data->swatches[s].color);
					_eui_draw_bordered_rect(ci, sr, data->swatches[s].color,
						match ? Color(1, 1, 1, 1)
							  : (data->swatches[s].hovered ? col_accent : col_border),
						match ? 2.0f : 1.0f);
				}
				iy += 34.0f;
			}

			// Bottom Inspector Duplicate & Delete buttons
			const float act_y = body_y + main_h - 38.0f;
			if (act_y > iy) {
				const float half_w = (inner_w - 8.0f) * 0.5f;
				data->inspector_buttons[8].rect = Rect2(ix + 10.0f, act_y, half_w, 28.0f);
				data->inspector_buttons[9].rect =
					Rect2(ix + 10.0f + half_w + 8.0f, act_y, half_w, 28.0f);
				_eui_draw_button(ci, data->ui_font, data->inspector_buttons[8], 12);
				_eui_draw_button(ci, data->ui_font, data->inspector_buttons[9], 12);
			}
		}
	}

	// 6. Bottom OUTPUT / CONSOLE Panel
	if (show_output) {
		const float oy = body_y + main_h + output_gap;
		Rect2 out_rect(pad, oy, content_w, output_h);
		Renderer::canvas_item_add_rect(ci, out_rect, col_dock);
		Rect2 out_hdr(pad, oy, content_w, 26.0f);
		Renderer::canvas_item_add_rect(ci, out_hdr, col_dock_hdr);
		if (data->ui_font.is_valid()) {
			data->ui_font->draw_string(ci,
				Point2(pad + 10.0f, _eui_baseline_y(data->ui_font, out_hdr, 12)),
				"OUTPUT   —   Shortcuts: Ctrl+S Save  |  F5 Play  |  Arrow Keys Nudge Node",
				HORIZONTAL_ALIGNMENT_LEFT, MAX(0.0f, content_w - 90.0f), 12, col_subtext);
		}
		data->dock_buttons[4].rect =
			Rect2(pad + content_w - 64.0f, oy + 3.0f, 56.0f, 20.0f);
		_eui_draw_button(ci, data->ui_font, data->dock_buttons[4], 11);

		if (data->ui_font.is_valid()) {
			const float line_h = 18.0f;
			const int max_lines = MAX(1, (int)((output_h - 32.0f) / line_h));
			const int start_idx = MAX(0, (int)data->logs.size() - max_lines);
			float ly = oy + 30.0f;
			for (int i = start_idx; i < data->logs.size(); i++) {
				Rect2 lr(pad + 10.0f, ly, content_w - 20.0f, line_h);
				data->ui_font->draw_string(ci,
					Point2(lr.position.x, _eui_baseline_y(data->ui_font, lr, 12)),
					"> " + data->logs[i], HORIZONTAL_ALIGNMENT_LEFT, lr.size.x, 12, col_text);
				ly += line_h;
			}
		}
	}
}

void EditorUI::finalize()
{
	if (!data) {
		return;
	}
	if (Input::is_initialized() && Input::data &&
		Input::data->event_dispatch_function == &EditorUI::handle_input_event) {
		Input::set_event_dispatch_function(data->prev_input_dispatch);
	}
	if (data->canvas_item_rid.is_valid()) {
		Renderer::free_rid(data->canvas_item_rid);
	}
	if (data->canvas_rid.is_valid()) {
		Renderer::free_rid(data->canvas_rid);
	}
	if (data->owns_viewport && data->viewport_rid.is_valid()) {
		Renderer::free_rid(data->viewport_rid);
	}
	memdelete(data);
	data = nullptr;
	_initialized = false;
}

void EditorUI::_back_to_project_list()
{
	if (data) {
		data->pending_return_to_pm = true;
	}
}

void EditorUI::_switch_view_2d()
{
	if (data) {
		data->view_mode = VIEW_2D;
		data->focus_field = FOCUS_NONE;
		data->needs_redraw = true;
	}
}

void EditorUI::_switch_view_3d()
{
	if (data) {
		data->view_mode = VIEW_3D;
		data->focus_field = FOCUS_NONE;
		data->needs_redraw = true;
	}
}

void EditorUI::_switch_view_script()
{
	if (data) {
		data->view_mode = VIEW_SCRIPT;
		data->needs_redraw = true;
	}
}

void EditorUI::_add_node()
{
	if (!data) {
		return;
	}
	SceneEntity ent;
	ent.name = vformat("Sprite2D_%d", data->next_entity_serial++);
	ent.type = "Sprite2D";
	float offset = 36.0f * ((data->entities.size() % 5));
	ent.position = Vector2(140.0f + offset, 120.0f + offset);
	ent.size = Size2(88.0f, 72.0f);
	ent.color = data->swatches[data->entities.size() % data->swatches.size()].color;
	ent.runtime_vel = Vector2(120.0f + offset, -110.0f);
	data->entities.push_back(ent);
	data->selected_entity_idx = (int)data->entities.size() - 1;
	data->scene_dirty = true;
	add_log("Added node: " + ent.name);
}

void EditorUI::_save_all()
{
	if (!data) {
		return;
	}
	save_scene();
	if (!data->open_file_path.is_empty() && data->script_dirty) {
		save_open_text_file();
	}
}

void EditorUI::_toggle_play()
{
	if (!data) {
		return;
	}
	if (data->playing) {
		stop_play();
	}
	else {
		start_play();
	}
}

void EditorUI::_stop_and_edit()
{
	if (!data) {
		return;
	}
	data->launched_directly_to_play = false;
	stop_play();
}

void EditorUI::_reset_simulation()
{
	if (!data) {
		return;
	}
	data->play_time = 0.0;
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		data->entities[i].runtime_pos = data->entities[i].position;
	}
	data->needs_redraw = true;
}

void EditorUI::_reset_camera_view()
{
	if (!data) {
		return;
	}
	data->pan_2d = Vector2();
	data->orbit_yaw = 0.65f;
	data->orbit_pitch = 0.42f;
	add_log("Reset viewport camera.");
}

void EditorUI::_refresh_files()
{
	if (!data) {
		return;
	}
	scan_project_files();
	add_log(vformat("Refreshed res:// (%d items)", data->files.size()));
}

void EditorUI::_new_script_file()
{
	if (!data || data->project_path.is_empty()) {
		return;
	}
	String path = data->project_path.path_join("main.gd");
	int idx = 1;
	while (FileAccess::exists(path)) {
		path = data->project_path.path_join(vformat("script_%d.gd", idx++));
	}
	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_valid()) {
		fa->store_line("# Voltaire Script");
		fa->store_line("extends Node2D");
		fa->store_line("");
		fa->store_line("func _ready() -> void:");
		fa->store_line("    print(\"Hello from Voltaire!\")");
		fa->close();
		scan_project_files();
		open_text_file(path);
		_switch_view_script();
		add_log("Created script: res://" + path.get_file());
	}
}

void EditorUI::_clear_output()
{
	if (data) {
		data->logs.clear();
		data->needs_redraw = true;
	}
}

void EditorUI::_duplicate_selected_node()
{
	if (!_can_delete_selected_node()) {
		return;
	}
	SceneEntity copy = data->entities[data->selected_entity_idx];
	copy.name += "_Copy";
	copy.position += Vector2(24.0f, 24.0f);
	data->entities.push_back(copy);
	data->selected_entity_idx = (int)data->entities.size() - 1;
	data->scene_dirty = true;
	add_log("Duplicated node as " + copy.name);
}

void EditorUI::_delete_selected_node()
{
	if (!_can_delete_selected_node()) {
		return;
	}
	String removed = data->entities[data->selected_entity_idx].name;
	data->entities.remove_at(data->selected_entity_idx);
	data->selected_entity_idx = MIN(data->selected_entity_idx, (int)data->entities.size() - 1);
	data->scene_dirty = true;
	add_log("Removed node: " + removed);
}

void EditorUI::_nudge_pos_x_neg()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.x -= 10.0f;
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_pos_x_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.x += 10.0f;
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_pos_y_neg()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.y -= 10.0f;
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_pos_y_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.y += 10.0f;
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_size_w_neg()
{
	if (_can_modify_selected_node()) {
		SceneEntity& e = data->entities[data->selected_entity_idx];
		e.size.x = MAX(16.0f, e.size.x - 10.0f);
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_size_w_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].size.x += 10.0f;
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_size_h_neg()
{
	if (_can_modify_selected_node()) {
		SceneEntity& e = data->entities[data->selected_entity_idx];
		e.size.y = MAX(16.0f, e.size.y - 10.0f);
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}

void EditorUI::_nudge_size_h_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].size.y += 10.0f;
		data->scene_dirty = true;
		data->needs_redraw = true;
	}
}


