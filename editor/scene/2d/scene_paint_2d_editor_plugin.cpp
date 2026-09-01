/**************************************************************************/
/*  scene_paint_2d_editor_plugin.cpp                                      */
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

#include "core/io/resource_loader.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/gui/box_container.h"
#include "scene/gui/item_list.h"
#include "scene/gui/tree.h"
#include "scene_paint_2d_editor_plugin.h"

bool ScenePaint2DEditor::_is_node_valid() { return node && node->is_inside_tree(); }

void ScenePaint2DEditor::_clear_instance(bool p_hide)
{
	if (p_hide && instance_container) {
		instance_container->hide();
	}
	else if (instance) {
		instance->queue_free();
		instance = nullptr;
	}
}

void ScenePaint2DEditor::_update_instance()
{
	if (_is_instance_valid()) {
		_clear_instance();
	}
	if (!_is_instance_valid()) {
		_add_instance();
	}
}

bool ScenePaint2DEditor::_is_instance_valid() { return instance && instance->is_inside_tree(); }

void ScenePaint2DEditor::_update_draw_overlay()
{
	if (custom_overlay) {
		custom_overlay->queue_redraw();
	}
}

void ScenePaint2DEditor::_update_hint_label()
{
	if (!is_tool_selected) {
		CanvasItemEditor::get_singleton()->get_viewport_control()->set_hint_label(
			String(), String());
	}
	else if (!cache_node) {
		CanvasItemEditor::get_singleton()->get_viewport_control()->set_hint_label(
			TTRC("Select a Node2D to enable painting."),
			TTRC("The node will be used as a parent for the painted scenes."));
	}
	else if (!selected_scene) {
		CanvasItemEditor::get_singleton()->get_viewport_control()->set_hint_label(
			TTRC("Pick a scene for painting."),
			vformat(TTR("Use the Scene Picker from the toolbar or %s+Click a 2D scene instance."),
				keycode_get_string(Key::CMD_OR_CTRL)));
	}
	else {
		CanvasItemEditor::get_singleton()->get_viewport_control()->set_hint_label(
			String(), String());
	}
}

Vector2 ScenePaint2DEditor::_get_mouse_grid_cell()
{
	if (!_is_node_valid()) {
		return Vector2();
	}
	CanvasItemEditor* canvas_item_editor = CanvasItemEditor::get_singleton();
	Vector2 pos = canvas_item_editor->get_canvas_transform().affine_inverse().xform(
		viewport->get_local_mouse_position());
	Vector2 local = node->get_global_transform().affine_inverse().xform(pos);
	Vector2 snapped;
	if (paint_mode == PAINT_MODE_SNAP_GRID_CELL_CENTER) {
		snapped = Vector2(
			Math::floor((local.x - grid_offset.x) / grid_step.x) * grid_step.x + grid_offset.x,
			Math::floor((local.y - grid_offset.y) / grid_step.y) * grid_step.y + grid_offset.y);
	}
	else if (paint_mode == PAINT_MODE_SNAP_GRID) {
		snapped = Vector2(
			Math::round((local.x - grid_offset.x) / grid_step.x) * grid_step.x + grid_offset.x,
			Math::round((local.y - grid_offset.y) / grid_step.y) * grid_step.y + grid_offset.y);
	}

	return node->get_global_transform().xform(snapped);
}

void ScenePaint2DEditor::_scene_picker_toggled(bool p_pressed)
{
	input_tool = p_pressed ? INPUT_TOOL_PICK : INPUT_TOOL_NONE;
}

bool ScenePaint2DEditor::_is_selected_scene_valid(Node2D* p_node) const
{
	Node* scene = EditorNode::get_singleton()->get_edited_scene();
	return p_node && p_node->is_instance() && p_node != scene;
}

bool ScenePaint2DEditor::_is_scene_painted(Node2D* p_node) const
{
	return p_node && p_node->has_meta("_scene_painted") && p_node->get_parent() == node;
}

void ScenePaint2DEditor::_set_pinned(bool p_pinned, Node* p_pinned_node)
{
	pinned = p_pinned;
	pin_node_button->set_pressed_no_signal(pinned);
	String tooltip_text = TTR("Pin the current node.\nWhen enabled, the painting parent node will "
							  "not change when selecting other nodes in the scene.");
	if (p_pinned_node && pinned) {
		tooltip_text += vformat("\n" + TTR("Pinned Node: %s"),
			EditorNode::get_singleton()->get_edited_scene()->get_path_to(p_pinned_node));
	}
	pin_node_button->set_tooltip_text(tooltip_text);
}

void ScenePaint2DEditor::_advanced_settings_pressed()
{
	Vector2 pos =
		advanced_settings_button->get_screen_position() + advanced_settings_button->get_size();
	advanced_settings_popup->set_position(
		pos - Vector2(advanced_settings_popup->get_contents_minimum_size().width / 2, 0));
	advanced_settings_popup->reset_size();
	advanced_settings_popup->popup();
	advanced_settings_popup->grab_focus();
}

void ScenePaint2DEditor::_grid_toggled(bool p_toggled)
{
	grid = p_toggled;
	_update_draw_overlay();
}

void ScenePaint2DEditor::_update_paint_mode()
{
	switch (paint_mode) {
	case PAINT_MODE_FREE:
		advanced_settings_button->set_button_icon(get_editor_theme_icon(SNAME("SnapDisable")));
		break;
	case PAINT_MODE_SNAP_GRID:
		advanced_settings_button->set_button_icon(get_editor_theme_icon(SNAME("SnapGrid")));
		break;
	case PAINT_MODE_SNAP_GRID_CELL_CENTER:
		advanced_settings_button->set_button_icon(get_editor_theme_icon(SNAME("Snap")));
		break;
	}
	advanced_settings_popup->set_item_disabled(
		MENU_ITEM_ALLOW_OVERLAPPING, paint_mode == PAINT_MODE_FREE);
}

void ScenePaint2DEditor::_grid_step_changed()
{
	grid_step = CanvasItemEditor::get_singleton()->get_grid_step();
	grid_offset = CanvasItemEditor::get_singleton()->get_grid_offset();
}

void ScenePaint2DEditor::set_painted_scene(Node2D* p_scene)
{
	if (_is_selected_scene_valid(p_scene)) {
		_set_picked_scene(p_scene);
	}
	else {
		_set_picked_scene(nullptr);
	}
}

Node2D* ScenePaint2DEditor::get_painted_scene() const { return instance; }

void ScenePaint2DEditorPlugin::forward_canvas_draw_over_viewport(Control* p_overlay)
{
	scene_paint_2d_editor->forward_canvas_draw_over_viewport(p_overlay);
}


