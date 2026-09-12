/**************************************************************************/
/*  shader_create_dialog.cpp                                              */
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
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/editor_node.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_validation_panel.h"
#include "editor/settings/editor_settings.h"
#include "editor/shader/editor_shader_language_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/shader_include.h"
#include "servers/rendering/shader_types.h"
#include "shader_create_dialog.h"

void ShaderCreateDialog::_refresh_type_icons()
{
	for (int i = 0; i < type_menu->get_item_count(); i++) {
		const String item_name = type_menu->get_item_text(i);
		Ref<Texture2D> icon = get_editor_theme_icon(item_name);
		if (icon.is_valid()) {
			type_menu->set_item_icon(i, icon);
		}
		else {
			icon = get_editor_theme_icon("TextFile");
			if (icon.is_valid()) {
				type_menu->set_item_icon(i, icon);
			}
		}
	}
}

void ShaderCreateDialog::_update_language_info()
{
	type_data.clear();

	for (int i = 0; i < EditorShaderLanguagePlugin::get_shader_language_variation_count(); i++) {
		ShaderTypeData shader_type_data;
		if (i == 0) {
			// HACK: The ShaderCreateDialog class currently only shows templates for text shaders.
			// Generalize this later.
			shader_type_data.use_templates = true;
		}
		shader_type_data.default_extension =
			EditorShaderLanguagePlugin::get_file_extension_for_index(i);
		shader_type_data.extensions.push_back(shader_type_data.default_extension);
		if (shader_type_data.default_extension != "tres") {
			shader_type_data.extensions.push_back("tres");
		}
		shader_type_data.extensions.push_back("res");
		type_data.push_back(shader_type_data);
	}
}

void ShaderCreateDialog::_path_hbox_sorted()
{
	if (is_visible()) {
		int filename_start_pos = initial_base_path.rfind_char('/') + 1;
		int filename_end_pos = initial_base_path.length();

		if (!is_built_in) {
			file_path->select(filename_start_pos, filename_end_pos);
		}

		file_path->set_caret_column(file_path->get_text().length());
		file_path->set_caret_column(filename_start_pos);

		file_path->grab_focus();
	}
}

void ShaderCreateDialog::_built_in_toggled(bool p_enabled)
{
	is_built_in = p_enabled;
	if (p_enabled) {
		is_new_shader_created = true;
	}
	else {
		_path_changed(file_path->get_text());
	}
	validation_panel->update();
}

void ShaderCreateDialog::_file_selected(const String& p_file)
{
	String p = ProjectSettings::get_singleton()->localize_path(p_file);
	file_path->set_text(p);
	_path_changed(p);

	String filename = p.get_file().get_basename();
	int select_start = p.rfind(filename);
	file_path->select(select_start, select_start + filename.length());
	file_path->set_caret_column(select_start + filename.length());
	file_path->grab_focus();
}

void ShaderCreateDialog::_path_changed(const String& p_path)
{
	if (is_built_in) {
		return;
	}

	is_path_valid = false;
	is_new_shader_created = true;

	path_error = _validate_path(p_path);
	if (!path_error.is_empty()) {
		validation_panel->update();
		return;
	}

	Ref<DirAccess> f = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	String p = ProjectSettings::get_singleton()->localize_path(p_path.strip_edges());
	if (f->file_exists(p)) {
		is_new_shader_created = false;
	}

	is_path_valid = true;
	validation_panel->update();
}

void ShaderCreateDialog::_path_submitted(const String& p_path)
{
	if (!get_ok_button()->is_disabled()) {
		ok_pressed();
	}
}

String ShaderCreateDialog::_validate_path(const String& p_path)
{
	ERR_FAIL_COND_V(current_type >= type_data.size(), TTR("Invalid shader type selected."));
	String stripped_file_path = p_path.strip_edges();

	if (stripped_file_path.is_empty()) {
		return TTRC("Path is empty.");
	}
	if (stripped_file_path.get_file().get_basename().is_empty()) {
		return TTRC("Filename is empty.");
	}

	stripped_file_path = ProjectSettings::get_singleton()->localize_path(stripped_file_path);
	if (!stripped_file_path.begins_with("res://")) {
		return TTRC("Path is not local.");
	}

	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (d->change_dir(stripped_file_path.get_base_dir()) != OK) {
		return TTRC("Invalid base path.");
	}

	Ref<DirAccess> f = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (f->dir_exists(stripped_file_path)) {
		return TTRC("A directory with the same name exists.");
	}

	const ShaderCreateDialog::ShaderTypeData& current_type_data = type_data.get(current_type);
	const String file_extension = stripped_file_path.get_extension();

	for (const String& type_ext : current_type_data.extensions) {
		if (type_ext.nocasecmp_to(file_extension) == 0) {
			return "";
		}
	}

	return TTRC("Invalid extension for selected shader type.");
}


