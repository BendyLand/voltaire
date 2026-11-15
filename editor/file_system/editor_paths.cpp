/**************************************************************************/
/*  editor_paths.cpp                                                      */
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor_paths.h"
#include "main/main.h"

bool EditorPaths::are_paths_valid() { return data ? data->paths_valid : false; }

String EditorPaths::get_data_dir() { return data ? data->data_dir : String(); }

String EditorPaths::get_config_dir() { return data ? data->config_dir : String(); }

String EditorPaths::get_cache_dir() { return data ? data->cache_dir : String(); }

String EditorPaths::get_temp_dir() { return data ? data->temp_dir : String(); }

String EditorPaths::get_project_data_dir() { return data ? data->project_data_dir : String(); }

bool EditorPaths::is_self_contained() { return data ? data->self_contained : false; }

String EditorPaths::get_self_contained_file()
{
	return data ? data->self_contained_file : String();
}

String EditorPaths::get_export_templates_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_data_dir().path_join(data->export_templates_folder);
}

String EditorPaths::get_debug_keystore_path()
{
#ifdef ANDROID_ENABLED
	return "assets://keystores/debug.keystore";
#else
	return get_data_dir().path_join("keystores/debug.keystore");
#endif
}

// This returns paths like "res://.godot/editor".
String EditorPaths::get_project_settings_dir()
{
	return get_project_data_dir().path_join("editor");
}

String EditorPaths::get_text_editor_themes_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_config_dir().path_join(data->text_editor_themes_folder);
}

String EditorPaths::get_script_templates_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_config_dir().path_join(data->script_templates_folder);
}

String EditorPaths::get_feature_profiles_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_config_dir().path_join(data->feature_profiles_folder);
}

void EditorPaths::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void EditorPaths::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);

	data->project_data_dir = ProjectSettings::get_project_data_path();

	// Self-contained mode if a `._sc_` or `_sc_` file is present in executable dir.
	String exe_path = OS::get_executable_path().get_base_dir();
	Ref<DirAccess> d = DirAccess::create_for_path(exe_path);
	if (d->file_exists(exe_path + "/._sc_")) {
		data->self_contained = true;
		data->self_contained_file = exe_path + "/._sc_";
	}
	else if (d->file_exists(exe_path + "/_sc_")) {
		data->self_contained = true;
		data->self_contained_file = exe_path + "/_sc_";
	}

	// On macOS, look outside .app bundle, since .app bundle is read-only.
	// Note: This will not work if Gatekeeper path randomization is active.
	if (OS::has_feature("macos") && exe_path.ends_with("MacOS") &&
		exe_path.path_join("..").simplify_path().ends_with("Contents")) {
		exe_path = exe_path.path_join("../../..").simplify_path();
		d = DirAccess::create_for_path(exe_path);
		if (d->file_exists(exe_path + "/._sc_")) {
			data->self_contained = true;
			data->self_contained_file = exe_path + "/._sc_";
		}
		else if (d->file_exists(exe_path + "/_sc_")) {
			data->self_contained = true;
			data->self_contained_file = exe_path + "/_sc_";
		}
	}

	String data_path;
	String config_path;
	String cache_path;

	if (data->self_contained) {
		// editor is self contained, all in same folder
		data_path = exe_path;
		data->data_dir = data_path.path_join("editor_data");
		config_path = exe_path;
		data->config_dir = data->data_dir;
		cache_path = exe_path;
		data->cache_dir = data->data_dir.path_join("cache");
		data->temp_dir = data->data_dir.path_join("temp");
	}
	else {
		// Typically XDG_DATA_HOME or %APPDATA%.
		data_path = OS::get_data_path();
		data->data_dir = data_path.path_join(OS::get_godot_dir_name());
		// Can be different from data_path e.g. on Linux or macOS.
		config_path = OS::get_config_path();
		data->config_dir = config_path.path_join(OS::get_godot_dir_name());
		// Can be different from above paths, otherwise a subfolder of data_dir.
		cache_path = OS::get_cache_path();
		if (cache_path == data_path) {
			data->cache_dir = data->data_dir.path_join("cache");
		}
		else {
			data->cache_dir = cache_path.path_join(OS::get_godot_dir_name());
		}
		data->temp_dir = OS::get_temp_path();
	}

	data->paths_valid =
		(!data_path.is_empty() && !config_path.is_empty() && !cache_path.is_empty());
	ERR_FAIL_COND_MSG(!data->paths_valid, "Editor data, config, or cache paths are invalid.");

	// Validate or create each dir and its relevant subdirectories.

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	// Data dir.
	{
		if (dir->change_dir(data->data_dir) != OK) {
			dir->make_dir_recursive(data->data_dir);
			if (dir->change_dir(data->data_dir) != OK) {
				ERR_PRINT("Could not create editor data directory: " + data->data_dir);
				data->paths_valid = false;
			}
		}

		if (!dir->dir_exists(data->export_templates_folder)) {
			dir->make_dir(data->export_templates_folder);
		}
	}

	// Config dir.
	{
		if (dir->change_dir(data->config_dir) != OK) {
			dir->make_dir_recursive(data->config_dir);
			if (dir->change_dir(data->config_dir) != OK) {
				ERR_PRINT("Could not create editor config directory: " + data->config_dir);
				data->paths_valid = false;
			}
		}

		if (!dir->dir_exists(data->text_editor_themes_folder)) {
			dir->make_dir(data->text_editor_themes_folder);
		}
		if (!dir->dir_exists(data->script_templates_folder)) {
			dir->make_dir(data->script_templates_folder);
		}
		if (!dir->dir_exists(data->feature_profiles_folder)) {
			dir->make_dir(data->feature_profiles_folder);
		}
	}

	// Cache dir.
	{
		if (dir->change_dir(data->cache_dir) != OK) {
			dir->make_dir_recursive(data->cache_dir);
			if (dir->change_dir(data->cache_dir) != OK) {
				ERR_PRINT("Could not create editor cache directory: " + data->cache_dir);
				data->paths_valid = false;
			}
		}
	}

	// Temporary dir.
	{
		if (dir->change_dir(data->temp_dir) != OK) {
			dir->make_dir_recursive(data->temp_dir);
			if (dir->change_dir(data->temp_dir) != OK) {
				ERR_PRINT("Could not create editor temporary directory: " + data->temp_dir);
				data->paths_valid = false;
			}
		}
	}

	// Validate or create project-specific editor data dir,
	// including shader cache subdir.
	if (Engine::is_project_manager_hint() ||
		(Main::is_cmdline_tool() && !ProjectSettings::is_project_loaded())) {
		// Nothing to create, use shared editor data dir for shader cache.
		Engine::set_shader_cache_path(data->data_dir);
	}
	else {
		Ref<DirAccess> dir_res = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		if (dir_res->change_dir(data->project_data_dir) != OK) {
			dir_res->make_dir_recursive(data->project_data_dir);
			if (dir_res->change_dir(data->project_data_dir) != OK) {
				ERR_PRINT("Could not create project data directory (" + data->project_data_dir +
						  ") in: " + dir_res->get_current_dir());
				data->paths_valid = false;
			}
		}

		// Check that the project data directory `.gdignore` file exists.
		String project_data_gdignore_file_path = data->project_data_dir.path_join(".gdignore");
		if (!FileAccess::exists(project_data_gdignore_file_path)) {
			// Add an empty .gdignore file to avoid scan.
			Ref<FileAccess> f =
				FileAccess::open(project_data_gdignore_file_path, FileAccess::WRITE);
			if (f.is_valid()) {
				f->store_line("");
			}
			else {
				ERR_PRINT("Failed to create file " + project_data_gdignore_file_path.quote() + ".");
			}
		}

		Engine::set_shader_cache_path(data->project_data_dir);

		// Editor metadata dir.
		if (!dir_res->dir_exists("editor")) {
			dir_res->make_dir("editor");
		}
		// Imported assets dir.
		String imported_files_path = ProjectSettings::get_imported_files_path();
		if (!dir_res->dir_exists(imported_files_path)) {
			dir_res->make_dir(imported_files_path);
		}
	}
}


