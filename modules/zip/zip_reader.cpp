/**************************************************************************/
/*  zip_reader.cpp                                                        */
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

#include "zip_reader.h"
#include "register_types.h"

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/os/time.h"

// ZIPPacker

Error ZIPPacker::open(const String& p_path, ZipAppend p_append)
{
	if (fa.is_valid()) {
		close();
	}

	zlib_filefunc_def io = zipio_create_io(&fa);
	zf = zipOpen2(p_path.utf8().get_data(), p_append, nullptr, &io);
	return zf != nullptr ? OK : FAILED;
}

Error ZIPPacker::close()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker cannot be closed because it is not open.");

	Error err = zipClose(zf, nullptr) == ZIP_OK ? OK : FAILED;
	if (err == OK) {
		DEV_ASSERT(fa.is_null());
		directories.clear();
		zf = nullptr;
	}

	return err;
}

void ZIPPacker::set_compression_level(int p_compression_level)
{
	ERR_FAIL_COND_MSG(
		p_compression_level < Z_DEFAULT_COMPRESSION || p_compression_level > Z_BEST_COMPRESSION,
		"Invalid compression level.");
	compression_level = p_compression_level;
}

int ZIPPacker::get_compression_level() const { return compression_level; }

Error ZIPPacker::write_file(const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker must be opened before use.");

	return zipWriteInFileInZip(zf, p_data.ptr(), p_data.size()) == ZIP_OK ? OK : FAILED;
}

Error ZIPPacker::close_file()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker must be opened before use.");

	return zipCloseFileInZip(zf) == ZIP_OK ? OK : FAILED;
}

ZIPPacker::ZIPPacker() {}

ZIPPacker::~ZIPPacker()
{
	if (fa.is_valid()) {
		close();
	}
}

Error ZIPPacker::start_file(
	const String& p_path, uint32_t p_permissions, uint64_t p_modified_time)
{
	return OK;
}

// ZIPReader

Error ZIPReader::open(const String& p_path)
{
	if (fa.is_valid()) {
		close();
	}

	zlib_filefunc_def io = zipio_create_io(&fa);
	uzf = unzOpen2(p_path.utf8().get_data(), &io);
	return uzf != nullptr ? OK : FAILED;
}

Error ZIPReader::close()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPReader cannot be closed because it is not open.");

	Error err = unzClose(uzf) == UNZ_OK ? OK : FAILED;
	if (err == OK) {
		DEV_ASSERT(fa.is_null());
		uzf = nullptr;
	}

	return err;
}

PackedStringArray ZIPReader::get_files()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedStringArray(), "ZIPReader must be opened before use.");

	unz_global_info gi;
	int err = unzGetGlobalInfo(uzf, &gi);
	ERR_FAIL_COND_V(err != UNZ_OK, PackedStringArray());
	if (gi.number_entry == 0) {
		return PackedStringArray();
	}

	err = unzGoToFirstFile(uzf);
	ERR_FAIL_COND_V(err != UNZ_OK, PackedStringArray());

	List<String> s;
	do {
		unz_file_info64 file_info;
		String filepath;

		err = godot_unzip_get_current_file_info(uzf, file_info, filepath);
		if (err == UNZ_OK) {
			s.push_back(filepath);
		}
	} while (unzGoToNextFile(uzf) == UNZ_OK);

	PackedStringArray arr;
	arr.resize(s.size());
	int idx = 0;
	for (const List<String>::Element* E = s.front(); E; E = E->next()) {
		arr.set(idx++, E->get());
	}
	return arr;
}

PackedByteArray ZIPReader::read_file(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "ZIPReader must be opened before use.");

	int err = UNZ_OK;

	// Locate and open the file.
	err = godot_unzip_locate_file(uzf, p_path, p_case_sensitive);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "File does not exist in zip archive: " + p_path);
	err = unzOpenCurrentFile(uzf);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "Could not open file within zip archive.");

	// Read the file info.
	unz_file_info info;
	err = unzGetCurrentFileInfo(uzf, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "Unable to read file information from zip archive.");
	ERR_FAIL_COND_V_MSG(info.uncompressed_size > INT_MAX, PackedByteArray(),
		"File contents too large to read from zip archive (>2 GB).");

	// Read the file data.
	PackedByteArray data;
	data.resize(info.uncompressed_size);
	uint8_t* buffer = data.ptrw();
	int to_read = data.size();
	while (to_read > 0) {
		int bytes_read = unzReadCurrentFile(uzf, buffer, to_read);
		ERR_FAIL_COND_V_MSG(
			bytes_read < 0, PackedByteArray(), "IO/zlib error reading file from zip archive.");
		ERR_FAIL_COND_V_MSG(bytes_read == UNZ_EOF && to_read != 0, PackedByteArray(),
			"Incomplete file read from zip archive.");
		DEV_ASSERT(bytes_read <= to_read);
		buffer += bytes_read;
		to_read -= bytes_read;
	}

	// Verify the data and return.
	err = unzCloseCurrentFile(uzf);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "CRC error reading file from zip archive.");
	return data;
}

bool ZIPReader::file_exists(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "ZIPReader must be opened before use.");

	int cs = p_case_sensitive ? 1 : 2;
	if (unzLocateFile(uzf, p_path.utf8().get_data(), cs) != UNZ_OK) {
		return false;
	}
	if (unzOpenCurrentFile(uzf) != UNZ_OK) {
		return false;
	}

	unzCloseCurrentFile(uzf);
	return true;
}

int ZIPReader::get_compression_level(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), -1, "ZIPReader must be opened before use.");

	int cs = p_case_sensitive ? 1 : 2;
	if (unzLocateFile(uzf, p_path.utf8().get_data(), cs) != UNZ_OK) {
		return -1;
	}

	int method;
	int level;
	if (unzOpenCurrentFile2(uzf, &method, &level, 1) != UNZ_OK) {
		return -1;
	}

	unzCloseCurrentFile(uzf);

	return level;
}

ZIPReader::ZIPReader() {}

ZIPReader::~ZIPReader()
{
	if (fa.is_valid()) {
		close();
	}
}

// Module registration

void initialize_zip_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_zip_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
