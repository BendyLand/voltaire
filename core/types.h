/**************************************************************************/
/*  types.h                                                               */
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

#ifndef TYPES_H
#define TYPES_H

// Core definitions & memory management
#include "core/os/memory.h"
#include "core/typedefs.h"
#include "core/os/mutex.h"
// --- Inlined/Folded from core/templates/ref.h ---
#ifndef REF_H
#define REF_H

#include <atomic>
#include <type_traits>
#include <utility>
#include "core/os/memory.h"
#include "core/typedefs.h"

class RefCounted
{
	mutable std::atomic<uint32_t> _ref_count{0};
	mutable std::atomic<uint32_t> _ref_count_init{0};

public:
	_FORCE_INLINE_ bool reference() const
	{
		uint32_t count = _ref_count.fetch_add(1, std::memory_order_relaxed);
		return count > 0 || _ref_count_init.exchange(1, std::memory_order_acq_rel) == 0;
	}

	_FORCE_INLINE_ bool unreference() const
	{
		if (_ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			memdelete(const_cast<RefCounted*>(this));
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ uint32_t get_reference_count() const
	{
		return _ref_count.load(std::memory_order_relaxed);
	}

	RefCounted() = default;
	virtual ~RefCounted() = default;

	RefCounted(const RefCounted&) = delete;
	RefCounted& operator=(const RefCounted&) = delete;
};

template <typename T> class Ref
{
	T* data = nullptr;

	_FORCE_INLINE_ void ref(T* p_ptr)
	{
		if (p_ptr == data) {
			return;
		}
		unref();
		if (p_ptr && p_ptr->reference()) {
			data = p_ptr;
		}
	}

public:
	// --- Constructors ---
	_FORCE_INLINE_ Ref() = default;

	_FORCE_INLINE_ Ref(std::nullptr_t) {}

	_FORCE_INLINE_ Ref(T* p_ptr)
	{
		if (p_ptr) {
			ref(p_ptr);
		}
	}

	_FORCE_INLINE_ Ref(const Ref& p_from) { ref(p_from.data); }

	_FORCE_INLINE_ Ref(Ref&& p_from) noexcept : data(p_from.data) { p_from.data = nullptr; }

	// Universal converting copy constructor (Upcasts & Downcasts)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> &&
									(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>)>>
	_FORCE_INLINE_ Ref(const Ref<U>& p_from)
	{
		if constexpr (std::is_base_of_v<T, U>) {
			ref(p_from.ptr());
		}
		else if constexpr (std::is_base_of_v<U, T>) {
			ref(dynamic_cast<T*>(p_from.ptr()));
		}
	}

	// Converting move constructor (Upcast only)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ Ref(Ref<U>&& p_from) noexcept : data(p_from.data)
	{
		p_from.data = nullptr;
	}

	_FORCE_INLINE_ ~Ref() { unref(); }

	// --- Lifecycle & Mutators ---
	_FORCE_INLINE_ void unref()
	{
		if (data) {
			data->unreference();
			data = nullptr;
		}
	}

	template <typename... Args> _FORCE_INLINE_ void instantiate(Args&&... p_args)
	{
		ref(memnew(T(std::forward<Args>(p_args)...)));
	}

	// --- Assignment Operators ---
	_FORCE_INLINE_ Ref& operator=(const Ref& p_from)
	{
		if (this != &p_from) {
			ref(p_from.data);
		}
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(Ref&& p_from) noexcept
	{
		if (this != &p_from) {
			unref();
			data = p_from.data;
			p_from.data = nullptr;
		}
		return *this;
	}

	// Universal converting copy assignment (Upcasts & Downcasts)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> &&
									(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>)>>
	_FORCE_INLINE_ Ref& operator=(const Ref<U>& p_from)
	{
		if constexpr (std::is_base_of_v<T, U>) {
			ref(p_from.ptr());
		}
		else if constexpr (std::is_base_of_v<U, T>) {
			ref(dynamic_cast<T*>(p_from.ptr()));
		}
		return *this;
	}

	// Converting move assignment (Upcast only)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ Ref& operator=(Ref<U>&& p_from) noexcept
	{
		unref();
		data = p_from.data;
		p_from.data = nullptr;
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(T* p_ptr)
	{
		ref(p_ptr);
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(std::nullptr_t)
	{
		unref();
		return *this;
	}

	_FORCE_INLINE_ T* ptr() const { return data; }

	_FORCE_INLINE_ T* operator->() const { return data; }

	_FORCE_INLINE_ T& operator*() const { return *data; }

	_FORCE_INLINE_ explicit operator bool() const { return data != nullptr; }

	_FORCE_INLINE_ bool is_valid() const { return data != nullptr; }

	_FORCE_INLINE_ bool is_null() const { return data == nullptr; }

	_FORCE_INLINE_ bool operator==(const Ref& p_r) const { return data == p_r.data; }

	_FORCE_INLINE_ bool operator!=(const Ref& p_r) const { return data != p_r.data; }

	_FORCE_INLINE_ bool operator==(const T* p_ptr) const { return data == p_ptr; }

	_FORCE_INLINE_ bool operator!=(const T* p_ptr) const { return data != p_ptr; }

	_FORCE_INLINE_ bool operator<(const Ref& p_r) const { return data < p_r.data; }

	template <typename U> friend class Ref;
};

template <typename T, typename... Args> _FORCE_INLINE_ Ref<T> memnew_ref(Args&&... p_args)
{
	return Ref<T>(memnew(T(std::forward<Args>(p_args)...)));
}

#endif // REF_H

// Math primitives
#include "core/math/transform_3d.h"
#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/transform_3d.h"
// --- Folded from core/math/face3.h ---
#include "core/math/aabb.h"
#include "core/math/plane.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"

struct [[nodiscard]] Face3 {
	enum Side {
		SIDE_OVER,
		SIDE_UNDER,
		SIDE_SPANNING,
		SIDE_COPLANAR
	};

	Vector3 vertex[3];

	/**
	 * @param p_plane plane used to split the face
	 * @param p_res array of at least 3 faces, amount used in function return
	 * @param p_is_point_over array of at least 3 booleans, determining which face is over the plane, amount used in function return
	 * @return amount of faces generated by the split, either 0 (means no split possible), 2 or 3
	 */
	int split_by_plane(const Plane &p_plane, Face3 *p_res, bool *p_is_point_over) const;

	Plane get_plane(ClockDirection p_dir = CLOCKWISE) const;
	Vector3 get_random_point_inside() const;

	bool is_degenerate() const;
	real_t get_area() const;

	Vector3 get_closest_point_to(const Vector3 &p_point) const;

	bool intersects_ray(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection = nullptr) const;
	bool intersects_segment(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection = nullptr) const;

	void get_support(const Vector3 &p_normal, const Transform3D &p_transform, Vector3 *p_vertices, int *p_count, int p_max) const;
	void project_range(const Vector3 &p_normal, const Transform3D &p_transform, real_t &r_min, real_t &r_max) const;

	AABB get_aabb() const {
		AABB aabb(vertex[0], Vector3());
		aabb.expand_to(vertex[1]);
		aabb.expand_to(vertex[2]);
		return aabb;
	}

	bool intersects_aabb(const AABB &p_aabb) const;
	_FORCE_INLINE_ bool intersects_aabb2(const AABB &p_aabb) const;
	explicit operator String() const;

	Face3() = default;
	constexpr Face3(const Vector3 &p_v1, const Vector3 &p_v2, const Vector3 &p_v3) :
			vertex{ p_v1, p_v2, p_v3 } {}
};

bool Face3::intersects_aabb2(const AABB &p_aabb) const {
	Vector3 perp = (vertex[0] - vertex[2]).cross(vertex[0] - vertex[1]);

	Vector3 half_extents = p_aabb.size * 0.5f;
	Vector3 ofs = p_aabb.position + half_extents;

	Vector3 sup = Vector3(
			(perp.x > 0) ? -half_extents.x : half_extents.x,
			(perp.y > 0) ? -half_extents.y : half_extents.y,
			(perp.z > 0) ? -half_extents.z : half_extents.z);

	real_t d = perp.dot(vertex[0]);
	real_t dist_a = perp.dot(ofs + sup) - d;
	real_t dist_b = perp.dot(ofs - sup) - d;

	if (dist_a * dist_b > 0) {
		return false; //does not intersect the plane
	}

#define TEST_AXIS(m_ax) \
	{ \
		real_t aabb_min = p_aabb.position.m_ax; \
		real_t aabb_max = p_aabb.position.m_ax + p_aabb.size.m_ax; \
		real_t tri_min, tri_max; \
		for (int i = 0; i < 3; i++) { \
			if (i == 0 || vertex[i].m_ax > tri_max) \
				tri_max = vertex[i].m_ax; \
			if (i == 0 || vertex[i].m_ax < tri_min) \
				tri_min = vertex[i].m_ax; \
		} \
\
		if (tri_max < aabb_min || aabb_max < tri_min) \
			return false; \
	}

	TEST_AXIS(x);
	TEST_AXIS(y);
	TEST_AXIS(z);

#undef TEST_AXIS

	const Vector3 edge_norms[3] = {
		vertex[0] - vertex[1],
		vertex[1] - vertex[2],
		vertex[2] - vertex[0],
	};

	for (int i = 0; i < 12; i++) {
		Vector3 from, to;
		switch (i) {
			case 0: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z);
			} break;
			case 1: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z);
			} break;
			case 2: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 3: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 4: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);
			} break;
			case 5: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);
			} break;
			case 6: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 7: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);

			} break;
			case 8: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);

			} break;
			case 9: {
				from = Vector3(p_aabb.position.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);

			} break;
			case 10: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z);

			} break;
			case 11: {
				from = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y, p_aabb.position.z + p_aabb.size.z);
				to = Vector3(p_aabb.position.x + p_aabb.size.x, p_aabb.position.y + p_aabb.size.y, p_aabb.position.z + p_aabb.size.z);

			} break;
		}

		Vector3 e1 = from - to;
		for (int j = 0; j < 3; j++) {
			Vector3 e2 = edge_norms[j];

			Vector3 axis = vec3_cross(e1, e2);

			if (axis.length_squared() < 0.0001f) {
				continue; // coplanar
			}
			//axis.normalize();

			Vector3 sup2 = Vector3(
					(axis.x > 0) ? -half_extents.x : half_extents.x,
					(axis.y > 0) ? -half_extents.y : half_extents.y,
					(axis.z > 0) ? -half_extents.z : half_extents.z);

			real_t maxB = axis.dot(ofs + sup2);
			real_t minB = axis.dot(ofs - sup2);
			if (minB > maxB) {
				SWAP(maxB, minB);
			}

			real_t minT = 1e20, maxT = -1e20;
			for (int k = 0; k < 3; k++) {
				real_t vert_d = axis.dot(vertex[k]);

				if (vert_d > maxT) {
					maxT = vert_d;
				}

				if (vert_d < minT) {
					minT = vert_d;
				}
			}

			if (maxB < minT || maxT < minB) {
				return false;
			}
		}
	}
	return true;
}

template <>
struct is_zero_constructible<Face3> : std::true_type {};
// --- End core/math/face3.h ---
#include "core/math/rect2.h"
#include "core/math/transform_2d.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/math/vector3.h"
#include "core/math/projection.h"

// Strings & Paths
// --- Inlined/Folded from core/string/node_path.h ---
#include <climits>
#include "core/string/ustring.h"

// Represents a path to a node or property in a hierarchy of nodes
// Note that NodePath is (effectively) const: If you hold a NodePath,
// you can expect it to remain unchanged, even if you make copies of
// it. This is achieved through copy-on-write (CoW).
class [[nodiscard]] _WARN_UNUSED_ NodePath
{
	struct Data
	{
		SafeRefCount refcount;
		Vector<String> path;
		Vector<String> subpath;
		String concatenated_path;
		String concatenated_subpath;
		bool absolute;
		mutable bool hash_cache_valid;
		mutable uint32_t hash_cache;
	};

	mutable Data* data = nullptr;
	void unref();

	void _update_hash_cache() const;

	// Copies the underlying data.
	// Every non-const function must call this before starting to mutate data.
	void _copy_on_write();

public:
	bool is_absolute() const;
	int get_name_count() const;
	String get_name(int p_idx) const;
	int get_subname_count() const;
	String get_subname(int p_idx) const;
	int get_total_name_count() const;
	Vector<String> get_names() const;
	Vector<String> get_subnames() const;
	String get_concatenated_names() const;
	String get_concatenated_subnames() const;
	NodePath slice(int p_begin, int p_end = INT_MAX) const;

	NodePath rel_path_to(const NodePath& p_np) const;
	NodePath get_as_property_path() const;

	void prepend_period();

	_FORCE_INLINE_ uint32_t hash() const
	{
		if (!data) {
			return 0;
		}
		if (!data->hash_cache_valid) {
			_update_hash_cache();
		}
		return data->hash_cache;
	}

	explicit operator String() const;
	bool is_empty() const;

	bool operator==(const NodePath& p_path) const;
	bool operator!=(const NodePath& p_path) const;
	void operator=(const NodePath& p_path);

	void simplify();
	NodePath simplified() const;

	NodePath(const Vector<String>& p_path, bool p_absolute);
	NodePath(
		const Vector<String>& p_path, const Vector<String>& p_subpath, bool p_absolute);
	NodePath(const NodePath& p_path);
	NodePath(const String& p_path);

	NodePath() {}

	~NodePath();
};

// Zero-constructing NodePath initializes data to nullptr (and thus empty).
template <> struct is_zero_constructible<NodePath> : std::true_type
{
};
#include "core/string/ustring.h"

// Core Identifiers & Handles
#include "core/templates/rid.h"

// Native Container Templates
#include "core/templates/vector.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
// --- Inlined/Folded from core/templates/ring_buffer.h ---
#include "core/templates/local_vector.h"

template <typename T> class _WARN_UNUSED_ RingBuffer
{
	LocalVector<T> data;
	int read_pos = 0;
	int write_pos = 0;
	int size_mask;

	inline int inc(int& p_var, int p_size) const
	{
		int ret = p_var;
		p_var += p_size;
		p_var = p_var & size_mask;
		return ret;
	}

public:
	T read()
	{
		ERR_FAIL_COND_V(data_left() < 1, T());
		return data.ptr()[inc(read_pos, 1)];
	}

	int read(T* p_buf, int p_size, bool p_advance = true)
	{
		int left = data_left();
		p_size = MIN(left, p_size);
		int pos = read_pos;
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			const T* read = data.ptr();
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = read[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		if (p_advance) {
			inc(read_pos, p_size);
		}
		return p_size;
	}

	int copy(T* p_buf, int p_offset, int p_size) const
	{
		int left = data_left();
		if ((p_offset + p_size) > left) {
			p_size = left - p_offset;
			if (p_size <= 0) {
				return 0;
			}
		}
		p_size = MIN(left, p_size);
		int pos = read_pos;
		inc(pos, p_offset);
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = data[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		return p_size;
	}

	int find(const T& p_value, int p_offset, int p_max_size) const
	{
		int left = data_left();
		if ((p_offset + p_max_size) > left) {
			p_max_size = left - p_offset;
			if (p_max_size <= 0) {
				return 0;
			}
		}
		p_max_size = MIN(left, p_max_size);
		int pos = read_pos;
		inc(pos, p_offset);
		int to_read = p_max_size;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			for (int i = 0; i < total; i++) {
				if (data[pos + i] == p_value) {
					return i + (p_max_size - to_read);
				}
			}
			to_read -= total;
			pos = 0;
		}
		return -1;
	}

	inline int advance_read(int p_n)
	{
		p_n = MIN(p_n, data_left());
		inc(read_pos, p_n);
		return p_n;
	}

	inline int decrease_write(int p_n)
	{
		p_n = MIN(p_n, data_left());
		inc(write_pos, size_mask + 1 - p_n);
		return p_n;
	}

	Error write(const T& p_v)
	{
		ERR_FAIL_COND_V(space_left() < 1, FAILED);
		data[inc(write_pos, 1)] = p_v;
		return OK;
	}

	int write(const T* p_buf, int p_size)
	{
		int left = space_left();
		p_size = MIN(left, p_size);

		int pos = write_pos;
		int to_write = p_size;
		int src = 0;
		while (to_write) {
			int end = pos + to_write;
			end = MIN(end, size());
			int total = end - pos;

			for (int i = 0; i < total; i++) {
				data[pos + i] = p_buf[src++];
			}
			to_write -= total;
			pos = 0;
		}

		inc(write_pos, p_size);
		return p_size;
	}

	inline int space_left() const
	{
		int left = read_pos - write_pos;
		if (left < 0) {
			return size() + left - 1;
		}
		if (left == 0) {
			return size() - 1;
		}
		return left - 1;
	}

	inline int data_left() const { return size() - space_left() - 1; }

	inline int size() const { return data.size(); }

	inline void clear()
	{
		read_pos = 0;
		write_pos = 0;
	}

	void resize(int p_power)
	{
		int old_size = size();
		int new_size = 1 << p_power;
		int mask = new_size - 1;
		data.resize(int64_t(1) << int64_t(p_power));
		if (old_size < new_size && read_pos > write_pos) {
			for (int i = 0; i < write_pos; i++) {
				data[(old_size + i) & mask] = data[i];
			}
			write_pos = (old_size + write_pos) & mask;
		}
		else {
			read_pos = read_pos & mask;
			write_pos = write_pos & mask;
		}

		size_mask = mask;
	}

	RingBuffer(int p_power = 0) { resize(p_power); }
};
#include "core/templates/vector.h"

// Packed Arrays
typedef Vector<uint8_t> PackedByteArray;
typedef Vector<int32_t> PackedInt32Array;
typedef Vector<int64_t> PackedInt64Array;
typedef Vector<float> PackedFloat32Array;
typedef Vector<double> PackedFloat64Array;
typedef Vector<String> PackedStringArray;
typedef Vector<Vector2> PackedVector2Array;
typedef Vector<Vector3> PackedVector3Array;
typedef Vector<Color> PackedColorArray;
typedef Vector<Vector4> PackedVector4Array;

template <typename T>
struct HashMapHasherDefaultImpl<Ref<T>> {
	static _FORCE_INLINE_ uint32_t hash(const Ref<T> &p_ref) {
		return hash_one_uint64(reinterpret_cast<uint64_t>(p_ref.ptr()));
	}
};

#include "core/math/expression.h"
#include "thirdparty/openxr/include/openxr/openxr.h"

#endif // TYPES_H
