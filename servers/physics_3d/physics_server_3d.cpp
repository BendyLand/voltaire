/**************************************************************************/
/*  physics_server_3d.cpp                                                 */
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
#include "physics_server_3d.h"

bool PhysicsServer3DDriver::_body_test_motion(RID p_body, PhysicsTestMotionParameters3D* rp_parameters,
	const Ref<PhysicsTestMotionResult3D>& p_result)
{
	PS3DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

RID PhysicsServer3DDriver::shape_create(PS3DE::ShapeType p_shape)
{
	switch (p_shape) {
	case PS3DE::SHAPE_WORLD_BOUNDARY:
		return world_boundary_shape_create();
	case PS3DE::SHAPE_SEPARATION_RAY:
		return separation_ray_shape_create();
	case PS3DE::SHAPE_SPHERE:
		return sphere_shape_create();
	case PS3DE::SHAPE_BOX:
		return box_shape_create();
	case PS3DE::SHAPE_CAPSULE:
		return capsule_shape_create();
	case PS3DE::SHAPE_CYLINDER:
		return cylinder_shape_create();
	case PS3DE::SHAPE_CONVEX_POLYGON:
		return convex_polygon_shape_create();
	case PS3DE::SHAPE_CONCAVE_POLYGON:
		return concave_polygon_shape_create();
	case PS3DE::SHAPE_HEIGHTMAP:
		return heightmap_shape_create();
	case PS3DE::SHAPE_CUSTOM:
		return custom_shape_create();
	default:
		return RID();
	}
}

void PhysicsServer3D::initialize(PhysicsServer3DDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void PhysicsServer3D::finalize()
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

void PhysicsServer3D::soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_update_rendering_server(p_body, rp_rendering_server_handler);
}

RID PhysicsServer3D::shape_create(PS3DE::ShapeType p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->shape_create(p_shape);
}

RID PhysicsServer3D::world_boundary_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->world_boundary_shape_create();
}

RID PhysicsServer3D::separation_ray_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->separation_ray_shape_create();
}

RID PhysicsServer3D::sphere_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->sphere_shape_create();
}

RID PhysicsServer3D::box_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->box_shape_create();
}

RID PhysicsServer3D::capsule_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->capsule_shape_create();
}

RID PhysicsServer3D::cylinder_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->cylinder_shape_create();
}

RID PhysicsServer3D::convex_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->convex_polygon_shape_create();
}

RID PhysicsServer3D::concave_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->concave_polygon_shape_create();
}

RID PhysicsServer3D::heightmap_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->heightmap_shape_create();
}

RID PhysicsServer3D::custom_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->custom_shape_create();
}

void PhysicsServer3D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->shape_set_custom_solver_bias(p_shape, p_bias);
}

PS3DE::ShapeType PhysicsServer3D::shape_get_type(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::ShapeType());
	return data->driver->shape_get_type(p_shape);
}

void PhysicsServer3D::shape_set_margin(RID p_shape, real_t p_margin)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->shape_set_margin(p_shape, p_margin);
}

real_t PhysicsServer3D::shape_get_margin(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->shape_get_margin(p_shape);
}

real_t PhysicsServer3D::shape_get_custom_solver_bias(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->shape_get_custom_solver_bias(p_shape);
}

RID PhysicsServer3D::space_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->space_create();
}

void PhysicsServer3D::space_set_active(RID p_space, bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->space_set_active(p_space, p_active);
}

bool PhysicsServer3D::space_is_active(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->space_is_active(p_space);
}

void PhysicsServer3D::space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->space_set_param(p_space, p_param, p_value);
}

real_t PhysicsServer3D::space_get_param(RID p_space, PS3DE::SpaceParameter p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->space_get_param(p_space, p_param);
}

PhysicsDirectSpaceState3D* PhysicsServer3D::space_get_direct_state(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->driver->space_get_direct_state(p_space);
}

void PhysicsServer3D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->space_set_debug_contacts(p_space, p_max_contacts);
}

Vector<Vector3> PhysicsServer3D::space_get_contacts(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<Vector3>());
	return data->driver->space_get_contacts(p_space);
}

int PhysicsServer3D::space_get_contact_count(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->space_get_contact_count(p_space);
}

RID PhysicsServer3D::area_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->area_create();
}

void PhysicsServer3D::area_set_space(RID p_area, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_space(p_area, p_space);
}

RID PhysicsServer3D::area_get_space(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->area_get_space(p_area);
}

void PhysicsServer3D::area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_add_shape(p_area, p_shape, p_transform, p_disabled);
}

void PhysicsServer3D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_shape(p_area, p_shape_idx, p_shape);
}

void PhysicsServer3D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_shape_transform(p_area, p_shape_idx, p_transform);
}

int PhysicsServer3D::area_get_shape_count(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->area_get_shape_count(p_area);
}

RID PhysicsServer3D::area_get_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->area_get_shape(p_area, p_shape_idx);
}

Transform3D PhysicsServer3D::area_get_shape_transform(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->driver->area_get_shape_transform(p_area, p_shape_idx);
}

void PhysicsServer3D::area_remove_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_remove_shape(p_area, p_shape_idx);
}

void PhysicsServer3D::area_clear_shapes(RID p_area)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_clear_shapes(p_area);
}

void PhysicsServer3D::area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_shape_disabled(p_area, p_shape_idx, p_disabled);
}

void PhysicsServer3D::area_set_transform(RID p_area, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_transform(p_area, p_transform);
}

Transform3D PhysicsServer3D::area_get_transform(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->driver->area_get_transform(p_area);
}

void PhysicsServer3D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_collision_layer(p_area, p_layer);
}

uint32_t PhysicsServer3D::area_get_collision_layer(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->area_get_collision_layer(p_area);
}

void PhysicsServer3D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_collision_mask(p_area, p_mask);
}

uint32_t PhysicsServer3D::area_get_collision_mask(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->area_get_collision_mask(p_area);
}

void PhysicsServer3D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_monitorable(p_area, p_monitorable);
}

void PhysicsServer3D::area_set_ray_pickable(RID p_area, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->area_set_ray_pickable(p_area, p_enable);
}

RID PhysicsServer3D::body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->body_create();
}

void PhysicsServer3D::body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_space(p_body, p_space);
}

RID PhysicsServer3D::body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->body_get_space(p_body);
}

void PhysicsServer3D::body_set_mode(RID p_body, PS3DE::BodyMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_mode(p_body, p_mode);
}

PS3DE::BodyMode PhysicsServer3D::body_get_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::BodyMode());
	return data->driver->body_get_mode(p_body);
}

void PhysicsServer3D::body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_shape(p_body, p_shape, p_transform, p_disabled);
}

void PhysicsServer3D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape(p_body, p_shape_idx, p_shape);
}

void PhysicsServer3D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape_transform(p_body, p_shape_idx, p_transform);
}

int PhysicsServer3D::body_get_shape_count(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_shape_count(p_body);
}

RID PhysicsServer3D::body_get_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->body_get_shape(p_body, p_shape_idx);
}

Transform3D PhysicsServer3D::body_get_shape_transform(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->driver->body_get_shape_transform(p_body, p_shape_idx);
}

void PhysicsServer3D::body_remove_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_remove_shape(p_body, p_shape_idx);
}

void PhysicsServer3D::body_clear_shapes(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_clear_shapes(p_body);
}

void PhysicsServer3D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_shape_disabled(p_body, p_shape_idx, p_disabled);
}

void PhysicsServer3D::body_set_enable_continuous_collision_detection(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_enable_continuous_collision_detection(p_body, p_enable);
}

bool PhysicsServer3D::body_is_continuous_collision_detection_enabled(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_is_continuous_collision_detection_enabled(p_body);
}

void PhysicsServer3D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer3D::body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_collision_layer(p_body);
}

void PhysicsServer3D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer3D::body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_collision_mask(p_body);
}

void PhysicsServer3D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_collision_priority(p_body, p_priority);
}

real_t PhysicsServer3D::body_get_collision_priority(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_collision_priority(p_body);
}

void PhysicsServer3D::body_set_user_flags(RID p_body, uint32_t p_flags)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_user_flags(p_body, p_flags);
}

uint32_t PhysicsServer3D::body_get_user_flags(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_user_flags(p_body);
}

void PhysicsServer3D::body_reset_mass_properties(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_reset_mass_properties(p_body);
}

void PhysicsServer3D::body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer3D::body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_impulse(p_body, p_impulse, p_position);
}

void PhysicsServer3D::body_apply_torque_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_torque_impulse(p_body, p_impulse);
}

void PhysicsServer3D::body_apply_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_central_force(p_body, p_force);
}

void PhysicsServer3D::body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_force(p_body, p_force, p_position);
}

void PhysicsServer3D::body_apply_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_apply_torque(p_body, p_torque);
}

void PhysicsServer3D::body_add_constant_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_constant_central_force(p_body, p_force);
}

void PhysicsServer3D::body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_constant_force(p_body, p_force, p_position);
}

void PhysicsServer3D::body_add_constant_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_constant_torque(p_body, p_torque);
}

void PhysicsServer3D::body_set_constant_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_constant_force(p_body, p_force);
}

Vector3 PhysicsServer3D::body_get_constant_force(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->driver->body_get_constant_force(p_body);
}

void PhysicsServer3D::body_set_constant_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_constant_torque(p_body, p_torque);
}

Vector3 PhysicsServer3D::body_get_constant_torque(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->driver->body_get_constant_torque(p_body);
}

void PhysicsServer3D::body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_axis_velocity(p_body, p_axis_velocity);
}

void PhysicsServer3D::body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_axis_lock(p_body, p_axis, p_lock);
}

bool PhysicsServer3D::body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_is_axis_locked(p_body, p_axis);
}

void PhysicsServer3D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer3D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_max_contacts_reported(p_body, p_contacts);
}

int PhysicsServer3D::body_get_max_contacts_reported(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_max_contacts_reported(p_body);
}

void PhysicsServer3D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_contacts_reported_depth_threshold(p_body, p_threshold);
}

real_t PhysicsServer3D::body_get_contacts_reported_depth_threshold(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->body_get_contacts_reported_depth_threshold(p_body);
}

void PhysicsServer3D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_omit_force_integration(p_body, p_omit);
}

bool PhysicsServer3D::body_is_omitting_force_integration(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_is_omitting_force_integration(p_body);
}

void PhysicsServer3D::body_set_ray_pickable(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->body_set_ray_pickable(p_body, p_enable);
}

PhysicsDirectBodyState3D* PhysicsServer3D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->driver->body_get_direct_state(p_body);
}

bool PhysicsServer3D::body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->body_test_motion(p_body, p_parameters, r_result);
}

RID PhysicsServer3D::soft_body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->soft_body_create();
}

void PhysicsServer3D::soft_body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_space(p_body, p_space);
}

RID PhysicsServer3D::soft_body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->soft_body_get_space(p_body);
}

void PhysicsServer3D::soft_body_set_mesh(RID p_body, RID p_mesh)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_mesh(p_body, p_mesh);
}

AABB PhysicsServer3D::soft_body_get_bounds(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), AABB());
	return data->driver->soft_body_get_bounds(p_body);
}

void PhysicsServer3D::soft_body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer3D::soft_body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_collision_layer(p_body);
}

void PhysicsServer3D::soft_body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer3D::soft_body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_collision_mask(p_body);
}

void PhysicsServer3D::soft_body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::soft_body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer3D::soft_body_set_transform(RID p_body, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_transform(p_body, p_transform);
}

void PhysicsServer3D::soft_body_set_ray_pickable(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_ray_pickable(p_body, p_enable);
}

void PhysicsServer3D::soft_body_set_simulation_precision(RID p_body, int p_simulation_precision)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_simulation_precision(p_body, p_simulation_precision);
}

int PhysicsServer3D::soft_body_get_simulation_precision(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_simulation_precision(p_body);
}

void PhysicsServer3D::soft_body_set_total_mass(RID p_body, real_t p_total_mass)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_total_mass(p_body, p_total_mass);
}

real_t PhysicsServer3D::soft_body_get_total_mass(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_total_mass(p_body);
}

void PhysicsServer3D::soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_linear_stiffness(p_body, p_stiffness);
}

real_t PhysicsServer3D::soft_body_get_linear_stiffness(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_linear_stiffness(p_body);
}

void PhysicsServer3D::soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_shrinking_factor(p_body, p_shrinking_factor);
}

real_t PhysicsServer3D::soft_body_get_shrinking_factor(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_shrinking_factor(p_body);
}

void PhysicsServer3D::soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_pressure_coefficient(p_body, p_pressure_coefficient);
}

real_t PhysicsServer3D::soft_body_get_pressure_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_pressure_coefficient(p_body);
}

void PhysicsServer3D::soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_damping_coefficient(p_body, p_damping_coefficient);
}

real_t PhysicsServer3D::soft_body_get_damping_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_damping_coefficient(p_body);
}

void PhysicsServer3D::soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_set_drag_coefficient(p_body, p_drag_coefficient);
}

real_t PhysicsServer3D::soft_body_get_drag_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->soft_body_get_drag_coefficient(p_body);
}

void PhysicsServer3D::soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_move_point(p_body, p_point_index, p_global_position);
}

Vector3 PhysicsServer3D::soft_body_get_point_global_position(RID p_body, int p_point_index)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->driver->soft_body_get_point_global_position(p_body, p_point_index);
}

void PhysicsServer3D::soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_apply_point_impulse(p_body, p_point_index, p_impulse);
}

void PhysicsServer3D::soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_apply_point_force(p_body, p_point_index, p_force);
}

void PhysicsServer3D::soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer3D::soft_body_apply_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_apply_central_force(p_body, p_force);
}

void PhysicsServer3D::soft_body_remove_all_pinned_points(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_remove_all_pinned_points(p_body);
}

void PhysicsServer3D::soft_body_pin_point(RID p_body, int p_point_index, bool p_pin)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->soft_body_pin_point(p_body, p_point_index, p_pin);
}

bool PhysicsServer3D::soft_body_is_point_pinned(RID p_body, int p_point_index)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->soft_body_is_point_pinned(p_body, p_point_index);
}

RID PhysicsServer3D::joint_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->joint_create();
}

void PhysicsServer3D::joint_clear(RID p_joint)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_clear(p_joint);
}

PS3DE::JointType PhysicsServer3D::joint_get_type(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::JointType());
	return data->driver->joint_get_type(p_joint);
}

void PhysicsServer3D::joint_set_solver_priority(RID p_joint, int p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_set_solver_priority(p_joint, p_priority);
}

int PhysicsServer3D::joint_get_solver_priority(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->joint_get_solver_priority(p_joint);
}

void PhysicsServer3D::joint_disable_collisions_between_bodies(RID p_joint, bool p_disable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_disable_collisions_between_bodies(p_joint, p_disable);
}

bool PhysicsServer3D::joint_is_disabled_collisions_between_bodies(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->joint_is_disabled_collisions_between_bodies(p_joint);
}

void PhysicsServer3D::joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_pin(p_joint, p_body_A, p_local_A, p_body_B, p_local_B);
}

void PhysicsServer3D::pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pin_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->pin_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::pin_joint_set_local_a(RID p_joint, const Vector3& p_A)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pin_joint_set_local_a(p_joint, p_A);
}

Vector3 PhysicsServer3D::pin_joint_get_local_a(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->driver->pin_joint_get_local_a(p_joint);
}

void PhysicsServer3D::pin_joint_set_local_b(RID p_joint, const Vector3& p_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pin_joint_set_local_b(p_joint, p_B);
}

Vector3 PhysicsServer3D::pin_joint_get_local_b(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->driver->pin_joint_get_local_b(p_joint);
}

void PhysicsServer3D::joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_hinge(p_joint, p_body_A, p_hinge_A, p_body_B, p_hinge_B);
}

void PhysicsServer3D::joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_hinge_simple(p_joint, p_body_A, p_pivot_A, p_axis_A, p_body_B, p_pivot_B, p_axis_B);
}

void PhysicsServer3D::hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->hinge_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->hinge_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->hinge_joint_set_flag(p_joint, p_flag, p_enabled);
}

bool PhysicsServer3D::hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->hinge_joint_get_flag(p_joint, p_flag);
}

void PhysicsServer3D::joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_slider(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->slider_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->slider_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_cone_twist(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->cone_twist_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->cone_twist_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->joint_make_generic_6dof(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::generic_6dof_joint_set_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->generic_6dof_joint_set_param(p_joint, p_arg1, p_param, p_value);
}

real_t PhysicsServer3D::generic_6dof_joint_get_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->generic_6dof_joint_get_param(p_joint, p_arg1, p_param);
}

void PhysicsServer3D::generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->generic_6dof_joint_set_flag(p_joint, p_arg1, p_flag, p_enable);
}

bool PhysicsServer3D::generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->generic_6dof_joint_get_flag(p_joint, p_arg1, p_flag);
}

void PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->generic_6dof_joint_set_angular_target_rotation(p_joint, p_target_rotation);
}

Quaternion PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Quaternion());
	return data->driver->generic_6dof_joint_get_angular_target_rotation(p_joint);
}

void PhysicsServer3D::free_rid(RID p_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->free_rid(p_rid);
}

void PhysicsServer3D::set_active(bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_active(p_active);
}

void PhysicsServer3D::init()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->init();
}

void PhysicsServer3D::step(real_t p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->step(p_step);
}

void PhysicsServer3D::sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->sync();
}

void PhysicsServer3D::flush_queries()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->flush_queries();
}

void PhysicsServer3D::end_sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->end_sync();
}

void PhysicsServer3D::finish()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->finish();
}

bool PhysicsServer3D::is_flushing_queries()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_flushing_queries();
}

int PhysicsServer3D::get_process_info(PS3DE::ProcessInfo p_info)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_process_info(p_info);
}

const String PhysicsServer3DManager::setting_property_name(PNAME("physics/3d/physics_engine"));

void PhysicsServer3DManager::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void PhysicsServer3DManager::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void PhysicsServer3DManager::set_default_server(const String& p_name, int p_priority)
{
	ERR_FAIL_NULL(data);
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (data->default_server_priority < p_priority) {
		data->default_server_id = id;
		data->default_server_priority = p_priority;
	}
}

int PhysicsServer3DManager::find_server_id(const String& p_name)
{
	ERR_FAIL_NULL_V(data, -1);
	for (int i = data->physics_servers.size() - 1; 0 <= i; --i) {
		if (p_name == data->physics_servers[i].name) {
			return i;
		}
	}
	return -1;
}

int PhysicsServer3DManager::get_servers_count()
{
	return data ? data->physics_servers.size() : 0;
}

String PhysicsServer3DManager::get_server_name(int p_id)
{
	ERR_FAIL_NULL_V(data, "");
	ERR_FAIL_INDEX_V(p_id, get_servers_count(), "");
	return data->physics_servers[p_id].name;
}

PhysicsServer3DDriver* PhysicsServer3DManager::new_default_server() { return nullptr; }

PhysicsServer3DDriver* PhysicsServer3DManager::new_server(const String& p_name) { return nullptr; }
