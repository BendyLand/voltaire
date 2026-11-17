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

#include "core/config/project_settings.h"
#include "physics_server_2d.compat.inc"
#include "physics_server_2d.h"

bool PhysicsServer2DDriver::_body_test_motion(RID p_body, PhysicsTestMotionParameters2D* rp_parameters,
	const Ref<PhysicsTestMotionResult2D>& p_result)
{
	PS2DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

void PhysicsServer2D::initialize(PhysicsServer2DDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void PhysicsServer2D::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

RID PhysicsServer2D::world_boundary_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->world_boundary_shape_create();
}

RID PhysicsServer2D::separation_ray_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->separation_ray_shape_create();
}

RID PhysicsServer2D::segment_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->segment_shape_create();
}

RID PhysicsServer2D::circle_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->circle_shape_create();
}

RID PhysicsServer2D::rectangle_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->rectangle_shape_create();
}

RID PhysicsServer2D::capsule_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->capsule_shape_create();
}

RID PhysicsServer2D::convex_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->convex_polygon_shape_create();
}

RID PhysicsServer2D::concave_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->concave_polygon_shape_create();
}

void PhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->shape_set_custom_solver_bias(p_shape, p_bias);
}

PS2DE::ShapeType PhysicsServer2D::shape_get_type(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::ShapeType());
	return data->driver->shape_get_type(p_shape);
}

real_t PhysicsServer2D::shape_get_custom_solver_bias(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->shape_get_custom_solver_bias(p_shape);
}

bool PhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A, const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B, const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->shape_collide(p_shape_A, p_xform_A, p_motion_A, p_shape_B, p_xform_B, p_motion_B, r_results, p_result_max, r_result_count);
}

RID PhysicsServer2D::space_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->space_create();
}

void PhysicsServer2D::space_set_active(RID p_space, bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->space_set_active(p_space, p_active);
}

bool PhysicsServer2D::space_is_active(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->space_is_active(p_space);
}

void PhysicsServer2D::space_set_param(RID p_space, PS2DE::SpaceParameter p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->space_set_param(p_space, p_param, p_value);
}

real_t PhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->space_get_param(p_space, p_param);
}

PhysicsDirectSpaceState2D* PhysicsServer2D::space_get_direct_state(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->driver->space_get_direct_state(p_space);
}

void PhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->space_set_debug_contacts(p_space, p_max_contacts);
}

Vector<Vector2> PhysicsServer2D::space_get_contacts(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<Vector2>());
	return data->driver->space_get_contacts(p_space);
}

int PhysicsServer2D::space_get_contact_count(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->space_get_contact_count(p_space);
}

RID PhysicsServer2D::area_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->area_create();
}

void PhysicsServer2D::area_set_space(RID p_area, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_space(p_area, p_space);
}

RID PhysicsServer2D::area_get_space(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->area_get_space(p_area);
}

void PhysicsServer2D::area_add_shape(RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_add_shape(p_area, p_shape, p_transform, p_disabled);
}

void PhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_shape(p_area, p_shape_idx, p_shape);
}

void PhysicsServer2D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_shape_transform(p_area, p_shape_idx, p_transform);
}

int PhysicsServer2D::area_get_shape_count(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->area_get_shape_count(p_area);
}

RID PhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->area_get_shape(p_area, p_shape_idx);
}

Transform2D PhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->driver->area_get_shape_transform(p_area, p_shape_idx);
}

void PhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_remove_shape(p_area, p_shape_idx);
}

void PhysicsServer2D::area_clear_shapes(RID p_area)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_clear_shapes(p_area);
}

void PhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_shape_disabled(p_area, p_shape, p_disabled);
}

void PhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_transform(p_area, p_transform);
}

Transform2D PhysicsServer2D::area_get_transform(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->driver->area_get_transform(p_area);
}

void PhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_collision_layer(p_area, p_layer);
}

uint32_t PhysicsServer2D::area_get_collision_layer(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->area_get_collision_layer(p_area);
}

void PhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_collision_mask(p_area, p_mask);
}

uint32_t PhysicsServer2D::area_get_collision_mask(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->area_get_collision_mask(p_area);
}

void PhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_monitorable(p_area, p_monitorable);
}

void PhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_pickable(p_area, p_pickable);
}

RID PhysicsServer2D::body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->body_create();
}

void PhysicsServer2D::body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_space(p_body, p_space);
}

RID PhysicsServer2D::body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->body_get_space(p_body);
}

void PhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_mode(p_body, p_mode);
}

PS2DE::BodyMode PhysicsServer2D::body_get_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::BodyMode());
	return data->driver->body_get_mode(p_body);
}

void PhysicsServer2D::body_add_shape(RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_shape(p_body, p_shape, p_transform, p_disabled);
}

void PhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape(p_body, p_shape_idx, p_shape);
}

void PhysicsServer2D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape_transform(p_body, p_shape_idx, p_transform);
}

int PhysicsServer2D::body_get_shape_count(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_shape_count(p_body);
}

RID PhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->body_get_shape(p_body, p_shape_idx);
}

Transform2D PhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->driver->body_get_shape_transform(p_body, p_shape_idx);
}

void PhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape_disabled(p_body, p_shape, p_disabled);
}

void PhysicsServer2D::body_set_shape_as_one_way_collision(RID p_body, int p_shape, bool p_enabled, real_t p_margin, const Vector2& p_direction)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape_as_one_way_collision(p_body, p_shape, p_enabled, p_margin, p_direction);
}

void PhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_remove_shape(p_body, p_shape_idx);
}

void PhysicsServer2D::body_clear_shapes(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_clear_shapes(p_body);
}

void PhysicsServer2D::body_set_continuous_collision_detection_mode(RID p_body, PS2DE::CCDMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_continuous_collision_detection_mode(p_body, p_mode);
}

PS2DE::CCDMode PhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::CCDMode());
	return data->driver->body_get_continuous_collision_detection_mode(p_body);
}

void PhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer2D::body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_collision_layer(p_body);
}

void PhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer2D::body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_collision_mask(p_body);
}

void PhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_collision_priority(p_body, p_priority);
}

real_t PhysicsServer2D::body_get_collision_priority(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_collision_priority(p_body);
}

void PhysicsServer2D::body_reset_mass_properties(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_reset_mass_properties(p_body);
}

void PhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_torque_impulse(p_body, p_torque);
}

void PhysicsServer2D::body_apply_impulse(RID p_body, const Vector2& p_impulse, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_impulse(p_body, p_impulse, p_position);
}

void PhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_central_force(p_body, p_force);
}

void PhysicsServer2D::body_apply_force(RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_force(p_body, p_force, p_position);
}

void PhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_torque(p_body, p_torque);
}

void PhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_constant_central_force(p_body, p_force);
}

void PhysicsServer2D::body_add_constant_force(RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_constant_force(p_body, p_force, p_position);
}

void PhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_constant_torque(p_body, p_torque);
}

void PhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_constant_force(p_body, p_force);
}

Vector2 PhysicsServer2D::body_get_constant_force(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector2());
	return data->driver->body_get_constant_force(p_body);
}

void PhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_constant_torque(p_body, p_torque);
}

real_t PhysicsServer2D::body_get_constant_torque(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_constant_torque(p_body);
}

void PhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_axis_velocity(p_body, p_axis_velocity);
}

void PhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_max_contacts_reported(p_body, p_contacts);
}

int PhysicsServer2D::body_get_max_contacts_reported(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_max_contacts_reported(p_body);
}

void PhysicsServer2D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_contacts_reported_depth_threshold(p_body, p_threshold);
}

real_t PhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_contacts_reported_depth_threshold(p_body);
}

void PhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_omit_force_integration(p_body, p_omit);
}

bool PhysicsServer2D::body_is_omitting_force_integration(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_is_omitting_force_integration(p_body);
}

bool PhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape, const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max, int& r_result_count)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_collide_shape(p_body, p_body_shape, p_shape, p_shape_xform, p_motion, r_results, p_result_max, r_result_count);
}

void PhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_pickable(p_body, p_pickable);
}

PhysicsDirectBodyState2D* PhysicsServer2D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->driver->body_get_direct_state(p_body);
}

bool PhysicsServer2D::body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_test_motion(p_body, p_parameters, r_result);
}

RID PhysicsServer2D::joint_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->joint_create();
}

void PhysicsServer2D::joint_clear(RID p_joint)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_clear(p_joint);
}

void PhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->joint_get_param(p_joint, p_param);
}

void PhysicsServer2D::joint_disable_collisions_between_bodies(RID p_joint, const bool p_disable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_disable_collisions_between_bodies(p_joint, p_disable);
}

bool PhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->joint_is_disabled_collisions_between_bodies(p_joint);
}

void PhysicsServer2D::joint_make_pin(RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_pin(p_joint, p_anchor, p_body_a, p_body_b);
}

void PhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1, const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_groove(p_joint, p_a_groove1, p_a_groove2, p_b_anchor, p_body_a, p_body_b);
}

void PhysicsServer2D::joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_damped_spring(p_joint, p_anchor_a, p_anchor_b, p_body_a, p_body_b);
}

void PhysicsServer2D::pin_joint_set_param(RID p_joint, PS2DE::PinJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pin_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->pin_joint_get_param(p_joint, p_param);
}

void PhysicsServer2D::pin_joint_set_flag(RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pin_joint_set_flag(p_joint, p_flag, p_enabled);
}

bool PhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->pin_joint_get_flag(p_joint, p_flag);
}

void PhysicsServer2D::damped_spring_joint_set_param(RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->damped_spring_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::damped_spring_joint_get_param(RID p_joint, PS2DE::DampedSpringParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->damped_spring_joint_get_param(p_joint, p_param);
}

PS2DE::JointType PhysicsServer2D::joint_get_type(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::JointType());
	return data->driver->joint_get_type(p_joint);
}

void PhysicsServer2D::free_rid(RID p_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->free_rid(p_rid);
}

void PhysicsServer2D::set_active(bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_active(p_active);
}

void PhysicsServer2D::init()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->init();
}

void PhysicsServer2D::step(real_t p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->step(p_step);
}

void PhysicsServer2D::sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->sync();
}

void PhysicsServer2D::flush_queries()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->flush_queries();
}

void PhysicsServer2D::end_sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->end_sync();
}

void PhysicsServer2D::finish()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->finish();
}

bool PhysicsServer2D::is_flushing_queries()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_flushing_queries();
}

int PhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_process_info(p_info);
}

const String PhysicsServer2DManager::setting_property_name(PNAME("physics/2d/physics_engine"));

void PhysicsServer2DManager::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void PhysicsServer2DManager::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void PhysicsServer2DManager::set_default_server(const String& p_name, int p_priority)
{
	ERR_FAIL_NULL(data);
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (data->default_server_priority < p_priority) {
		data->default_server_id = id;
		data->default_server_priority = p_priority;
	}
}

int PhysicsServer2DManager::find_server_id(const String& p_name) { return -1; }

int PhysicsServer2DManager::get_servers_count() { return 0; }

String PhysicsServer2DManager::get_server_name(int p_id) { return ""; }

PhysicsServer2DDriver* PhysicsServer2DManager::new_default_server() { return nullptr; }

PhysicsServer2DDriver* PhysicsServer2DManager::new_server(const String& p_name) { return nullptr; }
