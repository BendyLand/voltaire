/**************************************************************************/
/*  visible_on_screen_notifier_3d.cpp                                     */
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
#include "visible_on_screen_notifier_3d.h"

void VisibleOnScreenNotifier3D::set_aabb(const AABB& p_aabb)
{
	if (aabb == p_aabb) {
		return;
	}
	aabb = p_aabb;

	RS::get_singleton()->visibility_notifier_set_aabb(get_base(), aabb);

	update_gizmos();
}

AABB VisibleOnScreenNotifier3D::get_aabb() const { return aabb; }

bool VisibleOnScreenNotifier3D::is_on_screen() const { return on_screen; }

void VisibleOnScreenNotifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
	case NOTIFICATION_EXIT_TREE: {
		on_screen = false;
	} break;
	}
}

VisibleOnScreenNotifier3D::~VisibleOnScreenNotifier3D()
{
	RID base_old = get_base();
	set_base(RID());
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(base_old);
}

void VisibleOnScreenEnabler3D::_screen_enter() { _update_enable_mode(true); }

void VisibleOnScreenEnabler3D::_screen_exit() { _update_enable_mode(false); }

void VisibleOnScreenEnabler3D::set_enable_mode(EnableMode p_mode)
{
	enable_mode = p_mode;
	if (is_inside_tree()) {
		_update_enable_mode(is_on_screen());
	}
}

VisibleOnScreenEnabler3D::EnableMode VisibleOnScreenEnabler3D::get_enable_mode()
{
	return enable_mode;
}

NodePath VisibleOnScreenEnabler3D::get_enable_node_path() { return enable_node_path; }

VisibleOnScreenEnabler3D::VisibleOnScreenEnabler3D() {}


