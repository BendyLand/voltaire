/**************************************************************************/
/*  godot_physics_server_2d.h                                             */
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

#include "core/templates/rid_owner.h"
#include "godot_constraints_2d.h"
#include "godot_shape_2d.h"
#include "godot_space_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

class GodotPhysicsServer2D final
{
	friend class PhysicsDirectSpaceState2D;
	friend class PhysicsDirectBodyState2D;
	bool active = true;
	bool doing_sync = false;

	int island_count = 0;
	int active_objects = 0;
	int collision_pairs = 0;

	bool using_threads = false;

	bool flushing_queries = false;

	GodotStep2D* stepper = nullptr;
	HashSet<GodotSpace2D*> active_spaces;

	mutable RID_PtrOwner<GodotShape2D, true> shape_owner;
	mutable RID_PtrOwner<GodotSpace2D, true> space_owner;
	mutable RID_PtrOwner<GodotArea2D, true> area_owner;
	mutable RID_PtrOwner<GodotBody2D, true> body_owner{65536, 1048576};
	mutable RID_PtrOwner<GodotJoint2D, true> joint_owner;

	friend class GodotCollisionObject2D;
	SelfList<GodotCollisionObject2D>::List pending_shape_update_list;
	void _update_shapes();

	RID _shape_create(PS2DE::ShapeType p_shape);

public:
	struct CollCbkData
	{
		Vector2 valid_dir;
		real_t valid_depth = 0.0;
		int max = 0;
		int amount = 0;
		int passed = 0;
		int invalid_by_dir = 0;
		Vector2* ptr = nullptr;
	};

	RID world_boundary_shape_create();
	RID separation_ray_shape_create();
	RID segment_shape_create();
	RID circle_shape_create();
	RID rectangle_shape_create();
	RID capsule_shape_create();
	RID convex_polygon_shape_create();
	RID concave_polygon_shape_create();

	static void _shape_col_cbk(
		const Vector2& p_point_A, const Vector2& p_point_B, void* p_userdata);

	void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);

	PS2DE::ShapeType shape_get_type(RID p_shape) const;
	real_t shape_get_custom_solver_bias(RID p_shape) const;

	bool shape_collide(RID p_shape_A, const Transform2D& p_xform_A,
		const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B,
		const Vector2& p_motion_B, Vector2* r_results, int p_result_max,
		int& r_result_count);

	/* SPACE API */

	RID space_create();
	void space_set_active(RID p_space, bool p_active);
	bool space_is_active(RID p_space) const;

	void space_set_param(
		RID p_space, PS2DE::SpaceParameter p_param, real_t p_value);
	real_t space_get_param(RID p_space, PS2DE::SpaceParameter p_param) const;

	void space_set_debug_contacts(RID p_space, int p_max_contacts);
	Vector<Vector2> space_get_contacts(RID p_space) const;
	int space_get_contact_count(RID p_space) const;

	// this function only works on physics process, errors and returns null otherwise
	PhysicsDirectSpaceState2D* space_get_direct_state(RID p_space);

	/* AREA API */

	RID area_create();

	void area_set_space(RID p_area, RID p_space);
	RID area_get_space(RID p_area) const;

	void area_add_shape(RID p_area, RID p_shape,
		const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	void area_set_shape_transform(
		RID p_area, int p_shape_idx, const Transform2D& p_transform);

	int area_get_shape_count(RID p_area) const;
	RID area_get_shape(RID p_area, int p_shape_idx) const;
	Transform2D area_get_shape_transform(RID p_area, int p_shape_idx) const;

	void area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled);

	void area_remove_shape(RID p_area, int p_shape_idx);
	void area_clear_shapes(RID p_area);

	void area_set_transform(RID p_area, const Transform2D& p_transform);

	Transform2D area_get_transform(RID p_area) const;
	void area_set_monitorable(RID p_area, bool p_monitorable);

	void area_set_collision_layer(RID p_area, uint32_t p_layer);
	uint32_t area_get_collision_layer(RID p_area) const;

	void area_set_collision_mask(RID p_area, uint32_t p_mask);
	uint32_t area_get_collision_mask(RID p_area) const;

	void area_set_pickable(RID p_area, bool p_pickable);

	/* BODY API */

	RID body_create();

	void body_set_space(RID p_body, RID p_space);
	RID body_get_space(RID p_body) const;

	void body_set_mode(RID p_body, PS2DE::BodyMode p_mode);
	PS2DE::BodyMode body_get_mode(RID p_body) const;

	void body_add_shape(RID p_body, RID p_shape,
		const Transform2D& p_transform = Transform2D(), bool p_disabled = false);
	void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	void body_set_shape_transform(
		RID p_body, int p_shape_idx, const Transform2D& p_transform);

	int body_get_shape_count(RID p_body) const;
	RID body_get_shape(RID p_body, int p_shape_idx) const;
	Transform2D body_get_shape_transform(RID p_body, int p_shape_idx) const;

	void body_remove_shape(RID p_body, int p_shape_idx);
	void body_clear_shapes(RID p_body);

	void body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled);
	void body_set_shape_as_one_way_collision(RID p_body, int p_shape_idx, bool p_enable,
		real_t p_margin = 0, const Vector2& p_direction = Vector2(0, 1));

	void body_set_continuous_collision_detection_mode(
		RID p_body, PS2DE::CCDMode p_mode);
	PS2DE::CCDMode body_get_continuous_collision_detection_mode(RID p_body) const;

	void body_set_collision_layer(RID p_body, uint32_t p_layer);
	uint32_t body_get_collision_layer(RID p_body) const;

	void body_set_collision_mask(RID p_body, uint32_t p_mask);
	uint32_t body_get_collision_mask(RID p_body) const;

	void body_set_collision_priority(RID p_body, real_t p_priority);
	real_t body_get_collision_priority(RID p_body) const;

	void body_reset_mass_properties(RID p_body);

	void body_apply_central_impulse(RID p_body, const Vector2& p_impulse);
	void body_apply_torque_impulse(RID p_body, real_t p_torque);
	void body_apply_impulse(
		RID p_body, const Vector2& p_impulse, const Vector2& p_position = Vector2());

	void body_apply_central_force(RID p_body, const Vector2& p_force);
	void body_apply_force(
		RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	void body_apply_torque(RID p_body, real_t p_torque);

	void body_add_constant_central_force(RID p_body, const Vector2& p_force);
	void body_add_constant_force(
		RID p_body, const Vector2& p_force, const Vector2& p_position = Vector2());
	void body_add_constant_torque(RID p_body, real_t p_torque);

	void body_set_constant_force(RID p_body, const Vector2& p_force);
	Vector2 body_get_constant_force(RID p_body) const;

	void body_set_constant_torque(RID p_body, real_t p_torque);
	real_t body_get_constant_torque(RID p_body) const;

	void body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity);

	void body_add_collision_exception(RID p_body, RID p_body_b);
	void body_remove_collision_exception(RID p_body, RID p_body_b);
	void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);

	void body_set_contacts_reported_depth_threshold(
		RID p_body, real_t p_threshold);
	real_t body_get_contacts_reported_depth_threshold(RID p_body) const;

	void body_set_omit_force_integration(RID p_body, bool p_omit);
	bool body_is_omitting_force_integration(RID p_body) const;

	void body_set_max_contacts_reported(RID p_body, int p_contacts);
	int body_get_max_contacts_reported(RID p_body) const;

	bool body_collide_shape(RID p_body, int p_body_shape, RID p_shape,
		const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results,
		int p_result_max, int& r_result_count);

	void body_set_pickable(RID p_body, bool p_pickable);

	bool _body_test_motion(RID p_body, PhysicsTestMotionParameters2D* rp_parameters,
		const Ref<PhysicsTestMotionResult2D>& p_result = Ref<PhysicsTestMotionResult2D>());
	bool body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters,
		PS2DT::MotionResult* r_result = nullptr);

	// this function only works on physics process, errors and returns null otherwise
	PhysicsDirectBodyState2D* body_get_direct_state(RID p_body);

	/* JOINT API */

	RID joint_create();

	void joint_clear(RID p_joint);

	void joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value);
	real_t joint_get_param(RID p_joint, PS2DE::JointParam p_param) const;

	void joint_disable_collisions_between_bodies(
		RID p_joint, const bool p_disabled);
	bool joint_is_disabled_collisions_between_bodies(RID p_joint) const;

	void joint_make_pin(
		RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b = RID());
	void joint_make_groove(RID p_joint, const Vector2& p_a_groove1,
		const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b);
	void joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a,
		const Vector2& p_anchor_b, RID p_body_a, RID p_body_b = RID());

	void pin_joint_set_flag(
		RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled);
	bool pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) const;
	void pin_joint_set_param(
		RID p_joint, PS2DE::PinJointParam p_param, real_t p_value);
	real_t pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) const;
	void damped_spring_joint_set_param(
		RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value);
	real_t damped_spring_joint_get_param(
		RID p_joint, PS2DE::DampedSpringParam p_param) const;

	PS2DE::JointType joint_get_type(RID p_joint) const;

	/* MISC */

	void free_rid(RID p_rid);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	void set_active(bool p_active);
	void init();
	void step(real_t p_step);
	void sync();
	void flush_queries();
	void end_sync();
	void finish();

	bool is_flushing_queries() const { return flushing_queries; }

	int get_process_info(PS2DE::ProcessInfo p_info);

	explicit GodotPhysicsServer2D(bool p_using_threads = false);

	~GodotPhysicsServer2D()
	{
		finish();
	}
};
