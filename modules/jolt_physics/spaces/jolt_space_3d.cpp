/**************************************************************************/
/*  jolt_space_3d.cpp                                                     */
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

/*
Adapted to Godot from the Jolt Physics library.
Copyright 2021 Jorrit Rouwe
*/

#include "jolt_space_3d.h"

#include "../joints/jolt_joint_3d.h"
#include "../jolt_globals.h"
#include "../jolt_physics_server_3d.h"
#include "../objects/jolt_objects_3d.h"
#include "../shapes/jolt_custom_shapes.h"
#include "../shapes/jolt_shape_3d.h"

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/print_string.h"
#include "servers/physics_3d/direct_states/physics_direct_space_state_3d.h"
#include "servers/physics_3d/physics_server_3d.h"

// must stay last
#include <Jolt/Jolt.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Geometry/GJKClosestPoint.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollidePointResult.h>
#include <Jolt/Physics/Collision/CollideShapeVsShapePerLeaf.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/EstimateCollisionResponse.h>
#include <Jolt/Physics/Collision/ManifoldBetweenTwoFaces.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsScene.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyManifold.h>

// ============================================================================
// JoltTempAllocator
// ============================================================================

namespace {

template <typename TValue, typename TAlignment>
constexpr TValue align_up(TValue p_value, TAlignment p_alignment) {
	return (p_value + p_alignment - 1) & ~(p_alignment - 1);
}

} // namespace

JoltTempAllocator::JoltTempAllocator() {}

JoltTempAllocator::~JoltTempAllocator() {
	JPH::Free(base);
}

void *JoltTempAllocator::Allocate(uint32_t p_size) {
	if (p_size == 0) {
		return nullptr;
	}

	p_size = align_up(p_size, 16U);

	const uint64_t new_top = top + p_size;

	void *ptr = nullptr;

	if (new_top <= capacity) {
		ptr = base + top;
	} else {
		ptr = JPH::Allocate(p_size);
	}

	top = new_top;

	return ptr;
}

void JoltTempAllocator::Free(void *p_ptr, uint32_t p_size) {
	if (p_ptr == nullptr) {
		return;
	}

	p_size = align_up(p_size, 16U);

	const uint64_t new_top = top - p_size;

	if (top <= capacity) {
		if (base + new_top != p_ptr) {
			// CRASH_NOW_MSG("Jolt Physics temporary memory was freed in the wrong order.");
		}
	} else {
		JPH::Free(p_ptr);
	}

	top = new_top;
}

// ============================================================================
// JoltJobSystem
// ============================================================================

void JoltJobSystem::Job::_execute(void *p_user_data) {
	Job *job = static_cast<Job *>(p_user_data);

#ifdef DEBUG_ENABLED
	const uint64_t time_start = Time::get_ticks_usec();
#endif

	job->Execute();

#ifdef DEBUG_ENABLED
	const uint64_t time_end = Time::get_ticks_usec();
	const uint64_t time_elapsed = time_end - time_start;

	timings_lock.lock();
	timings_by_job[job->name] += time_elapsed;
	timings_lock.unlock();
#endif

	job->Release();
}

JoltJobSystem::Job::Job(const char *p_name, JPH::ColorArg p_color, JPH::JobSystem *p_job_system, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count) :
		JPH::JobSystem::Job(p_name, p_color, p_job_system, p_job_function, p_dependency_count)
#ifdef DEBUG_ENABLED
		,
		name(p_name)
#endif
{
}

void JoltJobSystem::Job::push_completed(Job *p_job) {
	Job *prev_head = nullptr;

	do {
		prev_head = completed_head.load(std::memory_order_relaxed);
		p_job->completed_next = prev_head;
	} while (!completed_head.compare_exchange_weak(prev_head, p_job, std::memory_order_release, std::memory_order_relaxed));
}

JoltJobSystem::Job *JoltJobSystem::Job::pop_completed() {
	Job *prev_head = nullptr;

	do {
		prev_head = completed_head.load(std::memory_order_relaxed);
		if (prev_head == nullptr) {
			return nullptr;
		}
	} while (!completed_head.compare_exchange_weak(prev_head, prev_head->completed_next, std::memory_order_acquire, std::memory_order_relaxed));

	return prev_head;
}

int JoltJobSystem::GetMaxConcurrency() const { return thread_count; }

JPH::JobHandle JoltJobSystem::CreateJob(const char *p_name, JPH::ColorArg p_color, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count) {
	Job *job = nullptr;

	while (true) {
		JPH::uint32 job_index = jobs.ConstructObject(p_name, p_color, this, p_job_function, p_dependency_count);

		if (job_index != JPH::FixedSizeFreeList<Job>::cInvalidObjectIndex) {
			job = &jobs.Get(job_index);
			break;
		}

		WARN_PRINT_ONCE("Jolt Physics job system exceeded the maximum number of jobs. This should "
						"not happen. Please report this. Waiting for jobs to become available...");

		OS::delay_usec(100);

		_reclaim_jobs();
	}

	JPH::JobHandle job_handle(job);

	if (p_dependency_count == 0) {
		QueueJob(job);
	}

	return job_handle;
}

void JoltJobSystem::QueueJob(JPH::JobSystem::Job *p_job) { static_cast<Job *>(p_job)->queue(); }

void JoltJobSystem::QueueJobs(JPH::JobSystem::Job **p_jobs, JPH::uint p_job_count) {
	for (JPH::uint i = 0; i < p_job_count; ++i) {
		QueueJob(p_jobs[i]);
	}
}

void JoltJobSystem::FreeJob(JPH::JobSystem::Job *p_job) {
	Job::push_completed(static_cast<Job *>(p_job));
}

void JoltJobSystem::_reclaim_jobs() {
	while (Job *job = Job::pop_completed()) {
		jobs.DestructObject(job);
	}
}

void JoltJobSystem::post_step() { _reclaim_jobs(); }
void JoltJobSystem::Job::queue() {}
void JoltJobSystem::pre_step() {}
JoltJobSystem::JoltJobSystem() {}
JoltJobSystem::Job::~Job() {}
#ifdef DEBUG_ENABLED
void JoltJobSystem::flush_timings() {}
#endif

// ============================================================================
// JoltLayers
// ============================================================================

static_assert(sizeof(JPH::ObjectLayer) == 2, "Size of Jolt's object layer has changed.");
static_assert(sizeof(JPH::BroadPhaseLayer::Type) == 1, "Size of Jolt's broadphase layer has changed.");
static_assert(JoltBroadPhaseLayer::COUNT <= 8, "Maximum number of broadphase layers exceeded.");

namespace {

template <uint8_t TSize = JoltBroadPhaseLayer::COUNT>
class JoltBroadPhaseMatrix {
	typedef JPH::BroadPhaseLayer LayerType;
	typedef LayerType::Type UnderlyingType;

public:
	JoltBroadPhaseMatrix() {
		using namespace JoltBroadPhaseLayer;

		allow_collision(BODY_STATIC, BODY_DYNAMIC);
		allow_collision(BODY_STATIC, AREA_DETECTABLE);
		allow_collision(BODY_STATIC, AREA_UNDETECTABLE);
		allow_collision(BODY_STATIC_BIG, BODY_DYNAMIC);
		allow_collision(BODY_STATIC_BIG, AREA_DETECTABLE);
		allow_collision(BODY_STATIC_BIG, AREA_UNDETECTABLE);
		allow_collision(BODY_DYNAMIC, BODY_STATIC);
		allow_collision(BODY_DYNAMIC, BODY_STATIC_BIG);
		allow_collision(BODY_DYNAMIC, BODY_DYNAMIC);
		allow_collision(BODY_DYNAMIC, AREA_DETECTABLE);
		allow_collision(BODY_DYNAMIC, AREA_UNDETECTABLE);
		allow_collision(AREA_DETECTABLE, BODY_DYNAMIC);
		allow_collision(AREA_DETECTABLE, BODY_STATIC);
		allow_collision(AREA_DETECTABLE, BODY_STATIC_BIG);
		allow_collision(AREA_DETECTABLE, AREA_DETECTABLE);
		allow_collision(AREA_DETECTABLE, AREA_UNDETECTABLE);
		allow_collision(AREA_UNDETECTABLE, BODY_DYNAMIC);
		allow_collision(AREA_UNDETECTABLE, BODY_STATIC);
		allow_collision(AREA_UNDETECTABLE, BODY_STATIC_BIG);
		allow_collision(AREA_UNDETECTABLE, AREA_DETECTABLE);
	}

	void allow_collision(UnderlyingType p_layer1, UnderlyingType p_layer2) {
		masks[p_layer1] |= uint8_t(1U << p_layer2);
	}

	void allow_collision(LayerType p_layer1, LayerType p_layer2) {
		allow_collision((UnderlyingType)p_layer1, (UnderlyingType)p_layer2);
	}

	bool should_collide(UnderlyingType p_layer1, UnderlyingType p_layer2) const {
		return (masks[p_layer1] & uint8_t(1U << p_layer2)) != 0;
	}

	bool should_collide(LayerType p_layer1, LayerType p_layer2) const {
		return should_collide((UnderlyingType)p_layer1, (UnderlyingType)p_layer2);
	}

private:
	uint8_t masks[TSize] = {};
};

constexpr JPH::ObjectLayer encode_layers(JPH::BroadPhaseLayer p_broad_phase_layer, JPH::ObjectLayer p_object_layer) {
	const uint16_t upper_bits = uint16_t((uint8_t)p_broad_phase_layer << 13U);
	const uint16_t lower_bits = uint16_t(p_object_layer);
	return JPH::ObjectLayer(upper_bits | lower_bits);
}

constexpr void decode_layers(JPH::ObjectLayer p_encoded_layers, JPH::BroadPhaseLayer &r_broad_phase_layer, JPH::ObjectLayer &r_object_layer) {
	r_broad_phase_layer = JPH::BroadPhaseLayer(uint8_t(p_encoded_layers >> 13U));
	r_object_layer = JPH::ObjectLayer(p_encoded_layers & 0b0001'1111'1111'1111U);
}

constexpr uint64_t encode_collision(uint32_t p_collision_layer, uint32_t p_collision_mask) {
	const uint64_t upper_bits = (uint64_t)p_collision_layer << 32U;
	const uint64_t lower_bits = (uint64_t)p_collision_mask;
	return upper_bits | lower_bits;
}

constexpr void decode_collision(uint64_t p_collision, uint32_t &r_collision_layer, uint32_t &r_collision_mask) {
	r_collision_layer = uint32_t(p_collision >> 32U);
	r_collision_mask = uint32_t(p_collision & 0xFFFFFFFFU);
}

} // namespace

uint32_t JoltLayers::GetNumBroadPhaseLayers() const { return JoltBroadPhaseLayer::COUNT; }

JPH::BroadPhaseLayer JoltLayers::GetBroadPhaseLayer(JPH::ObjectLayer p_layer) const {
	JPH::BroadPhaseLayer broad_phase_layer = JoltBroadPhaseLayer::BODY_STATIC;
	JPH::ObjectLayer object_layer = 0;
	decode_layers(p_layer, broad_phase_layer, object_layer);

	return broad_phase_layer;
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)

const char *JoltLayers::GetBroadPhaseLayerName(JPH::BroadPhaseLayer p_layer) const {
	switch ((JPH::BroadPhaseLayer::Type)p_layer) {
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC: {
			return "BODY_STATIC";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC_BIG: {
			return "BODY_STATIC_BIG";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_DYNAMIC: {
			return "BODY_DYNAMIC";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_DETECTABLE: {
			return "AREA_DETECTABLE";
		}
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_UNDETECTABLE: {
			return "AREA_UNDETECTABLE";
		}
		default: {
			return "UNKNOWN";
		}
	}
}

#endif

bool JoltLayers::ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::ObjectLayer p_encoded_layer2) const {
	JPH::BroadPhaseLayer broad_phase_layer1 = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t collision_layer1 = 0;
	uint32_t collision_mask1 = 0;
	from_object_layer(p_encoded_layer1, broad_phase_layer1, collision_layer1, collision_mask1);

	JPH::BroadPhaseLayer broad_phase_layer2 = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t collision_layer2 = 0;
	uint32_t collision_mask2 = 0;
	from_object_layer(p_encoded_layer2, broad_phase_layer2, collision_layer2, collision_mask2);

	const bool first_scans_second = (collision_mask1 & collision_layer2) != 0;
	const bool second_scans_first = (collision_mask2 & collision_layer1) != 0;

	return first_scans_second || second_scans_first;
}

bool JoltLayers::ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::BroadPhaseLayer p_broad_phase_layer2) const {
	static const JoltBroadPhaseMatrix matrix;

	JPH::BroadPhaseLayer broad_phase_layer1 = JoltBroadPhaseLayer::BODY_STATIC;
	JPH::ObjectLayer object_layer1 = 0;
	decode_layers(p_encoded_layer1, broad_phase_layer1, object_layer1);

	return matrix.should_collide(broad_phase_layer1, p_broad_phase_layer2);
}

JPH::ObjectLayer JoltLayers::_allocate_object_layer(uint64_t p_collision) {
	const JPH::ObjectLayer new_object_layer = next_object_layer++;

	collisions_by_layer.resize(new_object_layer + 1);
	collisions_by_layer[new_object_layer] = p_collision;

	layers_by_collision[p_collision] = new_object_layer;

	return new_object_layer;
}

JoltLayers::JoltLayers() { _allocate_object_layer(0); }
JoltLayers::~JoltLayers() = default;

JPH::ObjectLayer JoltLayers::to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask) {
	const uint64_t collision = encode_collision(p_collision_layer, p_collision_mask);

	JPH::ObjectLayer object_layer = 0;

	HashMap<uint64_t, JPH::ObjectLayer>::Iterator iter = layers_by_collision.find(collision);
	if (iter != layers_by_collision.end()) {
		object_layer = iter->value;
	} else {
		object_layer = _allocate_object_layer(collision);
	}

	return encode_layers(p_broad_phase_layer, object_layer);
}

void JoltLayers::from_object_layer(JPH::ObjectLayer p_encoded_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const {
	JPH::ObjectLayer object_layer = 0;
	decode_layers(p_encoded_layer, r_broad_phase_layer, object_layer);

	const uint64_t collision = collisions_by_layer[object_layer];
	decode_collision(collision, r_collision_layer, r_collision_mask);
}

// ============================================================================
// JoltBodyActivationListener3D
// ============================================================================

void JoltBodyActivationListener3D::OnBodyActivated(const JPH::BodyID &p_body_id, JPH::uint64 p_body_user_data) {
	if (JoltBody3D *body = reinterpret_cast<JoltObject3D *>(p_body_user_data)->as_body()) {
		body->_on_wake_up();
	}
}

// ============================================================================
// JoltContactListener3D
// ============================================================================

void JoltContactListener3D::OnContactAdded(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) {
	_try_override_collision_response(p_body1, p_body2, p_settings);
	_try_apply_surface_velocities(p_body1, p_body2, p_settings);
	_try_add_contacts(p_body1, p_body2, p_manifold, p_settings);
	_try_evaluate_area_overlap(p_body1, p_body2, p_manifold.mSubShapeID1, p_manifold.mSubShapeID2);

#ifdef DEBUG_ENABLED
	_try_add_debug_contacts(p_body1, p_body2, p_manifold);
#endif
}

void JoltContactListener3D::OnContactPersisted(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) {
	_try_override_collision_response(p_body1, p_body2, p_settings);
	_try_apply_surface_velocities(p_body1, p_body2, p_settings);
	_try_add_contacts(p_body1, p_body2, p_manifold, p_settings);
	_try_evaluate_area_overlap(p_body1, p_body2, p_manifold.mSubShapeID1, p_manifold.mSubShapeID2);

#ifdef DEBUG_ENABLED
	_try_add_debug_contacts(p_body1, p_body2, p_manifold);
#endif
}

void JoltContactListener3D::OnContactRemoved(const JPH::SubShapeIDPair &p_shape_pair) {
	if (_try_remove_contacts(p_shape_pair)) {
		return;
	}

	if (_try_remove_area_overlap(p_shape_pair)) {
		return;
	}
}

JPH::SoftBodyValidateResult JoltContactListener3D::OnSoftBodyContactValidate(const JPH::Body &p_soft_body, const JPH::Body &p_other_body, JPH::SoftBodyContactSettings &p_settings) {
	_try_override_collision_response(p_soft_body, p_other_body, p_settings);
	return JPH::SoftBodyValidateResult::AcceptContact;
}

void JoltContactListener3D::OnSoftBodyContactAdded(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold) {
	for (JPH::uint i = 0; i < p_manifold.GetNumSensorContacts(); i++) {
		if (JPH::Body *other_jolt_body = space->try_get_jolt_body(p_manifold.GetSensorContactBodyID(i))) {
			_try_evaluate_area_overlap(p_soft_body, *other_jolt_body, JPH::SubShapeID(), JPH::SubShapeID());
		}
	}

#ifdef DEBUG_ENABLED
	_try_add_debug_contacts(p_soft_body, p_manifold);
#endif
}

bool JoltContactListener3D::_try_override_collision_response(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings) {
	if (p_jolt_body1.IsSensor() || p_jolt_body2.IsSensor()) {
		return false;
	}

	if (!p_jolt_body1.IsDynamic() && !p_jolt_body2.IsDynamic()) {
		return false;
	}

	const JoltBody3D *body1 = reinterpret_cast<JoltBody3D *>(p_jolt_body1.GetUserData());
	const JoltBody3D *body2 = reinterpret_cast<JoltBody3D *>(p_jolt_body2.GetUserData());

	const bool can_collide1 = body1->can_collide_with(*body2);
	const bool can_collide2 = body2->can_collide_with(*body1);

	if (can_collide1 && !can_collide2) {
		p_settings.mInvMassScale2 = 0.0f;
		p_settings.mInvInertiaScale2 = 0.0f;
	} else if (can_collide2 && !can_collide1) {
		p_settings.mInvMassScale1 = 0.0f;
		p_settings.mInvInertiaScale1 = 0.0f;
	}

	return true;
}

bool JoltContactListener3D::_try_override_collision_response(const JPH::Body &p_jolt_soft_body, const JPH::Body &p_jolt_other_body, JPH::SoftBodyContactSettings &p_settings) {
	if (p_jolt_other_body.IsSensor()) {
		return false;
	}

	const JoltSoftBody3D *soft_body = reinterpret_cast<JoltSoftBody3D *>(p_jolt_soft_body.GetUserData());
	const JoltBody3D *other_body = reinterpret_cast<JoltBody3D *>(p_jolt_other_body.GetUserData());

	const bool can_collide1 = soft_body->can_collide_with(*other_body);
	const bool can_collide2 = other_body->can_collide_with(*soft_body);

	if (can_collide1 && !can_collide2) {
		p_settings.mInvMassScale2 = 0.0f;
		p_settings.mInvInertiaScale2 = 0.0f;
	} else if (can_collide2 && !can_collide1) {
		p_settings.mInvMassScale1 = 0.0f;
	}

	return true;
}

bool JoltContactListener3D::_try_apply_surface_velocities(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings) {
	if (p_jolt_body1.IsSensor() || p_jolt_body2.IsSensor()) {
		return false;
	}

	const bool supports_surface_velocity1 = !p_jolt_body1.IsDynamic();
	const bool supports_surface_velocity2 = !p_jolt_body2.IsDynamic();

	if (supports_surface_velocity1 == supports_surface_velocity2) {
		return false;
	}

	const JoltBody3D *body1 = reinterpret_cast<JoltBody3D *>(p_jolt_body1.GetUserData());
	const JoltBody3D *body2 = reinterpret_cast<JoltBody3D *>(p_jolt_body2.GetUserData());

	const bool has_surface_velocity1 = supports_surface_velocity1 && (body1->get_linear_surface_velocity() != Vector3() || body1->get_angular_surface_velocity() != Vector3());
	const bool has_surface_velocity2 = supports_surface_velocity2 && (body2->get_linear_surface_velocity() != Vector3() || body2->get_angular_surface_velocity() != Vector3());

	if (has_surface_velocity1 == has_surface_velocity2) {
		return false;
	}

	const JPH::Vec3 linear_velocity1 = to_jolt(body1->get_linear_surface_velocity());
	const JPH::Vec3 angular_velocity1 = to_jolt(body1->get_angular_surface_velocity());

	const JPH::Vec3 linear_velocity2 = to_jolt(body2->get_linear_surface_velocity());
	const JPH::Vec3 angular_velocity2 = to_jolt(body2->get_angular_surface_velocity());

	const JPH::RVec3 com1 = p_jolt_body1.GetCenterOfMassPosition();
	const JPH::RVec3 com2 = p_jolt_body2.GetCenterOfMassPosition();
	const JPH::Vec3 rel_com2 = JPH::Vec3(com2 - com1);

	const JPH::Vec3 angular_linear_velocity2 = rel_com2.Cross(angular_velocity2);
	const JPH::Vec3 total_linear_velocity2 = linear_velocity2 + angular_linear_velocity2;

	p_settings.mRelativeLinearSurfaceVelocity = total_linear_velocity2 - linear_velocity1;
	p_settings.mRelativeAngularSurfaceVelocity = angular_velocity2 - angular_velocity1;

	return true;
}

bool JoltContactListener3D::_try_add_contacts(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) {
	if (p_jolt_body1.IsSensor() || p_jolt_body2.IsSensor()) {
		return false;
	}

	const JoltBody3D *body1 = reinterpret_cast<JoltBody3D *>(p_jolt_body1.GetUserData());
	const JoltBody3D *body2 = reinterpret_cast<JoltBody3D *>(p_jolt_body2.GetUserData());

	if (!body1->reports_contacts() && !body2->reports_contacts()) {
		return false;
	}

	const JPH::SubShapeIDPair shape_pair(p_jolt_body1.GetID(), p_manifold.mSubShapeID1, p_jolt_body2.GetID(), p_manifold.mSubShapeID2);

	Manifold &manifold = [&]() -> Manifold & {
		const MutexLock write_lock(write_mutex);
		return manifolds_by_shape_pair[shape_pair];
	}();

	if (unlikely(!manifold.contacts1.is_empty())) {
		return false;
	}

	const JPH::uint contact_count = p_manifold.mRelativeContactPointsOn1.size();

	manifold.contacts1.reserve((uint32_t)contact_count);
	manifold.contacts2.reserve((uint32_t)contact_count);
	manifold.depth = p_manifold.mPenetrationDepth;

	JPH::CollisionEstimationResult collision;
	const JPH::Vec3 friction_impulse1 = contact_count > 0 ? (collision.mTangent1 * collision.mFrictionImpulse1) / contact_count : JPH::Vec3::sZero();
	const JPH::Vec3 friction_impulse2 = contact_count > 0 ? (collision.mTangent2 * collision.mFrictionImpulse2) / contact_count : JPH::Vec3::sZero();

	for (JPH::uint i = 0; i < contact_count; ++i) {
		const JPH::RVec3 relative_point1 = JPH::RVec3(p_manifold.mRelativeContactPointsOn1[i]);
		const JPH::RVec3 relative_point2 = JPH::RVec3(p_manifold.mRelativeContactPointsOn2[i]);

		const JPH::RVec3 world_point1 = p_manifold.mBaseOffset + relative_point1;
		const JPH::RVec3 world_point2 = p_manifold.mBaseOffset + relative_point2;

		const JPH::Vec3 velocity1 = p_jolt_body1.GetPointVelocity(world_point1);
		const JPH::Vec3 velocity2 = p_jolt_body2.GetPointVelocity(world_point2);

		const JPH::Vec3 contact_impulse = p_manifold.mWorldSpaceNormal * collision.mContactImpulse[i];
		const JPH::Vec3 combined_impulse = contact_impulse + friction_impulse1 + friction_impulse2;

		Contact contact1;
		contact1.point_self = to_godot(world_point1);
		contact1.point_other = to_godot(world_point2);
		contact1.normal = to_godot(-p_manifold.mWorldSpaceNormal);
		contact1.velocity_self = to_godot(velocity1);
		contact1.velocity_other = to_godot(velocity2);
		contact1.impulse = to_godot(-combined_impulse);
		manifold.contacts1.push_back(contact1);

		Contact contact2;
		contact2.point_self = to_godot(world_point2);
		contact2.point_other = to_godot(world_point1);
		contact2.normal = to_godot(p_manifold.mWorldSpaceNormal);
		contact2.velocity_self = to_godot(velocity2);
		contact2.velocity_other = to_godot(velocity1);
		contact2.impulse = to_godot(combined_impulse);
		manifold.contacts2.push_back(contact2);
	}

	return true;
}

bool JoltContactListener3D::_try_evaluate_area_overlap(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::SubShapeID &p_shape_id1, const JPH::SubShapeID &p_shape_id2) {
	if (!p_body1.IsSensor() && !p_body2.IsSensor()) {
		return false;
	}

	const JPH::SubShapeIDPair shape_pair1(p_body1.GetID(), p_shape_id1, p_body2.GetID(), p_shape_id2);
	const JPH::SubShapeIDPair shape_pair2(p_body2.GetID(), p_shape_id2, p_body1.GetID(), p_shape_id1);

	const JoltObject3D *object1 = reinterpret_cast<JoltObject3D *>(p_body1.GetUserData());
	const JoltObject3D *object2 = reinterpret_cast<JoltObject3D *>(p_body2.GetUserData());

	const JoltArea3D *area1 = object1->as_area();
	const JoltArea3D *area2 = object2->as_area();

	const JoltBody3D *body1 = object1->as_body();
	const JoltBody3D *body2 = object2->as_body();

	const JoltSoftBody3D *soft_body1 = object1->as_soft_body();
	const JoltSoftBody3D *soft_body2 = object2->as_soft_body();

	if (area1 != nullptr && area2 != nullptr) {
		_evaluate_area_overlap(*area1, *area2, shape_pair1);
		_evaluate_area_overlap(*area2, *area1, shape_pair2);
	} else if (area1 != nullptr && body2 != nullptr) {
		_evaluate_area_overlap(*area1, *body2, shape_pair1);
	} else if (area2 != nullptr && body1 != nullptr) {
		_evaluate_area_overlap(*area2, *body1, shape_pair2);
	} else if (area1 != nullptr && soft_body2 != nullptr) {
		_evaluate_area_overlap(*area1, *soft_body2, shape_pair1);
	} else if (area2 != nullptr && soft_body1 != nullptr) {
		_evaluate_area_overlap(*area2, *soft_body1, shape_pair2);
	}

	return true;
}

bool JoltContactListener3D::_try_remove_contacts(const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);
	return manifolds_by_shape_pair.erase(p_shape_pair);
}

bool JoltContactListener3D::_try_remove_area_overlap(const JPH::SubShapeIDPair &p_shape_pair) {
	const JPH::SubShapeIDPair swapped_shape_pair(p_shape_pair.GetBody2ID(), p_shape_pair.GetSubShapeID2(), p_shape_pair.GetBody1ID(), p_shape_pair.GetSubShapeID1());

	const MutexLock write_lock(write_mutex);

	bool removed = false;

	if (area_overlaps.erase(p_shape_pair)) {
		area_exits.insert(p_shape_pair);
		removed = true;
	}

	if (area_overlaps.erase(swapped_shape_pair)) {
		area_exits.insert(swapped_shape_pair);
		removed = true;
	}

	return removed;
}

#ifdef DEBUG_ENABLED

bool JoltContactListener3D::_try_add_debug_contacts(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold) {
	if (p_body1.IsSensor() || p_body2.IsSensor()) {
		return false;
	}

	const int64_t max_count = debug_contacts.size();

	if (max_count == 0) {
		return false;
	}

	const int additional_pairs = (int)p_manifold.mRelativeContactPointsOn1.size();
	const int additional_contacts = additional_pairs * 2;

	int current_count = debug_contact_count.load(std::memory_order_relaxed);
	bool exchanged = false;

	do {
		const int new_count = current_count + additional_contacts;

		if (new_count > max_count) {
			return false;
		}

		exchanged = debug_contact_count.compare_exchange_weak(current_count, new_count, std::memory_order_release, std::memory_order_relaxed);
	} while (!exchanged);

	for (int i = 0; i < additional_pairs; ++i) {
		const int pair_index = current_count + i * 2;

		const JPH::RVec3 point_on_1 = p_manifold.GetWorldSpaceContactPointOn1((JPH::uint)i);
		const JPH::RVec3 point_on_2 = p_manifold.GetWorldSpaceContactPointOn2((JPH::uint)i);

		debug_contacts.write[pair_index + 0] = to_godot(point_on_1);
		debug_contacts.write[pair_index + 1] = to_godot(point_on_2);
	}

	return true;
}

bool JoltContactListener3D::_try_add_debug_contacts(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold) {
	const int64_t max_count = debug_contacts.size();
	if (max_count == 0) {
		return false;
	}

	int additional_contacts = 0;

	for (const JPH::SoftBodyVertex &vertex : p_manifold.GetVertices()) {
		if (p_manifold.HasContact(vertex)) {
			additional_contacts += 1;
		}
	}

	int current_count = debug_contact_count.load(std::memory_order_relaxed);
	bool exchanged = false;

	do {
		const int new_count = current_count + additional_contacts;

		if (new_count > max_count) {
			return false;
		}

		exchanged = debug_contact_count.compare_exchange_weak(current_count, new_count, std::memory_order_release, std::memory_order_relaxed);
	} while (!exchanged);

	int contact_index = current_count;

	for (const JPH::SoftBodyVertex &vertex : p_manifold.GetVertices()) {
		if (!p_manifold.HasContact(vertex)) {
			continue;
		}

		const JPH::RMat44 body_com_transform = p_soft_body.GetCenterOfMassTransform();
		const JPH::Vec3 local_contact_point = p_manifold.GetLocalContactPoint(vertex);
		const JPH::RVec3 contact_point = body_com_transform * local_contact_point;

		debug_contacts.write[contact_index++] = to_godot(contact_point);
	}

	return true;
}

#endif

bool JoltContactListener3D::_has_shape_shifted(const JoltObject3D &p_object, const JPH::SubShapeID &p_sub_shape_id) {
	return p_object.get_previous_jolt_shape() != nullptr &&
			p_object.get_jolt_shape()->GetSubShapeUserData(p_sub_shape_id) != p_object.get_previous_jolt_shape()->GetSubShapeUserData(p_sub_shape_id);
}

void JoltContactListener3D::_evaluate_area_overlap(const JoltArea3D &p_area, const JoltArea3D &p_other_area, const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);

	if (p_area.can_monitor(p_other_area)) {
		if (!area_overlaps.has(p_shape_pair)) {
			area_overlaps.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		} else if (_has_shape_shifted(p_area, p_shape_pair.GetSubShapeID1()) || _has_shape_shifted(p_other_area, p_shape_pair.GetSubShapeID2())) {
			area_exits.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		}
	} else {
		if (area_overlaps.erase(p_shape_pair)) {
			area_exits.insert(p_shape_pair);
		}
	}
}

void JoltContactListener3D::_evaluate_area_overlap(const JoltArea3D &p_area, const JoltBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);

	if (p_area.can_monitor(p_body)) {
		if (!area_overlaps.has(p_shape_pair)) {
			area_overlaps.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		} else if (_has_shape_shifted(p_area, p_shape_pair.GetSubShapeID1()) || _has_shape_shifted(p_body, p_shape_pair.GetSubShapeID2())) {
			area_exits.insert(p_shape_pair);
			area_enters.insert(p_shape_pair);
		}
	} else {
		if (area_overlaps.erase(p_shape_pair)) {
			area_exits.insert(p_shape_pair);
		}
	}
}

void JoltContactListener3D::_evaluate_area_overlap(const JoltArea3D &p_area, const JoltSoftBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair) {
	const MutexLock write_lock(write_mutex);

	if (p_area.can_monitor(p_body)) {
		area_soft_body_overlaps.push_back(p_shape_pair);
		if (!area_exits.erase(p_shape_pair)) {
			area_enters.insert(p_shape_pair);
		}
	}
}

void JoltContactListener3D::_flush_contacts() {
	for (KeyValue<JPH::SubShapeIDPair, Manifold> &E : manifolds_by_shape_pair) {
		const JPH::SubShapeIDPair &shape_pair = E.key;
		Manifold &manifold = E.value;

		JoltBody3D *body1 = space->try_get_body(shape_pair.GetBody1ID());
		ERR_FAIL_NULL(body1);

		JoltBody3D *body2 = space->try_get_body(shape_pair.GetBody2ID());
		ERR_FAIL_NULL(body2);

		const int shape_index1 = body1->find_shape_index(shape_pair.GetSubShapeID1());
		const int shape_index2 = body2->find_shape_index(shape_pair.GetSubShapeID2());

		for (const Contact &contact : manifold.contacts1) {
			body1->add_contact(body2, manifold.depth, shape_index1, shape_index2, contact.normal, contact.point_self, contact.point_other, contact.velocity_self, contact.velocity_other, contact.impulse);
		}

		for (const Contact &contact : manifold.contacts2) {
			body2->add_contact(body1, manifold.depth, shape_index2, shape_index1, contact.normal, contact.point_self, contact.point_other, contact.velocity_self, contact.velocity_other, contact.impulse);
		}

		manifold.contacts1.clear();
		manifold.contacts2.clear();
	}
}

void JoltContactListener3D::_flush_area_enters() {
	for (const JPH::SubShapeIDPair &shape_pair : area_enters) {
		const JPH::BodyID &body_id1 = shape_pair.GetBody1ID();
		const JPH::BodyID &body_id2 = shape_pair.GetBody2ID();

		JoltObject3D *object1 = space->try_get_object(body_id1);
		JoltObject3D *object2 = space->try_get_object(body_id2);

		if (object1 == nullptr || object2 == nullptr) {
			continue;
		}

		JoltArea3D *area1 = object1->as_area();
		JoltArea3D *area2 = object2->as_area();

		const JPH::SubShapeID &sub_shape_id1 = shape_pair.GetSubShapeID1();
		const JPH::SubShapeID &sub_shape_id2 = shape_pair.GetSubShapeID2();

		if (area1 != nullptr && area2 != nullptr) {
			area1->area_shape_entered(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area1 != nullptr && area2 == nullptr) {
			area1->body_shape_entered(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area1 == nullptr && area2 != nullptr) {
			area2->body_shape_entered(body_id1, sub_shape_id1, sub_shape_id2);
		}
	}

	area_enters.clear();
}

void JoltContactListener3D::_flush_area_exits() {
	for (const JPH::SubShapeIDPair &shape_pair : area_exits) {
		const JPH::BodyID &body_id1 = shape_pair.GetBody1ID();
		const JPH::BodyID &body_id2 = shape_pair.GetBody2ID();

		JoltObject3D *object1 = space->try_get_object(body_id1);
		JoltObject3D *object2 = space->try_get_object(body_id2);

		JoltArea3D *area1 = object1 != nullptr ? object1->as_area() : nullptr;
		JoltArea3D *area2 = object2 != nullptr ? object2->as_area() : nullptr;

		const JoltBody3D *body1 = object1 != nullptr ? object1->as_body() : nullptr;
		const JoltBody3D *body2 = object2 != nullptr ? object2->as_body() : nullptr;

		const JPH::SubShapeID &sub_shape_id1 = shape_pair.GetSubShapeID1();
		const JPH::SubShapeID &sub_shape_id2 = shape_pair.GetSubShapeID2();

		if (area1 != nullptr && area2 != nullptr) {
			area1->area_shape_exited(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area1 != nullptr && body2 != nullptr) {
			area1->body_shape_exited(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (body1 != nullptr && area2 != nullptr) {
			area2->body_shape_exited(body_id1, sub_shape_id1, sub_shape_id2);
		} else if (area1 != nullptr) {
			area1->shape_exited(body_id2, sub_shape_id2, sub_shape_id1);
		} else if (area2 != nullptr) {
			area2->shape_exited(body_id1, sub_shape_id1, sub_shape_id2);
		}
	}

	area_exits.clear();
}

void JoltContactListener3D::_clear_area_soft_body_overlaps() {
	area_exits.reserve(area_soft_body_overlaps.size());
	for (const JPH::SubShapeIDPair &shape_pair : area_soft_body_overlaps) {
		area_exits.insert(shape_pair);
	}

	area_soft_body_overlaps.clear();
}

void JoltContactListener3D::pre_step() {
	_clear_area_soft_body_overlaps();

#ifdef DEBUG_ENABLED
	debug_contact_count = 0;
#endif
}

void JoltContactListener3D::post_step() {
	_flush_contacts();
	_flush_area_exits();
	_flush_area_enters();
}

// ============================================================================
// JoltMotionFilter3D / JoltQueryFilter3D
// ============================================================================

bool JoltMotionFilter3D::ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const {
	const JPH::BroadPhaseLayer::Type broad_phase_layer = (JPH::BroadPhaseLayer::Type)p_broad_phase_layer;

	switch (broad_phase_layer) {
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC_BIG:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_DYNAMIC: {
			return true;
		} break;
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_DETECTABLE:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_UNDETECTABLE: {
			return false;
		} break;
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled broad phase layer: '%d'. This should not happen. Please report this.", broad_phase_layer));
		}
	}
}

bool JoltMotionFilter3D::ShouldCollide(JPH::ObjectLayer p_object_layer) const {
	JPH::BroadPhaseLayer object_broad_phase_layer = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t object_collision_layer = 0;
	uint32_t object_collision_mask = 0;

	space.map_from_object_layer(p_object_layer, object_broad_phase_layer, object_collision_layer, object_collision_mask);

	return (body_self.get_collision_mask() & object_collision_layer) != 0;
}

bool JoltMotionFilter3D::ShouldCollide(const JPH::BodyID &p_jolt_id) const {
	return p_jolt_id != body_self.get_jolt_id();
}

bool JoltMotionFilter3D::ShouldCollideLocked(const JPH::Body &p_jolt_body) const {
	if (p_jolt_body.IsSoftBody()) {
		return false;
	}

	const JoltObject3D *object = reinterpret_cast<const JoltObject3D *>(p_jolt_body.GetUserData());
	if (excluded_bodies.has(object->get_rid())) {
		return false;
	}

	return body_self.get_jolt_body()->GetCollisionGroup().CanCollide(p_jolt_body.GetCollisionGroup());
}

bool JoltMotionFilter3D::ShouldCollide(const JPH::Shape *p_jolt_shape, const JPH::SubShapeID &p_jolt_shape_id) const {
	return true;
}

bool JoltMotionFilter3D::ShouldCollide(const JPH::Shape *p_jolt_shape_self, const JPH::SubShapeID &p_jolt_shape_id_self, const JPH::Shape *p_jolt_shape_other, const JPH::SubShapeID &p_jolt_shape_id_other) const {
	if (collide_separation_ray) {
		return true;
	}

	const JoltCustomMotionShape *motion_shape = static_cast<const JoltCustomMotionShape *>(p_jolt_shape_self);
	const JPH::ConvexShape &actual_shape_self = motion_shape->get_inner_shape();
	if (actual_shape_self.GetSubType() == JoltCustomShapeSubType::RAY) {
		return static_cast<const JoltCustomRayShape &>(actual_shape_self).slide_on_slope;
	}

	return true;
}

JoltQueryFilter3D::JoltQueryFilter3D(const PhysicsDirectSpaceState3D &p_space_state, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas, const HashSet<RID> &p_excluded, bool p_picking) :
		space(p_space_state.get_space()),
		excluded(p_excluded),
		collision_mask(p_collision_mask),
		collide_with_bodies(p_collide_with_bodies),
		collide_with_areas(p_collide_with_areas),
		picking(p_picking) {
}

bool JoltQueryFilter3D::ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const {
	const JPH::BroadPhaseLayer::Type broad_phase_layer = (JPH::BroadPhaseLayer::Type)p_broad_phase_layer;

	switch (broad_phase_layer) {
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_STATIC_BIG:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::BODY_DYNAMIC: {
			return collide_with_bodies;
		} break;
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_DETECTABLE:
		case (JPH::BroadPhaseLayer::Type)JoltBroadPhaseLayer::AREA_UNDETECTABLE: {
			return collide_with_areas;
		} break;
		default: {
			ERR_FAIL_V_MSG(false, vformat("Unhandled broad phase layer: '%d'. This should not happen. Please report this.", broad_phase_layer));
		}
	}
}

bool JoltQueryFilter3D::ShouldCollide(JPH::ObjectLayer p_object_layer) const {
	JPH::BroadPhaseLayer object_broad_phase_layer = JoltBroadPhaseLayer::BODY_STATIC;
	uint32_t object_collision_layer = 0;
	uint32_t object_collision_mask = 0;

	space.map_from_object_layer(p_object_layer, object_broad_phase_layer, object_collision_layer, object_collision_mask);

	return (collision_mask & object_collision_layer) != 0;
}

bool JoltQueryFilter3D::ShouldCollide(const JPH::BodyID &p_body_id) const {
	return true;
}

bool JoltQueryFilter3D::ShouldCollideLocked(const JPH::Body &p_body) const {
	JoltObject3D *object = reinterpret_cast<JoltObject3D *>(p_body.GetUserData());
	return (!picking || object->is_pickable()) && !excluded.has(object->get_rid());
}

// ============================================================================
// JoltSpace3D
// ============================================================================

namespace {

constexpr double SPACE_DEFAULT_CONTACT_RECYCLE_RADIUS = 0.01;
constexpr double SPACE_DEFAULT_CONTACT_MAX_SEPARATION = 0.05;
constexpr double SPACE_DEFAULT_CONTACT_MAX_ALLOWED_PENETRATION = 0.01;
constexpr double SPACE_DEFAULT_CONTACT_DEFAULT_BIAS = 0.8;
constexpr double SPACE_DEFAULT_SLEEP_THRESHOLD_LINEAR = 0.1;
constexpr double SPACE_DEFAULT_SLEEP_THRESHOLD_ANGULAR = 8.0 * Math::PI / 180;
constexpr double SPACE_DEFAULT_SOLVER_ITERATIONS = 8;

} // namespace

void JoltSpace3D::_pre_step(float p_step) {
	flush_pending_objects();

	while (needs_optimization_list.first()) {
		JoltObject3D *object = needs_optimization_list.first()->self();
		needs_optimization_list.remove(needs_optimization_list.first());
		object->commit_shapes(true);
	}

	contact_listener->pre_step();

	const JPH::BodyLockInterface &lock_iface = get_lock_iface();
	const JPH::BodyID *active_rigid_bodies = physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::RigidBody);
	const JPH::uint32 active_rigid_body_count = physics_system->GetNumActiveBodies(JPH::EBodyType::RigidBody);

	for (JPH::uint32 i = 0; i < active_rigid_body_count; i++) {
		JPH::Body *jolt_body = lock_iface.TryGetBody(active_rigid_bodies[i]);
		JoltObject3D *object = reinterpret_cast<JoltObject3D *>(jolt_body->GetUserData());
		object->pre_step(p_step);
	}

	const JPH::BodyID *active_soft_bodies = physics_system->GetActiveBodiesUnsafe(JPH::EBodyType::SoftBody);
	const JPH::uint32 active_soft_body_count = physics_system->GetNumActiveBodies(JPH::EBodyType::SoftBody);

	for (JPH::uint32 i = 0; i < active_soft_body_count; i++) {
		JPH::Body *jolt_body = lock_iface.TryGetBody(active_soft_bodies[i]);
		JoltObject3D *object = reinterpret_cast<JoltObject3D *>(jolt_body->GetUserData());
		object->pre_step(p_step);
	}

	physics_system->SetBodyActivationListener(body_activation_listener);
}

void JoltSpace3D::_post_step(float p_step) {
	physics_system->SetBodyActivationListener(nullptr);

	contact_listener->post_step();

	while (shapes_changed_list.first()) {
		JoltObject3D *object = shapes_changed_list.first()->self();
		shapes_changed_list.remove(shapes_changed_list.first());
		object->clear_previous_shape();
	}
}

JoltSpace3D::~JoltSpace3D() {
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

void JoltSpace3D::call_queries() {
	while (body_call_queries_list.first()) {
		JoltBody3D *body = body_call_queries_list.first()->self();
		body_call_queries_list.remove(body_call_queries_list.first());
		body->call_queries();
	}

	while (area_call_queries_list.first()) {
		JoltArea3D *body = area_call_queries_list.first()->self();
		area_call_queries_list.remove(area_call_queries_list.first());
		body->call_queries();
	}
}

double JoltSpace3D::get_param(PS3DE::SpaceParameter p_param) const {
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
			ERR_FAIL_V_MSG(0.0, vformat("Unhandled space parameter: '%d'. This should not happen. Please report this.", p_param));
		}
	}
}

void JoltSpace3D::set_param(PS3DE::SpaceParameter p_param, double p_value) {
	switch (p_param) {
		case PS3DE::SPACE_PARAM_CONTACT_RECYCLE_RADIUS: {
			WARN_PRINT("Space-specific contact recycle radius is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_CONTACT_MAX_SEPARATION: {
			WARN_PRINT("Space-specific contact max separation is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_CONTACT_MAX_ALLOWED_PENETRATION: {
			WARN_PRINT("Space-specific contact max allowed penetration is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_CONTACT_DEFAULT_BIAS: {
			WARN_PRINT("Space-specific contact default bias is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_BODY_LINEAR_VELOCITY_SLEEP_THRESHOLD: {
			WARN_PRINT("Space-specific linear velocity sleep threshold is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_BODY_ANGULAR_VELOCITY_SLEEP_THRESHOLD: {
			WARN_PRINT("Space-specific angular velocity sleep threshold is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_BODY_TIME_TO_SLEEP: {
			WARN_PRINT("Space-specific body sleep time is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		case PS3DE::SPACE_PARAM_SOLVER_ITERATIONS: {
			WARN_PRINT("Space-specific solver iterations is not supported when using Jolt Physics. Any such value will be ignored.");
		} break;
		default: {
			ERR_FAIL_MSG(vformat("Unhandled space parameter: '%d'. This should not happen. Please report this.", p_param));
		} break;
	}
}

JPH::BodyInterface &JoltSpace3D::get_body_iface() {
	return physics_system->GetBodyInterfaceNoLock();
}

const JPH::BodyInterface &JoltSpace3D::get_body_iface() const {
	return physics_system->GetBodyInterfaceNoLock();
}

const JPH::BodyLockInterface &JoltSpace3D::get_lock_iface() const {
	return physics_system->GetBodyLockInterfaceNoLock();
}

const JPH::BroadPhaseQuery &JoltSpace3D::get_broad_phase_query() const {
	return physics_system->GetBroadPhaseQuery();
}

const JPH::NarrowPhaseQuery &JoltSpace3D::get_narrow_phase_query() const {
	return physics_system->GetNarrowPhaseQueryNoLock();
}

JPH::ObjectLayer JoltSpace3D::map_to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask) {
	return layers->to_object_layer(p_broad_phase_layer, p_collision_layer, p_collision_mask);
}

void JoltSpace3D::map_from_object_layer(JPH::ObjectLayer p_object_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const {
	layers->from_object_layer(p_object_layer, r_broad_phase_layer, r_collision_layer, r_collision_mask);
}

JPH::Body *JoltSpace3D::try_get_jolt_body(const JPH::BodyID &p_body_id) const {
	return get_lock_iface().TryGetBody(p_body_id);
}

JoltObject3D *JoltSpace3D::try_get_object(const JPH::BodyID &p_body_id) const {
	const JPH::Body *jolt_body = try_get_jolt_body(p_body_id);
	if (unlikely(jolt_body == nullptr)) {
		return nullptr;
	}

	return reinterpret_cast<JoltObject3D *>(jolt_body->GetUserData());
}

JoltBody3D *JoltSpace3D::try_get_body(const JPH::BodyID &p_body_id) const {
	JoltObject3D *object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_body();
}

JoltArea3D *JoltSpace3D::try_get_area(const JPH::BodyID &p_body_id) const {
	JoltObject3D *object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_area();
}

JoltSoftBody3D *JoltSpace3D::try_get_soft_body(const JPH::BodyID &p_body_id) const {
	JoltObject3D *object = try_get_object(p_body_id);
	if (unlikely(object == nullptr)) {
		return nullptr;
	}

	return object->as_soft_body();
}

PhysicsDirectSpaceState3D *JoltSpace3D::get_direct_state() {
	if (direct_state == nullptr) {
		direct_state = memnew(PhysicsDirectSpaceState3D(this));
	}

	return direct_state;
}

void JoltSpace3D::remove_object(const JPH::BodyID &p_jolt_id) {
	JPH::BodyInterface &body_iface = get_body_iface();

	if (!pending_objects_sleeping.erase_unordered(p_jolt_id) && !pending_objects_awake.erase_unordered(p_jolt_id)) {
		body_iface.RemoveBody(p_jolt_id);
	}

	body_iface.DestroyBody(p_jolt_id);

	if (PhysicsServer3D::get_driver() && !PhysicsServer3D::get_driver()->is_active() && physics_system->GetNumBodies() <= 1) {
		physics_system->OptimizeBroadPhase();
	}
}

void JoltSpace3D::flush_pending_objects() {
	if (pending_objects_sleeping.is_empty() && pending_objects_awake.is_empty()) {
		return;
	}

	MutexLock pending_objects_lock(pending_objects_mutex);

	JPH::BodyInterface &body_iface = get_body_iface();

	if (!pending_objects_sleeping.is_empty()) {
		JPH::BodyInterface::AddState add_state = body_iface.AddBodiesPrepare(pending_objects_sleeping.ptr(), pending_objects_sleeping.size());
		body_iface.AddBodiesFinalize(pending_objects_sleeping.ptr(), pending_objects_sleeping.size(), add_state, JPH::EActivation::DontActivate);
		pending_objects_sleeping.reset();
	}

	if (!pending_objects_awake.is_empty()) {
		JPH::BodyInterface::AddState add_state = body_iface.AddBodiesPrepare(pending_objects_awake.ptr(), pending_objects_awake.size());
		body_iface.AddBodiesFinalize(pending_objects_awake.ptr(), pending_objects_awake.size(), add_state, JPH::EActivation::Activate);
		pending_objects_awake.reset();
	}
}

void JoltSpace3D::set_is_object_sleeping(const JPH::BodyID &p_jolt_id, bool p_enable) {
	if (p_enable) {
		if (pending_objects_awake.erase_unordered(p_jolt_id)) {
			pending_objects_sleeping.push_back(p_jolt_id);
		} else if (pending_objects_sleeping.has(p_jolt_id)) {
			// Do nothing.
		} else {
			get_body_iface().DeactivateBody(p_jolt_id);
		}
	} else {
		if (pending_objects_sleeping.erase_unordered(p_jolt_id)) {
			pending_objects_awake.push_back(p_jolt_id);
		} else if (pending_objects_awake.has(p_jolt_id)) {
			// Do nothing.
		} else {
			get_body_iface().ActivateBody(p_jolt_id);
		}
	}
}

void JoltSpace3D::enqueue_call_queries(SelfList<JoltBody3D> *p_body) {
	MutexLock body_call_queries_lock(body_call_queries_mutex);

	if (!p_body->in_list()) {
		body_call_queries_list.add(p_body);
	}
}

void JoltSpace3D::enqueue_call_queries(SelfList<JoltArea3D> *p_area) {
	if (!p_area->in_list()) {
		area_call_queries_list.add(p_area);
	}
}

void JoltSpace3D::dequeue_call_queries(SelfList<JoltBody3D> *p_body) {
	if (p_body->in_list()) {
		body_call_queries_list.remove(p_body);
	}
}

void JoltSpace3D::dequeue_call_queries(SelfList<JoltArea3D> *p_area) {
	if (p_area->in_list()) {
		area_call_queries_list.remove(p_area);
	}
}

void JoltSpace3D::enqueue_shapes_changed(SelfList<JoltObject3D> *p_object) {
	if (!p_object->in_list()) {
		shapes_changed_list.add(p_object);
	}
}

void JoltSpace3D::dequeue_shapes_changed(SelfList<JoltObject3D> *p_object) {
	if (p_object->in_list()) {
		shapes_changed_list.remove(p_object);
	}
}

void JoltSpace3D::enqueue_needs_optimization(SelfList<JoltObject3D> *p_object) {
	if (!p_object->in_list()) {
		needs_optimization_list.add(p_object);
	}
}

void JoltSpace3D::dequeue_needs_optimization(SelfList<JoltObject3D> *p_object) {
	if (p_object->in_list()) {
		needs_optimization_list.remove(p_object);
	}
}

void JoltSpace3D::add_joint(JPH::Constraint *p_jolt_ref) {
	physics_system->AddConstraint(p_jolt_ref);
}

void JoltSpace3D::add_joint(JoltJoint3D *p_joint) { add_joint(p_joint->get_jolt_ref()); }

void JoltSpace3D::remove_joint(JPH::Constraint *p_jolt_ref) {
	physics_system->RemoveConstraint(p_jolt_ref);
}

void JoltSpace3D::remove_joint(JoltJoint3D *p_joint) { remove_joint(p_joint->get_jolt_ref()); }

#ifdef DEBUG_ENABLED

const PackedVector3Array &JoltSpace3D::get_debug_contacts() const {
	return contact_listener->get_debug_contacts();
}

int JoltSpace3D::get_debug_contact_count() const {
	return contact_listener->get_debug_contact_count();
}

int JoltSpace3D::get_max_debug_contacts() const {
	return contact_listener->get_max_debug_contacts();
}

void JoltSpace3D::set_max_debug_contacts(int p_count) {
	contact_listener->set_max_debug_contacts(p_count);
}

void JoltSpace3D::dump_debug_snapshot(const String &) {}

#endif

void JoltSpace3D::step(float) {}
JoltSpace3D::JoltSpace3D(JPH::JobSystem *, JPH::TempAllocator *) {}
JPH::Body *JoltSpace3D::add_object(const JoltObject3D &, const JPH::BodyCreationSettings &, bool) { return nullptr; }

// ============================================================================
// PhysicsDirectSpaceState3D
// ============================================================================

namespace {

bool _cast_motion_impl(JoltSpace3D *p_space, const JPH::Shape &p_jolt_shape, const Transform3D &p_transform_com, const Vector3 &p_scale, const Vector3 &p_motion, bool p_use_edge_removal, bool p_ignore_overlaps, const JPH::CollideShapeSettings &p_settings, const JPH::BroadPhaseLayerFilter &p_broad_phase_layer_filter, const JPH::ObjectLayerFilter &p_object_layer_filter, const JPH::BodyFilter &p_body_filter, const JPH::ShapeFilter &p_shape_filter, real_t &r_closest_safe, real_t &r_closest_unsafe) {
	r_closest_safe = 1.0f;
	r_closest_unsafe = 1.0f;

	ERR_FAIL_NULL_V(p_space, false);
	ERR_FAIL_COND_V_MSG(p_jolt_shape.GetType() != JPH::EShapeType::Convex, false, "Shape-casting with non-convex shapes is not supported.");

	const float motion_length = (float)p_motion.length();

	if (p_ignore_overlaps && motion_length == 0.0f) {
		return false;
	}

	const JPH::RMat44 transform_com = to_jolt_r(p_transform_com);
	const JPH::Vec3 scale = to_jolt(p_scale);
	const JPH::Vec3 motion = to_jolt(p_motion);
	const JPH::Vec3 motion_local = transform_com.Multiply3x3Transposed(motion);

	JPH::AABox aabb = p_jolt_shape.GetWorldSpaceBounds(transform_com, scale);
	JPH::AABox aabb_translated = aabb;
	aabb_translated.Translate(motion);
	aabb.Encapsulate(aabb_translated);

	JoltQueryCollectorAnyMulti<JPH::CollideShapeBodyCollector, 1024> aabb_collector;
	p_space->get_broad_phase_query().CollideAABox(aabb, aabb_collector, p_broad_phase_layer_filter, p_object_layer_filter);

	if (!aabb_collector.had_hit()) {
		return false;
	}

	const JPH::RVec3 base_offset = transform_com.GetTranslation();

	JoltCustomMotionShape motion_shape(static_cast<const JPH::ConvexShape &>(p_jolt_shape));

	auto collides = [&](const JPH::Body &p_other_body, float p_fraction) {
		motion_shape.set_motion(motion_local * p_fraction);

		const JPH::TransformedShape other_shape = p_other_body.GetTransformedShape();

		JoltQueryCollectorAny<JPH::CollideShapeCollector> collector;

		if (p_use_edge_removal) {
			JPH::CollideShapeSettings eier_settings = p_settings;
			eier_settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
			eier_settings.mCollectFacesMode = JPH::ECollectFacesMode::CollectFaces;

			JPH::InternalEdgeRemovingCollector eier_collector(collector, JPH::cDefaultInternalEdgeRemovalVertexToleranceSq);
			other_shape.CollideShape(&motion_shape, scale, transform_com, eier_settings, base_offset, eier_collector, p_shape_filter);
			eier_collector.Flush();
		} else {
			other_shape.CollideShape(&motion_shape, scale, transform_com, p_settings, base_offset, collector, p_shape_filter);
		}

		return collector.had_hit();
	};

	const int step_count = CLAMP(int(std::log(1000.0f * motion_length) / (float)Math::LN2), 4, 16);

	bool collided = false;

	for (int i = 0; i < aabb_collector.get_hit_count(); ++i) {
		const JPH::BodyID other_jolt_id = aabb_collector.get_hit(i);
		if (!p_body_filter.ShouldCollide(other_jolt_id)) {
			continue;
		}

		const JPH::Body *other_jolt_body = p_space->try_get_jolt_body(other_jolt_id);
		if (!p_body_filter.ShouldCollideLocked(*other_jolt_body)) {
			continue;
		}

		if (!collides(*other_jolt_body, 1.0f)) {
			continue;
		}

		if (p_ignore_overlaps && collides(*other_jolt_body, 0.0f)) {
			continue;
		}

		float lo = 0.0f;
		float hi = 1.0f;
		float coeff = 0.5f;

		for (int j = 0; j < step_count; ++j) {
			const float fraction = lo + (hi - lo) * coeff;

			if (collides(*other_jolt_body, fraction)) {
				collided = true;

				hi = fraction;

				if (j == 0 || lo > 0.0f) {
					coeff = 0.5f;
				} else {
					coeff = 0.25f;
				}
			} else {
				lo = fraction;

				if (j == 0 || hi < 1.0f) {
					coeff = 0.5f;
				} else {
					coeff = 0.75f;
				}
			}
		}

		if (lo < r_closest_safe) {
			r_closest_safe = lo;
			r_closest_unsafe = hi;
		}
	}

	return collided;
}

void _generate_manifold(JoltSpace3D *p_space, const JPH::CollideShapeResult &p_hit, JPH::ContactPoints &r_contact_points1, JPH::ContactPoints &r_contact_points2 JPH_IF_DEBUG_RENDERER(, JPH::RVec3Arg p_center_of_mass)) {
	const JPH::PhysicsSystem &physics_system = p_space->get_physics_system();
	const JPH::PhysicsSettings &physics_settings = physics_system.GetPhysicsSettings();
	const JPH::Vec3 penetration_axis = p_hit.mPenetrationAxis.Normalized();

	JPH::ManifoldBetweenTwoFaces(p_hit.mContactPointOn1, p_hit.mContactPointOn2, penetration_axis, physics_settings.mManifoldTolerance, p_hit.mShape1Face, p_hit.mShape2Face, r_contact_points1, r_contact_points2 JPH_IF_DEBUG_RENDERER(, p_center_of_mass));

	if (r_contact_points1.size() > 4) {
		JPH::PruneContactPoints(penetration_axis, r_contact_points1, r_contact_points2 JPH_IF_DEBUG_RENDERER(, p_center_of_mass));
	}
}

void _collide_shape_queries(JoltSpace3D *p_space, const JPH::Shape *p_shape, JPH::Vec3Arg p_scale, JPH::RMat44Arg p_transform_com, const JPH::CollideShapeSettings &p_settings, JPH::RVec3Arg p_base_offset, JPH::CollideShapeCollector &p_collector, const JPH::BroadPhaseLayerFilter &p_broad_phase_layer_filter = JPH::BroadPhaseLayerFilter(), const JPH::ObjectLayerFilter &p_object_layer_filter = JPH::ObjectLayerFilter(), const JPH::BodyFilter &p_body_filter = JPH::BodyFilter(), const JPH::ShapeFilter &p_shape_filter = JPH::ShapeFilter()) {
}

} // namespace

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D(JoltSpace3D *p_space) :
		space(p_space) {
}

bool PhysicsDirectSpaceState3D::collide_shape(const PS3DT::ShapeParameters &p_parameters, Vector3 *r_results, int p_result_max, int &r_result_count) {
	r_result_count = 0;

	ERR_FAIL_NULL_V(space, false);
	ERR_FAIL_COND_V_MSG(space->is_stepping(), false, "collide_shape must not be called while the physics space is being stepped.");

	if (p_result_max == 0) {
		return false;
	}

	space->flush_pending_objects();

	JoltPhysicsServer3D *physics_server = PhysicsServer3D::get_driver();
	ERR_FAIL_NULL_V(physics_server, false);
	JoltShape3D *shape = physics_server->get_shape(p_parameters.shape_rid);
	ERR_FAIL_NULL_V(shape, false);

	const JPH::ShapeRefC jolt_shape = shape->try_build();
	ERR_FAIL_NULL_V(jolt_shape, false);

	Transform3D transform = p_parameters.transform;
	JOLT_ENSURE_SCALE_NOT_ZERO(transform, "collide_shape was passed an invalid transform.");

	Vector3 scale;
	JoltMath::decompose(transform, scale);
	JOLT_ENSURE_SCALE_VALID(jolt_shape, scale, "collide_shape was passed an invalid transform.");

	const Vector3 com_scaled = to_godot(jolt_shape->GetCenterOfMass());
	const Transform3D transform_com = transform.translated_local(com_scaled);

	JPH::CollideShapeSettings settings;
	settings.mCollectFacesMode = JPH::ECollectFacesMode::CollectFaces;
	settings.mMaxSeparationDistance = (float)p_parameters.margin;

	const Vector3 &base_offset = transform_com.origin;

	const JoltQueryFilter3D query_filter(*this, p_parameters.collision_mask, p_parameters.collide_with_bodies, p_parameters.collide_with_areas, p_parameters.exclude);
	JoltQueryCollectorAnyMulti<JPH::CollideShapeCollector, 32> collector(p_result_max);
	_collide_shape_queries(space, jolt_shape, to_jolt(scale), to_jolt_r(transform_com), settings, to_jolt_r(base_offset), collector, query_filter, query_filter, query_filter);

	if (!collector.had_hit()) {
		return false;
	}

	const int max_points = p_result_max * 2;

	int point_count = 0;

	for (int i = 0; i < collector.get_hit_count(); ++i) {
		const JPH::CollideShapeResult &hit = collector.get_hit(i);

		const Vector3 penetration_axis = to_godot(hit.mPenetrationAxis.Normalized());
		const Vector3 margin_offset = penetration_axis * (float)p_parameters.margin;

		JPH::ContactPoints contact_points1;
		JPH::ContactPoints contact_points2;

		_generate_manifold(space, hit, contact_points1, contact_points2 JPH_IF_DEBUG_RENDERER(, to_jolt_r(base_offset)));

		for (JPH::uint j = 0; j < contact_points1.size(); ++j) {
			r_results[point_count++] = base_offset + to_godot(contact_points1[j]) + margin_offset;
			r_results[point_count++] = base_offset + to_godot(contact_points2[j]);

			if (point_count >= max_points) {
				break;
			}
		}

		if (point_count >= max_points) {
			break;
		}
	}

	r_result_count = point_count / 2;

	return true;
}

Vector3 PhysicsDirectSpaceState3D::get_closest_point_to_object_volume(RID p_object, Vector3 p_point) const {
	ERR_FAIL_NULL_V(space, Vector3());
	ERR_FAIL_COND_V_MSG(space->is_stepping(), Vector3(), "get_closest_point_to_object_volume must not be called while the physics space is being stepped.");

	space->flush_pending_objects();

	JoltPhysicsServer3D *physics_server = PhysicsServer3D::get_driver();
	ERR_FAIL_NULL_V(physics_server, Vector3());
	JoltObject3D *object = physics_server->get_area(p_object);

	if (object == nullptr) {
		object = physics_server->get_body(p_object);
	}

	ERR_FAIL_NULL_V(object, Vector3());
	ERR_FAIL_COND_V(object->get_space() != space, Vector3());

	JoltQueryCollectorAll<JPH::TransformedShapeCollector, 32> collector;
	const JPH::TransformedShape root_shape = object->get_jolt_body()->GetTransformedShape();
	root_shape.CollectTransformedShapes(object->get_jolt_body()->GetWorldSpaceBounds(), collector);

	const JPH::RVec3 point = to_jolt_r(p_point);

	float closest_distance_sq = FLT_MAX;
	JPH::RVec3 closest_point = JPH::RVec3::sZero();

	bool found_point = false;

	for (int i = 0; i < collector.get_hit_count(); ++i) {
		const JPH::TransformedShape &shape_transformed = collector.get_hit(i);
		const JPH::Shape &shape = *shape_transformed.mShape;

		if (shape.GetType() != JPH::EShapeType::Convex) {
			continue;
		}

		const JPH::ConvexShape &shape_convex = static_cast<const JPH::ConvexShape &>(shape);

		JPH::GJKClosestPoint gjk;

		JPH::ConvexShape::SupportBuffer shape_support_buffer;
		const JPH::ConvexShape::Support *shape_support = shape_convex.GetSupportFunction(JPH::ConvexShape::ESupportMode::IncludeConvexRadius, shape_support_buffer, shape_transformed.GetShapeScale());

		const JPH::RMat44 shape_rotation = JPH::RMat44::sRotation(shape_transformed.mShapeRotation);
		const JPH::Vec3 shape_com = shape_rotation.Multiply3x3(shape.GetCenterOfMass());
		const JPH::RVec3 shape_pos = shape_transformed.mShapePositionCOM - JPH::RVec3(shape_com);
		const JPH::RMat44 shape_xform = shape_rotation.PostTranslated(shape_pos);
		const JPH::RMat44 shape_xform_inv = shape_xform.InversedRotationTranslation();

		JPH::PointConvexSupport point_support;
		point_support.mPoint = JPH::Vec3(shape_xform_inv * point);

		JPH::Vec3 separating_axis = JPH::Vec3::sAxisX();
		JPH::Vec3 point_on_a = JPH::Vec3::sZero();
		JPH::Vec3 point_on_b = JPH::Vec3::sZero();

		const float distance_sq = gjk.GetClosestPoints(*shape_support, point_support, JPH::cDefaultCollisionTolerance, FLT_MAX, separating_axis, point_on_a, point_on_b);

		if (distance_sq == 0.0f) {
			closest_point = point;
			found_point = true;
			break;
		}

		if (distance_sq < closest_distance_sq) {
			closest_distance_sq = distance_sq;
			closest_point = shape_xform * point_on_a;
			found_point = true;
		}
	}

	if (found_point) {
		return to_godot(closest_point);
	} else {
		return to_godot(object->get_jolt_body()->GetPosition());
	}
}

bool PhysicsDirectSpaceState3D::body_test_motion(const JoltBody3D &p_body, const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult *r_result) const {
	return false;
}

bool PhysicsDirectSpaceState3D::intersect_ray(const PS3DT::RayParameters &p_ray_params, PS3DT::RayResult &r_result) {
	return false;
}

int PhysicsDirectSpaceState3D::intersect_point(const PS3DT::PointParameters &p_point_params, PS3DT::ShapeResult *r_results, int p_result_max) {
	return 0;
}

int PhysicsDirectSpaceState3D::intersect_shape(const PS3DT::ShapeParameters &p_shape_params, PS3DT::ShapeResult *r_results, int p_result_max) {
	return 0;
}

bool PhysicsDirectSpaceState3D::cast_motion(const PS3DT::ShapeParameters &p_shape_params, real_t &r_closest_safe, real_t &r_closest_unsafe, PS3DT::ShapeRestInfo *r_info) {
	return false;
}

bool PhysicsDirectSpaceState3D::rest_info(const PS3DT::ShapeParameters &p_shape_params, PS3DT::ShapeRestInfo *r_info) {
	return false;
}
