/**************************************************************************/
/*  renderer_scene_cull.h                                                 */
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
// --- Folded from core/math/dynamic_bvh.h ---
#include "core/math/aabb.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "core/typedefs.h"

// Based on bullet Dbvh

/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2013 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

///DynamicBVH implementation by Nathanael Presson
// The DynamicBVH class implements a fast dynamic bounding volume tree based on axis aligned bounding boxes (aabb tree).

class DynamicBVH {
	struct Node;

public:
	struct ID {
		Node *node = nullptr;

	public:
		_FORCE_INLINE_ bool is_valid() const { return node != nullptr; }
	};

private:
	struct Volume {
		Vector3 min, max;

		_FORCE_INLINE_ Vector3 get_center() const { return ((min + max) / 2); }
		_FORCE_INLINE_ Vector3 get_length() const { return (max - min); }

		_FORCE_INLINE_ bool contains(const Volume &p_a) const {
			return ((min.x <= p_a.min.x) &&
					(min.y <= p_a.min.y) &&
					(min.z <= p_a.min.z) &&
					(max.x >= p_a.max.x) &&
					(max.y >= p_a.max.y) &&
					(max.z >= p_a.max.z));
		}

		_FORCE_INLINE_ Volume merge(const Volume &p_b) const {
			Volume r;
			for (int i = 0; i < 3; ++i) {
				if (min[i] < p_b.min[i]) {
					r.min[i] = min[i];
				} else {
					r.min[i] = p_b.min[i];
				}
				if (max[i] > p_b.max[i]) {
					r.max[i] = max[i];
				} else {
					r.max[i] = p_b.max[i];
				}
			}
			return r;
		}

		_FORCE_INLINE_ real_t get_size() const {
			const Vector3 edges = get_length();
			return (edges.x * edges.y * edges.z +
					edges.x + edges.y + edges.z);
		}

		_FORCE_INLINE_ bool is_not_equal_to(const Volume &p_b) const {
			return ((min.x != p_b.min.x) ||
					(min.y != p_b.min.y) ||
					(min.z != p_b.min.z) ||
					(max.x != p_b.max.x) ||
					(max.y != p_b.max.y) ||
					(max.z != p_b.max.z));
		}

		_FORCE_INLINE_ real_t get_proximity_to(const Volume &p_b) const {
			const Vector3 d = (min + max) - (p_b.min + p_b.max);
			return (Math::abs(d.x) + Math::abs(d.y) + Math::abs(d.z));
		}

		_FORCE_INLINE_ int select_by_proximity(const Volume &p_a, const Volume &p_b) const {
			return (get_proximity_to(p_a) < get_proximity_to(p_b) ? 0 : 1);
		}

		//
		_FORCE_INLINE_ bool intersects(const Volume &p_b) const {
			return ((min.x <= p_b.max.x) &&
					(max.x >= p_b.min.x) &&
					(min.y <= p_b.max.y) &&
					(max.y >= p_b.min.y) &&
					(min.z <= p_b.max.z) &&
					(max.z >= p_b.min.z));
		}

		_FORCE_INLINE_ bool intersects_convex(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count) const {
			Vector3 half_extents = (max - min) * 0.5;
			Vector3 ofs = min + half_extents;

			for (int i = 0; i < p_plane_count; i++) {
				const Plane &p = p_planes[i];
				Vector3 point(
						(p.normal.x > 0) ? -half_extents.x : half_extents.x,
						(p.normal.y > 0) ? -half_extents.y : half_extents.y,
						(p.normal.z > 0) ? -half_extents.z : half_extents.z);
				point += ofs;
				if (p.is_point_over(point)) {
					return false;
				}
			}

			// Make sure all points in the shape aren't fully separated from the AABB on
			// each axis.
			int bad_point_counts_positive[3] = { 0 };
			int bad_point_counts_negative[3] = { 0 };

			for (int k = 0; k < 3; k++) {
				for (int i = 0; i < p_point_count; i++) {
					if (p_points[i][k] > ofs[k] + half_extents[k]) {
						bad_point_counts_positive[k]++;
					}
					if (p_points[i][k] < ofs[k] - half_extents[k]) {
						bad_point_counts_negative[k]++;
					}
				}

				if (bad_point_counts_negative[k] == p_point_count) {
					return false;
				}
				if (bad_point_counts_positive[k] == p_point_count) {
					return false;
				}
			}

			return true;
		}
	};

	struct Node {
		Volume volume;
		Node *parent = nullptr;
		union {
			Node *children[2];
			void *data;
		};

		_FORCE_INLINE_ bool is_leaf() const { return children[1] == nullptr; }
		_FORCE_INLINE_ bool is_internal() const { return (!is_leaf()); }

		_FORCE_INLINE_ int get_index_in_parent() const {
			ERR_FAIL_NULL_V(parent, 0);
			return (parent->children[1] == this) ? 1 : 0;
		}
		void get_max_depth(int p_depth, int &r_maxdepth) {
			if (is_internal()) {
				children[0]->get_max_depth(p_depth + 1, r_maxdepth);
				children[1]->get_max_depth(p_depth + 1, r_maxdepth);
			} else {
				r_maxdepth = MAX(r_maxdepth, p_depth);
			}
		}

		//
		int count_leaves() const {
			if (is_internal()) {
				return children[0]->count_leaves() + children[1]->count_leaves();
			} else {
				return (1);
			}
		}

		bool is_left_of_axis(const Vector3 &p_org, const Vector3 &p_axis) const {
			return p_axis.dot(volume.get_center() - p_org) <= 0;
		}

		Node() {
			children[0] = nullptr;
			children[1] = nullptr;
		}
	};

	PagedAllocator<Node> node_allocator;
	// Fields
	Node *bvh_root = nullptr;
	int lkhd = -1;
	int total_leaves = 0;
	uint32_t opath = 0;
	uint32_t index = 0;

	enum {
		ALLOCA_STACK_SIZE = 128
	};

	_FORCE_INLINE_ void _delete_node(Node *p_node);
	void _recurse_delete_node(Node *p_node);
	_FORCE_INLINE_ Node *_create_node(Node *p_parent, void *p_data);
	_FORCE_INLINE_ DynamicBVH::Node *_create_node_with_volume(Node *p_parent, const Volume &p_volume, void *p_data);
	_FORCE_INLINE_ void _insert_leaf(Node *p_root, Node *p_leaf);
	_FORCE_INLINE_ Node *_remove_leaf(Node *r_leaf);
	void _fetch_leaves(Node *p_root, LocalVector<Node *> &r_leaves, int p_depth = -1);
	static int _split(Node **r_leaves, int p_count, const Vector3 &p_org, const Vector3 &p_axis);
	static Volume _bounds(Node **r_leaves, int p_count);
	void _bottom_up(Node **r_leaves, int p_count);
	Node *_top_down(Node **r_leaves, int p_count, int p_bu_threshold);
	Node *_node_sort(Node *r_node, Node *&r_root);

	_FORCE_INLINE_ void _update(Node *r_leaf, int p_lookahead = -1);

	void _extract_leaves(Node *p_node, List<ID> *r_elements);

	_FORCE_INLINE_ bool _ray_aabb(const Vector3 &p_ray_from, const Vector3 &p_ray_inv_direction, const unsigned int p_ray_sign[3], const Vector3 p_bounds[2], real_t &r_tmin, real_t p_lambda_min, real_t p_lambda_max) {
		real_t tmax, tymin, tymax, tzmin, tzmax;
		r_tmin = (p_bounds[p_ray_sign[0]].x - p_ray_from.x) * p_ray_inv_direction.x;
		tmax = (p_bounds[1 - p_ray_sign[0]].x - p_ray_from.x) * p_ray_inv_direction.x;
		tymin = (p_bounds[p_ray_sign[1]].y - p_ray_from.y) * p_ray_inv_direction.y;
		tymax = (p_bounds[1 - p_ray_sign[1]].y - p_ray_from.y) * p_ray_inv_direction.y;

		if ((r_tmin > tymax) || (tymin > tmax)) {
			return false;
		}

		if (tymin > r_tmin) {
			r_tmin = tymin;
		}

		if (tymax < tmax) {
			tmax = tymax;
		}

		tzmin = (p_bounds[p_ray_sign[2]].z - p_ray_from.z) * p_ray_inv_direction.z;
		tzmax = (p_bounds[1 - p_ray_sign[2]].z - p_ray_from.z) * p_ray_inv_direction.z;

		if ((r_tmin > tzmax) || (tzmin > tmax)) {
			return false;
		}
		if (tzmin > r_tmin) {
			r_tmin = tzmin;
		}
		if (tzmax < tmax) {
			tmax = tzmax;
		}
		return ((r_tmin < p_lambda_max) && (tmax > p_lambda_min));
	}

public:
	// Methods
	void clear();
	bool is_empty() const { return (nullptr == bvh_root); }
	void optimize_bottom_up();
	void optimize_top_down(int p_bu_threshold = 128);
	void optimize_incremental(int p_passes);
	ID insert(const AABB &p_box, void *p_userdata);
	bool update(const ID &p_id, const AABB &p_box);
	void remove(const ID &p_id);
	void get_elements(List<ID> *r_elements);

	int get_leaf_count() const;
	int get_max_depth() const;

	/* Discouraged, but works as a reference on how it must be used */
	struct DefaultQueryResult {
		virtual bool operator()(void *p_data) = 0; //return true whether you want to continue the query
		virtual ~DefaultQueryResult() {}
	};

	template <typename QueryResult>
	_FORCE_INLINE_ void aabb_query(const AABB &p_aabb, QueryResult &r_result);
	template <typename QueryResult>
	_FORCE_INLINE_ void convex_query(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count, QueryResult &r_result);
	template <typename QueryResult>
	_FORCE_INLINE_ void ray_query(const Vector3 &p_from, const Vector3 &p_to, QueryResult &r_result);

	void set_index(uint32_t p_index);
	uint32_t get_index() const;

	~DynamicBVH();
};

template <typename QueryResult>
void DynamicBVH::aabb_query(const AABB &p_box, QueryResult &r_result) {
	if (!bvh_root) {
		return;
	}

	Volume volume;
	volume.min = p_box.position;
	volume.max = p_box.position + p_box.size;

	const Node **alloca_stack = (const Node **)alloca(ALLOCA_STACK_SIZE * sizeof(const Node *));
	const Node **stack = alloca_stack;
	stack[0] = bvh_root;
	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;

	LocalVector<const Node *> aux_stack; //only used in rare occasions when you run out of alloca memory because tree is too unbalanced. Should correct itself over time.

	do {
		depth--;
		const Node *n = stack[depth];
		if (n->volume.intersects(volume)) {
			if (n->is_internal()) {
				if (depth > threshold) {
					if (aux_stack.is_empty()) {
						aux_stack.resize(ALLOCA_STACK_SIZE * 2);
						memcpy(aux_stack.ptr(), alloca_stack, ALLOCA_STACK_SIZE * sizeof(const Node *));
						alloca_stack = nullptr;
					} else {
						aux_stack.resize(aux_stack.size() * 2);
					}
					stack = aux_stack.ptr();
					threshold = aux_stack.size() - 2;
				}
				stack[depth++] = n->children[0];
				stack[depth++] = n->children[1];
			} else {
				if (r_result(n->data)) {
					return;
				}
			}
		}
	} while (depth > 0);
}

template <typename QueryResult>
void DynamicBVH::convex_query(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count, QueryResult &r_result) {
	if (!bvh_root) {
		return;
	}

	//generate a volume anyway to improve pre-testing
	Volume volume;
	for (int i = 0; i < p_point_count; i++) {
		if (i == 0) {
			volume.min = p_points[0];
			volume.max = p_points[0];
		} else {
			volume.min = volume.min.min(p_points[i]);
			volume.max = volume.max.max(p_points[i]);
		}
	}

	const Node **alloca_stack = (const Node **)alloca(ALLOCA_STACK_SIZE * sizeof(const Node *));
	const Node **stack = alloca_stack;
	stack[0] = bvh_root;
	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;

	LocalVector<const Node *> aux_stack; //only used in rare occasions when you run out of alloca memory because tree is too unbalanced. Should correct itself over time.

	do {
		depth--;
		const Node *n = stack[depth];
		if (n->volume.intersects(volume) && n->volume.intersects_convex(p_planes, p_plane_count, p_points, p_point_count)) {
			if (n->is_internal()) {
				if (depth > threshold) {
					if (aux_stack.is_empty()) {
						aux_stack.resize(ALLOCA_STACK_SIZE * 2);
						memcpy(aux_stack.ptr(), alloca_stack, ALLOCA_STACK_SIZE * sizeof(const Node *));
						alloca_stack = nullptr;
					} else {
						aux_stack.resize(aux_stack.size() * 2);
					}
					stack = aux_stack.ptr();
					threshold = aux_stack.size() - 2;
				}
				stack[depth++] = n->children[0];
				stack[depth++] = n->children[1];
			} else {
				if (r_result(n->data)) {
					return;
				}
			}
		}
	} while (depth > 0);
}
template <typename QueryResult>
void DynamicBVH::ray_query(const Vector3 &p_from, const Vector3 &p_to, QueryResult &r_result) {
	if (!bvh_root) {
		return;
	}

	Vector3 ray_dir = (p_to - p_from);
	ray_dir.normalize();

	///what about division by zero? --> just set rayDirection[i] to INF/B3_LARGE_FLOAT
	Vector3 inv_dir;
	inv_dir[0] = ray_dir[0] == real_t(0.0) ? real_t(1e20) : real_t(1.0) / ray_dir[0];
	inv_dir[1] = ray_dir[1] == real_t(0.0) ? real_t(1e20) : real_t(1.0) / ray_dir[1];
	inv_dir[2] = ray_dir[2] == real_t(0.0) ? real_t(1e20) : real_t(1.0) / ray_dir[2];
	unsigned int signs[3] = { inv_dir[0] < 0.0, inv_dir[1] < 0.0, inv_dir[2] < 0.0 };

	real_t lambda_max = ray_dir.dot(p_to - p_from);

	Vector3 bounds[2];

	const Node **alloca_stack = (const Node **)alloca(ALLOCA_STACK_SIZE * sizeof(const Node *));
	const Node **stack = alloca_stack;
	stack[0] = bvh_root;
	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;

	LocalVector<const Node *> aux_stack; //only used in rare occasions when you run out of alloca memory because tree is too unbalanced. Should correct itself over time.

	do {
		depth--;
		const Node *node = stack[depth];
		bounds[0] = node->volume.min;
		bounds[1] = node->volume.max;
		real_t tmin = 1.f, lambda_min = 0.f;
		unsigned int result1 = false;
		result1 = _ray_aabb(p_from, inv_dir, signs, bounds, tmin, lambda_min, lambda_max);
		if (result1) {
			if (node->is_internal()) {
				if (depth > threshold) {
					if (aux_stack.is_empty()) {
						aux_stack.resize(ALLOCA_STACK_SIZE * 2);
						memcpy(aux_stack.ptr(), alloca_stack, ALLOCA_STACK_SIZE * sizeof(const Node *));
						alloca_stack = nullptr;
					} else {
						aux_stack.resize(aux_stack.size() * 2);
					}
					stack = aux_stack.ptr();
					threshold = aux_stack.size() - 2;
				}
				stack[depth++] = node->children[0];
				stack[depth++] = node->children[1];
			} else {
				if (r_result(node->data)) {
					return;
				}
			}
		}
	} while (depth > 0);
}

// --- End core/math/dynamic_bvh.h ---
#include "core/math/transform_interpolator.h"
// --- Folded from core/templates/bin_sorted_array.h ---
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"

template <typename T>
class _WARN_UNUSED_ BinSortedArray {
	PagedArray<T> array;
	LocalVector<uint64_t> bin_limits;

	// Implement if elements need to keep track of their own index in the array.
	_FORCE_INLINE_ virtual void _update_idx(T &r_element, uint64_t p_idx) {}

	_FORCE_INLINE_ void _swap(uint64_t p_a, uint64_t p_b) {
		SWAP(array[p_a], array[p_b]);
		_update_idx(array[p_a], p_a);
		_update_idx(array[p_b], p_b);
	}

public:
	uint64_t insert(T &p_element, uint64_t p_bin) {
		array.push_back(p_element);
		uint64_t new_idx = array.size() - 1;
		_update_idx(p_element, new_idx);
		bin_limits[0] = new_idx;
		if (p_bin != 0) {
			new_idx = move(new_idx, p_bin);
		}
		return new_idx;
	}

	uint64_t move(uint64_t p_idx, uint64_t p_bin) {
		ERR_FAIL_UNSIGNED_INDEX_V(p_idx, array.size(), -1);

		uint64_t current_bin = bin_limits.size() - 1;
		while (p_idx > bin_limits[current_bin]) {
			current_bin--;
		}

		if (p_bin == current_bin) {
			return p_idx;
		}

		uint64_t current_idx = p_idx;
		if (p_bin > current_bin) {
			while (p_bin > current_bin) {
				uint64_t swap_idx = 0;

				if (current_bin == bin_limits.size() - 1) {
					bin_limits.push_back(0);
				} else {
					bin_limits[current_bin + 1]++;
					swap_idx = bin_limits[current_bin + 1];
				}

				if (current_idx != swap_idx) {
					_swap(current_idx, swap_idx);
					current_idx = swap_idx;
				}

				current_bin++;
			}
		} else {
			while (p_bin < current_bin) {
				uint64_t swap_idx = bin_limits[current_bin];

				if (current_idx != swap_idx) {
					_swap(current_idx, swap_idx);
				}

				if (current_bin == bin_limits.size() - 1 && bin_limits[current_bin] == 0) {
					bin_limits.resize(bin_limits.size() - 1);
				} else {
					bin_limits[current_bin]--;
				}
				current_idx = swap_idx;
				current_bin--;
			}
		}

		return current_idx;
	}

	void remove_at(uint64_t p_idx) {
		ERR_FAIL_UNSIGNED_INDEX(p_idx, array.size());
		uint64_t new_idx = move(p_idx, 0);
		uint64_t swap_idx = array.size() - 1;

		if (new_idx != swap_idx) {
			_swap(new_idx, swap_idx);
		}

		if (bin_limits[0] > 0) {
			bin_limits[0]--;
		}

		array.pop_back();
	}

	void set_page_pool(PagedArrayPool<T> *p_page_pool) {
		array.set_page_pool(p_page_pool);
	}

	_FORCE_INLINE_ const T &operator[](uint64_t p_index) const {
		return array[p_index];
	}

	_FORCE_INLINE_ T &operator[](uint64_t p_index) {
		return array[p_index];
	}

	int get_bin_count() {
		if (array.size() == 0) {
			return 0;
		}
		return bin_limits.size();
	}

	int get_bin_start(int p_bin) {
		ERR_FAIL_COND_V(p_bin >= get_bin_count(), ~0U);
		if ((unsigned int)p_bin == bin_limits.size() - 1) {
			return 0;
		}
		return bin_limits[p_bin + 1] + 1;
	}

	int get_bin_size(int p_bin) {
		ERR_FAIL_COND_V(p_bin >= get_bin_count(), 0);
		if ((unsigned int)p_bin == bin_limits.size() - 1) {
			return bin_limits[p_bin] + 1;
		}
		return bin_limits[p_bin] - bin_limits[p_bin + 1];
	}

	void reset() {
		array.reset();
		bin_limits.clear();
		bin_limits.push_back(0);
	}

	BinSortedArray() {
		bin_limits.push_back(0);
	}

	virtual ~BinSortedArray() {
		reset();
	}
};

// --- End core/templates/bin_sorted_array.h ---
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "core/math/plane.h"
#include "core/math/vector3.h"
#include "servers/rendering/storage/material_storage.h"
#include "servers/rendering/renderer.h"
// --- Folded from servers/rendering/renderer_scene_occlusion_cull.h ---
#include "core/io/image.h"
#include "core/math/projection.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/rendering_server_enums.h"

#include <cfloat> // FLT_MIN, FLT_MAX

class RendererSceneOcclusionCull final
{
public:
	class HZBuffer
	{
	public:
		static const int TILE_SIZE = 4;
		static const int TILE_RAYS = TILE_SIZE * TILE_SIZE;
		struct CameraRayThreadData;

	protected:
		LocalVector<float> data;
		LocalVector<Size2i> sizes;
		LocalVector<float *> mips;

		RID debug_texture;
		Ref<Image> debug_image;
		PackedByteArray debug_data;
		float debug_tex_range = 0.0f;

		uint64_t occlusion_frame = 0;
		Size2i occlusion_buffer_size;

		Size2i tile_grid_size;

		void _clear_mips();
		void _resize_mips(const Size2i &p_size);
		void _camera_rays_threaded(uint32_t p_thread, const CameraRayThreadData *p_data);
		void _generate_camera_rays(const CameraRayThreadData *p_data, int p_from, int p_to);

		_FORCE_INLINE_ bool _is_occluded(const real_t p_bounds[6], const Vector3 &p_cam_position, const Transform3D &p_cam_inv_transform, const Projection &p_cam_projection, real_t p_near, bool p_is_orthogonal) const {
			if (is_empty()) {
				return false;
			}

			Vector3 closest_point = p_cam_position.clamp(Vector3(p_bounds[0], p_bounds[1], p_bounds[2]), Vector3(p_bounds[3], p_bounds[4], p_bounds[5]));

			if (closest_point == p_cam_position) {
				return false;
			}

			Vector3 closest_point_view = p_cam_inv_transform.xform(closest_point);
			if (closest_point_view.z > -p_near) {
				return false;
			}

			// Force distance calculation to use double precision to avoid floating-point overflow for distant objects.
			closest_point = closest_point - p_cam_position;
			float min_depth = Math::sqrt((double)closest_point.x * (double)closest_point.x + (double)closest_point.y * (double)closest_point.y + (double)closest_point.z * (double)closest_point.z);

			Vector2 rect_min = Vector2(FLT_MAX, FLT_MAX);
			Vector2 rect_max = Vector2(FLT_MIN, FLT_MIN);

			for (int j = 0; j < 8; j++) {
				// Bitmask to cycle through the corners of the AABB.
				Vector3 corner = Vector3(
						j & 4 ? p_bounds[0] : p_bounds[3],
						j & 2 ? p_bounds[1] : p_bounds[4],
						j & 1 ? p_bounds[2] : p_bounds[5]);
				Vector3 view = p_cam_inv_transform.xform(corner);

				// When using an orthogonal camera, the closest point of an AABB to the camera is guaranteed to be a corner.
				if (p_is_orthogonal) {
					min_depth = MIN(min_depth, -view.z);
				}

				Vector3 projected = p_cam_projection.xform(view);

				if (-view.z < 0.0) {
					rect_min = Vector2(0.0f, 0.0f);
					rect_max = Vector2(1.0f, 1.0f);
					break;
				}

				Vector2 normalized = Vector2(projected.x * 0.5f + 0.5f, projected.y * 0.5f + 0.5f);
				rect_min = rect_min.min(normalized);
				rect_max = rect_max.max(normalized);
			}

			rect_max = rect_max.minf(1);
			rect_min = rect_min.maxf(0);

			int mip_count = mips.size();

			Vector2 screen_diagonal = (rect_max - rect_min) * sizes[0];
			float size = MAX(screen_diagonal.x, screen_diagonal.y);
			float l = Math::ceil(Math::log2(size));
			int lod = CLAMP(l, 0, mip_count - 1);

			const int max_samples = 512;
			int sample_count = 0;
			bool visible = true;

			for (; lod >= 0; lod--) {
				int w = sizes[lod].x;
				int h = sizes[lod].y;

				int minx = CLAMP(rect_min.x * w - 1, 0, w - 1);
				int maxx = CLAMP(rect_max.x * w + 1, 0, w - 1);

				int miny = CLAMP(rect_min.y * h - 1, 0, h - 1);
				int maxy = CLAMP(rect_max.y * h + 1, 0, h - 1);

				sample_count += (maxx - minx + 1) * (maxy - miny + 1);

				if (sample_count > max_samples) {
					return false;
				}

				visible = false;
				for (int y = miny; y <= maxy; y++) {
					for (int x = minx; x <= maxx; x++) {
						float depth = mips[lod][y * w + x];
						if (depth > min_depth) {
							visible = true;
							break;
						}
					}
					if (visible) {
						break;
					}
				}

				if (!visible) {
					return true;
				}
			}

			return !visible;
		}

	public:
		static bool occlusion_jitter_enabled;

		unsigned int camera_rays_tile_count = 0;
		uint8_t *camera_rays_unaligned_buffer = nullptr;
		void *camera_rays = nullptr;
		LocalVector<uint32_t> camera_ray_masks;
		RID scenario_rid;

		_FORCE_INLINE_ bool is_empty() const {
			return sizes.is_empty();
		}

		void clear();
		void resize(const Size2i &p_size);
		void sort_rays(const Vector3 &p_camera_dir, bool p_orthogonal);
		void update_camera_rays(const Transform3D &p_cam_transform, const Vector3 &p_near_bottom_left, const Vector2 &p_near_extents, real_t p_z_far, bool p_cam_orthogonal);

		void update_mips();

		// Thin wrapper around _is_occluded(),
		// allowing occlusion timers to delay the disappearance
		// of objects to prevent flickering when using jittering.
		_FORCE_INLINE_ bool is_occluded(const real_t p_bounds[6], const Vector3 &p_cam_position, const Transform3D &p_cam_inv_transform, const Projection &p_cam_projection, real_t p_near, bool p_is_orthogonal, uint64_t &r_occlusion_timeout) const {
			bool occluded = _is_occluded(p_bounds, p_cam_position, p_cam_inv_transform, p_cam_projection, p_near, p_is_orthogonal);

			// Special case, temporal jitter disabled,
			// so we don't use occlusion timers.
			if (!occlusion_jitter_enabled) {
				return occluded;
			}

			if (!occluded) {
//#define DEBUG_RASTER_OCCLUSION_JITTER
#ifdef DEBUG_RASTER_OCCLUSION_JITTER
				r_occlusion_timeout = occlusion_frame + 1;
#else
				r_occlusion_timeout = occlusion_frame + 9;
#endif
			} else if (r_occlusion_timeout) {
				// Regular timeout, allow occlusion culling
				// to proceed as normal after the delay.
				if (occlusion_frame >= r_occlusion_timeout) {
					r_occlusion_timeout = 0;
				}
			}

			return occluded && !r_occlusion_timeout;
		}

		RID get_debug_texture();
		const Size2i &get_occlusion_buffer_size() const { return occlusion_buffer_size; }

		~HZBuffer();
	};

private:
	static void _print_warning()
	{
		WARN_PRINT_ONCE("Occlusion culling is disabled at build-time.");
	}

	static bool _default_is_occluder(RID p_rid) { return false; }
	static RID _default_occluder_allocate() { return RID(); }
	static void _default_occluder_initialize(RID p_occluder) {}
	static void _default_free_occluder(RID p_occluder) { _print_warning(); }
	static void _default_occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
	{
		_print_warning();
	}

	static void _default_add_scenario(RID p_scenario) {}
	static void _default_remove_scenario(RID p_scenario) {}
	static void _default_scenario_set_instance(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled)
	{
		_print_warning();
	}
	static void _default_scenario_remove_instance(RID p_scenario, RID p_instance)
	{
		_print_warning();
	}

	static void _default_add_buffer(RID p_buffer) { _print_warning(); }
	static void _default_remove_buffer(RID p_buffer) { _print_warning(); }
	static HZBuffer* _default_buffer_get_ptr(RID p_buffer) { return nullptr; }
	static void _default_buffer_set_scenario(RID p_buffer, RID p_scenario) { _print_warning(); }
	static void _default_buffer_set_size(RID p_buffer, const Vector2i& p_size) { _print_warning(); }
	static void _default_buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal)
	{
	}

	static RID _default_buffer_get_debug_texture(RID p_buffer)
	{
		_print_warning();
		return RID();
	}

	static void _default_set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality) {}

public:
	inline static bool (*is_occluder)(RID p_rid) = &_default_is_occluder;
	inline static RID (*occluder_allocate)() = &_default_occluder_allocate;
	inline static void (*occluder_initialize)(RID p_occluder) = &_default_occluder_initialize;
	inline static void (*free_occluder)(RID p_occluder) = &_default_free_occluder;
	inline static void (*occluder_set_mesh)(RID p_occluder, const PackedVector3Array& p_vertices,
		const PackedInt32Array& p_indices) = &_default_occluder_set_mesh;

	inline static void (*add_scenario)(RID p_scenario) = &_default_add_scenario;
	inline static void (*remove_scenario)(RID p_scenario) = &_default_remove_scenario;
	inline static void (*scenario_set_instance)(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled) = &_default_scenario_set_instance;
	inline static void (*scenario_remove_instance)(
		RID p_scenario, RID p_instance) = &_default_scenario_remove_instance;

	inline static void (*add_buffer)(RID p_buffer) = &_default_add_buffer;
	inline static void (*remove_buffer)(RID p_buffer) = &_default_remove_buffer;
	inline static HZBuffer* (*buffer_get_ptr)(RID p_buffer) = &_default_buffer_get_ptr;
	inline static void (*buffer_set_scenario)(
		RID p_buffer, RID p_scenario) = &_default_buffer_set_scenario;
	inline static void (*buffer_set_size)(
		RID p_buffer, const Vector2i& p_size) = &_default_buffer_set_size;
	inline static void (*buffer_update)(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal) = &_default_buffer_update;

	inline static RID (*buffer_get_debug_texture)(RID p_buffer) = &_default_buffer_get_debug_texture;

	inline static void (*set_build_quality)(
		RSE::ViewportOcclusionCullingBuildQuality p_quality) = &_default_set_build_quality;

	template <typename Impl> static void bind()
	{
		is_occluder = &Impl::is_occluder;
		occluder_allocate = &Impl::occluder_allocate;
		occluder_initialize = &Impl::occluder_initialize;
		free_occluder = &Impl::free_occluder;
		occluder_set_mesh = &Impl::occluder_set_mesh;

		add_scenario = &Impl::add_scenario;
		remove_scenario = &Impl::remove_scenario;
		scenario_set_instance = &Impl::scenario_set_instance;
		scenario_remove_instance = &Impl::scenario_remove_instance;

		add_buffer = &Impl::add_buffer;
		remove_buffer = &Impl::remove_buffer;
		buffer_get_ptr = &Impl::buffer_get_ptr;
		buffer_set_scenario = &Impl::buffer_set_scenario;
		buffer_set_size = &Impl::buffer_set_size;
		buffer_update = &Impl::buffer_update;

		buffer_get_debug_texture = &Impl::buffer_get_debug_texture;

		set_build_quality = &Impl::set_build_quality;
	}

	static void reset()
	{
		is_occluder = &_default_is_occluder;
		occluder_allocate = &_default_occluder_allocate;
		occluder_initialize = &_default_occluder_initialize;
		free_occluder = &_default_free_occluder;
		occluder_set_mesh = &_default_occluder_set_mesh;

		add_scenario = &_default_add_scenario;
		remove_scenario = &_default_remove_scenario;
		scenario_set_instance = &_default_scenario_set_instance;
		scenario_remove_instance = &_default_scenario_remove_instance;

		add_buffer = &_default_add_buffer;
		remove_buffer = &_default_remove_buffer;
		buffer_get_ptr = &_default_buffer_get_ptr;
		buffer_set_scenario = &_default_buffer_set_scenario;
		buffer_set_size = &_default_buffer_set_size;
		buffer_update = &_default_buffer_update;

		buffer_get_debug_texture = &_default_buffer_get_debug_texture;

		set_build_quality = &_default_set_build_quality;
	}

	RendererSceneOcclusionCull() = delete;
	RendererSceneOcclusionCull(const RendererSceneOcclusionCull&) = delete;
	RendererSceneOcclusionCull& operator=(const RendererSceneOcclusionCull&) = delete;
	~RendererSceneOcclusionCull() = delete;
};

// --- End servers/rendering/renderer_scene_occlusion_cull.h ---
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"
#include "servers/xr/xr_interface.h"

class InstanceUniforms final
{
public:
	void free(RID p_self);

	void materials_start();

	inline int32_t location() const { return _location; }

	inline bool is_allocated() const { return _location != -1; }

private:
	struct Item
	{
		int32_t index = -1;
		int32_t flags = 0;
		inline bool is_valid() const { return index != -1; }
	};

	int32_t _location = -1;
	HashMap<String, Item> _parameters;

	void _init_param(
		Item& r_item, const InstanceShaderParam& p_param) const;
	void _invalidate_items();
};

class RenderingLightCuller;

class RendererSceneCull final
{
public:
	enum
	{
		SDFGI_MAX_CASCADES = 8,
		SDFGI_MAX_REGIONS_PER_CASCADE = 3,
		MAX_INSTANCE_PAIRS = 32,
		MAX_UPDATE_SHADOWS = 512
	};

	/* EVENT QUEUING */

	static void tick();
	static void pre_draw(bool p_will_draw);

	/* CAMERA API */

	struct Camera
	{
		enum Type
		{
			PERSPECTIVE,
			ORTHOGONAL,
			FRUSTUM
		};

		Type type;
		float fov;
		float znear, zfar;
		float size;
		Vector2 offset;
		uint32_t visible_layers;
		bool vaspect;
		RID env;
		RID attributes;
		RID compositor;

		Transform3D transform;

		Camera()
		{
			visible_layers = 0xFFFFFFFF;
			fov = 75;
			type = PERSPECTIVE;
			znear = 0.05;
			zfar = 4000;
			size = 1.0;
			offset = Vector2();
			vaspect = false;
		}
	};


	static RID camera_allocate();
	static void camera_initialize(RID p_rid);

	static void camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	static void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	static void camera_set_frustum(
		RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	static void camera_set_transform(RID p_camera, const Transform3D& p_transform);
	static void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	static void camera_set_environment(RID p_camera, RID p_env);
	static void camera_set_camera_attributes(RID p_camera, RID p_attributes);
	static void camera_set_compositor(RID p_camera, RID p_compositor);
	static void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);
	static bool is_camera(RID p_camera);

	/* OCCLUDER API */

	static RID occluder_allocate();
	static void occluder_initialize(RID p_occluder);
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);

	/* SCENARIO API */

	struct Instance;

	struct PlaneSign
	{
		_ALWAYS_INLINE_ PlaneSign() {}

		_ALWAYS_INLINE_ PlaneSign(const Plane& p_plane)
		{
			if (p_plane.normal.x > 0) {
				signs[0] = 0;
			}
			else {
				signs[0] = 3;
			}
			if (p_plane.normal.y > 0) {
				signs[1] = 1;
			}
			else {
				signs[1] = 4;
			}
			if (p_plane.normal.z > 0) {
				signs[2] = 2;
			}
			else {
				signs[2] = 5;
			}
		}

		uint32_t signs[3];
	};

	struct Frustum
	{
		Vector<Plane> planes;
		Vector<PlaneSign> plane_signs;
		const Plane* planes_ptr;
		const PlaneSign* plane_signs_ptr;
		uint32_t plane_count;

		_ALWAYS_INLINE_ Frustum() {}

		_ALWAYS_INLINE_ Frustum(const Frustum& p_frustum)
		{
			planes = p_frustum.planes;
			plane_signs = p_frustum.plane_signs;

			planes_ptr = planes.ptr();
			plane_signs_ptr = plane_signs.ptr();
			plane_count = p_frustum.plane_count;
		}

		_ALWAYS_INLINE_ void operator=(const Frustum& p_frustum)
		{
			planes = p_frustum.planes;
			plane_signs = p_frustum.plane_signs;

			planes_ptr = planes.ptr();
			plane_signs_ptr = plane_signs.ptr();
			plane_count = p_frustum.plane_count;
		}

		_ALWAYS_INLINE_ Frustum(const Vector<Plane>& p_planes)
		{
			planes = p_planes;
			planes_ptr = planes.ptrw();
			plane_count = planes.size();
			for (int i = 0; i < planes.size(); i++) {
				PlaneSign ps(p_planes[i]);
				plane_signs.push_back(ps);
			}

			plane_signs_ptr = plane_signs.ptr();
		}
	};

	struct InstanceBounds
	{
		// Efficiently store instance bounds.
		// Because bounds checking is performed first,
		// keep it separated from data.

		real_t bounds[6];

		_ALWAYS_INLINE_ InstanceBounds() {}

		_ALWAYS_INLINE_ InstanceBounds(const AABB& p_aabb)
		{
			bounds[0] = p_aabb.position.x;
			bounds[1] = p_aabb.position.y;
			bounds[2] = p_aabb.position.z;
			bounds[3] = p_aabb.position.x + p_aabb.size.x;
			bounds[4] = p_aabb.position.y + p_aabb.size.y;
			bounds[5] = p_aabb.position.z + p_aabb.size.z;
		}

		_ALWAYS_INLINE_ bool in_frustum(const Frustum& p_frustum) const
		{
			// This is not a full SAT check and the possibility of false positives exist,
			// but the tradeoff vs performance is still very good.

			for (uint32_t i = 0; i < p_frustum.plane_count; i++) {
				Vector3 min(bounds[p_frustum.plane_signs_ptr[i].signs[0]],
					bounds[p_frustum.plane_signs_ptr[i].signs[1]],
					bounds[p_frustum.plane_signs_ptr[i].signs[2]]);

				if (p_frustum.planes_ptr[i].distance_to(min) >= 0.0) {
					return false;
				}
			}

			return true;
		}

		_ALWAYS_INLINE_ bool in_aabb(const AABB& p_aabb) const
		{
			Vector3 end = p_aabb.position + p_aabb.size;

			if (bounds[0] >= end.x) {
				return false;
			}
			if (bounds[3] <= p_aabb.position.x) {
				return false;
			}
			if (bounds[1] >= end.y) {
				return false;
			}
			if (bounds[4] <= p_aabb.position.y) {
				return false;
			}
			if (bounds[2] >= end.z) {
				return false;
			}
			if (bounds[5] <= p_aabb.position.z) {
				return false;
			}

			return true;
		}
	};

	struct InstanceBaseData;

	struct InstanceData
	{
		// Store instance pointer as well as common instance processing information,
		// to make processing more cache friendly.
		enum Flags : uint32_t
		{
			FLAG_BASE_TYPE_MASK = 0xFF,
			FLAG_CAST_SHADOWS = (1 << 8),
			FLAG_CAST_SHADOWS_ONLY = (1 << 9),
			FLAG_REDRAW_IF_VISIBLE = (1 << 10),
			FLAG_GEOM_LIGHTING_DIRTY = (1 << 11),
			FLAG_GEOM_REFLECTION_DIRTY = (1 << 12),
			FLAG_GEOM_DECAL_DIRTY = (1 << 13),
			FLAG_GEOM_VOXEL_GI_DIRTY = (1 << 14),
			FLAG_LIGHTMAP_CAPTURE = (1 << 15),
			FLAG_USES_BAKED_LIGHT = (1 << 16),
			FLAG_USES_MESH_INSTANCE = (1 << 17),
			FLAG_REFLECTION_PROBE_DIRTY = (1 << 18),
			FLAG_IGNORE_OCCLUSION_CULLING = (1 << 19),
			FLAG_VISIBILITY_DEPENDENCY_NEEDS_CHECK =
				(3 << 20), // 2 bits, overlaps with the other vis. dependency flags
			FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE = (1 << 20),
			FLAG_VISIBILITY_DEPENDENCY_HIDDEN = (1 << 21),
			FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN = (1 << 22),
			FLAG_GEOM_PROJECTOR_SOFTSHADOW_DIRTY = (1 << 23),
			FLAG_IGNORE_ALL_CULLING = (1 << 24),
		};

		uint32_t flags = 0;
		uint32_t layer_mask = 0; // for fast layer-mask discard
		RID base_rid;

		union
		{
			uint64_t instance_data_rid;
			RenderGeometryInstance* instance_geometry;
			InstanceBaseData* visibility_notifier = nullptr;
		};

		Instance* instance = nullptr;
		int32_t parent_array_index = -1;
		int32_t visibility_index = -1;

		// Each time occlusion culling determines an instance is visible,
		// set this to occlusion_frame plus some delay.
		// Once the timeout is reached, allow the instance to be occlusion culled.
		// This creates a delay for occlusion culling, which prevents flickering
		// when jittering the raster occlusion projection.
		uint64_t occlusion_timeout = 0;
	};

	struct InstanceVisibilityData
	{
		uint64_t viewport_state = 0;
		int32_t array_index = -1;
		RSE::VisibilityRangeFadeMode fade_mode = RSE::VISIBILITY_RANGE_FADE_DISABLED;
		Vector3 position;
		Instance* instance = nullptr;
		float range_begin = 0.0f;
		float range_end = 0.0f;
		float range_begin_margin = 0.0f;
		float range_end_margin = 0.0f;
		float children_fade_alpha = 1.0f;
	};

	class VisibilityArray : public BinSortedArray<InstanceVisibilityData>
	{
		_FORCE_INLINE_ void _update_idx(InstanceVisibilityData& r_element, uint64_t p_idx)
		{
			r_element.instance->visibility_index = p_idx;
			if (r_element.instance->scenario && r_element.instance->array_index != -1) {
				r_element.instance->scenario->instance_data[r_element.instance->array_index]
					.visibility_index = p_idx;
			}
		}
	};


	struct Scenario
	{
		enum IndexerType
		{
			INDEXER_GEOMETRY, // for geometry
			INDEXER_VOLUMES,  // for everything else
			INDEXER_MAX
		};

		DynamicBVH indexers[INDEXER_MAX];

		RID self;

		List<Instance*> directional_lights;
		RID environment;
		RID fallback_environment;
		RID camera_attributes;
		RID compositor;
		RID reflection_probe_shadow_atlas;
		RID reflection_atlas;
		uint64_t used_viewport_visibility_bits;
		HashMap<RID, uint64_t> viewport_visibility_masks;

		SelfList<Instance>::List instances;

		LocalVector<RID> dynamic_lights;

		PagedArray<InstanceBounds> instance_aabbs;
		PagedArray<InstanceData> instance_data;
		VisibilityArray instance_visibility;

		Scenario()
		{
			indexers[INDEXER_GEOMETRY].set_index(INDEXER_GEOMETRY);
			indexers[INDEXER_VOLUMES].set_index(INDEXER_VOLUMES);
			used_viewport_visibility_bits = 0;
		}
	};


	static void _instance_pair(Instance* p_A, Instance* p_B);
	static void _instance_unpair(Instance* p_A, Instance* p_B);

	static void _instance_update_mesh_instance(Instance* p_instance);

	static RID scenario_allocate();
	static void scenario_initialize(RID p_rid);

	static void scenario_set_environment(RID p_scenario, RID p_environment);
	static void scenario_set_camera_attributes(RID p_scenario, RID p_attributes);
	static void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	static void scenario_set_compositor(RID p_scenario, RID p_compositor);
	static void scenario_set_reflection_atlas_size(
		RID p_scenario, int p_reflection_size, int p_reflection_count);
	static bool is_scenario(RID p_scenario);
	static RID scenario_get_environment(RID p_scenario);
	static void scenario_add_viewport_visibility_mask(RID p_scenario, RID p_viewport);
	static void scenario_remove_viewport_visibility_mask(RID p_scenario, RID p_viewport);

	/* INSTANCING API */

	struct InstancePair
	{
		Instance* a = nullptr;
		Instance* b = nullptr;
		SelfList<InstancePair> list_a;
		SelfList<InstancePair> list_b;

		InstancePair() : list_a(this), list_b(this) {}
	};


	struct InstanceBaseData
	{
		struct LightCache
		{
			RSE::LightType type;
			Transform3D transform;
			Color color;
			float energy;
			float intensity;
			float bake_energy;
			float radius;
			float attenuation;
			float spot_angle;
			float spot_attenuation;
			bool has_shadow;
			RSE::LightDirectionalSkyMode sky_mode;
			Vector2 area_size;
			bool area_normalize_energy;
			RID area_texture;
		};

		// Shared fields across instance base types
		Instance* owner = nullptr;
		RID instance;
		uint32_t cull_mask = 0xFFFFFFFF;
		HashSet<Instance*> geometries;
		HashSet<Instance*> lights;

		// Geometry data
		RenderGeometryInstance* geometry_instance = nullptr;
		bool can_cast_shadows = true;
		bool material_is_animated = true;
		uint32_t projector_count = 0;
		uint32_t softshadow_count = 0;
		HashSet<Instance*> decals;
		HashSet<Instance*> reflection_probes;
		HashSet<Instance*> voxel_gi_instances;
		HashSet<Instance*> lightmap_captures;

		// Reflection probe data
		SelfList<InstanceBaseData> update_list;
		int render_step = -1;

		// Fog volume data
		bool is_global = false;

		// Visibility notifier data
		bool just_visible = false;
		uint64_t visible_in_frame = 0;
		RID base;
		SelfList<InstanceBaseData> list_element;

		// Light data
		uint64_t last_version = 0;
		List<Instance*>::Element* D = nullptr; // directional light in scenario
		bool uses_projector = false;
		bool uses_softshadow = false;
		Instance* baked_light = nullptr;
		RSE::LightBakeMode bake_mode = RSE::LIGHT_BAKE_DISABLED;
		uint32_t max_sdfgi_cascade = 2;

		// VoxelGI data
		HashSet<Instance*> dynamic_geometries;
		Vector<LightCache> light_cache;
		Vector<RID> light_instances;
		RID probe_instance;
		bool invalid = true;
		uint32_t base_version = 0;
		SelfList<InstanceBaseData> update_element;

		// Lightmap data
		HashSet<Instance*> users;

	private:
		// Instead of a single dirty flag, we maintain a count
		// so that we can detect lights that are being made dirty
		// each frame, and switch on tighter caster culling.
		int32_t shadow_dirty_count = 1;

		uint32_t light_update_frame_id = UINT32_MAX;
		bool light_intersects_multiple_cameras = false;
		uint32_t light_intersects_multiple_cameras_timeout_frame_id = UINT32_MAX;

	public:
		bool is_shadow_dirty() const { return shadow_dirty_count != 0; }

		void make_shadow_dirty() { shadow_dirty_count = light_intersects_multiple_cameras ? 1 : 2; }

		void detect_light_intersects_multiple_cameras(uint32_t p_frame_id)
		{
			if (p_frame_id == light_update_frame_id) {
				light_intersects_multiple_cameras = true;
				light_intersects_multiple_cameras_timeout_frame_id = p_frame_id + 60;
			}
			else {
				if (light_intersects_multiple_cameras &&
					(p_frame_id >= light_intersects_multiple_cameras_timeout_frame_id)) {
					light_intersects_multiple_cameras = false;
					light_intersects_multiple_cameras_timeout_frame_id = UINT32_MAX;
				}
			}
			light_update_frame_id = p_frame_id;
		}

		void decrement_shadow_dirty()
		{
			shadow_dirty_count--;
			DEV_ASSERT(shadow_dirty_count >= 0);
		}

		// Shadow updates can either full (everything in the shadow volume)
		// or closely culled to the camera frustum.
		bool is_shadow_update_full() const { return shadow_dirty_count == 0; }

		InstanceBaseData() : update_list(this), list_element(this), update_element(this) {}
		~InstanceBaseData() = default;
	};

	struct Instance
	{
		RSE::InstanceType base_type;
		RID base;

		RID skeleton;
		RID material_override;
		RID material_overlay;

		RID mesh_instance; // only used for meshes and when skeleton/blendshapes exist

		Transform3D transform;
		bool teleported = false;

		float lod_bias;

		bool ignore_occlusion_culling;
		bool ignore_all_culling;

		Vector<RID> materials;

		RSE::ShadowCastingSetting cast_shadows;

		uint32_t layer_mask;
		// Fit in 32 bits.
		bool mirror : 1;
		bool receive_shadows : 1;
		bool visible : 1;
		bool baked_light : 1; // This flag is only to know if it actually did use baked light.
		bool dynamic_gi : 1;  // Same as above for dynamic objects.
		bool redraw_if_visible : 1;

		Instance* lightmap = nullptr;
		Rect2 lightmap_uv_scale;
		int lightmap_slice_index;
		uint32_t lightmap_cull_index;
		Vector<Color> lightmap_sh; // spherical harmonic

		AABB aabb;
		AABB transformed_aabb;
		AABB prev_transformed_aabb;

		InstanceUniforms instance_uniforms;

		//

		RID self;
		// scenario stuff
		DynamicBVH::ID indexer_id;
		int32_t array_index = -1;
		int32_t visibility_index = -1;
		float visibility_range_begin = 0.0f;
		float visibility_range_end = 0.0f;
		float visibility_range_begin_margin = 0.0f;
		float visibility_range_end_margin = 0.0f;
		RSE::VisibilityRangeFadeMode visibility_range_fade_mode =
			RSE::VISIBILITY_RANGE_FADE_DISABLED;
		Instance* visibility_parent = nullptr;
		HashSet<Instance*> visibility_dependencies;
		uint32_t visibility_dependencies_depth = 0;
		float transparency = 0.0f;
		Scenario* scenario = nullptr;
		SelfList<Instance> scenario_item;

		// aabb stuff
		bool update_aabb;
		bool update_dependencies;

		SelfList<Instance> update_item;

		AABB* custom_aabb = nullptr; // <Zylann> would using aabb directly with a bool be better?
		float extra_margin;

		// sorting
		float sorting_offset = 0.0;
		bool use_aabb_center = true;

		Vector<Color> lightmap_target_sh; // target is used for incrementally changing the SH over
										  // time, this avoids pops in some corner cases and when
										  // going interior <-> exterior

		uint64_t last_frame_pass;

		uint64_t version; // changes to this, and changes to base increase version

		InstanceBaseData* base_data = nullptr;

		SelfList<InstancePair>::List pairs;
		uint64_t pair_check;

		DependencyTracker dependency_tracker;

		static void dependency_changed(
			Dependency::DependencyChangedNotification p_notification, DependencyTracker* tracker)
		{
			Instance* instance = (Instance*)tracker->userdata;
			switch (p_notification) {
			case Dependency::DEPENDENCY_CHANGED_SKELETON_DATA:
			case Dependency::DEPENDENCY_CHANGED_SKELETON_BONES:
			case Dependency::DEPENDENCY_CHANGED_AABB: {
				RendererSceneCull::_instance_queue_update(instance, true, false);

			} break;
			case Dependency::DEPENDENCY_CHANGED_MULTIMESH_VISIBLE_INSTANCES:
			case Dependency::DEPENDENCY_CHANGED_MATERIAL: {
				RendererSceneCull::_instance_queue_update(instance, false, true);
			} break;
			case Dependency::DEPENDENCY_CHANGED_MESH:
			case Dependency::DEPENDENCY_CHANGED_PARTICLES:
			case Dependency::DEPENDENCY_CHANGED_MULTIMESH:
			case Dependency::DEPENDENCY_CHANGED_DECAL:
			case Dependency::DEPENDENCY_CHANGED_LIGHT: {
				RendererSceneCull::_instance_queue_update(instance, true, true);
			} break;
			case Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE:
			case Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR:
			case Dependency::DEPENDENCY_CHANGED_CULL_MASK: {
				// requires repairing
				if (instance->indexer_id.is_valid()) {
					RendererSceneCull::_unpair_instance(instance);
					RendererSceneCull::_instance_queue_update(instance, true, true);
				}

			} break;
			default: {
				// Ignored notifications.
			} break;
			}
		}

		static void dependency_deleted(const RID& p_dependency, DependencyTracker* tracker)
		{
			Instance* instance = (Instance*)tracker->userdata;

			if (p_dependency == instance->base) {
				RendererSceneCull::instance_set_base(instance->self, RID());
			}
			else if (p_dependency == instance->skeleton) {
				RendererSceneCull::instance_attach_skeleton(instance->self, RID());
			}
			else {
				// It's possible the same material is used in multiple slots,
				// so we check whether we need to clear them all.
				if (p_dependency == instance->material_override) {
					RendererSceneCull::instance_geometry_set_material_override(instance->self, RID());
				}
				if (p_dependency == instance->material_overlay) {
					RendererSceneCull::instance_geometry_set_material_overlay(instance->self, RID());
				}
				for (int i = 0; i < instance->materials.size(); i++) {
					if (p_dependency == instance->materials[i]) {
						RendererSceneCull::instance_set_surface_override_material(instance->self, i, RID());
					}
				}
				if (instance->base_type == RSE::INSTANCE_PARTICLES) {
					RID particle_material =
						Renderer::particles_storage->particles_get_process_material(instance->base);
					if (p_dependency == particle_material) {
						Renderer::particles_storage->particles_set_process_material(
							instance->base, RID());
					}
				}

				// Even if no change is made we still need to call `_instance_queue_update`.
				// This dependency could also be a result of the freed material being used
				// by the mesh this mesh instance uses.
				RendererSceneCull::_instance_queue_update(instance, false, true);
			}
		}

		Instance() : scenario_item(this), update_item(this)
		{
			base_type = RSE::INSTANCE_NONE;
			cast_shadows = RSE::SHADOW_CASTING_SETTING_ON;
			receive_shadows = true;
			visible = true;
			layer_mask = 1;
			baked_light = true;
			dynamic_gi = false;
			redraw_if_visible = false;

			lightmap_slice_index = 0;
			lightmap = nullptr;
			lightmap_cull_index = 0;
			lod_bias = 1.0;
			ignore_occlusion_culling = false;
			ignore_all_culling = false;

			scenario = nullptr;

			update_aabb = false;
			update_dependencies = false;

			extra_margin = 0;

			visible = true;

			visibility_range_begin = 0;
			visibility_range_end = 0;
			visibility_range_begin_margin = 0;
			visibility_range_end_margin = 0;

			last_frame_pass = 0;
			version = 1;
			base_data = nullptr;

			custom_aabb = nullptr;

			pair_check = 0;
			array_index = -1;

			dependency_tracker.userdata = this;
			dependency_tracker.changed_callback = dependency_changed;
			dependency_tracker.deleted_callback = dependency_deleted;
		}

		~Instance()
		{
			if (base_data) {
				memdelete(base_data);
			}
			if (custom_aabb) {
				memdelete(custom_aabb);
			}
		}
	};

	static void _instance_queue_update(
		Instance* p_instance, bool p_update_aabb, bool p_update_dependencies = false);


	struct PairInstances
	{
		Instance* instance = nullptr;
		PagedAllocator<InstancePair>* pair_allocator = nullptr;
		SelfList<InstancePair>::List pairs_found;
		DynamicBVH* bvh = nullptr;
		DynamicBVH* bvh2 = nullptr; // some may need to cull in two
		uint32_t pair_mask;
		uint64_t pair_pass;

		_FORCE_INLINE_ bool operator()(void* p_data)
		{
			Instance* p_instance = (Instance*)p_data;

			if (instance != p_instance &&
				instance->transformed_aabb.intersects(p_instance->transformed_aabb) &&
				(pair_mask & (1 << p_instance->base_type))) {
				// test is more coarse in indexer
				p_instance->pair_check = pair_pass;
				InstancePair* pair = pair_allocator->alloc();
				pair->a = instance;
				pair->b = p_instance;
				pairs_found.add(&pair->list_a);
			}
			return false;
		}

		void pair()
		{
			if (bvh) {
				bvh->aabb_query(instance->transformed_aabb, *this);
			}
			if (bvh2) {
				bvh2->aabb_query(instance->transformed_aabb, *this);
			}
			while (instance->pairs.first()) {
				InstancePair* pair = instance->pairs.first()->self();
				Instance* other_instance = instance == pair->a ? pair->b : pair->a;
				if (other_instance->pair_check != pair_pass) {
					// unpaired
					_instance_unpair(instance, other_instance);
				}
				else {
					// kept
					other_instance->pair_check = 0; // if kept, then put pair check to zero, so we
													// can distinguish with the newly added ones
				}

				pair_allocator->free(pair);
			}
			while (pairs_found.first()) {
				InstancePair* pair = pairs_found.first()->self();
				pairs_found.remove(pairs_found.first());

				if (pair->b->pair_check == pair_pass) {
					// paired
					_instance_pair(instance, pair->b);
				}
				pair->a->pairs.add(&pair->list_a);
				pair->b->pairs.add(&pair->list_b);
			}
		}
	};


	struct InstanceCullResult
	{
		PagedArray<RenderGeometryInstance*> geometry_instances;
		PagedArray<Instance*> lights;
		PagedArray<RID> light_instances;
		PagedArray<RID> lightmaps;
		PagedArray<RID> reflections;
		PagedArray<RID> decals;
		PagedArray<RID> voxel_gi_instances;
		PagedArray<RID> mesh_instances;
		PagedArray<RID> fog_volumes;

		struct DirectionalShadow
		{
			PagedArray<RenderGeometryInstance*>
				cascade_geometry_instances[RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES];
		} directional_shadows[RendererSceneRender::MAX_DIRECTIONAL_LIGHTS];

		PagedArray<RenderGeometryInstance*>
			sdfgi_region_geometry_instances[SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE];
		PagedArray<RID> sdfgi_cascade_lights[SDFGI_MAX_CASCADES];

		void clear()
		{
			geometry_instances.clear();
			lights.clear();
			light_instances.clear();
			lightmaps.clear();
			reflections.clear();
			decals.clear();
			voxel_gi_instances.clear();
			mesh_instances.clear();
			fog_volumes.clear();
			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].clear();
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].clear();
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].clear();
			}
		}

		void reset()
		{
			geometry_instances.reset();
			lights.reset();
			light_instances.reset();
			lightmaps.reset();
			reflections.reset();
			decals.reset();
			voxel_gi_instances.reset();
			mesh_instances.reset();
			fog_volumes.reset();
			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].reset();
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].reset();
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].reset();
			}
		}

		void append_from(InstanceCullResult& p_cull_result)
		{
			geometry_instances.merge_unordered(p_cull_result.geometry_instances);
			lights.merge_unordered(p_cull_result.lights);
			light_instances.merge_unordered(p_cull_result.light_instances);
			lightmaps.merge_unordered(p_cull_result.lightmaps);
			reflections.merge_unordered(p_cull_result.reflections);
			decals.merge_unordered(p_cull_result.decals);
			voxel_gi_instances.merge_unordered(p_cull_result.voxel_gi_instances);
			mesh_instances.merge_unordered(p_cull_result.mesh_instances);
			fog_volumes.merge_unordered(p_cull_result.fog_volumes);

			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].merge_unordered(
						p_cull_result.directional_shadows[i].cascade_geometry_instances[j]);
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].merge_unordered(
					p_cull_result.sdfgi_region_geometry_instances[i]);
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].merge_unordered(p_cull_result.sdfgi_cascade_lights[i]);
			}
		}

		void init(PagedArrayPool<RID>* p_rid_pool,
			PagedArrayPool<RenderGeometryInstance*>* p_geometry_instance_pool,
			PagedArrayPool<Instance*>* p_instance_pool)
		{
			geometry_instances.set_page_pool(p_geometry_instance_pool);
			light_instances.set_page_pool(p_rid_pool);
			lights.set_page_pool(p_instance_pool);
			lightmaps.set_page_pool(p_rid_pool);
			reflections.set_page_pool(p_rid_pool);
			decals.set_page_pool(p_rid_pool);
			voxel_gi_instances.set_page_pool(p_rid_pool);
			mesh_instances.set_page_pool(p_rid_pool);
			fog_volumes.set_page_pool(p_rid_pool);
			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].set_page_pool(
						p_geometry_instance_pool);
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].set_page_pool(p_geometry_instance_pool);
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].set_page_pool(p_rid_pool);
			}
		}
	};


	static RID instance_allocate();
	static void instance_initialize(RID p_rid);

	static void instance_set_base(RID p_instance, RID p_base);
	static void instance_set_scenario(RID p_instance, RID p_scenario);
	static void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	static void instance_set_pivot_data(RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	static void instance_set_transform(RID p_instance, const Transform3D& p_transform);
	static void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	static void instance_set_surface_override_material(RID p_instance, int p_surface, RID p_material);
	static void instance_set_visible(RID p_instance, bool p_visible);
	static void instance_geometry_set_transparency(RID p_instance, float p_transparency);

	static void instance_teleport(RID p_instance);

	static void instance_set_custom_aabb(RID p_instance, AABB p_aabb);

	static void instance_attach_skeleton(RID p_instance, RID p_skeleton);

	static void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);

	static void instance_set_visibility_parent(RID p_instance, RID p_parent_instance);

	static void instance_set_ignore_culling(RID p_instance, bool p_enabled);

	static bool _update_instance_visibility_depth(Instance* p_instance);
	static void _update_instance_visibility_dependencies(Instance* p_instance);

	static void instance_geometry_set_flag(RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled);
	static void instance_geometry_set_cast_shadows_setting(
		RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting);
	static void instance_geometry_set_material_override(RID p_instance, RID p_material);
	static void instance_geometry_set_material_overlay(RID p_instance, RID p_material);

	static void instance_geometry_set_visibility_range(RID p_instance, float p_min, float p_max,
		float p_min_margin, float p_max_margin, RSE::VisibilityRangeFadeMode p_fade_mode);

	static void instance_geometry_set_lightmap(
		RID p_instance, RID p_lightmap, const Rect2& p_lightmap_uv_scale, int p_slice_index);
	static void instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias);

	static void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation);
	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	static _FORCE_INLINE_ void _update_instance(Instance* p_instance);
	static _FORCE_INLINE_ void _update_instance_aabb(Instance* p_instance);
	static _FORCE_INLINE_ void _update_instance_lightmap_captures(Instance* p_instance);
	static void _unpair_instance(Instance* p_instance);

	static void _light_instance_setup_directional_shadow(int p_shadow_index, Instance* p_instance,
		const Transform3D p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, bool p_cam_vaspect);

	static _FORCE_INLINE_ bool _light_instance_update_shadow(Instance* p_instance,
		const Transform3D p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, bool p_cam_vaspect, RID p_shadow_atlas, Scenario* p_scenario,
		float p_screen_mesh_lod_threshold, uint32_t p_visible_layers = 0xFFFFFF);

	static RID _render_get_environment(RID p_camera, RID p_scenario);
	static RID _render_get_compositor(RID p_camera, RID p_scenario);

	struct Cull
	{
		struct Shadow
		{
			RID light_instance;
			uint32_t caster_mask;

			struct Cascade
			{
				Frustum frustum;

				Projection projection;
				Transform3D transform;
				real_t zfar;
				real_t split;
				real_t shadow_texel_size;
				real_t bias_scale;
				real_t range_begin;
				Vector2 uv_scale;

			} cascades[RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES]; // max 4 cascades

			uint32_t cascade_count;

		} shadows[RendererSceneRender::MAX_DIRECTIONAL_LIGHTS];

		uint32_t shadow_count;

		struct SDFGI
		{
			// have arrays here because SDFGI functions expects this, plus regions can have areas
			AABB region_aabb[SDFGI_MAX_CASCADES *
							 SDFGI_MAX_REGIONS_PER_CASCADE]; // max 3 regions per cascade
			uint32_t region_cascade[SDFGI_MAX_CASCADES *
									SDFGI_MAX_REGIONS_PER_CASCADE]; // max 3 regions per cascade
			uint32_t region_count = 0;

			uint32_t cascade_light_index[SDFGI_MAX_CASCADES];
			uint32_t cascade_light_count = 0;

		} sdfgi;

		SpinLock lock;

		Frustum frustum;
	};

	struct VisibilityCullData
	{
		uint64_t viewport_mask;
		Scenario* scenario = nullptr;
		Vector3 camera_position;
		uint32_t cull_offset;
		uint32_t cull_count;
	};

	static void _visibility_cull_threaded(uint32_t p_thread, VisibilityCullData* cull_data);
	static void _visibility_cull(const VisibilityCullData& cull_data, uint64_t p_from, uint64_t p_to);
	template <bool p_fade_check>
	static _FORCE_INLINE_ int _visibility_range_check(
		InstanceVisibilityData& r_vis_data, const Vector3& p_camera_pos, uint64_t p_viewport_mask);

	struct CullData
	{
		Cull* cull = nullptr;
		Scenario* scenario = nullptr;
		RID shadow_atlas;
		Transform3D cam_transform;
		uint32_t visible_layers;
		Instance* render_reflection_probe = nullptr;
		const RendererSceneOcclusionCull::HZBuffer* occlusion_buffer;
		const Projection* camera_matrix;
		uint64_t visibility_viewport_mask;
	};

	static void _scene_cull_threaded(uint32_t p_thread, CullData* cull_data);
	static void _scene_cull(
		CullData& cull_data, InstanceCullResult& cull_result, uint64_t p_from, uint64_t p_to);
	static void _scene_particles_set_view_axis(
		RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis);
	static _FORCE_INLINE_ bool _visibility_parent_check(
		const CullData& p_cull_data, const InstanceData& p_instance_data);

	static bool _render_reflection_probe_step(Instance* p_instance, int p_step);

	static void render_empty_scene(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_scenario,
		RID p_shadow_atlas, float p_window_output_max_value);

	static void render_camera(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_camera,
		RID p_scenario, RID p_viewport, Size2 p_viewport_size, uint32_t p_jitter_phase_count,
		float p_screen_mesh_lod_threshold, RID p_shadow_atlas, Ref<XRInterface>& p_xr_interface,
		float p_window_output_max_value, RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	static void render_particle_colliders();
	static void render_probes();

	struct InterpolationData
	{
		bool interpolation_enabled = false;
	};

	struct Data
	{
		RendererSceneRender* scene_render = nullptr;
		uint64_t render_pass = 0;

		RID_Owner<Camera, true> camera_owner;

		PagedArrayPool<InstanceBounds> instance_aabb_page_pool;
		PagedArrayPool<InstanceData> instance_data_page_pool;
		PagedArrayPool<InstanceVisibilityData> instance_visibility_data_page_pool;

		int indexer_update_iterations = 0;
		RID_Owner<Scenario, true> scenario_owner;

		PagedAllocator<InstancePair> pair_allocator;
		SelfList<Instance>::List _instance_update_list;
		SelfList<InstanceBaseData>::List reflection_probe_render_list;

		SpinLock visible_notifier_list_lock;
		SelfList<InstanceBaseData>::List visible_notifier_list;
		SelfList<InstanceBaseData>::List voxel_gi_update_list;

		uint64_t pair_pass = 1;
		HashSet<Instance*> heightfield_particle_colliders_update_list;

		PagedArrayPool<Instance*> instance_cull_page_pool;
		PagedArrayPool<RenderGeometryInstance*> geometry_instance_cull_page_pool;
		PagedArrayPool<RID> rid_cull_page_pool;

		PagedArray<Instance*> instance_cull_result;
		PagedArray<Instance*> instance_shadow_cull_result;

		InstanceCullResult scene_cull_result;
		LocalVector<InstanceCullResult> scene_cull_result_threads;

		RendererSceneRender::RenderShadowData render_shadow_data[MAX_UPDATE_SHADOWS];
		uint32_t max_shadows_used = 0;

		RendererSceneRender::RenderSDFGIData
			render_sdfgi_data[SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE];
		RendererSceneRender::RenderSDFGIUpdateData sdfgi_update_data;

		uint32_t thread_cull_threshold = 200;

		RID_Owner<Instance, true> instance_owner{65536, 4194304};

		uint32_t geometry_instance_pair_mask = 0;

		LocalVector<Vector2> camera_jitter_array;
		RenderingLightCuller* light_culler = nullptr;

		Cull cull;
		InterpolationData _interpolation_data;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }
	static _FORCE_INLINE_ RendererSceneRender* get_scene_render()
	{
		return data ? data->scene_render : nullptr;
	}

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_arg1) { if (data && data->scene_render) { data->scene_render->voxel_gi_set_quality(p_arg1); } }

	/* SKY API */

	static RID sky_allocate() { ERR_FAIL_COND_V(!data || !data->scene_render, RID()); return data->scene_render->sky_allocate(); }
	static void sky_initialize(RID p_arg1) { if (data && data->scene_render) { data->scene_render->sky_initialize(p_arg1); } }

	static void sky_set_radiance_size(RID p_arg1, int p_arg2) { if (data && data->scene_render) { data->scene_render->sky_set_radiance_size(p_arg1, p_arg2); } }
	static void sky_set_mode(RID p_arg1, RSE::SkyMode p_arg2) { if (data && data->scene_render) { data->scene_render->sky_set_mode(p_arg1, p_arg2); } }
	static void sky_set_material(RID p_arg1, RID p_arg2) { if (data && data->scene_render) { data->scene_render->sky_set_material(p_arg1, p_arg2); } }
	static Ref<Image> sky_bake_panorama(RID p_arg1, float p_arg2, bool p_arg3, const Size2i& p_arg4) { ERR_FAIL_COND_V(!data || !data->scene_render, Ref<Image>()); return data->scene_render->sky_bake_panorama(p_arg1, p_arg2, p_arg3, p_arg4); }

	// Compositor effect

	static RID compositor_effect_allocate() { return RendererSceneRender::compositor_effect_allocate(); }
	static void compositor_effect_initialize(RID p_arg1) { RendererSceneRender::compositor_effect_initialize(p_arg1); }

	static bool is_compositor_effect(RID p_arg1) { return RendererSceneRender::is_compositor_effect(p_arg1); }

	static void compositor_effect_set_enabled(RID p_arg1, bool p_arg2) { RendererSceneRender::compositor_effect_set_enabled(p_arg1, p_arg2); }
	static void compositor_effect_set_flag(RID p_arg1, RSE::CompositorEffectFlags p_arg2, bool p_arg3) { RendererSceneRender::compositor_effect_set_flag(p_arg1, p_arg2, p_arg3); }

	// Compositor

	static RID compositor_allocate() { return RendererSceneRender::compositor_allocate(); }
	static void compositor_initialize(RID p_arg1) { RendererSceneRender::compositor_initialize(p_arg1); }

	static bool is_compositor(RID p_arg1) { return RendererSceneRender::is_compositor(p_arg1); }

	// Environment

	static RID environment_allocate() { return RendererSceneRender::environment_allocate(); }
	static void environment_initialize(RID p_arg1) { RendererSceneRender::environment_initialize(p_arg1); }

	static bool is_environment(RID p_arg1) { return RendererSceneRender::is_environment(p_arg1); }

	// Background
	static void environment_set_background(RID p_arg1, RSE::EnvironmentBG p_arg2) { RendererSceneRender::environment_set_background(p_arg1, p_arg2); }
	static void environment_set_sky(RID p_arg1, RID p_arg2) { RendererSceneRender::environment_set_sky(p_arg1, p_arg2); }
	static void environment_set_sky_custom_fov(RID p_arg1, float p_arg2) { RendererSceneRender::environment_set_sky_custom_fov(p_arg1, p_arg2); }
	static void environment_set_sky_orientation(RID p_arg1, const Basis& p_arg2) { RendererSceneRender::environment_set_sky_orientation(p_arg1, p_arg2); }
	static void environment_set_bg_color(RID p_arg1, const Color& p_arg2) { RendererSceneRender::environment_set_bg_color(p_arg1, p_arg2); }
	static void environment_set_bg_energy(RID p_arg1, float p_arg2, float p_arg3) { RendererSceneRender::environment_set_bg_energy(p_arg1, p_arg2, p_arg3); }
	static void environment_set_canvas_max_layer(RID p_arg1, int p_arg2) { RendererSceneRender::environment_set_canvas_max_layer(p_arg1, p_arg2); }
	static void environment_set_ambient_light(RID p_arg1, const Color& p_arg2, RSE::EnvironmentAmbientSource p_arg3, float p_arg4, float p_arg5, RSE::EnvironmentReflectionSource p_arg6) { RendererSceneRender::environment_set_ambient_light(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); }
	static void environment_set_camera_feed_id(RID p_arg1, int p_arg2) { RendererSceneRender::environment_set_camera_feed_id(p_arg1, p_arg2); }

	static RSE::EnvironmentBG environment_get_background(RID p_arg1) { return RendererSceneRender::environment_get_background(p_arg1); }
	static RID environment_get_sky(RID p_arg1) { return RendererSceneRender::environment_get_sky(p_arg1); }
	static float environment_get_sky_custom_fov(RID p_arg1) { return RendererSceneRender::environment_get_sky_custom_fov(p_arg1); }
	static Basis environment_get_sky_orientation(RID p_arg1) { return RendererSceneRender::environment_get_sky_orientation(p_arg1); }
	static Color environment_get_bg_color(RID p_arg1) { return RendererSceneRender::environment_get_bg_color(p_arg1); }
	static float environment_get_bg_energy_multiplier(RID p_arg1) { return RendererSceneRender::environment_get_bg_energy_multiplier(p_arg1); }
	static float environment_get_bg_intensity(RID p_arg1) { return RendererSceneRender::environment_get_bg_intensity(p_arg1); }
	static int environment_get_canvas_max_layer(RID p_arg1) { return RendererSceneRender::environment_get_canvas_max_layer(p_arg1); }
	static RSE::EnvironmentAmbientSource environment_get_ambient_source(RID p_arg1) { return RendererSceneRender::environment_get_ambient_source(p_arg1); }
	static Color environment_get_ambient_light(RID p_arg1) { return RendererSceneRender::environment_get_ambient_light(p_arg1); }
	static float environment_get_ambient_light_energy(RID p_arg1) { return RendererSceneRender::environment_get_ambient_light_energy(p_arg1); }
	static float environment_get_ambient_sky_contribution(RID p_arg1) { return RendererSceneRender::environment_get_ambient_sky_contribution(p_arg1); }
	static RSE::EnvironmentReflectionSource environment_get_reflection_source(RID p_arg1) { return RendererSceneRender::environment_get_reflection_source(p_arg1); }

	// Tonemap
	static void environment_set_tonemap(RID p_arg1, RSE::EnvironmentToneMapper p_arg2, float p_arg3, float p_arg4) { RendererSceneRender::environment_set_tonemap(p_arg1, p_arg2, p_arg3, p_arg4); }
	static void environment_set_tonemap_agx_contrast(RID p_arg1, float p_arg2) { RendererSceneRender::environment_set_tonemap_agx_contrast(p_arg1, p_arg2); }
	static RSE::EnvironmentToneMapper environment_get_tone_mapper(RID p_arg1) { return RendererSceneRender::environment_get_tone_mapper(p_arg1); }
	static float environment_get_exposure(RID p_arg1) { return RendererSceneRender::environment_get_exposure(p_arg1); }
	static float environment_get_white(RID p_arg1, bool p_arg2, float p_arg3) { return RendererSceneRender::environment_get_white(p_arg1, p_arg2, p_arg3); }

	// Fog
	static void environment_set_fog(RID p_arg1, bool p_arg2, const Color& p_arg3, float p_arg4, float p_arg5, float p_arg6, float p_arg7, float p_arg8, float p_arg9, float p_arg10, RSE::EnvironmentFogMode p_arg11) { RendererSceneRender::environment_set_fog(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11); }

	static bool environment_get_fog_enabled(RID p_arg1) { return RendererSceneRender::environment_get_fog_enabled(p_arg1); }
	static Color environment_get_fog_light_color(RID p_arg1) { return RendererSceneRender::environment_get_fog_light_color(p_arg1); }
	static float environment_get_fog_light_energy(RID p_arg1) { return RendererSceneRender::environment_get_fog_light_energy(p_arg1); }
	static float environment_get_fog_sun_scatter(RID p_arg1) { return RendererSceneRender::environment_get_fog_sun_scatter(p_arg1); }
	static float environment_get_fog_density(RID p_arg1) { return RendererSceneRender::environment_get_fog_density(p_arg1); }
	static float environment_get_fog_sky_affect(RID p_arg1) { return RendererSceneRender::environment_get_fog_sky_affect(p_arg1); }
	static float environment_get_fog_height(RID p_arg1) { return RendererSceneRender::environment_get_fog_height(p_arg1); }
	static float environment_get_fog_height_density(RID p_arg1) { return RendererSceneRender::environment_get_fog_height_density(p_arg1); }
	static float environment_get_fog_aerial_perspective(RID p_arg1) { return RendererSceneRender::environment_get_fog_aerial_perspective(p_arg1); }
	static RSE::EnvironmentFogMode environment_get_fog_mode(RID p_arg1) { return RendererSceneRender::environment_get_fog_mode(p_arg1); }

	static void environment_set_volumetric_fog_volume_size(int p_arg1, int p_arg2) { if (data && data->scene_render) { data->scene_render->environment_set_volumetric_fog_volume_size(p_arg1, p_arg2); } }
	static void environment_set_volumetric_fog_filter_active(bool p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_volumetric_fog_filter_active(p_arg1); } }

	// Depth Fog
	static void environment_set_fog_depth(RID p_arg1, float p_arg2, float p_arg3, float p_arg4) { RendererSceneRender::environment_set_fog_depth(p_arg1, p_arg2, p_arg3, p_arg4); }
	static float environment_get_fog_depth_curve(RID p_arg1) { return RendererSceneRender::environment_get_fog_depth_curve(p_arg1); }
	static float environment_get_fog_depth_begin(RID p_arg1) { return RendererSceneRender::environment_get_fog_depth_begin(p_arg1); }
	static float environment_get_fog_depth_end(RID p_arg1) { return RendererSceneRender::environment_get_fog_depth_end(p_arg1); }

	// Volumentric Fog
	static void environment_set_volumetric_fog(RID p_arg1, bool p_arg2, float p_arg3, const Color& p_arg4, const Color& p_arg5, float p_arg6, float p_arg7, float p_arg8, float p_arg9, float p_arg10, bool p_arg11, float p_arg12, float p_arg13, float p_arg14) { RendererSceneRender::environment_set_volumetric_fog(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11, p_arg12, p_arg13, p_arg14); }

	static bool environment_get_volumetric_fog_enabled(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_enabled(p_arg1); }
	static float environment_get_volumetric_fog_density(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_density(p_arg1); }
	static Color environment_get_volumetric_fog_scattering(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_scattering(p_arg1); }
	static Color environment_get_volumetric_fog_emission(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_emission(p_arg1); }
	static float environment_get_volumetric_fog_emission_energy(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_emission_energy(p_arg1); }
	static float environment_get_volumetric_fog_anisotropy(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_anisotropy(p_arg1); }
	static float environment_get_volumetric_fog_length(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_length(p_arg1); }
	static float environment_get_volumetric_fog_detail_spread(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_detail_spread(p_arg1); }
	static float environment_get_volumetric_fog_gi_inject(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_gi_inject(p_arg1); }
	static float environment_get_volumetric_fog_sky_affect(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_sky_affect(p_arg1); }
	static bool environment_get_volumetric_fog_temporal_reprojection(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection(p_arg1); }
	static float environment_get_volumetric_fog_temporal_reprojection_amount(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection_amount(p_arg1); }
	static float environment_get_volumetric_fog_ambient_inject(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_ambient_inject(p_arg1); }

	// Glow
	static void environment_set_glow(RID p_arg1, bool p_arg2, Vector<float> p_arg3, float p_arg4, float p_arg5, float p_arg6, float p_arg7, RSE::EnvironmentGlowBlendMode p_arg8, float p_arg9, float p_arg10, float p_arg11, float p_arg12, RID p_arg13) { RendererSceneRender::environment_set_glow(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11, p_arg12, p_arg13); }

	static bool environment_get_glow_enabled(RID p_arg1) { return RendererSceneRender::environment_get_glow_enabled(p_arg1); }
	static Vector<float> environment_get_glow_levels(RID p_arg1) { return RendererSceneRender::environment_get_glow_levels(p_arg1); }
	static float environment_get_glow_intensity(RID p_arg1) { return RendererSceneRender::environment_get_glow_intensity(p_arg1); }
	static float environment_get_glow_strength(RID p_arg1) { return RendererSceneRender::environment_get_glow_strength(p_arg1); }
	static float environment_get_glow_bloom(RID p_arg1) { return RendererSceneRender::environment_get_glow_bloom(p_arg1); }
	static float environment_get_glow_mix(RID p_arg1) { return RendererSceneRender::environment_get_glow_mix(p_arg1); }
	static RSE::EnvironmentGlowBlendMode environment_get_glow_blend_mode(RID p_arg1) { return RendererSceneRender::environment_get_glow_blend_mode(p_arg1); }
	static float environment_get_glow_hdr_bleed_threshold(RID p_arg1) { return RendererSceneRender::environment_get_glow_hdr_bleed_threshold(p_arg1); }
	static float environment_get_glow_hdr_luminance_cap(RID p_arg1) { return RendererSceneRender::environment_get_glow_hdr_luminance_cap(p_arg1); }
	static float environment_get_glow_hdr_bleed_scale(RID p_arg1) { return RendererSceneRender::environment_get_glow_hdr_bleed_scale(p_arg1); }
	static float environment_get_glow_map_strength(RID p_arg1) { return RendererSceneRender::environment_get_glow_map_strength(p_arg1); }
	static RID environment_get_glow_map(RID p_arg1) { return RendererSceneRender::environment_get_glow_map(p_arg1); }

	static void environment_glow_set_use_bicubic_upscale(bool p_arg1) { if (data && data->scene_render) { data->scene_render->environment_glow_set_use_bicubic_upscale(p_arg1); } }

	// SSR
	static void environment_set_ssr(RID p_arg1, bool p_arg2, int p_arg3, float p_arg4, float p_arg5, float p_arg6) { RendererSceneRender::environment_set_ssr(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); }

	static bool environment_get_ssr_enabled(RID p_arg1) { return RendererSceneRender::environment_get_ssr_enabled(p_arg1); }
	static int environment_get_ssr_max_steps(RID p_arg1) { return RendererSceneRender::environment_get_ssr_max_steps(p_arg1); }
	static float environment_get_ssr_fade_in(RID p_arg1) { return RendererSceneRender::environment_get_ssr_fade_in(p_arg1); }
	static float environment_get_ssr_fade_out(RID p_arg1) { return RendererSceneRender::environment_get_ssr_fade_out(p_arg1); }
	static float environment_get_ssr_depth_tolerance(RID p_arg1) { return RendererSceneRender::environment_get_ssr_depth_tolerance(p_arg1); }

	static void environment_set_ssr_half_size(bool p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_ssr_half_size(p_arg1); } }
	static void environment_set_ssr_roughness_quality(RSE::EnvironmentSSRRoughnessQuality p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_ssr_roughness_quality(p_arg1); } }

	// SSAO
	static void environment_set_ssao(RID p_arg1, bool p_arg2, float p_arg3, float p_arg4, float p_arg5, float p_arg6, float p_arg7, float p_arg8, float p_arg9, float p_arg10) { RendererSceneRender::environment_set_ssao(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10); }

	static bool environment_get_ssao_enabled(RID p_arg1) { return RendererSceneRender::environment_get_ssao_enabled(p_arg1); }
	static float environment_get_ssao_radius(RID p_arg1) { return RendererSceneRender::environment_get_ssao_radius(p_arg1); }
	static float environment_get_ssao_intensity(RID p_arg1) { return RendererSceneRender::environment_get_ssao_intensity(p_arg1); }
	static float environment_get_ssao_power(RID p_arg1) { return RendererSceneRender::environment_get_ssao_power(p_arg1); }
	static float environment_get_ssao_detail(RID p_arg1) { return RendererSceneRender::environment_get_ssao_detail(p_arg1); }
	static float environment_get_ssao_horizon(RID p_arg1) { return RendererSceneRender::environment_get_ssao_horizon(p_arg1); }
	static float environment_get_ssao_sharpness(RID p_arg1) { return RendererSceneRender::environment_get_ssao_sharpness(p_arg1); }
	static float environment_get_ssao_direct_light_affect(RID p_arg1) { return RendererSceneRender::environment_get_ssao_direct_light_affect(p_arg1); }
	static float environment_get_ssao_ao_channel_affect(RID p_arg1) { return RendererSceneRender::environment_get_ssao_ao_channel_affect(p_arg1); }

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_arg1, bool p_arg2, float p_arg3, int p_arg4, float p_arg5, float p_arg6) { if (data && data->scene_render) { data->scene_render->environment_set_ssao_quality(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); } }

	// SSIL
	static void environment_set_ssil(RID p_arg1, bool p_arg2, float p_arg3, float p_arg4, float p_arg5, float p_arg6) { RendererSceneRender::environment_set_ssil(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); }

	static bool environment_get_ssil_enabled(RID p_arg1) { return RendererSceneRender::environment_get_ssil_enabled(p_arg1); }
	static float environment_get_ssil_radius(RID p_arg1) { return RendererSceneRender::environment_get_ssil_radius(p_arg1); }
	static float environment_get_ssil_intensity(RID p_arg1) { return RendererSceneRender::environment_get_ssil_intensity(p_arg1); }
	static float environment_get_ssil_sharpness(RID p_arg1) { return RendererSceneRender::environment_get_ssil_sharpness(p_arg1); }
	static float environment_get_ssil_normal_rejection(RID p_arg1) { return RendererSceneRender::environment_get_ssil_normal_rejection(p_arg1); }

	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_arg1, bool p_arg2, float p_arg3, int p_arg4, float p_arg5, float p_arg6) { if (data && data->scene_render) { data->scene_render->environment_set_ssil_quality(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); } }

	// SDFGI

	static void environment_set_sdfgi(RID p_arg1, bool p_arg2, int p_arg3, float p_arg4, RSE::EnvironmentSDFGIYScale p_arg5, bool p_arg6, float p_arg7, bool p_arg8, float p_arg9, float p_arg10, float p_arg11) { RendererSceneRender::environment_set_sdfgi(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11); }

	static bool environment_get_sdfgi_enabled(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_enabled(p_arg1); }
	static int environment_get_sdfgi_cascades(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_cascades(p_arg1); }
	static float environment_get_sdfgi_min_cell_size(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_min_cell_size(p_arg1); }
	static bool environment_get_sdfgi_use_occlusion(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_use_occlusion(p_arg1); }
	static float environment_get_sdfgi_bounce_feedback(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_bounce_feedback(p_arg1); }
	static bool environment_get_sdfgi_read_sky_light(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_read_sky_light(p_arg1); }
	static float environment_get_sdfgi_energy(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_energy(p_arg1); }
	static float environment_get_sdfgi_normal_bias(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_normal_bias(p_arg1); }
	static float environment_get_sdfgi_probe_bias(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_probe_bias(p_arg1); }
	static RSE::EnvironmentSDFGIYScale environment_get_sdfgi_y_scale(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_y_scale(p_arg1); }

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_sdfgi_ray_count(p_arg1); } }
	static void environment_set_sdfgi_frames_to_converge(RSE::EnvironmentSDFGIFramesToConverge p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_sdfgi_frames_to_converge(p_arg1); } }
	static void environment_set_sdfgi_frames_to_update_light(RSE::EnvironmentSDFGIFramesToUpdateLight p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_sdfgi_frames_to_update_light(p_arg1); } }

	// Adjustment
	static void environment_set_adjustment(RID p_arg1, bool p_arg2, float p_arg3, float p_arg4, float p_arg5, bool p_arg6, RID p_arg7) { RendererSceneRender::environment_set_adjustment(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7); }

	static bool environment_get_adjustments_enabled(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_enabled(p_arg1); }
	static float environment_get_adjustments_brightness(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_brightness(p_arg1); }

	static float environment_get_adjustments_contrast(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_contrast(p_arg1); }
	static float environment_get_adjustments_saturation(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_saturation(p_arg1); }
	static bool environment_get_use_1d_color_correction(RID p_arg1) { return RendererSceneRender::environment_get_use_1d_color_correction(p_arg1); }
	static RID environment_get_color_correction(RID p_arg1) { return RendererSceneRender::environment_get_color_correction(p_arg1); }

	static Ref<Image> environment_bake_panorama(RID p_arg1, bool p_arg2, const Size2i& p_arg3) { ERR_FAIL_COND_V(!data || !data->scene_render, Ref<Image>()); return data->scene_render->environment_bake_panorama(p_arg1, p_arg2, p_arg3); }

	static void screen_space_roughness_limiter_set_active(bool p_arg1, float p_arg2, float p_arg3) { if (data && data->scene_render) { data->scene_render->screen_space_roughness_limiter_set_active(p_arg1, p_arg2, p_arg3); } }
	static void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_arg1) { if (data && data->scene_render) { data->scene_render->sub_surface_scattering_set_quality(p_arg1); } }
	static void sub_surface_scattering_set_scale(float p_arg1, float p_arg2) { if (data && data->scene_render) { data->scene_render->sub_surface_scattering_set_scale(p_arg1, p_arg2); } }

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_arg1) { if (data && data->scene_render) { data->scene_render->positional_soft_shadow_filter_set_quality(p_arg1); } }
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_arg1) { if (data && data->scene_render) { data->scene_render->directional_soft_shadow_filter_set_quality(p_arg1); } }

	static void sdfgi_set_debug_probe_select(const Vector3& p_arg1, const Vector3& p_arg2) { if (data && data->scene_render) { data->scene_render->sdfgi_set_debug_probe_select(p_arg1, p_arg2); } }

	/* Render Buffers */

	static Ref<RenderSceneBuffers> render_buffers_create() { ERR_FAIL_COND_V(!data || !data->scene_render, Ref<RenderSceneBuffers>()); return data->scene_render->render_buffers_create(); }
	static void gi_set_use_half_resolution(bool p_arg1) { if (data && data->scene_render) { data->scene_render->gi_set_use_half_resolution(p_arg1); } }

	/* Misc */
	static void set_debug_draw_mode(RSE::ViewportDebugDraw p_arg1) { if (data && data->scene_render) { data->scene_render->set_debug_draw_mode(p_arg1); } }

	static void decals_set_filter(RSE::DecalFilter p_arg1) { if (data && data->scene_render) { data->scene_render->decals_set_filter(p_arg1); } }
	static void light_projectors_set_filter(RSE::LightProjectorFilter p_arg1) { if (data && data->scene_render) { data->scene_render->light_projectors_set_filter(p_arg1); } }
	static void lightmaps_set_bicubic_filter(bool p_arg1) { if (data && data->scene_render) { data->scene_render->lightmaps_set_bicubic_filter(p_arg1); } }
	static void material_set_use_debanding(bool p_arg1) { if (data && data->scene_render) { data->scene_render->material_set_use_debanding(p_arg1); } }

	static void update();

	static bool free(RID p_rid);

	static void set_scene_render(RendererSceneRender* p_scene_render);

	static void update_visibility_notifiers();

	/* INTERPOLATION */

	static void update_interpolation_tick(bool p_process = true);
	static void update_interpolation_frame(bool p_process = true);
	static void set_physics_interpolation_enabled(bool p_enabled);

	RendererSceneCull() = delete;
	RendererSceneCull(const RendererSceneCull&) = delete;
	RendererSceneCull& operator=(const RendererSceneCull&) = delete;
	~RendererSceneCull() = delete;
};

struct Projection;
struct Transform3D;

// For testing performance improvements from the LightCuller:
// Uncomment LIGHT_CULLER_DEBUG_FLASH and it will turn the culler
// on and off every LIGHT_CULLER_DEBUG_FLASH_FREQUENCY camera prepares.
// Uncomment LIGHT_CULLER_DEBUG_LOGGING to get periodic print of the number of casters culled before / after.
// Uncomment LIGHT_CULLER_DEBUG_DIRECTIONAL_LIGHT to get periodic print of the number of casters culled for the directional light..

// #define LIGHT_CULLER_DEBUG_LOGGING
// #define LIGHT_CULLER_DEBUG_DIRECTIONAL_LIGHT
// #define LIGHT_CULLER_DEBUG_REGULAR_LIGHT
// #define LIGHT_CULLER_DEBUG_FLASH
#define LIGHT_CULLER_DEBUG_FLASH_FREQUENCY 1024
////////////////////////////////////////////////////////////////////////////////////////////////

// The code to generate the lookup table is included but commented out.
// This may be useful for debugging / regenerating the LUT in the future,
// especially if the order of planes changes.
// When this define is set, the generated lookup table will be printed to debug output.
// The generated lookup table can be copy pasted
// straight to LUT_entry_sizes and LUT_entries.
// See the referenced article for explanation.
// #define RENDERING_LIGHT_CULLER_CALCULATE_LUT

////////////////////////////////////////////////////////////////////////////////////////////////
// This define will be set automatically depending on earlier defines, you can leave this as is.
#if defined(LIGHT_CULLER_DEBUG_LOGGING) || defined(RENDERING_LIGHT_CULLER_CALCULATE_LUT)
#define RENDERING_LIGHT_CULLER_DEBUG_STRINGS
#endif

// Culls shadow casters that can't cast shadows into the camera frustum.
class RenderingLightCuller final {
public:
	RenderingLightCuller();

private:
	class LightSource {
	public:
		enum SourceType {
			ST_UNKNOWN,
			ST_DIRECTIONAL,
			ST_SPOTLIGHT,
			ST_OMNI,
			ST_AREA,
		};

		LightSource() {
			type = ST_UNKNOWN;
			angle = 0.0f;
			range = FLT_MAX;
			cascade_count = 0;
			cascade_splits[0] = cascade_splits[1] = cascade_splits[2] = 0;
			blend_splits = false;
		}

		// All in world space, culling done in world space.
		Vector3 pos;
		Vector3 dir;
		SourceType type;

		float angle; // For spotlight.
		float range;
		float width; // For area light.
		float height;
		Vector2 area_size; // For area light.

		int cascade_count;
		float cascade_splits[3]; // Max 4 cascades, which only has 3 splits.
		bool blend_splits;
	};

	// Directional lights have separate cull frustums for each cascade, so this struct is needed to specify which one to use for each cull step.
	struct CullFrustumData {
		// Functions expect this to store a frustum, so it must be ALWAYS at least Plane[6]. Undefined behavior otherwise.
		const Plane *frustum_planes = nullptr;
		// Functions expect this to store frustum corners, so it must be ALWAYS at least Vector3[8]. UB otherwise.
		const Vector3 *frustum_points = nullptr;
	};

	// Same order as godot.
	enum PlaneOrder {
		PLANE_NEAR,
		PLANE_FAR,
		PLANE_LEFT,
		PLANE_TOP,
		PLANE_RIGHT,
		PLANE_BOTTOM,
		PLANE_TOTAL,
	};

	// Same order as godot.
	enum PointOrder {
		PT_FAR_LEFT_TOP,
		PT_FAR_LEFT_BOTTOM,
		PT_FAR_RIGHT_TOP,
		PT_FAR_RIGHT_BOTTOM,
		PT_NEAR_LEFT_TOP,
		PT_NEAR_LEFT_BOTTOM,
		PT_NEAR_RIGHT_TOP,
		PT_NEAR_RIGHT_BOTTOM,
	};

	// 6 bits, 6 planes.
	enum {
		NUM_CAM_PLANES = 6,
		NUM_CAM_POINTS = 8,
		MAX_CULL_PLANES = 17,
		LUT_SIZE = 64,
	};

public:
	// Before each pass with a different camera, you must call this so the culler can pre-create
	// the camera frustum planes and corner points in world space which are used for the culling.
	bool prepare_camera(const Transform3D &p_cam_transform, const Projection &p_cam_matrix);

	// REGULAR LIGHTS (SPOT, OMNI).
	// These are prepared then used for culling one by one, single threaded.
	// prepare_regular_light() returns false if the entire light is culled (i.e. there is no intersection between the light and the view frustum).
	bool prepare_regular_light(const RendererSceneCull::Instance &p_instance) { return _prepare_light(p_instance, -1); }

	// Cull according to the regular light planes that were setup in the previous call to prepare_regular_light.
	void cull_regular_light(PagedArray<RendererSceneCull::Instance *> &r_instance_shadow_cull_result);

	// Directional lights are prepared in advance, and can be culled multithreaded chopping and changing between
	// different directional_light_id.
	void prepare_directional_light(const RendererSceneCull::Instance *p_instance, int32_t p_directional_light_id);

	// Return false if the instance is to be culled.
	bool cull_directional_light(const RendererSceneCull::InstanceBounds &p_bound, int32_t p_directional_light_id, int32_t p_cascade);

	// Can turn on and off from the engine if desired.
	void set_caster_culling_active(bool p_active) { data.caster_culling_active = p_active; }
	void set_light_culling_active(bool p_active) { data.light_culling_active = p_active; }

private:
	struct LightCullPlanes {
		void add_cull_plane(const Plane &p);
		Plane cull_planes[MAX_CULL_PLANES];
		int num_cull_planes = 0;
#ifdef LIGHT_CULLER_DEBUG_DIRECTIONAL_LIGHT
		uint32_t rejected_count = 0;
#endif
	};

	struct DirectionalCullPlanes {
		LightCullPlanes planes[4]; // One set of cull planes per cascade
	};

	bool _prepare_light(const RendererSceneCull::Instance &p_instance, int32_t p_directional_light_id = -1);

	// Avoid adding extra culling planes derived from near colinear triangles.
	// The normals derived from these will be inaccurate, and can lead to false
	// culling of objects that should be within the light volume.
	// See:
	// - issue GH-89702 "Tighter Shadow Caster Culling causes some object shadows to not render for a Frame"
	// - issue GH-89560 "Directional Shadows disappear with large Camera Z Far values at some angles"
	// - issue GH-91976 "SpotLight3D shadows exhibit flickering when moved around."
	// - PR GH-92078 which gave it the current value.
	bool _is_colinear_tri(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c) const {
		// Lengths of sides a, b and c.
		float la = (p_b - p_a).length();
		float lb = (p_c - p_b).length();
		float lc = (p_c - p_a).length();

		// Get longest side into lc.
		if (lb < la) {
			SWAP(la, lb);
		}
		if (lc < lb) {
			SWAP(lb, lc);
		}

		// Prevent divide by zero.
		if (lc > 0.001f) {
			// If the summed length of the smaller two
			// sides is close to the length of the longest side,
			// the points are colinear, and the triangle is near degenerate.
			float ld = ((la + lb) - lc) / lc;

			// ld will be close to zero for colinear tris.

			// Long frustums are made out of significantly stretched-out triangles,
			// so large threshold will produce large amounts of cullable but unculled meshes.
			// For example: 0.001 fails to cull cullable meshes with camera FOV of 70 and ortho shadows at ~50m at certain view angles.
			// 0.0001 fails to cull cullable meshes with camera FOV of 70 and ortho shadows at ~500m at certain view angles.
			// ...These apply less to cascades since they have large near planes in comparison to far planes, unlike ortho lights.
			// If you're reading this and the value for directional lights is still 0.001f,
			// that is fine as is and it only means GH-115176 didn't make it.
			return ld < 0.001f;
		}

		// Don't create planes from tiny triangles,
		// they won't be accurate.
		return true;
	}

	// Internal version uses LightSource.
	bool _add_light_camera_planes(LightCullPlanes &r_cull_planes, const LightSource &p_light_source, const CullFrustumData &p_cull_frustum);

	// Directional light gives parallel culling planes (as opposed to point lights).
	bool add_light_camera_planes_directional(LightCullPlanes &r_cull_planes, const LightSource &p_light_source, const CullFrustumData &p_cull_frustum);

	// Is the light culler active? maybe not in the editor...
	bool is_caster_culling_active() const { return data.caster_culling_active; }
	bool is_light_culling_active() const { return data.light_culling_active; }

	// Ensure result is at least a Plane[6] for the frustum input and Vector3[8] to store result before calling. If not, undefined behavior.
	bool create_frustum_points(const Plane *p_frustum_planes, Vector3 *r_result) const;

	// Do we want to log some debug output?
	bool is_logging() const { return data.debug_count == 0; }

	struct Data {
		// Camera frustum planes (world space) - order ePlane.
		Vector<Plane> frustum_planes;

		// Camera frustum corners (world space) - order ePoint.
		Vector3 frustum_points[NUM_CAM_POINTS];

		// Master can have multiple directional lights.
		// These need to store their own cull planes individually, as master
		// chops and changes between culling different lights
		// instead of doing one by one, and we don't want to prepare
		// lights multiple times per frame.
		LocalVector<DirectionalCullPlanes> directional_cull_planes;

		Transform3D camera_transform;
		Projection camera_projection;

		// Single threaded cull planes for regular lights
		// (OMNI, SPOT). These lights reuse the same set of cull plane data.
		LightCullPlanes regular_cull_planes;

#ifdef LIGHT_CULLER_DEBUG_REGULAR_LIGHT
		uint32_t regular_rejected_count = 0;
#endif
		// The whole regular light can be out of range of the view frustum, in which case all casters should be culled.
		bool out_of_range = false;

#ifdef RENDERING_LIGHT_CULLER_DEBUG_STRINGS
		static String plane_bitfield_to_string(unsigned int BF);
		// Names of the plane and point enums, useful for debugging.
		static const char *string_planes[];
		static const char *string_points[];
#endif

		// Precalculated look up table.
		static uint8_t LUT_entry_sizes[LUT_SIZE];
		static uint8_t LUT_entries[LUT_SIZE][8];

		bool caster_culling_active = true;
		bool light_culling_active = true;

		// Light culling is a basic on / off switch.
		// Caster culling only works if light culling is also on.
		bool is_active() const { return light_culling_active; }

		// Ideally a frame counter, but for ease of implementation
		// this is just incremented on each prepare_camera.
		// used to turn on and off debugging features.
		int debug_count = -1;
	} data;

	// This functionality is not required in general use (and is compiled out),
	// as the lookup table can normally be hard coded
	// (provided order of planes etc does not change).
	// It is provided for debugging / future maintenance.
#ifdef RENDERING_LIGHT_CULLER_CALCULATE_LUT
	void get_neighbouring_planes(PlaneOrder p_plane, PlaneOrder r_neigh_planes[4]) const;
	void get_corners_of_planes(PlaneOrder p_plane_a, PlaneOrder p_plane_b, PointOrder r_points[2]) const;
	void create_LUT();
	void compact_LUT_entry(uint32_t p_entry_id);
	void debug_print_LUT();
	void debug_print_LUT_as_table();
	void add_LUT(int p_plane_0, int p_plane_1, PointOrder p_pts[2]);
	void add_LUT_entry(uint32_t p_entry_id, PointOrder p_pts[2]);
	String debug_string_LUT_entry(const LocalVector<uint8_t> &p_entry, bool p_pair = false);
	String string_LUT_entry(const LocalVector<uint8_t> &p_entry);

	// Contains a list of points for each combination of plane facing directions.
	LocalVector<uint8_t> _calculated_LUT[LUT_SIZE];
#endif
};
