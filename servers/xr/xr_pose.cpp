/**************************************************************************/
/*  xr_pose.cpp                                                           */
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

#include "servers/xr/xr_server.h"
#include "xr_pose.h"

void XRPose::_bind_methods() {}

void XRPose::set_has_tracking_data(const bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRPose::get_has_tracking_data() const { return has_tracking_data; }

void XRPose::set_name(const StringName& p_name) { name = p_name; }

StringName XRPose::get_name() const { return name; }

void XRPose::set_transform(const Transform3D p_transform) { transform = p_transform; }

Transform3D XRPose::get_transform() const { return transform; }

Transform3D XRPose::get_adjusted_transform() const
{
	Transform3D adjusted_transform = transform;

	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, transform);

	// apply world scale
	adjusted_transform.origin *= xr_server->get_world_scale();

	// apply reference frame
	adjusted_transform = xr_server->get_reference_frame() * adjusted_transform;

	return adjusted_transform;
}

void XRPose::set_linear_velocity(const Vector3 p_velocity) { linear_velocity = p_velocity; }

Vector3 XRPose::get_linear_velocity() const { return linear_velocity; }

void XRPose::set_angular_velocity(const Vector3 p_velocity) { angular_velocity = p_velocity; }

Vector3 XRPose::get_angular_velocity() const { return angular_velocity; }

void XRPose::set_tracking_confidence(const XRPose::TrackingConfidence p_tracking_confidence)
{
	tracking_confidence = p_tracking_confidence;
}

XRPose::TrackingConfidence XRPose::get_tracking_confidence() const { return tracking_confidence; }


