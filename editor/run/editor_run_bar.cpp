/**************************************************************************/
/*  editor_run_bar.cpp                                                    */
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
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export_platform.h"
#include "editor/export/editor_export_preset.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/run/editor_run_native.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/settings/project_settings_editor.h"
#include "editor/themes/editor_scale.h"
#include "editor_run_bar.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel_container.h"
#include "scene/main/scene_tree.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

EditorRunBar* EditorRunBar::singleton = nullptr;

void EditorRunBar::_reset_play_buttons()
{
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		return;
	}

	play_button->set_pressed(false);
	play_button->set_button_icon(get_editor_theme_icon(SNAME("MainPlay")));
	play_button->set_tooltip_text(TTRC("Run the project's main scene."));

	play_scene_button->set_pressed(false);
	play_scene_button->set_button_icon(get_editor_theme_icon(SNAME("PlayScene")));
	play_scene_button->set_tooltip_text(TTRC("Play the currently edited scene."));

	play_custom_scene_button->set_pressed(false);
	play_custom_scene_button->set_button_icon(get_editor_theme_icon(SNAME("PlayCustom")));
	play_custom_scene_button->set_tooltip_text(TTRC("Play a custom scene."));
}

void EditorRunBar::_update_play_buttons()
{
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		return;
	}

	_reset_play_buttons();
	if (!is_playing()) {
		return;
	}

	Button* active_button = nullptr;
	if (current_mode == RUN_CURRENT) {
		active_button = play_scene_button;
		active_button->set_tooltip_text(TTRC("Reload the played scene that was being edited."));
	}
	else if (current_mode == RUN_CUSTOM) {
		active_button = play_custom_scene_button;
		active_button->set_tooltip_text(TTRC("Reload the played custom scene."));
	}
	else {
		active_button = play_button;
		active_button->set_tooltip_text(TTRC("Reload the played main scene."));
	}

	if (active_button) {
		active_button->set_pressed(true);
		active_button->set_button_icon(get_editor_theme_icon(SNAME("Reload")));
	}
}

void EditorRunBar::_movie_maker_item_pressed(int p_id)
{
	switch (p_id) {
	case MOVIE_MAKER_TOGGLE: {
		bool new_enabled = !is_movie_maker_enabled();
		set_movie_maker_enabled(new_enabled);
		write_movie_button->get_popup()->set_item_checked(0, new_enabled);
		write_movie_button->set_pressed(new_enabled);
		_write_movie_toggled(new_enabled);
		break;
	}
	case MOVIE_MAKER_OPEN_SETTINGS:
		ProjectSettingsEditor::get_singleton()->popup_project_settings(true);
		ProjectSettingsEditor::get_singleton()->set_general_page("editor/movie_writer");
		break;
	}
}

void EditorRunBar::_write_movie_toggled(bool p_enabled)
{
	if (p_enabled) {
		add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("LaunchPadMovieMode"), EditorStringName(EditorStyles)).ptr());
		write_movie_panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("MovieWriterButtonPressed"), EditorStringName(EditorStyles))
				.ptr());
	}
	else {
		add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("LaunchPadNormal"), EditorStringName(EditorStyles)).ptr());
		write_movie_panel->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("MovieWriterButtonNormal"), EditorStringName(EditorStyles))
				.ptr());
	}
}

Vector<String> EditorRunBar::_get_xr_mode_play_args(RunXRModeMenuItem p_menu_item)
{
	Vector<String> play_args;
	if (p_menu_item == RunXRModeMenuItem::OFF) {
		// Play in regular mode, xr mode off.
		play_args.push_back("--xr-mode");
		play_args.push_back("off");
	}
	else if (p_menu_item == RunXRModeMenuItem::ON) {
		// Play in xr mode.
		play_args.push_back("--xr-mode");
		play_args.push_back("on");
	}
	return play_args;
}

void EditorRunBar::_quick_run_selected(const String& p_file_path, int p_menu_item)
{
	play_custom_scene(
		p_file_path, _get_xr_mode_play_args(static_cast<RunXRModeMenuItem>(p_menu_item)));
}

void EditorRunBar::recovery_mode_show_dialog() { recovery_mode_popup->popup_centered(); }

void EditorRunBar::recovery_mode_reload_project()
{
	EditorNode::get_singleton()->trigger_menu_option(
		EditorNode::PROJECT_RELOAD_CURRENT_PROJECT, false);
}

void EditorRunBar::play_main_scene(bool p_from_native, const Vector<String>& p_play_args)
{
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		EditorToaster::get_singleton()->popup_str(
			TTR("Recovery Mode is enabled. Disable it to run the project."),
			EditorToaster::SEVERITY_WARNING);
		return;
	}

	if (p_from_native) {
		run_native->resume_run_native();
	}
	else {
		stop_playing();

		current_mode = RunMode::RUN_MAIN;
		_run_scene("", p_play_args);
	}
}

void EditorRunBar::play_current_scene(bool p_reload, const Vector<String>& p_play_args)
{
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		EditorToaster::get_singleton()->popup_str(
			TTR("Recovery Mode is enabled. Disable it to run the project."),
			EditorToaster::SEVERITY_WARNING);
		return;
	}

	String last_current_scene =
		run_current_filename; // This is necessary to have a copy of the string.

	EditorNode::get_singleton()->save_default_environment();
	stop_playing();

	current_mode = RunMode::RUN_CURRENT;
	if (p_reload) {
		_run_scene(last_current_scene, p_play_args);
	}
	else {
		_run_scene("", p_play_args);
	}
}

void EditorRunBar::play_custom_scene(const String& p_custom, const Vector<String>& p_play_args)
{
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		EditorToaster::get_singleton()->popup_str(
			TTR("Recovery Mode is enabled. Disable it to run the project."),
			EditorToaster::SEVERITY_WARNING);
		return;
	}

	stop_playing();

	current_mode = RunMode::RUN_CUSTOM;
	_run_scene(p_custom, p_play_args);
}

bool EditorRunBar::is_playing() const
{
	EditorRun::Status status = editor_run.get_status();
	return (status == EditorRun::STATUS_PLAY || status == EditorRun::STATUS_PAUSED);
}

Error EditorRunBar::start_native_device(int p_device_id) const
{
	return run_native->start_run_native(p_device_id);
}

ProcessID EditorRunBar::has_child_process(ProcessID p_pid) const
{
	return editor_run.has_child_process(p_pid);
}

void EditorRunBar::stop_child_process(ProcessID p_pid)
{
	if (!has_child_process(p_pid)) {
		return;
	}

	editor_run.stop_child_process(p_pid);
	if (!editor_run.get_child_process_count()) { // All children stopped. Closing.
		stop_playing();
	}
}

ProcessID EditorRunBar::get_current_process() const { return editor_run.get_current_process(); }

void EditorRunBar::set_movie_maker_enabled(bool p_enabled)
{
	movie_maker_enabled = p_enabled;
	write_movie_button->get_popup()->set_item_checked(0, p_enabled);
}

bool EditorRunBar::is_movie_maker_enabled() const { return movie_maker_enabled; }

HBoxContainer* EditorRunBar::get_buttons_container() { return main_hbox; }


