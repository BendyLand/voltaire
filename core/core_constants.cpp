/**************************************************************************/
/*  core_constants.cpp                                                    */
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

#include "core/input/input_enums.h"
#include "core/os/keyboard.h"
#include "core_constants.h"

struct _CoreConstant
{
#ifdef DEBUG_ENABLED
	bool ignore_value_in_docs = false;
	bool is_bitfield = false;
#endif // DEBUG_ENABLED
	StringName enum_name;
	const char* name = nullptr;
	int64_t value = 0;

	_CoreConstant() {}

#ifdef DEBUG_ENABLED
	_CoreConstant(const StringName& p_enum_name, const char* p_name, int64_t p_value,
		bool p_ignore_value_in_docs = false, bool p_is_bitfield = false)
		: ignore_value_in_docs(p_ignore_value_in_docs), is_bitfield(p_is_bitfield),
		  enum_name(p_enum_name), name(p_name), value(p_value)
	{
	}
#else
	_CoreConstant(const StringName& p_enum_name, const char* p_name, int64_t p_value)
		: enum_name(p_enum_name), name(p_name), value(p_value)
	{
	}
#endif // DEBUG_ENABLED
};

static Vector<_CoreConstant> _global_constants;
static HashMap<StringName, int> _global_constants_map;
static HashMap<StringName, Vector<_CoreConstant>> _global_enums;

#ifdef DEBUG_ENABLED

#define BIND_CORE_CONSTANT(m_constant)                                                             \
	_global_constants.push_back(_CoreConstant(StringName(), #m_constant, m_constant));             \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT(m_constant)                                                        \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_FLAG(m_constant)                                                        \
	{                                                                                              \
		StringName enum_name = __constant_get_bitfield_name(m_constant);                           \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_constant, m_constant, false, true));                       \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

// This just binds enum classes as if they were regular enum constants.
#define BIND_CORE_ENUM_CLASS_CONSTANT(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		StringName enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(_CoreConstant(                                                 \
			enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member, false, true));          \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member)); \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		StringName enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member, false, true));            \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_NO_VAL(m_enum, m_prefix, m_member)                           \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member, true));   \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM(m_custom_name, m_constant)                                  \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant));          \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_CONSTANT_NO_VAL(m_constant)                                                      \
	_global_constants.push_back(_CoreConstant(StringName(), #m_constant, m_constant, true));       \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT_NO_VAL(m_constant)                                                 \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant, true));      \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM_NO_VAL(m_custom_name, m_constant)                           \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant, true));    \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#else

#define BIND_CORE_CONSTANT(m_constant)                                                             \
	_global_constants.push_back(_CoreConstant(StringName(), #m_constant, m_constant));             \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT(m_constant)                                                        \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_FLAG(m_constant)                                                        \
	{                                                                                              \
		StringName enum_name = __constant_get_bitfield_name(m_constant);                           \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

// This just binds enum classes as if they were regular enum constants.
#define BIND_CORE_ENUM_CLASS_CONSTANT(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		StringName enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member)); \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		StringName enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member)); \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_NO_VAL(m_enum, m_prefix, m_member)                           \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM(m_custom_name, m_constant)                                  \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant));          \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_CONSTANT_NO_VAL(m_constant)                                                      \
	_global_constants.push_back(_CoreConstant(StringName(), #m_constant, m_constant));             \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT_NO_VAL(m_constant)                                                 \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM_NO_VAL(m_custom_name, m_constant)                           \
	{                                                                                              \
		StringName enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant));          \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#endif // DEBUG_ENABLED

void register_global_constants() {}

void unregister_global_constants()
{
	_global_constants.clear();
	_global_constants_map.clear();
	_global_enums.clear();
}

int CoreConstants::get_global_constant_count() { return _global_constants.size(); }

StringName CoreConstants::get_global_constant_enum(int p_idx)
{
	return _global_constants[p_idx].enum_name;
}

#ifdef DEBUG_ENABLED
bool CoreConstants::is_global_constant_bitfield(int p_idx)
{
	return _global_constants[p_idx].is_bitfield;
}

bool CoreConstants::get_ignore_value_in_docs(int p_idx)
{
	return _global_constants[p_idx].ignore_value_in_docs;
}
#else
bool CoreConstants::is_global_constant_bitfield(int p_idx) { return false; }

bool CoreConstants::get_ignore_value_in_docs(int p_idx) { return false; }
#endif // DEBUG_ENABLED

const char* CoreConstants::get_global_constant_name(int p_idx)
{
	return _global_constants[p_idx].name;
}

int64_t CoreConstants::get_global_constant_value(int p_idx)
{
	return _global_constants[p_idx].value;
}

bool CoreConstants::is_global_constant(const StringName& p_name)
{
	return _global_constants_map.has(p_name);
}

int CoreConstants::get_global_constant_index(const StringName& p_name)
{
	ERR_FAIL_COND_V_MSG(
		!_global_constants_map.has(p_name), -1, "Trying to get index of non-existing constant.");
	return _global_constants_map[p_name];
}

bool CoreConstants::is_global_enum(const StringName& p_enum) { return _global_enums.has(p_enum); }

void CoreConstants::get_enum_values(
	const StringName& p_enum, HashMap<StringName, int64_t>* p_values)
{
	ERR_FAIL_NULL_MSG(p_values, "Trying to get enum values with null map.");
	ERR_FAIL_COND_MSG(!_global_enums.has(p_enum), "Trying to get values of non-existing enum.");
	for (const _CoreConstant& constant : _global_enums[p_enum]) {
		(*p_values)[constant.name] = constant.value;
	}
}

#ifdef TOOLS_ENABLED

void CoreConstants::get_global_enums(List<StringName>* r_values)
{
	for (const KeyValue<StringName, Vector<_CoreConstant>>& global_enum : _global_enums) {
		r_values->push_back(global_enum.key);
	}
}

#endif


