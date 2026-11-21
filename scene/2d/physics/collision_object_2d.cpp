/**************************************************************************/
/*  collision_object_2d.cpp                                               */
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

#include "collision_object_2d.h"

#include "scene/resources/world_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

void CollisionState2D::init(bool p_area, PS2DE::BodyMode p_mode) {
	area = p_area;
	body_mode = p_mode;
	if (area) {
		rid = PhysicsServer2D::area_create();
	} else {
		rid = PhysicsServer2D::body_create();
		PhysicsServer2D::body_set_mode(rid, body_mode);
	}
}

void CollisionState2D::free_rid() {
	if (rid.is_valid() && PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::free_rid(rid);
	}
	rid = RID();
}

void CollisionState2D::handle_notification(Node2D *p_owner, int p_what) {
	switch (p_what) {
		case Node::NOTIFICATION_ENTER_TREE: {
			if (area) {
				PhysicsServer2D::area_set_transform(rid, p_owner->get_global_transform());
			}
			if (p_owner->is_enabled()) {
				RID space = p_owner->get_world_2d()->get_space();
				if (area) {
					PhysicsServer2D::area_set_space(rid, space);
				} else {
					PhysicsServer2D::body_set_space(rid, space);
				}
			}
			update_pickable(p_owner);
		} break;
		case CanvasItem::NOTIFICATION_TRANSFORM_CHANGED: {
			if (!only_update_transform_changes && area) {
				PhysicsServer2D::area_set_transform(rid, p_owner->get_global_transform());
			}
		} break;
		case CanvasItem::NOTIFICATION_VISIBILITY_CHANGED: {
			update_pickable(p_owner);
		} break;
		case Node::NOTIFICATION_EXIT_TREE: {
			if (callback_lock == 0) {
				if (area) {
					PhysicsServer2D::area_set_space(rid, RID());
				} else {
					PhysicsServer2D::body_set_space(rid, RID());
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

void CollisionState2D::set_collision_layer(uint32_t p_layer) {
	collision_layer = p_layer;
	if (area) {
		PhysicsServer2D::area_set_collision_layer(rid, p_layer);
	} else {
		PhysicsServer2D::body_set_collision_layer(rid, p_layer);
	}
}

void CollisionState2D::set_collision_mask(uint32_t p_mask) {
	collision_mask = p_mask;
	if (area) {
		PhysicsServer2D::area_set_collision_mask(rid, p_mask);
	} else {
		PhysicsServer2D::body_set_collision_mask(rid, p_mask);
	}
}

void CollisionState2D::set_collision_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t layer_new = collision_layer;
	if (p_value) {
		layer_new |= 1 << (p_layer_number - 1);
	} else {
		layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(layer_new);
}

bool CollisionState2D::get_collision_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_layer & (1 << (p_layer_number - 1));
}

void CollisionState2D::set_collision_mask_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1 || p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = collision_mask;
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	} else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool CollisionState2D::get_collision_mask_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return collision_mask & (1 << (p_layer_number - 1));
}

void CollisionState2D::set_collision_priority(real_t p_priority) {
	collision_priority = p_priority;
	if (!area) {
		PhysicsServer2D::body_set_collision_priority(rid, p_priority);
	}
}

void CollisionState2D::set_disable_mode(Node2D *p_owner, DisableMode p_mode) {
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

void CollisionState2D::apply_disabled(Node2D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				if (callback_lock > 0) {
					ERR_PRINT("Disabling a collision node during a physics callback is not allowed.");
				} else {
					if (area) {
						PhysicsServer2D::area_set_space(rid, RID());
					} else {
						PhysicsServer2D::body_set_space(rid, RID());
					}
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS2DE::BODY_MODE_STATIC)) {
				PhysicsServer2D::body_set_mode(rid, PS2DE::BODY_MODE_STATIC);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState2D::apply_enabled(Node2D *p_owner) {
	switch (disable_mode) {
		case DISABLE_MODE_REMOVE: {
			if (p_owner->is_inside_tree()) {
				RID space = p_owner->get_world_2d()->get_space();
				if (area) {
					PhysicsServer2D::area_set_space(rid, space);
				} else {
					PhysicsServer2D::body_set_space(rid, space);
				}
			}
		} break;
		case DISABLE_MODE_MAKE_STATIC: {
			if (!area && (body_mode != PS2DE::BODY_MODE_STATIC)) {
				PhysicsServer2D::body_set_mode(rid, body_mode);
			}
		} break;
		case DISABLE_MODE_KEEP_ACTIVE: {
		} break;
	}
}

void CollisionState2D::set_body_mode(const Node2D *p_owner, PS2DE::BodyMode p_mode) {
	ERR_FAIL_COND(area);
	if (body_mode == p_mode) {
		return;
	}
	body_mode = p_mode;
	if (p_owner->is_inside_tree() && !p_owner->is_enabled() && (disable_mode == DISABLE_MODE_MAKE_STATIC)) {
		return;
	}
	PhysicsServer2D::body_set_mode(rid, p_mode);
}

void CollisionState2D::set_pickable(const Node2D *p_owner, bool p_enabled) {
	if (pickable == p_enabled) {
		return;
	}
	pickable = p_enabled;
	update_pickable(p_owner);
}

void CollisionState2D::update_pickable(const Node2D *p_owner) {
	if (!p_owner->is_inside_tree()) {
		return;
	}
	bool is_pick = pickable && p_owner->is_visible_in_tree();
	if (area) {
		PhysicsServer2D::area_set_pickable(rid, is_pick);
	} else {
		PhysicsServer2D::body_set_pickable(rid, is_pick);
	}
}

// Inline shape slots.

int CollisionState2D::add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform, bool p_disabled) {
	ShapeSlot slot;
	slot.local_xform = p_xform;
	slot.shape = p_shape;
	slot.disabled = p_disabled;
	slot.owner_id = create_shape_owner();
	shape_owner_set_transform(slot.owner_id, p_xform);
	shape_owner_set_disabled(slot.owner_id, p_disabled);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, const_cast<Shape2D *>(p_shape.ptr()));
	}
	int idx = (int)inline_shapes.size();
	inline_shapes.push_back(slot);
	return idx;
}

void CollisionState2D::set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	ShapeSlot &slot = inline_shapes[p_index];
	slot.shape = p_shape;
	shape_owner_clear_shapes(slot.owner_id);
	if (p_shape.is_valid()) {
		shape_owner_add_shape(slot.owner_id, const_cast<Shape2D *>(p_shape.ptr()));
	}
}

Ref<Shape2D> CollisionState2D::get_inline_shape(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Ref<Shape2D>());
	return inline_shapes[p_index].shape;
}

void CollisionState2D::set_inline_shape_transform(int p_index, const Transform2D &p_xform) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].local_xform = p_xform;
	shape_owner_set_transform(inline_shapes[p_index].owner_id, p_xform);
}

Transform2D CollisionState2D::get_inline_shape_transform(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), Transform2D());
	return inline_shapes[p_index].local_xform;
}

void CollisionState2D::set_inline_shape_disabled(int p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	inline_shapes[p_index].disabled = p_disabled;
	shape_owner_set_disabled(inline_shapes[p_index].owner_id, p_disabled);
}

bool CollisionState2D::is_inline_shape_disabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)inline_shapes.size(), false);
	return inline_shapes[p_index].disabled;
}

void CollisionState2D::remove_inline_shape(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)inline_shapes.size());
	remove_shape_owner(inline_shapes[p_index].owner_id);
	inline_shapes.remove_at(p_index);
}

void CollisionState2D::clear_inline_shapes() {
	for (uint32_t i = 0; i < inline_shapes.size(); i++) {
		remove_shape_owner(inline_shapes[i].owner_id);
	}
	inline_shapes.clear();
}

// Shape owners.

uint32_t CollisionState2D::create_shape_owner(Node *p_owner) {
	uint32_t id = next_owner_id++;
	ShapeData sd;
	sd.owner_node = p_owner;
	shapes[id] = sd;
	return id;
}

void CollisionState2D::remove_shape_owner(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	shape_owner_clear_shapes(p_owner);
	shapes.erase(p_owner);
}

void CollisionState2D::get_shape_owners(List<uint32_t> *r_owners) const {
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		r_owners->push_back(E.key);
	}
}

PackedInt32Array CollisionState2D::get_shape_owners_array() const {
	PackedInt32Array ret;
	for (const KeyValue<uint32_t, ShapeData> &E : shapes) {
		ret.push_back(E.key);
	}
	return ret;
}

void CollisionState2D::shape_owner_set_transform(uint32_t p_owner, const Transform2D &p_transform) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.xform = p_transform;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer2D::area_set_shape_transform(rid, sd.shapes[i].index, sd.xform);
		} else {
			PhysicsServer2D::body_set_shape_transform(rid, sd.shapes[i].index, sd.xform);
		}
	}
}

Transform2D CollisionState2D::shape_owner_get_transform(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Transform2D());
	return shapes[p_owner].xform;
}

void CollisionState2D::shape_owner_set_disabled(uint32_t p_owner, bool p_disabled) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.disabled = p_disabled;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer2D::area_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		} else {
			PhysicsServer2D::body_set_shape_disabled(rid, sd.shapes[i].index, p_disabled);
		}
	}
}

bool CollisionState2D::is_shape_owner_disabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].disabled;
}

void CollisionState2D::shape_owner_set_one_way_collision(uint32_t p_owner, bool p_enable) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision = p_enable;
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

bool CollisionState2D::is_shape_owner_one_way_collision_enabled(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);
	return shapes[p_owner].one_way_collision;
}

void CollisionState2D::shape_owner_set_one_way_collision_margin(uint32_t p_owner, real_t p_margin) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision_margin = p_margin;
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

real_t CollisionState2D::get_shape_owner_one_way_collision_margin(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].one_way_collision_margin;
}

void CollisionState2D::shape_owner_set_one_way_collision_direction(uint32_t p_owner, const Vector2 &p_direction) {
	if (area) {
		return;
	}
	ERR_FAIL_COND(!shapes.has(p_owner));
	ShapeData &sd = shapes[p_owner];
	sd.one_way_collision_direction = p_direction.normalized();
	for (int i = 0; i < sd.shapes.size(); i++) {
		PhysicsServer2D::body_set_shape_as_one_way_collision(rid, sd.shapes[i].index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
	}
}

Vector2 CollisionState2D::get_shape_owner_one_way_collision_direction(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Vector2());
	return shapes[p_owner].one_way_collision_direction;
}

void CollisionState2D::shape_owner_add_shape(uint32_t p_owner, Shape2D *rp_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_NULL(rp_shape);
	ShapeData &sd = shapes[p_owner];
	ShapeData::Shape s;
	s.index = total_subshapes;
	s.shape = rp_shape;
	if (area) {
		PhysicsServer2D::area_add_shape(rid, rp_shape->get_rid(), sd.xform, sd.disabled);
	} else {
		PhysicsServer2D::body_add_shape(rid, rp_shape->get_rid(), sd.xform, sd.disabled);
		if (sd.one_way_collision) {
			PhysicsServer2D::body_set_shape_as_one_way_collision(rid, s.index, sd.one_way_collision, sd.one_way_collision_margin, sd.one_way_collision_direction);
		}
	}
	sd.shapes.push_back(s);
	total_subshapes++;
}

int CollisionState2D::shape_owner_get_shape_count(uint32_t p_owner) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);
	return shapes[p_owner].shapes.size();
}

Ref<Shape2D> CollisionState2D::shape_owner_get_shape(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), Ref<Shape2D>());
	return shapes[p_owner].shapes[p_shape].shape;
}

int CollisionState2D::shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const {
	ERR_FAIL_COND_V(!shapes.has(p_owner), -1);
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), -1);
	return shapes[p_owner].shapes[p_shape].index;
}

void CollisionState2D::shape_owner_remove_shape(uint32_t p_owner, int p_shape) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	ERR_FAIL_INDEX(p_shape, shapes[p_owner].shapes.size());

	int index_to_remove = shapes[p_owner].shapes[p_shape].index;
	if (area) {
		PhysicsServer2D::area_remove_shape(rid, index_to_remove);
	} else {
		PhysicsServer2D::body_remove_shape(rid, index_to_remove);
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

void CollisionState2D::shape_owner_clear_shapes(uint32_t p_owner) {
	ERR_FAIL_COND(!shapes.has(p_owner));
	while (shape_owner_get_shape_count(p_owner) > 0) {
		shape_owner_remove_shape(p_owner, 0);
	}
}

uint32_t CollisionState2D::shape_find_owner(int p_shape_index) const {
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
// Merged from scene/2d/physics/area_2d.cpp
// ==========================================================================

#include "area_2d.h"

#include "core/config/engine.h"
#include "servers/audio/audio_server.h"
#include "servers/physics_2d/physics_server_2d.h"

Area2D::Area2D() {
	collision_state.init(true);
	set_notify_transform(true);
	set_monitoring(true);
	set_monitorable(true);
}

Area2D::~Area2D() {
	collision_state.free_rid();
}

void Area2D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);
	if (p_what == NOTIFICATION_EXIT_TREE || p_what == NOTIFICATION_DISABLED) {
		_clear_monitoring();
	}
}

void Area2D::set_gravity_space_override_mode(SpaceOverride p_mode) {
	gravity_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_gravity_space_override_mode() const {
	return gravity_space_override;
}

void Area2D::set_gravity_is_point(bool p_enabled) {
	gravity_is_point = p_enabled;
}

bool Area2D::is_gravity_a_point() const {
	return gravity_is_point;
}

void Area2D::set_gravity_point_unit_distance(real_t p_scale) {
	gravity_point_unit_distance = p_scale;
}

real_t Area2D::get_gravity_point_unit_distance() const {
	return gravity_point_unit_distance;
}

void Area2D::set_gravity_point_center(const Vector2 &p_center) {
	gravity_vec = p_center;
}

const Vector2 &Area2D::get_gravity_point_center() const {
	return gravity_vec;
}

void Area2D::set_gravity_direction(const Vector2 &p_direction) {
	gravity_vec = p_direction;
}

const Vector2 &Area2D::get_gravity_direction() const {
	return gravity_vec;
}

void Area2D::set_gravity(real_t p_gravity) {
	gravity = p_gravity;
}

real_t Area2D::get_gravity() const {
	return gravity;
}

void Area2D::set_linear_damp_space_override_mode(SpaceOverride p_mode) {
	linear_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_linear_damp_space_override_mode() const {
	return linear_damp_space_override;
}

void Area2D::set_angular_damp_space_override_mode(SpaceOverride p_mode) {
	angular_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_angular_damp_space_override_mode() const {
	return angular_damp_space_override;
}

void Area2D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

real_t Area2D::get_linear_damp() const {
	return linear_damp;
}

void Area2D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

real_t Area2D::get_angular_damp() const {
	return angular_damp;
}

void Area2D::set_priority(int p_priority) {
	priority = p_priority;
}

int Area2D::get_priority() const {
	return priority;
}

void Area2D::set_monitoring(bool p_enable) {
	if (p_enable == monitoring) {
		return;
	}
	monitoring = p_enable;
	if (!monitoring) {
		_clear_monitoring();
	}
}

bool Area2D::is_monitoring() const {
	return monitoring;
}

void Area2D::set_monitorable(bool p_enable) {
	ERR_FAIL_COND_MSG(locked || (is_inside_tree() && PhysicsServer2D::is_flushing_queries()),
			"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");
	if (p_enable == monitorable) {
		return;
	}
	monitorable = p_enable;
	PhysicsServer2D::area_set_monitorable(get_rid(), monitorable);
}

bool Area2D::is_monitorable() const {
	return monitorable;
}

bool Area2D::has_overlapping_bodies() const {
	return false;
}

bool Area2D::has_overlapping_areas() const {
	return false;
}

bool Area2D::overlaps_area(Node *rp_area) const {
	return false;
}

bool Area2D::overlaps_body(Node *rp_body) const {
	return false;
}

void Area2D::set_audio_bus_override(bool p_override) {
	audio_bus_override = p_override;
}

bool Area2D::is_overriding_audio_bus() const {
	return audio_bus_override;
}

void Area2D::set_audio_bus_name(const String &p_audio_bus) {
	audio_bus = p_audio_bus;
}

String Area2D::get_audio_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneString(Master);
}

void Area2D::_clear_monitoring() {}

PackedStringArray Area2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape2D / CollisionPolygon2D child."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/2d/physics/collision_shape_2d.cpp
// ==========================================================================

#include "collision_shape_2d.h"

#include "core/config/engine.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"

void CollisionShape2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionShape2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape.ptr());
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

void CollisionShape2D::_shape_changed() {
	queue_redraw();
}

void CollisionShape2D::set_shape(const Ref<Shape2D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape.ptr());
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

Ref<Shape2D> CollisionShape2D::get_shape() const {
	return shape;
}

bool CollisionShape2D::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	if (shape.is_null()) {
		return false;
	}
	return shape->_edit_is_selected_on_click(p_point, p_tolerance);
}

void CollisionShape2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape2D::is_disabled() const {
	return disabled;
}

void CollisionShape2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionShape2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionShape2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionShape2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionShape2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionShape2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

Color CollisionShape2D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape2D::set_debug_color(const Color &p_color) {
	debug_color = p_color;
	queue_redraw();
}

Color CollisionShape2D::get_debug_color() const {
	return debug_color;
}

#ifdef DEBUG_ENABLED
bool CollisionShape2D::_property_can_revert(const String &p_name) const {
	return p_name == "debug_color";
}
#endif // DEBUG_ENABLED

CollisionShape2D::CollisionShape2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionShape2D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/2d/physics/collision_polygon_2d.cpp
// ==========================================================================

#include "collision_polygon_2d.h"

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/shape_2d.h"

void CollisionPolygon2D::_build_polygon() {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_clear_shapes(owner_id);

	if (build_mode == BUILD_SOLIDS) {
		if (polygon.size() < 3) {
			return;
		}
		Vector<Vector<Vector2>> decomp = _decompose_in_convex();
		for (int i = 0; i < decomp.size(); i++) {
			Ref<Shape2D> convex = memnew(Shape2D(Shape2D::SHAPE_CONVEX_POLYGON));
			convex->set_points(decomp[i]);
			collision_state->shape_owner_add_shape(owner_id, convex.ptr());
		}
	} else {
		if (polygon.size() < 2) {
			return;
		}
		Ref<Shape2D> concave = memnew(Shape2D(Shape2D::SHAPE_CONCAVE_POLYGON));
		Vector<Vector2> segments;
		segments.resize(polygon.size() * 2);
		Vector2 *w = segments.ptrw();
		for (int i = 0; i < polygon.size(); i++) {
			w[(i << 1) + 0] = polygon[i];
			w[(i << 1) + 1] = polygon[(i + 1) % polygon.size()];
		}
		concave->set_segments(segments);
		collision_state->shape_owner_add_shape(owner_id, concave.ptr());
	}
}

Vector<Vector<Vector2>> CollisionPolygon2D::_decompose_in_convex() {
	return Geometry2D::decompose_polygon_in_convex(polygon);
}

void CollisionPolygon2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionPolygon2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				_build_polygon();
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

void CollisionPolygon2D::set_polygon(const Vector<Point2> &p_polygon) {
	polygon = p_polygon;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

Vector<Point2> CollisionPolygon2D::get_polygon() const {
	return polygon;
}

void CollisionPolygon2D::set_build_mode(BuildMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 2);
	build_mode = p_mode;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

CollisionPolygon2D::BuildMode CollisionPolygon2D::get_build_mode() const {
	return build_mode;
}

#ifdef DEBUG_ENABLED
Rect2 CollisionPolygon2D::_edit_get_rect() const {
	return aabb;
}

bool CollisionPolygon2D::_edit_use_rect() const {
	return true;
}

bool CollisionPolygon2D::_edit_is_selected_on_click(const Vector2 &p_point, double p_tolerance) const {
	return false;
}
#endif

void CollisionPolygon2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon2D::is_disabled() const {
	return disabled;
}

void CollisionPolygon2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionPolygon2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionPolygon2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionPolygon2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionPolygon2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionPolygon2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

CollisionPolygon2D::CollisionPolygon2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
}

PackedStringArray CollisionPolygon2D::get_configuration_warnings() const {
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/2d/physics/ray_cast_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "ray_cast_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

Vector2 RayCast2D::get_target_position() const { return target_position; }

void RayCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t RayCast2D::get_collision_mask() const { return collision_mask; }

void RayCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool RayCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

bool RayCast2D::is_colliding() const { return collided; }

RID RayCast2D::get_collider_rid() const { return against_rid; }

int RayCast2D::get_collider_shape() const { return against_shape; }

Vector2 RayCast2D::get_collision_point() const { return collision_point; }

Vector2 RayCast2D::get_collision_normal() const { return collision_normal; }

bool RayCast2D::is_enabled() const { return enabled; }

bool RayCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void RayCast2D::_draw_debug_shape()
{
	Color draw_col = collided ? Color(1.0, 0.01, 0) : SceneTree::get_debug_collisions_color();
	if (!enabled) {
		const float g = draw_col.get_v();
		draw_col.r = g;
		draw_col.g = g;
		draw_col.b = g;
	}

	// Draw an arrow indicating where the RayCast is pointing to
	const real_t max_arrow_size = 6;
	const real_t line_width = 1.4;
	bool no_line = target_position.length() < line_width;
	real_t arrow_size = CLAMP(target_position.length() * 2 / 3, line_width, max_arrow_size);

	if (no_line) {
		arrow_size = target_position.length();
	}
	else {
		draw_line(Vector2(), target_position - target_position.normalized() * arrow_size, draw_col,
			line_width);
	}

	Transform2D xf;
	xf.rotate(target_position.angle());
	xf.translate_local(Vector2(no_line ? 0 : target_position.length() - arrow_size, 0));

	Vector<Vector2> pts = {xf.xform(Vector2(arrow_size, 0)), xf.xform(Vector2(0, 0.5 * arrow_size)),
		xf.xform(Vector2(0, -0.5 * arrow_size))};

	Vector<Color> cols = {draw_col, draw_col, draw_col};

	draw_primitive(pts, cols, Vector<Vector2>());
}

void RayCast2D::force_raycast_update() { _update_raycast_state(); }

void RayCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void RayCast2D::add_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	add_exception_rid(cs->rid);
}

void RayCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void RayCast2D::remove_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	remove_exception_rid(cs->rid);
}

void RayCast2D::set_collide_with_areas(bool p_enabled) { collide_with_areas = p_enabled; }

bool RayCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void RayCast2D::set_collide_with_bodies(bool p_enabled) { collide_with_bodies = p_enabled; }

bool RayCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void RayCast2D::set_hit_from_inside(bool p_enabled) { hit_from_inside = p_enabled; }

bool RayCast2D::is_hit_from_inside_enabled() const { return hit_from_inside; }

RayCast2D::RayCast2D() { set_hide_clip_children(true); }



void RayCast2D::_update_raycast_state() {}

// ==========================================================================
// Merged from scene/2d/physics/shape_cast_2d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "shape_cast_2d.h"

Vector2 ShapeCast2D::get_target_position() const { return target_position; }

void ShapeCast2D::set_margin(real_t p_margin) { margin = p_margin; }

real_t ShapeCast2D::get_margin() const { return margin; }

void ShapeCast2D::set_max_results(int p_max_results) { max_results = p_max_results; }

int ShapeCast2D::get_max_results() const { return max_results; }

void ShapeCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t ShapeCast2D::get_collision_mask() const { return collision_mask; }

void ShapeCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool ShapeCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

int ShapeCast2D::get_collision_count() const { return result.size(); }

bool ShapeCast2D::is_colliding() const { return collided; }

RID ShapeCast2D::get_collider_rid(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), RID(), "No collider RID found.");
	return result[p_idx].rid;
}

int ShapeCast2D::get_collider_shape(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), -1, "No collider shape found.");
	return result[p_idx].shape;
}

Vector2 ShapeCast2D::get_collision_point(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision point found.");
	return result[p_idx].point;
}

Vector2 ShapeCast2D::get_collision_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision normal found.");
	return result[p_idx].normal;
}

real_t ShapeCast2D::get_closest_collision_safe_fraction() const { return collision_safe_fraction; }

real_t ShapeCast2D::get_closest_collision_unsafe_fraction() const
{
	return collision_unsafe_fraction;
}

bool ShapeCast2D::is_enabled() const { return enabled; }

Ref<Shape2D> ShapeCast2D::get_shape() const { return shape; }

bool ShapeCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void ShapeCast2D::force_shapecast_update() { _update_shapecast_state(); }

void ShapeCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void ShapeCast2D::add_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	add_exception_rid(cs->rid);
}

void ShapeCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void ShapeCast2D::remove_exception(const Node2D* rp_node)
{
	ERR_FAIL_NULL(rp_node);
	const CollisionState2D* cs = rp_node->get_collision_state_2d();
	ERR_FAIL_NULL(cs);
	remove_exception_rid(cs->rid);
}

void ShapeCast2D::clear_exceptions() { exclude.clear(); }

void ShapeCast2D::set_collide_with_areas(bool p_clip) { collide_with_areas = p_clip; }

bool ShapeCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void ShapeCast2D::set_collide_with_bodies(bool p_clip) { collide_with_bodies = p_clip; }

bool ShapeCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

PackedStringArray ShapeCast2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (shape.is_null()) {
		warnings.push_back(
			RTR("This node cannot interact with other objects unless a Shape2D is assigned."));
	}
	return warnings;
}

ShapeCast2D::ShapeCast2D() { set_hide_clip_children(true); }



void ShapeCast2D::_update_shapecast_state() {}

// ==========================================================================
// Merged from scene/2d/physics/touch_screen_button.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/input/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "touch_screen_button.h"

Ref<Texture2D> TouchScreenButton::get_texture_normal() const { return texture_normal; }

Ref<Texture2D> TouchScreenButton::get_texture_pressed() const { return texture_pressed; }

void TouchScreenButton::set_bitmask(const Ref<BitMap>& p_bitmask) { bitmask = p_bitmask; }

Ref<BitMap> TouchScreenButton::get_bitmask() const { return bitmask; }

Ref<Shape2D> TouchScreenButton::get_shape() const { return shape; }

bool TouchScreenButton::is_shape_visible() const { return shape_visible; }

bool TouchScreenButton::is_shape_centered() const { return shape_centered; }

bool TouchScreenButton::is_pressed() const { return finger_pressed != -1; }

void TouchScreenButton::set_action(const String& p_action) { action = p_action; }

String TouchScreenButton::get_action() const { return action; }

bool TouchScreenButton::_is_point_inside(const Point2& p_point)
{
	Point2 coord = (get_global_transform_with_canvas()).affine_inverse().xform(p_point);

	bool touched = false;
	bool check_rect = true;

	if (shape.is_valid()) {
		check_rect = false;

		Vector2 pos;
		if (shape_centered && texture_normal.is_valid()) {
			pos = texture_normal->get_size() * 0.5;
		}

		touched = shape->collide(Transform2D().translated_local(pos), unit_rect.ptr(),
			Transform2D(0, coord + Vector2(0.5, 0.5)));
	}

	if (bitmask.is_valid()) {
		check_rect = false;
		if (!touched && Rect2(Point2(), bitmask->get_size()).has_point(coord)) {
			if (bitmask->get_bitv(coord)) {
				touched = true;
			}
		}
	}

	if (!touched && check_rect) {
		if (texture_normal.is_valid()) {
			touched = Rect2(Size2(), texture_normal->get_size()).has_point(coord);
		}
	}

	return touched;
}

#ifdef DEBUG_ENABLED
Rect2 TouchScreenButton::_edit_get_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::_edit_get_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

bool TouchScreenButton::_edit_use_rect() const { return texture_normal.is_valid(); }
#endif // DEBUG_ENABLED

Rect2 TouchScreenButton::get_anchorable_rect() const
{
	if (texture_normal.is_null()) {
		return CanvasItem::get_anchorable_rect();
	}

	return Rect2(Size2(), texture_normal->get_size());
}

TouchScreenButton::VisibilityMode TouchScreenButton::get_visibility_mode() const
{
	return visibility;
}

void TouchScreenButton::set_passby_press(bool p_enable) { passby_press = p_enable; }

bool TouchScreenButton::is_passby_press_enabled() const { return passby_press; }

TouchScreenButton::TouchScreenButton()
{
	unit_rect = memnew(Shape2D(Shape2D::SHAPE_RECTANGLE));
	unit_rect->set_size(Vector2(1, 1));
}

void TouchScreenButton::input(const InputEvent& p_event) {}
