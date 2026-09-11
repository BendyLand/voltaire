/**************************************************************************/
/*  game_view_plugin.cpp                                                  */
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
#include "core/os/process_id.h"
#include "core/string/translation_server.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/editor_interface.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/window_wrapper.h"
#include "editor/run/editor_run_bar.h"
#include "editor/run/embedded_process.h"
#include "editor/run/run_instances_dialog.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_feature_profile.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "game_view_plugin.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/separator.h"
#include "scene/main/scene_tree.h"
#include "servers/display/display_server.h"

void GameViewDebugger::set_debug_mute_audio(bool p_enabled)
{
	mute_audio = p_enabled;
	EditorDebuggerNode::get_singleton()->set_debug_mute_audio(p_enabled);
}

void GameViewDebugger::set_camera_override(bool p_enabled)
{
	EditorDebuggerNode::get_singleton()->set_camera_override(
		p_enabled ? camera_override_mode : EditorDebuggerNode::OVERRIDE_NONE);
}

void GameViewDebugger::set_camera_manipulate_mode(EditorDebuggerNode::CameraOverride p_mode)
{
	camera_override_mode = p_mode;

	if (EditorDebuggerNode::get_singleton()->get_camera_override() !=
		EditorDebuggerNode::OVERRIDE_NONE) {
		set_camera_override(true);
	}
}

void GameViewDebugger::_feature_profile_changed()
{
	Ref<EditorFeatureProfile> profile =
		EditorFeatureProfileManager::get_singleton()->get_current_profile();
	is_feature_enabled =
		profile.is_null() || !profile->is_feature_disabled(EditorFeatureProfile::FEATURE_GAME);
}

bool GameViewDebugger::has_capture(const String& p_capture) const
{
	return p_capture == "game_view";
}

void GameView::_instance_starting_static(int p_idx, List<String>& r_arguments)
{
	ERR_FAIL_NULL(singleton);
	singleton->_instance_starting(p_idx, r_arguments);
}

void GameView::_show_update_window_wrapper()
{
	EditorRun::WindowPlacement placement = EditorRun::get_window_placement();
	Point2 position = floating_window_rect.position;
	Size2i size = floating_window_rect.size;
	int screen = floating_window_screen;

	// Obtain the size around the embedded process control. Usually, the difference between the game
	// view's get_size and the embedded control should work. However, when the control is hidden and
	// has never been displayed, the size of the embedded control is not calculated.
	Size2 old_min_size = embedded_process->get_custom_minimum_size();
	embedded_process->set_custom_minimum_size(Size2i());

	Size2 embedded_process_min_size = get_minimum_size();
	Size2 wrapped_margins_size = window_wrapper->get_margins_size();
	Size2 wrapped_min_size = window_wrapper->get_minimum_size();
	Point2 offset_embedded_process =
		embedded_process->get_global_position() - get_global_position();

	// On the first startup, the global position of the embedded process control is invalid because
	// it was never displayed. We will calculate it manually using the minimum size of the window.
	if (offset_embedded_process == Point2()) {
		offset_embedded_process.y = wrapped_min_size.y;
	}
	offset_embedded_process.x += embedded_process->get_margin_size(SIDE_LEFT);
	offset_embedded_process.y += embedded_process->get_margin_size(SIDE_TOP);
	offset_embedded_process += window_wrapper->get_margins_top_left();

	embedded_process->set_custom_minimum_size(old_min_size);

	Point2 size_diff_embedded_process =
		Point2(0, embedded_process_min_size.y) + embedded_process->get_margins_size();

	if (placement.position != Point2i(INT_MAX, INT_MAX)) {
		position = placement.position - offset_embedded_process;
		screen = placement.screen;
	}
	if (placement.size != Size2i()) {
		size = placement.size + size_diff_embedded_process + wrapped_margins_size;
	}
	window_wrapper->restore_window_from_saved_position(Rect2(position, size), screen, Rect2i());
}

void GameView::_play_pressed()
{
	if (!is_feature_enabled) {
		return;
	}

	ProcessID current_process_id = EditorRunBar::get_singleton()->get_current_process();
	if (current_process_id == 0) {
		return;
	}

	if (!window_wrapper->get_window_enabled()) {
		screen_index_before_start =
			EditorNode::get_singleton()->get_editor_main_screen()->get_selected_index();
	}

	if (embed_on_play && _get_embed_available() == EMBED_AVAILABLE) {
		// It's important to disable the low power mode when unfocused because otherwise
		// the button in the editor are not responsive and if the user moves the mouse quickly,
		// the mouse clicks are not registered.
		EditorNode::get_singleton()->set_unfocused_low_processor_usage_mode_enabled(false);
		_update_embed_window_size();
		if (!window_wrapper->get_window_enabled()) {
			EditorNode::get_singleton()->get_editor_main_screen()->select(
				EditorMainScreen::EDITOR_GAME);
			// Reset the normal size of the bottom panel when fully expanded.
			EditorNode::get_singleton()->get_bottom_panel()->set_expanded(false);

			if (embedded_process->get_focus_mode_with_override() != FOCUS_NONE) {
				embedded_process->grab_focus();
			}
		}
		embedded_process->embed_process(current_process_id);
		_update_ui();
	}
}

void GameView::_stop_pressed()
{
	if (!is_feature_enabled) {
		return;
	}

	_detach_script_debugger();
	paused = false;

	EditorNode::get_singleton()->set_unfocused_low_processor_usage_mode_enabled(true);
	embedded_process->reset();
	_update_ui();

	if (window_wrapper->get_window_enabled()) {
		window_wrapper->set_window_enabled(false);
	}

	if (screen_index_before_start >= 0 &&
		EditorNode::get_singleton()->get_editor_main_screen()->get_selected_index() ==
			EditorMainScreen::EDITOR_GAME) {
		// We go back to the screen where the user was before starting the game.
		EditorNode::get_singleton()->get_editor_main_screen()->select(screen_index_before_start);
	}

	screen_index_before_start = -1;
}

void GameView::_embedding_failed()
{
	state_label->set_text(TTRC("Connection impossible to the game process."));
}

void GameView::_embedded_process_focused()
{
	if (embed_on_play && !window_wrapper->get_window_enabled()) {
		EditorNode::get_singleton()->get_editor_main_screen()->select(
			EditorMainScreen::EDITOR_GAME);
	}
}

void GameView::_editor_or_project_settings_changed()
{
	if (!is_inside_tree()) {
		return;
	}

	// Update the window size and aspect ratio.
	_update_embed_window_size();

	if (window_wrapper->get_window_enabled()) {
		_show_update_window_wrapper();
		if (embedded_process->is_embedding_completed()) {
			embedded_process->queue_update_embedded_process();
		}
	}

	_update_ui();
}

void GameView::_update_debugger_buttons()
{
	bool empty = active_sessions == 0;

	suspend_button->set_disabled(empty);
	camera_override_button->set_disabled(empty);
	speed_state_button->set_disabled(empty);
	game_size_label->set_visible(!empty);

	PopupMenu* menu = camera_override_menu->get_popup();

	bool disable_camera_reset = empty || !camera_override_button->is_pressed() ||
								!menu->is_item_checked(menu->get_item_index(CAMERA_MODE_INGAME));
	menu->set_item_disabled(CAMERA_RESET_2D, disable_camera_reset);
	menu->set_item_disabled(CAMERA_RESET_3D, disable_camera_reset);

	if (empty) {
		suspend_button->set_pressed(false);
		camera_override_button->set_pressed(false);
		_reset_time_scales();
		game_size_label->set_text("");
		game_size_label->set_tooltip_text("");
		game_window_size = Size2i(-1, -1);
		hdr_output_enabled = false;
		output_max_linear_value = 1.0f;
	}

	next_frame_button->set_disabled(!suspend_button->is_pressed());

	menu = game_window_options_menu->get_popup();
	if (empty) {
		int menu_item_index = menu->get_item_index(WINDOW_SEPARATOR_DYNAMIC_RANGE);
		if (menu_item_index >= 0) {
			menu->remove_item(menu_item_index);
		}
		menu_item_index = menu->get_item_index(WINDOW_REQUEST_HDR_OUTPUT);
		if (menu_item_index >= 0) {
			menu->remove_item(menu_item_index);
		}
		menu_item_index = menu->get_item_index(WINDOW_HDR_OUTPUT_ERROR);
		if (menu_item_index >= 0) {
			menu->remove_item(menu_item_index);
		}
	}
	else {
		int menu_item_index = menu->get_item_index(WINDOW_SEPARATOR_DYNAMIC_RANGE);
		if (menu_item_index < 0) {
			menu->add_separator(TTRC("Window Dynamic Range"), WINDOW_SEPARATOR_DYNAMIC_RANGE);
		}
		if (menu->get_item_index(WINDOW_REQUEST_HDR_OUTPUT) < 0) {
			if (menu->get_item_index(WINDOW_HDR_OUTPUT_ERROR) < 0) {
				menu->add_item(TTRC("Loading..."), WINDOW_HDR_OUTPUT_ERROR);
				menu->set_item_disabled(menu->get_item_index(WINDOW_HDR_OUTPUT_ERROR), true);
				menu->set_item_tooltip(menu->get_item_index(WINDOW_HDR_OUTPUT_ERROR), "");
			}
		}
	}
}

void GameView::_handle_shortcut_requested(int p_embed_action)
{
	switch (p_embed_action) {
	case ScriptEditorDebugger::EMBED_SUSPEND_TOGGLE: {
		_toggle_suspend_button();
	} break;
	case ScriptEditorDebugger::EMBED_NEXT_FRAME: {
		debugger->next_frame();
	} break;
	}
}

void GameView::_toggle_suspend_button()
{
	const bool new_pressed = !suspend_button->is_pressed();
	suspend_button->set_pressed(new_pressed);
	_suspend_button_toggled(new_pressed);
}

void GameView::_suspend_button_toggled(bool p_pressed)
{
	_update_debugger_buttons();

	debugger->set_suspend(p_pressed);
}

void GameView::_reset_time_scales()
{
	time_scale_index = DEFAULT_TIME_SCALE_INDEX;
	debugger->reset_time_scale();
	if (is_inside_tree()) {
		_update_speed_state_icon(DEFAULT_TIME_SCALE_INDEX);
		_update_speed_buttons();
	}
}

void GameView::_update_speed_state_icon(int p_id)
{
	PopupMenu* menu = speed_state_button->get_popup();
	for (int i = 0; i < speed_state_button->get_item_count(); i++) {
		if (i == DEFAULT_TIME_SCALE_INDEX) {
			continue;
		}

		menu->set_item_icon(i, nullptr);
	}

	menu->set_item_icon(p_id, get_editor_theme_icon(SNAME("KeyValue")));
	if (p_id == DEFAULT_TIME_SCALE_INDEX) {
		menu->set_item_icon_modulate(
			p_id, get_theme_color(SNAME("mono_color"), EditorStringName(Editor)));
	}
	else {
		menu->set_item_icon(
			DEFAULT_TIME_SCALE_INDEX, get_editor_theme_icon(SNAME("KeyBezierHandle")));

		if (p_id > DEFAULT_TIME_SCALE_INDEX) {
			menu->set_item_icon_modulate(
				p_id, get_theme_color(SNAME("success_color"), EditorStringName(Editor)));
		}
		else {
			menu->set_item_icon_modulate(
				p_id, get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		}
	}
}

void GameView::_update_speed_state_color()
{
	Color text_color;
	if (time_scale_index == DEFAULT_TIME_SCALE_INDEX) {
		text_color = get_theme_color(SceneStringName(font_color), EditorStringName(Editor));
	}
	else if (time_scale_index > DEFAULT_TIME_SCALE_INDEX) {
		text_color = get_theme_color(SNAME("success_color"), EditorStringName(Editor));
	}
	else if (time_scale_index < DEFAULT_TIME_SCALE_INDEX) {
		text_color = get_theme_color(SNAME("warning_color"), EditorStringName(Editor));
	}
	speed_state_button->add_theme_color_override(SceneStringName(font_color), text_color);
	speed_state_button->add_theme_color_override(SNAME("font_hover_color"), text_color);
	speed_state_button->add_theme_color_override(SNAME("font_hover_pressed_color"), text_color);
	speed_state_button->add_theme_color_override(SNAME("font_pressed_color"), text_color);
}

void GameView::_update_embed_menu_options()
{
	PopupMenu* menu = game_window_options_menu->get_popup();
	menu->set_item_checked(
		menu->get_item_index(WINDOW_SIZE_MODE_FIXED), embed_size_mode == SIZE_MODE_FIXED);
	menu->set_item_checked(menu->get_item_index(WINDOW_SIZE_MODE_KEEP_ASPECT),
		embed_size_mode == SIZE_MODE_KEEP_ASPECT);
	menu->set_item_checked(
		menu->get_item_index(WINDOW_SIZE_MODE_STRETCH), embed_size_mode == SIZE_MODE_STRETCH);
}

void GameView::_update_embed_buttons()
{
	game_embed_mode_button[EmbedMode::EMBED_TYPE_EDITOR]->set_pressed(
		embed_on_play && !make_floating_on_play);
	game_embed_mode_button[EmbedMode::EMBED_TYPE_FLOATING]->set_pressed(
		make_floating_on_play && window_wrapper->is_window_available());
	game_embed_mode_button[EmbedMode::EMBED_TYPE_DISABLED]->set_pressed(
		!embed_on_play && !make_floating_on_play);
}

void GameView::_update_game_window_size_label()
{
	String window_size_string =
		game_window_size.x < 0 ? "" : vformat("%dx%d", game_window_size.x, game_window_size.y);
	game_size_label->set_text(hdr_output_enabled ? vformat("%s %s (%.2f)", window_size_string,
													   TTRC("HDR"), output_max_linear_value)
												 : window_size_string);
	game_size_label->set_tooltip_text(
		vformat(TTR("Window size: %s\nMode: %s\nMaximum linear value: %.2f%s"), window_size_string,
			hdr_output_enabled ? TTRC("HDR") : TTRC("SDR"), output_max_linear_value,
			hdr_output_enabled
				? vformat(TTR("\nReference luminance: %.0f nits\nMaximum luminance: %.0f nits"),
					  current_reference_luminance, current_max_luminance)
				: ""));
}

void GameView::_update_embed_window_size()
{
	if (paused) {
		// When paused, Godot does not re-render. As a result, resizing the game window to a larger
		// size causes artifacts and flickering. However, resizing to a smaller size seems fine. To
		// prevent artifacts and flickering, we will force the game window to maintain its size.
		// Using the same technique as SIZE_MODE_FIXED, the embedded process control will
		// prevent resizing the game to a larger size while maintaining the aspect ratio.
		embedded_process->set_window_size(size_paused);
		embedded_process->set_keep_aspect(false);

	}
	else {
		if (embed_size_mode == SIZE_MODE_FIXED || embed_size_mode == SIZE_MODE_KEEP_ASPECT) {
			// The embedded process control will need the desired window size.
			EditorRun::WindowPlacement placement = EditorRun::get_window_placement();
			embedded_process->set_window_size(placement.size);
		}
		else {
			// Stretch... No need for the window size.
			embedded_process->set_window_size(Size2i());
		}
		embedded_process->set_keep_aspect(embed_size_mode == SIZE_MODE_KEEP_ASPECT);
	}
}

void GameView::_debug_mute_audio_button_pressed()
{
	debug_mute_audio = !debug_mute_audio;
	debug_mute_audio_button->set_button_icon(
		get_editor_theme_icon(debug_mute_audio ? SNAME("AudioMute") : SNAME("AudioStreamPlayer")));
	debug_mute_audio_button->set_tooltip_text(
		debug_mute_audio ? TTRC("Unmute game audio.") : TTRC("Mute game audio."));
	debugger->set_debug_mute_audio(debug_mute_audio);
}

void GameView::_setup_complete()
{
	debugger->window_request_size();
	debugger->hdr_output_request_state();
}

void GameView::_camera_override_button_toggled(bool p_pressed)
{
	_update_debugger_buttons();

	debugger->set_camera_override(p_pressed);
}

void GameView::_update_floating_window_settings()
{
	if (window_wrapper->get_window_enabled()) {
		floating_window_rect = window_wrapper->get_window_rect();
		floating_window_screen = window_wrapper->get_window_screen();
	}
}

void GameView::_remote_window_title_changed(String title)
{
	window_wrapper->set_window_title(title);
}

void GameView::_update_arguments_for_instance(int p_idx, List<String>& r_arguments)
{
	if (p_idx != 0 || !embed_on_play || _get_embed_available() != EMBED_AVAILABLE) {
		return;
	}

	// Remove duplicates/unwanted parameters.
	List<String>::Element* E = r_arguments.front();
	List<String>::Element* user_args_element = nullptr;
	HashSet<String> remove_args({"--position", "--resolution", "--screen"});
#ifdef MACOS_ENABLED
	// macOS requires the embedded display driver.
	remove_args.insert("--display-driver");
#endif

#ifdef WAYLAND_ENABLED
	// Wayland requires its display driver.
	if (DisplayServer::get_singleton()->get_name() == "Wayland") {
		remove_args.insert("--display-driver");
	}
#endif

#ifdef X11_ENABLED
	// X11 requires its display driver.
	if (DisplayServer::get_singleton()->get_name() == "X11") {
		remove_args.insert("--display-driver");
	}
#endif

	while (E) {
		List<String>::Element* N = E->next();

		// For these parameters, we need to also remove the value.
		if (remove_args.has(E->get())) {
			r_arguments.erase(E);
			if (N) {
				List<String>::Element* V = N->next();
				r_arguments.erase(N);
				N = V;
			}
		}
		else if (E->get() == "-f" || E->get() == "--fullscreen" || E->get() == "-m" ||
				   E->get() == "--maximized" || E->get() == "-t" || E->get() == "-always-on-top") {
			r_arguments.erase(E);
		}
		else if (E->get() == "--" || E->get() == "++") {
			user_args_element = E;
			break;
		}

		E = N;
	}

	// Add the editor window's native ID so the started game can directly set it as its parent.
	List<String>::Element* N = r_arguments.insert_before(user_args_element, "--wid");
	N = r_arguments.insert_after(
		N, itos(DisplayServer::get_singleton()->window_get_native_handle(
			   DisplayServerEnums::WINDOW_HANDLE, get_window()->get_window_id())));

#if MACOS_ENABLED
	N = r_arguments.insert_after(N, "--embedded");
#endif

#ifdef WAYLAND_ENABLED
	if (DisplayServer::get_singleton()->get_name() == "Wayland") {
		N = r_arguments.insert_after(N, "--display-driver");
		N = r_arguments.insert_after(N, "wayland");
	}
#endif

#ifdef X11_ENABLED
	if (DisplayServer::get_singleton()->get_name() == "X11") {
		N = r_arguments.insert_after(N, "--display-driver");
		N = r_arguments.insert_after(N, "x11");
	}
#endif

	// Be sure to have the correct window size in the embedded_process control.
	_update_embed_window_size();
	Rect2i rect = embedded_process->get_screen_embedded_window_rect();

	// Usually, the global rect of the embedded process control is invalid because it was hidden. We
	// will calculate it manually.
	if (!window_wrapper->get_window_enabled()) {
		Size2 old_min_size = embedded_process->get_custom_minimum_size();
		embedded_process->set_custom_minimum_size(Size2i());

		Control* container = EditorNode::get_singleton()->get_editor_main_screen()->get_control();
		rect = container->get_global_rect();

		Size2 wrapped_min_size = window_wrapper->get_minimum_size();
		rect.position.y += wrapped_min_size.y;
		rect.size.y -= wrapped_min_size.y;

		rect = embedded_process->get_adjusted_embedded_window_rect(rect);

		embedded_process->set_custom_minimum_size(old_min_size);
	}

	// When using the floating window, we need to force the position and size from the
	// editor/project settings, because the get_screen_embedded_window_rect of the
	// embedded_process will be updated only on the next frame.
	if (window_wrapper->get_window_enabled()) {
		EditorRun::WindowPlacement placement = EditorRun::get_window_placement();
		if (placement.position != Point2i(INT_MAX, INT_MAX)) {
			rect.position = placement.position;
		}
		if (placement.size != Size2i()) {
			rect.size = placement.size;
		}
	}

	N = r_arguments.insert_after(N, "--position");
	N = r_arguments.insert_after(N, itos(rect.position.x) + "," + itos(rect.position.y));
	N = r_arguments.insert_after(N, "--resolution");
	r_arguments.insert_after(N, itos(rect.size.x) + "x" + itos(rect.size.y));
}

void GameView::_debugger_breaked(bool p_breaked, bool p_can_debug)
{
	if (p_breaked == paused) {
		return;
	}

	paused = p_breaked;

	if (paused) {
		size_paused = embedded_process->get_screen_embedded_window_rect().size;
	}

	_update_embed_window_size();
}

///////

void GameViewPluginBase::selected_notify()
{
	if (_is_window_wrapper_enabled()) {
#ifdef ANDROID_ENABLED
		notify_main_screen_changed(get_plugin_name());
#else
		window_wrapper->grab_window_focus();
#endif // ANDROID_ENABLED
		_focus_another_editor();
	}
}

#ifndef ANDROID_ENABLED
void GameViewPluginBase::set_window_layout(Ref<ConfigFile> p_layout)
{
	game_view->set_window_layout(p_layout);
}

void GameViewPluginBase::get_window_layout(Ref<ConfigFile> p_layout)
{
	game_view->get_window_layout(p_layout);
}

#endif // ANDROID_ENABLED

void GameViewPluginBase::_save_last_editor(const String& p_editor)
{
	if (p_editor != get_plugin_name()) {
		last_editor = p_editor;
	}
}

void GameViewPluginBase::_focus_another_editor()
{
	if (_is_window_wrapper_enabled()) {
		if (last_editor.is_empty() ||
			(last_editor == "Script" && ScriptEditor::get_singleton()->is_editor_floating())) {
			EditorNode::get_singleton()->get_editor_main_screen()->select(
				EditorMainScreen::EDITOR_2D);
		}
		else {
			EditorInterface::get_singleton()->set_main_screen_editor(last_editor);
		}
	}
}

bool GameViewPluginBase::_is_window_wrapper_enabled() const
{
#ifdef ANDROID_ENABLED
	return true;
#else
	return window_wrapper->get_window_enabled();
#endif // ANDROID_ENABLED
}

GameViewPluginBase::GameViewPluginBase()
{
#ifdef ANDROID_ENABLED
	debugger.instantiate();
#endif
}

GameViewPlugin::GameViewPlugin()
{
#ifndef ANDROID_ENABLED
	Ref<GameViewDebugger> game_view_debugger;
	game_view_debugger.instantiate();
	EmbeddedProcess* embedded_process = memnew(EmbeddedProcess);
	setup(game_view_debugger, embedded_process);
#endif
}


