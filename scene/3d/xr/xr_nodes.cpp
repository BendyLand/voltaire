/**************************************************************************/
/*  xr_nodes.cpp                                                          */
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
#include "core/config/project_settings.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/xr/xr_interface.h"
#include "xr_nodes.h"

void XRCamera3D::_changed_tracker(const StringName& p_tracker_name, int p_tracker_type)
{
	if (p_tracker_name == tracker_name) {
		_bind_tracker();
	}
}

void XRCamera3D::_removed_tracker(const StringName& p_tracker_name, int p_tracker_type)
{
	if (p_tracker_name == tracker_name) {
		_unbind_tracker();
	}
}

void XRCamera3D::_pose_changed(const Ref<XRPose>& p_pose)
{
	if (p_pose->get_name() == pose_name) {
		set_transform(p_pose->get_adjusted_transform());
	}
}

void XRCamera3D::_physics_interpolated_changed()
{
	Camera3D::_physics_interpolated_changed();
	update_configuration_warnings();
}

Vector3 XRCamera3D::project_local_ray_normal(const Point2& p_pos) const
{
	// get our XRServer
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Vector3());

	Ref<XRInterface> xr_interface = xr_server->get_primary_interface();
	if (xr_interface.is_null()) {
		// we might be in the editor or have VR turned off, just call superclass
		return Camera3D::project_local_ray_normal(p_pos);
	}

	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");

	Size2 viewport_size = get_viewport()->get_camera_rect_size();
	Vector2 cpos = get_viewport()->get_camera_coords(p_pos);
	Vector3 ray;

	// Just use the first view, if multiple views are supported this function has no good result
	Projection cm =
		xr_interface->get_projection_for_view(0, viewport_size.aspect(), get_near(), get_far());
	Vector2 screen_he = cm.get_viewport_half_extents();
	ray = Vector3(((cpos.x / viewport_size.width) * 2.0 - 1.0) * screen_he.x,
		((1.0 - (cpos.y / viewport_size.height)) * 2.0 - 1.0) * screen_he.y, -get_near())
			  .normalized();

	return ray;
}

Point2 XRCamera3D::unproject_position(const Vector3& p_pos) const
{
	// get our XRServer
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Vector2());

	Ref<XRInterface> xr_interface = xr_server->get_primary_interface();
	if (xr_interface.is_null()) {
		// we might be in the editor or have VR turned off, just call superclass
		return Camera3D::unproject_position(p_pos);
	}

	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector2(), "Camera is not inside scene.");

	Size2 viewport_size = get_viewport()->get_visible_rect().size;

	// Just use the first view, if multiple views are supported this function has no good result
	Projection cm =
		xr_interface->get_projection_for_view(0, viewport_size.aspect(), get_near(), get_far());

	Plane p(get_camera_transform().xform_inv(p_pos), 1.0);

	p = cm.xform4(p);
	p.normal /= p.d;

	Point2 res;
	res.x = (p.normal.x * 0.5 + 0.5) * viewport_size.x;
	res.y = (-p.normal.y * 0.5 + 0.5) * viewport_size.y;

	return res;
}

Vector3 XRCamera3D::project_position(const Point2& p_point, real_t p_z_depth) const
{
	// get our XRServer
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Vector3());

	Ref<XRInterface> xr_interface = xr_server->get_primary_interface();
	if (xr_interface.is_null()) {
		// we might be in the editor or have VR turned off, just call superclass
		return Camera3D::project_position(p_point, p_z_depth);
	}

	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");

	Size2 viewport_size = get_viewport()->get_visible_rect().size;

	// Just use the first view, if multiple views are supported this function has no good result
	Projection cm =
		xr_interface->get_projection_for_view(0, viewport_size.aspect(), get_near(), get_far());

	Vector2 vp_he = cm.get_viewport_half_extents();

	Vector2 point;
	point.x = (p_point.x / viewport_size.x) * 2.0 - 1.0;
	point.y = (1.0 - (p_point.y / viewport_size.y)) * 2.0 - 1.0;
	point *= vp_he;

	Vector3 p(point.x, point.y, -p_z_depth);

	return get_camera_transform().xform(p);
}

Vector<Plane> XRCamera3D::get_frustum() const
{
	// get our XRServer
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, Vector<Plane>());

	Ref<XRInterface> xr_interface = xr_server->get_primary_interface();
	if (xr_interface.is_null()) {
		// we might be in the editor or have VR turned off, just call superclass
		return Camera3D::get_frustum();
	}

	ERR_FAIL_COND_V(!is_inside_world(), Vector<Plane>());

	Size2 viewport_size = get_viewport()->get_visible_rect().size;
	// TODO Just use the first view for now, this is mostly for debugging so we may look into using
	// our combined projection here.
	Projection cm =
		xr_interface->get_projection_for_view(0, viewport_size.aspect(), get_near(), get_far());
	return cm.get_projection_planes(get_camera_transform());
}

// XRNode3D is a node that has it's transform updated by an XRPositionalTracker.
// Note that trackers are only available in runtime and only after an XRInterface registers one.
// So we bind by name and as long as a tracker isn't available, our node remains inactive.

StringName XRNode3D::get_tracker() const { return tracker_name; }

void XRNode3D::set_pose_name(const StringName& p_pose_name)
{
	pose_name = p_pose_name;

	// Update pose if we are bound to a tracker with a valid pose
	Ref<XRPose> pose = get_pose();
	if (pose.is_valid()) {
		set_transform(pose->get_adjusted_transform());
	}
}

StringName XRNode3D::get_pose_name() const { return pose_name; }

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
	XRServer* xr_server = XRServer::get_singleton();
	if (xr_server != nullptr) {
		Ref<XRInterface> xr_interface = xr_server->get_primary_interface();
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

void XRNode3D::_changed_tracker(const StringName& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// just in case unref our current tracker
		_unbind_tracker();

		// get our new tracker
		_bind_tracker();
	}
}

void XRNode3D::_removed_tracker(const StringName& p_tracker_name, int p_tracker_type)
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

void XRNode3D::_update_visibility()
{
	// If configured, show or hide the node based on tracking data.
	if (show_when_tracked) {
		// Only react to this if we have a primary interface.
		XRServer* xr_server = XRServer::get_singleton();
		if (xr_server != nullptr) {
			Ref<XRInterface> xr_interface = xr_server->get_primary_interface();
			if (xr_interface.is_valid()) {
				set_visible(has_tracking_data);
			}
		}
	}
}

void XRNode3D::_physics_interpolated_changed() { update_configuration_warnings(); }

////////////////////////////////////////////////////////////////////////////////////////////////////

XRPositionalTracker::TrackerHand XRController3D::get_tracker_hand() const
{
	// get our XRServer
	if (tracker.is_null()) {
		return XRPositionalTracker::TRACKER_HAND_UNKNOWN;
	}

	return tracker->get_tracker_hand();
}

Vector3 XRAnchor3D::get_size() const { return size; }

Plane XRAnchor3D::get_plane() const
{
	Vector3 location = get_position();
	Basis orientation = get_transform().basis;

	Plane plane(orientation.get_column(1).normalized(), location);

	return plane;
}

Vector<XROrigin3D*> XROrigin3D::origin_nodes;

void XROrigin3D::_bind_methods() {}

real_t XROrigin3D::get_world_scale() const
{
	// get our XRServer
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL_V(xr_server, 1.0);

	return xr_server->get_world_scale();
}

void XROrigin3D::set_world_scale(real_t p_world_scale)
{
	// get our XRServer
	XRServer* xr_server = XRServer::get_singleton();
	ERR_FAIL_NULL(xr_server);

	xr_server->set_world_scale(p_world_scale);
}

void XROrigin3D::_set_current(bool p_enabled, bool p_update_others)
{
	// We run this logic even if current already equals p_enabled as we may have set this previously
	// before we entered our tree. This is then called a second time on NOTIFICATION_ENTER_TREE
	// where we actually process activating this origin node.
	current = p_enabled;

	if (!is_inside_tree() || Engine::get_singleton()->is_editor_hint()) {
		return;
	}

	// Notify us of any transform changes
	set_notify_local_transform(current);
	set_notify_transform(current);

	// update XRServer with our current position
	if (current) {
		XRServer* xr_server = XRServer::get_singleton();
		ERR_FAIL_NULL(xr_server);

		xr_server->set_world_origin(get_global_transform());

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
	if (Engine::get_singleton()->is_editor_hint()) {
		// return as is
		return current;
	}
	else {
		return current && is_inside_tree();
	}
}

void XROrigin3D::_physics_interpolated_changed()
{
	if (current && !Engine::get_singleton()->is_editor_hint()) {
		set_process_internal(is_physics_interpolated());
	}
}


