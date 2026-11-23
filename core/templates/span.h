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

#include "core/error/error_macros.h"
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
