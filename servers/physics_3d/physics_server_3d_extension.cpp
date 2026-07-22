/**************************************************************************/
/*  physics_server_3d_extension.cpp                                       */
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

#include "physics_server_3d_extension.h"

#include "core/object/class_db.h"

bool PhysicsDirectSpaceState3DExtension::is_body_excluded_from_query(const RID &p_body) const {
	return exclude && exclude->has(p_body);
}

thread_local const HashSet<RID> *PhysicsDirectSpaceState3DExtension::exclude = nullptr;

void PhysicsDirectSpaceState3DExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("is_body_excluded_from_query", "body"), &PhysicsDirectSpaceState3DExtension::is_body_excluded_from_query);

	VLTRVIRTUAL_BIND(_intersect_ray, "from", "to", "collision_mask", "collide_with_bodies", "collide_with_areas", "hit_from_inside", "hit_back_faces", "pick_ray", "r_result");
	VLTRVIRTUAL_BIND(_intersect_point, "position", "collision_mask", "collide_with_bodies", "collide_with_areas", "r_results", "max_results");
	VLTRVIRTUAL_BIND(_intersect_shape, "shape_rid", "transform", "motion", "margin", "collision_mask", "collide_with_bodies", "collide_with_areas", "r_result_count", "max_results");
	VLTRVIRTUAL_BIND(_cast_motion, "shape_rid", "transform", "motion", "margin", "collision_mask", "collide_with_bodies", "collide_with_areas", "r_closest_safe", "r_closest_unsafe", "r_info");
	VLTRVIRTUAL_BIND(_collide_shape, "shape_rid", "transform", "motion", "margin", "collision_mask", "collide_with_bodies", "collide_with_areas", "r_results", "max_results", "r_result_count");
	VLTRVIRTUAL_BIND(_rest_info, "shape_rid", "transform", "motion", "margin", "collision_mask", "collide_with_bodies", "collide_with_areas", "r_rest_info");
	VLTRVIRTUAL_BIND(_get_closest_point_to_object_volume, "object", "point");
}

PhysicsDirectSpaceState3DExtension::PhysicsDirectSpaceState3DExtension() {
}

void PhysicsDirectBodyState3DExtension::_bind_methods() {
	VLTRVIRTUAL_BIND(_get_total_gravity);
	VLTRVIRTUAL_BIND(_get_total_linear_damp);
	VLTRVIRTUAL_BIND(_get_total_angular_damp);

	VLTRVIRTUAL_BIND(_get_center_of_mass);
	VLTRVIRTUAL_BIND(_get_center_of_mass_local);
	VLTRVIRTUAL_BIND(_get_principal_inertia_axes);

	VLTRVIRTUAL_BIND(_get_inverse_mass);
	VLTRVIRTUAL_BIND(_get_inverse_inertia);
	VLTRVIRTUAL_BIND(_get_inverse_inertia_tensor);

	VLTRVIRTUAL_BIND(_set_linear_velocity, "velocity");
	VLTRVIRTUAL_BIND(_get_linear_velocity);

	VLTRVIRTUAL_BIND(_set_angular_velocity, "velocity");
	VLTRVIRTUAL_BIND(_get_angular_velocity);

	VLTRVIRTUAL_BIND(_set_transform, "transform");
	VLTRVIRTUAL_BIND(_get_transform);

	VLTRVIRTUAL_BIND(_get_velocity_at_local_position, "local_position");

	VLTRVIRTUAL_BIND(_apply_central_impulse, "impulse");
	VLTRVIRTUAL_BIND(_apply_impulse, "impulse", "position");
	VLTRVIRTUAL_BIND(_apply_torque_impulse, "impulse");

	VLTRVIRTUAL_BIND(_apply_central_force, "force");
	VLTRVIRTUAL_BIND(_apply_force, "force", "position");
	VLTRVIRTUAL_BIND(_apply_torque, "torque");

	VLTRVIRTUAL_BIND(_add_constant_central_force, "force");
	VLTRVIRTUAL_BIND(_add_constant_force, "force", "position");
	VLTRVIRTUAL_BIND(_add_constant_torque, "torque");

	VLTRVIRTUAL_BIND(_set_constant_force, "force");
	VLTRVIRTUAL_BIND(_get_constant_force);

	VLTRVIRTUAL_BIND(_set_constant_torque, "torque");
	VLTRVIRTUAL_BIND(_get_constant_torque);

	VLTRVIRTUAL_BIND(_set_sleep_state, "enabled");
	VLTRVIRTUAL_BIND(_is_sleeping);

	VLTRVIRTUAL_BIND(_set_collision_layer, "layer");
	VLTRVIRTUAL_BIND(_get_collision_layer);

	VLTRVIRTUAL_BIND(_set_collision_mask, "mask");
	VLTRVIRTUAL_BIND(_get_collision_mask);

	VLTRVIRTUAL_BIND(_get_contact_count);

	VLTRVIRTUAL_BIND(_get_contact_local_position, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_local_normal, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_impulse, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_local_shape, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_local_velocity_at_position, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_collider, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_collider_position, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_collider_id, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_collider_object, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_collider_shape, "contact_idx");
	VLTRVIRTUAL_BIND(_get_contact_collider_velocity_at_position, "contact_idx");
	VLTRVIRTUAL_BIND(_get_step);
	VLTRVIRTUAL_BIND(_integrate_forces);
	VLTRVIRTUAL_BIND(_get_space_state);
}

PhysicsDirectBodyState3DExtension::PhysicsDirectBodyState3DExtension() {
}

thread_local const HashSet<RID> *PhysicsServer3DExtension::exclude_bodies = nullptr;
thread_local const HashSet<ObjectID> *PhysicsServer3DExtension::exclude_objects = nullptr;

bool PhysicsServer3DExtension::body_test_motion_is_excluding_body(RID p_body) const {
	return exclude_bodies && exclude_bodies->has(p_body);
}

bool PhysicsServer3DExtension::body_test_motion_is_excluding_object(ObjectID p_object) const {
	return exclude_objects && exclude_objects->has(p_object);
}

void PhysicsServer3DExtension::_bind_methods() {
	/* SHAPE API */

	VLTRVIRTUAL_BIND(_world_boundary_shape_create);
	VLTRVIRTUAL_BIND(_separation_ray_shape_create);
	VLTRVIRTUAL_BIND(_sphere_shape_create);
	VLTRVIRTUAL_BIND(_box_shape_create);
	VLTRVIRTUAL_BIND(_capsule_shape_create);
	VLTRVIRTUAL_BIND(_cylinder_shape_create);
	VLTRVIRTUAL_BIND(_convex_polygon_shape_create);
	VLTRVIRTUAL_BIND(_concave_polygon_shape_create);
	VLTRVIRTUAL_BIND(_heightmap_shape_create);
	VLTRVIRTUAL_BIND(_custom_shape_create);

	VLTRVIRTUAL_BIND(_shape_set_data, "shape", "data");
	VLTRVIRTUAL_BIND(_shape_set_custom_solver_bias, "shape", "bias");

	VLTRVIRTUAL_BIND(_shape_set_margin, "shape", "margin");
	VLTRVIRTUAL_BIND(_shape_get_margin, "shape");

	VLTRVIRTUAL_BIND(_shape_get_type, "shape");
	VLTRVIRTUAL_BIND(_shape_get_data, "shape");
	VLTRVIRTUAL_BIND(_shape_get_custom_solver_bias, "shape");

	/* SPACE API */

	VLTRVIRTUAL_BIND(_space_create);
	VLTRVIRTUAL_BIND(_space_set_active, "space", "active");
	VLTRVIRTUAL_BIND(_space_is_active, "space");

	VLTRVIRTUAL_BIND(_space_set_param, "space", "param", "value");
	VLTRVIRTUAL_BIND(_space_get_param, "space", "param");

	VLTRVIRTUAL_BIND(_space_get_direct_state, "space");

	VLTRVIRTUAL_BIND(_space_set_debug_contacts, "space", "max_contacts");
	VLTRVIRTUAL_BIND(_space_get_contacts, "space");
	VLTRVIRTUAL_BIND(_space_get_contact_count, "space");

	/* AREA API */

	VLTRVIRTUAL_BIND(_area_create);

	VLTRVIRTUAL_BIND(_area_set_space, "area", "space");
	VLTRVIRTUAL_BIND(_area_get_space, "area");

	VLTRVIRTUAL_BIND(_area_add_shape, "area", "shape", "transform", "disabled");
	VLTRVIRTUAL_BIND(_area_set_shape, "area", "shape_idx", "shape");
	VLTRVIRTUAL_BIND(_area_set_shape_transform, "area", "shape_idx", "transform");
	VLTRVIRTUAL_BIND(_area_set_shape_disabled, "area", "shape_idx", "disabled");

	VLTRVIRTUAL_BIND(_area_get_shape_count, "area");
	VLTRVIRTUAL_BIND(_area_get_shape, "area", "shape_idx");
	VLTRVIRTUAL_BIND(_area_get_shape_transform, "area", "shape_idx");

	VLTRVIRTUAL_BIND(_area_remove_shape, "area", "shape_idx");
	VLTRVIRTUAL_BIND(_area_clear_shapes, "area");

	VLTRVIRTUAL_BIND(_area_attach_object_instance_id, "area", "id");
	VLTRVIRTUAL_BIND(_area_get_object_instance_id, "area");

	VLTRVIRTUAL_BIND(_area_set_param, "area", "param", "value");
	VLTRVIRTUAL_BIND(_area_set_transform, "area", "transform");

	VLTRVIRTUAL_BIND(_area_get_param, "area", "param");
	VLTRVIRTUAL_BIND(_area_get_transform, "area");

	VLTRVIRTUAL_BIND(_area_set_collision_layer, "area", "layer");
	VLTRVIRTUAL_BIND(_area_get_collision_layer, "area");

	VLTRVIRTUAL_BIND(_area_set_collision_mask, "area", "mask");
	VLTRVIRTUAL_BIND(_area_get_collision_mask, "area");

	VLTRVIRTUAL_BIND(_area_set_monitorable, "area", "monitorable");
	VLTRVIRTUAL_BIND(_area_set_ray_pickable, "area", "enable");

	VLTRVIRTUAL_BIND(_area_set_monitor_callback, "area", "callback");
	VLTRVIRTUAL_BIND(_area_set_area_monitor_callback, "area", "callback");

	/* BODY API */

	ClassDB::bind_method(D_METHOD("body_test_motion_is_excluding_body", "body"), &PhysicsServer3DExtension::body_test_motion_is_excluding_body);
	ClassDB::bind_method(D_METHOD("body_test_motion_is_excluding_object", "object"), &PhysicsServer3DExtension::body_test_motion_is_excluding_object);

	VLTRVIRTUAL_BIND(_body_create);

	VLTRVIRTUAL_BIND(_body_set_space, "body", "space");
	VLTRVIRTUAL_BIND(_body_get_space, "body");

	VLTRVIRTUAL_BIND(_body_set_mode, "body", "mode");
	VLTRVIRTUAL_BIND(_body_get_mode, "body");

	VLTRVIRTUAL_BIND(_body_add_shape, "body", "shape", "transform", "disabled");
	VLTRVIRTUAL_BIND(_body_set_shape, "body", "shape_idx", "shape");
	VLTRVIRTUAL_BIND(_body_set_shape_transform, "body", "shape_idx", "transform");
	VLTRVIRTUAL_BIND(_body_set_shape_disabled, "body", "shape_idx", "disabled");

	VLTRVIRTUAL_BIND(_body_get_shape_count, "body");
	VLTRVIRTUAL_BIND(_body_get_shape, "body", "shape_idx");
	VLTRVIRTUAL_BIND(_body_get_shape_transform, "body", "shape_idx");

	VLTRVIRTUAL_BIND(_body_remove_shape, "body", "shape_idx");
	VLTRVIRTUAL_BIND(_body_clear_shapes, "body");

	VLTRVIRTUAL_BIND(_body_attach_object_instance_id, "body", "id");
	VLTRVIRTUAL_BIND(_body_get_object_instance_id, "body");

	VLTRVIRTUAL_BIND(_body_set_enable_continuous_collision_detection, "body", "enable");
	VLTRVIRTUAL_BIND(_body_is_continuous_collision_detection_enabled, "body");

	VLTRVIRTUAL_BIND(_body_set_collision_layer, "body", "layer");
	VLTRVIRTUAL_BIND(_body_get_collision_layer, "body");

	VLTRVIRTUAL_BIND(_body_set_collision_mask, "body", "mask");
	VLTRVIRTUAL_BIND(_body_get_collision_mask, "body");

	VLTRVIRTUAL_BIND(_body_set_collision_priority, "body", "priority");
	VLTRVIRTUAL_BIND(_body_get_collision_priority, "body");

	VLTRVIRTUAL_BIND(_body_set_user_flags, "body", "flags");
	VLTRVIRTUAL_BIND(_body_get_user_flags, "body");

	VLTRVIRTUAL_BIND(_body_set_param, "body", "param", "value");
	VLTRVIRTUAL_BIND(_body_get_param, "body", "param");

	VLTRVIRTUAL_BIND(_body_reset_mass_properties, "body");

	VLTRVIRTUAL_BIND(_body_set_state, "body", "state", "value");
	VLTRVIRTUAL_BIND(_body_get_state, "body", "state");

	VLTRVIRTUAL_BIND(_body_apply_central_impulse, "body", "impulse");
	VLTRVIRTUAL_BIND(_body_apply_impulse, "body", "impulse", "position");
	VLTRVIRTUAL_BIND(_body_apply_torque_impulse, "body", "impulse");

	VLTRVIRTUAL_BIND(_body_apply_central_force, "body", "force");
	VLTRVIRTUAL_BIND(_body_apply_force, "body", "force", "position");
	VLTRVIRTUAL_BIND(_body_apply_torque, "body", "torque");

	VLTRVIRTUAL_BIND(_body_add_constant_central_force, "body", "force");
	VLTRVIRTUAL_BIND(_body_add_constant_force, "body", "force", "position");
	VLTRVIRTUAL_BIND(_body_add_constant_torque, "body", "torque");

	VLTRVIRTUAL_BIND(_body_set_constant_force, "body", "force");
	VLTRVIRTUAL_BIND(_body_get_constant_force, "body");

	VLTRVIRTUAL_BIND(_body_set_constant_torque, "body", "torque");
	VLTRVIRTUAL_BIND(_body_get_constant_torque, "body");

	VLTRVIRTUAL_BIND(_body_set_axis_velocity, "body", "axis_velocity");

	VLTRVIRTUAL_BIND(_body_set_axis_lock, "body", "axis", "lock");
	VLTRVIRTUAL_BIND(_body_is_axis_locked, "body", "axis");

	VLTRVIRTUAL_BIND(_body_add_collision_exception, "body", "excepted_body");
	VLTRVIRTUAL_BIND(_body_remove_collision_exception, "body", "excepted_body");
	VLTRVIRTUAL_BIND(_body_get_collision_exceptions, "body");

	VLTRVIRTUAL_BIND(_body_set_max_contacts_reported, "body", "amount");
	VLTRVIRTUAL_BIND(_body_get_max_contacts_reported, "body");

	VLTRVIRTUAL_BIND(_body_set_contacts_reported_depth_threshold, "body", "threshold");
	VLTRVIRTUAL_BIND(_body_get_contacts_reported_depth_threshold, "body");

	VLTRVIRTUAL_BIND(_body_set_omit_force_integration, "body", "enable");
	VLTRVIRTUAL_BIND(_body_is_omitting_force_integration, "body");

	VLTRVIRTUAL_BIND(_body_set_state_sync_callback, "body", "callable");
	VLTRVIRTUAL_BIND(_body_set_force_integration_callback, "body", "callable", "userdata");

	VLTRVIRTUAL_BIND(_body_set_ray_pickable, "body", "enable");

	VLTRVIRTUAL_BIND(_body_test_motion, "body", "from", "motion", "margin", "max_collisions", "collide_separation_ray", "recovery_as_collision", "r_result");

	VLTRVIRTUAL_BIND(_body_get_direct_state, "body");

	/* SOFT BODY API */

	VLTRVIRTUAL_BIND(_soft_body_create);

	VLTRVIRTUAL_BIND(_soft_body_update_rendering_server, "body", "rendering_server_handler");

	VLTRVIRTUAL_BIND(_soft_body_set_space, "body", "space");
	VLTRVIRTUAL_BIND(_soft_body_get_space, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_ray_pickable, "body", "enable");

	VLTRVIRTUAL_BIND(_soft_body_set_collision_layer, "body", "layer");
	VLTRVIRTUAL_BIND(_soft_body_get_collision_layer, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_collision_mask, "body", "mask");
	VLTRVIRTUAL_BIND(_soft_body_get_collision_mask, "body");

	VLTRVIRTUAL_BIND(_soft_body_add_collision_exception, "body", "body_b");
	VLTRVIRTUAL_BIND(_soft_body_remove_collision_exception, "body", "body_b");
	VLTRVIRTUAL_BIND(_soft_body_get_collision_exceptions, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_state, "body", "state", "variant");
	VLTRVIRTUAL_BIND(_soft_body_get_state, "body", "state");

	VLTRVIRTUAL_BIND(_soft_body_set_transform, "body", "transform");

	VLTRVIRTUAL_BIND(_soft_body_set_simulation_precision, "body", "simulation_precision");
	VLTRVIRTUAL_BIND(_soft_body_get_simulation_precision, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_total_mass, "body", "total_mass");
	VLTRVIRTUAL_BIND(_soft_body_get_total_mass, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_linear_stiffness, "body", "linear_stiffness");
	VLTRVIRTUAL_BIND(_soft_body_get_linear_stiffness, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_shrinking_factor, "body", "shrinking_factor");
	VLTRVIRTUAL_BIND(_soft_body_get_shrinking_factor, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_pressure_coefficient, "body", "pressure_coefficient");
	VLTRVIRTUAL_BIND(_soft_body_get_pressure_coefficient, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_damping_coefficient, "body", "damping_coefficient");
	VLTRVIRTUAL_BIND(_soft_body_get_damping_coefficient, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_drag_coefficient, "body", "drag_coefficient");
	VLTRVIRTUAL_BIND(_soft_body_get_drag_coefficient, "body");

	VLTRVIRTUAL_BIND(_soft_body_set_mesh, "body", "mesh");

	VLTRVIRTUAL_BIND(_soft_body_get_bounds, "body");

	VLTRVIRTUAL_BIND(_soft_body_move_point, "body", "point_index", "global_position");
	VLTRVIRTUAL_BIND(_soft_body_get_point_global_position, "body", "point_index");

	VLTRVIRTUAL_BIND(_soft_body_remove_all_pinned_points, "body");
	VLTRVIRTUAL_BIND(_soft_body_pin_point, "body", "point_index", "pin");
	VLTRVIRTUAL_BIND(_soft_body_is_point_pinned, "body", "point_index");

	VLTRVIRTUAL_BIND(_soft_body_apply_point_impulse, "body", "point_index", "impulse");
	VLTRVIRTUAL_BIND(_soft_body_apply_point_force, "body", "point_index", "force");
	VLTRVIRTUAL_BIND(_soft_body_apply_central_impulse, "body", "impulse");
	VLTRVIRTUAL_BIND(_soft_body_apply_central_force, "body", "force");

	/* JOINT API */

	VLTRVIRTUAL_BIND(_joint_create);
	VLTRVIRTUAL_BIND(_joint_clear, "joint");

	VLTRVIRTUAL_BIND(_joint_make_pin, "joint", "body_A", "local_A", "body_B", "local_B");

	VLTRVIRTUAL_BIND(_pin_joint_set_param, "joint", "param", "value");
	VLTRVIRTUAL_BIND(_pin_joint_get_param, "joint", "param");

	VLTRVIRTUAL_BIND(_pin_joint_set_local_a, "joint", "local_A");
	VLTRVIRTUAL_BIND(_pin_joint_get_local_a, "joint");

	VLTRVIRTUAL_BIND(_pin_joint_set_local_b, "joint", "local_B");
	VLTRVIRTUAL_BIND(_pin_joint_get_local_b, "joint");

	VLTRVIRTUAL_BIND(_joint_make_hinge, "joint", "body_A", "hinge_A", "body_B", "hinge_B");
	VLTRVIRTUAL_BIND(_joint_make_hinge_simple, "joint", "body_A", "pivot_A", "axis_A", "body_B", "pivot_B", "axis_B");

	VLTRVIRTUAL_BIND(_hinge_joint_set_param, "joint", "param", "value");
	VLTRVIRTUAL_BIND(_hinge_joint_get_param, "joint", "param");

	VLTRVIRTUAL_BIND(_hinge_joint_set_flag, "joint", "flag", "enabled");
	VLTRVIRTUAL_BIND(_hinge_joint_get_flag, "joint", "flag");

	VLTRVIRTUAL_BIND(_joint_make_slider, "joint", "body_A", "local_ref_A", "body_B", "local_ref_B");

	VLTRVIRTUAL_BIND(_slider_joint_set_param, "joint", "param", "value");
	VLTRVIRTUAL_BIND(_slider_joint_get_param, "joint", "param");

	VLTRVIRTUAL_BIND(_joint_make_cone_twist, "joint", "body_A", "local_ref_A", "body_B", "local_ref_B");

	VLTRVIRTUAL_BIND(_cone_twist_joint_set_param, "joint", "param", "value");
	VLTRVIRTUAL_BIND(_cone_twist_joint_get_param, "joint", "param");

	VLTRVIRTUAL_BIND(_joint_make_generic_6dof, "joint", "body_A", "local_ref_A", "body_B", "local_ref_B");

	VLTRVIRTUAL_BIND(_generic_6dof_joint_set_param, "joint", "axis", "param", "value");
	VLTRVIRTUAL_BIND(_generic_6dof_joint_get_param, "joint", "axis", "param");

	VLTRVIRTUAL_BIND(_generic_6dof_joint_set_flag, "joint", "axis", "flag", "enable");
	VLTRVIRTUAL_BIND(_generic_6dof_joint_get_flag, "joint", "axis", "flag");

	VLTRVIRTUAL_BIND(_generic_6dof_joint_set_angular_target_rotation, "joint", "target_rotation");
	VLTRVIRTUAL_BIND(_generic_6dof_joint_get_angular_target_rotation, "joint");

	VLTRVIRTUAL_BIND(_joint_get_type, "joint");

	VLTRVIRTUAL_BIND(_joint_set_solver_priority, "joint", "priority");
	VLTRVIRTUAL_BIND(_joint_get_solver_priority, "joint");

	VLTRVIRTUAL_BIND(_joint_disable_collisions_between_bodies, "joint", "disable");
	VLTRVIRTUAL_BIND(_joint_is_disabled_collisions_between_bodies, "joint");

	VLTRVIRTUAL_BIND(_free_rid, "rid");

	VLTRVIRTUAL_BIND(_set_active, "active");

	VLTRVIRTUAL_BIND(_init);
	VLTRVIRTUAL_BIND(_step, "step");
	VLTRVIRTUAL_BIND(_sync);
	VLTRVIRTUAL_BIND(_flush_queries);
	VLTRVIRTUAL_BIND(_end_sync);
	VLTRVIRTUAL_BIND(_finish);

	VLTRVIRTUAL_BIND(_is_flushing_queries);
	VLTRVIRTUAL_BIND(_get_process_info, "process_info");
}

PhysicsServer3DExtension::PhysicsServer3DExtension() {
}

PhysicsServer3DExtension::~PhysicsServer3DExtension() {
}
