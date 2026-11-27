/**************************************************************************/
/*  ustring.h                                                             */
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

#include <type_traits>
#include <utility>
#include "core/typedefs.h"

static constexpr char hex_char_table_upper[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
static constexpr char hex_char_table_lower[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

struct CharRange {
	char32_t start;
	char32_t end;
};

extern const CharRange xid_start[];
extern const int xid_start_size;
extern const CharRange xid_continue[];
extern const int xid_continue_size;
extern const CharRange uppercase_letter[];
extern const int uppercase_letter_size;
extern const CharRange lowercase_letter[];
extern const int lowercase_letter_size;
extern const CharRange unicode_letter[];
extern const int unicode_letter_size;

#define BSEARCH_CHAR_RANGE(m_array, m_size) \
	int low = 0; \
	int high = m_size - 1; \
	int middle = (low + high) / 2; \
\
	while (low <= high) { \
		if (p_char < m_array[middle].start) { \
			high = middle - 1; \
		} else if (p_char > m_array[middle].end) { \
			low = middle + 1; \
		} else { \
			return true; \
		} \
\
		middle = (low + high) / 2; \
	} \
\
	return false

inline bool is_unicode_identifier_start(char32_t p_char) {
	BSEARCH_CHAR_RANGE(xid_start, xid_start_size);
}

inline bool is_unicode_identifier_continue(char32_t p_char) {
	BSEARCH_CHAR_RANGE(xid_continue, xid_continue_size);
}

inline bool is_unicode_upper_case(char32_t p_char) {
	BSEARCH_CHAR_RANGE(uppercase_letter, uppercase_letter_size);
}

inline bool is_unicode_lower_case(char32_t p_char) {
	BSEARCH_CHAR_RANGE(lowercase_letter, lowercase_letter_size);
}

inline bool is_unicode_letter(char32_t p_char) {
	BSEARCH_CHAR_RANGE(unicode_letter, unicode_letter_size);
}

#undef BSEARCH_CHAR_RANGE

constexpr bool is_ascii_upper_case(char32_t p_char) {
	return (p_char >= 'A' && p_char <= 'Z');
}

constexpr bool is_ascii_lower_case(char32_t p_char) {
	return (p_char >= 'a' && p_char <= 'z');
}

constexpr bool is_digit(char32_t p_char) {
	return (p_char >= '0' && p_char <= '9');
}

constexpr bool is_hex_digit(char32_t p_char) {
	return (is_digit(p_char) || (p_char >= 'a' && p_char <= 'f') || (p_char >= 'A' && p_char <= 'F'));
}

constexpr bool is_binary_digit(char32_t p_char) {
	return (p_char == '0' || p_char == '1');
}

constexpr bool is_ascii_alphabet_char(char32_t p_char) {
	return (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z');
}

constexpr bool is_ascii_alphanumeric_char(char32_t p_char) {
	return (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z') || (p_char >= '0' && p_char <= '9');
}

constexpr bool is_ascii_identifier_char(char32_t p_char) {
	return (p_char >= 'a' && p_char <= 'z') || (p_char >= 'A' && p_char <= 'Z') || (p_char >= '0' && p_char <= '9') || p_char == '_';
}

constexpr bool is_symbol(char32_t p_char) {
	return p_char != '_' && ((p_char >= '!' && p_char <= '/') || (p_char >= ':' && p_char <= '@') || (p_char >= '[' && p_char <= '`') || (p_char >= '{' && p_char <= '~') || p_char == '\t' || p_char == ' ');
}

constexpr bool is_control(char32_t p_char) {
	return (p_char <= 0x001f) || (p_char >= 0x007f && p_char <= 0x009f);
}

constexpr bool is_whitespace(char32_t p_char) {
	return (p_char == ' ') || (p_char == 0x00a0) || (p_char == 0x1680) || (p_char >= 0x2000 && p_char <= 0x200b) || (p_char == 0x202f) || (p_char == 0x205f) || (p_char == 0x3000) || (p_char == 0x2028) || (p_char == 0x2029) || (p_char >= 0x0009 && p_char <= 0x000d) || (p_char == 0x0085);
}

constexpr bool is_linebreak(char32_t p_char) {
	return (p_char >= 0x000a && p_char <= 0x000d) || (p_char == 0x0085) || (p_char == 0x2028) || (p_char == 0x2029);
}

constexpr bool is_punct(char32_t p_char) {
	return (p_char >= ' ' && p_char <= '/') || (p_char >= ':' && p_char <= '@') || (p_char >= '[' && p_char <= '^') || (p_char == '`') || (p_char >= '{' && p_char <= '~') || (p_char >= 0x2000 && p_char <= 0x206f) || (p_char >= 0x3000 && p_char <= 0x303f);
}

constexpr bool is_underscore(char32_t p_char) {
	return (p_char == '_');
}

constexpr bool is_hyphen(char32_t p_char) {
	return (p_char == '-') || (p_char == 0x2010) || (p_char == 0x2011);
}

/**
 * @class Vector
 * Vector container. Simple copy-on-write container.
 *
 * LocalVector is an alternative available for internal use when COW is not
 * required.
 */

#include <initializer_list>
#include "core/os/memory.h"

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

#include <initializer_list>
#include <type_traits>

#ifdef ASAN_ENABLED
#include <sanitizer/asan_interface.h>
#endif

static_assert(std::is_trivially_destructible_v<std::atomic<uint64_t>>);

// Silences false-positive warnings.
VLTR_GCC_WARNING_PUSH
VLTR_GCC_WARNING_IGNORE("-Wplacement-new") // Silence a false positive warning (see GH-52119).
VLTR_GCC_WARNING_IGNORE("-Wmaybe-uninitialized") // False positive raised when using constexpr.
VLTR_GCC_WARNING_IGNORE("-Warray-bounds")
VLTR_GCC_WARNING_IGNORE("-Wrestrict")
VLTR_GCC_PRAGMA(GCC diagnostic warning "-Wstringop-overflow=0") // Can't "ignore" this for some reason.
#ifdef WINDOWS_ENABLED
VLTR_GCC_PRAGMA(GCC diagnostic warning "-Wdangling-pointer=0") // Can't "ignore" this for some reason.
#endif

template <typename T>
class CowData {
public:
	typedef int64_t Size;
	typedef uint64_t USize;
	static constexpr USize MAX_INT = INT64_MAX;

private:
	// Alignment:  ↓ max_align_t           ↓ USize          ↓ USize            ↓ MAX_ALIGN
	//             ┌────────────────────┬──┬───────────────┬──┬─────────────┬──┬───────────...
	//             │ SafeNumeric<USize> │░░│ USize         │░░│ USize       │░░│ T[]
	//             │ ref. count         │░░│ data capacity │░░│ data size   │░░│ data
	//             └────────────────────┴──┴───────────────┴──┴─────────────┴──┴───────────...
	// Offset:     ↑ REF_COUNT_OFFSET      ↑ CAPACITY_OFFSET  ↑ SIZE_OFFSET    ↑ DATA_OFFSET

	static constexpr size_t REF_COUNT_OFFSET = 0;
	static constexpr size_t CAPACITY_OFFSET = Memory::get_aligned_address(REF_COUNT_OFFSET + sizeof(SafeNumeric<USize>), alignof(USize));
	static constexpr size_t SIZE_OFFSET = Memory::get_aligned_address(CAPACITY_OFFSET + sizeof(USize), alignof(USize));
	static constexpr size_t DATA_OFFSET = Memory::get_aligned_address(SIZE_OFFSET + sizeof(USize), Memory::MAX_ALIGN);

	mutable T *_ptr = nullptr;

	// internal helpers

	static constexpr _FORCE_INLINE_ USize grow_capacity(USize p_previous_capacity) {
		// 1.5x the given size.
		// This ratio was chosen because it is close to the ideal growth rate of the golden ratio.
		// See https://archive.ph/Z2R8w for details.
		return MAX((USize)2, p_previous_capacity + ((1 + p_previous_capacity) >> 1));
	}

	static constexpr _FORCE_INLINE_ USize next_capacity(USize p_previous_capacity, USize p_size) {
		if (p_previous_capacity < p_size) {
			return MAX(grow_capacity(p_previous_capacity), p_size);
		}
		return p_previous_capacity;
	}

	static constexpr _FORCE_INLINE_ USize smaller_capacity(USize p_previous_capacity, USize p_size) {
		if (p_size < p_previous_capacity >> 2) {
			return grow_capacity(p_size);
		}
		return p_previous_capacity;
	}

	static _FORCE_INLINE_ T *_get_data_ptr(uint8_t *p_ptr) {
		return (T *)(p_ptr + DATA_OFFSET);
	}

	/// Note: Assumes _ptr != nullptr.
	_FORCE_INLINE_ SafeNumeric<USize> *_get_refcount() const {
		return (SafeNumeric<USize> *)((uint8_t *)_ptr - DATA_OFFSET + REF_COUNT_OFFSET);
	}

	/// Note: Assumes _ptr != nullptr.
	_FORCE_INLINE_ USize *_get_size() const {
		return (USize *)((uint8_t *)_ptr - DATA_OFFSET + SIZE_OFFSET);
	}

	/// Note: Assumes _ptr != nullptr.
	_FORCE_INLINE_ USize *_get_capacity() const {
		return (USize *)((uint8_t *)_ptr - DATA_OFFSET + CAPACITY_OFFSET);
	}

	// Decrements the reference count. Deallocates the backing buffer if needed.
	// After this function, _ptr is guaranteed to be NULL.
	void _unref();
	void _ref(const CowData *p_from);
	void _ref(const CowData &p_from);

	/// Allocates a backing array of the given capacity. The reference count is initialized to 1, size to 0.
	/// It is the responsibility of the caller to:
	/// - Ensure _ptr == nullptr
	/// - Ensure p_capacity > 0
	Error _alloc_exact(USize p_capacity);

	/// Re-allocates the backing array to the given capacity.
	/// It is the responsibility of the caller to:
	/// - Ensure we are the only owner of the backing array
	/// - Ensure p_capacity > 0
	Error _realloc_exact(USize p_capacity);

	/// Create a new buffer and copies over elements from the old buffer.
	/// Elements are inserted first from the start, then a gap is left uninitialized, and then elements are inserted from the back.
	/// It is the responsibility of the caller to:
	/// - Construct elements in the gap.
	/// - Ensure size() >= p_size_from_start and size() >= p_size_from_back.
	/// - Ensure p_capacity is enough to hold all elements.
	[[nodiscard]] Error _copy_to_new_buffer_exact(USize p_capacity, USize p_size_from_start, USize p_gap, USize p_size_from_back);

	/// Ensure we are the only owners of the backing buffer.
	[[nodiscard]] Error _copy_on_write();

public:
	void operator=(const CowData<T> &p_from) { _ref(p_from); }
	void operator=(CowData<T> &&p_from) {
		if (_ptr == p_from._ptr) {
			return;
		}

		_unref();
		_ptr = p_from._ptr;
		p_from._ptr = nullptr;
	}

	_FORCE_INLINE_ T *ptrw() _LIFETIME_BOUND_ {
		// If forking fails, we can only crash.
		CRASH_COND(_copy_on_write());
		return _ptr;
	}

	_FORCE_INLINE_ const T *ptr() const _LIFETIME_BOUND_ {
		return _ptr;
	}

	_FORCE_INLINE_ Size size() const { return !_ptr ? 0 : *_get_size(); }
	_FORCE_INLINE_ USize capacity() const { return !_ptr ? 0 : *_get_capacity(); }
	_FORCE_INLINE_ USize refcount() const { return !_ptr ? 0 : *_get_refcount(); }

	_FORCE_INLINE_ void clear() { _unref(); }
	_FORCE_INLINE_ bool is_empty() const { return size() == 0; }

	_FORCE_INLINE_ void set(Size p_index, const T &p_elem) {
		ERR_FAIL_INDEX(p_index, size());
		// TODO Returning the error would be more appropriate.
		CRASH_COND(_copy_on_write());
		_ptr[p_index] = p_elem;
	}

	_FORCE_INLINE_ T &get_m(Size p_index) _LIFETIME_BOUND_ {
		CRASH_BAD_INDEX(p_index, size());
		// If we fail to fork, all we can do is crash,
		// since the caller may write incorrectly to the unforked array.
		CRASH_COND(_copy_on_write());
		return _ptr[p_index];
	}

	_FORCE_INLINE_ const T &get(Size p_index) const _LIFETIME_BOUND_ {
		CRASH_BAD_INDEX(p_index, size());

		return _ptr[p_index];
	}

	template <bool p_init = false>
	Error resize(Size p_size);

	template <bool p_exact = false>
	Error reserve(USize p_min_capacity);
	_FORCE_INLINE_ Error reserve_exact(USize p_capacity) {
		return reserve<true>(p_capacity);
	}

	_FORCE_INLINE_ void remove_at(Size p_index);

	Error insert(Size p_pos, T &&p_val);
	Error push_back(T &&p_val);

	_FORCE_INLINE_ operator Span<T>() const _LIFETIME_BOUND_ { return Span<T>(ptr(), size()); }
	_FORCE_INLINE_ Span<T> span() const _LIFETIME_BOUND_ { return operator Span<T>(); }

	_FORCE_INLINE_ CowData() {}
	_FORCE_INLINE_ ~CowData() { _unref(); }
	_FORCE_INLINE_ CowData(std::initializer_list<T> p_init);
	_FORCE_INLINE_ explicit CowData(Span<T> p_span);
	_FORCE_INLINE_ CowData(const CowData<T> &p_from) { _ref(p_from); }
	_FORCE_INLINE_ CowData(CowData<T> &&p_from) {
		_ptr = p_from._ptr;
		p_from._ptr = nullptr;
	}
};

template <typename T>
void CowData<T>::_unref() {
	if (!_ptr) {
		return;
	}

	if (_get_refcount()->decrement() > 0) {
		// Data is still in use elsewhere.
		_ptr = nullptr;
		return;
	}
	// We had the only reference; destroy the data.

	// First, invalidate our own reference.
	// NOTE: It is required to do so immediately because it must not be observable outside of this
	//       function after refcount has already been reduced to 0.
	// WARNING: It must be done before calling the destructors, because one of them may otherwise
	//          observe it through a reference to us. In this case, it may try to access the buffer,
	//          which is illegal after some of the elements in it have already been destructed, and
	//          may lead to a segmentation fault.
	USize current_size = size();
	T *prev_ptr = _ptr;
	_ptr = nullptr;

#ifdef ASAN_ENABLED
	// Access during destruction is illegal in C++, and results in undefined behavior.
	// In address sanitizer builds, we can poison ourselves (_ptr) to catch this.
	__asan_poison_memory_region(this, sizeof(CowData));
#endif

	destruct_arr_placement(prev_ptr, current_size);

	// Free Memory.
	Memory::free_static((uint8_t *)prev_ptr - DATA_OFFSET, false);

#ifdef ASAN_ENABLED
	__asan_unpoison_memory_region(this, sizeof(CowData));
#elif defined(DEBUG_ENABLED)
	// In a non-asan build, the best we can do is catch if elements were added during destruction.
	ERR_FAIL_COND_MSG(_ptr != nullptr, "Internal bug, please report: CowData was modified during destruction.");
#endif
}

template <typename T>
void CowData<T>::remove_at(Size p_index) {
	const Size prev_size = size();
	ERR_FAIL_INDEX(p_index, prev_size);

	if (prev_size == 1) {
		// Removing the only element.
		_unref();
		return;
	}

	const USize new_size = prev_size - 1;

	if (_get_refcount()->get() == 1) {
		// We're the only owner; remove in-place.

		// Destruct the element, then relocate the rest one down.
		_ptr[p_index].~T();
		memmove((void *)(_ptr + p_index), (void *)(_ptr + p_index + 1), (new_size - p_index) * sizeof(T));

		// Shrink to fit if necessary.
		const USize new_capacity = smaller_capacity(capacity(), new_size);
		if (new_capacity < capacity()) {
			Error err = _realloc_exact(new_capacity);
			CRASH_COND(err);
		}
		*_get_size() = new_size;
	} else {
		// Remove by forking.
		Error err = _copy_to_new_buffer_exact(smaller_capacity(capacity(), new_size), p_index, 0, new_size - p_index);
		CRASH_COND(err);
	}
}

template <typename T>
Error CowData<T>::insert(Size p_pos, T &&p_val) {
	const Size new_size = size() + 1;
	ERR_FAIL_INDEX_V(p_pos, new_size, ERR_INVALID_PARAMETER);

	if (!_ptr) {
		_alloc_exact(next_capacity(0, 1));
		*_get_size() = 1;
	} else if (_get_refcount()->get() == 1) {
		if ((USize)new_size > capacity()) {
			// Need to grow.
			const Error error = _realloc_exact(grow_capacity(capacity()));
			if (error) {
				return error;
			}
		}

		// Relocate elements one position up.
		memmove((void *)(_ptr + p_pos + 1), (void *)(_ptr + p_pos), (size() - p_pos) * sizeof(T));
		*_get_size() = new_size;
	} else {
		// Insert new element by forking.
		// Use the max of capacity and new_size, to ensure we don't accidentally shrink after reserve.
		const USize new_capacity = next_capacity(capacity(), new_size);
		const Error error = _copy_to_new_buffer_exact(new_capacity, p_pos, 1, size() - p_pos);
		if (error) {
			return error;
		}
	}

	// Create the new element at the given index.
	memnew_placement(_ptr + p_pos, T(std::move(p_val)));

	return OK;
}

template <typename T>
Error CowData<T>::push_back(T &&p_val) {
	const Size new_size = size() + 1;

	if (!_ptr) {
		// Grow by allocating.
		_alloc_exact(next_capacity(0, 1));
		*_get_size() = 1;
	} else if (_get_refcount()->get() == 1) {
		// Grow in-place.
		if ((USize)new_size > capacity()) {
			// Need to grow.
			const Error error = _realloc_exact(grow_capacity(capacity()));
			if (error) {
				return error;
			}
		}

		*_get_size() = new_size;
	} else {
		// Grow by forking.
		// Use the max of capacity and new_size, to ensure we don't accidentally shrink after reserve.
		const USize new_capacity = next_capacity(capacity(), new_size);
		const Error error = _copy_to_new_buffer_exact(new_capacity, size(), 1, 0);
		if (error) {
			return error;
		}
	}

	// Create the new element at the given index.
	memnew_placement(_ptr + new_size - 1, T(std::move(p_val)));

	return OK;
}

template <typename T>
template <bool p_exact>
Error CowData<T>::reserve(USize p_min_capacity) {
	USize new_capacity = p_exact ? p_min_capacity : next_capacity(capacity(), p_min_capacity);
	if (new_capacity <= capacity()) {
		if (p_min_capacity < (USize)size()) {
			WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This is likely a mistake.");
		}
		// No need to reserve more, we already have (at least) the right size.
		return OK;
	}

	if (!_ptr) {
		// Initial allocation.
		return _alloc_exact(new_capacity);
	} else if (_get_refcount()->get() == 1) {
		// Grow in-place.
		return _realloc_exact(new_capacity);
	} else {
		// Grow by forking.
		return _copy_to_new_buffer_exact(new_capacity, size(), 0, 0);
	}
}

template <typename T>
template <bool p_initialize>
Error CowData<T>::resize(Size p_size) {
	ERR_FAIL_COND_V(p_size < 0, ERR_INVALID_PARAMETER);

	const Size prev_size = size();
	if (p_size == prev_size) {
		// Caller wants to stay the same size, so we do nothing.
		return OK;
	}

	if (p_size > prev_size) {
		// Caller wants to grow.

		if (!_ptr) {
			// Grow by allocating.
			const Error error = _alloc_exact(next_capacity(0, p_size));
			if (error) {
				return error;
			}
		} else if (_get_refcount()->get() == 1) {
			// Grow in-place.
			if ((USize)p_size > capacity()) {
				const Error error = _realloc_exact(next_capacity(capacity(), p_size));
				if (error) {
					return error;
				}
			}
		} else {
			// Grow by forking.
			const Error error = _copy_to_new_buffer_exact(next_capacity(capacity(), p_size), prev_size, 0, 0);
			if (error) {
				return error;
			}
		}

		// Construct new elements.
		if constexpr (p_initialize) {
			memnew_arr_placement(_ptr + prev_size, p_size - prev_size);
		}
		*_get_size() = p_size;

		return OK;
	} else {
		// Caller wants to shrink.

		if (p_size == 0) {
			_unref();
			return OK;
		} else if (_get_refcount()->get() == 1) {
			// Shrink in-place.
			destruct_arr_placement(_ptr + p_size, prev_size - p_size);

			// Shrink buffer if necessary.
			const USize new_capacity = smaller_capacity(capacity(), p_size);
			if (new_capacity < capacity()) {
				Error err = _realloc_exact(new_capacity);
				CRASH_COND(err);
			}

			*_get_size() = p_size;
			return OK;
		} else {
			// Shrink by forking.
			const USize new_capacity = smaller_capacity(capacity(), p_size);
			return _copy_to_new_buffer_exact(new_capacity, p_size, 0, 0);
		}
	}
}

template <typename T>
Error CowData<T>::_alloc_exact(USize p_capacity) {
	DEV_ASSERT(!_ptr);

	uint8_t *mem_new = (uint8_t *)Memory::alloc_static(p_capacity * sizeof(T) + DATA_OFFSET, false);
	ERR_FAIL_NULL_V(mem_new, ERR_OUT_OF_MEMORY);

	_ptr = _get_data_ptr(mem_new);

	// If we alloc, we're guaranteed to be the only reference.
	new (_get_refcount()) SafeNumeric<USize>(1);
	*_get_size() = 0;
	// The actual capacity is whatever we can stuff into the alloc_size.
	*_get_capacity() = p_capacity;

	return OK;
}

template <typename T>
Error CowData<T>::_realloc_exact(USize p_capacity) {
	DEV_ASSERT(_ptr);

	uint8_t *mem_new = (uint8_t *)Memory::realloc_static(((uint8_t *)_ptr) - DATA_OFFSET, p_capacity * sizeof(T) + DATA_OFFSET, false);
	ERR_FAIL_NULL_V(mem_new, ERR_OUT_OF_MEMORY);

	_ptr = _get_data_ptr(mem_new);

	// If we realloc, we're guaranteed to be the only reference.
	// So the reference was 1 and was copied to be 1 again.
	DEV_ASSERT(_get_refcount()->get() == 1);
	// The size was also copied from the previous allocation.
	// The actual capacity is whatever we can stuff into the alloc_size.
	*_get_capacity() = p_capacity;

	return OK;
}

template <typename T>
Error CowData<T>::_copy_to_new_buffer_exact(USize p_capacity, USize p_size_from_start, USize p_gap, USize p_size_from_back) {
	DEV_ASSERT(p_capacity >= p_size_from_start + p_size_from_back + p_gap);
	DEV_ASSERT((USize)size() >= p_size_from_start && (USize)size() >= p_size_from_back);

	// Create a temporary CowData to hold ownership over our _ptr.
	// It will be used to copy elements from the old buffer over to our new buffer.
	// At the end of the block, it will be automatically destructed by going out of scope.
	const CowData prev_data;
	prev_data._ptr = _ptr;
	_ptr = nullptr;

	const Error error = _alloc_exact(p_capacity);
	if (error) {
		// On failure to allocate, recover the old data and return the error.
		_ptr = prev_data._ptr;
		prev_data._ptr = nullptr;
		return error;
	}

	// Copy over elements.
	copy_arr_placement(_ptr, prev_data._ptr, p_size_from_start);
	copy_arr_placement(
			_ptr + p_size_from_start + p_gap,
			prev_data._ptr + prev_data.size() - p_size_from_back,
			p_size_from_back);
	*_get_size() = p_size_from_start + p_gap + p_size_from_back;

	return OK;
}

template <typename T>
Error CowData<T>::_copy_on_write() {
	if (!_ptr || _get_refcount()->get() == 1) {
		// Nothing to do.
		return OK;
	}

	// Fork to become the only reference.
	return _copy_to_new_buffer_exact(capacity(), size(), 0, 0);
}

template <typename T>
void CowData<T>::_ref(const CowData *p_from) {
	_ref(*p_from);
}

template <typename T>
void CowData<T>::_ref(const CowData &p_from) {
	if (_ptr == p_from._ptr) {
		return; // self assign, do nothing.
	}

	_unref(); // Resets _ptr to nullptr.

	if (!p_from._ptr) {
		return; //nothing to do
	}

	if (p_from._get_refcount()->conditional_increment() > 0) { // could reference
		_ptr = p_from._ptr;
	}
}

template <typename T>
CowData<T>::CowData(std::initializer_list<T> p_init) {
	CRASH_COND(_alloc_exact(p_init.size()));

	copy_arr_placement(_ptr, p_init.begin(), p_init.size());
	*_get_size() = p_init.size();
}

template <typename T>
CowData<T>::CowData(Span<T> p_span) {
	CRASH_COND(_alloc_exact(p_span.size()));

	copy_arr_placement(_ptr, p_span.begin(), p_span.size());
	*_get_size() = p_span.size();
}

VLTR_GCC_WARNING_POP

// Zero-constructing CowData initializes _ptr to nullptr (and thus empty).
template <typename T>
struct is_zero_constructible<CowData<T>> : std::true_type {};

template <typename T> class Vector;

template <typename T> class VectorWriteProxy
{
public:
	_FORCE_INLINE_ T& operator[](typename CowData<T>::Size p_index)
	{
		CRASH_BAD_INDEX(p_index, ((Vector<T>*)(this))->_cowdata.size());

		return ((Vector<T>*)(this))->_cowdata.ptrw()[p_index];
	}
};

template <typename T> class _WARN_UNUSED_ Vector
{
	friend class VectorWriteProxy<T>;

public:
	VectorWriteProxy<T> write;
	typedef typename CowData<T>::Size Size;
	typedef typename CowData<T>::USize USize;

private:
	CowData<T> _cowdata;

public:
	// Must take a copy instead of a reference (see GH-31736).
	_FORCE_INLINE_ bool push_back(T p_elem) { return _cowdata.push_back(std::move(p_elem)); }

	_FORCE_INLINE_ T pop_back()
	{
		if (!_cowdata.is_empty()) {
			const int n = _cowdata.size() - 1;
			T ret = std::move(_cowdata.get(n));
			_cowdata.resize(n);
			return ret;
		}
		return T();
	}

	_FORCE_INLINE_ bool append(T p_elem) { return _cowdata.push_back(std::move(p_elem)); } // alias

	void fill(T p_elem);

	void remove_at(Size p_index) { _cowdata.remove_at(p_index); }

	_FORCE_INLINE_ bool erase(const T& p_val)
	{
		Size idx = find(p_val);
		if (idx >= 0) {
			remove_at(idx);
			return true;
		}
		return false;
	}

	void reverse();

	_FORCE_INLINE_ T* ptrw() _LIFETIME_BOUND_ { return _cowdata.ptrw(); }

	_FORCE_INLINE_ const T* ptr() const _LIFETIME_BOUND_ { return _cowdata.ptr(); }

	_FORCE_INLINE_ Size size() const { return _cowdata.size(); }

	_FORCE_INLINE_ USize capacity() const { return _cowdata.capacity(); }

	_FORCE_INLINE_ operator Span<T>() const _LIFETIME_BOUND_ { return _cowdata.span(); }

	_FORCE_INLINE_ Span<T> span() const _LIFETIME_BOUND_ { return _cowdata.span(); }

	_FORCE_INLINE_ void clear() { _cowdata.clear(); }

	_FORCE_INLINE_ bool is_empty() const { return _cowdata.is_empty(); }

	_FORCE_INLINE_ T get(Size p_index) _LIFETIME_BOUND_ { return _cowdata.get(p_index); }

	_FORCE_INLINE_ const T& get(Size p_index) const _LIFETIME_BOUND_
	{
		return _cowdata.get(p_index);
	}

	_FORCE_INLINE_ void set(Size p_index, const T& p_elem) { _cowdata.set(p_index, p_elem); }

	/// Resize the vector.
	/// Elements are initialized (or not) depending on what the default C++ behavior for this type
	/// is.
	_FORCE_INLINE_ Error resize(Size p_size)
	{
		return _cowdata.template resize<!std::is_trivially_constructible_v<T>>(p_size);
	}

	/// Resize and set new values to 0 / false / nullptr.
	/// This is only available for zero constructible types.
	_FORCE_INLINE_ Error resize_initialized(Size p_size)
	{
		return _cowdata.template resize<true>(p_size);
	}

	/// Resize and keep memory uninitialized.
	/// This means that any newly added elements have an unknown value, and are expected to be set
	/// after the `resize_uninitialized` call. This is only available for trivially destructible
	/// types (otherwise, trivial resize might be UB).
	_FORCE_INLINE_ Error resize_uninitialized(Size p_size)
	{
		// resize() statically asserts that T is compatible, no need to do it ourselves.
		return _cowdata.template resize<false>(p_size);
	}

	/// Reserves capacity for at least p_size total elements.
	/// You can use `reserve` before repeated insertions to improve performance.
	/// The capacity grows in 1.5x increments when possible, and uses `p_size`
	/// exactly otherwise.
	Error reserve(Size p_size)
	{
		ERR_FAIL_COND_V(p_size < 0, ERR_INVALID_PARAMETER);
		return _cowdata.reserve(p_size);
	}

	/// Reserves capacity for exactly p_size total elements.
	/// This can be useful to reduce RAM use of large vectors, but wastes CPU
	/// time if more than p_size elements are added after the `reserve_exact` call.
	/// Prefer using `reserve`, unless the vector (or copies of it) will never
	/// grow again after p_size elements are inserted.
	Error reserve_exact(Size p_size)
	{
		ERR_FAIL_COND_V(p_size < 0, ERR_INVALID_PARAMETER);
		return _cowdata.reserve_exact(p_size);
	}

	_FORCE_INLINE_ const T& operator[](Size p_index) const _LIFETIME_BOUND_
	{
		return _cowdata.get(p_index);
	}

	// Must take a copy instead of a reference (see GH-31736).
	Error insert(Size p_pos, T p_val) { return _cowdata.insert(p_pos, std::move(p_val)); }

	Size find(const T& p_val, Size p_from = 0) const
	{
		if (p_from < 0) {
			p_from = size() + p_from;
		}
		if (p_from < 0 || p_from >= size()) {
			return -1;
		}
		return span().find(p_val, p_from);
	}

	Size rfind(const T& p_val, Size p_from = -1) const
	{
		if (p_from < 0) {
			p_from = size() + p_from;
		}
		if (p_from < 0 || p_from >= size()) {
			return -1;
		}
		return span().rfind(p_val, p_from);
	}

	Size count(const T& p_val) const { return span().count(p_val); }

	// Must take a copy instead of a reference (see GH-31736).
	void append_array(Vector<T> p_other);

	_FORCE_INLINE_ bool has(const T& p_val) const { return find(p_val) != -1; }

	void sort() { sort_custom<Comparator<T>>(); }

	template <typename Comparator, bool Validate = SORT_ARRAY_VALIDATE_ENABLED, typename... Args>
	void sort_custom(Args&&... p_args)
	{
		Size len = _cowdata.size();
		if (len == 0) {
			return;
		}

		T* data = ptrw();
		SortArray<T, Comparator, Validate> sorter{p_args...};
		sorter.sort(data, len);
	}

	Size bsearch(const T& p_value, bool p_before) const
	{
		return bsearch_custom<Comparator<T>>(p_value, p_before);
	}

	template <typename Comparator, typename Value, typename... Args>
	Size bsearch_custom(const Value& p_value, bool p_before, Args&&... p_args) const
	{
		return span().bisect(p_value, p_before, Comparator{p_args...});
	}

	Vector<T> duplicate() const { return *this; }

	void ordered_insert(const T& p_val)
	{
		Size i;
		for (i = 0; i < _cowdata.size(); i++) {
			if (p_val < operator[](i)) {
				break;
			}
		}
		insert(i, p_val);
	}

	void operator=(const Vector& p_from) { _cowdata = p_from._cowdata; }

	void operator=(Vector&& p_from) { _cowdata = std::move(p_from._cowdata); }

	Vector<uint8_t> to_byte_array() const
	{
		Vector<uint8_t> ret;
		if (is_empty()) {
			return ret;
		}
		size_t alloc_size = size() * sizeof(T);
		ret.resize(alloc_size);
		if (alloc_size) {
			memcpy(ret.ptrw(), ptr(), alloc_size);
		}
		return ret;
	}

	Vector<T> slice(Size p_begin, Size p_end = CowData<T>::MAX_INT) const
	{
		Vector<T> result;

		const Size s = size();

		Size begin = CLAMP(p_begin, -s, s);
		if (begin < 0) {
			begin += s;
		}
		Size end = CLAMP(p_end, -s, s);
		if (end < 0) {
			end += s;
		}

		ERR_FAIL_COND_V(begin > end, result);

		Size result_size = end - begin;
		result.resize(result_size);

		const T* const r = ptr();
		T* const w = result.ptrw();
		for (Size i = 0; i < result_size; ++i) {
			w[i] = r[begin + i];
		}

		return result;
	}

	bool operator==(const Vector<T>& p_arr) const { return span() == p_arr.span(); }

	bool operator!=(const Vector<T>& p_arr) const { return span() != p_arr.span(); }

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

	_FORCE_INLINE_ Iterator begin() _LIFETIME_BOUND_ { return Iterator(ptrw()); }

	_FORCE_INLINE_ Iterator end() _LIFETIME_BOUND_ { return Iterator(ptrw() + size()); }

	_FORCE_INLINE_ ConstIterator begin() const _LIFETIME_BOUND_ { return ConstIterator(ptr()); }

	_FORCE_INLINE_ ConstIterator end() const _LIFETIME_BOUND_
	{
		return ConstIterator(ptr() + size());
	}

	_FORCE_INLINE_ Vector() {}

	_FORCE_INLINE_ Vector(std::initializer_list<T> p_init) : _cowdata(p_init) {}

	_FORCE_INLINE_ explicit Vector(Span<T> p_span) : _cowdata(p_span) {}

	_FORCE_INLINE_ Vector(const Vector& p_from) = default;
	_FORCE_INLINE_ Vector(Vector&& p_from) = default;
};

template <typename T> void Vector<T>::reverse()
{
	T* p = ptrw();
	for (Size i = 0; i < size() / 2; i++) {
		SWAP(p[i], p[size() - i - 1]);
	}
}

template <typename T> void Vector<T>::append_array(Vector<T> p_other)
{
	const Size ds = p_other.size();
	if (ds == 0) {
		return;
	}
	const Size bs = size();
	resize(bs + ds);
	T* p = ptrw();
	for (Size i = 0; i < ds; ++i) {
		p[bs + i] = p_other[i];
	}
}

template <typename T> void Vector<T>::fill(T p_elem)
{
	T* p = ptrw();
	for (Size i = 0; i < size(); i++) {
		p[i] = p_elem;
	}
}

// Zero-constructing Vector initializes CowData.ptr() to nullptr and thus empty.
template <typename T> struct is_zero_constructible<Vector<T>> : std::true_type
{
};

template <typename T>
class _WARN_UNUSED_ VSet {
	Vector<T> _data;

protected:
	_FORCE_INLINE_ int _find(const T &p_val, bool &r_exact) const {
		r_exact = false;
		if (_data.is_empty()) {
			return 0;
		}

		int64_t pos = _data.span().bisect(p_val, true);

		if (pos < _data.size() && !(p_val < _data[pos]) && !(_data[pos] < p_val)) {
			r_exact = true;
		}
		return pos;
	}

	_FORCE_INLINE_ int _find_exact(const T &p_val) const {
		if (_data.is_empty()) {
			return -1;
		}

		int64_t pos = _data.span().bisect(p_val, true);

		if (pos < _data.size() && !(p_val < _data[pos]) && !(_data[pos] < p_val)) {
			return pos;
		}
		return -1;
	}

public:
	void insert(const T &p_val) {
		bool exact;
		int pos = _find(p_val, exact);
		if (exact) {
			return;
		}
		_data.insert(pos, p_val);
	}

	bool has(const T &p_val) const {
		return _find_exact(p_val) != -1;
	}

	void erase(const T &p_val) {
		int pos = _find_exact(p_val);
		if (pos < 0) {
			return;
		}
		_data.remove_at(pos);
	}

	int find(const T &p_val) const {
		return _find_exact(p_val);
	}

	_FORCE_INLINE_ bool is_empty() const { return _data.is_empty(); }

	_FORCE_INLINE_ int size() const { return _data.size(); }

	inline T &operator[](int p_index) {
		return _data.write[p_index];
	}

	inline const T &operator[](int p_index) const {
		return _data[p_index];
	}

	_FORCE_INLINE_ VSet() {}
	_FORCE_INLINE_ VSet(std::initializer_list<T> p_init) :
			_data(p_init) {}
};
#include "core/math/vector2.h"

#define UNIQUE_NODE_PREFIX "%"

class String;

template <typename T> class CharStringT;

/*************************************************************************/
/*  Type-Traits for String Conversion (C++17 Compatible)                 */
/*************************************************************************/

namespace string_traits
{

template <typename, typename = void> struct has_op_string : std::false_type
{
};

template <typename T>
struct has_op_string<T, std::void_t<decltype(std::declval<T>().operator String())>> : std::true_type
{
};

template <typename, typename = void> struct has_to_string : std::false_type
{
};

template <typename T>
struct has_to_string<T, std::void_t<decltype(std::declval<T>().to_string())>> : std::true_type
{
};

template <typename, typename = void> struct has_as_text : std::false_type
{
};

template <typename T>
struct has_as_text<T, std::void_t<decltype(std::declval<T>().as_text())>> : std::true_type
{
};

} // namespace string_traits

/*************************************************************************/
/*  Utility Functions                                                    */
/*************************************************************************/

// Not defined by std.
// strlen equivalent function for char16_t * arguments.
constexpr size_t strlen(const char16_t* p_str)
{
	const char16_t* ptr = p_str;
	while (*ptr != 0) {
		++ptr;
	}
	return ptr - p_str;
}

// strlen equivalent function for char32_t * arguments.
constexpr size_t strlen(const char32_t* p_str)
{
	const char32_t* ptr = p_str;
	while (*ptr != 0) {
		++ptr;
	}
	return ptr - p_str;
}

// strlen equivalent function for wchar_t * arguments; depends on the platform.
constexpr size_t strlen(const wchar_t* p_str)
{
	// Use static_cast twice because reinterpret_cast is not allowed in constexpr
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	return strlen(static_cast<const char16_t*>(static_cast<const void*>(p_str)));
#else
	// wchar_t is 32-bit
	return strlen(static_cast<const char32_t*>(static_cast<const void*>(p_str)));
#endif
}

// strnlen equivalent function for char16_t * arguments.
constexpr size_t strnlen(const char16_t* p_str, size_t p_clip_to_len)
{
	size_t len = 0;
	while (len < p_clip_to_len && *(p_str++) != 0) {
		len++;
	}
	return len;
}

// strnlen equivalent function for char32_t * arguments.
constexpr size_t strnlen(const char32_t* p_str, size_t p_clip_to_len)
{
	size_t len = 0;
	while (len < p_clip_to_len && *(p_str++) != 0) {
		len++;
	}
	return len;
}

// strnlen equivalent function for wchar_t * arguments; depends on the platform.
constexpr size_t strnlen(const wchar_t* p_str, size_t p_clip_to_len)
{
	// Use static_cast twice because reinterpret_cast is not allowed in constexpr
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	return strnlen(static_cast<const char16_t*>(static_cast<const void*>(p_str)), p_clip_to_len);
#else
	// wchar_t is 32-bit
	return strnlen(static_cast<const char32_t*>(static_cast<const void*>(p_str)), p_clip_to_len);
#endif
}

template <typename L, typename R> constexpr int64_t str_compare(const L* p_left, const R* p_right)
{
	while (true) {
		const char32_t l = *p_left;
		const char32_t r = *p_right;

		if (l == 0 || l != r) {
			return static_cast<int64_t>(l) - static_cast<int64_t>(r);
		}

		p_left++;
		p_right++;
	}
}

/*************************************************************************/
/*  CharProxy                                                            */
/*************************************************************************/

template <typename T> class [[nodiscard]] CharProxy
{
	friend String;
	friend CharStringT<T>;

	const int _index;
	CowData<T>& _cowdata;
	static constexpr T _null = 0;

	_FORCE_INLINE_ CharProxy(const int& p_index, CowData<T>& p_cowdata)
		: _index(p_index), _cowdata(p_cowdata)
	{
	}

public:
	_FORCE_INLINE_ CharProxy(const CharProxy<T>& p_other)
		: _index(p_other._index), _cowdata(p_other._cowdata)
	{
	}

	_FORCE_INLINE_ operator T() const
	{
		if (unlikely(_index == _cowdata.size())) {
			return _null;
		}

		return _cowdata.get(_index);
	}

	_FORCE_INLINE_ const T* operator&() const { return _cowdata.ptr() + _index; }

	_FORCE_INLINE_ void operator=(const T& p_other) const { _cowdata.set(_index, p_other); }

	_FORCE_INLINE_ void operator=(const CharProxy<T>& p_other) const
	{
		_cowdata.set(_index, p_other.operator T());
	}
};

/*************************************************************************/
/*  CharStringT                                                          */
/*************************************************************************/

template <typename T> class [[nodiscard]] CharStringT
{
	CowData<T> _cowdata;
	static constexpr T _null = 0;

public:
	_FORCE_INLINE_ T* ptrw() _LIFETIME_BOUND_ { return _cowdata.ptrw(); }

	_FORCE_INLINE_ const T* ptr() const _LIFETIME_BOUND_ { return _cowdata.ptr(); }

	_FORCE_INLINE_ const T* get_data() const _LIFETIME_BOUND_ { return size() ? ptr() : &_null; }

	// Returns the number of characters in the buffer, including the terminating NUL character.
	// In most cases, length() should be used instead.
	_FORCE_INLINE_ int size() const { return _cowdata.size(); }

	// Returns the number of characters in the string (excluding terminating NUL character).
	_FORCE_INLINE_ int length() const { return size() ? size() - 1 : 0; }

	_FORCE_INLINE_ bool is_empty() const { return length() == 0; }

	_FORCE_INLINE_ operator Span<T>() const _LIFETIME_BOUND_ { return Span(ptr(), length()); }

	_FORCE_INLINE_ Span<T> span() const _LIFETIME_BOUND_ { return Span(ptr(), length()); }

	/// Resizes the string. The given size must include the null terminator.
	/// New characters are not initialized, and should be set by the caller.
	_FORCE_INLINE_ Error resize_uninitialized(int64_t p_size)
	{
		return _cowdata.template resize<false>(p_size);
	}

	_FORCE_INLINE_ T get(int p_index) const { return _cowdata.get(p_index); }

	_FORCE_INLINE_ void set(int p_index, const T& p_elem) { _cowdata.set(p_index, p_elem); }

	_FORCE_INLINE_ const T& operator[](int p_index) const
	{
		if (unlikely(p_index == _cowdata.size())) {
			return _null;
		}
		return _cowdata.get(p_index);
	}

	_FORCE_INLINE_ CharProxy<T> operator[](int p_index) { return CharProxy<T>(p_index, _cowdata); }

	_FORCE_INLINE_ CharStringT() = default;
	_FORCE_INLINE_ CharStringT(const CharStringT& p_str) = default;
	_FORCE_INLINE_ CharStringT(CharStringT&& p_str) = default;

	_FORCE_INLINE_ void operator=(const CharStringT& p_str) { _cowdata = p_str._cowdata; }

	_FORCE_INLINE_ void operator=(CharStringT&& p_str) { _cowdata = std::move(p_str._cowdata); }

	_FORCE_INLINE_ CharStringT(const T* p_cstr) { copy_from(p_cstr); }

	_FORCE_INLINE_ void operator=(const T* p_cstr) { copy_from(p_cstr); }

	_FORCE_INLINE_ bool operator==(const CharStringT<T>& p_other) const
	{
		return span() == p_other.span();
	}

	_FORCE_INLINE_ bool operator!=(const CharStringT<T>& p_other) const
	{
		return !(*this == p_other);
	}

	_FORCE_INLINE_ bool operator<(const CharStringT<T>& p_other) const
	{
		if (length() == 0) {
			return p_other.length() != 0;
		}
		return str_compare(get_data(), p_other.get_data()) < 0;
	}

	_FORCE_INLINE_ CharStringT<T>& operator+=(T p_char)
	{
		const int lhs_len = length();
		resize_uninitialized(lhs_len + 2);

		T* dst = ptrw();
		dst[lhs_len] = p_char;
		dst[lhs_len + 1] = _null;

		return *this;
	}

	uint32_t hash() const { return hash_djb2(get_data()); }

protected:
	void copy_from(const T* p_cstr)
	{
		if (!p_cstr) {
			resize_uninitialized(0);
			return;
		}

		size_t len = strlen(p_cstr);
		if (len == 0) {
			resize_uninitialized(0);
			return;
		}

		Error err = resize_uninitialized(++len); // include terminating null char.

		ERR_FAIL_COND_MSG(err != OK, "Failed to copy C-string.");

		memcpy(ptrw(), p_cstr, len * sizeof(T));
	}
};

template <typename T> struct is_zero_constructible<CharStringT<T>> : std::true_type
{
};

using CharString = CharStringT<char>;
using Char16String = CharStringT<char16_t>;

/*************************************************************************/
/*  String                                                               */
/*************************************************************************/

class [[nodiscard]] _WARN_UNUSED_ String
{
	CowData<char32_t> _cowdata;
	static constexpr char32_t _null = 0;
	static constexpr char32_t _replacement_char = 0xfffd;

	// NULL-terminated c string copy - automatically parse the string to find the length.
	void append_latin1(const char* p_cstr)
	{
		append_latin1(Span(p_cstr, p_cstr ? strlen(p_cstr) : 0));
	}

	void append_utf32(const char32_t* p_cstr)
	{
		append_utf32(Span(p_cstr, p_cstr ? strlen(p_cstr) : 0));
	}

	void append_wstring(const wchar_t* p_cstr)
	{
#ifdef WINDOWS_ENABLED
		// wchar_t is 16-bit, parse as UTF-16
		append_utf16((const char16_t*)p_cstr);
#else
		// wchar_t is 32-bit, copy directly
		append_utf32((const char32_t*)p_cstr);
#endif
	}

	bool _base_is_subsequence_of(const String& p_string, bool p_case_insensitive) const;
	int _count(const String& p_string, int p_from, int p_to, bool p_case_insensitive) const;
	int _count(const char* p_string, int p_from, int p_to, bool p_case_insensitive) const;
	String _separate_compound_words() const;

public:
	// --- Direct Numeric & Primitive Constructors ---
	_FORCE_INLINE_ explicit String(bool p_val) { *this = p_val ? "true" : "false"; }

	_FORCE_INLINE_ explicit String(int8_t p_val) { *this = String::num_int64(p_val); }

	_FORCE_INLINE_ explicit String(uint8_t p_val) { *this = String::num_uint64(p_val); }

	_FORCE_INLINE_ explicit String(int16_t p_val) { *this = String::num_int64(p_val); }

	_FORCE_INLINE_ explicit String(uint16_t p_val) { *this = String::num_uint64(p_val); }

	_FORCE_INLINE_ explicit String(int32_t p_val) { *this = String::num_int64(p_val); }

	_FORCE_INLINE_ explicit String(uint32_t p_val) { *this = String::num_uint64(p_val); }

	_FORCE_INLINE_ explicit String(int64_t p_val) { *this = String::num_int64(p_val); }

	_FORCE_INLINE_ explicit String(uint64_t p_val) { *this = String::num_uint64(p_val); }

	_FORCE_INLINE_ explicit String(float p_val) { *this = String::num_real(p_val); }

	_FORCE_INLINE_ explicit String(double p_val) { *this = String::num_real(p_val); }
	_FORCE_INLINE_ bool is_node_unique_name() const {
	    if (_cowdata.is_empty()) {
	        return false;
	    }
	    return (char32_t)_cowdata.get(0) == (char32_t)UNIQUE_NODE_PREFIX[0]; // '%'
	}

	_FORCE_INLINE_ ~String() = default;

	// --- Universal Auto-Converter Template (Enums, Objects with to_string/as_text/cast) ---
	template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, String> &&
													  !std::is_pointer_v<std::decay_t<T>> &&
													  !std::is_array_v<std::remove_reference_t<T>>>>
	_FORCE_INLINE_ explicit String(const T& p_val)
	{
		if constexpr (std::is_enum_v<T>) {
			*this = String::num_int64(static_cast<int64_t>(p_val));
		}
		else if constexpr (string_traits::has_op_string<T>::value) {
			*this = p_val.operator String();
		}
		else if constexpr (string_traits::has_to_string<T>::value) {
			*this = p_val.to_string();
		}
		else if constexpr (string_traits::has_as_text<T>::value) {
			*this = p_val.as_text();
		}
	}

	enum
	{
		npos =
			-1 ///< for "some" compatibility with std::string (npos is a huge value in std::string)
	};

	_FORCE_INLINE_ char32_t* ptrw() _LIFETIME_BOUND_ { return _cowdata.ptrw(); }

	_FORCE_INLINE_ const char32_t* ptr() const _LIFETIME_BOUND_ { return _cowdata.ptr(); }

	_FORCE_INLINE_ const char32_t* get_data() const _LIFETIME_BOUND_
	{
		return size() ? ptr() : &_null;
	}

	// Returns the number of characters in the buffer, including the terminating NUL character.
	// In most cases, length() should be used instead.
	_FORCE_INLINE_ int size() const { return _cowdata.size(); }

	// Returns the number of characters in the string (excluding terminating NUL character).
	_FORCE_INLINE_ int length() const { return size() ? size() - 1 : 0; }

	_FORCE_INLINE_ bool is_empty() const { return length() == 0; }

	_FORCE_INLINE_ operator Span<char32_t>() const _LIFETIME_BOUND_
	{
		return Span(ptr(), length());
	}

	_FORCE_INLINE_ Span<char32_t> span() const _LIFETIME_BOUND_ { return Span(ptr(), length()); }

	void remove_at(int p_index) { _cowdata.remove_at(p_index); }

	_FORCE_INLINE_ void clear() { resize_uninitialized(0); }

	_FORCE_INLINE_ char32_t get(int p_index) const { return _cowdata.get(p_index); }

	_FORCE_INLINE_ void set(int p_index, const char32_t& p_elem) { _cowdata.set(p_index, p_elem); }

	/// Resizes the string. The given size must include the null terminator.
	/// New characters are not initialized, and should be set by the caller.
	Error resize_uninitialized(int64_t p_size) { return _cowdata.resize<false>(p_size); }

	Error reserve(int64_t p_size)
	{
		ERR_FAIL_COND_V(p_size < 0, ERR_INVALID_PARAMETER);
		return _cowdata.reserve(p_size);
	}

	_FORCE_INLINE_ const char32_t& operator[](int p_index) const
	{
		if (unlikely(p_index == _cowdata.size())) {
			return _null;
		}

		return _cowdata.get(p_index);
	}

	_FORCE_INLINE_ CharProxy<char32_t> operator[](int p_index)
	{
		return CharProxy<char32_t>(p_index, _cowdata);
	}

	/* Compatibility Operators */

	bool operator==(const String& p_str) const;
	bool operator!=(const String& p_str) const;
	String operator+(const String& p_str) const;
	String operator+(const char* p_char) const;
	String operator+(const wchar_t* p_char) const;
	String operator+(const char32_t* p_char) const;
	String operator+(char32_t p_char) const;

	String& operator+=(const String&);
	String& operator+=(char32_t p_char);
	String& operator+=(const char* p_str);
	String& operator+=(const wchar_t* p_str);
	String& operator+=(const char32_t* p_str);

	bool operator==(const char* p_str) const;
	bool operator==(const wchar_t* p_str) const;
	bool operator==(const char32_t* p_str) const;
	bool operator==(const Span<char32_t>& p_str_range) const;

	bool operator!=(const char* p_str) const;
	bool operator!=(const wchar_t* p_str) const;
	bool operator!=(const char32_t* p_str) const;

	bool operator<(const char32_t* p_str) const;
	bool operator<(const char* p_str) const;
	bool operator<(const wchar_t* p_str) const;

	bool operator<(const String& p_str) const;
	bool operator<=(const String& p_str) const;
	bool operator>(const String& p_str) const;
	bool operator>=(const String& p_str) const;

	signed char casecmp_to(const String& p_str) const;
	signed char nocasecmp_to(const String& p_str) const;
	signed char naturalcasecmp_to(const String& p_str) const;
	signed char naturalnocasecmp_to(const String& p_str) const;
	// Special sorting for file names. Names starting with `_` are put before all others except
	// those starting with `.`, otherwise natural comparison is used.
	signed char filecasecmp_to(const String& p_str) const;
	signed char filenocasecmp_to(const String& p_str) const;

	bool is_valid_string() const;

	/* debug, error messages */
	void print_unicode_error(const String& p_message, bool p_critical = false) const;

	/* complex helpers */
	String substr(int p_from, int p_chars = -1) const;
	int find(const String& p_str, int p_from = 0) const;  ///< return <0 if failed
	int find(const char* p_str, int p_from = 0) const;	  ///< return <0 if failed
	int find_char(char32_t p_char, int p_from = 0) const; ///< return <0 if failed
	int findn(const String& p_str, int p_from = 0) const; ///< return <0 if failed, case insensitive
	int findn(const char* p_str, int p_from = 0) const;	  ///< return <0 if failed
	int rfind(const String& p_str, int p_from = -1) const;	///< return <0 if failed
	int rfind(const char* p_str, int p_from = -1) const;	///< return <0 if failed
	int rfind_char(char32_t p_char, int p_from = -1) const; ///< return <0 if failed
	int rfindn(
		const String& p_str, int p_from = -1) const;	  ///< return <0 if failed, case insensitive
	int rfindn(const char* p_str, int p_from = -1) const; ///< return <0 if failed
	int findmk(const Vector<String>& p_keys, int p_from = 0,
		int* r_key = nullptr) const; ///< return <0 if failed
	bool match(const String& p_wildcard) const;
	bool matchn(const String& p_wildcard) const;
	bool begins_with(const String& p_string) const;
	bool begins_with(const char* p_string) const;
	bool ends_with(const String& p_string) const;
	bool ends_with(const char* p_string) const;
	bool is_enclosed_in(const String& p_string) const;
	bool is_subsequence_of(const String& p_string) const;
	bool is_subsequence_ofn(const String& p_string) const;
	bool is_quoted() const;
	bool is_lowercase() const;
	Vector<String> bigrams() const;
	float similarity(const String& p_string) const;
	String replace_first(const String& p_key, const String& p_with) const;
	String replace_first(const char* p_key, const char* p_with) const;
	String replace(const String& p_key, const String& p_with) const;
	String replace(const char* p_key, const char* p_with) const;
	String replace_char(char32_t p_key, char32_t p_with) const;
	String replace_chars(const String& p_keys, char32_t p_with) const;
	String replace_chars(const char* p_keys, char32_t p_with) const;
	String replacen(const String& p_key, const String& p_with) const;
	String replacen(const char* p_key, const char* p_with) const;
	String repeat(int p_count) const;
	String reverse() const;
	String insert(int p_at_pos, const String& p_string) const;
	String erase(int p_pos, int p_chars = 1) const;
	String remove_char(char32_t p_what) const;
	String remove_chars(const String& p_chars) const;
	String remove_chars(const char* p_chars) const;
	String pad_decimals(int p_digits) const;
	String pad_zeros(int p_digits) const;
	String trim_prefix(const String& p_prefix) const;
	String trim_prefix(const char* p_prefix) const;
	String trim_suffix(const String& p_suffix) const;
	String trim_suffix(const char* p_suffix) const;
	String lpad(int p_min_length, const String& p_character = " ") const;
	String rpad(int p_min_length, const String& p_character = " ") const;
	String quote(const String& p_quotechar = "\"") const;
	String unquote() const;
	static String num(double p_num, int p_decimals = -1);
	static String num_scientific(double p_num);
	static String num_scientific(float p_num);
	static String num_real(double p_num, bool p_trailing = true);
	static String num_real(float p_num, bool p_trailing = true);
	static String num_int64(int64_t p_num, int p_base = 10, bool p_capitalize_hex = false);
	static String num_uint64(uint64_t p_num, int p_base = 10, bool p_capitalize_hex = false);

	static String chr(char32_t p_char)
	{
		String string;
		string.append_utf32(Span(&p_char, 1));
		return string;
	}

	static String md5(const uint8_t* p_md5);
	static String hex_encode_buffer(const uint8_t* p_buffer, int p_len);
	Vector<uint8_t> hex_decode() const;

	bool is_numeric() const;

	double to_float() const;
	int64_t hex_to_int() const;
	int64_t bin_to_int() const;
	int64_t to_int() const;

	static int64_t to_int(const char* p_str, int p_len = -1);
	static int64_t to_int(const wchar_t* p_str, int p_len = -1);
	static int64_t to_int(const char32_t* p_str, int p_len = -1, bool p_clamp = false);

	static double to_float(const char* p_str);
	static double to_float(const wchar_t* p_str, const wchar_t** r_end = nullptr);
	static double to_float(const char32_t* p_str, const char32_t** r_end = nullptr);
	static uint32_t num_characters(int64_t p_int);

	String capitalize() const;
	String to_camel_case() const;
	String to_pascal_case() const;
	String to_snake_case() const;
	String to_kebab_case() const;

	String get_with_code_lines() const;
	int get_slice_count(const String& p_splitter) const;
	int get_slice_count(const char* p_splitter) const;
	String get_slice(const String& p_splitter, int p_slice) const;
	String get_slice(const char* p_splitter, int p_slice) const;
	String get_slicec(char32_t p_splitter, int p_slice) const;

	Vector<String> split(
		const String& p_splitter = "", bool p_allow_empty = true, int p_maxsplit = 0) const;
	Vector<String> split(
		const char* p_splitter = "", bool p_allow_empty = true, int p_maxsplit = 0) const;
	Vector<String> rsplit(
		const String& p_splitter = "", bool p_allow_empty = true, int p_maxsplit = 0) const;
	Vector<String> rsplit(
		const char* p_splitter = "", bool p_allow_empty = true, int p_maxsplit = 0) const;
	Vector<String> split_spaces(int p_maxsplit = 0) const;
	Vector<double> split_floats(const String& p_splitter, bool p_allow_empty = true) const;
	Vector<float> split_floats_mk(
		const Vector<String>& p_splitters, bool p_allow_empty = true) const;
	Vector<int> split_ints(const String& p_splitter, bool p_allow_empty = true) const;
	Vector<int> split_ints_mk(const Vector<String>& p_splitters, bool p_allow_empty = true) const;

	String join(const Vector<String>& p_parts) const;

	static char32_t char_uppercase(char32_t p_char);
	static char32_t char_lowercase(char32_t p_char);
	String to_upper() const;
	String to_lower() const;

	int count(const String& p_string, int p_from = 0, int p_to = 0) const;
	int count(const char* p_string, int p_from = 0, int p_to = 0) const;
	int countn(const String& p_string, int p_from = 0, int p_to = 0) const;
	int countn(const char* p_string, int p_from = 0, int p_to = 0) const;

	String left(int p_len) const;
	String right(int p_len) const;
	String indent(const String& p_prefix) const;
	String dedent() const;
	String strip_edges(bool p_left = true, bool p_right = true) const;
	String strip_escapes() const;
	String lstrip(const String& p_chars) const;
	String rstrip(const String& p_chars) const;
	String get_extension() const;
	String get_basename() const;
	String path_join(const String& p_path) const;
	char32_t unicode_at(int p_idx) const;

	bool has_extension(const char* p_ext) const { return get_extension().to_lower() == p_ext; }

	bool has_extension(const String& p_ext) const { return get_extension().to_lower() == p_ext; }

	CharString ascii(bool p_allow_extended = false) const;
	// Parse an ascii string.
	// If any character is > 127, an error will be logged, and 0xfffd will be inserted.
	Error append_ascii(const Span<char>& p_range);

	static String ascii(const Span<char>& p_range)
	{
		String s;
		s.append_ascii(p_range);
		return s;
	}

	CharString latin1() const { return ascii(true); }

	void append_latin1(const Span<char>& p_cstr);

	static String latin1(const Span<char>& p_string)
	{
		String string;
		string.append_latin1(p_string);
		return string;
	}

	CharString utf8(Vector<uint8_t>* r_ch_length_map = nullptr) const;
	Error append_utf8(const char* p_utf8, int p_len = -1);

	Error append_utf8(const Span<char>& p_range)
	{
		return append_utf8(p_range.ptr(), p_range.size());
	}

	static String utf8(const char* p_utf8, int p_len = -1)
	{
		String ret;
		ret.append_utf8(p_utf8, p_len);
		return ret;
	}

	static String utf8(const Span<char>& p_range) { return utf8(p_range.ptr(), p_range.size()); }

	Char16String utf16() const;
	Error append_utf16(
		const char16_t* p_utf16, int p_len = -1, bool p_default_little_endian = true);

	Error append_utf16(const Span<char16_t> p_range, bool p_skip_cr = false)
	{
		return append_utf16(p_range.ptr(), p_range.size(), p_skip_cr);
	}

	static String utf16(const char16_t* p_utf16, int p_len = -1)
	{
		String ret;
		ret.append_utf16(p_utf16, p_len);
		return ret;
	}

	static String utf16(const Span<char16_t>& p_range)
	{
		return utf16(p_range.ptr(), p_range.size());
	}

	// wchar_t copy_from depends on the platform.
	Error append_wstring(const Span<wchar_t>& p_cstr)
	{
#ifdef WINDOWS_ENABLED
		// wchar_t is 16-bit, parse as UTF-16
		return append_utf16((const char16_t*)p_cstr.ptr(), p_cstr.size());
#else
		// wchar_t is 32-bit, copy directly
		return append_utf32((Span<char32_t>&)p_cstr);
#endif
	}

	static String wstring(const Span<wchar_t>& p_string)
	{
		String string;
		string.append_wstring(p_string);
		return string;
	}

	Error append_utf32(const Span<char32_t>& p_cstr);

	static String utf32(const Span<char32_t>& p_span)
	{
		String string;
		string.append_utf32(p_span);
		return string;
	}

	// Like append_utf32, but does not check the string for string integrity (and is thus faster).
	// Prefer this function for conversion from trusted utf32 strings.
	void append_utf32_unchecked(const Span<char32_t>& p_span);

	static String utf32_unchecked(const Span<char32_t>& p_string)
	{
		String string;
		string.append_utf32_unchecked(p_string);
		return string;
	}

	static uint32_t hash(const char32_t* p_cstr, int p_len); /* hash the string */
	static uint32_t hash(const char32_t* p_cstr);			 /* hash the string */
	static uint32_t hash(const wchar_t* p_cstr, int p_len);	 /* hash the string */
	static uint32_t hash(const wchar_t* p_cstr);			 /* hash the string */
	static uint32_t hash(const char* p_cstr, int p_len);	 /* hash the string */
	static uint32_t hash(const char* p_cstr);				 /* hash the string */
	uint32_t hash() const;									 /* hash the string */
	uint64_t hash64() const;								 /* hash the string */
	String md5_text() const;
	String sha1_text() const;
	String sha256_text() const;
	Vector<uint8_t> md5_buffer() const;
	Vector<uint8_t> sha1_buffer() const;
	Vector<uint8_t> sha256_buffer() const;

	_FORCE_INLINE_ bool contains(const char* p_str) const { return find(p_str) != -1; }

	_FORCE_INLINE_ bool contains(const String& p_str) const { return find(p_str) != -1; }

	_FORCE_INLINE_ bool contains_char(char32_t p_chr) const { return find_char(p_chr) != -1; }

	_FORCE_INLINE_ bool containsn(const char* p_str) const { return findn(p_str) != -1; }

	_FORCE_INLINE_ bool containsn(const String& p_str) const { return findn(p_str) != -1; }

	// path functions
	bool is_absolute_path() const;
	bool is_relative_path() const;
	bool is_resource_file() const;
	String path_to(const String& p_path) const;
	String path_to_file(const String& p_path) const;
	String get_base_dir() const;
	String get_file() const;
	static String humanize_size(uint64_t p_size);
	String simplify_path() const;
	bool is_network_share_path() const;

	String xml_escape(bool p_escape_quotes = false) const;
	String xml_unescape() const;
	String uri_encode() const;
	String uri_decode() const;
	String uri_file_decode() const;
	String c_escape() const;
	String c_escape_multiline() const;
	String c_unescape() const;
	String json_escape() const;
	Error parse_url(
		String& r_scheme, String& r_host, int& r_port, String& r_path, String& r_fragment) const;

	String property_name_encode() const;

	// node functions
	static String get_invalid_node_name_characters(bool p_allow_internal = false);
	String validate_node_name() const;
	String validate_ascii_identifier() const;
	String validate_unicode_identifier() const;
	String validate_filename() const;

	bool is_valid_ascii_identifier() const;
	bool is_valid_unicode_identifier() const;
	bool is_valid_int() const;
	bool is_valid_float() const;
	bool is_valid_hex_number(bool p_with_prefix) const;
	bool is_valid_html_color() const;
	bool is_valid_ip_address() const;
	bool is_valid_filename() const;

	// Use `is_valid_ascii_identifier()` instead. Kept for compatibility.
	bool is_valid_identifier() const { return is_valid_ascii_identifier(); }

	/**
	 * The constructors must not depend on other overloads
	 */

	_FORCE_INLINE_ String() {}

	_FORCE_INLINE_ String(const String& p_str) = default;
	_FORCE_INLINE_ String(String&& p_str) = default;
#ifdef SIZE_EXTRA
	_NO_INLINE_ ~String() {}
#endif
	_FORCE_INLINE_ void operator=(const String& p_str) { _cowdata = p_str._cowdata; }

	_FORCE_INLINE_ void operator=(String&& p_str) { _cowdata = std::move(p_str._cowdata); }

	Vector<uint8_t> to_ascii_buffer() const;
	Vector<uint8_t> to_utf8_buffer() const;
	Vector<uint8_t> to_utf16_buffer() const;
	Vector<uint8_t> to_utf32_buffer() const;
	Vector<uint8_t> to_wchar_buffer() const;
	Vector<uint8_t> to_multibyte_char_buffer(const String& p_encoding = String()) const;

	// Constructors for NULL terminated C strings.
	String(const char* p_cstr) { append_latin1(p_cstr); }

	String(const wchar_t* p_cstr) { append_wstring(p_cstr); }

	String(const char32_t* p_cstr) { append_utf32(p_cstr); }

	// Copy assignment for NULL terminated C strings.
	void operator=(const char* p_cstr)
	{
		clear();
		append_latin1(p_cstr);
	}

	void operator=(const wchar_t* p_cstr)
	{
		clear();
		append_wstring(p_cstr);
	}

	void operator=(const char32_t* p_cstr)
	{
		clear();
		append_utf32(p_cstr);
	}
};

// Zero-constructing String initializes _cowdata.ptr() to nullptr and thus empty.
template <> struct is_zero_constructible<String> : std::true_type
{
};

bool operator==(const char* p_chr, const String& p_str);
bool operator==(const wchar_t* p_chr, const String& p_str);
bool operator!=(const char* p_chr, const String& p_str);
bool operator!=(const wchar_t* p_chr, const String& p_str);

String operator+(const char* p_chr, const String& p_str);
String operator+(const wchar_t* p_chr, const String& p_str);
String operator+(char32_t p_chr, const String& p_str);

String itos(int64_t p_val);
String uitos(uint64_t p_val);
String rtos(double p_val);
String rtoss(double p_val); // scientific version

struct NaturalNoCaseComparator
{
	bool operator()(const String& p_a, const String& p_b) const
	{
		return p_a.naturalnocasecmp_to(p_b) < 0;
	}
};

struct FileNoCaseComparator
{
	bool operator()(const String& p_a, const String& p_b) const
	{
		return p_a.filenocasecmp_to(p_b) < 0;
	}
};

/* end of namespace */

// Tool translate (TTR and variants) for the editor UI,
// and doc translate for the class reference (DTR).
#ifdef TOOLS_ENABLED
// Gets parsed.
String TTR(const String& p_text, const String& p_context = "");
String TTRN(
	const String& p_text, const String& p_text_plural, int p_n, const String& p_context = "");
String DTR(const String& p_text, const String& p_context = "");
String DTRN(
	const String& p_text, const String& p_text_plural, int p_n, const String& p_context = "");
// Use for C strings.
#define TTRC(m_value) (m_value)
// Use to avoid parsing (for use later with C strings).
#define TTRGET(m_value) TTR(m_value)

#else
#define TTRC(m_value) (m_value)
#define TTRGET(m_value) (m_value)
#endif

// Use this to mark property names for editor translation.
// Often for dynamic properties defined in _get_property_list().
#define PNAME(m_value) (m_value)

// Similar to PNAME, but to mark groups, i.e. properties with PROPERTY_USAGE_GROUP.
#define GNAME(m_value, m_prefix) (m_value)

// Runtime translate for the public node API.
String RTR(const String& p_text, const String& p_context = "");
String RTRN(
	const String& p_text, const String& p_text_plural, int p_n, const String& p_context = "");

_FORCE_INLINE_ String ETR(const String& p_text, const String& p_context = "") { return p_text; }

_FORCE_INLINE_ String ETRN(
	const String& p_text, const String& p_text_plural, int p_n, const String& p_context = "")
{
	if (p_n == 1) {
		return p_text;
	}
	return p_text_plural;
}

template <typename... P> _FORCE_INLINE_ Vector<String> sarray(P... p_args)
{
	return Vector<String>({String(p_args)...});
}

namespace vformat_detail
{

inline String to_str(const String& v) { return v; }

inline String to_str(const char* v) { return String(v ? v : ""); }

inline String to_str(char* v) { return String(v ? v : ""); }

inline String to_str(bool v) { return v ? "true" : "false"; }

inline String to_str(char v) { return String::chr(v); }

inline String to_str(int8_t v) { return itos(v); }

inline String to_str(uint8_t v) { return uitos(v); }

inline String to_str(int16_t v) { return itos(v); }

inline String to_str(uint16_t v) { return uitos(v); }

inline String to_str(int32_t v) { return itos(v); }

inline String to_str(uint32_t v) { return uitos(v); }

inline String to_str(int64_t v) { return itos(v); }

inline String to_str(uint64_t v) { return uitos(v); }

inline String to_str(float v) { return String::num_real(v); }

inline String to_str(double v) { return String::num_real(v); }

template <typename T>
inline auto to_str(const T& v) -> typename std::enable_if<std::is_enum<T>::value, String>::type
{
	return itos(static_cast<int64_t>(v));
}

template <typename T>
inline auto to_str(const T& v) ->
	typename std::enable_if<!std::is_enum<T>::value, decltype(String(v))>::type
{
	return String(v);
}

inline String dispatch(const String& p_fmt, const String* p_args, size_t p_count)
{
	String res;
	size_t arg_idx = 0;
	int len = p_fmt.length();

	for (int i = 0; i < len; ++i) {
		if (p_fmt[i] == '%' && i + 1 < len) {
			if (p_fmt[i + 1] == '%') {
				res += '%';
				i++;
				continue;
			}
			int j = i + 1;
			while (j < len && (p_fmt[j] == '.' || p_fmt[j] == '-' || p_fmt[j] == '+' ||
								  (p_fmt[j] >= '0' && p_fmt[j] <= '9'))) {
				j++;
			}
			if (j < len) {
				if (arg_idx < p_count) {
					res += p_args[arg_idx++];
				}
				i = j;
				continue;
			}
		}
		res += p_fmt[i];
	}
	return res;
}

} // namespace vformat_detail

template <typename... Args> _FORCE_INLINE_ String vformat(const String& p_fmt, Args&&... p_args)
{
	if constexpr (sizeof...(Args) == 0) {
		return p_fmt;
	}
	else {
		String args[sizeof...(Args)] = {vformat_detail::to_str(std::forward<Args>(p_args))...};
		return vformat_detail::dispatch(p_fmt, args, sizeof...(Args));
	}
}

#define SNAME(m_arg) String(m_arg)

// OEM encodings for Alt codes input.

inline constexpr char32_t alt_code_oem437[256] = {
	0x0000, 0x263a, 0x263b, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25d8, 0x25cb, 0x25d9, 0x2642, 0x2640, 0x266a, 0x266b, 0x263c,
	0x25ba, 0x25c4, 0x2195, 0x203c, 0x00b6, 0x00a7, 0x25ac, 0x21a8, 0x2191, 0x2193, 0x2192, 0x2190, 0x221f, 0x2194, 0x25b2, 0x25bc,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
	0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7, 0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
	0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9, 0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
	0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba, 0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
	0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
	0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f, 0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
	0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b, 0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
	0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03a4, 0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
	0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248, 0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
};

inline constexpr char32_t alt_code_cp1252[256] = {
	0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
	0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
	0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
	0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
	0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
	0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
	0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
	0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
	0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
	0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178,
	0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7, 0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
	0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
	0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7, 0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
	0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
	0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7, 0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
	0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7, 0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

#include <initializer_list>
#include <type_traits>

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

	template <typename C, typename... Args> void sort_custom(Args&&... p_args)
	{
		U len = count;
		if (len == 0) {
			return;
		}

		SortArray<T, C> sorter{p_args...};
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

class StringBuilder
{
	uint32_t string_length = 0;

	LocalVector<String> strings;
	LocalVector<const char*> c_strings;

	// -1 means it's a Godot String
	// a natural number means C string.
	LocalVector<int32_t> appended_strings;

public:
	StringBuilder& append(const String& p_string);
	StringBuilder& append(const char* p_cstring);

	_FORCE_INLINE_ StringBuilder& operator+(const String& p_string) { return append(p_string); }

	_FORCE_INLINE_ StringBuilder& operator+(const char* p_cstring) { return append(p_cstring); }

	_FORCE_INLINE_ void operator+=(const String& p_string) { append(p_string); }

	_FORCE_INLINE_ void operator+=(const char* p_cstring) { append(p_cstring); }

	_FORCE_INLINE_ int num_strings_appended() const { return appended_strings.size(); }

	_FORCE_INLINE_ uint32_t get_string_length() const { return string_length; }

	String as_string() const;

	_FORCE_INLINE_ operator String() const { return as_string(); }
};
