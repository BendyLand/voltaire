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

#include "marker_3d.h"

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

#include "remote_transform_3d.h"

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

#include "velocity_tracker_3d.h"

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

#include "audio_listener_3d.h"
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

#include "audio_stream_player_3d.h"

bool AudioStreamPlayer3D::_is_autoplay_enabled_bind_compat_86907() { return is_autoplay_enabled(); }

void AudioStreamPlayer3D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "audio_stream_player_3d.h"
#include "core/config/project_settings.h"
#include "scene/3d/audio_listener_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/velocity_tracker_3d.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/main/viewport.h"
#include "servers/audio/audio_stream.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/physics/area_3d.h"
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
#include "world_environment.h"

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
