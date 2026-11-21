/**************************************************************************/
/*  xr_nodes.cpp                                                          */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/types.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/xr/xr_interface.h"
#include "xr_nodes.h"

// XRNode3D is a node that has its transform updated by an XRTracker.
// Note that trackers are only available at runtime and only after an XRInterface registers one.
// So we bind by name and as long as a tracker isn't available, our node remains inactive.

void XRNode3D::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	update_configuration_warnings();
}

void XRNode3D::set_tracker(const String& p_tracker_name)
{
	if (tracker.is_valid() && tracker->get_tracker_name() == p_tracker_name) {
		return;
	}

	_unbind_tracker();

	tracker_name = p_tracker_name;
	pose_name = SceneString(default_);

	_bind_tracker();

	update_configuration_warnings();
}

String XRNode3D::get_tracker() const { return tracker_name; }

void XRNode3D::set_pose_name(const String& p_pose_name)
{
	pose_name = p_pose_name;

	// Update pose if we are bound to a tracker with a valid pose
	Ref<XRPose> pose = get_pose();
	if (pose.is_valid()) {
		set_transform(pose->get_adjusted_transform());
	}
}

String XRNode3D::get_pose_name() const { return pose_name; }

void XRNode3D::set_show_when_tracked(bool p_show)
{
	show_when_tracked = p_show;

	_update_visibility();
}

bool XRNode3D::get_show_when_tracked() const { return show_when_tracked; }

bool XRNode3D::get_is_active() const
{
	if (tracker.is_null()) {
		return false;
	}
	else if (!tracker->has_pose(pose_name)) {
		return false;
	}
	else {
		return true;
	}
}

bool XRNode3D::get_has_tracking_data() const { return has_tracking_data; }

void XRNode3D::trigger_haptic_pulse(const String& p_action_name, double p_frequency,
	double p_amplitude, double p_duration_sec, double p_delay_sec)
{
	// TODO need to link trackers to the interface that registered them so we can call this on the
	// correct interface. For now this works fine as in 99% of the cases we only have our primary
	// interface active
	if (XRServer::is_initialized()) {
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			xr_interface->trigger_haptic_pulse(
				p_action_name, tracker_name, p_frequency, p_amplitude, p_duration_sec, p_delay_sec);
		}
	}
}

Ref<XRPose> XRNode3D::get_pose()
{
	if (tracker.is_valid()) {
		return tracker->get_pose(pose_name);
	}
	else {
		return Ref<XRPose>();
	}
}

void XRNode3D::_bind_tracker()
{
	ERR_FAIL_COND(tracker.is_valid());

	if (XRServer::is_initialized()) {
		tracker = XRServer::get_tracker(tracker_name);
		if (tracker.is_null()) {
			return;
		}

		Ref<XRPose> pose = get_pose();
		if (pose.is_valid()) {
			set_transform(pose->get_adjusted_transform());
			_set_has_tracking_data(pose->get_has_tracking_data());
		}
		else {
			_set_has_tracking_data(false);
		}
	}
}

void XRNode3D::_unbind_tracker()
{
	if (tracker.is_valid()) {
		tracker.unref();
		_set_has_tracking_data(false);
	}
}

void XRNode3D::_changed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// just in case unref our current tracker
		_unbind_tracker();

		// get our new tracker
		_bind_tracker();
	}
}

void XRNode3D::_removed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// unref our tracker, it's no longer available
		_unbind_tracker();
	}
}

void XRNode3D::_pose_changed(const Ref<XRPose>& p_pose)
{
	if (p_pose.is_valid() && p_pose->get_name() == pose_name) {
		set_transform(p_pose->get_adjusted_transform());
		_set_has_tracking_data(p_pose->get_has_tracking_data());
	}
}

void XRNode3D::_pose_lost_tracking(const Ref<XRPose>& p_pose)
{
	if (p_pose.is_valid() && p_pose->get_name() == pose_name) {
		_set_has_tracking_data(false);
	}
}

void XRNode3D::_set_has_tracking_data(bool p_has_tracking_data)
{
	if (has_tracking_data != p_has_tracking_data) {
		has_tracking_data = p_has_tracking_data;
		_update_visibility();
	}
}

void XRNode3D::_update_visibility()
{
	// If configured, show or hide the node based on tracking data.
	if (show_when_tracked) {
		// Only react to this if we have a primary interface.
		if (XRServer::is_initialized()) {
			Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
			if (xr_interface.is_valid()) {
				set_visible(has_tracking_data);
			}
		}
	}
}

void XRNode3D::_button_pressed(const String& p_name) {}

void XRNode3D::_button_released(const String& p_name) {}

void XRNode3D::_input_float_changed(const String& p_name, float p_value) {}

void XRNode3D::_input_vector2_changed(const String& p_name, Vector2 p_value) {}

void XRNode3D::_profile_changed(const String& p_role) {}

bool XRNode3D::is_button_pressed(const String& p_name) const
{
	if (tracker.is_null()) {
		return false;
	}
	return tracker->is_button_pressed(p_name);
}

XRTracker::InputValue XRNode3D::get_input(const String& p_name) const
{
	if (tracker.is_null()) {
		return XRTracker::InputValue();
	}
	return tracker->get_input(p_name);
}

float XRNode3D::get_float(const String& p_name) const
{
	if (tracker.is_null()) {
		return 0.0f;
	}
	return tracker->get_input_float(p_name);
}

Vector2 XRNode3D::get_vector2(const String& p_name) const
{
	if (tracker.is_null()) {
		return Vector2();
	}
	return tracker->get_input_vector2(p_name);
}

XRTracker::TrackerHand XRNode3D::get_tracker_hand() const
{
	if (tracker.is_null()) {
		return XRTracker::TRACKER_HAND_UNKNOWN;
	}

	return tracker->get_tracker_hand();
}

Vector3 XRNode3D::get_size() const { return size; }

Plane XRNode3D::get_plane() const
{
	Vector3 location = get_position();
	Basis orientation = get_transform().basis;

	Plane plane(orientation.get_column(1).normalized(), location);

	return plane;
}

PackedStringArray XRNode3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (is_visible() && is_inside_tree()) {
		const XROrigin3D* origin = dynamic_cast<const XROrigin3D*>(get_parent());
		if (origin == nullptr) {
			warnings.push_back(RTR("XRNode3D may not function as expected without an XROrigin3D node as its parent."));
		}
		if (tracker_name.is_empty()) {
			warnings.push_back(RTR("No tracker name is set."));
		}
		if (mode != MODE_ANCHOR && pose_name.is_empty()) {
			warnings.push_back(RTR("No pose is set."));
		}
	}

	if (get_physics_interpolation_mode() != Node::PHYSICS_INTERPOLATION_MODE_OFF) {
		warnings.push_back(RTR("XRNode3D should have physics_interpolation_mode set to OFF in order to avoid jitter."));
	}

	return warnings;
}

XRNode3D::XRNode3D(Mode p_mode) : mode(p_mode) {}

XRNode3D::~XRNode3D() { _unbind_tracker(); }

// XROrigin3D

Vector<XROrigin3D*> XROrigin3D::origin_nodes;

real_t XROrigin3D::get_world_scale() const
{
	// get our XRServer
	ERR_FAIL_COND_V(!XRServer::is_initialized(), 1.0);

	return XRServer::get_world_scale();
}

void XROrigin3D::set_world_scale(real_t p_world_scale)
{
	// get our XRServer
	ERR_FAIL_COND(!XRServer::is_initialized());

	XRServer::set_world_scale(p_world_scale);
}

void XROrigin3D::_set_current(bool p_enabled, bool p_update_others)
{
	// We run this logic even if current already equals p_enabled as we may have set this previously
	// before we entered our tree. This is then called a second time on NOTIFICATION_ENTER_TREE
	// where we actually process activating this origin node.
	current = p_enabled;

	if (!is_inside_tree() || Engine::is_editor_hint()) {
		return;
	}

	// Notify us of any transform changes
	set_notify_local_transform(current);
	set_notify_transform(current);

	// update XRServer with our current position
	if (current) {
		ERR_FAIL_COND(!XRServer::is_initialized());

		XRServer::set_world_origin(get_global_transform());

		if (is_physics_interpolated()) {
			set_process_internal(true);
		}
	}
	else if (is_physics_interpolated()) {
		set_process_internal(false);
	}

	// Check if we need to update our other origin nodes accordingly
	if (p_update_others) {
		if (current) {
			for (int i = 0; i < origin_nodes.size(); i++) {
				if (origin_nodes[i] != this && origin_nodes[i]->current) {
					origin_nodes[i]->_set_current(false, false);
				}
			}
		}
		else {
			// We no longer have a current origin so find the first one we can make current
			for (int i = 0; i < origin_nodes.size(); i++) {
				if (origin_nodes[i] != this) {
					origin_nodes[i]->_set_current(true, false);
					return; // we are done.
				}
			}
		}
	}
}

void XROrigin3D::set_current(bool p_enabled) { _set_current(p_enabled, true); }

bool XROrigin3D::is_current() const
{
	if (Engine::is_editor_hint()) {
		// return as is
		return current;
	}
	else {
		return current && is_inside_tree();
	}
}

void XROrigin3D::_physics_interpolated_changed()
{
	if (current && !Engine::is_editor_hint()) {
		set_process_internal(is_physics_interpolated());
	}
}

PackedStringArray XROrigin3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (is_visible() && is_inside_tree()) {
		bool has_xr_camera = false;
		for (int i = 0; i < get_child_count(); i++) {
			const Camera3D* camera = dynamic_cast<const Camera3D*>(get_child(i));
			if (camera && camera->is_xr_enabled()) {
				has_xr_camera = true;
				break;
			}
		}
		if (!has_xr_camera) {
			warnings.push_back(RTR("XROrigin3D requires an XR-enabled Camera3D child node."));
		}
	}
	return warnings;
}
