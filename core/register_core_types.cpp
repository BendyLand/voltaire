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
#include "core/crypto/crypto.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/http_client.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/io/json.h"
#include "core/io/config_file.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "core/io/packet_peer.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/packet_peer_udp.h"
#include "core/io/compression.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/io/socket_server.h"
#include "core/io/stream_peer.h"
#include "core/io/stream_peer_tls.h"
#include "core/io/config_file.h"
#include "core/io/packet_peer_udp.h"
#include "core/io/xml_parser.h"
#include "core/math/geometry_3d.h"
#include "core/math/expression.h"
#include "core/math/random_number_generator.h"
#include "core/math/geometry_3d.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/translation_server.h"
#include "core/string/translation.h"
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

#include "core/input/input_enums.h"
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
#include "core/input/input_map.h"
#include "core/io/compression.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/marshalls.h"
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

#include "core/authors.gen.h"
#include "core/config/project_settings.h"
#include "core/donors.gen.h"
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

#include "core/core_globals.h"
#include "core/io/logger.h"
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

#include "core/error/error_list.h"

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

#include "scene/3d/camera_3d.h"

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
#include "core/input/input_map.h"
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
#include "core/input/input_map.h"

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

#include "core/input/shortcut.h"

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

#include "core/input/input.h"
#include "core/io/marshalls.h"

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
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/math/transform_2d.h"
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
#include "core/io/json.h"
#include "core/os/main_loop.h"
#include "core/os/midi_driver.h"
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

#include "core/os/main_loop.h"

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
#include "core/os/midi_driver.h"

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

#include "core/os/thread_safe.h"

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
#include "core/math/math_funcs_binary.h"
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
