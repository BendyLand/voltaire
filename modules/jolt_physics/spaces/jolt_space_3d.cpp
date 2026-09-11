/**************************************************************************/
/*  jolt_space_3d.cpp                                                     */
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

#include "../joints/jolt_joint_3d.h"
#include "../jolt_physics_server_3d.h"
#include "../misc/jolt_stream_wrappers.h"
#include "../objects/jolt_area_3d.h"
#include "../objects/jolt_body_3d.h"
#include "../shapes/jolt_shape_3d.h"
#include "core/io/file_access.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "jolt_body_activation_listener_3d.h"
#include "jolt_contact_listener_3d.h"
#include "jolt_layers.h"
#include "jolt_physics_direct_space_state_3d.h"
#include "jolt_space_3d.h"
// must stay last
#include <Jolt/Physics/Collision/CollideShapeVsShapePerLeaf.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/PhysicsScene.h>

namespace
{

constexpr double SPACE_DEFAULT_CONTACT_RECYCLE_RADIUS = 0.01;
constexpr double SPACE_DEFAULT_CONTACT_MAX_SEPARATION = 0.05;
constexpr double SPACE_DEFAULT_CONTACT_MAX_ALLOWED_PENETRATION = 0.01;
constexpr double SPACE_DEFAULT_CONTACT_DEFAULT_BIAS = 0.8;
constexpr double SPACE_DEFAULT_SLEEP_THRESHOLD_LINEAR = 0.1;
constexpr double SPACE_DEFAULT_SLEEP_THRESHOLD_ANGULAR = 8.0 * Math::PI / 180;
constexpr double SPACE_DEFAULT_SOLVER_ITERATIONS = 8;

} // namespace

void JoltSpace3D::_pre_step(float p_step)
{
	flush_pending_objects();

	while (needs_optimization_list.first()) {
		JoltShapedObject3D* object = needs_optimization_list.first()->self();
		needs_optimization_list.remove(needs_optimization_list.first());
		object->commit_shapes(true);
	}

	contact_listener->pre_step();

	const JPH::BodyLockInterface& lock_iface = get_lock_iface();
	const JPH::BodyID* active_rigid_bodies =
		physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::RigidBody);
	const JPH::uint32 active_rigid_body_count =
		physics_system->GetNumActiveBodies(JPH::EBodyType::RigidBody);

	for (JPH::uint32 i = 0; i < active_rigid_body_count; i++) {
		JPH::Body* jolt_body = lock_iface.TryGetBody(active_rigid_bodies[i]);
		JoltObject3D* object = reinterpret_cast<JoltObject3D*>(jolt_body->GetUserData());
		object->pre_step(p_step);
	}

	const JPH::BodyID* active_soft_bodies =
		physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::SoftBody);
	const JPH::uint32 active_soft_body_count =
		physics_system->GetNumActiveBodies(JPH::EBodyType::SoftBody);

	for (JPH::uint32 i = 0; i < active_soft_body_count; i++) {
		JPH::Body* jolt_body = lock_iface.TryGetBody(active_soft_bodies[i]);
		JoltObject3D* object = reinterpret_cast<JoltObject3D*>(jolt_body->GetUserData());
		object->pre_step(p_step);
	}

	physics_system->SetBodyActivationListener(body_activation_listener);
}

void JoltSpace3D::_post_step(float p_step)
{
	// We only want a listener during the step, as it will otherwise be called when pending bodies
	// are flushed, which causes issues (e.g. GH-115322).
	physics_system->SetBodyActivationListener(nullptr);

	contact_listener->post_step();

	while (shapes_changed_list.first()) {
		JoltShapedObject3D* object = shapes_changed_list.first()->self();
		shapes_changed_list.remove(shapes_changed_list.first());
		object->clear_previous_shape();
	}
}

JoltSpace3D::~JoltSpace3D()
{
	if (direct_state != nullptr) {
		memdelete(direct_state);
		direct_state = nullptr;
	}

	if (physics_system != nullptr) {
		delete physics_system;
		physics_system = nullptr;
	}

	if (body_activation_listener != nullptr) {
		delete body_activation_listener;
		body_activation_listener = nullptr;
	}

	if (contact_listener != nullptr) {
		delete contact_listener;
		contact_listener = nullptr;
	}

	if (layers != nullptr) {
		delete layers;
		layers = nullptr;
	}
}

void JoltSpace3D::call_queries()
{
	while (body_call_queries_list.first()) {
		JoltBody3D* body = body_call_queries_list.first()->self();
		body_call_queries_list.remove(body_call_queries_list.first());
		body->call_queries();
	}

	while (area_call_queries_list.first()) {
		JoltArea3D* body = area_call_queries_list.first()->self();
		area_call_queries_list.remove(area_call_queries_list.first());
		body->call_queries();
	}
}

double JoltSpace3D::get_param(PS3DE::SpaceParameter p_param) const
{
	switch (p_param) {
	case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS: {
		return SPACE_DEFAULT_CONTACT_RECYCLE_RADIUS;
	}
	case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION: {
		return SPACE_DEFAULT_CONTACT_MAX_SEPARATION;
	}
	case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION: {
		return SPACE_DEFAULT_CONTACT_MAX_ALLOWED_PENETRATION;
	}
	case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS: {
		return SPACE_DEFAULT_CONTACT_DEFAULT_BIAS;
	}
	case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD: {
		return SPACE_DEFAULT_SLEEP_THRESHOLD_LINEAR;
	}
	case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD: {
		return SPACE_DEFAULT_SLEEP_THRESHOLD_ANGULAR;
	}
	case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS: {
		return SPACE_DEFAULT_SOLVER_ITERATIONS;
	}
	default: {
		ERR_FAIL_V_MSG(0.0,
			vformat("Unhandled space parameter: '%d'. This should not happen. Please report this.",
				p_param));
	}
	}
}

void JoltSpace3D::set_param(PS3DE::SpaceParameter p_param, double p_value)
{
	switch (p_param) {
	case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS: {
		WARN_PRINT("Space-specific contact recycle radius is not supported when using Jolt "
				   "Physics. Any such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION: {
		WARN_PRINT("Space-specific contact max separation is not supported when using Jolt "
				   "Physics. Any such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION: {
		WARN_PRINT("Space-specific contact max allowed penetration is not supported when using "
				   "Jolt Physics. Any such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS: {
		WARN_PRINT("Space-specific contact default bias is not supported when using Jolt Physics. "
				   "Any such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD: {
		WARN_PRINT("Space-specific linear velocity sleep threshold is not supported when using "
				   "Jolt Physics. Any such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD: {
		WARN_PRINT("Space-specific angular velocity sleep threshold is not supported when using "
				   "Jolt Physics. Any such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_BODY_TIME_TO_SLEEP: {
		WARN_PRINT("Space-specific body sleep time is not supported when using Jolt Physics. Any "
				   "such value will be ignored.");
	} break;
	case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS: {
		WARN_PRINT("Space-specific solver iterations is not supported when using Jolt Physics. Any "
				   "such value will be ignored.");
	} break;
	default: {
		ERR_FAIL_MSG(
			vformat("Unhandled space parameter: '%d'. This should not happen. Please report this.",
				p_param));
	} break;
	}
}

JPH::BodyInterface& JoltSpace3D::get_body_iface()
{
	return physics_system->GetBodyInterfaceNoLock();
}

const JPH::BodyInterface& JoltSpace3D::get_body_iface() const
{
	return physics_system->GetBodyInterfaceNoLock();
}

const JPH::BodyLockInterface& JoltSpace3D::get_lock_iface() const
{
	return physics_system->GetBodyLockInterfaceNoLock();
}

const JPH::BroadPhaseQuery& JoltSpace3D::get_broad_phase_query() const
{
	return physics_system->GetBroadPhaseQuery();
}

const JPH::NarrowPhaseQuery& JoltSpace3D::get_narrow_phase_query() const
{
	return physics_system->GetNarrowPhaseQueryNoLock();
}

JPH::ObjectLayer JoltSpace3D::map_to_object_layer(
	JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask)
{
	return layers->to_object_layer(p_broad_phase_layer, p_collision_layer, p_collision_mask);
}

void JoltSpace3D::map_from_object_layer(JPH::ObjectLayer p_object_layer,
	JPH::BroadPhaseLayer& r_broad_phase_layer, uint32_t& r_collision_layer,
	uint32_t& r_collision_mask) const
{
	layers->from_object_layer(
		p_object_layer, r_broad_phase_layer, r_collision_layer, r_collision_mask);
}

JPH::Body* JoltSpace3D::try_get_jolt_body(const JPH::BodyID& p_body_id) const
{
	return get_lock_iface().TryGetBody(p_body_id);
}

JoltObject3D* JoltSpace3D::try_get_object(const JPH::BodyID& p_body_id) const
{
	const JPH::Body* jolt_body = try_get_jolt_body(p_body_id);
	if (unlikely(jolt_body == nullptr)) {
		return nullptr;
	}

	return reinterpret_cast<JoltObject3D*>(jolt_body->GetUserData());
}

JoltShapedObject3D* JoltSpace3D::try_get_shaped(const JPH::BodyID& p_body_id) const
{
	JoltObject3D* object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_shaped();
}

JoltBody3D* JoltSpace3D::try_get_body(const JPH::BodyID& p_body_id) const
{
	JoltObject3D* object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_body();
}

JoltArea3D* JoltSpace3D::try_get_area(const JPH::BodyID& p_body_id) const
{
	JoltObject3D* object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_area();
}

JoltSoftBody3D* JoltSpace3D::try_get_soft_body(const JPH::BodyID& p_body_id) const
{
	JoltObject3D* object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_soft_body();
}

JoltPhysicsDirectSpaceState3D* JoltSpace3D::get_direct_state()
{
	if (direct_state == nullptr) {
		direct_state = memnew(JoltPhysicsDirectSpaceState3D(this));
	}

	return direct_state;
}

void JoltSpace3D::remove_object(const JPH::BodyID& p_jolt_id)
{
	JPH::BodyInterface& body_iface = get_body_iface();

	if (!pending_objects_sleeping.erase_unordered(p_jolt_id) &&
		!pending_objects_awake.erase_unordered(p_jolt_id)) {
		body_iface.RemoveBody(p_jolt_id);
	}

	body_iface.DestroyBody(p_jolt_id);

	// If we're never going to step this space, like in the editor viewport, we need to manually
	// clean up Jolt's broad phase instead, otherwise performance can degrade when doing things like
	// switching scenes. We'll never actually have zero bodies in any space though, since we always
	// have the default area, so we check if there's one or fewer left instead.
	if (!JoltPhysicsServer3D::get_singleton()->is_active() && physics_system->GetNumBodies() <= 1) {
		physics_system->OptimizeBroadPhase();
	}
}

void JoltSpace3D::flush_pending_objects()
{
	if (pending_objects_sleeping.is_empty() && pending_objects_awake.is_empty()) {
		return;
	}

	// We only care about locking within this method, because it's called when performing queries,
	// which aren't covered by `PhysicsServer3DWrapMT`.
	MutexLock pending_objects_lock(pending_objects_mutex);

	JPH::BodyInterface& body_iface = get_body_iface();

	if (!pending_objects_sleeping.is_empty()) {
		JPH::BodyInterface::AddState add_state = body_iface.AddBodiesPrepare(
			pending_objects_sleeping.ptr(), pending_objects_sleeping.size());
		body_iface.AddBodiesFinalize(pending_objects_sleeping.ptr(),
			pending_objects_sleeping.size(), add_state, JPH::EActivation::DontActivate);
		pending_objects_sleeping.reset();
	}

	if (!pending_objects_awake.is_empty()) {
		JPH::BodyInterface::AddState add_state =
			body_iface.AddBodiesPrepare(pending_objects_awake.ptr(), pending_objects_awake.size());
		body_iface.AddBodiesFinalize(pending_objects_awake.ptr(), pending_objects_awake.size(),
			add_state, JPH::EActivation::Activate);
		pending_objects_awake.reset();
	}
}

void JoltSpace3D::set_is_object_sleeping(const JPH::BodyID& p_jolt_id, bool p_enable)
{
	if (p_enable) {
		if (pending_objects_awake.erase_unordered(p_jolt_id)) {
			pending_objects_sleeping.push_back(p_jolt_id);
		}
		else if (pending_objects_sleeping.has(p_jolt_id)) {
			// Do nothing.
		}
		else {
			get_body_iface().DeactivateBody(p_jolt_id);
		}
	}
	else {
		if (pending_objects_sleeping.erase_unordered(p_jolt_id)) {
			pending_objects_awake.push_back(p_jolt_id);
		}
		else if (pending_objects_awake.has(p_jolt_id)) {
			// Do nothing.
		}
		else {
			get_body_iface().ActivateBody(p_jolt_id);
		}
	}
}

void JoltSpace3D::enqueue_call_queries(SelfList<JoltBody3D>* p_body)
{
	// This method will be called from the body activation listener on multiple threads during the
	// simulation step.
	MutexLock body_call_queries_lock(body_call_queries_mutex);

	if (!p_body->in_list()) {
		body_call_queries_list.add(p_body);
	}
}

void JoltSpace3D::enqueue_call_queries(SelfList<JoltArea3D>* p_area)
{
	if (!p_area->in_list()) {
		area_call_queries_list.add(p_area);
	}
}

void JoltSpace3D::dequeue_call_queries(SelfList<JoltBody3D>* p_body)
{
	if (p_body->in_list()) {
		body_call_queries_list.remove(p_body);
	}
}

void JoltSpace3D::dequeue_call_queries(SelfList<JoltArea3D>* p_area)
{
	if (p_area->in_list()) {
		area_call_queries_list.remove(p_area);
	}
}

void JoltSpace3D::enqueue_shapes_changed(SelfList<JoltShapedObject3D>* p_object)
{
	if (!p_object->in_list()) {
		shapes_changed_list.add(p_object);
	}
}

void JoltSpace3D::dequeue_shapes_changed(SelfList<JoltShapedObject3D>* p_object)
{
	if (p_object->in_list()) {
		shapes_changed_list.remove(p_object);
	}
}

void JoltSpace3D::enqueue_needs_optimization(SelfList<JoltShapedObject3D>* p_object)
{
	if (!p_object->in_list()) {
		needs_optimization_list.add(p_object);
	}
}

void JoltSpace3D::dequeue_needs_optimization(SelfList<JoltShapedObject3D>* p_object)
{
	if (p_object->in_list()) {
		needs_optimization_list.remove(p_object);
	}
}

void JoltSpace3D::add_joint(JPH::Constraint* p_jolt_ref)
{
	physics_system->AddConstraint(p_jolt_ref);
}

void JoltSpace3D::add_joint(JoltJoint3D* p_joint) { add_joint(p_joint->get_jolt_ref()); }

void JoltSpace3D::remove_joint(JPH::Constraint* p_jolt_ref)
{
	physics_system->RemoveConstraint(p_jolt_ref);
}

void JoltSpace3D::remove_joint(JoltJoint3D* p_joint) { remove_joint(p_joint->get_jolt_ref()); }

#ifdef DEBUG_ENABLED

const PackedVector3Array& JoltSpace3D::get_debug_contacts() const
{
	return contact_listener->get_debug_contacts();
}

int JoltSpace3D::get_debug_contact_count() const
{
	return contact_listener->get_debug_contact_count();
}

int JoltSpace3D::get_max_debug_contacts() const
{
	return contact_listener->get_max_debug_contacts();
}

void JoltSpace3D::set_max_debug_contacts(int p_count)
{
	contact_listener->set_max_debug_contacts(p_count);
}

#endif


