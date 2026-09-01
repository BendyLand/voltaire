/**************************************************************************/
/*  visible_on_screen_notifier_2d.cpp                                     */
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
#include "servers/rendering/rendering_server.h"
#include "visible_on_screen_notifier_2d.h"

#ifdef TOOLS_ENABLED
void VisibleOnScreenNotifier2D::_edit_set_rect(const Rect2& p_edit_rect) { set_rect(p_edit_rect); }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 VisibleOnScreenNotifier2D::_edit_get_rect() const { return rect; }

bool VisibleOnScreenNotifier2D::_edit_use_rect() const { return show_rect; }
#endif // DEBUG_ENABLED

Rect2 VisibleOnScreenNotifier2D::get_rect() const { return rect; }

void VisibleOnScreenNotifier2D::set_show_rect(bool p_show_rect)
{
	if (show_rect == p_show_rect) {
		return;
	}
	show_rect = p_show_rect;
	queue_redraw();
}

bool VisibleOnScreenNotifier2D::is_showing_rect() const { return show_rect; }

bool VisibleOnScreenNotifier2D::is_on_screen() const { return on_screen; }

VisibleOnScreenNotifier2D::VisibleOnScreenNotifier2D()
{
	rect = Rect2(-10, -10, 20, 20);
	set_hide_clip_children(true);
}

//////////////////////////////////////

void VisibleOnScreenEnabler2D::_screen_enter() { _update_enable_mode(true); }

void VisibleOnScreenEnabler2D::_screen_exit() { _update_enable_mode(false); }

void VisibleOnScreenEnabler2D::set_enable_mode(EnableMode p_mode)
{
	enable_mode = p_mode;
	if (is_inside_tree()) {
		_update_enable_mode(is_on_screen());
	}
}

VisibleOnScreenEnabler2D::EnableMode VisibleOnScreenEnabler2D::get_enable_mode()
{
	return enable_mode;
}

NodePath VisibleOnScreenEnabler2D::get_enable_node_path() { return enable_node_path; }


