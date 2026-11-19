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
