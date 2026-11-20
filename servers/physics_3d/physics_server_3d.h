/**************************************************************************/
/*  physics_server_3d.h                                                   */
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

#include "servers/physics_3d/direct_states/physics_direct_body_state_3d.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"
#include "servers/physics_3d/physics_server_3d_enums.h"
#include "servers/physics_3d/physics_server_3d_rendering_server_handler.h"
#include "servers/physics_3d/physics_server_3d_types.h"
#include "servers/physics_3d/queries/physics_testmotion_query_parameters_3d.h"
#include "servers/physics_3d/queries/physics_testmotion_query_result_3d.h"

class JoltPhysicsServer3D;
class PhysicsDirectSpaceState3D;
template <typename T> class TypedArray;

template <typename T> class Ref;

class PhysicsTestMotionParameters3D;
class PhysicsTestMotionResult3D;

class PhysicsServer3D final
{
public:
	struct Data
	{
		JoltPhysicsServer3D* server = nullptr;
	};

	static inline Data* data = nullptr;

	static void initialize(bool p_on_separate_thread = false);
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized()
	{
		return data != nullptr && data->server != nullptr;
	}
	_FORCE_INLINE_ static JoltPhysicsServer3D* get_driver() { return data ? data->server : nullptr; }

	static void soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler);
	static RID shape_create(PS3DE::ShapeType p_shape);
	static RID world_boundary_shape_create();
	static RID separation_ray_shape_create();
	static RID sphere_shape_create();
	static RID box_shape_create();
	static RID capsule_shape_create();
	static RID cylinder_shape_create();
	static RID convex_polygon_shape_create();
	static RID concave_polygon_shape_create();
	static RID heightmap_shape_create();
	static RID custom_shape_create();
	static void shape_set_custom_solver_bias(RID p_shape, real_t p_bias);
	static PS3DE::ShapeType shape_get_type(RID p_shape);
	static void shape_set_margin(RID p_shape, real_t p_margin);
	static real_t shape_get_margin(RID p_shape);
	static real_t shape_get_custom_solver_bias(RID p_shape);
	static RID space_create();
	static void space_set_active(RID p_space, bool p_active);
	static bool space_is_active(RID p_space);
	static void space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value);
	static real_t space_get_param(RID p_space, PS3DE::SpaceParameter p_param);
	static PhysicsDirectSpaceState3D* space_get_direct_state(RID p_space);
	static void space_set_debug_contacts(RID p_space, int p_max_contacts);
	static Vector<Vector3> space_get_contacts(RID p_space);
	static int space_get_contact_count(RID p_space);
	static RID area_create();
	static void area_set_space(RID p_area, RID p_space);
	static RID area_get_space(RID p_area);
	static void area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform = Transform3D(), bool p_disabled = false);
	static void area_set_shape(RID p_area, int p_shape_idx, RID p_shape);
	static void area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform);
	static int area_get_shape_count(RID p_area);
	static RID area_get_shape(RID p_area, int p_shape_idx);
	static Transform3D area_get_shape_transform(RID p_area, int p_shape_idx);
	static void area_remove_shape(RID p_area, int p_shape_idx);
	static void area_clear_shapes(RID p_area);
	static void area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled);
	static void area_set_transform(RID p_area, const Transform3D& p_transform);
	static Transform3D area_get_transform(RID p_area);
	static void area_set_collision_layer(RID p_area, uint32_t p_layer);
	static uint32_t area_get_collision_layer(RID p_area);
	static void area_set_collision_mask(RID p_area, uint32_t p_mask);
	static uint32_t area_get_collision_mask(RID p_area);
	static void area_set_monitorable(RID p_area, bool p_monitorable);
	static void area_set_ray_pickable(RID p_area, bool p_enable);
	static RID body_create();
	static void body_set_space(RID p_body, RID p_space);
	static RID body_get_space(RID p_body);
	static void body_set_mode(RID p_body, PS3DE::BodyMode p_mode);
	static PS3DE::BodyMode body_get_mode(RID p_body);
	static void body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform = Transform3D(), bool p_disabled = false);
	static void body_set_shape(RID p_body, int p_shape_idx, RID p_shape);
	static void body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform);
	static int body_get_shape_count(RID p_body);
	static RID body_get_shape(RID p_body, int p_shape_idx);
	static Transform3D body_get_shape_transform(RID p_body, int p_shape_idx);
	static void body_remove_shape(RID p_body, int p_shape_idx);
	static void body_clear_shapes(RID p_body);
	static void body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled);
	static void body_set_enable_continuous_collision_detection(RID p_body, bool p_enable);
	static bool body_is_continuous_collision_detection_enabled(RID p_body);
	static void body_set_collision_layer(RID p_body, uint32_t p_layer);
	static uint32_t body_get_collision_layer(RID p_body);
	static void body_set_collision_mask(RID p_body, uint32_t p_mask);
	static uint32_t body_get_collision_mask(RID p_body);
	static void body_set_collision_priority(RID p_body, real_t p_priority);
	static real_t body_get_collision_priority(RID p_body);
	static void body_set_user_flags(RID p_body, uint32_t p_flags);
	static uint32_t body_get_user_flags(RID p_body);
	static void body_reset_mass_properties(RID p_body);
	static void body_apply_central_impulse(RID p_body, const Vector3& p_impulse);
	static void body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position = Vector3());
	static void body_apply_torque_impulse(RID p_body, const Vector3& p_impulse);
	static void body_apply_central_force(RID p_body, const Vector3& p_force);
	static void body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position = Vector3());
	static void body_apply_torque(RID p_body, const Vector3& p_torque);
	static void body_add_constant_central_force(RID p_body, const Vector3& p_force);
	static void body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position = Vector3());
	static void body_add_constant_torque(RID p_body, const Vector3& p_torque);
	static void body_set_constant_force(RID p_body, const Vector3& p_force);
	static Vector3 body_get_constant_force(RID p_body);
	static void body_set_constant_torque(RID p_body, const Vector3& p_torque);
	static Vector3 body_get_constant_torque(RID p_body);
	static void body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity);
	static void body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock);
	static bool body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis);
	static void body_add_collision_exception(RID p_body, RID p_body_b);
	static void body_remove_collision_exception(RID p_body, RID p_body_b);
	static void body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);
	static void body_set_max_contacts_reported(RID p_body, int p_contacts);
	static int body_get_max_contacts_reported(RID p_body);
	static void body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold);
	static real_t body_get_contacts_reported_depth_threshold(RID p_body);
	static void body_set_omit_force_integration(RID p_body, bool p_omit);
	static bool body_is_omitting_force_integration(RID p_body);
	static void body_set_ray_pickable(RID p_body, bool p_enable);
	static PhysicsDirectBodyState3D* body_get_direct_state(RID p_body);
	static bool body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result = nullptr);
	static RID soft_body_create();
	static void soft_body_set_space(RID p_body, RID p_space);
	static RID soft_body_get_space(RID p_body);
	static void soft_body_set_mesh(RID p_body, RID p_mesh);
	static AABB soft_body_get_bounds(RID p_body);
	static void soft_body_set_collision_layer(RID p_body, uint32_t p_layer);
	static uint32_t soft_body_get_collision_layer(RID p_body);
	static void soft_body_set_collision_mask(RID p_body, uint32_t p_mask);
	static uint32_t soft_body_get_collision_mask(RID p_body);
	static void soft_body_add_collision_exception(RID p_body, RID p_body_b);
	static void soft_body_remove_collision_exception(RID p_body, RID p_body_b);
	static void soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions);
	static void soft_body_set_transform(RID p_body, const Transform3D& p_transform);
	static void soft_body_set_ray_pickable(RID p_body, bool p_enable);
	static void soft_body_set_simulation_precision(RID p_body, int p_simulation_precision);
	static int soft_body_get_simulation_precision(RID p_body);
	static void soft_body_set_total_mass(RID p_body, real_t p_total_mass);
	static real_t soft_body_get_total_mass(RID p_body);
	static void soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness);
	static real_t soft_body_get_linear_stiffness(RID p_body);
	static void soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor);
	static real_t soft_body_get_shrinking_factor(RID p_body);
	static void soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient);
	static real_t soft_body_get_pressure_coefficient(RID p_body);
	static void soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient);
	static real_t soft_body_get_damping_coefficient(RID p_body);
	static void soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient);
	static real_t soft_body_get_drag_coefficient(RID p_body);
	static void soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position);
	static Vector3 soft_body_get_point_global_position(RID p_body, int p_point_index);
	static void soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse);
	static void soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force);
	static void soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse);
	static void soft_body_apply_central_force(RID p_body, const Vector3& p_force);
	static void soft_body_remove_all_pinned_points(RID p_body);
	static void soft_body_pin_point(RID p_body, int p_point_index, bool p_pin);
	static bool soft_body_is_point_pinned(RID p_body, int p_point_index);
	static RID joint_create();
	static void joint_clear(RID p_joint);
	static PS3DE::JointType joint_get_type(RID p_joint);
	static void joint_set_solver_priority(RID p_joint, int p_priority);
	static int joint_get_solver_priority(RID p_joint);
	static void joint_disable_collisions_between_bodies(RID p_joint, bool p_disable);
	static bool joint_is_disabled_collisions_between_bodies(RID p_joint);
	static void joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B);
	static void pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value);
	static real_t pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param);
	static void pin_joint_set_local_a(RID p_joint, const Vector3& p_A);
	static Vector3 pin_joint_get_local_a(RID p_joint);
	static void pin_joint_set_local_b(RID p_joint, const Vector3& p_B);
	static Vector3 pin_joint_get_local_b(RID p_joint);
	static void joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B);
	static void joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B);
	static void hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value);
	static real_t hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param);
	static void hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled);
	static bool hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag);
	static void joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B);
	static void slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value);
	static real_t slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param);
	static void joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B);
	static void cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value);
	static real_t cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param);
	static void joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B);
	static void generic_6dof_joint_set_param(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisParam p_param, real_t p_value);
	static real_t generic_6dof_joint_get_param(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisParam p_param);
	static void generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable);
	static bool generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis, PS3DE::G6DOFJointAxisFlag p_flag);
	static void generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation);
	static Quaternion generic_6dof_joint_get_angular_target_rotation(RID p_joint);
	static void free_rid(RID p_rid);
	static void set_active(bool p_active);
	static void init();
	static void step(real_t p_step);
	static void sync();
	static void flush_queries();
	static void end_sync();
	static void finish();
	static bool is_flushing_queries();
	static int get_process_info(PS3DE::ProcessInfo p_info);
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use `free_rid()` instead.")]] static void free(RID p_rid) { free_rid(p_rid); }
#endif // DISABLE_DEPRECATED

	PhysicsServer3D() = delete;
	PhysicsServer3D(const PhysicsServer3D&) = delete;
	PhysicsServer3D& operator=(const PhysicsServer3D&) = delete;
	~PhysicsServer3D() = delete;
};
