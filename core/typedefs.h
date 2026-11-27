/**************************************************************************/
/*  typedefs.h                                                            */
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

/**
 * Basic definitions and simple functions to be used everywhere.
 */

// IWYU pragma: always_keep

// Ensure that C++ standard is at least C++17.
// If on MSVC, also ensures that the `Zc:__cplusplus` flag is present.
static_assert(__cplusplus >= 201703L, "Minimum of C++17 required.");

// IWYU pragma: begin_exports

// Include first in case the platform needs to pre-define/include some things.
#include "platform_config.h"

// Should be available everywhere.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
// --- Inlined/Folded from core/error/error_list.h ---
/** Error List. Please never compare an error against FAILED
 * Either do result != OK , or !result. This way, Error fail
 * values can be more detailed in the future.
 *
 * This is a generic error list, mainly for organizing a language of returning errors.
 *
 * Errors:
 * - Are added to the Error enum in core/error/error_list.h
 * - Have a description added to error_names in core/error/error_list.cpp
 * - Are bound with BIND_CORE_ENUM_CONSTANT() in core/core_constants.cpp
 * - Have a matching Android version in
 * platform/android/java/lib/src/main/java/org/godotengine/godot/error/Error.kt
 */

enum Error
{
	OK,						   // (0)
	FAILED,					   ///< Generic fail error
	ERR_UNAVAILABLE,		   ///< What is requested is unsupported/unavailable
	ERR_UNCONFIGURED,		   ///< The object being used hasn't been properly set up yet
	ERR_UNAUTHORIZED,		   ///< Missing credentials for requested resource
	ERR_PARAMETER_RANGE_ERROR, ///< Parameter given out of range (5)
	ERR_OUT_OF_MEMORY,		   ///< Out of memory
	ERR_FILE_NOT_FOUND,
	ERR_FILE_BAD_DRIVE,
	ERR_FILE_BAD_PATH,
	ERR_FILE_NO_PERMISSION, // (10)
	ERR_FILE_ALREADY_IN_USE,
	ERR_FILE_CANT_OPEN,
	ERR_FILE_CANT_WRITE,
	ERR_FILE_CANT_READ,
	ERR_FILE_UNRECOGNIZED, // (15)
	ERR_FILE_CORRUPT,
	ERR_FILE_MISSING_DEPENDENCIES,
	ERR_FILE_EOF,
	ERR_CANT_OPEN,	 ///< Can't open a resource/socket/file
	ERR_CANT_CREATE, // (20)
	ERR_QUERY_FAILED,
	ERR_ALREADY_IN_USE,
	ERR_LOCKED, ///< resource is locked
	ERR_TIMEOUT,
	ERR_CANT_CONNECT, // (25)
	ERR_CANT_RESOLVE,
	ERR_CONNECTION_ERROR,
	ERR_CANT_ACQUIRE_RESOURCE,
	ERR_CANT_FORK,
	ERR_INVALID_DATA,		 ///< Data passed is invalid (30)
	ERR_INVALID_PARAMETER,	 ///< Parameter passed is invalid
	ERR_ALREADY_EXISTS,		 ///< When adding, item already exists
	ERR_DOES_NOT_EXIST,		 ///< When retrieving/erasing, if item does not exist
	ERR_DATABASE_CANT_READ,	 ///< database is full
	ERR_DATABASE_CANT_WRITE, ///< database is full (35)
	ERR_COMPILATION_FAILED,
	ERR_METHOD_NOT_FOUND,
	ERR_LINK_FAILED,
	ERR_SCRIPT_FAILED,
	ERR_CYCLIC_LINK, // (40)
	ERR_INVALID_DECLARATION,
	ERR_DUPLICATE_SYMBOL,
	ERR_PARSE_ERROR,
	ERR_BUSY,
	ERR_SKIP, // (45)
	ERR_HELP, ///< user requested help!!
	ERR_BUG,  ///< a bug in the software certainly happened, due to a double check failing or
			  ///< unexpected behavior.
	ERR_PRINTER_ON_FIRE, /// the parallel port printer is engulfed in flames
	ERR_MAX,			 // Not being returned, value represents the number of errors
};

extern const char* error_names[];

// IWYU pragma: end_exports

#if defined(__has_feature)
#define VLTR_HAS_FEATURE(m_feature) __has_feature(m_feature)
#else
#define VLTR_HAS_FEATURE(m_feature) 0
#endif

#if defined(__has_cpp_attribute)
#define VLTR_HAS_CPP_ATTRIBUTE(m_feature) __has_cpp_attribute(m_feature)
#else
#define VLTR_HAS_CPP_ATTRIBUTE(m_feature) 0
#endif

#if (VLTR_HAS_FEATURE(address_sanitizer) || defined(__SANITIZE_ADDRESS__)) && !defined(ASAN_ENABLED)
#error Address sanitizer was enabled without defining `ASAN_ENABLED`
#endif

#if (VLTR_HAS_FEATURE(leak_sanitizer) || defined(__SANITIZE_LEAKS__)) && !defined(LSAN_ENABLED)
#error Leak sanitizer was enabled without defining `LSAN_ENABLED`
#endif

#if (VLTR_HAS_FEATURE(memory_sanitizer) || defined(__SANITIZE_MEMORY__)) && !defined(MSAN_ENABLED)
#error Memory sanitizer was enabled without defining `MSAN_ENABLED`
#endif

#if (VLTR_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)) && !defined(TSAN_ENABLED)
#error Thread sanitizer was enabled without defining `TSAN_ENABLED`
#endif

#if (VLTR_HAS_FEATURE(undefined_behavior_sanitizer) || defined(__UNDEFINED_SANITIZER__)) &&        \
	!defined(UBSAN_ENABLED)
#error Undefined behavior sanitizer was enabled without defining `UBSAN_ENABLED`
#endif

// Turn argument to string constant:
// https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html#Stringizing
#ifndef _STR
#define _STR(m_x) #m_x
#define _MKSTR(m_x) _STR(m_x)
#endif

// Should always inline no matter what.
#ifndef _ALWAYS_INLINE_
#if defined(__GNUC__)
#define _ALWAYS_INLINE_ __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define _ALWAYS_INLINE_ __forceinline
#else
#define _ALWAYS_INLINE_ inline
#endif
#endif

// Should always inline, except in dev builds because it makes debugging harder,
// or `size_enabled` builds where inlining is actively avoided.
#ifndef _FORCE_INLINE_
#if defined(DEV_ENABLED) || defined(SIZE_EXTRA)
#define _FORCE_INLINE_ inline
#else
#define _FORCE_INLINE_ _ALWAYS_INLINE_
#endif
#endif

// Should never inline.
#ifndef _NO_INLINE_
#if defined(__GNUC__)
#define _NO_INLINE_ __attribute__((noinline))
#elif defined(_MSC_VER)
#define _NO_INLINE_ __declspec(noinline)
#else
#define _NO_INLINE_
#endif
#endif

// In some cases [[nodiscard]] will get false positives,
// we can prevent the warning in specific cases by preceding the call with a cast.
#ifndef _ALLOW_DISCARD_
#define _ALLOW_DISCARD_ (void)
#endif

#if VLTR_HAS_CPP_ATTRIBUTE(clang::lifetimebound)
#define _LIFETIME_BOUND_ [[clang::lifetimebound]]
#elif VLTR_HAS_CPP_ATTRIBUTE(gnu::lifetimebound)
#define _LIFETIME_BOUND_ [[gnu::lifetimebound]]
#elif VLTR_HAS_CPP_ATTRIBUTE(msvc::lifetimebound)
#define _LIFETIME_BOUND_ [[msvc::lifetimebound]]
#elif VLTR_HAS_CPP_ATTRIBUTE(lifetimebound)
#define _LIFETIME_BOUND_ [[lifetimebound]]
#else
#define _LIFETIME_BOUND_
#endif

// Make room for our constexpr's below by overriding potential system-specific macros.
#undef SIGN
#undef MIN
#undef MAX
#undef CLAMP

template <typename T> constexpr const T SIGN(const T p_value)
{
	return p_value > 0 ? +1.0f : (p_value < 0 ? -1.0f : 0.0f);
}

template <typename T, typename T2> constexpr auto MIN(const T p_left, const T2 p_right)
{
	return p_left < p_right ? p_left : p_right;
}

template <typename T, typename T2> constexpr auto MAX(const T p_left, const T2 p_right)
{
	return p_left > p_right ? p_left : p_right;
}

template <typename T, typename T2, typename T3>
constexpr auto CLAMP(const T p_value, const T2 p_min, const T3 p_max)
{
	return p_value < p_min ? p_min : (p_value > p_max ? p_max : p_value);
}

// Like std::size, but without requiring any additional includes.
template <typename T, size_t SIZE> constexpr size_t std_size(const T (&)[SIZE]) { return SIZE; }

// Generic swap template.
#ifndef SWAP
#define SWAP(m_x, m_y) std::swap((m_x), (m_y))
#endif // SWAP

// Swap 16, 32 and 64 bits value for endianness.
#if defined(__GNUC__)
#define BSWAP16(x) __builtin_bswap16(x)
#define BSWAP32(x) __builtin_bswap32(x)
#define BSWAP64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#define BSWAP16(x) _byteswap_ushort(x)
#define BSWAP32(x) _byteswap_ulong(x)
#define BSWAP64(x) _byteswap_uint64(x)
#else
static inline uint16_t BSWAP16(uint16_t x) { return (x >> 8) | (x << 8); }

static inline uint32_t BSWAP32(uint32_t x)
{
	return ((x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24));
}

static inline uint64_t BSWAP64(uint64_t x)
{
	x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
	x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
	x = (x & 0x00FF00FF00FF00FF) << 8 | (x & 0xFF00FF00FF00FF00) >> 8;
	return x;
}
#endif

// Generic comparator used in Map, List, etc.
template <typename T> struct Comparator
{
	_ALWAYS_INLINE_ bool operator()(const T& p_a, const T& p_b) const { return (p_a < p_b); }
};

// Global lock macro, relies on the static Mutex::_global_mutex.
void _global_lock();
void _global_unlock();

struct _GlobalLock
{
	_GlobalLock() { _global_lock(); }

	~_GlobalLock() { _global_unlock(); }
};

#define GLOBAL_LOCK_FUNCTION _GlobalLock _global_lock_;

#if defined(__GNUC__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

#if defined(__GNUC__)
#define _PRINTF_FORMAT_ATTRIBUTE_2_0 __attribute__((format(printf, 2, 0)))
#define _PRINTF_FORMAT_ATTRIBUTE_2_3 __attribute__((format(printf, 2, 3)))
#else
#define _PRINTF_FORMAT_ATTRIBUTE_2_0
#define _PRINTF_FORMAT_ATTRIBUTE_2_3
#endif

// This is needed due to a strange OpenGL API that expects a pointer
// type for an argument that is actually an offset.
#define CAST_INT_TO_UCHAR_PTR(ptr) ((uint8_t*)(uintptr_t)(ptr))

// Home-made index sequence trick, so it can be used everywhere without the costly include of
// std::tuple.
// https://stackoverflow.com/questions/15014096/c-index-of-type-during-variadic-template-expansion
template <size_t... Is> struct IndexSequence
{
};

template <size_t N, size_t... Is>
struct BuildIndexSequence : BuildIndexSequence<N - 1, N - 1, Is...>
{
};

template <size_t... Is> struct BuildIndexSequence<0, Is...> : IndexSequence<Is...>
{
};

// Limit the depth of recursive algorithms when dealing with Array/Dictionary
#define MAX_RECURSION 100

// Macro VLTR_IS_DEFINED() allows to check if a macro is defined. It needs to be defined to anything
// (say 1) to work.
#define __VLTRARG_PLACEHOLDER_1 false,
#define __gd_take_second_arg(__ignored, val, ...) val
#define ____gd_is_defined(arg1_or_junk) __gd_take_second_arg(arg1_or_junk true, false)
#define ___gd_is_defined(val) ____gd_is_defined(__VLTRARG_PLACEHOLDER_##val)
#define VLTR_IS_DEFINED(x) ___gd_is_defined(x)

// Whether the default value of a type is just all-0 bytes.
// This can most commonly be exploited by using memset for these types instead of loop-construct.
// Trivially constructible types are also zero-constructible.
template <typename T>
inline constexpr bool is_zero_constructible_v = std::is_trivially_constructible_v<T>;

template <typename T>
inline constexpr bool is_zero_constructible_v<const T> = is_zero_constructible_v<T>;

template <typename T>
inline constexpr bool is_zero_constructible_v<volatile T> = is_zero_constructible_v<T>;

template <typename T>
inline constexpr bool is_zero_constructible_v<const volatile T> = is_zero_constructible_v<T>;

#if VLTR_HAS_CPP_ATTRIBUTE(gnu::warn_unused)
// https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Attributes.html#index-warn_005funused
#define _WARN_UNUSED_ [[gnu::warn_unused]]
#else
#define _WARN_UNUSED_
#endif

// Warning suppression helper macros.
#if defined(__clang__)
#define VLTR_CLANG_PRAGMA(m_content) _Pragma(#m_content)
#define VLTR_CLANG_WARNING_PUSH VLTR_CLANG_PRAGMA(clang diagnostic push)
#define VLTR_CLANG_WARNING_IGNORE(m_warning) VLTR_CLANG_PRAGMA(clang diagnostic ignored m_warning)
#define VLTR_CLANG_WARNING_POP VLTR_CLANG_PRAGMA(clang diagnostic pop)
#define VLTR_CLANG_WARNING_PUSH_AND_IGNORE(m_warning)                                              \
	VLTR_CLANG_WARNING_PUSH VLTR_CLANG_WARNING_IGNORE(m_warning)
#else
#define VLTR_CLANG_PRAGMA(m_content)
#define VLTR_CLANG_WARNING_PUSH
#define VLTR_CLANG_WARNING_IGNORE(m_warning)
#define VLTR_CLANG_WARNING_POP
#define VLTR_CLANG_WARNING_PUSH_AND_IGNORE(m_warning)
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define VLTR_GCC_PRAGMA(m_content) _Pragma(#m_content)
#define VLTR_GCC_WARNING_PUSH VLTR_GCC_PRAGMA(GCC diagnostic push)
#define VLTR_GCC_WARNING_IGNORE(m_warning) VLTR_GCC_PRAGMA(GCC diagnostic ignored m_warning)
#define VLTR_GCC_WARNING_POP VLTR_GCC_PRAGMA(GCC diagnostic pop)
#define VLTR_GCC_WARNING_PUSH_AND_IGNORE(m_warning)                                                \
	VLTR_GCC_WARNING_PUSH VLTR_GCC_WARNING_IGNORE(m_warning)
#else
#define VLTR_GCC_PRAGMA(m_content)
#define VLTR_GCC_WARNING_PUSH
#define VLTR_GCC_WARNING_IGNORE(m_warning)
#define VLTR_GCC_WARNING_POP
#define VLTR_GCC_WARNING_PUSH_AND_IGNORE(m_warning)
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define VLTR_MSVC_PRAGMA(m_command) __pragma(m_command)
#define VLTR_MSVC_WARNING_PUSH VLTR_MSVC_PRAGMA(warning(push))
#define VLTR_MSVC_WARNING_IGNORE(m_warning) VLTR_MSVC_PRAGMA(warning(disable : m_warning))
#define VLTR_MSVC_WARNING_POP VLTR_MSVC_PRAGMA(warning(pop))
#define VLTR_MSVC_WARNING_PUSH_AND_IGNORE(m_warning)

VLTR_MSVC_WARNING_PU
SH VLTR_MSVC_WARNING_IGNORE(m_warning)
#else
#define VLTR_MSVC_PRAGMA(m_command)
#define VLTR_MSVC_WARNING_PUSH
#define VLTR_MSVC_WARNING_IGNORE(m_warning)
#define VLTR_MSVC_WARNING_POP
#define VLTR_MSVC_WARNING_PUSH_AND_IGNORE(m_warning)
#endif

// Deprecation warning suppression helper macros.
#if defined(__clang__)
#define VLTR_PUSH_IGNORE_DEPRECATION()                                                             \
	VLTR_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
#define VLTR_POP_IGNORE_DEPRECATION() VLTR_CLANG_WARNING_POP
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define VLTR_PUSH_IGNORE_DEPRECATION() VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
#define VLTR_POP_IGNORE_DEPRECATION() VLTR_GCC_WARNING_POP
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define VLTR_PUSH_IGNORE_DEPRECATION() VLTR_MSVC_WARNING_PUSH_AND_IGNORE(4996)
#define VLTR_POP_IGNORE_DEPRECATION() VLTR_MSVC_WARNING_POP
#endif

	template <typename T, typename = void>
	struct is_fully_defined : std::false_type
{
};

template <typename T> struct is_fully_defined<T, std::void_t<decltype(sizeof(T))>> : std::true_type
{
};

template <typename T> constexpr bool is_fully_defined_v = is_fully_defined<T>::value;

#ifndef SCU_BUILD_ENABLED
/// Enforces the requirement that a class is not fully defined.
/// This can be used to reduce include coupling and keep compile times low.
/// The check must be made at the top of the corresponding .cpp file of a header.
#define STATIC_ASSERT_INCOMPLETE_TYPE(m_keyword, m_type)                                           \
	m_keyword m_type;                                                                              \
	static_assert(!is_fully_defined_v<m_type>, #m_type                                             \
		" was unexpectedly fully defined. Please check the include hierarchy of '" __FILE__        \
		"' and remove includes that resolve the " #m_keyword ".");
#else
#define STATIC_ASSERT_INCOMPLETE_TYPE(m_keyword, m_type)
#endif

#define _VLTR_VARNAME_CONCAT_B_(m_ignore, m_name) m_name
#define _VLTR_VARNAME_CONCAT_A_(m_a, m_b, m_c) _VLTR_VARNAME_CONCAT_B_(hello there, m_a##m_b##m_c)
#define _VLTR_VARNAME_CONCAT_(m_a, m_b, m_c) _VLTR_VARNAME_CONCAT_A_(m_a, m_b, m_c)
#define VLTR_UNIQUE_NAME(m_name) _VLTR_VARNAME_CONCAT_(m_name, _, __COUNTER__)

// --- Folded from core/error/error_macros.h ---


class String;

enum ErrorHandlerType
{
	ERR_HANDLER_ERROR,
	ERR_HANDLER_WARNING,
	ERR_HANDLER_SCRIPT,
	ERR_HANDLER_SHADER,
};

constexpr const char* _error_handler_type_string(ErrorHandlerType p_type)
{
	switch (p_type) {
	case ERR_HANDLER_ERROR:
		return "ERROR";
	case ERR_HANDLER_WARNING:
		return "WARNING";
	case ERR_HANDLER_SCRIPT:
		return "SCRIPT ERROR";
	case ERR_HANDLER_SHADER:
		return "SHADER ERROR";
	}
	return "UNKNOWN ERROR";
}

// Pointer to the error handler printing function. Reassign to any function to have errors printed.
// Parameters: userdata, function, file, line, error, explanation, type.
typedef void (*ErrorHandlerFunc)(void*, const char*, const char*, int p_line, const char*,
	const char*, bool p_editor_notify, ErrorHandlerType p_type);

struct ErrorHandlerList
{
	ErrorHandlerFunc errfunc = nullptr;
	void* userdata = nullptr;

	ErrorHandlerList* next = nullptr;
};

void add_error_handler(ErrorHandlerList* p_handler);
void remove_error_handler(const ErrorHandlerList* p_handler);

// Functions used by the error macros.
_NO_INLINE_ void _err_print_error(const char* p_function, const char* p_file, int p_line,
	const char* p_error, bool p_editor_notify = false, ErrorHandlerType p_type = ERR_HANDLER_ERROR);
_NO_INLINE_ void _err_print_error(const char* p_function, const char* p_file, int p_line,
	const String& p_error, bool p_editor_notify = false,
	ErrorHandlerType p_type = ERR_HANDLER_ERROR);
_NO_INLINE_ void _err_print_error(const char* p_function, const char* p_file, int p_line,
	const char* p_error, const char* p_message, bool p_editor_notify = false,
	ErrorHandlerType p_type = ERR_HANDLER_ERROR);
_NO_INLINE_ void _err_print_error(const char* p_function, const char* p_file, int p_line,
	const String& p_error, const char* p_message, bool p_editor_notify = false,
	ErrorHandlerType p_type = ERR_HANDLER_ERROR);
_NO_INLINE_ void _err_print_error(const char* p_function, const char* p_file, int p_line,
	const char* p_error, const String& p_message, bool p_editor_notify = false,
	ErrorHandlerType p_type = ERR_HANDLER_ERROR);
_NO_INLINE_ void _err_print_error(const char* p_function, const char* p_file, int p_line,
	const String& p_error, const String& p_message, bool p_editor_notify = false,
	ErrorHandlerType p_type = ERR_HANDLER_ERROR);
void _err_print_error_asap(const String& p_error, ErrorHandlerType p_type = ERR_HANDLER_ERROR);
_NO_INLINE_ void _err_print_index_error(const char* p_function, const char* p_file, int p_line,
	int64_t p_index, int64_t p_size, const char* p_index_str, const char* p_size_str,
	const char* p_message = "", bool p_editor_notify = false, bool p_fatal = false);
_NO_INLINE_ void _err_print_index_error(const char* p_function, const char* p_file, int p_line,
	int64_t p_index, int64_t p_size, const char* p_index_str, const char* p_size_str,
	const String& p_message, bool p_editor_notify = false, bool p_fatal = false);
_NO_INLINE_ void _err_flush_stdout();

#ifdef __GNUC__
// #define FUNCTION_STR __PRETTY_FUNCTION__ - too annoying
#define FUNCTION_STR __FUNCTION__
#else
#define FUNCTION_STR __FUNCTION__
#endif

#if defined(_MSC_VER) && !defined(__clang__)
/**
 * Don't use GENERATE_TRAP() directly, should only be used be the macros below.
 */
#define GENERATE_TRAP() __debugbreak()
#else
/**
 * Don't use GENERATE_TRAP() directly, should only be used be the macros below.
 */
#define GENERATE_TRAP() __builtin_trap()
#endif

/**
 * Error macros.
 * WARNING: These macros work in the opposite way to assert().
 *
 * Unlike exceptions and asserts, these macros try to maintain consistency and stability.
 * In most cases, bugs and/or invalid data are not fatal. They should never allow a perfectly
 * running application to fail or crash.
 * Always try to return processable data, so the engine can keep running well.
 * Use the _MSG versions to print a meaningful message to help with debugging.
 *
 * The `((void)0)` no-op statement is used as a trick to force us to put a semicolon after
 * those macros, making them look like proper statements.
 * The if wrappers are used to ensure that the macro replacement does not trigger unexpected
 * issues when expanded e.g. after an `if (cond) ERR_FAIL();` without braces.
 */

// Index out of bounds error macros.
// These macros should be used instead of `ERR_FAIL_COND` for bounds checking.

// Integer index out of bounds error macros.

/**
 * Try using `ERR_FAIL_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns.
 */
#define ERR_FAIL_INDEX(m_index, m_size)                                                            \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(                                                                    \
			FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size));       \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns.
 */
#define ERR_FAIL_INDEX_MSG(m_index, m_size, m_msg)                                                 \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg);                                                                  \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_INDEX_MSG` but also notifies the editor.
 */
#define ERR_FAIL_INDEX_EDMSG(m_index, m_size, m_msg)                                               \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg, true);                                                            \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the current function returns `m_retval`.
 */
#define ERR_FAIL_INDEX_V(m_index, m_size, m_retval)                                                \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(                                                                    \
			FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size));       \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the current function returns `m_retval`.
 */
#define ERR_FAIL_INDEX_V_MSG(m_index, m_size, m_retval, m_msg)                                     \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg);                                                                  \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_INDEX_V_MSG` but also notifies the editor.
 */
#define ERR_FAIL_INDEX_V_EDMSG(m_index, m_size, m_retval, m_msg)                                   \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg, true);                                                            \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_INDEX_MSG` or `ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable, and
 * there is no sensible error message.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, the application crashes.
 */
#define CRASH_BAD_INDEX(m_index, m_size)                                                           \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), "", false, true);                                                        \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_INDEX_MSG` or `ERR_FAIL_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable.
 *
 * Ensures an integer index `m_index` is less than `m_size` and greater than or equal to 0.
 * If not, prints `m_msg` and the application crashes.
 */
#define CRASH_BAD_INDEX_MSG(m_index, m_size, m_msg)                                                \
	if (unlikely((m_index) < 0 || (m_index) >= (m_size))) {                                        \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg, false, true);                                                     \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

// Unsigned integer index out of bounds error macros.

/**
 * Try using `ERR_FAIL_UNSIGNED_INDEX_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the current function returns.
 */
#define ERR_FAIL_UNSIGNED_INDEX(m_index, m_size)                                                   \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(                                                                    \
			FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size));       \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the current function returns.
 */
#define ERR_FAIL_UNSIGNED_INDEX_MSG(m_index, m_size, m_msg)                                        \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg);                                                                  \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_UNSIGNED_INDEX_MSG` but also notifies the editor.
 */
#define ERR_FAIL_UNSIGNED_INDEX_EDMSG(m_index, m_size, m_msg)                                      \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg, true);                                                            \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the current function returns `m_retval`.
 */
#define ERR_FAIL_UNSIGNED_INDEX_V(m_index, m_size, m_retval)                                       \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(                                                                    \
			FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index), _STR(m_size));       \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the current function returns `m_retval`.
 */
#define ERR_FAIL_UNSIGNED_INDEX_V_MSG(m_index, m_size, m_retval, m_msg)                            \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg);                                                                  \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_UNSIGNED_INDEX_V_MSG` but also notifies the editor.
 */
#define ERR_FAIL_UNSIGNED_INDEX_V_EDMSG(m_index, m_size, m_retval, m_msg)                          \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg, true);                                                            \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_UNSIGNED_INDEX_MSG` or `ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable, and
 * there is no sensible error message.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, the application crashes.
 */
#define CRASH_BAD_UNSIGNED_INDEX(m_index, m_size)                                                  \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), "", false, true);                                                        \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_UNSIGNED_INDEX_MSG` or `ERR_FAIL_UNSIGNED_INDEX_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable.
 *
 * Ensures an unsigned integer index `m_index` is less than `m_size`.
 * If not, prints `m_msg` and the application crashes.
 */
#define CRASH_BAD_UNSIGNED_INDEX_MSG(m_index, m_size, m_msg)                                       \
	if (unlikely((m_index) >= (m_size))) {                                                         \
		_err_print_index_error(FUNCTION_STR, __FILE__, __LINE__, m_index, m_size, _STR(m_index),   \
			_STR(m_size), m_msg, false, true);                                                     \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

// Null reference error macros.

/**
 * Try using `ERR_FAIL_NULL_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns.
 */
#define ERR_FAIL_NULL(m_param)                                                                     \
	if (unlikely(m_param == nullptr)) {                                                            \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.");         \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns.
 */
#define ERR_FAIL_NULL_MSG(m_param, m_msg)                                                          \
	if (unlikely(m_param == nullptr)) {                                                            \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg);  \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_NULL_MSG` but also notifies the editor.
 */
#define ERR_FAIL_NULL_EDMSG(m_param, m_msg)                                                        \
	if (unlikely(m_param == nullptr)) {                                                            \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Parameter \"" _STR(m_param) "\" is null.", m_msg, true);                              \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_NULL_V_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures a pointer `m_param` is not null.
 * If it is null, the current function returns `m_retval`.
 */
#define ERR_FAIL_NULL_V(m_param, m_retval)                                                         \
	if (unlikely(m_param == nullptr)) {                                                            \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.");         \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns `m_retval`.
 */
#define ERR_FAIL_NULL_V_MSG(m_param, m_retval, m_msg)                                              \
	if (unlikely(m_param == nullptr)) {                                                            \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg);  \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_NULL_V_MSG` but also notifies the editor.
 */
#define ERR_FAIL_NULL_V_EDMSG(m_param, m_retval, m_msg)                                            \
	if (unlikely(m_param == nullptr)) {                                                            \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Parameter \"" _STR(m_param) "\" is null.", m_msg, true);                              \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns.
 */
#define ERR_FAIL_COND(m_cond)                                                                      \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.");          \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns.
 *
 * If checking for null use ERR_FAIL_NULL_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_MSG instead.
 */
#define ERR_FAIL_COND_MSG(m_cond, m_msg)                                                           \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.", m_msg);   \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_COND_MSG` but also notifies the editor.
 */
#define ERR_FAIL_COND_EDMSG(m_cond, m_msg)                                                         \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true.", m_msg, true);                               \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible error message.
 * If checking for null use ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_V_MSG instead.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current function returns `m_retval`.
 */
#define ERR_FAIL_COND_V(m_cond, m_retval)                                                          \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval));                 \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns `m_retval`.
 *
 * If checking for null use ERR_FAIL_NULL_V_MSG instead.
 * If checking index bounds use ERR_FAIL_INDEX_V_MSG instead.
 */
#define ERR_FAIL_COND_V_MSG(m_cond, m_retval, m_msg)                                               \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg);          \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_COND_V_MSG` but also notifies the editor.
 */
#define ERR_FAIL_COND_V_EDMSG(m_cond, m_retval, m_msg)                                             \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Returning: " _STR(m_retval), m_msg, true);    \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_CONTINUE_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current loop continues.
 */
#define ERR_CONTINUE(m_cond)                                                                       \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Continuing.");                                \
		continue;                                                                                  \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current loop continues.
 */
#define ERR_CONTINUE_MSG(m_cond, m_msg)                                                            \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Continuing.", m_msg);                         \
		continue;                                                                                  \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_CONTINUE_MSG` but also notifies the editor.
 */
#define ERR_CONTINUE_EDMSG(m_cond, m_msg)                                                          \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Continuing.", m_msg, true);                   \
		continue;                                                                                  \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_BREAK_MSG`.
 * Only use this macro if there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the current loop breaks.
 */
#define ERR_BREAK(m_cond)                                                                          \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Breaking.");                                  \
		break;                                                                                     \
	} \
	else                                                                                         \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current loop breaks.
 */
#define ERR_BREAK_MSG(m_cond, m_msg)                                                               \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Breaking.", m_msg);                           \
		break;                                                                                     \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_BREAK_MSG` but also notifies the editor.
 */
#define ERR_BREAK_EDMSG(m_cond, m_msg)                                                             \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Condition \"" _STR(m_cond) "\" is true. Breaking.", m_msg, true);                     \
		break;                                                                                     \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_MSG` or `ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable, and
 * there is no sensible error message.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, the application crashes.
 */
#define CRASH_COND(m_cond)                                                                         \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "FATAL: Condition \"" _STR(m_cond) "\" is true.");   \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_MSG` or `ERR_FAIL_COND_V_MSG`.
 * Only use this macro if there is no sensible fallback i.e. the error is unrecoverable.
 *
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the application crashes.
 */
#define CRASH_COND_MSG(m_cond, m_msg)                                                              \
	if (unlikely(m_cond)) {                                                                        \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"FATAL: Condition \"" _STR(m_cond) "\" is true.", m_msg);                              \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

// Generic error macros.

/**
 * Try using `ERR_FAIL_COND_MSG` or `ERR_FAIL_MSG`.
 * Only use this macro if more complex error detection or recovery is required, and
 * there is no sensible error message.
 *
 * The current function returns.
 */
#define ERR_FAIL()                                                                                 \
	if (true) {                                                                                    \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__, "Method/function failed.");             \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns.
 */
#define ERR_FAIL_MSG(m_msg)                                                                        \
	if (true) {                                                                                    \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__, "Method/function failed.", m_msg);      \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_MSG` but also notifies the editor.
 */
#define ERR_FAIL_EDMSG(m_msg)                                                                      \
	if (true) {                                                                                    \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "Method/function failed.", m_msg, true);             \
		return;                                                                                    \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_V_MSG` or `ERR_FAIL_V_MSG`.
 * Only use this macro if more complex error detection or recovery is required, and
 * there is no sensible error message.
 *
 * The current function returns `m_retval`.
 */
#define ERR_FAIL_V(m_retval)                                                                       \
	if (true) {                                                                                    \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Method/function failed. Returning: " _STR(m_retval));                                 \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_V_MSG`.
 * Only use this macro if more complex error detection or recovery is required.
 *
 * Prints `m_msg`, and the current function returns `m_retval`.
 */
#define ERR_FAIL_V_MSG(m_retval, m_msg)                                                            \
	if (true) {                                                                                    \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Method/function failed. Returning: " _STR(m_retval), m_msg);                          \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_FAIL_V_MSG` but also notifies the editor.
 */
#define ERR_FAIL_V_EDMSG(m_retval, m_msg)                                                          \
	if (true) {                                                                                    \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"Method/function failed. Returning: " _STR(m_retval), m_msg, true);                    \
		return m_retval;                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Try using `ERR_FAIL_COND_MSG`, `ERR_FAIL_COND_V_MSG`, `ERR_CONTINUE_MSG` or `ERR_BREAK_MSG`.
 * Only use this macro at the start of a function that has not been implemented yet, or
 * if more complex error detection or recovery is required.
 *
 * Prints `m_msg`.
 */
#define ERR_PRINT(m_msg) _err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg)

/**
 * Same as `ERR_PRINT` but also notifies the editor.
 */
#define ERR_PRINT_ED(m_msg) _err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg, true)

/**
 * Prints `m_msg` once during the application lifetime.
 */
#define ERR_PRINT_ONCE(m_msg)                                                                      \
	if (true) {                                                                                    \
		static bool warning_shown = false;                                                         \
		if (unlikely(!warning_shown)) {                                                            \
			warning_shown = true;                                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg);                             \
		}                                                                                          \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `ERR_PRINT_ONCE` but also notifies the editor.
 */
#define ERR_PRINT_ONCE_ED(m_msg)                                                                   \
	if (true) {                                                                                    \
		static bool warning_shown = false;                                                         \
		if (unlikely(!warning_shown)) {                                                            \
			warning_shown = true;                                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg, true);                       \
		}                                                                                          \
	} \
	else                                                                                         \
		((void)0)

// Print warning message macros.

/**
 * Prints `m_msg`.
 *
 * If warning about deprecated usage, use `WARN_DEPRECATED` or `WARN_DEPRECATED_MSG` instead.
 */
#define WARN_PRINT(m_msg)                                                                          \
	_err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg, false, ERR_HANDLER_WARNING)

/**
 * Same as `WARN_PRINT` but also notifies the editor.
 */
#define WARN_PRINT_ED(m_msg)                                                                       \
	_err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg, true, ERR_HANDLER_WARNING)

/**
 * Prints `m_msg` once during the application lifetime.
 *
 * If warning about deprecated usage, use `WARN_DEPRECATED` or `WARN_DEPRECATED_MSG` instead.
 */
#define WARN_PRINT_ONCE(m_msg)                                                                     \
	if (true) {                                                                                    \
		static bool warning_shown = false;                                                         \
		if (unlikely(!warning_shown)) {                                                            \
			warning_shown = true;                                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg, false, ERR_HANDLER_WARNING); \
		}                                                                                          \
	} \
	else                                                                                         \
		((void)0)

/**
 * Same as `WARN_PRINT_ONCE` but also notifies the editor.
 */
#define WARN_PRINT_ONCE_ED(m_msg)                                                                  \
	if (true) {                                                                                    \
		static bool warning_shown = false;                                                         \
		if (unlikely(!warning_shown)) {                                                            \
			warning_shown = true;                                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__, m_msg, true, ERR_HANDLER_WARNING);  \
		}                                                                                          \
	} \
	else                                                                                         \
		((void)0)

/**
 * Warns about `m_msg` only when verbose mode is enabled.
 */
#define WARN_VERBOSE(m_msg)                                                                        \
	{                                                                                              \
		if (is_print_verbose_enabled()) {                                                          \
			WARN_PRINT(m_msg);                                                                     \
		}                                                                                          \
	}

// Print deprecated warning message macros.

/**
 * Warns that the current function is deprecated.
 */
#define WARN_DEPRECATED                                                                            \
	if (true) {                                                                                    \
		static bool warning_shown = false;                                                         \
		if (unlikely(!warning_shown)) {                                                            \
			warning_shown = true;                                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                     \
				"This method has been deprecated and will be removed in the future.", false,       \
				ERR_HANDLER_WARNING);                                                              \
		}                                                                                          \
	} \
	else                                                                                         \
		((void)0)

/**
 * Warns that the current function is deprecated and prints `m_msg`.
 */
#define WARN_DEPRECATED_MSG(m_msg)                                                                 \
	if (true) {                                                                                    \
		static bool warning_shown = false;                                                         \
		if (unlikely(!warning_shown)) {                                                            \
			warning_shown = true;                                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                     \
				"This method has been deprecated and will be removed in the future.", m_msg,       \
				false, ERR_HANDLER_WARNING);                                                       \
		}                                                                                          \
	} \
	else                                                                                         \
		((void)0)

/**
 * Do not use.
 * If the application should never reach this point use CRASH_NOW_MSG(m_msg) to explain why.
 *
 * The application crashes.
 */
#define CRASH_NOW()                                                                                \
	if (true) {                                                                                    \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__, "FATAL: Method/function failed.");      \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Only use if the application should never reach this point.
 *
 * Prints `m_msg`, and then the application crashes.
 */
#define CRASH_NOW_MSG(m_msg)                                                                       \
	if (true) {                                                                                    \
		_err_print_error(                                                                          \
			FUNCTION_STR, __FILE__, __LINE__, "FATAL: Method/function failed.", m_msg);            \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else                                                                                         \
		((void)0)

/**
 * Note: IN MOST CASES YOU SHOULD NOT USE THIS MACRO.
 * Do not use unless you understand the trade-offs.
 *
 * DEV macros will be compiled out in releases, they are wrapped in DEV_ENABLED.
 *
 * Prefer WARNINGS / ERR_FAIL macros (which fail without crashing) - ERR_FAIL should be used in most
 * cases. Then CRASH_NOW_MSG macros (on rare occasions where error cannot be recovered).
 *
 * DEV_ASSERT should generally only be used when both of the following conditions are met:
 * 1) Bottleneck code where a check in release would be too expensive.
 * 2) Situations where the check would fail obviously and straight away during the maintenance of
 * the code (i.e. strict conditions that should be true no matter what) and that can't fail for
 * other contributors once the code is finish edand merged.
 */
#ifdef DEV_ENABLED
#define DEV_ASSERT(m_cond)                                                                         \
	if (unlikely(!(m_cond))) {                                                                     \
		_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                         \
			"FATAL: DEV_ASSERT failed  \"" _STR(m_cond) "\" is false.");                           \
		_err_flush_stdout();                                                                       \
		GENERATE_TRAP();                                                                           \
	} \
	else ((void)0)
#else
#define DEV_ASSERT(m_cond)
#endif

#ifdef DEV_ENABLED
#define DEV_CHECK_ONCE(m_cond)                                                                     \
	if (true) {                                                                                    \
		static bool first_print = true;                                                            \
		if (first_print && unlikely(!(m_cond))) {                                                  \
			_err_print_error(FUNCTION_STR, __FILE__, __LINE__,                                     \
				"DEV_CHECK_ONCE failed  \"" _STR(m_cond) "\" is false.");                          \
			first_print = false;                                                                   \
		}                                                                                          \
	} \
	else((void)0)
#else
#define DEV_CHECK_ONCE(m_cond)
#endif

/**
 * Physics Interpolation warnings.
 * These are spam protection warnings.
 */
#define PHYSICS_INTERPOLATION_NODE_WARNING(m_object_id, m_string)                                  \
	_physics_interpolation_warning(FUNCTION_STR, __FILE__, __LINE__, m_object_id, m_string)

#define PHYSICS_INTERPOLATION_WARNING(m_string)                                                    \
	_physics_interpolation_warning(FUNCTION_STR, __FILE__, __LINE__, ObjectID(UINT64_MAX), m_string)
// --- End core/error/error_macros.h ---

// --- Folded from core/math/math_defs.h ---
#include <limits>


namespace Math
{
inline constexpr double SQRT2 = 1.4142135623730950488016887242;
inline constexpr double SQRT3 = 1.7320508075688772935274463415059;
inline constexpr double SQRT12 = 0.7071067811865475244008443621048490;
inline constexpr double SQRT13 = 0.57735026918962576450914878050196;
inline constexpr double LN2 = 0.6931471805599453094172321215;
inline constexpr double TAU = 6.2831853071795864769252867666;
inline constexpr double PI = 3.1415926535897932384626433833;
inline constexpr double E = 2.7182818284590452353602874714;
inline constexpr double INF = std::numeric_limits<double>::infinity();
inline constexpr double NaN = std::numeric_limits<double>::quiet_NaN();
} // namespace Math

inline constexpr double CMP_EPSILON = 0.00001;
inline constexpr double CMP_EPSILON2 = CMP_EPSILON * CMP_EPSILON;

inline constexpr double CMP_NORMALIZE_TOLERANCE = 0.000001;
inline constexpr double CMP_POINT_IN_PLANE_EPSILON = 0.00001;

#ifdef DEBUG_ENABLED
#define MATH_CHECKS
#endif

// this epsilon is for values related to a unit size (scalar or vector len)
#ifdef PRECISE_MATH_CHECKS
inline constexpr double UNIT_EPSILON = 0.00001;
#else
// tolerate some more floating point error normally
inline constexpr double UNIT_EPSILON = 0.001;
#endif

#define USEC_TO_SEC(m_usec) ((m_usec) / 1000000.0)

enum ClockDirection
{
	CLOCKWISE,
	COUNTERCLOCKWISE
};

enum Orientation
{
	HORIZONTAL,
	VERTICAL
};

enum HorizontalAlignment
{
	HORIZONTAL_ALIGNMENT_LEFT,
	HORIZONTAL_ALIGNMENT_CENTER,
	HORIZONTAL_ALIGNMENT_RIGHT,
	HORIZONTAL_ALIGNMENT_FILL,
};

enum VerticalAlignment
{
	VERTICAL_ALIGNMENT_TOP,
	VERTICAL_ALIGNMENT_CENTER,
	VERTICAL_ALIGNMENT_BOTTOM,
	VERTICAL_ALIGNMENT_FILL,
};

enum InlineAlignment
{
	// Image alignment points.
	INLINE_ALIGNMENT_TOP_TO = 0b0000,
	INLINE_ALIGNMENT_CENTER_TO = 0b0001,
	INLINE_ALIGNMENT_BASELINE_TO = 0b0011,
	INLINE_ALIGNMENT_BOTTOM_TO = 0b0010,
	INLINE_ALIGNMENT_IMAGE_MASK = 0b0011,

	// Text alignment points.
	INLINE_ALIGNMENT_TO_TOP = 0b0000,
	INLINE_ALIGNMENT_TO_CENTER = 0b0100,
	INLINE_ALIGNMENT_TO_BASELINE = 0b1000,
	INLINE_ALIGNMENT_TO_BOTTOM = 0b1100,
	INLINE_ALIGNMENT_TEXT_MASK = 0b1100,

	// Presets.
	INLINE_ALIGNMENT_TOP = INLINE_ALIGNMENT_TOP_TO | INLINE_ALIGNMENT_TO_TOP,
	INLINE_ALIGNMENT_CENTER = INLINE_ALIGNMENT_CENTER_TO | INLINE_ALIGNMENT_TO_CENTER,
	INLINE_ALIGNMENT_BOTTOM = INLINE_ALIGNMENT_BOTTOM_TO | INLINE_ALIGNMENT_TO_BOTTOM
};

enum Side
{
	SIDE_LEFT,
	SIDE_TOP,
	SIDE_RIGHT,
	SIDE_BOTTOM
};

enum Corner
{
	CORNER_TOP_LEFT,
	CORNER_TOP_RIGHT,
	CORNER_BOTTOM_RIGHT,
	CORNER_BOTTOM_LEFT
};

enum class EulerOrder
{
	XYZ,
	XZY,
	YXZ,
	YZX,
	ZXY,
	ZYX
};

/**
 * The "Real" type is an abstract type used for real numbers, such as 1.5,
 * in contrast to integer numbers. Precision can be controlled with the
 * presence or absence of the REAL_T_IS_DOUBLE define.
 */
#ifdef REAL_T_IS_DOUBLE
typedef double real_t;
#else
typedef float real_t;
#endif

/**
 * Rarely, there will be a scenario where a function/variable expects one of the builtin integral
 *  types that do NOT utilize the fixed-width constants. In practice, the only discrepancies are
 *  with `long` or `long long` (and their unsigned equivalents) not being declared when most/all
 *  other integral constants are. We'll account for this with `int_alt_t` and `uint_alt_t`,
 *  which assign to the unused fixed-width slot. As this will only be used rarely, keep the types
 *  scoped to `Math` instead of the global namespace.
 */
namespace Math
{
using int_alt_t = std::conditional_t<std::is_same_v<int64_t, long>, long long, long>;
using uint_alt_t =
	std::conditional_t<std::is_same_v<uint64_t, unsigned long>, unsigned long long, unsigned long>;
} // namespace Math
// --- End core/math/math_defs.h ---

// --- Folded from core/math/math_funcs.h ---




#include <cmath>

namespace Math {

_ALWAYS_INLINE_ double sin(double p_x) {
	return std::sin(p_x);
}
_ALWAYS_INLINE_ float sin(float p_x) {
	return std::sin(p_x);
}

_ALWAYS_INLINE_ double cos(double p_x) {
	return std::cos(p_x);
}
_ALWAYS_INLINE_ float cos(float p_x) {
	return std::cos(p_x);
}

_ALWAYS_INLINE_ double tan(double p_x) {
	return std::tan(p_x);
}
_ALWAYS_INLINE_ float tan(float p_x) {
	return std::tan(p_x);
}

_ALWAYS_INLINE_ double sinh(double p_x) {
	return std::sinh(p_x);
}
_ALWAYS_INLINE_ float sinh(float p_x) {
	return std::sinh(p_x);
}

_ALWAYS_INLINE_ double sinc(double p_x) {
	return p_x == 0 ? 1 : sin(p_x) / p_x;
}
_ALWAYS_INLINE_ float sinc(float p_x) {
	return p_x == 0 ? 1 : sin(p_x) / p_x;
}

_ALWAYS_INLINE_ double sincn(double p_x) {
	return sinc(PI * p_x);
}
_ALWAYS_INLINE_ float sincn(float p_x) {
	return sinc((float)PI * p_x);
}

_ALWAYS_INLINE_ double cosh(double p_x) {
	return std::cosh(p_x);
}
_ALWAYS_INLINE_ float cosh(float p_x) {
	return std::cosh(p_x);
}

_ALWAYS_INLINE_ double tanh(double p_x) {
	return std::tanh(p_x);
}
_ALWAYS_INLINE_ float tanh(float p_x) {
	return std::tanh(p_x);
}

// Always does clamping so always safe to use.
_ALWAYS_INLINE_ double asin(double p_x) {
	return p_x < -1 ? (-PI / 2) : (p_x > 1 ? (PI / 2) : std::asin(p_x));
}
_ALWAYS_INLINE_ float asin(float p_x) {
	return p_x < -1 ? (-(float)PI / 2) : (p_x > 1 ? ((float)PI / 2) : std::asin(p_x));
}

// Always does clamping so always safe to use.
_ALWAYS_INLINE_ double acos(double p_x) {
	return p_x < -1 ? PI : (p_x > 1 ? 0 : std::acos(p_x));
}
_ALWAYS_INLINE_ float acos(float p_x) {
	return p_x < -1 ? (float)PI : (p_x > 1 ? 0 : std::acos(p_x));
}

_ALWAYS_INLINE_ double atan(double p_x) {
	return std::atan(p_x);
}
_ALWAYS_INLINE_ float atan(float p_x) {
	return std::atan(p_x);
}

_ALWAYS_INLINE_ double atan2(double p_y, double p_x) {
	return std::atan2(p_y, p_x);
}
_ALWAYS_INLINE_ float atan2(float p_y, float p_x) {
	return std::atan2(p_y, p_x);
}

_ALWAYS_INLINE_ double asinh(double p_x) {
	return std::asinh(p_x);
}
_ALWAYS_INLINE_ float asinh(float p_x) {
	return std::asinh(p_x);
}

// Always does clamping so always safe to use.
_ALWAYS_INLINE_ double acosh(double p_x) {
	return p_x < 1 ? 0 : std::acosh(p_x);
}
_ALWAYS_INLINE_ float acosh(float p_x) {
	return p_x < 1 ? 0 : std::acosh(p_x);
}

// Always does clamping so always safe to use.
_ALWAYS_INLINE_ double atanh(double p_x) {
	return p_x <= -1 ? -INF : (p_x >= 1 ? INF : std::atanh(p_x));
}
_ALWAYS_INLINE_ float atanh(float p_x) {
	return p_x <= -1 ? (float)-INF : (p_x >= 1 ? (float)INF : std::atanh(p_x));
}

_ALWAYS_INLINE_ double sqrt(double p_x) {
	return std::sqrt(p_x);
}
_ALWAYS_INLINE_ float sqrt(float p_x) {
	return std::sqrt(p_x);
}

_ALWAYS_INLINE_ double fmod(double p_x, double p_y) {
	return std::fmod(p_x, p_y);
}
_ALWAYS_INLINE_ float fmod(float p_x, float p_y) {
	return std::fmod(p_x, p_y);
}

_ALWAYS_INLINE_ double modf(double p_x, double *r_y) {
	return std::modf(p_x, r_y);
}
_ALWAYS_INLINE_ float modf(float p_x, float *r_y) {
	return std::modf(p_x, r_y);
}

_ALWAYS_INLINE_ double floor(double p_x) {
	return std::floor(p_x);
}
_ALWAYS_INLINE_ float floor(float p_x) {
	return std::floor(p_x);
}

_ALWAYS_INLINE_ double ceil(double p_x) {
	return std::ceil(p_x);
}
_ALWAYS_INLINE_ float ceil(float p_x) {
	return std::ceil(p_x);
}

_ALWAYS_INLINE_ double pow(double p_x, double p_y) {
	return std::pow(p_x, p_y);
}
_ALWAYS_INLINE_ float pow(float p_x, float p_y) {
	return std::pow(p_x, p_y);
}

_ALWAYS_INLINE_ double log(double p_x) {
	return std::log(p_x);
}
_ALWAYS_INLINE_ float log(float p_x) {
	return std::log(p_x);
}

_ALWAYS_INLINE_ double log1p(double p_x) {
	return std::log1p(p_x);
}
_ALWAYS_INLINE_ float log1p(float p_x) {
	return std::log1p(p_x);
}

_ALWAYS_INLINE_ double log2(double p_x) {
	return std::log2(p_x);
}
_ALWAYS_INLINE_ float log2(float p_x) {
	return std::log2(p_x);
}

_ALWAYS_INLINE_ double exp(double p_x) {
	return std::exp(p_x);
}
_ALWAYS_INLINE_ float exp(float p_x) {
	return std::exp(p_x);
}

constexpr double abs(double p_value) {
	return p_value == 0 ? 0.0 : (p_value < 0 ? -p_value : p_value);
}
constexpr float abs(float p_value) {
	return p_value == 0 ? 0.0 : (p_value < 0 ? -p_value : p_value);
}
constexpr int8_t abs(int8_t p_value) {
	return p_value > 0 ? p_value : -p_value;
}
constexpr int16_t abs(int16_t p_value) {
	return p_value > 0 ? p_value : -p_value;
}
constexpr int32_t abs(int32_t p_value) {
	return p_value > 0 ? p_value : -p_value;
}
constexpr int64_t abs(int64_t p_value) {
	return p_value > 0 ? p_value : -p_value;
}

constexpr bool is_nan(double p_val) {
	return p_val != p_val;
}
constexpr bool is_nan(float p_val) {
	return p_val != p_val;
}

constexpr bool is_inf(double p_val) {
	return abs(p_val) == INF;
}
constexpr bool is_inf(float p_val) {
	return abs(p_val) == (float)INF;
}

// These methods assume (p_num + p_den) doesn't overflow.
constexpr int32_t division_round_up(int32_t p_num, int32_t p_den) {
	int32_t offset = (p_num < 0 && p_den < 0) ? 1 : -1;
	return (p_num + p_den + offset) / p_den;
}
constexpr uint32_t division_round_up(uint32_t p_num, uint32_t p_den) {
	return (p_num + p_den - 1) / p_den;
}
constexpr int64_t division_round_up(int64_t p_num, int64_t p_den) {
	int32_t offset = (p_num < 0 && p_den < 0) ? 1 : -1;
	return (p_num + p_den + offset) / p_den;
}
constexpr uint64_t division_round_up(uint64_t p_num, uint64_t p_den) {
	return (p_num + p_den - 1) / p_den;
}

constexpr bool is_finite(double p_val) {
	return !is_nan(p_val) && !is_inf(p_val);
}
constexpr bool is_finite(float p_val) {
	return !is_nan(p_val) && !is_inf(p_val);
}

_ALWAYS_INLINE_ double fposmod(double p_x, double p_y) {
	double value = fmod(p_x, p_y);
	if (((value < 0) && (p_y > 0)) || ((value > 0) && (p_y < 0))) {
		value += p_y;
	}
	value += 0.0;
	return value;
}
_ALWAYS_INLINE_ float fposmod(float p_x, float p_y) {
	float value = fmod(p_x, p_y);
	if (((value < 0) && (p_y > 0)) || ((value > 0) && (p_y < 0))) {
		value += p_y;
	}
	value += 0.0f;
	return value;
}

_ALWAYS_INLINE_ double fposmodp(double p_x, double p_y) {
	double value = fmod(p_x, p_y);
	if (value < 0) {
		value += p_y;
	}
	value += 0.0;
	return value;
}
_ALWAYS_INLINE_ float fposmodp(float p_x, float p_y) {
	float value = fmod(p_x, p_y);
	if (value < 0) {
		value += p_y;
	}
	value += 0.0f;
	return value;
}

_ALWAYS_INLINE_ int64_t posmod(int64_t p_x, int64_t p_y) {
	ERR_FAIL_COND_V_MSG(p_y == 0, 0, "Division by zero in posmod is undefined. Returning 0 as fallback.");
	int64_t value = p_x % p_y;
	if (((value < 0) && (p_y > 0)) || ((value > 0) && (p_y < 0))) {
		value += p_y;
	}
	return value;
}

constexpr double deg_to_rad(double p_y) {
	return p_y * (PI / 180.0);
}
constexpr float deg_to_rad(float p_y) {
	return p_y * ((float)PI / 180.0f);
}

constexpr double rad_to_deg(double p_y) {
	return p_y * (180.0 / PI);
}
constexpr float rad_to_deg(float p_y) {
	return p_y * (180.0f / (float)PI);
}

constexpr double lerp(double p_from, double p_to, double p_weight) {
	return p_from + (p_to - p_from) * p_weight;
}
constexpr float lerp(float p_from, float p_to, float p_weight) {
	return p_from + (p_to - p_from) * p_weight;
}

constexpr double cubic_interpolate(double p_from, double p_to, double p_pre, double p_post, double p_weight) {
	return 0.5 *
			((p_from * 2.0) +
					(-p_pre + p_to) * p_weight +
					(2.0 * p_pre - 5.0 * p_from + 4.0 * p_to - p_post) * (p_weight * p_weight) +
					(-p_pre + 3.0 * p_from - 3.0 * p_to + p_post) * (p_weight * p_weight * p_weight));
}
constexpr float cubic_interpolate(float p_from, float p_to, float p_pre, float p_post, float p_weight) {
	return 0.5f *
			((p_from * 2.0f) +
					(-p_pre + p_to) * p_weight +
					(2.0f * p_pre - 5.0f * p_from + 4.0f * p_to - p_post) * (p_weight * p_weight) +
					(-p_pre + 3.0f * p_from - 3.0f * p_to + p_post) * (p_weight * p_weight * p_weight));
}

_ALWAYS_INLINE_ double cubic_interpolate_angle(double p_from, double p_to, double p_pre, double p_post, double p_weight) {
	double from_rot = fmod(p_from, TAU);

	double pre_diff = fmod(p_pre - from_rot, TAU);
	double pre_rot = from_rot + fmod(2.0 * pre_diff, TAU) - pre_diff;

	double to_diff = fmod(p_to - from_rot, TAU);
	double to_rot = from_rot + fmod(2.0 * to_diff, TAU) - to_diff;

	double post_diff = fmod(p_post - to_rot, TAU);
	double post_rot = to_rot + fmod(2.0 * post_diff, TAU) - post_diff;

	return cubic_interpolate(from_rot, to_rot, pre_rot, post_rot, p_weight);
}

_ALWAYS_INLINE_ float cubic_interpolate_angle(float p_from, float p_to, float p_pre, float p_post, float p_weight) {
	float from_rot = fmod(p_from, (float)TAU);

	float pre_diff = fmod(p_pre - from_rot, (float)TAU);
	float pre_rot = from_rot + fmod(2.0f * pre_diff, (float)TAU) - pre_diff;

	float to_diff = fmod(p_to - from_rot, (float)TAU);
	float to_rot = from_rot + fmod(2.0f * to_diff, (float)TAU) - to_diff;

	float post_diff = fmod(p_post - to_rot, (float)TAU);
	float post_rot = to_rot + fmod(2.0f * post_diff, (float)TAU) - post_diff;

	return cubic_interpolate(from_rot, to_rot, pre_rot, post_rot, p_weight);
}

constexpr double cubic_interpolate_in_time(double p_from, double p_to, double p_pre, double p_post, double p_weight,
		double p_to_t, double p_pre_t, double p_post_t) {
	/* Barry-Goldman method */
	double t = lerp(0.0, p_to_t, p_weight);
	double a1 = lerp(p_pre, p_from, p_pre_t == 0 ? 0.0 : (t - p_pre_t) / -p_pre_t);
	double a2 = lerp(p_from, p_to, p_to_t == 0 ? 0.5 : t / p_to_t);
	double a3 = lerp(p_to, p_post, p_post_t - p_to_t == 0 ? 1.0 : (t - p_to_t) / (p_post_t - p_to_t));
	double b1 = lerp(a1, a2, p_to_t - p_pre_t == 0 ? 0.0 : (t - p_pre_t) / (p_to_t - p_pre_t));
	double b2 = lerp(a2, a3, p_post_t == 0 ? 1.0 : t / p_post_t);
	return lerp(b1, b2, p_to_t == 0 ? 0.5 : t / p_to_t);
}
constexpr float cubic_interpolate_in_time(float p_from, float p_to, float p_pre, float p_post, float p_weight,
		float p_to_t, float p_pre_t, float p_post_t) {
	/* Barry-Goldman method */
	float t = lerp(0.0f, p_to_t, p_weight);
	float a1 = lerp(p_pre, p_from, p_pre_t == 0 ? 0.0f : (t - p_pre_t) / -p_pre_t);
	float a2 = lerp(p_from, p_to, p_to_t == 0 ? 0.5f : t / p_to_t);
	float a3 = lerp(p_to, p_post, p_post_t - p_to_t == 0 ? 1.0f : (t - p_to_t) / (p_post_t - p_to_t));
	float b1 = lerp(a1, a2, p_to_t - p_pre_t == 0 ? 0.0f : (t - p_pre_t) / (p_to_t - p_pre_t));
	float b2 = lerp(a2, a3, p_post_t == 0 ? 1.0f : t / p_post_t);
	return lerp(b1, b2, p_to_t == 0 ? 0.5f : t / p_to_t);
}

_ALWAYS_INLINE_ double cubic_interpolate_angle_in_time(double p_from, double p_to, double p_pre, double p_post, double p_weight,
		double p_to_t, double p_pre_t, double p_post_t) {
	double from_rot = fmod(p_from, TAU);

	double pre_diff = fmod(p_pre - from_rot, TAU);
	double pre_rot = from_rot + fmod(2.0 * pre_diff, TAU) - pre_diff;

	double to_diff = fmod(p_to - from_rot, TAU);
	double to_rot = from_rot + fmod(2.0 * to_diff, TAU) - to_diff;

	double post_diff = fmod(p_post - to_rot, TAU);
	double post_rot = to_rot + fmod(2.0 * post_diff, TAU) - post_diff;

	return cubic_interpolate_in_time(from_rot, to_rot, pre_rot, post_rot, p_weight, p_to_t, p_pre_t, p_post_t);
}
_ALWAYS_INLINE_ float cubic_interpolate_angle_in_time(float p_from, float p_to, float p_pre, float p_post, float p_weight,
		float p_to_t, float p_pre_t, float p_post_t) {
	float from_rot = fmod(p_from, (float)TAU);

	float pre_diff = fmod(p_pre - from_rot, (float)TAU);
	float pre_rot = from_rot + fmod(2.0f * pre_diff, (float)TAU) - pre_diff;

	float to_diff = fmod(p_to - from_rot, (float)TAU);
	float to_rot = from_rot + fmod(2.0f * to_diff, (float)TAU) - to_diff;

	float post_diff = fmod(p_post - to_rot, (float)TAU);
	float post_rot = to_rot + fmod(2.0f * post_diff, (float)TAU) - post_diff;

	return cubic_interpolate_in_time(from_rot, to_rot, pre_rot, post_rot, p_weight, p_to_t, p_pre_t, p_post_t);
}

constexpr double bezier_interpolate(double p_start, double p_control_1, double p_control_2, double p_end, double p_t) {
	/* Formula from Wikipedia article on Bezier curves. */
	double omt = (1.0 - p_t);
	double omt2 = omt * omt;
	double omt3 = omt2 * omt;
	double t2 = p_t * p_t;
	double t3 = t2 * p_t;

	return p_start * omt3 + p_control_1 * omt2 * p_t * 3.0 + p_control_2 * omt * t2 * 3.0 + p_end * t3;
}
constexpr float bezier_interpolate(float p_start, float p_control_1, float p_control_2, float p_end, float p_t) {
	/* Formula from Wikipedia article on Bezier curves. */
	float omt = (1.0f - p_t);
	float omt2 = omt * omt;
	float omt3 = omt2 * omt;
	float t2 = p_t * p_t;
	float t3 = t2 * p_t;

	return p_start * omt3 + p_control_1 * omt2 * p_t * 3.0f + p_control_2 * omt * t2 * 3.0f + p_end * t3;
}

constexpr double bezier_derivative(double p_start, double p_control_1, double p_control_2, double p_end, double p_t) {
	/* Formula from Wikipedia article on Bezier curves. */
	double omt = (1.0 - p_t);
	double omt2 = omt * omt;
	double t2 = p_t * p_t;

	double d = (p_control_1 - p_start) * 3.0 * omt2 + (p_control_2 - p_control_1) * 6.0 * omt * p_t + (p_end - p_control_2) * 3.0 * t2;
	return d;
}
constexpr float bezier_derivative(float p_start, float p_control_1, float p_control_2, float p_end, float p_t) {
	/* Formula from Wikipedia article on Bezier curves. */
	float omt = (1.0f - p_t);
	float omt2 = omt * omt;
	float t2 = p_t * p_t;

	float d = (p_control_1 - p_start) * 3.0f * omt2 + (p_control_2 - p_control_1) * 6.0f * omt * p_t + (p_end - p_control_2) * 3.0f * t2;
	return d;
}

_ALWAYS_INLINE_ double angle_difference(double p_from, double p_to) {
	double difference = fmod(p_to - p_from, TAU);
	return fmod(2.0 * difference, TAU) - difference;
}
_ALWAYS_INLINE_ float angle_difference(float p_from, float p_to) {
	float difference = fmod(p_to - p_from, (float)TAU);
	return fmod(2.0f * difference, (float)TAU) - difference;
}

_ALWAYS_INLINE_ double lerp_angle(double p_from, double p_to, double p_weight) {
	return p_from + angle_difference(p_from, p_to) * p_weight;
}
_ALWAYS_INLINE_ float lerp_angle(float p_from, float p_to, float p_weight) {
	return p_from + angle_difference(p_from, p_to) * p_weight;
}

constexpr double inverse_lerp(double p_from, double p_to, double p_value) {
	return (p_value - p_from) / (p_to - p_from);
}
constexpr float inverse_lerp(float p_from, float p_to, float p_value) {
	return (p_value - p_from) / (p_to - p_from);
}

constexpr double remap(double p_value, double p_istart, double p_istop, double p_ostart, double p_ostop) {
	return lerp(p_ostart, p_ostop, inverse_lerp(p_istart, p_istop, p_value));
}
constexpr float remap(float p_value, float p_istart, float p_istop, float p_ostart, float p_ostop) {
	return lerp(p_ostart, p_ostop, inverse_lerp(p_istart, p_istop, p_value));
}

constexpr bool is_equal_approx(double p_left, double p_right, double p_tolerance) {
	// Check for exact equality first, required to handle "infinity" values.
	if (p_left == p_right) {
		return true;
	}
	// Then check for approximate equality.
	return abs(p_left - p_right) < p_tolerance;
}
constexpr bool is_equal_approx(float p_left, float p_right, float p_tolerance) {
	// Check for exact equality first, required to handle "infinity" values.
	if (p_left == p_right) {
		return true;
	}
	// Then check for approximate equality.
	return abs(p_left - p_right) < p_tolerance;
}

constexpr bool is_equal_approx(double p_left, double p_right) {
	// Check for exact equality first, required to handle "infinity" values.
	if (p_left == p_right) {
		return true;
	}
	// Then check for approximate equality.
	double tolerance = CMP_EPSILON * abs(p_left);
	if (tolerance < CMP_EPSILON) {
		tolerance = CMP_EPSILON;
	}
	return abs(p_left - p_right) < tolerance;
}
constexpr bool is_equal_approx(float p_left, float p_right) {
	// Check for exact equality first, required to handle "infinity" values.
	if (p_left == p_right) {
		return true;
	}
	// Then check for approximate equality.
	float tolerance = (float)CMP_EPSILON * abs(p_left);
	if (tolerance < (float)CMP_EPSILON) {
		tolerance = (float)CMP_EPSILON;
	}
	return abs(p_left - p_right) < tolerance;
}

constexpr bool is_zero_approx(double p_value) {
	return abs(p_value) < CMP_EPSILON;
}
constexpr bool is_zero_approx(float p_value) {
	return abs(p_value) < (float)CMP_EPSILON;
}

constexpr bool is_same(double p_left, double p_right) {
	return (p_left == p_right) || (is_nan(p_left) && is_nan(p_right));
}
constexpr bool is_same(float p_left, float p_right) {
	return (p_left == p_right) || (is_nan(p_left) && is_nan(p_right));
}

constexpr double smoothstep(double p_from, double p_to, double p_s) {
	if (is_equal_approx(p_from, p_to)) {
		if (likely(p_from <= p_to)) {
			return p_s <= p_from ? 0.0 : 1.0;
		} else {
			return p_s <= p_to ? 1.0 : 0.0;
		}
	}
	double s = CLAMP((p_s - p_from) / (p_to - p_from), 0.0, 1.0);
	return s * s * (3.0 - 2.0 * s);
}
constexpr float smoothstep(float p_from, float p_to, float p_s) {
	if (is_equal_approx(p_from, p_to)) {
		if (likely(p_from <= p_to)) {
			return p_s <= p_from ? 0.0f : 1.0f;
		} else {
			return p_s <= p_to ? 1.0f : 0.0f;
		}
	}
	float s = CLAMP((p_s - p_from) / (p_to - p_from), 0.0f, 1.0f);
	return s * s * (3.0f - 2.0f * s);
}

constexpr double move_toward(double p_from, double p_to, double p_delta) {
	return abs(p_to - p_from) <= p_delta ? p_to : p_from + SIGN(p_to - p_from) * p_delta;
}
constexpr float move_toward(float p_from, float p_to, float p_delta) {
	return abs(p_to - p_from) <= p_delta ? p_to : p_from + SIGN(p_to - p_from) * p_delta;
}

_ALWAYS_INLINE_ double rotate_toward(double p_from, double p_to, double p_delta) {
	double difference = angle_difference(p_from, p_to);
	double abs_difference = abs(difference);
	// When `p_delta < 0` move no further than to PI radians away from `p_to` (as PI is the max possible angle distance).
	return p_from + CLAMP(p_delta, abs_difference - PI, abs_difference) * (difference >= 0.0 ? 1.0 : -1.0);
}
_ALWAYS_INLINE_ float rotate_toward(float p_from, float p_to, float p_delta) {
	float difference = angle_difference(p_from, p_to);
	float abs_difference = abs(difference);
	// When `p_delta < 0` move no further than to PI radians away from `p_to` (as PI is the max possible angle distance).
	return p_from + CLAMP(p_delta, abs_difference - (float)PI, abs_difference) * (difference >= 0.0f ? 1.0f : -1.0f);
}

_ALWAYS_INLINE_ double linear_to_db(double p_linear) {
	return log(p_linear) * 8.6858896380650365530225783783321;
}
_ALWAYS_INLINE_ float linear_to_db(float p_linear) {
	return log(p_linear) * (float)8.6858896380650365530225783783321;
}

_ALWAYS_INLINE_ double db_to_linear(double p_db) {
	return exp(p_db * 0.11512925464970228420089957273422);
}
_ALWAYS_INLINE_ float db_to_linear(float p_db) {
	return exp(p_db * (float)0.11512925464970228420089957273422);
}

_ALWAYS_INLINE_ double round(double p_val) {
	return std::round(p_val);
}
_ALWAYS_INLINE_ float round(float p_val) {
	return std::round(p_val);
}

_ALWAYS_INLINE_ double wrapf(double p_value, double p_min, double p_max) {
	double range = p_max - p_min;
	if (is_zero_approx(range)) {
		return p_min;
	}
	double result = p_value - (range * floor((p_value - p_min) / range));
	if (is_equal_approx(result, p_max)) {
		return p_min;
	}
	return result;
}
_ALWAYS_INLINE_ float wrapf(float p_value, float p_min, float p_max) {
	float range = p_max - p_min;
	if (is_zero_approx(range)) {
		return p_min;
	}
	float result = p_value - (range * floor((p_value - p_min) / range));
	if (is_equal_approx(result, p_max)) {
		return p_min;
	}
	return result;
}

constexpr int64_t wrapi(int64_t p_value, int64_t p_min, int64_t p_max) {
	int64_t range = p_max - p_min;
	return range == 0 ? p_min : p_min + ((((p_value - p_min) % range) + range) % range);
}

_ALWAYS_INLINE_ double fract(double p_value) {
	return p_value - floor(p_value);
}
_ALWAYS_INLINE_ float fract(float p_value) {
	return p_value - floor(p_value);
}

_ALWAYS_INLINE_ double pingpong(double p_value, double p_length) {
	return (p_length != 0.0) ? abs(fract((p_value - p_length) / (p_length * 2.0)) * p_length * 2.0 - p_length) : 0.0;
}
_ALWAYS_INLINE_ float pingpong(float p_value, float p_length) {
	return (p_length != 0.0f) ? abs(fract((p_value - p_length) / (p_length * 2.0f)) * p_length * 2.0f - p_length) : 0.0f;
}

// double only, as these functions are mainly used by the editor and not performance-critical,
double ease(double p_x, double p_c);
int step_decimals(double p_step);
int range_step_decimals(double p_step); // For editor use only.
double snapped(double p_value, double p_step);

uint32_t larger_prime(uint32_t p_val);

void seed(uint64_t p_seed);
void randomize();
uint32_t rand_from_seed(uint64_t *p_seed);
uint32_t rand();
_ALWAYS_INLINE_ double randd() {
	return (double)rand() / (double)UINT32_MAX;
}
_ALWAYS_INLINE_ float randf() {
	return (float)rand() / (float)UINT32_MAX;
}
double randfn(double p_mean, double p_deviation);

double random(double p_from, double p_to);
float random(float p_from, float p_to);
int random(int p_from, int p_to);

// This function should be as fast as possible and rounding mode should not matter.
_ALWAYS_INLINE_ int fast_ftoi(float p_value) {
	return std::rint(p_value);
}

constexpr uint32_t halfbits_to_floatbits(uint16_t p_half) {
	uint16_t h_exp = (p_half & 0x7c00u);
	uint32_t f_sgn = ((uint32_t)p_half & 0x8000u) << 16;
	switch (h_exp) {
		case 0x0000u: { /* 0 or subnormal */
			uint16_t h_sig = (p_half & 0x03ffu);
			/* Signed zero */
			if (h_sig == 0) {
				return f_sgn;
			}
			/* Subnormal */
			h_sig <<= 1;
			while ((h_sig & 0x0400u) == 0) {
				h_sig <<= 1;
				h_exp++;
			}
			uint32_t f_exp = ((uint32_t)(127 - 15 - h_exp)) << 23;
			uint32_t f_sig = ((uint32_t)(h_sig & 0x03ffu)) << 13;
			return f_sgn + f_exp + f_sig;
		} break;
		case 0x7c00u: /* inf or NaN */
			/* All-ones exponent and a copy of the significand */
			return f_sgn + 0x7f800000u + (((uint32_t)(p_half & 0x03ffu)) << 13);
		default: /* normalized */
			/* Just need to adjust the exponent and shift */
			return f_sgn + (((uint32_t)(p_half & 0x7fffu) + 0x1c000u) << 13);
	}
}

_ALWAYS_INLINE_ float halfptr_to_float(const uint16_t *p_half) {
	union {
		uint32_t u32;
		float f32;
	} u;

	u.u32 = halfbits_to_floatbits(*p_half);
	return u.f32;
}

_ALWAYS_INLINE_ float half_to_float(const uint16_t p_half) {
	return halfptr_to_float(&p_half);
}

_ALWAYS_INLINE_ uint16_t make_half_float(float p_value) {
	union {
		float fv;
		uint32_t ui;
	} ci;
	ci.fv = p_value;

	uint32_t x = ci.ui;
	uint32_t sign = (unsigned short)(x >> 31);
	uint32_t mantissa;
	uint32_t exponent;
	uint16_t hf;

	// get mantissa
	mantissa = x & ((1 << 23) - 1);
	// get exponent bits
	exponent = x & (0xFF << 23);
	if (exponent >= 0x47800000) {
		// check if the original single precision float number is a NaN
		if (mantissa && (exponent == (0xFF << 23))) {
			// we have a single precision NaN
			mantissa = (1 << 23) - 1;
		} else {
			// 16-bit half-float representation stores number as Inf
			mantissa = 0;
		}
		hf = (((uint16_t)sign) << 15) | (uint16_t)((0x1F << 10)) |
				(uint16_t)(mantissa >> 13);
	}
	// check if exponent is <= -15
	else if (exponent <= 0x38000000) {
		/*
		// store a denorm half-float value or zero
		exponent = (0x38000000 - exponent) >> 23;
		mantissa >>= (14 + exponent);

		hf = (((uint16_t)sign) << 15) | (uint16_t)(mantissa);
		*/
		hf = 0; //denormals do not work for 3D, convert to zero
	} else {
		hf = (((uint16_t)sign) << 15) |
				(uint16_t)((exponent - 0x38000000) >> 13) |
				(uint16_t)(mantissa >> 13);
	}

	return hf;
}

_ALWAYS_INLINE_ float snap_scalar(float p_offset, float p_step, float p_target) {
	return p_step != 0 ? snapped(p_target - p_offset, p_step) + p_offset : p_target;
}

_ALWAYS_INLINE_ float snap_scalar_separation(float p_offset, float p_step, float p_target, float p_separation) {
	if (p_step != 0) {
		float a = snapped(p_target - p_offset, p_step + p_separation) + p_offset;
		float b = a;
		if (p_target >= 0) {
			b -= p_separation;
		} else {
			b += p_step;
		}
		return (abs(p_target - a) < abs(p_target - b)) ? a : b;
	}
	return p_target;
}

}; // namespace Math
// --- End core/math/math_funcs.h ---

// --- Folded from core/version.h ---
#include "core/version_generated.gen.h" // IWYU pragma: export

// Copied from typedefs.h to stay lean.
#ifndef _STR
#define _STR(m_x) #m_x
#define _MKSTR(m_x) _STR(m_x)
#endif

// Godot versions are of the form <major>.<minor> for the initial release,
// and then <major>.<minor>.<patch> for subsequent bugfix releases where <patch> != 0
// That's arbitrary, but we find it pretty and it's the current policy.

// Defines the main "branch" version. Patch versions in this branch should be
// forward-compatible.
// Example: "3.1"
#define VLTR_VERSION_BRANCH _MKSTR(VLTR_VERSION_MAJOR) "." _MKSTR(VLTR_VERSION_MINOR)
#if VLTR_VERSION_PATCH
// Example: "3.1.4"
#define VLTR_VERSION_NUMBER VLTR_VERSION_BRANCH "." _MKSTR(VLTR_VERSION_PATCH)
#else // patch is 0, we don't include it in the "pretty" version number.
// Example: "3.1" instead of "3.1.0"
#define VLTR_VERSION_NUMBER VLTR_VERSION_BRANCH
#endif // VLTR_VERSION_PATCH

// Version number encoded as hexadecimal int with one byte for each number,
// for easy comparison from code.
// Example: 3.1.4 will be 0x030104, making comparison easy from script.
#define VLTR_VERSION_HEX 0x10000 * VLTR_VERSION_MAJOR + 0x100 * VLTR_VERSION_MINOR + VLTR_VERSION_PATCH

// Describes the full configuration of that Godot version, including the version number,
// the status (beta, stable, etc.), potential module-specific features (e.g. mono)
// and double-precision status.
// Example: "3.1.4.stable.mono.double"
#ifdef REAL_T_IS_DOUBLE
#define VLTR_VERSION_FULL_CONFIG VLTR_VERSION_NUMBER "." VLTR_VERSION_STATUS VLTR_VERSION_MODULE_CONFIG ".double"
#else
#define VLTR_VERSION_FULL_CONFIG VLTR_VERSION_NUMBER "." VLTR_VERSION_STATUS VLTR_VERSION_MODULE_CONFIG
#endif

// Similar to VLTR_VERSION_FULL_CONFIG, but also includes the (potentially custom) VLTR_VERSION_BUILD
// description (e.g. official, custom_build, etc.).
// Example: "3.1.4.stable.mono.double.official"
#define VLTR_VERSION_FULL_BUILD VLTR_VERSION_FULL_CONFIG "." VLTR_VERSION_BUILD

// Same as above, but prepended with Godot's name and a cosmetic "v" for "version".
// Example: "Godot v3.1.4.stable.official.mono.double"
#define VLTR_VERSION_FULL_NAME VLTR_VERSION_NAME " v" VLTR_VERSION_FULL_BUILD

// Git commit hash, generated at build time in `core/version_hash.gen.cpp`.
extern const char *const VLTR_VERSION_HASH;

// Git commit date UNIX timestamp (in seconds), generated at build time in `core/version_hash.gen.cpp`.
// Set to 0 if unknown.
extern const unsigned long long VLTR_VERSION_TIMESTAMP;

#ifndef DISABLE_DEPRECATED
// Compatibility with pre-4.5 modules.
#define VERSION_SHORT_NAME VLTR_VERSION_SHORT_NAME
#define VERSION_NAME VLTR_VERSION_NAME
#define VERSION_MAJOR VLTR_VERSION_MAJOR
#define VERSION_MINOR VLTR_VERSION_MINOR
#define VERSION_PATCH VLTR_VERSION_PATCH
#define VERSION_STATUS VLTR_VERSION_STATUS
#define VERSION_BUILD VLTR_VERSION_BUILD
#define VERSION_MODULE_CONFIG VLTR_VERSION_MODULE_CONFIG
#define VERSION_WEBSITE VLTR_VERSION_WEBSITE
#define VERSION_DOCS_BRANCH VLTR_VERSION_DOCS_BRANCH
#define VERSION_DOCS_URL VLTR_VERSION_DOCS_URL
#define VERSION_BRANCH VLTR_VERSION_BRANCH
#define VERSION_NUMBER VLTR_VERSION_NUMBER
#define VERSION_HEX VLTR_VERSION_HEX
#define VERSION_FULL_CONFIG VLTR_VERSION_FULL_CONFIG
#define VERSION_FULL_BUILD VLTR_VERSION_FULL_BUILD
#define VERSION_FULL_NAME VLTR_VERSION_FULL_NAME
#define VERSION_HASH VLTR_VERSION_HASH
#define VERSION_TIMESTAMP VLTR_VERSION_TIMESTAMP
#endif // DISABLE_DEPRECATED
// --- End core/version.h ---
