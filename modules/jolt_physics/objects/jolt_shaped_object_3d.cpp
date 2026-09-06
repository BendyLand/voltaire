/**************************************************************************/
/*  jolt_shaped_object_3d.cpp                                             */
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

#include "../misc/jolt_math_funcs.h"
#include "../misc/jolt_type_conversions.h"
#include "../shapes/jolt_shape_3d.h"
#include "../spaces/jolt_space_3d.h"
#include "jolt_shaped_object_3d.h"
// must stay last
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

bool JoltShapedObject3D::_is_big() const
{
	// This number is completely arbitrary, and mostly just needs to capture `WorldBoundaryShape3D`,
	// which needs to be kept out of the normal broadphase layers.
	return get_aabb().get_longest_axis_size() >= 1000.0f;
}

void JoltShapedObject3D::_enqueue_shapes_changed()
{
	if (space != nullptr) {
		space->enqueue_shapes_changed(&shapes_changed_element);
	}
}

void JoltShapedObject3D::_dequeue_shapes_changed()
{
	if (space != nullptr) {
		space->dequeue_shapes_changed(&shapes_changed_element);
	}
}

void JoltShapedObject3D::_enqueue_needs_optimization()
{
	if (space != nullptr) {
		space->enqueue_needs_optimization(&needs_optimization_element);
	}
}

void JoltShapedObject3D::_dequeue_needs_optimization()
{
	if (space != nullptr) {
		space->dequeue_needs_optimization(&needs_optimization_element);
	}
}

void JoltShapedObject3D::_shapes_changed() { commit_shapes(false); }

void JoltShapedObject3D::_shapes_committed() { _update_object_layer(); }

void JoltShapedObject3D::_space_changing()
{
	JoltObject3D::_space_changing();

	_dequeue_shapes_changed();
	_dequeue_needs_optimization();

	previous_jolt_shape = nullptr;

	if (in_space()) {
		jolt_settings = new JPH::BodyCreationSettings(jolt_body->GetBodyCreationSettings());
	}
}

JoltShapedObject3D::JoltShapedObject3D(ObjectType p_object_type)
	: JoltObject3D(p_object_type), shapes_changed_element(this), needs_optimization_element(this)
{
	jolt_settings->mAllowSleeping = true;
	jolt_settings->mFriction = 1.0f;
	jolt_settings->mRestitution = 0.0f;
	jolt_settings->mLinearDamping = 0.0f;
	jolt_settings->mAngularDamping = 0.0f;
}

JoltShapedObject3D::~JoltShapedObject3D()
{
	if (jolt_settings != nullptr) {
		delete jolt_settings;
		jolt_settings = nullptr;
	}
}

Transform3D JoltShapedObject3D::get_transform_unscaled() const
{
	if (!in_space()) {
		return Transform3D(to_godot(jolt_settings->mRotation), to_godot(jolt_settings->mPosition));
	}
	else {
		return Transform3D(to_godot(jolt_body->GetRotation()), to_godot(jolt_body->GetPosition()));
	}
}

Transform3D JoltShapedObject3D::get_transform_scaled() const
{
	return get_transform_unscaled().scaled_local(scale);
}

Basis JoltShapedObject3D::get_basis() const
{
	if (!in_space()) {
		return to_godot(jolt_settings->mRotation);
	}
	else {
		return to_godot(jolt_body->GetRotation());
	}
}

Vector3 JoltShapedObject3D::get_position() const
{
	if (!in_space()) {
		return to_godot(jolt_settings->mPosition);
	}
	else {
		return to_godot(jolt_body->GetPosition());
	}
}

Vector3 JoltShapedObject3D::get_center_of_mass() const
{
	ERR_FAIL_COND_V_MSG(!in_space(), Vector3(),
		vformat("Failed to retrieve center-of-mass of '%s'. Doing so without a physics space is "
				"not supported when using Jolt Physics. If this relates to a node, try adding the "
				"node to a scene tree first.",
			to_string()));
	return to_godot(jolt_body->GetCenterOfMassPosition());
}

Vector3 JoltShapedObject3D::get_center_of_mass_relative() const
{
	return get_center_of_mass() - get_position();
}

Vector3 JoltShapedObject3D::get_center_of_mass_local() const
{
	ERR_FAIL_NULL_V_MSG(space, Vector3(),
		vformat("Failed to retrieve local center-of-mass of '%s'. Doing so without a physics space "
				"is not supported when using Jolt Physics. If this relates to a node, try adding "
				"the node to a scene tree first.",
			to_string()));

	return get_transform_scaled().xform_inv(get_center_of_mass());
}

Vector3 JoltShapedObject3D::get_linear_velocity() const
{
	if (!in_space()) {
		return to_godot(jolt_settings->mLinearVelocity);
	}
	else {
		return to_godot(jolt_body->GetLinearVelocity());
	}
}

Vector3 JoltShapedObject3D::get_angular_velocity() const
{
	if (!in_space()) {
		return to_godot(jolt_settings->mAngularVelocity);
	}
	else {
		return to_godot(jolt_body->GetAngularVelocity());
	}
}

JPH::ShapeRefC JoltShapedObject3D::build_shapes(bool p_optimize_compound)
{
	JPH::ShapeRefC new_shape = _try_build_shape(p_optimize_compound);

	if (new_shape == nullptr) {
		if (has_custom_center_of_mass()) {
			new_shape = new JPH::EmptyShape(to_jolt(get_center_of_mass_custom()));
		}
		else {
			new_shape = new JPH::EmptyShape();
		}
	}

	return new_shape;
}

void JoltShapedObject3D::commit_shapes(bool p_optimize_compound)
{
	if (!in_space()) {
		_shapes_committed();
		return;
	}

	JPH::ShapeRefC new_shape = build_shapes(p_optimize_compound);
	if (new_shape == jolt_shape) {
		return;
	}

	if (previous_jolt_shape == nullptr) {
		previous_jolt_shape = jolt_shape;
	}

	jolt_shape = new_shape;

	space->get_body_iface().SetShape(
		jolt_body->GetID(), jolt_shape, false, JPH::EActivation::DontActivate);

	_enqueue_shapes_changed();

	if (!p_optimize_compound && jolt_shape->GetType() == JPH::EShapeType::Compound) {
		_enqueue_needs_optimization();
	}
	else {
		_dequeue_needs_optimization();
	}

	_shapes_committed();
}

void JoltShapedObject3D::clear_previous_shape() { previous_jolt_shape = nullptr; }

int JoltShapedObject3D::find_shape_index(const JPH::SubShapeID& p_sub_shape_id) const
{
	ERR_FAIL_NULL_V(jolt_shape, -1);
	return find_shape_index((uint32_t)jolt_shape->GetSubShapeUserData(p_sub_shape_id));
}


