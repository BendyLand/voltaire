/**************************************************************************/
/*  collision_object_2d.h                                                 */
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

#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "scene/2d/node_2d.h"
#include "scene/resources/2d/shape_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

// Plain non-virtual composed component for 2D collision state (shared by PhysicsBody2D and Area2D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape2D/CollisionPolygon2D owners.
struct CollisionState2D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform2D local_xform;
		Ref<Shape2D> shape;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0f;
		Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform2D xform;
		Node *owner_node = nullptr;

		struct Shape {
			Ref<Shape2D> shape;
			int index = 0;
		};

		Vector<Shape> shapes;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0f;
		Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	bool pickable = false;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS2DE::BodyMode body_mode = PS2DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	void init(bool p_area, PS2DE::BodyMode p_mode = PS2DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node2D *p_owner, int p_what);
	void apply_disabled(Node2D *p_owner);
	void apply_enabled(Node2D *p_owner);
	void update_pickable(const Node2D *p_owner);
	void set_body_mode(const Node2D *p_owner, PS2DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node2D *p_owner, DisableMode p_mode);
	void set_pickable(const Node2D *p_owner, bool p_enabled);

	// Inline shape slots (no child CollisionShape2D node required).
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform2D &p_xform);
	Transform2D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape2D / CollisionPolygon2D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform2D &p_transform);
	Transform2D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision(uint32_t p_owner, bool p_enable);
	bool is_shape_owner_one_way_collision_enabled(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision_margin(uint32_t p_owner, real_t p_margin);
	real_t get_shape_owner_one_way_collision_margin(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision_direction(uint32_t p_owner, const Vector2 &p_direction);
	Vector2 get_shape_owner_one_way_collision_direction(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, Shape2D *rp_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape2D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};
