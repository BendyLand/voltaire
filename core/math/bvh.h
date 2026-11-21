/**************************************************************************/
/*  bvh.h                                                                 */
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

// BVH
// This class provides a wrapper around BVH tree, which contains most of the functionality
// for a dynamic BVH with templated leaf size.
// However BVH also adds facilities for pairing, to maintain compatibility with Godot 3.2.
// Pairing is a collision pairing system, on top of the basic BVH.

// Some notes on the use of BVH / Octree from Godot 3.2.
// This is not well explained elsewhere.
// The rendering tree mask and types that are sent to the BVH are NOT layer masks.
// They are INSTANCE_TYPES (defined in visual_server.h), e.g. MESH, MULTIMESH, PARTICLES etc.
// Thus the lights do no cull by layer mask in the BVH.

// Layer masks are implemented in the renderers as a later step, and light_cull_mask appears to be
// implemented in GLES3 but not GLES2. Layer masks are not yet implemented for directional lights.

// In the physics, the pairable_type is based on 1 << p_object->get_type() where:
// TYPE_AREA,
// TYPE_BODY
// and pairable_mask is either 0 if static, or set to all if non static
// --- Folded from core/math/bvh_tree.h ---
// BVH Tree
// This is an implementation of a dynamic BVH with templated leaf size.
// This differs from most dynamic BVH in that it can handle more than 1 object
// in leaf nodes. This can make it far more efficient in certain circumstances.
// It also means that the splitting logic etc have to be completely different
// to a simpler tree.
// Note that MAX_CHILDREN should be fixed at 2 for now.

#include "core/math/aabb.h"
// --- Folded from core/math/bvh_abb.h ---
#include "core/math/aabb.h"

#include <cfloat> // FLT_MAX

// special optimized version of axis aligned bounding box
template <typename BOUNDS = AABB, typename POINT = Vector3>
struct BVH_ABB {
	struct ConvexHull {
		// convex hulls (optional)
		const Plane *planes;
		int num_planes;
		const Vector3 *points;
		int num_points;
	};

	struct Segment {
		POINT from;
		POINT to;
	};

	enum IntersectResult {
		IR_MISS = 0,
		IR_PARTIAL,
		IR_FULL,
	};

	// we store mins with a negative value in order to test them with SIMD
	POINT min;
	POINT neg_max;

	bool operator==(const BVH_ABB &p_other) const { return (min == p_other.min) && (neg_max == p_other.neg_max); }
	bool operator!=(const BVH_ABB &p_other) const { return (*this == p_other) == false; }

	void set(const POINT &p_min, const POINT &p_max) {
		min = p_min;
		neg_max = -p_max;
	}

	// to and from standard AABB
	void from(const BOUNDS &p_aabb) {
		min = p_aabb.position;
		neg_max = -(p_aabb.position + p_aabb.size);
	}

	void to(BOUNDS &r_aabb) const {
		r_aabb.position = min;
		r_aabb.size = calculate_size();
	}

	void merge(const BVH_ABB &p_o) {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			neg_max[axis] = MIN(neg_max[axis], p_o.neg_max[axis]);
			min[axis] = MIN(min[axis], p_o.min[axis]);
		}
	}

	POINT calculate_size() const {
		return -neg_max - min;
	}

	POINT calculate_center() const {
		return POINT((calculate_size() * 0.5) + min);
	}

	real_t get_proximity_to(const BVH_ABB &p_b) const {
		const POINT d = (min - neg_max) - (p_b.min - p_b.neg_max);
		real_t proximity = 0.0;
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			proximity += Math::abs(d[axis]);
		}
		return proximity;
	}

	int select_by_proximity(const BVH_ABB &p_a, const BVH_ABB &p_b) const {
		return (get_proximity_to(p_a) < get_proximity_to(p_b) ? 0 : 1);
	}

	uint32_t find_cutting_planes(const typename BVH_ABB::ConvexHull &p_hull, uint32_t *p_plane_ids) const {
		uint32_t count = 0;

		for (int n = 0; n < p_hull.num_planes; n++) {
			const Plane &p = p_hull.planes[n];
			if (intersects_plane(p)) {
				p_plane_ids[count++] = n;
			}
		}

		return count;
	}

	bool intersects_plane(const Plane &p_p) const {
		Vector3 size = calculate_size();
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = min + half_extents;

		// forward side of plane?
		Vector3 point_offset(
				(p_p.normal.x < 0) ? -half_extents.x : half_extents.x,
				(p_p.normal.y < 0) ? -half_extents.y : half_extents.y,
				(p_p.normal.z < 0) ? -half_extents.z : half_extents.z);
		Vector3 point = point_offset + ofs;

		if (!p_p.is_point_over(point)) {
			return false;
		}

		point = -point_offset + ofs;
		if (p_p.is_point_over(point)) {
			return false;
		}

		return true;
	}

	bool intersects_convex_optimized(const ConvexHull &p_hull, const uint32_t *p_plane_ids, uint32_t p_num_planes) const {
		Vector3 size = calculate_size();
		Vector3 half_extents = size * 0.5;
		Vector3 ofs = min + half_extents;

		for (unsigned int i = 0; i < p_num_planes; i++) {
			const Plane &p = p_hull.planes[p_plane_ids[i]];
			Vector3 point(
					(p.normal.x > 0) ? -half_extents.x : half_extents.x,
					(p.normal.y > 0) ? -half_extents.y : half_extents.y,
					(p.normal.z > 0) ? -half_extents.z : half_extents.z);
			point += ofs;
			if (p.is_point_over(point)) {
				return false;
			}
		}

		return true;
	}

	bool intersects_convex_partial(const ConvexHull &p_hull) const {
		BOUNDS bb;
		to(bb);
		return bb.intersects_convex_shape(p_hull.planes, p_hull.num_planes, p_hull.points, p_hull.num_points);
	}

	IntersectResult intersects_convex(const ConvexHull &p_hull) const {
		if (intersects_convex_partial(p_hull)) {
			// fully within? very important for tree checks
			if (is_within_convex(p_hull)) {
				return IR_FULL;
			}

			return IR_PARTIAL;
		}

		return IR_MISS;
	}

	bool is_within_convex(const ConvexHull &p_hull) const {
		// use half extents routine
		BOUNDS bb;
		to(bb);
		return bb.inside_convex_shape(p_hull.planes, p_hull.num_planes);
	}

	bool is_point_within_hull(const ConvexHull &p_hull, const Vector3 &p_pt) const {
		for (int n = 0; n < p_hull.num_planes; n++) {
			if (p_hull.planes[n].distance_to(p_pt) > 0.0f) {
				return false;
			}
		}
		return true;
	}

	bool intersects_segment(const Segment &p_s) const {
		BOUNDS bb;
		to(bb);
		return bb.intersects_segment(p_s.from, p_s.to);
	}

	bool intersects_point(const POINT &p_pt) const {
		if (_any_lessthan(-p_pt, neg_max)) {
			return false;
		}
		if (_any_lessthan(p_pt, min)) {
			return false;
		}
		return true;
	}

	// Very hot in profiling, make sure optimized
	bool intersects(const BVH_ABB &p_o) const {
		if (_any_morethan(p_o.min, -neg_max)) {
			return false;
		}
		if (_any_morethan(min, -p_o.neg_max)) {
			return false;
		}
		return true;
	}

	// for pre-swizzled tester (this object)
	bool intersects_swizzled(const BVH_ABB &p_o) const {
		if (_any_lessthan(min, p_o.min)) {
			return false;
		}
		if (_any_lessthan(neg_max, p_o.neg_max)) {
			return false;
		}
		return true;
	}

	bool is_other_within(const BVH_ABB &p_o) const {
		if (_any_lessthan(p_o.neg_max, neg_max)) {
			return false;
		}
		if (_any_lessthan(p_o.min, min)) {
			return false;
		}
		return true;
	}

	void grow(const POINT &p_change) {
		neg_max -= p_change;
		min -= p_change;
	}

	void expand(real_t p_change) {
		POINT change;
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			change[axis] = p_change;
		}
		grow(change);
	}

	// Actually surface area metric.
	real_t get_area() const {
		POINT d = calculate_size();
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	void set_to_max_opposite_extents() {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			neg_max[axis] = FLT_MAX;
		}
		min = neg_max;
	}

	bool _any_morethan(const POINT &p_a, const POINT &p_b) const {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			if (p_a[axis] > p_b[axis]) {
				return true;
			}
		}
		return false;
	}

	bool _any_lessthan(const POINT &p_a, const POINT &p_b) const {
		for (int axis = 0; axis < POINT::AXIS_COUNT; ++axis) {
			if (p_a[axis] < p_b[axis]) {
				return true;
			}
		}
		return false;
	}
};

// --- End core/math/bvh_abb.h ---
#include "core/math/vector3.h"
#include "core/templates/local_vector.h"
#include "core/templates/pooled_list.h"

#define BVHABB_CLASS BVH_ABB<BOUNDS, POINT>

// not sure if this is better yet so making optional
#define BVH_EXPAND_LEAF_AABBS

// never do these checks in release
#ifdef DEV_ENABLED
//#define BVH_VERBOSE
//#define BVH_VERBOSE_TREE
//#define BVH_VERBOSE_PAIRING
//#define BVH_VERBOSE_MOVES

//#define BVH_VERBOSE_FRAME
//#define BVH_CHECKS
//#define BVH_INTEGRITY_CHECKS
#endif

// debug only assert
#ifdef BVH_CHECKS
#define BVH_ASSERT(a) CRASH_COND((a) == false)
#else
#define BVH_ASSERT(a)
#endif

#ifdef BVH_VERBOSE
#define VERBOSE_PRINT print_line
#else
#define VERBOSE_PRINT(a)
#endif

// really just a namespace
struct BVHCommon {
	// these could possibly also be the same constant,
	// although this may be useful for debugging.
	// or use zero for invalid and +1 based indices.
	static const uint32_t INVALID = (0xffffffff);
	static const uint32_t INACTIVE = (0xfffffffe);
};

// really a handle, can be anything
// note that zero is a valid reference for the BVH .. this may involve using
// a plus one based ID for clients that expect 0 to be invalid.
struct BVHHandle {
	// conversion operator
	operator uint32_t() const { return _data; }
	void set(uint32_t p_value) { _data = p_value; }

	uint32_t _data;

	void set_invalid() { _data = BVHCommon::INVALID; }
	bool is_invalid() const { return _data == BVHCommon::INVALID; }
	uint32_t id() const { return _data; }
	void set_id(uint32_t p_id) { _data = p_id; }

	bool operator==(const BVHHandle &p_h) const { return _data == p_h._data; }
	bool operator!=(const BVHHandle &p_h) const { return (*this == p_h) == false; }
};

// helper class to make iterative versions of recursive functions
template <typename T>
class BVH_IterativeInfo {
public:
	constexpr static const size_t ALLOCA_STACK_SIZE = 128;

	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;
	T *stack;
	//only used in rare occasions when you run out of alloca memory
	// because tree is too unbalanced.
	LocalVector<T> aux_stack;
	int32_t get_alloca_stacksize() const { return ALLOCA_STACK_SIZE * sizeof(T); }

	T *get_first() const {
		return &stack[0];
	}

	// pop the last member of the stack, or return false
	bool pop(T &r_value) {
		if (!depth) {
			return false;
		}

		depth--;
		r_value = stack[depth];
		return true;
	}

	// request new addition to stack
	T *request() {
		if (depth > threshold) {
			if (aux_stack.is_empty()) {
				aux_stack.resize(ALLOCA_STACK_SIZE * 2);
				memcpy(aux_stack.ptr(), stack, get_alloca_stacksize());
			} else {
				aux_stack.resize(aux_stack.size() * 2);
			}
			stack = aux_stack.ptr();
			threshold = aux_stack.size() - 2;
		}
		return &stack[depth++];
	}
};

template <typename T>
class BVH_DummyPairTestFunction {
public:
	static bool user_collision_check(T *p_a, T *p_b) {
		// return false if no collision, decided by masks etc
		return true;
	}
};

template <typename T>
class BVH_DummyCullTestFunction {
public:
	static bool user_cull_check(T *p_a, T *p_b) {
		// return false if no collision
		return true;
	}
};

template <typename T, int NUM_TREES, int MAX_CHILDREN, int MAX_ITEMS, typename USER_PAIR_TEST_FUNCTION = BVH_DummyPairTestFunction<T>, typename USER_CULL_TEST_FUNCTION = BVH_DummyCullTestFunction<T>, bool USE_PAIRS = false, typename BOUNDS = AABB, typename POINT = Vector3>
class BVH_Tree {
	friend class BVH;

#include "core/math/bvh_pair.inc"
#include "core/math/bvh_structs.inc"

public:
	BVH_Tree() {
		for (int n = 0; n < NUM_TREES; n++) {
			_root_node_id[n] = BVHCommon::INVALID;
		}

		// disallow zero leaf ids
		// (as these ids are stored as negative numbers in the node)
		uint32_t dummy_leaf_id;
		_leaves.request(dummy_leaf_id);

		// In many cases you may want to change this default in the client code,
		// or expose this value to the user.
		// This default may make sense for a typically scaled 3d game, but maybe not for 2d on a pixel scale.
		params_set_pairing_expansion(0.1);
	}

private:
	bool node_add_child(uint32_t p_node_id, uint32_t p_child_node_id) {
		TNode &tnode = _nodes[p_node_id];
		if (tnode.is_full_of_children()) {
			return false;
		}

		tnode.children[tnode.num_children] = p_child_node_id;
		tnode.num_children += 1;

		// back link in the child to the parent
		TNode &tnode_child = _nodes[p_child_node_id];
		tnode_child.parent_id = p_node_id;

		return true;
	}

	void node_replace_child(uint32_t p_parent_id, uint32_t p_old_child_id, uint32_t p_new_child_id) {
		TNode &parent = _nodes[p_parent_id];
		BVH_ASSERT(!parent.is_leaf());

		int child_num = parent.find_child(p_old_child_id);
		BVH_ASSERT(child_num != -1);
		parent.children[child_num] = p_new_child_id;

		TNode &new_child = _nodes[p_new_child_id];
		new_child.parent_id = p_parent_id;
	}

	void node_remove_child(uint32_t p_parent_id, uint32_t p_child_id, uint32_t p_tree_id, bool p_prevent_sibling = false) {
		TNode &parent = _nodes[p_parent_id];
		BVH_ASSERT(!parent.is_leaf());

		int child_num = parent.find_child(p_child_id);
		BVH_ASSERT(child_num != -1);

		parent.remove_child_internal(child_num);

		// no need to keep back references for children at the moment

		uint32_t sibling_id = 0; // always a node id, as tnode is never a leaf
		bool sibling_present = false;

		// if there are more children, or this is the root node, don't try and delete
		if (parent.num_children > 1) {
			return;
		}

		// if there is 1 sibling, it can be moved to be a child of the
		if (parent.num_children == 1) {
			// else there is now a redundant node with one child, which can be removed
			sibling_id = parent.children[0];
			sibling_present = true;
		}

		// now there may be no children in this node .. in which case it can be deleted
		// remove node if empty
		// remove link from parent
		uint32_t grandparent_id = parent.parent_id;

		// special case for root node
		if (grandparent_id == BVHCommon::INVALID) {
			if (sibling_present) {
				// change the root node
				change_root_node(sibling_id, p_tree_id);

				// delete the old root node as no longer needed
				node_free_node_and_leaf(p_parent_id);
			}

			return;
		}

		if (sibling_present) {
			node_replace_child(grandparent_id, p_parent_id, sibling_id);
		} else {
			node_remove_child(grandparent_id, p_parent_id, p_tree_id, true);
		}

		// put the node on the free list to recycle
		node_free_node_and_leaf(p_parent_id);
	}

	// A node can either be a node, or a node AND a leaf combo.
	// Both must be deleted to prevent a leak.
	void node_free_node_and_leaf(uint32_t p_node_id) {
		TNode &node = _nodes[p_node_id];
		if (node.is_leaf()) {
			int leaf_id = node.get_leaf_id();
			_leaves.free(leaf_id);
		}

		_nodes.free(p_node_id);
	}

	void change_root_node(uint32_t p_new_root_id, uint32_t p_tree_id) {
		_root_node_id[p_tree_id] = p_new_root_id;
		TNode &root = _nodes[p_new_root_id];

		// mark no parent
		root.parent_id = BVHCommon::INVALID;
	}

	void node_make_leaf(uint32_t p_node_id) {
		uint32_t child_leaf_id;
		TLeaf *child_leaf = _leaves.request(child_leaf_id);
		child_leaf->clear();

		// zero is reserved at startup, to prevent this id being used
		// (as they are stored as negative values in the node, and zero is already taken)
		BVH_ASSERT(child_leaf_id != 0);

		TNode &node = _nodes[p_node_id];
		node.neg_leaf_id = -(int)child_leaf_id;
	}

	void node_remove_item(uint32_t p_ref_id, uint32_t p_tree_id, BVHABB_CLASS *r_old_aabb = nullptr) {
		// get the reference
		ItemRef &ref = _refs[p_ref_id];
		uint32_t owner_node_id = ref.tnode_id;

		// debug draw special
		// This may not be needed
		if (owner_node_id == BVHCommon::INVALID) {
			return;
		}

		TNode &tnode = _nodes[owner_node_id];
		CRASH_COND(!tnode.is_leaf());

		TLeaf &leaf = _node_get_leaf(tnode);

		// if the aabb is not determining the corner size, then there is no need to refit!
		// (optimization, as merging AABBs takes a lot of time)
		const BVHABB_CLASS &old_aabb = leaf.get_aabb(ref.item_id);

		// shrink a little to prevent using corner aabbs
		// in order to miss the corners first we shrink by node_expansion
		// (which is added to the overall bound of the leaf), then we also
		// shrink by an epsilon, in order to miss out the very corner aabbs
		// which are important in determining the bound. Any other aabb
		// within this can be removed and not affect the overall bound.
		BVHABB_CLASS node_bound = tnode.aabb;
		node_bound.expand(-_node_expansion - 0.001f);
		bool refit = true;

		if (node_bound.is_other_within(old_aabb)) {
			refit = false;
		}

		// record the old aabb if required (for incremental remove_and_reinsert)
		if (r_old_aabb) {
			*r_old_aabb = old_aabb;
		}

		leaf.remove_item_unordered(ref.item_id);

		if (leaf.num_items) {
			// the swapped item has to have its reference changed to, to point to the new item id
			uint32_t swapped_ref_id = leaf.get_item_ref_id(ref.item_id);

			ItemRef &swapped_ref = _refs[swapped_ref_id];

			swapped_ref.item_id = ref.item_id;

			// only have to refit if it is an edge item
			// This is a VERY EXPENSIVE STEP
			// we defer the refit updates until the update function is called once per frame
			if (refit) {
				leaf.set_dirty(true);
			}
		} else {
			// remove node if empty
			// remove link from parent
			if (tnode.parent_id != BVHCommon::INVALID) {
				// DANGER .. this can potentially end up with root node with 1 child ...
				// we don't want this and must check for it

				uint32_t parent_id = tnode.parent_id;

				node_remove_child(parent_id, owner_node_id, p_tree_id);
				refit_upward(parent_id);

				// put the node on the free list to recycle
				node_free_node_and_leaf(owner_node_id);
			}

			// else if no parent, it is the root node. Do not delete
		}

		ref.tnode_id = BVHCommon::INVALID;
		ref.item_id = BVHCommon::INVALID; // unset
	}

	// returns true if needs refit of PARENT tree only, the node itself AABB is calculated
	// within this routine
	bool _node_add_item(uint32_t p_node_id, uint32_t p_ref_id, const BVHABB_CLASS &p_aabb) {
		ItemRef &ref = _refs[p_ref_id];
		ref.tnode_id = p_node_id;

		TNode &node = _nodes[p_node_id];
		BVH_ASSERT(node.is_leaf());
		TLeaf &leaf = _node_get_leaf(node);

		// optimization - we only need to do a refit
		// if the added item is changing the AABB of the node.
		// in most cases it won't.
		bool needs_refit = true;

		// expand bound now
		BVHABB_CLASS expanded = p_aabb;
		expanded.expand(_node_expansion);

		// the bound will only be valid if there is an item in there already
		if (leaf.num_items) {
			if (node.aabb.is_other_within(expanded)) {
				// no change to node AABBs
				needs_refit = false;
			} else {
				node.aabb.merge(expanded);
			}
		} else {
			// bound of the node = the new aabb
			node.aabb = expanded;
		}

		ref.item_id = leaf.request_item();
		BVH_ASSERT(ref.item_id != BVHCommon::INVALID);

		// set the aabb of the new item
		leaf.get_aabb(ref.item_id) = p_aabb;

		// back reference on the item back to the item reference
		leaf.get_item_ref_id(ref.item_id) = p_ref_id;

		return needs_refit;
	}

	uint32_t _node_create_another_child(uint32_t p_node_id, const BVHABB_CLASS &p_aabb) {
		uint32_t child_node_id;
		TNode *child_node = _nodes.request(child_node_id);
		child_node->clear();

		// may not be necessary
		child_node->aabb = p_aabb;

		node_add_child(p_node_id, child_node_id);

		return child_node_id;
	}

#include "core/math/bvh_cull.inc"
#include "core/math/bvh_debug.inc"
#include "core/math/bvh_integrity.inc"
#include "core/math/bvh_logic.inc"
#include "core/math/bvh_misc.inc"
#include "core/math/bvh_public.inc"
#include "core/math/bvh_refit.inc"
#include "core/math/bvh_split.inc"
};

#undef VERBOSE_PRINT

// --- End core/math/bvh_tree.h ---
#include "core/math/geometry_3d.h"
#include "core/os/mutex.h"

#include <climits> // INT_MAX

#define BVHTREE_CLASS BVH_Tree<T, NUM_TREES, 2, MAX_ITEMS, USER_PAIR_TEST_FUNCTION, USER_CULL_TEST_FUNCTION, USE_PAIRS, BOUNDS, POINT>
#define BVH_LOCKED_FUNCTION BVHLockedFunction _lock_guard(&_mutex, BVH_THREAD_SAFE &&_thread_safe);

template <typename T, int NUM_TREES = 1, bool USE_PAIRS = false, int MAX_ITEMS = 32, typename USER_PAIR_TEST_FUNCTION = BVH_DummyPairTestFunction<T>, typename USER_CULL_TEST_FUNCTION = BVH_DummyCullTestFunction<T>, typename BOUNDS = AABB, typename POINT = Vector3, bool BVH_THREAD_SAFE = true>
class BVH_Manager {
public:
	// note we are using uint32_t instead of BVHHandle, losing type safety, but this
	// is for compatibility with octree
	typedef void *(*PairCallback)(void *, uint32_t, T *, int, uint32_t, T *, int);
	typedef void (*UnpairCallback)(void *, uint32_t, T *, int, uint32_t, T *, int, void *);
	typedef void *(*CheckPairCallback)(void *, uint32_t, T *, int, uint32_t, T *, int, void *);

	// allow locally toggling thread safety if the template has been compiled with BVH_THREAD_SAFE
	void params_set_thread_safe(bool p_enable) {
		_thread_safe = p_enable;
	}

	// these 2 are crucial for fine tuning, and can be applied manually
	// see the variable declarations for more info.
	void params_set_node_expansion(real_t p_value) {
		BVH_LOCKED_FUNCTION
		if (p_value >= 0.0) {
			tree._node_expansion = p_value;
			tree._auto_node_expansion = false;
		} else {
			tree._auto_node_expansion = true;
		}
	}

	void params_set_pairing_expansion(real_t p_value) {
		BVH_LOCKED_FUNCTION
		tree.params_set_pairing_expansion(p_value);
	}

	void set_pair_callback(PairCallback p_callback, void *p_userdata) {
		BVH_LOCKED_FUNCTION
		pair_callback = p_callback;
		pair_callback_userdata = p_userdata;
	}
	void set_unpair_callback(UnpairCallback p_callback, void *p_userdata) {
		BVH_LOCKED_FUNCTION
		unpair_callback = p_callback;
		unpair_callback_userdata = p_userdata;
	}
	void set_check_pair_callback(CheckPairCallback p_callback, void *p_userdata) {
		BVH_LOCKED_FUNCTION
		check_pair_callback = p_callback;
		check_pair_callback_userdata = p_userdata;
	}

	BVHHandle create(T *p_userdata, bool p_active = true, uint32_t p_tree_id = 0, uint32_t p_tree_collision_mask = 1, const BOUNDS &p_aabb = BOUNDS(), int p_subindex = 0) {
		BVH_LOCKED_FUNCTION

		// not sure if absolutely necessary to flush collisions here. It will cost performance to, instead
		// of waiting for update, so only uncomment this if there are bugs.
		if (USE_PAIRS) {
			//_check_for_collisions();
		}

		BVHHandle h = tree.item_add(p_userdata, p_active, p_aabb, p_subindex, p_tree_id, p_tree_collision_mask);

		if (USE_PAIRS) {
			// for safety initialize the expanded AABB
			BOUNDS &expanded_aabb = tree._pairs[h.id()].expanded_aabb;
			expanded_aabb = p_aabb;
			expanded_aabb.grow_by(tree._pairing_expansion);

			// force a collision check no matter the AABB
			if (p_active) {
				_add_changed_item(h, p_aabb, false);
				_check_for_collisions(true);
			}
		}

		return h;
	}

	////////////////////////////////////////////////////
	// wrapper versions that use uint32_t instead of handle
	// for backward compatibility. Less type safe
	void move(uint32_t p_handle, const BOUNDS &p_aabb) {
		BVHHandle h;
		h.set(p_handle);
		move(h, p_aabb);
	}

	void recheck_pairs(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		recheck_pairs(h);
	}

	void erase(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		erase(h);
	}

	void force_collision_check(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		force_collision_check(h);
	}

	bool activate(uint32_t p_handle, const BOUNDS &p_aabb, bool p_delay_collision_check = false) {
		BVHHandle h;
		h.set(p_handle);
		return activate(h, p_aabb, p_delay_collision_check);
	}

	bool deactivate(uint32_t p_handle) {
		BVHHandle h;
		h.set(p_handle);
		return deactivate(h);
	}

	void set_tree(uint32_t p_handle, uint32_t p_tree_id, uint32_t p_tree_collision_mask, bool p_force_collision_check = true) {
		BVHHandle h;
		h.set(p_handle);
		set_tree(h, p_tree_id, p_tree_collision_mask, p_force_collision_check);
	}

	uint32_t get_tree_id(uint32_t p_handle) const {
		BVHHandle h;
		h.set(p_handle);
		return item_get_tree_id(h);
	}
	int get_subindex(uint32_t p_handle) const {
		BVHHandle h;
		h.set(p_handle);
		return item_get_subindex(h);
	}

	T *get(uint32_t p_handle) const {
		BVHHandle h;
		h.set(p_handle);
		return item_get_userdata(h);
	}

	////////////////////////////////////////////////////

	void move(BVHHandle p_handle, const BOUNDS &p_aabb) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		if (tree.item_move(p_handle, p_aabb)) {
			if (USE_PAIRS) {
				_add_changed_item(p_handle, p_aabb);
			}
		}
	}

	void recheck_pairs(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		force_collision_check(p_handle);
	}

	void erase(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// call unpair and remove all references to the item
		// before deleting from the tree
		if (USE_PAIRS) {
			_remove_changed_item(p_handle);
		}

		tree.item_remove(p_handle);

		_check_for_collisions(true);
	}

	// use in conjunction with activate if you have deferred the collision check, and
	// set pairable has never been called.
	// (deferred collision checks are a workaround for visual server for historical reasons)
	void force_collision_check(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		if (USE_PAIRS) {
			// the aabb should already be up to date in the BVH
			BOUNDS aabb;
			item_get_AABB(p_handle, aabb);

			// add it as changed even if aabb not different
			_add_changed_item(p_handle, aabb, false);

			// force an immediate full collision check, much like calls to set_pairable
			_check_for_collisions(true);
		}
	}

	// these should be read as set_visible for render trees,
	// but generically this makes items add or remove from the
	// tree internally, to speed things up by ignoring inactive items
	bool activate(BVHHandle p_handle, const BOUNDS &p_aabb, bool p_delay_collision_check = false) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// sending the aabb here prevents the need for the BVH to maintain
		// a redundant copy of the aabb.
		// returns success
		if (tree.item_activate(p_handle, p_aabb)) {
			if (USE_PAIRS) {
				// in the special case of the render tree, when setting visibility we are using the combination of
				// activate then set_pairable. This would case 2 sets of collision checks. For efficiency here we allow
				// deferring to have a single collision check at the set_pairable call.
				// Watch for bugs! This may cause bugs if set_pairable is not called.
				if (!p_delay_collision_check) {
					_add_changed_item(p_handle, p_aabb, false);

					// force an immediate collision check, much like calls to set_pairable
					_check_for_collisions(true);
				}
			}
			return true;
		}

		return false;
	}

	bool deactivate(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// returns success
		if (tree.item_deactivate(p_handle)) {
			// call unpair and remove all references to the item
			// before deleting from the tree
			if (USE_PAIRS) {
				_remove_changed_item(p_handle);

				// force check for collisions, much like an erase was called
				_check_for_collisions(true);
			}
			return true;
		}

		return false;
	}

	bool get_active(BVHHandle p_handle) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		return tree.item_get_active(p_handle);
	}

	// call e.g. once per frame (this does a trickle optimize)
	void update() {
		BVH_LOCKED_FUNCTION
		tree.update();
		_check_for_collisions();
#ifdef BVH_INTEGRITY_CHECKS
		tree._integrity_check_all();
#endif
	}

	// this can be called more frequently than per frame if necessary
	void update_collisions() {
		BVH_LOCKED_FUNCTION
		_check_for_collisions();
	}

	// prefer calling this directly as type safe
	void set_tree(const BVHHandle &p_handle, uint32_t p_tree_id, uint32_t p_tree_collision_mask, bool p_force_collision_check = true) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVH_LOCKED_FUNCTION
		// Returns true if the pairing state has changed.
		bool state_changed = tree.item_set_tree(p_handle, p_tree_id, p_tree_collision_mask);

		if (USE_PAIRS) {
			// not sure if absolutely necessary to flush collisions here. It will cost performance to, instead
			// of waiting for update, so only uncomment this if there are bugs.
			//_check_for_collisions();

			if ((p_force_collision_check || state_changed) && tree.item_get_active(p_handle)) {
				// when the pairable state changes, we need to force a collision check because newly pairable
				// items may be in collision, and unpairable items might move out of collision.
				// We cannot depend on waiting for the next update, because that may come much later.
				BOUNDS aabb;
				item_get_AABB(p_handle, aabb);

				// passing false disables the optimization which prevents collision checks if
				// the aabb hasn't changed
				_add_changed_item(p_handle, aabb, false);

				// force an immediate collision check (probably just for this one item)
				// but it must be a FULL collision check, also checking pairable state and masks.
				// This is because AABB intersecting objects may have changed pairable state / mask
				// such that they should no longer be paired. E.g. lights.
				_check_for_collisions(true);
			} // only if active
		}
	}

	// cull tests
	int cull_aabb(const BOUNDS &p_aabb, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF, int *p_subindex_array = nullptr) {
		BVH_LOCKED_FUNCTION
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.tree_collision_mask = p_tree_collision_mask;
		params.abb.from(p_aabb);
		params.tester = p_tester;

		tree.cull_aabb(params);

		return params.result_count_overall;
	}

	int cull_segment(const POINT &p_from, const POINT &p_to, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF, int *p_subindex_array = nullptr) {
		BVH_LOCKED_FUNCTION
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.tester = p_tester;
		params.tree_collision_mask = p_tree_collision_mask;

		params.segment.from = p_from;
		params.segment.to = p_to;

		tree.cull_segment(params);

		return params.result_count_overall;
	}

	int cull_point(const POINT &p_point, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF, int *p_subindex_array = nullptr) {
		BVH_LOCKED_FUNCTION
		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = p_subindex_array;
		params.tester = p_tester;
		params.tree_collision_mask = p_tree_collision_mask;

		params.point = p_point;

		tree.cull_point(params);
		return params.result_count_overall;
	}

	int cull_convex(const Vector<Plane> &p_convex, T **p_result_array, int p_result_max, const T *p_tester, uint32_t p_tree_collision_mask = 0xFFFFFFFF) {
		BVH_LOCKED_FUNCTION
		if (!p_convex.size()) {
			return 0;
		}

		Vector<Vector3> convex_points = Geometry3D::compute_convex_mesh_points(&p_convex[0], p_convex.size());
		if (convex_points.is_empty()) {
			return 0;
		}

		typename BVHTREE_CLASS::CullParams params;
		params.result_count_overall = 0;
		params.result_max = p_result_max;
		params.result_array = p_result_array;
		params.subindex_array = nullptr;
		params.tester = p_tester;
		params.tree_collision_mask = p_tree_collision_mask;

		params.hull.planes = &p_convex[0];
		params.hull.num_planes = p_convex.size();
		params.hull.points = &convex_points[0];
		params.hull.num_points = convex_points.size();

		tree.cull_convex(params);

		return params.result_count_overall;
	}

private:
	// do this after moving etc.
	void _check_for_collisions(bool p_full_check = false) {
		if (!changed_items.size()) {
			// noop
			return;
		}

		typename BVHTREE_CLASS::CullParams params;

		params.result_count_overall = 0;
		params.result_max = INT_MAX;
		params.result_array = nullptr;
		params.subindex_array = nullptr;

		for (const BVHHandle &h : changed_items) {
			// use the expanded aabb for pairing
			const BOUNDS &expanded_aabb = tree._pairs[h.id()].expanded_aabb;
			BVHABB_CLASS abb;
			abb.from(expanded_aabb);

			tree.item_fill_cullparams(h, params);

			// find all the existing paired aabbs that are no longer
			// paired, and send callbacks
			_find_leavers(h, abb, p_full_check);

			uint32_t changed_item_ref_id = h.id();

			params.abb = abb;

			params.result_count_overall = 0; // might not be needed
			tree.cull_aabb(params, false);

			for (const uint32_t ref_id : tree._cull_hits) {
				// don't collide against ourself
				if (ref_id == changed_item_ref_id) {
					continue;
				}

				// checkmasks is already done in the cull routine.
				BVHHandle h_collidee;
				h_collidee.set_id(ref_id);

				// find NEW enterers, and send callbacks for them only
				_collide(h, h_collidee);
			}
		}
		_reset();
	}

public:
	void item_get_AABB(BVHHandle p_handle, BOUNDS &r_aabb) {
		DEV_ASSERT(!p_handle.is_invalid());
		BVHABB_CLASS abb;
		tree.item_get_ABB(p_handle, abb);
		abb.to(r_aabb);
	}

private:
	// supplemental funcs
	uint32_t item_get_tree_id(BVHHandle p_handle) const { return _get_extra(p_handle).tree_id; }
	T *item_get_userdata(BVHHandle p_handle) const { return _get_extra(p_handle).userdata; }
	int item_get_subindex(BVHHandle p_handle) const { return _get_extra(p_handle).subindex; }

	void _unpair(BVHHandle p_from, BVHHandle p_to) {
		tree._handle_sort(p_from, p_to);

		typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[p_from.id()];
		typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[p_to.id()];

		// if the userdata is the same, no collisions should occur
		if ((exa.userdata == exb.userdata) && exa.userdata) {
			return;
		}

		typename BVHTREE_CLASS::ItemPairs &pairs_from = tree._pairs[p_from.id()];
		typename BVHTREE_CLASS::ItemPairs &pairs_to = tree._pairs[p_to.id()];

		void *ud_from = pairs_from.remove_pair_to(p_to);
		pairs_to.remove_pair_to(p_from);

#ifdef BVH_VERBOSE_PAIRING
		print_line("_unpair " + itos(p_from.id()) + " from " + itos(p_to.id()));
#endif

		// callback
		if (unpair_callback) {
			unpair_callback(pair_callback_userdata, p_from, exa.userdata, exa.subindex, p_to, exb.userdata, exb.subindex, ud_from);
		}
	}

	void *_recheck_pair(BVHHandle p_from, BVHHandle p_to, void *p_pair_data) {
		tree._handle_sort(p_from, p_to);

		typename BVHTREE_CLASS::ItemExtra &exa = tree._extra[p_from.id()];
		typename BVHTREE_CLASS::ItemExtra &exb = tree._extra[p_to.id()];

		// if the userdata is the same, no collisions should occur
		if ((exa.userdata == exb.userdata) && exa.userdata) {
			return p_pair_data;
		}

		// callback
		if (check_pair_callback) {
			return check_pair_callback(check_pair_callback_userdata, p_from, exa.userdata, exa.subindex, p_to, exb.userdata, exb.subindex, p_pair_data);
		}

		return p_pair_data;
	}

	// returns true if unpair
	bool _find_leavers_process_pair(typename BVHTREE_CLASS::ItemPairs &p_pairs_from, const BVHABB_CLASS &p_abb_from, BVHHandle p_from, BVHHandle p_to, bool p_full_check) {
		BVHABB_CLASS abb_to;
		tree.item_get_ABB(p_to, abb_to);

		// do they overlap?
		if (p_abb_from.intersects(abb_to)) {
			// the full check for pairable / non pairable (i.e. tree_id and tree_masks) and mask changes is extra expense
			// this need not be done in most cases (for speed) except in the case where set_tree is called
			// where the masks etc of the objects in question may have changed
			if (!p_full_check) {
				return false;
			}
			const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(p_from);
			const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(p_to);

			// Checking tree_ids and tree_collision_masks
			if (exa.are_item_trees_compatible(exb)) {
				bool pair_allowed = USER_PAIR_TEST_FUNCTION::user_pair_check(exa.userdata, exb.userdata);

				// the masks must still be compatible to pair
				// i.e. if there is a hit between the two and they intersect, then they should stay paired
				if (pair_allowed) {
					return false;
				}
			}
		}

		_unpair(p_from, p_to);
		return true;
	}

	// find all the existing paired aabbs that are no longer
	// paired, and send callbacks
	void _find_leavers(BVHHandle p_handle, const BVHABB_CLASS &p_expanded_abb_from, bool p_full_check) {
		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_handle.id()];

		BVHABB_CLASS abb_from = p_expanded_abb_from;

		// remove from pairing list for every partner
		for (unsigned int n = 0; n < p_from.extended_pairs.size(); n++) {
			BVHHandle h_to = p_from.extended_pairs[n].handle;
			if (_find_leavers_process_pair(p_from, abb_from, p_handle, h_to, p_full_check)) {
				// we need to keep the counter n up to date if we deleted a pair
				// as the number of items in p_from.extended_pairs will have decreased by 1
				// and we don't want to miss an item
				n--;
			}
		}
	}

	// find NEW enterers, and send callbacks for them only
	// handle a and b
	void _collide(BVHHandle p_ha, BVHHandle p_hb) {
		// only have to do this oneway, lower ID then higher ID
		tree._handle_sort(p_ha, p_hb);

		const typename BVHTREE_CLASS::ItemExtra &exa = _get_extra(p_ha);
		const typename BVHTREE_CLASS::ItemExtra &exb = _get_extra(p_hb);

		// user collision callback
		if (!USER_PAIR_TEST_FUNCTION::user_pair_check(exa.userdata, exb.userdata)) {
			return;
		}

		// if the userdata is the same, no collisions should occur
		if ((exa.userdata == exb.userdata) && exa.userdata) {
			return;
		}

		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_ha.id()];
		typename BVHTREE_CLASS::ItemPairs &p_to = tree._pairs[p_hb.id()];

		// does this pair exist already?
		// or only check the one with lower number of pairs for greater speed
		if (p_from.num_pairs <= p_to.num_pairs) {
			if (p_from.contains_pair_to(p_hb)) {
				return;
			}
		} else {
			if (p_to.contains_pair_to(p_ha)) {
				return;
			}
		}

		// callback
		void *callback_userdata = nullptr;

#ifdef BVH_VERBOSE_PAIRING
		print_line("_pair " + itos(p_ha.id()) + " to " + itos(p_hb.id()));
#endif

		if (pair_callback) {
			callback_userdata = pair_callback(pair_callback_userdata, p_ha, exa.userdata, exa.subindex, p_hb, exb.userdata, exb.subindex);
		}

		// new pair! .. only really need to store the userdata on the lower handle, but both have storage so...
		p_from.add_pair_to(p_hb, callback_userdata);
		p_to.add_pair_to(p_ha, callback_userdata);
	}

	// if we remove an item, we need to immediately remove the pairs, to prevent reading the pair after deletion
	void _remove_pairs_containing(BVHHandle p_handle) {
		typename BVHTREE_CLASS::ItemPairs &p_from = tree._pairs[p_handle.id()];

		// remove from pairing list for every partner.
		// can't easily use a for loop here, because removing changes the size of the list
		while (p_from.extended_pairs.size()) {
			BVHHandle h_to = p_from.extended_pairs[0].handle;
			_unpair(p_handle, h_to);
		}
	}

	// Send pair callbacks again for all existing pairs for the given handle.
	void _recheck_pairs(BVHHandle p_handle) {
		typename BVHTREE_CLASS::ItemPairs &from = tree._pairs[p_handle.id()];

		// checking pair for every partner.
		for (unsigned int n = 0; n < from.extended_pairs.size(); n++) {
			typename BVHTREE_CLASS::ItemPairs::Link &pair = from.extended_pairs[n];
			BVHHandle h_to = pair.handle;
			void *new_pair_data = _recheck_pair(p_handle, h_to, pair.userdata);

			if (new_pair_data != pair.userdata) {
				pair.userdata = new_pair_data;

				// Update pair data for the second item.
				typename BVHTREE_CLASS::ItemPairs &to = tree._pairs[h_to.id()];
				for (unsigned int to_index = 0; to_index < to.extended_pairs.size(); to_index++) {
					typename BVHTREE_CLASS::ItemPairs::Link &to_pair = to.extended_pairs[to_index];
					if (to_pair.handle == p_handle) {
						to_pair.userdata = new_pair_data;
						break;
					}
				}
			}
		}
	}

private:
	const typename BVHTREE_CLASS::ItemExtra &_get_extra(BVHHandle p_handle) const {
		return tree._extra[p_handle.id()];
	}
	const typename BVHTREE_CLASS::ItemRef &_get_ref(BVHHandle p_handle) const {
		return tree._refs[p_handle.id()];
	}

	void _reset() {
		changed_items.clear();
		_tick++;
	}

	void _add_changed_item(BVHHandle p_handle, const BOUNDS &p_aabb, bool p_check_aabb = true) {
		// Note that non pairable items can pair with pairable,
		// so all types must be added to the list

#ifdef BVH_EXPAND_LEAF_AABBS
		// if using expanded AABB in the leaf, the redundancy check will already have been made
		BOUNDS &expanded_aabb = tree._pairs[p_handle.id()].expanded_aabb;
		item_get_AABB(p_handle, expanded_aabb);
#else
		// aabb check with expanded aabb. This greatly decreases processing
		// at the cost of slightly less accurate pairing checks
		// Note this pairing AABB is separate from the AABB in the actual tree
		BOUNDS &expanded_aabb = tree._pairs[p_handle.id()].expanded_aabb;

		// passing p_check_aabb false disables the optimization which prevents collision checks if
		// the aabb hasn't changed. This is needed where set_pairable has been called, but the position
		// has not changed.
		if (p_check_aabb && tree.expanded_aabb_encloses_not_shrink(expanded_aabb, p_aabb)) {
			return;
		}

		// ALWAYS update the new expanded aabb, even if already changed once
		// this tick, because it is vital that the AABB is kept up to date
		expanded_aabb = p_aabb;
		expanded_aabb.grow_by(tree._pairing_expansion);
#endif

		// this code is to ensure that changed items only appear once on the updated list
		// collision checking them multiple times is not needed, and repeats the same thing
		uint32_t &last_updated_tick = tree._extra[p_handle.id()].last_updated_tick;

		if (last_updated_tick == _tick) {
			return; // already on changed list
		}

		// mark as on list
		last_updated_tick = _tick;

		// add to the list
		changed_items.push_back(p_handle);
	}

	void _remove_changed_item(BVHHandle p_handle) {
		// Care has to be taken here for items that are deleted. The ref ID
		// could be reused on the same tick for new items. This is probably
		// rare but should be taken into consideration

		// callbacks
		_remove_pairs_containing(p_handle);

		// remove from changed items (not very efficient yet)
		for (int n = 0; n < (int)changed_items.size(); n++) {
			if (changed_items[n] == p_handle) {
				changed_items.remove_at_unordered(n);

				// because we are using an unordered remove,
				// the last changed item will now be at spot 'n',
				// and we need to redo it, so we prevent moving on to
				// the next n at the next for iteration.
				n--;
			}
		}

		// reset the last updated tick (may not be necessary but just in case)
		tree._extra[p_handle.id()].last_updated_tick = 0;
	}

	PairCallback pair_callback = nullptr;
	UnpairCallback unpair_callback = nullptr;
	CheckPairCallback check_pair_callback = nullptr;
	void *pair_callback_userdata = nullptr;
	void *unpair_callback_userdata = nullptr;
	void *check_pair_callback_userdata = nullptr;

	BVHTREE_CLASS tree;

	// for collision pairing,
	// maintain a list of all items moved etc on each frame / tick
	LocalVector<BVHHandle> changed_items;
	uint32_t _tick = 1; // Start from 1 so items with 0 indicate never updated.

	class BVHLockedFunction {
	public:
		BVHLockedFunction(Mutex *p_mutex, bool p_thread_safe) {
			// will be compiled out if not set in template
			if (p_thread_safe) {
				_mutex = p_mutex;
				_mutex->lock();

			} else {
				_mutex = nullptr;
			}
		}
		~BVHLockedFunction() {
			// will be compiled out if not set in template
			if (_mutex) {
				_mutex->unlock();
			}
		}

	private:
		Mutex *_mutex = nullptr;
	};

	Mutex _mutex;

	// local toggle for turning on and off thread safety in project settings
	bool _thread_safe = BVH_THREAD_SAFE;
};

#undef BVHTREE_CLASS
