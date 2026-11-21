/**************************************************************************/
/*  compression.h                                                         */
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

#include "core/templates/vector.h"
#include "core/typedefs.h"

#include <zlib.h>

class Compression {
public:
	static inline int zlib_level = Z_DEFAULT_COMPRESSION;
	static inline int gzip_level = Z_DEFAULT_COMPRESSION;
	static inline int zstd_level = 3;
	static inline bool zstd_long_distance_matching = false;
	static inline int zstd_window_log_size = 27; // ZSTD_WINDOWLOG_LIMIT_DEFAULT
	static inline int gzip_chunk = 16384;

	enum Mode : int32_t {
		MODE_FASTLZ,
		MODE_DEFLATE,
		MODE_ZSTD,
		MODE_GZIP,
		MODE_BROTLI
	};

	static int64_t compress(uint8_t *p_dst, const uint8_t *p_src, int64_t p_src_size, Mode p_mode = MODE_ZSTD);
	static int64_t get_max_compressed_buffer_size(int64_t p_src_size, Mode p_mode = MODE_ZSTD);
	static int64_t decompress(uint8_t *p_dst, int64_t p_dst_max_size, const uint8_t *p_src, int64_t p_src_size, Mode p_mode = MODE_ZSTD);
	static int decompress_dynamic(Vector<uint8_t> *p_dst_vect, int64_t p_max_dst_size, const uint8_t *p_src, int64_t p_src_size, Mode p_mode);
};

// --- Folded from core/io/delta_encoding.h ---
#include "core/templates/span.h"

#include <cstdint>

template <typename T>
class Vector;

class DeltaEncoding {
public:
	static Error encode_delta(Span<uint8_t> p_old_data, Span<uint8_t> p_new_data, Vector<uint8_t> &r_delta, int p_compression_level = 19);
	static Error decode_delta(Span<uint8_t> p_old_data, Span<uint8_t> p_delta, Vector<uint8_t> &r_new_data);
};

// --- End core/io/delta_encoding.h ---

// --- Folded from core/io/packed_data_container.h ---
#include "core/io/resource.h"

class PackedDataContainer : public Resource
{
	friend class PackedDataContainerRef;

	enum : uint32_t
	{
		TYPE_DICT = 0xFFFFFFFF,
		TYPE_ARRAY = 0xFFFFFFFE,
	};

	struct DictKey
	{
		uint32_t hash;
		bool operator<(const DictKey& p_key) const { return hash < p_key.hash; }
	};

	Vector<uint8_t> data;
	int datalen = 0;


	uint32_t _type_at_ofs(uint32_t p_ofs) const;
	int _size(uint32_t p_ofs) const;

protected:
	void _set_data(const Vector<uint8_t>& p_data);
	Vector<uint8_t> _get_data() const;

public:
	int size() const;
};

class PackedDataContainerRef : public RefCounted
{
	friend class PackedDataContainer;
	uint32_t offset = 0;
	Ref<PackedDataContainer> from;

protected:

public:
	int size() const;
};



// --- End core/io/packed_data_container.h ---

// --- Folded from core/io/pck_packer.h ---
#include "core/types.h"

class FileAccess;

class PCKPacker : public RefCounted
{
	Ref<FileAccess> file;
	int alignment = 0;

	Vector<uint8_t> key;
	bool enc_dir = false;

	uint64_t file_base = 0;
	uint64_t file_base_ofs = 0;
	uint64_t dir_base_ofs = 0;


	struct File
	{
		String path;
		String src_path;
		uint64_t ofs = 0;
		uint64_t size = 0;
		bool encrypted = false;
		bool removal = false;
		Vector<uint8_t> md5;
	};

	Vector<File> files;

	Error _add_file(const String& p_target_path, const String& p_source_path,
		const Vector<uint8_t>& p_data, bool p_encrypt = false);

public:
	Error pck_start(const String& p_pck_path, int p_alignment = 32,
		const String& p_key = "0000000000000000000000000000000000000000000000000000000000000000",
		bool p_encrypt_directory = false);
	Error add_file(
		const String& p_target_path, const String& p_source_path, bool p_encrypt = false);
	Error add_file_from_buffer(
		const String& p_target_path, const Vector<uint8_t>& p_data, bool p_encrypt = false);
	Error add_file_removal(const String& p_target_path);
	Error flush(bool p_verbose = false);

	~PCKPacker();
};



// --- End core/io/pck_packer.h ---
