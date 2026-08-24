/**************************************************************************/
/*  xr_interface.cpp                                                      */
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

#include "core/object/class_db.h"
#include "servers/xr/xr_server.h"
#include "xr_interface.h"

void XRInterface::_bind_methods() {}

bool XRInterface::is_primary()
{
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, false);

	return xr_server->get_primary_interface() == this;
}

void XRInterface::set_primary(bool p_primary)
{
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	if (p_primary) {
		ERR_FAIL_COND(!is_initialized());

		xr_server->set_primary_interface(this);
	}
	else if (xr_server->get_primary_interface() == this) {
		xr_server->set_primary_interface(nullptr);
	}
}

XRInterface::XRInterface() {}

XRInterface::~XRInterface() {}

// query if this interface supports this play area mode
bool XRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_UNKNOWN;
}

// get the current play area mode
XRInterface::PlayAreaMode XRInterface::get_play_area_mode() const { return XR_PLAY_AREA_UNKNOWN; }

// change the play area mode, note that this should return false if the mode is not available
bool XRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_UNKNOWN;
}

// if available, returns an array of vectors denoting the play area the player can move around in
PackedVector3Array XRInterface::get_play_area() const
{
	// Return an empty array by default.
	// Note implementation is responsible for applying our reference frame and world scale to the
	// raw data. `play_area_changed` should be emitted if play area data is available and either the
	// reference frame or world scale changes.
	return PackedVector3Array();
}

/** these will only be implemented on AR interfaces, so we want dummies for VR **/
bool XRInterface::get_anchor_detection_is_enabled() const { return false; }

void XRInterface::set_anchor_detection_is_enabled(bool p_enable) {}

int XRInterface::get_camera_feed_id() { return 0; }

RID XRInterface::get_vrs_texture() { return RID(); }

/** these are optional, so we want dummies **/

RID XRInterface::get_color_texture() { return RID(); }

RID XRInterface::get_depth_texture() { return RID(); }

RID XRInterface::get_velocity_texture() { return RID(); }

RID XRInterface::get_velocity_depth_texture() { return RID(); }

Size2i XRInterface::get_velocity_target_size() { return Size2i(); }

Rect2i XRInterface::get_render_region() { return Rect2i(); }

PackedStringArray XRInterface::get_suggested_tracker_names() const
{
	PackedStringArray arr;

	return arr;
}

PackedStringArray XRInterface::get_suggested_pose_names(const StringName& p_tracker_name) const
{
	PackedStringArray arr;

	return arr;
}

XRInterface::TrackingStatus XRInterface::get_tracking_status() const { return XR_UNKNOWN_TRACKING; }

void XRInterface::trigger_haptic_pulse(const String& p_action_name,
	const StringName& p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec,
	double p_delay_sec)
{
}

Array XRInterface::get_supported_environment_blend_modes()
{
	return Array{XR_ENV_BLEND_MODE_OPAQUE};
}


