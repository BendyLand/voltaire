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

bool EditorProgress::step(const String& p_state, int p_step, bool p_force_refresh)
{
	if (!force_background && Thread::is_main_thread()) {
		return EditorNode::progress_task_step(task, p_state, p_step, p_force_refresh);
	}
	else {
		EditorNode::progress_task_step_bg(task, p_step);
		return false;
	}
}

EditorProgress::EditorProgress(const String& p_task, const String& p_label, int p_amount,
	bool p_can_cancel, bool p_force_background)
{
	if (!p_force_background && Thread::is_main_thread()) {
		EditorNode::progress_add_task(p_task, p_label, p_amount, p_can_cancel);
	}
	else {
		EditorNode::progress_add_task_bg(p_task, p_label, p_amount);
	}
	task = p_task;
	force_background = p_force_background;
}

EditorProgress::~EditorProgress()
{
	if (!force_background && Thread::is_main_thread()) {
		EditorNode::progress_end_task(task);
	}
	else {
		EditorNode::progress_end_task_bg(task);
	}
}

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
	case VCS_METADATA: {
		VersionControlEditorPlugin::get_singleton()->popup_vcs_metadata_dialog();
	} break;
	case VCS_SETTINGS: {
		VersionControlEditorPlugin::get_singleton()->popup_vcs_set_up_dialog(gui_base);
	} break;
	}
}

void EditorNode::_update_unsaved_cache()
{
	bool is_unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(
						  EditorUndoRedoManager::GLOBAL_HISTORY) ||
					  EditorUndoRedoManager::get_singleton()->is_history_unsaved(
						  editor_data.get_current_edited_scene_history_id());

	if (unsaved_cache != is_unsaved) {
		unsaved_cache = is_unsaved;
		_update_title();
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

void EditorNode::_update_system_menu_icons(bool p_dark_mode)
{
	file_menu->set_item_icon(file_menu->get_item_index(SCENE_NEW_SCENE),
		get_editor_theme_native_menu_icon(
			SNAME("CreateNewSceneFrom"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	file_menu->set_item_icon(file_menu->get_item_index(SCENE_OPEN_SCENE),
		get_editor_theme_native_menu_icon(
			SNAME("PackedScene"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	file_menu->set_item_icon(file_menu->get_item_index(SCENE_SAVE_SCENE),
		get_editor_theme_native_menu_icon(
			SNAME("Save"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	file_menu->set_item_icon(file_menu->get_item_index(SCENE_QUICK_OPEN),
		get_editor_theme_native_menu_icon(
			SNAME("Load"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	file_menu->set_item_icon(file_menu->get_item_index(SCENE_UNDO),
		get_editor_theme_native_menu_icon(
			SNAME("RotateLeft"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	file_menu->set_item_icon(file_menu->get_item_index(SCENE_CLOSE),
		get_editor_theme_native_menu_icon(
			SNAME("CloseScene"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		file_menu->set_item_icon(
			file_menu->get_item_index(SCENE_QUIT), get_editor_theme_native_menu_icon(SNAME("Close"),
													   menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	}
#else
	file_menu->set_item_icon(
		file_menu->get_item_index(SCENE_QUIT), get_editor_theme_native_menu_icon(SNAME("Close"),
												   menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
#endif

	project_menu->set_item_icon(project_menu->get_item_index(PROJECT_OPEN_SETTINGS),
		get_editor_theme_native_menu_icon(
			SNAME("ClassList"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	project_menu->set_item_icon(project_menu->get_item_index(PROJECT_EXPORT),
		get_editor_theme_native_menu_icon(
			SNAME("ResourcePreloader"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	project_menu->set_item_icon(project_menu->get_item_index(PROJECT_QUIT_TO_PROJECT_MANAGER),
		get_editor_theme_native_menu_icon(
			SNAME("Close"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));

#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		settings_menu->set_item_icon(settings_menu->get_item_index(EDITOR_OPEN_SETTINGS),
			get_editor_theme_native_menu_icon(
				SNAME("Tools"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	}
	else {
		apple_menu->set_item_icon(apple_menu->get_item_index(EDITOR_OPEN_SETTINGS),
			get_editor_theme_native_menu_icon(
				SNAME("Tools"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	}
#else
	settings_menu->set_item_icon(settings_menu->get_item_index(EDITOR_OPEN_SETTINGS),
		get_editor_theme_native_menu_icon(
			SNAME("Tools"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
#endif

	help_menu->set_item_icon(help_menu->get_item_index(HELP_SEARCH),
		get_editor_theme_native_menu_icon(
			SNAME("HelpSearch"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	help_menu->set_item_icon(help_menu->get_item_index(HELP_COPY_SYSTEM_INFO),
		get_editor_theme_native_menu_icon(
			SNAME("ActionCopy"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		help_menu->set_item_icon(
			help_menu->get_item_index(HELP_ABOUT), get_editor_theme_native_menu_icon(SNAME("Godot"),
													   menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
	}
#else
	help_menu->set_item_icon(
		help_menu->get_item_index(HELP_ABOUT), get_editor_theme_native_menu_icon(SNAME("Godot"),
												   menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
#endif
	help_menu->set_item_icon(help_menu->get_item_index(HELP_SUPPORT_GODOT_DEVELOPMENT),
		get_editor_theme_native_menu_icon(
			SNAME("Heart"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode));
}

void EditorNode::update_preview_themes(int p_mode)
{
	if (!scene_root->is_inside_tree()) {
		return; // Too early.
	}

	Vector<Ref<Theme>> preview_themes;

	switch (p_mode) {
	case CanvasItemEditor::THEME_PREVIEW_PROJECT:
		preview_themes.push_back(ThemeDB::get_singleton()->get_project_theme());
		break;

	case CanvasItemEditor::THEME_PREVIEW_EDITOR:
		preview_themes.push_back(get_editor_theme());
		break;

	default:
		break;
	}

	preview_themes.push_back(ThemeDB::get_singleton()->get_default_theme());

	ThemeContext* preview_context = ThemeDB::get_singleton()->get_theme_context(scene_root);
	if (preview_context) {
		preview_context->set_themes(preview_themes);
	}
	else {
		ThemeDB::get_singleton()->create_theme_context(scene_root, preview_themes);
	}
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

void EditorNode::_fs_changed()
{
	for (FileDialog* E : file_dialogs) {
		E->invalidate();
	}

	_mark_unsaved_scenes();

	// FIXME: Move this to a cleaner location, it's hacky to do this in _fs_changed.
	String export_error;
	Error err = OK;
	// It's important to wait for the first scan to finish; otherwise, scripts or resources might
	// not be imported.
	if (!export_defer.preset.is_empty() && !EditorFileSystem::get_singleton()->is_scanning()) {
		String preset_name = export_defer.preset;
		// Ensures export_project does not loop infinitely, because notifications may
		// come during the export.
		export_defer.preset = "";
		Ref<EditorExportPreset> export_preset;
		for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); ++i) {
			export_preset = EditorExport::get_singleton()->get_export_preset(i);
			if (export_preset->get_name() == preset_name) {
				break;
			}
			export_preset.unref();
		}

		if (export_preset.is_null()) {
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
			if (da->file_exists("res://export_presets.cfg")) {
				err = FAILED;
				export_error = vformat("Invalid export preset name: %s.\nThe following presets "
									   "were detected in this project's `export_presets.cfg`:\n\n",
					preset_name);
				for (int i = 0; i < EditorExport::get_singleton()->get_export_preset_count(); ++i) {
					// Write the preset name between double quotes since it needs to be written
					// between quotes on the command line if it contains spaces.
					export_error += vformat("        \"%s\"\n",
						EditorExport::get_singleton()->get_export_preset(i)->get_name());
				}
			}
			else {
				err = FAILED;
				export_error =
					"This project doesn't have an `export_presets.cfg` file at its root.\nCreate "
					"an export preset from the \"Project > Export\"dialog and try again.";
			}
		}
		else {
			Ref<EditorExportPlatform> platform = export_preset->get_platform();
			const String export_path =
				export_defer.path.is_empty() ? export_preset->get_export_path() : export_defer.path;
			if (export_path.is_empty()) {
				err = FAILED;
				export_error = vformat("Export preset \"%s\" doesn't have a default export path, "
									   "and none was specified.",
					preset_name);
			}
			else if (platform.is_null()) {
				err = FAILED;
				export_error =
					vformat("Export preset \"%s\" doesn't have a matching platform.", preset_name);
			}
			else {
				export_preset->update_value_overrides();
				if (export_defer.pack_only) { // Only export .pck or .zip data pack.
					if (export_path.ends_with(".zip")) {
						if (export_defer.patch) {
							err = platform->export_zip_patch(export_preset, export_defer.debug,
								export_path, export_defer.patches);
						}
						else {
							err = platform->export_zip(
								export_preset, export_defer.debug, export_path);
						}
					}
					else if (export_path.ends_with(".pck")) {
						if (export_defer.patch) {
							err = platform->export_pack_patch(export_preset, export_defer.debug,
								export_path, export_defer.patches);
						}
						else {
							err = platform->export_pack(
								export_preset, export_defer.debug, export_path);
						}
					}
					else {
						ERR_PRINT(
							vformat("Export path \"%s\" doesn't end with a supported extension.",
								export_path));
						err = FAILED;
					}
				}
				else { // Normal project export.
					String config_error;
					bool missing_templates;
					if (export_defer.android_build_template) {
						export_template_manager->install_android_template(export_preset);
					}
					if (!platform->can_export(
							export_preset, config_error, missing_templates, export_defer.debug)) {
						ERR_PRINT(vformat("Cannot export project with preset \"%s\" due to "
										  "configuration errors:\n%s",
							preset_name, config_error));
						err = missing_templates ? ERR_FILE_NOT_FOUND : ERR_UNCONFIGURED;
					}
					else {
						platform->clear_messages();
						err = platform->export_project(
							export_preset, export_defer.debug, export_path);
					}
				}
				if (err != OK) {
					export_error = vformat("Project export for preset \"%s\" failed.", preset_name);
				}
				else if (platform->get_worst_message_type() >=
						   EditorExportPlatform::EXPORT_MESSAGE_WARNING) {
					export_error = vformat(
						"Project export for preset \"%s\" completed with warnings.", preset_name);
				}
			}
		}

		if (err != OK) {
			ERR_PRINT(export_error);
			_exit_editor(EXIT_FAILURE);
			return;
		}
		if (!export_error.is_empty()) {
			WARN_PRINT(export_error);
		}
		_exit_editor(EXIT_SUCCESS);
	}
}

void EditorNode::_resources_reimporting(const Vector<String>& p_resources)
{
	// This will copy all the modified properties of the nodes into 'scenes_modification_table'
	// before they are actually reimported. It's important to do this before the reimportation
	// because if a mesh is present in an inherited scene, the resource will be modified in
	// the inherited scene. Then, get_modified_properties_for_node will return the mesh property,
	// which will trigger a recopy of the previous mesh, preventing the reload.
	scenes_modification_table.clear();
	scenes_reimported.clear();
	resources_reimported.clear();
	EditorFileSystem* editor_file_system = EditorFileSystem::get_singleton();
	for (const String& res_path : p_resources) {
		// It's faster to use EditorFileSystem::get_file_type than fetching the resource type from
		// disk. This makes a big difference when reimporting many resources.
		String file_type = editor_file_system->get_file_type(res_path);
		if (file_type.is_empty()) {
			file_type = ResourceLoader::get_resource_type(res_path);
		}
		if (file_type == "PackedScene") {
			scenes_reimported.push_back(res_path);
		}
		else {
			resources_reimported.push_back(res_path);
		}
	}

	if (scenes_reimported.size() > 0) {
		preload_reimporting_with_path_in_edited_scenes(scenes_reimported);
	}
}

void EditorNode::_resources_reimported(const Vector<String>& p_resources)
{
	int current_tab = scene_tabs->get_current_tab();

	for (const String& res_path : resources_reimported) {
		if (!ResourceCache::has(res_path)) {
			// Not loaded, no need to reload.
			continue;
		}
		// Reload normally.
		Ref<Resource> resource = ResourceCache::get_ref(res_path);
		if (resource.is_valid()) {
			resource->reload_from_file();
		}
	}

	// Editor may crash when related animation is playing while re-importing GLTF scene, stop it in
	// advance.
	AnimationPlayer* ap = AnimationPlayerEditor::get_singleton()->get_player();
	if (ap && scenes_reimported.size() > 0) {
		ap->stop(true);
	}

	// Only refresh the current scene tab if it's been reimported.
	// Otherwise the scene tab will try to grab focus unnecessarily.
	bool should_refresh_current_scene_tab = false;
	const String current_scene_tab = editor_data.get_scene_path(current_tab);
	for (const String& E : scenes_reimported) {
		if (!should_refresh_current_scene_tab && E == current_scene_tab) {
			should_refresh_current_scene_tab = true;
		}
		if (editor_data.get_edited_scene_from_path(E) != -1) {
			reload_scene(E);
		}
	}

	reload_instances_with_path_in_edited_scenes();

	scenes_modification_table.clear();
	scenes_reimported.clear();
	resources_reimported.clear();

	if (should_refresh_current_scene_tab) {
		_set_current_scene_nocheck(current_tab);
	}
}

void EditorNode::_remove_lock_file() { OS::get_singleton()->remove_lock_file(); }

void EditorNode::_reload_modified_scenes()
{
	int current_idx = editor_data.get_edited_scene();

	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == "") {
			continue;
		}

		uint64_t last_date = editor_data.get_scene_modified_time(i);
		uint64_t date = FileAccess::get_modified_time(editor_data.get_scene_path(i));

		if (date > last_date) {
			String filename = editor_data.get_scene_path(i);
			editor_data.set_edited_scene(i);
			_remove_edited_scene(false);

			Error err = open_scene(filename);
			if (err != OK) {
				ERR_PRINT(vformat("Failed to load scene: %s", filename));
			}
			editor_data.move_edited_scene_to_index(i);
		}
	}

	_set_current_scene(current_idx);
	scene_tabs->update_scene_tabs();
	disk_changed->hide();
}

void EditorNode::_reload_project_settings()
{
	ProjectSettings::get_singleton()->setup(
		ProjectSettings::get_singleton()->get_resource_path(), String(), true, true);
}

void EditorNode::_vp_resized() {}

void EditorNode::_titlebar_resized()
{
	DisplayServer::get_singleton()->window_set_window_buttons_offset(
		Vector2i(title_bar->get_global_position().y + title_bar->get_size().y / 2,
			title_bar->get_global_position().y + title_bar->get_size().y / 2),
		DisplayServerEnums::MAIN_WINDOW_ID);
	const Vector3i& margin = DisplayServer::get_singleton()->window_get_safe_title_margins(
		DisplayServerEnums::MAIN_WINDOW_ID);
	if (left_menu_spacer) {
		int w = (gui_base->is_layout_rtl()) ? margin.y : margin.x;
		left_menu_spacer->set_custom_minimum_size(Size2(w, 0));
	}
	if (right_menu_spacer) {
		int w = (gui_base->is_layout_rtl()) ? margin.x : margin.y;
		right_menu_spacer->set_custom_minimum_size(Size2(w, 0));
	}
	if (title_bar) {
		title_bar->set_custom_minimum_size(Size2(0, margin.z - title_bar->get_global_position().y));
	}
}

void EditorNode::_update_undo_redo_allowed()
{
	EditorUndoRedoManager* undo_redo = EditorUndoRedoManager::get_singleton();
	file_menu->set_item_disabled(file_menu->get_item_index(SCENE_UNDO), !undo_redo->has_undo());
	file_menu->set_item_disabled(file_menu->get_item_index(SCENE_REDO), !undo_redo->has_redo());
}

void EditorNode::_node_renamed()
{
	if (InspectorDock::get_inspector_singleton()) {
		InspectorDock::get_inspector_singleton()->update_tree();
	}
}

void EditorNode::_open_command_palette() { command_palette->open_popup(); }

Error EditorNode::load_scene_or_resource(
	const String& p_path, bool p_ignore_broken_deps, bool p_change_scene_tab_if_already_open)
{
	return EditorNode::get_singleton()->load_resource(p_path, p_ignore_broken_deps);
}

void EditorNode::edit_resource(const Ref<Resource>& p_resource)
{
	InspectorDock::get_singleton()->edit_resource(p_resource);
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

void EditorNode::clear_node_reference(Ref<Resource> p_res)
{
	if (is_resource_internal_to_scene(p_res)) {
		return;
	}
	List<Node*>* node_list = resource_count.getptr(p_res);
	if (node_list != nullptr) {
		node_list->clear();
	}
}

void EditorNode::_menu_option(int p_option) { _menu_option_confirm(p_option, false); }

void EditorNode::_menu_confirm_current() { _menu_option_confirm(current_menu_option, true); }

void EditorNode::trigger_menu_option(int p_option, bool p_confirmed)
{
	_menu_option_confirm(p_option, p_confirmed);
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

void EditorNode::save_all_scenes()
{
	project_run_bar->stop_playing();
	_save_all_scenes();
}

void EditorNode::restart_editor(bool p_goto_project_manager)
{
	_menu_option_confirm(
		p_goto_project_manager ? PROJECT_QUIT_TO_PROJECT_MANAGER : PROJECT_RELOAD_CURRENT_PROJECT,
		false);
}

void EditorNode::_mark_unsaved_scenes()
{
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		Node* node = editor_data.get_edited_scene_root(i);
		if (!node) {
			continue;
		}

		String path = node->get_scene_file_path();
		if (!path.is_empty() && !FileAccess::exists(path)) {
			// Mark scene tab as unsaved if the file is gone.
			EditorUndoRedoManager::get_singleton()->set_history_as_unsaved(
				editor_data.get_scene_history_id(i));
		}
	}

	_update_title();
	scene_tabs->update_scene_tabs();
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

void EditorNode::edit_previous_item()
{
	if (editor_history.previous()) {
		_edit_current();
	}
}

void EditorNode::_android_build_source_selected(const String& p_file)
{
	export_template_manager->install_android_template_from_file(p_file, android_export_preset);
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

void EditorNode::_request_screenshot() { _screenshot(); }

void EditorNode::_check_system_theme_changed()
{
	DisplayServer* display_server = DisplayServer::get_singleton();

	bool system_theme_changed = false;

	if (follow_system_theme) {
		if (display_server->get_base_color() != last_system_base_color) {
			system_theme_changed = true;
			last_system_base_color = display_server->get_base_color();
		}

		if (display_server->is_dark_mode_supported() &&
			display_server->is_dark_mode() != last_dark_mode_state) {
			system_theme_changed = true;
			last_dark_mode_state = display_server->is_dark_mode();
		}
	}

	if (use_system_accent_color) {
		if (display_server->get_accent_color() != last_system_accent_color) {
			system_theme_changed = true;
			last_system_accent_color = display_server->get_accent_color();
		}
	}

	if (system_theme_changed) {
		class_icon_cache.clear();
		_update_theme();
		_build_icon_type_cache();
		recent_scenes->reset_size();
	}
	else if (menu_type == MENU_TYPE_GLOBAL && display_server->is_dark_mode_supported() &&
			   display_server->is_dark_mode() != last_dark_mode_state) {
		last_dark_mode_state = display_server->is_dark_mode();

		// Update system menus.
		bool dark_mode = DisplayServer::get_singleton()->is_dark_mode();

		_update_system_menu_icons(dark_mode);

		editor_dock_manager->update_docks_menu();
	}
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

void EditorNode::unload_editor_addons()
{
	for (const KeyValue<String, EditorPlugin*>& E : addon_name_to_plugin) {
		print_verbose(vformat("Unloading addon: %s", E.key));
		remove_editor_plugin(E.value, false);
		memdelete(E.value);
	}

	addon_name_to_plugin.clear();
}

void EditorNode::_discard_changes(const String& p_str)
{
	switch (current_menu_option) {
	case SCENE_CLOSE:
	case SCENE_TAB_CLOSE: {
		const String path = editor_data.get_scene_path(tab_closing_idx);
		if (!path.is_empty()) {
			_update_prev_closed_scenes(path, true);
		}

		// Don't close tabs when exiting the editor (required for "restore_scenes_on_load" setting).
		if (!_is_closing_editor()) {
			_remove_scene(tab_closing_idx);
			scene_tabs->update_scene_tabs();
		}
		_proceed_closing_scene_tabs();
	} break;
	case SCENE_RELOAD_SAVED_SCENE: {
		int cur_idx = editor_data.get_edited_scene();
		reload_scene(editor_data.get_scene_path(cur_idx));
		confirmation->hide();
	} break;
	case SCENE_QUIT: {
		project_run_bar->stop_playing();
		_exit_editor(EXIT_SUCCESS);

	} break;
	case PROJECT_QUIT_TO_PROJECT_MANAGER: {
		_restart_editor(true);
	} break;
	case PROJECT_RELOAD_CURRENT_PROJECT: {
		_restart_editor();
	} break;
	}
}

void EditorNode::_update_file_menu_opened()
{
	bool has_unsaved = false;
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (is_scene_unsaved(i)) {
			has_unsaved = true;
			break;
		}
	}
	if (has_unsaved) {
		file_menu->set_item_disabled(file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), false);
		file_menu->set_item_tooltip(file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), String());
	}
	else {
		file_menu->set_item_disabled(file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), true);
		file_menu->set_item_tooltip(
			file_menu->get_item_index(SCENE_SAVE_ALL_SCENES), TTR("All scenes are already saved."));
	}
	_update_undo_redo_allowed();
}

void EditorNode::add_extension_editor_plugin(const StringName& p_class_name) {}

void EditorNode::remove_extension_editor_plugin(const StringName& p_class_name)
{
	// If we're exiting, the editor plugins will get cleaned up anyway, so don't do anything.
	if (!singleton || singleton->exiting) {
		return;
	}

	ERR_FAIL_COND_MSG(!singleton->editor_data.has_extension_editor_plugin(p_class_name),
		vformat("No editor plugin added for class: %s", p_class_name));

	EditorPlugin* plugin = singleton->editor_data.get_extension_editor_plugin(p_class_name);
	remove_editor_plugin(plugin);
	memdelete(plugin);
	singleton->editor_data.remove_extension_editor_plugin(p_class_name);
}

bool EditorNode::is_addon_plugin_enabled(const String& p_addon) const
{
	if (p_addon.begins_with("res://")) {
		return addon_name_to_plugin.has(p_addon);
	}

	return addon_name_to_plugin.has("res://addons/" + p_addon + "/plugin.cfg");
}

void EditorNode::_remove_scene(int p_idx, bool p_change_tab)
{
	// Clear icon cache in case some scripts are no longer needed or class icons are outdated.
	// FIXME: Ideally the cache should never be cleared and only updated on per-script basis, when
	// an icon changes.
	editor_data.clear_script_icon_cache();
	class_icon_cache.clear();

	_save_editor_states(editor_data.get_scene_path(p_idx), p_idx);
	if (editor_data.get_edited_scene() == p_idx) {
		// Scene to remove is current scene.
		_remove_edited_scene(p_change_tab);
	}
	else {
		// Scene to remove is not active scene.
		editor_data.remove_scene(p_idx);
	}
}

void EditorNode::set_edited_scene(Node* p_scene) { set_edited_scene_root(p_scene, true); }

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

void EditorNode::_set_current_scene(int p_idx)
{
	if (p_idx == editor_data.get_edited_scene()) {
		return; // Pointless.
	}

	_set_current_scene_nocheck(p_idx);
}

bool EditorNode::is_scene_open(const String& p_path)
{
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == p_path) {
			return true;
		}
	}

	return false;
}

int EditorNode::new_scene()
{
	int idx = editor_data.add_edited_scene(-1);
	_set_current_scene(idx); // Before trying to remove an empty scene, set the current tab index to
							 // the newly added tab index.

	// Remove placeholder empty scene.
	if (editor_data.get_edited_scene_count() > 1) {
		for (int i = 0; i < editor_data.get_edited_scene_count() - 1; i++) {
			bool unsaved = EditorUndoRedoManager::get_singleton()->is_history_unsaved(
				editor_data.get_scene_history_id(i));
			if (!unsaved && editor_data.get_scene_path(i).is_empty() &&
				editor_data.get_edited_scene_root(i) == nullptr) {
				editor_data.remove_scene(i);
				idx--;
			}
		}
	}

	editor_data.clear_editor_states();
	scene_tabs->update_scene_tabs();
	return idx;
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

void EditorNode::request_instantiate_scene(const String& p_path)
{
	SceneTreeDock::get_singleton()->instantiate(p_path);
}

void EditorNode::request_instantiate_scenes(const Vector<String>& p_files)
{
	SceneTreeDock::get_singleton()->instantiate_scenes(p_files);
}

void EditorNode::_inherit_request(String p_file)
{
	current_menu_option = SCENE_NEW_INHERITED_SCENE;
	_dialog_action(p_file);
}

void EditorNode::_instantiate_request(const Vector<String>& p_files)
{
	request_instantiate_scenes(p_files);
}

void EditorNode::_close_messages()
{
	old_split_ofs = center_split->get_split_offset();
	center_split->set_split_offset(0);
}

void EditorNode::_show_messages() { center_split->set_split_offset(old_split_ofs); }

void EditorNode::_update_prev_closed_scenes(const String& p_scene_path, bool p_add_scene)
{
	if (!p_scene_path.is_empty()) {
		if (p_add_scene) {
			prev_closed_scenes.push_back(p_scene_path);
		}
		else {
			prev_closed_scenes.erase(p_scene_path);
		}
		file_menu->set_item_disabled(
			file_menu->get_item_index(SCENE_OPEN_PREV), prev_closed_scenes.is_empty());
	}
}

void EditorNode::_quick_opened(const String& p_file_path) { load_scene_or_resource(p_file_path); }

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

bool EditorNode::close_scene()
{
	int tab_index = editor_data.get_edited_scene();
	if (tab_index == 0 && get_edited_scene() == nullptr &&
		editor_data.get_scene_path(tab_index).is_empty()) {
		return false;
	}

	tab_closing_idx = tab_index;
	current_menu_option = SCENE_CLOSE;
	_discard_changes();
	changing_scene = false;
	return true;
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

void EditorNode::_pick_main_scene_custom_action(const String& p_custom_action_name)
{
	if (p_custom_action_name == "select_current") {
		Node* scene = editor_data.get_edited_scene_root();

		if (!scene) {
			show_warning(TTR("There is no defined scene to run."));
			return;
		}

		pick_main_scene->hide();

		if (!FileAccess::exists(scene->get_scene_file_path())) {
			current_menu_option = SAVE_AND_RUN_MAIN_SCENE;
			_menu_option_confirm(SCENE_SAVE_AS_SCENE, true);
			file->set_title(TTR("Save scene before running..."));
		}
		else {
			current_menu_option = SETTINGS_PICK_MAIN_SCENE;
			_dialog_action(scene->get_scene_file_path());
		}
	}
}

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

void EditorNode::_enable_pending_addons()
{
	for (uint32_t i = 0; i < pending_addons.size(); i++) {
		set_addon_plugin_enabled(pending_addons[i], true);
	}
	pending_addons.clear();
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

void EditorNode::_save_editor_layout()
{
	if (!load_editor_layout_done) {
		return;
	}
	Ref<ConfigFile> config;
	config.instantiate();
	// Load and amend existing config if it exists.
	config->load(
		EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));

	editor_dock_manager->save_docks_to_config(config, "docks");
	_save_open_scenes_to_config(config);
	_save_central_editor_layout_to_config(config);
	_save_window_settings_to_config(config, "EditorWindow");
	editor_data.get_plugin_window_layout(config);

	config->save(
		EditorPaths::get_singleton()->get_project_settings_dir().path_join("editor_layout.cfg"));
}

void EditorNode::save_editor_layout_delayed() { editor_layout_save_delay_timer->start(); }

void EditorNode::undo() { _menu_option_confirm(SCENE_UNDO, true); }

void EditorNode::redo() { _menu_option_confirm(SCENE_REDO, true); }

void EditorNode::_immediate_dialog_confirmed() { immediate_dialog_confirmed = true; }

bool EditorNode::is_cmdline_mode()
{
	ERR_FAIL_NULL_V(singleton, false);
	return singleton->cmdline_mode;
}

void EditorNode::cleanup() { _init_callbacks.clear(); }

void EditorNode::_update_layouts_menu()
{
	editor_layouts->clear();
	overridden_default_layout = false;

	editor_layouts->reset_size();
	editor_layouts->add_shortcut(ED_SHORTCUT("layout/save", TTRC("Save Layout...")), LAYOUT_SAVE);
	editor_layouts->add_shortcut(
		ED_SHORTCUT("layout/delete", TTRC("Delete Layout...")), LAYOUT_DELETE);
	editor_layouts->add_separator();

	Ref<ConfigFile> config;
	config.instantiate();
	Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
	if (err == OK && config->has_section("Default")) {
		overridden_default_layout = true;
	}

	editor_layouts->add_shortcut(
		ED_SHORTCUT("layout/default",
			overridden_default_layout ? TTRC("Default (Overridden)") : TTRC("Default")),
		LAYOUT_DEFAULT);

	if (err != OK) {
		return; // No config.
	}

	Vector<String> layouts = config->get_sections();
	for (const String& layout : layouts) {
		if (layout != "Default" && !layout.contains_char('/')) {
			editor_layouts->add_item(layout);
			editor_layouts->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_DISABLED);
		}
	}
}

void EditorNode::_layout_menu_option(int p_id)
{
	switch (p_id) {
	case LAYOUT_DEFAULT: {
		// Check if the default layout was overridden, and if so, select that instead.
		Ref<ConfigFile> config;
		config.instantiate();
		Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
		if (err == OK && config->has_section("Default")) {
			editor_dock_manager->load_docks_from_config(config, "Default");
			_save_editor_layout();

			return;
		}

		editor_dock_manager->load_docks_from_config(default_layout, "docks");
		_save_editor_layout();
	} break;

	default: {
		Ref<ConfigFile> config;
		config.instantiate();
		Error err = config->load(EditorSettings::get_singleton()->get_editor_layouts_config());
		if (err == OK) {
			editor_dock_manager->load_docks_from_config(
				config, editor_layouts->get_item_text(p_id));
			_save_editor_layout();
		}
	}
	}
}

void EditorNode::_proceed_closing_scene_tabs()
{
	List<String>::Element* E = tabs_to_close.front();
	if (!E) {
		if (_is_closing_editor()) {
			current_menu_option = tab_closing_menu_option;
			_menu_option_confirm(tab_closing_menu_option, true);
		}
		else {
			current_menu_option = -1;
			save_confirmation->hide();
		}
		return;
	}
	String scene_to_close = E->get();
	tabs_to_close.pop_front();

	int tab_idx = -1;
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == scene_to_close) {
			tab_idx = i;
			break;
		}
	}
	ERR_FAIL_COND(tab_idx < 0);

	_scene_tab_closed(tab_idx);
}

void EditorNode::_proceed_save_asing_scene_tabs()
{
	if (scenes_to_save_as.is_empty()) {
		return;
	}
	int scene_idx = scenes_to_save_as.front()->get();
	scenes_to_save_as.pop_front();
	_set_current_scene(scene_idx);
	_menu_option_confirm(SCENE_MULTI_SAVE_AS_SCENE, false);
}

bool EditorNode::_is_closing_editor() const
{
	return tab_closing_menu_option == SCENE_QUIT ||
		   tab_closing_menu_option == PROJECT_QUIT_TO_PROJECT_MANAGER ||
		   tab_closing_menu_option == PROJECT_RELOAD_CURRENT_PROJECT;
}

void EditorNode::_restart_editor(bool p_goto_project_manager)
{
	exiting = true;

	if (project_run_bar->is_playing()) {
		project_run_bar->stop_playing();
	}

	String to_reopen;
	if (!p_goto_project_manager && get_tree()->get_edited_scene_root()) {
		to_reopen = get_tree()->get_edited_scene_root()->get_scene_file_path();
	}

	_exit_editor(EXIT_SUCCESS);

	List<String> args;
	for (const String& a : Main::get_forwardable_cli_arguments(Main::CLI_SCOPE_TOOL)) {
		args.push_back(a);
	}

	if (p_goto_project_manager) {
		args.push_back("--project-manager");

		// Setup working directory.
		const String exec_dir = OS::get_singleton()->get_executable_path().get_base_dir();
		if (!exec_dir.is_empty()) {
			args.push_back("--path");
			args.push_back(exec_dir);
		}

		List<String>::Element* vbf = args.find("--verbose");
		if (vbf) {
			args.erase(vbf);
		}
	}
	else {
		args.push_back("--path");
		args.push_back(ProjectSettings::get_singleton()->get_resource_path());

		args.push_back("-e");
	}

	if (!to_reopen.is_empty()) {
		args.push_back(to_reopen);
	}

	OS::get_singleton()->set_restart_on_exit(true, args);
}

void EditorNode::_scene_tab_closed(int p_tab)
{
	current_menu_option = SCENE_TAB_CLOSE;
	tab_closing_idx = p_tab;
	Node* scene = editor_data.get_edited_scene_root(p_tab);
	if (!scene) {
		_discard_changes();
		return;
	}

	String scene_filename = scene->get_scene_file_path();
	String unsaved_message;

	if (EditorUndoRedoManager::get_singleton()->is_history_unsaved(
			editor_data.get_scene_history_id(p_tab))) {
		if (scene_filename.is_empty()) {
			unsaved_message = TTR("This scene was never saved.");
		}
		else {
			uint32_t time_opened = editor_data.get_scene_time_opened(p_tab);
			unsaved_message = _get_unsaved_scene_dialog_text(scene_filename, time_opened);
		}
	}
	else {
		// Check if any plugin has unsaved changes in that scene.
		for (int i = 0; i < editor_data.get_editor_plugin_count(); i++) {
			unsaved_message = editor_data.get_editor_plugin(i)->get_unsaved_status(scene_filename);
			if (!unsaved_message.is_empty()) {
				break;
			}
		}
	}

	if (!unsaved_message.is_empty()) {
		save_confirmation->set_ok_button_text(TTR("Save & Close"));
		save_confirmation->set_text(unsaved_message + "\n\n" + TTR("Save before closing?"));
		save_confirmation->reset_size();
		save_confirmation->popup_centered();
	}
	else {
		_discard_changes();
	}

	save_editor_layout_delayed();
	scene_tabs->update_scene_tabs();
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

void EditorNode::add_tool_submenu_item(const String& p_name, PopupMenu* p_submenu)
{
	ERR_FAIL_NULL(p_submenu);
	ERR_FAIL_COND(p_submenu->get_parent() != nullptr);
	tool_menu->add_submenu_node_item(p_name, p_submenu, TOOLS_CUSTOM);
}

void EditorNode::remove_tool_menu_item(const String& p_name)
{
	for (int i = 0; i < tool_menu->get_item_count(); i++) {
		if (tool_menu->get_item_id(i) != TOOLS_CUSTOM) {
			continue;
		}

		if (tool_menu->get_item_text(i) == p_name) {
			if (tool_menu->get_item_submenu(i) != "") {
				Node* n = tool_menu->get_node(tool_menu->get_item_submenu(i));
				tool_menu->remove_child(n);
				memdelete(n);
			}
			tool_menu->remove_item(i);
			tool_menu->reset_size();
			return;
		}
	}
}

PopupMenu* EditorNode::get_export_as_menu() { return export_as_menu; }

void EditorNode::_dropped_files(const Vector<String>& p_files)
{
	String to_path = FileSystemDock::get_singleton()->get_folder_path_at_mouse_position();
	if (to_path.is_empty()) {
		to_path = FileSystemDock::get_singleton()->get_current_directory();
	}
	to_path = ProjectSettings::get_singleton()->globalize_path(to_path);

	_add_dropped_files_recursive(p_files, to_path);

	EditorFileSystem::get_singleton()->scan_changes();
}

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

void EditorNode::_file_access_close_error_notify_impl(const String& p_str)
{
	add_io_error(vformat(
		TTR("Unable to write to file '%s', file in use, locked or lacking permissions."), p_str));
}

// Recursive function to inform nodes that an array of nodes have had their scene reimported.
// It will attempt to call a method named '_nodes_scene_reimported' on every node in the
// tree so that editor scripts which create transient nodes will have the opportunity
// to recreate them.

void EditorNode::reload_scene(const String& p_path)
{
	int scene_idx = -1;

	const String lpath = ProjectSettings::get_singleton()->localize_path(p_path);
	for (int i = 0; i < editor_data.get_edited_scene_count(); i++) {
		if (editor_data.get_scene_path(i) == lpath) {
			scene_idx = i;
			break;
		}
	}
	ERR_FAIL_COND_MSG(
		scene_idx == -1, vformat("Can't reload scene %s, as it's not opened.", p_path));

	int current_tab = editor_data.get_edited_scene();
	bool is_current_scene = current_tab == scene_idx;
	if (is_current_scene) {
		editor_data.apply_changes_in_editors();
	}

	// Reload scene.
	_remove_scene(scene_idx, false);
	Error err = load_scene(p_path, true, false, false, false);
	if (err != OK) {
		return;
	}

	// Adjust index so tab is back a the previous position.
	editor_data.move_scene_to_index(editor_data.get_edited_scene_count() - 1, scene_idx);
	EditorUndoRedoManager::get_singleton()->clear_history(
		editor_data.get_scene_history_id(scene_idx), false);

	// Recover the current tab.
	if (is_current_scene) {
		_set_current_scene_nocheck(current_tab, true);
	}
	else {
		editor_data.set_edited_scene(current_tab);
		scene_tabs->update_scene_tabs();
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

void EditorNode::preload_reimporting_with_path_in_edited_scenes(const List<String>& p_scenes)
{
	EditorProgress progress("preload_reimporting_scene", TTR("Preparing scenes for reload"),
		editor_data.get_edited_scene_count());

	int original_edited_scene_idx = editor_data.get_edited_scene();

	// Walk through each opened scene to get a global list of all instances which match
	// the current reimported scenes.
	for (int current_scene_idx = 0; current_scene_idx < editor_data.get_edited_scene_count();
		 current_scene_idx++) {
		progress.step(
			vformat(TTR("Analyzing scene %s"), editor_data.get_scene_title(current_scene_idx)),
			current_scene_idx);

		Node* edited_scene_root = editor_data.get_edited_scene_root(current_scene_idx);

		if (edited_scene_root) {
			SceneModificationsEntry scene_modifications;

			for (const String& instance_path : p_scenes) {
				if (editor_data.get_scene_path(current_scene_idx) == instance_path) {
					continue;
				}

				HashSet<Node*> instances_to_reimport;
				find_all_instances_inheriting_path_in_node(
					edited_scene_root, edited_scene_root, instance_path, instances_to_reimport);
				if (instances_to_reimport.size() > 0) {
					editor_data.set_edited_scene(current_scene_idx);

					List<Node*> instance_list_with_children;
					for (Node* original_node : instances_to_reimport) {
						InstanceModificationsEntry instance_modifications;

						// Fetching all the modified properties of the nodes reimported scene.
						get_preload_scene_modification_table(edited_scene_root, original_node,
							original_node, instance_modifications);

						instance_modifications.original_node = original_node;
						instance_modifications.instance_path = instance_path;
						scene_modifications.instance_list.push_back(instance_modifications);

						instance_list_with_children.push_back(original_node);
						get_children_nodes(original_node, instance_list_with_children);
					}

					// Search the scene to find nodes that references the nodes will be recreated.
					get_preload_modifications_reference_to_nodes(edited_scene_root,
						edited_scene_root, instances_to_reimport, instance_list_with_children,
						scene_modifications.other_instances_modifications);
				}
			}

			if (scene_modifications.instance_list.size() > 0) {
				scenes_modification_table[current_scene_idx] = scene_modifications;
			}
		}
	}

	editor_data.set_edited_scene(original_edited_scene_idx);

	progress.step(TTR("Preparation done."), editor_data.get_edited_scene_count());
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

void EditorNode::_inherit_imported(const String& p_action)
{
	open_imported->hide();
	open_scene(open_import_request, true, true);
}

void EditorNode::_open_imported() { open_scene(open_import_request, true, false, true); }

void EditorNode::dim_editor(bool p_dimming)
{
	dimmed = p_dimming;
	gui_base->set_modulate(p_dimming ? Color(0.5, 0.5, 0.5) : Color(1, 1, 1));
}

bool EditorNode::is_editor_dimmed() const { return dimmed; }

void EditorNode::open_export_template_manager() { export_template_manager->popup_manager(); }

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

void EditorNode::_resource_saved(Ref<Resource> p_resource, const String& p_path)
{
	if (singleton->saving_resources_in_path.has(p_resource)) {
		// This is going to be handled by save_resource_in_path when the time is right.
		return;
	}

	if (EditorFileSystem::get_singleton()) {
		EditorFileSystem::get_singleton()->update_file(p_path);
	}

	singleton->editor_folding.save_resource_folding(p_resource, p_path);
}

void EditorNode::_resource_loaded(Ref<Resource> p_resource, const String& p_path)
{
	singleton->editor_folding.load_resource_folding(p_resource, p_path);
}

void EditorNode::_feature_profile_changed()
{
	Ref<EditorFeatureProfile> profile = feature_profile_manager->get_current_profile();
	if (profile.is_valid()) {
		editor_dock_manager->set_dock_enabled(SignalsDock::get_singleton(),
			!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_SIGNALS_DOCK));
		editor_dock_manager->set_dock_enabled(GroupsDock::get_singleton(),
			!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GROUPS_DOCK));
		// The Import dock is useless without the FileSystem dock. Ensure the configuration is
		// valid.
		bool fs_dock_disabled =
			profile->is_feature_disabled(EditorFeatureProfile::FEATURE_FILESYSTEM_DOCK);
		editor_dock_manager->set_dock_enabled(FileSystemDock::get_singleton(), !fs_dock_disabled);
		editor_dock_manager->set_dock_enabled(ImportDock::get_singleton(),
			!fs_dock_disabled &&
				!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_IMPORT_DOCK));
		editor_dock_manager->set_dock_enabled(history_dock,
			!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_HISTORY_DOCK));

		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_3D,
			!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_3D));
		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_SCRIPT,
			!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_SCRIPT));
		if (!Engine::get_singleton()->is_recovery_mode_hint()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_GAME,
				!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GAME));
		}
		if (AssetLibraryEditorPlugin::is_available()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_ASSETLIB,
				!profile->is_feature_disabled(EditorFeatureProfile::FEATURE_ASSET_LIB));
		}
	}
	else {
		editor_dock_manager->set_dock_enabled(ImportDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(SignalsDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(GroupsDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(FileSystemDock::get_singleton(), true);
		editor_dock_manager->set_dock_enabled(history_dock, true);
		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_3D, true);
		editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_SCRIPT, true);
		if (!Engine::get_singleton()->is_recovery_mode_hint()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_GAME, true);
		}
		if (AssetLibraryEditorPlugin::is_available()) {
			editor_main_screen->set_button_enabled(EditorMainScreen::EDITOR_ASSETLIB, true);
		}
	}

	editor_dock_manager->update_docks_menu();
}

static Node* _resource_get_edited_scene()
{
	return EditorNode::get_singleton()->get_edited_scene();
}

void EditorNode::_print_handler_impl(const String& p_string, bool p_error, bool p_rich)
{
	if (!singleton) {
		return;
	}
	if (p_error) {
		singleton->log->add_message(p_string, EditorLog::MSG_TYPE_ERROR);
	}
	else if (p_rich) {
		singleton->log->add_message(p_string, EditorLog::MSG_TYPE_STD_RICH);
	}
	else {
		singleton->log->add_message(p_string, EditorLog::MSG_TYPE_STD);
	}
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

void EditorNode::_build_help_menu(bool p_dark_mode)
{
	if (!help_menu) {
		return;
	}
	help_menu->clear(false);

	if (menu_type == MENU_TYPE_GLOBAL &&
		NativeMenu::get_singleton()->has_system_menu(NativeMenu::HELP_MENU_ID)) {
		help_menu->set_system_menu(NativeMenu::HELP_MENU_ID);
	}
	else {
		help_menu->set_system_menu(NativeMenu::INVALID_MENU_ID);
	}

	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("HelpSearch"),
									 menu_type == MENU_TYPE_GLOBAL, p_dark_mode),
		ED_GET_SHORTCUT("editor/editor_help"), HELP_SEARCH);
	help_menu->add_separator();
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/online_docs"), HELP_DOCS);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/forum"), HELP_FORUM);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/community"), HELP_COMMUNITY);
	help_menu->add_separator();
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("ActionCopy"),
									 menu_type == MENU_TYPE_GLOBAL, p_dark_mode),
		ED_GET_SHORTCUT("editor/copy_system_info"), HELP_COPY_SYSTEM_INFO);
	help_menu->set_item_tooltip(
		-1, TTRC("Copies the system info as a single-line text into the clipboard."));
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/report_a_bug"), HELP_REPORT_A_BUG);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/suggest_a_feature"), HELP_SUGGEST_A_FEATURE);
	help_menu->add_shortcut(ED_GET_SHORTCUT("editor/send_docs_feedback"), HELP_SEND_DOCS_FEEDBACK);
	help_menu->add_separator();
#ifdef MACOS_ENABLED
	if (menu_type != MENU_TYPE_GLOBAL) {
		// On macOS "About" option is in the "app" menu.
		help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(SNAME("Godot"),
										 menu_type == MENU_TYPE_GLOBAL, p_dark_mode),
			ED_GET_SHORTCUT("editor/about"), HELP_ABOUT);
	}
#else
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(
									 SNAME("Godot"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode),
		ED_GET_SHORTCUT("editor/about"), HELP_ABOUT);
#endif
	help_menu->add_icon_shortcut(get_editor_theme_native_menu_icon(
									 SNAME("Heart"), menu_type == MENU_TYPE_GLOBAL, p_dark_mode),
		ED_GET_SHORTCUT("editor/support_development"), HELP_SUPPORT_GODOT_DEVELOPMENT);
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

void EditorNode::open_setting_override(const String& p_property)
{
	editor_settings_dialog->hide();
	project_settings_editor->popup_for_override(p_property);
}

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


