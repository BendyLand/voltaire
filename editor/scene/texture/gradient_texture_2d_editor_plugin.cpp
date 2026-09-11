/**************************************************************************/
/*  gradient_texture_2d_editor_plugin.cpp                                 */
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

#include "core/input/input.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_spin_slider.h"
#include "editor/themes/editor_scale.h"
#include "gradient_texture_2d_editor_plugin.h"
#include "scene/gui/button.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/separator.h"
#include "scene/resources/gradient_texture.h"

Point2 GradientTexture2DEdit::_get_handle_pos(const Handle p_handle)
{
	// Get the handle's mouse position in pixels relative to offset.
	return (p_handle == HANDLE_FROM ? texture->get_fill_from() : texture->get_fill_to())
			   .clampf(0, 1) *
		   size;
}

GradientTexture2DEdit::Handle GradientTexture2DEdit::get_handle_at(const Vector2& p_pos)
{
	Point2 from_pos = _get_handle_pos(HANDLE_FROM);
	Point2 to_pos = _get_handle_pos(HANDLE_TO);
	// If both handles are at the position, grab the one that's closer.
	if (p_pos.distance_squared_to(from_pos) < p_pos.distance_squared_to(to_pos)) {
		return Rect2(from_pos.round() - handle_size / 2, handle_size).has_point(p_pos)
				   ? HANDLE_FROM
				   : HANDLE_NONE;
	}
	else {
		return Rect2(to_pos.round() - handle_size / 2, handle_size).has_point(p_pos) ? HANDLE_TO
																					 : HANDLE_NONE;
	}
}

void GradientTexture2DEdit::set_fill_pos(const Vector2& p_pos)
{
	if (p_pos.is_equal_approx(initial_grab_pos)) {
		return;
	}

	const StringName property_name = (grabbed == HANDLE_FROM) ? "fill_from" : "fill_to";
	EditorUndoRedoManager* undo_redo = EditorUndoRedoManager::get_singleton();
	undo_redo->commit_action();
}

void GradientTexture2DEdit::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_MOUSE_EXIT: {
		if (hovered != HANDLE_NONE) {
			hovered = HANDLE_NONE;
			queue_redraw();
		}
	} break;
	case NOTIFICATION_THEME_CHANGED: {
		checkerboard->set_texture(get_editor_theme_icon(SNAME("GuiMiniCheckerboard")));
	} break;
	case NOTIFICATION_DRAW: {
		_draw();
	} break;
	}
}

void GradientTexture2DEdit::_draw()
{
	if (texture.is_null()) {
		return;
	}

	const Ref<Texture2D> fill_from_icon = get_editor_theme_icon(SNAME("EditorPathSmoothHandle"));
	const Ref<Texture2D> fill_to_icon = get_editor_theme_icon(SNAME("EditorPathSharpHandle"));
	handle_size = fill_from_icon->get_size();

	Size2 rect_size = get_size();

	// Get the size and position to draw the texture and handles at.
	// Subtract handle sizes so they stay inside the preview, but keep the texture's aspect ratio.
	Size2 available_size = rect_size - handle_size;
	Size2 ratio = available_size / texture->get_size();
	size = MIN(ratio.x, ratio.y) * texture->get_size();
	offset = ((rect_size - size) / 2).round();

	checkerboard->set_rect(Rect2(offset, size));

	draw_set_transform(offset);
	draw_texture_rect(texture.ptr(), Rect2(Point2(), size));

	// Draw grid snap lines.
	if (snap_enabled ||
		(Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL) && grabbed != HANDLE_NONE)) {
		const Color line_color = Color(0.5, 0.5, 0.5, 0.5);

		for (int idx = 0; idx < snap_count + 1; idx++) {
			float x = float(idx * size.width) / snap_count;
			float y = float(idx * size.height) / snap_count;
			draw_line(Point2(x, 0), Point2(x, size.height), line_color);
			draw_line(Point2(0, y), Point2(size.width, y), line_color);
		}
	}

	// Draw handles.
	const Color focus_modulate = Color(0.4, 1, 1);
	bool modulate_handle_from = grabbed == HANDLE_FROM || hovered == HANDLE_FROM;
	bool modulate_handle_to = grabbed == HANDLE_TO || hovered == HANDLE_TO;
	draw_texture(fill_from_icon.ptr(), (_get_handle_pos(HANDLE_FROM) - handle_size / 2).round(),
		modulate_handle_from ? focus_modulate : Color(1, 1, 1));
	draw_texture(fill_to_icon.ptr(), (_get_handle_pos(HANDLE_TO) - handle_size / 2).round(),
		modulate_handle_to ? focus_modulate : Color(1, 1, 1));
}

GradientTexture2DEdit::GradientTexture2DEdit()
{
	checkerboard = memnew(TextureRect);
	checkerboard->set_stretch_mode(TextureRect::STRETCH_TILE);
	checkerboard->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	checkerboard->set_draw_behind_parent(true);
	add_child(checkerboard, false, INTERNAL_MODE_FRONT);

	set_custom_minimum_size(Size2(0, 250 * EDSCALE));
}

///////////////////////

const int GradientTexture2DEditor::DEFAULT_SNAP = 10;

void GradientTexture2DEditor::_set_snap_enabled(bool p_enabled)
{
	texture_editor_rect->set_snap_enabled(p_enabled);
	snap_count_edit->set_visible(p_enabled);
}

void GradientTexture2DEditor::_set_snap_count(int p_snap_count)
{
	texture_editor_rect->set_snap_count(p_snap_count);
}

void GradientTexture2DEditor::set_texture(Ref<GradientTexture2D>& p_texture)
{
	texture = p_texture;
	texture_editor_rect->set_texture(p_texture);
}


