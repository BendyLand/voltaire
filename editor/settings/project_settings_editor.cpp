/**************************************************************************/
/*  project_settings_editor.cpp                                           */
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
#include "core/input/input_map.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/export/editor_export.h"
#include "editor/gui/editor_variant_type_selectors.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/editor_settings_dialog.h"
#include "editor/themes/editor_scale.h"
#include "project_settings_editor.h"
#include "scene/gui/check_button.h"
#include "servers/movie_writer/movie_writer.h"

void ProjectSettingsEditor::connect_filesystem_dock_signals(FileSystemDock* p_fs_dock)
{
	localization_editor->connect_filesystem_dock_signals(p_fs_dock);
	group_settings->connect_filesystem_dock_signals(p_fs_dock);
}

void ProjectSettingsEditor::popup_for_override(const String& p_override)
{
	popup_project_settings();
	tab_container->set_current_tab(0);
	general_settings_inspector->set_current_section(
		ProjectSettings::EDITOR_SETTING_OVERRIDE_PREFIX + p_override.get_slicec('/', 0));
}

void ProjectSettingsEditor::set_filter(const String& p_filter) { search_box->set_text(p_filter); }

void ProjectSettingsEditor::queue_save()
{
	settings_changed = true;
	timer->start();
}

void ProjectSettingsEditor::_save()
{
	settings_changed = false;
	if (ps) {
		ps->save();
	}
	if (pending_override_notify) {
		pending_override_notify = false;
		EditorNode::get_singleton()->notify_settings_overrides_changed();
	}
}

void ProjectSettingsEditor::set_plugins_page()
{
	tab_container->set_current_tab(tab_container->get_tab_idx_from_control(plugin_settings));
}

void ProjectSettingsEditor::set_general_page(const String& p_category)
{
	tab_container->set_current_tab(tab_container->get_tab_idx_from_control(general_editor));
	general_settings_inspector->set_current_section(p_category);
}

void ProjectSettingsEditor::update_plugins() { plugin_settings->update_plugins(); }

void ProjectSettingsEditor::init_autoloads() { autoload_settings->init_autoloads(); }

void ProjectSettingsEditor::_setting_edited(const String& p_name)
{
	const String full_name = general_settings_inspector->get_full_item_path(p_name);
	if (full_name.begins_with(ProjectSettings::EDITOR_SETTING_OVERRIDE_PREFIX)) {
		EditorSettings::get_singleton()->mark_setting_changed(
			full_name.trim_prefix(ProjectSettings::EDITOR_SETTING_OVERRIDE_PREFIX));
		pending_override_notify = true;
	}
	queue_save();
}

void ProjectSettingsEditor::_update_advanced(bool p_is_advanced)
{
	custom_properties->set_visible(p_is_advanced);
}

void ProjectSettingsEditor::_on_category_changed(const String& p_new_category)
{
	general_settings_inspector->get_inspector()->set_use_deletable_properties(
		p_new_category.begins_with(ProjectSettings::EDITOR_SETTING_OVERRIDE_PREFIX));
}

void ProjectSettingsEditor::_setting_selected(const String& p_path)
{
	if (p_path.is_empty()) {
		return;
	}

	property_box->set_text(general_settings_inspector->get_current_section() + "/" + p_path);

	_update_property_box(); // set_text doesn't trigger text_changed
}

void ProjectSettingsEditor::_property_box_changed(const String& p_text) { _update_property_box(); }

void ProjectSettingsEditor::_feature_selected(int p_index)
{
	const String property = property_box->get_text().strip_edges().get_slicec('.', 0);
	if (p_index == FEATURE_ALL) {
		property_box->set_text(property);
	}
	else if (p_index == FEATURE_CUSTOM) {
		property_box->set_text(property + ".custom");
		const int len = property.length() + 1;
		property_box->select(len);
		property_box->set_caret_column(len);
		property_box->grab_focus();
	}
	else {
		property_box->set_text(property + "." + feature_box->get_item_text(p_index));
	};
	_update_property_box();
}

String ProjectSettingsEditor::_get_setting_name() const
{
	String name = property_box->get_text().strip_edges();
	if (!name.begins_with("_") && !name.contains_char('/')) {
		name = "global/" + name;
	}
	return name;
}

void ProjectSettingsEditor::_add_feature_overrides()
{
	HashSet<String> presets;

	presets.insert("bptc");
	presets.insert("s3tc");
	presets.insert("etc2");
	presets.insert("editor");
	presets.insert("editor_hint");
	presets.insert("editor_runtime");
	presets.insert("template_debug");
	presets.insert("template_release");
	presets.insert("debug");
	presets.insert("release");
	presets.insert("template");
	presets.insert("double");
	presets.insert("single");
	presets.insert("32");
	presets.insert("64");
	presets.insert("movie");

	EditorExport* ee = EditorExport::get_singleton();

	for (int i = 0; i < ee->get_export_platform_count(); i++) {
		List<String> p;
		ee->get_export_platform(i)->get_platform_features(&p);
		for (const String& E : p) {
			presets.insert(E);
		}
	}

	for (int i = 0; i < ee->get_export_preset_count(); i++) {
		List<String> p;
		ee->get_export_preset(i)->get_platform()->get_preset_features(ee->get_export_preset(i), &p);
		for (const String& E : p) {
			presets.insert(E);
		}

		String custom = ee->get_export_preset(i)->get_custom_features();
		Vector<String> custom_list = custom.split(",");
		for (int j = 0; j < custom_list.size(); j++) {
			String f = custom_list[j].strip_edges();
			if (!f.is_empty()) {
				presets.insert(f);
			}
		}
	}

	feature_box->clear();
	feature_box->add_item(TTRC("All"), FEATURE_ALL); // So it is always on top.
	feature_box->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_ALWAYS);
	feature_box->add_item(TTRC("Custom"), FEATURE_CUSTOM);
	feature_box->set_item_auto_translate_mode(-1, AUTO_TRANSLATE_MODE_ALWAYS);
	feature_box->add_separator();

	int id = FEATURE_FIRST;
	for (const String& E : presets) {
		feature_box->add_item(E, id++);
	}
}

void ProjectSettingsEditor::_tabs_tab_changed(int p_tab) { _focus_current_search_box(); }

void ProjectSettingsEditor::_focus_current_search_box()
{
	Control* tab = tab_container->get_current_tab_control();
	LineEdit* current_search_box = nullptr;
	if (tab == general_editor) {
		current_search_box = search_box;
	}
	else if (tab == action_map_editor) {
		current_search_box = action_map_editor->get_search_box();
	}

	if (current_search_box) {
		current_search_box->grab_focus();
		current_search_box->select_all();
	}
}

void ProjectSettingsEditor::_focus_current_path_box()
{
	Control* tab = tab_container->get_current_tab_control();
	LineEdit* current_path_box = nullptr;
	if (tab == general_editor) {
		current_path_box = property_box;
	}
	else if (tab == action_map_editor) {
		current_path_box = action_map_editor->get_path_box();
	}
	else if (tab == shaders_global_shader_uniforms_editor) {
		current_path_box = shaders_global_shader_uniforms_editor->get_name_box();
	}
	else if (tab == group_settings) {
		current_path_box = group_settings->get_name_box();
	}

	if (current_path_box) {
		current_path_box->grab_focus();
		current_path_box->select_all();
	}
}

void ProjectSettingsEditor::_editor_restart()
{
	ProjectSettings::get_singleton()->save();
	EditorNode::get_singleton()->save_all_scenes();
	EditorNode::get_singleton()->restart_editor();
}

void ProjectSettingsEditor::_editor_restart_request() { restart_container->show(); }

void ProjectSettingsEditor::_editor_restart_close() { restart_container->hide(); }

void ProjectSettingsEditor::_update_theme()
{
	add_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
	del_button->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
	search_box->set_right_icon(get_editor_theme_icon(SNAME("Search")));
	restart_close_button->set_button_icon(get_editor_theme_icon(SNAME("Close")));
	restart_container->add_theme_style_override(
		SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());
	restart_icon->set_texture(get_editor_theme_icon(SNAME("StatusWarning")));
	restart_label->add_theme_color_override(SceneStringName(font_color),
		get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
}


