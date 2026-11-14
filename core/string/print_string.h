/**************************************************************************/
/*  print_string.h                                                        */
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

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "core/templates/span.h"

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

