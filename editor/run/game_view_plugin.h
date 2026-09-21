/**************************************************************************/
/*  game_view_plugin.h                                                    */
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

#pragma once

#include "editor/debugger/editor_debugger_node.h"
#include "editor/debugger/editor_debugger_plugin.h"
#include "editor/editor_main_screen.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"

class EmbeddedProcessBase;
class VSeparator;
class WindowWrapper;
class ScriptEditorDebugger;

class GameViewDebugger : public EditorDebuggerPlugin
{
private:
	Vector<Ref<EditorDebuggerSession>> sessions;

	bool is_feature_enabled = true;
	bool selection_visible = true;
	bool mute_audio = false;
	EditorDebuggerNode::CameraOverride camera_override_mode = EditorDebuggerNode::OVERRIDE_INGAME;

	bool selection_avoid_locked = false;
	bool selection_prefer_group = false;

	void _feature_profile_changed();

	struct ScreenshotCB
	{
		Rect2i rect;
	};

	int64_t scr_rq_id = 0;
	HashMap<uint64_t, ScreenshotCB> screenshot_callbacks;

public:
	virtual bool has_capture(const String& p_capture) const override;

	void set_debug_mute_audio(bool p_enabled);

	void set_camera_override(bool p_enabled);
	void set_camera_manipulate_mode(EditorDebuggerNode::CameraOverride p_mode);

	GameViewDebugger() = default;
};

class GameView : public VBoxContainer
{
	enum
	{
		CAMERA_RESET_2D,
		CAMERA_RESET_3D,
		CAMERA_MODE_INGAME,
		CAMERA_MODE_EDITORS,
		SELECTION_HIDE,
		SELECTION_AVOID_LOCKED,
		SELECTION_PREFER_GROUP,
		WINDOW_SIZE_MODE_FIXED,
		WINDOW_SIZE_MODE_KEEP_ASPECT,
		WINDOW_SIZE_MODE_STRETCH,
		WINDOW_SEPARATOR_DYNAMIC_RANGE,
		WINDOW_REQUEST_HDR_OUTPUT,
		WINDOW_HDR_OUTPUT_ERROR,
	};

	enum EmbedSizeMode
	{
		SIZE_MODE_FIXED,
		SIZE_MODE_KEEP_ASPECT,
		SIZE_MODE_STRETCH,
	};

	enum EmbedAvailability
	{
		EMBED_AVAILABLE,
		EMBED_NOT_AVAILABLE_FEATURE_NOT_SUPPORTED,
		EMBED_NOT_AVAILABLE_MINIMIZED,
		EMBED_NOT_AVAILABLE_MAXIMIZED,
		EMBED_NOT_AVAILABLE_FULLSCREEN,
		EMBED_NOT_AVAILABLE_SINGLE_WINDOW_MODE,
		EMBED_NOT_AVAILABLE_PROJECT_DISPLAY_DRIVER,
		EMBED_NOT_AVAILABLE_HEADLESS,
	};

	enum EmbedMode
	{
		EMBED_TYPE_DISABLED,
		EMBED_TYPE_FLOATING,
		EMBED_TYPE_EDITOR,
		EMBED_TYPE_MAX,
	};

	inline static GameView* singleton = nullptr;

	Ref<GameViewDebugger> debugger;
	WindowWrapper* window_wrapper = nullptr;

	bool is_feature_enabled = true;
	int active_sessions = 0;
	int screen_index_before_start = -1;
	ScriptEditorDebugger* embedded_script_debugger = nullptr;

	bool embed_on_play = true;
	bool make_floating_on_play = true;
	EmbedSizeMode embed_size_mode = SIZE_MODE_FIXED;
	bool paused = false;
	Size2 size_paused;

	Rect2i floating_window_rect;
	int floating_window_screen = -1;

	bool debug_mute_audio = false;

	bool selection_hide = true;
	bool selection_avoid_locked = false;
	bool selection_prefer_group = false;

	Button* suspend_button = nullptr;
	Button* next_frame_button = nullptr;

	MenuButton* selection_options_menu = nullptr;

	Button* camera_override_button = nullptr;
	MenuButton* camera_override_menu = nullptr;

	Button* debug_mute_audio_button = nullptr;

	HBoxContainer* embedding_hb = nullptr;
	MenuButton* game_window_options_menu = nullptr;
	Label* game_size_label = nullptr;
	HBoxContainer* game_hb = nullptr;
	Button* game_embed_mode_button[EmbedMode::EMBED_TYPE_MAX];
	Panel* panel = nullptr;
	EmbeddedProcessBase* embedded_process = nullptr;
	Label* state_label = nullptr;

	const int DEFAULT_TIME_SCALE_INDEX = 5;
	int time_scale_index = DEFAULT_TIME_SCALE_INDEX;

	Size2i game_window_size = Size2i(-1, -1);
	bool hdr_output_enabled = false;
	float current_max_luminance = 0.0f;
	float current_reference_luminance = 0.0f;
	float output_max_linear_value = 1.0f;
	bool display_server_supports_hdr_output = false;
	bool renderer_supports_hdr_output = false;

	MenuButton* speed_state_button = nullptr;

	void _embedding_failed();
	void _embedded_process_focused();

	void _update_embed_menu_options();
	void _update_embed_window_size();

	void _update_floating_window_settings();
	void _remote_window_title_changed(String title);

	void _debugger_breaked(bool p_breaked, bool p_can_debug);

public:
	GameView(Ref<GameViewDebugger> p_debugger, EmbeddedProcessBase* p_embedded_process,
		WindowWrapper* p_wrapper)
		: debugger(p_debugger), embedded_process(p_embedded_process), window_wrapper(p_wrapper)
	{
	}
};

class GameViewPluginBase : public EditorPlugin
{
#ifndef ANDROID_ENABLED
	GameView* game_view = nullptr;
	WindowWrapper* window_wrapper = nullptr;
#endif // ANDROID_ENABLED

	Ref<GameViewDebugger> debugger;

	String last_editor;

	void _save_last_editor(const String& p_editor);
	void _focus_another_editor();
	bool _is_window_wrapper_enabled() const;

public:
	virtual void selected_notify() override;

	Ref<GameViewDebugger> get_debugger() const { return debugger; }

	GameViewPluginBase();
};

class GameViewPlugin : public GameViewPluginBase
{
public:
	GameViewPlugin() = default;
};


