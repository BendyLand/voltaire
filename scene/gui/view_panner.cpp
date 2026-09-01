/**************************************************************************/
/*  view_panner.cpp                                                       */
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
#include "core/input/shortcut.h"
#include "core/os/keyboard.h"
#include "scene/main/viewport.h"
#include "view_panner.h"

void ViewPanner::release_pan_key()
{
	pan_key_pressed = false;
	if (drag_type == DragType::DRAG_TYPE_PAN) {
		drag_type = DragType::DRAG_TYPE_NONE;
	}
}

void ViewPanner::set_control_scheme(ControlScheme p_scheme) { control_scheme = p_scheme; }

void ViewPanner::set_enable_rmb(bool p_enable) { enable_rmb = p_enable; }

void ViewPanner::set_pan_shortcut(Ref<Shortcut> p_shortcut)
{
	pan_view_shortcut = p_shortcut;
	pan_key_pressed = false;
}

void ViewPanner::set_simple_panning_enabled(bool p_enabled) { simple_panning_enabled = p_enabled; }

void ViewPanner::set_scroll_speed(int p_scroll_speed)
{
	ERR_FAIL_COND(p_scroll_speed <= 0);
	scroll_speed = p_scroll_speed;
}

void ViewPanner::set_scroll_zoom_factor(float p_scroll_zoom_factor)
{
	ERR_FAIL_COND(p_scroll_zoom_factor <= 1.0);
	scroll_zoom_factor = p_scroll_zoom_factor;
}

void ViewPanner::set_pan_axis(PanAxis p_pan_axis) { pan_axis = p_pan_axis; }

void ViewPanner::set_zoom_style(ZoomStyle p_zoom_style) { zoom_style = p_zoom_style; }

void ViewPanner::setup(ControlScheme p_scheme, Ref<Shortcut> p_shortcut, bool p_simple_panning)
{
	set_control_scheme(p_scheme);
	set_pan_shortcut(p_shortcut);
	set_simple_panning_enabled(p_simple_panning);
}

void ViewPanner::setup_warped_panning(Node* p_owner, bool p_allowed)
{
	warped_panning_owner = p_allowed ? p_owner : nullptr;
}

bool ViewPanner::is_panning() const
{
	return (drag_type == DragType::DRAG_TYPE_PAN) || pan_key_pressed;
}

void ViewPanner::set_force_drag(bool p_force) { force_drag = p_force; }


