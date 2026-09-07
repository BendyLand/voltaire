/**************************************************************************/
/*  remote_transform_2d.cpp                                               */
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

#include "remote_transform_2d.h"

void RemoteTransform2D::set_remote_node(const NodePath& p_remote_node)
{
	if (remote_node == p_remote_node) {
		return;
	}

	remote_node = p_remote_node;
	if (is_inside_tree()) {
		_update_cache();
		_update_remote();
	}

	update_configuration_warnings();
}

NodePath RemoteTransform2D::get_remote_node() const { return remote_node; }

void RemoteTransform2D::set_use_global_coordinates(const bool p_enable)
{
	if (use_global_coordinates == p_enable) {
		return;
	}

	use_global_coordinates = p_enable;
	set_notify_transform(use_global_coordinates);
	set_notify_local_transform(!use_global_coordinates);
	_update_remote();
}

bool RemoteTransform2D::get_use_global_coordinates() const { return use_global_coordinates; }

void RemoteTransform2D::set_update_position(const bool p_update)
{
	if (update_remote_position == p_update) {
		return;
	}
	update_remote_position = p_update;
	_update_remote();
}

bool RemoteTransform2D::get_update_position() const { return update_remote_position; }

void RemoteTransform2D::set_update_rotation(const bool p_update)
{
	if (update_remote_rotation == p_update) {
		return;
	}
	update_remote_rotation = p_update;
	_update_remote();
}

bool RemoteTransform2D::get_update_rotation() const { return update_remote_rotation; }

void RemoteTransform2D::set_update_scale(const bool p_update)
{
	if (update_remote_scale == p_update) {
		return;
	}
	update_remote_scale = p_update;
	_update_remote();
}

bool RemoteTransform2D::get_update_scale() const { return update_remote_scale; }

void RemoteTransform2D::force_update_cache() { _update_cache(); }

RemoteTransform2D::RemoteTransform2D()
{
	set_notify_transform(use_global_coordinates);
	set_notify_local_transform(!use_global_coordinates);
	set_hide_clip_children(true);
}


