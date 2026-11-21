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
#include "core/crypto/hashing_context.h"
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
#include "core/math/a_star.h"
#include "core/math/expression.h"
#include "core/math/random_number_generator.h"
#include "core/math/triangle_mesh.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/translation_server.h"
#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "register_core_types.h"
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

extern Mutex _global_mutex;

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

#include "core/config/project_format.h"
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
