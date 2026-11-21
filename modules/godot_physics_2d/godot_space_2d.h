/**************************************************************************/
/*  godot_space_2d.h                                                      */
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

#include "core/math/bvh.h"
#include "core/math/rect2.h"
#include "core/math/vector2.h"
#include "core/templates/local_vector.h"
#include "core/typedefs.h"
#include "godot_objects_2d.h"
#include "servers/physics_2d/direct_states/physics_direct_space_state_2d.h"

class GodotConstraint2D;

class GodotBroadPhase2D final
{
public:
	typedef uint32_t ID;

	typedef void* (*PairCallback)(GodotCollisionObject2D* A, int p_subindex_A, GodotCollisionObject2D* B, int p_subindex_B, void* p_userdata);
	typedef void (*UnpairCallback)(GodotCollisionObject2D* A, int p_subindex_A, GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_userdata);

private:
	template <typename T>
	class UserPairTestFunction
	{
	public:
		static bool user_pair_check(const T* p_a, const T* p_b)
		{
			// return false if no collision, decided by masks etc
			return p_a->interacts_with(p_b);
		}
	};

	template <typename T>
	class UserCullTestFunction
	{
	public:
		static bool user_cull_check(const T* p_a, const T* p_b)
		{
			return true;
		}
	};

	enum Tree
	{
		TREE_STATIC = 0,
		TREE_DYNAMIC = 1,
	};

	enum TreeFlag
	{
		TREE_FLAG_STATIC = 1 << TREE_STATIC,
		TREE_FLAG_DYNAMIC = 1 << TREE_DYNAMIC,
	};

	BVH_Manager<GodotCollisionObject2D, 2, true, 128, UserPairTestFunction<GodotCollisionObject2D>, UserCullTestFunction<GodotCollisionObject2D>, Rect2, Vector2> bvh;

	static void* _pair_callback(void*, uint32_t, GodotCollisionObject2D*, int, uint32_t, GodotCollisionObject2D*, int);
	static void _unpair_callback(void*, uint32_t, GodotCollisionObject2D*, int, uint32_t, GodotCollisionObject2D*, int, void*);

	PairCallback pair_callback = nullptr;
	void* pair_userdata = nullptr;
	UnpairCallback unpair_callback = nullptr;
	void* unpair_userdata = nullptr;

public:
	// 0 is an invalid ID
	ID create(GodotCollisionObject2D* p_object, int p_subindex = 0, const Rect2& p_aabb = Rect2(), bool p_static = false);
	void move(ID p_id, const Rect2& p_aabb);
	void set_static(ID p_id, bool p_static);
	void remove(ID p_id);

	GodotCollisionObject2D* get_object(ID p_id) const;
	bool is_static(ID p_id) const;
	int get_subindex(ID p_id) const;

	int cull_segment(const Vector2& p_from, const Vector2& p_to, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices = nullptr);
	int cull_aabb(const Rect2& p_aabb, GodotCollisionObject2D** p_results, int p_max_results, int* p_result_indices = nullptr);

	void set_pair_callback(PairCallback p_pair_callback, void* p_userdata);
	void set_unpair_callback(UnpairCallback p_unpair_callback, void* p_userdata);

	void update();

	GodotBroadPhase2D();
	~GodotBroadPhase2D() = default;
};

class GodotSpace2D final
{
public:
	enum ElapsedTime
	{
		ELAPSED_TIME_INTEGRATE_FORCES,
		ELAPSED_TIME_GENERATE_ISLANDS,
		ELAPSED_TIME_SETUP_CONSTRAINTS,
		ELAPSED_TIME_SOLVE_CONSTRAINTS,
		ELAPSED_TIME_INTEGRATE_VELOCITIES,
		ELAPSED_TIME_MAX
	};

private:
	struct ExcludedShapeSW
	{
		GodotShape2D* local_shape = nullptr;
		const GodotCollisionObject2D* against_object = nullptr;
		int against_shape_index = 0;
	};

	uint64_t elapsed_time[ELAPSED_TIME_MAX] = {};

	PhysicsDirectSpaceState2D* direct_access = nullptr;
	RID self;

	GodotBroadPhase2D* broadphase = nullptr;
	SelfList<GodotBody2D>::List active_list;
	SelfList<GodotBody2D>::List mass_properties_update_list;
	SelfList<GodotBody2D>::List state_query_list;
	SelfList<GodotArea2D>::List monitor_query_list;
	SelfList<GodotArea2D>::List area_moved_list;

	static void* _broadphase_pair(GodotCollisionObject2D* A, int p_subindex_A,
		GodotCollisionObject2D* B, int p_subindex_B, void* p_self);
	static void _broadphase_unpair(GodotCollisionObject2D* A, int p_subindex_A,
		GodotCollisionObject2D* B, int p_subindex_B, void* p_data, void* p_self);

	HashSet<GodotCollisionObject2D*> objects;

	GodotArea2D* area = nullptr;

	int solver_iterations = 0;

	real_t contact_recycle_radius = 0.0;
	real_t contact_max_separation = 0.0;
	real_t contact_max_allowed_penetration = 0.0;
	real_t contact_bias = 0.0;
	real_t constraint_bias = 0.0;

	enum
	{
		INTERSECTION_QUERY_MAX = 2048
	};

	GodotCollisionObject2D* intersection_query_results[INTERSECTION_QUERY_MAX];
	int intersection_query_subindex_results[INTERSECTION_QUERY_MAX];

	real_t body_linear_velocity_sleep_threshold = 0.0;
	real_t body_angular_velocity_sleep_threshold = 0.0;
	real_t body_time_to_sleep = 0.0;

	bool locked = false;

	real_t last_step = 0.001;

	int island_count = 0;
	int active_objects = 0;
	int collision_pairs = 0;

	int _cull_aabb_for_body(GodotBody2D* p_body, const Rect2& p_aabb);

	Vector<Vector2> contact_debug;
	int contact_debug_count = 0;

	friend class PhysicsDirectSpaceState2D;

public:
	_FORCE_INLINE_ void set_self(const RID& p_self) { self = p_self; }

	_FORCE_INLINE_ RID get_self() const { return self; }

	void set_default_area(GodotArea2D* p_area) { area = p_area; }

	GodotArea2D* get_default_area() const { return area; }

	const SelfList<GodotBody2D>::List& get_active_body_list() const;
	void body_add_to_active_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_active_list(SelfList<GodotBody2D>* p_body);
	void body_add_to_mass_properties_update_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_mass_properties_update_list(SelfList<GodotBody2D>* p_body);
	void area_add_to_moved_list(SelfList<GodotArea2D>* p_area);
	void area_remove_from_moved_list(SelfList<GodotArea2D>* p_area);
	const SelfList<GodotArea2D>::List& get_moved_area_list() const;

	void body_add_to_state_query_list(SelfList<GodotBody2D>* p_body);
	void body_remove_from_state_query_list(SelfList<GodotBody2D>* p_body);

	void area_add_to_monitor_query_list(SelfList<GodotArea2D>* p_area);
	void area_remove_from_monitor_query_list(SelfList<GodotArea2D>* p_area);

	GodotBroadPhase2D* get_broadphase();

	void add_object(GodotCollisionObject2D* p_object);
	void remove_object(GodotCollisionObject2D* p_object);
	const HashSet<GodotCollisionObject2D*>& get_objects() const;

	_FORCE_INLINE_ int get_solver_iterations() const { return solver_iterations; }

	_FORCE_INLINE_ real_t get_contact_recycle_radius() const { return contact_recycle_radius; }

	_FORCE_INLINE_ real_t get_contact_max_separation() const { return contact_max_separation; }

	_FORCE_INLINE_ real_t get_contact_max_allowed_penetration() const
	{
		return contact_max_allowed_penetration;
	}

	_FORCE_INLINE_ real_t get_contact_bias() const { return contact_bias; }

	_FORCE_INLINE_ real_t get_constraint_bias() const { return constraint_bias; }

	_FORCE_INLINE_ real_t get_body_linear_velocity_sleep_threshold() const
	{
		return body_linear_velocity_sleep_threshold;
	}

	_FORCE_INLINE_ real_t get_body_angular_velocity_sleep_threshold() const
	{
		return body_angular_velocity_sleep_threshold;
	}

	_FORCE_INLINE_ real_t get_body_time_to_sleep() const { return body_time_to_sleep; }

	void update();
	void setup();
	void call_queries();

	bool is_locked() const;
	void lock();
	void unlock();

	real_t get_last_step() const { return last_step; }

	void set_last_step(real_t p_step) { last_step = p_step; }

	void set_param(PS2DE::SpaceParameter p_param, real_t p_value);
	real_t get_param(PS2DE::SpaceParameter p_param) const;

	void set_island_count(int p_island_count) { island_count = p_island_count; }

	int get_island_count() const { return island_count; }

	void set_active_objects(int p_active_objects) { active_objects = p_active_objects; }

	int get_active_objects() const { return active_objects; }

	int get_collision_pairs() const { return collision_pairs; }

	bool test_body_motion(GodotBody2D* p_body, const PS2DT::MotionParameters& p_parameters,
		PS2DT::MotionResult* r_result);

	void set_debug_contacts(int p_amount) { contact_debug.resize(p_amount); }

	_FORCE_INLINE_ bool is_debugging_contacts() const { return !contact_debug.is_empty(); }

	_FORCE_INLINE_ void add_debug_contact(const Vector2& p_contact)
	{
		if (contact_debug_count < contact_debug.size()) {
			contact_debug.write[contact_debug_count++] = p_contact;
		}
	}

	_FORCE_INLINE_ Vector<Vector2> get_debug_contacts() { return contact_debug; }

	_FORCE_INLINE_ int get_debug_contact_count() { return contact_debug_count; }

	PhysicsDirectSpaceState2D* get_direct_state();

	void set_elapsed_time(ElapsedTime p_time, uint64_t p_msec) { elapsed_time[p_time] = p_msec; }

	uint64_t get_elapsed_time(ElapsedTime p_time) const { return elapsed_time[p_time]; }

	GodotSpace2D();
	~GodotSpace2D();
};

class GodotStep2D final
{
	uint64_t _step = 1;

	int iterations = 0;
	real_t delta = 0.0;

	LocalVector<LocalVector<GodotBody2D*>> body_islands;
	LocalVector<LocalVector<GodotConstraint2D*>> constraint_islands;
	LocalVector<GodotConstraint2D*> all_constraints;

	void _populate_island(GodotBody2D* p_body, LocalVector<GodotBody2D*>& p_body_island, LocalVector<GodotConstraint2D*>& p_constraint_island);
	void _setup_constraint(uint32_t p_constraint_index, void* p_userdata = nullptr);
	void _pre_solve_island(LocalVector<GodotConstraint2D*>& p_constraint_island) const;
	void _solve_island(uint32_t p_island_index, void* p_userdata = nullptr) const;
	void _check_suspend(LocalVector<GodotBody2D*>& p_body_island) const;

public:
	void step(GodotSpace2D* p_space, real_t p_delta);
	GodotStep2D();
	~GodotStep2D();
};
