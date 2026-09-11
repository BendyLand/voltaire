/**************************************************************************/
/*  node_3d.cpp                                                           */
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

#include "core/config/engine.h"
#include "core/math/transform_interpolator.h"
#include "node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/property_utils.h"
#include "scene/resources/environment.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"

/*

 possible algorithms:

 Algorithm 1: (current)

 definition of invalidation: global is invalid

 1) If a node sets a LOCAL, it produces an invalidation of everything above
 .  a) If above is invalid, don't keep invalidating upwards
 2) If a node sets a GLOBAL, it is converted to LOCAL (and forces validation of everything pending
below)

 drawback: setting/reading globals is useful and used very often, and using affine inverses is slow

---

 Algorithm 2: (no longer current)

 definition of invalidation: NONE dirty, LOCAL dirty, GLOBAL dirty

 1) If a node sets a LOCAL, it must climb the tree and set it as GLOBAL dirty
 .  a) marking GLOBALs as dirty up all the tree must be done always
 2) If a node sets a GLOBAL, it marks local as dirty, and that's all?

 //is clearing the dirty state correct in this case?

 drawback: setting a local down the tree forces many tree walks often

--

future: no idea

 */

Node3DGizmo::Node3DGizmo() {}

void Node3D::_notify_dirty()
{
#ifdef TOOLS_ENABLED
	if ((!data.gizmos.is_empty() || data.notify_transform) && !data.ignore_notification &&
		!xform_change.in_list()) {
#else
	if (data.notify_transform && !data.ignore_notification && !xform_change.in_list()) {

#endif
		get_tree()->xform_change_list.add(&xform_change);
	}
}

void Node3D::_update_local_transform() const
{
	// This function is called when the local transform (data.local_transform) is dirty and the
	// right value is contained in the Euler rotation and scale.
	data.local_transform.basis.set_euler_scale(
		data.euler_rotation, data.scale, data.euler_rotation_order);
	_clear_dirty_bits(DIRTY_LOCAL_TRANSFORM);
}

void Node3D::_update_rotation_and_scale() const
{
	// This function is called when the Euler rotation (data.euler_rotation) is dirty and the right
	// value is contained in the local transform

	data.scale = data.local_transform.basis.get_scale();
	data.euler_rotation =
		data.local_transform.basis.get_euler_normalized(data.euler_rotation_order);
	_clear_dirty_bits(DIRTY_EULER_ROTATION_AND_SCALE);
}

void Node3D::set_basis(const Basis& p_basis)
{
	ERR_THREAD_GUARD;

	set_transform(Transform3D(p_basis, data.local_transform.origin));
}

Vector3 Node3D::get_global_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	return get_global_transform().get_origin();
}

Basis Node3D::get_global_basis() const
{
	ERR_READ_THREAD_GUARD_V(Basis());
	return get_global_transform().get_basis();
}

void Node3D::set_global_position(const Vector3& p_position)
{
	ERR_THREAD_GUARD;
	Transform3D transform = get_global_transform();
	transform.set_origin(p_position);
	set_global_transform(transform);
}

void Node3D::set_global_basis(const Basis& p_basis)
{
	ERR_THREAD_GUARD;
	Transform3D transform = get_global_transform();
	transform.set_basis(p_basis);
	set_global_transform(transform);
}

Vector3 Node3D::get_global_rotation() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	return get_global_transform().get_basis().get_euler_normalized();
}

Vector3 Node3D::get_global_rotation_degrees() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	Vector3 radians = get_global_rotation();
	return Vector3(
		Math::rad_to_deg(radians.x), Math::rad_to_deg(radians.y), Math::rad_to_deg(radians.z));
}

void Node3D::set_global_rotation(const Vector3& p_euler_rad)
{
	ERR_THREAD_GUARD;
	Transform3D transform = get_global_transform();
	transform.basis =
		Basis::from_euler(p_euler_rad) * Basis::from_scale(transform.basis.get_scale());
	set_global_transform(transform);
}

void Node3D::set_global_rotation_degrees(const Vector3& p_euler_degrees)
{
	ERR_THREAD_GUARD;
	Vector3 radians(Math::deg_to_rad(p_euler_degrees.x), Math::deg_to_rad(p_euler_degrees.y),
		Math::deg_to_rad(p_euler_degrees.z));
	set_global_rotation(radians);
}

void Node3D::fti_pump_xform() { data.local_transform_prev = get_transform(); }

void Node3D::fti_notify_node_changed(bool p_transform_changed)
{
	if (is_inside_tree()) {
		get_tree()->get_scene_tree_fti().node_3d_notify_changed(*this, p_transform_changed);
	}
}

Basis Node3D::get_basis() const
{
	ERR_READ_THREAD_GUARD_V(Basis());
	return get_transform().basis;
}

Quaternion Node3D::get_quaternion() const
{
	return get_transform().basis.get_rotation_quaternion();
}

void Node3D::set_global_transform(const Transform3D& p_transform)
{
	ERR_THREAD_GUARD;
	Transform3D xform = (data.parent && !data.top_level)
							? data.parent->get_global_transform().affine_inverse() * p_transform
							: p_transform;

	set_transform(xform);
}

Transform3D Node3D::get_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform3D());
	if (_test_dirty_bits(DIRTY_LOCAL_TRANSFORM)) {
		// This update can happen if needed over multiple threads.
		_update_local_transform();
	}

	return data.local_transform;
}

// Return false to timeout and remove from the client interpolation list.
bool Node3D::update_client_physics_interpolation_data()
{
	if (!is_inside_tree() || !_is_physics_interpolated_client_side()) {
		return false;
	}

	ERR_FAIL_NULL_V(data.client_physics_interpolation_data, false);
	ClientPhysicsInterpolationData& pid = *data.client_physics_interpolation_data;

	uint64_t tick = Engine::get_singleton()->get_physics_frames();

	// Has this update been done already this tick?
	// (For instance, get_global_transform_interpolated() could be called multiple times.)
	if (pid.current_physics_tick != tick) {
		// Timeout?
		if (tick >= pid.timeout_physics_tick) {
			return false;
		}

		if (pid.current_physics_tick == (tick - 1)) {
			// Normal interpolation situation, there is a continuous flow of data
			// from one tick to the next...
			pid.global_xform_prev = pid.global_xform_curr;
		}
		else {
			// There has been a gap, we cannot sensibly offer interpolation over
			// a multitick gap, so we will teleport.
			pid.global_xform_prev = get_global_transform();
		}
		pid.current_physics_tick = tick;
	}

	pid.global_xform_curr = get_global_transform();
	return true;
}

void Node3D::_disable_client_physics_interpolation()
{
	// Disable any current client side interpolation.
	// (This can always restart as normal if you later re-attach the node to the SceneTree.)
	if (data.client_physics_interpolation_data) {
		memdelete(data.client_physics_interpolation_data);
		data.client_physics_interpolation_data = nullptr;

		SceneTree* tree = get_tree();
		if (tree && _client_physics_interpolation_node_3d_list.in_list()) {
			tree->client_physics_interpolation_remove_node_3d(
				&_client_physics_interpolation_node_3d_list);
		}
	}
	_set_physics_interpolated_client_side(false);
}

Transform3D Node3D::_get_global_transform_interpolated(real_t p_interpolation_fraction)
{
	ERR_FAIL_COND_V(!is_inside_tree(), Transform3D());

	// Set in motion the mechanisms for client side interpolation if not already active.
	if (!_is_physics_interpolated_client_side()) {
		_set_physics_interpolated_client_side(true);

		ERR_FAIL_COND_V(data.client_physics_interpolation_data != nullptr, Transform3D());
		data.client_physics_interpolation_data = memnew(ClientPhysicsInterpolationData);
		data.client_physics_interpolation_data->global_xform_curr = get_global_transform();
		data.client_physics_interpolation_data->global_xform_prev =
			data.client_physics_interpolation_data->global_xform_curr;
		data.client_physics_interpolation_data->current_physics_tick =
			Engine::get_singleton()->get_physics_frames();
	}

	// Storing the last tick we requested client interpolation allows us to timeout
	// and remove client interpolated nodes from the list to save processing.
	// We use some arbitrary timeout here, but this could potentially be user defined.

	// Note: This timeout has to be larger than the number of ticks in a frame, otherwise the
	// interpolated data will stop flowing before the next frame is drawn. This should only be
	// relevant at high tick rates. We could alternatively do this by frames rather than ticks and
	// avoid this problem, but then the behavior would be machine dependent.
	data.client_physics_interpolation_data->timeout_physics_tick =
		Engine::get_singleton()->get_physics_frames() + 256;

	// Make sure data is up to date.
	update_client_physics_interpolation_data();

	// Interpolate the current data.
	const Transform3D& xform_curr = data.client_physics_interpolation_data->global_xform_curr;
	const Transform3D& xform_prev = data.client_physics_interpolation_data->global_xform_prev;

	Transform3D res;
	TransformInterpolator::interpolate_transform_3d(
		xform_prev, xform_curr, res, p_interpolation_fraction);

	SceneTree* tree = get_tree();

	// This should not happen, as is_inside_tree() is checked earlier.
	ERR_FAIL_NULL_V(tree, res);
	if (!_client_physics_interpolation_node_3d_list.in_list()) {
		tree->client_physics_interpolation_add_node_3d(&_client_physics_interpolation_node_3d_list);
	}

	return res;
}

Transform3D Node3D::get_global_transform() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), Transform3D());

	/* Due to how threads work at scene level, while this global transform won't be able to be
	 * changed from outside a thread, it is possible that multiple threads can access it while it's
	 * dirty from previous work. Due to this, we must ensure that the dirty/update process is thread
	 * safe by utilizing atomic copies.
	 */

	uint32_t dirty = _read_dirty_mask();
	if (dirty & DIRTY_GLOBAL_TRANSFORM) {
		if (dirty & DIRTY_LOCAL_TRANSFORM) {
			_update_local_transform(); // Update local transform atomically.
		}

		Transform3D new_global;
		if (data.parent && !data.top_level) {
			new_global = data.parent->get_global_transform() * data.local_transform;
		}
		else {
			new_global = data.local_transform;
		}

		if (data.disable_scale) {
			new_global.basis.orthonormalize();
		}

		data.global_transform = new_global;
		_clear_dirty_bits(DIRTY_GLOBAL_TRANSFORM);
	}

	return data.global_transform;
}

#ifdef TOOLS_ENABLED
Transform3D Node3D::get_global_gizmo_transform() const { return get_global_transform(); }

Transform3D Node3D::get_local_gizmo_transform() const { return get_transform(); }
#endif

Transform3D Node3D::get_relative_transform(const Node* p_parent) const
{
	ERR_READ_THREAD_GUARD_V(Transform3D());
	if (p_parent == this) {
		return Transform3D();
	}

	ERR_FAIL_NULL_V(data.parent, Transform3D());

	if (p_parent == data.parent) {
		return get_transform();
	}
	else {
		return data.parent->get_relative_transform(p_parent) * get_transform();
	}
}

Node3D::RotationEditMode Node3D::get_rotation_edit_mode() const
{
	ERR_READ_THREAD_GUARD_V(ROTATION_EDIT_MODE_EULER);
	return data.rotation_edit_mode;
}

EulerOrder Node3D::get_rotation_order() const
{
	ERR_READ_THREAD_GUARD_V(EulerOrder::XYZ);
	return data.euler_rotation_order;
}

void Node3D::set_rotation_degrees(const Vector3& p_euler_degrees)
{
	ERR_THREAD_GUARD;
	Vector3 radians(Math::deg_to_rad(p_euler_degrees.x), Math::deg_to_rad(p_euler_degrees.y),
		Math::deg_to_rad(p_euler_degrees.z));
	set_rotation(radians);
}

Vector3 Node3D::get_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	return data.local_transform.origin;
}

Vector3 Node3D::get_rotation() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	if (_test_dirty_bits(DIRTY_EULER_ROTATION_AND_SCALE)) {
		_update_rotation_and_scale();
	}

	return data.euler_rotation;
}

Vector3 Node3D::get_rotation_degrees() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	Vector3 radians = get_rotation();
	return Vector3(
		Math::rad_to_deg(radians.x), Math::rad_to_deg(radians.y), Math::rad_to_deg(radians.z));
}

Vector3 Node3D::get_scale() const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	if (_test_dirty_bits(DIRTY_EULER_ROTATION_AND_SCALE)) {
		_update_rotation_and_scale();
	}

	return data.scale;
}

void Node3D::add_gizmo(Ref<Node3DGizmo> p_gizmo)
{
	ERR_THREAD_GUARD;
#ifdef TOOLS_ENABLED
	if (data.gizmos_disabled || p_gizmo.is_null()) {
		return;
	}
	data.gizmos.push_back(p_gizmo);

	if (p_gizmo.is_valid() && is_inside_world()) {
		p_gizmo->create();
		if (is_visible_in_tree()) {
			p_gizmo->redraw();
		}
		p_gizmo->transform();
	}
#endif
}

void Node3D::remove_gizmo(Ref<Node3DGizmo> p_gizmo)
{
	ERR_THREAD_GUARD;
#ifdef TOOLS_ENABLED
	int idx = data.gizmos.find(p_gizmo);
	if (idx != -1) {
		p_gizmo->free();
		data.gizmos.remove_at(idx);
	}
#endif
}

void Node3D::clear_gizmos()
{
	ERR_THREAD_GUARD;
#ifdef TOOLS_ENABLED
	for (int i = 0; i < data.gizmos.size(); i++) {
		data.gizmos.write[i]->free();
	}
	data.gizmos.clear();
	data.gizmos_requested = false;
#endif
}

Vector<Ref<Node3DGizmo>> Node3D::get_gizmos() const
{
	ERR_THREAD_GUARD_V(Vector<Ref<Node3DGizmo>>());
#ifdef TOOLS_ENABLED
	return data.gizmos;
#else
	return Vector<Ref<Node3DGizmo>>();
#endif
}

void Node3D::_replace_dirty_mask(uint32_t p_mask) const
{
	if (is_group_processing()) {
		data.dirty.mt.set(p_mask);
	}
	else {
		data.dirty.st = p_mask;
	}
}

void Node3D::_set_dirty_bits(uint32_t p_bits) const
{
	if (is_group_processing()) {
		data.dirty.mt.bit_or(p_bits);
	}
	else {
		data.dirty.st |= p_bits;
	}
}

void Node3D::_clear_dirty_bits(uint32_t p_bits) const
{
	if (is_group_processing()) {
		data.dirty.mt.bit_and(~p_bits);
	}
	else {
		data.dirty.st &= ~p_bits;
	}
}

void Node3D::_update_gizmos()
{
#ifdef TOOLS_ENABLED
	if (data.gizmos_disabled || !is_inside_world() || !data.gizmos_dirty) {
		data.gizmos_dirty = false;
		return;
	}
	data.gizmos_dirty = false;
	for (int i = 0; i < data.gizmos.size(); i++) {
		if (is_visible_in_tree()) {
			data.gizmos.write[i]->redraw();
		}
		else {
			data.gizmos.write[i]->clear();
		}
	}
#endif
}

void Node3D::set_disable_gizmos(bool p_enabled)
{
	ERR_THREAD_GUARD;
#ifdef TOOLS_ENABLED
	data.gizmos_disabled = p_enabled;
	if (!p_enabled) {
		clear_gizmos();
	}
#endif
}

void Node3D::reparent(Node* p_parent, bool p_keep_global_transform)
{
	ERR_THREAD_GUARD;
	if (p_keep_global_transform) {
		Transform3D temp = get_global_transform();
		Node::reparent(p_parent, p_keep_global_transform);
		set_global_transform(temp);
	}
	else {
		Node::reparent(p_parent, p_keep_global_transform);
	}
}

void Node3D::set_disable_scale(bool p_enabled)
{
	ERR_THREAD_GUARD;
	data.disable_scale = p_enabled;
}

bool Node3D::is_scale_disabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.disable_scale;
}

void Node3D::set_as_top_level(bool p_enabled)
{
	ERR_THREAD_GUARD;
	if (data.top_level == p_enabled) {
		return;
	}
	if (is_inside_tree()) {
		if (p_enabled) {
			set_transform(get_global_transform());
		}
		else if (data.parent) {
			set_transform(
				data.parent->get_global_transform().affine_inverse() * get_global_transform());
		}
	}
	data.top_level = p_enabled;
	reset_physics_interpolation();
}

void Node3D::set_as_top_level_keep_local(bool p_enabled)
{
	ERR_THREAD_GUARD;
	if (data.top_level == p_enabled) {
		return;
	}
	data.top_level = p_enabled;
	_propagate_transform_changed(this);
	reset_physics_interpolation();
}

bool Node3D::is_set_as_top_level() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.top_level;
}

Ref<World3D> Node3D::get_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World3D>()); // World3D can only be set from main thread, so it's
											 // safe to obtain on threads.
	ERR_FAIL_COND_V(!is_inside_world(), Ref<World3D>());
	ERR_FAIL_NULL_V(data.viewport, Ref<World3D>());

	return data.viewport->find_world_3d();
}

void Node3D::show()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(true);
}

void Node3D::hide()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(false);
}

void Node3D::set_visible(bool p_visible)
{
	ERR_MAIN_THREAD_GUARD;
	if (data.visible == p_visible) {
		return;
	}

	data.visible = p_visible;

	if (!is_inside_tree()) {
		return;
	}
	_propagate_visibility_changed();
}

bool Node3D::is_visible() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.visible;
}

bool Node3D::is_visible_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(
		false); // Since visibility can only be changed from main thread, this is safe to call.
	const Node3D* s = this;

	while (s) {
		if (!s->data.visible) {
			return false;
		}
		s = s->data.parent;
	}

	return true;
}

void Node3D::rotate_object_local(const Vector3& p_axis, real_t p_angle)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.rotate_local(p_axis, p_angle);
	set_transform(t);
}

void Node3D::rotate(const Vector3& p_axis, real_t p_angle)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.rotate(p_axis, p_angle);
	set_transform(t);
}

void Node3D::rotate_x(real_t p_angle)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.rotate(Vector3(1, 0, 0), p_angle);
	set_transform(t);
}

void Node3D::rotate_y(real_t p_angle)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.rotate(Vector3(0, 1, 0), p_angle);
	set_transform(t);
}

void Node3D::rotate_z(real_t p_angle)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.rotate(Vector3(0, 0, 1), p_angle);
	set_transform(t);
}

void Node3D::translate(const Vector3& p_offset)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.translate_local(p_offset);
	set_transform(t);
}

void Node3D::translate_object_local(const Vector3& p_offset)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();

	Transform3D s;
	s.translate_local(p_offset);
	set_transform(t * s);
}

void Node3D::scale(const Vector3& p_ratio)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.scale(p_ratio);
	set_transform(t);
}

void Node3D::scale_object_local(const Vector3& p_scale)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.basis.scale_local(p_scale);
	set_transform(t);
}

void Node3D::global_rotate(const Vector3& p_axis, real_t p_angle)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_global_transform();
	t.basis.rotate(p_axis, p_angle);
	set_global_transform(t);
}

void Node3D::global_scale(const Vector3& p_scale)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_global_transform();
	t.basis.scale(p_scale);
	set_global_transform(t);
}

void Node3D::global_translate(const Vector3& p_offset)
{
	ERR_THREAD_GUARD;
	Transform3D t = get_global_transform();
	t.origin += p_offset;
	set_global_transform(t);
}

void Node3D::orthonormalize()
{
	ERR_THREAD_GUARD;
	Transform3D t = get_transform();
	t.orthonormalize();
	set_transform(t);
}

void Node3D::set_identity()
{
	ERR_THREAD_GUARD;
	set_transform(Transform3D());
}

void Node3D::look_at(const Vector3& p_target, const Vector3& p_up, bool p_use_model_front)
{
	ERR_THREAD_GUARD;
	ERR_FAIL_COND_MSG(
		!is_inside_tree(), "Node not inside tree. Use look_at_from_position() instead.");
	Vector3 origin = get_global_transform().origin;
	look_at_from_position(origin, p_target, p_up, p_use_model_front);
}

void Node3D::look_at_from_position(
	const Vector3& p_pos, const Vector3& p_target, const Vector3& p_up, bool p_use_model_front)
{
	ERR_THREAD_GUARD;
	ERR_FAIL_COND_MSG(p_pos.is_equal_approx(p_target),
		"Node origin and target are in the same position, look_at() failed.");
	ERR_FAIL_COND_MSG(p_up.is_zero_approx(), "The up vector can't be zero, look_at() failed.");

	Vector3 forward = p_target - p_pos;
	Basis lookat_basis = Basis::looking_at(forward, p_up, p_use_model_front);
	Vector3 original_scale = get_scale();
	set_global_transform(Transform3D(lookat_basis, p_pos));
	set_scale(original_scale);
}

Vector3 Node3D::to_local(Vector3 p_global) const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	return get_global_transform().affine_inverse().xform(p_global);
}

Vector3 Node3D::to_global(Vector3 p_local) const
{
	ERR_READ_THREAD_GUARD_V(Vector3());
	return get_global_transform().xform(p_local);
}

void Node3D::set_notify_transform(bool p_enabled)
{
	ERR_THREAD_GUARD;
	data.notify_transform = p_enabled;
}

bool Node3D::is_transform_notification_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.notify_transform;
}

void Node3D::set_notify_local_transform(bool p_enabled)
{
	ERR_THREAD_GUARD;
	data.notify_local_transform = p_enabled;
}

bool Node3D::is_local_transform_notification_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return data.notify_local_transform;
}

void Node3D::set_visibility_parent(const NodePath& p_path)
{
	ERR_MAIN_THREAD_GUARD;
	visibility_parent_path = p_path;
	if (is_inside_tree()) {
		_update_visibility_parent(true);
	}
}

NodePath Node3D::get_visibility_parent() const
{
	ERR_READ_THREAD_GUARD_V(NodePath());
	return visibility_parent_path;
}

bool Node3D::_property_can_revert(const StringName& p_name) const
{
	const String sname = p_name;
	if (sname == "basis") {
		return true;
	}
	else if (sname == "scale") {
		return true;
	}
	else if (sname == "quaternion") {
		return true;
	}
	else if (sname == "rotation") {
		return true;
	}
	else if (sname == "position") {
		return true;
	}
	return false;
}

Node3D::~Node3D()
{
	_disable_client_physics_interpolation();

	if (is_inside_tree()) {
		get_tree()->get_scene_tree_fti().node_3d_notify_delete(this);
	}
}


