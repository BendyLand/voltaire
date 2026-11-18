/**************************************************************************/
/*  editor_ui.h                                                           */
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

#pragma once

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/math/color.h"
#include "core/math/rect2.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "scene/resources/font.h"

class EditorUI
{
private:
	static inline bool _initialized = false;

public:
	EditorUI() = delete;

	static constexpr int DEFAULT_WINDOW_WIDTH = 1280;
	static constexpr int DEFAULT_WINDOW_HEIGHT = 720;

	enum ViewMode
	{
		VIEW_2D = 0,
		VIEW_3D = 1,
		VIEW_SCRIPT = 2,
	};

	enum FocusField
	{
		FOCUS_NONE = 0,
		FOCUS_NODE_NAME,
		FOCUS_SCRIPT_EDITOR,
	};

	struct UIButton
	{
		String label;
		Rect2 rect;
		void (*on_click)() = nullptr;
		bool (*is_enabled)() = nullptr;
		bool hovered = false;
		bool pressed = false;
		bool primary = false;
		bool active_tab = false;
	};

	struct SceneEntity
	{
		String name;
		String type = "Node2D";
		Vector2 position;
		Size2 size = Size2(96.0f, 64.0f);
		Color color = Color(0.0f, 0.75f, 0.95f, 1.0f);
		Vector2 runtime_pos;
		Vector2 runtime_vel = Vector2(140.0f, 95.0f);
		Rect2 tree_row_rect;
		bool tree_hovered = false;
	};

	struct FileEntry
	{
		String name;
		String full_path;
		bool is_dir = false;
		Rect2 row_rect;
		bool hovered = false;
	};

	struct ColorSwatch
	{
		Color color;
		Rect2 rect;
		bool hovered = false;
	};

	struct Data
	{
		RID viewport_rid;
		RID canvas_rid;
		RID canvas_item_rid;

		Size2i window_size = Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
		Point2 mouse_pos;

		String project_path;
		String project_name = "Untitled Project";

		Ref<Font> ui_font;
		ViewMode view_mode = VIEW_2D;
		FocusField focus_field = FOCUS_NONE;

		bool playing = false;
		bool launched_directly_to_play = false;
		double play_time = 0.0;
		double fps_accum = 0.0;
		int fps_frames = 0;
		int display_fps = 60;

		// Viewport interaction state
		Rect2 center_viewport_rect;
		Vector2 pan_2d = Vector2(0.0f, 0.0f);
		bool dragging_node_2d = false;
		Vector2 drag_node_offset;
		bool panning_viewport = false;
		float orbit_yaw = 0.65f;
		float orbit_pitch = 0.42f;

		// Scene & Inspector state
		LocalVector<SceneEntity> entities;
		int selected_entity_idx = 0;
		int next_entity_serial = 1;
		bool scene_dirty = false;
		Rect2 inspector_name_rect;
		LocalVector<ColorSwatch> swatches;

		// FileSystem & Script editor state
		LocalVector<FileEntry> files;
		int selected_file_idx = -1;
		String open_file_path;
		String open_file_name;
		Vector<String> script_lines;
		int caret_line = 0;
		int script_scroll_line = 0;
		bool script_dirty = false;

		// Output log
		Vector<String> logs;

		// Buttons
		LocalVector<UIButton> top_buttons;
		LocalVector<UIButton> dock_buttons;
		LocalVector<UIButton> inspector_buttons;
		LocalVector<UIButton> runtime_buttons;

		Input::EventDispatchFunc prev_input_dispatch = nullptr;
		bool owns_viewport = false;
		bool needs_redraw = true;
		bool pending_return_to_pm = false;
	};

	static inline Data* data = nullptr;

	static bool is_initialized() { return _initialized; }
	static void initialize(RID p_viewport_rid, Size2i p_size, const String& p_project_path,
		const String& p_project_name, bool p_start_playing = false);
	static void poll(double p_delta = 0.016);
	static void process_and_draw();
	static void finalize();

	static void handle_input_event(const Ref<InputEvent>& p_event);
	static void add_log(const String& p_msg);
	static void scan_project_files();
	static void load_scene();
	static void save_scene();
	static void open_text_file(const String& p_path);
	static void save_open_text_file();
	static void start_play();
	static void stop_play();

	// Button callbacks
	static void _back_to_project_list();
	static void _switch_view_2d();
	static void _switch_view_3d();
	static void _switch_view_script();
	static void _add_node();
	static void _save_all();
	static void _toggle_play();
	static void _stop_and_edit();
	static void _reset_simulation();
	static void _reset_camera_view();
	static void _refresh_files();
	static void _new_script_file();
	static void _clear_output();
	static void _duplicate_selected_node();
	static void _delete_selected_node();
	static void _nudge_pos_x_neg();
	static void _nudge_pos_x_pos();
	static void _nudge_pos_y_neg();
	static void _nudge_pos_y_pos();
	static void _nudge_size_w_neg();
	static void _nudge_size_w_pos();
	static void _nudge_size_h_neg();
	static void _nudge_size_h_pos();

	static bool _can_modify_selected_node();
	static bool _can_delete_selected_node();
};
