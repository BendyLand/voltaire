/**************************************************************************/
/*  project_manager.h                                                     */
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

class ProjectManager
{
private:
	static inline bool _initialized = false;

public:
	ProjectManager() = delete;

	static constexpr int DEFAULT_WINDOW_WIDTH = 1152;
	static constexpr int DEFAULT_WINDOW_HEIGHT = 648;

	enum ModalMode
	{
		MODAL_NONE = 0,
		MODAL_NEW_PROJECT,
		MODAL_IMPORT_PROJECT,
		MODAL_SCAN_PROJECTS,
	};

	enum ActiveField
	{
		FIELD_NONE = 0,
		FIELD_SEARCH,
		FIELD_MODAL_PRIMARY,
		FIELD_MODAL_SECONDARY,
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
	};

	struct ProjectEntry
	{
		String name;
		String path;
		bool favorite = false;
		bool missing = false;
		Rect2 card_rect;
		bool hovered = false;
	};

	struct Data
	{
		RID viewport_rid;
		RID canvas_rid;
		RID canvas_item_rid;

		Size2i window_size = Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
		Point2 mouse_pos;
		String search_text;
		String status_text = "Voltaire Project Manager ready.";
		String config_path;

		Ref<Font> ui_font;
		LocalVector<UIButton> buttons;
		LocalVector<UIButton> sidebar_buttons;
		LocalVector<UIButton> modal_buttons;

		LocalVector<ProjectEntry> projects;
		LocalVector<int> filtered_indices;
		int selected_project_idx = -1;
		int hovered_project_idx = -1;
		float scroll_offset = 0.0f;
		float max_scroll = 0.0f;

		Rect2 search_rect;
		Rect2 clear_search_rect;
		bool clear_search_hovered = false;
		Rect2 workspace_list_rect;

		ModalMode modal_mode = MODAL_NONE;
		ActiveField active_field = FIELD_NONE;
		String modal_field_primary;
		String modal_field_secondary;
		String modal_error_text;
		Rect2 modal_panel_rect;
		Rect2 modal_primary_rect;
		Rect2 modal_secondary_rect;

		Input::EventDispatchFunc prev_input_dispatch = nullptr;
		String pending_open_path;
		String pending_open_name;
		bool pending_open_play = false;
		bool owns_viewport = false;
		bool needs_redraw = true;
	};

	static inline Data* data = nullptr;

	// Lifecycle & Frame Hooks
	static bool is_initialized() { return _initialized; }
	static void initialize(RID p_viewport_rid = RID(),
		Size2i p_size = Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT));
	static void poll();
	static void process_and_draw();
	static void finalize();

	// Input & Project Helpers
	static void handle_input_event(const Ref<InputEvent>& p_event);
	static void load_projects();
	static void save_projects();
	static void rebuild_filtered_list();

	// Toolbar Button Actions
	static void _new_project();
	static void _import_project();
	static void _scan_projects();
	static void _open_donate_page();

	// Sidebar Button Actions
	static void _edit_selected_project();
	static void _run_selected_project();
	static void _open_selected_folder();
	static void _toggle_favorite_selected();
	static void _remove_selected_project();
	static void _quit_manager();

	// Modal Actions
	static void _confirm_modal();
	static void _confirm_modal_and_edit();
	static void _cancel_modal();

	// Predicates
	static bool _has_valid_selected_project();
	static bool _has_any_selected_project();
};
