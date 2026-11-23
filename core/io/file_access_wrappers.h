/**************************************************************************/
/*  file_access_wrappers.h                                                */
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

#include "core/crypto/crypto.h"
#include "core/io/compression.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_uid.h"
#include "core/string/print_string.h"
#include "core/templates/hash_set.h"
#include "core/templates/list.h"

// ============================================================================
// FileAccessMemory
// ============================================================================

class FileAccessMemory final : public FileAccess
{
	uint8_t* data = nullptr;
	uint64_t length = 0;
	mutable uint64_t pos = 0;

	static Ref<FileAccess> create();

public:
	static void register_file(const String& p_name, const Vector<uint8_t>& p_data);
	static void cleanup();

	Error open_custom(const uint8_t* p_data, uint64_t p_len);					  ///< open a file
	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual void seek(uint64_t p_position) override;	///< seek to a given position
	virtual void seek_end(int64_t p_position) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;		///< get position in the file
	virtual uint64_t get_length() const override;		///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(
		uint8_t* p_dst, uint64_t p_length) const override; ///< get an array of bytes

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual void close() override {}
};

// ============================================================================
// FileAccessPatched
// ============================================================================

class FileAccessPatched final : public FileAccess
{
	Ref<FileAccess> old_file;
	mutable Vector<uint8_t> patched_file_data;
	mutable Ref<FileAccessMemory> patched_file;
	mutable Error last_error = OK;

	Error _apply_patch() const;
	bool _try_apply_patch() const;

protected:
	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual Error open_internal(const String& p_path, int p_mode_flags) override
	{
		return ERR_UNAVAILABLE;
	}

public:
	Error open_custom(const Ref<FileAccess>& p_old_file);

	virtual bool is_open() const override;

	virtual void seek(uint64_t p_position) override;
	virtual void seek_end(int64_t p_position = 0) override;

	virtual uint64_t get_position() const override;
	virtual uint64_t get_length() const override;
	virtual bool eof_reached() const override;
	virtual Error get_error() const override;

	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;
	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual void close() override;

	virtual bool file_exists(const String& p_name) override;
};

// ============================================================================
// FileAccessCompressed
// ============================================================================

class FileAccessCompressed final : public FileAccess
{
	Compression::Mode cmode = Compression::MODE_ZSTD;
	bool writing = false;
	uint64_t write_pos = 0;
	uint8_t* write_ptr = nullptr;
	uint64_t write_buffer_size = 0;
	uint64_t write_max = 0;
	uint32_t block_size = 0;
	mutable bool read_eof = false;
	mutable bool at_end = false;

	struct ReadBlock
	{
		uint32_t csize;
		uint64_t offset;
	};

	mutable Vector<uint8_t> comp_buffer;
	uint8_t* read_ptr = nullptr;
	mutable uint32_t read_block = 0;
	uint32_t read_block_count = 0;
	mutable uint32_t read_block_size = 0;
	mutable uint64_t read_pos = 0;
	Vector<ReadBlock> read_blocks;
	uint64_t read_total = 0;

	String magic = "GCMP";
	mutable Vector<uint8_t> buffer;
	Ref<FileAccess> f;

	void _close();

public:
	void configure(const String& p_magic, Compression::Mode p_mode = Compression::MODE_ZSTD,
		uint32_t p_block_size = 4096);

	Error open_after_magic(Ref<FileAccess> p_base);

	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override;
	virtual uint64_t _get_access_time(const String& p_file) override;
	virtual int64_t _get_size(const String& p_file) override;
	virtual uint32_t _get_unix_permissions(const String& p_file) override;
	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override;

	virtual bool _get_hidden_attribute(const String& p_file) override;
	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override;
	virtual bool _get_read_only_attribute(const String& p_file) override;
	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override;

	virtual void close() override;

	virtual ~FileAccessCompressed();
};

// ============================================================================
// FileAccessEncrypted
// ============================================================================

#define ENCRYPTED_HEADER_MAGIC 0x43454447

class FileAccessEncrypted final : public FileAccess
{
public:
	enum Mode : int32_t
	{
		MODE_READ,
		MODE_WRITE_AES256,
		MODE_MAX
	};

private:
	Vector<uint8_t> iv;
	Vector<uint8_t> key;
	bool writing = false;
	Ref<FileAccess> file;
	uint64_t base = 0;
	uint64_t length = 0;
	Vector<uint8_t> data;
	mutable uint64_t pos = 0;
	mutable bool eofed = false;
	bool use_magic = true;

	void _close();

public:
	Error open_and_parse(Ref<FileAccess> p_base, const Vector<uint8_t>& p_key, Mode p_mode,
		bool p_with_magic = true, const Vector<uint8_t>& p_iv = Vector<uint8_t>());
	Error open_and_parse_password(Ref<FileAccess> p_base, const String& p_key, Mode p_mode);

	Vector<uint8_t> get_iv() const { return iv; }

	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override;
	virtual uint64_t _get_access_time(const String& p_file) override;
	virtual int64_t _get_size(const String& p_file) override;
	virtual uint32_t _get_unix_permissions(const String& p_file) override;
	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override;

	virtual bool _get_hidden_attribute(const String& p_file) override;
	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override;
	virtual bool _get_read_only_attribute(const String& p_file) override;
	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override;

	virtual void close() override;

	~FileAccessEncrypted();
};

// ============================================================================
// PackedData / PackSource / FileAccessPack / DirAccessPack
// ============================================================================

// Godot's packed file magic header ("GDPC" in ASCII).
#define PACK_HEADER_MAGIC 0x43504447

#define PACK_FORMAT_VERSION_V2 2
#define PACK_FORMAT_VERSION_V3 3
#define PACK_FORMAT_VERSION_V4 4

// The current packed file format version number.
#define PACK_FORMAT_VERSION PACK_FORMAT_VERSION_V4

enum PackFlags
{
	PACK_DIR_ENCRYPTED = 1 << 0,
	PACK_REL_FILEBASE = 1 << 1,
	PACK_SPARSE_BUNDLE = 1 << 2,
};

enum PackFileFlags
{
	PACK_FILE_ENCRYPTED = 1 << 0,
	PACK_FILE_REMOVAL = 1 << 1,
	PACK_FILE_DELTA = 1 << 2,
};

class PackSource;

class PackedData final
{
	friend class FileAccessPack;
	friend class DirAccessPack;
	friend class PackSource;

public:
	struct PackedFile
	{
		String pack;
		uint64_t offset; // if offset is ZERO, the file was ERASED
		uint64_t size;
		uint8_t md5[16];
		PackSource* src = nullptr;
		bool encrypted;
		bool bundle;
		bool delta;
		String salt;
	};

private:
	struct PackedDir
	{
		PackedDir* parent = nullptr;
		String name;
		HashMap<String, PackedDir*> subdirs;
		HashSet<String> files;
	};

	struct PathMD5
	{
		uint64_t a = 0;
		uint64_t b = 0;

		bool operator==(const PathMD5& p_val) const { return (a == p_val.a) && (b == p_val.b); }

		static uint32_t hash(const PathMD5& p_val)
		{
			uint32_t h = hash_murmur3_one_32(p_val.a);
			return hash_fmix32(hash_murmur3_one_32(p_val.b, h));
		}

		PathMD5() {}

		explicit PathMD5(const Vector<uint8_t>& p_buf)
		{
			a = *((uint64_t*)&p_buf[0]);
			b = *((uint64_t*)&p_buf[8]);
		}
	};

public:
	struct Data
	{
		HashMap<PathMD5, PackedFile, PathMD5> files;
		HashMap<PathMD5, Vector<PackedFile>, PathMD5> delta_patches;
		Vector<PackSource*> sources;
		PackedDir* root = nullptr;
		bool disabled = false;
	};

	static inline Data* data = nullptr;

private:
	static void _free_packed_dirs(PackedDir* p_dir);
	static void _get_file_paths(
		PackedDir* p_dir, const String& p_parent_dir, HashSet<String>& r_paths);

	_FORCE_INLINE_ static PathMD5 _get_simplified_path(const String& p_path)
	{
		String simplified_path = p_path;
		if (simplified_path.begins_with("uid://")) {
			simplified_path = ResourceUID::uid_to_path(simplified_path);
		}
		simplified_path = simplified_path.simplify_path().trim_prefix("res://");
		return PathMD5(simplified_path.md5_buffer());
	}

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void add_pack_source(PackSource* p_source);
	static void add_path(const String& p_pkg_path, const String& p_path, uint64_t p_ofs,
		uint64_t p_size, const uint8_t* p_md5, PackSource* p_src, bool p_replace_files,
		bool p_encrypted = false, bool p_bundle = false, bool p_delta = false,
		const String& p_salt = String()); // for PackSource
	static void remove_path(const String& p_path);
	static uint8_t* get_file_hash(const String& p_path);
	static Vector<PackedFile> get_delta_patches(const String& p_path);
	static bool has_delta_patches(const String& p_path);
	static HashSet<String> get_file_paths();

	static void set_disabled(bool p_disabled)
	{
		if (data != nullptr) {
			data->disabled = p_disabled;
		}
	}

	_FORCE_INLINE_ static bool is_disabled() { return data != nullptr ? data->disabled : true; }

	static Error add_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());

	static void clear();

	_FORCE_INLINE_ static Ref<FileAccess> try_open_path(
		const String& p_path, const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
	_FORCE_INLINE_ static bool has_path(const String& p_path);

	_FORCE_INLINE_ static int64_t get_size(const String& p_path);

	_FORCE_INLINE_ static Ref<DirAccess> try_open_directory(const String& p_path);
	_FORCE_INLINE_ static bool has_directory(const String& p_path);

	PackedData() = delete;
	PackedData(const PackedData&) = delete;
	PackedData& operator=(const PackedData&) = delete;
	~PackedData() = delete;
};

class PackSource
{
public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) = 0;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) = 0;

	virtual ~PackSource() {}
};

class PackedSourcePCK final : public PackSource
{
public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
};

class PackedSourceDirectory final : public PackSource
{
	void add_directory(const String& p_path, bool p_replace_files);

public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
};

class FileAccessPack final : public FileAccess
{
	PackedData::PackedFile pf;

	String path;
	mutable uint64_t pos;
	mutable bool eof;
	uint64_t off;

	Ref<FileAccess> f;
	virtual Error open_internal(const String& p_path, int p_mode_flags) override;

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

public:
	virtual bool is_open() const override;

	virtual String get_path() const override { return path; }

	virtual String get_path_absolute() const override { return path; }

	virtual void seek(uint64_t p_position) override;
	virtual void seek_end(int64_t p_position = 0) override;
	virtual uint64_t get_position() const override;
	virtual uint64_t get_length() const override;

	virtual bool eof_reached() const override;

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual void set_big_endian(bool p_big_endian) override;

	virtual Error get_error() const override;

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;

	virtual bool file_exists(const String& p_name) override;

	virtual void close() override;

	FileAccessPack(const String& p_path, const PackedData::PackedFile& p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
};

int64_t PackedData::get_size(const String& p_path)
{
	if (data == nullptr) {
		return -1;
	}
	HashMap<PathMD5, PackedFile, PathMD5>::Iterator E =
		data->files.find(_get_simplified_path(p_path));
	if (!E) {
		return -1; // File not found.
	}
	if (E->value.offset == 0) {
		return -1; // File was erased.
	}
	return E->value.size;
}

Ref<FileAccess> PackedData::try_open_path(
	const String& p_path, const Vector<uint8_t>& p_decryption_key)
{
	if (data == nullptr) {
		return nullptr;
	}
	HashMap<PathMD5, PackedFile, PathMD5>::Iterator E =
		data->files.find(_get_simplified_path(p_path));
	if (!E) {
		return nullptr; // Not found.
	}

	return E->value.src->get_file(p_path, &E->value, p_decryption_key);
}

bool PackedData::has_path(const String& p_path)
{
	return data != nullptr && data->files.has(_get_simplified_path(p_path));
}

bool PackedData::has_directory(const String& p_path)
{
	if (data == nullptr) {
		return false;
	}
	Ref<DirAccess> da = try_open_directory(p_path);
	if (da.is_valid()) {
		return true;
	}
	else {
		return false;
	}
}

class DirAccessPack final : public DirAccess
{
	PackedData::PackedDir* current;

	List<String> list_dirs;
	List<String> list_files;
	bool cdir = false;

	PackedData::PackedDir* _find_dir(const String& p_dir);

public:
	virtual Error list_dir_begin() override;
	virtual String get_next() override;
	virtual bool current_is_dir() const override;
	virtual bool current_is_hidden() const override;
	virtual void list_dir_end() override;

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;

	virtual Error change_dir(String p_dir) override;
	virtual String get_current_dir(bool p_include_drive = true) const override;

	virtual bool file_exists(String p_file) override;
	virtual bool dir_exists(String p_dir) override;

	virtual Error make_dir(String p_dir) override;

	virtual Error rename(String p_from, String p_to) override;
	virtual Error remove(String p_name) override;

	uint64_t get_space_left() override;

	virtual bool is_link(String p_file) override { return false; }

	virtual String read_link(String p_file) override { return p_file; }

	virtual Error create_link(String p_source, String p_target) override { return FAILED; }

	virtual String get_filesystem_type() const override;

	DirAccessPack();
};

Ref<DirAccess> PackedData::try_open_directory(const String& p_path)
{
	Ref<DirAccess> da = memnew(DirAccessPack());
	if (da->change_dir(p_path) != OK) {
		da = Ref<DirAccess>();
	}
	return da;
}

// ============================================================================
// ZipArchive / PackedSourceZIP / FileAccessZip
// ============================================================================

#ifdef MINIZIP_ENABLED

#include <thirdparty/minizip/unzip.h>

class ZipArchive final
{
public:
	struct File
	{
		int package = -1;
		unz_file_pos file_pos;
	};

private:
	struct Package
	{
		String filename;
	};

public:
	struct Data
	{
		Vector<Package> packages;
		HashMap<String, File> files;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void close_handle(unzFile p_file);
	static unzFile get_file_handle(const String& p_file);

	static bool file_exists(const String& p_name);

	static bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		PackSource* p_source, const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
	static Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());

	ZipArchive() = delete;
	ZipArchive(const ZipArchive&) = delete;
	ZipArchive& operator=(const ZipArchive&) = delete;
	~ZipArchive() = delete;
};

class PackedSourceZIP final : public PackSource
{
public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
};

class FileAccessZip final : public FileAccess
{
	unzFile zfile = nullptr;
	unz_file_info64 file_info;

	mutable bool at_eof = false;

	void _close();

public:
	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual void close() override;

	FileAccessZip(const String& p_path, const PackedData::PackedFile& p_file);
	~FileAccessZip();
};

#endif // MINIZIP_ENABLED

// --- Folded from modules/zip/zip_reader.h ---
#include <thirdparty/minizip/unzip.h>
#include <thirdparty/minizip/zip.h>
#include "core/io/file_access.h"

class ZIPPacker final : public RefCounted
{
	Ref<FileAccess> fa;
	zipFile zf = nullptr;
	int compression_level = Z_DEFAULT_COMPRESSION;
	HashSet<String> directories;

public:
	enum ZipAppend
	{
		APPEND_CREATE = 0,
		APPEND_CREATEAFTER = 1,
		APPEND_ADDINZIP = 2,
	};

	enum CompressionLevel
	{
		COMPRESSION_DEFAULT = Z_DEFAULT_COMPRESSION,
		COMPRESSION_NONE = Z_NO_COMPRESSION,
		COMPRESSION_FAST = Z_BEST_SPEED,
		COMPRESSION_BEST = Z_BEST_COMPRESSION,
	};

	Error open(const String& p_path, ZipAppend p_append);
	Error close();

	void set_compression_level(int p_compression_level);
	int get_compression_level() const;

	Error start_file(
		const String& p_path, uint32_t p_permissions = 0644, uint64_t p_modified_time = 0);
	Error write_file(const Vector<uint8_t>& p_data);
	Error close_file();

	Error add_directory(
		const String& p_path, uint32_t p_permissions = 0755, uint64_t p_modified_time = 0);

	ZIPPacker();
	~ZIPPacker();
};

class ZIPReader final : public RefCounted
{
	Ref<FileAccess> fa;
	unzFile uzf = nullptr;

public:
	Error open(const String& p_path);
	Error close();

	PackedStringArray get_files();
	PackedByteArray read_file(const String& p_path, bool p_case_sensitive);
	bool file_exists(const String& p_path, bool p_case_sensitive);
	int get_compression_level(const String& p_path, bool p_case_sensitive);

	ZIPReader();
	~ZIPReader();
};
// --- End modules/zip/zip_reader.h ---
