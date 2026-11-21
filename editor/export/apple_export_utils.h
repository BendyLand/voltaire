/**************************************************************************/
/*  apple_export_utils.h                                                  */
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

#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/string/ustring.h"

/*************************************************************************/
/* MachO                                                                 */
/*************************************************************************/

class MachO final : public RefCounted
{
public:
	struct MachHeader
	{
		uint32_t cputype;
		uint32_t cpusubtype;
		uint32_t filetype;
		uint32_t ncmds;
		uint32_t sizeofcmds;
		uint32_t flags;
	};

	enum LoadCommandID
	{
		LC_SEGMENT = 0x00000001,
		LC_SYMTAB = 0x00000002,
		LC_SYMSEG = 0x00000003,
		LC_THREAD = 0x00000004,
		LC_UNIXTHREAD = 0x00000005,
		LC_LOADFVMLIB = 0x00000006,
		LC_IDFVMLIB = 0x00000007,
		LC_IDENT = 0x00000008,
		LC_FVMFILE = 0x00000009,
		LC_PREPAGE = 0x0000000a,
		LC_DYSYMTAB = 0x0000000b,
		LC_LOAD_DYLIB = 0x0000000c,
		LC_ID_DYLIB = 0x0000000d,
		LC_LOAD_DYLINKER = 0x0000000e,
		LC_ID_DYLINKER = 0x0000000f,
		LC_PREBOUND_DYLIB = 0x00000010,
		LC_ROUTINES = 0x00000011,
		LC_SUB_FRAMEWORK = 0x00000012,
		LC_SUB_UMBRELLA = 0x00000013,
		LC_SUB_CLIENT = 0x00000014,
		LC_SUB_LIBRARY = 0x00000015,
		LC_TWOLEVEL_HINTS = 0x00000016,
		LC_PREBIND_CKSUM = 0x00000017,
		LC_LOAD_WEAK_DYLIB = 0x80000018,
		LC_SEGMENT_64 = 0x00000019,
		LC_ROUTINES_64 = 0x0000001a,
		LC_UUID = 0x0000001b,
		LC_RPATH = 0x8000001c,
		LC_CODE_SIGNATURE = 0x0000001d,
		LC_SEGMENT_SPLIT_INFO = 0x0000001e,
		LC_REEXPORT_DYLIB = 0x8000001f,
		LC_LAZY_LOAD_DYLIB = 0x00000020,
		LC_ENCRYPTION_INFO = 0x00000021,
		LC_DYLD_INFO = 0x00000022,
		LC_DYLD_INFO_ONLY = 0x80000022,
		LC_LOAD_UPWARD_DYLIB = 0x80000023,
		LC_VERSION_MIN_MACOSX = 0x00000024,
		LC_VERSION_MIN_IPHONEOS = 0x00000025,
		LC_FUNCTION_STARTS = 0x00000026,
		LC_DYLD_ENVIRONMENT = 0x00000027,
		LC_MAIN = 0x80000028,
		LC_DATA_IN_CODE = 0x00000029,
		LC_SOURCE_VERSION = 0x0000002a,
		LC_DYLIB_CODE_SIGN_DRS = 0x0000002b,
		LC_ENCRYPTION_INFO_64 = 0x0000002c,
		LC_LINKER_OPTION = 0x0000002d,
		LC_LINKER_OPTIMIZATION_HINT = 0x0000002e,
		LC_VERSION_MIN_TVOS = 0x0000002f,
		LC_VERSION_MIN_WATCHOS = 0x00000030,
		LC_BUILD_VERSION = 0x00000032,
	};

	enum PlatformID
	{
		PLATFORM_UNKNOWN = 0,
		PLATFORM_MACOS = 1,
		PLATFORM_IOS = 2,
		PLATFORM_TVOS = 3,
		PLATFORM_WATCHOS = 4,
		PLATFORM_BRIDGEOS = 5,
		PLATFORM_MACCATALYST = 6,
		PLATFORM_IOSSIMULATOR = 7,
		PLATFORM_TVOSSIMULATOR = 8,
		PLATFORM_WATCHOSSIMULATOR = 9,
		PLATFORM_DRIVERKIT = 10,
		PLATFORM_VISIONOS = 11,
		PLATFORM_VISIONOSSIMULATOR = 12,
	};

	struct LoadCommandHeader
	{
		uint32_t cmd;
		uint32_t cmdsize;
	};

	struct LoadCommandSegment
	{
		char segname[16];
		uint32_t vmaddr;
		uint32_t vmsize;
		uint32_t fileoff;
		uint32_t filesize;
		uint32_t maxprot;
		uint32_t initprot;
		uint32_t nsects;
		uint32_t flags;
	};

	struct LoadCommandSegment64
	{
		char segname[16];
		uint64_t vmaddr;
		uint64_t vmsize;
		uint64_t fileoff;
		uint64_t filesize;
		uint32_t maxprot;
		uint32_t initprot;
		uint32_t nsects;
		uint32_t flags;
	};

	struct Section
	{
		char sectname[16];
		char segname[16];
		uint32_t addr;
		uint32_t size;
		uint32_t offset;
		uint32_t align;
		uint32_t reloff;
		uint32_t nreloc;
		uint32_t flags;
		uint32_t reserved1;
		uint32_t reserved2;
	};

	struct Section64
	{
		char sectname[16];
		char segname[16];
		uint64_t addr;
		uint64_t size;
		uint32_t offset;
		uint32_t align;
		uint32_t reloff;
		uint32_t nreloc;
		uint32_t flags;
		uint32_t reserved1;
		uint32_t reserved2;
		uint32_t reserved3;
	};

private:
	Ref<FileAccess> fa;
	bool swap = false;

	uint64_t lc_limit = 0;

	uint64_t exe_limit = 0;
	uint64_t exe_base = std::numeric_limits<uint64_t>::max();
	uint32_t align = 0;
	uint32_t cputype = 0;
	uint32_t cpusubtype = 0;

	uint64_t link_edit_offset = 0;
	uint64_t signature_offset = 0;

	uint32_t seg_align(uint64_t p_vmaddr, uint32_t p_min, uint32_t p_max);
	bool alloc_signature(uint64_t p_size);

	static inline size_t PAD(size_t s, size_t a) { return (a - s % a); }

public:
	static bool is_macho(const String& p_path);
	static uint32_t get_filetype(const String& p_path);

	bool open_file(const String& p_path);

	uint64_t get_exe_base();
	uint64_t get_exe_limit();
	int32_t get_align();
	uint32_t get_cputype();
	uint32_t get_cpusubtype();
	uint64_t get_size();
	uint64_t get_code_limit();

	uint64_t get_signature_offset();
	bool is_signed();

	PackedByteArray get_cdhash_sha1();
	PackedByteArray get_cdhash_sha256();

	PackedByteArray get_requirements();

	const Ref<FileAccess> get_file() const;
	Ref<FileAccess> get_file();

	uint64_t get_signature_size();
	bool set_signature_size(uint64_t p_size);
};

/*************************************************************************/
/* LipO                                                                  */
/*************************************************************************/

class LipO final : public RefCounted
{
	struct FatArch
	{
		uint32_t cputype;
		uint32_t cpusubtype;
		uint64_t offset;
		uint64_t size;
		uint32_t align;
	};

	Ref<FileAccess> fa;
	Vector<FatArch> archs;

	static inline size_t PAD(size_t s, size_t a) { return (a - s % a); }

public:
	static bool is_lipo(const String& p_path);

	bool create_file(const String& p_output_path, const Vector<String>& p_files);
	bool create_file(const String& p_output_path, const Vector<String>& p_files,
		const Vector<Vector2i>& p_cputypes);

	bool open_file(const String& p_path);
	int get_arch_count() const;
	uint32_t get_arch_cputype(int p_index) const;
	uint32_t get_arch_cpusubtype(int p_index) const;
	bool extract_arch(int p_index, const String& p_path);

	void close();

	~LipO() override;
};

/*************************************************************************/
/* CodeSignCodeResources                                                 */
/*************************************************************************/

class CodeSignCodeResources
{
public:
	enum class CRMatch
	{
		CR_MATCH_NO,
		CR_MATCH_YES,
		CR_MATCH_NESTED,
		CR_MATCH_OPTIONAL,
	};

private:
	struct CRFile
	{
		String name;
		String hash;
		String hash2;
		bool optional;
		bool nested;
		String requirements;
	};

	struct CRRule
	{
		String file_pattern;
		String key;
		int weight;
		bool store;

		CRRule()
		{
			weight = 1;
			store = true;
		}

		CRRule(const String& p_file_pattern, const String& p_key, int p_weight, bool p_store)
		{
			file_pattern = p_file_pattern;
			key = p_key;
			weight = p_weight;
			store = p_store;
		}
	};

	Vector<CRRule> rules1;
	Vector<CRRule> rules2;

	Vector<CRFile> files1;
	Vector<CRFile> files2;

	String hash_sha1_base64(const String& p_path);
	String hash_sha256_base64(const String& p_path);

public:
	void add_rule1(
		const String& p_rule, const String& p_key = "", int p_weight = 0, bool p_store = true);
	void add_rule2(
		const String& p_rule, const String& p_key = "", int p_weight = 0, bool p_store = true);

	CRMatch match_rules1(const String& p_path) const;
	CRMatch match_rules2(const String& p_path) const;

	bool add_file1(const String& p_root, const String& p_path);
	bool add_file2(const String& p_root, const String& p_path);
	bool add_nested_file(const String& p_root, const String& p_path, const String& p_exepath);

	bool add_folder_recursive(
		const String& p_root, const String& p_path = "", const String& p_main_exe_path = "");

	bool save_to_file(const String& p_path);
};

/*************************************************************************/
/* CodeSignBlob                                                          */
/*************************************************************************/

class CodeSignBlob
{
public:
	enum IndexType : uint32_t
	{
		INDEX_CODE_DIRECTORY = 0x00000000,
		INDEX_REQUIREMENTS = 0x00000002,
		INDEX_ENTITLEMENTS_TEXT = 0x00000005,
		INDEX_ENTITLEMENTS_BINARY = 0x00000007,
		INDEX_SIGNATURE = 0x00010000,
	};

	enum Slot
	{
		SLOT_INFO_PLIST = -1,
		SLOT_REQUIREMENTS = -2,
		SLOT_RESOURCES = -3,
		SLOT_APP_SPECIFIC = -4,
		SLOT_ENTITLEMENTS = -5,
		SLOT_RESERVER1 = -6,
		SLOT_DER_ENTITLEMENTS = -7,
	};

	enum CodeSignExecSegFlags
	{
		EXECSEG_MAIN_BINARY = 0x1,
		EXECSEG_ALLOW_UNSIGNED = 0x10,
		EXECSEG_DEBUGGER = 0x20,
		EXECSEG_JIT = 0x40,
		EXECSEG_SKIP_LV = 0x80,
		EXECSEG_CAN_LOAD_CDHASH = 0x100,
		EXECSEG_CAN_EXEC_CDHASH = 0x200,
	};

	enum CodeSignatureFlags
	{
		SIGNATURE_HOST = 0x0001,
		SIGNATURE_ADHOC = 0x0002,
		SIGNATURE_TASK_ALLOW = 0x0004,
		SIGNATURE_INSTALLER = 0x0008,
		SIGNATURE_FORCED_LV = 0x0010,
		SIGNATURE_INVALID_ALLOWED = 0x0020,
		SIGNATURE_FORCE_HARD = 0x0100,
		SIGNATURE_FORCE_KILL = 0x0200,
		SIGNATURE_FORCE_EXPIRATION = 0x0400,
		SIGNATURE_RESTRICT = 0x0800,
		SIGNATURE_ENFORCEMENT = 0x1000,
		SIGNATURE_LIBRARY_VALIDATION = 0x2000,
		SIGNATURE_ENTITLEMENTS_VALIDATED = 0x4000,
		SIGNATURE_NVRAM_UNRESTRICTED = 0x8000,
		SIGNATURE_RUNTIME = 0x10000,
		SIGNATURE_LINKER_SIGNED = 0x20000,
	};

private:
	struct CodeDirectoryHeader
	{
		uint32_t version;
		uint32_t flags;
		uint32_t hash_offset;
		uint32_t ident_offset;
		uint32_t special_slots;
		uint32_t code_slots;
		uint32_t code_limit;
		uint8_t hash_size;
		uint8_t hash_type;
		uint8_t platform;
		uint8_t page_size;
		uint32_t spare2;
		uint32_t scatter_vector_offset;
		uint32_t team_offset;
		uint32_t spare3;
		uint64_t code_limit_64;
		uint64_t exec_seg_base;
		uint64_t exec_seg_limit;
		uint64_t exec_seg_flags;
		uint32_t runtime;
		uint32_t pre_encrypt_offset;
	};

	PackedByteArray blob;
	uint32_t index_type = INDEX_CODE_DIRECTORY;
	int32_t pages = 0;
	int32_t remain = 0;
	int32_t code_slots = 0;
	int32_t special_slots = 0;

	static inline size_t PAD(size_t s, size_t a) { return (s % a == 0) ? 0 : (a - s % a); }

	_FORCE_INLINE_ void _parse_certificate_slot(
		uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_key(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_oid_key(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_hash_string(
		uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_value(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_date(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ bool _parse_match(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;

public:
	CodeSignBlob() = default;

	static CodeSignBlob make_requirements();
	static CodeSignBlob make_requirements(const PackedByteArray& p_data);
	static CodeSignBlob make_entitlements_text();
	static CodeSignBlob make_entitlements_text(const String& p_string);
	static CodeSignBlob make_entitlements_binary();
	static CodeSignBlob make_entitlements_binary(const String& p_string);
	static CodeSignBlob make_code_directory();
	static CodeSignBlob make_code_directory(uint8_t p_hash_size, uint8_t p_hash_type, bool p_main,
		const CharString& p_id, const CharString& p_team_id, uint32_t p_page_size,
		uint64_t p_exe_limit, uint64_t p_code_limit);
	static CodeSignBlob make_signature();

	Vector<String> parse_requirements() const;

	int32_t get_page_count() const { return pages; }
	int32_t get_page_remainder() const { return remain; }
	bool set_hash_in_slot(const PackedByteArray& p_hash, int p_slot);

	bool is_empty() const { return blob.is_empty(); }
	PackedByteArray get_hash_sha1() const;
	PackedByteArray get_hash_sha256() const;
	int get_size() const { return blob.size(); }
	uint32_t get_index_type() const { return index_type; }
	void write_to_file(Ref<FileAccess> p_file) const;
};

/*************************************************************************/
/* CodeSignSuperBlob                                                     */
/*************************************************************************/

class CodeSignSuperBlob
{
	Vector<CodeSignBlob> blobs;

public:
	bool add_blob(const CodeSignBlob& p_blob);

	int get_size() const;
	void write_to_file(Ref<FileAccess> p_file) const;
};

/*************************************************************************/
/* CodeSign                                                              */
/*************************************************************************/

class CodeSign
{
	static PackedByteArray file_hash_sha1(const String& p_path);
	static PackedByteArray file_hash_sha256(const String& p_path);
	static Error _codesign_file(bool p_use_hardened_runtime, bool p_force, const String& p_info,
		const String& p_exe_path, const String& p_bundle_path, const String& p_ent_path,
		bool p_ios_bundle, String& r_error_msg);

public:
	static Error codesign(bool p_use_hardened_runtime, bool p_force, const String& p_path,
		const String& p_ent_path, String& r_error_msg);
};

/*************************************************************************/
/* PluginConfigAppleEmbedded                                             */
/*************************************************************************/

struct PluginConfigAppleEmbedded
{
	inline static const char* PLUGIN_CONFIG_EXT = ".gdip";

	inline static const char* CONFIG_SECTION = "config";
	inline static const char* CONFIG_NAME_KEY = "name";
	inline static const char* CONFIG_BINARY_KEY = "binary";
	inline static const char* CONFIG_INITIALIZE_KEY = "initialization";
	inline static const char* CONFIG_DEINITIALIZE_KEY = "deinitialization";

	inline static const char* DEPENDENCIES_SECTION = "dependencies";
	inline static const char* DEPENDENCIES_LINKED_KEY = "linked";
	inline static const char* DEPENDENCIES_EMBEDDED_KEY = "embedded";
	inline static const char* DEPENDENCIES_SYSTEM_KEY = "system";
	inline static const char* DEPENDENCIES_CAPABILITIES_KEY = "capabilities";
	inline static const char* DEPENDENCIES_FILES_KEY = "files";
	inline static const char* DEPENDENCIES_LINKER_FLAGS = "linker_flags";
	inline static const char* DEPENDENCIES_SPM_PACKAGES_KEY = "spm_packages";

	inline static const char* PLIST_SECTION = "plist";

	enum PlistItemType
	{
		UNKNOWN,
		STRING,
		INTEGER,
		BOOLEAN,
		RAW,
		STRING_INPUT,
	};

	struct PlistItem
	{
		PlistItemType type;
		String value;
	};

	struct SPMPackage
	{
		String url;
		String version;
		Vector<String> products;
	};

	bool valid_config = false;
	bool supports_targets = false;
	uint64_t last_updated = 0;

	String name;
	String binary;
	String initialization_method;
	String deinitialization_method;

	Vector<String> linked_dependencies;
	Vector<String> embedded_dependencies;
	Vector<String> system_dependencies;

	Vector<String> files_to_copy;
	Vector<String> capabilities;

	Vector<String> linker_flags;

	Vector<SPMPackage> spm_packages;

	HashMap<String, PlistItem> plist;

	static String resolve_local_dependency_path(String plugin_config_dir, String dependency_path);
	static String resolve_system_dependency_path(String dependency_path);
	static Vector<String> resolve_local_dependencies(
		String plugin_config_dir, Vector<String> p_paths);
	static Vector<String> resolve_system_dependencies(Vector<String> p_paths);
	static bool validate_plugin(PluginConfigAppleEmbedded& plugin_config);
	static String get_plugin_main_binary(PluginConfigAppleEmbedded& plugin_config, bool p_debug);
	static uint64_t get_plugin_modification_time(
		const PluginConfigAppleEmbedded& plugin_config, const String& config_path);
	static PluginConfigAppleEmbedded load_plugin_config(
		Ref<ConfigFile> config_file, const String& path);
};
