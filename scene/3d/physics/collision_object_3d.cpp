/**************************************************************************/
/*  collision_object_3d.cpp                                               */
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

#include "collision_object_3d.h"
#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server.h"

void CollisionObject3D::set_collision_layer(uint32_t p_layer)
{
	collision_layer = p_layer;
	if (area) {
		PhysicsServer3D::get_singleton()->area_set_collision_layer(get_rid(), p_layer);
	}
	else {
		PhysicsServer3D::get_singleton()->body_set_collision_layer(get_rid(), p_layer);
	}
}

uint32_t CollisionObject3D::get_collision_layer() const { return collision_layer; }

void CollisionObject3D::set_collision_mask(uint32_t p_mask)
{
	collision_mask = p_mask;
	if (area) {
		PhysicsServer3D::get_singleton()->area_set_collision_mask(get_rid(), p_mask);
	}
	else {
		PhysicsServer3D::get_singleton()->body_set_collision_mask(get_rid(), p_mask);
	}
}

uint32_t CollisionObject3D::get_collision_mask() const { return collision_mask; }

void CollisionObject3D::set_collision_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t collision_layer_new = get_collision_layer();
	if (p_value) {
		collision_layer_new |= 1 << (p_layer_number - 1);
	}
	else {
		collision_layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(collision_layer_new);
}

bool CollisionObject3D::get_collision_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void CollisionObject3D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool CollisionObject3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

void CollisionObject3D::set_collision_priority(real_t p_priority)
{
	collision_priority = p_priority;
	if (!area) {
		PhysicsServer3D::get_singleton()->body_set_collision_priority(get_rid(), p_priority);
	}
}

real_t CollisionObject3D::get_collision_priority() const { return collision_priority; }

void CollisionObject3D::set_disable_mode(DisableMode p_mode)
{
	if (disable_mode == p_mode) {
		return;
	}

	bool disabled = is_inside_tree() && !is_enabled();

	if (disabled) {
		// Cancel previous disable mode.
		_apply_enabled();
	}

	disable_mode = p_mode;

	if (disabled) {
		// Apply new disable mode.
		_apply_disabled();
	}
}

CollisionObject3D::DisableMode CollisionObject3D::get_disable_mode() const { return disable_mode; }

void CollisionObject3D::_apply_disabled()
{
	switch (disable_mode) {
	case DISABLE_MODE_REMOVE: {
		if (is_inside_tree()) {
			if (callback_lock > 0) {
				ERR_PRINT(
					"Disabling a CollisionObject node during a physics callback is not allowed and "
					"will cause undesired behavior. Disable with call_deferred() instead.");
			}
			else {
				if (area) {
					PhysicsServer3D::get_singleton()->area_set_space(rid, RID());
				}
				else {
					PhysicsServer3D::get_singleton()->body_set_space(rid, RID());
				}
				_space_changed(RID());
			}
		}
	} break;

	case DISABLE_MODE_MAKE_STATIC: {
		if (!area && (body_mode != PS3DE::BODY_MODE_STATIC)) {
			PhysicsServer3D::get_singleton()->body_set_mode(rid, PS3DE::BODY_MODE_STATIC);
		}
	} break;

	case DISABLE_MODE_KEEP_ACTIVE: {
		// Nothing to do.
	} break;
	}
}

void CollisionObject3D::_apply_enabled()
{
	switch (disable_mode) {
	case DISABLE_MODE_REMOVE: {
		if (is_inside_tree()) {
			RID space = get_world_3d()->get_space();
			if (area) {
				PhysicsServer3D::get_singleton()->area_set_space(rid, space);
			}
			else {
				PhysicsServer3D::get_singleton()->body_set_space(rid, space);
			}
			_space_changed(space);
		}
	} break;

	case DISABLE_MODE_MAKE_STATIC: {
		if (!area && (body_mode != PS3DE::BODY_MODE_STATIC)) {
			PhysicsServer3D::get_singleton()->body_set_mode(rid, body_mode);
		}
	} break;

	case DISABLE_MODE_KEEP_ACTIVE: {
		// Nothing to do.
	} break;
	}
}

void CollisionObject3D::set_body_mode(PS3DE::BodyMode p_mode)
{
	ERR_FAIL_COND(area);

	if (body_mode == p_mode) {
		return;
	}

	body_mode = p_mode;

	if (is_inside_tree() && !is_enabled() && (disable_mode == DISABLE_MODE_MAKE_STATIC)) {
		return;
	}

	PhysicsServer3D::get_singleton()->body_set_mode(rid, p_mode);
}

void CollisionObject3D::_space_changed(const RID& p_new_space) {}

void CollisionObject3D::set_only_update_transform_changes(bool p_enable)
{
	only_update_transform_changes = p_enable;
}

bool CollisionObject3D::is_only_update_transform_changes_enabled() const
{
	return only_update_transform_changes;
}

void CollisionObject3D::_update_pickable()
{
	if (!is_inside_tree()) {
		return;
	}

	bool pickable = ray_pickable && is_visible_in_tree();
	if (area) {
		PhysicsServer3D::get_singleton()->area_set_ray_pickable(rid, pickable);
	}
	else {
		PhysicsServer3D::get_singleton()->body_set_ray_pickable(rid, pickable);
	}
}

bool CollisionObject3D::_are_collision_shapes_visible()
{
	return is_inside_tree() && get_tree()->is_debugging_collisions_hint() &&
		   !Engine::get_singleton()->is_editor_hint();
}

void CollisionObject3D::_shape_changed(const Ref<Shape3D>& p_shape)
{
	for (KeyValue<uint32_t, ShapeData>& E : shapes) {
		ShapeData& shapedata = E.value;
		ShapeData::ShapeBase* shape_bases = shapedata.shapes.ptrw();
		for (int i = 0; i < shapedata.shapes.size(); i++) {
			ShapeData::ShapeBase& s = shape_bases[i];
			if (s.shape == p_shape && s.debug_shape.is_valid()) {
				Ref<Mesh> mesh = s.shape->get_debug_mesh();
				RS::get_singleton()->instance_set_base(s.debug_shape, mesh->get_rid());
			}
		}
	}
}

void CollisionObject3D::_on_transform_changed()
{
	if (debug_shapes_count > 0 &&
		!debug_shape_old_transform.is_equal_approx(get_global_transform())) {
		debug_shape_old_transform = get_global_transform();
		for (KeyValue<uint32_t, ShapeData>& E : shapes) {
			ShapeData& shapedata = E.value;
			if (shapedata.disabled) {
				continue; // If disabled then there are no debug shapes to update.
			}
			const ShapeData::ShapeBase* shape_bases = shapedata.shapes.ptr();
			for (int i = 0; i < shapedata.shapes.size(); i++) {
				if (shape_bases[i].debug_shape.is_null()) {
					continue;
				}
				RS::get_singleton()->instance_set_transform(
					shape_bases[i].debug_shape, debug_shape_old_transform * shapedata.xform);
			}
		}
	}
}

void CollisionObject3D::set_ray_pickable(bool p_ray_pickable)
{
	ray_pickable = p_ray_pickable;
	_update_pickable();
}

bool CollisionObject3D::is_ray_pickable() const { return ray_pickable; }

void CollisionObject3D::_bind_methods() {}

void CollisionObject3D::remove_shape_owner(uint32_t owner)
{
	ERR_FAIL_COND(!shapes.has(owner));

	shape_owner_clear_shapes(owner);

	shapes.erase(owner);
}

void CollisionObject3D::shape_owner_set_disabled(uint32_t p_owner, bool p_disabled)
{
	ERR_FAIL_COND(!shapes.has(p_owner));

	ShapeData& sd = shapes[p_owner];
	if (sd.disabled == p_disabled) {
		return;
	}
	sd.disabled = p_disabled;

	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer3D::get_singleton()->area_set_shape_disabled(
				rid, sd.shapes[i].index, p_disabled);
		}
		else {
			PhysicsServer3D::get_singleton()->body_set_shape_disabled(
				rid, sd.shapes[i].index, p_disabled);
		}
	}
	_update_shape_data(p_owner);
}

bool CollisionObject3D::is_shape_owner_disabled(uint32_t p_owner) const
{
	ERR_FAIL_COND_V(!shapes.has(p_owner), false);

	return shapes[p_owner].disabled;
}

void CollisionObject3D::get_shape_owners(List<uint32_t>* r_owners)
{
	for (const KeyValue<uint32_t, ShapeData>& E : shapes) {
		r_owners->push_back(E.key);
	}
}

PackedInt32Array CollisionObject3D::_get_shape_owners()
{
	PackedInt32Array ret;
	for (const KeyValue<uint32_t, ShapeData>& E : shapes) {
		ret.push_back(E.key);
	}

	return ret;
}

void CollisionObject3D::shape_owner_set_transform(uint32_t p_owner, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!shapes.has(p_owner));

	ShapeData& sd = shapes[p_owner];
	sd.xform = p_transform;
	for (int i = 0; i < sd.shapes.size(); i++) {
		if (area) {
			PhysicsServer3D::get_singleton()->area_set_shape_transform(
				rid, sd.shapes[i].index, p_transform);
		}
		else {
			PhysicsServer3D::get_singleton()->body_set_shape_transform(
				rid, sd.shapes[i].index, p_transform);
		}
	}

	_update_shape_data(p_owner);
}

Transform3D CollisionObject3D::shape_owner_get_transform(uint32_t p_owner) const
{
	ERR_FAIL_COND_V(!shapes.has(p_owner), Transform3D());

	return shapes[p_owner].xform;
}

void CollisionObject3D::shape_owner_add_shape(uint32_t p_owner, Shape3D* rp_shape)
{
	ERR_FAIL_COND(!shapes.has(p_owner));

	ShapeData& sd = shapes[p_owner];
	ShapeData::ShapeBase s;
	s.index = total_subshapes;
	s.shape = rp_shape;

	if (area) {
		PhysicsServer3D::get_singleton()->area_add_shape(
			rid, rp_shape->get_rid(), sd.xform, sd.disabled);
	}
	else {
		PhysicsServer3D::get_singleton()->body_add_shape(
			rid, rp_shape->get_rid(), sd.xform, sd.disabled);
	}
	sd.shapes.push_back(s);

	total_subshapes++;

	_update_shape_data(p_owner);
	update_gizmos();
}

int CollisionObject3D::shape_owner_get_shape_count(uint32_t p_owner) const
{
	ERR_FAIL_COND_V(!shapes.has(p_owner), 0);

	return shapes[p_owner].shapes.size();
}

Ref<Shape3D> CollisionObject3D::shape_owner_get_shape(uint32_t p_owner, int p_shape) const
{
	ERR_FAIL_COND_V(!shapes.has(p_owner), Ref<Shape3D>());
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), Ref<Shape3D>());

	return shapes[p_owner].shapes[p_shape].shape;
}

int CollisionObject3D::shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const
{
	ERR_FAIL_COND_V(!shapes.has(p_owner), -1);
	ERR_FAIL_INDEX_V(p_shape, shapes[p_owner].shapes.size(), -1);

	return shapes[p_owner].shapes[p_shape].index;
}

void CollisionObject3D::shape_owner_clear_shapes(uint32_t p_owner)
{
	ERR_FAIL_COND(!shapes.has(p_owner));

	while (shape_owner_get_shape_count(p_owner) > 0) {
		shape_owner_remove_shape(p_owner, 0);
	}

	update_gizmos();
}

uint32_t CollisionObject3D::shape_find_owner(int p_shape_index) const
{
	ERR_FAIL_INDEX_V(p_shape_index, total_subshapes, UINT32_MAX);

	for (const KeyValue<uint32_t, ShapeData>& E : shapes) {
		for (int i = 0; i < E.value.shapes.size(); i++) {
			if (E.value.shapes[i].index == p_shape_index) {
				return E.key;
			}
		}
	}

	// in theory it should be unreachable
	ERR_FAIL_V_MSG(UINT32_MAX, "Can't find owner for shape index " + itos(p_shape_index) + ".");
}

void CollisionObject3D::set_capture_input_on_drag(bool p_capture)
{
	capture_input_on_drag = p_capture;
}

bool CollisionObject3D::get_capture_input_on_drag() const { return capture_input_on_drag; }

PackedStringArray CollisionObject3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (shapes.is_empty()) {
		warnings.push_back(RTR(
			"This node has no shape, so it can't collide or interact with other objects.\nConsider "
			"adding a CollisionShape3D or CollisionPolygon3D as a child to define its shape."));
	}
	Vector3 scale = get_transform().get_basis().get_scale();
	if (!(Math::is_zero_approx(scale.x - scale.y) && Math::is_zero_approx(scale.y - scale.z))) {
		warnings.push_back(RTR("With a non-uniform scale this node will probably not function as "
							   "expected.\nPlease make its scale uniform (i.e. the same on all "
							   "axes), and change the size in children collision shapes instead."));
	}
	return warnings;
}

CollisionObject3D::~CollisionObject3D()
{
	ERR_FAIL_NULL(PhysicsServer3D::get_singleton());
	PhysicsServer3D::get_singleton()->free_rid(rid);
}


