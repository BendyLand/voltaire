/**************************************************************************/
/*  physics_server_2d.cpp                                                 */
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

#include "physics_server_2d.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#endif

void PhysicsServer2D::initialize(bool p_using_threads)
{
#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED
	if (is_initialized()) {
		return;
	}
	if (!data) {
		data = memnew(Data);
	}
	data->server = memnew(GodotPhysicsServer2D(p_using_threads));
	data->server->init();
#endif
}

void PhysicsServer2D::finalize()
{
	if (!data) {
		return;
	}
#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED
	if (data->server) {
		data->server->finish();
		memdelete(data->server);
		data->server = nullptr;
	}
#endif
	memdelete(data);
	data = nullptr;
}

#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED

RID PhysicsServer2D::world_boundary_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->world_boundary_shape_create();
}

RID PhysicsServer2D::separation_ray_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->separation_ray_shape_create();
}

RID PhysicsServer2D::segment_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->segment_shape_create();
}

RID PhysicsServer2D::circle_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->circle_shape_create();
}

RID PhysicsServer2D::rectangle_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->rectangle_shape_create();
}

RID PhysicsServer2D::capsule_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->capsule_shape_create();
}

RID PhysicsServer2D::convex_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->convex_polygon_shape_create();
}

RID PhysicsServer2D::concave_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->concave_polygon_shape_create();
}

void PhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->shape_set_custom_solver_bias(p_shape, p_bias);
}

PS2DE::ShapeType PhysicsServer2D::shape_get_type(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::ShapeType());
	return data->server->shape_get_type(p_shape);
}

real_t PhysicsServer2D::shape_get_custom_solver_bias(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->shape_get_custom_solver_bias(p_shape);
}

bool PhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A, const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B, const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->shape_collide(p_shape_A, p_xform_A, p_motion_A, p_shape_B, p_xform_B, p_motion_B, r_results, p_result_max, r_result_count);
}

RID PhysicsServer2D::space_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->space_create();
}

void PhysicsServer2D::space_set_active(RID p_space, bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_active(p_space, p_active);
}

bool PhysicsServer2D::space_is_active(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->space_is_active(p_space);
}

void PhysicsServer2D::space_set_param(RID p_space, PS2DE::SpaceParameter p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_param(p_space, p_param, p_value);
}

real_t PhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_param(p_space, p_param);
}

PhysicsDirectSpaceState2D* PhysicsServer2D::space_get_direct_state(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->space_get_direct_state(p_space);
}

void PhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_debug_contacts(p_space, p_max_contacts);
}

Vector<Vector2> PhysicsServer2D::space_get_contacts(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<Vector2>());
	return data->server->space_get_contacts(p_space);
}

int PhysicsServer2D::space_get_contact_count(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_contact_count(p_space);
}

RID PhysicsServer2D::area_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_create();
}

void PhysicsServer2D::area_set_space(RID p_area, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_space(p_area, p_space);
}

RID PhysicsServer2D::area_get_space(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_space(p_area);
}

void PhysicsServer2D::area_add_shape(RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_add_shape(p_area, p_shape, p_transform, p_disabled);
}

void PhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape(p_area, p_shape_idx, p_shape);
}

void PhysicsServer2D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_transform(p_area, p_shape_idx, p_transform);
}

int PhysicsServer2D::area_get_shape_count(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_shape_count(p_area);
}

RID PhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_shape(p_area, p_shape_idx);
}

Transform2D PhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->server->area_get_shape_transform(p_area, p_shape_idx);
}

void PhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_remove_shape(p_area, p_shape_idx);
}

void PhysicsServer2D::area_clear_shapes(RID p_area)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_clear_shapes(p_area);
}

void PhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_disabled(p_area, p_shape, p_disabled);
}

void PhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_transform(p_area, p_transform);
}

Transform2D PhysicsServer2D::area_get_transform(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->server->area_get_transform(p_area);
}

void PhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_layer(p_area, p_layer);
}

uint32_t PhysicsServer2D::area_get_collision_layer(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_layer(p_area);
}

void PhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_mask(p_area, p_mask);
}

uint32_t PhysicsServer2D::area_get_collision_mask(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_mask(p_area);
}

void PhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_monitorable(p_area, p_monitorable);
}

void PhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_pickable(p_area, p_pickable);
}

RID PhysicsServer2D::body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_create();
}

void PhysicsServer2D::body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_space(p_body, p_space);
}

RID PhysicsServer2D::body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_space(p_body);
}

void PhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_mode(p_body, p_mode);
}

PS2DE::BodyMode PhysicsServer2D::body_get_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::BodyMode());
	return data->server->body_get_mode(p_body);
}

void PhysicsServer2D::body_add_shape(RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_shape(p_body, p_shape, p_transform, p_disabled);
}

void PhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape(p_body, p_shape_idx, p_shape);
}

void PhysicsServer2D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_transform(p_body, p_shape_idx, p_transform);
}

int PhysicsServer2D::body_get_shape_count(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_shape_count(p_body);
}

RID PhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_shape(p_body, p_shape_idx);
}

Transform2D PhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->server->body_get_shape_transform(p_body, p_shape_idx);
}

void PhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_disabled(p_body, p_shape, p_disabled);
}

void PhysicsServer2D::body_set_shape_as_one_way_collision(RID p_body, int p_shape, bool p_enabled, real_t p_margin, const Vector2& p_direction)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_as_one_way_collision(p_body, p_shape, p_enabled, p_margin, p_direction);
}

void PhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_shape(p_body, p_shape_idx);
}

void PhysicsServer2D::body_clear_shapes(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_clear_shapes(p_body);
}

void PhysicsServer2D::body_set_continuous_collision_detection_mode(RID p_body, PS2DE::CCDMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_continuous_collision_detection_mode(p_body, p_mode);
}

PS2DE::CCDMode PhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::CCDMode());
	return data->server->body_get_continuous_collision_detection_mode(p_body);
}

void PhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer2D::body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_layer(p_body);
}

void PhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer2D::body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_mask(p_body);
}

void PhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_priority(p_body, p_priority);
}

real_t PhysicsServer2D::body_get_collision_priority(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_priority(p_body);
}

void PhysicsServer2D::body_reset_mass_properties(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_reset_mass_properties(p_body);
}

void PhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque_impulse(p_body, p_torque);
}

void PhysicsServer2D::body_apply_impulse(RID p_body, const Vector2& p_impulse, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_impulse(p_body, p_impulse, p_position);
}

void PhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_force(p_body, p_force);
}

void PhysicsServer2D::body_apply_force(RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_force(p_body, p_force, p_position);
}

void PhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque(p_body, p_torque);
}

void PhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_central_force(p_body, p_force);
}

void PhysicsServer2D::body_add_constant_force(RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_force(p_body, p_force, p_position);
}

void PhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_torque(p_body, p_torque);
}

void PhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_force(p_body, p_force);
}

Vector2 PhysicsServer2D::body_get_constant_force(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector2());
	return data->server->body_get_constant_force(p_body);
}

void PhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_torque(p_body, p_torque);
}

real_t PhysicsServer2D::body_get_constant_torque(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_constant_torque(p_body);
}

void PhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_axis_velocity(p_body, p_axis_velocity);
}

void PhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_max_contacts_reported(p_body, p_contacts);
}

int PhysicsServer2D::body_get_max_contacts_reported(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_max_contacts_reported(p_body);
}

void PhysicsServer2D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_contacts_reported_depth_threshold(p_body, p_threshold);
}

real_t PhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_contacts_reported_depth_threshold(p_body);
}

void PhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_omit_force_integration(p_body, p_omit);
}

bool PhysicsServer2D::body_is_omitting_force_integration(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_omitting_force_integration(p_body);
}

bool PhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape, const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max, int& r_result_count)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_collide_shape(p_body, p_body_shape, p_shape, p_shape_xform, p_motion, r_results, p_result_max, r_result_count);
}

void PhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_pickable(p_body, p_pickable);
}

PhysicsDirectBodyState2D* PhysicsServer2D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->body_get_direct_state(p_body);
}

bool PhysicsServer2D::body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_test_motion(p_body, p_parameters, r_result);
}

RID PhysicsServer2D::joint_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->joint_create();
}

void PhysicsServer2D::joint_clear(RID p_joint)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_clear(p_joint);
}

void PhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->joint_get_param(p_joint, p_param);
}

void PhysicsServer2D::joint_disable_collisions_between_bodies(RID p_joint, const bool p_disable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_disable_collisions_between_bodies(p_joint, p_disable);
}

bool PhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->joint_is_disabled_collisions_between_bodies(p_joint);
}

void PhysicsServer2D::joint_make_pin(RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_pin(p_joint, p_anchor, p_body_a, p_body_b);
}

void PhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1, const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_groove(p_joint, p_a_groove1, p_a_groove2, p_b_anchor, p_body_a, p_body_b);
}

void PhysicsServer2D::joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_damped_spring(p_joint, p_anchor_a, p_anchor_b, p_body_a, p_body_b);
}

void PhysicsServer2D::pin_joint_set_param(RID p_joint, PS2DE::PinJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->pin_joint_get_param(p_joint, p_param);
}

void PhysicsServer2D::pin_joint_set_flag(RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_flag(p_joint, p_flag, p_enabled);
}

bool PhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->pin_joint_get_flag(p_joint, p_flag);
}

void PhysicsServer2D::damped_spring_joint_set_param(RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->damped_spring_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::damped_spring_joint_get_param(RID p_joint, PS2DE::DampedSpringParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->damped_spring_joint_get_param(p_joint, p_param);
}

PS2DE::JointType PhysicsServer2D::joint_get_type(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::JointType());
	return data->server->joint_get_type(p_joint);
}

void PhysicsServer2D::free_rid(RID p_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->free_rid(p_rid);
}

void PhysicsServer2D::set_active(bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->set_active(p_active);
}

void PhysicsServer2D::init()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->init();
}

void PhysicsServer2D::step(real_t p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->step(p_step);
}

void PhysicsServer2D::sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->sync();
}

void PhysicsServer2D::flush_queries()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->flush_queries();
}

void PhysicsServer2D::end_sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->end_sync();
}

void PhysicsServer2D::finish()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->finish();
}

bool PhysicsServer2D::is_flushing_queries()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->is_flushing_queries();
}

int PhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->get_process_info(p_info);
}

#else // !MODULE_GODOT_PHYSICS_2D_ENABLED

RID PhysicsServer2D::world_boundary_shape_create() { return RID(); }
RID PhysicsServer2D::separation_ray_shape_create() { return RID(); }
RID PhysicsServer2D::segment_shape_create() { return RID(); }
RID PhysicsServer2D::circle_shape_create() { return RID(); }
RID PhysicsServer2D::rectangle_shape_create() { return RID(); }
RID PhysicsServer2D::capsule_shape_create() { return RID(); }
RID PhysicsServer2D::convex_polygon_shape_create() { return RID(); }
RID PhysicsServer2D::concave_polygon_shape_create() { return RID(); }
void PhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias) {}
PS2DE::ShapeType PhysicsServer2D::shape_get_type(RID p_shape) { return PS2DE::SHAPE_CUSTOM; }
real_t PhysicsServer2D::shape_get_custom_solver_bias(RID p_shape) { return 0; }
bool PhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A, const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B, const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count) { r_result_count = 0; return false; }
RID PhysicsServer2D::space_create() { return RID(); }
void PhysicsServer2D::space_set_active(RID p_space, bool p_active) {}
bool PhysicsServer2D::space_is_active(RID p_space) { return false; }
void PhysicsServer2D::space_set_param(RID p_space, PS2DE::SpaceParameter p_param, real_t p_value) {}
real_t PhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param) { return 0; }
PhysicsDirectSpaceState2D* PhysicsServer2D::space_get_direct_state(RID p_space) { return nullptr; }
void PhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts) {}
Vector<Vector2> PhysicsServer2D::space_get_contacts(RID p_space) { return Vector<Vector2>(); }
int PhysicsServer2D::space_get_contact_count(RID p_space) { return 0; }
RID PhysicsServer2D::area_create() { return RID(); }
void PhysicsServer2D::area_set_space(RID p_area, RID p_space) {}
RID PhysicsServer2D::area_get_space(RID p_area) { return RID(); }
void PhysicsServer2D::area_add_shape(RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled) {}
void PhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape) {}
void PhysicsServer2D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform2D& p_transform) {}
int PhysicsServer2D::area_get_shape_count(RID p_area) { return 0; }
RID PhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx) { return RID(); }
Transform2D PhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx) { return Transform2D(); }
void PhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx) {}
void PhysicsServer2D::area_clear_shapes(RID p_area) {}
void PhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled) {}
void PhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform) {}
Transform2D PhysicsServer2D::area_get_transform(RID p_area) { return Transform2D(); }
void PhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer) {}
uint32_t PhysicsServer2D::area_get_collision_layer(RID p_area) { return 0; }
void PhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask) {}
uint32_t PhysicsServer2D::area_get_collision_mask(RID p_area) { return 0; }
void PhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable) {}
void PhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable) {}
RID PhysicsServer2D::body_create() { return RID(); }
void PhysicsServer2D::body_set_space(RID p_body, RID p_space) {}
RID PhysicsServer2D::body_get_space(RID p_body) { return RID(); }
void PhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode) {}
PS2DE::BodyMode PhysicsServer2D::body_get_mode(RID p_body) { return PS2DE::BODY_MODE_STATIC; }
void PhysicsServer2D::body_add_shape(RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled) {}
void PhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape) {}
void PhysicsServer2D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform2D& p_transform) {}
int PhysicsServer2D::body_get_shape_count(RID p_body) { return 0; }
RID PhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx) { return RID(); }
Transform2D PhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx) { return Transform2D(); }
void PhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape, bool p_disabled) {}
void PhysicsServer2D::body_set_shape_as_one_way_collision(RID p_body, int p_shape, bool p_enabled, real_t p_margin, const Vector2& p_direction) {}
void PhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx) {}
void PhysicsServer2D::body_clear_shapes(RID p_body) {}
void PhysicsServer2D::body_set_continuous_collision_detection_mode(RID p_body, PS2DE::CCDMode p_mode) {}
PS2DE::CCDMode PhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body) { return PS2DE::CCD_MODE_DISABLED; }
void PhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer) {}
uint32_t PhysicsServer2D::body_get_collision_layer(RID p_body) { return 0; }
void PhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask) {}
uint32_t PhysicsServer2D::body_get_collision_mask(RID p_body) { return 0; }
void PhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority) {}
real_t PhysicsServer2D::body_get_collision_priority(RID p_body) { return 0; }
void PhysicsServer2D::body_reset_mass_properties(RID p_body) {}
void PhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse) {}
void PhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque) {}
void PhysicsServer2D::body_apply_impulse(RID p_body, const Vector2& p_impulse, const Vector2& p_position) {}
void PhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force) {}
void PhysicsServer2D::body_apply_force(RID p_body, const Vector2& p_force, const Vector2& p_position) {}
void PhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque) {}
void PhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force) {}
void PhysicsServer2D::body_add_constant_force(RID p_body, const Vector2& p_force, const Vector2& p_position) {}
void PhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque) {}
void PhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force) {}
Vector2 PhysicsServer2D::body_get_constant_force(RID p_body) { return Vector2(); }
void PhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque) {}
real_t PhysicsServer2D::body_get_constant_torque(RID p_body) { return 0; }
void PhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity) {}
void PhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}
void PhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts) {}
int PhysicsServer2D::body_get_max_contacts_reported(RID p_body) { return 0; }
void PhysicsServer2D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold) {}
real_t PhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body) { return 0; }
void PhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit) {}
bool PhysicsServer2D::body_is_omitting_force_integration(RID p_body) { return false; }
bool PhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape, const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max, int& r_result_count) { r_result_count = 0; return false; }
void PhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable) {}
PhysicsDirectBodyState2D* PhysicsServer2D::body_get_direct_state(RID p_body) { return nullptr; }
bool PhysicsServer2D::body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result) { return false; }
RID PhysicsServer2D::joint_create() { return RID(); }
void PhysicsServer2D::joint_clear(RID p_joint) {}
void PhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value) {}
real_t PhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param) { return 0; }
void PhysicsServer2D::joint_disable_collisions_between_bodies(RID p_joint, const bool p_disable) {}
bool PhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint) { return false; }
void PhysicsServer2D::joint_make_pin(RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b) {}
void PhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1, const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b) {}
void PhysicsServer2D::joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b) {}
void PhysicsServer2D::pin_joint_set_param(RID p_joint, PS2DE::PinJointParam p_param, real_t p_value) {}
real_t PhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) { return 0; }
void PhysicsServer2D::pin_joint_set_flag(RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled) {}
bool PhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) { return false; }
void PhysicsServer2D::damped_spring_joint_set_param(RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value) {}
real_t PhysicsServer2D::damped_spring_joint_get_param(RID p_joint, PS2DE::DampedSpringParam p_param) { return 0; }
PS2DE::JointType PhysicsServer2D::joint_get_type(RID p_joint) { return PS2DE::JOINT_TYPE_PIN; }
void PhysicsServer2D::free_rid(RID p_rid) {}
void PhysicsServer2D::set_active(bool p_active) {}
void PhysicsServer2D::init() {}
void PhysicsServer2D::step(real_t p_step) {}
void PhysicsServer2D::sync() {}
void PhysicsServer2D::flush_queries() {}
void PhysicsServer2D::end_sync() {}
void PhysicsServer2D::finish() {}
bool PhysicsServer2D::is_flushing_queries() { return false; }
int PhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info) { return 0; }

#endif // MODULE_GODOT_PHYSICS_2D_ENABLED

// ============================================================================
// Merged from servers/physics_2d/direct_states/physics_direct_body_state_2d.cpp
// ============================================================================

#include "servers/physics_2d/direct_states/physics_direct_body_state_2d.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_GODOT_PHYSICS_2D_ENABLED

PhysicsDirectBodyState2D::PhysicsDirectBodyState2D(GodotBody2D* p_body) : body(p_body) {}

Vector2 PhysicsDirectBodyState2D::get_total_gravity() const { return Vector2(); }
real_t PhysicsDirectBodyState2D::get_total_linear_damp() const { return 0.0; }
real_t PhysicsDirectBodyState2D::get_total_angular_damp() const { return 0.0; }

Vector2 PhysicsDirectBodyState2D::get_center_of_mass() const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_center_of_mass_local() const { return Vector2(); }
real_t PhysicsDirectBodyState2D::get_inverse_mass() const { return 0.0; }
real_t PhysicsDirectBodyState2D::get_inverse_inertia() const { return 0.0; }

void PhysicsDirectBodyState2D::set_linear_velocity(const Vector2& p_velocity) {}
Vector2 PhysicsDirectBodyState2D::get_linear_velocity() const { return Vector2(); }

void PhysicsDirectBodyState2D::set_angular_velocity(real_t p_velocity) {}
real_t PhysicsDirectBodyState2D::get_angular_velocity() const { return 0.0; }

void PhysicsDirectBodyState2D::set_transform(const Transform2D& p_transform) {}
Transform2D PhysicsDirectBodyState2D::get_transform() const { return Transform2D(); }

Vector2 PhysicsDirectBodyState2D::get_velocity_at_local_position(const Vector2& p_position) const { return Vector2(); }

void PhysicsDirectBodyState2D::apply_central_impulse(const Vector2& p_impulse) {}
void PhysicsDirectBodyState2D::apply_torque_impulse(real_t p_torque) {}
void PhysicsDirectBodyState2D::apply_impulse(const Vector2& p_impulse, const Vector2& p_position) {}

void PhysicsDirectBodyState2D::apply_central_force(const Vector2& p_force) {}
void PhysicsDirectBodyState2D::apply_force(const Vector2& p_force, const Vector2& p_position) {}
void PhysicsDirectBodyState2D::apply_torque(real_t p_torque) {}

void PhysicsDirectBodyState2D::add_constant_central_force(const Vector2& p_force) {}
void PhysicsDirectBodyState2D::add_constant_force(const Vector2& p_force, const Vector2& p_position) {}
void PhysicsDirectBodyState2D::add_constant_torque(real_t p_torque) {}

void PhysicsDirectBodyState2D::set_constant_force(const Vector2& p_force) {}
Vector2 PhysicsDirectBodyState2D::get_constant_force() const { return Vector2(); }

void PhysicsDirectBodyState2D::set_constant_torque(real_t p_torque) {}
real_t PhysicsDirectBodyState2D::get_constant_torque() const { return 0.0; }

void PhysicsDirectBodyState2D::set_sleep_state(bool p_enable) {}
bool PhysicsDirectBodyState2D::is_sleeping() const { return false; }

void PhysicsDirectBodyState2D::set_collision_layer(uint32_t p_layer) {}
uint32_t PhysicsDirectBodyState2D::get_collision_layer() const { return 0; }

void PhysicsDirectBodyState2D::set_collision_mask(uint32_t p_mask) {}
uint32_t PhysicsDirectBodyState2D::get_collision_mask() const { return 0; }

int PhysicsDirectBodyState2D::get_contact_count() const { return 0; }

Vector2 PhysicsDirectBodyState2D::get_contact_local_position(int p_contact_idx) const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_contact_local_normal(int p_contact_idx) const { return Vector2(); }
int PhysicsDirectBodyState2D::get_contact_local_shape(int p_contact_idx) const { return 0; }
Vector2 PhysicsDirectBodyState2D::get_contact_local_velocity_at_position(int p_contact_idx) const { return Vector2(); }

RID PhysicsDirectBodyState2D::get_contact_collider(int p_contact_idx) const { return RID(); }
Vector2 PhysicsDirectBodyState2D::get_contact_collider_position(int p_contact_idx) const { return Vector2(); }
int PhysicsDirectBodyState2D::get_contact_collider_shape(int p_contact_idx) const { return 0; }
Vector2 PhysicsDirectBodyState2D::get_contact_collider_velocity_at_position(int p_contact_idx) const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_contact_impulse(int p_contact_idx) const { return Vector2(); }

real_t PhysicsDirectBodyState2D::get_step() const { return 0.0; }

void PhysicsDirectBodyState2D::integrate_forces() {}

PhysicsDirectSpaceState2D* PhysicsDirectBodyState2D::get_space_state() { return nullptr; }

#endif // !MODULE_GODOT_PHYSICS_2D_ENABLED

// ============================================================================
// Merged from servers/physics_2d/direct_states/physics_direct_space_state_2d.cpp
// ============================================================================

#include "servers/physics_2d/direct_states/physics_direct_space_state_2d.h"

#include "modules/modules_enabled.gen.h"

Vector<real_t> PhysicsDirectSpaceState2D::_cast_motion(
	PhysicsShapeQueryParameters2D* rp_shape_query)
{
	real_t closest_safe = 1.0f, closest_unsafe = 1.0f;
	bool res = cast_motion(rp_shape_query->get_parameters(), closest_safe, closest_unsafe);
	if (!res) {
		return Vector<real_t>();
	}
	Vector<real_t> ret;
	ret.resize(2);
	ret.write[0] = closest_safe;
	ret.write[1] = closest_unsafe;
	return ret;
}

#ifndef MODULE_GODOT_PHYSICS_2D_ENABLED

PhysicsDirectSpaceState2D::PhysicsDirectSpaceState2D(GodotSpace2D* p_space) : space(p_space) {}

bool PhysicsDirectSpaceState2D::intersect_ray(
	const PS2DT::RayParameters& p_parameters, PS2DT::RayResult& r_result)
{
	return false;
}

int PhysicsDirectSpaceState2D::intersect_point(
	const PS2DT::PointParameters& p_parameters,
	PS2DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

int PhysicsDirectSpaceState2D::intersect_shape(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState2D::cast_motion(
	const PS2DT::ShapeParameters& p_parameters, real_t& p_closest_safe,
	real_t& p_closest_unsafe)
{
	return false;
}

bool PhysicsDirectSpaceState2D::collide_shape(
	const PS2DT::ShapeParameters& p_parameters, Vector2* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState2D::rest_info(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeRestInfo* r_info)
{
	return false;
}

#endif // !MODULE_GODOT_PHYSICS_2D_ENABLED

// ============================================================================
// Merged from servers/physics_2d/queries/physics_ray_query_parameters_2d.cpp
// ============================================================================

#include "servers/physics_2d/queries/physics_ray_query_parameters_2d.h"

Ref<PhysicsRayQueryParameters2D> PhysicsRayQueryParameters2D::create(
	Vector2 p_from, Vector2 p_to, uint32_t p_mask, const TypedArray<RID>& p_exclude)
{
	Ref<PhysicsRayQueryParameters2D> params;
	params.instantiate();
	params->set_from(p_from);
	params->set_to(p_to);
	params->set_collision_mask(p_mask);
	params->set_exclude(p_exclude);
	return params;
}



void PhysicsRayQueryParameters2D::set_exclude(TypedArray<RID> const&) {}

// ============================================================================
// Merged from servers/physics_2d/queries/physics_shape_query_parameters_2d.cpp
// ============================================================================

#include "core/io/resource.h"
#include "servers/physics_2d/queries/physics_shape_query_parameters_2d.h"

Ref<Resource> PhysicsShapeQueryParameters2D::get_shape() const { return shape_ref; }

void PhysicsShapeQueryParameters2D::set_shape(const Ref<Resource>& p_shape_ref)
{
	ERR_FAIL_COND(p_shape_ref.is_null());
	shape_ref = p_shape_ref;
	parameters.shape_rid = p_shape_ref->get_rid();
}

void PhysicsShapeQueryParameters2D::set_shape_rid(const RID& p_shape)
{
	if (parameters.shape_rid != p_shape) {
		shape_ref = Ref<Resource>();
		parameters.shape_rid = p_shape;
	}
}



// ============================================================================
// Merged from servers/physics_2d/queries/physics_testmotion_query_result_2d.cpp
// ============================================================================

#include "servers/physics_2d/queries/physics_testmotion_query_result_2d.h"

Vector2 PhysicsTestMotionResult2D::get_travel() const { return result.travel; }

Vector2 PhysicsTestMotionResult2D::get_remainder() const { return result.remainder; }

Vector2 PhysicsTestMotionResult2D::get_collision_point() const { return result.collision_point; }

Vector2 PhysicsTestMotionResult2D::get_collision_normal() const { return result.collision_normal; }

Vector2 PhysicsTestMotionResult2D::get_collider_velocity() const
{
	return result.collider_velocity;
}

RID PhysicsTestMotionResult2D::get_collider_rid() const { return result.collider; }

int PhysicsTestMotionResult2D::get_collider_shape() const { return result.collider_shape; }

int PhysicsTestMotionResult2D::get_collision_local_shape() const
{
	return result.collision_local_shape;
}

real_t PhysicsTestMotionResult2D::get_collision_depth() const { return result.collision_depth; }

real_t PhysicsTestMotionResult2D::get_collision_safe_fraction() const
{
	return result.collision_safe_fraction;
}

real_t PhysicsTestMotionResult2D::get_collision_unsafe_fraction() const
{
	return result.collision_unsafe_fraction;
}


