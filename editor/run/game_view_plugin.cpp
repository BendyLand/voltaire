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

void GameView::_update_embed_menu_options() {}

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


