/**************************************************************************/
/*  joint_2d.cpp                                                          */
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
#include "joint_2d.h"
#include "scene/2d/physics/physics_body_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

void Joint2D::_body_exit_tree()
{
	_disconnect_signals();
	_update_joint(true);
	update_configuration_warnings();
}

NodePath Joint2D::get_node_a() const { return a; }

NodePath Joint2D::get_node_b() const { return b; }

void Joint2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (is_configured()) {
			_disconnect_signals();
		}
		_update_joint();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (is_configured()) {
			_disconnect_signals();
		}
		_update_joint(true);
	} break;
	}
}

void Joint2D::set_bias(real_t p_bias)
{
	bias = p_bias;
	if (joint.is_valid()) {
		PhysicsServer2D::get_singleton()->joint_set_param(joint, PS2DE::JOINT_PARAM_BIAS, bias);
	}
}

real_t Joint2D::get_bias() const { return bias; }

void Joint2D::set_exclude_nodes_from_collision(bool p_enable)
{
	if (exclude_from_collision == p_enable) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	exclude_from_collision = p_enable;
	_update_joint();
}

bool Joint2D::get_exclude_nodes_from_collision() const { return exclude_from_collision; }

PackedStringArray Joint2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}

	return warnings;
}


Joint2D::Joint2D()
{
	joint = PhysicsServer2D::get_singleton()->joint_create();
	set_hide_clip_children(true);
}

Joint2D::~Joint2D()
{
	ERR_FAIL_NULL(PhysicsServer2D::get_singleton());
	PhysicsServer2D::get_singleton()->free_rid(joint);
}


