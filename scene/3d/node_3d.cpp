/**************************************************************************/
/*  node_3d.cpp                                                           */
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

#include "core/config/engine.h"
#include "core/math/geometry_3d.h"
#include "node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/node.h"
#include "scene/resources/environment.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"

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
		if (SceneTree::data) {
			SceneTree::data->xform_change_list.add(&xform_change);
		}
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

void Node3D::set_transform(const Transform3D& p_transform) {}

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
		SceneTreeFTI::node_3d_notify_changed(*this, p_transform_changed);
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

	uint64_t tick = Engine::get_physics_frames();

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

		if (SceneTree::is_initialized() && _client_physics_interpolation_node_3d_list.in_list()) {
			SceneTree::client_physics_interpolation_remove_node_3d(
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
			Engine::get_physics_frames();
	}

	// Storing the last tick we requested client interpolation allows us to timeout
	// and remove client interpolated nodes from the list to save processing.
	// We use some arbitrary timeout here, but this could potentially be user defined.

	// Note: This timeout has to be larger than the number of ticks in a frame, otherwise the
	// interpolated data will stop flowing before the next frame is drawn. This should only be
	// relevant at high tick rates. We could alternatively do this by frames rather than ticks and
	// avoid this problem, but then the behavior would be machine dependent.
	data.client_physics_interpolation_data->timeout_physics_tick =
		Engine::get_physics_frames() + 256;

	// Make sure data is up to date.
	update_client_physics_interpolation_data();

	// Interpolate the current data.
	const Transform3D& xform_curr = data.client_physics_interpolation_data->global_xform_curr;
	const Transform3D& xform_prev = data.client_physics_interpolation_data->global_xform_prev;

	Transform3D res;
	TransformInterpolator::interpolate_transform_3d(
		xform_prev, xform_curr, res, p_interpolation_fraction);

	ERR_FAIL_COND_V(!SceneTree::is_initialized(), res);
	if (!_client_physics_interpolation_node_3d_list.in_list()) {
		SceneTree::client_physics_interpolation_add_node_3d(
			&_client_physics_interpolation_node_3d_list);
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

void Node3D::update_gizmos() {}

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

bool Node3D::_property_can_revert(const String& p_name) const
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
		SceneTreeFTI::node_3d_notify_delete(this);
	}
}

Transform3D Node3D::get_global_transform_interpolated()
{
	return Transform3D();
}



void Node3D::_update_visibility_parent(bool) {}

void Node3D::set_subgizmo_selection(Ref<Node3DGizmo>, int, Transform3D) {}

void Node3D::set_scale(Vector3 const&) {}

void Node3D::set_rotation(Vector3 const&) {}

void Node3D::_propagate_visibility_changed() {}

void Node3D::_propagate_transform_changed(Node3D*) {}

void Node3D::clear_subgizmo_selection() {}

// ==========================================================================
// Merged from scene/3d/camera_3d.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"
#include "core/config/engine.h"
#include "core/math/projection.h"
#include "core/math/geometry_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

void Camera3D::_update_audio_listener_state() {}

void Camera3D::_request_camera_update() { _update_camera(); }

void Camera3D::set_xr_enabled(bool p_enabled)
{
	if (xr_enabled == p_enabled) {
		return;
	}

	if (xr_enabled && is_inside_tree()) {
		_unbind_tracker();
	}

	xr_enabled = p_enabled;
	if (xr_enabled) {
		set_physics_interpolation_mode(Node::PHYSICS_INTERPOLATION_MODE_OFF);
		if (is_inside_tree()) {
			_bind_tracker();
		}
	}

	update_configuration_warnings();
}

void Camera3D::_bind_tracker()
{
	ERR_FAIL_COND(tracker.is_valid());
	if (XRServer::is_initialized()) {
		tracker = XRServer::get_tracker(tracker_name);
		if (tracker.is_valid()) {
			Ref<XRPose> pose = tracker->get_pose(pose_name);
			if (pose.is_valid()) {
				set_transform(pose->get_adjusted_transform());
			}
		}
	}
}

void Camera3D::_unbind_tracker()
{
	if (tracker.is_valid()) {
		tracker.unref();
	}
}

void Camera3D::_changed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (xr_enabled && p_tracker_name == tracker_name) {
		_unbind_tracker();
		_bind_tracker();
	}
}

void Camera3D::_removed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (xr_enabled && p_tracker_name == tracker_name) {
		_unbind_tracker();
	}
}

void Camera3D::_pose_changed(const Ref<XRPose>& p_pose)
{
	if (xr_enabled && p_pose.is_valid() && p_pose->get_name() == pose_name) {
		set_transform(p_pose->get_adjusted_transform());
	}
}

void Camera3D::_notification(int p_what)
{
	if (!xr_enabled) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_bind_tracker();
	} break;
	case NOTIFICATION_EXIT_TREE: {
		_unbind_tracker();
	} break;
	}
}

PackedStringArray Camera3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (xr_enabled) {
		if (is_visible() && is_inside_tree()) {
			const XROrigin3D* origin = dynamic_cast<const XROrigin3D*>(get_parent());
			if (origin == nullptr) {
				warnings.push_back(RTR("XR Camera3D may not function as expected without an "
									   "XROrigin3D node as its parent."));
			}
		}
		if (get_physics_interpolation_mode() != Node::PHYSICS_INTERPOLATION_MODE_OFF) {
			warnings.push_back(RTR("XR Camera3D should have physics_interpolation_mode set to OFF "
								   "in order to avoid jitter."));
		}
	}

	return warnings;
}

void Camera3D::fti_pump_property()
{
	switch (mode) {
	default:
		break;
	case PROJECTION_PERSPECTIVE: {
		fov.pump();
	} break;
	case PROJECTION_ORTHOGONAL: {
		size.pump();
	} break;
	case PROJECTION_FRUSTUM: {
		size.pump();
		frustum_offset.pump();
	} break;
	}
	_near.pump();
	_far.pump();

	Node3D::fti_pump_property();
}

void Camera3D::fti_update_servers_property()
{
	if (camera.is_valid()) {
		float f = Engine::get_physics_interpolation_fraction();

		bool update_fov = fov.interpolate(f);
		bool update_near = _near.interpolate(f);
		bool update_far = _far.interpolate(f);
		bool update_size = size.interpolate(f);
		bool update_frustum_offset = frustum_offset.interpolate(f);

		// If there have been changes due to interpolated values, OR we are forcing an update,
		// update the servers.
		switch (mode) {
		default:
			break;
		case PROJECTION_PERSPECTIVE: {
			if (update_fov || update_near || update_far) {
				Renderer::camera_set_perspective(
					camera, fov.interpolated(), _near.interpolated(), _far.interpolated());
			}
		} break;
		case PROJECTION_ORTHOGONAL: {
			if (update_size || update_near || update_far) {
				Renderer::camera_set_orthogonal(
					camera, size.interpolated(), _near.interpolated(), _far.interpolated());
			}
		} break;
		case PROJECTION_FRUSTUM: {
			if (update_size || update_frustum_offset || update_near || update_far) {
				Renderer::camera_set_frustum(camera, size.interpolated(),
					frustum_offset.interpolated(), _near.interpolated(), _far.interpolated());
			}
		} break;
		}
	}

	Node3D::fti_update_servers_property();
}

void Camera3D::fti_update_servers_xform()
{
	if (camera.is_valid()) {
		Transform3D tr =
			_get_adjusted_camera_transform(_get_cached_global_transform_interpolated());
		Renderer::camera_set_transform(camera, tr);
	}
	Node3D::fti_update_servers_xform();
}

void Camera3D::_update_camera()
{
	if (!is_inside_tree()) {
		return;
	}

	if (!is_physics_interpolated_and_enabled()) {
		Renderer::camera_set_transform(camera, get_camera_transform());
	}
	else {
		// Force a refresh next frame.
		fti_notify_node_changed();
	}

	if (is_part_of_edited_scene() || !is_current()) {
		return;
	}

	get_viewport()->_camera_3d_transform_changed_notify();
}

void Camera3D::_physics_interpolated_changed() { _update_process_mode(); }

void Camera3D::set_desired_process_modes(bool p_process_internal, bool p_physics_process_internal)
{
	_desired_process_internal = p_process_internal;
	_desired_physics_process_internal = p_physics_process_internal;
	_update_process_mode();
}

void Camera3D::_update_process_mode()
{
	set_process_internal(_desired_process_internal);
	set_physics_process_internal(_desired_physics_process_internal);
}

Transform3D Camera3D::_get_adjusted_camera_transform(const Transform3D& p_xform) const
{
	Transform3D tr = p_xform.orthonormalized();
	tr.origin += tr.basis.get_column(1) * v_offset;
	tr.origin += tr.basis.get_column(0) * h_offset;
	return tr;
}

Transform3D Camera3D::get_camera_transform() const
{
	if (is_physics_interpolated_and_enabled() && !Engine::is_in_physics_frame()) {
		return _get_adjusted_camera_transform(_get_cached_global_transform_interpolated());
	}

	return _get_adjusted_camera_transform(get_global_transform());
}

Projection Camera3D::_get_camera_projection(real_t p_near) const
{
	Size2 viewport_size = get_viewport()->get_visible_rect().size;
	Projection cm;

	switch (mode) {
	case PROJECTION_PERSPECTIVE: {
		cm.set_perspective(fov, viewport_size.aspect(), p_near, _far, keep_aspect == KEEP_WIDTH);
	} break;
	case PROJECTION_ORTHOGONAL: {
		cm.set_orthogonal(size, viewport_size.aspect(), p_near, _far, keep_aspect == KEEP_WIDTH);
	} break;
	case PROJECTION_FRUSTUM: {
		cm.set_frustum(size, viewport_size.aspect(), frustum_offset, p_near, _far);
	} break;
	}

	return cm;
}

Projection Camera3D::get_camera_projection() const
{
	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Projection(), "Camera is not inside the scene tree.");
	return _get_camera_projection(_near);
}

RID Camera3D::get_camera() const { return camera; }

void Camera3D::make_current()
{
	current = true;

	if (!is_inside_tree()) {
		return;
	}

	get_viewport()->_camera_3d_set(this);
}

void Camera3D::clear_current(bool p_enable_next)
{
	current = false;
	if (!is_inside_tree()) {
		return;
	}

	if (get_viewport()->get_camera_3d() == this) {
		get_viewport()->_camera_3d_set(nullptr);

		if (p_enable_next && !Engine::is_editor_hint()) {
			get_viewport()->_camera_3d_make_next_current(this);
		}
	}
}

void Camera3D::set_current(bool p_enabled)
{
	if (p_enabled) {
		make_current();
	}
	else {
		clear_current();
	}
}

bool Camera3D::is_current() const
{
	if (is_inside_tree() && !is_part_of_edited_scene()) {
		return get_viewport()->get_camera_3d() == this;
	}
	else {
		return current;
	}
}

Vector3 Camera3D::project_ray_normal(const Point2& p_pos) const
{
	Vector3 ray = project_local_ray_normal(p_pos);
	return get_camera_transform().basis.xform(ray).normalized();
}

Vector3 Camera3D::project_local_ray_normal(const Point2& p_pos) const
{
	if (xr_enabled) {
		ERR_FAIL_COND_V(!XRServer::is_initialized(), Vector3());
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");
			Size2 viewport_size = get_viewport()->get_camera_rect_size();
			Vector2 cpos = get_viewport()->get_camera_coords(p_pos);
			Projection cm = xr_interface->get_projection_for_view(
				0, viewport_size.aspect(), get_near(), get_far());
			Vector2 screen_he = cm.get_viewport_half_extents();
			return Vector3(((cpos.x / viewport_size.width) * 2.0 - 1.0) * screen_he.x,
				((1.0 - (cpos.y / viewport_size.height)) * 2.0 - 1.0) * screen_he.y, -get_near())
				.normalized();
		}
	}

	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");

	Size2 viewport_size = get_viewport()->get_camera_rect_size();
	Vector2 cpos = get_viewport()->get_camera_coords(p_pos);
	Vector3 ray;

	if (mode == PROJECTION_ORTHOGONAL) {
		ray = Vector3(0, 0, -1);
	}
	else {
		Projection cm = _get_camera_projection(_near);
		Vector2 screen_he = cm.get_viewport_half_extents();
		ray = Vector3(((cpos.x / viewport_size.width) * 2.0 - 1.0) * screen_he.x,
			((1.0 - (cpos.y / viewport_size.height)) * 2.0 - 1.0) * screen_he.y, -_near)
				  .normalized();
	}

	return ray;
}

Vector3 Camera3D::project_ray_origin(const Point2& p_pos) const
{
	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");

	Size2 viewport_size = get_viewport()->get_camera_rect_size();
	Vector2 cpos = get_viewport()->get_camera_coords(p_pos);
	ERR_FAIL_COND_V(viewport_size.y == 0, Vector3());

	if (mode == PROJECTION_ORTHOGONAL) {
		Vector2 pos = cpos / viewport_size;
		real_t vsize, hsize;
		if (keep_aspect == KEEP_WIDTH) {
			vsize = size / viewport_size.aspect();
			hsize = size;
		}
		else {
			hsize = size * viewport_size.aspect();
			vsize = size;
		}

		Vector3 ray;
		ray.x = pos.x * (hsize)-hsize / 2;
		ray.y = (1.0 - pos.y) * (vsize)-vsize / 2;
		ray.z = -_near;
		ray = get_camera_transform().xform(ray);
		return ray;
	}
	else {
		return get_camera_transform().origin;
	};
}

bool Camera3D::is_position_behind(const Vector3& p_pos) const
{
	Transform3D t = get_global_transform();
	Vector3 eyedir = -t.basis.get_column(2).normalized();
	return eyedir.dot(p_pos - t.origin) < _near;
}

Vector<Vector3> Camera3D::get_near_plane_points() const
{
	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector<Vector3>(), "Camera is not inside scene.");

	Projection cm = _get_camera_projection(_near);

	Vector3 endpoints[8];
	cm.get_endpoints(Transform3D(), endpoints);

	Vector<Vector3> points = {Vector3(), endpoints[4], endpoints[5], endpoints[6], endpoints[7]};
	return points;
}

Point2 Camera3D::unproject_position(const Vector3& p_pos) const
{
	if (xr_enabled) {
		ERR_FAIL_COND_V(!XRServer::is_initialized(), Vector2());
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector2(), "Camera is not inside scene.");
			Size2 viewport_size = get_viewport()->get_visible_rect().size;
			Projection cm = xr_interface->get_projection_for_view(
				0, viewport_size.aspect(), get_near(), get_far());
			Plane p(get_camera_transform().xform_inv(p_pos), 1.0);
			p = cm.xform4(p);
			ERR_FAIL_COND_V(p.d == 0, Point2());
			p.normal /= p.d;
			Point2 res;
			res.x = (p.normal.x * 0.5 + 0.5) * viewport_size.x;
			res.y = (-p.normal.y * 0.5 + 0.5) * viewport_size.y;
			return res;
		}
	}

	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector2(), "Camera is not inside scene.");

	Size2 viewport_size = get_viewport()->get_visible_rect().size;

	Projection cm = _get_camera_projection(_near);

	Plane p(get_camera_transform().xform_inv(p_pos), 1.0);

	p = cm.xform4(p);

	// Prevent divide by zero.
	ERR_FAIL_COND_V(p.d == 0, Point2());

	p.normal /= p.d;

	Point2 res;
	res.x = (p.normal.x * 0.5 + 0.5) * viewport_size.x;
	res.y = (-p.normal.y * 0.5 + 0.5) * viewport_size.y;

	return res;
}

Vector3 Camera3D::project_position(const Point2& p_point, real_t p_z_depth) const
{
	if (xr_enabled) {
		ERR_FAIL_COND_V(!XRServer::is_initialized(), Vector3());
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");
			Size2 viewport_size = get_viewport()->get_visible_rect().size;
			Projection cm = xr_interface->get_projection_for_view(
				0, viewport_size.aspect(), get_near(), get_far());
			Vector2 vp_he = cm.get_viewport_half_extents();
			Vector2 point;
			point.x = (p_point.x / viewport_size.x) * 2.0 - 1.0;
			point.y = (1.0 - (p_point.y / viewport_size.y)) * 2.0 - 1.0;
			point *= vp_he;
			Vector3 p(point.x, point.y, -p_z_depth);
			return get_camera_transform().xform(p);
		}
	}

	ERR_FAIL_COND_V_MSG(!is_inside_tree(), Vector3(), "Camera is not inside scene.");

	if (p_z_depth == 0 && mode != PROJECTION_ORTHOGONAL) {
		return get_global_transform().origin;
	}
	Size2 viewport_size = get_viewport()->get_visible_rect().size;

	Projection cm = _get_camera_projection(_near);

	Plane z_slice(Vector3(0, 0, 1), -p_z_depth);
	Vector3 res;
	z_slice.intersect_3(cm.get_projection_plane(Projection::Planes::PLANE_RIGHT),
		cm.get_projection_plane(Projection::Planes::PLANE_TOP), &res);
	Vector2 vp_he(res.x, res.y);

	Vector2 point;
	point.x = (p_point.x / viewport_size.x) * 2.0 - 1.0;
	point.y = (1.0 - (p_point.y / viewport_size.y)) * 2.0 - 1.0;
	point *= vp_he;

	Vector3 p(point.x, point.y, -p_z_depth);

	return get_camera_transform().xform(p);
}

Ref<Environment> Camera3D::get_environment() const { return environment; }

Ref<CameraAttributes> Camera3D::get_attributes() const { return attributes; }

Ref<Compositor> Camera3D::get_compositor() const { return compositor; }

Camera3D::KeepAspect Camera3D::get_keep_aspect_mode() const { return keep_aspect; }

Camera3D::DopplerTracking Camera3D::get_doppler_tracking() const { return doppler_tracking; }

real_t Camera3D::get_fov() const { return fov; }

real_t Camera3D::get_size() const { return size; }

real_t Camera3D::get_near() const { return _near; }

Vector2 Camera3D::get_frustum_offset() const { return frustum_offset; }

real_t Camera3D::get_far() const { return _far; }

Camera3D::ProjectionType Camera3D::get_projection() const { return mode; }

uint32_t Camera3D::get_cull_mask() const { return layers; }

bool Camera3D::get_cull_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return layers & (1 << (p_layer_number - 1));
}

Vector<Plane> Camera3D::get_frustum() const
{
	if (xr_enabled) {
		ERR_FAIL_COND_V(!XRServer::is_initialized(), Vector<Plane>());
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			ERR_FAIL_COND_V(!is_inside_world(), Vector<Plane>());
			Size2 viewport_size = get_viewport()->get_visible_rect().size;
			Projection cm = xr_interface->get_projection_for_view(
				0, viewport_size.aspect(), get_near(), get_far());
			return cm.get_projection_planes(get_camera_transform());
		}
	}

	ERR_FAIL_COND_V(!is_inside_world(), Vector<Plane>());

	Projection cm = _get_camera_projection(_near);

	return cm.get_projection_planes(get_camera_transform());
}

bool Camera3D::is_position_in_frustum(const Vector3& p_position) const
{
	Vector<Plane> frustum = get_frustum();
	for (int i = 0; i < frustum.size(); i++) {
		if (frustum[i].is_point_over(p_position)) {
			return false;
		}
	}
	return true;
}

void Camera3D::set_v_offset(real_t p_offset)
{
	v_offset = p_offset;
	_update_camera();
}

real_t Camera3D::get_v_offset() const { return v_offset; }

void Camera3D::set_h_offset(real_t p_offset)
{
	h_offset = p_offset;
	_update_camera();
}

real_t Camera3D::get_h_offset() const { return h_offset; }

Vector3 Camera3D::get_doppler_tracked_velocity() const
{
	if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
		return velocity_tracker->get_tracked_linear_velocity();
	}
	else {
		return Vector3();
	}
}

Camera3D::Camera3D(bool p_xr)
{
	pose_name = SceneString(default_);
	if (p_xr) {
		set_xr_enabled(true);
	}
}

Camera3D::~Camera3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(camera);
#ifndef PHYSICS_3D_DISABLED
	if (pyramid_shape.is_valid()) {
		ERR_FAIL_COND(!PhysicsServer3D::is_initialized());
		PhysicsServer3D::free_rid(pyramid_shape);
	}
#endif // PHYSICS_3D_DISABLED
}

// ==========================================================================
// Merged from scene/3d/marker_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/marker_3d.h ---
#include "scene/3d/node_3d.h"

class Marker3D : public Node3D
{
	real_t gizmo_extents = 0.25;

protected:

public:
	real_t get_gizmo_extents() const;

	Marker3D();
};



// --- End scene/3d/marker_3d.h ---

real_t Marker3D::get_gizmo_extents() const { return gizmo_extents; }

Marker3D::Marker3D() {}

// ==========================================================================
// Merged from scene/3d/path_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

Path3D::Path3D()
{
	if (SceneTree::is_initialized() && SceneTree::is_debugging_paths_hint()) {
		debug_instance = Renderer::instance_create();
		set_notify_transform(true);
		_update_debug_mesh();
	}
}

Path3D::~Path3D()
{
	if (debug_instance.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(debug_instance);
	}
	if (debug_mesh.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(debug_mesh->get_rid());
	}
}

void Path3D::set_debug_custom_color(const Color& p_color)
{
	debug_custom_color = p_color;
	_update_debug_path_material();
}

Ref<StandardMaterial3D> Path3D::get_debug_material() { return debug_material; }

const Color& Path3D::get_debug_custom_color() const { return debug_custom_color; }

Ref<Curve3D> Path3D::get_curve() const { return curve; }

void PathFollow3D::update_transform()
{
	if (!path) {
		return;
	}
	Ref<Curve3D> c = path->get_curve();
	if (c.is_null()) {
		return;
	}
	real_t bl = c->get_baked_length();
	if (bl == 0.0) {
		return;
	}
	Transform3D t;
	if (rotation_mode == ROTATION_NONE) {
		Vector3 pos = c->sample_baked(progress, cubic);
		t.origin = pos;
	}
	else {
		t = c->sample_baked_with_rotation(progress, cubic, false);
		Vector3 tangent = -t.basis.get_column(2); // Retain tangent for applying tilt.
		t = PathFollow3D::correct_posture(t, rotation_mode);

		// Switch Z+ and Z- if necessary.
		if (use_model_front) {
			t.basis *= Basis::from_scale(Vector3(-1.0, 1.0, -1.0));
		}

		// Apply tilt *after* correct_posture().
		if (tilt_enabled) {
			const real_t tilt = c->sample_baked_tilt(progress);

			const Basis twist(tangent, tilt);
			t.basis = twist * t.basis;
		}
	}

	// Apply offset and scale.
	Vector3 scale = get_transform().basis.get_scale();
	t.translate_local(Vector3(h_offset, v_offset, 0));
	t.basis.scale_local(scale);

	set_transform(t);
}

void PathFollow3D::set_cubic_interpolation_enabled(bool p_enabled) { cubic = p_enabled; }

bool PathFollow3D::is_cubic_interpolation_enabled() const { return cubic; }

Transform3D PathFollow3D::correct_posture(
	Transform3D p_transform, PathFollow3D::RotationMode p_rotation_mode)
{
	Transform3D t = p_transform;

	// Modify frame according to rotation mode.
	if (p_rotation_mode == PathFollow3D::ROTATION_NONE) {
		// Clear rotation.
		t.basis = Basis();
	}
	else if (p_rotation_mode == PathFollow3D::ROTATION_ORIENTED) {
		Vector3 tangent = -t.basis.get_column(2);

		// Y-axis points up by default.
		t.basis = Basis::looking_at(tangent);
	}
	else {
		// Lock some euler axes.
		Vector3 euler = t.basis.get_euler_normalized(EulerOrder::YXZ);
		if (p_rotation_mode == PathFollow3D::ROTATION_Y) {
			// Only Y-axis allowed.
			euler[0] = 0;
			euler[2] = 0;
		}
		else if (p_rotation_mode == PathFollow3D::ROTATION_XY) {
			// XY allowed.
			euler[2] = 0;
		}

		Basis locked = Basis::from_euler(euler, EulerOrder::YXZ);
		t.basis = locked;
	}

	return t;
}


void PathFollow3D::set_progress(real_t p_progress)
{
	ERR_FAIL_COND(!std::isfinite(p_progress));
	if (progress == p_progress) {
		return;
	}
	progress = p_progress;

	if (path) {
		if (path->get_curve().is_valid()) {
			real_t path_length = path->get_curve()->get_baked_length();

			if (loop && path_length) {
				progress = Math::fposmod(progress, path_length);
				if (!Math::is_zero_approx(p_progress) && Math::is_zero_approx(progress)) {
					progress = path_length;
				}
			}
			else {
				progress = CLAMP(progress, 0, path_length);
			}
		}

		update_transform();
	}
}

void PathFollow3D::set_h_offset(real_t p_h_offset)
{
	if (h_offset == p_h_offset) {
		return;
	}
	h_offset = p_h_offset;
	update_transform();
}

real_t PathFollow3D::get_h_offset() const { return h_offset; }

void PathFollow3D::set_v_offset(real_t p_v_offset)
{
	if (v_offset == p_v_offset) {
		return;
	}
	v_offset = p_v_offset;
	update_transform();
}

real_t PathFollow3D::get_v_offset() const { return v_offset; }

real_t PathFollow3D::get_progress() const { return progress; }

void PathFollow3D::set_progress_ratio(real_t p_ratio)
{
	ERR_FAIL_NULL_MSG(path, "Can only set progress ratio on a PathFollow3D that is the child of a "
							"Path3D which is itself part of the scene tree.");
	ERR_FAIL_COND_MSG(path->get_curve().is_null(),
		"Can't set progress ratio on a PathFollow3D that does not have a Curve.");
	ERR_FAIL_COND_MSG(!path->get_curve()->get_baked_length(),
		"Can't set progress ratio on a PathFollow3D that has a 0 length curve.");
	set_progress(p_ratio * path->get_curve()->get_baked_length());
}

real_t PathFollow3D::get_progress_ratio() const
{
	if (path && path->get_curve().is_valid() && path->get_curve()->get_baked_length()) {
		return get_progress() / path->get_curve()->get_baked_length();
	}
	else {
		return 0;
	}
}

PathFollow3D::RotationMode PathFollow3D::get_rotation_mode() const { return rotation_mode; }

void PathFollow3D::set_use_model_front(bool p_use_model_front)
{
	if (use_model_front == p_use_model_front) {
		return;
	}
	use_model_front = p_use_model_front;
	update_transform();
}

bool PathFollow3D::is_using_model_front() const { return use_model_front; }

void PathFollow3D::set_loop(bool p_loop)
{
	if (loop == p_loop) {
		return;
	}
	loop = p_loop;
	update_transform();
}

bool PathFollow3D::has_loop() const { return loop; }

void PathFollow3D::set_tilt_enabled(bool p_enabled)
{
	if (tilt_enabled == p_enabled) {
		return;
	}
	tilt_enabled = p_enabled;
	update_transform();
}

bool PathFollow3D::is_tilt_enabled() const { return tilt_enabled; }



void Path3D::_update_debug_path_material() {}

void Path3D::_update_debug_mesh() {}

// ==========================================================================
// Merged from scene/3d/remote_transform_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/remote_transform_3d.h ---
#include "scene/3d/node_3d.h"

class RemoteTransform3D : public Node3D
{
	NodePath remote_node;

	bool use_global_coordinates = true;
	bool update_remote_position = true;
	bool update_remote_rotation = true;
	bool update_remote_scale = true;

	void _update_remote();
	void _update_cache();

protected:
	void _notification(int p_what);

public:
	NodePath get_remote_node() const;

	void set_use_global_coordinates(const bool p_enable);
	bool get_use_global_coordinates() const;

	void set_update_position(const bool p_update);
	bool get_update_position() const;

	void set_update_rotation(const bool p_update);
	bool get_update_rotation() const;

	void set_update_scale(const bool p_update);
	bool get_update_scale() const;

	void force_update_cache();

	PackedStringArray get_configuration_warnings() const override;

	RemoteTransform3D();
};



// --- End scene/3d/remote_transform_3d.h ---

NodePath RemoteTransform3D::get_remote_node() const { return remote_node; }

void RemoteTransform3D::set_use_global_coordinates(const bool p_enable)
{
	if (use_global_coordinates == p_enable) {
		return;
	}

	use_global_coordinates = p_enable;

	set_notify_transform(use_global_coordinates);
	set_notify_local_transform(!use_global_coordinates);
	_update_remote();
}

bool RemoteTransform3D::get_use_global_coordinates() const { return use_global_coordinates; }

void RemoteTransform3D::set_update_position(const bool p_update)
{
	if (update_remote_position == p_update) {
		return;
	}
	update_remote_position = p_update;
	_update_remote();
}

bool RemoteTransform3D::get_update_position() const { return update_remote_position; }

void RemoteTransform3D::set_update_rotation(const bool p_update)
{
	if (update_remote_rotation == p_update) {
		return;
	}
	update_remote_rotation = p_update;
	_update_remote();
}

bool RemoteTransform3D::get_update_rotation() const { return update_remote_rotation; }

void RemoteTransform3D::set_update_scale(const bool p_update)
{
	if (update_remote_scale == p_update) {
		return;
	}
	update_remote_scale = p_update;
	_update_remote();
}

bool RemoteTransform3D::get_update_scale() const { return update_remote_scale; }

void RemoteTransform3D::force_update_cache() { _update_cache(); }

RemoteTransform3D::RemoteTransform3D()
{
	set_notify_transform(use_global_coordinates);
	set_notify_local_transform(!use_global_coordinates);
}



void RemoteTransform3D::_update_remote() {}

PackedStringArray RemoteTransform3D::get_configuration_warnings() const { return PackedStringArray(); }

void RemoteTransform3D::_update_cache() {}

// ==========================================================================
// Merged from scene/3d/velocity_tracker_3d.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"

#include "core/config/engine.h"

void VelocityTracker3D::set_track_physics_step(bool p_track_physics_step) {
	physics_step = p_track_physics_step;
}

bool VelocityTracker3D::is_tracking_physics_step() const {
	return physics_step;
}

void VelocityTracker3D::update_position(const Vector3 &p_position) {
	PositionHistory ph;
	ph.position = p_position;
	if (physics_step) {
		ph.frame = Engine::get_physics_frames();
	} else {
		ph.frame = Engine::get_frame_ticks();
	}

	if (position_history_len == 0 || position_history[0].frame != ph.frame) { //in same frame, use latest
		position_history_len = MIN(position_history.size(), position_history_len + 1);
		for (int i = position_history_len - 1; i > 0; i--) {
			position_history.write[i] = position_history[i - 1];
		}
	}

	position_history.write[0] = ph;
}

Vector3 VelocityTracker3D::get_tracked_linear_velocity() const {
	Vector3 linear_velocity;

	double max_time = 1 / 5.0; //maximum time to interpolate a velocity

	Vector3 distance_accum;
	double time_accum = 0.0;
	double base_time = 0.0;

	if (position_history_len) {
		if (physics_step) {
			uint64_t base = Engine::get_physics_frames();
			base_time = double(base - position_history[0].frame) / Engine::get_user_physics_ticks_per_second();
		} else {
			uint64_t base = Engine::get_frame_ticks();
			base_time = double(base - position_history[0].frame) / 1000000.0;
		}
	}

	for (int i = 0; i < position_history_len - 1; i++) {
		double delta = 0.0;
		uint64_t diff = position_history[i].frame - position_history[i + 1].frame;
		Vector3 distance = position_history[i].position - position_history[i + 1].position;

		if (physics_step) {
			delta = double(diff) / Engine::get_user_physics_ticks_per_second();
		} else {
			delta = double(diff) / 1000000.0;
		}

		if (base_time + time_accum + delta > max_time) {
			break;
		}

		distance_accum += distance;
		time_accum += delta;
	}

	if (time_accum) {
		linear_velocity = distance_accum / time_accum;
	}

	return linear_velocity;
}

void VelocityTracker3D::reset(const Vector3 &p_new_pos) {
	PositionHistory ph;
	ph.position = p_new_pos;
	if (physics_step) {
		ph.frame = Engine::get_physics_frames();
	} else {
		ph.frame = Engine::get_frame_ticks();
	}

	position_history.write[0] = ph;
	position_history_len = 1;
}

VelocityTracker3D::VelocityTracker3D() {
	position_history.resize(4); // should be configurable
}

// ==========================================================================
// Merged from scene/3d/audio_listener_3d.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"
#include "scene/main/viewport.h"

void AudioListener3D::_update_audio_listener_state() {}

void AudioListener3D::_request_listener_update() { _update_listener(); }

void AudioListener3D::_update_listener()
{
	if (is_inside_tree() && is_current()) {
		get_viewport()->_listener_transform_3d_changed_notify();
	}
}

void AudioListener3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_WORLD: {
		bool first_listener = get_viewport()->_audio_listener_3d_add(this);
		if (!is_part_of_edited_scene() && (current || first_listener)) {
			make_current();
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_request_listener_update();
		if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
			velocity_tracker->update_position(get_global_transform().origin);
		}
	} break;

	case NOTIFICATION_EXIT_WORLD: {
		if (!is_part_of_edited_scene()) {
			if (is_current()) {
				clear_current();
				current = true; // keep it true

			}
			else {
				current = false;
			}
		}

		get_viewport()->_audio_listener_3d_remove(this);
	} break;
	}
}

Transform3D AudioListener3D::get_listener_transform() const
{
	return get_global_transform().orthonormalized();
}

void AudioListener3D::make_current()
{
	current = true;

	if (!is_inside_tree()) {
		return;
	}

	get_viewport()->_audio_listener_3d_set(this);
}

void AudioListener3D::clear_current()
{
	current = false;
	if (!is_inside_tree()) {
		return;
	}

	if (get_viewport()->get_audio_listener_3d() == this) {
		get_viewport()->_audio_listener_3d_set(nullptr);
		get_viewport()->_audio_listener_3d_make_next_current(this);
	}
}

bool AudioListener3D::is_current() const
{
	if (is_inside_tree() && !is_part_of_edited_scene()) {
		return get_viewport()->get_audio_listener_3d() == this;
	}
	else {
		return current;
	}
}

void AudioListener3D::set_doppler_tracking(DopplerTracking p_tracking)
{
	if (doppler_tracking == p_tracking) {
		return;
	}

	doppler_tracking = p_tracking;
	if (p_tracking != DOPPLER_TRACKING_DISABLED) {
		velocity_tracker->set_track_physics_step(doppler_tracking == DOPPLER_TRACKING_PHYSICS_STEP);
		if (is_inside_tree()) {
			velocity_tracker->reset(get_global_transform().origin);
		}
	}
}

AudioListener3D::DopplerTracking AudioListener3D::get_doppler_tracking() const
{
	return doppler_tracking;
}


Vector3 AudioListener3D::get_doppler_tracked_velocity() const
{
	if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
		return velocity_tracker->get_tracked_linear_velocity();
	}
	else {
		return Vector3();
	}
}

AudioListener3D::AudioListener3D()
{
	set_notify_transform(true);
	velocity_tracker.instantiate();
}

AudioListener3D::~AudioListener3D() {}

// ==========================================================================
// Merged from scene/3d/audio_stream_player_3d.cpp
// ==========================================================================

// --- audio_stream_player_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "scene/3d/visual_instance_3d.h"

bool AudioStreamPlayer3D::_is_autoplay_enabled_bind_compat_86907() { return is_autoplay_enabled(); }

void AudioStreamPlayer3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/3d/visual_instance_3d.h"
#include "core/config/project_settings.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/main/viewport.h"
#include "servers/audio/audio_stream.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/skeleton_3d.h"
#endif // PHYSICS_3D_DISABLED

// Based on "A Novel Multichannel Panning Method for Standard and Arbitrary Loudspeaker
// Configurations" by Ramy Sadek and Chris Kyriakakis (2004) Speaker-Placement Correction Amplitude
// Panning (SPCAP)
class Spcap
{
private:
	struct Speaker
	{
		Vector3 direction;
		real_t effective_number_of_speakers = 0; // precalculated
		mutable real_t squared_gain = 0;		 // temporary
	};

	Vector<Speaker> speakers;

public:
	Spcap(unsigned int speaker_count, const Vector3* speaker_directions)
	{
		speakers.resize(speaker_count);
		Speaker* w = speakers.ptrw();
		for (unsigned int speaker_num = 0; speaker_num < speaker_count; speaker_num++) {
			w[speaker_num].direction = speaker_directions[speaker_num];
			w[speaker_num].squared_gain = 0.0;
			w[speaker_num].effective_number_of_speakers = 0.0;
		}
		for (unsigned int speaker_num = 0; speaker_num < speaker_count; speaker_num++) {
			for (unsigned int other_speaker_num = 0; other_speaker_num < speaker_count;
				 other_speaker_num++) {
				w[speaker_num].effective_number_of_speakers +=
					0.5 * (1.0 + w[speaker_num].direction.dot(w[other_speaker_num].direction));
			}
		}
	}

	unsigned int get_speaker_count() const { return (unsigned int)speakers.size(); }

	Vector3 get_speaker_direction(unsigned int index) const
	{
		return speakers.ptr()[index].direction;
	}

	void calculate(const Vector3& source_direction, real_t tightness, unsigned int volume_count,
		real_t* volumes) const
	{
		const Speaker* r = speakers.ptr();
		real_t sum_squared_gains = 0.0;
		for (unsigned int speaker_num = 0; speaker_num < (unsigned int)speakers.size();
			 speaker_num++) {
			real_t initial_gain =
				0.5 * std::pow(1.0 + r[speaker_num].direction.dot(source_direction), tightness) /
				r[speaker_num].effective_number_of_speakers;
			r[speaker_num].squared_gain = initial_gain * initial_gain;
			sum_squared_gains += r[speaker_num].squared_gain;
		}

		for (unsigned int speaker_num = 0;
			 speaker_num < MIN(volume_count, (unsigned int)speakers.size()); speaker_num++) {
			volumes[speaker_num] = std::sqrt(r[speaker_num].squared_gain / sum_squared_gains);
		}
	}
};

// TODO: hardcoded main speaker directions for 2, 3.1, 5.1 and 7.1 setups - these are simplified and
// could also be made configurable
static const Vector3 speaker_directions[7] = {
	Vector3(-1.0, 0.0, -1.0).normalized(), // front-left
	Vector3(1.0, 0.0, -1.0).normalized(),  // front-right
	Vector3(0.0, 0.0, -1.0).normalized(),  // center
	Vector3(-1.0, 0.0, 1.0).normalized(),  // rear-left
	Vector3(1.0, 0.0, 1.0).normalized(),   // rear-right
	Vector3(-1.0, 0.0, 0.0).normalized(),  // side-left
	Vector3(1.0, 0.0, 0.0).normalized(),   // side-right
};

void AudioStreamPlayer3D::_calc_output_vol(const Vector3& source_dir, real_t tightness,
	FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& output)
{
	unsigned int speaker_count = 0; // only main speakers (no LFE)
	switch (AudioServer::get_speaker_mode()) {
	case AudioServer::SPEAKER_MODE_STEREO:
		speaker_count = 2;
		break;
	case AudioServer::SPEAKER_SURROUND_31:
		speaker_count = 3;
		break;
	case AudioServer::SPEAKER_SURROUND_51:
		speaker_count = 5;
		break;
	case AudioServer::SPEAKER_SURROUND_71:
		speaker_count = 7;
		break;
	}

	Spcap spcap(speaker_count, speaker_directions); // TODO: should only be created/recreated once
													// the speaker mode / speaker positions changes
	real_t volumes[7] = {};
	spcap.calculate(source_dir, tightness, speaker_count, volumes);

	switch (AudioServer::get_speaker_mode()) {
	case AudioServer::SPEAKER_SURROUND_71:
		output[3].left = volumes[5];  // side-left
		output[3].right = volumes[6]; // side-right
		[[fallthrough]];
	case AudioServer::SPEAKER_SURROUND_51:
		output[2].left = volumes[3];  // rear-left
		output[2].right = volumes[4]; // rear-right
		[[fallthrough]];
	case AudioServer::SPEAKER_SURROUND_31:
		output[1].right = 1.0;		 // LFE - always full power
		output[1].left = volumes[2]; // center
		[[fallthrough]];
	case AudioServer::SPEAKER_MODE_STEREO:
		output[0].right = volumes[1]; // front-right
		output[0].left = volumes[0];  // front-left
		break;
	}
}

// Set the volume to cosine of half horizontal the angle from the source to the left/right speaker
// direction ignoring elevation. Then scale `cosx` so that greatest ratio of the speaker volumes is
// `1-panning_strength`. See https://github.com/godotengine/godot/issues/103989 for evidence that
// this is the most standard implementation.
AudioFrame AudioStreamPlayer3D::_calc_output_vol_stereo(
	const Vector3& source_dir, real_t panning_strength)
{
	double flatrad = sqrt(source_dir.x * source_dir.x + source_dir.z * source_dir.z);
	double g = CLAMP((1.0 - panning_strength) * (1.0 - panning_strength), 0.0, 1.0);
	double f = (1.0 - g) / (1.0 + g);
	double cosx = CLAMP(source_dir.x / (flatrad == 0.0 ? 1.0 : flatrad), -1.0, 1.0);
	double fcosx = cosx * f;
	return AudioFrame(sqrt((-fcosx + 1.0) / 2.0), sqrt((fcosx + 1.0) / 2.0));
}

#ifndef PHYSICS_3D_DISABLED
void AudioStreamPlayer3D::_calc_reverb_vol(Area3D* area, Vector3 listener_area_pos,
	const FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& direct_path_vol,
	FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& reverb_vol)
{
	reverb_vol.resize_uninitialized(VOLUME_VECTOR_SIZE);
	for (AudioFrame& frame : reverb_vol) {
		frame = AudioFrame(0, 0);
	}

	float uniformity = area->get_reverb_uniformity();
	float area_send = area->get_reverb_amount();

	if (uniformity > 0.0) {
		float distance = listener_area_pos.length();
		float attenuation = Math::db_to_linear(_get_attenuation_db(distance));

		// Determine the fraction of sound that would come from each speaker if they were all driven
		// uniformly.
		float center_val[3] = {0.5f, 0.25f, 0.16666f};
		int channel_count = AudioServer::get_channel_count();
		AudioFrame center_frame(center_val[channel_count - 1], center_val[channel_count - 1]);

		if (attenuation < 1.0) {
			// pan the uniform sound
			Vector3 rev_pos = listener_area_pos;
			rev_pos.y = 0;
			rev_pos.normalize();

			// Stereo pair.
			float c = rev_pos.x * 0.5 + 0.5;
			reverb_vol[0].left = 1.0 - c;
			reverb_vol[0].right = c;

			if (channel_count >= 3) {
				// Center pair + Side pair
				float xl = Vector3(-1, 0, -1).normalized().dot(rev_pos) * 0.5 + 0.5;
				float xr = Vector3(1, 0, -1).normalized().dot(rev_pos) * 0.5 + 0.5;

				reverb_vol[1].left = xl;
				reverb_vol[1].right = xr;
				reverb_vol[2].left = 1.0 - xr;
				reverb_vol[2].right = 1.0 - xl;
			}

			if (channel_count >= 4) {
				// Rear pair
				// FIXME: Not sure what math should be done here
				reverb_vol[3].left = 1.0 - c;
				reverb_vol[3].right = c;
			}

			for (int i = 0; i < channel_count; i++) {
				reverb_vol[i] = reverb_vol[i].lerp(center_frame, attenuation);
			}
		}
		else {
			for (int i = 0; i < channel_count; i++) {
				reverb_vol[i] = center_frame;
			}
		}

		for (int i = 0; i < channel_count; i++) {
			reverb_vol[i] = direct_path_vol[i].lerp(reverb_vol[i] * attenuation, uniformity);
			reverb_vol[i] *= area_send;
		}

	}
	else {
		for (int i = 0; i < 4; i++) {
			reverb_vol[i] = direct_path_vol[i] * area_send;
		}
	}
}
#endif // PHYSICS_3D_DISABLED

float AudioStreamPlayer3D::_get_attenuation_db(float p_distance) const
{
	float att = 0;
	switch (attenuation_model) {
	case ATTENUATION_INVERSE_DISTANCE: {
		att = Math::linear_to_db(1.0 / ((p_distance / unit_size) + CMP_EPSILON));
	} break;
	case ATTENUATION_INVERSE_SQUARE_DISTANCE: {
		float d = (p_distance / unit_size);
		d *= d;
		att = Math::linear_to_db(1.0 / (d + CMP_EPSILON));
	} break;
	case ATTENUATION_LOGARITHMIC: {
		att = -20 * Math::log(p_distance / unit_size + CMP_EPSILON);
	} break;
	case ATTENUATION_DISABLED:
		break;
	default: {
		ERR_PRINT("Unknown attenuation type");
		break;
	}
	}

	att += internal->volume_db;
	if (att > max_db) {
		att = max_db;
	}

	return att;
}

void AudioStreamPlayer3D::_notification(int p_what)
{
	internal->notification(p_what);
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		velocity_tracker->reset(get_global_transform().origin);
		AudioServer::add_listener_changed_callback(_listener_changed_cb, this);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		AudioServer::remove_listener_changed_callback(_listener_changed_cb, this);
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
			velocity_tracker->update_position(get_global_transform().origin);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		// Update anything related to position first, if possible of course.
		Vector<AudioFrame> volume_vector;
		if (setplay.get() > 0 ||
			(internal->active.is_set() &&
				last_mix_count != AudioServer::get_mix_count()) ||
			force_update_panning) {
			force_update_panning = false;
			volume_vector = _update_panning();
		}

		if (setplayback.is_valid() && setplay.get() >= 0) {
			internal->active.set();
			HashMap<String, Vector<AudioFrame>> bus_map;
			bus_map[_get_actual_bus()] = volume_vector;
			AudioServer::start_playback_stream(setplayback, bus_map, setplay.get(),
				actual_pitch_scale, linear_attenuation, attenuation_filter_cutoff_hz);
			setplayback.unref();
			setplay.set(-1);
		}

		if (!internal->stream_playbacks.is_empty() && internal->active.is_set()) {
			internal->process();
		}
		internal->ensure_playback_limit();
	} break;
	}
}

// Interacts with PhysicsServer3D, so can only be called during _physics_process.
String AudioStreamPlayer3D::_get_actual_bus()
{
#ifndef PHYSICS_3D_DISABLED
	Area3D* overriding_area = _get_overriding_area();
	if (overriding_area && overriding_area->is_overriding_audio_bus() &&
		!overriding_area->is_using_reverb_bus()) {
		return overriding_area->get_audio_bus_name();
	}
#endif // PHYSICS_3D_DISABLED
	return internal->bus;
}

static void _apply_max_volume_from_vector(Vector<AudioFrame>& r_tgt_volume_vector,
	const FixedVector<AudioFrame, AudioServer::MAX_CHANNELS_PER_BUS>& p_src_volume_vector)
{
	// note: indexed loops with Vector are slow, so we use raw pointers on purpose here
	AudioFrame* tgt_ptr = r_tgt_volume_vector.ptrw();
	const AudioFrame* src_ptr = p_src_volume_vector.ptr();

	for (int64_t i = 0; i < r_tgt_volume_vector.size(); i++) {
		tgt_ptr[i].left = MAX(src_ptr[i].left, tgt_ptr[i].left);
		tgt_ptr[i].right = MAX(src_ptr[i].right, tgt_ptr[i].right);
	}
}

static float _get_max_volume(
	const FixedVector<AudioFrame, AudioServer::MAX_CHANNELS_PER_BUS>& p_src_volume_vector)
{
	float max_vol = 0.0;
	for (const AudioFrame& frame : p_src_volume_vector) {
		max_vol = MAX(max_vol, MAX(frame.left, frame.right));
	}
	return max_vol;
}

// Interacts with PhysicsServer3D, so can only be called during _physics_process.
Vector<AudioFrame> AudioStreamPlayer3D::_update_panning()
{
	Vector<AudioFrame> output_volume_vector;
	output_volume_vector.resize(VOLUME_VECTOR_SIZE);
	output_volume_vector.fill(AudioFrame(0, 0));

	if (!internal->active.is_set() || internal->stream.is_null()) {
		return output_volume_vector;
	}

	Vector3 linear_velocity;

	// compute linear velocity for doppler
	if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
		linear_velocity = velocity_tracker->get_tracked_linear_velocity();
	}

	Vector3 global_pos = get_global_transform().origin;

	Ref<World3D> world_3d = get_world_3d();
	ERR_FAIL_COND_V(world_3d.is_null(), output_volume_vector);

	HashSet<Camera3D*> cameras(world_3d->get_cameras());
	cameras.insert(get_viewport()->get_camera_3d());

#ifndef PHYSICS_3D_DISABLED
	PhysicsDirectSpaceState3D* space_state =
		PhysicsServer3D::space_get_direct_state(world_3d->get_space());
	Area3D* area = _get_overriding_area();
#endif // PHYSICS_3D_DISABLED

	// set pitch to default value
	// this will only change if the Doppler effect is activated
	actual_pitch_scale = internal->pitch_scale;

	Vector<AudioFrame> output_reverb_vector;
	output_reverb_vector.resize(VOLUME_VECTOR_SIZE);
	output_reverb_vector.fill(AudioFrame(0, 0));

	// keep track of a weighted average of the pitch
	float pitch_scale_sum = 0.0F;
	float pitch_scale_weight = 0.0F;

	bool has_any_listener_in_range = false;
	linear_attenuation = 0;
	for (Camera3D* camera : cameras) {
		if (!camera) {
			continue;
		}
		Viewport* vp = camera->get_viewport();
		if (!vp) {
			continue;
		}
		if (!vp->is_audio_listener_3d()) {
			continue;
		}

		Node3D* listener_node = camera;

		AudioListener3D* listener = vp->get_audio_listener_3d();
		if (listener) {
			listener_node = listener;
		}

		Vector3 local_pos =
			listener_node->get_global_transform().orthonormalized().affine_inverse().xform(
				global_pos);

		float dist = local_pos.length();

#ifndef PHYSICS_3D_DISABLED
		Vector3 area_sound_pos;
		Vector3 listener_area_pos;

		if (area && area->is_using_reverb_bus() && area->get_reverb_uniformity() > 0) {
			area_sound_pos = space_state->get_closest_point_to_object_volume(
				area->get_rid(), listener_node->get_global_transform().origin);
			listener_area_pos =
				listener_node->get_global_transform().affine_inverse().xform(area_sound_pos);
		}
#endif // PHYSICS_3D_DISABLED

		if (max_distance > 0) {
			float total_max = max_distance;

#ifndef PHYSICS_3D_DISABLED
			if (area && area->is_using_reverb_bus() && area->get_reverb_uniformity() > 0) {
				total_max = MAX(total_max, listener_area_pos.length());
			}
#endif // PHYSICS_3D_DISABLED
			if (dist > total_max || total_max > max_distance) {
				continue; // can't hear this sound in this listener
			}
		}
		has_any_listener_in_range = true;

		float multiplier = Math::db_to_linear(_get_attenuation_db(dist));
		if (max_distance > 0) {
			multiplier *= MAX(0, 1.0 - (dist / max_distance));
		}

		float db_att = (1.0 - MIN(1.0, multiplier)) * attenuation_filter_db;

		if (emission_angle_enabled) {
			Vector3 listenertopos = global_pos - listener_node->get_global_transform().origin;
			float c = listenertopos.normalized().dot(
				get_global_transform().basis.get_column(2).normalized()); // it's z negative
			float angle = Math::rad_to_deg(Math::acos(c));
			if (angle > emission_angle) {
				db_att -= -emission_angle_filter_attenuation_db;
			}
		}

		linear_attenuation = MAX(linear_attenuation, Math::db_to_linear(db_att));

		FixedVector<AudioFrame, VOLUME_VECTOR_SIZE> listener_volume_vector;
		listener_volume_vector.resize_uninitialized(VOLUME_VECTOR_SIZE);
		for (AudioFrame& frame : listener_volume_vector) {
			frame = AudioFrame(0, 0);
		}

		if (AudioServer::get_speaker_mode() == AudioServer::SPEAKER_MODE_STEREO) {
			listener_volume_vector[0] = _calc_output_vol_stereo(
				local_pos, cached_global_panning_strength * panning_strength);
		}
		else {
			// Bake in a constant factor here to allow the project setting defaults for 2d and 3d to
			// be normalized to 1.0.
			float tightness = cached_global_panning_strength * 2.0f;
			tightness *= panning_strength;
			_calc_output_vol(local_pos.normalized(), tightness, listener_volume_vector);
		}

		for (AudioFrame& frame : listener_volume_vector) {
			frame *= multiplier;
		}

		_apply_max_volume_from_vector(output_volume_vector, listener_volume_vector);

#ifndef PHYSICS_3D_DISABLED

		if (area && area->is_using_reverb_bus()) {
			FixedVector<AudioFrame, VOLUME_VECTOR_SIZE> listener_reverb_vector;
			_calc_reverb_vol(
				area, listener_area_pos, listener_volume_vector, listener_reverb_vector);
			_apply_max_volume_from_vector(output_reverb_vector, listener_reverb_vector);
		}
#endif

		if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
			Vector3 listener_velocity;

			if (listener) {
				listener_velocity = listener->get_doppler_tracked_velocity();
			}
			else {
				listener_velocity = camera->get_doppler_tracked_velocity();
			}

			const Vector3 local_velocity =
				listener_node->get_global_transform().orthonormalized().basis.xform_inv(
					linear_velocity - listener_velocity);
			if (local_velocity != Vector3()) {
				const float approaching = local_pos.normalized().dot(local_velocity.normalized());
				const float velocity = local_velocity.length();
				static constexpr float speed_of_sound = 343.0F;

				float doppler_pitch_scale = internal->pitch_scale * speed_of_sound /
											(speed_of_sound + velocity * approaching);

				// limit the pitch scale, so we do not get execeedingly extreme pitch effects in
				// edge cases
				doppler_pitch_scale = CLAMP(doppler_pitch_scale, (1.0F / 8.0F), 8.0F);

				// just use the maximum volume of the current volume vector as weight
				// so the pitch effect fades out with lower volumes
				float weight = _get_max_volume(listener_volume_vector);
				pitch_scale_sum += weight * doppler_pitch_scale;
				pitch_scale_weight += weight;
			}
		}
	}

	if (pitch_scale_weight > 0.0F) {
		actual_pitch_scale = pitch_scale_sum / pitch_scale_weight;
	}

	for (Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_highshelf_params(
			playback, linear_attenuation, attenuation_filter_cutoff_hz);
	}

	bus_volumes.clear();
	if (has_any_listener_in_range) {
#ifndef PHYSICS_3D_DISABLED
		if (area) {
			if (area->is_overriding_audio_bus()) {
				// override audio bus
				bus_volumes[area->get_audio_bus_name()] = output_volume_vector;
			}

			if (area->is_using_reverb_bus()) {
				String reverb_bus_name = area->get_reverb_bus_name();
				bus_volumes[reverb_bus_name] = output_reverb_vector;
			}
		}
		else
#endif // PHYSICS_3D_DISABLED
		{
			bus_volumes[internal->bus] = output_volume_vector;
		}
	}

	// If no listeners are in range and this was the case last frame, then we can skip setting any
	// audio.
	const bool skip_setting_volumes =
		!has_any_listener_in_range && was_further_than_max_distance_last_frame;
	was_further_than_max_distance_last_frame = !has_any_listener_in_range;

	if (!skip_setting_volumes) {
		for (Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
			AudioServer::set_playback_bus_volumes_linear(playback, bus_volumes);
		}

		for (Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
			AudioServer::set_playback_pitch_scale(playback, actual_pitch_scale);
			if (playback->get_is_sample()) {
				Ref<AudioSamplePlayback> sample_playback = playback->get_sample_playback();
				if (sample_playback.is_valid()) {
					AudioServer::update_sample_playback_pitch_scale(
						sample_playback, actual_pitch_scale);
				}
			}
		}
	}
	return output_volume_vector;
}

void AudioStreamPlayer3D::set_stream(Ref<AudioStream> p_stream) { internal->set_stream(p_stream); }

Ref<AudioStream> AudioStreamPlayer3D::get_stream() const { return internal->stream; }

void AudioStreamPlayer3D::set_volume_db(float p_volume)
{
	ERR_FAIL_COND_MSG(Math::is_nan(p_volume), "Volume can't be set to NaN.");
	internal->volume_db = p_volume;
}

float AudioStreamPlayer3D::get_volume_db() const { return internal->volume_db; }

void AudioStreamPlayer3D::set_volume_linear(float p_volume)
{
	set_volume_db(Math::linear_to_db(p_volume));
}

float AudioStreamPlayer3D::get_volume_linear() const { return Math::db_to_linear(get_volume_db()); }

float AudioStreamPlayer3D::get_unit_size() const { return unit_size; }

void AudioStreamPlayer3D::set_max_db(float p_boost) { max_db = p_boost; }

float AudioStreamPlayer3D::get_max_db() const { return max_db; }

void AudioStreamPlayer3D::set_pitch_scale(float p_pitch_scale)
{
	internal->set_pitch_scale(p_pitch_scale);
}

float AudioStreamPlayer3D::get_pitch_scale() const { return internal->pitch_scale; }

void AudioStreamPlayer3D::play(float p_from_pos)
{
	Ref<AudioStreamPlayback> stream_playback = internal->play_basic();
	if (stream_playback.is_null()) {
		return;
	}
	setplayback = stream_playback;
	setplay.set(p_from_pos);

	// Sample handling.
	if (stream_playback->get_is_sample() && stream_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = stream_playback->get_sample_playback();
		sample_playback->offset = p_from_pos;
		sample_playback->bus = _get_actual_bus();

		AudioServer::start_sample_playback(sample_playback);
	}
}

void AudioStreamPlayer3D::seek(float p_seconds) { internal->seek(p_seconds); }

void AudioStreamPlayer3D::stop()
{
	setplay.set(-1);
	internal->stop_basic();
}

bool AudioStreamPlayer3D::is_playing() const
{
	if (setplay.get() >= 0) {
		return true; // play() has been called this frame, but no playback exists just yet.
	}
	return internal->is_playing();
}

float AudioStreamPlayer3D::get_playback_position()
{
	if (setplay.get() >= 0) {
		return setplay.get(); // play() has been called this frame, but no playback exists just yet.
	}
	return internal->get_playback_position();
}

void AudioStreamPlayer3D::set_bus(const String& p_bus)
{
	internal->bus = p_bus; // This will be pushed to the audio server during the next physics
						   // timestep, which is fast enough.
}

String AudioStreamPlayer3D::get_bus() const { return internal->get_bus(); }

void AudioStreamPlayer3D::set_autoplay(bool p_enable) { internal->autoplay = p_enable; }

bool AudioStreamPlayer3D::is_autoplay_enabled() const { return internal->autoplay; }

void AudioStreamPlayer3D::_set_playing(bool p_enable) { internal->set_playing(p_enable); }

float AudioStreamPlayer3D::get_max_distance() const { return max_distance; }

void AudioStreamPlayer3D::set_area_mask(uint32_t p_mask) { area_mask = p_mask; }

uint32_t AudioStreamPlayer3D::get_area_mask() const { return area_mask; }

bool AudioStreamPlayer3D::is_emission_angle_enabled() const { return emission_angle_enabled; }

float AudioStreamPlayer3D::get_emission_angle() const { return emission_angle; }

void AudioStreamPlayer3D::set_emission_angle_filter_attenuation_db(float p_angle_attenuation_db)
{
	emission_angle_filter_attenuation_db = p_angle_attenuation_db;
}

float AudioStreamPlayer3D::get_emission_angle_filter_attenuation_db() const
{
	return emission_angle_filter_attenuation_db;
}

void AudioStreamPlayer3D::set_attenuation_filter_cutoff_hz(float p_hz)
{
	attenuation_filter_cutoff_hz = p_hz;
}

float AudioStreamPlayer3D::get_attenuation_filter_cutoff_hz() const
{
	return attenuation_filter_cutoff_hz;
}

void AudioStreamPlayer3D::set_attenuation_filter_db(float p_db) { attenuation_filter_db = p_db; }

float AudioStreamPlayer3D::get_attenuation_filter_db() const { return attenuation_filter_db; }

AudioStreamPlayer3D::AttenuationModel AudioStreamPlayer3D::get_attenuation_model() const
{
	return attenuation_model;
}

void AudioStreamPlayer3D::set_doppler_tracking(DopplerTracking p_tracking)
{
	if (doppler_tracking == p_tracking) {
		return;
	}

	doppler_tracking = p_tracking;

	if (doppler_tracking != DOPPLER_TRACKING_DISABLED) {
		set_notify_transform(true);
		velocity_tracker->set_track_physics_step(doppler_tracking == DOPPLER_TRACKING_PHYSICS_STEP);
		if (is_inside_tree()) {
			velocity_tracker->reset(get_global_transform().origin);
		}
	}
	else {
		set_notify_transform(false);
	}
}

AudioStreamPlayer3D::DopplerTracking AudioStreamPlayer3D::get_doppler_tracking() const
{
	return doppler_tracking;
}

void AudioStreamPlayer3D::set_stream_paused(bool p_pause) { internal->set_stream_paused(p_pause); }

bool AudioStreamPlayer3D::get_stream_paused() const { return internal->get_stream_paused(); }

bool AudioStreamPlayer3D::has_stream_playback() { return internal->has_stream_playback(); }

Ref<AudioStreamPlayback> AudioStreamPlayer3D::get_stream_playback()
{
	return internal->get_stream_playback();
}

void AudioStreamPlayer3D::set_max_polyphony(int p_max_polyphony)
{
	internal->set_max_polyphony(p_max_polyphony);
}

int AudioStreamPlayer3D::get_max_polyphony() const { return internal->max_polyphony; }

void AudioStreamPlayer3D::set_panning_strength(float p_panning_strength)
{
	ERR_FAIL_COND_MSG(p_panning_strength < 0, "Panning strength must be a positive number.");
	panning_strength = p_panning_strength;
}

float AudioStreamPlayer3D::get_panning_strength() const { return panning_strength; }

AudioServer::PlaybackType AudioStreamPlayer3D::get_playback_type() const
{
	return internal->get_playback_type();
}

void AudioStreamPlayer3D::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	internal->set_playback_type(p_playback_type);
}

AudioStreamPlayer3D::~AudioStreamPlayer3D() { memdelete(internal); }



Area3D* AudioStreamPlayer3D::_get_overriding_area() {}

// ==========================================================================
// Merged from scene/3d/world_environment.cpp
// ==========================================================================

#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/3d/visual_instance_3d.h"

void WorldEnvironment::_notification(int p_what)
{
	switch (p_what) {
	case Node3D::NOTIFICATION_ENTER_WORLD:
	case Node3D::NOTIFICATION_ENTER_TREE: {
		if (environment.is_valid()) {
			add_to_group("_world_environment_" +
						 itos(get_viewport()->find_world_3d()->get_scenario().get_id()));
			_update_current_environment();
		}

		if (camera_attributes.is_valid()) {
			add_to_group("_world_camera_attributes_" +
						 itos(get_viewport()->find_world_3d()->get_scenario().get_id()));
			_update_current_camera_attributes();
		}

		if (compositor.is_valid()) {
			add_to_group("_world_compositor_" +
						 itos(get_viewport()->find_world_3d()->get_scenario().get_id()));
			_update_current_compositor();
		}
	} break;

	case Node3D::NOTIFICATION_EXIT_WORLD:
	case Node3D::NOTIFICATION_EXIT_TREE: {
		if (environment.is_valid()) {
			remove_from_group("_world_environment_" +
							  itos(get_viewport()->find_world_3d()->get_scenario().get_id()));
			_update_current_environment();
		}

		if (camera_attributes.is_valid()) {
			remove_from_group("_world_camera_attributes_" +
							  itos(get_viewport()->find_world_3d()->get_scenario().get_id()));
			_update_current_camera_attributes();
		}

		if (compositor.is_valid()) {
			remove_from_group("_world_compositor_" +
							  itos(get_viewport()->find_world_3d()->get_scenario().get_id()));
			_update_current_compositor();
		}
	} break;
	}
}

Ref<Environment> WorldEnvironment::get_environment() const { return environment; }

Ref<CameraAttributes> WorldEnvironment::get_camera_attributes() const { return camera_attributes; }

Ref<Compositor> WorldEnvironment::get_compositor() const { return compositor; }

PackedStringArray WorldEnvironment::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (environment.is_null() && camera_attributes.is_null()) {
		warnings.push_back(
			RTR("To have any visible effect, WorldEnvironment requires its \"Environment\" "
				"property to contain an Environment, its \"Camera Attributes\" property to contain "
				"a CameraAttributes resource, or both."));
	}

	if (!is_inside_tree()) {
		return warnings;
	}

	if (environment.is_valid() &&
		get_viewport()->find_world_3d()->get_environment() != environment) {
		warnings.push_back(("Only the first Environment has an effect in a scene (or set of "
							"instantiated scenes)."));
	}

	if (camera_attributes.is_valid() &&
		get_viewport()->find_world_3d()->get_camera_attributes() != camera_attributes) {
		warnings.push_back(
			RTR("Only one WorldEnvironment is allowed per scene (or set of instantiated scenes)."));
	}

	if (compositor.is_valid() && get_viewport()->find_world_3d()->get_compositor() != compositor) {
		warnings.push_back((
			"Only the first Compositor has an effect in a scene (or set of instantiated scenes)."));
	}

	return warnings;
}

WorldEnvironment::WorldEnvironment() {}



void WorldEnvironment::_update_current_environment() {}

void WorldEnvironment::_update_current_camera_attributes() {}

void WorldEnvironment::_update_current_compositor() {}

// ============================================================================
// Merged from scene/3d/skeleton_3d.cpp
// ============================================================================

#include "scene/3d/skeleton_3d.h"
// --- skeleton_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "skeleton_3d.h"

void Skeleton3D::_add_bone_bind_compat_88791(const String& p_name) { add_bone(p_name); }

void Skeleton3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "skeleton_3d.h"
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
#include "scene/3d/skeleton_3d.h"
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED
#include "servers/rendering/renderer.h"

void SkinReference::_skin_changed()
{
	if (skeleton_node) {
		skeleton_node->_make_dirty();
	}
	skeleton_version = 0;
}

void Skeleton3D::_make_dirty() {}

RID SkinReference::get_skeleton() const { return skeleton; }

Ref<Skin> SkinReference::get_skin() const { return skin; }

SkinReference::~SkinReference()
{
	ERR_FAIL_NULL(Renderer::data);
	if (skeleton_node) {
		skeleton_node->skin_bindings.erase(this);
	}
	Renderer::free_rid(skeleton);
}

void Skeleton3D::_update_bone_names() const
{
	String names;
	for (uint32_t i = 0; i < bones.size(); i++) {
		if (i > 0) {
			names += ",";
		}
		names += bones[i].name;
	}
	concatenated_bone_names = String(names);
}

String Skeleton3D::get_concatenated_bone_names() const
{
	if (concatenated_bone_names == String()) {
		_update_bone_names();
	}
	return concatenated_bone_names;
}

void Skeleton3D::set_modifier_callback_mode_process(Skeleton3D::ModifierCallbackModeProcess p_mode)
{
	if (modifier_callback_mode_process == p_mode) {
		return;
	}
	modifier_callback_mode_process = p_mode;
	_process_changed();
}

Skeleton3D::ModifierCallbackModeProcess Skeleton3D::get_modifier_callback_mode_process() const
{
	return modifier_callback_mode_process;
}

void Skeleton3D::_process_changed()
{
	if (modifier_callback_mode_process == MODIFIER_CALLBACK_MODE_PROCESS_IDLE) {
		set_process_internal(true);
		set_physics_process_internal(false);
	}
	else if (modifier_callback_mode_process == MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS) {
		set_process_internal(false);
		set_physics_process_internal(true);
	}
	else {
		set_process_internal(false);
		set_physics_process_internal(false);
	}
}

void Skeleton3D::_make_modifiers_dirty()
{
	modifiers_dirty = true;
	_update_deferred(UPDATE_FLAG_MODIFIER);
}

void Skeleton3D::_update_bones_nested_set() const
{
	nested_set_offset_to_bone_index.resize(bones.size());
	bone_global_pose_dirty.resize(bones.size());
	_make_bone_global_poses_dirty();

	int offset = 0;
	for (int bone : parentless_bones) {
		offset += _update_bone_nested_set(bone, offset);
	}
}

int Skeleton3D::_update_bone_nested_set(int p_bone, int p_offset) const
{
	Bone& bone = bones[p_bone];
	int offset = p_offset + 1;
	int span = 1;

	for (int child_bone : bone.child_bones) {
		int subspan = _update_bone_nested_set(child_bone, offset);
		offset += subspan;
		span += subspan;
	}

	nested_set_offset_to_bone_index[p_offset] = p_bone;
	bone.nested_set_offset = p_offset;
	bone.nested_set_span = span;

	return span;
}

void Skeleton3D::_make_bone_global_poses_dirty() const
{
	for (uint32_t i = 0; i < bone_global_pose_dirty.size(); i++) {
		bone_global_pose_dirty[i] = true;
	}
}

void Skeleton3D::_make_bone_global_pose_subtree_dirty(int p_bone) const
{
	if (process_order_dirty) {
		return;
	}

	const Bone& bone = bones[p_bone];
	int span_offset = bone.nested_set_offset;
	// No need to make subtree dirty when bone is already dirty.
	if (bone_global_pose_dirty[span_offset]) {
		return;
	}

	// Make global poses of subtree dirty.
	int span_end = span_offset + bone.nested_set_span;
	for (int i = span_offset; i < span_end; i++) {
		bone_global_pose_dirty[i] = true;
	}
}

void Skeleton3D::_update_bone_global_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	_update_process_order();

	// Global pose is already calculated.
	int nested_set_offset = bones[p_bone].nested_set_offset;
	if (!bone_global_pose_dirty[nested_set_offset]) {
		return;
	}

	thread_local LocalVector<int> bone_list;
	bone_list.clear();
	Transform3D global_pose;

	// Create list of parent bones for which the global pose needs to be recalculated.
	for (int bone = p_bone; bone >= 0; bone = bones[bone].parent) {
		int offset = bones[bone].nested_set_offset;
		// Stop searching when global pose is not dirty.
		if (!bone_global_pose_dirty[offset]) {
			global_pose = bones[bone].global_pose;
			break;
		}

		bone_list.push_back(bone);
	}

	// Calculate global poses for all parent bones and the current bone.
	for (int i = bone_list.size() - 1; i >= 0; i--) {
		int bone_idx = bone_list[i];
		Bone& bone = bones[bone_idx];
		bool bone_enabled = bone.enabled && !show_rest_only;
		Transform3D bone_pose = bone_enabled ? get_bone_pose(bone_idx) : get_bone_rest(bone_idx);

		global_pose *= bone_pose;
#ifndef DISABLE_DEPRECATED
		if (bone.global_pose_override_amount >= CMP_EPSILON) {
			global_pose = global_pose.interpolate_with(
				bone.global_pose_override, bone.global_pose_override_amount);
		}
#endif // _DISABLE_DEPRECATED

		bone.global_pose = global_pose;
		bone_global_pose_dirty[bone.nested_set_offset] = false;
	}
}

Transform3D Skeleton3D::get_bone_global_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	_update_bone_global_pose(p_bone);
	return bones[p_bone].global_pose;
}

void Skeleton3D::set_bone_global_pose(int p_bone, const Transform3D& p_pose)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	Transform3D pt;
	if (bones[p_bone].parent >= 0) {
		pt = get_bone_global_pose(bones[p_bone].parent);
	}
	Transform3D t = pt.affine_inverse() * p_pose;
	set_bone_pose(p_bone, t);
}

void Skeleton3D::set_motion_scale(float p_motion_scale)
{
	if (p_motion_scale <= 0) {
		motion_scale = 1;
		ERR_FAIL_MSG("Motion scale must be larger than 0.");
	}
	motion_scale = p_motion_scale;
}

float Skeleton3D::get_motion_scale() const
{
	ERR_FAIL_COND_V(motion_scale <= 0, 1);
	return motion_scale;
}

// Skeleton creation api

uint64_t Skeleton3D::get_version() const { return version; }

int Skeleton3D::find_bone(const String& p_name) const
{
	const int* bone_index_ptr = name_to_bone_index.getptr(p_name);
	return bone_index_ptr != nullptr ? *bone_index_ptr : -1;
}

String Skeleton3D::get_bone_name(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, "");
	return bones[p_bone].name;
}

void Skeleton3D::set_bone_name(int p_bone, const String& p_name)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	const int* bone_index_ptr = name_to_bone_index.getptr(p_name);
	if (bone_index_ptr != nullptr) {
		ERR_FAIL_COND_MSG(*bone_index_ptr != p_bone,
			"Skeleton3D: '" + get_name() + "', bone name:  '" + p_name + "' already exists.");
		return; // No need to rename, the bone already has the given name.
	}

	name_to_bone_index.erase(bones[p_bone].name);
	bones[p_bone].name = p_name;
	name_to_bone_index.insert(p_name, p_bone);

	version++;
}

bool Skeleton3D::is_bone_parent_of(int p_bone, int p_parent_bone_id) const
{
	int parent_of_bone = get_bone_parent(p_bone);

	if (-1 == parent_of_bone) {
		return false;
	}

	if (parent_of_bone == p_parent_bone_id) {
		return true;
	}

	return is_bone_parent_of(parent_of_bone, p_parent_bone_id);
}

int Skeleton3D::get_bone_count() const { return bones.size(); }

void Skeleton3D::set_bone_parent(int p_bone, int p_parent)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	ERR_FAIL_COND(p_parent != -1 && (p_parent < 0));
	ERR_FAIL_COND(p_bone == p_parent);

	bones[p_bone].parent = p_parent;
	process_order_dirty = true;
	rest_dirty = true;
	_make_dirty();
}

void Skeleton3D::unparent_bone_and_rest(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	_update_process_order();

	int parent = bones[p_bone].parent;
	while (parent >= 0) {
		bones[p_bone].rest = bones[parent].rest * bones[p_bone].rest;
		parent = bones[parent].parent;
	}

	bones[p_bone].parent = -1;
	process_order_dirty = true;

	rest_dirty = true;
	_make_dirty();
}

int Skeleton3D::get_bone_parent(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, -1);
	if (process_order_dirty) {
		_update_process_order();
	}
	return bones[p_bone].parent;
}

Vector<int> Skeleton3D::get_bone_children(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector<int>());
	if (process_order_dirty) {
		_update_process_order();
	}
	return bones[p_bone].child_bones;
}

Vector<int> Skeleton3D::get_parentless_bones() const
{
	if (process_order_dirty) {
		_update_process_order();
	}
	return parentless_bones;
}

void Skeleton3D::set_bone_rest(int p_bone, const Transform3D& p_rest)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);

	bones[p_bone].rest = p_rest;
	rest_dirty = true;
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}

Transform3D Skeleton3D::get_bone_rest(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());

	return bones[p_bone].rest;
}

Transform3D Skeleton3D::get_bone_global_rest(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	if (rest_dirty) {
		_force_update_all_bone_transforms();
	}
	return bones[p_bone].global_rest;
}

bool Skeleton3D::is_bone_enabled(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, false);
	return bones[p_bone].enabled;
}

bool Skeleton3D::is_show_rest_only() const { return show_rest_only; }

void Skeleton3D::clear_bones()
{
	bones.clear();
	name_to_bone_index.clear();

	// All these structures contain references to now invalid bone indices.
	skin_bindings.clear();
	bone_global_pose_dirty.clear();
	parentless_bones.clear();
	nested_set_offset_to_bone_index.clear();

	process_order_dirty = true;
	version++;
	_make_dirty();
}

// Posing api

void Skeleton3D::set_bone_pose(int p_bone, const Transform3D& p_pose)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_position = p_pose.origin;
	bones[p_bone].pose_rotation = p_pose.basis.get_rotation_quaternion();
	bones[p_bone].pose_scale = p_pose.basis.get_scale();
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_position(int p_bone, const Vector3& p_position)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_position = p_position;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_rotation(int p_bone, const Quaternion& p_rotation)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_rotation = p_rotation;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

void Skeleton3D::set_bone_pose_scale(int p_bone, const Vector3& p_scale)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	if (modifier_updating) {
		bones[p_bone].make_bone_modified();
	}
	bones[p_bone].pose_scale = p_scale;
	bones[p_bone].pose_cache_dirty = true;
	if (is_inside_tree()) {
		_make_dirty();
		_make_bone_global_pose_subtree_dirty(p_bone);
	}
}

Vector3 Skeleton3D::get_bone_pose_position(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector3());
	return bones[p_bone].pose_position;
}

Quaternion Skeleton3D::get_bone_pose_rotation(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Quaternion());
	return bones[p_bone].pose_rotation;
}

Vector3 Skeleton3D::get_bone_pose_scale(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Vector3());
	return bones[p_bone].pose_scale;
}

void Skeleton3D::reset_bone_pose(int p_bone)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	set_bone_pose_position(p_bone, bones[p_bone].rest.origin);
	set_bone_pose_rotation(p_bone, bones[p_bone].rest.basis.get_rotation_quaternion());
	set_bone_pose_scale(p_bone, bones[p_bone].rest.basis.get_scale());
}

void Skeleton3D::reset_bone_poses()
{
	for (uint32_t i = 0; i < bones.size(); i++) {
		reset_bone_pose(i);
	}
}

Transform3D Skeleton3D::get_bone_pose(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	bones[p_bone].update_pose_cache();
	return bones[p_bone].pose_cache;
}

void Skeleton3D::_update_deferred(UpdateFlag p_update_flag)
{
	if (is_inside_tree()) {
#ifdef TOOLS_ENABLED
		if (saving) {
			update_flags |= p_update_flag;
			_notification(NOTIFICATION_UPDATE_SKELETON);
			return;
		}
#endif // TOOLS_ENABLED
		if (update_flags == UPDATE_FLAG_NONE && !updating) {
			notify_deferred_thread_group(
				NOTIFICATION_UPDATE_SKELETON); // It must never be called more than once in a single
											   // frame.
		}
		update_flags |= p_update_flag;
	}
}

void Skeleton3D::localize_rests()
{
	Vector<int> bones_to_process = get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		bones_to_process.erase(current_bone_idx);

		if (bones[current_bone_idx].parent >= 0) {
			set_bone_rest(
				current_bone_idx, bones[bones[current_bone_idx].parent].rest.affine_inverse() *
									  bones[current_bone_idx].rest);
		}

		// Add the bone's children to the list of bones to be processed.
		int child_bone_size = bones[current_bone_idx].child_bones.size();
		for (int i = 0; i < child_bone_size; i++) {
			bones_to_process.push_back(bones[current_bone_idx].child_bones[i]);
		}
	}
}

void Skeleton3D::_skin_changed() { _make_dirty(); }

Ref<Skin> Skeleton3D::create_skin_from_rest_transforms()
{
	Ref<Skin> skin;

	skin.instantiate();
	skin->set_bind_count(bones.size());

	// Pose changed, rebuild cache of inverses.
	const Bone* bonesptr = bones.ptr();
	uint32_t len = bones.size();

	// Calculate global rests and invert them.
	LocalVector<int> bones_to_process;
	bones_to_process = get_parentless_bones();
	while (bones_to_process.size() > 0) {
		int current_bone_idx = bones_to_process[0];
		const Bone& b = bonesptr[current_bone_idx];
		bones_to_process.erase(current_bone_idx);
		LocalVector<int> child_bones_vector;
		child_bones_vector = get_bone_children(current_bone_idx);
		int child_bones_size = child_bones_vector.size();
		if (b.parent < 0) {
			skin->set_bind_pose(current_bone_idx, b.rest);
		}
		for (int i = 0; i < child_bones_size; i++) {
			int child_bone_idx = child_bones_vector[i];
			const Bone& cb = bonesptr[child_bone_idx];
			skin->set_bind_pose(child_bone_idx, skin->get_bind_pose(current_bone_idx) * cb.rest);
			// Add the bone's children to the list of bones to be processed.
			bones_to_process.push_back(child_bones_vector[i]);
		}
	}

	for (uint32_t i = 0; i < len; i++) {
		// The inverse is what is actually required.
		skin->set_bind_bone(i, i);
		skin->set_bind_pose(i, skin->get_bind_pose(i).affine_inverse());
	}

	return skin;
}

void Skeleton3D::force_update_deferred() { _make_dirty(); }

void Skeleton3D::force_update_all_dirty_bones() { _force_update_all_dirty_bones(); }

void Skeleton3D::_force_update_all_dirty_bones() const
{
	if (!dirty) {
		return;
	}
	_force_update_all_bone_transforms();
}

void Skeleton3D::force_update_all_bone_transforms() { _force_update_all_bone_transforms(); }

void Skeleton3D::force_update_bone_children_transforms(int p_bone_idx)
{
	_force_update_bone_children_transforms(p_bone_idx);
}

void Skeleton3D::_force_update_bone_children_transforms(int p_bone_idx) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone_idx, bone_size);

	_update_process_order();

	Bone* bonesptr = bones.ptr();

	// Loop through nested set.
	for (int offset = 0; offset < bone_size; offset++) {
		if (rest_dirty) {
			int current_bone_idx = nested_set_offset_to_bone_index[offset];
			Bone& b = bonesptr[current_bone_idx];
			b.global_rest = b.parent >= 0 ? bonesptr[b.parent].global_rest * b.rest
										  : b.rest; // Rest needs update apert from pose.
		}

		if (!bone_global_pose_dirty[offset]) {
			continue;
		}

		int current_bone_idx = nested_set_offset_to_bone_index[offset];
		Bone& b = bonesptr[current_bone_idx];
		bool bone_enabled = b.enabled && !show_rest_only;

		if (bone_enabled) {
			b.update_pose_cache();
			Transform3D pose = b.pose_cache;

			if (b.parent >= 0) {
				b.global_pose = bonesptr[b.parent].global_pose * pose;
			}
			else {
				b.global_pose = pose;
			}
		}
		else {
			if (b.parent >= 0) {
				b.global_pose = bonesptr[b.parent].global_pose * b.rest;
			}
			else {
				b.global_pose = b.rest;
			}
		}

#ifndef DISABLE_DEPRECATED
		if (bone_enabled) {
			Transform3D pose = b.pose_cache;
			if (b.parent >= 0) {
				b.pose_global_no_override = bonesptr[b.parent].pose_global_no_override * pose;
			}
			else {
				b.pose_global_no_override = pose;
			}
		}
		else {
			if (b.parent >= 0) {
				b.pose_global_no_override = bonesptr[b.parent].pose_global_no_override * b.rest;
			}
			else {
				b.pose_global_no_override = b.rest;
			}
		}
		if (b.global_pose_override_amount >= CMP_EPSILON) {
			b.global_pose = b.global_pose.interpolate_with(
				b.global_pose_override, b.global_pose_override_amount);
		}
		if (b.global_pose_override_reset) {
			b.global_pose_override_amount = 0.0;
		}
#endif // _DISABLE_DEPRECATED

		bone_global_pose_dirty[offset] = false;
	}
}

#ifndef DISABLE_DEPRECATED
void Skeleton3D::clear_bones_global_pose_override()
{
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		bones[i].global_pose_override_amount = 0;
		bones[i].global_pose_override_reset = true;
	}
	_make_dirty();
	_make_bone_global_poses_dirty();
}

void Skeleton3D::set_bone_global_pose_override(
	int p_bone, const Transform3D& p_pose, real_t p_amount, bool p_persistent)
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose_override_amount = p_amount;
	bones[p_bone].global_pose_override = p_pose;
	bones[p_bone].global_pose_override_reset = !p_persistent;
	_make_dirty();
	_make_bone_global_pose_subtree_dirty(p_bone);
}

Transform3D Skeleton3D::get_bone_global_pose_override(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose_override;
}

Transform3D Skeleton3D::get_bone_global_pose_no_override(int p_bone) const
{
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	_force_update_all_dirty_bones();
	return bones[p_bone].pose_global_no_override;
}

#ifndef PHYSICS_3D_DISABLED
Node* Skeleton3D::get_simulator() { return simulator; }

bool Skeleton3D::get_animate_physical_bones() const { return animate_physical_bones; }

#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

Skeleton3D::Skeleton3D() {}

Skeleton3D::~Skeleton3D()
{
	// Some skins may remain bound.
	for (SkinReference* E : skin_bindings) {
		E->skeleton_node = nullptr;
	}
}

void Skeleton3D::_update_process_order() const {}

void Skeleton3D::_force_update_all_bone_transforms() const {}

void Skeleton3D::_notification(int) {}

int Skeleton3D::add_bone(String const&) { return 0; }

Ref<SkinReference> Skeleton3D::register_skin(Ref<Skin> const&)
{
	Ref<SkinReference> sr = memnew(SkinReference);
	return sr;
}

void Skeleton3D::add_child_notify(Node* p_child) {}

void Skeleton3D::remove_child_notify(Node* p_child) {}

void Skeleton3D::move_child_notify(Node* p_child) {}

// ==========================================================================
// Merged from scene/3d/skeleton_modifier_3d.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "scene/3d/skeleton_3d.h"

bool SkeletonModifier3D::should_check_node_path()
{
	return (bool)GLOBAL_GET_CACHED(
			   bool, "animation/warnings/check_invalid_skeleton_modifier_node_paths") &&
		   is_inside_tree();
}

bool SkeletonModifier3D::is_active() const { return active; }

void SkeletonModifier3D::_set_active(bool p_active) {}

void SkeletonModifier3D::set_influence(real_t p_influence) { influence = p_influence; }

PackedStringArray SkeletonModifier3D::get_configuration_warnings() const
{
	return PackedStringArray();
}

Skeleton3D* SkeletonModifier3D::get_skeleton() const
{
	Skeleton3D s = Skeleton3D();
	return &s;
}

real_t SkeletonModifier3D::get_influence() const { return influence; }

Vector3 SkeletonModifier3D::get_vector_from_bone_axis(BoneAxis p_axis)
{
	Vector3 ret;
	switch (p_axis) {
	case BONE_AXIS_PLUS_X: {
		ret = Vector3(1, 0, 0);
	} break;
	case BONE_AXIS_MINUS_X: {
		ret = Vector3(-1, 0, 0);
	} break;
	case BONE_AXIS_PLUS_Y: {
		ret = Vector3(0, 1, 0);
	} break;
	case BONE_AXIS_MINUS_Y: {
		ret = Vector3(0, -1, 0);
	} break;
	case BONE_AXIS_PLUS_Z: {
		ret = Vector3(0, 0, 1);
	} break;
	case BONE_AXIS_MINUS_Z: {
		ret = Vector3(0, 0, -1);
	} break;
	}
	return ret;
}

Vector3 SkeletonModifier3D::get_vector_from_axis(Vector3::Axis p_axis)
{
	Vector3 ret;
	switch (p_axis) {
	case Vector3::AXIS_X: {
		ret = Vector3(1, 0, 0);
	} break;
	case Vector3::AXIS_Y: {
		ret = Vector3(0, 1, 0);
	} break;
	case Vector3::AXIS_Z: {
		ret = Vector3(0, 0, 1);
	} break;
	}
	return ret;
}

Vector3::Axis SkeletonModifier3D::get_axis_from_bone_axis(BoneAxis p_axis)
{
	Vector3::Axis ret = Vector3::AXIS_X;
	switch (p_axis) {
	case BONE_AXIS_PLUS_X:
	case BONE_AXIS_MINUS_X: {
		ret = Vector3::AXIS_X;
	} break;
	case BONE_AXIS_PLUS_Y:
	case BONE_AXIS_MINUS_Y: {
		ret = Vector3::AXIS_Y;
	} break;
	case BONE_AXIS_PLUS_Z:
	case BONE_AXIS_MINUS_Z: {
		ret = Vector3::AXIS_Z;
	} break;
	}
	return ret;
}

Vector3 SkeletonModifier3D::limit_length(
	const Vector3& p_origin, const Vector3& p_destination, float p_length)
{
	return p_origin + (p_destination - p_origin).normalized() * p_length;
}

Quaternion SkeletonModifier3D::get_local_pose_rotation(
	Skeleton3D* p_skeleton, int p_bone, const Quaternion& p_global_pose_rotation)
{
	int parent = p_skeleton->get_bone_parent(p_bone);
	if (parent < 0) {
		return p_global_pose_rotation;
	}
	return (p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion().inverse() *
			p_global_pose_rotation)
		.normalized();
}

Quaternion SkeletonModifier3D::get_from_to_rotation(
	const Vector3& p_from, const Vector3& p_to, const Quaternion& p_prev_rot)
{
	if (Math::is_equal_approx((float)p_from.dot(p_to), -1.0f)) {
		return p_prev_rot; // For preventing to glitch, checking dot for detecting flip is more
						   // accurate than checking cross.
	}
	Vector3 axis = p_from.cross(p_to);
	if (axis.is_zero_approx()) {
		return p_prev_rot;
	}
	float angle = p_from.angle_to(p_to);
	if (Math::is_zero_approx(angle)) {
		angle = 0.0;
	}
	return Quaternion(axis.normalized(), angle);
}

Quaternion SkeletonModifier3D::get_from_to_rotation_by_axis(
	const Vector3& p_from, const Vector3& p_to, const Vector3& p_axis)
{
	const double ALMOST_ONE = 1.0 - CMP_EPSILON;
	double dot = p_from.dot(p_to);
	if (dot > ALMOST_ONE) {
		return Quaternion();
	}
	if (dot < -ALMOST_ONE) {
		return Quaternion(p_axis, Math::PI);
	}
	double angle = p_from.angle_to(p_to);
	Vector3 cross = p_from.cross(p_to);
	if (std::signbit(cross.dot(p_axis))) {
		angle = -angle;
	}
	return Quaternion(p_axis, angle);
}

Quaternion SkeletonModifier3D::get_swing(const Quaternion& p_rotation, const Vector3& p_axis)
{
	if (p_axis.is_zero_approx()) {
		return p_rotation;
	}
	Quaternion rot = p_rotation;
	if (!rot.is_normalized()) {
		rot.normalize();
	}
	Vector3 axis = p_axis.normalized();
	const Vector3 v(rot.x, rot.y, rot.z);
	const real_t proj_len = v.dot(axis);
	const Vector3 twist_vec = axis * proj_len;
	Quaternion twist(twist_vec.x, twist_vec.y, twist_vec.z, rot.w);
	if (!twist.is_normalized()) {
		if (Math::is_zero_approx(twist.length_squared())) {
			return rot;
		}
		twist.normalize();
	}
	Quaternion swing = rot * twist.inverse();
	swing.normalize();
	return swing;
}

Vector3 SkeletonModifier3D::snap_vector_to_plane(
	const Vector3& p_plane_normal, const Vector3& p_vector)
{
	if (Math::is_zero_approx(p_plane_normal.length_squared())) {
		return p_vector;
	}
	double length = p_vector.length();
	Vector3 normalized_vec = p_vector.normalized();
	Vector3 normal = p_plane_normal.normalized();
	return normalized_vec.slide(normal) * length;
}

double SkeletonModifier3D::symmetrize_angle(double p_angle)
{
	double angle = Math::fposmod(p_angle, Math::TAU);
	return angle > Math::PI ? angle - Math::TAU : angle;
}

double SkeletonModifier3D::get_roll_angle(const Quaternion& p_rotation, const Vector3& p_roll_axis)
{
	// Ensure roll axis is normalized.
	Vector3 roll_axis = p_roll_axis.normalized();

	// Project the quaternion rotation onto the roll axis.
	// This gives us the component of rotation around that axis.
	double dot =
		p_rotation.x * roll_axis.x + p_rotation.y * roll_axis.y + p_rotation.z * roll_axis.z;

	// Create a quaternion representing just the roll component.
	Quaternion roll_component;
	roll_component.x = roll_axis.x * dot;
	roll_component.y = roll_axis.y * dot;
	roll_component.z = roll_axis.z * dot;
	roll_component.w = p_rotation.w;

	// Normalize this component.
	double length = roll_component.length();
	if (length > CMP_EPSILON) {
		roll_component = roll_component / length;
	}
	else {
		return 0.0;
	}

	// Extract the angle.
	double angle = 2.0 * Math::acos(CLAMP(roll_component.w, -1.0, 1.0));

	// Determine the sign.
	double direction = (roll_component.x * roll_axis.x + roll_component.y * roll_axis.y +
							   roll_component.z * roll_axis.z >
						   0)
						   ? 1.0
						   : -1.0;

	return symmetrize_angle(angle * direction);
}

Vector3 SkeletonModifier3D::get_projected_normal(
	const Vector3& p_a, const Vector3& p_b, const Vector3& p_point)
{
	// Get nearest normal vector to p_point from the infinite line p_a to p_b.
	const Vector3 dir = p_b - p_a;
	const real_t denom = dir.length_squared();
	if (Math::is_zero_approx(denom)) {
		return Vector3();
	}
	const Vector3 w = p_point - p_a;
	const real_t t = w.dot(dir) / denom;
	const Vector3 h = p_a + dir * t;
	return (p_point - h).normalized();
}

SkeletonModifier3D::SkeletonModifier3D() {}

void SkeletonModifier3D::_skeleton_changed(Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}

void SkeletonModifier3D::_validate_bone_names() {}

void SkeletonModifier3D::_process_modification(double p_delta) {}



void SkeletonModifier3D::process_modification(double) {}

// ==========================================================================
// Merged from scene/3d/bone_attachment_3d.cpp
// ==========================================================================

// --- bone_attachment_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "scene/3d/skeleton_3d.h"

void BoneAttachment3D::_on_bone_pose_update_bind_compat_90575(int p_bone_index)
{
	return on_skeleton_update();
}

void BoneAttachment3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/3d/skeleton_3d.h"
#include "core/config/engine.h"

void BoneAttachment3D::_transform_changed()
{
	if (!is_inside_tree()) {
		return;
	}

	if (override_pose && !overriding) {
		Skeleton3D* sk = get_skeleton();

		ERR_FAIL_NULL_MSG(sk, "Cannot override pose: Skeleton not found!");
		ERR_FAIL_INDEX_MSG(
			bone_idx, sk->get_bone_count(), "Cannot override pose: Bone index is out of range!");

		Transform3D our_trans = get_transform();
		if (use_external_skeleton) {
			if (!sk->is_inside_tree()) {
				return;
			}
			our_trans = sk->get_global_transform().affine_inverse() * get_global_transform();
		}

		overriding = true;
		sk->set_bone_global_pose(bone_idx, our_trans);
		sk->force_update_all_dirty_bones();
	}
	overriding = false;
}

void BoneAttachment3D::set_bone_name(const String& p_name)
{
	bone_name = p_name;
	Skeleton3D* sk = get_skeleton();
	if (sk) {
		set_bone_idx(sk->find_bone(bone_name));
	}
}

String BoneAttachment3D::get_bone_name() const { return bone_name; }

int BoneAttachment3D::get_bone_idx() const { return bone_idx; }

bool BoneAttachment3D::get_override_pose() const { return override_pose; }

bool BoneAttachment3D::get_use_external_skeleton() const { return use_external_skeleton; }

NodePath BoneAttachment3D::get_external_skeleton() const { return external_skeleton_node; }

void BoneAttachment3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (use_external_skeleton) {
			_update_external_skeleton_cache();
		}
		_check_bind();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_check_unbind();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_transform_changed();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (_override_dirty) {
			_override_dirty = false;
		}
	} break;
	}
}

void BoneAttachment3D::on_skeleton_update()
{
	if (updating) {
		return;
	}
	updating = true;
	if (bone_idx >= 0) {
		Skeleton3D* sk = get_skeleton();
		if (sk) {
			if (!override_pose) {
				if (use_external_skeleton) {
					if (sk->is_inside_tree()) {
						set_global_transform(
							sk->get_global_transform() * sk->get_bone_global_pose(bone_idx));
						// Else, do nothing, the transform will be set when the skeleton enters the
						// tree: Skeleton3D::_notification(NOTIFICATION_ENTER_TREE) -> calls
						// Skeleton3D::_notification(NOTIFICATION_UPDATE_SKELETON)
						// -> emits skeleton_updated signal -> connected to
						// BoneAttachment3D::on_skeleton_update()
					}
				}
				else {
					set_transform(sk->get_bone_global_pose(bone_idx));
				}
			}
			else {
				if (!_override_dirty) {
					_transform_changed();
					_override_dirty = true;
				}
			}
		}
	}
	updating = false;
}

#ifdef TOOLS_ENABLED

void BoneAttachment3D::notify_rebind_required()
{
	// Ensures bindings are properly updated after a scene reload.
	_check_unbind();
	if (use_external_skeleton) {
		_update_external_skeleton_cache();
	}
	bone_idx = -1;
	_check_bind();
}
#endif // TOOLS_ENABLED

Skeleton3D* BoneAttachment3D::get_skeleton() {}

void BoneAttachment3D::_check_bind() {}

void BoneAttachment3D::_update_external_skeleton_cache() {}

void BoneAttachment3D::_check_unbind() {}

void BoneAttachment3D::set_bone_idx(int const&) {}

PackedStringArray BoneAttachment3D::get_configuration_warnings() const
{
	return PackedStringArray();
}

// ==========================================================================
// Merged from scene/3d/look_at_modifier_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
// --- Inlined from scene/3d/look_at_modifier_3d.h ---
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_mixer.h"

class LookAtModifier3D : public SkeletonModifier3D
{
public:
	enum OriginFrom
	{
		ORIGIN_FROM_SELF,
		ORIGIN_FROM_SPECIFIC_BONE,
		ORIGIN_FROM_EXTERNAL_NODE,
	};

private:
	String bone_name;
	int bone = -1;

	Vector3 forward_vector;
	Vector3 forward_vector_nrm;
	BoneAxis forward_axis = BONE_AXIS_PLUS_Z;
	Vector3::Axis primary_rotation_axis = Vector3::AXIS_Y;
	Vector3::Axis secondary_rotation_axis = Vector3::AXIS_X;
	bool use_secondary_rotation = true;
	bool relative = false;

	OriginFrom origin_from = ORIGIN_FROM_SELF;
	String origin_bone_name;
	int origin_bone = -1;
	NodePath origin_external_node;

	Vector3 origin_offset;
	float origin_safe_margin = 0.1;

	NodePath target_node;

	float duration = 0;
	Tween::TransitionType transition_type = Tween::TRANS_LINEAR;
	Tween::EaseType ease_type = Tween::EASE_IN;

	bool use_angle_limitation = false;
	bool symmetry_limitation = true;

	float primary_limit_angle = Math::TAU;
	float primary_damp_threshold = 1.0f;
	float primary_positive_limit_angle = Math::PI;
	float primary_positive_damp_threshold = 1.0f;
	float primary_negative_limit_angle = Math::PI;
	float primary_negative_damp_threshold = 1.0f;

	float secondary_limit_angle = Math::TAU;
	float secondary_damp_threshold = 1.0f;
	float secondary_positive_limit_angle = Math::PI;
	float secondary_positive_damp_threshold = 1.0f;
	float secondary_negative_limit_angle = Math::PI;
	float secondary_negative_damp_threshold = 1.0f;

	bool is_within_limitations = false;

	// For time-based interpolation.
	Quaternion from_q;
	Quaternion prev_q;

	float remaining = 0;
	float time_step = 1.0;

	float remap_damped(float p_from, float p_to, float p_damp_threshold, float p_value) const;
	double get_bspline_y(
		const Vector2& p_from, const Vector2& p_control, const Vector2& p_to, double p_x) const;
	bool is_intersecting_axis(const Vector3& p_prev, const Vector3& p_current,
		Vector3::Axis p_flipping_axis, Vector3::Axis p_check_axis,
		bool p_check_plane = false) const;

	Transform3D look_at_with_axes(const Transform3D& p_rest);
	void init_transition();

protected:
	virtual PackedStringArray get_configuration_warnings() const override;
	virtual void _process_modification(double p_delta) override;

public:
	String get_bone_name() const;
	int get_bone() const;

	BoneAxis get_forward_axis() const;
	Vector3::Axis get_primary_rotation_axis() const;
	void set_use_secondary_rotation(bool p_enabled);
	bool is_using_secondary_rotation() const;
	void set_relative(bool p_enabled);
	bool is_relative() const;

	void set_origin_from(OriginFrom p_origin_from);
	OriginFrom get_origin_from() const;
	String get_origin_bone_name() const;
	int get_origin_bone() const;
	void set_origin_external_node(const NodePath& p_external_node);
	NodePath get_origin_external_node() const;

	void set_origin_offset(const Vector3& p_offset);
	Vector3 get_origin_offset() const;
	void set_origin_safe_margin(float p_margin);
	float get_origin_safe_margin() const;

	void set_target_node(const NodePath& p_target_node);
	NodePath get_target_node() const;

	void set_duration(float p_duration);
	float get_duration() const;
	void set_transition_type(Tween::TransitionType p_transition_type);
	Tween::TransitionType get_transition_type() const;
	void set_ease_type(Tween::EaseType p_ease_type);
	Tween::EaseType get_ease_type() const;

	void set_use_angle_limitation(bool p_enabled);
	bool is_using_angle_limitation() const;
	void set_symmetry_limitation(bool p_enabled);
	bool is_limitation_symmetry() const;

	void set_primary_limit_angle(float p_angle);
	float get_primary_limit_angle() const;
	void set_primary_damp_threshold(float p_power);
	float get_primary_damp_threshold() const;

	void set_primary_positive_limit_angle(float p_angle);
	float get_primary_positive_limit_angle() const;
	void set_primary_positive_damp_threshold(float p_power);
	float get_primary_positive_damp_threshold() const;
	void set_primary_negative_limit_angle(float p_angle);
	float get_primary_negative_limit_angle() const;
	void set_primary_negative_damp_threshold(float p_power);
	float get_primary_negative_damp_threshold() const;

	void set_secondary_limit_angle(float p_angle);
	float get_secondary_limit_angle() const;
	void set_secondary_damp_threshold(float p_power);
	float get_secondary_damp_threshold() const;

	void set_secondary_positive_limit_angle(float p_angle);
	float get_secondary_positive_limit_angle() const;
	void set_secondary_positive_damp_threshold(float p_power);
	float get_secondary_positive_damp_threshold() const;
	void set_secondary_negative_limit_angle(float p_angle);
	float get_secondary_negative_limit_angle() const;
	void set_secondary_negative_damp_threshold(float p_power);
	float get_secondary_negative_damp_threshold() const;

	float get_interpolation_remaining() const;
	bool is_interpolating() const;
	bool is_target_within_limitation() const;

	static Vector3::Axis get_secondary_rotation_axis(
		BoneAxis p_forward_axis, Vector3::Axis p_primary_rotation_axis);
	static Vector3 get_basis_vector_from_bone_axis(const Basis& p_basis, BoneAxis p_axis);
	static Vector2 get_projection_vector(const Vector3& p_vector, Vector3::Axis p_axis);
};



// --- End scene/3d/look_at_modifier_3d.h ---
#include "scene/resources/animation.h"

PackedStringArray LookAtModifier3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (get_axis_from_bone_axis(forward_axis) == primary_rotation_axis) {
		warnings.push_back(RTR("Forward axis and primary rotation axis must not be parallel."));
	}
	return warnings;
}

String LookAtModifier3D::get_bone_name() const { return bone_name; }

int LookAtModifier3D::get_bone() const { return bone; }

SkeletonModifier3D::BoneAxis LookAtModifier3D::get_forward_axis() const { return forward_axis; }

Vector3::Axis LookAtModifier3D::get_primary_rotation_axis() const { return primary_rotation_axis; }

bool LookAtModifier3D::is_using_secondary_rotation() const { return use_secondary_rotation; }

void LookAtModifier3D::set_relative(bool p_enabled) { relative = p_enabled; }

bool LookAtModifier3D::is_relative() const { return relative; }

NodePath LookAtModifier3D::get_target_node() const { return target_node; }

LookAtModifier3D::OriginFrom LookAtModifier3D::get_origin_from() const { return origin_from; }

String LookAtModifier3D::get_origin_bone_name() const { return origin_bone_name; }

int LookAtModifier3D::get_origin_bone() const { return origin_bone; }

NodePath LookAtModifier3D::get_origin_external_node() const { return origin_external_node; }

void LookAtModifier3D::set_origin_offset(const Vector3& p_offset) { origin_offset = p_offset; }

Vector3 LookAtModifier3D::get_origin_offset() const { return origin_offset; }

void LookAtModifier3D::set_origin_safe_margin(float p_margin) { origin_safe_margin = p_margin; }

float LookAtModifier3D::get_origin_safe_margin() const { return origin_safe_margin; }

void LookAtModifier3D::set_duration(float p_duration)
{
	duration = p_duration;
	if (Math::is_zero_approx(p_duration)) {
		time_step = 0;
		remaining = 0;
	}
	else {
		time_step = 1.0 / p_duration; // Cache to avoid division.
	}
}

float LookAtModifier3D::get_duration() const { return duration; }

void LookAtModifier3D::set_transition_type(Tween::TransitionType p_transition_type)
{
	transition_type = p_transition_type;
}

Tween::TransitionType LookAtModifier3D::get_transition_type() const { return transition_type; }

void LookAtModifier3D::set_ease_type(Tween::EaseType p_ease_type) { ease_type = p_ease_type; }

Tween::EaseType LookAtModifier3D::get_ease_type() const { return ease_type; }

bool LookAtModifier3D::is_using_angle_limitation() const { return use_angle_limitation; }

bool LookAtModifier3D::is_limitation_symmetry() const { return symmetry_limitation; }

void LookAtModifier3D::set_primary_limit_angle(float p_angle) { primary_limit_angle = p_angle; }

float LookAtModifier3D::get_primary_limit_angle() const { return primary_limit_angle; }

void LookAtModifier3D::set_primary_damp_threshold(float p_power)
{
	primary_damp_threshold = p_power;
}

float LookAtModifier3D::get_primary_damp_threshold() const { return primary_damp_threshold; }

void LookAtModifier3D::set_primary_positive_limit_angle(float p_angle)
{
	primary_positive_limit_angle = p_angle;
}

float LookAtModifier3D::get_primary_positive_limit_angle() const
{
	return primary_positive_limit_angle;
}

void LookAtModifier3D::set_primary_positive_damp_threshold(float p_power)
{
	primary_positive_damp_threshold = p_power;
}

float LookAtModifier3D::get_primary_positive_damp_threshold() const
{
	return primary_positive_damp_threshold;
}

void LookAtModifier3D::set_primary_negative_limit_angle(float p_angle)
{
	primary_negative_limit_angle = p_angle;
}

float LookAtModifier3D::get_primary_negative_limit_angle() const
{
	return primary_negative_limit_angle;
}

void LookAtModifier3D::set_primary_negative_damp_threshold(float p_power)
{
	primary_negative_damp_threshold = p_power;
}

float LookAtModifier3D::get_primary_negative_damp_threshold() const
{
	return primary_negative_damp_threshold;
}

void LookAtModifier3D::set_secondary_limit_angle(float p_angle) { secondary_limit_angle = p_angle; }

float LookAtModifier3D::get_secondary_limit_angle() const { return secondary_limit_angle; }

void LookAtModifier3D::set_secondary_damp_threshold(float p_power)
{
	secondary_damp_threshold = p_power;
}

float LookAtModifier3D::get_secondary_damp_threshold() const { return secondary_damp_threshold; }

void LookAtModifier3D::set_secondary_positive_limit_angle(float p_angle)
{
	secondary_positive_limit_angle = p_angle;
}

float LookAtModifier3D::get_secondary_positive_limit_angle() const
{
	return secondary_positive_limit_angle;
}

void LookAtModifier3D::set_secondary_positive_damp_threshold(float p_power)
{
	secondary_positive_damp_threshold = p_power;
}

float LookAtModifier3D::get_secondary_positive_damp_threshold() const
{
	return secondary_positive_damp_threshold;
}

void LookAtModifier3D::set_secondary_negative_limit_angle(float p_angle)
{
	secondary_negative_limit_angle = p_angle;
}

float LookAtModifier3D::get_secondary_negative_limit_angle() const
{
	return secondary_negative_limit_angle;
}

void LookAtModifier3D::set_secondary_negative_damp_threshold(float p_power)
{
	secondary_negative_damp_threshold = p_power;
}

float LookAtModifier3D::get_secondary_negative_damp_threshold() const
{
	return secondary_negative_damp_threshold;
}

bool LookAtModifier3D::is_target_within_limitation() const { return is_within_limitations; }

float LookAtModifier3D::get_interpolation_remaining() const { return remaining * duration; }

bool LookAtModifier3D::is_interpolating() const { return Math::is_zero_approx(remaining); }

bool LookAtModifier3D::is_intersecting_axis(const Vector3& p_prev, const Vector3& p_current,
	Vector3::Axis p_flipping_axis, Vector3::Axis p_check_axis, bool p_check_plane) const
{
	// Prevent that the angular velocity does not become too large.
	// Check that is p_flipping_axis flipped nearby p_check_axis (close than origin_safe_margin) or
	// not. If p_check_plane is true, check two axes of crossed plane.
	if (p_check_plane) {
		if (get_projection_vector(p_prev, p_check_axis).length() > origin_safe_margin &&
			get_projection_vector(p_current, p_check_axis).length() > origin_safe_margin) {
			return false;
		}
	}
	else if (Math::abs(p_prev[p_check_axis]) > origin_safe_margin &&
			   Math::abs(p_current[p_check_axis]) > origin_safe_margin) {
		return false;
	}

	return std::signbit(p_prev[p_flipping_axis]) != std::signbit(p_current[p_flipping_axis]);
}

Vector3 LookAtModifier3D::get_basis_vector_from_bone_axis(const Basis& p_basis, BoneAxis p_axis)
{
	Vector3 ret;
	switch (p_axis) {
	case BONE_AXIS_PLUS_X: {
		ret = p_basis.get_column(0);
	} break;
	case BONE_AXIS_MINUS_X: {
		ret = -p_basis.get_column(0);
	} break;
	case BONE_AXIS_PLUS_Y: {
		ret = p_basis.get_column(1);
	} break;
	case BONE_AXIS_MINUS_Y: {
		ret = -p_basis.get_column(1);
	} break;
	case BONE_AXIS_PLUS_Z: {
		ret = p_basis.get_column(2);
	} break;
	case BONE_AXIS_MINUS_Z: {
		ret = -p_basis.get_column(2);
	} break;
	}
	return ret;
}

Vector3::Axis LookAtModifier3D::get_secondary_rotation_axis(
	BoneAxis p_forward_axis, Vector3::Axis p_primary_rotation_axis)
{
	Vector3 secondary_plane =
		get_vector_from_bone_axis(p_forward_axis) + get_vector_from_axis(p_primary_rotation_axis);
	return Math::is_zero_approx(secondary_plane.x)
			   ? Vector3::AXIS_X
			   : (Math::is_zero_approx(secondary_plane.y) ? Vector3::AXIS_Y : Vector3::AXIS_Z);
}

Vector2 LookAtModifier3D::get_projection_vector(const Vector3& p_vector, Vector3::Axis p_axis)
{
	// NOTE: axis is swapped between 2D and 3D.
	Vector2 ret;
	switch (p_axis) {
	case Vector3::AXIS_X: {
		ret = Vector2(p_vector.z, p_vector.y);
	} break;
	case Vector3::AXIS_Y: {
		ret = Vector2(p_vector.x, p_vector.z);
	} break;
	case Vector3::AXIS_Z: {
		ret = Vector2(p_vector.y, p_vector.x);
	} break;
	}
	return ret;
}

float LookAtModifier3D::remap_damped(
	float p_from, float p_to, float p_damp_threshold, float p_value) const
{
	float sign = std::signbit(p_value) ? -1.0f : 1.0f;
	float abs_value = Math::abs(p_value);

	if (Math::is_equal_approx(p_damp_threshold, 1.0f) || Math::is_zero_approx(p_to)) {
		return sign * CLAMP(abs_value, p_from, p_to); // Avoid division by zero.
	}

	float value = Math::inverse_lerp(p_from, p_to, abs_value);

	if (value <= p_damp_threshold) {
		return sign * CLAMP(abs_value, p_from, p_to);
	}

	double limit = Math::PI;
	double inv_to = 1.0 / p_to;
	double end_x = limit * inv_to;
	double position = abs_value * inv_to;
	Vector2 start = Vector2(p_damp_threshold, p_damp_threshold);
	Vector2 mid = Vector2(1.0, 1.0);
	Vector2 end = Vector2(end_x, 1.0);
	value = get_bspline_y(start, mid, end, position);

	return sign * Math::lerp(p_from, p_to, value);
}

double LookAtModifier3D::get_bspline_y(
	const Vector2& p_from, const Vector2& p_control, const Vector2& p_to, double p_x) const
{
	double a = p_from.x - 2.0 * p_control.x + p_to.x;
	double b = -2.0 * p_from.x + 2.0 * p_control.x;
	double c = p_from.x - p_x;
	double t = 0.0;
	if (Math::is_zero_approx(a)) {
		t = -c / b; // Almost linear.
	}
	else {
		double discriminant = b * b - 4.0 * a * c;
		double sqrt_discriminant = Math::sqrt(discriminant);
		double e = 1.0 / (2.0 * a);
		double t1 = (-b + sqrt_discriminant) * e;
		t = (0.0 <= t1 && t1 <= 1.0) ? t1 : (-b - sqrt_discriminant) * e;
	}
	double u = 1.0 - t;
	double y = u * u * p_from.y + 2.0 * u * t * p_control.y + t * t * p_to.y;
	return y;
}

Transform3D LookAtModifier3D::look_at_with_axes(const Transform3D& p_rest)
{
	// Primary rotation by projection to 2D plane by xform_inv and picking elements.
	Vector3 current_vector =
		get_basis_vector_from_bone_axis(p_rest.basis, forward_axis).normalized();
	Vector2 src_vec2 =
		get_projection_vector(p_rest.basis.xform_inv(forward_vector_nrm), primary_rotation_axis)
			.normalized();
	Vector2 dst_vec2 =
		get_projection_vector(p_rest.basis.xform_inv(current_vector), primary_rotation_axis)
			.normalized();
	real_t calculated_angle = src_vec2.angle_to(dst_vec2);
	Transform3D primary_result
=
		p_rest.rotated_local(get_vector_from_axis(primary_rotation_axis), calculated_angle);
	Transform3D current_result =
		primary_result; // primary_result will be used by calculation of secondary rotation,
						// current_result is rotated by that.
	float limit_angle = 0.0;
	float damp_threshold = 0.0;

	if (use_angle_limitation) {
		if (symmetry_limitation) {
			limit_angle = primary_limit_angle * 0.5f;
			damp_threshold = primary_damp_threshold;
		}
		else {
			if (std::signbit(calculated_angle)) {
				limit_angle = primary_negative_limit_angle;
				damp_threshold = primary_negative_damp_threshold;
			}
			else {
				limit_angle = primary_positive_limit_angle;
				damp_threshold = primary_positive_damp_threshold;
			}
		}
		if (Math::abs(calculated_angle) > limit_angle) {
			is_within_limitations = false;
		}
		calculated_angle = remap_damped(0, limit_angle, damp_threshold, calculated_angle);
		current_result =
			p_rest.rotated_local(get_vector_from_axis(primary_rotation_axis), calculated_angle);
	}

	// Needs for detecting flipping even if use_secondary_rotation is false.
	secondary_rotation_axis = get_secondary_rotation_axis(forward_axis, primary_rotation_axis);

	if (!use_secondary_rotation) {
		return current_result;
	}

	// Secondary rotation by projection to 2D plane by xform_inv and picking elements.
	current_vector =
		get_basis_vector_from_bone_axis(primary_result.basis, forward_axis).normalized();
	src_vec2 = get_projection_vector(
		primary_result.basis.xform_inv(forward_vector_nrm), secondary_rotation_axis)
				   .normalized();
	dst_vec2 = get_projection_vector(
		primary_result.basis.xform_inv(current_vector), secondary_rotation_axis)
				   .normalized();
	calculated_angle = src_vec2.angle_to(dst_vec2);

	if (use_angle_limitation) {
		if (symmetry_limitation) {
			limit_angle = secondary_limit_angle * 0.5f;
			damp_threshold = secondary_damp_threshold;
		}
		else {
			if (std::signbit(calculated_angle)) {
				limit_angle = secondary_negative_limit_angle;
				damp_threshold = secondary_negative_damp_threshold;
			}
			else {
				limit_angle = secondary_positive_limit_angle;
				damp_threshold = secondary_positive_damp_threshold;
			}
		}
		if (Math::abs(calculated_angle) > limit_angle) {
			is_within_limitations = false;
		}
		calculated_angle = remap_damped(0, limit_angle, damp_threshold, calculated_angle);
	}
	current_result = current_result.rotated_local(
		get_vector_from_axis(secondary_rotation_axis), calculated_angle);

	return current_result;
}

void LookAtModifier3D::init_transition()
{
	if (Math::is_zero_approx(duration)) {
		return;
	}
	from_q = prev_q;
	remaining = 1.0;
}

void LookAtModifier3D::_process_modification(double p_delta) {}

// ==========================================================================
// Merged from scene/3d/retarget_modifier_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/retarget_modifier_3d.h ---
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/animation.h"

class RetargetModifier3D : public SkeletonModifier3D
{
public:
	enum TransformFlag
	{
		TRANSFORM_FLAG_POSITION = 1,
		TRANSFORM_FLAG_ROTATION = 2,
		TRANSFORM_FLAG_SCALE = 4,
		TRANSFORM_FLAG_ALL =
			TRANSFORM_FLAG_POSITION | TRANSFORM_FLAG_ROTATION | TRANSFORM_FLAG_SCALE,
	};

private:
	Ref<SkeletonProfile> profile;

	bool use_global_pose = false;
	uint32_t enable_flags = TRANSFORM_FLAG_ALL;

	struct RetargetBoneInfo
	{
		int bone_id = -1;
		Basis pre_basis;
		Basis post_basis;
	};

	struct RetargetInfo
	{
		Vector<RetargetBoneInfo> humanoid_bone_rests;
	};

	Vector<RetargetInfo> child_skeletons;
	Vector<int> source_bone_ids;

	void _update_child_skeleton_rests(int p_child_skeleton_idx);
	void _update_child_skeletons();
	void _reset_child_skeleton_poses();
	void _reset_child_skeletons();

#ifdef TOOLS_ENABLED
	void _force_update_child_skeletons();
#endif // TOOLS_ENABLED

	Vector<RetargetBoneInfo> get_humanoid_bone_rests(Skeleton3D* p_skeleton);

	void _retarget_global_pose();
	void _retarget_pose();

protected:
	virtual void _notification(int p_what);
	virtual void _skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new) override;

	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

	virtual void _set_active(bool p_active) override;
	virtual void _process_modification(double p_delta) override;

	void _profile_changed(Ref<SkeletonProfile> p_old, Ref<SkeletonProfile> p_new);

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	void set_use_global_pose(bool p_use_global_pose);
	bool is_using_global_pose() const;
	void set_enable_flags(uint32_t p_enable_flags);
	uint32_t get_enable_flags() const;

	void set_position_enabled(bool p_enabled);
	bool is_position_enabled() const;
	void set_rotation_enabled(bool p_enabled);
	bool is_rotation_enabled() const;
	void set_scale_enabled(bool p_enabled);
	bool is_scale_enabled() const;

	void set_profile(Ref<SkeletonProfile> p_profile);
	Ref<SkeletonProfile> get_profile() const;

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const override { return true; }
#endif

	RetargetModifier3D();
	virtual ~RetargetModifier3D();
};



// --- End scene/3d/retarget_modifier_3d.h ---

PackedStringArray RetargetModifier3D::get_configuration_warnings() const
{
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (child_skeletons.is_empty()) {
		warnings.push_back(RTR("There is no child Skeleton3D!"));
	}
	return warnings;
}

void RetargetModifier3D::_reset_child_skeletons()
{
	_reset_child_skeleton_poses();
	child_skeletons.clear();
}

void RetargetModifier3D::_set_active(bool p_active)
{
	if (!p_active) {
		_reset_child_skeleton_poses();
	}
}

void RetargetModifier3D::_process_modification(double p_delta)
{
	if (use_global_pose) {
		_retarget_global_pose();
	}
	else {
		_retarget_pose();
	}
}

void RetargetModifier3D::set_profile(Ref<SkeletonProfile> p_profile)
{
	if (profile == p_profile) {
		return;
	}
	_profile_changed(profile, p_profile);
}

Ref<SkeletonProfile> RetargetModifier3D::get_profile() const { return profile; }

bool RetargetModifier3D::is_using_global_pose() const { return use_global_pose; }

void RetargetModifier3D::set_enable_flags(uint32_t p_enable_flag)
{
	if (enable_flags != p_enable_flag) {
		_reset_child_skeleton_poses();
	}
	enable_flags = p_enable_flag;
}

uint32_t RetargetModifier3D::get_enable_flags() const { return enable_flags; }

void RetargetModifier3D::_reset_child_skeleton_poses() {}

void RetargetModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_update_child_skeletons();
	} break;
	case NOTIFICATION_EXIT_TREE: {
		_reset_child_skeletons();
	} break;
	}
}

RetargetModifier3D::RetargetModifier3D() {}

RetargetModifier3D::~RetargetModifier3D() {}

void RetargetModifier3D::_update_child_skeletons() {}

void RetargetModifier3D::_retarget_pose() {}

void RetargetModifier3D::_retarget_global_pose() {}

void RetargetModifier3D::_profile_changed(Ref<SkeletonProfile>, Ref<SkeletonProfile>) {}

void RetargetModifier3D::add_child_notify(Node* p_child) {}

void RetargetModifier3D::remove_child_notify(Node* p_child) {}

void RetargetModifier3D::move_child_notify(Node* p_child) {}

void RetargetModifier3D::_skeleton_changed(Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}

// ==========================================================================
// Merged from scene/3d/spring_bone_collision_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/spring_bone_collision_3d.h ---
#include "scene/3d/skeleton_3d.h"

class SpringBoneCollision3D : public Node3D {
public:
	enum ShapeType {
		SHAPE_SPHERE,
		SHAPE_CAPSULE,
		SHAPE_PLANE,
	};

private:
	ShapeType shape_type = SHAPE_SPHERE;
	float radius = 0.1f;
	float height = 0.5f;
	bool inside = false;

	String bone_name;
	int bone = -1;

	Vector3 position_offset;
	Quaternion rotation_offset;

	void _validate_bone_name();

	static Vector3 _collide_sphere(const Vector3 &p_origin, float p_radius, bool p_inside, float p_bone_radius, float p_bone_length, const Vector3 &p_current);

protected:
	PackedStringArray get_configuration_warnings() const override;
	void _notification(int p_what);

public:
	SpringBoneCollision3D(ShapeType p_shape_type = SHAPE_SPHERE);

	void set_shape_type(ShapeType p_shape_type);
	ShapeType get_shape_type() const { return shape_type; }

	void set_radius(float p_radius);
	float get_radius() const { return radius; }

	void set_height(float p_height);
	float get_height() const { return height; }
	void set_mid_height(real_t p_mid_height);
	real_t get_mid_height() const { return height - radius * 2.0f; }

	void set_inside(bool p_enabled);
	bool is_inside() const { return inside; }

	Pair<Vector3, Vector3> get_head_and_tail(const Transform3D &p_center) const;

	Skeleton3D *get_skeleton() const;

	void set_bone_name(const String &p_name);
	String get_bone_name() const;
	void set_bone(int p_bone);
	int get_bone() const;

	void set_position_offset(const Vector3 &p_offset);
	Vector3 get_position_offset() const;
	void set_rotation_offset(const Quaternion &p_offset);
	Quaternion get_rotation_offset() const;

	void sync_pose();
	Transform3D get_transform_from_skeleton(const Transform3D &p_center) const;

	Vector3 collide(const Transform3D &p_center, float p_bone_radius, float p_bone_length, const Vector3 &p_current) const;
};

// --- End scene/3d/spring_bone_collision_3d.h ---

#include "core/config/engine.h"
// --- Inlined from scene/3d/spring_bone_simulator_3d.h ---
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/curve.h"

#ifndef DISABLE_DEPRECATED
namespace compat::SpringBoneSimulator3D
{
enum BoneDirection : int;
enum RotationAxis : int;
} // namespace compat::SpringBoneSimulator3D
#endif

class SpringBoneSimulator3D : public SkeletonModifier3D
{
#ifdef TOOLS_ENABLED
	bool saving = false;
#endif // TOOLS_ENABLED

	bool joints_dirty = false;

	bool collisions_dirty = false;
	void _find_collisions();
	void _process_collisions();
	void _make_collisions_dirty();

public:
	enum CenterFrom
	{
		CENTER_FROM_WORLD_ORIGIN,
		CENTER_FROM_NODE,
		CENTER_FROM_BONE,
	};

	struct SpringBone3DVerletInfo
	{
		Vector3 prev_tail;
		Vector3 current_tail;
		Quaternion current_rot;
		Vector3 forward_vector;
		float length = 0.0;
	};

	struct SpringBone3DJointSetting
	{
		String bone_name;
		int bone = -1;

		RotationAxis rotation_axis = ROTATION_AXIS_ALL;
		Vector3 rotation_axis_vector = Vector3(1, 0, 0);

		Vector3 get_rotation_axis_vector() const
		{
			Vector3 ret;
			switch (rotation_axis) {
			case ROTATION_AXIS_X:
				ret = Vector3(1, 0, 0);
				break;
			case ROTATION_AXIS_Y:
				ret = Vector3(0, 1, 0);
				break;
			case ROTATION_AXIS_Z:
				ret = Vector3(0, 0, 1);
				break;
			case ROTATION_AXIS_ALL:
				ret = Vector3(0, 0, 0);
				break;
			case ROTATION_AXIS_CUSTOM:
				ret = rotation_axis_vector;
				break;
			}
			return ret;
		}

		float radius = 0.1;
		float stiffness = 1.0;
		float drag = 0.0;
		float gravity = 0.0;
		Vector3 gravity_direction = Vector3(0, -1, 0);

		// To process.
		SpringBone3DVerletInfo* verlet = nullptr;
	};

	struct SpringBone3DSetting
	{
		bool joints_dirty = false;

		String root_bone_name;
		int root_bone = -1;

		String end_bone_name;
		int end_bone = -1;

		// To make virtual end joint.
		bool extend_end_bone = false;
		BoneDirection end_bone_direction = BONE_DIRECTION_FROM_PARENT;
		float end_bone_length = 0.0;

		CenterFrom center_from = CENTER_FROM_WORLD_ORIGIN;
		NodePath center_node;
		String center_bone_name;
		int center_bone = -1;

		// Cache into joints.
		bool individual_config = false;
		float radius = 0.02;
		Ref<Curve> radius_damping_curve;
		float stiffness = 1.0;
		Ref<Curve> stiffness_damping_curve;
		float drag = 0.4;
		Ref<Curve> drag_damping_curve;
		float gravity = 0.0;
		Ref<Curve> gravity_damping_curve;
		Vector3 gravity_direction = Vector3(0, -1, 0);
		RotationAxis rotation_axis = ROTATION_AXIS_ALL;
		Vector3 rotation_axis_vector = Vector3(1, 0, 0);
		LocalVector<SpringBone3DJointSetting*> joints;

		// Cache into collisions.
		bool enable_all_child_collisions = true;
		LocalVector<NodePath> collisions;
		LocalVector<NodePath> exclude_collisions;

		// To process.
		bool simulation_dirty = false;
		Transform3D cached_center;
		Transform3D cached_inverted_center;
	};

protected:
	LocalVector<SpringBone3DSetting*> settings;
	Vector3 external_force;
	bool mutable_bone_axes = true;
	void _notification(int p_what);

	virtual void _validate_bone_names() override;
	virtual void _skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new) override;
	virtual void _process_modification(double p_delta) override;

	void _init_joints(Skeleton3D* p_skeleton, SpringBone3DSetting* p_setting);
	void _make_joints_dirty(int p_index, bool p_reset = false);

#ifdef TOOLS_ENABLED
	bool gizmo_dirty = false;
	void _make_gizmo_dirty();
#endif // TOOLS_ENABLED

	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

#ifndef DISABLE_DEPRECATED
	compat::SpringBoneSimulator3D::BoneDirection _get_end_bone_direction_bind_compat_110120(
		int p_index) const;
	void _set_end_bone_direction_bind_compat_110120(
		int p_index, compat::SpringBoneSimulator3D::BoneDirection p_bone_direction);
	compat::SpringBoneSimulator3D::RotationAxis _get_rotation_axis_bind_compat_110120(
		int p_index) const;
	void _set_rotation_axis_bind_compat_110120(
		int p_index, compat::SpringBoneSimulator3D::RotationAxis p_axis);
	compat::SpringBoneSimulator3D::RotationAxis _get_joint_rotation_axis_bind_compat_110120(
		int p_index, int p_joint) const;
	void _set_joint_rotation_axis_bind_compat_110120(
		int p_index, int p_joint, compat::SpringBoneSimulator3D::RotationAxis p_axis);

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	// Setting.
	String get_root_bone_name(int p_index) const;
	int get_root_bone(int p_index) const;

	String get_end_bone_name(int p_index) const;
	void set_end_bone(int p_index, int p_bone);
	int get_end_bone(int p_index) const;

	void set_extend_end_bone(int p_index, bool p_enabled);
	bool is_end_bone_extended(int p_index) const;
	void set_end_bone_direction(int p_index, BoneDirection p_bone_direction);
	BoneDirection get_end_bone_direction(int p_index) const;
	void set_end_bone_length(int p_index, float p_length);
	float get_end_bone_length(int p_index) const;

	void set_center_from(int p_index, CenterFrom p_center_from);
	CenterFrom get_center_from(int p_index) const;
	void set_center_node(int p_index, const NodePath& p_node_path);
	NodePath get_center_node(int p_index) const;
	String get_center_bone_name(int p_index) const;
	int get_center_bone(int p_index) const;

	void set_rotation_axis(int p_index, RotationAxis p_axis);
	RotationAxis get_rotation_axis(int p_index) const;
	void set_rotation_axis_vector(int p_index, const Vector3& p_vector);
	Vector3 get_rotation_axis_vector(int p_index) const;
	void set_radius(int p_index, float p_radius);
	float get_radius(int p_index) const;
	void set_radius_damping_curve(int p_index, const Ref<Curve>& p_damping_curve);
	Ref<Curve> get_radius_damping_curve(int p_index) const;
	void set_stiffness(int p_index, float p_stiffness);
	float get_stiffness(int p_index) const;
	void set_stiffness_damping_curve(int p_index, const Ref<Curve>& p_damping_curve);
	Ref<Curve> get_stiffness_damping_curve(int p_index) const;
	void set_drag(int p_index, float p_drag);
	float get_drag(int p_index) const;
	void set_drag_damping_curve(int p_index, const Ref<Curve>& p_damping_curve);
	Ref<Curve> get_drag_damping_curve(int p_index) const;
	void set_gravity(int p_index, float p_gravity);
	float get_gravity(int p_index) const;
	void set_gravity_damping_curve(int p_index, const Ref<Curve>& p_damping_curve);
	Ref<Curve> get_gravity_damping_curve(int p_index) const;
	void set_gravity_direction(int p_index, const Vector3& p_gravity_direction);
	Vector3 get_gravity_direction(int p_index) const;

	void set_setting_count(int p_count);
	int get_setting_count() const;
	void clear_settings();

	// Individual joints.
	void set_individual_config(int p_index, bool p_enabled);
	bool is_config_individual(int p_index) const;

	String get_joint_bone_name(int p_index, int p_joint) const;
	int get_joint_bone(int p_index, int p_joint) const;

	void set_joint_rotation_axis(int p_index, int p_joint, RotationAxis p_axis);
	RotationAxis get_joint_rotation_axis(int p_index, int p_joint) const;
	Vector3 get_joint_rotation_axis_vector(int p_index, int p_joint) const;
	void set_joint_radius(int p_index, int p_joint, float p_radius);
	float get_joint_radius(int p_index, int p_joint) const;
	void set_joint_stiffness(int p_index, int p_joint, float p_stiffness);
	float get_joint_stiffness(int p_index, int p_joint) const;
	void set_joint_drag(int p_index, int p_joint, float p_drag);
	float get_joint_drag(int p_index, int p_joint) const;
	void set_joint_gravity(int p_index, int p_joint, float p_gravity);
	float get_joint_gravity(int p_index, int p_joint) const;
	void set_joint_gravity_direction(int p_index, int p_joint, const Vector3& p_gravity_direction);
	Vector3 get_joint_gravity_direction(int p_index, int p_joint) const;

	void set_joint_count(int p_index, int p_count);
	int get_joint_count(int p_index) const;

	// Individual collisions.
	void set_enable_all_child_collisions(int p_index, bool p_enabled);
	bool are_all_child_collisions_enabled(int p_index) const;

	void set_exclude_collision_path(int p_index, int p_collision, const NodePath& p_node_path);
	NodePath get_exclude_collision_path(int p_index, int p_collision) const;

	void set_exclude_collision_count(int p_index, int p_count);
	int get_exclude_collision_count(int p_index) const;
	void clear_exclude_collisions(int p_index);

	void set_collision_path(int p_index, int p_collision, const NodePath& p_node_path);
	NodePath get_collision_path(int p_index, int p_collision) const;

	void set_collision_count(int p_index, int p_count);
	int get_collision_count(int p_index) const;
	void clear_collisions(int p_index);

	void set_external_force(const Vector3& p_force);
	Vector3 get_external_force() const;

	void set_mutable_bone_axes(bool p_enabled);
	bool are_bone_axes_mutable() const;

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const override { return true; }
#endif // TOOLS_ENABLED

	~SpringBoneSimulator3D();
};



// --- End scene/3d/spring_bone_simulator_3d.h ---

SpringBoneCollision3D::SpringBoneCollision3D(ShapeType p_shape_type) :
		shape_type(p_shape_type) {
}

void SpringBoneCollision3D::set_shape_type(ShapeType p_shape_type) {
	shape_type = p_shape_type;
}

void SpringBoneCollision3D::set_radius(float p_radius) {
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		height = radius * 2.0f;
	}
}

void SpringBoneCollision3D::set_height(float p_height) {
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
}

void SpringBoneCollision3D::set_mid_height(real_t p_mid_height) {
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "SpringBoneCollision3D capsule mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
}

void SpringBoneCollision3D::set_inside(bool p_enabled) {
	inside = p_enabled;
}

Pair<Vector3, Vector3> SpringBoneCollision3D::get_head_and_tail(const Transform3D &p_center) const {
	Transform3D tr = get_transform_from_skeleton(p_center);
	return Pair<Vector3, Vector3>(
			tr.origin + tr.basis.xform(Vector3::UP * (height * 0.5f - radius)),
			tr.origin + tr.basis.xform(Vector3::DOWN * (height * 0.5f - radius)));
}

void SpringBoneCollision3D::_validate_bone_name() {
	if (!bone_name.is_empty()) {
		set_bone_name(bone_name);
	} else if (bone != -1) {
		set_bone(bone);
	}
}

Skeleton3D *SpringBoneCollision3D::get_skeleton() const {
	SpringBoneSimulator3D *sim = dynamic_cast<SpringBoneSimulator3D *>(get_parent());
	if (sim) {
		return sim->get_skeleton();
	}
	return nullptr;
}

void SpringBoneCollision3D::set_bone_name(const String &p_name) {
	bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_bone(sk->find_bone(bone_name));
	}
}

String SpringBoneCollision3D::get_bone_name() const {
	return bone_name;
}

void SpringBoneCollision3D::set_bone(int p_bone) {
	bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && bone >= 0 && bone < sk->get_bone_count()) {
		bone_name = sk->get_bone_name(bone);
	}
	sync_pose();
}

int SpringBoneCollision3D::get_bone() const {
	return bone;
}

void SpringBoneCollision3D::set_position_offset(const Vector3 &p_offset) {
	if (position_offset == p_offset) {
		return;
	}
	position_offset = p_offset;
	sync_pose();
}

Vector3 SpringBoneCollision3D::get_position_offset() const {
	return position_offset;
}

void SpringBoneCollision3D::set_rotation_offset(const Quaternion &p_offset) {
	if (rotation_offset == p_offset) {
		return;
	}
	rotation_offset = p_offset;
	sync_pose();
}

Quaternion SpringBoneCollision3D::get_rotation_offset() const {
	return rotation_offset;
}

void SpringBoneCollision3D::sync_pose() {
	if (bone >= 0) {
		Skeleton3D *sk = get_skeleton();
		if (sk) {
			Transform3D tr = sk->get_global_transform() * sk->get_bone_global_pose(bone);
			tr.origin += tr.basis.get_rotation_quaternion().xform(position_offset);
			tr.basis *= Basis(rotation_offset);
			set_global_transform(tr);
		}
	}
}

Transform3D SpringBoneCollision3D::get_transform_from_skeleton(const Transform3D &p_center) const {
	Transform3D gtr = get_global_transform();
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		Transform3D tr = sk->get_global_transform();
		gtr = tr.affine_inverse() * p_center * gtr;
	}
	return gtr;
}

void SpringBoneCollision3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_PARENTED: {
			_validate_bone_name();
		} break;
	}
}

Vector3 SpringBoneCollision3D::_collide_sphere(const Vector3 &p_origin, float p_radius, bool p_inside, float p_bone_radius, float p_bone_length, const Vector3 &p_current) {
	Vector3 diff = p_current - p_origin;
	float length = diff.length();
	float r = p_inside ? p_radius - p_bone_radius : p_bone_radius + p_radius;
	float distance = p_inside ? r - length : length - r;
	if (distance > 0) {
		return p_current;
	}
	return p_origin + diff.normalized() * r;
}

Vector3 SpringBoneCollision3D::collide(const Transform3D &p_center, float p_bone_radius, float p_bone_length, const Vector3 &p_current) const {
	switch (shape_type) {
		case SHAPE_SPHERE: {
			return _collide_sphere(get_transform_from_skeleton(p_center).origin, radius, inside, p_bone_radius, p_bone_length, p_current);
		}
		case SHAPE_CAPSULE: {
			Pair<Vector3, Vector3> head_tail = get_head_and_tail(p_center);
			Vector3 head = head_tail.first;
			Vector3 tail = head_tail.second;
			Vector3 p = tail - head;
			Vector3 q = p_current - head;
			float dot = p.dot(q);
			if (dot <= 0) {
				return _collide_sphere(head, radius, inside, p_bone_radius, p_bone_length, p_current);
			}
			float pls = p.length_squared();
			if (Math::is_zero_approx(pls)) {
				return p_current;
			}
			if (pls <= dot) {
				return _collide_sphere(head + p, radius, inside, p_bone_radius, p_bone_length, p_current);
			}
			return _collide_sphere(head + p * (dot / pls), radius, inside, p_bone_radius, p_bone_length, p_current);
		}
		case SHAPE_PLANE: {
			Transform3D tr = get_transform_from_skeleton(p_center);
			Vector3 pos = tr.origin;
			Vector3 normal = tr.basis.get_rotation_quaternion().xform(Vector3::UP);
			Vector3 to_vec = p_current - pos;
			float distance = to_vec.dot(normal) - p_bone_radius;
			if (distance > 0) {
				return p_current;
			}
			return p_current + normal * -distance;
		}
	}
	return p_current;
}

PackedStringArray SpringBoneCollision3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!dynamic_cast<SpringBoneSimulator3D *>(get_parent())) {
		warnings.push_back(RTR("SpringBoneCollision3D must be a child of SpringBoneSimulator3D."));
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/3d/spring_bone_simulator_3d.cpp
// ==========================================================================

#include "core/config/engine.h"

// --- spring_bone_simulator_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED


namespace compat::SpringBoneSimulator3D
{
enum BoneDirection : int
{
	BONE_DIRECTION_PLUS_X = SkeletonModifier3D::BONE_DIRECTION_PLUS_X,
	BONE_DIRECTION_MINUS_X = SkeletonModifier3D::BONE_DIRECTION_MINUS_X,
	BONE_DIRECTION_PLUS_Y = SkeletonModifier3D::BONE_DIRECTION_PLUS_Y,
	BONE_DIRECTION_MINUS_Y = SkeletonModifier3D::BONE_DIRECTION_MINUS_Y,
	BONE_DIRECTION_PLUS_Z = SkeletonModifier3D::BONE_DIRECTION_PLUS_Z,
	BONE_DIRECTION_MINUS_Z = SkeletonModifier3D::BONE_DIRECTION_MINUS_Z,
	BONE_DIRECTION_FROM_PARENT = SkeletonModifier3D::BONE_DIRECTION_FROM_PARENT,
};

enum RotationAxis : int
{
	ROTATION_AXIS_X = SkeletonModifier3D::ROTATION_AXIS_X,
	ROTATION_AXIS_Y = SkeletonModifier3D::ROTATION_AXIS_Y,
	ROTATION_AXIS_Z = SkeletonModifier3D::ROTATION_AXIS_Z,
	ROTATION_AXIS_ALL = SkeletonModifier3D::ROTATION_AXIS_ALL,
	ROTATION_AXIS_CUSTOM = SkeletonModifier3D::ROTATION_AXIS_CUSTOM,
};
} // namespace compat::SpringBoneSimulator3D

compat::SpringBoneSimulator3D::BoneDirection
SpringBoneSimulator3D::_get_end_bone_direction_bind_compat_110120(int p_index) const
{
	return static_cast<compat::SpringBoneSimulator3D::BoneDirection>(
		(int)get_end_bone_direction(p_index));
}

void SpringBoneSimulator3D::_set_end_bone_direction_bind_compat_110120(
	int p_index, compat::SpringBoneSimulator3D::BoneDirection p_bone_direction)
{
	set_end_bone_direction(
		p_index, static_cast<SkeletonModifier3D::BoneDirection>((int)p_bone_direction));
}

compat::SpringBoneSimulator3D::RotationAxis
SpringBoneSimulator3D::_get_rotation_axis_bind_compat_110120(int p_index) const
{
	return static_cast<compat::SpringBoneSimulator3D::RotationAxis>(
		(int)get_rotation_axis(p_index));
}

void SpringBoneSimulator3D::_set_rotation_axis_bind_compat_110120(
	int p_index, compat::SpringBoneSimulator3D::RotationAxis p_axis)
{
	set_rotation_axis(p_index, static_cast<SkeletonModifier3D::RotationAxis>((int)p_axis));
}

compat::SpringBoneSimulator3D::RotationAxis
SpringBoneSimulator3D::_get_joint_rotation_axis_bind_compat_110120(int p_index, int p_joint) const
{
	return static_cast<compat::SpringBoneSimulator3D::RotationAxis>(
		(int)get_joint_rotation_axis(p_index, p_joint));
}

void SpringBoneSimulator3D::_set_joint_rotation_axis_bind_compat_110120(
	int p_index, int p_joint, compat::SpringBoneSimulator3D::RotationAxis p_axis)
{
	set_joint_rotation_axis(
		p_index, p_joint, static_cast<SkeletonModifier3D::RotationAxis>((int)p_axis));
}

void SpringBoneSimulator3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


// Original VRM Spring Bone movement logic was distributed by (c) VRM Consortium. Licensed under the
// MIT license.

void SpringBoneSimulator3D::_notification(int p_what)
{
	switch (p_what) {
#ifdef TOOLS_ENABLED
	case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
		_make_gizmo_dirty();
	} break;
	case NOTIFICATION_EDITOR_PRE_SAVE: {
		saving = true;
	} break;
	case NOTIFICATION_EDITOR_POST_SAVE: {
		saving = false;
	} break;
#endif // TOOLS_ENABLED
	}
}

String SpringBoneSimulator3D::get_root_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone_name;
}

int SpringBoneSimulator3D::get_root_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone;
}

String SpringBoneSimulator3D::get_end_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->end_bone_name;
}

int SpringBoneSimulator3D::get_end_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->end_bone;
}

bool SpringBoneSimulator3D::is_end_bone_extended(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->extend_end_bone;
}

void SpringBoneSimulator3D::set_end_bone_direction(int p_index, BoneDirection p_bone_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone_direction = p_bone_direction;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes) {
		return; // Chain dir will be recaluclated in _update_bone_axis().
	}
	_make_joints_dirty(p_index, true);
}

SkeletonModifier3D::BoneDirection SpringBoneSimulator3D::get_end_bone_direction(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index]->end_bone_direction;
}

void SpringBoneSimulator3D::set_end_bone_length(int p_index, float p_length)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	float old = settings[p_index]->end_bone_length;
	settings[p_index]->end_bone_length = p_length;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes && Math::is_zero_approx(old) == Math::is_zero_approx(p_length)) {
		return; // If chain size is not changed, length will be recaluclated in _update_bone_axis().
	}
	_make_joints_dirty(p_index, true);
}

float SpringBoneSimulator3D::get_end_bone_length(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->end_bone_length;
}

SpringBoneSimulator3D::CenterFrom SpringBoneSimulator3D::get_center_from(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), CENTER_FROM_WORLD_ORIGIN);
	return settings[p_index]->center_from;
}

NodePath SpringBoneSimulator3D::get_center_node(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->center_node;
}

String SpringBoneSimulator3D::get_center_bone_name(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->center_bone_name;
}

int SpringBoneSimulator3D::get_center_bone(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->center_bone;
}

void SpringBoneSimulator3D::set_radius(int p_index, float p_radius)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->radius = p_radius;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_radius(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->radius;
}

Ref<Curve> SpringBoneSimulator3D::get_radius_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->radius_damping_curve;
}

void SpringBoneSimulator3D::set_stiffness(int p_index, float p_stiffness)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->stiffness = p_stiffness;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_stiffness(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->stiffness;
}

Ref<Curve> SpringBoneSimulator3D::get_stiffness_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->stiffness_damping_curve;
}

void SpringBoneSimulator3D::set_drag(int p_index, float p_drag)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->drag = p_drag;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_drag(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->drag;
}

Ref<Curve> SpringBoneSimulator3D::get_drag_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->drag_damping_curve;
}

void SpringBoneSimulator3D::set_gravity(int p_index, float p_gravity)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->gravity = p_gravity;
	_make_joints_dirty(p_index);
}

float SpringBoneSimulator3D::get_gravity(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->gravity;
}

Ref<Curve> SpringBoneSimulator3D::get_gravity_damping_curve(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index]->gravity_damping_curve;
}

void SpringBoneSimulator3D::set_gravity_direction(int p_index, const Vector3& p_gravity_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_gravity_direction.is_zero_approx());
	if (is_config_individual(p_index)) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->gravity_direction = p_gravity_direction;
	_make_joints_dirty(p_index);
}

Vector3 SpringBoneSimulator3D::get_gravity_direction(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3(0, -1, 0));
	return settings[p_index]->gravity_direction;
}

SkeletonModifier3D::RotationAxis SpringBoneSimulator3D::get_rotation_axis(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	return settings[p_index]->rotation_axis;
}

void SpringBoneSimulator3D::set_rotation_axis_vector(int p_index, const Vector3& p_vector)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (is_config_individual(p_index) || settings[p_index]->rotation_axis != ROTATION_AXIS_CUSTOM) {
		return; // Joint config is individual mode.
	}
	settings[p_index]->rotation_axis_vector = p_vector;
	_make_joints_dirty(p_index);
}

Vector3 SpringBoneSimulator3D::get_rotation_axis_vector(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	Vector3 ret;
	switch (settings[p_index]->rotation_axis) {
	case ROTATION_AXIS_X:
		ret = Vector3(1, 0, 0);
		break;
	case ROTATION_AXIS_Y:
		ret = Vector3(0, 1, 0);
		break;
	case ROTATION_AXIS_Z:
		ret = Vector3(0, 0, 1);
		break;
	case ROTATION_AXIS_ALL:
		ret = Vector3(0, 0, 0);
		break;
	case ROTATION_AXIS_CUSTOM:
		ret = settings[p_index]->rotation_axis_vector;
		break;
	}
	return ret;
}

int SpringBoneSimulator3D::get_setting_count() const { return settings.size(); }

void SpringBoneSimulator3D::clear_settings() { set_setting_count(0); }

// Individual joints.

bool SpringBoneSimulator3D::is_config_individual(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->individual_config;
}

String SpringBoneSimulator3D::get_joint_bone_name(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint]->bone_name;
}

int SpringBoneSimulator3D::get_joint_bone(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint]->bone;
}

void SpringBoneSimulator3D::set_joint_radius(int p_index, int p_joint, float p_radius)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->radius = p_radius;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
}

float SpringBoneSimulator3D::get_joint_radius(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->radius;
}

void SpringBoneSimulator3D::set_joint_stiffness(int p_index, int p_joint, float p_stiffness)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->stiffness = p_stiffness;
}

float SpringBoneSimulator3D::get_joint_stiffness(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->stiffness;
}

void SpringBoneSimulator3D::set_joint_drag(int p_index, int p_joint, float p_drag)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->drag = p_drag;
}

float SpringBoneSimulator3D::get_joint_drag(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->drag;
}

void SpringBoneSimulator3D::set_joint_gravity(int p_index, int p_joint, float p_gravity)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->gravity = p_gravity;
}

float SpringBoneSimulator3D::get_joint_gravity(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0);
	return joints[p_joint]->gravity;
}

void SpringBoneSimulator3D::set_joint_gravity_direction(
	int p_index, int p_joint, const Vector3& p_gravity_direction)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_gravity_direction.is_zero_approx());
	if (!is_config_individual(p_index)) {
		return; // Joints are read-only.
	}
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint]->gravity_direction = p_gravity_direction;
}

Vector3 SpringBoneSimulator3D::get_joint_gravity_direction(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3(0, -1, 0));
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), Vector3(0, -1, 0));
	return joints[p_joint]->gravity_direction;
}

SkeletonModifier3D::RotationAxis SpringBoneSimulator3D::get_joint_rotation_axis(
	int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), ROTATION_AXIS_ALL);
	return joints[p_joint]->rotation_axis;
}

Vector3 SpringBoneSimulator3D::get_joint_rotation_axis_vector(int p_index, int p_joint) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), Vector3());
	return joints[p_joint]->get_rotation_axis_vector();
}

int SpringBoneSimulator3D::get_joint_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<SpringBone3DJointSetting*>& joints = settings[p_index]->joints;
	return joints.size();
}

bool SpringBoneSimulator3D::are_all_child_collisions_enabled(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->enable_all_child_collisions;
}

void SpringBoneSimulator3D::set_exclude_collision_path(
	int p_index, int p_collision, const NodePath& p_node_path)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!are_all_child_collisions_enabled(p_index)) {
		return; // Exclude collision list is disabled.
	}
	LocalVector<NodePath>& setting_exclude_collisions = settings[p_index]->exclude_collisions;
	ERR_FAIL_INDEX(p_collision, (int)setting_exclude_collisions.size());
	setting_exclude_collisions[p_collision] = NodePath(); // Reset first.
	if (is_inside_tree()) {
		Node* node = get_node_or_null(p_node_path);
		if (!node) {
			_make_collisions_dirty();
			return;
		}
		node = node->get_parent();
		if (!node || node != this) {
			_make_collisions_dirty();
			ERR_FAIL_EDMSG("Collision must be child of current SpringBoneSimulator3D.");
		}
	}
	setting_exclude_collisions[p_collision] = p_node_path;
	_make_collisions_dirty();
}

NodePath SpringBoneSimulator3D::get_exclude_collision_path(int p_index, int p_collision) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	const LocalVector<NodePath>& setting_exclude_collisions = settings[p_index]->exclude_collisions;
	ERR_FAIL_INDEX_V(p_collision, (int)setting_exclude_collisions.size(), NodePath());
	return setting_exclude_collisions[p_collision];
}

int SpringBoneSimulator3D::get_exclude_collision_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<NodePath>& setting_exclude_collisions = settings[p_index]->exclude_collisions;
	return setting_exclude_collisions.size();
}

void SpringBoneSimulator3D::clear_exclude_collisions(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (!are_all_child_collisions_enabled(p_index)) {
		return; // Exclude collision list is disabled.
	}
	set_exclude_collision_count(p_index, 0);
}

void SpringBoneSimulator3D::set_collision_path(
	int p_index, int p_collision, const NodePath& p_node_path)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (are_all_child_collisions_enabled(p_index)) {
		return; // Collision list is disabled.
	}
	LocalVector<NodePath>& setting_collisions = settings[p_index]->collisions;
	ERR_FAIL_INDEX(p_collision, (int)setting_collisions.size());
	setting_collisions[p_collision] = NodePath(); // Reset first.
	if (is_inside_tree()) {
		Node* node = get_node_or_null(p_node_path);
		if (!node) {
			_make_collisions_dirty();
			return;
		}
		node = node->get_parent();
		if (!node || node != this) {
			_make_collisions_dirty();
			ERR_FAIL_EDMSG("Collision must be child of current SpringBoneSimulator3D.");
		}
	}
	setting_collisions[p_collision] = p_node_path;
	_make_collisions_dirty();
}

NodePath SpringBoneSimulator3D::get_collision_path(int p_index, int p_collision) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	const LocalVector<NodePath>& setting_collisions = settings[p_index]->collisions;
	ERR_FAIL_INDEX_V(p_collision, (int)setting_collisions.size(), NodePath());
	return setting_collisions[p_collision];
}

int SpringBoneSimulator3D::get_collision_count(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	const LocalVector<NodePath>& setting_collisions = settings[p_index]->collisions;
	return setting_collisions.size();
}

void SpringBoneSimulator3D::clear_collisions(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (are_all_child_collisions_enabled(p_index)) {
		return; // Collision list is disabled.
	}
	set_collision_count(p_index, 0);
}

void SpringBoneSimulator3D::set_external_force(const Vector3& p_force) { external_force = p_force; }

Vector3 SpringBoneSimulator3D::get_external_force() const { return external_force; }

void SpringBoneSimulator3D::set_mutable_bone_axes(bool p_enabled)
{
	mutable_bone_axes = p_enabled;
	for (SpringBone3DSetting* setting : settings) {
		setting->simulation_dirty = true;
	}
}

bool SpringBoneSimulator3D::are_bone_axes_mutable() const { return mutable_bone_axes; }

void SpringBoneSimulator3D::_make_collisions_dirty() { collisions_dirty = true; }

SpringBoneSimulator3D::~SpringBoneSimulator3D() { clear_settings(); }

void SpringBoneSimulator3D::_validate_bone_names() {}

void SpringBoneSimulator3D::_make_joints_dirty(int p_index, bool p_reset) {}

void SpringBoneSimulator3D::_make_gizmo_dirty() {}

void SpringBoneSimulator3D::set_setting_count(int) {}

void SpringBoneSimulator3D::set_rotation_axis(int, SkeletonModifier3D::RotationAxis) {}

void SpringBoneSimulator3D::set_joint_rotation_axis(int, int, SkeletonModifier3D::RotationAxis) {}

void SpringBoneSimulator3D::set_exclude_collision_count(int, int) {}

void SpringBoneSimulator3D::set_collision_count(int, int) {}

void SpringBoneSimulator3D::add_child_notify(Node* p_child) {}

void SpringBoneSimulator3D::remove_child_notify(Node* p_child) {}

void SpringBoneSimulator3D::move_child_notify(Node* p_child) {}

void SpringBoneSimulator3D::_skeleton_changed(
	Skeleton3D* p_old_skeleton, Skeleton3D* p_new_skeleton)
{
}

void SpringBoneSimulator3D::_process_modification(double p_delta) {}

// ============================================================================
// Merged from scene/3d/bone_constraint_3d.cpp
// ============================================================================

#include "scene/3d/skeleton_3d.h"
// --- Inlined from scene/3d/ik_modifier_3d.h ---
#include "scene/3d/skeleton_3d.h"

class Curve3D;

class IKModifier3D : public SkeletonModifier3D {
public:
	enum SolverType {
		SOLVER_TWO_BONE,
		SOLVER_SPLINE,
		SOLVER_CCD,
		SOLVER_FABRIK,
		SOLVER_JACOBIAN,
	};

	struct BoneJoint {
		String name;
		int bone = -1;
	};

	struct IKModifier3DSolverInfo {
		Quaternion current_lpose;
		Quaternion current_lrest;
		Quaternion current_gpose;
		Quaternion current_grest;
		Vector3 current_vector; // Global so needs xform_inv by gpose or grest in the process.
		Vector3 forward_vector; // Local.
		float length = 0.0f;
	};

	struct JointSetting {
		RotationAxis rotation_axis = ROTATION_AXIS_ALL;
		Vector3 rotation_axis_vector = Vector3(1, 0, 0);
		Ref<JointLimitation3D> limitation;
		SecondaryDirection limitation_right_axis = SECONDARY_DIRECTION_NONE;
		Vector3 limitation_right_axis_vector = Vector3(1, 0, 0);
		Quaternion limitation_rotation_offset;

		Vector3 get_rotation_axis_vector() const;
		Vector3 get_limitation_right_axis_vector() const;
		Quaternion get_limitation_space(const Vector3 &p_local_forward) const;
		Vector3 get_projected_rotation(const Quaternion &p_offset, const Vector3 &p_vector) const;
		Vector3 get_limited_rotation(const Quaternion &p_offset, const Vector3 &p_vector, const Vector3 &p_forward) const;
	};

	struct ChainSetting {
		bool simulation_dirty = true;
		bool joints_dirty = false;
		bool simulated = false;

#ifdef TOOLS_ENABLED
		Transform3D root_global_rest;
#endif // TOOLS_ENABLED

		BoneJoint root_bone;
		BoneJoint middle_bone; // Used by SOLVER_TWO_BONE.
		BoneJoint end_bone;

		bool use_virtual_end = false;
		bool extend_end_bone = false;
		BoneDirection end_bone_direction = BONE_DIRECTION_FROM_PARENT;
		float end_bone_length = 0.0f;

		// Target & Pole (IterateIK / TwoBoneIK).
		NodePath target_node;
		NodePath pole_node;
		SecondaryDirection pole_direction = SECONDARY_DIRECTION_NONE;
		Vector3 pole_direction_vector;

		// Spline (SplineIK).
		NodePath path_3d;
		bool tilt_enabled = true;
		int tilt_fade_in = 1;
		int tilt_fade_out = 1;
		LocalVector<double> chain_length_accum;
		LocalVector<double> twists;

		// Chain joints & solver info (IterateIK / SplineIK).
		LocalVector<BoneJoint> joints;
		LocalVector<JointSetting> joint_settings;
		LocalVector<IKModifier3DSolverInfo *> solver_info_list;
		LocalVector<Vector3> chain;

		// Two-bone state (TwoBoneIK).
		IKModifier3DSolverInfo *root_joint_solver_info = nullptr;
		IKModifier3DSolverInfo *mid_joint_solver_info = nullptr;
		Vector3 root_pos;
		Vector3 mid_pos;
		Vector3 end_pos;
		double cached_length_sq = 0.0;

		bool is_two_bone_valid() const { return root_joint_solver_info && mid_joint_solver_info; }
		bool is_two_bone_end_valid() const {
			return (!use_virtual_end && end_bone.bone != -1) || (use_virtual_end && !Math::is_zero_approx(end_bone_length));
		}
		int get_two_bone_end_bone() const { return use_virtual_end ? middle_bone.bone : end_bone.bone; }
		Vector3 get_pole_direction_vector() const;

		void clear_solver_info();

		void update_chain_coordinate(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position);
		void update_chain_coordinate_bw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position);
		void update_chain_coordinate_fw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position);
		void cache_current_vector(Skeleton3D *p_skeleton, int p_index);
		void cache_current_vectors(Skeleton3D *p_skeleton);
		void init_current_joint_rotations(Skeleton3D *p_skeleton);

		void cache_iterate_joint_rotations(Skeleton3D *p_skeleton, double p_angular_delta_limit = Math::PI);
		void init_iterate_joints(Skeleton3D *p_skeleton, bool p_mutable_bone_axes);

		int32_t find_nearest_point(const Vector3 &p_origin, double p_length_sq, const PackedVector3Array &p_points, bool p_is_path_closed, uint32_t p_first, double *r_ret) const;
		void cache_spline_joint_rotations(Skeleton3D *p_skeleton, bool p_use_tilt = false);

		void cache_two_bone_vectors(Skeleton3D *p_skeleton);
		void init_two_bone_joint_rotations(Skeleton3D *p_skeleton);
		void cache_two_bone_joint_rotations(Skeleton3D *p_skeleton, const Vector3 &p_pole_destination);

		~ChainSetting() { clear_solver_info(); }
	};

protected:
#ifdef TOOLS_ENABLED
	bool saving = false;
	bool gizmo_dirty = false;
	void _make_gizmo_dirty();
	void _update_mutable_info();
#endif // TOOLS_ENABLED

	SolverType solver_type = SOLVER_FABRIK;
	bool mutable_bone_axes = true;
	Transform3D cached_space;
	bool joints_dirty = false;

	// Iterative solver settings (CCD / FABRIK / Jacobian).
	int max_iterations = 4;
	double min_distance = 0.001;
	double min_distance_squared = min_distance * min_distance;
	double angular_delta_limit = Math::deg_to_rad(2.0);
	bool deterministic = false;

	LocalVector<ChainSetting *> settings;

	void _notification(int p_what);
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _validate_bone_names() override;
	virtual void _process_modification(double p_delta) override;

	void _make_all_joints_dirty();
	void _init_joints(Skeleton3D *p_skeleton, int p_index);
	void _update_joints(int p_index);
	void _clear_joints(int p_index);
	void _make_simulation_dirty(int p_index);
	void _update_bone_axis(Skeleton3D *p_skeleton, int p_index);

	void _validate_axes(Skeleton3D *p_skeleton) const;
	void _validate_axis(Skeleton3D *p_skeleton, int p_index, int p_joint) const;
	void _validate_pole_directions(Skeleton3D *p_skeleton) const;
	void _validate_pole_direction(Skeleton3D *p_skeleton, int p_index) const;
	Transform3D _get_two_bone_global_rest(Skeleton3D *p_skeleton, int p_bone, int p_root) const;

	void _process_ik(Skeleton3D *p_skeleton, double p_delta);
	void _process_iterate_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _solve_ccd_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _solve_fabrik_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _solve_jacobian_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination);
	void _process_spline_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, Ref<Curve3D> p_curve, const Transform3D &p_curve_space);
	void _process_two_bone_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination, const Vector3 &p_pole_destination);

public:
	IKModifier3D(SolverType p_solver_type = SOLVER_FABRIK);
	~IKModifier3D();

	void set_solver_type(SolverType p_solver_type);
	SolverType get_solver_type() const { return solver_type; }
	bool is_iterative_solver() const {
		return solver_type == SOLVER_CCD || solver_type == SOLVER_FABRIK || solver_type == SOLVER_JACOBIAN;
	}

	virtual PackedStringArray get_configuration_warnings() const override;

	int get_setting_count() const;
	void set_setting_count(int p_count);
	void clear_settings();

	void set_mutable_bone_axes(bool p_enabled);
	bool are_bone_axes_mutable() const;

	// Iterative solver settings.
	void set_max_iterations(int p_max_iterations);
	int get_max_iterations() const;
	void set_min_distance(double p_min_distance);
	double get_min_distance() const;
	void set_angular_delta_limit(double p_angular_delta_limit);
	double get_angular_delta_limit() const;
	void set_deterministic(bool p_deterministic);
	bool is_deterministic() const;

	// Chain / TwoBone bone configuration.
	void set_root_bone_name(int p_index, const String &p_bone_name);
	String get_root_bone_name(int p_index) const;
	void set_root_bone(int p_index, int p_bone);
	int get_root_bone(int p_index) const;

	void set_middle_bone_name(int p_index, const String &p_bone_name);
	String get_middle_bone_name(int p_index) const;
	void set_middle_bone(int p_index, int p_bone);
	int get_middle_bone(int p_index) const;

	void set_end_bone_name(int p_index, const String &p_bone_name);
	String get_end_bone_name(int p_index) const;
	void set_end_bone(int p_index, int p_bone);
	int get_end_bone(int p_index) const;

	void set_use_virtual_end(int p_index, bool p_enabled);
	bool is_using_virtual_end(int p_index) const;
	void set_extend_end_bone(int p_index, bool p_enabled);
	bool is_end_bone_extended(int p_index) const;
	void set_end_bone_direction(int p_index, BoneDirection p_direction);
	BoneDirection get_end_bone_direction(int p_index) const;
	void set_end_bone_length(int p_index, float p_length);
	float get_end_bone_length(int p_index) const;

	// Target / Pole / Path3D settings.
	void set_target_node(int p_index, const NodePath &p_target_node);
	NodePath get_target_node(int p_index) const;

	void set_pole_node(int p_index, const NodePath &p_pole_node);
	NodePath get_pole_node(int p_index) const;
	void set_pole_direction(int p_index, SecondaryDirection p_axis);
	SecondaryDirection get_pole_direction(int p_index) const;
	void set_pole_direction_vector(int p_index, const Vector3 &p_vector);
	Vector3 get_pole_direction_vector(int p_index) const;
	bool is_valid(int p_index) const;

	void set_path_3d(int p_index, const NodePath &p_path_3d);
	NodePath get_path_3d(int p_index) const;
	void set_tilt_enabled(int p_index, bool p_enabled);
	bool is_tilt_enabled(int p_index) const;
	void set_tilt_fade_in(int p_index, int p_size);
	int get_tilt_fade_in(int p_index) const;
	void set_tilt_fade_out(int p_index, int p_size);
	int get_tilt_fade_out(int p_index) const;

	// Individual joints.
	String get_joint_bone_name(int p_index, int p_joint) const;
	int get_joint_bone(int p_index, int p_joint) const;
	void set_joint_count(int p_index, int p_count);
	int get_joint_count(int p_index) const;

	void set_joint_rotation_axis(int p_index, int p_joint, RotationAxis p_axis);
	RotationAxis get_joint_rotation_axis(int p_index, int p_joint) const;
	void set_joint_rotation_axis_vector(int p_index, int p_joint, const Vector3 &p_vector);
	Vector3 get_joint_rotation_axis_vector(int p_index, int p_joint) const;
	void set_joint_limitation(int p_index, int p_joint, const Ref<JointLimitation3D> &p_limitation);
	Ref<JointLimitation3D> get_joint_limitation(int p_index, int p_joint) const;
	void set_joint_limitation_right_axis(int p_index, int p_joint, SecondaryDirection p_direction);
	SecondaryDirection get_joint_limitation_right_axis(int p_index, int p_joint) const;
	void set_joint_limitation_right_axis_vector(int p_index, int p_joint, const Vector3 &p_vector);
	Vector3 get_joint_limitation_right_axis_vector(int p_index, int p_joint) const;
	void set_joint_limitation_rotation_offset(int p_index, int p_joint, const Quaternion &p_offset);
	Quaternion get_joint_limitation_rotation_offset(int p_index, int p_joint) const;
	Quaternion get_joint_limitation_space(int p_index, int p_joint, const Vector3 &p_forward) const;

#ifdef TOOLS_ENABLED
	static Transform3D get_bone_global_rest_mutable(Skeleton3D *p_skeleton, int p_bone);
	Transform3D get_chain_root_global_rest(int p_index) const;
	Vector3 get_bone_vector(int p_index, int p_joint) const;
#endif // TOOLS_ENABLED

	static Quaternion get_local_pose_rotation(Skeleton3D *p_skeleton, int p_bone, const Quaternion &p_global_pose_rotation);
	static Vector3 get_bone_axis(Skeleton3D *p_skeleton, int p_end_bone, BoneDirection p_direction, bool p_mutable_bone_axes);
};

// --- End scene/3d/ik_modifier_3d.h ---


BoneConstraint3D::BoneConstraint3D(ConstraintType p_type) :
		constraint_type(p_type) {
}

BoneConstraint3D::~BoneConstraint3D() {
	clear_settings();
}

void BoneConstraint3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_make_velocity_joints_dirty();
			_make_all_twist_joints_dirty();
		} break;
		default:
			break;
	}
}

void BoneConstraint3D::_set_active(bool p_active) {
	if (p_active) {
		reset();
		_make_all_twist_joints_dirty();
	}
}

void BoneConstraint3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
}

void BoneConstraint3D::set_constraint_type(ConstraintType p_type) {
	if (constraint_type == p_type) {
		return;
	}
	constraint_type = p_type;
	reset();
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
	update_configuration_warnings();
}

PackedStringArray BoneConstraint3D::get_configuration_warnings() const {
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (constraint_type == CONSTRAINT_AIM) {
		for (uint32_t i = 0; i < settings.size(); i++) {
			if (settings[i].use_euler && get_axis_from_bone_axis(settings[i].forward_axis) == settings[i].primary_rotation_axis) {
				warnings.push_back(vformat(RTR("Forward axis and primary rotation axis must not be parallel in setting %s."), itos(i)));
			}
		}
	}
	return warnings;
}

void BoneConstraint3D::set_setting_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	settings.resize(p_count);
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
}

int BoneConstraint3D::get_setting_count() const {
	return (int)settings.size();
}

void BoneConstraint3D::clear_settings() {
	settings.clear();
	velocity_joints.clear();
	velocity_bones.clear();
}

void BoneConstraint3D::_validate_bone_names() {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}
	if (!target_bone_name.is_empty()) {
		target_bone = sk->find_bone(target_bone_name);
	}
	for (uint32_t i = 0; i < settings.size(); i++) {
		if (!settings[i].apply_bone_name.is_empty()) {
			settings[i].apply_bone = sk->find_bone(settings[i].apply_bone_name);
		}
		if (!settings[i].reference_bone_name.is_empty()) {
			settings[i].reference_bone = sk->find_bone(settings[i].reference_bone_name);
		}
		if (!settings[i].root_bone.name.is_empty()) {
			settings[i].root_bone.bone = sk->find_bone(settings[i].root_bone.name);
		}
		if (!settings[i].end_bone.name.is_empty()) {
			settings[i].end_bone.bone = sk->find_bone(settings[i].end_bone.name);
		}
	}
	_make_velocity_joints_dirty();
	_make_all_twist_joints_dirty();
}

void BoneConstraint3D::set_amount(int p_index, float p_amount) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].amount = p_amount;
}

float BoneConstraint3D::get_amount(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].amount;
}

void BoneConstraint3D::set_apply_bone_name(int p_index, const String &p_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].apply_bone = sk->find_bone(p_name);
	}
}

String BoneConstraint3D::get_apply_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].apply_bone_name;
}

void BoneConstraint3D::set_apply_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].apply_bone_name = sk->get_bone_name(p_bone);
	}
}

int BoneConstraint3D::get_apply_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].apply_bone;
}

void BoneConstraint3D::set_reference_type(int p_index, ReferenceType p_type) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_type = p_type;
}

BoneConstraint3D::ReferenceType BoneConstraint3D::get_reference_type(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), REFERENCE_TYPE_BONE);
	return settings[p_index].reference_type;
}

void BoneConstraint3D::set_reference_bone_name(int p_index, const String &p_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_bone_name = p_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].reference_bone = sk->find_bone(p_name);
	}
}

String BoneConstraint3D::get_reference_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].reference_bone_name;
}

void BoneConstraint3D::set_reference_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].reference_bone_name = sk->get_bone_name(p_bone);
	}
}

int BoneConstraint3D::get_reference_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].reference_bone;
}

void BoneConstraint3D::set_reference_node(int p_index, const NodePath &p_node) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_node = p_node;
}

NodePath BoneConstraint3D::get_reference_node(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index].reference_node;
}

void BoneConstraint3D::set_relative(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].relative = p_enabled;
}

bool BoneConstraint3D::is_relative(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].is_relative_effective(constraint_type);
}

void BoneConstraint3D::set_additive(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].additive = p_enabled;
}

bool BoneConstraint3D::is_additive(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].additive;
}

// Aim properties.

void BoneConstraint3D::set_forward_axis(int p_index, BoneAxis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].forward_axis = p_axis;
	update_configuration_warnings();
}

SkeletonModifier3D::BoneAxis BoneConstraint3D::get_forward_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_AXIS_PLUS_Y);
	return settings[p_index].forward_axis;
}

void BoneConstraint3D::set_use_euler(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].use_euler = p_enabled;
	update_configuration_warnings();
}

bool BoneConstraint3D::is_using_euler(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].use_euler;
}

void BoneConstraint3D::set_primary_rotation_axis(int p_index, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].primary_rotation_axis = p_axis;
	update_configuration_warnings();
}

Vector3::Axis BoneConstraint3D::get_primary_rotation_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	return settings[p_index].primary_rotation_axis;
}

void BoneConstraint3D::set_use_secondary_rotation(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].use_secondary_rotation = p_enabled;
}

bool BoneConstraint3D::is_using_secondary_rotation(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].use_secondary_rotation;
}

// Convert Transform properties.

void BoneConstraint3D::set_apply_transform_mode(int p_index, TransformMode p_transform_mode) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_transform_mode = p_transform_mode;
}

BoneConstraint3D::TransformMode BoneConstraint3D::get_apply_transform_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), TRANSFORM_MODE_POSITION);
	return settings[p_index].apply_transform_mode;
}

void BoneConstraint3D::set_apply_axis(int p_index, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_axis = p_axis;
}

Vector3::Axis BoneConstraint3D::get_apply_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	return settings[p_index].apply_axis;
}

void BoneConstraint3D::set_apply_range_min(int p_index, float p_range_min) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_range_min = p_range_min;
}

float BoneConstraint3D::get_apply_range_min(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].apply_range_min;
}

void BoneConstraint3D::set_apply_range_max(int p_index, float p_range_max) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].apply_range_max = p_range_max;
}

float BoneConstraint3D::get_apply_range_max(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].apply_range_max;
}

void BoneConstraint3D::set_reference_transform_mode(int p_index, TransformMode p_transform_mode) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_transform_mode = p_transform_mode;
}

BoneConstraint3D::TransformMode BoneConstraint3D::get_reference_transform_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), TRANSFORM_MODE_POSITION);
	return settings[p_index].reference_transform_mode;
}

void BoneConstraint3D::set_reference_axis(int p_index, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_axis = p_axis;
}

Vector3::Axis BoneConstraint3D::get_reference_axis(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3::AXIS_X);
	return settings[p_index].reference_axis;
}

void BoneConstraint3D::set_reference_range_min(int p_index, float p_range_min) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_range_min = p_range_min;
}

float BoneConstraint3D::get_reference_range_min(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].reference_range_min;
}

void BoneConstraint3D::set_reference_range_max(int p_index, float p_range_max) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].reference_range_max = p_range_max;
}

float BoneConstraint3D::get_reference_range_max(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].reference_range_max;
}

// Copy Transform properties.

void BoneConstraint3D::set_copy_flags(int p_index, uint32_t p_copy_flags) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].copy_flags = p_copy_flags;
}

uint32_t BoneConstraint3D::get_copy_flags(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index].copy_flags;
}

void BoneConstraint3D::set_copy_position(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].copy_flags |= TRANSFORM_FLAG_POSITION;
	} else {
		settings[p_index].copy_flags &= ~TRANSFORM_FLAG_POSITION;
	}
}

bool BoneConstraint3D::is_position_copying(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].copy_flags & TRANSFORM_FLAG_POSITION) != 0;
}

void BoneConstraint3D::set_copy_rotation(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].copy_flags |= TRANSFORM_FLAG_ROTATION;
	} else {
		settings[p_index].copy_flags &= ~TRANSFORM_FLAG_ROTATION;
	}
}

bool BoneConstraint3D::is_rotation_copying(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].copy_flags & TRANSFORM_FLAG_ROTATION) != 0;
}

void BoneConstraint3D::set_copy_scale(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].copy_flags |= TRANSFORM_FLAG_SCALE;
	} else {
		settings[p_index].copy_flags &= ~TRANSFORM_FLAG_SCALE;
	}
}

bool BoneConstraint3D::is_scale_copying(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].copy_flags & TRANSFORM_FLAG_SCALE) != 0;
}

void BoneConstraint3D::set_axis_flags(int p_index, uint32_t p_axis_flags) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].axis_flags = p_axis_flags;
}

uint32_t BoneConstraint3D::get_axis_flags(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index].axis_flags;
}

void BoneConstraint3D::set_axis_x_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].axis_flags |= AXIS_FLAG_X;
	} else {
		settings[p_index].axis_flags &= ~AXIS_FLAG_X;
	}
}

bool BoneConstraint3D::is_axis_x_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].axis_flags & AXIS_FLAG_X) != 0;
}

void BoneConstraint3D::set_axis_y_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].axis_flags |= AXIS_FLAG_Y;
	} else {
		settings[p_index].axis_flags &= ~AXIS_FLAG_Y;
	}
}

bool BoneConstraint3D::is_axis_y_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].axis_flags & AXIS_FLAG_Y) != 0;
}

void BoneConstraint3D::set_axis_z_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].axis_flags |= AXIS_FLAG_Z;
	} else {
		settings[p_index].axis_flags &= ~AXIS_FLAG_Z;
	}
}

bool BoneConstraint3D::is_axis_z_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].axis_flags & AXIS_FLAG_Z) != 0;
}

void BoneConstraint3D::set_invert_flags(int p_index, uint32_t p_axis_flags) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].invert_flags = p_axis_flags;
}

uint32_t BoneConstraint3D::get_invert_flags(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index].invert_flags;
}

void BoneConstraint3D::set_axis_x_inverted(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].invert_flags |= AXIS_FLAG_X;
	} else {
		settings[p_index].invert_flags &= ~AXIS_FLAG_X;
	}
}

bool BoneConstraint3D::is_axis_x_inverted(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].invert_flags & AXIS_FLAG_X) != 0;
}

void BoneConstraint3D::set_axis_y_inverted(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].invert_flags |= AXIS_FLAG_Y;
	} else {
		settings[p_index].invert_flags &= ~AXIS_FLAG_Y;
	}
}

bool BoneConstraint3D::is_axis_y_inverted(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].invert_flags & AXIS_FLAG_Y) != 0;
}

void BoneConstraint3D::set_axis_z_inverted(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	if (p_enabled) {
		settings[p_index].invert_flags |= AXIS_FLAG_Z;
	} else {
		settings[p_index].invert_flags &= ~AXIS_FLAG_Z;
	}
}

bool BoneConstraint3D::is_axis_z_inverted(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return (settings[p_index].invert_flags & AXIS_FLAG_Z) != 0;
}

// --- BoneTarget properties ---

void BoneConstraint3D::set_bone_name(const String &p_bone_name) {
	target_bone_name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		target_bone = sk->find_bone(target_bone_name);
	}
}

String BoneConstraint3D::get_bone_name() const {
	return target_bone_name;
}

void BoneConstraint3D::set_bone(int p_bone) {
	target_bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		target_bone_name = sk->get_bone_name(p_bone);
	}
}

int BoneConstraint3D::get_bone() const {
	return target_bone;
}

// --- Chain root/end properties (LimitAngularVelocity & TwistDisperser) ---

void BoneConstraint3D::set_root_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].root_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].root_bone.bone = sk->find_bone(p_bone_name);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

String BoneConstraint3D::get_root_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].root_bone.name;
}

void BoneConstraint3D::set_root_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].root_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].root_bone.name = sk->get_bone_name(p_bone);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

int BoneConstraint3D::get_root_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].root_bone.bone;
}

void BoneConstraint3D::set_end_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].end_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		settings[p_index].end_bone.bone = sk->find_bone(p_bone_name);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

String BoneConstraint3D::get_end_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index].end_bone.name;
}

void BoneConstraint3D::set_end_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].end_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index].end_bone.name = sk->get_bone_name(p_bone);
	}
	_make_velocity_joints_dirty();
	_make_twist_joints_dirty(p_index);
}

int BoneConstraint3D::get_end_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index].end_bone.bone;
}

// --- LimitAngularVelocity properties & helpers ---

void BoneConstraint3D::set_chain_count(int p_count) {
	set_setting_count(p_count);
}

int BoneConstraint3D::get_chain_count() const {
	return get_setting_count();
}

void BoneConstraint3D::clear_chains() {
	set_chain_count(0);
}

String BoneConstraint3D::get_joint_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)velocity_joints.size(), String());
	return velocity_joints[p_index].name;
}

int BoneConstraint3D::get_joint_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)velocity_joints.size(), -1);
	return velocity_joints[p_index].bone;
}

int BoneConstraint3D::get_velocity_joint_count() const {
	return (int)velocity_joints.size();
}

void BoneConstraint3D::set_max_angular_velocity(double p_angular_velocity) {
	max_angular_velocity = p_angular_velocity;
}

double BoneConstraint3D::get_max_angular_velocity() const {
	return max_angular_velocity;
}

void BoneConstraint3D::set_exclude(bool p_exclude) {
	exclude = p_exclude;
	reset();
}

bool BoneConstraint3D::is_exclude() const {
	return exclude;
}

void BoneConstraint3D::reset() {
	velocity_init_needed = true;
}

void BoneConstraint3D::_make_velocity_joints_dirty() {
	velocity_joints_dirty = true;
	velocity_init_needed = true;
}

bool BoneConstraint3D::_is_velocity_joint_contained(int p_bone) const {
	for (const BoneJoint &joint : velocity_joints) {
		if (joint.bone == p_bone) {
			return true;
		}
	}
	return false;
}

void BoneConstraint3D::_update_velocity_joints() {
	velocity_joints.clear();
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		velocity_joints_dirty = false;
		return;
	}

	for (uint32_t i = 0; i < settings.size(); i++) {
		int root = settings[i].root_bone.bone;
		int end = settings[i].end_bone.bone;
		if (root < 0 || end < 0 || root >= sk->get_bone_count() || end >= sk->get_bone_count()) {
			continue;
		}
		if (root != end && !sk->is_bone_parent_of(end, root)) {
			continue;
		}
		Vector<int> chain_bones;
		int cur = end;
		while (cur != -1) {
			chain_bones.push_back(cur);
			if (cur == root) {
				break;
			}
			cur = sk->get_bone_parent(cur);
		}
		for (int j = chain_bones.size() - 1; j >= 0; j--) {
			int b = chain_bones[j];
			if (!_is_velocity_joint_contained(b)) {
				BoneJoint bj;
				bj.bone = b;
				bj.name = sk->get_bone_name(b);
				velocity_joints.push_back(bj);
			}
		}
	}

	velocity_bones.clear();
	if (exclude) {
		for (int b = 0; b < sk->get_bone_count(); b++) {
			if (!_is_velocity_joint_contained(b)) {
				velocity_bones.push_back(BoneRot(b, Quaternion()));
			}
		}
	} else {
		for (const BoneJoint &joint : velocity_joints) {
			velocity_bones.push_back(BoneRot(joint.bone, Quaternion()));
		}
	}

	velocity_joints_dirty = false;
	velocity_init_needed = true;
}

// --- TwistDisperser properties & helpers ---

void BoneConstraint3D::set_mutable_bone_axes(bool p_enabled) {
	mutable_bone_axes = p_enabled;
	_make_all_twist_joints_dirty();
}

bool BoneConstraint3D::are_bone_axes_mutable() const {
	return mutable_bone_axes;
}

void BoneConstraint3D::set_extend_end_bone(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].extend_end_bone = p_enabled;
	_make_twist_joints_dirty(p_index);
}

bool BoneConstraint3D::is_end_bone_extended(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index].extend_end_bone;
}

void BoneConstraint3D::set_end_bone_direction(int p_index, BoneDirection p_bone_direction) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].end_bone_direction = p_bone_direction;
	_make_twist_joints_dirty(p_index);
}

SkeletonModifier3D::BoneDirection BoneConstraint3D::get_end_bone_direction(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index].end_bone_direction;
}

void BoneConstraint3D::set_twist_from_rest(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].twist_from_rest = p_enabled;
}

bool BoneConstraint3D::is_twist_from_rest(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), true);
	return settings[p_index].twist_from_rest;
}

void BoneConstraint3D::set_twist_from(int p_index, const Quaternion &p_from) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].twist_from = p_from;
}

Quaternion BoneConstraint3D::get_twist_from(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	return settings[p_index].twist_from;
}

void BoneConstraint3D::set_disperse_mode(int p_index, DisperseMode p_disperse_mode) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].disperse_mode = p_disperse_mode;
	_make_twist_joints_dirty(p_index);
}

BoneConstraint3D::DisperseMode BoneConstraint3D::get_disperse_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), DISPERSE_MODE_EVEN);
	return settings[p_index].disperse_mode;
}

void BoneConstraint3D::set_weight_position(int p_index, float p_position) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].weight_position = p_position;
	_make_twist_joints_dirty(p_index);
}

float BoneConstraint3D::get_weight_position(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index].weight_position;
}

void BoneConstraint3D::set_damping_curve(int p_index, const Ref<Curve> &p_damping_curve) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index].damping_curve = p_damping_curve;
	_update_twist_curve(p_index);
}

Ref<Curve> BoneConstraint3D::get_damping_curve(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<Curve>());
	return settings[p_index].damping_curve;
}

String BoneConstraint3D::get_joint_bone_name(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].joint.name;
}

int BoneConstraint3D::get_joint_bone(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint].joint.bone;
}

void BoneConstraint3D::set_joint_twist_amount(int p_index, int p_joint, float p_amount) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX(p_joint, (int)joints.size());
	joints[p_joint].custom_amount = p_amount;
}

float BoneConstraint3D::get_joint_twist_amount(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	const LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), 0.0f);
	return (float)joints[p_joint].custom_amount;
}

void BoneConstraint3D::set_joint_count(int p_index, int p_count) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_count < 0);
	settings[p_index].joints.resize(p_count);
}

int BoneConstraint3D::get_joint_count(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return (int)settings[p_index].joints.size();
}

void BoneConstraint3D::_make_all_twist_joints_dirty() {
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_twist_joints_dirty((int)i);
	}
}

void BoneConstraint3D::_make_twist_joints_dirty(int p_index) {
	if (p_index >= 0 && p_index < (int)settings.size()) {
		settings[p_index].joints_dirty = true;
	}
}

void BoneConstraint3D::_update_twist_curve(int p_index) {
	Ref<Curve> curve = settings[p_index].damping_curve;
	if (curve.is_null()) {
		return;
	}
	LocalVector<DisperseJointSetting> &joints = settings[p_index].joints;
	float unit = (int)joints.size() > 1 ? (1.0f / float((int)joints.size() - 1)) : 0.0f;
	for (uint32_t i = 0; i < joints.size(); i++) {
		joints[i].custom_amount = curve->sample(i * unit);
	}
}

void BoneConstraint3D::_update_twist_joints(int p_index) {
	ConstraintSetting &s = settings[p_index];
	s.joints_dirty = false;

	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}

	int root = s.root_bone.bone;
	int end = s.end_bone.bone;
	if (root < 0 || end < 0 || root >= sk->get_bone_count() || end >= sk->get_bone_count()) {
		s.joints.clear();
		return;
	}
	if (root != end && !sk->is_bone_parent_of(end, root)) {
		s.joints.clear();
		return;
	}

	Vector<int> chain_bones;
	int cur = end;
	while (cur != -1) {
		chain_bones.push_back(cur);
		if (cur == root) {
			break;
		}
		cur = sk->get_bone_parent(cur);
	}

	// If not extending end bone, the end bone itself is the reference bone and not twisted as an intermediate joint.
	int ref_bone = end;
	if (s.extend_end_bone) {
		Vector<int> children = sk->get_bone_children(end);
		if (children.size() == 1) {
			ref_bone = children[0];
		}
	} else if (chain_bones.size() > 1) {
		chain_bones.remove_at(0);
	}

	s.reference_bone = ref_bone;
	s.reference_bone_name = (ref_bone >= 0 && ref_bone < sk->get_bone_count()) ? sk->get_bone_name(ref_bone) : String();

	LocalVector<DisperseJointSetting> old_joints = std::move(s.joints);
	s.joints.resize(chain_bones.size());

	for (int i = 0; i < chain_bones.size(); i++) {
		int b = chain_bones[chain_bones.size() - 1 - i];
		s.joints[i].joint.bone = b;
		s.joints[i].joint.name = sk->get_bone_name(b);
		if ((uint32_t)i < old_joints.size()) {
			s.joints[i].custom_amount = old_joints[i].custom_amount;
		} else {
			s.joints[i].custom_amount = float(i + 1) / float(chain_bones.size());
		}
	}

	const uint32_t j_count = s.joints.size();
	if (j_count == 0) {
		return;
	}

	if (s.disperse_mode == DISPERSE_MODE_EVEN) {
		for (uint32_t i = 0; i < j_count; i++) {
			s.joints[i].amount = double(i + 1) / double(j_count + (s.extend_end_bone ? 0 : 1));
		}
	} else if (s.disperse_mode == DISPERSE_MODE_WEIGHTED) {
		double total_len = 0.0;
		LocalVector<double> accum;
		accum.resize(j_count);
		for (uint32_t i = 0; i < j_count; i++) {
			double seg = (i + 1 < j_count) ? (double)sk->get_bone_rest(s.joints[i + 1].joint.bone).origin.length() : (double)sk->get_bone_rest(ref_bone).origin.length();
			accum[i] = total_len + seg * s.weight_position;
			total_len += seg;
		}
		for (uint32_t i = 0; i < j_count; i++) {
			s.joints[i].amount = total_len > CMP_EPSILON ? CLAMP(accum[i] / total_len, 0.0, 1.0) : double(i + 1) / double(j_count);
		}
	} else {
		_update_twist_curve(p_index);
		for (uint32_t i = 0; i < j_count; i++) {
			s.joints[i].amount = s.joints[i].custom_amount;
		}
	}
}

// --- Processing ---

void BoneConstraint3D::_process_modification(double p_delta) {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}

	if (constraint_type == CONSTRAINT_BONE_TARGET) {
		_process_bone_target(sk);
		return;
	}
	if (constraint_type == CONSTRAINT_LIMIT_ANGULAR_VELOCITY) {
		_process_limit_angular_velocity(sk, p_delta);
		return;
	}
	if (constraint_type == CONSTRAINT_TWIST_DISPERSER) {
		_process_twist_disperser(sk);
		return;
	}

	Transform3D skel_inv = sk->get_global_transform_interpolated().affine_inverse();

	for (uint32_t i = 0; i < settings.size(); i++) {
		const ConstraintSetting &s = settings[i];
		if (s.apply_bone < 0 || s.apply_bone >= sk->get_bone_count() || Math::is_zero_approx(s.amount)) {
			continue;
		}

		if (s.reference_type == REFERENCE_TYPE_BONE) {
			if (s.reference_bone < 0 || s.reference_bone >= sk->get_bone_count()) {
				continue;
			}
			if (constraint_type == CONSTRAINT_AIM) {
				if (s.apply_bone == s.reference_bone) {
					continue;
				}
				Vector3 ref_origin = sk->get_bone_global_pose(s.reference_bone).origin;
				_process_aim(i, sk, s.apply_bone, ref_origin, s.amount);
			} else {
				Transform3D dest = sk->get_bone_pose(s.reference_bone);
				if (s.is_relative_effective(constraint_type)) {
					Vector3 scl_rel = dest.basis.get_scale() / sk->get_bone_rest(s.reference_bone).basis.get_scale();
					dest.basis = sk->get_bone_rest(s.reference_bone).basis.get_rotation_quaternion().inverse() * dest.basis.get_rotation_quaternion();
					dest.basis.scale_local(scl_rel);
					dest.origin = dest.origin - sk->get_bone_rest(s.reference_bone).origin;
				}
				if (constraint_type == CONSTRAINT_CONVERT_TRANSFORM) {
					_process_convert(i, sk, s.apply_bone, dest, s.amount);
				} else {
					_process_copy(i, sk, s.apply_bone, dest, s.amount);
				}
			}
		} else {
			Node3D *ref_node = dynamic_cast<Node3D *>(get_node_or_null(s.reference_node));
			if (!ref_node) {
				continue;
			}
			Transform3D ref_local = skel_inv * ref_node->get_global_transform_interpolated();
			if (constraint_type == CONSTRAINT_AIM) {
				_process_aim(i, sk, s.apply_bone, ref_local.origin, s.amount);
			} else if (constraint_type == CONSTRAINT_CONVERT_TRANSFORM) {
				_process_convert(i, sk, s.apply_bone, ref_local, s.amount);
			} else {
				_process_copy(i, sk, s.apply_bone, ref_local, s.amount);
			}
		}
	}
}

void BoneConstraint3D::_process_bone_target(Skeleton3D *p_skeleton) {
	if (target_bone < 0 || target_bone >= p_skeleton->get_bone_count()) {
		return;
	}
	set_global_transform(p_skeleton->get_global_transform_interpolated() * p_skeleton->get_bone_global_pose(target_bone));
}

void BoneConstraint3D::_process_limit_angular_velocity(Skeleton3D *p_skeleton, double p_delta) {
	if (velocity_joints_dirty) {
		_update_velocity_joints();
	}
	if (velocity_bones.is_empty()) {
		return;
	}
	if (velocity_init_needed || p_delta <= 0.0) {
		for (uint32_t i = 0; i < velocity_bones.size(); i++) {
			int b = velocity_bones[i].first;
			if (b >= 0 && b < p_skeleton->get_bone_count()) {
				velocity_bones[i].second = p_skeleton->get_bone_pose_rotation(b);
			}
		}
		velocity_init_needed = false;
		return;
	}

	double limit_angle = max_angular_velocity * p_delta;
	for (uint32_t i = 0; i < velocity_bones.size(); i++) {
		int b = velocity_bones[i].first;
		if (b < 0 || b >= p_skeleton->get_bone_count()) {
			continue;
		}
		Quaternion prev_rot = velocity_bones[i].second;
		Quaternion curr_rot = p_skeleton->get_bone_pose_rotation(b);
		double angle_diff = prev_rot.angle_to(curr_rot);
		if (angle_diff > limit_angle && angle_diff > CMP_EPSILON) {
			curr_rot = prev_rot.slerp(curr_rot, real_t(limit_angle / angle_diff));
			p_skeleton->set_bone_pose_rotation(b, curr_rot);
		}
		velocity_bones[i].second = curr_rot;
	}
}

void BoneConstraint3D::_process_twist_disperser(Skeleton3D *p_skeleton) {
	for (uint32_t i = 0; i < settings.size(); i++) {
		if (settings[i].joints_dirty) {
			_update_twist_joints((int)i);
		}
		ConstraintSetting &s = settings[i];
		if (s.joints.is_empty() || s.reference_bone < 0 || s.reference_bone >= p_skeleton->get_bone_count()) {
			continue;
		}

		Vector3 ref_axis = IKModifier3D::get_bone_axis(p_skeleton, s.end_bone.bone, s.end_bone_direction, mutable_bone_axes);
		if (ref_axis.is_zero_approx()) {
			continue;
		}
		ref_axis.normalize();

		Quaternion base_rot = s.twist_from_rest ? p_skeleton->get_bone_rest(s.reference_bone).basis.get_rotation_quaternion() : s.twist_from;
		Quaternion ref_delta = base_rot.inverse() * p_skeleton->get_bone_pose_rotation(s.reference_bone);
		double total_twist = get_roll_angle(ref_delta, ref_axis);

		for (uint32_t j = 0; j < s.joints.size(); j++) {
			int b = s.joints[j].joint.bone;
			if (b < 0 || b >= p_skeleton->get_bone_count()) {
				continue;
			}
			int next_b = (j + 1 < s.joints.size()) ? s.joints[j + 1].joint.bone : s.reference_bone;
			Vector3 joint_axis = (next_b != b) ? (mutable_bone_axes ? p_skeleton->get_bone_pose_position(next_b) : p_skeleton->get_bone_rest(next_b).origin) : ref_axis;
			if (joint_axis.is_zero_approx()) {
				joint_axis = ref_axis;
			} else {
				joint_axis.normalize();
			}
			s.joints[j].axis = joint_axis;

			double prev_amount = j > 0 ? s.joints[j - 1].amount : 0.0;
			double delta_twist = total_twist * ((s.disperse_mode == DISPERSE_MODE_CUSTOM ? s.joints[j].custom_amount : s.joints[j].amount) - prev_amount);
			Quaternion swing = get_swing(p_skeleton->get_bone_pose_rotation(b), joint_axis);
			Quaternion twist_rot(joint_axis, real_t(delta_twist));
			p_skeleton->set_bone_pose_rotation(b, swing * twist_rot);
		}

		if (!s.extend_end_bone && s.reference_bone == s.end_bone.bone) {
			double last_amount = s.disperse_mode == DISPERSE_MODE_CUSTOM ? s.joints[s.joints.size() - 1].custom_amount : s.joints[s.joints.size() - 1].amount;
			double remaining_twist = total_twist * (1.0 - last_amount);
			Quaternion end_swing = get_swing(p_skeleton->get_bone_pose_rotation(s.end_bone.bone), ref_axis);
			p_skeleton->set_bone_pose_rotation(s.end_bone.bone, end_swing * Quaternion(ref_axis, real_t(remaining_twist)));
		}
	}
}

void BoneConstraint3D::_process_aim(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Vector3 &p_target, float p_amount) {
	const ConstraintSetting &setting = settings[p_index];

	Transform3D src_bone_rest = setting.relative ? p_skeleton->get_bone_pose(p_apply_bone) : p_skeleton->get_bone_rest(p_apply_bone);
	Transform3D bone_rest_space;
	int parent_bone = p_skeleton->get_bone_parent(p_apply_bone);
	if (parent_bone < 0) {
		bone_rest_space.translate_local(src_bone_rest.origin);
	} else {
		bone_rest_space = p_skeleton->get_bone_global_pose(parent_bone);
		bone_rest_space.translate_local(src_bone_rest.origin);
	}
	Vector3 forward_vector = bone_rest_space.basis.get_rotation_quaternion().xform_inv(p_target - bone_rest_space.origin);
	if (forward_vector.is_zero_approx()) {
		return;
	}
	forward_vector.normalize();

	Quaternion destination;
	if (setting.use_euler) {
		Vector3 current_vector = LookAtModifier3D::get_basis_vector_from_bone_axis(src_bone_rest.basis, setting.forward_axis).normalized();
		Vector2 src_vec2 = LookAtModifier3D::get_projection_vector(src_bone_rest.basis.xform_inv(forward_vector), setting.primary_rotation_axis).normalized();
		Vector2 dst_vec2 = LookAtModifier3D::get_projection_vector(src_bone_rest.basis.xform_inv(current_vector), setting.primary_rotation_axis).normalized();
		real_t calculated_angle = src_vec2.angle_to(dst_vec2);
		Transform3D primary_result = src_bone_rest.rotated_local(get_vector_from_axis(setting.primary_rotation_axis), calculated_angle);
		Transform3D current_result = primary_result;
		if (setting.use_secondary_rotation) {
			Vector3::Axis secondary_rotation_axis = LookAtModifier3D::get_secondary_rotation_axis(setting.forward_axis, setting.primary_rotation_axis);
			current_vector = LookAtModifier3D::get_basis_vector_from_bone_axis(primary_result.basis, setting.forward_axis).normalized();
			src_vec2 = LookAtModifier3D::get_projection_vector(primary_result.basis.xform_inv(forward_vector), secondary_rotation_axis).normalized();
			dst_vec2 = LookAtModifier3D::get_projection_vector(primary_result.basis.xform_inv(current_vector), secondary_rotation_axis).normalized();
			calculated_angle = src_vec2.angle_to(dst_vec2);
			current_result = primary_result.rotated_local(get_vector_from_axis(secondary_rotation_axis), calculated_angle);
		}
		destination = current_result.basis.get_rotation_quaternion();
	} else {
		Vector3 current_vector = LookAtModifier3D::get_basis_vector_from_bone_axis(src_bone_rest.basis, setting.forward_axis).normalized();
		destination = Quaternion(current_vector, forward_vector) * src_bone_rest.basis.get_rotation_quaternion();
	}

	p_skeleton->set_bone_pose_rotation(p_apply_bone, p_skeleton->get_bone_pose_rotation(p_apply_bone).slerp(destination, p_amount));
}

void BoneConstraint3D::_process_convert(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount) {
	const ConstraintSetting &setting = settings[p_index];
	bool rel = setting.is_relative_effective(constraint_type);

	Transform3D destination = p_destination;
	double point = 0.0;
	int axis = (int)setting.reference_axis;
	switch (setting.reference_transform_mode) {
		case TRANSFORM_MODE_POSITION: {
			point = destination.origin[axis];
		} break;
		case TRANSFORM_MODE_ROTATION: {
			Quaternion tgt_rot = destination.basis.get_rotation_quaternion();
			point = get_roll_angle(tgt_rot, get_vector_from_axis(setting.reference_axis));
		} break;
		case TRANSFORM_MODE_SCALE: {
			point = destination.basis.get_scale()[axis];
		} break;
	}

	destination = p_skeleton->get_bone_pose(p_apply_bone);
	if (Math::is_equal_approx(setting.reference_range_min, setting.reference_range_max)) {
		point = point <= (double)setting.reference_range_min ? 0 : 1;
	} else {
		point = Math::inverse_lerp((double)setting.reference_range_min, (double)setting.reference_range_max, point);
	}
	point = Math::lerp((double)setting.apply_range_min, (double)setting.apply_range_max, CLAMP(point, 0, 1));
	axis = (int)setting.apply_axis;

	switch (setting.apply_transform_mode) {
		case TRANSFORM_MODE_POSITION: {
			if (setting.additive) {
				point = p_skeleton->get_bone_pose(p_apply_bone).origin[axis] + point;
			} else if (rel) {
				point = p_skeleton->get_bone_rest(p_apply_bone).origin[axis] + point;
			}
			destination.origin[axis] = point;
		} break;
		case TRANSFORM_MODE_ROTATION: {
			Vector3 rot_axis = get_vector_from_axis(setting.apply_axis);
			Vector3 dest_scl = destination.basis.get_scale();
			if (influence < 1.0 || p_amount < 1.0) {
				point = CLAMP(point, CMP_EPSILON - Math::PI, Math::PI - CMP_EPSILON);
			}
			Quaternion rot(rot_axis, point);
			if (setting.additive) {
				destination.basis = p_skeleton->get_bone_pose(p_apply_bone).basis.get_rotation_quaternion() * rot;
			} else if (rel) {
				destination.basis = p_skeleton->get_bone_rest(p_apply_bone).basis.get_rotation_quaternion() * rot;
			} else {
				destination.basis = rot;
			}
			destination.basis.scale_local(dest_scl);
		} break;
		case TRANSFORM_MODE_SCALE: {
			Vector3 dest_scl = p_skeleton->get_bone_pose(p_apply_bone).basis.get_scale();
			if (setting.additive) {
				dest_scl[axis] = dest_scl[axis] * point;
			} else if (rel) {
				dest_scl = p_skeleton->get_bone_rest(p_apply_bone).basis.get_scale();
				dest_scl[axis] = dest_scl[axis] * point;
			} else {
				dest_scl[axis] = point;
			}
			destination.basis = destination.basis.orthonormalized().scaled_local(dest_scl);
		} break;
	}

	destination = p_skeleton->get_bone_pose(p_apply_bone).interpolate_with(destination, p_amount);
	switch (setting.apply_transform_mode) {
		case TRANSFORM_MODE_POSITION:
			p_skeleton->set_bone_pose_position(p_apply_bone, destination.origin);
			break;
		case TRANSFORM_MODE_ROTATION:
			p_skeleton->set_bone_pose_rotation(p_apply_bone, destination.basis.get_rotation_quaternion());
			break;
		case TRANSFORM_MODE_SCALE:
			p_skeleton->set_bone_pose_scale(p_apply_bone, destination.basis.get_scale());
			break;
	}
}

void BoneConstraint3D::_process_copy(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount) {
	const ConstraintSetting &setting = settings[p_index];
	bool rel = setting.is_relative_effective(constraint_type);
	Transform3D cur_pose = p_skeleton->get_bone_pose(p_apply_bone);
	Transform3D rest_pose = p_skeleton->get_bone_rest(p_apply_bone);
	Transform3D target = cur_pose;

	if (setting.copy_flags & TRANSFORM_FLAG_POSITION) {
		Vector3 pos = p_destination.origin;
		for (int a = 0; a < 3; a++) {
			if (!(setting.axis_flags & (1 << a))) {
				pos[a] = 0.0f;
				continue;
			}
			if (setting.invert_flags & (1 << a)) {
				pos[a] = -pos[a];
			}
			if (setting.additive) {
				target.origin[a] = cur_pose.origin[a] + pos[a];
			} else if (rel) {
				target.origin[a] = rest_pose.origin[a] + pos[a];
			} else {
				target.origin[a] = pos[a];
			}
		}
		p_skeleton->set_bone_pose_position(p_apply_bone, cur_pose.origin.lerp(target.origin, p_amount));
	}

	if (setting.copy_flags & TRANSFORM_FLAG_ROTATION) {
		Vector3 euler = p_destination.basis.get_euler();
		for (int a = 0; a < 3; a++) {
			if (!(setting.axis_flags & (1 << a))) {
				euler[a] = 0.0f;
			} else if (setting.invert_flags & (1 << a)) {
				euler[a] = -euler[a];
			}
		}
		Quaternion rot = Quaternion::from_euler(euler);
		Quaternion base_rot = cur_pose.basis.get_rotation_quaternion();
		Quaternion dst_rot = rot;
		if (setting.additive) {
			dst_rot = base_rot * rot;
		} else if (rel) {
			dst_rot = rest_pose.basis.get_rotation_quaternion() * rot;
		}
		p_skeleton->set_bone_pose_rotation(p_apply_bone, base_rot.slerp(dst_rot, p_amount));
	}

	if (setting.copy_flags & TRANSFORM_FLAG_SCALE) {
		Vector3 scl = p_destination.basis.get_scale();
		Vector3 cur_scl = cur_pose.basis.get_scale();
		Vector3 rest_scl = rest_pose.basis.get_scale();
		Vector3 dst_scl = cur_scl;
		for (int a = 0; a < 3; a++) {
			if (!(setting.axis_flags & (1 << a))) {
				continue;
			}
			real_t v = scl[a];
			if ((setting.invert_flags & (1 << a)) && !Math::is_zero_approx(v)) {
				v = 1.0f / v;
			}
			if (setting.additive) {
				dst_scl[a] = cur_scl[a] * v;
			} else if (rel) {
				dst_scl[a] = rest_scl[a] * v;
			} else {
				dst_scl[a] = v;
			}
		}
		p_skeleton->set_bone_pose_scale(p_apply_bone, cur_scl.lerp(dst_scl, p_amount));
	}
}

// ============================================================================
// Merged from scene/3d/ik_modifier_3d.cpp
// ============================================================================


#include "core/config/engine.h"
#include "scene/3d/node_3d.h"

// --- JointSetting ---

Vector3 IKModifier3D::JointSetting::get_rotation_axis_vector() const {
	switch (rotation_axis) {
		case ROTATION_AXIS_X:
			return Vector3(1, 0, 0);
		case ROTATION_AXIS_Y:
			return Vector3(0, 1, 0);
		case ROTATION_AXIS_Z:
			return Vector3(0, 0, 1);
		case ROTATION_AXIS_ALL:
			return Vector3(0, 0, 0);
		case ROTATION_AXIS_CUSTOM:
			return rotation_axis_vector;
	}
	return Vector3(0, 0, 0);
}

Vector3 IKModifier3D::JointSetting::get_limitation_right_axis_vector() const {
	switch (limitation_right_axis) {
		case SECONDARY_DIRECTION_NONE:
			return Vector3(0, 0, 0);
		case SECONDARY_DIRECTION_PLUS_X:
			return Vector3(1, 0, 0);
		case SECONDARY_DIRECTION_MINUS_X:
			return Vector3(-1, 0, 0);
		case SECONDARY_DIRECTION_PLUS_Y:
			return Vector3(0, 1, 0);
		case SECONDARY_DIRECTION_MINUS_Y:
			return Vector3(0, -1, 0);
		case SECONDARY_DIRECTION_PLUS_Z:
			return Vector3(0, 0, 1);
		case SECONDARY_DIRECTION_MINUS_Z:
			return Vector3(0, 0, -1);
		case SECONDARY_DIRECTION_CUSTOM:
			return limitation_right_axis_vector;
	}
	return Vector3(0, 0, 0);
}

Quaternion IKModifier3D::JointSetting::get_limitation_space(const Vector3 &p_local_forward) const {
	if (limitation.is_null()) {
		return Quaternion();
	}
	return limitation->make_space(p_local_forward, get_limitation_right_axis_vector(), limitation_rotation_offset);
}

Vector3 IKModifier3D::JointSetting::get_projected_rotation(const Quaternion &p_offset, const Vector3 &p_vector) const {
	ERR_FAIL_COND_V(rotation_axis == ROTATION_AXIS_ALL, p_vector);
	const double ALMOST_ONE = 1.0 - CMP_EPSILON;
	Vector3 axis = get_rotation_axis_vector().normalized();
	Vector3 local_vector = p_offset.xform_inv(p_vector);
	double length = local_vector.length();
	Vector3 projected = snap_vector_to_plane(axis, local_vector.normalized());
	if (!Math::is_zero_approx(length)) {
		projected = projected.normalized() * length;
	}
	if (Math::abs(local_vector.normalized().dot(axis)) > ALMOST_ONE) {
		return p_vector;
	}
	return p_offset.xform(projected);
}

Vector3 IKModifier3D::JointSetting::get_limited_rotation(const Quaternion &p_offset, const Vector3 &p_vector, const Vector3 &p_forward) const {
	ERR_FAIL_COND_V(limitation.is_null(), p_vector);
	Vector3 local_vector = p_offset.xform_inv(p_vector);
	float length = local_vector.length();
	if (Math::is_zero_approx(length)) {
		return p_vector;
	}
	Vector3 limited = limitation->solve(p_forward, get_limitation_right_axis_vector(), limitation_rotation_offset, local_vector.normalized()) * length;
	return p_offset.xform(limited);
}

// --- ChainSetting ---

Vector3 IKModifier3D::ChainSetting::get_pole_direction_vector() const {
	switch (pole_direction) {
		case SECONDARY_DIRECTION_NONE:
			return Vector3(0, 0, 0);
		case SECONDARY_DIRECTION_PLUS_X:
			return Vector3(1, 0, 0);
		case SECONDARY_DIRECTION_MINUS_X:
			return Vector3(-1, 0, 0);
		case SECONDARY_DIRECTION_PLUS_Y:
			return Vector3(0, 1, 0);
		case SECONDARY_DIRECTION_MINUS_Y:
			return Vector3(0, -1, 0);
		case SECONDARY_DIRECTION_PLUS_Z:
			return Vector3(0, 0, 1);
		case SECONDARY_DIRECTION_MINUS_Z:
			return Vector3(0, 0, -1);
		case SECONDARY_DIRECTION_CUSTOM:
			return pole_direction_vector;
	}
	return Vector3(0, 0, 0);
}

void IKModifier3D::ChainSetting::clear_solver_info() {
	for (uint32_t i = 0; i < solver_info_list.size(); i++) {
		if (solver_info_list[i]) {
			memdelete(solver_info_list[i]);
			solver_info_list[i] = nullptr;
		}
	}
	solver_info_list.clear();
	if (root_joint_solver_info) {
		memdelete(root_joint_solver_info);
		root_joint_solver_info = nullptr;
	}
	if (mid_joint_solver_info) {
		memdelete(mid_joint_solver_info);
		mid_joint_solver_info = nullptr;
	}
}

void IKModifier3D::ChainSetting::update_chain_coordinate(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position) {
	if (Math::is_zero_approx(chain[p_index].distance_to(p_position))) {
		return;
	}
	chain[p_index] = p_position;
	cache_current_vector(p_skeleton, p_index);
}

void IKModifier3D::ChainSetting::update_chain_coordinate_bw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position) {
	if (Math::is_zero_approx(chain[p_index].distance_to(p_position))) {
		return;
	}
	Vector3 result = p_position;
	int HEAD = p_index - 1;
	int TAIL = p_index;
	if (HEAD >= 0 && HEAD < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (solver_info) {
			Vector3 old_head_to_tail = solver_info->current_vector;
			Vector3 new_head_to_tail = (result - chain[HEAD]).normalized();
			if (Math::is_equal_approx((double)old_head_to_tail.dot(new_head_to_tail), -1.0)) {
				chain[TAIL] = chain[HEAD] + old_head_to_tail * solver_info->length;
				return;
			}
		}
	}
	chain[p_index] = result;
	cache_current_vector(p_skeleton, p_index);
}

void IKModifier3D::ChainSetting::update_chain_coordinate_fw(Skeleton3D *p_skeleton, int p_index, const Vector3 &p_position) {
	if (Math::is_zero_approx(chain[p_index].distance_to(p_position))) {
		return;
	}
	Vector3 result = p_position;
	int HEAD = p_index;
	int TAIL = p_index + 1;
	if (TAIL >= 0 && TAIL < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (solver_info) {
			Vector3 old_head_to_tail = solver_info->current_vector;
			Vector3 new_head_to_tail = (chain[TAIL] - result).normalized();
			if (Math::is_equal_approx((double)old_head_to_tail.dot(new_head_to_tail), -1.0)) {
				chain[HEAD] = chain[TAIL] - old_head_to_tail * solver_info->length;
				return;
			}
		}
	}
	chain[p_index] = result;
	cache_current_vector(p_skeleton, p_index);
}

void IKModifier3D::ChainSetting::cache_current_vector(Skeleton3D *p_skeleton, int p_index) {
	int cur_head = p_index - 1;
	int cur_tail = p_index;
	if (cur_head >= 0 && cur_head < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[cur_head];
		if (solver_info) {
			solver_info->current_vector = (chain[cur_tail] - chain[cur_head]).normalized();
		}
	}
	cur_head = p_index;
	cur_tail = p_index + 1;
	if (cur_tail < (int)chain.size() && cur_head < (int)solver_info_list.size()) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[cur_head];
		if (solver_info) {
			solver_info->current_vector = (chain[cur_tail] - chain[cur_head]).normalized();
		}
	}
}

void IKModifier3D::ChainSetting::cache_current_vectors(Skeleton3D *p_skeleton) {
	for (uint32_t i = 0; i < joints.size(); i++) {
		int HEAD = i;
		int TAIL = i + 1;
		if (HEAD >= (int)solver_info_list.size() || TAIL >= (int)chain.size()) {
			break;
		}
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (!solver_info) {
			continue;
		}
		solver_info->current_vector = (chain[TAIL] - chain[HEAD]).normalized();
	}
}

void IKModifier3D::ChainSetting::init_current_joint_rotations(Skeleton3D *p_skeleton) {
	if (root_bone.bone < 0) {
		return;
	}
	Quaternion parent_gpose;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose = p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion();
	}
	for (uint32_t i = 0; i < joints.size(); i++) {
		IKModifier3DSolverInfo *solver_info = solver_info_list[i];
		if (!solver_info) {
			continue;
		}
		solver_info->current_lrest = p_skeleton->get_bone_pose(joints[i].bone).basis.get_rotation_quaternion();
		solver_info->current_grest = parent_gpose * solver_info->current_lrest;
		solver_info->current_grest.normalize();
		solver_info->current_lpose = solver_info->current_lrest;
		solver_info->current_gpose = solver_info->current_grest;
		parent_gpose = solver_info->current_gpose;
	}
	cache_current_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::cache_iterate_joint_rotations(Skeleton3D *p_skeleton, double p_angular_delta_limit) {
	Transform3D parent_gpose_tr;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose_tr = p_skeleton->get_bone_global_pose(parent);
	}
	Quaternion parent_gpose = parent_gpose_tr.basis.get_rotation_quaternion();

	for (uint32_t i = 0; i < joints.size(); i++) {
		int HEAD = i;
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (!solver_info) {
			continue;
		}
		solver_info->current_lrest = p_skeleton->get_bone_pose(joints[HEAD].bone).basis.get_rotation_quaternion();
		solver_info->current_grest = parent_gpose * solver_info->current_lrest;
		solver_info->current_grest.normalize();
		Vector3 from = solver_info->forward_vector;
		Vector3 to = solver_info->current_grest.xform_inv(solver_info->current_vector).normalized();
		Quaternion prev = solver_info->current_lpose;
		if (joint_settings[HEAD].rotation_axis == ROTATION_AXIS_ALL) {
			solver_info->current_lpose = solver_info->current_lrest * get_swing(Quaternion(from, to), from);
		} else {
			solver_info->current_lpose = solver_info->current_lrest * get_from_to_rotation_by_axis(from, to, joint_settings[HEAD].get_rotation_axis_vector().normalized());
		}
		double diff = prev.angle_to(solver_info->current_lpose);
		if (!Math::is_zero_approx(diff)) {
			solver_info->current_lpose = prev.slerp(solver_info->current_lpose, MIN(1.0, p_angular_delta_limit / diff));
		}
		solver_info->current_gpose = parent_gpose * solver_info->current_lpose;
		solver_info->current_gpose.normalize();
		parent_gpose = solver_info->current_gpose;
	}

	if (chain.is_empty()) {
		return;
	}
	chain[0] = p_skeleton->get_bone_global_pose(root_bone.bone).origin;
	for (uint32_t i = 0; i < solver_info_list.size(); i++) {
		int HEAD = i;
		int TAIL = i + 1;
		IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
		if (!solver_info || TAIL >= (int)chain.size()) {
			continue;
		}
		chain[TAIL] = chain[HEAD] + solver_info->current_gpose.xform(solver_info->forward_vector) * solver_info->length;
	}
	cache_current_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::init_iterate_joints(Skeleton3D *p_skeleton, bool p_mutable_bone_axes) {
	chain.clear();
	bool extends_end = extend_end_bone && end_bone_length > 0;
	for (uint32_t i = 0; i < joints.size(); i++) {
		chain.push_back(p_skeleton->get_bone_global_pose(joints[i].bone).origin);
		bool last = i == joints.size() - 1;
		if (last && extends_end) {
			Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, end_bone.bone, end_bone_direction, p_mutable_bone_axes);
			if (axis.is_zero_approx()) {
				continue;
			}
			if (!solver_info_list[i]) {
				solver_info_list[i] = memnew(IKModifier3DSolverInfo);
			}
			solver_info_list[i]->forward_vector = snap_vector_to_plane(joint_settings[i].get_rotation_axis_vector(), axis.normalized());
			solver_info_list[i]->length = end_bone_length;
			chain.push_back(p_skeleton->get_bone_global_pose(joints[i].bone).xform(axis * end_bone_length));
		} else if (!last) {
			Vector3 axis = p_skeleton->get_bone_rest(joints[i + 1].bone).origin;
			if (axis.is_zero_approx()) {
				continue;
			}
			if (!solver_info_list[i]) {
				solver_info_list[i] = memnew(IKModifier3DSolverInfo);
			}
			solver_info_list[i]->forward_vector = snap_vector_to_plane(joint_settings[i].get_rotation_axis_vector(), axis.normalized());
			solver_info_list[i]->length = axis.length();
		}
	}
	init_current_joint_rotations(p_skeleton);
}

int32_t IKModifier3D::ChainSetting::find_nearest_point(const Vector3 &p_origin, double p_length_sq, const PackedVector3Array &p_points, bool p_is_path_closed, uint32_t p_first, double *r_ret) const {
	ERR_FAIL_COND_V(p_first >= p_points.size(), p_points.size() - 1);
	uint32_t i = p_first;
	if (p_is_path_closed) {
		while (true) {
			if (p_origin.distance_squared_to(p_points[i]) >= p_length_sq) {
				break;
			}
			i++;
			i = Math::posmod(i, p_points.size());
			if (i == p_first) {
				i = p_points.size();
				break;
			}
		}
	} else {
		while (i < p_points.size()) {
			if (p_origin.distance_squared_to(p_points[i]) >= p_length_sq) {
				break;
			}
			i++;
		}
	}
	if (i == 0) {
		*r_ret = 0.0;
		return p_first;
	} else if (i >= p_points.size()) {
		*r_ret = 1.0;
		return i;
	}
	*r_ret = Math::inverse_lerp((double)p_origin.distance_squared_to(p_points[i - 1]), (double)p_origin.distance_squared_to(p_points[i]), p_length_sq);
	return i - 1;
}

void IKModifier3D::ChainSetting::cache_spline_joint_rotations(Skeleton3D *p_skeleton, bool p_use_tilt) {
	Transform3D parent_gpose_tr;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose_tr = p_skeleton->get_bone_global_pose(parent);
	}
	Quaternion parent_gpose = parent_gpose_tr.basis.get_rotation_quaternion();

	if (p_use_tilt) {
		double parent_twist = 0.0;
		for (uint32_t i = 0; i < joints.size(); i++) {
			int HEAD = i;
			IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
			if (!solver_info) {
				continue;
			}
			solver_info->current_lrest = p_skeleton->get_bone_pose(joints[HEAD].bone).basis.get_rotation_quaternion();
			solver_info->current_grest = parent_gpose * solver_info->current_lrest;
			solver_info->current_grest.normalize();
			Vector3 from = solver_info->forward_vector;
			Vector3 to = solver_info->current_grest.xform_inv(solver_info->current_vector).normalized();

			Basis b = get_swing(Quaternion(from, to), from);
			b.rotate_local(from, twists[HEAD] - parent_twist);
			parent_twist = twists[HEAD];
			solver_info->current_lpose = solver_info->current_lrest * b.get_rotation_quaternion();

			solver_info->current_gpose = parent_gpose * solver_info->current_lpose;
			solver_info->current_gpose.normalize();
			parent_gpose = solver_info->current_gpose;
		}
	} else {
		for (uint32_t i = 0; i < joints.size(); i++) {
			int HEAD = i;
			IKModifier3DSolverInfo *solver_info = solver_info_list[HEAD];
			if (!solver_info) {
				continue;
			}
			solver_info->current_lrest = p_skeleton->get_bone_pose(joints[HEAD].bone).basis.get_rotation_quaternion();
			solver_info->current_grest = parent_gpose * solver_info->current_lrest;
			solver_info->current_grest.normalize();
			Vector3 from = solver_info->forward_vector;
			Vector3 to = solver_info->current_grest.xform_inv(solver_info->current_vector).normalized();

			solver_info->current_lpose = solver_info->current_lrest * get_swing(Quaternion(from, to), from);

			solver_info->current_gpose = parent_gpose * solver_info->current_lpose;
			solver_info->current_gpose.normalize();
			parent_gpose = solver_info->current_gpose;
		}
	}
	cache_current_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::cache_two_bone_vectors(Skeleton3D *p_skeleton) {
	if (!is_two_bone_valid()) {
		return;
	}
	root_joint_solver_info->current_vector = (mid_pos - root_pos).normalized();
	mid_joint_solver_info->current_vector = (end_pos - mid_pos).normalized();
}

void IKModifier3D::ChainSetting::init_two_bone_joint_rotations(Skeleton3D *p_skeleton) {
	if (!is_two_bone_valid()) {
		return;
	}

	Quaternion parent_gpose;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose = p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion();
	}
	root_joint_solver_info->current_lrest = p_skeleton->get_bone_pose(root_bone.bone).basis.get_rotation_quaternion();
	root_joint_solver_info->current_grest = parent_gpose * root_joint_solver_info->current_lrest;
	root_joint_solver_info->current_grest.normalize();
	root_joint_solver_info->current_lpose = root_joint_solver_info->current_lrest;
	root_joint_solver_info->current_gpose = root_joint_solver_info->current_grest;
	parent_gpose = root_joint_solver_info->current_gpose;

	mid_joint_solver_info->current_lrest = p_skeleton->get_bone_global_pose(root_bone.bone).basis.get_rotation_quaternion().inverse() *
			p_skeleton->get_bone_global_pose(middle_bone.bone).basis.get_rotation_quaternion();
	mid_joint_solver_info->current_grest = parent_gpose * mid_joint_solver_info->current_lrest;
	mid_joint_solver_info->current_grest.normalize();
	mid_joint_solver_info->current_lpose = mid_joint_solver_info->current_lrest;
	mid_joint_solver_info->current_gpose = mid_joint_solver_info->current_grest;

	cache_two_bone_vectors(p_skeleton);
}

void IKModifier3D::ChainSetting::cache_two_bone_joint_rotations(Skeleton3D *p_skeleton, const Vector3 &p_pole_destination) {
	if (!is_two_bone_valid()) {
		return;
	}

	Quaternion parent_gpose;
	int parent = p_skeleton->get_bone_parent(root_bone.bone);
	if (parent >= 0) {
		parent_gpose = p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion();
	}

	root_joint_solver_info->current_lrest = p_skeleton->get_bone_pose(root_bone.bone).basis.get_rotation_quaternion();
	root_joint_solver_info->current_grest = parent_gpose * root_joint_solver_info->current_lrest;
	root_joint_solver_info->current_grest.normalize();

	Vector3 from = root_joint_solver_info->forward_vector;
	Vector3 to = root_joint_solver_info->current_grest.xform_inv(root_joint_solver_info->current_vector).normalized();
	root_joint_solver_info->current_lpose = root_joint_solver_info->current_lrest * get_swing(Quaternion(from, to), from);

	root_joint_solver_info->current_gpose = parent_gpose * root_joint_solver_info->current_lpose;
	root_joint_solver_info->current_gpose.normalize();
	Quaternion root_gpose = root_joint_solver_info->current_gpose;

	mid_joint_solver_info->current_lrest = p_skeleton->get_bone_global_pose(root_bone.bone).basis.get_rotation_quaternion().inverse() *
			p_skeleton->get_bone_global_pose(middle_bone.bone).basis.get_rotation_quaternion();
	mid_joint_solver_info->current_grest = root_gpose * mid_joint_solver_info->current_lrest;
	mid_joint_solver_info->current_grest.normalize();

	from = mid_joint_solver_info->forward_vector;
	to = mid_joint_solver_info->current_grest.xform_inv(mid_joint_solver_info->current_vector).normalized();
	mid_joint_solver_info->current_lpose = mid_joint_solver_info->current_lrest * get_swing(Quaternion(from, to), from);

	mid_joint_solver_info->current_gpose = root_gpose * mid_joint_solver_info->current_lpose;
	mid_joint_solver_info->current_gpose.normalize();

	bool is_pole_defined = pole_direction != SECONDARY_DIRECTION_NONE &&
			(pole_direction != SECONDARY_DIRECTION_CUSTOM || !pole_direction_vector.is_zero_approx());
	if (is_pole_defined) {
		Vector3 pole_dir = get_projected_normal(root_pos, end_pos, p_pole_destination);
		if (pole_dir.is_zero_approx()) {
			return;
		}
		Vector3 a = mid_joint_solver_info->current_vector.normalized();
		Vector3 k = mid_joint_solver_info->current_gpose.xform(get_pole_direction_vector()).normalized();
		Vector3 n = pole_dir.cross((mid_pos - root_pos).normalized()).normalized();

		if (a.is_zero_approx() || k.is_zero_approx() || n.is_zero_approx() || Math::is_zero_approx(n.dot(k))) {
			return;
		}
		double c0 = n.dot(k - a * k.dot(a));
		double c1 = n.dot(a.cross(k));
		double c2 = n.dot(a) * k.dot(a);
		double r = Math::sqrt(c0 * c0 + c1 * c1);
		double cos_arg = CLAMP(-c2 / r, -1.0, 1.0);
		double phi = Math::atan2(c1, c0);
		double acosv = Math::acos(cos_arg);

		double t1 = phi + acosv;
		double t2 = phi - acosv;
		Quaternion q1(a, t1);
		Quaternion q2(a, t2);

		Vector3 pole_proj = snap_vector_to_plane(n, pole_dir).normalized();
		Vector3 k1p = snap_vector_to_plane(n, q1.xform(k)).normalized();
		Vector3 k2p = snap_vector_to_plane(n, q2.xform(k)).normalized();
		double s1 = pole_proj.is_zero_approx() ? Math::abs(t1) : k1p.dot(pole_proj);
		double s2 = pole_proj.is_zero_approx() ? Math::abs(t2) : k2p.dot(pole_proj);

		double t = s1 >= s2 ? t1 : t2;
		Quaternion root_roll_rot(root_joint_solver_info->forward_vector, t);
		Quaternion mid_roll_rot(mid_joint_solver_info->forward_vector, t);

		root_joint_solver_info->current_lpose = root_joint_solver_info->current_lpose * root_roll_rot;
		root_joint_solver_info->current_gpose = parent_gpose * root_joint_solver_info->current_lpose;
		root_joint_solver_info->current_gpose.normalize();
		root_gpose = root_joint_solver_info->current_gpose;

		mid_joint_solver_info->current_lpose = root_roll_rot.inverse() * mid_joint_solver_info->current_lpose * mid_roll_rot;
		mid_joint_solver_info->current_gpose = root_gpose * mid_joint_solver_info->current_lpose;
		mid_joint_solver_info->current_gpose.normalize();
	}
}

// --- IKModifier3D ---

IKModifier3D::IKModifier3D(SolverType p_solver_type) :
		solver_type(p_solver_type) {
}

IKModifier3D::~IKModifier3D() {
	clear_settings();
}

void IKModifier3D::set_solver_type(SolverType p_solver_type) {
	if (solver_type == p_solver_type) {
		return;
	}
	solver_type = p_solver_type;
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_simulation_dirty(i);
	}
	update_configuration_warnings();
}

#ifdef TOOLS_ENABLED
void IKModifier3D::_make_gizmo_dirty() {
	gizmo_dirty = true;
}

void IKModifier3D::_update_mutable_info() {}
#endif // TOOLS_ENABLED

void IKModifier3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (Engine::is_editor_hint()) {
				set_notify_local_transform(true);
			}
			_update_mutable_info();
			_make_gizmo_dirty();
#endif // TOOLS_ENABLED
			_make_all_joints_dirty();
		} break;
#ifdef TOOLS_ENABLED
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			_make_gizmo_dirty();
		} break;
#endif // TOOLS_ENABLED
	}
}

PackedStringArray IKModifier3D::get_configuration_warnings() const {
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (solver_type == SOLVER_SPLINE) {
		for (uint32_t i = 0; i < settings.size(); i++) {
			if (settings[i]->path_3d.is_empty()) {
				warnings.push_back(RTR("Detecting settings with no Path3D set! Spline IK must have a Path3D to work."));
				break;
			}
		}
	} else {
		for (uint32_t i = 0; i < settings.size(); i++) {
			if (settings[i]->target_node.is_empty()) {
				warnings.push_back(RTR("Detecting settings with no target set! IKModifier3D must have a target to work."));
				break;
			}
		}
		if (solver_type == SOLVER_TWO_BONE) {
			for (uint32_t i = 0; i < settings.size(); i++) {
				if (settings[i]->pole_node.is_empty()) {
					warnings.push_back(RTR("Detecting settings with no pole target set! Two-Bone IK must have a pole target to work."));
					break;
				}
			}
		}
	}
	return warnings;
}

int IKModifier3D::get_setting_count() const {
	return settings.size();
}

void IKModifier3D::set_setting_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	int delta = p_count - (int)settings.size();
	if (delta < 0) {
		for (int i = delta; i < 0; i++) {
			memdelete(settings[settings.size() + i]);
			settings[settings.size() + i] = nullptr;
		}
	}
	settings.resize(p_count);
	if (delta > 0) {
		for (int i = 1; i <= delta; i++) {
			settings[p_count - i] = memnew(ChainSetting);
		}
	}
}

void IKModifier3D::clear_settings() {
	set_setting_count(0);
}

void IKModifier3D::set_mutable_bone_axes(bool p_enabled) {
	mutable_bone_axes = p_enabled;
	for (uint32_t i = 0; i < settings.size(); i++) {
		_make_simulation_dirty(i);
	}
#ifdef TOOLS_ENABLED
	_update_mutable_info();
#endif // TOOLS_ENABLED
}

bool IKModifier3D::are_bone_axes_mutable() const {
	return mutable_bone_axes;
}

void IKModifier3D::set_max_iterations(int p_max_iterations) {
	max_iterations = p_max_iterations;
}

int IKModifier3D::get_max_iterations() const {
	return max_iterations;
}

void IKModifier3D::set_min_distance(double p_min_distance) {
	min_distance = p_min_distance;
	min_distance_squared = min_distance * min_distance;
}

double IKModifier3D::get_min_distance() const {
	return min_distance;
}

void IKModifier3D::set_angular_delta_limit(double p_angular_delta_limit) {
	angular_delta_limit = p_angular_delta_limit;
}

double IKModifier3D::get_angular_delta_limit() const {
	return angular_delta_limit;
}

void IKModifier3D::set_deterministic(bool p_deterministic) {
	deterministic = p_deterministic;
}

bool IKModifier3D::is_deterministic() const {
	return deterministic;
}

// Bone getters & setters.

void IKModifier3D::set_root_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->root_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_root_bone(p_index, sk->find_bone(p_bone_name));
	}
}

String IKModifier3D::get_root_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->root_bone.name;
}

void IKModifier3D::set_root_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->root_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index]->root_bone.name = sk->get_bone_name(p_bone);
	}
	_update_joints(p_index);
}

int IKModifier3D::get_root_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->root_bone.bone;
}

void IKModifier3D::set_middle_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->middle_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_middle_bone(p_index, sk->find_bone(p_bone_name));
	}
}

String IKModifier3D::get_middle_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->middle_bone.name;
}

void IKModifier3D::set_middle_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->middle_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index]->middle_bone.name = sk->get_bone_name(p_bone);
	}
	_make_simulation_dirty(p_index);
}

int IKModifier3D::get_middle_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->middle_bone.bone;
}

void IKModifier3D::set_end_bone_name(int p_index, const String &p_bone_name) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone.name = p_bone_name;
	Skeleton3D *sk = get_skeleton();
	if (sk) {
		set_end_bone(p_index, sk->find_bone(p_bone_name));
	}
}

String IKModifier3D::get_end_bone_name(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	return settings[p_index]->end_bone.name;
}

void IKModifier3D::set_end_bone(int p_index, int p_bone) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone.bone = p_bone;
	Skeleton3D *sk = get_skeleton();
	if (sk && p_bone >= 0 && p_bone < sk->get_bone_count()) {
		settings[p_index]->end_bone.name = sk->get_bone_name(p_bone);
	}
	_update_joints(p_index);
}

int IKModifier3D::get_end_bone(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	if (solver_type == SOLVER_TWO_BONE) {
		return settings[p_index]->get_two_bone_end_bone();
	}
	return settings[p_index]->end_bone.bone;
}

void IKModifier3D::set_use_virtual_end(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->use_virtual_end = p_enabled;
	_make_simulation_dirty(p_index);
}

bool IKModifier3D::is_using_virtual_end(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->use_virtual_end;
}

void IKModifier3D::set_extend_end_bone(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->extend_end_bone = p_enabled;
	_make_simulation_dirty(p_index);
}

bool IKModifier3D::is_end_bone_extended(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->extend_end_bone;
}

void IKModifier3D::set_end_bone_direction(int p_index, BoneDirection p_direction) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->end_bone_direction = p_direction;
	_make_simulation_dirty(p_index);
}

SkeletonModifier3D::BoneDirection IKModifier3D::get_end_bone_direction(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), BONE_DIRECTION_FROM_PARENT);
	return settings[p_index]->end_bone_direction;
}

void IKModifier3D::set_end_bone_length(int p_index, float p_length) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	float old = settings[p_index]->end_bone_length;
	settings[p_index]->end_bone_length = p_length;
#ifdef TOOLS_ENABLED
	_make_gizmo_dirty();
#endif // TOOLS_ENABLED
	if (mutable_bone_axes && Math::is_zero_approx(old) == Math::is_zero_approx(p_length)) {
		return;
	}
	_make_simulation_dirty(p_index);
}

float IKModifier3D::get_end_bone_length(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0.0f);
	return settings[p_index]->end_bone_length;
}

void IKModifier3D::set_target_node(int p_index, const NodePath &p_target_node) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->target_node = p_target_node;
	update_configuration_warnings();
}

NodePath IKModifier3D::get_target_node(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->target_node;
}

void IKModifier3D::set_pole_node(int p_index, const NodePath &p_pole_node) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->pole_node = p_pole_node;
	update_configuration_warnings();
}

NodePath IKModifier3D::get_pole_node(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->pole_node;
}

void IKModifier3D::set_pole_direction(int p_index, SecondaryDirection p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->pole_direction = p_axis;
}

SkeletonModifier3D::SecondaryDirection IKModifier3D::get_pole_direction(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), SECONDARY_DIRECTION_NONE);
	return settings[p_index]->pole_direction;
}

void IKModifier3D::set_pole_direction_vector(int p_index, const Vector3 &p_vector) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->pole_direction_vector = p_vector;
}

Vector3 IKModifier3D::get_pole_direction_vector(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	return settings[p_index]->get_pole_direction_vector();
}

bool IKModifier3D::is_valid(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	if (solver_type == SOLVER_TWO_BONE) {
		return settings[p_index]->root_bone.bone != -1 &&
				settings[p_index]->middle_bone.bone != -1 &&
				settings[p_index]->is_two_bone_end_valid();
	}
	return settings[p_index]->root_bone.bone != -1 && settings[p_index]->end_bone.bone != -1;
}

void IKModifier3D::set_path_3d(int p_index, const NodePath &p_path_3d) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->path_3d = p_path_3d;
	update_configuration_warnings();
}

NodePath IKModifier3D::get_path_3d(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), NodePath());
	return settings[p_index]->path_3d;
}

void IKModifier3D::set_tilt_enabled(int p_index, bool p_enabled) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->tilt_enabled = p_enabled;
}

bool IKModifier3D::is_tilt_enabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), false);
	return settings[p_index]->tilt_enabled;
}

void IKModifier3D::set_tilt_fade_in(int p_index, int p_size) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->tilt_fade_in = p_size;
}

int IKModifier3D::get_tilt_fade_in(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->tilt_fade_in;
}

void IKModifier3D::set_tilt_fade_out(int p_index, int p_size) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	settings[p_index]->tilt_fade_out = p_size;
}

int IKModifier3D::get_tilt_fade_out(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	return settings[p_index]->tilt_fade_out;
}

// Individual joints.

String IKModifier3D::get_joint_bone_name(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), String());
	const LocalVector<BoneJoint> &joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), String());
	return joints[p_joint].name;
}

int IKModifier3D::get_joint_bone(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), -1);
	const LocalVector<BoneJoint> &joints = settings[p_index]->joints;
	ERR_FAIL_INDEX_V(p_joint, (int)joints.size(), -1);
	return joints[p_joint].bone;
}

void IKModifier3D::set_joint_count(int p_index, int p_count) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_COND(p_count < 0);
	settings[p_index]->joints.resize(p_count);
	settings[p_index]->joint_settings.resize(p_count);
	settings[p_index]->twists.resize(p_count);
	settings[p_index]->chain_length_accum.resize(p_count);
	_make_simulation_dirty(p_index);
}

int IKModifier3D::get_joint_count(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), 0);
	return settings[p_index]->joints.size();
}

void IKModifier3D::set_joint_rotation_axis(int p_index, int p_joint, RotationAxis p_axis) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].rotation_axis = p_axis;
	_make_simulation_dirty(p_index);
}

SkeletonModifier3D::RotationAxis IKModifier3D::get_joint_rotation_axis(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), ROTATION_AXIS_ALL);
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), ROTATION_AXIS_ALL);
	return js[p_joint].rotation_axis;
}

void IKModifier3D::set_joint_rotation_axis_vector(int p_index, int p_joint, const Vector3 &p_vector) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].rotation_axis_vector = p_vector;
	_make_simulation_dirty(p_index);
}

Vector3 IKModifier3D::get_joint_rotation_axis_vector(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Vector3());
	return js[p_joint].get_rotation_axis_vector();
}

void IKModifier3D::set_joint_limitation(int p_index, int p_joint, const Ref<JointLimitation3D> &p_limitation) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation = p_limitation;
	_make_simulation_dirty(p_index);
}

Ref<JointLimitation3D> IKModifier3D::get_joint_limitation(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Ref<JointLimitation3D>());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Ref<JointLimitation3D>());
	return js[p_joint].limitation;
}

void IKModifier3D::set_joint_limitation_right_axis(int p_index, int p_joint, SecondaryDirection p_direction) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation_right_axis = p_direction;
}

SkeletonModifier3D::SecondaryDirection IKModifier3D::get_joint_limitation_right_axis(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), SECONDARY_DIRECTION_NONE);
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), SECONDARY_DIRECTION_NONE);
	return js[p_joint].limitation_right_axis;
}

void IKModifier3D::set_joint_limitation_right_axis_vector(int p_index, int p_joint, const Vector3 &p_vector) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation_right_axis_vector = p_vector;
}

Vector3 IKModifier3D::get_joint_limitation_right_axis_vector(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Vector3());
	return js[p_joint].get_limitation_right_axis_vector();
}

void IKModifier3D::set_joint_limitation_rotation_offset(int p_index, int p_joint, const Quaternion &p_offset) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ERR_FAIL_INDEX(p_joint, (int)settings[p_index]->joint_settings.size());
	settings[p_index]->joint_settings[p_joint].limitation_rotation_offset = p_offset;
}

Quaternion IKModifier3D::get_joint_limitation_rotation_offset(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Quaternion());
	return js[p_joint].limitation_rotation_offset;
}

Quaternion IKModifier3D::get_joint_limitation_space(int p_index, int p_joint, const Vector3 &p_forward) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Quaternion());
	const LocalVector<JointSetting> &js = settings[p_index]->joint_settings;
	ERR_FAIL_INDEX_V(p_joint, (int)js.size(), Quaternion());
	return js[p_joint].get_limitation_space(p_forward);
}

Quaternion IKModifier3D::get_local_pose_rotation(Skeleton3D *p_skeleton, int p_bone, const Quaternion &p_global_pose_rotation) {
	int parent = p_skeleton->get_bone_parent(p_bone);
	if (parent < 0) {
		return p_global_pose_rotation;
	}
	return p_skeleton->get_bone_global_pose(parent).basis.get_rotation_quaternion().inverse() * p_global_pose_rotation;
}

Vector3 IKModifier3D::get_bone_axis(Skeleton3D *p_skeleton, int p_end_bone, BoneDirection p_direction, bool p_mutable_bone_axes) {
	if (!p_skeleton || !p_skeleton->is_inside_tree()) {
		return Vector3();
	}
	Vector3 axis;
	if (p_direction == BONE_DIRECTION_FROM_PARENT) {
		axis = p_skeleton->get_bone_rest(p_end_bone).basis.xform_inv(p_mutable_bone_axes ? p_skeleton->get_bone_pose(p_end_bone).origin : p_skeleton->get_bone_rest(p_end_bone).origin);
		axis.normalize();
	} else {
		axis = get_vector_from_bone_axis(static_cast<BoneAxis>((int)p_direction));
	}
	return axis;
}

#ifdef TOOLS_ENABLED
Transform3D IKModifier3D::get_bone_global_rest_mutable(Skeleton3D *p_skeleton, int p_bone) {
	int current = p_bone;
	Transform3D accum;
	int parent = p_skeleton->get_bone_parent(current);
	if (parent >= 0) {
		accum = p_skeleton->get_bone_global_rest(parent);
	}
	Transform3D tr = p_skeleton->get_bone_rest(current);
	tr.origin = p_skeleton->get_bone_pose_position(current);
	accum *= tr;
	return accum;
}

Transform3D IKModifier3D::get_chain_root_global_rest(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Transform3D());
	return settings[p_index]->root_global_rest;
}

Vector3 IKModifier3D::get_bone_vector(int p_index, int p_joint) const {
	ERR_FAIL_INDEX_V(p_index, (int)settings.size(), Vector3());
	const LocalVector<IKModifier3DSolverInfo *> &infos = settings[p_index]->solver_info_list;
	ERR_FAIL_INDEX_V(p_joint, (int)infos.size(), Vector3());
	if (!infos[p_joint]) {
		return Vector3();
	}
	return infos[p_joint]->forward_vector * infos[p_joint]->length;
}
#endif // TOOLS_ENABLED

// Internal lifecycle & validation.

void IKModifier3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	_make_all_joints_dirty();
}

void IKModifier3D::_validate_bone_names() {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}
	for (uint32_t i = 0; i < settings.size(); i++) {
		ChainSetting *s = settings[i];
		if (!s->root_bone.name.is_empty()) {
			s->root_bone.bone = sk->find_bone(s->root_bone.name);
		}
		if (!s->middle_bone.name.is_empty()) {
			s->middle_bone.bone = sk->find_bone(s->middle_bone.name);
		}
		if (!s->end_bone.name.is_empty()) {
			s->end_bone.bone = sk->find_bone(s->end_bone.name);
		}
		_update_joints(i);
	}
}

void IKModifier3D::_make_all_joints_dirty() {
	for (uint32_t i = 0; i < settings.size(); i++) {
		_update_joints(i);
	}
}

void IKModifier3D::_update_joints(int p_index) {
	ERR_FAIL_INDEX(p_index, (int)settings.size());
	ChainSetting *s = settings[p_index];
	Skeleton3D *sk = get_skeleton();
	if (!sk || s->root_bone.bone < 0 || s->end_bone.bone < 0) {
		_make_simulation_dirty(p_index);
		return;
	}
	if (solver_type != SOLVER_TWO_BONE) {
		LocalVector<int> chain_bones;
		int cur = s->end_bone.bone;
		while (cur >= 0) {
			chain_bones.push_back(cur);
			if (cur == s->root_bone.bone) {
				break;
			}
			cur = sk->get_bone_parent(cur);
		}
		if (chain_bones.is_empty() || chain_bones[chain_bones.size() - 1] != s->root_bone.bone) {
			set_joint_count(p_index, 0);
			return;
		}
		chain_bones.reverse();
		set_joint_count(p_index, chain_bones.size());
		for (uint32_t j = 0; j < chain_bones.size(); j++) {
			s->joints[j].bone = chain_bones[j];
			s->joints[j].name = sk->get_bone_name(chain_bones[j]);
		}
	}
	_make_simulation_dirty(p_index);
}

void IKModifier3D::_clear_joints(int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}
	setting->clear_solver_info();
	setting->root_pos = Vector3();
	setting->mid_pos = Vector3();
	setting->end_pos = Vector3();
	if (solver_type != SOLVER_TWO_BONE) {
		setting->solver_info_list.resize_initialized(setting->joints.size());
	}
}

void IKModifier3D::_make_simulation_dirty(int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}
	setting->simulation_dirty = true;
#ifdef TOOLS_ENABLED
	if (!mutable_bone_axes) {
		_make_gizmo_dirty();
	}
#endif // TOOLS_ENABLED
}

void IKModifier3D::_validate_axes(Skeleton3D *p_skeleton) const {
	for (uint32_t i = 0; i < settings.size(); i++) {
		for (uint32_t j = 0; j < settings[i]->joints.size(); j++) {
			_validate_axis(p_skeleton, i, j);
		}
	}
}

void IKModifier3D::_validate_axis(Skeleton3D *p_skeleton, int p_index, int p_joint) const {
	RotationAxis axis = settings[p_index]->joint_settings[p_joint].rotation_axis;
	if (axis == ROTATION_AXIS_ALL) {
		return;
	}
	Vector3 rot = get_joint_rotation_axis_vector(p_index, p_joint).normalized();
	Vector3 fwd;
	if (p_joint < (int)settings[p_index]->joints.size() - 1) {
		fwd = p_skeleton->get_bone_rest(settings[p_index]->joints[p_joint + 1].bone).origin;
	} else if (settings[p_index]->extend_end_bone) {
		fwd = IKModifier3D::get_bone_axis(p_skeleton, settings[p_index]->end_bone.bone, settings[p_index]->end_bone_direction, mutable_bone_axes);
		if (fwd.is_zero_approx()) {
			return;
		}
	}
	fwd.normalize();
	if (Math::is_equal_approx(Math::abs(rot.dot(fwd)), 1)) {
		WARN_PRINT_ED("Setting: " + itos(p_index) + " Joint: " + itos(p_joint) + ": Rotation axis and forward vector are colinear.");
	}
}

void IKModifier3D::_validate_pole_directions(Skeleton3D *p_skeleton) const {
	for (uint32_t i = 0; i < settings.size(); i++) {
		_validate_pole_direction(p_skeleton, i);
	}
}

void IKModifier3D::_validate_pole_direction(Skeleton3D *p_skeleton, int p_index) const {
	ChainSetting *setting = settings[p_index];
	SecondaryDirection dir = setting->pole_direction;
	if (!is_valid(p_index) || dir == SECONDARY_DIRECTION_NONE) {
		return;
	}
	Vector3 kv = get_pole_direction_vector(p_index).normalized();
	int valid_end_bone = setting->get_two_bone_end_bone();
	Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, valid_end_bone, setting->end_bone_direction, mutable_bone_axes);
	Vector3 global_rest_origin;
	if (setting->extend_end_bone && setting->end_bone_length > 0 && !axis.is_zero_approx()) {
		global_rest_origin = p_skeleton->get_bone_global_rest(valid_end_bone).xform(axis * setting->end_bone_length);
	} else {
		global_rest_origin = p_skeleton->get_bone_global_rest(valid_end_bone).origin;
	}
	axis = global_rest_origin - p_skeleton->get_bone_global_rest(setting->middle_bone.bone).origin;
	if (axis.is_zero_approx()) {
		return;
	}
	Vector3 fwd = p_skeleton->get_bone_global_rest(setting->middle_bone.bone).basis.get_rotation_quaternion().xform_inv(axis).normalized();
	if (Math::is_equal_approx(Math::abs(kv.dot(fwd)), 1)) {
		WARN_PRINT_ED("Setting: " + itos(p_index) + ": Pole direction and forward vector are colinear.");
	}
}

Transform3D IKModifier3D::_get_two_bone_global_rest(Skeleton3D *p_skeleton, int p_bone, int p_root) const {
	if (!mutable_bone_axes) {
		return p_skeleton->get_bone_global_rest(p_bone);
	}
	Transform3D tr = p_skeleton->get_bone_global_rest(p_root);
	tr.origin = tr.origin - p_skeleton->get_bone_rest(p_root).origin + p_skeleton->get_bone_pose(p_root).origin;
	LocalVector<int> path;
	for (int bone = p_bone; bone != p_root && bone != -1; bone = p_skeleton->get_bone_parent(bone)) {
		path.push_back(bone);
	}
	path.reverse();
	for (int bone : path) {
		tr = tr * Transform3D(p_skeleton->get_bone_rest(bone).basis, p_skeleton->get_bone_pose(bone).origin);
	}
	return tr;
}

void IKModifier3D::_init_joints(Skeleton3D *p_skeleton, int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}
	cached_space = p_skeleton->get_global_transform_interpolated();

	if (solver_type == SOLVER_TWO_BONE) {
		if (!setting->simulation_dirty) {
			if (mutable_bone_axes) {
				_update_bone_axis(p_skeleton, p_index);
			}
			return;
		}
		_clear_joints(p_index);
		if (setting->root_bone.bone == -1 || setting->middle_bone.bone == -1 || !setting->is_two_bone_end_valid()) {
			return;
		}
		_update_bone_axis(p_skeleton, p_index);
		setting->simulation_dirty = false;
		return;
	}

	if (solver_type == SOLVER_SPLINE) {
		if (!setting->simulation_dirty) {
			if (mutable_bone_axes) {
				_update_bone_axis(p_skeleton, p_index);
			}
			return;
		}
		_clear_joints(p_index);
		setting->chain.clear();
		bool extends_end = setting->extend_end_bone && setting->end_bone_length > 0;
		double accum = 0.0;
		for (uint32_t i = 0; i < setting->joints.size(); i++) {
			setting->chain.push_back(p_skeleton->get_bone_global_pose(setting->joints[i].bone).origin);
			bool last = i == setting->joints.size() - 1;
			if (last && extends_end) {
				Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, setting->end_bone.bone, setting->end_bone_direction, mutable_bone_axes);
				if (axis.is_zero_approx()) {
					setting->chain_length_accum[i] = accum;
					continue;
				}
				setting->solver_info_list[i] = memnew(IKModifier3DSolverInfo);
				setting->solver_info_list[i]->forward_vector = axis.normalized();
				setting->solver_info_list[i]->length = setting->end_bone_length;
				setting->chain.push_back(p_skeleton->get_bone_global_pose(setting->joints[i].bone).xform(axis * setting->end_bone_length));
			} else if (!last) {
				Vector3 axis = p_skeleton->get_bone_rest(setting->joints[i + 1].bone).origin;
				if (axis.is_zero_approx()) {
					setting->chain_length_accum[i] = accum;
					continue;
				}
				setting->solver_info_list[i] = memnew(IKModifier3DSolverInfo);
				setting->solver_info_list[i]->forward_vector = axis.normalized();
				setting->solver_info_list[i]->length = axis.length();
			}
			if (setting->solver_info_list[i]) {
				accum += setting->solver_info_list[i]->length;
			}
			setting->chain_length_accum[i] = accum;
		}
		if (mutable_bone_axes) {
			_update_bone_axis(p_skeleton, p_index);
#ifdef TOOLS_ENABLED
		} else {
			_make_gizmo_dirty();
#endif // TOOLS_ENABLED
		}
		setting->init_current_joint_rotations(p_skeleton);
		setting->simulation_dirty = false;
		return;
	}

	// Iterative solvers (CCD, FABRIK, Jacobian).
	if (setting->simulation_dirty) {
		_clear_joints(p_index);
		setting->init_iterate_joints(p_skeleton, mutable_bone_axes);
		setting->simulation_dirty = false;
	} else if (deterministic) {
		setting->init_iterate_joints(p_skeleton, mutable_bone_axes);
	}

	if (mutable_bone_axes) {
#ifdef TOOLS_ENABLED
		_update_mutable_info();
#endif // TOOLS_ENABLED
		_update_bone_axis(p_skeleton, p_index);
	}
	setting->simulated = false;
}

void IKModifier3D::_update_bone_axis(Skeleton3D *p_skeleton, int p_index) {
	ChainSetting *setting = settings[p_index];
	if (!setting) {
		return;
	}

	if (solver_type == SOLVER_TWO_BONE) {
		if (!setting->mid_joint_solver_info) {
			setting->mid_joint_solver_info = memnew(IKModifier3DSolverInfo);
		}
		if (!setting->root_joint_solver_info) {
			setting->root_joint_solver_info = memnew(IKModifier3DSolverInfo);
		}
		int valid_end_bone = setting->get_two_bone_end_bone();
		Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, valid_end_bone, setting->end_bone_direction, mutable_bone_axes);
		Vector3 global_rest_origin;
		if (setting->extend_end_bone && setting->end_bone_length > 0 && !axis.is_zero_approx()) {
			setting->end_pos = p_skeleton->get_bone_global_pose(valid_end_bone).xform(axis * setting->end_bone_length);
			global_rest_origin = _get_two_bone_global_rest(p_skeleton, valid_end_bone, setting->root_bone.bone).xform(axis * setting->end_bone_length);
		} else {
			setting->end_pos = p_skeleton->get_bone_global_pose(valid_end_bone).origin;
			global_rest_origin = _get_two_bone_global_rest(p_skeleton, valid_end_bone, setting->root_bone.bone).origin;
		}

		Vector3 orig = _get_two_bone_global_rest(p_skeleton, setting->middle_bone.bone, setting->root_bone.bone).origin;
		axis = global_rest_origin - orig;
		global_rest_origin = orig;
		if (!axis.is_zero_approx()) {
			setting->mid_pos = p_skeleton->get_bone_global_pose(setting->middle_bone.bone).origin;
			setting->mid_joint_solver_info->forward_vector = p_skeleton->get_bone_global_rest(setting->middle_bone.bone).basis.get_rotation_quaternion().xform_inv(axis).normalized();
			setting->mid_joint_solver_info->length = axis.length();
		} else {
			_clear_joints(p_index);
			return;
		}

		orig = _get_two_bone_global_rest(p_skeleton, setting->root_bone.bone, setting->root_bone.bone).origin;
		axis = global_rest_origin - orig;
		if (!axis.is_zero_approx()) {
			setting->root_pos = p_skeleton->get_bone_global_pose(setting->root_bone.bone).origin;
			setting->root_joint_solver_info->forward_vector = p_skeleton->get_bone_global_rest(setting->root_bone.bone).basis.get_rotation_quaternion().xform_inv(axis).normalized();
			setting->root_joint_solver_info->length = axis.length();
		} else {
			_clear_joints(p_index);
			return;
		}

		setting->init_two_bone_joint_rotations(p_skeleton);
		double total_length = setting->root_joint_solver_info->length + setting->mid_joint_solver_info->length;
		setting->cached_length_sq = total_length * total_length;
		return;
	}

	const LocalVector<BoneJoint> &joints = setting->joints;
	const LocalVector<IKModifier3DSolverInfo *> &solver_info_list = setting->solver_info_list;
	int len = (int)solver_info_list.size() - 1;
	bool use_rot_axis = is_iterative_solver();

	for (int j = 0; j < len; j++) {
		if (!solver_info_list[j]) {
			continue;
		}
		Vector3 axis = p_skeleton->get_bone_pose(joints[j + 1].bone).origin;
		if (axis.is_zero_approx()) {
			continue;
		}
		if (use_rot_axis) {
			solver_info_list[j]->forward_vector = snap_vector_to_plane(setting->joint_settings[j].get_rotation_axis_vector(), axis.normalized());
		} else {
			solver_info_list[j]->forward_vector = axis.normalized();
		}
		solver_info_list[j]->length = axis.length();
	}
	if (setting->extend_end_bone && len >= 0 && solver_info_list[len]) {
		Vector3 axis = IKModifier3D::get_bone_axis(p_skeleton, setting->end_bone.bone, setting->end_bone_direction, mutable_bone_axes);
		if (!axis.is_zero_approx()) {
			if (use_rot_axis) {
				solver_info_list[len]->forward_vector = snap_vector_to_plane(setting->joint_settings[len].get_rotation_axis_vector(), axis.normalized());
			} else {
				solver_info_list[len]->forward_vector = axis.normalized();
			}
			solver_info_list[len]->length = setting->end_bone_length;
		}
	}
}

void IKModifier3D::_process_modification(double p_delta) {
	Skeleton3D *sk = get_skeleton();
	if (!sk) {
		return;
	}
	_process_ik(sk, p_delta);
}

void IKModifier3D::_process_ik(Skeleton3D *p_skeleton, double p_delta) {
	Transform3D skel_inv = p_skeleton->get_global_transform_interpolated().affine_inverse();
	for (uint32_t i = 0; i < settings.size(); i++) {
		ChainSetting *setting = settings[i];
		if (!setting) {
			continue;
		}
		_init_joints(p_skeleton, i);

		if (solver_type == SOLVER_TWO_BONE) {
			if (!setting->is_two_bone_valid()) {
				continue;
			}
			Node3D *target = dynamic_cast<Node3D *>(get_node_or_null(setting->target_node));
			if (!target) {
				continue;
			}
			Vector3 dest = skel_inv.xform(target->get_global_transform_interpolated().origin);
			Vector3 pole_dest;
			Node3D *pole = dynamic_cast<Node3D *>(get_node_or_null(setting->pole_node));
			if (pole) {
				pole_dest = skel_inv.xform(pole->get_global_transform_interpolated().origin);
			}
			_process_two_bone_joints(p_delta, p_skeleton, setting, dest, pole_dest);
		} else if (solver_type == SOLVER_SPLINE) {
			Path3D *path = dynamic_cast<Path3D *>(get_node_or_null(setting->path_3d));
			if (!path || path->get_curve().is_null() || path->get_curve()->get_point_count() == 0) {
				continue;
			}
			Transform3D curve_space = skel_inv * path->get_global_transform_interpolated();
			_process_spline_joints(p_delta, p_skeleton, setting, path->get_curve(), curve_space);
		} else {
			if (setting->chain.is_empty()) {
				continue;
			}
			Node3D *target = dynamic_cast<Node3D *>(get_node_or_null(setting->target_node));
			if (!target) {
				continue;
			}
			Vector3 dest = skel_inv.xform(target->get_global_transform_interpolated().origin);
			_process_iterate_joints(p_delta, p_skeleton, setting, dest);
		}
	}
}

void IKModifier3D::_process_iterate_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	double distance_to_target_sq = INFINITY;
	int iteration_count = 0;

	if (p_setting->simulated) {
		distance_to_target_sq = p_setting->chain[p_setting->chain.size() - 1].distance_squared_to(p_destination);
	}

	while (distance_to_target_sq > min_distance_squared && iteration_count < max_iterations) {
		switch (solver_type) {
			case SOLVER_CCD:
				_solve_ccd_iteration(p_skeleton, p_setting, p_destination);
				break;
			case SOLVER_FABRIK:
				_solve_fabrik_iteration(p_skeleton, p_setting, p_destination);
				break;
			case SOLVER_JACOBIAN:
				_solve_jacobian_iteration(p_skeleton, p_setting, p_destination);
				break;
			default:
				break;
		}
		p_setting->cache_iterate_joint_rotations(p_skeleton, angular_delta_limit);
		distance_to_target_sq = p_setting->chain[p_setting->chain.size() - 1].distance_squared_to(p_destination);
		iteration_count++;
	}

	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		p_skeleton->set_bone_pose_rotation(p_setting->joints[i].bone, solver_info->current_lpose);
	}

	p_setting->simulated = true;
}

void IKModifier3D::_solve_ccd_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	int joint_size = (int)p_setting->joints.size();
	int chain_size = (int)p_setting->chain.size();

	for (int ancestor = joint_size - 1; ancestor >= 0; ancestor--) {
		for (int i = ancestor; i < joint_size; i++) {
			IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
			if (!solver_info || Math::is_zero_approx(solver_info->length)) {
				continue;
			}
			int HEAD = i;
			int TAIL = i + 1;

			Vector3 current_head = p_setting->chain[HEAD];
			Vector3 current_effector = p_setting->chain[chain_size - 1];
			Vector3 head_to_effector = current_effector - current_head;
			Vector3 head_to_destination = p_destination - current_head;

			if (Math::is_zero_approx(head_to_destination.length_squared() * head_to_effector.length_squared())) {
				continue;
			}

			Quaternion to_rot = Quaternion(head_to_effector.normalized(), head_to_destination.normalized());
			Vector3 to_tail = p_setting->chain[TAIL] - current_head;

			p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, current_head + to_rot.xform(to_tail));

			if (p_setting->joint_settings[HEAD].rotation_axis != ROTATION_AXIS_ALL) {
				p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_projected_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD]));
			}
			if (p_setting->joint_settings[HEAD].limitation.is_valid()) {
				p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_limited_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD], solver_info->forward_vector));
			}
		}
	}
}

void IKModifier3D::_solve_fabrik_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	int joint_size = (int)p_setting->joints.size();

	// Backwards.
	bool first = true;
	for (int i = joint_size - 1; i >= 0; i--) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		int HEAD = i;
		int TAIL = i + 1;

		if (first) {
			p_setting->update_chain_coordinate_bw(p_skeleton, TAIL, p_destination);
			first = false;
		}

		p_setting->update_chain_coordinate_bw(p_skeleton, HEAD, limit_length(p_setting->chain[TAIL], p_setting->chain[HEAD], solver_info->length));

		if (p_setting->joint_settings[HEAD].rotation_axis != ROTATION_AXIS_ALL) {
			p_setting->update_chain_coordinate_bw(p_skeleton, HEAD, p_setting->chain[TAIL] + p_setting->joint_settings[HEAD].get_projected_rotation(solver_info->current_grest, p_setting->chain[HEAD] - p_setting->chain[TAIL]));
		}
		if (p_setting->joint_settings[HEAD].limitation.is_valid()) {
			p_setting->update_chain_coordinate_bw(p_skeleton, HEAD, p_setting->chain[TAIL] + p_setting->joint_settings[HEAD].get_limited_rotation(solver_info->current_grest, p_setting->chain[HEAD] - p_setting->chain[TAIL], solver_info->forward_vector));
		}
	}

	// Forwards.
	first = true;
	for (int i = 0; i < joint_size; i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		int HEAD = i;
		int TAIL = i + 1;

		if (first) {
			p_setting->update_chain_coordinate_fw(p_skeleton, HEAD, p_skeleton->get_bone_global_pose(p_setting->joints[HEAD].bone).origin);
			first = false;
		}

		p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_setting->chain[TAIL], solver_info->length));

		if (p_setting->joint_settings[HEAD].rotation_axis != ROTATION_AXIS_ALL) {
			p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_projected_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD]));
		}
		if (p_setting->joint_settings[HEAD].limitation.is_valid()) {
			p_setting->update_chain_coordinate_fw(p_skeleton, TAIL, p_setting->chain[HEAD] + p_setting->joint_settings[HEAD].get_limited_rotation(solver_info->current_grest, p_setting->chain[TAIL] - p_setting->chain[HEAD], solver_info->forward_vector));
		}
	}
}

void IKModifier3D::_solve_jacobian_iteration(Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination) {
	int joint_size = (int)p_setting->joints.size();
	int chain_size = (int)p_setting->chain.size();

	for (int i = 0; i < joint_size; i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		int HEAD = i;
		int TAIL = i + 1;

		Vector3 current_head = p_setting->chain[HEAD];
		Vector3 current_effector = p_setting->chain[chain_size - 1];
		Vector3 head_to_effector = current_effector - current_head;
		Vector3 effector_to_destination = p_destination - current_effector;
		Vector3 axis = head_to_effector.cross(effector_to_destination);

		if (Math::is_zero_approx(axis.length_squared())) {
			continue;
		}

		Quaternion to_rot = Quaternion(axis.normalized(), MIN(axis.length() / MAX(CMP_EPSILON, head_to_effector.length_squared()), angular_delta_limit));

		for (int j = TAIL; j < chain_size; j++) {
			Vector3 to_tail = p_setting->chain[j] - current_head;
			p_setting->update_chain_coordinate_fw(p_skeleton, j, current_head + to_rot.xform(to_tail));

			int k = j - 1;
			if (p_setting->joint_settings[k].rotation_axis != ROTATION_AXIS_ALL) {
				p_setting->update_chain_coordinate_fw(p_skeleton, j, p_setting->chain[k] + p_setting->joint_settings[k].get_projected_rotation(solver_info->current_grest, p_setting->chain[j] - p_setting->chain[k]));
			}
			if (p_setting->joint_settings[k].limitation.is_valid()) {
				p_setting->update_chain_coordinate_fw(p_skeleton, j, p_setting->chain[k] + p_setting->joint_settings[k].get_limited_rotation(solver_info->current_grest, p_setting->chain[j] - p_setting->chain[k], solver_info->forward_vector));
			}
		}
	}
}

void IKModifier3D::_process_spline_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, Ref<Curve3D> p_curve, const Transform3D &p_curve_space) {
	if (p_setting->solver_info_list.is_empty()) {
		return;
	}
	uint32_t joint_count = p_setting->joints.size();
	uint32_t joint_last = joint_count - 1;

	double path_length = p_curve->get_baked_length();
	PackedVector3Array points = p_curve->get_baked_points();
	Vector<real_t> tilts = p_curve->get_baked_tilts();
	Vector<real_t> dists = p_curve->get_baked_dist_cache();
	uint32_t point_count = points.size();
	if (point_count == 0) {
		return;
	}
	uint32_t point_last = point_count - 1;

	Vector3 start_point = p_curve_space.xform(points[0]);
	Vector3 start_vector = start_point - p_skeleton->get_bone_global_pose(p_setting->joints[0].bone).origin;
	double start_dist = start_vector.length();

	uint32_t chain_path_start = 0;
	while (chain_path_start < joint_count) {
		if (p_setting->chain_length_accum[chain_path_start] >= start_dist) {
			break;
		}
		chain_path_start++;
	}
	chain_path_start = (uint32_t)CLAMP((int)chain_path_start, 0, (int)joint_last);

	double fade_in_denom = 0.0;
	int denom_start = p_setting->tilt_fade_in > 0 ? CLAMP(p_setting->tilt_fade_in - 1, (int)chain_path_start, (int)joint_count) : -1;
	int denom_start_to = denom_start - p_setting->tilt_fade_in;
	if (denom_start >= 0) {
		for (int i = denom_start; i > denom_start_to; i--) {
			if (i < 0) {
				break;
			}
			IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
			if (!solver_info || Math::is_zero_approx(solver_info->length)) {
				continue;
			}
			fade_in_denom += solver_info->length;
		}
	}

	uint32_t ended = 0;
	Vector3 end_point = p_curve_space.xform(points[point_last]);
	Vector3 end_vector;
	double end_to_end_length = 0.0;
	double fade_out_denom = 0.0;

	uint32_t last_nearest = 0;
	uint32_t last_nearest_next = 0;
	double last_interpolate = 0.0;

	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		uint32_t HEAD = i;
		uint32_t TAIL = i + 1;
		bool is_fitting_first = HEAD == chain_path_start;

		if (ended > 0) {
			p_setting->update_chain_coordinate(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_setting->chain[HEAD] + end_vector, solver_info->length));
			if (p_setting->tilt_enabled) {
				if (p_setting->tilt_fade_out < 0) {
					p_setting->twists[HEAD] = 0.0;
				} else if (p_setting->tilt_fade_out == 0) {
					p_setting->twists[HEAD] = tilts[point_last];
				} else {
					if (ended == 1) {
						end_to_end_length = p_setting->chain[TAIL].distance_to(end_point);
					} else {
						end_to_end_length += solver_info->length;
					}
					double fade_out_dumping = CLAMP(end_to_end_length / fade_out_denom, 0.0, 1.0);
					p_setting->twists[HEAD] = Math::lerp(ended == 1 ? Math::lerp((double)tilts[last_nearest], (double)tilts[last_nearest_next], last_interpolate) : (double)tilts[point_last], 0.0, fade_out_dumping);
					ended = 2;
				}
			}
			continue;
		} else if (point_count == 1 || HEAD <= chain_path_start) {
			bool update_coordinate = !is_fitting_first || point_count == 1;
			if (update_coordinate) {
				p_setting->update_chain_coordinate(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_setting->chain[HEAD] + start_vector, solver_info->length));
			}
			if (p_setting->tilt_enabled) {
				if (p_setting->tilt_fade_in < 0) {
					p_setting->twists[HEAD] = 0.0;
				} else if (p_setting->tilt_fade_in == 0) {
					p_setting->twists[HEAD] = tilts[0];
				} else {
					double fade_in_dumping = CLAMP((double)(p_setting->chain[HEAD].distance_to(start_point) / fade_in_denom), 0.0, 1.0);
					p_setting->twists[HEAD] = Math::lerp((double)tilts[0], 0.0, fade_in_dumping);
				}
			}
			if (update_coordinate) {
				continue;
			}
		}

		double lsq = solver_info->length * solver_info->length;
		Vector3 head_in_chain_space = p_curve_space.xform_inv(p_setting->chain[HEAD]);
		double interpolate = 0.0;
		uint32_t nearest = p_setting->find_nearest_point(head_in_chain_space, lsq, points, p_curve->is_closed(), last_nearest, &interpolate);
		if (nearest >= point_count) {
			if (HEAD == 0) {
				nearest = point_count - 2;
				interpolate = 1.0;
			} else {
				Vector3 chain_end = (p_setting->chain[HEAD] - p_setting->chain[HEAD - 1]).normalized();
				Vector3 path_end = (p_curve_space.xform(points[point_last]) - p_setting->chain[HEAD]).normalized();
				double rest_path_length = path_length - Math::lerp((double)dists[last_nearest], (double)dists[last_nearest_next], last_interpolate);
				interpolate = CLAMP(rest_path_length / solver_info->length, 0.0, 1.0);
				end_vector = chain_end.lerp(path_end, interpolate);

				int denom_end = p_setting->tilt_fade_out > 0 ? CLAMP((int)joint_last - p_setting->tilt_fade_out, 0, (int)last_nearest) : -1;
				int denom_end_to = denom_end + p_setting->tilt_fade_out;
				if (denom_end >= 0) {
					for (int e = denom_end; e < denom_end_to; e++) {
						if (e >= (int)joint_count) {
							break;
						}
						IKModifier3DSolverInfo *end_solver_info = p_setting->solver_info_list[e];
						if (!end_solver_info || Math::is_zero_approx(end_solver_info->length)) {
							continue;
						}
						fade_out_denom += end_solver_info->length;
					}
				}
				ended = 1;
				i--;
				continue;
			}
		}
		uint32_t nearest_next = p_curve->is_closed() ? Math::posmod(nearest + 1, point_count) : CLAMP(nearest, (uint32_t)0, point_last);
		p_setting->update_chain_coordinate(p_skeleton, TAIL, limit_length(p_setting->chain[HEAD], p_curve_space.xform(points[nearest].lerp(points[nearest_next], interpolate)), solver_info->length));
		if (!is_fitting_first) {
			p_setting->twists[HEAD] = Math::lerp((double)tilts[last_nearest], (double)tilts[last_nearest_next], last_interpolate);
		}
		last_nearest = nearest;
		last_nearest_next = nearest_next;
		last_interpolate = interpolate;
	}

	p_setting->cache_spline_joint_rotations(p_skeleton, p_setting->tilt_enabled);

	for (uint32_t i = 0; i < p_setting->solver_info_list.size(); i++) {
		IKModifier3DSolverInfo *solver_info = p_setting->solver_info_list[i];
		if (!solver_info || Math::is_zero_approx(solver_info->length)) {
			continue;
		}
		p_skeleton->set_bone_pose_rotation(p_setting->joints[i].bone, solver_info->current_lpose);
	}
}

void IKModifier3D::_process_two_bone_joints(double p_delta, Skeleton3D *p_skeleton, ChainSetting *p_setting, const Vector3 &p_destination, const Vector3 &p_pole_destination) {
	Vector3 destination = p_destination;
	p_setting->root_pos = p_skeleton->get_bone_global_pose(p_setting->root_bone.bone).origin;
	Vector3 root_to_destination = destination - p_setting->root_pos;
	if (root_to_destination.is_zero_approx()) {
		return;
	}

	double rd_len_sq = root_to_destination.length_squared();
	if (rd_len_sq >= p_setting->cached_length_sq) {
		Vector3 rd_nrm = root_to_destination.normalized();
		p_setting->mid_pos = p_setting->root_pos + rd_nrm * p_setting->root_joint_solver_info->length;
		p_setting->end_pos = p_setting->mid_pos + rd_nrm * p_setting->mid_joint_solver_info->length;
	} else {
		double sub = p_setting->root_joint_solver_info->length - p_setting->mid_joint_solver_info->length;
		if (rd_len_sq < sub * sub) {
			Vector3 push_nrm = (destination - p_setting->root_pos).normalized();
			destination = p_setting->root_pos + push_nrm * Math::abs(sub);
			root_to_destination = destination - p_setting->root_pos;
		}
		p_setting->end_pos = destination;

		double l_chain = root_to_destination.length();
		Vector3 u = root_to_destination.normalized();
		Vector3 pole_vec = get_projected_normal(p_setting->root_pos, p_setting->end_pos, p_pole_destination);

		double r_root = p_setting->root_joint_solver_info->length;
		double r_mid = p_setting->mid_joint_solver_info->length;

		double a = (l_chain * l_chain + r_root * r_root - r_mid * r_mid) / (2.0 * l_chain);
		double h2 = MAX(0.0, r_root * r_root - a * a);
		double h = Math::sqrt(h2);

		Vector3 det_plus = (p_setting->root_pos + u * a) + pole_vec * h;
		Vector3 det_minus = (p_setting->root_pos + u * a) - pole_vec * h;

		p_setting->mid_pos = p_pole_destination.distance_squared_to(det_plus) < p_pole_destination.distance_squared_to(det_minus) ? det_plus : det_minus;
	}

	p_setting->cache_two_bone_vectors(p_skeleton);
	p_setting->cache_two_bone_joint_rotations(p_skeleton, p_pole_destination);

	p_skeleton->set_bone_pose_rotation(p_setting->root_bone.bone, p_setting->root_joint_solver_info->current_lpose);
	p_skeleton->set_bone_pose_rotation(p_setting->middle_bone.bone, get_local_pose_rotation(p_skeleton, p_setting->middle_bone.bone, p_setting->mid_joint_solver_info->current_gpose));
}


// --- Merged from scene/3d/physics/physics_body_3d.cpp ---
#if !defined(_3D_DISABLED)
#include "scene/3d/skeleton_3d.h"

#include "core/config/engine.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_3d/physics_server_3d.h"

namespace {

struct VehicleJacobianEntry {
	Vector3 m_linearJointAxis;
	Vector3 m_aJ;
	Vector3 m_bJ;
	Vector3 m_0MinvJt;
	Vector3 m_1MinvJt;
	real_t m_Adiag = 0.0f;

	VehicleJacobianEntry() = default;

	VehicleJacobianEntry(const Basis &world2A, const Basis &world2B, const Vector3 &rel_pos1,
			const Vector3 &rel_pos2, const Vector3 &jointAxis, const Vector3 &inertiaInvA,
			real_t massInvA, const Vector3 &inertiaInvB, real_t massInvB) :
			m_linearJointAxis(jointAxis) {
		m_aJ = world2A.xform(rel_pos1.cross(m_linearJointAxis));
		m_bJ = world2B.xform(rel_pos2.cross(-m_linearJointAxis));
		m_0MinvJt = inertiaInvA * m_aJ;
		m_1MinvJt = inertiaInvB * m_bJ;
		m_Adiag = massInvA + m_0MinvJt.dot(m_aJ) + massInvB + m_1MinvJt.dot(m_bJ);
	}
};

} // namespace

// ============================================================================
// VehicleWheel3D
// ============================================================================

void VehicleWheel3D::FTIData::update_world_xform(Transform3D &r_xform, real_t p_interpolation_fraction) {
	if (!unset) {
		real_t f = p_interpolation_fraction;

		WheelXform i;
		i.up = prev.up.lerp(curr.up, f).normalized();
		i.right = prev.right.lerp(curr.right, f).normalized();
		Vector3 fwd = i.up.cross(i.right).normalized();

		i.origin = prev.origin.lerp(curr.origin, f);
		i.steering = Math::lerp(prev.steering, curr.steering, f);

		real_t rotation = Math::lerp(curr_rotation - curr_rotation_delta, curr_rotation, f);

		Basis steeringMat(i.up, i.steering);
		Basis rotatingMat(i.right, rotation);
		Basis basis2(i.right[0], i.up[0], fwd[0], i.right[1], i.up[1], fwd[1], i.right[2], i.up[2], fwd[2]);

		r_xform.set_basis(steeringMat * rotatingMat * basis2);
		r_xform.set_origin(i.origin);
	}
}

void VehicleWheel3D::_update(PhysicsDirectBodyState3D *s) {
	if (m_raycastInfo.m_isInContact) {
		real_t project = m_raycastInfo.m_contactNormalWS.dot(m_raycastInfo.m_wheelDirectionWS);
		Vector3 relpos = m_raycastInfo.m_contactPointWS - s->get_transform().origin;
		Vector3 chassis_velocity_at_contact = s->get_linear_velocity() + s->get_angular_velocity().cross(relpos);
		real_t projVel = m_raycastInfo.m_contactNormalWS.dot(chassis_velocity_at_contact);
		if (project >= real_t(-0.1)) {
			m_suspensionRelativeVelocity = 0.0f;
			m_clippedInvContactDotSuspension = 10.0f;
		} else {
			real_t inv = real_t(-1.0) / project;
			m_suspensionRelativeVelocity = projVel * inv;
			m_clippedInvContactDotSuspension = inv;
		}
	} else {
		m_raycastInfo.m_suspensionLength = m_suspensionRestLength;
		m_suspensionRelativeVelocity = 0.0f;
		m_raycastInfo.m_contactNormalWS = -m_raycastInfo.m_wheelDirectionWS;
		m_clippedInvContactDotSuspension = 1.0f;
	}
}

void VehicleWheel3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			PhysicsBody3D *parent_body = dynamic_cast<PhysicsBody3D *>(get_parent());
			if (parent_body && parent_body->get_body_mode() == PhysicsBody3D::MODE_VEHICLE) {
				body = parent_body;
				local_xform = get_transform();
				m_chassisConnectionPointCS = local_xform.origin;
				m_wheelDirectionCS = -local_xform.basis.get_column(Vector3::AXIS_Y).normalized();
				m_wheelAxleCS = local_xform.basis.get_column(Vector3::AXIS_X).normalized();
				body->vehicle.wheels.push_back(this);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (body) {
				body->vehicle.wheels.erase(this);
				body = nullptr;
			}
		} break;
	}
}

Node3D *VehicleWheel3D::get_contact_body() const {
	return m_raycastInfo.m_groundObject;
}

PackedStringArray VehicleWheel3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	const PhysicsBody3D *parent_body = dynamic_cast<const PhysicsBody3D *>(get_parent());
	if (!parent_body || parent_body->get_body_mode() != PhysicsBody3D::MODE_VEHICLE) {
		warnings.push_back(RTR("VehicleWheel3D serves to provide a wheel system to a PhysicsBody3D in MODE_VEHICLE. Please use it as a child of a vehicle PhysicsBody3D."));
	}
	return warnings;
}

// ============================================================================
// PhysicsBody3D
// ============================================================================

PhysicsBody3D::PhysicsBody3D(BodyMode p_mode) :
		mode(p_mode) {
	collision_state.init(false, PS3DE::BODY_MODE_STATIC);
	set_notify_transform(true);
	if (mode == MODE_VEHICLE) {
		mass = 40.0f;
		vehicle.exclude.insert(get_rid());
	} else if (mode == MODE_PHYSICAL_BONE) {
		bone_state.joint = PhysicsServer3D::joint_create();
	}
	_apply_server_body_mode();
}

PhysicsBody3D::~PhysicsBody3D() {
	if (bone_state.joint.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(bone_state.joint);
	}
	collision_state.free_rid();
}

void PhysicsBody3D::set_body_mode(BodyMode p_mode) {
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	if (mode == MODE_VEHICLE) {
		vehicle.exclude.insert(get_rid());
	} else if (mode == MODE_PHYSICAL_BONE && !bone_state.joint.is_valid()) {
		bone_state.joint = PhysicsServer3D::joint_create();
	}
	_apply_server_body_mode();
	update_configuration_warnings();
}

void PhysicsBody3D::_apply_server_body_mode() {
	switch (mode) {
		case MODE_STATIC: {
			collision_state.set_body_mode(this, PS3DE::BODY_MODE_STATIC);
		} break;
		case MODE_ANIMATABLE:
		case MODE_CHARACTER: {
			collision_state.set_body_mode(this, PS3DE::BODY_MODE_KINEMATIC);
		} break;
		case MODE_RIGID:
		case MODE_VEHICLE:
		case MODE_PHYSICAL_BONE: {
			if (mode == MODE_PHYSICAL_BONE && !bone_state.internal_simulate_physics) {
				collision_state.set_body_mode(this, PS3DE::BODY_MODE_STATIC);
				break;
			}
			if (freeze) {
				collision_state.set_body_mode(this, freeze_mode == FREEZE_MODE_STATIC ? PS3DE::BODY_MODE_STATIC : PS3DE::BODY_MODE_KINEMATIC);
			} else if (lock_rotation) {
				collision_state.set_body_mode(this, PS3DE::BODY_MODE_RIGID_LINEAR);
			} else {
				collision_state.set_body_mode(this, PS3DE::BODY_MODE_RIGID);
			}
		} break;
	}
}

void PhysicsBody3D::_update_kinematic_motion() {
#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint()) {
		return;
	}
#endif
	if (mode == MODE_ANIMATABLE && sync_to_physics) {
		collision_state.only_update_transform_changes = true;
		set_notify_local_transform(true);
	} else {
		collision_state.only_update_transform_changes = false;
		set_notify_local_transform(false);
	}
}

void PhysicsBody3D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);

	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
#ifdef TOOLS_ENABLED
			if (Engine::is_editor_hint() && (mode == MODE_RIGID || mode == MODE_VEHICLE || mode == MODE_PHYSICAL_BONE)) {
				set_notify_local_transform(true);
			}
#endif
			if (mode == MODE_ANIMATABLE) {
				_update_kinematic_motion();
			} else if (mode == MODE_VEHICLE) {
				_update_vehicle_process_mode();
			}
		} break;
		case NOTIFICATION_INTERNAL_PROCESS: {
			if (mode == MODE_VEHICLE) {
				real_t f = Engine::get_physics_interpolation_fraction();
				Transform3D xform;
				Transform3D inv_vehicle_xform = get_global_transform_interpolated().affine_inverse();
				for (int i = 0; i < vehicle.wheels.size(); i++) {
					VehicleWheel3D &w = *vehicle.wheels[i];
					w.fti_data.update_world_xform(xform, f);
					w.set_transform(inv_vehicle_xform * xform);
				}
			}
		} break;
		case NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
			if (mode == MODE_VEHICLE) {
				_update_vehicle_process_mode();
				for (int i = 0; i < vehicle.wheels.size(); i++) {
					vehicle.wheels[i]->fti_data.reset_queued = true;
				}
			}
		} break;
	}
}

void PhysicsBody3D::_physics_interpolated_changed() {
	if (mode == MODE_VEHICLE) {
		_update_vehicle_process_mode();
	}
	Node3D::_physics_interpolated_changed();
}

void PhysicsBody3D::fti_pump_xform() {
	if (mode == MODE_VEHICLE) {
		for (int i = 0; i < vehicle.wheels.size(); i++) {
			vehicle.wheels[i]->fti_data.pump();
		}
	}
	Node3D::fti_pump_xform();
}

void PhysicsBody3D::_body_state_changed(PhysicsDirectBodyState3D *p_state) {
	if (mode == MODE_ANIMATABLE) {
		linear_velocity = p_state->get_linear_velocity();
		angular_velocity = p_state->get_angular_velocity();
		if (!sync_to_physics) {
			return;
		}
		last_valid_transform = p_state->get_transform();
		set_notify_local_transform(false);
		set_global_transform(last_valid_transform);
		set_notify_local_transform(true);
		collision_state.on_transform_changed(this);
		return;
	}

	Transform3D new_transform = p_state->get_transform();
	if (new_transform != get_global_transform()) {
		set_ignore_transform_notification(true);
		set_global_transform(new_transform);
		set_ignore_transform_notification(false);
	}

	linear_velocity = p_state->get_linear_velocity();
	angular_velocity = p_state->get_angular_velocity();
	inverse_inertia_tensor = p_state->get_inverse_inertia_tensor();
	contact_count = p_state->get_contact_count();
	sleeping = p_state->is_sleeping();

	if (mode == MODE_VEHICLE) {
		_step_vehicle(p_state);
	}
}

// Common motion, locks & exceptions.

bool PhysicsBody3D::move_and_collide(const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult &r_result, bool p_test_only, bool p_cancel_sliding) {
	bool colliding = PhysicsServer3D::body_test_motion(get_rid(), p_parameters, &r_result);

	if (p_cancel_sliding) {
		real_t motion_length = p_parameters.motion.length();
		real_t precision = 0.001f;

		if (colliding) {
			precision += motion_length * (r_result.collision_unsafe_fraction - r_result.collision_safe_fraction);
			if (r_result.collisions[0].depth > p_parameters.margin + precision) {
				p_cancel_sliding = false;
			}
		}

		if (p_cancel_sliding) {
			Vector3 motion_normal;
			if (motion_length > CMP_EPSILON) {
				motion_normal = p_parameters.motion / motion_length;
			}

			real_t projected_length = r_result.travel.dot(motion_normal);
			Vector3 recovery = r_result.travel - motion_normal * projected_length;
			if (recovery.length() < p_parameters.margin + precision) {
				r_result.travel = motion_normal * projected_length;
				r_result.remainder = p_parameters.motion - r_result.travel;
			}
		}
	}

	for (int i = 0; i < 3; i++) {
		if (locked_axis & (1 << i)) {
			r_result.travel[i] = 0;
		}
	}

	if (!p_test_only) {
		Transform3D gt = p_parameters.from;
		gt.origin += r_result.travel;
		set_global_transform(gt);
	}

	return colliding;
}

bool PhysicsBody3D::test_move(const Transform3D &p_from, const Vector3 &p_motion, const Ref<KinematicCollision3D> &r_collision, real_t p_margin, bool p_recovery_as_collision, int p_max_collisions) {
	ERR_FAIL_COND_V(!is_inside_tree(), false);

	PS3DT::MotionResult *r = nullptr;
	PS3DT::MotionResult temp_result;
	if (r_collision.is_valid()) {
		r = &r_collision->result;
	} else {
		r = &temp_result;
	}

	PS3DT::MotionParameters parameters(p_from, p_motion, p_margin);
	parameters.recovery_as_collision = p_recovery_as_collision;
	parameters.max_collisions = p_max_collisions;

	return PhysicsServer3D::body_test_motion(get_rid(), parameters, r);
}

Vector3 PhysicsBody3D::get_gravity() const {
	PhysicsDirectBodyState3D *state = PhysicsServer3D::body_get_direct_state(get_rid());
	ERR_FAIL_NULL_V(state, Vector3());
	return state->get_total_gravity();
}

void PhysicsBody3D::set_axis_lock(PS3DE::BodyAxis p_axis, bool p_lock) {
	if (p_lock) {
		locked_axis |= p_axis;
	} else {
		locked_axis &= (~p_axis);
	}
	PhysicsServer3D::body_set_axis_lock(get_rid(), p_axis, p_lock);
}

bool PhysicsBody3D::get_axis_lock(PS3DE::BodyAxis p_axis) const {
	return (locked_axis & p_axis) != 0;
}

void PhysicsBody3D::add_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody3D *other = dynamic_cast<PhysicsBody3D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody3D.");
	PhysicsServer3D::body_add_collision_exception(get_rid(), other->get_rid());
}

void PhysicsBody3D::remove_collision_exception_with(Node *rp_node) {
	ERR_FAIL_NULL(rp_node);
	PhysicsBody3D *other = dynamic_cast<PhysicsBody3D *>(rp_node);
	ERR_FAIL_NULL_MSG(other, "Collision exception target must be a PhysicsBody3D.");
	PhysicsServer3D::body_remove_collision_exception(get_rid(), other->get_rid());
}

// Static / Animatable / Rigid API.

void PhysicsBody3D::set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override) {
	physics_material_override = p_physics_material_override;
}

void PhysicsBody3D::set_sync_to_physics(bool p_enable) {
	if (sync_to_physics == p_enable) {
		return;
	}
	sync_to_physics = p_enable;
	if (is_inside_tree()) {
		_update_kinematic_motion();
	}
}

void PhysicsBody3D::set_lock_rotation_enabled(bool p_lock_rotation) {
	if (lock_rotation == p_lock_rotation) {
		return;
	}
	lock_rotation = p_lock_rotation;
	_apply_server_body_mode();
}

void PhysicsBody3D::set_freeze_enabled(bool p_freeze) {
	if (freeze == p_freeze) {
		return;
	}
	freeze = p_freeze;
	_apply_server_body_mode();
}

void PhysicsBody3D::set_freeze_mode(FreezeMode p_freeze_mode) {
	if (freeze_mode == p_freeze_mode) {
		return;
	}
	freeze_mode = p_freeze_mode;
	_apply_server_body_mode();
}

void PhysicsBody3D::set_mass(real_t p_mass) {
	ERR_FAIL_COND(p_mass <= 0);
	mass = p_mass;
}

real_t PhysicsBody3D::get_inverse_mass() const {
	if (mode == MODE_RIGID || mode == MODE_VEHICLE || mode == MODE_PHYSICAL_BONE) {
		return mass > 0.0f ? (1.0f / mass) : 0.0f;
	}
	return 0.0f;
}

Vector3 PhysicsBody3D::get_linear_velocity() const {
	if (mode == MODE_CHARACTER) {
		return character.real_velocity;
	}
	if (mode == MODE_STATIC) {
		return constant_linear_velocity;
	}
	return linear_velocity;
}

void PhysicsBody3D::set_axis_velocity(const Vector3 &p_axis) {
	PhysicsServer3D::body_set_axis_velocity(get_rid(), p_axis);
}

Vector3 PhysicsBody3D::get_angular_velocity() const {
	if (mode == MODE_STATIC) {
		return constant_angular_velocity;
	}
	return angular_velocity;
}

void PhysicsBody3D::set_use_custom_integrator(bool p_enable) {
	if (custom_integrator == p_enable) {
		return;
	}
	custom_integrator = p_enable;
	PhysicsServer3D::body_set_omit_force_integration(get_rid(), p_enable);
}

void PhysicsBody3D::set_max_contacts_reported(int p_amount) {
	ERR_FAIL_INDEX_MSG(p_amount, PS3DC::MAX_CONTACTS_REPORTED_3D_MAX, "Max contacts reported allocates memory and must not be set too high.");
	max_contacts_reported = p_amount;
	PhysicsServer3D::body_set_max_contacts_reported(get_rid(), p_amount);
}

void PhysicsBody3D::set_use_continuous_collision_detection(bool p_enable) {
	ccd = p_enable;
	PhysicsServer3D::body_set_enable_continuous_collision_detection(get_rid(), p_enable);
}

void PhysicsBody3D::apply_central_impulse(const Vector3 &p_impulse) {
	PhysicsServer3D::body_apply_central_impulse(get_rid(), p_impulse);
}

void PhysicsBody3D::apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position) {
	PhysicsServer3D::body_apply_impulse(get_rid(), p_impulse, p_position);
}

void PhysicsBody3D::apply_torque_impulse(const Vector3 &p_impulse) {
	PhysicsServer3D::body_apply_torque_impulse(get_rid(), p_impulse);
}

void PhysicsBody3D::apply_central_force(const Vector3 &p_force) {
	PhysicsServer3D::body_apply_central_force(get_rid(), p_force);
}

void PhysicsBody3D::apply_force(const Vector3 &p_force, const Vector3 &p_position) {
	PhysicsServer3D::body_apply_force(get_rid(), p_force, p_position);
}

void PhysicsBody3D::apply_torque(const Vector3 &p_torque) {
	PhysicsServer3D::body_apply_torque(get_rid(), p_torque);
}

void PhysicsBody3D::add_constant_central_force(const Vector3 &p_force) {
	PhysicsServer3D::body_add_constant_central_force(get_rid(), p_force);
}

void PhysicsBody3D::add_constant_force(const Vector3 &p_force, const Vector3 &p_position) {
	PhysicsServer3D::body_add_constant_force(get_rid(), p_force, p_position);
}

void PhysicsBody3D::add_constant_torque(const Vector3 &p_torque) {
	PhysicsServer3D::body_add_constant_torque(get_rid(), p_torque);
}

void PhysicsBody3D::set_constant_force(const Vector3 &p_force) {
	PhysicsServer3D::body_set_constant_force(get_rid(), p_force);
}

Vector3 PhysicsBody3D::get_constant_force() const {
	return PhysicsServer3D::body_get_constant_force(get_rid());
}

void PhysicsBody3D::set_constant_torque(const Vector3 &p_torque) {
	PhysicsServer3D::body_set_constant_torque(get_rid(), p_torque);
}

Vector3 PhysicsBody3D::get_constant_torque() const {
	return PhysicsServer3D::body_get_constant_torque(get_rid());
}

// ============================================================================
// CharacterMover3D
// ============================================================================

void PhysicsBody3D::_set_collision_direction(const PS3DT::MotionResult &p_result, CharacterMover3D::FloorFlags &r_state, CharacterMover3D::FloorFlags p_apply_state) {
	r_state = CharacterMover3D::FloorFlags();
	if (character.motion_mode == MOTION_MODE_GROUNDED && character.up_direction != Vector3()) {
		for (int i = p_result.collision_count - 1; i >= 0; i--) {
			const PS3DT::MotionCollision &col = p_result.collisions[i];
			real_t floor_angle = Math::acos(col.normal.dot(character.up_direction));
			if (floor_angle <= character.floor_max_angle + 0.01f) {
				r_state.floor = true;
				if (p_apply_state.floor) {
					character.floor_normal = col.normal;
				}
			} else if (floor_angle >= (Math::PI - character.floor_max_angle - 0.01f)) {
				r_state.ceiling = true;
				if (p_apply_state.ceiling) {
					character.ceiling_normal = col.normal;
				}
			} else {
				r_state.wall = true;
				if (p_apply_state.wall) {
					character.wall_normal = col.normal;
				}
			}
		}
	} else {
		if (p_result.collision_count > 0) {
			r_state.wall = true;
			if (p_apply_state.wall) {
				character.wall_normal = p_result.collisions[0].normal;
			}
		}
	}
}

void PhysicsBody3D::apply_floor_snap() {
	if (character.flags.floor) {
		return;
	}

	real_t length = MAX(character.floor_snap_length, character.margin);
	PS3DT::MotionParameters parameters(get_global_transform(), -character.up_direction * length, character.margin);
	parameters.max_collisions = 4;
	parameters.recovery_as_collision = true;
	parameters.collide_separation_ray = true;

	PS3DT::MotionResult result;
	if (move_and_collide(parameters, result, true, false)) {
		CharacterMover3D::FloorFlags result_state;
		_set_collision_direction(result, result_state, CharacterMover3D::FloorFlags(true, false, false));

		if (result_state.floor) {
			if (result.travel.length() > character.margin) {
				result.travel = character.up_direction * character.up_direction.dot(result.travel);
			} else {
				result.travel = Vector3();
			}
			parameters.from.origin += result.travel;
			set_global_transform(parameters.from);
		}
	}
}

void PhysicsBody3D::_snap_on_floor(bool p_was_on_floor, bool p_vel_dir_facing_up) {
	if (character.flags.floor || !p_was_on_floor || p_vel_dir_facing_up) {
		return;
	}
	apply_floor_snap();
}

bool PhysicsBody3D::_on_floor_if_snapped(bool p_was_on_floor, bool p_vel_dir_facing_up) {
	if (character.up_direction == Vector3() || character.flags.floor || !p_was_on_floor || p_vel_dir_facing_up) {
		return false;
	}

	real_t length = MAX(character.floor_snap_length, character.margin);
	PS3DT::MotionParameters parameters(get_global_transform(), -character.up_direction * length, character.margin);
	parameters.max_collisions = 4;
	parameters.recovery_as_collision = true;
	parameters.collide_separation_ray = true;

	PS3DT::MotionResult result;
	if (move_and_collide(parameters, result, true, false)) {
		CharacterMover3D::FloorFlags result_state;
		_set_collision_direction(result, result_state, CharacterMover3D::FloorFlags(false, false, false));
		return result_state.floor;
	}
	return false;
}

bool PhysicsBody3D::move_and_slide() {
	double delta = Engine::is_in_physics_frame() ? get_physics_process_delta_time() : get_process_delta_time();
	if (delta <= 0.0) {
		return false;
	}

	Transform3D gt = get_global_transform();
	character.previous_position = gt.origin;

	bool was_on_floor = character.flags.floor;
	character.flags = CharacterMover3D::FloorFlags();
	character.motion_results.clear();
	character.last_motion = Vector3();

	Vector3 motion = character.velocity * delta;
	for (int iteration = 0; iteration < character.max_slides; ++iteration) {
		PS3DT::MotionParameters parameters(get_global_transform(), motion, character.margin);
		parameters.max_collisions = 6;
		parameters.recovery_as_collision = true;

		PS3DT::MotionResult result;
		bool collided = move_and_collide(parameters, result, false, false);
		character.last_motion = result.travel;

		if (collided) {
			character.motion_results.push_back(result);
			CharacterMover3D::FloorFlags step_flags;
			_set_collision_direction(result, step_flags);
			character.flags.floor = character.flags.floor || step_flags.floor;
			character.flags.wall = character.flags.wall || step_flags.wall;
			character.flags.ceiling = character.flags.ceiling || step_flags.ceiling;

			if (result.remainder.is_zero_approx()) {
				break;
			}
			const Vector3 &hit_normal = result.collisions[0].normal;
			motion = result.remainder.slide(hit_normal);
			character.velocity = character.velocity.slide(hit_normal);
		} else {
			break;
		}
	}

	if (character.motion_mode == MOTION_MODE_GROUNDED) {
		bool vel_dir_facing_up = character.velocity.dot(character.up_direction) > 0;
		_snap_on_floor(was_on_floor, vel_dir_facing_up);
	}

	character.real_velocity = (get_global_transform().origin - character.previous_position) / delta;
	return !character.motion_results.is_empty();
}

void PhysicsBody3D::set_up_direction(const Vector3 &p_up_direction) {
	ERR_FAIL_COND_MSG(p_up_direction == Vector3(), "up_direction can't be equal to Vector3.ZERO, consider using Floating motion mode instead.");
	character.up_direction = p_up_direction.normalized();
}

real_t PhysicsBody3D::get_floor_angle(const Vector3 &p_up_direction) const {
	ERR_FAIL_COND_V(p_up_direction == Vector3(), 0.0f);
	return Math::acos(character.floor_normal.dot(p_up_direction));
}

PS3DT::MotionResult PhysicsBody3D::get_slide_collision(int p_bounce) const {
	ERR_FAIL_INDEX_V(p_bounce, character.motion_results.size(), PS3DT::MotionResult());
	return character.motion_results[p_bounce];
}

void PhysicsBody3D::set_max_slides(int p_max_slides) {
	ERR_FAIL_COND(p_max_slides < 1);
	character.max_slides = p_max_slides;
}

void PhysicsBody3D::set_floor_snap_length(real_t p_floor_snap_length) {
	ERR_FAIL_COND(p_floor_snap_length < 0);
	character.floor_snap_length = p_floor_snap_length;
}

// ============================================================================
// VehicleRig3D
// ============================================================================

PhysicsBody3D::VehicleWheelContactPoint::VehicleWheelContactPoint(PhysicsDirectBodyState3D *s,
		PhysicsBody3D *body1, const Vector3 &frictionPosWorld, const Vector3 &frictionDirectionWorld,
		real_t maxImpulse) :
		m_s(s), m_body1(body1), m_frictionPositionWorld(frictionPosWorld),
		m_frictionDirectionWorld(frictionDirectionWorld), m_maxImpulse(maxImpulse) {
	Vector3 r0 = frictionPosWorld - s->get_transform().origin;
	Vector3 c0 = r0.cross(frictionDirectionWorld);
	Vector3 vec = s->get_inverse_inertia_tensor().xform_inv(c0).cross(r0);
	real_t denom0 = s->get_inverse_mass() + frictionDirectionWorld.dot(vec);
	m_jacDiagABInv = denom0 != 0.0f ? (1.0f / denom0) : 0.0f;
}

void PhysicsBody3D::_update_wheel_transform(VehicleWheel3D &wheel, PhysicsDirectBodyState3D *s) {
	wheel.m_raycastInfo.m_isInContact = false;
	Transform3D chassisTrans = s->get_transform();
	wheel.m_raycastInfo.m_hardPointWS = chassisTrans.xform(wheel.m_chassisConnectionPointCS);
	wheel.m_raycastInfo.m_wheelDirectionWS = chassisTrans.get_basis().xform(wheel.m_wheelDirectionCS).normalized();
	wheel.m_raycastInfo.m_wheelAxleWS = chassisTrans.get_basis().xform(wheel.m_wheelAxleCS).normalized();
}

void PhysicsBody3D::_update_wheel(int p_idx, PhysicsDirectBodyState3D *s) {
	VehicleWheel3D &wheel = *vehicle.wheels[p_idx];
	_update_wheel_transform(wheel, s);

	Vector3 up = -wheel.m_raycastInfo.m_wheelDirectionWS;
	const Vector3 &right = wheel.m_raycastInfo.m_wheelAxleWS;
	Vector3 fwd = up.cross(right).normalized();

	VehicleWheel3D::FTIData &id = wheel.fti_data;
	Vector3 origin = wheel.m_raycastInfo.m_hardPointWS + wheel.m_raycastInfo.m_wheelDirectionWS * wheel.m_raycastInfo.m_suspensionLength;

	if (is_physics_interpolated_and_enabled()) {
		id.curr.up = up;
		id.curr.right = right;
		id.curr.steering = wheel.m_steering;
		id.curr.origin = origin;
		if (id.unset || id.reset_queued) {
			id.unset = false;
			id.reset_queued = false;
			id.pump();
		}
	}

	Basis steeringMat(up, wheel.m_steering);
	Basis rotatingMat(right, wheel.m_rotation);
	Basis basis2(right[0], up[0], fwd[0], right[1], up[1], fwd[1], right[2], up[2], fwd[2]);

	wheel.m_worldTransform.set_basis(steeringMat * rotatingMat * basis2);
	wheel.m_worldTransform.set_origin(origin);
}

void PhysicsBody3D::_update_vehicle_suspension(PhysicsDirectBodyState3D *s) {
	for (int w_it = 0; w_it < vehicle.wheels.size(); w_it++) {
		VehicleWheel3D &wheel_info = *vehicle.wheels[w_it];
		if (wheel_info.m_raycastInfo.m_isInContact) {
			real_t length_diff = wheel_info.m_suspensionRestLength - wheel_info.m_raycastInfo.m_suspensionLength;
			real_t force = wheel_info.m_suspensionStiffness * length_diff * wheel_info.m_clippedInvContactDotSuspension;

			real_t projected_rel_vel = wheel_info.m_suspensionRelativeVelocity;
			real_t susp_damping = projected_rel_vel < 0.0f ? wheel_info.m_wheelsDampingCompression : wheel_info.m_wheelsDampingRelaxation;
			force -= susp_damping * projected_rel_vel;

			wheel_info.m_wheelsSuspensionForce = MAX(force * mass, real_t(0.0));
		} else {
			wheel_info.m_wheelsSuspensionForce = 0.0f;
		}
	}
}

void PhysicsBody3D::_resolve_single_bilateral(PhysicsDirectBodyState3D *s, const Vector3 &pos1,
		PhysicsBody3D *body2, const Vector3 &pos2, const Vector3 &normal, real_t &impulse, real_t p_rollInfluence) {
	if (normal.length_squared() > real_t(1.1)) {
		impulse = 0.0f;
		return;
	}

	Vector3 rel_pos1 = pos1 - s->get_transform().origin;
	Vector3 rel_pos2 = body2 ? (pos2 - body2->get_global_transform().origin) : Vector3();

	Vector3 vel1 = s->get_linear_velocity() + s->get_angular_velocity().cross(rel_pos1);
	Vector3 vel2 = body2 ? (body2->get_linear_velocity() + body2->get_angular_velocity().cross(rel_pos2)) : Vector3();
	real_t rel_vel = normal.dot(vel1 - vel2);

	real_t b2invmass = body2 ? body2->get_inverse_mass() : 0.0f;
	real_t contactDamping = 0.2f;
	if (p_rollInfluence > 0.0f) {
		contactDamping = MIN(contactDamping, s->get_step() / p_rollInfluence);
	}

	real_t massTerm = 1.0f / ((1.0f / mass) + b2invmass);
	impulse = -contactDamping * rel_vel * massTerm;
}

real_t PhysicsBody3D::_calc_rolling_friction(VehicleWheelContactPoint &contactPoint) {
	const Vector3 &contactPosWorld = contactPoint.m_frictionPositionWorld;
	Vector3 rel_pos1 = contactPosWorld - contactPoint.m_s->get_transform().origin;
	Vector3 rel_pos2 = contactPoint.m_body1 ? (contactPosWorld - contactPoint.m_body1->get_global_transform().origin) : Vector3();

	Vector3 vel1 = contactPoint.m_s->get_linear_velocity() + contactPoint.m_s->get_angular_velocity().cross(rel_pos1);
	Vector3 vel2 = contactPoint.m_body1 ? (contactPoint.m_body1->get_linear_velocity() + contactPoint.m_body1->get_angular_velocity().cross(rel_pos2)) : Vector3();

	real_t vrel = contactPoint.m_frictionDirectionWorld.dot(vel1 - vel2);
	real_t j1 = -vrel * contactPoint.m_jacDiagABInv;
	return CLAMP(j1, -contactPoint.m_maxImpulse, contactPoint.m_maxImpulse);
}

void PhysicsBody3D::_update_vehicle_friction(PhysicsDirectBodyState3D *s) {
	int numWheel = vehicle.wheels.size();
	if (!numWheel) {
		return;
	}

	vehicle.forward_ws.resize(numWheel);
	vehicle.axle.resize(numWheel);
	vehicle.forward_impulse.resize(numWheel);
	vehicle.side_impulse.resize(numWheel);

	for (int i = 0; i < numWheel; i++) {
		vehicle.side_impulse.write[i] = 0.0f;
		vehicle.forward_impulse.write[i] = 0.0f;

		VehicleWheel3D &wheelInfo = *vehicle.wheels[i];
		if (wheelInfo.m_raycastInfo.m_isInContact) {
			Basis wheelBasis0 = wheelInfo.m_worldTransform.basis;
			vehicle.axle.write[i] = wheelBasis0.get_column(Vector3::AXIS_X);
			const Vector3 &surfNormalWS = wheelInfo.m_raycastInfo.m_contactNormalWS;
			real_t proj = vehicle.axle[i].dot(surfNormalWS);
			vehicle.axle.write[i] = (vehicle.axle[i] - surfNormalWS * proj).normalized();
			vehicle.forward_ws.write[i] = surfNormalWS.cross(vehicle.axle[i]).normalized();

			_resolve_single_bilateral(s, wheelInfo.m_raycastInfo.m_contactPointWS,
					wheelInfo.m_raycastInfo.m_groundObject,
					wheelInfo.m_raycastInfo.m_contactPointWS, vehicle.axle[i], vehicle.side_impulse.write[i],
					wheelInfo.m_rollInfluence);
		}
	}

	bool sliding = false;
	for (int wheel = 0; wheel < numWheel; wheel++) {
		VehicleWheel3D &wheelInfo = *vehicle.wheels[wheel];
		real_t rollingFriction = 0.0f;
		if (wheelInfo.m_raycastInfo.m_isInContact) {
			if (wheelInfo.m_engineForce != 0.0f) {
				rollingFriction = -wheelInfo.m_engineForce * s->get_step();
			} else {
				real_t maxImpulse = wheelInfo.m_brake ? wheelInfo.m_brake : 0.0f;
				VehicleWheelContactPoint contactPt(s, wheelInfo.m_raycastInfo.m_groundObject,
						wheelInfo.m_raycastInfo.m_contactPointWS, vehicle.forward_ws[wheel], maxImpulse);
				rollingFriction = _calc_rolling_friction(contactPt);
			}
		}

		vehicle.forward_impulse.write[wheel] = 0.0f;
		wheelInfo.m_skidInfo = 1.0f;

		if (wheelInfo.m_raycastInfo.m_isInContact) {
			real_t maximp = wheelInfo.m_wheelsSuspensionForce * s->get_step() * wheelInfo.m_frictionSlip;
			real_t maximpSquared = maximp * maximp;
			vehicle.forward_impulse.write[wheel] = rollingFriction;

			real_t x = vehicle.forward_impulse[wheel] * 0.5f;
			real_t y = vehicle.side_impulse[wheel];
			real_t impulseSquared = x * x + y * y;
			if (impulseSquared > maximpSquared) {
				sliding = true;
				wheelInfo.m_skidInfo *= maximp / Math::sqrt(impulseSquared);
			}
		}
	}

	if (sliding) {
		for (int wheel = 0; wheel < numWheel; wheel++) {
			if (vehicle.side_impulse[wheel] != 0.0f && vehicle.wheels[wheel]->m_skidInfo < 1.0f) {
				vehicle.forward_impulse.write[wheel] *= vehicle.wheels[wheel]->m_skidInfo;
				vehicle.side_impulse.write[wheel] *= vehicle.wheels[wheel]->m_skidInfo;
			}
		}
	}

	for (int wheel = 0; wheel < numWheel; wheel++) {
		VehicleWheel3D &wheelInfo = *vehicle.wheels[wheel];
		Vector3 rel_pos = wheelInfo.m_raycastInfo.m_contactPointWS - s->get_transform().origin;
		if (vehicle.forward_impulse[wheel] != 0.0f) {
			s->apply_impulse(vehicle.forward_ws[wheel] * vehicle.forward_impulse[wheel], rel_pos);
		}
		if (vehicle.side_impulse[wheel] != 0.0f) {
			Vector3 sideImp = vehicle.axle[wheel] * vehicle.side_impulse[wheel];
			Vector3 vChassisWorldUp = s->get_transform().basis.transposed()[1];
			rel_pos -= vChassisWorldUp * (vChassisWorldUp.dot(rel_pos) * (1.0f - wheelInfo.m_rollInfluence));
			s->apply_impulse(sideImp, rel_pos);
		}
	}
}

real_t PhysicsBody3D::_vehicle_ray_cast(int p_idx, PhysicsDirectBodyState3D *s) {
	VehicleWheel3D &wheel = *vehicle.wheels[p_idx];
	_update_wheel_transform(wheel, s);

	real_t raylen = wheel.m_suspensionRestLength + wheel.m_wheelRadius;
	Vector3 rayvector = wheel.m_raycastInfo.m_wheelDirectionWS * raylen;
	const Vector3 &source = wheel.m_raycastInfo.m_hardPointWS;
	wheel.m_raycastInfo.m_contactPointWS = source + rayvector;

	PhysicsDirectSpaceState3D *ss = s->get_space_state();
	if (ss) {
		PS3DT::RayParameters ray_params;
		ray_params.from = source;
		ray_params.to = source + rayvector;
		ray_params.exclude = vehicle.exclude;
		ray_params.collision_mask = collision_state.collision_mask;

		PS3DT::RayResult rr;
		if (ss->intersect_ray(ray_params, rr)) {
			real_t param = source.distance_to(rr.position) / raylen;
			wheel.m_raycastInfo.m_contactNormalWS = rr.normal;
			wheel.m_raycastInfo.m_isInContact = true;
			wheel.m_raycastInfo.m_groundObject = nullptr;
			real_t hit_dist = param * raylen;
			wheel.m_raycastInfo.m_suspensionLength = CLAMP(hit_dist - wheel.m_wheelRadius, wheel.m_suspensionRestLength - wheel.m_maxSuspensionTravel, wheel.m_suspensionRestLength + wheel.m_maxSuspensionTravel);
			wheel.m_raycastInfo.m_contactPointWS = rr.position;
			wheel._update(s);
			return hit_dist;
		}
	}

	wheel.m_raycastInfo.m_suspensionLength = wheel.m_suspensionRestLength;
	wheel.m_raycastInfo.m_groundObject = nullptr;
	wheel._update(s);
	return -1.0f;
}

void PhysicsBody3D::_update_vehicle_process_mode() {
	set_process_internal(is_physics_interpolated_and_enabled());
}

void PhysicsBody3D::_step_vehicle(PhysicsDirectBodyState3D *p_state) {
	real_t step = p_state->get_step();
	bool use_fti = is_physics_interpolated_and_enabled();

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		_update_wheel(i, p_state);
	}

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		_vehicle_ray_cast(i, p_state);
		if (!use_fti) {
			vehicle.wheels[i]->set_transform(p_state->get_transform().inverse() * vehicle.wheels[i]->m_worldTransform);
		}
	}

	_update_vehicle_suspension(p_state);

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		VehicleWheel3D &wheel = *vehicle.wheels[i];
		real_t suspensionForce = MIN(wheel.m_wheelsSuspensionForce, wheel.m_maxSuspensionForce);
		Vector3 impulse = wheel.m_raycastInfo.m_contactNormalWS * suspensionForce * step;
		Vector3 relative_position = wheel.m_raycastInfo.m_contactPointWS - p_state->get_transform().origin;
		p_state->apply_impulse(impulse, relative_position);
	}

	_update_vehicle_friction(p_state);

	for (int i = 0; i < vehicle.wheels.size(); i++) {
		VehicleWheel3D &wheel = *vehicle.wheels[i];
		Vector3 relpos = wheel.m_raycastInfo.m_hardPointWS - p_state->get_transform().origin;
		Vector3 vel = p_state->get_linear_velocity() + p_state->get_angular_velocity().cross(relpos);

		if (wheel.m_raycastInfo.m_isInContact) {
			const Transform3D &chassisWorldTransform = p_state->get_transform();
			Vector3 fwd(chassisWorldTransform.basis[0][Vector3::AXIS_Z],
					chassisWorldTransform.basis[1][Vector3::AXIS_Z],
					chassisWorldTransform.basis[2][Vector3::AXIS_Z]);
			if (wheel.steers) {
				fwd = Basis(Vector3(0, 1, 0), wheel.m_steering).xform(fwd);
			}
			real_t proj = fwd.dot(wheel.m_raycastInfo.m_contactNormalWS);
			fwd = (fwd - wheel.m_raycastInfo.m_contactNormalWS * proj).normalized();
			wheel.m_deltaRotation = (fwd.dot(vel) * step) / wheel.m_wheelRadius;
		}

		wheel.m_rotation += wheel.m_deltaRotation;
		if (use_fti) {
			wheel.fti_data.rotate(wheel.m_deltaRotation);
		}
		wheel.m_rpm = ((wheel.m_deltaRotation / step) * 60.0f) / Math::TAU;
		wheel.m_deltaRotation *= 0.99f;
	}
}

void PhysicsBody3D::set_engine_force(real_t p_engine_force) {
	vehicle.engine_force = p_engine_force;
	for (int i = 0; i < vehicle.wheels.size(); i++) {
		if (vehicle.wheels[i]->engine_traction) {
			vehicle.wheels[i]->m_engineForce = p_engine_force;
		}
	}
}

void PhysicsBody3D::set_brake(real_t p_brake) {
	vehicle.brake = p_brake;
	for (int i = 0; i < vehicle.wheels.size(); i++) {
		vehicle.wheels[i]->m_brake = p_brake;
	}
}

void PhysicsBody3D::set_steering(real_t p_steering) {
	vehicle.steering = p_steering;
	for (int i = 0; i < vehicle.wheels.size(); i++) {
		if (vehicle.wheels[i]->steers) {
			vehicle.wheels[i]->m_steering = p_steering;
		}
	}
}

// ============================================================================
// PhysicalBoneState3D
// ============================================================================

PhysicalBoneSimulator3D *PhysicsBody3D::get_simulator() const {
	return dynamic_cast<PhysicalBoneSimulator3D *>(get_parent());
}

void PhysicsBody3D::set_body_offset(const Transform3D &p_offset) {
	bone_state.body_offset = p_offset;
	bone_state.body_offset_inverse = p_offset.affine_inverse();
	if (get_simulator()) {
		bone_state.joint_offset.origin = bone_state.body_offset_inverse.origin;
	}
}

void PhysicsBody3D::set_simulate_physics(bool p_simulate) {
	if (bone_state.simulate_physics == p_simulate) {
		return;
	}
	bone_state.simulate_physics = p_simulate;
	reset_physics_simulation_state();
}

void PhysicsBody3D::reset_physics_simulation_state() {
	if (bone_state.simulate_physics) {
		_start_bone_physics_simulation();
	} else {
		_stop_bone_physics_simulation();
	}
}

void PhysicsBody3D::_start_bone_physics_simulation() {
	if (bone_state.internal_simulate_physics) {
		return;
	}
	bone_state.internal_simulate_physics = true;
	_apply_server_body_mode();
}

void PhysicsBody3D::_stop_bone_physics_simulation() {
	if (!bone_state.internal_simulate_physics) {
		return;
	}
	bone_state.internal_simulate_physics = false;
	_apply_server_body_mode();
}

#ifdef TOOLS_ENABLED
Transform3D PhysicsBody3D::get_global_gizmo_transform() const {
	if (mode == MODE_PHYSICAL_BONE && bone_state.gizmo_move_joint) {
		return get_global_transform() * bone_state.joint_offset;
	}
	return Node3D::get_global_gizmo_transform();
}

Transform3D PhysicsBody3D::get_local_gizmo_transform() const {
	if (mode == MODE_PHYSICAL_BONE && bone_state.gizmo_move_joint) {
		return get_transform() * bone_state.joint_offset;
	}
	return Node3D::get_local_gizmo_transform();
}
#endif

PackedStringArray PhysicsBody3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nConsider adding an inline Shape3D or a CollisionShape3D / CollisionPolygon3D child."));
	}

	if (SceneTree::is_fti_enabled_in_project() && !is_physics_interpolated()) {
		warnings.push_back(RTR("PhysicsBody3D will not work correctly on a non-interpolated branch of the SceneTree.\nCheck the node's inherited physics_interpolation_mode."));
	}

	if (mode == MODE_RIGID || mode == MODE_VEHICLE) {
		Vector3 scale = get_transform().get_basis().get_scale();
		if (Math::abs(scale.x - 1.0f) > 0.05f || Math::abs(scale.y - 1.0f) > 0.05f || Math::abs(scale.z - 1.0f) > 0.05f) {
			warnings.push_back(RTR("Scale changes to Rigid/Vehicle PhysicsBody3D will be overridden by the physics engine when running.\nPlease change the size in collision shapes instead."));
		}
	}

	return warnings;
}

// ==========================================================================
// Merged from scene/3d/physics/kinematic_collision_3d.cpp
// ==========================================================================

#include "scene/3d/skeleton_3d.h"

Vector3 KinematicCollision3D::get_travel() const { return result.travel; }

Vector3 KinematicCollision3D::get_remainder() const { return result.remainder; }

int KinematicCollision3D::get_collision_count() const { return result.collision_count; }

real_t KinematicCollision3D::get_depth() const { return result.collision_depth; }

Vector3 KinematicCollision3D::get_position(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].position;
}

Vector3 KinematicCollision3D::get_normal(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].normal;
}

real_t KinematicCollision3D::get_angle(int p_collision_index, const Vector3& p_up_direction) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0.0);
	ERR_FAIL_COND_V(p_up_direction == Vector3(), 0);
	return result.collisions[p_collision_index].get_angle(p_up_direction);
}

RID KinematicCollision3D::get_collider_rid(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, RID());
	return result.collisions[p_collision_index].collider;
}

int KinematicCollision3D::get_collider_shape_index(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].collider_shape;
}

Vector3 KinematicCollision3D::get_collider_velocity(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].collider_velocity;
}

// ==========================================================================
// Merged from scene/3d/physics/physical_bone_simulator_3d.cpp
// ==========================================================================

#include "scene/3d/skeleton_3d.h"

#include "core/config/engine.h"
#include "scene/3d/skeleton_3d.h"

void PhysicalBoneSimulator3D::_bone_pose_updated(Skeleton3D *p_skeleton, int p_bone_id) {
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_bone_id, bones.size());
	bones[p_bone_id].global_pose = p_skeleton->get_bone_global_pose(p_bone_id);
}

void PhysicalBoneSimulator3D::_set_active(bool p_active) {
	if (!Engine::is_editor_hint()) {
		_reset_physical_bones_state();
	}
}

void PhysicalBoneSimulator3D::_reset_physical_bones_state() {
	for (uint32_t i = 0; i < bones.size(); i += 1) {
		if (bones[i].physical_bone) {
			bones[i].physical_bone->reset_physics_simulation_state();
		}
	}
}

bool PhysicalBoneSimulator3D::is_simulating_physics() const {
	return simulating;
}

int PhysicalBoneSimulator3D::get_bone_count() const {
	return bones.size();
}

PhysicsBody3D *PhysicalBoneSimulator3D::get_physical_bone(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);
	return bones[p_bone].physical_bone;
}

PhysicsBody3D *PhysicalBoneSimulator3D::get_physical_bone_parent(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);
	if (bones[p_bone].cache_parent_physical_bone) {
		return bones[p_bone].cache_parent_physical_bone;
	}
	return _get_physical_bone_parent(p_bone);
}

PhysicsBody3D *PhysicalBoneSimulator3D::_get_physical_bone_parent(int p_bone) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, nullptr);
	const int parent_bone = bones[p_bone].parent;
	if (parent_bone < 0) {
		return nullptr;
	}
	PhysicsBody3D *pb = bones[parent_bone].physical_bone;
	return pb ? pb : get_physical_bone_parent(parent_bone);
}

Transform3D PhysicalBoneSimulator3D::get_bone_global_pose(int p_bone) const {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX_V(p_bone, bone_size, Transform3D());
	return bones[p_bone].global_pose;
}

void PhysicalBoneSimulator3D::set_bone_global_pose(int p_bone, const Transform3D &p_pose) {
	const int bone_size = bones.size();
	ERR_FAIL_INDEX(p_bone, bone_size);
	bones[p_bone].global_pose = p_pose;
}

void PhysicalBoneSimulator3D::_skeleton_changed(Skeleton3D *p_old_skeleton, Skeleton3D *p_new_skeleton) {}

// ============================================================================
// Merged from scene/3d/physics/collision_object_3d.cpp
// ============================================================================

#include "scene/3d/skeleton_3d.h"

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

#include "scene/3d/skeleton_3d.h"

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

#include "scene/3d/skeleton_3d.h"

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
// --- Inlined from scene/3d/physics/collision_polygon_3d.h ---
#include "scene/3d/node_3d.h"

class CollisionPolygon3D : public Node3D {
	real_t margin = 0.04f;

protected:
	real_t depth = 1.0f;
	AABB aabb = AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	Vector<Point2> polygon;

	uint32_t owner_id = 0;
	CollisionState3D *collision_state = nullptr;

	Color debug_color;
	bool debug_fill = true;
	bool disabled = false;

	Color _get_default_debug_color() const;
	void _update_in_shape_owner(bool p_xform_only = false);
	bool _is_editable_3d_polygon() const;
	void _notification(int p_what);

public:
	real_t get_depth() const;
	Vector<Point2> get_polygon() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	Color get_debug_color() const;
	bool get_debug_fill_enabled() const;

	virtual AABB get_item_rect() const;
	real_t get_margin() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionPolygon3D();
};

// --- End scene/3d/physics/collision_polygon_3d.h ---

#include "scene/3d/skeleton_3d.h"
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
// --- Inlined from scene/3d/physics/ray_cast_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/material.h"

class ArrayMesh;

class RayCast3D : public Node3D
{
	bool enabled = true;
	bool collided = false;
	RID against_rid;
	int against_shape = 0;
	Vector3 collision_point;
	Vector3 collision_normal;
	int collision_face_index = -1;

	Vector3 target_position = Vector3(0, -1, 0);
	HashSet<RID> exclude;

	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;

	Ref<Material> debug_material;
	Color debug_shape_custom_color = Color(0.0, 0.0, 0.0);
	int debug_shape_thickness = 2;
	Vector<Vector3> debug_shape_vertices;
	Vector<Vector3> debug_line_vertices;

	void _create_debug_shape();
	void _update_debug_shape();
	void _update_debug_shape_material(bool p_check_collision = false);
	void _update_debug_shape_vertices();
	void _clear_debug_shape();

	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	bool hit_from_inside = false;
	bool hit_back_faces = true;

	RID debug_instance;
	Ref<ArrayMesh> debug_mesh;

protected:
	void _notification(int p_what);
	void _update_raycast_state();

public:
	void set_collide_with_areas(bool p_enabled);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_enabled);
	bool is_collide_with_bodies_enabled() const;

	void set_hit_from_inside(bool p_enabled);
	bool is_hit_from_inside_enabled() const;

	void set_hit_back_faces(bool p_enabled);
	bool is_hit_back_faces_enabled() const;

	bool is_enabled() const;

	Vector3 get_target_position() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	const Color& get_debug_shape_custom_color() const;
	void set_debug_shape_custom_color(const Color& p_color);

	const Vector<Vector3>& get_debug_shape_vertices() const;
	const Vector<Vector3>& get_debug_line_vertices() const;

	Ref<StandardMaterial3D> get_debug_material();

	int get_debug_shape_thickness() const;

	void force_raycast_update();
	bool is_colliding() const;
	RID get_collider_rid() const;
	int get_collider_shape() const;
	Vector3 get_collision_point() const;
	Vector3 get_collision_normal() const;
	int get_collision_face_index() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node3D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void remove_exception(const Node3D* rp_node);
	void clear_exceptions();

	RayCast3D() = default;
};



// --- End scene/3d/physics/ray_cast_3d.h ---
#include "scene/3d/skeleton_3d.h"
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
#include "scene/3d/skeleton_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/3d/physics/shape_cast_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/material.h"

class ShapeCast3D : public Node3D
{
	bool enabled = true;
#ifndef DISABLE_DEPRECATED
	void resource_changed(Ref<Resource> p_res);
#endif

	Ref<Shape3D> shape;
	RID shape_rid;
	Vector3 target_position = Vector3(0, -1, 0);

	HashSet<RID> exclude;
	real_t margin = 0.0;
	uint32_t collision_mask = 1;
	bool exclude_parent_body = true;
	bool collide_with_areas = false;
	bool collide_with_bodies = true;

	Ref<Material> debug_material;
	Color debug_shape_custom_color = Color(0.0, 0.0, 0.0);
	Vector<Vector3> debug_shape_vertices;
	Vector<Vector3> debug_line_vertices;

	void _create_debug_shape();
	void _update_debug_shape();
	void _update_debug_shape_material(bool p_check_collision = false);
	void _update_debug_shape_vertices();
	void _clear_debug_shape();

	// Result
	int max_results = 32;
	Vector<PS3DT::ShapeRestInfo> result;
	bool collided = false;
	real_t collision_safe_fraction = 1.0;
	real_t collision_unsafe_fraction = 1.0;

	RID debug_instance;
	Ref<ArrayMesh> debug_mesh;

protected:
	void _notification(int p_what);
	void _update_shapecast_state();

public:
	void set_collide_with_areas(bool p_clip);
	bool is_collide_with_areas_enabled() const;

	void set_collide_with_bodies(bool p_clip);
	bool is_collide_with_bodies_enabled() const;

	bool is_enabled() const;

	void set_shape(const Ref<Shape3D>& p_shape);
	Ref<Shape3D> get_shape() const;

	Vector3 get_target_position() const;

	void set_margin(real_t p_margin);
	real_t get_margin() const;

	void set_max_results(int p_max_results);
	int get_max_results() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_exclude_parent_body(bool p_exclude_parent_body);
	bool get_exclude_parent_body() const;

	const Color& get_debug_shape_custom_color() const;
	void set_debug_shape_custom_color(const Color& p_color);

	const Vector<Vector3>& get_debug_shape_vertices() const;
	const Vector<Vector3>& get_debug_line_vertices() const;

	Ref<StandardMaterial3D> get_debug_material();

	int get_collision_count() const;
	RID get_collider_rid(int p_idx) const;
	int get_collider_shape(int p_idx) const;
	Vector3 get_collision_point(int p_idx) const;
	Vector3 get_collision_normal(int p_idx) const;

	real_t get_closest_collision_safe_fraction() const;
	real_t get_closest_collision_unsafe_fraction() const;

	void force_shapecast_update();
	bool is_colliding() const;

	void add_exception_rid(const RID& p_rid);
	void add_exception(const Node3D* rp_node);
	void remove_exception_rid(const RID& p_rid);
	void remove_exception(const Node3D* rp_node);
	void clear_exceptions();

	virtual PackedStringArray get_configuration_warnings() const override;
};



// --- End scene/3d/physics/shape_cast_3d.h ---

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
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
// --- Inlined from scene/3d/physics/spring_arm_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/shape_3d.h"

class SpringArm3D : public Node3D
{
	Ref<Shape3D> shape;
	HashSet<RID> excluded_objects;
	real_t spring_length = 1.0;
	real_t current_spring_length = 0.0;
	uint32_t mask = 1;
	real_t margin = 0.01;

protected:
	void _notification(int p_what);

public:
	real_t get_length() const;
	void set_shape(Ref<Shape3D> p_shape);
	Ref<Shape3D> get_shape() const;
	void set_mask(uint32_t p_mask);
	uint32_t get_mask();
	void add_excluded_object(RID p_rid);
	bool remove_excluded_object(RID p_rid);
	void clear_excluded_objects();
	real_t get_hit_length();
	void set_margin(real_t p_margin);
	real_t get_margin();

private:
	void process_spring();
};



// --- End scene/3d/physics/spring_arm_3d.h ---

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

// ============================================================================
// Merged from scene/3d/physics/joints/joint_3d.cpp
// ============================================================================
// --- Inlined from scene/3d/physics/joints/joint_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"

class Joint3D : public Node3D {
public:
	enum JointType {
		JOINT_PIN,
		JOINT_HINGE,
		JOINT_SLIDER,
		JOINT_CONE_TWIST,
		JOINT_6DOF,
	};

	using PinParam = PS3DE::PinJointParam;
	using HingeParam = PS3DE::HingeJointParam;
	using HingeFlag = PS3DE::HingeJointFlag;
	using SliderParam = PS3DE::SliderJointParam;
	using ConeTwistParam = PS3DE::ConeTwistJointParam;
	using G6DOFParam = PS3DE::G6DOFJointAxisParam;
	using G6DOFFlag = PS3DE::G6DOFJointAxisFlag;

private:
	JointType joint_type = JOINT_PIN;
	RID ba, bb;
	RID joint;
	NodePath a;
	NodePath b;
	int solver_priority = 1;
	bool exclude_from_collision = true;
	String warning;
	bool configured = false;

	// Pin parameters.
	real_t pin_params[3] = { 0.3f, 1.0f, 0.0f };

	// Hinge parameters & flags.
	real_t hinge_params[PS3DE::HINGE_JOINT_MAX] = {};
	bool hinge_flags[PS3DE::HINGE_JOINT_FLAG_MAX] = {};

	// Slider parameters.
	real_t slider_params[PS3DE::SLIDER_JOINT_MAX] = {};

	// Cone twist parameters.
	static constexpr int CONE_TWIST_PARAM_MAX = 5;
	real_t cone_params[CONE_TWIST_PARAM_MAX] = {};

	// Generic 6DOF parameters & flags.
	real_t g6dof_params[3][PS3DE::G6DOF_JOINT_MAX] = {};
	bool g6dof_flags[3][PS3DE::G6DOF_JOINT_FLAG_MAX] = {};
	bool linear_drive_force_limit_set[3] = {};
	bool angular_drive_torque_limit_set[3] = {};
	Quaternion angular_target_rotation;
	bool has_angular_target_rotation = false;

	void _init_defaults();
	void _disconnect_signals();
	void _update_joint(bool p_only_free = false);
	void _configure_joint(RID p_joint, PhysicsBody3D *body_a, PhysicsBody3D *body_b);
	bool _should_replay_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const;

protected:
	void _notification(int p_what);
	_FORCE_INLINE_ bool is_configured() const { return configured; }

public:
	Joint3D(JointType p_type = JOINT_PIN);
	~Joint3D();

	void set_joint_type(JointType p_type);
	JointType get_joint_type() const { return joint_type; }

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_node_a(const NodePath &p_node_a);
	NodePath get_node_a() const { return a; }

	void set_node_b(const NodePath &p_node_b);
	NodePath get_node_b() const { return b; }

	void set_solver_priority(int p_priority);
	int get_solver_priority() const { return solver_priority; }

	void set_exclude_nodes_from_collision(bool p_enable);
	bool get_exclude_nodes_from_collision() const { return exclude_from_collision; }

	// Pin API.
	void set_pin_param(PinParam p_param, real_t p_value);
	real_t get_pin_param(PinParam p_param) const;

	// Hinge API.
	void set_hinge_param(HingeParam p_param, real_t p_value);
	real_t get_hinge_param(HingeParam p_param) const;
	void set_hinge_flag(HingeFlag p_flag, bool p_enabled);
	bool get_hinge_flag(HingeFlag p_flag) const;

	// Slider API.
	void set_slider_param(SliderParam p_param, real_t p_value);
	real_t get_slider_param(SliderParam p_param) const;

	// ConeTwist API.
	void set_cone_twist_param(ConeTwistParam p_param, real_t p_value);
	real_t get_cone_twist_param(ConeTwistParam p_param) const;

	// Generic 6DOF API.
	void set_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param, real_t p_value);
	real_t get_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const;
	void set_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag, bool p_enabled);
	bool get_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag) const;

	void set_param_x(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_X, p_param, p_value); }
	real_t get_param_x(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_X, p_param); }
	void set_param_y(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_Y, p_param, p_value); }
	real_t get_param_y(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_Y, p_param); }
	void set_param_z(G6DOFParam p_param, real_t p_value) { set_g6dof_param(Vector3::AXIS_Z, p_param, p_value); }
	real_t get_param_z(G6DOFParam p_param) const { return get_g6dof_param(Vector3::AXIS_Z, p_param); }

	void set_flag_x(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_X, p_flag, p_enabled); }
	bool get_flag_x(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_X, p_flag); }
	void set_flag_y(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_Y, p_flag, p_enabled); }
	bool get_flag_y(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_Y, p_flag); }
	void set_flag_z(G6DOFFlag p_flag, bool p_enabled) { set_g6dof_flag(Vector3::AXIS_Z, p_flag, p_enabled); }
	bool get_flag_z(G6DOFFlag p_flag) const { return get_g6dof_flag(Vector3::AXIS_Z, p_flag); }

	void set_angular_target_rotation(const Quaternion &p_target_rotation);
	Quaternion get_angular_target_rotation() const;
	bool has_target_rotation() const { return has_angular_target_rotation; }
	void clear_angular_target_rotation();

	RID get_rid() const { return joint; }
};

// --- End scene/3d/physics/joints/joint_3d.h ---

void Joint3D::_init_defaults() {
	pin_params[PS3DE::PIN_JOINT_BIAS] = 0.3f;
	pin_params[PS3DE::PIN_JOINT_DAMPING] = 1.0f;
	pin_params[PS3DE::PIN_JOINT_IMPULSE_CLAMP] = 0.0f;

	hinge_params[PS3DE::HINGE_JOINT_BIAS] = 0.3f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_UPPER] = Math::PI * 0.5f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_LOWER] = -Math::PI * 0.5f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_BIAS] = 0.3f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_SOFTNESS] = 0.9f;
	hinge_params[PS3DE::HINGE_JOINT_LIMIT_RELAXATION] = 1.0f;
	hinge_params[PS3DE::HINGE_JOINT_MOTOR_TARGET_VELOCITY] = 1.0f;
	hinge_params[PS3DE::HINGE_JOINT_MOTOR_MAX_IMPULSE] = 1.0f;
	hinge_flags[PS3DE::HINGE_JOINT_FLAG_USE_LIMIT] = false;
	hinge_flags[PS3DE::HINGE_JOINT_FLAG_ENABLE_MOTOR] = false;

	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_UPPER] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_LOWER] = -1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_LIMIT_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_MOTION_DAMPING] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_LINEAR_ORTHOGONAL_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_UPPER] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_LOWER] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_LIMIT_DAMPING] = 0.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_MOTION_DAMPING] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_SOFTNESS] = 1.0f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_RESTITUTION] = 0.7f;
	slider_params[PS3DE::SLIDER_JOINT_ANGULAR_ORTHOGONAL_DAMPING] = 1.0f;

	cone_params[PS3DE::CONE_TWIST_JOINT_SWING_SPAN] = Math::PI * 0.25f;
	cone_params[PS3DE::CONE_TWIST_JOINT_TWIST_SPAN] = Math::PI;
	cone_params[PS3DE::CONE_TWIST_JOINT_BIAS] = 0.3f;
	cone_params[PS3DE::CONE_TWIST_JOINT_SOFTNESS] = 0.8f;
	cone_params[PS3DE::CONE_TWIST_JOINT_RELAXATION] = 1.0f;

	for (int ax = 0; ax < 3; ax++) {
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_LIMIT_SOFTNESS] = 0.7f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_RESTITUTION] = 0.5f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_LINEAR_DAMPING] = 1.0f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_LIMIT_SOFTNESS] = 0.5f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_DAMPING] = 1.0f;
		g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_ERP] = 0.5f;
		g6dof_flags[ax][PS3DE::G6DOF_JOINT_FLAG_ENABLE_LINEAR_LIMIT] = true;
		g6dof_flags[ax][PS3DE::G6DOF_JOINT_FLAG_ENABLE_ANGULAR_LIMIT] = true;
	}
}

Joint3D::Joint3D(JointType p_type) :
		joint_type(p_type) {
	_init_defaults();
	set_notify_transform(true);
	joint = PhysicsServer3D::joint_create();
}

Joint3D::~Joint3D() {
	if (joint.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(joint);
	}
}

void Joint3D::set_joint_type(JointType p_type) {
	if (joint_type == p_type) {
		return;
	}
	joint_type = p_type;
	_update_joint();
}

void Joint3D::_disconnect_signals() {}

void Joint3D::_update_joint(bool p_only_free) {
	if (configured) {
		PhysicsServer3D::joint_clear(joint);
		ba = RID();
		bb = RID();
		configured = false;
	}

	if (p_only_free || !is_inside_tree()) {
		warning = String();
		return;
	}

	Node *node_a = a.is_empty() ? nullptr : get_node_or_null(a);
	Node *node_b = b.is_empty() ? nullptr : get_node_or_null(b);

	PhysicsBody3D *body_a = dynamic_cast<PhysicsBody3D *>(node_a);
	PhysicsBody3D *body_b = dynamic_cast<PhysicsBody3D *>(node_b);

	if (!body_a && body_b) {
		SWAP(body_a, body_b);
	}

	if (!body_a) {
		warning = RTR("Node A and Node B must be PhysicsBody3Ds");
		update_configuration_warnings();
		return;
	}

	if (body_a == body_b) {
		warning = RTR("Node A and Node B must be different PhysicsBody3Ds");
		update_configuration_warnings();
		return;
	}

	warning = String();
	update_configuration_warnings();

	ba = body_a->get_rid();
	bb = body_b ? body_b->get_rid() : RID();

	_configure_joint(joint, body_a, body_b);
	PhysicsServer3D::joint_set_solver_priority(joint, solver_priority);
	PhysicsServer3D::joint_disable_collisions_between_bodies(joint, exclude_from_collision);
	configured = true;
}

bool Joint3D::_should_replay_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const {
	if (p_param == PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT) {
		return linear_drive_force_limit_set[p_axis];
	}
	if (p_param == PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT) {
		return angular_drive_torque_limit_set[p_axis];
	}
	return true;
}

void Joint3D::_configure_joint(RID p_joint, PhysicsBody3D *body_a, PhysicsBody3D *body_b) {
	Transform3D gt = get_global_transform();

	if (joint_type == JOINT_PIN) {
		Vector3 pinpos = gt.origin;
		Vector3 local_a = body_a->to_local(pinpos);
		Vector3 local_b = body_b ? body_b->to_local(pinpos) : pinpos;
		PhysicsServer3D::joint_make_pin(p_joint, body_a->get_rid(), local_a, body_b ? body_b->get_rid() : RID(), local_b);
		for (int i = 0; i < 3; i++) {
			PhysicsServer3D::pin_joint_set_param(p_joint, PinParam(i), pin_params[i]);
		}
		return;
	}

	Transform3D local_a = (body_a->get_global_transform().affine_inverse() * gt).orthonormalized();
	Transform3D local_b = (body_b ? (body_b->get_global_transform().affine_inverse() * gt) : gt).orthonormalized();
	RID rid_b = body_b ? body_b->get_rid() : RID();

	switch (joint_type) {
		case JOINT_HINGE: {
			PhysicsServer3D::joint_make_hinge(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < PS3DE::HINGE_JOINT_MAX; i++) {
				PhysicsServer3D::hinge_joint_set_param(p_joint, HingeParam(i), hinge_params[i]);
			}
			for (int i = 0; i < PS3DE::HINGE_JOINT_FLAG_MAX; i++) {
				PhysicsServer3D::hinge_joint_set_flag(p_joint, HingeFlag(i), hinge_flags[i]);
			}
		} break;
		case JOINT_SLIDER: {
			PhysicsServer3D::joint_make_slider(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < PS3DE::SLIDER_JOINT_MAX; i++) {
				PhysicsServer3D::slider_joint_set_param(p_joint, SliderParam(i), slider_params[i]);
			}
		} break;
		case JOINT_CONE_TWIST: {
			PhysicsServer3D::joint_make_cone_twist(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int i = 0; i < CONE_TWIST_PARAM_MAX; i++) {
				PhysicsServer3D::cone_twist_joint_set_param(p_joint, ConeTwistParam(i), cone_params[i]);
			}
		} break;
		case JOINT_6DOF: {
			PhysicsServer3D::joint_make_generic_6dof(p_joint, body_a->get_rid(), local_a, rid_b, local_b);
			for (int ax = 0; ax < 3; ax++) {
				Vector3::Axis axis = Vector3::Axis(ax);
				for (int i = 0; i < PS3DE::G6DOF_JOINT_MAX; i++) {
					G6DOFParam param = G6DOFParam(i);
					if (_should_replay_g6dof_param(axis, param)) {
						PhysicsServer3D::generic_6dof_joint_set_param(p_joint, axis, param, g6dof_params[ax][i]);
					}
				}
				for (int i = 0; i < PS3DE::G6DOF_JOINT_FLAG_MAX; i++) {
					PhysicsServer3D::generic_6dof_joint_set_flag(p_joint, axis, G6DOFFlag(i), g6dof_flags[ax][i]);
				}
			}
			if (has_angular_target_rotation) {
				PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(p_joint, angular_target_rotation);
			}
		} break;
		default:
			break;
	}
}

void Joint3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint();
		} break;
		case NOTIFICATION_TRANSFORM_CHANGED: {
			if (is_inside_tree()) {
				_update_joint();
			}
		} break;
		case NOTIFICATION_EXIT_TREE: {
			if (is_configured()) {
				_disconnect_signals();
			}
			_update_joint(true);
		} break;
	}
}

void Joint3D::set_node_a(const NodePath &p_node_a) {
	if (a == p_node_a) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	a = p_node_a;
	_update_joint();
}

void Joint3D::set_node_b(const NodePath &p_node_b) {
	if (b == p_node_b) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	b = p_node_b;
	_update_joint();
}

void Joint3D::set_solver_priority(int p_priority) {
	solver_priority = p_priority;
	if (joint.is_valid()) {
		PhysicsServer3D::joint_set_solver_priority(joint, solver_priority);
	}
}

void Joint3D::set_exclude_nodes_from_collision(bool p_enable) {
	if (exclude_from_collision == p_enable) {
		return;
	}
	if (is_configured()) {
		_disconnect_signals();
	}
	_update_joint(true);
	exclude_from_collision = p_enable;
	_update_joint();
}

void Joint3D::set_pin_param(PinParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, 3);
	pin_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_PIN) {
		PhysicsServer3D::pin_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_pin_param(PinParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, 3, 0.0f);
	return pin_params[p_param];
}

void Joint3D::set_hinge_param(HingeParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, PS3DE::HINGE_JOINT_MAX);
	hinge_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_HINGE) {
		PhysicsServer3D::hinge_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_hinge_param(HingeParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, PS3DE::HINGE_JOINT_MAX, 0.0f);
	return hinge_params[p_param];
}

void Joint3D::set_hinge_flag(HingeFlag p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_flag, PS3DE::HINGE_JOINT_FLAG_MAX);
	hinge_flags[p_flag] = p_enabled;
	if (is_configured() && joint_type == JOINT_HINGE) {
		PhysicsServer3D::hinge_joint_set_flag(joint, p_flag, p_enabled);
	}
}

bool Joint3D::get_hinge_flag(HingeFlag p_flag) const {
	ERR_FAIL_INDEX_V(p_flag, PS3DE::HINGE_JOINT_FLAG_MAX, false);
	return hinge_flags[p_flag];
}

void Joint3D::set_slider_param(SliderParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, PS3DE::SLIDER_JOINT_MAX);
	slider_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_SLIDER) {
		PhysicsServer3D::slider_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_slider_param(SliderParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, PS3DE::SLIDER_JOINT_MAX, 0.0f);
	return slider_params[p_param];
}

void Joint3D::set_cone_twist_param(ConeTwistParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_param, CONE_TWIST_PARAM_MAX);
	cone_params[p_param] = p_value;
	if (is_configured() && joint_type == JOINT_CONE_TWIST) {
		PhysicsServer3D::cone_twist_joint_set_param(joint, p_param, p_value);
	}
}

real_t Joint3D::get_cone_twist_param(ConeTwistParam p_param) const {
	ERR_FAIL_INDEX_V(p_param, CONE_TWIST_PARAM_MAX, 0.0f);
	return cone_params[p_param];
}

void Joint3D::set_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param, real_t p_value) {
	ERR_FAIL_INDEX(p_axis, 3);
	ERR_FAIL_INDEX(p_param, PS3DE::G6DOF_JOINT_MAX);
	g6dof_params[p_axis][p_param] = p_value;
	if (p_param == PS3DE::G6DOF_JOINT_LINEAR_DRIVE_FORCE_LIMIT) {
		linear_drive_force_limit_set[p_axis] = true;
	} else if (p_param == PS3DE::G6DOF_JOINT_ANGULAR_DRIVE_TORQUE_LIMIT) {
		angular_drive_torque_limit_set[p_axis] = true;
	}
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_param(joint, p_axis, p_param, p_value);
	}
}

real_t Joint3D::get_g6dof_param(Vector3::Axis p_axis, G6DOFParam p_param) const {
	ERR_FAIL_INDEX_V(p_axis, 3, 0.0f);
	ERR_FAIL_INDEX_V(p_param, PS3DE::G6DOF_JOINT_MAX, 0.0f);
	return g6dof_params[p_axis][p_param];
}

void Joint3D::set_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag, bool p_enabled) {
	ERR_FAIL_INDEX(p_axis, 3);
	ERR_FAIL_INDEX(p_flag, PS3DE::G6DOF_JOINT_FLAG_MAX);
	g6dof_flags[p_axis][p_flag] = p_enabled;
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_flag(joint, p_axis, p_flag, p_enabled);
	}
}

bool Joint3D::get_g6dof_flag(Vector3::Axis p_axis, G6DOFFlag p_flag) const {
	ERR_FAIL_INDEX_V(p_axis, 3, false);
	ERR_FAIL_INDEX_V(p_flag, PS3DE::G6DOF_JOINT_FLAG_MAX, false);
	return g6dof_flags[p_axis][p_flag];
}

void Joint3D::set_angular_target_rotation(const Quaternion &p_target_rotation) {
	ERR_FAIL_COND_MSG(!(p_target_rotation.is_finite() && p_target_rotation.length_squared() > CMP_EPSILON),
			"Angular target rotation must be a finite, non-zero quaternion.");
	angular_target_rotation = p_target_rotation.normalized();
	has_angular_target_rotation = true;
	if (is_configured() && joint_type == JOINT_6DOF) {
		PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(joint, angular_target_rotation);
	}
}

Quaternion Joint3D::get_angular_target_rotation() const {
	if (has_angular_target_rotation) {
		return angular_target_rotation;
	}
	if (is_configured() && joint_type == JOINT_6DOF) {
		return PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(joint);
	}
	return Quaternion();
}

void Joint3D::clear_angular_target_rotation() {
	if (!has_angular_target_rotation) {
		return;
	}
	has_angular_target_rotation = false;
	angular_target_rotation = Quaternion();
	if (is_configured() && joint_type == JOINT_6DOF) {
		for (int ax = 0; ax < 3; ax++) {
			PhysicsServer3D::generic_6dof_joint_set_param(joint, Vector3::Axis(ax),
					PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT,
					g6dof_params[ax][PS3DE::G6DOF_JOINT_ANGULAR_SPRING_EQUILIBRIUM_POINT]);
		}
	}
}

PackedStringArray Joint3D::get_configuration_warnings() const {
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (!warning.is_empty()) {
		warnings.push_back(warning);
	}
	return warnings;
}

// ============================================================================
// Merged from scene/3d/physics/soft_body_3d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "scene/3d/skeleton_3d.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/3d/physics/soft_body_3d.h ---
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

class MeshInstance3D;
class PhysicsBody3D;

class SoftBody3D : public Node3D
{
public:
	enum DisableMode
	{
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct PinnedPoint
	{
		int point_index = -1;
		NodePath spatial_attachment_path;
		Node3D* spatial_attachment = nullptr; // Cache
		Vector3 offset;

		PinnedPoint();
		PinnedPoint(const PinnedPoint& obj_tocopy);
		void operator=(const PinnedPoint& obj);
	};

	RenderInstance3D render_instance;

private:
	Ref<Mesh> mesh;
	PhysicsServer3DRenderingServerHandler* rendering_server_handler = nullptr;

	RID physics_rid;

	DisableMode disable_mode = DISABLE_MODE_REMOVE;

	RID owned_mesh;
	uint32_t collision_mask = 1;
	uint32_t collision_layer = 1;
	NodePath parent_collision_ignore;
	Vector<PinnedPoint> pinned_points;
	bool simulation_started = false;
	bool pinned_points_cache_dirty = true;

	Ref<ArrayMesh> debug_mesh_cache;
	MeshInstance3D* debug_mesh = nullptr;

	bool capture_input_on_drag = false;
	bool ray_pickable = true;

	void _update_pickable();

	void _update_physics_server();
	void _draw_soft_mesh();

	void _prepare_physics_server();
	void _become_mesh_owner();

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;
	PackedStringArray get_configuration_warnings() const override;

public:
	virtual bool has_render_instance() const override { return true; }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const { return mesh; }
	AABB get_aabb() const { return mesh.is_valid() ? mesh->get_aabb() : AABB(); }

	RID get_physics_rid() const { return physics_rid; }

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_disable_mode(DisableMode p_mode);
	DisableMode get_disable_mode() const;

	void set_parent_collision_ignore(const NodePath& p_parent_collision_ignore);
	const NodePath& get_parent_collision_ignore() const;
	int get_point_count() const;

	void set_pinned_points_indices(Vector<PinnedPoint> p_pinned_points_indices);
	Vector<PinnedPoint> get_pinned_points_indices();

	void set_simulation_precision(int p_simulation_precision);
	int get_simulation_precision();

	void set_total_mass(real_t p_total_mass);
	real_t get_total_mass();

	void set_linear_stiffness(real_t p_linear_stiffness);
	real_t get_linear_stiffness();

	void set_shrinking_factor(real_t p_shrinking_factor);
	real_t get_shrinking_factor();

	void set_pressure_coefficient(real_t p_pressure_coefficient);
	real_t get_pressure_coefficient();

	void set_damping_coefficient(real_t p_damping_coefficient);
	real_t get_damping_coefficient();

	void set_drag_coefficient(real_t p_drag_coefficient);
	real_t get_drag_coefficient();

	void add_collision_exception_with(Node* rp_node);
	void remove_collision_exception_with(Node* rp_node);

	Vector3 get_point_transform(int p_point_index);

	void pin_point(int p_point_index, bool pin,
		const NodePath& p_spatial_attachment_path = NodePath(), int p_insert_at = -1);
	bool is_point_pinned(int p_point_index) const;

	void _pin_point_deferred(int p_point_index, bool pin, const NodePath p_spatial_attachment_path);

	void set_ray_pickable(bool p_ray_pickable);
	bool is_ray_pickable() const;

	void apply_impulse(int p_point_index, const Vector3& p_impulse);
	void apply_force(int p_point_index, const Vector3& p_force);
	void apply_central_impulse(const Vector3& p_impulse);
	void apply_central_force(const Vector3& p_force);

	SoftBody3D();
	~SoftBody3D();

private:
	void _make_cache_dirty();
	void _update_cache_pin_points_datas();

	void _pin_point_on_physics_server(int p_point_index, bool pin);
	void _add_pinned_point(
		int p_point_index, const NodePath& p_spatial_attachment_path, int p_insert_at = -1);
	void _reset_points_offsets();

	void _remove_pinned_point(int p_point_index);
	int _get_pinned_point(int p_point_index, PinnedPoint*& r_point) const;
	int _has_pinned_point(int p_point_index) const;
};

// --- End scene/3d/physics/soft_body_3d.h ---

SoftBody3D::PinnedPoint::PinnedPoint() {}

SoftBody3D::PinnedPoint::PinnedPoint(const PinnedPoint& obj_tocopy)
{
	point_index = obj_tocopy.point_index;
	spatial_attachment_path = obj_tocopy.spatial_attachment_path;
	spatial_attachment = obj_tocopy.spatial_attachment;
	offset = obj_tocopy.offset;
}

void SoftBody3D::PinnedPoint::operator=(const PinnedPoint& obj)
{
	point_index = obj.point_index;
	spatial_attachment_path = obj.spatial_attachment_path;
	spatial_attachment = obj.spatial_attachment;
	offset = obj.offset;
}

void SoftBody3D::_update_pickable()
{
	if (!is_inside_tree()) {
		return;
	}
	bool pickable = ray_pickable && is_visible_in_tree();
	PhysicsServer3D::soft_body_set_ray_pickable(physics_rid, pickable);
}

void SoftBody3D::set_mesh(const Ref<Mesh>& p_mesh)
{
	mesh = p_mesh;
	render_instance.set_base(mesh.is_valid() ? mesh->get_rid() : RID());
	if (is_inside_tree()) {
		_prepare_physics_server();
	}
}

void SoftBody3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void SoftBody3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_WORLD: {
		if (Engine::is_editor_hint()) {
			// I have no idea what this is supposed to do, it's really weird
			// leaving for upcoming PK work on physics
			// add_change_receptor(this);
		}

		RID space = get_world_3d()->get_space();
		PhysicsServer3D::soft_body_set_space(physics_rid, space);
		_prepare_physics_server();
	} break;

	case NOTIFICATION_READY: {
		if (!parent_collision_ignore.is_empty()) {
			add_collision_exception_with(get_node(parent_collision_ignore));
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		if (Engine::is_editor_hint()) {
			_reset_points_offsets();
			return;
		}

		PhysicsServer3D::soft_body_set_transform(
			physics_rid, get_global_transform());

		set_notify_transform(false);
		// Required to be top level with Transform at center of world in order to modify
		// Renderer only to support custom Transform
		set_as_top_level(true);
		set_transform(Transform3D());
		set_notify_transform(true);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_pickable();
	} break;

	case NOTIFICATION_EXIT_WORLD: {
		PhysicsServer3D::soft_body_set_space(physics_rid, RID());
	} break;

	case NOTIFICATION_DISABLED: {
		if (is_inside_tree() && (disable_mode == DISABLE_MODE_REMOVE)) {
			_prepare_physics_server();
		}
	} break;

	case NOTIFICATION_ENABLED: {
		if (is_inside_tree() && (disable_mode == DISABLE_MODE_REMOVE)) {
			_prepare_physics_server();
		}
	} break;
	}
}

PackedStringArray SoftBody3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	render_instance.append_geometry_warnings(warnings);

	if (mesh.is_null()) {
		warnings.push_back(RTR("This body will be ignored until you set a mesh."));
	}

	return warnings;
}

void SoftBody3D::_update_physics_server()
{
	if (!simulation_started) {
		return;
	}

	_update_cache_pin_points_datas();
	// Submit bone attachment
	const int pinned_points_indices_size = pinned_points.size();
	const PinnedPoint* r = pinned_points.ptr();
	for (int i = 0; i < pinned_points_indices_size; ++i) {
		if (r[i].spatial_attachment) {
			PhysicsServer3D::soft_body_move_point(physics_rid, r[i].point_index,
				r[i].spatial_attachment->get_global_transform().xform(r[i].offset));
		}
	}
}

void SoftBody3D::set_collision_mask(uint32_t p_mask)
{
	collision_mask = p_mask;
	PhysicsServer3D::soft_body_set_collision_mask(physics_rid, p_mask);
}

uint32_t SoftBody3D::get_collision_mask() const { return collision_mask; }

void SoftBody3D::set_collision_layer(uint32_t p_layer)
{
	collision_layer = p_layer;
	PhysicsServer3D::soft_body_set_collision_layer(physics_rid, p_layer);
}

uint32_t SoftBody3D::get_collision_layer() const { return collision_layer; }

void SoftBody3D::set_collision_layer_value(int p_layer_number, bool p_value)
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

bool SoftBody3D::get_collision_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void SoftBody3D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool SoftBody3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

void SoftBody3D::set_disable_mode(DisableMode p_mode)
{
	if (disable_mode == p_mode) {
		return;
	}

	disable_mode = p_mode;

	if (mesh.is_valid() && is_inside_tree() && !is_enabled()) {
		_prepare_physics_server();
	}
}

SoftBody3D::DisableMode SoftBody3D::get_disable_mode() const { return disable_mode; }

void SoftBody3D::set_parent_collision_ignore(const NodePath& p_parent_collision_ignore)
{
	parent_collision_ignore = p_parent_collision_ignore;
}

const NodePath& SoftBody3D::get_parent_collision_ignore() const { return parent_collision_ignore; }

void SoftBody3D::set_pinned_points_indices(Vector<SoftBody3D::PinnedPoint> p_pinned_points_indices)
{
	pinned_points = p_pinned_points_indices;
	for (int i = pinned_points.size() - 1; 0 <= i; --i) {
		pin_point(p_pinned_points_indices[i].point_index, true);
	}
}

Vector<SoftBody3D::PinnedPoint> SoftBody3D::get_pinned_points_indices() { return pinned_points; }

int SoftBody3D::get_simulation_precision()
{
	return PhysicsServer3D::soft_body_get_simulation_precision(physics_rid);
}

void SoftBody3D::set_simulation_precision(int p_simulation_precision)
{
	PhysicsServer3D::soft_body_set_simulation_precision(
		physics_rid, p_simulation_precision);
}

real_t SoftBody3D::get_total_mass()
{
	return PhysicsServer3D::soft_body_get_total_mass(physics_rid);
}

void SoftBody3D::set_total_mass(real_t p_total_mass)
{
	PhysicsServer3D::soft_body_set_total_mass(physics_rid, p_total_mass);
}

void SoftBody3D::set_linear_stiffness(real_t p_linear_stiffness)
{
	PhysicsServer3D::soft_body_set_linear_stiffness(
		physics_rid, p_linear_stiffness);
}

real_t SoftBody3D::get_linear_stiffness()
{
	return PhysicsServer3D::soft_body_get_linear_stiffness(physics_rid);
}

void SoftBody3D::set_shrinking_factor(real_t p_shrinking_factor)
{
	PhysicsServer3D::soft_body_set_shrinking_factor(
		physics_rid, p_shrinking_factor);
}

real_t SoftBody3D::get_shrinking_factor()
{
	return PhysicsServer3D::soft_body_get_shrinking_factor(physics_rid);
}

real_t SoftBody3D::get_pressure_coefficient()
{
	return PhysicsServer3D::soft_body_get_pressure_coefficient(physics_rid);
}

void SoftBody3D::set_pressure_coefficient(real_t p_pressure_coefficient)
{
	PhysicsServer3D::soft_body_set_pressure_coefficient(
		physics_rid, p_pressure_coefficient);
}

real_t SoftBody3D::get_damping_coefficient()
{
	return PhysicsServer3D::soft_body_get_damping_coefficient(physics_rid);
}

void SoftBody3D::set_damping_coefficient(real_t p_damping_coefficient)
{
	PhysicsServer3D::soft_body_set_damping_coefficient(
		physics_rid, p_damping_coefficient);
}

real_t SoftBody3D::get_drag_coefficient()
{
	return PhysicsServer3D::soft_body_get_drag_coefficient(physics_rid);
}

void SoftBody3D::set_drag_coefficient(real_t p_drag_coefficient)
{
	PhysicsServer3D::soft_body_set_drag_coefficient(
		physics_rid, p_drag_coefficient);
}

Vector3 SoftBody3D::get_point_transform(int p_point_index)
{
	return PhysicsServer3D::soft_body_get_point_global_position(
		physics_rid, p_point_index);
}

int SoftBody3D::get_point_count() const
{
	if (mesh.is_null()) {
		return 0;
	}

	const int vertex_count = mesh->surface_get_array_len(0);
	ERR_FAIL_COND_V(vertex_count < 0, 0);
	return vertex_count;
}

void SoftBody3D::apply_impulse(int p_point_index, const Vector3& p_impulse)
{
	PhysicsServer3D::soft_body_apply_point_impulse(
		physics_rid, p_point_index, p_impulse);
}

void SoftBody3D::apply_force(int p_point_index, const Vector3& p_force)
{
	PhysicsServer3D::soft_body_apply_point_force(
		physics_rid, p_point_index, p_force);
}

void SoftBody3D::apply_central_impulse(const Vector3& p_impulse)
{
	PhysicsServer3D::soft_body_apply_central_impulse(physics_rid, p_impulse);
}

void SoftBody3D::apply_central_force(const Vector3& p_force)
{
	PhysicsServer3D::soft_body_apply_central_force(physics_rid, p_force);
}

void SoftBody3D::pin_point(
	int p_point_index, bool pin, const NodePath& p_spatial_attachment_path, int p_insert_at)
{
	ERR_FAIL_COND_MSG(p_insert_at < -1 || p_insert_at >= pinned_points.size(),
		"Invalid index for pin point insertion position.");
	_pin_point_on_physics_server(p_point_index, pin);
	if (pin) {
		_add_pinned_point(p_point_index, p_spatial_attachment_path, p_insert_at);
	}
	else {
		_remove_pinned_point(p_point_index);
	}
}

void SoftBody3D::_pin_point_deferred(
	int p_point_index, bool pin, const NodePath p_spatial_attachment_path)
{
	pin_point(p_point_index, pin, p_spatial_attachment_path);
	_make_cache_dirty();
}

bool SoftBody3D::is_point_pinned(int p_point_index) const
{
	return -1 != _has_pinned_point(p_point_index);
}

void SoftBody3D::set_ray_pickable(bool p_ray_pickable)
{
	ray_pickable = p_ray_pickable;
	_update_pickable();
}

bool SoftBody3D::is_ray_pickable() const { return ray_pickable; }

SoftBody3D::SoftBody3D()
{
	rendering_server_handler = memnew(PhysicsServer3DRenderingServerHandler);
	if (PhysicsServer3D::is_initialized()) {
		physics_rid = PhysicsServer3D::soft_body_create();
	}
}

SoftBody3D::~SoftBody3D()
{
	memdelete(rendering_server_handler);
	if (physics_rid.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(physics_rid);
	}
}

void SoftBody3D::_make_cache_dirty() { pinned_points_cache_dirty = true; }

void SoftBody3D::_pin_point_on_physics_server(int p_point_index, bool pin)
{
	PhysicsServer3D::soft_body_pin_point(physics_rid, p_point_index, pin);
}

void SoftBody3D::_remove_pinned_point(int p_point_index)
{
	const int id(_has_pinned_point(p_point_index));
	if (-1 != id) {
		pinned_points.remove_at(id);
	}
}

int SoftBody3D::_get_pinned_point(int p_point_index, SoftBody3D::PinnedPoint*& r_point) const
{
	const int id = _has_pinned_point(p_point_index);
	if (-1 == id) {
		r_point = nullptr;
		return -1;
	}
	else {
		r_point = const_cast<SoftBody3D::PinnedPoint*>(&pinned_points.ptr()[id]);
		return id;
	}
}

int SoftBody3D::_has_pinned_point(int p_point_index) const
{
	const PinnedPoint* r = pinned_points.ptr();
	for (int i = pinned_points.size() - 1; 0 <= i; --i) {
		if (p_point_index == r[i].point_index) {
			return i;
		}
	}
	return -1;
}

void SoftBody3D::_prepare_physics_server() {}

void SoftBody3D::_update_cache_pin_points_datas() {}

void SoftBody3D::_reset_points_offsets() {}

void SoftBody3D::_add_pinned_point(int, NodePath const&, int) {}

void SoftBody3D::add_collision_exception_with(Node*) {}

// ============================================================================
// Merged from scene/resources/3d/shape_3d.cpp
// ============================================================================

#include "scene/resources/3d/shape_3d.h"

#include <cfloat> // FLT_MAX

#include "core/io/image.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

RID Shape3D::_create_shape_rid(ShapeType p_type)
{
	switch (p_type) {
	case SHAPE_WORLD_BOUNDARY:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_WORLD_BOUNDARY);
	case SHAPE_SEPARATION_RAY:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_SEPARATION_RAY);
	case SHAPE_SPHERE:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_SPHERE);
	case SHAPE_BOX:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_BOX);
	case SHAPE_CAPSULE:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CAPSULE);
	case SHAPE_CYLINDER:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CYLINDER);
	case SHAPE_CONVEX_POLYGON:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CONVEX_POLYGON);
	case SHAPE_CONCAVE_POLYGON:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_CONCAVE_POLYGON);
	case SHAPE_HEIGHTMAP:
		return PhysicsServer3D::shape_create(PS3DE::SHAPE_HEIGHTMAP);
	}
	return RID();
}

void Shape3D::_ensure_heightmap_initialized()
{
	if (shape_type == SHAPE_HEIGHTMAP && map_data.size() != map_width * map_depth) {
		map_data.resize(map_width * map_depth);
		real_t* w = map_data.ptrw();
		for (int i = 0; i < map_width * map_depth; i++) {
			w[i] = 0.0;
		}
	}
}

void Shape3D::set_shape_type(ShapeType p_type)
{
	if (shape_type == p_type) {
		return;
	}
	if (shape.is_valid() && PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::free_rid(shape);
	}
	shape_type = p_type;
	shape = _create_shape_rid(shape_type);
	_ensure_heightmap_initialized();
	if (custom_bias != 0.0) {
		PhysicsServer3D::shape_set_custom_solver_bias(shape, custom_bias);
	}
	if (margin != 0.04) {
		PhysicsServer3D::shape_set_margin(shape, margin);
	}
	_update_shape();
}

// World boundary

void Shape3D::set_plane(const Plane& p_plane)
{
	plane = p_plane;
	_update_shape();
}

const Plane& Shape3D::get_plane() const { return plane; }

// Separation ray

void Shape3D::set_length(float p_length)
{
	length = p_length;
	_update_shape();
}

float Shape3D::get_length() const { return length; }

void Shape3D::set_slide_on_slope(bool p_active)
{
	slide_on_slope = p_active;
	_update_shape();
}

bool Shape3D::get_slide_on_slope() const { return slide_on_slope; }

// Sphere / Capsule / Cylinder

void Shape3D::set_radius(float p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0f, "Shape3D radius cannot be negative.");
	radius = p_radius;
	if (shape_type == SHAPE_CAPSULE && height < radius * 2.0f) {
		height = radius * 2.0f;
	}
	_update_shape();
}

float Shape3D::get_radius() const { return radius; }

void Shape3D::set_height(float p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0f, "Shape3D height cannot be negative.");
	height = p_height;
	if (shape_type == SHAPE_CAPSULE && radius > height * 0.5f) {
		radius = height * 0.5f;
	}
	_update_shape();
}

float Shape3D::get_height() const { return height; }

void Shape3D::set_mid_height(real_t p_mid_height)
{
	ERR_FAIL_COND_MSG(p_mid_height < 0.0f, "Shape3D mid-height cannot be negative.");
	height = p_mid_height + radius * 2.0f;
	_update_shape();
}

real_t Shape3D::get_mid_height() const { return height - radius * 2.0f; }

// Box

void Shape3D::set_size(const Vector3& p_size)
{
	ERR_FAIL_COND_MSG(
		p_size.x < 0 || p_size.y < 0 || p_size.z < 0, "Shape3D size cannot be negative.");
	size = p_size;
	_update_shape();
}

Vector3 Shape3D::get_size() const { return size; }

// Convex polygon

void Shape3D::set_points(const Vector<Vector3>& p_points)
{
	points = p_points;
	_update_shape();
}

Vector<Vector3> Shape3D::get_points() const { return points; }

// Concave polygon

void Shape3D::set_faces(const Vector<Vector3>& p_faces)
{
	faces = p_faces;
	_update_shape();
}

Vector<Vector3> Shape3D::get_faces() const { return faces; }

void Shape3D::set_backface_collision_enabled(bool p_enabled)
{
	backface_collision = p_enabled;
	if (!faces.is_empty()) {
		_update_shape();
	}
}

bool Shape3D::is_backface_collision_enabled() const { return backface_collision; }

// Heightmap

void Shape3D::set_map_width(int p_new)
{
	if (p_new < 1) {
		// ignore
	}
	else if (map_width != p_new) {
		_ensure_heightmap_initialized();
		int was_size = map_width * map_depth;
		map_width = p_new;

		int new_size = map_width * map_depth;
		map_data.resize(new_size);

		real_t* w = map_data.ptrw();
		while (was_size < new_size) {
			w[was_size++] = 0.0;
		}

		_update_shape();
	}
}

int Shape3D::get_map_width() const { return map_width; }

void Shape3D::set_map_depth(int p_new)
{
	if (p_new < 1) {
		// ignore
	}
	else if (map_depth != p_new) {
		_ensure_heightmap_initialized();
		int was_size = map_width * map_depth;
		map_depth = p_new;

		int new_size = map_width * map_depth;
		map_data.resize(new_size);

		real_t* w = map_data.ptrw();
		while (was_size < new_size) {
			w[was_size++] = 0.0;
		}

		_update_shape();
	}
}

int Shape3D::get_map_depth() const { return map_depth; }

void Shape3D::set_map_data(Vector<real_t> p_new)
{
	int expected_size = (map_width * map_depth);
	if (p_new.size() != expected_size) {
		return;
	}

	map_data.resize(expected_size);
	real_t* w = map_data.ptrw();
	const real_t* r = p_new.ptr();
	for (int i = 0; i < expected_size; i++) {
		real_t val = r[i];
		w[i] = val;
		if (i == 0) {
			min_height = val;
			max_height = val;
		}
		else {
			if (min_height > val) {
				min_height = val;
			}
			if (max_height < val) {
				max_height = val;
			}
		}
	}

	_update_shape();
}

Vector<real_t> Shape3D::get_map_data() const { return map_data; }

real_t Shape3D::get_min_height() const { return min_height; }

real_t Shape3D::get_max_height() const { return max_height; }

void Shape3D::update_map_data_from_image(
	const Ref<Image>& p_image, real_t p_height_min, real_t p_height_max)
{
	ERR_FAIL_COND_MSG(
		p_image.is_null(), "Heightmap update image requires a valid Image reference.");
	ERR_FAIL_COND_MSG(p_image->get_format() != Image::FORMAT_RF &&
						  p_image->get_format() != Image::FORMAT_RH &&
						  p_image->get_format() != Image::FORMAT_R8,
		"Heightmap update image requires Image in format FORMAT_RF (32 bit), FORMAT_RH (16 bit), "
		"or FORMAT_R8 (8 bit).");
	ERR_FAIL_COND_MSG(
		p_image->get_width() < 2, "Heightmap update image requires a minimum Image width of 2.");
	ERR_FAIL_COND_MSG(
		p_image->get_height() < 2, "Heightmap update image requires a minimum Image height of 2.");
	ERR_FAIL_COND_MSG(p_height_min > p_height_max,
		"Heightmap update image requires height_max to be greater than height_min.");

	map_width = p_image->get_width();
	map_depth = p_image->get_height();
	map_data.resize(map_width * map_depth);

	real_t new_min_height = FLT_MAX;
	real_t new_max_height = -FLT_MAX;

	float remap_height_min = float(p_height_min);
	float remap_height_max = float(p_height_max);

	real_t* map_data_ptrw = map_data.ptrw();

	switch (p_image->get_format()) {
	case Image::FORMAT_RF: {
		const float* image_data_ptr = (float*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = image_data_ptr[i];

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	case Image::FORMAT_RH: {
		const uint16_t* image_data_ptr = (uint16_t*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = Math::half_to_float(image_data_ptr[i]);

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	case Image::FORMAT_R8: {
		const uint8_t* image_data_ptr = (uint8_t*)p_image->get_data().ptr();

		for (int i = 0; i < map_data.size(); i++) {
			float pixel_value = float(image_data_ptr[i] / 255.0);

			DEV_ASSERT(pixel_value >= 0.0 && pixel_value <= 1.0);

			real_t height_value =
				Math::remap(pixel_value, 0.0f, 1.0f, remap_height_min, remap_height_max);

			if (height_value < new_min_height) {
				new_min_height = height_value;
			}
			if (height_value > new_max_height) {
				new_max_height = height_value;
			}

			map_data_ptrw[i] = height_value;
		}

	} break;

	default: {
		return;
	}
	}

	min_height = new_min_height;
	max_height = new_max_height;

	_update_shape();
}

// Debug & bounds

Vector<Vector3> Shape3D::get_debug_mesh_lines() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY: {
		Plane p = plane;
		Vector3 n1 = p.get_any_perpendicular_normal();
		Vector3 n2 = p.normal.cross(n1).normalized();

		Vector3 pface[4] = {
			p.normal * p.d + n1 * 10.0 + n2 * 10.0,
			p.normal * p.d + n1 * 10.0 + n2 * -10.0,
			p.normal * p.d + n1 * -10.0 + n2 * -10.0,
			p.normal * p.d + n1 * -10.0 + n2 * 10.0,
		};

		Vector<Vector3> wb_points = {pface[0], pface[1], pface[1], pface[2], pface[2], pface[3],
			pface[3], pface[0], p.normal * p.d, p.normal * p.d + p.normal * 3};
		return wb_points;
	}
	case SHAPE_SEPARATION_RAY: {
		return {Vector3(), Vector3(0, 0, length)};
	}
	case SHAPE_SPHERE: {
		Vector<Vector3> sphere_points;
		for (int i = 0; i <= 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			sphere_points.push_back(Vector3(a.x, 0, a.y));
			sphere_points.push_back(Vector3(b.x, 0, b.y));
			sphere_points.push_back(Vector3(0, a.x, a.y));
			sphere_points.push_back(Vector3(0, b.x, b.y));
			sphere_points.push_back(Vector3(a.x, a.y, 0));
			sphere_points.push_back(Vector3(b.x, b.y, 0));
		}
		return sphere_points;
	}
	case SHAPE_BOX: {
		return Vector<Vector3>();
	}
	case SHAPE_CAPSULE: {
		Vector<Vector3> capsule_points;
		Vector3 d(0, height * 0.5f - radius, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			capsule_points.push_back(Vector3(a.x, 0, a.y) + d);
			capsule_points.push_back(Vector3(b.x, 0, b.y) + d);

			capsule_points.push_back(Vector3(a.x, 0, a.y) - d);
			capsule_points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				capsule_points.push_back(Vector3(a.x, 0, a.y) + d);
				capsule_points.push_back(Vector3(a.x, 0, a.y) - d);
			}

			Vector3 dud = i < 180 ? d : -d;

			capsule_points.push_back(Vector3(0, a.x, a.y) + dud);
			capsule_points.push_back(Vector3(0, b.x, b.y) + dud);
			capsule_points.push_back(Vector3(a.y, a.x, 0) + dud);
			capsule_points.push_back(Vector3(b.y, b.x, 0) + dud);
		}
		return capsule_points;
	}
	case SHAPE_CYLINDER: {
		Vector<Vector3> cylinder_points;
		Vector3 d(0, height * 0.5, 0);
		for (int i = 0; i < 360; i++) {
			float ra = Math::deg_to_rad((float)i);
			float rb = Math::deg_to_rad((float)i + 1);
			Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * radius;
			Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * radius;

			cylinder_points.push_back(Vector3(a.x, 0, a.y) + d);
			cylinder_points.push_back(Vector3(b.x, 0, b.y) + d);

			cylinder_points.push_back(Vector3(a.x, 0, a.y) - d);
			cylinder_points.push_back(Vector3(b.x, 0, b.y) - d);

			if (i % 90 == 0) {
				cylinder_points.push_back(Vector3(a.x, 0, a.y) + d);
				cylinder_points.push_back(Vector3(a.x, 0, a.y) - d);
			}
		}
		return cylinder_points;
	}
	case SHAPE_CONVEX_POLYGON: {
		return Vector<Vector3>();
	}
	case SHAPE_CONCAVE_POLYGON: {
		HashSet<DrawEdge, DrawEdge> edges;

		int index_count = faces.size();
		ERR_FAIL_COND_V((index_count % 3) != 0, Vector<Vector3>());

		const Vector3* r = faces.ptr();

		for (int i = 0; i < index_count; i += 3) {
			for (int j = 0; j < 3; j++) {
				DrawEdge de(r[i + j], r[i + ((j + 1) % 3)]);
				edges.insert(de);
			}
		}

		Vector<Vector3> concave_points;
		concave_points.resize(edges.size() * 2);
		int idx = 0;
		for (const DrawEdge& E : edges) {
			concave_points.write[idx + 0] = E.a;
			concave_points.write[idx + 1] = E.b;
			idx += 2;
		}
		return concave_points;
	}
	case SHAPE_HEIGHTMAP: {
		Vector<Vector3> hm_points;
		if ((map_width != 0) && (map_depth != 0) && map_data.size() >= map_width * map_depth) {
			Vector2 hm_size(map_width - 1, map_depth - 1);
			Vector2 start = hm_size * -0.5;

			const real_t* r = map_data.ptr();

			hm_points.resize(((map_width - 1) * map_depth * 2) + (map_width * (map_depth - 1) * 2) +
							 ((map_width - 1) * (map_depth - 1) * 2));

			int r_offset = 0;
			int w_offset = 0;
			for (int d = 0; d < map_depth; d++) {
				Vector3 h_pt(start.x, 0.0, start.y);

				for (int w = 0; w < map_width; w++) {
					h_pt.y = r[r_offset++];

					if (w != map_width - 1) {
						hm_points.write[w_offset++] = h_pt;
						hm_points.write[w_offset++] = Vector3(h_pt.x + 1.0, r[r_offset], h_pt.z);
					}

					if (d != map_depth - 1) {
						hm_points.write[w_offset++] = h_pt;
						hm_points.write[w_offset++] =
							Vector3(h_pt.x, r[r_offset + map_width - 1], h_pt.z + 1.0);
					}

					if ((w != map_width - 1) && (d != map_depth - 1)) {
						hm_points.write[w_offset++] = Vector3(h_pt.x + 1.0, r[r_offset], h_pt.z);
						hm_points.write[w_offset++] =
							Vector3(h_pt.x, r[r_offset + map_width - 1], h_pt.z + 1.0);
					}

					h_pt.x += 1.0;
				}

				start.y += 1.0;
			}
		}
		return hm_points;
	}
	}
	return Vector<Vector3>();
}

Ref<ArrayMesh> Shape3D::get_debug_arraymesh_faces(const Color& p_modulate) const
{
	(void)p_modulate;
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
	case SHAPE_SEPARATION_RAY:
	case SHAPE_SPHERE:
	case SHAPE_BOX:
	case SHAPE_CAPSULE:
	case SHAPE_CYLINDER:
	case SHAPE_CONVEX_POLYGON:
	case SHAPE_CONCAVE_POLYGON:
	case SHAPE_HEIGHTMAP:
	default:
		return memnew(ArrayMesh);
	}
}

real_t Shape3D::get_enclosing_radius() const
{
	switch (shape_type) {
	case SHAPE_WORLD_BOUNDARY:
		return 0;
	case SHAPE_SEPARATION_RAY:
		return length;
	case SHAPE_SPHERE:
		return radius;
	case SHAPE_BOX:
		return size.length() / 2;
	case SHAPE_CAPSULE:
		return height * 0.5f;
	case SHAPE_CYLINDER:
		return Vector2(radius, height * 0.5).length();
	case SHAPE_CONVEX_POLYGON: {
		const Vector3* read = points.ptr();
		real_t r = 0.0;
		for (int i = 0; i < points.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_CONCAVE_POLYGON: {
		const Vector3* read = faces.ptr();
		real_t r = 0.0;
		for (int i = 0; i < faces.size(); i++) {
			r = MAX(read[i].length_squared(), r);
		}
		return Math::sqrt(r);
	}
	case SHAPE_HEIGHTMAP:
		return Vector3(real_t(map_width), max_height - min_height, real_t(map_depth)).length();
	}
	return 0;
}

void Shape3D::add_vertices_to_array(Vector<Vector3>& array, const Transform3D& p_xform)
{
	Vector<Vector3> toadd = get_debug_mesh_lines();

	if (toadd.size()) {
		int base = array.size();
		array.resize(base + toadd.size());
		Vector3* w = array.ptrw();
		for (int i = 0; i < toadd.size(); i++) {
			w[i + base] = p_xform.xform(toadd[i]);
		}
	}
}

void Shape3D::set_custom_solver_bias(real_t p_bias)
{
	custom_bias = p_bias;
	PhysicsServer3D::shape_set_custom_solver_bias(shape, custom_bias);
}

real_t Shape3D::get_custom_solver_bias() const { return custom_bias; }

real_t Shape3D::get_margin() const { return margin; }

void Shape3D::set_margin(real_t p_margin)
{
	margin = p_margin;
	PhysicsServer3D::shape_set_margin(shape, margin);
}

void Shape3D::set_debug_color(const Color& p_color)
{
	if (p_color == debug_color) {
		return;
	}

	debug_color = p_color;
#ifdef DEBUG_ENABLED
	debug_properties_edited = true;
#endif // DEBUG_ENABLED
	_update_shape();
}

Color Shape3D::get_debug_color() const { return debug_color; }

void Shape3D::set_debug_fill(bool p_fill)
{
	if (p_fill == debug_fill) {
		return;
	}

	debug_fill = p_fill;
#ifdef DEBUG_ENABLED
	debug_properties_edited = true;
#endif // DEBUG_ENABLED
	_update_shape();
}

bool Shape3D::get_debug_fill() const { return debug_fill; }

void Shape3D::_update_shape()
{
	emit_changed();
	debug_mesh_cache.unref();
}

Shape3D::Shape3D(ShapeType p_type) : shape_type(p_type)
{
	shape = _create_shape_rid(shape_type);
	_ensure_heightmap_initialized();
	_update_shape();
}

Shape3D::~Shape3D()
{
	ERR_FAIL_COND(!PhysicsServer3D::is_initialized());
	PhysicsServer3D::free_rid(shape);
}

Ref<ArrayMesh> Shape3D::get_debug_mesh()
{
	if (debug_mesh_cache.is_null()) {
		debug_mesh_cache.instantiate();
	}
	return debug_mesh_cache;
}
#endif // !defined(_3D_DISABLED)


// --- Merged from scene/3d/visual_instance_3d.cpp ---
#if !defined(_3D_DISABLED)
#include "scene/3d/visual_instance_3d.h"

#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

void RenderInstance3D::update_visibility(Node3D* p_owner)
{
	if (!p_owner || !p_owner->is_inside_tree()) {
		return;
	}

	bool already_visible = p_owner->_is_vi_visible();
	bool visible = p_owner->is_visible_in_tree();
	p_owner->_set_vi_visible(visible);

	if (visible && !already_visible) {
		if (!p_owner->_is_using_identity_transform()) {
			Transform3D gt = p_owner->get_global_transform();
			Renderer::instance_set_transform(instance, gt);
		}
	}

	Renderer::instance_set_visible(instance, visible);
}

void RenderInstance3D::set_instance_use_identity_transform(Node3D* p_owner, bool p_enable)
{
	if (!p_owner) {
		return;
	}
	p_owner->_set_use_identity_transform(p_enable);

	if (p_owner->is_inside_tree()) {
		if (p_enable) {
			Renderer::instance_set_transform(instance, Transform3D());
		}
		else {
			Renderer::instance_set_transform(instance, p_owner->get_global_transform());
		}
	}
}

void RenderInstance3D::fti_update_servers_xform(Node3D* p_owner)
{
	if (p_owner && !p_owner->_is_using_identity_transform()) {
		Renderer::instance_set_transform(
			instance, p_owner->_get_cached_global_transform_interpolated());
	}
}

void RenderInstance3D::notification(Node3D* p_owner, int p_what)
{
	if (!p_owner) {
		return;
	}

	switch (p_what) {
	case Node3D::NOTIFICATION_ENTER_WORLD: {
		ERR_FAIL_COND(p_owner->get_world_3d().is_null());
		if (!instance.is_valid() && Renderer::data) {
			instance = Renderer::instance_create();
			if (base.is_valid()) {
				Renderer::instance_set_base(instance, base);
			}
			Renderer::instance_set_layer_mask(instance, layers);
		}
		Renderer::instance_set_scenario(instance, p_owner->get_world_3d()->get_scenario());
		update_visibility(p_owner);
	} break;

	case Node3D::NOTIFICATION_TRANSFORM_CHANGED: {
		if (p_owner->_is_vi_visible() &&
			!(p_owner->is_inside_tree() && SceneTree::is_physics_interpolation_enabled()) &&
			!p_owner->_is_using_identity_transform()) {
			Renderer::instance_set_transform(instance, p_owner->get_global_transform());
		}
	} break;

	case Node::NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		if (p_owner->_is_vi_visible() && p_owner->is_inside_tree()) {
			Renderer::instance_teleport(instance);
		}
	} break;

	case Node3D::NOTIFICATION_EXIT_WORLD: {
		Renderer::instance_set_scenario(instance, RID());
		Renderer::instance_attach_skeleton(instance, RID());
		p_owner->_set_vi_visible(false);
	} break;

	case Node3D::NOTIFICATION_VISIBILITY_CHANGED: {
		update_visibility(p_owner);
	} break;
	}
}

RID RenderInstance3D::get_instance() const { return instance; }

void RenderInstance3D::set_layer_mask(uint32_t p_mask)
{
	layers = p_mask;
	if (instance.is_valid()) {
		Renderer::instance_set_layer_mask(instance, p_mask);
	}
}

uint32_t RenderInstance3D::get_layer_mask() const { return layers; }

void RenderInstance3D::set_layer_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_layer_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_layer_mask(mask);
}

bool RenderInstance3D::get_layer_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return layers & (1 << (p_layer_number - 1));
}

void RenderInstance3D::set_sorting_offset(float p_offset)
{
	sorting_offset = p_offset;
	if (instance.is_valid()) {
		Renderer::instance_set_pivot_data(instance, sorting_offset, sorting_use_aabb_center);
	}
}

float RenderInstance3D::get_sorting_offset() const { return sorting_offset; }

void RenderInstance3D::set_sorting_use_aabb_center(bool p_enabled)
{
	sorting_use_aabb_center = p_enabled;
	if (instance.is_valid()) {
		Renderer::instance_set_pivot_data(instance, sorting_offset, sorting_use_aabb_center);
	}
}

bool RenderInstance3D::is_sorting_use_aabb_center() const { return sorting_use_aabb_center; }

void RenderInstance3D::set_base(const RID& p_base)
{
	base = p_base;
	if (!instance.is_valid() && base.is_valid() && Renderer::data) {
		instance = Renderer::instance_create();
		Renderer::instance_set_layer_mask(instance, layers);
	}
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_set_base(instance, p_base);
	}
}

RID RenderInstance3D::get_base() const { return base; }

void RenderInstance3D::set_material_override(const Ref<Material>& p_material)
{
	material_override = p_material;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_material_override(
			instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> RenderInstance3D::get_material_override() const { return material_override; }

void RenderInstance3D::set_material_overlay(const Ref<Material>& p_material)
{
	material_overlay = p_material;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_material_overlay(
			instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> RenderInstance3D::get_material_overlay() const { return material_overlay; }

float RenderInstance3D::get_transparency() const { return transparency; }

float RenderInstance3D::get_visibility_range_begin() const { return visibility_range_begin; }

float RenderInstance3D::get_visibility_range_end() const { return visibility_range_end; }

float RenderInstance3D::get_visibility_range_begin_margin() const
{
	return visibility_range_begin_margin;
}

float RenderInstance3D::get_visibility_range_end_margin() const
{
	return visibility_range_end_margin;
}

RenderInstance3D::VisibilityRangeFadeMode RenderInstance3D::get_visibility_range_fade_mode() const
{
	return visibility_range_fade_mode;
}

const String* RenderInstance3D::_instance_uniform_get_remap(const String& p_name) const
{
	String* r = instance_shader_parameter_property_remap.getptr(p_name);
	if (!r) {
		String s = p_name;
#ifndef DISABLE_DEPRECATED
		if (s.begins_with("shader_uniforms/")) {
			s = s.replace("shader_uniforms/", "instance_shader_parameters/");
		}
#endif // DISABLE_DEPRECATED
		if (s.begins_with("instance_shader_parameters/")) {
			String pname = String(s);
			String name = s.replace("instance_shader_parameters/", "");
			instance_shader_parameter_property_remap[pname] = name;
			return instance_shader_parameter_property_remap.getptr(pname);
		}
		return nullptr;
	}

	return r;
}

void RenderInstance3D::set_cast_shadows_setting(ShadowCastingSetting p_shadow_casting_setting)
{
	shadow_casting_setting = p_shadow_casting_setting;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_cast_shadows_setting(
			instance, (RSE::ShadowCastingSetting)p_shadow_casting_setting);
	}
}

RenderInstance3D::ShadowCastingSetting RenderInstance3D::get_cast_shadows_setting() const
{
	return shadow_casting_setting;
}

void RenderInstance3D::set_extra_cull_margin(float p_margin)
{
	ERR_FAIL_COND(p_margin < 0);
	extra_cull_margin = p_margin;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_set_extra_visibility_margin(instance, extra_cull_margin);
	}
}

float RenderInstance3D::get_extra_cull_margin() const { return extra_cull_margin; }

void RenderInstance3D::set_lod_bias(float p_bias)
{
	ERR_FAIL_COND(p_bias < 0.0);
	lod_bias = p_bias;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_lod_bias(instance, lod_bias);
	}
}

float RenderInstance3D::get_lod_bias() const { return lod_bias; }

AABB RenderInstance3D::get_custom_aabb() const { return custom_aabb; }

void RenderInstance3D::set_lightmap_texel_scale(float p_scale) { lightmap_texel_scale = p_scale; }

float RenderInstance3D::get_lightmap_texel_scale() const { return lightmap_texel_scale; }

void RenderInstance3D::set_gi_mode(GIMode p_mode)
{
	gi_mode = p_mode;
	if (!instance.is_valid() || !Renderer::data) {
		return;
	}

	switch (p_mode) {
	case GI_MODE_DISABLED: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
	} break;
	case GI_MODE_STATIC: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, true);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
	} break;
	case GI_MODE_DYNAMIC: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, true);
	} break;
	}
}

RenderInstance3D::GIMode RenderInstance3D::get_gi_mode() const { return gi_mode; }

void RenderInstance3D::set_ignore_occlusion_culling(bool p_enabled)
{
	ignore_occlusion_culling = p_enabled;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_flag(
			instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, ignore_occlusion_culling);
	}
}

bool RenderInstance3D::is_ignoring_occlusion_culling() const { return ignore_occlusion_culling; }

void RenderInstance3D::append_geometry_warnings(PackedStringArray& r_warnings) const
{
	if (!Math::is_zero_approx(visibility_range_end) &&
		visibility_range_end <= visibility_range_begin) {
		r_warnings.push_back(
			RTR("The visibility range's End distance is set to a non-zero value, but is lower than "
				"the Begin distance.\nTo resolve this, set the End distance to 0 or to a value "
				"greater than the Begin distance."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		!Math::is_zero_approx(visibility_range_begin) &&
		Math::is_zero_approx(visibility_range_begin_margin)) {
		r_warnings.push_back(RTR("Configured to fade in smoothly over distance, but the fade "
								 "transition distance is set to 0.\nTo resolve this, increase "
								 "Visibility Range Begin Margin above 0."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		!Math::is_zero_approx(visibility_range_end) &&
		Math::is_zero_approx(visibility_range_end_margin)) {
		r_warnings.push_back(RTR("Configured to fade out smoothly over distance, but the fade "
								 "transition distance is set to 0.\nTo resolve this, increase "
								 "Visibility Range End Margin above 0."));
	}

	if (!Math::is_zero_approx(transparency) &&
		OS::get_current_rendering_method() != "forward_plus") {
		r_warnings.push_back(
			RTR("Instance transparency is only available when using the Forward+ renderer."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		OS::get_current_rendering_method() != "forward_plus") {
		r_warnings.push_back(RTR("Visibility range transparency fade is only available when using "
								 "the Forward+ renderer."));
	}
}

RenderInstance3D::~RenderInstance3D()
{
	if (material_overlay.is_valid()) {
		set_material_overlay(Ref<Material>());
	}
	if (material_override.is_valid()) {
		set_material_override(Ref<Material>());
	}
	if (instance.is_valid() && Renderer::data) {
		Renderer::free_rid(instance);
	}
}

// ==========================================================================
// Merged from scene/3d/mesh_instance_3d.cpp
// ==========================================================================

#include "scene/3d/skeleton_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/3d/shape_3d.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

#include <cfloat> // FLT_EPSILON

#ifndef NAVIGATION_3D_DISABLED
RID MeshInstance3D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

Ref<Mesh> MeshInstance3D::get_mesh() const { return mesh; }

int MeshInstance3D::get_blend_shape_count() const
{
	if (mesh.is_null()) {
		return 0;
	}
	return mesh->get_blend_shape_count();
}

int MeshInstance3D::find_blend_shape_by_name(const String& p_name)
{
	if (mesh.is_null()) {
		return -1;
	}
	for (int i = 0; i < mesh->get_blend_shape_count(); i++) {
		if (mesh->get_blend_shape_name(i) == p_name) {
			return i;
		}
	}
	return -1;
}

float MeshInstance3D::get_blend_shape_value(int p_blend_shape) const
{
	ERR_FAIL_COND_V(mesh.is_null(), 0.0);
	ERR_FAIL_INDEX_V(p_blend_shape, (int)blend_shape_tracks.size(), 0);
	return blend_shape_tracks[p_blend_shape];
}

void MeshInstance3D::set_blend_shape_value(int p_blend_shape, float p_value)
{
	ERR_FAIL_COND(mesh.is_null());
	ERR_FAIL_INDEX(p_blend_shape, (int)blend_shape_tracks.size());
	blend_shape_tracks[p_blend_shape] = p_value;
	Renderer::instance_set_blend_shape_weight(
		get_instance(), p_blend_shape, p_value);
}

void MeshInstance3D::set_skin(const Ref<Skin>& p_skin)
{
	skin_internal = p_skin;
	skin = p_skin;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}

Ref<Skin> MeshInstance3D::get_skin() const { return skin; }

Ref<SkinReference> MeshInstance3D::get_skin_reference() const { return skin_ref; }

void MeshInstance3D::set_skeleton_path(const NodePath& p_skeleton)
{
	skeleton_path = p_skeleton;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}

NodePath MeshInstance3D::get_skeleton_path() { return skeleton_path; }

AABB MeshInstance3D::get_aabb() const
{
	if (mesh.is_valid()) {
		return mesh->get_aabb();
	}

	return AABB();
}

#ifndef PHYSICS_3D_DISABLED
Node* MeshInstance3D::create_trimesh_collision_node()
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<Shape3D> shape = mesh->create_trimesh_shape();
	if (shape.is_null()) {
		return nullptr;
	}

	PhysicsBody3D* static_body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	CollisionShape3D* cshape = memnew(CollisionShape3D);
	cshape->set_shape(shape);
	static_body->add_child(cshape, true);
	return static_body;
}

Node* MeshInstance3D::create_convex_collision_node(bool p_clean, bool p_simplify)
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<Shape3D> shape = mesh->create_convex_shape(p_clean, p_simplify);
	if (shape.is_null()) {
		return nullptr;
	}

	PhysicsBody3D* static_body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	CollisionShape3D* cshape = memnew(CollisionShape3D);
	cshape->set_shape(shape);
	static_body->add_child(cshape, true);
	return static_body;
}

Node* MeshInstance3D::create_multiple_convex_collisions_node(
	const Ref<MeshConvexDecompositionSettings>& p_settings)
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<MeshConvexDecompositionSettings> settings;
	if (p_settings.is_valid()) {
		settings = p_settings;
	}
	else {
		settings.instantiate();
	}

	Vector<Ref<Shape3D>> shapes = mesh->convex_decompose(settings);
	if (!shapes.size()) {
		return nullptr;
	}

	PhysicsBody3D* static_body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	for (int i = 0; i < shapes.size(); i++) {
		CollisionShape3D* cshape = memnew(CollisionShape3D);
		cshape->set_shape(shapes[i]);
		static_body->add_child(cshape, true);
	}
	return static_body;
}

#endif // PHYSICS_3D_DISABLED

int MeshInstance3D::get_surface_override_material_count() const
{
	return surface_override_materials.size();
}

void MeshInstance3D::set_surface_override_material(int p_surface, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_surface, surface_override_materials.size());

	surface_override_materials.write[p_surface] = p_material;

	if (surface_override_materials[p_surface].is_valid()) {
		Renderer::instance_set_surface_override_material(
			get_instance(), p_surface, surface_override_materials[p_surface]->get_rid());
	}
	else {
		Renderer::instance_set_surface_override_material(
			get_instance(), p_surface, RID());
	}
}

Ref<Material> MeshInstance3D::get_surface_override_material(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surface_override_materials.size(), Ref<Material>());

	return surface_override_materials[p_surface];
}

Ref<Material> MeshInstance3D::get_active_material(int p_surface) const
{
	Ref<Material> mat_override = get_material_override();
	if (mat_override.is_valid()) {
		return mat_override;
	}

	Ref<Mesh> m = get_mesh();
	if (m.is_null() || m->get_surface_count() == 0) {
		return Ref<Material>();
	}

	Ref<Material> surface_material = get_surface_override_material(p_surface);
	if (surface_material.is_valid()) {
		return surface_material;
	}

	return m->surface_get_material(p_surface);
}

void MeshInstance3D::create_debug_tangents()
{
	MeshInstance3D* mi = create_debug_tangents_node();
	if (!mi) {
		return;
	}

	add_child(mi, true);
	if (is_inside_tree() && this == SceneTree::get_edited_scene_root()) {
		mi->set_owner(this);
	}
	else {
		mi->set_owner(get_owner());
	}
}

bool MeshInstance3D::_property_can_revert(const String& p_name) const
{
	HashMap<String, int>::ConstIterator E = blend_shape_properties.find(p_name);
	if (E) {
		return true;
	}
	return false;
}

Ref<TriangleMesh> MeshInstance3D::generate_triangle_mesh() const
{
	if (mesh.is_valid()) {
		return mesh->generate_triangle_mesh();
	}
	return Ref<TriangleMesh>();
}

PackedStringArray MeshInstance3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	render_instance.append_geometry_warnings(warnings);
	if (mesh.is_null()) {
		warnings.push_back(RTR("MeshInstance3D requires a Mesh to render anything. Please add a "
							   "mesh resource for it!"));
	}
	return warnings;
}

void MeshInstance3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void MeshInstance3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void MeshInstance3D::_resolve_skeleton_path() {}

void MeshInstance3D::set_mesh(Ref<Mesh> const& p_mesh)
{
	mesh = p_mesh;
	render_instance.set_base(mesh.is_valid() ? mesh->get_rid() : RID());
}

MeshInstance3D::MeshInstance3D() {}

MeshInstance3D* MeshInstance3D::create_debug_tangents_node()
{
	return nullptr;
}

// ==========================================================================
// Merged from scene/3d/multimesh_instance_3d.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"

#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"

RID MultiMeshInstance3D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

void MultiMeshInstance3D::_refresh_interpolated()
{
	if (is_inside_tree() && multimesh.is_valid()) {
		bool interpolated = is_physics_interpolated_and_enabled();
		multimesh->set_physics_interpolated(interpolated);
	}
}

void MultiMeshInstance3D::_physics_interpolated_changed()
{
	Node3D::_physics_interpolated_changed();
	_refresh_interpolated();
}

void MultiMeshInstance3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void MultiMeshInstance3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
	if (p_what == NOTIFICATION_ENTER_TREE) {
		_refresh_interpolated();
	}
}

void MultiMeshInstance3D::set_multimesh(const Ref<MultiMesh>& p_multimesh)
{
	multimesh = p_multimesh;
	if (multimesh.is_valid()) {
		set_base(multimesh->get_rid());
		_refresh_interpolated();
	}
	else {
		set_base(RID());
	}
}

Ref<MultiMesh> MultiMeshInstance3D::get_multimesh() const { return multimesh; }

AABB MultiMeshInstance3D::get_aabb() const
{
	if (multimesh.is_null()) {
		return AABB();
	}
	else {
		return multimesh->get_aabb();
	}
}

// ==========================================================================
// Merged from scene/3d/importer_mesh_instance_3d.cpp
// ==========================================================================

#include "scene/3d/skeleton_3d.h"
#include "scene/resources/navigation_mesh.h"

void ImporterMeshInstance3D::set_mesh(const Ref<ImporterMesh>& p_mesh) { mesh = p_mesh; }

Ref<ImporterMesh> ImporterMeshInstance3D::get_mesh() const { return mesh; }

void ImporterMeshInstance3D::set_skin(const Ref<Skin>& p_skin) { skin = p_skin; }

Ref<Skin> ImporterMeshInstance3D::get_skin() const { return skin; }

void ImporterMeshInstance3D::set_surface_material(int p_idx, const Ref<Material>& p_material)
{
	ERR_FAIL_COND(p_idx < 0);
	if (p_idx >= surface_materials.size()) {
		surface_materials.resize(p_idx + 1);
	}

	surface_materials.write[p_idx] = p_material;
}

Ref<Material> ImporterMeshInstance3D::get_surface_material(int p_idx) const
{
	ERR_FAIL_COND_V(p_idx < 0, Ref<Material>());
	if (p_idx >= surface_materials.size()) {
		return Ref<Material>();
	}
	return surface_materials[p_idx];
}

void ImporterMeshInstance3D::set_skeleton_path(const NodePath& p_path) { skeleton_path = p_path; }

NodePath ImporterMeshInstance3D::get_skeleton_path() const { return skeleton_path; }

uint32_t ImporterMeshInstance3D::get_layer_mask() const { return layer_mask; }

void ImporterMeshInstance3D::set_layer_mask(const uint32_t p_layer_mask)
{
	layer_mask = p_layer_mask;
}

void ImporterMeshInstance3D::set_cast_shadows_setting(
	RenderInstance3D::ShadowCastingSetting p_shadow_casting_setting)
{
	shadow_casting_setting = p_shadow_casting_setting;
}

RenderInstance3D::ShadowCastingSetting ImporterMeshInstance3D::get_cast_shadows_setting() const
{
	return shadow_casting_setting;
}

float ImporterMeshInstance3D::get_visibility_range_begin() const { return visibility_range_begin; }

float ImporterMeshInstance3D::get_visibility_range_end() const { return visibility_range_end; }

float ImporterMeshInstance3D::get_visibility_range_begin_margin() const
{
	return visibility_range_begin_margin;
}

float ImporterMeshInstance3D::get_visibility_range_end_margin() const
{
	return visibility_range_end_margin;
}

RenderInstance3D::VisibilityRangeFadeMode
ImporterMeshInstance3D::get_visibility_range_fade_mode() const
{
	return visibility_range_fade_mode;
}

// ==========================================================================
// Merged from scene/3d/sprite_3d.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"

#include "core/config/engine.h"
#include "core/math/geometry_3d.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/renderer.h"

Color Sprite3D::_get_color_accum()
{
	if (!color_dirty) {
		return color_accum;
	}

	if (parent_sprite) {
		color_accum = parent_sprite->_get_color_accum();
	}
	else {
		color_accum = Color(1, 1, 1, 1);
	}

	color_accum.r *= modulate.r;
	color_accum.g *= modulate.g;
	color_accum.b *= modulate.b;
	color_accum.a *= modulate.a;
	color_dirty = false;
	return color_accum;
}

void Sprite3D::_queue_redraw()
{
	redraw_needed = true;
}

void Sprite3D::_propagate_color_changed()
{
	if (color_dirty) {
		return;
	}

	color_dirty = true;
	_queue_redraw();

	for (Sprite3D*& E : children) {
		E->_propagate_color_changed();
	}
}

void Sprite3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void Sprite3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_im_update();
	} break;

	case NOTIFICATION_UNPARENTED: {
		if (parent_sprite) {
			parent_sprite->children.erase(pI);
			pI = nullptr;
			parent_sprite = nullptr;

			_propagate_color_changed();
		}
	} break;
	}
}

void Sprite3D::set_centered(bool p_center)
{
	if (centered == p_center) {
		return;
	}
	centered = p_center;
	_queue_redraw();
}

bool Sprite3D::is_centered() const { return centered; }

void Sprite3D::set_offset(const Point2& p_offset)
{
	if (offset == p_offset) {
		return;
	}
	offset = p_offset;
	_queue_redraw();
}

Point2 Sprite3D::get_offset() const { return offset; }

void Sprite3D::set_flip_h(bool p_flip)
{
	if (hflip == p_flip) {
		return;
	}
	hflip = p_flip;
	_queue_redraw();
}

bool Sprite3D::is_flipped_h() const { return hflip; }

void Sprite3D::set_flip_v(bool p_flip)
{
	if (vflip == p_flip) {
		return;
	}
	vflip = p_flip;
	_queue_redraw();
}

bool Sprite3D::is_flipped_v() const { return vflip; }

void Sprite3D::set_modulate(const Color& p_color)
{
	if (modulate == p_color) {
		return;
	}
	modulate = p_color;
	_propagate_color_changed();
	_queue_redraw();
}

Color Sprite3D::get_modulate() const { return modulate; }

void Sprite3D::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);
	if (render_priority == p_priority) {
		return;
	}
	render_priority = p_priority;
	_queue_redraw();
}

int Sprite3D::get_render_priority() const { return render_priority; }

void Sprite3D::set_pixel_size(real_t p_amount)
{
	if (pixel_size == p_amount) {
		return;
	}
	pixel_size = p_amount;
	_queue_redraw();
}

real_t Sprite3D::get_pixel_size() const { return pixel_size; }

void Sprite3D::set_axis(Vector3::Axis p_axis)
{
	ERR_FAIL_INDEX(p_axis, 3);
	if (axis == p_axis) {
		return;
	}
	axis = p_axis;
	_queue_redraw();
}

Vector3::Axis Sprite3D::get_axis() const { return axis; }

void Sprite3D::_im_update()
{
	if (!redraw_needed) {
		return;
	}
	_draw();
	redraw_needed = false;
	pending_update = false;
}

AABB Sprite3D::get_aabb() const { return aabb; }

Ref<TriangleMesh> Sprite3D::generate_triangle_mesh() const
{
	if (triangle_mesh.is_valid()) {
		return triangle_mesh;
	}

	Vector<Vector3> faces;
	faces.resize(6);
	Vector3* facesw = faces.ptrw();

	Rect2 final_rect = get_item_rect();
	if (final_rect.size.x == 0 || final_rect.size.y == 0) {
		return Ref<TriangleMesh>();
	}

	real_t px_size = get_pixel_size();
	Vector2 vertices[4] = {
		(final_rect.position + Vector2(0, final_rect.size.y)) * px_size,
		(final_rect.position + final_rect.size) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, 0)) * px_size,
		final_rect.position * px_size,
	};

	int x_axis = ((axis + 1) % 3);
	int y_axis = ((axis + 2) % 3);

	if (axis != Vector3::AXIS_Z) {
		SWAP(x_axis, y_axis);
		for (int i = 0; i < 4; i++) {
			if (axis == Vector3::AXIS_Y) {
				vertices[i].y = -vertices[i].y;
			}
			else if (axis == Vector3::AXIS_X) {
				vertices[i].x = -vertices[i].x;
			}
		}
	}

	static const int indices[6] = {0, 1, 2, 0, 2, 3};
	for (int j = 0; j < 6; j++) {
		int i = indices[j];
		Vector3 vtx;
		vtx[x_axis] = vertices[i][0];
		vtx[y_axis] = vertices[i][1];
		facesw[j] = vtx;
	}

	triangle_mesh.instantiate();
	triangle_mesh->create(faces);
	return triangle_mesh;
}

void Sprite3D::set_draw_flag(DrawFlags p_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	if (flags[p_flag] == p_enable) {
		return;
	}
	flags[p_flag] = p_enable;
	_queue_redraw();
}

bool Sprite3D::get_draw_flag(DrawFlags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void Sprite3D::set_alpha_cut_mode(AlphaCutMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, ALPHA_CUT_MAX);
	if (alpha_cut == p_mode) {
		return;
	}
	alpha_cut = p_mode;
	_queue_redraw();
}

Sprite3D::AlphaCutMode Sprite3D::get_alpha_cut_mode() const { return alpha_cut; }

void Sprite3D::set_alpha_hash_scale(float p_hash_scale)
{
	if (alpha_hash_scale == p_hash_scale) {
		return;
	}
	alpha_hash_scale = p_hash_scale;
	_queue_redraw();
}

float Sprite3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

void Sprite3D::set_alpha_scissor_threshold(float p_threshold)
{
	if (alpha_scissor_threshold == p_threshold) {
		return;
	}
	alpha_scissor_threshold = p_threshold;
	_queue_redraw();
}

float Sprite3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

void Sprite3D::set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa)
{
	if (alpha_antialiasing_mode == p_alpha_aa) {
		return;
	}
	alpha_antialiasing_mode = p_alpha_aa;
	_queue_redraw();
}

StandardMaterial3D::AlphaAntiAliasing Sprite3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void Sprite3D::set_alpha_antialiasing_edge(float p_edge)
{
	if (alpha_antialiasing_edge == p_edge) {
		return;
	}
	alpha_antialiasing_edge = p_edge;
	_queue_redraw();
}

float Sprite3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

void Sprite3D::set_billboard_mode(StandardMaterial3D::BillboardMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, 3);
	if (billboard_mode == p_mode) {
		return;
	}
	billboard_mode = p_mode;
	_queue_redraw();
}

StandardMaterial3D::BillboardMode Sprite3D::get_billboard_mode() const { return billboard_mode; }

void Sprite3D::set_texture_filter(StandardMaterial3D::TextureFilter p_filter)
{
	if (texture_filter == p_filter) {
		return;
	}
	texture_filter = p_filter;
	_queue_redraw();
}

StandardMaterial3D::TextureFilter Sprite3D::get_texture_filter() const { return texture_filter; }

void Sprite3D::_draw()
{
	if (get_base() != mesh) {
		set_base(mesh);
	}

	if (frames.is_valid()) {
		if (!frames->has_animation(animation)) {
			return;
		}
		Ref<Texture2D> frame_tex = frames->get_frame_texture(animation, frame);
		if (frame_tex.is_null()) {
			set_base(RID());
			return;
		}
		Size2 tsize = frame_tex->get_size();
		if (tsize.x == 0 || tsize.y == 0) {
			return;
		}
		Point2 ofs = offset;
		if (centered) {
			ofs -= tsize / 2;
		}
		draw_texture_rect(frame_tex, Rect2(ofs, tsize), Rect2(Point2(), tsize));
		return;
	}

	if (texture.is_null()) {
		set_base(RID());
		return;
	}
	Vector2 tsize = texture->get_size();
	if (tsize.x == 0 || tsize.y == 0) {
		return;
	}

	Rect2 base_rect =
		region ? region_rect : Rect2(0, 0, texture->get_width(), texture->get_height());
	Size2 frame_size = base_rect.size / Size2(hframes, vframes);
	Point2 frame_offset = Point2(frame % hframes, frame / hframes) * frame_size;

	Point2 dst_offset = offset;
	if (centered) {
		dst_offset -= frame_size / 2.0f;
	}

	draw_texture_rect(texture, Rect2(dst_offset, frame_size),
		Rect2(base_rect.position + frame_offset, frame_size));
}

void Sprite3D::set_texture(const Ref<Texture2D>& p_texture)
{
	if (texture == p_texture) {
		return;
	}
	texture = p_texture;
	triangle_mesh.unref();
	_queue_redraw();
}

Ref<Texture2D> Sprite3D::get_texture() const { return texture; }

void Sprite3D::set_region_enabled(bool p_region)
{
	if (region == p_region) {
		return;
	}
	region = p_region;
	_queue_redraw();
}

bool Sprite3D::is_region_enabled() const { return region; }

void Sprite3D::set_region_rect(const Rect2& p_region_rect)
{
	if (region_rect == p_region_rect) {
		return;
	}
	region_rect = p_region_rect;
	if (region) {
		_queue_redraw();
	}
}

Rect2 Sprite3D::get_region_rect() const { return region_rect; }

void Sprite3D::set_frame(int p_frame)
{
	if (frames.is_valid()) {
		if (!frames->has_animation(animation)) {
			return;
		}
		int limit = frames->get_frame_count(animation);
		p_frame = CLAMP(p_frame, 0, MAX(0, limit - 1));
	}
	else {
		ERR_FAIL_INDEX(p_frame, vframes * hframes);
	}
	if (frame == p_frame) {
		return;
	}
	frame = p_frame;
	_calc_frame_speed_scale();
	_queue_redraw();
}

int Sprite3D::get_frame() const { return frame; }

void Sprite3D::set_frame_coords(const Vector2i& p_coord)
{
	ERR_FAIL_INDEX(p_coord.x, hframes);
	ERR_FAIL_INDEX(p_coord.y, vframes);
	set_frame(p_coord.y * hframes + p_coord.x);
}

Vector2i Sprite3D::get_frame_coords() const { return Vector2i(frame % hframes, frame / hframes); }

void Sprite3D::set_vframes(int p_amount)
{
	ERR_FAIL_COND(p_amount < 1);
	vframes = p_amount;
	_queue_redraw();
}

int Sprite3D::get_vframes() const { return vframes; }

void Sprite3D::set_hframes(int p_amount)
{
	ERR_FAIL_COND(p_amount < 1);
	hframes = p_amount;
	_queue_redraw();
}

int Sprite3D::get_hframes() const { return hframes; }

void Sprite3D::set_sprite_frames(const Ref<SpriteFrames>& p_frames)
{
	frames = p_frames;
	triangle_mesh.unref();
	_queue_redraw();
}

Ref<SpriteFrames> Sprite3D::get_sprite_frames() const { return frames; }

void Sprite3D::play(const String& p_name, float p_custom_scale, bool p_from_end)
{
	if (!p_name.is_empty()) {
		animation = p_name;
	}
	custom_speed_scale = p_custom_scale;
	playing = true;
	if (p_from_end && frames.is_valid() && frames->has_animation(animation)) {
		frame = MAX(0, frames->get_frame_count(animation) - 1);
	}
	_calc_frame_speed_scale();
	_queue_redraw();
}

void Sprite3D::play_backwards(const String& p_name) { play(p_name, -1.0f, true); }

void Sprite3D::pause() { _stop_internal(false); }

void Sprite3D::stop() { _stop_internal(true); }

bool Sprite3D::is_playing() const { return playing; }

void Sprite3D::set_animation(const String& p_name)
{
	if (animation == p_name) {
		return;
	}
	animation = p_name;
	frame = 0;
	_calc_frame_speed_scale();
	_queue_redraw();
}

String Sprite3D::get_animation() const { return animation; }

void Sprite3D::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}
	autoplay = p_name;
}

String Sprite3D::get_autoplay() const { return autoplay; }

void Sprite3D::set_frame_progress(real_t p_progress) { frame_progress = p_progress; }

real_t Sprite3D::get_frame_progress() const { return frame_progress; }

void Sprite3D::set_speed_scale(float p_speed_scale) { speed_scale = p_speed_scale; }

float Sprite3D::get_speed_scale() const { return speed_scale; }

float Sprite3D::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * custom_speed_scale;
}

double Sprite3D::_get_frame_duration()
{
	if (frames.is_valid() && frames->has_animation(animation)) {
		return frames->get_frame_duration(animation, frame);
	}
	return 1.0;
}

void Sprite3D::_calc_frame_speed_scale()
{
	frame_speed_scale = 1.0 / _get_frame_duration();
}

void Sprite3D::_stop_internal(bool p_reset)
{
	playing = false;
	if (p_reset) {
		frame = 0;
		frame_progress = 0.0;
		_queue_redraw();
	}
}

Rect2 Sprite3D::get_item_rect() const
{
	if (frames.is_valid()) {
		if (!frames->has_animation(animation) || frame < 0 ||
			frame >= frames->get_frame_count(animation)) {
			return Rect2(0, 0, 1, 1);
		}
		Ref<Texture2D> t = !animation.is_empty() ? frames->get_frame_texture(animation, frame)
												 : Ref<Texture2D>();
		if (t.is_null()) {
			return Rect2(0, 0, 1, 1);
		}
		Size2 s = t->get_size();
		Point2 ofs = offset;
		if (centered) {
			ofs -= s / 2;
		}
		if (s == Size2(0, 0)) {
			s = Size2(1, 1);
		}
		return Rect2(ofs, s);
	}

	if (texture.is_null()) {
		return Rect2(0, 0, 1, 1);
	}

	Size2 s = region ? region_rect.size : (texture->get_size() / Point2(hframes, vframes));
	Point2 ofs = offset;
	if (centered) {
		ofs -= s / 2;
	}
	if (s == Size2(0, 0)) {
		s = Size2(1, 1);
	}
	return Rect2(ofs, s);
}

PackedStringArray Sprite3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	render_instance.append_geometry_warnings(warnings);
	return warnings;
}

#ifdef TOOLS_ENABLED
void Sprite3D::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 && frames.is_valid()) {
		if (pf == "play" || pf == "play_backwards" || pf == "set_animation" ||
			pf == "set_autoplay") {
			List<String> al;
			frames->get_animation_list(&al);
			for (const String& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node3D::get_argument_options(p_function, p_idx, r_options);
}
#endif

void Sprite3D::draw_texture_rect(Ref<Texture2D>, Rect2, Rect2) {}

Sprite3D::Sprite3D() {}

Sprite3D::~Sprite3D()
{
	if (!Renderer::data) {
		return;
	}
	if (mesh.is_valid()) {
		Renderer::free_rid(mesh);
	}
	if (material.is_valid()) {
		Renderer::free_rid(material);
	}
}

// ==========================================================================
// Merged from scene/3d/label_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/math/geometry_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/window.h"
#include "scene/resources/theme.h"
#include "servers/rendering/renderer.h"

AABB Label3D::get_aabb() const { return aabb; }

Ref<TriangleMesh> Label3D::generate_triangle_mesh() const
{
	if (triangle_mesh.is_valid()) {
		return triangle_mesh;
	}

	Ref<Font> font = _get_font_or_default();
	if (font.is_null()) {
		return Ref<TriangleMesh>();
	}

	Vector<Vector3> faces;
	faces.resize(6);
	Vector3* facesw = faces.ptrw();

	float total_h = 0.0;
	float max_line_w = 0.0;
	for (int i = 0; i < lines_rid.size(); i++) {
		total_h += TS->shaped_text_get_size(lines_rid[i]).y + line_spacing;
		max_line_w = MAX(max_line_w, TS->shaped_text_get_width(lines_rid[i]));
	}

	float vbegin = 0;
	switch (vertical_alignment) {
	case VERTICAL_ALIGNMENT_FILL:
	case VERTICAL_ALIGNMENT_TOP: {
		// Nothing.
	} break;
	case VERTICAL_ALIGNMENT_CENTER: {
		vbegin = (total_h - line_spacing) / 2.0;
	} break;
	case VERTICAL_ALIGNMENT_BOTTOM: {
		vbegin = (total_h - line_spacing);
	} break;
	}

	Vector2 offset = Vector2(0, vbegin);
	switch (horizontal_alignment) {
	case HORIZONTAL_ALIGNMENT_LEFT:
		break;
	case HORIZONTAL_ALIGNMENT_FILL:
	case HORIZONTAL_ALIGNMENT_CENTER: {
		offset.x = -max_line_w / 2.0;
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		offset.x = -max_line_w;
	} break;
	}

	Rect2 final_rect = Rect2(offset + lbl_offset, Size2(max_line_w, total_h));

	if (final_rect.size.x == 0 || final_rect.size.y == 0) {
		return Ref<TriangleMesh>();
	}

	real_t px_size = get_pixel_size();

	Vector2 vertices[4] = {

		(final_rect.position + Vector2(0, -final_rect.size.y)) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, -final_rect.size.y)) * px_size,
		(final_rect.position + Vector2(final_rect.size.x, 0)) * px_size,
		final_rect.position * px_size,

	};

	static const int indices[6] = {0, 1, 2, 0, 2, 3};

	for (int j = 0; j < 6; j++) {
		int i = indices[j];
		Vector3 vtx;
		vtx[0] = vertices[i][0];
		vtx[1] = vertices[i][1];
		facesw[j] = vtx;
	}

	triangle_mesh.instantiate();
	triangle_mesh->create(faces);

	return triangle_mesh;
}

String Label3D::get_text() const { return text; }

void Label3D::set_horizontal_alignment(HorizontalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (horizontal_alignment != p_alignment) {
		if (horizontal_alignment == HORIZONTAL_ALIGNMENT_FILL ||
			p_alignment == HORIZONTAL_ALIGNMENT_FILL) {
			dirty_lines = true; // Reshape lines.
		}
		horizontal_alignment = p_alignment;
		_queue_update();
	}
}

HorizontalAlignment Label3D::get_horizontal_alignment() const { return horizontal_alignment; }

void Label3D::set_vertical_alignment(VerticalAlignment p_alignment)
{
	ERR_FAIL_INDEX((int)p_alignment, 4);
	if (vertical_alignment != p_alignment) {
		vertical_alignment = p_alignment;
		_queue_update();
	}
}

VerticalAlignment Label3D::get_vertical_alignment() const { return vertical_alignment; }

void Label3D::set_text_direction(TextServer::Direction p_text_direction)
{
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction != p_text_direction) {
		text_direction = p_text_direction;
		dirty_text = true;
		_queue_update();
	}
}

TextServer::Direction Label3D::get_text_direction() const { return text_direction; }

void Label3D::set_language(const String& p_language)
{
	if (language != p_language) {
		language = p_language;
		dirty_text = true;
		_queue_update();
	}
}

String Label3D::get_language() const { return language; }

void Label3D::set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser)
{
	if (st_parser != p_parser) {
		st_parser = p_parser;
		dirty_text = true;
		_queue_update();
	}
}

TextServer::StructuredTextParser Label3D::get_structured_text_bidi_override() const
{
	return st_parser;
}

void Label3D::set_uppercase(bool p_uppercase)
{
	if (uppercase != p_uppercase) {
		uppercase = p_uppercase;
		dirty_text = true;
		_queue_update();
	}
}

bool Label3D::is_uppercase() const { return uppercase; }

void Label3D::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);
	if (render_priority != p_priority) {
		render_priority = p_priority;
		_queue_update();
	}
}

int Label3D::get_render_priority() const { return render_priority; }

void Label3D::set_outline_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RSE::MATERIAL_RENDER_PRIORITY_MIN ||
				  p_priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);
	if (outline_render_priority != p_priority) {
		outline_render_priority = p_priority;
		_queue_update();
	}
}

int Label3D::get_outline_render_priority() const { return outline_render_priority; }

void Label3D::_font_changed()
{
	dirty_font = true;
	_queue_update();
}

Ref<Font> Label3D::get_font() const { return font_override; }

void Label3D::set_font_size(int p_size)
{
	if (font_size != p_size) {
		font_size = p_size;
		dirty_font = true;
		_queue_update();
	}
}

int Label3D::get_font_size() const { return font_size; }

void Label3D::set_outline_size(int p_size)
{
	if (outline_size != p_size) {
		outline_size = p_size;
		_queue_update();
	}
}

int Label3D::get_outline_size() const { return outline_size; }

void Label3D::set_modulate(const Color& p_color)
{
	if (modulate != p_color) {
		modulate = p_color;
		_queue_update();
	}
}

Color Label3D::get_modulate() const { return modulate; }

void Label3D::set_outline_modulate(const Color& p_color)
{
	if (outline_modulate != p_color) {
		outline_modulate = p_color;
		_queue_update();
	}
}

Color Label3D::get_outline_modulate() const { return outline_modulate; }

void Label3D::set_autowrap_mode(TextServer::AutowrapMode p_mode)
{
	if (autowrap_mode != p_mode) {
		autowrap_mode = p_mode;
		dirty_lines = true;
		_queue_update();
	}
}

TextServer::AutowrapMode Label3D::get_autowrap_mode() const { return autowrap_mode; }

void Label3D::set_autowrap_trim_flags(uint32_t p_flags)
{
	if (autowrap_flags_trim != (p_flags & TextServer::BREAK_TRIM_MASK)) {
		autowrap_flags_trim = (p_flags & TextServer::BREAK_TRIM_MASK);
		dirty_lines = true;
		_queue_update();
	}
}

uint32_t Label3D::get_autowrap_trim_flags() const { return autowrap_flags_trim; }

void Label3D::set_justification_flags(uint32_t p_flags)
{
	if (jst_flags != p_flags) {
		jst_flags = p_flags;
		dirty_lines = true;
		_queue_update();
	}
}

uint32_t Label3D::get_justification_flags() const { return jst_flags; }

void Label3D::set_width(float p_width)
{
	if (width != p_width) {
		width = p_width;
		dirty_lines = true;
		_queue_update();
	}
}

float Label3D::get_width() const { return width; }

void Label3D::set_pixel_size(real_t p_amount)
{
	if (pixel_size != p_amount) {
		pixel_size = p_amount;
		_queue_update();
	}
}

real_t Label3D::get_pixel_size() const { return pixel_size; }

void Label3D::set_offset(const Point2& p_offset)
{
	if (lbl_offset != p_offset) {
		lbl_offset = p_offset;
		_queue_update();
	}
}

Point2 Label3D::get_offset() const { return lbl_offset; }

void Label3D::set_line_spacing(float p_line_spacing)
{
	if (line_spacing != p_line_spacing) {
		line_spacing = p_line_spacing;
		_queue_update();
	}
}

float Label3D::get_line_spacing() const { return line_spacing; }

void Label3D::set_draw_flag(DrawFlags p_flag, bool p_enable)
{
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	if (flags[p_flag] != p_enable) {
		flags[p_flag] = p_enable;
		_queue_update();
	}
}

bool Label3D::get_draw_flag(DrawFlags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void Label3D::set_billboard_mode(StandardMaterial3D::BillboardMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, 3);
	if (billboard_mode != p_mode) {
		billboard_mode = p_mode;
		_queue_update();
	}
}

StandardMaterial3D::BillboardMode Label3D::get_billboard_mode() const { return billboard_mode; }

void Label3D::set_texture_filter(StandardMaterial3D::TextureFilter p_filter)
{
	if (texture_filter != p_filter) {
		texture_filter = p_filter;
		_queue_update();
	}
}

StandardMaterial3D::TextureFilter Label3D::get_texture_filter() const { return texture_filter; }

Label3D::AlphaCutMode Label3D::get_alpha_cut_mode() const { return alpha_cut; }

void Label3D::set_alpha_hash_scale(float p_hash_scale)
{
	if (alpha_hash_scale != p_hash_scale) {
		alpha_hash_scale = p_hash_scale;
		_queue_update();
	}
}

float Label3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

void Label3D::set_alpha_scissor_threshold(float p_threshold)
{
	if (alpha_scissor_threshold != p_threshold) {
		alpha_scissor_threshold = p_threshold;
		_queue_update();
	}
}

float Label3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

void Label3D::set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa)
{
	if (alpha_antialiasing_mode != p_alpha_aa) {
		alpha_antialiasing_mode = p_alpha_aa;
		_queue_update();
	}
}

StandardMaterial3D::AlphaAntiAliasing Label3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void Label3D::set_alpha_antialiasing_edge(float p_edge)
{
	if (alpha_antialiasing_edge != p_edge) {
		alpha_antialiasing_edge = p_edge;
		_queue_update();
	}
}

float Label3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

Label3D::Label3D()
{
	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = (i == FLAG_DOUBLE_SIDED);
	}

	text_rid = TS->create_shaped_text();

	mesh = Renderer::mesh_create();

	// Disable shadow casting by default to improve performance and avoid unintended visual
	// artifacts.
	render_instance.set_cast_shadows_setting(RenderInstance3D::SHADOW_CASTING_SETTING_OFF);

	// Label3D can't contribute to GI in any way, so disable it to improve performance.
	render_instance.set_gi_mode(RenderInstance3D::GI_MODE_DISABLED);

	set_base(mesh);
}

void Label3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void Label3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

Label3D::~Label3D()
{
	for (int i = 0; i < lines_rid.size(); i++) {
		TS->free_rid(lines_rid[i]);
	}
	lines_rid.clear();

	TS->free_rid(text_rid);

	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(mesh);
	for (KeyValue<SurfaceKey, SurfaceData> E : surfaces) {
		Renderer::free_rid(E.value.material);
	}
	surfaces.clear();
}

void Label3D::_queue_update() {}


Ref<Font> Label3D::_get_font_or_default() const { return Ref<Font>(); }

// ==========================================================================
// Merged from scene/3d/decal.cpp
// ==========================================================================

#include "core/os/os.h"
#include "scene/3d/visual_instance_3d.h"
#include "servers/rendering/renderer.h"

Vector3 Decal::get_size() const { return size; }

Ref<Texture2D> Decal::get_texture(DecalTexture p_type) const
{
	ERR_FAIL_INDEX_V(p_type, TEXTURE_MAX, Ref<Texture2D>());
	return textures[p_type];
}

void Decal::set_emission_energy(real_t p_energy)
{
	emission_energy = p_energy;
	Renderer::decal_set_emission_energy(decal, emission_energy);
}

real_t Decal::get_emission_energy() const { return emission_energy; }

void Decal::set_albedo_mix(real_t p_mix)
{
	albedo_mix = p_mix;
	Renderer::decal_set_albedo_mix(decal, albedo_mix);
}

real_t Decal::get_albedo_mix() const { return albedo_mix; }

void Decal::set_upper_fade(real_t p_fade)
{
	upper_fade = MAX(p_fade, 0.0);
	Renderer::decal_set_fade(decal, upper_fade, lower_fade);
}

real_t Decal::get_upper_fade() const { return upper_fade; }

void Decal::set_lower_fade(real_t p_fade)
{
	lower_fade = MAX(p_fade, 0.0);
	Renderer::decal_set_fade(decal, upper_fade, lower_fade);
}

real_t Decal::get_lower_fade() const { return lower_fade; }

void Decal::set_normal_fade(real_t p_fade)
{
	normal_fade = p_fade;
	Renderer::decal_set_normal_fade(decal, normal_fade);
}

real_t Decal::get_normal_fade() const { return normal_fade; }

void Decal::set_modulate(Color p_modulate)
{
	modulate = p_modulate;
	Renderer::decal_set_modulate(decal, p_modulate);
}

Color Decal::get_modulate() const { return modulate; }

bool Decal::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Decal::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	Renderer::decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
}

real_t Decal::get_distance_fade_begin() const { return distance_fade_begin; }

void Decal::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	Renderer::decal_set_distance_fade(
		decal, distance_fade_enabled, distance_fade_begin, distance_fade_length);
}

real_t Decal::get_distance_fade_length() const { return distance_fade_length; }

uint32_t Decal::get_cull_mask() const { return cull_mask; }

AABB Decal::get_aabb() const
{
	AABB aabb;
	aabb.position = -size / 2;
	aabb.size = size;
	return aabb;
}

PackedStringArray Decal::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (OS::get_current_rendering_method() == "gl_compatibility" ||
		OS::get_current_rendering_method() == "dummy") {
		warnings.push_back(
			RTR("Decals are only available when using the Forward+ or Mobile renderer."));
		return warnings;
	}

	if (textures[TEXTURE_ALBEDO].is_null() && textures[TEXTURE_NORMAL].is_null() &&
		textures[TEXTURE_ORM].is_null() && textures[TEXTURE_EMISSION].is_null()) {
		warnings.push_back(RTR("The decal has no textures loaded into any of its texture "
							   "properties, and will therefore not be visible."));
	}

	if ((textures[TEXTURE_NORMAL].is_valid() || textures[TEXTURE_ORM].is_valid()) &&
		textures[TEXTURE_ALBEDO].is_null()) {
		warnings.push_back(
			RTR("The decal has a Normal and/or ORM texture, but no Albedo texture is set.\nAn "
				"Albedo texture with an alpha channel is required to blend the normal/ORM maps "
				"onto the underlying surface.\nIf you don't want the Albedo texture to be visible, "
				"set Albedo Mix to 0."));
	}

	if (cull_mask == 0) {
		warnings.push_back(RTR("The decal's Cull Mask has no bits enabled, which means the decal "
							   "will not paint objects on any layer.\nTo resolve this, enable at "
							   "least one bit in the Cull Mask property."));
	}

	return warnings;
}

void Decal::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void Decal::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

Decal::Decal()
{
	decal = Renderer::decal_create();
	set_base(decal);
}

Decal::~Decal()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(decal);
}

// ==========================================================================
// Merged from scene/3d/fog_volume.cpp
// ==========================================================================

#include "core/os/os.h"
// --- Inlined from scene/3d/fog_volume.h ---
#include "core/templates/rid.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/material.h"

class FogVolume : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Vector3 size = Vector3(2, 2, 2);
	Ref<Material> material;
	RSE::FogVolumeShape shape = RSE::FOG_VOLUME_SHAPE_BOX;

	RID volume;

protected:
	_FORCE_INLINE_ RID _get_volume() { return volume; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	Vector3 get_size() const;

	void set_shape(RSE::FogVolumeShape p_type);
	RSE::FogVolumeShape get_shape() const;

	Ref<Material> get_material() const;

	AABB get_aabb() const;
	PackedStringArray get_configuration_warnings() const override;

	FogVolume();
	~FogVolume();
};

// --- End scene/3d/fog_volume.h ---
#include "scene/main/viewport.h"
#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"

Vector3 FogVolume::get_size() const { return size; }

RSE::FogVolumeShape FogVolume::get_shape() const { return shape; }

Ref<Material> FogVolume::get_material() const { return material; }

AABB FogVolume::get_aabb() const
{
	if (shape != RSE::FOG_VOLUME_SHAPE_WORLD) {
		return AABB(-size / 2, size);
	}
	return AABB();
}

PackedStringArray FogVolume::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	Ref<Environment> environment = get_viewport()->find_world_3d()->get_environment();

	if (OS::get_current_rendering_method() != "forward_plus") {
		warnings.push_back(RTR("Fog Volumes are only visible when using the Forward+ renderer."));
		return warnings;
	}

	if (environment.is_valid() && !environment->is_volumetric_fog_enabled()) {
		warnings.push_back(RTR("Fog Volumes need volumetric fog to be enabled in the scene's "
							   "Environment in order to be visible."));
	}

	return warnings;
}

void FogVolume::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void FogVolume::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

FogVolume::FogVolume()
{
	volume = Renderer::fog_volume_create();
	Renderer::fog_volume_set_shape(volume, RSE::FOG_VOLUME_SHAPE_BOX);
	set_base(volume);
}

FogVolume::~FogVolume()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(volume);
}

// ==========================================================================
// Merged from scene/3d/occluder_instance_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/marshalls.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry_2d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

RID Occluder3D::get_rid() const { return occluder; }

void Occluder3D::_update()
{
	_update_arrays(vertices, indices);

	aabb = AABB();

	const Vector3* ptr = vertices.ptr();
	for (int i = 0; i < vertices.size(); i++) {
		aabb.expand_to(ptr[i]);
	}

	debug_lines.clear();
	debug_mesh.unref();

	Renderer::occluder_set_mesh(occluder, vertices, indices);
	emit_changed();
}

PackedVector3Array Occluder3D::get_vertices() const { return vertices; }

PackedInt32Array Occluder3D::get_indices() const { return indices; }

Vector<Vector3> Occluder3D::get_debug_lines() const
{
	if (!debug_lines.is_empty()) {
		return debug_lines;
	}

	if (indices.size() % 3 != 0) {
		return Vector<Vector3>();
	}

	const Vector3* vertices_ptr = vertices.ptr();
	debug_lines.resize(indices.size() / 3 * 6);
	Vector3* line_ptr = debug_lines.ptrw();
	int line_i = 0;
	for (int i = 0; i < indices.size() / 3; i++) {
		for (int j = 0; j < 3; j++) {
			int a = indices[i * 3 + j];
			int b = indices[i * 3 + (j + 1) % 3];
			ERR_FAIL_INDEX_V_MSG(
				a, vertices.size(), Vector<Vector3>(), "Occluder indices are out of range.");
			ERR_FAIL_INDEX_V_MSG(
				b, vertices.size(), Vector<Vector3>(), "Occluder indices are out of range.");
			line_ptr[line_i++] = vertices_ptr[a];
			line_ptr[line_i++] = vertices_ptr[b];
		}
	}
	return debug_lines;
}

AABB Occluder3D::get_aabb() const { return aabb; }

void Occluder3D::set_occluder_type(OccluderType p_type)
{
	if (occluder_type == p_type) {
		return;
	}
	occluder_type = p_type;
	_update();
}

Occluder3D::OccluderType Occluder3D::get_occluder_type() const { return occluder_type; }

void Occluder3D::set_arrays(PackedVector3Array p_vertices, PackedInt32Array p_indices)
{
	array_vertices = p_vertices;
	array_indices = p_indices;
	if (occluder_type == OCCLUDER_ARRAY) {
		_update();
	}
}

void Occluder3D::set_vertices(PackedVector3Array p_vertices)
{
	array_vertices = p_vertices;
	if (occluder_type == OCCLUDER_ARRAY) {
		_update();
	}
}

void Occluder3D::set_indices(PackedInt32Array p_indices)
{
	array_indices = p_indices;
	if (occluder_type == OCCLUDER_ARRAY) {
		_update();
	}
}

void Occluder3D::set_quad_size(const Size2& p_size)
{
	if (quad_size == p_size) {
		return;
	}

	quad_size = p_size.maxf(0);
	if (occluder_type == OCCLUDER_QUAD) {
		_update();
	}
}

Size2 Occluder3D::get_quad_size() const { return quad_size; }

void Occluder3D::set_box_size(const Vector3& p_size)
{
	if (box_size == p_size) {
		return;
	}

	box_size = p_size.maxf(0);
	if (occluder_type == OCCLUDER_BOX) {
		_update();
	}
}

Vector3 Occluder3D::get_box_size() const { return box_size; }

void Occluder3D::set_radius(float p_radius)
{
	if (radius == p_radius) {
		return;
	}

	radius = MAX(p_radius, 0.0f);
	if (occluder_type == OCCLUDER_SPHERE) {
		_update();
	}
}

float Occluder3D::get_radius() const { return radius; }

void Occluder3D::set_polygon(const Vector<Vector2>& p_polygon)
{
	polygon = p_polygon;
	if (occluder_type == OCCLUDER_POLYGON) {
		_update();
	}
}

Vector<Vector2> Occluder3D::get_polygon() const { return polygon; }

bool Occluder3D::_has_editable_3d_polygon_no_depth() const { return false; }

void Occluder3D::_update_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices)
{
	switch (occluder_type) {
	case OCCLUDER_ARRAY: {
		r_vertices = array_vertices;
		r_indices = array_indices;
	} break;
	case OCCLUDER_QUAD: {
		Size2 _size = Size2(quad_size.x / 2.0f, quad_size.y / 2.0f);

		r_vertices = {
			Vector3(-_size.x, -_size.y, 0),
			Vector3(-_size.x, _size.y, 0),
			Vector3(_size.x, _size.y, 0),
			Vector3(_size.x, -_size.y, 0),
		};

		r_indices = {0, 1, 2, 0, 2, 3};
	} break;
	case OCCLUDER_BOX: {
		Vector3 _size = Vector3(box_size.x / 2.0f, box_size.y / 2.0f, box_size.z / 2.0f);

		r_vertices = {
			// front
			Vector3(-_size.x, -_size.y, _size.z),
			Vector3(_size.x, -_size.y, _size.z),
			Vector3(_size.x, _size.y, _size.z),
			Vector3(-_size.x, _size.y, _size.z),
			// back
			Vector3(-_size.x, -_size.y, -_size.z),
			Vector3(_size.x, -_size.y, -_size.z),
			Vector3(_size.x, _size.y, -_size.z),
			Vector3(-_size.x, _size.y, -_size.z),
		};

		r_indices = {// front
			0, 1, 2, 2, 3, 0,
			// right
			1, 5, 6, 6, 2, 1,
			// back
			7, 6, 5, 5, 4, 7,
			// left
			4, 0, 3, 3, 7, 4,
			// bottom
			4, 5, 1, 1, 0, 4,
			// top
			3, 2, 6, 6, 7, 3};
	} break;
	case OCCLUDER_SPHERE: {
		r_vertices.resize((RINGS + 2) * (RADIAL_SEGMENTS + 1));
		int vertex_i = 0;
		Vector3* vertex_ptr = r_vertices.ptrw();

		r_indices.resize((RINGS + 1) * RADIAL_SEGMENTS * 6);
		int idx_i = 0;
		int* idx_ptr = r_indices.ptrw();

		int current_row = 0;
		int previous_row = 0;
		int point = 0;
		for (int j = 0; j <= (RINGS + 1); j++) {
			float v = j / float(RINGS + 1);
			float w = Math::sin(Math::PI * v);
			float y = Math::cos(Math::PI * v);
			for (int i = 0; i <= RADIAL_SEGMENTS; i++) {
				float u = i / float(RADIAL_SEGMENTS);

				float x = Math::cos(u * Math::TAU);
				float z = Math::sin(u * Math::TAU);
				vertex_ptr[vertex_i++] = Vector3(x * w, y, z * w) * radius;

				if (i > 0 && j > 0) {
					idx_ptr[idx_i++] = previous_row + i - 1;
					idx_ptr[idx_i++] = previous_row + i;
					idx_ptr[idx_i++] = current_row + i - 1;

					idx_ptr[idx_i++] = previous_row + i;
					idx_ptr[idx_i++] = current_row + i;
					idx_ptr[idx_i++] = current_row + i - 1;
				}

				point++;
			}

			previous_row = current_row;
			current_row = point;
		}
	} break;
	case OCCLUDER_POLYGON: {
		if (polygon.size() < 3) {
			r_vertices.clear();
			r_indices.clear();
			return;
		}

		Vector<Point2> occluder_polygon = polygon;
		if (Triangulate::get_area(occluder_polygon) > 0) {
			occluder_polygon.reverse();
		}

		Vector<int> occluder_indices = Geometry2D::triangulate_polygon(occluder_polygon);

		if (occluder_indices.size() < 3) {
			r_vertices.clear();
			r_indices.clear();
			ERR_FAIL_MSG("Failed to triangulate Occluder3D. Make sure the polygon doesn't have "
						 "any intersecting edges.");
		}

		r_vertices.resize(occluder_polygon.size());
		Vector3* vertex_ptr = r_vertices.ptrw();
		const Vector2* polygon_ptr = occluder_polygon.ptr();
		for (int i = 0; i < occluder_polygon.size(); i++) {
			vertex_ptr[i] = Vector3(polygon_ptr[i].x, polygon_ptr[i].y, 0.0);
		}

		r_indices.resize(occluder_indices.size());
		memcpy(r_indices.ptrw(), occluder_indices.ptr(), occluder_indices.size() * sizeof(int));
	} break;
	}
}

Occluder3D::Occluder3D(OccluderType p_type) : occluder_type(p_type)
{
	occluder = Renderer::occluder_create();
	if (occluder_type != OCCLUDER_ARRAY && occluder_type != OCCLUDER_POLYGON) {
		_update();
	}
}

Occluder3D::~Occluder3D()
{
	if (occluder.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(occluder);
	}
}

/////////////////////////////////////////////////

AABB OccluderInstance3D::get_aabb() const
{
	if (occluder.is_valid()) {
		return occluder->get_aabb();
	}
	return AABB();
}

Ref<Occluder3D> OccluderInstance3D::get_occluder() const { return occluder; }

uint32_t OccluderInstance3D::get_bake_mask() const { return bake_mask; }

void OccluderInstance3D::set_bake_simplification_distance(float p_dist)
{
	bake_simplification_dist = MAX(p_dist, 0.0f);
}

float OccluderInstance3D::get_bake_simplification_distance() const
{
	return bake_simplification_dist;
}

bool OccluderInstance3D::get_bake_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return bake_mask & (1 << (p_layer_number - 1));
}

OccluderInstance3D::BakeError OccluderInstance3D::bake_scene(
	Node* p_from_node, String p_occluder_path)
{
	if (p_occluder_path.is_empty()) {
		if (get_occluder().is_null()) {
			return BAKE_ERROR_NO_SAVE_PATH;
		}
		p_occluder_path = get_occluder()->get_path();
		if (!p_occluder_path.is_resource_file()) {
			return BAKE_ERROR_NO_SAVE_PATH;
		}
	}

	PackedVector3Array vertices;
	PackedInt32Array indices;

	_bake_node(p_from_node, vertices, indices);

	if (vertices.is_empty() || indices.is_empty()) {
		return BAKE_ERROR_NO_MESHES;
	}

	Ref<Occluder3D> occ;
	if (get_occluder().is_valid()) {
		if (get_occluder()->get_occluder_type() == Occluder3D::OCCLUDER_ARRAY) {
			occ = get_occluder();
		}
		set_occluder(Ref<Occluder3D>()); // clear
	}

	if (occ.is_null()) {
		occ = memnew(Occluder3D(Occluder3D::OCCLUDER_ARRAY));
	}

	occ->set_arrays(vertices, indices);

	Error err = ResourceSaver::save(occ.ptr(), p_occluder_path);

	if (err != OK) {
		return BAKE_ERROR_CANT_SAVE;
	}

	occ->set_path(p_occluder_path);
	set_occluder(occ);

	return BAKE_ERROR_OK;
}

PackedStringArray OccluderInstance3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!bool(GLOBAL_GET_CACHED(bool, "rendering/occlusion_culling/use_occlusion_culling"))) {
		warnings.push_back(RTR(
			"Occlusion culling is disabled in the Project Settings, which means occlusion culling "
			"won't be performed in the root viewport.\nTo resolve this, open the Project Settings "
			"and enable Rendering > Occlusion Culling > Use Occlusion Culling."));
	}

	if (bake_mask == 0) {
		warnings.push_back(RTR("The Bake Mask has no bits enabled, which means baking will not "
							   "produce any occluder meshes for this OccluderInstance3D.\nTo "
							   "resolve this, enable at least one bit in the Bake Mask property."));
	}

	if (occluder.is_null()) {
		warnings.push_back(
			RTR("No occluder mesh is defined in the Occluder property, so no occlusion culling "
				"will be performed using this OccluderInstance3D.\nTo resolve this, set the "
				"Occluder property to one of the primitive occluder types or bake the scene meshes "
				"by selecting the OccluderInstance3D and pressing the Bake Occluders button at the "
				"top of the 3D editor viewport."));
	}
	else {
		if (occluder->get_occluder_type() == Occluder3D::OCCLUDER_ARRAY &&
			occluder->get_indices().size() < 3) {
			// Setting a new array Occluder3D from the inspector will create an empty occluder,
			// so warn the user about this.
			warnings.push_back(
				RTR("The occluder mesh has less than 3 vertices, so no occlusion culling will be "
					"performed using this OccluderInstance3D.\nTo generate a proper occluder mesh, "
					"select the OccluderInstance3D then use the Bake Occluders button at the top "
					"of the 3D editor viewport."));
		}
		if (occluder->get_occluder_type() == Occluder3D::OCCLUDER_POLYGON &&
			occluder->get_polygon().size() < 3) {
			warnings.push_back(RTR(
				"The polygon occluder has less than 3 vertices, so no occlusion culling will be "
				"performed using this OccluderInstance3D.\nVertices can be added in the inspector "
				"or using the polygon editing tools at the top of the 3D editor viewport."));
		}
	}

	return warnings;
}

bool OccluderInstance3D::_is_editable_3d_polygon() const
{
	return occluder.is_valid() && occluder->get_occluder_type() == Occluder3D::OCCLUDER_POLYGON;
}

Ref<Resource> OccluderInstance3D::_get_editable_3d_polygon_resource() const { return occluder; }


void OccluderInstance3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void OccluderInstance3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

OccluderInstance3D::OccluderInstance3D() {}

OccluderInstance3D::~OccluderInstance3D() {}



void OccluderInstance3D::set_occluder(Ref<Occluder3D> const&) {}

void OccluderInstance3D::_bake_node(Node*, Vector<Vector3>&, Vector<int>&) {}

// ==========================================================================
// Merged from scene/3d/visible_on_screen_notifier_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/visible_on_screen_notifier_3d.h ---
#include "scene/3d/visual_instance_3d.h"

class VisibleOnScreenNotifier3D : public Node3D
{
public:
	enum EnableMode
	{
		ENABLE_MODE_INHERIT,
		ENABLE_MODE_ALWAYS,
		ENABLE_MODE_WHEN_PAUSED,
	};

	RenderInstance3D render_instance;

private:
	AABB aabb = AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	bool on_screen = false;
	bool enabler_enabled = false;
	EnableMode enable_mode = ENABLE_MODE_INHERIT;
	NodePath enable_node_path = NodePath("..");

	void _visibility_enter();
	void _visibility_exit();
	void _update_enable_mode(bool p_enable);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_aabb(const AABB& p_aabb);
	AABB get_aabb() const;
	bool is_on_screen() const;

	void set_enabler_enabled(bool p_enabled);
	bool is_enabler_enabled() const { return enabler_enabled; }

	void set_enable_mode(EnableMode p_mode);
	EnableMode get_enable_mode() const { return enable_mode; }

	void set_enable_node_path(const NodePath& p_path);
	NodePath get_enable_node_path() const { return enable_node_path; }

	VisibleOnScreenNotifier3D(bool p_enabler = false);
	~VisibleOnScreenNotifier3D();
};

// --- End scene/3d/visible_on_screen_notifier_3d.h ---

#include "core/config/engine.h"
#include "servers/rendering/renderer.h"

void VisibleOnScreenNotifier3D::set_aabb(const AABB& p_aabb) { aabb = p_aabb; }

AABB VisibleOnScreenNotifier3D::get_aabb() const { return aabb; }

bool VisibleOnScreenNotifier3D::is_on_screen() const { return on_screen; }

void VisibleOnScreenNotifier3D::set_enabler_enabled(bool p_enabled)
{
	enabler_enabled = p_enabled;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier3D::set_enable_mode(EnableMode p_mode)
{
	enable_mode = p_mode;
	if (enabler_enabled && is_inside_tree()) {
		_update_enable_mode(on_screen);
	}
}

void VisibleOnScreenNotifier3D::set_enable_node_path(const NodePath& p_path)
{
	enable_node_path = p_path;
}

void VisibleOnScreenNotifier3D::_visibility_enter()
{
	on_screen = true;
	if (enabler_enabled) {
		_update_enable_mode(true);
	}
}

void VisibleOnScreenNotifier3D::_visibility_exit()
{
	on_screen = false;
	if (enabler_enabled) {
		_update_enable_mode(false);
	}
}

void VisibleOnScreenNotifier3D::_update_enable_mode(bool p_enable)
{
	if (Engine::is_editor_hint()) {
		return;
	}
	Node* node = get_node_or_null(enable_node_path);
	if (!node) {
		return;
	}
	if (p_enable) {
		switch (enable_mode) {
		case ENABLE_MODE_INHERIT:
			node->set_process_mode(PROCESS_MODE_INHERIT);
			break;
		case ENABLE_MODE_ALWAYS:
			node->set_process_mode(PROCESS_MODE_ALWAYS);
			break;
		case ENABLE_MODE_WHEN_PAUSED:
			node->set_process_mode(PROCESS_MODE_WHEN_PAUSED);
			break;
		}
	}
	else {
		node->set_process_mode(PROCESS_MODE_DISABLED);
	}
}

void VisibleOnScreenNotifier3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void VisibleOnScreenNotifier3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
	case NOTIFICATION_EXIT_TREE: {
		on_screen = false;
	} break;
	}
}

VisibleOnScreenNotifier3D::VisibleOnScreenNotifier3D(bool p_enabler) : enabler_enabled(p_enabler) {}

VisibleOnScreenNotifier3D::~VisibleOnScreenNotifier3D()
{
	RID base_old = get_base();
	set_base(RID());
	if (base_old.is_valid() && Renderer::data) {
		Renderer::free_rid(base_old);
	}
}

// ============================================================================
// Merged from scene/3d/cpu_particles_3d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/math/random_pcg.h"
// --- Inlined from scene/3d/cpu_particles_3d.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/curve.h"
#include "scene/resources/gradient.h"

class Mesh;
class RandomNumberGenerator;

class CPUParticles3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawOrder
	{
		DRAW_ORDER_INDEX,
		DRAW_ORDER_LIFETIME,
		DRAW_ORDER_VIEW_DEPTH,
		DRAW_ORDER_MAX
	};

	enum Parameter
	{
		PARAM_INITIAL_LINEAR_VELOCITY,
		PARAM_ANGULAR_VELOCITY,
		PARAM_ORBIT_VELOCITY,
		PARAM_LINEAR_ACCEL,
		PARAM_RADIAL_ACCEL,
		PARAM_TANGENTIAL_ACCEL,
		PARAM_DAMPING,
		PARAM_ANGLE,
		PARAM_SCALE,
		PARAM_HUE_VARIATION,
		PARAM_ANIM_SPEED,
		PARAM_ANIM_OFFSET,
		PARAM_MAX
	};

	enum ParticleFlags
	{
		PARTICLE_FLAG_ALIGN_Y_TO_VELOCITY,
		PARTICLE_FLAG_ROTATE_Y,
		PARTICLE_FLAG_DISABLE_Z,
		PARTICLE_FLAG_MAX
	};

	enum EmissionShape
	{
		EMISSION_SHAPE_POINT,
		EMISSION_SHAPE_SPHERE,
		EMISSION_SHAPE_SPHERE_SURFACE,
		EMISSION_SHAPE_BOX,
		EMISSION_SHAPE_POINTS,
		EMISSION_SHAPE_DIRECTED_POINTS,
		EMISSION_SHAPE_RING,
		EMISSION_SHAPE_MAX
	};

private:
	bool emitting = false;
	bool active = false;

	struct Particle
	{
		Transform3D transform;
		Color color;
		real_t custom[4] = {};
		Vector3 velocity;
		bool active = false;
		real_t angle_rand = 0.0;
		real_t scale_rand = 0.0;
		real_t hue_rot_rand = 0.0;
		real_t anim_offset_rand = 0.0;
		Color start_color_rand;
		double time = 0.0;
		double lifetime = 0.0;
		Color base_color;

		uint32_t seed = 0;
	};

	double time = 0.0;
	double frame_remainder = 0.0;
	int cycle = 0;
	bool redraw = false;

	RID multimesh;

	Vector<Particle> particles;
	Vector<float> particle_data;
	Vector<int> particle_order;

	struct SortLifetime
	{
		const Particle* particles = nullptr;

		bool operator()(int p_a, int p_b) const
		{
			return particles[p_a].time > particles[p_b].time;
		}
	};

	struct SortAxis
	{
		const Particle* particles = nullptr;
		Vector3 axis;

		bool operator()(int p_a, int p_b) const
		{
			return axis.dot(particles[p_a].transform.origin) <
				   axis.dot(particles[p_b].transform.origin);
		}
	};

	//

	bool one_shot = false;

	double lifetime = 1.0;
	double pre_process_time = 0.0;
	real_t _request_process_time = 0.0;
	real_t _request_process_time_residual = 0.0;
	real_t explosiveness_ratio = 0.0;
	real_t randomness_ratio = 0.0;
	double lifetime_randomness = 0.0;
	double speed_scale = 1.0;
	AABB visibility_aabb;
	bool local_coords = false;
	int fixed_fps = 0;
	bool fractional_delta = true;
	uint32_t seed = 0;
	bool use_fixed_seed = false;

	Transform3D inv_emission_transform;

	SafeFlag can_update;

	DrawOrder draw_order = DRAW_ORDER_INDEX;

	Ref<Mesh> mesh;

	////////

	Vector3 direction = Vector3(1, 0, 0);
	real_t spread = 45.0;
	real_t flatness = 0.0;

	real_t parameters_min[PARAM_MAX] = {};
	real_t parameters_max[PARAM_MAX] = {};

	Ref<Curve> curve_parameters[PARAM_MAX];
	Color color = Color(1, 1, 1, 1);
	Ref<Gradient> color_ramp;
	Ref<Gradient> color_initial_ramp;

	bool particle_flags[PARTICLE_FLAG_MAX] = {};

	EmissionShape emission_shape = EMISSION_SHAPE_POINT;
	real_t emission_sphere_radius = 1.0;
	Vector3 emission_box_extents = Vector3(1, 1, 1);
	Vector<Vector3> emission_points;
	Vector<Vector3> emission_normals;
	Vector<Color> emission_colors;
	Vector3 emission_ring_axis;
	real_t emission_ring_height = 0.0;
	real_t emission_ring_radius = 0.0;
	real_t emission_ring_inner_radius = 0.0;
	real_t emission_ring_cone_angle = 0.0;

	Ref<Curve> scale_curve_x;
	Ref<Curve> scale_curve_y;
	Ref<Curve> scale_curve_z;
	bool split_scale = false;

	Vector3 gravity = Vector3(0, -9.8, 0);

	Ref<RandomNumberGenerator> rng;

	void _update_internal();
	void _particles_process(double p_delta);
	void _update_particle_data_buffer();
	void _set_emitting();

	Mutex update_mutex;

	void _update_render_thread();

	void _set_redraw(bool p_redraw);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	AABB get_aabb() const;

	void set_emitting(bool p_emitting);
	void set_amount(int p_amount);
	void set_lifetime(double p_lifetime);
	void set_one_shot(bool p_one_shot);
	void set_pre_process_time(double p_time);
	void set_explosiveness_ratio(real_t p_ratio);
	void set_randomness_ratio(real_t p_ratio);
	void set_lifetime_randomness(double p_random);
	void set_use_local_coordinates(bool p_enable);
	void set_speed_scale(double p_scale);

	bool is_emitting() const;
	int get_amount() const;
	double get_lifetime() const;
	bool get_one_shot() const;
	double get_pre_process_time() const;
	real_t get_explosiveness_ratio() const;
	real_t get_randomness_ratio() const;
	AABB get_visibility_aabb() const;
	double get_lifetime_randomness() const;
	bool get_use_local_coordinates() const;
	double get_speed_scale() const;

	void set_fixed_fps(int p_count);
	int get_fixed_fps() const;

	void set_fractional_delta(bool p_enable);
	bool get_fractional_delta() const;

	void set_draw_order(DrawOrder p_order);
	DrawOrder get_draw_order() const;

	Ref<Mesh> get_mesh() const;

	void set_use_fixed_seed(bool p_use_fixed_seed = false);
	bool get_use_fixed_seed() const;

	void set_seed(uint32_t p_seed);
	uint32_t get_seed() const;

	void request_particles_process(
		real_t p_requested_process_time, real_t p_request_process_time_residual = 0.0f);

	///////////////////

	void set_direction(Vector3 p_direction);
	Vector3 get_direction() const;

	void set_spread(real_t p_spread);
	real_t get_spread() const;

	void set_flatness(real_t p_flatness);
	real_t get_flatness() const;

	real_t get_param_min(Parameter p_param) const;

	real_t get_param_max(Parameter p_param) const;

	Ref<Curve> get_param_curve(Parameter p_param) const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_color_ramp(const Ref<Gradient>& p_ramp);
	Ref<Gradient> get_color_ramp() const;

	void set_color_initial_ramp(const Ref<Gradient>& p_ramp);
	Ref<Gradient> get_color_initial_ramp() const;

	void set_particle_flag(ParticleFlags p_particle_flag, bool p_enable);
	bool get_particle_flag(ParticleFlags p_particle_flag) const;

	void set_emission_points(const Vector<Vector3>& p_points);
	void set_emission_normals(const Vector<Vector3>& p_normals);
	void set_emission_colors(const Vector<Color>& p_colors);
	void set_scale_curve_x(Ref<Curve> p_scale_curve);
	void set_scale_curve_y(Ref<Curve> p_scale_curve);
	void set_scale_curve_z(Ref<Curve> p_scale_curve);
	void set_split_scale(bool p_split_scale);

	EmissionShape get_emission_shape() const;
	real_t get_emission_sphere_radius() const;
	Vector3 get_emission_box_extents() const;
	Vector<Vector3> get_emission_points() const;
	Vector<Vector3> get_emission_normals() const;
	Vector<Color> get_emission_colors() const;
	Vector3 get_emission_ring_axis() const;
	real_t get_emission_ring_height() const;
	real_t get_emission_ring_radius() const;
	real_t get_emission_ring_inner_radius() const;
	real_t get_emission_ring_cone_angle() const;
	Ref<Curve> get_scale_curve_x() const;
	Ref<Curve> get_scale_curve_y() const;
	Ref<Curve> get_scale_curve_z() const;
	bool get_split_scale();

	void set_gravity(const Vector3& p_gravity);
	Vector3 get_gravity() const;

	PackedStringArray get_configuration_warnings() const override;

	void restart(bool p_keep_seed = false);

	void convert_from_particles(Node* p_particles);

	AABB capture_aabb() const;

	CPUParticles3D() = default;
	~CPUParticles3D();
};



// --- End scene/3d/cpu_particles_3d.h ---
#include "scene/3d/visual_instance_3d.h"
// --- Inlined from scene/3d/gpu_particles_3d.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/skeleton_3d.h"

class Mesh;

class GPUParticles3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawOrder
	{
		DRAW_ORDER_INDEX,
		DRAW_ORDER_LIFETIME,
		DRAW_ORDER_REVERSE_LIFETIME,
		DRAW_ORDER_VIEW_DEPTH,
	};

	enum TransformAlign
	{
		TRANSFORM_ALIGN_DISABLED = RSE::PARTICLES_TRANSFORM_ALIGN_DISABLED,
		TRANSFORM_ALIGN_Z_BILLBOARD = RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD,
		TRANSFORM_ALIGN_Y_TO_VELOCITY = RSE::PARTICLES_TRANSFORM_ALIGN_Y_TO_VELOCITY,
		TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY =
			RSE::PARTICLES_TRANSFORM_ALIGN_Z_BILLBOARD_Y_TO_VELOCITY,
		TRANSFORM_ALIGN_LOCAL_BILLBOARD = RSE::PARTICLES_TRANSFORM_ALIGN_LOCAL_BILLBOARD,
	};

	enum
	{
		MAX_DRAW_PASSES = 4
	};

private:
	RID particles;

	bool emitting = false;
	bool active = false;
	bool signal_canceled = false;
	bool one_shot = false;
	int amount = 0;
	float amount_ratio = 1.0;
	double lifetime = 0.0;
	double pre_process_time = 0.0;
	real_t explosiveness_ratio = 0.0;
	real_t randomness_ratio = 0.0;
	double speed_scale = 0.0;
	AABB visibility_aabb;
	bool local_coords = false;
	int fixed_fps = 0;
	bool fractional_delta = false;
	bool interpolate = true;
	NodePath sub_emitter;
	real_t collision_base_size = 0.01;
	uint32_t seed = 0;
	bool use_fixed_seed = false;

	bool trail_enabled = false;
	double trail_lifetime = 0.3;

	TransformAlign transform_align = TRANSFORM_ALIGN_DISABLED;
	RSE::ParticlesTransformAlignCustomSrc transform_align_channel_filter =
		RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_X;
	RSE::ParticlesTransformAlignAxis transform_align_axis =
		RSE::ParticlesTransformAlignAxis::PARTICLES_ALIGN_AXIS_Y;
	bool transform_align_use_velocity = true;

	Ref<Material> process_material;

	DrawOrder draw_order = DRAW_ORDER_INDEX;

	Vector<Ref<Mesh>> draw_passes;
	Ref<Skin> skin;

	double time = 0.0;
	double emission_time = 0.0;
	double active_time = 0.0;
	float interp_to_end_factor = 0;
	Vector3 previous_velocity;
	Vector3 previous_position;

	void _attach_sub_emitter();

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	AABB get_aabb() const;

	void set_emitting(bool p_emitting);
	void set_amount(int p_amount);
	void set_lifetime(double p_lifetime);
	void set_one_shot(bool p_one_shot);
	void set_pre_process_time(double p_time);
	void set_explosiveness_ratio(real_t p_ratio);
	void set_randomness_ratio(real_t p_ratio);
	void set_use_local_coordinates(bool p_enable);
	void set_process_material(const Ref<Material>& p_material);
	void set_speed_scale(double p_scale);
	void set_collision_base_size(real_t p_ratio);
	void set_trail_lifetime(double p_seconds);
	void set_interp_to_end(float p_interp);

	bool is_emitting() const;
	int get_amount() const;

	double get_lifetime() const;
	bool get_one_shot() const;
	double get_pre_process_time() const;
	real_t get_explosiveness_ratio() const;
	real_t get_randomness_ratio() const;
	AABB get_visibility_aabb() const;
	bool get_use_local_coordinates() const;
	Ref<Material> get_process_material() const;
	double get_speed_scale() const;
	real_t get_collision_base_size() const;
	bool is_trail_enabled() const;
	double get_trail_lifetime() const;
	float get_interp_to_end() const;

	void set_amount_ratio(float p_ratio);
	float get_amount_ratio() const;

	void set_fixed_fps(int p_count);
	int get_fixed_fps() const;

	void set_fractional_delta(bool p_enable);
	bool get_fractional_delta() const;

	void set_interpolate(bool p_enable);
	bool get_interpolate() const;

	void set_draw_order(DrawOrder p_order);
	DrawOrder get_draw_order() const;

	void set_draw_passes(int p_count);
	int get_draw_passes() const;

	void set_draw_pass_mesh(int p_pass, const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_draw_pass_mesh(int p_pass) const;

	PackedStringArray get_configuration_warnings() const override;

	NodePath get_sub_emitter() const;

	Ref<Skin> get_skin() const;

	void set_transform_align(TransformAlign p_align);
	TransformAlign get_transform_align() const;

	void set_transform_align_channel_filter(
		RSE::ParticlesTransformAlignCustomSrc p_align_channel_filter);
	RSE::ParticlesTransformAlignCustomSrc get_transform_align_channel_filter() const;

	void set_transform_align_axis(RSE::ParticlesTransformAlignAxis p_axis);
	RSE::ParticlesTransformAlignAxis get_transform_align_axis() const;

	void restart(bool p_keep_seed = false);

	void set_use_fixed_seed(bool p_use_fixed_seed);
	bool get_use_fixed_seed() const;

	void set_seed(uint32_t p_seed);
	uint32_t get_seed() const;
	void request_particles_process(
		real_t p_requested_process_time, real_t p_request_process_time_residual = 0.0f);

	enum EmitFlags
	{
		EMIT_FLAG_POSITION = RSE::PARTICLES_EMIT_FLAG_POSITION,
		EMIT_FLAG_ROTATION_SCALE = RSE::PARTICLES_EMIT_FLAG_ROTATION_SCALE,
		EMIT_FLAG_VELOCITY = RSE::PARTICLES_EMIT_FLAG_VELOCITY,
		EMIT_FLAG_COLOR = RSE::PARTICLES_EMIT_FLAG_COLOR,
		EMIT_FLAG_CUSTOM = RSE::PARTICLES_EMIT_FLAG_CUSTOM
	};

	void emit_particle(const Transform3D& p_transform, const Vector3& p_velocity,
		const Color& p_color, const Color& p_custom, uint32_t p_emit_flags);

	AABB capture_aabb() const;
	void convert_from_particles(Node* p_particles);

	GPUParticles3D();
	~GPUParticles3D();
};



// --- End scene/3d/gpu_particles_3d.h ---
#include "scene/main/viewport.h"
#include "scene/resources/material.h"
#include "scene/resources/gradient.h"
#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

AABB CPUParticles3D::get_aabb() const { return AABB(); }

void CPUParticles3D::set_emitting(bool p_emitting)
{
	if (emitting == p_emitting) {
		return;
	}

	if (p_emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}

	emitting = p_emitting;
	if (emitting) {
		_set_emitting();
	}
}

void CPUParticles3D::_set_emitting()
{
	active = true;
	set_process_internal(true);
	// first update before rendering to avoid one frame delay after emitting starts
	if (time == 0) {
		_update_internal();
	}
}

void CPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles must be greater than 0.");

	particles.resize(p_amount);
	{
		Particle* w = particles.ptrw();

		for (int i = 0; i < p_amount; i++) {
			w[i].active = false;
			w[i].custom[3] = 1.0; // Make sure w component isn't garbage data and doesn't break
								  // shaders with CUSTOM.y/Custom.w
		}
	}

	particle_data.resize((12 + 4 + 4) * p_amount);
	Renderer::multimesh_set_visible_instances(multimesh, -1);
	Renderer::multimesh_allocate_data(
		multimesh, p_amount, RSE::MULTIMESH_TRANSFORM_3D, true, true);

	particle_order.resize(p_amount);
}

void CPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
}

void CPUParticles3D::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }

void CPUParticles3D::set_pre_process_time(double p_time) { pre_process_time = p_time; }

void CPUParticles3D::set_explosiveness_ratio(real_t p_ratio) { explosiveness_ratio = p_ratio; }

void CPUParticles3D::set_randomness_ratio(real_t p_ratio) { randomness_ratio = p_ratio; }

void CPUParticles3D::set_lifetime_randomness(double p_random) { lifetime_randomness = p_random; }

void CPUParticles3D::set_use_local_coordinates(bool p_enable) { local_coords = p_enable; }

void CPUParticles3D::set_speed_scale(double p_scale) { speed_scale = p_scale; }

bool CPUParticles3D::is_emitting() const { return emitting; }

int CPUParticles3D::get_amount() const { return particles.size(); }

double CPUParticles3D::get_lifetime() const { return lifetime; }

bool CPUParticles3D::get_one_shot() const { return one_shot; }

double CPUParticles3D::get_pre_process_time() const { return pre_process_time; }

real_t CPUParticles3D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t CPUParticles3D::get_randomness_ratio() const { return randomness_ratio; }

AABB CPUParticles3D::get_visibility_aabb() const { return visibility_aabb; }

double CPUParticles3D::get_lifetime_randomness() const { return lifetime_randomness; }

bool CPUParticles3D::get_use_local_coordinates() const { return local_coords; }

double CPUParticles3D::get_speed_scale() const { return speed_scale; }

void CPUParticles3D::set_draw_order(DrawOrder p_order)
{
	ERR_FAIL_INDEX(p_order, DRAW_ORDER_MAX);
	draw_order = p_order;
}

CPUParticles3D::DrawOrder CPUParticles3D::get_draw_order() const { return draw_order; }

Ref<Mesh> CPUParticles3D::get_mesh() const { return mesh; }

void CPUParticles3D::set_fixed_fps(int p_count) { fixed_fps = p_count; }

int CPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void CPUParticles3D::set_fractional_delta(bool p_enable) { fractional_delta = p_enable; }

bool CPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void CPUParticles3D::restart(bool p_keep_seed)
{
	time = 0;
	frame_remainder = 0;
	cycle = 0;
	emitting = false;

	{
		int pc = particles.size();
		Particle* w = particles.ptrw();

		for (int i = 0; i < pc; i++) {
			w[i].active = false;
		}
	}
	if (!p_keep_seed && !use_fixed_seed) {
		seed = Math::rand();
	}

	emitting = true;
	_set_emitting();
}

void CPUParticles3D::set_direction(Vector3 p_direction) { direction = p_direction; }

Vector3 CPUParticles3D::get_direction() const { return direction; }

void CPUParticles3D::set_spread(real_t p_spread) { spread = p_spread; }

real_t CPUParticles3D::get_spread() const { return spread; }

void CPUParticles3D::set_flatness(real_t p_flatness) { flatness = p_flatness; }

real_t CPUParticles3D::get_flatness() const { return flatness; }

real_t CPUParticles3D::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_min[p_param];
}

real_t CPUParticles3D::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return parameters_max[p_param];
}

static void _adjust_curve_range(const Ref<Curve>& p_curve, real_t p_min, real_t p_max)
{
	Ref<Curve> curve = p_curve;
	if (curve.is_null()) {
		return;
	}

	curve->ensure_default_setup(p_min, p_max);
}

Ref<Curve> CPUParticles3D::get_param_curve(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Curve>());

	return curve_parameters[p_param];
}

void CPUParticles3D::set_color(const Color& p_color) { color = p_color; }

Color CPUParticles3D::get_color() const { return color; }

void CPUParticles3D::set_color_ramp(const Ref<Gradient>& p_ramp) { color_ramp = p_ramp; }

Ref<Gradient> CPUParticles3D::get_color_ramp() const { return color_ramp; }

void CPUParticles3D::set_color_initial_ramp(const Ref<Gradient>& p_ramp)
{
	color_initial_ramp = p_ramp;
}

Ref<Gradient> CPUParticles3D::get_color_initial_ramp() const { return color_initial_ramp; }

bool CPUParticles3D::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

void CPUParticles3D::set_emission_points(const Vector<Vector3>& p_points)
{
	emission_points = p_points;
}

void CPUParticles3D::set_emission_normals(const Vector<Vector3>& p_normals)
{
	emission_normals = p_normals;
}

void CPUParticles3D::set_emission_colors(const Vector<Color>& p_colors)
{
	emission_colors = p_colors;
}

void CPUParticles3D::set_scale_curve_x(Ref<Curve> p_scale_curve) { scale_curve_x = p_scale_curve; }

void CPUParticles3D::set_scale_curve_y(Ref<Curve> p_scale_curve) { scale_curve_y = p_scale_curve; }

void CPUParticles3D::set_scale_curve_z(Ref<Curve> p_scale_curve) { scale_curve_z = p_scale_curve; }

real_t CPUParticles3D::get_emission_sphere_radius() const { return emission_sphere_radius; }

Vector3 CPUParticles3D::get_emission_box_extents() const { return emission_box_extents; }

Vector<Vector3> CPUParticles3D::get_emission_points() const { return emission_points; }

Vector<Vector3> CPUParticles3D::get_emission_normals() const { return emission_normals; }

Vector<Color> CPUParticles3D::get_emission_colors() const { return emission_colors; }

Vector3 CPUParticles3D::get_emission_ring_axis() const { return emission_ring_axis; }

real_t CPUParticles3D::get_emission_ring_height() const { return emission_ring_height; }

real_t CPUParticles3D::get_emission_ring_radius() const { return emission_ring_radius; }

real_t CPUParticles3D::get_emission_ring_inner_radius() const { return emission_ring_inner_radius; }

real_t CPUParticles3D::get_emission_ring_cone_angle() const { return emission_ring_cone_angle; }

CPUParticles3D::EmissionShape CPUParticles3D::get_emission_shape() const { return emission_shape; }

void CPUParticles3D::set_gravity(const Vector3& p_gravity) { gravity = p_gravity; }

Vector3 CPUParticles3D::get_gravity() const { return gravity; }

Ref<Curve> CPUParticles3D::get_scale_curve_x() const { return scale_curve_x; }

Ref<Curve> CPUParticles3D::get_scale_curve_y() const { return scale_curve_y; }

Ref<Curve> CPUParticles3D::get_scale_curve_z() const { return scale_curve_z; }

bool CPUParticles3D::get_split_scale() { return split_scale; }

AABB CPUParticles3D::capture_aabb() const
{
	Renderer::multimesh_set_custom_aabb(multimesh, AABB());
	return Renderer::multimesh_get_aabb(multimesh);
}

bool CPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void CPUParticles3D::set_seed(uint32_t p_seed) { seed = p_seed; }

uint32_t CPUParticles3D::get_seed() const { return seed; }

void CPUParticles3D::request_particles_process(
	real_t p_request_process_time, real_t p_request_process_time_residual)
{
	_request_process_time = p_request_process_time;
	_request_process_time_residual = p_request_process_time_residual;
	_update_internal();
}

static uint32_t idhash(uint32_t x)
{
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = ((x >> uint32_t(16)) ^ x) * uint32_t(0x45d9f3b);
	x = (x >> uint32_t(16)) ^ x;
	return x;
}

static real_t rand_from_seed(uint32_t& seed)
{
	int k;
	int s = int(seed);
	if (s == 0) {
		s = 305420679;
	}
	k = s / 127773;
	s = 16807 * (s - k * 127773) - 2836 * k;
	if (s < 0) {
		s += 2147483647;
	}
	seed = uint32_t(s);
	return (seed % uint32_t(65536)) / 65535.0;
}

void CPUParticles3D::_update_internal()
{
	if (particles.is_empty() || !is_visible_in_tree()) {
		_set_redraw(false);
		return;
	}

	double delta = get_process_delta_time();
	if (!active && !emitting) {
		set_process_internal(false);
		_set_redraw(false);

		// reset variables
		time = 0;
		frame_remainder = 0;
		cycle = 0;
		return;
	}
	_set_redraw(true);

	bool processed = false;

	{
		float todo = time == 0 ? pre_process_time : 0;
		todo = todo > _request_process_time ? todo : _request_process_time;
		todo = todo > _request_process_time_residual ? todo : _request_process_time_residual;

		if (todo > 0.0) {
			real_t frame_time;
			if (fixed_fps > 0) {
				frame_time = 1.0 / fixed_fps;
			}
			else {
				frame_time = 1.0 / 30.0;
			}

			float tmp_scale = speed_scale;
			// We need this otherwise the speed scale of the particle system influences the TODO.
			speed_scale = 1.0;
			if (time == 0) {
				todo = pre_process_time;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time > 0.0) {
				todo = _request_process_time;
				emitting = true;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			if (_request_process_time_residual > 0.0) {
				emitting = false;
				todo = _request_process_time_residual;
				while (todo > 0.0) {
					_particles_process(frame_time > todo ? todo : frame_time);
					todo -= frame_time;
				}
			}
			speed_scale = tmp_scale;
			processed = true;
		}
		_request_process_time = 0;
		_request_process_time_residual = 0;
	}

	double frame_time;
	if (fixed_fps > 0) {
		frame_time = 1.0 / fixed_fps;
	}
	else {
		frame_time = 1.0 / 30.0;
	}

	if (fixed_fps > 0) {
		double decr = frame_time;

		double ldelta = delta;
		if (ldelta > 0.1) { // avoid recursive stalls if fps goes below 10
			ldelta = 0.1;
		}
		else if (ldelta < 0.0) {
			ldelta = 0.0;
		}
		double todo = frame_remainder + ldelta;

		while (todo >= frame_time) {
			_particles_process(frame_time);
			processed = true;
			todo -= decr;
		}

		frame_remainder = todo;

	}
	else {
		_particles_process(delta);
		processed = true;
	}

	if (processed) {
		_update_particle_data_buffer();
	}
}

void CPUParticles3D::_update_particle_data_buffer()
{
	MutexLock lock(update_mutex);

	int pc = particles.size();

	int* ow;
	int* order = nullptr;

	float* w = particle_data.ptrw();
	const Particle* r = particles.ptr();
	float* ptr = w;

	if (draw_order != DRAW_ORDER_INDEX) {
		ow = particle_order.ptrw();
		order = ow;

		for (int i = 0; i < pc; i++) {
			order[i] = i;
		}
		if (draw_order == DRAW_ORDER_LIFETIME) {
			SortArray<int, SortLifetime> sorter;
			sorter.compare.particles = r;
			sorter.sort(order, pc);
		}
		else if (draw_order == DRAW_ORDER_VIEW_DEPTH) {
			ERR_FAIL_NULL(get_viewport());
			Camera3D* c = get_viewport()->get_camera_3d();
			if (c) {
				Vector3 dir = c->get_global_transform().basis.get_column(2); // far away to close

				if (local_coords) {
					// will look different from Particles in editor as this is based on the camera
					// in the scenetree and not the editor camera
					dir = inv_emission_transform.xform(dir).normalized();
				}
				else {
					dir = dir.normalized();
				}

				SortArray<int, SortAxis> sorter;
				sorter.compare.particles = r;
				sorter.compare.axis = dir;
				sorter.sort(order, pc);
			}
		}
	}

	for (int i = 0; i < pc; i++) {
		int idx = order ? order[i] : i;

		Transform3D t = r[idx].transform;

		if (!local_coords) {
			t = inv_emission_transform * t;
		}

		if (r[idx].active) {
			ptr[0] = t.basis.rows[0][0];
			ptr[1] = t.basis.rows[0][1];
			ptr[2] = t.basis.rows[0][2];
			ptr[3] = t.origin.x;
			ptr[4] = t.basis.rows[1][0];
			ptr[5] = t.basis.rows[1][1];
			ptr[6] = t.basis.rows[1][2];
			ptr[7] = t.origin.y;
			ptr[8] = t.basis.rows[2][0];
			ptr[9] = t.basis.rows[2][1];
			ptr[10] = t.basis.rows[2][2];
			ptr[11] = t.origin.z;
		}
		else {
			memset(ptr, 0, sizeof(float) * 12);
		}

		Color c = r[idx].color;

		ptr[12] = c.r;
		ptr[13] = c.g;
		ptr[14] = c.b;
		ptr[15] = c.a;

		ptr[16] = r[idx].custom[0];
		ptr[17] = r[idx].custom[1];
		ptr[18] = r[idx].custom[2];
		ptr[19] = r[idx].custom[3];

		ptr += 20;
	}

	can_update.set();
}

void CPUParticles3D::_update_render_thread()
{
	MutexLock lock(update_mutex);

	if (can_update.is_set()) {
		Renderer::multimesh_set_buffer(multimesh, particle_data);
		can_update.clear(); // wait for next time
	}
}

void CPUParticles3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void CPUParticles3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		set_process_internal(emitting);

		// first update before rendering to avoid one frame delay after emitting starts
		if (emitting && (time == 0)) {
			_update_internal();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_set_redraw(false);
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		// first update before rendering to avoid one frame delay after emitting starts
		if (emitting && (time == 0)) {
			_update_internal();
		}
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_internal();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Transform3D global_transform = get_global_transform();
		if (unlikely(global_transform.basis.determinant() == 0)) {
			return;
		}
		inv_emission_transform = global_transform.affine_inverse();

		if (!local_coords) {
			int pc = particles.size();

			float* w = particle_data.ptrw();
			const Particle* r = particles.ptr();
			float* ptr = w;

			for (int i = 0; i < pc; i++) {
				Transform3D t = inv_emission_transform * r[i].transform;

				if (r[i].active) {
					ptr[0] = t.basis.rows[0][0];
					ptr[1] = t.basis.rows[0][1];
					ptr[2] = t.basis.rows[0][2];
					ptr[3] = t.origin.x;
					ptr[4] = t.basis.rows[1][0];
					ptr[5] = t.basis.rows[1][1];
					ptr[6] = t.basis.rows[1][2];
					ptr[7] = t.origin.y;
					ptr[8] = t.basis.rows[2][0];
					ptr[9] = t.basis.rows[2][1];
					ptr[10] = t.basis.rows[2][2];
					ptr[11] = t.origin.z;
				}
				else {
					memset(ptr, 0, sizeof(float) * 12);
				}

				ptr += 20;
			}

			can_update.set();
		}
	} break;
	}
}

CPUParticles3D::~CPUParticles3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(multimesh);
}

void CPUParticles3D::_set_redraw(bool p_redraw) {}

void CPUParticles3D::_particles_process(double p_delta) {}

PackedStringArray CPUParticles3D::get_configuration_warnings() const { return PackedStringArray(); }

// ==========================================================================
// Merged from scene/3d/gpu_particles_3d.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/os/os.h"


#include "scene/resources/material.h"
#include "scene/resources/gradient.h"
#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

AABB GPUParticles3D::get_aabb() const { return AABB(); }

void GPUParticles3D::set_emitting(bool p_emitting)
{
	// Do not return even if `p_emitting == emitting` because `emitting` is just an approximation.
	if (p_emitting && p_emitting != emitting && !use_fixed_seed && one_shot) {
		set_seed(Math::rand());
	}
	if (p_emitting && one_shot) {
		if (!active && !emitting) {
			// Last cycle ended.
			active = true;
			time = 0;
			signal_canceled = false;
			emission_time = lifetime;
			active_time = lifetime * (2 - explosiveness_ratio);
		}
		else {
			signal_canceled = true;
		}
		set_process_internal(true);
	}
	else if (!p_emitting) {
		if (one_shot) {
			set_process_internal(true);
		}
		else {
			set_process_internal(false);
		}
	}
	else {
		set_process_internal(true);
	}

	emitting = p_emitting;
	Renderer::particles_set_emitting(particles, p_emitting);
}

void GPUParticles3D::set_amount(int p_amount)
{
	ERR_FAIL_COND_MSG(p_amount < 1, "Amount of particles cannot be smaller than 1.");
	amount = p_amount;
	Renderer::particles_set_amount(particles, amount);
}

void GPUParticles3D::set_lifetime(double p_lifetime)
{
	ERR_FAIL_COND_MSG(p_lifetime <= 0, "Particles lifetime must be greater than 0.");
	lifetime = p_lifetime;
	Renderer::particles_set_lifetime(particles, lifetime);
}

void GPUParticles3D::set_interp_to_end(float p_interp)
{
	interp_to_end_factor = CLAMP(p_interp, 0.0, 1.0);
	Renderer::particles_set_interp_to_end(particles, interp_to_end_factor);
}

void GPUParticles3D::set_one_shot(bool p_one_shot)
{
	one_shot = p_one_shot;
	Renderer::particles_set_one_shot(particles, one_shot);

	if (is_emitting()) {
		if (!one_shot) {
			Renderer::particles_restart(particles);
		}
	}
}

bool GPUParticles3D::get_use_fixed_seed() const { return use_fixed_seed; }

void GPUParticles3D::set_seed(uint32_t p_seed)
{
	seed = p_seed;
	Renderer::particles_set_seed(particles, p_seed);
}

uint32_t GPUParticles3D::get_seed() const { return seed; }

void GPUParticles3D::set_pre_process_time(double p_time)
{
	pre_process_time = p_time;
	Renderer::particles_set_pre_process_time(particles, pre_process_time);
}

void GPUParticles3D::set_explosiveness_ratio(real_t p_ratio)
{
	explosiveness_ratio = p_ratio;
	Renderer::particles_set_explosiveness_ratio(particles, explosiveness_ratio);
}

void GPUParticles3D::set_randomness_ratio(real_t p_ratio)
{
	randomness_ratio = p_ratio;
	Renderer::particles_set_randomness_ratio(particles, randomness_ratio);
}

void GPUParticles3D::set_use_local_coordinates(bool p_enable)
{
	local_coords = p_enable;
	Renderer::particles_set_use_local_coordinates(particles, local_coords);
}

void GPUParticles3D::set_speed_scale(double p_scale)
{
	speed_scale = p_scale;
	Renderer::particles_set_speed_scale(particles, p_scale);
}

void GPUParticles3D::set_collision_base_size(real_t p_size)
{
	collision_base_size = p_size;
	Renderer::particles_set_collision_base_size(particles, p_size);
}

bool GPUParticles3D::is_emitting() const { return emitting; }

int GPUParticles3D::get_amount() const { return amount; }

double GPUParticles3D::get_lifetime() const { return lifetime; }

float GPUParticles3D::get_interp_to_end() const { return interp_to_end_factor; }

bool GPUParticles3D::get_one_shot() const { return one_shot; }

double GPUParticles3D::get_pre_process_time() const { return pre_process_time; }

real_t GPUParticles3D::get_explosiveness_ratio() const { return explosiveness_ratio; }

real_t GPUParticles3D::get_randomness_ratio() const { return randomness_ratio; }

AABB GPUParticles3D::get_visibility_aabb() const { return visibility_aabb; }

bool GPUParticles3D::get_use_local_coordinates() const { return local_coords; }

Ref<Material> GPUParticles3D::get_process_material() const { return process_material; }

double GPUParticles3D::get_speed_scale() const { return speed_scale; }

real_t GPUParticles3D::get_collision_base_size() const { return collision_base_size; }

void GPUParticles3D::set_draw_order(DrawOrder p_order)
{
	draw_order = p_order;
	Renderer::particles_set_draw_order(particles, RSE::ParticlesDrawOrder(p_order));
}

void GPUParticles3D::set_trail_lifetime(double p_seconds)
{
	ERR_FAIL_COND(p_seconds < 0.01 - CMP_EPSILON);
	trail_lifetime = p_seconds;
	Renderer::particles_set_trails(particles, trail_enabled, trail_lifetime);
}

bool GPUParticles3D::is_trail_enabled() const { return trail_enabled; }

double GPUParticles3D::get_trail_lifetime() const { return trail_lifetime; }

GPUParticles3D::DrawOrder GPUParticles3D::get_draw_order() const { return draw_order; }

int GPUParticles3D::get_draw_passes() const { return draw_passes.size(); }

Ref<Mesh> GPUParticles3D::get_draw_pass_mesh(int p_pass) const
{
	ERR_FAIL_INDEX_V(p_pass, draw_passes.size(), Ref<Mesh>());

	return draw_passes[p_pass];
}

void GPUParticles3D::set_fixed_fps(int p_count)
{
	fixed_fps = p_count;
	Renderer::particles_set_fixed_fps(particles, p_count);
}

int GPUParticles3D::get_fixed_fps() const { return fixed_fps; }

void GPUParticles3D::set_fractional_delta(bool p_enable)
{
	fractional_delta = p_enable;
	Renderer::particles_set_fractional_delta(particles, p_enable);
}

bool GPUParticles3D::get_fractional_delta() const { return fractional_delta; }

void GPUParticles3D::set_interpolate(bool p_enable)
{
	interpolate = p_enable;
	Renderer::particles_set_interpolate(particles, p_enable);
}

bool GPUParticles3D::get_interpolate() const { return interpolate; }

void GPUParticles3D::restart(bool p_keep_seed)
{
	if (!p_keep_seed && !use_fixed_seed) {
		set_seed(Math::rand());
	}
	Renderer::particles_restart(particles);
	Renderer::particles_set_emitting(particles, true);

	emitting = true;
	active = true;
	signal_canceled = false;
	time = 0;
	emission_time = lifetime * (1 - explosiveness_ratio);
	active_time = lifetime * (2 - explosiveness_ratio);
	set_process_internal(true);
}

AABB GPUParticles3D::capture_aabb() const
{
	return Renderer::particles_get_current_aabb(particles);
}

void GPUParticles3D::request_particles_process(
	real_t p_requested_process_time, real_t p_request_process_time_residual)
{
	Renderer::particles_request_process_time(
		particles, p_requested_process_time, p_request_process_time_residual);
	// Setting emitting independently from set_emitting is important here
	// we assume to be in a controlled process situation.
	// setting RS emission to true is necessary to ensure particles will emit when
	// regular process time is > than zero but the particles are already in trailing mode.
	// this could also be done on the GScript/tool side.
	if (p_requested_process_time > 0.0) {
		emitting = true;
		Renderer::particles_set_emitting(particles, true);
	}
	if (p_request_process_time_residual > 0.0) {
		emitting = false;
	}
}

void GPUParticles3D::emit_particle(const Transform3D& p_transform, const Vector3& p_velocity,
	const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Renderer::particles_emit(
		particles, p_transform, p_velocity, p_color, p_custom, p_emit_flags);
}

NodePath GPUParticles3D::get_sub_emitter() const { return sub_emitter; }

Ref<Skin> GPUParticles3D::get_skin() const { return skin; }

GPUParticles3D::TransformAlign GPUParticles3D::get_transform_align() const
{
	return transform_align;
}

void GPUParticles3D::set_transform_align_channel_filter(
	RSE::ParticlesTransformAlignCustomSrc p_align_channel_filter)
{
	ERR_FAIL_INDEX(uint32_t(p_align_channel_filter),
		uint32_t(RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_MAX));
	transform_align_channel_filter = p_align_channel_filter;
	Renderer::particles_set_transform_align_channel_filter(
		particles, transform_align_channel_filter);
}

RSE::ParticlesTransformAlignCustomSrc GPUParticles3D::get_transform_align_channel_filter() const
{
	return transform_align_channel_filter;
}

void GPUParticles3D::set_transform_align_axis(RSE::ParticlesTransformAlignAxis p_axis)
{
	transform_align_axis = p_axis;
	Renderer::particles_set_transform_align_axis(particles, p_axis);
}

RSE::ParticlesTransformAlignAxis GPUParticles3D::get_transform_align_axis() const
{
	return transform_align_axis;
}

void GPUParticles3D::set_amount_ratio(float p_ratio)
{
	amount_ratio = p_ratio;
	Renderer::particles_set_amount_ratio(particles, p_ratio);
}

float GPUParticles3D::get_amount_ratio() const { return amount_ratio; }

GPUParticles3D::GPUParticles3D()
{
	particles = Renderer::particles_create();
	Renderer::particles_set_mode(particles, RSE::PARTICLES_MODE_3D);
	set_base(particles);
	one_shot = false; // Needed so that set_emitting doesn't access uninitialized values
	set_emitting(true);
	set_one_shot(false);
	set_seed(Math::rand());
	set_amount_ratio(1.0);
	set_amount(8);
	set_lifetime(1);
	set_fixed_fps(30);
	set_fractional_delta(true);
	set_interpolate(true);
	set_pre_process_time(0);
	set_explosiveness_ratio(0);
	set_randomness_ratio(0);
	set_trail_lifetime(0.3);
	set_use_local_coordinates(false);
	set_draw_passes(1);
	set_draw_order(DRAW_ORDER_INDEX);
	set_speed_scale(1);
	set_collision_base_size(collision_base_size);
	set_transform_align(TRANSFORM_ALIGN_DISABLED);
	set_transform_align_channel_filter(
		RSE::ParticlesTransformAlignCustomSrc::PARTICLES_ALIGN_CHANNEL_FILTER_X);
	set_transform_align_axis(RSE::ParticlesTransformAlignAxis::PARTICLES_ALIGN_AXIS_Y);
	set_use_fixed_seed(false);
}

void GPUParticles3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void GPUParticles3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

GPUParticles3D::~GPUParticles3D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(particles);
}

PackedStringArray GPUParticles3D::get_configuration_warnings() const { return PackedStringArray(); }

void GPUParticles3D::set_use_fixed_seed(bool) {}

void GPUParticles3D::set_transform_align(GPUParticles3D::TransformAlign) {}

void GPUParticles3D::set_draw_passes(int) {}

// ==========================================================================
// Merged from scene/3d/gpu_particles_collision_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/gpu_particles_collision_3d.h ---
#include "core/templates/local_vector.h"
#include "scene/3d/visual_instance_3d.h"

class Mesh;

class GPUParticlesCollision3D : public Node3D
{
public:
	enum ShapeType
	{
		SHAPE_SPHERE,
		SHAPE_BOX,
		SHAPE_SDF,
		SHAPE_HEIGHT_FIELD,
	};

	enum SDFResolution
	{
		SDF_RESOLUTION_16,
		SDF_RESOLUTION_32,
		SDF_RESOLUTION_64,
		SDF_RESOLUTION_128,
		SDF_RESOLUTION_256,
		SDF_RESOLUTION_512,
		SDF_RESOLUTION_MAX,
	};

	enum HeightFieldResolution
	{
		HEIGHTFIELD_RESOLUTION_256,
		HEIGHTFIELD_RESOLUTION_512,
		HEIGHTFIELD_RESOLUTION_1024,
		HEIGHTFIELD_RESOLUTION_2048,
		HEIGHTFIELD_RESOLUTION_4096,
		HEIGHTFIELD_RESOLUTION_8192,
		HEIGHTFIELD_RESOLUTION_MAX,
	};

	enum UpdateMode
	{
		UPDATE_MODE_WHEN_MOVED,
		UPDATE_MODE_ALWAYS,
	};

	typedef void (*BakeBeginFunc)(int);
	typedef void (*BakeStepFunc)(int, const String&);
	typedef void (*BakeEndFunc)();

	RenderInstance3D render_instance;

private:
	ShapeType shape_type = SHAPE_BOX;
	uint32_t cull_mask = 0xFFFFFFFF;
	RID collision;

	real_t radius = 1.0;
	Vector3 size = Vector3(2, 2, 2);

	// SDF parameters
	SDFResolution sdf_resolution = SDF_RESOLUTION_64;
	uint32_t bake_mask = 0xFFFFFFFF;
	Ref<Texture3D> texture;
	float thickness = 1.0f;

	// HeightField parameters
	uint32_t heightfield_mask = (1 << 20) - 1;
	HeightFieldResolution heightfield_resolution = HEIGHTFIELD_RESOLUTION_1024;
	bool follow_camera_mode = false;
	UpdateMode update_mode = UPDATE_MODE_WHEN_MOVED;

	void _apply_shape_type();

protected:
	_FORCE_INLINE_ RID _get_collision() const { return collision; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_shape_type(ShapeType p_type);
	ShapeType get_shape_type() const { return shape_type; }

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_thickness(float p_thickness);
	float get_thickness() const { return thickness; }

	SDFResolution get_sdf_resolution() const { return sdf_resolution; }
	uint32_t get_bake_mask() const { return bake_mask; }
	bool get_bake_mask_value(int p_layer_number) const;

	void set_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_texture() const { return texture; }

	Vector3i get_estimated_cell_size() const;

	HeightFieldResolution get_heightfield_resolution() const { return heightfield_resolution; }

	void set_update_mode(UpdateMode p_update_mode);
	UpdateMode get_update_mode() const { return update_mode; }

	void set_heightfield_mask(uint32_t p_heightfield_mask);
	uint32_t get_heightfield_mask() const { return heightfield_mask; }

	void set_heightfield_mask_value(int p_layer_number, bool p_value);
	bool get_heightfield_mask_value(int p_layer_number) const;

	void set_follow_camera_enabled(bool p_enabled);
	bool is_follow_camera_enabled() const { return follow_camera_mode; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	GPUParticlesCollision3D(ShapeType p_type = SHAPE_BOX);
	~GPUParticlesCollision3D();
};

class GPUParticlesAttractor3D : public Node3D
{
public:
	enum AttractorType
	{
		ATTRACTOR_SPHERE,
		ATTRACTOR_BOX,
		ATTRACTOR_VECTOR_FIELD,
	};

	RenderInstance3D render_instance;

private:
	AttractorType attractor_type = ATTRACTOR_SPHERE;
	uint32_t cull_mask = 0xFFFFFFFF;
	RID collision;
	real_t strength = 1.0;
	real_t attenuation = 1.0;
	real_t directionality = 0.0;

	real_t radius = 1.0;
	Vector3 size = Vector3(2, 2, 2);
	Ref<Texture3D> texture;

	void _apply_attractor_type();

protected:
	_FORCE_INLINE_ RID _get_collision() const { return collision; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_attractor_type(AttractorType p_type);
	AttractorType get_attractor_type() const { return attractor_type; }

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_strength(real_t p_strength);
	real_t get_strength() const;

	void set_attenuation(real_t p_attenuation);
	real_t get_attenuation() const;

	real_t get_directionality() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_texture() const { return texture; }

	AABB get_aabb() const;

	GPUParticlesAttractor3D(AttractorType p_type = ATTRACTOR_SPHERE);
	~GPUParticlesAttractor3D();
};

// --- End scene/3d/gpu_particles_collision_3d.h ---

#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"

GPUParticlesCollision3D::BakeBeginFunc GPUParticlesCollision3D::bake_begin_function = nullptr;
GPUParticlesCollision3D::BakeStepFunc GPUParticlesCollision3D::bake_step_function = nullptr;
GPUParticlesCollision3D::BakeEndFunc GPUParticlesCollision3D::bake_end_function = nullptr;

void GPUParticlesCollision3D::_apply_shape_type()
{
	if (!collision.is_valid()) {
		return;
	}
	RSE::ParticlesCollisionType rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE;
	switch (shape_type) {
	case SHAPE_SPHERE:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE;
		break;
	case SHAPE_BOX:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE;
		break;
	case SHAPE_SDF:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SDF_COLLIDE;
		break;
	case SHAPE_HEIGHT_FIELD:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE;
		break;
	}
	Renderer::particles_collision_set_collision_type(collision, rs_type);
}

void GPUParticlesCollision3D::set_shape_type(ShapeType p_type)
{
	shape_type = p_type;
	_apply_shape_type();
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

void GPUParticlesCollision3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	Renderer::particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesCollision3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesCollision3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	Renderer::particles_collision_set_sphere_radius(collision, radius);
}

void GPUParticlesCollision3D::set_size(const Vector3& p_size)
{
	size = p_size;
	Renderer::particles_collision_set_box_extents(collision, size / 2);
}

void GPUParticlesCollision3D::set_thickness(float p_thickness) { thickness = p_thickness; }

bool GPUParticlesCollision3D::get_bake_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(p_layer_number < 1 || p_layer_number > 20, false,
		vformat(
			"The render layer number (%d) must be between 1 and 20 (inclusive).", p_layer_number));
	return bake_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollision3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	Renderer::particles_collision_set_field_texture(collision, tex);
}

Vector3i GPUParticlesCollision3D::get_estimated_cell_size() const
{
	static const int subdivs[SDF_RESOLUTION_MAX] = {16, 32, 64, 128, 256, 512};
	int subdiv = subdivs[sdf_resolution];
	AABB aabb(-size / 2, size);
	float cell_size = aabb.get_longest_axis_size() / float(subdiv);
	Vector3i sdf_size = Vector3i(aabb.size / cell_size);
	return sdf_size.maxi(1);
}

void GPUParticlesCollision3D::set_update_mode(UpdateMode p_update_mode)
{
	update_mode = p_update_mode;
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

void GPUParticlesCollision3D::set_heightfield_mask(uint32_t p_heightfield_mask)
{
	heightfield_mask = p_heightfield_mask;
	Renderer::particles_collision_set_height_field_mask(collision, p_heightfield_mask);
}

void GPUParticlesCollision3D::set_heightfield_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_heightfield_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_heightfield_mask(mask);
}

bool GPUParticlesCollision3D::get_heightfield_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return heightfield_mask & (1 << (p_layer_number - 1));
}

void GPUParticlesCollision3D::set_follow_camera_enabled(bool p_enabled)
{
	follow_camera_mode = p_enabled;
	set_process_internal(shape_type == SHAPE_HEIGHT_FIELD &&
						 (follow_camera_mode || update_mode == UPDATE_MODE_ALWAYS));
}

AABB GPUParticlesCollision3D::get_aabb() const
{
	if (shape_type == SHAPE_SPHERE) {
		return AABB(
			Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
	}
	return AABB(-size / 2, size);
}

PackedStringArray GPUParticlesCollision3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (shape_type == SHAPE_SDF && bake_mask == 0) {
		warnings.push_back(RTR("The Bake Mask has no bits enabled, which means baking will not "
							   "produce any collision for this GPUParticlesCollision3D.\nTo "
							   "resolve this, enable at least one bit in the Bake Mask property."));
	}

	return warnings;
}

void GPUParticlesCollision3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void GPUParticlesCollision3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	if (shape_type != SHAPE_HEIGHT_FIELD) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (update_mode == UPDATE_MODE_ALWAYS) {
			Renderer::particles_collision_height_field_update(collision);
		}

		if (follow_camera_mode && get_viewport()) {
			Camera3D* cam = get_viewport()->get_camera_3d();
			if (cam) {
				Transform3D xform = get_global_transform();
				Vector3 x_axis = xform.basis.get_column(Vector3::AXIS_X).normalized();
				Vector3 z_axis = xform.basis.get_column(Vector3::AXIS_Z).normalized();
				float x_len = xform.basis.get_scale().x;
				float z_len = xform.basis.get_scale().z;

				Vector3 cam_pos = cam->get_global_transform().origin;
				Transform3D new_xform = xform;

				while (x_axis.dot(cam_pos - new_xform.origin) > x_len) {
					new_xform.origin += x_axis * x_len;
				}
				while (x_axis.dot(cam_pos - new_xform.origin) < -x_len) {
					new_xform.origin -= x_axis * x_len;
				}

				while (z_axis.dot(cam_pos - new_xform.origin) > z_len) {
					new_xform.origin += z_axis * z_len;
				}
				while (z_axis.dot(cam_pos - new_xform.origin) < -z_len) {
					new_xform.origin -= z_axis * z_len;
				}

				if (new_xform != xform) {
					set_global_transform(new_xform);
					Renderer::particles_collision_height_field_update(collision);
				}
			}
		}
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Renderer::particles_collision_height_field_update(collision);
	} break;
	}
}

GPUParticlesCollision3D::GPUParticlesCollision3D(ShapeType p_type) : shape_type(p_type)
{
	if (Renderer::data) {
		collision = Renderer::particles_collision_create();
		_apply_shape_type();
		set_base(collision);
	}
}

GPUParticlesCollision3D::~GPUParticlesCollision3D()
{
	if (collision.is_valid() && Renderer::data) {
		Renderer::free_rid(collision);
	}
}

void GPUParticlesAttractor3D::_apply_attractor_type()
{
	if (!collision.is_valid()) {
		return;
	}
	RSE::ParticlesCollisionType rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
	switch (attractor_type) {
	case ATTRACTOR_SPHERE:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
		break;
	case ATTRACTOR_BOX:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_BOX_ATTRACT;
		break;
	case ATTRACTOR_VECTOR_FIELD:
		rs_type = RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT;
		break;
	}
	Renderer::particles_collision_set_collision_type(collision, rs_type);
}

void GPUParticlesAttractor3D::set_attractor_type(AttractorType p_type)
{
	attractor_type = p_type;
	_apply_attractor_type();
}

void GPUParticlesAttractor3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	Renderer::particles_collision_set_cull_mask(collision, p_cull_mask);
}

uint32_t GPUParticlesAttractor3D::get_cull_mask() const { return cull_mask; }

void GPUParticlesAttractor3D::set_strength(real_t p_strength)
{
	strength = p_strength;
	Renderer::particles_collision_set_attractor_strength(collision, p_strength);
}

real_t GPUParticlesAttractor3D::get_strength() const { return strength; }

void GPUParticlesAttractor3D::set_attenuation(real_t p_attenuation)
{
	attenuation = p_attenuation;
	Renderer::particles_collision_set_attractor_attenuation(collision, p_attenuation);
}

real_t GPUParticlesAttractor3D::get_attenuation() const { return attenuation; }

real_t GPUParticlesAttractor3D::get_directionality() const { return directionality; }

void GPUParticlesAttractor3D::set_radius(real_t p_radius)
{
	radius = p_radius;
	Renderer::particles_collision_set_sphere_radius(collision, radius);
}

void GPUParticlesAttractor3D::set_size(const Vector3& p_size)
{
	size = p_size;
	Renderer::particles_collision_set_box_extents(collision, size / 2);
}

void GPUParticlesAttractor3D::set_texture(const Ref<Texture3D>& p_texture)
{
	texture = p_texture;
	RID tex = texture.is_valid() ? texture->get_rid() : RID();
	Renderer::particles_collision_set_field_texture(collision, tex);
}

AABB GPUParticlesAttractor3D::get_aabb() const
{
	if (attractor_type == ATTRACTOR_SPHERE) {
		return AABB(
			Vector3(-radius, -radius, -radius), Vector3(radius * 2, radius * 2, radius * 2));
	}
	return AABB(-size / 2, size);
}

void GPUParticlesAttractor3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void GPUParticlesAttractor3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

GPUParticlesAttractor3D::GPUParticlesAttractor3D(AttractorType p_type) : attractor_type(p_type)
{
	if (Renderer::data) {
		collision = Renderer::particles_collision_create();
		_apply_attractor_type();
		set_base(collision);
	}
}

GPUParticlesAttractor3D::~GPUParticlesAttractor3D()
{
	if (collision.is_valid() && Renderer::data) {
		Renderer::free_rid(collision);
	}
}

// ============================================================================
// Merged from scene/3d/light_3d.cpp
// ============================================================================

#include "scene/3d/visual_instance_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void Light3D::_create_light_rid()
{
	if (!Renderer::data) {
		return;
	}
	switch (type) {
	case RSE::LIGHT_DIRECTIONAL:
		light = Renderer::directional_light_create();
		break;
	case RSE::LIGHT_OMNI:
		light = Renderer::omni_light_create();
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(omni_shadow_mode));
		break;
	case RSE::LIGHT_SPOT:
		light = Renderer::spot_light_create();
		break;
	case RSE::LIGHT_AREA:
		light = Renderer::area_light_create();
		break;
	}
	set_base(light);
}

void Light3D::_free_light_rid()
{
	if (light.is_valid() && Renderer::data) {
		set_base(RID());
		Renderer::free_rid(light);
		light = RID();
	}
}

void Light3D::set_light_type(RSE::LightType p_type)
{
	if (type == p_type) {
		return;
	}
	_free_light_rid();
	type = p_type;
	_create_light_rid();
}

void Light3D::set_param(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	param[p_param] = p_value;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_param(light, RSE::LightParam(p_param), p_value);
	}
}

real_t Light3D::get_param(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return param[p_param];
}

void Light3D::set_shadow(bool p_enable)
{
	shadow = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow(light, p_enable);
	}
}

bool Light3D::has_shadow() const { return shadow; }

void Light3D::set_negative(bool p_enable)
{
	negative = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_negative(light, p_enable);
	}
}

bool Light3D::is_negative() const { return negative; }

void Light3D::set_enable_distance_fade(bool p_enable)
{
	distance_fade_enabled = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

bool Light3D::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Light3D::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_begin() const { return distance_fade_begin; }

void Light3D::set_distance_fade_shadow(real_t p_distance)
{
	distance_fade_shadow = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_shadow() const { return distance_fade_shadow; }

void Light3D::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_length() const { return distance_fade_length; }

void Light3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_cull_mask(light, p_cull_mask);
	}
}

uint32_t Light3D::get_cull_mask() const { return cull_mask; }

void Light3D::set_color(const Color& p_color)
{
	color = p_color;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_color(light, color * correlated_color);
	}
}

Color Light3D::get_color() const { return color; }

void Light3D::set_shadow_reverse_cull_face(bool p_enable)
{
	reverse_cull = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_reverse_cull_face_mode(light, reverse_cull);
	}
}

bool Light3D::get_shadow_reverse_cull_face() const { return reverse_cull; }

void Light3D::set_shadow_caster_mask(uint32_t p_caster_mask)
{
	shadow_caster_mask = p_caster_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow_caster_mask(light, shadow_caster_mask);
	}
}

uint32_t Light3D::get_shadow_caster_mask() const { return shadow_caster_mask; }

AABB Light3D::get_aabb() const { return AABB(); }

void Light3D::set_bake_mode(BakeMode p_mode)
{
	bake_mode = p_mode;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_bake_mode(light, RSE::LightBakeMode(p_mode));
	}
}

Light3D::BakeMode Light3D::get_bake_mode() const { return bake_mode; }

void Light3D::set_projector(const Ref<Texture2D>& p_texture)
{
	projector = p_texture;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_projector(light, projector.is_valid() ? projector->get_rid() : RID());
	}
}

Ref<Texture2D> Light3D::get_projector() const { return projector; }

void Light3D::owner_changed_notify()
{
	_update_visibility();
}

Color _color_from_temperature(float p_temperature)
{
	float T2 = p_temperature * p_temperature;
	float u = (0.860117757f + 1.54118254e-4f * p_temperature + 1.28641212e-7f * T2) /
			  (1.0f + 8.42420235e-4f * p_temperature + 7.08145163e-7f * T2);
	float v = (0.317398726f + 4.22806245e-5f * p_temperature + 4.20481691e-8f * T2) /
			  (1.0f - 2.89741816e-5f * p_temperature + 1.61456053e-7f * T2);

	float d = 1.0f / (2.0f * u - 8.0f * v + 4.0f);
	float x = 3.0f * u * d;
	float y = 2.0f * v * d;

	const float a = 1.0 / MAX(y, 1e-5f);
	Vector3 xyz = Vector3(x * a, 1.0, (1.0f - x - y) * a);

	Vector3 linear = Vector3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
		-0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
		0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
	linear /= MAX(1e-5f, linear[linear.max_axis_index()]);
	return Color(linear.x, linear.y, linear.z).clamp().linear_to_srgb();
}

Color Light3D::get_correlated_color() const { return correlated_color; }

float Light3D::get_temperature() const { return temperature; }

void Light3D::_update_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	bool editor_ok = true;

#ifdef TOOLS_ENABLED
	if (editor_only) {
		if (!Engine::is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (SceneTree::get_edited_scene_root() &&
						 (this == SceneTree::get_edited_scene_root() ||
							 get_owner() == SceneTree::get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif

	if (get_instance().is_valid() && Renderer::data) {
		Renderer::instance_set_visible(get_instance(), is_visible_in_tree() && editor_ok);
	}
}

void Light3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void Light3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_ENTER_TREE: {
		_update_visibility();
	} break;
	}
}

void Light3D::set_editor_only(bool p_editor_only)
{
	editor_only = p_editor_only;
	_update_visibility();
}

bool Light3D::is_editor_only() const { return editor_only; }

void Light3D::set_directional_shadow_mode(DirectionalShadowMode p_mode)
{
	directional_shadow_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_shadow_mode(
			light, RSE::LightDirectionalShadowMode(p_mode));
	}
}

void Light3D::set_blend_splits(bool p_enable)
{
	blend_splits = p_enable;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_blend_splits(light, p_enable);
	}
}

void Light3D::set_sky_mode(SkyMode p_mode)
{
	sky_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_sky_mode(light, RSE::LightDirectionalSkyMode(p_mode));
	}
}

void Light3D::set_omni_shadow_mode(OmniShadowMode p_mode)
{
	omni_shadow_mode = p_mode;
	if (type == RSE::LIGHT_OMNI && light.is_valid() && Renderer::data) {
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(p_mode));
	}
}

void Light3D::set_area_size(const Vector2& p_size)
{
	area_size = p_size;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_size(light, area_size);
	}
}

void Light3D::set_area_texture(const Ref<Texture2D>& p_texture)
{
	area_texture = p_texture;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_texture(
			light, area_texture.is_valid() ? area_texture->get_rid() : RID());
	}
}

void Light3D::set_area_normalize_energy(bool p_enabled)
{
	area_normalize_energy = p_enabled;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_normalize_energy(light, p_enabled);
	}
}

PackedStringArray Light3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_scale().is_equal_approx(Vector3(1, 1, 1))) {
		warnings.push_back(RTR("A light's scale does not affect the visual size of the light."));
	}

	switch (type) {
	case RSE::LIGHT_OMNI: {
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_SPOT: {
		if (has_shadow() && get_param(PARAM_SPOT_ANGLE) >= 90.0) {
			warnings.push_back(
				RTR("A SpotLight3D with an angle wider than 90 degrees cannot cast shadows."));
		}
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_AREA: {
		if (get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture is not supported for area lights. Use the "
								   "area_texture field instead."));
		}
		if (area_texture.is_valid() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR("Rendering textured area lights is not implemented in the "
								   "Compatibility rendering mode."));
		}
		if (has_shadow() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR(
				"Rendering area light shadows does not work in the Compatibility rendering mode."));
		}
	} break;

	default:
		break;
	}

	return warnings;
}

Light3D::Light3D(RSE::LightType p_type) : type(p_type)
{
	_create_light_rid();
}

Light3D::~Light3D()
{
	_free_light_rid();
}

// ==========================================================================
// Merged from scene/3d/voxelizer.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/math/geometry_3d.h"
#include "scene/resources/texture.h"
// --- voxelizer.h ---
#include "scene/3d/visual_instance_3d.h"

class Image;

class Voxelizer {
public:
	enum BakeResult {
		BAKE_RESULT_OK,
		BAKE_RESULT_INVALID_PARAMETER,
		BAKE_RESULT_CANCELLED,
	};

	typedef bool (*BakeStepFunc)(int, int);

private:
	enum : uint32_t {
		CHILD_EMPTY = 0xFFFFFFFF
	};

	struct Cell {
		uint32_t children[8];
		float albedo[3] = {}; //albedo in RGB24
		float emission[3] = {}; //accumulated light in 16:16 fixed point (needs to be integer for moving lights fast)
		float normal[3] = {};
		float alpha = 0.0; //used for upsampling
		uint16_t x = 0;
		uint16_t y = 0;
		uint16_t z = 0;
		uint16_t level = 0;

		Cell() {
			for (int i = 0; i < 8; i++) {
				children[i] = CHILD_EMPTY;
			}
		}
	};

	Vector<Cell> bake_cells;
	int cell_subdiv = 0;

	struct CellSort {
		union {
			uint64_t key = 0;
			struct {
				uint64_t z : 16;
				uint64_t y : 16;
				uint64_t x : 16;
				uint64_t level : 16;
			};
		};

		int32_t index = 0;

		_FORCE_INLINE_ bool operator<(const CellSort &p_cell_sort) const {
			return key < p_cell_sort.key;
		}
	};

	struct MaterialCache {
		//128x128 textures
		Vector<Color> albedo;
		Vector<Color> emission;
	};

	HashMap<Ref<Material>, MaterialCache> material_cache;
	float exposure_normalization = 1.0;
	AABB original_bounds;
	AABB po2_bounds;
	int axis_cell_size[3] = {};

	Transform3D to_cell_space;

	int color_scan_cell_width = 4;
	int bake_texture_size = 128;
	float cell_size = 0.0;

	int max_original_cells = 0;
	int leaf_voxel_count = 0;

	Vector<Color> _get_bake_texture(Ref<Image> p_image, const Color &p_color_mul, const Color &p_color_add);
	MaterialCache _get_material_cache(Ref<Material> p_material);

	void _plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3 *p_vtx, const Vector3 *p_normal, const Vector2 *p_uv, const MaterialCache &p_material, const AABB &p_aabb);
	void _fixup_plot(int p_idx, int p_level);
	void _debug_mesh(int p_idx, int p_level, const AABB &p_aabb, Ref<MultiMesh> &p_multimesh, int &idx);

	bool sorted = false;
	void _sort();

public:
	void begin_bake(int p_subdiv, const AABB &p_bounds, float p_exposure_normalization);
	int get_bake_steps(Ref<Mesh> &p_mesh) const;
	BakeResult plot_mesh(const Transform3D &p_xform, Ref<Mesh> &p_mesh, const Vector<Ref<Material>> &p_materials, const Ref<Material> &p_override_material, BakeStepFunc p_bake_step_function);
	void end_bake();

	int get_voxel_gi_octree_depth() const;
	Vector3i get_voxel_gi_octree_size() const;
	int get_voxel_gi_cell_count() const;
	Vector<uint8_t> get_voxel_gi_octree_cells() const;
	Vector<uint8_t> get_voxel_gi_data_cells() const;
	Vector<int> get_voxel_gi_level_cell_count() const;
	BakeResult get_sdf_3d_image(Vector<uint8_t> &r_image, BakeStepFunc p_bake_step_function) const;

	Ref<MultiMesh> create_debug_multimesh();

	Transform3D get_to_cell_space_xform() const;
	Voxelizer();
};

static _FORCE_INLINE_ void get_uv_and_normal(const Vector3& p_pos, const Vector3* p_vtx,
	const Vector2* p_uv, const Vector3* p_normal, Vector2& r_uv, Vector3& r_normal)
{
	if (p_pos.is_equal_approx(p_vtx[0])) {
		r_uv = p_uv[0];
		r_normal = p_normal[0];
		return;
	}
	if (p_pos.is_equal_approx(p_vtx[1])) {
		r_uv = p_uv[1];
		r_normal = p_normal[1];
		return;
	}
	if (p_pos.is_equal_approx(p_vtx[2])) {
		r_uv = p_uv[2];
		r_normal = p_normal[2];
		return;
	}

	Vector3 v0 = p_vtx[1] - p_vtx[0];
	Vector3 v1 = p_vtx[2] - p_vtx[0];
	Vector3 v2 = p_pos - p_vtx[0];

	real_t d00 = v0.dot(v0);
	real_t d01 = v0.dot(v1);
	real_t d11 = v1.dot(v1);
	real_t d20 = v2.dot(v0);
	real_t d21 = v2.dot(v1);
	real_t denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_uv = p_uv[0];
		r_normal = p_normal[0];
		return;
	}
	real_t v = (d11 * d20 - d01 * d21) / denom;
	real_t w = (d00 * d21 - d01 * d20) / denom;
	real_t u = 1.0f - v - w;

	r_uv = p_uv[0] * u + p_uv[1] * v + p_uv[2] * w;
	r_normal = (p_normal[0] * u + p_normal[1] * v + p_normal[2] * w).normalized();
}

void Voxelizer::_plot_face(int p_idx, int p_level, int p_x, int p_y, int p_z, const Vector3* p_vtx,
	const Vector3* p_normal, const Vector2* p_uv, const MaterialCache& p_material,
	const AABB& p_aabb)
{
	if (p_level == cell_subdiv) {
		// plot the face by guessing its albedo and emission value

		// find best axis to map to, for scanning values
		int closest_axis = 0;
		real_t closest_dot = 0;

		Plane plane = Plane(p_vtx[0], p_vtx[1], p_vtx[2]);
		Vector3 normal = plane.normal;

		for (int i = 0; i < 3; i++) {
			Vector3 axis;
			axis[i] = 1.0;
			real_t dot = Math::abs(normal.dot(axis));
			if (i == 0 || dot > closest_dot) {
				closest_axis = i;
				closest_dot = dot;
			}
		}

		Vector3 axis;
		axis[closest_axis] = 1.0;
		Vector3 t1;
		t1[(closest_axis + 1) % 3] = 1.0;
		Vector3 t2;
		t2[(closest_axis + 2) % 3] = 1.0;

		t1 *= p_aabb.size[(closest_axis + 1) % 3] / real_t(color_scan_cell_width);
		t2 *= p_aabb.size[(closest_axis + 2) % 3] / real_t(color_scan_cell_width);

		Color albedo_accum;
		Color emission_accum;
		Vector3 normal_accum;

		float alpha = 0.0;

		// map to a grid average in the best axis for this face
		for (int i = 0; i < color_scan_cell_width; i++) {
			Vector3 ofs_i = real_t(i) * t1;

			for (int j = 0; j < color_scan_cell_width; j++) {
				Vector3 ofs_j = real_t(j) * t2;

				Vector3 from = p_aabb.position + ofs_i + ofs_j;
				Vector3 to = from + t1 + t2 + axis * p_aabb.size[closest_axis];
				Vector3 half = (to - from) * 0.5;

				// is in this cell?
				if (!Geometry3D::triangle_box_overlap(from + half, half, p_vtx)) {
					continue; // face does not span this cell
				}

				// go from -size to +size*2 to avoid skipping collisions
				Vector3 ray_from = from + (t1 + t2) * 0.5 - axis * p_aabb.size[closest_axis];
				Vector3 ray_to = ray_from + axis * p_aabb.size[closest_axis] * 2;

				if (normal.dot(ray_from - ray_to) < 0) {
					SWAP(ray_from, ray_to);
				}

				Vector3 intersection;

				if (!plane.intersects_segment(ray_from, ray_to, &intersection)) {
					if (Math::abs(plane.distance_to(ray_from)) <
						Math::abs(plane.distance_to(ray_to))) {
						intersection = plane.project(ray_from);
					}
					else {
						intersection = plane.project(ray_to);
					}
				}

				intersection =
					Face3(p_vtx[0], p_vtx[1], p_vtx[2]).get_closest_point_to(intersection);

				Vector2 uv;
				Vector3 lnormal;
				get_uv_and_normal(intersection, p_vtx, p_uv, p_normal, uv, lnormal);
				if (lnormal == Vector3()) { // just in case normal is not provided
					lnormal = normal;
				}

				int uv_x = CLAMP(int(Math::fposmod(uv.x, (real_t)1.0) * bake_texture_size), 0,
					bake_texture_size - 1);
				int uv_y = CLAMP(int(Math::fposmod(uv.y, (real_t)1.0) * bake_texture_size), 0,
					bake_texture_size - 1);

				int ofs = uv_y * bake_texture_size + uv_x;
				albedo_accum.r += p_material.albedo[ofs].r;
				albedo_accum.g += p_material.albedo[ofs].g;
				albedo_accum.b += p_material.albedo[ofs].b;
				albedo_accum.a += p_material.albedo[ofs].a;

				emission_accum.r += p_material.emission[ofs].r;
				emission_accum.g += p_material.emission[ofs].g;
				emission_accum.b += p_material.emission[ofs].b;

				normal_accum += lnormal;

				alpha += 1.0;
			}
		}

		if (alpha == 0) {
			// could not in any way get texture information.. so use closest point to center

			Face3 f(p_vtx[0], p_vtx[1], p_vtx[2]);
			Vector3 inters = f.get_closest_point_to(p_aabb.get_center());

			Vector3 lnormal;
			Vector2 uv;
			get_uv_and_normal(inters, p_vtx, p_uv, p_normal, uv, normal);
			if (lnormal == Vector3()) { // just in case normal is not provided
				lnormal = normal;
			}

			int uv_x = CLAMP(
				Math::fposmod(uv.x, (real_t)1.0) * bake_texture_size, 0, bake_texture_size - 1);
			int uv_y = CLAMP(
				Math::fposmod(uv.y, (real_t)1.0) * bake_texture_size, 0, bake_texture_size - 1);

			int ofs = uv_y * bake_texture_size + uv_x;

			alpha = 1.0 / (color_scan_cell_width * color_scan_cell_width);

			albedo_accum.r = p_material.albedo[ofs].r * alpha;
			albedo_accum.g = p_material.albedo[ofs].g * alpha;
			albedo_accum.b = p_material.albedo[ofs].b * alpha;
			albedo_accum.a = p_material.albedo[ofs].a * alpha;

			emission_accum.r = p_material.emission[ofs].r * alpha;
			emission_accum.g = p_material.emission[ofs].g * alpha;
			emission_accum.b = p_material.emission[ofs].b * alpha;

			normal_accum = lnormal * alpha;

		}
		else {
			float accdiv = 1.0 / (color_scan_cell_width * color_scan_cell_width);
			alpha *= accdiv;

			albedo_accum.r *= accdiv;
			albedo_accum.g *= accdiv;
			albedo_accum.b *= accdiv;
			albedo_accum.a *= accdiv;

			emission_accum.r *= accdiv;
			emission_accum.g *= accdiv;
			emission_accum.b *= accdiv;

			normal_accum *= accdiv;
		}

		// put this temporarily here, corrected in a later step
		bake_cells.write[p_idx].albedo[0] += albedo_accum.r;
		bake_cells.write[p_idx].albedo[1] += albedo_accum.g;
		bake_cells.write[p_idx].albedo[2] += albedo_accum.b;
		bake_cells.write[p_idx].emission[0] += emission_accum.r;
		bake_cells.write[p_idx].emission[1] += emission_accum.g;
		bake_cells.write[p_idx].emission[2] += emission_accum.b;
		bake_cells.write[p_idx].normal[0] += normal_accum.x;
		bake_cells.write[p_idx].normal[1] += normal_accum.y;
		bake_cells.write[p_idx].normal[2] += normal_accum.z;
		bake_cells.write[p_idx].alpha += alpha;

	}
	else {
		// go down

		int half = (1 << cell_subdiv) >> (p_level + 1);
		for (int i = 0; i < 8; i++) {
			AABB aabb = p_aabb;
			aabb.size *= 0.5;

			int nx = p_x;
			int ny = p_y;
			int nz = p_z;

			if (i & 1) {
				aabb.position.x += aabb.size.x;
				nx += half;
			}
			if (i & 2) {
				aabb.position.y += aabb.size.y;
				ny += half;
			}
			if (i & 4) {
				aabb.position.z += aabb.size.z;
				nz += half;
			}
			// make sure to not plot beyond limits
			if (nx < 0 || nx >= axis_cell_size[0] || ny < 0 || ny >= axis_cell_size[1] || nz < 0 ||
				nz >= axis_cell_size[2]) {
				continue;
			}

			{
				AABB test_aabb = aabb;
				// test_aabb.grow_by(test_aabb.get_longest_axis_size()*0.05); //grow a bit to avoid
				// numerical error in real-time
				Vector3 qsize = test_aabb.size * 0.5; // quarter size, for fast aabb test

				if (!Geometry3D::triangle_box_overlap(test_aabb.position + qsize, qsize, p_vtx)) {
					// if (!Face3(p_vtx[0],p_vtx[1],p_vtx[2]).intersects_aabb2(aabb)) {
					// does not fit in child, go on
					continue;
				}
			}

			if (bake_cells[p_idx].children[i] == CHILD_EMPTY) {
				// sub cell must be created

				uint32_t child_idx = bake_cells.size();
				bake_cells.write[p_idx].children[i] = child_idx;
				bake_cells.resize(bake_cells.size() + 1);
				bake_cells.write[child_idx].level = p_level + 1;
				bake_cells.write[child_idx].x = nx / half;
				bake_cells.write[child_idx].y = ny / half;
				bake_cells.write[child_idx].z = nz / half;
			}

			_plot_face(bake_cells[p_idx].children[i], p_level + 1, nx, ny, nz, p_vtx, p_normal,
				p_uv, p_material, aabb);
		}
	}
}

Vector<Color> Voxelizer::_get_bake_texture(
	Ref<Image> p_image, const Color& p_color_mul, const Color& p_color_add)
{
	Vector<Color> ret;

	if (p_image.is_null() || p_image->is_empty()) {
		ret.resize(bake_texture_size * bake_texture_size);
		for (int i = 0; i < bake_texture_size * bake_texture_size; i++) {
			ret.write[i] = p_color_add;
		}

		return ret;
	}
	if (p_image->is_compressed()) {
		p_image->decompress();
	}
	p_image->convert(Image::FORMAT_RGBA8);
	p_image->resize(bake_texture_size, bake_texture_size, Image::INTERPOLATE_CUBIC);

	const uint8_t* r = p_image->get_data().ptr();
	ret.resize(bake_texture_size * bake_texture_size);

	for (int i = 0; i < bake_texture_size * bake_texture_size; i++) {
		Color c;
		Color src = Color(
			r[i * 4 + 0] / 255.0, r[i * 4 + 1] / 255.0, r[i * 4 + 2] / 255.0, r[i * 4 + 3] / 255.0);
		src = src.srgb_to_linear();

		c.r = src.r * p_color_mul.r + p_color_add.r;
		c.g = src.g * p_color_mul.g + p_color_add.g;
		c.b = src.b * p_color_mul.b + p_color_add.b;
		c.a = src.a;

		ret.write[i] = c;
	}

	return ret;
}

Voxelizer::MaterialCache Voxelizer::_get_material_cache(Ref<Material> p_material)
{
	// This way of obtaining materials is inaccurate and also does not support some compressed
	// formats very well.
	Ref<StandardMaterial3D> mat = p_material;

	Ref<Material> material = mat; // hack for now

	if (material_cache.has(material)) {
		return material_cache[material];
	}

	MaterialCache mc;

	if (mat.is_valid()) {
		Ref<Texture2D> albedo_tex = mat->get_texture(StandardMaterial3D::TEXTURE_ALBEDO);

		Ref<Image> img_albedo;
		if (albedo_tex.is_valid()) {
			img_albedo = albedo_tex->get_image();
			mc.albedo = _get_bake_texture(img_albedo, mat->get_albedo().srgb_to_linear(),
				Color(0, 0, 0)); // albedo texture, color is multiplicative
		}
		else {
			mc.albedo = _get_bake_texture(img_albedo, Color(1, 1, 1),
				mat->get_albedo().srgb_to_linear()); // no albedo texture, color is additive
		}
		if (mat->get_feature(StandardMaterial3D::FEATURE_EMISSION)) {
			Ref<Texture2D> emission_tex = mat->get_texture(StandardMaterial3D::TEXTURE_EMISSION);

			Color emission_col = mat->get_emission().srgb_to_linear();
			float emission_energy = mat->get_emission_energy_multiplier() * exposure_normalization;
			if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
				emission_energy *= mat->get_emission_intensity();
			}

			Ref<Image> img_emission;

			if (emission_tex.is_valid()) {
				img_emission = emission_tex->get_image();
			}

			if (mat->get_emission_operator() == StandardMaterial3D::EMISSION_OP_ADD) {
				mc.emission = _get_bake_texture(
					img_emission, Color(1, 1, 1) * emission_energy, emission_col * emission_energy);
			}
			else {
				mc.emission =
					_get_bake_texture(img_emission, emission_col * emission_energy, Color(0, 0, 0));
			}
		}
		else {
			Ref<Image> empty;
			mc.emission = _get_bake_texture(empty, Color(0, 0, 0), Color(0, 0, 0));
		}

	}
	else {
		Ref<Image> empty;

		mc.albedo = _get_bake_texture(empty, Color(0, 0, 0), Color(1, 1, 1));
		mc.emission = _get_bake_texture(empty, Color(0, 0, 0), Color(0, 0, 0));
	}

	material_cache[p_material] = mc;
	return mc;
}

void Voxelizer::_sort()
{
	// cells need to be sorted by level and coordinates
	// it is important that level has more priority (for compute), and that Z has the least,
	// given it may aid older implementations plot using GPU

	Vector<CellSort> sorted_cells;
	uint32_t cell_count = bake_cells.size();
	sorted_cells.resize(cell_count);
	{
		CellSort* sort_cellsp = sorted_cells.ptrw();
		const Cell* bake_cellsp = bake_cells.ptr();

		for (uint32_t i = 0; i < cell_count; i++) {
			sort_cellsp[i].x = bake_cellsp[i].x;
			sort_cellsp[i].y = bake_cellsp[i].y;
			sort_cellsp[i].z = bake_cellsp[i].z;
			sort_cellsp[i].level = bake_cellsp[i].level;
			sort_cellsp[i].index = i;
		}
	}

	sorted_cells.sort();

	// verify just in case, index 0 must be level 0
	ERR_FAIL_COND(sorted_cells[0].level != 0);

	Vector<Cell> new_bake_cells;
	new_bake_cells.resize(cell_count);
	Vector<uint32_t> reverse_map;

	{
		reverse_map.resize(cell_count);
		const CellSort* sort_cellsp = sorted_cells.ptr();
		uint32_t* reverse_mapp = reverse_map.ptrw();

		for (uint32_t i = 0; i < cell_count; i++) {
			reverse_mapp[sort_cellsp[i].index] = i;
		}
	}

	{
		const CellSort* sort_cellsp = sorted_cells.ptr();
		const Cell* bake_cellsp = bake_cells.ptr();
		const uint32_t* reverse_mapp = reverse_map.ptr();
		Cell* new_bake_cellsp = new_bake_cells.ptrw();

		for (uint32_t i = 0; i < cell_count; i++) {
			// copy to new cell
			new_bake_cellsp[i] = bake_cellsp[sort_cellsp[i].index];
			// remap children
			for (uint32_t j = 0; j < 8; j++) {
				if (new_bake_cellsp[i].children[j] != CHILD_EMPTY) {
					new_bake_cellsp[i].children[j] = reverse_mapp[new_bake_cellsp[i].children[j]];
				}
			}
		}
	}

	bake_cells = new_bake_cells;
	sorted = true;
}

void Voxelizer::_fixup_plot(int p_idx, int p_level)
{
	if (p_level == cell_subdiv) {
		leaf_voxel_count++;
		float alpha = bake_cells[p_idx].alpha;

		bake_cells.write[p_idx].albedo[0] /= alpha;
		bake_cells.write[p_idx].albedo[1] /= alpha;
		bake_cells.write[p_idx].albedo[2] /= alpha;

		// transfer emission to light
		bake_cells.write[p_idx].emission[0] /= alpha;
		bake_cells.write[p_idx].emission[1] /= alpha;
		bake_cells.write[p_idx].emission[2] /= alpha;

		bake_cells.write[p_idx].normal[0] /= alpha;
		bake_cells.write[p_idx].normal[1] /= alpha;
		bake_cells.write[p_idx].normal[2] /= alpha;

		Vector3 n(
			bake_cells[p_idx].normal[0], bake_cells[p_idx].normal[1], bake_cells[p_idx].normal[2]);
		if (n.length() < 0.01f) {
			// too much fight over normal, zero it
			bake_cells.write[p_idx].normal[0] = 0;
			bake_cells.write[p_idx].normal[1] = 0;
			bake_cells.write[p_idx].normal[2] = 0;
		}
		else {
			n.normalize();
			bake_cells.write[p_idx].normal[0] = n.x;
			bake_cells.write[p_idx].normal[1] = n.y;
			bake_cells.write[p_idx].normal[2] = n.z;
		}

		bake_cells.write[p_idx].alpha = 1.0;

		/*if (bake_light.size()) {
			for(int i=0;i<6;i++) {
			}
		}*/

	}
	else {
		// go down

		bake_cells.write[p_idx].emission[0] = 0;
		bake_cells.write[p_idx].emission[1] = 0;
		bake_cells.write[p_idx].emission[2] = 0;
		bake_cells.write[p_idx].normal[0] = 0;
		bake_cells.write[p_idx].normal[1] = 0;
		bake_cells.write[p_idx].normal[2] = 0;
		bake_cells.write[p_idx].albedo[0] = 0;
		bake_cells.write[p_idx].albedo[1] = 0;
		bake_cells.write[p_idx].albedo[2] = 0;

		float alpha_average = 0;

		for (int i = 0; i < 8; i++) {
			uint32_t child = bake_cells[p_idx].children[i];

			if (child == CHILD_EMPTY) {
				continue;
			}

			_fixup_plot(child, p_level + 1);
			alpha_average += bake_cells[child].alpha;
		}

		bake_cells.write[p_idx].alpha = alpha_average / 8.0;
	}
}

void Voxelizer::begin_bake(int p_subdiv, const AABB& p_bounds, float p_exposure_normalization)
{
	sorted = false;
	original_bounds = p_bounds;
	cell_subdiv = p_subdiv;
	exposure_normalization = p_exposure_normalization;
	bake_cells.resize(1);
	material_cache.clear();

	// find out the actual real bounds, power of 2, which gets the highest subdivision
	po2_bounds = p_bounds;
	int longest_axis = po2_bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;
	leaf_voxel_count = 0;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		real_t axis_size = po2_bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= po2_bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}

		po2_bounds.size[i] = po2_bounds.size[longest_axis];
	}

	Transform3D to_bounds;
	to_bounds.basis.scale(Vector3(po2_bounds.size[longest_axis], po2_bounds.size[longest_axis],
		po2_bounds.size[longest_axis]));
	to_bounds.origin = po2_bounds.position;

	Transform3D to_grid;
	to_grid.basis.scale(Vector3(
		axis_cell_size[longest_axis], axis_cell_size[longest_axis], axis_cell_size[longest_axis]));

	to_cell_space = to_grid * to_bounds.affine_inverse();

	cell_size = po2_bounds.size[longest_axis] / axis_cell_size[longest_axis];
}

void Voxelizer::end_bake()
{
	if (!sorted) {
		_sort();
	}
	_fixup_plot(0, 0);
}

// create the data for rendering server

int Voxelizer::get_voxel_gi_octree_depth() const { return cell_subdiv; }

Vector3i Voxelizer::get_voxel_gi_octree_size() const
{
	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

int Voxelizer::get_voxel_gi_cell_count() const { return bake_cells.size(); }

Vector<uint8_t> Voxelizer::get_voxel_gi_octree_cells() const
{
	Vector<uint8_t> data;
	data.resize((8 * 4) * bake_cells.size()); // 8 uint32t values
	{
		uint8_t* w = data.ptrw();
		uint32_t* children_cells = (uint32_t*)w;
		const Cell* cells = bake_cells.ptr();

		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			for (uint32_t j = 0; j < 8; j++) {
				children_cells[i * 8 + j] = cells[i].children[j];
			}
		}
	}

	return data;
}

Vector<uint8_t> Voxelizer::get_voxel_gi_data_cells() const
{
	Vector<uint8_t> data;
	data.resize((4 * 4) * bake_cells.size()); // 8 uint32t values
	{
		uint8_t* w = data.ptrw();
		uint32_t* dataptr = (uint32_t*)w;
		const Cell* cells = bake_cells.ptr();

		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			{ // position

				uint32_t x = cells[i].x;
				uint32_t y = cells[i].y;
				uint32_t z = cells[i].z;

				uint32_t position = x;
				position |= y << 11;
				position |= z << 21;

				dataptr[i * 4 + 0] = position;
			}

			{ // albedo + alpha
				uint32_t rgba = uint32_t(CLAMP(cells[i].alpha * 255.0, 0, 255)) << 24; // a
				rgba |= uint32_t(CLAMP(cells[i].albedo[2] * 255.0, 0, 255)) << 16;	   // b
				rgba |= uint32_t(CLAMP(cells[i].albedo[1] * 255.0, 0, 255)) << 8;	   // g
				rgba |= uint32_t(CLAMP(cells[i].albedo[0] * 255.0, 0, 255));		   // r

				dataptr[i * 4 + 1] = rgba;
			}

			{ // emission, as rgbe9995
				Color emission =
					Color(cells[i].emission[0], cells[i].emission[1], cells[i].emission[2]);
				dataptr[i * 4 + 2] = emission.to_rgbe9995();
			}

			{ // normal

				Vector3 n(
					bake_cells[i].normal[0], bake_cells[i].normal[1], bake_cells[i].normal[2]);
				n.normalize();

				uint32_t normal = uint32_t(uint8_t(int8_t(CLAMP(n.x * 127.0, -128, 127))));
				normal |= uint32_t(uint8_t(int8_t(CLAMP(n.y * 127.0, -128, 127)))) << 8;
				normal |= uint32_t(uint8_t(int8_t(CLAMP(n.z * 127.0, -128, 127)))) << 16;

				dataptr[i * 4 + 3] = normal;
			}
		}
	}

	return data;
}

Vector<int> Voxelizer::get_voxel_gi_level_cell_count() const
{
	uint32_t cell_count = bake_cells.size();
	const Cell* cells = bake_cells.ptr();
	Vector<int> level_count;
	level_count.resize(cell_subdiv + 1); // remember, always x+1 levels for x subdivisions
	{
		int* w = level_count.ptrw();
		for (int i = 0; i < cell_subdiv + 1; i++) {
			w[i] = 0;
		}

		for (uint32_t i = 0; i < cell_count; i++) {
			w[cells[i].level]++;
		}
	}

	return level_count;
}

// euclidean distance computation based on:
// https://prideout.net/blog/distance_fields/

#define square(m_s) ((m_s) * (m_s))
#define BIG_VAL 1e20

/* dt of 1d function using squared distance */
static void edt(float* f, int stride, int n)
{
	float* d = (float*)alloca(sizeof(float) * n + sizeof(int) * n + sizeof(float) * (n + 1));
	int* v = reinterpret_cast<int*>(&(d[n]));
	float* z = reinterpret_cast<float*>(&v[n]);

	int k = 0;
	v[0] = 0;
	z[0] = -BIG_VAL;
	z[1] = +BIG_VAL;
	for (int q = 1; q <= n - 1; q++) {
		float s =
			((f[q * stride] + square(q)) - (f[v[k] * stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		while (s <= z[k]) {
			k--;
			s = ((f[q * stride] + square(q)) - (f[v[k] * stride] + square(v[k]))) /
				(2 * q - 2 * v[k]);
		}
		k++;
		v[k] = q;

		z[k] = s;
		z[k + 1] = +BIG_VAL;
	}

	k = 0;
	for (int q = 0; q <= n - 1; q++) {
		while (z[k + 1] < q) {
			k++;
		}
		d[q] = square(q - v[k]) + f[v[k] * stride];
	}

	for (int i = 0; i < n; i++) {
		f[i * stride] = d[i];
	}
}

#undef square

Voxelizer::BakeResult Voxelizer::get_sdf_3d_image(
	Vector<uint8_t>& r_image, BakeStepFunc p_bake_step_function) const
{
	Vector3i octree_size = get_voxel_gi_octree_size();

	uint32_t float_count = octree_size.x * octree_size.y * octree_size.z;
	float* work_memory = memnew_arr(float, float_count);
	for (uint32_t i = 0; i < float_count; i++) {
		work_memory[i] = BIG_VAL;
	}

	uint32_t y_mult = octree_size.x;
	uint32_t z_mult = y_mult * octree_size.y;

	// plot solid cells
	{
		const Cell* cells = bake_cells.ptr();
		uint32_t cell_count = bake_cells.size();

		for (uint32_t i = 0; i < cell_count; i++) {
			if (cells[i].level < cell_subdiv) {
				continue; // do not care about this level
			}

			work_memory[cells[i].x + cells[i].y * y_mult + cells[i].z * z_mult] = 0;
		}
	}

	// process in each direction

	int bake_total = octree_size.x * 2 + octree_size.y, bake_current = 0;

	// xy->z

	for (int i = 0; i < octree_size.x; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.y; j++) {
			edt(&work_memory[i + j * y_mult], z_mult, octree_size.z);
		}
	}

	// xz->y

	for (int i = 0; i < octree_size.x; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.z; j++) {
			edt(&work_memory[i + j * z_mult], y_mult, octree_size.y);
		}
	}

	// yz->x
	for (int i = 0; i < octree_size.y; i++, bake_current++) {
		if (p_bake_step_function) {
			if (p_bake_step_function(bake_current, bake_total)) {
				memdelete_arr(work_memory);
				return BAKE_RESULT_CANCELLED;
			}
		}
		for (int j = 0; j < octree_size.z; j++) {
			edt(&work_memory[i * y_mult + j * z_mult], 1, octree_size.x);
		}
	}

	r_image.resize(float_count);
	{
		uint8_t* w = r_image.ptrw();
		for (uint32_t i = 0; i < float_count; i++) {
			uint32_t d = uint32_t(Math::sqrt(work_memory[i]));
			if (d == 0) {
				w[i] = 0;
			}
			else {
				w[i] = MIN(d, 254u) + 1;
			}
		}
	}

	memdelete_arr(work_memory);

	return BAKE_RESULT_OK;
}

#undef BIG_VAL

void Voxelizer::_debug_mesh(
	int p_idx, int p_level, const AABB& p_aabb, Ref<MultiMesh>& p_multimesh, int& idx)
{
	if (p_level == cell_subdiv - 1) {
		Vector3 center = p_aabb.get_center();
		Transform3D xform;
		xform.origin = center;
		xform.basis.scale(p_aabb.size * 0.5);
		p_multimesh->set_instance_transform(idx, xform);
		Color col;
		col = Color(
			bake_cells[p_idx].albedo[0], bake_cells[p_idx].albedo[1], bake_cells[p_idx].albedo[2]);
		// Color col = Color(bake_cells[p_idx].emission[0], bake_cells[p_idx].emission[1],
		// bake_cells[p_idx].emission[2]);
		p_multimesh->set_instance_color(idx, col);

		idx++;

	}
	else {
		for (int i = 0; i < 8; i++) {
			uint32_t child = bake_cells[p_idx].children[i];

			if (child == CHILD_EMPTY || child >= (uint32_t)max_original_cells) {
				continue;
			}

			AABB aabb = p_aabb;
			aabb.size *= 0.5;

			if (i & 1) {
				aabb.position.x += aabb.size.x;
			}
			if (i & 2) {
				aabb.position.y += aabb.size.y;
			}
			if (i & 4) {
				aabb.position.z += aabb.size.z;
			}

			_debug_mesh(bake_cells[p_idx].children[i], p_level + 1, aabb, p_multimesh, idx);
		}
	}
}

Transform3D Voxelizer::get_to_cell_space_xform() const { return to_cell_space; }

Voxelizer::Voxelizer() {}

// ==========================================================================
// Merged from scene/3d/lightmap_gi.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry_3d.h"
// --- Inlined from scene/3d/lightmap_gi.h ---
#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "scene/3d/visual_instance_3d.h"

class CameraAttributes;
class Light3D;
class Mesh;
class Sky;

class LightmapGIData : public Resource
{
public:
	enum ShadowmaskMode
	{
		SHADOWMASK_MODE_NONE,
		SHADOWMASK_MODE_REPLACE,
		SHADOWMASK_MODE_OVERLAY,
		SHADOWMASK_MODE_ONLY,
	};

private:
	// The 'merged' texture atlases actually used by the renderer.
	Ref<TextureLayered> combined_light_texture;
	Ref<TextureLayered> combined_shadowmask_texture;

	bool uses_spherical_harmonics = false;
	bool interior = false;

	bool _uses_packed_directional = false;

	RID lightmap;
	AABB bounds;
	float baked_exposure = 1.0;
	uint32_t lightprobe_hash = 0;

	struct User
	{
		NodePath path;
		int32_t sub_instance = 0;
		Rect2 uv_scale;
		int slice_index = 0;
	};

	Vector<User> users;

	void _reset_lightmap_textures();
	void _reset_shadowmask_textures();

protected:

public:
	void add_user(const NodePath& p_path, const Rect2& p_uv_scale, int p_slice_index,
		int32_t p_sub_instance = -1);
	int get_user_count() const;
	NodePath get_user_path(int p_user) const;
	int32_t get_user_sub_instance(int p_user) const;
	Rect2 get_user_lightmap_uv_scale(int p_user) const;
	int get_user_lightmap_slice_index(int p_user) const;
	void clear_users();

#ifndef DISABLE_DEPRECATED
	void set_light_texture(const Ref<TextureLayered>& p_light_texture);
	Ref<TextureLayered> get_light_texture() const;
#endif

	void set_uses_spherical_harmonics(bool p_enable);
	bool is_using_spherical_harmonics() const;

	void _set_uses_packed_directional(bool p_enable);
	bool _is_using_packed_directional() const;

	void update_shadowmask_mode(ShadowmaskMode p_mode);
	ShadowmaskMode get_shadowmask_mode() const;

	bool is_interior() const;
	float get_baked_exposure() const;

	void set_capture_data(const AABB& p_bounds, bool p_interior, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree, float p_baked_exposure, uint32_t p_lightprobe_hash);
	PackedVector3Array get_capture_points() const;
	PackedColorArray get_capture_sh() const;
	PackedInt32Array get_capture_tetrahedra() const;
	PackedInt32Array get_capture_bsp_tree() const;
	uint32_t get_lightprobe_hash() const;

	AABB get_capture_bounds() const;

	void clear();

	void clear_shadowmask_textures();
	bool has_shadowmask_textures();

	virtual RID get_rid() const override;
	LightmapGIData();
	~LightmapGIData();
};

class LightmapGI : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum BakeQuality
	{
		BAKE_QUALITY_LOW,
		BAKE_QUALITY_MEDIUM,
		BAKE_QUALITY_HIGH,
		BAKE_QUALITY_ULTRA,
	};

	enum GenerateProbes
	{
		GENERATE_PROBES_DISABLED,
		GENERATE_PROBES_SUBDIV_4,
		GENERATE_PROBES_SUBDIV_8,
		GENERATE_PROBES_SUBDIV_16,
		GENERATE_PROBES_SUBDIV_32,
	};

	enum BakeError
	{
		BAKE_ERROR_OK,
		BAKE_ERROR_NO_SCENE_ROOT,
		BAKE_ERROR_FOREIGN_DATA,
		BAKE_ERROR_NO_LIGHTMAPPER,
		BAKE_ERROR_NO_SAVE_PATH,
		BAKE_ERROR_NO_MESHES,
		BAKE_ERROR_MESHES_INVALID,
		BAKE_ERROR_CANT_CREATE_IMAGE,
		BAKE_ERROR_USER_ABORTED,
		BAKE_ERROR_TEXTURE_SIZE_TOO_SMALL,
		BAKE_ERROR_LIGHTMAP_TOO_SMALL,
		BAKE_ERROR_ATLAS_TOO_SMALL,
	};

	enum EnvironmentMode
	{
		ENVIRONMENT_MODE_DISABLED,
		ENVIRONMENT_MODE_SCENE,
		ENVIRONMENT_MODE_CUSTOM_SKY,
		ENVIRONMENT_MODE_CUSTOM_COLOR,
	};

private:
	BakeQuality bake_quality = BAKE_QUALITY_MEDIUM;
	bool use_denoiser = true;
	float denoiser_strength = 0.1f;
	int denoiser_range = 10;
	int bounces = 3;
	float bounce_indirect_energy = 1.0;
	float bias = 0.0005;
	float texel_scale = 1.0;
	int max_texture_size = 16384;
	bool supersampling_enabled = false;
	float supersampling_factor = 2.0;
	bool interior = false;
	EnvironmentMode environment_mode = ENVIRONMENT_MODE_SCENE;
	Ref<Sky> environment_custom_sky;
	Color environment_custom_color = Color(1, 1, 1);
	float environment_custom_energy = 1.0;
	bool directional = false;
	bool use_texture_for_bounces = true;
	LightmapGIData::ShadowmaskMode shadowmask_mode = LightmapGIData::SHADOWMASK_MODE_NONE;
	GenerateProbes gen_probes = GENERATE_PROBES_SUBDIV_8;
	Ref<CameraAttributes> camera_attributes;

	Ref<LightmapGIData> light_data;
	Node* last_owner = nullptr;

	struct LightsFound
	{
		Transform3D xform;
		Light3D* light = nullptr;
	};

	struct AreaLightAtlasTexture
	{
		Rect2 texture_rect;
		float max_mipmap;
	};

	struct MeshesFound
	{
		Transform3D xform;
		NodePath node_path;
		int32_t subindex = 0;
		Ref<Mesh> mesh;
		float lightmap_scale = 0.0;
		Vector<Ref<Material>> overrides;
	};

	void _find_meshes_and_lights(Node* p_at_node, Vector<MeshesFound>& meshes,
		Vector<LightsFound>& lights, Vector<Vector3>& probes);

	void _assign_lightmaps();
	void _clear_lightmaps();

	struct BSPSimplex
	{
		int vertices[4] = {};
		int planes[4] = {};
	};

	struct BSPNode
	{
		static const int32_t EMPTY_LEAF = INT32_MIN;
		Plane plane;
		int32_t over = EMPTY_LEAF;
		int32_t under = EMPTY_LEAF;
	};

	int _bsp_get_simplex_side(const LocalVector<Vector3>& p_points,
		const LocalVector<BSPSimplex>& p_simplices, const Plane& p_plane, uint32_t p_simplex) const;
	int32_t _compute_bsp_tree(const LocalVector<Vector3>& p_points,
		const LocalVector<Plane>& p_planes, LocalVector<int32_t>& planes_tested,
		const LocalVector<BSPSimplex>& p_simplices, const LocalVector<int32_t>& p_simplex_indices,
		LocalVector<BSPNode>& bsp_nodes);

	struct BakeStepUD
	{
		Lightmapper::BakeStepFunc func;
		void* ud = nullptr;
		float from_percent = 0.0;
		float to_percent = 0.0;
	};

	static bool _lightmap_bake_step_function(
		float p_completion, const String& p_text, void* ud, bool p_refresh);

	struct GenProbesOctree
	{
		Vector3i offset;
		uint32_t size = 0;
		GenProbesOctree* children[8] = {
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};

		~GenProbesOctree()
		{
			for (int i = 0; i < 8; i++) {
				memdelete(children[i]);
			}
		}
	};

	void _plot_triangle_into_octree(
		GenProbesOctree* p_cell, float p_cell_size, const Vector3* p_triangle);
	void _gen_new_positions_from_octree(const GenProbesOctree* p_cell, float p_cell_size,
		const Vector<Vector3>& probe_positions, LocalVector<Vector3>& new_probe_positions,
		HashMap<Vector3i, bool>& positions_used, const AABB& p_bounds);

	void _build_area_light_texture_atlas(const Vector<LightmapGI::LightsFound>& lights_found,
		HashMap<Ref<Texture2D>, AreaLightAtlasTexture>& r_texture_rects, Size2i& r_atlas_size,
		int& r_mipmaps) const;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	Ref<LightmapGIData> get_light_data() const;

	void set_bake_quality(BakeQuality p_quality);
	BakeQuality get_bake_quality() const;

	void set_use_denoiser(bool p_enable);
	bool is_using_denoiser() const;

	void set_denoiser_strength(float p_denoiser_strength);
	float get_denoiser_strength() const;

	void set_denoiser_range(int p_denoiser_range);
	int get_denoiser_range() const;

	void set_directional(bool p_enable);
	bool is_directional() const;

	LightmapGIData::ShadowmaskMode get_shadowmask_mode() const;

	void set_use_texture_for_bounces(bool p_enable);
	bool is_using_texture_for_bounces() const;

	void set_interior(bool p_interior);
	bool is_interior() const;

	void set_environment_mode(EnvironmentMode p_mode);
	EnvironmentMode get_environment_mode() const;

	void set_environment_custom_sky(const Ref<Sky>& p_sky);
	Ref<Sky> get_environment_custom_sky() const;

	void set_environment_custom_color(const Color& p_color);
	Color get_environment_custom_color() const;

	void set_environment_custom_energy(float p_energy);
	float get_environment_custom_energy() const;

	void set_bounces(int p_bounces);
	int get_bounces() const;

	void set_bounce_indirect_energy(float p_indirect_energy);
	float get_bounce_indirect_energy() const;

	void set_bias(float p_bias);
	float get_bias() const;

	void set_texel_scale(float p_multiplier);
	float get_texel_scale() const;

	void set_max_texture_size(int p_size);
	int get_max_texture_size() const;

	void set_supersampling_enabled(bool p_enable);
	bool is_supersampling_enabled() const;

	void set_supersampling_factor(float p_factor);
	float get_supersampling_factor() const;

	void set_generate_probes(GenerateProbes p_generate_probes);
	GenerateProbes get_generate_probes() const;

	void set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes);
	Ref<CameraAttributes> get_camera_attributes() const;

	AABB get_aabb() const;

	BakeError bake(Node* p_from_node, String p_image_data_path = "",
		Lightmapper::BakeStepFunc p_bake_step = nullptr, void* p_bake_userdata = nullptr);

	virtual PackedStringArray get_configuration_warnings() const override;

	LightmapGI();
};



// --- End scene/3d/lightmap_gi.h ---
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For lightmapper_rd.
#include "scene/3d/visual_instance_3d.h"
// --- Inlined from scene/3d/lightmap_probe.h ---
#include "scene/3d/node_3d.h"

class LightmapProbe : public Node3D
{
public:
	LightmapProbe();
};



// --- End scene/3d/lightmap_probe.h ---
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/material.h"
#include "scene/resources/environment.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/environment.h"
#include "servers/rendering/renderer.h"

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
#include "servers/display/display_server.h"
#endif

#if defined(ANDROID_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
#include "core/os/os.h"
#endif

void LightmapGIData::add_user(
	const NodePath& p_path, const Rect2& p_uv_scale, int p_slice_index, int32_t p_sub_instance)
{
	User user;
	user.path = p_path;
	user.uv_scale = p_uv_scale;
	user.slice_index = p_slice_index;
	user.sub_instance = p_sub_instance;
	users.push_back(user);
}

int LightmapGIData::get_user_count() const { return users.size(); }

NodePath LightmapGIData::get_user_path(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), NodePath());
	return users[p_user].path;
}

int32_t LightmapGIData::get_user_sub_instance(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].sub_instance;
}

Rect2 LightmapGIData::get_user_lightmap_uv_scale(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), Rect2());
	return users[p_user].uv_scale;
}

int LightmapGIData::get_user_lightmap_slice_index(int p_user) const
{
	ERR_FAIL_INDEX_V(p_user, users.size(), -1);
	return users[p_user].slice_index;
}

void LightmapGIData::clear_users() { users.clear(); }

RID LightmapGIData::get_rid() const { return lightmap; }

void LightmapGIData::clear() { users.clear(); }

void LightmapGIData::_reset_lightmap_textures()
{
	Renderer::lightmap_set_textures(lightmap,
		combined_light_texture.is_valid() ? combined_light_texture->get_rid() : RID(),
		uses_spherical_harmonics);
}

void LightmapGIData::_reset_shadowmask_textures()
{
	Renderer::lightmap_set_shadowmask_textures(lightmap,
		combined_shadowmask_texture.is_valid() ? combined_shadowmask_texture->get_rid() : RID());
}

void LightmapGIData::set_uses_spherical_harmonics(bool p_enable)
{
	uses_spherical_harmonics = p_enable;
	_reset_lightmap_textures();
}

bool LightmapGIData::is_using_spherical_harmonics() const { return uses_spherical_harmonics; }

void LightmapGIData::_set_uses_packed_directional(bool p_enable)
{
	_uses_packed_directional = p_enable;
}

bool LightmapGIData::_is_using_packed_directional() const { return _uses_packed_directional; }

void LightmapGIData::update_shadowmask_mode(ShadowmaskMode p_mode)
{
	Renderer::lightmap_set_shadowmask_mode(lightmap, (RSE::ShadowmaskMode)p_mode);
}

LightmapGIData::ShadowmaskMode LightmapGIData::get_shadowmask_mode() const
{
	return (ShadowmaskMode)Renderer::lightmap_get_shadowmask_mode(lightmap);
}

void LightmapGIData::set_capture_data(const AABB& p_bounds, bool p_interior,
	const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
	const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree,
	float p_baked_exposure, uint32_t p_lightprobe_hash)
{
	if (p_points.size()) {
		int pc = p_points.size();
		ERR_FAIL_COND(pc * 9 != p_point_sh.size());
		ERR_FAIL_COND((p_tetrahedra.size() % 4) != 0);
		ERR_FAIL_COND((p_bsp_tree.size() % 6) != 0);
		Renderer::lightmap_set_probe_capture_data(
			lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
		Renderer::lightmap_set_probe_bounds(lightmap, p_bounds);
		Renderer::lightmap_set_probe_interior(lightmap, p_interior);
	}
	else {
		Renderer::lightmap_set_probe_capture_data(lightmap, PackedVector3Array(),
			PackedColorArray(), PackedInt32Array(), PackedInt32Array());
		Renderer::lightmap_set_probe_bounds(lightmap, AABB());
		Renderer::lightmap_set_probe_interior(lightmap, false);
	}
	Renderer::lightmap_set_baked_exposure_normalization(lightmap, p_baked_exposure);
	baked_exposure = p_baked_exposure;
	lightprobe_hash = p_lightprobe_hash;
	interior = p_interior;
	bounds = p_bounds;
}

PackedVector3Array LightmapGIData::get_capture_points() const
{
	return Renderer::lightmap_get_probe_capture_points(lightmap);
}

PackedColorArray LightmapGIData::get_capture_sh() const
{
	return Renderer::lightmap_get_probe_capture_sh(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_tetrahedra() const
{
	return Renderer::lightmap_get_probe_capture_tetrahedra(lightmap);
}

PackedInt32Array LightmapGIData::get_capture_bsp_tree() const
{
	return Renderer::lightmap_get_probe_capture_bsp_tree(lightmap);
}

uint32_t LightmapGIData::get_lightprobe_hash() const { return lightprobe_hash; }

AABB LightmapGIData::get_capture_bounds() const { return bounds; }

bool LightmapGIData::is_interior() const { return interior; }

float LightmapGIData::get_baked_exposure() const { return baked_exposure; }

LightmapGIData::LightmapGIData() { lightmap = Renderer::lightmap_create(); }

LightmapGIData::~LightmapGIData()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(lightmap);
}

///////////////////////////

int LightmapGI::_bsp_get_simplex_side(const LocalVector<Vector3>& p_points,
	const LocalVector<BSPSimplex>& p_simplices, const Plane& p_plane, uint32_t p_simplex) const
{
	int over = 0;
	int under = 0;
	const BSPSimplex& s = p_simplices[p_simplex];
	for (int i = 0; i < 4; i++) {
		const Vector3 v = p_points[s.vertices[i]];
		// The tolerance used here comes from experiments on scenes up to
		// 1000x1000x100 meters. If it's any smaller, some simplices will
		// appear to self-intersect due to a lack of precision in Plane.
		if (p_plane.has_point(v, 1.0 / (1 << 13))) {
			// Coplanar.
		}
		else if (p_plane.is_point_over(v)) {
			over++;
		}
		else {
			under++;
		}
	}

	ERR_FAIL_COND_V(under == 0 && over == 0,
		-2); // should never happen, we discarded flat simplices before, but in any case drop it
			 // from the bsp tree and throw an error
	if (under == 0) {
		return 1; // all over
	}
	else if (over == 0) {
		return -1; // all under
	}
	else {
		return 0; // crossing
	}
}

// #define DEBUG_BSP

int32_t LightmapGI::_compute_bsp_tree(const LocalVector<Vector3>& p_points,
	const LocalVector<Plane>& p_planes, LocalVector<int32_t>& planes_tested,
	const LocalVector<BSPSimplex>& p_simplices, const LocalVector<int32_t>& p_simplex_indices,
	LocalVector<BSPNode>& bsp_nodes)
{
	ERR_FAIL_COND_V(p_simplex_indices.size() < 2, -1);

	int32_t node_index = (int32_t)bsp_nodes.size();
	bsp_nodes.push_back(BSPNode());

	// test with all the simplex planes
	Plane best_plane;
	float best_plane_score = -1.0;

	for (const int idx : p_simplex_indices) {
		const BSPSimplex& s = p_simplices[idx];
		for (int j = 0; j < 4; j++) {
			uint32_t plane_index = s.planes[j];
			if (planes_tested[plane_index] == node_index) {
				continue; // tested this plane already
			}

			planes_tested[plane_index] = node_index;

			static const int face_order[4][3] = {{0, 1, 2}, {0, 2, 3}, {0, 1, 3}, {1, 2, 3}};

			// despite getting rid of plane duplicates, we should still use here the actual plane to
			// avoid numerical error from thinking this same simplex is intersecting rather than on
			// a side
			Vector3 v0 = p_points[s.vertices[face_order[j][0]]];
			Vector3 v1 = p_points[s.vertices[face_order[j][1]]];
			Vector3 v2 = p_points[s.vertices[face_order[j][2]]];

			Plane plane(v0, v1, v2);

			// test with all the simplices
			int over_count = 0;
			int under_count = 0;

			for (const int& index : p_simplex_indices) {
				int side = _bsp_get_simplex_side(p_points, p_simplices, plane, index);
				if (side == -2) {
					continue; // this simplex is invalid, skip for now
				}
				else if (side < 0) {
					under_count++;
				}
				else if (side > 0) {
					over_count++;
				}
			}

			if (under_count == 0 && over_count == 0) {
				continue; // most likely precision issue with a flat simplex, do not try this plane
			}

			if (under_count > over_count) { // make sure under is always less than over, so we can
											// compute the same ratio
				SWAP(under_count, over_count);
			}

			float score = 0; // by default, score is 0 (worst)
			if (over_count > 0) {
				// Simplices that are intersected by the plane are moved into both the over
				// and under subtrees which makes the entire tree deeper, so the best plane
				// will have the least intersections while separating the simplices evenly.
				float balance = float(under_count) / over_count;
				float separation = float(over_count + under_count) / p_simplex_indices.size();
				score = balance * separation * separation;
			}

			if (score > best_plane_score) {
				best_plane = plane;
				best_plane_score = score;
			}
		}
	}

	// We often end up with two (or on rare occasions, three) simplices that are
	// either disjoint or share one vertex and don't have a separating plane
	// among their faces. The fallback is to loop through new planes created
	// with one vertex of the first simplex and two vertices of the second until
	// we find a winner.
	if (best_plane_score == 0) {
		const BSPSimplex& simplex0 = p_simplices[p_simplex_indices[0]];
		const BSPSimplex& simplex1 = p_simplices[p_simplex_indices[1]];

		for (uint32_t i = 0; i < 4 && !best_plane_score; i++) {
			Vector3 v0 = p_points[simplex0.vertices[i]];
			for (uint32_t j = 0; j < 3 && !best_plane_score; j++) {
				if (simplex0.vertices[i] == simplex1.vertices[j]) {
					break;
				}
				Vector3 v1 = p_points[simplex1.vertices[j]];
				for (uint32_t k = j + 1; k < 4; k++) {
					if (simplex0.vertices[i] == simplex1.vertices[k]) {
						break;
					}
					Vector3 v2 = p_points[simplex1.vertices[k]];

					Plane plane = Plane(v0, v1, v2);
					if (plane ==
						Plane()) { // When v0, v1, and v2 are collinear, they can't form a plane.
						continue;
					}
					int32_t side0 =
						_bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[0]);
					int32_t side1 =
						_bsp_get_simplex_side(p_points, p_simplices, plane, p_simplex_indices[1]);
					if ((side0 == 1 && side1 == -1) || (side0 == -1 && side1 == 1)) {
						best_plane = plane;
						best_plane_score = 1.0;
						break;
					}
				}
			}
		}
	}

	LocalVector<int32_t> indices_over;
	LocalVector<int32_t> indices_under;

	// split again, but add to list
	for (const uint32_t index : p_simplex_indices) {
		int side = _bsp_get_simplex_side(p_points, p_simplices, best_plane, index);

		if (side == -2) {
			continue; // simplex sits on the plane, does not make sense to use it
		}
		if (side <= 0) {
			indices_under.push_back(index);
		}

		if (side >= 0) {
			indices_over.push_back(index);
		}
	}

#ifdef DEBUG_BSP
	print_line("node " + itos(node_index) + " found plane: " + best_plane +
			   " score:" + rtos(best_plane_score) + " - over " + itos(indices_over.size()) +
			   " under " + itos(indices_under.size()) + " intersecting " + itos(intersecting));
#endif

	if (best_plane_score < 0.0 || indices_over.size() == p_simplex_indices.size() ||
		indices_under.size() == p_simplex_indices.size()) {
		// Failed to separate the tetrahedrons using planes
		// this means Delaunay broke at some point.
		// Luckily, because we are using tetrahedrons, we can resort to
		// less precise but still working ways to generate the separating plane
		// this will most likely look bad when interpolating, but at least it will not crash.
		// and the artifact will most likely also be very small, so too difficult to notice.

		// find the longest axis

		WARN_PRINT("Inconsistency found in triangulation while building BSP, probe interpolation "
				   "quality may degrade a bit.");

		LocalVector<Vector3> centers;
		AABB bounds_all;
		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			AABB bounds;
			for (uint32_t j = 0; j < 4; j++) {
				Vector3 p = p_points[p_simplices[p_simplex_indices[i]].vertices[j]];
				if (j == 0) {
					bounds.position = p;
				}
				else {
					bounds.expand_to(p);
				}
			}
			if (i == 0) {
				centers.push_back(bounds.get_center());
			}
			else {
				bounds_all.merge_with(bounds);
			}
		}
		Vector3::Axis longest_axis = Vector3::Axis(bounds_all.get_longest_axis_index());

		// find the simplex that will go under
		uint32_t min_d_idx = 0xFFFFFFFF;
		float min_d_dist = 1e20;

		for (uint32_t i = 0; i < centers.size(); i++) {
			if (centers[i][longest_axis] < min_d_dist) {
				min_d_idx = i;
				min_d_dist = centers[i][longest_axis];
			}
		}
		// rebuild best_plane and over/under arrays
		best_plane = Plane();
		best_plane.normal[longest_axis] = 1.0;
		best_plane.d = min_d_dist;

		indices_under.clear();
		indices_under.push_back(min_d_idx);

		indices_over.clear();

		for (uint32_t i = 0; i < p_simplex_indices.size(); i++) {
			if (i == min_d_idx) {
				continue;
			}
			indices_over.push_back(p_simplex_indices[i]);
		}
	}

	BSPNode node;
	node.plane = best_plane;

	if (indices_under.is_empty()) {
		// nothing to do here
		node.under = BSPNode::EMPTY_LEAF;
	}
	else if (indices_under.size() == 1) {
		node.under = -(indices_under[0] + 1);
	}
	else {
		node.under = _compute_bsp_tree(
			p_points, p_planes, planes_tested, p_simplices, indices_under, bsp_nodes);
	}

	if (indices_over.is_empty()) {
		// nothing to do here
		node.over = BSPNode::EMPTY_LEAF;
	}
	else if (indices_over.size() == 1) {
		node.over = -(indices_over[0] + 1);
	}
	else {
		node.over = _compute_bsp_tree(
			p_points, p_planes, planes_tested, p_simplices, indices_over, bsp_nodes);
	}

	bsp_nodes[node_index] = node;

	return node_index;
}

bool LightmapGI::_lightmap_bake_step_function(
	float p_completion, const String& p_text, void* ud, bool p_refresh)
{
	BakeStepUD* bsud = (BakeStepUD*)ud;
	bool ret = false;
	if (bsud->func) {
		ret =
			bsud->func(bsud->from_percent + p_completion * (bsud->to_percent - bsud->from_percent),
				p_text, bsud->ud, p_refresh);
	}
	return ret;
}

void LightmapGI::_plot_triangle_into_octree(
	GenProbesOctree* p_cell, float p_cell_size, const Vector3* p_triangle)
{
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		uint32_t half_size = p_cell->size / 2;
		if (i & 1) {
			pos.x += half_size;
		}
		if (i & 2) {
			pos.y += half_size;
		}
		if (i & 4) {
			pos.z += half_size;
		}

		AABB subcell;
		subcell.position = Vector3(pos) * p_cell_size;
		subcell.size = Vector3(half_size, half_size, half_size) * p_cell_size;

		if (!Geometry3D::triangle_box_overlap(
				subcell.get_center(), subcell.size * 0.5, p_triangle)) {
			continue;
		}

		if (p_cell->children[i] == nullptr) {
			GenProbesOctree* child = memnew(GenProbesOctree);
			child->offset = pos;
			child->size = half_size;
			p_cell->children[i] = child;
		}

		if (half_size > 1) {
			// still levels missing
			_plot_triangle_into_octree(p_cell->children[i], p_cell_size, p_triangle);
		}
	}
}

void LightmapGI::_gen_new_positions_from_octree(const GenProbesOctree* p_cell, float p_cell_size,
	const Vector<Vector3>& probe_positions, LocalVector<Vector3>& new_probe_positions,
	HashMap<Vector3i, bool>& positions_used, const AABB& p_bounds)
{
	for (int i = 0; i < 8; i++) {
		Vector3i pos = p_cell->offset;
		if (i & 1) {
			pos.x += p_cell->size;
		}
		if (i & 2) {
			pos.y += p_cell->size;
		}
		if (i & 4) {
			pos.z += p_cell->size;
		}

		if (p_cell->size == 1 && !positions_used.has(pos)) {
			// new position to insert!
			Vector3 real_pos = p_bounds.position + Vector3(pos) * p_cell_size;
			// see if a user submitted probe is too close
			int ppcount = probe_positions.size();
			const Vector3* pp = probe_positions.ptr();
			bool exists = false;
			for (int j = 0; j < ppcount; j++) {
				if (pp[j].distance_to(real_pos) < (p_cell_size * 0.5f)) {
					exists = true;
					break;
				}
			}

			if (!exists) {
				new_probe_positions.push_back(real_pos);
			}

			positions_used[pos] = true;
		}

		if (p_cell->children[i] != nullptr) {
			_gen_new_positions_from_octree(p_cell->children[i], p_cell_size, probe_positions,
				new_probe_positions, positions_used, p_bounds);
		}
	}
}

void LightmapGI::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void LightmapGI::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (light_data.is_valid()) {
			ERR_FAIL_COND_MSG(light_data->is_using_spherical_harmonics() &&
								  !light_data->_is_using_packed_directional(),
				vformat("%s (%s): The directional lightmap textures are stored in a format that "
						"isn't supported anymore. Please bake lightmaps again to make lightmaps "
						"display from this node again.",
					get_light_data()->get_path(), get_name()));

			if (last_owner && last_owner != get_owner()) {
				light_data->clear_users();
			}

			_assign_lightmaps();
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		last_owner = get_owner();

		if (light_data.is_valid()) {
			_clear_lightmaps();
		}
	} break;
	}
}

Ref<LightmapGIData> LightmapGI::get_light_data() const { return light_data; }

void LightmapGI::set_bake_quality(BakeQuality p_quality) { bake_quality = p_quality; }

LightmapGI::BakeQuality LightmapGI::get_bake_quality() const { return bake_quality; }

AABB LightmapGI::get_aabb() const { return AABB(); }

bool LightmapGI::is_using_denoiser() const { return use_denoiser; }

void LightmapGI::set_denoiser_strength(float p_denoiser_strength)
{
	denoiser_strength = p_denoiser_strength;
}

float LightmapGI::get_denoiser_strength() const { return denoiser_strength; }

void LightmapGI::set_denoiser_range(int p_denoiser_range) { denoiser_range = p_denoiser_range; }

int LightmapGI::get_denoiser_range() const { return denoiser_range; }

void LightmapGI::set_directional(bool p_enable) { directional = p_enable; }

bool LightmapGI::is_directional() const { return directional; }

LightmapGIData::ShadowmaskMode LightmapGI::get_shadowmask_mode() const { return shadowmask_mode; }

void LightmapGI::set_use_texture_for_bounces(bool p_enable) { use_texture_for_bounces = p_enable; }

bool LightmapGI::is_using_texture_for_bounces() const { return use_texture_for_bounces; }

void LightmapGI::set_interior(bool p_enable) { interior = p_enable; }

bool LightmapGI::is_interior() const { return interior; }

LightmapGI::EnvironmentMode LightmapGI::get_environment_mode() const { return environment_mode; }

void LightmapGI::set_environment_custom_sky(const Ref<Sky>& p_sky)
{
	environment_custom_sky = p_sky;
}

Ref<Sky> LightmapGI::get_environment_custom_sky() const { return environment_custom_sky; }

void LightmapGI::set_environment_custom_color(const Color& p_color)
{
	environment_custom_color = p_color;
}

Color LightmapGI::get_environment_custom_color() const { return environment_custom_color; }

void LightmapGI::set_environment_custom_energy(float p_energy)
{
	environment_custom_energy = p_energy;
}

float LightmapGI::get_environment_custom_energy() const { return environment_custom_energy; }

void LightmapGI::set_bounces(int p_bounces)
{
	ERR_FAIL_COND(p_bounces < 0 || p_bounces > 16);
	bounces = p_bounces;
}

int LightmapGI::get_bounces() const { return bounces; }

void LightmapGI::set_bounce_indirect_energy(float p_indirect_energy)
{
	ERR_FAIL_COND(p_indirect_energy < 0.0);
	bounce_indirect_energy = p_indirect_energy;
}

float LightmapGI::get_bounce_indirect_energy() const { return bounce_indirect_energy; }

void LightmapGI::set_bias(float p_bias)
{
	ERR_FAIL_COND(p_bias < 0.00001);
	bias = p_bias;
}

float LightmapGI::get_bias() const { return bias; }

void LightmapGI::set_texel_scale(float p_multiplier)
{
	ERR_FAIL_COND(p_multiplier < (0.01 - CMP_EPSILON));
	texel_scale = p_multiplier;
}

float LightmapGI::get_texel_scale() const { return texel_scale; }

void LightmapGI::set_max_texture_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 2048, vformat("The LightmapGI maximum texture size supplied (%d) is "
											 "too small. The minimum allowed value is 2048.",
										 p_size));
	ERR_FAIL_COND_MSG(p_size > 16384, vformat("The LightmapGI maximum texture size supplied (%d) "
											  "is too large. The maximum allowed value is 16384.",
										  p_size));
	max_texture_size = p_size;
}

int LightmapGI::get_max_texture_size() const { return max_texture_size; }

bool LightmapGI::is_supersampling_enabled() const { return supersampling_enabled; }

void LightmapGI::set_supersampling_factor(float p_factor)
{
	ERR_FAIL_COND(p_factor < 1);

	supersampling_factor = p_factor;
}

float LightmapGI::get_supersampling_factor() const { return supersampling_factor; }

void LightmapGI::set_generate_probes(GenerateProbes p_generate_probes)
{
	gen_probes = p_generate_probes;
}

LightmapGI::GenerateProbes LightmapGI::get_generate_probes() const { return gen_probes; }

void LightmapGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;
}

Ref<CameraAttributes> LightmapGI::get_camera_attributes() const { return camera_attributes; }

PackedStringArray LightmapGI::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

#ifdef MODULE_LIGHTMAPPER_RD_ENABLED
	if (!DisplayServer::can_create_rendering_device()) {
		warnings.push_back(vformat(
			RTR("Lightmaps can only be baked from a GPU that supports the RenderingDevice "
				"backends.\nYour GPU (%s) does not support RenderingDevice, as it does not support "
				"Vulkan, Direct3D 12, or Metal.\nLightmap baking will not be available on this "
				"device, although rendering existing baked lightmaps will work."),
			Renderer::get_video_adapter_name()));
		return warnings;
	}

	if (shadowmask_mode != LightmapGIData::SHADOWMASK_MODE_NONE && light_data.is_valid() &&
		!light_data->has_shadowmask_textures()) {
		warnings.push_back(RTR("The lightmap has no baked shadowmask textures. Please rebake with "
							   "the Shadowmask Mode set to anything other than None."));
	}

#elif defined(ANDROID_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	warnings.push_back(vformat(
		RTR("Lightmaps cannot be baked on %s. Rendering existing baked lightmaps will still work."),
		OS::get_name()));
#else
	warnings.push_back(RTR("Lightmaps cannot be baked, as the `lightmapper_rd` module was disabled "
						   "at compile-time. Rendering existing baked lightmaps will still work."));
#endif

	return warnings;
}

LightmapGI::LightmapGI() {}



bool LightmapGIData::has_shadowmask_textures() { return false; }

void LightmapGI::_clear_lightmaps() {}

void LightmapGI::_assign_lightmaps() {}

// ==========================================================================
// Merged from scene/3d/lightmap_probe.cpp
// ==========================================================================


LightmapProbe::LightmapProbe() {
}

// ==========================================================================
// Merged from scene/3d/lightmapper.cpp
// ==========================================================================

#include "scene/3d/visual_instance_3d.h"

#include "modules/modules_enabled.gen.h"

Ref<LightmapDenoiser> (*LightmapDenoiser::create_function)() = nullptr;

Ref<LightmapDenoiser> LightmapDenoiser::create()
{
	if (create_function) {
		return Ref<LightmapDenoiser>(create_function());
	}
	return Ref<LightmapDenoiser>();
}

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<LightmapRaycaster> LightmapRaycaster::create() { return Ref<LightmapRaycaster>(); }

bool LightmapRaycaster::intersect(Ray& p_ray) { return false; }

void LightmapRaycaster::intersect(Vector<Ray>& r_rays) {}

void LightmapRaycaster::add_mesh(const Vector<Vector3>& p_vertices,
	const Vector<Vector3>& p_normals, const Vector<Vector2>& p_uv2s, unsigned int p_id)
{
}

void LightmapRaycaster::set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id) {}

void LightmapRaycaster::commit() {}

void LightmapRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void LightmapRaycaster::clear_mesh_filter() {}

LightmapRaycaster::LightmapRaycaster() {}

LightmapRaycaster::~LightmapRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

#ifndef MODULE_LIGHTMAPPER_RD_ENABLED

Ref<Lightmapper> Lightmapper::create() { return Ref<Lightmapper>(); }

Lightmapper::Lightmapper() {}

Lightmapper::~Lightmapper() {}

void Lightmapper::add_mesh(const MeshData& p_mesh) {}

void Lightmapper::add_directional_light(const String& p_name, bool p_static,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_angular_distance, float p_shadow_blur)
{
}

void Lightmapper::add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
	float p_attenuation, float p_size, float p_shadow_blur)
{
}

void Lightmapper::add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation, float p_size,
	float p_shadow_blur)
{
}

void Lightmapper::add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
	const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
	float p_range, float p_attenuation, const Vector3& p_area_width, const Vector3& p_area_height,
	float p_size, float p_shadow_blur, const Rect2& p_texture_rect, float p_max_mipmap)
{
}

void Lightmapper::add_area_light_atlas(
	const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data)
{
}

void Lightmapper::add_probe(const Vector3& p_position) {}

Lightmapper::BakeError Lightmapper::bake(BakeQuality p_quality, bool p_use_denoiser,
	float p_denoiser_strength, int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy,
	float p_bias, int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask,
	bool p_texture_for_bounces, GenerateProbes p_generate_probes,
	const Ref<Image>& p_environment_panorama, const Basis& p_environment_transform,
	BakeStepFunc p_step_function, void* p_step_userdata, float p_exposure_normalization,
	float p_supersampling_factor)
{
	return BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES;
}

int Lightmapper::get_bake_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_bake_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_shadowmask_texture_count() const { return 0; }

Ref<Image> Lightmapper::get_shadowmask_texture(int p_index) const { return Ref<Image>(); }

int Lightmapper::get_bake_mesh_count() const { return 0; }

Rect2 Lightmapper::get_bake_mesh_uv_scale(int p_index) const { return Rect2(); }

int Lightmapper::get_bake_mesh_texture_slice(int p_index) const { return 0; }

int Lightmapper::get_bake_probe_count() const { return 0; }

Vector3 Lightmapper::get_bake_probe_point(int p_probe) const { return Vector3(); }

Vector<Color> Lightmapper::get_bake_probe_sh(int p_probe) const { return Vector<Color>(); }

#endif // MODULE_LIGHTMAPPER_RD_ENABLED

// ==========================================================================
// Merged from scene/3d/reflection_probe.cpp
// ==========================================================================

#include "core/config/engine.h"
// --- Inlined from scene/3d/reflection_probe.h ---
#include "scene/3d/visual_instance_3d.h"

class ReflectionProbe : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum UpdateMode
	{
		UPDATE_ONCE,
		UPDATE_ALWAYS,
	};

	enum AmbientMode
	{
		AMBIENT_DISABLED,
		AMBIENT_ENVIRONMENT,
		AMBIENT_COLOR
	};

private:
	RID probe;
	float intensity = 1.0;
	float blend_distance = 1.0;
	float max_distance = 0.0;
	Vector3 size = Vector3(20, 20, 20);
	Vector3 origin_offset = Vector3(0, 0, 0);
	bool box_projection = false;
	bool enable_shadows = false;
	bool interior = false;
	AmbientMode ambient_mode = AMBIENT_ENVIRONMENT;
	Color ambient_color = Color(0, 0, 0);
	float ambient_color_energy = 1.0;
	float mesh_lod_threshold = 1.0;

	uint32_t cull_mask = (1 << 20) - 1;
	uint32_t reflection_mask = (1 << 20) - 1;
	UpdateMode update_mode = UPDATE_ONCE;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_intensity(float p_intensity);
	float get_intensity() const;

	float get_blend_distance() const;

	void set_ambient_mode(AmbientMode p_mode);
	AmbientMode get_ambient_mode() const;

	void set_ambient_color(Color p_ambient);
	Color get_ambient_color() const;

	void set_ambient_color_energy(float p_energy);
	float get_ambient_color_energy() const;

	void set_interior_ambient_probe_contribution(float p_contribution);
	float get_interior_ambient_probe_contribution() const;

	void set_max_distance(float p_distance);
	float get_max_distance() const;

	void set_mesh_lod_threshold(float p_pixels);
	float get_mesh_lod_threshold() const;

	Vector3 get_size() const;

	Vector3 get_origin_offset() const;

	void set_as_interior(bool p_enable);
	bool is_set_as_interior() const;

	void set_enable_box_projection(bool p_enable);
	bool is_box_projection_enabled() const;

	void set_enable_shadows(bool p_enable);
	bool are_shadows_enabled() const;

	void set_cull_mask(uint32_t p_layers);
	uint32_t get_cull_mask() const;

	void set_reflection_mask(uint32_t p_layers);
	uint32_t get_reflection_mask() const;

	void set_update_mode(UpdateMode p_mode);
	UpdateMode get_update_mode() const;

	AABB get_aabb() const;

	ReflectionProbe();
	~ReflectionProbe();
};



// --- End scene/3d/reflection_probe.h ---
#include "servers/rendering/renderer.h"

void ReflectionProbe::set_intensity(float p_intensity)
{
	intensity = p_intensity;
	Renderer::reflection_probe_set_intensity(probe, p_intensity);
}

float ReflectionProbe::get_intensity() const { return intensity; }

float ReflectionProbe::get_blend_distance() const { return blend_distance; }

ReflectionProbe::AmbientMode ReflectionProbe::get_ambient_mode() const { return ambient_mode; }

void ReflectionProbe::set_ambient_color(Color p_ambient)
{
	ambient_color = p_ambient;
	Renderer::reflection_probe_set_ambient_color(probe, p_ambient);
}

void ReflectionProbe::set_ambient_color_energy(float p_energy)
{
	ambient_color_energy = p_energy;
	Renderer::reflection_probe_set_ambient_energy(probe, p_energy);
}

float ReflectionProbe::get_ambient_color_energy() const { return ambient_color_energy; }

Color ReflectionProbe::get_ambient_color() const { return ambient_color; }

void ReflectionProbe::set_max_distance(float p_distance)
{
	max_distance = CLAMP(p_distance, 0.0, 262'144.0);
	// Reflection rendering breaks if distance exceeds 262,144 units (due to floating-point
	// precision with the near plane being 0.01).
	Renderer::reflection_probe_set_max_distance(probe, max_distance);
}

float ReflectionProbe::get_max_distance() const { return max_distance; }

void ReflectionProbe::set_mesh_lod_threshold(float p_pixels)
{
	mesh_lod_threshold = p_pixels;
	Renderer::reflection_probe_set_mesh_lod_threshold(probe, p_pixels);
}

float ReflectionProbe::get_mesh_lod_threshold() const { return mesh_lod_threshold; }

Vector3 ReflectionProbe::get_size() const { return size; }

Vector3 ReflectionProbe::get_origin_offset() const { return origin_offset; }

void ReflectionProbe::set_enable_box_projection(bool p_enable)
{
	box_projection = p_enable;
	Renderer::reflection_probe_set_enable_box_projection(probe, p_enable);
}

bool ReflectionProbe::is_box_projection_enabled() const { return box_projection; }

void ReflectionProbe::set_as_interior(bool p_enable)
{
	interior = p_enable;
	Renderer::reflection_probe_set_as_interior(probe, interior);
}

bool ReflectionProbe::is_set_as_interior() const { return interior; }

void ReflectionProbe::set_enable_shadows(bool p_enable)
{
	enable_shadows = p_enable;
	Renderer::reflection_probe_set_enable_shadows(probe, p_enable);
}

bool ReflectionProbe::are_shadows_enabled() const { return enable_shadows; }

void ReflectionProbe::set_cull_mask(uint32_t p_layers)
{
	cull_mask = p_layers;
	Renderer::reflection_probe_set_cull_mask(probe, p_layers);
}

uint32_t ReflectionProbe::get_cull_mask() const { return cull_mask; }

void ReflectionProbe::set_reflection_mask(uint32_t p_layers)
{
	reflection_mask = p_layers;
	Renderer::reflection_probe_set_reflection_mask(probe, p_layers);
}

uint32_t ReflectionProbe::get_reflection_mask() const { return reflection_mask; }

void ReflectionProbe::set_update_mode(UpdateMode p_mode)
{
	update_mode = p_mode;
	Renderer::reflection_probe_set_update_mode(
		probe, RSE::ReflectionProbeUpdateMode(p_mode));
}

ReflectionProbe::UpdateMode ReflectionProbe::get_update_mode() const { return update_mode; }

AABB ReflectionProbe::get_aabb() const
{
	AABB aabb;
	aabb.position = -size / 2;
	aabb.size = size;
	return aabb;
}

void ReflectionProbe::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void ReflectionProbe::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

ReflectionProbe::ReflectionProbe()
{
	probe = Renderer::reflection_probe_create();
	set_base(probe);
	set_disable_scale(true);
}

ReflectionProbe::~ReflectionProbe()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(probe);
}

// ==========================================================================
// Merged from scene/3d/voxel_gi.cpp
// ==========================================================================

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/3d/voxel_gi.h ---
#include "scene/3d/visual_instance_3d.h"

class CameraAttributes;
class Mesh;

class VoxelGIData : public Resource
{
	RID probe;

	Transform3D to_cell_xform;
	AABB bounds;
	Vector3 octree_size;

	float dynamic_range = 2.0;
	float energy = 1.0;
	float bias = 1.5;
	float normal_bias = 0.0;
	float propagation = 0.5;
	bool interior = false;
	bool use_two_bounces = true;

protected:

public:
	void allocate(const Transform3D& p_to_cell_xform, const AABB& p_aabb,
		const Vector3& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts);
	AABB get_bounds() const;
	Vector3 get_octree_size() const;
	Vector<uint8_t> get_octree_cells() const;
	Vector<uint8_t> get_data_cells() const;
	Vector<uint8_t> get_distance_field() const;
	Vector<int> get_level_counts() const;
	Transform3D get_to_cell_xform() const;

	void set_dynamic_range(float p_range);
	float get_dynamic_range() const;

	void set_propagation(float p_propagation);
	float get_propagation() const;

	void set_energy(float p_energy);
	float get_energy() const;

	void set_bias(float p_bias);
	float get_bias() const;

	void set_normal_bias(float p_normal_bias);
	float get_normal_bias() const;

	void set_interior(bool p_enable);
	bool is_interior() const;

	void set_use_two_bounces(bool p_enable);
	bool is_using_two_bounces() const;

	virtual RID get_rid() const override;

	VoxelGIData();
	~VoxelGIData();
};

class VoxelGI : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum Subdiv
	{
		SUBDIV_64,
		SUBDIV_128,
		SUBDIV_256,
		SUBDIV_512,
		SUBDIV_MAX

	};

	typedef void (*BakeBeginFunc)();
	typedef bool (*BakeStepFunc)(int, const String&);
	typedef void (*BakeEndFunc)();

private:
	Ref<VoxelGIData> probe_data;

	RID voxel_gi;

	Subdiv subdiv = SUBDIV_128;
	Vector3 size = Vector3(20, 20, 20);
	Ref<CameraAttributes> camera_attributes;

	struct PlotMesh
	{
		Ref<Material> override_material;
		Vector<Ref<Material>> instance_materials;
		Ref<Mesh> mesh;
		Transform3D local_xform;
	};

	void _find_meshes(Node* p_at_node, List<PlotMesh>& plot_meshes);
	void _debug_bake();

	float _get_camera_exposure_normalization();

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

	Ref<VoxelGIData> get_probe_data() const;

	Subdiv get_subdiv() const;

	Vector3 get_size() const;

	void set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes);
	Ref<CameraAttributes> get_camera_attributes() const;

	Vector3i get_estimated_cell_size() const;

	void bake(Node* p_from_node = nullptr, bool p_create_visual_debug = false);

	AABB get_aabb() const;

	PackedStringArray get_configuration_warnings() const override;

	VoxelGI();
	~VoxelGI();
};



// --- End scene/3d/voxel_gi.h ---

void VoxelGIData::allocate(const Transform3D& p_to_cell_xform, const AABB& p_aabb,
	const Vector3& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	Renderer::voxel_gi_allocate_data(probe, p_to_cell_xform, p_aabb, p_octree_size,
		p_octree_cells, p_data_cells, p_distance_field, p_level_counts);
	bounds = p_aabb;
	to_cell_xform = p_to_cell_xform;
	octree_size = p_octree_size;
}

AABB VoxelGIData::get_bounds() const { return bounds; }

Vector3 VoxelGIData::get_octree_size() const { return octree_size; }

Vector<uint8_t> VoxelGIData::get_octree_cells() const
{
	return Renderer::voxel_gi_get_octree_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_data_cells() const
{
	return Renderer::voxel_gi_get_data_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_distance_field() const
{
	return Renderer::voxel_gi_get_distance_field(probe);
}

Vector<int> VoxelGIData::get_level_counts() const
{
	return Renderer::voxel_gi_get_level_counts(probe);
}

Transform3D VoxelGIData::get_to_cell_xform() const { return to_cell_xform; }

void VoxelGIData::set_dynamic_range(float p_range)
{
	Renderer::voxel_gi_set_dynamic_range(probe, p_range);
	dynamic_range = p_range;
}

float VoxelGIData::get_dynamic_range() const { return dynamic_range; }

void VoxelGIData::set_propagation(float p_propagation)
{
	Renderer::voxel_gi_set_propagation(probe, p_propagation);
	propagation = p_propagation;
}

float VoxelGIData::get_propagation() const { return propagation; }

void VoxelGIData::set_energy(float p_energy)
{
	Renderer::voxel_gi_set_energy(probe, p_energy);
	energy = p_energy;
}

float VoxelGIData::get_energy() const { return energy; }

void VoxelGIData::set_bias(float p_bias)
{
	Renderer::voxel_gi_set_bias(probe, p_bias);
	bias = p_bias;
}

float VoxelGIData::get_bias() const { return bias; }

void VoxelGIData::set_normal_bias(float p_normal_bias)
{
	Renderer::voxel_gi_set_normal_bias(probe, p_normal_bias);
	normal_bias = p_normal_bias;
}

float VoxelGIData::get_normal_bias() const { return normal_bias; }

void VoxelGIData::set_interior(bool p_enable)
{
	Renderer::voxel_gi_set_interior(probe, p_enable);
	interior = p_enable;
}

bool VoxelGIData::is_interior() const { return interior; }

void VoxelGIData::set_use_two_bounces(bool p_enable)
{
	Renderer::voxel_gi_set_use_two_bounces(probe, p_enable);
	use_two_bounces = p_enable;
}

bool VoxelGIData::is_using_two_bounces() const { return use_two_bounces; }

RID VoxelGIData::get_rid() const { return probe; }

VoxelGIData::VoxelGIData() { probe = Renderer::voxel_gi_create(); }

VoxelGIData::~VoxelGIData()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(probe);
}

Ref<VoxelGIData> VoxelGI::get_probe_data() const { return probe_data; }

VoxelGI::Subdiv VoxelGI::get_subdiv() const { return subdiv; }

Vector3 VoxelGI::get_size() const { return size; }

void VoxelGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;

	if (probe_data.is_valid()) {
		Renderer::voxel_gi_set_baked_exposure_normalization(
			probe_data->get_rid(), _get_camera_exposure_normalization());
	}
}

Ref<CameraAttributes> VoxelGI::get_camera_attributes() const { return camera_attributes; }

VoxelGI::BakeBeginFunc VoxelGI::bake_begin_function = nullptr;
VoxelGI::BakeStepFunc VoxelGI::bake_step_function = nullptr;
VoxelGI::BakeEndFunc VoxelGI::bake_end_function = nullptr;

static int voxelizer_plot_bake_base = 0;
static int voxelizer_plot_bake_total = 0;

static bool voxelizer_plot_bake_step_function(int current, int)
{
	return VoxelGI::bake_step_function(
		(voxelizer_plot_bake_base + current) * 500 / voxelizer_plot_bake_total,
		RTR("Plotting Meshes"));
}

static bool voxelizer_sdf_bake_step_function(int current, int total)
{
	return VoxelGI::bake_step_function(
		500 + current * 500 / total, RTR("Generating Distance Field"));
}

Vector3i VoxelGI::get_estimated_cell_size() const
{
	static const int subdiv_value[SUBDIV_MAX] = {6, 7, 8, 9};
	int cell_subdiv = subdiv_value[subdiv];
	int axis_cell_size[3];
	AABB bounds = AABB(-size / 2, size);
	int longest_axis = bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		float axis_size = bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}
	}

	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

void VoxelGI::_debug_bake() { bake(nullptr, true); }

float VoxelGI::_get_camera_exposure_normalization()
{
	float exposure_normalization = 1.0;
	if (camera_attributes.is_valid()) {
		exposure_normalization = camera_attributes->get_exposure_multiplier();
		if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
			exposure_normalization = camera_attributes->calculate_exposure_normalization();
		}
	}
	return exposure_normalization;
}

AABB VoxelGI::get_aabb() const { return AABB(-size / 2, size); }

PackedStringArray VoxelGI::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (OS::get_current_rendering_method() == "gl_compatibility") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Compatibility "
							   "renderer yet. Support will be added in a future release."));
	}
	else if (OS::get_current_rendering_method() == "dummy") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Dummy renderer."));
	}
	else if (probe_data.is_null()) {
		warnings.push_back(RTR(
			"No VoxelGI data set, so this node is disabled. Bake static objects to enable GI."));
	}
	return warnings;
}

void VoxelGI::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void VoxelGI::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

VoxelGI::VoxelGI()
{
	voxel_gi = Renderer::voxel_gi_create();
	set_disable_scale(true);
}

VoxelGI::~VoxelGI()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(voxel_gi);
}

void VoxelGI::bake(Node*, bool) {}
#endif // !defined(_3D_DISABLED)


// --- Merged from scene/3d/navigation/navigation_3d.cpp ---
#if !defined(_3D_DISABLED) && !defined(NAVIGATION_3D_DISABLED)
#include "scene/3d/node_3d.h"

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "core/math/geometry_3d.h"
#include "core/math/random_pcg.h"
#include "scene/main/scene_tree.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/renderer.h"

void NavigationAgent3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		// need to use POST_ENTER_TREE cause with normal ENTER_TREE not all required Nodes are
		// ready. cannot use READY as ready does not get called if Node is re-added to SceneTree
		set_agent_parent(get_parent());
		set_physics_process_internal(true);

		if (agent_parent && avoidance_enabled) {
			NavigationServer3D::agent_set_position(
				agent, agent_parent->get_global_transform().origin);
		}

#ifdef DEBUG_ENABLED
		if (NavigationServer3D::get_debug_enabled()) {
			debug_path_dirty = true;
		}
#endif // DEBUG_ENABLED

	} break;

	case NOTIFICATION_PARENTED: {
		if (is_inside_tree() && (get_parent() != agent_parent)) {
			// only react to PARENTED notifications when already inside_tree and parent changed,
			// e.g. users switch nodes around PARENTED notification fires also when Node is added in
			// scripts to a parent this would spam transforms fails and world fails while Node is
			// outside SceneTree when node gets reparented when joining the tree POST_ENTER_TREE
			// takes care of this
			set_agent_parent(get_parent());
			set_physics_process_internal(true);
		}
	} break;

	case NOTIFICATION_UNPARENTED: {
		// if agent has no parent no point in processing it until reparented
		set_agent_parent(nullptr);
		set_physics_process_internal(false);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_agent_parent(nullptr);
		set_physics_process_internal(false);

#ifdef DEBUG_ENABLED
		if (debug_path_instance.is_valid()) {
			Renderer::instance_set_visible(debug_path_instance, false);
		}
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (agent_parent) {
			NavigationServer3D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (SceneTree::is_paused()) {
			break;
		}
		[[fallthrough]];
	}

	case NOTIFICATION_UNPAUSED: {
		if (agent_parent) {
			NavigationServer3D::agent_set_paused(
				get_rid(), !agent_parent->can_process());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (agent_parent && avoidance_enabled) {
			NavigationServer3D::agent_set_position(
				agent, agent_parent->get_global_position());
		}
		if (agent_parent && target_position_submitted) {
			if (velocity_submitted) {
				velocity_submitted = false;
				if (avoidance_enabled) {
					if (!use_3d_avoidance) {
						if (keep_y_velocity) {
							stored_y_velocity = velocity.y;
						}
						velocity.y = 0.0;
					}
					NavigationServer3D::agent_set_velocity(agent, velocity);
				}
			}
			if (velocity_forced_submitted) {
				velocity_forced_submitted = false;
				if (avoidance_enabled) {
					NavigationServer3D::agent_set_velocity_forced(
						agent, velocity_forced);
				}
			}
		}
#ifdef DEBUG_ENABLED
		if (debug_path_dirty) {
			_update_debug_path();
		}
#endif // DEBUG_ENABLED
	} break;
	}
}









bool NavigationAgent3D::get_avoidance_enabled() const { return avoidance_enabled; }



void NavigationAgent3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	if (target_position_submitted) {
		_request_repath();
	}
}

uint32_t NavigationAgent3D::get_navigation_layers() const { return navigation_layers; }

void NavigationAgent3D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");
	uint32_t _navigation_layers = get_navigation_layers();
	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}
	set_navigation_layers(_navigation_layers);
}

bool NavigationAgent3D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");
	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent3D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	if (pathfinding_algorithm == p_pathfinding_algorithm) {
		return;
	}

	pathfinding_algorithm = p_pathfinding_algorithm;

	navigation_query->set_pathfinding_algorithm(pathfinding_algorithm);
}

void NavigationAgent3D::set_path_postprocessing(
	const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing)
{
	if (path_postprocessing == p_path_postprocessing) {
		return;
	}

	path_postprocessing = p_path_postprocessing;

	navigation_query->set_path_postprocessing(path_postprocessing);
}

void NavigationAgent3D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
	navigation_query->set_simplify_path(simplify_path);
}

bool NavigationAgent3D::get_simplify_path() const { return simplify_path; }

void NavigationAgent3D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
	navigation_query->set_simplify_epsilon(simplify_epsilon);
}

real_t NavigationAgent3D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationAgent3D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
	navigation_query->set_path_return_max_length(path_return_max_length);
}

float NavigationAgent3D::get_path_return_max_length() const { return path_return_max_length; }

void NavigationAgent3D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
	navigation_query->set_path_return_max_radius(path_return_max_radius);
}

float NavigationAgent3D::get_path_return_max_radius() const { return path_return_max_radius; }

void NavigationAgent3D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
	navigation_query->set_path_search_max_polygons(path_search_max_polygons);
}

int NavigationAgent3D::get_path_search_max_polygons() const { return path_search_max_polygons; }

void NavigationAgent3D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
	navigation_query->set_path_search_max_distance(path_search_max_distance);
}

float NavigationAgent3D::get_path_search_max_distance() const { return path_search_max_distance; }

float NavigationAgent3D::get_path_length() const { return navigation_result->get_path_length(); }

void NavigationAgent3D::set_path_metadata_flags(
	uint32_t p_path_metadata_flags)
{
	if (path_metadata_flags == p_path_metadata_flags) {
		return;
	}

	path_metadata_flags = p_path_metadata_flags;
}

void NavigationAgent3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::agent_set_map(agent, map_override);
	if (target_position_submitted) {
		_request_repath();
	}
}

RID NavigationAgent3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (agent_parent != nullptr) {
		return agent_parent->get_world_3d()->get_navigation_map();
	}
	return RID();
}

void NavigationAgent3D::set_path_desired_distance(real_t p_path_desired_distance)
{
	if (Math::is_equal_approx(path_desired_distance, p_path_desired_distance)) {
		return;
	}

	path_desired_distance = p_path_desired_distance;
}

void NavigationAgent3D::set_target_desired_distance(real_t p_target_desired_distance)
{
	if (Math::is_equal_approx(target_desired_distance, p_target_desired_distance)) {
		return;
	}

	target_desired_distance = p_target_desired_distance;
}

void NavigationAgent3D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}
	radius = p_radius;

	NavigationServer3D::agent_set_radius(agent, radius);
}

void NavigationAgent3D::set_height(real_t p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0, "Height must be positive.");
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}
	height = p_height;
	NavigationServer3D::agent_set_height(agent, height);
}

void NavigationAgent3D::set_path_height_offset(real_t p_path_height_offset)
{
	path_height_offset = p_path_height_offset;
}



void NavigationAgent3D::set_keep_y_velocity(bool p_enabled)
{
	keep_y_velocity = p_enabled;
	stored_y_velocity = 0.0;
}

bool NavigationAgent3D::get_keep_y_velocity() const { return keep_y_velocity; }

void NavigationAgent3D::set_neighbor_distance(real_t p_distance)
{
	if (Math::is_equal_approx(neighbor_distance, p_distance)) {
		return;
	}

	neighbor_distance = p_distance;

	NavigationServer3D::agent_set_neighbor_distance(agent, neighbor_distance);
}

void NavigationAgent3D::set_max_neighbors(int p_count)
{
	if (max_neighbors == p_count) {
		return;
	}

	max_neighbors = p_count;

	NavigationServer3D::agent_set_max_neighbors(agent, max_neighbors);
}

void NavigationAgent3D::set_time_horizon_agents(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_agents, p_time_horizon)) {
		return;
	}
	time_horizon_agents = p_time_horizon;
	NavigationServer3D::agent_set_time_horizon_agents(agent, time_horizon_agents);
}

void NavigationAgent3D::set_time_horizon_obstacles(real_t p_time_horizon)
{
	ERR_FAIL_COND_MSG(p_time_horizon < 0.0, "Time horizon must be positive.");
	if (Math::is_equal_approx(time_horizon_obstacles, p_time_horizon)) {
		return;
	}
	time_horizon_obstacles = p_time_horizon;
	NavigationServer3D::agent_set_time_horizon_obstacles(
		agent, time_horizon_obstacles);
}

void NavigationAgent3D::set_max_speed(real_t p_max_speed)
{
	ERR_FAIL_COND_MSG(p_max_speed < 0.0, "Max speed must be positive.");
	if (Math::is_equal_approx(max_speed, p_max_speed)) {
		return;
	}
	max_speed = p_max_speed;

	NavigationServer3D::agent_set_max_speed(agent, max_speed);
}

void NavigationAgent3D::set_path_max_distance(real_t p_path_max_distance)
{
	if (Math::is_equal_approx(path_max_distance, p_path_max_distance)) {
		return;
	}

	path_max_distance = p_path_max_distance;
}

real_t NavigationAgent3D::get_path_max_distance() { return path_max_distance; }

void NavigationAgent3D::set_target_position(Vector3 p_position)
{
	// Intentionally not checking for equality of the parameter, as we want to update the path even
	// if the target position is the same in case the world changed. Revisit later when the
	// navigation server can update the path without requesting a new path.

	target_position = p_position;
	target_position_submitted = true;

	_request_repath();
}

Vector3 NavigationAgent3D::get_target_position() const { return target_position; }

Vector3 NavigationAgent3D::get_next_path_position()
{
	_update_navigation();

	const Vector<Vector3>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		ERR_FAIL_NULL_V_MSG(agent_parent, Vector3(), "The agent has no parent.");
		return agent_parent->get_global_position();
	}
	else {
		return navigation_path[navigation_path_index] - Vector3(0, path_height_offset, 0);
	}
}

real_t NavigationAgent3D::distance_to_target() const
{
	ERR_FAIL_NULL_V_MSG(agent_parent, 0.0, "The agent has no parent.");
	return agent_parent->get_global_position().distance_to(target_position);
}

bool NavigationAgent3D::is_target_reached() const { return target_reached; }

bool NavigationAgent3D::is_target_reachable()
{
	_update_navigation();
	return _is_target_reachable();
}

bool NavigationAgent3D::_is_target_reachable() const
{
	return target_desired_distance >= _get_final_position().distance_to(target_position);
}

bool NavigationAgent3D::is_navigation_finished()
{
	_update_navigation();
	return navigation_finished;
}

Vector3 NavigationAgent3D::get_final_position()
{
	_update_navigation();
	return _get_final_position();
}

Vector3 NavigationAgent3D::_get_final_position() const
{
	const Vector<Vector3>& navigation_path = navigation_result->get_path();
	if (navigation_path.is_empty()) {
		return Vector3();
	}
	return navigation_path[navigation_path.size() - 1] - Vector3(0, path_height_offset, 0);
}

void NavigationAgent3D::set_velocity_forced(Vector3 p_velocity)
{
	// Intentionally not checking for equality of the parameter.
	// We need to always submit the velocity to the navigation server, even when it is the same, in
	// order to run avoidance every frame. Revisit later when the navigation server can update
	// avoidance without users resubmitting the velocity.

	velocity_forced = p_velocity;
	velocity_forced_submitted = true;
}

void NavigationAgent3D::set_velocity(const Vector3 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}







void NavigationAgent3D::_advance_waypoints(const Vector3& p_origin)
{
	if (last_waypoint_reached) {
		return;
	}

	// Advance to the farthest possible waypoint.
	while (_is_within_waypoint_distance(p_origin)) {
		_trigger_waypoint_reached();

		if (_is_last_waypoint()) {
			last_waypoint_reached = true;
			break;
		}

		_move_to_next_waypoint();
	}
}

void NavigationAgent3D::_request_repath()
{
	navigation_result->reset();
	target_reached = false;
	navigation_finished = false;
	last_waypoint_reached = false;
}

bool NavigationAgent3D::_is_last_waypoint() const
{
	return navigation_path_index == navigation_result->get_path().size() - 1;
}

void NavigationAgent3D::_move_to_next_waypoint() { navigation_path_index += 1; }

bool NavigationAgent3D::_is_within_waypoint_distance(const Vector3& p_origin) const
{
	const Vector<Vector3>& navigation_path = navigation_result->get_path();
	Vector3 waypoint = navigation_path[navigation_path_index] - Vector3(0, path_height_offset, 0);
	return p_origin.distance_to(waypoint) < path_desired_distance;
}

bool NavigationAgent3D::_is_within_target_distance(const Vector3& p_origin) const
{
	return p_origin.distance_to(target_position) < target_desired_distance;
}







void NavigationAgent3D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	NavigationServer3D::agent_set_avoidance_layers(get_rid(), avoidance_layers);
}

uint32_t NavigationAgent3D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationAgent3D::set_avoidance_mask(uint32_t p_mask)
{
	avoidance_mask = p_mask;
	NavigationServer3D::agent_set_avoidance_mask(get_rid(), avoidance_mask);
}

uint32_t NavigationAgent3D::get_avoidance_mask() const { return avoidance_mask; }

void NavigationAgent3D::set_avoidance_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Avoidance layer number must be between 1 and 32 inclusive.");
	uint32_t avoidance_layers_new = get_avoidance_layers();
	if (p_value) {
		avoidance_layers_new |= 1 << (p_layer_number - 1);
	}
	else {
		avoidance_layers_new &= ~(1 << (p_layer_number - 1));
	}
	set_avoidance_layers(avoidance_layers_new);
}

bool NavigationAgent3D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent3D::set_avoidance_mask_value(int p_mask_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_mask_number < 1, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_mask_number > 32, "Avoidance mask number must be between 1 and 32 inclusive.");
	uint32_t mask = get_avoidance_mask();
	if (p_value) {
		mask |= 1 << (p_mask_number - 1);
	}
	else {
		mask &= ~(1 << (p_mask_number - 1));
	}
	set_avoidance_mask(mask);
}

bool NavigationAgent3D::get_avoidance_mask_value(int p_mask_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_mask_number < 1, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_mask_number > 32, false, "Avoidance mask number must be between 1 and 32 inclusive.");
	return get_avoidance_mask() & (1 << (p_mask_number - 1));
}

void NavigationAgent3D::set_avoidance_priority(real_t p_priority)
{
	ERR_FAIL_COND_MSG(
		p_priority < 0.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	ERR_FAIL_COND_MSG(
		p_priority > 1.0, "Avoidance priority must be between 0.0 and 1.0 inclusive.");
	avoidance_priority = p_priority;
	NavigationServer3D::agent_set_avoidance_priority(get_rid(), p_priority);
}

real_t NavigationAgent3D::get_avoidance_priority() const { return avoidance_priority; }

////////DEBUG////////////////////////////////////////////////////////////

void NavigationAgent3D::set_debug_enabled(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_enabled == p_enabled) {
		return;
	}

	debug_enabled = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent3D::get_debug_enabled() const { return debug_enabled; }

void NavigationAgent3D::set_debug_use_custom(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_use_custom == p_enabled) {
		return;
	}

	debug_use_custom = p_enabled;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

bool NavigationAgent3D::get_debug_use_custom() const { return debug_use_custom; }

void NavigationAgent3D::set_debug_path_custom_color(Color p_color)
{
#ifdef DEBUG_ENABLED
	if (debug_path_custom_color == p_color) {
		return;
	}

	debug_path_custom_color = p_color;
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

Color NavigationAgent3D::get_debug_path_custom_color() const { return debug_path_custom_color; }

void NavigationAgent3D::set_debug_path_custom_point_size(float p_point_size)
{
#ifdef DEBUG_ENABLED
	if (Math::is_equal_approx(debug_path_custom_point_size, p_point_size)) {
		return;
	}

	debug_path_custom_point_size = MAX(0.0, p_point_size);
	debug_path_dirty = true;
#endif // DEBUG_ENABLED
}

float NavigationAgent3D::get_debug_path_custom_point_size() const
{
	return debug_path_custom_point_size;
}

#ifdef DEBUG_ENABLED
void NavigationAgent3D::_navigation_debug_changed() { debug_path_dirty = true; }
#endif // DEBUG_ENABLED



void NavigationAgent3D::_update_navigation() {}

void NavigationAgent3D::set_agent_parent(Node*) {}

#ifdef DEBUG_ENABLED
void NavigationAgent3D::_update_debug_path() {}
#endif // DEBUG_ENABLED

void NavigationAgent3D::_trigger_waypoint_reached() {}


void NavigationLink3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_link_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_link_update_transform();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_link_exit_navigation_map();
	} break;

#ifdef DEBUG_ENABLED
	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_debug_mesh();
	} break;
#endif // DEBUG_ENABLED
	}
}

NavigationLink3D::~NavigationLink3D()
{
	ERR_FAIL_NULL(NavigationServer3D::data);
	NavigationServer3D::free_rid(link);
	link = RID();

#ifdef DEBUG_ENABLED
	ERR_FAIL_NULL(Renderer::data);
	if (debug_instance.is_valid()) {
		Renderer::free_rid(debug_instance);
	}
	if (debug_mesh.is_valid()) {
		Renderer::free_rid(debug_mesh->get_rid());
	}
#endif // DEBUG_ENABLED
}

RID NavigationLink3D::get_rid() const { return link; }

void NavigationLink3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::link_set_map(link, map_override);
}

RID NavigationLink3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}

void NavigationLink3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer3D::link_set_navigation_layers(link, navigation_layers);
}

void NavigationLink3D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layers();

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layers(_navigation_layers);
}

bool NavigationLink3D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

Vector3 NavigationLink3D::get_global_start_position() const
{
	if (is_inside_tree()) {
		return to_global(start_position);
	}
	else {
		return start_position;
	}
}

Vector3 NavigationLink3D::get_global_end_position() const
{
	if (is_inside_tree()) {
		return to_global(end_position);
	}
	else {
		return end_position;
	}
}

void NavigationLink3D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer3D::link_set_enter_cost(link, enter_cost);
}

void NavigationLink3D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer3D::link_set_travel_cost(link, travel_cost);
}

PackedStringArray NavigationLink3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (start_position.is_equal_approx(end_position)) {
		warnings.push_back(RTR("NavigationLink3D start position should be different than the end "
							   "position to be useful."));
	}

	return warnings;
}

void NavigationLink3D::_link_enter_navigation_map()
{
	if (!is_inside_tree()) {
		return;
	}

	if (map_override.is_valid()) {
		NavigationServer3D::link_set_map(link, map_override);
	}
	else {
		NavigationServer3D::link_set_map(
			link, get_world_3d()->get_navigation_map());
	}

	NavigationServer3D::link_set_start_position(
		link, get_global_transform().xform(start_position));
	NavigationServer3D::link_set_end_position(
		link, get_global_transform().xform(end_position));
	NavigationServer3D::link_set_enabled(link, enabled);

#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}

void NavigationLink3D::_link_exit_navigation_map()
{
	NavigationServer3D::link_set_map(link, RID());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		Renderer::instance_set_visible(debug_instance, false);
	}
#endif // DEBUG_ENABLED
}

void NavigationLink3D::_link_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::link_set_start_position(
		link, get_global_transform().xform(start_position));
	NavigationServer3D::link_set_end_position(
		link, get_global_transform().xform(end_position));
#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}



#ifdef DEBUG_ENABLED
void NavigationLink3D::_update_debug_mesh() {}
#endif // DEBUG_ENABLED


RID NavigationObstacle3D::_navmesh_source_geometry_parser;

void NavigationObstacle3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_POST_ENTER_TREE: {
		if (map_override.is_valid()) {
			_update_map(map_override);
		}
		else if (is_inside_tree()) {
			_update_map(get_world_3d()->get_navigation_map());
		}
		else {
			_update_map(RID());
		}
		// need to trigger map controlled agent assignment somehow for the fake_agent since
		// obstacles use no callback like regular agents
		NavigationServer3D::obstacle_set_avoidance_enabled(
			obstacle, avoidance_enabled);
		_update_transform();
		set_physics_process_internal(true);
#ifdef DEBUG_ENABLED
		_update_debug();
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_EXIT_TREE: {
		set_physics_process_internal(false);
		_update_map(RID());
#ifdef DEBUG_ENABLED
		_clear_debug();
#endif // DEBUG_ENABLED
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (!can_process()) {
			map_before_pause = map_current;
			_update_map(RID());
		}
		else if (can_process() && !(map_before_pause == RID())) {
			_update_map(map_before_pause);
			map_before_pause = RID();
		}
		NavigationServer3D::obstacle_set_paused(obstacle, !can_process());
	} break;

	case NOTIFICATION_UNSUSPENDED: {
		if (SceneTree::is_paused()) {
			break;
		}
		[[fallthrough]];
	}

	case NOTIFICATION_UNPAUSED: {
		if (!can_process()) {
			map_before_pause = map_current;
			_update_map(RID());
		}
		else if (can_process() && !(map_before_pause == RID())) {
			_update_map(map_before_pause);
			map_before_pause = RID();
		}
		NavigationServer3D::obstacle_set_paused(obstacle, !can_process());
	} break;

#ifdef DEBUG_ENABLED
	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_debug();
	} break;
#endif // DEBUG_ENABLED

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (is_inside_tree()) {
			_update_transform();

			if (velocity_submitted) {
				velocity_submitted = false;
				// only update if there is a noticeable change, else the rvo agent preferred
				// velocity stays the same
				if (!previous_velocity.is_equal_approx(velocity)) {
					NavigationServer3D::obstacle_set_velocity(obstacle, velocity);
				}
				previous_velocity = velocity;
			}
#ifdef DEBUG_ENABLED
			if (fake_agent_radius_debug_instance_rid.is_valid() && radius > 0.0) {
				// Prevent non-positive scaling.
				const Vector3 safe_scale = get_global_basis().get_scale().abs().maxf(0.001);
				// Agent radius is a scalar value and does not support non-uniform scaling, choose
				// the largest axis.
				const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
				const Vector3 uniform_max_scale =
					Vector3(scaling_max_value, scaling_max_value, scaling_max_value);
				const Transform3D debug_transform =
					Transform3D(Basis().scaled(uniform_max_scale), get_global_position());

				Renderer::instance_set_transform(
					fake_agent_radius_debug_instance_rid, debug_transform);
			}
			if (static_obstacle_debug_instance_rid.is_valid() && get_vertices().size() > 0) {
				// Prevent non-positive scaling.
				const Vector3 safe_scale = get_global_basis().get_scale().abs().maxf(0.001);
				// Obstacles are projected to the xz-plane, so only rotation around the y-axis can
				// be taken into account.
				const Transform3D debug_transform =
					Transform3D(Basis()
									.scaled(safe_scale)
									.rotated(Vector3(0.0, 1.0, 0.0), get_global_rotation().y),
						get_global_position());

				Renderer::instance_set_transform(
					static_obstacle_debug_instance_rid, debug_transform);
			}
#endif // DEBUG_ENABLED
		}
	} break;
	}
}

void NavigationObstacle3D::set_vertices(const Vector<Vector3>& p_vertices)
{
	vertices = p_vertices;

	Vector<Vector2> vertices_2d;
	vertices_2d.resize(vertices.size());

	const Vector3* vertices_ptr = vertices.ptr();
	Vector2* vertices_2d_ptrw = vertices_2d.ptrw();

	for (int i = 0; i < vertices.size(); i++) {
		vertices_2d_ptrw[i] = Vector2(vertices_ptr[i].x, vertices_ptr[i].z);
	}

	vertices_are_clockwise =
		!Geometry2D::is_polygon_clockwise(vertices_2d); // Geometry2D is inverted.
	vertices_are_valid = !Geometry2D::triangulate_polygon(vertices_2d).is_empty();

	const Basis basis = is_inside_tree() ? get_global_basis() : get_basis();
	const float rotation_y = is_inside_tree() ? get_global_rotation().y : get_rotation().y;
	const Vector3 safe_scale = basis.get_scale().abs().maxf(0.001);
	const Transform3D safe_transform = Transform3D(
		Basis().scaled(safe_scale).rotated(Vector3(0.0, 1.0, 0.0), rotation_y), Vector3());
	NavigationServer3D::obstacle_set_vertices(
		obstacle, safe_transform.xform(vertices));
}

void NavigationObstacle3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}
	map_override = p_navigation_map;
	_update_map(map_override);
}

RID NavigationObstacle3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}

void NavigationObstacle3D::set_radius(real_t p_radius)
{
	ERR_FAIL_COND_MSG(p_radius < 0.0, "Radius must be positive.");
	if (Math::is_equal_approx(radius, p_radius)) {
		return;
	}

	radius = p_radius;

	// Prevent non-positive or non-uniform scaling of dynamic obstacle radius.
	const Vector3 safe_scale =
		(is_inside_tree() ? get_global_basis() : get_basis()).get_scale().abs().maxf(0.001);
	NavigationServer3D::obstacle_set_radius(
		obstacle, safe_scale[safe_scale.max_axis_index()] * radius);
}

void NavigationObstacle3D::set_height(real_t p_height)
{
	ERR_FAIL_COND_MSG(p_height < 0.0, "Height must be positive.");
	if (Math::is_equal_approx(height, p_height)) {
		return;
	}

	height = p_height;
	const float scale_factor =
		MAX(Math::abs((is_inside_tree() ? get_global_basis() : get_basis()).get_scale().y), 0.001);
	NavigationServer3D::obstacle_set_height(obstacle, scale_factor * height);
}

void NavigationObstacle3D::set_avoidance_layers(uint32_t p_layers)
{
	avoidance_layers = p_layers;
	NavigationServer3D::obstacle_set_avoidance_layers(obstacle, avoidance_layers);
}

uint32_t NavigationObstacle3D::get_avoidance_layers() const { return avoidance_layers; }

void NavigationObstacle3D::set_avoidance_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Avoidance layer number must be between 1 and 32 inclusive.");
	uint32_t avoidance_layers_new = get_avoidance_layers();
	if (p_value) {
		avoidance_layers_new |= 1 << (p_layer_number - 1);
	}
	else {
		avoidance_layers_new &= ~(1 << (p_layer_number - 1));
	}
	set_avoidance_layers(avoidance_layers_new);
}

bool NavigationObstacle3D::get_avoidance_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Avoidance layer number must be between 1 and 32 inclusive.");
	return get_avoidance_layers() & (1 << (p_layer_number - 1));
}

void NavigationObstacle3D::set_avoidance_enabled(bool p_enabled)
{
	if (avoidance_enabled == p_enabled) {
		return;
	}

	avoidance_enabled = p_enabled;
	NavigationServer3D::obstacle_set_avoidance_enabled(
		obstacle, avoidance_enabled);
}

bool NavigationObstacle3D::get_avoidance_enabled() const { return avoidance_enabled; }

void NavigationObstacle3D::set_velocity(const Vector3 p_velocity)
{
	velocity = p_velocity;
	velocity_submitted = true;
}

void NavigationObstacle3D::set_affect_navigation_mesh(bool p_enabled)
{
	affect_navigation_mesh = p_enabled;
}

bool NavigationObstacle3D::get_affect_navigation_mesh() const { return affect_navigation_mesh; }

void NavigationObstacle3D::set_carve_navigation_mesh(bool p_enabled)
{
	carve_navigation_mesh = p_enabled;
}

bool NavigationObstacle3D::get_carve_navigation_mesh() const { return carve_navigation_mesh; }

PackedStringArray NavigationObstacle3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (get_global_rotation().x != 0.0 || get_global_rotation().z != 0.0) {
		warnings.push_back(
			RTR("NavigationObstacle3D only takes global rotation around the y-axis into account. "
				"Rotations around the x-axis or z-axis might lead to unexpected results."));
	}

	const Vector3 global_scale = get_global_basis().get_scale();
	if (global_scale.x < 0.001 || global_scale.y < 0.001 || global_scale.z < 0.001) {
		warnings.push_back(RTR("NavigationObstacle3D does not support negative or zero scaling."));
	}

	if (radius > 0.0 && !get_global_basis().is_conformal()) {
		warnings.push_back(RTR("The agent radius can only be scaled uniformly. The largest scale "
							   "value along the three axes will be used."));
	}

	return warnings;
}

void NavigationObstacle3D::_update_map(RID p_map)
{
	NavigationServer3D::obstacle_set_map(obstacle, p_map);
	map_current = p_map;
}

void NavigationObstacle3D::_update_position(const Vector3 p_position)
{
	NavigationServer3D::obstacle_set_position(obstacle, p_position);
}

void NavigationObstacle3D::_update_transform()
{
	_update_position(get_global_position());

	// Prevent non-positive or non-uniform scaling of dynamic obstacle radius.
	const Vector3 safe_scale = get_global_basis().get_scale().abs().maxf(0.001);
	const float scaling_max_value = safe_scale[safe_scale.max_axis_index()];
	NavigationServer3D::obstacle_set_radius(obstacle, scaling_max_value * radius);

	// Apply modified node transform which only takes y-axis rotation into account to vertices.
	const Transform3D safe_transform = Transform3D(
		Basis().scaled(safe_scale).rotated(Vector3(0.0, 1.0, 0.0), get_global_rotation().y),
		Vector3());
	NavigationServer3D::obstacle_set_vertices(
		obstacle, safe_transform.xform(vertices));
	NavigationServer3D::obstacle_set_height(obstacle, safe_scale.y * height);
}

void NavigationObstacle3D::_update_use_3d_avoidance(bool p_use_3d_avoidance)
{
	NavigationServer3D::obstacle_set_use_3d_avoidance(obstacle, use_3d_avoidance);
	_update_map(map_current);
}

#ifdef DEBUG_ENABLED
void NavigationObstacle3D::_update_debug()
{
	if (is_inside_tree()) {
		Renderer::instance_set_visible(fake_agent_radius_debug_instance_rid, is_visible_in_tree());
		Renderer::instance_set_visible(static_obstacle_debug_instance_rid, is_visible_in_tree());
		Renderer::instance_set_scenario(
			fake_agent_radius_debug_instance_rid, get_world_3d()->get_scenario());
		Renderer::instance_set_scenario(
			static_obstacle_debug_instance_rid, get_world_3d()->get_scenario());
		Renderer::instance_set_transform(
			fake_agent_radius_debug_instance_rid, Transform3D(Basis(), get_global_position()));
		Renderer::instance_set_transform(
			static_obstacle_debug_instance_rid, Transform3D(Basis(), get_global_position()));
		_update_fake_agent_radius_debug();
		_update_static_obstacle_debug();
	}
	else {
		Renderer::mesh_clear(fake_agent_radius_debug_mesh_rid);
		Renderer::mesh_clear(static_obstacle_debug_mesh_rid);
		Renderer::instance_set_scenario(fake_agent_radius_debug_instance_rid, RID());
		Renderer::instance_set_scenario(static_obstacle_debug_instance_rid, RID());
	}
}

#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationObstacle3D::_clear_debug()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::mesh_clear(fake_agent_radius_debug_mesh_rid);
	Renderer::mesh_clear(static_obstacle_debug_mesh_rid);
	Renderer::instance_set_scenario(fake_agent_radius_debug_instance_rid, RID());
	Renderer::instance_set_scenario(static_obstacle_debug_instance_rid, RID());
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationObstacle3D::_update_static_obstacle_debug() {}

void NavigationObstacle3D::_update_fake_agent_radius_debug() {}
#endif // DEBUG_ENABLED


RID NavigationRegion3D::get_rid() const { return region; }

bool NavigationRegion3D::is_enabled() const { return enabled; }

void NavigationRegion3D::set_use_edge_connections(bool p_enabled)
{
	if (use_edge_connections == p_enabled) {
		return;
	}

	use_edge_connections = p_enabled;

	NavigationServer3D::region_set_use_edge_connections(
		region, use_edge_connections);
}

bool NavigationRegion3D::get_use_edge_connections() const { return use_edge_connections; }

void NavigationRegion3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	if (navigation_layers == p_navigation_layers) {
		return;
	}

	navigation_layers = p_navigation_layers;

	NavigationServer3D::region_set_navigation_layers(region, navigation_layers);
}

uint32_t NavigationRegion3D::get_navigation_layers() const { return navigation_layers; }

void NavigationRegion3D::set_navigation_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layers();

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layers(_navigation_layers);
}

bool NavigationRegion3D::get_navigation_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationRegion3D::set_enter_cost(real_t p_enter_cost)
{
	ERR_FAIL_COND_MSG(p_enter_cost < 0.0, "The enter_cost must be positive.");
	if (Math::is_equal_approx(enter_cost, p_enter_cost)) {
		return;
	}

	enter_cost = p_enter_cost;

	NavigationServer3D::region_set_enter_cost(region, enter_cost);
}

real_t NavigationRegion3D::get_enter_cost() const { return enter_cost; }

void NavigationRegion3D::set_travel_cost(real_t p_travel_cost)
{
	ERR_FAIL_COND_MSG(p_travel_cost < 0.0, "The travel_cost must be positive.");
	if (Math::is_equal_approx(travel_cost, p_travel_cost)) {
		return;
	}

	travel_cost = p_travel_cost;

	NavigationServer3D::region_set_travel_cost(region, travel_cost);
}

real_t NavigationRegion3D::get_travel_cost() const { return travel_cost; }

RID NavigationRegion3D::get_region_rid() const { return get_rid(); }

void NavigationRegion3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_region_enter_navigation_map();
	} break;

	case NOTIFICATION_TRANSFORM_CHANGED: {
		_region_update_transform();
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_region_exit_navigation_map();
	} break;
	}
}

Ref<NavigationMesh> NavigationRegion3D::get_navigation_mesh() const { return navigation_mesh; }

void NavigationRegion3D::set_navigation_map(RID p_navigation_map)
{
	if (map_override == p_navigation_map) {
		return;
	}

	map_override = p_navigation_map;

	NavigationServer3D::region_set_map(region, map_override);
}

RID NavigationRegion3D::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}

PackedStringArray NavigationRegion3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (is_visible_in_tree() && is_inside_tree()) {
		if (navigation_mesh.is_null()) {
			warnings.push_back(
				RTR("A NavigationMesh resource must be set or created for this node to work."));
		}
	}

	return warnings;
}

#ifdef DEBUG_ENABLED
void NavigationRegion3D::_navigation_map_changed(RID p_map)
{
	if (is_inside_tree() && p_map == get_world_3d()->get_navigation_map()) {
		_update_debug_edge_connections_mesh();
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationRegion3D::_navigation_debug_changed()
{
	if (is_inside_tree()) {
		_update_debug_mesh();
		_update_debug_edge_connections_mesh();
	}
}
#endif // DEBUG_ENABLED

void NavigationRegion3D::_region_enter_navigation_map()
{
	if (!is_inside_tree()) {
		return;
	}

	if (map_override.is_valid()) {
		NavigationServer3D::region_set_map(region, map_override);
	}
	else {
		NavigationServer3D::region_set_map(
			region, get_world_3d()->get_navigation_map());
	}

	NavigationServer3D::region_set_transform(region, get_global_transform());
	NavigationServer3D::region_set_enabled(region, enabled);

#ifdef DEBUG_ENABLED
	if (NavigationServer3D::get_debug_navigation_enabled()) {
		_update_debug_mesh();
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_region_exit_navigation_map()
{
	NavigationServer3D::region_set_map(region, RID());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		Renderer::instance_set_visible(debug_instance, false);
	}
	if (debug_edge_connections_instance.is_valid()) {
		Renderer::instance_set_visible(debug_edge_connections_instance, false);
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_region_update_transform()
{
	if (!is_inside_tree()) {
		return;
	}

	NavigationServer3D::region_set_transform(region, get_global_transform());
#ifdef DEBUG_ENABLED
	if (debug_instance.is_valid()) {
		Renderer::instance_set_transform(debug_instance, get_global_transform());
	}
#endif // DEBUG_ENABLED
}

void NavigationRegion3D::_update_bounds()
{
	if (navigation_mesh.is_null()) {
		bounds = AABB();
		return;
	}

	const Vector<Vector3>& vertices = navigation_mesh->get_vertices();
	if (vertices.is_empty()) {
		bounds = AABB();
		return;
	}

	const Transform3D gt = is_inside_tree() ? get_global_transform() : get_transform();

	AABB new_bounds;
	new_bounds.position = gt.xform(vertices[0]);

	for (const Vector3& vertex : vertices) {
		new_bounds.expand_to(gt.xform(vertex));
	}
	bounds = new_bounds;
}



#ifdef DEBUG_ENABLED
void NavigationRegion3D::_update_debug_mesh() {}

void NavigationRegion3D::_update_debug_edge_connections_mesh() {}
#endif // DEBUG_ENABLED

// ============================================================================
// Merged from scene/resources/3d/navigation_mesh_source_geometry_data_3d.cpp
// ============================================================================

#include "core/config/engine.h"
#include "servers/navigation_3d/navigation_server_3d.h"

void NavigationMeshSourceGeometryData3D::set_vertices(const Vector<float>& p_vertices)
{
	RWLockWrite write_lock(geometry_rwlock);
	vertices = p_vertices;
	bounds_dirty = true;
}

const Vector<float>& NavigationMeshSourceGeometryData3D::get_vertices() const
{
	RWLockRead read_lock(geometry_rwlock);
	return vertices;
}

void NavigationMeshSourceGeometryData3D::set_indices(const Vector<int>& p_indices)
{
	ERR_FAIL_COND(vertices.size() < p_indices.size());
	RWLockWrite write_lock(geometry_rwlock);
	indices = p_indices;
	bounds_dirty = true;
}

const Vector<int>& NavigationMeshSourceGeometryData3D::get_indices() const
{
	RWLockRead read_lock(geometry_rwlock);
	return indices;
}

void NavigationMeshSourceGeometryData3D::append_arrays(
	const Vector<float>& p_vertices, const Vector<int>& p_indices)
{
	RWLockWrite write_lock(geometry_rwlock);

	const int64_t number_of_vertices_before_merge = vertices.size();
	const int64_t number_of_indices_before_merge = indices.size();

	vertices.append_array(p_vertices);
	indices.append_array(p_indices);

	for (int64_t i = number_of_indices_before_merge; i < indices.size(); i++) {
		indices.set(i, indices[i] + number_of_vertices_before_merge / 3);
	}
	bounds_dirty = true;
}

bool NavigationMeshSourceGeometryData3D::has_data()
{
	RWLockRead read_lock(geometry_rwlock);
	return vertices.size() && indices.size();
}

void NavigationMeshSourceGeometryData3D::clear()
{
	RWLockWrite write_lock(geometry_rwlock);
	vertices.clear();
	indices.clear();
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::clear_projected_obstructions()
{
	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.clear();
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::_add_vertex(const Vector3& p_vec3)
{
	vertices.push_back(p_vec3.x);
	vertices.push_back(p_vec3.y);
	vertices.push_back(p_vec3.z);
}

void NavigationMeshSourceGeometryData3D::_add_faces(
	const PackedVector3Array& p_faces, const Transform3D& p_xform)
{
	ERR_FAIL_COND(p_faces.is_empty());
	ERR_FAIL_COND(p_faces.size() % 3 != 0);
	int face_count = p_faces.size() / 3;
	int current_vertex_count = vertices.size() / 3;

	for (int j = 0; j < face_count; j++) {
		_add_vertex(p_xform.xform(p_faces[j * 3 + 0]));
		_add_vertex(p_xform.xform(p_faces[j * 3 + 1]));
		_add_vertex(p_xform.xform(p_faces[j * 3 + 2]));

		indices.push_back(current_vertex_count + (j * 3 + 0));
		indices.push_back(current_vertex_count + (j * 3 + 2));
		indices.push_back(current_vertex_count + (j * 3 + 1));
	}
}

void NavigationMeshSourceGeometryData3D::add_mesh(
	const Ref<Mesh>& p_mesh, const Transform3D& p_xform)
{
	ERR_FAIL_COND(p_mesh.is_null());

#ifdef DEBUG_ENABLED
	if (!Engine::is_editor_hint()) {
		WARN_PRINT_ONCE(
			"Source geometry parsing for navigation mesh baking had to parse Renderer meshes at runtime.\n\
		This poses a significant performance issues as visual meshes store geometry data on the GPU and transferring this data back to the CPU blocks the rendering.\n\
		For runtime (re)baking navigation meshes use and parse collision shapes as source geometry or create geometry data procedurally in scripts.");
	}
#endif

	_add_mesh(p_mesh, root_node_transform * p_xform);
}

void NavigationMeshSourceGeometryData3D::add_faces(
	const PackedVector3Array& p_faces, const Transform3D& p_xform)
{
	ERR_FAIL_COND(p_faces.size() % 3 != 0);
	RWLockWrite write_lock(geometry_rwlock);
	_add_faces(p_faces, root_node_transform * p_xform);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::merge(
	const Ref<NavigationMeshSourceGeometryData3D>& p_other_geometry)
{
	ERR_FAIL_COND(p_other_geometry.is_null());

	Vector<float> other_vertices;
	Vector<int> other_indices;
	Vector<ProjectedObstruction> other_projected_obstructions;

	p_other_geometry->get_data(other_vertices, other_indices, other_projected_obstructions);

	RWLockWrite write_lock(geometry_rwlock);
	const int64_t number_of_vertices_before_merge = vertices.size();
	const int64_t number_of_indices_before_merge = indices.size();

	vertices.append_array(other_vertices);
	indices.append_array(other_indices);

	for (int64_t i = number_of_indices_before_merge; i < indices.size(); i++) {
		indices.set(i, indices[i] + number_of_vertices_before_merge / 3);
	}

	_projected_obstructions.append_array(other_projected_obstructions);
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::add_projected_obstruction(
	const Vector<Vector3>& p_vertices, float p_elevation, float p_height, bool p_carve)
{
	ERR_FAIL_COND(p_vertices.size() < 3);
	ERR_FAIL_COND(p_height < 0.0);

	ProjectedObstruction projected_obstruction;
	projected_obstruction.vertices.resize(p_vertices.size() * 3);
	projected_obstruction.elevation = p_elevation;
	projected_obstruction.height = p_height;
	projected_obstruction.carve = p_carve;

	float* obstruction_vertices_ptrw = projected_obstruction.vertices.ptrw();

	int vertex_index = 0;
	for (const Vector3& vertex : p_vertices) {
		obstruction_vertices_ptrw[vertex_index++] = vertex.x;
		obstruction_vertices_ptrw[vertex_index++] = vertex.y;
		obstruction_vertices_ptrw[vertex_index++] = vertex.z;
	}

	RWLockWrite write_lock(geometry_rwlock);
	_projected_obstructions.push_back(projected_obstruction);
	bounds_dirty = true;
}

Vector<NavigationMeshSourceGeometryData3D::ProjectedObstruction>
NavigationMeshSourceGeometryData3D::_get_projected_obstructions() const
{
	RWLockRead read_lock(geometry_rwlock);
	return _projected_obstructions;
}

void NavigationMeshSourceGeometryData3D::set_data(const Vector<float>& p_vertices,
	const Vector<int>& p_indices, Vector<ProjectedObstruction>& p_projected_obstructions)
{
	RWLockWrite write_lock(geometry_rwlock);
	vertices = p_vertices;
	indices = p_indices;
	_projected_obstructions = p_projected_obstructions;
	bounds_dirty = true;
}

void NavigationMeshSourceGeometryData3D::get_data(Vector<float>& r_vertices, Vector<int>& r_indices,
	Vector<ProjectedObstruction>& r_projected_obstructions)
{
	RWLockRead read_lock(geometry_rwlock);
	r_vertices = vertices;
	r_indices = indices;
	r_projected_obstructions = _projected_obstructions;
}

AABB NavigationMeshSourceGeometryData3D::get_bounds()
{
	geometry_rwlock.read_lock();

	if (bounds_dirty) {
		geometry_rwlock.read_unlock();
		RWLockWrite write_lock(geometry_rwlock);

		bounds_dirty = false;
		bounds = AABB();
		bool first_vertex = true;

		for (int i = 0; i < vertices.size() / 3; i++) {
			const Vector3 vertex =
				Vector3(vertices[i * 3], vertices[i * 3 + 1], vertices[i * 3 + 2]);
			if (first_vertex) {
				first_vertex = false;
				bounds.position = vertex;
			}
			else {
				bounds.expand_to(vertex);
			}
		}
		for (const ProjectedObstruction& projected_obstruction : _projected_obstructions) {
			for (int i = 0; i < projected_obstruction.vertices.size() / 3; i++) {
				const Vector3 vertex = Vector3(projected_obstruction.vertices[i * 3],
					projected_obstruction.vertices[i * 3 + 1],
					projected_obstruction.vertices[i * 3 + 2]);
				if (first_vertex) {
					first_vertex = false;
					bounds.position = vertex;
				}
				else {
					bounds.expand_to(vertex);
				}
			}
		}
	}
	else {
		geometry_rwlock.read_unlock();
	}

	RWLockRead read_lock(geometry_rwlock);
	return bounds;
}



void NavigationMeshSourceGeometryData3D::_add_mesh(Ref<Mesh> const&, Transform3D const&) {}
#endif // !defined(_3D_DISABLED) && !defined(NAVIGATION_3D_DISABLED)


// --- Merged from scene/3d/xr/xr_nodes.cpp ---
#if !defined(_3D_DISABLED) && !defined(XR_DISABLED)
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/types.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "servers/xr/xr_server.h"
#include "scene/3d/node_3d.h"

// XRNode3D is a node that has its transform updated by an XRTracker.
// Note that trackers are only available at runtime and only after an XRInterface registers one.
// So we bind by name and as long as a tracker isn't available, our node remains inactive.

void XRNode3D::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	update_configuration_warnings();
}

void XRNode3D::set_tracker(const String& p_tracker_name)
{
	if (tracker.is_valid() && tracker->get_tracker_name() == p_tracker_name) {
		return;
	}

	_unbind_tracker();

	tracker_name = p_tracker_name;
	pose_name = SceneString(default_);

	_bind_tracker();

	update_configuration_warnings();
}

String XRNode3D::get_tracker() const { return tracker_name; }

void XRNode3D::set_pose_name(const String& p_pose_name)
{
	pose_name = p_pose_name;

	// Update pose if we are bound to a tracker with a valid pose
	Ref<XRPose> pose = get_pose();
	if (pose.is_valid()) {
		set_transform(pose->get_adjusted_transform());
	}
}

String XRNode3D::get_pose_name() const { return pose_name; }

void XRNode3D::set_show_when_tracked(bool p_show)
{
	show_when_tracked = p_show;

	_update_visibility();
}

bool XRNode3D::get_show_when_tracked() const { return show_when_tracked; }

bool XRNode3D::get_is_active() const
{
	if (tracker.is_null()) {
		return false;
	}
	else if (!tracker->has_pose(pose_name)) {
		return false;
	}
	else {
		return true;
	}
}

bool XRNode3D::get_has_tracking_data() const { return has_tracking_data; }

void XRNode3D::trigger_haptic_pulse(const String& p_action_name, double p_frequency,
	double p_amplitude, double p_duration_sec, double p_delay_sec)
{
	// TODO need to link trackers to the interface that registered them so we can call this on the
	// correct interface. For now this works fine as in 99% of the cases we only have our primary
	// interface active
	if (XRServer::is_initialized()) {
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid()) {
			xr_interface->trigger_haptic_pulse(
				p_action_name, tracker_name, p_frequency, p_amplitude, p_duration_sec, p_delay_sec);
		}
	}
}

Ref<XRPose> XRNode3D::get_pose()
{
	if (tracker.is_valid()) {
		return tracker->get_pose(pose_name);
	}
	else {
		return Ref<XRPose>();
	}
}

void XRNode3D::_bind_tracker()
{
	ERR_FAIL_COND(tracker.is_valid());

	if (XRServer::is_initialized()) {
		tracker = XRServer::get_tracker(tracker_name);
		if (tracker.is_null()) {
			return;
		}

		Ref<XRPose> pose = get_pose();
		if (pose.is_valid()) {
			set_transform(pose->get_adjusted_transform());
			_set_has_tracking_data(pose->get_has_tracking_data());
		}
		else {
			_set_has_tracking_data(false);
		}
	}
}

void XRNode3D::_unbind_tracker()
{
	if (tracker.is_valid()) {
		tracker.unref();
		_set_has_tracking_data(false);
	}
}

void XRNode3D::_changed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// just in case unref our current tracker
		_unbind_tracker();

		// get our new tracker
		_bind_tracker();
	}
}

void XRNode3D::_removed_tracker(const String& p_tracker_name, int p_tracker_type)
{
	if (tracker_name == p_tracker_name) {
		// unref our tracker, it's no longer available
		_unbind_tracker();
	}
}

void XRNode3D::_pose_changed(const Ref<XRPose>& p_pose)
{
	if (p_pose.is_valid() && p_pose->get_name() == pose_name) {
		set_transform(p_pose->get_adjusted_transform());
		_set_has_tracking_data(p_pose->get_has_tracking_data());
	}
}

void XRNode3D::_pose_lost_tracking(const Ref<XRPose>& p_pose)
{
	if (p_pose.is_valid() && p_pose->get_name() == pose_name) {
		_set_has_tracking_data(false);
	}
}

void XRNode3D::_set_has_tracking_data(bool p_has_tracking_data)
{
	if (has_tracking_data != p_has_tracking_data) {
		has_tracking_data = p_has_tracking_data;
		_update_visibility();
	}
}

void XRNode3D::_update_visibility()
{
	// If configured, show or hide the node based on tracking data.
	if (show_when_tracked) {
		// Only react to this if we have a primary interface.
		if (XRServer::is_initialized()) {
			Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
			if (xr_interface.is_valid()) {
				set_visible(has_tracking_data);
			}
		}
	}
}

void XRNode3D::_button_pressed(const String& p_name) {}

void XRNode3D::_button_released(const String& p_name) {}

void XRNode3D::_input_float_changed(const String& p_name, float p_value) {}

void XRNode3D::_input_vector2_changed(const String& p_name, Vector2 p_value) {}

void XRNode3D::_profile_changed(const String& p_role) {}

bool XRNode3D::is_button_pressed(const String& p_name) const
{
	if (tracker.is_null()) {
		return false;
	}
	return tracker->is_button_pressed(p_name);
}

XRTracker::InputValue XRNode3D::get_input(const String& p_name) const
{
	if (tracker.is_null()) {
		return XRTracker::InputValue();
	}
	return tracker->get_input(p_name);
}

float XRNode3D::get_float(const String& p_name) const
{
	if (tracker.is_null()) {
		return 0.0f;
	}
	return tracker->get_input_float(p_name);
}

Vector2 XRNode3D::get_vector2(const String& p_name) const
{
	if (tracker.is_null()) {
		return Vector2();
	}
	return tracker->get_input_vector2(p_name);
}

XRTracker::TrackerHand XRNode3D::get_tracker_hand() const
{
	if (tracker.is_null()) {
		return XRTracker::TRACKER_HAND_UNKNOWN;
	}

	return tracker->get_tracker_hand();
}

Vector3 XRNode3D::get_size() const { return size; }

Plane XRNode3D::get_plane() const
{
	Vector3 location = get_position();
	Basis orientation = get_transform().basis;

	Plane plane(orientation.get_column(1).normalized(), location);

	return plane;
}

PackedStringArray XRNode3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (is_visible() && is_inside_tree()) {
		const XROrigin3D* origin = dynamic_cast<const XROrigin3D*>(get_parent());
		if (origin == nullptr) {
			warnings.push_back(RTR("XRNode3D may not function as expected without an XROrigin3D node as its parent."));
		}
		if (tracker_name.is_empty()) {
			warnings.push_back(RTR("No tracker name is set."));
		}
		if (mode != MODE_ANCHOR && pose_name.is_empty()) {
			warnings.push_back(RTR("No pose is set."));
		}
	}

	if (get_physics_interpolation_mode() != Node::PHYSICS_INTERPOLATION_MODE_OFF) {
		warnings.push_back(RTR("XRNode3D should have physics_interpolation_mode set to OFF in order to avoid jitter."));
	}

	return warnings;
}

XRNode3D::XRNode3D(Mode p_mode) : mode(p_mode) {}

XRNode3D::~XRNode3D() { _unbind_tracker(); }

// XROrigin3D

Vector<XROrigin3D*> XROrigin3D::origin_nodes;

real_t XROrigin3D::get_world_scale() const
{
	// get our XRServer
	ERR_FAIL_COND_V(!XRServer::is_initialized(), 1.0);

	return XRServer::get_world_scale();
}

void XROrigin3D::set_world_scale(real_t p_world_scale)
{
	// get our XRServer
	ERR_FAIL_COND(!XRServer::is_initialized());

	XRServer::set_world_scale(p_world_scale);
}

void XROrigin3D::_set_current(bool p_enabled, bool p_update_others)
{
	// We run this logic even if current already equals p_enabled as we may have set this previously
	// before we entered our tree. This is then called a second time on NOTIFICATION_ENTER_TREE
	// where we actually process activating this origin node.
	current = p_enabled;

	if (!is_inside_tree() || Engine::is_editor_hint()) {
		return;
	}

	// Notify us of any transform changes
	set_notify_local_transform(current);
	set_notify_transform(current);

	// update XRServer with our current position
	if (current) {
		ERR_FAIL_COND(!XRServer::is_initialized());

		XRServer::set_world_origin(get_global_transform());

		if (is_physics_interpolated()) {
			set_process_internal(true);
		}
	}
	else if (is_physics_interpolated()) {
		set_process_internal(false);
	}

	// Check if we need to update our other origin nodes accordingly
	if (p_update_others) {
		if (current) {
			for (int i = 0; i < origin_nodes.size(); i++) {
				if (origin_nodes[i] != this && origin_nodes[i]->current) {
					origin_nodes[i]->_set_current(false, false);
				}
			}
		}
		else {
			// We no longer have a current origin so find the first one we can make current
			for (int i = 0; i < origin_nodes.size(); i++) {
				if (origin_nodes[i] != this) {
					origin_nodes[i]->_set_current(true, false);
					return; // we are done.
				}
			}
		}
	}
}

void XROrigin3D::set_current(bool p_enabled) { _set_current(p_enabled, true); }

bool XROrigin3D::is_current() const
{
	if (Engine::is_editor_hint()) {
		// return as is
		return current;
	}
	else {
		return current && is_inside_tree();
	}
}

void XROrigin3D::_physics_interpolated_changed()
{
	if (current && !Engine::is_editor_hint()) {
		set_process_internal(is_physics_interpolated());
	}
}

PackedStringArray XROrigin3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();
	if (is_visible() && is_inside_tree()) {
		bool has_xr_camera = false;
		for (int i = 0; i < get_child_count(); i++) {
			const Camera3D* camera = dynamic_cast<const Camera3D*>(get_child(i));
			if (camera && camera->is_xr_enabled()) {
				has_xr_camera = true;
				break;
			}
		}
		if (!has_xr_camera) {
			warnings.push_back(RTR("XROrigin3D requires an XR-enabled Camera3D child node."));
		}
	}
	return warnings;
}

// ==========================================================================
// Merged from scene/3d/xr/xr_body_modifier_3d.cpp
// ==========================================================================
// --- Inlined from scene/3d/xr/xr_body_modifier_3d.h ---
#include "scene/3d/skeleton_3d.h"
#include "servers/xr/xr_server.h"

class Skeleton3D;

/**
	The XRBodyModifier3D node drives a body or hand skeleton using tracking
	data from an XRTracker instance.
 */

class XRBodyModifier3D final : public SkeletonModifier3D {
public:
	enum TargetMode {
		TARGET_BODY,
		TARGET_HAND,
	};

	enum BodyUpdate {
		BODY_UPDATE_UPPER_BODY = 1,
		BODY_UPDATE_LOWER_BODY = 2,
		BODY_UPDATE_HANDS = 4,
	};

	enum BoneUpdate {
		BONE_UPDATE_FULL,
		BONE_UPDATE_ROTATION_ONLY,
		BONE_UPDATE_MAX,
	};

	XRBodyModifier3D(TargetMode p_mode = TARGET_BODY);

	void set_target_mode(TargetMode p_mode);
	TargetMode get_target_mode() const { return target_mode; }

	void set_tracker_name(const String &p_tracker_name);
	String get_tracker_name() const;

	void set_body_tracker(const String &p_tracker_name);
	String get_body_tracker() const;

	void set_hand_tracker(const String &p_tracker_name);
	String get_hand_tracker() const;

	void set_body_update(uint32_t p_body_update);
	uint32_t get_body_update() const;

	void set_bone_update(BoneUpdate p_bone_update);
	BoneUpdate get_bone_update() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	void _notification(int p_what);

protected:
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _process_modification(double p_delta) override;

private:
	struct JointData {
		int bone = -1;
		int parent_joint = -1;
	};

	TargetMode target_mode = TARGET_BODY;
	String tracker_name = "/user/body_tracker";
	uint32_t body_update = BODY_UPDATE_UPPER_BODY | BODY_UPDATE_LOWER_BODY | BODY_UPDATE_HANDS;
	BoneUpdate bone_update = BONE_UPDATE_FULL;
	JointData body_joints[XRTracker::JOINT_MAX];
	JointData hand_joints[XRTracker::HAND_JOINT_MAX];

	bool has_stored_previous_transforms = false;
	Vector<Transform3D> previous_relative_transforms;

	void _get_joint_data();
	void _get_body_joint_data();
	void _get_hand_joint_data();
	void _process_body_modification(Skeleton3D *p_skeleton);
	void _process_hand_modification(Skeleton3D *p_skeleton);
	void _tracker_changed(const String &p_tracker_name, XRServer::TrackerType p_tracker_type);
};

// --- End scene/3d/xr/xr_body_modifier_3d.h ---

#include "scene/3d/skeleton_3d.h"
#include "servers/xr/xr_server.h"

XRBodyModifier3D::XRBodyModifier3D(TargetMode p_mode) :
		target_mode(p_mode) {
	if (p_mode == TARGET_HAND) {
		tracker_name = "/user/hand_tracker/left";
	} else {
		tracker_name = "/user/body_tracker";
	}
}

void XRBodyModifier3D::set_target_mode(TargetMode p_mode) {
	if (target_mode == p_mode) {
		return;
	}
	target_mode = p_mode;
	if (target_mode == TARGET_HAND && tracker_name == "/user/body_tracker") {
		tracker_name = "/user/hand_tracker/left";
	} else if (target_mode == TARGET_BODY && tracker_name.begins_with("/user/hand_tracker")) {
		tracker_name = "/user/body_tracker";
	}
	has_stored_previous_transforms = false;
	previous_relative_transforms.clear();
	if (is_inside_tree()) {
		_get_joint_data();
	}
	update_configuration_warnings();
}

void XRBodyModifier3D::set_tracker_name(const String &p_tracker_name) {
	tracker_name = p_tracker_name;
	if (is_inside_tree()) {
		_get_joint_data();
	}
}

String XRBodyModifier3D::get_tracker_name() const {
	return tracker_name;
}

void XRBodyModifier3D::set_body_tracker(const String &p_tracker_name) {
	set_tracker_name(p_tracker_name);
}

String XRBodyModifier3D::get_body_tracker() const {
	return tracker_name;
}

void XRBodyModifier3D::set_hand_tracker(const String &p_tracker_name) {
	set_tracker_name(p_tracker_name);
}

String XRBodyModifier3D::get_hand_tracker() const {
	return tracker_name;
}

void XRBodyModifier3D::set_body_update(uint32_t p_body_update) {
	body_update = p_body_update;
	if (is_inside_tree()) {
		_get_joint_data();
	}
}

uint32_t XRBodyModifier3D::get_body_update() const {
	return body_update;
}

void XRBodyModifier3D::set_bone_update(BoneUpdate p_bone_update) {
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);
	bone_update = p_bone_update;
}

XRBodyModifier3D::BoneUpdate XRBodyModifier3D::get_bone_update() const {
	return bone_update;
}

PackedStringArray XRBodyModifier3D::get_configuration_warnings() const {
	PackedStringArray warnings = SkeletonModifier3D::get_configuration_warnings();
	if (tracker_name.is_empty()) {
		warnings.push_back(RTR("No XR tracker name specified."));
	}
	return warnings;
}

void XRBodyModifier3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			_get_joint_data();
		} break;
		case NOTIFICATION_EXIT_TREE: {
			for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
				body_joints[i].bone = -1;
				body_joints[i].parent_joint = -1;
			}
			for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
				hand_joints[i].bone = -1;
				hand_joints[i].parent_joint = -1;
			}
			has_stored_previous_transforms = false;
			previous_relative_transforms.clear();
		} break;
		default:
			break;
	}
}

void XRBodyModifier3D::_tracker_changed(const String &p_tracker_name, XRServer::TrackerType p_tracker_type) {
	if (tracker_name == p_tracker_name) {
		_get_joint_data();
	}
}

void XRBodyModifier3D::_skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) {
	has_stored_previous_transforms = false;
	previous_relative_transforms.clear();
	_get_joint_data();
}

void XRBodyModifier3D::_get_joint_data() {
	if (target_mode == TARGET_HAND) {
		_get_hand_joint_data();
	} else {
		_get_body_joint_data();
	}
}

void XRBodyModifier3D::_get_body_joint_data() {
	for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
		body_joints[i].bone = -1;
		body_joints[i].parent_joint = -1;
	}

	const Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	static const char *const bone_names[XRTracker::JOINT_MAX] = {
		"Root",
		// Upper body.
		"Hips",
		"Spine",
		"Chest",
		"UpperChest",
		"Neck",
		"Head",
		"HeadTip",
		"LeftShoulder",
		"LeftUpperArm",
		"LeftLowerArm",
		"RightShoulder",
		"RightUpperArm",
		"RightLowerArm",
		// Lower body.
		"LeftUpperLeg",
		"LeftLowerLeg",
		"LeftFoot",
		"LeftToes",
		"RightUpperLeg",
		"RightLowerLeg",
		"RightFoot",
		"RightToes",
		// Left hand.
		"LeftHand",
		"LeftPalm",
		"LeftWrist",
		"LeftThumbMetacarpal",
		"LeftThumbProximal",
		"LeftThumbDistal",
		"LeftThumbTip",
		"LeftIndexMetacarpal",
		"LeftIndexProximal",
		"LeftIndexIntermediate",
		"LeftIndexDistal",
		"LeftIndexTip",
		"LeftMiddleMetacarpal",
		"LeftMiddleProximal",
		"LeftMiddleIntermediate",
		"LeftMiddleDistal",
		"LeftMiddleTip",
		"LeftRingMetacarpal",
		"LeftRingProximal",
		"LeftRingIntermediate",
		"LeftRingDistal",
		"LeftRingTip",
		"LeftLittleMetacarpal",
		"LeftLittleProximal",
		"LeftLittleIntermediate",
		"LeftLittleDistal",
		"LeftLittleTip",
		// Right hand.
		"RightHand",
		"RightPalm",
		"RightWrist",
		"RightThumbMetacarpal",
		"RightThumbProximal",
		"RightThumbDistal",
		"RightThumbTip",
		"RightIndexMetacarpal",
		"RightIndexProximal",
		"RightIndexIntermediate",
		"RightIndexDistal",
		"RightIndexTip",
		"RightMiddleMetacarpal",
		"RightMiddleProximal",
		"RightMiddleIntermediate",
		"RightMiddleDistal",
		"RightMiddleTip",
		"RightRingMetacarpal",
		"RightRingProximal",
		"RightRingIntermediate",
		"RightRingDistal",
		"RightRingTip",
		"RightLittleMetacarpal",
		"RightLittleProximal",
		"RightLittleIntermediate",
		"RightLittleDistal",
		"RightLittleTip",
		// Extra joints.
		"LowerChest",
		"LeftScapula",
		"LeftWristTwist",
		"RightScapula",
		"RightWristTwist",
		"LeftFootTwist",
		"LeftHeel",
		"LeftMiddleFoot",
		"RightFootTwist",
		"RightHeel",
		"RightMiddleFoot",
	};

	int bones[XRTracker::JOINT_MAX];
	for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
		bones[i] = -1;
	}

	bones[XRTracker::JOINT_ROOT] = skeleton->find_bone(bone_names[XRTracker::JOINT_ROOT]);

	if (body_update & BODY_UPDATE_UPPER_BODY) {
		for (int i = XRTracker::JOINT_HIPS; i <= XRTracker::JOINT_RIGHT_LOWER_ARM; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
		bones[XRTracker::JOINT_LOWER_CHEST] = skeleton->find_bone(bone_names[XRTracker::JOINT_LOWER_CHEST]);
		bones[XRTracker::JOINT_LEFT_SCAPULA] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_SCAPULA]);
		bones[XRTracker::JOINT_LEFT_WRIST_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_WRIST_TWIST]);
		bones[XRTracker::JOINT_RIGHT_SCAPULA] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_SCAPULA]);
		bones[XRTracker::JOINT_RIGHT_WRIST_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_WRIST_TWIST]);
	}

	if (body_update & BODY_UPDATE_LOWER_BODY) {
		for (int i = XRTracker::JOINT_LEFT_UPPER_LEG; i <= XRTracker::JOINT_RIGHT_TOES; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
		bones[XRTracker::JOINT_LEFT_FOOT_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_FOOT_TWIST]);
		bones[XRTracker::JOINT_LEFT_HEEL] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_HEEL]);
		bones[XRTracker::JOINT_LEFT_MIDDLE_FOOT] = skeleton->find_bone(bone_names[XRTracker::JOINT_LEFT_MIDDLE_FOOT]);
		bones[XRTracker::JOINT_RIGHT_FOOT_TWIST] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_FOOT_TWIST]);
		bones[XRTracker::JOINT_RIGHT_HEEL] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_HEEL]);
		bones[XRTracker::JOINT_RIGHT_MIDDLE_FOOT] = skeleton->find_bone(bone_names[XRTracker::JOINT_RIGHT_MIDDLE_FOOT]);
	}

	if (body_update & BODY_UPDATE_HANDS) {
		for (int i = XRTracker::JOINT_LEFT_HAND; i <= XRTracker::JOINT_RIGHT_PINKY_FINGER_TIP; i++) {
			bones[i] = skeleton->find_bone(bone_names[i]);
		}
	}

	for (int i = 0; i < XRTracker::JOINT_MAX; i++) {
		const int b = bones[i];
		if (b == -1) {
			continue;
		}
		body_joints[i].bone = b;
		int parent_bone = skeleton->get_bone_parent(b);
		while (parent_bone != -1) {
			bool found = false;
			for (int j = 0; j < XRTracker::JOINT_MAX; j++) {
				if (bones[j] == parent_bone) {
					body_joints[i].parent_joint = j;
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
			parent_bone = skeleton->get_bone_parent(parent_bone);
		}
	}
}

void XRBodyModifier3D::_get_hand_joint_data() {
	for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
		hand_joints[i].bone = -1;
		hand_joints[i].parent_joint = -1;
	}

	const Skeleton3D *skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	static const char *const left_hand_names[XRTracker::HAND_JOINT_MAX] = {
		"LeftPalm",
		"LeftHand",
		"LeftThumbMetacarpal",
		"LeftThumbProximal",
		"LeftThumbDistal",
		"LeftThumbTip",
		"LeftIndexMetacarpal",
		"LeftIndexProximal",
		"LeftIndexIntermediate",
		"LeftIndexDistal",
		"LeftIndexTip",
		"LeftMiddleMetacarpal",
		"LeftMiddleProximal",
		"LeftMiddleIntermediate",
		"LeftMiddleDistal",
		"LeftMiddleTip",
		"LeftRingMetacarpal",
		"LeftRingProximal",
		"LeftRingIntermediate",
		"LeftRingDistal",
		"LeftRingTip",
		"LeftLittleMetacarpal",
		"LeftLittleProximal",
		"LeftLittleIntermediate",
		"LeftLittleDistal",
		"LeftLittleTip",
	};

	static const char *const right_hand_names[XRTracker::HAND_JOINT_MAX] = {
		"RightPalm",
		"RightHand",
		"RightThumbMetacarpal",
		"RightThumbProximal",
		"RightThumbDistal",
		"RightThumbTip",
		"RightIndexMetacarpal",
		"RightIndexProximal",
		"RightIndexIntermediate",
		"RightIndexDistal",
		"RightIndexTip",
		"RightMiddleMetacarpal",
		"RightMiddleProximal",
		"RightMiddleIntermediate",
		"RightMiddleDistal",
		"RightMiddleTip",
		"RightRingMetacarpal",
		"RightRingProximal",
		"RightRingIntermediate",
		"RightRingDistal",
		"RightRingTip",
		"RightLittleMetacarpal",
		"RightLittleProximal",
		"RightLittleIntermediate",
		"RightLittleDistal",
		"RightLittleTip",
	};

	const bool is_right = tracker_name.contains("right");
	const char *const *names = is_right ? right_hand_names : left_hand_names;

	int bones[XRTracker::HAND_JOINT_MAX];
	for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
		bones[i] = skeleton->find_bone(names[i]);
	}

	for (int i = 0; i < XRTracker::HAND_JOINT_MAX; i++) {
		const int b = bones[i];
		if (b == -1) {
			continue;
		}
		hand_joints[i].bone = b;
		int parent_bone = skeleton->get_bone_parent(b);
		while (parent_bone != -1) {
			bool found = false;
			for (int j = 0; j < XRTracker::HAND_JOINT_MAX; j++) {
				if (bones[j] == parent_bone) {
					hand_joints[i].parent_joint = j;
					found = true;
					break;
				}
			}
			if (found) {
				break;
			}
			parent_bone = skeleton->get_bone_parent(parent_bone);
		}
	}
}

void XRBodyModifier3D::_process_modification(double p_delta) {
	Skeleton3D *skeleton = get_skeleton();
	if (!skeleton || !XRServer::is_initialized()) {
		return;
	}

	if (target_mode == TARGET_HAND) {
		_process_hand_modification(skeleton);
	} else {
		_process_body_modification(skeleton);
	}
}

void XRBodyModifier3D::_process_body_modification(Skeleton3D *p_skeleton) {
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_BODY || !tracker->get_has_tracking_data()) {
		return;
	}

	const float ss = p_skeleton->get_motion_scale();
	Transform3D transforms[XRTracker::JOINT_MAX];
	Transform3D inv_transforms[XRTracker::JOINT_MAX];
	bool valid[XRTracker::JOINT_MAX] = {};

	for (int joint = 0; joint < XRTracker::JOINT_MAX; joint++) {
		uint32_t flags = tracker->get_joint_flags(static_cast<XRTracker::Joint>(joint));
		if ((flags & XRTracker::JOINT_FLAG_ORIENTATION_VALID) && (flags & XRTracker::JOINT_FLAG_POSITION_VALID)) {
			valid[joint] = true;
			transforms[joint] = tracker->get_joint_transform(static_cast<XRTracker::Joint>(joint));
			transforms[joint].origin *= ss;
			inv_transforms[joint] = transforms[joint].inverse();
		}
	}

	if (!valid[XRTracker::JOINT_ROOT]) {
		return;
	}

	for (int joint = 0; joint < XRTracker::JOINT_MAX; joint++) {
		const int b = body_joints[joint].bone;
		if (b == -1 || !valid[joint]) {
			continue;
		}
		const int parent_joint = body_joints[joint].parent_joint;
		Transform3D rel_transform;
		if (parent_joint != -1 && valid[parent_joint]) {
			rel_transform = inv_transforms[parent_joint] * transforms[joint];
		} else {
			rel_transform = transforms[joint];
		}

		p_skeleton->set_bone_pose_rotation(b, Quaternion(rel_transform.basis));
		if (bone_update == BONE_UPDATE_FULL) {
			p_skeleton->set_bone_pose_position(b, rel_transform.origin);
		}
	}
}

void XRBodyModifier3D::_process_hand_modification(Skeleton3D *p_skeleton) {
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_HAND) {
		return;
	}

	if (!tracker->get_has_tracking_data()) {
		if (!has_stored_previous_transforms) {
			return;
		}
		for (int joint = 0; joint < XRTracker::HAND_JOINT_MAX; joint++) {
			const int b = hand_joints[joint].bone;
			if (b == -1) {
				continue;
			}
			p_skeleton->set_bone_pose_rotation(b, Quaternion(previous_relative_transforms[joint].basis));
			if (bone_update == BONE_UPDATE_FULL) {
				p_skeleton->set_bone_pose_position(b, previous_relative_transforms[joint].origin);
			}
		}
		return;
	}

	const float ss = p_skeleton->get_motion_scale();
	Transform3D transforms[XRTracker::HAND_JOINT_MAX];
	Transform3D inv_transforms[XRTracker::HAND_JOINT_MAX];
	bool valid[XRTracker::HAND_JOINT_MAX] = {};

	previous_relative_transforms.resize(XRTracker::HAND_JOINT_MAX);

	for (int joint = 0; joint < XRTracker::HAND_JOINT_MAX; joint++) {
		uint32_t flags = tracker->get_hand_joint_flags(static_cast<XRTracker::HandJoint>(joint));
		if (flags & XRTracker::HAND_JOINT_FLAG_ORIENTATION_VALID) {
			valid[joint] = true;
			transforms[joint] = tracker->get_hand_joint_transform(static_cast<XRTracker::HandJoint>(joint));
			transforms[joint].origin *= ss;
			inv_transforms[joint] = transforms[joint].inverse();
		}
	}

	if (!valid[XRTracker::HAND_JOINT_WRIST]) {
		return;
	}

	const Transform3D skel_inv = p_skeleton->get_global_transform_interpolated().affine_inverse();

	for (int joint = 0; joint < XRTracker::HAND_JOINT_MAX; joint++) {
		const int b = hand_joints[joint].bone;
		if (b == -1 || !valid[joint]) {
			continue;
		}
		const int parent_joint = hand_joints[joint].parent_joint;
		Transform3D rel_transform;
		if (parent_joint != -1 && valid[parent_joint]) {
			rel_transform = inv_transforms[parent_joint] * transforms[joint];
		} else {
			rel_transform = skel_inv * transforms[joint];
		}

		previous_relative_transforms.write[joint] = rel_transform;
		p_skeleton->set_bone_pose_rotation(b, Quaternion(rel_transform.basis));
		if (bone_update == BONE_UPDATE_FULL) {
			p_skeleton->set_bone_pose_position(b, rel_transform.origin);
		}
	}

	has_stored_previous_transforms = true;
}

// ==========================================================================
// Merged from scene/3d/xr/xr_face_modifier_3d.cpp
// ==========================================================================

#include "servers/xr/xr_server.h"
// --- Inlined from scene/3d/xr/xr_face_modifier_3d.h ---
#include "core/templates/rb_map.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/node_3d.h"

/**
	The XRFaceModifier3D node drives the blend shapes of a MeshInstance3D
	with facial expressions from an XRFaceTracking instance.

	The blend shapes provided by the mesh are interrogated, and used to
	deduce an optimal mapping from the Unified Expressions blend shapes
	provided by the	XRFaceTracking instance to drive the face.
 */

class XRFaceModifier3D final : public Node3D {

private:
	String tracker_name = "/user/face_tracker";
	NodePath target;

	// Map from XRFaceTracker blend shape index to mesh blend shape index.
	RBMap<int, int> blend_mapping;

	MeshInstance3D *get_mesh_instance() const;
	void _get_blend_data();
	void _update_face_blends() const;

protected:

public:
	void set_face_tracker(const String &p_tracker_name);
	String get_face_tracker() const;

	void set_target(const NodePath &p_target);
	NodePath get_target() const;

	void _notification(int p_what);
};

// --- End scene/3d/xr/xr_face_modifier_3d.h ---

// This method takes the name of a mesh blend shape and returns the
// corresponding XRFaceTracker blend shape. If no match is
// found then the function returns -1.
static int find_face_blend_shape(const String& p_name)
{
	// Entry for blend shape name table.
	struct blend_map_entry
	{
		int blend;
		const char* name[4];
	};

	// Table of blend shape names.
	//
	// This table consists of the XRFaceTracker blend shape and
	// the corresponding names (lowercase and no underscore) of:
	// - The Unified Expression blend shape name.
	// - The ARKit blend shape name (if present and different).
	// - The SRanipal blend shape name (if present and different).
	// - The Meta blend shape name (if present and different).
	static constexpr blend_map_entry blend_map[] = {
		{XRTracker::FT_EYE_LOOK_OUT_RIGHT,
			{"eyelookoutright", "eyerightright", "eyeslookoutr"}},
		{XRTracker::FT_EYE_LOOK_IN_RIGHT, {"eyelookinright", "eyerightleft", "eyeslookinr"}},
		{XRTracker::FT_EYE_LOOK_UP_RIGHT, {"eyelookupright", "eyerightlookup", "eyeslookupr"}},
		{XRTracker::FT_EYE_LOOK_DOWN_RIGHT,
			{"eyelookdownright", "eyerightlookdown", "eyeslookdownr"}},
		{XRTracker::FT_EYE_LOOK_OUT_LEFT, {"eyelookoutleft", "eyeleftleft", "eyeslookoutl"}},
		{XRTracker::FT_EYE_LOOK_IN_LEFT, {"eyelookinleft", "eyeleftright", "eyeslookinl"}},
		{XRTracker::FT_EYE_LOOK_UP_LEFT, {"eyelookupleft", "eyeleftlookup", "eyeslookupl"}},
		{XRTracker::FT_EYE_LOOK_DOWN_LEFT,
			{"eyelookdownleft", "eyeleftlookdown", "eyeslookdownl"}},
		{XRTracker::FT_EYE_CLOSED_RIGHT,
			{"eyeclosedright", "eyeblinkright", "eyerightblink", "eyesclosedr"}},
		{XRTracker::FT_EYE_CLOSED_LEFT,
			{"eyeclosedleft", "eyeblinkleft", "eyeleftblink", "eyesclosedl"}},
		{XRTracker::FT_EYE_SQUINT_RIGHT, {"eyesquintright", "eyessquintr"}},
		{XRTracker::FT_EYE_SQUINT_LEFT, {"eyesquintleft", "eyessquintl"}},
		{XRTracker::FT_EYE_WIDE_RIGHT, {"eyewideright", "eyerightwide", "eyeswidenr"}},
		{XRTracker::FT_EYE_WIDE_LEFT, {"eyewideleft", "eyeleftwide", "eyeswidenl"}},
		{XRTracker::FT_EYE_DILATION_RIGHT, {"eyedilationright", "eyerightdilation"}},
		{XRTracker::FT_EYE_DILATION_LEFT, {"eyedilationleft", "eyeleftdilation"}},
		{XRTracker::FT_EYE_CONSTRICT_RIGHT, {"eyeconstrictright", "eyerightconstrict"}},
		{XRTracker::FT_EYE_CONSTRICT_LEFT, {"eyeconstrictleft", "eyeleftconstrict"}},
		{XRTracker::FT_BROW_PINCH_RIGHT, {"browpinchright"}},
		{XRTracker::FT_BROW_PINCH_LEFT, {"browpinchleft"}},
		{XRTracker::FT_BROW_LOWERER_RIGHT, {"browlowererright"}},
		{XRTracker::FT_BROW_LOWERER_LEFT, {"browlowererleft"}},
		{XRTracker::FT_BROW_INNER_UP_RIGHT, {"browinnerupright", "innerbrowraiserr"}},
		{XRTracker::FT_BROW_INNER_UP_LEFT, {"browinnerupleft", "innerbrowraiserl"}},
		{XRTracker::FT_BROW_OUTER_UP_RIGHT, {"browouterupright", "outerbrowraiserr"}},
		{XRTracker::FT_BROW_OUTER_UP_LEFT, {"browouterupleft", "outerbrowraiserl"}},
		{XRTracker::FT_NOSE_SNEER_RIGHT, {"nosesneerright", "nosewrinklerr"}},
		{XRTracker::FT_NOSE_SNEER_LEFT, {"nosesneerleft", "nosewrinklerl"}},
		{XRTracker::FT_NASAL_DILATION_RIGHT, {"nasaldilationright"}},
		{XRTracker::FT_NASAL_DILATION_LEFT, {"nasaldilationleft"}},
		{XRTracker::FT_NASAL_CONSTRICT_RIGHT, {"nasalconstrictright"}},
		{XRTracker::FT_NASAL_CONSTRICT_LEFT, {"nasalconstrictleft"}},
		{XRTracker::FT_CHEEK_SQUINT_RIGHT, {"cheeksquintright", "cheekraiserr"}},
		{XRTracker::FT_CHEEK_SQUINT_LEFT, {"cheeksquintleft", "cheekraiserl"}},
		{XRTracker::FT_CHEEK_PUFF_RIGHT, {"cheekpuffright", "cheekpuffr"}},
		{XRTracker::FT_CHEEK_PUFF_LEFT, {"cheekpuffleft", "cheekpuffl"}},
		{XRTracker::FT_CHEEK_SUCK_RIGHT, {"cheeksuckright", "cheeksuckr"}},
		{XRTracker::FT_CHEEK_SUCK_LEFT, {"cheeksuckleft", "cheeksuckl"}},
		{XRTracker::FT_JAW_OPEN, {"jawopen", "jawdrop"}},
		{XRTracker::FT_MOUTH_CLOSED,
			{"mouthclosed", "mouthclose", "mouthapeshape", "lipstoward"}},
		{XRTracker::FT_JAW_RIGHT, {"jawright", "jawsidewaysright"}},
		{XRTracker::FT_JAW_LEFT, {"jawleft", "jawsidewaysleft"}},
		{XRTracker::FT_JAW_FORWARD, {"jawforward", "jawthrust"}},
		{XRTracker::FT_JAW_BACKWARD, {"jawbackward"}},
		{XRTracker::FT_JAW_CLENCH, {"jawclench"}},
		{XRTracker::FT_JAW_MANDIBLE_RAISE, {"jawmandibleraise"}},
		{XRTracker::FT_LIP_SUCK_UPPER_RIGHT, {"lipsuckupperright", "lipsuckrt"}},
		{XRTracker::FT_LIP_SUCK_UPPER_LEFT, {"lipsuckupperleft", "lipsucklt"}},
		{XRTracker::FT_LIP_SUCK_LOWER_RIGHT, {"lipsucklowerright", "lipsuckrb"}},
		{XRTracker::FT_LIP_SUCK_LOWER_LEFT, {"lipsucklowerleft", "lipsucklb"}},
		{XRTracker::FT_LIP_SUCK_CORNER_RIGHT, {"lipsuckcornerright"}},
		{XRTracker::FT_LIP_SUCK_CORNER_LEFT, {"lipsuckcornerleft"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT, {"lipfunnelupperright", "lipfunnelerrt"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER_LEFT, {"lipfunnelupperleft", "lipfunnelerlt"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT, {"lipfunnellowerright", "lipsuckrb"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER_LEFT, {"lipfunnellowerleft", "lipsucklb"}},
		{XRTracker::FT_LIP_PUCKER_UPPER_RIGHT, {"lippuckerupperright"}},
		{XRTracker::FT_LIP_PUCKER_UPPER_LEFT, {"lippuckerupperleft"}},
		{XRTracker::FT_LIP_PUCKER_LOWER_RIGHT, {"lippuckerlowerright"}},
		{XRTracker::FT_LIP_PUCKER_LOWER_LEFT, {"lippuckerlowerleft"}},
		{XRTracker::FT_MOUTH_UPPER_UP_RIGHT, {"mouthupperupright", "upperlipraiserr"}},
		{XRTracker::FT_MOUTH_UPPER_UP_LEFT, {"mouthupperupleft", "upperlipraiserl"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT,
			{"mouthlowerdownright", "mouthlowerupright", "lowerlipdepressorr"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN_LEFT,
			{"mouthlowerdownleft", "mouthlowerupleft", "lowerlipdepressorl"}},
		{XRTracker::FT_MOUTH_UPPER_DEEPEN_RIGHT, {"mouthupperdeepenright"}},
		{XRTracker::FT_MOUTH_UPPER_DEEPEN_LEFT, {"mouthupperdeepenleft"}},
		{XRTracker::FT_MOUTH_UPPER_RIGHT, {"mouthupperright"}},
		{XRTracker::FT_MOUTH_UPPER_LEFT, {"mouthupperleft"}},
		{XRTracker::FT_MOUTH_LOWER_RIGHT, {"mouthlowerright"}},
		{XRTracker::FT_MOUTH_LOWER_LEFT, {"mouthlowerleft"}},
		{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, {"mouthcornerpullright"}},
		{XRTracker::FT_MOUTH_CORNER_PULL_LEFT, {"mouthcornerpullleft"}},
		{XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT, {"mouthcornerslantright"}},
		{XRTracker::FT_MOUTH_CORNER_SLANT_LEFT, {"mouthcornerslantleft"}},
		{XRTracker::FT_MOUTH_FROWN_RIGHT, {"mouthfrownright", "lipcornerdepressorr"}},
		{XRTracker::FT_MOUTH_FROWN_LEFT, {"mouthfrownleft", "lipcornerdepressorl"}},
		{XRTracker::FT_MOUTH_STRETCH_RIGHT, {"mouthstretchright", "lipstretcherr"}},
		{XRTracker::FT_MOUTH_STRETCH_LEFT, {"mouthstretchleft", "lipstretcherl"}},
		{XRTracker::FT_MOUTH_DIMPLE_RIGHT,
			{"mouthdimplerright", "mouthdimpleright", "dimplerr"}},
		{XRTracker::FT_MOUTH_DIMPLE_LEFT, {"mouthdimplerleft", "mouthdimpleleft", "dimplerl"}},
		{XRTracker::FT_MOUTH_RAISER_UPPER,
			{"mouthraiserupper", "mouthshrugupper", "chinraisert"}},
		{XRTracker::FT_MOUTH_RAISER_LOWER,
			{"mouthraiserlower", "mouthshruglower", "mouthloweroverlay", "chinraiserb"}},
		{XRTracker::FT_MOUTH_PRESS_RIGHT, {"mouthpressright", "lippressorr"}},
		{XRTracker::FT_MOUTH_PRESS_LEFT, {"mouthpressleft", "lippressorl"}},
		{XRTracker::FT_MOUTH_TIGHTENER_RIGHT, {"mouthtightenerright", "liptightenerr"}},
		{XRTracker::FT_MOUTH_TIGHTENER_LEFT, {"mouthtightenerleft", "liptightenerl"}},
		{XRTracker::FT_TONGUE_OUT, {"tongueout", "tonguelongstep2"}},
		{XRTracker::FT_TONGUE_UP, {"tongueup"}},
		{XRTracker::FT_TONGUE_DOWN, {"tonguedown"}},
		{XRTracker::FT_TONGUE_RIGHT, {"tongueright"}},
		{XRTracker::FT_TONGUE_LEFT, {"tongueleft"}},
		{XRTracker::FT_TONGUE_ROLL, {"tongueroll"}},
		{XRTracker::FT_TONGUE_BLEND_DOWN, {"tongueblenddown"}},
		{XRTracker::FT_TONGUE_CURL_UP, {"tonguecurlup"}},
		{XRTracker::FT_TONGUE_SQUISH, {"tonguesquish"}},
		{XRTracker::FT_TONGUE_FLAT, {"tongueflat"}},
		{XRTracker::FT_TONGUE_TWIST_RIGHT, {"tonguetwistright"}},
		{XRTracker::FT_TONGUE_TWIST_LEFT, {"tonguetwistleft"}},
		{XRTracker::FT_SOFT_PALATE_CLOSE, {"softpalateclose"}},
		{XRTracker::FT_THROAT_SWALLOW, {"throatswallow"}},
		{XRTracker::FT_NECK_FLEX_RIGHT, {"neckflexright"}},
		{XRTracker::FT_NECK_FLEX_LEFT, {"neckflexleft"}},
		{XRTracker::FT_EYE_CLOSED, {"eyeclosed"}}, {XRTracker::FT_EYE_WIDE, {"eyewide"}},
		{XRTracker::FT_EYE_SQUINT, {"eyesquint"}},
		{XRTracker::FT_EYE_DILATION, {"eyedilation"}},
		{XRTracker::FT_EYE_CONSTRICT, {"eyeconstrict"}},
		{XRTracker::FT_BROW_DOWN_RIGHT, {"browdownright", "browlowererr"}},
		{XRTracker::FT_BROW_DOWN_LEFT, {"browdownleft", "browlowererl"}},
		{XRTracker::FT_BROW_DOWN, {"browdown"}},
		{XRTracker::FT_BROW_UP_RIGHT, {"browupright"}},
		{XRTracker::FT_BROW_UP_LEFT, {"browupleft"}}, {XRTracker::FT_BROW_UP, {"browup"}},
		{XRTracker::FT_NOSE_SNEER, {"nosesneer"}},
		{XRTracker::FT_NASAL_DILATION, {"nasaldilation"}},
		{XRTracker::FT_NASAL_CONSTRICT, {"nasalconstrict"}},
		{XRTracker::FT_CHEEK_PUFF, {"cheekpuff"}},
		{XRTracker::FT_CHEEK_SUCK, {"cheeksuck"}},
		{XRTracker::FT_CHEEK_SQUINT, {"cheeksquint"}},
		{XRTracker::FT_LIP_SUCK_UPPER, {"lipsuckupper", "mouthrollupper", "mouthupperinside"}},
		{XRTracker::FT_LIP_SUCK_LOWER, {"lipsucklower", "mouthrolllower", "mouthlowerinside"}},
		{XRTracker::FT_LIP_SUCK, {"lipsuck"}},
		{XRTracker::FT_LIP_FUNNEL_UPPER, {"lipfunnelupper", "mouthupperoverturn"}},
		{XRTracker::FT_LIP_FUNNEL_LOWER, {"lipfunnellower", "mouthloweroverturn"}},
		{XRTracker::FT_LIP_FUNNEL, {"lipfunnel", "mouthfunnel"}},
		{XRTracker::FT_LIP_PUCKER_UPPER, {"lippuckerupper"}},
		{XRTracker::FT_LIP_PUCKER_LOWER, {"lippuckerlower"}},
		{XRTracker::FT_LIP_PUCKER, {"lippucker", "mouthpucker", "mouthpout"}},
		{XRTracker::FT_MOUTH_UPPER_UP, {"mouthupperup"}},
		{XRTracker::FT_MOUTH_LOWER_DOWN, {"mouthlowerdown"}},
		{XRTracker::FT_MOUTH_OPEN, {"mouthopen"}},
		{XRTracker::FT_MOUTH_RIGHT, {"mouthright"}},
		{XRTracker::FT_MOUTH_LEFT, {"mouthleft"}},
		{XRTracker::FT_MOUTH_SMILE_RIGHT, {"mouthsmileright", "lipcornerpullerr"}},
		{XRTracker::FT_MOUTH_SMILE_LEFT, {"mouthsmileleft", "lipcornerpullerl"}},
		{XRTracker::FT_MOUTH_SMILE, {"mouthsmile"}},
		{XRTracker::FT_MOUTH_SAD_RIGHT, {"mouthsadright"}},
		{XRTracker::FT_MOUTH_SAD_LEFT, {"mouthsadleft"}},
		{XRTracker::FT_MOUTH_SAD, {"mouthsad"}},
		{XRTracker::FT_MOUTH_STRETCH, {"mouthstretch"}},
		{XRTracker::FT_MOUTH_DIMPLE, {"mouthdimple"}},
		{XRTracker::FT_MOUTH_TIGHTENER, {"mouthtightener"}},
		{XRTracker::FT_MOUTH_PRESS, {"mouthpress"}}};

	// Convert the name to lower-case and strip non-alphanumeric characters.
	const String name = String(p_name).to_lower().remove_char('_');

	// Iterate through the blend map.
	for (const blend_map_entry& entry : blend_map) {
		for (const char* n : entry.name) {
			if (n == nullptr) {
				break;
			}

			if (name == n) {
				return entry.blend;
			}
		}
	}

	// Blend shape not found.
	return -1;
}

// This method adds all the identified XRFaceTracker blend shapes of
// the mesh to the p_blend_mapping map. The map is indexed by the
// XRFaceTracker blend shape, and the value is the index of the mesh
// blend shape.
static void identify_face_blend_shapes(RBMap<int, int>& p_blend_mapping, const Ref<Mesh>& mesh)
{
	// Find all blend shapes.
	const int count = mesh->get_blend_shape_count();
	for (int i = 0; i < count; i++) {
		const int blend = find_face_blend_shape(mesh->get_blend_shape_name(i));
		if (blend >= 0) {
			p_blend_mapping[blend] = i;
		}
	}
}

// This method removes any unified blend shapes from the p_blend_mapping map
// if all the individual blend shapes are found and going to be driven.
static void remove_driven_unified_blend_shapes(RBMap<int, int>& p_blend_mapping)
{
	// Entry for unified blend table.
	struct unified_blend_entry
	{
		int unified;
		int individual[4];
	};

	// Table of unified blend shapes.
	//
	// This table consists of:
	// - The XRFaceTracker unified blend shape.
	// - The individual blend shapes that make up the unified blend shape.
	static constexpr unified_blend_entry unified_blends[] = {
		{XRTracker::FT_EYE_CLOSED,
			{XRTracker::FT_EYE_CLOSED_RIGHT, XRTracker::FT_EYE_CLOSED_LEFT, -1, -1}},
		{XRTracker::FT_EYE_WIDE,
			{XRTracker::FT_EYE_WIDE_RIGHT, XRTracker::FT_EYE_WIDE_LEFT, -1, -1}},
		{XRTracker::FT_EYE_SQUINT,
			{XRTracker::FT_EYE_SQUINT_RIGHT, XRTracker::FT_EYE_SQUINT_LEFT, -1, -1}},
		{XRTracker::FT_EYE_DILATION,
			{XRTracker::FT_EYE_DILATION_RIGHT, XRTracker::FT_EYE_DILATION_LEFT, -1, -1}},
		{XRTracker::FT_EYE_CONSTRICT,
			{XRTracker::FT_EYE_CONSTRICT_RIGHT, XRTracker::FT_EYE_CONSTRICT_LEFT, -1, -1}},
		{XRTracker::FT_BROW_DOWN_RIGHT,
			{XRTracker::FT_BROW_LOWERER_RIGHT, XRTracker::FT_BROW_PINCH_RIGHT, -1, -1}},
		{XRTracker::FT_BROW_DOWN_LEFT,
			{XRTracker::FT_BROW_LOWERER_LEFT, XRTracker::FT_BROW_PINCH_LEFT, -1, -1}},
		{XRTracker::FT_BROW_DOWN,
			{XRTracker::FT_BROW_LOWERER_RIGHT, XRTracker::FT_BROW_PINCH_RIGHT,
				XRTracker::FT_BROW_LOWERER_LEFT, XRTracker::FT_BROW_PINCH_LEFT}},
		{XRTracker::FT_BROW_UP_RIGHT,
			{XRTracker::FT_BROW_INNER_UP_RIGHT, XRTracker::FT_BROW_OUTER_UP_RIGHT, -1, -1}},
		{XRTracker::FT_BROW_UP_LEFT,
			{XRTracker::FT_BROW_INNER_UP_LEFT, XRTracker::FT_BROW_OUTER_UP_LEFT, -1, -1}},
		{XRTracker::FT_BROW_UP,
			{XRTracker::FT_BROW_INNER_UP_RIGHT, XRTracker::FT_BROW_OUTER_UP_RIGHT,
				XRTracker::FT_BROW_INNER_UP_LEFT, XRTracker::FT_BROW_OUTER_UP_LEFT}},
		{XRTracker::FT_NOSE_SNEER,
			{XRTracker::FT_NOSE_SNEER_RIGHT, XRTracker::FT_NOSE_SNEER_LEFT, -1, -1}},
		{XRTracker::FT_NASAL_DILATION, {XRTracker::FT_NASAL_DILATION_RIGHT,
											   XRTracker::FT_NASAL_DILATION_LEFT, -1, -1}},
		{XRTracker::FT_NASAL_CONSTRICT, {XRTracker::FT_NASAL_CONSTRICT_RIGHT,
												XRTracker::FT_NASAL_CONSTRICT_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_PUFF,
			{XRTracker::FT_CHEEK_PUFF_RIGHT, XRTracker::FT_CHEEK_PUFF_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_SUCK,
			{XRTracker::FT_CHEEK_SUCK_RIGHT, XRTracker::FT_CHEEK_SUCK_LEFT, -1, -1}},
		{XRTracker::FT_CHEEK_SQUINT,
			{XRTracker::FT_CHEEK_SQUINT_RIGHT, XRTracker::FT_CHEEK_SQUINT_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK_UPPER, {XRTracker::FT_LIP_SUCK_UPPER_RIGHT,
											   XRTracker::FT_LIP_SUCK_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK_LOWER, {XRTracker::FT_LIP_SUCK_LOWER_RIGHT,
											   XRTracker::FT_LIP_SUCK_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_SUCK,
			{XRTracker::FT_LIP_SUCK_UPPER_RIGHT, XRTracker::FT_LIP_SUCK_UPPER_LEFT,
				XRTracker::FT_LIP_SUCK_LOWER_RIGHT, XRTracker::FT_LIP_SUCK_LOWER_LEFT}},
		{XRTracker::FT_LIP_FUNNEL_UPPER, {XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT,
												 XRTracker::FT_LIP_FUNNEL_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_FUNNEL_LOWER, {XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT,
												 XRTracker::FT_LIP_FUNNEL_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_FUNNEL,
			{XRTracker::FT_LIP_FUNNEL_UPPER_RIGHT, XRTracker::FT_LIP_FUNNEL_UPPER_LEFT,
				XRTracker::FT_LIP_FUNNEL_LOWER_RIGHT, XRTracker::FT_LIP_FUNNEL_LOWER_LEFT}},
		{XRTracker::FT_LIP_PUCKER_UPPER, {XRTracker::FT_LIP_PUCKER_UPPER_RIGHT,
												 XRTracker::FT_LIP_PUCKER_UPPER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_PUCKER_LOWER, {XRTracker::FT_LIP_PUCKER_LOWER_RIGHT,
												 XRTracker::FT_LIP_PUCKER_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_LIP_PUCKER,
			{XRTracker::FT_LIP_PUCKER_UPPER_RIGHT, XRTracker::FT_LIP_PUCKER_UPPER_LEFT,
				XRTracker::FT_LIP_PUCKER_LOWER_RIGHT, XRTracker::FT_LIP_PUCKER_LOWER_LEFT}},
		{XRTracker::FT_MOUTH_UPPER_UP, {XRTracker::FT_MOUTH_UPPER_UP_RIGHT,
											   XRTracker::FT_MOUTH_UPPER_UP_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_LOWER_DOWN, {XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT,
												 XRTracker::FT_MOUTH_LOWER_DOWN_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_OPEN,
			{XRTracker::FT_MOUTH_UPPER_UP_RIGHT, XRTracker::FT_MOUTH_UPPER_UP_LEFT,
				XRTracker::FT_MOUTH_LOWER_DOWN_RIGHT, XRTracker::FT_MOUTH_LOWER_DOWN_LEFT}},
		{XRTracker::FT_MOUTH_RIGHT,
			{XRTracker::FT_MOUTH_UPPER_RIGHT, XRTracker::FT_MOUTH_LOWER_RIGHT, -1, -1}},
		{XRTracker::FT_MOUTH_LEFT,
			{XRTracker::FT_MOUTH_UPPER_LEFT, XRTracker::FT_MOUTH_LOWER_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_SMILE_RIGHT,
			{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT,
				-1, -1}},
		{XRTracker::FT_MOUTH_SMILE_LEFT,
			{XRTracker::FT_MOUTH_CORNER_PULL_LEFT, XRTracker::FT_MOUTH_CORNER_SLANT_LEFT,
				-1, -1}},
		{XRTracker::FT_MOUTH_SMILE,
			{XRTracker::FT_MOUTH_CORNER_PULL_RIGHT, XRTracker::FT_MOUTH_CORNER_SLANT_RIGHT,
				XRTracker::FT_MOUTH_CORNER_PULL_LEFT,
				XRTracker::FT_MOUTH_CORNER_SLANT_LEFT}},
		{XRTracker::FT_MOUTH_SAD_RIGHT,
			{XRTracker::FT_MOUTH_FROWN_RIGHT, XRTracker::FT_MOUTH_STRETCH_RIGHT, -1, -1}},
		{XRTracker::FT_MOUTH_SAD_LEFT,
			{XRTracker::FT_MOUTH_FROWN_LEFT, XRTracker::FT_MOUTH_STRETCH_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_SAD,
			{XRTracker::FT_MOUTH_FROWN_RIGHT, XRTracker::FT_MOUTH_STRETCH_RIGHT,
				XRTracker::FT_MOUTH_FROWN_LEFT, XRTracker::FT_MOUTH_STRETCH_LEFT}},
		{XRTracker::FT_MOUTH_STRETCH,
			{XRTracker::FT_MOUTH_STRETCH_RIGHT, XRTracker::FT_MOUTH_STRETCH_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_DIMPLE,
			{XRTracker::FT_MOUTH_DIMPLE_RIGHT, XRTracker::FT_MOUTH_DIMPLE_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_TIGHTENER, {XRTracker::FT_MOUTH_TIGHTENER_RIGHT,
												XRTracker::FT_MOUTH_TIGHTENER_LEFT, -1, -1}},
		{XRTracker::FT_MOUTH_PRESS,
			{XRTracker::FT_MOUTH_PRESS_RIGHT, XRTracker::FT_MOUTH_PRESS_LEFT, -1, -1}}};

	// Remove unified blend shapes if individual blend shapes are found.
	for (const unified_blend_entry& entry : unified_blends) {
		// Check if all individual blend shapes are found.
		bool found = true;
		for (const int i : entry.individual) {
			if (i >= 0 && !p_blend_mapping.find(i)) {
				found = false;
				break;
			}
		}

		// If all individual blend shapes are found then remove the unified blend shape.
		if (found) {
			p_blend_mapping.erase(entry.unified);
		}
	}
}


void XRFaceModifier3D::set_face_tracker(const String& p_tracker_name)
{
	tracker_name = p_tracker_name;
	if (is_inside_tree()) {
		_get_blend_data();
	}
}

String XRFaceModifier3D::get_face_tracker() const { return tracker_name; }

void XRFaceModifier3D::set_target(const NodePath& p_target)
{
	target = p_target;

	if (is_inside_tree()) {
		_get_blend_data();
	}
}

NodePath XRFaceModifier3D::get_target() const { return target; }

void XRFaceModifier3D::_get_blend_data()
{
	// This method constructs the blend mapping from the XRFaceTracker
	// blend shapes to the available blend shapes of the target mesh. It does this
	// by:
	//
	// 1. Identifying the blend shapes of the target mesh and identifying what
	//    XRFaceTracker blend shape they correspond to. The results are
	//    placed in the blend_mapping map.
	// 2. Prevent over-driving facial blend-shapes by removing any unified blend
	//    shapes from the map if all the individual blend shapes are already
	//    found and going to be driven.

	blend_mapping.clear();

	// Get the target MeshInstance3D.
	const MeshInstance3D* mesh_instance = get_mesh_instance();
	if (!mesh_instance) {
		return;
	}

	// Get the mesh.
	const Ref<Mesh> mesh = mesh_instance->get_mesh();
	if (mesh.is_null()) {
		return;
	}

	// Identify all face blend shapes and populate the map.
	identify_face_blend_shapes(blend_mapping, mesh);

	// Remove the unified blend shapes if all the individual blend shapes are found.
	remove_driven_unified_blend_shapes(blend_mapping);
}

void XRFaceModifier3D::_update_face_blends() const
{
	// Get the XR Server.
	if (!XRServer::is_initialized()) {
		return;
	}

	// Get the face tracker.
	const Ref<XRTracker> tracker = XRServer::get_tracker(tracker_name);
	if (tracker.is_null() || tracker->get_tracker_type() != XRServer::TRACKER_FACE) {
		return;
	}

	// Get the face mesh.
	MeshInstance3D* mesh_instance = get_mesh_instance();
	if (!mesh_instance) {
		return;
	}

	// Get the blend weights.
	const PackedFloat32Array weights = tracker->get_blend_shapes();

	// Apply all the face blend weights to the mesh.
	for (const KeyValue<int, int>& it : blend_mapping) {
		mesh_instance->set_blend_shape_value(it.value, weights[it.key]);
	}
}

void XRFaceModifier3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_blend_data();
		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);
		blend_mapping.clear();
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_face_blends();
	} break;
	default: {
	} break;
	}
}



MeshInstance3D* XRFaceModifier3D::get_mesh_instance() const { return nullptr; }
#endif // !defined(_3D_DISABLED) && !defined(XR_DISABLED)
