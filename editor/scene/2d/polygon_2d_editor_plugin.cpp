/**************************************************************************/
/*  polygon_2d_editor_plugin.cpp                                          */
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

#include "core/input/input_event.h"
#include "core/math/geometry_2d.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "polygon_2d_editor_plugin.h"
#include "scene/2d/skeleton_2d.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/view_panner.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"

Node2D* Polygon2DEditor::_get_node() const { return node; }

Vector2 Polygon2DEditor::_get_offset(int p_idx) const { return node->get_offset(); }

int Polygon2DEditor::_get_polygon_count() const
{
	if (node->get_internal_vertex_count() > 0) {
		return 0; // do not edit if internal vertices exist
	}
	else {
		return 1;
	}
}

void Polygon2DEditor::_center_view()
{
	Size2 texture_size;
	if (node->get_texture().is_valid()) {
		texture_size = node->get_texture()->get_size();
		Vector2 zoom_factor = (canvas->get_size() - Vector2(1, 1) * 50 * EDSCALE) / texture_size;
		zoom_widget->set_zoom(MIN(zoom_factor.x, zoom_factor.y));
	}
	else {
		zoom_widget->set_zoom(EDSCALE);
	}
	// Recalculate scroll limits.
	_update_zoom_and_pan(false);

	Size2 offset = (texture_size - canvas->get_size() / draw_zoom) / 2;
	hscroll->set_value_no_signal(offset.x);
	vscroll->set_value_no_signal(offset.y);
	_update_zoom_and_pan(false);
}

void Polygon2DEditor::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	hscroll->set_value_no_signal(hscroll->get_value() - p_scroll_vec.x / draw_zoom);
	vscroll->set_value_no_signal(vscroll->get_value() - p_scroll_vec.y / draw_zoom);
	_update_zoom_and_pan(false);
}

void Polygon2DEditor::_zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	zoom_widget->set_zoom(draw_zoom * p_zoom_factor);
	draw_offset += p_origin / draw_zoom - p_origin / zoom_widget->get_zoom();
	hscroll->set_value_no_signal(draw_offset.x);
	vscroll->set_value_no_signal(draw_offset.y);
	_update_zoom_and_pan(false);
}

Vector2 Polygon2DEditor::snap_point(Vector2 p_target) const
{
	if (use_snap) {
		p_target.x = Math::snap_scalar(
			(snap_offset.x - draw_offset.x) * draw_zoom, snap_step.x * draw_zoom, p_target.x);
		p_target.y = Math::snap_scalar(
			(snap_offset.y - draw_offset.y) * draw_zoom, snap_step.y * draw_zoom, p_target.y);
	}

	return p_target;
}

Polygon2DEditorPlugin::Polygon2DEditorPlugin()
	: AbstractPolygon2DEditorPlugin(memnew(Polygon2DEditor), "Polygon2D")
{
}



void Polygon2DEditor::_update_zoom_and_pan(bool) {}

Polygon2DEditor::Polygon2DEditor() {}
