/**************************************************************************/
/*  xr_nodes.h                                                            */
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

#pragma once

#include "scene/3d/node_3d.h"
#include "servers/xr/xr_tracker.h"

/*
	XRNode3D is a consolidated helper node that binds to an XRTracker
	(supporting generic tracker, controller, and anchor modes).

	It must be a child node of an XROrigin3D node.
*/

class XRNode3D final : public Node3D
{
public:
	enum Mode
	{
		MODE_TRACKER,
		MODE_CONTROLLER,
		MODE_ANCHOR,
	};

private:
	Mode mode = MODE_TRACKER;
	String tracker_name;
	String pose_name = SceneString(default_);
	bool has_tracking_data = false;
	bool show_when_tracked = false;
	Vector3 size;

	Ref<XRTracker> tracker;

	void _bind_tracker();
	void _unbind_tracker();
	void _changed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _removed_tracker(const String& p_tracker_name, int p_tracker_type);

	void _pose_changed(const Ref<XRPose>& p_pose);
	void _pose_lost_tracking(const Ref<XRPose>& p_pose);
	void _set_has_tracking_data(bool p_has_tracking_data);

	void _update_visibility();

	void _button_pressed(const String& p_name);
	void _button_released(const String& p_name);
	void _input_float_changed(const String& p_name, float p_value);
	void _input_vector2_changed(const String& p_name, Vector2 p_value);
	void _profile_changed(const String& p_role);

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const { return mode; }

	void set_tracker(const String& p_tracker_name);
	String get_tracker() const;

	void set_pose_name(const String& p_pose);
	String get_pose_name() const;

	bool get_is_active() const;
	bool get_has_tracking_data() const;

	void set_show_when_tracked(bool p_show);
	bool get_show_when_tracked() const;

	void trigger_haptic_pulse(const String& p_action_name, double p_frequency, double p_amplitude,
		double p_duration_sec, double p_delay_sec = 0);

	Ref<XRPose> get_pose();

	// Controller methods (MODE_CONTROLLER)
	bool is_button_pressed(const String& p_name) const;
	XRTracker::InputValue get_input(const String& p_name) const;
	float get_float(const String& p_name) const;
	Vector2 get_vector2(const String& p_name) const;
	XRTracker::TrackerHand get_tracker_hand() const;

	// Anchor methods (MODE_ANCHOR)
	Vector3 get_size() const;
	Plane get_plane() const;

	PackedStringArray get_configuration_warnings() const override;

	XRNode3D(Mode p_mode = MODE_TRACKER);
	~XRNode3D();
};

/*
	XROrigin3D is a special spatial node that acts as our origin point mapping our real world center
	of our tracking volume into our virtual world.
*/

class XROrigin3D final : public Node3D
{
private:
	bool current = false;
	static Vector<XROrigin3D*> origin_nodes; // all origin nodes in tree

	void _set_current(bool p_enabled, bool p_update_others);

protected:
	void _notification(int p_what);
	virtual void _physics_interpolated_changed() override;

public:
	PackedStringArray get_configuration_warnings() const override;

	real_t get_world_scale() const;
	void set_world_scale(real_t p_world_scale);

	void set_current(bool p_enabled);
	bool is_current() const;
};
