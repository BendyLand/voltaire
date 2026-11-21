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
#include "core/crypto/aes_context.h"
#include "core/crypto/crypto.h"
#include "core/crypto/crypto_resource_format.h"
#include "core/crypto/hashing_context.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/dtls_server.h"
#include "core/io/http_client.h"
#include "core/io/image_loader.h"
#include "core/io/image_resource_format.h"
#include "core/io/json.h"
#include "core/io/json_resource_format.h"
#include "core/io/marshalls.h"
#include "core/io/missing_resource.h"
#include "core/io/packet_peer.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/packet_peer_udp.h"
#include "core/io/pck_packer.h"
#include "core/io/resource_format_binary.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/io/socket_server.h"
#include "core/io/stream_peer_gzip.h"
#include "core/io/stream_peer_tls.h"
#include "core/io/translation_loader_po.h"
#include "core/io/udp_server.h"
#include "core/io/xml_parser.h"
#include "core/math/a_star.h"
#include "core/math/expression.h"
#include "core/math/random_number_generator.h"
#include "core/math/triangle_mesh.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/fuzzy_search.h"
#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "register_core_types.h"
#ifndef DISABLE_DEPRECATED
#include "core/io/packed_data_container.h"
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
#include "core_constants.h"

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
