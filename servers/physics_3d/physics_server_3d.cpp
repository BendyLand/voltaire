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

#include "physics_server_3d.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_JOLT_PHYSICS_ENABLED
#include "modules/jolt_physics/jolt_globals.h"
#include "modules/jolt_physics/jolt_physics_server_3d.h"
#endif

void PhysicsServer3D::initialize(bool p_on_separate_thread)
{
#ifdef MODULE_JOLT_PHYSICS_ENABLED
	if (is_initialized()) {
		return;
	}
	jolt_initialize();
	if (!data) {
		data = memnew(Data);
	}
	data->server = memnew(JoltPhysicsServer3D(p_on_separate_thread));
	data->server->init();
#endif
}

void PhysicsServer3D::finalize()
{
	if (!data) {
		return;
	}
#ifdef MODULE_JOLT_PHYSICS_ENABLED
	if (data->server) {
		data->server->finish();
		memdelete(data->server);
		data->server = nullptr;
	}
	jolt_deinitialize();
#endif
	memdelete(data);
	data = nullptr;
}

#ifdef MODULE_JOLT_PHYSICS_ENABLED

void PhysicsServer3D::soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_update_rendering_server(p_body, rp_rendering_server_handler);
}

RID PhysicsServer3D::shape_create(PS3DE::ShapeType p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->shape_create(p_shape);
}

RID PhysicsServer3D::world_boundary_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->world_boundary_shape_create();
}

RID PhysicsServer3D::separation_ray_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->separation_ray_shape_create();
}

RID PhysicsServer3D::sphere_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->sphere_shape_create();
}

RID PhysicsServer3D::box_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->box_shape_create();
}

RID PhysicsServer3D::capsule_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->capsule_shape_create();
}

RID PhysicsServer3D::cylinder_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->cylinder_shape_create();
}

RID PhysicsServer3D::convex_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->convex_polygon_shape_create();
}

RID PhysicsServer3D::concave_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->concave_polygon_shape_create();
}

RID PhysicsServer3D::heightmap_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->heightmap_shape_create();
}

RID PhysicsServer3D::custom_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->custom_shape_create();
}

void PhysicsServer3D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->shape_set_custom_solver_bias(p_shape, p_bias);
}

PS3DE::ShapeType PhysicsServer3D::shape_get_type(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::ShapeType());
	return data->server->shape_get_type(p_shape);
}

void PhysicsServer3D::shape_set_margin(RID p_shape, real_t p_margin)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->shape_set_margin(p_shape, p_margin);
}

real_t PhysicsServer3D::shape_get_margin(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->shape_get_margin(p_shape);
}

real_t PhysicsServer3D::shape_get_custom_solver_bias(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->shape_get_custom_solver_bias(p_shape);
}

RID PhysicsServer3D::space_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->space_create();
}

void PhysicsServer3D::space_set_active(RID p_space, bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_active(p_space, p_active);
}

bool PhysicsServer3D::space_is_active(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->space_is_active(p_space);
}

void PhysicsServer3D::space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_param(p_space, p_param, p_value);
}

real_t PhysicsServer3D::space_get_param(RID p_space, PS3DE::SpaceParameter p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_param(p_space, p_param);
}

PhysicsDirectSpaceState3D* PhysicsServer3D::space_get_direct_state(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->space_get_direct_state(p_space);
}

void PhysicsServer3D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_debug_contacts(p_space, p_max_contacts);
}

Vector<Vector3> PhysicsServer3D::space_get_contacts(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<Vector3>());
	return data->server->space_get_contacts(p_space);
}

int PhysicsServer3D::space_get_contact_count(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_contact_count(p_space);
}

RID PhysicsServer3D::area_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_create();
}

void PhysicsServer3D::area_set_space(RID p_area, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_space(p_area, p_space);
}

RID PhysicsServer3D::area_get_space(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_space(p_area);
}

void PhysicsServer3D::area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_add_shape(p_area, p_shape, p_transform, p_disabled);
}

void PhysicsServer3D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape(p_area, p_shape_idx, p_shape);
}

void PhysicsServer3D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_transform(p_area, p_shape_idx, p_transform);
}

int PhysicsServer3D::area_get_shape_count(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_shape_count(p_area);
}

RID PhysicsServer3D::area_get_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_shape(p_area, p_shape_idx);
}

Transform3D PhysicsServer3D::area_get_shape_transform(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->server->area_get_shape_transform(p_area, p_shape_idx);
}

void PhysicsServer3D::area_remove_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_remove_shape(p_area, p_shape_idx);
}

void PhysicsServer3D::area_clear_shapes(RID p_area)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_clear_shapes(p_area);
}

void PhysicsServer3D::area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_disabled(p_area, p_shape_idx, p_disabled);
}

void PhysicsServer3D::area_set_transform(RID p_area, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_transform(p_area, p_transform);
}

Transform3D PhysicsServer3D::area_get_transform(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->server->area_get_transform(p_area);
}

void PhysicsServer3D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_layer(p_area, p_layer);
}

uint32_t PhysicsServer3D::area_get_collision_layer(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_layer(p_area);
}

void PhysicsServer3D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_mask(p_area, p_mask);
}

uint32_t PhysicsServer3D::area_get_collision_mask(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_mask(p_area);
}

void PhysicsServer3D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_monitorable(p_area, p_monitorable);
}

void PhysicsServer3D::area_set_ray_pickable(RID p_area, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_ray_pickable(p_area, p_enable);
}

RID PhysicsServer3D::body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_create();
}

void PhysicsServer3D::body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_space(p_body, p_space);
}

RID PhysicsServer3D::body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_space(p_body);
}

void PhysicsServer3D::body_set_mode(RID p_body, PS3DE::BodyMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_mode(p_body, p_mode);
}

PS3DE::BodyMode PhysicsServer3D::body_get_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::BodyMode());
	return data->server->body_get_mode(p_body);
}

void PhysicsServer3D::body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_shape(p_body, p_shape, p_transform, p_disabled);
}

void PhysicsServer3D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape(p_body, p_shape_idx, p_shape);
}

void PhysicsServer3D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_transform(p_body, p_shape_idx, p_transform);
}

int PhysicsServer3D::body_get_shape_count(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_shape_count(p_body);
}

RID PhysicsServer3D::body_get_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_shape(p_body, p_shape_idx);
}

Transform3D PhysicsServer3D::body_get_shape_transform(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->server->body_get_shape_transform(p_body, p_shape_idx);
}

void PhysicsServer3D::body_remove_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_shape(p_body, p_shape_idx);
}

void PhysicsServer3D::body_clear_shapes(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_clear_shapes(p_body);
}

void PhysicsServer3D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_disabled(p_body, p_shape_idx, p_disabled);
}

void PhysicsServer3D::body_set_enable_continuous_collision_detection(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_enable_continuous_collision_detection(p_body, p_enable);
}

bool PhysicsServer3D::body_is_continuous_collision_detection_enabled(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_continuous_collision_detection_enabled(p_body);
}

void PhysicsServer3D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer3D::body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_layer(p_body);
}

void PhysicsServer3D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer3D::body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_mask(p_body);
}

void PhysicsServer3D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_priority(p_body, p_priority);
}

real_t PhysicsServer3D::body_get_collision_priority(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_priority(p_body);
}

void PhysicsServer3D::body_set_user_flags(RID p_body, uint32_t p_flags)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_user_flags(p_body, p_flags);
}

uint32_t PhysicsServer3D::body_get_user_flags(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_user_flags(p_body);
}

void PhysicsServer3D::body_reset_mass_properties(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_reset_mass_properties(p_body);
}

void PhysicsServer3D::body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer3D::body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_impulse(p_body, p_impulse, p_position);
}

void PhysicsServer3D::body_apply_torque_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque_impulse(p_body, p_impulse);
}

void PhysicsServer3D::body_apply_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_force(p_body, p_force);
}

void PhysicsServer3D::body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_force(p_body, p_force, p_position);
}

void PhysicsServer3D::body_apply_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque(p_body, p_torque);
}

void PhysicsServer3D::body_add_constant_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_central_force(p_body, p_force);
}

void PhysicsServer3D::body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_force(p_body, p_force, p_position);
}

void PhysicsServer3D::body_add_constant_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_torque(p_body, p_torque);
}

void PhysicsServer3D::body_set_constant_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_force(p_body, p_force);
}

Vector3 PhysicsServer3D::body_get_constant_force(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->body_get_constant_force(p_body);
}

void PhysicsServer3D::body_set_constant_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_torque(p_body, p_torque);
}

Vector3 PhysicsServer3D::body_get_constant_torque(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->body_get_constant_torque(p_body);
}

void PhysicsServer3D::body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_axis_velocity(p_body, p_axis_velocity);
}

void PhysicsServer3D::body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_axis_lock(p_body, p_axis, p_lock);
}

bool PhysicsServer3D::body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_axis_locked(p_body, p_axis);
}

void PhysicsServer3D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer3D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_max_contacts_reported(p_body, p_contacts);
}

int PhysicsServer3D::body_get_max_contacts_reported(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_max_contacts_reported(p_body);
}

void PhysicsServer3D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_contacts_reported_depth_threshold(p_body, p_threshold);
}

real_t PhysicsServer3D::body_get_contacts_reported_depth_threshold(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_contacts_reported_depth_threshold(p_body);
}

void PhysicsServer3D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_omit_force_integration(p_body, p_omit);
}

bool PhysicsServer3D::body_is_omitting_force_integration(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_omitting_force_integration(p_body);
}

void PhysicsServer3D::body_set_ray_pickable(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_ray_pickable(p_body, p_enable);
}

PhysicsDirectBodyState3D* PhysicsServer3D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->body_get_direct_state(p_body);
}

bool PhysicsServer3D::body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_test_motion(p_body, p_parameters, r_result);
}

RID PhysicsServer3D::soft_body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->soft_body_create();
}

void PhysicsServer3D::soft_body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_space(p_body, p_space);
}

RID PhysicsServer3D::soft_body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->soft_body_get_space(p_body);
}

void PhysicsServer3D::soft_body_set_mesh(RID p_body, RID p_mesh)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_mesh(p_body, p_mesh);
}

AABB PhysicsServer3D::soft_body_get_bounds(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), AABB());
	return data->server->soft_body_get_bounds(p_body);
}

void PhysicsServer3D::soft_body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer3D::soft_body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_collision_layer(p_body);
}

void PhysicsServer3D::soft_body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer3D::soft_body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_collision_mask(p_body);
}

void PhysicsServer3D::soft_body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::soft_body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer3D::soft_body_set_transform(RID p_body, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_transform(p_body, p_transform);
}

void PhysicsServer3D::soft_body_set_ray_pickable(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_ray_pickable(p_body, p_enable);
}

void PhysicsServer3D::soft_body_set_simulation_precision(RID p_body, int p_simulation_precision)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_simulation_precision(p_body, p_simulation_precision);
}

int PhysicsServer3D::soft_body_get_simulation_precision(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_simulation_precision(p_body);
}

void PhysicsServer3D::soft_body_set_total_mass(RID p_body, real_t p_total_mass)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_total_mass(p_body, p_total_mass);
}

real_t PhysicsServer3D::soft_body_get_total_mass(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_total_mass(p_body);
}

void PhysicsServer3D::soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_linear_stiffness(p_body, p_stiffness);
}

real_t PhysicsServer3D::soft_body_get_linear_stiffness(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_linear_stiffness(p_body);
}

void PhysicsServer3D::soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_shrinking_factor(p_body, p_shrinking_factor);
}

real_t PhysicsServer3D::soft_body_get_shrinking_factor(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_shrinking_factor(p_body);
}

void PhysicsServer3D::soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_pressure_coefficient(p_body, p_pressure_coefficient);
}

real_t PhysicsServer3D::soft_body_get_pressure_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_pressure_coefficient(p_body);
}

void PhysicsServer3D::soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_damping_coefficient(p_body, p_damping_coefficient);
}

real_t PhysicsServer3D::soft_body_get_damping_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_damping_coefficient(p_body);
}

void PhysicsServer3D::soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_drag_coefficient(p_body, p_drag_coefficient);
}

real_t PhysicsServer3D::soft_body_get_drag_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_drag_coefficient(p_body);
}

void PhysicsServer3D::soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_move_point(p_body, p_point_index, p_global_position);
}

Vector3 PhysicsServer3D::soft_body_get_point_global_position(RID p_body, int p_point_index)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->soft_body_get_point_global_position(p_body, p_point_index);
}

void PhysicsServer3D::soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_point_impulse(p_body, p_point_index, p_impulse);
}

void PhysicsServer3D::soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_point_force(p_body, p_point_index, p_force);
}

void PhysicsServer3D::soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer3D::soft_body_apply_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_central_force(p_body, p_force);
}

void PhysicsServer3D::soft_body_remove_all_pinned_points(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_remove_all_pinned_points(p_body);
}

void PhysicsServer3D::soft_body_pin_point(RID p_body, int p_point_index, bool p_pin)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_pin_point(p_body, p_point_index, p_pin);
}

bool PhysicsServer3D::soft_body_is_point_pinned(RID p_body, int p_point_index)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->soft_body_is_point_pinned(p_body, p_point_index);
}

RID PhysicsServer3D::joint_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->joint_create();
}

void PhysicsServer3D::joint_clear(RID p_joint)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_clear(p_joint);
}

PS3DE::JointType PhysicsServer3D::joint_get_type(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::JointType());
	return data->server->joint_get_type(p_joint);
}

void PhysicsServer3D::joint_set_solver_priority(RID p_joint, int p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_set_solver_priority(p_joint, p_priority);
}

int PhysicsServer3D::joint_get_solver_priority(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->joint_get_solver_priority(p_joint);
}

void PhysicsServer3D::joint_disable_collisions_between_bodies(RID p_joint, bool p_disable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_disable_collisions_between_bodies(p_joint, p_disable);
}

bool PhysicsServer3D::joint_is_disabled_collisions_between_bodies(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->joint_is_disabled_collisions_between_bodies(p_joint);
}

void PhysicsServer3D::joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_pin(p_joint, p_body_A, p_local_A, p_body_B, p_local_B);
}

void PhysicsServer3D::pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->pin_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::pin_joint_set_local_a(RID p_joint, const Vector3& p_A)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_local_a(p_joint, p_A);
}

Vector3 PhysicsServer3D::pin_joint_get_local_a(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->pin_joint_get_local_a(p_joint);
}

void PhysicsServer3D::pin_joint_set_local_b(RID p_joint, const Vector3& p_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_local_b(p_joint, p_B);
}

Vector3 PhysicsServer3D::pin_joint_get_local_b(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->pin_joint_get_local_b(p_joint);
}

void PhysicsServer3D::joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_hinge(p_joint, p_body_A, p_hinge_A, p_body_B, p_hinge_B);
}

void PhysicsServer3D::joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_hinge_simple(p_joint, p_body_A, p_pivot_A, p_axis_A, p_body_B, p_pivot_B, p_axis_B);
}

void PhysicsServer3D::hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->hinge_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->hinge_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->hinge_joint_set_flag(p_joint, p_flag, p_enabled);
}

bool PhysicsServer3D::hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->hinge_joint_get_flag(p_joint, p_flag);
}

void PhysicsServer3D::joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_slider(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->slider_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->slider_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_cone_twist(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->cone_twist_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->cone_twist_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_generic_6dof(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::generic_6dof_joint_set_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->generic_6dof_joint_set_param(p_joint, p_arg1, p_param, p_value);
}

real_t PhysicsServer3D::generic_6dof_joint_get_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->generic_6dof_joint_get_param(p_joint, p_arg1, p_param);
}

void PhysicsServer3D::generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->generic_6dof_joint_set_flag(p_joint, p_arg1, p_flag, p_enable);
}

bool PhysicsServer3D::generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->generic_6dof_joint_get_flag(p_joint, p_arg1, p_flag);
}

void PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->generic_6dof_joint_set_angular_target_rotation(p_joint, p_target_rotation);
}

Quaternion PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Quaternion());
	return data->server->generic_6dof_joint_get_angular_target_rotation(p_joint);
}

void PhysicsServer3D::free_rid(RID p_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->free_rid(p_rid);
}

void PhysicsServer3D::set_active(bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->set_active(p_active);
}

void PhysicsServer3D::init()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->init();
}

void PhysicsServer3D::step(real_t p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->step(p_step);
}

void PhysicsServer3D::sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->sync();
}

void PhysicsServer3D::flush_queries()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->flush_queries();
}

void PhysicsServer3D::end_sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->end_sync();
}

void PhysicsServer3D::finish()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->finish();
}

bool PhysicsServer3D::is_flushing_queries()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->is_flushing_queries();
}

int PhysicsServer3D::get_process_info(PS3DE::ProcessInfo p_info)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->get_process_info(p_info);
}

#else // !MODULE_JOLT_PHYSICS_ENABLED

void PhysicsServer3D::soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler) {}
RID PhysicsServer3D::shape_create(PS3DE::ShapeType p_shape) { return RID(); }
RID PhysicsServer3D::world_boundary_shape_create() { return RID(); }
RID PhysicsServer3D::separation_ray_shape_create() { return RID(); }
RID PhysicsServer3D::sphere_shape_create() { return RID(); }
RID PhysicsServer3D::box_shape_create() { return RID(); }
RID PhysicsServer3D::capsule_shape_create() { return RID(); }
RID PhysicsServer3D::cylinder_shape_create() { return RID(); }
RID PhysicsServer3D::convex_polygon_shape_create() { return RID(); }
RID PhysicsServer3D::concave_polygon_shape_create() { return RID(); }
RID PhysicsServer3D::heightmap_shape_create() { return RID(); }
RID PhysicsServer3D::custom_shape_create() { return RID(); }
void PhysicsServer3D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias) {}
PS3DE::ShapeType PhysicsServer3D::shape_get_type(RID p_shape) { return PS3DE::SHAPE_CUSTOM; }
void PhysicsServer3D::shape_set_margin(RID p_shape, real_t p_margin) {}
real_t PhysicsServer3D::shape_get_margin(RID p_shape) { return 0; }
real_t PhysicsServer3D::shape_get_custom_solver_bias(RID p_shape) { return 0; }
RID PhysicsServer3D::space_create() { return RID(); }
void PhysicsServer3D::space_set_active(RID p_space, bool p_active) {}
bool PhysicsServer3D::space_is_active(RID p_space) { return false; }
void PhysicsServer3D::space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value) {}
real_t PhysicsServer3D::space_get_param(RID p_space, PS3DE::SpaceParameter p_param) { return 0; }
PhysicsDirectSpaceState3D* PhysicsServer3D::space_get_direct_state(RID p_space) { return nullptr; }
void PhysicsServer3D::space_set_debug_contacts(RID p_space, int p_max_contacts) {}
Vector<Vector3> PhysicsServer3D::space_get_contacts(RID p_space) { return Vector<Vector3>(); }
int PhysicsServer3D::space_get_contact_count(RID p_space) { return 0; }
RID PhysicsServer3D::area_create() { return RID(); }
void PhysicsServer3D::area_set_space(RID p_area, RID p_space) {}
RID PhysicsServer3D::area_get_space(RID p_area) { return RID(); }
void PhysicsServer3D::area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform, bool p_disabled) {}
void PhysicsServer3D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape) {}
void PhysicsServer3D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform) {}
int PhysicsServer3D::area_get_shape_count(RID p_area) { return 0; }
RID PhysicsServer3D::area_get_shape(RID p_area, int p_shape_idx) { return RID(); }
Transform3D PhysicsServer3D::area_get_shape_transform(RID p_area, int p_shape_idx) { return Transform3D(); }
void PhysicsServer3D::area_remove_shape(RID p_area, int p_shape_idx) {}
void PhysicsServer3D::area_clear_shapes(RID p_area) {}
void PhysicsServer3D::area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled) {}
void PhysicsServer3D::area_set_transform(RID p_area, const Transform3D& p_transform) {}
Transform3D PhysicsServer3D::area_get_transform(RID p_area) { return Transform3D(); }
void PhysicsServer3D::area_set_collision_layer(RID p_area, uint32_t p_layer) {}
uint32_t PhysicsServer3D::area_get_collision_layer(RID p_area) { return 0; }
void PhysicsServer3D::area_set_collision_mask(RID p_area, uint32_t p_mask) {}
uint32_t PhysicsServer3D::area_get_collision_mask(RID p_area) { return 0; }
void PhysicsServer3D::area_set_monitorable(RID p_area, bool p_monitorable) {}
void PhysicsServer3D::area_set_ray_pickable(RID p_area, bool p_enable) {}
RID PhysicsServer3D::body_create() { return RID(); }
void PhysicsServer3D::body_set_space(RID p_body, RID p_space) {}
RID PhysicsServer3D::body_get_space(RID p_body) { return RID(); }
void PhysicsServer3D::body_set_mode(RID p_body, PS3DE::BodyMode p_mode) {}
PS3DE::BodyMode PhysicsServer3D::body_get_mode(RID p_body) { return PS3DE::BODY_MODE_STATIC; }
void PhysicsServer3D::body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform, bool p_disabled) {}
void PhysicsServer3D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape) {}
void PhysicsServer3D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform) {}
int PhysicsServer3D::body_get_shape_count(RID p_body) { return 0; }
RID PhysicsServer3D::body_get_shape(RID p_body, int p_shape_idx) { return RID(); }
Transform3D PhysicsServer3D::body_get_shape_transform(RID p_body, int p_shape_idx) { return Transform3D(); }
void PhysicsServer3D::body_remove_shape(RID p_body, int p_shape_idx) {}
void PhysicsServer3D::body_clear_shapes(RID p_body) {}
void PhysicsServer3D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled) {}
void PhysicsServer3D::body_set_enable_continuous_collision_detection(RID p_body, bool p_enable) {}
bool PhysicsServer3D::body_is_continuous_collision_detection_enabled(RID p_body) { return false; }
void PhysicsServer3D::body_set_collision_layer(RID p_body, uint32_t p_layer) {}
uint32_t PhysicsServer3D::body_get_collision_layer(RID p_body) { return 0; }
void PhysicsServer3D::body_set_collision_mask(RID p_body, uint32_t p_mask) {}
uint32_t PhysicsServer3D::body_get_collision_mask(RID p_body) { return 0; }
void PhysicsServer3D::body_set_collision_priority(RID p_body, real_t p_priority) {}
real_t PhysicsServer3D::body_get_collision_priority(RID p_body) { return 0; }
void PhysicsServer3D::body_set_user_flags(RID p_body, uint32_t p_flags) {}
uint32_t PhysicsServer3D::body_get_user_flags(RID p_body) { return 0; }
void PhysicsServer3D::body_reset_mass_properties(RID p_body) {}
void PhysicsServer3D::body_apply_central_impulse(RID p_body, const Vector3& p_impulse) {}
void PhysicsServer3D::body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position) {}
void PhysicsServer3D::body_apply_torque_impulse(RID p_body, const Vector3& p_impulse) {}
void PhysicsServer3D::body_apply_central_force(RID p_body, const Vector3& p_force) {}
void PhysicsServer3D::body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position) {}
void PhysicsServer3D::body_apply_torque(RID p_body, const Vector3& p_torque) {}
void PhysicsServer3D::body_add_constant_central_force(RID p_body, const Vector3& p_force) {}
void PhysicsServer3D::body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position) {}
void PhysicsServer3D::body_add_constant_torque(RID p_body, const Vector3& p_torque) {}
void PhysicsServer3D::body_set_constant_force(RID p_body, const Vector3& p_force) {}
Vector3 PhysicsServer3D::body_get_constant_force(RID p_body) { return Vector3(); }
void PhysicsServer3D::body_set_constant_torque(RID p_body, const Vector3& p_torque) {}
Vector3 PhysicsServer3D::body_get_constant_torque(RID p_body) { return Vector3(); }
void PhysicsServer3D::body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity) {}
void PhysicsServer3D::body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock) {}
bool PhysicsServer3D::body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis) { return false; }
void PhysicsServer3D::body_add_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::body_remove_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}
void PhysicsServer3D::body_set_max_contacts_reported(RID p_body, int p_contacts) {}
int PhysicsServer3D::body_get_max_contacts_reported(RID p_body) { return 0; }
void PhysicsServer3D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold) {}
real_t PhysicsServer3D::body_get_contacts_reported_depth_threshold(RID p_body) { return 0; }
void PhysicsServer3D::body_set_omit_force_integration(RID p_body, bool p_omit) {}
bool PhysicsServer3D::body_is_omitting_force_integration(RID p_body) { return false; }
void PhysicsServer3D::body_set_ray_pickable(RID p_body, bool p_enable) {}
PhysicsDirectBodyState3D* PhysicsServer3D::body_get_direct_state(RID p_body) { return nullptr; }
bool PhysicsServer3D::body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result) { return false; }
RID PhysicsServer3D::soft_body_create() { return RID(); }
void PhysicsServer3D::soft_body_set_space(RID p_body, RID p_space) {}
RID PhysicsServer3D::soft_body_get_space(RID p_body) { return RID(); }
void PhysicsServer3D::soft_body_set_mesh(RID p_body, RID p_mesh) {}
AABB PhysicsServer3D::soft_body_get_bounds(RID p_body) { return AABB(); }
void PhysicsServer3D::soft_body_set_collision_layer(RID p_body, uint32_t p_layer) {}
uint32_t PhysicsServer3D::soft_body_get_collision_layer(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_collision_mask(RID p_body, uint32_t p_mask) {}
uint32_t PhysicsServer3D::soft_body_get_collision_mask(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_add_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::soft_body_remove_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}
void PhysicsServer3D::soft_body_set_transform(RID p_body, const Transform3D& p_transform) {}
void PhysicsServer3D::soft_body_set_ray_pickable(RID p_body, bool p_enable) {}
void PhysicsServer3D::soft_body_set_simulation_precision(RID p_body, int p_simulation_precision) {}
int PhysicsServer3D::soft_body_get_simulation_precision(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_total_mass(RID p_body, real_t p_total_mass) {}
real_t PhysicsServer3D::soft_body_get_total_mass(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness) {}
real_t PhysicsServer3D::soft_body_get_linear_stiffness(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor) {}
real_t PhysicsServer3D::soft_body_get_shrinking_factor(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient) {}
real_t PhysicsServer3D::soft_body_get_pressure_coefficient(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient) {}
real_t PhysicsServer3D::soft_body_get_damping_coefficient(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient) {}
real_t PhysicsServer3D::soft_body_get_drag_coefficient(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position) {}
Vector3 PhysicsServer3D::soft_body_get_point_global_position(RID p_body, int p_point_index) { return Vector3(); }
void PhysicsServer3D::soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse) {}
void PhysicsServer3D::soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force) {}
void PhysicsServer3D::soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse) {}
void PhysicsServer3D::soft_body_apply_central_force(RID p_body, const Vector3& p_force) {}
void PhysicsServer3D::soft_body_remove_all_pinned_points(RID p_body) {}
void PhysicsServer3D::soft_body_pin_point(RID p_body, int p_point_index, bool p_pin) {}
bool PhysicsServer3D::soft_body_is_point_pinned(RID p_body, int p_point_index) { return false; }
RID PhysicsServer3D::joint_create() { return RID(); }
void PhysicsServer3D::joint_clear(RID p_joint) {}
PS3DE::JointType PhysicsServer3D::joint_get_type(RID p_joint) { return PS3DE::JOINT_TYPE_PIN; }
void PhysicsServer3D::joint_set_solver_priority(RID p_joint, int p_priority) {}
int PhysicsServer3D::joint_get_solver_priority(RID p_joint) { return 0; }
void PhysicsServer3D::joint_disable_collisions_between_bodies(RID p_joint, bool p_disable) {}
bool PhysicsServer3D::joint_is_disabled_collisions_between_bodies(RID p_joint) { return false; }
void PhysicsServer3D::joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B) {}
void PhysicsServer3D::pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param) { return 0; }
void PhysicsServer3D::pin_joint_set_local_a(RID p_joint, const Vector3& p_A) {}
Vector3 PhysicsServer3D::pin_joint_get_local_a(RID p_joint) { return Vector3(); }
void PhysicsServer3D::pin_joint_set_local_b(RID p_joint, const Vector3& p_B) {}
Vector3 PhysicsServer3D::pin_joint_get_local_b(RID p_joint) { return Vector3(); }
void PhysicsServer3D::joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B) {}
void PhysicsServer3D::joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B) {}
void PhysicsServer3D::hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param) { return 0; }
void PhysicsServer3D::hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled) {}
bool PhysicsServer3D::hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag) { return false; }
void PhysicsServer3D::joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B) {}
void PhysicsServer3D::slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param) { return 0; }
void PhysicsServer3D::joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B) {}
void PhysicsServer3D::cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param) { return 0; }
void PhysicsServer3D::joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B) {}
void PhysicsServer3D::generic_6dof_joint_set_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param, real_t p_value) {}
real_t PhysicsServer3D::generic_6dof_joint_get_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param) { return 0; }
void PhysicsServer3D::generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable) {}
bool PhysicsServer3D::generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag) { return false; }
void PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation) {}
Quaternion PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(RID p_joint) { return Quaternion(); }
void PhysicsServer3D::free_rid(RID p_rid) {}
void PhysicsServer3D::set_active(bool p_active) {}
void PhysicsServer3D::init() {}
void PhysicsServer3D::step(real_t p_step) {}
void PhysicsServer3D::sync() {}
void PhysicsServer3D::flush_queries() {}
void PhysicsServer3D::end_sync() {}
void PhysicsServer3D::finish() {}
bool PhysicsServer3D::is_flushing_queries() { return false; }
int PhysicsServer3D::get_process_info(PS3DE::ProcessInfo p_info) { return 0; }

#endif // MODULE_JOLT_PHYSICS_ENABLED

// ============================================================================
// Merged from servers/physics_3d/physics_server_3d_rendering_server_handler.cpp
// ============================================================================

#include "physics_server_3d_rendering_server_handler.h"

#include "servers/rendering/renderer.h"

void PhysicsServer3DRenderingServerHandler::prepare(RID p_mesh_rid, int p_surface)
{
	clear();

	ERR_FAIL_COND(!p_mesh_rid.is_valid());

	mesh = p_mesh_rid;
	surface = p_surface;

	RenderingServerTypes::SurfaceData surface_data = Renderer::mesh_get_surface(mesh, surface);

	uint32_t surface_offsets[RSE::ARRAY_MAX];
	uint32_t vertex_stride;
	uint32_t normal_tangent_stride;
	uint32_t attrib_stride;
	uint32_t skin_stride;
	Renderer::mesh_surface_make_offsets_from_format(surface_data.format,
		surface_data.vertex_count, surface_data.index_count, surface_offsets, vertex_stride,
		normal_tangent_stride, attrib_stride, skin_stride);

	buffer = surface_data.vertex_data;
	stride = vertex_stride;
	normal_stride = normal_tangent_stride;
	offset_vertices = surface_offsets[RSE::ARRAY_VERTEX];
	offset_normal = surface_offsets[RSE::ARRAY_NORMAL];
}

void PhysicsServer3DRenderingServerHandler::clear()
{
	buffer.resize(0);
	stride = 0;
	normal_stride = 0;
	offset_vertices = 0;
	offset_normal = 0;

	surface = 0;
	mesh = RID();
}

void PhysicsServer3DRenderingServerHandler::open() { write_buffer = buffer.ptrw(); }

void PhysicsServer3DRenderingServerHandler::close() { write_buffer = nullptr; }

void PhysicsServer3DRenderingServerHandler::commit_changes()
{
	Renderer::mesh_surface_update_vertex_region(mesh, surface, 0, buffer);
}

void PhysicsServer3DRenderingServerHandler::set_vertex(int p_vertex_id, const Vector3& p_vertex)
{
	ERR_FAIL_NULL(write_buffer);
	float* vertex_buffer =
		reinterpret_cast<float*>(write_buffer + p_vertex_id * stride + offset_vertices);
	*vertex_buffer++ = (float)p_vertex.x;
	*vertex_buffer++ = (float)p_vertex.y;
	*vertex_buffer++ = (float)p_vertex.z;
}

void PhysicsServer3DRenderingServerHandler::set_normal(int p_vertex_id, const Vector3& p_normal)
{
	ERR_FAIL_NULL(write_buffer);
	Vector2 res = p_normal.octahedron_encode();
	uint32_t value = 0;
	value |= (uint16_t)CLAMP(res.x * 65535, 0, 65535);
	value |= (uint16_t)CLAMP(res.y * 65535, 0, 65535) << 16;
	memcpy(&write_buffer[p_vertex_id * normal_stride + offset_normal], &value, sizeof(uint32_t));
}

void PhysicsServer3DRenderingServerHandler::set_aabb(const AABB& p_aabb)
{
	Renderer::mesh_set_custom_aabb(mesh, p_aabb);
}

// ============================================================================
// Merged from servers/physics_3d/direct_states/physics_direct_body_state_3d.cpp
// ============================================================================

#include "servers/physics_3d/direct_states/physics_direct_body_state_3d.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_JOLT_PHYSICS_ENABLED

PhysicsDirectBodyState3D::PhysicsDirectBodyState3D(JoltBody3D* p_body) : body(p_body) {}

Vector3 PhysicsDirectBodyState3D::get_total_gravity() const { return Vector3(); }
real_t PhysicsDirectBodyState3D::get_total_angular_damp() const { return 0.0; }
real_t PhysicsDirectBodyState3D::get_total_linear_damp() const { return 0.0; }

Vector3 PhysicsDirectBodyState3D::get_center_of_mass() const { return Vector3(); }
Vector3 PhysicsDirectBodyState3D::get_center_of_mass_local() const { return Vector3(); }
Basis PhysicsDirectBodyState3D::get_principal_inertia_axes() const { return Basis(); }
real_t PhysicsDirectBodyState3D::get_inverse_mass() const { return 0.0; }
Vector3 PhysicsDirectBodyState3D::get_inverse_inertia() const { return Vector3(); }
Basis PhysicsDirectBodyState3D::get_inverse_inertia_tensor() const { return Basis(); }

void PhysicsDirectBodyState3D::set_linear_velocity(const Vector3& p_velocity) {}
Vector3 PhysicsDirectBodyState3D::get_linear_velocity() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_angular_velocity(const Vector3& p_velocity) {}
Vector3 PhysicsDirectBodyState3D::get_angular_velocity() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_transform(const Transform3D& p_transform) {}
Transform3D PhysicsDirectBodyState3D::get_transform() const { return Transform3D(); }

Vector3 PhysicsDirectBodyState3D::get_velocity_at_local_position(const Vector3& p_position) const { return Vector3(); }

void PhysicsDirectBodyState3D::apply_central_impulse(const Vector3& p_impulse) {}
void PhysicsDirectBodyState3D::apply_impulse(const Vector3& p_impulse, const Vector3& p_position) {}
void PhysicsDirectBodyState3D::apply_torque_impulse(const Vector3& p_impulse) {}

void PhysicsDirectBodyState3D::apply_central_force(const Vector3& p_force) {}
void PhysicsDirectBodyState3D::apply_force(const Vector3& p_force, const Vector3& p_position) {}
void PhysicsDirectBodyState3D::apply_torque(const Vector3& p_torque) {}

void PhysicsDirectBodyState3D::add_constant_central_force(const Vector3& p_force) {}
void PhysicsDirectBodyState3D::add_constant_force(const Vector3& p_force, const Vector3& p_position) {}
void PhysicsDirectBodyState3D::add_constant_torque(const Vector3& p_torque) {}

void PhysicsDirectBodyState3D::set_constant_force(const Vector3& p_force) {}
Vector3 PhysicsDirectBodyState3D::get_constant_force() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_constant_torque(const Vector3& p_torque) {}
Vector3 PhysicsDirectBodyState3D::get_constant_torque() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_sleep_state(bool p_sleep) {}
bool PhysicsDirectBodyState3D::is_sleeping() const { return false; }

void PhysicsDirectBodyState3D::set_collision_layer(uint32_t p_layer) {}
uint32_t PhysicsDirectBodyState3D::get_collision_layer() const { return 0; }

void PhysicsDirectBodyState3D::set_collision_mask(uint32_t p_mask) {}
uint32_t PhysicsDirectBodyState3D::get_collision_mask() const { return 0; }

int PhysicsDirectBodyState3D::get_contact_count() const { return 0; }

Vector3 PhysicsDirectBodyState3D::get_contact_local_position(int p_contact_idx) const { return Vector3(); }
Vector3 PhysicsDirectBodyState3D::get_contact_local_normal(int p_contact_idx) const { return Vector3(); }
Vector3 PhysicsDirectBodyState3D::get_contact_impulse(int p_contact_idx) const { return Vector3(); }
int PhysicsDirectBodyState3D::get_contact_local_shape(int p_contact_idx) const { return 0; }
Vector3 PhysicsDirectBodyState3D::get_contact_local_velocity_at_position(int p_contact_idx) const { return Vector3(); }

RID PhysicsDirectBodyState3D::get_contact_collider(int p_contact_idx) const { return RID(); }
Vector3 PhysicsDirectBodyState3D::get_contact_collider_position(int p_contact_idx) const { return Vector3(); }
int PhysicsDirectBodyState3D::get_contact_collider_shape(int p_contact_idx) const { return 0; }
Vector3 PhysicsDirectBodyState3D::get_contact_collider_velocity_at_position(int p_contact_idx) const { return Vector3(); }

real_t PhysicsDirectBodyState3D::get_step() const { return 0.0; }

void PhysicsDirectBodyState3D::integrate_forces() {}

PhysicsDirectSpaceState3D* PhysicsDirectBodyState3D::get_space_state() { return nullptr; }

#endif // !MODULE_JOLT_PHYSICS_ENABLED

// ============================================================================
// Merged from servers/physics_3d/direct_states/physics_direct_space_state_3d.cpp
// ============================================================================

#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"

#include "modules/modules_enabled.gen.h"

Vector<real_t> PhysicsDirectSpaceState3D::_cast_motion(
	PhysicsShapeQueryParameters3D* rp_shape_query)
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

#ifndef MODULE_JOLT_PHYSICS_ENABLED

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D(JoltSpace3D* p_space) : space(p_space) {}

bool PhysicsDirectSpaceState3D::intersect_ray(
	const PS3DT::RayParameters& p_parameters, PS3DT::RayResult& r_result)
{
	return false;
}

int PhysicsDirectSpaceState3D::intersect_point(
	const PS3DT::PointParameters& p_point_params,
	PS3DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

int PhysicsDirectSpaceState3D::intersect_shape(
	const PS3DT::ShapeParameters& p_shape_params,
	PS3DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState3D::cast_motion(
	const PS3DT::ShapeParameters& p_shape_params, real_t& r_closest_safe,
	real_t& r_closest_unsafe, PS3DT::ShapeRestInfo* r_info)
{
	return false;
}

bool PhysicsDirectSpaceState3D::collide_shape(
	const PS3DT::ShapeParameters& p_shape_params, Vector3* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState3D::rest_info(
	const PS3DT::ShapeParameters& p_shape_params,
	PS3DT::ShapeRestInfo* r_info)
{
	return false;
}

Vector3 PhysicsDirectSpaceState3D::get_closest_point_to_object_volume(
	RID p_object, Vector3 p_point) const
{
	return Vector3();
}

bool PhysicsDirectSpaceState3D::body_test_motion(const JoltBody3D& p_body,
	const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result) const
{
	return false;
}

#endif // !MODULE_JOLT_PHYSICS_ENABLED

// ============================================================================
// Merged from servers/physics_3d/queries/physics_shape_query_parameters_3d.cpp
// ============================================================================

#include "core/io/resource.h"
#include "servers/physics_3d/queries/physics_shape_query_parameters_3d.h"

Ref<Resource> PhysicsShapeQueryParameters3D::get_shape() const { return shape_ref; }

void PhysicsShapeQueryParameters3D::set_shape(const Ref<Resource>& p_shape_ref)
{
	ERR_FAIL_COND(p_shape_ref.is_null());
	shape_ref = p_shape_ref;
	parameters.shape_rid = p_shape_ref->get_rid();
}

void PhysicsShapeQueryParameters3D::set_shape_rid(const RID& p_shape)
{
	if (parameters.shape_rid != p_shape) {
		shape_ref = Ref<Resource>();
		parameters.shape_rid = p_shape;
	}
}



// ============================================================================
// Merged from servers/physics_3d/queries/physics_testmotion_query_result_3d.cpp
// ============================================================================

#include "servers/physics_3d/queries/physics_testmotion_query_result_3d.h"

Vector3 PhysicsTestMotionResult3D::get_travel() const { return result.travel; }

Vector3 PhysicsTestMotionResult3D::get_remainder() const { return result.remainder; }

real_t PhysicsTestMotionResult3D::get_collision_safe_fraction() const
{
	return result.collision_safe_fraction;
}

real_t PhysicsTestMotionResult3D::get_collision_unsafe_fraction() const
{
	return result.collision_unsafe_fraction;
}

int PhysicsTestMotionResult3D::get_collision_count() const { return result.collision_count; }

Vector3 PhysicsTestMotionResult3D::get_collision_point(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].position;
}

Vector3 PhysicsTestMotionResult3D::get_collision_normal(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].normal;
}

Vector3 PhysicsTestMotionResult3D::get_collider_velocity(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].collider_velocity;
}

RID PhysicsTestMotionResult3D::get_collider_rid(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, RID());
	return result.collisions[p_collision_index].collider;
}

int PhysicsTestMotionResult3D::get_collider_shape(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].collider_shape;
}

int PhysicsTestMotionResult3D::get_collision_local_shape(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].local_shape;
}

real_t PhysicsTestMotionResult3D::get_collision_depth(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0.0);
	return result.collisions[p_collision_index].depth;
}


