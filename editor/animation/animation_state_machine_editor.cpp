/**************************************************************************/
/*  animation_state_machine_editor.cpp                                    */
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

#include "animation_state_machine_editor.h"
#include "core/io/resource_loader.h"
#include "core/math/geometry_2d.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/style_box_flat.h"
#include "scene/theme/theme_db.h"

bool AnimationNodeStateMachineEditor::can_edit(const Ref<AnimationNode>& p_node)
{
	Ref<AnimationNodeStateMachine> ansm = p_node;
	return ansm.is_valid();
}

void AnimationNodeStateMachineEditor::edit(const Ref<AnimationNode>& p_node)
{
	state_machine = p_node;

	read_only = false;

	if (state_machine.is_valid()) {
		read_only = EditorNode::get_singleton()->is_resource_read_only(state_machine);

		selected_transition_from = StringName();
		selected_transition_to = StringName();
		selected_transition_index = -1;
		selected_node = StringName();
		selected_nodes.clear();
		connected_nodes.clear();
		_update_mode();
		_update_graph();
	}

	if (read_only) {
		tool_create->set_pressed(false);
		tool_connect->set_pressed(false);
	}

	tool_create->set_disabled(read_only);
	tool_connect->set_disabled(read_only);
}

String AnimationNodeStateMachineEditor::_get_root_playback_path(String& r_node_directory)
{
	AnimationTree* tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
	Vector<String> edited_path = AnimationTreeEditor::get_singleton()->get_edited_path();

	String base_path;
	Vector<String> node_directory_path;

	bool is_playable_anodesm_found = false;

	if (edited_path.size()) {
		while (!is_playable_anodesm_found) {
			base_path = String("/").join(edited_path);
			Ref<AnimationNodeStateMachine> anodesm =
				!edited_path.size() ? Ref<AnimationNode>(tree->get_root_animation_node().ptr())
									: tree->get_root_animation_node()->find_node_by_path(base_path);
			if (anodesm.is_null()) {
				break;
			}
			else {
				if (anodesm->get_state_machine_type() !=
					AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
					is_playable_anodesm_found = true;
				}
				else {
					int idx = edited_path.size() - 1;
					node_directory_path.push_back(edited_path[idx]);
					edited_path.remove_at(idx);
				}
			}
		}
	}

	if (is_playable_anodesm_found) {
		// Return Root/Nested state machine playback.
		node_directory_path.reverse();
		r_node_directory = String("/").join(node_directory_path);
		if (node_directory_path.size()) {
			r_node_directory += "/";
		}
		base_path = !edited_path.size() ? Animation::PARAMETERS_BASE_PATH + "playback"
										: Animation::PARAMETERS_BASE_PATH + base_path + "/playback";
	}
	else {
		// Hmmm, we have to return Grouped state machine playback...
		// It will give the user the error that Root/Nested state machine should be retrieved, that
		// would be kind :-)
		r_node_directory = String();
		base_path = AnimationTreeEditor::get_singleton()->get_base_path() + "playback";
	}

	return base_path;
}

Control::CursorShape AnimationNodeStateMachineEditor::get_cursor_shape(const Point2& p_pos) const
{
	Control::CursorShape cursor_shape = get_default_cursor_shape();
	if (!read_only) {
		// Put ibeam (text cursor) over names to make it clearer that they are editable.
		Transform2D xform = panel->get_transform() * state_machine_draw->get_transform();
		Point2 pos = xform.xform_inv(p_pos);

		for (int i = node_rects.size() - 1; i >= 0; i--) { // Inverse to draw order.
			if (node_rects[i].node.has_point(pos)) {
				if (node_rects[i].name.has_point(pos)) {
					if (state_machine->can_edit_node(node_rects[i].node_name)) {
						cursor_shape = Control::CURSOR_IBEAM;
					}
				}
				break;
			}
		}
	}
	return cursor_shape;
}

void AnimationNodeStateMachineEditor::_open_menu(const Vector2& p_position)
{
	AnimationTree* tree = AnimationTreeEditor::get_singleton()->get_animation_tree();
	if (!tree) {
		return;
	}

	menu->clear(false);
	animations_menu->clear();
	animations_to_add.clear();

	LocalVector<StringName> animation_names = tree->get_sorted_animation_list();
	menu->add_submenu_node_item(TTR("Add Animation"), animations_menu);
	if (animation_names.is_empty()) {
		menu->set_item_disabled(menu->get_item_idx_from_text(TTR("Add Animation")), true);
	}
	else {
		for (const StringName& name : animation_names) {
			animations_menu->add_icon_item(theme_cache.animation_icon, name);
			animations_to_add.push_back(name);
		}
	}

	LocalVector<StringName> classes;
	classes.sort_custom<StringName::AlphCompare>();

	for (const StringName& class_name : classes) {
		String name = String(class_name).replace_first("AnimationNode", "");
		if (name == "Animation" || name == "StartState" || name == "EndState") {
			continue; // nope
		}
		int idx = menu->get_item_count();
		menu->add_item(vformat(TTR("Add %s"), name), idx);
	}
	Ref<AnimationNode> clipb = EditorSettings::get_singleton()->get_resource_clipboard();

	if (clipb.is_valid()) {
		menu->add_separator();
		menu->add_item(TTR("Paste"), MENU_PASTE);
	}
	menu->add_separator();
	menu->add_item(TTR("Load..."), MENU_LOAD_FILE);

	menu->set_position(state_machine_draw->get_screen_transform().xform(p_position));
	menu->popup();
	add_node_pos = p_position / EDSCALE + state_machine->get_graph_offset();
}

bool AnimationNodeStateMachineEditor::_create_submenu(PopupMenu* p_menu,
	Ref<AnimationNodeStateMachine> p_nodesm, const StringName& p_name, const StringName& p_path)
{
	LocalVector<StringName> nodes = p_nodesm->get_node_list();

	PopupMenu* nodes_menu = memnew(PopupMenu);
	nodes_menu->set_name(p_name);
	p_menu->add_child(nodes_menu);

	bool node_added = false;
	for (const StringName& E : nodes) {
		if (p_nodesm->can_edit_node(E)) {
			Ref<AnimationNodeStateMachine> ansm = p_nodesm->get_node(E);

			String path = String(p_path) + "/" + E;

			if (ansm == state_machine) {
				end_menu->add_item(E, nodes_to_connect.size());
				nodes_to_connect.push_back(SceneStringName(End));
				continue;
			}

			if (ansm.is_valid()) {
				state_machine_menu->add_item(E, nodes_to_connect.size());
				nodes_to_connect.push_back(path);

				if (_create_submenu(nodes_menu, ansm, E, path)) {
					nodes_menu->add_submenu_item(E, E);
					node_added = true;
				}
			}
			else {
				nodes_menu->add_item(E, nodes_to_connect.size());
				nodes_to_connect.push_back(path);
				node_added = true;
			}
		}
	}

	return node_added;
}

void AnimationNodeStateMachineEditor::_stop_connecting()
{
	connecting = false;
	state_machine_draw->queue_redraw();
}

void AnimationNodeStateMachineEditor::_connection_draw(const Vector2& p_from, const Vector2& p_to,
	AnimationNodeStateMachineTransition::SwitchMode p_mode, bool p_enabled, bool p_selected,
	bool p_travel, float p_fade_ratio, bool p_auto_advance, bool p_is_across_group, float p_opacity,
	bool p_endpoint_hovered, bool p_endpoint_hovered_start)
{
	Color line_color =
		p_enabled ? theme_cache.transition_color : theme_cache.transition_disabled_color;
	Color icon_color =
		p_enabled ? theme_cache.transition_icon_color : theme_cache.transition_icon_disabled_color;
	Color highlight_color =
		p_enabled ? theme_cache.highlight_color : theme_cache.highlight_disabled_color;

	line_color.a *= p_opacity;
	icon_color.a *= p_opacity;
	highlight_color.a *= p_opacity;

	if (p_travel) {
		line_color = highlight_color;
	}

	// Add gradient on hovered endpoint.
	if (p_endpoint_hovered) {
		// Calculate gradient length based on transition length.
		float transition_length = p_from.distance_to(p_to);
		float gradient_distance = MIN(20.0f, transition_length * 0.2f);

		Vector2 gradient_start;
		Vector2 gradient_end;
		if (p_endpoint_hovered_start) {
			gradient_start = p_from;
			gradient_end = p_from + (p_to - p_from).normalized() * gradient_distance;
		}
		else {
			gradient_end = p_to;
			gradient_start = p_to - (p_to - p_from).normalized() * gradient_distance;
		}

		PackedVector2Array points;
		PackedColorArray colors;

		points.push_back(gradient_start);
		points.push_back(gradient_end);

		Color start_color = highlight_color;
		Color end_color = highlight_color;

		if (p_endpoint_hovered_start) {
			end_color.a = 0.0f;
		}
		else {
			start_color.a = 0.0f;
		}

		if (p_selected) {
			start_color = start_color.lightened(0.2f);
			end_color = end_color.lightened(0.2f);
			start_color.a *= 1.5f;
			end_color.a *= 1.5f;
		}

		colors.push_back(start_color);
		colors.push_back(end_color);

		float line_width = p_selected ? 10.0f : 8.0f;
		state_machine_draw->draw_polyline_colors(points, colors, line_width, true);
	}

	if (p_selected) {
		state_machine_draw->draw_line(p_from, p_to, highlight_color, 6, true);
	}
	state_machine_draw->draw_line(p_from, p_to, line_color, 2, true);

	if (p_fade_ratio > 0.0) {
		Color fade_line_color = highlight_color;
		fade_line_color.set_hsv(1.0, fade_line_color.get_s(), fade_line_color.get_v());
		fade_line_color.a *= p_opacity;
		state_machine_draw->draw_line(p_from, p_from.lerp(p_to, p_fade_ratio), fade_line_color, 2);
	}

	const int ICON_COUNT = std_size(theme_cache.transition_icons);
	int icon_index = p_mode + (p_auto_advance ? ICON_COUNT / 2 : 0);
	ERR_FAIL_COND(icon_index >= ICON_COUNT);
	Ref<Texture2D> icon = theme_cache.transition_icons[icon_index];

	Transform2D xf;
	xf.columns[0] = (p_to - p_from).normalized();
	xf.columns[1] = xf.columns[0].orthogonal();
	xf.columns[2] = (p_from + p_to) * 0.5 - xf.columns[1] * icon->get_height() * 0.5 -
					xf.columns[0] * icon->get_height() * 0.5;

	state_machine_draw->draw_set_transform_matrix(xf);
	if (!p_is_across_group) {
		state_machine_draw->draw_texture(icon.ptr(), Vector2(), icon_color);
	}
	state_machine_draw->draw_set_transform_matrix(Transform2D());
}

void AnimationNodeStateMachineEditor::_clip_src_line_to_rect(
	Vector2& r_from, const Vector2& p_to, const Rect2& p_rect)
{
	if (p_to == r_from) {
		return;
	}

	// this could be optimized...
	Vector2 n = (p_to - r_from).normalized();
	while (p_rect.has_point(r_from)) {
		r_from += n;
	}
}

void AnimationNodeStateMachineEditor::_clip_dst_line_to_rect(
	const Vector2& p_from, Vector2& r_to, const Rect2& p_rect)
{
	if (r_to == p_from) {
		return;
	}

	// this could be optimized...
	Vector2 n = (r_to - p_from).normalized();
	while (p_rect.has_point(r_to)) {
		r_to -= n;
	}
}

void AnimationNodeStateMachineEditor::_update_connected_nodes(const StringName& p_node)
{
	connected_nodes.clear();
	if (p_node != StringName()) {
		connected_nodes.insert(p_node);

		Vector<StringName> nodes_to = state_machine->get_nodes_with_transitions_to(p_node);
		for (const StringName& node_to : nodes_to) {
			connected_nodes.insert(node_to);
		}

		Vector<StringName> nodes_from = state_machine->get_nodes_with_transitions_from(p_node);
		for (const StringName& node_from : nodes_from) {
			connected_nodes.insert(node_from);
		}
	}
}

void AnimationNodeStateMachineEditor::_update_graph()
{
	if (updating) {
		return;
	}

	updating = true;

	state_machine_draw->queue_redraw();

	updating = false;
}

void AnimationNodeStateMachineEditor::_open_editor(const String& p_name)
{
	AnimationTreeEditor::get_singleton()->enter_editor(p_name);
}

void AnimationNodeStateMachineEditor::_scroll_changed(double)
{
	if (updating) {
		return;
	}

	state_machine->set_graph_offset(Vector2(h_scroll->get_value(), v_scroll->get_value()));
	state_machine_draw->queue_redraw();
}

void AnimationNodeStateMachineEditor::_update_mode()
{
	if (tool_select->is_pressed()) {
		selection_tools_hb->show();
		bool nothing_selected = selected_nodes.is_empty() &&
								selected_transition_from == StringName() &&
								selected_transition_to == StringName();
		bool start_end_selected =
			selected_nodes.size() == 1 && (*selected_nodes.begin() == SceneStringName(Start) ||
											  *selected_nodes.begin() == SceneStringName(End));
		tool_erase->set_disabled(nothing_selected || start_end_selected || read_only);
	}
	else {
		selection_tools_hb->hide();
	}

	if (read_only) {
		tool_create->set_pressed(false);
		tool_connect->set_pressed(false);
	}

	if (tool_connect->is_pressed()) {
		transition_tools_hb->show();
	}
	else {
		transition_tools_hb->hide();
	}
}

void AnimationNodeStateMachineEditor::_bind_methods() {}

AnimationNodeStateMachineEditor* AnimationNodeStateMachineEditor::singleton = nullptr;

void EditorAnimationMultiTransitionEdit::add_transition(const StringName& p_from,
	const StringName& p_to, Ref<AnimationNodeStateMachineTransition> p_transition)
{
	Transition tr;
	tr.from = p_from;
	tr.to = p_to;
	tr.transition = p_transition;
	transitions.push_back(tr);
}


