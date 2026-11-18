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
#include "scene/theme/theme_db.h"
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

static String _read_project_name_from_dir(const String& p_dir)
{
	String proj_file = p_dir.path_join("project.godot");
	if (!FileAccess::exists(proj_file)) {
		return p_dir.get_file();
	}
	Ref<FileAccess> fa = FileAccess::open(proj_file, FileAccess::READ);
	if (fa.is_null()) {
		return p_dir.get_file();
	}
	while (!fa->eof_reached()) {
		String line = fa->get_line().strip_edges();
		if (line.begins_with("config/name=")) {
			String val = line.substr(12).strip_edges();
			if (val.length() >= 2 && val.begins_with("\"") && val.ends_with("\"")) {
				val = val.substr(1, val.length() - 2);
			}
			if (!val.is_empty()) {
				return val;
			}
		}
	}
	String fallback = p_dir.get_file();
	return fallback.is_empty() ? p_dir : fallback;
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

static void _draw_bordered_rect(RID p_ci, const Rect2& p_rect, const Color& p_fill,
	const Color& p_border, float p_border_w = 1.0f)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_fill);
	const float bw = MIN(p_border_w, MIN(p_rect.size.x * 0.5f, p_rect.size.y * 0.5f));
	if (bw > 0.0f) {
		Renderer::canvas_item_add_rect(
			p_ci, Rect2(p_rect.position.x, p_rect.position.y, p_rect.size.x, bw), p_border);
		Renderer::canvas_item_add_rect(p_ci,
			Rect2(p_rect.position.x, p_rect.position.y + p_rect.size.y - bw, p_rect.size.x, bw),
			p_border);
		Renderer::canvas_item_add_rect(
			p_ci, Rect2(p_rect.position.x, p_rect.position.y, bw, p_rect.size.y), p_border);
		Renderer::canvas_item_add_rect(p_ci,
			Rect2(p_rect.position.x + p_rect.size.x - bw, p_rect.position.y, bw, p_rect.size.y),
			p_border);
	}
}

static float _centered_baseline_y(const Ref<Font>& p_font, const Rect2& p_rect, int p_font_size)
{
	if (p_font.is_valid()) {
		float ascent = p_font->get_ascent(p_font_size);
		float descent = p_font->get_descent(p_font_size);
		if (ascent > 0.0f) {
			return Math::round(p_rect.position.y + (p_rect.size.y + ascent - descent) * 0.5f);
		}
	}
	return Math::round(p_rect.position.y + p_rect.size.y * 0.65f);
}

static void _draw_ui_button(RID p_ci, const Ref<Font>& p_font, const ProjectManager::UIButton& p_btn,
	int p_font_size = 14)
{
	if (p_btn.rect.size.x <= 2.0f || p_btn.rect.size.y <= 2.0f) {
		return;
	}
	const bool enabled = (p_btn.is_enabled == nullptr) || p_btn.is_enabled();

	Color bg(0.20f, 0.24f, 0.30f, 1.0f);
	Color accent(0.0f, 0.75f, 0.95f, 1.0f);
	Color text_col(0.92f, 0.94f, 0.96f, 1.0f);

	if (!enabled) {
		bg = Color(0.14f, 0.16f, 0.19f, 1.0f);
		accent = Color(0.22f, 0.26f, 0.31f, 1.0f);
		text_col = Color(0.45f, 0.49f, 0.55f, 1.0f);
	}
	else if (p_btn.primary) {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.0f, 0.50f, 0.68f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.05f, 0.68f, 0.88f, 1.0f);
		}
		else {
			bg = Color(0.02f, 0.58f, 0.78f, 1.0f);
		}
		accent = Color(0.45f, 0.92f, 1.0f, 1.0f);
		text_col = Color(0.98f, 0.99f, 1.0f, 1.0f);
	}
	else {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.15f, 0.20f, 0.27f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.28f, 0.35f, 0.45f, 1.0f);
		}
	}

	Renderer::canvas_item_add_rect(p_ci, p_btn.rect, bg);
	Renderer::canvas_item_add_rect(p_ci,
		Rect2(p_btn.rect.position.x, p_btn.rect.position.y + p_btn.rect.size.y - 2.0f,
			p_btn.rect.size.x, 2.0f),
		accent);

	if (p_font.is_valid() && p_btn.rect.size.x > 16.0f) {
		const float pad_x = 8.0f;
		const float avail_w = p_btn.rect.size.x - pad_x * 2.0f;
		const float base_y = _centered_baseline_y(p_font, p_btn.rect, p_font_size);
		p_font->draw_string(p_ci, Point2(p_btn.rect.position.x + pad_x, base_y), p_btn.label,
			HORIZONTAL_ALIGNMENT_CENTER, avail_w, p_font_size, text_col);
	}
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
					entry.name = _read_project_name_from_dir(entry.path);
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
	String needle = data->search_text.strip_edges().to_lower();

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
			if (_has_any_selected_project() && data->projects[data->selected_project_idx].favorite) {
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
	data->window_size = (p_size.x > 0 && p_size.y > 0)
							? p_size
							: Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

	// 1. Allocate or reuse the root Viewport & Canvas directly on the Renderer
	if (p_viewport_rid.is_valid()) {
		data->viewport_rid = p_viewport_rid;
		data->owns_viewport = false;
	}
	else {
		data->viewport_rid = Renderer::viewport_create();
		data->owns_viewport = true;
	}

	data->canvas_rid = Renderer::canvas_create();
	Renderer::viewport_attach_canvas(data->viewport_rid, data->canvas_rid);
	Renderer::viewport_set_size(data->viewport_rid, data->window_size.x, data->window_size.y);
	Renderer::viewport_set_active(data->viewport_rid, true);
	Renderer::viewport_set_update_mode(data->viewport_rid, RSE::VIEWPORT_UPDATE_ALWAYS);
	Renderer::viewport_attach_to_screen(data->viewport_rid,
		Rect2(0, 0, data->window_size.x, data->window_size.y), DisplayServerEnums::MAIN_WINDOW_ID);

	// 2. Allocate a single CanvasItem for the ProjectManager UI
	data->canvas_item_rid = Renderer::canvas_item_create();
	Renderer::canvas_item_set_parent(data->canvas_item_rid, data->canvas_rid);
	Renderer::canvas_item_set_visible(data->canvas_item_rid, true);

	// 3. Grab default fallback font from ThemeDB if available
	if (ThemeDB::is_initialized()) {
		data->ui_font = ThemeDB::get_fallback_font();
	}
	print_verbose(vformat("ProjectManager: TextServer='%s', ui_font=%s, test_size=%v",
		TS.is_valid() ? TS->get_name() : "null", data->ui_font.is_valid() ? "valid" : "null",
		data->ui_font.is_valid() ? data->ui_font->get_string_size("VOLTAIRE", HORIZONTAL_ALIGNMENT_LEFT, -1, 16)
								 : Size2()));

	// 4. Register top toolbar buttons
	data->buttons.push_back(
		{"+ New Project", Rect2(), &ProjectManager::_new_project, nullptr, false, false, true});
	data->buttons.push_back(
		{"Import", Rect2(), &ProjectManager::_import_project, nullptr, false, false, false});
	data->buttons.push_back(
		{"Scan", Rect2(), &ProjectManager::_scan_projects, nullptr, false, false, false});
	data->buttons.push_back(
		{"Support", Rect2(), &ProjectManager::_open_donate_page, nullptr, false, false, false});

	// 5. Register right-hand project action sidebar buttons
	data->sidebar_buttons.push_back({"Edit", Rect2(), &ProjectManager::_edit_selected_project,
		&ProjectManager::_has_valid_selected_project, false, false, true});
	data->sidebar_buttons.push_back({"Run", Rect2(), &ProjectManager::_run_selected_project,
		&ProjectManager::_has_valid_selected_project, false, false, false});
	data->sidebar_buttons.push_back({"Open Folder", Rect2(), &ProjectManager::_open_selected_folder,
		&ProjectManager::_has_valid_selected_project, false, false, false});
	data->sidebar_buttons.push_back({"Favorite", Rect2(),
		&ProjectManager::_toggle_favorite_selected, &ProjectManager::_has_any_selected_project,
		false, false, false});
	data->sidebar_buttons.push_back({"Remove", Rect2(), &ProjectManager::_remove_selected_project,
		&ProjectManager::_has_any_selected_project, false, false, false});
	data->sidebar_buttons.push_back(
		{"Quit", Rect2(), &ProjectManager::_quit_manager, nullptr, false, false, false});

	// 6. Hook Input event dispatch so mouse/keyboard events drive the UI on all platforms
	if (Input::is_initialized() && Input::data) {
		data->prev_input_dispatch = Input::data->event_dispatch_function;
		Input::set_event_dispatch_function(&ProjectManager::handle_input_event);
	}

	load_projects();

	_initialized = true;
	data->needs_redraw = false;
	process_and_draw();
}

void ProjectManager::handle_input_event(const Ref<InputEvent>& p_event)
{
	if (!data || p_event.is_null()) {
		return;
	}
	if (data->prev_input_dispatch && data->prev_input_dispatch != &ProjectManager::handle_input_event) {
		data->prev_input_dispatch(p_event);
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		data->mouse_pos = mm->get_position();
		bool changed = false;

		if (data->modal_mode != MODAL_NONE) {
			for (uint32_t i = 0; i < data->modal_buttons.size(); i++) {
				bool h = data->modal_buttons[i].rect.has_point(data->mouse_pos);
				if (data->modal_buttons[i].hovered != h) {
					data->modal_buttons[i].hovered = h;
					changed = true;
				}
			}
		}
		else {
			for (uint32_t i = 0; i < data->buttons.size(); i++) {
				bool h = data->buttons[i].rect.has_point(data->mouse_pos);
				if (data->buttons[i].hovered != h) {
					data->buttons[i].hovered = h;
					changed = true;
				}
			}
			for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
				bool h = data->sidebar_buttons[i].rect.has_point(data->mouse_pos);
				if (data->sidebar_buttons[i].hovered != h) {
					data->sidebar_buttons[i].hovered = h;
					changed = true;
				}
			}
			bool clear_h =
				!data->search_text.is_empty() && data->clear_search_rect.has_point(data->mouse_pos);
			if (data->clear_search_hovered != clear_h) {
				data->clear_search_hovered = clear_h;
				changed = true;
			}

			int new_hover_proj = -1;
			if (data->workspace_list_rect.has_point(data->mouse_pos)) {
				for (uint32_t i = 0; i < data->filtered_indices.size(); i++) {
					int pidx = data->filtered_indices[i];
					if (data->projects[pidx].card_rect.has_point(data->mouse_pos)) {
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
			data->needs_redraw = true;
		}
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		data->mouse_pos = mb->get_position();

		if (mb->is_pressed() && data->modal_mode == MODAL_NONE &&
			data->workspace_list_rect.has_point(data->mouse_pos)) {
			if (mb->get_button_index() == MouseButton::WHEEL_UP) {
				float next_scroll = MAX(0.0f, data->scroll_offset - 40.0f);
				if (next_scroll != data->scroll_offset) {
					data->scroll_offset = next_scroll;
					data->needs_redraw = true;
				}
				return;
			}
			if (mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				float next_scroll = MIN(data->max_scroll, data->scroll_offset + 40.0f);
				if (next_scroll != data->scroll_offset) {
					data->scroll_offset = next_scroll;
					data->needs_redraw = true;
				}
				return;
			}
		}

		if (mb->get_button_index() != MouseButton::LEFT) {
			return;
		}

		if (data->modal_mode != MODAL_NONE) {
			if (mb->is_pressed()) {
				bool hit_button = false;
				for (uint32_t i = 0; i < data->modal_buttons.size(); i++) {
					UIButton& btn = data->modal_buttons[i];
					btn.hovered = btn.rect.has_point(data->mouse_pos);
					const bool enabled = (btn.is_enabled == nullptr) || btn.is_enabled();
					btn.pressed = btn.hovered && enabled;
					if (btn.pressed) {
						hit_button = true;
					}
				}
				if (!hit_button) {
					if (data->modal_primary_rect.has_point(data->mouse_pos)) {
						data->active_field = FIELD_MODAL_PRIMARY;
					}
					else if (data->modal_mode == MODAL_NEW_PROJECT &&
							 data->modal_secondary_rect.has_point(data->mouse_pos)) {
						data->active_field = FIELD_MODAL_SECONDARY;
					}
					else if (!data->modal_panel_rect.has_point(data->mouse_pos)) {
						_cancel_modal();
						return;
					}
				}
				data->needs_redraw = true;
			}
			else {
				void (*to_invoke)() = nullptr;
				for (uint32_t i = 0; i < data->modal_buttons.size(); i++) {
					UIButton& btn = data->modal_buttons[i];
					const bool was_pressed = btn.pressed;
					btn.pressed = false;
					btn.hovered = btn.rect.has_point(data->mouse_pos);
					const bool enabled = (btn.is_enabled == nullptr) || btn.is_enabled();
					if (was_pressed && btn.hovered && enabled && btn.on_click) {
						to_invoke = btn.on_click;
					}
				}
				data->needs_redraw = true;
				if (to_invoke) {
					to_invoke();
				}
			}
			return;
		}

		// Normal (non-modal) left click press / release
		if (mb->is_pressed()) {
			bool hit_ui = false;
			for (uint32_t i = 0; i < data->buttons.size(); i++) {
				UIButton& btn = data->buttons[i];
				btn.hovered = btn.rect.has_point(data->mouse_pos);
				const bool enabled = (btn.is_enabled == nullptr) || btn.is_enabled();
				btn.pressed = btn.hovered && enabled;
				if (btn.pressed) {
					hit_ui = true;
				}
			}
			for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
				UIButton& btn = data->sidebar_buttons[i];
				btn.hovered = btn.rect.has_point(data->mouse_pos);
				const bool enabled = (btn.is_enabled == nullptr) || btn.is_enabled();
				btn.pressed = btn.hovered && enabled;
				if (btn.pressed) {
					hit_ui = true;
				}
			}

			if (!data->search_text.is_empty() &&
				data->clear_search_rect.has_point(data->mouse_pos)) {
				data->search_text = String();
				rebuild_filtered_list();
				data->active_field = FIELD_SEARCH;
				data->needs_redraw = true;
				return;
			}

			if (data->search_rect.has_point(data->mouse_pos)) {
				data->active_field = FIELD_SEARCH;
				data->needs_redraw = true;
				return;
			}

			if (!hit_ui) {
				data->active_field = FIELD_NONE;
			}

			if (data->workspace_list_rect.has_point(data->mouse_pos)) {
				for (uint32_t i = 0; i < data->filtered_indices.size(); i++) {
					int pidx = data->filtered_indices[i];
					if (data->projects[pidx].card_rect.has_point(data->mouse_pos)) {
						data->selected_project_idx = pidx;
						rebuild_filtered_list();
						const ProjectEntry& sel = data->projects[pidx];
						data->status_text = sel.missing
												? vformat("Selected missing project: %s", sel.path)
												: vformat("Selected '%s' (%s)", sel.name, sel.path);
						data->needs_redraw = true;
						if (mb->is_double_click() && !sel.missing) {
							_edit_selected_project();
						}
						return;
					}
				}
			}

			data->needs_redraw = true;
		}
		else {
			void (*to_invoke)() = nullptr;
			for (uint32_t i = 0; i < data->buttons.size(); i++) {
				UIButton& btn = data->buttons[i];
				const bool was_pressed = btn.pressed;
				btn.pressed = false;
				btn.hovered = btn.rect.has_point(data->mouse_pos);
				const bool enabled = (btn.is_enabled == nullptr) || btn.is_enabled();
				if (was_pressed && btn.hovered && enabled && btn.on_click) {
					to_invoke = btn.on_click;
				}
			}
			for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
				UIButton& btn = data->sidebar_buttons[i];
				const bool was_pressed = btn.pressed;
				btn.pressed = false;
				btn.hovered = btn.rect.has_point(data->mouse_pos);
				const bool enabled = (btn.is_enabled == nullptr) || btn.is_enabled();
				if (was_pressed && btn.hovered && enabled && btn.on_click) {
					to_invoke = btn.on_click;
				}
			}
			data->needs_redraw = true;
			if (to_invoke) {
				to_invoke();
			}
		}
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed()) {
		const Key code = k->get_keycode();

		if (code == Key::ESCAPE) {
			if (data->modal_mode != MODAL_NONE) {
				_cancel_modal();
			}
			else if (data->active_field == FIELD_SEARCH) {
				if (!data->search_text.is_empty()) {
					data->search_text = String();
					rebuild_filtered_list();
				}
				else {
					data->active_field = FIELD_NONE;
				}
				data->needs_redraw = true;
			}
			return;
		}

		if (code == Key::TAB && data->modal_mode == MODAL_NEW_PROJECT) {
			data->active_field = (data->active_field == FIELD_MODAL_PRIMARY) ? FIELD_MODAL_SECONDARY
																			 : FIELD_MODAL_PRIMARY;
			data->needs_redraw = true;
			return;
		}

		if (code == Key::ENTER || code == Key::KP_ENTER) {
			if (data->modal_mode != MODAL_NONE) {
				_confirm_modal();
			}
			else if (data->active_field == FIELD_SEARCH) {
				data->active_field = FIELD_NONE;
				data->needs_redraw = true;
			}
			else if (_has_valid_selected_project()) {
				_edit_selected_project();
			}
			return;
		}

		String* target_str = nullptr;
		if (data->active_field == FIELD_SEARCH) {
			target_str = &data->search_text;
		}
		else if (data->active_field == FIELD_MODAL_PRIMARY) {
			target_str = &data->modal_field_primary;
		}
		else if (data->active_field == FIELD_MODAL_SECONDARY) {
			target_str = &data->modal_field_secondary;
		}

		if (target_str) {
			if (code == Key::BACKSPACE) {
				if (!target_str->is_empty()) {
					*target_str = target_str->substr(0, target_str->length() - 1);
					if (data->active_field == FIELD_SEARCH) {
						data->scroll_offset = 0.0f;
						rebuild_filtered_list();
					}
					data->modal_error_text = String();
					data->needs_redraw = true;
				}
				return;
			}
			if (k->is_ctrl_pressed() && code == Key::V && DisplayServer::is_initialized()) {
				String clip = DisplayServer::clipboard_get().replace("\r", "").replace("\n", "");
				if (!clip.is_empty()) {
					*target_str += clip;
					if (data->active_field == FIELD_SEARCH) {
						data->scroll_offset = 0.0f;
						rebuild_filtered_list();
					}
					data->modal_error_text = String();
					data->needs_redraw = true;
				}
				return;
			}
			char32_t ch = k->get_unicode();
			if (ch >= 32 && ch != 127 && !k->is_ctrl_pressed() && !k->is_meta_pressed()) {
				*target_str += String::chr(ch);
				if (data->active_field == FIELD_SEARCH) {
					data->scroll_offset = 0.0f;
					rebuild_filtered_list();
				}
				data->modal_error_text = String();
				data->needs_redraw = true;
				return;
			}
		}
		else if (data->modal_mode == MODAL_NONE && !data->filtered_indices.is_empty()) {
			if (code == Key::UP || code == Key::DOWN) {
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
				data->needs_redraw = true;
			}
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
		Size2i win_size = data->window_size;
		String open_path = data->pending_open_path;
		String open_name = data->pending_open_name;
		bool open_play = data->pending_open_play;
		finalize();
		EditorUI::initialize(RID(), win_size, open_path, open_name, open_play);
		return;
	}

	if (DisplayServer::is_initialized()) {
		Size2i ds_size = DisplayServer::window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
		if (ds_size.x > 0 && ds_size.y > 0 && ds_size != data->window_size) {
			data->window_size = ds_size;
			Renderer::viewport_set_size(
				data->viewport_rid, data->window_size.x, data->window_size.y);
			Renderer::viewport_attach_to_screen(data->viewport_rid,
				Rect2(0, 0, data->window_size.x, data->window_size.y),
				DisplayServerEnums::MAIN_WINDOW_ID);
			data->needs_redraw = true;
		}
	}

	if (data->needs_redraw) {
		data->needs_redraw = false;
		process_and_draw();
	}
}

void ProjectManager::process_and_draw()
{
	if (!data || !data->canvas_item_rid.is_valid()) {
		return;
	}

	RID ci = data->canvas_item_rid;
	Renderer::canvas_item_clear(ci);

	const float W = (float)data->window_size.x;
	const float H = (float)data->window_size.y;
	const float pad = 24.0f;

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

	// 1. Full-window background
	Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), col_bg);

	const float content_w = MAX(0.0f, W - pad * 2.0f);
	if (content_w <= 0.0f || H <= 0.0f) {
		return;
	}

	// 2. Top Header Bar (y = 20, height = 48)
	float y = 20.0f;
	const float header_h = 48.0f;
	Rect2 header_rect(pad, y, content_w, header_h);
	Renderer::canvas_item_add_rect(ci, header_rect, col_header);
	Renderer::canvas_item_add_rect(ci, Rect2(pad, y, MIN(4.0f, content_w), header_h), col_accent);

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

	if (data->ui_font.is_valid()) {
		const float title_max_w = MAX(0.0f, btn_x - (pad + 28.0f));
		if (title_max_w > 12.0f) {
			const float title_base_y = _centered_baseline_y(data->ui_font, header_rect, 16);
			data->ui_font->draw_string(ci, Point2(pad + 18.0f, title_base_y),
				"VOLTAIRE // PROJECT MANAGER", HORIZONTAL_ALIGNMENT_LEFT, title_max_w, 16,
				col_text);
		}
	}

	for (uint32_t i = 0; i < btn_count; i++) {
		UIButton& btn = data->buttons[i];
		btn.rect = Rect2(btn_x, btn_y, btn_w, btn_h);
		_draw_ui_button(ci, data->ui_font, btn, 14);
		btn_x += btn_w + btn_gap;
	}

	y += header_h + 12.0f;

	// 4. Horizontal Separator (height = 2)
	Renderer::canvas_item_add_rect(
		ci, Rect2(pad, y, content_w, 2.0f), Color(0.22f, 0.25f, 0.30f, 1.0f));
	y += 14.0f;

	// 5. Search Bar Box (height = 38)
	const float search_h = 38.0f;
	data->search_rect = Rect2(pad, y, content_w, search_h);
	const bool search_focused = (data->active_field == FIELD_SEARCH);
	_draw_bordered_rect(ci, data->search_rect, col_search_bg,
		search_focused ? col_accent : Color(0.18f, 0.21f, 0.26f, 1.0f),
		search_focused ? 2.0f : 1.0f);

	data->clear_search_rect = Rect2();
	float search_text_max_w = content_w - 24.0f;
	if (!data->search_text.is_empty() && content_w > 64.0f) {
		const float clr_sz = 24.0f;
		data->clear_search_rect =
			Rect2(pad + content_w - clr_sz - 8.0f, y + (search_h - clr_sz) * 0.5f, clr_sz, clr_sz);
		Renderer::canvas_item_add_rect(ci, data->clear_search_rect,
			data->clear_search_hovered ? Color(0.30f, 0.36f, 0.45f, 1.0f)
									   : Color(0.16f, 0.19f, 0.24f, 1.0f));
		if (data->ui_font.is_valid()) {
			data->ui_font->draw_string(ci,
				Point2(data->clear_search_rect.position.x,
					_centered_baseline_y(data->ui_font, data->clear_search_rect, 13)),
				"X", HORIZONTAL_ALIGNMENT_CENTER, clr_sz, 13, col_text);
		}
		search_text_max_w -= (clr_sz + 12.0f);
	}

	if (data->ui_font.is_valid() && search_text_max_w > 12.0f) {
		const bool empty = data->search_text.is_empty();
		String display_search = empty ? (search_focused ? "|" : "Filter projects by name or path...")
									  : (search_focused ? data->search_text + "|" : data->search_text);
		Color search_col = empty && !search_focused ? col_subtext : col_text;
		data->ui_font->draw_string(ci,
			Point2(pad + 12.0f, _centered_baseline_y(data->ui_font, data->search_rect, 14)),
			display_search, HORIZONTAL_ALIGNMENT_LEFT, search_text_max_w, 14, search_col);
	}
	y += search_h + 14.0f;

	// 6. Bottom Status Bar reservation (height = 30)
	const float status_h = 30.0f;
	const float workspace_h = MAX(0.0f, H - y - pad - status_h - 10.0f);

	// 7. Main Workspace Split: Left Project List + Right Action Sidebar
	for (uint32_t i = 0; i < data->projects.size(); i++) {
		data->projects[i].card_rect = Rect2();
	}

	if (workspace_h > 0.0f) {
		const bool show_sidebar = content_w >= 420.0f;
		const float sidebar_gap = show_sidebar ? 14.0f : 0.0f;
		const float sidebar_w = show_sidebar ? MIN(180.0f, content_w * 0.28f) : 0.0f;
		const float list_w = MAX(0.0f, content_w - sidebar_w - sidebar_gap);

		data->workspace_list_rect = Rect2(pad, y, list_w, workspace_h);
		Renderer::canvas_item_add_rect(ci, data->workspace_list_rect, col_workspace);

		// Project cards inside workspace_list_rect
		const float list_pad = 12.0f;
		const float card_h = 66.0f;
		const float card_gap = 8.0f;
		const float inner_h = MAX(0.0f, workspace_h - list_pad * 2.0f);
		const float card_w = MAX(0.0f, list_w - list_pad * 2.0f - 8.0f);
		const uint32_t vis_count = data->filtered_indices.size();
		const float total_list_h =
			vis_count > 0 ? vis_count * card_h + (vis_count - 1) * card_gap : 0.0f;

		data->max_scroll = MAX(0.0f, total_list_h - inner_h);
		data->scroll_offset = CLAMP(data->scroll_offset, 0.0f, data->max_scroll);

		if (vis_count == 0) {
			if (data->ui_font.is_valid() && card_w > 24.0f && inner_h > 30.0f) {
				String empty_msg = data->projects.is_empty()
									   ? "No projects yet. Use '+ New Project', 'Import', or 'Scan'."
									   : "No projects match the current filter.";
				data->ui_font->draw_string(ci,
					Point2(pad + list_pad, y + workspace_h * 0.5f), empty_msg,
					HORIZONTAL_ALIGNMENT_CENTER, card_w, 15, col_subtext);
			}
		}
		else if (card_w > 16.0f && inner_h > 16.0f) {
			const float top_clip = y + list_pad;
			const float bot_clip = y + workspace_h - list_pad;

			for (uint32_t i = 0; i < vis_count; i++) {
				int pidx = data->filtered_indices[i];
				ProjectEntry& proj = data->projects[pidx];
				float cy = top_clip + i * (card_h + card_gap) - data->scroll_offset;
				if (cy + card_h <= top_clip || cy >= bot_clip) {
					continue;
				}
				// Clip partially visible cards cleanly to list bounds
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
				Renderer::canvas_item_add_rect(ci, crect, cbg);

				Color strip_col = proj.missing
									  ? col_warn
									  : (is_sel ? col_accent
												: (proj.favorite ? Color(0.95f, 0.78f, 0.20f, 1.0f)
																 : Color(0.26f, 0.31f, 0.38f, 1.0f)));
				Renderer::canvas_item_add_rect(
					ci, Rect2(crect.position.x, crect.position.y, MIN(4.0f, card_w), draw_h),
					strip_col);

				if (data->ui_font.is_valid() && draw_h >= 48.0f && card_w > 32.0f) {
					String title = proj.favorite ? ("[★] " + proj.name) : proj.name;
					if (proj.missing) {
						title += "  [Missing]";
					}
					data->ui_font->draw_string(ci, Point2(crect.position.x + 16.0f, cy + 27.0f),
						title, HORIZONTAL_ALIGNMENT_LEFT, card_w - 28.0f, 15,
						proj.missing ? col_warn : col_text);
					data->ui_font->draw_string(ci, Point2(crect.position.x + 16.0f, cy + 50.0f),
						proj.path, HORIZONTAL_ALIGNMENT_LEFT, card_w - 28.0f, 12, col_subtext);
				}
			}

			// Scrollbar indicator if list overflows
			if (data->max_scroll > 0.0f && inner_h > 12.0f) {
				float track_x = pad + list_w - 8.0f;
				Renderer::canvas_item_add_rect(ci, Rect2(track_x, top_clip, 4.0f, inner_h),
					Color(0.08f, 0.09f, 0.11f, 1.0f));
				float thumb_h = MAX(20.0f, inner_h * (inner_h / total_list_h));
				float thumb_y =
					top_clip + (inner_h - thumb_h) * (data->scroll_offset / data->max_scroll);
				Renderer::canvas_item_add_rect(
					ci, Rect2(track_x, thumb_y, 4.0f, thumb_h), col_accent);
			}
		}

		// Right Action Sidebar
		if (show_sidebar) {
			const float sx = pad + list_w + sidebar_gap;
			Rect2 sidebar_rect(sx, y, sidebar_w, workspace_h);
			Renderer::canvas_item_add_rect(ci, sidebar_rect, col_sidebar);

			float sy = y + 12.0f;
			if (data->ui_font.is_valid() && sidebar_w > 24.0f) {
				data->ui_font->draw_string(ci, Point2(sx + 12.0f, sy + 14.0f), "ACTIONS",
					HORIZONTAL_ALIGNMENT_LEFT, sidebar_w - 24.0f, 12, col_subtext);
				sy += 24.0f;
			}

			const float sbtn_h = 34.0f;
			const float sbtn_gap = 8.0f;
			const float sbtn_w = MAX(0.0f, sidebar_w - 24.0f);
			for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
				UIButton& sbtn = data->sidebar_buttons[i];
				// Push 'Quit' to the bottom of the sidebar when there is room
				if (i + 1 == data->sidebar_buttons.size()) {
					float bottom_y = y + workspace_h - 12.0f - sbtn_h;
					if (bottom_y > sy + 8.0f) {
						sy = bottom_y;
					}
				}
				if (sy + sbtn_h <= y + workspace_h - 6.0f) {
					sbtn.rect = Rect2(sx + 12.0f, sy, sbtn_w, sbtn_h);
					_draw_ui_button(ci, data->ui_font, sbtn, 14);
				}
				else {
					sbtn.rect = Rect2();
				}
				sy += sbtn_h + sbtn_gap;
			}
		}
		else {
			for (uint32_t i = 0; i < data->sidebar_buttons.size(); i++) {
				data->sidebar_buttons[i].rect = Rect2();
			}
		}
	}

	// 8. Bottom Status Bar
	const float status_y = H - pad - status_h;
	if (status_y > y) {
		Rect2 status_rect(pad, status_y, content_w, status_h);
		Renderer::canvas_item_add_rect(ci, status_rect, col_header);
		Renderer::canvas_item_add_rect(
			ci, Rect2(pad, status_y, MIN(3.0f, content_w), status_h), col_accent);
		if (data->ui_font.is_valid() && content_w > 24.0f) {
			String count_badge =
				vformat("%d / %d projects", data->filtered_indices.size(), data->projects.size());
			float badge_w = MIN(160.0f, content_w * 0.3f);
			float msg_w = MAX(0.0f, content_w - badge_w - 32.0f);
			float sbase_y = _centered_baseline_y(data->ui_font, status_rect, 13);
			if (msg_w > 16.0f) {
				data->ui_font->draw_string(ci, Point2(pad + 14.0f, sbase_y), data->status_text,
					HORIZONTAL_ALIGNMENT_LEFT, msg_w, 13, col_text);
			}
			data->ui_font->draw_string(ci, Point2(pad + content_w - badge_w - 12.0f, sbase_y),
				count_badge, HORIZONTAL_ALIGNMENT_RIGHT, badge_w, 13, col_subtext);
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

		_draw_bordered_rect(ci, data->modal_panel_rect, Color(0.13f, 0.15f, 0.19f, 1.0f),
			col_accent, 2.0f);

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
		if (data->ui_font.is_valid() && inner_w > 20.0f) {
			data->ui_font->draw_string(ci, Point2(mx + 18.0f, cy + 16.0f), modal_title,
				HORIZONTAL_ALIGNMENT_LEFT, inner_w, 15, col_text);
		}
		cy += 28.0f;
		Renderer::canvas_item_add_rect(
			ci, Rect2(mx + 18.0f, cy, inner_w, 1.0f), Color(0.24f, 0.28f, 0.34f, 1.0f));
		cy += 14.0f;

		// Primary Field
		if (data->ui_font.is_valid() && inner_w > 20.0f) {
			data->ui_font->draw_string(ci, Point2(mx + 18.0f, cy + 12.0f), label_1,
				HORIZONTAL_ALIGNMENT_LEFT, inner_w, 13, col_subtext);
		}
		cy += 18.0f;
		data->modal_primary_rect = Rect2(mx + 18.0f, cy, inner_w, 34.0f);
		const bool p1_focus = (data->active_field == FIELD_MODAL_PRIMARY);
		_draw_bordered_rect(ci, data->modal_primary_rect, col_search_bg,
			p1_focus ? col_accent : Color(0.24f, 0.28f, 0.34f, 1.0f), p1_focus ? 2.0f : 1.0f);
		if (data->ui_font.is_valid() && inner_w > 20.0f) {
			String txt = p1_focus ? (data->modal_field_primary + "|") : data->modal_field_primary;
			data->ui_font->draw_string(ci,
				Point2(mx + 28.0f, _centered_baseline_y(data->ui_font, data->modal_primary_rect, 14)),
				txt, HORIZONTAL_ALIGNMENT_LEFT, inner_w - 20.0f, 14, col_text);
		}
		cy += 44.0f;

		// Secondary Field (New Project parent directory)
		if (data->modal_mode == MODAL_NEW_PROJECT) {
			if (data->ui_font.is_valid() && inner_w > 20.0f) {
				data->ui_font->draw_string(ci, Point2(mx + 18.0f, cy + 12.0f), label_2,
					HORIZONTAL_ALIGNMENT_LEFT, inner_w, 13, col_subtext);
			}
			cy += 18.0f;
			data->modal_secondary_rect = Rect2(mx + 18.0f, cy, inner_w, 34.0f);
			const bool p2_focus = (data->active_field == FIELD_MODAL_SECONDARY);
			_draw_bordered_rect(ci, data->modal_secondary_rect, col_search_bg,
				p2_focus ? col_accent : Color(0.24f, 0.28f, 0.34f, 1.0f), p2_focus ? 2.0f : 1.0f);
			if (data->ui_font.is_valid() && inner_w > 20.0f) {
				String txt =
					p2_focus ? (data->modal_field_secondary + "|") : data->modal_field_secondary;
				data->ui_font->draw_string(ci,
					Point2(mx + 28.0f,
						_centered_baseline_y(data->ui_font, data->modal_secondary_rect, 14)),
					txt, HORIZONTAL_ALIGNMENT_LEFT, inner_w - 20.0f, 14, col_text);
			}
			cy += 42.0f;
		}
		else {
			data->modal_secondary_rect = Rect2();
		}

		// Modal error or target preview hint
		const float mbtn_w = 108.0f;
		const float mbtn_h = 34.0f;
		const float mbtn_gap = 8.0f;
		const float total_mbtns_w =
			data->modal_buttons.size() * (mbtn_w + mbtn_gap) - mbtn_gap;

		if (data->ui_font.is_valid() && inner_w > 20.0f) {
			float hint_max_w = MAX(20.0f, inner_w - total_mbtns_w - 14.0f);
			if (!data->modal_error_text.is_empty()) {
				data->ui_font->draw_string(ci, Point2(mx + 18.0f, my + mh - 24.0f),
					data->modal_error_text, HORIZONTAL_ALIGNMENT_LEFT, hint_max_w, 12, col_warn);
			}
			else if (data->modal_mode == MODAL_NEW_PROJECT) {
				String folder_slug = data->modal_field_primary.strip_edges();
				String preview = data->modal_field_secondary.strip_edges().path_join(folder_slug);
				data->ui_font->draw_string(ci, Point2(mx + 18.0f, my + mh - 24.0f), preview,
					HORIZONTAL_ALIGNMENT_LEFT, hint_max_w, 12, col_subtext);
			}
		}

		// Modal Action Buttons (bottom-right)
		float mbtn_x = mx + mw - 18.0f - total_mbtns_w;
		float mbtn_y = my + mh - 16.0f - mbtn_h;
		for (uint32_t i = 0; i < data->modal_buttons.size(); i++) {
			data->modal_buttons[i].rect = Rect2(mbtn_x, mbtn_y, mbtn_w, mbtn_h);
			_draw_ui_button(ci, data->ui_font, data->modal_buttons[i], 13);
			mbtn_x += mbtn_w + mbtn_gap;
		}
	}
}

void ProjectManager::finalize()
{
	if (!data) {
		return;
	}
	if (Input::is_initialized() && Input::data &&
		Input::data->event_dispatch_function == &ProjectManager::handle_input_event) {
		Input::set_event_dispatch_function(data->prev_input_dispatch);
	}
	if (data->canvas_item_rid.is_valid()) {
		Renderer::free_rid(data->canvas_item_rid);
	}
	if (data->canvas_rid.is_valid()) {
		Renderer::free_rid(data->canvas_rid);
	}
	if (data->owns_viewport && data->viewport_rid.is_valid()) {
		Renderer::free_rid(data->viewport_rid);
	}
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
	data->modal_field_primary = "New Voltaire Project";
	data->modal_field_secondary = _get_default_projects_dir();
	data->modal_error_text = String();
	data->active_field = FIELD_MODAL_PRIMARY;
	data->modal_buttons.clear();
	data->modal_buttons.push_back(
		{"Cancel", Rect2(), &ProjectManager::_cancel_modal, nullptr, false, false, false});
	data->modal_buttons.push_back(
		{"Create", Rect2(), &ProjectManager::_confirm_modal, nullptr, false, false, false});
	data->modal_buttons.push_back({"Create & Edit", Rect2(),
		&ProjectManager::_confirm_modal_and_edit, nullptr, false, false, true});
	data->needs_redraw = true;
}

void ProjectManager::_import_project()
{
	if (!data) {
		return;
	}
	data->modal_mode = MODAL_IMPORT_PROJECT;
	data->modal_field_primary = _get_default_projects_dir();
	data->modal_field_secondary = String();
	data->modal_error_text = String();
	data->active_field = FIELD_MODAL_PRIMARY;
	data->modal_buttons.clear();
	data->modal_buttons.push_back(
		{"Cancel", Rect2(), &ProjectManager::_cancel_modal, nullptr, false, false, false});
	data->modal_buttons.push_back(
		{"Import", Rect2(), &ProjectManager::_confirm_modal, nullptr, false, false, true});
	data->needs_redraw = true;
}

void ProjectManager::_scan_projects()
{
	if (!data) {
		return;
	}
	data->modal_mode = MODAL_SCAN_PROJECTS;
	data->modal_field_primary = _get_default_projects_dir();
	data->modal_field_secondary = String();
	data->modal_error_text = String();
	data->active_field = FIELD_MODAL_PRIMARY;
	data->modal_buttons.clear();
	data->modal_buttons.push_back(
		{"Cancel", Rect2(), &ProjectManager::_cancel_modal, nullptr, false, false, false});
	data->modal_buttons.push_back(
		{"Scan Folder", Rect2(), &ProjectManager::_confirm_modal, nullptr, false, false, true});
	data->needs_redraw = true;
}

void ProjectManager::_open_donate_page()
{
	Error err = OS::shell_open("https://ko-fi.com/bendyland");
	if (data) {
		data->status_text = (err == OK) ? "Opened support page in browser."
										: "Support URL: https://ko-fi.com/bendyland";
		data->needs_redraw = true;
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
	data->needs_redraw = true;
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
	data->needs_redraw = true;
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
	data->needs_redraw = true;
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
	data->needs_redraw = true;
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
				data->projects[i].name = _read_project_name_from_dir(clean);
				return (int)i;
			}
		}
		ProjectEntry entry;
		entry.path = clean;
		entry.name = _read_project_name_from_dir(clean);
		entry.missing = !FileAccess::exists(clean.path_join("project.godot"));
		data->projects.push_back(entry);
		return (int)data->projects.size() - 1;
	};

	if (data->modal_mode == MODAL_NEW_PROJECT) {
		String proj_name = data->modal_field_primary.strip_edges();
		String parent_dir = data->modal_field_secondary.strip_edges().simplify_path();
		if (proj_name.is_empty()) {
			data->modal_error_text = "Project name cannot be empty.";
			data->needs_redraw = true;
			return;
		}
		if (parent_dir.is_empty()) {
			data->modal_error_text = "Parent directory cannot be empty.";
			data->needs_redraw = true;
			return;
		}
		String target_dir = parent_dir.path_join(proj_name);
		String proj_file = target_dir.path_join("project.godot");
		if (FileAccess::exists(proj_file)) {
			data->modal_error_text = "A project already exists at that path.";
			data->needs_redraw = true;
			return;
		}
		Error dir_err = DirAccess::make_dir_recursive_absolute(target_dir);
		if (dir_err != OK && !DirAccess::dir_exists_absolute(target_dir)) {
			data->modal_error_text = "Failed to create project directory.";
			data->needs_redraw = true;
			return;
		}
		Ref<FileAccess> fa = FileAccess::open(proj_file, FileAccess::WRITE);
		if (fa.is_null()) {
			data->modal_error_text = "Failed to write project.godot.";
			data->needs_redraw = true;
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
		String raw_path = data->modal_field_primary.strip_edges().simplify_path();
		if (raw_path.ends_with("/project.godot")) {
			raw_path = raw_path.get_base_dir();
		}
		if (raw_path.is_empty() || !FileAccess::exists(raw_path.path_join("project.godot"))) {
			data->modal_error_text = "No project.godot found in the specified directory.";
			data->needs_redraw = true;
			return;
		}
		int idx = register_project_dir(raw_path);
		save_projects();
		rebuild_filtered_list();
		data->selected_project_idx = idx;
		rebuild_filtered_list();
		data->status_text =
			vformat("Imported project '%s'.", data->projects[idx].name);
		_cancel_modal();
		return;
	}

	if (data->modal_mode == MODAL_SCAN_PROJECTS) {
		String scan_dir = data->modal_field_primary.strip_edges().simplify_path();
		if (scan_dir.is_empty() || !DirAccess::dir_exists_absolute(scan_dir)) {
			data->modal_error_text = "Directory does not exist.";
			data->needs_redraw = true;
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
