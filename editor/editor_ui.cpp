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

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "editor/editor_ui.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

namespace
{
bool _project_3d(
	const Vector3& p_pt, float p_yaw, float p_pitch, const Rect2& p_vp, Point2& r_screen)
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
					ent.size =
						Size2(MAX(16.0f, parts[4].to_float()), MAX(16.0f, parts[5].to_float()));
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
		fa->store_line(vformat("%s|%s|%.1f|%.1f|%.1f|%.1f|%.3f|%.3f|%.3f", e.name.replace("|", "_"),
			e.type.replace("|", "_"), e.position.x, e.position.y, e.size.x, e.size.y, e.color.r,
			e.color.g, e.color.b));
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
	data->project_name =
		p_project_name.is_empty() ? UIDraw::read_project_name(data->project_path) : p_project_name;
	if (data->project_name.is_empty()) {
		data->project_name = "Voltaire Project";
	}

	data->ui.initialize(p_viewport_rid, p_size, Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT),
		&EditorUI::handle_input_event);

	data->inspector_name_field.placeholder = "Node";

	// Top toolbar buttons: [0]=< Project List, [1]=2D, [2]=3D, [3]=Script,
	// [4]=Project, [5]=+ Add Node, [6]=Save, [7]=Play
	data->top_buttons.add("< Project List", &EditorUI::_back_to_project_list);
	data->top_buttons.add("2D", &EditorUI::_switch_view_2d, nullptr, false, true);
	data->top_buttons.add("3D", &EditorUI::_switch_view_3d);
	data->top_buttons.add("Script", &EditorUI::_switch_view_script);
	data->top_buttons.add("Project", &EditorUI::_open_project_settings);
	data->top_buttons.add("+ Add Node", &EditorUI::_add_node);
	data->top_buttons.add("Save", &EditorUI::_save_all);
	data->top_buttons.add("Play", &EditorUI::_toggle_play, nullptr, true);

	// Dock & viewport utility buttons: [0]=+ Node, [1]=+ Script, [2]=Refresh, [3]=Reset View,
	// [4]=Clear Output
	data->dock_buttons.add("+ Node", &EditorUI::_add_node);
	data->dock_buttons.add("+ Script", &EditorUI::_new_script_file);
	data->dock_buttons.add("Refresh", &EditorUI::_refresh_files);
	data->dock_buttons.add("Reset View", &EditorUI::_reset_camera_view);
	data->dock_buttons.add("Clear", &EditorUI::_clear_output);

	// Inspector buttons: [0..7]=pos/size nudges, [8]=Duplicate, [9]=Delete Node
	data->inspector_buttons.add(
		"-10", &EditorUI::_nudge_pos_x_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"+10", &EditorUI::_nudge_pos_x_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"-10", &EditorUI::_nudge_pos_y_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"+10", &EditorUI::_nudge_pos_y_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"-10", &EditorUI::_nudge_size_w_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"+10", &EditorUI::_nudge_size_w_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"-10", &EditorUI::_nudge_size_h_neg, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"+10", &EditorUI::_nudge_size_h_pos, &EditorUI::_can_modify_selected_node);
	data->inspector_buttons.add(
		"Duplicate", &EditorUI::_duplicate_selected_node, &EditorUI::_can_delete_selected_node);
	data->inspector_buttons.add(
		"Delete Node", &EditorUI::_delete_selected_node, &EditorUI::_can_delete_selected_node);
		// Project Settings modal buttons:
	// [0..1]=Width -/+, [2..3]=Height -/+, [4..5]=Physics Ticks -/+,
	// [6]=VSync, [7]=Resizable, [8]=Borderless, [9]=Use 3D, [10]=Cancel, [11]=Save & Close
	data->ps_buttons.add("-80", &EditorUI::_ps_width_dec);
	data->ps_buttons.add("+80", &EditorUI::_ps_width_inc);
	data->ps_buttons.add("-45", &EditorUI::_ps_height_dec);
	data->ps_buttons.add("+45", &EditorUI::_ps_height_inc);
	data->ps_buttons.add("-10", &EditorUI::_ps_ticks_dec);
	data->ps_buttons.add("+10", &EditorUI::_ps_ticks_inc);
	data->ps_buttons.add("VSync: ON", &EditorUI::_ps_toggle_vsync);
	data->ps_buttons.add("Resizable: ON", &EditorUI::_ps_toggle_resizable);
	data->ps_buttons.add("Borderless: OFF", &EditorUI::_ps_toggle_borderless);
	data->ps_buttons.add("3D Mode: OFF", &EditorUI::_ps_toggle_use_3d);
	data->ps_buttons.add("Cancel", &EditorUI::_close_project_settings);
	data->ps_buttons.add("Save & Close", &EditorUI::_save_project_settings, nullptr, true);

	data->ps_name_field.placeholder = "Project Name";
	data->ps_main_scene_field.placeholder = "res://main.vscn";

	vltr_project_init(&data->project_data, data->project_name.utf8().get_data(), "res://main.vscn");
	String proj_path = data->project_path.path_join("project.vltr");
	Ref<FileAccess> fa = FileAccess::open(proj_path, FileAccess::READ);
	if (fa.is_valid() && fa->get_length() >= (int64_t)sizeof(VoltaireProjectFile)) {
		VoltaireProjectFile loaded;
		fa->get_buffer((uint8_t*)&loaded, sizeof(VoltaireProjectFile));
		if (vltr_project_valid(&loaded)) {
			data->project_data = loaded;
			if (loaded.config.project_name[0] != '\0') {
				data->project_name = String::utf8(loaded.config.project_name);
			}
		}
	}

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

void EditorUI::handle_input_event(const InputEvent& p_event)
{
	if (!data || p_event.is_null()) {
		return;
	}
	data->ui.forward_input(p_event, &EditorUI::handle_input_event);

	if (p_event.type == InputEventType::MOUSE_MOTION) {
		Point2 prev_mouse = data->ui.mouse_pos;
		data->ui.mouse_pos = p_event.mouse_motion.position;
		Vector2 delta = data->ui.mouse_pos - prev_mouse;

		if (data->playing) {
			if (data->runtime_buttons.update_hover(data->ui.mouse_pos)) {
				data->ui.needs_redraw = true;
			}
			return;
		}

		if (data->show_project_settings) {
			if (data->ps_name_field.selecting) {
				if (data->ps_name_field.handle_mouse_motion(p_event, data->ui.font, 13)) {
					data->ui.needs_redraw = true;
				}
				return;
			}
			if (data->ps_main_scene_field.selecting) {
				if (data->ps_main_scene_field.handle_mouse_motion(p_event, data->ui.font, 13)) {
					data->ui.needs_redraw = true;
				}
				return;
			}
			if (data->ps_buttons.update_hover(data->ui.mouse_pos)) {
				data->ui.needs_redraw = true;
			}
			return;
		}

		if (data->inspector_name_field.selecting) {
			if (data->inspector_name_field.handle_mouse_motion(p_event, data->ui.font, 13)) {
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
				data->orbit_pitch = CLAMP(data->orbit_pitch - delta.y * 0.01f, -1.35f, 1.35f);
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

	if (p_event.type == InputEventType::MOUSE_BUTTON) {
		const MouseButton btn = p_event.mouse_button.button_index;
		const bool is_pressed = p_event.is_pressed();
		data->ui.mouse_pos = p_event.mouse_button.position;

		if (data->show_project_settings) {
			if (btn != MouseButton::LEFT) {
				return;
			}
			if (is_pressed) {
				if (data->ps_buttons.press(data->ui.mouse_pos)) {
					data->ui.needs_redraw = true;
					return;
				}
				if (data->ps_name_field.has_point(data->ui.mouse_pos)) {
					data->focus_field = FOCUS_PS_NAME;
					data->ps_name_field.handle_mouse_button(p_event, data->ui.font, 13);
					data->ui.needs_redraw = true;
					return;
				}
				if (data->ps_main_scene_field.has_point(data->ui.mouse_pos)) {
					data->focus_field = FOCUS_PS_MAIN_SCENE;
					data->ps_main_scene_field.handle_mouse_button(p_event, data->ui.font, 13);
					data->ui.needs_redraw = true;
					return;
				}
				data->focus_field = FOCUS_NONE;
				data->ui.needs_redraw = true;
			}
			else {
				if (data->ps_name_field.selecting) {
					data->ps_name_field.handle_mouse_button(p_event, data->ui.font, 13);
				}
				if (data->ps_main_scene_field.selecting) {
					data->ps_main_scene_field.handle_mouse_button(p_event, data->ui.font, 13);
				}
				void (*to_call)() = data->ps_buttons.release(data->ui.mouse_pos);
				data->ui.needs_redraw = true;
				if (to_call) {
					to_call();
				}
			}
			return;
		}

		// Scroll wheel in Script view
		if (is_pressed && !data->playing && data->view_mode == VIEW_SCRIPT &&
			data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
			if (btn == MouseButton::WHEEL_UP && data->script_scroll_line > 0) {
				data->script_scroll_line = MAX(0, data->script_scroll_line - 3);
				data->ui.needs_redraw = true;
				return;
			}
			if (btn == MouseButton::WHEEL_DOWN) {
				int max_sc = MAX(0, (int)data->script_lines.size() - 5);
				data->script_scroll_line = MIN(max_sc, data->script_scroll_line + 3);
				data->ui.needs_redraw = true;
				return;
			}
		}

		// Right or middle mouse drag pans 2D / orbits 3D
		if (btn == MouseButton::RIGHT || btn == MouseButton::MIDDLE) {
			if (!data->playing && is_pressed &&
				data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
				data->panning_viewport = true;
			}
			else if (!is_pressed) {
				data->panning_viewport = false;
			}
			return;
		}

		if (btn != MouseButton::LEFT) {
			return;
		}

		if (data->playing) {
			if (is_pressed) {
				bool hit_btn = data->runtime_buttons.press(data->ui.mouse_pos);
				// Clicking in the runtime arena gives moving entities an interactive bounce impulse
				if (!hit_btn && data->center_viewport_rect.has_point(data->ui.mouse_pos)) {
					for (uint32_t i = 1; i < data->entities.size(); i++) {
						data->entities[i].runtime_vel =
							Vector2(-data->entities[i].runtime_vel.y * 1.05f,
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
		if (is_pressed) {
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
				const String& nm = data->entities[data->selected_entity_idx].name;
				if (data->focus_field != FOCUS_NODE_NAME || data->inspector_name_field.text != nm) {
					data->inspector_name_field.set_text(nm);
				}
				data->focus_field = FOCUS_NODE_NAME;
				data->inspector_name_field.handle_mouse_button(p_event, data->ui.font, 13);
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
			if (data->inspector_name_field.selecting) {
				data->inspector_name_field.handle_mouse_button(p_event, data->ui.font, 13);
			}
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

	if (p_event.type == InputEventType::KEY && p_event.is_pressed()) {
		const Key code = (p_event.key.keycode != Key::NONE) ? p_event.key.keycode
															: p_event.key.physical_keycode;

		if (data->show_project_settings) {
			if (code == Key::ESCAPE) {
				_close_project_settings();
				return;
			}
			if (code == Key::ENTER || code == Key::KP_ENTER) {
				_save_project_settings();
				return;
			}
			if (data->focus_field == FOCUS_PS_NAME) {
				if (data->ps_name_field.handle_key(p_event)) {
					data->ui.needs_redraw = true;
				}
			}
			else if (data->focus_field == FOCUS_PS_MAIN_SCENE) {
				if (data->ps_main_scene_field.handle_key(p_event)) {
					data->ui.needs_redraw = true;
				}
			}
			return;
		}

		if (code == Key::F5 && !data->playing) {
			start_play();
			return;
		}
		if ((code == Key::F8 || code == Key::ESCAPE) && data->playing) {
			stop_play();
			return;
		}
		if (p_event.is_ctrl_pressed() && code == Key::S && !data->playing) {
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
			if (data->inspector_name_field.handle_key(p_event, '|')) {
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
			char32_t ch = p_event.key.unicode;
			if (ch >= 32 && ch != 127 && !p_event.is_ctrl_pressed() && !p_event.is_meta_pressed()) {
				data->script_lines.write[data->caret_line] += String::chr(ch);
				data->script_dirty = true;
				data->ui.needs_redraw = true;
				return;
			}
		}

		// Arrow-key nudging in 2D view when no text field is focused
		if (data->focus_field == FOCUS_NONE && data->view_mode == VIEW_2D &&
			_can_modify_selected_node()) {
			float step = p_event.is_shift_pressed() ? 10.0f : 2.0f;
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

	// Right action buttons: [Project] [+ Add Node] [Save] [Play]
	float right_cur = W - pad - 10.0f;
	const float right_widths[4] = {78.0f, 96.0f, 68.0f, 76.0f};
	for (int idx = 7; idx >= 4; idx--) {
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
			font->draw_string(ci, Point2(left_cur, UIDraw::centered_baseline_y(font, top_rect, 15)),
				title, HORIZONTAL_ALIGNMENT_LEFT, title_w, 15, col_text);
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
		data->dock_buttons[0].rect = Rect2(scene_rect.position.x + scene_rect.size.x - 62.0f,
			scene_rect.position.y + 4.0f, 56.0f, 22.0f);
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
						fsel ? Color(0.17f, 0.26f, 0.35f, 1.0f) : Color(0.15f, 0.19f, 0.24f, 1.0f));
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
					"2D VIEWPORT  --  Drag nodes to move, Right-drag to pan",
					HORIZONTAL_ALIGNMENT_LEFT, vp.size.x - 120.0f, 12, col_subtext);
			}
		}
		else if (data->view_mode == VIEW_3D) {
			// Ground Grid (-5..5 on XZ plane)
			const Color gcol(0.14f, 0.17f, 0.22f, 1.0f);
			for (int g = -5; g <= 5; g++) {
				Point2 a, b;
				if (_project_3d(Vector3((float)g, 0.0f, -5.0f), data->orbit_yaw, data->orbit_pitch,
						vp, a) &&
					_project_3d(
						Vector3((float)g, 0.0f, 5.0f), data->orbit_yaw, data->orbit_pitch, vp, b)) {
					UIDraw::draw_clipped_line(ci, vp, a, b,
						g == 0 ? Color(0.24f, 0.42f, 0.78f, 1.0f) : gcol, g == 0 ? 2.0f : 1.0f);
				}
				if (_project_3d(Vector3(-5.0f, 0.0f, (float)g), data->orbit_yaw, data->orbit_pitch,
						vp, a) &&
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
					"3D PERSPECTIVE VIEWPORT  --  Drag anywhere to orbit camera",
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
					font->draw_string(ci, Point2(ix + 10.0f, iy + 12.0f),
						"Tint Color:", HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
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
			"OUTPUT   --   Shortcuts: Ctrl+S Save  |  F5 Play  |  Arrow Keys Nudge Node", col_dock,
			col_dock_hdr, col_subtext, 26.0f, 80.0f);
		data->dock_buttons[4].rect = Rect2(bottom_r.position.x + bottom_r.size.x - 64.0f,
			bottom_r.position.y + 3.0f, 56.0f, 20.0f);
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

	// 7. Project Settings Modal Overlay (edits project.vltr)
	data->ps_buttons.clear_rects();
	data->ps_name_field.rect = Rect2();
	data->ps_main_scene_field.rect = Rect2();

	if (data->show_project_settings) {
		Renderer::canvas_item_add_rect(ci, Rect2(0, 0, W, H), Color(0.02f, 0.03f, 0.04f, 0.72f));

		const float mw = MIN(520.0f, W - 40.0f);
		const float mh = MIN(470.0f, H - 40.0f);
		const float mx = Math::floor((W - mw) * 0.5f);
		const float my = Math::floor((H - mh) * 0.5f);
		Rect2 modal_r(mx, my, mw, mh);

		UIDraw::draw_bordered_rect(ci, modal_r, col_dock, col_accent, 2.0f);
		Rect2 mhdr(mx + 2.0f, my + 2.0f, mw - 4.0f, 36.0f);
		Renderer::canvas_item_add_rect(ci, mhdr, col_dock_hdr);
		if (font.is_valid()) {
			font->draw_string(ci,
				Point2(mx + 16.0f, UIDraw::centered_baseline_y(font, mhdr, 14)),
				"PROJECT SETTINGS  (project.vltr)", HORIZONTAL_ALIGNMENT_LEFT, mw - 32.0f, 14,
				col_text);
		}

		float cy = my + 50.0f;
		const float inner_x = mx + 18.0f;
		const float inner_w = mw - 36.0f;

		if (font.is_valid()) {
			font->draw_string(ci, Point2(inner_x, cy + 12.0f), "Project Name:",
				HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
		}
		cy += 18.0f;
		data->ps_name_field.rect = Rect2(inner_x, cy, inner_w, 30.0f);
		data->ps_name_field.draw(ci, font, data->focus_field == FOCUS_PS_NAME, 13, col_vp_bg,
			col_accent, col_border);
		cy += 40.0f;

		if (font.is_valid()) {
			font->draw_string(ci, Point2(inner_x, cy + 12.0f), "Main Scene:",
				HORIZONTAL_ALIGNMENT_LEFT, inner_w, 12, col_subtext);
		}
		cy += 18.0f;
		data->ps_main_scene_field.rect = Rect2(inner_x, cy, inner_w, 30.0f);
		data->ps_main_scene_field.draw(ci, font, data->focus_field == FOCUS_PS_MAIN_SCENE, 13,
			col_vp_bg, col_accent, col_border);
		cy += 42.0f;

		auto draw_modal_stepper = [&](const String& p_lbl, uint32_t p_val, int p_dec, int p_inc) {
			if (font.is_valid()) {
				font->draw_string(ci, Point2(inner_x, cy + 18.0f), p_lbl,
					HORIZONTAL_ALIGNMENT_LEFT, inner_w * 0.45f, 13, col_text);
			}
			const float bw = 48.0f;
			const float val_w = 78.0f;
			const float right_edge = inner_x + inner_w;
			data->ps_buttons[p_inc].rect = Rect2(right_edge - bw, cy, bw, 28.0f);
			Rect2 vr(right_edge - bw - 6.0f - val_w, cy, val_w, 28.0f);
			data->ps_buttons[p_dec].rect = Rect2(vr.position.x - 6.0f - bw, cy, bw, 28.0f);

			UIDraw::draw_button(ci, font, data->ps_buttons[p_dec], 12);
			Renderer::canvas_item_add_rect(ci, vr, col_vp_bg);
			if (font.is_valid()) {
				font->draw_string(ci, Point2(vr.position.x, UIDraw::centered_baseline_y(font, vr, 13)),
					itos((int64_t)p_val), HORIZONTAL_ALIGNMENT_CENTER, vr.size.x, 13, col_text);
			}
			UIDraw::draw_button(ci, font, data->ps_buttons[p_inc], 12);
			cy += 36.0f;
		};

		draw_modal_stepper("Window Width", data->ps_draft.config.window_width, 0, 1);
		draw_modal_stepper("Window Height", data->ps_draft.config.window_height, 2, 3);
		draw_modal_stepper(
			"Physics Ticks / Sec", data->ps_draft.config.physics_ticks_per_second, 4, 5);

		cy += 4.0f;
		const uint32_t fl = data->ps_draft.header.flags;
		data->ps_buttons[6].label = (fl & PROJECT_FLAG_VSYNC) ? "VSync: ON" : "VSync: OFF";
		data->ps_buttons[7].label =
			(fl & PROJECT_FLAG_RESIZABLE) ? "Resizable: ON" : "Resizable: OFF";
		data->ps_buttons[8].label =
			(fl & PROJECT_FLAG_BORDERLESS) ? "Borderless: ON" : "Borderless: OFF";
		data->ps_buttons[9].label = (fl & PROJECT_FLAG_USE_3D) ? "3D Mode: ON" : "3D Mode: OFF";

		const float half_w = (inner_w - 10.0f) * 0.5f;
		data->ps_buttons[6].rect = Rect2(inner_x, cy, half_w, 30.0f);
		data->ps_buttons[7].rect = Rect2(inner_x + half_w + 10.0f, cy, half_w, 30.0f);
		UIDraw::draw_button(ci, font, data->ps_buttons[6], 12);
		UIDraw::draw_button(ci, font, data->ps_buttons[7], 12);
		cy += 38.0f;

		data->ps_buttons[8].rect = Rect2(inner_x, cy, half_w, 30.0f);
		data->ps_buttons[9].rect = Rect2(inner_x + half_w + 10.0f, cy, half_w, 30.0f);
		UIDraw::draw_button(ci, font, data->ps_buttons[8], 12);
		UIDraw::draw_button(ci, font, data->ps_buttons[9], 12);

		// Footer Cancel & Save buttons
		const float fy = my + mh - 44.0f;
		data->ps_buttons[11].rect = Rect2(inner_x + inner_w - 122.0f, fy, 122.0f, 30.0f);
		data->ps_buttons[10].rect =
			Rect2(data->ps_buttons[11].rect.position.x - 94.0f, fy, 86.0f, 30.0f);
		UIDraw::draw_button(ci, font, data->ps_buttons[10], 12);
		UIDraw::draw_button(ci, font, data->ps_buttons[11], 12);
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

void EditorUI::_open_project_settings()
{
	if (!data) {
		return;
	}
	data->ps_draft = data->project_data;
	data->ps_name_field.set_text(String::utf8(data->ps_draft.config.project_name));
	data->ps_main_scene_field.set_text(String::utf8(data->ps_draft.config.main_scene));
	data->show_project_settings = true;
	data->focus_field = FOCUS_PS_NAME;
	data->ui.needs_redraw = true;
}

void EditorUI::_close_project_settings()
{
	if (!data) {
		return;
	}
	data->show_project_settings = false;
	data->focus_field = FOCUS_NONE;
	data->ui.needs_redraw = true;
}

void EditorUI::_save_project_settings()
{
	if (!data) {
		return;
	}
	String new_name = data->ps_name_field.text.strip_edges();
	if (new_name.is_empty()) {
		new_name = "Voltaire Project";
	}
	String new_scene = data->ps_main_scene_field.text.strip_edges();
	if (new_scene.is_empty()) {
		new_scene = "res://main.vscn";
	}

	vltr_copy_cstr(data->ps_draft.config.project_name, sizeof(data->ps_draft.config.project_name),
		new_name.utf8().get_data());
	vltr_copy_cstr(data->ps_draft.config.main_scene, sizeof(data->ps_draft.config.main_scene),
		new_scene.utf8().get_data());

	data->project_data = data->ps_draft;
	data->project_name = new_name;

	String proj_path = data->project_path.path_join("project.vltr");
	Ref<FileAccess> fa = FileAccess::open(proj_path, FileAccess::WRITE);
	if (fa.is_valid()) {
		fa->store_buffer((const uint8_t*)&data->project_data, sizeof(VoltaireProjectFile));
		fa->close();
		scan_project_files();
		add_log(vformat("Saved project.vltr (%s, %dx%d)", data->project_name,
			data->project_data.config.window_width, data->project_data.config.window_height));
	}
	else {
		add_log("ERROR: Failed to write " + proj_path);
	}

	if (DisplayServer::is_initialized()) {
		DisplayServer::window_set_title(
			data->project_name + " - Voltaire Editor", DisplayServerEnums::MAIN_WINDOW_ID);
	}

	data->show_project_settings = false;
	data->focus_field = FOCUS_NONE;
	data->ui.needs_redraw = true;
}

void EditorUI::_ps_width_dec()
{
	if (data) {
		data->ps_draft.config.window_width = MAX(320u,
			data->ps_draft.config.window_width > 80u ? data->ps_draft.config.window_width - 80u
													: 320u);
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_width_inc()
{
	if (data) {
		data->ps_draft.config.window_width = MIN(7680u, data->ps_draft.config.window_width + 80u);
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_height_dec()
{
	if (data) {
		data->ps_draft.config.window_height = MAX(240u,
			data->ps_draft.config.window_height > 45u ? data->ps_draft.config.window_height - 45u
													 : 240u);
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_height_inc()
{
	if (data) {
		data->ps_draft.config.window_height = MIN(4320u, data->ps_draft.config.window_height + 45u);
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_ticks_dec()
{
	if (data) {
		data->ps_draft.config.physics_ticks_per_second =
			MAX(10u, data->ps_draft.config.physics_ticks_per_second > 10u
						 ? data->ps_draft.config.physics_ticks_per_second - 10u
						 : 10u);
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_ticks_inc()
{
	if (data) {
		data->ps_draft.config.physics_ticks_per_second =
			MIN(240u, data->ps_draft.config.physics_ticks_per_second + 10u);
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_toggle_vsync()
{
	if (data) {
		data->ps_draft.header.flags ^= PROJECT_FLAG_VSYNC;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_toggle_resizable()
{
	if (data) {
		data->ps_draft.header.flags ^= PROJECT_FLAG_RESIZABLE;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_toggle_borderless()
{
	if (data) {
		data->ps_draft.header.flags ^= PROJECT_FLAG_BORDERLESS;
		data->ui.needs_redraw = true;
	}
}

void EditorUI::_ps_toggle_use_3d()
{
	if (data) {
		data->ps_draft.header.flags ^= PROJECT_FLAG_USE_3D;
		data->ui.needs_redraw = true;
	}
}


// ============================================================================
// Merged from editor/ui_widgets.cpp
// ============================================================================

#include "core/config/project_format.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "editor/editor_ui.h"
#include "scene/theme/theme_db.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

static inline bool _is_word_char(char32_t c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static inline int _prev_word_boundary(const String& s, int pos)
{
	pos = CLAMP(pos, 0, s.length());
	while (pos > 0 && !_is_word_char(s[pos - 1])) {
		pos--;
	}
	while (pos > 0 && _is_word_char(s[pos - 1])) {
		pos--;
	}
	return pos;
}

static inline int _next_word_boundary(const String& s, int pos)
{
	int len = s.length();
	pos = CLAMP(pos, 0, len);
	while (pos < len && _is_word_char(s[pos])) {
		pos++;
	}
	while (pos < len && !_is_word_char(s[pos])) {
		pos++;
	}
	return pos;
}

int UITextField::hit_test_cursor(const Ref<Font>& p_font, int p_font_size, float p_mouse_x) const
{
	if (p_font.is_null() || text.is_empty()) {
		return 0;
	}
	float local_x = p_mouse_x - (rect.position.x + 10.0f) + scroll_x;
	if (local_x <= 0.0f) {
		return 0;
	}
	int len = text.length();
	float prev_w = 0.0f;
	for (int i = 1; i <= len; i++) {
		float w =
			p_font->get_string_size(text.substr(0, i), HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_size)
				.x;
		if (local_x < (prev_w + w) * 0.5f) {
			return i - 1;
		}
		prev_w = w;
	}
	return len;
}

bool UITextField::handle_mouse_button(
	const InputEvent& p_event, const Ref<Font>& p_font, int p_font_size)
{
	if (p_event.type != InputEventType::MOUSE_BUTTON ||
		p_event.mouse_button.button_index != MouseButton::LEFT) {
		return false;
	}
	if (!p_event.is_pressed()) {
		if (selecting) {
			selecting = false;
			return true;
		}
		return false;
	}
	if (!rect.has_point(p_event.mouse_button.position)) {
		selecting = false;
		return false;
	}

	int len = text.length();
	cursor = CLAMP(cursor, 0, len);
	anchor = CLAMP(anchor, 0, len);

	if (p_event.mouse_button.double_click) {
		select_all();
		selecting = false;
		return true;
	}

	int idx = hit_test_cursor(p_font, p_font_size, p_event.mouse_button.position.x);
	cursor = idx;
	if (!p_event.is_shift_pressed()) {
		anchor = idx;
	}
	selecting = true;
	return true;
}

bool UITextField::handle_mouse_motion(
	const InputEvent& p_event, const Ref<Font>& p_font, int p_font_size)
{
	if (!selecting || p_event.type != InputEventType::MOUSE_MOTION) {
		return false;
	}
	int next = hit_test_cursor(p_font, p_font_size, p_event.mouse_motion.position.x);
	if (next != cursor) {
		cursor = next;
		return true;
	}
	return false;
}

bool UITextField::handle_key(const InputEvent& p_key_event, char32_t p_disallow_char)
{
	if (p_key_event.type != InputEventType::KEY || !p_key_event.is_pressed()) {
		return false;
	}

	int len = text.length();
	cursor = CLAMP(cursor, 0, len);
	anchor = CLAMP(anchor, 0, len);

	const Key code = p_key_event.key.keycode;
	const bool ctrl = p_key_event.is_ctrl_pressed() || p_key_event.is_meta_pressed();
	const bool shift = p_key_event.is_shift_pressed();

	auto erase_selection = [&]() -> bool {
		int sel_a = MIN(cursor, anchor);
		int sel_b = MAX(cursor, anchor);
		if (sel_a == sel_b) {
			return false;
		}
		text = text.substr(0, sel_a) + text.substr(sel_b);
		cursor = anchor = sel_a;
		return true;
	};

	// Select All (Ctrl+A)
	if (ctrl && code == Key::A) {
		select_all();
		return true;
	}

	// Copy / Cut (Ctrl+C / Ctrl+X)
	if (ctrl && (code == Key::C || code == Key::X)) {
		int sel_a = MIN(cursor, anchor);
		int sel_b = MAX(cursor, anchor);
		if (sel_a < sel_b && DisplayServer::is_initialized()) {
			DisplayServer::clipboard_set(text.substr(sel_a, sel_b - sel_a));
			if (code == Key::X) {
				erase_selection();
			}
			return true;
		}
		return false;
	}

	// Paste (Ctrl+V)
	if (ctrl && code == Key::V && DisplayServer::is_initialized()) {
		String clip = DisplayServer::clipboard_get().replace("\r", "").replace("\n", "");
		if (p_disallow_char != 0) {
			clip = clip.replace(String::chr(p_disallow_char), "_");
		}
		bool changed = erase_selection();
		if (!clip.is_empty()) {
			text = text.substr(0, cursor) + clip + text.substr(cursor);
			cursor += clip.length();
			anchor = cursor;
			changed = true;
		}
		return changed;
	}

	// Caret navigation
	if (code == Key::LEFT || code == Key::RIGHT || code == Key::HOME || code == Key::END) {
		int next = cursor;
		if (code == Key::LEFT) {
			if (!shift && cursor != anchor) {
				next = MIN(cursor, anchor);
			}
			else {
				next = ctrl ? _prev_word_boundary(text, cursor) : MAX(0, cursor - 1);
			}
		}
		else if (code == Key::RIGHT) {
			if (!shift && cursor != anchor) {
				next = MAX(cursor, anchor);
			}
			else {
				next = ctrl ? _next_word_boundary(text, cursor) : MIN(len, cursor + 1);
			}
		}
		else if (code == Key::HOME) {
			next = 0;
		}
		else if (code == Key::END) {
			next = len;
		}
		cursor = next;
		if (!shift) {
			anchor = cursor;
		}
		return true;
	}

	// Backspace
	if (code == Key::BACKSPACE) {
		if (erase_selection()) {
			return true;
		}
		if (cursor > 0) {
			int del_from = ctrl ? _prev_word_boundary(text, cursor) : (cursor - 1);
			text = text.substr(0, del_from) + text.substr(cursor);
			cursor = anchor = del_from;
			return true;
		}
		return false;
	}

	// Delete
	if (code == Key::KEY_DELETE) {
		if (erase_selection()) {
			return true;
		}
		if (cursor < len) {
			int del_to = ctrl ? _next_word_boundary(text, cursor) : (cursor + 1);
			text = text.substr(0, cursor) + text.substr(del_to);
			anchor = cursor;
			return true;
		}
		return false;
	}

	// Printable character insertion (replaces selection if active)
	char32_t ch = p_key_event.key.unicode;
	if (ch >= 32 && ch != 127 && ch != p_disallow_char && !ctrl) {
		erase_selection();
		text = text.substr(0, cursor) + String::chr(ch) + text.substr(cursor);
		cursor += 1;
		anchor = cursor;
		return true;
	}

	return false;
}

void UITextField::draw(RID p_ci, const Ref<Font>& p_font, bool p_focused, int p_font_size,
	const Color& p_bg, const Color& p_accent, const Color& p_border, float p_right_reserve) const
{
	UIDraw::draw_bordered_rect(
		p_ci, rect, p_bg, p_focused ? p_accent : p_border, p_focused ? 2.0f : 1.0f);

	const float max_w = rect.size.x - 20.0f - p_right_reserve;
	if (p_font.is_null() || max_w <= 12.0f) {
		return;
	}

	const float text_x = rect.position.x + 10.0f;
	const float base_y = UIDraw::centered_baseline_y(p_font, rect, p_font_size);

	if (text.is_empty()) {
		if (!placeholder.is_empty()) {
			p_font->draw_string(p_ci, Point2(text_x, base_y), placeholder,
				HORIZONTAL_ALIGNMENT_LEFT, max_w, p_font_size, Color(0.58f, 0.64f, 0.72f, 1.0f));
		}
		if (p_focused) {
			float caret_h = MAX(12.0f, rect.size.y - 14.0f);
			float caret_y = rect.position.y + (rect.size.y - caret_h) * 0.5f;
			Renderer::canvas_item_add_rect(p_ci, Rect2(text_x, caret_y, 2.0f, caret_h), p_accent);
		}
		return;
	}

	int len = text.length();
	int cur = CLAMP(cursor, 0, len);
	int anc = CLAMP(anchor, 0, len);

	auto prefix_w = [&](int idx) -> float {
		if (idx <= 0) {
			return 0.0f;
		}
		return p_font
			->get_string_size(text.substr(0, idx), HORIZONTAL_ALIGNMENT_LEFT, -1, p_font_size)
			.x;
	};

	float cur_px = prefix_w(cur);
	float sx = scroll_x;
	if (cur_px - sx > max_w - 4.0f) {
		sx = cur_px - max_w + 4.0f;
	}
	else if (cur_px - sx < 0.0f) {
		sx = cur_px;
	}
	const_cast<UITextField*>(this)->scroll_x = MAX(0.0f, sx);

	// Find visible character slice [vis_start, vis_end) that fits inside max_w
	int vis_start = 0;
	while (vis_start < len && prefix_w(vis_start + 1) <= sx) {
		vis_start++;
	}
	float start_px = prefix_w(vis_start);
	int vis_end = vis_start;
	while (vis_end < len && (prefix_w(vis_end + 1) - start_px) <= max_w) {
		vis_end++;
	}

	const float caret_h = MAX(12.0f, rect.size.y - 12.0f);
	const float caret_y = rect.position.y + (rect.size.y - caret_h) * 0.5f;

	// Draw selection highlight rectangle
	if (p_focused && cur != anc) {
		int sel_a = CLAMP(MIN(cur, anc), vis_start, vis_end);
		int sel_b = CLAMP(MAX(cur, anc), vis_start, vis_end);
		if (sel_b > sel_a) {
			float x0 = text_x + (prefix_w(sel_a) - start_px);
			float x1 = text_x + (prefix_w(sel_b) - start_px);
			Renderer::canvas_item_add_rect(p_ci, Rect2(x0, caret_y, x1 - x0, caret_h),
				Color(p_accent.r, p_accent.g, p_accent.b, 0.32f));
		}
	}

	// Draw visible text slice
	String vis_str = text.substr(vis_start, vis_end - vis_start);
	p_font->draw_string(p_ci, Point2(text_x, base_y), vis_str, HORIZONTAL_ALIGNMENT_LEFT, max_w,
		p_font_size, Color(0.92f, 0.94f, 0.96f, 1.0f));

	// Draw vertical caret bar
	if (p_focused) {
		float caret_x = text_x + CLAMP(cur_px - start_px, 0.0f, max_w);
		Renderer::canvas_item_add_rect(p_ci, Rect2(caret_x, caret_y, 2.0f, caret_h), p_accent);
	}
}

void UIContext::initialize(
	RID p_viewport_rid, Size2i p_size, Size2i p_default_size, Input::EventDispatchFunc p_handler)
{
	window_size = (p_size.x > 0 && p_size.y > 0) ? p_size : p_default_size;

	if (p_viewport_rid.is_valid()) {
		viewport_rid = p_viewport_rid;
		owns_viewport = false;
	}
	else {
		viewport_rid = Renderer::viewport_create();
		owns_viewport = true;
	}

	canvas_rid = Renderer::canvas_create();
	Renderer::viewport_attach_canvas(viewport_rid, canvas_rid);
	Renderer::viewport_set_size(viewport_rid, window_size.x, window_size.y);
	Renderer::viewport_set_active(viewport_rid, true);
	Renderer::viewport_set_update_mode(viewport_rid, RSE::VIEWPORT_UPDATE_ALWAYS);
	Renderer::viewport_attach_to_screen(viewport_rid, Rect2(0, 0, window_size.x, window_size.y),
		DisplayServerEnums::MAIN_WINDOW_ID);

	canvas_item_rid = Renderer::canvas_item_create();
	Renderer::canvas_item_set_parent(canvas_item_rid, canvas_rid);
	Renderer::canvas_item_set_visible(canvas_item_rid, true);

	if (ThemeDB::is_initialized()) {
		font = ThemeDB::get_fallback_font();
	}

	if (p_handler && Input::is_initialized() && Input::data) {
		prev_input_dispatch = Input::data->event_dispatch_function;
		Input::set_event_dispatch_function(p_handler);
	}

	needs_redraw = true;
}

void UIContext::teardown(Input::EventDispatchFunc p_handler)
{
	if (p_handler && Input::is_initialized() && Input::data &&
		Input::data->event_dispatch_function == p_handler) {
		Input::set_event_dispatch_function(prev_input_dispatch);
	}
	prev_input_dispatch = nullptr;

	if (canvas_item_rid.is_valid()) {
		Renderer::free_rid(canvas_item_rid);
		canvas_item_rid = RID();
	}
	if (canvas_rid.is_valid()) {
		Renderer::free_rid(canvas_rid);
		canvas_rid = RID();
	}
	if (owns_viewport && viewport_rid.is_valid()) {
		Renderer::free_rid(viewport_rid);
		viewport_rid = RID();
	}
	owns_viewport = false;
}

void UIContext::forward_input(const InputEvent& p_event, Input::EventDispatchFunc p_handler) const
{
	if (prev_input_dispatch && prev_input_dispatch != p_handler) {
		prev_input_dispatch(p_event);
	}
}

bool UIContext::sync_window_size()
{
	if (!DisplayServer::is_initialized()) {
		return false;
	}
	Size2i ds_size = DisplayServer::window_get_size(DisplayServerEnums::MAIN_WINDOW_ID);
	if (ds_size.x > 0 && ds_size.y > 0 && ds_size != window_size) {
		window_size = ds_size;
		Renderer::viewport_set_size(viewport_rid, window_size.x, window_size.y);
		Renderer::viewport_attach_to_screen(viewport_rid, Rect2(0, 0, window_size.x, window_size.y),
			DisplayServerEnums::MAIN_WINDOW_ID);
		needs_redraw = true;
		return true;
	}
	return false;
}

RID UIContext::begin_draw() const
{
	if (!canvas_item_rid.is_valid()) {
		return RID();
	}
	Renderer::canvas_item_clear(canvas_item_rid);
	return canvas_item_rid;
}

void UIButtonGroup::clear_rects()
{
	for (uint32_t i = 0; i < items.size(); i++) {
		items[i].rect = Rect2();
	}
}

void UIButtonGroup::add(const String& p_label, void (*p_on_click)(), bool (*p_is_enabled)(),
	bool p_primary, bool p_active_tab)
{
	items.push_back(
		{p_label, Rect2(), p_on_click, p_is_enabled, false, false, p_primary, p_active_tab});
}

bool UIButtonGroup::update_hover(Point2 p_mouse)
{
	bool changed = false;
	for (uint32_t i = 0; i < items.size(); i++) {
		bool h = items[i].rect.has_point(p_mouse);
		if (items[i].hovered != h) {
			items[i].hovered = h;
			changed = true;
		}
	}
	return changed;
}

bool UIButtonGroup::press(Point2 p_mouse)
{
	bool any = false;
	for (uint32_t i = 0; i < items.size(); i++) {
		UIButton& b = items[i];
		b.hovered = b.rect.has_point(p_mouse);
		const bool enabled = (b.is_enabled == nullptr) || b.is_enabled();
		b.pressed = b.hovered && enabled;
		any |= b.pressed;
	}
	return any;
}

void (*UIButtonGroup::release(Point2 p_mouse))()
{
	void (*to_invoke)() = nullptr;
	for (uint32_t i = 0; i < items.size(); i++) {
		UIButton& b = items[i];
		const bool was_pressed = b.pressed;
		b.pressed = false;
		b.hovered = b.rect.has_point(p_mouse);
		const bool enabled = (b.is_enabled == nullptr) || b.is_enabled();
		if (was_pressed && b.hovered && enabled && b.on_click) {
			to_invoke = b.on_click;
		}
	}
	return to_invoke;
}

void UIScrollView::update_metrics(float p_content_h, float p_view_h)
{
	max_scroll = MAX(0.0f, p_content_h - p_view_h);
	offset = CLAMP(offset, 0.0f, max_scroll);
}

bool UIScrollView::handle_wheel(MouseButton p_btn, float p_step)
{
	if (p_btn == MouseButton::WHEEL_UP) {
		float next = MAX(0.0f, offset - p_step);
		if (next != offset) {
			offset = next;
			return true;
		}
	}
	else if (p_btn == MouseButton::WHEEL_DOWN) {
		float next = MIN(max_scroll, offset + p_step);
		if (next != offset) {
			offset = next;
			return true;
		}
	}
	return false;
}

void UIScrollView::draw_scrollbar(RID p_ci, float p_track_x, float p_top_y, float p_view_h,
	float p_content_h, const Color& p_accent) const
{
	if (max_scroll <= 0.0f || p_view_h <= 12.0f || p_content_h <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(
		p_ci, Rect2(p_track_x, p_top_y, 4.0f, p_view_h), Color(0.08f, 0.09f, 0.11f, 1.0f));
	float thumb_h = MAX(20.0f, p_view_h * (p_view_h / p_content_h));
	float thumb_y = p_top_y + (p_view_h - thumb_h) * (offset / max_scroll);
	Renderer::canvas_item_add_rect(p_ci, Rect2(p_track_x, thumb_y, 4.0f, thumb_h), p_accent);
}

void UISplitLayout::compute_sidebar_split(const Rect2& p_area, float p_min_width_for_sidebar,
	float p_max_sidebar_w, float p_sidebar_ratio, float p_gap)
{
	show_secondary = p_area.size.x >= p_min_width_for_sidebar;
	const float gap = show_secondary ? p_gap : 0.0f;
	const float side_w =
		show_secondary ? MIN(p_max_sidebar_w, p_area.size.x * p_sidebar_ratio) : 0.0f;
	const float prim_w = MAX(0.0f, p_area.size.x - side_w - gap);

	primary_rect = Rect2(p_area.position.x, p_area.position.y, prim_w, p_area.size.y);
	secondary_rect = show_secondary ? Rect2(p_area.position.x + prim_w + gap, p_area.position.y,
										  side_w, p_area.size.y)
									: Rect2();
}

void UISplitLayout::compute_editor_workspace(const Rect2& p_area, float p_gap)
{
	show_bottom = p_area.size.y >= 240.0f;
	const float bot_gap = show_bottom ? p_gap : 0.0f;
	const float bot_h = show_bottom ? MIN(124.0f, p_area.size.y * 0.24f) : 0.0f;
	const float main_h = MAX(0.0f, p_area.size.y - bot_h - bot_gap);

	show_left = p_area.size.x >= 620.0f;
	show_right = p_area.size.x >= 820.0f;

	const float left_w = show_left ? MIN(230.0f, p_area.size.x * 0.23f) : 0.0f;
	const float right_w = show_right ? MIN(250.0f, p_area.size.x * 0.25f) : 0.0f;
	const float center_x = p_area.position.x + (show_left ? left_w + p_gap : 0.0f);
	const float center_w = MAX(0.0f, p_area.size.x - (show_left ? left_w + p_gap : 0.0f) -
										 (show_right ? right_w + p_gap : 0.0f));

	left_rect = show_left ? Rect2(p_area.position.x, p_area.position.y, left_w, main_h) : Rect2();
	center_rect = Rect2(center_x, p_area.position.y, center_w, main_h);
	right_rect = show_right ? Rect2(center_x + center_w + p_gap, p_area.position.y, right_w, main_h)
							: Rect2();
	bottom_rect = show_bottom ? Rect2(p_area.position.x, p_area.position.y + main_h + bot_gap,
									p_area.size.x, bot_h)
							  : Rect2();
}

namespace UIDraw
{
float centered_baseline_y(const Ref<Font>& p_font, const Rect2& p_rect, int p_font_size)
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

void draw_bordered_rect(
	RID p_ci, const Rect2& p_rect, const Color& p_fill, const Color& p_border, float p_border_w)
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

void draw_header_banner(
	RID p_ci, const Rect2& p_rect, const Color& p_bg, const Color& p_accent, float p_strip_w)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_bg);
	Renderer::canvas_item_add_rect(p_ci,
		Rect2(p_rect.position.x, p_rect.position.y, MIN(p_strip_w, p_rect.size.x), p_rect.size.y),
		p_accent);
}

void draw_dock_panel(RID p_ci, const Ref<Font>& p_font, const Rect2& p_rect, const String& p_title,
	const Color& p_dock_bg, const Color& p_hdr_bg, const Color& p_title_col, float p_hdr_h,
	float p_right_reserve)
{
	if (p_rect.size.x <= 0.0f || p_rect.size.y <= 0.0f) {
		return;
	}
	Renderer::canvas_item_add_rect(p_ci, p_rect, p_dock_bg);
	Rect2 hdr(p_rect.position.x, p_rect.position.y, p_rect.size.x, MIN(p_hdr_h, p_rect.size.y));
	Renderer::canvas_item_add_rect(p_ci, hdr, p_hdr_bg);
	if (p_font.is_valid()) {
		float max_w = MAX(0.0f, p_rect.size.x - 10.0f - p_right_reserve);
		if (max_w > 8.0f) {
			p_font->draw_string(p_ci,
				Point2(p_rect.position.x + 10.0f, centered_baseline_y(p_font, hdr, 12)), p_title,
				HORIZONTAL_ALIGNMENT_LEFT, max_w, 12, p_title_col);
		}
	}
}

void draw_button(RID p_ci, const Ref<Font>& p_font, const UIButton& p_btn, int p_font_size)
{
	if (p_btn.rect.size.x <= 2.0f || p_btn.rect.size.y <= 2.0f) {
		return;
	}
	const bool enabled = (p_btn.is_enabled == nullptr) || p_btn.is_enabled();

	Color bg(0.19f, 0.23f, 0.29f, 1.0f);
	Color accent(0.0f, 0.75f, 0.95f, 1.0f);
	Color text_col(0.92f, 0.94f, 0.96f, 1.0f);

	if (!enabled) {
		bg = Color(0.13f, 0.15f, 0.18f, 1.0f);
		accent = Color(0.20f, 0.24f, 0.29f, 1.0f);
		text_col = Color(0.45f, 0.49f, 0.55f, 1.0f);
	}
	else if (p_btn.primary) {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.0f, 0.50f, 0.68f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.06f, 0.68f, 0.88f, 1.0f);
		}
		else {
			bg = Color(0.02f, 0.58f, 0.78f, 1.0f);
		}
		accent = Color(0.45f, 0.92f, 1.0f, 1.0f);
		text_col = Color(0.98f, 0.99f, 1.0f, 1.0f);
	}
	else if (p_btn.active_tab) {
		bg = p_btn.hovered ? Color(0.24f, 0.34f, 0.44f, 1.0f) : Color(0.20f, 0.30f, 0.40f, 1.0f);
		accent = Color(0.0f, 0.85f, 1.0f, 1.0f);
	}
	else {
		if (p_btn.pressed && p_btn.hovered) {
			bg = Color(0.15f, 0.20f, 0.27f, 1.0f);
		}
		else if (p_btn.hovered) {
			bg = Color(0.27f, 0.34f, 0.44f, 1.0f);
		}
	}

	const float strip_h = p_btn.active_tab ? 3.0f : 2.0f;
	Renderer::canvas_item_add_rect(p_ci, p_btn.rect, bg);
	Renderer::canvas_item_add_rect(p_ci,
		Rect2(p_btn.rect.position.x, p_btn.rect.position.y + p_btn.rect.size.y - strip_h,
			p_btn.rect.size.x, strip_h),
		accent);

	if (p_font.is_valid() && p_btn.rect.size.x > 14.0f) {
		const float pad_x = 6.0f;
		const float avail_w = p_btn.rect.size.x - pad_x * 2.0f;
		const float base_y = centered_baseline_y(p_font, p_btn.rect, p_font_size);
		p_font->draw_string(p_ci, Point2(p_btn.rect.position.x + pad_x, base_y), p_btn.label,
			HORIZONTAL_ALIGNMENT_CENTER, avail_w, p_font_size, text_col);
	}
}

bool clip_segment(const Rect2& p_bounds, Point2& r_a, Point2& r_b)
{
	const float xmin = p_bounds.position.x;
	const float ymin = p_bounds.position.y;
	const float xmax = p_bounds.position.x + p_bounds.size.x;
	const float ymax = p_bounds.position.y + p_bounds.size.y;

	auto outcode = [&](const Point2& p) -> int {
		int code = 0;
		if (p.x < xmin) {
			code |= 1;
		}
		else if (p.x > xmax) {
			code |= 2;
		}
		if (p.y < ymin) {
			code |= 4;
		}
		else if (p.y > ymax) {
			code |= 8;
		}
		return code;
	};

	int c0 = outcode(r_a);
	int c1 = outcode(r_b);
	for (int iter = 0; iter < 12; iter++) {
		if (!(c0 | c1)) {
			return true;
		}
		if (c0 & c1) {
			return false;
		}
		int cout = c0 ? c0 : c1;
		Point2 p;
		if (cout & 8) {
			p.x = r_a.x + (r_b.x - r_a.x) * (ymax - r_a.y) / (r_b.y - r_a.y);
			p.y = ymax;
		}
		else if (cout & 4) {
			p.x = r_a.x + (r_b.x - r_a.x) * (ymin - r_a.y) / (r_b.y - r_a.y);
			p.y = ymin;
		}
		else if (cout & 2) {
			p.y = r_a.y + (r_b.y - r_a.y) * (xmax - r_a.x) / (r_b.x - r_a.x);
			p.x = xmax;
		}
		else {
			p.y = r_a.y + (r_b.y - r_a.y) * (xmin - r_a.x) / (r_b.x - r_a.x);
			p.x = xmin;
		}
		if (cout == c0) {
			r_a = p;
			c0 = outcode(r_a);
		}
		else {
			r_b = p;
			c1 = outcode(r_b);
		}
	}
	return false;
}

void draw_clipped_line(
	RID p_ci, const Rect2& p_clip, Point2 p_a, Point2 p_b, const Color& p_col, float p_width)
{
	if (clip_segment(p_clip, p_a, p_b)) {
		Renderer::canvas_item_add_line(p_ci, p_a, p_b, p_col, p_width, true);
	}
}

bool read_voltaire_project(const String& p_dir, VoltaireProjectFile* r_out)
{
	if (!r_out) {
		return false;
	}
	String proj_file = p_dir.path_join("project.vltr");
	Ref<FileAccess> fa = FileAccess::open(proj_file, FileAccess::READ);
	if (fa.is_null() || fa->get_length() < (int64_t)sizeof(VoltaireProjectFile)) {
		return false;
	}
	fa->get_buffer((uint8_t*)r_out, sizeof(VoltaireProjectFile));
	return vltr_project_valid(r_out);
}

String read_project_name(const String& p_dir)
{
	VoltaireProjectFile proj;
	if (read_voltaire_project(p_dir, &proj) && proj.config.project_name[0] != '\0') {
		return String::utf8(proj.config.project_name);
	}
	String fallback = p_dir.get_file();
	return fallback.is_empty() ? p_dir : fallback;
}
} // namespace UIDraw

// ============================================================================
// Merged from editor/doc/doc_tools.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/compression.h"
#include "core/io/dir_access.h"
#include "core/io/resource_importer.h"
#include "core/io/xml_parser.h"
#include "core/string/translation_server.h"
#include "editor/editor_ui.h"
#include "editor/export/editor_export_platform.h"
#include "editor/settings/editor_settings.h"
#include "scene/property_list_helper.h"
#include "scene/resources/theme.h"
#include "scene/theme/theme_db.h"

static String _get_indent(const String& p_text)
{
	String indent;
	bool has_text = false;
	int line_start = 0;

	for (int i = 0; i < p_text.length(); i++) {
		const char32_t c = p_text[i];
		if (c == '\n') {
			line_start = i + 1;
		}
		else if (c > 32) {
			has_text = true;
			indent = p_text.substr(line_start, i - line_start);
			break; // Indentation of the first line that has text.
		}
	}
	if (!has_text) {
		return p_text;
	}
	return indent;
}

static String _translate_doc_string(const String& p_text)
{
	const String indent = _get_indent(p_text);
	const String message = p_text.dedent().strip_edges();
	const String translated =
		TranslationServer::get_doc_domain()->translate(message, String());
	// No need to restore stripped edges because they'll be stripped again later.
	return translated.indent(indent);
}

Error DocTools::load_classes(const String& p_dir)
{
	Error err;
	Ref<DirAccess> da = DirAccess::open(p_dir, &err);
	if (da.is_null()) {
		return err;
	}

	da->list_dir_begin();
	String path;
	path = da->get_next();
	while (!path.is_empty()) {
		if (!da->current_is_dir() && path.ends_with("xml")) {
			Ref<XMLParser> parser = memnew(XMLParser);
			Error err2 = parser->open(p_dir.path_join(path));
			if (err2) {
				return err2;
			}

			_load(parser);
		}
		path = da->get_next();
	}

	da->list_dir_end();

	return OK;
}

Error DocTools::erase_classes(const String& p_dir)
{
	Error err;
	Ref<DirAccess> da = DirAccess::open(p_dir, &err);
	if (da.is_null()) {
		return err;
	}

	List<String> to_erase;

	da->list_dir_begin();
	String path;
	path = da->get_next();
	while (!path.is_empty()) {
		if (!da->current_is_dir() && path.ends_with("xml")) {
			to_erase.push_back(path);
		}
		path = da->get_next();
	}
	da->list_dir_end();

	while (to_erase.size()) {
		da->remove(to_erase.front()->get());
		to_erase.pop_front();
	}

	return OK;
}

static void _write_string(Ref<FileAccess> f, int p_tablevel, const String& p_string)
{
	if (p_string.is_empty()) {
		return;
	}
	String tab = String("\t").repeat(p_tablevel);
	f->store_string(tab + p_string + "\n");
}

Error DocTools::load_compressed(
	const uint8_t* p_data, int64_t p_compressed_size, int64_t p_uncompressed_size)
{
	Vector<uint8_t> data;
	data.resize(p_uncompressed_size);
	const int64_t ret = Compression::decompress(
		data.ptrw(), p_uncompressed_size, p_data, p_compressed_size, Compression::MODE_DEFLATE);
	ERR_FAIL_COND_V_MSG(ret == -1, ERR_FILE_CORRUPT, "Compressed file is corrupt.");

	Ref<XMLParser> parser = memnew(XMLParser);
	Error err = parser->open_buffer(data);
	if (err) {
		return err;
	}

	_load(parser);

	return OK;
}

Error DocTools::load_xml(const uint8_t* p_data, int64_t p_size)
{
	Ref<XMLParser> parser = memnew(XMLParser);
	Error err = parser->_open_buffer(p_data, p_size);
	if (err) {
		return err;
	}

	_load(parser);

	return OK;
}

Error DocTools::_load(Ref<XMLParser> parser) { return OK; }

void DocTools::remove_doc(const String& p_class_name) {}

bool DocTools::has_doc(const String& p_class_name) { return false; }

void DocTools::generate(unsigned int p_flags) {}

void DocTools::remove_script_doc_by_path(const String& p_path) {}

void DocTools::merge_from(const DocTools& p_data) {}

Error DocTools::save_classes(const String& p_default_path,
	const HashMap<String, String>& p_class_path, bool p_use_relative_schema)
{
	return OK;
}



// ============================================================================
// Merged from editor/project_upgrade/project_converter_3_to_4.cpp
// ============================================================================

#include "editor/editor_ui.h"

#ifndef DISABLE_DEPRECATED

#include "core/error/error_macros.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "core/os/time.h"
#include "core/templates/list.h"
#include "editor/project_upgrade/renames_map_3_to_4.h"
#include "modules/regex/regex.h"

// Find "OS.set_property(x)", capturing x into $1.
static String make_regex_gds_os_property_set(const String& name_set)
{
	return String("\\bOS\\.") + name_set + "\\s*\\((.*)\\)";
}

// Find "OS.property = x", capturing x into $1 or $2.
static String make_regex_gds_os_property_assign(const String& name)
{
	return String("\\bOS\\.") + name + "\\s*=\\s*([^#]+)";
}

// Find "OS.property" OR "OS.get_property()" / "OS.is_property()".
static String make_regex_gds_os_property_get(const String& name, const String& get)
{
	return String("\\bOS\\.(") + get + "_)?" + name + "(\\s*\\(\\s*\\))?";
}

class ProjectConverter3To4::RegExContainer
{
public:
	// Custom GDScript.
	RegEx reg_is_empty = RegEx("\\bempty\\(");
	RegEx reg_super = RegEx("([\t ])\\.([a-zA-Z_])");
	RegEx reg_json_to = RegEx("\\bto_json\\b");
	RegEx reg_json_parse = RegEx("([\t ]{0,})([^\n]+)parse_json\\(([^\n]+)");
	RegEx reg_json_non_new = RegEx("([\t ]{0,})([^\n]+)JSON\\.parse\\(([^\n]+)");
	RegEx reg_json_print = RegEx("\\bJSON\\b\\.print\\(");
	RegEx reg_export_simple = RegEx("export[ ]*\\(([a-zA-Z0-9_]+)\\)[ ]*var[ ]+([a-zA-Z0-9_]+)");
	RegEx reg_export_typed =
		RegEx("export[ ]*\\(([a-zA-Z0-9_]+)\\)[ ]*var[ ]+([a-zA-Z0-9_]+)[ ]*:[ ]*[a-zA-Z0-9_]+");
	RegEx reg_export_inferred_type =
		RegEx("export[ ]*\\([a-zA-Z0-9_]+\\)[ ]*var[ ]+([a-zA-Z0-9_]+)[ ]*:[ ]*=");
	RegEx reg_export_advanced =
		RegEx("export[ ]*\\(([^)^\n]+)\\)[ ]*var[ ]+([a-zA-Z0-9_]+)([^\n]+)");
	RegEx reg_setget_setget = RegEx("var[ ]+([a-zA-Z0-9_]+)([^\n]+?)[ \t]*setget[ "
									"\t]+([a-zA-Z0-9_]+)[ \t]*,[ \t]*([a-zA-Z0-9_]+)");
	RegEx reg_setget_set =
		RegEx("var[ ]+([a-zA-Z0-9_]+)([^\n]+?)[ \t]*setget[ \t]+([a-zA-Z0-9_]+)[ \t]*[,]*[^\n]*$");
	RegEx reg_setget_get =
		RegEx("var[ ]+([a-zA-Z0-9_]+)([^\n]+?)[ \t]*setget[ \t]+,[ \t]*([a-zA-Z0-9_]+)[ \t]*$");
	RegEx reg_join = RegEx("([\\(\\)a-zA-Z0-9_]+)\\.join\\(([^\n^\\)]+)\\)");
	RegEx reg_image_lock = RegEx("([a-zA-Z0-9_\\.]+)\\.lock\\(\\)");
	RegEx reg_image_unlock = RegEx("([a-zA-Z0-9_\\.]+)\\.unlock\\(\\)");
	RegEx reg_instantiate = RegEx("\\.instance\\(([^\\)]*)\\)");
	// Simple OS properties with getters/setters.
	RegEx reg_os_current_screen = RegEx("\\bOS\\.((set_|get_)?)current_screen\\b");
	RegEx reg_os_min_window_size = RegEx("\\bOS\\.((set_|get_)?)min_window_size\\b");
	RegEx reg_os_max_window_size = RegEx("\\bOS\\.((set_|get_)?)max_window_size\\b");
	RegEx reg_os_window_position = RegEx("\\bOS\\.((set_|get_)?)window_position\\b");
	RegEx reg_os_window_size = RegEx("\\bOS\\.((set_|get_)?)window_size\\b");
	RegEx reg_os_getset_screen_orient = RegEx("\\bOS\\.(s|g)et_screen_orientation\\b");
	// OS property getters/setters for non trivial replacements.
	RegEx reg_os_set_window_resizable =
		RegEx(make_regex_gds_os_property_set("set_window_resizable"));
	RegEx reg_os_assign_window_resizable =
		RegEx(make_regex_gds_os_property_assign("window_resizable"));
	RegEx reg_os_is_window_resizable =
		RegEx(make_regex_gds_os_property_get("window_resizable", "is"));
	RegEx reg_os_set_fullscreen = RegEx(make_regex_gds_os_property_set("set_window_fullscreen"));
	RegEx reg_os_assign_fullscreen = RegEx(make_regex_gds_os_property_assign("window_fullscreen"));
	RegEx reg_os_is_fullscreen = RegEx(make_regex_gds_os_property_get("window_fullscreen", "is"));
	RegEx reg_os_set_maximized = RegEx(make_regex_gds_os_property_set("set_window_maximized"));
	RegEx reg_os_assign_maximized = RegEx(make_regex_gds_os_property_assign("window_maximized"));
	RegEx reg_os_is_maximized = RegEx(make_regex_gds_os_property_get("window_maximized", "is"));
	RegEx reg_os_set_minimized = RegEx(make_regex_gds_os_property_set("set_window_minimized"));
	RegEx reg_os_assign_minimized = RegEx(make_regex_gds_os_property_assign("window_minimized"));
	RegEx reg_os_is_minimized = RegEx(make_regex_gds_os_property_get("window_minimized", "is"));
	RegEx reg_os_set_vsync = RegEx(make_regex_gds_os_property_set("set_use_vsync"));
	RegEx reg_os_assign_vsync = RegEx(make_regex_gds_os_property_assign("vsync_enabled"));
	RegEx reg_os_is_vsync = RegEx(make_regex_gds_os_property_get("vsync_enabled", "is"));
	// OS properties specific cases & specific replacements.
	RegEx reg_os_assign_screen_orient =
		RegEx("^(\\s*)OS\\.screen_orientation\\s*=\\s*([^#]+)"); // $1 - indent, $2 - value
	RegEx reg_os_set_always_on_top =
		RegEx(make_regex_gds_os_property_set("set_window_always_on_top"));
	RegEx reg_os_is_always_on_top = RegEx("\\bOS\\.is_window_always_on_top\\s*\\(.*\\)");
	RegEx reg_os_set_borderless = RegEx(make_regex_gds_os_property_set("set_borderless_window"));
	RegEx reg_os_get_borderless = RegEx("\\bOS\\.get_borderless_window\\s*\\(\\s*\\)");
	RegEx reg_os_screen_orient_enum =
		RegEx("\\bOS\\.SCREEN_ORIENTATION_(\\w+)\\b"); // $1 - constant suffix

	// GDScript keywords.
	RegEx keyword_gdscript_tool = RegEx("^tool");
	RegEx keyword_gdscript_export_single = RegEx("^export");
	RegEx keyword_gdscript_export_multi = RegEx("([\t]+)export\\b");
	RegEx keyword_gdscript_onready = RegEx("^onready");
	RegEx keyword_gdscript_remote = RegEx("^remote func");
	RegEx keyword_gdscript_remotesync = RegEx("^remotesync func");
	RegEx keyword_gdscript_sync = RegEx("^sync func");
	RegEx keyword_gdscript_slave = RegEx("^slave func");
	RegEx keyword_gdscript_puppet = RegEx("^puppet func");
	RegEx keyword_gdscript_puppetsync = RegEx("^puppetsync func");
	RegEx keyword_gdscript_master = RegEx("^master func");
	RegEx keyword_gdscript_mastersync = RegEx("^mastersync func");

	RegEx gdscript_comment = RegEx("^\\s*#");
	RegEx csharp_comment = RegEx("^\\s*\\/\\/");

	// CSharp keywords.
	RegEx keyword_csharp_remote = RegEx("\\[Remote(Attribute)?(\\(\\))?\\]");
	RegEx keyword_csharp_remotesync = RegEx("\\[(Remote)?Sync(Attribute)?(\\(\\))?\\]");
	RegEx keyword_csharp_puppet = RegEx("\\[(Puppet|Slave)(Attribute)?(\\(\\))?\\]");
	RegEx keyword_csharp_puppetsync = RegEx("\\[PuppetSync(Attribute)?(\\(\\))?\\]");
	RegEx keyword_csharp_master = RegEx("\\[Master(Attribute)?(\\(\\))?\\]");
	RegEx keyword_csharp_mastersync = RegEx("\\[MasterSync(Attribute)?(\\(\\))?\\]");

	// Colors.
	LocalVector<Ref<RegEx>> color_regexes;
	LocalVector<String> color_renamed;

	RegEx color_hexadecimal_short_constructor =
		RegEx("Color\\(\"#?([a-fA-F0-9]{1})([a-fA-F0-9]{3})\\b");
	RegEx color_hexadecimal_full_constructor =
		RegEx("Color\\(\"#?([a-fA-F0-9]{2})([a-fA-F0-9]{6})\\b");

	// Classes.
	LocalVector<Ref<RegEx>> class_tscn_regexes;
	LocalVector<Ref<RegEx>> class_gd_regexes;
	LocalVector<Ref<RegEx>> class_shader_regexes;

	// Keycode.
	RegEx input_map_keycode = RegEx("\\b,\"((physical_)?)scancode\":(\\d+)\\b");

	// Button index and joypad axis.
	RegEx joypad_button_index =
		RegEx("\\b,\"button_index\":(\\d+),(\"pressure\":\\d+\\.\\d+,\"pressed\":(false|true))\\b");
	RegEx joypad_axis = RegEx("\\b,\"axis\":(\\d+)\\b");

	// Index represents Godot 3's value, entry represents Godot 4 value equivalency.
	// i.e: Button4(L1 - Godot3) -> joypad_button_mappings[4]=9 -> Button9(L1 - Godot4).
	int joypad_button_mappings[23] = {0, 1, 2, 3, 9, 10, -1 /*L2*/, -1 /*R2*/, 7, 8, 4, 6, 11, 12,
		13, 14, 5, 15, 16, 17, 18, 19, 20};
	// Entries for L2 and R2 are -1 since they match to joypad axes and no longer to joypad buttons
	// in Godot 4.

	// Animation suffixes.
	RegEx animation_suffix = RegEx("([\"'])([a-zA-Z0-9_-]+)(-(?:loop|cycle))([\"'])");

	LocalVector<Ref<RegEx>> class_regexes;

	RegEx class_temp_tscn = RegEx("\\bTEMP_RENAMED_CLASS.tscn\\b");
	RegEx class_temp_gd = RegEx("\\bTEMP_RENAMED_CLASS.gd\\b");
	RegEx class_temp_shader = RegEx("\\bTEMP_RENAMED_CLASS.shader\\b");

	LocalVector<String> class_temp_tscn_renames;
	LocalVector<String> class_temp_gd_renames;
	LocalVector<String> class_temp_shader_renames;

	// Common.
	LocalVector<Ref<RegEx>> enum_regexes;
	LocalVector<Ref<RegEx>> gdscript_function_regexes;
	LocalVector<Ref<RegEx>> project_settings_regexes;
	LocalVector<Ref<RegEx>> project_godot_regexes;
	LocalVector<Ref<RegEx>> input_map_regexes;
	LocalVector<Ref<RegEx>> gdscript_properties_regexes;
	LocalVector<Ref<RegEx>> gdscript_signals_regexes;
	LocalVector<Ref<RegEx>> shaders_regexes;
	LocalVector<Ref<RegEx>> builtin_types_regexes;
	LocalVector<Ref<RegEx>> theme_override_regexes;
	LocalVector<Ref<RegEx>> csharp_function_regexes;
	LocalVector<Ref<RegEx>> csharp_properties_regexes;
	LocalVector<Ref<RegEx>> csharp_signal_regexes;

	RegExContainer()
	{
		// Common.
		{
			// Enum.
			for (unsigned int current_index = 0; RenamesMap3To4::enum_renames[current_index][0];
				 current_index++) {
				enum_regexes.push_back(memnew(
					RegEx(String("\\b") + RenamesMap3To4::enum_renames[current_index][0] + "\\b")));
			}
			// GDScript functions.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::gdscript_function_renames[current_index][0]; current_index++) {
				gdscript_function_regexes.push_back(memnew(
					RegEx(String("\\b") +
						  RenamesMap3To4::gdscript_function_renames[current_index][0] + "\\b")));
			}
			// Project Settings in scripts.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::project_settings_renames[current_index][0]; current_index++) {
				project_settings_regexes.push_back(memnew(
					RegEx(String("\\b") +
						  RenamesMap3To4::project_settings_renames[current_index][0] + "\\b")));
			}
			// Project Settings in project.vltr.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::project_godot_renames[current_index][0]; current_index++) {
				project_godot_regexes.push_back(
					memnew(RegEx(String("\\b") +
								 RenamesMap3To4::project_godot_renames[current_index][0] + "\\b")));
			}
			// Input Map.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::input_map_renames[current_index][0]; current_index++) {
				input_map_regexes.push_back(memnew(RegEx(
					String("\\b") + RenamesMap3To4::input_map_renames[current_index][0] + "\\b")));
			}
			// GDScript properties.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::gdscript_properties_renames[current_index][0]; current_index++) {
				gdscript_properties_regexes.push_back(memnew(
					RegEx(String("\\b") +
						  RenamesMap3To4::gdscript_properties_renames[current_index][0] + "\\b")));
			}
			// GDScript Signals.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::gdscript_signals_renames[current_index][0]; current_index++) {
				gdscript_signals_regexes.push_back(memnew(
					RegEx(String("\\b") +
						  RenamesMap3To4::gdscript_signals_renames[current_index][0] + "\\b")));
			}
			// Shaders.
			for (unsigned int current_index = 0; RenamesMap3To4::shaders_renames[current_index][0];
				 current_index++) {
				shaders_regexes.push_back(memnew(RegEx(
					String("\\b") + RenamesMap3To4::shaders_renames[current_index][0] + "\\b")));
			}
			// Builtin types.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::builtin_types_renames[current_index][0]; current_index++) {
				builtin_types_regexes.push_back(
					memnew(RegEx(String("\\b") +
								 RenamesMap3To4::builtin_types_renames[current_index][0] + "\\b")));
			}
			// Theme overrides.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::theme_override_renames[current_index][0]; current_index++) {
				theme_override_regexes.push_back(memnew(
					RegEx(String("\\b") + RenamesMap3To4::theme_override_renames[current_index][0] +
						  "\\b")));
			}
			// CSharp function renames.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::csharp_function_renames[current_index][0]; current_index++) {
				csharp_function_regexes.push_back(memnew(
					RegEx(String("\\b") +
						  RenamesMap3To4::csharp_function_renames[current_index][0] + "\\b")));
			}
			// CSharp properties renames.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::csharp_properties_renames[current_index][0]; current_index++) {
				csharp_properties_regexes.push_back(memnew(
					RegEx(String("\\b") +
						  RenamesMap3To4::csharp_properties_renames[current_index][0] + "\\b")));
			}
			// CSharp signals renames.
			for (unsigned int current_index = 0;
				 RenamesMap3To4::csharp_signals_renames[current_index][0]; current_index++) {
				csharp_signal_regexes.push_back(memnew(
					RegEx(String("\\b") + RenamesMap3To4::csharp_signals_renames[current_index][0] +
						  "\\b")));
			}
		}

		// Colors.
		{
			for (unsigned int current_index = 0; RenamesMap3To4::color_renames[current_index][0];
				 current_index++) {
				color_regexes.push_back(
					memnew(RegEx(String("\\bColor.") +
								 RenamesMap3To4::color_renames[current_index][0] + "\\b")));
				color_renamed.push_back(
					String("Color.") + RenamesMap3To4::color_renames[current_index][1]);
			}
		}
		// Classes.
		{
			for (unsigned int current_index = 0; RenamesMap3To4::class_renames[current_index][0];
				 current_index++) {
				const String class_name = RenamesMap3To4::class_renames[current_index][0];
				class_tscn_regexes.push_back(
					memnew(RegEx(String("\\b") + class_name + ".tscn\\b")));
				class_gd_regexes.push_back(memnew(RegEx(String("\\b") + class_name + ".gd\\b")));
				class_shader_regexes.push_back(
					memnew(RegEx(String("\\b") + class_name + ".shader\\b")));

				class_regexes.push_back(memnew(RegEx(String("\\b") + class_name + "\\b")));

				class_temp_tscn_renames.push_back(class_name + ".tscn");
				class_temp_gd_renames.push_back(class_name + ".gd");
				class_temp_shader_renames.push_back(class_name + ".shader");
			}
		}
	}
};

ProjectConverter3To4::ProjectConverter3To4(int p_maximum_file_size_kb, int p_maximum_line_length)
{
	maximum_file_size = p_maximum_file_size_kb * 1024;
	maximum_line_length = p_maximum_line_length;
}

// Collect files which will be checked, excluding ".txt", ".mp4", ".wav" etc. files.
Vector<String> ProjectConverter3To4::check_for_files()
{
	Vector<String> collected_files = Vector<String>();

	Vector<String> directories_to_check = Vector<String>();
	directories_to_check.push_back("res://");

	while (!directories_to_check.is_empty()) {
		String path = directories_to_check.get(
			directories_to_check.size() - 1); // Is there any pop_back function?
		directories_to_check.resize(directories_to_check.size() - 1); // Remove last element

		Ref<DirAccess> dir = DirAccess::open(path);
		if (dir.is_valid()) {
			dir->set_include_hidden(true);
			dir->list_dir_begin();
			String current_dir = dir->get_current_dir();
			String file_name = dir->_get_next();

			while (file_name != "") {
				if (file_name == ".git" || file_name == ".godot") {
					file_name = dir->_get_next();
					continue;
				}
				if (dir->current_is_dir()) {
					directories_to_check.append(current_dir.path_join(file_name) + "/");
				}
				else {
					bool proper_extension = false;
					if (file_name.ends_with(".gd") || file_name.ends_with(".shader") ||
						file_name.ends_with(".gdshader") || file_name.ends_with(".tscn") ||
						file_name.ends_with(".tres") || file_name.ends_with(".godot") ||
						file_name.ends_with(".cs") || file_name.ends_with(".csproj") ||
						file_name.ends_with(".import")) {
						proper_extension = true;
					}

					if (proper_extension) {
						collected_files.append(current_dir.path_join(file_name));
					}
				}
				file_name = dir->_get_next();
			}
		}
		else {
			print_verbose("Failed to open " + path);
		}
	}
	return collected_files;
}

Vector<SourceLine> ProjectConverter3To4::split_lines(const String& text)
{
	Vector<String> lines = text.split("\n");
	Vector<SourceLine> source_lines;
	for (String& line : lines) {
		SourceLine source_line;
		source_line.line = line;
		source_line.is_comment = false;

		source_lines.append(source_line);
	}
	return source_lines;
}

// Test expected results of gdscript
bool ProjectConverter3To4::test_conversion_gdscript_builtin(const String& name,
	const String& expected,
	void (ProjectConverter3To4::*func)(Vector<SourceLine>&, const RegExContainer&, bool),
	const String& what, const RegExContainer& reg_container, bool builtin_script)
{
	Vector<SourceLine> got = split_lines(name);

	(this->*func)(got, reg_container, builtin_script);
	String got_str = collect_string_from_vector(got);
	ERR_FAIL_COND_V_MSG(expected != got_str, false,
		vformat("Failed to convert %s \"%s\" to \"%s\", got instead \"%s\"", what, name, expected,
			got_str));

	return true;
}

bool ProjectConverter3To4::test_conversion_with_regex(const String& name, const String& expected,
	void (ProjectConverter3To4::*func)(Vector<SourceLine>&, const RegExContainer&),
	const String& what, const RegExContainer& reg_container)
{
	Vector<SourceLine> got = split_lines(name);

	(this->*func)(got, reg_container);
	String got_str = collect_string_from_vector(got);
	ERR_FAIL_COND_V_MSG(expected != got_str, false,
		vformat("Failed to convert %s \"%s\" to \"%s\", got instead \"%s\"", what, name, expected,
			got_str));

	return true;
}

bool ProjectConverter3To4::test_conversion_basic(const String& name, const String& expected,
	const char* array[][2], LocalVector<Ref<RegEx>>& regex_cache, const String& what)
{
	Vector<SourceLine> got = split_lines(name);

	rename_common(array, regex_cache, got);
	String got_str = collect_string_from_vector(got);
	ERR_FAIL_COND_V_MSG(expected != got_str, false,
		vformat("Failed to convert %s \"%s\" to \"%s\", got instead \"%s\"", what, name, expected,
			got_str));

	return true;
}

// Validate if conversions are proper.
bool ProjectConverter3To4::test_conversion(RegExContainer& reg_container)
{
	bool valid = true;

	valid = valid &&
			test_conversion_with_regex("tool", "@tool", &ProjectConverter3To4::fix_tool_declaration,
				"gdscript keyword", reg_container);
	valid = valid &&
			test_conversion_with_regex("\n    tool", "\n    tool",
				&ProjectConverter3To4::fix_tool_declaration, "gdscript keyword", reg_container);
	valid = valid &&
			test_conversion_with_regex("\n\ntool", "@tool\n\n",
				&ProjectConverter3To4::fix_tool_declaration, "gdscript keyword", reg_container);

	valid = valid && test_conversion_with_regex("pause_mode = 2", "pause_mode = 3",
						 &ProjectConverter3To4::fix_pause_mode, "pause_mode", reg_container);
	valid = valid && test_conversion_with_regex("pause_mode = 1", "pause_mode = 1",
						 &ProjectConverter3To4::fix_pause_mode, "pause_mode", reg_container);
	valid = valid && test_conversion_with_regex("pause_mode = 3", "pause_mode = 3",
						 &ProjectConverter3To4::fix_pause_mode, "pause_mode", reg_container);
	valid = valid && test_conversion_with_regex("somepause_mode = 2", "somepause_mode = 2",
						 &ProjectConverter3To4::fix_pause_mode, "pause_mode", reg_container);
	valid = valid && test_conversion_with_regex("pause_mode_ext = 2", "pause_mode_ext = 2",
						 &ProjectConverter3To4::fix_pause_mode, "pause_mode", reg_container);

	valid = valid && test_conversion_basic("TYPE_REAL", "TYPE_FLOAT", RenamesMap3To4::enum_renames,
						 reg_container.enum_regexes, "enum");

	valid = valid && test_conversion_basic("can_instance", "can_instantiate",
						 RenamesMap3To4::gdscript_function_renames,
						 reg_container.gdscript_function_regexes, "gdscript function");

	valid = valid && test_conversion_basic("CanInstance", "CanInstantiate",
						 RenamesMap3To4::csharp_function_renames,
						 reg_container.csharp_function_regexes, "csharp function");

	valid = valid && test_conversion_basic("translation", "position",
						 RenamesMap3To4::gdscript_properties_renames,
						 reg_container.gdscript_properties_regexes, "gdscript property");

	valid = valid && test_conversion_basic("Translation", "Position",
						 RenamesMap3To4::csharp_properties_renames,
						 reg_container.csharp_properties_regexes, "csharp property");

	valid = valid && test_conversion_basic("NORMALMAP", "NORMAL_MAP",
						 RenamesMap3To4::shaders_renames, reg_container.shaders_regexes, "shader");

	valid = valid && test_conversion_basic("text_entered", "text_submitted",
						 RenamesMap3To4::gdscript_signals_renames,
						 reg_container.gdscript_signals_regexes, "gdscript signal");

	valid = valid && test_conversion_basic("TextEntered", "TextSubmitted",
						 RenamesMap3To4::csharp_signals_renames,
						 reg_container.csharp_signal_regexes, "csharp signal");

	valid = valid && test_conversion_basic("audio/channel_disable_threshold_db",
						 "audio/buses/channel_disable_threshold_db",
						 RenamesMap3To4::project_settings_renames,
						 reg_container.project_settings_regexes, "project setting");

	valid =
		valid &&
		test_conversion_basic(
			"\"device\":-1,\"alt\":false,\"shift\":false,\"control\":false,\"meta\":false,"
			"\"doubleclick\":false,\"scancode\":0,\"physical_scancode\":16777254,\"script\":null",
			"\"device\":-1,\"alt_pressed\":false,\"shift_pressed\":false,\"ctrl_pressed\":false,"
			"\"meta_pressed\":false,\"double_click\":false,\"keycode\":0,\"physical_keycode\":"
			"16777254,\"script\":null",
			RenamesMap3To4::input_map_renames, reg_container.input_map_regexes, "input map");

	valid = valid &&
			test_conversion_basic("Transform", "Transform3D", RenamesMap3To4::builtin_types_renames,
				reg_container.builtin_types_regexes, "builtin type");

	valid = valid &&
			test_conversion_basic("custom_constants/margin_right",
				"theme_override_constants/margin_right", RenamesMap3To4::theme_override_renames,
				reg_container.theme_override_regexes, "theme overrides");

	// Custom Renames.

	valid = valid && test_conversion_with_regex("(Connect(A,B,C,D,E,F,G) != OK):",
						 "(Connect(A, new Callable(B, C), D, E, F, G) != OK):",
						 &ProjectConverter3To4::rename_csharp_functions, "custom rename csharp",
						 reg_container);
	valid = valid && test_conversion_with_regex(
						 "(Disconnect(A,B,C) != OK):", "(Disconnect(A, new Callable(B, C)) != OK):",
						 &ProjectConverter3To4::rename_csharp_functions, "custom rename csharp",
						 reg_container);
	valid = valid &&
			test_conversion_with_regex(
				"(IsConnected(A,B,C) != OK):", "(IsConnected(A, new Callable(B, C)) != OK):",
				&ProjectConverter3To4::rename_csharp_functions, "custom rename", reg_container);

	valid = valid && test_conversion_with_regex("[Remote]", "[RPC(MultiplayerAPI.RPCMode.AnyPeer)]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);
	valid = valid && test_conversion_with_regex("[RemoteSync]",
						 "[RPC(MultiplayerAPI.RPCMode.AnyPeer, CallLocal = true)]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);
	valid = valid && test_conversion_with_regex("[Sync]",
						 "[RPC(MultiplayerAPI.RPCMode.AnyPeer, CallLocal = true)]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);
	valid = valid && test_conversion_with_regex("[Slave]", "[RPC]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);
	valid = valid && test_conversion_with_regex("[Puppet]", "[RPC]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);
	valid = valid && test_conversion_with_regex("[PuppetSync]", "[RPC(CallLocal = true)]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);
	valid =
		valid &&
		test_conversion_with_regex("[Master]",
			"The master and mastersync rpc behavior is not officially supported anymore. Try using "
			"another keyword or making custom logic using Multiplayer.GetRemoteSenderId()\n[RPC]",
			&ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp", reg_container);
	valid = valid && test_conversion_with_regex("[MasterSync]",
						 "The master and mastersync rpc behavior is not officially supported "
						 "anymore. Try using another keyword or making custom logic using "
						 "Multiplayer.GetRemoteSenderId()\n[RPC(CallLocal = true)]",
						 &ProjectConverter3To4::rename_csharp_attributes, "custom rename csharp",
						 reg_container);

	valid = valid && test_conversion_gdscript_builtin("\tif OS.window_resizable: pass",
						 "\tif (not get_window().unresizable): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tif OS.is_window_resizable(): pass",
						 "\tif (not get_window().unresizable): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.set_window_resizable(Settings.resizable)",
					 "\tget_window().unresizable = not (Settings.resizable)",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tOS.window_resizable = Settings.resizable",
						 "\tget_window().unresizable = not (Settings.resizable)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("\tif OS.window_fullscreen: pass",
						 "\tif ((get_window().mode == Window.MODE_EXCLUSIVE_FULLSCREEN) or "
						 "(get_window().mode == Window.MODE_FULLSCREEN)): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tif OS.is_window_fullscreen(): pass",
						 "\tif ((get_window().mode == Window.MODE_EXCLUSIVE_FULLSCREEN) or "
						 "(get_window().mode == Window.MODE_FULLSCREEN)): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.set_window_fullscreen(Settings.fullscreen)",
					 "\tget_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if "
					 "(Settings.fullscreen) else Window.MODE_WINDOWED",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.window_fullscreen = Settings.fullscreen",
					 "\tget_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if "
					 "(Settings.fullscreen) else Window.MODE_WINDOWED",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("\tif OS.window_maximized: pass",
						 "\tif (get_window().mode == Window.MODE_MAXIMIZED): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tif OS.is_window_maximized(): pass",
						 "\tif (get_window().mode == Window.MODE_MAXIMIZED): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.set_window_maximized(Settings.maximized)",
					 "\tget_window().mode = Window.MODE_MAXIMIZED if (Settings.maximized) else "
					 "Window.MODE_WINDOWED",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tOS.window_maximized = Settings.maximized",
						 "\tget_window().mode = Window.MODE_MAXIMIZED if (Settings.maximized) else "
						 "Window.MODE_WINDOWED",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("\tif OS.window_minimized: pass",
						 "\tif (get_window().mode == Window.MODE_MINIMIZED): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tif OS.is_window_minimized(): pass",
						 "\tif (get_window().mode == Window.MODE_MINIMIZED): pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.set_window_minimized(Settings.minimized)",
					 "\tget_window().mode = Window.MODE_MINIMIZED if (Settings.minimized) else "
					 "Window.MODE_WINDOWED",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tOS.window_minimized = Settings.minimized",
						 "\tget_window().mode = Window.MODE_MINIMIZED if (Settings.minimized) else "
						 "Window.MODE_WINDOWED",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid =
		valid &&
		test_conversion_gdscript_builtin("\tif OS.vsync_enabled: pass",
			"\tif (DisplayServer.window_get_vsync_mode() != DisplayServer.VSYNC_DISABLED): pass",
			&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
			false);
	valid =
		valid &&
		test_conversion_gdscript_builtin("\tif OS.is_vsync_enabled(): pass",
			"\tif (DisplayServer.window_get_vsync_mode() != DisplayServer.VSYNC_DISABLED): pass",
			&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
			false);
	valid = valid && test_conversion_gdscript_builtin("\tOS.set_use_vsync(Settings.vsync)",
						 "\tDisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if "
						 "(Settings.vsync) else DisplayServer.VSYNC_DISABLED)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tOS.vsync_enabled = Settings.vsync",
						 "\tDisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if "
						 "(Settings.vsync) else DisplayServer.VSYNC_DISABLED)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin(
				"\tif OS.screen_orientation = OS.SCREEN_ORIENTATION_VERTICAL: pass",
				"\tif DisplayServer.screen_get_orientation() = DisplayServer.SCREEN_VERTICAL: pass",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid =
		valid &&
		test_conversion_gdscript_builtin(
			"\tif OS.get_screen_orientation() = OS.SCREEN_ORIENTATION_LANDSCAPE: pass",
			"\tif DisplayServer.screen_get_orientation() = DisplayServer.SCREEN_LANDSCAPE: pass",
			&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
			false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.set_screen_orientation(Settings.orient)",
					 "\tDisplayServer.screen_set_orientation(Settings.orient)",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tOS.screen_orientation = Settings.orient",
						 "\tDisplayServer.screen_set_orientation(Settings.orient)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("\tif OS.is_window_always_on_top(): pass",
						 "\tif get_window().always_on_top: pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("\tOS.set_window_always_on_top(Settings.alwaystop)",
				"\tget_window().always_on_top = (Settings.alwaystop)",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);

	valid = valid && test_conversion_gdscript_builtin("\tif OS.get_borderless_window(): pass",
						 "\tif get_window().borderless: pass",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\tOS.set_borderless_window(Settings.borderless)",
					 "\tget_window().borderless = (Settings.borderless)",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin(
						 "\tvar aa = roman(r.move_and_slide( a, b, c, d, e, f )) # Roman",
						 "\tr.set_velocity(a)\n\tr.set_up_direction(b)\n\tr.set_floor_stop_on_"
						 "slope_enabled(c)\n\tr.set_max_slides(d)\n\tr.set_floor_max_angle(e)\n\t# "
						 "TODOConverter3To4 infinite_inertia were removed in Godot 4 - previous "
						 "value `f`\n\tr.move_and_slide()\n\tvar aa = roman(r.velocity) # Roman",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid &&
		test_conversion_gdscript_builtin("\tmove_and_slide( a, b, c, d, e, f ) # Roman",
			"\tset_velocity(a)\n\tset_up_direction(b)\n\tset_floor_stop_on_slope_enabled(c)\n\tset_"
			"max_slides(d)\n\tset_floor_max_angle(e)\n\t# TODOConverter3To4 infinite_inertia were "
			"removed in Godot 4 - previous value `f`\n\tmove_and_slide() # Roman",
			&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
			false);
	valid =
		valid && test_conversion_gdscript_builtin(
					 "\tvar aa = roman(r.move_and_slide_with_snap( a, g, b, c, d, e, f )) # Roman",
					 "\tr.set_velocity(a)\n\t# TODOConverter3To4 looks that snap in Godot 4 is "
					 "float, not vector like in Godot 3 - previous value "
					 "`g`\n\tr.set_up_direction(b)\n\tr.set_floor_stop_on_slope_enabled(c)\n\tr."
					 "set_max_slides(d)\n\tr.set_floor_max_angle(e)\n\t# TODOConverter3To4 "
					 "infinite_inertia were removed in Godot 4 - previous value "
					 "`f`\n\tr.move_and_slide()\n\tvar aa = roman(r.velocity) # Roman",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin(
					 "\tmove_and_slide_with_snap( a, g, b, c, d, e, f ) # Roman",
					 "\tset_velocity(a)\n\t# TODOConverter3To4 looks that snap in Godot 4 is "
					 "float, not vector like in Godot 3 - previous value "
					 "`g`\n\tset_up_direction(b)\n\tset_floor_stop_on_slope_enabled(c)\n\tset_max_"
					 "slides(d)\n\tset_floor_max_angle(e)\n\t# TODOConverter3To4 infinite_inertia "
					 "were removed in Godot 4 - previous value `f`\n\tmove_and_slide() # Roman",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin("remove_and_slide(a,b,c,d,e,f)",
				"remove_and_slide(a,b,c,d,e,f)", &ProjectConverter3To4::rename_gdscript_functions,
				"custom rename", reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("list_dir_begin( a , b )",
						 "list_dir_begin() # TODOConverter3To4 fill missing arguments "
						 "https://github.com/godotengine/godot/pull/40547",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("list_dir_begin( a )",
						 "list_dir_begin() # TODOConverter3To4 fill missing arguments "
						 "https://github.com/godotengine/godot/pull/40547",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("list_dir_begin( )",
						 "list_dir_begin() # TODOConverter3To4 fill missing arguments "
						 "https://github.com/godotengine/godot/pull/40547",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin("sort_custom( a , b )", "sort_custom(Callable(a, b))",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);

	valid = valid && test_conversion_gdscript_builtin("func c(var a, var b)", "func c(a, b)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("draw_line(1, 2, 3, 4, 5)",
						 "draw_line(1, 2, 3, 4)", &ProjectConverter3To4::rename_gdscript_functions,
						 "custom rename", reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin("\timage.lock()",
				"\tfalse # image.lock() # TODOConverter3To4, Image no longer requires locking, "
				"`false` helps to not break one line if/else, so it can freely be removed",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid &&
			test_conversion_gdscript_builtin("\timage.unlock()",
				"\tfalse # image.unlock() # TODOConverter3To4, Image no longer requires locking, "
				"`false` helps to not break one line if/else, so it can freely be removed",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid &&
			test_conversion_gdscript_builtin("\troman.image.unlock()",
				"\tfalse # roman.image.unlock() # TODOConverter3To4, Image no longer requires "
				"locking, `false` helps to not break one line if/else, so it can freely be removed",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid && test_conversion_gdscript_builtin("\tmtx.lock()", "\tmtx.lock()",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\tmutex.unlock()", "\tmutex.unlock()",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_with_regex("extends CSGBox", "extends CSGBox3D",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);
	valid = valid && test_conversion_with_regex("CSGBox", "CSGBox3D",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);
	valid = valid && test_conversion_with_regex("Spatial", "Node3D",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);
	valid = valid && test_conversion_with_regex("Spatial.tscn", "Spatial.tscn",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);
	valid = valid && test_conversion_with_regex("Spatial.gd", "Spatial.gd",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);
	valid = valid && test_conversion_with_regex("Spatial.shader", "Spatial.shader",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);
	valid = valid && test_conversion_with_regex("Spatial.other", "Node3D.other",
						 &ProjectConverter3To4::rename_classes, "classes", reg_container);

	valid = valid && test_conversion_gdscript_builtin("\nonready", "\n@onready",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("onready", "@onready",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(" onready", " onready",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\nexport", "\n@export",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\texport", "\t@export",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\texport_dialog", "\texport_dialog",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("export", "@export",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(" export", " export",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("\n\nremote func", "\n\n@rpc(\"any_peer\") func",
					 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
					 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("\n\nremote func", "\n\n@rpc(\\\"any_peer\\\") func",
				&ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword", reg_container,
				true);
	valid = valid && test_conversion_gdscript_builtin("\n\nremotesync func",
						 "\n\n@rpc(\"any_peer\", \"call_local\") func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\n\nremotesync func",
						 "\n\n@rpc(\\\"any_peer\\\", \\\"call_local\\\") func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, true);
	valid = valid && test_conversion_gdscript_builtin("\n\nsync func",
						 "\n\n@rpc(\"any_peer\", \"call_local\") func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\n\nsync func",
						 "\n\n@rpc(\\\"any_peer\\\", \\\"call_local\\\") func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, true);
	valid = valid && test_conversion_gdscript_builtin("\n\nslave func", "\n\n@rpc func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\n\npuppet func", "\n\n@rpc func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("\n\npuppetsync func", "\n\n@rpc(\"call_local\") func",
				&ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword", reg_container,
				false);
	valid = valid && test_conversion_gdscript_builtin("\n\npuppetsync func",
						 "\n\n@rpc(\\\"call_local\\\") func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, true);
	valid = valid && test_conversion_gdscript_builtin("\n\nmaster func",
						 "\n\nThe master and mastersync rpc behavior is not officially supported "
						 "anymore. Try using another keyword or making custom logic using "
						 "get_multiplayer().get_remote_sender_id()\n@rpc func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\n\nmastersync func",
						 "\n\nThe master and mastersync rpc behavior is not officially supported "
						 "anymore. Try using another keyword or making custom logic using "
						 "get_multiplayer().get_remote_sender_id()\n@rpc(\"call_local\") func",
						 &ProjectConverter3To4::rename_gdscript_keywords, "gdscript keyword",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin(
						 "var size: Vector2 = Vector2() setget set_function, get_function",
						 "var size: Vector2 = Vector2(): get = get_function, set = set_function",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("var size: Vector2 = Vector2() setget set_function, ",
				"var size: Vector2 = Vector2(): set = set_function",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid &&
			test_conversion_gdscript_builtin("var size: Vector2 = Vector2() setget set_function",
				"var size: Vector2 = Vector2(): set = set_function",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid &&
			test_conversion_gdscript_builtin("var size: Vector2 = Vector2() setget , get_function",
				"var size: Vector2 = Vector2(): get = get_function",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);

	valid = valid && test_conversion_gdscript_builtin("get_node(@", "get_node(",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("yield(this, \"timeout\")",
						 "await this.timeout", &ProjectConverter3To4::rename_gdscript_functions,
						 "custom rename", reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("yield(this, \\\"timeout\\\")",
						 "await this.timeout", &ProjectConverter3To4::rename_gdscript_functions,
						 "custom rename", reg_container, true);

	valid =
		valid && test_conversion_gdscript_builtin(" Transform.xform(Vector3(a,b,c) + Vector3.UP) ",
					 " Transform * (Vector3(a,b,c) + Vector3.UP) ",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin(" Transform.xform_inv(Vector3(a,b,c) + Vector3.UP) ",
				" (Vector3(a,b,c) + Vector3.UP) * Transform ",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);

	valid = valid && test_conversion_gdscript_builtin("export(float) var lifetime = 3.0",
						 "export var lifetime: float = 3.0",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("export (int)var spaces=1",
					 "export var spaces: int=1", &ProjectConverter3To4::rename_gdscript_functions,
					 "custom rename", reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin(
				"export(String, 'AnonymousPro', 'CourierPrime') var _font_name = 'AnonymousPro'",
				"export var _font_name = 'AnonymousPro' # (String, 'AnonymousPro', 'CourierPrime')",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false); // TODO, this is only a workaround
	valid = valid && test_conversion_gdscript_builtin("export(PackedScene) var mob_scene",
						 "export var mob_scene: PackedScene",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("export(float) var lifetime: float = 3.0",
						 "export var lifetime: float = 3.0",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("export var lifetime: float = 3.0",
						 "export var lifetime: float = 3.0",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("export var lifetime := 3.0",
					 "export var lifetime := 3.0", &ProjectConverter3To4::rename_gdscript_functions,
					 "custom rename", reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("export(float) var lifetime := 3.0",
					 "export var lifetime := 3.0", &ProjectConverter3To4::rename_gdscript_functions,
					 "custom rename", reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("var d = parse_json(roman(sfs))",
						 "var test_json_conv = JSON.new()\ntest_json_conv.parse(roman(sfs))\nvar d "
						 "= test_json_conv.get_data()",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin("to_json( AA ) szon",
				"JSON.new().stringify( AA ) szon", &ProjectConverter3To4::rename_gdscript_functions,
				"custom rename", reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("s to_json", "s JSON.new().stringify",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("AF to_json2", "AF to_json2",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("var rr = JSON.parse(a)",
						 "var test_json_conv = JSON.new()\ntest_json_conv.parse(a)\nvar rr = "
						 "test_json_conv.get_data()",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("empty()", "is_empty()",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(".empty", ".empty",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin(").roman(", ").roman(",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\t.roman(", "\tsuper.roman(",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(" .roman(", " super.roman(",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(".1", ".1",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(" .1", " .1",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("'.'", "'.'",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("'.a'", "'.a'",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("\t._input(_event)", "\tsuper._input(_event)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin(
						 "(connect(A,B,C) != OK):", "(connect(A, Callable(B, C)) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(
						 "(connect(A,B,C,D) != OK):", "(connect(A, Callable(B, C).bind(D)) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(connect(A,B,C,[D]) != OK):",
						 "(connect(A, Callable(B, C).bind(D)) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(connect(A,B,C,[D,E]) != OK):",
						 "(connect(A, Callable(B, C).bind(D,E)) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(connect(A,B,C,[D,E],F) != OK):",
						 "(connect(A, Callable(B, C).bind(D,E), F) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(connect(A,B,C,D,E) != OK):",
						 "(connect(A, Callable(B, C).bind(D), E) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid =
		valid && test_conversion_gdscript_builtin(".connect(A,B,C)", ".connect(A, Callable(B, C))",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("abc.connect(A,B,C)", "abc.connect(A, Callable(B, C))",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid &&
			test_conversion_gdscript_builtin("\tconnect(A,B,C)", "\tconnect(A, Callable(B, C))",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid =
		valid && test_conversion_gdscript_builtin(" connect(A,B,C)", " connect(A, Callable(B, C))",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("_connect(A,B,C)", "_connect(A,B,C)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("do_connect(A,B,C)", "do_connect(A,B,C)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("$connect(A,B,C)", "$connect(A,B,C)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("@connect(A,B,C)", "@connect(A,B,C)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin("(start(A,B) != OK):",
				"(start(Callable(A, B)) != OK):", &ProjectConverter3To4::rename_gdscript_functions,
				"custom rename", reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("func start(A,B):", "func start(A,B):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(start(A,B,C,D,E,F,G) != OK):",
						 "(start(Callable(A, B).bind(C), D, E, F, G) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(
						 "disconnect(A,B,C) != OK):", "disconnect(A, Callable(B, C)) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(
						 "is_connected(A,B,C) != OK):", "is_connected(A, Callable(B, C)) != OK):",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("is_connected(A,B,C))",
						 "is_connected(A, Callable(B, C)))",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("(tween_method(A,B,C,D,E).foo())",
						 "(tween_method(Callable(A, B), C, D, E).foo())",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(tween_method(A,B,C,D,E,[F,G]).foo())",
						 "(tween_method(Callable(A, B).bind(F,G), C, D, E).foo())",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(tween_callback(A,B).foo())",
						 "(tween_callback(Callable(A, B)).foo())",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("(tween_callback(A,B,[C,D]).foo())",
						 "(tween_callback(Callable(A, B).bind(C,D)).foo())",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("func _init(", "func _init(",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(
						 "func _init(a,b,c).(d,e,f):", "func _init(a,b,c):\n\tsuper(d,e,f)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("func _init(a,b,c).(a.b(),c.d()):",
						 "func _init(a,b,c):\n\tsuper(a.b(),c.d())",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("func _init(p_x:int)->void:",
					 "func _init(p_x:int)->void:", &ProjectConverter3To4::rename_gdscript_functions,
					 "custom rename", reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("func _init(a: int).(d,e,f) -> void:",
						 "func _init(a: int) -> void:\n\tsuper(d,e,f)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("q_PackedDataContainer._iter_init(variable1)",
						 "q_PackedDataContainer._iter_init(variable1)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid =
		valid && test_conversion_gdscript_builtin("create_from_image(aa, bb)",
					 "create_from_image(aa) #,bb", &ProjectConverter3To4::rename_gdscript_functions,
					 "custom rename", reg_container, false);
	valid = valid && test_conversion_gdscript_builtin(
						 "q_ImageTexture.create_from_image(variable1, variable2)",
						 "q_ImageTexture.create_from_image(variable1) #,variable2",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("set_cell_item(a, b, c, d ,e) # AA",
						 "set_cell_item(Vector3(a, b, c), d, e) # AA",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("set_cell_item(a, b)", "set_cell_item(a, b)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("get_cell_item_orientation(a, b,c)",
						 "get_cell_item_orientation(Vector3i(a, b, c))",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("get_cell_item(a, b,c)",
						 "get_cell_item(Vector3i(a, b, c))",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("map_to_world(a, b,c)",
				"map_to_local(Vector3i(a, b, c))", &ProjectConverter3To4::rename_gdscript_functions,
				"custom rename", reg_container, false);

	valid = valid && test_conversion_gdscript_builtin("PackedStringArray(req_godot).join('.')",
						 "'.'.join(PackedStringArray(req_godot))",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("=PackedStringArray(req_godot).join('.')",
						 "='.'.join(PackedStringArray(req_godot))",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid &&
			test_conversion_gdscript_builtin("apply_force(position, impulse)",
				"apply_force(impulse, position)", &ProjectConverter3To4::rename_gdscript_functions,
				"custom rename", reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("apply_impulse(position, impulse)",
						 "apply_impulse(impulse, position)",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid &&
			test_conversion_gdscript_builtin("draw_rect(a,b,c,d,e).abc",
				"draw_rect(a, b, c, d).abc# e) TODOConverter3To4 Antialiasing argument is missing",
				&ProjectConverter3To4::rename_gdscript_functions, "custom rename", reg_container,
				false);
	valid = valid && test_conversion_gdscript_builtin("get_focus_owner()",
						 "get_viewport().gui_get_focus_owner()",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid =
		valid && test_conversion_gdscript_builtin("button.pressed = 1", "button.button_pressed = 1",
					 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
					 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("button.pressed=1", "button.button_pressed=1",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);
	valid = valid && test_conversion_gdscript_builtin("button.pressed SF", "button.pressed SF",
						 &ProjectConverter3To4::rename_gdscript_functions, "custom rename",
						 reg_container, false);

	valid = valid &&
			test_conversion_with_regex("Color(\"#f47d\")", "Color(\"#47df\")",
				&ProjectConverter3To4::convert_hexadecimal_colors, "color literals", reg_container);
	valid = valid &&
			test_conversion_with_regex("Color(\"#ff478cbf\")", "Color(\"#478cbfff\")",
				&ProjectConverter3To4::convert_hexadecimal_colors, "color literals", reg_container);
	valid = valid &&
			test_conversion_with_regex("Color(\"#de32bf\")", "Color(\"#de32bf\")",
				&ProjectConverter3To4::convert_hexadecimal_colors, "color literals", reg_container);
	valid = valid && test_conversion_with_regex("AAA Color.white AF", "AAA Color.WHITE AF",
						 &ProjectConverter3To4::rename_colors, "color constants", reg_container);

	// Note: Do not change to *scancode*, it is applied before that conversion.
	valid = valid &&
			test_conversion_with_regex(
				"\"device\":-1,\"scancode\":16777231,\"physical_scancode\":16777232",
				"\"device\":-1,\"scancode\":4194319,\"physical_scancode\":4194320",
				&ProjectConverter3To4::rename_input_map_scancode, "custom rename", reg_container);
	valid = valid &&
			test_conversion_with_regex("\"device\":-1,\"scancode\":65,\"physical_scancode\":66",
				"\"device\":-1,\"scancode\":65,\"physical_scancode\":66",
				&ProjectConverter3To4::rename_input_map_scancode, "custom rename", reg_container);

	valid = valid && test_conversion_with_regex(
						 "\"device\":0,\"button_index\":5,\"pressure\":0.0,\"pressed\":false,",
						 "\"device\":0,\"button_index\":10,\"pressure\":0.0,\"pressed\":false,",
						 &ProjectConverter3To4::rename_joypad_buttons_and_axes, "custom rename",
						 reg_container);
	valid =
		valid &&
		test_conversion_with_regex("\"device\":0,\"axis\":6,", "\"device\":0,\"axis\":4,",
			&ProjectConverter3To4::rename_joypad_buttons_and_axes, "custom rename", reg_container);
	valid =
		valid &&
		test_conversion_with_regex("InputEventJoypadButton,\"button_index\":7,\"pressure\":0.0,"
								   "\"pressed\":false,\"script\":null",
			"InputEventJoypadMotion,\"axis\":5,\"axis_value\":1.0,\"script\":null",
			&ProjectConverter3To4::rename_joypad_buttons_and_axes, "custom rename", reg_container);

	// Custom rule conversion
	{
		String from = "instance";
		String to = "instantiate";
		String name = "AA.instance()";

		Vector<SourceLine> got = split_lines(name);

		String expected = "AA.instantiate()";
		custom_rename(got, from, to);
		String got_str = collect_string_from_vector(got);
		if (got_str != expected) {

		ERR_PRINT(
				vformat("Failed to convert custom rename \"%s\" to \"%s\", got \"%s\", instead.",
					name, expected, got_str));
		}
		valid = valid && (got_str == expected);
	}

	// get_object_of_execution
	{
		String base = "var roman = kieliszek.";
		String expected = "kieliszek.";
		String got = get_object_of_execution(base);
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from get_object_of_execution. \"%s\" "
							  "should return \"%s\"(%d), got \"%s\"(%d), instead.",
				base, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}
	{
		String base = "r.";
		String expected = "r.";
		String got = get_object_of_execution(base);
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from get_object_of_execution. \"%s\" "
							  "should return \"%s\"(%d), got \"%s\"(%d), instead.",
				base, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}
	{
		String base = "mortadela(";
		String expected = "";
		String got = get_object_of_execution(base);
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from get_object_of_execution. \"%s\" "
							  "should return \"%s\"(%d), got \"%s\"(%d), instead.",
				base, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}
	{
		String base = "var node = $world/ukraine/lviv.";
		String expected = "$world/ukraine/lviv.";
		String got = get_object_of_execution(base);
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from get_object_of_execution. \"%s\" "
							  "should return \"%s\"(%d), got \"%s\"(%d), instead.",
				base, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}

	// get_starting_space
	{
		String base = "\t\t\t var roman = kieliszek.";
		String expected = "\t\t\t";
		String got = get_starting_space(base);
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from get_object_of_execution. \"%s\" "
							  "should return \"%s\"(%d), got \"%s\"(%d), instead.",
				base, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}

	// Parse Arguments
	{
		String line = "( )";
		Vector<String> got_vector = parse_arguments(line);
		String got = "";
		String expected = "";
		for (String& part : got_vector) {
			got += part + "|||";
		}
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from parse_arguments. \"%s\" should "
							  "return \"%s\"(%d), got \"%s\"(%d), instead.",
				line, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}
	{
		String line = "(a , b , c)";
		Vector<String> got_vector = parse_arguments(line);
		String got = "";
		String expected = "a|||b|||c|||";
		for (String& part : got_vector) {
			got += part + "|||";
		}
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from parse_arguments. \"%s\" should "
							  "return \"%s\"(%d), got \"%s\"(%d), instead.",
				line, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}
	{
		String line = "(a , \"b,\" , c)";
		Vector<String> got_vector = parse_arguments(line);
		String got = "";
		String expected = "a|||\"b,\"|||c|||";
		for (String& part : got_vector) {
			got += part + "|||";
		}
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from parse_arguments. \"%s\" should "
							  "return \"%s\"(%d), got \"%s\"(%d), instead.",
				line, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}
	{
		String line = "(a , \"(,),,,,\" , c)";
		Vector<String> got_vector = parse_arguments(line);
		String got = "";
		String expected = "a|||\"(,),,,,\"|||c|||";
		for (String& part : got_vector) {
			got += part + "|||";
		}
		if (got != expected) {
			ERR_PRINT(vformat("Failed to get proper data from parse_arguments. \"%s\" should "
							  "return \"%s\"(%d), got \"%s\"(%d), instead.",
				line, expected, expected.size(), got, got.size()));
		}
		valid = valid && (got == expected);
	}

	return valid;
}

// Validate in all arrays if names don't do cyclic renames "Node" -> "Node2D" | "Node2D" -> "2DNode"
bool ProjectConverter3To4::test_array_names()
{
	bool valid = true;

	// Validate if all classes are valid.
	{
		for (unsigned int current_index = 0; RenamesMap3To4::class_renames[current_index][0];
			 current_index++) {
			const String old_class = RenamesMap3To4::class_renames[current_index][0];
			const String new_class = RenamesMap3To4::class_renames[current_index][1];
		}
	}

	{
		HashSet<String> all_functions;

		// List of excluded functions from builtin types and global namespace, because currently it
		// is not possible to get list of functions from them. This will be available when
		// https://github.com/godotengine/godot/pull/49053 or similar will be included into Godot.
		static const char* builtin_types_excluded_functions[] = {"dict_to_inst", "inst_to_dict",
			"bytes_to_var", "bytes_to_var_with_objects", "db_to_linear", "deg_to_rad",
			"linear_to_db", "rad_to_deg", "randf_range", "snapped", "str_to_var", "var_to_str",
			"var_to_bytes", "var_to_bytes_with_objects", "move_toward", "uri_encode", "uri_decode",
			"remove_at", "get_rotation_quaternion", "limit_length", "grow_side", "is_absolute_path",
			"is_valid_int", "lerp", "to_ascii_buffer", "to_utf8_buffer", "to_utf32_buffer",
			"to_wchar_buffer", "snapped", "remap", "rfind", nullptr};
		for (int current_index = 0; builtin_types_excluded_functions[current_index];
			 current_index++) {
			all_functions.insert(builtin_types_excluded_functions[current_index]);
		}

		// for (int type = Variant::Type::NIL + 1; type < Variant::Type::VARIANT_MAX; type++) {
		//	List<MethodInfo> method_list;
		//	Variant::get_method_list_by_type(&method_list, Variant::Type(type));
		//	for (MethodInfo &function_data : method_list) {
		//		if (!all_functions.has(function_data.name)) {
		//			all_functions.insert(function_data.name);
		//		}
		//	}
		// }

		int current_element = 0;
		while (RenamesMap3To4::gdscript_function_renames[current_element][0] != nullptr) {
			String name_3_x = RenamesMap3To4::gdscript_function_renames[current_element][0];
			String name_4_0 = RenamesMap3To4::gdscript_function_renames[current_element][1];
			if (!all_functions.has(name_4_0)) {
				ERR_PRINT(vformat(
					"Missing GDScript function in pair (%s - ===> %s <===)", name_3_x, name_4_0));
				valid = false;
			}
			current_element++;
		}
	}
	if (!valid) {
		ERR_PRINT("Found function which is used in the converter, but it cannot be found in Godot "
				  "4. Rename this element or remove its entry if it's obsolete.");
	}

	valid = valid && test_single_array(RenamesMap3To4::enum_renames);
	valid = valid && test_single_array(RenamesMap3To4::class_renames, true);
	valid = valid && test_single_array(RenamesMap3To4::gdscript_function_renames, true);
	valid = valid && test_single_array(RenamesMap3To4::csharp_function_renames, true);
	valid = valid && test_single_array(RenamesMap3To4::gdscript_properties_renames, true);
	valid = valid && test_single_array(RenamesMap3To4::csharp_properties_renames, true);
	valid = valid && test_single_array(RenamesMap3To4::shaders_renames, true);
	valid = valid && test_single_array(RenamesMap3To4::gdscript_signals_renames);
	valid = valid && test_single_array(RenamesMap3To4::project_settings_renames);
	valid = valid && test_single_array(RenamesMap3To4::project_godot_renames);
	valid = valid && test_single_array(RenamesMap3To4::input_map_renames);
	valid = valid && test_single_array(RenamesMap3To4::builtin_types_renames);
	valid = valid && test_single_array(RenamesMap3To4::color_renames);

	return valid;
}

// Validates the array to prevent cyclic renames, such as `Node` -> `Node2D`, then `Node2D` ->
// `2DNode`. Also checks if names contain leading or trailing spaces.
bool ProjectConverter3To4::test_single_array(const char* p_array[][2], bool p_ignore_4_0_name)
{
	bool valid = true;
	Vector<String> names = Vector<String>();

	for (unsigned int current_index = 0; p_array[current_index][0]; current_index++) {
		String name_3_x = p_array[current_index][0];
		String name_4_0 = p_array[current_index][1];
		if (name_3_x != name_3_x.strip_edges()) {
			ERR_PRINT(
				vformat("Invalid Entry \"%s\" contains leading or trailing spaces.", name_3_x));
			valid = false;
		}
		if (names.has(name_3_x)) {
			ERR_PRINT(vformat("Found duplicated entry, pair ( -> %s , %s)", name_3_x, name_4_0));
			valid = false;
		}
		names.append(name_3_x);

		if (name_4_0 != name_4_0.strip_edges()) {
			ERR_PRINT(
				vformat("Invalid Entry \"%s\" contains leading or trailing spaces.", name_3_x));
			valid = false;
		}
		if (names.has(name_4_0)) {
			ERR_PRINT(vformat("Found duplicated entry, pair ( -> %s , %s)", name_3_x, name_4_0));
			valid = false;
		}
		if (!p_ignore_4_0_name) {
			names.append(name_4_0);
		}
	}
	return valid;
}

// Returns arguments from given function execution, this cannot be really done as regex.
// `abc(d,e(f,g),h)` -> [d], [e(f,g)], [h]
Vector<String> ProjectConverter3To4::parse_arguments(const String& line)
{
	Vector<String> parts;
	int string_size = line.length();
	int start_part = 0; // Index of beginning of start part.
	int parts_counter = 0;
	char32_t previous_character = '\0';
	bool is_inside_string = false; // If true, it ignores these 3 characters ( , ) inside string.

	ERR_FAIL_COND_V_MSG(line.count("(") != line.count(")"), parts,
		vformat("Converter internal bug: substring should have equal number of open and close "
				"parentheses in line - \"%s\".",
			line));

	for (int current_index = 0; current_index < string_size; current_index++) {
		char32_t character = line.get(current_index);
		switch (character) {
		case '(':
		case '[':
		case '{': {
			parts_counter++;
			if (parts_counter == 1 && !is_inside_string) {
				start_part = current_index;
			}
			break;
		};
		case ')':
		case '}': {
			parts_counter--;
			if (parts_counter == 0 && !is_inside_string) {
				parts.append(line.substr(start_part + 1, current_index - start_part - 1));
				start_part = current_index;
			}
			break;
		};
		case ']': {
			parts_counter--;
			if (parts_counter == 0 && !is_inside_string) {
				parts.append(line.substr(start_part, current_index - start_part));
				start_part = current_index;
			}
			break;
		};
		case ',': {
			if (parts_counter == 1 && !is_inside_string) {
				parts.append(line.substr(start_part + 1, current_index - start_part - 1));
				start_part = current_index;
			}
			break;
		};
		case '"': {
			if (previous_character != '\\') {
				is_inside_string = !is_inside_string;
			}
		}
		}
		previous_character = character;
	}

	Vector<String> clean_parts;
	for (String& part : parts) {
		part = part.strip_edges();
		if (!part.is_empty()) {
			clean_parts.append(part);
		}
	}

	return clean_parts;
}

// Finds latest parenthesis owned by function.
// `function(abc(a,b),DD)):` finds this parenthess `function(abc(a,b),DD => ) <= ):`
int ProjectConverter3To4::get_end_parenthesis(const String& line) const
{
	int current_state = 0;
	for (int current_index = 0; line.length() > current_index; current_index++) {
		char32_t character = line.get(current_index);
		if (character == '(') {
			current_state++;
		}
		if (character == ')') {
			current_state--;
			if (current_state == 0) {
				return current_index;
			}
		}
	}
	return -1;
}

// Merges multiple arguments into a single String.
// Needed when after processing e.g. 2 arguments, later arguments are not changed in any way.
String ProjectConverter3To4::connect_arguments(
	const Vector<String>& arguments, int from, int to) const
{
	if (to == -1) {
		to = arguments.size();
	}

	String value;
	if (arguments.size() > 0 && from != 0 && from < to) {
		value = ", ";
	}

	for (int i = from; i < to; i++) {
		value += arguments[i];
		if (i != to - 1) {
			value += ", ";
		}
	}
	return value;
}

// Returns the indentation (spaces and tabs) at the start of the line e.g. `\t\tmove_this` returns
// `\t\t`.
String ProjectConverter3To4::get_starting_space(const String& line) const
{
	String empty_space;
	int current_character = 0;

	if (line.is_empty()) {
		return empty_space;
	}

	if (line[0] == ' ') {
		while (current_character < line.size()) {
			if (line[current_character] == ' ') {
				empty_space += ' ';
				current_character++;
			}
			else {
				break;
			}
		}
	}
	if (line[0] == '\t') {
		while (current_character < line.size()) {
			if (line[current_character] == '\t') {
				empty_space += '\t';
				current_character++;
			}
			else {
				break;
			}
		}
	}
	return empty_space;
}

// Returns the object that’s executing the function in the line.
// e.g. Passing the line "var roman = kieliszek.funkcja()" to this function returns "kieliszek".
String ProjectConverter3To4::get_object_of_execution(const String& line) const
{
	int end = line.size() - 1; // Last one is \0
	int variable_start = end - 1;
	int start = end - 1;

	bool is_possibly_nodepath = false;
	bool is_valid_nodepath = false;

	while (start >= 0) {
		char32_t character = line[start];
		bool is_variable_char = (character >= 'A' && character <= 'Z') ||
								(character >= 'a' && character <= 'z') || character == '.' ||
								character == '_';
		bool is_nodepath_start = character == '$';
		bool is_nodepath_sep = character == '/';
		if (is_variable_char || is_nodepath_start || is_nodepath_sep) {
			if (start == 0) {
				break;
			}
			else if (is_nodepath_sep) {
				// Freeze variable_start, try to fetch more chars since this might be a Node path
				// literal.
				is_possibly_nodepath = true;
			}
			else if (is_nodepath_start) {
				// Found $, this is a Node path literal.
				is_valid_nodepath = true;
				break;
			}
			if (!is_possibly_nodepath) {
				variable_start--;
			}
			start--;
			continue;
		}
		else {
			// Abandon all hope, this is neither a variable nor a Node path literal.
			variable_start++; // Found invalid character, needs to be ignored.
			break;
		}
	}
	if (is_valid_nodepath) {
		variable_start = start;
	}
	return line.substr(variable_start, (end - variable_start));
}

void ProjectConverter3To4::rename_colors(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			if (line.contains("Color.")) {
				for (unsigned int current_index = 0;
					 RenamesMap3To4::color_renames[current_index][0]; current_index++) {
					line = reg_container.color_regexes[current_index]->sub(
						line, reg_container.color_renamed[current_index], true);
				}
			}
		}
	}
}

// Convert hexadecimal colors from ARGB to RGBA
void ProjectConverter3To4::convert_hexadecimal_colors(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			if (line.contains("Color(\"")) {
				line = reg_container.color_hexadecimal_short_constructor.sub(
					line, "Color(\"#$2$1", true);
				line = reg_container.color_hexadecimal_full_constructor.sub(
					line, "Color(\"#$2$1", true);
			}
		}
	}
}

void ProjectConverter3To4::fix_tool_declaration(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	// In godot4, "tool" became "@tool" and must be located at the top of the file.
	for (int i = 0; i < source_lines.size(); ++i) {
		if (source_lines[i].line == "tool") {
			source_lines.remove_at(i);
			source_lines.insert(0, {"@tool", false});
			return; // assuming there's at most 1 tool declaration.
		}
	}
}

void ProjectConverter3To4::fix_pause_mode(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	// In Godot 3, the pause_mode 2 equals the PAUSE_MODE_PROCESS value.
	// In Godot 4, the pause_mode PAUSE_MODE_PROCESS was renamed to PROCESS_MODE_ALWAYS and equals
	// the number 3. We therefore convert pause_mode = 2 to pause_mode = 3.
	for (SourceLine& source_line : source_lines) {
		String& line = source_line.line;

		if (line == "pause_mode = 2") {
			// Note: pause_mode is renamed to process_mode later on, so no need to do it here.
			line = "pause_mode = 3";
		}
	}
}

void ProjectConverter3To4::rename_classes(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			for (unsigned int current_index = 0; RenamesMap3To4::class_renames[current_index][0];
				 current_index++) {
				if (line.contains(RenamesMap3To4::class_renames[current_index][0])) {
					bool found_ignored_items = false;
					// Renaming Spatial.tscn to TEMP_RENAMED_CLASS.tscn.
					if (line.contains(
							String(RenamesMap3To4::class_renames[current_index][0]) + ".")) {
						found_ignored_items = true;
						line = reg_container.class_tscn_regexes[current_index]->sub(
							line, "TEMP_RENAMED_CLASS.tscn", true);
						line = reg_container.class_gd_regexes[current_index]->sub(
							line, "TEMP_RENAMED_CLASS.gd", true);
						line = reg_container.class_shader_regexes[current_index]->sub(
							line, "TEMP_RENAMED_CLASS.shader", true);
					}

					// Causal renaming Spatial -> Node3D.
					line = reg_container.class_regexes[current_index]->sub(
						line, RenamesMap3To4::class_renames[current_index][1], true);

					// Restore Spatial.tscn from TEMP_RENAMED_CLASS.tscn.
					if (found_ignored_items) {
						line = reg_container.class_temp_tscn.sub(
							line, reg_container.class_temp_tscn_renames[current_index], true);
						line = reg_container.class_temp_gd.sub(
							line, reg_container.class_temp_gd_renames[current_index], true);
						line = reg_container.class_temp_shader.sub(
							line, reg_container.class_temp_shader_renames[current_index], true);
					}
				}
			}
		}
	}
}

void ProjectConverter3To4::rename_gdscript_functions(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container, bool builtin)
{
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			process_gdscript_line(line, reg_container, builtin);
		}
	}
}

Vector<String> ProjectConverter3To4::check_for_rename_gdscript_functions(
	Vector<String>& lines, const RegExContainer& reg_container, bool builtin)
{
	int current_line = 1;

	Vector<String> found_renames;

	for (String& line : lines) {
		if (uint64_t(line.length()) <= maximum_line_length) {
			String old_line = line;
			process_gdscript_line(line, reg_container, builtin);
			if (old_line != line) {
				found_renames.append(simple_line_formatter(current_line, old_line, line));
			}
		}
	}

	return found_renames;
}

bool ProjectConverter3To4::contains_function_call(const String& line, const String& function) const
{
	// We want to convert the function only if it is completely standalone.
	// For example, when we search for "connect(", we don't want to accidentally convert
	// "reconnect(".
	if (!line.contains(function)) {
		return false;
	}

	int index = line.find(function);
	if (index == 0) {
		return true;
	}

	char32_t previous_char = line.get(index - 1);
	return (previous_char < '0' || previous_char > '9') &&
		   (previous_char < 'a' || previous_char > 'z') &&
		   (previous_char < 'A' || previous_char > 'Z') && previous_char != '_' &&
		   previous_char != '$' && previous_char != '@';
}

// TODO, this function should run only on all ".gd" files and also on lines in ".tscn" files which
// are parts of built-in Scripts.
void ProjectConverter3To4::process_gdscript_line(
	String& line, const RegExContainer& reg_container, bool builtin)
{
	// In this and other functions, reg.sub() is used only after checking lines with str.contains().
	// With longer lines, doing so can sometimes be significantly faster.

	if ((line.contains(".lock") || line.contains(".unlock")) && !line.contains("mtx") &&
		!line.contains("mutex") && !line.contains("Mutex")) {
		line = reg_container.reg_image_lock.sub(line,
			"false # $1.lock() # TODOConverter3To4, Image no longer requires locking, `false` "
			"helps to not break one line if/else, so it can freely be removed",
			true);
		line = reg_container.reg_image_unlock.sub(line,
			"false # $1.unlock() # TODOConverter3To4, Image no longer requires locking, `false` "
			"helps to not break one line if/else, so it can freely be removed",
			true);
	}

	// PackedStringArray(req_godot).join('.') -> '.'.join(PackedStringArray(req_godot))
	// PoolStringArray
	if (line.contains(".join")) {
		line = reg_container.reg_join.sub(line, "$2.join($1)", true);
	}

	// -- empty() -> is_empty()       Pool*Array
	if (line.contains("empty")) {
		line = reg_container.reg_is_empty.sub(line, "is_empty(", true);
	}

	// -- \t.func() -> \tsuper.func()       Object
	if (line.contains_char('(') && line.contains_char('.')) {
		line = reg_container.reg_super.sub(
			line, "$1super.$2", true); // TODO, not sure if possible, but for now this broke String
									   // text e.g. "Chosen .gitignore" -> "Chosen super.gitignore"
	}

	// -- JSON.parse(a) -> JSON.new().parse(a) etc.    JSON
	if (line.contains("parse")) {
		line = reg_container.reg_json_non_new.sub(line,
			"$1var test_json_conv = "
			"JSON.new()\n$1test_json_conv.parse($3\n$1$2test_json_conv.get_data()",
			true);
	}

	// -- to_json(a) -> JSON.new().stringify(a)     Object
	if (line.contains("to_json")) {
		line = reg_container.reg_json_to.sub(line, "JSON.new().stringify", true);
	}
	// -- parse_json(a) -> JSON.get_data() etc.    Object
	if (line.contains("parse_json")) {
		line = reg_container.reg_json_parse.sub(line,
			"$1var test_json_conv = "
			"JSON.new()\n$1test_json_conv.parse($3\n$1$2test_json_conv.get_data()",
			true);
	}
	// -- JSON.print( -> JSON.stringify(
	if (line.contains("JSON.print(")) {
		line = reg_container.reg_json_print.sub(line, "JSON.stringify(", true);
	}

	// -- get_node(@ -> get_node(       Node
	if (line.contains("get_node")) {
		line = line.replace("get_node(@", "get_node(");
	}

	if (line.contains("export")) {
		// 1. export(float) var lifetime: float = 3.0 -> export var lifetime: float = 3.0
		line = reg_container.reg_export_typed.sub(line, "export var $2: $1");
		// 2. export(float) var lifetime := 3.0 -> export var lifetime := 3.0
		line = reg_container.reg_export_inferred_type.sub(line, "export var $1 :=");
		// 3. export(float) var lifetime = 3.0 -> export var lifetime: float = 3.0     GDScript
		line = reg_container.reg_export_simple.sub(line, "export var $2: $1");
		// 4. export(String, 'AnonymousPro', 'CourierPrime') var _font_name = 'AnonymousPro' ->
		// export var _font_name = 'AnonymousPro' #(String, 'AnonymousPro', 'CourierPrime') GDScript
		line = reg_container.reg_export_advanced.sub(line, "export var $2$3 # ($1)");
	}

	// Setget Setget
	if (line.contains("setget")) {
		line = reg_container.reg_setget_setget.sub(line, "var $1$2: get = $4, set = $3", true);
	}

	// Setget set
	if (line.contains("setget")) {
		line = reg_container.reg_setget_set.sub(line, "var $1$2: set = $3", true);
	}

	// Setget get
	if (line.contains("setget")) {
		line = reg_container.reg_setget_get.sub(line, "var $1$2: get = $3", true);
	}

	if (line.contains("window_resizable")) {
		// OS.set_window_resizable(a) -> get_window().unresizable = not (a)
		line = reg_container.reg_os_set_window_resizable.sub(
			line, "get_window().unresizable = not ($1)", true);
		// OS.window_resizable = a -> same
		line = reg_container.reg_os_assign_window_resizable.sub(
			line, "get_window().unresizable = not ($1)", true);
		// OS.[is_]window_resizable() -> (not get_window().unresizable)
		line = reg_container.reg_os_is_window_resizable.sub(
			line, "(not get_window().unresizable)", true);
	}

	if (line.contains("window_fullscreen")) {
		// OS.window_fullscreen(a) -> get_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if (a)
		// else Window.MODE_WINDOWED
		line = reg_container.reg_os_set_fullscreen.sub(line,
			"get_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if ($1) else "
			"Window.MODE_WINDOWED",
			true);
		// window_fullscreen = a -> same
		line = reg_container.reg_os_assign_fullscreen.sub(line,
			"get_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if ($1) else "
			"Window.MODE_WINDOWED",
			true);
		// OS.[is_]window_fullscreen() -> ((get_window().mode == Window.MODE_EXCLUSIVE_FULLSCREEN)
		// or (get_window().mode == Window.MODE_FULLSCREEN))
		line = reg_container.reg_os_is_fullscreen.sub(line,
			"((get_window().mode == Window.MODE_EXCLUSIVE_FULLSCREEN) or (get_window().mode == "
			"Window.MODE_FULLSCREEN))",
			true);
	}

	if (line.contains("window_maximized")) {
		// OS.window_maximized(a) -> get_window().mode = Window.MODE_MAXIMIZED if (a) else
		// Window.MODE_WINDOWED
		line = reg_container.reg_os_set_maximized.sub(line,
			"get_window().mode = Window.MODE_MAXIMIZED if ($1) else Window.MODE_WINDOWED", true);
		// window_maximized = a -> same
		line = reg_container.reg_os_assign_maximized.sub(line,
			"get_window().mode = Window.MODE_MAXIMIZED if ($1) else Window.MODE_WINDOWED", true);
		// OS.[is_]window_maximized() -> (get_window().mode == Window.MODE_MAXIMIZED)
		line = reg_container.reg_os_is_maximized.sub(
			line, "(get_window().mode == Window.MODE_MAXIMIZED)", true);
	}

	if (line.contains("window_minimized")) {
		// OS.window_minimized(a) -> get_window().mode = Window.MODE_MINIMIZED if (a) else
		// Window.MODE_WINDOWED
		line = reg_container.reg_os_set_minimized.sub(line,
			"get_window().mode = Window.MODE_MINIMIZED if ($1) else Window.MODE_WINDOWED", true);
		// window_minimized = a -> same
		line = reg_container.reg_os_assign_minimized.sub(line,
			"get_window().mode = Window.MODE_MINIMIZED if ($1) else Window.MODE_WINDOWED", true);
		// OS.[is_]window_minimized() -> (get_window().mode == Window.MODE_MINIMIZED)
		line = reg_container.reg_os_is_minimized.sub(
			line, "(get_window().mode == Window.MODE_MINIMIZED)", true);
	}

	if (line.contains("set_use_vsync")) {
		// OS.set_use_vsync(a) -> get_window().window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if
		// (a) else DisplayServer.VSYNC_DISABLED)
		line = reg_container.reg_os_set_vsync.sub(line,
			"DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if ($1) else "
			"DisplayServer.VSYNC_DISABLED)",
			true);
	}
	if (line.contains("vsync_enabled")) {
		// vsync_enabled = a -> get_window().window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if
		// (a) else DisplayServer.VSYNC_DISABLED)
		line = reg_container.reg_os_assign_vsync.sub(line,
			"DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if ($1) else "
			"DisplayServer.VSYNC_DISABLED)",
			true);
		// OS.[is_]vsync_enabled() -> (DisplayServer.window_get_vsync_mode() !=
		// DisplayServer.VSYNC_DISABLED)
		line = reg_container.reg_os_is_vsync.sub(
			line, "(DisplayServer.window_get_vsync_mode() != DisplayServer.VSYNC_DISABLED)", true);
	}

	if (line.contains("OS.screen_orientation")) { // keep "OS." at start
		// OS.screen_orientation = a -> DisplayServer.screen_set_orientation(a)
		line = reg_container.reg_os_assign_screen_orient.sub(
			line, "$1DisplayServer.screen_set_orientation($2)", true); // assignment
		line = line.replace(
			"OS.screen_orientation", "DisplayServer.screen_get_orientation()"); // value access
	}

	if (line.contains("_window_always_on_top")) {
		// OS.set_window_always_on_top(a) -> get_window().always_on_top = (a)
		line = reg_container.reg_os_set_always_on_top.sub(
			line, "get_window().always_on_top = ($1)", true);
		// OS.is_window_always_on_top() -> get_window().always_on_top
		line = reg_container.reg_os_is_always_on_top.sub(line, "get_window().always_on_top", true);
	}

	if (line.contains("et_borderless_window")) {
		// OS.set_borderless_window(a) -> get_window().borderless = (a)
		line =
			reg_container.reg_os_set_borderless.sub(line, "get_window().borderless = ($1)", true);
		// OS.get_borderless_window() -> get_window().borderless
		line = reg_container.reg_os_get_borderless.sub(line, "get_window().borderless", true);
	}

	// OS.SCREEN_ORIENTATION_* -> DisplayServer.SCREEN_*
	if (line.contains("OS.SCREEN_ORIENTATION_")) {
		line = reg_container.reg_os_screen_orient_enum.sub(line, "DisplayServer.SCREEN_$1", true);
	}

	// OS -> Window simple replacements with optional set/get.
	if (line.contains("current_screen")) {
		line = reg_container.reg_os_current_screen.sub(line, "get_window().$1current_screen", true);
	}
	if (line.contains("min_window_size")) {
		line = reg_container.reg_os_min_window_size.sub(line, "get_window().$1min_size", true);
	}
	if (line.contains("max_window_size")) {
		line = reg_container.reg_os_max_window_size.sub(line, "get_window().$1max_size", true);
	}
	if (line.contains("window_position")) {
		line = reg_container.reg_os_window_position.sub(line, "get_window().$1position", true);
	}
	if (line.contains("window_size")) {
		line = reg_container.reg_os_window_size.sub(line, "get_window().$1size", true);
	}
	if (line.contains("et_screen_orientation")) {
		line = reg_container.reg_os_getset_screen_orient.sub(
			line, "DisplayServer.screen_$1et_orientation", true);
	}

	// Instantiate
	if (contains_function_call(line, "instance")) {
		line = reg_container.reg_instantiate.sub(line, ".instantiate($1)", true);
	}

	// -- r.move_and_slide( a, b, c, d, e )  ->  r.set_velocity(a) ... r.move_and_slide()
	// KinematicBody
	if (contains_function_call(line, "move_and_slide(")) {
		int start = line.find("move_and_slide(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			String base_obj = get_object_of_execution(line.substr(0, start));
			String starting_space = get_starting_space(line);

			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() >= 1) {
				String line_new;

				// motion_velocity
				line_new += starting_space + base_obj + "set_velocity(" + parts[0] + ")\n";

				// up_direction
				if (parts.size() >= 2) {
					line_new += starting_space + base_obj + "set_up_direction(" + parts[1] + ")\n";
				}

				// stop_on_slope
				if (parts.size() >= 3) {
					line_new += starting_space + base_obj + "set_floor_stop_on_slope_enabled(" +
								parts[2] + ")\n";
				}

				// max_slides
				if (parts.size() >= 4) {
					line_new += starting_space + base_obj + "set_max_slides(" + parts[3] + ")\n";
				}

				// floor_max_angle
				if (parts.size() >= 5) {
					line_new +=
						starting_space + base_obj + "set_floor_max_angle(" + parts[4] + ")\n";
				}

				// infiinite_interia
				if (parts.size() >= 6) {
					line_new += starting_space +
								"# TODOConverter3To4 infinite_inertia were removed in Godot 4 - "
								"previous value `" +
								parts[5] + "`\n";
				}

				line_new += starting_space + base_obj + "move_and_slide()";

				if (!line.begins_with(starting_space + "move_and_slide")) {
					line = line_new + "\n" + line.substr(0, start) + "velocity" +
						   line.substr(end + start);
				}
				else {
					line = line_new + line.substr(end + start);
				}
			}
		}
	}

	// -- r.move_and_slide_with_snap( a, b, c, d, e )  ->  r.set_velocity(a) ... r.move_and_slide()
	// KinematicBody
	if (contains_function_call(line, "move_and_slide_with_snap(")) {
		int start = line.find("move_and_slide_with_snap(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			String base_obj = get_object_of_execution(line.substr(0, start));
			String starting_space = get_starting_space(line);

			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() >= 1) {
				String line_new;

				// motion_velocity
				line_new += starting_space + base_obj + "set_velocity(" + parts[0] + ")\n";

				// snap
				if (parts.size() >= 2) {
					line_new += starting_space +
								"# TODOConverter3To4 looks that snap in Godot 4 is float, not "
								"vector like in Godot 3 - previous value `" +
								parts[1] + "`\n";
				}

				// up_direction
				if (parts.size() >= 3) {
					line_new += starting_space + base_obj + "set_up_direction(" + parts[2] + ")\n";
				}

				// stop_on_slope
				if (parts.size() >= 4) {
					line_new += starting_space + base_obj + "set_floor_stop_on_slope_enabled(" +
								parts[3] + ")\n";
				}

				// max_slides
				if (parts.size() >= 5) {
					line_new += starting_space + base_obj + "set_max_slides(" + parts[4] + ")\n";
				}

				// floor_max_angle
				if (parts.size() >= 6) {
					line_new +=
						starting_space + base_obj + "set_floor_max_angle(" + parts[5] + ")\n";
				}

				// infiinite_interia
				if (parts.size() >= 7) {
					line_new += starting_space +
								"# TODOConverter3To4 infinite_inertia were removed in Godot 4 - "
								"previous value `" +
								parts[6] + "`\n";
				}

				line_new += starting_space + base_obj + "move_and_slide()";

				if (!line.begins_with(starting_space + "move_and_slide_with_snap")) {
					line = line_new + "\n" + line.substr(0, start) + "velocity" +
						   line.substr(end + start);
				}
				else {
					line = line_new + line.substr(end + start);
				}
			}
		}
	}

	// -- sort_custom( a , b )  ->  sort_custom(Callable( a , b ))            Object
	if (contains_function_call(line, "sort_custom(")) {
		int start = line.find("sort_custom(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 2) {
				line = line.substr(0, start) + "sort_custom(Callable(" + parts[0] + ", " +
					   parts[1] + "))" + line.substr(end + start);
			}
		}
	}

	// -- list_dir_begin( )  ->  list_dir_begin()            Object
	if (contains_function_call(line, "list_dir_begin(")) {
		int start = line.find("list_dir_begin(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			line = line.substr(0, start) + "list_dir_begin() " + line.substr(end + start) +
				   "# TODOConverter3To4 fill missing arguments "
				   "https://github.com/godotengine/godot/pull/40547";
		}
	}

	// -- draw_line(1,2,3,4,5) -> draw_line(1, 2, 3, 4)            CanvasItem
	if (contains_function_call(line, "draw_line(")) {
		int start = line.find("draw_line(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 5) {
				line = line.substr(0, start) + "draw_line(" + parts[0] + ", " + parts[1] + ", " +
					   parts[2] + ", " + parts[3] + ")" + line.substr(end + start);
			}
		}
	}

	// -- func c(var a, var b) -> func c(a, b)
	if (line.contains("func ") && line.contains("var ")) {
		int start = line.find("func ");
		start = line.substr(start).find_char('(') + start;
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));

			String start_string = line.substr(0, start) + "(";
			for (int i = 0; i < parts.size(); i++) {
				start_string += parts[i].strip_edges().trim_prefix("var ");
				if (i != parts.size() - 1) {
					start_string += ", ";
				}
			}
			line = start_string + ")" + line.substr(end + start);
		}
	}

	// -- yield(this, \"timeout\") -> await this.timeout         GDScript
	if (contains_function_call(line, "yield(")) {
		int start = line.find("yield(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 2) {
				if (builtin) {
					line = line.substr(0, start) + "await " + parts[0] + "." +
						   parts[1].replace("\\\"", "").replace("\\'", "").remove_char(' ') +
						   line.substr(end + start);
				}
				else {
					line = line.substr(0, start) + "await " + parts[0] + "." +
						   parts[1].remove_chars("\"' ") + line.substr(end + start);
				}
			}
		}
	}

	// -- parse_json( AA ) -> TODO       Object
	if (contains_function_call(line, "parse_json(")) {
		int start = line.find("parse_json(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			line = line.substr(0, start) + "JSON.new().stringify(" + connect_arguments(parts, 0) +
				   ")" + line.substr(end + start);
		}
	}

	// -- .xform(Vector3(a,b,c)) -> * Vector3(a,b,c)            Transform
	if (line.contains(".xform(")) {
		int start = line.find(".xform(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 1) {
				line = line.substr(0, start) + " * (" + parts[0] + ")" + line.substr(end + start);
			}
		}
	}

	// -- .xform_inv(Vector3(a,b,c)) -> * Vector3(a,b,c)       Transform
	if (line.contains(".xform_inv(")) {
		int start = line.find(".xform_inv(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			String object_exec = get_object_of_execution(line.substr(0, start));
			if (line.contains(object_exec + ".xform")) {
				int start2 = line.find(object_exec + ".xform");
				Vector<String> parts = parse_arguments(line.substr(start, end));
				if (parts.size() == 1) {
					line = line.substr(0, start2) + "(" + parts[0] + ") * " + object_exec +
						   line.substr(end + start);
				}
			}
		}
	}

	// -- "(connect(A,B,C,D,E) != OK):", "(connect(A, Callable(B, C).bind(D), E)      Object
	if (contains_function_call(line, "connect(")) {
		int start = line.find("connect(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "connect(" + parts[0] + ", Callable(" + parts[1] +
					   ", " + parts[2] + "))" + line.substr(end + start);
			}
			else if (parts.size() >= 4) {
				line = line.substr(0, start) + "connect(" + parts[0] + ", Callable(" + parts[1] +
					   ", " + parts[2] + ").bind(" + parts[3].lstrip(" [").rstrip("] ") + ")" +
					   connect_arguments(parts, 4) + ")" + line.substr(end + start);
			}
		}
	}
	// -- disconnect(a,b,c) -> disconnect(a,Callable(b,c))      Object
	if (contains_function_call(line, "disconnect(")) {
		int start = line.find("disconnect(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "disconnect(" + parts[0] + ", Callable(" + parts[1] +
					   ", " + parts[2] + "))" + line.substr(end + start);
			}
		}
	}
	// -- is_connected(a,b,c) -> is_connected(a,Callable(b,c))      Object
	if (contains_function_call(line, "is_connected(")) {
		int start = line.find("is_connected(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "is_connected(" + parts[0] + ", Callable(" +
					   parts[1] + ", " + parts[2] + "))" + line.substr(end + start);
			}
		}
	}
	// -- "(tween_method(A,B,C,D,E) != OK):", "(tween_method(Callable(A,B),C,D,E)      Object
	// -- "(tween_method(A,B,C,D,E,[F,G]) != OK):", "(tween_method(Callable(A,B).bind(F,G),C,D,E)
	// Object
	if (contains_function_call(line, "tween_method(")) {
		int start = line.find("tween_method(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 5) {
				line = line.substr(0, start) + "tween_method(Callable(" + parts[0] + ", " +
					   parts[1] + "), " + parts[2] + ", " + parts[3] + ", " + parts[4] + ")" +
					   line.substr(end + start);
			}
			else if (parts.size() >= 6) {
				line = line.substr(0, start) + "tween_method(Callable(" + parts[0] + ", " +
					   parts[1] + ").bind(" +
					   connect_arguments(parts, 5).substr(1).lstrip(" [").rstrip("] ") + "), " +
					   parts[2] + ", " + parts[3] + ", " + parts[4] + ")" +
					   line.substr(end + start);
			}
		}
	}
	// -- "(tween_callback(A,B,[C,D]) != OK):", "(connect(Callable(A,B).bind(C,D))      Object
	if (contains_function_call(line, "tween_callback(")) {
		int start = line.find("tween_callback(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 2) {
				line = line.substr(0, start) + "tween_callback(Callable(" + parts[0] + ", " +
					   parts[1] + "))" + line.substr(end + start);
			}
			else if (parts.size() >= 3) {
				line = line.substr(0, start) + "tween_callback(Callable(" + parts[0] + ", " +
					   parts[1] + ").bind(" +
					   connect_arguments(parts, 2).substr(1).lstrip(" [").rstrip("] ") + "))" +
					   line.substr(end + start);
			}
		}
	}
	// -- start(a,b) -> start(Callable(a, b))      Thread
	// -- start(a,b,c,d) -> start(Callable(a, b).bind(c), d)      Thread
	if (contains_function_call(line, "start(")) {
		int start = line.find("start(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		// Protection from 'func start'
		if (!line.begins_with("func ")) {
			if (end > -1) {
				Vector<String> parts = parse_arguments(line.substr(start, end));
				if (parts.size() == 2) {
					line = line.substr(0, start) + "start(Callable(" + parts[0] + ", " + parts[1] +
						   "))" + line.substr(end + start);
				}
				else if (parts.size() >= 3) {
					line = line.substr(0, start) + "start(Callable(" + parts[0] + ", " + parts[1] +
						   ").bind(" + parts[2] + ")" + connect_arguments(parts, 3) + ")" +
						   line.substr(end + start);
				}
			}
		}
	}
	// -- func _init(p_x:int).(p_x):  -> func _init(p_x:int):\n\tsuper(p_x)    Object #
	// https://github.com/godotengine/godot/issues/70542
	if (line.contains(" _init(") && line.rfind_char(':') > 0) {
		//     func _init(p_arg1).(super4, super5, super6)->void:
		// ^--^indent            ^super_start   super_end^
		int indent = line.count("\t", 0, line.find("func"));
		int super_start = line.find(".(");
		int super_end = line.rfind_char(')');
		if (super_start > 0 && super_end > super_start) {
			line = line.substr(0, super_start) + line.substr(super_end + 1) + "\n" +
				   String("\t").repeat(indent + 1) + "super" +
				   line.substr(super_start + 1, super_end - super_start);
		}
	}

	//  create_from_image(aa, bb)  ->   create_from_image(aa) #, bb   ImageTexture
	if (contains_function_call(line, "create_from_image(")) {
		int start = line.find("create_from_image(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 2) {
				line = line.substr(0, start) + "create_from_image(" + parts[0] + ") " + "#," +
					   parts[1] + line.substr(end + start);
			}
		}
	}
	//  set_cell_item(a, b, c, d ,e)  ->   set_cell_item(Vector3(a, b, c), d ,e)
	if (contains_function_call(line, "set_cell_item(")) {
		int start = line.find("set_cell_item(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() > 2) {
				line = line.substr(0, start) + "set_cell_item(Vector3(" + parts[0] + ", " +
					   parts[1] + ", " + parts[2] + ")" + connect_arguments(parts, 3).lstrip(" ") +
					   ")" + line.substr(end + start);
			}
		}
	}
	//  get_cell_item(a, b, c)  ->   get_cell_item(Vector3i(a, b, c))
	if (contains_function_call(line, "get_cell_item(")) {
		int start = line.find("get_cell_item(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "get_cell_item(Vector3i(" + parts[0] + ", " +
					   parts[1] + ", " + parts[2] + "))" + line.substr(end + start);
			}
		}
	}
	//  get_cell_item_orientation(a, b, c)  ->   get_cell_item_orientation(Vector3i(a, b, c))
	if (contains_function_call(line, "get_cell_item_orientation(")) {
		int start = line.find("get_cell_item_orientation(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "get_cell_item_orientation(Vector3i(" + parts[0] +
					   ", " + parts[1] + ", " + parts[2] + "))" + line.substr(end + start);
			}
		}
	}
	//  apply_impulse(A, B)  ->   apply_impulse(B, A)
	if (contains_function_call(line, "apply_impulse(")) {
		int start = line.find("apply_impulse(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 2) {
				line = line.substr(0, start) + "apply_impulse(" + parts[1] + ", " + parts[0] + ")" +
					   line.substr(end + start);
			}
		}
	}
	//  apply_force(A, B)  ->   apply_force(B, A)
	if (contains_function_call(line, "apply_force(")) {
		int start = line.find("apply_force(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 2) {
				line = line.substr(0, start) + "apply_force(" + parts[1] + ", " + parts[0] + ")" +
					   line.substr(end + start);
			}
		}
	}
	//  map_to_world(a, b, c)  ->   map_to_local(Vector3i(a, b, c))
	if (contains_function_call(line, "map_to_world(")) {
		int start = line.find("map_to_world(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "map_to_local(Vector3i(" + parts[0] + ", " +
					   parts[1] + ", " + parts[2] + "))" + line.substr(end + start);
			}
			else if (parts.size() == 1) {
				line = line.substr(0, start) + "map_to_local(" + parts[0] + ")" +
					   line.substr(end + start);
			}
		}
	}

	//  set_rotating(true)  ->   set_ignore_rotation(false)
	if (contains_function_call(line, "set_rotating(")) {
		int start = line.find("set_rotating(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 1) {
				String opposite = parts[0] == "true" ? "false" : "true";
				line = line.substr(0, start) + "set_ignore_rotation(" + opposite + ")";
			}
		}
	}

	//  OS.get_window_safe_area()  ->   DisplayServer.get_display_safe_area()
	if (line.contains("OS.get_window_safe_area(")) {
		int start = line.find("OS.get_window_safe_area(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.is_empty()) {
				line = line.substr(0, start) + "DisplayServer.get_display_safe_area()" +
					   line.substr(end + start);
			}
		}
	}
	//  draw_rect(a,b,c,d,e)  ->   draw_rect(a,b,c,d)#e) TODOConverter3To4 Antialiasing argument is
	//  missing
	if (contains_function_call(line, "draw_rect(")) {
		int start = line.find("draw_rect(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 5) {
				line = line.substr(0, start) + "draw_rect(" + parts[0] + ", " + parts[1] + ", " +
					   parts[2] + ", " + parts[3] + ")" + line.substr(end + start) + "# " +
					   parts[4] + ") TODOConverter3To4 Antialiasing argument is missing";
			}
		}
	}
	// get_focus_owner() -> get_viewport().gui_get_focus_owner()
	if (contains_function_call(line, "get_focus_owner()")) {
		line = line.replace("get_focus_owner()", "get_viewport().gui_get_focus_owner()");
	}

	// button.pressed = 1 -> button.button_pressed = 1
	if (line.contains(".pressed")) {
		int start = line.find(".pressed");
		bool foundNextEqual = false;
		String line_to_check = line.substr(start + String(".pressed").length());
		for (int current_index = 0; line_to_check.length() > current_index; current_index++) {
			char32_t chr = line_to_check.get(current_index);
			if (chr == '\t' || chr == ' ') {
				continue;
			}
			else if (chr == '=') {
				foundNextEqual = true;
			}
			else {
				break;
			}
		}
		if (foundNextEqual) {
			line = line.substr(0, start) + ".button_pressed" +
				   line.substr(start + String(".pressed").length());
		}
	}

	// rotating = true  ->   ignore_rotation = false # reversed "rotating" for Camera2D
	if (contains_function_call(line, "rotating")) {
		String function_name = "rotating";
		int start = line.find(function_name);
		bool foundNextEqual = false;
		String line_to_check = line.substr(start + function_name.length());
		String assigned_value;
		for (int current_index = 0; line_to_check.length() > current_index; current_index++) {
			char32_t chr = line_to_check.get(current_index);
			if (chr == '\t' || chr == ' ') {
				continue;
			}
			else if (chr == '=') {
				foundNextEqual = true;
				assigned_value =
					line.substr(start + function_name.length() + current_index + 1).strip_edges();
				assigned_value = assigned_value == "true" ? "false" : "true";
			}
			else {
				break;
			}
		}
		if (foundNextEqual) {
			line = line.substr(0, start) + "ignore_rotation = " + assigned_value +
				   " # reversed \"rotating\" for Camera2D";
		}
	}

	// OS -> Time functions
	if (line.contains("OS.get_ticks_msec")) {
		line = line.replace("OS.get_ticks_msec", "Time.get_ticks_msec");
	}
	if (line.contains("OS.get_ticks_usec")) {
		line = line.replace("OS.get_ticks_usec", "Time.get_ticks_usec");
	}
	if (line.contains("OS.get_unix_time")) {
		line = line.replace("OS.get_unix_time", "Time.get_unix_time_from_system");
	}
	if (line.contains("OS.get_datetime")) {
		line = line.replace("OS.get_datetime", "Time.get_datetime_dict_from_system");
	}

	// OS -> DisplayServer
	if (line.contains("OS.get_display_cutouts")) {
		line = line.replace("OS.get_display_cutouts", "DisplayServer.get_display_cutouts");
	}
	if (line.contains("OS.get_screen_count")) {
		line = line.replace("OS.get_screen_count", "DisplayServer.get_screen_count");
	}
	if (line.contains("OS.get_screen_dpi")) {
		line = line.replace("OS.get_screen_dpi", "DisplayServer.screen_get_dpi");
	}
	if (line.contains("OS.get_screen_max_scale")) {
		line = line.replace("OS.get_screen_max_scale", "DisplayServer.screen_get_max_scale");
	}
	if (line.contains("OS.get_screen_position")) {
		line = line.replace("OS.get_screen_position", "DisplayServer.screen_get_position");
	}
	if (line.contains("OS.get_screen_refresh_rate")) {
		line = line.replace("OS.get_screen_refresh_rate", "DisplayServer.screen_get_refresh_rate");
	}
	if (line.contains("OS.get_screen_scale")) {
		line = line.replace("OS.get_screen_scale", "DisplayServer.screen_get_scale");
	}
	if (line.contains("OS.get_screen_size")) {
		line = line.replace("OS.get_screen_size", "DisplayServer.screen_get_size");
	}
	if (line.contains("OS.set_icon")) {
		line = line.replace("OS.set_icon", "DisplayServer.set_icon");
	}
	if (line.contains("OS.set_native_icon")) {
		line = line.replace("OS.set_native_icon", "DisplayServer.set_native_icon");
	}

	// OS -> Window
	if (line.contains("OS.window_borderless")) {
		line = line.replace("OS.window_borderless", "get_window().borderless");
	}
	if (line.contains("OS.get_real_window_size")) {
		line = line.replace("OS.get_real_window_size", "get_window().get_size_with_decorations");
	}
	if (line.contains("OS.is_window_focused")) {
		line = line.replace("OS.is_window_focused", "get_window().has_focus");
	}
	if (line.contains("OS.move_window_to_foreground")) {
		line = line.replace("OS.move_window_to_foreground", "get_window().grab_focus");
	}
	if (line.contains("OS.request_attention")) {
		line = line.replace("OS.request_attention", "get_window().request_attention");
	}
	if (line.contains("OS.set_window_title")) {
		line = line.replace("OS.set_window_title", "get_window().set_title");
	}

	// get_tree().set_input_as_handled() -> get_viewport().set_input_as_handled()
	if (line.contains("get_tree().set_input_as_handled()")) {
		line = line.replace(
			"get_tree().set_input_as_handled()", "get_viewport().set_input_as_handled()");
	}

	// Fix the simple case of using _unhandled_key_input
	// func _unhandled_key_input(event: InputEventKey) -> _unhandled_key_input(event: InputEvent)
	if (line.contains("_unhandled_key_input(event: InputEventKey)")) {
		line = line.replace("_unhandled_key_input(event: InputEventKey)",
			"_unhandled_key_input(event: InputEvent)");
	}

	if (line.contains("Engine.editor_hint")) {
		line = line.replace("Engine.editor_hint", "Engine.is_editor_hint()");
	}
}

void ProjectConverter3To4::process_csharp_line(String& line, const RegExContainer& reg_container)
{
	line = line.replace("OS.GetWindowSafeArea()", "DisplayServer.ScreenGetUsableRect()");

	// GetTree().SetInputAsHandled() -> GetViewport().SetInputAsHandled()
	if (line.contains("GetTree().SetInputAsHandled()")) {
		line = line.replace("GetTree().SetInputAsHandled()", "GetViewport().SetInputAsHandled()");
	}

	// Fix the simple case of using _UnhandledKeyInput
	// func _UnhandledKeyInput(InputEventKey @event) -> _UnhandledKeyInput(InputEvent @event)
	if (line.contains("_UnhandledKeyInput(InputEventKey @event)")) {
		line = line.replace(
			"_UnhandledKeyInput(InputEventKey @event)", "_UnhandledKeyInput(InputEvent @event)");
	}

	// -- Connect(,,,things) -> Connect(,Callable(,),things)      Object
	if (line.contains("Connect(")) {
		int start = line.find("Connect(");
		// Protection from disconnect
		if (start == 0 || line.get(start - 1) != 's') {
			int end = get_end_parenthesis(line.substr(start)) + 1;
			if (end > -1) {
				Vector<String> parts = parse_arguments(line.substr(start, end));
				if (parts.size() >= 3) {
					line = line.substr(0, start) + "Connect(" + parts[0] + ", new Callable(" +
						   parts[1] + ", " + parts[2] + ")" + connect_arguments(parts, 3) + ")" +
						   line.substr(end + start);
				}
			}
		}
	}
	// -- Disconnect(a,b,c) -> Disconnect(a,Callable(b,c))      Object
	if (line.contains("Disconnect(")) {
		int start = line.find("Disconnect(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "Disconnect(" + parts[0] + ", new Callable(" +
					   parts[1] + ", " + parts[2] + "))" + line.substr(end + start);
			}
		}
	}
	// -- IsConnected(a,b,c) -> IsConnected(a,Callable(b,c))      Object
	if (line.contains("IsConnected(")) {
		int start = line.find("IsConnected(");
		int end = get_end_parenthesis(line.substr(start)) + 1;
		if (end > -1) {
			Vector<String> parts = parse_arguments(line.substr(start, end));
			if (parts.size() == 3) {
				line = line.substr(0, start) + "IsConnected(" + parts[0] + ", new Callable(" +
					   parts[1] + ", " + parts[2] + "))" + line.substr(end + start);
			}
		}
	}
}

void ProjectConverter3To4::rename_csharp_functions(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			process_csharp_line(line, reg_container);
		}
	}
}

Vector<String> ProjectConverter3To4::check_for_rename_csharp_functions(
	Vector<String>& lines, const RegExContainer& reg_container)
{
	int current_line = 1;

	Vector<String> found_renames;

	for (String& line : lines) {
		if (uint64_t(line.length()) <= maximum_line_length) {
			String old_line = line;
			process_csharp_line(line, reg_container);
			if (old_line != line) {
				found_renames.append(simple_line_formatter(current_line, old_line, line));
			}
		}
	}

	return found_renames;
}

void ProjectConverter3To4::rename_csharp_attributes(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container)
{
	static String error_message =
		"The master and mastersync rpc behavior is not officially supported anymore. Try using "
		"another keyword or making custom logic using Multiplayer.GetRemoteSenderId()\n";

	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			line = reg_container.keyword_csharp_remote.sub(
				line, "[RPC(MultiplayerAPI.RPCMode.AnyPeer)]", true);
			line = reg_container.keyword_csharp_remotesync.sub(
				line, "[RPC(MultiplayerAPI.RPCMode.AnyPeer, CallLocal = true)]", true);
			line = reg_container.keyword_csharp_puppet.sub(line, "[RPC]", true);
			line =
				reg_container.keyword_csharp_puppetsync.sub(line, "[RPC(CallLocal = true)]", true);
			line = reg_container.keyword_csharp_master.sub(line, error_message + "[RPC]", true);
			line = reg_container.keyword_csharp_mastersync.sub(
				line, error_message + "[RPC(CallLocal = true)]", true);
		}
	}
}

Vector<String> ProjectConverter3To4::check_for_rename_csharp_attributes(
	Vector<String>& lines, const RegExContainer& reg_container)
{
	int current_line = 1;

	Vector<String> found_renames;

	for (String& line : lines) {
		if (uint64_t(line.length()) <= maximum_line_length) {
			String old;
			old = line;
			line = reg_container.keyword_csharp_remote.sub(
				line, "[RPC(MultiplayerAPI.RPCMode.AnyPeer)]", true);
			if (old != line) {
				found_renames.append(line_formatter(
					current_line, "[Remote]", "[RPC(MultiplayerAPI.RPCMode.AnyPeer)]", line));
			}

			old = line;
			line = reg_container.keyword_csharp_remotesync.sub(
				line, "[RPC(MultiplayerAPI.RPCMode.AnyPeer, CallLocal = true)]", true);
			if (old != line) {
				found_renames.append(line_formatter(current_line, "[RemoteSync]",
					"[RPC(MultiplayerAPI.RPCMode.AnyPeer, CallLocal = true)]", line));
			}

			old = line;
			line = reg_container.keyword_csharp_puppet.sub(line, "[RPC]", true);
			if (old != line) {
				found_renames.append(line_formatter(current_line, "[Puppet]", "[RPC]", line));
			}

			old = line;
			line =
				reg_container.keyword_csharp_puppetsync.sub(line, "[RPC(CallLocal = true)]", true);
			if (old != line) {
				found_renames.append(
					line_formatter(current_line, "[PuppetSync]", "[RPC(CallLocal = true)]", line));
			}

			old = line;
			line = reg_container.keyword_csharp_master.sub(line, "[RPC]", true);
			if (old != line) {
				found_renames.append(line_formatter(current_line, "[Master]", "[RPC]", line));
			}

			old = line;
			line =
				reg_container.keyword_csharp_mastersync.sub(line, "[RPC(CallLocal = true)]", true);
			if (old != line) {
				found_renames.append(
					line_formatter(current_line, "[MasterSync]", "[RPC(CallLocal = true)]", line));
			}
		}
		current_line++;
	}

	return found_renames;
}

_FORCE_INLINE_ static String builtin_escape(const String& p_str, bool p_builtin)
{
	if (p_builtin) {
		return p_str.replace("\"", "\\\"");
	}
	else {
		return p_str;
	}
}

void ProjectConverter3To4::rename_gdscript_keywords(
	Vector<SourceLine>& source_lines, const RegExContainer& reg_container, bool builtin)
{
	static String error_message =
		"The master and mastersync rpc behavior is not officially supported anymore. Try using "
		"another keyword or making custom logic using get_multiplayer().get_remote_sender_id()\n";

	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			if (line.contains("export")) {
				line = reg_container.keyword_gdscript_export_single.sub(line, "@export", true);
			}
			if (line.contains("export")) {
				line = reg_container.keyword_gdscript_export_multi.sub(line, "$1@export", true);
			}
			if (line.contains("onready")) {
				line = reg_container.keyword_gdscript_onready.sub(line, "@onready", true);
			}
			if (line.contains("remote")) {
				line = reg_container.keyword_gdscript_remote.sub(
					line, builtin_escape("@rpc(\"any_peer\") func", builtin), true);
			}
			if (line.contains("remote")) {
				line = reg_container.keyword_gdscript_remotesync.sub(
					line, builtin_escape("@rpc(\"any_peer\", \"call_local\") func", builtin), true);
			}
			if (line.contains("sync")) {
				line = reg_container.keyword_gdscript_sync.sub(
					line, builtin_escape("@rpc(\"any_peer\", \"call_local\") func", builtin), true);
			}
			if (line.contains("slave")) {
				line = reg_container.keyword_gdscript_slave.sub(line, "@rpc func", true);
			}
			if (line.contains("puppet")) {
				line = reg_container.keyword_gdscript_puppet.sub(line, "@rpc func", true);
			}
			if (line.contains("puppet")) {
				line = reg_container.keyword_gdscript_puppetsync.sub(
					line, builtin_escape("@rpc(\"call_local\") func", builtin), true);
			}
			if (line.contains("master")) {
				line = reg_container.keyword_gdscript_master.sub(
					line, error_message + "@rpc func", true);
			}
			if (line.contains("master")) {
				line = reg_container.keyword_gdscript_mastersync.sub(line,
					error_message + builtin_escape("@rpc(\"call_local\") func", builtin), true);
			}
		}
	}
}

Vector<String> ProjectConverter3To4::check_for_rename_gdscript_keywords(
	Vector<String>& lines, const RegExContainer& reg_container, bool builtin)
{
	Vector<String> found_renames;

	int current_line = 1;
	for (String& line : lines) {
		if (uint64_t(line.length()) <= maximum_line_length) {
			String old;

			if (line.contains("tool")) {
				old = line;
				line = reg_container.keyword_gdscript_tool.sub(line, "@tool", true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "tool", "@tool", line));
				}
			}

			if (line.contains("export")) {
				old = line;
				line = reg_container.keyword_gdscript_export_single.sub(line, "$1@export", true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "export", "@export", line));
				}
			}

			if (line.contains("export")) {
				old = line;
				line = reg_container.keyword_gdscript_export_multi.sub(line, "@export", true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "export", "@export", line));
				}
			}

			if (line.contains("onready")) {
				old = line;
				line = reg_container.keyword_gdscript_tool.sub(line, "@onready", true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "onready", "@onready", line));
				}
			}

			if (line.contains("remote")) {
				old = line;
				line = reg_container.keyword_gdscript_remote.sub(
					line, builtin_escape("@rpc(\"any_peer\") func", builtin), true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "remote func",
						builtin_escape("@rpc(\"any_peer\") func", builtin), line));
				}
			}

			if (line.contains("remote")) {
				old = line;
				line = reg_container.keyword_gdscript_remotesync.sub(line,
					builtin_escape("@rpc(\"any_peer\", \"call_local\")) func", builtin), true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "remotesync func",
						builtin_escape("@rpc(\"any_peer\", \"call_local\")) func", builtin), line));
				}
			}

			if (line.contains("sync")) {
				old = line;
				line = reg_container.keyword_gdscript_sync.sub(line,
					builtin_escape("@rpc(\"any_peer\", \"call_local\")) func", builtin), true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "sync func",
						builtin_escape("@rpc(\"any_peer\", \"call_local\")) func", builtin), line));
				}
			}

			if (line.contains("slave")) {
				old = line;
				line = reg_container.keyword_gdscript_slave.sub(line, "@rpc func", true);
				if (old != line) {
					found_renames.append(
						line_formatter(current_line, "slave func", "@rpc func", line));
				}
			}

			if (line.contains("puppet")) {
				old = line;
				line = reg_container.keyword_gdscript_puppet.sub(line, "@rpc func", true);
				if (old != line) {
					found_renames.append(
						line_formatter(current_line, "puppet func", "@rpc func", line));
				}
			}

			if (line.contains("puppet")) {
				old = line;
				line = reg_container.keyword_gdscript_puppetsync.sub(
					line, builtin_escape("@rpc(\"call_local\") func", builtin), true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "puppetsync func",
						builtin_escape("@rpc(\"call_local\") func", builtin), line));
				}
			}

			if (line.contains("master")) {
				old = line;
				line = reg_container.keyword_gdscript_master.sub(line, "@rpc func", true);
				if (old != line) {
					found_renames.append(
						line_formatter(current_line, "master func", "@rpc func", line));
				}
			}

			if (line.contains("master")) {
				old = line;
				line = reg_container.keyword_gdscript_master.sub(
					line, builtin_escape("@rpc(\"call_local\") func", builtin), true);
				if (old != line) {
					found_renames.append(line_formatter(current_line, "mastersync func",
						builtin_escape("@rpc(\"call_local\") func", builtin), line));
				}
			}
		}
		current_line++;
	}

	return found_renames;
}

void ProjectConverter3To4::custom_rename(
	Vector<SourceLine>& source_lines, const String& from, const String& to)
{
	RegEx reg = RegEx(String("\\b") + from + "\\b");
	CRASH_COND(!reg.is_valid());
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			line = reg.sub(line, to, true);
		}
	}
}

void ProjectConverter3To4::rename_common(const char* array[][2],
	LocalVector<Ref<RegEx>>& cached_regexes, Vector<SourceLine>& source_lines)
{
	for (SourceLine& source_line : source_lines) {
		if (source_line.is_comment) {
			continue;
		}

		String& line = source_line.line;
		if (uint64_t(line.length()) <= maximum_line_length) {
			for (unsigned int current_index = 0; current_index < cached_regexes.size();
				 current_index++) {
				if (line.contains(array[current_index][0])) {
					line = cached_regexes[current_index]->sub(line, array[current_index][1], true);
				}
			}
		}
	}
}

// Prints full info about renamed things e.g.:
// Line (67) remove -> remove_at  -  LINE """ doubler._blacklist.remove(0) """
String ProjectConverter3To4::line_formatter(int current_line, String from, String to, String line)
{
	if (from.size() > 200) {
		from = from.substr(0, 197) + "...";
	}
	if (to.size() > 200) {
		to = to.substr(0, 197) + "...";
	}
	if (line.size() > 400) {
		line = line.substr(0, 397) + "...";
	}

	from = from.strip_escapes();
	to = to.strip_escapes();
	line = line.remove_chars("\r\n").strip_edges();

	return vformat("Line(%d), %s -> %s  -  LINE \"\"\" %s \"\"\"", current_line, from, to, line);
}

// Prints only full lines e.g.:
// Line (1) - FULL LINES - """yield(get_tree().create_timer(3), 'timeout')"""  =====>  """ await
// get_tree().create_timer(3).timeout """
String ProjectConverter3To4::simple_line_formatter(
	int current_line, String old_line, String new_line)
{
	if (old_line.size() > 1000) {
		old_line = old_line.substr(0, 997) + "...";
	}
	if (new_line.size() > 1000) {
		new_line = new_line.substr(0, 997) + "...";
	}

	old_line = old_line.remove_chars("\r\n").strip_edges();
	new_line = new_line.remove_chars("\r\n").strip_edges();

	return vformat("Line (%d) - FULL LINES - \"\"\" %s \"\"\"  =====>  \"\"\" %s \"\"\"",
		current_line, old_line, new_line);
}

// Collects string from vector strings
String ProjectConverter3To4::collect_string_from_vector(Vector<SourceLine>& vector)
{
	String string = "";
	for (int i = 0; i < vector.size(); i++) {
		string += vector[i].line;

		if (i != vector.size() - 1) {
			string += "\n";
		}
	}
	return string;
}

#endif // DISABLE_DEPRECATED



void ProjectConverter3To4::rename_joypad_buttons_and_axes(Vector<SourceLine>&, ProjectConverter3To4::RegExContainer const&) {}

void ProjectConverter3To4::rename_input_map_scancode(Vector<SourceLine>&, ProjectConverter3To4::RegExContainer const&) {}

bool ProjectConverter3To4::validate_conversion() {}

bool ProjectConverter3To4::convert() {}

// ============================================================================
// Merged from editor/project_manager/project_manager.cpp
// ============================================================================

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "editor/editor_ui.h"
#include "editor/file_system/editor_paths.h"
#include "editor/editor_ui.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

String ProjectManager::_get_default_projects_dir()
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

void ProjectManager::_scan_dir_for_projects(
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
	VoltaireProjectFile tmp;
	if (_read_voltaire_project(cur_dir, &tmp)) {
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
					VoltaireProjectFile proj_data;
					if (_read_voltaire_project(entry.path, &proj_data)) {
						entry.missing = false;
						entry.name = String::utf8(proj_data.config.project_name);
					}
					else {
						entry.missing = true;
						entry.name = entry.path.get_file();
					}
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

	data->ui.initialize(p_viewport_rid, p_size, Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT),
		&ProjectManager::handle_input_event);

	data->search_field.placeholder = "Filter projects by name or path...";

	// Top toolbar buttons
	data->buttons.add("+ New Project", &ProjectManager::_new_project, nullptr, true);
	data->buttons.add("Import", &ProjectManager::_import_project);
	data->buttons.add("Scan", &ProjectManager::_scan_projects);
	data->buttons.add("Donate", &ProjectManager::_open_donate_page);

	// Right-hand project action sidebar buttons
	data->sidebar_buttons.add("Edit", &ProjectManager::_edit_selected_project,
		&ProjectManager::_has_valid_selected_project, true);
	data->sidebar_buttons.add("Run", &ProjectManager::_run_selected_project,
		&ProjectManager::_has_valid_selected_project);
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
			changed |= data->modal_primary_field.handle_mouse_motion(p_event, data->ui.font, 14);
			changed |= data->modal_secondary_field.handle_mouse_motion(p_event, data->ui.font, 14);
		}
		else {
			changed |= data->search_field.handle_mouse_motion(p_event, data->ui.font, 14);
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
					if (data->modal_primary_field.handle_mouse_button(p_event, data->ui.font, 14)) {
						data->active_field = FIELD_MODAL_PRIMARY;
					}
					else if (data->modal_mode == MODAL_NEW_PROJECT &&
							   data->modal_secondary_field.handle_mouse_button(
								   p_event, data->ui.font, 14)) {
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
				data->modal_primary_field.handle_mouse_button(p_event, data->ui.font, 14);
				data->modal_secondary_field.handle_mouse_button(p_event, data->ui.font, 14);
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

			if (data->search_field.handle_mouse_button(p_event, data->ui.font, 14)) {
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
			data->search_field.handle_mouse_button(p_event, data->ui.font, 14);
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
			if (data->active_field == FIELD_MODAL_PRIMARY) {
				data->active_field = FIELD_MODAL_SECONDARY;
				data->modal_secondary_field.select_all();
			}
			else {
				data->active_field = FIELD_MODAL_PRIMARY;
				data->modal_primary_field.select_all();
			}
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
			font->draw_string(ci, Point2(pad + 18.0f, title_base_y), "VOLTAIRE // PROJECT MANAGER",
				HORIZONTAL_ALIGNMENT_LEFT, title_max_w, 16, col_text);
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
				String empty_msg =
					data->projects.is_empty()
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
					proj.missing ? col_warn
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
			label_1 = "Project Folder or project.vltr Path:";
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
		Renderer::canvas_item_add_rect(
			ci, Rect2(mx + 18.0f, cy, inner_w, 1.0f), modal_field_border);
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
		const float total_mbtns_w = data->modal_buttons.size() * (mbtn_w + mbtn_gap) - mbtn_gap;

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
	data->modal_buttons.add(
		"Create & Edit", &ProjectManager::_confirm_modal_and_edit, nullptr, true);
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
				VoltaireProjectFile proj_data;
				if (_read_voltaire_project(clean, &proj_data)) {
					data->projects[i].missing = false;
					data->projects[i].name = String::utf8(proj_data.config.project_name);
				}
				else {
					data->projects[i].missing = true;
					data->projects[i].name = clean.get_file();
				}
				return (int)i;
			}
		}
		ProjectEntry entry;
		entry.path = clean;
		VoltaireProjectFile proj_data;
		if (_read_voltaire_project(entry.path, &proj_data)) {
			entry.missing = false;
			entry.name = String::utf8(proj_data.config.project_name);
		}
		else {
			entry.missing = true;
			entry.name = entry.path.get_file();
		}
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
		String proj_file = target_dir.path_join("project.vltr");
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
			data->modal_error_text = "Failed to write project.vltr.";
			data->ui.needs_redraw = true;
			return;
		}

		VoltaireProjectFile new_proj;
		vltr_project_init(&new_proj, proj_name.utf8().get_data(), "res://MainScene.tscn");
		fa->store_buffer((const uint8_t*)&new_proj, sizeof(new_proj));
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
		VoltaireProjectFile tmp;
		String raw_path = data->modal_primary_field.text.strip_edges().simplify_path();
		if (raw_path.is_empty() || !_read_voltaire_project(raw_path, &tmp)) {
			data->modal_error_text = "Invalid or missing project.vltr in directory.";
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

bool ProjectManager::_read_voltaire_project(const String& p_dir, VoltaireProjectFile* r_out)
{
	String file_path = p_dir.path_join("project.vltr");
	Ref<FileAccess> fa = FileAccess::open(file_path, FileAccess::READ);
	if (fa.is_null() || fa->get_length() < (int64_t)sizeof(VoltaireProjectFile)) {
		return false;
	}
	fa->get_buffer((uint8_t*)r_out, sizeof(VoltaireProjectFile));
	return vltr_project_valid(r_out);
}

// ============================================================================
// Merged from editor/settings/editor_settings.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/certs_compressed.gen.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/ip.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "core/templates/rb_set.h"
#include "core/version.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "main/main.h"
#include "modules/regex/regex.h"
#include "scene/gui/dialogs.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/animation.h"
#include "servers/display/display_server.h"

// PRIVATE METHODS

// Properties

struct _EVCSort
{
	String name;
	int order = 0;
	bool basic = false;
	bool save = false;
	bool restart_if_changed = false;

	bool operator<(const _EVCSort& p_vcs) const { return order < p_vcs.order; }
};

// Default configs

void EditorSettings::_set_initialized()
{
	if (data) {
		data->initialized = true;
	}
}

static LocalVector<String> _get_skipped_locales()
{
	// Skip locales if Text server lack required features.
	LocalVector<String> locales_to_skip;
	if (!TS->has_feature(TextServer::FEATURE_BIDI_LAYOUT) ||
		!TS->has_feature(TextServer::FEATURE_SHAPING)) {
		locales_to_skip.push_back("ar"); // Arabic.
		locales_to_skip.push_back("fa"); // Persian.
		locales_to_skip.push_back("ur"); // Urdu.
	}
	if (!TS->has_feature(TextServer::FEATURE_BIDI_LAYOUT)) {
		locales_to_skip.push_back("he"); // Hebrew.
	}
	if (!TS->has_feature(TextServer::FEATURE_SHAPING)) {
		locales_to_skip.push_back("bn"); // Bengali.
		locales_to_skip.push_back("hi"); // Hindi.
		locales_to_skip.push_back("ml"); // Malayalam.
		locales_to_skip.push_back("si"); // Sinhala.
		locales_to_skip.push_back("ta"); // Tamil.
		locales_to_skip.push_back("te"); // Telugu.
	}
	return locales_to_skip;
}

String EditorSettings::_get_project_metadata_path()
{
	return EditorPaths::get_project_settings_dir().path_join(
		"project_metadata.cfg");
}

#ifndef DISABLE_DEPRECATED
void EditorSettings::_handle_setting_compatibility()
{
	// Remove deprecated settings.
	erase("interface/theme/preset");
	erase("network/connection/engine_version_update_mode");
	erase("run/output/always_open_output_on_play");
	erase("run/output/always_close_output_on_stop");
	erase("text_editor/theme/line_spacing"); // See GH-106137.
	erase("interface/editors/show_scene_tree_root_selection");
	erase("asset_library/available_urls"); // Workaround bugged settings treating the previous
										   // default as a modified value (see GH-118755).

	// Handle renamed settings.
	_rename_setting(
		"interface/editor/editor_language", "interface/editor/localization/editor_language");
	_rename_setting(
		"interface/editor/localize_settings", "interface/editor/localization/localize_settings");
	_rename_setting("interface/editor/dock_tab_style", "interface/editor/docks/dock_tab_style");
	_rename_setting(
		"interface/editor/bottom_dock_tab_style", "interface/editor/docks/bottom_dock_tab_style");
	_rename_setting("interface/editor/ui_layout_direction",
		"interface/editor/localization/ui_layout_direction");
	_rename_setting("interface/editor/display_scale", "interface/editor/appearance/display_scale");
	_rename_setting("interface/editor/custom_display_scale",
		"interface/editor/appearance/custom_display_scale");
	_rename_setting("interface/editor/editor_screen", "interface/editor/appearance/editor_screen");
	_rename_setting("interface/editor/tablet_driver", "interface/editor/input/tablet_driver");
	_rename_setting("interface/editor/project_manager_screen",
		"interface/editor/appearance/project_manager_screen");
	_rename_setting(
		"interface/editor/use_embedded_menu", "interface/editor/appearance/use_embedded_menu");
	_rename_setting("interface/editor/use_native_file_dialogs",
		"interface/editor/appearance/use_native_file_dialogs");
	_rename_setting(
		"interface/editor/expand_to_title", "interface/editor/appearance/expand_to_title");
	_rename_setting("interface/editor/main_font_size", "interface/editor/fonts/main_font_size");
	_rename_setting("interface/editor/code_font_size", "interface/editor/fonts/code_font_size");
	_rename_setting("interface/editor/main_font_custom_opentype_features",
		"interface/editor/fonts/main_font_custom_opentype_features");
	_rename_setting("interface/editor/code_font_contextual_ligatures",
		"interface/editor/fonts/code_font_contextual_ligatures");
	_rename_setting("interface/editor/code_font_custom_opentype_features",
		"interface/editor/fonts/code_font_custom_opentype_features");
	_rename_setting("interface/editor/code_font_custom_variations",
		"interface/editor/fonts/code_font_custom_variations");
	_rename_setting(
		"interface/editor/font_antialiasing", "interface/editor/fonts/font_antialiasing");
	_rename_setting("interface/editor/font_hinting", "interface/editor/fonts/font_hinting");
	_rename_setting("interface/editor/font_subpixel_positioning",
		"interface/editor/fonts/font_subpixel_positioning");
	_rename_setting("interface/editor/font_disable_embedded_bitmaps",
		"interface/editor/fonts/font_disable_embedded_bitmaps");
	_rename_setting("interface/editor/font_allow_msdf", "interface/editor/fonts/font_allow_msdf");
	_rename_setting("interface/editor/main_font", "interface/editor/fonts/main_font");
	_rename_setting("interface/editor/main_font_bold", "interface/editor/fonts/main_font_bold");
	_rename_setting("interface/editor/code_font", "interface/editor/fonts/code_font");
	_rename_setting("interface/editor/dragging_hover_wait_seconds",
		"interface/editor/timers/dragging_hover_wait_seconds");
	_rename_setting("interface/editor/separate_distraction_mode",
		"interface/editor/behavior/separate_distraction_mode");
	_rename_setting("interface/editor/automatically_open_screenshots",
		"interface/editor/behavior/automatically_open_screenshots");
	_rename_setting(
		"interface/editor/single_window_mode", "interface/editor/display/single_window_mode");
	_rename_setting("interface/editor/mouse_extra_buttons_navigate_history",
		"interface/editor/input/mouse_extra_buttons_navigate_history");
	_rename_setting("interface/editor/save_each_scene_on_quit",
		"interface/editor/behavior/save_each_scene_on_quit");
	_rename_setting(
		"interface/editor/save_on_focus_loss", "interface/editor/behavior/save_on_focus_loss");
	_rename_setting("interface/editor/accept_dialog_cancel_ok_buttons",
		"interface/editor/appearance/accept_dialog_cancel_ok_buttons");
	_rename_setting("interface/editor/show_internal_errors_in_toast_notifications",
		"interface/editor/behavior/show_internal_errors_in_toast_notifications");
	_rename_setting(
		"interface/editor/show_update_spinner", "interface/editor/appearance/show_update_spinner");
	_rename_setting("interface/editor/keep_screen_on", "interface/editor/display/keep_screen_on");
	_rename_setting("interface/editor/low_processor_mode_sleep_usec",
		"interface/editor/timers/low_processor_mode_sleep_usec");
	_rename_setting("interface/editor/unfocused_low_processor_mode_sleep_usec",
		"interface/editor/timers/unfocused_low_processor_mode_sleep_usec");
	_rename_setting("interface/editor/import_resources_when_unfocused",
		"interface/editor/behavior/import_resources_when_unfocused");
	_rename_setting("interface/editor/vsync_mode", "interface/editor/display/vsync_mode");
	_rename_setting(
		"interface/editor/update_continuously", "interface/editor/display/update_continuously");
	_rename_setting(
		"interface/editor/collapse_main_menu", "interface/editor/appearance/collapse_main_menu");
	_rename_setting("asset_library/use_threads", "asset_store/use_threads");
	_rename_setting("interface/editors/derive_script_globals_by_name",
		"docks/scene_tree/derive_script_globals_by_name");

	// Handle renamed shortcuts.
	_rename_shortcut("editor/editor_assetlib", "editor/editor_asset_store");
}

void EditorSettings::_rename_shortcut(const String& p_old_path, const String& p_new_path)
{
	if (!data || !data->shortcuts.has(p_old_path)) {
		return;
	}
	if (!data->shortcuts.has(p_new_path)) {
		data->shortcuts[p_new_path] = data->shortcuts[p_old_path];
	}
	data->shortcuts.erase(p_old_path);
}
#endif

// PUBLIC METHODS

void EditorSettings::initialize()
{
	if (!data) {
		data = memnew(Data);
		data->project_metadata.instantiate();
		data->initialized = true;
	}
}

void EditorSettings::finalize()
{
	if (!data) {
		return;
	}
	save();
	memdelete(data);
	data = nullptr;
}

String EditorSettings::get_existing_settings_path()
{
	const String config_dir = EditorPaths::get_config_dir();
	int minor = VLTR_VERSION_MINOR;
	String filename;

	do {
		if (VLTR_VERSION_MAJOR == 4 && minor < 3) {
			// Minor version is used since 4.3, so special case to load older settings.
			filename = vformat("editor_settings-%d.tres", VLTR_VERSION_MAJOR);
			minor = -1;
		}
		else {
			filename = vformat("editor_settings-%d.%d.tres", VLTR_VERSION_MAJOR, minor);
			minor--;
		}
	} while (minor >= 0 && !FileAccess::exists(config_dir.path_join(filename)));
	return config_dir.path_join(filename);
}

String EditorSettings::get_newest_settings_path()
{
	const String config_file_name =
		vformat("editor_settings-%d.%d.tres", VLTR_VERSION_MAJOR, VLTR_VERSION_MINOR);
	return EditorPaths::get_config_dir().path_join(config_file_name);
}

void EditorSettings::init_shortcuts()
{
	ED_SHORTCUT("editor/open_search", TTRC("Focus Search/Filter Bar"),
		KeyModifierMask::CMD_OR_CTRL | Key::F);
}

void EditorSettings::save()
{
	if (!data) {
		return;
	}

	data->changed_settings.clear();
	print_verbose("EditorSettings: Save OK!");
}

PackedStringArray EditorSettings::get_changed_settings()
{
	PackedStringArray arr;
	if (!data) {
		return arr;
	}
	for (const String& setting : data->changed_settings) {
		arr.push_back(setting);
	}

	return arr;
}

bool EditorSettings::check_changed_settings_in_group(const String& p_setting_prefix)
{
	if (!data) {
		return false;
	}
	for (const String& setting : data->changed_settings) {
		if (setting.begins_with(p_setting_prefix)) {
			return true;
		}
	}

	return false;
}

void EditorSettings::mark_setting_changed(const String& p_setting)
{
	if (!data) {
		return;
	}
	data->changed_settings.insert(p_setting);
}

void EditorSettings::set_optimize_save(bool p_optimize)
{
	if (data) {
		data->optimize_save = p_optimize;
	}
}

// Properties

// Metadata

void EditorSettings::save_project_metadata()
{
	if (!data || !data->project_metadata_dirty) {
		return;
	}
	const String path = _get_project_metadata_path();
	Error err = data->project_metadata->save(path);
	ERR_FAIL_COND_MSG(err != OK, "Cannot save project metadata to file '" + path + "'.");
	data->project_metadata_dirty = false;
}

void EditorSettings::set_favorites_bind(const Vector<String>& p_favorites)
{
	if (!data) {
		return;
	}
	data->favorites = p_favorites;
	String favorites_file;
	if (Engine::is_project_manager_hint()) {
		favorites_file = EditorPaths::get_config_dir().path_join("favorite_dirs");
	}
	else {
		favorites_file =
			EditorPaths::get_project_settings_dir().path_join("favorites");
	}
	Ref<FileAccess> f = FileAccess::open(favorites_file, FileAccess::WRITE);
	if (f.is_valid()) {
		for (int i = 0; i < data->favorites.size(); i++) {
			f->store_line(data->favorites[i]);
		}
	}
}

Vector<String> EditorSettings::get_favorites()
{
	return data ? data->favorites : Vector<String>();
}

Vector<String> EditorSettings::get_favorite_folders()
{
	Vector<String> folder_favorites;
	if (!data) {
		return folder_favorites;
	}
	folder_favorites.resize(data->favorites.size());
	String* folder_write = folder_favorites.ptrw();

	int i = 0;
	for (const String& fav : data->favorites) {
		if (fav.ends_with("/")) {
			folder_write[i] = fav;
			i++;
		}
	}
	folder_favorites.resize(i);
	return folder_favorites;
}

HashMap<String, PackedStringArray> EditorSettings::get_favorite_properties()
{
	return data ? HashMap<String, PackedStringArray>(data->favorite_properties)
				: HashMap<String, PackedStringArray>();
}

void EditorSettings::set_recent_dirs(const Vector<String>& p_recent_dirs, bool p_update_file_dialog)
{
	if (p_update_file_dialog) {
		FileDialog::set_recent_list(p_recent_dirs);
	}
	set_recent_dirs_bind(p_recent_dirs);
}

void EditorSettings::set_recent_dirs_bind(const Vector<String>& p_recent_dirs)
{
	if (!data) {
		return;
	}
	data->recent_dirs = p_recent_dirs;
	String recent_dirs_file;
	if (Engine::is_project_manager_hint()) {
		recent_dirs_file = EditorPaths::get_config_dir().path_join("recent_dirs");
	}
	else {
		recent_dirs_file =
			EditorPaths::get_project_settings_dir().path_join("recent_dirs");
	}
	Ref<FileAccess> f = FileAccess::open(recent_dirs_file, FileAccess::WRITE);
	if (f.is_valid()) {
		for (int i = 0; i < data->recent_dirs.size(); i++) {
			f->store_line(data->recent_dirs[i]);
		}
	}
}

Vector<String> EditorSettings::get_recent_dirs()
{
	return data ? data->recent_dirs : Vector<String>();
}

HashMap<String, Color> EditorSettings::get_godot2_text_editor_theme()
{
	// Godot 2 is only a dark theme; it doesn't have a light theme counterpart.
	HashMap<String, Color> colors;
	colors["text_editor/theme/highlighting/symbol_color"] = Color(0.73, 0.87, 1.0);
	colors["text_editor/theme/highlighting/keyword_color"] = Color(1.0, 1.0, 0.7);
	colors["text_editor/theme/highlighting/control_flow_keyword_color"] = Color(1.0, 0.85, 0.7);
	colors["text_editor/theme/highlighting/base_type_color"] = Color(0.64, 1.0, 0.83);
	colors["text_editor/theme/highlighting/engine_type_color"] = Color(0.51, 0.83, 1.0);
	colors["text_editor/theme/highlighting/user_type_color"] = Color(0.42, 0.67, 0.93);
	colors["text_editor/theme/highlighting/comment_color"] = Color(0.4, 0.4, 0.4);
	colors["text_editor/theme/highlighting/doc_comment_color"] = Color(0.5, 0.6, 0.7);
	colors["text_editor/theme/highlighting/string_color"] = Color(0.94, 0.43, 0.75);
	colors["text_editor/theme/highlighting/string_placeholder_color"] = Color(1, 0.75, 0.4);
	colors["text_editor/theme/highlighting/background_color"] = Color(0.13, 0.12, 0.15);
	colors["text_editor/theme/highlighting/completion_background_color"] = Color(0.17, 0.16, 0.2);
	colors["text_editor/theme/highlighting/completion_selected_color"] = Color(0.26, 0.26, 0.27);
	colors["text_editor/theme/highlighting/completion_existing_color"] =
		Color(0.87, 0.87, 0.87, 0.13);
	colors["text_editor/theme/highlighting/completion_scroll_color"] = Color(1, 1, 1, 0.29);
	colors["text_editor/theme/highlighting/completion_scroll_hovered_color"] = Color(1, 1, 1, 0.4);
	colors["text_editor/theme/highlighting/completion_font_color"] = Color(0.67, 0.67, 0.67);
	colors["text_editor/theme/highlighting/text_color"] = Color(0.67, 0.67, 0.67);
	colors["text_editor/theme/highlighting/line_number_color"] = Color(0.67, 0.67, 0.67, 0.4);
	colors["text_editor/theme/highlighting/safe_line_number_color"] = Color(0.67, 0.78, 0.67, 0.6);
	colors["text_editor/theme/highlighting/caret_color"] = Color(0.67, 0.67, 0.67);
	colors["text_editor/theme/highlighting/caret_background_color"] = Color(0, 0, 0);
	colors["text_editor/theme/highlighting/text_selected_color"] = Color(0, 0, 0, 0);
	colors["text_editor/theme/highlighting/selection_color"] = Color(0.41, 0.61, 0.91, 0.35);
	colors["text_editor/theme/highlighting/brace_mismatch_color"] = Color(1, 0.2, 0.2);
	colors["text_editor/theme/highlighting/current_line_color"] = Color(0.3, 0.5, 0.8, 0.15);
	colors["text_editor/theme/highlighting/line_length_guideline_color"] =
		Color(0.3, 0.5, 0.8, 0.1);
	colors["text_editor/theme/highlighting/word_highlighted_color"] = Color(0.8, 0.9, 0.9, 0.15);
	colors["text_editor/theme/highlighting/number_color"] = Color(0.92, 0.58, 0.2);
	colors["text_editor/theme/highlighting/function_color"] = Color(0.4, 0.64, 0.81);
	colors["text_editor/theme/highlighting/member_variable_color"] = Color(0.9, 0.31, 0.35);
	colors["text_editor/theme/highlighting/mark_color"] = Color(1.0, 0.4, 0.4, 0.4);
	colors["text_editor/theme/highlighting/warning_color"] = Color(1.0, 0.8, 0.4, 0.1);
	colors["text_editor/theme/highlighting/bookmark_color"] = Color(0.08, 0.49, 0.98);
	colors["text_editor/theme/highlighting/breakpoint_color"] = Color(0.9, 0.29, 0.3);
	colors["text_editor/theme/highlighting/executing_line_color"] = Color(0.98, 0.89, 0.27);
	colors["text_editor/theme/highlighting/code_folding_color"] = Color(0.8, 0.8, 0.8, 0.8);
	colors["text_editor/theme/highlighting/folded_code_region_color"] =
		Color(0.68, 0.46, 0.77, 0.2);
	colors["text_editor/theme/highlighting/search_result_color"] = Color(0.05, 0.25, 0.05, 1);
	colors["text_editor/theme/highlighting/search_result_border_color"] =
		Color(0.41, 0.61, 0.91, 0.38);
	colors["text_editor/theme/highlighting/gdscript/function_definition_color"] =
		Color(0.4, 0.9, 1.0);

	colors["text_editor/theme/highlighting/gdscript/global_function_color"] =
		Color(0.64, 0.64, 0.96);
	colors["text_editor/theme/highlighting/gdscript/node_path_color"] = Color(0.72, 0.77, 0.49);
	colors["text_editor/theme/highlighting/gdscript/node_reference_color"] =
		Color(0.39, 0.76, 0.35);
	colors["text_editor/theme/highlighting/gdscript/annotation_color"] = Color(1.0, 0.7, 0.45);
	colors["text_editor/theme/highlighting/gdscript/string_name_color"] = Color(1.0, 0.76, 0.65);
	colors["text_editor/theme/highlighting/comment_markers/critical_color"] =
		Color(0.77, 0.35, 0.35);
	colors["text_editor/theme/highlighting/comment_markers/warning_color"] =
		Color(0.72, 0.61, 0.48);
	colors["text_editor/theme/highlighting/comment_markers/notice_color"] = Color(0.56, 0.67, 0.51);
	return colors;
}

bool EditorSettings::is_default_text_editor_theme(const String& p_theme_name)
{
	return p_theme_name == "default" || p_theme_name == "godot 2" || p_theme_name == "custom";
}

Vector<String> EditorSettings::get_script_templates(
	const String& p_extension, const String& p_custom_path)
{
	Vector<String> templates;
	String template_dir = EditorPaths::get_script_templates_dir();
	if (!p_custom_path.is_empty()) {
		template_dir = p_custom_path;
	}
	Ref<DirAccess> d = DirAccess::open(template_dir);
	if (d.is_valid()) {
		d->list_dir_begin();
		String file = d->get_next();
		while (!file.is_empty()) {
			if (file.get_extension() == p_extension) {
				templates.push_back(file.get_basename());
			}
			file = d->get_next();
		}
		d->list_dir_end();
	}
	return templates;
}

String EditorSettings::get_editor_layouts_config()
{
	return EditorPaths::get_config_dir().path_join("editor_layouts.cfg");
}

float EditorSettings::get_auto_display_scale()
{
#ifdef LINUXBSD_ENABLED
	if (DisplayServer::get_name() == "Wayland") {
		float main_window_scale = DisplayServer::screen_get_scale(
			DisplayServerEnums::SCREEN_OF_MAIN_WINDOW);

		if (DisplayServer::get_screen_count() == 1 ||
			Math::fract(main_window_scale) != 0) {
			// If we have a single screen or the screen of the window is fractional, all
			// bets are off. At this point, let's just return the current's window scale,
			// which is special-cased to the scale of `DisplayServerEnums::SCREEN_OF_MAIN_WINDOW`.
			return main_window_scale;
		}

		// If the above branch didn't fire, fractional scaling isn't going to work
		// properly anyways (we're need the ability to change the UI scale at runtime).
		// At this point it's more convenient to "supersample" like we do with other
		// platforms, hoping that the user is only using integer-scaled screens.
		return DisplayServer::screen_get_max_scale();
	}
#endif

#if defined(MACOS_ENABLED) || defined(ANDROID_ENABLED)
	return DisplayServer::screen_get_max_scale();
#else
	const int screen = DisplayServer::window_get_current_screen();

	if (DisplayServer::screen_get_size(screen) == Vector2i()) {
		// Invalid screen size, skip.
		return 1.0;
	}

#if defined(WINDOWS_ENABLED)
	return DisplayServer::screen_get_dpi(screen) / 96.0;
#else
	// Use the smallest dimension to use a correct display scale on portrait displays.
	const int smallest_dimension = MIN(DisplayServer::screen_get_size(screen).x,
		DisplayServer::screen_get_size(screen).y);
	if (DisplayServer::screen_get_dpi(screen) >= 192 &&
		smallest_dimension >= 1400) {
		// hiDPI display.
		return 2.0;
	}
	else if (smallest_dimension >= 1700) {
		// Likely a hiDPI display, but we aren't certain due to the returned DPI.
		// Use an intermediate scale to handle this situation.
		return 1.5;
	}
	else if (smallest_dimension <= 800) {
		// Small loDPI display. Use a smaller display scale so that editor elements fit more easily.
		// Icons won't look great, but this is better than having editor elements overflow from its
		// window.
		return 0.75;
	}
	return 1.0;
#endif // defined(WINDOWS_ENABLED)

#endif // defined(MACOS_ENABLED) || defined(ANDROID_ENABLED)
}

// Shortcuts

void EditorSettings::_add_shortcut_default(const String& p_path, const Ref<Shortcut>& p_shortcut)
{
	if (data) {
		data->shortcuts[p_path] = p_shortcut;
	}
}

void EditorSettings::remove_shortcut(const String& p_path)
{
	if (data) {
		data->shortcuts.erase(p_path);
	}
}

bool EditorSettings::is_shortcut(const String& p_path, const InputEvent& p_event)
{
	if (!data) {
		return false;
	}
	HashMap<String, Ref<Shortcut>>::ConstIterator E = data->shortcuts.find(p_path);
	ERR_FAIL_COND_V_MSG(!E, false, "Unknown Shortcut: " + p_path + ".");

	return E->value->matches_event(p_event);
}

bool EditorSettings::has_shortcut(const String& p_path)
{
	return get_shortcut(p_path).is_valid();
}

Ref<Shortcut> EditorSettings::get_shortcut(const String& p_path)
{
	if (!data) {
		return Ref<Shortcut>();
	}
	HashMap<String, Ref<Shortcut>>::ConstIterator SC = data->shortcuts.find(p_path);
	if (SC) {
		return SC->value;
	}

	// If no shortcut with the provided name is found in the list, check the built-in shortcuts.
	// Use the first item in the action list for the shortcut event, since a shortcut can only have
	// 1 linked event.

	Ref<Shortcut> sc;
	HashMap<String, List<InputEvent>>::ConstIterator builtin_override =
		data->builtin_action_overrides.find(p_path);
	if (builtin_override) {
		sc.instantiate();
		sc->set_events_list(&builtin_override->value);
		sc->set_name(InputMap::get_builtin_display_name(p_path));
	}

	// If there was no override, check the default builtins to see if it has an InputEvent for the
	// provided name.
	if (sc.is_null()) {
		HashMap<String, List<InputEvent>>::ConstIterator builtin_default =
			InputMap::get_builtins_with_feature_overrides_applied().find(p_path);
		if (builtin_default) {
			sc.instantiate();
			sc->set_events_list(&builtin_default->value);
			sc->set_name(InputMap::get_builtin_display_name(p_path));
		}
	}

	if (sc.is_valid()) {
		// Add the shortcut to the list.
		data->shortcuts[p_path] = sc;
		return sc;
	}

	return Ref<Shortcut>();
}

Vector<String> EditorSettings::_get_shortcut_list()
{
	List<String> shortcut_list;
	get_shortcut_list(&shortcut_list);
	Vector<String> ret;
	for (const String& shortcut : shortcut_list) {
		ret.push_back(shortcut);
	}
	return ret;
}

void EditorSettings::get_shortcut_list(List<String>* r_shortcuts)
{
	if (!data) {
		return;
	}
	for (const KeyValue<String, Ref<Shortcut>>& E : data->shortcuts) {
		r_shortcuts->push_back(E.key);
	}
}

Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path)
{
	ERR_FAIL_COND_V_MSG(
		!EditorSettings::is_initialized(), nullptr, "EditorSettings not instantiated yet.");

	Ref<Shortcut> sc = EditorSettings::get_shortcut(p_path);

	ERR_FAIL_COND_V_MSG(sc.is_null(), sc, "Used ED_GET_SHORTCUT with invalid shortcut: " + p_path);

	return sc;
}

void ED_SHORTCUT_OVERRIDE(
	const String& p_path, const String& p_feature, Key p_keycode, bool p_physical)
{
	if (!EditorSettings::is_initialized()) {
		return;
	}

	Ref<Shortcut> sc = EditorSettings::get_shortcut(p_path);
	ERR_FAIL_COND_MSG(sc.is_null(), "Used ED_SHORTCUT_OVERRIDE with invalid shortcut: " + p_path);

	PackedInt32Array arr;
	arr.push_back((int32_t)p_keycode);

	ED_SHORTCUT_OVERRIDE_ARRAY(p_path, p_feature, arr, p_physical);
}

Ref<Shortcut> ED_SHORTCUT(
	const String& p_path, const String& p_name, Key p_keycode, bool p_physical)
{
	PackedInt32Array arr;
	arr.push_back((int32_t)p_keycode);
	return ED_SHORTCUT_ARRAY(p_path, p_name, arr, p_physical);
}

void EditorSettings::_rename_setting(const String& p_old_name, const String& p_new_name) {}

void EditorSettings::erase(const String& p_setting) {}

bool EditorSettings::has_setting(const String& p_setting) { return false; }

String EditorSettings::get_language() { return String(); }

void EditorSettings::set_favorites(const Vector<String>& p_favorites, bool p_update_file_dialog) {}

void EditorSettings::set_favorite_properties(
	const HashMap<String, PackedStringArray>& p_favorite_properties)
{
}

void EditorSettings::notify_changes() {}

void ED_SHORTCUT_OVERRIDE_ARRAY(const String& p_path, const String& p_feature,
	const PackedInt32Array& p_keycodes, bool p_physical)
{
}

Ref<Shortcut> ED_SHORTCUT_ARRAY(
	const String& p_path, const String& p_name, const PackedInt32Array& p_keycodes, bool p_physical)
{
	Ref<Shortcut> s = memnew(Shortcut);
	return s;
}



// ============================================================================
// Merged from editor/themes/editor_scale.cpp
// ============================================================================

#include "editor/themes/editor_scale.h"

float EditorScale::_scale = 1.0f;

void EditorScale::set_scale(float p_scale) {
	_scale = p_scale;
}

float EditorScale::get_scale() {
	return _scale;
}

// ============================================================================
// Merged from editor/translations/editor_translation.cpp
// ============================================================================

#include "editor/settings/editor_settings.h"

#include "core/io/compression.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/config_file.h"
#include "core/string/translation_server.h"
#include "editor/translations/doc_translations.gen.h"
#include "editor/translations/editor_translations.gen.h"
#include "editor/translations/extractable_translations.gen.h"
#include "editor/translations/property_translations.gen.h"

Vector<String> get_editor_locales() {
	Vector<String> locales;

	for (const EditorTranslationList *etl = _editor_translations; etl->data; etl++) {
		const String &locale = etl->lang;
		locales.push_back(locale);
	}

	return locales;
}

static void _load(const Ref<TranslationDomain> p_domain, const String &p_locale, const EditorTranslationList *p_etl) {
	for (const EditorTranslationList *etl = p_etl; etl->data; etl++) {
		if (etl->lang == p_locale) {
			LocalVector<uint8_t> data;
			data.resize_uninitialized(etl->uncomp_size);
			const int64_t ret = Compression::decompress(data.ptr(), etl->uncomp_size, etl->data, etl->comp_size, Compression::MODE_DEFLATE);
			ERR_FAIL_COND_MSG(ret == -1, "Compressed file is corrupt.");

			Ref<FileAccessMemory> fa;
			fa.instantiate();
			fa->open_custom(data.ptr(), data.size());

			Ref<Translation> tr = TranslationLoaderPO::load_translation(fa);
			if (tr.is_valid()) {
				tr->set_locale(etl->lang);
				p_domain->add_translation(tr);
				break;
			}
		}
	}
}

void load_editor_translations(const String &p_locale) {
	Ref<TranslationDomain> domain;

	domain = TranslationServer::get_editor_domain();
	domain->clear();
	_load(domain, p_locale, _editor_translations);
	_load(domain, p_locale, _extractable_translations);

	domain = TranslationServer::get_property_domain();
	domain->clear();
	_load(domain, p_locale, _property_translations);
}

void load_doc_translations(const String &p_locale) {
	const Ref<TranslationDomain> domain = TranslationServer::get_doc_domain();
	domain->clear();
	_load(domain, p_locale, _doc_translations);
}

Vector<Vector<String>> get_extractable_message_list() {
	Vector<Vector<String>> list;

	for (const EditorTranslationList *etl = _extractable_translations; etl->data; etl++) {
		if (strcmp(etl->lang, "source")) {
			continue;
		}

		LocalVector<uint8_t> data;
		data.resize_uninitialized(etl->uncomp_size);
		const int64_t ret = Compression::decompress(data.ptr(), etl->uncomp_size, etl->data, etl->comp_size, Compression::MODE_DEFLATE);
		ERR_FAIL_COND_V_MSG(ret == -1, list, "Compressed file is corrupt.");

		Ref<FileAccessMemory> fa;
		fa.instantiate();
		fa->open_custom(data.ptr(), data.size());

		// Taken from TranslationLoaderPO, modified to work specifically with POTs.
		{
			const String path = fa->get_path();

			fa->seek(0);

			enum Status {
				STATUS_NONE,
				STATUS_READING_ID,
				STATUS_READING_STRING,
				STATUS_READING_CONTEXT,
				STATUS_READING_PLURAL,
			};

			Status status = STATUS_NONE;

			String msg_id;
			String msg_id_plural;
			String msg_context;

			int line = 1;
			bool entered_context = false;
			bool is_eof = false;

			while (!is_eof) {
				String l = fa->get_line().strip_edges();
				is_eof = fa->eof_reached();

				// If we reached last line and it's not a content line, break, otherwise let processing that last loop.
				if (is_eof && l.is_empty()) {
					if (status == STATUS_READING_ID || status == STATUS_READING_CONTEXT || status == STATUS_READING_PLURAL) {
						ERR_FAIL_V_MSG(Vector<Vector<String>>(), "Unexpected EOF while reading POT file at: " + path + ":" + itos(line));
					} else {
						break;
					}
				}

				if (l.begins_with("msgctxt")) {
					ERR_FAIL_COND_V_MSG(status != STATUS_READING_STRING && status != STATUS_READING_PLURAL, Vector<Vector<String>>(),
							"Unexpected 'msgctxt', was expecting 'msgid_plural' or 'msgstr' before 'msgctxt' while parsing: " + path + ":" + itos(line));

					// In POT files, "msgctxt" appears before "msgid". If we encounter a "msgctxt", we add what we have read
					// and set "entered_context" to true to prevent adding twice.
					if (!msg_id.is_empty()) {
						Vector<String> msgs;
						msgs.push_back(msg_id);
						msgs.push_back(msg_context);
						msgs.push_back(msg_id_plural);
						list.push_back(msgs);
					}
					msg_context = "";
					l = l.substr(7).strip_edges();
					status = STATUS_READING_CONTEXT;
					entered_context = true;
				}

				if (l.begins_with("msgid_plural")) {
					if (status != STATUS_READING_ID) {
						ERR_FAIL_V_MSG(Vector<Vector<String>>(), "Unexpected 'msgid_plural', was expecting 'msgid' before 'msgid_plural' while parsing: " + path + ":" + itos(line));
					}
					l = l.substr(12).strip_edges();
					status = STATUS_READING_PLURAL;
				} else if (l.begins_with("msgid")) {
					ERR_FAIL_COND_V_MSG(status == STATUS_READING_ID, Vector<Vector<String>>(), "Unexpected 'msgid', was expecting 'msgstr' while parsing: " + path + ":" + itos(line));

					if (!msg_id.is_empty() && !entered_context) {
						Vector<String> msgs;
						msgs.push_back(msg_id);
						msgs.push_back(msg_context);
						msgs.push_back(msg_id_plural);
						list.push_back(msgs);
					}

					l = l.substr(5).strip_edges();
					status = STATUS_READING_ID;
					// If we did not encounter msgctxt, we reset context to empty to reset it.
					if (!entered_context) {
						msg_context = "";
					}
					msg_id = "";
					msg_id_plural = "";
					entered_context = false;
				}

				if (l.begins_with("msgstr[")) {
					ERR_FAIL_COND_V_MSG(status != STATUS_READING_PLURAL, Vector<Vector<String>>(),
							"Unexpected 'msgstr[]', was expecting 'msgid_plural' before 'msgstr[]' while parsing: " + path + ":" + itos(line));
					l = l.substr(9).strip_edges();
				} else if (l.begins_with("msgstr")) {
					ERR_FAIL_COND_V_MSG(status != STATUS_READING_ID, Vector<Vector<String>>(),
							"Unexpected 'msgstr', was expecting 'msgid' before 'msgstr' while parsing: " + path + ":" + itos(line));
					l = l.substr(6).strip_edges();
					status = STATUS_READING_STRING;
				}

				if (l.is_empty() || l.begins_with("#")) {
					line++;
					continue; // Nothing to read or comment.
				}

				ERR_FAIL_COND_V_MSG(!l.begins_with("\"") || status == STATUS_NONE, Vector<Vector<String>>(), "Invalid line '" + l + "' while parsing: " + path + ":" + itos(line));

				l = l.substr(1);
				// Find final quote, ignoring escaped ones (\").
				// The escape_next logic is necessary to properly parse things like \\"
				// where the backslash is the one being escaped, not the quote.
				int end_pos = -1;
				bool escape_next = false;
				for (int i = 0; i < l.length(); i++) {
					if (l[i] == '\\' && !escape_next) {
						escape_next = true;
						continue;
					}

					if (l[i] == '"' && !escape_next) {
						end_pos = i;
						break;
					}

					escape_next = false;
				}

				ERR_FAIL_COND_V_MSG(end_pos == -1, Vector<Vector<String>>(), "Expected '\"' at end of message while parsing: " + path + ":" + itos(line));

				l = l.substr(0, end_pos);
				l = l.c_unescape();

				if (status == STATUS_READING_ID) {
					msg_id += l;
				} else if (status == STATUS_READING_CONTEXT) {
					msg_context += l;
				} else if (status == STATUS_READING_PLURAL) {
					msg_id_plural += l;
				}

				line++;
			}
		}
	}

	return list;
}

// ============================================================================
// Merged from editor/file_system/editor_file_system.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/os.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

int EditorFileSystem::nb_files_total = 0;
EditorFileSystem::ScannedDirectory* EditorFileSystem::first_scan_root_dir = nullptr;

// the name is the version, to keep compatibility with different versions of Godot
#define CACHE_FILE_NAME "filesystem_cache10"

int EditorFileSystemDirectory::find_file_index(const String& p_file) const
{
	for (int i = 0; i < files.size(); i++) {
		if (files[i]->file == p_file) {
			return i;
		}
	}
	return -1;
}

int EditorFileSystemDirectory::find_dir_index(const String& p_dir) const
{
	for (int i = 0; i < subdirs.size(); i++) {
		if (subdirs[i]->name == p_dir) {
			return i;
		}
	}

	return -1;
}

void EditorFileSystemDirectory::force_update()
{
	// We set modified_time to 0 to force `EditorFileSystem::_scan_fs_changes` to search changes in
	// the directory
	modified_time = 0;
}

int EditorFileSystemDirectory::get_subdir_count() const { return subdirs.size(); }

EditorFileSystemDirectory* EditorFileSystemDirectory::get_subdir(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, subdirs.size(), nullptr);
	return subdirs[p_idx];
}

int EditorFileSystemDirectory::get_file_count() const { return files.size(); }

String EditorFileSystemDirectory::get_file(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), "");

	return files[p_idx]->file;
}

String EditorFileSystemDirectory::get_path() const
{
	int parents = 0;
	const EditorFileSystemDirectory* efd = this;
	// Determine the level of nesting.
	while (efd->parent) {
		parents++;
		efd = efd->parent;
	}

	if (parents == 0) {
		return "res://";
	}

	// Using PackedStringArray, because the path is built in reverse order.
	PackedStringArray path_bits;
	// Allocate an array based on nesting. It will store path bits.
	path_bits.resize(parents + 2); // Last String is empty, so paths end with /.
	String* path_write = path_bits.ptrw();
	path_write[0] = "res:/";

	efd = this;
	for (int i = parents; i > 0; i--) {
		path_write[i] = efd->name;
		efd = efd->parent;
	}
	return String("/").join(path_bits);
}

String EditorFileSystemDirectory::get_file_path(int p_idx) const
{
	return get_path().path_join(get_file(p_idx));
}

ResourceUID::ID EditorFileSystemDirectory::get_file_uid(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), ResourceUID::INVALID_ID);
	return files[p_idx]->uid;
}

Vector<String> EditorFileSystemDirectory::get_file_deps(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), Vector<String>());
	Vector<String> deps;

	for (int i = 0; i < files[p_idx]->deps.size(); i++) {
		String dep = files[p_idx]->deps[i];
		int sep_idx = dep.find("::"); // may contain type information, unwanted
		if (sep_idx != -1) {
			dep = dep.substr(0, sep_idx);
		}
		ResourceUID::ID uid = ResourceUID::text_to_id(dep);
		if (uid != ResourceUID::INVALID_ID) {
			// return proper dependency resource from uid
			if (ResourceUID::has_id(uid)) {
				dep = ResourceUID::get_id_path(uid);
			}
			else {
				continue;
			}
		}
		deps.push_back(dep);
	}
	return deps;
}

bool EditorFileSystemDirectory::get_file_import_is_valid(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), false);
	return files[p_idx]->import_valid;
}

uint64_t EditorFileSystemDirectory::get_file_modified_time(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), 0);
	return files[p_idx]->modified_time;
}

uint64_t EditorFileSystemDirectory::get_file_import_modified_time(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), 0);
	return files[p_idx]->import_modified_time;
}

String EditorFileSystemDirectory::get_file_script_class_name(int p_idx) const
{
	return files[p_idx]->class_info.name;
}

String EditorFileSystemDirectory::get_file_script_class_extends(int p_idx) const
{
	return files[p_idx]->class_info.extends;
}

String EditorFileSystemDirectory::get_file_script_class_icon_path(int p_idx) const
{
	return files[p_idx]->class_info.icon_path;
}

String EditorFileSystemDirectory::get_file_icon_path(int p_idx) const
{
	return files[p_idx]->class_info.icon_path;
}

String EditorFileSystemDirectory::get_file_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), "");
	return files[p_idx]->type;
}

String EditorFileSystemDirectory::get_file_resource_script_class(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, files.size(), "");
	return files[p_idx]->resource_script_class;
}

String EditorFileSystemDirectory::get_name() { return name; }

EditorFileSystemDirectory* EditorFileSystemDirectory::get_parent() { return parent; }


EditorFileSystemDirectory::EditorFileSystemDirectory()
{
	modified_time = 0;
	parent = nullptr;
}

EditorFileSystemDirectory::~EditorFileSystemDirectory()
{
	for (EditorFileSystemDirectory::FileInfo* fi : files) {
		memdelete(fi);
	}

	for (EditorFileSystemDirectory* dir : subdirs) {
		memdelete(dir);
	}
}

EditorFileSystem::ScannedDirectory::~ScannedDirectory()
{
	for (ScannedDirectory* dir : subdirs) {
		memdelete(dir);
	}
}

void EditorFileSystem::_load_first_scan_root_dir()
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	first_scan_root_dir = memnew(ScannedDirectory);
	first_scan_root_dir->full_path = "res://";

	nb_files_total = _scan_new_dir(first_scan_root_dir, d);
}

void EditorFileSystem::scan_for_uid()
{
	// Load file structure into memory.
	_load_first_scan_root_dir();

	// Load extensions for which an .import should exists.
	List<String> extensionsl;
	HashSet<String> import_extensions;
	ResourceFormatImporter::get_recognized_extensions(&extensionsl);
	for (const String& E : extensionsl) {
		import_extensions.insert(E);
	}

	// Scan the file system to load uid.
	_scan_for_uid_directory(first_scan_root_dir, import_extensions);

	// It's done, resetting the callback method to prevent a second scan.
	ResourceUID::scan_for_uid_on_startup = nullptr;
}

void EditorFileSystem::_scan_for_uid_directory(
	const ScannedDirectory* p_scan_dir, const HashSet<String>& p_import_extensions)
{
	for (ScannedDirectory* scan_sub_dir : p_scan_dir->subdirs) {
		_scan_for_uid_directory(scan_sub_dir, p_import_extensions);
	}

	for (const String& scan_file : p_scan_dir->files) {
		const String ext = scan_file.get_extension().to_lower();

		if (ext == "uid" || ext == "import") {
			continue;
		}

		const String path = p_scan_dir->full_path.path_join(scan_file);
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		if (p_import_extensions.has(ext)) {
			if (FileAccess::exists(path + ".import")) {
				uid = ResourceFormatImporter::get_resource_uid(path);
			}
		}
		else {
			uid = ResourceLoader::get_resource_uid(path);
		}

		if (uid != ResourceUID::INVALID_ID) {
			if (!ResourceUID::has_id(uid)) {
				ResourceUID::add_id(uid, path);
			}
		}
	}
}

void EditorFileSystem::_save_filesystem_cache()
{
	group_file_cache.clear();

	String fscache =
		EditorPaths::get_project_settings_dir().path_join(CACHE_FILE_NAME);

	Ref<FileAccess> f = FileAccess::open(fscache, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(
		f.is_null(), "Cannot create file '" + fscache + "'. Check user write permissions.");

	f->store_line(filesystem_settings_version_for_import);
	_save_filesystem_cache(filesystem, f);
}

bool EditorFileSystem::_is_test_for_reimport_needed(const String& p_path,
	uint64_t p_last_modification_time, uint64_t p_modification_time,
	uint64_t p_last_import_modification_time, uint64_t p_import_modification_time,
	const Vector<String>& p_import_dest_paths)
{
	// The idea here is to trust the cache. If the last modification times in the cache correspond
	// to the last modification times of the files on disk, it means the files have not changed
	// since the last import, and the files in .godot/imported (p_import_dest_paths) should all be
	// valid.
	if (p_last_modification_time != p_modification_time) {
		return true;
	}
	if (p_last_import_modification_time != p_import_modification_time) {
		return true;
	}
	if (reimport_on_missing_imported_files) {
		for (const String& path : p_import_dest_paths) {
			if (!FileAccess::exists(path)) {
				return true;
			}
		}
	}
	return false;
}

void EditorFileSystem::ScanProgress::increment()
{
	current++;
	float ratio = current / MAX(hi, 1.0f);
	EditorFileSystem::instance->scan_total = ratio;
}

int EditorFileSystem::_scan_new_dir(ScannedDirectory* p_dir, Ref<DirAccess>& da)
{
	List<String> dirs;
	List<String> files;

	String cd = da->get_current_dir();

	da->list_dir_begin();
	while (true) {
		String f = da->get_next();
		if (f.is_empty()) {
			break;
		}

		if (da->current_is_hidden()) {
			continue;
		}

		if (da->current_is_dir()) {
			if (f.begins_with(".")) { // Ignore special and . / ..
				continue;
			}

			if (_should_skip_directory(cd.path_join(f))) {
				continue;
			}

			dirs.push_back(f);

		}
		else {
			files.push_back(f);
		}
	}

	da->list_dir_end();

	dirs.sort_custom<FileNoCaseComparator>();
	files.sort_custom<FileNoCaseComparator>();

	int nb_files_total_scan = 0;

	for (const String& dir : dirs) {
		if (da->change_dir(dir) == OK) {
			String d = da->get_current_dir();

			if (d == cd || !d.begins_with(cd)) {
				da->change_dir(cd); // avoid recursion
			}
			else {
				ScannedDirectory* sd = memnew(ScannedDirectory);
				sd->name = dir;
				sd->full_path = p_dir->full_path.path_join(sd->name);

				nb_files_total_scan += _scan_new_dir(sd, da);

				p_dir->subdirs.push_back(sd);

				da->change_dir("..");
			}
		}
		else {
			ERR_PRINT("Cannot go into subdir '" + dir + "'.");
		}
	}

	p_dir->files = files;
	nb_files_total_scan += files.size();

	return nb_files_total_scan;
}

void EditorFileSystem::_process_removed_files(const HashSet<String>& p_processed_files) {}

void EditorFileSystem::_delete_internal_files(const String& p_file)
{
	if (FileAccess::exists(p_file + ".import")) {
		List<String> paths;
		ResourceFormatImporter::get_internal_resource_path_list(p_file, &paths);
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		for (const String& E : paths) {
			da->remove(E);
		}
		da->remove(p_file + ".import");
	}
	if (FileAccess::exists(p_file + ".uid")) {
		DirAccess::remove_absolute(p_file + ".uid");
	}
}

int EditorFileSystem::_insert_actions_delete_files_directory(EditorFileSystemDirectory* p_dir)
{
	int nb_files = 0;
	for (EditorFileSystemDirectory::FileInfo* fi : p_dir->files) {
		ItemAction ia;
		ia.action = ItemAction::ACTION_FILE_REMOVE;
		ia.dir = p_dir;
		ia.file = fi->file;
		scan_actions.push_back(ia);
		nb_files++;
	}

	for (EditorFileSystemDirectory* sub_dir : p_dir->subdirs) {
		nb_files += _insert_actions_delete_files_directory(sub_dir);
	}

	return nb_files;
}

String EditorFileSystem::_get_file_by_class_name(EditorFileSystemDirectory* p_dir,
	const String& p_class_name, EditorFileSystemDirectory::FileInfo*& r_file_info)
{
	for (EditorFileSystemDirectory::FileInfo* fi : p_dir->files) {
		if (fi->class_info.name == p_class_name) {
			r_file_info = fi;
			return p_dir->get_path().path_join(fi->file);
		}
	}

	for (EditorFileSystemDirectory* sub_dir : p_dir->subdirs) {
		String file = _get_file_by_class_name(sub_dir, p_class_name, r_file_info);
		if (!file.is_empty()) {
			return file;
		}
	}
	r_file_info = nullptr;
	return "";
}

bool EditorFileSystem::is_scanning()
{
	return EditorFileSystem::instance->_impl_is_scanning();
}

bool EditorFileSystem::_impl_is_scanning() const
{ return scanning || scanning_changes || first_scan; }

float EditorFileSystem::get_scanning_progress() const { return scan_total; }

EditorFileSystemDirectory* EditorFileSystem::get_filesystem()
{
	return EditorFileSystem::instance->_impl_get_filesystem();
}

EditorFileSystemDirectory* EditorFileSystem::_impl_get_filesystem()
{ return filesystem; }

void EditorFileSystem::_save_filesystem_cache(
	EditorFileSystemDirectory* p_dir, Ref<FileAccess> p_file)
{
	if (!p_dir) {
		return; // none
	}
	p_file->store_line("::" + p_dir->get_path() + "::" + String::num_int64(p_dir->modified_time));

	for (int i = 0; i < p_dir->files.size(); i++) {
		const EditorFileSystemDirectory::FileInfo* file_info = p_dir->files[i];
		if (!file_info->import_group_file.is_empty()) {
			group_file_cache.insert(file_info->import_group_file);
		}

		String type = file_info->type;
		if (!file_info->resource_script_class.is_empty()) {
			type += "/" + String(file_info->resource_script_class);
		}

		PackedStringArray cache_string;
		cache_string.append(file_info->file);
		cache_string.append(type);
		cache_string.append(itos(file_info->uid));
		cache_string.append(itos(file_info->modified_time));
		cache_string.append(itos(file_info->import_modified_time));
		cache_string.append(itos(file_info->import_valid));
		cache_string.append(file_info->import_group_file);
		cache_string.append(String("<>").join({file_info->class_info.name,
			file_info->class_info.extends, file_info->class_info.icon_path,
			itos(file_info->class_info.is_abstract), itos(file_info->class_info.is_tool),
			file_info->import_md5, String("<*>").join(file_info->import_dest_paths)}));
		cache_string.append(String("<>").join(file_info->deps));

		p_file->store_line(String("::").join(cache_string));
	}

	for (int i = 0; i < p_dir->subdirs.size(); i++) {
		_save_filesystem_cache(p_dir->subdirs[i], p_file);
	}
}

bool EditorFileSystem::_find_file(
	const String& p_file, EditorFileSystemDirectory** r_d, int& r_file_pos) const
{
	// todo make faster

	if (!filesystem || scanning) {
		return false;
	}

	String f = ProjectSettings::localize_path(p_file);
	if (!f.begins_with("res://")) {
		return false;
	}
	f = f.substr(6);
	f = f.replace_char('\\', '/');

	Vector<String> path = f.split("/");

	if (path.is_empty()) {
		return false;
	}
	const String file = path[path.size() - 1];
	const String file_lower = file.to_lower();
	path.resize(path.size() - 1);

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	EditorFileSystemDirectory* fs = filesystem;

	for (const String& path_bit : path) {
		if (path_bit.begins_with(".")) {
			return false;
		}
		const String path_bit_lower = path_bit.to_lower();

		int idx = -1;
		for (int j = 0; j < fs->get_subdir_count(); j++) {
			if (is_case_sensitive) {
				if (fs->get_subdir(j)->get_name() == path_bit) {
					idx = j;
					break;
				}
			}
			else {
				if (fs->get_subdir(j)->get_name().to_lower() == path_bit_lower) {
					idx = j;
					break;
				}
			}
		}

		if (idx == -1) {
			// Only create a missing directory in memory when it exists on disk.
			if (!dir->dir_exists(fs->get_path().path_join(path_bit))) {
				return false;
			}
			EditorFileSystemDirectory* efsd = memnew(EditorFileSystemDirectory);

			efsd->name = path_bit;
			efsd->parent = fs;

			int idx2 = 0;
			for (int j = 0; j < fs->get_subdir_count(); j++) {
				if (efsd->name.filenocasecmp_to(fs->get_subdir(j)->get_name()) < 0) {
					break;
				}
				idx2++;
			}

			if (idx2 == fs->get_subdir_count()) {
				fs->subdirs.push_back(efsd);
			}
			else {
				fs->subdirs.insert(idx2, efsd);
			}
			fs = efsd;
		}
		else {
			fs = fs->get_subdir(idx);
		}
	}

	int cpos = -1;
	for (int i = 0; i < fs->files.size(); i++) {
		if (is_case_sensitive) {
			if (fs->files[i]->file == file) {
				cpos = i;
				break;
			}
		}
		else {
			if (fs->files[i]->file.to_lower() == file_lower) {
				cpos = i;
				break;
			}
		}
	}

	r_file_pos = cpos;
	*r_d = fs;

	return cpos != -1;
}

String EditorFileSystem::get_file_type(const String& p_file)
{
	return EditorFileSystem::instance->_impl_get_file_type(p_file);
}

String EditorFileSystem::_impl_get_file_type(const String& p_file) const
{
	EditorFileSystemDirectory* fs = nullptr;
	int cpos = -1;

	if (!_find_file(p_file, &fs, cpos)) {
		return "";
	}

	return fs->files[cpos]->type;
}

EditorFileSystemDirectory* EditorFileSystem::find_file(const String& p_file, int* r_index)
{
	return EditorFileSystem::instance->_impl_find_file(p_file, r_index);
}

EditorFileSystemDirectory* EditorFileSystem::_impl_find_file(const String& p_file, int* r_index) const
{
	if (!filesystem || scanning) {
		return nullptr;
	}

	EditorFileSystemDirectory* fs = nullptr;
	int cpos = -1;
	if (!_find_file(p_file, &fs, cpos)) {
		return nullptr;
	}

	if (r_index) {
		*r_index = cpos;
	}

	return fs;
}

ResourceUID::ID EditorFileSystem::get_file_uid(const String& p_path)
{
	return EditorFileSystem::instance->_impl_get_file_uid(p_path);
}

ResourceUID::ID EditorFileSystem::_impl_get_file_uid(const String& p_path) const
{
	int file_idx;
	EditorFileSystemDirectory* directory = find_file(p_path, &file_idx);

	if (!directory) {
		return ResourceUID::INVALID_ID;
	}
	return directory->files[file_idx]->uid;
}

EditorFileSystemDirectory* EditorFileSystem::get_filesystem_path(const String& p_path)
{
	return EditorFileSystem::instance->_impl_get_filesystem_path(p_path);
}

EditorFileSystemDirectory* EditorFileSystem::_impl_get_filesystem_path(const String& p_path)
{
	if (!filesystem || scanning) {
		return nullptr;
	}

	String f = ProjectSettings::localize_path(p_path);

	if (!f.begins_with("res://")) {
		return nullptr;
	}

	f = f.substr(6);
	f = f.replace_char('\\', '/');
	if (f.is_empty()) {
		return filesystem;
	}

	if (f.ends_with("/")) {
		f = f.substr(0, f.length() - 1);
	}

	Vector<String> path = f.split("/");

	if (path.is_empty()) {
		return nullptr;
	}

	EditorFileSystemDirectory* fs = filesystem;

	for (int i = 0; i < path.size(); i++) {
		int idx = -1;
		for (int j = 0; j < fs->get_subdir_count(); j++) {
			if (fs->get_subdir(j)->get_name() == path[i]) {
				idx = j;
				break;
			}
		}

		if (idx == -1) {
			return nullptr;
		}
		else {
			fs = fs->get_subdir(idx);
		}
	}

	return fs;
}

void EditorFileSystem::_save_late_updated_files()
{
	// files that already existed, and were modified, need re-scanning for dependencies upon project
	// restart. This is done via saving this special file
	String fscache =
		EditorPaths::get_project_settings_dir().path_join("filesystem_update4");
	Ref<FileAccess> f = FileAccess::open(fscache, FileAccess::WRITE);
	ERR_FAIL_COND_MSG(
		f.is_null(), "Cannot create file '" + fscache + "'. Check user write permissions.");
	for (const String& E : late_update_files) {
		f->store_line(E);
	}
}

Vector<String> EditorFileSystem::_get_dependencies(const String& p_path)
{
	// Avoid error spam on first opening of a not yet imported project by treating the following
	// situation as a benign one, not letting the file open error happen: the resource is of an
	// importable type but it has not been imported yet.
	if (ResourceFormatImporter::recognize_path(p_path)) {
		const String& internal_path =
			ResourceFormatImporter::get_internal_resource_path(p_path);
		if (!internal_path.is_empty() &&
			!FileAccess::exists(
				internal_path)) { // If path is empty (error), keep the code flow to the error.
			return Vector<String>();
		}
	}

	List<String> deps;
	ResourceLoader::get_dependencies(p_path, &deps);

	Vector<String> ret;
	for (const String& E : deps) {
		ret.push_back(E);
	}

	return ret;
}

void EditorFileSystem::_queue_update_script_class(
	const String& p_path, const ScriptClassInfoUpdate& p_script_update)
{
	MutexLock update_script_lock(update_script_mutex);

	update_script_paths.insert(p_path, p_script_update);
	update_script_paths_documentation.insert(p_path);
}

void EditorFileSystem::_queue_update_scene_groups(const String& p_path)
{
	MutexLock update_scene_lock(update_scene_mutex);
	update_scene_paths.insert(p_path);
}

void EditorFileSystem::_get_all_scenes(EditorFileSystemDirectory* p_dir, HashSet<String>& r_list)
{
	for (int i = 0; i < p_dir->get_file_count(); i++) {
		if (p_dir->get_file_type(i) == SNAME("PackedScene")) {
			r_list.insert(p_dir->get_file_path(i));
		}
	}

	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_get_all_scenes(p_dir->get_subdir(i), r_list);
	}
}

HashSet<String> EditorFileSystem::get_valid_extensions() const
{
	return HashSet<String>(valid_extensions);
}

void EditorFileSystem::_find_group_files(EditorFileSystemDirectory* efd,
	HashMap<String, Vector<String>>& group_files, HashSet<String>& groups_to_reimport)
{
	int fc = efd->files.size();
	const EditorFileSystemDirectory::FileInfo* const* files = efd->files.ptr();
	for (int i = 0; i < fc; i++) {
		if (groups_to_reimport.has(files[i]->import_group_file)) {
			if (!group_files.has(files[i]->import_group_file)) {
				group_files[files[i]->import_group_file] = Vector<String>();
			}
			group_files[files[i]->import_group_file].push_back(efd->get_file_path(i));
		}
	}

	for (int i = 0; i < efd->get_subdir_count(); i++) {
		_find_group_files(efd->get_subdir(i), group_files, groups_to_reimport);
	}
}

bool EditorFileSystem::_should_skip_directory(const String& p_path)
{
	String project_data_path = ProjectSettings::get_project_data_path();
	if (p_path == project_data_path || p_path.begins_with(project_data_path + "/")) {
		return true;
	}

	if (FileAccess::exists(p_path.path_join("project.vltr"))) {
		// Skip if another project inside this.
		if (!EditorFileSystem::is_initialized() || EditorFileSystem::doing_first_scan()) {
			WARN_PRINT_ONCE(vformat(
				"Detected another project.vltr at %s. The folder will be ignored.", p_path));
		}
		return true;
	}

	if (FileAccess::exists(p_path.path_join(".gdignore"))) {
		// Skip if a `.gdignore` file is inside this.
		return true;
	}

	return false;
}

bool EditorFileSystem::is_group_file(const String& p_path) const
{
	return group_file_cache.has(p_path);
}

ResourceUID::ID EditorFileSystem::_resource_saver_get_resource_id_for_path(
	const String& p_path, bool p_generate)
{
	if (!p_path.is_resource_file() ||
		p_path.begins_with(ProjectSettings::get_project_data_path())) {
		// Saved externally (configuration file) or internal file, do not assign an ID.
		return ResourceUID::INVALID_ID;
	}

	EditorFileSystemDirectory* fs = nullptr;
	int cpos = -1;

	if (!EditorFileSystem::instance->_find_file(p_path, &fs, cpos)) {
		// Fallback to ResourceLoader if filesystem cache fails (can happen during scanning etc.).
		ResourceUID::ID fallback = ResourceLoader::get_resource_uid(p_path);
		if (fallback != ResourceUID::INVALID_ID) {
			return fallback;
		}

		if (p_generate) {
			return ResourceUID::create_id_for_path(
				p_path); // Just create a new one, we will be notified of save anyway and fetch the
						 // right UID at that time, to keep things simple.
		}
		else {
			return ResourceUID::INVALID_ID;
		}
	}
	else if (fs->files[cpos]->uid != ResourceUID::INVALID_ID) {
		return fs->files[cpos]->uid;
	}
	else if (p_generate) {
		return ResourceUID::create_id_for_path(
			p_path); // Just create a new one, we will be notified of save anyway and fetch the
					 // right UID at that time, to keep things simple.
	}
	else {
		return ResourceUID::INVALID_ID;
	}
}

bool EditorFileSystem::_can_import_file(const String& p_file)
{
	for (const String& F : import_extensions) {
		if (p_file.right(F.length()).nocasecmp_to(F) == 0) {
			return true;
		}
	}

	return false;
}

EditorFileSystem::~EditorFileSystem()
{
	memdelete(filesystem);
	filesystem = nullptr;
	ResourceSaver::set_get_resource_id_for_path(nullptr);
}


bool EditorFileSystem::_scan_extensions() { return true; }

#ifdef CACHE_FILE_NAME
#undef CACHE_FILE_NAME
#endif

// ============================================================================
// Merged from editor/file_system/editor_paths.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/file_system/editor_paths.h"
#include "main/main.h"

bool EditorPaths::are_paths_valid() { return data ? data->paths_valid : false; }

String EditorPaths::get_data_dir() { return data ? data->data_dir : String(); }

String EditorPaths::get_config_dir() { return data ? data->config_dir : String(); }

String EditorPaths::get_cache_dir() { return data ? data->cache_dir : String(); }

String EditorPaths::get_temp_dir() { return data ? data->temp_dir : String(); }

String EditorPaths::get_project_data_dir() { return data ? data->project_data_dir : String(); }

bool EditorPaths::is_self_contained() { return data ? data->self_contained : false; }

String EditorPaths::get_self_contained_file()
{
	return data ? data->self_contained_file : String();
}

String EditorPaths::get_export_templates_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_data_dir().path_join(data->export_templates_folder);
}

String EditorPaths::get_debug_keystore_path()
{
#ifdef ANDROID_ENABLED
	return "assets://keystores/debug.keystore";
#else
	return get_data_dir().path_join("keystores/debug.keystore");
#endif
}

// This returns paths like "res://.godot/editor".
String EditorPaths::get_project_settings_dir()
{
	return get_project_data_dir().path_join("editor");
}

String EditorPaths::get_text_editor_themes_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_config_dir().path_join(data->text_editor_themes_folder);
}

String EditorPaths::get_script_templates_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_config_dir().path_join(data->script_templates_folder);
}

String EditorPaths::get_feature_profiles_dir()
{
	ERR_FAIL_NULL_V(data, String());
	return get_config_dir().path_join(data->feature_profiles_folder);
}

void EditorPaths::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void EditorPaths::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);

	data->project_data_dir = ProjectSettings::get_project_data_path();

	// Self-contained mode if a `._sc_` or `_sc_` file is present in executable dir.
	String exe_path = OS::get_executable_path().get_base_dir();
	Ref<DirAccess> d = DirAccess::create_for_path(exe_path);
	if (d->file_exists(exe_path + "/._sc_")) {
		data->self_contained = true;
		data->self_contained_file = exe_path + "/._sc_";
	}
	else if (d->file_exists(exe_path + "/_sc_")) {
		data->self_contained = true;
		data->self_contained_file = exe_path + "/_sc_";
	}

	// On macOS, look outside .app bundle, since .app bundle is read-only.
	// Note: This will not work if Gatekeeper path randomization is active.
	if (OS::has_feature("macos") && exe_path.ends_with("MacOS") &&
		exe_path.path_join("..").simplify_path().ends_with("Contents")) {
		exe_path = exe_path.path_join("../../..").simplify_path();
		d = DirAccess::create_for_path(exe_path);
		if (d->file_exists(exe_path + "/._sc_")) {
			data->self_contained = true;
			data->self_contained_file = exe_path + "/._sc_";
		}
		else if (d->file_exists(exe_path + "/_sc_")) {
			data->self_contained = true;
			data->self_contained_file = exe_path + "/_sc_";
		}
	}

	String data_path;
	String config_path;
	String cache_path;

	if (data->self_contained) {
		// editor is self contained, all in same folder
		data_path = exe_path;
		data->data_dir = data_path.path_join("editor_data");
		config_path = exe_path;
		data->config_dir = data->data_dir;
		cache_path = exe_path;
		data->cache_dir = data->data_dir.path_join("cache");
		data->temp_dir = data->data_dir.path_join("temp");
	}
	else {
		// Typically XDG_DATA_HOME or %APPDATA%.
		data_path = OS::get_data_path();
		data->data_dir = data_path.path_join(OS::get_godot_dir_name());
		// Can be different from data_path e.g. on Linux or macOS.
		config_path = OS::get_config_path();
		data->config_dir = config_path.path_join(OS::get_godot_dir_name());
		// Can be different from above paths, otherwise a subfolder of data_dir.
		cache_path = OS::get_cache_path();
		if (cache_path == data_path) {
			data->cache_dir = data->data_dir.path_join("cache");
		}
		else {
			data->cache_dir = cache_path.path_join(OS::get_godot_dir_name());
		}
		data->temp_dir = OS::get_temp_path();
	}

	data->paths_valid =
		(!data_path.is_empty() && !config_path.is_empty() && !cache_path.is_empty());
	ERR_FAIL_COND_MSG(!data->paths_valid, "Editor data, config, or cache paths are invalid.");

	// Validate or create each dir and its relevant subdirectories.

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	// Data dir.
	{
		if (dir->change_dir(data->data_dir) != OK) {
			dir->make_dir_recursive(data->data_dir);
			if (dir->change_dir(data->data_dir) != OK) {
				ERR_PRINT("Could not create editor data directory: " + data->data_dir);
				data->paths_valid = false;
			}
		}

		if (!dir->dir_exists(data->export_templates_folder)) {
			dir->make_dir(data->export_templates_folder);
		}
	}

	// Config dir.
	{
		if (dir->change_dir(data->config_dir) != OK) {
			dir->make_dir_recursive(data->config_dir);
			if (dir->change_dir(data->config_dir) != OK) {
				ERR_PRINT("Could not create editor config directory: " + data->config_dir);
				data->paths_valid = false;
			}
		}

		if (!dir->dir_exists(data->text_editor_themes_folder)) {
			dir->make_dir(data->text_editor_themes_folder);
		}
		if (!dir->dir_exists(data->script_templates_folder)) {
			dir->make_dir(data->script_templates_folder);
		}
		if (!dir->dir_exists(data->feature_profiles_folder)) {
			dir->make_dir(data->feature_profiles_folder);
		}
	}

	// Cache dir.
	{
		if (dir->change_dir(data->cache_dir) != OK) {
			dir->make_dir_recursive(data->cache_dir);
			if (dir->change_dir(data->cache_dir) != OK) {
				ERR_PRINT("Could not create editor cache directory: " + data->cache_dir);
				data->paths_valid = false;
			}
		}
	}

	// Temporary dir.
	{
		if (dir->change_dir(data->temp_dir) != OK) {
			dir->make_dir_recursive(data->temp_dir);
			if (dir->change_dir(data->temp_dir) != OK) {
				ERR_PRINT("Could not create editor temporary directory: " + data->temp_dir);
				data->paths_valid = false;
			}
		}
	}

	// Validate or create project-specific editor data dir,
	// including shader cache subdir.
	if (Engine::is_project_manager_hint() ||
		(Main::is_cmdline_tool() && !ProjectSettings::is_project_loaded())) {
		// Nothing to create, use shared editor data dir for shader cache.
		Engine::set_shader_cache_path(data->data_dir);
	}
	else {
		Ref<DirAccess> dir_res = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		if (dir_res->change_dir(data->project_data_dir) != OK) {
			dir_res->make_dir_recursive(data->project_data_dir);
			if (dir_res->change_dir(data->project_data_dir) != OK) {
				ERR_PRINT("Could not create project data directory (" + data->project_data_dir +
						  ") in: " + dir_res->get_current_dir());
				data->paths_valid = false;
			}
		}

		// Check that the project data directory `.gdignore` file exists.
		String project_data_gdignore_file_path = data->project_data_dir.path_join(".gdignore");
		if (!FileAccess::exists(project_data_gdignore_file_path)) {
			// Add an empty .gdignore file to avoid scan.
			Ref<FileAccess> f =
				FileAccess::open(project_data_gdignore_file_path, FileAccess::WRITE);
			if (f.is_valid()) {
				f->store_line("");
			}
			else {
				ERR_PRINT("Failed to create file " + project_data_gdignore_file_path.quote() + ".");
			}
		}

		Engine::set_shader_cache_path(data->project_data_dir);

		// Editor metadata dir.
		if (!dir_res->dir_exists("editor")) {
			dir_res->make_dir("editor");
		}
		// Imported assets dir.
		String imported_files_path = ProjectSettings::get_imported_files_path();
		if (!dir_res->dir_exists(imported_files_path)) {
			dir_res->make_dir(imported_files_path);
		}
	}
}



// ============================================================================
// Merged from editor/file_system/file_info.cpp
// ============================================================================
// --- Inlined from editor/file_system/file_info.h ---
#include "core/string/ustring.h"
#include "core/templates/list.h"

enum class FileSortOption {
	FILE_SORT_NAME = 0,
	FILE_SORT_NAME_REVERSE = 1,
	FILE_SORT_TYPE = 2,
	FILE_SORT_TYPE_REVERSE = 3,
	FILE_SORT_MODIFIED_TIME = 4,
	FILE_SORT_MODIFIED_TIME_REVERSE = 5,
	FILE_SORT_MAX = 6,
};

struct FileInfo {
	String name;
	String path;
	String icon_path;
	String type;
	Vector<String> sources;
	bool import_broken = false;
	uint64_t modified_time = 0;

	bool operator<(const FileInfo &p_fi) const {
		return FileNoCaseComparator()(name, p_fi.name);
	}
};

struct FileInfoTypeComparator {
	bool operator()(const FileInfo &p_a, const FileInfo &p_b) const {
		return FileNoCaseComparator()(p_a.name.get_extension() + p_a.type + p_a.name.get_basename(), p_b.name.get_extension() + p_b.type + p_b.name.get_basename());
	}
};

struct FileInfoModifiedTimeComparator {
	bool operator()(const FileInfo &p_a, const FileInfo &p_b) const {
		return p_a.modified_time > p_b.modified_time;
	}
};

void sort_file_info_list(List<FileInfo> &r_file_list, FileSortOption p_file_sort_option);

// --- End editor/file_system/file_info.h ---

void sort_file_info_list(List<FileInfo> &r_file_list, FileSortOption p_file_sort_option) {
	// Sort the file list if needed.
	switch (p_file_sort_option) {
		case FileSortOption::FILE_SORT_TYPE:
			r_file_list.sort_custom<FileInfoTypeComparator>();
			break;
		case FileSortOption::FILE_SORT_TYPE_REVERSE:
			r_file_list.sort_custom<FileInfoTypeComparator>();
			r_file_list.reverse();
			break;
		case FileSortOption::FILE_SORT_MODIFIED_TIME:
			r_file_list.sort_custom<FileInfoModifiedTimeComparator>();
			break;
		case FileSortOption::FILE_SORT_MODIFIED_TIME_REVERSE:
			r_file_list.sort_custom<FileInfoModifiedTimeComparator>();
			r_file_list.reverse();
			break;
		case FileSortOption::FILE_SORT_NAME_REVERSE:
			r_file_list.sort();
			r_file_list.reverse();
			break;
		case FileSortOption::FILE_SORT_NAME:
			r_file_list.sort();
			break;
		default:
			ERR_FAIL_MSG("Invalid file sort option.");
			break;
	}
}
