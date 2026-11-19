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

#include "editor/ui_widgets.h"

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
		// Composed shared Canvas UI context, layout & widgets
		UIContext ui;
		UISplitLayout layout;
		UIScrollView list_scroll;
		UIButtonGroup buttons;
		UIButtonGroup sidebar_buttons;
		UIButtonGroup modal_buttons;
		UITextField search_field;
		UITextField modal_primary_field;
		UITextField modal_secondary_field;

		String status_text = "Voltaire Project Manager ready.";
		String config_path;

		LocalVector<ProjectEntry> projects;
		LocalVector<int> filtered_indices;
		int selected_project_idx = -1;
		int hovered_project_idx = -1;

		Rect2 clear_search_rect;
		bool clear_search_hovered = false;

		ModalMode modal_mode = MODAL_NONE;
		ActiveField active_field = FIELD_NONE;
		String modal_error_text;
		Rect2 modal_panel_rect;

		String pending_open_path;
		String pending_open_name;
		bool pending_open_play = false;
	};

	static inline Data* data = nullptr;

	// Lifecycle & Frame Hooks
	static bool is_initialized() { return _initialized; }
	static void initialize(RID p_viewport_rid, Size2i p_size);
	static void poll();
	static void process_and_draw();
	static void finalize();

	static void handle_input_event(const Ref<InputEvent>& p_event);
	static void load_projects();
	static void save_projects();
	static void rebuild_filtered_list();

	// Button callbacks
	static void _new_project();
	static void _import_project();
	static void _scan_projects();
	static void _open_donate_page();
	static void _edit_selected_project();
	static void _run_selected_project();
	static void _open_selected_folder();
	static void _toggle_favorite_selected();
	static void _remove_selected_project();
	static void _quit_manager();
	static void _cancel_modal();
	static void _confirm_modal();
	static void _confirm_modal_and_edit();

	static bool _has_valid_selected_project();
	static bool _has_any_selected_project();
};
