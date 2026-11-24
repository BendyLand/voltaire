/**************************************************************************/
/*  memory.h                                                              */
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

#include <new> // IWYU pragma: keep // `new` operators.
#include <type_traits>
#include "core/typedefs.h"

namespace Memory
{
constexpr size_t get_aligned_address(size_t p_address, size_t p_alignment)
{
	const size_t n_bytes_unaligned = p_address % p_alignment;
	return (n_bytes_unaligned == 0) ? p_address : (p_address + p_alignment - n_bytes_unaligned);
}

#if defined(__MINGW32__) && !defined(__MINGW64__)
// Note: Using hardcoded value, since the value can end up different in different compile units on
// 32-bit windows due to a compiler bug (see GH-113145)
static constexpr size_t MAX_ALIGN = 16;
static_assert(MAX_ALIGN % alignof(max_align_t) == 0);
#else
static constexpr size_t MAX_ALIGN = alignof(max_align_t);
#endif

// Alignment:  ↓ max_align_t        ↓ uint64_t          ↓ MAX_ALIGN
//             ┌─────────────────┬──┬────────────────┬──┬───────────...
//             │ uint64_t        │░░│ uint64_t       │░░│ T[]
//             │ alloc size      │░░│ element count  │░░│ data
//             └─────────────────┴──┴────────────────┴──┴───────────...
// Offset:     ↑ SIZE_OFFSET        ↑ ELEMENT_OFFSET    ↑ DATA_OFFSET

inline constexpr size_t SIZE_OFFSET = 0;
inline constexpr size_t ELEMENT_OFFSET =
	get_aligned_address(SIZE_OFFSET + sizeof(uint64_t), alignof(uint64_t));
inline constexpr size_t DATA_OFFSET =
	get_aligned_address(ELEMENT_OFFSET + sizeof(uint64_t), MAX_ALIGN);

template <bool p_ensure_zero = false> void* alloc_static(size_t p_bytes, bool p_pad_align = false);

_FORCE_INLINE_ static void* alloc_static_zeroed(size_t p_bytes, bool p_pad_align = false)
{
	return alloc_static<true>(p_bytes, p_pad_align);
}

void* realloc_static(void* p_memory, size_t p_bytes, bool p_pad_align = false);
void free_static(void* p_ptr, bool p_pad_align = false);

//	                            ↓ return value of alloc_aligned_static
//	┌─────────────────┬─────────┬─────────┬──────────────────┐
//	│ padding (up to  │ uint32_t│ void*   │ padding (up to   │
//	│ p_alignment - 1)│ offset  │ p_bytes │ p_alignment - 1) │
//	└─────────────────┴─────────┴─────────┴──────────────────┘
//
// alloc_aligned_static will allocate p_bytes + p_alignment - 1 + sizeof(uint32_t) and
// then offset the pointer until alignment is satisfied.
//
// This offset is stored before the start of the returned ptr so we can retrieve the original/real
// start of the ptr in order to free it.
//
// The rest is wasted as padding in the beginning and end of the ptr. The sum of padding at
// both start and end of the block must add exactly to p_alignment - 1.
//
// p_alignment MUST be a power of 2.
void* alloc_aligned_static(size_t p_bytes, size_t p_alignment);
void* realloc_aligned_static(
	void* p_memory, size_t p_bytes, size_t p_prev_bytes, size_t p_alignment);
// Pass the ptr returned by alloc_aligned_static to free it.
// e.g.
//	void *data = realloc_aligned_static( bytes, 16 );
//  free_aligned_static( data );
void free_aligned_static(void* p_memory);

uint64_t get_mem_available();
uint64_t get_mem_usage();
uint64_t get_mem_max_usage();
}; // namespace Memory

class DefaultAllocator
{
public:
	_FORCE_INLINE_ static void* alloc(size_t p_memory)
	{
		return Memory::alloc_static(p_memory, false);
	}

	_FORCE_INLINE_ static void free(void* p_ptr) { Memory::free_static(p_ptr, false); }
};

// Overload of `new` operator to use the `Memory::alloc_static()` function.
// The `DefaultAllocator` parameter is just a tag to select this overload.
// NOTE: do not inline `new` operators due to GCC+LTO compiler bug (see GH-119752).
void* operator new(size_t p_size, DefaultAllocator p_allocator);

// Overload of `new` operator to use a custom allocation function.
void* operator new(size_t p_size, void* (*p_allocfunc)(size_t p_size));

#if defined(_MSC_VER) && !defined(__clang__)
// When compiling with VC++ 2017, the above declarations of placement new generate many irrelevant
// warnings (C4291). The purpose of the following definitions is to muffle these warnings, not to
// provide a usable implementation of placement delete.
inline void operator delete(void* p_mem, DefaultAllocator p_allocator)
{
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}

inline void operator delete(void* p_mem, void* (*p_allocfunc)(size_t p_size))
{
	CRASH_NOW_MSG("Call to placement delete should not happen.");
}
#endif // defined(_MSC_VER) && !defined(__clang__)

#define memalloc(m_size) Memory::alloc_static(m_size)
#define memalloc_zeroed(m_size) Memory::alloc_static_zeroed(m_size)
#define memrealloc(m_mem, m_size) Memory::realloc_static(m_mem, m_size)
#define memfree(m_mem) Memory::free_static(m_mem)

template <typename T, typename Enable = void> struct memnew_result
{
	using class_name = T*;
};

template <typename T> using memnew_result_t = typename memnew_result<T>::class_name;

_ALWAYS_INLINE_ void postinitialize_handler(void*) {}

template <typename T> _ALWAYS_INLINE_ memnew_result_t<T> _post_initialize(T* p_obj)
{
	memnew_result_t<T> result{p_obj};
	postinitialize_handler(result);
	return result;
}

#define memnew(m_class) _post_initialize(::new (DefaultAllocator{}) m_class)

#define memnew_allocator(m_class, m_allocator) _post_initialize(::new (m_allocator::alloc) m_class)
#define memnew_placement(m_placement, m_class) _post_initialize(::new (m_placement) m_class)

_ALWAYS_INLINE_ bool predelete_handler(void*) { return true; }

template <typename T> void memdelete(T* p_class)

{
	if (unlikely(p_class == nullptr)) {
		return;
	}
	if (!predelete_handler(p_class)) {
		return; // doesn't want to be deleted
	}
	if constexpr (!std::is_trivially_destructible_v<T>) {
		p_class->~T();
	}

	Memory::free_static(p_class, false);
}

template <typename T, typename A> void memdelete_allocator(T* p_class)
{
	if (unlikely(p_class == nullptr)) {
		return;
	}
	if (!predelete_handler(p_class)) {
		return; // doesn't want to be deleted
	}
	if constexpr (!std::is_trivially_destructible_v<T>) {
		p_class->~T();
	}

	A::free(p_class);
}

#define memnew_arr(m_class, m_count) memnew_arr_template<m_class>(m_count)

_FORCE_INLINE_ uint64_t* _get_element_count_ptr(uint8_t* p_ptr)
{
	return (uint64_t*)(p_ptr - Memory::DATA_OFFSET + Memory::ELEMENT_OFFSET);
}

template <typename T> T* memnew_arr_template(size_t p_elements)
{
	if (p_elements == 0) {
		return nullptr;
	}
	/** overloading operator new[] cannot be done , because it may not return the real allocated
	address (it may pad the 'element count' before the actual array). Because of that, it must be
	done by hand. This is the same strategy used by std::vector, and the Vector class, so it should
	be safe.*/

	size_t len = sizeof(T) * p_elements;
	uint8_t* mem = (uint8_t*)Memory::alloc_static(len, true);
	T* failptr = nullptr; // get rid of a warning
	ERR_FAIL_NULL_V(mem, failptr);

	uint64_t* _elem_count_ptr = _get_element_count_ptr(mem);
	*(_elem_count_ptr) = p_elements;

	if constexpr (!std::is_trivially_constructible_v<T>) {
		T* elems = (T*)mem;

		/* call operator new */
		for (size_t i = 0; i < p_elements; i++) {
			::new (&elems[i]) T;
		}
	}

	return (T*)mem;
}

// Fast alternative to a loop constructor pattern.
template <typename T> _FORCE_INLINE_ void memnew_arr_placement(T* p_start, size_t p_num)
{
	if constexpr (is_zero_constructible_v<T>) {
		// Can optimize with memset.
		memset(static_cast<void*>(p_start), 0, p_num * sizeof(T));
	}
	else {
		// Need to use a for loop.
		for (size_t i = 0; i < p_num; i++) {
			memnew_placement(p_start + i, T());
		}
	}
}

// Convenient alternative to a loop copy pattern.
template <typename T> _FORCE_INLINE_ void copy_arr_placement(T* p_dst, const T* p_src, size_t p_num)
{
	if constexpr (std::is_trivially_copyable_v<T>) {
		memcpy((uint8_t*)p_dst, (uint8_t*)p_src, p_num * sizeof(T));
	}
	else {
		for (size_t i = 0; i < p_num; i++) {
			memnew_placement(p_dst + i, T(p_src[i]));
		}
	}
}

// Convenient alternative to a loop destructor pattern.
template <typename T> _FORCE_INLINE_ void destruct_arr_placement(T* p_dst, size_t p_num)
{
	if constexpr (!std::is_trivially_destructible_v<T>) {
		for (size_t i = 0; i < p_num; i++) {
			p_dst[i].~T();
		}
	}
}

/**
 * Wonders of having own array functions, you can actually check the length of
 * an allocated-with memnew_arr() array
 */

template <typename T> size_t memarr_len(const T* p_class)
{
	uint8_t* ptr = (uint8_t*)p_class;
	uint64_t* _elem_count_ptr = _get_element_count_ptr(ptr);
	return *(_elem_count_ptr);
}

template <typename T> void memdelete_arr(T* p_class)
{
	uint8_t* ptr = (uint8_t*)p_class;

	if constexpr (!std::is_trivially_destructible_v<T>) {
		uint64_t* _elem_count_ptr = _get_element_count_ptr(ptr);
		uint64_t elem_count = *(_elem_count_ptr);

		for (uint64_t i = 0; i < elem_count; i++) {
			p_class[i].~T();
		}
	}

	Memory::free_static(ptr, true);
}

struct _GlobalNil
{
	int color = 1;
	_GlobalNil* right = nullptr;
	_GlobalNil* left = nullptr;
	_GlobalNil* parent = nullptr;

	_GlobalNil();
};

struct _GlobalNilClass
{
	static _GlobalNil _nil;
};

template <typename T> class DefaultTypedAllocator
{
public:
	template <typename... Args> _FORCE_INLINE_ T* new_allocation(const Args&&... p_args)
	{
		return memnew(T(p_args...));
	}

	_FORCE_INLINE_ void delete_allocation(T* p_allocation) { memdelete(p_allocation); }
};

// --- Folded from core/templates/safe_refcount.h ---
#include "core/typedefs.h"

#ifdef DEV_ENABLED
#include "core/typedefs.h"
#endif

#include <atomic>
#include <type_traits> // IWYU pragma: keep // Used in macro.

// Design goals for these classes:
// - No automatic conversions or arithmetic operators,
//   to keep explicit the use of atomics everywhere.
// - Using acquire-release semantics, even to set the first value.
//   The first value may be set relaxedly in many cases, but adding the distinction
//   between relaxed and unrelaxed operation to the interface would make it needlessly
//   flexible. There's negligible waste in having release semantics for the initial
//   value and, as an important benefit, you can be sure the value is properly synchronized
//   even with threads that are already running.

// These are used in very specific areas of the engine where it's critical that these guarantees are
// held
#define SAFE_NUMERIC_TYPE_PUN_GUARANTEES(m_type)                                                   \
	static_assert(sizeof(SafeNumeric<m_type>) == sizeof(m_type));                                  \
	static_assert(alignof(SafeNumeric<m_type>) == alignof(m_type));                                \
	static_assert(std::is_trivially_destructible_v<std::atomic<m_type>>);
#define SAFE_FLAG_TYPE_PUN_GUARANTEES                                                              \
	static_assert(sizeof(SafeFlag) == sizeof(bool));                                               \
	static_assert(alignof(SafeFlag) == alignof(bool));

template <typename T> class SafeNumeric
{
	std::atomic<T> value;

	static_assert(std::atomic<T>::is_always_lock_free);

public:
	_ALWAYS_INLINE_ void set(T p_value) { value.store(p_value, std::memory_order_release); }

	_ALWAYS_INLINE_ T get() const { return value.load(std::memory_order_acquire); }

	_ALWAYS_INLINE_ T increment() { return value.fetch_add(1, std::memory_order_acq_rel) + 1; }

	// Returns the original value instead of the new one
	_ALWAYS_INLINE_ T postincrement() { return value.fetch_add(1, std::memory_order_acq_rel); }

	_ALWAYS_INLINE_ T decrement() { return value.fetch_sub(1, std::memory_order_acq_rel) - 1; }

	// Returns the original value instead of the new one
	_ALWAYS_INLINE_ T postdecrement() { return value.fetch_sub(1, std::memory_order_acq_rel); }

	_ALWAYS_INLINE_ T add(T p_value)
	{
		return value.fetch_add(p_value, std::memory_order_acq_rel) + p_value;
	}

	// Returns the original value instead of the new one
	_ALWAYS_INLINE_ T postadd(T p_value)
	{
		return value.fetch_add(p_value, std::memory_order_acq_rel);
	}

	_ALWAYS_INLINE_ T sub(T p_value)
	{
		return value.fetch_sub(p_value, std::memory_order_acq_rel) - p_value;
	}

	_ALWAYS_INLINE_ T bit_or(T p_value)
	{
		return value.fetch_or(p_value, std::memory_order_acq_rel);
	}

	_ALWAYS_INLINE_ T bit_and(T p_value)
	{
		return value.fetch_and(p_value, std::memory_order_acq_rel);
	}

	_ALWAYS_INLINE_ T bit_xor(T p_value)
	{
		return value.fetch_xor(p_value, std::memory_order_acq_rel);
	}

	// Returns the original value instead of the new one
	_ALWAYS_INLINE_ T postsub(T p_value)
	{
		return value.fetch_sub(p_value, std::memory_order_acq_rel);
	}

	_ALWAYS_INLINE_ T exchange_if_greater(T p_value)
	{
		while (true) {
			T tmp = value.load(std::memory_order_acquire);
			if (tmp >= p_value) {
				return tmp; // already greater, or equal
			}

			if (value.compare_exchange_weak(tmp, p_value, std::memory_order_acq_rel)) {
				return p_value;
			}
		}
	}

	_ALWAYS_INLINE_ T conditional_increment()
	{
		while (true) {
			T c = value.load(std::memory_order_acquire);
			if (c == 0) {
				return 0;
			}
			if (value.compare_exchange_weak(c, c + 1, std::memory_order_acq_rel)) {
				return c + 1;
			}
		}
	}

	_ALWAYS_INLINE_ explicit SafeNumeric(T p_value = static_cast<T>(0)) { set(p_value); }
};

class SafeFlag
{
	std::atomic_bool flag;

	static_assert(std::atomic_bool::is_always_lock_free);

public:
	_ALWAYS_INLINE_ bool is_set() const { return flag.load(std::memory_order_acquire); }

	_ALWAYS_INLINE_ void set() { flag.store(true, std::memory_order_release); }

	_ALWAYS_INLINE_ bool set_if_clear() { return !flag.exchange(true, std::memory_order_acq_rel); }

	_ALWAYS_INLINE_ void clear() { flag.store(false, std::memory_order_release); }

	_ALWAYS_INLINE_ bool clear_if_set() { return flag.exchange(false, std::memory_order_acq_rel); }

	_ALWAYS_INLINE_ void set_to(bool p_value) { flag.store(p_value, std::memory_order_release); }

	_ALWAYS_INLINE_ explicit SafeFlag(bool p_value = false) { set_to(p_value); }
};

class SafeRefCount
{
	SafeNumeric<uint32_t> count;

#ifdef DEV_ENABLED
	_ALWAYS_INLINE_ void _check_unref_safety()
	{
		// This won't catch every misuse, but it's better than nothing.
		CRASH_COND_MSG(count.get() == 0,
			"Trying to unreference a SafeRefCount which is already zero is wrong and a symptom of "
			"it being misused.\n"
			"Upon a SafeRefCount reaching zero any object whose lifetime is tied to it, as well as "
			"the ref count itself, must be destroyed.\n"
			"Moreover, to guarantee that, no multiple threads should be racing to do the final "
			"unreferencing to zero.");
	}
#endif

public:
	_ALWAYS_INLINE_ bool ref()
	{ // true on success
		return count.conditional_increment() != 0;
	}

	_ALWAYS_INLINE_ uint32_t refval()
	{ // none-zero on success
		return count.conditional_increment();
	}

	_ALWAYS_INLINE_ bool unref()
	{ // true if must be disposed of
#ifdef DEV_ENABLED
		_check_unref_safety();
#endif
		return count.decrement() == 0;
	}

	_ALWAYS_INLINE_ uint32_t unrefval()
	{ // 0 if must be disposed of
#ifdef DEV_ENABLED
		_check_unref_safety();
#endif
		return count.decrement();
	}

	_ALWAYS_INLINE_ uint32_t get() const { return count.get(); }

	_ALWAYS_INLINE_ void init(uint32_t p_value = 1) { count.set(p_value); }
};
// --- End core/templates/safe_refcount.h ---
