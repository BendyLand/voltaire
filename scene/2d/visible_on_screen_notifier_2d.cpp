/**************************************************************************/
/*  visible_on_screen_notifier_2d.cpp                                     */
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

#include "visible_on_screen_notifier_2d.h"

#include "core/config/engine.h"
#include "servers/rendering/renderer.h"

#ifdef TOOLS_ENABLED
void VisibleOnScreenNotifier2D::_edit_set_rect(const Rect2& p_edit_rect) { set_rect(p_edit_rect); }
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
Rect2 VisibleOnScreenNotifier2D::_edit_get_rect() const { return rect; }

bool VisibleOnScreenNotifier2D::_edit_use_rect() const { return show_rect; }
#endif // DEBUG_ENABLED

void VisibleOnScreenNotifier2D::set_rect(const Rect2& p_rect)
{
	rect = p_rect;
	queue_redraw();
}

Rect2 VisibleOnScreenNotifier2D::get_rect() const { return rect; }

void VisibleOnScreenNotifier2D::set_show_rect(bool p_show_rect)
{
	show_rect = p_show_rect;
	queue_redraw();
}

bool VisibleOnScreenNotifier2D::is_showing_rect() const { return show_rect; }

bool VisibleOnScreenNotifier2D::is_on_screen() const { return on_screen; }

void VisibleOnScreenNotifier2D::set_enabler_enabled(bool p_enabled)
{
	enabler_enabled = p_enabled;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier2D::set_enable_mode(EnableMode p_mode)
{
	enable_mode = p_mode;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier2D::set_enable_node_path(const NodePath& p_path)
{
	enable_node_path = p_path;
}

void VisibleOnScreenNotifier2D::_visibility_enter()
{
	on_screen = true;
	if (enabler_enabled) {
		_update_enable_mode(true);
	}
}

void VisibleOnScreenNotifier2D::_visibility_exit()
{
	on_screen = false;
	if (enabler_enabled) {
		_update_enable_mode(false);
	}
}

void VisibleOnScreenNotifier2D::_update_enable_mode(bool p_enable)
{
	if (Engine::is_editor_hint()) {
		return;
	}
	Node* node = get_node_or_null(enable_node_path);
	if (!node) {
		return;
	}
	if (p_enable) {
		switch (enable_mode) {
		case ENABLE_MODE_INHERIT:
			node->set_process_mode(PROCESS_MODE_INHERIT);
			break;
		case ENABLE_MODE_ALWAYS:
			node->set_process_mode(PROCESS_MODE_ALWAYS);
			break;
		case ENABLE_MODE_WHEN_PAUSED:
			node->set_process_mode(PROCESS_MODE_WHEN_PAUSED);
			break;
		}
	}
	else {
		node->set_process_mode(PROCESS_MODE_DISABLED);
	}
}

void VisibleOnScreenNotifier2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
	case NOTIFICATION_EXIT_TREE: {
		on_screen = false;
	} break;
	}
}

VisibleOnScreenNotifier2D::VisibleOnScreenNotifier2D(bool p_enabler) : enabler_enabled(p_enabler)
{
	rect = Rect2(-10, -10, 20, 20);
	set_hide_clip_children(true);
}
