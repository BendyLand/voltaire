/**************************************************************************/
/*  plugin_config_apple_embedded.cpp                                      */
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

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "plugin_config_apple_embedded.h"

String PluginConfigAppleEmbedded::resolve_local_dependency_path(
	String plugin_config_dir, String dependency_path)
{
	String absolute_path;

	if (dependency_path.is_empty()) {
		return absolute_path;
	}

	if (dependency_path.is_absolute_path()) {
		return dependency_path;
	}

	String res_path = ProjectSettings::get_singleton()->globalize_path("res://");
	absolute_path = plugin_config_dir.path_join(dependency_path);

	return absolute_path.replace(res_path, "res://");
}

String PluginConfigAppleEmbedded::resolve_system_dependency_path(String dependency_path)
{
	String absolute_path;

	if (dependency_path.is_empty()) {
		return absolute_path;
	}

	if (dependency_path.is_absolute_path()) {
		return dependency_path;
	}

	String system_path = "/System/Library/Frameworks";

	return system_path.path_join(dependency_path);
}

Vector<String> PluginConfigAppleEmbedded::resolve_local_dependencies(
	String plugin_config_dir, Vector<String> p_paths)
{
	Vector<String> paths;

	for (int i = 0; i < p_paths.size(); i++) {
		String path = resolve_local_dependency_path(plugin_config_dir, p_paths[i]);

		if (path.is_empty()) {
			continue;
		}

		paths.push_back(path);
	}

	return paths;
}

Vector<String> PluginConfigAppleEmbedded::resolve_system_dependencies(Vector<String> p_paths)
{
	Vector<String> paths;

	for (int i = 0; i < p_paths.size(); i++) {
		String path = resolve_system_dependency_path(p_paths[i]);

		if (path.is_empty()) {
			continue;
		}

		paths.push_back(path);
	}

	return paths;
}

bool PluginConfigAppleEmbedded::validate_plugin(PluginConfigAppleEmbedded& plugin_config)
{
	bool valid_name = !plugin_config.name.is_empty();
	bool valid_binary_name = !plugin_config.binary.is_empty();
	bool valid_initialize = !plugin_config.initialization_method.is_empty();
	bool valid_deinitialize = !plugin_config.deinitialization_method.is_empty();

	bool fields_value = valid_name && valid_binary_name && valid_initialize && valid_deinitialize;

	if (!fields_value) {
		return false;
	}

	String plugin_extension = plugin_config.binary.get_extension().to_lower();

	if ((plugin_extension == "a" && FileAccess::exists(plugin_config.binary)) ||
		(plugin_extension == "xcframework" && DirAccess::exists(plugin_config.binary))) {
		plugin_config.valid_config = true;
		plugin_config.supports_targets = false;
	}
	else {
		String file_path = plugin_config.binary.get_base_dir();
		String file_name = plugin_config.binary.get_basename().get_file();
		String file_extension = plugin_config.binary.get_extension();
		String release_file_name = file_path.path_join(file_name + ".release." + file_extension);
		String debug_file_name = file_path.path_join(file_name + ".debug." + file_extension);

		if ((plugin_extension == "a" && FileAccess::exists(release_file_name) &&
				FileAccess::exists(debug_file_name)) ||
			(plugin_extension == "xcframework" && DirAccess::exists(release_file_name) &&
				DirAccess::exists(debug_file_name))) {
			plugin_config.valid_config = true;
			plugin_config.supports_targets = true;
		}
	}

	return plugin_config.valid_config;
}

String PluginConfigAppleEmbedded::get_plugin_main_binary(
	PluginConfigAppleEmbedded& plugin_config, bool p_debug)
{
	if (!plugin_config.supports_targets) {
		return plugin_config.binary;
	}

	String plugin_binary_dir = plugin_config.binary.get_base_dir();
	String plugin_name_prefix = plugin_config.binary.get_basename().get_file();
	String plugin_extension = plugin_config.binary.get_extension();
	String plugin_file =
		plugin_name_prefix + "." + (p_debug ? "debug" : "release") + "." + plugin_extension;

	return plugin_binary_dir.path_join(plugin_file);
}

uint64_t PluginConfigAppleEmbedded::get_plugin_modification_time(
	const PluginConfigAppleEmbedded& plugin_config, const String& config_path)
{
	uint64_t last_updated = FileAccess::get_modified_time(config_path);

	if (!plugin_config.supports_targets) {
		last_updated = MAX(last_updated, FileAccess::get_modified_time(plugin_config.binary));
	}
	else {
		String file_path = plugin_config.binary.get_base_dir();
		String file_name = plugin_config.binary.get_basename().get_file();
		String plugin_extension = plugin_config.binary.get_extension();
		String release_file_name = file_path.path_join(file_name + ".release." + plugin_extension);
		String debug_file_name = file_path.path_join(file_name + ".debug." + plugin_extension);

		last_updated = MAX(last_updated, FileAccess::get_modified_time(release_file_name));
		last_updated = MAX(last_updated, FileAccess::get_modified_time(debug_file_name));
	}

	return last_updated;
}


