/**************************************************************************/
/*  collision_object_3d.cpp                                               */
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

#include "collision_object_3d.h"

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

void CollisionState3D::init(bool p_area, PS3DE::BodyMode p_mode) {
	area = p_area;
	body_mode = p_mode;
	if (area) {
		rid = PhysicsServer3D::area_create();
	} else {
		rid = PhysicsServer3D::body_create();
		PhysicsServer3D::body_set_mode(rid, body_mode);
	}
}

void CollisionState3D::free_rid() {
	clear_debug_shapes();
	if (rid.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(rid);
	}
	rid = RID();
}

void CollisionState3D::clear_debug_shapes() {
	if (!Renderer::data) {
		debug_shapes_count = 0;
		return;
	}
	for (KeyValue<uint32_t, ShapeData> &E : shapes) {
		ShapeData &sd = E.value;
		for (int i = 0; i < sd.shapes.size(); i++) {
			if (sd.shapes[i].debug_shape.is_valid()) {
				Renderer::free_rid(sd.shapes[i].debug_shape);
				sd.shapes.write[i].debug_shape = RID();
			}
		}
	}
	debug_shapes_count = 0;
}

void CollisionState3D::on_transform_changed(const Node3D *p_owner) {
	if (debug_shapes_count > 0 && !debug_shape_old_transform.is_equal_approx(p_owner->get_global_transform())) {
		debug_shape_old_transform = p_owner->get_global_transform();
		for (KeyValue<uint32_t, ShapeData> &E : shapes) {
			ShapeData &sd = E.value;
			if (sd.disabled) {
				continue;
			}
			const ShapeData::ShapeBase *shape_bases = sd.shapes.ptr();
			for (int i = 0; i < sd.shapes.size(); i++) {
				if (shape_bases[i].debug_shape.is_null()) {
					continue;
				}
				Renderer::instance_set_transform(shape_bases[i].debug_shape, debug_shape_old_transform * sd.xform);
			}
		}
	}
}

void CollisionState3D::handle_notification(Node3D *p_owner, int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			if (area) {
				PhysicsServer3D::area_set_transform(rid, p_owner->get_global_transform());
			}
			if (p_owner->is_enabled() && p_owner->get_world_3d().is_valid()) {
				RID space = p_owner->get_world_3d()->get_space();
				if (area) {
					PhysicsServer3D::area_set_space(rid, space);
				} else {
					PhysicsServer3D::body_set_space(rid, space);
				}
			}
			update_pickable(p_owner);
		} break;
		case Node3D::NOTIFICATION_TRANSFORM_CHANGED: {
			on_transform_changed(p_owner);
			if (!only_update_transform_changes && area) {
				PhysicsServer3D::area_set_transform(rid, p_owner->get_global_transform());
			}
		} break;
		case Node3D::NOTIFICATION_VISIBILITY_CHANGED: {
			update_pickable(p_owner);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			clear_debug_shapes();
			if (callback_lock == 0) {
				if (area) {
					PhysicsServer3D::area_set_space(rid, RID());
				} else {
					PhysicsServer3D::body_set_space(rid, RID());
				}
			}
		} break;
		case Node::NOTIFICATION_DISABLED: {
			apply_disabled(p_owner);
		} break;
		case Node::NOTIFICATION_ENABLED: {
			apply_enabled(p_owner);
		} break;
	}
}

void CollisionState3D::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	if (area) {
		PhysicsServer3D::area_set_collision_layer(rid, p_layer);
	} else {
		PhysicsServer3D::body_set_collision_layer(rid, p_layer);
	}
}

void CollisionState3D::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	if (area) {
		PhysicsServer3D::area_set_collision_mask(rid, p_mask);
	} else {
		PhysicsServer3D::body_set_collision_mask(rid, p_mask);
	}
}

void CollisionState3D::set_collision_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t layer_new = collision_layer;
	if (p_value) {
		layer_new |= 1 << (p_layer_number - 1);
	} else {
		layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(layer_new);
}

bool CollisionState3D::get_collision_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_layer & (1 << (p_layer_number - 1));
}

void CollisionState3D::set_collision_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = collision_mask;
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool CollisionState3D::get_collision_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_mask & (1 << (p_layer_number - 1));
}

void CollisionState3D::set_collision_priority(real_t p_priority) {
	collision_priority = p_priority;
	if (!area) {
		PhysicsServer3D::body_set_collision_priority(rid, p_priority);
	}
}

void CollisionState3D::set_disable_mode(Node3D *p_owner, DisableMode p_mode) {
	if (disable_mode == p_mode) {
		return;
	}
	bool disabled = p_owner->is_inside_tree() && !p_owner->is_enabled();
	if (disabled) {
		apply_enabled(p_owner);
	}
	disable_mode = p_mode;
	if (disabled) {
		apply_disabled(p_owner);
	}
}

void CollisionState3D::apply_disabled(Node3D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				if (callback_lock > 0) {
					ERR_PRINT("Disabling a collision node during a physics callback is not allowed.");
				} else {
					if (area) {
						PhysicsServer3D::area_set_space(rid, RID());
					} else {
						PhysicsServer3D::body_set_space(rid, RID());
					}
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS3DE::BODY_MODE_STATIC)) {
				PhysicsServer3D::body_set_mode(rid, PS3DE::BODY_MODE_STATIC);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState3D::apply_enabled(Node3D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree() && p_owner->get_world_3d().is_valid()) {
				RID space = p_owner->get_world_3d()->get_space();
				if (area) {
					PhysicsServer3D::area_set_space(rid, space);
				} else {
					PhysicsServer3D::body_set_space(rid, space);
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS3DE::BODY_MODE_STATIC)) {
				PhysicsServer3D::body_set_mode(rid, body_mode);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState3D::set_body_mode(const Node3D *p_owner, PS3DE::BodyMode p_mode) {
	ERR_FAIL_COND(area);
	if (body_mode == p_mode) {
		return;
	}
	body_mode = p_mode;
	if (p_owner->is_inside_tree() && !p_owner->is_enabled() && (disable_mode == DISABLE_MODE_MAKE_STATIC)) {
		return;
	}
	PhysicsServer3D::body_set_mode(rid, p_mode);
}

void CollisionState3D::set_ray_pickable(const Node3D *p_owner, bool p_ray_pickable) {
	ray_pickable = p_ray_pickable;
	update_pickable(p_owner);
}

void CollisionState3D::update_pickable(const Node3D *p_owner) {
	if (!p_owner->is_inside_tree()) {
		return;
	}
	bool pick = ray_pickable && p_owner->is_visible_in_tree();
	if (area) {
		PhysicsServer3D::area_set_ray_pickable(rid, pick);
	} else {
		PhysicsServer3D::body_set_ray_pickable(rid, pick);
	}
}

// Inline shape slots.

int CollisionState3D::add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform, bool p_disabled) {
	ShapeSlot slot;
	slot.local_xform = p_xform;
	slot.shape = p_shape;
	slot.disabled = p_disabled;
	slot.owner_id = create_shape_owner();
	shape_owner_set_transform(slot.owner_id, p_xform);
	shape_owner_set_disabled(slot.owner_id, p_disabled);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, p_shape);
	}
	int idx = (int)inline_shapes.size();
	inline_shapes.push_back(slot);
	return idx;
}

void CollisionState3D::set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	ShapeSlot &slot = inline_shapes[p_index];
	slot.shape = p_shape;
	shape_owner_clear_shapes(slot.owner_id);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, p_shape);
	}
}

Ref<Shape3D> CollisionState3D::get_inline_shape(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Ref<Shape3D>());
	return inline_shapes[p_index].shape;
}

void CollisionState3D::set_inline_shape_transform(int p_index, const Transform3D &p_xform) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].local_xform = p_xform;
	shape_owner_set_transform(inline_shapes[p_index].owner_id, p_xform);
}

Transform3D CollisionState3D::get_inline_shape_transform(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Transform3D());
	return inline_shapes[p_index].local_xform;
}

void CollisionState3D::set_inline_shape_disabled(int p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].disabled = p_disabled;
	shape_owner_set_disabled(inline_shapes[p_index].owner_id, p_disabled);
}

bool CollisionState3D::is_inline_shape_disabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), false);
	return inline_shapes[p_index].disabled;
}

void CollisionState3D::remove_inline_shape(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	remove_shape_owner(inline_shapes[p_index].owner_id);
	inline_shapes.remove_at(p_index);
}

void CollisionState3D::clear_inline_shapes() {
	for (uint32_t i = 0; i < inline_shapes.size(); i++) {
		remove_shape_owner(inline_shapes[i].owner_id);
	}
	inline_shapes.clear();
}

// Shape owners.

uint32_t CollisionState3D::create_shape_owner(Node *p_owner) {
	uint32_t id = next_owner_id++;
	ShapeData sd;
	sd.owner_node = p_owner;
	shapes[id] = sd;
	return id;
}

void CollisionState3D::remove_shape_owner(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	shape_owner_clear_shapes(p_owner);
	shapes.erase(p_owner);
}

void CollisionState3D::get_shape_owners(List<uint32_t> *r_owners) const {
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		r_owners->push_back(E.key);
	}
}

PackedInt32Array CollisionState3D::get_shape_owners_array() const {
	PackedInt32Array ret;
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		ret.push_back(E.key);
	}
	return ret;
}

void CollisionState3D::shape_owner_set_transform(uint32_t p_owner, const Transform3D &p_transform) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.xform = p_transform;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer3D::area_set_shape_transform(rid, sd.shapes[i].index, p_transform);
		} else {
			PhysicsServer3D::body_set_shape_transform(rid, sd.shapes[i].index, p_transform);
		}
	}
}

Transform3D CollisionState3D::shape_owner_get_transform(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Transform3D());
	return shapes[p_owner].xform;
}

void CollisionState3D::shape_owner_set_disabled(uint32_t p_owner, bool p_disabled) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	if (sd.disabled == p_disabled) {
		return;
	}
	sd.disabled = p_disabled;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer3D::area_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		} else {
			PhysicsServer3D::body_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		}
	}
}

bool CollisionState3D::is_shape_owner_disabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].disabled;
}

void CollisionState3D::shape_owner_add_shape(uint32_t p_owner, const Ref<Shape3D> &p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_COND(p_shape.is_null());
	ShapeData &sd = shapes[p_owner];
	ShapeData::ShapeBase s;
	s.index = total_subshapes;
	s.shape = p_shape;
	if (area) {
		PhysicsServer3D::area_add_shape(rid, p_shape->get_rid(), sd.xform, sd.disabled);
	} else {
		PhysicsServer3D::body_add_shape(rid, p_shape->get_rid(), sd.xform, sd.disabled);
	}
	sd.shapes.push_back(s);
	total_subshapes++;
}

int CollisionState3D::shape_owner_get_shape_count(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].shapes.size();
}

Ref<Shape3D> CollisionState3D::shape_owner_get_shape(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Ref<Shape3D>());
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), Ref<Shape3D>());
	return shapes[p_owner].shapes[p_shape].shape;
}

int CollisionState3D::shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), -1);
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), -1);
	return shapes[p_owner].shapes[p_shape].index;
}

void CollisionState3D::shape_owner_remove_shape(uint32_t p_owner, int p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_INDEX(p_shape, shapes[p_owner].shapes.size());

	int index_to_remove = shapes[p_owner].shapes[p_shape].index;
	RID debug_rid = shapes[p_owner].shapes[p_shape].debug_shape;
	if (debug_rid.is_valid() && Renderer::data) {
		Renderer::free_rid(debug_rid);
		debug_shapes_count--;
	}
	if (area) {
		PhysicsServer3D::area_remove_shape(rid, index_to_remove);
	} else {
		PhysicsServer3D::body_remove_shape(rid, index_to_remove);
	}

	shapes[p_owner].shapes.remove_at(p_shape);

	for (KeyValue<uint32_t, ShapeData> &E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index > index_to_remove) {
				E.value.shapes.write[i].index -= 1;
			}
		}
	}
	total_subshapes--;
}

void CollisionState3D::shape_owner_clear_shapes(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	while (shape_owner_get_shape_count(p_owner) > 0) {
		shape_owner_remove_shape(p_owner, 0);
	}
}

uint32_t CollisionState3D::shape_find_owner(int p_shape_index) const {
	ERR_FAIL_INDEX_V(p_shape_index, total_subshapes, UINT32_MAX);
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index == p_shape_index) {
				return E.key;
			}
		}
	}
	ERR_FAIL_V_MSG(UINT32_MAX, "Can't find owner for shape index " + itos(p_shape_index) + ".");
}

// ==========================================================================
// Merged from scene/3d/physics/area_3d.cpp
// ==========================================================================

#include "area_3d.h"

#include "core/config/engine.h"
#include "servers/audio/audio_server.h"

Area3D::Area3D() {
	collision_state.init(true);
	set_notify_transform(true);
	set_monitoring(true);
	set_monitorable(true);
}

Area3D::~Area3D() {
	collision_state.free_rid();
}

void Area3D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_initialize_wind();
		} break;
		case NOTIFICATION_EXIT_TREE:
		case NOTIFICATION_DISABLED: {
			_clear_monitoring();
		} break;
	}
}

void Area3D::set_gravity_space_override_mode(SpaceOverride p_mode) {
	gravity_space_override = p_mode;
}

Area3D::SpaceOverride Area3D::get_gravity_space_override_mode() const {
	return gravity_space_override;
}

void Area3D::set_gravity_is_point(bool p_enabled) {
	gravity_is_point = p_enabled;
}

bool Area3D::is_gravity_a_point() const {
	return gravity_is_point;
}

void Area3D::set_gravity_point_unit_distance(real_t p_scale) {
	gravity_point_unit_distance = p_scale;
}

real_t Area3D::get_gravity_point_unit_distance() const {
	return gravity_point_unit_distance;
}

void Area3D::set_gravity_point_center(const Vector3 &p_center) {
	gravity_vec = p_center;
}

const Vector3 &Area3D::get_gravity_point_center() const {
	return gravity_vec;
}

void Area3D::set_gravity_direction(const Vector3 &p_direction) {
	gravity_vec = p_direction;
}

const Vector3 &Area3D::get_gravity_direction() const {
	return gravity_vec;
}

void Area3D::set_gravity(real_t p_gravity) {
	gravity = p_gravity;
}

real_t Area3D::get_gravity() const {
	return gravity;
}

void Area3D::set_linear_damp_space_override_mode(SpaceOverride p_mode) {
	linear_damp_space_override = p_mode;
}

Area3D::SpaceOverride Area3D::get_linear_damp_space_override_mode() const {
	return linear_damp_space_override;
}

void Area3D::set_angular_damp_space_override_mode(SpaceOverride p_mode) {
	angular_damp_space_override = p_mode;
}

Area3D::SpaceOverride Area3D::get_angular_damp_space_override_mode() const {
	return angular_damp_space_override;
}

void Area3D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

real_t Area3D::get_linear_damp() const {
	return linear_damp;
}

void Area3D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

real_t Area3D::get_angular_damp() const {
	return angular_damp;
}

void Area3D::set_priority(int p_priority) {
	priority = p_priority;
}

int Area3D::get_priority() const {
	return priority;
}

void Area3D::set_wind_force_magnitude(real_t p_wind_force_magnitude) {
	wind_force_magnitude = p_wind_force_magnitude;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

real_t Area3D::get_wind_force_magnitude() const {
	return wind_force_magnitude;
}

void Area3D::set_wind_attenuation_factor(real_t p_wind_force_attenuation_factor) {
	wind_attenuation_factor = p_wind_force_attenuation_factor;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

real_t Area3D::get_wind_attenuation_factor() const {
	return wind_attenuation_factor;
}

void Area3D::set_wind_source_path(const NodePath &p_wind_source_path) {
	wind_source_path = p_wind_source_path;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

const NodePath &Area3D::get_wind_source_path() const {
	return wind_source_path;
}

void Area3D::set_monitoring(bool p_enable) {
	if (p_enable == monitoring) {
		return;
	}
	monitoring = p_enable;
	if (!monitoring) {
		_clear_monitoring();
	}
}

bool Area3D::is_monitoring() const {
	return monitoring;
}

void Area3D::set_monitorable(bool p_enable) {
	ERR_FAIL_COND_MSG(locked || (is_inside_tree() && PhysicsServer3D::is_flushing_queries()),
			"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");
	if (p_enable == monitorable) {
		return;
	}
	monitorable = p_enable;
	PhysicsServer3D::area_set_monitorable(get_rid(), monitorable);
}

bool Area3D::is_monitorable() const {
	return monitorable;
}

bool Area3D::has_overlapping_bodies() const {
	return false;
}

bool Area3D::has_overlapping_areas() const {
	return false;
}

bool Area3D::overlaps_area(Node *rp_area) const {
	return false;
}

bool Area3D::overlaps_body(Node *rp_body) const {
	return false;
}

void Area3D::set_audio_bus_override(bool p_override) {
	audio_bus_override = p_override;
}

bool Area3D::is_overriding_audio_bus() const {
	return audio_bus_override;
}

void Area3D::set_audio_bus_name(const String &p_audio_bus) {
	audio_bus = p_audio_bus;
}

String Area3D::get_audio_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneString(Master);
}

void Area3D::set_use_reverb_bus(bool p_enable) {
	use_reverb_bus = p_enable;
}

bool Area3D::is_using_reverb_bus() const {
	return use_reverb_bus;
}

void Area3D::set_reverb_bus_name(const String &p_audio_bus) {
	reverb_bus = p_audio_bus;
}

String Area3D::get_reverb_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == reverb_bus) {
			return reverb_bus;
		}
	}
	return SceneString(Master);
}

void Area3D::set_reverb_amount(float p_amount) {
	reverb_amount = p_amount;
}

float Area3D::get_reverb_amount() const {
	return reverb_amount;
}

void Area3D::set_reverb_uniformity(float p_uniformity) {
	reverb_uniformity = p_uniformity;
}

float Area3D::get_reverb_uniformity() const {
	return reverb_uniformity;
}

void Area3D::_initialize_wind() {}

void Area3D::_clear_monitoring() {}

PackedStringArray Area3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape3D / CollisionPolygon3D child."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/3d/physics/collision_shape_3d.cpp
// ==========================================================================

#include "collision_shape_3d.h"

#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"

void CollisionShape3D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
}

void CollisionShape3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node3D *parent_3d = dynamic_cast<Node3D *>(get_parent());
			collision_state = parent_3d ? parent_3d->get_collision_state_3d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape);
				}
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

#ifndef DISABLE_DEPRECATED
void CollisionShape3D::resource_changed(Ref<Resource> res) {}
#endif

Ref<Shape3D> CollisionShape3D::get_shape() const {
	return shape;
}

void CollisionShape3D::set_shape(const Ref<Shape3D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	if (shape.is_valid()) {
		shape->set_debug_color(debug_color);
		shape->set_debug_fill(debug_fill);
	}
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape);
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

void CollisionShape3D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape3D::is_disabled() const {
	return disabled;
}

Color CollisionShape3D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape3D::set_debug_color(const Color &p_color) {
	if (debug_color == p_color) {
		return;
	}
	debug_color = p_color;
	if (shape.is_valid()) {
		shape->set_debug_color(p_color);
	}
}

Color CollisionShape3D::get_debug_color() const {
	return debug_color;
}

void CollisionShape3D::set_debug_fill_enabled(bool p_enable) {
	if (debug_fill == p_enable) {
		return;
	}
	debug_fill = p_enable;
	if (shape.is_valid()) {
		shape->set_debug_fill(p_enable);
	}
}

bool CollisionShape3D::get_debug_fill_enabled() const {
	return debug_fill;
}

#ifdef DEBUG_ENABLED
void CollisionShape3D::_shape_changed() {
	if (shape.is_valid()) {
		if (shape->get_debug_color() != debug_color) {
			set_debug_color(shape->get_debug_color());
		}
		if (shape->get_debug_fill() != debug_fill) {
			set_debug_fill_enabled(shape->get_debug_fill());
		}
	}
}
#endif // DEBUG_ENABLED

CollisionShape3D::CollisionShape3D() {
	set_notify_local_transform(true);
	debug_color = _get_default_debug_color();
}

CollisionShape3D::~CollisionShape3D() {}

PackedStringArray CollisionShape3D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/3d/physics/collision_polygon_3d.cpp
// ==========================================================================

#include "collision_polygon_3d.h"

#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"

void CollisionPolygon3D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
}

void CollisionPolygon3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node3D *parent_3d = dynamic_cast<Node3D *>(get_parent());
			collision_state = parent_3d ? parent_3d->get_collision_state_3d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

Vector<Point2> CollisionPolygon3D::get_polygon() const {
	return polygon;
}

AABB CollisionPolygon3D::get_item_rect() const {
	return aabb;
}

real_t CollisionPolygon3D::get_depth() const {
	return depth;
}

void CollisionPolygon3D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon3D::is_disabled() const {
	return disabled;
}

Color CollisionPolygon3D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

Color CollisionPolygon3D::get_debug_color() const {
	return debug_color;
}

bool CollisionPolygon3D::get_debug_fill_enabled() const {
	return debug_fill;
}

real_t CollisionPolygon3D::get_margin() const {
	return margin;
}

bool CollisionPolygon3D::_is_editable_3d_polygon() const {
	return true;
}

CollisionPolygon3D::CollisionPolygon3D() {
	set_notify_local_transform(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionPolygon3D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/3d/physics/ray_cast_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "ray_cast_3d.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

Vector3 RayCast3D::get_target_position() const { return target_position; }

void RayCast3D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t RayCast3D::get_collision_mask() const { return collision_mask; }

void RayCast3D::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool RayCast3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

bool RayCast3D::is_colliding() const { return collided; }

RID RayCast3D::get_collider_rid() const { return against_rid; }

int RayCast3D::get_collider_shape() const { return against_shape; }

Vector3 RayCast3D::get_collision_point() const { return collision_point; }

Vector3 RayCast3D::get_collision_normal() const { return collision_normal; }

int RayCast3D::get_collision_face_index() const { return collision_face_index; }

bool RayCast3D::is_enabled() const { return enabled; }

bool RayCast3D::get_exclude_parent_body() const { return exclude_parent_body; }

void RayCast3D::force_raycast_update() { _update_raycast_state(); }

void RayCast3D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void RayCast3D::add_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	add_exception_rid(state->rid);
}

void RayCast3D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void RayCast3D::remove_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	remove_exception_rid(state->rid);
}

void RayCast3D::set_collide_with_areas(bool p_enabled) { collide_with_areas = p_enabled; }

bool RayCast3D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void RayCast3D::set_collide_with_bodies(bool p_enabled) { collide_with_bodies = p_enabled; }

bool RayCast3D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void RayCast3D::set_hit_from_inside(bool p_enabled) { hit_from_inside = p_enabled; }

bool RayCast3D::is_hit_from_inside_enabled() const { return hit_from_inside; }

void RayCast3D::set_hit_back_faces(bool p_enabled) { hit_back_faces = p_enabled; }

bool RayCast3D::is_hit_back_faces_enabled() const { return hit_back_faces; }

int RayCast3D::get_debug_shape_thickness() const { return debug_shape_thickness; }

void RayCast3D::_update_debug_shape_vertices()
{
	debug_shape_vertices.clear();
	debug_line_vertices.clear();

	if (target_position == Vector3()) {
		return;
	}

	debug_line_vertices.push_back(Vector3());
	debug_line_vertices.push_back(target_position);

	if (debug_shape_thickness > 1) {
		float scale_factor = 100.0;
		Vector3 dir = Vector3(target_position).normalized();
		// Draw truncated pyramid
		Vector3 normal = (std::abs(dir.x) + std::abs(dir.y) > CMP_EPSILON)
							 ? Vector3(-dir.y, dir.x, 0).normalized()
							 : Vector3(0, -dir.z, dir.y).normalized();
		normal *= debug_shape_thickness / scale_factor;
		int vertices_strip_order[14] = {4, 5, 0, 1, 2, 5, 6, 4, 7, 0, 3, 2, 7, 6};
		for (int v = 0; v < 14; v++) {
			Vector3 vertex = vertices_strip_order[v] < 4 ? normal : normal / 3.0 + target_position;
			debug_shape_vertices.push_back(
				vertex.rotated(dir, Math::PI * (0.5 * (vertices_strip_order[v] % 4) + 0.25)));
		}
	}
}

const Vector<Vector3>& RayCast3D::get_debug_shape_vertices() const { return debug_shape_vertices; }

const Vector<Vector3>& RayCast3D::get_debug_line_vertices() const { return debug_line_vertices; }

void RayCast3D::set_debug_shape_custom_color(const Color& p_color)
{
	debug_shape_custom_color = p_color;
	if (debug_material.is_valid()) {
		_update_debug_shape_material();
	}
}

Ref<StandardMaterial3D> RayCast3D::get_debug_material()
{
	_update_debug_shape_material();
	return debug_material;
}

const Color& RayCast3D::get_debug_shape_custom_color() const { return debug_shape_custom_color; }

void RayCast3D::_create_debug_shape()
{
	_update_debug_shape_material();

	if (!debug_instance.is_valid()) {
		debug_instance = Renderer::instance_create();
	}

	if (debug_mesh.is_null()) {
		debug_mesh.instantiate();
	}
}

void RayCast3D::_update_debug_shape_material(bool p_check_collision)
{
	if (debug_material.is_null()) {
		Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
		debug_material = material;

		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		// Use double-sided rendering so that the RayCast can be seen if the camera is inside.
		material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
		material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	}

	Color color = debug_shape_custom_color;
	if (color == Color(0.0, 0.0, 0.0)) {
		// Use the default debug shape color defined in the Project Settings.
		color = SceneTree::get_debug_collisions_color();
	}

	if (p_check_collision && collided) {
		if ((color.get_h() < 0.055 || color.get_h() > 0.945) && color.get_s() > 0.5 &&
			color.get_v() > 0.5) {
			// If base color is already quite reddish, highlight collision with green color
			color = Color(0.0, 1.0, 0.0, color.a);
		}
		else {
			// Else, highlight collision with red color
			color = Color(1.0, 0, 0, color.a);
		}
	}

	Ref<StandardMaterial3D> material = static_cast<Ref<StandardMaterial3D>>(debug_material);
}

void RayCast3D::_clear_debug_shape()
{
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
		debug_instance = RID();
	}
	if (debug_mesh.is_valid()) {
		Renderer::free_rid(debug_mesh->get_rid());
		debug_mesh = Ref<ArrayMesh>();
	}
}

void RayCast3D::_update_raycast_state() {}

// ==========================================================================
// Merged from scene/3d/physics/shape_cast_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
#include "shape_cast_3d.h"

bool ShapeCast3D::is_enabled() const { return enabled; }

Vector3 ShapeCast3D::get_target_position() const { return target_position; }

void ShapeCast3D::set_margin(real_t p_margin) { margin = p_margin; }

real_t ShapeCast3D::get_margin() const { return margin; }

void ShapeCast3D::set_max_results(int p_max_results) { max_results = p_max_results; }

int ShapeCast3D::get_max_results() const { return max_results; }

void ShapeCast3D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t ShapeCast3D::get_collision_mask() const { return collision_mask; }

void ShapeCast3D::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool ShapeCast3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

int ShapeCast3D::get_collision_count() const { return result.size(); }

bool ShapeCast3D::is_colliding() const { return collided; }

RID ShapeCast3D::get_collider_rid(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), RID(), "No collider RID found.");
	return result[p_idx].rid;
}

int ShapeCast3D::get_collider_shape(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), -1, "No collider shape found.");
	return result[p_idx].shape;
}

Vector3 ShapeCast3D::get_collision_point(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector3(), "No collision point found.");
	return result[p_idx].point;
}

Vector3 ShapeCast3D::get_collision_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector3(), "No collision normal found.");
	return result[p_idx].normal;
}

real_t ShapeCast3D::get_closest_collision_safe_fraction() const { return collision_safe_fraction; }

real_t ShapeCast3D::get_closest_collision_unsafe_fraction() const
{
	return collision_unsafe_fraction;
}

#ifndef DISABLE_DEPRECATED
void ShapeCast3D::resource_changed(Ref<Resource> p_res) {}
#endif

Ref<Shape3D> ShapeCast3D::get_shape() const { return shape; }

bool ShapeCast3D::get_exclude_parent_body() const { return exclude_parent_body; }

void ShapeCast3D::_update_shapecast_state()
{
	result.clear();

	ERR_FAIL_COND_MSG(shape.is_null(),
		"Null reference to shape. ShapeCast3D requires a Shape3D to sweep for collisions.");

	Ref<World3D> w3d = get_world_3d();
	ERR_FAIL_COND(w3d.is_null());

	PhysicsDirectSpaceState3D* dss =
		PhysicsServer3D::space_get_direct_state(w3d->get_space());
	ERR_FAIL_NULL(dss);

	Transform3D gt = get_global_transform();

	PS3DT::ShapeParameters params;
	params.shape_rid = shape_rid;
	params.transform = gt;
	params.motion = gt.basis.xform(target_position);
	params.margin = margin;
	params.exclude = exclude;
	params.collision_mask = collision_mask;
	params.collide_with_bodies = collide_with_bodies;
	params.collide_with_areas = collide_with_areas;

	collision_safe_fraction = 0.0;
	collision_unsafe_fraction = 0.0;

	if (target_position != Vector3()) {
		dss->cast_motion(params, collision_safe_fraction, collision_unsafe_fraction);
		if (collision_unsafe_fraction < 1.0) {
			// Move shape transform to the point of impact,
			// so we can collect contact info at that point.
			gt.set_origin(
				gt.get_origin() + params.motion * (collision_unsafe_fraction + CMP_EPSILON));
			params.transform = gt;
		}
	}
	// Regardless of whether the shape is stuck or it's moved along
	// the motion vector, we'll only consider static collisions from now on.
	params.motion = Vector3();

	bool intersected = true;
	while (intersected && result.size() < max_results) {
		PS3DT::ShapeRestInfo info;
		intersected = dss->rest_info(params, &info);
		if (intersected) {
			result.push_back(info);
			params.exclude.insert(info.rid);
		}
	}
	collided = !result.is_empty();
}

void ShapeCast3D::force_shapecast_update() { _update_shapecast_state(); }

void ShapeCast3D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void ShapeCast3D::add_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	add_exception_rid(state->rid);
}

void ShapeCast3D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void ShapeCast3D::remove_exception(const Node3D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState3D* state = rp_node->get_collision_state_3d();
	ERR_FAIL_NULL_MSG(state, "The passed Node must have a CollisionState3D (PhysicsBody3D or Area3D).");
	remove_exception_rid(state->rid);
}

void ShapeCast3D::clear_exceptions() { exclude.clear(); }

void ShapeCast3D::set_collide_with_areas(bool p_clip) { collide_with_areas = p_clip; }

bool ShapeCast3D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void ShapeCast3D::set_collide_with_bodies(bool p_clip) { collide_with_bodies = p_clip; }

bool ShapeCast3D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void ShapeCast3D::_update_debug_shape_vertices()
{
	debug_shape_vertices.clear();
	debug_line_vertices.clear();

	if (shape.is_valid()) {
		debug_shape_vertices.append_array(shape->get_debug_mesh_lines());
		for (int i = 0; i < debug_shape_vertices.size(); i++) {
			debug_shape_vertices.set(
				i, debug_shape_vertices[i] +
					   Vector3(target_position * get_closest_collision_safe_fraction()));
		}
	}

	if (target_position == Vector3()) {
		return;
	}

	debug_line_vertices.push_back(Vector3());
	debug_line_vertices.push_back(target_position);
}

const Vector<Vector3>& ShapeCast3D::get_debug_shape_vertices() const
{
	return debug_shape_vertices;
}

const Vector<Vector3>& ShapeCast3D::get_debug_line_vertices() const { return debug_line_vertices; }

void ShapeCast3D::set_debug_shape_custom_color(const Color& p_color)
{
	debug_shape_custom_color = p_color;
	if (debug_material.is_valid()) {
		_update_debug_shape_material();
	}
}

Ref<StandardMaterial3D> ShapeCast3D::get_debug_material()
{
	_update_debug_shape_material();
	return debug_material;
}

const Color& ShapeCast3D::get_debug_shape_custom_color() const { return debug_shape_custom_color; }

void ShapeCast3D::_create_debug_shape()
{
	_update_debug_shape_material();

	if (!debug_instance.is_valid()) {
		debug_instance = Renderer::instance_create();
	}

	if (debug_mesh.is_null()) {
		debug_mesh.instantiate();
	}
}

void ShapeCast3D::_update_debug_shape_material(bool p_check_collision)
{
	if (debug_material.is_null()) {
		Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);
		debug_material = material;

		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
		// Use double-sided rendering so that the RayCast can be seen if the camera is inside.
		material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
		material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	}

	Color color = debug_shape_custom_color;
	if (color == Color(0.0, 0.0, 0.0)) {
		// Use the default debug shape color defined in the Project Settings.
		color = SceneTree::get_debug_collisions_color();
	}

	if (p_check_collision && collided) {
		if ((color.get_h() < 0.055 || color.get_h() > 0.945) && color.get_s() > 0.5 &&
			color.get_v() > 0.5) {
			// If base color is already quite reddish, highlight collision with green color
			color = Color(0.0, 1.0, 0.0, color.a);
		}
		else {
			// Else, highlight collision with red color
			color = Color(1.0, 0, 0, color.a);
		}
	}

	Ref<StandardMaterial3D> material = static_cast<Ref<StandardMaterial3D>>(debug_material);
}

void ShapeCast3D::_clear_debug_shape()
{
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
		debug_instance = RID();
	}
	if (debug_mesh.is_valid()) {
		Renderer::free_rid(debug_mesh->get_rid());
		debug_mesh = Ref<ArrayMesh>();
	}
}

// ==========================================================================
// Merged from scene/3d/physics/spring_arm_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
#include "spring_arm_3d.h"

void SpringArm3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!Engine::is_editor_hint()) {
			set_physics_process_internal(true);
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (!Engine::is_editor_hint()) {
			set_physics_process_internal(false);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		process_spring();
	} break;
	}
}


real_t SpringArm3D::get_length() const { return spring_length; }

void SpringArm3D::set_shape(Ref<Shape3D> p_shape) { shape = p_shape; }

Ref<Shape3D> SpringArm3D::get_shape() const { return shape; }

void SpringArm3D::set_mask(uint32_t p_mask) { mask = p_mask; }

uint32_t SpringArm3D::get_mask() { return mask; }

real_t SpringArm3D::get_margin() { return margin; }

void SpringArm3D::set_margin(real_t p_margin) { margin = p_margin; }

void SpringArm3D::add_excluded_object(RID p_rid) { excluded_objects.insert(p_rid); }

bool SpringArm3D::remove_excluded_object(RID p_rid) { return excluded_objects.erase(p_rid); }

void SpringArm3D::clear_excluded_objects() { excluded_objects.clear(); }

real_t SpringArm3D::get_hit_length() { return current_spring_length; }



void SpringArm3D::process_spring() {}
