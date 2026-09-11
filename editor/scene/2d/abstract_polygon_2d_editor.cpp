/**************************************************************************/
/*  abstract_polygon_2d_editor.cpp                                        */
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

#include "abstract_polygon_2d_editor.h"
#include "core/math/geometry_2d.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"

bool AbstractPolygon2DEditor::Vertex::operator==(
	const AbstractPolygon2DEditor::Vertex& p_vertex) const
{
	return polygon == p_vertex.polygon && vertex == p_vertex.vertex;
}

bool AbstractPolygon2DEditor::Vertex::operator!=(
	const AbstractPolygon2DEditor::Vertex& p_vertex) const
{
	return !(*this == p_vertex);
}

bool AbstractPolygon2DEditor::Vertex::valid() const { return vertex >= 0; }

bool AbstractPolygon2DEditor::_is_line() const { return false; }

bool AbstractPolygon2DEditor::_has_uv() const { return false; }

int AbstractPolygon2DEditor::_get_polygon_count() const { return 1; }

Vector2 AbstractPolygon2DEditor::_get_offset(int p_idx) const { return Vector2(0, 0); }

bool AbstractPolygon2DEditor::_has_resource() const { return true; }

void AbstractPolygon2DEditor::_create_resource() {}

void AbstractPolygon2DEditor::_node_removed(Node* p_node)
{
	if (p_node == _get_node()) {
		edit(nullptr);
		hide();

		canvas_item_editor->update_viewport();
	}
}

void AbstractPolygon2DEditor::_wip_cancel()
{
	wip.clear();
	wip_active = false;

	edited_point = PosVertex();
	hover_point = Vertex();
	selected_point = Vertex();
	center_drag = false;

	canvas_item_editor->update_viewport();
}

void AbstractPolygon2DEditor::disable_polygon_editing(bool p_disable, const String& p_reason)
{
	_polygon_editing_enabled = !p_disable;

	button_create->set_disabled(p_disable);
	button_edit->set_disabled(p_disable);
	button_delete->set_disabled(p_disable);
	button_center->set_disabled(p_disable);

	if (p_disable) {
		button_create->set_tooltip_text(p_reason);
		button_edit->set_tooltip_text(p_reason);
		button_delete->set_tooltip_text(p_reason);
		button_center->set_tooltip_text(p_reason);
	}
	else {
		button_create->set_tooltip_text(TTRC("Create points."));
		button_edit->set_tooltip_text(TTRC("Edit points.\nLMB: Move Point\nRMB: Erase Point"));
		button_delete->set_tooltip_text(TTRC("Erase points."));
		button_center->set_tooltip_text(TTRC("Move center of gravity to geometric center."));
	}
}

void AbstractPolygon2DEditor::set_edit_origin_and_center(bool p_enabled)
{
	edit_origin_and_center = p_enabled;
	if (button_center) {
		button_center->set_visible(edit_origin_and_center);
	}
}

void AbstractPolygon2DEditor::edit(Node* p_polygon)
{
	if (!canvas_item_editor) {
		canvas_item_editor = CanvasItemEditor::get_singleton();
	}

	if (p_polygon) {
		_set_node(p_polygon);

		// Enable the pencil tool if the polygon is empty.
		if (_is_empty()) {
			_menu_option(MODE_CREATE);
		}
		else {
			_menu_option(MODE_EDIT);
		}

		wip.clear();
		wip_active = false;
		edited_point = PosVertex();
		hover_point = Vertex();
		selected_point = Vertex();
		center_drag = false;
	}
	else {
		_set_node(nullptr);
	}

	canvas_item_editor->update_viewport();
}

AbstractPolygon2DEditor::Vertex AbstractPolygon2DEditor::get_active_point() const
{
	return hover_point.valid() ? hover_point : selected_point;
}

AbstractPolygon2DEditorPlugin::AbstractPolygon2DEditorPlugin(
	AbstractPolygon2DEditor* p_polygon_editor, const String& p_class)
	: polygon_editor(p_polygon_editor), klass(p_class)
{
	CanvasItemEditor::get_singleton()->add_control_to_menu_panel(polygon_editor);
	polygon_editor->hide();
}


