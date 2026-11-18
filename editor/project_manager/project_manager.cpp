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

#include "core/os/os.h"
#include "project_manager.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

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

	// 4. Register toolbar buttons (layout rects are computed in process_and_draw)
	data->buttons.push_back({"+ New Project", Rect2(), &ProjectManager::_new_project, false});
	data->buttons.push_back({"Import", Rect2(), &ProjectManager::_import_project, false});
	data->buttons.push_back({"Scan", Rect2(), &ProjectManager::_scan_projects, false});
	data->buttons.push_back({"Support", Rect2(), &ProjectManager::_open_donate_page, false});

	_initialized = true;
	data->needs_redraw = false;
	process_and_draw();
}

void ProjectManager::poll()
{
	if (!data) {
		return;
	}

	if (DisplayServer::is_initialized()) {
		Size2i ds_size =
			DisplayServer::window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
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
	const Color col_btn(0.20f, 0.24f, 0.30f, 1.0f);
	const Color col_btn_hover(0.28f, 0.35f, 0.45f, 1.0f);
	const Color col_accent(0.0f, 0.75f, 0.95f, 1.0f);
	const Color col_search_bg(0.06f, 0.07f, 0.09f, 1.0f);
	const Color col_workspace(0.12f, 0.14f, 0.17f, 1.0f);
	const Color col_text(0.92f, 0.94f, 0.96f, 1.0f);

	// 1. Full-window background
	Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), col_bg);

	const float content_w = MAX(0.0f, W - pad * 2.0f);
	if (content_w <= 0.0f || H <= 0.0f) {
		return;
	}

	// 2. Top Header Bar (y = 20, height = 48)
	float y = 20.0f;
	const float header_h = 48.0f;
	Renderer::canvas_item_add_rect(ci, Rect2(pad, y, content_w, header_h), col_header);
	// Left cyan accent strip on header
	Renderer::canvas_item_add_rect(ci, Rect2(pad, y, MIN(4.0f, content_w), header_h), col_accent);

	// 3. Toolbar Buttons aligned to the right of the header bar
	const uint32_t btn_count = data->buttons.size();
	float btn_w = 120.0f;
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
		if (title_max_w > 0.0f) {
			data->ui_font->draw_string(ci, Point2(pad + 18.0f, y + 30.0f),
				"VOLTAIRE // PROJECT MANAGER", HORIZONTAL_ALIGNMENT_LEFT, title_max_w, 16,
				col_text);
		}
	}

	for (uint32_t i = 0; i < btn_count; i++) {
		UIButton& btn = data->buttons[i];
		btn.rect = Rect2(btn_x, btn_y, btn_w, btn_h);
		if (btn_w > 0.0f) {
			Renderer::canvas_item_add_rect(ci, btn.rect, btn.hovered ? col_btn_hover : col_btn);
			// Bottom button border highlight
			Renderer::canvas_item_add_rect(
				ci, Rect2(btn_x, btn_y + btn_h - 2.0f, btn_w, 2.0f), col_accent);

			if (data->ui_font.is_valid() && btn_w > 24.0f) {
				data->ui_font->draw_string(ci, Point2(btn_x + 12.0f, btn_y + 22.0f), btn.label,
					HORIZONTAL_ALIGNMENT_LEFT, btn_w - 24.0f, 14, col_text);
			}
		}
		btn_x += btn_w + btn_gap;
	}

	y += header_h + 12.0f;

	// 4. Horizontal Separator (height = 2)
	Renderer::canvas_item_add_rect(
		ci, Rect2(pad, y, content_w, 2.0f), Color(0.22f, 0.25f, 0.30f, 1.0f));
	y += 14.0f;

	// 5. Search Bar Box (height = 38)
	const float search_h = 38.0f;
	Renderer::canvas_item_add_rect(ci, Rect2(pad, y, content_w, search_h), col_search_bg);
	if (data->ui_font.is_valid() && content_w > 24.0f) {
		String display_search =
			data->search_text.is_empty() ? "Filter projects..." : data->search_text;
		data->ui_font->draw_string(ci, Point2(pad + 12.0f, y + 24.0f), display_search,
			HORIZONTAL_ALIGNMENT_LEFT, content_w - 24.0f, 14, Color(0.55f, 0.60f, 0.68f, 1.0f));
	}
	y += search_h + 14.0f;

	// 6. Main Workspace Panel (fills remaining height)
	const float workspace_h = MAX(0.0f, H - y - pad);
	if (workspace_h > 0.0f) {
		Renderer::canvas_item_add_rect(ci, Rect2(pad, y, content_w, workspace_h), col_workspace);

		// Inner project card preview row inside workspace
		const float card_w = MAX(0.0f, content_w - 32.0f);
		const float card_h = MIN(64.0f, MAX(0.0f, workspace_h - 32.0f));
		if (card_w > 0.0f && card_h > 0.0f) {
			Renderer::canvas_item_add_rect(
				ci, Rect2(pad + 16.0f, y + 16.0f, card_w, card_h), Color(0.16f, 0.19f, 0.24f, 1.0f));
			Renderer::canvas_item_add_rect(
				ci, Rect2(pad + 16.0f, y + 16.0f, MIN(4.0f, card_w), card_h), col_accent);

			if (data->ui_font.is_valid() && card_w > 32.0f && card_h >= 40.0f) {
				data->ui_font->draw_string(ci, Point2(pad + 32.0f, y + 54.0f), data->status_text,
					HORIZONTAL_ALIGNMENT_LEFT, card_w - 32.0f, 15, col_text);
			}
		}
	}
}

void ProjectManager::finalize()
{
	if (!data) {
		return;
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
	if (data) {
		data->status_text = "Action: + New Project clicked";
		process_and_draw();
	}
}

void ProjectManager::_import_project()
{
	if (data) {
		data->status_text = "Action: Import Project clicked";
		process_and_draw();
	}
}

void ProjectManager::_scan_projects()
{
	if (data) {
		data->status_text = "Action: Scan Projects clicked";
		process_and_draw();
	}
}

void ProjectManager::_open_donate_page() { OS::shell_open("https://ko-fi.com/bendyland"); }
