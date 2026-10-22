/**************************************************************************/
/*  print_string.h                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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
/* The above copyright notice and this permission notice shall be          */
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

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
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

template <typename T> std::string to_string(const T& p_val)
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
			return "nullptr (0x0)";
		}
		std::ostringstream oss;
		oss << static_cast<const void*>(p_val);
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
		oss << p_val;
		return oss.str();
	}
	else if constexpr (is_streamable_v<Decayed>) {
		std::ostringstream oss;
		oss << p_val;
		return oss.str();
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

