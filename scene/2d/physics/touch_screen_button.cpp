/**************************************************************************/
/*  touch_screen_button.cpp                                               */
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

#include "core/config/engine.h"
#include "core/input/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "touch_screen_button.h"

Ref<Texture2D> TouchScreenButton::get_texture_normal() const { return texture_normal; }

Ref<Texture2D> TouchScreenButton::get_texture_pressed() const { return texture_pressed; }

void TouchScreenButton::set_bitmask(const Ref<BitMap>& p_bitmask) { bitmask = p_bitmask; }

Ref<BitMap> TouchScreenButton::get_bitmask() const { return bitmask; }

Ref<Shape2D> TouchScreenButton::get_shape() const { return shape; }

bool TouchScreenButton::is_shape_visible() const { return shape_visible; }

bool TouchScreenButton::is_shape_centered() const { return shape_centered; }

bool TouchScreenButton::is_pressed() const { return finger_pressed != -1; }

void TouchScreenButton::set_action(const String& p_action) { action = p_action; }

String TouchScreenButton::get_action() const { return action; }

bool TouchScreenButton::_is_point_inside(const Point2& p_point)
{
	Point2 coord = (get_global_transform_with_canvas()).affine_inverse().xform(p_point);

	bool touched = false;
	bool check_rect = true;

	if (shape.is_valid()) {
		check_rect = false;

		Vector2 pos;
		if (shape_centered && texture_normal.is_valid()) {
			pos = texture_normal->get_size() * 0.5;
		}

		touched = shape->collide(Transform2D().translated_local(pos), unit_rect.ptr(),
			Transform2D(0, coord + Vector2(0.5, 0.5)));
	}

	if (bitmask.is_valid()) {
		check_rect = false;
		if (!touched && Rect2(Point2(), bitmask->get_size()).has_point(coord)) {
			if (bitmask->get_bitv(coord)) {
				touched = true;
			}
		}
	}

	if (!touched && check_rect) {
		if (texture_normal.is_valid()) {
			touched = Rect2(Size2(), texture_normal->get_size()).has_point(coord);
		}
	}

	return touched;
}

#ifdef DEBUG_ENABLED
Rect2 TouchScreenButton::_edit_get_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::_edit_get_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

bool TouchScreenButton::_edit_use_rect() const { return texture_normal.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 TouchScreenButton::get_anchorable_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::get_anchorable_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

TouchScreenButton::VisibilityMode TouchScreenButton::get_visibility_mode() const
{
	return visibility;
}

void TouchScreenButton::set_passby_press(bool p_enable) { passby_press = p_enable; }

bool TouchScreenButton::is_passby_press_enabled() const { return passby_press; }

TouchScreenButton::TouchScreenButton()
{
	unit_rect.instantiate();
	unit_rect->set_size(Vector2(1, 1));
}


