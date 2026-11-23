/**************************************************************************/
/*  local_vector.h                                                        */
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

#include <initializer_list>
#include <type_traits>
#include "core/error/error_macros.h"
#include "core/os/memory.h"
#include "core/string/print_string.h" // IWYU pragma: keep. `WARN_VERBOSE` macro.
#include "core/templates/sort_array.h"
#include "core/templates/vector.h"

// If tight, it grows strictly as much as needed.
// Otherwise, it grows exponentially (the default and what you want in most cases).
template <typename T, typename U = uint32_t, bool force_trivial = false, bool tight = false>
class _WARN_UNUSED_ LocalVector
{
	static_assert(
		!force_trivial, "force_trivial is no longer supported. Use resize_uninitialized instead.");

private:
	U count = 0;
	U capacity = 0;
	T* data = nullptr;

	template <bool p_init> void _resize(U p_size)
	{
		if (p_size < count) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (U i = p_size; i < count; i++) {
					data[i].~T();
				}
			}
			count = p_size;
		}
		else if (p_size > count) {
			reserve(p_size);
			if constexpr (p_init) {
				memnew_arr_placement(data + count, p_size - count);
			}
			else {
				static_assert(std::is_trivially_destructible_v<T>,
					"T must be trivially destructible to resize uninitialized");
			}
			count = p_size;
		}
	}

public:
	_FORCE_INLINE_ T* ptr() _LIFETIME_BOUND_ { return data; }

	_FORCE_INLINE_ const T* ptr() const _LIFETIME_BOUND_ { return data; }

	_FORCE_INLINE_ U size() const { return count; }

	_FORCE_INLINE_ Span<T> span() const _LIFETIME_BOUND_ { return Span(data, count); }

	_FORCE_INLINE_ operator Span<T>() const _LIFETIME_BOUND_ { return span(); }

	// Must take a copy instead of a reference (see GH-31736).
	_FORCE_INLINE_ void push_back(T p_elem)
	{
		if (unlikely(count == capacity)) {
			reserve(count + 1);
		}

		memnew_placement(&data[count++], T(std::move(p_elem)));
	}

	void remove_at(U p_index)
	{
		ERR_FAIL_UNSIGNED_INDEX(p_index, count);
		count--;
		for (U i = p_index; i < count; i++) {
			data[i] = std::move(data[i + 1]);
		}
		data[count].~T();
	}

	/// Removes the item copying the last value into the position of the one to
	/// remove. It's generally faster than `remove_at`.
	void remove_at_unordered(U p_index)
	{
		ERR_FAIL_INDEX(p_index, count);
		count--;
		if (count > p_index) {
			data[p_index] = std::move(data[count]);
		}
		data[count].~T();
	}

	_FORCE_INLINE_ bool erase(const T& p_val)
	{
		int64_t idx = find(p_val);
		if (idx >= 0) {
			remove_at(idx);
			return true;
		}
		return false;
	}

	bool erase_unordered(const T& p_val)
	{
		int64_t idx = find(p_val);
		if (idx >= 0) {
			remove_at_unordered(idx);
			return true;
		}
		return false;
	}

	U erase_multiple_unordered(const T& p_val)
	{
		U from = 0;
		U occurrences = 0;
		while (true) {
			int64_t idx = find(p_val, from);

			if (idx == -1) {
				break;
			}
			remove_at_unordered(idx);
			from = idx;
			occurrences++;
		}
		return occurrences;
	}

	void reverse()
	{
		for (U i = 0; i < count / 2; i++) {
			SWAP(data[i], data[count - i - 1]);
		}
	}
#ifndef DISABLE_DEPRECATED
	[[deprecated("Use reverse() instead")]] void invert() { reverse(); }
#endif

	_FORCE_INLINE_ void clear() { resize(0); }

	_FORCE_INLINE_ void reset()
	{
		clear();
		if (data) {
			memfree(data);
			data = nullptr;
			capacity = 0;
		}
	}

	_FORCE_INLINE_ bool is_empty() const { return count == 0; }

	_FORCE_INLINE_ U get_capacity() const { return capacity; }

	void reserve(U p_size)
	{
		if (p_size > capacity) {
			if (tight) {
				capacity = p_size;
			}
			else {
				// Try 1.5x the current capacity.
				// This ratio was chosen because it is close to the ideal growth rate of the golden
				// ratio. See https://archive.ph/Z2R8w for details.
				capacity = MAX((U)2, capacity + ((1 + capacity) >> 1));
				// If 1.5x growth isn't enough, just use the needed size exactly.
				if (p_size > capacity) {
					capacity = p_size;
				}
			}
			data = (T*)memrealloc(data, capacity * sizeof(T));
			CRASH_COND_MSG(!data, "Out of memory");
		}
		else if (p_size < count) {
			WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This is "
						 "likely a mistake.");
		}
	}

	/// Resize the vector.
	/// Elements are initialized (or not) depending on what the default C++ behavior for T is.
	/// Note: If force_trivial is set, this will behave like resize_uninitialized instead.
	void resize(U p_size)
	{
		// Don't init when trivially constructible.
		_resize<!std::is_trivially_constructible_v<T>>(p_size);
	}

	/// Resize and set new values to 0 / false / nullptr.
	_FORCE_INLINE_ void resize_initialized(U p_size) { _resize<true>(p_size); }

	/// Resize and keep memory uninitialized.
	/// This means that any newly added elements have an unknown value, and are expected to be set
	/// after the `resize_uninitialized` call. This is only available for trivially destructible
	/// types (otherwise, trivial resize might be UB).
	_FORCE_INLINE_ void resize_uninitialized(U p_size) { _resize<false>(p_size); }

	_FORCE_INLINE_ const T& operator[](U p_index) const _LIFETIME_BOUND_
	{
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}

	_FORCE_INLINE_ T& operator[](U p_index) _LIFETIME_BOUND_
	{
		CRASH_BAD_UNSIGNED_INDEX(p_index, count);
		return data[p_index];
	}

	struct Iterator
	{
		_FORCE_INLINE_ T& operator*() const { return *elem_ptr; }

		_FORCE_INLINE_ T* operator->() const { return elem_ptr; }

		_FORCE_INLINE_ Iterator& operator++()
		{
			elem_ptr++;
			return *this;
		}

		_FORCE_INLINE_ Iterator& operator--()
		{
			elem_ptr--;
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator& p_other) const
		{
			return elem_ptr == p_other.elem_ptr;
		}

		_FORCE_INLINE_ bool operator!=(const Iterator& p_other) const
		{
			return elem_ptr != p_other.elem_ptr;
		}

		Iterator(T* p_ptr) { elem_ptr = p_ptr; }

		Iterator() {}

		Iterator(const Iterator& p_it) { elem_ptr = p_it.elem_ptr; }

	private:
		T* elem_ptr = nullptr;
	};

	struct ConstIterator
	{
		_FORCE_INLINE_ const T& operator*() const { return *elem_ptr; }

		_FORCE_INLINE_ const T* operator->() const { return elem_ptr; }

		_FORCE_INLINE_ ConstIterator& operator++()
		{
			elem_ptr++;
			return *this;
		}

		_FORCE_INLINE_ ConstIterator& operator--()
		{
			elem_ptr--;
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator& p_other) const
		{
			return elem_ptr == p_other.elem_ptr;
		}

		_FORCE_INLINE_ bool operator!=(const ConstIterator& p_other) const
		{
			return elem_ptr != p_other.elem_ptr;
		}

		ConstIterator(const T* p_ptr) { elem_ptr = p_ptr; }

		ConstIterator() {}

		ConstIterator(const ConstIterator& p_it) { elem_ptr = p_it.elem_ptr; }

	private:
		const T* elem_ptr = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() _LIFETIME_BOUND_ { return Iterator(data); }

	_FORCE_INLINE_ Iterator end() _LIFETIME_BOUND_ { return Iterator(data + size()); }

	_FORCE_INLINE_ ConstIterator begin() const _LIFETIME_BOUND_ { return ConstIterator(ptr()); }

	_FORCE_INLINE_ ConstIterator end() const _LIFETIME_BOUND_
	{
		return ConstIterator(ptr() + size());
	}

	void insert(U p_pos, T p_val)
	{
		ERR_FAIL_UNSIGNED_INDEX(p_pos, count + 1);
		if (p_pos == count) {
			push_back(std::move(p_val));
		}
		else {
			resize(count + 1);
			for (U i = count - 1; i > p_pos; i--) {
				data[i] = std::move(data[i - 1]);
			}
			data[p_pos] = std::move(p_val);
		}
	}

	int64_t find(const T& p_val, int64_t p_from = 0) const
	{
		if (p_from < 0) {
			p_from = size() + p_from;
		}
		if (p_from < 0 || p_from >= size()) {
			return -1;
		}
		return span().find(p_val, p_from);
	}

	bool has(const T& p_val) const { return find(p_val) != -1; }

	template <typename C> void sort_custom()
	{
		U len = count;
		if (len == 0) {
			return;
		}

		SortArray<T, C> sorter;
		sorter.sort(data, len);
	}

	void sort() { sort_custom<Comparator<T>>(); }

	void ordered_insert(T p_val)
	{
		U i;
		for (i = 0; i < count; i++) {
			if (p_val < data[i]) {
				break;
			}
		}
		insert(i, p_val);
	}

	Vector<uint8_t> to_byte_array() const
	{ // useful to pass stuff to gpu or variant
		Vector<uint8_t> ret;
		ret.resize(count * sizeof(T));
		uint8_t* w = ret.ptrw();
		if (w) {
			memcpy(w, data, sizeof(T) * count);
		}
		return ret;
	}

	_FORCE_INLINE_ LocalVector() {}

	_FORCE_INLINE_ LocalVector(std::initializer_list<T> p_init)
	{
		reserve(p_init.size());
		for (const T& element : p_init) {
			push_back(element);
		}
	}

	_FORCE_INLINE_ explicit LocalVector(const LocalVector& p_from)
	{
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
	}

	_FORCE_INLINE_ LocalVector(LocalVector&& p_from)
	{
		data = p_from.data;
		count = p_from.count;
		capacity = p_from.capacity;

		p_from.data = nullptr;
		p_from.count = 0;
		p_from.capacity = 0;
	}

	inline void operator=(const LocalVector& p_from)
	{
		resize(p_from.size());
		for (U i = 0; i < p_from.count; i++) {
			data[i] = p_from.data[i];
		}
	}

	inline void operator=(const Vector<T>& p_from)
	{
		resize(p_from.size());
		for (U i = 0; i < count; i++) {
			data[i] = p_from[i];
		}
	}

	inline void operator=(LocalVector&& p_from)
	{
		if (unlikely(this == &p_from)) {
			return;
		}
		reset();

		data = p_from.data;
		count = p_from.count;
		capacity = p_from.capacity;

		p_from.data = nullptr;
		p_from.count = 0;
		p_from.capacity = 0;
	}

	inline void operator=(Vector<T>&& p_from)
	{
		resize(p_from.size());
		for (U i = 0; i < count; i++) {
			data[i] = std::move(p_from[i]);
		}
	}

	_FORCE_INLINE_ ~LocalVector()
	{
		if (data) {
			reset();
		}
	}
};

template <typename T, typename U = uint32_t>
using TightLocalVector = LocalVector<T, U, false, true>;

// Zero-constructing LocalVector initializes count, capacity and data to 0 and thus empty.
template <typename T, typename U, bool force_trivial, bool tight>
struct is_zero_constructible<LocalVector<T, U, force_trivial, tight>> : std::true_type
{
};

// --- Folded from core/templates/pooled_list.h ---
// Simple template to provide a pool with O(1) allocate and free.
// The freelist could alternatively be a linked list placed within the unused elements
// to use less memory, however a separate freelist is probably more cache friendly.

// NOTE : Take great care when using this with non POD types. The construction and destruction
// is done in the LocalVector, NOT as part of the pool. So requesting a new item does not guarantee
// a constructor is run, and free does not guarantee a destructor.
// You should generally handle clearing
// an item explicitly after a request, as it may contain 'leftovers'.
// This is by design for fastest use in the BVH. If you want a more general pool
// that does call constructors / destructors on request / free, this should probably be
// a separate template.

// The zero_on_first_request feature is optional and is useful for e.g. pools of handles,
// which may use a ref count which we want to be initialized to zero the first time a handle is created,
// but left alone on subsequent allocations (as will typically be incremented).

// Note that there is no function to compact the pool - this would
// invalidate any existing pool IDs held externally.
// Compaction can be done but would rely on a more complex method
// of preferentially giving out lower IDs in the freelist first.


template <typename T, typename U = uint32_t, bool force_trivial = false, bool zero_on_first_request = false>
class _WARN_UNUSED_ PooledList {
	LocalVector<T, U> list;
	LocalVector<U, U> freelist;

	// not all list members are necessarily used
	U _used_size;

public:
	PooledList() {
		_used_size = 0;
	}

	// Use with care, in most cases you should make sure to
	// free all elements first (i.e. _used_size would be zero),
	// although it could also be used without this as an optimization
	// in some cases.
	void clear() {
		list.clear();
		freelist.clear();
		_used_size = 0;
	}

	uint64_t estimate_memory_use() const {
		return ((uint64_t)list.size() * sizeof(T)) + ((uint64_t)freelist.size() * sizeof(U));
	}

	const T &operator[](U p_index) const {
		return list[p_index];
	}
	T &operator[](U p_index) {
		return list[p_index];
	}

	// To be explicit in a pool there is a distinction
	// between the number of elements that are currently
	// in use, and the number of elements that have been reserved.
	// Using size() would be vague.
	U used_size() const { return _used_size; }
	U reserved_size() const { return list.size(); }

	T *request(U &r_id) {
		_used_size++;

		if (freelist.size()) {
			// pop from freelist
			int new_size = freelist.size() - 1;
			r_id = freelist[new_size];
			freelist.resize_uninitialized(new_size);

			return &list[r_id];
		}

		r_id = list.size();
		if constexpr (force_trivial || std::is_trivially_constructible_v<T>) {
			list.resize_uninitialized(r_id + 1);
		} else {
			list.resize_initialized(r_id + 1);
		}

		static_assert((!zero_on_first_request) || (__is_pod(T)), "zero_on_first_request requires trivial type");
		if constexpr (zero_on_first_request && __is_pod(T)) {
			list[r_id] = {};
		}

		return &list[r_id];
	}
	void free(const U &p_id) {
		// should not be on free list already
		ERR_FAIL_UNSIGNED_INDEX(p_id, list.size());
		freelist.push_back(p_id);
		ERR_FAIL_COND_MSG(!_used_size, "_used_size has become out of sync, have you double freed an item?");
		_used_size--;
	}
};

// a pooled list which automatically keeps a list of the active members
template <typename T, typename U = uint32_t, bool force_trivial = false, bool zero_on_first_request = false>
class TrackedPooledList {
public:
	U pool_used_size() const { return _pool.used_size(); }
	U pool_reserved_size() const { return _pool.reserved_size(); }
	U active_size() const { return _active_list.size(); }

	// use with care, see the earlier notes in the PooledList clear()
	void clear() {
		_pool.clear();
		_active_list.clear();
		_active_map.clear();
	}

	U get_active_id(U p_index) const {
		return _active_list[p_index];
	}

	const T &get_active(U p_index) const {
		return _pool[get_active_id(p_index)];
	}

	T &get_active(U p_index) {
		return _pool[get_active_id(p_index)];
	}

	const T &operator[](U p_index) const {
		return _pool[p_index];
	}
	T &operator[](U p_index) {
		return _pool[p_index];
	}

	T *request(U &r_id) {
		T *item = _pool.request(r_id);

		// add to the active list
		U active_list_id = _active_list.size();
		_active_list.push_back(r_id);

		// expand the active map (this should be in sync with the pool list
		if (_pool.used_size() > _active_map.size()) {
			_active_map.resize_uninitialized(_pool.used_size());
		}

		// store in the active map
		_active_map[r_id] = active_list_id;

		return item;
	}

	void free(const U &p_id) {
		_pool.free(p_id);

		// remove from the active list.
		U list_id = _active_map[p_id];

		// zero the _active map to detect bugs (only in debug?)
		_active_map[p_id] = -1;

		_active_list.remove_unordered(list_id);

		// keep the replacement in sync with the correct list Id
		if (list_id < _active_list.size()) {
			// which pool id has been replaced in the active list
			U replacement_id = _active_list[list_id];

			// keep that replacements map up to date with the new position
			_active_map[replacement_id] = list_id;
		}
	}

	const LocalVector<U, U> &get_active_list() const { return _active_list; }

private:
	PooledList<T, U, force_trivial, zero_on_first_request> _pool;
	LocalVector<U, U> _active_map;
	LocalVector<U, U> _active_list;
};
