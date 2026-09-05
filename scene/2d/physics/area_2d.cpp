/**************************************************************************/
/*  area_2d.cpp                                                           */
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

#include "area_2d.h"
#include "core/config/engine.h"
#include "servers/audio/audio_server.h"
#include "servers/physics_2d/physics_server_2d.h"

void Area2D::set_gravity_space_override_mode(SpaceOverride p_mode)
{
	gravity_space_override = p_mode;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_GRAVITY_OVERRIDE_MODE, p_mode);
}

Area2D::SpaceOverride Area2D::get_gravity_space_override_mode() const
{
	return gravity_space_override;
}

void Area2D::set_gravity_is_point(bool p_enabled)
{
	gravity_is_point = p_enabled;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_GRAVITY_IS_POINT, p_enabled);
}

bool Area2D::is_gravity_a_point() const { return gravity_is_point; }

void Area2D::set_gravity_point_unit_distance(real_t p_scale)
{
	gravity_point_unit_distance = p_scale;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_GRAVITY_POINT_UNIT_DISTANCE, p_scale);
}

real_t Area2D::get_gravity_point_unit_distance() const { return gravity_point_unit_distance; }

void Area2D::set_gravity_point_center(const Vector2& p_center)
{
	gravity_vec = p_center;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_GRAVITY_VECTOR, p_center);
}

const Vector2& Area2D::get_gravity_point_center() const { return gravity_vec; }

void Area2D::set_gravity_direction(const Vector2& p_direction)
{
	gravity_vec = p_direction;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_GRAVITY_VECTOR, p_direction);
}

const Vector2& Area2D::get_gravity_direction() const { return gravity_vec; }

void Area2D::set_gravity(real_t p_gravity)
{
	gravity = p_gravity;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_GRAVITY, p_gravity);
}

real_t Area2D::get_gravity() const { return gravity; }

void Area2D::set_linear_damp_space_override_mode(SpaceOverride p_mode)
{
	linear_damp_space_override = p_mode;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_LINEAR_DAMP_OVERRIDE_MODE, p_mode);
}

Area2D::SpaceOverride Area2D::get_linear_damp_space_override_mode() const
{
	return linear_damp_space_override;
}

void Area2D::set_angular_damp_space_override_mode(SpaceOverride p_mode)
{
	angular_damp_space_override = p_mode;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_ANGULAR_DAMP_OVERRIDE_MODE, p_mode);
}

Area2D::SpaceOverride Area2D::get_angular_damp_space_override_mode() const
{
	return angular_damp_space_override;
}

void Area2D::set_linear_damp(real_t p_linear_damp)
{
	linear_damp = p_linear_damp;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_LINEAR_DAMP, p_linear_damp);
}

real_t Area2D::get_linear_damp() const { return linear_damp; }

void Area2D::set_angular_damp(real_t p_angular_damp)
{
	angular_damp = p_angular_damp;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_ANGULAR_DAMP, p_angular_damp);
}

real_t Area2D::get_angular_damp() const { return angular_damp; }

void Area2D::set_priority(int p_priority)
{
	priority = p_priority;
	PhysicsServer2D::get_singleton()->area_set_param(
		get_rid(), PS2DE::AREA_PARAM_PRIORITY, p_priority);
}

int Area2D::get_priority() const { return priority; }

void Area2D::_body_enter_tree(ObjectID p_id)
{
	Object* obj = ObjectDB::get_instance(p_id);
	Node* node = Object::cast_to<Node>(obj);
	ERR_FAIL_NULL(node);

	HashMap<ObjectID, BodyState>::Iterator E = body_map.find(p_id);
	ERR_FAIL_COND(!E);
	ERR_FAIL_COND(E->value.in_tree);

	E->value.in_tree = true;
	this->obj->emit_signal(SceneStringName(body_entered), node);
	for (int i = 0; i < E->value.shapes.size(); i++) {
		this->obj->emit_signal(SceneStringName(body_shape_entered), E->value.rid, node,
			E->value.shapes[i].body_shape, E->value.shapes[i].area_shape);
	}
}

void Area2D::_body_exit_tree(ObjectID p_id)
{
	Object* obj = ObjectDB::get_instance(p_id);
	Node* node = Object::cast_to<Node>(obj);
	ERR_FAIL_NULL(node);
	HashMap<ObjectID, BodyState>::Iterator E = body_map.find(p_id);
	ERR_FAIL_COND(!E);
	ERR_FAIL_COND(!E->value.in_tree);
	E->value.in_tree = false;
	this->obj->emit_signal(SceneStringName(body_exited), node);
	for (int i = 0; i < E->value.shapes.size(); i++) {
		this->obj->emit_signal(SceneStringName(body_shape_exited), E->value.rid, node,
			E->value.shapes[i].body_shape, E->value.shapes[i].area_shape);
	}
}

void Area2D::_body_inout(
	int p_status, const RID& p_body, ObjectID p_instance, int p_body_shape, int p_area_shape)
{
	bool body_in = p_status == PS2DE::AREA_BODY_ADDED;
	ObjectID objid = p_instance;

	// Exit early if instance is invalid.
	if (objid.is_null()) {
		// Emit the appropriate signals.
		lock_callback();
		locked = true;
		if (body_in) {
			this->obj->emit_signal(SceneStringName(body_shape_entered), p_body, (Node*)nullptr,
				p_body_shape, p_area_shape);
		}
		else {
			this->obj->emit_signal(SceneStringName(body_shape_exited), p_body, (Node*)nullptr,
				p_body_shape, p_area_shape);
		}
		locked = false;
		unlock_callback();
		return;
	}

	Object* obj = ObjectDB::get_instance(objid);
	Node* node = Object::cast_to<Node>(obj);

	HashMap<ObjectID, BodyState>::Iterator E = body_map.find(objid);

	if (!body_in && !E) {
		return; // does not exist because it was likely removed from the tree
	}

	lock_callback();
	locked = true;

	if (body_in) {
		if (!E) {
			E = body_map.insert(objid, BodyState());
			E->value.rid = p_body;
			E->value.rc = 0;
			E->value.in_tree = node && node->is_inside_tree();
			if (node) {
				node->connect(SceneStringName(tree_entered),
					callable_mp(this, &Area2D::_body_enter_tree).bind(objid));
				node->connect(SceneStringName(tree_exiting),
					callable_mp(this, &Area2D::_body_exit_tree).bind(objid));
				if (E->value.in_tree) {
					this->obj->emit_signal(SceneStringName(body_entered), node);
				}
			}
		}
		E->value.rc++;
		if (node) {
			E->value.shapes.insert(ShapePair(p_body_shape, p_area_shape));
		}

		if (!node || E->value.in_tree) {
			this->obj->emit_signal(
				SceneStringName(body_shape_entered), p_body, node, p_body_shape, p_area_shape);
		}

	}
	else {
		E->value.rc--;

		if (node) {
			E->value.shapes.erase(ShapePair(p_body_shape, p_area_shape));
		}

		bool in_tree = E->value.in_tree;
		if (E->value.rc == 0) {
			body_map.remove(E);
			if (node) {
				node->disconnect(
					SceneStringName(tree_entered), callable_mp(this, &Area2D::_body_enter_tree));
				node->disconnect(
					SceneStringName(tree_exiting), callable_mp(this, &Area2D::_body_exit_tree));
				if (in_tree) {
					this->obj->emit_signal(SceneStringName(body_exited), obj);
				}
			}
		}
		if (!node || in_tree) {
			this->obj->emit_signal(
				SceneStringName(body_shape_exited), p_body, obj, p_body_shape, p_area_shape);
		}
	}

	locked = false;
	unlock_callback();
}

void Area2D::_area_enter_tree(ObjectID p_id)
{
	Object* obj = ObjectDB::get_instance(p_id);
	Node* node = Object::cast_to<Node>(obj);
	ERR_FAIL_NULL(node);

	HashMap<ObjectID, AreaState>::Iterator E = area_map.find(p_id);
	ERR_FAIL_COND(!E);
	ERR_FAIL_COND(E->value.in_tree);

	E->value.in_tree = true;
	this->obj->emit_signal(SceneStringName(area_entered), node);
	for (int i = 0; i < E->value.shapes.size(); i++) {
		this->obj->emit_signal(SceneStringName(area_shape_entered), E->value.rid, node,
			E->value.shapes[i].area_shape, E->value.shapes[i].self_shape);
	}
}

void Area2D::_area_exit_tree(ObjectID p_id)
{
	Object* obj = ObjectDB::get_instance(p_id);
	Node* node = Object::cast_to<Node>(obj);
	ERR_FAIL_NULL(node);
	HashMap<ObjectID, AreaState>::Iterator E = area_map.find(p_id);
	ERR_FAIL_COND(!E);
	ERR_FAIL_COND(!E->value.in_tree);
	E->value.in_tree = false;
	this->obj->emit_signal(SceneStringName(area_exited), node);
	for (int i = 0; i < E->value.shapes.size(); i++) {
		this->obj->emit_signal(SceneStringName(area_shape_exited), E->value.rid, node,
			E->value.shapes[i].area_shape, E->value.shapes[i].self_shape);
	}
}

void Area2D::_area_inout(
	int p_status, const RID& p_area, ObjectID p_instance, int p_area_shape, int p_self_shape)
{
	bool area_in = p_status == PS2DE::AREA_BODY_ADDED;
	ObjectID objid = p_instance;

	// Exit early if instance is invalid.
	if (objid.is_null()) {
		// Emit the appropriate signals.
		lock_callback();
		locked = true;
		if (area_in) {
			this->obj->emit_signal(SceneStringName(area_shape_entered), p_area, (Node*)nullptr,
				p_area_shape, p_self_shape);
		}
		else {
			this->obj->emit_signal(SceneStringName(area_shape_exited), p_area, (Node*)nullptr,
				p_area_shape, p_self_shape);
		}
		locked = false;
		unlock_callback();
		return;
	}

	Object* obj = ObjectDB::get_instance(objid);
	Node* node = Object::cast_to<Node>(obj);

	HashMap<ObjectID, AreaState>::Iterator E = area_map.find(objid);

	if (!area_in && !E) {
		return; // likely removed from the tree
	}

	lock_callback();
	locked = true;

	if (area_in) {
		if (!E) {
			E = area_map.insert(objid, AreaState());
			E->value.rid = p_area;
			E->value.rc = 0;
			E->value.in_tree = node && node->is_inside_tree();
			if (node) {
				node->connect(SceneStringName(tree_entered),
					callable_mp(this, &Area2D::_area_enter_tree).bind(objid));
				node->connect(SceneStringName(tree_exiting),
					callable_mp(this, &Area2D::_area_exit_tree).bind(objid));
				if (E->value.in_tree) {
					this->obj->emit_signal(SceneStringName(area_entered), node);
				}
			}
		}
		E->value.rc++;
		if (node) {
			E->value.shapes.insert(AreaShapePair(p_area_shape, p_self_shape));
		}

		if (!node || E->value.in_tree) {
			this->obj->emit_signal(
				SceneStringName(area_shape_entered), p_area, node, p_area_shape, p_self_shape);
		}

	}
	else {
		E->value.rc--;

		if (node) {
			E->value.shapes.erase(AreaShapePair(p_area_shape, p_self_shape));
		}

		bool in_tree = E->value.in_tree;
		if (E->value.rc == 0) {
			area_map.remove(E);
			if (node) {
				node->disconnect(
					SceneStringName(tree_entered), callable_mp(this, &Area2D::_area_enter_tree));
				node->disconnect(
					SceneStringName(tree_exiting), callable_mp(this, &Area2D::_area_exit_tree));
				if (in_tree) {
					this->obj->emit_signal(SceneStringName(area_exited), obj);
				}
			}
		}
		if (!node || in_tree) {
			this->obj->emit_signal(
				SceneStringName(area_shape_exited), p_area, obj, p_area_shape, p_self_shape);
		}
	}

	locked = false;
	unlock_callback();
}

void Area2D::_clear_monitoring()
{
	ERR_FAIL_COND_MSG(locked, "This function can't be used during the in/out signal.");

	{
		HashMap<ObjectID, BodyState> bmcopy(body_map);
		body_map.clear();
		// disconnect all monitored stuff

		for (const KeyValue<ObjectID, BodyState>& E : bmcopy) {
			Object* obj = ObjectDB::get_instance(E.key);
			Node* node = Object::cast_to<Node>(obj);

			if (!node) { // node may have been deleted in previous frame or at other legitimate
						 // point
				continue;
			}

			node->disconnect(
				SceneStringName(tree_entered), callable_mp(this, &Area2D::_body_enter_tree));
			node->disconnect(
				SceneStringName(tree_exiting), callable_mp(this, &Area2D::_body_exit_tree));

			if (!E.value.in_tree) {
				continue;
			}

			for (int i = 0; i < E.value.shapes.size(); i++) {
				this->obj->emit_signal(SceneStringName(body_shape_exited), E.value.rid, node,
					E.value.shapes[i].body_shape, E.value.shapes[i].area_shape);
			}

			this->obj->emit_signal(SceneStringName(body_exited), obj);
		}
	}

	{
		HashMap<ObjectID, AreaState> bmcopy(area_map);
		area_map.clear();
		// disconnect all monitored stuff

		for (const KeyValue<ObjectID, AreaState>& E : bmcopy) {
			Object* obj = ObjectDB::get_instance(E.key);
			Node* node = Object::cast_to<Node>(obj);

			if (!node) { // node may have been deleted in previous frame or at other legitimate
						 // point
				continue;
			}

			node->disconnect(
				SceneStringName(tree_entered), callable_mp(this, &Area2D::_area_enter_tree));
			node->disconnect(
				SceneStringName(tree_exiting), callable_mp(this, &Area2D::_area_exit_tree));

			if (!E.value.in_tree) {
				continue;
			}

			for (int i = 0; i < E.value.shapes.size(); i++) {
				this->obj->emit_signal(SceneStringName(area_shape_exited), E.value.rid, node,
					E.value.shapes[i].area_shape, E.value.shapes[i].self_shape);
			}

			this->obj->emit_signal(SceneStringName(area_exited), obj);
		}
	}
}

void Area2D::_space_changed(const RID& p_new_space)
{
	if (p_new_space.is_null()) {
		_clear_monitoring();
	}
}

void Area2D::set_monitoring(bool p_enable)
{
	if (p_enable == monitoring) {
		return;
	}
	ERR_FAIL_COND_MSG(locked,
		"Function blocked during in/out signal. Use set_deferred(\"monitoring\", true/false).");

	monitoring = p_enable;

	if (monitoring) {
		PhysicsServer2D::get_singleton()->area_set_monitor_callback(
			get_rid(), callable_mp(this, &Area2D::_body_inout));
		PhysicsServer2D::get_singleton()->area_set_area_monitor_callback(
			get_rid(), callable_mp(this, &Area2D::_area_inout));
	}
	else {
		PhysicsServer2D::get_singleton()->area_set_monitor_callback(get_rid(), Callable());
		PhysicsServer2D::get_singleton()->area_set_area_monitor_callback(get_rid(), Callable());
		_clear_monitoring();
	}
}

bool Area2D::is_monitoring() const { return monitoring; }

void Area2D::set_monitorable(bool p_enable)
{
	ERR_FAIL_COND_MSG(
		locked || (is_inside_tree() && PhysicsServer2D::get_singleton()->is_flushing_queries()),
		"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");

	if (p_enable == monitorable) {
		return;
	}

	monitorable = p_enable;

	PhysicsServer2D::get_singleton()->area_set_monitorable(get_rid(), monitorable);
}

bool Area2D::is_monitorable() const { return monitorable; }

Array Area2D::get_overlapping_bodies() const
{
	Array ret;
	ERR_FAIL_COND_V_MSG(!monitoring, ret, "Can't find overlapping bodies when monitoring is off.");
	ret.resize(body_map.size());
	int idx = 0;
	for (const KeyValue<ObjectID, BodyState>& E : body_map) {
		Object* obj = ObjectDB::get_instance(E.key);
		if (obj) {
			ret[idx] = obj;
			idx++;
		}
	}

	ret.resize(idx);
	return ret;
}

Array Area2D::get_overlapping_areas() const
{
	Array ret;
	ERR_FAIL_COND_V_MSG(!monitoring, ret, "Can't find overlapping areas when monitoring is off.");
	ret.resize(area_map.size());
	int idx = 0;
	for (const KeyValue<ObjectID, AreaState>& E : area_map) {
		Object* obj = ObjectDB::get_instance(E.key);
		if (obj) {
			ret[idx] = obj;
			idx++;
		}
	}

	ret.resize(idx);
	return ret;
}

bool Area2D::has_overlapping_bodies() const
{
	ERR_FAIL_COND_V_MSG(
		!monitoring, false, "Can't find overlapping bodies when monitoring is off.");
	return !body_map.is_empty();
}

bool Area2D::has_overlapping_areas() const
{
	ERR_FAIL_COND_V_MSG(!monitoring, false, "Can't find overlapping areas when monitoring is off.");
	return !area_map.is_empty();
}

bool Area2D::overlaps_area(Node* rp_area) const
{
	HashMap<ObjectID, AreaState>::ConstIterator E = area_map.find(rp_area->obj->get_instance_id());
	if (!E) {
		return false;
	}
	return E->value.in_tree;
}

bool Area2D::overlaps_body(Node* rp_body) const
{
	HashMap<ObjectID, BodyState>::ConstIterator E = body_map.find(rp_body->obj->get_instance_id());
	if (!E) {
		return false;
	}
	return E->value.in_tree;
}

void Area2D::set_audio_bus_override(bool p_override) { audio_bus_override = p_override; }

bool Area2D::is_overriding_audio_bus() const { return audio_bus_override; }

void Area2D::set_audio_bus_name(const StringName& p_audio_bus) { audio_bus = p_audio_bus; }

StringName Area2D::get_audio_bus_name() const
{
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
		if (AudioServer::get_singleton()->get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneStringName(Master);
}

void Area2D::_validate_property(PropertyInfo& p_property) const
{
	if (!Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (p_property.name == "audio_bus_name") {
		String options;
		for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
			if (i > 0) {
				options += ",";
			}
			String name = AudioServer::get_singleton()->get_bus_name(i);
			options += name;
		}

		p_property.hint_string = options;
	}
	else if (p_property.name.begins_with("gravity") &&
			   p_property.name != "gravity_space_override") {
		if (gravity_space_override == SPACE_OVERRIDE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		else {
			if (gravity_is_point) {
				if (p_property.name == "gravity_direction") {
					p_property.usage = PROPERTY_USAGE_NO_EDITOR;
				}
			}
			else {
				if (p_property.name.begins_with("gravity_point_")) {
					p_property.usage = PROPERTY_USAGE_NO_EDITOR;
				}
			}
		}
	}
	else if (p_property.name.begins_with("linear_damp") &&
			   p_property.name != "linear_damp_space_override") {
		if (linear_damp_space_override == SPACE_OVERRIDE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
	}
	else if (p_property.name.begins_with("angular_damp") &&
			   p_property.name != "angular_damp_space_override") {
		if (angular_damp_space_override == SPACE_OVERRIDE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
	}
}

void Area2D::_bind_methods() {}

Area2D::Area2D() : CollisionObject2D(PhysicsServer2D::get_singleton()->area_create(), true)
{
	this->obj->_define_ancestry(Object::AncestralClass::AREA_2D);

	set_gravity(980);
	set_gravity_direction(Vector2(0, 1));
	set_monitoring(true);
	set_monitorable(true);
	set_hide_clip_children(true);
}

Area2D::~Area2D() {}


