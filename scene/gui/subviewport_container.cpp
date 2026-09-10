/**************************************************************************/
/*  subviewport_container.cpp                                             */
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
#include "scene/main/viewport.h"
#include "subviewport_container.h"

bool SubViewportContainer::is_stretch_enabled() const { return stretch; }

int SubViewportContainer::get_stretch_shrink() const { return shrink; }

Vector<int> SubViewportContainer::get_allowed_size_flags_horizontal() const
{
	return Vector<int>();
}

Vector<int> SubViewportContainer::get_allowed_size_flags_vertical() const { return Vector<int>(); }

void SubViewportContainer::input(const Ref<InputEvent>& p_event)
{
	_propagate_nonpositional_event(p_event);
}

void SubViewportContainer::unhandled_input(const Ref<InputEvent>& p_event)
{
	_propagate_nonpositional_event(p_event);
}

void SubViewportContainer::_propagate_nonpositional_event(const Ref<InputEvent>& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	if (_is_propagated_in_gui_input(p_event)) {
		return;
	}

	_send_event_to_viewports(p_event);
}

void SubViewportContainer::gui_input(const Ref<InputEvent>& p_event)
{
	ERR_FAIL_COND(p_event.is_null());

	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	if (!_is_propagated_in_gui_input(p_event)) {
		return;
	}

	if (stretch && shrink > 1) {
		Transform2D xform;
		xform.scale(Vector2(1, 1) / shrink);
		_send_event_to_viewports(p_event->xformed_by(xform));
	}
	else {
		_send_event_to_viewports(p_event);
	}
}

void SubViewportContainer::set_mouse_target(bool p_enable) { mouse_target = p_enable; }

bool SubViewportContainer::is_mouse_target_enabled() { return mouse_target; }

SubViewportContainer::SubViewportContainer()
{
	set_process_unhandled_input(true);
	set_focus_mode(FOCUS_CLICK);
}


