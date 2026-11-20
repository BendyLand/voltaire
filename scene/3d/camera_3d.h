/**************************************************************************/
/*  camera_3d.h                                                           */
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

#include "core/templates/interpolated_property.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/velocity_tracker_3d.h"
#include "scene/resources/camera_attributes.h"
#include "scene/resources/compositor.h"
#include "scene/resources/environment.h"
#include "servers/xr/xr_positional_tracker.h"

class Camera3D : public Node3D
{
public:
	enum ProjectionType
	{
		PROJECTION_PERSPECTIVE,
		PROJECTION_ORTHOGONAL,
		PROJECTION_FRUSTUM
	};

	enum KeepAspect
	{
		KEEP_WIDTH,
		KEEP_HEIGHT
	};

	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP
	};

private:
	bool force_change = false;
	bool current = false;
	Viewport* viewport = nullptr;

	ProjectionType mode = PROJECTION_PERSPECTIVE;

	InterpolatedProperty<real_t> fov = 75.0;
	InterpolatedProperty<real_t> size = 1.0;
	InterpolatedProperty<Vector2> frustum_offset;
	// _ prefix to avoid conflict with Windows defines.
	InterpolatedProperty<real_t> _near = 0.05;
	InterpolatedProperty<real_t> _far = 4000.0;
	real_t v_offset = 0.0;
	real_t h_offset = 0.0;
	KeepAspect keep_aspect = KEEP_HEIGHT;

	RID camera;
	RID scenario_id;

	uint32_t layers = 0xfffff;

	Ref<Environment> environment;
	Ref<CameraAttributes> attributes;
	Ref<Compositor> compositor;
	void _attributes_changed();

	friend class Viewport;
	void _update_audio_listener_state();

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;
	Ref<VelocityTracker3D> velocity_tracker;

#ifndef PHYSICS_3D_DISABLED
	RID pyramid_shape;
	Vector<Vector3> pyramid_shape_points;
#endif // PHYSICS_3D_DISABLED

	bool _desired_process_internal = false;
	bool _desired_physics_process_internal = false;

	bool xr_enabled = false;
	String tracker_name = "head";
	String pose_name = "default";
	Ref<XRPositionalTracker> tracker;

	void _bind_tracker();
	void _unbind_tracker();
	void _changed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _removed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _pose_changed(const Ref<XRPose>& p_pose);

	void _update_process_mode();

protected:
	void set_desired_process_modes(bool p_process_internal, bool p_physics_process_internal);

	virtual void _physics_interpolated_changed() override;
	Transform3D _get_adjusted_camera_transform(const Transform3D& p_xform) const;

	void _update_camera();
	void _request_camera_update();

	virtual void fti_pump_property() override;
	virtual void fti_update_servers_property() override;
	virtual void fti_update_servers_xform() override;

	void _notification(int p_what);

	Projection _get_camera_projection(real_t p_near) const;

public:
	enum
	{
		NOTIFICATION_BECAME_CURRENT = 50,
		NOTIFICATION_LOST_CURRENT = 51
	};

	void set_xr_enabled(bool p_enabled);
	bool is_xr_enabled() const { return xr_enabled; }

	PackedStringArray get_configuration_warnings() const override;

	void set_projection(Camera3D::ProjectionType p_mode);

	void make_current();
	void clear_current(bool p_enable_next = true);
	void set_current(bool p_enabled);
	bool is_current() const;

	RID get_camera() const;

	real_t get_fov() const;
	real_t get_size() const;
	real_t get_far() const;
	real_t get_near() const;
	Vector2 get_frustum_offset() const;

	ProjectionType get_projection() const;

	Transform3D get_camera_transform() const;
	Projection get_camera_projection() const;

	Vector3 project_ray_normal(const Point2& p_pos) const;
	Vector3 project_ray_origin(const Point2& p_pos) const;
	Vector3 project_local_ray_normal(const Point2& p_pos) const;
	Point2 unproject_position(const Vector3& p_pos) const;
	bool is_position_behind(const Vector3& p_pos) const;
	Vector3 project_position(const Point2& p_point, real_t p_z_depth) const;

	Vector<Vector3> get_near_plane_points() const;

	uint32_t get_cull_mask() const;

	bool get_cull_mask_value(int p_layer_number) const;

	Vector<Plane> get_frustum() const;
	bool is_position_in_frustum(const Vector3& p_position) const;

	Ref<Environment> get_environment() const;

	void set_attributes(const Ref<CameraAttributes>& p_effects);
	Ref<CameraAttributes> get_attributes() const;

	Ref<Compositor> get_compositor() const;

	void set_keep_aspect_mode(KeepAspect p_aspect);
	KeepAspect get_keep_aspect_mode() const;

	void set_v_offset(real_t p_offset);
	real_t get_v_offset() const;

	void set_h_offset(real_t p_offset);
	real_t get_h_offset() const;

	DopplerTracking get_doppler_tracking() const;

	Vector3 get_doppler_tracked_velocity() const;

#ifndef PHYSICS_3D_DISABLED
	RID get_pyramid_shape_rid();
#endif // PHYSICS_3D_DISABLED

	Camera3D(bool p_xr = false);
	~Camera3D();
};
