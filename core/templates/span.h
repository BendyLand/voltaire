/**************************************************************************/
/*  span.h                                                                */
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

#include "core/typedefs.h"

template <typename LHS, typename RHS>
bool are_spans_equal(const LHS *p_lhs, const RHS *p_rhs, size_t p_size) {
	if constexpr (std::is_same_v<LHS, RHS> && std::is_fundamental_v<LHS>) {
		// Optimize trivial type comparison.
		// is_trivially_equality_comparable would help, but it doesn't exist.
		// memcmp requires pointer argument to be valid even on size = 0 (C11 §7.24.1(2)).
		// Span allows ptr arguments to be invalid as long as size = 0, so only call when p_size > 0.
		return p_size == 0 || memcmp(p_lhs, p_rhs, p_size * sizeof(LHS)) == 0;
	} else {
		// Normal case: Need to iterate the array manually.
		for (size_t j = 0; j < p_size; j++) {
			if (p_lhs[j] != p_rhs[j]) {
				return false;
			}
		}

		return true;
	}
}

// Equivalent of std::span.
// Represents a view into a contiguous memory space.
// DISCLAIMER: This data type does not own the underlying buffer. DO NOT STORE IT.
//  Additionally, for the lifetime of the Span, do not resize the buffer, and do not insert or remove elements from it.
//  Failure to respect this may lead to crashes or undefined behavior.
template <typename T>
class Span {
	const T *_ptr = nullptr;
	uint64_t _len = 0;

public:
	static constexpr bool is_string = std::disjunction_v<
			std::is_same<T, char>,
			std::is_same<T, char16_t>,
			std::is_same<T, char32_t>,
			std::is_same<T, wchar_t>>;

	_FORCE_INLINE_ constexpr Span() = default;

	_FORCE_INLINE_ Span(const T *p_ptr, uint64_t p_len) :
			_ptr(p_ptr), _len(p_len) {
#ifdef DEBUG_ENABLED
		// TODO In c++20, make this check run only in non-consteval, and make this constructor constexpr.
		if (_ptr == nullptr && _len > 0) {
			ERR_PRINT("Internal bug, please report: Span was created from nullptr with size > 0. Recovering by using size = 0.");
			_len = 0;
		}
#endif
	}

	// Allows creating Span directly from C arrays and string literals.
	template <size_t N>
	_FORCE_INLINE_ constexpr Span(const T (&p_array)[N]) :
			_ptr(p_array), _len(N) {
		if constexpr (is_string) {
			// Cut off the \0 terminator implicitly added to string literals.
			if (N > 0 && p_array[N - 1] == '\0') {
				_len--;
			}
		}
	}

	_FORCE_INLINE_ constexpr uint64_t size() const { return _len; }
	_FORCE_INLINE_ constexpr bool is_empty() const { return _len == 0; }

	_FORCE_INLINE_ constexpr const T *ptr() const { return _ptr; }

	// NOTE: Span subscripts sanity check the bounds to avoid undefined behavior.
	//       This is slower than direct buffer access and can prevent autovectorization.
	//       If the bounds are known, use ptr() subscript instead.
	_FORCE_INLINE_ constexpr const T &operator[](uint64_t p_idx) const {
		CRASH_COND(p_idx >= _len);
		return _ptr[p_idx];
	}

	_FORCE_INLINE_ constexpr const T *begin() const { return _ptr; }
	_FORCE_INLINE_ constexpr const T *end() const { return _ptr + _len; }

	template <typename T1>
	_FORCE_INLINE_ constexpr Span<T1> reinterpret() const {
		return Span<T1>(reinterpret_cast<const T1 *>(_ptr), _len * sizeof(T) / sizeof(T1));
	}

	// Algorithms.
	constexpr int64_t find(const T &p_val, uint64_t p_from = 0) const;
	template <typename T1 = T>
	constexpr int64_t find_sequence(const Span<T1> &p_span, uint64_t p_from = 0) const;
	constexpr int64_t rfind(const T &p_val, uint64_t p_from) const;
	_FORCE_INLINE_ constexpr int64_t rfind(const T &p_val) const { return rfind(p_val, size() - 1); }
	template <typename T1 = T>
	constexpr int64_t rfind_sequence(const Span<T1> &p_span, uint64_t p_from) const;
	template <typename T1 = T>
	_FORCE_INLINE_ constexpr int64_t rfind_sequence(const Span<T1> &p_span) const { return rfind_sequence(p_span, size() - p_span.size()); }
	constexpr uint64_t count(const T &p_val) const;
	/// Find the index of the given value using binary search.
	/// Note: Assumes that elements in the span are sorted. Otherwise, use find() instead.
	template <typename Comparator = Comparator<T>>
	constexpr uint64_t bisect(const T &p_value, bool p_before, Comparator p_compare = Comparator()) const;

	/// The caller is responsible to ensure size() > 0.
	constexpr T max() const;
};

template <typename T>
constexpr int64_t Span<T>::find(const T &p_val, uint64_t p_from) const {
	for (uint64_t i = p_from; i < size(); i++) {
		if (ptr()[i] == p_val) {
			return i;
		}
	}
	return -1;
}

template <typename T>
template <typename T1>
constexpr int64_t Span<T>::find_sequence(const Span<T1> &p_span, uint64_t p_from) const {
	for (uint64_t i = p_from; i <= size() - p_span.size(); i++) {
		if (are_spans_equal(ptr() + i, p_span.ptr(), p_span.size())) {
			return i;
		}
	}

	return -1;
}

template <typename T>
constexpr int64_t Span<T>::rfind(const T &p_val, uint64_t p_from) const {
	DEV_ASSERT(p_from < size());
	for (int64_t i = p_from; i >= 0; i--) {
		if (ptr()[i] == p_val) {
			return i;
		}
	}
	return -1;
}

template <typename T>
template <typename T1>
constexpr int64_t Span<T>::rfind_sequence(const Span<T1> &p_span, uint64_t p_from) const {
	DEV_ASSERT(p_from + p_span.size() <= size());
	for (int64_t i = p_from; i >= 0; i--) {
		if (are_spans_equal(ptr() + i, p_span.ptr(), p_span.size())) {
			return i;
		}
	}

	return -1;
}

template <typename T>
constexpr uint64_t Span<T>::count(const T &p_val) const {
	uint64_t amount = 0;
	for (uint64_t i = 0; i < size(); i++) {
		if (ptr()[i] == p_val) {
			amount++;
		}
	}
	return amount;
}

template <typename T>
template <typename Comparator>
constexpr uint64_t Span<T>::bisect(const T &p_value, bool p_before, Comparator p_compare) const {
	uint64_t lo = 0;
	uint64_t hi = size();
	if (p_before) {
		while (lo < hi) {
			const uint64_t mid = (lo + hi) / 2;
			if (p_compare(ptr()[mid], p_value)) {
				lo = mid + 1;
			} else {
				hi = mid;
			}
		}
	} else {
		while (lo < hi) {
			const uint64_t mid = (lo + hi) / 2;
			if (p_compare(p_value, ptr()[mid])) {
				hi = mid;
			} else {
				lo = mid + 1;
			}
		}
	}
	return lo;
}

template <typename T>
constexpr T Span<T>::max() const {
	DEV_ASSERT(size() > 0);
	T max_val = _ptr[0];
	for (size_t i = 1; i < _len; ++i) {
		if (_ptr[i] > max_val) {
			max_val = _ptr[i];
		}
	}
	return max_val;
}

template <typename LHS, typename RHS>
bool operator==(const Span<LHS> &p_lhs, const Span<RHS> &p_rhs) {
	return p_lhs.size() == p_rhs.size() && are_spans_equal(p_lhs.ptr(), p_rhs.ptr(), p_lhs.size());
}

template <typename LHS, typename RHS>
_FORCE_INLINE_ bool operator!=(const Span<LHS> &p_lhs, const Span<RHS> &p_rhs) {
	return !(p_lhs == p_rhs);
}

// Zero-constructing Span initializes _ptr and _len to 0 (and thus empty).
template <typename T>
struct is_zero_constructible<Span<T>> : std::true_type {};

// --- Folded from core/templates/fixed_vector.h ---
#include "core/os/memory.h"

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Warray-bounds")

/**
 * A high performance Vector of fixed capacity.
 * Especially useful if you need to create an array on the stack, to
 *  prevent dynamic allocations (especially in bottleneck code).
 *
 * Choose CAPACITY such that it is enough for all elements that could be added through all branches.
 *
 */
template <class T, uint32_t CAPACITY>
class _WARN_UNUSED_ FixedVector {
	// This declaration allows us to access other FixedVector's private members.
	template <class T_, uint32_t CAPACITY_>
	friend class FixedVector;

	uint32_t _size = 0;
	alignas(T) uint8_t _data[CAPACITY * sizeof(T)];

	constexpr static uint32_t DATA_PADDING = MAX(alignof(T), alignof(uint32_t)) - alignof(uint32_t);

public:
	_FORCE_INLINE_ constexpr FixedVector() = default;

	constexpr FixedVector(std::initializer_list<T> p_init) {
		ERR_FAIL_COND(p_init.size() > CAPACITY);
		for (const T &element : p_init) {
			memnew_placement(ptr() + _size++, T(element));
		}
	}

	constexpr FixedVector(const FixedVector &p_from) {
		if constexpr (std::is_trivially_copyable_v<T>) {
			// Copy size and all provided elements at once.
			memcpy((void *)&_size, (void *)&p_from._size, sizeof(_size) + DATA_PADDING + p_from.size() * sizeof(T));
		} else {
			for (const T &element : p_from) {
				memnew_placement(ptr() + _size++, T(element));
			}
		}
	}

	constexpr FixedVector(FixedVector &&p_from) {
		// Copy size and all provided elements at once.
		// Note: Assumes trivial relocatability.
		memcpy((void *)&_size, (void *)&p_from._size, sizeof(_size) + DATA_PADDING + p_from.size() * sizeof(T));
		p_from._size = 0;
	}

	constexpr FixedVector &operator=(const FixedVector &p_from) {
		if constexpr (std::is_trivially_copyable_v<T>) {
			// Copy size and all provided elements at once.
			memcpy((void *)&_size, (void *)&p_from._size, sizeof(_size) + DATA_PADDING + p_from.size() * sizeof(T));
		} else {
			// Destruct extraneous elements.
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (uint32_t i = p_from.size(); i < _size; i++) {
					ptr()[i].~T();
				}
			}

			_size = 0; // Loop-assign the rest.
			for (const T &element : p_from) {
				ptr()[_size++] = element;
			}
		}
		return *this;
	}

	constexpr FixedVector &operator=(FixedVector &&p_from) {
		// Destruct extraneous elements.
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (uint32_t i = p_from.size(); i < _size; i++) {
				ptr()[i].~T();
			}
		}

		// Relocate elements (and size) into our buffer.
		memcpy((void *)&_size, (void *)&p_from._size, sizeof(_size) + DATA_PADDING + p_from.size() * sizeof(T));
		p_from._size = 0;

		return *this;
	}

	~FixedVector() {
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (uint32_t i = 0; i < _size; i++) {
				ptr()[i].~T();
			}
		}
	}

	_FORCE_INLINE_ constexpr T *ptr() _LIFETIME_BOUND_ { return (T *)(_data); }
	_FORCE_INLINE_ constexpr const T *ptr() const _LIFETIME_BOUND_ { return (const T *)(_data); }

	_FORCE_INLINE_ constexpr operator Span<T>() const _LIFETIME_BOUND_ { return Span<T>(ptr(), size()); }
	_FORCE_INLINE_ constexpr Span<T> span() const _LIFETIME_BOUND_ { return operator Span<T>(); }

	_FORCE_INLINE_ constexpr uint32_t size() const { return _size; }
	_FORCE_INLINE_ constexpr bool is_empty() const { return !_size; }
	_FORCE_INLINE_ constexpr bool is_full() const { return _size == CAPACITY; }
	_FORCE_INLINE_ constexpr uint32_t capacity() const { return CAPACITY; }

	_FORCE_INLINE_ constexpr void clear() { resize_initialized(0); }

	/// Changes the size of the vector.
	/// If p_size > size(), constructs new elements.
	/// If p_size < size(), destructs new elements.
	constexpr Error resize_initialized(uint32_t p_size) {
		if (p_size > _size) {
			ERR_FAIL_COND_V(p_size > CAPACITY, ERR_OUT_OF_MEMORY);
			memnew_arr_placement(ptr() + _size, p_size - _size);
		} else if (p_size < _size) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				for (uint32_t i = p_size; i < _size; i++) {
					ptr()[i].~T();
				}
			}
		}

		_size = p_size;
		return OK;
	}

	/// Changes the size of the vector.
	/// The initializer of new elements is skipped, making this function faster than resize_initialized.
	/// The caller is required to initialize the new values.
	constexpr Error resize_uninitialized(uint32_t p_size) {
		static_assert(std::is_trivially_destructible_v<T>, "resize_uninitialized is unsafe to call if T is not trivially destructible.");
		ERR_FAIL_COND_V(p_size > CAPACITY, ERR_OUT_OF_MEMORY);
		_size = p_size;
		return OK;
	}

	constexpr void push_back(const T &p_val) {
		ERR_FAIL_COND(_size >= CAPACITY);
		memnew_placement(ptr() + _size, T(p_val));
		_size++;
	}

	constexpr void push_back(T &&p_val) {
		ERR_FAIL_COND(_size >= CAPACITY);
		memnew_placement(ptr() + _size, T(std::move(p_val)));
		_size++;
	}

	constexpr void pop_back() {
		ERR_FAIL_COND(_size == 0);
		_size--;
		ptr()[_size].~T();
	}

	// NOTE: Subscripts sanity check the bounds to avoid undefined behavior.
	//       This is slower than direct buffer access and can prevent autovectorization.
	//       If the bounds are known, use ptr() subscript instead.
	constexpr const T &operator[](uint32_t p_index) const _LIFETIME_BOUND_ {
		CRASH_COND(p_index >= _size);
		return ptr()[p_index];
	}

	constexpr T &operator[](uint32_t p_index) _LIFETIME_BOUND_ {
		CRASH_COND(p_index >= _size);
		return ptr()[p_index];
	}

	_FORCE_INLINE_ constexpr T *begin() _LIFETIME_BOUND_ { return ptr(); }
	_FORCE_INLINE_ constexpr T *end() _LIFETIME_BOUND_ { return ptr() + _size; }

	_FORCE_INLINE_ constexpr const T *begin() const _LIFETIME_BOUND_ { return ptr(); }
	_FORCE_INLINE_ constexpr const T *end() const _LIFETIME_BOUND_ { return ptr() + _size; }
};

VLTR_GCC_WARNING_POP

// --- Folded from core/string/print_string.h ---
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>


class String;

extern void (*_print_func)(String);

using PrintHandlerFunc = void (*)(
	void* p_userdata, const String& p_string, bool p_error, bool p_rich);

struct PrintHandlerList
{
	PrintHandlerFunc printfunc = nullptr;
	void* userdata = nullptr;
	PrintHandlerList* next = nullptr;
};

void add_print_handler(PrintHandlerList* p_handler);
void remove_print_handler(const PrintHandlerList* p_handler);

extern void __print_line(const String& p_string);
extern void __print_line_rich(const String& p_string);
extern void print_raw(const String& p_string);
extern void print_error(const String& p_string);
extern bool is_print_verbose_enabled();

namespace dbg
{

// Forward declaration so recursion can see it. The default depth lives here only.
template <typename T> std::string to_string(const T& p_val, int p_depth = 2);

// ---------------------------------------------------------------------------
// Detection traits
// ---------------------------------------------------------------------------

// Detection: has .position and .size (Rect2, Rect2i)
template <typename T, typename = void> struct has_pos_size : std::false_type
{
};

template <typename T>
struct has_pos_size<T,
	std::void_t<decltype(std::declval<T>().position), decltype(std::declval<T>().size)>>
	: std::true_type
{
};

template <typename T> inline constexpr bool has_pos_size_v = has_pos_size<T>::value;

// Detection: has .x and .y (Vector2, Vector2i, Point2, Size2)
template <typename T, typename = void> struct has_xy : std::false_type
{
};

template <typename T>
struct has_xy<T, std::void_t<decltype(std::declval<T>().x), decltype(std::declval<T>().y)>>
	: std::true_type
{
};

template <typename T> inline constexpr bool has_xy_v = has_xy<T>::value;

// Detection: supports std::ostream << val
template <typename T, typename = void> struct is_streamable : std::false_type
{
};

template <typename T>
struct is_streamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
	: std::true_type
{
};

template <typename T> inline constexpr bool is_streamable_v = is_streamable<T>::value;

// ---------------------------------------------------------------------------
// Aggregate field walking (values only, no reflection needed)
// ---------------------------------------------------------------------------

constexpr std::size_t k_max_fields = 10;

// Converts to anything, used to probe how many initializers a struct accepts.
struct any_t
{
	template <typename U> constexpr operator U() const noexcept;
};

template <typename T, typename Seq, typename = void> struct brace_ok : std::false_type
{
};

template <typename T, std::size_t... I>
struct brace_ok<T, std::index_sequence<I...>, std::void_t<decltype(T{(void(I), any_t{})...})>>
	: std::true_type
{
};

template <typename T, std::size_t N = 0> constexpr std::size_t count_fields()
{
	if constexpr (N < 16 && brace_ok<T, std::make_index_sequence<N + 1>>::value) {
		return count_fields<T, N + 1>();
	}
	else {
		return N;
	}
}

template <typename T> constexpr bool dumpable_check()
{
	if constexpr (std::is_aggregate_v<T> && !std::is_array_v<T> && !std::is_union_v<T>) {
		constexpr std::size_t n = count_fields<T>();
		return n >= 1 && n <= k_max_fields;
	}
	else {
		return false;
	}
}

// Only complete types qualify (incomplete ones like VkCommandBuffer_T SFINAE out).
template <typename T, typename = void> struct is_dumpable : std::false_type
{
};

template <typename T>
struct is_dumpable<T, std::void_t<decltype(sizeof(T))>> : std::bool_constant<dumpable_check<T>()>
{
};

template <typename T> inline constexpr bool is_dumpable_v = is_dumpable<T>::value;

template <typename T, typename F> void for_each_field(const T& p_val, F&& p_fn)
{
	constexpr std::size_t N = count_fields<T>();

	if constexpr (N == 1) {
		const auto& [a0] = p_val;
		p_fn(a0);
	}
	else if constexpr (N == 2) {
		const auto& [a0, a1] = p_val;
		p_fn(a0); p_fn(a1);
	}
	else if constexpr (N == 3) {
		const auto& [a0, a1, a2] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2);
	}
	else if constexpr (N == 4) {
		const auto& [a0, a1, a2, a3] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3);
	}
	else if constexpr (N == 5) {
		const auto& [a0, a1, a2, a3, a4] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3); p_fn(a4);
	}
	else if constexpr (N == 6) {
		const auto& [a0, a1, a2, a3, a4, a5] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3); p_fn(a4); p_fn(a5);
	}
	else if constexpr (N == 7) {
		const auto& [a0, a1, a2, a3, a4, a5, a6] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3); p_fn(a4); p_fn(a5); p_fn(a6);
	}
	else if constexpr (N == 8) {
		const auto& [a0, a1, a2, a3, a4, a5, a6, a7] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3); p_fn(a4); p_fn(a5); p_fn(a6); p_fn(a7);
	}
	else if constexpr (N == 9) {
		const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3); p_fn(a4); p_fn(a5); p_fn(a6); p_fn(a7);
		p_fn(a8);
	}
	else if constexpr (N == 10) {
		const auto& [a0, a1, a2, a3, a4, a5, a6, a7, a8, a9] = p_val;
		p_fn(a0); p_fn(a1); p_fn(a2); p_fn(a3); p_fn(a4); p_fn(a5); p_fn(a6); p_fn(a7);
		p_fn(a8); p_fn(a9);
	}
}

// ---------------------------------------------------------------------------
// Value formatting
// ---------------------------------------------------------------------------

template <typename T> std::string to_string(const T& p_val, int p_depth)
{
	using Decayed = std::decay_t<T>;

	if constexpr (std::is_null_pointer_v<Decayed>) {
		return "nullptr";
	}
	else if constexpr (std::is_same_v<Decayed, bool>) {
		return p_val ? "true" : "false";
	}
	else if constexpr (std::is_same_v<Decayed, char>) {
		return std::string("'") + p_val + "'";
	}
	else if constexpr (std::is_same_v<Decayed, const char*> || std::is_same_v<Decayed, char*>) {
		return p_val ? std::string("\"") + p_val + "\"" : "nullptr";
	}
	else if constexpr (std::is_same_v<Decayed, std::string>) {
		return "\"" + p_val + "\"";
	}
	else if constexpr (std::is_same_v<Decayed, std::string_view>) {
		return "\"" + std::string(p_val) + "\"";
	}
	else if constexpr (std::is_pointer_v<Decayed>) {
		if (!p_val) {
			return "nullptr";
		}
		std::ostringstream oss;
		oss << static_cast<const void*>(p_val);

		using Pointee = std::remove_cv_t<std::remove_pointer_t<Decayed>>;
		if constexpr (is_dumpable_v<Pointee>) {
			if (p_depth > 0) {
				oss << " -> " << to_string(*p_val, p_depth - 1);
			}
		}
		return oss.str();
	}
	else if constexpr (has_pos_size_v<Decayed>) {
		std::ostringstream oss;
		oss << "[pos: (" << p_val.position.x << ", " << p_val.position.y << "), "
			<< "size: (" << p_val.size.x << ", " << p_val.size.y << ")]";
		return oss.str();
	}
	else if constexpr (has_xy_v<Decayed>) {
		std::ostringstream oss;
		oss << "(" << p_val.x << ", " << p_val.y << ")";
		return oss.str();
	}
	else if constexpr (std::is_arithmetic_v<Decayed>) {
		std::ostringstream oss;
		if constexpr (sizeof(Decayed) == 1) {
			// int8_t / uint8_t: print the number, not the raw character.
			oss << static_cast<int>(p_val);
		}
		else {
			oss << p_val;
		}
		return oss.str();
	}
	else if constexpr (is_streamable_v<Decayed>) {
		std::ostringstream oss;
		oss << p_val;
		return oss.str();
	}
	else if constexpr (is_dumpable_v<Decayed>) {
		std::string out = "{";
		bool first = true;
		for_each_field(p_val, [&](const auto& p_field) {
			if (!first) {
				out += ", ";
			}
			first = false;
			out += to_string(p_field, p_depth);
		});
		out += "}";
		return out;
	}
	else {
		// Raw POD fallback
		std::ostringstream oss;
		oss << "[raw " << sizeof(Decayed) << "B: ";
		const auto* bytes = reinterpret_cast<const uint8_t*>(&p_val);
		for (size_t i = 0; i < sizeof(Decayed); ++i) {
			oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
				<< static_cast<uint32_t>(bytes[i]) << " ";
		}
		oss << "]";
		return oss.str();
	}
}

// ---------------------------------------------------------------------------
// DUMP support
// ---------------------------------------------------------------------------

inline void print_unpacked(std::string_view) { std::cout << std::endl; }

template <typename First, typename... Rest>
void print_unpacked(std::string_view p_names, const First& p_first, const Rest&... p_rest)
{
	size_t comma_pos = p_names.find(',');
	std::string_view raw_name = p_names.substr(0, comma_pos);

	size_t start = raw_name.find_first_not_of(" \t");
	size_t end = raw_name.find_last_not_of(" \t");
	std::string_view trimmed_name =
		(start == std::string_view::npos) ? "" : raw_name.substr(start, end - start + 1);

	std::cout << trimmed_name << " = " << to_string(p_first);

	if (comma_pos != std::string_view::npos) {
		std::cout << " | ";
		std::string_view remaining = p_names.substr(comma_pos + 1);
		print_unpacked(remaining, p_rest...);
	}
	else {
		std::cout << std::endl;
	}
}

} // namespace dbg

#define DUMP(...) dbg::print_unpacked(#__VA_ARGS__, __VA_ARGS__)

#define print_verbose(m_text)                                                                      \
	{                                                                                              \
		if (is_print_verbose_enabled()) {                                                          \
			__print_line(m_text);                                                                  \
		}                                                                                          \
	}
// --- End core/string/print_string.h ---

// --- Folded from core/templates/sort_array.h ---
#include "core/typedefs.h"

#define ERR_BAD_COMPARE(cond) \
	if (unlikely(cond)) { \
		ERR_PRINT("bad comparison function; sorting will be broken"); \
		break; \
	}

#ifdef DEBUG_ENABLED
#define SORT_ARRAY_VALIDATE_ENABLED true
#else
#define SORT_ARRAY_VALIDATE_ENABLED false
#endif

template <typename T, typename Comparator = Comparator<T>, bool Validate = SORT_ARRAY_VALIDATE_ENABLED>
class SortArray {
	enum {
		INTROSORT_THRESHOLD = 16
	};

public:
	Comparator compare;

	inline int64_t median_of_3_index(const T *p_ptr, int64_t p_a_index, int64_t p_b_index, int64_t p_c_index) const {
		const T &a = p_ptr[p_a_index];
		const T &b = p_ptr[p_b_index];
		const T &c = p_ptr[p_c_index];
		if (compare(a, b)) {
			if (compare(b, c)) {
				return p_b_index;
			} else if (compare(a, c)) {
				return p_c_index;
			} else {
				return p_a_index;
			}
		} else if (compare(a, c)) {
			return p_a_index;
		} else if (compare(b, c)) {
			return p_c_index;
		} else {
			return p_b_index;
		}
	}

	inline int64_t bitlog(int64_t p_n) const {
		int64_t k;
		for (k = 0; p_n != 1; p_n >>= 1) {
			++k;
		}
		return k;
	}

	/* Heap / Heapsort functions */

	inline void push_heap(int64_t p_first, int64_t p_hole_idx, int64_t p_top_index, T p_value, T *p_array) const {
		int64_t parent = (p_hole_idx - 1) / 2;
		while (p_hole_idx > p_top_index && compare(p_array[p_first + parent], p_value)) {
			p_array[p_first + p_hole_idx] = p_array[p_first + parent];
			p_hole_idx = parent;
			parent = (p_hole_idx - 1) / 2;
		}
		p_array[p_first + p_hole_idx] = p_value;
	}

	inline void pop_heap(int64_t p_first, int64_t p_last, int64_t p_result, T p_value, T *p_array) const {
		p_array[p_result] = p_array[p_first];
		adjust_heap(p_first, 0, p_last - p_first, p_value, p_array);
	}
	inline void pop_heap(int64_t p_first, int64_t p_last, T *p_array) const {
		pop_heap(p_first, p_last - 1, p_last - 1, p_array[p_last - 1], p_array);
	}

	inline void adjust_heap(int64_t p_first, int64_t p_hole_idx, int64_t p_len, T p_value, T *p_array) const {
		int64_t top_index = p_hole_idx;
		int64_t second_child = 2 * p_hole_idx + 2;

		while (second_child < p_len) {
			if (compare(p_array[p_first + second_child], p_array[p_first + (second_child - 1)])) {
				second_child--;
			}

			p_array[p_first + p_hole_idx] = p_array[p_first + second_child];
			p_hole_idx = second_child;
			second_child = 2 * (second_child + 1);
		}

		if (second_child == p_len) {
			p_array[p_first + p_hole_idx] = p_array[p_first + (second_child - 1)];
			p_hole_idx = second_child - 1;
		}
		push_heap(p_first, p_hole_idx, top_index, p_value, p_array);
	}

	inline void sort_heap(int64_t p_first, int64_t p_last, T *p_array) const {
		while (p_last - p_first > 1) {
			pop_heap(p_first, p_last--, p_array);
		}
	}

	inline void make_heap(int64_t p_first, int64_t p_last, T *p_array) const {
		if (p_last - p_first < 2) {
			return;
		}
		int64_t len = p_last - p_first;
		int64_t parent = (len - 2) / 2;

		while (true) {
			adjust_heap(p_first, parent, len, p_array[p_first + parent], p_array);
			if (parent == 0) {
				return;
			}
			parent--;
		}
	}

	inline void partial_sort(int64_t p_first, int64_t p_last, int64_t p_middle, T *p_array) const {
		make_heap(p_first, p_middle, p_array);
		for (int64_t i = p_middle; i < p_last; i++) {
			if (compare(p_array[i], p_array[p_first])) {
				pop_heap(p_first, p_middle, i, p_array[i], p_array);
			}
		}
		sort_heap(p_first, p_middle, p_array);
	}

	inline void partial_select(int64_t p_first, int64_t p_last, int64_t p_middle, T *p_array) const {
		make_heap(p_first, p_middle, p_array);
		for (int64_t i = p_middle; i < p_last; i++) {
			if (compare(p_array[i], p_array[p_first])) {
				pop_heap(p_first, p_middle, i, p_array[i], p_array);
			}
		}
	}

	inline int64_t partitioner(int64_t p_first, int64_t p_last, int64_t p_pivot, T *p_array) const {
		const int64_t unmodified_first = p_first;
		const int64_t unmodified_last = p_last;
		const T *pivot_element_location = &p_array[p_pivot];

		while (true) {
			while (p_first != p_pivot && compare(p_array[p_first], *pivot_element_location)) {
				if constexpr (Validate) {
					ERR_BAD_COMPARE(p_first == unmodified_last - 1);
				}
				p_first++;
			}
			p_last--;
			while (p_last != p_pivot && compare(*pivot_element_location, p_array[p_last])) {
				if constexpr (Validate) {
					ERR_BAD_COMPARE(p_last == unmodified_first);
				}
				p_last--;
			}

			if (p_first >= p_last) {
				return p_first;
			}

			if (pivot_element_location == &p_array[p_first]) {
				pivot_element_location = &p_array[p_last];
			} else if (pivot_element_location == &p_array[p_last]) {
				pivot_element_location = &p_array[p_first];
			}
			SWAP(p_array[p_first], p_array[p_last]);
			p_first++;
		}
	}

	inline void introsort(int64_t p_first, int64_t p_last, T *p_array, int64_t p_max_depth) const {
		while (p_last - p_first > INTROSORT_THRESHOLD) {
			if (p_max_depth == 0) {
				partial_sort(p_first, p_last, p_last, p_array);
				return;
			}

			p_max_depth--;

			int64_t cut = partitioner(
					p_first,
					p_last,
					median_of_3_index(p_array, p_first, p_first + (p_last - p_first) / 2, p_last - 1),
					p_array);

			introsort(cut, p_last, p_array, p_max_depth);
			p_last = cut;
		}
	}

	inline void introselect(int64_t p_first, int64_t p_nth, int64_t p_last, T *p_array, int64_t p_max_depth) const {
		while (p_last - p_first > 3) {
			if (p_max_depth == 0) {
				partial_select(p_first, p_nth + 1, p_last, p_array);
				SWAP(p_first, p_nth);
				return;
			}

			p_max_depth--;

			int64_t cut = partitioner(
					p_first,
					p_last,
					median_of_3_index(p_array, p_first, p_first + (p_last - p_first) / 2, p_last - 1),
					p_array);

			if (cut <= p_nth) {
				p_first = cut;
			} else {
				p_last = cut;
			}
		}

		insertion_sort(p_first, p_last, p_array);
	}

	inline void unguarded_linear_insert(int64_t p_last, T p_value, T *p_array) const {
		int64_t next = p_last - 1;
		while (compare(p_value, p_array[next])) {
			if constexpr (Validate) {
				ERR_BAD_COMPARE(next == 0);
			}
			p_array[p_last] = p_array[next];
			p_last = next;
			next--;
		}
		p_array[p_last] = p_value;
	}

	inline void linear_insert(int64_t p_first, int64_t p_last, T *p_array) const {
		T val = p_array[p_last];
		if (compare(val, p_array[p_first])) {
			for (int64_t i = p_last; i > p_first; i--) {
				p_array[i] = p_array[i - 1];
			}

			p_array[p_first] = val;
		} else {
			unguarded_linear_insert(p_last, val, p_array);
		}
	}

	inline void insertion_sort(int64_t p_first, int64_t p_last, T *p_array) const {
		if (p_first == p_last) {
			return;
		}
		for (int64_t i = p_first + 1; i != p_last; i++) {
			linear_insert(p_first, i, p_array);
		}
	}

	inline void unguarded_insertion_sort(int64_t p_first, int64_t p_last, T *p_array) const {
		for (int64_t i = p_first; i != p_last; i++) {
			unguarded_linear_insert(i, p_array[i], p_array);
		}
	}

	inline void final_insertion_sort(int64_t p_first, int64_t p_last, T *p_array) const {
		if (p_last - p_first > INTROSORT_THRESHOLD) {
			insertion_sort(p_first, p_first + INTROSORT_THRESHOLD, p_array);
			unguarded_insertion_sort(p_first + INTROSORT_THRESHOLD, p_last, p_array);
		} else {
			insertion_sort(p_first, p_last, p_array);
		}
	}

	inline void sort_range(int64_t p_first, int64_t p_last, T *p_array) const {
		if (p_first != p_last) {
			introsort(p_first, p_last, p_array, bitlog(p_last - p_first) * 2);
			final_insertion_sort(p_first, p_last, p_array);
		}
	}

	inline void sort(T *p_array, int64_t p_len) const {
		sort_range(0, p_len, p_array);
	}

	inline void nth_element(int64_t p_first, int64_t p_last, int64_t p_nth, T *p_array) const {
		if (p_first == p_last || p_nth == p_last) {
			return;
		}
		introselect(p_first, p_nth, p_last, p_array, bitlog(p_last - p_first) * 2);
	}
};
// --- End core/templates/sort_array.h ---
