/**************************************************************************/
/*  project_manager.cpp                                                   */
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

#include "project_manager.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "editor/file_system/editor_paths.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

static String _get_default_projects_dir()
{
	String home = OS::get_environment("HOME");
	if (home.is_empty()) {
		home = OS::get_environment("USERPROFILE");
	}
	if (!home.is_empty()) {
		return home.simplify_path();
	}
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_valid()) {
		return da->get_current_dir();
	}
	return ".";
}

static void _scan_dir_for_projects(
	const String& p_dir, int p_depth, int p_max_depth, LocalVector<String>& r_found)
{
	if (p_depth > p_max_depth) {
		return;
	}
	Ref<DirAccess> da = DirAccess::open(p_dir);
	if (da.is_null()) {
		return;
	}

	String cur_dir = da->get_current_dir();
	if (FileAccess::exists(cur_dir.path_join("project.godot"))) {
		r_found.push_back(cur_dir);
		return;
	}

	da->list_dir_begin();
	String item = da->get_next();
	while (!item.is_empty()) {
		if (item != "." && item != ".." && !item.begins_with(".")) {
			if (da->current_is_dir()) {
				_scan_dir_for_projects(cur_dir.path_join(item), p_depth + 1, p_max_depth, r_found);
			}
		}
		item = da->get_next();
	}
	da->list_dir_end();
}

bool ProjectManager::_has_valid_selected_project()
{
	if (!data || data->selected_project_idx < 0 ||
		data->selected_project_idx >= (int)data->projects.size()) {
		return false;
	}
	return !data->projects[data->selected_project_idx].missing;
}

bool ProjectManager::_has_any_selected_project()
{
	return data && data->selected_project_idx >= 0 &&
		   data->selected_project_idx < (int)data->projects.size();
}

void ProjectManager::load_projects()
{
	if (!data) {
		return;
	}
	data->projects.clear();

	String data_dir = EditorPaths::get_data_dir();
	if (data_dir.is_empty()) {
		data_dir = _get_default_projects_dir().path_join(".local/share/voltaire");
	}
	data->config_path = data_dir.path_join("projects.cfg");

	if (FileAccess::exists(data->config_path)) {
		Ref<FileAccess> fa = FileAccess::open(data->config_path, FileAccess::READ);
		if (fa.is_valid()) {
			String current_section;
			bool current_fav = false;
			auto flush_section = [&]() {
				if (!current_section.is_empty()) {
					ProjectEntry entry;
					entry.path = current_section.simplify_path();
					entry.favorite = current_fav;
					entry.missing = !FileAccess::exists(entry.path.path_join("project.godot"));
					entry.name = UIDraw::read_project_name(entry.path);
					data->projects.push_back(entry);
				}
			};

			while (!fa->eof_reached()) {
				String line = fa->get_line().strip_edges();
				if (line.is_empty() || line.begins_with(";") || line.begins_with("#")) {
					continue;
				}
				if (line.begins_with("[") && line.ends_with("]")) {
					flush_section();
					current_section = line.substr(1, line.length() - 2).strip_edges();
					current_fav = false;
				}
				else if (line.begins_with("favorite=")) {
					String val = line.substr(9).strip_edges().to_lower();
					current_fav = (val == "true" || val == "1");
				}
			}
			flush_section();
		}
	}

	rebuild_filtered_list();
	if (!data->filtered_indices.is_empty()) {
		data->selected_project_idx = data->filtered_indices[0];
		data->status_text = vformat("Loaded %d project(s).", data->projects.size());
	}
	else {
		data->selected_project_idx = -1;
		data->status_text =
			"No projects registered yet. Click '+ New Project', 'Import', or 'Scan' to begin.";
	}
}

void ProjectManager::save_projects()
{
	if (!data || data->config_path.is_empty()) {
		return;
	}
	String base_dir = data->config_path.get_base_dir();
	if (!DirAccess::dir_exists_absolute(base_dir)) {
		DirAccess::make_dir_recursive_absolute(base_dir);
	}
	Ref<FileAccess> fa = FileAccess::open(data->config_path, FileAccess::WRITE);
	if (fa.is_null()) {
		return;
	}
	for (uint32_t i = 0; i < data->projects.size(); i++) {
		const ProjectEntry& p = data->projects[i];
		fa->store_line("[" + p.path + "]");
		fa->store_line(String("favorite=") + (p.favorite ? "true" : "false"));
		fa->store_line("");
	}
}

void ProjectManager::rebuild_filtered_list()
{
	if (!data) {
		return;
	}
	data->filtered_indices.clear();
	String needle = data->search_field.text.strip_edges().to_lower();

	// Pass 1: favorites first, Pass 2: non-favorites
	for (int pass = 0; pass < 2; pass++) {
		const bool want_fav = (pass == 0);
		for (uint32_t i = 0; i < data->projects.size(); i++) {
			const ProjectEntry& p = data->projects[i];
			if (p.favorite != want_fav) {
				continue;
			}
			if (!needle.is_empty()) {
				if (!p.name.to_lower().contains(needle) && !p.path.to_lower().contains(needle)) {
					continue;
				}
			}
			data->filtered_indices.push_back((int)i);
		}
	}

	// Update Favorite button label dynamically based on current selection
	for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
		if (data->sidebar_buttons[i].on_click == &ProjectManager::_toggle_favorite_selected) {
			if (_has_any_selected_project() &&
				data->projects[data->selected_project_idx].favorite) {
				data->sidebar_buttons[i].label = "Unfavorite";
			}
			else {
				data->sidebar_buttons[i].label = "Favorite";
			}
		}
	}
}

void ProjectManager::initialize(RID p_viewport_rid, Size2i p_size)
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->ui.initialize(p_viewport_rid, p_size,
		Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT), &ProjectManager::handle_input_event);

	data->search_field.placeholder = "Filter projects by name or path...";

	// Top toolbar buttons
	data->buttons.add("+ New Project", &ProjectManager::_new_project, nullptr, true);
	data->buttons.add("Import", &ProjectManager::_import_project);
	data->buttons.add("Scan", &ProjectManager::_scan_projects);
	data->buttons.add("Support", &ProjectManager::_open_donate_page);

	// Right-hand project action sidebar buttons
	data->sidebar_buttons.add(
		"Edit", &ProjectManager::_edit_selected_project, &ProjectManager::_has_valid_selected_project, true);
	data->sidebar_buttons.add(
		"Run", &ProjectManager::_run_selected_project, &ProjectManager::_has_valid_selected_project);
	data->sidebar_buttons.add("Open Folder", &ProjectManager::_open_selected_folder,
		&ProjectManager::_has_valid_selected_project);
	data->sidebar_buttons.add("Favorite", &ProjectManager::_toggle_favorite_selected,
		&ProjectManager::_has_any_selected_project);
	data->sidebar_buttons.add("Remove", &ProjectManager::_remove_selected_project,
		&ProjectManager::_has_any_selected_project);
	data->sidebar_buttons.add("Quit", &ProjectManager::_quit_manager);

	load_projects();

	_initialized = true;
	data->ui.needs_redraw = false;
	process_and_draw();
}

void ProjectManager::handle_input_event(const InputEvent& p_event)
{
	if (!data || p_event.is_null()) {
		return;
	}
	data->ui.forward_input(p_event, &ProjectManager::handle_input_event);

	if (p_event.type == InputEventType::MOUSE_MOTION) {
		data->ui.mouse_pos = p_event.mouse_motion.position;
		bool changed = false;

		if (data->modal_mode != MODAL_NONE) {
			changed |= data->modal_buttons.update_hover(data->ui.mouse_pos);
		}
		else {
			changed |= data->buttons.update_hover(data->ui.mouse_pos);
			changed |= data->sidebar_buttons.update_hover(data->ui.mouse_pos);

			bool clear_h = !data->search_field.text.is_empty() &&
						   data->clear_search_rect.has_point(data->ui.mouse_pos);
			if (data->clear_search_hovered != clear_h) {
				data->clear_search_hovered = clear_h;
				changed = true;
			}

			int new_hover_proj = -1;
			if (data->list_scroll.bounds.has_point(data->ui.mouse_pos)) {
				for (uint32_t i = 0; i < data->filtered_indices.size(); i++) {
					int pidx = data->filtered_indices[i];
					if (data->projects[pidx].card_rect.has_point(data->ui.mouse_pos)) {
						new_hover_proj = pidx;
						break;
					}
				}
			}
			if (data->hovered_project_idx != new_hover_proj) {
				if (data->hovered_project_idx >= 0 &&
					data->hovered_project_idx < (int)data->projects.size()) {
					data->projects[data->hovered_project_idx].hovered = false;
				}
				data->hovered_project_idx = new_hover_proj;
				if (new_hover_proj >= 0) {
					data->projects[new_hover_proj].hovered = true;
				}
				changed = true;
			}
		}

		if (changed) {
			data->ui.needs_redraw = true;
		}
		return;
	}

	if (p_event.type == InputEventType::MOUSE_BUTTON) {
		const MouseButton btn = p_event.mouse_button.button_index;
		const bool is_pressed = p_event.is_pressed();
		data->ui.mouse_pos = p_event.mouse_button.position;

		if (is_pressed && data->modal_mode == MODAL_NONE &&
			data->list_scroll.bounds.has_point(data->ui.mouse_pos)) {
			if (data->list_scroll.handle_wheel(btn, 40.0f)) {
				data->ui.needs_redraw = true;
				return;
			}
		}

		if (btn != MouseButton::LEFT) {
			return;
		}

		if (data->modal_mode != MODAL_NONE) {
			if (is_pressed) {
				bool hit_button = data->modal_buttons.press(data->ui.mouse_pos);
				if (!hit_button) {
					if (data->modal_primary_field.has_point(data->ui.mouse_pos)) {
						data->active_field = FIELD_MODAL_PRIMARY;
					}
					else if (data->modal_mode == MODAL_NEW_PROJECT &&
							 data->modal_secondary_field.has_point(data->ui.mouse_pos)) {
						data->active_field = FIELD_MODAL_SECONDARY;
					}
					else if (!data->modal_panel_rect.has_point(data->ui.mouse_pos)) {
						_cancel_modal();
						return;
					}
				}
				data->ui.needs_redraw = true;
			}
			else {
				void (*to_invoke)() = data->modal_buttons.release(data->ui.mouse_pos);
				data->ui.needs_redraw = true;
				if (to_invoke) {
					to_invoke();
				}
			}
			return;
		}

		// Normal (non-modal) left click press / release
		if (is_pressed) {
			bool hit_ui = false;
			hit_ui |= data->buttons.press(data->ui.mouse_pos);
			hit_ui |= data->sidebar_buttons.press(data->ui.mouse_pos);

			if (!data->search_field.text.is_empty() &&
				data->clear_search_rect.has_point(data->ui.mouse_pos)) {
				data->search_field.text = String();
				rebuild_filtered_list();
				data->active_field = FIELD_SEARCH;
				data->ui.needs_redraw = true;
				return;
			}

			if (data->search_field.has_point(data->ui.mouse_pos)) {
				data->active_field = FIELD_SEARCH;
				data->ui.needs_redraw = true;
				return;
			}

			if (!hit_ui) {
				data->active_field = FIELD_NONE;
			}

			if (data->list_scroll.bounds.has_point(data->ui.mouse_pos)) {
				for (uint32_t i = 0; i < data->filtered_indices.size(); i++) {
					int pidx = data->filtered_indices[i];
					if (data->projects[pidx].card_rect.has_point(data->ui.mouse_pos)) {
						data->selected_project_idx = pidx;
						rebuild_filtered_list();
						const ProjectEntry& sel = data->projects[pidx];
						data->status_text = sel.missing
												? vformat("Selected missing project: %s", sel.path)
												: vformat("Selected '%s' (%s)", sel.name, sel.path);
						data->ui.needs_redraw = true;
						if (p_event.mouse_button.double_click && !sel.missing) {
							_edit_selected_project();
						}
						return;
					}
				}
			}

			data->ui.needs_redraw = true;
		}
		else {
			void (*c0)() = data->buttons.release(data->ui.mouse_pos);
			void (*c1)() = data->sidebar_buttons.release(data->ui.mouse_pos);
			void (*to_invoke)() = c1 ? c1 : c0;
			data->ui.needs_redraw = true;
			if (to_invoke) {
				to_invoke();
			}
		}
		return;
	}

	if (p_event.type == InputEventType::KEY && p_event.is_pressed()) {
		const Key code = p_event.key.keycode;

		if (code == Key::ESCAPE) {
			if (data->modal_mode != MODAL_NONE) {
				_cancel_modal();
			}
			else if (data->active_field == FIELD_SEARCH) {
				if (!data->search_field.text.is_empty()) {
					data->search_field.text = String();
					rebuild_filtered_list();
				}
				else {
					data->active_field = FIELD_NONE;
				}
				data->ui.needs_redraw = true;
			}
			return;
		}

		if (code == Key::TAB && data->modal_mode == MODAL_NEW_PROJECT) {
			data->active_field = (data->active_field == FIELD_MODAL_PRIMARY) ? FIELD_MODAL_SECONDARY
																			 : FIELD_MODAL_PRIMARY;
			data->ui.needs_redraw = true;
			return;
		}

		if (code == Key::ENTER || code == Key::KP_ENTER) {
			if (data->modal_mode != MODAL_NONE) {
				_confirm_modal();
			}
			else if (data->active_field == FIELD_SEARCH) {
				data->active_field = FIELD_NONE;
				data->ui.needs_redraw = true;
			}
			else if (_has_valid_selected_project()) {
				_edit_selected_project();
			}
			return;
		}

		UITextField* target_field = nullptr;
		if (data->active_field == FIELD_SEARCH) {
			target_field = &data->search_field;
		}
		else if (data->active_field == FIELD_MODAL_PRIMARY) {
			target_field = &data->modal_primary_field;
		}
		else if (data->active_field == FIELD_MODAL_SECONDARY) {
			target_field = &data->modal_secondary_field;
		}

		if (target_field) {
			if (target_field->handle_key(p_event)) {
				if (data->active_field == FIELD_SEARCH) {
					data->list_scroll.offset = 0.0f;
					rebuild_filtered_list();
				}
				data->modal_error_text = String();
				data->ui.needs_redraw = true;
			}
			return;
		}

		if (data->modal_mode == MODAL_NONE && !data->filtered_indices.is_empty() &&
			(code == Key::UP || code == Key::DOWN)) {
			int cur_pos = -1;
			for (uint32_t i = 0; i < data->filtered_indices.size(); i++) {
				if (data->filtered_indices[i] == data->selected_project_idx) {
					cur_pos = (int)i;
					break;
				}
			}
			if (cur_pos < 0) {
				cur_pos = 0;
			}
			else if (code == Key::UP && cur_pos > 0) {
				cur_pos--;
			}
			else if (code == Key::DOWN && cur_pos + 1 < (int)data->filtered_indices.size()) {
				cur_pos++;
			}
			data->selected_project_idx = data->filtered_indices[cur_pos];
			rebuild_filtered_list();
			data->ui.needs_redraw = true;
		}
	}
}

void ProjectManager::poll()
{
	if (!data) {
		return;
	}

	if (Input::is_initialized()) {
		Input::flush_buffered_events();
	}

	if (!data->pending_open_path.is_empty()) {
		Size2i win_size = data->ui.window_size;
		String open_path = data->pending_open_path;
		String open_name = data->pending_open_name;
		bool open_play = data->pending_open_play;
		finalize();
		EditorUI::initialize(RID(), win_size, open_path, open_name, open_play);
		return;
	}

	data->ui.sync_window_size();

	if (data->ui.needs_redraw) {
		data->ui.needs_redraw = false;
		process_and_draw();
	}
}

void ProjectManager::process_and_draw()
{
	if (!data) {
		return;
	}

	RID ci = data->ui.begin_draw();
	if (!ci.is_valid()) {
		return;
	}

	const float W = (float)data->ui.window_size.x;
	const float H = (float)data->ui.window_size.y;
	const float pad = 24.0f;
	const Ref<Font>& font = data->ui.font;

	// Palette
	const Color col_bg(0.09f, 0.10f, 0.12f, 1.0f);
	const Color col_header(0.13f, 0.15f, 0.18f, 1.0f);
	const Color col_accent(0.0f, 0.75f, 0.95f, 1.0f);
	const Color col_search_bg(0.06f, 0.07f, 0.09f, 1.0f);
	const Color col_workspace(0.12f, 0.14f, 0.17f, 1.0f);
	const Color col_sidebar(0.11f, 0.13f, 0.16f, 1.0f);
	const Color col_card(0.16f, 0.19f, 0.24f, 1.0f);
	const Color col_card_hover(0.20f, 0.24f, 0.31f, 1.0f);
	const Color col_card_selected(0.18f, 0.27f, 0.36f, 1.0f);
	const Color col_text(0.92f, 0.94f, 0.96f, 1.0f);
	const Color col_subtext(0.58f, 0.64f, 0.72f, 1.0f);
	const Color col_warn(0.95f, 0.45f, 0.40f, 1.0f);
	const Color col_border(0.18f, 0.21f, 0.26f, 1.0f);

	// 1. Full-window background
	Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), col_bg);

	const float content_w = MAX(0.0f, W - pad * 2.0f);
	if (content_w <= 0.0f || H <= 0.0f) {
		return;
	}

	// 2. Top Header Bar
	float y = 20.0f;
	const float header_h = 48.0f;
	Rect2 header_rect(pad, y, content_w, header_h);
	UIDraw::draw_header_banner(ci, header_rect, col_header, col_accent, 4.0f);

	// 3. Toolbar Buttons aligned to the right of the header bar
	const uint32_t btn_count = data->buttons.size();
	float btn_w = 126.0f;
	const float btn_h = 34.0f;
	const float btn_gap = 10.0f;
	if (btn_count > 0) {
		const float avail_btn_w = MAX(0.0f, content_w - 20.0f);
		const float total_gaps = btn_gap * (btn_count - 1);
		if (avail_btn_w < btn_count * btn_w + total_gaps) {
			btn_w = MAX(0.0f, (avail_btn_w - total_gaps) / (float)btn_count);
		}
	}
	float btn_x = (W - pad - 10.0f) - (btn_count * (btn_w + btn_gap)) + btn_gap;
	const float btn_y = y + (header_h - btn_h) * 0.5f;

	if (font.is_valid()) {
		const float title_max_w = MAX(0.0f, btn_x - (pad + 28.0f));
		if (title_max_w > 12.0f) {
			const float title_base_y = UIDraw::centered_baseline_y(font, header_rect, 16);
			font->draw_string(ci, Point2(pad + 18.0f, title_base_y),
				"VOLTAIRE // PROJECT MANAGER", HORIZONTAL_ALIGNMENT_LEFT, title_max_w, 16,
				col_text);
		}
	}

	for (uint32_t i = 0; i < btn_count; i++) {
		UIButton& btn = data->buttons[i];
		btn.rect = Rect2(btn_x, btn_y, btn_w, btn_h);
		UIDraw::draw_button(ci, font, btn, 14);
		btn_x += btn_w + btn_gap;
	}

	y += header_h + 12.0f;

	// 4. Horizontal Separator
	Renderer::canvas_item_add_rect(
		ci, Rect2(pad, y, content_w, 2.0f), Color(0.22f, 0.25f, 0.30f, 1.0f));
	y += 14.0f;

	// 5. Search Bar Box via composed UITextField
	const float search_h = 38.0f;
	data->search_field.rect = Rect2(pad, y, content_w, search_h);
	const bool has_clear = !data->search_field.text.is_empty() && content_w > 64.0f;
	const float clear_reserve = has_clear ? 36.0f : 0.0f;
	data->search_field.draw(ci, font, data->active_field == FIELD_SEARCH, 14, col_search_bg,
		col_accent, col_border, clear_reserve);

	data->clear_search_rect = Rect2();
	if (has_clear) {
		const float clr_sz = 24.0f;
		data->clear_search_rect =
			Rect2(pad + content_w - clr_sz - 8.0f, y + (search_h - clr_sz) * 0.5f, clr_sz, clr_sz);
		Renderer::canvas_item_add_rect(ci, data->clear_search_rect,
			data->clear_search_hovered ? Color(0.30f, 0.36f, 0.45f, 1.0f)
									   : Color(0.16f, 0.19f, 0.24f, 1.0f));
		if (font.is_valid()) {
			font->draw_string(ci,
				Point2(data->clear_search_rect.position.x,
					UIDraw::centered_baseline_y(font, data->clear_search_rect, 13)),
				"X", HORIZONTAL_ALIGNMENT_CENTER, clr_sz, 13, col_text);
		}
	}
	y += search_h + 14.0f;

	// 6. Bottom Status Bar reservation & Main Workspace Split via composed UISplitLayout
	const float status_h = 30.0f;
	const float workspace_h = MAX(0.0f, H - y - pad - status_h - 10.0f);

	for (uint32_t i = 0; i < data->projects.size(); i++) {
		data->projects[i].card_rect = Rect2();
	}

	if (workspace_h > 0.0f) {
		data->layout.compute_sidebar_split(
			Rect2(pad, y, content_w, workspace_h), 420.0f, 180.0f, 0.28f, 14.0f);
		const Rect2 list_rect = data->layout.primary_rect;
		const Rect2 sidebar_rect = data->layout.secondary_rect;

		data->list_scroll.bounds = list_rect;
		Renderer::canvas_item_add_rect(ci, list_rect, col_workspace);

		// Project cards inside list_scroll.bounds
		const float list_pad = 12.0f;
		const float card_h = 66.0f;
		const float card_gap = 8.0f;
		const float inner_h = MAX(0.0f, workspace_h - list_pad * 2.0f);
		const float card_w = MAX(0.0f, list_rect.size.x - list_pad * 2.0f - 8.0f);
		const uint32_t vis_count = data->filtered_indices.size();
		const float total_list_h =
			vis_count > 0 ? vis_count * card_h + (vis_count - 1) * card_gap : 0.0f;

		data->list_scroll.update_metrics(total_list_h, inner_h);

		if (vis_count == 0) {
			if (font.is_valid() && card_w > 24.0f && inner_h > 30.0f) {
				String empty_msg = data->projects.is_empty()
									   ? "No projects yet. Use '+ New Project', 'Import', or 'Scan'."
									   : "No projects match the current filter.";
				font->draw_string(ci, Point2(pad + list_pad, y + workspace_h * 0.5f), empty_msg,
					HORIZONTAL_ALIGNMENT_CENTER, card_w, 15, col_subtext);
			}
		}
		else if (card_w > 16.0f && inner_h > 16.0f) {
			const float top_clip = y + list_pad;
			const float bot_clip = y + workspace_h - list_pad;

			for (uint32_t i = 0; i < vis_count; i++) {
				int pidx = data->filtered_indices[i];
				ProjectEntry& proj = data->projects[pidx];
				float cy = top_clip + i * (card_h + card_gap) - data->list_scroll.offset;
				if (cy + card_h <= top_clip || cy >= bot_clip) {
					continue;
				}
				float draw_y = MAX(cy, top_clip);
				float draw_end = MIN(cy + card_h, bot_clip);
				float draw_h = draw_end - draw_y;
				if (draw_h <= 4.0f) {
					continue;
				}

				Rect2 crect(pad + list_pad, draw_y, card_w, draw_h);
				proj.card_rect = crect;

				const bool is_sel = (pidx == data->selected_project_idx);
				Color cbg = is_sel ? col_card_selected : (proj.hovered ? col_card_hover : col_card);
				Color strip_col =
					proj.missing
						? col_warn
						: (is_sel ? col_accent
								  : (proj.favorite ? Color(0.95f, 0.78f, 0.20f, 1.0f)
												   : Color(0.26f, 0.31f, 0.38f, 1.0f)));
				UIDraw::draw_header_banner(ci, crect, cbg, strip_col, 4.0f);

				if (font.is_valid() && draw_h >= 48.0f && card_w > 32.0f) {
					String title = proj.favorite ? ("[★] " + proj.name) : proj.name;
					if (proj.missing) {
						title += "  [Missing]";
					}
					font->draw_string(ci, Point2(crect.position.x + 16.0f, cy + 27.0f), title,
						HORIZONTAL_ALIGNMENT_LEFT, card_w - 28.0f, 15,
						proj.missing ? col_warn : col_text);
					font->draw_string(ci, Point2(crect.position.x + 16.0f, cy + 50.0f), proj.path,
						HORIZONTAL_ALIGNMENT_LEFT, card_w - 28.0f, 12, col_subtext);
				}
			}

			data->list_scroll.draw_scrollbar(
				ci, pad + list_rect.size.x - 8.0f, top_clip, inner_h, total_list_h, col_accent);
		}

		// Right Action Sidebar
		if (data->layout.show_secondary) {
			const float sx = sidebar_rect.position.x;
			const float sidebar_w = sidebar_rect.size.x;
			Renderer::canvas_item_add_rect(ci, sidebar_rect, col_sidebar);

			float sy = y + 12.0f;
			if (font.is_valid() && sidebar_w > 24.0f) {
				font->draw_string(ci, Point2(sx + 12.0f, sy + 14.0f), "ACTIONS",
					HORIZONTAL_ALIGNMENT_LEFT, sidebar_w - 24.0f, 12, col_subtext);
				sy += 24.0f;
			}

			const float sbtn_h = 34.0f;
			const float sbtn_gap = 8.0f;
			const float sbtn_w = MAX(0.0f, sidebar_w - 24.0f);
			for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
				UIButton& sbtn = data->sidebar_buttons[i];
				if (i + 1 == data->sidebar_buttons.size()) {
					float bottom_y = y + workspace_h - 12.0f - sbtn_h;
					if (bottom_y > sy + 8.0f) {
						sy = bottom_y;
					}
				}
				if (sy + sbtn_h <= y + workspace_h - 6.0f) {
					sbtn.rect = Rect2(sx + 12.0f, sy, sbtn_w, sbtn_h);
					UIDraw::draw_button(ci, font, sbtn, 14);
				}
				else {
					sbtn.rect = Rect2();
				}
				sy += sbtn_h + sbtn_gap;
			}
		}
		else {
			data->sidebar_buttons.clear_rects();
		}
	}

	// 8. Bottom Status Bar
	const float status_y = H - pad - status_h;
	if (status_y > y) {
		Rect2 status_rect(pad, status_y, content_w, status_h);
		UIDraw::draw_header_banner(ci, status_rect, col_header, col_accent, 3.0f);
		if (font.is_valid() && content_w > 24.0f) {
			String count_badge =
				vformat("%d / %d projects", data->filtered_indices.size(), data->projects.size());
			float badge_w = MIN(160.0f, content_w * 0.3f);
			float msg_w = MAX(0.0f, content_w - badge_w - 32.0f);
			float sbase_y = UIDraw::centered_baseline_y(font, status_rect, 13);
			if (msg_w > 16.0f) {
				font->draw_string(ci, Point2(pad + 14.0f, sbase_y), data->status_text,
					HORIZONTAL_ALIGNMENT_LEFT, msg_w, 13, col_text);
			}
			font->draw_string(ci, Point2(pad + content_w - badge_w - 12.0f, sbase_y), count_badge,
				HORIZONTAL_ALIGNMENT_RIGHT, badge_w, 13, col_subtext);
		}
	}

	// 9. Inline Modal Dialog Overlay (New Project / Import / Scan)
	if (data->modal_mode != MODAL_NONE) {
		Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), Color(0.03f, 0.04f, 0.06f, 0.72f));

		const float mw = MIN(540.0f, MAX(240.0f, content_w - 24.0f));
		const float mh = (data->modal_mode == MODAL_NEW_PROJECT) ? 268.0f : 204.0f;
		const float mx = (W - mw) * 0.5f;
		const float my = MAX(20.0f, (H - mh) * 0.5f);
		data->modal_panel_rect = Rect2(mx, my, mw, mh);

		UIDraw::draw_bordered_rect(
			ci, data->modal_panel_rect, Color(0.13f, 0.15f, 0.19f, 1.0f), col_accent, 2.0f);

		String modal_title;
		String label_1;
		String label_2;
		if (data->modal_mode == MODAL_NEW_PROJECT) {
			modal_title = "CREATE NEW VOLTAIRE PROJECT";
			label_1 = "Project Name:";
			label_2 = "Parent Directory:";
		}
		else if (data->modal_mode == MODAL_IMPORT_PROJECT) {
			modal_title = "IMPORT EXISTING PROJECT";
			label_1 = "Project Folder or project.godot Path:";
		}
		else {
			modal_title = "SCAN DIRECTORY FOR PROJECTS";
			label_1 = "Directory to Scan Recursively:";
		}

		float cy = my + 16.0f;
		const float inner_w = MAX(0.0f, mw - 36.0f);
		if (font.is_valid() && inner_w > 20.0f) {
			font->draw_string(ci, Point2(mx + 18.0f, cy + 16.0f), modal_title,
				HORIZONTAL_ALIGNMENT_LEFT, inner_w, 15, col_text);
		}
		cy += 28.0f;
		const Color modal_field_border(0.24f, 0.28f, 0.34f, 1.0f);
		Renderer::canvas_item_add_rect(ci, Rect2(mx + 18.0f, cy, inner_w, 1.0f), modal_field_border);
		cy += 14.0f;

		// Primary Field
		if (font.is_valid() && inner_w > 20.0f) {
			font->draw_string(ci, Point2(mx + 18.0f, cy + 12.0f), label_1,
				HORIZONTAL_ALIGNMENT_LEFT, inner_w, 13, col_subtext);
		}
		cy += 18.0f;
		data->modal_primary_field.rect = Rect2(mx + 18.0f, cy, inner_w, 34.0f);
		data->modal_primary_field.draw(ci, font, data->active_field == FIELD_MODAL_PRIMARY, 14,
			col_search_bg, col_accent, modal_field_border);
		cy += 44.0f;

		// Secondary Field (New Project parent directory)
		if (data->modal_mode == MODAL_NEW_PROJECT) {
			if (font.is_valid() && inner_w > 20.0f) {
				font->draw_string(ci, Point2(mx + 18.0f, cy + 12.0f), label_2,
					HORIZONTAL_ALIGNMENT_LEFT, inner_w, 13, col_subtext);
			}
			cy += 18.0f;
			data->modal_secondary_field.rect = Rect2(mx + 18.0f, cy, inner_w, 34.0f);
			data->modal_secondary_field.draw(ci, font, data->active_field == FIELD_MODAL_SECONDARY,
				14, col_search_bg, col_accent, modal_field_border);
			cy += 42.0f;
		}
		else {
			data->modal_secondary_field.rect = Rect2();
		}

		// Modal error or target preview hint
		const float mbtn_w = 108.0f;
		const float mbtn_h = 34.0f;
		const float mbtn_gap = 8.0f;
		const float total_mbtns_w =
			data->modal_buttons.size() * (mbtn_w + mbtn_gap) - mbtn_gap;

		if (font.is_valid() && inner_w > 20.0f) {
			float hint_max_w = MAX(20.0f, inner_w - total_mbtns_w - 14.0f);
			if (!data->modal_error_text.is_empty()) {
				font->draw_string(ci, Point2(mx + 18.0f, my + mh - 24.0f), data->modal_error_text,
					HORIZONTAL_ALIGNMENT_LEFT, hint_max_w, 12, col_warn);
			}
			else if (data->modal_mode == MODAL_NEW_PROJECT) {
				String folder_slug = data->modal_primary_field.text.strip_edges();
				String preview =
					data->modal_secondary_field.text.strip_edges().path_join(folder_slug);
				font->draw_string(ci, Point2(mx + 18.0f, my + mh - 24.0f), preview,
					HORIZONTAL_ALIGNMENT_LEFT, hint_max_w, 12, col_subtext);
			}
		}

		// Modal Action Buttons (bottom-right)
		float mbtn_x = mx + mw - 18.0f - total_mbtns_w;
		float mbtn_y = my + mh - 16.0f - mbtn_h;
		for (uint32_t i = 0; i < data->modal_buttons.size(); i++) {
			data->modal_buttons[i].rect = Rect2(mbtn_x, mbtn_y, mbtn_w, mbtn_h);
			UIDraw::draw_button(ci, font, data->modal_buttons[i], 13);
			mbtn_x += mbtn_w + mbtn_gap;
		}
	}
}

void ProjectManager::finalize()
{
	if (!data) {
		return;
	}
	data->ui.teardown(&ProjectManager::handle_input_event);
	memdelete(data);
	data = nullptr;
	_initialized = false;
}

void ProjectManager::_new_project()
{
	if (!data) {
		return;
	}
	data->modal_mode = MODAL_NEW_PROJECT;
	data->modal_primary_field.text = "New Voltaire Project";
	data->modal_secondary_field.text = _get_default_projects_dir();
	data->modal_error_text = String();
	data->active_field = FIELD_MODAL_PRIMARY;
	data->modal_buttons.clear();
	data->modal_buttons.add("Cancel", &ProjectManager::_cancel_modal);
	data->modal_buttons.add("Create", &ProjectManager::_confirm_modal);
	data->modal_buttons.add("Create & Edit", &ProjectManager::_confirm_modal_and_edit, nullptr, true);
	data->ui.needs_redraw = true;
}

void ProjectManager::_import_project()
{
	if (!data) {
		return;
	}
	data->modal_mode = MODAL_IMPORT_PROJECT;
	data->modal_primary_field.text = _get_default_projects_dir();
	data->modal_secondary_field.text = String();
	data->modal_error_text = String();
	data->active_field = FIELD_MODAL_PRIMARY;
	data->modal_buttons.clear();
	data->modal_buttons.add("Cancel", &ProjectManager::_cancel_modal);
	data->modal_buttons.add("Import", &ProjectManager::_confirm_modal, nullptr, true);
	data->ui.needs_redraw = true;
}

void ProjectManager::_scan_projects()
{
	if (!data) {
		return;
	}
	data->modal_mode = MODAL_SCAN_PROJECTS;
	data->modal_primary_field.text = _get_default_projects_dir();
	data->modal_secondary_field.text = String();
	data->modal_error_text = String();
	data->active_field = FIELD_MODAL_PRIMARY;
	data->modal_buttons.clear();
	data->modal_buttons.add("Cancel", &ProjectManager::_cancel_modal);
	data->modal_buttons.add("Scan Folder", &ProjectManager::_confirm_modal, nullptr, true);
	data->ui.needs_redraw = true;
}

void ProjectManager::_open_donate_page()
{
	Error err = OS::shell_open("https://ko-fi.com/bendyland");
	if (data) {
		data->status_text = (err == OK) ? "Opened support page in browser."
										: "Support URL: https://ko-fi.com/bendyland";
		data->ui.needs_redraw = true;
	}
}

void ProjectManager::_edit_selected_project()
{
	if (!_has_valid_selected_project()) {
		return;
	}
	const ProjectEntry& proj = data->projects[data->selected_project_idx];
	data->pending_open_path = proj.path;
	data->pending_open_name = proj.name;
	data->pending_open_play = false;
}

void ProjectManager::_run_selected_project()
{
	if (!_has_valid_selected_project()) {
		return;
	}
	const ProjectEntry& proj = data->projects[data->selected_project_idx];
	data->pending_open_path = proj.path;
	data->pending_open_name = proj.name;
	data->pending_open_play = true;
}

void ProjectManager::_open_selected_folder()
{
	if (!_has_valid_selected_project()) {
		return;
	}
	const ProjectEntry& proj = data->projects[data->selected_project_idx];
	Error err = OS::shell_show_in_file_manager(proj.path, true);
	data->status_text = (err == OK) ? vformat("Opened folder: %s", proj.path)
									: vformat("Could not open folder: %s", proj.path);
	data->ui.needs_redraw = true;
}

void ProjectManager::_toggle_favorite_selected()
{
	if (!_has_any_selected_project()) {
		return;
	}
	ProjectEntry& proj = data->projects[data->selected_project_idx];
	proj.favorite = !proj.favorite;
	String target_path = proj.path;
	save_projects();
	rebuild_filtered_list();
	for (uint32_t i = 0; i < data->projects.size(); i++) {
		if (data->projects[i].path == target_path) {
			data->selected_project_idx = (int)i;
			break;
		}
	}
	data->status_text = proj.favorite ? vformat("Pinned '%s' to favorites.", proj.name)
									  : vformat("Unpinned '%s' from favorites.", proj.name);
	data->ui.needs_redraw = true;
}

void ProjectManager::_remove_selected_project()
{
	if (!_has_any_selected_project()) {
		return;
	}
	String removed_name = data->projects[data->selected_project_idx].name;
	data->projects.remove_at(data->selected_project_idx);
	save_projects();
	rebuild_filtered_list();
	if (!data->filtered_indices.is_empty()) {
		data->selected_project_idx = data->filtered_indices[0];
	}
	else {
		data->selected_project_idx = -1;
	}
	data->status_text = vformat("Removed '%s' from project list.", removed_name);
	data->ui.needs_redraw = true;
}

void ProjectManager::_quit_manager()
{
	extern bool s_quit_requested;
	s_quit_requested = true;
}

void ProjectManager::_cancel_modal()
{
	if (!data) {
		return;
	}
	data->modal_mode = MODAL_NONE;
	data->active_field = FIELD_NONE;
	data->modal_error_text = String();
	data->modal_buttons.clear();
	data->ui.needs_redraw = true;
}

void ProjectManager::_confirm_modal_and_edit()
{
	if (!data) {
		return;
	}
	_confirm_modal();
	if (data && data->modal_mode == MODAL_NONE && _has_valid_selected_project()) {
		_edit_selected_project();
	}
}

void ProjectManager::_confirm_modal()
{
	if (!data) {
		return;
	}

	auto register_project_dir = [&](const String& p_dir) -> int {
		String clean = p_dir.simplify_path();
		for (uint32_t i = 0; i < data->projects.size(); i++) {
			if (data->projects[i].path == clean) {
				data->projects[i].missing = !FileAccess::exists(clean.path_join("project.godot"));
				data->projects[i].name = UIDraw::read_project_name(clean);
				return (int)i;
			}
		}
		ProjectEntry entry;
		entry.path = clean;
		entry.name = UIDraw::read_project_name(clean);
		entry.missing = !FileAccess::exists(clean.path_join("project.godot"));
		data->projects.push_back(entry);
		return (int)data->projects.size() - 1;
	};

	if (data->modal_mode == MODAL_NEW_PROJECT) {
		String proj_name = data->modal_primary_field.text.strip_edges();
		String parent_dir = data->modal_secondary_field.text.strip_edges().simplify_path();
		if (proj_name.is_empty()) {
			data->modal_error_text = "Project name cannot be empty.";
			data->ui.needs_redraw = true;
			return;
		}
		if (parent_dir.is_empty()) {
			data->modal_error_text = "Parent directory cannot be empty.";
			data->ui.needs_redraw = true;
			return;
		}
		String target_dir = parent_dir.path_join(proj_name);
		String proj_file = target_dir.path_join("project.godot");
		if (FileAccess::exists(proj_file)) {
			data->modal_error_text = "A project already exists at that path.";
			data->ui.needs_redraw = true;
			return;
		}
		Error dir_err = DirAccess::make_dir_recursive_absolute(target_dir);
		if (dir_err != OK && !DirAccess::dir_exists_absolute(target_dir)) {
			data->modal_error_text = "Failed to create project directory.";
			data->ui.needs_redraw = true;
			return;
		}
		Ref<FileAccess> fa = FileAccess::open(proj_file, FileAccess::WRITE);
		if (fa.is_null()) {
			data->modal_error_text = "Failed to write project.godot.";
			data->ui.needs_redraw = true;
			return;
		}
		fa->store_line("; Engine configuration file.");
		fa->store_line("config_version=5");
		fa->store_line("");
		fa->store_line("[application]");
		fa->store_line("");
		fa->store_line(vformat("config/name=\"%s\"", proj_name.replace("\"", "'")));
		fa->store_line("config/features=PackedStringArray(\"4.6\")");
		fa->close();

		int idx = register_project_dir(target_dir);
		save_projects();
		rebuild_filtered_list();
		data->selected_project_idx = idx;
		rebuild_filtered_list();
		data->status_text = vformat("Created project '%s' at %s", proj_name, target_dir);
		_cancel_modal();
		return;
	}

	if (data->modal_mode == MODAL_IMPORT_PROJECT) {
		String raw_path = data->modal_primary_field.text.strip_edges().simplify_path();
		if (raw_path.ends_with("/project.godot")) {
			raw_path = raw_path.get_base_dir();
		}
		if (raw_path.is_empty() || !FileAccess::exists(raw_path.path_join("project.godot"))) {
			data->modal_error_text = "No project.godot found in the specified directory.";
			data->ui.needs_redraw = true;
			return;
		}
		int idx = register_project_dir(raw_path);
		save_projects();
		rebuild_filtered_list();
		data->selected_project_idx = idx;
		rebuild_filtered_list();
		data->status_text = vformat("Imported project '%s'.", data->projects[idx].name);
		_cancel_modal();
		return;
	}

	if (data->modal_mode == MODAL_SCAN_PROJECTS) {
		String scan_dir = data->modal_primary_field.text.strip_edges().simplify_path();
		if (scan_dir.is_empty() || !DirAccess::dir_exists_absolute(scan_dir)) {
			data->modal_error_text = "Directory does not exist.";
			data->ui.needs_redraw = true;
			return;
		}
		LocalVector<String> found;
		_scan_dir_for_projects(scan_dir, 0, 3, found);
		int added = 0;
		int last_idx = data->selected_project_idx;
		for (uint32_t i = 0; i < found.size(); i++) {
			uint32_t before = data->projects.size();
			last_idx = register_project_dir(found[i]);
			if (data->projects.size() > before) {
				added++;
			}
		}
		save_projects();
		rebuild_filtered_list();
		if (last_idx >= 0) {
			data->selected_project_idx = last_idx;
			rebuild_filtered_list();
		}
		data->status_text =
			vformat("Scanned %s: %d project(s) found (%d new).", scan_dir, found.size(), added);
		_cancel_modal();
		return;
	}
}
