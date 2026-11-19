/**************************************************************************/
/*  collision_object_3d.h                                                 */
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
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

// Plain non-virtual composed component for 3D collision state (shared by PhysicsBody3D and Area3D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape3D/CollisionPolygon3D owners.
struct CollisionState3D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform3D local_xform;
		Ref<Shape3D> shape;
		bool disabled = false;
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform3D xform;
		Node *owner_node = nullptr;

		struct ShapeBase {
			RID debug_shape;
			Ref<Shape3D> shape;
			int index = 0;
		};

		Vector<ShapeBase> shapes;
		bool disabled = false;
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS3DE::BodyMode body_mode = PS3DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;
	bool capture_input_on_drag = false;
	bool ray_pickable = true;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	int debug_shapes_count = 0;
	Transform3D debug_shape_old_transform;

	void init(bool p_area, PS3DE::BodyMode p_mode = PS3DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node3D *p_owner, int p_what);
	void on_transform_changed(const Node3D *p_owner);
	void clear_debug_shapes();

	void apply_disabled(Node3D *p_owner);
	void apply_enabled(Node3D *p_owner);
	void update_pickable(const Node3D *p_owner);
	void set_body_mode(const Node3D *p_owner, PS3DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node3D *p_owner, DisableMode p_mode);
	void set_ray_pickable(const Node3D *p_owner, bool p_ray_pickable);

	// Inline shape slots (no child CollisionShape3D node required).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform);
	Transform3D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape3D / CollisionPolygon3D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform3D &p_transform);
	Transform3D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, const Ref<Shape3D> &p_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape3D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};
