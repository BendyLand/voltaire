/**************************************************************************/
/*  editor_ui.cpp                                                         */
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

#include "editor/editor_ui.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "editor/project_manager/project_manager.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

namespace
{
bool _project_3d(const Vector3& p_pt, float p_yaw, float p_pitch, const Rect2& p_vp,
	Point2& r_screen)
{
	const float cy = Math::cos(p_yaw);
	const float sy = Math::sin(p_yaw);
	const float cp = Math::cos(p_pitch);
	const float sp = Math::sin(p_pitch);

	// Orbit around origin at distance 9.0
	float x1 = p_pt.x * cy - p_pt.z * sy;
	float z1 = p_pt.x * sy + p_pt.z * cy;
	float y2 = p_pt.y * cp - z1 * sp;
	float z2 = p_pt.y * sp + z1 * cp + 9.0f;

	if (z2 <= 0.4f) {
		return false;
	}
	const float fov_scale = MIN(p_vp.size.x, p_vp.size.y) * 0.85f;
	r_screen.x = p_vp.position.x + p_vp.size.x * 0.5f + (x1 / z2) * fov_scale;
	r_screen.y = p_vp.position.y + p_vp.size.y * 0.5f - (y2 / z2) * fov_scale;
	return true;
}
} // namespace

bool EditorUI::_can_modify_selected_node()
{
	return data && data->selected_entity_idx >= 0 &&
		   data->selected_entity_idx < (int)data->entities.size();
}

bool EditorUI::_can_delete_selected_node()
{
	// Keep index 0 (Root) so scene always has a root node
	return data && data->selected_entity_idx > 0 &&
		   data->selected_entity_idx < (int)data->entities.size();
}

void EditorUI::add_log(const String& p_msg)
{
	if (!data) {
		return;
	}
	data->logs.push_back(p_msg);
	if (data->logs.size() > 64) {
		data->logs.remove_at(0);
	}
	data->ui.needs_redraw = true;
}

void EditorUI::scan_project_files()
{
	if (!data || data->project_path.is_empty()) {
		return;
	}
	data->files.clear();
	Ref<DirAccess> da = DirAccess::open(data->project_path);
	if (da.is_null()) {
		return;
	}

	LocalVector<FileEntry> dirs;
	LocalVector<FileEntry> regular;
	da->list_dir_begin();
	String item = da->get_next();
	while (!item.is_empty()) {
		if (item != "." && item != ".." && !item.begins_with(".")) {
			FileEntry fe;
			fe.name = item;
			fe.full_path = data->project_path.path_join(item);
			fe.is_dir = da->current_is_dir();
			if (fe.is_dir) {
				dirs.push_back(fe);
			}
			else {
				regular.push_back(fe);
			}
		}
		item = da->get_next();
	}
	da->list_dir_end();

	for (uint32_t i = 0; i < dirs.size(); i++) {
		data->files.push_back(dirs[i]);
	}
	for (uint32_t i = 0; i < regular.size(); i++) {
		data->files.push_back(regular[i]);
	}
}

void EditorUI::load_scene()
{
	if (!data) {
		return;
	}
	data->entities.clear();

	String scene_path = data->project_path.path_join("main.vscn");
	if (FileAccess::exists(scene_path)) {
		Ref<FileAccess> fa = FileAccess::open(scene_path, FileAccess::READ);
		if (fa.is_valid()) {
			while (!fa->eof_reached()) {
				String line = fa->get_line().strip_edges();
				if (line.is_empty() || line.begins_with("#") || line.begins_with(";")) {
					continue;
				}
				Vector<String> parts = line.split("|");
				if (parts.size() >= 9) {
					SceneEntity ent;
					ent.name = parts[0];
					ent.type = parts[1];
					ent.position = Vector2(parts[2].to_float(), parts[3].to_float());
					ent.size = Size2(MAX(16.0f, parts[4].to_float()), MAX(16.0f, parts[5].to_float()));
					ent.color =
						Color(parts[6].to_float(), parts[7].to_float(), parts[8].to_float(), 1.0f);
					ent.runtime_vel = Vector2(130.0f + 25.0f * (data->entities.size() % 3),
						95.0f + 20.0f * (data->entities.size() % 2));
					data->entities.push_back(ent);
				}
			}
		}
	}

	if (data->entities.is_empty()) {
		SceneEntity root;
		root.name = "MainScene";
		root.type = "Node2D";
		root.position = Vector2(0.0f, 0.0f);
		root.size = Size2(48.0f, 48.0f);
		root.color = Color(0.55f, 0.65f, 0.78f, 1.0f);
		data->entities.push_back(root);

		SceneEntity player;
		player.name = "Player";
		player.type = "Sprite2D";
		player.position = Vector2(220.0f, 180.0f);
		player.size = Size2(96.0f, 96.0f);
		player.color = Color(0.0f, 0.75f, 0.95f, 1.0f);
		player.runtime_vel = Vector2(170.0f, 125.0f);
		data->entities.push_back(player);

		SceneEntity platform;
		platform.name = "GroundPlatform";
		platform.type = "StaticBody2D";
		platform.position = Vector2(160.0f, 380.0f);
		platform.size = Size2(360.0f, 44.0f);
		platform.color = Color(0.22f, 0.78f, 0.55f, 1.0f);
		platform.runtime_vel = Vector2(-95.0f, 65.0f);
		data->entities.push_back(platform);
	}

	data->selected_entity_idx = data->entities.size() > 1 ? 1 : 0;
	data->next_entity_serial = (int)data->entities.size();
	data->scene_dirty = false;
}

void EditorUI::save_scene()
{
	if (!data || data->project_path.is_empty()) {
		return;
	}
	String scene_path = data->project_path.path_join("main.vscn");
	Ref<FileAccess> fa = FileAccess::open(scene_path, FileAccess::WRITE);
	if (fa.is_null()) {
		add_log("ERROR: Could not write scene to " + scene_path);
		return;
	}
	fa->store_line("# Voltaire Scene v1");
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		const SceneEntity& e = data->entities[i];
		fa->store_line(vformat("%s|%s|%.1f|%.1f|%.1f|%.1f|%.3f|%.3f|%.3f",
			e.name.replace("|", "_"), e.type.replace("|", "_"), e.position.x, e.position.y,
			e.size.x, e.size.y, e.color.r, e.color.g, e.color.b));
	}
	fa->close();
	data->scene_dirty = false;
	scan_project_files();
	add_log("Saved scene: res://main.vscn (" + itos(data->entities.size()) + " nodes)");
}

void EditorUI::open_text_file(const String& p_path)
{
	if (!data || p_path.is_empty() || !FileAccess::exists(p_path)) {
		return;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	if (fa.is_null()) {
		add_log("Could not open file: " + p_path.get_file());
		return;
	}
	data->script_lines.clear();
	while (!fa->eof_reached()) {
		data->script_lines.push_back(fa->get_line().replace("\t", "    "));
	}
	if (data->script_lines.is_empty()) {
		data->script_lines.push_back("");
	}
	data->open_file_path = p_path;
	data->open_file_name = p_path.get_file();
	data->caret_line = 0;
	data->script_scroll_line = 0;
	data->script_dirty = false;
	data->ui.needs_redraw = true;
}

void EditorUI::save_open_text_file()
{
	if (!data || data->open_file_path.is_empty()) {
		return;
	}
	Ref<FileAccess> fa = FileAccess::open(data->open_file_path, FileAccess::WRITE);
	if (fa.is_null()) {
		add_log("ERROR: Failed to save " + data->open_file_name);
		return;
	}
	for (int i = 0; i < data->script_lines.size(); i++) {
		fa->store_line(data->script_lines[i]);
	}
	fa->close();
	data->script_dirty = false;
	add_log("Saved file: res://" + data->open_file_name);
}

void EditorUI::initialize(RID p_viewport_rid, Size2i p_size, const String& p_project_path,
	const String& p_project_name, bool p_start_playing)
{
	if (data) {
		return;
	}
	data = memnew(Data);
	data->project_path = p_project_path.simplify_path();
	if (data->project_path.begins_with("res://")) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		if (da.is_valid()) {
			data->project_path = da->get_current_dir();
		}
	}
	data->project_name = p_project_name.is_empty() ? UIDraw::read_project_name(data->project_path)
												   : p_project_name;
	if (data->project_name.is_empty()) {
		data->project_name = "Voltaire Project";
	}

	data->ui.initialize(p_viewport_rid, p_size,
		Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT), &EditorUI::handle_input_event);

	data->inspector_name_field.placeholder = "Node";

	// Top toolbar buttons: [0]=< Project List, [1]=2D, [2]=3D, [3]=Script, [4]=+ Add Node, [5]=Save, [6]=Play
	data->top_buttons.add("< Project List", &EditorUI::_back_to_project_list);
	data->top_buttons.add("2D", &EditorUI::_switch_view_2d, nullptr, false, true);
	data->top_buttons.add("3D", &EditorUI::_switch_view_3d);
	data->top_buttons.add("Script", &EditorUI::_switch_view_script);
	data->top_buttons.add("+ Add Node", &EditorUI::_add_node);
	data->top_buttons.add("Save", &EditorUI::_save_all);
	data->top_buttons.add("Play", &EditorUI::_toggle_play, nullptr, true);

	// Dock & viewport utility buttons: [0]=+ Node, [1]=+ Script, [2]=Refresh, [3]=Reset View, [4]=Clear Output
	data->dock_buttons.add("+ Node", &EditorUI::_add_node);
	data->dock_buttons.add("+ Script", &EditorUI::_new_script_file);
	data->dock_buttons.add("Refresh", &EditorUI::_refresh_files);
	data->dock_buttons.add("Reset View", &EditorUI::_reset_camera_view);
	data->dock_buttons.add("Clear", &EditorUI::_clear_output);

	// Inspector buttons: [0..7]=pos/size nudges, [8]=Duplicate, [9]=Delete Node
	data->inspector_buttons.add("-10", &EditorUI::_nudge_pos_x_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("+10", &EditorUI::_nudge_pos_x_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("-10", &EditorUI::_nudge_pos_y_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("+10", &EditorUI::_nudge_pos_y_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("-10", &EditorUI::_nudge_size_w_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("+10", &EditorUI::_nudge_size_w_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("-10", &EditorUI::_nudge_size_h_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("+10", &EditorUI::_nudge_size_h_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add("Duplicate", &EditorUI::_duplicate_selected_node, &EditorUI::_can_delete_selected_node);
	data->inspector_buttons.add("Delete Node", &EditorUI::_delete_selected_node, &EditorUI::_can_delete_selected_node);

	// Inspector color swatches
	data->swatches.push_back({Color(0.0f, 0.75f, 0.95f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.95f, 0.40f, 0.42f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.95f, 0.75f, 0.22f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.22f, 0.80f, 0.55f, 1.0f), Rect2(), false});
	data->swatches.push_back({Color(0.68f, 0.48f, 0.96f, 1.0f), Rect2(), false});

	// Runtime preview toolbar buttons: [0]=Reset Sim, [1]=Open in Editor, [2]=Stop
	data->runtime_buttons.add("Reset Sim", &EditorUI::_reset_simulation);
	data->runtime_buttons.add("Open in Editor", &EditorUI::_stop_and_edit);
	data->runtime_buttons.add("Stop", &EditorUI::_toggle_play, nullptr, true);

	load_scene();
	scan_project_files();
	String proj_godot = data->project_path.path_join("project.godot");
	if (FileAccess::exists(proj_godot)) {
		open_text_file(proj_godot);
	}

	add_log(vformat("Opened project '%s' (%s)", data->project_name, data->project_path));

	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + (p_start_playing ? " (Running) - Voltaire" : " - Voltaire Editor"),
			DisplayServerEnums::MAIN_WINDOW_ID);
	}

	_initialized = true;
	if (p_start_playing) {
		data->launched_directly_to_play = true;
		start_play();
	}
	data->ui.needs_redraw = false;
	process_and_draw();
}

void EditorUI::start_play()
{
	if (!data) {
		return;
	}
	data->playing = true;
	data->play_time = 0.0;
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		data->entities[i].runtime_pos = data->entities[i].position;
		if (data->entities[i].runtime_vel.length_squared() < 1.0f) {
			data->entities[i].runtime_vel = Vector2(140.0f, 100.0f);
		}
	}
	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + " (Running) - Voltaire", DisplayServerEnums::MAIN_WINDOW_ID);
	}
	add_log("Started runtime preview.");
	data->ui.needs_redraw = true;
}

void EditorUI::stop_play()
{
	if (!data) {
		return;
	}
	if (data->launched_directly_to_play) {
		data->pending_return_to_pm = true;
		return;
	}
	data->playing = false;
	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + " - Voltaire Editor", DisplayServerEnums::MAIN_WINDOW_ID);
	}
	add_log("Stopped runtime preview.");
	data->ui.needs_redraw = true;
}

void EditorUI::handle_input_event(const Ref<InputEvent>& p_event)
{
	if (!data || p_event.is_null()) {
		return;
	}
	data->ui.forward_input(p_event, &EditorUI::handle_input_event);

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		Point2 prev_mouse = data->ui.mouse_pos;
		data->ui.mouse_pos = mm->get_position();
		Vector2 delta = data->ui.mouse_pos - prev_mouse;

		if (data->playing) {
			if (data->runtime_buttons.update_hover(data->ui.mouse_pos)) {
				data->ui.needs_redraw = true;
			}
			return;
		}

		if (data->dragging_node_2d && _can_modify_selected_node()) {
			Vector2 vp_origin =
				data->center_viewport_rect.position + Vector2(24.0f, 24.0f) + data->pan_2d;
			SceneEntity& ent = data->entities[data->selected_entity_idx];
			ent.position = (data->ui.mouse_pos - vp_origin - data->drag_node_offset).round();
			data->scene_dirty = true;
			data->ui.needs_redraw = true;
			return;
		}

		if (data->panning_viewport) {
			if (data->view_mode == VIEW_2D) {
				data->pan_2d += delta;
				data->ui.needs_redraw = true;
			}
			else if (data->view_mode == VIEW_3D) {
				data->orbit_yaw += delta.x * 0.01f;
				data->orbit_pitch = CLAMP(data->orbit_pitch + delta.y * 0.01f, -1.35f, 1.35f);
				data->ui.needs_redraw = true;
			}
			return;
		}

		bool changed = false;
		changed |= data->top_buttons.update_hover(data->ui.mouse_pos);
		changed |= data->dock_buttons.update_hover(data->ui.mouse_pos);
		changed |= data->inspector_buttons.update_hover(data->ui.mouse_pos);

		for (uint32_t i = 0; i < data->swatches.size(); i++) {
			bool h = data->swatches[i].rect.has_point(data->ui.mouse_pos);
			if (data->swatches[i].hovered != h) {
				data->swatches[i].hovered = h;
				changed = true;
			}
		}
		for (uint32_t i = 0; i < data->entities.size(); i++) {
			bool h = data->entities[i].tree_row_rect.has_point(data->ui.mouse_pos);
			if (data->entities[i].tree_hovered != h) {
				data->entities[i].tree_hovered = h;
				changed = true;
			}
		}
		for (uint32_t i = 0; i < data->files.size(); i++) {
			bool h = data->files[i].row_rect.has_point(data->ui.mouse_pos);
			if (data->files[i].hovered != h) {
				data->files[i].hovered = h;
				changed = true;
			}
		}
		if (changed) {
			data->ui.needs_redraw = true;
		}
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		data->ui.mouse_pos = mb->get_position();

		// Scroll wheel in Script view
		if (mb->is_pressed() && !data->playing && data->view_mode == VIEW_SCRIPT &&
			data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
			if (mb->get_button_index() == MouseButton::WHEEL_UP && data->script_scroll_line > 0) {
				data->script_scroll_line = MAX(0, data->script_scroll_line - 3);
				data->ui.needs_redraw = true;
				return;
			}
			if (mb->get_button_index() == MouseButton::WHEEL_DOWN) {
				int max_sc = MAX(0, (int)data->script_lines.size() - 5);
				data->script_scroll_line = MIN(max_sc, data->script_scroll_line + 3);
				data->ui.needs_redraw = true;
				return;
			}
		}

		// Right or middle mouse drag pans 2D / orbits 3D
		if (mb->get_button_index() == MouseButton::RIGHT ||
			mb->get_button_index() == MouseButton::MIDDLE) {
			if (!data->playing && mb->is_pressed() &&
				data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
				data->panning_viewport = true;
			}
			else if (!mb->is_pressed()) {
				data->panning_viewport = false;
			}
			return;
		}

		if (mb->get_button_index() != MouseButton::LEFT) {
			return;
		}

		if (data->playing) {
			if (mb->is_pressed()) {
				bool hit_btn = data->runtime_buttons.press(data->ui.mouse_pos);
				// Clicking in the runtime arena gives moving entities an interactive bounce impulse
				if (!hit_btn && data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
					for (uint32_t i = 1; i < data->entities.size(); i++) {
						data->entities[i].runtime_vel = Vector2(
							-data->entities[i].runtime_vel.y * 1.05f,
							data->entities[i].runtime_vel.x * 1.05f);
					}
				}
				data->ui.needs_redraw = true;
			}
			else {
				void (*to_call)() = data->runtime_buttons.release(data->ui.mouse_pos);
				data->ui.needs_redraw = true;
				if (to_call) {
					to_call();
				}
			}
			return;
		}

		// Editor Mode Left Click
		if (mb->is_pressed()) {
			bool hit_btn = false;
			hit_btn |= data->top_buttons.press(data->ui.mouse_pos);
			hit_btn |= data->dock_buttons.press(data->ui.mouse_pos);
			hit_btn |= data->inspector_buttons.press(data->ui.mouse_pos);
			if (hit_btn) {
				data->ui.needs_redraw = true;
				return;
			}

			// Inspector node name box focus
			if (data->inspector_name_field.has_point(data->ui.mouse_pos) &&
				_can_modify_selected_node()) {
				data->focus_field = FOCUS_NODE_NAME;
				data->ui.needs_redraw = true;
				return;
			}

			// Color swatches
			if (_can_modify_selected_node()) {
				for (uint32_t i = 0; i < data->swatches.size(); i++) {
					if (data->swatches[i].rect.has_point(data->ui.mouse_pos)) {
						data->entities[data->selected_entity_idx].color = data->swatches[i].color;
						data->scene_dirty = true;
						data->ui.needs_redraw = true;
						return;
					}
				}
			}

			// Scene Tree rows
			for (uint32_t i = 0; i < data->entities.size(); i++) {
				if (data->entities[i].tree_row_rect.has_point(data->ui.mouse_pos)) {
					data->selected_entity_idx = (int)i;
					data->focus_field = FOCUS_NONE;
					data->ui.needs_redraw = true;
					return;
				}
			}

			// FileSystem rows
			for (uint32_t i = 0; i < data->files.size(); i++) {
				if (data->files[i].row_rect.has_point(data->ui.mouse_pos)) {
					data->selected_file_idx = (int)i;
					if (!data->files[i].is_dir) {
						open_text_file(data->files[i].full_path);
						_switch_view_script();
						add_log("Opened res://" + data->files[i].name);
					}
					else {
						add_log("Folder: res://" + data->files[i].name + "/");
					}
					data->ui.needs_redraw = true;
					return;
				}
			}

			// Center Viewport click
			if (data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
				if (data->view_mode == VIEW_SCRIPT) {
					data->focus_field = FOCUS_SCRIPT_EDITOR;
					int clicked_row = data->script_scroll_line +
									  (int)((data->ui.mouse_pos.y -
												(data->center_viewport_rect.position.y + 38.0f)) /
											22.0f);
					if (!data->script_lines.is_empty()) {
						data->caret_line =
							CLAMP(clicked_row, 0, (int)data->script_lines.size() - 1);
					}
					data->ui.needs_redraw = true;
					return;
				}
				if (data->view_mode == VIEW_2D) {
					data->focus_field = FOCUS_NONE;
					Vector2 vp_origin =
						data->center_viewport_rect.position + Vector2(24.0f, 24.0f) + data->pan_2d;
					// Hit-test entities front-to-back (reverse order)
					for (int i = (int)data->entities.size() - 1; i >= 1; i--) {
						Rect2 world_r(
							vp_origin + data->entities[i].position, data->entities[i].size);
						if (world_r.has_point(data->ui.mouse_pos)) {
							data->selected_entity_idx = i;
							data->dragging_node_2d = true;
							data->drag_node_offset = data->ui.mouse_pos - world_r.position;
							data->ui.needs_redraw = true;
							return;
						}
					}
					// Empty space in 2D viewport pans
					data->panning_viewport = true;
					data->ui.needs_redraw = true;
					return;
				}
				if (data->view_mode == VIEW_3D) {
					data->focus_field = FOCUS_NONE;
					data->panning_viewport = true;
					data->ui.needs_redraw = true;
					return;
				}
			}

			data->focus_field = FOCUS_NONE;
			data->ui.needs_redraw = true;
		}
		else {
			data->dragging_node_2d = false;
			data->panning_viewport = false;

			void (*c0)() = data->top_buttons.release(data->ui.mouse_pos);
			void (*c1)() = data->dock_buttons.release(data->ui.mouse_pos);
			void (*c2)() = data->inspector_buttons.release(data->ui.mouse_pos);
			void (*to_call)() = c2 ? c2 : (c1 ? c1 : c0);

			data->ui.needs_redraw = true;
			if (to_call) {
				to_call();
			}
		}
		return;
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed()) {
		const Key code = k->get_keycode();

		if (code == Key::F5 && !data->playing) {
			start_play();
			return;
		}
		if ((code == Key::F8 || code == Key::ESCAPE) && data->playing) {
			stop_play();
			return;
		}
		if (k->is_ctrl_pressed() && code == Key::S && !data->playing) {
			_save_all();
			return;
		}
		if (code == Key::ESCAPE) {
			data->focus_field = FOCUS_NONE;
			data->ui.needs_redraw = true;
			return;
		}

		// Inspector Node Name editing via composed UITextField
		if (data->focus_field == FOCUS_NODE_NAME && _can_modify_selected_node()) {
			String& nm = data->entities[data->selected_entity_idx].name;
			if (code == Key::ENTER || code == Key::KP_ENTER) {
				if (nm.strip_edges().is_empty()) {
					nm = "Node";
				}
				data->focus_field = FOCUS_NONE;
				data->ui.needs_redraw = true;
				return;
			}
			data->inspector_name_field.text = nm;
			if (data->inspector_name_field.handle_key(k, '|')) {
				nm = data->inspector_name_field.text;
				data->scene_dirty = true;
				data->ui.needs_redraw = true;
			}
			return;
		}

		// Script Editor keyboard editing
		if (data->focus_field == FOCUS_SCRIPT_EDITOR && data->view_mode == VIEW_SCRIPT &&
			!data->script_lines.is_empty()) {
			data->caret_line = CLAMP(data->caret_line, 0, (int)data->script_lines.size() - 1);
			if (code == Key::UP && data->caret_line > 0) {
				data->caret_line--;
				if (data->caret_line < data->script_scroll_line) {
					data->script_scroll_line = data->caret_line;
				}
				data->ui.needs_redraw = true;
				return;
			}
			if (code == Key::DOWN && data->caret_line + 1 < data->script_lines.size()) {
				data->caret_line++;
				data->ui.needs_redraw = true;
				return;
			}
			if (code == Key::ENTER || code == Key::KP_ENTER) {
				data->script_lines.insert(data->caret_line + 1, "");
				data->caret_line++;
				data->script_dirty = true;
				data->ui.needs_redraw = true;
				return;
			}
			if (code == Key::BACKSPACE) {
				String cur = data->script_lines[data->caret_line];
				if (!cur.is_empty()) {
					data->script_lines.write[data->caret_line] = cur.substr(0, cur.length() - 1);
				}
				else if (data->caret_line > 0) {
					data->script_lines.remove_at(data->caret_line);
					data->caret_line--;
				}
				data->script_dirty = true;
				data->ui.needs_redraw = true;
				return;
			}
			if (code == Key::TAB) {
				data->script_lines.write[data->caret_line] += "    ";
				data->script_dirty = true;
				data->ui.needs_redraw = true;
				return;
			}
			char32_t ch = k->get_unicode();
			if (ch >= 32 && ch != 127 && !k->is_ctrl_pressed() && !k->is_meta_pressed()) {
				data->script_lines.write[data->caret_line] += String::chr(ch);
				data->script_dirty = true;
				data->ui.needs_redraw = true;
				return;
			}
		}

		// Arrow-key nudging in 2D view when no text field is focused
		if (data->focus_field == FOCUS_NONE && data->view_mode == VIEW_2D &&
			_can_modify_selected_node()) {
			float step = k->is_shift_pressed() ? 10.0f : 2.0f;
			SceneEntity& ent = data->entities[data->selected_entity_idx];
			if (code == Key::LEFT) {
				ent.position.x -= step;
				data->scene_dirty = true;
				data->ui.needs_redraw = true;
			}
			else if (code == Key::RIGHT) {
				ent.position.x += step;
				data->scene_dirty = true;
				data->ui.needs_redraw = true;
			}
			else if (code == Key::UP) {
				ent.position.y -= step;
				data->scene_dirty = true;
				data->ui.needs_redraw = true;
			}
			else if (code == Key::DOWN) {
				ent.position.y += step;
				data->scene_dirty = true;
				data->ui.needs_redraw = true;
			}
			else if (code == Key::KEY_DELETE && _can_delete_selected_node()) {
				_delete_selected_node();
			}
		}
	}
}

void EditorUI::poll(double p_delta)
{
	if (!data) {
		return;
	}

	if (Input::is_initialized()) {
		Input::flush_buffered_events();
	}

	if (data->pending_return_to_pm) {
		Size2i win_size = data->ui.window_size;
		String last_proj = data->project_path;
		finalize();
		if (DisplayServer::is_initialized()) {
			DisplayServer::window_set_title(
				"Voltaire - Project Manager", DisplayServerEnums::MAIN_WINDOW_ID);
		}
		ProjectManager::initialize(RID(), win_size);
		if (ProjectManager::data && !last_proj.is_empty()) {
			for (uint32_t i = 0; i < ProjectManager::data->projects.size(); i++) {
				if (ProjectManager::data->projects[i].path == last_proj) {
					ProjectManager::data->selected_project_idx = (int)i;
					ProjectManager::rebuild_filtered_list();
					ProjectManager::data->ui.needs_redraw = true;
					break;
				}
			}
		}
		return;
	}

	data->ui.sync_window_size();

	if (data->playing) {
		double dt = CLAMP(p_delta, 0.001, 0.05);
		data->play_time += dt;
		data->fps_accum += dt;
		data->fps_frames++;
		if (data->fps_accum >= 0.5) {
			data->display_fps = MAX(1, (int)Math::round(data->fps_frames / data->fps_accum));
			data->fps_accum = 0.0;
			data->fps_frames = 0;
		}

		const float arena_w = MAX(160.0f, data->center_viewport_rect.size.x - 32.0f);
		const float arena_h = MAX(120.0f, data->center_viewport_rect.size.y - 32.0f);
		for (uint32_t i = 1; i < data->entities.size(); i++) {
			SceneEntity& e = data->entities[i];
			e.runtime_pos += e.runtime_vel * (float)dt;
			if (e.runtime_pos.x < 0.0f) {
				e.runtime_pos.x = 0.0f;
				e.runtime_vel.x = Math::abs(e.runtime_vel.x);
			}
			else if (e.runtime_pos.x + e.size.x > arena_w) {
				e.runtime_pos.x = MAX(0.0f, arena_w - e.size.x);
				e.runtime_vel.x = -Math::abs(e.runtime_vel.x);
			}
			if (e.runtime_pos.y < 0.0f) {
				e.runtime_pos.y = 0.0f;
				e.runtime_vel.y = Math::abs(e.runtime_vel.y);
			}
			else if (e.runtime_pos.y + e.size.y > arena_h) {
				e.runtime_pos.y = MAX(0.0f, arena_h - e.size.y);
				e.runtime_vel.y = -Math::abs(e.runtime_vel.y);
			}
		}
		data->ui.needs_redraw = true;
	}

	if (data->ui.needs_redraw) {
		data->ui.needs_redraw = false;
		process_and_draw();
	}
}

void EditorUI::process_and_draw()
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
	if (W <= 8.0f || H <= 8.0f) {
		return;
	}

	const Ref<Font>& font = data->ui.font;
	const Color col_bg(0.08f, 0.09f, 0.11f, 1.0f);
	const Color col_header(0.12f, 0.14f, 0.17f, 1.0f);
	const Color col_dock(0.11f, 0.13f, 0.16f, 1.0f);
	const Color col_dock_hdr(0.14f, 0.17f, 0.21f, 1.0f);
	const Color col_vp_bg(0.06f, 0.07f, 0.09f, 1.0f);
	const Color col_accent(0.0f, 0.75f, 0.95f, 1.0f);
	const Color col_text(0.92f, 0.94f, 0.96f, 1.0f);
	const Color col_subtext(0.56f, 0.62f, 0.70f, 1.0f);
	const Color col_border(0.19f, 0.23f, 0.28f, 1.0f);

	Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), col_bg);

	const float pad = 12.0f;
	const float content_w = MAX(0.0f, W - pad * 2.0f);

	// =========================================================================
	// RUNTIME PREVIEW MODE
	// =========================================================================
	if (data->playing) {
		const float bar_h = 46.0f;
		Rect2 bar_rect(pad, pad, content_w, bar_h);
		UIDraw::draw_header_banner(
			ci, bar_rect, col_header, Color(0.22f, 0.85f, 0.52f, 1.0f), 4.0f);

		// Right-aligned runtime control buttons
		float rx = W - pad - 10.0f;
		for (int i = (int)data->runtime_buttons.size() - 1; i >= 0; i--) {
			UIButton& rb = data->runtime_buttons[i];
			if (rb.on_click == &EditorUI::_stop_and_edit && !data->launched_directly_to_play) {
				rb.rect = Rect2();
				continue;
			}
			float bw = (rb.on_click == &EditorUI::_stop_and_edit) ? 124.0f : 96.0f;
			rx -= bw;
			rb.rect = Rect2(rx, pad + (bar_h - 32.0f) * 0.5f, bw, 32.0f);
			UIDraw::draw_button(ci, font, rb, 13);
			rx -= 8.0f;
		}

		if (font.is_valid()) {
			float avail = MAX(0.0f, rx - (pad + 20.0f));
			if (avail > 30.0f) {
				String run_title = vformat("RUNNING // %s   |   %d FPS   |   %.1fs",
					data->project_name, data->display_fps, data->play_time);
				font->draw_string(ci,
					Point2(pad + 16.0f, UIDraw::centered_baseline_y(font, bar_rect, 15)), run_title,
					HORIZONTAL_ALIGNMENT_LEFT, avail, 15, col_text);
			}
		}

		float vp_y = pad + bar_h + 10.0f;
		float vp_h = MAX(0.0f, H - vp_y - pad);
		data->center_viewport_rect = Rect2(pad, vp_y, content_w, vp_h);
		UIDraw::draw_bordered_rect(ci, data->center_viewport_rect, col_vp_bg, col_border, 1.0f);

		Vector2 arena_origin = data->center_viewport_rect.position + Vector2(16.0f, 16.0f);
		for (uint32_t i = 1; i < data->entities.size(); i++) {
			const SceneEntity& e = data->entities[i];
			Rect2 r = Rect2(arena_origin + e.runtime_pos, e.size)
						  .intersection(data->center_viewport_rect);
			if (r.size.x > 1.0f && r.size.y > 1.0f) {
				Renderer::canvas_item_add_rect(ci, r, e.color);
				if (font.is_valid() && r.size.x > 24.0f && r.size.y >= 20.0f) {
					font->draw_string(ci,
						Point2(r.position.x + 6.0f, UIDraw::centered_baseline_y(font, r, 13)),
						e.name, HORIZONTAL_ALIGNMENT_CENTER, r.size.x - 12.0f, 13,
						Color(0.06f, 0.08f, 0.10f, 1.0f));
				}
			}
		}

		if (font.is_valid() && vp_h > 40.0f && content_w > 80.0f) {
			font->draw_string(ci, Point2(pad + 16.0f, vp_y + vp_h - 14.0f),
				"Click inside viewport to impulse entities  |  Press Esc or Stop to exit runtime",
				HORIZONTAL_ALIGNMENT_LEFT, content_w - 32.0f, 12, col_subtext);
		}
		return;
	}

	// =========================================================================
	// EDITOR WORKSPACE MODE
	// =========================================================================
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		data->entities[i].tree_row_rect = Rect2();
	}
	for (uint32_t i = 0; i < data->files.size(); i++) {
		data->files[i].row_rect = Rect2();
	}
	data->dock_buttons.clear_rects();
	data->inspector_buttons.clear_rects();
	for (uint32_t i = 0; i < data->swatches.size(); i++) {
		data->swatches[i].rect = Rect2();
	}
	data->inspector_name_field.rect = Rect2();

	// 1. Top Editor Bar
	const float top_h = 46.0f;
	Rect2 top_rect(pad, pad, content_w, top_h);
	UIDraw::draw_header_banner(ci, top_rect, col_header, col_accent, 4.0f);

	data->top_buttons[1].active_tab = (data->view_mode == VIEW_2D);
	data->top_buttons[2].active_tab = (data->view_mode == VIEW_3D);
	data->top_buttons[3].active_tab = (data->view_mode == VIEW_SCRIPT);

	const float tbtn_h = 32.0f;
	const float tbtn_y = pad + (top_h - tbtn_h) * 0.5f;

	// Left '< Project List' button
	float left_cur = pad + 10.0f;
	const float back_w = MIN(118.0f, MAX(0.0f, content_w * 0.22f));
	data->top_buttons[0].rect = Rect2(left_cur, tbtn_y, back_w, tbtn_h);
	UIDraw::draw_button(ci, font, data->top_buttons[0], 13);
	left_cur += back_w + 14.0f;

	// Right action buttons: [+ Add Node] [Save] [Play]
	float right_cur = W - pad - 10.0f;
	const float right_widths[3] = {100.0f, 76.0f, 84.0f};
	for (int idx = 6; idx >= 4; idx--) {
		float bw = right_widths[idx - 4];
		if (right_cur - bw > left_cur + 40.0f) {
			right_cur -= bw;
			data->top_buttons[idx].rect = Rect2(right_cur, tbtn_y, bw, tbtn_h);
			UIDraw::draw_button(ci, font, data->top_buttons[idx], 13);
			right_cur -= 8.0f;
		}
		else {
			data->top_buttons[idx].rect = Rect2();
		}
	}

	// Center workspace switcher tabs: [2D] [3D] [Script]
	const float tab_w = 64.0f;
	const float tab_gap = 6.0f;
	const float tabs_total = tab_w * 3.0f + tab_gap * 2.0f;
	float tabs_x = Math::floor((W - tabs_total) * 0.5f);
	if (tabs_x < left_cur + 8.0f) {
		tabs_x = left_cur + 8.0f;
	}
	if (tabs_x + tabs_total <= right_cur) {
		for (int t = 0; t < 3; t++) {
			data->top_buttons[1 + t].rect =
				Rect2(tabs_x + t * (tab_w + tab_gap), tbtn_y, tab_w, tbtn_h);
			UIDraw::draw_button(ci, font, data->top_buttons[1 + t], 13);
		}
	}
	else {
		tabs_x = right_cur;
		for (int t = 0; t < 3; t++) {
			data->top_buttons[1 + t].rect = Rect2();
		}
	}

	// Project Title between '< Project List' and center tabs
	if (font.is_valid()) {
		float title_w = MAX(0.0f, tabs_x - left_cur - 12.0f);
		if (title_w > 24.0f) {
			String dirty_mark = (data->scene_dirty || data->script_dirty) ? " *" : "";
			String title = "VOLTAIRE // " + data->project_name + dirty_mark;
			font->draw_string(ci,
				Point2(left_cur, UIDraw::centered_baseline_y(font, top_rect, 15)), title,
				HORIZONTAL_ALIGNMENT_LEFT, title_w, 15, col_text);
		}
	}

	// 2. Main Body + Bottom Output Layout via composed UISplitLayout
	const float body_y = pad + top_h + 10.0f;
	const float avail_h = MAX(0.0f, H - body_y - pad);
	if (avail_h <= 20.0f) {
		return;
	}

	data->layout.compute_editor_workspace(Rect2(pad, body_y, content_w, avail_h), 10.0f);
	const Rect2 left_r = data->layout.left_rect;
	const Rect2 center_r = data->layout.center_rect;
	const Rect2 right_r = data->layout.right_rect;
	const Rect2 bottom_r = data->layout.bottom_rect;

	// 3. Left Column: SCENE Dock (top) + FILESYSTEM Dock (bottom)
	if (data->layout.show_left && left_r.size.y > 40.0f) {
		const float split_gap = 10.0f;
		const float scene_h = Math::floor((left_r.size.y - split_gap) * 0.52f);
		const float fs_h = MAX(0.0f, left_r.size.y - scene_h - split_gap);

		// SCENE Dock
		Rect2 scene_rect(left_r.position.x, left_r.position.y, left_r.size.x, scene_h);
		UIDraw::draw_dock_panel(
			ci, font, scene_rect, "SCENE", col_dock, col_dock_hdr, col_subtext, 30.0f, 66.0f);
		data->dock_buttons[0].rect =
			Rect2(scene_rect.position.x + scene_rect.size.x - 62.0f, scene_rect.position.y + 4.0f,
				56.0f, 22.0f);
		UIDraw::draw_button(ci, font, data->dock_buttons[0], 11);

		float ry = scene_rect.position.y + 36.0f;
		const float row_h = 26.0f;
		for (uint32_t i = 0; i < data->entities.size(); i++) {
			if (ry + row_h > scene_rect.position.y + scene_h - 6.0f) {
				break;
			}
			SceneEntity& ent = data->entities[i];
			const float indent = (i == 0) ? 8.0f : 22.0f;
			Rect2 rrect(scene_rect.position.x + 6.0f, ry, scene_rect.size.x - 12.0f, row_h);
			ent.tree_row_rect = rrect;

			const bool is_sel = ((int)i == data->selected_entity_idx);
			if (is_sel || ent.tree_hovered) {
				Renderer::canvas_item_add_rect(ci, rrect,
					is_sel ? Color(0.18f, 0.28f, 0.38f, 1.0f) : Color(0.16f, 0.20f, 0.26f, 1.0f));
			}
			Renderer::canvas_item_add_rect(
				ci, Rect2(scene_rect.position.x + indent, ry + 7.0f, 10.0f, 12.0f), ent.color);
			if (font.is_valid() && scene_rect.size.x > indent + 28.0f) {
				String row_lbl = ent.name + "  (" + ent.type + ")";
				font->draw_string(ci,
					Point2(scene_rect.position.x + indent + 16.0f,
						UIDraw::centered_baseline_y(font, rrect, 12)),
					row_lbl, HORIZONTAL_ALIGNMENT_LEFT, scene_rect.size.x - indent - 26.0f, 12,
					is_sel ? col_text : Color(0.80f, 0.84f, 0.90f, 1.0f));
			}
			ry += row_h + 2.0f;
		}

		// FILESYSTEM Dock
		const float fs_y = scene_rect.position.y + scene_h + split_gap;
		if (fs_h > 34.0f) {
			Rect2 fs_rect(left_r.position.x, fs_y, left_r.size.x, fs_h);
			UIDraw::draw_dock_panel(ci, font, fs_rect, "FILESYSTEM", col_dock, col_dock_hdr,
				col_subtext, 30.0f, 124.0f);
			data->dock_buttons[1].rect =
				Rect2(fs_rect.position.x + fs_rect.size.x - 122.0f, fs_y + 4.0f, 58.0f, 22.0f);
			data->dock_buttons[2].rect =
				Rect2(fs_rect.position.x + fs_rect.size.x - 60.0f, fs_y + 4.0f, 54.0f, 22.0f);
			UIDraw::draw_button(ci, font, data->dock_buttons[1], 11);
			UIDraw::draw_button(ci, font, data->dock_buttons[2], 11);

			float fy = fs_y + 36.0f;
			for (uint32_t i = 0; i < data->files.size(); i++) {
				if (fy + 24.0f > fs_y + fs_h - 6.0f) {
					break;
				}
				FileEntry& fe = data->files[i];
				Rect2 frect(fs_rect.position.x + 6.0f, fy, fs_rect.size.x - 12.0f, 24.0f);
				fe.row_rect = frect;
				const bool fsel = ((int)i == data->selected_file_idx) ||
								  (!fe.is_dir && fe.full_path == data->open_file_path);
				if (fsel || fe.hovered) {
					Renderer::canvas_item_add_rect(ci, frect,
						fsel ? Color(0.17f, 0.26f, 0.35f, 1.0f)
							 : Color(0.15f, 0.19f, 0.24f, 1.0f));
				}
				if (font.is_valid() && fs_rect.size.x > 28.0f) {
					String disp = fe.is_dir ? ("[DIR] " + fe.name) : fe.name;
					font->draw_string(ci,
						Point2(fs_rect.position.x + 12.0f,
							UIDraw::centered_baseline_y(font, frect, 12)),
						disp, HORIZONTAL_ALIGNMENT_LEFT, fs_rect.size.x - 24.0f, 12,
						fe.is_dir ? col_accent : col_text);
				}
				fy += 25.0f;
			}
		}
	}

	// 4. Center Main Viewport (2D / 3D / Script)
	data->center_viewport_rect = center_r;
	UIDraw::draw_bordered_rect(ci, data->center_viewport_rect, col_vp_bg, col_border, 1.0f);

	if (center_r.size.x > 40.0f && center_r.size.y > 40.0f) {
		const Rect2 vp = data->center_viewport_rect;

		if (data->view_mode == VIEW_2D) {
			const Vector2 origin = vp.position + Vector2(24.0f, 24.0f) + data->pan_2d;
			const float step = 48.0f;
			const Color grid_col(0.11f, 0.13f, 0.16f, 1.0f);

			float start_gx = vp.position.x + Math::fposmod(origin.x - vp.position.x, step);
			for (float gx = start_gx; gx < vp.position.x + vp.size.x; gx += step) {
				UIDraw::draw_clipped_line(ci, vp, Point2(gx, vp.position.y + 1.0f),
					Point2(gx, vp.position.y + vp.size.y - 1.0f), grid_col, 1.0f);
			}
			float start_gy = vp.position.y + Math::fposmod(origin.y - vp.position.y, step);
			for (float gy = start_gy; gy < vp.position.y + vp.size.y; gy += step) {
				UIDraw::draw_clipped_line(ci, vp, Point2(vp.position.x + 1.0f, gy),
					Point2(vp.position.x + vp.size.x - 1.0f, gy), grid_col, 1.0f);
			}

			// X & Y Origin Axes
			UIDraw::draw_clipped_line(ci, vp, Point2(vp.position.x + 1.0f, origin.y),
				Point2(vp.position.x + vp.size.x - 1.0f, origin.y),
				Color(0.55f, 0.22f, 0.26f, 1.0f), 1.5f);
			UIDraw::draw_clipped_line(ci, vp, Point2(origin.x, vp.position.y + 1.0f),
				Point2(origin.x, vp.position.y + vp.size.y - 1.0f),
				Color(0.20f, 0.52f, 0.34f, 1.0f), 1.5f);

			// Reference camera frame (640x360 preview frame)
			const Vector2 cam_sz(640.0f, 360.0f);
			const Color cam_col(0.28f, 0.36f, 0.52f, 1.0f);
			UIDraw::draw_clipped_line(
				ci, vp, origin, origin + Vector2(cam_sz.x, 0.0f), cam_col, 1.0f);
			UIDraw::draw_clipped_line(
				ci, vp, origin + Vector2(cam_sz.x, 0.0f), origin + cam_sz, cam_col, 1.0f);
			UIDraw::draw_clipped_line(
				ci, vp, origin + cam_sz, origin + Vector2(0.0f, cam_sz.y), cam_col, 1.0f);
			UIDraw::draw_clipped_line(
				ci, vp, origin + Vector2(0.0f, cam_sz.y), origin, cam_col, 1.0f);

			// Draw 2D scene entities
			for (uint32_t i = 1; i < data->entities.size(); i++) {
				const SceneEntity& e = data->entities[i];
				Rect2 world_r(origin + e.position, e.size);
				Rect2 clipped = world_r.intersection(vp);
				if (clipped.size.x > 1.0f && clipped.size.y > 1.0f) {
					const bool is_sel = ((int)i == data->selected_entity_idx);
					UIDraw::draw_bordered_rect(ci, clipped, e.color,
						is_sel ? Color(1.0f, 1.0f, 1.0f, 1.0f) : Color(0.05f, 0.07f, 0.09f, 0.8f),
						is_sel ? 2.0f : 1.0f);
					if (font.is_valid() && clipped.size.x > 24.0f && clipped.size.y >= 18.0f) {
						font->draw_string(ci,
							Point2(clipped.position.x + 4.0f,
								UIDraw::centered_baseline_y(font, clipped, 12)),
							e.name, HORIZONTAL_ALIGNMENT_CENTER, clipped.size.x - 8.0f, 12,
							Color(0.06f, 0.08f, 0.10f, 1.0f));
					}
				}
			}

			// Overlay badge & Reset View button
			data->dock_buttons[3].label = "Reset View";
			data->dock_buttons[3].rect =
				Rect2(vp.position.x + vp.size.x - 92.0f, vp.position.y + 8.0f, 84.0f, 26.0f);
			UIDraw::draw_button(ci, font, data->dock_buttons[3], 11);
			if (font.is_valid() && vp.size.x > 160.0f) {
				font->draw_string(ci, Point2(vp.position.x + 12.0f, vp.position.y + 24.0f),
					"2D VIEWPORT  —  Drag nodes to move, Right-drag to pan",
					HORIZONTAL_ALIGNMENT_LEFT, vp.size.x - 120.0f, 12, col_subtext);
			}
		}
		else if (data->view_mode == VIEW_3D) {
			// Ground Grid (-5..5 on XZ plane)
			const Color gcol(0.14f, 0.17f, 0.22f, 1.0f);
			for (int g = -5; g <= 5; g++) {
				Point2 a, b;
				if (_project_3d(
						Vector3((float)g, 0.0f, -5.0f), data->orbit_yaw, data->orbit_pitch, vp, a) &&
					_project_3d(
						Vector3((float)g, 0.0f, 5.0f), data->orbit_yaw, data->orbit_pitch, vp, b)) {
					UIDraw::draw_clipped_line(ci, vp, a, b,
						g == 0 ? Color(0.24f, 0.42f, 0.78f, 1.0f) : gcol, g == 0 ? 2.0f : 1.0f);
				}
				if (_project_3d(
						Vector3(-5.0f, 0.0f, (float)g), data->orbit_yaw, data->orbit_pitch, vp, a) &&
					_project_3d(
						Vector3(5.0f, 0.0f, (float)g), data->orbit_yaw, data->orbit_pitch, vp, b)) {
					UIDraw::draw_clipped_line(ci, vp, a, b,
						g == 0 ? Color(0.78f, 0.26f, 0.30f, 1.0f) : gcol, g == 0 ? 2.0f : 1.0f);
				}
			}
			// Y-up axis
			{
				Point2 o, up;
				if (_project_3d(Vector3(0, 0, 0), data->orbit_yaw, data->orbit_pitch, vp, o) &&
					_project_3d(Vector3(0, 2.5f, 0), data->orbit_yaw, data->orbit_pitch, vp, up)) {
					UIDraw::draw_clipped_line(
						ci, vp, o, up, Color(0.25f, 0.80f, 0.45f, 1.0f), 2.0f);
				}
			}

			// Project scene entities as 3D wireframe boxes
			for (uint32_t i = 1; i < data->entities.size(); i++) {
				const SceneEntity& e = data->entities[i];
				const float cx = (e.position.x - 240.0f) / 80.0f;
				const float cz = (e.position.y - 200.0f) / 80.0f;
				const float hw = MAX(0.25f, e.size.x / 180.0f);
				const float hh = MAX(0.25f, e.size.y / 120.0f);
				Vector3 corners[8] = {
					Vector3(cx - hw, 0.0f, cz - hw),
					Vector3(cx + hw, 0.0f, cz - hw),
					Vector3(cx + hw, 0.0f, cz + hw),
					Vector3(cx - hw, 0.0f, cz + hw),
					Vector3(cx - hw, hh * 2.0f, cz - hw),
					Vector3(cx + hw, hh * 2.0f, cz - hw),
					Vector3(cx + hw, hh * 2.0f, cz + hw),
					Vector3(cx - hw, hh * 2.0f, cz + hw),
				};
				Point2 pts[8];
				bool ok = true;
				for (int c = 0; c < 8; c++) {
					if (!_project_3d(corners[c], data->orbit_yaw, data->orbit_pitch, vp, pts[c])) {
						ok = false;
						break;
					}
				}
				if (ok) {
					const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
						{6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
					const bool is_sel = ((int)i == data->selected_entity_idx);
					Color wire_col = is_sel ? Color(1.0f, 1.0f, 1.0f, 1.0f) : e.color;
					for (int ed = 0; ed < 12; ed++) {
						UIDraw::draw_clipped_line(ci, vp, pts[edges[ed][0]], pts[edges[ed][1]],
							wire_col, is_sel ? 2.0f : 1.4f);
					}
				}
			}

			data->dock_buttons[3].label = "Reset Orbit";
			data->dock_buttons[3].rect =
				Rect2(vp.position.x + vp.size.x - 96.0f, vp.position.y + 8.0f, 88.0f, 26.0f);
			UIDraw::draw_button(ci, font, data->dock_buttons[3], 11);
			if (font.is_valid() && vp.size.x > 160.0f) {
				font->draw_string(ci, Point2(vp.position.x + 12.0f, vp.position.y + 24.0f),
					"3D PERSPECTIVE VIEWPORT  —  Drag anywhere to orbit camera",
					HORIZONTAL_ALIGNMENT_LEFT, vp.size.x - 120.0f, 12, col_subtext);
			}
		}
		else {
			// SCRIPT / TEXT EDITOR VIEW
			Rect2 shdr(vp.position.x + 1.0f, vp.position.y + 1.0f, vp.size.x - 2.0f, 30.0f);
			Renderer::canvas_item_add_rect(ci, shdr, col_dock_hdr);
			if (font.is_valid()) {
				String fname = data->open_file_name.is_empty() ? "No file open"
															   : ("res://" + data->open_file_name);
				if (data->script_dirty) {
					fname += " *";
				}
				font->draw_string(ci,
					Point2(shdr.position.x + 12.0f, UIDraw::centered_baseline_y(font, shdr, 13)),
					fname + "   (Click below to type, Ctrl+S or Save to write to disk)",
					HORIZONTAL_ALIGNMENT_LEFT, MAX(0.0f, shdr.size.x - 24.0f), 13, col_text);
			}

			const float gutter_w = 44.0f;
			Renderer::canvas_item_add_rect(ci,
				Rect2(vp.position.x + 1.0f, vp.position.y + 31.0f, gutter_w, vp.size.y - 32.0f),
				Color(0.08f, 0.09f, 0.12f, 1.0f));

			const float line_h = 22.0f;
			float ly = vp.position.y + 36.0f;
			const int max_vis = MAX(1, (int)((vp.size.y - 42.0f) / line_h));
			for (int idx = data->script_scroll_line;
				 idx < data->script_lines.size() && (idx - data->script_scroll_line) < max_vis;
				 idx++) {
				const bool is_caret =
					(data->focus_field == FOCUS_SCRIPT_EDITOR && idx == data->caret_line);
				Rect2 lrect(
					vp.position.x + gutter_w + 2.0f, ly, vp.size.x - gutter_w - 6.0f, line_h);
				if (is_caret) {
					Renderer::canvas_item_add_rect(ci, lrect, Color(0.13f, 0.18f, 0.25f, 1.0f));
				}
				if (font.is_valid()) {
					float by = UIDraw::centered_baseline_y(font, lrect, 13);
					font->draw_string(ci, Point2(vp.position.x + 6.0f, by), itos(idx + 1),
						HORIZONTAL_ALIGNMENT_RIGHT, gutter_w - 12.0f, 12, col_subtext);
					String code_txt =
						is_caret ? (data->script_lines[idx] + "|") : data->script_lines[idx];
					Color line_col = data->script_lines[idx].strip_edges().begins_with("#") ||
											 data->script_lines[idx].strip_edges().begins_with(";")
										 ? Color(0.45f, 0.62f, 0.50f, 1.0f)
										 : col_text;
					if (lrect.size.x > 16.0f) {
						font->draw_string(ci, Point2(lrect.position.x + 8.0f, by), code_txt,
							HORIZONTAL_ALIGNMENT_LEFT, lrect.size.x - 16.0f, 13, line_col);
					}
				}
				ly += line_h;
			}
		}
	}

	// 5. Right Column: INSPECTOR Dock
	if (data->layout.show_right && right_r.size.y > 40.0f) {
		const float ix = right_r.position.x;
		const float right_w = right_r.size.x;
		const float main_h = right_r.size.y;
		UIDraw::draw_dock_panel(
			ci, font, right_r, "INSPECTOR", col_dock, col_dock_hdr, col_subtext, 30.0f, 10.0f);

		if (_can_modify_selected_node() && right_w > 60.0f) {
			const SceneEntity& sel = data->entities[data->selected_entity_idx];
			float iy = body_y + 40.0f;
			const float inner_w = right_w - 20.0f;

			if (font.is_valid()) {
				font->draw_string(ci, Point2(ix + 10.0f, iy + 12.0f),
					"Node Name (" + sel.type + "):", HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12,
					col_subtext);
			}
			iy += 18.0f;
			data->inspector_name_field.text = sel.name;
			data->inspector_name_field.rect = Rect2(ix + 10.0f, iy, inner_w, 30.0f);
			data->inspector_name_field.draw(ci, font, data->focus_field == FOCUS_NODE_NAME, 13,
				col_vp_bg, col_accent, col_border);
			iy += 42.0f;

			auto draw_prop_stepper = [&](const String& p_lbl, float p_val, int p_btn_neg,
										 int p_btn_pos) {
				if (iy + 46.0f > body_y + main_h - 48.0f) {
					return;
				}
				if (font.is_valid()) {
					font->draw_string(ci, Point2(ix + 10.0f, iy + 11.0f), p_lbl,
						HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
				}
				iy += 16.0f;
				const float step_bw = 42.0f;
				data->inspector_buttons[p_btn_neg].rect = Rect2(ix + 10.0f, iy, step_bw, 26.0f);
				data->inspector_buttons[p_btn_pos].rect =
					Rect2(ix + 10.0f + inner_w - step_bw, iy, step_bw, 26.0f);
				UIDraw::draw_button(ci, font, data->inspector_buttons[p_btn_neg], 12);
				UIDraw::draw_button(ci, font, data->inspector_buttons[p_btn_pos], 12);

				Rect2 val_r(
					ix + 10.0f + step_bw + 6.0f, iy, inner_w - step_bw * 2.0f - 12.0f, 26.0f);
				Renderer::canvas_item_add_rect(ci, val_r, col_vp_bg);
				if (font.is_valid() && val_r.size.x > 12.0f) {
					font->draw_string(ci,
						Point2(val_r.position.x, UIDraw::centered_baseline_y(font, val_r, 13)),
						vformat("%.0f", p_val), HORIZONTAL_ALIGNMENT_CENTER, val_r.size.x, 13,
						col_text);
				}
				iy += 34.0f;
			};

			draw_prop_stepper("Position X", sel.position.x, 0, 1);
			draw_prop_stepper("Position Y", sel.position.y, 2, 3);
			draw_prop_stepper("Width", sel.size.x, 4, 5);
			draw_prop_stepper("Height", sel.size.y, 6, 7);

			// Color Swatches
			if (iy + 46.0f <= body_y + main_h - 48.0f) {
				if (font.is_valid()) {
					font->draw_string(ci, Point2(ix + 10.0f, iy + 12.0f), "Tint Color:",
						HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
				}
				iy += 18.0f;
				const float sw_gap = 8.0f;
				const float sw_w =
					(inner_w - sw_gap * (data->swatches.size() - 1)) / (float)data->swatches.size();
				for (uint32_t s = 0; s < data->swatches.size(); s++) {
					Rect2 sr(ix + 10.0f + s * (sw_w + sw_gap), iy, sw_w, 24.0f);
					data->swatches[s].rect = sr;
					bool match = sel.color.is_equal_approx(data->swatches[s].color);
					UIDraw::draw_bordered_rect(ci, sr, data->swatches[s].color,
						match ? Color(1, 1, 1, 1)
							  : (data->swatches[s].hovered ? col_accent : col_border),
						match ? 2.0f : 1.0f);
				}
				iy += 34.0f;
			}

			// Bottom Inspector Duplicate & Delete buttons
			const float act_y = body_y + main_h - 38.0f;
			if (act_y > iy) {
				const float half_w = (inner_w - 8.0f) * 0.5f;
				data->inspector_buttons[8].rect = Rect2(ix + 10.0f, act_y, half_w, 28.0f);
				data->inspector_buttons[9].rect =
					Rect2(ix + 10.0f + half_w + 8.0f, act_y, half_w, 28.0f);
				UIDraw::draw_button(ci, font, data->inspector_buttons[8], 12);
				UIDraw::draw_button(ci, font, data->inspector_buttons[9], 12);
			}
		}
	}

	// 6. Bottom OUTPUT / CONSOLE Panel
	if (data->layout.show_bottom) {
		UIDraw::draw_dock_panel(ci, font, bottom_r,
			"OUTPUT   —   Shortcuts: Ctrl+S Save  |  F5 Play  |  Arrow Keys Nudge Node",
			col_dock, col_dock_hdr, col_subtext, 26.0f, 80.0f);
		data->dock_buttons[4].rect =
			Rect2(bottom_r.position.x + bottom_r.size.x - 64.0f, bottom_r.position.y + 3.0f, 56.0f,
				20.0f);
		UIDraw::draw_button(ci, font, data->dock_buttons[4], 11);

		if (font.is_valid()) {
			const float line_h = 18.0f;
			const int max_lines = MAX(1, (int)((bottom_r.size.y - 32.0f) / line_h));
			const int start_idx = MAX(0, (int)data->logs.size() - max_lines);
			float ly = bottom_r.position.y + 30.0f;
			for (int i = start_idx; i < data->logs.size(); i++) {
				Rect2 lr(bottom_r.position.x + 10.0f, ly, bottom_r.size.x - 20.0f, line_h);
				font->draw_string(ci,
					Point2(lr.position.x, UIDraw::centered_baseline_y(font, lr, 12)),
					"> " + data->logs[i], HORIZONTAL_ALIGNMENT_LEFT, lr.size.x, 12, col_text);
				ly += line_h;
			}
		}
	}
}

void EditorUI::finalize()
{
	if (!data) {
		return;
	}
	data->ui.teardown(&EditorUI::handle_input_event);
	memdelete(data);
	data = nullptr;
	_initialized = false;
}

void EditorUI::_back_to_project_list()
{
	if (data) {
		data->pending_return_to_pm = true;
	}
}

void EditorUI::_switch_view_2d()
{
	if (data) {
		data->view_mode = VIEW_2D;
		data->focus_field = FOCUS_NONE;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_switch_view_3d()
{
	if (data) {
		data->view_mode = VIEW_3D;
		data->focus_field = FOCUS_NONE;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_switch_view_script()
{
	if (data) {
		data->view_mode = VIEW_SCRIPT;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_add_node()
{
	if (!data) {
		return;
	}
	SceneEntity ent;
	ent.name = vformat("Sprite2D_%d", data->next_entity_serial++);
	ent.type = "Sprite2D";
	float offset = 36.0f * ((data->entities.size() % 5));
	ent.position = Vector2(140.0f + offset, 120.0f + offset);
	ent.size = Size2(88.0f, 72.0f);
	ent.color = data->swatches[data->entities.size() % data->swatches.size()].color;
	ent.runtime_vel = Vector2(120.0f + offset, -110.0f);
	data->entities.push_back(ent);
	data->selected_entity_idx = (int)data->entities.size() - 1;
	data->scene_dirty = true;
	add_log("Added node: " + ent.name);
}

void EditorUI::_save_all()
{
	if (!data) {
		return;
	}
	save_scene();
	if (!data->open_file_path.is_empty() && data->script_dirty) {
		save_open_text_file();
	}
}

void EditorUI::_toggle_play()
{
	if (!data) {
		return;
	}
	if (data->playing) {
		stop_play();
	}
	else {
		start_play();
	}
}

void EditorUI::_stop_and_edit()
{
	if (!data) {
		return;
	}
	data->launched_directly_to_play = false;
	stop_play();
}

void EditorUI::_reset_simulation()
{
	if (!data) {
		return;
	}
	data->play_time = 0.0;
	for (uint32_t i = 0; i < data->entities.size(); i++) {
		data->entities[i].runtime_pos = data->entities[i].position;
	}
	data->ui.needs_redraw = true;
}

void EditorUI::_reset_camera_view()
{
	if (!data) {
		return;
	}
	data->pan_2d = Vector2();
	data->orbit_yaw = 0.65f;
	data->orbit_pitch = 0.42f;
	add_log("Reset viewport camera.");
}

void EditorUI::_refresh_files()
{
	if (!data) {
		return;
	}
	scan_project_files();
	add_log(vformat("Refreshed res:// (%d items)", data->files.size()));
}

void EditorUI::_new_script_file()
{
	if (!data || data->project_path.is_empty()) {
		return;
	}
	String path = data->project_path.path_join("main.gd");
	int idx = 1;
	while (FileAccess::exists(path)) {
		path = data->project_path.path_join(vformat("script_%d.gd", idx++));
	}
	Ref<FileAccess> fa = FileAccess::open(path, FileAccess::WRITE);
	if (fa.is_valid()) {
		fa->store_line("# Voltaire Script");
		fa->store_line("extends Node2D");
		fa->store_line("");
		fa->store_line("func _ready() -> void:");
		fa->store_line("    print(\"Hello from Voltaire!\")");
		fa->close();
		scan_project_files();
		open_text_file(path);
		_switch_view_script();
		add_log("Created script: res://" + path.get_file());
	}
}

void EditorUI::_clear_output()
{
	if (data) {
		data->logs.clear();
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_duplicate_selected_node()
{
	if (!_can_delete_selected_node()) {
		return;
	}
	SceneEntity copy = data->entities[data->selected_entity_idx];
	copy.name = vformat("%s_%d", copy.name, data->next_entity_serial++);
	copy.position += Vector2(24.0f, 24.0f);
	data->entities.push_back(copy);
	data->selected_entity_idx = (int)data->entities.size() - 1;
	data->scene_dirty = true;
	add_log("Duplicated node as " + copy.name);
}

void EditorUI::_delete_selected_node()
{
	if (!_can_delete_selected_node()) {
		return;
	}
	String removed = data->entities[data->selected_entity_idx].name;
	data->entities.remove_at(data->selected_entity_idx);
	data->selected_entity_idx = MIN(data->selected_entity_idx, (int)data->entities.size() - 1);
	data->scene_dirty = true;
	add_log("Removed node: " + removed);
}

void EditorUI::_nudge_pos_x_neg()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.x -= 10.0f;
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_pos_x_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.x += 10.0f;
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_pos_y_neg()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.y -= 10.0f;
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_pos_y_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].position.y += 10.0f;
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_size_w_neg()
{
	if (_can_modify_selected_node()) {
		SceneEntity& e = data->entities[data->selected_entity_idx];
		e.size.x = MAX(16.0f, e.size.x - 10.0f);
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_size_w_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].size.x += 10.0f;
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_size_h_neg()
{
	if (_can_modify_selected_node()) {
		SceneEntity& e = data->entities[data->selected_entity_idx];
		e.size.y = MAX(16.0f, e.size.y - 10.0f);
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_nudge_size_h_pos()
{
	if (_can_modify_selected_node()) {
		data->entities[data->selected_entity_idx].size.y += 10.0f;
		data->scene_dirty = true;
		data->ui.needs_redraw = true;
	}
}
