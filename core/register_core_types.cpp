/**************************************************************************/
/*  register_core_types.cpp                                               */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/stream_peer.h"
#include "core/io/resource_loader.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/packet_peer_udp.h"
#include "core/io/compression.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/io/stream_peer.h"
#include "core/io/config_file.h"
#include "core/io/packet_peer_udp.h"
#include "core/io/config_file.h"
#include "core/math/geometry_3d.h"
#include "core/types.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/translation_server.h"
#include "main/main.h"
#ifndef DISABLE_DEPRECATED
#include "core/io/compression.h"
#endif

static Ref<ResourceFormatSaverBinary> resource_saver_binary;
static Ref<ResourceFormatLoaderBinary> resource_loader_binary;
static Ref<ResourceFormatImporterLoader> resource_format_importer;
static Ref<ResourceFormatImporterSaver> resource_format_importer_saver;
static Ref<ResourceFormatLoaderImage> resource_format_image;
static Ref<TranslationLoaderPO> resource_format_po;
static Ref<ResourceFormatSaverCrypto> resource_format_saver_crypto;
static Ref<ResourceFormatLoaderCrypto> resource_format_loader_crypto;
static Ref<ResourceFormatSaverJSON> resource_saver_json;
static Ref<ResourceFormatLoaderJSON> resource_loader_json;

static CoreBind::ResourceLoader* _resource_loader = nullptr;

static Mutex _global_mutex;

extern void register_global_constants();
extern void unregister_global_constants();

static bool _is_core_extensions_registered = false;

void register_core_types()
{
	OS::benchmark_begin_measure("Core", "Register Types");

	CryptoCore::initialize();

	register_global_constants();

	ResourceLoader::initialize();

	resource_format_po.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_po);

	resource_saver_binary.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_binary);
	resource_loader_binary.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_binary);

	ResourceFormatImporter::initialize();
	resource_format_importer.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_importer);

	resource_format_importer_saver.instantiate();
	ResourceSaver::add_resource_format_saver(resource_format_importer_saver);

	resource_format_image.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_image);

	resource_format_saver_crypto.instantiate();
	ResourceSaver::add_resource_format_saver(resource_format_saver_crypto);

	resource_format_loader_crypto.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_loader_crypto);

	resource_saver_json.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_json);

	resource_loader_json.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_json);

#ifndef DISABLE_DEPRECATED
#endif

	ResourceUID::initialize();

	IP::initialize();

	OS::benchmark_end_measure("Core", "Register Types");
}

void register_core_settings() {}

void register_early_core_singletons() {}

void register_core_singletons() {}

void register_core_extensions()
{
	OS::benchmark_begin_measure("Core", "Register Extensions");

	// Hardcoded for now.
	_is_core_extensions_registered = true;

	OS::benchmark_end_measure("Core", "Register Extensions");
}

void unregister_core_extensions()
{
	OS::benchmark_begin_measure("Core", "Unregister Extensions");
	OS::benchmark_end_measure("Core", "Unregister Extensions");
}

void unregister_core_types()
{
	IP::finalize();
	ResourceFormatImporter::finalize();
	ResourceUID::finalize();
}



// ============================================================================
// Merged from core/core_constants.cpp
// ============================================================================

#include "core/input/input_event.h"
#include "core/os/keyboard.h"
#include "core/config/project_settings.h"

struct _CoreConstant
{
#ifdef DEBUG_ENABLED
	bool ignore_value_in_docs = false;
	bool is_bitfield = false;
#endif // DEBUG_ENABLED
	String enum_name;
	const char* name = nullptr;
	int64_t value = 0;

	_CoreConstant() {}

#ifdef DEBUG_ENABLED
	_CoreConstant(const String& p_enum_name, const char* p_name, int64_t p_value,
		bool p_ignore_value_in_docs = false, bool p_is_bitfield = false)
		: ignore_value_in_docs(p_ignore_value_in_docs), is_bitfield(p_is_bitfield),
		  enum_name(p_enum_name), name(p_name), value(p_value)
	{
	}
#else
	_CoreConstant(const String& p_enum_name, const char* p_name, int64_t p_value)
		: enum_name(p_enum_name), name(p_name), value(p_value)
	{
	}
#endif // DEBUG_ENABLED
};

static Vector<_CoreConstant> _global_constants;
static HashMap<String, int> _global_constants_map;
static HashMap<String, Vector<_CoreConstant>> _global_enums;

#ifdef DEBUG_ENABLED

#define BIND_CORE_CONSTANT(m_constant)                                                             \
	_global_constants.push_back(_CoreConstant(String(), #m_constant, m_constant));             \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT(m_constant)                                                        \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_FLAG(m_constant)                                                        \
	{                                                                                              \
		String enum_name = __constant_get_bitfield_name(m_constant);                           \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_constant, m_constant, false, true));                       \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

// This just binds enum classes as if they were regular enum constants.
#define BIND_CORE_ENUM_CLASS_CONSTANT(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		String enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(_CoreConstant(                                                 \
			enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member, false, true));          \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member)); \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		String enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member, false, true));            \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_NO_VAL(m_enum, m_prefix, m_member)                           \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member, true));   \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM(m_custom_name, m_constant)                                  \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant));          \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_CONSTANT_NO_VAL(m_constant)                                                      \
	_global_constants.push_back(_CoreConstant(String(), #m_constant, m_constant, true));       \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT_NO_VAL(m_constant)                                                 \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant, true));      \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM_NO_VAL(m_custom_name, m_constant)                           \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant, true));    \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#else

#define BIND_CORE_CONSTANT(m_constant)                                                             \
	_global_constants.push_back(_CoreConstant(String(), #m_constant, m_constant));             \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT(m_constant)                                                        \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_FLAG(m_constant)                                                        \
	{                                                                                              \
		String enum_name = __constant_get_bitfield_name(m_constant);                           \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

// This just binds enum classes as if they were regular enum constants.
#define BIND_CORE_ENUM_CLASS_CONSTANT(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG(m_enum, m_prefix, m_member)                                  \
	{                                                                                              \
		String enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member)); \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_BITFIELD_CLASS_FLAG_CUSTOM(m_enum, m_name, m_member)                             \
	{                                                                                              \
		String enum_name = __constant_get_bitfield_name(m_enum::m_member);                     \
		_global_constants.push_back(_CoreConstant(enum_name, #m_name, (int64_t)m_enum::m_member)); \
		_global_constants_map[#m_name] = _global_constants.size() - 1;                             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CLASS_CONSTANT_NO_VAL(m_enum, m_prefix, m_member)                           \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_enum::m_member);                         \
		_global_constants.push_back(                                                               \
			_CoreConstant(enum_name, #m_prefix "_" #m_member, (int64_t)m_enum::m_member));         \
		_global_constants_map[#m_prefix "_" #m_member] = _global_constants.size() - 1;             \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM(m_custom_name, m_constant)                                  \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, m_custom_name, m_constant));          \
		_global_constants_map[m_custom_name] = _global_constants.size() - 1;                       \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_CONSTANT_NO_VAL(m_constant)                                                      \
	_global_constants.push_back(_CoreConstant(String(), #m_constant, m_constant));             \
	_global_constants_map[#m_constant] = _global_constants.size() - 1;

#define BIND_CORE_ENUM_CONSTANT_NO_VAL(m_constant)                                                 \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
		_global_constants.push_back(_CoreConstant(enum_name, #m_constant, m_constant));            \
		_global_constants_map[#m_constant] = _global_constants.size() - 1;                         \
		_global_enums[enum_name].push_back(                                                        \
			(_global_constants.ptr())[_global_constants.size() - 1]);                              \
	}

#define BIND_CORE_ENUM_CONSTANT_CUSTOM_NO_VAL(m_custom_name, m_constant)                           \
	{                                                                                              \
		String enum_name = __constant_get_enum_name(m_constant);                               \
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

String CoreConstants::get_global_constant_enum(int p_idx)
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

bool CoreConstants::is_global_constant(const String& p_name)
{
	return _global_constants_map.has(p_name);
}

int CoreConstants::get_global_constant_index(const String& p_name)
{
	ERR_FAIL_COND_V_MSG(
		!_global_constants_map.has(p_name), -1, "Trying to get index of non-existing constant.");
	return _global_constants_map[p_name];
}

bool CoreConstants::is_global_enum(const String& p_enum) { return _global_enums.has(p_enum); }

void CoreConstants::get_enum_values(
	const String& p_enum, HashMap<String, int64_t>* p_values)
{
	ERR_FAIL_NULL_MSG(p_values, "Trying to get enum values with null map.");
	ERR_FAIL_COND_MSG(!_global_enums.has(p_enum), "Trying to get values of non-existing enum.");
	for (const _CoreConstant& constant : _global_enums[p_enum]) {
		(*p_values)[constant.name] = constant.value;
	}
}

#ifdef TOOLS_ENABLED

void CoreConstants::get_global_enums(List<String>* r_values)
{
	for (const KeyValue<String, Vector<_CoreConstant>>& global_enum : _global_enums) {
		r_values->push_back(global_enum.key);
	}
}

#endif

#ifdef BIND_CORE_CONSTANT
#undef BIND_CORE_CONSTANT
#endif
#ifdef BIND_CORE_ENUM_CONSTANT
#undef BIND_CORE_ENUM_CONSTANT
#endif
#ifdef BIND_CORE_BITFIELD_FLAG
#undef BIND_CORE_BITFIELD_FLAG
#endif
#ifdef BIND_CORE_ENUM_CLASS_CONSTANT
#undef BIND_CORE_ENUM_CLASS_CONSTANT
#endif
#ifdef BIND_CORE_BITFIELD_CLASS_FLAG
#undef BIND_CORE_BITFIELD_CLASS_FLAG
#endif
#ifdef BIND_CORE_ENUM_CLASS_CONSTANT_CUSTOM
#undef BIND_CORE_ENUM_CLASS_CONSTANT_CUSTOM
#endif
#ifdef BIND_CORE_BITFIELD_CLASS_FLAG_CUSTOM
#undef BIND_CORE_BITFIELD_CLASS_FLAG_CUSTOM
#endif
#ifdef BIND_CORE_ENUM_CLASS_CONSTANT_NO_VAL
#undef BIND_CORE_ENUM_CLASS_CONSTANT_NO_VAL
#endif
#ifdef BIND_CORE_ENUM_CONSTANT_CUSTOM
#undef BIND_CORE_ENUM_CONSTANT_CUSTOM
#endif
#ifdef BIND_CORE_CONSTANT_NO_VAL
#undef BIND_CORE_CONSTANT_NO_VAL
#endif
#ifdef BIND_CORE_ENUM_CONSTANT_NO_VAL
#undef BIND_CORE_ENUM_CONSTANT_NO_VAL
#endif
#ifdef BIND_CORE_ENUM_CONSTANT_CUSTOM_NO_VAL
#undef BIND_CORE_ENUM_CONSTANT_CUSTOM_NO_VAL
#endif

// ============================================================================
// Merged from core/config/project_settings.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/compression.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/os/os.h"
#include "core/templates/rb_set.h"
#include "core/version.h"
#include "core/config/project_settings.h"

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#endif									 // TOOLS_ENABLED

String ProjectSettings::get_project_data_dir_name() { return data->project_data_dir_name; }

String ProjectSettings::get_project_data_path() { return "res://" + get_project_data_dir_name(); }

String ProjectSettings::get_resource_path()
{
	if (data) {
		return data->resource_path;
	}
	return "res://";
}

// This returns paths like "res://.godot/imported".
String ProjectSettings::get_imported_files_path()
{
	return get_project_data_path().path_join("imported");
}

#ifdef TOOLS_ENABLED
// Returns the features that a project must have when opened with this build of Godot.
// This is used by the project manager to provide the initial_settings for config/features.
const Vector<String> ProjectSettings::get_required_features()
{
	Vector<String> features;
	features.append(VLTR_VERSION_BRANCH);
#ifdef REAL_T_IS_DOUBLE
	features.append("Double Precision");
#endif
	return features;
}

// Returns the features supported by this build of Godot. Includes all required features.
const Vector<String> ProjectSettings::_get_supported_features()
{
	Vector<String> features = get_required_features();

#ifdef LIBGODOT_ENABLED
	features.append("LibGodot");
#endif

#ifdef MODULE_MONO_ENABLED
	features.append("C#");
#endif
	// Allow pinning to a specific patch number or build type by marking
	// them as supported. They're only used if the user adds them manually.
	features.append(VLTR_VERSION_BRANCH "." _MKSTR(VLTR_VERSION_PATCH));
	features.append(VLTR_VERSION_FULL_CONFIG);
	features.append(VLTR_VERSION_FULL_BUILD);

#ifdef RD_ENABLED
	features.append("Forward Plus");
	features.append("Mobile");
#endif

#ifdef GLES3_ENABLED
	features.append("GL Compatibility");
#endif
	return features;
}

// Returns the features that this project needs but this build of Godot lacks.
const Vector<String> ProjectSettings::get_unsupported_features(
	const Vector<String>& p_project_features)
{
	Vector<String> unsupported_features;
	Vector<String> supported_features = _get_supported_features();
	for (int i = 0; i < p_project_features.size(); i++) {
		if (!supported_features.has(p_project_features[i])) {
			// Temporary compatibility code to ease upgrade to 4.0 beta 2+.
			if (p_project_features[i].begins_with("Vulkan")) {
				continue;
			}
			unsupported_features.append(p_project_features[i]);
		}
	}
	unsupported_features.sort();
	return unsupported_features;
}

// Returns the features that both this project has and this build of Godot has, ensuring required
// features exist.
const Vector<String> ProjectSettings::_trim_to_supported_features(
	const Vector<String>& p_project_features)
{
	// Remove unsupported features if present.
	Vector<String> features = Vector<String>(p_project_features);
	Vector<String> supported_features = _get_supported_features();
	for (int i = p_project_features.size() - 1; i > -1; i--) {
		if (!supported_features.has(p_project_features[i])) {
			features.remove_at(i);
		}
	}
	// Add required features if not present.
	Vector<String> required_features = get_required_features();
	for (int i = 0; i < required_features.size(); i++) {
		if (!features.has(required_features[i])) {
			features.append(required_features[i]);
		}
	}
	features.sort();
	return features;
}
#endif // TOOLS_ENABLED

String ProjectSettings::localize_path(const String& p_path)
{
	String path = p_path.simplify_path();

	if (data->resource_path.is_empty() ||
		(path.is_absolute_path() && !path.begins_with(data->resource_path))) {
		return path;
	}

	// Check if we have a special path (like res://) or a protocol identifier.
	int p = path.find("://");
	bool found = false;
	if (p > 0) {
		found = true;
		for (int i = 0; i < p; i++) {
			if (!is_ascii_alphanumeric_char(path[i])) {
				found = false;
				break;
			}
		}
	}
	if (found) {
		return path;
	}

	Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	if (dir->change_dir(path) == OK) {
		String cwd = dir->get_current_dir();
		cwd = cwd.replace_char('\\', '/');

		// Ensure that we end with a '/'.
		// This is important to ensure that we do not wrongly localize the resource path
		// in an absolute path that just happens to contain this string but points to a
		// different folder (e.g. "/my/project" as resource_path would be contained in
		// "/my/project_data", even though the latter is not part of res://.
		// `path_join("")` is an easy way to ensure we have a trailing '/'.
		const String res_path = data->resource_path.path_join("");

		// DirAccess::get_current_dir() is not guaranteed to return a path that with a trailing '/',
		// so we must make sure we have it as well in order to compare with 'res_path'.
		cwd = cwd.path_join("");

		if (!cwd.begins_with(res_path)) {
			return path;
		}

		return cwd.replace_first(res_path, "res://");
	}
	else {
		int sep = path.rfind_char('/');
		if (sep == -1) {
			return "res://" + path;
		}

		String parent = path.substr(0, sep);

		String plocal = localize_path(parent);
		if (plocal.is_empty()) {
			return "";
		}
		// Only strip the starting '/' from 'path' if its parent ('plocal') ends with '/'
		if (plocal[plocal.length() - 1] == '/') {
			sep += 1;
		}
		return plocal + path.substr(sep);
	}
}

void ProjectSettings::add_hidden_prefix(const String& p_prefix)
{
	data->hidden_prefixes.push_back(p_prefix);
}

String ProjectSettings::globalize_path(const String& p_path)
{
	if (p_path.begins_with("res://")) {
		if (!data->resource_path.is_empty()) {
			return p_path.replace("res:/", data->resource_path);
		}
		return p_path.replace("res://", "");
	}
	else if (p_path.begins_with("uid://")) {
		const String path = ResourceUID::uid_to_path(p_path);
		if (!data->resource_path.is_empty()) {
			return path.replace("res:/", data->resource_path);
		}
		return path.replace("res://", "");
	}
	else if (p_path.begins_with("user://")) {
		String data_dir = OS::get_user_data_dir();
		if (!data_dir.is_empty()) {
			return p_path.replace("user:/", data_dir);
		}
		return p_path.replace("user://", "");
	}

	return p_path;
}

struct _VCSort
{
	String name;
	int order = 0;
	uint32_t flags = 0;

	bool operator<(const _VCSort& p_vcs) const
	{
		return order == p_vcs.order ? name < p_vcs.name : order < p_vcs.order;
	}
};

void ProjectSettings::_queue_changed(const String& p_name)
{
	data->changed_settings.insert(p_name);
}

void ProjectSettings::_emit_changed()
{
	if (!data->is_changed) {
		return;
	}
	data->is_changed = false;

	// Clear the changed settings after emitting the signal
	data->changed_settings.clear();
}

bool ProjectSettings::load_resource_pack(const String& p_pack, bool p_replace_files, int p_offset)
{
	return ProjectSettings::_load_resource_pack(p_pack, p_replace_files, p_offset, false);
}

bool ProjectSettings::_load_resource_pack(
	const String& p_pack, bool p_replace_files, int p_offset, bool p_main_pack)
{
	if (PackedData::is_disabled()) {
		return false;
	}

	if (p_pack == "res://") {
		// Loading the resource directory as a pack source is reserved for internal use only.
		return false;
	}

	if (!p_main_pack && !data->using_datapack && !OS::get_resource_dir().is_empty()) {
		// Add the project's resource file system to PackedData so directory access keeps working
		// when the game is running without a main pack, like in the editor or on Android.
		PackedData::add_pack_source(memnew(PackedSourceDirectory));
		PackedData::add_pack("res://", false, 0);
		DirAccess::make_default<DirAccessPack>(DirAccess::ACCESS_RESOURCES);
		data->using_datapack = true;
	}

	bool ok = PackedData::add_pack(p_pack, p_replace_files, p_offset) == OK;
	if (!ok) {
		return false;
	}

	if (data->project_loaded) {
		// This pack may have declared new global classes (make sure they are picked up).
		refresh_global_class_list();

		// This pack may have defined new UIDs, make sure they are cached.
		ResourceUID::load_from_cache(false);
	}

	// If the data pack was found, all directory access will be from here.
	if (!data->using_datapack) {
		DirAccess::make_default<DirAccessPack>(DirAccess::ACCESS_RESOURCES);
		data->using_datapack = true;
	}

	return true;
}

/*
 * This method is responsible for loading a project.vltr file and/or data file
 * using the following merit order:
 *  - If using NetworkClient, try to lookup project file or fail.
 *  - If --main-pack was passed by the user (`p_main_pack`), load it or fail.
 *  - Search for project PCKs automatically. For each step we try loading a potential
 *    PCK, and if it doesn't work, we proceed to the next step. If any step succeeds,
 *    we try loading the project settings, and abort if it fails. Steps:
 *    o Bundled PCK in the executable.
 *    o [macOS only] PCK with same basename as the binary in the .app resource dir.
 *    o PCK with same basename as the binary in the binary's directory. We handle both
 *      changing the extension to '.pck' (e.g. 'win_game.exe' -> 'win_game.pck') and
 *      appending '.pck' to the binary name (e.g. 'linux_game' -> 'linux_game.pck').
 *    o PCK with the same basename as the binary in the current working directory.
 *      Same as above for the two possible PCK file names.
 *  - On Android, look for 'assets.sparsepck' and try loading it, if it doesn't work,
 *    proceed to the next step.
 *  - On relevant platforms (Android/iOS), lookup project file in OS resource path.
 *    If found, load it or fail.
 *  - Lookup project file in passed `p_path` (--path passed by the user), i.e. we
 *    are running from source code.
 *    If not found and `p_upwards` is true, look for project files in parent folders
 *    up to the system root (used to run a game from command line while in a subfolder).
 *    If a project file is found, load it or fail.
 *    If nothing was found, error out.
 */
Error ProjectSettings::_setup(
	const String& p_path, const String& p_main_pack, bool p_upwards, bool p_ignore_override)
{
	if (!OS::get_resource_dir().is_empty()) {
		// OS will call ProjectSettings->get_resource_path which will be empty if not overridden!
		// If the OS would rather use a specific location, then it will not be empty.
		data->resource_path = OS::get_resource_dir().replace_char('\\', '/');
		if (!data->resource_path.is_empty() &&
			data->resource_path[data->resource_path.length() - 1] == '/') {
			data->resource_path =
				data->resource_path.substr(0, data->resource_path.length() - 1); // Chop end.
		}
	}

	// Attempt with a user-defined main pack first

	if (!p_main_pack.is_empty()) {
		bool ok = _load_resource_pack(p_main_pack, false, 0, true);
		Error err = _load_settings_text_or_binary("res://project.vltr", "res://project.binary");
		return err;
	}

	String exec_path = OS::get_executable_path();

	if (!exec_path.is_empty()) {
		// We do several tests sequentially until one succeeds to find a PCK,
		// and if so, we attempt loading it at the end.

		// Attempt with PCK bundled into executable.
		bool found = _load_resource_pack(exec_path, false, 0, true);

		// Attempt with exec_name.pck.
		// (This is the usual case when distributing a Godot game.)
		String exec_dir = exec_path.get_base_dir();
		String exec_filename = exec_path.get_file();
		String exec_basename = exec_filename.get_basename();

		// Based on the OS, it can be the exec path + '.pck' (Linux w/o extension, macOS in .app
		// bundle) or the exec path's basename + '.pck' (Windows). We need to test both
		// possibilities as extensions for Linux binaries are optional (so both 'mygame.bin' and
		// 'mygame' should be able to find 'mygame.pck').

#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
		if (!found) {
			// Attempt to load PCK from macOS .app bundle resources.
			found =
				_load_resource_pack(OS::get_bundle_resource_dir().path_join(exec_basename + ".pck"),
					false, 0, true) ||
				_load_resource_pack(OS::get_bundle_resource_dir().path_join(exec_filename + ".pck"),
					false, 0, true);
		}
#endif

		if (!found) {
			// Try to load data pack at the location of the executable.
			// As mentioned above, we have two potential names to attempt.
			found =
				_load_resource_pack(exec_dir.path_join(exec_basename + ".pck"), false, 0, true) ||
				_load_resource_pack(exec_dir.path_join(exec_filename + ".pck"), false, 0, true);
		}

		if (!found) {
			// If we couldn't find them next to the executable, we attempt
			// the current working directory. Same story, two tests.
			found = _load_resource_pack(exec_basename + ".pck", false, 0, true) ||
					_load_resource_pack(exec_filename + ".pck", false, 0, true);
		}

		// If we opened our package, try and load our project.
		if (found) {
			Error err = _load_settings_text_or_binary("res://project.vltr", "res://project.binary");
			return err;
		}
	}

#ifdef ANDROID_ENABLED
	// Attempt to load sparse PCK assets.
	_load_resource_pack("res://assets.sparsepck", false, 0, true);
#endif

	// Try to use the filesystem for files, according to OS.
	// (Only Android -when reading from PCK-.)

	if (!OS::get_resource_dir().is_empty()) {
		Error err = _load_settings_text_or_binary("res://project.vltr", "res://project.binary");
		return err;
	}

#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	// Attempt to load project file from macOS .app bundle resources.
	resource_path = OS::get_bundle_resource_dir();
	if (!resource_path.is_empty()) {
		if (resource_path[resource_path.length() - 1] == '/') {
			resource_path = resource_path.substr(0, resource_path.length() - 1); // Chop end.
		}
		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		ERR_FAIL_COND_V_MSG(d.is_null(), ERR_CANT_CREATE,
			vformat("Cannot create DirAccess for path '%s'.", resource_path));
		d->change_dir(resource_path);

		Error err;

		err = _load_settings_text_or_binary(
			resource_path.path_join("project.vltr"), resource_path.path_join("project.binary"));
		if (err == OK && !p_ignore_override) {
			// Optional, we don't mind if it fails.
#ifdef OVERRIDE_ENABLED
			bool disable_override =
				GLOBAL_GET("application/config/disable_project_settings_override");
			if (!disable_override) {
				_load_settings_text(resource_path.path_join("override.cfg"));
			}
#endif // OVERRIDE_ENABLED
			return err;
		}
	}
#endif // MACOS_ENABLED

	// Nothing was found, try to find a project file in provided path (`p_path`)
	// or, if requested (`p_upwards`) in parent directories.

	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	d->change_dir(p_path);

	String current_dir = d->get_current_dir();
	bool found = false;
	Error err;

	while (true) {
		// Set the resource path early so things can be resolved when loading.
		data->resource_path = current_dir;
		data->resource_path =
			data->resource_path.replace_char('\\', '/'); // Windows path to Unix path just in case.
		err = _load_settings_text_or_binary(
			current_dir.path_join("project.vltr"), current_dir.path_join("project.binary"));
		if (err == OK) {
			found = true;
			break;
		}

#if defined(OVERRIDE_PATH_ENABLED)
		if (p_upwards) {
			// Try to load settings ascending through parent directories
			d->change_dir("..");
			if (d->get_current_dir() == current_dir) {
				break; // not doing anything useful
			}
			current_dir = d->get_current_dir();
		}
		else {
#else
		{
#endif
			break;
		}
	}

	if (!found) {
		return err;
	}

	if (data->resource_path.length() &&
		data->resource_path[data->resource_path.length() - 1] == '/') {
		data->resource_path =
			data->resource_path.substr(0, data->resource_path.length() - 1); // Chop end.
	}

	return OK;
}

Error ProjectSettings::setup(
	const String& p_path, const String& p_main_pack, bool p_upwards, bool p_ignore_override)
{
	Error err = _setup(p_path, p_main_pack, p_upwards, p_ignore_override);

	// Updating the default value after the project settings have loaded.
	data->project_data_dir_name = PROJECT_DATA_DIR_NAME_SUFFIX;

	// Using GLOBAL_GET on every block for compressing can be slow, so assigning here.
	load_scene_groups_cache();

	data->project_loaded = err == OK;
	return err;
}

Error ProjectSettings::_load_settings_binary(const String& p_path)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (err != OK) {
		return err;
	}

	uint8_t hdr[4];
	f->get_buffer(hdr, 4);
	ERR_FAIL_COND_V_MSG((hdr[0] != 'E' || hdr[1] != 'C' || hdr[2] != 'F' || hdr[3] != 'G'),
		ERR_FILE_CORRUPT, "Corrupted header in binary project.binary (not ECFG).");

	uint32_t count = f->get_32();

	for (uint32_t i = 0; i < count; i++) {
		uint32_t slen = f->get_32();
		CharString cs;
		cs.resize_uninitialized(slen + 1);
		cs[slen] = 0;
		f->get_buffer((uint8_t*)cs.ptr(), slen);
		String key = String::utf8(cs.ptr(), slen);

		uint32_t vlen = f->get_32();
		Vector<uint8_t> d;
		d.resize(vlen);
		f->get_buffer(d.ptrw(), vlen);
	}

	return OK;
}

Error ProjectSettings::save()
{
	Error error = save_custom(get_resource_path().path_join("project.vltr"));
	if (error == OK) {
		data->last_save_time =
			FileAccess::get_modified_time(get_resource_path().path_join("project.vltr"));
	}
	return error;
}

Error ProjectSettings::_save_settings_binary(const String& p_file,
	const RBMap<String, List<String>>& p_props, const StringMap& p_custom,
	const String& p_custom_features)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);

	uint8_t hdr[4] = {'E', 'C', 'F', 'G'};
	file->store_buffer(hdr, 4);

	int count = 0;

	for (const KeyValue<String, List<String>>& E : p_props) {
		count += E.value.size();
	}

	if (!p_custom_features.is_empty()) {
		// Store how many properties are saved, add one for custom features, which must always go
		// first.
		file->store_32(uint32_t(count + 1));

		int len;
		Vector<uint8_t> buff;
		buff.resize(len);
		file->store_32(uint32_t(len));
		file->store_buffer(buff.ptr(), buff.size());

	}
	else {
		// Store how many properties are saved.
		file->store_32(uint32_t(count));
	}

	for (const KeyValue<String, List<String>>& E : p_props) {
		for (const String& key : E.value) {
			String k = key;
			if (!E.key.is_empty()) {
				k = E.key + "/" + k;
			}
			file->store_pascal_string(k);
			int len;
			Vector<uint8_t> buff;
			buff.resize(len);
			file->store_32(uint32_t(len));
			file->store_buffer(buff.ptr(), buff.size());
		}
	}

	return OK;
}

Error ProjectSettings::_save_settings_text(const String& p_file,
	const RBMap<String, List<String>>& p_props, const StringMap& p_custom,
	const String& p_custom_features)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);

	file->store_line("; Engine configuration file.");
	file->store_line("; It's best edited using the editor UI and not directly,");
	file->store_line("; since the parameters that go here are not all obvious.");
	file->store_line(";");
	file->store_line("; Format:");
	file->store_line(";   [section] ; section goes between []");
	file->store_line(";   param=value ; assign values to parameters");
	file->store_line("");

	file->store_string("config_version=" + itos(CONFIG_VERSION) + "\n");
	if (!p_custom_features.is_empty()) {
		file->store_string("custom_features=\"" + p_custom_features + "\"\n");
	}
	file->store_string("\n");

	for (const KeyValue<String, List<String>>& E : p_props) {
		if (E.key != p_props.begin()->key) {
			file->store_string("\n");
		}

		if (!E.key.is_empty()) {
			file->store_string("[" + E.key + "]\n\n");
		}
		for (const String& F : E.value) {
			String key = F;
			if (!E.key.is_empty()) {
				key = E.key + "/" + key;
			}
			String vstr;
			file->store_string(F.property_name_encode() + "=" + vstr + "\n");
		}
	}

	return OK;
}

Error ProjectSettings::_save_custom_bnd(const String& p_file) { return save_custom(p_file); }

#ifdef TOOLS_ENABLED
bool _csproj_exists(const String& p_root_dir)
{
	Ref<DirAccess> dir = DirAccess::open(p_root_dir);
	ERR_FAIL_COND_V(dir.is_null(), false);

	dir->list_dir_begin();
	String file_name = dir->_get_next();
	while (file_name != "") {
		if (!dir->current_is_dir() && file_name.get_extension() == "csproj") {
			return true;
		}
		file_name = dir->_get_next();
	}

	return false;
}
#endif // TOOLS_ENABLED

Error ProjectSettings::save_custom(const String& p_path, const StringMap& p_custom,
	const Vector<String>& p_custom_features, bool p_merge_with_current)
{
	ERR_FAIL_COND_V_MSG(
		p_path.is_empty(), ERR_INVALID_PARAMETER, "Project settings save path cannot be empty.");

	RBSet<_VCSort> vclist;
	RBMap<String, List<String>> save_props;

	for (const _VCSort& E : vclist) {
		String category = E.name;
		String name = E.name;

		int div = category.find_char('/');

		if (div < 0) {
			category = "";
		}
		else {
			category = category.substr(0, div);
			name = name.substr(div + 1);
		}
		save_props[category].push_back(name);
	}
	String save_features;
	for (int i = 0; i < p_custom_features.size(); i++) {
		if (i > 0) {
			save_features += ",";
		}
		String f = p_custom_features[i].strip_edges().remove_char('\"');
		save_features += f;
	}
	if (p_path.ends_with(".vltr")) {
		Error err = OK;
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
		ERR_FAIL_COND_V_MSG(file.is_null(), err != OK ? err : ERR_CANT_OPEN,
			vformat("Failed to open '%s' for writing.", p_path));
		file->store_buffer((const uint8_t*)&data->project, sizeof(VoltaireProjectFile));
		return OK;
	}
	else if (p_path.ends_with("override.cfg")) {
		return _save_settings_text(p_path, save_props, p_custom, save_features);
	}
	else if (p_path.ends_with(".binary")) {
		return _save_settings_binary(p_path, save_props, p_custom, save_features);
	}
	return ERR_FILE_UNRECOGNIZED;
}

bool ProjectSettings::is_using_datapack() { return data->using_datapack; }

bool ProjectSettings::is_project_loaded() { return data->project_loaded; }

Vector<String> ProjectSettings::get_changed_settings()
{
	Vector<String> arr;
	for (const String& setting : data->changed_settings) {
		arr.push_back(setting);
	}
	return arr;
}

bool ProjectSettings::check_changed_settings_in_group(const String& p_setting_prefix)
{
	for (const String& setting : data->changed_settings) {
		if (String(setting).begins_with(p_setting_prefix)) {
			return true;
		}
	}
	return false;
}

String ProjectSettings::get_global_class_list_path()
{
	return get_project_data_path().path_join("global_script_class_cache.cfg");
}

bool ProjectSettings::has_custom_feature(const String& p_feature)
{
	return data->custom_features.has(p_feature);
}

const HashMap<String, ProjectSettings::AutoloadInfo>& ProjectSettings::get_autoload_list()
{
	return data->autoloads;
}

void ProjectSettings::add_autoload(const AutoloadInfo& p_autoload, bool p_front_insert)
{
	ERR_FAIL_COND_MSG(p_autoload.name == String(), "Trying to add autoload with no name.");
	if (p_front_insert) {
		if (data->autoloads.has(p_autoload.name)) {
			data->autoloads.erase(p_autoload.name);
		}
		data->autoloads.insert(p_autoload.name, p_autoload, true);
	}
	else {
		data->autoloads[p_autoload.name] = p_autoload;
	}
}

void ProjectSettings::remove_autoload(const String& p_autoload)
{
	ERR_FAIL_COND_MSG(!data->autoloads.has(p_autoload), "Trying to remove non-existent autoload.");
	data->autoloads.erase(p_autoload);
}

bool ProjectSettings::has_autoload(const String& p_autoload)
{
	return data->autoloads.has(p_autoload);
}

ProjectSettings::AutoloadInfo ProjectSettings::get_autoload(const String& p_name)
{
	ERR_FAIL_COND_V_MSG(
		!data->autoloads.has(p_name), AutoloadInfo(), "Trying to get non-existent autoload.");
	return data->autoloads[p_name];
}

void ProjectSettings::fix_autoload_paths()
{
	for (KeyValue<String, AutoloadInfo>& kv : data->autoloads) {
		kv.value.path = ResourceUID::ensure_path(kv.value.path);
	}
}

const HashMap<String, String>& ProjectSettings::get_global_groups_list()
{
	return data->global_groups;
}

void ProjectSettings::add_global_group(const String& p_name, const String& p_description)
{
	ERR_FAIL_COND_MSG(p_name == String(), "Trying to add global group with no name.");
	data->global_groups[p_name] = p_description;
}

void ProjectSettings::remove_global_group(const String& p_name)
{
	ERR_FAIL_COND_MSG(
		!data->global_groups.has(p_name), "Trying to remove non-existent global group.");
	data->global_groups.erase(p_name);
}

bool ProjectSettings::has_global_group(const String& p_name)
{
	return data->global_groups.has(p_name);
}

void ProjectSettings::remove_scene_groups_cache(const String& p_path)
{
	data->scene_groups_cache.erase(p_path);
}

void ProjectSettings::add_scene_groups_cache(const String& p_path, const HashSet<String>& p_cache)
{
	data->scene_groups_cache[p_path] = p_cache;
}

void ProjectSettings::save_scene_groups_cache()
{
	Ref<ConfigFile> cf;
	cf.instantiate();
	for (const KeyValue<String, HashSet<String>>& E : data->scene_groups_cache) {
		if (E.value.is_empty()) {
			continue;
		}
	}
	cf->save(get_scene_groups_cache_path());
}

String ProjectSettings::get_scene_groups_cache_path()
{
	return get_project_data_path().path_join("scene_groups_cache.cfg");
}

const HashMap<String, HashSet<String>>& ProjectSettings::get_scene_groups_cache()
{
	return data->scene_groups_cache;
}

bool ProjectSettings::has_editor_setting_override(const String& p_setting)
{
	return has_setting(EDITOR_SETTING_OVERRIDE_PREFIX + p_setting);
}

bool ProjectSettings::has_setting(const String& p_var) { return true; }

void ProjectSettings::load_scene_groups_cache() {}

void ProjectSettings::_convert_to_last_version(int p_from_version) {}

void ProjectSettings::refresh_global_class_list() {}

void ProjectSettings::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options)
{
}

bool ProjectSettings::is_initialized() { return data != nullptr; }

void ProjectSettings::initialize()
{
	if (!data) {
		data = memnew(Data);
		vltr_project_init(&data->project, "Voltaire Project", "");
		_add_builtin_input_map();
	}
}

void ProjectSettings::finalize()
{
	memdelete(data);
	data = nullptr;
}

void ProjectSettings::_add_builtin_input_map() {}

static Error _load_settings_vltr(const String& p_path, VoltaireProjectFile& r_out)
{
	Error err = OK;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (f.is_null()) {
		return err != OK ? err : ERR_FILE_NOT_FOUND;
	}
	if (f->get_length() < (int64_t)
sizeof(VoltaireProjectFile)) {
		return ERR_FILE_CORRUPT;
	}
	VoltaireProjectFile loaded;
	f->get_buffer((uint8_t*)&loaded, sizeof(VoltaireProjectFile));
	if (!vltr_project_valid(&loaded)) {
		return ERR_FILE_CORRUPT;
	}
	r_out = loaded;
	return OK;
}

Error ProjectSettings::_load_settings_text(const String& p_path)
{
	if (p_path.ends_with(".vltr")) {
		Error err = _load_settings_vltr(p_path, data->project);
		if (err == OK) {
			data->last_save_time = FileAccess::get_modified_time(p_path);
		}
		return err;
	}

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (f.is_null()) {
		return ERR_FILE_NOT_FOUND;
	}
	data->last_save_time = FileAccess::get_modified_time(p_path);
	return OK;
}

Error ProjectSettings::_load_settings_text_or_binary(
	const String& p_vltr_path, const String& p_bin_path)
{
	// Attempt first to load the binary project.vltr file.
	Error err = _load_settings_vltr(p_vltr_path, data->project);
	if (err == OK) {
		data->last_save_time = FileAccess::get_modified_time(p_vltr_path);
		return OK;
	}

	// Fallback to legacy project.binary if present.
	return _load_settings_binary(p_bin_path);
}

Error ProjectSettings::load_custom(const String& p_path)
{
	if (p_path.ends_with(".vltr")) {
		return _load_settings_vltr(p_path, data->project);
	}
	if (p_path.ends_with(".binary")) {
		return _load_settings_binary(p_path);
	}
	return _load_settings_text(p_path);
}

// ============================================================================
// Merged from core/config/engine.cpp
// ============================================================================

#include "core/license.gen.h"
#include "core/config/project_settings.h"
#include "core/license.gen.h"
#include "core/version.h"
#include "core/config/engine.h"
#include "servers/rendering/rendering_device.h"

void Engine::_update_time_scale()
{
	_time_scale = _user_time_scale * _game_time_scale;
	user_ips = MAX(1, ips * _user_time_scale);
	max_user_physics_steps_per_frame =
		MAX(max_physics_steps_per_frame, max_physics_steps_per_frame * _user_time_scale);
}

void Engine::set_physics_ticks_per_second(int p_ips)
{
	ERR_FAIL_COND_MSG(p_ips <= 0, "Engine iterations per second must be greater than 0.");
	ips = p_ips;
	_update_time_scale();
}

int Engine::get_physics_ticks_per_second() { return ips; }

int Engine::get_user_physics_ticks_per_second() { return user_ips; }

void Engine::set_max_physics_steps_per_frame(int p_max_physics_steps)
{
	ERR_FAIL_COND_MSG(p_max_physics_steps <= 0,
		"Maximum number of physics steps per frame must be greater than 0.");
	max_physics_steps_per_frame = p_max_physics_steps;
	_update_time_scale();
}

int Engine::get_max_physics_steps_per_frame() { return max_physics_steps_per_frame; }

int Engine::get_user_max_physics_steps_per_frame()
{
	return max_user_physics_steps_per_frame;
}

void Engine::set_physics_jitter_fix(double p_threshold)
{
	if (p_threshold < 0) {
		p_threshold = 0;
	}
	physics_jitter_fix = p_threshold;
}

double Engine::get_physics_jitter_fix() { return physics_jitter_fix; }

void Engine::set_max_fps(int p_fps)
{
	_max_fps = p_fps > 0 ? p_fps : 0;

	if (RD::data) {
		RD::_set_max_fps(_max_fps);
	}
}

int Engine::get_max_fps() { return _max_fps; }

void Engine::set_audio_output_latency(int p_msec)
{
	_audio_output_latency = p_msec > 1 ? p_msec : 1;
}

int Engine::get_audio_output_latency() { return _audio_output_latency; }

void Engine::increment_frames_drawn()
{
	if (frame_server_synced) {
		server_syncs++;
	}
	else {
		server_syncs = 0;
	}
	frame_server_synced = false;

	frames_drawn++;
}

uint64_t Engine::get_frames_drawn() { return frames_drawn; }

void Engine::set_frame_delay(uint32_t p_msec) { _frame_delay = p_msec; }

uint32_t Engine::get_frame_delay() { return _frame_delay; }

void Engine::set_time_scale(double p_scale)
{
	_game_time_scale = p_scale;
	_update_time_scale();
}

double Engine::get_time_scale() { return freeze_time_scale ? 0.0 : _game_time_scale; }

void Engine::set_user_time_scale(double p_scale)
{
	_user_time_scale = p_scale;
	_update_time_scale();
}

double Engine::get_effective_time_scale() { return freeze_time_scale ? 0.0 : _time_scale; }

double Engine::get_unfrozen_time_scale() { return _time_scale; }

String Engine::get_license_text() { return String(GODOT_LICENSE_TEXT); }

String Engine::get_architecture_name()
{
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
	return "x86_64";
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
	return "x86_32";
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
	return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
	return "arm32";
#elif defined(__riscv)
	return "rv64";
#elif defined(__powerpc64__)
	return "ppc64";
#elif defined(__loongarch64)
	return "loongarch64";
#elif defined(__wasm64__)
	return "wasm64";
#elif defined(__wasm32__)
	return "wasm32";
#endif
}

bool Engine::is_abort_on_gpu_errors_enabled() { return abort_on_gpu_errors; }

int32_t Engine::get_gpu_index() { return gpu_idx; }

bool Engine::is_validation_layers_enabled() { return use_validation_layers; }

bool Engine::is_generate_spirv_debug_info_enabled() { return generate_spirv_debug_info; }

bool Engine::is_extra_gpu_memory_tracking_enabled() { return extra_gpu_memory_tracking; }

#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
bool Engine::is_accurate_breadcrumbs_enabled() { return accurate_breadcrumbs; }
#endif

void Engine::set_print_to_stdout(bool p_enabled) { CoreGlobals::print_line_enabled = p_enabled; }

bool Engine::is_printing_to_stdout() { return CoreGlobals::print_line_enabled; }

void Engine::set_print_error_messages(bool p_enabled)
{
	CoreGlobals::print_error_enabled = p_enabled;
}

bool Engine::is_printing_error_messages() { return CoreGlobals::print_error_enabled; }

void Engine::print_header(const String& p_string)
{
	if (_print_header) {
		__print_line(p_string);
	}
}

void Engine::print_header_rich(const String& p_string)
{
	if (_print_header) {
		__print_line_rich(p_string);
	}
}

String Engine::get_write_movie_path() { return write_movie_path; }

void Engine::set_write_movie_path(const String& p_path) { write_movie_path = p_path; }

void Engine::set_shader_cache_path(const String& p_path) { shader_cache_path = p_path; }

String Engine::get_shader_cache_path() { return shader_cache_path; }

bool Engine::notify_frame_server_synced()
{
	frame_server_synced = true;
	return server_syncs > SERVER_SYNC_FRAME_COUNT_WARNING;
}

void Engine::set_freeze_time_scale(bool p_frozen) { freeze_time_scale = p_frozen; }

void Engine::set_embedded_in_editor(bool p_enabled) { embedded_in_editor = p_enabled; }

bool Engine::is_embedded_in_editor() { return embedded_in_editor; }



// ============================================================================
// Merged from core/error/error_macros.cpp
// ============================================================================

#include "core/error/error_macros.h"

#include "core/types.h"
#include "core/os/os.h"
#include "core/string/ustring.h"

#include <cstdio>

// Optional physics interpolation warnings try to include the path to the relevant node.
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
#include "core/config/project_settings.h"
#include "scene/main/node.h"
#endif

static ErrorHandlerList *error_handler_list = nullptr;
static thread_local bool is_printing_error = false;

static void _err_print_fallback(const char *p_function, const char *p_file, int p_line, const char *p_error_details, ErrorHandlerType p_type, bool p_reentrance) {
	if (p_reentrance) {
		fprintf(stderr, "While attempting to print an error, another error was printed:\n");
	}

	fprintf(stderr, "%s: %s\n", _error_handler_type_string(p_type), p_error_details);

	if (p_function && p_file) {
		fprintf(stderr, "   at: %s (%s:%i)\n", p_function, p_file, p_line);
	}
}

void add_error_handler(ErrorHandlerList *p_handler) {
	// If p_handler is already in error_handler_list
	// we'd better remove it first then we can add it.
	// This prevent cyclic redundancy.
	remove_error_handler(p_handler);

	_global_lock();

	p_handler->next = error_handler_list;
	error_handler_list = p_handler;

	_global_unlock();
}

void remove_error_handler(const ErrorHandlerList *p_handler) {
	_global_lock();

	ErrorHandlerList *prev = nullptr;
	ErrorHandlerList *l = error_handler_list;

	while (l) {
		if (l == p_handler) {
			if (prev) {
				prev->next = l->next;
			} else {
				error_handler_list = l->next;
			}
			break;
		}
		prev = l;
		l = l->next;
	}

	_global_unlock();
}

// Errors without messages.
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, "", p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), "", p_editor_notify, p_type);
}

// Main error printing function.
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	if (!CoreGlobals::print_ready) {
		const char *err_details = (p_message && *p_message) ? p_message : p_error;
		_err_print_fallback(p_function, p_file, p_line, err_details, p_type, false);
		return;
	}

	if (is_printing_error) {
		// Fallback if we're already printing an error, to prevent infinite recursion.
		const char *err_details = (p_message && *p_message) ? p_message : p_error;
		_err_print_fallback(p_function, p_file, p_line, err_details, p_type, true);
		return;
	}

	is_printing_error = true;

	// Fallback if errors happen before OS init or after it's destroyed.
	const char *err_details = (p_message && *p_message) ? p_message : p_error;
	_err_print_fallback(p_function, p_file, p_line, err_details, p_type, false);

	_global_lock();

	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
		l = l->next;
	}

	_global_unlock();

	is_printing_error = false;
}

// For printing errors when we may crash at any point, so we must flush ASAP a lot of lines
// but we don't want to make it noisy by printing lots of file & line info (because it's already
// been printing by a preceding _err_print_error).
void _err_print_error_asap(const String &p_error, ErrorHandlerType p_type) {
	const CharString err_details_str = p_error.utf8();
	const char *err_details = err_details_str.get_data();

	if (!CoreGlobals::print_ready) {
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, false);
		return;
	}

	if (is_printing_error) {
		// Fallback if we're already printing an error, to prevent infinite recursion.
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, true);
		return;
	}

	is_printing_error = true;
	if (OS::is_initialized()) {
		OS::printerr("%s: %s\n", _error_handler_type_string(p_type), err_details);
	} else {
		// Fallback if errors happen before OS init or after it's destroyed.
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, false);
	}

	_global_lock();

	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, "", "", 0, err_details, "", false, p_type);
		l = l->next;
	}

	_global_unlock();

	is_printing_error = false;
}

// Errors with message. (All combinations of p_error and p_message as String or char*.)
void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message, p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const String &p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, p_message.utf8().get_data(), p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const String &p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message.utf8().get_data(), p_editor_notify, p_type);
}

// Index errors. (All combinations of p_message as String or char*.)
void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const char *p_message, bool p_editor_notify, bool p_fatal) {
	String fstr(p_fatal ? "FATAL: " : "");
	String err(fstr + "Index " + p_index_str + " = " + itos(p_index) + " is out of bounds (" + p_size_str + " = " + itos(p_size) + ").");
	_err_print_error(p_function, p_file, p_line, err.utf8().get_data(), p_message, p_editor_notify, ERR_HANDLER_ERROR);
}

void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const String &p_message, bool p_editor_notify, bool p_fatal) {
	_err_print_index_error(p_function, p_file, p_line, p_index, p_size, p_index_str, p_size_str, p_message.utf8().get_data(), p_editor_notify, p_fatal);
}

void _err_flush_stdout() {
	fflush(stdout);
}

// Prevent error spam by limiting the warnings to a certain frequency.
void _physics_interpolation_warning(const char *p_function, const char *p_file, int p_line, const char *p_warn_string) {
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
	const uint32_t warn_max = 2048;
	const uint32_t warn_timeout_seconds = 15;

	static uint32_t warn_count = warn_max;
	static uint32_t warn_timeout = warn_timeout_seconds;

	uint32_t time_now = UINT32_MAX;

	if (warn_count) {
		warn_count--;
	}

	if (!warn_count) {
		time_now = OS::get_ticks_msec() / 1000;
	}

	if ((warn_count == 0) && (time_now >= warn_timeout)) {
		warn_count = warn_max;
		warn_timeout = time_now + warn_timeout_seconds;
	}
#endif
}

// ============================================================================
// Merged from core/error/error_list.cpp
// ============================================================================

#include "core/typedefs.h"

#include "core/string/ustring.h"
#include "core/typedefs.h"

const char *error_names[] = {
	TTRC("OK"), // OK
	TTRC("Failed"), // FAILED
	TTRC("Unavailable"), // ERR_UNAVAILABLE
	TTRC("Unconfigured"), // ERR_UNCONFIGURED
	TTRC("Unauthorized"), // ERR_UNAUTHORIZED
	TTRC("Parameter out of range"), // ERR_PARAMETER_RANGE_ERROR
	TTRC("Out of memory"), // ERR_OUT_OF_MEMORY
	TTRC("File not found"), // ERR_FILE_NOT_FOUND
	TTRC("File: Bad drive"), // ERR_FILE_BAD_DRIVE
	TTRC("File: Bad path"), // ERR_FILE_BAD_PATH
	TTRC("File: Permission denied"), // ERR_FILE_NO_PERMISSION
	TTRC("File already in use"), // ERR_FILE_ALREADY_IN_USE
	TTRC("Can't open file"), // ERR_FILE_CANT_OPEN
	TTRC("Can't write file"), // ERR_FILE_CANT_WRITE
	TTRC("Can't read file"), // ERR_FILE_CANT_READ
	TTRC("File unrecognized"), // ERR_FILE_UNRECOGNIZED
	TTRC("File corrupt"), // ERR_FILE_CORRUPT
	TTRC("Missing dependencies for file"), // ERR_FILE_MISSING_DEPENDENCIES
	TTRC("End of file"), // ERR_FILE_EOF
	TTRC("Can't open"), // ERR_CANT_OPEN
	TTRC("Can't create"), // ERR_CANT_CREATE
	TTRC("Query failed"), // ERR_QUERY_FAILED
	TTRC("Already in use"), // ERR_ALREADY_IN_USE
	TTRC("Locked"), // ERR_LOCKED
	TTRC("Timeout"), // ERR_TIMEOUT
	TTRC("Can't connect"), // ERR_CANT_CONNECT
	TTRC("Can't resolve"), // ERR_CANT_RESOLVE
	TTRC("Connection error"), // ERR_CONNECTION_ERROR
	TTRC("Can't acquire resource"), // ERR_CANT_ACQUIRE_RESOURCE
	TTRC("Can't fork"), // ERR_CANT_FORK
	TTRC("Invalid data"), // ERR_INVALID_DATA
	TTRC("Invalid parameter"), // ERR_INVALID_PARAMETER
	TTRC("Already exists"), // ERR_ALREADY_EXISTS
	TTRC("Does not exist"), // ERR_DOES_NOT_EXIST
	TTRC("Can't read database"), // ERR_DATABASE_CANT_READ
	TTRC("Can't write database"), // ERR_DATABASE_CANT_WRITE
	TTRC("Compilation failed"), // ERR_COMPILATION_FAILED
	TTRC("Method not found"), // ERR_METHOD_NOT_FOUND
	TTRC("Link failed"), // ERR_LINK_FAILED
	TTRC("Script failed"), // ERR_SCRIPT_FAILED
	TTRC("Cyclic link detected"), // ERR_CYCLIC_LINK
	TTRC("Invalid declaration"), // ERR_INVALID_DECLARATION
	TTRC("Duplicate symbol"), // ERR_DUPLICATE_SYMBOL
	TTRC("Parse error"), // ERR_PARSE_ERROR
	TTRC("Busy"), // ERR_BUSY
	TTRC("Skip"), // ERR_SKIP
	TTRC("Help"), // ERR_HELP
	TTRC("Bug"), // ERR_BUG
	TTRC("Printer on fire"), // ERR_PRINTER_ON_FIRE
};

static_assert(std_size(error_names) == ERR_MAX);

// ============================================================================
// Merged from core/templates/hashfuncs.cpp
// ============================================================================

#include "core/templates/hashfuncs.h"

uint32_t hash_murmur3_one_float(float p_in, uint32_t p_seed) {
	union {
		float f;
		uint32_t i;
	} u;

	// Normalize +/- 0.0 and NaN values so they hash the same.
	if (p_in == 0.0f) {
		u.f = 0.0;
	} else if (Math::is_nan(p_in)) {
		u.f = Math::NaN;
	} else {
		u.f = p_in;
	}

	return hash_murmur3_one_32(u.i, p_seed);
}

uint32_t hash_murmur3_one_double(double p_in, uint32_t p_seed) {
	union {
		double d;
		uint64_t i;
	} u;

	// Normalize +/- 0.0 and NaN values so they hash the same.
	if (p_in == 0.0f) {
		u.d = 0.0;
	} else if (Math::is_nan(p_in)) {
		u.d = Math::NaN;
	} else {
		u.d = p_in;
	}

	return hash_murmur3_one_64(u.i, p_seed);
}

uint32_t hash_murmur3_buffer(const void *p_key, int p_length, const uint32_t p_seed) {
	// Although not required, this is a random prime number.
	const uint8_t *data = (const uint8_t *)p_key;
	const int nblocks = p_length / 4;

	uint32_t h1 = p_seed;

	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;

	const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);

	for (int i = -nblocks; i; i++) {
		uint32_t k1 = blocks[i];

		k1 *= c1;
		k1 = hash_rotl32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = hash_rotl32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);

	uint32_t k1 = 0;

	switch (p_length & 3) {
		case 3:
			k1 ^= tail[2] << 16;
			[[fallthrough]];
		case 2:
			k1 ^= tail[1] << 8;
			[[fallthrough]];
		case 1:
			k1 ^= tail[0];
			k1 *= c1;
			k1 = hash_rotl32(k1, 15);
			k1 *= c2;
			h1 ^= k1;
	};

	// Finalize with additional bit mixing.
	h1 ^= p_length;
	return hash_fmix32(h1);
}

uint32_t hash_djb2_one_float(double p_in, uint32_t p_prev) {
	union {
		double d;
		uint64_t i;
	} u;

	// Normalize +/- 0.0 and NaN values so they hash the same.
	if (p_in == 0.0f) {
		u.d = 0.0;
	} else if (Math::is_nan(p_in)) {
		u.d = Math::NaN;
	} else {
		u.d = p_in;
	}

	return ((p_prev << 5) + p_prev) + hash_one_uint64(u.i);
}

uint64_t hash_djb2_one_float_64(double p_in, uint64_t p_prev) {
	union {
		double d;
		uint64_t i;
	} u;

	// Normalize +/- 0.0 and NaN values so they hash the same.
	if (p_in == 0.0f) {
		u.d = 0.0;
	} else if (Math::is_nan(p_in)) {
		u.d = Math::NaN;
	} else {
		u.d = p_in;
	}

	return ((p_prev << 5) + p_prev) + u.i;
}

// ============================================================================
// Merged from core/templates/a_hash_map.cpp
// ============================================================================

#include "core/templates/a_hash_map.h"

// Explicit instantiation.
template class AHashMap<int, int>;
template class AHashMap<String, int>;
template class AHashMap<String, String>;

// ============================================================================
// Merged from core/templates/interpolated_property.cpp
// ============================================================================

#include "scene/3d/node_3d.h"

#include "core/math/vector2.h"

namespace InterpolatedPropertyFuncs {

float lerp(float p_a, float p_b, float p_fraction) {
	return Math::lerp(p_a, p_b, p_fraction);
}

double lerp(double p_a, double p_b, float p_fraction) {
	return Math::lerp(p_a, p_b, (double)p_fraction);
}

Vector2 lerp(const Vector2 &p_a, const Vector2 &p_b, float p_fraction) {
	return p_a.lerp(p_b, p_fraction);
}

} //namespace InterpolatedPropertyFuncs


// --- Merged from core/input/input.cpp ---
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/os.h"
#include "core/input/input.h"

// --- input.compat.inc ---
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

#ifndef DISABLE_DEPRECATED


void Input::_vibrate_handheld_bind_compat_91143(int p_duration_ms)
{
	vibrate_handheld(p_duration_ms, -1.0);
}

void Input::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


#ifdef DEV_ENABLED
#include "core/os/thread.h"
#endif

#include <thirdparty/gamepadmotionhelpers/GamepadMotion.hpp>

#define STANDARD_GRAVITY 9.80665f

static const char* _joy_buttons[(size_t)JoyButton::SDL_MAX] = {
	"a",
	"b",
	"x",
	"y",
	"back",
	"guide",
	"start",
	"leftstick",
	"rightstick",
	"leftshoulder",
	"rightshoulder",
	"dpup",
	"dpdown",
	"dpleft",
	"dpright",
	"misc1",
	"paddle1",
	"paddle2",
	"paddle3",
	"paddle4",
	"touchpad",
	"misc2",
	"misc3",
	"misc4",
	"misc5",
	"misc6",
};

static const char* _joy_axes[(size_t)JoyAxis::SDL_MAX] = {
	"leftx",
	"lefty",
	"rightx",
	"righty",
	"lefttrigger",
	"righttrigger",
};

void (*Input::set_mouse_mode_func)(Input::MouseMode) = nullptr;
Input::MouseMode (*Input::get_mouse_mode_func)() = nullptr;
void (*Input::set_mouse_mode_override_func)(Input::MouseMode) = nullptr;
Input::MouseMode (*Input::get_mouse_mode_override_func)() = nullptr;
void (*Input::set_mouse_mode_override_enabled_func)(bool) = nullptr;
bool (*Input::is_mouse_mode_override_enabled_func)() = nullptr;
void (*Input::warp_mouse_func)(const Vector2& p_position) = nullptr;
Input::CursorShape (*Input::get_current_cursor_shape_func)() = nullptr;
void (*Input::set_custom_mouse_cursor_func)(
	const Ref<Resource>&, Input::CursorShape, const Vector2&) = nullptr;

void Input::set_mouse_mode(MouseMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, MouseMode::MOUSE_MODE_MAX);
	if (set_mouse_mode_func) {
		set_mouse_mode_func(p_mode);
	}
}

Input::MouseMode Input::get_mouse_mode()
{
	return get_mouse_mode_func ? get_mouse_mode_func() : MouseMode::MOUSE_MODE_VISIBLE;
}

void Input::set_mouse_mode_override(MouseMode p_mode)
{
	ERR_FAIL_INDEX(p_mode, MouseMode::MOUSE_MODE_MAX);
	if (set_mouse_mode_override_func) {
		set_mouse_mode_override_func(p_mode);
	}
}

Input::MouseMode Input::get_mouse_mode_override()
{
	return get_mouse_mode_override_func ? get_mouse_mode_override_func()
										: MouseMode::MOUSE_MODE_VISIBLE;
}

void Input::set_mouse_mode_override_enabled(bool p_override_enabled)
{
	if (set_mouse_mode_override_enabled_func) {
		set_mouse_mode_override_enabled_func(p_override_enabled);
	}
}

bool Input::is_mouse_mode_override_enabled()
{
	return is_mouse_mode_override_enabled_func ? is_mouse_mode_override_enabled_func() : false;
}

void Input::VelocityTrack::update(const Vector2& p_delta_p, const Vector2& p_screen_delta_p)
{
	uint64_t tick = OS::get_ticks_usec();
	uint32_t tdiff = tick - last_tick;
	float delta_t = tdiff / 1000000.0;
	last_tick = tick;

	if (delta_t > max_ref_frame) {
		// First movement in a long time, reset and start again.
		velocity = Vector2();
		screen_velocity = Vector2();
		accum = p_delta_p;
		screen_accum = p_screen_delta_p;
		accum_t = 0;
		return;
	}

	accum += p_delta_p;
	screen_accum += p_screen_delta_p;
	accum_t += delta_t;

	if (accum_t < min_ref_frame) {
		// Not enough time has passed to calculate speed precisely.
		return;
	}

	velocity = accum / accum_t;
	screen_velocity = screen_accum / accum_t;
	accum = Vector2();
	screen_accum = Vector2();
	accum_t = 0;
}

void Input::VelocityTrack::reset()
{
	last_tick = OS::get_ticks_usec();
	velocity = Vector2();
	screen_velocity = Vector2();
	accum = Vector2();
	screen_accum = Vector2();
	accum_t = 0;
}

Input::VelocityTrack::VelocityTrack()
{
	min_ref_frame = 0.1;
	max_ref_frame = 3.0;
	reset();
}

bool Input::is_anything_pressed()
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	if (!data->keys_pressed.is_empty() || !data->joy_buttons_pressed.is_empty()) {
		return true;
	}

	for (const KeyValue<String, Input::ActionState>& E : data->action_states) {
		if (E.value.cache.pressed) {
			return true;
		}
	}

	return false;
}

bool Input::is_any_key_pressed()
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return !data->keys_pressed.is_empty();
}

bool Input::is_key_pressed(Key p_keycode)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->keys_pressed.has(p_keycode);
}

bool Input::is_physical_key_pressed(Key p_keycode)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->physical_keys_pressed.has(p_keycode);
}

bool Input::is_key_label_pressed(Key p_keycode)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->key_label_pressed.has(p_keycode);
}

bool Input::is_mouse_button_pressed(MouseButton p_button)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	if (data->disable_input) {
		return false;
	}

	return (data->mouse_button_mask & (uint32_t)mouse_button_to_mask(p_button)) != 0;
}

bool Input::_should_ignore_joypad_events()
{
	return data->ignore_joypad_on_unfocused_application && !data->application_focused && !data->embedder_focused;
}

static JoyAxis _combine_device(JoyAxis p_value, int p_device)
{
	return JoyAxis((int)p_value | (p_device << 20));
}

static JoyButton _combine_device(JoyButton p_value, int p_device)
{
	return JoyButton((int)p_value | (p_device << 20));
}

bool Input::is_joy_button_pressed(int p_device, JoyButton p_button)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return false;
	}

	return data->joy_buttons_pressed.has(_combine_device(p_button, p_device));
}

bool Input::is_action_pressed(const String& p_action, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	return E->value.cache.pressed && (p_exact ? E->value.exact : true);
}

bool Input::is_action_just_pressed(const String& p_action, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently pressed.
	bool pressed_requirement = data->legacy_just_pressed_behavior ? E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return pressed_requirement &&
			   E->value.pressed_physics_frame == Engine::get_physics_frames();
	}
	else {
		return pressed_requirement &&
			   E->value.pressed_process_frame == Engine::get_process_frames();
	}
}

bool Input::is_action_just_pressed_by_event(
	const String& p_action, const InputEvent& p_event, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));
	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently pressed.
	bool pressed_requirement = data->legacy_just_pressed_behavior ? E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return pressed_requirement &&
			   E->value.pressed_physics_frame == Engine::get_physics_frames();
	}
	else {
		return pressed_requirement &&
			   E->value.pressed_process_frame == Engine::get_process_frames();
	}
}

bool Input::is_action_just_released(const String& p_action, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently released.
	bool released_requirement = data->legacy_just_pressed_behavior ? !E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return released_requirement &&
			   E->value.released_physics_frame == Engine::get_physics_frames();
	}
	else {
		return released_requirement &&
			   E->value.released_process_frame == Engine::get_process_frames();
	}
}

bool Input::is_action_just_released_by_event(
	const String& p_action, const InputEvent& p_event, bool p_exact)
{
	if (!data) {
		return false;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), false, InputMap::suggest_actions(p_action));
	if (data->disable_input) {
		return false;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return false;
	}

	if (p_exact && E->value.exact == false) {
		return false;
	}

	// Backward compatibility for legacy behavior, only return true if currently released.
	bool released_requirement = data->legacy_just_pressed_behavior ? !E->value.cache.pressed : true;

	if (Engine::is_in_physics_frame()) {
		return released_requirement &&
			   E->value.released_physics_frame == Engine::get_physics_frames();
	}
	else {
		return released_requirement &&
			   E->value.released_process_frame == Engine::get_process_frames();
	}
}

float Input::get_action_strength(const String& p_action, bool p_exact)
{
	if (!data) {
		return 0.0f;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), 0.0, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return 0.0f;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return 0.0f;
	}

	if (p_exact && E->value.exact == false) {
		return 0.0f;
	}

	return E->value.cache.strength;
}

float Input::get_action_raw_strength(const String& p_action, bool p_exact)
{
	if (!data) {
		return 0.0f;
	}
	ERR_FAIL_COND_V_MSG(!InputMap::has_action(p_action), 0.0, InputMap::suggest_actions(p_action));

	if (data->disable_input) {
		return 0.0f;
	}

	HashMap<String, ActionState>::ConstIterator E = data->action_states.find(p_action);
	if (!E) {
		return 0.0f;
	}

	if (p_exact && E->value.exact == false) {
		return 0.0f;
	}

	return E->value.cache.raw_strength;
}

float Input::get_axis(
	const String& p_negative_action, const String& p_positive_action)
{
	return get_action_strength(p_positive_action) - get_action_strength(p_negative_action);
}

Vector2 Input::get_vector(const String& p_negative_x, const String& p_positive_x,
	const String& p_negative_y, const String& p_positive_y, float p_deadzone)
{
	Vector2 vector =
		Vector2(get_action_raw_strength(p_positive_x) - get_action_raw_strength(p_negative_x),
			get_action_raw_strength(p_positive_y) - get_action_raw_strength(p_negative_y));

	if (p_deadzone < 0.0f) {
		// If the deadzone isn't specified, get it from the average of the actions.
		p_deadzone = 0.25 * (InputMap::action_get_deadzone(p_positive_x) +
								InputMap::action_get_deadzone(p_negative_x) +
								InputMap::action_get_deadzone(p_positive_y) +
								InputMap::action_get_deadzone(p_negative_y));
	}

	// Circular length limiting and deadzone.
	float length = vector.length();
	if (length <= p_deadzone) {
		return Vector2();
	}
	else if (length > 1.0f) {
		return vector / length;
	}
	else {
		// Inverse lerp length to map (p_deadzone, 1) to (0, 1).
		return vector * (Math::inverse_lerp(p_deadzone, 1.0f, length) / length);
	}
}

float Input::get_joy_axis(int p_device, JoyAxis p_axis)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_

	if (data->disable_input) {
		return 0;
	}

	JoyAxis c = _combine_device(p_axis, p_device);
	if (data->_joy_axis.has(c)) {
		return data->_joy_axis[c];
	}
	else {
		return 0;
	}
}

String Input::get_joy_name(int p_idx)
{
	if (!data) {
		return String();
	}
	_THREAD_SAFE_METHOD_
	return data->joy_names[p_idx].name;
}

Vector2 Input::get_joy_vibration_strength(int p_device)
{
	if (!data) {
		return Vector2();
	}
	if (data->joy_vibration.has(p_device)) {
		return Vector2(
			data->joy_vibration[p_device].weak_magnitude, data->joy_vibration[p_device].strong_magnitude);
	}
	else {
		return Vector2(0, 0);
	}
}

uint64_t Input::get_joy_vibration_timestamp(int p_device)
{
	if (!data) {
		return 0;
	}
	if (data->joy_vibration.has(p_device)) {
		return data->joy_vibration[p_device].timestamp;
	}
	else {
		return 0;
	}
}

float Input::get_joy_vibration_duration(int p_device)
{
	if (!data) {
		return 0.0f;
	}
	if (data->joy_vibration.has(p_device)) {
		return data->joy_vibration[p_device].duration;
	}
	else {
		return 0.f;
	}
}

float Input::get_joy_vibration_remaining_duration(int p_device)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_
	const Joypad* joypad = data->joy_names.getptr(p_device);
	if (joypad == nullptr || !joypad->has_vibration) {
		return 0.f;
	}
	const VibrationInfo* vibration = data->joy_vibration.getptr(p_device);
	if (vibration == nullptr ||
		(vibration->weak_magnitude == 0.f && vibration->strong_magnitude == 0.f) ||
		vibration->duration < 0.f) {
		return 0.f;
	}
	float vibration_duration = vibration->duration;
	if (vibration_duration > 0xFFFF / 1000.f || vibration_duration == 0.f) {
		vibration_duration = 0xFFFF / 1000.f; // SDL_MAX_RUMBLE_DURATION_MS / 1000.f
	}
	return MAX(
		vibration_duration - (OS::get_ticks_usec() - vibration->timestamp) / 1e6,
		0.f);
}

bool Input::is_joy_vibrating(int p_device)
{
	return get_joy_vibration_remaining_duration(p_device) > 0.0f;
}

bool Input::has_joy_vibration(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const Joypad* joypad = data->joy_names.getptr(p_device);
	return joypad != nullptr && joypad->has_vibration;
}

static String _hex_str(uint8_t p_byte)
{
	static const char* dict = "0123456789abcdef";
	char ret[3];
	ret[2] = 0;

	ret[0] = dict[p_byte >> 4];
	ret[1] = dict[p_byte & 0xf];

	return ret;
}

Vector3 Input::get_gravity()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->gravity_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_gravity` is not enabled in project settings.");
	}
#endif

	return data->gravity;
}

Vector3 Input::get_accelerometer()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->accelerometer_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_accelerometer` is not enabled in project settings.");
	}
#endif

	return data->accelerometer;
}

Vector3 Input::get_magnetometer()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->magnetometer_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_magnetometer` is not enabled in project settings.");
	}
#endif

	return data->magnetometer;
}

Vector3 Input::get_gyroscope()
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

#if defined(DEBUG_ENABLED) && defined(ANDROID_ENABLED)
	if (!data->gyroscope_enabled) {
		WARN_PRINT_ONCE(
			"`input_devices/sensors/enable_gyroscope` is not enabled in project settings.");
	}
#endif

	return data->gyroscope;
}

Vector2 Input::get_joy_touchpad_finger_position(int p_device, int p_finger, int p_touchpad)
{
	if (!data) {
		return Vector2();
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return Vector2(-1, -1);
	}

	uint16_t index = p_finger | (p_touchpad << 8);
	const TouchpadFingerInfo* finger_info = touch->finger_info.getptr(index);
	if (finger_info == nullptr) {
		return Vector2(-1, -1);
	}

	return finger_info->position;
}

float Input::get_joy_touchpad_finger_pressure(int p_device, int p_finger, int p_touchpad)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return -1.0f;
	}

	uint16_t index = p_finger | (p_touchpad << 8);
	const TouchpadFingerInfo* finger_info = touch->finger_info.getptr(index);
	if (finger_info == nullptr) {
		return -1.0f;
	}

	return finger_info->pressure;
}

Vector<uint32_t> Input::get_joy_touchpad_fingers(int p_device, int p_touchpad)
{
	if (!data) {
		return Vector<uint32_t>();
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return Vector<uint32_t>();
	}

	Vector<uint32_t> result;
	for (const KeyValue<uint16_t, TouchpadFingerInfo>& index : touch->finger_info) {
		int touchpad = index.key >> 8;
		if (touchpad == p_touchpad) {
			result.append(index.key & 0xFF);
		}
	}
	return result;
}

int Input::get_joy_num_touchpads(int p_device)
{
	if (!data) {
		return 0;
	}
	_THREAD_SAFE_METHOD_
	const TouchpadInfo* touch = data->joy_touch.getptr(p_device);
	if (touch == nullptr) {
		return 0;
	}
	return touch->num_touchpads;
}

void Input::_parse_input_event_impl(const InputEvent& p_event, bool p_is_emulated)
{
	// This function does the final delivery of the input event to user land.
	// Regardless where the event came from originally, this has to happen on the main thread.
	DEV_ASSERT(Thread::get_caller_id() == Thread::get_main_id());

	InputEvent dispatched_event = p_event;

	switch (dispatched_event.type) {
	case InputEventType::KEY: {
		if (!dispatched_event.key.echo) {
			if (dispatched_event.key.keycode != Key::NONE) {
				if (dispatched_event.is_pressed()) {
					data->keys_pressed.insert(dispatched_event.key.keycode);
				}
				else {
					data->keys_pressed.erase(dispatched_event.key.keycode);
				}
			}
			if (dispatched_event.key.physical_keycode != Key::NONE) {
				if (dispatched_event.is_pressed()) {
					data->physical_keys_pressed.insert(dispatched_event.key.physical_keycode);
				}
				else {
					data->physical_keys_pressed.erase(dispatched_event.key.physical_keycode);
				}
			}
			if (dispatched_event.key.key_label != Key::NONE) {
				if (dispatched_event.is_pressed()) {
					data->key_label_pressed.insert(dispatched_event.key.key_label);
				}
				else {
					data->key_label_pressed.erase(dispatched_event.key.key_label);
				}
			}
		}
	} break;

	case InputEventType::MOUSE_BUTTON: {
		if (dispatched_event.is_pressed()) {
			data->mouse_button_mask |=
				(uint32_t)mouse_button_to_mask(dispatched_event.mouse_button.button_index);
		}
		else {
			data->mouse_button_mask &=
				~(uint32_t)mouse_button_to_mask(dispatched_event.mouse_button.button_index);
		}

		Point2 pos = dispatched_event.mouse_button.global_position;
		if (data->mouse_pos != pos) {
			set_mouse_position(pos);
		}

		if (data->event_dispatch_function && data->emulate_touch_from_mouse && !p_is_emulated &&
			dispatched_event.mouse_button.button_index == MouseButton::LEFT) {
			InputEvent touch_event = InputEvent::make_screen_touch();
			touch_event.pressed = dispatched_event.is_pressed();
			touch_event.canceled = dispatched_event.is_canceled();
			touch_event.touch.position = dispatched_event.mouse_button.position;
			touch_event.touch.double_tap = dispatched_event.mouse_button.double_click;
			touch_event.window_id = dispatched_event.window_id;
			touch_event.device = InputEvent::DEVICE_ID_EMULATION;
			_THREAD_SAFE_UNLOCK_
			data->event_dispatch_function(touch_event);
			_THREAD_SAFE_LOCK_
		}
	} break;

	case InputEventType::MOUSE_MOTION: {
		Point2 position = dispatched_event.mouse_motion.global_position;
		if (data->mouse_pos != position) {
			set_mouse_position(position);
		}
		Vector2 relative = dispatched_event.mouse_motion.relative;
		Vector2 screen_relative = dispatched_event.mouse_motion.screen_relative;
		data->mouse_velocity_track.update(relative, screen_relative);

		if (data->event_dispatch_function && data->emulate_touch_from_mouse && !p_is_emulated &&
			(dispatched_event.mouse_motion.button_mask & (uint32_t)MouseButtonMask::LEFT) != 0) {
			InputEvent drag_event = InputEvent::make_screen_drag();
			drag_event.screen_drag.position = position;
			drag_event.screen_drag.relative = relative;
			drag_event.screen_drag.screen_relative = screen_relative;
			drag_event.screen_drag.tilt = dispatched_event.mouse_motion.tilt;
			drag_event.screen_drag.pen_inverted = dispatched_event.mouse_motion.pen_inverted;
			drag_event.screen_drag.pressure = dispatched_event.mouse_motion.pressure;
			drag_event.screen_drag.velocity = get_last_mouse_velocity();
			drag_event.screen_drag.screen_velocity = get_last_mouse_screen_velocity();
			drag_event.device = InputEvent::DEVICE_ID_EMULATION;

			_THREAD_SAFE_UNLOCK_
			data->event_dispatch_function(drag_event);
			_THREAD_SAFE_LOCK_
		}
	} break;

	case InputEventType::SCREEN_TOUCH: {
		if (dispatched_event.is_pressed()) {
			VelocityTrack& track = data->touch_velocity_track[dispatched_event.touch.index];
			track.reset();
		}
		else {
			data->touch_velocity_track.erase(dispatched_event.touch.index);
		}

		if (data->emulate_mouse_from_touch) {
			bool translate = false;
			if (dispatched_event.is_pressed()) {
				if (data->mouse_from_touch_index == -1) {
					translate = true;
					data->mouse_from_touch_index = dispatched_event.touch.index;
				}
			}
			else if (dispatched_event.touch.index == data->mouse_from_touch_index) {
				translate = true;
				data->mouse_from_touch_index = -1;
			}

			if (translate) {
				InputEvent button_event = InputEvent::make_mouse_button();
				button_event.device = InputEvent::DEVICE_ID_EMULATION;
				button_event.mouse_button.position = dispatched_event.touch.position;
				button_event.mouse_button.global_position = dispatched_event.touch.position;
				button_event.pressed = dispatched_event.is_pressed();
				button_event.canceled = dispatched_event.is_canceled();
				button_event.mouse_button.button_index = MouseButton::LEFT;
				button_event.mouse_button.double_click = dispatched_event.touch.double_tap;
				button_event.window_id = dispatched_event.window_id;

				uint32_t ev_bm = data->mouse_button_mask;
				if (dispatched_event.is_pressed()) {
					ev_bm |= (uint32_t)MouseButtonMask::LEFT;
				}
				else {
					ev_bm &= ~(uint32_t)MouseButtonMask::LEFT;
				}
				button_event.mouse_button.button_mask = ev_bm;

				_parse_input_event_impl(button_event, true);
			}
		}
	} break;

	case InputEventType::SCREEN_DRAG: {
		VelocityTrack& track = data->touch_velocity_track[dispatched_event.screen_drag.index];
		track.update(
			dispatched_event.screen_drag.relative, dispatched_event.screen_drag.screen_relative);
		dispatched_event.screen_drag.velocity = track.velocity;
		dispatched_event.screen_drag.screen_velocity = track.screen_velocity;

		if (data->emulate_mouse_from_touch &&
			dispatched_event.screen_drag.index == data->mouse_from_touch_index) {
			InputEvent motion_event = InputEvent::make_mouse_motion();
			motion_event.device = InputEvent::DEVICE_ID_EMULATION;
			motion_event.mouse_motion.tilt = dispatched_event.screen_drag.tilt;
			motion_event.mouse_motion.pen_inverted = dispatched_event.screen_drag.pen_inverted;
			motion_event.mouse_motion.pressure = dispatched_event.screen_drag.pressure;
			motion_event.mouse_motion.position = dispatched_event.screen_drag.position;
			motion_event.mouse_motion.global_position = dispatched_event.screen_drag.position;
			motion_event.mouse_motion.relative = dispatched_event.screen_drag.relative;
			motion_event.mouse_motion.screen_relative =
				dispatched_event.screen_drag.screen_relative;
			motion_event.mouse_motion.velocity = dispatched_event.screen_drag.velocity;
			motion_event.mouse_motion.screen_velocity =
				dispatched_event.screen_drag.screen_velocity;
			motion_event.mouse_motion.button_mask = data->mouse_button_mask;
			motion_event.window_id = dispatched_event.window_id;

			_parse_input_event_impl(motion_event, true);
		}
	} break;

	case InputEventType::JOY_BUTTON: {
		JoyButton c =
			_combine_device(dispatched_event.joy_button.button_index, dispatched_event.device);
		if (dispatched_event.is_pressed()) {
			data->joy_buttons_pressed.insert(c);
		}
		else {
			data->joy_buttons_pressed.erase(c);
		}
	} break;

	case InputEventType::JOY_MOTION: {
		set_joy_axis(dispatched_event.device, dispatched_event.joy_motion.axis,
			dispatched_event.joy_motion.axis_value);
	} break;

	case InputEventType::MAGNIFY_GESTURE:
	case InputEventType::PAN_GESTURE: {
		if (data->event_dispatch_function) {
			_THREAD_SAFE_UNLOCK_
			data->event_dispatch_function(dispatched_event);
			_THREAD_SAFE_LOCK_
		}
	} break;

	default:
		break;
	}

	for (const KeyValue<String, InputMap::Action>& E : InputMap::get_action_map()) {
		const int event_index = InputMap::event_get_index(dispatched_event, E.key);
		if (event_index == -1) {
			continue;
		}
		ERR_FAIL_COND_MSG(event_index >= (int)MAX_EVENT,
			vformat("Input singleton does not support more than %d events assigned to an action.",
				MAX_EVENT));

		int device_id = dispatched_event.device;
		bool is_pressed = dispatched_event.is_action_pressed(E.key, true);
		ActionState& action_state = data->action_states[E.key];

		// Update the action's per-device state.
		ActionState::DeviceState& device_state = action_state.device_states[device_id];
		device_state.pressed[event_index] = is_pressed;
		device_state.strength[event_index] = dispatched_event.get_action_strength(E.key);
		device_state.raw_strength[event_index] = dispatched_event.get_action_raw_strength(E.key);
		device_state.event_type[event_index] = dispatched_event.type;

		// Update the action's global state and cache.
		if (!is_pressed) {
			action_state.api_pressed =
				false; // Always release the event from action_press() method.
			action_state.api_strength = 0.0;
		}
		action_state.exact = InputMap::event_is_action(dispatched_event, E.key, true);

		bool was_pressed = action_state.cache.pressed;
		_update_action_cache(E.key, action_state);
		// As input may come in part way through a physics tick, the earliest we can react to it is
		// the next physics tick.
		if (action_state.cache.pressed && !was_pressed) {
			action_state.pressed_physics_frame = Engine::get_physics_frames() + 1;
			action_state.pressed_process_frame = Engine::get_process_frames();
		}
		if (!action_state.cache.pressed && was_pressed) {
			action_state.released_physics_frame = Engine::get_physics_frames() + 1;
			action_state.released_process_frame = Engine::get_process_frames();
		}
	}

	if (data->event_dispatch_function) {
		_THREAD_SAFE_UNLOCK_
		data->event_dispatch_function(dispatched_event);
		_THREAD_SAFE_LOCK_
	}
}

void Input::set_joy_axis(int p_device, JoyAxis p_axis, float p_value)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	JoyAxis c = _combine_device(p_axis, p_device);
	data->_joy_axis[c] = p_value;
}

void Input::set_joy_features(int p_device, JoypadFeatures* p_features)
{
	if (!data) {
		return;
	}
	Joypad* joypad = data->joy_names.getptr(p_device);
	if (!joypad) {
		return;
	}
	joypad->features = p_features;
	_update_joypad_features(p_device);
}

void Input::set_joy_light(int p_device, const Color& p_color)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	Joypad* joypad = data->joy_names.getptr(p_device);
	if (!joypad || !joypad->has_light || joypad->features == nullptr) {
		return;
	}
	Color linear = p_color.srgb_to_linear();
	joypad->features->set_joy_light(linear);
}

bool Input::has_joy_light(int p_device)
{
	if (!data) {
		return false;
	}
	const Joypad* joypad = data->joy_names.getptr(p_device);
	return joypad && joypad->has_light;
}

Vector3 Input::get_joy_accelerometer(int p_device)
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return Vector3();
	}

	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return Vector3();
	}

	float joy_acceleration_data[3];
	motion->gamepad_motion->GetProcessedAcceleration(
		joy_acceleration_data[0], joy_acceleration_data[1], joy_acceleration_data[2]);
	Vector3 joy_acceleration(
		joy_acceleration_data[0], joy_acceleration_data[1], joy_acceleration_data[2]);

	float joy_gravity_data[3];
	motion->gamepad_motion->GetGravity(
		joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);
	Vector3 joy_gravity(joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);

	return (-joy_acceleration + joy_gravity) * STANDARD_GRAVITY;
}

Vector3 Input::get_joy_gravity(int p_device)
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return Vector3();
	}

	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return Vector3();
	}

	float joy_gravity_data[3];
	motion->gamepad_motion->GetGravity(
		joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);
	Vector3 joy_gravity(joy_gravity_data[0], joy_gravity_data[1], joy_gravity_data[2]);

	return joy_gravity.normalized() * STANDARD_GRAVITY;
}

Vector3 Input::get_joy_gyroscope(int p_device)
{
	if (!data) {
		return Vector3();
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return Vector3();
	}

	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return Vector3();
	}

	float joy_gyro_data[3];
	motion->gamepad_motion->GetCalibratedGyro(joy_gyro_data[0], joy_gyro_data[1], joy_gyro_data[2]);
	Vector3 joy_gyro(joy_gyro_data[0], joy_gyro_data[1], joy_gyro_data[2]);

	return joy_gyro * M_PI / 180.0;
}

void Input::set_joy_motion_sensors_enabled(int p_device, bool p_enable)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	Joypad* joypad = data->joy_names.getptr(p_device);
	if (joypad == nullptr || joypad->features == nullptr) {
		return;
	}
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}
	joypad->features->set_joy_motion_sensors_enabled(p_enable);
	motion->sensors_enabled = p_enable;
}

bool Input::is_joy_motion_sensors_enabled(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	return motion != nullptr && motion->sensors_enabled;
}

bool Input::has_joy_motion_sensors(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	return data->joy_motion.has(p_device);
}

float Input::get_joy_motion_sensors_rate(int p_device)
{
	if (!data) {
		return 0.0f;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return 0.0f;
	}
	return motion->sensor_data_rate;
}

void Input::start_joy_motion_sensors_calibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	ERR_FAIL_COND_MSG(!motion->sensors_enabled, "Motion sensors are not enabled on the joypad.");
	ERR_FAIL_COND_MSG(motion->calibrating, "Calibration already in progress.");

	motion->gamepad_motion->ResetContinuousCalibration();
	motion->gamepad_motion->StartContinuousCalibration();

	motion->calibrating = true;
	motion->calibrated = false;
}

void Input::stop_joy_motion_sensors_calibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	ERR_FAIL_COND_MSG(!motion->sensors_enabled, "Motion sensors are not enabled on the joypad.");
	ERR_FAIL_COND_MSG(!motion->calibrating, "Calibration hasn't been started.");

	motion->gamepad_motion->PauseContinuousCalibration();

	motion->calibrating = false;
	motion->calibrated = true;
}

void Input::clear_joy_motion_sensors_calibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	// Calibration might be in progress and the developer or the user might want to reset it,
	// so no need to stop the calibration.

	motion->gamepad_motion->ResetContinuousCalibration();
}

bool Input::is_joy_motion_sensors_calibrating(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return false;
	}
	return motion->calibrating;
}

bool Input::is_joy_motion_sensors_calibrated(int p_device)
{
	if (!data) {
		return false;
	}
	_THREAD_SAFE_METHOD_
	const MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return false;
	}
	return motion->calibrated;
}

void Input::set_joy_motion_sensors_rate(int p_device, float p_rate)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	motion->sensor_data_rate = p_rate;
}

void Input::start_joy_vibration(
	int p_device, float p_weak_magnitude, float p_strong_magnitude, float p_duration)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	if (p_weak_magnitude < 0.f || p_weak_magnitude > 1.f || p_strong_magnitude < 0.f ||
		p_strong_magnitude > 1.f) {
		return;
	}
	VibrationInfo vibration;
	vibration.weak_magnitude = p_weak_magnitude;
	vibration.strong_magnitude = p_strong_magnitude;
	vibration.duration = p_duration;
	vibration.timestamp = OS::get_ticks_usec();
	data->joy_vibration[p_device] = vibration;
}

void Input::stop_joy_vibration(int p_device)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	VibrationInfo vibration;
	vibration.weak_magnitude = 0;
	vibration.strong_magnitude = 0;
	vibration.duration = 0;
	vibration.timestamp = OS::get_ticks_usec();
	data->joy_vibration[p_device] = vibration;
}

void Input::vibrate_handheld(int p_duration_ms, float p_amplitude)
{
	OS::vibrate_handheld(p_duration_ms, p_amplitude);
}

void Input::set_ignore_joypad_on_unfocused_application(bool p_ignore)
{
	if (!data) {
		return;
	}
	data->ignore_joypad_on_unfocused_application = p_ignore;
	if (_should_ignore_joypad_events()) {
		release_pressed_events();
	}
}

bool Input::is_ignoring_joypad_on_unfocused_application()
{
	return data ? data->ignore_joypad_on_unfocused_application : false;
}

void Input::set_application_focused(bool p_focused)
{
	if (data) {
		data->application_focused = p_focused;
	}
}

void Input::set_gravity(const Vector3& p_gravity)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->gravity = p_gravity;
}

void Input::set_accelerometer(const Vector3& p_accel)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->accelerometer = p_accel;
}

void Input::set_magnetometer(const Vector3& p_magnetometer)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->magnetometer = p_magnetometer;
}

void Input::set_gyroscope(const Vector3& p_gyroscope)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	data->gyroscope = p_gyroscope;
}

void Input::set_mouse_position(const Point2& p_posf)
{
	if (data) {
		data->mouse_pos = p_posf;
	}
}

Point2 Input::get_mouse_position() { return data ? data->mouse_pos : Point2(); }

Point2 Input::get_last_mouse_velocity()
{
	if (!data) {
		return Point2();
	}
	data->mouse_velocity_track.update(Vector2(), Vector2());
	return data->mouse_velocity_track.velocity;
}

Point2 Input::get_last_mouse_screen_velocity()
{
	if (!data) {
		return Point2();
	}
	data->mouse_velocity_track.update(Vector2(), Vector2());
	return data->mouse_velocity_track.screen_velocity;
}

uint32_t Input::get_mouse_button_mask()
{
	if (!data) {
		return 0;
	}
	return data->mouse_button_mask; // do not trust OS implementation, should remove it -
							  // OS::get_mouse_button_state();
}

void Input::warp_mouse(const Vector2& p_position)
{
	if (warp_mouse_func) {
		warp_mouse_func(p_position);
	}
}

Point2 Input::warp_mouse_motion(const InputEvent& p_motion, const Rect2& p_rect)
{
	ERR_FAIL_COND_V(p_motion.type != InputEventType::MOUSE_MOTION, Point2());

	const Point2 rel_sign(p_motion.mouse_motion.relative.x >= 0.0f ? 1 : -1,
		p_motion.mouse_motion.relative.y >= 0.0 ? 1 : -1);
	const Size2 warp_margin = p_rect.size * 0.5f;
	const Point2 rel_warped(
		Math::fmod(p_motion.mouse_motion.relative.x + rel_sign.x * warp_margin.x, p_rect.size.x) -
			rel_sign.x * warp_margin.x,
		Math::fmod(p_motion.mouse_motion.relative.y + rel_sign.y * warp_margin.y, p_rect.size.y) -
			rel_sign.y * warp_margin.y);

	const Point2 pos_local = p_motion.mouse_motion.global_position - p_rect.position;
	const Point2 pos_warped(
		Math::fposmod(pos_local.x, p_rect.size.x), Math::fposmod(pos_local.y, p_rect.size.y));
	if (pos_warped != pos_local) {
		warp_mouse(pos_warped + p_rect.position);
	}

	return rel_warped;
}

void Input::action_press(const String& p_action, float p_strength)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(!InputMap::has_action(p_action), InputMap::suggest_actions(p_action));

	// Create or retrieve existing action.
	ActionState& action_state = data->action_states[p_action];

	// As input may come in part way through a physics tick, the earliest we can react to it is the
	// next physics tick.
	if (!action_state.cache.pressed) {
		action_state.pressed_physics_frame = Engine::get_physics_frames() + 1;
		action_state.pressed_process_frame = Engine::get_process_frames();
	}
	action_state.exact = true;
	action_state.api_pressed = true;
	action_state.api_strength = CLAMP(p_strength, 0.0f, 1.0f);
	_update_action_cache(p_action, action_state);
}

void Input::action_release(const String& p_action)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(!InputMap::has_action(p_action), InputMap::suggest_actions(p_action));

	// Create or retrieve existing action.
	ActionState& action_state = data->action_states[p_action];
	action_state.cache.pressed = false;
	action_state.cache.strength = 0.0;
	action_state.cache.raw_strength = 0.0;
	// As input may come in part way through a physics tick, the earliest we can react to it is the
	// next physics tick.
	action_state.released_physics_frame = Engine::get_physics_frames() + 1;
	action_state.released_process_frame = Engine::get_process_frames();
	action_state.device_states.clear();
	action_state.exact = true;
	action_state.api_pressed = false;
	action_state.api_strength = 0.0;
}

void Input::set_emulate_touch_from_mouse(bool p_emulate)
{
	if (data) {
		data->emulate_touch_from_mouse = p_emulate;
	}
}

bool Input::is_emulating_touch_from_mouse() { return data ? data->emulate_touch_from_mouse : false; }

// Calling this whenever the game window is focused helps unsticking the "touch mouse"
// if the OS or its abstraction class hasn't properly reported that touch pointers raised
void Input::ensure_touch_mouse_raised()
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_
	if (data->mouse_from_touch_index != -1) {
		data->mouse_from_touch_index = -1;

		InputEvent button_event = InputEvent::make_mouse_button();
		button_event.device = InputEvent::DEVICE_ID_EMULATION;
		button_event.mouse_button.position = data->mouse_pos;
		button_event.mouse_button.global_position = data->mouse_pos;
		button_event.pressed = false;
		button_event.mouse_button.button_index = MouseButton::LEFT;
		uint32_t ev_bm = data->mouse_button_mask;
		ev_bm &= ~(uint32_t)MouseButtonMask::LEFT;
		button_event.mouse_button.button_mask = ev_bm;

		_parse_input_event_impl(button_event, true);
	}
}

void Input::set_emulate_mouse_from_touch(bool p_emulate)
{
	if (data) {
		data->emulate_mouse_from_touch = p_emulate;
	}
}

bool Input::is_emulating_mouse_from_touch() { return data ? data->emulate_mouse_from_touch : false; }

Input::CursorShape Input::get_default_cursor_shape() { return data ? data->default_shape : CursorShape::CURSOR_ARROW; }

void Input::set_default_cursor_shape(CursorShape p_shape)
{
	if (!data) {
		return;
	}
	if (data->default_shape == p_shape) {
		return;
	}

	data->default_shape = p_shape;
	// The default shape is set in Viewport::_gui_input_event. To instantly
	// see the shape in the viewport we need to trigger a mouse motion event.
	InputEvent mm = InputEvent::make_mouse_motion();
	mm.mouse_motion.position = data->mouse_pos;
	mm.mouse_motion.global_position = data->mouse_pos;
	mm.device = InputEvent::DEVICE_ID_INTERNAL;
	parse_input_event(mm);
}

Input::CursorShape Input::get_current_cursor_shape()
{
	return get_current_cursor_shape_func ? get_current_cursor_shape_func()
										 : CursorShape::CURSOR_ARROW;
}

void Input::set_custom_mouse_cursor(
	const Ref<Resource>& p_cursor, CursorShape p_shape, const Vector2& p_hotspot)
{
	if (Engine::is_editor_hint()) {
		return;
	}

	ERR_FAIL_INDEX(p_shape, CursorShape::CURSOR_MAX);

	if (set_custom_mouse_cursor_func) {
		set_custom_mouse_cursor_func(p_cursor, p_shape, p_hotspot);
	}
}

void Input::parse_input_event(const InputEvent& p_event)
{
	if (!data || p_event.is_null()) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (data->use_accumulated_input) {
		if (data->buffered_events.is_empty() ||
			!data->buffered_events.back()->get().accumulate(p_event)) {
			data->buffered_events.push_back(p_event);
		}
	}
	else if (data->agile_input_event_flushing) {
		data->buffered_events.push_back(p_event);
	}
	else {
		_parse_input_event_impl(p_event, false);
	}
}

void Input::flush_buffered_events()
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	while (data->buffered_events.front()) {
		List<InputEvent>::Element* E = data->buffered_events.front();
		InputEvent e = E->get();
		data->buffered_events.pop_front();

		_parse_input_event_impl(e, false);
	}
}

bool Input::is_agile_input_event_flushing() { return data ? data->agile_input_event_flushing : false; }

void Input::set_agile_input_event_flushing(bool p_enable)
{
	if (data) {
		data->agile_input_event_flushing = p_enable;
	}
}

void Input::set_use_accumulated_input(bool p_enable)
{
	if (data) {
		data->use_accumulated_input = p_enable;
	}
}

bool Input::is_using_accumulated_input() { return data ? data->use_accumulated_input : false; }

void Input::release_pressed_events()
{
	if (!data) {
		return;
	}
	// Don't release the events if the application (or the window it's embedded in) is still
	// focused.
	if (data->application_focused || data->embedder_focused) {
		return;
	}

	flush_buffered_events(); // this is needed to release actions strengths

	data->keys_pressed.clear();
	data->physical_keys_pressed.clear();
	data->key_label_pressed.clear();
	if (data->ignore_joypad_on_unfocused_application) {
		data->joy_buttons_pressed.clear();
		data->_joy_axis.clear();

		for (KeyValue<String, Input::ActionState>& E : data->action_states) {
			if (E.value.cache.pressed) {
				action_release(E.key);
			}
		}
	}
	else {
		for (KeyValue<String, Input::ActionState>& E : data->action_states) {
			if (E.value.cache.pressed) {
				bool is_only_joypad_pressed = true;
				int max_event = InputMap::action_get_events(E.key)->size() +
								1; // +1 comes from InputEventAction.

				for (const KeyValue<int, ActionState::DeviceState>& kv : E.value.device_states) {
					const ActionState::DeviceState& device_state = kv.value;
					for (int i = 0; i < max_event; i++) {
						if (device_state.event_type[i] != InputEventType::JOY_MOTION &&
							device_state.event_type[i] != InputEventType::JOY_BUTTON &&
							device_state.strength[i] > 0.0f) {
							is_only_joypad_pressed = false;
							action_release(E.key);
							break;
						}
					}
					if (!is_only_joypad_pressed) {
						break;
					}
				}
			}
		}
	}
}

void Input::set_event_dispatch_function(EventDispatchFunc p_function)
{
	if (!data) {
		return;
	}
	data->event_dispatch_function = p_function;
}

void Input::joy_button(int p_device, JoyButton p_button, bool p_pressed)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;

	if (_should_ignore_joypad_events()) {
		return;
	}

	Joypad& joy = data->joy_names[p_device];
	ERR_FAIL_INDEX((int)p_button, (int)JoyButton::MAX);

	if (joy.last_buttons[(size_t)p_button] == p_pressed) {
		return;
	}
	joy.last_buttons[(size_t)p_button] = p_pressed;
	if (joy.mapping == -1) {
		_button_event(p_device, p_button, p_pressed);
		return;
	}

	JoyEvent map = _get_mapped_button_event(data->map_db[joy.mapping], p_button);

	if (map.type == TYPE_BUTTON) {
		_button_event(p_device, (JoyButton)map.index, p_pressed);
		return;
	}

	if (map.type == TYPE_AXIS) {
		_axis_event(p_device, (JoyAxis)map.index, p_pressed ? map.value : 0.0);
	}
	// no event?
}

void Input::joy_axis(int p_device, JoyAxis p_axis, float p_value)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;

	if (_should_ignore_joypad_events()) {
		return;
	}

	ERR_FAIL_INDEX((int)p_axis, (int)JoyAxis::MAX);

	Joypad& joy = data->joy_names[p_device];

	if (joy.last_axis[(size_t)p_axis] == p_value) {
		return;
	}

	joy.last_axis[(size_t)p_axis] = p_value;

	if (joy.mapping == -1) {
		_axis_event(p_device, p_axis, p_value);
		return;
	}

	JoyAxisRange range;
	JoyEvent map = _get_mapped_axis_event(data->map_db[joy.mapping], p_axis, p_value, range);

	if (map.type == TYPE_BUTTON) {
		bool pressed = map.value > 0.5;
		if (pressed != data->joy_buttons_pressed.has(_combine_device((JoyButton)map.index, p_device))) {
			_button_event(p_device, (JoyButton)map.index, pressed);
		}

		// Ensure opposite D-Pad button is also released.
		switch ((JoyButton)map.index) {
		case JoyButton::DPAD_UP:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_DOWN, p_device))) {
				_button_event(p_device, JoyButton::DPAD_DOWN, false);
			}
			break;
		case JoyButton::DPAD_DOWN:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_UP, p_device))) {
				_button_event(p_device, JoyButton::DPAD_UP, false);
			}
			break;
		case JoyButton::DPAD_LEFT:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_RIGHT, p_device))) {
				_button_event(p_device, JoyButton::DPAD_RIGHT, false);
			}
			break;
		case JoyButton::DPAD_RIGHT:
			if (data->joy_buttons_pressed.has(_combine_device(JoyButton::DPAD_LEFT, p_device))) {
				_button_event(p_device, JoyButton::DPAD_LEFT, false);
			}
			break;
		default:
			// Nothing to do.
			break;
		}
		return;
	}

	if (map.type == TYPE_AXIS) {
		JoyAxis axis = JoyAxis(map.index);
		float value = map.value;
#ifndef ANDROID_ENABLED // Android trigger values are already between 0.0f and 1.0f.
		if (range == FULL_AXIS &&
			(axis == JoyAxis::TRIGGER_LEFT || axis == JoyAxis::TRIGGER_RIGHT)) {
			// Convert to a value between 0.0f and 1.0f.
			value = 0.5f + value / 2.0f;
		}
#endif
		_axis_event(p_device, axis, value);
		return;
	}
}

void Input::joy_hat(int p_device, uint32_t p_val)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;

	if (_should_ignore_joypad_events()) {
		return;
	}

	const Joypad& joy = data->joy_names[p_device];

	JoyEvent map[(size_t)HatDir::MAX];

	map[(size_t)HatDir::UP].type = TYPE_BUTTON;
	map[(size_t)HatDir::UP].index = (int)JoyButton::DPAD_UP;
	map[(size_t)HatDir::UP].value = 0;

	map[(size_t)HatDir::RIGHT].type = TYPE_BUTTON;
	map[(size_t)HatDir::RIGHT].index = (int)JoyButton::DPAD_RIGHT;
	map[(size_t)HatDir::RIGHT].value = 0;

	map[(size_t)HatDir::DOWN].type = TYPE_BUTTON;
	map[(size_t)HatDir::DOWN].index = (int)JoyButton::DPAD_DOWN;
	map[(size_t)HatDir::DOWN].value = 0;

	map[(size_t)HatDir::LEFT].type = TYPE_BUTTON;
	map[(size_t)HatDir::LEFT].index = (int)JoyButton::DPAD_LEFT;
	map[(size_t)HatDir::LEFT].value = 0;

	if (joy.mapping != -1) {
		_get_mapped_hat_events(data->map_db[joy.mapping], (HatDir)0, map);
	}

	int cur_val = data->joy_names[p_device].hat_current;

	for (int hat_direction = 0, hat_mask = 1; hat_direction < (int)HatDir::MAX;
		 hat_direction++, hat_mask <<= 1) {
		if (((int)p_val & hat_mask) != (cur_val & hat_mask)) {
			if (map[hat_direction].type == TYPE_BUTTON) {
				_button_event(p_device, (JoyButton)map[hat_direction].index, (int)p_val & hat_mask);
			}
			if (map[hat_direction].type == TYPE_AXIS) {
				_axis_event(p_device, (JoyAxis)map[hat_direction].index,
					((int)p_val & hat_mask) ? map[hat_direction].value : 0.0);
			}
		}
	}

	data->joy_names[p_device].hat_current = (int)p_val;
}

void Input::joy_motion_sensors(
	int p_device, const Vector3& p_accelerometer, const Vector3& p_gyroscope)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	// TODO: events
	MotionInfo* motion = data->joy_motion.getptr(p_device);
	if (motion == nullptr) {
		return;
	}

	Vector3 gyro_degrees = p_gyroscope * 180.0 / M_PI;
	Vector3 accel_g = -p_accelerometer / STANDARD_GRAVITY;
	uint64_t new_timestamp = OS::get_ticks_msec();
	float delta_time = (new_timestamp - motion->last_timestamp) / 1000.0f;
	motion->last_timestamp = new_timestamp;
	motion->gamepad_motion->ProcessMotion(gyro_degrees.x, gyro_degrees.y, gyro_degrees.z, accel_g.x,
		accel_g.y, accel_g.z, delta_time);
}

void Input::joy_touchpad(int p_device, int p_touchpad, int p_finger, const Vector2& p_position,
	float p_pressure, bool p_pressed)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_

	if (_should_ignore_joypad_events()) {
		return;
	}

	TouchpadInfo& touch = data->joy_touch[p_device];
	uint16_t index = p_finger | (p_touchpad << 8);
	if (p_pressed) {
		touch.finger_info[index] = TouchpadFingerInfo{p_position, p_pressure};
	}
	else {
		touch.finger_info.erase(index);
	}
}

void Input::_button_event(int p_device, JoyButton p_index, bool p_pressed)
{
	InputEvent ievent = InputEvent::make_joypad_button(p_index, p_device);
	ievent.pressed = p_pressed;
	parse_input_event(ievent);
}

void Input::_axis_event(int p_device, JoyAxis p_axis, float p_value)
{
	InputEvent ievent = InputEvent::make_joypad_motion(p_axis, p_value, p_device);
	parse_input_event(ievent);
}

void Input::_update_action_cache(const String& p_action_name, ActionState& r_action_state)
{
	// Update the action cache, computed from the per-device and per-event states.
	r_action_state.cache.pressed = false;
	r_action_state.cache.strength = 0.0;
	r_action_state.cache.raw_strength = 0.0;

	int max_event = InputMap::action_get_events(p_action_name)->size() +
					1; // +1 comes from InputEventAction.
	for (const KeyValue<int, ActionState::DeviceState>& kv : r_action_state.device_states) {
		const ActionState::DeviceState& device_state = kv.value;
		for (int i = 0; i < max_event; i++) {
			r_action_state.cache.pressed = r_action_state.cache.pressed || device_state.pressed[i];
			r_action_state.cache.strength =
				MAX(r_action_state.cache.strength, device_state.strength[i]);
			r_action_state.cache.raw_strength =
				MAX(r_action_state.cache.raw_strength, device_state.raw_strength[i]);
		}
	}

	if (r_action_state.api_pressed) {
		r_action_state.cache.pressed = true;
		r_action_state.cache.strength =
			MAX(r_action_state.cache.strength, r_action_state.api_strength);
		r_action_state.cache.raw_strength = MAX(r_action_state.cache.raw_strength,
			r_action_state
				.api_strength); // Use the strength as raw_strength for API-pressed states.
	}
}

void Input::_update_joypad_features(int p_device)
{
	Joypad* joypad = data->joy_names.getptr(p_device);
	if (!joypad || joypad->features == nullptr) {
		return;
	}
	if (joypad->features->has_joy_vibration()) {
		joypad->has_vibration = true;
	}
	if (joypad->features->has_joy_light()) {
		joypad->has_light = true;
	}
	if (joypad->features->has_joy_motion_sensors()) {
		MotionInfo& motion = data->joy_motion[p_device];

		if (!motion.gamepad_motion) {
			motion.gamepad_motion = new GamepadMotion();
		}
		else {
			motion.gamepad_motion->Reset();
		}
		motion.last_timestamp = OS::get_ticks_msec();
	}
	if (joypad->features->get_joy_num_touchpads() > 0) {
		data->joy_touch[p_device].num_touchpads = joypad->features->get_joy_num_touchpads();
	}
}

Input::JoyEvent Input::_get_mapped_button_event(
	const JoyDeviceMapping& p_mapping, JoyButton p_button)
{
	JoyEvent event;

	for (int i = 0; i < p_mapping.bindings.size(); i++) {
		const JoyBinding binding = p_mapping.bindings[i];
		if (binding.inputType == TYPE_BUTTON && binding.input.button == p_button) {
			event.type = binding.outputType;
			switch (binding.outputType) {
			case TYPE_BUTTON:
				event.index = (int)binding.output.button;
				return event;
			case TYPE_AXIS:
				event.index = (int)binding.output.axis.axis;
				switch (binding.output.axis.range) {
				case POSITIVE_HALF_AXIS:
					event.value = 1;
					break;
				case NEGATIVE_HALF_AXIS:
					event.value = -1;
					break;
				case FULL_AXIS:
					// It doesn't make sense for a button to map to a full axis,
					// but keeping as a default for a trigger with a positive half-axis.
					event.value = 1;
					break;
				}
				return event;
			default:
				ERR_PRINT_ONCE("Joypad button mapping error.");
			}
		}
	}
	return event;
}

Input::JoyEvent Input::_get_mapped_axis_event(
	const JoyDeviceMapping& p_mapping, JoyAxis p_axis, float p_value, JoyAxisRange& r_range)
{
	JoyEvent event;

	for (int i = 0; i < p_mapping.bindings.size(); i++) {
		const JoyBinding binding = p_mapping.bindings[i];
		if (binding.inputType == TYPE_AXIS && binding.input.axis.axis == p_axis) {
			float value = p_value;
			if (binding.input.axis.invert) {
				value = -value;
			}
			if (binding.input.axis.range == FULL_AXIS ||
				(binding.input.axis.range == POSITIVE_HALF_AXIS && value >= 0) ||
				(binding.input.axis.range == NEGATIVE_HALF_AXIS && value < 0)) {
				event.type = binding.outputType;
				float shifted_positive_value = 0;
				switch (binding.input.axis.range) {
				case POSITIVE_HALF_AXIS:
					shifted_positive_value = value;
					break;
				case NEGATIVE_HALF_AXIS:
					shifted_positive_value = value + 1;
					break;
				case FULL_AXIS:
					shifted_positive_value = (value + 1) / 2;
					break;
				}
				switch (binding.outputType) {
				case TYPE_BUTTON:
					event.index = (int)binding.output.button;
					switch (binding.input.axis.range) {
					case POSITIVE_HALF_AXIS:
						event.value = shifted_positive_value;
						break;
					case NEGATIVE_HALF_AXIS:
						event.value = 1 - shifted_positive_value;
						break;
					case FULL_AXIS:
						// It doesn't make sense for a full axis to map to a button,
						// but keeping as a default for a trigger with a positive half-axis.
						event.value = (shifted_positive_value * 2) - 1;
						break;
					}
					return event;
				case TYPE_AXIS:
					event.index = (int)binding.output.axis.axis;
					event.value = value;
					r_range = binding.output.axis.range;
					if (binding.output.axis.range != binding.input.axis.range) {
						switch (binding.output.axis.range) {
						case POSITIVE_HALF_AXIS:
							event.value = shifted_positive_value;
							break;
						case NEGATIVE_HALF_AXIS:
							event.value = shifted_positive_value - 1;
							break;
						case FULL_AXIS:
							event.value = (shifted_positive_value * 2) - 1;
							break;
						}
					}
					return event;
				default:
					ERR_PRINT_ONCE("Joypad axis mapping error.");
				}
			}
		}
	}
	return event;
}

void Input::_get_mapped_hat_events(
	const JoyDeviceMapping& p_mapping, HatDir p_hat, JoyEvent r_events[(size_t)HatDir::MAX])
{
	for (int i = 0; i < p_mapping.bindings.size(); i++) {
		const JoyBinding binding = p_mapping.bindings[i];
		if (binding.inputType == TYPE_HAT && binding.input.hat.hat == p_hat) {
			HatDir hat_direction;
			switch (binding.input.hat.hat_mask) {
			case HatMask::UP:
				hat_direction = HatDir::UP;
				break;
			case HatMask::RIGHT:
				hat_direction = HatDir::RIGHT;
				break;
			case HatMask::DOWN:
				hat_direction = HatDir::DOWN;
				break;
			case HatMask::LEFT:
				hat_direction = HatDir::LEFT;
				break;
			default:
				ERR_PRINT_ONCE("Joypad button mapping error.");
				continue;
			}

			r_events[(size_t)hat_direction].type = binding.outputType;
			switch (binding.outputType) {
			case TYPE_BUTTON:
				r_events[(size_t)hat_direction].index = (int)binding.output.button;
				break;
			case TYPE_AXIS:
				r_events[(size_t)hat_direction].index = (int)binding.output.axis.axis;
				switch (binding.output.axis.range) {
				case POSITIVE_HALF_AXIS:
					r_events[(size_t)hat_direction].value = 1;
					break;
				case NEGATIVE_HALF_AXIS:
					r_events[(size_t)hat_direction].value = -1;
					break;
				case FULL_AXIS:
					// It doesn't make sense for a hat direction to map to a full axis,
					// but keeping as a default for a trigger with a positive half-axis.
					r_events[(size_t)hat_direction].value = 1;
					break;
				}
				break;
			default:
				ERR_PRINT_ONCE("Joypad button mapping error.");
			}
		}
	}
}

JoyButton Input::_get_output_button(const String& p_output)
{
	for (int i = 0; i < (int)JoyButton::SDL_MAX; i++) {
		if (p_output == _joy_buttons[i]) {
			return JoyButton(i);
		}
	}
	return JoyButton::INVALID;
}

JoyAxis Input::_get_output_axis(const String& p_output)
{
	for (int i = 0; i < (int)JoyAxis::SDL_MAX; i++) {
		if (p_output == _joy_axes[i]) {
			return JoyAxis(i);
		}
	}
	return JoyAxis::INVALID;
}

void Input::parse_mapping(const String& p_mapping)
{
	if (!data) {
		return;
	}
	_THREAD_SAFE_METHOD_;
	JoyDeviceMapping mapping;

	Vector<String> entry = p_mapping.split(",");
	if (entry.size() < 2) {
		return;
	}

	mapping.uid = entry[0];
	mapping.name = entry[1];

	int idx = 1;
	while (++idx < entry.size()) {
		if (entry[idx].is_empty()) {
			continue;
		}

		String output = entry[idx].get_slicec(':', 0).remove_char(' ');
		String input = entry[idx].get_slicec(':', 1).remove_char(' ');
		if (output.length() < 1 || input.length() < 2) {
			continue;
		}

		if (output == "platform" || output == "hint") {
			continue;
		}

		JoyAxisRange output_range = FULL_AXIS;
		if (output[0] == '+' || output[0] == '-') {
			ERR_CONTINUE_MSG(output.length() < 2,
				vformat("Invalid output entry \"%s\" in mapping:\n%s", entry[idx], p_mapping));
			if (output[0] == '+') {
				output_range = POSITIVE_HALF_AXIS;
			}
			else if (output[0] == '-') {
				output_range = NEGATIVE_HALF_AXIS;
			}
			output = output.substr(1);
		}

		JoyAxisRange input_range = FULL_AXIS;
		if (input[0] == '+') {
			input_range = POSITIVE_HALF_AXIS;
			input = input.substr(1);
		}
		else if (input[0] == '-') {
			input_range = NEGATIVE_HALF_AXIS;
			input = input.substr(1);
		}
		bool invert_axis = false;
		if (input[input.length() - 1] == '~') {
			invert_axis = true;
			input = input.left(input.length() - 1);
		}

		JoyButton output_button = _get_output_button(output);
		JoyAxis output_axis = _get_output_axis(output);
		if (output_button == JoyButton::INVALID && output_axis == JoyAxis::INVALID) {
			print_verbose(
				vformat("Unrecognized output string \"%s\" in mapping:\n%s", output, p_mapping));
			continue;
		}
		ERR_CONTINUE_MSG(output_button != JoyButton::INVALID && output_axis != JoyAxis::INVALID,
			vformat("Output string \"%s\" matched both button and axis in mapping:\n%s", output,
				p_mapping));

		JoyBinding binding;
		if (output_button != JoyButton::INVALID) {
			binding.outputType = TYPE_BUTTON;
			binding.output.button = output_button;
		}
		else if (output_axis != JoyAxis::INVALID) {
			binding.outputType = TYPE_AXIS;
			binding.output.axis.axis = output_axis;
			binding.output.axis.range = output_range;
		}

		switch (input[0]) {
		case 'b':
			binding.inputType = TYPE_BUTTON;
			binding.input.button = (JoyButton)input.substr(1).to_int();
			break;
		case 'a':
			binding.inputType = TYPE_AXIS;
			binding.input.axis.axis = (JoyAxis)input.substr(1).to_int();
			binding.input.axis.range = input_range;
			binding.input.axis.invert = invert_axis;
			break;
		case 'h':
			ERR_CONTINUE_MSG(input.length() != 4 || input[2] != '.',
				vformat("Invalid had input \"%s\" in mapping:\n%s", input, p_mapping));
			binding.inputType = TYPE_HAT;
			binding.input.hat.hat = (HatDir)input.substr(1, 1).to_int();
			binding.input.hat.hat_mask = static_cast<HatMask>(input.substr(3).to_int());
			break;
		default:
			ERR_CONTINUE_MSG(true,
				vformat("Unrecognized input string \"%s\" in mapping:\n%s", input, p_mapping));
		}

		mapping.bindings.push_back(binding);
	}

	data->map_db.push_back(mapping);
}

void Input::add_joy_mapping(const String& p_mapping, bool p_update_existing)
{
	if (!data) {
		return;
	}
	parse_mapping(p_mapping);
	if (p_update_existing) {
		const String uid = p_mapping.get_slicec(',', 0);
		for (KeyValue<int, Joypad>& E : data->joy_names) {
			Joypad& joy = E.value;
			if (joy.uid == uid) {
				_set_joypad_mapping(joy, data->map_db.size() - 1);
			}
		}
	}
}

void Input::remove_joy_mapping(const String& p_guid)
{
	if (!data) {
		return;
	}
	// One GUID can exist multiple times in `map_db`, and
	// `add_joy_mapping` can choose not to update the existing mapping,
	// so the indices can be all over the place. Therefore we need to remember them.
	Vector<int> removed_idx;
	int min_removed_idx = -1;
	int max_removed_idx = -1;
	int fallback_mapping_offset = 0;

	for (int i = data->map_db.size() - 1; i >= 0; i--) {
		if (p_guid == data->map_db[i].uid) {
			data->map_db.remove_at(i);

			if (max_removed_idx == -1) {
				max_removed_idx = i;
			}
			min_removed_idx = i;
			removed_idx.push_back(i);

			if (i < data->fallback_mapping) {
				fallback_mapping_offset++;
			}
			else if (i == data->fallback_mapping) {
				data->fallback_mapping = -1;
				WARN_PRINT_ONCE(vformat("Removed fallback joypad input mapping \"%s\". This could "
										"lead to joypads not working as intended.",
					p_guid));
			}
		}
	}

	if (min_removed_idx == -1) {
		return; // Nothing removed.
	}

	if (data->fallback_mapping > 0) {
		// Fix the shifted index.
		data->fallback_mapping -= fallback_mapping_offset;
	}

	int removed_idx_size = removed_idx.size();

	// Update joypad mapping references: some
	// * should use the fallback_mapping (if set; if not, they get unmapped), or
	// * need their mapping reference fixed, because the deletion(s) offset them.
	for (KeyValue<int, Joypad>& E : data->joy_names) {
		Joypad& joy = E.value;
		if (joy.mapping < min_removed_idx) {
			continue; // Not affected.
		}

		if (joy.mapping > max_removed_idx) {
			_set_joypad_mapping(joy, joy.mapping - removed_idx_size);
			continue; // Simple offset fix.
		}

		// removed_idx is in reverse order (ie. high to low), because the first loop is in reverse
		// order.
		for (int i = 0; i < removed_idx.size(); i++) {
			if (removed_idx[i] == joy.mapping) {
				// Set to fallback_mapping, if defined, else unmap the joypad.
				// Currently, the fallback_mapping is only set internally, and only for Android.
				_set_joypad_mapping(joy, data->fallback_mapping);
				break;
			}
			if (removed_idx[i] < joy.mapping) {
				// Complex offset fix:
				// This mapping was shifted by `(removed_idx_size - i)` deletions.
				_set_joypad_mapping(joy, joy.mapping - (removed_idx_size - i));
				break;
			}
		}
	}
}

void Input::_set_joypad_mapping(Joypad& p_js, int p_map_index)
{
	if (p_map_index != data->fallback_mapping && p_map_index >= 0 && p_map_index < data->map_db.size() &&
		p_js.uid != "__XINPUT_DEVICE__") {
		// Prefer the joypad name defined in the mapping.
		// Exceptions:
		// * On Windows for XInput devices the mapping would change the joypad's name to a
		// collective name.
		// * A fallback mapping is not allowed to override the joypad's name.
		p_js.name = data->map_db[p_map_index].name;
	}
	p_js.mapping = p_map_index;
}

void Input::set_fallback_mapping(const String& p_guid)
{
	if (!data) {
		return;
	}
	for (int i = 0; i < data->map_db.size(); i++) {
		if (data->map_db[i].uid == p_guid) {
			data->fallback_mapping = i;
			return;
		}
	}
}

// platforms that use the remapping system can override and call to these ones
bool Input::is_joy_known(int p_device)
{
	if (!data) {
		return false;
	}
	return data->joy_names.has(p_device) && data->joy_names[p_device].is_known;
}

String Input::get_joy_guid(int p_device)
{
	if (!data) {
		return String();
	}
	ERR_FAIL_COND_V(!data->joy_names.has(p_device), "");
	return data->joy_names[p_device].uid;
}

bool Input::should_ignore_device(int p_vendor_id, int p_product_id)
{
	if (!data) {
		return false;
	}
	uint32_t full_id = (((uint32_t)p_vendor_id) << 16) | ((uint16_t)p_product_id);
	return data->ignored_device_ids.has(full_id);
}

int Input::get_unused_joy_id()
{
	if (!data) {
		return -1;
	}
	for (int i = 0; i < JOYPADS_MAX; i++) {
		if (!data->joy_names.has(i) || !data->joy_names[i].connected) {
			return i;
		}
	}
	return -1;
}

void Input::set_disable_input(bool p_disable)
{
	if (data) {
		data->disable_input = p_disable;
	}
}

bool Input::is_input_disabled() { return data ? data->disable_input : false; }

void Input::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);

	// Parse default mappings.
	{
		int i = 0;
		while (DefaultControllerMappings::mappings[i]) {
			parse_mapping(DefaultControllerMappings::mappings[i++]);
		}
	}

	// If defined, parse SDL_GAMECONTROLLERCONFIG for possible new mappings/overrides.
	String env_mapping = OS::get_environment("SDL_GAMECONTROLLERCONFIG");
	if (!env_mapping.is_empty()) {
		Vector<String> entries = env_mapping.split("\n");
		for (int i = 0; i < entries.size(); i++) {
			if (entries[i].is_empty()) {
				continue;
			}
			parse_mapping(entries[i]);
		}
	}

	String env_ignore_devices =
		OS::get_environment("SDL_GAMECONTROLLER_IGNORE_DEVICES");
	if (!env_ignore_devices.is_empty()) {
		Vector<String> entries = env_ignore_devices.split(",");
		for (int i = 0; i < entries.size(); i++) {
			Vector<String> vid_pid = entries[i].split("/");

			if (vid_pid.size() < 2) {
				continue;
			}

			print_verbose(
				vformat("Device Ignored -- Vendor: %s Product: %s", vid_pid[0], vid_pid[1]));
			const uint16_t vid_unswapped = vid_pid[0].hex_to_int();
			const uint16_t pid_unswapped = vid_pid[1].hex_to_int();
			const uint16_t vid = BSWAP16(vid_unswapped);
			const uint16_t pid = BSWAP16(pid_unswapped);

			uint32_t full_id = (((uint32_t)vid) << 16) | ((uint16_t)pid);
			data->ignored_device_ids.insert(full_id);
		}
	}

	if (Engine::is_editor_hint()) {
		// Always use standard behavior in the editor.
		data->legacy_just_pressed_behavior = false;
	}
}

void Input::finalize()
{
	if (data == nullptr) {
		return;
	}
	for (KeyValue<int, MotionInfo>& E : data->joy_motion) {
		delete E.value.gamepad_motion;
		E.value.gamepad_motion = nullptr;
	}
	memdelete(data);
	data = nullptr;
}
#ifdef STANDARD_GRAVITY
#undef STANDARD_GRAVITY
#endif

// ==========================================================================
// input_map.cpp
// ==========================================================================

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

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/input/input.h"

// --- input_map.compat.inc ---
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

#ifndef DISABLE_DEPRECATED


void InputMap::_add_action_bind_compat_97281(const String& p_action, float p_deadzone)
{
	add_action(p_action, p_deadzone);
}

void InputMap::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


void InputMap::initialize()
{
	if (data == nullptr) {
		data = memnew(Data);
	}
}

void InputMap::finalize()
{
	if (data != nullptr) {
		memdelete(data);
		data = nullptr;
	}
}

/**
 * Returns an nonexistent action error message with a suggestion of the closest
 * matching action name (if possible).
 */
String InputMap::suggest_actions(const String& p_action)
{
	String closest_action;
	float closest_similarity = 0.0;

	if (data != nullptr) {
		// Find the most action with the most similar name.
		for (const KeyValue<String, Action>& kv : data->input_map) {
			const float similarity = String(kv.key).similarity(p_action);

			if (similarity > closest_similarity) {
				closest_action = kv.key;
				closest_similarity = similarity;
			}
		}
	}

	String error_message = vformat("The InputMap action \"%s\" doesn't exist.", p_action);

	if (closest_similarity >= 0.4) {
		// Only include a suggestion in the error message if it's similar enough.
		error_message += vformat(" Did you mean \"%s\"?", closest_action);
	}
	return error_message;
}

#ifdef TOOLS_ENABLED
void InputMap::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options)
{
	(void)r_options;
	const String pf = p_function;
	bool first_argument_is_action = false;
	if (p_idx == 0) {
		first_argument_is_action =
			(pf == "has_action" || pf == "erase_action" || pf == "action_set_deadzone" ||
				pf == "action_get_deadzone" || pf == "action_has_event" ||
				pf == "action_add_event" || pf == "action_get_events" ||
				pf == "action_erase_event" || pf == "action_erase_events");
	}
	(void)first_argument_is_action;
}
#endif

void InputMap::add_action(const String& p_action, float p_deadzone)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND_MSG(data->input_map.has(p_action),
		vformat("InputMap already has action \"%s\".", String(p_action)));
	data->input_map[p_action] = Action();
	data->input_map[p_action].id = data->last_id;
	data->input_map[p_action].deadzone = p_deadzone;
	data->last_id++;
}

void InputMap::erase_action(const String& p_action)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND_MSG(!data->input_map.has(p_action), suggest_actions(p_action));

	data->input_map.erase(p_action);
}

List<InputEvent>::Element* InputMap::_find_event(Action& p_action, const InputEvent& p_event,
	bool p_exact_match, bool* r_pressed, float* r_strength, float* r_raw_strength,
	int* r_event_index)
{
	ERR_FAIL_COND_V(p_event.is_null(), nullptr);

	int i = 0;
	for (List<InputEvent>::Element* E = p_action.inputs.front(); E; E = E->next()) {
		int device = E->get().get_device();
		if (device == ALL_DEVICES || device == p_event.get_device()) {
			if (E->get().action_match(
					p_event, p_exact_match, p_action.deadzone, r_pressed, r_strength,
					r_raw_strength)) {
				if (r_event_index) {
					*r_event_index = i;
				}
				return E;
			}
		}
		i++;
	}

	return nullptr;
}

bool InputMap::has_action(const String& p_action)
{
	return data != nullptr && data->input_map.has(p_action);
}

String InputMap::get_action_description(const String& p_action)
{
	(void)p_action;
	return String();
}

float InputMap::action_get_deadzone(const String& p_action)
{
	ERR_FAIL_NULL_V(data, 0.0f);
	ERR_FAIL_COND_V_MSG(!data->input_map.has(p_action), 0.0f, suggest_actions(p_action));

	return data->input_map[p_action].deadzone;
}

void InputMap::action_set_deadzone(const String& p_action, float p_deadzone)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND_MSG(!data->input_map.has(p_action), suggest_actions(p_action));

	data->input_map[p_action].deadzone = p_deadzone;
}

void InputMap::action_add_event(const String& p_action, const InputEvent& p_event)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(p_event.is_null());
	ERR_FAIL_COND_MSG(!data->input_map.has(p_action), suggest_actions(p_action));
	if (_find_event(data->input_map[p_action], p_event, true)) {
		return; // Already added.
	}

	InputEvent ev = p_event;
	// Normalize legacy device IDs: before the device ID change,
	// keyboard and mouse events defaulted to device=0.
	if (ev.device == 0) {
		switch (ev.type) {
		case InputEventType::KEY:
			ev.device = InputEvent::DEVICE_ID_KEYBOARD;
			break;
		case InputEventType::MOUSE_BUTTON:
		case InputEventType::MOUSE_MOTION:
			ev.device = InputEvent::DEVICE_ID_MOUSE;
			break;
		default:
			break;
		}
	}

	data->input_map[p_action].inputs.push_back(ev);
}

bool InputMap::action_has_event(const String& p_action, const InputEvent& p_event)
{
	ERR_FAIL_NULL_V(data, false);
	ERR_FAIL_COND_V_MSG(!data->input_map.has(p_action), false, suggest_actions(p_action));
	return (_find_event(data->input_map[p_action], p_event, true) != nullptr);
}

void InputMap::action_erase_event(const String& p_action, const InputEvent& p_event)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND_MSG(!data->input_map.has(p_action), suggest_actions(p_action));

	List<InputEvent>::Element* E = _find_event(data->input_map[p_action], p_event, true);
	if (E) {
		data->input_map[p_action].inputs.erase(E);

		if (Input::is_action_pressed(p_action)) {
			Input::action_release(p_action);
		}
	}
}

void InputMap::action_erase_events(const String& p_action)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND_MSG(!data->input_map.has(p_action), suggest_actions(p_action));

	if (Input::is_action_pressed(p_action)) {
		Input::action_release(p_action);
	}

	data->input_map[p_action].inputs.clear();
}

const List<InputEvent>* InputMap::action_get_events(const String& p_action)
{
	ERR_FAIL_NULL_V(data, nullptr);
	HashMap<String, Action>::Iterator E = data->input_map.find(p_action);
	if (!E) {
		return nullptr;
	}

	return &E->value.inputs;
}

bool InputMap::event_is_action(
	const InputEvent& p_event, const String& p_action, bool p_exact_match)
{
	return event_get_action_status(p_event, p_action, p_exact_match);
}

int InputMap::event_get_index(
	const InputEvent& p_event, const String& p_action, bool p_exact_match)
{
	int index = -1;
	bool valid = event_get_action_status(
		p_event, p_action, p_exact_match, nullptr, nullptr, nullptr, &index);
	return valid ? index : -1;
}

bool InputMap::event_get_action_status(const InputEvent& p_event, const String& p_action,
	bool p_exact_match, bool* r_pressed, float* r_strength, float* r_raw_strength,
	int* r_event_index)
{
	ERR_FAIL_NULL_V(data, false);
	if (p_event.is_null()) {
		return false;
	}
	HashMap<String, Action>::Iterator E = data->input_map.find(p_action);
	ERR_FAIL_COND_V_MSG(!E, false, suggest_actions(p_action));

	if (p_event.type == InputEventType::ACTION) {
		if (p_event.action.action == p_action) {
			if (r_pressed) {
				*r_pressed = p_event.is_pressed();
			}
			if (r_strength) {
				*r_strength = p_event.action.strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = p_event.action.strength;
			}
			if (r_event_index) {
				*r_event_index = p_event.action.event_index;
			}
			return true;
		}
	}

	const Action& action = E->value;
	int i = 0;
	for (const InputEvent& e : action.inputs) {
		if (e.is_null()) {
			i++;
			continue;
		}

		bool pressed = false;
		float strength = 0.0f;
		float raw_strength = 0.0f;

		if (e.action_match(
				p_event, p_exact_match, action.deadzone, &pressed, &strength, &raw_strength)) {
			if (r_pressed) {
				*r_pressed = pressed;
			}
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = raw_strength;
			}
			if (r_event_index) {
				*r_event_index = i;
			}
			return true;
		}
		i++;
	}

	return false;
}

const HashMap<String, InputMap::Action>& InputMap::get_action_map()
{
	static const HashMap<String, Action> _empty_action_map;
	ERR_FAIL_NULL_V(data, _empty_action_map);
	return data->input_map;
}

struct _BuiltinActionDisplayName
{
	const char* name;
	const char* display_name;
};

static const _BuiltinActionDisplayName _builtin_action_display_names[] = {
	/* clang-format off */
	{ "ui_accept",                                     TTRC("Accept") },
	{ "ui_select",                                     TTRC("Select") },
	{ "ui_cancel",                                     TTRC("Cancel") },
	{ "ui_close_dialog",                               TTRC("Close Dialog") },
	{ "ui_focus_next",                                 TTRC("Focus Next") },
	{ "ui_focus_prev",                                 TTRC("Focus Prev") },
	{ "ui_left",                                       TTRC("Left") },
	{ "ui_right",                                      TTRC("Right") },
	{ "ui_up",                                         TTRC("Up") },
	{ "ui_down",                                       TTRC("Down") },
	{ "ui_page_up",                                    TTRC("Page Up") },
	{ "ui_page_down",                                  TTRC("Page Down") },
	{ "ui_home",                                       TTRC("Home") },
	{ "ui_end",                                        TTRC("End") },
	{ "ui_cut",                                        TTRC("Cut") },
	{ "ui_copy",                                       TTRC("Copy") },
	{ "ui_paste",                                      TTRC("Paste") },
	{ "ui_focus_mode",                                 TTRC("Toggle Tab Focus Mode") },
	{ "ui_undo",                                       TTRC("Undo") },
	{ "ui_redo",                                       TTRC("Redo") },
	{ "ui_text_completion_query",                      TTRC("Completion Query") },
	{ "ui_text_newline",                               TTRC("New Line") },
	{ "ui_text_newline_blank",                         TTRC("New Blank Line") },
	{ "ui_text_newline_above",                         TTRC("New Line Above") },
	{ "ui_text_indent",                                TTRC("Indent") },
	{ "ui_text_dedent",                                TTRC("Dedent") },
	{ "ui_text_backspace",                             TTRC("Backspace") },
	{ "ui_text_backspace_word",                        TTRC("Backspace Word") },
	{ "ui_text_backspace_all_to_left",                 TTRC("Backspace all to Left") },
	{ "ui_text_delete",                                TTRC("Delete") },
	{ "ui_text_delete_word",                           TTRC("Delete Word") },
	{ "ui_text_delete_all_to_right",                   TTRC("Delete all to Right") },
	{ "ui_text_caret_left",                            TTRC("Caret Left") },
	{ "ui_text_caret_word_left",                       TTRC("Caret Word Left") },
	{ "ui_text_caret_right",                           TTRC("Caret Right") },
	{ "ui_text_caret_word_right",                      TTRC("Caret Word Right") },
	{ "ui_text_caret_up",                              TTRC("Caret Up") },
	{ "ui_text_caret_down",                            TTRC("Caret Down") },
	{ "ui_text_caret_line_start",                      TTRC("Caret Line Start") },
	{ "ui_text_caret_line_end",                        TTRC("Caret Line End") },
	{ "ui_text_caret_page_up",                         TTRC("Caret Page Up") },
	{ "ui_text_caret_page_down",                       TTRC("Caret Page Down") },
	{ "ui_text_caret_document_start",                  TTRC("Caret Document Start") },
	{ "ui_text_caret_document_end",                    TTRC("Caret Document End") },
	{ "ui_text_caret_add_below",                       TTRC("Caret Add Below") },
	{ "ui_text_caret_add_above",                       TTRC("Caret Add Above") },
	{ "ui_text_scroll_up",                             TTRC("Scroll Up") },
	{ "ui_text_scroll_down",                           TTRC("Scroll Down") },
	{ "ui_text_select_all",                            TTRC("Select All") },
	{ "ui_text_select_word_under_caret",               TTRC("Select Word Under Caret") },
	{ "ui_text_add_selection_for_next_occurrence",     TTRC("Add Selection for Next Occurrence") },
	{ "ui_text_skip_selection_for_next_occurrence",    TTRC("Skip Selection for Next Occurrence") },
	{ "ui_text_clear_carets_and_selection",            TTRC("Clear Carets and Selection") },
	{ "ui_text_toggle_insert_mode",                    TTRC("Toggle Insert Mode") },
	{ "ui_text_submit",                                TTRC("Submit Text") },
	{ "ui_graph_duplicate",                            TTRC("Duplicate Nodes") },
	{ "ui_graph_delete",                               TTRC("Delete Nodes") },
	{ "ui_graph_follow_left",                          TTRC("Follow Input Port Connection") },
	{ "ui_graph_follow_right",                         TTRC("Follow Output Port Connection") },
	{ "ui_filedialog_delete",                          TTRC("Delete") },
	{ "ui_filedialog_up_one_level",                    TTRC("Go Up One Level") },
	{ "ui_filedialog_refresh",                         TTRC("Refresh") },
	{ "ui_filedialog_show_hidden",                     TTRC("Show Hidden") },
	{ "ui_filedialog_find",                            TTRC("Find") },
	{ "ui_filedialog_focus_path",                      TTRC("Focus Path") },
	{ "ui_swap_input_direction",                       TTRC("Swap Input Direction") },
	{ "ui_unicode_start",                              TTRC("Start Unicode Character Input") },
	{ "ui_colorpicker_delete_preset",                  TTRC("ColorPicker: Delete Preset") },
	{ "ui_accessibility_drag_and_drop",                TTRC("Accessibility: Keyboard Drag and Drop") },
	{ "ui_toggle_fullscreen",                          TTRC("Toggle Fullscreen") },
	{ "",                                              ""}
	/* clang-format on */
};

String InputMap::get_builtin_display_name(const String& p_name)
{
	const String name = p_name.get_slicec('.', 0);
	constexpr int len = std_size(_builtin_action_display_names);
	for (int i = 0; i < len; i++) {
		if (_builtin_action_display_names[i].name == name) {
			return _builtin_action_display_names[i].display_name;
		}
	}

	return p_name;
}

const HashMap<String, List<InputEvent>>& InputMap::get_builtins()
{
	static const HashMap<String, List<InputEvent>> _empty_builtin_map;
	ERR_FAIL_NULL_V(data, _empty_builtin_map);

	HashMap<String, List<InputEvent>>& default_builtin_cache = data->default_builtin_cache;

	// Return cache if it has already been built.
	if (default_builtin_cache.size()) {
		return default_builtin_cache;
	}

	List<InputEvent> inputs;
	inputs.push_back(InputEvent::make_key(Key::ENTER));
	inputs.push_back(InputEvent::make_key(Key::KP_ENTER));
	inputs.push_back(InputEvent::make_key(Key::SPACE));
	default_builtin_cache.insert("ui_accept", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_joypad_button(JoyButton::Y));
	inputs.push_back(InputEvent::make_key(Key::SPACE));
	default_builtin_cache.insert("ui_select", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ESCAPE));
	default_builtin_cache.insert("ui_cancel", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ESCAPE));
	default_builtin_cache.insert("ui_close_dialog", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::W | KeyModifierMask::META));
	inputs.push_back(InputEvent::make_key(Key::ESCAPE));
	default_builtin_cache.insert("ui_close_dialog.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::TAB));
	default_builtin_cache.insert("ui_focus_next", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::TAB | KeyModifierMask::SHIFT));
	default_builtin_cache.insert("ui_focus_prev", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::LEFT));
	inputs.push_back(InputEvent::make_joypad_button(JoyButton::DPAD_LEFT));
	inputs.push_back(InputEvent::make_joypad_motion(JoyAxis::LEFT_X, -1.0));
	default_builtin_cache.insert("ui_left", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::RIGHT));
	inputs.push_back(InputEvent::make_joypad_button(JoyButton::DPAD_RIGHT));
	inputs.push_back(InputEvent::make_joypad_motion(JoyAxis::LEFT_X, 1.0));
	default_builtin_cache.insert("ui_right", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::UP));
	inputs.push_back(InputEvent::make_joypad_button(JoyButton::DPAD_UP));
	inputs.push_back(InputEvent::make_joypad_motion(JoyAxis::LEFT_Y, -1.0));
	default_builtin_cache.insert("ui_up", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::DOWN));
	inputs.push_back(InputEvent::make_joypad_button(JoyButton::DPAD_DOWN));
	inputs.push_back(InputEvent::make_joypad_motion(JoyAxis::LEFT_Y, 1.0));
	default_builtin_cache.insert("ui_down", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::PAGEUP));
	default_builtin_cache.insert("ui_page_up", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::PAGEDOWN));
	default_builtin_cache.insert("ui_page_down", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::HOME));
	default_builtin_cache.insert("ui_home", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::END));
	default_builtin_cache.insert("ui_end", inputs);

	inputs = List<InputEvent>();
	default_builtin_cache.insert("ui_accessibility_drag_and_drop", inputs);

	// ///// UI basic Shortcuts /////

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::X | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::KEY_DELETE | KeyModifierMask::SHIFT));
	default_builtin_cache.insert("ui_cut", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::C | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::INSERT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_copy", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::M | KeyModifierMask::CTRL));
	default_builtin_cache.insert("ui_focus_mode", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::V | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::INSERT | KeyModifierMask::SHIFT));
	default_builtin_cache.insert("ui_paste", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::Z | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_undo", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::Z | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT));
	inputs.push_back(InputEvent::make_key(Key::Y | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_redo", inputs);

	// ///// UI Text Input Shortcuts /////
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::SPACE | KeyModifierMask::CTRL));
	default_builtin_cache.insert("ui_text_completion_query", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::TAB));
	inputs.push_back(InputEvent::make_key(Key::ENTER));
	inputs.push_back(InputEvent::make_key(Key::KP_ENTER));
	default_builtin_cache.insert("ui_text_completion_accept", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(KeyModifierMask::SHIFT | Key::TAB));
	inputs.push_back(InputEvent::make_key(KeyModifierMask::SHIFT | Key::ENTER));
	inputs.push_back(InputEvent::make_key(KeyModifierMask::SHIFT | Key::KP_ENTER));
	default_builtin_cache.insert("ui_text_completion_replace", inputs);

	// Newlines
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ENTER));
	inputs.push_back(InputEvent::make_key(Key::KP_ENTER));
	inputs.push_back(InputEvent::make_key(KeyModifierMask::SHIFT | Key::ENTER));
	inputs.push_back(InputEvent::make_key(KeyModifierMask::SHIFT | Key::KP_ENTER));
	default_builtin_cache.insert("ui_text_newline", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ENTER | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::KP_ENTER | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_newline_blank", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::ENTER | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(
		Key::KP_ENTER | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_newline_above", inputs);

	// Indentation
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::TAB));
	default_builtin_cache.insert("ui_text_indent", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::TAB | KeyModifierMask::SHIFT));
	default_builtin_cache.insert("ui_text_dedent", inputs);

	// Text Backspace and Delete
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::BACKSPACE));
	inputs.push_back(InputEvent::make_key(Key::BACKSPACE | KeyModifierMask::SHIFT));
	default_builtin_cache.insert("ui_text_backspace", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::BACKSPACE | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_backspace_word", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::BACKSPACE | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_backspace_word.macos", inputs);

	inputs = List<InputEvent>();
	default_builtin_cache.insert("ui_text_backspace_all_to_left", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::BACKSPACE | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_backspace_all_to_left.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::KEY_DELETE));
	default_builtin_cache.insert("ui_text_delete", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::KEY_DELETE | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_delete_word", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::KEY_DELETE | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_delete_word.macos", inputs);

	inputs = List<InputEvent>();
	default_builtin_cache.insert("ui_text_delete_all_to_right", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::KEY_DELETE | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_delete_all_to_right.macos", inputs);

	// Text Caret Movement Left/Right

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::LEFT));
	default_builtin_cache.insert("ui_text_caret_left", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::LEFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_word_left", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::LEFT | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_caret_word_left.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::RIGHT));
	default_builtin_cache.insert("ui_text_caret_right", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::RIGHT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_word_right", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::RIGHT | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_caret_word_right.macos", inputs);

	// Text Caret Movement Up/Down

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::UP));
	default_builtin_cache.insert("ui_text_caret_up", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::DOWN));
	default_builtin_cache.insert("ui_text_caret_down", inputs);

	// Text Caret Movement Line Start/End

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::HOME));
	default_builtin_cache.insert("ui_text_caret_line_start", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::A | KeyModifierMask::CTRL));
	inputs.push_back(InputEvent::make_key(Key::LEFT | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::HOME));
	default_builtin_cache.insert("ui_text_caret_line_start.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::END));
	default_builtin_cache.insert("ui_text_caret_line_end", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::E | KeyModifierMask::CTRL));
	inputs.push_back(InputEvent::make_key(Key::RIGHT | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::END));
	default_builtin_cache.insert("ui_text_caret_line_end.macos", inputs);

	// Text Caret Movement Page Up/Down

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::PAGEUP));
	default_builtin_cache.insert("ui_text_caret_page_up", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::PAGEDOWN));
	default_builtin_cache.insert("ui_text_caret_page_down", inputs);

	// Text Caret Movement Document Start/End

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::HOME | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_document_start", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::UP | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::HOME | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_document_start.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::END | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_document_end", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::DOWN | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::END | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_document_end.macos", inputs);

	// Text Caret Addition Below/Above

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::DOWN | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_add_below", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::L | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_add_below.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::UP | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_add_above", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::O | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_caret_add_above.macos", inputs);

	// Text Scrolling

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::UP | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_scroll_up", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::UP | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_scroll_up.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::DOWN | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_scroll_down", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::DOWN | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_scroll_down.macos", inputs);

	// Text Misc

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::A | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_select_all", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::G | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_select_word_under_caret", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::G | KeyModifierMask::CTRL | KeyModifierMask::META));
	default_builtin_cache.insert("ui_text_select_word_under_caret.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::D | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_text_add_selection_for_next_occurrence", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(
		Key::D | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_text_skip_selection_for_next_occurrence", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ESCAPE));
	default_builtin_cache.insert("ui_text_clear_carets_and_selection", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::INSERT));
	default_builtin_cache.insert("ui_text_toggle_insert_mode", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::MENU));
	default_builtin_cache.insert("ui_menu", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ENTER));
	inputs.push_back(InputEvent::make_key(Key::KP_ENTER));
	default_builtin_cache.insert("ui_text_submit", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::U | KeyModifierMask::CTRL | KeyModifierMask::SHIFT));
	default_builtin_cache.insert("ui_unicode_start", inputs);

	// ///// UI Graph Shortcuts /////

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::D | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_graph_duplicate", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::KEY_DELETE));
	default_builtin_cache.insert("ui_graph_delete", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::LEFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_graph_follow_left", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::LEFT | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_graph_follow_left.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::RIGHT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_graph_follow_right", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::RIGHT | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_graph_follow_right.macos", inputs);

	// ///// UI File Dialog Shortcuts /////
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::KEY_DELETE));
	default_builtin_cache.insert("ui_filedialog_delete", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::BACKSPACE));
	default_builtin_cache.insert("ui_filedialog_up_one_level", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::F5));
	default_builtin_cache.insert("ui_filedialog_refresh", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::H));
	default_builtin_cache.insert("ui_filedialog_show_hidden", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::F | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_filedialog_find", inputs);

	inputs = List<InputEvent>();
	// Ctrl + L (matches most Windows/Linux file managers' "focus on path bar" shortcut,
	// plus macOS Safari's "focus on address bar" shortcut).
	inputs.push_back(InputEvent::make_key(Key::L | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_filedialog_focus_path", inputs);

	inputs = List<InputEvent>();
	// Cmd + Shift + G (matches Finder's "Go To" shortcut).
	inputs.push_back(InputEvent::make_key(Key::G | KeyModifierMask::CMD_OR_CTRL));
	inputs.push_back(InputEvent::make_key(Key::L | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_filedialog_focus_path.macos", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::QUOTELEFT | KeyModifierMask::CMD_OR_CTRL));
	default_builtin_cache.insert("ui_swap_input_direction", inputs);

	// ///// UI ColorPicker Shortcuts /////
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_joypad_button(JoyButton::X));
	inputs.push_back(InputEvent::make_key(Key::KEY_DELETE));
	default_builtin_cache.insert("ui_colorpicker_delete_preset", inputs);

	// ///// Miscellaneous Shortcuts /////
	inputs = List<InputEvent>();
	inputs.push_back(InputEvent::make_key(Key::ENTER | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_toggle_fullscreen", inputs);

	inputs = List<InputEvent>();
	inputs.push_back(
		InputEvent::make_key(Key::F | KeyModifierMask::CTRL | KeyModifierMask::META));
	inputs.push_back(InputEvent::make_key(Key::ENTER | KeyModifierMask::ALT));
	default_builtin_cache.insert("ui_toggle_fullscreen.macos", inputs);

	return default_builtin_cache;
}

const HashMap<String, List<InputEvent>>&
InputMap::get_builtins_with_feature_overrides_applied()
{
	static const HashMap<String, List<InputEvent>> _empty_builtin_map;
	ERR_FAIL_NULL_V(data, _empty_builtin_map);

	HashMap<String, List<InputEvent>>& default_builtin_with_overrides_cache =
		data->default_builtin_with_overrides_cache;

	if (default_builtin_with_overrides_cache.size() > 0) {
		return default_builtin_with_overrides_cache;
	}

	const HashMap<String, List<InputEvent>>& builtins = get_builtins();

	// Get a list of all built in inputs which are valid overrides for the OS
	// Key = builtin name (e.g. ui_accept)
	// Value = override/feature names (e.g. macos, if it was defined as "ui_accept.macos" and the
	// platform supports that feature)
	HashMap<String, Vector<String>> builtins_with_overrides;
	for (const KeyValue<String, List<InputEvent>>& E : builtins) {
		String fullname = E.key;

		const String& name = fullname.get_slicec('.', 0);
		String override_for =
			fullname.get_slice_count(".") > 1 ? fullname.get_slicec('.', 1) : String();

		if (!override_for.is_empty() && OS::has_feature(override_for)) {
			builtins_with_overrides[name].push_back(override_for);
		}
	}

	for (const KeyValue<String, List<InputEvent>>& E : builtins) {
		String fullname = E.key;

		const String& name = fullname.get_slicec('.', 0);
		String override_for =
			fullname.get_slice_count(".") > 1 ? fullname.get_slicec('.', 1) : String();

		if (builtins_with_overrides.has(name) && override_for.is_empty()) {
			// Builtin has an override but this particular one is not an override, so skip.
			continue;
		}

		if (!override_for.is_empty() && !OS::has_feature(override_for)) {
			// OS does not support this override - skip.
			continue;
		}

		default_builtin_with_overrides_cache.insert(name, E.value);
	}

	return default_builtin_with_overrides_cache;
}

void InputMap::load_default()
{
	HashMap<String, List<InputEvent>> builtins(get_builtins_with_feature_overrides_applied());

	for (const KeyValue<String, List<InputEvent>>& E : builtins) {
		String name = E.key;
		add_action(name);
		for (const InputEvent& event : E.value) {
			action_add_event(name, event);
		}
	}
}

void InputMap::load_from_project_settings() {}

// ==========================================================================
// shortcut.cpp
// ==========================================================================

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

#include "core/input/input_event.h"

void Shortcut::set_events(const Vector<InputEvent>& p_events)
{
	events = p_events;
	emit_changed();
}

const Vector<InputEvent>& Shortcut::get_events() const { return events; }

void Shortcut::set_events_list(const List<InputEvent>* p_events)
{
	events.clear();
	if (!p_events) {
		return;
	}
	for (const InputEvent& ie : *p_events) {
		events.push_back(ie);
	}
}

bool Shortcut::matches_event(const InputEvent& p_event) const
{
	if (p_event.type == InputEventType::SHORTCUT && p_event.shortcut.shortcut == this) {
		return true;
	}

	for (int i = 0; i < events.size(); i++) {
		const InputEvent& ie = events[i];
		if (ie.is_valid() && ie.is_match(p_event)) {
			return true;
		}
	}

	return false;
}

String Shortcut::get_as_text() const
{
	for (int i = 0; i < events.size(); i++) {
		const InputEvent& ie = events[i];
		if (ie.is_valid()) {
			return ie.as_text();
		}
	}

	return "None";
}

Ref<Shortcut> Shortcut::make_from_action(const String& p_action)
{
	InputEvent event = InputEvent::make_action(p_action);

	Ref<Shortcut> shortcut;
	shortcut.instantiate();
	shortcut->set_events({event});
	return shortcut;
}

bool Shortcut::has_valid_event() const
{
	for (int i = 0; i < events.size(); i++) {
		if (events[i].is_valid()) {
			return true;
		}
	}

	return false;
}

bool Shortcut::is_event_array_equal(
	const Vector<InputEvent>& p_event_array1, const Vector<InputEvent>& p_event_array2)
{
	if (p_event_array1.size() != p_event_array2.size()) {
		return false;
	}
	for (int i = 0; i < p_event_array1.size(); i++) {
		if (!p_event_array1[i].is_match(p_event_array2[i], true)) {
			return false;
		}
	}
	return true;
}

// ==========================================================================
// input_event_codec.cpp
// ==========================================================================

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

#include "core/input/input.h"

#include "core/io/resource_loader.h"

enum class BoolShift : uint8_t
{
	SHIFT = 0,
	CTRL,
	ALT,
	META,
	ECHO,
	PRESSED,
	DOUBLE_CLICK,
	PEN_INVERTED,
};

inline uint8_t operator<<(uint8_t p_left, BoolShift p_right)
{
	return p_left << static_cast<uint8_t>(p_right);
}

static uint8_t encode_key_modifier_state(const InputEvent& p_event)
{
	uint8_t bools = 0;
	bools |= (uint8_t)p_event.is_shift_pressed() << BoolShift::SHIFT;
	bools |= (uint8_t)p_event.is_ctrl_pressed() << BoolShift::CTRL;
	bools |= (uint8_t)p_event.is_alt_pressed() << BoolShift::ALT;
	bools |= (uint8_t)p_event.is_meta_pressed() << BoolShift::META;
	return bools;
}

static void decode_key_modifier_state(uint8_t p_bools, InputEvent& r_event)
{
	r_event.set_shift_pressed(p_bools & (1 << BoolShift::SHIFT));
	r_event.set_ctrl_pressed(p_bools & (1 << BoolShift::CTRL));
	r_event.set_alt_pressed(p_bools & (1 << BoolShift::ALT));
	r_event.set_meta_pressed(p_bools & (1 << BoolShift::META));
}

static int encode_vector2(const Vector2& p_vector, uint8_t* p_data)
{
	p_data += encode_float(p_vector.x, p_data);
	encode_float(p_vector.y, p_data);
	return sizeof(float) * 2;
}

static const uint8_t* decode_vector2(Vector2& r_vector, const uint8_t* p_data)
{
	r_vector.x = decode_float(p_data);
	p_data += sizeof(float);
	r_vector.y = decode_float(p_data);
	p_data += sizeof(float);
	return p_data;
}

void encode_input_event_key(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(19);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::KEY;
	data++;
	uint8_t bools = encode_key_modifier_state(p_event);
	bools |= (uint8_t)p_event.key.echo << BoolShift::ECHO;
	bools |= (uint8_t)p_event.is_pressed() << BoolShift::PRESSED;
	*data = bools;
	data++;
	data += encode_uint32((uint32_t)p_event.key.keycode, data);
	data += encode_uint32((uint32_t)p_event.key.physical_keycode, data);
	data += encode_uint32((uint32_t)p_event.key.key_label, data);
	data += encode_uint32(p_event.key.unicode, data);
	*data = (uint8_t)p_event.key.location;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static Error decode_input_event_key(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::KEY);
	data++;

	r_event = InputEvent::make_key();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);
	r_event.key.echo = (bools & (1 << BoolShift::ECHO)) != 0;
	r_event.pressed = (bools & (1 << BoolShift::PRESSED)) != 0;

	r_event.key.keycode = (Key)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.physical_keycode = (Key)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.key_label = (Key)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.unicode = (char32_t)decode_uint32(data);
	data += sizeof(uint32_t);
	r_event.key.location = (KeyLocation)*data;

	return OK;
}

void encode_input_event_mouse_button(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(12);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::MOUSE_BUTTON;
	data++;

	uint8_t bools = encode_key_modifier_state(p_event);
	bools |= (uint8_t)p_event.is_pressed() << BoolShift::PRESSED;
	bools |= (uint8_t)p_event.mouse_button.double_click << BoolShift::DOUBLE_CLICK;
	*data = bools;
	data++;

	*data = (uint8_t)p_event.mouse_button.button_index;
	data++;

	data += encode_vector2(p_event.mouse_button.position, data);
	*data = (uint8_t)p_event.mouse_button.button_mask;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static Error decode_input_event_mouse_button(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::MOUSE_BUTTON);
	data++;

	r_event = InputEvent::make_mouse_button();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);
	r_event.pressed = (bools & (1 << BoolShift::PRESSED)) != 0;
	r_event.mouse_button.double_click = (bools & (1 << BoolShift::DOUBLE_CLICK)) != 0;

	r_event.mouse_button.button_index = (MouseButton)*data;
	data++;

	Vector2 pos;
	data = decode_vector2(pos, data);
	r_event.mouse_button.position = pos;
	r_event.mouse_button.global_position = pos;
	r_event.mouse_button.button_mask = (uint32_t)*data;

	return OK;
}

void encode_input_event_mouse_motion(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(31);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::MOUSE_MOTION;
	data++;

	uint8_t bools = encode_key_modifier_state(p_event);
	bools |= (uint8_t)p_event.mouse_motion.pen_inverted << BoolShift::PEN_INVERTED;
	*data = bools;
	data++;

	data += encode_vector2(p_event.mouse_motion.position, data);
	data += encode_float(p_event.mouse_motion.pressure, data);
	data += encode_vector2(p_event.mouse_motion.tilt, data);
	data += encode_vector2(p_event.mouse_motion.relative, data);
	*data = (uint8_t)p_event.mouse_motion.button_mask;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_mouse_motion(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::MOUSE_MOTION);
	data++;

	r_event = InputEvent::make_mouse_motion();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);
	r_event.mouse_motion.pen_inverted = (bools & (1 << BoolShift::PEN_INVERTED)) != 0;

	{
		Vector2 pos;
		data = decode_vector2(pos, data);
		r_event.mouse_motion.position = pos;
		r_event.mouse_motion.global_position = pos;
	}
	r_event.mouse_motion.pressure = decode_float(data);
	data += sizeof(float);
	data = decode_vector2(r_event.mouse_motion.tilt, data);
	r_event.mouse_motion.velocity = Input::get_last_mouse_velocity();
	r_event.mouse_motion.screen_velocity = Input::get_last_mouse_screen_velocity();
	{
		Vector2 relative;
		data = decode_vector2(relative, data);
		r_event.mouse_motion.relative = relative;
		r_event.mouse_motion.screen_relative = relative;
	}
	r_event.mouse_motion.button_mask = (uint32_t)*data;
	data++;

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_joypad_button(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(11);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::JOY_BUTTON;
	data++;

	uint8_t bools = 0;
	bools |= (uint8_t)p_event.is_pressed() << BoolShift::PRESSED;
	*data = bools;
	data++;

	data += encode_uint64(p_event.device, data);
	*data = (uint8_t)p_event.joy_button.button_index;
	data++;

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_joypad_button(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::JOY_BUTTON);
	data++;

	r_event = InputEvent::make_joypad_button();
	uint8_t bools = *data;
	data++;
	r_event.pressed = (bools & (1 << BoolShift::PRESSED)) != 0;
	r_event.device = decode_uint64(data);
	data += sizeof(uint64_t);
	r_event.joy_button.button_index = (JoyButton)*data;
	data++;

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_joypad_motion(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(14);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::JOY_MOTION;
	data++;

	data += encode_uint64(p_event.device, data);
	*data = (uint8_t)p_event.joy_motion.axis;
	data++;
	data += encode_float(p_event.joy_motion.axis_value, data);

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_joypad_motion(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::JOY_MOTION);
	data++;

	int dev = decode_uint64(data);
	data += sizeof(uint64_t);
	JoyAxis axis = (JoyAxis)*data;
	data++;
	float val = decode_float(data);
	data += sizeof(float);

	r_event = InputEvent::make_joypad_motion(axis, val, dev);

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_gesture_pan(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(18);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::PAN_GESTURE;
	data++;

	*data = encode_key_modifier_state(p_event);
	data++;
	data += encode_vector2(p_event.gesture.position, data);
	data += encode_vector2(p_event.gesture.delta, data);

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_gesture_pan(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::PAN_GESTURE);
	data++;

	r_event = InputEvent::make_pan_gesture();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);

	data = decode_vector2(r_event.gesture.position, data);
	data = decode_vector2(r_event.gesture.delta, data);

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

void encode_input_event_gesture_magnify(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	r_data.resize(14);

	uint8_t* data = r_data.ptrw();
	*data = (uint8_t)InputEventType::MAGNIFY_GESTURE;
	data++;

	*data = encode_key_modifier_state(p_event);
	data++;
	data += encode_vector2(p_event.gesture.position, data);
	data += encode_float(p_event.gesture.factor, data);

	DEV_ASSERT(r_data.size() >= (data - r_data.ptrw()));
}

static void decode_input_event_gesture_magnify(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();
	DEV_ASSERT(static_cast<InputEventType>(*data) == InputEventType::MAGNIFY_GESTURE);
	data++;

	r_event = InputEvent::make_magnify_gesture();
	uint8_t bools = *data;
	data++;
	decode_key_modifier_state(bools, r_event);

	data = decode_vector2(r_event.gesture.position, data);
	r_event.gesture.factor = decode_float(data);
	data += sizeof(float);

	DEV_ASSERT(p_data.size() >= (data - p_data.ptr()));
}

bool encode_input_event(const InputEvent& p_event, Vector<uint8_t>& r_data)
{
	switch (p_event.type) {
	case InputEventType::KEY:
		encode_input_event_key(p_event, r_data);
		break;
	case InputEventType::MOUSE_BUTTON:
		encode_input_event_mouse_button(p_event, r_data);
		break;
	case InputEventType::MOUSE_MOTION:
		encode_input_event_mouse_motion(p_event, r_data);
		break;
	case InputEventType::JOY_MOTION:
		encode_input_event_joypad_motion(p_event, r_data);
		break;
	case InputEventType::JOY_BUTTON:
		encode_input_event_joypad_button(p_event, r_data);
		break;
	case InputEventType::MAGNIFY_GESTURE:
		encode_input_event_gesture_magnify(p_event, r_data);
		break;
	case InputEventType::PAN_GESTURE:
		encode_input_event_gesture_pan(p_event, r_data);
		break;
	default:
		return false;
	}
	return true;
}

void decode_input_event(const Vector<uint8_t>& p_data, InputEvent& r_event)
{
	const uint8_t* data = p_data.ptr();

	switch (static_cast<InputEventType>(*data)) {
	case InputEventType::KEY:
		decode_input_event_key(p_data, r_event);
		break;
	case InputEventType::MOUSE_BUTTON:
		decode_input_event_mouse_button(p_data, r_event);
		break;
	case InputEventType::MOUSE_MOTION:
		decode_input_event_mouse_motion(p_data, r_event);
		break;
	case InputEventType::JOY_BUTTON:
		decode_input_event_joypad_button(p_data, r_event);
		break;
	case InputEventType::JOY_MOTION:
		decode_input_event_joypad_motion(p_data, r_event);
		break;
	case InputEventType::PAN_GESTURE:
		decode_input_event_gesture_pan(p_data, r_event);
		break;
	case InputEventType::MAGNIFY_GESTURE:
		decode_input_event_gesture_magnify(p_data, r_event);
		break;
	default:
		r_event = InputEvent();
		WARN_PRINT(vformat("Unknown event type %d.", static_cast<int>(*data)));
		break;
	}
}

// ============================================================================
// Merged from core/input/input_event.cpp
// ============================================================================

#include "core/input/input_event.h"

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/types.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/ustring.h"

static const char* _mouse_button_descriptions[9] = {
	TTRC("Left Mouse Button"),
	TTRC("Right Mouse Button"),
	TTRC("Middle Mouse Button"),
	TTRC("Mouse Wheel Up"),
	TTRC("Mouse Wheel Down"),
	TTRC("Mouse Wheel Left"),
	TTRC("Mouse Wheel Right"),
	TTRC("Mouse Thumb Button 1"),
	TTRC("Mouse Thumb Button 2"),
};

static const char* _joy_axis_descriptions[(size_t)JoyAxis::MAX] = {
	TTRC("Left Stick X-Axis, Joystick 0 X-Axis"),
	TTRC("Left Stick Y-Axis, Joystick 0 Y-Axis"),
	TTRC("Right Stick X-Axis, Joystick 1 X-Axis"),
	TTRC("Right Stick Y-Axis, Joystick 1 Y-Axis"),
	TTRC("Joystick 2 X-Axis, Left Trigger, Sony L2, Xbox LT"),
	TTRC("Joystick 2 Y-Axis, Right Trigger, Sony R2, Xbox RT"),
	TTRC("Joystick 3 X-Axis"),
	TTRC("Joystick 3 Y-Axis"),
	TTRC("Joystick 4 X-Axis"),
	TTRC("Joystick 4 Y-Axis"),
};

static const char* _joy_button_descriptions[(size_t)JoyButton::SDL_MAX] = {
	TTRC("Bottom Action, Sony Cross, Xbox A, Nintendo B"),
	TTRC("Right Action, Sony Circle, Xbox B, Nintendo A"),
	TTRC("Left Action, Sony Square, Xbox X, Nintendo Y"),
	TTRC("Top Action, Sony Triangle, Xbox Y, Nintendo X"),
	TTRC("Back, Sony Select, Xbox Back, Nintendo -"),
	TTRC("Guide, Sony PS, Xbox Home"),
	TTRC("Start, Xbox Menu, Nintendo +"),
	TTRC("Left Stick, Sony L3, Xbox L/LS"),
	TTRC("Right Stick, Sony R3, Xbox R/RS"),
	TTRC("Left Shoulder, Sony L1, Xbox LB"),
	TTRC("Right Shoulder, Sony R1, Xbox RB"),
	TTRC("D-pad Up"),
	TTRC("D-pad Down"),
	TTRC("D-pad Left"),
	TTRC("D-pad Right"),
	TTRC("Xbox Share, PS5 Microphone, Nintendo Capture"),
	TTRC("Xbox Paddle 1"),
	TTRC("Xbox Paddle 2"),
	TTRC("Xbox Paddle 3"),
	TTRC("Xbox Paddle 4"),
	TTRC("PS4/5 Touchpad"),
};

InputEvent InputEvent::make_key(Key p_keycode, bool p_physical)
{
	InputEvent ie;
	ie.type = InputEventType::KEY;
	ie.device = DEVICE_ID_KEYBOARD;
	ie.key = KeyData{};

	if (p_keycode == Key::NONE) {
		return ie;
	}

	if (p_physical) {
		ie.key.physical_keycode = p_keycode & KeyModifierMask::CODE_MASK;
	}
	else {
		ie.key.keycode = p_keycode & KeyModifierMask::CODE_MASK;
	}

	char32_t ch = char32_t(p_keycode & KeyModifierMask::CODE_MASK);
	if (ch < 0xd800 || (ch > 0xdfff && ch <= 0x10ffff)) {
		ie.key.unicode = ch;
	}

	if ((p_keycode & KeyModifierMask::SHIFT) != Key::NONE) {
		ie.set_shift_pressed(true);
	}
	if ((p_keycode & KeyModifierMask::ALT) != Key::NONE) {
		ie.set_alt_pressed(true);
	}
	if ((p_keycode & KeyModifierMask::CMD_OR_CTRL) != Key::NONE) {
		ie.set_command_or_control_autoremap(true);
		if ((p_keycode & KeyModifierMask::CTRL) != Key::NONE ||
			(p_keycode & KeyModifierMask::META) != Key::NONE) {
			WARN_PRINT("Invalid Key Modifiers: Command or Control autoremapping is enabled, Meta "
					   "and Control values are ignored!");
		}
	}
	else {
		if ((p_keycode & KeyModifierMask::CTRL) != Key::NONE) {
			ie.set_ctrl_pressed(true);
		}
		if ((p_keycode & KeyModifierMask::META) != Key::NONE) {
			ie.set_meta_pressed(true);
		}
	}

	return ie;
}

InputEvent InputEvent::make_mouse_button()
{
	InputEvent ie;
	ie.type = InputEventType::MOUSE_BUTTON;
	ie.device = DEVICE_ID_MOUSE;
	ie.mouse_button = MouseButtonData{};
	return ie;
}

InputEvent InputEvent::make_mouse_motion()
{
	InputEvent ie;
	ie.type = InputEventType::MOUSE_MOTION;
	ie.device = DEVICE_ID_MOUSE;
	ie.mouse_motion = MouseMotionData{};
	return ie;
}

InputEvent InputEvent::make_joypad_button(JoyButton p_btn_index, int p_device)
{
	InputEvent ie;
	ie.type = InputEventType::JOY_BUTTON;
	ie.device = p_device;
	ie.joy_button = JoypadButtonData{};
	ie.joy_button.button_index = p_btn_index;
	return ie;
}

InputEvent InputEvent::make_joypad_motion(JoyAxis p_axis, float p_value, int p_device)
{
	InputEvent ie;
	ie.type = InputEventType::JOY_MOTION;
	ie.device = p_device;
	ie.joy_motion = JoypadMotionData{};
	ie.set_joy_axis(p_axis);
	ie.set_joy_axis_value(p_value);
	return ie;
}

InputEvent InputEvent::make_screen_touch()
{
	InputEvent ie;
	ie.type = InputEventType::SCREEN_TOUCH;
	ie.touch = TouchData{};
	return ie;
}

InputEvent InputEvent::make_screen_drag()
{
	InputEvent ie;
	ie.type = InputEventType::SCREEN_DRAG;
	ie.screen_drag = ScreenDragData{};
	return ie;
}

InputEvent InputEvent::make_magnify_gesture()
{
	InputEvent ie;
	ie.type = InputEventType::MAGNIFY_GESTURE;
	ie.gesture = GestureData{};
	return ie;
}

InputEvent InputEvent::make_pan_gesture()
{
	InputEvent ie;
	ie.type = InputEventType::PAN_GESTURE;
	ie.gesture = GestureData{};
	return ie;
}

InputEvent InputEvent::make_midi()
{
	InputEvent ie;
	ie.type = InputEventType::MIDI;
	ie.midi = MIDIData{};
	return ie;
}

InputEvent InputEvent::make_action(const String& p_action, bool p_pressed, float p_strength)
{
	InputEvent ie;
	ie.type = InputEventType::ACTION;
	ie.pressed = p_pressed;
	ie.action.action = p_action;
	ie.action.strength = CLAMP(p_strength, 0.0f, 1.0f);
	ie.action.event_index = -1;
	return ie;
}

InputEvent InputEvent::make_shortcut(const Shortcut* p_shortcut)
{
	InputEvent ie;
	ie.type = InputEventType::SHORTCUT;
	ie.pressed = true;
	ie.shortcut = ShortcutData{p_shortcut};
	return ie;
}

Vector2 InputEvent::get_mouse_position() const
{
	if (type == InputEventType::MOUSE_BUTTON) {
		return mouse_button.position;
	}
	if (type == InputEventType::MOUSE_MOTION) {
		return mouse_motion.position;
	}
	return Vector2();
}

void InputEvent::set_mouse_position(const Vector2& p_pos)
{
	if (type == InputEventType::MOUSE_BUTTON) {
		mouse_button.position = p_pos;
	}
	else if (type == InputEventType::MOUSE_MOTION) {
		mouse_motion.position = p_pos;
	}
}

Vector2 InputEvent::get_mouse_global_position() const
{
	if (type == InputEventType::MOUSE_BUTTON) {
		return mouse_button.global_position;
	}
	if (type == InputEventType::MOUSE_MOTION) {
		return mouse_motion.global_position;
	}
	return Vector2();
}

void InputEvent::set_mouse_global_position(const Vector2& p_global_pos)
{
	if (type == InputEventType::MOUSE_BUTTON) {
		mouse_button.global_position = p_global_pos;
	}
	else if (type == InputEventType::MOUSE_MOTION) {
		mouse_motion.global_position = p_global_pos;
	}
}

uint32_t InputEvent::get_mouse_button_mask() const
{
	if (type == InputEventType::MOUSE_BUTTON) {
		return mouse_button.button_mask;
	}
	if (type == InputEventType::MOUSE_MOTION) {
		return mouse_motion.button_mask;
	}
	return 0;
}

void InputEvent::set_mouse_button_mask(uint32_t p_mask)
{
	if (type == InputEventType::MOUSE_BUTTON) {
		mouse_button.button_mask = p_mask;
	}
	else if (type == InputEventType::MOUSE_MOTION) {
		mouse_motion.button_mask = p_mask;
	}
}

void InputEvent::set_joy_axis(JoyAxis p_axis)
{
	ERR_FAIL_COND(p_axis < JoyAxis::INVALID || p_axis > JoyAxis::MAX);
	joy_motion.axis = p_axis;
}

void InputEvent::set_joy_axis_value(float p_value)
{
	joy_motion.axis_value = p_value;
	pressed = Math::abs(p_value) >= InputMap::DEFAULT_TOGGLE_DEADZONE;
}

void InputEvent::set_command_or_control_autoremap(bool p_enabled)
{
	if (is_command_or_control_autoremap() == p_enabled) {
		return;
	}
	if (p_enabled) {
		modifiers.set_flag(KeyModifierMask::CMD_OR_CTRL);
		if (OS::prefer_meta_over_ctrl()) {
			modifiers.clear_flag(KeyModifierMask::CTRL);
			modifiers.set_flag(KeyModifierMask::META);
		}
		else {
			modifiers.set_flag(KeyModifierMask::CTRL);
			modifiers.clear_flag(KeyModifierMask::META);
		}
	}
	else {
		modifiers.clear_flag(KeyModifierMask::CMD_OR_CTRL);
		modifiers.clear_flag(KeyModifierMask::CTRL);
		modifiers.clear_flag(KeyModifierMask::META);
	}
}

bool InputEvent::is_command_or_control_pressed() const
{
	return OS::prefer_meta_over_ctrl() ? is_meta_pressed() : is_ctrl_pressed();
}

void InputEvent::set_shift_pressed(bool p_pressed)
{
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::SHIFT);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::SHIFT);
	}
}

void InputEvent::set_alt_pressed(bool p_pressed)
{
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::ALT);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::ALT);
	}
}

void InputEvent::set_ctrl_pressed(bool p_pressed)
{
	ERR_FAIL_COND_MSG(is_command_or_control_autoremap(),
		"Command or Control autoremapping is enabled, cannot set Control directly!");
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::CTRL);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::CTRL);
	}
}

void InputEvent::set_meta_pressed(bool p_pressed)
{
	ERR_FAIL_COND_MSG(is_command_or_control_autoremap(),
		"Command or Control autoremapping is enabled, cannot set Meta directly!");
	if (p_pressed) {
		modifiers.set_flag(KeyModifierMask::META);
	}
	else {
		modifiers.clear_flag(KeyModifierMask::META);
	}
}

void InputEvent::copy_modifiers_from(const InputEvent& p_event)
{
	modifiers = p_event.modifiers;
}

String InputEvent::modifiers_as_text() const
{
	Vector<String> mod_names;
	if (is_ctrl_pressed()) {
		mod_names.push_back(find_keycode_name(Key::CTRL));
	}
	if (is_alt_pressed()) {
		mod_names.push_back(find_keycode_name(Key::ALT));
	}
	if (is_shift_pressed()) {
		mod_names.push_back(find_keycode_name(Key::SHIFT));
	}
	if (is_meta_pressed()) {
		mod_names.push_back(find_keycode_name(Key::META));
	}
	return mod_names.is_empty() ? String() : String("+").join(mod_names);
}

Key InputEvent::get_keycode_with_modifiers() const
{
	return type == InputEventType::KEY ? (key.keycode | get_modifiers_mask()) : Key::NONE;
}

Key InputEvent::get_physical_keycode_with_modifiers() const
{
	return type == InputEventType::KEY ? (key.physical_keycode | get_modifiers_mask()) : Key::NONE;
}

Key InputEvent::get_key_label_with_modifiers() const
{
	return type == InputEventType::KEY ? (key.key_label | get_modifiers_mask()) : Key::NONE;
}

String InputEvent::as_text_physical_keycode() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	String kc = (key.physical_keycode != Key::NONE) ? keycode_get_string(key.physical_keycode)
													: "(" + RTR("unset") + ")";
	if (kc.is_empty()) {
		return kc;
	}
	String mods_text = modifiers_as_text();
	return mods_text.is_empty() ? kc : mods_text + "+" + kc;
}

String InputEvent::as_text_keycode() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	String kc =
		(key.keycode != Key::NONE) ? keycode_get_string(key.keycode) : "(" + RTR("unset") + ")";
	if (kc.is_empty()) {
		return kc;
	}
	String mods_text = modifiers_as_text();
	return mods_text.is_empty() ? kc : mods_text + "+" + kc;
}

String InputEvent::as_text_key_label() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	String kc =
		(key.key_label != Key::NONE) ? keycode_get_string(key.key_label) : "(" + RTR("unset") + ")";
	if (kc.is_empty()) {
		return kc;
	}
	String mods_text = modifiers_as_text();
	return mods_text.is_empty() ? kc : mods_text + "+" + kc;
}

String InputEvent::as_text_location() const
{
	if (type != InputEventType::KEY) {
		return String();
	}
	switch (key.location) {
	case KeyLocation::LEFT:
		return "left";
	case KeyLocation::RIGHT:
		return "right";
	default:
		return String();
	}
}

bool InputEvent::is_action(const String& p_action, bool p_exact_match) const
{
	return InputMap::event_is_action(*this, p_action, p_exact_match);
}

bool InputEvent::is_action_pressed(
	const String& p_action, bool p_allow_echo, bool p_exact_match) const
{
	bool pressed_state = false;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, &pressed_state, nullptr, nullptr);
	return valid && pressed_state && (p_allow_echo || !is_echo());
}

bool InputEvent::is_action_released(const String& p_action, bool p_exact_match) const
{
	bool pressed_state = false;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, &pressed_state, nullptr, nullptr);
	return valid && !pressed_state;
}

bool InputEvent::is_action_just_pressed_or_echo(
	const String& p_action, bool p_exact_match) const
{
	return is_action(p_action, p_exact_match) &&
		   (is_echo() || Input::is_action_just_pressed_by_event(p_action, *this, p_exact_match));
}

float InputEvent::get_action_strength(const String& p_action, bool p_exact_match) const
{
	float strength = 0.0f;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, nullptr, &strength, nullptr);
	return valid ? strength : 0.0f;
}

float InputEvent::get_action_raw_strength(const String& p_action, bool p_exact_match) const
{
	float raw_strength = 0.0f;
	bool valid = InputMap::event_get_action_status(
		*this, p_action, p_exact_match, nullptr, nullptr, &raw_strength);
	return valid ? raw_strength : 0.0f;
}

bool InputEvent::is_action_type() const
{
	switch (type) {
	case InputEventType::KEY:
	case InputEventType::MOUSE_BUTTON:
	case InputEventType::JOY_BUTTON:
	case InputEventType::JOY_MOTION:
	case InputEventType::ACTION:
		return true;
	default:
		return false;
	}
}

bool InputEvent::action_match(const InputEvent& p_event, bool p_exact_match, float p_deadzone,
	bool* r_pressed, float* r_strength, float* r_raw_strength) const
{
	if (type != p_event.type) {
		return false;
	}

	switch (type) {
	case InputEventType::KEY: {
		bool match = false;
		if (key.keycode == Key::NONE && key.physical_keycode == Key::NONE &&
			key.key_label != Key::NONE) {
			match = (key.key_label == p_event.key.key_label);
		}
		else if (key.keycode != Key::NONE) {
			match = (key.keycode == p_event.key.keycode);
		}
		else if (key.physical_keycode != Key::NONE) {
			match = (key.physical_keycode == p_event.key.physical_keycode);
			if (key.location != KeyLocation::UNSPECIFIED) {
				match &= (key.location == p_event.key.location);
			}
		}

		uint32_t action_mask = get_modifiers_mask();
		uint32_t key_mask = p_event.get_modifiers_mask();
		if (p_event.is_pressed()) {
			match &= ((action_mask & key_mask) == action_mask);
		}
		if (p_exact_match) {
			match &= (action_mask == key_mask);
		}
		if (match) {
			bool key_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = key_pressed;
			}
			float strength = key_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = strength;
			}
		}
		return match;
	}

	case InputEventType::MOUSE_BUTTON: {
		bool match = (mouse_button.button_index == p_event.mouse_button.button_index);
		uint32_t action_mask = get_modifiers_mask();
		uint32_t btn_mask = p_event.get_modifiers_mask();
		if (p_event.is_pressed()) {
			match &= ((action_mask & btn_mask) == action_mask);
		}
		if (p_exact_match) {
			match &= (action_mask == btn_mask);
		}
		if (match) {
			bool mb_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = mb_pressed;
			}
			float strength = mb_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = strength;
			}
		}
		return match;
	}

	case InputEventType::JOY_MOTION: {
		bool match = (joy_motion.axis == p_event.joy_motion.axis);
		if (p_exact_match) {
			match &= ((joy_motion.axis_value < 0) == (p_event.joy_motion.axis_value < 0));
		}
		if (match) {
			float jm_abs = Math::abs(p_event.joy_motion.axis_value);
			bool same_direction =
				(((joy_motion.axis_value < 0) == (p_event.joy_motion.axis_value < 0)) ||
					p_event.joy_motion.axis_value == 0);
			bool pressed_state = same_direction && jm_abs >= p_deadzone;
			if (r_pressed) {
				*r_pressed = pressed_state;
			}
			if (r_strength) {
				if (pressed_state) {
					*r_strength = (p_deadzone == 1.0f)
									  ? 1.0f
									  : CLAMP(Math::inverse_lerp(p_deadzone, 1.0f, jm_abs), 0.0f,
											1.0f);
				}
				else {
					*r_strength = 0.0f;
				}
			}
			if (r_raw_strength) {
				*r_raw_strength = same_direction ? jm_abs : 0.0f;
			}
		}
		return match;
	}

	case InputEventType::JOY_BUTTON: {
		bool match = (joy_button.button_index == p_event.joy_button.button_index);
		if (match) {
			bool jb_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = jb_pressed;
			}
			float strength = jb_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = strength;
			}
		}
		return match;
	}

	case InputEventType::ACTION: {
		bool match = (action.action == p_event.action.action);
		if (match) {
			bool act_pressed = p_event.is_pressed();
			if (r_pressed) {
				*r_pressed = act_pressed;
			}
			float act_strength = act_pressed ? 1.0f : 0.0f;
			if (r_strength) {
				*r_strength = act_strength;
			}
			if (r_raw_strength) {
				*r_raw_strength = act_strength;
			}
		}
		return match;
	}

	default:
		return false;
	}
}

bool InputEvent::is_match(const InputEvent& p_event, bool p_exact_match) const
{
	if (type == InputEventType::ACTION) {
		return p_event.is_valid() && p_event.is_action(action.action, p_exact_match);
	}
	if (type != p_event.type) {
		return false;
	}

	switch (type) {
	case InputEventType::KEY:
		if (key.keycode == Key::NONE && key.physical_keycode == Key::NONE &&
			key.key_label != Key::NONE) {
			return (key.key_label == p_event.key.key_label) &&
				   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());
		}
		else if (key.keycode != Key::NONE) {
			return (key.keycode == p_event.key.keycode) &&
				   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());
		}
		else if (key.physical_keycode != Key::NONE) {
			if (key.location != KeyLocation::UNSPECIFIED && key.location != p_event.key.location) {
				return false;
			}
			return (key.physical_keycode == p_event.key.physical_keycode) &&
				   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());
		}
		return false;

	case InputEventType::MOUSE_BUTTON:
		return (mouse_button.button_index == p_event.mouse_button.button_index) &&
			   (!p_exact_match || get_modifiers_mask() == p_event.get_modifiers_mask());

	case InputEventType::JOY_MOTION:
		return (joy_motion.axis == p_event.joy_motion.axis) &&
			   (!p_exact_match || ((joy_motion.axis_value < 0) == (p_event.joy_motion.axis_value < 0)));

	case InputEventType::JOY_BUTTON:
		return joy_button.button_index == p_event.joy_button.button_index;

	default:
		return false;
	}
}

InputEvent InputEvent::xformed_by(const Transform2D& p_xform, const Vector2& p_local_ofs) const
{
	InputEvent ev = *this;
	switch (type) {
	case InputEventType::MOUSE_BUTTON:
		ev.mouse_button.position = p_xform.xform(mouse_button.position + p_local_ofs);
		break;
	case InputEventType::MOUSE_MOTION:
		ev.mouse_motion.position = p_xform.xform(mouse_motion.position + p_local_ofs);
		ev.mouse_motion.relative = p_xform.basis_xform(mouse_motion.relative);
		ev.mouse_motion.velocity = p_xform.basis_xform(mouse_motion.velocity);
		break;
	case InputEventType::SCREEN_TOUCH:
		ev.touch.position = p_xform.xform(touch.position + p_local_ofs);
		break;
	case InputEventType::SCREEN_DRAG:
		ev.screen_drag.position = p_xform.xform(screen_drag.position + p_local_ofs);
		ev.screen_drag.relative = p_xform.basis_xform(screen_drag.relative);
		ev.screen_drag.velocity = p_xform.basis_xform(screen_drag.velocity);
		break;
	case InputEventType::MAGNIFY_GESTURE:
	case InputEventType::PAN_GESTURE:
		ev.gesture.position = p_xform.xform(gesture.position + p_local_ofs);
		break;
	default:
		break;
	}
	return ev;
}

bool InputEvent::accumulate(const InputEvent& p_event)
{
	if (type != p_event.type) {
		return false;
	}

	if (type == InputEventType::MOUSE_MOTION) {
		if (window_id != p_event.window_id || is_canceled() != p_event.is_canceled() ||
			is_pressed() != p_event.is_pressed() ||
			mouse_motion.button_mask != p_event.mouse_motion.button_mask ||
			get_modifiers_mask() != p_event.get_modifiers_mask()) {
			return false;
		}
		mouse_motion.position = p_event.mouse_motion.position;
		mouse_motion.global_position = p_event.mouse_motion.global_position;
		mouse_motion.velocity = p_event.mouse_motion.velocity;
		mouse_motion.screen_velocity = p_event.mouse_motion.screen_velocity;
		mouse_motion.relative += p_event.mouse_motion.relative;
		mouse_motion.screen_relative += p_event.mouse_motion.screen_relative;
		return true;
	}

	if (type == InputEventType::SCREEN_DRAG) {
		if (screen_drag.index != p_event.screen_drag.index) {
			return false;
		}
		screen_drag.position = p_event.screen_drag.position;
		screen_drag.velocity = p_event.screen_drag.velocity;
		screen_drag.screen_velocity = p_event.screen_drag.screen_velocity;
		screen_drag.relative += p_event.screen_drag.relative;
		screen_drag.screen_relative += p_event.screen_drag.screen_relative;
		return true;
	}

	return false;
}

String InputEvent::as_text() const
{
	switch (type) {
	case InputEventType::KEY: {
		String kc;
		if (key.keycode == Key::NONE && key.physical_keycode == Key::NONE &&
			key.key_label != Key::NONE) {
			kc = keycode_get_string(key.key_label) + " - Unicode";
		}
		else if (key.keycode != Key::NONE) {
			kc = keycode_get_string(key.keycode);
		}
		else if (key.physical_keycode != Key::NONE) {
			kc = keycode_get_string(key.physical_keycode);
			if ((key.physical_keycode & Key::SPECIAL) != Key::SPECIAL) {
				kc += " - " + RTR("Physical");
			}
		}
		else {
			kc = "(" + RTR("unset") + ")";
		}
		if (kc.is_empty()) {
			return kc;
		}
		String mods_text = modifiers_as_text();
		return mods_text.is_empty() ? kc : mods_text + "+" + kc;
	}

	case InputEventType::MOUSE_BUTTON: {
		String mods_text = modifiers_as_text();
		String full_string = mods_text.is_empty() ? "" : mods_text + "+";
		MouseButton idx = mouse_button.button_index;
		switch (idx) {
		case MouseButton::LEFT:
		case MouseButton::RIGHT:
		case MouseButton::MIDDLE:
		case MouseButton::WHEEL_UP:
		case MouseButton::WHEEL_DOWN:
		case MouseButton::WHEEL_LEFT:
		case MouseButton::WHEEL_RIGHT:
		case MouseButton::MB_XBUTTON1:
		case MouseButton::MB_XBUTTON2:
			full_string += RTR(_mouse_button_descriptions[(size_t)idx - 1]);
			break;
		default:
			full_string += RTR("Button") + " #" + itos((int64_t)idx);
			break;
		}
		if (mouse_button.double_click) {
			full_string += " (" + RTR("Double Click") + ")";
		}
		return full_string;
	}

	case InputEventType::MOUSE_MOTION:
		return vformat(RTR("Mouse motion at position (%s) with velocity (%s)"),
			String(mouse_motion.position), String(mouse_motion.velocity));

	case InputEventType::JOY_MOTION: {
		String desc = joy_motion.axis < JoyAxis::MAX
						  ? TTRGET(_joy_axis_descriptions[(size_t)joy_motion.axis])
						  : RTR("Unknown Joypad Axis");
		return vformat(RTR("Joypad Motion on Axis %d (%s) with Value %.2f"), joy_motion.axis, desc,
			joy_motion.axis_value);
	}

	case InputEventType::JOY_BUTTON: {
		String text = vformat(RTR("Joypad Button %d"), (int64_t)joy_button.button_index);
		if (joy_button.button_index > JoyButton::INVALID &&
			joy_button.button_index < JoyButton::SDL_MAX) {
			text +=
				vformat(" (%s)", TTRGET(_joy_button_descriptions[(size_t)joy_button.button_index]));
		}
		if (joy_button.pressure != 0) {
			text += ", " + RTR("Pressure:") + " " + String(joy_button.pressure);
		}
		return text;
	}

	case InputEventType::SCREEN_TOUCH: {
		String status = canceled ? RTR("canceled") : (pressed ? RTR("touched") : RTR("released"));
		return vformat(RTR("Screen %s at (%s) with %s touch points"), status,
			String(touch.position), itos(touch.index));
	}

	case InputEventType::SCREEN_DRAG:
		return vformat(
			RTR("Screen dragged with %s touch points at position (%s) with velocity of (%s)"),
			itos(screen_drag.index), String(screen_drag.position), String(screen_drag.velocity));

	case InputEventType::ACTION: {
		const List<InputEvent>* events = InputMap::action_get_events(action.action);
		if (!events) {
			return String();
		}
		for (const InputEvent& E : *events) {
			if (E.is_valid()) {
				return E.as_text();
			}
		}
		return String();
	}

	case InputEventType::MAGNIFY_GESTURE:
		return vformat(RTR("Magnify Gesture at (%s) with factor %s"), String(gesture.position),
			rtos(gesture.factor));

	case InputEventType::PAN_GESTURE:
		return vformat(RTR("Pan Gesture at (%s) with delta (%s)"), String(gesture.position),
			String(gesture.delta));

	case InputEventType::MIDI:
		return vformat(RTR("MIDI Input on Channel=%s Message=%s"), itos(midi.channel),
			itos((int64_t)midi.message));

	case InputEventType::SHORTCUT:
		ERR_FAIL_NULL_V(shortcut.shortcut, "None");
		return vformat(RTR("Input Event with Shortcut=%s"), shortcut.shortcut->get_as_text());

	default:
		return String();
	}
}

String InputEvent::to_string() const { return as_text(); }


// --- Merged from core/os/os.cpp ---
#include <cstdarg>
#include <cstdio>
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/config_file.h"
#include "core/os/os.h"
#include "core/version_generated.gen.h"
#include "core/os/os.h"

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.thread.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <thread>
#define THREADING_NAMESPACE std
#endif

bool OS::prefer_meta_over_ctrl()
{
#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
	return true;
#elif defined(WEB_ENABLED)
	return has_feature("web_macos") || has_feature("web_ios");
#else
	return false;
#endif
}

void OS::delete_main_loop() { MainLoop::clear(); }

uint64_t OS::get_ticks_msec() { return get_ticks_usec() / 1000ULL; }

double OS::get_unix_time() { return 0; }

void OS::_set_logger(CompositeLogger* p_logger)
{
	if (_logger) {
		memdelete(_logger);
	}
	_logger = p_logger;
}

void OS::add_logger(Logger* p_logger)
{
	if (!_logger) {
		Vector<Logger*> loggers;
		loggers.push_back(p_logger);
		_logger = memnew(CompositeLogger(loggers));
	}
	else {
		_logger->add_logger(p_logger);
	}
}

String OS::get_identifier() { return get_name().to_lower(); }

void OS::print(const char* p_format, ...)
{
	if (!_stdout_enabled) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	if (_logger) {
		_logger->logv(p_format, argp, false);
	}

	va_end(argp);
}

void OS::print_rich(const char* p_format, ...)
{
	if (!_stdout_enabled) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	if (_logger) {
		_logger->logv(p_format, argp, false);
	}

	va_end(argp);
}

void OS::printerr(const char* p_format, ...)
{
	if (!_stderr_enabled) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	if (_logger) {
		_logger->logv(p_format, argp, true);
	}

	va_end(argp);
}

void OS::alert(const String& p_alert, const String& p_title)
{
	fprintf(stderr, "%s: %s\n", p_title.utf8().get_data(), p_alert.utf8().get_data());
}

void OS::set_low_processor_usage_mode(bool p_enabled) { low_processor_usage_mode = p_enabled; }

bool OS::is_in_low_processor_usage_mode() { return low_processor_usage_mode; }

void OS::set_low_processor_usage_mode_sleep_usec(int p_usec)
{
	low_processor_usage_mode_sleep_usec = p_usec;
}

int OS::get_low_processor_usage_mode_sleep_usec()
{
	return low_processor_usage_mode_sleep_usec;
}

void OS::set_delta_smoothing(bool p_enabled) { _delta_smoothing_enabled = p_enabled; }

bool OS::is_delta_smoothing_enabled() { return _delta_smoothing_enabled; }

String OS::get_executable_path() { return _execpath; }

int OS::get_process_id() { return -1; }

bool OS::is_stdout_verbose() { return _verbose_stdout; }

bool OS::is_stdout_debug_enabled() { return _debug_stdout; }

bool OS::is_stdout_enabled() { return _stdout_enabled; }

bool OS::is_stderr_enabled() { return _stderr_enabled; }

void OS::set_stdout_enabled(bool p_enabled) { _stdout_enabled = p_enabled; }

void OS::set_stderr_enabled(bool p_enabled) { _stderr_enabled = p_enabled; }

String OS::multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array)
{
	return String();
}

PackedByteArray OS::string_to_multibyte(const String& p_encoding, const String& p_string)
{
	return PackedByteArray();
}

int OS::get_exit_code() { return _exit_code; }

void OS::set_exit_code(int p_code) { _exit_code = p_code; }

String OS::get_locale() { return "en"; }

// Non-virtual helper to extract the 2 or 3-letter language code from
// `get_locale()` in a way that's consistent for all platforms.
String OS::get_locale_language() { return get_locale().left(3).remove_char('_'); }

// Embedded PCK offset.
uint64_t OS::get_embedded_pck_offset() { return 0; }

// Default boot screen rect scale mode is "Keep Aspect Centered"
Rect2 OS::calculate_boot_screen_rect(const Size2& p_window_size, const Size2& p_imgrect_size)
{
	Rect2 screenrect;
	if (p_window_size.width > p_window_size.height) {
		// Scale horizontally.
		screenrect.size.y = p_window_size.height;
		screenrect.size.x = p_imgrect_size.x * p_window_size.height / p_imgrect_size.y;
		screenrect.position.x = (p_window_size.width - screenrect.size.x) / 2;
	}
	else {
		// Scale vertically.
		screenrect.size.x = p_window_size.width;
		screenrect.size.y = p_imgrect_size.y * p_window_size.width / p_imgrect_size.x;
		screenrect.position.y = (p_window_size.height - screenrect.size.y) / 2;
	}
	return screenrect;
}

// Helper function to ensure that a dir name/path will be valid on the OS
String OS::get_safe_dir_name(const String& p_dir_name, bool p_allow_paths)
{
	String safe_dir_name = p_dir_name;
	Vector<String> invalid_chars = String(": * ? \" < > |").split(" ");
	if (p_allow_paths) {
		// Dir separators are allowed, but disallow ".." to avoid going up the filesystem
		invalid_chars.push_back("..");
		safe_dir_name = safe_dir_name.replace_char('\\', '/').replace("//", "/").strip_edges();
	}
	else {
		invalid_chars.push_back("/");
		invalid_chars.push_back("\\");
		safe_dir_name = safe_dir_name.strip_edges();

		// These directory names are invalid.
		if (safe_dir_name == ".") {
			safe_dir_name = "dot";
		}
		else if (safe_dir_name == "..") {
			safe_dir_name = "twodots";
		}
	}

	for (int i = 0; i < invalid_chars.size(); i++) {
		safe_dir_name = safe_dir_name.replace(invalid_chars[i], "-");
	}

	// Trim trailing periods from the returned value as it's not valid for folder names on Windows.
	// This check is still applied on non-Windows platforms so the returned value is consistent
	// across platforms.
	return safe_dir_name.rstrip(".");
}

// Path to data, config, cache, etc. OS-specific folders

// Get properly capitalized engine name for system paths
String OS::get_godot_dir_name()
{
	// Default to lowercase, so only override when different case is needed
	return String(VLTR_VERSION_SHORT_NAME).to_lower();
}

// OS equivalent of XDG_DATA_HOME
String OS::get_data_path() { return "."; }

// OS equivalent of XDG_CONFIG_HOME
String OS::get_config_path() { return "."; }

// OS equivalent of XDG_CACHE_HOME
String OS::get_cache_path() { return "."; }

String OS::get_temp_path() { return "."; }

// Path to macOS .app bundle resources
String OS::get_bundle_resource_dir() { return "."; }

// Path to macOS .app bundle embedded icon (.icns file).
String OS::get_bundle_icon_path() { return String(); }

// Name of macOS .app bundle embedded icon (Liquid Glass asset name).
String OS::get_bundle_icon_name() { return String(); }

// OS specific path for user://
String OS::get_user_data_dir(const String& p_user_dir) { return "."; }

// Absolute path to res://
String OS::get_resource_dir()
{
	return ProjectSettings::get_resource_path();
}

// Access system-specific dirs like Documents, Downloads, etc.
String OS::get_system_dir(SystemDir p_dir, bool p_shared_storage) { return "."; }

String OS::expand_path(const String& p_path) { return p_path; }

void OS::create_lock_file()
{
	if (Engine::is_recovery_mode_hint()) {
		return;
	}

	String lock_file_path = get_user_data_dir().path_join(".recovery_mode_lock");
	Ref<FileAccess> lock_file = FileAccess::open(lock_file_path, FileAccess::WRITE);
	if (lock_file.is_valid()) {
		lock_file->close();
	}
}

void OS::remove_lock_file()
{
	String lock_file_path = get_user_data_dir().path_join(".recovery_mode_lock");
	DirAccess::remove_absolute(lock_file_path);
}

#if !defined(LINUXBSD_ENABLED)
Error OS::shell_open(const String& p_uri) { return ERR_UNAVAILABLE; }
#endif

Error OS::shell_show_in_file_manager(String p_path, bool p_open_folder)
{
	p_path = p_path.trim_prefix("file://");

	if (!DirAccess::dir_exists_absolute(p_path)) {
		p_path = p_path.get_base_dir();
	}

	p_path = String("file://") + p_path;

	return shell_open(p_path);
}

// implement these with the canvas?

uint64_t OS::get_static_memory_usage() { return Memory::get_mem_usage(); }

uint64_t OS::get_static_memory_peak_usage() { return Memory::get_mem_max_usage(); }

#if !defined(LINUXBSD_ENABLED)
Error OS::set_cwd(const String& p_cwd) { return ERR_CANT_OPEN; }

String OS::get_cwd() { return "."; }
#endif

void OS::yield() {}

void OS::ensure_user_data_dir()
{
	String dd = get_user_data_dir();
	if (DirAccess::exists(dd)) {
		return;
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	Error err = da->make_dir_recursive(dd);
	ERR_FAIL_COND_MSG(err != OK, vformat("Error attempting to create data dir: %s.", dd));
}

String OS::get_model_name() { return "GenericDevice"; }

void OS::set_cmdline(
	const char* p_execpath, const List<String>& p_args, const List<String>& p_user_args)
{
	_execpath = String::utf8(p_execpath);
	_cmdline = p_args;
	_user_args = p_user_args;
}

String OS::get_unique_id() { return ""; }

int OS::get_processor_count() { return THREADING_NAMESPACE::thread::hardware_concurrency(); }

String OS::get_processor_name() { return ""; }

void OS::set_has_server_feature_callback(HasServerFeatureCallback p_callback)
{
	has_server_feature_callback = p_callback;
}

bool OS::has_feature(const String& p_feature)
{
	// Feature tags are always lowercase for consistency.
	if (p_feature == get_identifier()) {
		return true;
	}

	if (p_feature == "movie") {
		return _writing_movie;
	}

#ifdef DEBUG_ENABLED
	if (p_feature == "debug") {
		return true;
	}
#endif // DEBUG_ENABLED

#ifdef TOOLS_ENABLED
	if (p_feature == "editor") {
		return true;
	}
	if (p_feature == "editor_hint") {
		return _in_editor;
	}
	else if (p_feature == "editor_runtime") {
		return !_in_editor;
	}
	else if (p_feature == "embedded_in_editor") {
		return _embedded_in_editor;
	}
#else
	if (p_feature == "template") {
		return true;
	}
#ifdef DEBUG_ENABLED
	if (p_feature == "template_debug") {
		return true;
	}
#else
	if (p_feature == "template_release" || p_feature == "release") {
		return true;
	}
#endif // DEBUG_ENABLED
#endif // TOOLS_ENABLED

#ifdef REAL_T_IS_DOUBLE
	if (p_feature == "double") {
		return true;
	}
#else
	if (p_feature == "single") {
		return true;
	}
#endif // REAL_T_IS_DOUBLE

	if (sizeof(void*) == 8 && p_feature == "64") {
		return true;
	}
	if (sizeof(void*) == 4 && p_feature == "32") {
		return true;
	}
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__i386) ||           \
	defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
#if defined(MACOS_ENABLED)
	if (p_feature == "universal") {
		return true;
	}
#endif
	if (p_feature == "x86_64") {
		return true;
	}
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
	if (p_feature == "x86_32") {
		return true;
	}
#endif
	if (p_feature == "x86") {
		return true;
	}
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#if defined(__aarch64__) || defined(_M_ARM64)
#if defined(MACOS_ENABLED)
	if (p_feature == "universal") {
		return true;
	}
#endif
	if (p_feature == "arm64") {
		return true;
	}
#elif defined(__arm__) || defined(_M_ARM)
	if (p_feature == "arm32") {
		return true;
	}
#endif
#if defined(__ARM_ARCH_7A__)
	if (p_feature == "armv7a" || p_feature == "armv7") {
		return true;
	}
#endif
#if defined(__ARM_ARCH_7S__)
	if (p_feature == "armv7s" || p_feature == "armv7") {
		return true;
	}
#endif
	if (p_feature == "arm") {
		return true;
	}
#elif defined(__riscv)
#if __riscv_xlen == 8
	if (p_feature == "rv64") {
		return true;
	}
#endif
	if (p_feature == "riscv") {
		return true;
	}
#elif defined(__powerpc__)
#if defined(__powerpc64__)
	if (p_feature == "ppc64") {
		return true;
	}
#endif
	if (p_feature == "ppc") {
		return true;
	}
#elif defined(__wasm__)
#if defined(__wasm64__)
	if (p_feature == "wasm64") {
		return true;
	}
#elif defined(__wasm32__)
	if (p_feature == "wasm32") {
		return true;
	}
#endif
	if (p_feature == "wasm") {
		return true;
	}
#elif defined(__loongarch64)
	if (p_feature == "loongarch64") {
		return true;
	}
#endif

#if defined(IOS_SIMULATOR) || defined(VISIONOS_SIMULATOR)
	if (p_feature == "simulator") {
		return true;
	}
#endif

	if (p_feature == "threads") {
#ifdef THREADS_ENABLED
		return true;
#else
		return false;
#endif
	}
	if (p_feature == "nothreads") {
#ifdef THREADS_ENABLED
		return false;
#else
		return true;
#endif
	}

	if (_check_internal_feature_support(p_feature)) {
		return true;
	}

	if (has_server_feature_callback && has_server_feature_callback(p_feature)) {
		return true;
	}

	if (ProjectSettings::has_custom_feature(p_feature)) {
		return true;
	}

	return false;
}

bool OS::is_sandboxed() { return false; }

void OS::set_restart_on_exit(bool p_restart, const List<String>& p_restart_arguments)
{
	restart_on_exit = p_restart;
	restart_commandline = p_restart_arguments;
}

bool OS::is_restart_on_exit_set() { return restart_on_exit; }

List<String> OS::get_restart_on_exit_arguments() { return List<String>(restart_commandline); }

PackedStringArray OS::get_connected_midi_inputs()
{
	if (MIDIDriver::is_bound()) {
		return MIDIDriver::get_connected_inputs();
	}

	PackedStringArray list;
	ERR_FAIL_V_MSG(
		list, vformat("MIDI input isn't supported on %s.", OS::get_name()));
}

void OS::open_midi_inputs()
{
	if (MIDIDriver::is_bound()) {
		MIDIDriver::open();
	}
	else {
		ERR_PRINT(vformat("MIDI input isn't supported on %s.", OS::get_name()));
	}
}

void OS::close_midi_inputs()
{
	if (MIDIDriver::is_bound()) {
		MIDIDriver::close();
	}
	else {
		ERR_PRINT(vformat("MIDI input isn't supported on %s.", OS::get_name()));
	}
}

uint64_t OS::get_frame_delay(bool p_can_draw)
{
	const uint32_t frame_delay = Engine::get_frame_delay();

	// Add a dynamic frame delay to decrease CPU/GPU usage. This takes the
	// previous frame time into account for a smoother result.
	uint64_t dynamic_delay = 0;
	if (is_in_low_processor_usage_mode() || !p_can_draw) {
		dynamic_delay = get_low_processor_usage_mode_sleep_usec();
	}
	const int max_fps = Engine::get_max_fps();
	if (max_fps > 0 && !Engine::is_editor_hint()) {
		// Override the low processor usage mode sleep delay if the target FPS is lower.
		dynamic_delay = MAX(dynamic_delay, (uint64_t)(1000000 / max_fps));
	}

	return frame_delay + dynamic_delay;
}

void OS::add_frame_delay(bool p_can_draw, bool p_wake_for_events)
{
	const uint32_t frame_delay = Engine::get_frame_delay();
	if (frame_delay) {
		// Add fixed frame delay to decrease CPU/GPU usage. This doesn't take
		// the actual frame time into account.
		// Due to the high fluctuation of the actual sleep duration, it's not recommended
		// to use this as a FPS limiter.
		delay_usec(frame_delay * 1000);
	}

	// Add a dynamic frame delay to decrease CPU/GPU usage. This takes the
	// previous frame time into account for a smoother result.
	uint64_t dynamic_delay = 0;
	if (is_in_low_processor_usage_mode() || !p_can_draw) {
		dynamic_delay = get_low_processor_usage_mode_sleep_usec();
	}
	const int max_fps = Engine::get_max_fps();
	if (max_fps > 0 && !Engine::is_editor_hint()) {
		// Override the low processor usage mode sleep delay if the target FPS is lower.
		dynamic_delay = MAX(dynamic_delay, (uint64_t)(1000000 / max_fps));
	}

	if (dynamic_delay > 0) {
		target_ticks += dynamic_delay;
		uint64_t current_ticks = get_ticks_usec();

		if (current_ticks < target_ticks) {
			delay_usec(target_ticks - current_ticks);
		}

		current_ticks = get_ticks_usec();
		target_ticks =
			MIN(MAX(target_ticks, current_ticks - dynamic_delay), current_ticks + dynamic_delay);
	}
}

Error OS::setup_remote_filesystem(
	const String& p_server_host, int p_port, const String& p_password, String& r_project_path)
{
	return default_rfs.synchronize_with_server(p_server_host, p_port, p_password, r_project_path);
}

OS::PreferredTextureFormat OS::get_preferred_texture_format()
{
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
	return PREFERRED_TEXTURE_FORMAT_ETC2_ASTC; // By rule, ARM hardware uses ETC texture
											   // compression.
#elif defined(__x86_64__) || defined(_M_X64) || defined(i386) || defined(__i386__) ||              \
	defined(__i386) || defined(_M_IX86)
	return PREFERRED_TEXTURE_FORMAT_S3TC_BPTC; // By rule, X86 hardware prefers S3TC and
											   // derivatives.
#else
	return PREFERRED_TEXTURE_FORMAT_S3TC_BPTC; // Override in platform if needed.
#endif
}

void OS::set_use_benchmark(bool p_use_benchmark) { use_benchmark = p_use_benchmark; }

bool OS::is_use_benchmark_set() { return use_benchmark; }

void OS::set_benchmark_file(const String& p_benchmark_file) { benchmark_file = p_benchmark_file; }

String OS::get_benchmark_file() { return benchmark_file; }

void OS::benchmark_begin_measure(const String& p_context, const String& p_what)
{
#ifdef TOOLS_ENABLED
	Pair<String, String> mark_key(p_context, p_what);
	ERR_FAIL_COND_MSG(benchmark_marks_from.has(mark_key),
		vformat("Benchmark key '%s:%s' already exists.", p_context, p_what));

	benchmark_marks_from[mark_key] = OS::get_ticks_usec();
#endif
}

void OS::benchmark_end_measure(const String& p_context, const String& p_what)
{
#ifdef TOOLS_ENABLED
	Pair<String, String> mark_key(p_context, p_what);
	ERR_FAIL_COND_MSG(!benchmark_marks_from.has(mark_key),
		vformat("Benchmark key '%s:%s' doesn't exist.", p_context, p_what));

	uint64_t total = OS::get_ticks_usec() - benchmark_marks_from[mark_key];
	double total_f = double(total) / double(1000000);
	benchmark_marks_final[mark_key] = total_f;
#endif
}

void OS::benchmark_dump()
{
#ifdef TOOLS_ENABLED
	if (!use_benchmark) {
		return;
	}

	if (!benchmark_file.is_empty()) {
		Ref<FileAccess> f = FileAccess::open(benchmark_file, FileAccess::WRITE);
		if (f.is_valid()) {
			Ref<JSON> json;
			json.instantiate();
		}
	}
	else {
		HashMap<String, String> results;
		for (const KeyValue<Pair<String, String>, double>& E : benchmark_marks_final) {
			if (E.key.first == "Startup" && !results.has(E.key.first)) {
				results.insert(
					E.key.first, "", true); // Hack to make sure "Startup" always comes first.
			}

			results[E.key.first] +=
				vformat("\t\t- %s: %.3f msec.\n", E.key.second, (E.value * 1000));
		}

		__print_line("BENCHMARK:");
		for (const KeyValue<String, String>& E : results) {
			__print_line(vformat("\t[%s]\n%s", E.key, E.value));
		}
	}
#endif
}

bool OS::is_initialized()
{
	return _initialized;
}

String OS::get_user_data_dir() { return String(); }

#ifdef MINGW_STDTHREAD_REDUNDANCY_WARNING
#undef MINGW_STDTHREAD_REDUNDANCY_WARNING
#endif


// ============================================================================
// Merged from core/os/keyboard.cpp
// ============================================================================

#include "core/os/keyboard.h"

#include "core/os/os.h"
#include "core/string/ustring.h"

struct _KeyCodeText {
	Key code;
	const char *text;
};

static const _KeyCodeText _keycodes[] = {
	/* clang-format off */
	{Key::ESCAPE                ,"Escape"},
	{Key::TAB                   ,"Tab"},
	{Key::BACKTAB               ,"Backtab"},
	{Key::BACKSPACE             ,"Backspace"},
	{Key::ENTER                 ,"Enter"},
	{Key::KP_ENTER              ,"Kp Enter"},
	{Key::INSERT                ,"Insert"},
	{Key::KEY_DELETE            ,"Delete"},
	{Key::PAUSE                 ,"Pause"},
	{Key::PRINT                 ,"Print"},
	{Key::SYSREQ                ,"SysReq"},
	{Key::CLEAR                 ,"Clear"},
	{Key::HOME                  ,"Home"},
	{Key::END                   ,"End"},
	{Key::LEFT                  ,"Left"},
	{Key::UP                    ,"Up"},
	{Key::RIGHT                 ,"Right"},
	{Key::DOWN                  ,"Down"},
	{Key::PAGEUP                ,"PageUp"},
	{Key::PAGEDOWN              ,"PageDown"},
	{Key::SHIFT                 ,"Shift"},
	{Key::CTRL                  ,"Ctrl"},
#if defined(MACOS_ENABLED)
	{Key::META                  ,"Command"},
	{Key::CMD_OR_CTRL           ,"Command"},
	{Key::ALT                   ,"Option"},
#elif defined(WINDOWS_ENABLED)
	{Key::META                  ,"Windows"},
	{Key::CMD_OR_CTRL           ,"Ctrl"},
	{Key::ALT                   ,"Alt"},
#else
	{Key::META                  ,"Meta"},
	{Key::CMD_OR_CTRL           ,"Ctrl"},
	{Key::ALT                   ,"Alt"},
#endif
	{Key::CAPSLOCK              ,"CapsLock"},
	{Key::NUMLOCK               ,"NumLock"},
	{Key::SCROLLLOCK            ,"ScrollLock"},
	{Key::F1                    ,"F1"},
	{Key::F2                    ,"F2"},
	{Key::F3                    ,"F3"},
	{Key::F4                    ,"F4"},
	{Key::F5                    ,"F5"},
	{Key::F6                    ,"F6"},
	{Key::F7                    ,"F7"},
	{Key::F8                    ,"F8"},
	{Key::F9                    ,"F9"},
	{Key::F10                   ,"F10"},
	{Key::F11                   ,"F11"},
	{Key::F12                   ,"F12"},
	{Key::F13                   ,"F13"},
	{Key::F14                   ,"F14"},
	{Key::F15                   ,"F15"},
	{Key::F16                   ,"F16"},
	{Key::F17                   ,"F17"},
	{Key::F18                   ,"F18"},
	{Key::F19                   ,"F19"},
	{Key::F20                   ,"F20"},
	{Key::F21                   ,"F21"},
	{Key::F22                   ,"F22"},
	{Key::F23                   ,"F23"},
	{Key::F24                   ,"F24"},
	{Key::F25                   ,"F25"},
	{Key::F26                   ,"F26"},
	{Key::F27                   ,"F27"},
	{Key::F28                   ,"F28"},
	{Key::F29                   ,"F29"},
	{Key::F30                   ,"F30"},
	{Key::F31                   ,"F31"},
	{Key::F32                   ,"F32"},
	{Key::F33                   ,"F33"},
	{Key::F34                   ,"F34"},
	{Key::F35                   ,"F35"},
	{Key::KP_MULTIPLY           ,"Kp Multiply"},
	{Key::KP_DIVIDE             ,"Kp Divide"},
	{Key::KP_SUBTRACT           ,"Kp Subtract"},
	{Key::KP_PERIOD             ,"Kp Period"},
	{Key::KP_ADD                ,"Kp Add"},
	{Key::KP_0                  ,"Kp 0"},
	{Key::KP_1                  ,"Kp 1"},
	{Key::KP_2                  ,"Kp 2"},
	{Key::KP_3                  ,"Kp 3"},
	{Key::KP_4                  ,"Kp 4"},
	{Key::KP_5                  ,"Kp 5"},
	{Key::KP_6                  ,"Kp 6"},
	{Key::KP_7                  ,"Kp 7"},
	{Key::KP_8                  ,"Kp 8"},
	{Key::KP_9                  ,"Kp 9"},
	{Key::MENU                  ,"Menu"},
	{Key::HYPER                 ,"Hyper"},
	{Key::HELP                  ,"Help"},
	{Key::BACK                  ,"Back"},
	{Key::FORWARD               ,"Forward"},
	{Key::STOP                  ,"Stop"},
	{Key::REFRESH               ,"Refresh"},
	{Key::VOLUMEDOWN            ,"VolumeDown"},
	{Key::VOLUMEMUTE            ,"VolumeMute"},
	{Key::VOLUMEUP              ,"VolumeUp"},
	{Key::MEDIAPLAY             ,"MediaPlay"},
	{Key::MEDIASTOP             ,"MediaStop"},
	{Key::MEDIAPREVIOUS         ,"MediaPrevious"},
	{Key::MEDIANEXT             ,"MediaNext"},
	{Key::MEDIARECORD           ,"MediaRecord"},
	{Key::HOMEPAGE              ,"HomePage"},
	{Key::FAVORITES             ,"Favorites"},
	{Key::SEARCH                ,"Search"},
	{Key::STANDBY               ,"StandBy"},
	{Key::OPENURL               ,"OpenURL"},
	{Key::LAUNCHMAIL            ,"LaunchMail"},
	{Key::LAUNCHMEDIA           ,"LaunchMedia"},
	{Key::LAUNCH0               ,"Launch0"},
	{Key::LAUNCH1               ,"Launch1"},
	{Key::LAUNCH2               ,"Launch2"},
	{Key::LAUNCH3               ,"Launch3"},
	{Key::LAUNCH4               ,"Launch4"},
	{Key::LAUNCH5               ,"Launch5"},
	{Key::LAUNCH6               ,"Launch6"},
	{Key::LAUNCH7               ,"Launch7"},
	{Key::LAUNCH8               ,"Launch8"},
	{Key::LAUNCH9               ,"Launch9"},
	{Key::LAUNCHA               ,"LaunchA"},
	{Key::LAUNCHB               ,"LaunchB"},
	{Key::LAUNCHC               ,"LaunchC"},
	{Key::LAUNCHD               ,"LaunchD"},
	{Key::LAUNCHE               ,"LaunchE"},
	{Key::LAUNCHF               ,"LaunchF"},
	{Key::GLOBE                 ,"Globe"},
	{Key::KEYBOARD              ,"On-screen keyboard"},
	{Key::JIS_EISU              ,"JIS Eisu"},
	{Key::JIS_KANA              ,"JIS Kana"},
	{Key::UNKNOWN               ,"Unknown"},
	{Key::SPACE                 ,"Space"},
	{Key::EXCLAM                ,"Exclam"},
	{Key::QUOTEDBL              ,"QuoteDbl"},
	{Key::NUMBERSIGN            ,"NumberSign"},
	{Key::DOLLAR                ,"Dollar"},
	{Key::PERCENT               ,"Percent"},
	{Key::AMPERSAND             ,"Ampersand"},
	{Key::APOSTROPHE            ,"Apostrophe"},
	{Key::PARENLEFT             ,"ParenLeft"},
	{Key::PARENRIGHT            ,"ParenRight"},
	{Key::ASTERISK              ,"Asterisk"},
	{Key::PLUS                  ,"Plus"},
	{Key::COMMA                 ,"Comma"},
	{Key::MINUS                 ,"Minus"},
	{Key::PERIOD                ,"Period"},
	{Key::SLASH                 ,"Slash"},
	{Key::KEY_0                 ,"0"},
	{Key::KEY_1                 ,"1"},
	{Key::KEY_2                 ,"2"},
	{Key::KEY_3                 ,"3"},
	{Key::KEY_4                 ,"4"},
	{Key::KEY_5                 ,"5"},
	{Key::KEY_6                 ,"6"},
	{Key::KEY_7                 ,"7"},
	{Key::KEY_8                 ,"8"},
	{Key::KEY_9                 ,"9"},
	{Key::COLON                 ,"Colon"},
	{Key::SEMICOLON             ,"Semicolon"},
	{Key::LESS                  ,"Less"},
	{Key::EQUAL                 ,"Equal"},
	{Key::GREATER               ,"Greater"},
	{Key::QUESTION              ,"Question"},
	{Key::AT                    ,"At"},
	{Key::A                     ,"A"},
	{Key::B                     ,"B"},
	{Key::C                     ,"C"},
	{Key::D                     ,"D"},
	{Key::E                     ,"E"},
	{Key::F                     ,"F"},
	{Key::G                     ,"G"},
	{Key::H                     ,"H"},
	{Key::I                     ,"I"},
	{Key::J                     ,"J"},
	{Key::K                     ,"K"},
	{Key::L                     ,"L"},
	{Key::M                     ,"M"},
	{Key::N                     ,"N"},
	{Key::O                     ,"O"},
	{Key::P                     ,"P"},
	{Key::Q                     ,"Q"},
	{Key::R                     ,"R"},
	{Key::S                     ,"S"},
	{Key::T                     ,"T"},
	{Key::U                     ,"U"},
	{Key::V                     ,"V"},
	{Key::W                     ,"W"},
	{Key::X                     ,"X"},
	{Key::Y                     ,"Y"},
	{Key::Z                     ,"Z"},
	{Key::BRACKETLEFT           ,"BracketLeft"},
	{Key::BACKSLASH             ,"BackSlash"},
	{Key::BRACKETRIGHT          ,"BracketRight"},
	{Key::ASCIICIRCUM           ,"AsciiCircum"},
	{Key::UNDERSCORE            ,"UnderScore"},
	{Key::QUOTELEFT             ,"QuoteLeft"},
	{Key::BRACELEFT             ,"BraceLeft"},
	{Key::BAR                   ,"Bar"},
	{Key::BRACERIGHT            ,"BraceRight"},
	{Key::ASCIITILDE            ,"AsciiTilde"},
	{Key::YEN                   ,"Yen"},
	{Key::SECTION               ,"Section"},
	{Key::NONE                  ,nullptr}
	/* clang-format on */
};

bool keycode_has_unicode(Key p_keycode) {
	switch (p_keycode) {
		case Key::ESCAPE:
		case Key::TAB:
		case Key::BACKTAB:
		case Key::BACKSPACE:
		case Key::ENTER:
		case Key::KP_ENTER:
		case Key::INSERT:
		case Key::KEY_DELETE:
		case Key::PAUSE:
		case Key::PRINT:
		case Key::SYSREQ:
		case Key::CLEAR:
		case Key::HOME:
		case Key::END:
		case Key::LEFT:
		case Key::UP:
		case Key::RIGHT:
		case Key::DOWN:
		case Key::PAGEUP:
		case Key::PAGEDOWN:
		case Key::SHIFT:
		case Key::CTRL:
		case Key::META:
		case Key::ALT:
		case Key::CAPSLOCK:
		case Key::NUMLOCK:
		case Key::SCROLLLOCK:
		case Key::F1:
		case Key::F2:
		case Key::F3:
		case Key::F4:
		case Key::F5:
		case Key::F6:
		case Key::F7:
		case Key::F8:
		case Key::F9:
		case Key::F10:
		case Key::F11:
		case Key::F12:
		case Key::F13:
		case Key::F14:
		case Key::F15:
		case Key::F16:
		case Key::F17:
		case Key::F18:
		case Key::F19:
		case Key::F20:
		case Key::F21:
		case Key::F22:
		case Key::F23:
		case Key::F24:
		case Key::F25:
		case Key::F26:
		case Key::F27:
		case Key::F28:
		case Key::F29:
		case Key::F30:
		case Key::F31:
		case Key::F32:
		case Key::F33:
		case Key::F34:
		case Key::F35:
		case Key::MENU:
		case Key::HYPER:
		case Key::HELP:
		case Key::BACK:
		case Key::FORWARD:
		case Key::STOP:
		case Key::REFRESH:
		case Key::VOLUMEDOWN:
		case Key::VOLUMEMUTE:
		case Key::VOLUMEUP:
		case Key::MEDIAPLAY:
		case Key::MEDIASTOP:
		case Key::MEDIAPREVIOUS:
		case Key::MEDIANEXT:
		case Key::MEDIARECORD:
		case Key::HOMEPAGE:
		case Key::FAVORITES:
		case Key::SEARCH:
		case Key::STANDBY:
		case Key::OPENURL:
		case Key::LAUNCHMAIL:
		case Key::LAUNCHMEDIA:
		case Key::LAUNCH0:
		case Key::LAUNCH1:
		case Key::LAUNCH2:
		case Key::LAUNCH3:
		case Key::LAUNCH4:
		case Key::LAUNCH5:
		case Key::LAUNCH6:
		case Key::LAUNCH7:
		case Key::LAUNCH8:
		case Key::LAUNCH9:
		case Key::LAUNCHA:
		case Key::LAUNCHB:
		case Key::LAUNCHC:
		case Key::LAUNCHD:
		case Key::LAUNCHE:
		case Key::LAUNCHF:
		case Key::GLOBE:
		case Key::KEYBOARD:
		case Key::JIS_EISU:
		case Key::JIS_KANA:
			return false;
		default: {
		}
	}

	return true;
}

String keycode_get_string(Key p_code) {
	Vector<String> keycode_string;
	if ((p_code & KeyModifierMask::CMD_OR_CTRL) != Key::NONE && !OS::prefer_meta_over_ctrl()) {
		keycode_string.push_back(find_keycode_name(Key::CTRL));
	}
	if ((p_code & KeyModifierMask::CTRL) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::CTRL));
	}
	if ((p_code & KeyModifierMask::ALT) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::ALT));
	}
	if ((p_code & KeyModifierMask::SHIFT) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::SHIFT));
	}
	if ((p_code & KeyModifierMask::CMD_OR_CTRL) != Key::NONE && OS::prefer_meta_over_ctrl()) {
		keycode_string.push_back(find_keycode_name(Key::META));
	}
	if ((p_code & KeyModifierMask::META) != Key::NONE) {
		keycode_string.push_back(find_keycode_name(Key::META));
	}

	p_code &= KeyModifierMask::CODE_MASK;
	if ((char32_t)p_code == 0) {
		// The key was just a modifier without any code.
		return String("+").join(keycode_string);
	}

	// The key is a named keycode.
	const _KeyCodeText *kct = &_keycodes[0];
	while (kct->text) {
		if (kct->code == p_code) {
			keycode_string.push_back(kct->text);
			return String("+").join(keycode_string);
		}
		kct++;
	}

	// The key is a single character.
	keycode_string.push_back(String::chr((char32_t)p_code));
	return String("+").join(keycode_string);
}

Key find_keycode(const String &p_codestr) {
	Key keycode = Key::NONE;
	Vector<String> code_parts = p_codestr.split("+");
	if (code_parts.is_empty()) {
		return keycode;
	}

	const String &last_part = code_parts[code_parts.size() - 1];
	const _KeyCodeText *kct = &_keycodes[0];

	while (kct->text) {
		if (last_part.nocasecmp_to(kct->text) == 0) {
			keycode = kct->code;
			break;
		}
		kct++;
	}

	for (int part = 0; part < code_parts.size() - 1; part++) {
		const String &code_part = code_parts[part];
		if (code_part.nocasecmp_to(find_keycode_name(Key::SHIFT)) == 0) {
			keycode |= KeyModifierMask::SHIFT;
		} else if (code_part.nocasecmp_to(find_keycode_name(Key::CTRL)) == 0) {
			keycode |= KeyModifierMask::CTRL;
		} else if (code_part.nocasecmp_to(find_keycode_name(Key::META)) == 0) {
			keycode |= KeyModifierMask::META;
		} else if (code_part.nocasecmp_to(find_keycode_name(Key::ALT)) == 0) {
			keycode |= KeyModifierMask::ALT;
		}
	}

	return keycode;
}

const char *find_keycode_name(Key p_keycode) {
	const _KeyCodeText *kct = &_keycodes[0];

	while (kct->text) {
		if (kct->code == p_keycode) {
			return kct->text;
		}
		kct++;
	}

	return "";
}

int keycode_get_count() {
	const _KeyCodeText *kct = &_keycodes[0];

	int count = 0;
	while (kct->text) {
		count++;
		kct++;
	}
	return count;
}

int keycode_get_value_by_index(int p_index) {
	return (int)_keycodes[p_index].code;
}

const char *keycode_get_name_by_index(int p_index) {
	return _keycodes[p_index].text;
}

char32_t fix_unicode(char32_t p_char) {
	if (p_char >= 0x20 && p_char != 0x7F) {
		return p_char;
	}
	return 0;
}

Key fix_keycode(char32_t p_char, Key p_key) {
	if (p_char >= 0x20 && p_char <= 0x7E) {
		return (Key)String::char_uppercase(p_char);
	}
	return p_key;
}

Key fix_key_label(char32_t p_char, Key p_key) {
	if (p_char >= 0x20 && p_char != 0x7F) {
		return (Key)String::char_uppercase(p_char);
	}
	return p_key;
}

// ============================================================================
// Merged from core/os/main_loop.cpp
// ============================================================================

#include "core/os/os.h"

bool s_quit_requested = false;

void MainLoop::reset_backend()
{
	initialize = &_default_initialize;
	iteration_prepare = &_default_iteration_prepare;
	physics_process = &_default_physics_process;
	iteration_end = &_default_iteration_end;
	process = &_default_process;
	finalize = &_default_finalize;
	notification = &_default_notification;
	destroy = &_default_destroy;
	_active = false;
}

void MainLoop::clear()
{
	void (*destroy_fn)() = destroy;
	reset_backend();
	if (destroy_fn) {
		destroy_fn();
	}
}

// ============================================================================
// Merged from core/os/midi_driver.cpp
// ============================================================================

#include "core/input/input.h"
#include "core/os/os.h"

uint8_t MIDIDriver::last_received_message = 0x00;

void MIDIDriver::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	_initialize_backend();
}

void MIDIDriver::finalize()
{
	if (data == nullptr) {
		return;
	}
	close();
	_finalize_backend();
	memdelete(data);
	data = nullptr;
}

void MIDIDriver::reset_backend()
{
	_initialize_backend = &_default_initialize_backend;
	_finalize_backend = &_default_finalize_backend;
	open = &_default_open;
	close = &_default_close;
	_is_bound = false;
}

MIDIDriver::MessageCategory MIDIDriver::Parser::category(uint8_t p_midi_fragment)
{
	if (p_midi_fragment >= 0xf8) {
		return MessageCategory::RealTime;
	}
	else if (p_midi_fragment >= 0xf0) {
		// System Exclusive begin/end are specified as System Common Category
		// messages, but we separate them here and give them their own categories
		// as their behavior is significantly different.
		if (p_midi_fragment == 0xf0) {
			return MessageCategory::SysExBegin;
		}
		else if (p_midi_fragment == 0xf7) {
			return MessageCategory::SysExEnd;
		}
		return MessageCategory::SystemCommon;
	}
	else if (p_midi_fragment >= 0x80) {
		return MessageCategory::Voice;
	}
	return MessageCategory::Data;
}

MIDIMessage MIDIDriver::Parser::status_to_msg_enum(uint8_t p_status_byte)
{
	if (p_status_byte & 0x80) {
		if (p_status_byte < 0xf0) {
			return MIDIMessage(p_status_byte >> 4);
		}
		else {
			return MIDIMessage(p_status_byte);
		}
	}
	return MIDIMessage::NONE;
}

size_t MIDIDriver::Parser::expected_data(uint8_t p_status_byte)
{
	return expected_data(status_to_msg_enum(p_status_byte));
}

size_t MIDIDriver::Parser::expected_data(MIDIMessage p_msg_type)
{
	switch (p_msg_type) {
	case MIDIMessage::NOTE_OFF:
	case MIDIMessage::NOTE_ON:
	case MIDIMessage::AFTERTOUCH:
	case MIDIMessage::CONTROL_CHANGE:
	case MIDIMessage::PITCH_BEND:
	case MIDIMessage::SONG_POSITION_POINTER:
		return 2;
	case MIDIMessage::PROGRAM_CHANGE:
	case MIDIMessage::CHANNEL_PRESSURE:
	case MIDIMessage::QUARTER_FRAME:
	case MIDIMessage::SONG_SELECT:
		return 1;
	default:
		return 0;
	}
}

uint8_t MIDIDriver::Parser::channel(uint8_t p_status_byte)
{
	if (category(p_status_byte) == MessageCategory::Voice) {
		return p_status_byte & 0x0f;
	}
	return 0;
}

void MIDIDriver::send_event(
	int p_device_index, uint8_t p_status, const uint8_t* p_data, size_t p_data_len)
{
	const MIDIMessage msg = Parser::status_to_msg_enum(p_status);
	ERR_FAIL_COND(p_data_len < Parser::expected_data(msg));

	InputEvent event = InputEvent::make_midi();
	event.device = p_device_index;
	event.midi.channel = Parser::channel(p_status);
	event.midi.message = msg;
	switch (msg) {
	case MIDIMessage::NOTE_OFF:
	case MIDIMessage::NOTE_ON:
		event.midi.pitch = p_data[0];
		event.midi.velocity = p_data[1];
		break;
	case MIDIMessage::AFTERTOUCH:
		event.midi.pitch = p_data[0];
		event.midi.pressure = p_data[1];
		break;
	case MIDIMessage::CONTROL_CHANGE:
		event.midi.controller_number = p_data[0];
		event.midi.controller_value = p_data[1];
		break;
	case MIDIMessage::PROGRAM_CHANGE:
		event.midi.instrument = p_data[0];
		break;
	case MIDIMessage::CHANNEL_PRESSURE:
		event.midi.pressure = p_data[0];
		break;
	case MIDIMessage::PITCH_BEND:
		event.midi.pitch = (p_data[1] << 7) | p_data[0];
		break;
	// QUARTER_FRAME, SONG_POSITION_POINTER, and SONG_SELECT not yet implemented.
	default:
		break;
	}
	Input::parse_input_event(event);
}

void MIDIDriver::Parser::parse_fragment(uint8_t p_fragment)
{
	switch (category(p_fragment)) {
	case MessageCategory::RealTime:
		// Real-Time messages are single byte messages that can
		// occur at any point and do not interrupt other messages.
		// We pass them straight through.
		MIDIDriver::send_event(device_index, p_fragment);
		break;

	case MessageCategory::SysExBegin:
		status_byte = p_fragment;
		skipping_sys_ex = true;
		break;

	case MessageCategory::SysExEnd:
		status_byte = 0;
		skipping_sys_ex = false;
		break;

	case MessageCategory::Voice:
	case MessageCategory::SystemCommon:
		skipping_sys_ex = false; // If we were in SysEx, assume it was aborted.
		received_data_len = 0;
		status_byte = 0;
		ERR_FAIL_COND(expected_data(p_fragment) > DATA_BUFFER_SIZE);
		if (expected_data(p_fragment) == 0) {
			// No data bytes needed, post it now.
			MIDIDriver::send_event(device_index, p_fragment);
		}
		else {
			status_byte = p_fragment;
		}
		break;

	case MessageCategory::Data:
		// We don't currently process SysEx messages, so ignore their data.
		if (!skipping_sys_ex) {
			const size_t expected = expected_data(status_byte);
			if (received_data_len < expected) {
				data_buffer[received_data_len] = p_fragment;
				received_data_len++;
				if (received_data_len == expected) {
					MIDIDriver::send_event(device_index, status_byte, data_buffer, expected);
					received_data_len = 0;
					// Voice messages can use 'running status', sending further
					// messages without resending their status byte.
					// For other messages types we clear the cached status byte.
					if (category(status_byte) != MessageCategory::Voice) {
						status_byte = 0;
					}
				}
			}
		}
		break;
	}
}

void MIDIDriver::add_connected_input(const String& p_name)
{
	if (data != nullptr) {
		data->connected_input_names.push_back(p_name);
	}
}

void MIDIDriver::clear_connected_inputs()
{
	if (data != nullptr) {
		data->connected_input_names.clear();
	}
}

PackedStringArray MIDIDriver::get_connected_inputs()
{
	return data != nullptr ? data->connected_input_names : PackedStringArray();
}



// ============================================================================
// Merged from core/os/mutex.cpp
// ============================================================================

#include "core/os/mutex.h"

void _global_lock() {
	_global_mutex.lock();
}

void _global_unlock() {
	_global_mutex.unlock();
}

#ifdef THREADS_ENABLED

template class MutexImpl<THREADING_NAMESPACE::recursive_mutex>;
template class MutexImpl<THREADING_NAMESPACE::mutex>;
template class MutexLock<MutexImpl<THREADING_NAMESPACE::recursive_mutex>>;
template class MutexLock<MutexImpl<THREADING_NAMESPACE::mutex>>;

#endif

// ============================================================================
// Merged from core/os/thread.cpp
// ============================================================================

#include "core/types.h"
#include "platform_config.h" // IWYU pragma: keep. Can override the implementation.

#ifndef PLATFORM_THREAD_OVERRIDE // See details in thread.h.

#include "core/os/thread.h"

#ifdef THREADS_ENABLED

SafeNumeric<uint64_t> Thread::id_counter(
	1); // The first value after .increment() is 2, hence by default the main thread ID should be 1.
thread_local Thread::ID Thread::caller_id = Thread::id_counter.increment();

#endif

Thread::PlatformFunctions Thread::platform_functions;

void Thread::_set_platform_functions(const PlatformFunctions& p_functions)
{
	platform_functions = p_functions;
}

#ifdef THREADS_ENABLED
void Thread::callback(
	ID p_caller_id, const Settings& p_settings, Callback p_callback, void* p_userdata)
{
	Thread::caller_id = p_caller_id;
	if (platform_functions.set_priority) {
		platform_functions.set_priority(p_settings.priority);
	}
	if (platform_functions.init) {
		platform_functions.init();
	}
	if (platform_functions.wrapper) {
		platform_functions.wrapper(p_callback, p_userdata);
	}
	else {
		p_callback(p_userdata);
	}
	if (platform_functions.term) {
		platform_functions.term();
	}
}

Thread::ID Thread::start(Thread::Callback p_callback, void* p_user, const Settings& p_settings)
{
	ERR_FAIL_COND_V_MSG(id != UNASSIGNED_ID, UNASSIGNED_ID,
		"A Thread object has been re-started without wait_to_finish() having been called on it.");
	id = id_counter.increment();
	thread = THREADING_NAMESPACE::thread(&Thread::callback, id, p_settings, p_callback, p_user);
	return id;
}

bool Thread::is_started() const { return id != UNASSIGNED_ID; }

void Thread::wait_to_finish()
{
	ERR_FAIL_COND_MSG(
		id == UNASSIGNED_ID, "Attempt of waiting to finish on a thread that was never started.");
	ERR_FAIL_COND_MSG(id == get_caller_id(),
		"Threads can't wait to finish on themselves, another thread must wait.");
	thread.join();
	thread = THREADING_NAMESPACE::thread();
	id = UNASSIGNED_ID;
}

void Thread::make_main_thread()
{
	if (caller_id == MAIN_ID) {
		return; // We're already the main thread
	}
	CRASH_COND_MSG(!is_main_thread_assigned.set_if_clear(),
		"A second thread attempted to become the main thread.");
	caller_id = MAIN_ID;
}

void Thread::release_main_thread()
{
	CRASH_COND_MSG(
		caller_id != MAIN_ID, "Trying to release main thread from a thread that isn't main.");
	CRASH_COND(!is_main_thread_assigned.clear_if_set());
	caller_id = id_counter.increment();
}

Error Thread::set_name(const String& p_name)
{
	if (platform_functions.set_name) {
		return platform_functions.set_name(p_name);
	}

	return ERR_UNAVAILABLE;
}

Thread::~Thread()
{
	if (id != UNASSIGNED_ID) {
#ifdef DEBUG_ENABLED
		WARN_PRINT(
			"A Thread object is being destroyed without its completion having been realized.\n"
			"Please call wait_to_finish() on it to ensure correct cleanup.");
#endif
		thread.detach();
	}
}

#endif // THREADS_ENABLED

#endif // PLATFORM_THREAD_OVERRIDE



// ============================================================================
// Merged from core/os/thread_safe.cpp
// ============================================================================

#ifndef THREAD_SAFE_CPP
#define THREAD_SAFE_CPP

#include "core/os/os.h"

static thread_local bool current_thread_safe_for_nodes = false;

bool is_current_thread_safe_for_nodes() {
	return current_thread_safe_for_nodes;
}

void set_current_thread_safe_for_nodes(bool p_safe) {
	current_thread_safe_for_nodes = p_safe;
}

#endif // THREAD_SAFE_CPP

#ifdef THREAD_SAFE_CPP
#undef THREAD_SAFE_CPP
#endif

// ============================================================================
// Merged from core/os/time.cpp
// ============================================================================

#include "core/os/os.h"
#include "core/os/time.h" // NOLINT(modernize-deprecated-headers) False positive with C-Header of the same name.

#define UNIX_EPOCH_YEAR_AD 1970		   // 1970
#define SECONDS_PER_DAY (24 * 60 * 60) // 86400
#define IS_LEAP_YEAR(year) (!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define YEAR_SIZE(year) (IS_LEAP_YEAR(year) ? 366 : 365)

static constexpr int64_t total_leap_days(int64_t p_year)
{
	if (p_year > 0) {
		--p_year;
		return 1 + (p_year / 4 - p_year / 100 + p_year / 400);
	}

	return p_year / 4 - p_year / 100 + p_year / 400;
}

static constexpr int64_t year_to_days(int64_t p_year)
{
	return p_year * 365 + total_leap_days(p_year);
}

static constexpr int64_t days_to_year(int64_t p_days)
{
	int64_t year = 400 * p_days / year_to_days(400);
	if (year < 0) {
		--year;
	}
	if (year_to_days(year) > p_days) {
		--year;
	}
	if (year_to_days(year + 1) <= p_days) {
		++year;
	}
	return year;
}

#define YEAR_KEY "year"
#define MONTH_KEY "month"
#define DAY_KEY "day"
#define WEEKDAY_KEY "weekday"
#define HOUR_KEY "hour"
#define MINUTE_KEY "minute"
#define SECOND_KEY "second"
#define DST_KEY "dst"

// Table of number of days in each month (for regular year and leap year).
static const uint8_t MONTH_DAYS_TABLE[2][12] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

#define UNIX_TIME_TO_HMS                                                                           \
	uint8_t hour, minute, second;                                                                  \
	{                                                                                              \
		/* The time of the day (in seconds since start of day). */                                 \
		uint32_t day_clock = Math::posmod(p_unix_time_val, SECONDS_PER_DAY);                       \
		/* On x86 these 4 lines can be optimized to only 2 divisions. */                           \
		second = day_clock % 60;                                                                   \
		day_clock /= 60;                                                                           \
		minute = day_clock % 60;                                                                   \
		hour = day_clock / 60;                                                                     \
	}

#define UNIX_TIME_TO_YMD                                                                           \
	int64_t year;                                                                                  \
	Month month;                                                                                   \
	uint8_t day;                                                                                   \
	/* The day number since Unix epoch (0-index). Days before 1970 are negative. */                \
	int64_t day_number = Math::floor(p_unix_time_val / (double)SECONDS_PER_DAY);                   \
	{                                                                                              \
		int64_t day_number_copy = day_number;                                                      \
		day_number_copy += year_to_days(UNIX_EPOCH_YEAR_AD);                                       \
		year = days_to_year(day_number_copy);                                                      \
		day_number_copy -= year_to_days(year);                                                     \
		uint8_t month_zero_index = 0;                                                              \
		/* After the above, day_number now represents the day of the year (0-index). */            \
		while (day_number_copy >= MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][month_zero_index]) {        \
			day_number_copy -= MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][month_zero_index];             \
			month_zero_index++;                                                                    \
		}                                                                                          \
		/* After the above, day_number now represents the day of the month (0-index). */           \
		month = (Month)(month_zero_index + 1);                                                     \
		day = day_number_copy + 1;                                                                 \
	}

#define VALIDATE_YMDHMS(ret)                                                                       \
	ERR_FAIL_COND_V_MSG(month == 0, ret,                                                           \
		"Invalid month value of: " + itos(month) +                                                 \
			", months are 1-indexed and cannot be 0. See the Time.Month enum for valid values.");  \
	ERR_FAIL_COND_V_MSG(month < 0, ret, "Invalid month value of: " + itos(month) + ".");           \
	ERR_FAIL_COND_V_MSG(month > 12, ret,                                                           \
		"Invalid month value of: " + itos(month) + ". See the Time.Month enum for valid values."); \
	ERR_FAIL_COND_V_MSG(hour > 23, ret, "Invalid hour value of: " + itos(hour) + ".");             \
	ERR_FAIL_COND_V_MSG(hour < 0, ret, "Invalid hour value of: " + itos(hour) + ".");              \
	ERR_FAIL_COND_V_MSG(minute > 59, ret, "Invalid minute value of: " + itos(minute) + ".");       \
	ERR_FAIL_COND_V_MSG(minute < 0, ret, "Invalid minute value of: " + itos(minute) + ".");        \
	ERR_FAIL_COND_V_MSG(second > 59, ret,                                                          \
		"Invalid second value of: " + itos(second) + " (leap seconds are not supported).");        \
	ERR_FAIL_COND_V_MSG(second < 0, ret, "Invalid second value of: " + itos(second) + ".");        \
	ERR_FAIL_COND_V_MSG(day == 0, ret,                                                             \
		"Invalid day value of: " + itos(day) + ", days are 1-indexed and cannot be 0.");           \
	ERR_FAIL_COND_V_MSG(day < 0, ret, "Invalid day value of: " + itos(day) + ".");                 \
	/* Do this check after month is tested as valid. */                                            \
	uint8_t days_in_this_month = MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][month - 1];                  \
	ERR_FAIL_COND_V_MSG(day > days_in_this_month, ret,                                             \
		"Invalid day value of: " + itos(day) +                                                     \
			" which is larger than the maximum for this month, " + itos(days_in_this_month) +      \
			".");

#define YMD_TO_DAY_NUMBER                                                                          \
	/* The day number since Unix epoch (0-index). Days before 1970 are negative. */                \
	int64_t day_number = day - 1;                                                                  \
	/* Add the days in the months to day_number. */                                                \
	for (int i = 0; i < month - 1; i++) {                                                          \
		day_number += MONTH_DAYS_TABLE[IS_LEAP_YEAR(year)][i];                                     \
	}                                                                                              \
	/* Add the days in the years to day_number. */                                                 \
	day_number += year_to_days(year);                                                              \
	day_number -= year_to_days(UNIX_EPOCH_YEAR_AD);

#define PARSE_ISO8601_STRING(ret)                                                                  \
	int64_t year = UNIX_EPOCH_YEAR_AD;                                                             \
	Month month = MONTH_JANUARY;                                                                   \
	int day = 1;                                                                                   \
	int hour = 0;                                                                                  \
	int minute = 0;                                                                                \
	int second = 0;                                                                                \
	{                                                                                              \
		bool has_date = false, has_time = false;                                                   \
		String date, time;                                                                         \
		if (p_datetime.find_char('T') > 0) {                                                       \
			has_date = has_time = true;                                                            \
			PackedStringArray array = p_datetime.split("T");                                       \
			ERR_FAIL_COND_V_MSG(array.size() < 2, ret, "Invalid ISO 8601 date/time string.");      \
			date = array[0];                                                                       \
			time = array[1];                                                                       \
		} \
		else if (p_datetime.find_char(' ') > 0) {                                                \
			has_date = has_time = true;                                                            \
			PackedStringArray array = p_datetime.split(" ");                                       \
			ERR_FAIL_COND_V_MSG(array.size() < 2, ret, "Invalid ISO 8601 date/time string.");      \
			date = array[0];                                                                       \
			time = array[1];                                                                       \
		} \
		else if (p_datetime.find_char('-', 1) > 0) {                                             \
			has_date = true;                                                                       \
			date = p_datetime;                                                                     \
		} \
		else if (p_datetime.find_char(':') > 0) {                                                \
			has_time = true;                                                                       \
			time = p_datetime;                                                                     \
		}                                                                                          \
		/* Set the variables from the contents of the string. */                                   \
		if (has_date) {                                                                            \
			PackedInt32Array array = date.split_ints("-", false);                                  \
			ERR_FAIL_COND_V_MSG(array.size() < 3, ret, "Invalid ISO 8601 date string.");           \
			year = array[0];                                                                       \
			month = (Month)array[1];                                                               \
			day = array[2];                                                                        \
			/* Handle negative years. */                                                           \
			if (p_datetime.find_char('-') == 0) {                                                  \
				year *= -1;                                                                        \
			}                                                                                      \
		}                                                                                          \
		if (has_time) {                                                                            \
			PackedInt32Array array = time.split_ints(":", false);                                  \
			ERR_FAIL_COND_V_MSG(array.size() < 3, ret, "Invalid ISO 8601 time string.");           \
			hour = array[0];                                                                       \
			minute = array[1];                                                                     \
			second = array[2];                                                                     \
		}                                                                                          \
	}

#define EXTRACT_FROM_DICTIONARY                                                                    \
	/* Get all time values from the dictionary. If it doesn't exist, set the */                    \
	/* values to the default values for Unix epoch (1970-01-01 00:00:00). */                       \
	int64_t year = p_datetime.has(YEAR_KEY) ? int64_t(p_datetime[YEAR_KEY]) : UNIX_EPOCH_YEAR_AD;  \
	Month month = Month((p_datetime.has(MONTH_KEY)) ? int(p_datetime[MONTH_KEY]) : 1);             \
	int day = p_datetime.has(DAY_KEY) ? int(p_datetime[DAY_KEY]) : 1;                              \
	int hour = p_datetime.has(HOUR_KEY) ? int(p_datetime[HOUR_KEY]) : 0;                           \
	int minute = p_datetime.has(MINUTE_KEY) ? int(p_datetime[MINUTE_KEY]) : 0;                     \
	int second = p_datetime.has(SECOND_KEY) ? int(p_datetime[SECOND_KEY]) : 0;

String Time::get_datetime_string_from_unix_time(int64_t p_unix_time_val, bool p_use_space)
{
	UNIX_TIME_TO_HMS
	UNIX_TIME_TO_YMD
	const String format_string =
		p_use_space ? "%04d-%02d-%02d %02d:%02d:%02d" : "%04d-%02d-%02dT%02d:%02d:%02d";
	return vformat(format_string, year, (uint8_t)month, day, hour, minute, second);
}

String Time::get_date_string_from_unix_time(int64_t p_unix_time_val)
{
	UNIX_TIME_TO_YMD
	// Android is picky about the types passed to make Variant, so we need a cast.
	return vformat("%04d-%02d-%02d", year, (uint8_t)month, day);
}

String Time::get_time_string_from_unix_time(int64_t p_unix_time_val)
{
	UNIX_TIME_TO_HMS
	return vformat("%02d:%02d:%02d", hour, minute, second);
}

int64_t Time::get_unix_time_from_datetime_string(const String& p_datetime)
{
	PARSE_ISO8601_STRING(-1)
	VALIDATE_YMDHMS(0)
	YMD_TO_DAY_NUMBER
	return day_number * SECONDS_PER_DAY + hour * 3600 + minute * 60 + second;
}

String Time::get_offset_string_from_offset_minutes(int64_t p_offset_minutes)
{
	String sign;
	if (p_offset_minutes < 0) {
		sign = "-";
		p_offset_minutes = -p_offset_minutes;
	}
	else {
		sign = "+";
	}
	// These two lines can be optimized to one instruction on x86 and others.
	// Note that % is acceptable here only because we ensure it's positive.
	int64_t offset_hours = p_offset_minutes / 60;
	int64_t offset_minutes = p_offset_minutes % 60;
	return vformat("%s%02d:%02d", sign, offset_hours, offset_minutes);
}

String Time::get_datetime_string_from_system(bool p_utc, bool p_use_space)
{
	OS::DateTime dt = OS::get_datetime(p_utc);
	const String format_string =
		p_use_space ? "%04d-%02d-%02d %02d:%02d:%02d" : "%04d-%02d-%02dT%02d:%02d:%02d";
	return vformat(
		format_string, dt.year, (uint8_t)dt.month, dt.day, dt.hour, dt.minute, dt.second);
}

String Time::get_date_string_from_system(bool p_utc)
{
	OS::DateTime dt = OS::get_datetime(p_utc);
	// Android is picky about the types passed to make Variant, so we need a cast.
	return vformat("%04d-%02d-%02d", dt.year, (uint8_t)dt.month, dt.day);
}

String Time::get_time_string_from_system(bool p_utc)
{
	OS::DateTime dt = OS::get_datetime(p_utc);
	return vformat("%02d:%02d:%02d", dt.hour, dt.minute, dt.second);
}

double Time::get_unix_time_from_system() { return OS::get_unix_time(); }

uint64_t Time::get_ticks_msec() { return OS::get_ticks_msec(); }

uint64_t Time::get_ticks_usec() { return OS::get_ticks_usec(); }

#ifdef UNIX_EPOCH_YEAR_AD
#undef UNIX_EPOCH_YEAR_AD
#endif
#ifdef SECONDS_PER_DAY
#undef SECONDS_PER_DAY
#endif
#ifdef IS_LEAP_YEAR
#undef IS_LEAP_YEAR
#endif
#ifdef YEAR_SIZE
#undef YEAR_SIZE
#endif
#ifdef YEAR_KEY
#undef YEAR_KEY
#endif
#ifdef MONTH_KEY
#undef MONTH_KEY
#endif
#ifdef DAY_KEY
#undef DAY_KEY
#endif
#ifdef WEEKDAY_KEY
#undef WEEKDAY_KEY
#endif
#ifdef HOUR_KEY
#undef HOUR_KEY
#endif
#ifdef MINUTE_KEY
#undef MINUTE_KEY
#endif
#ifdef SECOND_KEY
#undef SECOND_KEY
#endif
#ifdef DST_KEY
#undef DST_KEY
#endif
#ifdef UNIX_TIME_TO_HMS
#undef UNIX_TIME_TO_HMS
#endif
#ifdef UNIX_TIME_TO_YMD
#undef UNIX_TIME_TO_YMD
#endif
#ifdef VALIDATE_YMDHMS
#undef VALIDATE_YMDHMS
#endif
#ifdef YMD_TO_DAY_NUMBER
#undef YMD_TO_DAY_NUMBER
#endif
#ifdef PARSE_ISO8601_STRING
#undef PARSE_ISO8601_STRING
#endif
#ifdef EXTRACT_FROM_DICTIONARY
#undef EXTRACT_FROM_DICTIONARY
#endif

// ============================================================================
// Merged from core/os/memory.cpp
// ============================================================================

#include "core/templates/safe_refcount.h"
#include "core/os/memory.h"

#ifdef DEV_ENABLED
#include "core/types.h"
#endif

#include <cstdlib>

#ifdef DEBUG_ENABLED
static SafeNumeric<uint64_t> _current_mem_usage;
static SafeNumeric<uint64_t> _max_mem_usage;
#endif

void* operator new(size_t p_size, DefaultAllocator p_allocator)
{
	return Memory::alloc_static(p_size);
}

void* operator new(size_t p_size, void* (*p_allocfunc)(size_t p_size))
{
	return p_allocfunc(p_size);
}

void* Memory::alloc_aligned_static(size_t p_bytes, size_t p_alignment)
{
	DEV_ASSERT(Math::is_power_of_2(p_alignment));

	void *p1, *p2;
	if ((p1 = (void*)malloc(p_bytes + p_alignment - 1 + sizeof(uint32_t))) == nullptr) {
		return nullptr;
	}
	p2 = (void*)(((uintptr_t)p1 + sizeof(uint32_t) + p_alignment - 1) & ~((p_alignment)-1));
	*((uint32_t*)p2 - 1) = (uint32_t)((uintptr_t)p2 - (uintptr_t)p1);
	return p2;
}

void* Memory::realloc_aligned_static(
	void* p_memory, size_t p_bytes, size_t p_prev_bytes, size_t p_alignment)
{
	if (p_memory == nullptr) {
		return alloc_aligned_static(p_bytes, p_alignment);
	}

	void* ret = alloc_aligned_static(p_bytes, p_alignment);
	if (ret) {
		memcpy(ret, p_memory, p_prev_bytes);
	}
	free_aligned_static(p_memory);
	return ret;
}

void Memory::free_aligned_static(void* p_memory)
{
	uint32_t offset = *((uint32_t*)p_memory - 1);
	void* p = (void*)((uint8_t*)p_memory - offset);
	free(p);
}

template <bool p_ensure_zero> void* Memory::alloc_static(size_t p_bytes, bool p_pad_align)
{
#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	void* mem;
	if constexpr (p_ensure_zero) {
		mem = calloc(1, p_bytes + (prepad ? DATA_OFFSET : 0));
	}
	else {
		mem = malloc(p_bytes + (prepad ? DATA_OFFSET : 0));
	}

	ERR_FAIL_NULL_V(mem, nullptr);

	if (prepad) {
		uint8_t* s8 = (uint8_t*)mem;

		uint64_t* s = (uint64_t*)(s8 + SIZE_OFFSET);
		*s = p_bytes;

#ifdef DEBUG_ENABLED
		uint64_t new_mem_usage = _current_mem_usage.add(p_bytes);
		_max_mem_usage.exchange_if_greater(new_mem_usage);
#endif
		return s8 + DATA_OFFSET;
	}
	else {
		return mem;
	}
}

template void* Memory::alloc_static<true>(size_t p_bytes, bool p_pad_align);
template void* Memory::alloc_static<false>(size_t p_bytes, bool p_pad_align);

void* Memory::realloc_static(void* p_memory, size_t p_bytes, bool p_pad_align)
{
	if (p_memory == nullptr) {
		return alloc_static(p_bytes, p_pad_align);
	}

	uint8_t* mem = (uint8_t*)p_memory;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;
		uint64_t* s = (uint64_t*)(mem + SIZE_OFFSET);

#ifdef DEBUG_ENABLED
		if (p_bytes > *s) {
			uint64_t new_mem_usage = _current_mem_usage.add(p_bytes - *s);
			_max_mem_usage.exchange_if_greater(new_mem_usage);
		}
		else {
			_current_mem_usage.sub(*s - p_bytes);
		}
#endif

		if (p_bytes == 0) {
			free(mem);
			return nullptr;
		}
		else {
			*s = p_bytes;

			mem = (uint8_t*)realloc(mem, p_bytes + DATA_OFFSET);
			ERR_FAIL_NULL_V(mem, nullptr);
			s = (uint64_t*)(mem + SIZE_OFFSET);
			*s = p_bytes;

			return mem + DATA_OFFSET;
		}
	}
	else {
		mem = (uint8_t*)realloc(mem, p_bytes);
		ERR_FAIL_COND_V(mem == nullptr && p_bytes > 0, nullptr);
		return mem;
	}
}

void Memory::free_static(void* p_ptr, bool p_pad_align)
{
	ERR_FAIL_NULL(p_ptr);

	uint8_t* mem = (uint8_t*)p_ptr;

#ifdef DEBUG_ENABLED
	bool prepad = true;
#else
	bool prepad = p_pad_align;
#endif

	if (prepad) {
		mem -= DATA_OFFSET;
#ifdef DEBUG_ENABLED
		uint64_t* s = (uint64_t*)(mem + SIZE_OFFSET);
		_current_mem_usage.sub(*s);
#endif
		free(mem);
	}
	else {
		free(mem);
	}
}

uint64_t Memory::get_mem_available()
{
	return -1; // 0xFFFF...
}

uint64_t Memory::get_mem_usage()
{
#ifdef DEBUG_ENABLED
	return _current_mem_usage.get();
#else
	return 0;
#endif
}

uint64_t Memory::get_mem_max_usage()
{
#ifdef DEBUG_ENABLED
	return _max_mem_usage.get();
#else
	return 0;
#endif
}

_GlobalNil::_GlobalNil()
{
	left = this;
	right = this;
	parent = this;
}

_GlobalNil _GlobalNilClass::_nil;


// --- Merged from core/crypto/crypto.cpp ---
#include "core/crypto/crypto.h"

#include "modules/modules_enabled.gen.h"

Ref<TLSOptions> TLSOptions::client(
	Ref<X509Certificate> p_trusted_chain, const String& p_common_name_override)
{
	Ref<TLSOptions> opts;
	opts.instantiate();
	opts->mode = MODE_CLIENT;
	opts->trusted_ca_chain = p_trusted_chain;
	opts->common_name = p_common_name_override;
	return opts;
}

Ref<TLSOptions> TLSOptions::client_unsafe(Ref<X509Certificate> p_trusted_chain)
{
	Ref<TLSOptions> opts;
	opts.instantiate();
	opts->mode = MODE_CLIENT_UNSAFE;
	opts->trusted_ca_chain = p_trusted_chain;
	return opts;
}

Ref<TLSOptions> TLSOptions::server(Ref<CryptoKey> p_own_key, Ref<X509Certificate> p_own_certificate)
{
	Ref<TLSOptions> opts;
	opts.instantiate();
	opts->mode = MODE_SERVER;
	opts->own_certificate = p_own_certificate;
	opts->private_key = p_own_key;
	return opts;
}

Vector<uint8_t> Crypto::hmac_digest(HashingContext::HashType p_hash_type,
	const Vector<uint8_t>& p_key, const Vector<uint8_t>& p_msg)
{
	Ref<HMACContext> ctx = Ref<HMACContext>(HMACContext::create());
	ERR_FAIL_COND_V_MSG(
		ctx.is_null(), Vector<uint8_t>(), "HMAC is not available without mbedtls module.");
	Error err = ctx->start(p_hash_type, p_key);
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	err = ctx->update(p_msg);
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	return ctx->finish();
}

// Compares two HMACS for equality without leaking timing information in order to prevent timing
// attacks.
// @see:
// https://paragonie.com/blog/2015/11/preventing-timing-attacks-on-string-comparison-with-double-hmac-strategy
bool Crypto::constant_time_compare(
	const Vector<uint8_t>& p_trusted, const Vector<uint8_t>& p_received)
{
	const uint8_t* t = p_trusted.ptr();
	const uint8_t* r = p_received.ptr();
	int tlen = p_trusted.size();
	int rlen = p_received.size();
	// If the lengths are different then nothing else matters.
	if (tlen != rlen) {
		return false;
	}

	uint8_t v = 0;
	for (int i = 0; i < tlen; i++) {
		v |= t[i] ^ r[i];
	}
	return v == 0;
}

#ifndef MODULE_MBEDTLS_ENABLED

CryptoKey* CryptoKey::create(bool p_notify_postinitialize) { return nullptr; }
bool CryptoKey::is_available() { return false; }
Error CryptoKey::load(const String& p_path, bool p_public_only) { return ERR_UNAVAILABLE; }
Error CryptoKey::save(const String& p_path, bool p_public_only) { return ERR_UNAVAILABLE; }
String CryptoKey::save_to_string(bool p_public_only) { return String(); }
Error CryptoKey::load_from_string(const String& p_string_key, bool p_public_only)
{
	return ERR_UNAVAILABLE;
}
bool CryptoKey::is_public_only() const { return true; }
CryptoKey::CryptoKey() {}
CryptoKey::~CryptoKey() {}

X509Certificate* X509Certificate::create(bool p_notify_postinitialize) { return nullptr; }
bool X509Certificate::is_available() { return false; }
Error X509Certificate::load(const String& p_path) { return ERR_UNAVAILABLE; }
Error X509Certificate::load_from_memory(const uint8_t* p_buffer, int p_len)
{
	return ERR_UNAVAILABLE;
}
Error X509Certificate::save(const String& p_path) { return ERR_UNAVAILABLE; }
String X509Certificate::save_to_string() { return String(); }
Error X509Certificate::load_from_string(const String& p_string) { return ERR_UNAVAILABLE; }
X509Certificate::X509Certificate() {}
X509Certificate::~X509Certificate() {}

HMACContext* HMACContext::create(bool p_notify_postinitialize)
{
	ERR_FAIL_V_MSG(nullptr, "HMACContext is not available when the mbedtls module is disabled.");
}
bool HMACContext::is_available() { return false; }
Error HMACContext::start(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_key)
{
	return ERR_UNAVAILABLE;
}
Error HMACContext::update(const Vector<uint8_t>& p_data) { return ERR_UNAVAILABLE; }
Vector<uint8_t> HMACContext::finish() { return Vector<uint8_t>(); }
HMACContext::HMACContext() {}
HMACContext::~HMACContext() {}

Crypto* Crypto::create(bool p_notify_postinitialize)
{
	ERR_FAIL_V_MSG(nullptr, "Crypto is not available when the mbedtls module is disabled.");
}
bool Crypto::is_available() { return false; }
void Crypto::load_default_certificates(const String& p_path) {}
Vector<uint8_t> Crypto::generate_random_bytes(int p_bytes) { return Vector<uint8_t>(); }
Ref<CryptoKey> Crypto::generate_rsa(int p_bytes) { return nullptr; }
Ref<X509Certificate> Crypto::generate_self_signed_certificate(Ref<CryptoKey> p_key,
	const String& p_issuer_name, const String& p_not_before, const String& p_not_after)
{
	return nullptr;
}
Vector<uint8_t> Crypto::sign(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
	Ref<CryptoKey> p_key)
{
	return Vector<uint8_t>();
}
bool Crypto::verify(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
	const Vector<uint8_t>& p_signature, Ref<CryptoKey> p_key)
{
	return false;
}
Vector<uint8_t> Crypto::encrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_plaintext)
{
	return Vector<uint8_t>();
}
Vector<uint8_t> Crypto::decrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_ciphertext)
{
	return Vector<uint8_t>();
}
Crypto::Crypto() {}
Crypto::~Crypto() {}

#endif // !MODULE_MBEDTLS_ENABLED

// ==========================================================================
// crypto_core.cpp
// ==========================================================================

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

#include "core/crypto/crypto.h"

#include "core/os/os.h"
#include "core/string/ustring.h"

#include <mbedtls/base64.h>
#include <psa/crypto.h>

// MD5
CryptoCore::MD5Context::MD5Context() {
	ctx = memalloc_zeroed(sizeof(psa_hash_operation_t));
}

CryptoCore::MD5Context::~MD5Context() {
	psa_hash_abort((psa_hash_operation_t *)ctx);
	memfree((psa_hash_operation_t *)ctx);
}

Error CryptoCore::MD5Context::start() {
	int ret = psa_hash_setup((psa_hash_operation_t *)ctx, PSA_ALG_MD5);
	return ret ? FAILED : OK;
}

Error CryptoCore::MD5Context::update(const uint8_t *p_src, size_t p_len) {
	int ret = psa_hash_update((psa_hash_operation_t *)ctx, p_src, p_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::MD5Context::finish(unsigned char r_hash[16]) {
	size_t size = 0;
	int ret = psa_hash_finish((psa_hash_operation_t *)ctx, r_hash, 16, &size);
	if (ret) {
		psa_hash_abort((psa_hash_operation_t *)ctx);
		return FAILED;
	}
	ERR_FAIL_COND_V(size != 16, ERR_BUG);
	return OK;
}

// SHA1
CryptoCore::SHA1Context::SHA1Context() {
	ctx = memalloc_zeroed(sizeof(psa_hash_operation_t));
}

CryptoCore::SHA1Context::~SHA1Context() {
	psa_hash_abort((psa_hash_operation_t *)ctx);
	memfree((psa_hash_operation_t *)ctx);
}

Error CryptoCore::SHA1Context::start() {
	int ret = psa_hash_setup((psa_hash_operation_t *)ctx, PSA_ALG_SHA_1);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA1Context::update(const uint8_t *p_src, size_t p_len) {
	int ret = psa_hash_update((psa_hash_operation_t *)ctx, p_src, p_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA1Context::finish(unsigned char r_hash[20]) {
	size_t size = 0;
	int ret = psa_hash_finish((psa_hash_operation_t *)ctx, r_hash, 20, &size);
	if (ret) {
		psa_hash_abort((psa_hash_operation_t *)ctx);
		return FAILED;
	}
	ERR_FAIL_COND_V(size != 20, ERR_BUG);
	return OK;
}

// SHA256
CryptoCore::SHA256Context::SHA256Context() {
	ctx = memalloc_zeroed(sizeof(psa_hash_operation_t));
}

CryptoCore::SHA256Context::~SHA256Context() {
	psa_hash_abort((psa_hash_operation_t *)ctx);
	memfree((psa_hash_operation_t *)ctx);
}

Error CryptoCore::SHA256Context::start() {
	int ret = psa_hash_setup((psa_hash_operation_t *)ctx, PSA_ALG_SHA_256);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA256Context::update(const uint8_t *p_src, size_t p_len) {
	int ret = psa_hash_update((psa_hash_operation_t *)ctx, p_src, p_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA256Context::finish(unsigned char r_hash[32]) {
	size_t size = 0;
	int ret = psa_hash_finish((psa_hash_operation_t *)ctx, r_hash, 32, &size);
	if (ret) {
		psa_hash_abort((psa_hash_operation_t *)ctx);
		return FAILED;
	}
	ERR_FAIL_COND_V(size != 32, ERR_BUG);
	return OK;
}

// AES256
CryptoCore::AESContext::AESContext() {
	ctx = memalloc_zeroed(sizeof(psa_cipher_operation_t));
}

CryptoCore::AESContext::~AESContext() {
	psa_cipher_abort((psa_cipher_operation_t *)ctx);
	psa_destroy_key(key_id);
	memfree((psa_cipher_operation_t *)ctx);
}

void CryptoCore::AESContext::reset() {
	psa_cipher_abort((psa_cipher_operation_t *)ctx);
	psa_destroy_key(key_id);
	key_id = PSA_KEY_ID_NULL;
	alg = PSA_ALG_NONE;
}

Error CryptoCore::AESContext::setup(Mode p_mode, Cipher p_cipher, const uint8_t *p_key, size_t p_key_length, const uint8_t *p_iv, size_t p_iv_size) {
	reset();
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	switch (p_mode) {
		case Mode::NONE:
			return OK;
		case Mode::ENCRYPT:
		case Mode::DECRYPT:
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}
	switch (p_cipher) {
		case Cipher::CBC:
			alg = PSA_ALG_CBC_NO_PADDING;
			break;
		case Cipher::ECB:
			alg = PSA_ALG_ECB_NO_PADDING;
			break;
		case Cipher::CFB:
			alg = PSA_ALG_CFB;
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, p_key_length << 3);
	int ret = psa_import_key(&attr, p_key, p_key_length, &key_id);
	if (ret) {
		return FAILED;
	}
	if (p_mode == Mode::ENCRYPT) {
		ret = psa_cipher_encrypt_setup((psa_cipher_operation_t *)ctx, key_id, alg);
	} else {
		ret = psa_cipher_decrypt_setup((psa_cipher_operation_t *)ctx, key_id, alg);
	}
	if (ret == PSA_SUCCESS && p_iv_size) {
		ret = psa_cipher_set_iv((psa_cipher_operation_t *)ctx, p_iv, p_iv_size);
	}
	if (ret) {
		reset();
		return FAILED;
	}
	return OK;
}

Error CryptoCore::AESContext::update(const uint8_t *p_src, size_t p_src_size, uint8_t *r_dst, size_t p_dst_size) {
	size_t size = 0;
	int ret = psa_cipher_update((psa_cipher_operation_t *)ctx, p_src, p_src_size, r_dst, p_dst_size, &size);
	if (ret) {
		return FAILED;
	}
	ERR_FAIL_COND_V(p_dst_size != size, ERR_BUG);
	return OK;
}

Error CryptoCore::AESContext::finish(uint8_t *r_dst, size_t p_dst_size) {
	size_t size = 0;
	int ret = psa_cipher_finish((psa_cipher_operation_t *)ctx, r_dst, p_dst_size, &size);
	reset();
	if (ret) {
		return FAILED;
	}
	ERR_FAIL_COND_V(p_dst_size != size, ERR_BUG);
	return OK;
}

// CryptoCore
Error CryptoCore::generate_random(uint8_t *r_buffer, size_t p_buffer_len) {
	if (unlikely(!initialized)) {
		// Fall back to raw get_entropy (might affect performance).
		return OS::get_entropy(r_buffer, p_buffer_len);
	}
	int ret = psa_generate_random(r_buffer, p_buffer_len);
	ERR_FAIL_COND_V(ret, FAILED);
	return OK;
}

String CryptoCore::b64_encode_str(const uint8_t *p_src, size_t p_src_len) {
	size_t b64len = p_src_len / 3 * 4 + 4 + 1;
	Vector<uint8_t> b64buff;
	b64buff.resize(b64len);
	uint8_t *w64 = b64buff.ptrw();
	size_t strlen = 0;
	int ret = b64_encode(&w64[0], b64len, &strlen, p_src, p_src_len);
	w64[strlen] = 0;
	return ret ? String() : (const char *)&w64[0];
}

Error CryptoCore::b64_encode(uint8_t *r_dst, size_t p_dst_len, size_t *r_len, const uint8_t *p_src, size_t p_src_len) {
	int ret = mbedtls_base64_encode(r_dst, p_dst_len, r_len, p_src, p_src_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::b64_decode(uint8_t *r_dst, size_t p_dst_len, size_t *r_len, const uint8_t *p_src, size_t p_src_len) {
	int ret = mbedtls_base64_decode(r_dst, p_dst_len, r_len, p_src, p_src_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::md5(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[16]) {
	ERR_FAIL_COND_V(PSA_HASH_LENGTH(PSA_ALG_MD5) != 16, ERR_BUG);
	size_t size = 0;
	int ret = psa_hash_compute(PSA_ALG_MD5, p_src, p_src_len, r_hash, 16, &size);
	return ret ? FAILED : OK;
}

Error CryptoCore::sha1(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[20]) {
	ERR_FAIL_COND_V(PSA_HASH_LENGTH(PSA_ALG_SHA_1) != 20, ERR_BUG);
	size_t size = 0;
	int ret = psa_hash_compute(PSA_ALG_SHA_1, p_src, p_src_len, r_hash, 20, &size);
	return ret ? FAILED : OK;
}

Error CryptoCore::sha256(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[32]) {
	ERR_FAIL_COND_V(PSA_HASH_LENGTH(PSA_ALG_SHA_256) != 32, ERR_BUG);
	size_t size = 0;
	int ret = psa_hash_compute(PSA_ALG_SHA_256, p_src, p_src_len, r_hash, 32, &size);
	return ret ? FAILED : OK;
}

Error CryptoCore::encrypt_cfb(const uint8_t *p_src, uint8_t *r_dst, size_t p_length, const uint8_t *p_key, size_t p_key_length, const uint8_t p_iv[16]) {
	AESContext ctx;
	Error err = ctx.setup(AESContext::Mode::ENCRYPT, AESContext::Cipher::CFB, p_key, p_key_length, p_iv, 16);
	ERR_FAIL_COND_V(err, err);
	err = ctx.update(p_src, p_length, r_dst, p_length);
	ERR_FAIL_COND_V(err, err);
	err = ctx.finish(nullptr, 0);
	ERR_FAIL_COND_V(err, err);
	return OK;
}

Error CryptoCore::decrypt_cfb(const uint8_t *p_src, uint8_t *r_dst, size_t p_length, const uint8_t *p_key, size_t p_key_length, const uint8_t p_iv[16]) {
	AESContext ctx;
	Error err = ctx.setup(AESContext::Mode::DECRYPT, AESContext::Cipher::CFB, p_key, p_key_length, p_iv, 16);
	ERR_FAIL_COND_V(err, err);
	err = ctx.update(p_src, p_length, r_dst, p_length);
	ERR_FAIL_COND_V(err, err);
	err = ctx.finish(nullptr, 0);
	ERR_FAIL_COND_V(err, err);
	return OK;
}

bool CryptoCore::initialized = false;
void CryptoCore::initialize() {
	ERR_FAIL_COND(initialized);
#ifdef GODOT_MBEDTLS_PLATFORM
	godot_mbedtls_platform_init();
#endif

	int status = psa_crypto_init();
	ERR_FAIL_COND_MSG(status != PSA_SUCCESS, "Failed to initialize psa crypto. Cryptographic functions will not work.");
	initialized = true;
}
void CryptoCore::finalize() {
	if (!initialized) {
		return;
	}
	initialized = false;
	mbedtls_psa_crypto_free(); // Not part of PSA for reasons.
#ifdef GODOT_MBEDTLS_PLATFORM
	godot_mbedtls_platform_free();
#endif
}

#ifndef VOLTAIRE_BUILD
extern "C" {
psa_status_t psa_crypto_init(void) { return 0; }
psa_status_t psa_hash_abort(psa_hash_operation_t *operation) { return 0; }
psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg) { return 0; }
psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *input, size_t input_length) { return 0; }
psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *hash, size_t hash_size, size_t *hash_length) { return 0; }
psa_status_t psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length, uint8_t *hash, size_t hash_size, size_t *hash_length) { return 0; }
psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation) { return 0; }
psa_status_t psa_destroy_key(psa_key_id_t key) { return 0; }
psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data, size_t data_length, psa_key_id_t *key) { return 0; }
psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key, psa_algorithm_t alg) { return 0; }
psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key, psa_algorithm_t alg) { return 0; }
psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv, size_t iv_length) { return 0; }
psa_status_t psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) { return 0; }
psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output, size_t output_size, size_t *output_length) { return 0; }
psa_status_t psa_generate_random(uint8_t *output, size_t output_size) { return 0; }
void mbedtls_psa_crypto_free(void) {}
int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen) { return 0; }
int mbedtls_base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen) { return 0; }
}
#endif

// ==========================================================================
// hashing_context.cpp
// ==========================================================================

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

#include "core/crypto/crypto.h"

Error HashingContext::start(HashType p_type)
{
	ERR_FAIL_COND_V(ctx != nullptr, ERR_ALREADY_IN_USE);
	_create_ctx(p_type);
	ERR_FAIL_NULL_V(ctx, ERR_UNAVAILABLE);
	switch (type) {
	case HASH_MD5:
		return ((CryptoCore::MD5Context*)ctx)->start();
	case HASH_SHA1:
		return ((CryptoCore::SHA1Context*)ctx)->start();
	case HASH_SHA256:
		return ((CryptoCore::SHA256Context*)ctx)->start();
	}
	return ERR_UNAVAILABLE;
}

Error HashingContext::update(const Vector<uint8_t>& p_chunk)
{
	ERR_FAIL_NULL_V(ctx, ERR_UNCONFIGURED);
	size_t len = p_chunk.size();
	ERR_FAIL_COND_V(len == 0, FAILED);
	const uint8_t* r = p_chunk.ptr();
	switch (type) {
	case HASH_MD5:
		return ((CryptoCore::MD5Context*)ctx)->update(&r[0], len);
	case HASH_SHA1:
		return ((CryptoCore::SHA1Context*)ctx)->update(&r[0], len);
	case HASH_SHA256:
		return ((CryptoCore::SHA256Context*)ctx)->update(&r[0], len);
	}
	return ERR_UNAVAILABLE;
}

Vector<uint8_t> HashingContext::finish()
{
	ERR_FAIL_NULL_V(ctx, Vector<uint8_t>());
	Vector<uint8_t> out;
	Error err = FAILED;
	switch (type) {
	case HASH_MD5:
		out.resize(16);
		err = ((CryptoCore::MD5Context*)ctx)->finish(out.ptrw());
		break;
	case HASH_SHA1:
		out.resize(20);
		err = ((CryptoCore::SHA1Context*)ctx)->finish(out.ptrw());
		break;
	case HASH_SHA256:
		out.resize(32);
		err = ((CryptoCore::SHA256Context*)ctx)->finish(out.ptrw());
		break;
	}
	_delete_ctx();
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	return out;
}

void HashingContext::_create_ctx(HashType p_type)
{
	type = p_type;
	switch (type) {
	case HASH_MD5:
		ctx = memnew(CryptoCore::MD5Context);
		break;
	case HASH_SHA1:
		ctx = memnew(CryptoCore::SHA1Context);
		break;
	case HASH_SHA256:
		ctx = memnew(CryptoCore::SHA256Context);
		break;
	default:
		ctx = nullptr;
	}
}

void HashingContext::_delete_ctx()
{
	switch (type) {
	case HASH_MD5:
		memdelete((CryptoCore::MD5Context*)ctx);
		break;
	case HASH_SHA1:
		memdelete((CryptoCore::SHA1Context*)ctx);
		break;
	case HASH_SHA256:
		memdelete((CryptoCore::SHA256Context*)ctx);
		break;
	}
	ctx = nullptr;
}

HashingContext::~HashingContext()
{
	if (ctx != nullptr) {
		_delete_ctx();
	}
}

// ==========================================================================
// aes_context.cpp
// ==========================================================================

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

#include "core/crypto/crypto.h"

using AESMode = CryptoCore::AESContext::Mode;
using AESCipher = CryptoCore::AESContext::Cipher;

Error AESContext::start(Mode p_mode, const Vector<uint8_t>& p_key, const Vector<uint8_t>& p_iv)
{
	ERR_FAIL_COND_V_MSG(mode != MODE_MAX, ERR_ALREADY_IN_USE,
		"AESContext already started. Call 'finish' before starting a new one.");
	ERR_FAIL_COND_V_MSG(
		p_mode < 0 || p_mode >= MODE_MAX, ERR_INVALID_PARAMETER, "Invalid mode requested.");
	// Key check.
	int key_bits = p_key.size() << 3;
	ERR_FAIL_COND_V_MSG(key_bits != 128 && key_bits != 256, ERR_INVALID_PARAMETER,
		"AES key must be either 16 or 32 bytes");

	AESMode ctx_mode = AESMode::NONE;
	AESCipher ctx_cipher = AESCipher::NONE;
	switch (p_mode) {
	case MODE_ECB_ENCRYPT:
		ctx_mode = AESMode::ENCRYPT;
		ctx_cipher = AESCipher::ECB;
		break;
	case MODE_ECB_DECRYPT:
		ctx_mode = AESMode::DECRYPT;
		ctx_cipher = AESCipher::ECB;
		break;
	case MODE_CBC_ENCRYPT:
		ctx_mode = AESMode::ENCRYPT;
		ctx_cipher = AESCipher::CBC;
		break;
	case MODE_CBC_DECRYPT:
		ctx_mode = AESMode::DECRYPT;
		ctx_cipher = AESCipher::CBC;
		break;
	default:
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	}

	// Initialization vector.
	Vector<uint8_t> iv = p_iv;
	ERR_FAIL_COND_V_MSG(ctx_cipher == AESCipher::CBC && iv.size() != 16, ERR_INVALID_PARAMETER,
		"The initialization vector (IV) must be exactly 16 bytes.");
	if (unlikely(ctx_cipher == AESCipher::ECB && iv.size())) {
		WARN_PRINT("ECB mode does not require an initialization vector (IV). Pass an empty IV "
				   "argument when using ECB.");
		iv.clear();
	}
	Error err = ctx.setup(
		ctx_mode, ctx_cipher, p_key.ptr(), p_key.size(), iv.size() ? iv.ptr() : nullptr, iv.size());
	if (err != OK) {
		return err;
	}
	mode = p_mode;
	return OK;
}

Vector<uint8_t> AESContext::update(const Vector<uint8_t>& p_src)

{
	ERR_FAIL_COND_V_MSG(mode < 0 || mode >= MODE_MAX, Vector<uint8_t>(),
		"AESContext not started. Call 'start' before calling 'update'.");
	int len = p_src.size();
	ERR_FAIL_COND_V_MSG(len % 16, Vector<uint8_t>(),
		"The number of bytes to be encrypted must be multiple of 16. Add padding if needed");
	Vector<uint8_t> out;
	out.resize(len);
	Error err = ctx.update(p_src.ptr(), len, out.ptrw(), len);
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	return out;
}

Vector<uint8_t> AESContext::get_iv_state()
{
	ERR_FAIL_V_MSG(Vector<uint8_t>(), "Calling 'get_iv_state' is no longer supported.");
}

void AESContext::finish()
{
	ctx.finish(nullptr, 0);
	mode = MODE_MAX;
}

// ==========================================================================
// crypto_resource_format.cpp
// ==========================================================================

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

#include "core/crypto/crypto.h"

Ref<Resource> ResourceFormatLoaderCrypto::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	String el = p_path.get_extension().to_lower();
	if (el == "crt") {
		X509Certificate* cert = X509Certificate::create();
		if (cert) {
			cert->load(p_path);
		}
		return cert;
	}
	else if (el == "key") {
		CryptoKey* key = CryptoKey::create();
		if (key) {
			key->load(p_path, false);
		}
		return key;
	}
	else if (el == "pub") {
		CryptoKey* key = CryptoKey::create();
		if (key) {
			key->load(p_path, true);
		}
		return key;
	}
	return nullptr;
}

void ResourceFormatLoaderCrypto::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("crt");
	p_extensions->push_back("key");
	p_extensions->push_back("pub");
}

bool ResourceFormatLoaderCrypto::handles_type(const String& p_type) const
{
	return p_type == "X509Certificate" || p_type == "CryptoKey";
}

String ResourceFormatLoaderCrypto::get_resource_type(const String& p_path) const
{
	String el = p_path.get_extension().to_lower();
	if (el == "crt") {
		return "X509Certificate";
	}
	else if (el == "key" || el == "pub") {
		return "CryptoKey";
	}
	return "";
}

Error ResourceFormatSaverCrypto::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Error err;
	Ref<X509Certificate> cert = p_resource;
	Ref<CryptoKey> key = p_resource;
	if (cert.is_valid()) {
		err = cert->save(p_path);
	}
	else if (key.is_valid()) {
		err = key->save(p_path, p_path.has_extension("pub"));
	}
	else {
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	}
	ERR_FAIL_COND_V_MSG(
		err != OK, err, vformat("Cannot save Crypto resource to file '%s'.", p_path));
	return OK;
}

bool ResourceFormatSaverCrypto::recognize(const Ref<Resource>& p_resource) const { return false; }

void ResourceFormatSaverCrypto::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
}

#include "core/version_hash.gen.cpp"

#include "core/script_encryption_key.gen.cpp"

#include "core/input/default_controller_mappings.gen.cpp"

// === Merged from core/math/geometry_3d.cpp ===
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

#include "core/math/geometry_3d.h"

#include "core/templates/hash_map.h"

void Geometry3D::get_closest_points_between_segments(const Vector3 &p_p0, const Vector3 &p_p1, const Vector3 &p_q0, const Vector3 &p_q1, Vector3 &r_ps, Vector3 &r_qt) {
	// Based on David Eberly's Computation of Distance Between Line Segments algorithm.

	Vector3 p = p_p1 - p_p0;
	Vector3 q = p_q1 - p_q0;
	Vector3 r = p_p0 - p_q0;

	real_t a = p.dot(p);
	real_t b = p.dot(q);
	real_t c = q.dot(q);
	real_t d = p.dot(r);
	real_t e = q.dot(r);

	real_t s = 0.0f;
	real_t t = 0.0f;

	real_t det = a * c - b * b;
	if (det > CMP_EPSILON) {
		// Non-parallel segments
		real_t bte = b * e;
		real_t ctd = c * d;

		if (bte <= ctd) {
			// s <= 0.0f
			if (e <= 0.0f) {
				// t <= 0.0f
				s = (-d >= a ? 1 : (-d > 0.0f ? -d / a : 0.0f));
				t = 0.0f;
			} else if (e < c) {
				// 0.0f < t < 1
				s = 0.0f;
				t = e / c;
			} else {
				// t >= 1
				s = (b - d >= a ? 1 : (b - d > 0.0f ? (b - d) / a : 0.0f));
				t = 1;
			}
		} else {
			// s > 0.0f
			s = bte - ctd;
			if (s >= det) {
				// s >= 1
				if (b + e <= 0.0f) {
					// t <= 0.0f
					s = (-d <= 0.0f ? 0.0f : (-d < a ? -d / a : 1));
					t = 0.0f;
				} else if (b + e < c) {
					// 0.0f < t < 1
					s = 1;
					t = (b + e) / c;
				} else {
					// t >= 1
					s = (b - d <= 0.0f ? 0.0f : (b - d < a ? (b - d) / a : 1));
					t = 1;
				}
			} else {
				// 0.0f < s < 1
				real_t ate = a * e;
				real_t btd = b * d;

				if (ate <= btd) {
					// t <= 0.0f
					s = (-d <= 0.0f ? 0.0f : (-d >= a ? 1 : -d / a));
					t = 0.0f;
				} else {
					// t > 0.0f
					t = ate - btd;
					if (t >= det) {
						// t >= 1
						s = (b - d <= 0.0f ? 0.0f : (b - d >= a ? 1 : (b - d) / a));
						t = 1;
					} else {
						// 0.0f < t < 1
						s /= det;
						t /= det;
					}
				}
			}
		}
	} else {
		// Parallel segments
		if (e <= 0.0f) {
			s = (-d <= 0.0f ? 0.0f : (-d >= a ? 1 : -d / a));
			t = 0.0f;
		} else if (e >= c) {
			s = (b - d <= 0.0f ? 0.0f : (b - d >= a ? 1 : (b - d) / a));
			t = 1;
		} else {
			s = 0.0f;
			t = e / c;
		}
	}

	r_ps = (1 - s) * p_p0 + s * p_p1;
	r_qt = (1 - t) * p_q0 + t * p_q1;
}

real_t Geometry3D::get_closest_distance_between_segments(const Vector3 &p_p0, const Vector3 &p_p1, const Vector3 &p_q0, const Vector3 &p_q1) {
	Vector3 ps;
	Vector3 qt;
	get_closest_points_between_segments(p_p0, p_p1, p_q0, p_q1, ps, qt);
	Vector3 st = qt - ps;
	return st.length();
}

void Geometry3D::MeshData::optimize_vertices() {
	HashMap<int, int> vtx_remap;

	for (MeshData::Face &face : faces) {
		for (int &index : face.indices) {
			if (!vtx_remap.has(index)) {
				int ni = vtx_remap.size();
				vtx_remap[index] = ni;
			}
			index = vtx_remap[index];
		}
	}

	for (MeshData::Edge &edge : edges) {
		int a = edge.vertex_a;
		int b = edge.vertex_b;

		if (!vtx_remap.has(a)) {
			int ni = vtx_remap.size();
			vtx_remap[a] = ni;
		}
		if (!vtx_remap.has(b)) {
			int ni = vtx_remap.size();
			vtx_remap[b] = ni;
		}

		edge.vertex_a = vtx_remap[a];
		edge.vertex_b = vtx_remap[b];
	}

	LocalVector<Vector3> new_vertices;
	new_vertices.resize(vtx_remap.size());

	for (uint32_t i = 0; i < vertices.size(); i++) {
		if (vtx_remap.has(i)) {
			new_vertices[vtx_remap[i]] = vertices[i];
		}
	}
	vertices = new_vertices;
}

struct _FaceClassify {
	struct _Link {
		int face = -1;
		int edge = -1;
		void clear() {
			face = -1;
			edge = -1;
		}
	};
	bool valid = false;
	int group = -1;
	_Link links[3];
	Face3 face;
};

/*** GEOMETRY WRAPPER ***/

enum _CellFlags {
	_CELL_SOLID = 1,
	_CELL_EXTERIOR = 2,
	_CELL_STEP_MASK = 0x1C,
	_CELL_STEP_NONE = 0 << 2,
	_CELL_STEP_Y_POS = 1 << 2,
	_CELL_STEP_Y_NEG = 2 << 2,
	_CELL_STEP_X_POS = 3 << 2,
	_CELL_STEP_X_NEG = 4 << 2,
	_CELL_STEP_Z_POS = 5 << 2,
	_CELL_STEP_Z_NEG = 6 << 2,
	_CELL_STEP_DONE = 7 << 2,
	_CELL_PREV_MASK = 0xE0,
	_CELL_PREV_NONE = 0 << 5,
	_CELL_PREV_Y_POS = 1 << 5,
	_CELL_PREV_Y_NEG = 2 << 5,
	_CELL_PREV_X_POS = 3 << 5,
	_CELL_PREV_X_NEG = 4 << 5,
	_CELL_PREV_Z_POS = 5 << 5,
	_CELL_PREV_Z_NEG = 6 << 5,
	_CELL_PREV_FIRST = 7 << 5,
};

static inline void _plot_face(uint8_t ***p_cell_status, int p_x, int p_y, int p_z, int p_len_x, int p_len_y, int p_len_z, const Vector3 &p_voxelsize, const Face3 &p_face) {
	AABB aabb(Vector3(p_x, p_y, p_z), Vector3(p_len_x, p_len_y, p_len_z));
	aabb.position = aabb.position * p_voxelsize;
	aabb.size = aabb.size * p_voxelsize;

	if (!p_face.intersects_aabb(aabb)) {
		return;
	}

	if (p_len_x == 1 && p_len_y == 1 && p_len_z == 1) {
		p_cell_status[p_x][p_y][p_z] = _CELL_SOLID;
		return;
	}

	int div_x = p_len_x > 1 ? 2 : 1;
	int div_y = p_len_y > 1 ? 2 : 1;
	int div_z = p_len_z > 1 ? 2 : 1;

#define SPLIT_DIV(m_i, m_div, m_v, m_len_v, m_new_v, m_new_len_v) \
	if (m_div == 1) { \
		m_new_v = m_v; \
		m_new_len_v = 1; \
	} else if (m_i == 0) { \
		m_new_v = m_v; \
		m_new_len_v = m_len_v / 2; \
	} else { \
		m_new_v = m_v + m_len_v / 2; \
		m_new_len_v = m_len_v - m_len_v / 2; \
	}

	int new_x;
	int new_len_x;
	int new_y;
	int new_len_y;
	int new_z;
	int new_len_z;

	for (int i = 0; i < div_x; i++) {
		SPLIT_DIV(i, div_x, p_x, p_len_x, new_x, new_len_x);

		for (int j = 0; j < div_y; j++) {
			SPLIT_DIV(j, div_y, p_y, p_len_y, new_y, new_len_y);

			for (int k = 0; k < div_z; k++) {
				SPLIT_DIV(k, div_z, p_z, p_len_z, new_z, new_len_z);

				_plot_face(p_cell_status, new_x, new_y, new_z, new_len_x, new_len_y, new_len_z, p_voxelsize, p_face);
			}
		}
	}

#undef SPLIT_DIV
}

static inline void _mark_outside(uint8_t ***p_cell_status, int p_x, int p_y, int p_z, int p_len_x, int p_len_y, int p_len_z) {
	if (p_cell_status[p_x][p_y][p_z] & 3) {
		return; // Nothing to do, already used and/or visited.
	}

	p_cell_status[p_x][p_y][p_z] = _CELL_PREV_FIRST;

	while (true) {
		uint8_t &c = p_cell_status[p_x][p_y][p_z];

		if ((c & _CELL_STEP_MASK) == _CELL_STEP_NONE) {
			// Haven't been in here, mark as outside.
			p_cell_status[p_x][p_y][p_z] |= _CELL_EXTERIOR;
		}

		if ((c & _CELL_STEP_MASK) != _CELL_STEP_DONE) {
			// If not done, increase step.
			c += 1 << 2;
		}

		if ((c & _CELL_STEP_MASK) == _CELL_STEP_DONE) {
			// Go back.
			switch (c & _CELL_PREV_MASK) {
				case _CELL_PREV_FIRST: {
					return;
				} break;
				case _CELL_PREV_Y_POS: {
					p_y++;
					ERR_FAIL_COND(p_y >= p_len_y);
				} break;
				case _CELL_PREV_Y_NEG: {
					p_y--;
					ERR_FAIL_COND(p_y < 0);
				} break;
				case _CELL_PREV_X_POS: {
					p_x++;
					ERR_FAIL_COND(p_x >= p_len_x);
				} break;
				case _CELL_PREV_X_NEG: {
					p_x--;
					ERR_FAIL_COND(p_x < 0);
				} break;
				case _CELL_PREV_Z_POS: {
					p_z++;
					ERR_FAIL_COND(p_z >= p_len_z);
				} break;
				case _CELL_PREV_Z_NEG: {
					p_z--;
					ERR_FAIL_COND(p_z < 0);
				} break;
				default: {
					ERR_FAIL();
				}
			}
			continue;
		}

		int next_x = p_x, next_y = p_y, next_z = p_z;
		uint8_t prev = 0;

		switch (c & _CELL_STEP_MASK) {
			case _CELL_STEP_Y_POS: {
				next_y++;
				prev = _CELL_PREV_Y_NEG;
			} break;
			case _CELL_STEP_Y_NEG: {
				next_y--;
				prev = _CELL_PREV_Y_POS;
			} break;
			case _CELL_STEP_X_POS: {
				next_x++;
				prev = _CELL_PREV_X_NEG;
			} break;
			case _CELL_STEP_X_NEG: {
				next_x--;
				prev = _CELL_PREV_X_POS;
			} break;
			case _CELL_STEP_Z_POS: {
				next_z++;
				prev = _CELL_PREV_Z_NEG;
			} break;
			case _CELL_STEP_Z_NEG: {
				next_z--;
				prev = _CELL_PREV_Z_POS;
			} break;
			default:
				ERR_FAIL();
		}

		if (next_x < 0 || next_x >= p_len_x) {
			continue;
		}
		if (next_y < 0 || next_y >= p_len_y) {
			continue;
		}
		if (next_z < 0 || next_z >= p_len_z) {
			continue;
		}

		if (p_cell_status[next_x][next_y][next_z] & 3) {
			continue;
		}

		p_x = next_x;
		p_y = next_y;
		p_z = next_z;
		p_cell_status[p_x][p_y][p_z] |= prev;
	}
}

static inline void _build_faces(uint8_t ***p_cell_status, int p_x, int p_y, int p_z, int p_len_x, int p_len_y, int p_len_z, Vector<Face3> &r_faces) {
	ERR_FAIL_INDEX(p_x, p_len_x);
	ERR_FAIL_INDEX(p_y, p_len_y);
	ERR_FAIL_INDEX(p_z, p_len_z);

	if (p_cell_status[p_x][p_y][p_z] & _CELL_EXTERIOR) {
		return;
	}

#define vert(m_idx) Vector3(((m_idx) & 4) >> 2, ((m_idx) & 2) >> 1, (m_idx) & 1)

	static const uint8_t indices[6][4] = {
		{ 7, 6, 4, 5 },
		{ 7, 3, 2, 6 },
		{ 7, 5, 1, 3 },
		{ 0, 2, 3, 1 },
		{ 0, 1, 5, 4 },
		{ 0, 4, 6, 2 },

	};

	for (int i = 0; i < 6; i++) {
		Vector3 face_points[4];
		int disp_x = p_x + ((i % 3) == 0 ? ((i < 3) ? 1 : -1) : 0);
		int disp_y = p_y + (((i - 1) % 3) == 0 ? ((i < 3) ? 1 : -1) : 0);
		int disp_z = p_z + (((i - 2) % 3) == 0 ? ((i < 3) ? 1 : -1) : 0);

		bool plot = false;

		if (disp_x < 0 || disp_x >= p_len_x) {
			plot = true;
		}
		if (disp_y < 0 || disp_y >= p_len_y) {
			plot = true;
		}
		if (disp_z < 0 || disp_z >= p_len_z) {
			plot = true;
		}

		if (!plot && (p_cell_status[disp_x][disp_y][disp_z] & _CELL_EXTERIOR)) {
			plot = true;
		}

		if (!plot) {
			continue;
		}

		for (int j = 0; j < 4; j++) {
			face_points[j] = vert(indices[i][j]) + Vector3(p_x, p_y, p_z);
		}

		r_faces.push_back(
				Face3(
						face_points[0],
						face_points[1],
						face_points[2]));

		r_faces.push_back(
				Face3(
						face_points[2],
						face_points[3],
						face_points[0]));
	}
}

Vector<Face3> Geometry3D::wrap_geometry(const Vector<Face3> &p_array, real_t *r_error) {
	int face_count = p_array.size();
	const Face3 *faces = p_array.ptr();
	constexpr double min_size = 1.0;
	constexpr int max_length = 20;

	AABB global_aabb;

	for (int i = 0; i < face_count; i++) {
		if (i == 0) {
			global_aabb = faces[i].get_aabb();
		} else {
			global_aabb.merge_with(faces[i].get_aabb());
		}
	}

	global_aabb.grow_by(0.01f); // Avoid numerical error.

	// Determine amount of cells in grid axis.
	int div_x, div_y, div_z;

	if (global_aabb.size.x / min_size < max_length) {
		div_x = (int)(global_aabb.size.x / min_size) + 1;
	} else {
		div_x = max_length;
	}

	if (global_aabb.size.y / min_size < max_length) {
		div_y = (int)(global_aabb.size.y / min_size) + 1;
	} else {
		div_y = max_length;
	}

	if (global_aabb.size.z / min_size < max_length) {
		div_z = (int)(global_aabb.size.z / min_size) + 1;
	} else {
		div_z = max_length;
	}

	Vector3 voxelsize = global_aabb.size;
	voxelsize.x /= div_x;
	voxelsize.y /= div_y;
	voxelsize.z /= div_z;

	// Create and initialize cells to zero.

	uint8_t ***cell_status = memnew_arr(uint8_t **, div_x);
	for (int i = 0; i < div_x; i++) {
		cell_status[i] = memnew_arr(uint8_t *, div_y);

		for (int j = 0; j < div_y; j++) {
			cell_status[i][j] = memnew_arr(uint8_t, div_z);

			for (int k = 0; k < div_z; k++) {
				cell_status[i][j][k] = 0;
			}
		}
	}

	// Plot faces into cells.

	for (int i = 0; i < face_count; i++) {
		Face3 f = faces[i];
		for (int j = 0; j < 3; j++) {
			f.vertex[j] -= global_aabb.position;
		}
		_plot_face(cell_status, 0, 0, 0, div_x, div_y, div_z, voxelsize, f);
	}

	// Determine which cells connect to the outside by traversing the outside and recursively flood-fill marking.

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_y; j++) {
			_mark_outside(cell_status, i, j, 0, div_x, div_y, div_z);
			_mark_outside(cell_status, i, j, div_z - 1, div_x, div_y, div_z);
		}
	}

	for (int i = 0; i < div_z; i++) {
		for (int j = 0; j < div_y; j++) {
			_mark_outside(cell_status, 0, j, i, div_x, div_y, div_z);
			_mark_outside(cell_status, div_x - 1, j, i, div_x, div_y, div_z);
		}
	}

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_z; j++) {
			_mark_outside(cell_status, i, 0, j, div_x, div_y, div_z);
			_mark_outside(cell_status, i, div_y - 1, j, div_x, div_y, div_z);
		}
	}

	// Build faces for the inside-outside cell divisors.

	Vector<Face3> wrapped_faces;

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_y; j++) {
			for (int k = 0; k < div_z; k++) {
				_build_faces(cell_status, i, j, k, div_x, div_y, div_z, wrapped_faces);
			}
		}
	}

	// Transform face vertices to global coords.

	int wrapped_faces_count = wrapped_faces.size();
	Face3 *wrapped_faces_ptr = wrapped_faces.ptrw();

	for (int i = 0; i < wrapped_faces_count; i++) {
		for (int j = 0; j < 3; j++) {
			Vector3 &v = wrapped_faces_ptr[i].vertex[j];
			v = v * voxelsize;
			v += global_aabb.position;
		}
	}

	// clean up grid

	for (int i = 0; i < div_x; i++) {
		for (int j = 0; j < div_y; j++) {
			memdelete_arr(cell_status[i][j]);
		}

		memdelete_arr(cell_status[i]);
	}

	memdelete_arr(cell_status);
	if (r_error) {
		*r_error = voxelsize.length();
	}

	return wrapped_faces;
}

Geometry3D::MeshData Geometry3D::build_convex_mesh(const Vector<Plane> &p_planes) {
	MeshData mesh;

#define SUBPLANE_SIZE 1024.0

	real_t subplane_size = 1024.0; // Should compute this from the actual plane.
	for (int i = 0; i < p_planes.size(); i++) {
		Plane p = p_planes[i];

		Vector3 ref = Vector3(0.0, 1.0, 0.0);

		if (Math::abs(p.normal.dot(ref)) > 0.95f) {
			ref = Vector3(0.0, 0.0, 1.0); // Change axis.
		}

		Vector3 right = p.normal.cross(ref).normalized();
		Vector3 up = p.normal.cross(right).normalized();

		Vector3 center = p.get_center();

		// make a quad clockwise
		LocalVector<Vector3> vertices = {
			center - up * subplane_size + right * subplane_size,
			center - up * subplane_size - right * subplane_size,
			center + up * subplane_size - right * subplane_size,
			center + up * subplane_size + right * subplane_size
		};

		for (int j = 0; j < p_planes.size(); j++) {
			if (j == i) {
				continue;
			}

			LocalVector<Vector3> new_vertices;
			Plane clip = p_planes[j];

			if (clip.normal.dot(p.normal) > 0.95f) {
				continue;
			}

			if (vertices.size() < 3) {
				break;
			}

			for (uint32_t k = 0; k < vertices.size(); k++) {
				int k_n = (k + 1) % vertices.size();

				Vector3 edge0_A = vertices[k];
				Vector3 edge1_A = vertices[k_n];

				real_t dist0 = clip.distance_to(edge0_A);
				real_t dist1 = clip.distance_to(edge1_A);

				if (dist0 <= 0) { // Behind plane.

					new_vertices.push_back(vertices[k]);
				}

				// Check for different sides and non coplanar.
				if ((dist0 * dist1) < 0) {
					// Calculate intersection.
					Vector3 rel = edge1_A - edge0_A;

					real_t den = clip.normal.dot(rel);
					if (Math::is_zero_approx(den)) {
						continue; // Point too short.
					}

					real_t dist = -(clip.normal.dot(edge0_A) - clip.d) / den;
					Vector3 inters = edge0_A + rel * dist;
					new_vertices.push_back(inters);
				}
			}

			vertices = new_vertices;
		}

		if (vertices.size() < 3) {
			continue;
		}

		// Result is a clockwise face.

		MeshData::Face face;

		// Add face indices.
		for (const Vector3 &vertex : vertices) {
			int idx = -1;
			for (uint32_t k = 0; k < mesh.vertices.size(); k++) {
				if (mesh.vertices[k].distance_to(vertex) < 0.001f) {
					idx = k;
					break;
				}
			}

			if (idx == -1) {
				idx = mesh.vertices.size();
				mesh.vertices.push_back(vertex);
			}

			face.indices.push_back(idx);
		}
		face.plane = p;
		mesh.faces.push_back(face);

		// Add edge.

		for (uint32_t j = 0; j < face.indices.size(); j++) {
			int a = face.indices[j];
			int b = face.indices[(j + 1) % face.indices.size()];

			bool found = false;
			int found_idx = -1;
			for (uint32_t k = 0; k < mesh.edges.size(); k++) {
				if (mesh.edges[k].vertex_a == a && mesh.edges[k].vertex_b == b) {
					found = true;
					found_idx = k;
					break;
				}
				if (mesh.edges[k].vertex_b == a && mesh.edges[k].vertex_a == b) {
					found = true;
					found_idx = k;
					break;
				}
			}

			if (found) {
				mesh.edges[found_idx].face_b = j;
				continue;
			}
			MeshData::Edge edge;
			edge.vertex_a = a;
			edge.vertex_b = b;
			edge.face_a = j;
			edge.face_b = -1;
			mesh.edges.push_back(edge);
		}
	}

	return mesh;
}

Vector<Plane> Geometry3D::build_box_planes(const Vector3 &p_extents) {
	Vector<Plane> planes = {
		Plane(Vector3(1, 0, 0), p_extents.x),
		Plane(Vector3(-1, 0, 0), p_extents.x),
		Plane(Vector3(0, 1, 0), p_extents.y),
		Plane(Vector3(0, -1, 0), p_extents.y),
		Plane(Vector3(0, 0, 1), p_extents.z),
		Plane(Vector3(0, 0, -1), p_extents.z)
	};

	return planes;
}

Vector<Plane> Geometry3D::build_cylinder_planes(real_t p_radius, real_t p_height, int p_sides, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX_V(p_axis, 3, Vector<Plane>());

	Vector<Plane> planes;

	const double sides_step = Math::TAU / p_sides;
	for (int i = 0; i < p_sides; i++) {
		Vector3 normal;
		normal[(p_axis + 1) % 3] = Math::cos(i * sides_step);
		normal[(p_axis + 2) % 3] = Math::sin(i * sides_step);

		planes.push_back(Plane(normal, p_radius));
	}

	Vector3 axis;
	axis[p_axis] = 1.0;

	planes.push_back(Plane(axis, p_height * 0.5f));
	planes.push_back(Plane(-axis, p_height * 0.5f));

	return planes;
}

Vector<Plane> Geometry3D::build_sphere_planes(real_t p_radius, int p_lats, int p_lons, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX_V(p_axis, 3, Vector<Plane>());

	Vector<Plane> planes;

	Vector3 axis;
	axis[p_axis] = 1.0;

	Vector3 axis_neg;
	axis_neg[(p_axis + 1) % 3] = 1.0;
	axis_neg[(p_axis + 2) % 3] = 1.0;
	axis_neg[p_axis] = -1.0;

	const double lon_step = Math::TAU / p_lons;
	for (int i = 0; i < p_lons; i++) {
		Vector3 normal;
		normal[(p_axis + 1) % 3] = Math::cos(i * lon_step);
		normal[(p_axis + 2) % 3] = Math::sin(i * lon_step);

		planes.push_back(Plane(normal, p_radius));

		for (int j = 1; j <= p_lats; j++) {
			Vector3 plane_normal = normal.lerp(axis, j / (real_t)p_lats).normalized();
			planes.push_back(Plane(plane_normal, p_radius));
			planes.push_back(Plane(plane_normal * axis_neg, p_radius));
		}
	}

	return planes;
}

Vector<Plane> Geometry3D::build_capsule_planes(real_t p_radius, real_t p_height, int p_sides, int p_lats, Vector3::Axis p_axis) {
	ERR_FAIL_INDEX_V(p_axis, 3, Vector<Plane>());

	Vector<Plane> planes;

	Vector3 axis;
	axis[p_axis] = 1.0;

	Vector3 axis_neg;
	axis_neg[(p_axis + 1) % 3] = 1.0;
	axis_neg[(p_axis + 2) % 3] = 1.0;
	axis_neg[p_axis] = -1.0;

	const double sides_step = Math::TAU / p_sides;
	for (int i = 0; i < p_sides; i++) {
		Vector3 normal;
		normal[(p_axis + 1) % 3] = Math::cos(i * sides_step);
		normal[(p_axis + 2) % 3] = Math::sin(i * sides_step);

		planes.push_back(Plane(normal, p_radius));

		for (int j = 1; j <= p_lats; j++) {
			Vector3 plane_normal = normal.lerp(axis, j / (real_t)p_lats).normalized();
			Vector3 position = axis * p_height * 0.5f + plane_normal * p_radius;
			planes.push_back(Plane(plane_normal, position));
			planes.push_back(Plane(plane_normal * axis_neg, position * axis_neg));
		}
	}

	return planes;
}

Vector<Vector3> Geometry3D::compute_convex_mesh_points(const Plane *p_planes, int p_plane_count) {
	Vector<Vector3> points;

	// Iterate through every unique combination of any three planes.
	for (int i = p_plane_count - 1; i >= 0; i--) {
		for (int j = i - 1; j >= 0; j--) {
			for (int k = j - 1; k >= 0; k--) {
				// Find the point where these planes all cross over (if they
				// do at all).
				Vector3 convex_shape_point;
				if (p_planes[i].intersect_3(p_planes[j], p_planes[k], &convex_shape_point)) {
					// See if any *other* plane excludes this point because it's
					// on the wrong side.
					bool excluded = false;
					for (int n = 0; n < p_plane_count; n++) {
						if (n != i && n != j && n != k) {
							real_t dp = p_planes[n].normal.dot(convex_shape_point);
							if (dp - p_planes[n].d > (real_t)CMP_EPSILON) {
								excluded = true;
								break;
							}
						}
					}

					// Only add the point if it passed all tests.
					if (!excluded) {
						points.push_back(convex_shape_point);
					}
				}
			}
		}
	}

	return points;
}

#define square(m_s) ((m_s) * (m_s))
#define BIG_VAL 1e20

/* dt of 1d function using squared distance */
static void edt(float *p_f, int p_stride, int p_n) {
	float *d = (float *)alloca(sizeof(float) * p_n + sizeof(int) * p_n + sizeof(float) * (p_n + 1));
	int *v = reinterpret_cast<int *>(&(d[p_n]));
	float *z = reinterpret_cast<float *>(&v[p_n]);

	int k = 0;
	v[0] = 0;
	z[0] = -BIG_VAL;
	z[1] = +BIG_VAL;
	for (int q = 1; q <= p_n - 1; q++) {
		float s = ((p_f[q * p_stride] + square(q)) - (p_f[v[k] * p_stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		while (s <= z[k]) {
			k--;
			s = ((p_f[q * p_stride] + square(q)) - (p_f[v[k] * p_stride] + square(v[k]))) / (2 * q - 2 * v[k]);
		}
		k++;
		v[k] = q;

		z[k] = s;
		z[k + 1] = +BIG_VAL;
	}

	k = 0;
	for (int q = 0; q <= p_n - 1; q++) {
		while (z[k + 1] < q) {
			k++;
		}
		d[q] = square(q - v[k]) + p_f[v[k] * p_stride];
	}

	for (int i = 0; i < p_n; i++) {
		p_f[i * p_stride] = d[i];
	}
}

#undef square

Vector<uint32_t> Geometry3D::generate_edf(const Vector<bool> &p_voxels, const Vector3i &p_size, bool p_negative) {
	uint32_t float_count = p_size.x * p_size.y * p_size.z;

	ERR_FAIL_COND_V((uint32_t)p_voxels.size() != float_count, Vector<uint32_t>());

	float *work_memory = memnew_arr(float, float_count);
	for (uint32_t i = 0; i < float_count; i++) {
		work_memory[i] = BIG_VAL;
	}

	uint32_t y_mult = p_size.x;
	uint32_t z_mult = y_mult * p_size.y;

	//plot solid cells
	{
		const bool *voxr = p_voxels.ptr();
		for (uint32_t i = 0; i < float_count; i++) {
			bool plot = voxr[i];
			if (p_negative) {
				plot = !plot;
			}
			if (plot) {
				work_memory[i] = 0;
			}
		}
	}

	//process in each direction

	//xy->z

	for (int i = 0; i < p_size.x; i++) {
		for (int j = 0; j < p_size.y; j++) {
			edt(&work_memory[i + j * y_mult], z_mult, p_size.z);
		}
	}

	//xz->y

	for (int i = 0; i < p_size.x; i++) {
		for (int j = 0; j < p_size.z; j++) {
			edt(&work_memory[i + j * z_mult], y_mult, p_size.y);
		}
	}

	//yz->x
	for (int i = 0; i < p_size.y; i++) {
		for (int j = 0; j < p_size.z; j++) {
			edt(&work_memory[i * y_mult + j * z_mult], 1, p_size.x);
		}
	}

	Vector<uint32_t> ret;
	ret.resize(float_count);
	{
		uint32_t *w = ret.ptrw();
		for (uint32_t i = 0; i < float_count; i++) {
			w[i] = uint32_t(Math::sqrt(work_memory[i]));
		}
	}

	memdelete_arr(work_memory);

	return ret;
}

#undef BIG_VAL

Vector<int8_t> Geometry3D::generate_sdf8(const Vector<uint32_t> &p_positive, const Vector<uint32_t> &p_negative) {
	ERR_FAIL_COND_V(p_positive.size() != p_negative.size(), Vector<int8_t>());
	Vector<int8_t> sdf8;
	int s = p_positive.size();
	sdf8.resize(s);

	const uint32_t *rpos = p_positive.ptr();
	const uint32_t *rneg = p_negative.ptr();
	int8_t *wsdf = sdf8.ptrw();
	for (int i = 0; i < s; i++) {
		int32_t diff = int32_t(rpos[i]) - int32_t(rneg[i]);
		wsdf[i] = CLAMP(diff, -128, 127);
	}
	return sdf8;
}

// ============================================================================
// Merged from core/math/dynamic_bvh.cpp
// ============================================================================

#include "servers/rendering/renderer.h"

void DynamicBVH::_delete_node(Node *p_node) {
	node_allocator.free(p_node);
}

void DynamicBVH::_recurse_delete_node(Node *p_node) {
	if (!p_node->is_leaf()) {
		_recurse_delete_node(p_node->children[0]);
		_recurse_delete_node(p_node->children[1]);
	}
	if (p_node == bvh_root) {
		bvh_root = nullptr;
	}
	_delete_node(p_node);
}

DynamicBVH::Node *DynamicBVH::_create_node(Node *p_parent, void *p_data) {
	Node *node = node_allocator.alloc();
	node->parent = p_parent;
	node->data = p_data;
	return (node);
}

DynamicBVH::Node *DynamicBVH::_create_node_with_volume(Node *p_parent, const Volume &p_volume, void *p_data) {
	Node *node = _create_node(p_parent, p_data);
	node->volume = p_volume;
	return node;
}

void DynamicBVH::_insert_leaf(Node *p_root, Node *p_leaf) {
	if (!bvh_root) {
		bvh_root = p_leaf;
		p_leaf->parent = nullptr;
	} else {
		if (!p_root->is_leaf()) {
			do {
				p_root = p_root->children[p_leaf->volume.select_by_proximity(
						p_root->children[0]->volume,
						p_root->children[1]->volume)];
			} while (!p_root->is_leaf());
		}
		Node *prev = p_root->parent;
		Node *node = _create_node_with_volume(prev, p_leaf->volume.merge(p_root->volume), nullptr);
		if (prev) {
			prev->children[p_root->get_index_in_parent()] = node;
			node->children[0] = p_root;
			p_root->parent = node;
			node->children[1] = p_leaf;
			p_leaf->parent = node;
			do {
				if (!prev->volume.contains(node->volume)) {
					prev->volume = prev->children[0]->volume.merge(prev->children[1]->volume);
				} else {
					break;
				}
				node = prev;
			} while (nullptr != (prev = node->parent));
		} else {
			node->children[0] = p_root;
			p_root->parent = node;
			node->children[1] = p_leaf;
			p_leaf->parent = node;
			bvh_root = node;
		}
	}
}

DynamicBVH::Node *DynamicBVH::_remove_leaf(Node *r_leaf) {
	if (r_leaf == bvh_root) {
		bvh_root = nullptr;
		return (nullptr);
	} else {
		Node *parent = r_leaf->parent;
		Node *prev = parent->parent;
		Node *sibling = parent->children[1 - r_leaf->get_index_in_parent()];
		if (prev) {
			prev->children[parent->get_index_in_parent()] = sibling;
			sibling->parent = prev;
			_delete_node(parent);
			while (prev) {
				const Volume pb = prev->volume;
				prev->volume = prev->children[0]->volume.merge(prev->children[1]->volume);
				if (pb.is_not_equal_to(prev->volume)) {
					prev = prev->parent;
				} else {
					break;
				}
			}
			return (prev ? prev : bvh_root);
		} else {
			bvh_root = sibling;
			sibling->parent = nullptr;
			_delete_node(parent);
			return (bvh_root);
		}
	}
}

void DynamicBVH::_fetch_leaves(Node *p_root, LocalVector<Node *> &r_leaves, int p_depth) {
	if (p_root->is_internal() && p_depth) {
		_fetch_leaves(p_root->children[0], r_leaves, p_depth - 1);
		_fetch_leaves(p_root->children[1], r_leaves, p_depth - 1);
		_delete_node(p_root);
	} else {
		r_leaves.push_back(p_root);
	}
}

// Partitions leaves such that leaves[0, n) are on the
// left of axis, and leaves[n, count) are on the right
// of axis. returns N.
int DynamicBVH::_split(Node **r_leaves, int p_count, const Vector3 &p_org, const Vector3 &p_axis) {
	int begin = 0;
	int end = p_count;
	for (;;) {
		while (begin != end && r_leaves[begin]->is_left_of_axis(p_org, p_axis)) {
			++begin;
		}

		if (begin == end) {
			break;
		}

		while (begin != end && !r_leaves[end - 1]->is_left_of_axis(p_org, p_axis)) {
			--end;
		}

		if (begin == end) {
			break;
		}

		// swap out of place nodes
		--end;
		Node *temp = r_leaves[begin];
		r_leaves[begin] = r_leaves[end];
		r_leaves[end] = temp;
		++begin;
	}

	return begin;
}

DynamicBVH::Volume DynamicBVH::_bounds(Node **r_leaves, int p_count) {
	Volume volume = r_leaves[0]->volume;
	for (int i = 1, ni = p_count; i < ni; ++i) {
		volume = volume.merge(r_leaves[i]->volume);
	}
	return (volume);
}

void DynamicBVH::_bottom_up(Node **r_leaves, int p_count) {
	while (p_count > 1) {
		real_t minsize = Math::INF;
		int minidx[2] = { -1, -1 };
		for (int i = 0; i < p_count; ++i) {
			for (int j = i + 1; j < p_count; ++j) {
				const real_t sz = r_leaves[i]->volume.merge(r_leaves[j]->volume).get_size();
				if (sz < minsize) {
					minsize = sz;
					minidx[0] = i;
					minidx[1] = j;
				}
			}
		}
		Node *n[] = { r_leaves[minidx[0]], r_leaves[minidx[1]] };
		Node *p = _create_node_with_volume(nullptr, n[0]->volume.merge(n[1]->volume), nullptr);
		p->children[0] = n[0];
		p->children[1] = n[1];
		n[0]->parent = p;
		n[1]->parent = p;
		r_leaves[minidx[0]] = p;
		r_leaves[minidx[1]] = r_leaves[p_count - 1];
		--p_count;
	}
}

DynamicBVH::Node *DynamicBVH::_top_down(Node **r_leaves, int p_count, int p_bu_threshold) {
	static const Vector3 axis[] = { Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1) };

	ERR_FAIL_COND_V(p_bu_threshold <= 1, nullptr);
	if (p_count > 1) {
		if (p_count > p_bu_threshold) {
			const Volume vol = _bounds(r_leaves, p_count);
			const Vector3 org = vol.get_center();
			int partition;
			int bestaxis = -1;
			int bestmidp = p_count;
			int splitcount[3][2] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };
			int i;
			for (i = 0; i < p_count; ++i) {
				const Vector3 x = r_leaves[i]->volume.get_center() - org;
				for (int j = 0; j < 3; ++j) {
					++splitcount[j][x.dot(axis[j]) > 0 ? 1 : 0];
				}
			}
			for (i = 0; i < 3; ++i) {
				if ((splitcount[i][0] > 0) && (splitcount[i][1] > 0)) {
					const int midp = (int)Math::abs(real_t(splitcount[i][0] - splitcount[i][1]));
					if (midp < bestmidp) {
						bestaxis = i;
						bestmidp = midp;
					}
				}
			}
			if (bestaxis >= 0) {
				partition = _split(r_leaves, p_count, org, axis[bestaxis]);
				ERR_FAIL_COND_V(partition == 0 || partition == p_count, nullptr);
			} else {
				partition = p_count / 2 + 1;
			}

			Node *node = _create_node_with_volume(nullptr, vol, nullptr);
			node->children[0] = _top_down(&r_leaves[0], partition, p_bu_threshold);
			node->children[1] = _top_down(&r_leaves[partition], p_count - partition, p_bu_threshold);
			node->children[0]->parent = node;
			node->children[1]->parent = node;
			return (node);
		} else {
			_bottom_up(r_leaves, p_count);
			return (r_leaves[0]);
		}
	}
	return (r_leaves[0]);
}

DynamicBVH::Node *DynamicBVH::_node_sort(Node *r_node, Node *&r_root) {
	Node *p = r_node->parent;
	ERR_FAIL_COND_V(!r_node->is_internal(), nullptr);
	if (p > r_node) {
		const int i = r_node->get_index_in_parent();
		const int j = 1 - i;
		Node *s = p->children[j];
		Node *q = p->parent;
		ERR_FAIL_COND_V(r_node != p->children[i], nullptr);
		if (q) {
			q->children[p->get_index_in_parent()] = r_node;
		} else {
			r_root = r_node;
		}
		s->parent = r_node;
		p->parent = r_node;
		r_node->parent = q;
		p->children[0] = r_node->children[0];
		p->children[1] = r_node->children[1];
		r_node->children[0]->parent = p;
		r_node->children[1]->parent = p;
		r_node->children[i] = p;
		r_node->children[j] = s;
		SWAP(p->volume, r_node->volume);
		return (p);
	}
	return (r_node);
}

void DynamicBVH::clear() {
	if (bvh_root) {
		_recurse_delete_node(bvh_root);
	}
	lkhd = -1;
	opath = 0;
}

void DynamicBVH::optimize_bottom_up() {
	if (bvh_root) {
		LocalVector<Node *> leaves;
		_fetch_leaves(bvh_root, leaves);
		_bottom_up(&leaves[0], leaves.size());
		bvh_root = leaves[0];
	}
}

void DynamicBVH::optimize_top_down(int p_bu_threshold) {
	if (bvh_root) {
		LocalVector<Node *> leaves;
		_fetch_leaves(bvh_root, leaves);
		bvh_root = _top_down(&leaves[0], leaves.size(), p_bu_threshold);
	}
}

void DynamicBVH::optimize_incremental(int p_passes) {
	if (p_passes < 0) {
		p_passes = total_leaves;
	}
	if (p_passes > 0) {
		do {
			if (!bvh_root) {
				break;
			}
			Node *node = bvh_root;
			unsigned bit = 0;
			while (node->is_internal()) {
				node = _node_sort(node, bvh_root)->children[(opath >> bit) & 1];
				bit = (bit + 1) & (sizeof(unsigned) * 8 - 1);
			}
			_update(node);
			++opath;
		} while (--p_passes);
	}
}

DynamicBVH::ID DynamicBVH::insert(const AABB &p_box, void *p_userdata) {
	Volume volume;
	volume.min = p_box.position;
	volume.max = p_box.position + p_box.size;

	Node *leaf = _create_node_with_volume(nullptr, volume, p_userdata);
	_insert_leaf(bvh_root, leaf);
	++total_leaves;

	ID id;
	id.node = leaf;

	return id;
}

void DynamicBVH::_update(Node *r_leaf, int p_lookahead) {
	Node *root = _remove_leaf(r_leaf);
	if (root) {
		if (p_lookahead >= 0) {
			for (int i = 0; (i < p_lookahead) && root->parent; ++i) {
				root = root->parent;
			}
		} else {
			root = bvh_root;
		}
	}
	_insert_leaf(root, r_leaf);
}

bool DynamicBVH::update(const ID &p_id, const AABB &p_box) {
	ERR_FAIL_COND_V(!p_id.is_valid(), false);
	Node *leaf = p_id.node;

	Volume volume;
	volume.min = p_box.position;
	volume.max = p_box.position + p_box.size;

	if (leaf->volume.min.is_equal_approx(volume.min) && leaf->volume.max.is_equal_approx(volume.max)) {
		// noop
		return false;
	}

	Node *base = _remove_leaf(leaf);
	if (base) {
		if (lkhd >= 0) {
			for (int i = 0; (i < lkhd) && base->parent; ++i) {
				base = base->parent;
			}
		} else {
			base = bvh_root;
		}
	}
	leaf->volume = volume;
	_insert_leaf(base, leaf);
	return true;
}

void DynamicBVH::remove(const ID &p_id) {
	ERR_FAIL_COND(!p_id.is_valid());
	Node *leaf = p_id.node;
	_remove_leaf(leaf);
	_delete_node(leaf);
	--total_leaves;
}

void DynamicBVH::_extract_leaves(Node *p_node, List<ID> *r_elements) {
	if (p_node->is_internal()) {
		_extract_leaves(p_node->children[0], r_elements);
		_extract_leaves(p_node->children[1], r_elements);
	} else {
		ID id;
		id.node = p_node;
		r_elements->push_back(id);
	}
}

void DynamicBVH::set_index(uint32_t p_index) {
	ERR_FAIL_COND(bvh_root != nullptr);
	index = p_index;
}

uint32_t DynamicBVH::get_index() const {
	return index;
}

void DynamicBVH::get_elements(List<ID> *r_elements) {
	if (bvh_root) {
		_extract_leaves(bvh_root, r_elements);
	}
}

int DynamicBVH::get_leaf_count() const {
	return total_leaves;
}
int DynamicBVH::get_max_depth() const {
	if (bvh_root) {
		int depth = 1;
		int max_depth = 0;
		bvh_root->get_max_depth(depth, max_depth);
		return max_depth;
	} else {
		return 0;
	}
}

DynamicBVH::~DynamicBVH() {
	clear();
}

// ============================================================================
// Merged from core/math/triangle_mesh.cpp
// ============================================================================

#include "core/templates/sort_array.h"
#include "core/math/geometry_3d.h"

int TriangleMesh::_create_bvh(
	BVH* p_bvh, BVH** p_bb, int p_from, int p_size, int p_depth, int& r_max_depth, int& r_max_alloc)
{
	if (p_depth > r_max_depth) {
		r_max_depth = p_depth;
	}

	if (p_size == 1) {
		return p_bb[p_from] - p_bvh;
	}
	else if (p_size == 0) {
		return -1;
	}

	AABB aabb;
	aabb = p_bb[p_from]->aabb;
	for (int i = 1; i < p_size; i++) {
		aabb.merge_with(p_bb[p_from + i]->aabb);
	}

	int li = aabb.get_longest_axis_index();

	switch (li) {
	case Vector3::AXIS_X: {
		SortArray<BVH*, BVHCmpX> sort_x;
		sort_x.nth_element(0, p_size, p_size / 2, &p_bb[p_from]);
		// sort_x.sort(&p_bb[p_from],p_size);
	} break;
	case Vector3::AXIS_Y: {
		SortArray<BVH*, BVHCmpY> sort_y;
		sort_y.nth_element(0, p_size, p_size / 2, &p_bb[p_from]);
		// sort_y.sort(&p_bb[p_from],p_size);
	} break;
	case Vector3::AXIS_Z: {
		SortArray<BVH*, BVHCmpZ> sort_z;
		sort_z.nth_element(0, p_size, p_size / 2, &p_bb[p_from]);
		// sort_z.sort(&p_bb[p_from],p_size);

	} break;
	}

	int left = _create_bvh(p_bvh, p_bb, p_from, p_size / 2, p_depth + 1, r_max_depth, r_max_alloc);
	int right = _create_bvh(p_bvh, p_bb, p_from + p_size / 2, p_size - p_size / 2, p_depth + 1,
		r_max_depth, r_max_alloc);

	int index = r_max_alloc++;
	BVH* _new = &p_bvh[index];
	_new->aabb = aabb;
	_new->center = aabb.get_center();
	_new->face_index = -1;
	_new->left = left;
	_new->right = right;

	return index;
}

void TriangleMesh::get_indices(Vector<int>* r_triangles_indices) const
{
	if (!valid) {
		return;
	}

	const int triangles_num = triangles.size();

	// Parse vertices indices
	const Triangle* triangles_read = triangles.ptr();

	r_triangles_indices->resize(triangles_num * 3);
	int* r_indices_write = r_triangles_indices->ptrw();

	for (int i = 0; i < triangles_num; ++i) {
		r_indices_write[3 * i + 0] = triangles_read[i].indices[0];
		r_indices_write[3 * i + 1] = triangles_read[i].indices[1];
		r_indices_write[3 * i + 2] = triangles_read[i].indices[2];
	}
}

void TriangleMesh::create(const Vector<Vector3>& p_faces, const Vector<int32_t>& p_surface_indices)
{
	valid = false;

	ERR_FAIL_COND(p_surface_indices.size() && p_surface_indices.size() != p_faces.size());

	int fc = p_faces.size();
	ERR_FAIL_COND(!fc || ((fc % 3) != 0));
	fc /= 3;
	triangles.resize(fc);

	bvh.resize(fc * 3); // will never be larger than this (todo make better)
	BVH* bw = bvh.ptrw();

	{
		// create faces and indices and base bvh
		// except for the Set for repeated triangles, everything
		// goes in-place.

		const Vector3* r = p_faces.ptr();
		const int32_t* si = p_surface_indices.ptr();
		Triangle* w = triangles.ptrw();
		HashMap<Vector3, int> db;

		for (int i = 0; i < fc; i++) {
			Triangle& f = w[i];
			const Vector3* v = &r[i * 3];

			for (int j = 0; j < 3; j++) {
				int vidx = -1;
				Vector3 vs = v[j].snappedf(0.0001);
				HashMap<Vector3, int>::Iterator E = db.find(vs);
				if (E) {
					vidx = E->value;
				}
				else {
					vidx = db.size();
					db[vs] = vidx;
				}

				f.indices[j] = vidx;
				if (j == 0) {
					bw[i].aabb.position = vs;
				}
				else {
					bw[i].aabb.expand_to(vs);
				}
			}

			f.normal = Face3(r[i * 3 + 0], r[i * 3 + 1], r[i * 3 + 2]).get_plane().get_normal();
			f.surface_index = si ? si[i] : 0;

			bw[i].left = -1;
			bw[i].right = -1;
			bw[i].face_index = i;
			bw[i].center = bw[i].aabb.get_center();
		}

		vertices.resize(db.size());
		Vector3* vw = vertices.ptrw();
		for (const KeyValue<Vector3, int>& E : db) {
			vw[E.value] = E.key;
		}
	}

	Vector<BVH*> bwptrs;
	bwptrs.resize(fc);
	BVH** bwp = bwptrs.ptrw();
	for (int i = 0; i < fc; i++) {
		bwp[i] = &bw[i];
	}

	max_depth = 0;
	int max_alloc = fc;
	_create_bvh(bw, bwp, 0, fc, 1, max_depth, max_alloc);

	bvh.resize(max_alloc); // resize back

	valid = true;
}

bool TriangleMesh::intersect_segment(const Vector3& p_begin, const Vector3& p_end, Vector3& r_point,
	Vector3& r_normal, int32_t* r_surf_index, int32_t* r_face_index) const
{
	if (!valid) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * max_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,

	};

	Vector3 n = (p_end - p_begin).normalized();
	real_t d = 1e10;
	bool inters = false;

	int level = 0;

	const Triangle* triangleptr = triangles.ptr();
	const Vector3* vertexptr = vertices.ptr();
	const BVH* bvhptr = bvh.ptr();

	int pos = bvh.size() - 1;

	stack[0] = pos;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& b = bvhptr[node];
		bool done = false;

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			if (!b.aabb.intersects_segment(p_begin, p_end)) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (b.face_index >= 0) {
					const Triangle& s = triangleptr[b.face_index];
					Face3 f3(
						vertexptr[s.indices[0]], vertexptr[s.indices[1]], vertexptr[s.indices[2]]);

					Vector3 res;

					if (f3.intersects_segment(p_begin, p_end, &res)) {
						real_t nd = n.dot(res);
						if (nd < d) {
							d = nd;
							r_point = res;
							r_normal = f3.get_plane().get_normal();
							if (r_surf_index) {
								*r_surf_index = s.surface_index;
							}
							if (r_face_index) {
								*r_face_index = b.face_index;
							}
							inters = true;
						}
					}

					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
			continue;
		}
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.left | TEST_AABB_BIT;
			continue;
		}
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.right | TEST_AABB_BIT;
			continue;
		}
		case VISIT_DONE_BIT: {
			if (level == 0) {
				done = true;
				break;
			}
			else {
				level--;
			}
			continue;
		}
		}

		if (done) {
			break;
		}
	}

	if (inters) {
		if (n.dot(r_normal) > 0) {
			r_normal = -r_normal;
		}
	}

	return inters;
}

bool TriangleMesh::intersect_ray(const Vector3& p_begin, const Vector3& p_dir, Vector3& r_point,
	Vector3& r_normal, int32_t* r_surf_index, int32_t* r_face_index) const
{
	if (!valid) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * max_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,

	};

	Vector3 n = p_dir;
	real_t d = 1e20;
	bool inters = false;

	int level = 0;

	const Triangle* triangleptr = triangles.ptr();
	const Vector3* vertexptr = vertices.ptr();
	const BVH* bvhptr = bvh.ptr();

	int pos = bvh.size() - 1;

	stack[0] = pos;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& b = bvhptr[node];
		bool done = false;

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			if (!b.aabb.intersects_ray(p_begin, p_dir)) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			}
			else {
				if (b.face_index >= 0) {
					const Triangle& s = triangleptr[b.face_index];
					Face3 f3(
						vertexptr[s.indices[0]], vertexptr[s.indices[1]], vertexptr[s.indices[2]]);

					Vector3 res;

					if (f3.intersects_ray(p_begin, p_dir, &res)) {
						real_t nd = n.dot(res);
						if (nd < d) {
							d = nd;
							r_point = res;
							r_normal = f3.get_plane().get_normal();
							if (r_surf_index) {
								*r_surf_index = s.surface_index;
							}
							if (r_face_index) {
								*r_face_index = b.face_index;
							}
							inters = true;
						}
					}

					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
			continue;
		}
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.left | TEST_AABB_BIT;
			continue;
		}
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.right | TEST_AABB_BIT;
			continue;
		}
		case VISIT_DONE_BIT: {
			if (level == 0) {
				done = true;
				break;
			}
			else {
				level--;
			}
			continue;
		}
		}

		if (done) {
			break;
		}
	}

	if (inters) {
		if (n.dot(r_normal) > 0) {
			r_normal = -r_normal;
		}
	}

	return inters;
}

bool TriangleMesh::inside_convex_shape(const Plane* p_planes, int p_plane_count,
	const Vector3* p_points, int p_point_count, Vector3 p_scale) const
{
	if (!valid) {
		return false;
	}

	uint32_t* stack = (uint32_t*)alloca(sizeof(int) * max_depth);

	enum
	{
		TEST_AABB_BIT = 0,
		VISIT_LEFT_BIT = 1,
		VISIT_RIGHT_BIT = 2,
		VISIT_DONE_BIT = 3,
		VISITED_BIT_SHIFT = 29,
		NODE_IDX_MASK = (1 << VISITED_BIT_SHIFT) - 1,
		VISITED_BIT_MASK = ~NODE_IDX_MASK,

	};

	int level = 0;

	const Triangle* triangleptr = triangles.ptr();
	const Vector3* vertexptr = vertices.ptr();
	const BVH* bvhptr = bvh.ptr();

	Transform3D scale(Basis().scaled(p_scale));

	int pos = bvh.size() - 1;

	stack[0] = pos;
	while (true) {
		uint32_t node = stack[level] & NODE_IDX_MASK;
		const BVH& b = bvhptr[node];
		bool done = false;

		switch (stack[level] >> VISITED_BIT_SHIFT) {
		case TEST_AABB_BIT: {
			bool intersects = scale.xform(b.aabb).intersects_convex_shape(
				p_planes, p_plane_count, p_points, p_point_count);
			if (!intersects) {
				return false;
			}

			bool inside = scale.xform(b.aabb).inside_convex_shape(p_planes, p_plane_count);
			if (inside) {
				stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

			}
			else {
				if (b.face_index >= 0) {
					const Triangle& s = triangleptr[b.face_index];
					for (int j = 0; j < 3; ++j) {
						Vector3 point = scale.xform(vertexptr[s.indices[j]]);
						for (int i = 0; i < p_plane_count; i++) {
							const Plane& p = p_planes[i];
							if (p.is_point_over(point)) {
								return false;
							}
						}
					}

					stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;

				}
				else {
					stack[level] = (VISIT_LEFT_BIT << VISITED_BIT_SHIFT) | node;
				}
			}
			continue;
		}
		case VISIT_LEFT_BIT: {
			stack[level] = (VISIT_RIGHT_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.left | TEST_AABB_BIT;
			continue;
		}
		case VISIT_RIGHT_BIT: {
			stack[level] = (VISIT_DONE_BIT << VISITED_BIT_SHIFT) | node;
			level++;
			stack[level] = b.right | TEST_AABB_BIT;
			continue;
		}
		case VISIT_DONE_BIT: {
			if (level == 0) {
				done = true;
				break;
			}
			else {
				level--;
			}
			continue;
		}
		}

		if (done) {
			break;
		}
	}

	return true;
}

bool TriangleMesh::is_valid() const { return valid; }

Vector<Face3> TriangleMesh::get_faces() const
{
	if (!valid) {
		return Vector<Face3>();
	}

	Vector<Face3> faces;
	int ts = triangles.size();
	faces.resize(triangles.size());

	Face3* w = faces.ptrw();
	const Triangle* r = triangles.ptr();
	const Vector3* rv = vertices.ptr();

	for (int i = 0; i < ts; i++) {
		for (int j = 0; j < 3; j++) {
			w[i].vertex[j] = rv[r[i].indices[j]];
		}
	}

	return faces;
}

bool TriangleMesh::create_from_faces(const Vector<Vector3>& p_faces)
{
	create(p_faces);
	return is_valid();
}

Vector<Vector3> TriangleMesh::get_faces_scriptwrap() const
{
	if (!valid) {
		return Vector<Vector3>();
	}

	Vector<Vector3> faces;
	int ts = triangles.size();
	faces.resize(triangles.size() * 3);

	Vector3* w = faces.ptrw();
	const Triangle* r = triangles.ptr();
	const Vector3* rv = vertices.ptr();

	for (int i = 0; i < ts; i++) {
		for (int j = 0; j < 3; j++) {
			w[i * 3 + j] = rv[r[i].indices[j]];
		}
	}

	return faces;
}

TriangleMesh::TriangleMesh()
{
	valid = false;
	max_depth = 0;
}


// ============================================================================
// Merged from core/math/expression.cpp
// ============================================================================

#include "core/types.h"

Error Expression::_get_token(Token& r_token)
{
	while (true) {
#define GET_CHAR() (str_ofs >= expression.length() ? 0 : expression[str_ofs++])

		char32_t cchar = GET_CHAR();

		switch (cchar) {
		case 0: {
			r_token.type = TK_EOF;
			return OK;
		}
		case '{': {
			r_token.type = TK_CURLY_BRACKET_OPEN;
			return OK;
		}
		case '}': {
			r_token.type = TK_CURLY_BRACKET_CLOSE;
			return OK;
		}
		case '[': {
			r_token.type = TK_BRACKET_OPEN;
			return OK;
		}
		case ']': {
			r_token.type = TK_BRACKET_CLOSE;
			return OK;
		}
		case '(': {
			r_token.type = TK_PARENTHESIS_OPEN;
			return OK;
		}
		case ')': {
			r_token.type = TK_PARENTHESIS_CLOSE;
			return OK;
		}
		case ',': {
			r_token.type = TK_COMMA;
			return OK;
		}
		case ':': {
			r_token.type = TK_COLON;
			return OK;
		}
		case '$': {
			r_token.type = TK_INPUT;
			int index = 0;
			do {
				if (!is_digit(expression[str_ofs])) {
					_set_error("Expected number after '$'");
					r_token.type = TK_ERROR;
					return ERR_PARSE_ERROR;
				}
				index *= 10;
				index += expression[str_ofs] - '0';
				str_ofs++;

			} while (is_digit(expression[str_ofs]));
			return OK;
		}
		case '=': {
			cchar = GET_CHAR();
			if (cchar == '=') {
				r_token.type = TK_OP_EQUAL;
			}
			else {
				_set_error("Expected '='");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}
			return OK;
		}
		case '!': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_NOT_EQUAL;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_NOT;
			}
			return OK;
		}
		case '>': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_GREATER_EQUAL;
				str_ofs++;
			}
			else if (expression[str_ofs] == '>') {
				r_token.type = TK_OP_SHIFT_RIGHT;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_GREATER;
			}
			return OK;
		}
		case '<': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_LESS_EQUAL;
				str_ofs++;
			}
			else if (expression[str_ofs] == '<') {
				r_token.type = TK_OP_SHIFT_LEFT;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_LESS;
			}
			return OK;
		}
		case '+': {
			r_token.type = TK_OP_ADD;
			return OK;
		}
		case '-': {
			r_token.type = TK_OP_SUB;
			return OK;
		}
		case '/': {
			r_token.type = TK_OP_DIV;
			return OK;
		}
		case '*': {
			if (expression[str_ofs] == '*') {
				r_token.type = TK_OP_POW;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_MUL;
			}
			return OK;
		}
		case '%': {
			r_token.type = TK_OP_MOD;
			return OK;
		}
		case '&': {
			if (expression[str_ofs] == '&') {
				r_token.type = TK_OP_AND;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_BIT_AND;
			}
			return OK;
		}
		case '|': {
			if (expression[str_ofs] == '|') {
				r_token.type = TK_OP_OR;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_BIT_OR;
			}
			return OK;
		}
		case '^': {
			r_token.type = TK_OP_BIT_XOR;

			return OK;
		}
		case '~': {
			r_token.type = TK_OP_BIT_INVERT;

			return OK;
		}
		case '\'':
		case '"': {
			String str;
			char32_t prev = 0;
			while (true) {
				char32_t ch = GET_CHAR();

				if (ch == 0) {
					_set_error("Unterminated String");
					r_token.type = TK_ERROR;
					return ERR_PARSE_ERROR;
				}
				else if (ch == cchar) {
					// cchar contain a corresponding quote symbol
					break;
				}
				else if (ch == '\\') {
					// escaped characters...

					char32_t next = GET_CHAR();
					if (next == 0) {
						_set_error("Unterminated String");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					char32_t res = 0;

					switch (next) {
					case 'b':
						res = 8;
						break;
					case 't':
						res = 9;
						break;
					case 'n':
						res = 10;
						break;
					case 'f':
						res = 12;
						break;
					case 'r':
						res = 13;
						break;
					case 'U':
					case 'u': {
						// Hexadecimal sequence.
						int hex_len = (next == 'U') ? 6 : 4;
						for (int j = 0; j < hex_len; j++) {
							char32_t c = GET_CHAR();

							if (c == 0) {
								_set_error("Unterminated String");
								r_token.type = TK_ERROR;
								return ERR_PARSE_ERROR;
							}
							if (!is_hex_digit(c)) {
								_set_error("Malformed hex constant in string");
								r_token.type = TK_ERROR;
								return ERR_PARSE_ERROR;
							}
							char32_t v;
							if (is_digit(c)) {
								v = c - '0';
							}
							else if (c >= 'a' && c <= 'f') {
								v = c - 'a';
								v += 10;
							}
							else if (c >= 'A' && c <= 'F') {
								v = c - 'A';
								v += 10;
							}
							else {
								ERR_PRINT("Bug parsing hex constant.");
								v = 0;
							}

							res <<= 4;
							res |= v;
						}

					} break;
					default: {
						res = next;
					} break;
					}

					// Parse UTF-16 pair.
					if ((res & 0xfffffc00) == 0xd800) {
						if (prev == 0) {
							prev = res;
							continue;
						}
						else {
							_set_error(
								"Invalid UTF-16 sequence in string, unpaired lead surrogate");
							r_token.type = TK_ERROR;
							return ERR_PARSE_ERROR;
						}
					}
					else if ((res & 0xfffffc00) == 0xdc00) {
						if (prev == 0) {
							_set_error(
								"Invalid UTF-16 sequence in string, unpaired trail surrogate");
							r_token.type = TK_ERROR;
							return ERR_PARSE_ERROR;
						}
						else {
							res = (prev << 10UL) + res - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
							prev = 0;
						}
					}
					if (prev != 0) {
						_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					str += res;
				}
				else {
					if (prev != 0) {
						_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					str += ch;
				}
			}
			if (prev != 0) {
				_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}

			r_token.type = TK_CONSTANT;
			return OK;

		} break;
		default: {
			if (cchar <= 32) {
				break;
			}

			char32_t next_char = (str_ofs >= expression.length()) ? 0 : expression[str_ofs];
			if (is_digit(cchar) || (cchar == '.' && is_digit(next_char))) {
				// a number

				String num;
#define READING_SIGN 0
#define READING_INT 1
#define READING_HEX 2
#define READING_BIN 3
#define READING_DEC 4
#define READING_EXP 5
#define READING_DONE 6
				int reading = READING_INT;

				char32_t c = cchar;
				bool exp_sign = false;
				bool exp_beg = false;
				bool bin_beg = false;
				bool hex_beg = false;
				bool is_float = false;
				bool is_first_char = true;

				while (true) {
					switch (reading) {
					case READING_INT: {
						if (is_digit(c)) {
							if (is_first_char && c == '0') {
								if (next_char == 'b' || next_char == 'B') {
									reading = READING_BIN;
								}
								else if (next_char == 'x' || next_char == 'X') {
									reading = READING_HEX;
								}
							}
						}
						else if (c == '.') {
							reading = READING_DEC;
							is_float = true;
						}
						else if (c == 'e' || c == 'E') {
							reading = READING_EXP;
							is_float = true;
						}
						else {
							reading = READING_DONE;
						}

					} break;
					case READING_BIN: {
						if (bin_beg && !is_binary_digit(c)) {
							reading = READING_DONE;
						}
						else if (c == 'b' || c == 'B') {
							bin_beg = true;
						}

					} break;
					case READING_HEX: {
						if (hex_beg && !is_hex_digit(c)) {
							reading = READING_DONE;
						}
						else if (c == 'x' || c == 'X') {
							hex_beg = true;
						}

					} break;
					case READING_DEC: {
						if (is_digit(c)) {
						}
						else if (c == 'e' || c == 'E') {
							reading = READING_EXP;
						}
						else {
							reading = READING_DONE;
						}

					} break;
					case READING_EXP: {
						if (is_digit(c)) {
							exp_beg = true;

						}
						else if ((c == '-' || c == '+') && !exp_sign && !exp_beg) {
							exp_sign = true;

						}
						else {
							reading = READING_DONE;
						}
					} break;
					}

					if (reading == READING_DONE) {
						break;
					}
					num += c;
					c = GET_CHAR();
					is_first_char = false;
				}

				if (c != 0) {
					str_ofs--;
				}

				r_token.type = TK_CONSTANT;

				return OK;

			}
			else if (is_unicode_identifier_start(cchar)) {
				String id = String::chr(cchar);
				cchar = GET_CHAR();

				while (is_unicode_identifier_continue(cchar)) {
					id += cchar;
					cchar = GET_CHAR();
				}

				str_ofs--; // go back one

				if (id == "in") {
					r_token.type = TK_OP_IN;
				}
				else if (id == "null") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "true") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "false") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "PI") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "TAU") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "INF") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "NAN") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "not") {
					r_token.type = TK_OP_NOT;
				}
				else if (id == "or") {
					r_token.type = TK_OP_OR;
				}
				else if (id == "and") {
					r_token.type = TK_OP_AND;
				}
				else if (id == "self") {
					r_token.type = TK_SELF;
				}
				else {
					r_token.type = TK_IDENTIFIER;
				}
				return OK;

			}
			else if (cchar == '.') {
				// Handled down there as we support '.[0-9]' as numbers above
				r_token.type = TK_PERIOD;
				return OK;

			}
			else {
				_set_error("Unexpected character.");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}
		}
		}
#undef GET_CHAR
	}

	r_token.type = TK_ERROR;
	return ERR_PARSE_ERROR;
}

const char* Expression::token_name[TK_MAX] = {"CURLY BRACKET OPEN", "CURLY BRACKET CLOSE",
	"BRACKET OPEN", "BRACKET CLOSE", "PARENTHESIS OPEN", "PARENTHESIS CLOSE", "IDENTIFIER",
	"BUILTIN FUNC", "SELF", "CONSTANT", "BASIC TYPE", "COLON", "COMMA", "PERIOD", "OP IN",
	"OP EQUAL", "OP NOT EQUAL", "OP LESS", "OP LESS EQUAL", "OP GREATER", "OP GREATER EQUAL",
	"OP AND", "OP OR", "OP NOT", "OP ADD", "OP SUB", "OP MUL", "OP DIV", "OP MOD", "OP POW",
	"OP SHIFT LEFT", "OP SHIFT RIGHT", "OP BIT AND", "OP BIT OR", "OP BIT XOR", "OP BIT INVERT",
	"OP INPUT", "EOF", "ERROR"};

bool Expression::_compile_expression()
{
	if (!expression_dirty) {
		return error_set;
	}

	if (nodes) {
		memdelete(nodes);
		nodes = nullptr;
		root = nullptr;
	}

	error_str = String();
	error_set = false;
	str_ofs = 0;

	root = _parse_expression();

	if (error_set) {
		root = nullptr;
		memdelete(nodes);
		nodes = nullptr;
		return true;
	}

	expression_dirty = false;
	return false;
}

Error Expression::parse(const String& p_expression, const Vector<String>& p_input_names)
{
	if (nodes) {
		memdelete(nodes);
		nodes = nullptr;
		root = nullptr;
	}

	error_str = String();
	error_set = false;
	str_ofs = 0;
	input_names = p_input_names;

	expression = p_expression;
	root = _parse_expression();

	if (error_set) {
		root = nullptr;
		memdelete(nodes);
		nodes = nullptr;
		return ERR_INVALID_PARAMETER;
	}

	return OK;
}

bool Expression::has_execute_failed() const { return execution_error; }

String Expression::get_error_text() const { return error_str; }

Expression::~Expression()
{
	if (nodes) {
		memdelete(nodes);
	}
}

Expression::ENode* Expression::_parse_expression()
{
	return nullptr;
}


// ============================================================================
// Merged from core/math/a_star.cpp
// ============================================================================

#include "core/math/geometry_3d.h"

int64_t AStar3D::get_available_point_id() const
{
	if (points.has(last_free_id)) {
		int64_t cur_new_id = last_free_id + 1;
		while (points.has(cur_new_id)) {
			cur_new_id++;
		}
		last_free_id = cur_new_id;
	}

	return last_free_id;
}

void AStar3D::add_point(int64_t p_id, const Vector3& p_pos, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(p_id < 0, vformat("Can't add a point with negative id: %d.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't add a point with weight scale less than 0.0: %f.", p_weight_scale));

	Point** point_entry = points.getptr(p_id);

	if (!point_entry) {
		Point* pt = memnew(Point);
		pt->id = p_id;
		pt->pos = p_pos;
		pt->weight_scale = p_weight_scale;
		pt->prev_point = nullptr;
		pt->open_pass = 0;
		pt->closed_pass = 0;
		pt->enabled = true;
		points.insert_new(p_id, pt);
	}
	else {
		Point* found_pt = *point_entry;
		found_pt->pos = p_pos;
		found_pt->weight_scale = p_weight_scale;
	}
}

Vector3 AStar3D::get_point_position(int64_t p_id) const
{
	Point* const* point_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!point_entry, Vector3(),
		vformat("Can't get point's position. Point with id: %d doesn't exist.", p_id));

	return (*point_entry)->pos;
}

void AStar3D::set_point_position(int64_t p_id, const Vector3& p_pos)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!point_entry,
		vformat("Can't set point's position. Point with id: %d doesn't exist.", p_id));

	(*point_entry)->pos = p_pos;
}

real_t AStar3D::get_point_weight_scale(int64_t p_id) const
{
	Point* const* point_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!point_entry, 0,
		vformat("Can't get point's weight scale. Point with id: %d doesn't exist.", p_id));

	return (*point_entry)->weight_scale;
}

void AStar3D::set_point_weight_scale(int64_t p_id, real_t p_weight_scale)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!point_entry,
		vformat("Can't set point's weight scale. Point with id: %d doesn't exist.", p_id));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	(*point_entry)->weight_scale = p_weight_scale;
}

void AStar3D::remove_point(int64_t p_id)
{
	Point** point_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!point_entry, vformat("Can't remove point. Point with id: %d doesn't exist.", p_id));
	Point* p = *point_entry;

	for (KeyValue<int64_t, Point*>& kv : p->neighbors) {
		Segment s(p_id, kv.key);
		segments.erase(s);

		kv.value->neighbors.erase(p->id);
		kv.value->unlinked_neighbours.erase(p->id);
	}

	for (KeyValue<int64_t, Point*>& kv : p->unlinked_neighbours) {
		Segment s(p_id, kv.key);
		segments.erase(s);

		kv.value->neighbors.erase(p->id);
		kv.value->unlinked_neighbours.erase(p->id);
	}

	memdelete(p);
	points.erase(p_id);
	last_free_id = p_id;
}

void AStar3D::connect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	ERR_FAIL_COND_MSG(
		p_id == p_with_id, vformat("Can't connect point with id: %d to itself.", p_id));

	Point** a_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!a_entry, vformat("Can't connect points. Point with id: %d doesn't exist.", p_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_with_id);
	ERR_FAIL_COND_MSG(
		!b_entry, vformat("Can't connect points. Point with id: %d doesn't exist.", p_with_id));
	Point* b = *b_entry;

	a->neighbors.insert(b->id, b);

	if (p_bidirectional) {
		b->neighbors.insert(a->id, a);
	}
	else {
		b->unlinked_neighbours.insert(a->id, a);
	}

	Segment s(p_id, p_with_id);
	if (p_bidirectional) {
		s.direction = Segment::BIDIRECTIONAL;
	}

	HashSet<Segment, Segment>::Iterator element = segments.find(s);
	if (element) {
		s.direction |= element->direction;
		if (s.direction == Segment::BIDIRECTIONAL) {
			// Both are neighbors of each other now
			a->unlinked_neighbours.erase(b->id);
			b->unlinked_neighbours.erase(a->id);
		}
		segments.remove(element);
	}

	segments.insert(s);
}

void AStar3D::disconnect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	Point** a_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(
		!a_entry, vformat("Can't disconnect points. Point with id: %d doesn't exist.", p_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_with_id);
	ERR_FAIL_COND_MSG(
		!b_entry, vformat("Can't disconnect points. Point with id: %d doesn't exist.", p_with_id));
	Point* b = *b_entry;

	Segment s(p_id, p_with_id);
	int remove_direction = p_bidirectional ? (int)Segment::BIDIRECTIONAL : (int)s.direction;

	HashSet<Segment, Segment>::Iterator element = segments.find(s);
	if (element) {
		// s is the new segment
		// Erase the directions to be removed
		s.direction = (element->direction & ~remove_direction);

		a->neighbors.erase(b->id);
		if (p_bidirectional) {
			b->neighbors.erase(a->id);
			if (element->direction != Segment::BIDIRECTIONAL) {
				a->unlinked_neighbours.erase(b->id);
				b->unlinked_neighbours.erase(a->id);
			}
		}
		else {
			if (s.direction == Segment::NONE) {
				b->unlinked_neighbours.erase(a->id);
			}
			else {
				a->unlinked_neighbours.insert(b->id, b);
			}
		}

		segments.remove(element);
		if (s.direction != Segment::NONE) {
			segments.insert(s);
		}
	}
}

bool AStar3D::has_point(int64_t p_id) const { return points.has(p_id); }

PackedInt64Array AStar3D::get_point_ids()
{
	PackedInt64Array point_list;

	for (KeyValue<int64_t, Point*>& kv : points) {
		point_list.push_back(kv.key);
	}

	return point_list;
}

Vector<int64_t> AStar3D::get_point_connections(int64_t p_id)
{
	Point** p_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!p_entry, Vector<int64_t>(),
		vformat("Can't get point's connections. Point with id: %d doesn't exist.", p_id));

	Vector<int64_t> point_list;

	for (KeyValue<int64_t, Point*>& kv : (*p_entry)->neighbors) {
		point_list.push_back(kv.key);
	}

	return point_list;
}

bool AStar3D::are_points_connected(int64_t p_id, int64_t p_with_id, bool p_bidirectional) const
{
	Segment s(p_id, p_with_id);
	const HashSet<Segment, Segment>::Iterator element = segments.find(s);

	return element && (p_bidirectional || (element->direction & s.direction) == s.direction);
}

void AStar3D::clear()
{
	last_free_id = 0;
	for (KeyValue<int64_t, Point*>& kv : points) {
		memdelete(kv.value);
	}
	segments.clear();
	points.clear();
}

int64_t AStar3D::get_point_count() const { return points.size(); }

int64_t AStar3D::get_point_capacity() const { return points.get_capacity(); }

void AStar3D::reserve_space(int64_t p_num_nodes)
{
	ERR_FAIL_COND_MSG(p_num_nodes <= 0,
		vformat("New capacity must be greater than 0, new was: %d.", p_num_nodes));
	points.reserve(p_num_nodes);
}

int64_t AStar3D::get_closest_point(const Vector3& p_point, bool p_include_disabled) const
{
	int64_t closest_id = -1;
	real_t closest_dist = 1e20;

	for (const KeyValue<int64_t, Point*>& kv : points) {
		if (!p_include_disabled && !kv.value->enabled) {
			continue; // Disabled points should not be considered.
		}

		// Keep the closest point's ID, and in case of multiple closest IDs,
		// the smallest one (makes it deterministic).
		real_t d = p_point.distance_squared_to(kv.value->pos);
		int64_t id = kv.key;
		if (d <= closest_dist) {
			if (d == closest_dist && id > closest_id) { // Keep lowest ID.
				continue;
			}
			closest_dist = d;
			closest_id = id;
		}
	}

	return closest_id;
}

Vector3 AStar3D::get_closest_position_in_segment(const Vector3& p_point) const
{
	real_t closest_dist = 1e20;
	Vector3 closest_point;

	for (const Segment& E : segments) {
		const Point* from_point = *points.getptr(E.key.first);
		const Point* to_point = *points.getptr(E.key.second);

		if (!(from_point->enabled && to_point->enabled)) {
			continue;
		}

		Vector3 p =
			Geometry3D::get_closest_point_to_segment(p_point, from_point->pos, to_point->pos);
		real_t d = p_point.distance_squared_to(p);
		if (d < closest_dist) {
			closest_point = p;
			closest_dist = d;
		}
	}

	return closest_point;
}

bool AStar3D::_solve(Point* p_begin_point, Point* p_end_point, bool p_allow_partial_path)
{
	last_closest_point = nullptr;
	pass++;

	if (!p_begin_point->enabled) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (!p_end_point->enabled && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point*> open_list;
	SortArray<Point*, SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == nullptr || last_closest_point->abs_f_score > p->abs_f_score ||
			(last_closest_point->abs_f_score >= p->abs_f_score &&
				last_closest_point->abs_g_score > p->abs_g_score)) {
			last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		for (const KeyValue<int64_t, Point*>& kv : p->neighbors) {
			Point* e = kv.value; // The neighbor point.

			if (!e->enabled || e->closed_pass == pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);
			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

real_t AStar3D::_estimate_cost(int64_t p_from_id, int64_t p_end_id)
{
	Point** from_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!from_entry, 0,
		vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));
	Point* from_point = *from_entry;

	Point** end_entry = points.getptr(p_end_id);
	ERR_FAIL_COND_V_MSG(
		!end_entry, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_end_id));
	Point* end_point = *end_entry;

	return from_point->pos.distance_to(end_point->pos);
}

real_t AStar3D::_compute_cost(int64_t p_from_id, int64_t p_to_id)
{
	Point** from_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(
		!from_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));
	Point* from_point = *from_entry;

	Point** to_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(
		!to_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));
	Point* to_point = *to_entry;

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector3> AStar3D::get_point_path(
	int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	Point** a_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<Vector3>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<Vector3>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));
	Point* b = *b_entry;

	Point* begin_point = a;
	Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<Vector3>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector3> path;
	path.resize(pc);

	{
		Vector3* w = path.ptrw();

		Point* p2 = end_point;
		int64_t idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = p2->pos;
			p2 = p2->prev_point;
		}

		w[0] = p2->pos; // Assign first
	}

	return path;
}

Vector<int64_t> AStar3D::get_id_path(int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	Point** a_entry = points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));
	Point* a = *a_entry;

	Point** b_entry = points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));
	Point* b = *b_entry;

	Point* begin_point = a;
	Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<int64_t>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int64_t> path;
	path.resize(pc);

	{
		int64_t* w = path.ptrw();

		p = end_point;
		int64_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

bool AStar3D::is_neighbor_filter_enabled() const { return neighbor_filter_enabled; }

void AStar3D::set_neighbor_filter_enabled(bool p_enabled) { neighbor_filter_enabled = p_enabled; }

void AStar3D::set_point_disabled(int64_t p_id, bool p_disabled)
{
	Point** p_entry = points.getptr(p_id);
	ERR_FAIL_COND_MSG(!p_entry,
		vformat("Can't set if point is disabled. Point with id: %d doesn't exist.", p_id));
	Point* p = *p_entry;

	p->enabled = !p_disabled;
}

bool AStar3D::is_point_disabled(int64_t p_id) const
{
	Point* const* p_entry = points.getptr(p_id);
	ERR_FAIL_COND_V_MSG(!p_entry, false,
		vformat("Can't get if point is disabled. Point with id: %d doesn't exist.", p_id));
	Point* p = *p_entry;

	return !p->enabled;
}

AStar3D::~AStar3D() { clear(); }

int64_t AStar2D::get_available_point_id() const { return astar.get_available_point_id(); }

void AStar2D::add_point(int64_t p_id, const Vector2& p_pos, real_t p_weight_scale)
{
	astar.add_point(p_id, Vector3(p_pos.x, p_pos.y, 0), p_weight_scale);
}

Vector2 AStar2D::get_point_position(int64_t p_id) const
{
	Vector3 p = astar.get_point_position(p_id);
	return Vector2(p.x, p.y);
}

void AStar2D::set_point_position(int64_t p_id, const Vector2& p_pos)
{
	astar.set_point_position(p_id, Vector3(p_pos.x, p_pos.y, 0));
}

real_t AStar2D::get_point_weight_scale(int64_t p_id) const
{
	return astar.get_point_weight_scale(p_id);
}

void AStar2D::set_point_weight_scale(int64_t p_id, real_t p_weight_scale)
{
	astar.set_point_weight_scale(p_id, p_weight_scale);
}

void AStar2D::remove_point(int64_t p_id) { astar.remove_point(p_id); }

bool AStar2D::has_point(int64_t p_id) const { return astar.has_point(p_id); }

Vector<int64_t> AStar2D::get_point_connections(int64_t p_id)
{
	return astar.get_point_connections(p_id);
}

PackedInt64Array AStar2D::get_point_ids() { return astar.get_point_ids(); }

bool AStar2D::is_neighbor_filter_enabled() const { return astar.neighbor_filter_enabled; }

void AStar2D::set_neighbor_filter_enabled(bool p_enabled)
{
	astar.neighbor_filter_enabled = p_enabled;
}

void AStar2D::set_point_disabled(int64_t p_id, bool p_disabled)
{
	astar.set_point_disabled(p_id, p_disabled);
}

bool AStar2D::is_point_disabled(int64_t p_id) const { return astar.is_point_disabled(p_id); }

void AStar2D::connect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	astar.connect_points(p_id, p_with_id, p_bidirectional);
}

void AStar2D::disconnect_points(int64_t p_id, int64_t p_with_id, bool p_bidirectional)
{
	astar.disconnect_points(p_id, p_with_id, p_bidirectional);
}

bool AStar2D::are_points_connected(int64_t p_id, int64_t p_with_id, bool p_bidirectional) const
{
	return astar.are_points_connected(p_id, p_with_id, p_bidirectional);
}

int64_t AStar2D::get_point_count() const { return astar.get_point_count(); }

int64_t AStar2D::get_point_capacity() const { return astar.get_point_capacity(); }

void AStar2D::clear() { astar.clear(); }

void AStar2D::reserve_space(int64_t p_num_nodes) { astar.reserve_space(p_num_nodes); }

int64_t AStar2D::get_closest_point(const Vector2& p_point, bool p_include_disabled) const
{
	return astar.get_closest_point(Vector3(p_point.x, p_point.y, 0), p_include_disabled);
}

Vector2 AStar2D::get_closest_position_in_segment(const Vector2& p_point) const
{
	Vector3 p = astar.get_closest_position_in_segment(Vector3(p_point.x, p_point.y, 0));
	return Vector2(p.x, p.y);
}

real_t AStar2D::_estimate_cost(int64_t p_from_id, int64_t p_end_id)
{
	AStar3D::Point** from_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!from_entry, 0,
		vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* from_point = *from_entry;

	AStar3D::Point** end_entry = astar.points.getptr(p_end_id);
	ERR_FAIL_COND_V_MSG(
		!end_entry, 0, vformat("Can't estimate cost. Point with id: %d doesn't exist.", p_end_id));
	AStar3D::Point* end_point = *end_entry;

	return from_point->pos.distance_to(end_point->pos);
}

real_t AStar2D::_compute_cost(int64_t p_from_id, int64_t p_to_id)
{
	AStar3D::Point** from_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(
		!from_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* from_point = *from_entry;

	AStar3D::Point** to_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(
		!to_entry, 0, vformat("Can't compute cost. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* to_point = *to_entry;

	return from_point->pos.distance_to(to_point->pos);
}

Vector<Vector2> AStar2D::get_point_path(
	int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	AStar3D::Point** a_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<Vector2>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* a = *a_entry;

	AStar3D::Point** b_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!b_entry, Vector<Vector2>(),
		vformat("Can't get point path. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* b = *b_entry;

	AStar3D::Point* begin_point = a;
	AStar3D::Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || astar.last_closest_point == nullptr) {
			return Vector<Vector2>();
		}

		// Use closest point instead.
		end_point = astar.last_closest_point;
	}

	AStar3D::Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2* w = path.ptrw();

		AStar3D::Point* p2 = end_point;
		int64_t idx = pc - 1;
		while (p2 != begin_point) {
			w[idx--] = Vector2(p2->pos.x, p2->pos.y);
			p2 = p2->prev_point;
		}

		w[0] = Vector2(p2->pos.x, p2->pos.y); // Assign first
	}

	return path;
}

Vector<int64_t> AStar2D::get_id_path(int64_t p_from_id, int64_t p_to_id, bool p_allow_partial_path)
{
	AStar3D::Point** a_entry = astar.points.getptr(p_from_id);
	ERR_FAIL_COND_V_MSG(!a_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_from_id));
	AStar3D::Point* a = *a_entry;

	AStar3D::Point** to_entry = astar.points.getptr(p_to_id);
	ERR_FAIL_COND_V_MSG(!to_entry, Vector<int64_t>(),
		vformat("Can't get id path. Point with id: %d doesn't exist.", p_to_id));
	AStar3D::Point* b = *to_entry;

	AStar3D::Point* begin_point = a;
	AStar3D::Point* end_point = b;

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || astar.last_closest_point == nullptr) {
			return Vector<int64_t>();
		}

		// Use closest point instead.
		end_point = astar.last_closest_point;
	}

	AStar3D::Point* p = end_point;
	int64_t pc = 1; // Begin point
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<int64_t> path;
	path.resize(pc);

	{
		int64_t* w = path.ptrw();

		p = end_point;
		int64_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->id;
			p = p->prev_point;
		}

		w[0] = p->id; // Assign first
	}

	return path;
}

bool AStar2D::_solve(
	AStar3D::Point* p_begin_point, AStar3D::Point* p_end_point, bool p_allow_partial_path)
{
	astar.last_closest_point = nullptr;
	astar.pass++;

	if (!p_begin_point->enabled) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (!p_end_point->enabled && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<AStar3D::Point*> open_list;
	SortArray<AStar3D::Point*, AStar3D::SortPoints> sorter;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);

	while (!open_list.is_empty()) {
		AStar3D::Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (astar.last_closest_point == nullptr ||
			astar.last_closest_point->abs_f_score > p->abs_f_score ||
			(astar.last_closest_point->abs_f_score >= p->abs_f_score &&
				astar.last_closest_point->abs_g_score > p->abs_g_score)) {
			astar.last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route =
 true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = astar.pass; // Mark the point as closed.

		for (KeyValue<int64_t, AStar3D::Point*>& kv : p->neighbors) {
			AStar3D::Point* e = kv.value; // The neighbor point.

			if (!e->enabled || e->closed_pass == astar.pass) {
				continue;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * e->weight_scale;

			bool new_point = false;

			if (e->open_pass != astar.pass) { // The point wasn't inside the open list.
				e->open_pass = astar.pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);
			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

// --- a_star.compat.inc ---

#ifndef DISABLE_DEPRECATED


Vector<int64_t> AStar3D::_get_id_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector3> AStar3D::_get_point_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStar3D::_bind_compatibility_methods() {}

Vector<int64_t> AStar2D::_get_id_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector2> AStar2D::_get_point_path_bind_compat_88047(int64_t p_from_id, int64_t p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStar2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- a_star_grid_2d.cpp ---

static real_t heuristic_euclidean(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return (real_t)Math::sqrt(dx * dx + dy * dy);
}

static real_t heuristic_manhattan(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return dx + dy;
}

static real_t heuristic_octile(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	real_t F = Math::SQRT2 - 1;
	return (dx < dy) ? F * dx + dy : F * dy + dx;
}

static real_t heuristic_chebyshev(const Vector2i& p_from, const Vector2i& p_to)
{
	real_t dx = (real_t)Math::abs(p_to.x - p_from.x);
	real_t dy = (real_t)Math::abs(p_to.y - p_from.y);
	return MAX(dx, dy);
}

static real_t (*heuristics[AStarGrid2D::HEURISTIC_MAX])(const Vector2i&, const Vector2i&) = {
	heuristic_euclidean, heuristic_manhattan, heuristic_octile, heuristic_chebyshev};

void AStarGrid2D::set_region(const Rect2i& p_region)
{
	ERR_FAIL_COND(p_region.size.x < 0 || p_region.size.y < 0);
	if (p_region != region) {
		region = p_region;
		dirty = true;
	}
}

Rect2i AStarGrid2D::get_region() const { return region; }

void AStarGrid2D::set_size(const Size2i& p_size)
{
	WARN_DEPRECATED_MSG(R"(The "size" property is deprecated, use "region" instead.)");
	ERR_FAIL_COND(p_size.x < 0 || p_size.y < 0);
	if (p_size != region.size) {
		region.size = p_size;
		dirty = true;
	}
}

Size2i AStarGrid2D::get_size() const { return region.size; }

void AStarGrid2D::set_offset(const Vector2& p_offset)
{
	if (!offset.is_equal_approx(p_offset)) {
		offset = p_offset;
		dirty = true;
	}
}

Vector2 AStarGrid2D::get_offset() const { return offset; }

void AStarGrid2D::set_cell_size(const Size2& p_cell_size)
{
	if (!cell_size.is_equal_approx(p_cell_size)) {
		cell_size = p_cell_size;
		dirty = true;
	}
}

Size2 AStarGrid2D::get_cell_size() const { return cell_size; }

void AStarGrid2D::set_cell_shape(CellShape p_cell_shape)
{
	if (cell_shape == p_cell_shape) {
		return;
	}

	ERR_FAIL_INDEX(p_cell_shape, CellShape::CELL_SHAPE_MAX);
	cell_shape = p_cell_shape;
	dirty = true;
}

AStarGrid2D::CellShape AStarGrid2D::get_cell_shape() const { return cell_shape; }

void AStarGrid2D::update()
{
	if (!dirty) {
		return;
	}

	points.clear();
	solid_mask.clear();

	const int32_t end_x = region.get_end().x;
	const int32_t end_y = region.get_end().y;
	const Vector2 half_cell_size = cell_size / 2;
	for (int32_t x = region.position.x; x < end_x + 2; x++) {
		solid_mask.push_back(true);
	}

	for (int32_t y = region.position.y; y < end_y; y++) {
		LocalVector<Point> line;
		solid_mask.push_back(true);
		for (int32_t x = region.position.x; x < end_x; x++) {
			Vector2 v = offset;
			switch (cell_shape) {
			case CELL_SHAPE_ISOMETRIC_RIGHT:
				v += half_cell_size + Vector2(x + y, y - x) * half_cell_size;
				break;
			case CELL_SHAPE_ISOMETRIC_DOWN:
				v += half_cell_size + Vector2(x - y, x + y) * half_cell_size;
				break;
			case CELL_SHAPE_SQUARE:
				v += Vector2(x, y) * cell_size;
				break;
			default:
				break;
			}
			line.push_back(Point(Vector2i(x, y), v));
			solid_mask.push_back(false);
		}
		solid_mask.push_back(true);
		points.push_back(std::move(line));
	}

	for (int32_t x = region.position.x; x < end_x + 2; x++) {
		solid_mask.push_back(true);
	}

	dirty = false;
}

bool AStarGrid2D::is_in_bounds(int32_t p_x, int32_t p_y) const
{
	return region.has_point(Vector2i(p_x, p_y));
}

bool AStarGrid2D::is_in_boundsv(const Vector2i& p_id) const { return region.has_point(p_id); }

bool AStarGrid2D::is_dirty() const { return dirty; }

void AStarGrid2D::set_jumping_enabled(bool p_enabled) { jumping_enabled = p_enabled; }

bool AStarGrid2D::is_jumping_enabled() const { return jumping_enabled; }

void AStarGrid2D::set_diagonal_mode(DiagonalMode p_diagonal_mode)
{
	ERR_FAIL_INDEX((int)p_diagonal_mode, (int)DIAGONAL_MODE_MAX);
	diagonal_mode = p_diagonal_mode;
}

AStarGrid2D::DiagonalMode AStarGrid2D::get_diagonal_mode() const { return diagonal_mode; }

void AStarGrid2D::set_default_compute_heuristic(Heuristic p_heuristic)
{
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_compute_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_compute_heuristic() const
{
	return default_compute_heuristic;
}

void AStarGrid2D::set_default_estimate_heuristic(Heuristic p_heuristic)
{
	ERR_FAIL_INDEX((int)p_heuristic, (int)HEURISTIC_MAX);
	default_estimate_heuristic = p_heuristic;
}

AStarGrid2D::Heuristic AStarGrid2D::get_default_estimate_heuristic() const
{
	return default_estimate_heuristic;
}

void AStarGrid2D::set_point_solid(const Vector2i& p_id, bool p_solid)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id),
		vformat("Can't set if point is disabled. Point %s out of bounds %s.", p_id, region));
	_set_solid_unchecked(p_id, p_solid);
}

bool AStarGrid2D::is_point_solid(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, false, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), false,
		vformat("Can't get if point is disabled. Point %s out of bounds %s.", p_id, region));
	return _get_solid_unchecked(p_id);
}

void AStarGrid2D::set_point_weight_scale(const Vector2i& p_id, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(!is_in_boundsv(p_id),
		vformat("Can't set point's weight scale. Point %s out of bounds %s.", p_id, region));
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));
	_get_point_unchecked(p_id)->weight_scale = p_weight_scale;
}

real_t AStarGrid2D::get_point_weight_scale(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, 0, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), 0,
		vformat("Can't get point's weight scale. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->weight_scale;
}

void AStarGrid2D::fill_solid_region(const Rect2i& p_region, bool p_solid)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_set_solid_unchecked(x, y, p_solid);
		}
	}
}

void AStarGrid2D::fill_weight_scale_region(const Rect2i& p_region, real_t p_weight_scale)
{
	ERR_FAIL_COND_MSG(dirty, "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_MSG(p_weight_scale < 0.0,
		vformat("Can't set point's weight scale less than 0.0: %f.", p_weight_scale));

	const Rect2i safe_region = p_region.intersection(region);
	const int32_t end_x = safe_region.get_end().x;
	const int32_t end_y = safe_region.get_end().y;

	for (int32_t y = safe_region.position.y; y < end_y; y++) {
		for (int32_t x = safe_region.position.x; x < end_x; x++) {
			_get_point_unchecked(x, y)->weight_scale = p_weight_scale;
		}
	}
}

AStarGrid2D::Point* AStarGrid2D::_jump(Point* p_from, Point* p_to)
{
	int32_t from_x = p_from->id.x;
	int32_t from_y = p_from->id.y;

	int32_t to_x = p_to->id.x;
	int32_t to_y = p_to->id.y;

	int32_t dx = to_x - from_x;
	int32_t dy = to_y - from_y;

	if (diagonal_mode == DIAGONAL_MODE_ALWAYS ||
		diagonal_mode == DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE) {
		if (dx == 0 || dy == 0) {
			return _forced_successor(to_x, to_y, dx, dy);
		}

		while (_is_walkable(to_x, to_y) &&
			   (diagonal_mode == DIAGONAL_MODE_ALWAYS || _is_walkable(to_x, to_y - dy) ||
				   _is_walkable(to_x - dx, to_y))) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x - dx, to_y + dy) && !_is_walkable(to_x - dx, to_y)) ||
				(_is_walkable(to_x + dx, to_y - dy) && !_is_walkable(to_x, to_y - dy))) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x + dx, to_y, dx, 0) != nullptr ||
				_forced_successor(to_x, to_y + dy, 0, dy) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_x += dx;
			to_y += dy;
		}

	}
	else if (diagonal_mode == DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES) {
		if (dx == 0 || dy == 0) {
			return _forced_successor(from_x, from_y, dx, dy, true);
		}

		while (_is_walkable(to_x, to_y) && _is_walkable(to_x, to_y - dy) &&
			   _is_walkable(to_x - dx, to_y)) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x + dx, to_y + dy) && !_is_walkable(to_x, to_y + dy)) ||
				!_is_walkable(to_x + dx, to_y)) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x, to_y, dx, 0) != nullptr ||
				_forced_successor(to_x, to_y, 0, dy) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_x += dx;
			to_y += dy;
		}

	}
	else { // DIAGONAL_MODE_NEVER
		if (dy == 0) {
			return _forced_successor(from_x, from_y, dx, 0, true);
		}

		while (_is_walkable(to_x, to_y)) {
			if (end->id.x == to_x && end->id.y == to_y) {
				return end;
			}

			if ((_is_walkable(to_x - 1, to_y) && !_is_walkable(to_x - 1, to_y - dy)) ||
				(_is_walkable(to_x + 1, to_y) && !_is_walkable(to_x + 1, to_y - dy))) {
				return _get_point_unchecked(to_x, to_y);
			}

			if (_forced_successor(to_x, to_y, 1, 0, true) != nullptr ||
				_forced_successor(to_x, to_y, -1, 0, true) != nullptr) {
				return _get_point_unchecked(to_x, to_y);
			}

			to_y += dy;
		}
	}

	return nullptr;
}

AStarGrid2D::Point* AStarGrid2D::_forced_successor(
	int32_t p_x, int32_t p_y, int32_t p_dx, int32_t p_dy, bool p_inclusive)
{
	// Remembering previous results can improve performance.
	bool l_prev = false, r_prev = false, l = false, r = false;

	int32_t o_x = p_x, o_y = p_y;
	if (p_inclusive) {
		o_x += p_dx;
		o_y += p_dy;
	}

	int32_t l_x = p_x - p_dy, l_y = p_y - p_dx;
	int32_t r_x = p_x + p_dy, r_y = p_y + p_dx;

	while (_is_walkable(o_x, o_y)) {
		if (end->id.x == o_x && end->id.y == o_y) {
			return end;
		}

		l_prev = l || _is_walkable(l_x, l_y);
		r_prev = r || _is_walkable(r_x, r_y);

		l_x += p_dx;
		l_y += p_dy;
		r_x += p_dx;
		r_y += p_dy;

		l = _is_walkable(l_x, l_y);
		r = _is_walkable(r_x, r_y);

		if ((l && !l_prev) || (r && !r_prev)) {
			return _get_point_unchecked(o_x, o_y);
		}

		o_x += p_dx;
		o_y += p_dy;
	}
	return nullptr;
}

void AStarGrid2D::_get_nbors(Point* p_point, LocalVector<Point*>& r_nbors)
{
	bool ts0 = false, td0 = false, ts1 = false, td1 = false, ts2 = false, td2 = false, ts3 = false,
		 td3 = false;

	Point* left = nullptr;
	Point* right = nullptr;
	Point* top = nullptr;
	Point* bottom = nullptr;

	Point* top_left = nullptr;
	Point* top_right = nullptr;
	Point* bottom_left = nullptr;
	Point* bottom_right = nullptr;

	{
		bool has_left = false;
		bool has_right = false;

		if (p_point->id.x - 1 >= region.position.x) {
			left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y);
			has_left = true;
		}
		if (p_point->id.x + 1 < region.position.x + region.size.width) {
			right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y);
			has_right = true;
		}
		if (p_point->id.y - 1 >= region.position.y) {
			top = _get_point_unchecked(p_point->id.x, p_point->id.y - 1);
			if (has_left) {
				top_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y - 1);
			}
			if (has_right) {
				top_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y - 1);
			}
		}
		if (p_point->id.y + 1 < region.position.y + region.size.height) {
			bottom = _get_point_unchecked(p_point->id.x, p_point->id.y + 1);
			if (has_left) {
				bottom_left = _get_point_unchecked(p_point->id.x - 1, p_point->id.y + 1);
			}
			if (has_right) {
				bottom_right = _get_point_unchecked(p_point->id.x + 1, p_point->id.y + 1);
			}
		}
	}

	if (top && !_get_solid_unchecked(top->id)) {
		r_nbors.push_back(top);
		ts0 = true;
	}
	if (right && !_get_solid_unchecked(right->id)) {
		r_nbors.push_back(right);
		ts1 = true;
	}
	if (bottom && !_get_solid_unchecked(bottom->id)) {
		r_nbors.push_back(bottom);
		ts2 = true;
	}
	if (left && !_get_solid_unchecked(left->id)) {
		r_nbors.push_back(left);
		ts3 = true;
	}

	switch (diagonal_mode) {
	case DIAGONAL_MODE_ALWAYS: {
		td0 = true;
		td1 = true;
		td2 = true;
		td3 = true;
	} break;
	case DIAGONAL_MODE_NEVER: {
	} break;
	case DIAGONAL_MODE_AT_LEAST_ONE_WALKABLE: {
		td0 = ts3 || ts0;
		td1 = ts0 || ts1;
		td2 = ts1 || ts2;
		td3 = ts2 || ts3;
	} break;
	case DIAGONAL_MODE_ONLY_IF_NO_OBSTACLES: {
		td0 = ts3 && ts0;
		td1 = ts0 && ts1;
		td2 = ts1 && ts2;
		td3 = ts2 && ts3;
	} break;
	default:
		break;
	}

	if (td0 && (top_left && !_get_solid_unchecked(top_left->id))) {
		r_nbors.push_back(top_left);
	}
	if (td1 && (top_right && !_get_solid_unchecked(top_right->id))) {
		r_nbors.push_back(top_right);
	}
	if (td2 && (bottom_right && !_get_solid_unchecked(bottom_right->id))) {
		r_nbors.push_back(bottom_right);
	}
	if (td3 && (bottom_left && !_get_solid_unchecked(bottom_left->id))) {
		r_nbors.push_back(bottom_left);
	}
}

bool AStarGrid2D::_solve(Point* p_begin_point, Point* p_end_point, bool p_allow_partial_path)
{
	last_closest_point = nullptr;
	pass++;

	if (_get_solid_unchecked(p_begin_point->id)) {
		return false;
	}
	if (p_begin_point == p_end_point) {
		return true;
	}
	if (_get_solid_unchecked(p_end_point->id) && !p_allow_partial_path) {
		return false;
	}

	bool found_route = false;

	LocalVector<Point*> open_list;
	SortArray<Point*, SortPoints> sorter;
	LocalVector<Point*> nbors;

	p_begin_point->g_score = 0;
	p_begin_point->f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	p_begin_point->abs_g_score = 0;
	p_begin_point->abs_f_score = _estimate_cost(p_begin_point->id, p_end_point->id);
	open_list.push_back(p_begin_point);
	end = p_end_point;

	while (!open_list.is_empty()) {
		Point* p = open_list[0]; // The currently processed point.

		// Find point closer to end_point, or same distance to end_point but closer to begin_point.
		if (last_closest_point == nullptr || last_closest_point->abs_f_score > p->abs_f_score ||
			(last_closest_point->abs_f_score >= p->abs_f_score &&
				last_closest_point->abs_g_score > p->abs_g_score)) {
			last_closest_point = p;
		}

		if (p == p_end_point) {
			found_route = true;
			break;
		}

		sorter.pop_heap(
			0, open_list.size(), open_list.ptr()); // Remove the current point from the open list.
		open_list.remove_at(open_list.size() - 1);
		p->closed_pass = pass; // Mark the point as closed.

		nbors.clear();
		_get_nbors(p, nbors);

		for (Point* e : nbors) {
			real_t weight_scale = 1.0;

			if (jumping_enabled) {
				// TODO: Make it works with weight_scale.
				e = _jump(p, e);
				if (!e || e->closed_pass == pass) {
					continue;
				}
			}
			else {
				if (_get_solid_unchecked(e->id) || e->closed_pass == pass) {
					continue;
				}
				weight_scale = e->weight_scale;
			}

			real_t tentative_g_score = p->g_score + _compute_cost(p->id, e->id) * weight_scale;
			bool new_point = false;

			if (e->open_pass != pass) { // The point wasn't inside the open list.
				e->open_pass = pass;
				open_list.push_back(e);
				new_point = true;
			}
			else if (tentative_g_score >=
					   e->g_score) { // The new path is worse than the previous.
				continue;
			}

			e->prev_point = p;
			e->g_score = tentative_g_score;
			e->f_score = e->g_score + _estimate_cost(e->id, p_end_point->id);

			e->abs_g_score = tentative_g_score;
			e->abs_f_score = e->f_score - e->g_score;

			if (new_point) { // The position of the new points is already known.
				sorter.push_heap(0, open_list.size() - 1, 0, e, open_list.ptr());
			}
			else {
				sorter.push_heap(0, open_list.find(e), 0, e, open_list.ptr());
			}
		}
	}

	return found_route;
}

real_t AStarGrid2D::_estimate_cost(const Vector2i& p_from_id, const Vector2i& p_end_id)
{
	return heuristics[default_estimate_heuristic](p_from_id, p_end_id);
}

real_t AStarGrid2D::_compute_cost(const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return heuristics[default_compute_heuristic](p_from_id, p_to_id);
}

void AStarGrid2D::clear()
{
	points.clear();
	region = Rect2i();
}

Vector2 AStarGrid2D::get_point_position(const Vector2i& p_id) const
{
	ERR_FAIL_COND_V_MSG(dirty, Vector2(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_id), Vector2(),
		vformat("Can't get point's position. Point %s out of bounds %s.", p_id, region));
	return _get_point_unchecked(p_id)->pos;
}

Vector<Vector2> AStarGrid2D::get_point_path(
	const Vector2i& p_from_id, const Vector2i& p_to_id, bool p_allow_partial_path)
{
	ERR_FAIL_COND_V_MSG(
		dirty, Vector<Vector2>(), "Grid is not initialized. Call the update method.");
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_from_id), Vector<Vector2>(),
		vformat("Can't get id path. Point %s out of bounds %s.", p_from_id, region));
	ERR_FAIL_COND_V_MSG(!is_in_boundsv(p_to_id), Vector<Vector2>(),
		vformat("Can't get id path. Point %s out of bounds %s.", p_to_id, region));

	Point* begin_point = _get_point(p_from_id.x, p_from_id.y);
	Point* end_point = _get_point(p_to_id.x, p_to_id.y);

	bool found_route = _solve(begin_point, end_point, p_allow_partial_path);
	if (!found_route) {
		if (!p_allow_partial_path || last_closest_point == nullptr) {
			return Vector<Vector2>();
		}

		// Use closest point instead.
		end_point = last_closest_point;
	}

	Point* p = end_point;
	int32_t pc = 1;
	while (p != begin_point) {
		pc++;
		p = p->prev_point;
	}

	Vector<Vector2> path;
	path.resize(pc);

	{
		Vector2* w = path.ptrw();

		p = end_point;
		int32_t idx = pc - 1;
		while (p != begin_point) {
			w[idx--] = p->pos;
			p = p->prev_point;
		}

		w[0] = p->pos;
	}

	return path;
}



Vector<Vector2i> AStarGrid2D::get_id_path(Vector2i const&, Vector2i const&, bool) {}

// --- a_star_grid_2d.compat.inc ---

#ifndef DISABLE_DEPRECATED


Vector<Vector2i> AStarGrid2D::_get_id_path_bind_compat_88047(
	const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return get_id_path(p_from_id, p_to_id, false);
}

Vector<Vector2> AStarGrid2D::_get_point_path_bind_compat_88047(
	const Vector2i& p_from_id, const Vector2i& p_to_id)
{
	return get_point_path(p_from_id, p_to_id, false);
}

void AStarGrid2D::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from core/math/geometry_2d.cpp
// ============================================================================

#include "core/math/geometry_2d.h"

#include "core/types.h"

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Walloc-zero")
#include <thirdparty/clipper2/include/clipper2/clipper.h>
VLTR_GCC_WARNING_POP
#include <thirdparty/misc/polypartition.h>
#define STB_RECT_PACK_IMPLEMENTATION
#include <thirdparty/misc/stb_rect_pack.h>

const int clipper_precision = 5; // Based on CMP_EPSILON.

void Geometry2D::merge_many_polygons(const Vector<Vector<Vector2>> &p_polygons, Vector<Vector<Vector2>> &r_out_polygons, Vector<Vector<Vector2>> &r_out_holes) {
	using namespace Clipper2Lib;

	PathsD subjects;
	for (const Vector<Vector2> &polygon : p_polygons) {
		PathD path(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			const Vector2 &point = polygon[i];
			path[i] = PointD(point.x, point.y);
		}
		subjects.push_back(path);
	}

	PathsD solution = Union(subjects, FillRule::NonZero);
	solution = SimplifyPaths(solution, 0.01);

	r_out_polygons.clear();
	r_out_holes.clear();
	for (PathsD::size_type i = 0; i < solution.size(); ++i) {
		PathD &path = solution[i];

		Vector<Point2> output_polygon;
		output_polygon.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			output_polygon.set(j, Vector2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		if (IsPositive(path)) {
			r_out_polygons.push_back(output_polygon);
		} else {
			r_out_holes.push_back(output_polygon);
		}
	}
}

Vector<Vector<Vector2>> Geometry2D::decompose_many_polygons_in_convex(const Vector<Vector<Point2>> &p_polygons, const Vector<Vector<Point2>> &p_holes) {
	Vector<Vector<Vector2>> decomp;
	List<TPPLPoly> in_poly, out_poly;

	for (const Vector<Vector2> &polygon : p_polygons) {
		TPPLPoly inp;
		inp.Init(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			inp.GetPoint(i) = polygon[i];
		}
		inp.SetOrientation(TPPL_ORIENTATION_CCW);
		in_poly.push_back(inp);
	}
	for (const Vector<Vector2> &polygon : p_holes) {
		TPPLPoly inp;
		inp.Init(polygon.size());
		for (int i = 0; i < polygon.size(); i++) {
			inp.GetPoint(i) = polygon[i];
		}
		inp.SetOrientation(TPPL_ORIENTATION_CW);
		inp.SetHole(true);
		in_poly.push_back(inp);
	}
	TPPLPartition tpart;
	if (tpart.ConvexPartition_HM(&in_poly, &out_poly) == 0) { // Failed.
		ERR_PRINT("Convex decomposing failed!");
		return decomp;
	}

	decomp.resize(out_poly.size());
	int idx = 0;
	for (TPPLPoly &tp : out_poly) {
		decomp.write[idx].resize(tp.GetNumPoints());

		for (int64_t i = 0; i < tp.GetNumPoints(); i++) {
			decomp.write[idx].write[i] = tp.GetPoint(i);
		}

		idx++;
	}

	return decomp;
}

Vector<Vector<Vector2>> Geometry2D::decompose_polygon_in_convex(const Vector<Point2> &p_polygon) {
	return Geometry2D::decompose_many_polygons_in_convex({ p_polygon }, {});
}

struct _AtlasWorkRect {
	Size2i s;
	Point2i p;
	int idx = 0;
	_FORCE_INLINE_ bool operator<(const _AtlasWorkRect &p_r) const { return s.width > p_r.s.width; }
};

struct _AtlasWorkRectResult {
	Vector<_AtlasWorkRect> result;
	int max_w = 0;
	int max_h = 0;
};

void Geometry2D::make_atlas(const Vector<Size2i> &p_rects, Vector<Point2i> &r_result, Size2i &r_size) {
	// Super simple, almost brute force scanline stacking fitter.
	// It's pretty basic for now, but it tries to make sure that the aspect ratio of the
	// resulting atlas is somehow square. This is necessary because video cards have limits
	// on texture size (usually 2048 or 4096), so the squarer a texture, the more the chances
	// that it will work in every hardware.
	// For example, it will prioritize a 1024x1024 atlas (works everywhere) instead of a
	// 256x8192 atlas (won't work anywhere).

	ERR_FAIL_COND(p_rects.is_empty());
	for (int i = 0; i < p_rects.size(); i++) {
		ERR_FAIL_COND(p_rects[i].width <= 0);
		ERR_FAIL_COND(p_rects[i].height <= 0);
	}

	Vector<_AtlasWorkRect> wrects;
	wrects.resize(p_rects.size());
	for (int i = 0; i < p_rects.size(); i++) {
		wrects.write[i].s = p_rects[i];
		wrects.write[i].idx = i;
	}
	wrects.sort();
	int widest = wrects[0].s.width;

	Vector<_AtlasWorkRectResult> results;

	for (int i = 0; i <= 12; i++) {
		int w = 1 << i;
		int max_h = 0;
		int max_w = 0;
		if (w < widest) {
			continue;
		}

		Vector<int> hmax;
		hmax.resize(w);
		for (int j = 0; j < w; j++) {
			hmax.write[j] = 0;
		}

		// Place them.
		int ofs = 0;
		int limit_h = 0;
		for (int j = 0; j < wrects.size(); j++) {
			if (ofs + wrects[j].s.width > w) {
				ofs = 0;
			}

			int from_y = 0;
			for (int k = 0; k < wrects[j].s.width; k++) {
				if (hmax[ofs + k] > from_y) {
					from_y = hmax[ofs + k];
				}
			}

			wrects.write[j].p.x = ofs;
			wrects.write[j].p.y = from_y;
			int end_h = from_y + wrects[j].s.height;
			int end_w = ofs + wrects[j].s.width;
			if (ofs == 0) {
				limit_h = end_h;
			}

			for (int k = 0; k < wrects[j].s.width; k++) {
				hmax.write[ofs + k] = end_h;
			}

			if (end_h > max_h) {
				max_h = end_h;
			}

			if (end_w > max_w) {
				max_w = end_w;
			}

			if (ofs == 0 || end_h > limit_h) { // While h limit not reached, keep stacking.
				ofs += wrects[j].s.width;
			}
		}

		_AtlasWorkRectResult result;
		result.result = wrects;
		result.max_h = max_h;
		result.max_w = max_w;
		results.push_back(result);
	}

	// Find the result with the best aspect ratio.

	int best = -1;
	real_t best_aspect = 1e20;

	for (int i = 0; i < results.size(); i++) {
		real_t h = Math::next_power_of_2((uint32_t)results[i].max_h);
		real_t w = Math::next_power_of_2((uint32_t)results[i].max_w);
		real_t aspect = h > w ? h / w : w / h;
		if (aspect < best_aspect) {
			best = i;
			best_aspect = aspect;
		}
	}

	r_result.resize(p_rects.size());

	for (int i = 0; i < p_rects.size(); i++) {
		r_result.write[results[best].result[i].idx] = results[best].result[i].p;
	}

	r_size = Size2(results[best].max_w, results[best].max_h);
}

Vector<Vector<Point2>> Geometry2D::_polypaths_do_operation(PolyBooleanOperation p_op, const Vector<Point2> &p_polypath_a, const Vector<Point2> &p_polypath_b, bool p_is_a_open) {
	using namespace Clipper2Lib;

	ClipType op = ClipType::Union;

	switch (p_op) {
		case OPERATION_UNION:
			op = ClipType::Union;
			break;
		case OPERATION_DIFFERENCE:
			op = ClipType::Difference;
			break;
		case OPERATION_INTERSECTION:
			op = ClipType::Intersection;
			break;
		case OPERATION_XOR:
			op = ClipType::Xor;
			break;
	}

	PathD path_a(p_polypath_a.size());
	for (int i = 0; i != p_polypath_a.size(); ++i) {
		path_a[i] = PointD(p_polypath_a[i].x, p_polypath_a[i].y);
	}
	PathD path_b(p_polypath_b.size());
	for (int i = 0; i != p_polypath_b.size(); ++i) {
		path_b[i] = PointD(p_polypath_b[i].x, p_polypath_b[i].y);
	}

	ClipperD clp(clipper_precision); // Scale points up internally to attain the desired precision.
	clp.PreserveCollinear(false); // Remove redundant vertices.
	if (p_is_a_open) {
		clp.AddOpenSubject({ path_a });
	} else {
		clp.AddSubject({ path_a });
	}
	clp.AddClip({ path_b });

	PathsD paths;

	if (p_is_a_open) {
		PolyTreeD tree; // Needed to populate polylines.
		clp.Execute(op, FillRule::EvenOdd, tree, paths);
	} else {
		clp.Execute(op, FillRule::EvenOdd, paths); // Works on closed polygons only.
	}

	Vector<Vector<Point2>> polypaths;
	polypaths.resize(paths.size());
	for (PathsD::size_type i = 0; i < paths.size(); i++) {
		const PathD &path = paths[i];

		Vector<Vector2> polypath;
		polypath.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			polypath.set(j, Point2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		polypaths.set(i, polypath);
	}
	return polypaths;
}

Vector<Vector<Point2>> Geometry2D::_polypath_offset(const Vector<Point2> &p_polypath, real_t p_delta, PolyJoinType p_join_type, PolyEndType p_end_type) {
	using namespace Clipper2Lib;

	JoinType jt = JoinType::Square;

	switch (p_join_type) {
		case JOIN_SQUARE:
			jt = JoinType::Square;
			break;
		case JOIN_ROUND:
			jt = JoinType::Round;
			break;
		case JOIN_MITER:
			jt = JoinType::Miter;
			break;
	}

	EndType et = EndType::Polygon;

	switch (p_end_type) {
		case END_POLYGON:
			et = EndType::Polygon;
			break;
		case END_JOINED:
			et = EndType::Joined;
			break;
		case END_BUTT:
			et = EndType::Butt;
			break;
		case END_SQUARE:
			et = EndType::Square;
			break;
		case END_ROUND:
			et = EndType::Round;
			break;
	}

	PathD polypath(p_polypath.size());
	for (int i = 0; i != p_polypath.size(); ++i) {
		polypath[i] = PointD(p_polypath[i].x, p_polypath[i].y);
	}

	// Inflate/deflate.
	PathsD paths = InflatePaths({ polypath }, p_delta, jt, et, 2.0, clipper_precision, 0.25);

	Vector<Vector<Point2>> polypaths;
	polypaths.resize(paths.size());
	for (PathsD::size_type i = 0; i < paths.size(); ++i) {
		const PathD &path = paths[i];

		Vector<Vector2> polypath2;
		polypath2.resize(path.size());
		for (PathsD::size_type j = 0; j < path.size(); ++j) {
			polypath2.set(j, Point2(static_cast<real_t>(path[j].x), static_cast<real_t>(path[j].y)));
		}
		polypaths.set(i, polypath2);
	}
	return polypaths;
}

Vector<Vector3i> Geometry2D::partial_pack_rects(const Vector<Vector2i> &p_sizes, const Size2i &p_atlas_size) {
	Vector<stbrp_node> nodes;
	nodes.resize(p_atlas_size.width);
	memset(nodes.ptrw(), 0, sizeof(stbrp_node) * nodes.size());

	stbrp_context context;
	stbrp_init_target(&context, p_atlas_size.width, p_atlas_size.height, nodes.ptrw(), p_atlas_size.width);

	Vector<stbrp_rect> rects;
	rects.resize(p_sizes.size());

	for (int i = 0; i < p_sizes.size(); i++) {
		rects.write[i].id = i;
		rects.write[i].w = p_sizes[i].width;
		rects.write[i].h = p_sizes[i].height;
		rects.write[i].x = 0;
		rects.write[i].y = 0;
		rects.write[i].was_packed = 0;
	}

	stbrp_pack_rects(&context, rects.ptrw(), rects.size());

	Vector<Vector3i> ret;
	ret.resize(p_sizes.size());

	for (int i = 0; i < p_sizes.size(); i++) {
		ret.write[rects[i].id] = Vector3i(rects[i].x, rects[i].y, rects[i].was_packed != 0 ? 1 : 0);
	}

	return ret;
}

// --- triangulate.cpp ---

real_t Triangulate::get_area(const Vector<Vector2> &p_contour) {
	int n = p_contour.size();
	const Vector2 *c = &p_contour[0];

	real_t A = 0.0;

	for (int p = n - 1, q = 0; q < n; p = q++) {
		A += c[p].cross(c[q]);
	}
	return A * 0.5f;
}

/* `is_inside_triangle` decides if a point P is inside the triangle
 * defined by A, B, C. */
bool Triangulate::is_inside_triangle(real_t p_ax, real_t p_ay, real_t p_bx, real_t p_by, real_t p_cx, real_t p_cy, real_t p_px, real_t p_py, bool p_include_edges) {
	real_t ax, ay, bx, by, cx, cy, apx, apy, bpx, bpy, cpx, cpy;
	real_t cCROSSap, bCROSScp, aCROSSbp;

	ax = p_cx - p_bx;
	ay = p_cy - p_by;
	bx = p_ax - p_cx;
	by = p_ay - p_cy;
	cx = p_bx - p_ax;
	cy = p_by - p_ay;
	apx = p_px - p_ax;
	apy = p_py - p_ay;
	bpx = p_px - p_bx;
	bpy = p_py - p_by;
	cpx = p_px - p_cx;
	cpy = p_py - p_cy;

	aCROSSbp = ax * bpy - ay * bpx;
	cCROSSap = cx * apy - cy * apx;
	bCROSScp = bx * cpy - by * cpx;

	if (p_include_edges) {
		return ((aCROSSbp > 0.0f) && (bCROSScp > 0.0f) && (cCROSSap > 0.0f));
	} else {
		return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
	}
}

bool Triangulate::snip(const Vector<Vector2> &p_contour, int p_u, int p_v, int p_w, int p_n, const Vector<int> &p_values, bool p_relaxed) {
	int p;
	real_t Ax, Ay, Bx, By, Cx, Cy, Px, Py;
	const Vector2 *contour = &p_contour[0];

	Ax = contour[p_values[p_u]].x;
	Ay = contour[p_values[p_u]].y;

	Bx = contour[p_values[p_v]].x;
	By = contour[p_values[p_v]].y;

	Cx = contour[p_values[p_w]].x;
	Cy = contour[p_values[p_w]].y;

	// It can happen that the triangulation ends up with three aligned vertices to deal with.
	// In this scenario, making the check below strict may reject the possibility of
	// forming a last triangle with these aligned vertices, preventing the triangulation
	// from completing.
	// To avoid that we allow zero-area triangles if all else failed.
	float threshold = p_relaxed ? -CMP_EPSILON : CMP_EPSILON;

	if (threshold > (((Bx - Ax) * (Cy - Ay)) - ((By - Ay) * (Cx - Ax)))) {
		return false;
	}

	for (p = 0; p < p_n; p++) {
		if ((p == p_u) || (p == p_v) || (p == p_w)) {
			continue;
		}
		Px = contour[p_values[p]].x;
		Py = contour[p_values[p]].y;
		if (is_inside_triangle(Ax, Ay, Bx, By, Cx, Cy, Px, Py, p_relaxed)) {
			return false;
		}
	}

	return true;
}

bool Triangulate::triangulate(const Vector<Vector2> &p_contour, Vector<int> &r_result) {
	/* allocate and initialize list of Vertices in polygon */

	int n = p_contour.size();
	if (n < 3) {
		return false;
	}

	Vector<int> V;
	V.resize(n);

	/* we want a counter-clockwise polygon in V */

	if (0.0f < get_area(p_contour)) {
		for (int v = 0; v < n; v++) {
			V.write[v] = v;
		}
	} else {
		for (int v = 0; v < n; v++) {
			V.write[v] = (n - 1) - v;
		}
	}

	bool relaxed = false;

	int nv = n;

	/*  remove nv-2 Vertices, creating 1 triangle every time */
	int count = 2 * nv; /* error detection */

	for (int v = nv - 1; nv > 2;) {
		/* if we loop, it is probably a non-simple polygon */
		if (0 >= (count--)) {
			if (relaxed) {
				//** Triangulate: ERROR - probable bad polygon!
				return false;
			} else {
				// There may be aligned vertices that the strict
				// checks prevent from triangulating. In this situation
				// we are better off adding flat triangles than
				// failing, so we relax the checks and try one last
				// round.
				// Only relaxing the constraints as a last resort avoids
				// degenerate triangles when they aren't necessary.
				count = 2 * nv;
				relaxed = true;
			}
		}

		/* three consecutive vertices in current polygon, <u,v,w> */
		int u = v;
		if (nv <= u) {
			u = 0; /* previous */
		}
		v = u + 1;
		if (nv <= v) {
			v = 0; /* new v    */
		}
		int w = v + 1;
		if (nv <= w) {
			w = 0; /* next     */
		}

		if (snip(p_contour, u, v, w, nv, V, relaxed)) {
			int a, b, c, s, t;

			/* true names of the vertices */
			a = V[u];
			b = V[v];
			c = V[w];

			/* output Triangle */
			r_result.push_back(a);
			r_result.push_back(b);
			r_result.push_back(c);

			/* remove v from remaining polygon */
			for (s = v, t = v + 1; t < nv; s++, t++) {
				V.write[s] = V[t];
			}

			nv--;

			/* reset error detection counter */
			count = 2 * nv;
		}
	}

	return true;
}


// --- Merged from core/math/convex_hull.cpp ---
#include "core/math/geometry_3d.h"

#include "core/error/error_macros.h"
#include "core/math/aabb.h"
#include "core/math/math_defs.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/types.h"

#include <cfloat> // FLT_MAX

//#define DEBUG_CONVEX_HULL
//#define SHOW_ITERATIONS

// -- GODOT start --
// Assembly optimizations are not used at the moment.
//#define USE_X86_64_ASM
// -- GODOT end --

#ifdef DEBUG_ENABLED
#define CHULL_ASSERT(m_cond) \
	if constexpr (true) { \
		if (unlikely(!(m_cond))) { \
			ERR_PRINT("Assertion \"" _STR(m_cond) "\" failed."); \
		} \
	} else \
		((void)0)
#else
#define CHULL_ASSERT(m_cond) \
	if constexpr (true) { \
	} else \
		((void)0)
#endif

#if defined(DEBUG_CONVEX_HULL) || defined(SHOW_ITERATIONS)
#include <cstdio>
#endif

// Convex hull implementation based on Preparata and Hong
// Ole Kniemeyer, MAXON Computer GmbH
class ConvexHullInternal {
public:
	class Point64 {
	public:
		int64_t x;
		int64_t y;
		int64_t z;

		Point64(int64_t p_x, int64_t p_y, int64_t p_z) {
			x = p_x;
			y = p_y;
			z = p_z;
		}

		bool is_zero() {
			return (x == 0) && (y == 0) && (z == 0);
		}

		int64_t dot(const Point64 &p_other) const {
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}
	};

	class Point32 {
	public:
		int32_t x = 0;
		int32_t y = 0;
		int32_t z = 0;
		int32_t index = -1;

		Point32() {
		}

		Point32(int32_t p_x, int32_t p_y, int32_t p_z) {
			x = p_x;
			y = p_y;
			z = p_z;
		}

		bool operator==(const Point32 &p_other) const {
			return (x == p_other.x) && (y == p_other.y) && (z == p_other.z);
		}

		bool operator!=(const Point32 &p_other) const {
			return (x != p_other.x) || (y != p_other.y) || (z != p_other.z);
		}

		bool is_zero() {
			return (x == 0) && (y == 0) && (z == 0);
		}

		Point64 cross(const Point32 &p_other) const {
			return Point64((int64_t)y * p_other.z - (int64_t)z * p_other.y, (int64_t)z * p_other.x - (int64_t)x * p_other.z, (int64_t)x * p_other.y - (int64_t)y * p_other.x);
		}

		Point64 cross(const Point64 &p_other) const {
			return Point64(y * p_other.z - z * p_other.y, z * p_other.x - x * p_other.z, x * p_other.y - y * p_other.x);
		}

		int64_t dot(const Point32 &p_other) const {
			return (int64_t)x * p_other.x + (int64_t)y * p_other.y + (int64_t)z * p_other.z;
		}

		int64_t dot(const Point64 &p_other) const {
			return x * p_other.x + y * p_other.y + z * p_other.z;
		}

		Point32 operator+(const Point32 &p_other) const {
			return Point32(x + p_other.x, y + p_other.y, z + p_other.z);
		}

		Point32 operator-(const Point32 &p_other) const {
			return Point32(x - p_other.x, y - p_other.y, z - p_other.z);
		}
	};

	class Int128 {
	public:
		uint64_t low = 0;
		uint64_t high = 0;

		Int128() {
		}

		Int128(uint64_t p_low, uint64_t p_high) {
			low = p_low;
			high = p_high;
		}

		Int128(uint64_t p_low) {
			low = p_low;
			high = 0;
		}

		Int128(int64_t p_value) {
			low = p_value;
			if (p_value >= 0) {
				high = 0;
			} else {
				high = (uint64_t)-1LL;
			}
		}

		static Int128 mul(int64_t p_left, int64_t p_right);

		static Int128 mul(uint64_t p_left, uint64_t p_right);

		Int128 operator-() const {
			return Int128(uint64_t(-int64_t(low)), ~high + (low == 0));
		}

		Int128 operator+(const Int128 &p_other) const {
#ifdef USE_X86_64_ASM
			Int128 result;
			__asm__("addq %[bl], %[rl]\n\t"
					"adcq %[bh], %[rh]\n\t"
					: [rl] "=r"(result.low), [rh] "=r"(result.high)
					: "0"(low), "1"(high), [bl] "g"(p_other.low), [bh] "g"(p_other.high)
					: "cc");
			return result;
#else
			uint64_t lo = low + p_other.low;
			return Int128(lo, high + p_other.high + (lo < low));
#endif
		}

		Int128 operator-(const Int128 &p_other) const {
#ifdef USE_X86_64_ASM
			Int128 result;
			__asm__("subq %[bl], %[rl]\n\t"
					"sbbq %[bh], %[rh]\n\t"
					: [rl] "=r"(result.low), [rh] "=r"(result.high)
					: "0"(low), "1"(high), [bl] "g"(p_other.low), [bh] "g"(p_other.high)
					: "cc");
			return result;
#else
			return *this + -p_other;
#endif
		}

		Int128 &operator+=(const Int128 &p_other) {
#ifdef USE_X86_64_ASM
			__asm__("addq %[bl], %[rl]\n\t"
					"adcq %[bh], %[rh]\n\t"
					: [rl] "=r"(low), [rh] "=r"(high)
					: "0"(low), "1"(high), [bl] "g"(p_other.low), [bh] "g"(p_other.high)
					: "cc");
#else
			uint64_t lo = low + p_other.low;
			if (lo < low) {
				++high;
			}
			low = lo;
			high += p_other.high;
#endif
			return *this;
		}

		Int128 &operator++() {
			if (++low == 0) {
				++high;
			}
			return *this;
		}

		Int128 operator*(int64_t p_other) const;

		real_t to_scalar() const {
			return ((int64_t)high >= 0) ? real_t(high) * (real_t(0x100000000LL) * real_t(0x100000000LL)) + real_t(low) : -(-*this).to_scalar();
		}

		int32_t get_sign() const {
			return ((int64_t)high < 0) ? -1 : ((high || low) ? 1 : 0);
		}

		bool operator<(const Int128 &p_other) const {
			return (high < p_other.high) || ((high == p_other.high) && (low < p_other.low));
		}

		int32_t ucmp(const Int128 &p_other) const {
			if (high < p_other.high) {
				return -1;
			}
			if (high > p_other.high) {
				return 1;
			}
			if (low < p_other.low) {
				return -1;
			}
			if (low > p_other.low) {
				return 1;
			}
			return 0;
		}
	};

	class Rational64 {
	private:
		uint64_t numerator;
		uint64_t denominator;
		int32_t sign;

	public:
		Rational64(int64_t p_numerator, int64_t p_denominator) {
			if (p_numerator > 0) {
				sign = 1;
				numerator = (uint64_t)p_numerator;
			} else if (p_numerator < 0) {
				sign = -1;
				numerator = (uint64_t)-p_numerator;
			} else {
				sign = 0;
				numerator = 0;
			}
			if (p_denominator > 0) {
				denominator = (uint64_t)p_denominator;
			} else if (p_denominator < 0) {
				sign = -sign;
				denominator = (uint64_t)-p_denominator;
			} else {
				denominator = 0;
			}
		}

		bool is_negative_infinity() const {
			return (sign < 0) && (denominator == 0);
		}

		bool is_nan() const {
			return (sign == 0) && (denominator == 0);
		}

		int32_t compare(const Rational64 &p_other) const;

		real_t to_scalar() const {
			return sign * ((denominator == 0) ? FLT_MAX : (real_t)numerator / denominator);
		}
	};

	class Rational128 {
	private:
		Int128 numerator;
		Int128 denominator;
		int32_t sign;
		bool is_int_64;

	public:
		Rational128(int64_t p_value) {
			if (p_value > 0) {
				sign = 1;
				numerator = p_value;
			} else if (p_value < 0) {
				sign = -1;
				numerator = -p_value;
			} else {
				sign = 0;
				numerator = (uint64_t)0;
			}
			denominator = (uint64_t)1;
			is_int_64 = true;
		}

		Rational128(const Int128 &p_numerator, const Int128 &p_denominator) {
			sign = p_numerator.get_sign();
			if (sign >= 0) {
				numerator = p_numerator;
			} else {
				numerator = -p_numerator;
			}
			int32_t dsign = p_denominator.get_sign();
			if (dsign >= 0) {
				denominator = p_denominator;
			} else {
				sign = -sign;
				denominator = -p_denominator;
			}
			is_int_64 = false;
		}

		int32_t compare(const Rational128 &p_other) const;

		int32_t compare(int64_t p_int64) const;

		real_t to_scalar() const {
			return sign * ((denominator.get_sign() == 0) ? FLT_MAX : numerator.to_scalar() / denominator.to_scalar());
		}
	};

	class PointR128 {
	public:
		Int128 x;
		Int128 y;
		Int128 z;
		Int128 denominator;

		PointR128() {
		}

		PointR128(Int128 p_x, Int128 p_y, Int128 p_z, Int128 p_denominator) {
			x = p_x;
			y = p_y;
			z = p_z;
			denominator = p_denominator;
		}

		real_t xvalue() const {
			return x.to_scalar() / denominator.to_scalar();
		}

		real_t yvalue() const {
			return y.to_scalar() / denominator.to_scalar();
		}

		real_t zvalue() const {
			return z.to_scalar() / denominator.to_scalar();
		}
	};

	class Edge;
	class Face;

	class Vertex {
	public:
		Vertex *next = nullptr;
		Vertex *prev = nullptr;
		Edge *edges = nullptr;
		Face *first_nearby_face = nullptr;
		Face *last_nearby_face = nullptr;
		PointR128 point128;
		Point32 point;
		int32_t copy = -1;

		Vertex() {
		}

#ifdef DEBUG_CONVEX_HULL
		void print() {
			printf("V%d (%d, %d, %d)", point.index, point.x, point.y, point.z);
		}

		void print_graph();
#endif

		Point32 operator-(const Vertex &p_other) const {
			return point - p_other.point;
		}

		Rational128 dot(const Point64 &p_other) const {
			return (point.index >= 0) ? Rational128(point.dot(p_other)) : Rational128(point128.x * p_other.x + point128.y * p_other.y + point128.z * p_other.z, point128.denominator);
		}

		real_t xvalue() const {
			return (point.index >= 0) ? real_t(point.x) : point128.xvalue();
		}

		real_t yvalue() const {
			return (point.index >= 0) ? real_t(point.y) : point128.yvalue();
		}

		real_t zvalue() const {
			return (point.index >= 0) ? real_t(point.z) : point128.zvalue();
		}

		void receive_nearby_faces(Vertex *p_src) {
			if (last_nearby_face) {
				last_nearby_face->next_with_same_nearby_vertex = p_src->first_nearby_face;
			} else {
				first_nearby_face = p_src->first_nearby_face;
			}
			if (p_src->last_nearby_face) {
				last_nearby_face = p_src->last_nearby_face;
			}
			for (Face *f = p_src->first_nearby_face; f; f = f->next_with_same_nearby_vertex) {
				CHULL_ASSERT(f->nearby_vertex == p_src);
				f->nearby_vertex = this;
			}
			p_src->first_nearby_face = nullptr;
			p_src->last_nearby_face = nullptr;
		}
	};

	class Edge {
	public:
		Edge *next = nullptr;
		Edge *prev = nullptr;
		Edge *reverse = nullptr;
		Vertex *target = nullptr;
		Face *face = nullptr;
		int32_t copy = -1;

		void link(Edge *p_edge) {
			CHULL_ASSERT(reverse->target == p_edge->reverse->target);
			next = p_edge;
			p_edge->prev = this;
		}

#ifdef DEBUG_CONVEX_HULL
		void print() {
			printf("E%p : %d -> %d,  n=%p p=%p   (0 %d\t%d\t%d) -> (%d %d %d)", this, reverse->target->point.index, target->point.index, next, prev,
					reverse->target->point.x, reverse->target->point.y, reverse->target->point.z, target->point.x, target->point.y, target->point.z);
		}
#endif
	};

	class Face {
	public:
		Face *next = nullptr;
		Vertex *nearby_vertex = nullptr;
		Face *next_with_same_nearby_vertex = nullptr;
		Point32 origin;
		Point32 dir0;
		Point32 dir1;

		Face() {
		}

		void init(Vertex *p_a, const Vertex *p_b, const Vertex *p_c) {
			nearby_vertex = p_a;
			origin = p_a->point;
			dir0 = *p_b - *p_a;
			dir1 = *p_c - *p_a;
			if (p_a->last_nearby_face) {
				p_a->last_nearby_face->next_with_same_nearby_vertex = this;
			} else {
				p_a->first_nearby_face = this;
			}
			p_a->last_nearby_face = this;
		}

		Point64 get_normal() {
			return dir0.cross(dir1);
		}
	};

	template <typename UWord, typename UHWord>
	class DMul {
	private:
		static uint32_t high(uint64_t p_value) {
			return (uint32_t)(p_value >> 32);
		}

		static uint32_t low(uint64_t p_value) {
			return (uint32_t)p_value;
		}

		static uint64_t mul(uint32_t p_left, uint32_t p_right) {
			return (uint64_t)p_left * (uint64_t)p_right;
		}

		static void shl_half(uint64_t &p_value) {
			p_value <<= 32;
		}

		static uint64_t high(Int128 p_value) {
			return p_value.high;
		}

		static uint64_t low(Int128 p_value) {
			return p_value.low;
		}

		static Int128 mul(uint64_t p_left, uint64_t p_right) {
			return Int128::mul(p_left, p_right);
		}

		static void shl_half(Int128 &p_value) {
			p_value.high = p_value.low;
			p_value.low = 0;
		}

	public:
		static void mul(UWord p_a, UWord p_b, UWord &r_low, UWord &r_high) {
			UWord p00 = mul(low(p_a), low(p_b));
			UWord p01 = mul(low(p_a), high(p_b));
			UWord p10 = mul(high(p_a), low(p_b));
			UWord p11 = mul(high(p_a), high(p_b));
			UWord p0110 = UWord(low(p01)) + UWord(low(p10));
			p11 += high(p01);
			p11 += high(p10);
			p11 += high(p0110);
			shl_half(p0110);
			p00 += p0110;
			if (p00 < p0110) {
				++p11;
			}
			r_low = p00;
			r_high = p11;
		}
	};

private:
	class IntermediateHull {
	public:
		Vertex *min_xy = nullptr;
		Vertex *max_xy = nullptr;
		Vertex *min_yx = nullptr;
		Vertex *max_yx = nullptr;

		IntermediateHull() {
		}
	};

	enum Orientation { ORIENTATION_NONE,
		ORIENTATION_CLOCKWISE,
		ORIENTATION_COUNTER_CLOCKWISE };

	Vector3 scaling;
	Vector3 center;
	PagedAllocator<Vertex> vertex_pool;
	PagedAllocator<Edge> edge_pool;
	PagedAllocator<Face> face_pool;
	LocalVector<Vertex *> original_vertices;
	int32_t merge_stamp = 0;
	Vector3::Axis min_axis = Vector3::Axis::AXIS_X;
	Vector3::Axis med_axis = Vector3::Axis::AXIS_X;
	Vector3::Axis max_axis = Vector3::Axis::AXIS_X;
	int32_t used_edge_pairs = 0;
	int32_t max_used_edge_pairs = 0;

	static Orientation get_orientation(const Edge *p_prev, const Edge *p_next, const Point32 &p_s, const Point32 &p_t);
	Edge *find_max_angle(bool p_ccw, const Vertex *p_start, const Point32 &p_s, const Point64 &p_rxs, const Point64 &p_ssxrxs, Rational64 &p_min_cot);
	void find_edge_for_coplanar_faces(Vertex *p_c0, Vertex *p_c1, Edge *&p_e0, Edge *&p_e1, const Vertex *p_stop0, const Vertex *p_stop1);

	Edge *new_edge_pair(Vertex *p_from, Vertex *p_to);

	void remove_edge_pair(Edge *p_edge) {
		Edge *n = p_edge->next;
		Edge *r = p_edge->reverse;

		CHULL_ASSERT(p_edge->target && r->target);

		if (n != p_edge) {
			n->prev = p_edge->prev;
			p_edge->prev->next = n;
			r->target->edges = n;
		} else {
			r->target->edges = nullptr;
		}

		n = r->next;

		if (n != r) {
			n->prev = r->prev;
			r->prev->next = n;
			p_edge->target->edges = n;
		} else {
			p_edge->target->edges = nullptr;
		}

		edge_pool.free(p_edge);
		edge_pool.free(r);
		used_edge_pairs--;
	}

	void compute_internal(int32_t p_start, int32_t p_end, IntermediateHull &r_result);

	bool merge_projection(IntermediateHull &p_h0, IntermediateHull &p_h1, Vertex *&r_c0, Vertex *&r_c1);

	void merge(IntermediateHull &p_h0, IntermediateHull &p_h1);

	Vector3 to_gd_vector(const Point32 &p_v);

	Vector3 get_gd_normal(Face *p_face);

	bool shift_face(Face *p_face, real_t p_amount, LocalVector<Vertex *> &p_stack);

public:
	~ConvexHullInternal() {
		vertex_pool.reset(true);
		edge_pool.reset(true);
		face_pool.reset(true);
	}

	Vertex *vertex_list = nullptr;

	void compute(const Vector3 *p_coords, int32_t p_count);

	Vector3 get_coordinates(const Vertex *p_v);

	real_t shrink(real_t p_amount, real_t p_clamp_amount);
};

ConvexHullInternal::Int128 ConvexHullInternal::Int128::operator*(int64_t p_other) const {
	bool negative = (int64_t)high < 0;
	Int128 a = negative ? -*this : *this;
	if (p_other < 0) {
		negative = !negative;
		p_other = -p_other;
	}
	Int128 result = mul(a.low, (uint64_t)p_other);
	result.high += a.high * (uint64_t)p_other;
	return negative ? -result : result;
}

ConvexHullInternal::Int128 ConvexHullInternal::Int128::mul(int64_t p_left, int64_t p_right) {
	Int128 result;

#ifdef USE_X86_64_ASM
	__asm__("imulq %[b]"
			: "=a"(result.low), "=d"(result.high)
			: "0"(p_left), [p_right] "r"(p_right)
			: "cc");
	return result;

#else
	bool negative = p_left < 0;
	if (negative) {
		p_left = -p_left;
	}
	if (p_right < 0) {
		negative = !negative;
		p_right = -p_right;
	}
	DMul<uint64_t, uint32_t>::mul((uint64_t)p_left, (uint64_t)p_right, result.low, result.high);
	return negative ? -result : result;
#endif
}

ConvexHullInternal::Int128 ConvexHullInternal::Int128::mul(uint64_t p_left, uint64_t p_right) {
	Int128 result;

#ifdef USE_X86_64_ASM
	__asm__("mulq %[b]"
			: "=a"(result.low), "=d"(result.high)
			: "0"(p_left), [p_right] "r"(p_right)
			: "cc");

#else
	DMul<uint64_t, uint32_t>::mul(p_left, p_right, result.low, result.high);
#endif

	return result;
}

int32_t ConvexHullInternal::Rational64::compare(const Rational64 &p_other) const {
	if (sign != p_other.sign) {
		return sign - p_other.sign;
	} else if (sign == 0) {
		return 0;
	}

#ifdef USE_X86_64_ASM

	int32_t result;
	int64_t tmp;
	int64_t dummy;
	__asm__("mulq %[bn]\n\t"
			"movq %%rax, %[tmp]\n\t"
			"movq %%rdx, %%rbx\n\t"
			"movq %[tn], %%rax\n\t"
			"mulq %[bd]\n\t"
			"subq %[tmp], %%rax\n\t"
			"sbbq %%rbx, %%rdx\n\t" // rdx:rax contains 128-bit-difference "numerator*b.denominator - b.numerator*denominator"
			"setnsb %%bh\n\t" // bh=1 if difference is non-negative, bh=0 otherwise
			"orq %%rdx, %%rax\n\t"
			"setnzb %%bl\n\t" // bl=1 if difference if non-zero, bl=0 if it is zero
			"decb %%bh\n\t" // now bx=0x0000 if difference is zero, 0xff01 if it is negative, 0x0001 if it is positive (i.e., same sign as difference)
			"shll $16, %%ebx\n\t" // ebx has same sign as difference
			: "=&b"(result), [tmp] "=&r"(tmp), "=a"(dummy)
			: "a"(denominator), [bn] "g"(p_other.numerator), [tn] "g"(numerator), [bd] "g"(p_other.denominator)
			: "%rdx", "cc");
	// if sign is +1, only bit 0 of result is inverted, which does not change the sign of result (and cannot result in zero)
	// if sign is -1, all bits of result are inverted, which changes the sign of result (and again cannot result in zero)
	return result ? result ^ sign : 0;

#else

	return sign * Int128::mul(numerator, p_other.denominator).ucmp(Int128::mul(denominator, p_other.numerator));

#endif
}

int32_t ConvexHullInternal::Rational128::compare(const Rational128 &p_other) const {
	if (sign != p_other.sign) {
		return sign - p_other.sign;
	} else if (sign == 0) {
		return 0;
	}
	if (is_int_64) {
		return -p_other.compare(sign * (int64_t)numerator.low);
	}

	Int128 nbd_low, nbd_high, dbn_low, dbn_high;
	DMul<Int128, uint64_t>::mul(numerator, p_other.denominator, nbd_low, nbd_high);
	DMul<Int128, uint64_t>::mul(denominator, p_other.numerator, dbn_low, dbn_high);

	int32_t cmp = nbd_high.ucmp(dbn_high);
	if (cmp) {
		return cmp * sign;
	}
	return nbd_low.ucmp(dbn_low) * sign;
}

int32_t ConvexHullInternal::Rational128::compare(int64_t p_int64) const {
	if (is_int_64) {
		int64_t a = sign * (int64_t)numerator.low;
		return (a > p_int64) ? 1 : ((a < p_int64) ? -1 : 0);
	}
	if (p_int64 > 0) {
		if (sign <= 0) {
			return -1;
		}
	} else if (p_int64 < 0) {
		if (sign >= 0) {
			return 1;
		}
		p_int64 = -p_int64;
	} else {
		return sign;
	}

	return numerator.ucmp(denominator * p_int64) * sign;
}

ConvexHullInternal::Edge *ConvexHullInternal::new_edge_pair(Vertex *p_from, Vertex *p_to) {
	CHULL_ASSERT(p_from && p_to);
	Edge *e = edge_pool.alloc();
	Edge *r = edge_pool.alloc();
	e->reverse = r;
	r->reverse = e;
	e->copy = merge_stamp;
	r->copy = merge_stamp;
	e->target = p_to;
	r->target = p_from;
	e->face = nullptr;
	r->face = nullptr;
	used_edge_pairs++;
	if (used_edge_pairs > max_used_edge_pairs) {
		max_used_edge_pairs = used_edge_pairs;
	}
	return e;
}

bool ConvexHullInternal::merge_projection(IntermediateHull &r_h0, IntermediateHull &r_h1, Vertex *&r_c0, Vertex *&r_c1) {
	Vertex *v0 = r_h0.max_yx;
	Vertex *v1 = r_h1.min_yx;
	if ((v0->point.x == v1->point.x) && (v0->point.y == v1->point.y)) {
		CHULL_ASSERT(v0->point.z < v1->point.z);
		Vertex *v1p = v1->prev;
		if (v1p == v1) {
			r_c0 = v0;
			if (v1->edges) {
				CHULL_ASSERT(v1->edges->next == v1->edges);
				v1 = v1->edges->target;
				CHULL_ASSERT(v1->edges->next == v1->edges);
			}
			r_c1 = v1;
			return false;
		}
		Vertex *v1n = v1->next;
		v1p->next = v1n;
		v1n->prev = v1p;
		if (v1 == r_h1.min_xy) {
			if ((v1n->point.x < v1p->point.x) || ((v1n->point.x == v1p->point.x) && (v1n->point.y < v1p->point.y))) {
				r_h1.min_xy = v1n;
			} else {
				r_h1.min_xy = v1p;
			}
		}
		if (v1 == r_h1.max_xy) {
			if ((v1n->point.x > v1p->point.x) || ((v1n->point.x == v1p->point.x) && (v1n->point.y > v1p->point.y))) {
				r_h1.max_xy = v1n;
			} else {
				r_h1.max_xy = v1p;
			}
		}
	}

	v0 = r_h0.max_xy;
	v1 = r_h1.max_xy;
	Vertex *v00 = nullptr;
	Vertex *v10 = nullptr;
	int32_t sign = 1;

	for (int32_t side = 0; side <= 1; side++) {
		int32_t dx = (v1->point.x - v0->point.x) * sign;
		if (dx > 0) {
			while (true) {
				int32_t dy = v1->point.y - v0->point.y;

				Vertex *w0 = side ? v0->next : v0->prev;
				if (w0 != v0) {
					int32_t dx0 = (w0->point.x - v0->point.x) * sign;
					int32_t dy0 = w0->point.y - v0->point.y;
					if ((dy0 <= 0) && ((dx0 == 0) || ((dx0 < 0) && (dy0 * dx <= dy * dx0)))) {
						v0 = w0;
						dx = (v1->point.x - v0->point.x) * sign;
						continue;
					}
				}

				Vertex *w1 = side ? v1->next : v1->prev;
				if (w1 != v1) {
					int32_t dx1 = (w1->point.x - v1->point.x) * sign;
					int32_t dy1 = w1->point.y - v1->point.y;
					int32_t dxn = (w1->point.x - v0->point.x) * sign;
					if ((dxn > 0) && (dy1 < 0) && ((dx1 == 0) || ((dx1 < 0) && (dy1 * dx < dy * dx1)))) {
						v1 = w1;
						dx = dxn;
						continue;
					}
				}

				break;
			}
		} else if (dx < 0) {
			while (true) {
				int32_t dy = v1->point.y - v0->point.y;

				Vertex *w1 = side ? v1->prev : v1->next;
				if (w1 != v1) {
					int32_t dx1 = (w1->point.x - v1->point.x) * sign;
					int32_t dy1 = w1->point.y - v1->point.y;
					if ((dy1 >= 0) && ((dx1 == 0) || ((dx1 < 0) && (dy1 * dx <= dy * dx1)))) {
						v1 = w1;
						dx = (v1->point.x - v0->point.x) * sign;
						continue;
					}
				}

				Vertex *w0 = side ? v0->prev : v0->next;
				if (w0 != v0) {
					int32_t dx0 = (w0->point.x - v0->point.x) * sign;
					int32_t dy0 = w0->point.y - v0->point.y;
					int32_t dxn = (v1->point.x - w0->point.x) * sign;
					if ((dxn < 0) && (dy0 > 0) && ((dx0 == 0) || ((dx0 < 0) && (dy0 * dx < dy * dx0)))) {
						v0 = w0;
						dx = dxn;
						continue;
					}
				}

				break;
			}
		} else {
			int32_t x = v0->point.x;
			int32_t y0 = v0->point.y;
			Vertex *w0 = v0;
			Vertex *t;
			while (((t = side ? w0->next : w0->prev) != v0) && (t->point.x == x) && (t->point.y <= y0)) {
				w0 = t;
				y0 = t->point.y;
			}
			v0 = w0;

			int32_t y1 = v1->point.y;
			Vertex *w1 = v1;
			while (((t = side ? w1->prev : w1->next) != v1) && (t->point.x == x) && (t->point.y >= y1)) {
				w1 = t;
				y1 = t->point.y;
			}
			v1 = w1;
		}

		if (side == 0) {
			v00 = v0;
			v10 = v1;

			v0 = r_h0.min_xy;
			v1 = r_h1.min_xy;
			sign = -1;
		}
	}

	v0->prev = v1;
	v1->next = v0;

	v00->next = v10;
	v10->prev = v00;

	if (r_h1.min_xy->point.x < r_h0.min_xy->point.x) {
		r_h0.min_xy = r_h1.min_xy;
	}
	if (r_h1.max_xy->point.x >= r_h0.max_xy->point.x) {
		r_h0.max_xy = r_h1.max_xy;
	}

	r_h0.max_yx = r_h1.max_yx;

	r_c0 = v00;
	r_c1 = v10;

	return true;
}

void ConvexHullInternal::compute_internal(int32_t p_start, int32_t p_end, IntermediateHull &r_result) {
	int32_t n = p_end - p_start;
	switch (n) {
		case 0:
			r_result.min_xy = nullptr;
			r_result.max_xy = nullptr;
			r_result.min_yx = nullptr;
			r_result.max_yx = nullptr;
			return;
		case 2: {
			Vertex *v = original_vertices[p_start];
			Vertex *w = original_vertices[p_start + 1];
			if (v->point != w->point) {
				int32_t dx = v->point.x - w->point.x;
				int32_t dy = v->point.y - w->point.y;

				if ((dx == 0) && (dy == 0)) {
					if (v->point.z > w->point.z) {
						Vertex *t = w;
						w = v;
						v = t;
					}
					CHULL_ASSERT(v->point.z < w->point.z);
					v->next = v;
					v->prev = v;
					r_result.min_xy = v;
					r_result.max_xy = v;
					r_result.min_yx = v;
					r_result.max_yx = v;
				} else {
					v->next = w;
					v->prev = w;
					w->next = v;
					w->prev = v;

					if ((dx < 0) || ((dx == 0) && (dy < 0))) {
						r_result.min_xy = v;
						r_result.max_xy = w;
					} else {
						r_result.min_xy = w;
						r_result.max_xy = v;
					}

					if ((dy < 0) || ((dy == 0) && (dx < 0))) {
						r_result.min_yx = v;
						r_result.max_yx = w;
					} else {
						r_result.min_yx = w;
						r_result.max_yx = v;
					}
				}

				Edge *e = new_edge_pair(v, w);
				e->link(e);
				v->edges = e;

				e = e->reverse;
				e->link(e);
				w->edges = e;

				return;
			}
			[[fallthrough]];
		}
		case 1: {
			Vertex *v = original_vertices[p_start];
			v->edges = nullptr;
			v->next = v;
			v->prev = v;

			r_result.min_xy = v;
			r_result.max_xy = v;
			r_result.min_yx = v;
			r_result.max_yx = v;

			return;
		}
	}

	int32_t split0 = p_start + n / 2;
	Point32 p = original_vertices[split0 - 1]->point;
	int32_t split1 = split0;
	while ((split1 < p_end) && (original_vertices[split1]->point == p)) {
		split1++;
	}
	compute_internal(p_start, split0, r_result);
	IntermediateHull hull1;
	compute_internal(split1, p_end, hull1);
#ifdef DEBUG_CONVEX_HULL
	printf("\n\nMerge\n");
	r_result.print();
	hull1.print();
#endif
	merge(r_result, hull1);
#ifdef DEBUG_CONVEX_HULL
	printf("\n  Result\n");
	r_result.print();
#endif
}

#ifdef DEBUG_CONVEX_HULL
void ConvexHullInternal::IntermediateHull::print() {
	printf("    Hull\n");
	for (Vertex *v = min_xy; v;) {
		printf("      ");
		v->print();
		if (v == max_xy) {
			printf(" max_xy");
		}
		if (v == min_yx) {
			printf(" min_yx");
		}
		if (v == max_yx) {
			printf(" max_yx");
		}
		if (v->next->prev != v) {
			printf(" Inconsistency");
		}
		printf("\n");
		v = v->next;
		if (v == min_xy) {
			break;
		}
	}
	if (min_xy) {
		min_xy->copy = (min_xy->copy == -1) ? -2 : -1;
		min_xy->print_graph();
	}
}

void ConvexHullInternal::Vertex::print_graph() {
	print();
	printf("\nEdges\n");
	Edge *e = edges;
	if (e) {
		do {
			e->print();
			printf("\n");
			e = e->next;
		} while (e != edges);
		do {
			Vertex *v = e->target;
			if (v->copy != copy) {
				v->copy = copy;
				v->print_graph();
			}
			e = e->next;
		} while (e != edges);
	}
}
#endif

ConvexHullInternal::Orientation ConvexHullInternal::get_orientation(const Edge *p_prev, const Edge *p_next, const Point32 &p_s, const Point32 &p_t) {
	CHULL_ASSERT(p_prev->reverse->target == p_next->reverse->target);
	if (p_prev->next == p_next) {
		if (p_prev->prev == p_next) {
			Point64 n = p_t.cross(p_s);
			Point64 m = (*p_prev->target - *p_next->reverse->target).cross(*p_next->target - *p_next->reverse->target);
			CHULL_ASSERT(!m.is_zero());
			int64_t dot = n.dot(m);
			CHULL_ASSERT(dot != 0);
			return (dot > 0) ? ORIENTATION_COUNTER_CLOCKWISE : ORIENTATION_CLOCKWISE;
		}
		return ORIENTATION_COUNTER_CLOCKWISE;
	} else if (p_prev->prev == p_next) {
		return ORIENTATION_CLOCKWISE;
	} else {
		return ORIENTATION_NONE;
	}
}

ConvexHullInternal::Edge *ConvexHullInternal::find_max_angle(bool p_ccw, const Vertex *p_start, const Point32 &p_s, const Point64 &p_rxs, const Point64 &p_sxrxs, Rational64 &p_min_cot) {
	Edge *min_edge = nullptr;

#ifdef DEBUG_CONVEX_HULL
	printf("find max edge for %d\n", p_start->point.index);
#endif
	Edge *e = p_start->edges;
	if (e) {
		do {
			if (e->copy > merge_stamp) {
				Point32 t = *e->target - *p_start;
				Rational64 cot(t.dot(p_sxrxs), t.dot(p_rxs));
#ifdef DEBUG_CONVEX_HULL
				printf("      Angle is %f (%d) for ", Math::atan(cot.to_scalar()), (int32_t)cot.is_nan());
				e->print();
#endif
				if (cot.is_nan()) {
					CHULL_ASSERT(p_ccw ? (t.dot(p_s) < 0) : (t.dot(p_s) > 0));
				} else {
					int32_t cmp;
					if (min_edge == nullptr) {
						p_min_cot = cot;
						min_edge = e;
					} else if ((cmp = cot.compare(p_min_cot)) < 0) {
						p_min_cot = cot;
						min_edge = e;
					} else if ((cmp == 0) && (p_ccw == (get_orientation(min_edge, e, p_s, t) == ORIENTATION_COUNTER_CLOCKWISE))) {
						min_edge = e;
					}
				}
#ifdef DEBUG_CONVEX_HULL
				printf("\n");
#endif
			}
			e = e->next;
		} while (e != p_start->edges);
	}
	return min_edge;
}

void ConvexHullInternal::find_edge_for_coplanar_faces(Vertex *p_c0, Vertex *p_c1, Edge *&p_e0, Edge *&p_e1, const Vertex *p_stop0, const Vertex *p_stop1) {
	Edge *start0 = p_e0;
	Edge *start1 = p_e1;
	Point32 et0 = start0 ? start0->target->point : p_c0->point;
	Point32 et1 = start1 ? start1->target->point : p_c1->point;
	Point32 s = p_c1->point - p_c0->point;
	Point64 normal = ((start0 ? start0 : start1)->target->point - p_c0->point).cross(s);
	int64_t dist = p_c0->point.dot(normal);
	CHULL_ASSERT(!start1 || (start1->target->point.dot(normal) == dist));
	Point64 perp = s.cross(normal);
	CHULL_ASSERT(!perp.is_zero());

#ifdef DEBUG_CONVEX_HULL
	printf("   Advancing %d %d  (%p %p, %d %d)\n", p_c0->point.index, p_c1->point.index, start0, start1, start0 ? start0->target->point.index : -1, start1 ? start1->target->point.index : -1);
#endif

	int64_t max_dot0 = et0.dot(perp);
	if (p_e0) {
		while (p_e0->target != p_stop0) {
			Edge *e = p_e0->reverse->prev;
			if (e->target->point.dot(normal) < dist) {
				break;
			}
			CHULL_ASSERT(e->target->point.dot(normal) == dist);
			if (e->copy == merge_stamp) {
				break;
			}
			int64_t dot = e->target->point.dot(perp);
			if (dot <= max_dot0) {
				break;
			}
			max_dot0 = dot;
			p_e0 = e;
			et0 = e->target->point;
		}
	}

	int64_t max_dot1 = et1.dot(perp);
	if (p_e1) {
		while (p_e1->target != p_stop1) {
			Edge *e = p_e1->reverse->next;
			if (e->target->point.dot(normal) < dist) {
				break;
			}
			CHULL_ASSERT(e->target->point.dot(normal) == dist);
			if (e->copy == merge_stamp) {
				break;
			}
			int64_t dot = e->target->point.dot(perp);
			if (dot <= max_dot1) {
				break;
			}
			max_dot1 = dot;
			p_e1 = e;
			et1 = e->target->point;
		}
	}

#ifdef DEBUG_CONVEX_HULL
	printf("   Starting at %d %d\n", et0.index, et1.index);
#endif

	int64_t dx = max_dot1 - max_dot0;
	if (dx > 0) {
		while (true) {
			int64_t dy = (et1 - et0).dot(s);

			if (p_e0 && (p_e0->target != p_stop0)) {
				Edge *f0 = p_e0->next->reverse;
				if (f0->copy > merge_stamp) {
					int64_t dx0 = (f0->target->point - et0).dot(perp);
					int64_t dy0 = (f0->target->point - et0).dot(s);
					if ((dx0 == 0) ? (dy0 < 0) : ((dx0 < 0) && (Rational64(dy0, dx0).compare(Rational64(dy, dx)) >= 0))) {
						et0 = f0->target->point;
						dx = (et1 - et0).dot(perp);
						p_e0 = (p_e0 == start0) ? nullptr : f0;
						continue;
					}
				}
			}

			if (p_e1 && (p_e1->target != p_stop1)) {
				Edge *f1 = p_e1->reverse->next;
				if (f1->copy > merge_stamp) {
					Point32 d1 = f1->target->point - et1;
					if (d1.dot(normal) == 0) {
						int64_t dx1 = d1.dot(perp);
						int64_t dy1 = d1.dot(s);
						int64_t dxn = (f1->target->point - et0).dot(perp);
						if ((dxn > 0) && ((dx1 == 0) ? (dy1 < 0) : ((dx1 < 0) && (Rational64(dy1, dx1).compare(Rational64(dy, dx)) > 0)))) {
							p_e1 = f1;
							et1 = p_e1->target->point;
							dx = dxn;
							continue;
						}
					} else {
						CHULL_ASSERT((p_e1 == start1) && (d1.dot(normal) < 0));
					}
				}
			}

			break;
		}
	} else if (dx < 0) {
		while (true) {
			int64_t dy = (et1 - et0).dot(s);

			if (p_e1 && (p_e1->target != p_stop1)) {
				Edge *f1 = p_e1->prev->reverse;
				if (f1->copy > merge_stamp) {
					int64_t dx1 = (f1->target->point - et1).dot(perp);
					int64_t dy1 = (f1->target->point - et1).dot(s);
					if ((dx1 == 0) ? (dy1 > 0) : ((dx1 < 0) && (Rational64(dy1, dx1).compare(Rational64(dy, dx)) <= 0))) {
						et1 = f1->target->point;
						dx = (et1 - et0).dot(perp);
						p_e1 = (p_e1 == start1) ? nullptr : f1;
						continue;
					}
				}
			}

			if (p_e0 && (p_e0->target != p_stop0)) {
				Edge *f0 = p_e0->reverse->prev;
				if (f0->copy > merge_stamp) {
					Point32 d0 = f0->target->point - et0;
					if (d0.dot(normal) == 0) {
						int64_t dx0 = d0.dot(perp);
						int64_t dy0 = d0.dot(s);
						int64_t dxn = (et1 - f0->target->point).dot(perp);
						if ((dxn < 0) && ((dx0 == 0) ? (dy0 > 0) : ((dx0 < 0) && (Rational64(dy0, dx0).compare(Rational64(dy, dx)) < 0)))) {
							p_e0 = f0;
							et0 = p_e0->target->point;
							dx = dxn;
							continue;
						}
					} else {
						CHULL_ASSERT((p_e0 == start0) && (d0.dot(normal) < 0));
					}
				}
			}

			break;
		}
	}
#ifdef DEBUG_CONVEX_HULL
	printf("   Advanced edges to %d %d\n", et0.index, et1.index);
#endif
}

void ConvexHullInternal::merge(IntermediateHull &p_h0, IntermediateHull &p_h1) {
	if (!p_h1.max_xy) {
		return;
	}
	if (!p_h0.max_xy) {
		p_h0 = p_h1;
		return;
	}

	merge_stamp--;

	Vertex *c0 = nullptr;
	Edge *to_prev0 = nullptr;
	Edge *first_new0 = nullptr;
	Edge *pending_head0 = nullptr;
	Edge *pending_tail0 = nullptr;
	Vertex *c1 = nullptr;
	Edge *to_prev1 = nullptr;
	Edge *first_new1 = nullptr;
	Edge *pending_head1 = nullptr;
	Edge *pending_tail1 = nullptr;
	Point32 prev_point;

	if (merge_projection(p_h0, p_h1, c0, c1)) {
		Point32 s = *c1 - *c0;
		Point64 normal = Point32(0, 0, -1).cross(s);
		Point64 t = s.cross(normal);
		CHULL_ASSERT(!t.is_zero());

		Edge *e = c0->edges;
		Edge *start0 = nullptr;
		if (e) {
			do {
				int64_t dot = (*e->target - *c0).dot(normal);
				CHULL_ASSERT(dot <= 0);
				if ((dot == 0) && ((*e->target - *c0).dot(t) > 0)) {
					if (!start0 || (get_orientation(start0, e, s, Point32(0, 0, -1)) == ORIENTATION_CLOCKWISE)) {
						start0 = e;
					}
				}
				e = e->next;
			} while (e != c0->edges);
		}

		e = c1->edges;
		Edge *start1 = nullptr;
		if (e) {
			do {
				int64_t dot = (*e->target - *c1).dot(normal);
				CHULL_ASSERT(dot <= 0);
				if ((dot == 0) && ((*e->target - *c1).dot(t) > 0)) {
					if (!start1 || (get_orientation(start1, e, s, Point32(0, 0, -1)) == ORIENTATION_COUNTER_CLOCKWISE)) {
						start1 = e;
					}
				}
				e = e->next;
			} while (e != c1->edges);
		}

		if (start0 || start1) {
			find_edge_for_coplanar_faces(c0, c1, start0, start1, nullptr, nullptr);
			if (start0) {
				c0 = start0->target;
			}
			if (start1) {
				c1 = start1->target;
			}
		}

		prev_point = c1->point;
		prev_point.z++;
	} else {
		prev_point = c1->point;
		prev_point.x++;
	}

	Vertex *first0 = c0;
	Vertex *first1 = c1;
	bool first_run = true;

	while (true) {
		Point32 s = *c1 - *c0;
		Point32 r = prev_point - c0->point;
		Point64 rxs = r.cross(s);
		Point64 sxrxs = s.cross(rxs);

#ifdef DEBUG_CONVEX_HULL
		printf("\n  Checking %d %d\n", c0->point.index, c1->point.index);
#endif
		Rational64 min_cot0(0, 0);
		Edge *min0 = find_max_angle(false, c0, s, rxs, sxrxs, min_cot0);
		Rational64 min_cot1(0, 0);
		Edge *min1 = find_max_angle(true, c1, s, rxs, sxrxs, min_cot1);
		if (!min0 && !min1) {
			Edge *e = new_edge_pair(c0, c1);
			e->link(e);
			c0->edges = e;

			e = e->reverse;
			e->link(e);
			c1->edges = e;
			return;
		} else {
			int32_t cmp = !min0 ? 1 : (!min1 ? -1 : min_cot0.compare(min_cot1));
#ifdef DEBUG_CONVEX_HULL
			printf("    -> Result %d\n", cmp);
#endif
			if (first_run || ((cmp >= 0) ? !min_cot1.is_negative_infinity() : !min_cot0.is_negative_infinity())) {
				Edge *e = new_edge_pair(c0, c1);
				if (pending_tail0) {
					pending_tail0->prev = e;
				} else {
					pending_head0 = e;
				}
				e->next = pending_tail0;
				pending_tail0 = e;

				e = e->reverse;
				if (pending_tail1) {
					pending_tail1->next = e;
				} else {
					pending_head1 = e;
				}
				e->prev = pending_tail1;
				pending_tail1 = e;
			}

			Edge *e0 = min0;
			Edge *e1 = min1;

#ifdef DEBUG_CONVEX_HULL
			printf("   Found min edges to %d %d\n", e0 ? e0->target->point.index : -1, e1 ? e1->target->point.index : -1);
#endif

			if (cmp == 0) {
				find_edge_for_coplanar_faces(c0, c1, e0, e1, nullptr, nullptr);
			}

			if ((cmp >= 0) && e1) {
				if (to_prev1) {
					for (Edge *e = to_prev1->next, *n = nullptr; e != min1; e = n) {
						n = e->next;
						remove_edge_pair(e);
					}
				}

				if (pending_tail1) {
					if (to_prev1) {
						to_prev1->link(pending_head1);
					} else {
						min1->prev->link(pending_head1);
						first_new1 = pending_head1;
					}
					pending_tail1->link(min1);
					pending_head1 = nullptr;
					pending_tail1 = nullptr;
				} else if (!to_prev1) {
					first_new1 = min1;
				}

				prev_point = c1->point;
				c1 = e1->target;
				to_prev1 = e1->reverse;
			}

			if ((cmp <= 0) && e0) {
				if (to_prev0) {
					for (Edge *e = to_prev0->prev, *n = nullptr; e != min0; e = n) {
						n = e->prev;
						remove_edge_pair(e);
					}
				}

				if (pending_tail0) {
					if (to_prev0) {
						pending_head0->link(to_prev0);
					} else {
						pending_head0->link(min0->next);
						first_new0 = pending_head0;
					}
					min0->link(pending_tail0);
					pending_head0 = nullptr;
					pending_tail0 = nullptr;
				} else if (!to_prev0) {
					first_new0 = min0;
				}

				prev_point = c0->point;
				c0 = e0->target;
				to_prev0 = e0->reverse;
			}
		}

		if ((c0 == first0) && (c1 == first1)) {
			if (to_prev0 == nullptr) {
				pending_head0->link(pending_tail0);
				c0->edges = pending_tail0;
			} else {
				for (Edge *e = to_prev0->prev, *n = nullptr; e != first_new0; e = n) {
					n = e->prev;
					remove_edge_pair(e);
				}
				if (pending_tail0) {
					pending_head0->link(to_prev0);
					first_new0->link(pending_tail0);
				}
			}

			if (to_prev1 == nullptr) {
				pending_tail1->link(pending_head1);
				c1->edges = pending_tail1;
			} else {
				for (Edge *e = to_prev1->next, *n = nullptr; e != first_new1; e = n) {
					n = e->next;
					remove_edge_pair(e);
				}
				if (pending_tail1) {
					to_prev1->link(pending_head1);
					pending_tail1->link(first_new1);
				}
			}

			return;
		}

		first_run = false;
	}
}

struct PointComparator {
	_FORCE_INLINE_ bool operator()(const ConvexHullInternal::Point32 &p_left, const ConvexHullInternal::Point32 &p_right) const {
		return (p_left.y < p_right.y) || ((p_left.y == p_right.y) && ((p_left.x < p_right.x) || ((p_left.x == p_right.x) && (p_left.z < p_right.z))));
	}
};

void ConvexHullInternal::compute(const Vector3 *p_coords, int32_t p_count) {
	AABB aabb;
	for (int32_t i = 0; i < p_count; i++) {
		Vector3 p = p_coords[i];
		if (i == 0) {
			aabb.position = p;
		} else {
			aabb.expand_to(p);
		}
	}

	Vector3 s = aabb.size;
	max_axis = s.max_axis_index();
	min_axis = s.min_axis_index();
	if (min_axis == max_axis) {
		min_axis = Vector3::Axis((max_axis + 1) % 3);
	}
	med_axis = Vector3::Axis(3 - max_axis - min_axis);

	s /= real_t(10216);
	if (((med_axis + 1) % 3) != max_axis) {
		s *= -1;
	}
	scaling = s;

	if (s[0] != 0) {
		s[0] = real_t(1) / s[0];
	}
	if (s[1] != 0) {
		s[1] = real_t(1) / s[1];
	}
	if (s[2] != 0) {
		s[2] = real_t(1) / s[2];
	}

	center = aabb.position;

	LocalVector<Point32> points;
	points.resize(p_count);
	for (int32_t i = 0; i < p_count; i++) {
		Vector3 p = p_coords[i];
		p = (p - center) * s;
		points[i].x = (int32_t)p[med_axis];
		points[i].y = (int32_t)p[max_axis];
		points[i].z = (int32_t)p[min_axis];
		points[i].index = i;
	}

	points.sort_custom<PointComparator>();

	vertex_pool.reset(true);
	original_vertices.resize(p_count);
	for (int32_t i = 0; i < p_count; i++) {
		Vertex *v = vertex_pool.alloc();
		v->edges = nullptr;
		v->point = points[i];
		v->copy = -1;
		original_vertices[i] = v;
	}

	points.clear();

	edge_pool.reset(true);

	used_edge_pairs = 0;
	max_used_edge_pairs = 0;

	merge_stamp = -3;

	IntermediateHull hull;
	compute_internal(0, p_count, hull);
	vertex_list = hull.min_xy;
#ifdef DEBUG_CONVEX_HULL
	printf("max. edges %d (3v = %d)", max_used_edge_pairs, 3 * p_count);
#endif
}

Vector3 ConvexHullInternal::to_gd_vector(const Point32 &p_v) {
	Vector3 p;
	p[med_axis] = real_t(p_v.x);
	p[max_axis] = real_t(p_v.y);
	p[min_axis] = real_t(p_v.z);
	return p * scaling;
}

Vector3 ConvexHullInternal::get_gd_normal(Face *p_face) {
	return to_gd_vector(p_face->dir0).cross(to_gd_vector(p_face->dir1)).normalized();
}

Vector3 ConvexHullInternal::get_coordinates(const Vertex *p_v) {
	Vector3 p;
	p[med_axis] = p_v->xvalue();
	p[max_axis] = p_v->yvalue();
	p[min_axis] = p_v->zvalue();
	return p * scaling + center;
}

real_t ConvexHullInternal::shrink(real_t p_amount, real_t p_clamp_amount) {
	if (!vertex_list) {
		return 0;
	}
	int32_t stamp = --merge_stamp;
	LocalVector<Vertex *> stack;
	vertex_list->copy = stamp;
	stack.push_back(vertex_list);
	LocalVector<Face *> faces;

	Point32 ref = vertex_list->point;
	Int128 hull_center_x(0, 0);
	Int128 hull_center_y(0, 0);
	Int128 hull_center_z(0, 0);
	Int128 volume(0, 0);

	while (stack.size() > 0) {
		Vertex *v = stack[stack.size() - 1];
		stack.remove_at(stack.size() - 1);
		Edge *e = v->edges;
		if (e) {
			do {
				if (e->target->copy != stamp) {
					e->target->copy = stamp;
					stack.push_back(e->target);
				}
				if (e->copy != stamp) {
					Face *face = face_pool.alloc();
					face->init(e->target, e->reverse->prev->target, v);
					faces.push_back(face);
					Edge *f = e;

					Vertex *a = nullptr;
					Vertex *b = nullptr;
					do {
						if (a && b) {
							int64_t vol = (v->point - ref).dot((a->point - ref).cross(b->point - ref));
							CHULL_ASSERT(vol >= 0);
							Point32 c = v->point + a->point + b->point + ref;
							hull_center_x += vol * c.x;
							hull_center_y += vol * c.y;
							hull_center_z += vol * c.z;
							volume += vol;
						}

						CHULL_ASSERT(f->copy != stamp);
						f->copy = stamp;
						f->face = face;

						a = b;
						b = f->target;

						f = f->reverse->prev;
					} while (f != e);
				}
				e = e->next;
			} while (e != v->edges);
		}
	}

	if (volume.get_sign() <= 0) {
		return 0;
	}

	Vector3 hull_center;
	hull_center[med_axis] = hull_center_x.to_scalar();
	hull_center[max_axis] = hull_center_y.to_scalar();
	hull_center[min_axis] = hull_center_z.to_scalar();
	hull_center /= 4 * volume.to_scalar();
	hull_center *= scaling;

	int32_t face_count = faces.size();

	if (p_clamp_amount > 0) {
		real_t min_dist = FLT_MAX;
		for (int32_t i = 0; i < face_count; i++) {
			Vector3 normal = get_gd_normal(faces[i]);
			real_t dist = normal.dot(to_gd_vector(faces[i]->origin) - hull_center);
			if (dist < min_dist) {
				min_dist = dist;
			}
		}

		if (min_dist <= 0) {
			return 0;
		}

		p_amount = MIN(p_amount, min_dist * p_clamp_amount);
	}

	uint32_t seed = 243703;
	for (int32_t i = 0; i < face_count; i++, seed = 1664525 * seed + 1013904223) {
		SWAP(faces[i], faces[seed % face_count]);
	}

	for (int32_t i = 0; i < face_count; i++) {
		if (!shift_face(faces[i], p_amount, stack)) {
			return -p_amount;
		}
	}

	return p_amount;
}

bool ConvexHullInternal::shift_face(Face *p_face, real_t p_amount, LocalVector<Vertex *> &p_stack) {
	Vector3 orig_shift = get_gd_normal(p_face) * -p_amount;
	if (scaling[0] != 0) {
		orig_shift[0] /= scaling[0];
	}
	if (scaling[1] != 0) {
		orig_shift[1] /= scaling[1];
	}
	if (scaling[2] != 0) {
		orig_shift[2] /= scaling[2];
	}
	Point32 shift((int32_t)orig_shift[med_axis], (int32_t)orig_shift[max_axis], (int32_t)orig_shift[min_axis]);
	if (shift.is_zero()) {
		return true;
	}
	Point64 normal = p_face->get_normal();
#ifdef DEBUG_CONVEX_HULL
	printf("\nShrinking p_face (%d %d %d) (%d %d %d) (%d %d %d) by (%d %d %d)\n",
			p_face->origin.x, p_face->origin.y, p_face->origin.z, p_face->dir0.x, p_face->dir0.y, p_face->dir0.z, p_face->dir1.x, p_face->dir1.y, p_face->dir1.z, shift.x, shift.y, shift.z);
#endif
	int64_t orig_dot = p_face->origin.dot(normal);
	Point32 shifted_origin = p_face->origin + shift;
	int64_t shifted_dot = shifted_origin.dot(normal);
	CHULL_ASSERT(shifted_dot <= orig_dot);
	if (shifted_dot >= orig_dot) {
		return false;
	}

	Edge *intersection = nullptr;

	Edge *start_edge = p_face->nearby_vertex->edges;
#ifdef DEBUG_CONVEX_HULL
	printf("Start edge is ");
	start_edge->print();
	printf(", normal is (%lld %lld %lld), shifted dot is %lld\n", normal.x, normal.y, normal.z, shifted_dot);
#endif
	Rational128 opt_dot = p_face->nearby_vertex->dot(normal);
	int32_t cmp = opt_dot.compare(shifted_dot);
#ifdef SHOW_ITERATIONS
	int32_t n = 0;
#endif
	if (cmp >= 0) {
		Edge *e = start_edge;
		do {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			Rational128 dot = e->target->dot(normal);
			CHULL_ASSERT(dot.compare(orig_dot) <= 0);
#ifdef DEBUG_CONVEX_HULL
			printf("Moving downwards, edge is ");
			e->print();
			printf(", dot is %f (%f %lld)\n", (float)dot.to_scalar(), (float)opt_dot.to_scalar(), shifted_dot);
#endif
			if (dot.compare(opt_dot) < 0) {
				int32_t c = dot.compare(shifted_dot);
				opt_dot = dot;
				e = e->reverse;
				start_edge = e;
				if (c < 0) {
					intersection = e;
					break;
				}
				cmp = c;
			}
			e = e->prev;
		} while (e != start_edge);

		if (!intersection) {
			return false;
		}
	} else {
		Edge *e = start_edge;
		do {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			Rational128 dot = e->target->dot(normal);
			CHULL_ASSERT(dot.compare(orig_dot) <= 0);
#ifdef DEBUG_CONVEX_HULL
			printf("Moving upwards, edge is ");
			e->print();
			printf(", dot is %f (%f %lld)\n", (float)dot.to_scalar(), (float)opt_dot.to_scalar(), shifted_dot);
#endif
			if (dot.compare(opt_dot) > 0) {
				cmp = dot.compare(shifted_dot);
				if (cmp >= 0) {
					intersection = e;
					break;
				}
				opt_dot = dot;
				e = e->reverse;
				start_edge = e;
			}
			e = e->prev;
		} while (e != start_edge);

		if (!intersection) {
			return true;
		}
	}

#ifdef SHOW_ITERATIONS
	printf("Needed %d iterations to find initial intersection\n", n);
#endif

	if (cmp == 0) {
		Edge *e = intersection->reverse->next;
#ifdef SHOW_ITERATIONS
		n = 0;
#endif
		while (e->target->dot(normal).compare(shifted_dot) <= 0) {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			e = e->next;
			if (e == intersection->reverse) {
				return true;
			}
#ifdef DEBUG_CONVEX_HULL
			printf("Checking for outwards edge, current edge is ");
			e->print();
			printf("\n");
#endif
		}
#ifdef SHOW_ITERATIONS
		printf("Needed %d iterations to check for complete containment\n", n);
#endif
	}

	Edge *first_intersection = nullptr;
	Edge *face_edge = nullptr;
	Edge *first_face_edge = nullptr;

#ifdef SHOW_ITERATIONS
	int32_t m = 0;
#endif
	while (true) {
#ifdef SHOW_ITERATIONS
		m++;
#endif
#ifdef DEBUG_CONVEX_HULL
		printf("Intersecting edge is ");
		intersection->print();
		printf("\n");
#endif
		if (cmp == 0) {
			Edge *e = intersection->reverse->next;
			start_edge = e;
#ifdef SHOW_ITERATIONS
			n = 0;
#endif
			while (true) {
#ifdef SHOW_ITERATIONS
				n++;
#endif
				if (e->target->dot(normal).compare(shifted_dot) >= 0) {
					break;
				}
				intersection = e->reverse;
				e = e->next;
				if (e == start_edge) {
					return true;
				}
			}
#ifdef SHOW_ITERATIONS
			printf("Needed %d iterations to advance intersection\n", n);
#endif
		}

#ifdef DEBUG_CONVEX_HULL
		printf("Advanced intersecting edge to ");
		intersection->print();
		printf(", cmp = %d\n", cmp);
#endif

		if (!first_intersection) {
			first_intersection = intersection;
		} else if (intersection == first_intersection) {
			break;
		}

		int32_t prev_cmp = cmp;
		Edge *prev_intersection = intersection;
		Edge *prev_face_edge = face_edge;

		Edge *e = intersection->reverse;
#ifdef SHOW_ITERATIONS
		n = 0;
#endif
		while (true) {
#ifdef SHOW_ITERATIONS
			n++;
#endif
			e = e->reverse->prev;
			CHULL_ASSERT(e != intersection->reverse);
			cmp = e->target->dot(normal).compare(shifted_dot);
#ifdef DEBUG_CONVEX_HULL
			printf("Testing edge ");
			e->print();
			printf(" -> cmp = %d\n", cmp);
#endif
			if (cmp >= 0) {
				intersection = e;
				break;
			}
		}
#ifdef SHOW_ITERATIONS
		printf("Needed %d iterations to find other intersection of p_face\n", n);
#endif

		if (cmp > 0) {
			Vertex *removed = intersection->target;
			e = intersection->reverse;
			if (e->prev == e) {
				removed->edges = nullptr;
			} else {
				removed->edges = e->prev;
				e->prev->link(e->next);
				e->link(e);
			}
#ifdef DEBUG_CONVEX_HULL
			printf("1: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif

			Point64 n0 = intersection->face->get_normal();
			Point64 n1 = intersection->reverse->face->get_normal();
			int64_t m00 = p_face->dir0.dot(n0);
			int64_t m01 = p_face->dir1.dot(n0);
			int64_t m10 = p_face->dir0.dot(n1);
			int64_t m11 = p_face->dir1.dot(n1);
			int64_t r0 = (intersection->face->origin - shifted_origin).dot(n0);
			int64_t r1 = (intersection->reverse->face->origin - shifted_origin).dot(n1);
			Int128 det = Int128::mul(m00, m11) - Int128::mul(m01, m10);
			CHULL_ASSERT(det.get_sign() != 0);
			Vertex *v = vertex_pool.alloc();
			v->point.index = -1;
			v->copy = -1;
			v->point128 = PointR128(Int128::mul(p_face->dir0.x * r0, m11) - Int128::mul(p_face->dir0.x * r1, m01) + Int128::mul(p_face->dir1.x * r1, m00) - Int128::mul(p_face->dir1.x * r0, m10) + det * shifted_origin.x,
					Int128::mul(p_face->dir0.y * r0, m11) - Int128::mul(p_face->dir0.y * r1, m01) + Int128::mul(p_face->dir1.y * r1, m00) - Int128::mul(p_face->dir1.y * r0, m10) + det * shifted_origin.y,
					Int128::mul(p_face->dir0.z * r0, m11) - Int128::mul(p_face->dir0.z * r1, m01) + Int128::mul(p_face->dir1.z * r1, m00) - Int128::mul(p_face->dir1.z * r0, m10) + det * shifted_origin.z,
					det);
			v->point.x = (int32_t)v->point128.xvalue();
			v->point.y = (int32_t)v->point128.yvalue();
			v->point.z = (int32_t)v->point128.zvalue();
			intersection->target = v;
			v->edges = e;

			p_stack.push_back(v);
			p_stack.push_back(removed);
			p_stack.push_back(nullptr);
		}

		if (cmp || prev_cmp || (prev_intersection->reverse->next->target != intersection->target)) {
			face_edge = new_edge_pair(prev_intersection->target, intersection->target);
			if (prev_cmp == 0) {
				face_edge->link(prev_intersection->reverse->next);
			}
			if ((prev_cmp == 0) || prev_face_edge) {
				prev_intersection->reverse->link(face_edge);
			}
			if (cmp == 0) {
				intersection->reverse->prev->link(face_edge->reverse);
			}
			face_edge->reverse->link(intersection->reverse);
		} else {
			face_edge = prev_intersection->reverse->next;
		}

		if (prev_face_edge) {
			if (prev_cmp > 0) {
				face_edge->link(prev_face_edge->reverse);
			} else if (face_edge != prev_face_edge->reverse) {
				p_stack.push_back(prev_face_edge->target);
				while (face_edge->next != prev_face_edge->reverse) {
					Vertex *removed = face_edge->next->target;
					remove_edge_pair(face_edge->next);
					p_stack.push_back(removed);
#ifdef DEBUG_CONVEX_HULL
					printf("2: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif
				}
				p_stack.push_back(nullptr);
			}
		}
		face_edge->face = p_face;
		face_edge->reverse->face = intersection->face;

		if (!first_face_edge) {
			first_face_edge = face_edge;
		}
	}
#ifdef SHOW_ITERATIONS
	printf("Needed %d iterations to process all intersections\n", m);
#endif

	if (cmp > 0) {
		first_face_edge->reverse->target = face_edge->target;
		first_intersection->reverse->link(first_face_edge);
		first_face_edge->link(face_edge->reverse);
	} else if (first_face_edge != face_edge->reverse) {
		p_stack.push_back(face_edge->target);
		while (first_face_edge->next != face_edge->reverse) {
			Vertex *removed = first_face_edge->next->target;
			remove_edge_pair(first_face_edge->next);
			p_stack.push_back(removed);
#ifdef DEBUG_CONVEX_HULL
			printf("3: Removed part contains (%d %d %d)\n", removed->point.x, removed->point.y, removed->point.z);
#endif
		}
		p_stack.push_back(nullptr);
	}

	CHULL_ASSERT(p_stack.size() > 0);
	vertex_list = p_stack[0];

#ifdef DEBUG_CONVEX_HULL
	printf("Removing part\n");
#endif
#ifdef SHOW_ITERATIONS
	n = 0;
#endif
	uint32_t pos = 0;
	while (pos < p_stack.size()) {
		uint32_t end = p_stack.size();
		while (pos < end) {
			Vertex *kept = p_stack[pos++];
#ifdef DEBUG_CONVEX_HULL
			kept->print();
#endif
			bool deeper = false;
			Vertex *removed;
			while ((removed = p_stack[pos++]) != nullptr) {
#ifdef SHOW_ITERATIONS
				n++;
#endif
				kept->receive_nearby_faces(removed);
				while (removed->edges) {
					if (!deeper) {
						deeper = true;
						p_stack.push_back(kept);
					}
					p_stack.push_back(removed->edges->target);
					remove_edge_pair(removed->edges);
				}
			}
			if (deeper) {
				p_stack.push_back(nullptr);
			}
		}
	}
#ifdef SHOW_ITERATIONS
	printf("Needed %d iterations to remove part\n", n);
#endif

	p_stack.clear();
	p_face->origin = shifted_origin;

	return true;
}

static int32_t get_vertex_copy(ConvexHullInternal::Vertex *p_vertex, LocalVector<ConvexHullInternal::Vertex *> &p_vertices) {
	int32_t index = p_vertex->copy;
	if (index < 0) {
		index = p_vertices.size();
		p_vertex->copy = index;
		p_vertices.push_back(p_vertex);
#ifdef DEBUG_CONVEX_HULL
		printf("Vertex %d gets index *%d\n", p_vertex->point.index, index);
#endif
	}
	return index;
}

real_t ConvexHullComputer::compute(const Vector3 *p_coords, int32_t p_count, real_t p_shrink, real_t p_shrink_clamp) {
	vertices.clear();
	edges.clear();
	faces.clear();

	if (p_count <= 0) {
		return 0;
	}

	ConvexHullInternal hull;
	hull.compute(p_coords, p_count);

	real_t shift = 0;
	if ((p_shrink > 0) && ((shift = hull.shrink(p_shrink, p_shrink_clamp)) < 0)) {
		return shift;
	}

	LocalVector<ConvexHullInternal::Vertex *> old_vertices;
	get_vertex_copy(hull.vertex_list, old_vertices);
	int32_t copied = 0;
	while (copied < (int32_t)old_vertices.size()) {
		ConvexHullInternal::Vertex *v = old_vertices[copied];
		vertices.push_back(hull.get_coordinates(v));
		ConvexHullInternal::Edge *first_edge = v->edges;
		if (first_edge) {
			int32_t first_copy = -1;
			int32_t prev_copy = -1;
			ConvexHullInternal::Edge *e = first_edge;
			do {
				if (e->copy < 0) {
					int32_t s = edges.size();
					edges.push_back(Edge());
					edges.push_back(Edge());
					Edge *c = &edges[s];
					Edge *r = &edges[s + 1];
					e->copy = s;
					e->reverse->copy = s + 1;
					c->reverse = 1;
					r->reverse = -1;
					c->target_vertex = get_vertex_copy(e->target, old_vertices);
					r->target_vertex = copied;
#ifdef DEBUG_CONVEX_HULL
					printf("      CREATE: Vertex *%d has edge to *%d\n", copied, c->get_target_vertex());
#endif
				}
				if (prev_copy >= 0) {
					edges[e->copy].next = prev_copy - e->copy;
				} else {
					first_copy = e->copy;
				}
				prev_copy = e->copy;
				e = e->next;
			} while (e != first_edge);
			edges[first_copy].next = prev_copy - first_copy;
		}
		copied++;
	}

	for (int32_t i = 0; i < copied; i++) {
		ConvexHullInternal::Vertex *v = old_vertices[i];
		ConvexHullInternal::Edge *first_edge = v->edges;
		if (first_edge) {
			ConvexHullInternal::Edge *e = first_edge;
			do {
				if (e->copy >= 0) {
#ifdef DEBUG_CONVEX_HULL
					printf("Vertex *%d has edge to *%d\n", i, edges[e->copy].get_target_vertex());
#endif
					faces.push_back(e->copy);
					ConvexHullInternal::Edge *f = e;
					do {
#ifdef DEBUG_CONVEX_HULL
						printf("   Face *%d\n", edges[f->copy].get_target_vertex());
#endif
						f->copy = -1;
						f = f->reverse->prev;
					} while (f != e);
				}
				e = e->next;
			} while (e != first_edge);
		}
	}

	return shift;
}

Error ConvexHullComputer::convex_hull(const Vector<Vector3> &p_points, Geometry3D::MeshData &r_mesh) {
	r_mesh = Geometry3D::MeshData(); // clear

	if (p_points.is_empty()) {
		return FAILED; // matches QuickHull
	}

	ConvexHullComputer ch;
	ch.compute(p_points.ptr(), p_points.size(), -1.0, -1.0);

	r_mesh.vertices = ch.vertices;

	// Tag which face each edge belongs to
	LocalVector<int32_t> edge_faces;
	edge_faces.resize(ch.edges.size());

	for (uint32_t i = 0; i < ch.edges.size(); i++) {
		edge_faces[i] = -1;
	}

	for (uint32_t i = 0; i < ch.faces.size(); i++) {
		const Edge *e_start = &ch.edges[ch.faces[i]];
		const Edge *e = e_start;
		do {
			int64_t ofs = e - ch.edges.ptr();
			edge_faces[ofs] = i;

			e = e->get_next_edge_of_face();
		} while (e != e_start);
	}

	// Copy the edges over. There's two "half-edges" for every edge, so we pick only one of them.
	r_mesh.edges.resize(ch.edges.size() / 2);
	AHashMap<uint64_t, int32_t> edge_map(ch.edges.size() * 4); // The higher the capacity, the faster the insert

	uint32_t edges_copied = 0;
	for (uint32_t i = 0; i < ch.edges.size(); i++) {
		ERR_CONTINUE(edge_faces[i] == -1); // Safety check.

		uint32_t a = (&ch.edges[i])->get_source_vertex();
		uint32_t b = (&ch.edges[i])->get_target_vertex();
		if (a < b) { // Copy only the "canonical" edge. For the reverse edge, this will be false.
			ERR_BREAK(edges_copied >= (uint32_t)r_mesh.edges.size());
			r_mesh.edges[edges_copied].vertex_a = a;
			r_mesh.edges[edges_copied].vertex_b = b;
			r_mesh.edges[edges_copied].face_a = edge_faces[i];
			r_mesh.edges[edges_copied].face_b = -1;

			uint64_t key = a;
			key <<= 32;
			key |= b;
			edge_map.insert(key, edges_copied);

			edges_copied++;
		} else {
			uint64_t key = b;
			key <<= 32;
			key |= a;
			int32_t *index_ptr = edge_map.getptr(key);
			if (!index_ptr) {
				ERR_PRINT("Invalid edge");
			} else {
				r_mesh.edges[*index_ptr].face_b = edge_faces[i];
			}
		}
	}

	if (edges_copied != (uint32_t)r_mesh.edges.size()) {
		ERR_PRINT("Invalid edge count.");
	}

	r_mesh.faces.resize(ch.faces.size());
	for (uint32_t i = 0; i < ch.faces.size(); i++) {
		const Edge *e_start = &ch.edges[ch.faces[i]];
		const Edge *e = e_start;
		Geometry3D::MeshData::Face &face = r_mesh.faces[i];

		do {
			face.indices.push_back(e->get_target_vertex());

			e = e->get_next_edge_of_face();
		} while (e != e_start);

		// reverse indices: Godot wants clockwise, but this is counter-clockwise
		if (face.indices.size() > 2) {
			// reverse all but the first index.
			int *indices = face.indices.ptr();
			for (uint32_t c = 0; c < (face.indices.size() - 1) / 2; c++) {
				SWAP(indices[c + 1], indices[face.indices.size() - 1 - c]);
			}
		}

		// compute normal
		if (face.indices.size() >= 3) {
			face.plane = Plane(r_mesh.vertices[face.indices[0]], r_mesh.vertices[face.indices[1]], r_mesh.vertices[face.indices[2]]);
		} else {
			WARN_PRINT("Too few vertices per face.");
		}
	}

	return OK;
}

// --- quick_hull.cpp ---

uint32_t QuickHull::debug_stop_after = 0xFFFFFFFF;

Error QuickHull::build(const Vector<Vector3> &p_points, Geometry3D::MeshData &r_mesh) {
	/* CREATE AABB VOLUME */

	AABB aabb;
	for (int i = 0; i < p_points.size(); i++) {
		if (i == 0) {
			aabb.position = p_points[i];
		} else {
			aabb.expand_to(p_points[i]);
		}
	}

	if (aabb.size == Vector3()) {
		return ERR_CANT_CREATE;
	}

	Vector<bool> valid_points;
	valid_points.resize(p_points.size());
	HashSet<Vector3> valid_cache;

	for (int i = 0; i < p_points.size(); i++) {
		Vector3 sp = p_points[i].snappedf(0.0001);
		if (valid_cache.has(sp)) {
			valid_points.write[i] = false;
		} else {
			valid_points.write[i] = true;
			valid_cache.insert(sp);
		}
	}

	/* CREATE INITIAL SIMPLEX */

	int longest_axis = aabb.get_longest_axis_index();

	//first two vertices are the most distant
	int simplex[4] = { 0 };

	{
		real_t max = 0, min = 0;

		for (int i = 0; i < p_points.size(); i++) {
			if (!valid_points[i]) {
				continue;
			}
			real_t d = p_points[i][longest_axis];
			if (i == 0 || d < min) {
				simplex[0] = i;
				min = d;
			}

			if (i == 0 || d > max) {
				simplex[1] = i;
				max = d;
			}
		}
	}

	//third vertex is one most further away from the line

	{
		real_t maxd = 0;
		Vector3 rel12 = p_points[simplex[0]] - p_points[simplex[1]];

		for (int i = 0; i < p_points.size(); i++) {
			if (!valid_points[i]) {
				continue;
			}

			Vector3 n = rel12.cross(p_points[simplex[0]] - p_points[i]).cross(rel12).normalized();
			real_t d = Math::abs(n.dot(p_points[simplex[0]]) - n.dot(p_points[i]));

			if (i == 0 || d > maxd) {
				maxd = d;
				simplex[2] = i;
			}
		}
	}

	//fourth vertex is the one most further away from the plane

	{
		real_t maxd = 0;
		Plane p(p_points[simplex[0]], p_points[simplex[1]], p_points[simplex[2]]);

		for (int i = 0; i < p_points.size(); i++) {
			if (!valid_points[i]) {
				continue;
			}

			real_t d = Math::abs(p.distance_to(p_points[i]));

			if (i == 0 || d > maxd) {
				maxd = d;
				simplex[3] = i;
			}
		}
	}

	//compute center of simplex, this is a point always warranted to be inside
	Vector3 center;

	for (int i = 0; i < 4; i++) {
		center += p_points[simplex[i]];
	}

	center /= 4.0;

	//add faces

	List<Face> faces;

	for (int i = 0; i < 4; i++) {
		static const int face_order[4][3] = {
			{ 0, 1, 2 },
			{ 0, 1, 3 },
			{ 0, 2, 3 },
			{ 1, 2, 3 }
		};

		Face f;
		for (int j = 0; j < 3; j++) {
			f.vertices[j] = simplex[face_order[i][j]];
		}

		Plane p(p_points[f.vertices[0]], p_points[f.vertices[1]], p_points[f.vertices[2]]);

		if (p.is_point_over(center)) {
			//flip face to clockwise if facing inwards
			SWAP(f.vertices[0], f.vertices[1]);
			p = -p;
		}

		f.plane = p;

		faces.push_back(f);
	}

	real_t over_tolerance = 3 * UNIT_EPSILON * (aabb.size.x + aabb.size.y + aabb.size.z);

	/* COMPUTE AVAILABLE VERTICES */

	for (int i = 0; i < p_points.size(); i++) {
		if (i == simplex[0]) {
			continue;
		}
		if (i == simplex[1]) {
			continue;
		}
		if (i == simplex[2]) {
			continue;
		}
		if (i == simplex[3]) {
			continue;
		}
		if (!valid_points[i]) {
			continue;
		}

		for (Face &E : faces) {
			if (E.plane.distance_to(p_points[i]) > over_tolerance) {
				E.points_over.push_back(i);
				break;
			}
		}
	}

	faces.sort(); // sort them, so the ones with points are in the back

	/* BUILD HULL */

	//poop face (while still remain)
	//find further away point
	//find lit faces
	//determine horizon edges
	//build new faces with horizon edges, them assign points side from all lit faces
	//remove lit faces

	uint32_t debug_stop = debug_stop_after;

	while (debug_stop > 0 && faces.back()->get().points_over.size()) {
		debug_stop--;
		Face &f = faces.back()->get();

		//find vertex most outside
		int next = -1;
		real_t next_d = 0;

		for (int i = 0; i < f.points_over.size(); i++) {
			real_t d = f.plane.distance_to(p_points[f.points_over[i]]);

			if (d > next_d) {
				next_d = d;
				next = i;
			}
		}

		ERR_FAIL_COND_V(next == -1, ERR_BUG);

		Vector3 v = p_points[f.points_over[next]];

		//find lit faces and lit edges
		List<List<Face>::Element *> lit_faces; //lit face is a death sentence

		HashMap<Edge, FaceConnect, Edge> lit_edges; //create this on the flight, should not be that bad for performance and simplifies code a lot

		for (List<Face>::Element *E = faces.front(); E; E = E->next()) {
			if (E->get().plane.distance_to(v) > 0) {
				lit_faces.push_back(E);

				for (int i = 0; i < 3; i++) {
					uint32_t a = E->get().vertices[i];
					uint32_t b = E->get().vertices[(i + 1) % 3];
					Edge e(a, b);

					HashMap<Edge, FaceConnect, Edge>::Iterator F = lit_edges.find(e);
					if (!F) {
						F = lit_edges.insert(e, FaceConnect());
					}
					if (e.vertices[0] == a) {
						//left
						F->value.left = E;
					} else {
						F->value.right = E;
					}
				}
			}
		}

		//create new faces from horizon edges
		List<List<Face>::Element *> new_faces; //new faces

		for (KeyValue<Edge, FaceConnect> &E : lit_edges) {
			FaceConnect &fc = E.value;
			if (fc.left && fc.right) {
				continue; //edge is uninteresting, not on horizon
			}

			//create new face!

			Face face;
			face.vertices[0] = f.points_over[next];
			face.vertices[1] = E.key.vertices[0];
			face.vertices[2] = E.key.vertices[1];

			Plane p(p_points[face.vertices[0]], p_points[face.vertices[1]], p_points[face.vertices[2]]);

			if (p.is_point_over(center)) {
				//flip face to clockwise if facing inwards
				SWAP(face.vertices[0], face.vertices[1]);
				p = -p;
			}

			face.plane = p;
			new_faces.push_back(faces.push_back(face));
		}

		//distribute points into new faces

		for (List<Face>::Element *&F : lit_faces) {
			Face &lf = F->get();

			for (int i = 0; i < lf.points_over.size(); i++) {
				if (lf.points_over[i] == f.points_over[next]) { //do not add current one
					continue;
				}

				Vector3 p = p_points[lf.points_over[i]];
				for (List<Face>::Element *&E : new_faces) {
					Face &f2 = E->get();
					if (f2.plane.distance_to(p) > over_tolerance) {
						f2.points_over.push_back(lf.points_over[i]);
						break;
					}
				}
			}
		}

		//erase lit faces

		while (lit_faces.size()) {
			faces.erase(lit_faces.front()->get());
			lit_faces.pop_front();
		}

		//put faces that contain no points on the front

		for (List<Face>::Element *&E : new_faces) {
			Face &f2 = E->get();
			if (f2.points_over.is_empty()) {
				faces.move_to_front(E);
			}
		}

		//whew, done with iteration, go next
	}

	/* CREATE MESHDATA */

	//make a map of edges again
	HashMap<Edge, RetFaceConnect, Edge> ret_edges;
	List<Geometry3D::MeshData::Face> ret_faces;

	for (const Face &E : faces) {
		Geometry3D::MeshData::Face f;
		f.plane = E.plane;

		for (int i = 0; i < 3; i++) {
			f.indices.push_back(E.vertices[i]);
		}

		List<Geometry3D::MeshData::Face>::Element *F = ret_faces.push_back(f);

		for (int i = 0; i < 3; i++) {
			uint32_t a = E.vertices[i];
			uint32_t b = E.vertices[(i + 1) % 3];
			Edge e(a, b);

			HashMap<Edge, RetFaceConnect, Edge>::Iterator G = ret_edges.find(e);
			if (!G) {
				G = ret_edges.insert(e, RetFaceConnect());
			}
			if (e.vertices[0] == a) {
				//left
				G->value.left = F;
			} else {
				G->value.right = F;
			}
		}
	}

	//fill faces

	for (List<Geometry3D::MeshData::Face>::Element *E = ret_faces.front(); E; E = E->next()) {
		Geometry3D::MeshData::Face &f = E->get();

		for (uint32_t i = 0; i < f.indices.size(); i++) {
			int a = E->get().indices[i];
			int b = E->get().indices[(i + 1) % f.indices.size()];
			Edge e(a, b);

			HashMap<Edge, RetFaceConnect, Edge>::Iterator F = ret_edges.find(e);

			ERR_CONTINUE(!F);
			List<Geometry3D::MeshData::Face>::Element *O = F->value.left == E ? F->value.right : F->value.left;
			ERR_CONTINUE(O == E);
			ERR_CONTINUE(O == nullptr);

			if (O->get().plane.is_equal_approx(f.plane)) {
				//merge and delete edge and contiguous face, while repointing edges (uuugh!)
				int o_index_size = O->get().indices.size();

				for (int j = 0; j < o_index_size; j++) {
					//search a
					if (O->get().indices[j] == a) {
						//append the rest
						for (int k = 0; k < o_index_size; k++) {
							int idx = O->get().indices[(k + j) % o_index_size];
							int idxn = O->get().indices[(k + j + 1) % o_index_size];
							if (idx == b && idxn == a) { //already have b!
								break;
							}
							if (idx != a) {
								f.indices.insert(i + 1, idx);
								i++;
							}
							Edge e2(idx, idxn);

							HashMap<Edge, RetFaceConnect, Edge>::Iterator F2 = ret_edges.find(e2);
							ERR_CONTINUE(!F2);
							//change faceconnect, point to this face instead
							if (F2->value.left == O) {
								F2->value.left = E;
							} else if (F2->value.right == O) {
								F2->value.right = E;
							}
						}

						break;
					}
				}

				// remove all edge connections to this face
				for (KeyValue<Edge, RetFaceConnect> &G : ret_edges) {
					if (G.value.left == O) {
						G.value.left = nullptr;
					}

					if (G.value.right == O) {
						G.value.right = nullptr;
					}
				}

				ret_edges.remove(F); //remove the edge
				ret_faces.erase(O); //remove the face
			}
		}
	}

	//fill mesh
	r_mesh.faces.clear();
	r_mesh.faces.resize(ret_faces.size());

	HashMap<List<Geometry3D::MeshData::Face>::Element *, int> face_indices;

	int idx = 0;
	for (List<Geometry3D::MeshData::Face>::Element *E = ret_faces.front(); E; E = E->next()) {
		face_indices[E] = idx;
		r_mesh.faces[idx++] = E->get();
	}
	r_mesh.edges.resize(ret_edges.size());
	idx = 0;
	for (const KeyValue<Edge, RetFaceConnect> &E : ret_edges) {
		Geometry3D::MeshData::Edge e;
		e.vertex_a = E.key.vertices[0];
		e.vertex_b = E.key.vertices[1];
		ERR_CONTINUE(!face_indices.has(E.value.left));
		ERR_CONTINUE(!face_indices.has(E.value.right));
		e.face_a = face_indices[E.value.left];
		e.face_b = face_indices[E.value.right];
		r_mesh.edges[idx++] = e;
	}

	r_mesh.vertices = p_points;

	return OK;
}


// --- Merged from core/math/math_vectors.cpp ---
#include "core/types.h"
#include "core/math/math_funcs.h"
#include "core/math/rect2.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/templates/hashfuncs.h"
#include "core/math/vector3.h"
#include "core/math/projection.h"
#include "core/string/ustring.h"

// --- vector2.cpp ---

real_t Vector2::angle() const {
	return Math::atan2(y, x);
}

Vector2 Vector2::from_angle(real_t p_angle) {
	return Vector2(Math::cos(p_angle), Math::sin(p_angle));
}

real_t Vector2::length() const {
	return Math::sqrt(x * x + y * y);
}

real_t Vector2::length_squared() const {
	return x * x + y * y;
}

void Vector2::normalize() {
	if (!is_finite()) {
#ifdef MATH_CHECKS
		WARN_PRINT("Vector2 cannot be normalized, the elements must be finite. Making (0, 0) as a fallback.");
#endif // MATH_CHECKS
		zero();
		return;
	}

	real_t l = length_squared();
	if (l == 0) {
		zero();
	} else {
		l = Math::sqrt(l);
		x /= l;
		y /= l;
	}
}

Vector2 Vector2::normalized() const {
	Vector2 v = *this;
	v.normalize();
	return v;
}

bool Vector2::is_normalized() const {
	// use length_squared() instead of length() to avoid sqrt(), makes it more stringent.
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON);
}

real_t Vector2::distance_to(const Vector2 &p_vector2) const {
	return Math::sqrt((x - p_vector2.x) * (x - p_vector2.x) + (y - p_vector2.y) * (y - p_vector2.y));
}

real_t Vector2::distance_squared_to(const Vector2 &p_vector2) const {
	return (x - p_vector2.x) * (x - p_vector2.x) + (y - p_vector2.y) * (y - p_vector2.y);
}

real_t Vector2::angle_to(const Vector2 &p_vector2) const {
	return Math::atan2(cross(p_vector2), dot(p_vector2));
}

real_t Vector2::angle_to_point(const Vector2 &p_vector2) const {
	return (p_vector2 - *this).angle();
}

real_t Vector2::dot(const Vector2 &p_other) const {
	return x * p_other.x + y * p_other.y;
}

real_t Vector2::cross(const Vector2 &p_other) const {
	return x * p_other.y - y * p_other.x;
}

Vector2 Vector2::sign() const {
	return Vector2(SIGN(x), SIGN(y));
}

Vector2 Vector2::floor() const {
	return Vector2(Math::floor(x), Math::floor(y));
}

Vector2 Vector2::ceil() const {
	return Vector2(Math::ceil(x), Math::ceil(y));
}

Vector2 Vector2::round() const {
	return Vector2(Math::round(x), Math::round(y));
}

Vector2 Vector2::rotated(real_t p_by) const {
	real_t sine = Math::sin(p_by);
	real_t cosi = Math::cos(p_by);
	return Vector2(
			x * cosi - y * sine,
			x * sine + y * cosi);
}

Vector2 Vector2::posmod(real_t p_mod) const {
	return Vector2(Math::fposmod(x, p_mod), Math::fposmod(y, p_mod));
}

Vector2 Vector2::posmodv(const Vector2 &p_modv) const {
	return Vector2(Math::fposmod(x, p_modv.x), Math::fposmod(y, p_modv.y));
}

Vector2 Vector2::project(const Vector2 &p_to) const {
	return p_to * (dot(p_to) / p_to.length_squared());
}

Vector2 Vector2::clamp(const Vector2 &p_min, const Vector2 &p_max) const {
	return Vector2(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y));
}

Vector2 Vector2::clampf(real_t p_min, real_t p_max) const {
	return Vector2(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max));
}

Vector2 Vector2::snapped(const Vector2 &p_step) const {
	return Vector2(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y));
}

Vector2 Vector2::snappedf(real_t p_step) const {
	return Vector2(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step));
}

Vector2 Vector2::limit_length(real_t p_len) const {
	const real_t l = length();
	Vector2 v = *this;
	if (l > 0 && p_len < l) {
		v /= l;
		v *= p_len;
	}

	return v;
}

Vector2 Vector2::move_toward(const Vector2 &p_to, real_t p_delta) const {
	Vector2 v = *this;
	Vector2 vd = p_to - v;
	real_t len = vd.length();
	return len <= p_delta || len < (real_t)CMP_EPSILON ? p_to : v + vd / len * p_delta;
}

// slide returns the component of the vector along the given plane, specified by its normal vector.
Vector2 Vector2::slide(const Vector2 &p_normal) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!p_normal.is_normalized(), Vector2(), "The normal Vector2 " + p_normal.operator String() + "must be normalized.");
#endif
	return *this - p_normal * dot(p_normal);
}

Vector2 Vector2::bounce(const Vector2 &p_normal) const {
	return -reflect(p_normal);
}

Vector2 Vector2::reflect(const Vector2 &p_normal) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!p_normal.is_normalized(), Vector2(), "The normal Vector2 " + p_normal.operator String() + "must be normalized.");
#endif
	return 2.0f * p_normal * dot(p_normal) - *this;
}

bool Vector2::is_equal_approx(const Vector2 &p_v) const {
	return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y);
}

bool Vector2::is_same(const Vector2 &p_v) const {
	return Math::is_same(x, p_v.x) && Math::is_same(y, p_v.y);
}

bool Vector2::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y);
}

bool Vector2::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y);
}

Vector2::operator String() const {
	return "(" + String::num_real(x, true) + ", " + String::num_real(y, true) + ")";
}

Vector2::operator Vector2i() const {
	return Vector2i(x, y);
}

// --- vector2i.cpp ---

Vector2i Vector2i::clamp(const Vector2i &p_min, const Vector2i &p_max) const {
	return Vector2i(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y));
}

Vector2i Vector2i::clampi(int32_t p_min, int32_t p_max) const {
	return Vector2i(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max));
}

Vector2i Vector2i::snapped(const Vector2i &p_step) const {
	return Vector2i(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y));
}

Vector2i Vector2i::snappedi(int32_t p_step) const {
	return Vector2i(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step));
}

int64_t Vector2i::length_squared() const {
	return x * (int64_t)x + y * (int64_t)y;
}

double Vector2i::length() const {
	return Math::sqrt((double)length_squared());
}

Vector2i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ")";
}

Vector2i::operator Vector2() const {
	return Vector2((int32_t)x, (int32_t)y);
}

// --- vector3.cpp ---

void Vector3::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = Basis(p_axis, p_angle).xform(*this);
}

Vector3 Vector3::rotated(const Vector3 &p_axis, real_t p_angle) const {
	Vector3 r = *this;
	r.rotate(p_axis, p_angle);
	return r;
}

void Vector3::snap(const Vector3 &p_step) {
	x = Math::snapped(x, p_step.x);
	y = Math::snapped(y, p_step.y);
	z = Math::snapped(z, p_step.z);
}

Vector3 Vector3::snapped(const Vector3 &p_step) const {
	Vector3 v = *this;
	v.snap(p_step);
	return v;
}

void Vector3::snapf(real_t p_step) {
	x = Math::snapped(x, p_step);
	y = Math::snapped(y, p_step);
	z = Math::snapped(z, p_step);
}

Vector3 Vector3::snappedf(real_t p_step) const {
	Vector3 v = *this;
	v.snapf(p_step);
	return v;
}

Vector3 Vector3::limit_length(real_t p_len) const {
	const real_t l = length();
	Vector3 v = *this;
	if (l > 0 && p_len < l) {
		v /= l;
		v *= p_len;
	}

	return v;
}

Vector3 Vector3::move_toward(const Vector3 &p_to, real_t p_delta) const {
	Vector3 v = *this;
	Vector3 vd = p_to - v;
	real_t len = vd.length();
	return len <= p_delta || len < (real_t)CMP_EPSILON ? p_to : v + vd / len * p_delta;
}

Vector2 Vector3::octahedron_encode() const {
	Vector3 n = *this;
	n /= Math::abs(n.x) + Math::abs(n.y) + Math::abs(n.z);
	Vector2 o;
	if (n.z >= 0.0f) {
		o.x = n.x;
		o.y = n.y;
	} else {
		o.x = (1.0f - Math::abs(n.y)) * (n.x >= 0.0f ? 1.0f : -1.0f);
		o.y = (1.0f - Math::abs(n.x)) * (n.y >= 0.0f ? 1.0f : -1.0f);
	}
	o.x = o.x * 0.5f + 0.5f;
	o.y = o.y * 0.5f + 0.5f;
	return o;
}

Vector3 Vector3::octahedron_decode(const Vector2 &p_oct) {
	Vector2 f(p_oct.x * 2.0f - 1.0f, p_oct.y * 2.0f - 1.0f);
	Vector3 n(f.x, f.y, 1.0f - Math::abs(f.x) - Math::abs(f.y));
	const real_t t = CLAMP(-n.z, 0.0f, 1.0f);
	n.x += n.x >= 0 ? -t : t;
	n.y += n.y >= 0 ? -t : t;
	return n.normalized();
}

Vector2 Vector3::octahedron_tangent_encode(float p_sign) const {
	const real_t bias = 1.0f / (real_t)32767.0f;
	Vector2 res = octahedron_encode();
	res.y = MAX(res.y, bias);
	res.y = res.y * 0.5f + 0.5f;
	res.y = p_sign >= 0.0f ? res.y : 1 - res.y;
	return res;
}

Vector3 Vector3::octahedron_tangent_decode(const Vector2 &p_oct, float *r_sign) {
	Vector2 oct_compressed = p_oct;
	oct_compressed.y = oct_compressed.y * 2 - 1;
	*r_sign = oct_compressed.y >= 0.0f ? 1.0f : -1.0f;
	oct_compressed.y = Math::abs(oct_compressed.y);
	Vector3 res = Vector3::octahedron_decode(oct_compressed);
	return res;
}

Basis Vector3::outer(const Vector3 &p_with) const {
	Basis basis;
	basis.rows[0] = Vector3(x * p_with.x, x * p_with.y, x * p_with.z);
	basis.rows[1] = Vector3(y * p_with.x, y * p_with.y, y * p_with.z);
	basis.rows[2] = Vector3(z * p_with.x, z * p_with.y, z * p_with.z);
	return basis;
}

bool Vector3::is_equal_approx(const Vector3 &p_v) const {
	return Math::is_equal_approx(x, p_v.x) && Math::is_equal_approx(y, p_v.y) && Math::is_equal_approx(z, p_v.z);
}

bool Vector3::is_same(const Vector3 &p_v) const {
	return Math::is_same(x, p_v.x) && Math::is_same(y, p_v.y) && Math::is_same(z, p_v.z);
}

bool Vector3::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y) && Math::is_zero_approx(z);
}

bool Vector3::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z);
}

Vector3::operator String() const {
	return "(" + String::num_real(x, true) + ", " + String::num_real(y, true) + ", " + String::num_real(z, true) + ")";
}

Vector3::operator Vector3i() const {
	return Vector3i(x, y, z);
}

// --- vector3i.cpp ---

Vector3i::Axis Vector3i::min_axis_index() const {
	return x < y ? (x < z ? Vector3i::AXIS_X : Vector3i::AXIS_Z) : (y < z ? Vector3i::AXIS_Y : Vector3i::AXIS_Z);
}

Vector3i::Axis Vector3i::max_axis_index() const {
	return x < y ? (y < z ? Vector3i::AXIS_Z : Vector3i::AXIS_Y) : (x < z ? Vector3i::AXIS_Z : Vector3i::AXIS_X);
}

Vector3i Vector3i::clamp(const Vector3i &p_min, const Vector3i &p_max) const {
	return Vector3i(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z));
}

Vector3i Vector3i::clampi(int32_t p_min, int32_t p_max) const {
	return Vector3i(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max));
}

Vector3i Vector3i::snapped(const Vector3i &p_step) const {
	return Vector3i(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y),
			Math::snapped(z, p_step.z));
}

Vector3i Vector3i::snappedi(int32_t p_step) const {
	return Vector3i(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step),
			Math::snapped(z, p_step));
}

Vector3i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ", " + itos(z) + ")";
}

Vector3i::operator Vector3() const {
	return Vector3(x, y, z);
}

// --- vector4.cpp ---

Vector4::Axis Vector4::min_axis_index() const {
	uint32_t min_index = 0;
	real_t min_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) <= min_value) {
			min_index = i;
			min_value = operator[](i);
		}
	}
	return Vector4::Axis(min_index);
}

Vector4::Axis Vector4::max_axis_index() const {
	uint32_t max_index = 0;
	real_t max_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) > max_value) {
			max_index = i;
			max_value = operator[](i);
		}
	}
	return Vector4::Axis(max_index);
}

bool Vector4::is_equal_approx(const Vector4 &p_vec4) const {
	return Math::is_equal_approx(x, p_vec4.x) && Math::is_equal_approx(y, p_vec4.y) && Math::is_equal_approx(z, p_vec4.z) && Math::is_equal_approx(w, p_vec4.w);
}

bool Vector4::is_same(const Vector4 &p_vec4) const {
	return Math::is_same(x, p_vec4.x) && Math::is_same(y, p_vec4.y) && Math::is_same(z, p_vec4.z) && Math::is_same(w, p_vec4.w);
}

bool Vector4::is_zero_approx() const {
	return Math::is_zero_approx(x) && Math::is_zero_approx(y) && Math::is_zero_approx(z) && Math::is_zero_approx(w);
}

bool Vector4::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

real_t Vector4::length() const {
	return Math::sqrt(length_squared());
}

void Vector4::normalize() {
	if (!is_finite()) {
#ifdef MATH_CHECKS
		WARN_PRINT("Vector4 cannot be normalized, the elements must be finite. Making (0, 0, 0, 0) as a fallback.");
#endif // MATH_CHECKS
		zero();
		return;
	}

	real_t l = length_squared();
	if (l == 0) {
		zero();
	} else {
		l = Math::sqrt(l);
		x /= l;
		y /= l;
		z /= l;
		w /= l;
	}
}

Vector4 Vector4::normalized() const {
	Vector4 v = *this;
	v.normalize();
	return v;
}

bool Vector4::is_normalized() const {
	return Math::is_equal_approx(length_squared(), (real_t)1, (real_t)UNIT_EPSILON);
}

real_t Vector4::distance_to(const Vector4 &p_to) const {
	return (p_to - *this).length();
}

real_t Vector4::distance_squared_to(const Vector4 &p_to) const {
	return (p_to - *this).length_squared();
}

Vector4 Vector4::direction_to(const Vector4 &p_to) const {
	Vector4 ret(p_to.x - x, p_to.y - y, p_to.z - z, p_to.w - w);
	ret.normalize();
	return ret;
}

Vector4 Vector4::abs() const {
	return Vector4(Math::abs(x), Math::abs(y), Math::abs(z), Math::abs(w));
}

Vector4 Vector4::sign() const {
	return Vector4(SIGN(x), SIGN(y), SIGN(z), SIGN(w));
}

Vector4 Vector4::floor() const {
	return Vector4(Math::floor(x), Math::floor(y), Math::floor(z), Math::floor(w));
}

Vector4 Vector4::ceil() const {
	return Vector4(Math::ceil(x), Math::ceil(y), Math::ceil(z), Math::ceil(w));
}

Vector4 Vector4::round() const {
	return Vector4(Math::round(x), Math::round(y), Math::round(z), Math::round(w));
}

Vector4 Vector4::lerp(const Vector4 &p_to, real_t p_weight) const {
	Vector4 res = *this;
	res.x = Math::lerp(res.x, p_to.x, p_weight);
	res.y = Math::lerp(res.y, p_to.y, p_weight);
	res.z = Math::lerp(res.z, p_to.z, p_weight);
	res.w = Math::lerp(res.w, p_to.w, p_weight);
	return res;
}

Vector4 Vector4::cubic_interpolate(const Vector4 &p_b, const Vector4 &p_pre_a, const Vector4 &p_post_b, real_t p_weight) const {
	Vector4 res = *this;
	res.x = Math::cubic_interpolate(res.x, p_b.x, p_pre_a.x, p_post_b.x, p_weight);
	res.y = Math::cubic_interpolate(res.y, p_b.y, p_pre_a.y, p_post_b.y, p_weight);
	res.z = Math::cubic_interpolate(res.z, p_b.z, p_pre_a.z, p_post_b.z, p_weight);
	res.w = Math::cubic_interpolate(res.w, p_b.w, p_pre_a.w, p_post_b.w, p_weight);
	return res;
}

Vector4 Vector4::cubic_interpolate_in_time(const Vector4 &p_b, const Vector4 &p_pre_a, const Vector4 &p_post_b, real_t p_weight, real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const {
	Vector4 res = *this;
	res.x = Math::cubic_interpolate_in_time(res.x, p_b.x, p_pre_a.x, p_post_b.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.y = Math::cubic_interpolate_in_time(res.y, p_b.y, p_pre_a.y, p_post_b.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.z = Math::cubic_interpolate_in_time(res.z, p_b.z, p_pre_a.z, p_post_b.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	res.w = Math::cubic_interpolate_in_time(res.w, p_b.w, p_pre_a.w, p_post_b.w, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	return res;
}

Vector4 Vector4::posmod(real_t p_mod) const {
	return Vector4(Math::fposmod(x, p_mod), Math::fposmod(y, p_mod), Math::fposmod(z, p_mod), Math::fposmod(w, p_mod));
}

Vector4 Vector4::posmodv(const Vector4 &p_modv) const {
	return Vector4(Math::fposmod(x, p_modv.x), Math::fposmod(y, p_modv.y), Math::fposmod(z, p_modv.z), Math::fposmod(w, p_modv.w));
}

void Vector4::snap(const Vector4 &p_step) {
	x = Math::snapped(x, p_step.x);
	y = Math::snapped(y, p_step.y);
	z = Math::snapped(z, p_step.z);
	w = Math::snapped(w, p_step.w);
}

void Vector4::snapf(real_t p_step) {
	x = Math::snapped(x, p_step);
	y = Math::snapped(y, p_step);
	z = Math::snapped(z, p_step);
	w = Math::snapped(w, p_step);
}

Vector4 Vector4::snapped(const Vector4 &p_step) const {
	Vector4 v = *this;
	v.snap(p_step);
	return v;
}

Vector4 Vector4::snappedf(real_t p_step) const {
	Vector4 v = *this;
	v.snapf(p_step);
	return v;
}

Vector4 Vector4::inverse() const {
	return Vector4(1.0f / x, 1.0f / y, 1.0f / z, 1.0f / w);
}

Vector4 Vector4::clamp(const Vector4 &p_min, const Vector4 &p_max) const {
	return Vector4(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z),
			CLAMP(w, p_min.w, p_max.w));
}

Vector4 Vector4::clampf(real_t p_min, real_t p_max) const {
	return Vector4(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max),
			CLAMP(w, p_min, p_max));
}

Vector4::operator String() const {
	return "(" + String::num_real(x, true) + ", " + String::num_real(y, true) + ", " + String::num_real(z, true) + ", " + String::num_real(w, true) + ")";
}

static_assert(sizeof(Vector4) == 4 * sizeof(real_t));

Vector4::operator Vector4i() const {
	return Vector4i(x, y, z, w);
}

// --- vector4i.cpp ---

Vector4i::Axis Vector4i::min_axis_index() const {
	uint32_t min_index = 0;
	int32_t min_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) <= min_value) {
			min_index = i;
			min_value = operator[](i);
		}
	}
	return Vector4i::Axis(min_index);
}

Vector4i::Axis Vector4i::max_axis_index() const {
	uint32_t max_index = 0;
	int32_t max_value = x;
	for (uint32_t i = 1; i < 4; i++) {
		if (operator[](i) > max_value) {
			max_index = i;
			max_value = operator[](i);
		}
	}
	return Vector4i::Axis(max_index);
}

Vector4i Vector4i::clamp(const Vector4i &p_min, const Vector4i &p_max) const {
	return Vector4i(
			CLAMP(x, p_min.x, p_max.x),
			CLAMP(y, p_min.y, p_max.y),
			CLAMP(z, p_min.z, p_max.z),
			CLAMP(w, p_min.w, p_max.w));
}

Vector4i Vector4i::clampi(int32_t p_min, int32_t p_max) const {
	return Vector4i(
			CLAMP(x, p_min, p_max),
			CLAMP(y, p_min, p_max),
			CLAMP(z, p_min, p_max),
			CLAMP(w, p_min, p_max));
}

Vector4i Vector4i::snapped(const Vector4i &p_step) const {
	return Vector4i(
			Math::snapped(x, p_step.x),
			Math::snapped(y, p_step.y),
			Math::snapped(z, p_step.z),
			Math::snapped(w, p_step.w));
}

Vector4i Vector4i::snappedi(int32_t p_step) const {
	return Vector4i(
			Math::snapped(x, p_step),
			Math::snapped(y, p_step),
			Math::snapped(z, p_step),
			Math::snapped(w, p_step));
}

Vector4i::operator String() const {
	return "(" + itos(x) + ", " + itos(y) + ", " + itos(z) + ", " + itos(w) + ")";
}

Vector4i::operator Vector4() const {
	return Vector4(x, y, z, w);
}

static_assert(sizeof(Vector4i) == 4 * sizeof(int32_t));

// --- rect2.cpp ---

bool Rect2::is_equal_approx(const Rect2 &p_rect) const {
	return position.is_equal_approx(p_rect.position) && size.is_equal_approx(p_rect.size);
}

bool Rect2::is_same(const Rect2 &p_rect) const {
	return position.is_same(p_rect.position) && size.is_same(p_rect.size);
}

bool Rect2::is_finite() const {
	return position.is_finite() && size.is_finite();
}

bool Rect2::intersects_segment(const Point2 &p_from, const Point2 &p_to, Point2 *r_pos, Point2 *r_normal) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0)) {
		ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a Rect2 with a positive size.");
	}
#endif
	real_t min = 0, max = 1;
	int axis = 0;
	real_t sign = 0;

	for (int i = 0; i < 2; i++) {
		real_t seg_from = p_from[i];
		real_t seg_to = p_to[i];
		real_t box_begin = position[i];
		real_t box_end = box_begin + size[i];
		real_t cmin, cmax;
		real_t csign;

		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;
			csign = -1.0;

		} else {
			if (seg_to > box_end || seg_from < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
			csign = 1.0;
		}

		if (cmin > min) {
			min = cmin;
			axis = i;
			sign = csign;
		}
		if (cmax < max) {
			max = cmax;
		}
		if (max < min) {
			return false;
		}
	}

	Vector2 rel = p_to - p_from;

	if (r_normal) {
		Vector2 normal;
		normal[axis] = sign;
		*r_normal = normal;
	}

	if (r_pos) {
		*r_pos = p_from + rel * min;
	}

	return true;
}

bool Rect2::intersects_transformed(const Transform2D &p_xform, const Rect2 &p_rect) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
		ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a Rect2 with a positive size.");
	}
#endif
	//SAT intersection between local and transformed rect2

	Vector2 xf_points[4] = {
		p_xform.xform(p_rect.position),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y)),
		p_xform.xform(Vector2(p_rect.position.x, p_rect.position.y + p_rect.size.y)),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y)),
	};

	real_t low_limit;

	//base rect2 first (faster)

	if (xf_points[0].y > position.y) {
		goto next1;
	}
	if (xf_points[1].y > position.y) {
		goto next1;
	}
	if (xf_points[2].y > position.y) {
		goto next1;
	}
	if (xf_points[3].y > position.y) {
		goto next1;
	}

	return false;

next1:

	low_limit = position.y + size.y;

	if (xf_points[0].y < low_limit) {
		goto next2;
	}
	if (xf_points[1].y < low_limit) {
		goto next2;
	}
	if (xf_points[2].y < low_limit) {
		goto next2;
	}
	if (xf_points[3].y < low_limit) {
		goto next2;
	}

	return false;

next2:

	if (xf_points[0].x > position.x) {
		goto next3;
	}
	if (xf_points[1].x > position.x) {
		goto next3;
	}
	if (xf_points[2].x > position.x) {
		goto next3;
	}
	if (xf_points[3].x > position.x) {
		goto next3;
	}

	return false;

next3:

	low_limit = position.x + size.x;

	if (xf_points[0].x < low_limit) {
		goto next4;
	}
	if (xf_points[1].x < low_limit) {
		goto next4;
	}
	if (xf_points[2].x < low_limit) {
		goto next4;
	}
	if (xf_points[3].x < low_limit) {
		goto next4;
	}

	return false;

next4:

	Vector2 xf_points2[4] = {
		position,
		Vector2(position.x + size.x, position.y),
		Vector2(position.x, position.y + size.y),
		Vector2(position.x + size.x, position.y + size.y),
	};

	real_t maxa = p_xform.columns[0].dot(xf_points2[0]);
	real_t mina = maxa;

	real_t dp = p_xform.columns[0].dot(xf_points2[1]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[0].dot(xf_points2[2]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[0].dot(xf_points2[3]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	real_t maxb = p_xform.columns[0].dot(xf_points[0]);
	real_t minb = maxb;

	dp = p_xform.columns[0].dot(xf_points[1]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[0].dot(xf_points[2]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[0].dot(xf_points[3]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	if (mina > maxb) {
		return false;
	}
	if (minb > maxa) {
		return false;
	}

	maxa = p_xform.columns[1].dot(xf_points2[0]);
	mina = maxa;

	dp = p_xform.columns[1].dot(xf_points2[1]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[1].dot(xf_points2[2]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	dp = p_xform.columns[1].dot(xf_points2[3]);
	maxa = MAX(dp, maxa);
	mina = MIN(dp, mina);

	maxb = p_xform.columns[1].dot(xf_points[0]);
	minb = maxb;

	dp = p_xform.columns[1].dot(xf_points[1]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[1].dot(xf_points[2]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	dp = p_xform.columns[1].dot(xf_points[3]);
	maxb = MAX(dp, maxb);
	minb = MIN(dp, minb);

	if (mina > maxb) {
		return false;
	}
	if (minb > maxa) {
		return false;
	}

	return true;
}

Rect2 Rect2::intersection_transformed(const Transform2D &p_xform, const Rect2 &p_rect) const {
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || p_rect.size.x < 0 || p_rect.size.y < 0)) {
		ERR_PRINT("Rect2 size is negative, this is not supported. Use Rect2.abs() to get a Rect2 with a positive size.");
	}
#endif

	if ((Math::is_zero_approx(p_xform.columns[0].y) && Math::is_zero_approx(p_xform.columns[1].x)) ||
			(Math::is_zero_approx(p_xform.columns[0].x) && Math::is_zero_approx(p_xform.columns[1].y))) {
		return intersection(p_xform.xform(p_rect));
	}

	if (!intersects_transformed(p_xform, p_rect)) {
		return Rect2();
	}

	const Vector2 xf_points[4] = {
		p_xform.xform(p_rect.position),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y)),
		p_xform.xform(Vector2(p_rect.position.x + p_rect.size.x, p_rect.position.y + p_rect.size.y)),
		p_xform.xform(Vector2(p_rect.position.x, p_rect.position.y + p_rect.size.y)),
	};

	// Use Sutherland–Hodgman algorithm.

	Vector2 subject[8];
	int subject_count = 4;
	subject[0] = xf_points[0];
	subject[1] = xf_points[1];
	subject[2] = xf_points[2];
	subject[3] = xf_points[3];

	const Vector2 min = position;
	const Vector2 max = position + size;
	Vector2 intersected;

	for (int edge = 0; edge < 4; edge++) {
		const int axis = edge % 2;
		const int another_axis = 1 - axis;
		const bool is_min = (edge < 2);
		intersected[axis] = is_min ? min[axis] : max[axis];

		Vector2 output[8];
		int output_count = 0;

		Vector2 prev = subject[subject_count - 1];
		bool prev_in_halfplane = is_min ? (prev[axis] >= intersected[axis]) : (prev[axis] <= intersected[axis]);

		for (int i = 0; i < subject_count; i++) {
			const Vector2 &curr = subject[i];
			bool curr_in_halfplane = is_min ? (curr[axis] >= intersected[axis]) : (curr[axis] <= intersected[axis]);
			if (prev_in_halfplane != curr_in_halfplane) {
				// Entering/exiting the half-plane.
				real_t t = (intersected[axis] - prev[axis]) / (curr[axis] - prev[axis]);
				intersected[another_axis] = prev[another_axis] + (curr[another_axis] - prev[another_axis]) * t;
				output[output_count++] = intersected;
			}
			if (curr_in_halfplane) {
				output[output_count++] = curr;
			}
			prev = curr;
			prev_in_halfplane = curr_in_halfplane;
		}

		for (int i = 0; i < output_count; i++) {
			subject[i] = output[i];
		}

		subject_count = output_count;
		if (subject_count == 0) {
			break;
		}
	}

	if (subject_count > 0) {
		return Rect2::from_points(subject, subject_count);
	}

	// Perform a reverse containment test; the current rect may be inside the transformed rect.

	const Vector2 corners[4] = {
		position,
		Vector2(max.x, min.y),
		max,
		Vector2(min.x, max.y)
	};

	Vector2 inside_points[4];
	int inside_count = 0;

	for (int point_idx = 0; point_idx < 4; point_idx++) {
		bool has_pos = false;
		bool has_neg = false;
		for (int idx = 0; idx < 4; idx++) {
			const int next_idx = (idx + 1) % 4;
			Vector2 v0 = xf_points[next_idx] - xf_points[idx];
			Vector2 v1 = corners[point_idx] - xf_points[idx];
			real_t cross = v0.cross(v1);
			if (cross > CMP_EPSILON) {
				has_pos = true;
			}
			if (cross < -CMP_EPSILON) {
				has_neg = true;
			}
			if (has_pos && has_neg) {
				break;
			}
		}
		if (!(has_pos && has_neg)) {
			inside_points[inside_count++] = corners[point_idx];
		}
	}

	return inside_count > 0 ? Rect2::from_points(inside_points, inside_count) : Rect2();
}

Rect2::operator String() const {
	return "[P: " + position.operator String() + ", S: " + size.operator String() + "]";
}

Rect2::operator Rect2i() const {
	return Rect2i(position, size);
}

// --- rect2i.cpp ---

Rect2i::operator String() const {
	return "[P: " + String(position) + ", S: " + String(size) + "]";
}

Rect2i::operator Rect2() const {
	return Rect2(position, size);
}


// --- Merged from core/math/math_primitives.cpp ---
#include "core/error/error_macros.h"
#include "core/math/aabb.h"
#include "core/templates/hashfuncs.h"
#include "core/math/color_names.inc"
#include "core/types.h"
#include "core/math/geometry_3d.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/types.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "modules/modules_enabled.gen.h"

#include <thirdparty/misc/ok_color.h>

// --- plane.cpp ---

void Plane::set_normal(const Vector3& p_normal) { normal = p_normal; }

void Plane::normalize()
{
#ifdef MATH_CHECKS
	if (!is_finite()) {
		WARN_PRINT("Plane cannot be normalized, the distance and the normal should be finite.");
	}
#endif // MATH_CHECKS

	real_t l = normal.length();
	if (l == 0) {
		zero();
		return;
	}
	normal /= l;
	d /= l;
}

Plane Plane::normalized() const
{
	Plane p = *this;
	p.normalize();
	return p;
}

Vector3 Plane::get_any_perpendicular_normal() const
{
	static const Vector3 p1 = Vector3(1, 0, 0);
	static const Vector3 p2 = Vector3(0, 1, 0);
	Vector3 p;

	if (Math::abs(normal.dot(p1)) > 0.99f) { // if too similar to p1
		p = p2;								 // use p2
	}
	else {
		p = p1; // use p1
	}

	p -= normal * normal.dot(p);
	p.normalize();

	return p;
}

/* intersections */

bool Plane::intersect_3(const Plane& p_plane1, const Plane& p_plane2, Vector3* r_result) const
{
	const Plane& p_plane0 = *this;
	Vector3 normal0 = p_plane0.normal;
	Vector3 normal1 = p_plane1.normal;
	Vector3 normal2 = p_plane2.normal;

	real_t denom = vec3_cross(normal0, normal1).dot(normal2);

	if (Math::is_zero_approx(denom)) {
		return false;
	}

	if (r_result) {
		*r_result = ((vec3_cross(normal1, normal2) * p_plane0.d) +
						(vec3_cross(normal2, normal0) * p_plane1.d) +
						(vec3_cross(normal0, normal1) * p_plane2.d)) /
					denom;
	}

	return true;
}

bool Plane::intersects_ray(
	const Vector3& p_from, const Vector3& p_dir, Vector3* p_intersection) const
{
	Vector3 segment = p_dir;
	real_t den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (normal.dot(p_from) - d) / den;

	if (dist >
		(real_t)CMP_EPSILON) { // this is a ray, before the emitting pos (p_from) doesn't exist

		return false;
	}

	dist = -dist;
	*p_intersection = p_from + segment * dist;

	return true;
}

bool Plane::intersects_segment(
	const Vector3& p_begin, const Vector3& p_end, Vector3* p_intersection) const
{
	Vector3 segment = p_begin - p_end;
	real_t den = normal.dot(segment);

	if (Math::is_zero_approx(den)) {
		return false;
	}

	real_t dist = (normal.dot(p_begin) - d) / den;

	if (dist < (real_t)-CMP_EPSILON || dist > (1.0f + (real_t)CMP_EPSILON)) {
		return false;
	}

	dist = -dist;
	*p_intersection = p_begin + segment * dist;

	return true;
}

/* misc */

bool Plane::is_equal_approx_any_side(const Plane& p_plane) const
{
	return (normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d)) ||
		   (normal.is_equal_approx(-p_plane.normal) && Math::is_equal_approx(d, -p_plane.d));
}

bool Plane::is_equal_approx(const Plane& p_plane) const
{
	return normal.is_equal_approx(p_plane.normal) && Math::is_equal_approx(d, p_plane.d);
}

bool Plane::is_same(const Plane& p_plane) const
{
	return normal.is_same(p_plane.normal) && Math::is_same(d, p_plane.d);
}

bool Plane::is_finite() const { return normal.is_finite() && Math::is_finite(d); }

Plane::operator String() const
{
	return "[N: " + normal.operator String() + ", D: " + String::num_real(d, false) + "]";
}

// --- face3.cpp ---

int Face3::split_by_plane(const Plane &p_plane, Face3 p_res[3], bool p_is_point_over[3]) const {
	ERR_FAIL_COND_V(is_degenerate(), 0);

	Vector3 above[4];
	int above_count = 0;

	Vector3 below[4];
	int below_count = 0;

	for (int i = 0; i < 3; i++) {
		if (p_plane.has_point(vertex[i], (real_t)CMP_EPSILON)) { // point is in plane

			ERR_FAIL_COND_V(above_count >= 4, 0);
			above[above_count++] = vertex[i];
			ERR_FAIL_COND_V(below_count >= 4, 0);
			below[below_count++] = vertex[i];

		} else {
			if (p_plane.is_point_over(vertex[i])) {
				//Point is over
				ERR_FAIL_COND_V(above_count >= 4, 0);
				above[above_count++] = vertex[i];

			} else {
				//Point is under
				ERR_FAIL_COND_V(below_count >= 4, 0);
				below[below_count++] = vertex[i];
			}

			/* Check for Intersection between this and the next vertex*/

			Vector3 inters;
			if (!p_plane.intersects_segment(vertex[i], vertex[(i + 1) % 3], &inters)) {
				continue;
			}

			/* Intersection goes to both */
			ERR_FAIL_COND_V(above_count >= 4, 0);
			above[above_count++] = inters;
			ERR_FAIL_COND_V(below_count >= 4, 0);
			below[below_count++] = inters;
		}
	}

	int polygons_created = 0;

	ERR_FAIL_COND_V(above_count >= 4 && below_count >= 4, 0); //bug in the algo

	if (above_count >= 3) {
		p_res[polygons_created] = Face3(above[0], above[1], above[2]);
		p_is_point_over[polygons_created] = true;
		polygons_created++;

		if (above_count == 4) {
			p_res[polygons_created] = Face3(above[2], above[3], above[0]);
			p_is_point_over[polygons_created] = true;
			polygons_created++;
		}
	}

	if (below_count >= 3) {
		p_res[polygons_created] = Face3(below[0], below[1], below[2]);
		p_is_point_over[polygons_created] = false;
		polygons_created++;

		if (below_count == 4) {
			p_res[polygons_created] = Face3(below[2], below[3], below[0]);
			p_is_point_over[polygons_created] = false;
			polygons_created++;
		}
	}

	return polygons_created;
}

bool Face3::intersects_ray(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection) const {
	return Geometry3D::ray_intersects_triangle(p_from, p_dir, vertex[0], vertex[1], vertex[2], p_intersection);
}

bool Face3::intersects_segment(const Vector3 &p_from, const Vector3 &p_dir, Vector3 *p_intersection) const {
	return Geometry3D::segment_intersects_triangle(p_from, p_dir, vertex[0], vertex[1], vertex[2], p_intersection);
}

bool Face3::is_degenerate() const {
	Vector3 normal = vec3_cross(vertex[0] - vertex[1], vertex[0] - vertex[2]);
	return (normal.length_squared() < (real_t)CMP_EPSILON2);
}

Vector3 Face3::get_random_point_inside() const {
	real_t a = Math::random(0.0, 1.0);
	real_t b = Math::random(0.0, 1.0);
	if (a > b) {
		SWAP(a, b);
	}

	return vertex[0] * a + vertex[1] * (b - a) + vertex[2] * (1.0f - b);
}

Plane Face3::get_plane(ClockDirection p_dir) const {
	return Plane(vertex[0], vertex[1], vertex[2], p_dir);
}

real_t Face3::get_area() const {
	return vec3_cross(vertex[0] - vertex[1], vertex[0] - vertex[2]).length() * 0.5f;
}

bool Face3::intersects_aabb(const AABB &p_aabb) const {
	/** TEST PLANE **/
	if (!p_aabb.intersects_plane(get_plane())) {
		return false;
	}

#define TEST_AXIS(m_ax) \
	/** TEST FACE AXIS */ \
	{ \
		real_t aabb_min = p_aabb.position.m_ax; \
		real_t aabb_max = p_aabb.position.m_ax + p_aabb.size.m_ax; \
		real_t tri_min = vertex[0].m_ax; \
		real_t tri_max = vertex[0].m_ax; \
		for (int i = 1; i < 3; i++) { \
			if (vertex[i].m_ax > tri_max) \
				tri_max = vertex[i].m_ax; \
			if (vertex[i].m_ax < tri_min) \
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

	/** TEST ALL EDGES **/

	const Vector3 edge_norms[3] = {
		vertex[0] - vertex[1],
		vertex[1] - vertex[2],
		vertex[2] - vertex[0],
	};

	for (int i = 0; i < 12; i++) {
		Vector3 from, to;
		p_aabb.get_edge(i, from, to);
		Vector3 e1 = from - to;
		for (int j = 0; j < 3; j++) {
			Vector3 e2 = edge_norms[j];

			Vector3 axis = vec3_cross(e1, e2);

			if (axis.length_squared() < 0.0001f) {
				continue; // coplanar
			}
			axis.normalize();

			real_t minA, maxA, minB, maxB;
			p_aabb.project_range_in_plane(Plane(axis), minA, maxA);
			project_range(axis, Transform3D(), minB, maxB);

			if (maxA < minB || maxB < minA) {
				return false;
			}
		}
	}
	return true;
}

Face3::operator String() const {
	return String() + String(vertex[0]) + ", " + String(vertex[1]) + ", " + String(vertex[2]);
}

void Face3::project_range(const Vector3 &p_normal, const Transform3D &p_transform, real_t &r_min, real_t &r_max) const {
	for (int i = 0; i < 3; i++) {
		Vector3 v = p_transform.xform(vertex[i]);
		real_t d = p_normal.dot(v);

		if (i == 0 || d > r_max) {
			r_max = d;
		}

		if (i == 0 || d < r_min) {
			r_min = d;
		}
	}
}

void Face3::get_support(const Vector3 &p_normal, const Transform3D &p_transform, Vector3 *p_vertices, int *p_count, int p_max) const {
	constexpr double face_support_threshold = 0.98;
	constexpr double edge_support_threshold = 0.05;

	if (p_max <= 0) {
		return;
	}

	Vector3 n = p_transform.basis.xform_inv(p_normal);

	/** TEST FACE AS SUPPORT **/
	if (get_plane().normal.dot(n) > face_support_threshold) {
		*p_count = MIN(3, p_max);

		for (int i = 0; i < *p_count; i++) {
			p_vertices[i] = p_transform.xform(vertex[i]);
		}

		return;
	}

	/** FIND SUPPORT VERTEX **/

	int vert_support_idx = -1;
	real_t support_max = 0;

	for (int i = 0; i < 3; i++) {
		real_t d = n.dot(vertex[i]);

		if (i == 0 || d > support_max) {
			support_max = d;
			vert_support_idx = i;
		}
	}

	/** TEST EDGES AS SUPPORT **/

	for (int i = 0; i < 3; i++) {
		if (i != vert_support_idx && i + 1 != vert_support_idx) {
			continue;
		}

		// check if edge is valid as a support
		real_t dot = (vertex[i] - vertex[(i + 1) % 3]).normalized().dot(n);
		dot = Math::abs(dot);
		if (dot < edge_support_threshold) {
			*p_count = MIN(2, p_max);

			for (int j = 0; j < *p_count; j++) {
				p_vertices[j] = p_transform.xform(vertex[(j + i) % 3]);
			}

			return;
		}
	}

	*p_count = 1;
	p_vertices[0] = p_transform.xform(vertex[vert_support_idx]);
}

Vector3 Face3::get_closest_point_to(const Vector3 &p_point) const {
	Vector3 edge0 = vertex[1] - vertex[0];
	Vector3 edge1 = vertex[2] - vertex[0];
	Vector3 v0 = vertex[0] - p_point;

	real_t a = edge0.dot(edge0);
	real_t b = edge0.dot(edge1);
	real_t c = edge1.dot(edge1);
	real_t d = edge0.dot(v0);
	real_t e = edge1.dot(v0);

	real_t det = a * c - b * b;
	real_t s = b * e - c * d;
	real_t t = b * d - a * e;

	if (s + t < det) {
		if (s < 0.f) {
			if (t < 0.f) {
				if (d < 0.f) {
					s = CLAMP(-d / a, 0.f, 1.f);
					t = 0.f;
				} else {
					s = 0.f;
					t = CLAMP(-e / c, 0.f, 1.f);
				}
			} else {
				s = 0.f;
				t = CLAMP(-e / c, 0.f, 1.f);
			}
		} else if (t < 0.f) {
			s = CLAMP(-d / a, 0.f, 1.f);
			t = 0.f;
		} else {
			real_t invDet = 1.f / det;
			s *= invDet;
			t *= invDet;
		}
	} else {
		if (s < 0.f) {
			real_t tmp0 = b + d;
			real_t tmp1 = c + e;
			if (tmp1 > tmp0) {
				real_t numer = tmp1 - tmp0;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				t = CLAMP(-e / c, 0.f, 1.f);
				s = 0.f;
			}
		} else if (t < 0.f) {
			if (a + d > b + e) {
				real_t numer = c + e - b - d;
				real_t denom = a - 2 * b + c;
				s = CLAMP(numer / denom, 0.f, 1.f);
				t = 1 - s;
			} else {
				s = CLAMP(-d / a, 0.f, 1.f);
				t = 0.f;
			}
		} else {
			real_t numer = c + e - b - d;
			real_t denom = a - 2 * b + c;
			s = CLAMP(numer / denom, 0.f, 1.f);
			t = 1.f - s;
		}
	}

	return vertex[0] + s * edge0 + t * edge1;
}

// --- aabb.cpp ---

real_t AABB::get_volume() const { return size.x * size.y * size.z; }

void AABB::merge_with(const AABB& p_aabb)
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 ||
				 p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 beg_1, beg_2;
	Vector3 end_1, end_2;
	Vector3 min, max;

	beg_1 = position;
	beg_2 = p_aabb.position;
	end_1 = size + beg_1;
	end_2 = p_aabb.size + beg_2;

	min.x = (beg_1.x < beg_2.x) ? beg_1.x : beg_2.x;
	min.y = (beg_1.y < beg_2.y) ? beg_1.y : beg_2.y;
	min.z = (beg_1.z < beg_2.z) ? beg_1.z : beg_2.z;

	max.x = (end_1.x > end_2.x) ? end_1.x : end_2.x;
	max.y = (end_1.y > end_2.y) ? end_1.y : end_2.y;
	max.z = (end_1.z > end_2.z) ? end_1.z : end_2.z;

	position = min;
	size = max - min;
}

bool AABB::is_equal_approx(const AABB& p_aabb) const
{
	return position.is_equal_approx(p_aabb.position) && size.is_equal_approx(p_aabb.size);
}

bool AABB::is_same(const AABB& p_aabb) const
{
	return position.is_same(p_aabb.position) && size.is_same(p_aabb.size);
}

bool AABB::is_finite() const { return position.is_finite() && size.is_finite(); }

AABB AABB::intersection(const AABB& p_aabb) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0 || p_aabb.size.x < 0 || p_aabb.size.y < 0 ||
				 p_aabb.size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 src_min = position;
	Vector3 src_max = position + size;
	Vector3 dst_min = p_aabb.position;
	Vector3 dst_max = p_aabb.position + p_aabb.size;

	Vector3 min, max;

	if (src_min.x > dst_max.x || src_max.x < dst_min.x) {
		return AABB();
	}
	else {
		min.x = (src_min.x > dst_min.x) ? src_min.x : dst_min.x;
		max.x = (src_max.x < dst_max.x) ? src_max.x : dst_max.x;
	}

	if (src_min.y > dst_max.y || src_max.y < dst_min.y) {
		return AABB();
	}
	else {
		min.y = (src_min.y > dst_min.y) ? src_min.y : dst_min.y;
		max.y = (src_max.y < dst_max.y) ? src_max.y : dst_max.y;
	}

	if (src_min.z > dst_max.z || src_max.z < dst_min.z) {
		return AABB();
	}
	else {
		min.z = (src_min.z > dst_min.z) ? src_min.z : dst_min.z;
		max.z = (src_max.z < dst_max.z) ? src_max.z : dst_max.z;
	}

	return AABB(min, max - min);
}

// Note that this routine returns the BACKTRACKED (i.e. behind the ray origin)
// intersection point + normal if INSIDE the AABB.
// The caller can therefore decide when INSIDE whether to use the
// backtracked intersection, or use p_from as the intersection, and
// carry on progressing without e.g. reflecting against the normal.
bool AABB::find_intersects_ray(const Vector3& p_from, const Vector3& p_dir, bool& r_inside,
	Vector3* r_intersection_point, Vector3* r_normal) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	Vector3 end = position + size;
	real_t tmin = -1e20;
	real_t tmax = 1e20;
	int axis = 0;

	// Make sure r_inside is always initialized,
	// to prevent reading uninitialized data in the client code.
	r_inside = false;

	for (int i = 0; i < 3; i++) {
		if (p_dir[i] == 0) {
			if ((p_from[i] < position[i]) || (p_from[i] > end[i])) {
				return false;
			}
		}
		else { // ray not parallel to planes in this direction
			real_t t1 = (position[i] - p_from[i]) / p_dir[i];
			real_t t2 = (end[i] - p_from[i]) / p_dir[i];

			if (t1 > t2) {
				SWAP(t1, t2);
			}
			if (t1 >= tmin) {
				tmin = t1;
				axis = i;
			}
			if (t2 < tmax) {
				if (t2 < 0) {
					return false;
				}
				tmax = t2;
			}
			if (tmin > tmax) {
				return false;
			}
		}
	}

	// Did the ray start from inside the box?
	// In which case the intersection returned is the point of entry
	// (behind the ray start) or the calling routine can use the ray origin as intersection point.
	r_inside = tmin < 0;

	if (r_intersection_point) {
		*r_intersection_point = p_from + p_dir * tmin;

		// Prevent float error by making sure the point is exactly
		// on the AABB border on the relevant axis.
		(*r_intersection_point)[axis] = (p_dir[axis] >= 0) ? position[axis] : end[axis];
	}
	if (r_normal) {
		*r_normal = Vector3();
		(*r_normal)[axis] = (p_dir[axis] >= 0) ? -1 : 1;
	}

	return true;
}

bool AABB::intersects_segment(const Vector3& p_from, const Vector3& p_to,
	Vector3* r_intersection_point, Vector3* r_normal) const
{
#ifdef MATH_CHECKS
	if (unlikely(size.x < 0 || size.y < 0 || size.z < 0)) {
		ERR_PRINT("AABB size is negative, this is not supported. Use AABB.abs() to get an AABB "
				  "with a positive size.");
	}
#endif
	real_t min = 0, max = 1;
	int axis = 0;
	real_t sign = 0;

	for (int i = 0; i < 3; i++) {
		real_t seg_from = p_from[i];
		real_t seg_to = p_to[i];
		real_t box_begin = position[i];
		real_t box_end = box_begin + size[i];
		real_t cmin, cmax;
		real_t csign;

		if (seg_from < seg_to) {
			if (seg_from > box_end || seg_to < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from < box_begin) ? ((box_begin - seg_from) / length) : 0;
			cmax = (seg_to > box_end) ? ((box_end - seg_from) / length) : 1;
			csign = -1.0;

		}
		else {
			if (seg_to > box_end || seg_from < box_begin) {
				return false;
			}
			real_t length = seg_to - seg_from;
			cmin = (seg_from > box_end) ? (box_end - seg_from) / length : 0;
			cmax = (seg_to < box_begin) ? (box_begin - seg_from) / length : 1;
			csign = 1.0;
		}

		if (cmin > min) {
			min = cmin;
			axis = i;
			sign = csign;
		}
		if (cmax < max) {
			max = cmax;
		}
		if (max < min) {
			return false;
		}
	}

	Vector3 rel = p_to - p_from;

	if (r_normal) {
		Vector3 normal;
		normal[axis] = sign;
		*r_normal = normal;
	}

	if (r_intersection_point) {
		*r_intersection_point = p_from + rel * min;
	}

	return true;
}

bool AABB::intersects_plane(const Plane& p_plane) const
{
	Vector3 points[8] = {
		Vector3(position.x, position.y, position.z),
		Vector3(position.x, position.y, position.z + size.z),
		Vector3(position.x, position.y + size.y, position.z),
		Vector3(position.x, position.y + size.y, position.z + size.z),
		Vector3(position.x + size.x, position.y, position.z),
		Vector3(position.x + size.x, position.y, position.z + size.z),
		Vector3(position.x + size.x, position.y + size.y, position.z),
		Vector3(position.x + size.x, position.y + size.y, position.z + size.z),
	};

	bool over = false;
	bool under = false;

	for (int i = 0; i < 8; i++) {
		if (p_plane.distance_to(points[i]) > 0) {
			over = true;
		}
		else {
			under = true;
		}
	}

	return under && over;
}

Vector3 AABB::get_longest_axis() const
{
	Vector3 axis(1, 0, 0);
	real_t max_size = size.x;

	if (size.y > max_size) {
		axis = Vector3(0, 1, 0);
		max_size = size.y;
	}

	if (size.z > max_size) {
		axis = Vector3(0, 0, 1);
	}

	return axis;
}

int AABB::get_longest_axis_index() const
{
	int axis = 0;
	real_t max_size = size.x;

	if (size.y > max_size) {
		axis = 1;
		max_size = size.y;
	}

	if (size.z > max_size) {
		axis = 2;
	}

	return axis;
}

Vector3 AABB::get_shortest_axis() const
{
	Vector3 axis(1, 0, 0);
	real_t min_size = size.x;

	if (size.y < min_size) {
		axis = Vector3(0, 1, 0);
		min_size = size.y;
	}

	if (size.z < min_size) {
		axis = Vector3(0, 0, 1);
	}

	return axis;
}

int AABB::get_shortest_axis_index() const
{
	int axis = 0;
	real_t min_size = size.x;

	if (size.y < min_size) {
		axis = 1;
		min_size = size.y;
	}

	if (size.z < min_size) {
		axis = 2;
	}

	return axis;
}

AABB AABB::merge(const AABB& p_with) const
{
	AABB aabb = *this;
	aabb.merge_with(p_with);
	return aabb;
}

AABB AABB::expand(const Vector3& p_vector) const
{
	AABB aabb = *this;
	aabb.expand_to(p_vector);
	return aabb;
}

AABB AABB::grow(real_t p_by) const
{
	AABB aabb = *this;
	aabb.grow_by(p_by);
	return aabb;
}

void AABB::get_edge(int p_edge, Vector3& r_from, Vector3& r_to) const
{
	ERR_FAIL_INDEX(p_edge, 12);
	switch (p_edge) {
	case 0: {
		r_from = Vector3(position.x + size.x, position.y, position.z);
		r_to = Vector3(position.x, position.y, position.z);
	} break;
	case 1: {
		r_from = Vector3(position.x + size.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y, position.z);
	} break;
	case 2: {
		r_from = Vector3(position.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y, position.z + size.z);

	} break;
	case 3: {
		r_from = Vector3(position.x, position.y, position.z);
		r_to = Vector3(position.x, position.y, position.z + size.z);

	} break;
	case 4: {
		r_from = Vector3(position.x, position.y + size.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z);
	} break;
	case 5: {
		r_from = Vector3(position.x + size.x, position.y + size.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
	} break;
	case 6: {
		r_from = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z + size.z);

	} break;
	case 7: {
		r_from = Vector3(position.x, position.y + size.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z);

	} break;
	case 8: {
		r_from = Vector3(position.x, position.y, position.z + size.z);
		r_to = Vector3(position.x, position.y + size.y, position.z + size.z);

	} break;
	case 9: {
		r_from = Vector3(position.x, position.y, position.z);
		r_to = Vector3(position.x, position.y + size.y, position.z);

	} break;
	case 10: {
		r_from = Vector3(position.x + size.x, position.y, position.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z);

	} break;
	case 11: {
		r_from = Vector3(position.x + size.x, position.y, position.z + size.z);
		r_to = Vector3(position.x + size.x, position.y + size.y, position.z + size.z);


	} break;
	}
}

AABB::operator String() const { return "[P: " + String(position) + ", S: " + String(size) + "]"; }

// --- color.cpp ---

uint32_t Color::to_argb32() const {
	uint32_t c = (uint8_t)Math::round(a * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(r * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);

	return c;
}

uint32_t Color::to_abgr32() const {
	uint32_t c = (uint8_t)Math::round(a * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(r * 255.0f);

	return c;
}

uint32_t Color::to_rgba32() const {
	uint32_t c = (uint8_t)Math::round(r * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(g * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(b * 255.0f);
	c <<= 8;
	c |= (uint8_t)Math::round(a * 255.0f);

	return c;
}

uint64_t Color::to_abgr64() const {
	uint64_t c = (uint16_t)Math::round(a * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(r * 65535.0f);

	return c;
}

uint64_t Color::to_argb64() const {
	uint64_t c = (uint16_t)Math::round(a * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(r * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);

	return c;
}

uint64_t Color::to_rgba64() const {
	uint64_t c = (uint16_t)Math::round(r * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(g * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(b * 65535.0f);
	c <<= 16;
	c |= (uint16_t)Math::round(a * 65535.0f);

	return c;
}

void _append_hex(float p_val, char32_t *r_string) {
	int v = Math::round(p_val * 255.0f);
	v = CLAMP(v, 0, 255);

	r_string[0] = hex_char_table_lower[(v >> 4) & 0xF];
	r_string[1] = hex_char_table_lower[v & 0xF];
}

String Color::to_html(bool p_alpha) const {
	String txt;
	txt.resize_uninitialized(p_alpha ? 9 : 7);
	char32_t *ptr = txt.ptrw();

	_append_hex(r, ptr + 0);
	_append_hex(g, ptr + 2);
	_append_hex(b, ptr + 4);
	if (p_alpha) {
		_append_hex(a, ptr + 6);
	}
	ptr[txt.size() - 1] = '\0';

	return txt;
}

float Color::get_h() const {
	float min = MIN(r, g);
	min = MIN(min, b);
	float max = MAX(r, g);
	max = MAX(max, b);

	float delta = max - min;

	if (delta == 0.0f) {
		return 0.0f;
	}

	float h;
	if (r == max) {
		h = (g - b) / delta; // between yellow & magenta
	} else if (g == max) {
		h = 2 + (b - r) / delta; // between cyan & yellow
	} else {
		h = 4 + (r - g) / delta; // between magenta & cyan
	}

	h /= 6.0f;
	if (h < 0.0f) {
		h += 1.0f;
	}

	return h;
}

float Color::get_s() const {
	float min = MIN(r, g);
	min = MIN(min, b);
	float max = MAX(r, g);
	max = MAX(max, b);

	float delta = max - min;

	return (max != 0.0f) ? (delta / max) : 0.0f;
}

float Color::get_v() const {
	float max = MAX(r, g);
	max = MAX(max, b);
	return max;
}

void Color::set_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	int i;
	float f, p, q, t;
	a = p_alpha;

	if (p_s == 0.0f) {
		// Achromatic (gray)
		r = g = b = p_v;
		return;
	}

	p_h *= 6.0f;
	p_h = Math::fmod(p_h, 6);
	i = Math::floor(p_h);

	f = p_h - i;
	p = p_v * (1.0f - p_s);
	q = p_v * (1.0f - p_s * f);
	t = p_v * (1.0f - p_s * (1.0f - f));

	switch (i) {
		case 0: // Red is the dominant color
			r = p_v;
			g = t;
			b = p;
			break;
		case 1: // Green is the dominant color
			r = q;
			g = p_v;
			b = p;
			break;
		case 2:
			r = p;
			g = p_v;
			b = t;
			break;
		case 3: // Blue is the dominant color
			r = p;
			g = q;
			b = p_v;
			break;
		case 4:
			r = t;
			g = p;
			b = p_v;
			break;
		default: // (5) Red is the dominant color
			r = p_v;
			g = p;
			b = q;
			break;
	}
}

void Color::set_ok_hsl(float p_h, float p_s, float p_l, float p_alpha) {
	ok_color::HSL hsl;
	hsl.h = p_h;
	hsl.s = p_s;
	hsl.l = p_l;
	ok_color::RGB rgb = ok_color::okhsl_to_srgb(hsl);
	Color c = Color(rgb.r, rgb.g, rgb.b, p_alpha).clamp();
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

void Color::set_ok_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	ok_color::HSV hsv;
	hsv.h = p_h;
	hsv.s = p_s;
	hsv.v = p_v;
	ok_color::RGB rgb = ok_color::okhsv_to_srgb(hsv);
	Color c = Color(rgb.r, rgb.g, rgb.b, p_alpha).clamp();
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

bool Color::is_equal_approx(const Color &p_color) const {
	return Math::is_equal_approx(r, p_color.r) && Math::is_equal_approx(g, p_color.g) && Math::is_equal_approx(b, p_color.b) && Math::is_equal_approx(a, p_color.a);
}

bool Color::is_same(const Color &p_color) const {
	return Math::is_same(r, p_color.r) && Math::is_same(g, p_color.g) && Math::is_same(b, p_color.b) && Math::is_same(a, p_color.a);
}

Color Color::clamp(const Color &p_min, const Color &p_max) const {
	return Color(
			CLAMP(r, p_min.r, p_max.r),
			CLAMP(g, p_min.g, p_max.g),
			CLAMP(b, p_min.b, p_max.b),
			CLAMP(a, p_min.a, p_max.a));
}

void Color::invert() {
	r = 1.0f - r;
	g = 1.0f - g;
	b = 1.0f - b;
}

Color Color::hex(uint32_t p_hex) {
	float a = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float b = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float g = (p_hex & 0xFF) / 255.0f;
	p_hex >>= 8;
	float r = (p_hex & 0xFF) / 255.0f;

	return Color(r, g, b, a);
}

Color Color::hex64(uint64_t p_hex) {
	float a = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float b = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float g = (p_hex & 0xFFFF) / 65535.0f;
	p_hex >>= 16;
	float r = (p_hex & 0xFFFF) / 65535.0f;

	return Color(r, g, b, a);
}

static int _parse_col4(const String &p_str, int p_ofs) {
	char character = p_str[p_ofs];

	if (character >= '0' && character <= '9') {
		return character - '0';
	} else if (character >= 'a' && character <= 'f') {
		return character + (10 - 'a');
	} else if (character >= 'A' && character <= 'F') {
		return character + (10 - 'A');
	}
	return -1;
}

static int _parse_col8(const String &p_str, int p_ofs) {
	return _parse_col4(p_str, p_ofs) * 16 + _parse_col4(p_str, p_ofs + 1);
}

Color Color::inverted() const {
	Color c = *this;
	c.invert();
	return c;
}

Color Color::html(const String &p_rgba) {
	if (p_rgba.is_empty()) {
		return Color();
	}

	const int current_pos = (p_rgba[0] == '#') ? 1 : 0;
	const int num_of_digits = p_rgba.length() - current_pos;

	float r, g, b, a = 1.0f;

	if (num_of_digits == 3) {
		// #rgb
		r = _parse_col4(p_rgba, current_pos) / 15.0f;
		g = _parse_col4(p_rgba, current_pos + 1) / 15.0f;
		b = _parse_col4(p_rgba, current_pos + 2) / 15.0f;
	} else if (num_of_digits == 4) {
		r = _parse_col4(p_rgba, current_pos) / 15.0f;
		g = _parse_col4(p_rgba, current_pos + 1) / 15.0f;
		b = _parse_col4(p_rgba, current_pos + 2) / 15.0f;
		a = _parse_col4(p_rgba, current_pos + 3) / 15.0f;
	} else if (num_of_digits == 6) {
		r = _parse_col8(p_rgba, current_pos) / 255.0f;
		g = _parse_col8(p_rgba, current_pos + 2) / 255.0f;
		b = _parse_col8(p_rgba, current_pos + 4) / 255.0f;
	} else if (num_of_digits == 8) {
		r = _parse_col8(p_rgba, current_pos) / 255.0f;
		g = _parse_col8(p_rgba, current_pos + 2) / 255.0f;
		b = _parse_col8(p_rgba, current_pos + 4) / 255.0f;
		a = _parse_col8(p_rgba, current_pos + 6) / 255.0f;
	} else {
		ERR_FAIL_V_MSG(Color(), "Invalid color code: " + p_rgba + ".");
	}

	ERR_FAIL_COND_V_MSG(r < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(g < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(b < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");
	ERR_FAIL_COND_V_MSG(a < 0.0f, Color(), "Invalid color code: " + p_rgba + ".");

	return Color(r, g, b, a);
}

bool Color::html_is_valid(const String &p_color) {
	String color = p_color;

	if (color.is_empty()) {
		return false;
	}

	const int current_pos = (color[0] == '#') ? 1 : 0;
	const int len = color.length();
	const int num_of_digits = len - current_pos;
	if (!(num_of_digits == 3 || num_of_digits == 4 || num_of_digits == 6 || num_of_digits == 8)) {
		return false;
	}

	// Check if each hex digit is valid.
	for (int i = current_pos; i < len; i++) {
		if (!is_hex_digit(p_color[i])) {
			return false;
		}
	}

	return true;
}

Color Color::named(const String &p_name) {
	int idx = find_named_color(p_name);
	if (idx == -1) {
		ERR_FAIL_V_MSG(Color(), "Invalid color name: " + p_name + ".");
	}
	return named_colors[idx].color;
}

Color Color::named(const String &p_name, const Color &p_default) {
	int idx = find_named_color(p_name);
	if (idx == -1) {
		return p_default;
	}
	return named_colors[idx].color;
}

int Color::find_named_color(const String &p_name) {
	String name = p_name;
	// Normalize name.
	name = name.remove_chars(" -_'.");
	name = name.to_upper();

	static HashMap<String, int> named_colors_hashmap;
	if (unlikely(named_colors_hashmap.is_empty())) {
		const int named_color_count = get_named_color_count();
		for (int i = 0; i < named_color_count; i++) {
			named_colors_hashmap[String(named_colors[i].name).remove_char('_')] = i;
		}
	}

	const HashMap<String, int>::ConstIterator E = named_colors_hashmap.find(name);
	if (E) {
		return E->value;
	}

	return -1;
}

int Color::get_named_color_count() {
	return std_size(named_colors);
}

String Color::get_named_color_name(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, get_named_color_count(), "");
	return named_colors[p_idx].name;
}

Color Color::get_named_color(int p_idx) {
	ERR_FAIL_INDEX_V(p_idx, get_named_color_count(), Color());
	return named_colors[p_idx].color;
}

// For a version that errors on invalid values instead of returning
// a default color, use the Color(String) constructor instead.
Color Color::from_string(const String &p_string, const Color &p_default) {
	if (html_is_valid(p_string)) {
		return html(p_string);
	} else {
		return named(p_string, p_default);
	}
}

Color Color::from_hsv(float p_h, float p_s, float p_v, float p_alpha) {
	Color c;
	c.set_hsv(p_h, p_s, p_v, p_alpha);
	return c;
}

Color Color::from_rgbe9995(uint32_t p_rgbe) {
	float r = p_rgbe & 0x1ff;
	float g = (p_rgbe >> 9) & 0x1ff;
	float b = (p_rgbe >> 18) & 0x1ff;
	float e = (p_rgbe >> 27);
	float m = Math::pow(2.0f, e - 15.0f - 9.0f);

	float rd = r * m;
	float gd = g * m;
	float bd = b * m;

	return Color(rd, gd, bd, 1.0f);
}

Color Color::from_rgba8(int64_t p_r8, int64_t p_g8, int64_t p_b8, int64_t p_a8) {
	return Color(p_r8 / 255.0f, p_g8 / 255.0f, p_b8 / 255.0f, p_a8 / 255.0f);
}

Color::operator String() const {
	return "(" + String::num(r, 4) + ", " + String::num(g, 4) + ", " + String::num(b, 4) + ", " + String::num(a, 4) + ")";
}

Color Color::from_ok_hsl(float p_h, float p_s, float p_l, float p_alpha) {
	Color c;
	c.set_ok_hsl(p_h, p_s, p_l, p_alpha);
	return c;
}

Color Color::from_ok_hsv(float p_h, float p_s, float p_l, float p_alpha) {
	Color c;
	c.set_ok_hsv(p_h, p_s, p_l, p_alpha);
	return c;
}

float Color::get_ok_hsl_h() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.h)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.h, 0.0f, 1.0f);
}

float Color::get_ok_hsl_s() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.s)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.s, 0.0f, 1.0f);
}

float Color::get_ok_hsl_l() const {
	ok_color::RGB rgb;
	rgb.r = r;
	rgb.g = g;
	rgb.b = b;
	ok_color::HSL ok_hsl = ok_color::srgb_to_okhsl(rgb);
	if (Math::is_nan(ok_hsl.l)) {
		return 0.0f;
	}
	return CLAMP(ok_hsl.l, 0.0f, 1.0f);
}

// --- math_funcs.cpp ---

static RandomPCG default_rand;

uint32_t Math::rand_from_seed(uint64_t *p_seed) {
	RandomPCG rng = RandomPCG(*p_seed);
	uint32_t r = rng.rand();
	*p_seed = rng.get_seed();
	return r;
}

void Math::seed(uint64_t p_value) {
	default_rand.seed(p_value);
}

void Math::randomize() {
	default_rand.randomize();
}

uint32_t Math::rand() {
	return default_rand.rand();
}

double Math::randfn(double p_mean, double p_deviation) {
	return default_rand.randfn(p_mean, p_deviation);
}

int Math::step_decimals(double p_step) {
	static const int maxn = 10;
	static const double sd[maxn] = {
		0.9999, // somehow compensate for floating point error
		0.09999,
		0.009999,
		0.0009999,
		0.00009999,
		0.000009999,
		0.0000009999,
		0.00000009999,
		0.000000009999,
		0.0000000009999
	};

	double abs = Math::abs(p_step);
	double decs = abs - (int)abs; // Strip away integer part
	for (int i = 0; i < maxn; i++) {
		if (decs >= sd[i]) {
			return i;
		}
	}

	return 0;
}

// Only meant for editor usage in float ranges, where a step of 0
// means that decimal digits should not be limited in String::num.
int Math::range_step_decimals(double p_step) {
	if (p_step < 0.0000000000001) {
		return 16; // Max value hardcoded in String::num
	}
	return step_decimals(p_step);
}

double Math::ease(double p_x, double p_c) {
	if (p_x < 0) {
		p_x = 0;
	} else if (p_x > 1.0) {
		p_x = 1.0;
	}
	if (p_c > 0) {
		if (p_c < 1.0) {
			return 1.0 - Math::pow(1.0 - p_x, 1.0 / p_c);
		} else {
			return Math::pow(p_x, p_c);
		}
	} else if (p_c < 0) {
		//inout ease

		if (p_x < 0.5) {
			return Math::pow(p_x * 2.0, -p_c) * 0.5;
		} else {
			return (1.0 - Math::pow(1.0 - (p_x - 0.5) * 2.0, -p_c)) * 0.5 + 0.5;
		}
	} else {
		return 0; // no ease (raw)
	}
}

double Math::snapped(double p_value, double p_step) {
	if (p_step != 0) {
		p_value = Math::floor(p_value / p_step + 0.5) * p_step;
	}
	return p_value;
}

uint32_t Math::larger_prime(uint32_t p_val) {
	static const uint32_t primes[] = {
		5,
		13,
		23,
		47,
		97,
		193,
		389,
		769,
		1543,
		3079,
		6151,
		12289,
		24593,
		49157,
		98317,
		196613,
		393241,
		786433,
		1572869,
		3145739,
		6291469,
		12582917,
		25165843,
		50331653,
		100663319,
		201326611,
		402653189,
		805306457,
		1610612741,
		0,
	};

	int idx = 0;
	while (true) {
		ERR_FAIL_COND_V(primes[idx] == 0, 0);
		if (primes[idx] > p_val) {
			return primes[idx];
		}
		idx++;
	}
}

double Math::random(double p_from, double p_to) {
	return default_rand.random(p_from, p_to);
}

float Math::random(float p_from, float p_to) {
	return default_rand.random(p_from, p_to);
}

int Math::random(int p_from, int p_to) {
	return default_rand.random(p_from, p_to);
}

// --- random_pcg.cpp ---

RandomPCG::RandomPCG(uint64_t p_seed, uint64_t p_inc) :
		pcg(),
		current_inc(p_inc) {
	seed(p_seed);
}

void RandomPCG::randomize() {
	seed(((uint64_t)OS::get_unix_time() + OS::get_ticks_usec()) * pcg.state + PCG_DEFAULT_INC_64);
}

int64_t RandomPCG::rand_weighted(const Vector<float> &p_weights) {
	ERR_FAIL_COND_V_MSG(p_weights.is_empty(), -1, "Weights array is empty.");
	int64_t weights_size = p_weights.size();
	const float *weights = p_weights.ptr();
	float weights_sum = 0.0;
	for (int64_t i = 0; i < weights_size; ++i) {
		ERR_FAIL_COND_V_MSG(weights[i] < 0.0, -1, vformat("Weight array contains the negative value %f at index %d.", weights[i], i));
		weights_sum += weights[i];
	}

	float remaining_distance = randf() * weights_sum;
	for (int64_t i = 0; i < weights_size; ++i) {
		remaining_distance -= weights[i];
		if (remaining_distance < 0) {
			return i;
		}
	}

	for (int64_t i = weights_size - 1; i >= 0; --i) {
		if (weights[i] > 0) {
			return i;
		}
	}
	return -1;
}

double RandomPCG::random(double p_from, double p_to) {
	return randd() * (p_to - p_from) + p_from;
}

float RandomPCG::random(float p_from, float p_to) {
	return randf() * (p_to - p_from) + p_from;
}

int RandomPCG::random(int p_from, int p_to) {
	if (p_from == p_to) {
		return p_from;
	}

	int64_t min = MIN(p_from, p_to);
	int64_t max = MAX(p_from, p_to);
	uint32_t diff = static_cast<uint32_t>(max - min);

	if (diff == UINT32_MAX) {
		// Can't add 1 to max uint32_t value for inclusive range, so call rand without passing bounds.
		return static_cast<int64_t>(rand()) + min;
	}

	return static_cast<int64_t>(rand(diff + 1U)) + min;
}

// --- static_raycaster.cpp ---

#if !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)

Ref<StaticRaycaster> StaticRaycaster::create() { return Ref<StaticRaycaster>(); }

void StaticRaycaster::free() {}

bool StaticRaycaster::intersect(Ray& p_ray) { return false; }

void StaticRaycaster::intersect(Vector<Ray>& r_rays) {}

void StaticRaycaster::add_mesh(
	const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices, unsigned int p_id)
{
}

void StaticRaycaster::commit() {}

void StaticRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids) {}

void StaticRaycaster::clear_mesh_filter() {}

StaticRaycaster::StaticRaycaster() {}

StaticRaycaster::~StaticRaycaster() {}

#endif // !defined(MODULE_RAYCAST_ENABLED) || !defined(TOOLS_ENABLED)


// --- Merged from core/math/math_transforms.cpp ---
#include "core/math/aabb.h"
#include "core/types.h"
#include "core/math/math_funcs.h"
#include "core/math/plane.h"
#include "core/math/projection.h"
#include "core/types.h"
#include "core/math/rect2.h"
#include "core/types.h"
#include "core/math/geometry_3d.h"
#include "core/string/ustring.h"

// --- quaternion.cpp ---

real_t Quaternion::angle_to(const Quaternion &p_to) const {
	real_t d = dot(p_to);
	// acos does clamping.
	return Math::acos(d * d * 2 - 1);
}

Vector3 Quaternion::get_euler(EulerOrder p_order) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Vector3(0, 0, 0), "The quaternion " + operator String() + " must be normalized.");
#endif
	return Basis(*this).get_euler(p_order);
}

bool Quaternion::is_equal_approx(const Quaternion &p_quaternion) const {
	return Math::is_equal_approx(x, p_quaternion.x) && Math::is_equal_approx(y, p_quaternion.y) && Math::is_equal_approx(z, p_quaternion.z) && Math::is_equal_approx(w, p_quaternion.w);
}

bool Quaternion::is_same(const Quaternion &p_quaternion) const {
	return Math::is_same(x, p_quaternion.x) && Math::is_same(y, p_quaternion.y) && Math::is_same(z, p_quaternion.z) && Math::is_same(w, p_quaternion.w);
}

bool Quaternion::is_finite() const {
	return Math::is_finite(x) && Math::is_finite(y) && Math::is_finite(z) && Math::is_finite(w);
}

real_t Quaternion::length() const {
	return Math::sqrt(length_squared());
}

void Quaternion::normalize() {
#ifdef MATH_CHECKS
	if (!is_finite()) {
		WARN_PRINT("Quaternion cannot be normalized, the elements should be finite.");
	}
#endif // MATH_CHECKS
	*this /= length();
}

Quaternion Quaternion::normalized() const {
	return *this / length();
}

bool Quaternion::is_normalized() const {
	return Math::is_equal_approx(length_squared(), 1, (real_t)UNIT_EPSILON); //use less epsilon
}

Quaternion Quaternion::inverse() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The quaternion " + operator String() + " must be normalized.");
#endif
	return Quaternion(-x, -y, -z, w);
}

Quaternion Quaternion::log() const {
	Quaternion src = *this;
	Vector3 src_v = src.get_axis() * src.get_angle();
	return Quaternion(src_v.x, src_v.y, src_v.z, 0);
}

Quaternion Quaternion::exp() const {
	Quaternion src = *this;
	Vector3 src_v = Vector3(src.x, src.y, src.z);
	real_t theta = src_v.length();
	src_v = src_v.normalized();
	if (theta < CMP_EPSILON || !src_v.is_normalized()) {
		return Quaternion(0, 0, 0, 1);
	}
	return Quaternion(src_v, theta);
}

Quaternion Quaternion::slerp(const Quaternion &p_to, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion " + p_to.operator String() + " must be normalized.");
#endif
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = dot(p_to);

	// adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1 = -p_to;
	} else {
		to1 = p_to;
	}

	// calculate coefficients

	if ((1.0f - cosom) > (real_t)CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0 - p_weight) * omega) / sinom;
		scale1 = Math::sin(p_weight * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_weight;
		scale1 = p_weight;
	}
	// calculate final values
	return Quaternion(
			scale0 * x + scale1 * to1.x,
			scale0 * y + scale1 * to1.y,
			scale0 * z + scale1 * to1.z,
			scale0 * w + scale1 * to1.w);
}

Quaternion Quaternion::slerpni(const Quaternion &p_to, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_to.is_normalized(), Quaternion(), "The end quaternion " + p_to.operator String() + " must be normalized.");
#endif
	const Quaternion &from = *this;

	real_t dot = from.dot(p_to);

	if (Math::abs(dot) > 0.9999f) {
		return from;
	}

	real_t theta = Math::acos(dot),
		   sinT = 1.0f / Math::sin(theta),
		   newFactor = Math::sin(p_weight * theta) * sinT,
		   invFactor = Math::sin((1.0f - p_weight) * theta) * sinT;

	return Quaternion(invFactor * from.x + newFactor * p_to.x,
			invFactor * from.y + newFactor * p_to.y,
			invFactor * from.z + newFactor * p_to.z,
			invFactor * from.w + newFactor * p_to.w);
}

Quaternion Quaternion::spherical_cubic_interpolate(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion " + p_b.operator String() + " must be normalized.");
#endif
	Quaternion from_q = *this;
	Quaternion pre_q = p_pre_a;
	Quaternion to_q = p_b;
	Quaternion post_q = p_post_b;

	// Align flip phases.
	from_q = Basis(from_q).get_rotation_quaternion();
	pre_q = Basis(pre_q).get_rotation_quaternion();
	to_q = Basis(to_q).get_rotation_quaternion();
	post_q = Basis(post_q).get_rotation_quaternion();

	// Flip quaternions to shortest path if necessary.
	bool flip1 = std::signbit(from_q.dot(pre_q));
	pre_q = flip1 ? -pre_q : pre_q;
	bool flip2 = std::signbit(from_q.dot(to_q));
	to_q = flip2 ? -to_q : to_q;
	bool flip3 = flip2 ? to_q.dot(post_q) <= 0 : std::signbit(to_q.dot(post_q));
	post_q = flip3 ? -post_q : post_q;

	// Calc by Expmap in from_q space.
	Quaternion ln_from = Quaternion(0, 0, 0, 0);
	Quaternion ln_to = (from_q.inverse() * to_q).log();
	Quaternion ln_pre = (from_q.inverse() * pre_q).log();
	Quaternion ln_post = (from_q.inverse() * post_q).log();
	Quaternion ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
	ln.y = Math::cubic_interpolate(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
	ln.z = Math::cubic_interpolate(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
	Quaternion q1 = from_q * ln.exp();

	// Calc by Expmap in to_q space.
	ln_from = (to_q.inverse() * from_q).log();
	ln_to = Quaternion(0, 0, 0, 0);
	ln_pre = (to_q.inverse() * pre_q).log();
	ln_post = (to_q.inverse() * post_q).log();
	ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight);
	ln.y = Math::cubic_interpolate(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight);
	ln.z = Math::cubic_interpolate(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight);
	Quaternion q2 = to_q * ln.exp();

	// To cancel error made by Expmap ambiguity, do blending.
	return q1.slerp(q2, p_weight);
}

Quaternion Quaternion::spherical_cubic_interpolate_in_time(const Quaternion &p_b, const Quaternion &p_pre_a, const Quaternion &p_post_b, real_t p_weight,
		real_t p_b_t, real_t p_pre_a_t, real_t p_post_b_t) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_normalized(), Quaternion(), "The start quaternion " + operator String() + " must be normalized.");
	ERR_FAIL_COND_V_MSG(!p_b.is_normalized(), Quaternion(), "The end quaternion " + p_b.operator String() + " must be normalized.");
#endif
	Quaternion from_q = *this;
	Quaternion pre_q = p_pre_a;
	Quaternion to_q = p_b;
	Quaternion post_q = p_post_b;

	// Align flip phases.
	from_q = Basis(from_q).get_rotation_quaternion();
	pre_q = Basis(pre_q).get_rotation_quaternion();
	to_q = Basis(to_q).get_rotation_quaternion();
	post_q = Basis(post_q).get_rotation_quaternion();

	// Flip quaternions to shortest path if necessary.
	bool flip1 = std::signbit(from_q.dot(pre_q));
	pre_q = flip1 ? -pre_q : pre_q;
	bool flip2 = std::signbit(from_q.dot(to_q));
	to_q = flip2 ? -to_q : to_q;
	bool flip3 = flip2 ? to_q.dot(post_q) <= 0 : std::signbit(to_q.dot(post_q));
	post_q = flip3 ? -post_q : post_q;

	// Calc by Expmap in from_q space.
	Quaternion ln_from = Quaternion(0, 0, 0, 0);
	Quaternion ln_to = (from_q.inverse() * to_q).log();
	Quaternion ln_pre = (from_q.inverse() * pre_q).log();
	Quaternion ln_post = (from_q.inverse() * post_q).log();
	Quaternion ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate_in_time(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.y = Math::cubic_interpolate_in_time(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.z = Math::cubic_interpolate_in_time(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	Quaternion q1 = from_q * ln.exp();

	// Calc by Expmap in to_q space.
	ln_from = (to_q.inverse() * from_q).log();
	ln_to = Quaternion(0, 0, 0, 0);
	ln_pre = (to_q.inverse() * pre_q).log();
	ln_post = (to_q.inverse() * post_q).log();
	ln = Quaternion(0, 0, 0, 0);
	ln.x = Math::cubic_interpolate_in_time(ln_from.x, ln_to.x, ln_pre.x, ln_post.x, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.y = Math::cubic_interpolate_in_time(ln_from.y, ln_to.y, ln_pre.y, ln_post.y, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	ln.z = Math::cubic_interpolate_in_time(ln_from.z, ln_to.z, ln_pre.z, ln_post.z, p_weight, p_b_t, p_pre_a_t, p_post_b_t);
	Quaternion q2 = to_q * ln.exp();

	// To cancel error made by Expmap ambiguity, do blending.
	return q1.slerp(q2, p_weight);
}

Quaternion::operator String() const {
	return "(" + String::num_real(x, false) + ", " + String::num_real(y, false) + ", " + String::num_real(z, false) + ", " + String::num_real(w, false) + ")";
}

Vector3 Quaternion::get_axis() const {
	if (Math::abs(w) > 1 - CMP_EPSILON) {
		return Vector3(x, y, z);
	}
	real_t r = ((real_t)1) / Math::sqrt(1 - w * w);
	return Vector3(x * r, y * r, z * r);
}

real_t Quaternion::get_angle() const {
	return 2 * Math::acos(w);
}

Quaternion::Quaternion(const Vector3 &p_axis, real_t p_angle) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 " + p_axis.operator String() + " must be normalized.");
#endif
	real_t d = p_axis.length();
	if (d == 0) {
		x = 0;
		y = 0;
		z = 0;
		w = 0;
	} else {
		real_t sin_angle = Math::sin(p_angle * 0.5f);
		real_t cos_angle = Math::cos(p_angle * 0.5f);
		real_t s = sin_angle / d;
		x = p_axis.x * s;
		y = p_axis.y * s;
		z = p_axis.z * s;
		w = cos_angle;
	}
}

// Euler constructor expects a vector containing the Euler angles in the format
// (ax, ay, az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// This implementation uses YXZ convention (Z is the first rotation).
Quaternion Quaternion::from_euler(const Vector3 &p_euler) {
	real_t half_a1 = p_euler.y * 0.5f;
	real_t half_a2 = p_euler.x * 0.5f;
	real_t half_a3 = p_euler.z * 0.5f;

	// R = Y(a1).X(a2).Z(a3) convention for Euler angles.
	// Conversion to quaternion as listed in https://ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19770024290.pdf (page A-6)
	// a3 is the angle of the first rotation, following the notation in this reference.

	real_t cos_a1 = Math::cos(half_a1);
	real_t sin_a1 = Math::sin(half_a1);
	real_t cos_a2 = Math::cos(half_a2);
	real_t sin_a2 = Math::sin(half_a2);
	real_t cos_a3 = Math::cos(half_a3);
	real_t sin_a3 = Math::sin(half_a3);

	return Quaternion(
			sin_a1 * cos_a2 * sin_a3 + cos_a1 * sin_a2 * cos_a3,
			sin_a1 * cos_a2 * cos_a3 - cos_a1 * sin_a2 * sin_a3,
			-sin_a1 * sin_a2 * cos_a3 + cos_a1 * cos_a2 * sin_a3,
			sin_a1 * sin_a2 * sin_a3 + cos_a1 * cos_a2 * cos_a3);
}

// --- basis.cpp ---

#define cofac(row1, col1, row2, col2) \
	(rows[row1][col1] * rows[row2][col2] - rows[row1][col2] * rows[row2][col1])

void Basis::invert() {
	real_t co[3] = {
		cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1)
	};
	real_t det = rows[0][0] * co[0] +
			rows[0][1] * co[1] +
			rows[0][2] * co[2];
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t s = 1.0f / det;

	set(co[0] * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
			co[1] * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
			co[2] * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
}

#undef cofac

void Basis::orthonormalize() {
	// Orthonormalizable check is done in Vector3 class below.
	// Gram-Schmidt Process:

	Vector3 x = get_column(0);
	Vector3 y = get_column(1);
	Vector3 z = get_column(2);

	x.normalize();
	y = (y - x * (x.dot(y)));
	y.normalize();
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	z.normalize();

	set_column(0, x);
	set_column(1, y);
	set_column(2, z);
}

Basis Basis::orthonormalized() const {
	Basis c = *this;
	c.orthonormalize();
	return c;
}

void Basis::orthogonalize() {
	Vector3 scl = get_scale();
	orthonormalize();
	scale_local(scl);
}

Basis Basis::orthogonalized() const {
	Basis c = *this;
	c.orthogonalize();
	return c;
}

// Returns true if the basis vectors are orthogonal (perpendicular), so it has no skew or shear, and can be decomposed into rotation and scale.
// See https://en.wikipedia.org/wiki/Orthogonal_basis
bool Basis::is_orthogonal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis vectors are orthonormal (orthogonal and normalized), so it has no scale, skew, or shear.
// See https://en.wikipedia.org/wiki/Orthonormal_basis
bool Basis::is_orthonormal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	return Math::is_equal_approx(x.length_squared(), 1) && Math::is_equal_approx(y.length_squared(), 1) && Math::is_equal_approx(z.length_squared(), 1) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis is conformal (orthogonal, uniform scale, preserves angles and distance ratios).
// See https://en.wikipedia.org/wiki/Conformal_linear_transformation
bool Basis::is_conformal() const {
	const Vector3 x = get_column(0);
	const Vector3 y = get_column(1);
	const Vector3 z = get_column(2);
	const real_t x_len_sq = x.length_squared();
	return Math::is_equal_approx(x_len_sq, y.length_squared()) && Math::is_equal_approx(x_len_sq, z.length_squared()) && Math::is_zero_approx(x.dot(y)) && Math::is_zero_approx(x.dot(z)) && Math::is_zero_approx(y.dot(z));
}

// Returns true if the basis only has diagonal elements, so it may only have scale or flip, but no rotation, skew, or shear.
bool Basis::is_diagonal() const {
	return (
			Math::is_zero_approx(rows[0][1]) && Math::is_zero_approx(rows[0][2]) &&
			Math::is_zero_approx(rows[1][0]) && Math::is_zero_approx(rows[1][2]) &&
			Math::is_zero_approx(rows[2][0]) && Math::is_zero_approx(rows[2][1]));
}

// Returns true if the basis is a pure rotation matrix, so it has no scale, skew, shear, or flip.
bool Basis::is_rotation() const {
	return is_conformal() && Math::is_equal_approx(determinant(), 1, (real_t)UNIT_EPSILON);
}

#ifdef MATH_CHECKS
// This method is only used once, in diagonalize. If it's desired elsewhere, feel free to remove the #ifdef.
bool Basis::is_symmetric() const {
	if (!Math::is_equal_approx(rows[0][1], rows[1][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[0][2], rows[2][0])) {
		return false;
	}
	if (!Math::is_equal_approx(rows[1][2], rows[2][1])) {
		return false;
	}

	return true;
}
#endif

Basis Basis::diagonalize() {
// NOTE: only implemented for symmetric matrices
// with the Jacobi iterative method
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!is_symmetric(), Basis());
#endif
	const int ite_max = 1024;

	real_t off_matrix_norm_2 = rows[0][1] * rows[0][1] + rows[0][2] * rows[0][2] + rows[1][2] * rows[1][2];

	int ite = 0;
	Basis acc_rot;
	while (off_matrix_norm_2 > (real_t)CMP_EPSILON2 && ite++ < ite_max) {
		real_t el01_2 = rows[0][1] * rows[0][1];
		real_t el02_2 = rows[0][2] * rows[0][2];
		real_t el12_2 = rows[1][2] * rows[1][2];
		// Find the pivot element
		int i, j;
		if (el01_2 > el02_2) {
			if (el12_2 > el01_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 1;
			}
		} else {
			if (el12_2 > el02_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 2;
			}
		}

		// Compute the rotation angle
		real_t angle;
		if (Math::is_equal_approx(rows[j][j], rows[i][i])) {
			angle = Math::PI / 4;
		} else {
			angle = 0.5f * Math::atan(2 * rows[i][j] / (rows[j][j] - rows[i][i]));
		}

		// Compute the rotation matrix
		Basis rot;
		rot.rows[i][i] = rot.rows[j][j] = Math::cos(angle);
		rot.rows[i][j] = -(rot.rows[j][i] = Math::sin(angle));

		// Update the off matrix norm
		off_matrix_norm_2 -= rows[i][j] * rows[i][j];

		// Apply the rotation
		*this = rot * *this * rot.transposed();
		acc_rot = rot * acc_rot;
	}

	return acc_rot;
}

Basis Basis::inverse() const {
	Basis inv = *this;
	inv.invert();
	return inv;
}

void Basis::transpose() {
	SWAP(rows[0][1], rows[1][0]);
	SWAP(rows[0][2], rows[2][0]);
	SWAP(rows[1][2], rows[2][1]);
}

Basis Basis::transposed() const {
	Basis tr = *this;
	tr.transpose();
	return tr;
}

Basis Basis::from_scale(const Vector3 &p_scale) {
	return Basis(p_scale.x, 0, 0, 0, p_scale.y, 0, 0, 0, p_scale.z);
}

// Multiplies the matrix from left by the scaling matrix: M -> S.M
// See the comment for Basis::rotated for further explanation.
void Basis::scale(const Vector3 &p_scale) {
	rows[0] *= p_scale.x;
	rows[1] *= p_scale.y;
	rows[2] *= p_scale.z;
}

Basis Basis::scaled(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale(p_scale);
	return m;
}

void Basis::scale_local(const Vector3 &p_scale) {
	// performs a scaling in object-local coordinate system:
	// M -> (M.S.Minv).M = M.S.
	rows[0] *= p_scale;
	rows[1] *= p_scale;
	rows[2] *= p_scale;
}

void Basis::scale_orthogonal(const Vector3 &p_scale) {
	*this = scaled_orthogonal(p_scale);
}

Basis Basis::scaled_orthogonal(const Vector3 &p_scale) const {
	Basis m = *this;
	Vector3 s = Vector3(-1, -1, -1) + p_scale;
	bool sign = std::signbit(s.x + s.y + s.z);
	Basis b = m.orthonormalized();
	s = b.xform_inv(s);
	Vector3 dots;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			dots[j] += s[i] * Math::abs(m.get_column(i).normalized().dot(b.get_column(j)));
		}
	}
	if (sign != std::signbit(dots.x + dots.y + dots.z)) {
		dots = -dots;
	}
	m.scale_local(Vector3(1, 1, 1) + dots);
	return m;
}

real_t Basis::get_uniform_scale() const {
	return (rows[0].length() + rows[1].length() + rows[2].length()) / 3.0f;
}

Basis Basis::scaled_local(const Vector3 &p_scale) const {
	Basis m = *this;
	m.scale_local(p_scale);
	return m;
}

Vector3 Basis::get_scale_abs() const {
	return Vector3(
			Vector3(rows[0][0], rows[1][0], rows[2][0]).length(),
			Vector3(rows[0][1], rows[1][1], rows[2][1]).length(),
			Vector3(rows[0][2], rows[1][2], rows[2][2]).length());
}

Vector3 Basis::get_scale_global() const {
	real_t det_sign = SIGN(determinant());
	return det_sign * Vector3(rows[0].length(), rows[1].length(), rows[2].length());
}

// get_scale works with get_rotation, use get_scale_abs if you need to enforce positive signature.
Vector3 Basis::get_scale() const {
	// FIXME: We are assuming M = R.S (R is rotation and S is scaling), and use polar decomposition to extract R and S.
	// A polar decomposition is M = O.P, where O is an orthogonal matrix (meaning rotation and reflection) and
	// P is a positive semi-definite matrix (meaning it contains absolute values of scaling along its diagonal).
	//
	// Despite being different from what we want to achieve, we can nevertheless make use of polar decomposition
	// here as follows. We can split O into a rotation and a reflection as O = R.Q, and obtain M = R.S where
	// we defined S = Q.P. Now, R is a proper rotation matrix and S is a (signed) scaling matrix,
	// which can involve negative scalings. However, there is a catch: unlike the polar decomposition of M = O.P,
	// the decomposition of O into a rotation and reflection matrix as O = R.Q is not unique.
	// Therefore, we are going to do this decomposition by sticking to a particular convention.
	// This may lead to confusion for some users though.
	//
	// The convention we use here is to absorb the sign flip into the scaling matrix.
	// The same convention is also used in other similar functions such as get_rotation_axis_angle, get_rotation, ...
	//
	// A proper way to get rid of this issue would be to store the scaling values (or at least their signs)
	// as a part of Basis. However, if we go that path, we need to disable direct (write) access to the
	// matrix elements.
	//
	// The rotation part of this decomposition is returned by get_rotation* functions.
	real_t det_sign = SIGN(determinant());
	return det_sign * get_scale_abs();
}

// Decomposes a Basis into a rotation-reflection matrix (an element of the group O(3)) and a positive scaling matrix as B = O.S.
// Returns the rotation-reflection matrix via reference argument, and scaling information is returned as a Vector3.
// This (internal) function is too specific and named too ugly to expose to users, and probably there's no need to do so.
Vector3 Basis::rotref_posscale_decomposition(Basis &r_rotref) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(determinant() == 0, Vector3());

	Basis m = transposed() * (*this);
	ERR_FAIL_COND_V(!m.is_diagonal(), Vector3());
#endif
	Vector3 scale = get_scale();
	Basis inv_scale = Basis().scaled(scale.inverse()); // this will also absorb the sign of scale
	r_rotref = (*this) * inv_scale;

#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!r_rotref.is_orthogonal(), Vector3());
#endif
	return scale.abs();
}

// Multiplies the matrix from left by the rotation matrix: M -> R.M
// Note that this does *not* rotate the matrix itself.
//
// The main use of Basis is as Transform.basis, which is used by the transformation matrix
// of 3D object. Rotate here refers to rotation of the object (which is R * (*this)),
// not the matrix itself (which is R * (*this) * R.transposed()).
Basis Basis::rotated(const Vector3 &p_axis, real_t p_angle) const {
	return Basis(p_axis, p_angle) * (*this);
}

void Basis::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

void Basis::rotate_local(const Vector3 &p_axis, real_t p_angle) {
	// performs a rotation in object-local coordinate system:
	// M -> (M.R.Minv).M = M.R.
	*this = rotated_local(p_axis, p_angle);
}

Basis Basis::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	return (*this) * Basis(p_axis, p_angle);
}

Basis Basis::rotated(const Vector3 &p_euler, EulerOrder p_order) const {
	return Basis::from_euler(p_euler, p_order) * (*this);
}

void Basis::rotate(const Vector3 &p_euler, EulerOrder p_order) {
	*this = rotated(p_euler, p_order);
}

Basis Basis::rotated(const Quaternion &p_quaternion) const {
	return Basis(p_quaternion) * (*this);
}

void Basis::rotate(const Quaternion &p_quaternion) {
	*this = rotated(p_quaternion);
}

Vector3 Basis::get_euler_normalized(EulerOrder p_order) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_euler(p_order);
}

Quaternion Basis::get_rotation_quaternion() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_quaternion();
}

void Basis::rotate_to_align(Vector3 p_start_direction, Vector3 p_end_direction) {
	// Takes two vectors and rotates the basis from the first vector to the second vector.
	// Adopted from: https://gist.github.com/kevinmoran/b45980723e53edeb8a5a43c49f134724
	const Vector3 axis = p_start_direction.cross(p_end_direction).normalized();
	if (axis.length_squared() != 0) {
		real_t dot = p_start_direction.dot(p_end_direction);
		dot = CLAMP(dot, -1.0f, 1.0f);
		const real_t angle_rads = Math::acos(dot);
		*this = Basis(axis, angle_rads) * (*this);
	}
}

void Basis::get_rotation_axis_angle(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
}

void Basis::get_rotation_axis_angle_local(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = transposed();
	m.orthonormalize();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
	p_angle = -p_angle;
}

Vector3 Basis::get_euler(EulerOrder p_order) const {
	// This epsilon value results in angles within a +/- 0.04 degree range being simplified/truncated.
	// Based on testing, this is the largest the epsilon can be without the angle truncation becoming
	// visually noticeable.
	const real_t epsilon = 0.00000025;

	switch (p_order) {
		case EulerOrder::XYZ: {
			// Euler angles in XYZ convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz          -cy*sz           sy
			//        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
			//       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

			Vector3 euler;
			real_t sy = rows[0][2];
			if (sy < (1.0f - epsilon)) {
				if (sy > -(1.0f - epsilon)) {
					// is this a pure Y rotation?
					if (rows[1][0] == 0 && rows[0][1] == 0 && rows[1][2] == 0 && rows[2][1] == 0 && rows[1][1] == 1) {
						// return the simplest form (human friendlier in editor and scripts)
						euler.x = 0;
						euler.y = std::atan2(rows[0][2], rows[0][0]);
						euler.z = 0;
					} else {
						euler.x = Math::atan2(-rows[1][2], rows[2][2]);
						euler.y = Math::asin(sy);
						euler.z = Math::atan2(-rows[0][1], rows[0][0]);
					}
				} else {
					euler.x = Math::atan2(rows[2][1], rows[1][1]);
					euler.y = -Math::PI / 2.0f;
					euler.z = 0.0f;
				}
			} else {
				euler.x = Math::atan2(rows[2][1], rows[1][1]);
				euler.y = Math::PI / 2.0f;
				euler.z = 0.0f;
			}
			return euler;
		}
		case EulerOrder::XZY: {
			// Euler angles in XZY convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy             -sz             cz*sy
			//        sx*sy+cx*cy*sz    cx*cz           cx*sz*sy-cy*sx
			//        cy*sx*sz          cz*sx           cx*cy+sx*sz*sy

			Vector3 euler;
			real_t sz = rows[0][1];
			if (sz < (1.0f - epsilon)) {
				if (sz > -(1.0f - epsilon)) {
					euler.x = Math::atan2(rows[2][1], rows[1][1]);
					euler.y = Math::atan2(rows[0][2], rows[0][0]);
					euler.z = Math::asin(-sz);
				} else {
					// It's -1
					euler.x = -Math::atan2(rows[1][2], rows[2][2]);
					euler.y = 0.0f;
					euler.z = Math::PI / 2.0f;
				}
			} else {
				// It's 1
				euler.x = -Math::atan2(rows[1][2], rows[2][2]);
				euler.y = 0.0f;
				euler.z = -Math::PI / 2.0f;
			}
			return euler;
		}
		case EulerOrder::YXZ: {
			// Euler angles in YXZ convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz+sy*sx*sz    cz*sy*sx-cy*sz        cx*sy
			//        cx*sz             cx*cz                 -sx
			//        cy*sx*sz-cz*sy    cy*cz*sx+sy*sz        cy*cx

			Vector3 euler;

			real_t m12 = rows[1][2];

			if (m12 < (1 - epsilon)) {
				if (m12 > -(1 - epsilon)) {
					// is this a pure X rotation?
					if (rows[1][0] == 0 && rows[0][1] == 0 && rows[0][2] == 0 && rows[2][0] == 0 && rows[0][0] == 1) {
						// return the simplest form (human friendlier in editor and scripts)
						euler.x = std::atan2(-m12, rows[1][1]);
						euler.y = 0;
						euler.z = 0;
					} else {
						euler.x = std::asin(-m12);
						euler.y = std::atan2(rows[0][2], rows[2][2]);
						euler.z = std::atan2(rows[1][0], rows[1][1]);
					}
				} else { // m12 == -1
					euler.x = Math::PI * 0.5f;
					euler.y = std::atan2(rows[0][1], rows[0][0]);
					euler.z = 0;
				}
			} else { // m12 == 1
				euler.x = -Math::PI * 0.5f;
				euler.y = -std::atan2(rows[0][1], rows[0][0]);
				euler.z = 0;
			}

			return euler;
		}
		case EulerOrder::YZX: {
			// Euler angles in YZX convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cy*cz             sy*sx-cy*cx*sz     cx*sy+cy*sz*sx
			//        sz                cz*cx              -cz*sx
			//        -cz*sy            cy*sx+cx*sy*sz     cy*cx-sy*sz*sx

			Vector3 euler;
			real_t sz = rows[1][0];
			if (sz < (1.0f - epsilon)) {
				if (sz > -(1.0f - epsilon)) {
					euler.x = Math::atan2(-rows[1][2], rows[1][1]);
					euler.y = Math::atan2(-rows[2][0], rows[0][0]);
					euler.z = Math::asin(sz);
				} else {
					// It's -1
					euler.x = Math::atan2(rows[2][1], rows[2][2]);
					euler.y = 0.0f;
					euler.z = -Math::PI / 2.0f;
				}
			} else {
				// It's 1
				euler.x = Math::atan2(rows[2][1], rows[2][2]);
				euler.y = 0.0f;
				euler.z = Math::PI / 2.0f;
			}
			return euler;
		} break;
		case EulerOrder::ZXY: {
			// Euler angles in ZXY convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy-sz*sx*sy    -cx*sz                cz*sy+cy*sz*sx
			//        cy*sz+cz*sx*sy    cz*cx                 sz*sy-cz*cy*sx
			//        -cx*sy            sx                    cx*cy
			Vector3 euler;
			real_t sx = rows[2][1];
			if (sx < (1.0f - epsilon)) {
				if (sx > -(1.0f - epsilon)) {
					euler.x = Math::asin(sx);
					euler.y = Math::atan2(-rows[2][0], rows[2][2]);
					euler.z = Math::atan2(-rows[0][1], rows[1][1]);
				} else {
					// It's -1
					euler.x = -Math::PI / 2.0f;
					euler.y = Math::atan2(rows[0][2], rows[0][0]);
					euler.z = 0;
				}
			} else {
				// It's 1
				euler.x = Math::PI / 2.0f;
				euler.y = Math::atan2(rows[0][2], rows[0][0]);
				euler.z = 0;
			}
			return euler;
		} break;
		case EulerOrder::ZYX: {
			// Euler angles in ZYX convention.
			// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
			//
			// rot =  cz*cy             cz*sy*sx-cx*sz        sz*sx+cz*cx*cy
			//        cy*sz             cz*cx+sz*sy*sx        cx*sz*sy-cz*sx
			//        -sy               cy*sx                 cy*cx
			Vector3 euler;
			real_t sy = rows[2][0];
			if (sy < (1.0f - epsilon)) {
				if (sy > -(1.0f - epsilon)) {
					euler.x = Math::atan2(rows[2][1], rows[2][2]);
					euler.y = Math::asin(-sy);
					euler.z = Math::atan2(rows[1][0], rows[0][0]);
				} else {
					// It's -1
					euler.x = 0;
					euler.y = Math::PI / 2.0f;
					euler.z = -Math::atan2(rows[0][1], rows[1][1]);
				}
			} else {
				// It's 1
				euler.x = 0;
				euler.y = -Math::PI / 2.0f;
				euler.z = -Math::atan2(rows[0][1], rows[1][1]);
			}
			return euler;
		}
		default: {
			ERR_FAIL_V_MSG(Vector3(), "Invalid parameter for get_euler(order)");
		}
	}
	return Vector3();
}

void Basis::set_euler(const Vector3 &p_euler, EulerOrder p_order) {
	real_t c, s;

	c = Math::cos(p_euler.x);
	s = Math::sin(p_euler.x);
	Basis xmat(1, 0, 0, 0, c, -s, 0, s, c);

	c = Math::cos(p_euler.y);
	s = Math::sin(p_euler.y);
	Basis ymat(c, 0, s, 0, 1, 0, -s, 0, c);

	c = Math::cos(p_euler.z);
	s = Math::sin(p_euler.z);
	Basis zmat(c, -s, 0, s, c, 0, 0, 0, 1);

	switch (p_order) {
		case EulerOrder::XYZ: {
			*this = xmat * (ymat * zmat);
		} break;
		case EulerOrder::XZY: {
			*this = xmat * zmat * ymat;
		} break;
		case EulerOrder::YXZ: {
			*this = ymat * xmat * zmat;
		} break;
		case EulerOrder::YZX: {
			*this = ymat * zmat * xmat;
		} break;
		case EulerOrder::ZXY: {
			*this = zmat * xmat * ymat;
		} break;
		case EulerOrder::ZYX: {
			*this = zmat * ymat * xmat;
		} break;
		default: {
			ERR_FAIL_MSG("Invalid Euler order parameter.");
		}
	}
}

bool Basis::is_equal_approx(const Basis &p_basis) const {
	return rows[0].is_equal_approx(p_basis.rows[0]) && rows[1].is_equal_approx(p_basis.rows[1]) && rows[2].is_equal_approx(p_basis.rows[2]);
}

bool Basis::is_same(const Basis &p_basis) const {
	return rows[0].is_same(p_basis.rows[0]) && rows[1].is_same(p_basis.rows[1]) && rows[2].is_same(p_basis.rows[2]);
}

bool Basis::is_finite() const {
	return rows[0].is_finite() && rows[1].is_finite() && rows[2].is_finite();
}

Basis::operator String() const {
	return "[X: " + get_column(0).operator String() +
			", Y: " + get_column(1).operator String() +
			", Z: " + get_column(2).operator String() + "]";
}

Quaternion Basis::get_quaternion() const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(!is_rotation(), Quaternion(), "Basis " + operator String() + " must be normalized in order to be casted to a Quaternion. Use get_rotation_quaternion() or call orthonormalized() if the Basis contains linearly independent vectors.");
#endif
	/* Allow getting a quaternion from an unnormalized transform */
	Basis m = *this;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0f) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

void Basis::get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
	/* checking this is a bad idea, because obtaining from scaled transform is a valid use case
#ifdef MATH_CHECKS
	ERR_FAIL_COND(!is_rotation());
#endif
	*/

	// https://www.euclideanspace.com/math/geometry/rotations/conversions/matrixToAngle/index.htm
	real_t x, y, z; // Variables for result.
	if (Math::is_zero_approx(rows[0][1] - rows[1][0]) && Math::is_zero_approx(rows[0][2] - rows[2][0]) && Math::is_zero_approx(rows[1][2] - rows[2][1])) {
		// Singularity found.
		// First check for identity matrix which must have +1 for all terms in leading diagonal and zero in other terms.
		if (is_diagonal() && (Math::abs(rows[0][0] + rows[1][1] + rows[2][2] - 3) < 3 * CMP_EPSILON)) {
			// This singularity is identity matrix so angle = 0.
			r_axis = Vector3(0, 1, 0);
			r_angle = 0;
			return;
		}
		// Otherwise this singularity is angle = 180.
		real_t xx = (rows[0][0] + 1) / 2;
		real_t yy = (rows[1][1] + 1) / 2;
		real_t zz = (rows[2][2] + 1) / 2;
		real_t xy = (rows[0][1] + rows[1][0]) / 4;
		real_t xz = (rows[0][2] + rows[2][0]) / 4;
		real_t yz = (rows[1][2] + rows[2][1]) / 4;

		if ((xx > yy) && (xx > zz)) { // rows[0][0] is the largest diagonal term.
			if (xx < CMP_EPSILON) {
				x = 0;
				y = Math::SQRT12;
				z = Math::SQRT12;
			} else {
				x = Math::sqrt(xx);
				y = xy / x;
				z = xz / x;
			}
		} else if (yy > zz) { // rows[1][1] is the largest diagonal term.
			if (yy < CMP_EPSILON) {
				x = Math::SQRT12;
				y = 0;
				z = Math::SQRT12;
			} else {
				y = Math::sqrt(yy);
				x = xy / y;
				z = yz / y;
			}
		} else { // rows[2][2] is the largest diagonal term so base result on this.
			if (zz < CMP_EPSILON) {
				x = Math::SQRT12;
				y = Math::SQRT12;
				z = 0;
			} else {
				z = Math::sqrt(zz);
				x = xz / z;
				y = yz / z;
			}
		}
		r_axis = Vector3(x, y, z);
		r_angle = Math::PI;
		return;
	}
	// As we have reached here there are no singularities so we can handle normally.
	double s = Math::sqrt((rows[2][1] - rows[1][2]) * (rows[2][1] - rows[1][2]) + (rows[0][2] - rows[2][0]) * (rows[0][2] - rows[2][0]) + (rows[1][0] - rows[0][1]) * (rows[1][0] - rows[0][1])); // Used to normalize.

	if (Math::abs(s) < CMP_EPSILON) {
		// Prevent divide by zero, should not happen if matrix is orthogonal and should be caught by singularity test above.
		s = 1;
	}

	x = (rows[2][1] - rows[1][2]) / s;
	y = (rows[0][2] - rows[2][0]) / s;
	z = (rows[1][0] - rows[0][1]) / s;

	r_axis = Vector3(x, y, z);
	// acos does clamping.
	r_angle = Math::acos((rows[0][0] + rows[1][1] + rows[2][2] - 1) / 2);
}

void Basis::set_quaternion(const Quaternion &p_quaternion) {
	real_t d = p_quaternion.length_squared();
	real_t s = 2.0f / d;
	real_t xs = p_quaternion.x * s, ys = p_quaternion.y * s, zs = p_quaternion.z * s;
	real_t wx = p_quaternion.w * xs, wy = p_quaternion.w * ys, wz = p_quaternion.w * zs;
	real_t xx = p_quaternion.x * xs, xy = p_quaternion.x * ys, xz = p_quaternion.x * zs;
	real_t yy = p_quaternion.y * ys, yz = p_quaternion.y * zs, zz = p_quaternion.z * zs;
	set(1.0f - (yy + zz), xy - wz, xz + wy,
			xy + wz, 1.0f - (xx + zz), yz - wx,
			xz - wy, yz + wx, 1.0f - (xx + yy));
}

void Basis::set_axis_angle(const Vector3 &p_axis, real_t p_angle) {
// Rotation matrix from axis and angle, see https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_angle
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(!p_axis.is_normalized(), "The axis Vector3 " + p_axis.operator String() + " must be normalized.");
#endif
	Vector3 axis_sq(p_axis.x * p_axis.x, p_axis.y * p_axis.y, p_axis.z * p_axis.z);
	real_t cosine = Math::cos(p_angle);
	rows[0][0] = axis_sq.x + cosine * (1.0f - axis_sq.x);
	rows[1][1] = axis_sq.y + cosine * (1.0f - axis_sq.y);
	rows[2][2] = axis_sq.z + cosine * (1.0f - axis_sq.z);

	real_t sine = Math::sin(p_angle);
	real_t t = 1 - cosine;

	real_t xyzt = p_axis.x * p_axis.y * t;
	real_t zyxs = p_axis.z * sine;
	rows[0][1] = xyzt - zyxs;
	rows[1][0] = xyzt + zyxs;

	xyzt = p_axis.x * p_axis.z * t;
	zyxs = p_axis.y * sine;
	rows[0][2] = xyzt + zyxs;
	rows[2][0] = xyzt - zyxs;

	xyzt = p_axis.y * p_axis.z * t;
	zyxs = p_axis.x * sine;
	rows[1][2] = xyzt - zyxs;
	rows[2][1] = xyzt + zyxs;
}

void Basis::set_axis_angle_scale(const Vector3 &p_axis, real_t p_angle, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_axis, p_angle);
}

void Basis::set_euler_scale(const Vector3 &p_euler, const Vector3 &p_scale, EulerOrder p_order) {
	_set_diagonal(p_scale);
	rotate(p_euler, p_order);
}

void Basis::set_quaternion_scale(const Quaternion &p_quaternion, const Vector3 &p_scale) {
	_set_diagonal(p_scale);
	rotate(p_quaternion);
}

// This also sets the non-diagonal elements to 0, which is misleading from the
// name, so we want this method to be private. Use `from_scale` externally.
void Basis::_set_diagonal(const Vector3 &p_diag) {
	rows[0][0] = p_diag.x;
	rows[0][1] = 0;
	rows[0][2] = 0;

	rows[1][0] = 0;
	rows[1][1] = p_diag.y;
	rows[1][2] = 0;

	rows[2][0] = 0;
	rows[2][1] = 0;
	rows[2][2] = p_diag.z;
}

Basis Basis::lerp(const Basis &p_to, real_t p_weight) const {
	Basis b;
	b.rows[0] = rows[0].lerp(p_to.rows[0], p_weight);
	b.rows[1] = rows[1].lerp(p_to.rows[1], p_weight);
	b.rows[2] = rows[2].lerp(p_to.rows[2], p_weight);

	return b;
}

Basis Basis::slerp(const Basis &p_to, real_t p_weight) const {
	//consider scale
	Quaternion from(*this);
	Quaternion to(p_to);

	Basis b(from.slerp(to, p_weight));
	b.rows[0] *= Math::lerp(rows[0].length(), p_to.rows[0].length(), p_weight);
	b.rows[1] *= Math::lerp(rows[1].length(), p_to.rows[1].length(), p_weight);
	b.rows[2] *= Math::lerp(rows[2].length(), p_to.rows[2].length(), p_weight);

	return b;
}

void Basis::rotate_sh(real_t *p_values) {
	// code by John Hable
	// http://filmicworlds.com/blog/simple-and-fast-spherical-harmonic-rotation/
	// this code is Public Domain

	const static real_t s_c3 = 0.94617469575; // (3*sqrt(5))/(4*sqrt(pi))
	const static real_t s_c4 = -0.31539156525; // (-sqrt(5))/(4*sqrt(pi))
	const static real_t s_c5 = 0.54627421529; // (sqrt(15))/(4*sqrt(pi))

	const static real_t s_c_scale = 1.0 / 0.91529123286551084;
	const static real_t s_c_scale_inv = 0.91529123286551084;

	const static real_t s_rc2 = 1.5853309190550713 * s_c_scale;
	const static real_t s_c4_div_c3 = s_c4 / s_c3;
	const static real_t s_c4_div_c3_x2 = (s_c4 / s_c3) * 2.0;

	const static real_t s_scale_dst2 = s_c3 * s_c_scale_inv;
	const static real_t s_scale_dst4 = s_c5 * s_c_scale_inv;

	const real_t src[9] = { p_values[0], p_values[1], p_values[2], p_values[3], p_values[4], p_values[5], p_values[6], p_values[7], p_values[8] };

	real_t m00 = rows[0][0];
	real_t m01 = rows[0][1];
	real_t m02 = rows[0][2];
	real_t m10 = rows[1][0];
	real_t m11 = rows[1][1];
	real_t m12 = rows[1][2];
	real_t m20 = rows[2][0];
	real_t m21 = rows[2][1];
	real_t m22 = rows[2][2];

	p_values[0] = src[0];
	p_values[1] = m11 * src[1] - m12 * src[2] + m10 * src[3];
	p_values[2] = -m21 * src[1] + m22 * src[2] - m20 * src[3];
	p_values[3] = m01 * src[1] - m02 * src[2] + m00 * src[3];

	real_t sh0 = src[7] + src[8] + src[8] - src[5];
	real_t sh1 = src[4] + s_rc2 * src[6] + src[7] + src[8];
	real_t sh2 = src[4];
	real_t sh3 = -src[7];
	real_t sh4 = -src[5];

	// Rotations.  R0 and R1 just use the raw matrix columns
	real_t r2x = m00 + m01;
	real_t r2y = m10 + m11;
	real_t r2z = m20 + m21;

	real_t r3x = m00 + m02;
	real_t r3y = m10 + m12;
	real_t r3z = m20 + m22;

	real_t r4x = m01 + m02;
	real_t r4y = m11 + m12;
	real_t r4z = m21 + m22;

	// dense matrix multiplication one column at a time

	// column 0
	real_t sh0_x = sh0 * m00;
	real_t sh0_y = sh0 * m10;
	real_t d0 = sh0_x * m10;
	real_t d1 = sh0_y * m20;
	real_t d2 = sh0 * (m20 * m20 + s_c4_div_c3);
	real_t d3 = sh0_x * m20;
	real_t d4 = sh0_x * m00 - sh0_y * m10;

	// column 1
	real_t sh1_x = sh1 * m02;
	real_t sh1_y = sh1 * m12;
	d0 += sh1_x * m12;
	d1 += sh1_y * m22;
	d2 += sh1 * (m22 * m22 + s_c4_div_c3);
	d3 += sh1_x * m22;
	d4 += sh1_x * m02 - sh1_y * m12;

	// column 2
	real_t sh2_x = sh2 * r2x;
	real_t sh2_y = sh2 * r2y;
	d0 += sh2_x * r2y;
	d1 += sh2_y * r2z;
	d2 += sh2 * (r2z * r2z + s_c4_div_c3_x2);
	d3 += sh2_x * r2z;
	d4 += sh2_x * r2x - sh2_y * r2y;

	// column 3
	real_t sh3_x = sh3 * r3x;
	real_t sh3_y = sh3 * r3y;
	d0 += sh3_x * r3y;
	d1 += sh3_y * r3z;
	d2 += sh3 * (r3z * r3z + s_c4_div_c3_x2);
	d3 += sh3_x * r3z;
	d4 += sh3_x * r3x - sh3_y * r3y;

	// column 4
	real_t sh4_x = sh4 * r4x;
	real_t sh4_y = sh4 * r4y;
	d0 += sh4_x * r4y;
	d1 += sh4_y * r4z;
	d2 += sh4 * (r4z * r4z + s_c4_div_c3_x2);
	d3 += sh4_x * r4z;
	d4 += sh4_x * r4x - sh4_y * r4y;

	// extra multipliers
	p_values[4] = d0;
	p_values[5] = -d1;
	p_values[6] = d2 * s_scale_dst2;
	p_values[7] = -d3;
	p_values[8] = d4 * s_scale_dst4;
}

Basis Basis::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(p_target.is_zero_approx(), Basis(), "The target vector can't be zero.");
	ERR_FAIL_COND_V_MSG(p_up.is_zero_approx(), Basis(), "The up vector can't be zero.");
#endif
	Vector3 v_z = p_target.normalized();
	if (!p_use_model_front) {
		v_z = -v_z;
	}
	Vector3 v_x = p_up.cross(v_z);
	if (v_x.is_zero_approx()) {
		WARN_PRINT("Target and up vectors are colinear. This is not advised as it may cause unwanted rotation around local Z axis.");
		v_x = p_up.get_any_perpendicular(); // Vectors are almost parallel.
	}
	v_x.normalize();
	Vector3 v_y = v_z.cross(v_x);

	Basis basis;
	basis.set_columns(v_x, v_y, v_z);
	return basis;
}

// --- transform_2d.cpp ---

void Transform2D::invert()
{
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform2D::affine_inverse can handle matrices with scaling, so GDScript should eventually
	// use that.
	SWAP(columns[0][1], columns[1][0]);
	columns[2] = basis_xform(-columns[2]);
}

Transform2D Transform2D::inverse() const
{
	Transform2D inv = *this;
	inv.invert();
	return inv;
}

void Transform2D::affine_invert()
{
	real_t det = determinant();
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t idet = 1.0f / det;

	SWAP(columns[0][0], columns[1][1]);
	columns[0] *= Vector2(idet, -idet);
	columns[1] *= Vector2(-idet, idet);

	columns[2] = basis_xform(-columns[2]);
}

Transform2D Transform2D::affine_inverse() const
{
	Transform2D inv = *this;
	inv.affine_invert();
	return inv;
}

void Transform2D::rotate(real_t p_angle) { *this = Transform2D(p_angle, Vector2()) * (*this); }

real_t Transform2D::get_skew() const
{
	real_t det = determinant();
	return Math::acos(columns[0].normalized().dot(SIGN(det) * columns[1].normalized())) -
		   (real_t)Math::PI * 0.5f;
}

void Transform2D::set_skew(real_t p_angle)
{
	real_t det = determinant();
	columns[1] = SIGN(det) * columns[0].rotated(((real_t)Math::PI * 0.5f + p_angle)).normalized() *
				 columns[1].length();
}

real_t Transform2D::get_rotation() const { return Math::atan2(columns[0].y, columns[0].x); }

void Transform2D::set_rotation(real_t p_rot)
{
	Size2 scale = get_scale();
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	columns[0][0] = cr;
	columns[0][1] = sr;
	columns[1][0] = -sr;
	columns[1][1] = cr;
	set_scale(scale);
}

Transform2D::Transform2D(real_t p_rot, const Vector2& p_pos)
{
	real_t cr = Math::cos(p_rot);
	real_t sr = Math::sin(p_rot);
	columns[0][0] = cr;
	columns[0][1] = sr;
	columns[1][0] = -sr;
	columns[1][1] = cr;
	columns[2] = p_pos;
}

Transform2D::Transform2D(real_t p_rot, const Size2& p_scale, real_t p_skew, const Vector2& p_pos)
{
	columns[0][0] = Math::cos(p_rot) * p_scale.x;
	columns[1][1] = Math::cos(p_rot + p_skew) * p_scale.y;
	columns[1][0] = -Math::sin(p_rot + p_skew) * p_scale.y;
	columns[0][1] = Math::sin(p_rot) * p_scale.x;
	columns[2] = p_pos;
}

Size2 Transform2D::get_scale() const
{
	real_t det_sign = SIGN(determinant());
	return Size2(columns[0].length(), det_sign * columns[1].length());
}

void Transform2D::set_scale(const Size2& p_scale)
{
	columns[0].normalize();
	columns[1].normalize();
	columns[0] *= p_scale.x;
	columns[1] *= p_scale.y;
}

void Transform2D::scale(const Size2& p_scale)
{
	scale_basis(p_scale);
	columns[2] *= p_scale;
}

void Transform2D::scale_basis(const Size2& p_scale)
{
	columns[0][0] *= p_scale.x;
	columns[0][1] *= p_scale.y;
	columns[1][0] *= p_scale.x;
	columns[1][1] *= p_scale.y;
}

void Transform2D::translate_local(real_t p_tx, real_t p_ty)
{
	translate_local(Vector2(p_tx, p_ty));
}

void Transform2D::translate_local(const Vector2& p_translation)
{
	columns[2] += basis_xform(p_translation);
}

void Transform2D::orthonormalize()
{
	// Orthonormalizable check is done in Vector2 class below.
	// Gram-Schmidt Process:

	Vector2 x = columns[0];
	Vector2 y = columns[1];

	x.normalize();
	y = y - x * x.dot(y);
	y.normalize();

	columns[0] = x;
	columns[1] = y;
}

Transform2D Transform2D::orthonormalized() const
{
	Transform2D ortho = *this;
	ortho.orthonormalize();
	return ortho;
}

bool Transform2D::is_conformal() const
{
	// Non-flipped case.
	if (Math::is_equal_approx(columns[0][0], columns[1][1]) &&
		Math::is_equal_approx(columns[0][1], -columns[1][0])) {
		return true;
	}
	// Flipped case.
	if (Math::is_equal_approx(columns[0][0], -columns[1][1]) &&
		Math::is_equal_approx(columns[0][1], columns[1][0])) {
		return true;
	}
	return false;
}

bool Transform2D::is_equal_approx(const Transform2D& p_transform) const
{
	return columns[0].is_equal_approx(p_transform.columns[0]) &&
		   columns[1].is_equal_approx(p_transform.columns[1]) &&
		   columns[2].is_equal_approx(p_transform.columns[2]);
}

bool Transform2D::is_same(const Transform2D& p_transform) const
{
	return columns[0].is_same(p_transform.columns[0]) &&
		   columns[1].is_same(p_transform.columns[1]) && columns[2].is_same(p_transform.columns[2]);
}

bool Transform2D::is_finite() const
{
	return columns[0].is_finite() && columns[1].is_finite() && columns[2].is_finite();
}

Transform2D Transform2D::looking_at(const Vector2& p_target) const
{
	Transform2D return_trans = Transform2D(get_rotation(), get_origin());
	Vector2 target_position = affine_inverse().xform(p_target);
	return_trans.set_rotation(
		return_trans.get_rotation() + (target_position * get_scale()).angle());
	return return_trans;
}

void Transform2D::operator*=(const Transform2D& p_transform)
{
	columns[2] = xform(p_transform.columns[2]);

	real_t x0, x1, y0, y1;

	x0 = tdotx(p_transform.columns[0]);
	x1 = tdoty(p_transform.columns[0]);
	y0 = tdotx(p_transform.columns[1]);
	y1 = tdoty(p_transform.columns[1]);

	columns[0][0] = x0;
	columns[0][1] = x1;
	columns[1][0] = y0;
	columns[1][1] = y1;
}

Transform2D Transform2D::operator*(const Transform2D& p_transform) const
{
	Transform2D t = *this;
	t *= p_transform;
	return t;
}

Transform2D Transform2D::scaled(const Size2& p_scale) const
{
	// Equivalent to left multiplication
	Transform2D copy = *this;
	copy.scale(p_scale);
	return copy;
}

Transform2D Transform2D::scaled_local(const Size2& p_scale) const
{
	// Equivalent to right multiplication
	return Transform2D(columns[0] * p_scale.x, columns[1] * p_scale.y, columns[2]);
}

Transform2D Transform2D::untranslated() const
{
	Transform2D copy = *this;
	copy.columns[2] = Vector2();
	return copy;
}

Transform2D Transform2D::translated(const Vector2& p_offset) const
{
	// Equivalent to left multiplication
	return Transform2D(columns[0], columns[1], columns[2] + p_offset);
}

Transform2D Transform2D::translated_local(const Vector2& p_offset) const
{
	// Equivalent to right multiplication
	return Transform2D(columns[0], columns[1], columns[2] + basis_xform(p_offset));
}

Transform2D Transform2D::rotated(real_t p_angle) const
{
	// Equivalent to left multiplication
	return Transform2D(p_angle, Vector2()) * (*this);
}

Transform2D Transform2D::rotated_local(real_t p_angle) const
{
	// Equivalent to right multiplication
	return (*this) * Transform2D(p_angle,
						 Vector2()); // Could be optimized, because origin transform can be skipped.
}

real_t Transform2D::determinant() const
{
	return columns[0].x * columns[1].y - columns[0].y * columns[1].x;
}

Transform2D Transform2D::interpolate_with(const Transform2D& p_transform, real_t p_weight) const
{
	return Transform2D(Math::lerp_angle(get_rotation(), p_transform.get_rotation(), p_weight),
		get_scale().lerp(p_transform.get_scale(), p_weight),
		Math::lerp_angle(get_skew(), p_transform.get_skew(), p_weight),
		get_origin().lerp(p_transform.get_origin(), p_weight));
}

Transform2D::operator String() const
{
	return "[X: " + columns[0].operator String() + ", Y: " + columns[1].operator String() +
		   ", O: " + columns[2].operator String() + "]";
}

// --- transform_3d.cpp ---

void Transform3D::affine_invert() {
	basis.invert();
	origin = basis.xform(-origin);
}

Transform3D Transform3D::affine_inverse() const {
	Transform3D ret = *this;
	ret.affine_invert();
	return ret;
}

void Transform3D::invert() {
	basis.transpose();
	origin = basis.xform(-origin);
}

Transform3D Transform3D::inverse() const {
	// FIXME: this function assumes the basis is a rotation matrix, with no scaling.
	// Transform3D::affine_inverse can handle matrices with scaling, so GDScript should eventually use that.
	Transform3D ret = *this;
	ret.invert();
	return ret;
}

void Transform3D::rotate(const Vector3 &p_axis, real_t p_angle) {
	*this = rotated(p_axis, p_angle);
}

Transform3D Transform3D::rotated(const Vector3 &p_axis, real_t p_angle) const {
	// Equivalent to left multiplication
	Basis p_basis(p_axis, p_angle);
	return Transform3D(p_basis * basis, p_basis.xform(origin));
}

Transform3D Transform3D::rotated_local(const Vector3 &p_axis, real_t p_angle) const {
	// Equivalent to right multiplication
	Basis p_basis(p_axis, p_angle);
	return Transform3D(basis * p_basis, origin);
}

void Transform3D::rotate_basis(const Vector3 &p_axis, real_t p_angle) {
	basis.rotate(p_axis, p_angle);
}

Transform3D Transform3D::looking_at(const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(origin.is_equal_approx(p_target), Transform3D(), "The transform's origin and target can't be equal.");
#endif
	Transform3D t = *this;
	t.basis = Basis::looking_at(p_target - origin, p_up, p_use_model_front);
	return t;
}

void Transform3D::set_look_at(const Vector3 &p_eye, const Vector3 &p_target, const Vector3 &p_up, bool p_use_model_front) {
#ifdef MATH_CHECKS
	ERR_FAIL_COND_MSG(p_eye.is_equal_approx(p_target), "The eye and target vectors can't be equal.");
#endif
	basis = Basis::looking_at(p_target - p_eye, p_up, p_use_model_front);
	origin = p_eye;
}

Transform3D Transform3D::interpolate_with(const Transform3D &p_transform, real_t p_c) const {
	Transform3D interp;

	Vector3 src_scale = basis.get_scale();
	Quaternion src_rot = basis.get_rotation_quaternion();
	Vector3 src_loc = origin;

	Vector3 dst_scale = p_transform.basis.get_scale();
	Quaternion dst_rot = p_transform.basis.get_rotation_quaternion();
	Vector3 dst_loc = p_transform.origin;

	interp.basis.set_quaternion_scale(src_rot.slerp(dst_rot, p_c).normalized(), src_scale.lerp(dst_scale, p_c));
	interp.origin = src_loc.lerp(dst_loc, p_c);

	return interp;
}

void Transform3D::scale(const Vector3 &p_scale) {
	basis.scale(p_scale);
	origin *= p_scale;
}

Transform3D Transform3D::scaled(const Vector3 &p_scale) const {
	// Equivalent to left multiplication
	return Transform3D(basis.scaled(p_scale), origin * p_scale);
}

Transform3D Transform3D::scaled_local(const Vector3 &p_scale) const {
	// Equivalent to right multiplication
	return Transform3D(basis.scaled_local(p_scale), origin);
}

void Transform3D::scale_basis(const Vector3 &p_scale) {
	basis.scale(p_scale);
}

void Transform3D::translate_local(real_t p_tx, real_t p_ty, real_t p_tz) {
	translate_local(Vector3(p_tx, p_ty, p_tz));
}

void Transform3D::translate_local(const Vector3 &p_translation) {
	for (int i = 0; i < 3; i++) {
		origin[i] += basis[i].dot(p_translation);
	}
}

Transform3D Transform3D::translated(const Vector3 &p_translation) const {
	// Equivalent to left multiplication
	return Transform3D(basis, origin + p_translation);
}

Transform3D Transform3D::translated_local(const Vector3 &p_translation) const {
	// Equivalent to right multiplication
	return Transform3D(basis, origin + basis.xform(p_translation));
}

void Transform3D::orthonormalize() {
	// Orthonormalizable check is done in Basis class below.
	basis.orthonormalize();
}

Transform3D Transform3D::orthonormalized() const {
	Transform3D _copy = *this;
	_copy.orthonormalize();
	return _copy;
}

void Transform3D::orthogonalize() {
	basis.orthogonalize();
}

Transform3D Transform3D::orthogonalized() const {
	Transform3D _copy = *this;
	_copy.orthogonalize();
	return _copy;
}

bool Transform3D::is_equal_approx(const Transform3D &p_transform) const {
	return basis.is_equal_approx(p_transform.basis) && origin.is_equal_approx(p_transform.origin);
}

bool Transform3D::is_same(const Transform3D &p_transform) const {
	return basis.is_same(p_transform.basis) && origin.is_same(p_transform.origin);
}

bool Transform3D::is_finite() const {
	return basis.is_finite() && origin.is_finite();
}

void Transform3D::operator*=(const Transform3D &p_transform) {
	origin = xform(p_transform.origin);
	basis *= p_transform.basis;
}

Transform3D Transform3D::operator*(const Transform3D &p_transform) const {
	Transform3D t = *this;
	t *= p_transform;
	return t;
}

Transform3D::operator String() const {
	return "[X: " + basis.get_column(0).operator String() +
			", Y: " + basis.get_column(1).operator String() +
			", Z: " + basis.get_column(2).operator String() +
			", O: " + origin.operator String() + "]";
}

// --- projection.cpp ---

real_t Projection::determinant() const {
	return columns[0][3] * columns[1][2] * columns[2][1] * columns[3][0] - columns[0][2] * columns[1][3] * columns[2][1] * columns[3][0] -
			columns[0][3] * columns[1][1] * columns[2][2] * columns[3][0] + columns[0][1] * columns[1][3] * columns[2][2] * columns[3][0] +
			columns[0][2] * columns[1][1] * columns[2][3] * columns[3][0] - columns[0][1] * columns[1][2] * columns[2][3] * columns[3][0] -
			columns[0][3] * columns[1][2] * columns[2][0] * columns[3][1] + columns[0][2] * columns[1][3] * columns[2][0] * columns[3][1] +
			columns[0][3] * columns[1][0] * columns[2][2] * columns[3][1] - columns[0][0] * columns[1][3] * columns[2][2] * columns[3][1] -
			columns[0][2] * columns[1][0] * columns[2][3] * columns[3][1] + columns[0][0] * columns[1][2] * columns[2][3] * columns[3][1] +
			columns[0][3] * columns[1][1] * columns[2][0] * columns[3][2] - columns[0][1] * columns[1][3] * columns[2][0] * columns[3][2] -
			columns[0][3] * columns[1][0] * columns[2][1] * columns[3][2] + columns[0][0] * columns[1][3] * columns[2][1] * columns[3][2] +
			columns[0][1] * columns[1][0] * columns[2][3] * columns[3][2] - columns[0][0] * columns[1][1] * columns[2][3] * columns[3][2] -
			columns[0][2] * columns[1][1] * columns[2][0] * columns[3][3] + columns[0][1] * columns[1][2] * columns[2][0] * columns[3][3] +
			columns[0][2] * columns[1][0] * columns[2][1] * columns[3][3] - columns[0][0] * columns[1][2] * columns[2][1] * columns[3][3] -
			columns[0][1] * columns[1][0] * columns[2][2] * columns[3][3] + columns[0][0] * columns[1][1] * columns[2][2] * columns[3][3];
}

void Projection::set_identity() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			columns[i][j] = (i == j) ? 1 : 0;
		}
	}
}

void Projection::set_zero() {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			columns[i][j] = 0;
		}
	}
}

Plane Projection::xform4(const Plane &p_vec4) const {
	Plane ret;

	ret.normal.x = columns[0][0] * p_vec4.normal.x + columns[1][0] * p_vec4.normal.y + columns[2][0] * p_vec4.normal.z + columns[3][0] * p_vec4.d;
	ret.normal.y = columns[0][1] * p_vec4.normal.x + columns[1][1] * p_vec4.normal.y + columns[2][1] * p_vec4.normal.z + columns[3][1] * p_vec4.d;
	ret.normal.z = columns[0][2] * p_vec4.normal.x + columns[1][2] * p_vec4.normal.y + columns[2][2] * p_vec4.normal.z + columns[3][2] * p_vec4.d;
	ret.d = columns[0][3] * p_vec4.normal.x + columns[1][3] * p_vec4.normal.y + columns[2][3] * p_vec4.normal.z + columns[3][3] * p_vec4.d;
	return ret;
}

Vector4 Projection::xform(const Vector4 &p_vec4) const {
	return Vector4(
			columns[0][0] * p_vec4.x + columns[1][0] * p_vec4.y + columns[2][0] * p_vec4.z + columns[3][0] * p_vec4.w,
			columns[0][1] * p_vec4.x + columns[1][1] * p_vec4.y + columns[2][1] * p_vec4.z + columns[3][1] * p_vec4.w,
			columns[0][2] * p_vec4.x + columns[1][2] * p_vec4.y + columns[2][2] * p_vec4.z + columns[3][2] * p_vec4.w,
			columns[0][3] * p_vec4.x + columns[1][3] * p_vec4.y + columns[2][3] * p_vec4.z + columns[3][3] * p_vec4.w);
}
Vector4 Projection::xform_inv(const Vector4 &p_vec4) const {
	return Vector4(
			columns[0][0] * p_vec4.x + columns[0][1] * p_vec4.y + columns[0][2] * p_vec4.z + columns[0][3] * p_vec4.w,
			columns[1][0] * p_vec4.x + columns[1][1] * p_vec4.y + columns[1][2] * p_vec4.z + columns[1][3] * p_vec4.w,
			columns[2][0] * p_vec4.x + columns[2][1] * p_vec4.y + columns[2][2] * p_vec4.z + columns[2][3] * p_vec4.w,
			columns[3][0] * p_vec4.x + columns[3][1] * p_vec4.y + columns[3][2] * p_vec4.z + columns[3][3] * p_vec4.w);
}

void Projection::adjust_perspective_znear(real_t p_new_znear) {
	real_t zfar = get_z_far();
	real_t znear = p_new_znear;

	real_t deltaZ = zfar - znear;
	columns[2][2] = -(zfar + znear) / deltaZ;
	columns[3][2] = -2 * znear * zfar / deltaZ;
}

Projection Projection::create_depth_correction(bool p_flip_y) {
	Projection proj;
	proj.set_depth_correction(p_flip_y);
	return proj;
}

Projection Projection::create_light_atlas_rect(const Rect2 &p_rect) {
	Projection proj;
	proj.set_light_atlas_rect(p_rect);
	return proj;
}

Projection Projection::create_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
	Projection proj;
	proj.set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov);
	return proj;
}

Projection Projection::create_perspective_hmd(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist) {
	Projection proj;
	proj.set_perspective(p_fovy_degrees, p_aspect, p_z_near, p_z_far, p_flip_fov, p_eye, p_intraocular_dist, p_convergence_dist);
	return proj;
}

Projection Projection::create_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far) {
	Projection proj;
	proj.set_for_hmd(p_eye, p_aspect, p_intraocular_dist, p_display_width, p_display_to_lens, p_oversample, p_z_near, p_z_far);
	return proj;
}

Projection Projection::create_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
	Projection proj;
	proj.set_orthogonal(p_left, p_right, p_bottom, p_top, p_znear, p_zfar);
	return proj;
}

Projection Projection::create_orthogonal_aspect(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov) {
	Projection proj;
	proj.set_orthogonal(p_size, p_aspect, p_znear, p_zfar, p_flip_fov);
	return proj;
}

Projection Projection::create_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far) {
	Projection proj;
	proj.set_frustum(p_left, p_right, p_bottom, p_top, p_near, p_far);
	return proj;
}

Projection Projection::create_frustum_aspect(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov) {
	Projection proj;
	proj.set_frustum(p_size, p_aspect, p_offset, p_near, p_far, p_flip_fov);
	return proj;
}

Projection Projection::create_fit_aabb(const AABB &p_aabb) {
	Projection proj;
	proj.scale_translate_to_fit(p_aabb);
	return proj;
}

Projection Projection::perspective_znear_adjusted(real_t p_new_znear) const {
	Projection proj = *this;
	proj.adjust_perspective_znear(p_new_znear);
	return proj;
}

Plane Projection::get_projection_plane(Planes p_plane) const {
	const real_t *matrix = (const real_t *)columns;

	switch (p_plane) {
		case PLANE_NEAR: {
			Plane new_plane = Plane(matrix[3] + matrix[2],
					matrix[7] + matrix[6],
					matrix[11] + matrix[10],
					matrix[15] + matrix[14]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_FAR: {
			Plane new_plane = Plane(matrix[3] - matrix[2],
					matrix[7] - matrix[6],
					matrix[11] - matrix[10],
					matrix[15] - matrix[14]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_LEFT: {
			Plane new_plane = Plane(matrix[3] + matrix[0],
					matrix[7] + matrix[4],
					matrix[11] + matrix[8],
					matrix[15] + matrix[12]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_TOP: {
			Plane new_plane = Plane(matrix[3] - matrix[1],
					matrix[7] - matrix[5],
					matrix[11] - matrix[9],
					matrix[15] - matrix[13]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_RIGHT: {
			Plane new_plane = Plane(matrix[3] - matrix[0],
					matrix[7] - matrix[4],
					matrix[11] - matrix[8],
					matrix[15] - matrix[12]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
		case PLANE_BOTTOM: {
			Plane new_plane = Plane(matrix[3] + matrix[1],
					matrix[7] + matrix[5],
					matrix[11] + matrix[9],
					matrix[15] + matrix[13]);

			new_plane.normal = -new_plane.normal;
			new_plane.normalize();
			return new_plane;
		}
	}

	return Plane();
}

Projection Projection::flipped_y() const {
	Projection proj = *this;
	proj.flip_y();
	return proj;
}

Projection Projection ::jitter_offseted(const Vector2 &p_offset) const {
	Projection proj = *this;
	proj.add_jitter_offset(p_offset);
	return proj;
}

void Projection::set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov) {
	if (p_flip_fov) {
		p_fovy_degrees = get_fovy(p_fovy_degrees, 1.0 / p_aspect);
	}

	real_t sine, cotangent, deltaZ;
	real_t radians = Math::deg_to_rad(p_fovy_degrees / 2.0);

	deltaZ = p_z_far - p_z_near;
	sine = Math::sin(radians);

	if ((deltaZ == 0) || (sine == 0) || (p_aspect == 0)) {
		return;
	}
	cotangent = Math::cos(radians) / sine;

	set_identity();

	columns[0][0] = cotangent / p_aspect;
	columns[1][1] = cotangent;
	columns[2][2] = -(p_z_far + p_z_near) / deltaZ;
	columns[2][3] = -1;
	columns[3][2] = -2 * p_z_near * p_z_far / deltaZ;
	columns[3][3] = 0;
}

void Projection::set_perspective(real_t p_fovy_degrees, real_t p_aspect, real_t p_z_near, real_t p_z_far, bool p_flip_fov, int p_eye, real_t p_intraocular_dist, real_t p_convergence_dist) {
	if (p_flip_fov) {
		p_fovy_degrees = get_fovy(p_fovy_degrees, 1.0 / p_aspect);
	}

	real_t left, right, modeltranslation, ymax, xmax, frustumshift;

	ymax = p_z_near * std::tan(Math::deg_to_rad(p_fovy_degrees / 2.0));
	xmax = ymax * p_aspect;
	frustumshift = (p_intraocular_dist / 2.0) * p_z_near / p_convergence_dist;

	switch (p_eye) {
		case 1: { // left eye
			left = -xmax + frustumshift;
			right = xmax + frustumshift;
			modeltranslation = p_intraocular_dist / 2.0;
		} break;
		case 2: { // right eye
			left = -xmax - frustumshift;
			right = xmax - frustumshift;
			modeltranslation = -p_intraocular_dist / 2.0;
		} break;
		default: { // mono, should give the same result as set_perspective(p_fovy_degrees,p_aspect,p_z_near,p_z_far,p_flip_fov)
			left = -xmax;
			right = xmax;
			modeltranslation = 0.0;
		} break;
	}

	set_frustum(left, right, -ymax, ymax, p_z_near, p_z_far);

	// translate matrix by (modeltranslation, 0.0, 0.0)
	Projection cm;
	cm.set_identity();
	cm.columns[3][0] = modeltranslation;
	*this = *this * cm;
}

void Projection::set_for_hmd(int p_eye, real_t p_aspect, real_t p_intraocular_dist, real_t p_display_width, real_t p_display_to_lens, real_t p_oversample, real_t p_z_near, real_t p_z_far) {
	// we first calculate our base frustum on our values without taking our lens magnification into account.
	real_t f1 = (p_intraocular_dist * 0.5) / p_display_to_lens;
	real_t f2 = ((p_display_width - p_intraocular_dist) * 0.5) / p_display_to_lens;
	real_t f3 = (p_display_width / 4.0) / p_display_to_lens;

	// now we apply our oversample factor to increase our FOV. how much we oversample is always a balance we strike between performance and how much
	// we're willing to sacrifice in FOV.
	real_t add = ((f1 + f2) * (p_oversample - 1.0)) / 2.0;
	f1 += add;
	f2 += add;
	f3 *= p_oversample;

	// always apply KEEP_WIDTH aspect ratio
	f3 /= p_aspect;

	switch (p_eye) {
		case 1: { // left eye
			set_frustum(-f2 * p_z_near, f1 * p_z_near, -f3 * p_z_near, f3 * p_z_near, p_z_near, p_z_far);
		} break;
		case 2: { // right eye
			set_frustum(-f1 * p_z_near, f2 * p_z_near, -f3 * p_z_near, f3 * p_z_near, p_z_near, p_z_far);
		} break;
		default: { // mono, does not apply here!
		} break;
	}
}

void Projection::set_orthogonal(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_znear, real_t p_zfar) {
	set_identity();

	columns[0][0] = 2.0 / (p_right - p_left);
	columns[3][0] = -((p_right + p_left) / (p_right - p_left));
	columns[1][1] = 2.0 / (p_top - p_bottom);
	columns[3][1] = -((p_top + p_bottom) / (p_top - p_bottom));
	columns[2][2] = -2.0 / (p_zfar - p_znear);
	columns[3][2] = -((p_zfar + p_znear) / (p_zfar - p_znear));
	columns[3][3] = 1.0;
}

void Projection::set_orthogonal(real_t p_size, real_t p_aspect, real_t p_znear, real_t p_zfar, bool p_flip_fov) {
	if (!p_flip_fov) {
		p_size *= p_aspect;
	}

	set_orthogonal(-p_size / 2, +p_size / 2, -p_size / p_aspect / 2, +p_size / p_aspect / 2, p_znear, p_zfar);
}

void Projection::set_frustum(real_t p_left, real_t p_right, real_t p_bottom, real_t p_top, real_t p_near, real_t p_far) {
	ERR_FAIL_COND(p_right <= p_left);
	ERR_FAIL_COND(p_top <= p_bottom);
	ERR_FAIL_COND(p_far <= p_near);

	real_t *te = &columns[0][0];
	real_t x = 2 * p_near / (p_right - p_left);
	real_t y = 2 * p_near / (p_top - p_bottom);

	real_t a = (p_right + p_left) / (p_right - p_left);
	real_t b = (p_top + p_bottom) / (p_top - p_bottom);
	real_t c = -(p_far + p_near) / (p_far - p_near);
	real_t d = -2 * p_far * p_near / (p_far - p_near);

	te[0] = x;
	te[1] = 0;
	te[2] = 0;
	te[3] = 0;
	te[4] = 0;
	te[5] = y;
	te[6] = 0;
	te[7] = 0;
	te[8] = a;
	te[9] = b;
	te[10] = c;
	te[11] = -1;
	te[12] = 0;
	te[13] = 0;
	te[14] = d;
	te[15] = 0;
}

void Projection::set_frustum(real_t p_size, real_t p_aspect, Vector2 p_offset, real_t p_near, real_t p_far, bool p_flip_fov) {
	if (!p_flip_fov) {
		p_size *= p_aspect;
	}

	set_frustum(-p_size / 2 + p_offset.x, +p_size / 2 + p_offset.x, -p_size / p_aspect / 2 + p_offset.y, +p_size / p_aspect / 2 + p_offset.y, p_near, p_far);
}

real_t Projection::get_z_far() const {
	// NOTE: This assumes z-facing near and far planes, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - near and far planes are z-facing (i.e. columns[0][2] and [1][2] == 0)
	return (columns[3][3] - columns[3][2]) / (columns[2][3] - columns[2][2]);
}

real_t Projection::get_z_near() const {
	// NOTE: This assumes z-facing near and far planes, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - near and far planes are z-facing (i.e. columns[0][2] and [1][2] == 0)
	return (columns[3][3] + columns[3][2]) / (columns[2][3] + columns[2][2]);
}

Vector2 Projection::get_viewport_half_extents() const {
	// NOTE: This assumes a symmetrical frustum, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	// - there is no offset / skew (i.e. columns[2][0] == columns[2][1] == 0)
	real_t w = -get_z_near() * columns[2][3] + columns[3][3];
	return Vector2(w / columns[0][0], w / columns[1][1]);
}

Vector2 Projection::get_far_plane_half_extents() const {
	// NOTE: This assumes a symmetrical frustum, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	// - there is no offset / skew (i.e. columns[2][0] == columns[2][1] == 0)
	real_t w = -get_z_far() * columns[2][3] + columns[3][3];
	return Vector2(w / columns[0][0], w / columns[1][1]);
}

bool Projection::get_endpoints(const Transform3D &p_transform, Vector3 *p_8points) const {
	Vector<Plane> planes = get_projection_planes(Transform3D());
	const Planes intersections[8][3] = {
		{ PLANE_FAR, PLANE_LEFT, PLANE_TOP },
		{ PLANE_FAR, PLANE_LEFT, PLANE_BOTTOM },
		{ PLANE_FAR, PLANE_RIGHT, PLANE_TOP },
		{ PLANE_FAR, PLANE_RIGHT, PLANE_BOTTOM },
		{ PLANE_NEAR, PLANE_LEFT, PLANE_TOP },
		{ PLANE_NEAR, PLANE_LEFT, PLANE_BOTTOM },
		{ PLANE_NEAR, PLANE_RIGHT, PLANE_TOP },
		{ PLANE_NEAR, PLANE_RIGHT, PLANE_BOTTOM },
	};

	for (int i = 0; i < 8; i++) {
		Vector3 point;
		Plane a = planes[intersections[i][0]];
		Plane b = planes[intersections[i][1]];
		Plane c = planes[intersections[i][2]];
		bool res = a.intersect_3(b, c, &point);
		ERR_FAIL_COND_V(!res, false);
		p_8points[i] = p_transform.xform(point);
	}

	return true;
}

Vector<Plane> Projection::get_projection_planes(const Transform3D &p_transform) const {
	/** Fast Plane Extraction from combined modelview/projection matrices.
	 * References:
	 * https://web.archive.org/web/20011221205252/https://www.markmorley.com/opengl/frustumculling.html
	 * https://web.archive.org/web/20061020020112/https://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
	 */

	Vector<Plane> planes;
	planes.resize(6);

	const real_t *matrix = (const real_t *)columns;

	Plane new_plane;

	///////--- Near Plane ---///////
	new_plane = Plane(matrix[3] + matrix[2],
			matrix[7] + matrix[6],
			matrix[11] + matrix[10],
			matrix[15] + matrix[14]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[0] = p_transform.xform(new_plane);

	///////--- Far Plane ---///////
	new_plane = Plane(matrix[3] - matrix[2],
			matrix[7] - matrix[6],
			matrix[11] - matrix[10],
			matrix[15] - matrix[14]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[1] = p_transform.xform(new_plane);

	///////--- Left Plane ---///////
	new_plane = Plane(matrix[3] + matrix[0],
			matrix[7] + matrix[4],
			matrix[11] + matrix[8],
			matrix[15] + matrix[12]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[2] = p_transform.xform(new_plane);

	///////--- Top Plane ---///////
	new_plane = Plane(matrix[3] - matrix[1],
			matrix[7] - matrix[5],
			matrix[11] - matrix[9],
			matrix[15] - matrix[13]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[3] = p_transform.xform(new_plane);

	///////--- Right Plane ---///////
	new_plane = Plane(matrix[3] - matrix[0],
			matrix[7] - matrix[4],
			matrix[11] - matrix[8],
			matrix[15] - matrix[12]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[4] = p_transform.xform(new_plane);

	///////--- Bottom Plane ---///////
	new_plane = Plane(matrix[3] + matrix[1],
			matrix[7] + matrix[5],
			matrix[11] + matrix[9],
			matrix[15] + matrix[13]);

	new_plane.normal = -new_plane.normal;
	new_plane.normalize();

	planes.write[5] = p_transform.xform(new_plane);

	return planes;
}

Projection Projection::inverse() const {
	Projection cm = *this;
	cm.invert();
	return cm;
}

void Projection::invert() {
	// Adapted from Mesa's `src/util/u_math.c` `util_invert_mat4x4`.
	// MIT licensed. Copyright 2008 VMware, Inc. Authored by Jacques Leroy.
	Projection temp;
	real_t *out = (real_t *)temp.columns;
	real_t *m = (real_t *)columns;

	real_t wtmp[4][8];
	real_t m0, m1, m2, m3, s;
	real_t *r0, *r1, *r2, *r3;

#define MAT(m, r, c) (m)[(c) * 4 + (r)]

	r0 = wtmp[0];
	r1 = wtmp[1];
	r2 = wtmp[2];
	r3 = wtmp[3];

	r0[0] = MAT(m, 0, 0);
	r0[1] = MAT(m, 0, 1);
	r0[2] = MAT(m, 0, 2);
	r0[3] = MAT(m, 0, 3);
	r0[4] = 1.0;
	r0[5] = 0.0;
	r0[6] = 0.0;
	r0[7] = 0.0;

	r1[0] = MAT(m, 1, 0);
	r1[1] = MAT(m, 1, 1);
	r1[2] = MAT(m, 1, 2);
	r1[3] = MAT(m, 1, 3);
	r1[5] = 1.0;
	r1[4] = 0.0;
	r1[6] = 0.0;
	r1[7] = 0.0;

	r2[0] = MAT(m, 2, 0);
	r2[1] = MAT(m, 2, 1);
	r2[2] = MAT(m, 2, 2);
	r2[3] = MAT(m, 2, 3);
	r2[6] = 1.0;
	r2[4] = 0.0;
	r2[5] = 0.0;
	r2[7] = 0.0;

	r3[0] = MAT(m, 3, 0);
	r3[1] = MAT(m, 3, 1);
	r3[2] = MAT(m, 3, 2);
	r3[3] = MAT(m, 3, 3);

	r3[7] = 1.0;
	r3[4] = 0.0;
	r3[5] = 0.0;
	r3[6] = 0.0;

	/* choose pivot - or die */
	if (Math::abs(r3[0]) > Math::abs(r2[0])) {
		SWAP(r3, r2);
	}
	if (Math::abs(r2[0]) > Math::abs(r1[0])) {
		SWAP(r2, r1);
	}
	if (Math::abs(r1[0]) > Math::abs(r0[0])) {
		SWAP(r1, r0);
	}
	ERR_FAIL_COND(0.0 == r0[0]);

	/* eliminate first variable     */
	m1 = r1[0] / r0[0];
	m2 = r2[0] / r0[0];
	m3 = r3[0] / r0[0];
	s = r0[1];
	r1[1] -= m1 * s;
	r2[1] -= m2 * s;
	r3[1] -= m3 * s;
	s = r0[2];
	r1[2] -= m1 * s;
	r2[2] -= m2 * s;
	r3[2] -= m3 * s;
	s = r0[3];
	r1[3] -= m1 * s;
	r2[3] -= m2 * s;
	r3[3] -= m3 * s;
	s = r0[4];
	if (s != 0.0) {
		r1[4] -= m1 * s;
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r0[5];
	if (s != 0.0) {
		r1[5] -= m1 * s;
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r0[6];
	if (s != 0.0) {
		r1[6] -= m1 * s;
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r0[7];
	if (s != 0.0) {
		r1[7] -= m1 * s;
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (Math::abs(r3[1]) > Math::abs(r2[1])) {
		SWAP(r3, r2);
	}
	if (Math::abs(r2[1]) > Math::abs(r1[1])) {
		SWAP(r2, r1);
	}
	ERR_FAIL_COND(0.0 == r1[1]);

	/* eliminate second variable */
	m2 = r2[1] / r1[1];
	m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2];
	r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3];
	r3[3] -= m3 * r1[3];
	s = r1[4];
	if (0.0 != s) {
		r2[4] -= m2 * s;
		r3[4] -= m3 * s;
	}
	s = r1[5];
	if (0.0 != s) {
		r2[5] -= m2 * s;
		r3[5] -= m3 * s;
	}
	s = r1[6];
	if (0.0 != s) {
		r2[6] -= m2 * s;
		r3[6] -= m3 * s;
	}
	s = r1[7];
	if (0.0 != s) {
		r2[7] -= m2 * s;
		r3[7] -= m3 * s;
	}

	/* choose pivot - or die */
	if (Math::abs(r3[2]) > Math::abs(r2[2])) {
		SWAP(r3, r2);
	}
	ERR_FAIL_COND(0.0 == r2[2]);

	/* eliminate third variable */
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3];
	r3[4] -= m3 * r2[4];
	r3[5] -= m3 * r2[5];
	r3[6] -= m3 * r2[6];
	r3[7] -= m3 * r2[7];

	/* last check */
	ERR_FAIL_COND(0.0 == r3[3]);

	s = 1.0 / r3[3]; /* now back substitute row 3 */
	r3[4] *= s;
	r3[5] *= s;
	r3[6] *= s;
	r3[7] *= s;

	m2 = r2[3]; /* now back substitute row 2 */
	s = 1.0 / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2);
	r2[5] = s * (r2[5] - r3[5] * m2);
	r2[6] = s * (r2[6] - r3[6] * m2);
	r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1;
	r1[5] -= r3[5] * m1;
	r1[6] -= r3[6] * m1;
	r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0;
	r0[5] -= r3[5] * m0;
	r0[6] -= r3[6] * m0;
	r0[7] -= r3[7] * m0;

	m1 = r1[2]; /* now back substitute row 1 */
	s = 1.0 / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1);
	r1[5] = s * (r1[5] - r2[5] * m1),
	r1[6] = s * (r1[6] - r2[6] * m1);
	r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0;
	r0[5] -= r2[5] * m0;
	r0[6] -= r2[6] * m0;
	r0[7] -= r2[7] * m0;

	m0 = r0[1]; /* now back substitute row 0 */
	s = 1.0 / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0);
	r0[5] = s * (r0[5] - r1[5] * m0),
	r0[6] = s * (r0[6] - r1[6] * m0);
	r0[7] = s * (r0[7] - r1[7] * m0);

	MAT(out, 0, 0) = r0[4];
	MAT(out, 0, 1) = r0[5];
	MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7];
	MAT(out, 1, 0) = r1[4];
	MAT(out, 1, 1) = r1[5];
	MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7];
	MAT(out, 2, 0) = r2[4];
	MAT(out, 2, 1) = r2[5];
	MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7];
	MAT(out, 3, 0) = r3[4];
	MAT(out, 3, 1) = r3[5];
	MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];

#undef MAT

	*this = temp;
}

void Projection::flip_y() {
	for (int i = 0; i < 4; i++) {
		columns[1][i] = -columns[1][i];
	}
}

bool Projection::is_same(const Projection &p_cam) const {
	return columns[0].is_same(p_cam.columns[0]) && columns[1].is_same(p_cam.columns[1]) && columns[2].is_same(p_cam.columns[2]) && columns[3].is_same(p_cam.columns[3]);
}

void Projection::set_depth_correction(bool p_flip_y, bool p_reverse_z, bool p_remap_z) {
	// p_remap_z is used to convert from OpenGL-style clip space (-1 - 1) to Vulkan style (0 - 1).
	real_t *m = &columns[0][0];

	m[0] = 1;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = p_flip_y ? -1 : 1;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = p_remap_z ? (p_reverse_z ? -0.5 : 0.5) : (p_reverse_z ? -1.0 : 1.0);
	m[11] = 0.0;
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = p_remap_z ? 0.5 : 0.0;
	m[15] = 1.0;
}

void Projection::set_light_bias() {
	real_t *m = &columns[0][0];

	m[0] = 0.5;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = 0.5;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 0.5;
	m[11] = 0.0;
	m[12] = 0.5;
	m[13] = 0.5;
	m[14] = 0.5;
	m[15] = 1.0;
}

void Projection::set_light_atlas_rect(const Rect2 &p_rect) {
	real_t *m = &columns[0][0];

	m[0] = p_rect.size.width;
	m[1] = 0.0;
	m[2] = 0.0;
	m[3] = 0.0;
	m[4] = 0.0;
	m[5] = p_rect.size.height;
	m[6] = 0.0;
	m[7] = 0.0;
	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = 1.0;
	m[11] = 0.0;
	m[12] = p_rect.position.x;
	m[13] = p_rect.position.y;
	m[14] = 0.0;
	m[15] = 1.0;
}

Projection::operator String() const {
	return "[X: " + columns[0].operator String() +
			", Y: " + columns[1].operator String() +
			", Z: " + columns[2].operator String() +
			", W: " + columns[3].operator String() + "]";
}

real_t Projection::get_aspect() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	return columns[1][1] / columns[0][0];
}

int Projection::get_pixels_per_meter(int p_for_pixel_width) const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	real_t width = 2 * (-get_z_near() * columns[2][3] + columns[3][3]) / columns[0][0];
	return p_for_pixel_width / width; // Note : return type should be real_t (kept as int for compatibility for now).
}

bool Projection::is_orthogonal() const {
	// NOTE: This assumes that the matrix is a projection across z-axis
	// i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0
	return columns[2][3] == 0.0;
}

real_t Projection::get_fov() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	if (columns[2][0] == 0) {
		return Math::rad_to_deg(2 * Math::atan2(1, columns[0][0]));
	} else {
		// The frustum is asymmetrical so we need to calculate the left and right angles separately.
		real_t right = Math::atan2(columns[2][0] + 1, columns[0][0]);
		real_t left = Math::atan2(columns[2][0] - 1, columns[0][0]);
		return Math::rad_to_deg(right - left);
	}
}

real_t Projection::get_lod_multiplier() const {
	// NOTE: This assumes a rectangular projection plane, i.e. that :
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3], [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] != 0)
	return 2 / columns[0][0];
}

void Projection::make_scale(const Vector3 &p_scale) {
	set_identity();
	columns[0][0] = p_scale.x;
	columns[1][1] = p_scale.y;
	columns[2][2] = p_scale.z;
}

void Projection::scale_translate_to_fit(const AABB &p_aabb) {
	Vector3 min = p_aabb.position;
	Vector3 max = p_aabb.position + p_aabb.size;

	columns[0][0] = 2 / (max.x - min.x);
	columns[1][0] = 0;
	columns[2][0] = 0;
	columns[3][0] = -(max.x + min.x) / (max.x - min.x);

	columns[0][1] = 0;
	columns[1][1] = 2 / (max.y - min.y);
	columns[2][1] = 0;
	columns[3][1] = -(max.y + min.y) / (max.y - min.y);

	columns[0][2] = 0;
	columns[1][2] = 0;
	columns[2][2] = 2 / (max.z - min.z);
	columns[3][2] = -(max.z + min.z) / (max.z - min.z);

	columns[0][3] = 0;
	columns[1][3] = 0;
	columns[2][3] = 0;
	columns[3][3] = 1;
}

void Projection::add_jitter_offset(const Vector2 &p_offset) {
	columns[3][0] += p_offset.x;
	columns[3][1] += p_offset.y;
}

Projection::operator Transform3D() const {
	Transform3D tr;
	const real_t *m = &columns[0][0];

	tr.basis.rows[0][0] = m[0];
	tr.basis.rows[1][0] = m[1];
	tr.basis.rows[2][0] = m[2];

	tr.basis.rows[0][1] = m[4];
	tr.basis.rows[1][1] = m[5];
	tr.basis.rows[2][1] = m[6];

	tr.basis.rows[0][2] = m[8];
	tr.basis.rows[1][2] = m[9];
	tr.basis.rows[2][2] = m[10];

	tr.origin.x = m[12];
	tr.origin.y = m[13];
	tr.origin.z = m[14];

	return tr;
}

Projection::Projection(const Transform3D &p_transform) {
	const Transform3D &tr = p_transform;
	real_t *m = &columns[0][0];

	m[0] = tr.basis.rows[0][0];
	m[1] = tr.basis.rows[1][0];
	m[2] = tr.basis.rows[2][0];
	m[3] = 0.0;
	m[4] = tr.basis.rows[0][1];
	m[5] = tr.basis.rows[1][1];
	m[6] = tr.basis.rows[2][1];
	m[7] = 0.0;
	m[8] = tr.basis.rows[0][2];
	m[9] = tr.basis.rows[1][2];
	m[10] = tr.basis.rows[2][2];
	m[11] = 0.0;
	m[12] = tr.origin.x;
	m[13] = tr.origin.y;
	m[14] = tr.origin.z;
	m[15] = 1.0;
}

// --- transform_interpolator.cpp ---

void TransformInterpolator::interpolate_transform_2d(const Transform2D &p_prev, const Transform2D &p_curr, Transform2D &r_result, real_t p_fraction) {
	// Special case for physics interpolation, if flipping, don't interpolate basis.
	// If the determinant polarity changes, the handedness of the coordinate system changes.
	if (_sign(p_prev.determinant()) != _sign(p_curr.determinant())) {
		r_result.columns[0] = p_curr.columns[0];
		r_result.columns[1] = p_curr.columns[1];
		r_result.set_origin(p_prev.get_origin().lerp(p_curr.get_origin(), p_fraction));
		return;
	}

	r_result = p_prev.interpolate_with(p_curr, p_fraction);
}

void TransformInterpolator::interpolate_transform_3d(const Transform3D &p_prev, const Transform3D &p_curr, Transform3D &r_result, real_t p_fraction) {
	r_result.origin = p_prev.origin + ((p_curr.origin - p_prev.origin) * p_fraction);
	interpolate_basis(p_prev.basis, p_curr.basis, r_result.basis, p_fraction);
}

void TransformInterpolator::interpolate_basis(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) {
	Method method = find_method(p_prev, p_curr);
	interpolate_basis_via_method(p_prev, p_curr, r_result, p_fraction, method);
}

void TransformInterpolator::interpolate_transform_3d_via_method(const Transform3D &p_prev, const Transform3D &p_curr, Transform3D &r_result, real_t p_fraction, Method p_method) {
	r_result.origin = p_prev.origin + ((p_curr.origin - p_prev.origin) * p_fraction);
	interpolate_basis_via_method(p_prev.basis, p_curr.basis, r_result.basis, p_fraction, p_method);
}

void TransformInterpolator::interpolate_basis_via_method(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction, Method p_method) {
	switch (p_method) {
		default: {
			interpolate_basis_linear(p_prev, p_curr, r_result, p_fraction);
		} break;
		case INTERP_SLERP: {
			r_result = _basis_slerp_unchecked(p_prev, p_curr, p_fraction);
		} break;
		case INTERP_SCALED_SLERP: {
			interpolate_basis_scaled_slerp(p_prev, p_curr, r_result, p_fraction);
		} break;
	}
}

Quaternion TransformInterpolator::_basis_to_quat_unchecked(const Basis &p_basis) {
	Basis m = p_basis;
	real_t trace = m.rows[0][0] + m.rows[1][1] + m.rows[2][2];
	real_t temp[4];

	if (trace > 0.0) {
		real_t s = Math::sqrt(trace + 1.0f);
		temp[3] = (s * 0.5f);
		s = 0.5f / s;

		temp[0] = ((m.rows[2][1] - m.rows[1][2]) * s);
		temp[1] = ((m.rows[0][2] - m.rows[2][0]) * s);
		temp[2] = ((m.rows[1][0] - m.rows[0][1]) * s);
	} else {
		int i = m.rows[0][0] < m.rows[1][1]
				? (m.rows[1][1] < m.rows[2][2] ? 2 : 1)
				: (m.rows[0][0] < m.rows[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(m.rows[i][i] - m.rows[j][j] - m.rows[k][k] + 1.0f);
		temp[i] = s * 0.5f;
		s = 0.5f / s;

		temp[3] = (m.rows[k][j] - m.rows[j][k]) * s;
		temp[j] = (m.rows[j][i] + m.rows[i][j]) * s;
		temp[k] = (m.rows[k][i] + m.rows[i][k]) * s;
	}

	return Quaternion(temp[0], temp[1], temp[2], temp[3]);
}

Quaternion TransformInterpolator::_quat_slerp_unchecked(const Quaternion &p_from, const Quaternion &p_to, real_t p_fraction) {
	Quaternion to1;
	real_t omega, cosom, sinom, scale0, scale1;

	// Calculate cosine.
	cosom = p_from.dot(p_to);

	// Adjust signs (if necessary)
	if (cosom < 0.0f) {
		cosom = -cosom;
		to1.x = -p_to.x;
		to1.y = -p_to.y;
		to1.z = -p_to.z;
		to1.w = -p_to.w;
	} else {
		to1.x = p_to.x;
		to1.y = p_to.y;
		to1.z = p_to.z;
		to1.w = p_to.w;
	}

	// Calculate coefficients.

	// This check could possibly be removed as we dealt with this
	// case in the find_method() function, but is left for safety, it probably
	// isn't a bottleneck.
	if ((1.0f - cosom) > (real_t)CMP_EPSILON) {
		// standard case (slerp)
		omega = Math::acos(cosom);
		sinom = Math::sin(omega);
		scale0 = Math::sin((1.0f - p_fraction) * omega) / sinom;
		scale1 = Math::sin(p_fraction * omega) / sinom;
	} else {
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - p_fraction;
		scale1 = p_fraction;
	}
	// Calculate final values.
	return Quaternion(
			scale0 * p_from.x + scale1 * to1.x,
			scale0 * p_from.y + scale1 * to1.y,
			scale0 * p_from.z + scale1 * to1.z,
			scale0 * p_from.w + scale1 * to1.w);
}

Basis TransformInterpolator::_basis_slerp_unchecked(Basis p_from, Basis p_to, real_t p_fraction) {
	Quaternion from = _basis_to_quat_unchecked(p_from);
	Quaternion to = _basis_to_quat_unchecked(p_to);

	Basis b(_quat_slerp_unchecked(from, to, p_fraction));
	return b;
}

void TransformInterpolator::interpolate_basis_scaled_slerp(Basis p_prev, Basis p_curr, Basis &r_result, real_t p_fraction) {
	// Normalize both and find lengths.
	Vector3 lengths_prev = _basis_orthonormalize(p_prev);
	Vector3 lengths_curr = _basis_orthonormalize(p_curr);

	r_result = _basis_slerp_unchecked(p_prev, p_curr, p_fraction);

	// Now the result is unit length basis, we need to scale.
	Vector3 lengths_lerped = lengths_prev + ((lengths_curr - lengths_prev) * p_fraction);

	// Keep a note that the column / row order of the basis is weird,
	// so keep an eye for bugs with this.
	r_result[0] *= lengths_lerped;
	r_result[1] *= lengths_lerped;
	r_result[2] *= lengths_lerped;
}

void TransformInterpolator::interpolate_basis_linear(const Basis &p_prev, const Basis &p_curr, Basis &r_result, real_t p_fraction) {
	// Interpolate basis.
	r_result = p_prev.lerp(p_curr, p_fraction);

	// It turns out we need to guard against zero scale basis.
	// This is kind of silly, as we should probably fix the bugs elsewhere in Godot that can't deal with
	// zero scale, but until that time...
	for (int n = 0; n < 3; n++) {
		Vector3 &axis = r_result[n];

		// Not ok, this could cause errors due to bugs elsewhere,
		// so we will bodge set this to a small value.
		const real_t smallest = 0.0001f;
		const real_t smallest_squared = smallest * smallest;
		if (axis.length_squared() < smallest_squared) {
			// Setting a different component to the smallest
			// helps prevent the situation where all the axes are pointing in the same direction,
			// which could be a problem for e.g. cross products...
			axis[n] = smallest;
		}
	}
}

// Returns length.
real_t TransformInterpolator::_vec3_normalize(Vector3 &p_vec) {
	real_t lengthsq = p_vec.length_squared();
	if (lengthsq == 0.0f) {
		p_vec.x = p_vec.y = p_vec.z = 0.0f;
		return 0.0f;
	}
	real_t length = Math::sqrt(lengthsq);
	p_vec.x /= length;
	p_vec.y /= length;
	p_vec.z /= length;
	return length;
}

// Returns lengths.
Vector3 TransformInterpolator::_basis_orthonormalize(Basis &r_basis) {
	// Gram-Schmidt Process.

	Vector3 x = r_basis.get_column(0);
	Vector3 y = r_basis.get_column(1);
	Vector3 z = r_basis.get_column(2);

	Vector3 lengths;

	lengths.x = _vec3_normalize(x);
	y = (y - x * (x.dot(y)));
	lengths.y = _vec3_normalize(y);
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	lengths.z = _vec3_normalize(z);

	r_basis.set_column(0, x);
	r_basis.set_column(1, y);
	r_basis.set_column(2, z);

	return lengths;
}

TransformInterpolator::Method TransformInterpolator::_test_basis(Basis p_basis, bool r_needed_normalize, Quaternion &r_quat) {
	// Axis lengths.
	Vector3 al = Vector3(p_basis.get_column(0).length_squared(),
			p_basis.get_column(1).length_squared(),
			p_basis.get_column(2).length_squared());

	// Non unit scale?
	if (r_needed_normalize || !_vec3_is_equal_approx(al, Vector3(1.0, 1.0, 1.0), (real_t)0.001f)) {
		// If the basis is not normalized (at least approximately), it will fail the checks needed for slerp.
		// So we try to detect a scaled (but not sheared) basis, which we *can* slerp by normalizing first,
		// and lerping the scales separately.

		// If any of the axes are really small, it is unlikely to be a valid rotation, or is scaled too small to deal with float error.
		const real_t sl_epsilon = 0.00001f;
		if ((al.x < sl_epsilon) ||
				(al.y < sl_epsilon) ||
				(al.z < sl_epsilon)) {
			return INTERP_LERP;
		}

		// Normalize the basis.
		Basis norm_basis = p_basis;

		al.x = Math::sqrt(al.x);
		al.y = Math::sqrt(al.y);
		al.z = Math::sqrt(al.z);

		norm_basis.set_column(0, norm_basis.get_column(0) / al.x);
		norm_basis.set_column(1, norm_basis.get_column(1) / al.y);
		norm_basis.set_column(2, norm_basis.get_column(2) / al.z);

		// This doesn't appear necessary, as the later checks will catch it.
		// if (!_basis_is_orthogonal_any_scale(norm_basis)) {
		// return INTERP_LERP;
		// }

		p_basis = norm_basis;

		// Orthonormalize not necessary as normal normalization(!) works if the
		// axes are orthonormal.
		// p_basis.orthonormalize();

		// If we needed to normalize one of the two bases, we will need to normalize both,
		// regardless of whether the 2nd needs it, just to make sure it takes the path to return
		// INTERP_SCALED_LERP on the 2nd call of _test_basis.
		r_needed_normalize = true;
	}

	// Apply less stringent tests than the built in slerp, the standard Godot slerp
	// is too susceptible to float error to be useful.
	real_t det = p_basis.determinant();
	if (!Math::is_equal_approx(det, 1, (real_t)0.01f)) {
		return INTERP_LERP;
	}

	if (!_basis_is_orthogonal(p_basis)) {
		return INTERP_LERP;
	}

	// TODO: This could possibly be less stringent too, check this.
	r_quat = _basis_to_quat_unchecked(p_basis);
	if (!r_quat.is_normalized()) {
		return INTERP_LERP;
	}

	return r_needed_normalize ? INTERP_SCALED_SLERP : INTERP_SLERP;
}

// This check doesn't seem to be needed but is preserved in case of bugs.
bool TransformInterpolator::_basis_is_orthogonal_any_scale(const Basis &p_basis) {
	Vector3 cross = p_basis.get_column(0).cross(p_basis.get_column(1));
	real_t l = _vec3_normalize(cross);
	// Too small numbers, revert to lerp.
	if (l < 0.001f) {
		return false;
	}

	const real_t epsilon = 0.9995f;

	real_t dot = cross.dot(p_basis.get_column(2));
	if (dot < epsilon) {
		return false;
	}

	cross = p_basis.get_column(1).cross(p_basis.get_column(2));
	l = _vec3_normalize(cross);
	// Too small numbers, revert to lerp.
	if (l < 0.001f) {
		return false;
	}

	dot = cross.dot(p_basis.get_column(0));
	if (dot < epsilon) {
		return false;
	}

	return true;
}

bool TransformInterpolator::_basis_is_orthogonal(const Basis &p_basis, real_t p_epsilon) {
	Basis identity;
	Basis m = p_basis * p_basis.transposed();

	// Less stringent tests than the standard Godot slerp.
	if (!_vec3_is_equal_approx(m[0], identity[0], p_epsilon) || !_vec3_is_equal_approx(m[1], identity[1], p_epsilon) || !_vec3_is_equal_approx(m[2], identity[2], p_epsilon)) {
		return false;
	}
	return true;
}

real_t TransformInterpolator::checksum_transform_3d(const Transform3D &p_transform) {
	// just a really basic checksum, this can probably be improved
	real_t sum = _vec3_sum(p_transform.origin);
	sum -= _vec3_sum(p_transform.basis.rows[0]);
	sum += _vec3_sum(p_transform.basis.rows[1]);
	sum -= _vec3_sum(p_transform.basis.rows[2]);
	return sum;
}

TransformInterpolator::Method TransformInterpolator::find_method(const Basis &p_a, const Basis &p_b) {
	bool needed_normalize = false;

	Quaternion q0;
	Method method = _test_basis(p_a, needed_normalize, q0);
	if (method == INTERP_LERP) {
		return method;
	}

	Quaternion q1;
	method = _test_basis(p_b, needed_normalize, q1);
	if (method == INTERP_LERP) {
		return method;
	}

	// Are they close together?
	// Apply the same test that will revert to lerp as is present in the slerp routine.
	// Calculate cosine.
	real_t cosom = Math::abs(q0.dot(q1));
	if ((1.0f - cosom) <= (real_t)CMP_EPSILON) {
		return INTERP_LERP;
	}

	return method;
}
