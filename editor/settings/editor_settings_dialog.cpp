/**************************************************************************/
/*  editor_settings_dialog.cpp                                            */
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
#include "core/input/input_map.h"
#include "core/os/keyboard.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/inspector/editor_sectioned_inspector.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/settings/editor_event_search_bar.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/event_listener_line_edit.h"
#include "editor/settings/input_event_configuration_dialog.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor_settings_dialog.h"
#include "scene/gui/check_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/gui/tree.h"
#include "scene/main/timer.h"

void EditorSettingsDialog::ok_pressed()
{
	if (!EditorSettings::get_singleton()) {
		return;
	}
	_settings_save();
}

void EditorSettingsDialog::_settings_changed() { timer->start(); }

void EditorSettingsDialog::_settings_save()
{
	if (!timer->is_stopped()) {
		timer->stop();
	}
	EditorSettings::get_singleton()->notify_changes();
	EditorSettings::get_singleton()->save();
}

void EditorSettingsDialog::cancel_pressed()
{
	if (!EditorSettings::get_singleton()) {
		return;
	}

	EditorSettings::get_singleton()->notify_changes();
}

void EditorSettingsDialog::set_advanced_mode_enabled(bool p_enabled)
{
	advanced_switch->set_pressed(p_enabled);
}

void EditorSettingsDialog::set_current_section(const String& p_section)
{
	inspector->set_current_section(p_section);
}

void EditorSettingsDialog::_undo_redo_callback(void* p_self, const String& p_name)
{
	EditorNode::get_log()->add_message(p_name, EditorLog::MSG_TYPE_EDITOR);
}

void EditorSettingsDialog::_update_icons()
{
	search_box->set_right_icon(get_editor_theme_icon(SNAME("Search")));
	search_box->set_clear_button_enabled(true);

	restart_close_button->set_button_icon(get_editor_theme_icon(SNAME("Close")));
	restart_container->add_theme_style_override(
		SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());
	restart_icon->set_texture(get_editor_theme_icon(SNAME("StatusWarning")));
	restart_label->add_theme_color_override(SceneStringName(font_color),
		get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
}

bool EditorSettingsDialog::_is_in_project_manager() const
{
	return !ProjectSettings::get_singleton()->is_project_loaded();
}

void EditorSettingsDialog::_tabs_tab_changed(int p_tab)
{
	_focus_current_search_box();

	// When tab has switched, shortcuts may have changed.
	_update_dynamic_property_hints();
	inspector->get_inspector()->update_tree();
}

void EditorSettingsDialog::_focus_current_search_box()
{
	Control* tab = tabs->get_current_tab_control();
	LineEdit* current_search_box = nullptr;
	if (tab == tab_general) {
		current_search_box = search_box;
	}
	else if (tab == tab_shortcuts) {
		current_search_box = shortcut_search_bar->get_name_search_box();
	}

	if (current_search_box) {
		current_search_box->grab_focus();
		current_search_box->select_all();
	}
}

void EditorSettingsDialog::_editor_restart_request() { restart_container->show(); }

void EditorSettingsDialog::_editor_restart_close() { restart_container->hide(); }

EditorSettingsDialog::~EditorSettingsDialog() { singleton = nullptr; }

void EditorSettingsPropertyWrapper::_update_override()
{
	// Don't allow overriding theme properties, because it causes problems. Overriding Project
	// Manager settings makes no sense.
	// TODO: Find a better way to define exception prefixes (if the list happens to grow).
	if (property.begins_with("interface/theme") || property.begins_with("project_manager") ||
		Engine::get_singleton()->is_project_manager_hint()) {
		can_override = false;
		return;
	}

	const bool has_override =
		ProjectSettings::get_singleton()->is_project_loaded() &&
		ProjectSettings::get_singleton()->has_editor_setting_override(property);
	if (has_override) {
		if (!override_container) {
			_setup_override_info();
		}
		override_editor_property->update_property();
		set_bottom_editor(override_container);
		override_container->show();
	}
	else if (override_container) {
		override_container->hide();
		set_bottom_editor(nullptr);
	}
	can_override = !has_override;
}

void EditorSettingsPropertyWrapper::_notification(int p_what)
{
	if (override_container && p_what == NOTIFICATION_THEME_CHANGED) {
		override_icon->set_texture(get_editor_theme_icon(SNAME("Hierarchy")));
		goto_button->set_button_icon(get_editor_theme_icon(SNAME("MethodOverride")));
		remove_button->set_button_icon(get_editor_theme_icon(SNAME("Close")));
	}
}

void EditorSettingsPropertyWrapper::update_property() { editor_property->update_property(); }


