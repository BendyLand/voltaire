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
#include "core/math/transform_interpolator.h"
#include "node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/property_utils.h"
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

#include "camera_3d.h"
#include "core/config/engine.h"
#include "core/math/projection.h"
#include "core/math/transform_interpolator.h"
#include "scene/3d/xr/xr_nodes.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_interface.h"

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
#include "path_3d.h"
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

#include "scene/3d/camera_3d.h"

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

#include "scene/3d/camera_3d.h"
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

#include "scene/3d/camera_3d.h"

bool AudioStreamPlayer3D::_is_autoplay_enabled_bind_compat_86907() { return is_autoplay_enabled(); }

void AudioStreamPlayer3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/3d/camera_3d.h"
#include "core/config/project_settings.h"
#include "scene/3d/camera_3d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/main/viewport.h"
#include "servers/audio/audio_stream.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/physics/collision_object_3d.h"
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
#include "scene/3d/camera_3d.h"

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

#include "scene/3d/skeleton_modifier_3d.h"
// --- skeleton_3d.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "skeleton_3d.h"

void Skeleton3D::_add_bone_bind_compat_88791(const String& p_name) { add_bone(p_name); }

void Skeleton3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "skeleton_3d.h"
#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
#include "scene/3d/physics/collision_object_3d.h"
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
#include "skeleton_modifier_3d.h"

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

#include "bone_attachment_3d.h"

void BoneAttachment3D::_on_bone_pose_update_bind_compat_90575(int p_bone_index)
{
	return on_skeleton_update();
}

void BoneAttachment3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "bone_attachment_3d.h"
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
#include "scene/3d/skeleton_modifier_3d.h"
#include "scene/animation/tween.h"

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
#include "scene/3d/skeleton_modifier_3d.h"
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
#include "scene/3d/skeleton_modifier_3d.h"
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

#include "scene/3d/skeleton_modifier_3d.h"
// --- Inlined from scene/3d/ik_modifier_3d.h ---
#include "scene/3d/skeleton_modifier_3d.h"

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
#include "scene/3d/path_3d.h"

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
