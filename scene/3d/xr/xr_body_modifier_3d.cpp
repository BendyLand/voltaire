/**************************************************************************/
/*  xr_body_modifier_3d.cpp                                               */
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

#include "scene/3d/skeleton_3d.h"
#include "servers/xr/xr_server.h"
#include "xr_body_modifier_3d.h"

void XRBodyModifier3D::_bind_methods() {}

void XRBodyModifier3D::set_body_tracker(const StringName& p_tracker_name)
{
	tracker_name = p_tracker_name;
}

StringName XRBodyModifier3D::get_body_tracker() const { return tracker_name; }

void XRBodyModifier3D::set_body_update(uint32_t p_body_update)
{
	body_update = p_body_update;

	if (is_inside_tree()) {
		_get_joint_data();
	}
}

uint32_t XRBodyModifier3D::get_body_update() const { return body_update; }

void XRBodyModifier3D::set_bone_update(BoneUpdate p_bone_update)
{
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);
	bone_update = p_bone_update;
}

XRBodyModifier3D::BoneUpdate XRBodyModifier3D::get_bone_update() const { return bone_update; }

void XRBodyModifier3D::_tracker_changed(
	const StringName& p_tracker_name, XRServer::TrackerType p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		_get_joint_data();
	}
}

void XRBodyModifier3D::_skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new)
{
	_get_joint_data();
}


