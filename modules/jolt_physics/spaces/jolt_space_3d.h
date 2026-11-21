/**************************************************************************/
/*  jolt_space_3d.h                                                       */
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

#include "../jolt_globals.h"

#include "core/math/vector3.h"
#include "core/os/mutex.h"
#include "core/os/spin_lock.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/self_list.h"
#include "core/types.h"
#include "servers/physics_3d/physics_server_3d_enums.h"

// includes must stay...
#include <Jolt/Jolt.h>
// ... in this order
#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystem.h>
#include <Jolt/Core/JobSystemWithBarrier.h>
#include <Jolt/Core/STLLocalAllocator.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseQuery.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/InternalEdgeRemovingCollector.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/ShapeFilter.h>
#include <Jolt/Physics/Constraints/Constraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/SoftBody/SoftBodyContactListener.h>

#include <atomic>
#include <cstdint>

class JoltArea3D;
class JoltBody3D;
class JoltJoint3D;
class JoltObject3D;
class JoltPhysicsServer3D;
class JoltSoftBody3D;
class JoltSpace3D;
class PhysicsDirectSpaceState3D;

// ============================================================================
// JoltBroadPhaseLayer
// ============================================================================

namespace JoltBroadPhaseLayer {

constexpr JPH::BroadPhaseLayer BODY_STATIC(0);
constexpr JPH::BroadPhaseLayer BODY_STATIC_BIG(1);
constexpr JPH::BroadPhaseLayer BODY_DYNAMIC(2);
constexpr JPH::BroadPhaseLayer AREA_DETECTABLE(3);
constexpr JPH::BroadPhaseLayer AREA_UNDETECTABLE(4);

constexpr uint32_t COUNT = 5;

} // namespace JoltBroadPhaseLayer

// ============================================================================
// Query Collectors
// ============================================================================

template <typename TBase, int TDefaultCapacity>
class JoltQueryCollectorAll : public TBase {
public:
	typedef typename TBase::ResultType Hit;
	typedef JPH::Array<Hit, JPH::STLLocalAllocator<Hit, TDefaultCapacity>> HitArray;

private:
	HitArray hits;

public:
	JoltQueryCollectorAll() {
		hits.reserve(TDefaultCapacity);
	}

	bool had_hit() const { return !hits.is_empty(); }
	int get_hit_count() const { return hits.size(); }
	const Hit &get_hit(int p_index) const { return hits[p_index]; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		hits.clear();
	}

	virtual void AddHit(const Hit &p_hit) override {
		hits.push_back(p_hit);
	}
};

template <typename TBase>
class JoltQueryCollectorAny : public TBase {
public:
	typedef typename TBase::ResultType Hit;

private:
	Hit hit;
	bool valid = false;

public:
	bool had_hit() const { return valid; }
	const Hit &get_hit() const { return hit; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		valid = false;
	}

	virtual void AddHit(const Hit &p_hit) override {
		hit = p_hit;
		valid = true;

		TBase::ForceEarlyOut();
	}
};

template <typename TBase, int TDefaultCapacity>
class JoltQueryCollectorAnyMulti : public TBase {
public:
	typedef typename TBase::ResultType Hit;
	typedef JPH::Array<Hit, JPH::STLLocalAllocator<Hit, TDefaultCapacity>> HitArray;

private:
	HitArray hits;
	int max_hits = 0;

public:
	explicit JoltQueryCollectorAnyMulti(int p_max_hits = TDefaultCapacity) :
			max_hits(p_max_hits) {
		hits.reserve(TDefaultCapacity);
	}

	bool had_hit() const { return hits.size() > 0; }
	int get_hit_count() const { return hits.size(); }
	const Hit &get_hit(int p_index) const { return hits[p_index]; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		hits.clear();
	}

	virtual void AddHit(const Hit &p_hit) override {
		if ((int)hits.size() < max_hits) {
			hits.push_back(p_hit);
		}

		if ((int)hits.size() == max_hits) {
			TBase::ForceEarlyOut();
		}
	}
};

template <typename TBase>
class JoltQueryCollectorClosest : public TBase {
public:
	typedef typename TBase::ResultType Hit;

private:
	Hit hit;
	bool valid = false;

public:
	bool had_hit() const { return valid; }
	const Hit &get_hit() const { return hit; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		valid = false;
	}

	virtual void AddHit(const Hit &p_hit) override {
		const float early_out = p_hit.GetEarlyOutFraction();

		if (!valid || early_out < hit.GetEarlyOutFraction()) {
			TBase::UpdateEarlyOutFraction(early_out);

			hit = p_hit;
			valid = true;
		}
	}
};

template <typename TBase, int TDefaultCapacity>
class JoltQueryCollectorClosestMulti : public TBase {
public:
	typedef typename TBase::ResultType Hit;
	typedef JPH::Array<Hit, JPH::STLLocalAllocator<Hit, TDefaultCapacity + 1>> HitArray;

private:
	HitArray hits;
	int max_hits = 0;

public:
	explicit JoltQueryCollectorClosestMulti(int p_max_hits = TDefaultCapacity) :
			max_hits(p_max_hits) {
		hits.reserve(TDefaultCapacity + 1);
	}

	bool had_hit() const { return hits.size() > 0; }
	int get_hit_count() const { return hits.size(); }
	const Hit &get_hit(int p_index) const { return hits[p_index]; }

	void reset() { Reset(); }

	virtual void Reset() override {
		TBase::Reset();
		hits.clear();
	}

	virtual void AddHit(const Hit &p_hit) override {
		typename HitArray::const_iterator E = hits.cbegin();
		for (; E != hits.cend(); ++E) {
			if (p_hit.GetEarlyOutFraction() < E->GetEarlyOutFraction()) {
				break;
			}
		}

		hits.insert(E, p_hit);

		if ((int)hits.size() > max_hits) {
			hits.resize(max_hits);
		}
	}
};

template <typename TCollector, int TDefaultCapacity>
class JoltQueryCollectorMotion final : public JoltQueryCollectorClosestMulti<TCollector, TDefaultCapacity> {
public:
	typedef JoltQueryCollectorClosestMulti<TCollector, TDefaultCapacity> Base;
	typedef typename Base::Hit Hit;

private:
	Vector3 direction;
	float distance_sq = 0.0f;
	float margin = 0.0f;

public:
	JoltQueryCollectorMotion(const Vector3 &p_motion, float p_margin, int p_max_hits = TDefaultCapacity) :
			Base(p_max_hits),
			direction(p_motion.normalized()),
			distance_sq(p_motion.length_squared()),
			margin(p_margin) {}

	virtual void AddHit(const Hit &p_hit) override {
		const float penetration_depth = p_hit.mPenetrationDepth + margin;
		if (penetration_depth <= 0.0f) {
			return;
		}

		if (distance_sq > 0) {
			const Vector3 normal = to_godot(-p_hit.mPenetrationAxis.Normalized());
			if (direction.dot(normal) >= -CMP_EPSILON) {
				return;
			}
		}

		Base::AddHit(p_hit);
	}
};

// ============================================================================
// Space Subsystems (Allocator, JobSystem, Layers, Listeners, Filters)
// ============================================================================

class JoltTempAllocator final : public JPH::TempAllocator {
	uint64_t capacity = 0;
	uint64_t top = 0;
	uint8_t *base = nullptr;

public:
	explicit JoltTempAllocator();
	virtual ~JoltTempAllocator() override;

	virtual void *Allocate(JPH::uint p_size) override;
	virtual void Free(void *p_ptr, JPH::uint p_size) override;
};

class JoltJobSystem final : public JPH::JobSystemWithBarrier {
	class Job final : public JPH::JobSystem::Job {
		inline static std::atomic<Job *> completed_head = nullptr;

#ifdef DEBUG_ENABLED
		const char *name = nullptr;
#endif

		int64_t task_id = -1;

		std::atomic<Job *> completed_next = nullptr;

		static void _execute(void *p_user_data);

	public:
		Job(const char *p_name, JPH::ColorArg p_color, JPH::JobSystem *p_job_system, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count);
		Job(const Job &p_other) = delete;
		Job(Job &&p_other) = delete;
		~Job();

		static void push_completed(Job *p_job);
		static Job *pop_completed();

		void queue();

		Job &operator=(const Job &p_other) = delete;
		Job &operator=(Job &&p_other) = delete;
	};

#ifdef DEBUG_ENABLED
	inline static HashMap<const void *, uint64_t> timings_by_job;
	inline static SpinLock timings_lock;
#endif

	JPH::FixedSizeFreeList<Job> jobs;

	int thread_count = 0;

	virtual int GetMaxConcurrency() const override;

	virtual JPH::JobHandle CreateJob(const char *p_name, JPH::ColorArg p_color, const JPH::JobSystem::JobFunction &p_job_function, JPH::uint32 p_dependency_count = 0) override;
	virtual void QueueJob(JPH::JobSystem::Job *p_job) override;
	virtual void QueueJobs(JPH::JobSystem::Job **p_jobs, JPH::uint p_job_count) override;
	virtual void FreeJob(JPH::JobSystem::Job *p_job) override;

	void _reclaim_jobs();

public:
	JoltJobSystem();

	void pre_step();
	void post_step();

#ifdef DEBUG_ENABLED
	void flush_timings();
#endif
};

class JoltLayers final
		: public JPH::BroadPhaseLayerInterface,
		  public JPH::ObjectLayerPairFilter,
		  public JPH::ObjectVsBroadPhaseLayerFilter {
	LocalVector<uint64_t> collisions_by_layer;
	HashMap<uint64_t, JPH::ObjectLayer> layers_by_collision;
	JPH::ObjectLayer next_object_layer = 0;

	virtual uint32_t GetNumBroadPhaseLayers() const override;
	virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer p_layer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer p_layer) const override;
#endif

	virtual bool ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::ObjectLayer p_encoded_layer2) const override;
	virtual bool ShouldCollide(JPH::ObjectLayer p_encoded_layer1, JPH::BroadPhaseLayer p_broad_phase_layer2) const override;

	JPH::ObjectLayer _allocate_object_layer(uint64_t p_collision);

public:
	JoltLayers();
	virtual ~JoltLayers();

	JPH::ObjectLayer to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask);
	void from_object_layer(JPH::ObjectLayer p_encoded_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const;
};

class JoltBodyActivationListener3D final : public JPH::BodyActivationListener {
public:
	virtual void OnBodyActivated(const JPH::BodyID &p_body_id, JPH::uint64 p_body_user_data) override;
	virtual void OnBodyDeactivated(const JPH::BodyID &p_body_id, JPH::uint64 p_body_user_data) override {}
};

class JoltContactListener3D final : public JPH::ContactListener, public JPH::SoftBodyContactListener {
	struct BodyIDHasher {
		static uint32_t hash(const JPH::BodyID &p_id) {
			return hash_fmix32(p_id.GetIndexAndSequenceNumber());
		}
	};

	struct ShapePairHasher {
		static uint32_t hash(const JPH::SubShapeIDPair &p_pair) {
			uint32_t hash = hash_murmur3_one_32(p_pair.GetBody1ID().GetIndexAndSequenceNumber());
			hash = hash_murmur3_one_32(p_pair.GetSubShapeID1().GetValue(), hash);
			hash = hash_murmur3_one_32(p_pair.GetBody2ID().GetIndexAndSequenceNumber(), hash);
			hash = hash_murmur3_one_32(p_pair.GetSubShapeID2().GetValue(), hash);
			return hash_fmix32(hash);
		}
	};

	struct Contact {
		Vector3 point_self;
		Vector3 point_other;
		Vector3 normal;
		Vector3 velocity_self;
		Vector3 velocity_other;
		Vector3 impulse;
	};

	typedef LocalVector<Contact> Contacts;

	struct Manifold {
		Contacts contacts1;
		Contacts contacts2;
		float depth = 0.0f;
	};

	HashMap<JPH::SubShapeIDPair, Manifold, ShapePairHasher> manifolds_by_shape_pair;
	HashSet<JPH::SubShapeIDPair, ShapePairHasher> area_overlaps;
	HashSet<JPH::SubShapeIDPair, ShapePairHasher> area_enters;
	HashSet<JPH::SubShapeIDPair, ShapePairHasher> area_exits;
	LocalVector<JPH::SubShapeIDPair> area_soft_body_overlaps;
	Mutex write_mutex;
	JoltSpace3D *space = nullptr;

#ifdef DEBUG_ENABLED
	PackedVector3Array debug_contacts;
	std::atomic_int debug_contact_count = 0;
#endif

	virtual void OnContactAdded(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) override;
	virtual void OnContactPersisted(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings) override;
	virtual void OnContactRemoved(const JPH::SubShapeIDPair &p_shape_pair) override;

	virtual JPH::SoftBodyValidateResult OnSoftBodyContactValidate(const JPH::Body &p_soft_body, const JPH::Body &p_other_body, JPH::SoftBodyContactSettings &p_settings) override;
	virtual void OnSoftBodyContactAdded(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold) override;

	bool _try_override_collision_response(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings);
	bool _try_override_collision_response(const JPH::Body &p_jolt_soft_body, const JPH::Body &p_jolt_other_body, JPH::SoftBodyContactSettings &p_settings);
	bool _try_apply_surface_velocities(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, JPH::ContactSettings &p_settings);
	bool _try_add_contacts(const JPH::Body &p_jolt_body1, const JPH::Body &p_jolt_body2, const JPH::ContactManifold &p_manifold, JPH::ContactSettings &p_settings);
	bool _try_evaluate_area_overlap(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::SubShapeID &p_shape_id1, const JPH::SubShapeID &p_shape_id2);
	bool _try_remove_contacts(const JPH::SubShapeIDPair &p_shape_pair);
	bool _try_remove_area_overlap(const JPH::SubShapeIDPair &p_shape_pair);

#ifdef DEBUG_ENABLED
	bool _try_add_debug_contacts(const JPH::Body &p_body1, const JPH::Body &p_body2, const JPH::ContactManifold &p_manifold);
	bool _try_add_debug_contacts(const JPH::Body &p_soft_body, const JPH::SoftBodyManifold &p_manifold);
#endif

	bool _has_shape_shifted(const JoltObject3D &p_object, const JPH::SubShapeID &p_sub_shape_id);

	void _evaluate_area_overlap(const JoltArea3D &p_area, const JoltArea3D &p_other_area, const JPH::SubShapeIDPair &p_shape_pair);
	void _evaluate_area_overlap(const JoltArea3D &p_area, const JoltBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair);
	void _evaluate_area_overlap(const JoltArea3D &p_area, const JoltSoftBody3D &p_body, const JPH::SubShapeIDPair &p_shape_pair);

	void _flush_contacts();
	void _flush_area_enters();
	void _flush_area_exits();
	void _clear_area_soft_body_overlaps();

public:
	explicit JoltContactListener3D(JoltSpace3D *p_space) :
			space(p_space) {}

	void pre_step();
	void post_step();

#ifdef DEBUG_ENABLED
	const PackedVector3Array &get_debug_contacts() const { return debug_contacts; }
	int get_debug_contact_count() const { return debug_contact_count.load(std::memory_order_acquire); }
	int get_max_debug_contacts() const { return (int)debug_contacts.size(); }
	void set_max_debug_contacts(int p_count) { debug_contacts.resize(p_count); }
#endif
};

class JoltMotionFilter3D final
		: public JPH::BroadPhaseLayerFilter,
		  public JPH::ObjectLayerFilter,
		  public JPH::BodyFilter,
		  public JPH::ShapeFilter {
	const JoltBody3D &body_self;
	const JoltSpace3D &space;
	const HashSet<RID> &excluded_bodies;
	bool collide_separation_ray = false;

public:
	virtual bool ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const override;
	virtual bool ShouldCollide(JPH::ObjectLayer p_object_layer) const override;
	virtual bool ShouldCollide(const JPH::BodyID &p_jolt_id) const override;
	virtual bool ShouldCollideLocked(const JPH::Body &p_jolt_body) const override;
	virtual bool ShouldCollide(const JPH::Shape *p_jolt_shape, const JPH::SubShapeID &p_jolt_shape_id) const override;
	virtual bool ShouldCollide(const JPH::Shape *p_jolt_shape_self, const JPH::SubShapeID &p_jolt_shape_id_self, const JPH::Shape *p_jolt_shape_other, const JPH::SubShapeID &p_jolt_shape_id_other) const override;
};

class JoltQueryFilter3D final
		: public JPH::BroadPhaseLayerFilter,
		  public JPH::ObjectLayerFilter,
		  public JPH::BodyFilter {
	const JoltSpace3D &space;
	const HashSet<RID> &excluded;
	uint32_t collision_mask = 0;
	bool collide_with_bodies = false;
	bool collide_with_areas = false;
	bool picking = false;

public:
	JoltQueryFilter3D(const PhysicsDirectSpaceState3D &p_space_state, uint32_t p_collision_mask, bool p_collide_with_bodies, bool p_collide_with_areas, const HashSet<RID> &p_excluded, bool p_picking = false);

	virtual bool ShouldCollide(JPH::BroadPhaseLayer p_broad_phase_layer) const override;
	virtual bool ShouldCollide(JPH::ObjectLayer p_object_layer) const override;
	virtual bool ShouldCollide(const JPH::BodyID &p_body_id) const override;
	virtual bool ShouldCollideLocked(const JPH::Body &p_body) const override;
};

// ============================================================================
// JoltSpace3D
// ============================================================================

class JoltSpace3D final {
	Mutex pending_objects_mutex;
	Mutex body_call_queries_mutex;

	SelfList<JoltBody3D>::List body_call_queries_list;
	SelfList<JoltArea3D>::List area_call_queries_list;
	SelfList<JoltObject3D>::List shapes_changed_list;
	SelfList<JoltObject3D>::List needs_optimization_list;

	LocalVector<JPH::BodyID> pending_objects_sleeping;
	LocalVector<JPH::BodyID> pending_objects_awake;

	RID rid;

	JPH::JobSystem *job_system = nullptr;
	JPH::TempAllocator *temp_allocator = nullptr;
	JoltLayers *layers = nullptr;
	JoltContactListener3D *contact_listener = nullptr;
	JoltBodyActivationListener3D *body_activation_listener = nullptr;
	JPH::PhysicsSystem *physics_system = nullptr;
	PhysicsDirectSpaceState3D *direct_state = nullptr;
	JoltArea3D *default_area = nullptr;

	float last_step = 0.0f;

	uint16_t default_area_changed_count = 0;

	bool active = false;
	bool stepping = false;

	void _pre_step(float p_step);
	void _post_step(float p_step);

public:
	explicit JoltSpace3D(JPH::JobSystem *p_job_system, JPH::TempAllocator *p_temp_allocator);
	~JoltSpace3D();

	void step(float p_step);

	void call_queries();

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	bool is_active() const { return active; }
	void set_active(bool p_active) { active = p_active; }

	bool is_stepping() const { return stepping; }

	double get_param(PS3DE::SpaceParameter p_param) const;
	void set_param(PS3DE::SpaceParameter p_param, double p_value);

	JPH::PhysicsSystem &get_physics_system() const { return *physics_system; }
	JPH::TempAllocator &get_temp_allocator() const { return *temp_allocator; }

	JPH::BodyInterface &get_body_iface();
	const JPH::BodyInterface &get_body_iface() const;
	const JPH::BodyLockInterface &get_lock_iface() const;

	const JPH::BroadPhaseQuery &get_broad_phase_query() const;
	const JPH::NarrowPhaseQuery &get_narrow_phase_query() const;

	JPH::ObjectLayer map_to_object_layer(JPH::BroadPhaseLayer p_broad_phase_layer, uint32_t p_collision_layer, uint32_t p_collision_mask);
	void map_from_object_layer(JPH::ObjectLayer p_object_layer, JPH::BroadPhaseLayer &r_broad_phase_layer, uint32_t &r_collision_layer, uint32_t &r_collision_mask) const;

	JPH::Body *try_get_jolt_body(const JPH::BodyID &p_body_id) const;
	JoltObject3D *try_get_object(const JPH::BodyID &p_body_id) const;
	JoltBody3D *try_get_body(const JPH::BodyID &p_body_id) const;
	JoltArea3D *try_get_area(const JPH::BodyID &p_body_id) const;
	JoltSoftBody3D *try_get_soft_body(const JPH::BodyID &p_body_id) const;

	PhysicsDirectSpaceState3D *get_direct_state();

	JoltArea3D *get_default_area() const { return default_area; }
	void set_default_area(JoltArea3D *p_area) { default_area = p_area; }

	void increment_default_area_changed_count() { default_area_changed_count++; }
	uint16_t get_default_area_changed_count() const { return default_area_changed_count; }

	float get_last_step() const { return last_step; }

	JPH::Body *add_object(const JoltObject3D &p_object, const JPH::BodyCreationSettings &p_settings, bool p_sleeping = false);
	JPH::Body *add_object(const JoltObject3D &p_object, const JPH::SoftBodyCreationSettings &p_settings, bool p_sleeping = false);
	void remove_object(const JPH::BodyID &p_jolt_id);
	void flush_pending_objects();

	void set_is_object_sleeping(const JPH::BodyID &p_jolt_id, bool p_enable);

	void enqueue_call_queries(SelfList<JoltBody3D> *p_body);
	void enqueue_call_queries(SelfList<JoltArea3D> *p_area);
	void dequeue_call_queries(SelfList<JoltBody3D> *p_body);
	void dequeue_call_queries(SelfList<JoltArea3D> *p_area);

	void enqueue_shapes_changed(SelfList<JoltObject3D> *p_object);
	void dequeue_shapes_changed(SelfList<JoltObject3D> *p_object);

	void enqueue_needs_optimization(SelfList<JoltObject3D> *p_object);
	void dequeue_needs_optimization(SelfList<JoltObject3D> *p_object);

	void add_joint(JPH::Constraint *p_jolt_ref);
	void add_joint(JoltJoint3D *p_joint);
	void remove_joint(JPH::Constraint *p_jolt_ref);
	void remove_joint(JoltJoint3D *p_joint);

#ifdef DEBUG_ENABLED
	void dump_debug_snapshot(const String &p_dir);
	const PackedVector3Array &get_debug_contacts() const;
	int get_debug_contact_count() const;
	int get_max_debug_contacts() const;
	void set_max_debug_contacts(int p_count);
#endif
};
