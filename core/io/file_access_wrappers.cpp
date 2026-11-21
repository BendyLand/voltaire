/**************************************************************************/
/*  file_access_wrappers.cpp                                              */
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

#include "file_access_wrappers.h"

#include "core/config/project_settings.h"
#include "core/io/delta_encoding.h"
#include "core/math/math_funcs_binary.h"
#include "core/os/os.h"
#include "core/version.h"

// ============================================================================
// FileAccessMemory
// ============================================================================

static HashMap<String, Vector<uint8_t>>* memory_files = nullptr;

void FileAccessMemory::register_file(const String& p_name, const Vector<uint8_t>& p_data)
{
	if (!memory_files) {
		memory_files = memnew((HashMap<String, Vector<uint8_t>>));
	}

	String name;
	if (ProjectSettings::is_initialized()) {
		name = ProjectSettings::globalize_path(p_name);
	}
	else {
		name = p_name;
	}

	(*memory_files)[name] = p_data;
}

void FileAccessMemory::cleanup()
{
	if (!memory_files) {
		return;
	}

	memdelete(memory_files);
	memory_files = nullptr;
}

Ref<FileAccess> FileAccessMemory::create() { return memnew(FileAccessMemory); }

bool FileAccessMemory::file_exists(const String& p_name)
{
	String name = fix_path(p_name);
	return memory_files && (memory_files->find(name) != nullptr);
}

Error FileAccessMemory::open_custom(const uint8_t* p_data, uint64_t p_len)
{
	data = (uint8_t*)p_data;
	length = p_len;
	pos = 0;
	return OK;
}

Error FileAccessMemory::open_internal(const String& p_path, int p_mode_flags)
{
	ERR_FAIL_NULL_V(memory_files, ERR_FILE_NOT_FOUND);

	String name = fix_path(p_path);

	HashMap<String, Vector<uint8_t>>::Iterator E = memory_files->find(name);
	ERR_FAIL_COND_V_MSG(!E, ERR_FILE_NOT_FOUND, vformat("Can't find file '%s'.", p_path));

	data = E->value.ptrw();
	length = E->value.size();
	pos = 0;

	return OK;
}

bool FileAccessMemory::is_open() const { return data != nullptr; }

void FileAccessMemory::seek(uint64_t p_position)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(p_position > length);
	pos = p_position;
}

void FileAccessMemory::seek_end(int64_t p_position)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND((int64_t)length + p_position < 0);
	seek(length + p_position);
}

uint64_t FileAccessMemory::get_position() const
{
	ERR_FAIL_NULL_V(data, 0);
	return pos;
}

uint64_t FileAccessMemory::get_length() const
{
	ERR_FAIL_NULL_V(data, 0);
	return length;
}

bool FileAccessMemory::eof_reached() const { return pos >= length; }

uint64_t FileAccessMemory::get_buffer(uint8_t* p_dst, uint64_t p_length) const
{
	if (!p_length) {
		return 0;
	}

	ERR_FAIL_NULL_V(p_dst, -1);
	ERR_FAIL_NULL_V(data, -1);

	uint64_t left = length - pos;
	uint64_t read = MIN(p_length, left);

	if (read < p_length) {
		WARN_PRINT("Reading less data than requested");
	}

	memcpy(p_dst, &data[pos], read);
	pos += read;

	return read;
}

Error FileAccessMemory::get_error() const { return pos >= length ? ERR_FILE_EOF : OK; }

void FileAccessMemory::flush() { ERR_FAIL_NULL(data); }

bool FileAccessMemory::store_buffer(const uint8_t* p_src, uint64_t p_length)
{
	if (!p_length) {
		return true;
	}

	ERR_FAIL_NULL_V(p_src, false);

	uint64_t left = length - pos;
	uint64_t write = MIN(p_length, left);

	memcpy(&data[pos], p_src, write);
	pos += write;

	ERR_FAIL_COND_V_MSG(write < p_length, false, "Writing less data than requested.");

	return true;
}

// ============================================================================
// FileAccessPatched
// ============================================================================

Error FileAccessPatched::_apply_patch() const
{
	ERR_FAIL_COND_V(!is_open(), FAILED);

	String path = old_file->get_path();
	Vector<PackedData::PackedFile> delta_patches = PackedData::get_delta_patches(path);
	Vector<uint8_t> old_file_data = old_file->get_buffer(old_file->get_length());

	for (int i = 0; i < delta_patches.size(); ++i) {
		const PackedData::PackedFile& delta_patch = delta_patches[i];
		ERR_FAIL_COND_V(delta_patch.bundle, FAILED);

		Error err = OK;

		uint64_t total_usec_start = OS::get_ticks_usec();
		uint64_t io_usec_start = OS::get_ticks_usec();

		Ref<FileAccess> patch_file = FileAccess::open(delta_patch.pack, FileAccess::READ, &err);
		ERR_FAIL_COND_V(err != OK, err);

		patch_file->seek(delta_patch.offset);
		ERR_FAIL_COND_V(patch_file->get_error() != OK, patch_file->get_error());

		Vector<uint8_t> patch_data = patch_file->get_buffer(delta_patch.size);
		ERR_FAIL_COND_V(patch_data.is_empty(), ERR_FILE_CANT_READ);

		uint64_t io_usec_end = OS::get_ticks_usec();
		uint64_t decode_usec_start = OS::get_ticks_usec();

		Vector<uint8_t> new_file_data;
		err = DeltaEncoding::decode_delta(old_file_data, patch_data, new_file_data);
		ERR_FAIL_COND_V_MSG(err != OK, err,
			vformat("Failed to apply delta patch (%d of %d) to \"%s\".", i + 1,
				delta_patches.size(), path));

		uint64_t decode_usec_end = OS::get_ticks_usec();

		old_file_data = new_file_data;

		uint64_t total_usec_end = OS::get_ticks_usec();

		print_verbose(vformat(
			U"Applied delta patch to \"%s\" from \"%s\" in %d μs (%d μs I/O, %d μs decoding).",
			path, delta_patch.pack.get_file(), total_usec_end - total_usec_start,
			io_usec_end - io_usec_start, decode_usec_end - decode_usec_start));
	}

	patched_file_data = old_file_data;
	patched_file.instantiate();
	return patched_file->open_custom(patched_file_data.ptr(), patched_file_data.size());
}

bool FileAccessPatched::_try_apply_patch() const
{
	if (last_error != OK) {
		return false;
	}

	if (patched_file.is_valid()) {
		return true;
	}

	last_error = _apply_patch();
	return last_error == OK;
}

Error FileAccessPatched::open_custom(const Ref<FileAccess>& p_old_file)
{
	close();

	if (!p_old_file->is_open()) {
		last_error = ERR_FILE_CANT_OPEN;
		return last_error;
	}

	old_file = p_old_file;

	return OK;
}

bool FileAccessPatched::is_open() const { return old_file.is_valid() && old_file->is_open(); }

void FileAccessPatched::seek(uint64_t p_position)
{
	if (!_try_apply_patch()) {
		return;
	}

	patched_file->seek(p_position);
}

void FileAccessPatched::seek_end(int64_t p_position)
{
	if (!_try_apply_patch()) {
		return;
	}

	patched_file->seek_end(p_position);
}

uint64_t FileAccessPatched::get_position() const
{
	if (!_try_apply_patch()) {
		return 0;
	}

	return patched_file->get_position();
}

uint64_t FileAccessPatched::get_length() const
{
	if (!_try_apply_patch()) {
		return 0;
	}

	return patched_file->get_length();
}

bool FileAccessPatched::eof_reached() const
{
	if (!_try_apply_patch()) {
		return true;
	}

	return patched_file->eof_reached();
}

Error FileAccessPatched::get_error() const
{
	if (last_error != OK) {
		return last_error;
	}

	if (patched_file.is_valid()) {
		Error inner_error = patched_file->get_error();
		if (inner_error != OK) {
			return inner_error;
		}
	}

	return last_error;
}

bool FileAccessPatched::store_buffer(const uint8_t* p_src, uint64_t p_length)
{
	if (!_try_apply_patch()) {
		return false;
	}

	return patched_file->store_buffer(p_src, p_length);
}

uint64_t FileAccessPatched::get_buffer(uint8_t* p_dst, uint64_t p_length) const
{
	if (!_try_apply_patch()) {
		return 0;
	}

	return patched_file->get_buffer(p_dst, p_length);
}

void FileAccessPatched::flush()
{
	if (!_try_apply_patch()) {
		return;
	}

	patched_file->flush();
}

void FileAccessPatched::close()
{
	old_file = Ref<FileAccess>();
	patched_file = Ref<FileAccessMemory>();
	patched_file_data.clear();
	last_error = OK;
}

bool FileAccessPatched::file_exists(const String& p_name)
{
	ERR_FAIL_COND_V(old_file.is_null(), false);
	return old_file->file_exists(p_name);
}

// ============================================================================
// FileAccessCompressed
// ============================================================================

void FileAccessCompressed::configure(
	const String& p_magic, Compression::Mode p_mode, uint32_t p_block_size)
{
	magic = p_magic.ascii().get_data();
	magic = (magic + "    ").substr(0, 4);

	cmode = p_mode;
	block_size = p_block_size;
}

Error FileAccessCompressed::open_after_magic(Ref<FileAccess> p_base)
{
	f = p_base;
	cmode = (Compression::Mode)f->get_32();
	block_size = f->get_32();
	if (block_size == 0) {
		f.unref();
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT,
			vformat("Can't open compressed file '%s' with block size 0, it is corrupted.",
				p_base->get_path()));
	}
	read_total = f->get_32();
	uint32_t bc = (read_total / block_size) + 1;
	uint64_t acc_ofs = f->get_position() + bc * 4;
	uint32_t max_bs = 0;
	for (uint32_t i = 0; i < bc; i++) {
		ReadBlock rb;
		rb.offset = acc_ofs;
		rb.csize = f->get_32();
		acc_ofs += rb.csize;
		max_bs = MAX(max_bs, rb.csize);
		read_blocks.push_back(rb);
	}

	comp_buffer.resize(max_bs);
	buffer.resize(block_size);
	read_ptr = buffer.ptrw();
	f->get_buffer(comp_buffer.ptrw(), read_blocks[0].csize);
	at_end = false;
	read_eof = false;
	read_block_count = bc;
	read_block_size = read_blocks.size() == 1 ? read_total : block_size;

	const int64_t ret = Compression::decompress(
		buffer.ptrw(), read_block_size, comp_buffer.ptr(), read_blocks[0].csize, cmode);
	read_block = 0;
	read_pos = 0;

	return ret == -1 ? ERR_FILE_CORRUPT : OK;
}

Error FileAccessCompressed::open_internal(const String& p_path, int p_mode_flags)
{
	ERR_FAIL_COND_V(p_mode_flags == READ_WRITE, ERR_UNAVAILABLE);
	_close();

	Error err;
	f = FileAccess::open(p_path, p_mode_flags, &err);
	if (err != OK) {
		// not openable
		f.unref();
		return err;
	}

	if (p_mode_flags & WRITE) {
		buffer.clear();
		writing = true;
		write_pos = 0;
		write_buffer_size = 256;
		buffer.resize(256);
		write_max = 0;
		write_ptr = buffer.ptrw();
	}
	else {
		char rmagic[5];
		f->get_buffer((uint8_t*)rmagic, 4);
		rmagic[4] = 0;
		err = ERR_FILE_UNRECOGNIZED;
		if (magic != rmagic || (err = open_after_magic(f)) != OK) {
			f.unref();
			return err;
		}
	}

	return OK;
}

void FileAccessCompressed::_close()
{
	if (f.is_null()) {
		return;
	}

	if (writing) {
		// save block table and all compressed blocks
		CharString mgc = magic.utf8();
		f->store_buffer((const uint8_t*)mgc.get_data(), mgc.length()); // write header 4
		f->store_32(cmode);											   // write compression mode 4
		f->store_32(block_size);									   // write block size 4
		f->store_32(uint32_t(write_max));							   // max amount of data written 4
		uint32_t bc = (write_max / block_size) + 1;

		for (uint32_t i = 0; i < bc; i++) {
			f->store_32(0); // compressed sizes, will update later
		}

		uint32_t last_block_size = write_max % block_size;

		// Temporary buffer for compressed data blocks.
		LocalVector<uint8_t> temp_cblock;
		temp_cblock.resize(Compression::get_max_compressed_buffer_size(
			bc == 1 ? last_block_size : block_size, cmode));
		uint8_t* temp_cblock_ptr = temp_cblock.ptr();

		// Compress and store the blocks.
		LocalVector<uint32_t> block_sizes;
		for (uint32_t i = 0; i < bc; i++) {
			uint32_t bl = i == (bc - 1) ? last_block_size : block_size;
			uint8_t* bp = &write_ptr[i * block_size];

			const int64_t compressed_size = Compression::compress(temp_cblock_ptr, bp, bl, cmode);
			ERR_FAIL_COND_MSG(compressed_size < 0, "FileAccessCompressed: Error compressing data.");

			f->store_buffer(temp_cblock_ptr, (uint64_t)compressed_size);
			block_sizes.push_back(compressed_size);
		}

		f->seek(16); // ok write block sizes
		for (uint32_t i = 0; i < bc; i++) {
			f->store_32(block_sizes[i]);
		}
		f->seek_end();
		f->store_buffer((const uint8_t*)mgc.get_data(), mgc.length()); // magic at the end too
	}
	else {
		comp_buffer.clear();
		read_blocks.clear();
	}
	buffer.clear();
	f.unref();
}

bool FileAccessCompressed::is_open() const { return f.is_valid(); }

String FileAccessCompressed::get_path() const
{
	if (f.is_valid()) {
		return f->get_path();
	}
	else {
		return "";
	}
}

String FileAccessCompressed::get_path_absolute() const
{
	if (f.is_valid()) {
		return f->get_path_absolute();
	}
	else {
		return "";
	}
}

void FileAccessCompressed::seek(uint64_t p_position)
{
	ERR_FAIL_COND_MSG(f.is_null(), "File must be opened before use.");

	if (writing) {
		ERR_FAIL_COND(p_position > write_max);

		write_pos = p_position;
	}
	else {
		ERR_FAIL_COND(p_position > read_total);
		if (p_position == read_total) {
			at_end = true;
		}
		else {
			at_end = false;
			read_eof = false;
			uint32_t block_idx = p_position / block_size;
			if (block_idx != read_block) {
				read_block = block_idx;
				f->seek(read_blocks[read_block].offset);
				f->get_buffer(comp_buffer.ptrw(), read_blocks[read_block].csize);
				const int64_t ret = Compression::decompress(buffer.ptrw(),
					read_blocks.size() == 1 ? read_total : block_size, comp_buffer.ptr(),
					read_blocks[read_block].csize, cmode);
				ERR_FAIL_COND_MSG(ret == -1, "Compressed file is corrupt.");
				read_block_size =
					read_block == read_block_count - 1 ? read_total % block_size : block_size;
			}

			read_pos = p_position % block_size;
		}
	}
}

void FileAccessCompressed::seek_end(int64_t p_position)
{
	ERR_FAIL_COND_MSG(f.is_null(), "File must be opened before use.");
	if (writing) {
		seek(write_max + p_position);
	}
	else {
		seek(read_total + p_position);
	}
}

uint64_t FileAccessCompressed::get_position() const
{
	ERR_FAIL_COND_V_MSG(f.is_null(), 0, "File must be opened before use.");
	if (writing) {
		return write_pos;
	}
	else {
		return (uint64_t)read_block * block_size + read_pos;
	}
}

uint64_t FileAccessCompressed::get_length() const
{
	ERR_FAIL_COND_V_MSG(f.is_null(), 0, "File must be opened before use.");
	if (writing) {
		return write_max;
	}
	else {
		return read_total;
	}
}

bool FileAccessCompressed::eof_reached() const
{
	ERR_FAIL_COND_V_MSG(f.is_null(), false, "File must be opened before use.");
	if (writing) {
		return false;
	}
	else {
		return read_eof;
	}
}

uint64_t FileAccessCompressed::get_buffer(uint8_t* p_dst, uint64_t p_length) const
{
	if (p_length == 0) {
		return 0;
	}

	ERR_FAIL_NULL_V(p_dst, -1);
	ERR_FAIL_COND_V_MSG(f.is_null(), -1, "File must be opened before use.");
	ERR_FAIL_COND_V_MSG(writing, -1, "File has not been opened in read mode.");

	if (at_end) {
		read_eof = true;
		return 0;
	}

	uint64_t dst_idx = 0;
	while (true) {
		// Copy over as much of our current block as possible.
		const uint32_t copied_bytes_count = MIN(p_length - dst_idx, read_block_size - read_pos);
		memcpy(p_dst + dst_idx, read_ptr + read_pos, copied_bytes_count);
		dst_idx += copied_bytes_count;
		read_pos += copied_bytes_count;

		if (dst_idx == p_length) {
			// We're done! We read back all that was requested.
			return p_length;
		}

		// We're not done yet; try reading the next block.
		read_block++;

		if (read_block >= read_block_count) {
			// We're done! We read back the whole file.
			read_block--;
			at_end = true;
			if (dst_idx + 1 < p_length) {
				read_eof = true;
			}
			return dst_idx;
		}

		// Read the next block of compressed data.
		f->get_buffer(comp_buffer.ptrw(), read_blocks[read_block].csize);
		const int64_t ret = Compression::decompress(buffer.ptrw(),
			read_blocks.size() == 1 ? read_total : block_size, comp_buffer.ptr(),
			read_blocks[read_block].csize, cmode);
		ERR_FAIL_COND_V_MSG(ret == -1, -1, "Compressed file is corrupt.");
		read_block_size = read_block == read_block_count - 1 ? read_total % block_size : block_size;
		read_pos = 0;
	}

	return p_length;
}

Error FileAccessCompressed::get_error() const { return read_eof ? ERR_FILE_EOF : OK; }

void FileAccessCompressed::flush()
{
	ERR_FAIL_COND_MSG(f.is_null(), "File must be opened before use.");
	ERR_FAIL_COND_MSG(!writing, "File has not been opened in write mode.");
}

bool FileAccessCompressed::store_buffer(const uint8_t* p_src, uint64_t p_length)
{
	ERR_FAIL_COND_V_MSG(f.is_null(), false, "File must be opened before use.");
	ERR_FAIL_COND_V_MSG(!writing, false, "File has not been opened in write mode.");

	if (write_pos + (p_length) > write_max) {
		write_max = write_pos + (p_length);
	}
	if (write_max > write_buffer_size) {
		write_buffer_size = Math::next_power_of_2(write_max);
		ERR_FAIL_COND_V(buffer.resize(write_buffer_size) != OK, false);
		write_ptr = buffer.ptrw();
	}

	if (p_length) {
		memcpy(write_ptr + write_pos, p_src, p_length);
	}

	write_pos += p_length;
	return true;
}

bool FileAccessCompressed::file_exists(const String& p_name)
{
	Ref<FileAccess> fa = FileAccess::open(p_name, FileAccess::READ);
	if (fa.is_null()) {
		return false;
	}
	return true;
}

uint64_t FileAccessCompressed::_get_modified_time(const String& p_file)
{
	if (f.is_valid()) {
		return f->get_modified_time(p_file);
	}
	else {
		return 0;
	}
}

uint64_t FileAccessCompressed::_get_access_time(const String& p_file)
{
	if (f.is_valid()) {
		return f->get_access_time(p_file);
	}
	else {
		return 0;
	}
}

int64_t FileAccessCompressed::_get_size(const String& p_file)
{
	if (f.is_valid()) {
		return f->get_size(p_file);
	}
	else {
		return -1;
	}
}

uint32_t FileAccessCompressed::_get_unix_permissions(const String& p_file)
{
	if (f.is_valid()) {
		return f->_get_unix_permissions(p_file);
	}
	return 0;
}

Error FileAccessCompressed::_set_unix_permissions(const String& p_file, uint32_t p_permissions)
{
	if (f.is_valid()) {
		return f->_set_unix_permissions(p_file, p_permissions);
	}
	return FAILED;
}

bool FileAccessCompressed::_get_hidden_attribute(const String& p_file)
{
	if (f.is_valid()) {
		return f->_get_hidden_attribute(p_file);
	}
	return false;
}

Error FileAccessCompressed::_set_hidden_attribute(const String& p_file, bool p_hidden)
{
	if (f.is_valid()) {
		return f->_set_hidden_attribute(p_file, p_hidden);
	}
	return FAILED;
}

bool FileAccessCompressed::_get_read_only_attribute(const String& p_file)
{
	if (f.is_valid()) {
		return f->_get_read_only_attribute(p_file);
	}
	return false;
}

Error FileAccessCompressed::_set_read_only_attribute(const String& p_file, bool p_ro)
{
	if (f.is_valid()) {
		return f->_set_read_only_attribute(p_file, p_ro);
	}
	return FAILED;
}

void FileAccessCompressed::close() { _close(); }

FileAccessCompressed::~FileAccessCompressed() { _close(); }

// ============================================================================
// FileAccessEncrypted
// ============================================================================

Error FileAccessEncrypted::open_and_parse(Ref<FileAccess> p_base, const Vector<uint8_t>& p_key,
	Mode p_mode, bool p_with_magic, const Vector<uint8_t>& p_iv)
{
	ERR_FAIL_COND_V_MSG(file.is_valid(), ERR_ALREADY_IN_USE,
		vformat("Can't open file while another file from path '%s' is open.",
			file->get_path_absolute()));
	ERR_FAIL_COND_V(p_key.size() != 32, ERR_INVALID_PARAMETER);

	pos = 0;
	eofed = false;
	use_magic = p_with_magic;

	if (p_mode == MODE_WRITE_AES256) {
		data.clear();
		writing = true;
		file = p_base;
		key = p_key;
		if (p_iv.is_empty()) {
			iv.resize(16);
			Error err = CryptoCore::generate_random(iv.ptrw(), 16);
			ERR_FAIL_COND_V(err != OK, err);
		}
		else {
			ERR_FAIL_COND_V(p_iv.size() != 16, ERR_INVALID_PARAMETER);
			iv = p_iv;
		}
	}
	else if (p_mode == MODE_READ) {
		writing = false;
		key = p_key;

		if (use_magic) {
			uint32_t magic = p_base->get_32();
			ERR_FAIL_COND_V(magic != ENCRYPTED_HEADER_MAGIC, ERR_FILE_UNRECOGNIZED);
		}

		unsigned char md5d[16];
		p_base->get_buffer(md5d, 16);
		length = p_base->get_64();

		iv.resize(16);
		p_base->get_buffer(iv.ptrw(), 16);

		base = p_base->get_position();
		ERR_FAIL_COND_V(p_base->get_length() < base + length, ERR_FILE_CORRUPT);
		uint64_t ds = length;
		if (ds % 16) {
			ds += 16 - (ds % 16);
		}
		data.resize(ds);

		uint64_t blen = p_base->get_buffer(data.ptrw(), ds);
		ERR_FAIL_COND_V(blen != ds, ERR_FILE_CORRUPT);

		CryptoCore::decrypt_cfb(data.ptrw(), data.ptrw(), ds, key.ptrw(), key.size(), iv.ptrw());
		data.resize(length);

		unsigned char hash[16];
		ERR_FAIL_COND_V(CryptoCore::md5(data.ptr(), data.size(), hash) != OK, ERR_BUG);

		ERR_FAIL_COND_V_MSG(String::md5(hash) != String::md5(md5d), ERR_FILE_CORRUPT,
			"The MD5 sum of the decrypted file does not match the expected value. It could be that "
			"the file is corrupt, or that the provided decryption key is invalid.");

		file = p_base;
	}

	return OK;
}

Error FileAccessEncrypted::open_and_parse_password(
	Ref<FileAccess> p_base, const String& p_key, Mode p_mode)
{
	String cs = p_key.md5_text();
	ERR_FAIL_COND_V(cs.length() != 32, ERR_INVALID_PARAMETER);
	Vector<uint8_t> key_md5;
	key_md5.resize(32);
	for (int i = 0; i < 32; i++) {
		key_md5.write[i] = cs[i];
	}

	return open_and_parse(p_base, key_md5, p_mode);
}

Error FileAccessEncrypted::open_internal(const String& p_path, int p_mode_flags) { return OK; }

void FileAccessEncrypted::_close()
{
	if (file.is_null()) {
		return;
	}

	if (writing) {
		LocalVector<uint8_t> compressed;
		uint64_t len = data.size();
		if (len % 16) {
			len += 16 - (len % 16);
		}

		unsigned char hash[16];
		ERR_FAIL_COND(CryptoCore::md5(data.ptr(), data.size(), hash) != OK);

		compressed.resize(len);
		memcpy(compressed.ptr(), data.ptr(), data.size());
		memset(compressed.ptr() + data.size(), 0, len - data.size());
		Error err = CryptoCore::encrypt_cfb(
			compressed.ptr(), compressed.ptr(), len, key.ptr(), key.size(), iv.ptr());
		ERR_FAIL_COND_MSG(err, "Error encrypting file data: " + itos(err));

		if (use_magic) {
			file->store_32(ENCRYPTED_HEADER_MAGIC);
		}

		file->store_buffer(hash, 16);
		file->store_64(data.size());
		file->store_buffer(iv.ptr(), 16);

		file->store_buffer(compressed.ptr(), compressed.size());
		data.clear();
	}

	file.unref();
}

bool FileAccessEncrypted::is_open() const { return file.is_valid(); }

String FileAccessEncrypted::get_path() const
{
	if (file.is_valid()) {
		return file->get_path();
	}
	else {
		return "";
	}
}

String FileAccessEncrypted::get_path_absolute() const
{
	if (file.is_valid()) {
		return file->get_path_absolute();
	}
	else {
		return "";
	}
}

void FileAccessEncrypted::seek(uint64_t p_position)
{
	if (p_position > get_length()) {
		p_position = get_length();
	}

	pos = p_position;
	eofed = false;
}

void FileAccessEncrypted::seek_end(int64_t p_position) { seek(get_length() + p_position); }

uint64_t FileAccessEncrypted::get_position() const { return pos; }

uint64_t FileAccessEncrypted::get_length() const { return data.size(); }

bool FileAccessEncrypted::eof_reached() const { return eofed; }

uint64_t FileAccessEncrypted::get_buffer(uint8_t* p_dst, uint64_t p_length) const
{
	ERR_FAIL_COND_V_MSG(writing, -1, "File has not been opened in read mode.");

	if (!p_length) {
		return 0;
	}

	ERR_FAIL_NULL_V(p_dst, -1);

	uint64_t to_copy = MIN(p_length, get_length() - pos);

	memcpy(p_dst, data.ptr() + pos, to_copy);
	pos += to_copy;

	if (to_copy < p_length) {
		eofed = true;
	}

	return to_copy;
}

Error FileAccessEncrypted::get_error() const { return eofed ? ERR_FILE_EOF : OK; }

bool FileAccessEncrypted::store_buffer(const uint8_t* p_src, uint64_t p_length)
{
	ERR_FAIL_COND_V_MSG(!writing, false, "File has not been opened in write mode.");

	if (!p_length) {
		return true;
	}

	ERR_FAIL_NULL_V(p_src, false);

	if (pos + p_length >= get_length()) {
		ERR_FAIL_COND_V(data.resize(pos + p_length) != OK, false);
	}

	memcpy(data.ptrw() + pos, p_src, p_length);
	pos += p_length;

	return true;
}

void FileAccessEncrypted::flush()
{
	ERR_FAIL_COND_MSG(!writing, "File has not been opened in write mode.");
}

bool FileAccessEncrypted::file_exists(const String& p_name)
{
	Ref<FileAccess> fa = FileAccess::open(p_name, FileAccess::READ);
	if (fa.is_null()) {
		return false;
	}
	return true;
}

uint64_t FileAccessEncrypted::_get_modified_time(const String& p_file)
{
	if (file.is_valid()) {
		return file->get_modified_time(p_file);
	}
	else {
		return 0;
	}
}

uint64_t FileAccessEncrypted::_get_access_time(const String& p_file)
{
	if (file.is_valid()) {
		return file->get_access_time(p_file);
	}
	else {
		return 0;
	}
}

int64_t FileAccessEncrypted::_get_size(const String& p_file)
{
	if (file.is_valid()) {
		return file->get_size(p_file);
	}
	else {
		return -1;
	}
}

uint32_t FileAccessEncrypted::_get_unix_permissions(const String& p_file)
{
	if (file.is_valid()) {
		return file->_get_unix_permissions(p_file);
	}
	return 0;
}

Error FileAccessEncrypted::_set_unix_permissions(const String& p_file, uint32_t p_permissions)
{
	if (file.is_valid()) {
		return file->_set_unix_permissions(p_file, p_permissions);
	}
	return FAILED;
}

bool FileAccessEncrypted::_get_hidden_attribute(const String& p_file)
{
	if (file.is_valid()) {
		return file->_get_hidden_attribute(p_file);
	}
	return false;
}

Error FileAccessEncrypted::_set_hidden_attribute(const String& p_file, bool p_hidden)
{
	if (file.is_valid()) {
		return file->_set_hidden_attribute(p_file, p_hidden);
	}
	return FAILED;
}

bool FileAccessEncrypted::_get_read_only_attribute(const String& p_file)
{
	if (file.is_valid()) {
		return file->_get_read_only_attribute(p_file);
	}
	return false;
}

Error FileAccessEncrypted::_set_read_only_attribute(const String& p_file, bool p_ro)
{
	if (file.is_valid()) {
		return file->_set_read_only_attribute(p_file, p_ro);
	}
	return FAILED;
}

void FileAccessEncrypted::close() { _close(); }

FileAccessEncrypted::~FileAccessEncrypted() { _close(); }

// ============================================================================
// PackedData / PackSource / FileAccessPack / DirAccessPack
// ============================================================================

void PackedData::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	data->root = memnew(PackedDir);
	add_pack_source(memnew(PackedSourcePCK));
}

void PackedData::finalize()
{
	if (data == nullptr) {
		return;
	}

	for (int i = 0; i < data->sources.size(); i++) {
		memdelete(data->sources[i]);
	}
	if (data->root != nullptr) {
		_free_packed_dirs(data->root);
	}
	memdelete(data);
	data = nullptr;
}

Error PackedData::add_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
	const Vector<uint8_t>& p_decryption_key)
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);
	for (int i = 0; i < data->sources.size(); i++) {
		if (data->sources[i]->try_open_pack(p_path, p_replace_files, p_offset, p_decryption_key)) {
			return OK;
		}
	}

	return ERR_FILE_UNRECOGNIZED;
}

void PackedData::add_path(const String& p_pkg_path, const String& p_path, uint64_t p_ofs,
	uint64_t p_size, const uint8_t* p_md5, PackSource* p_src, bool p_replace_files,
	bool p_encrypted, bool p_bundle, bool p_delta, const String& p_salt)
{
	ERR_FAIL_NULL(data);
	String simplified_path = p_path.simplify_path().trim_prefix("res://");
	PathMD5 pmd5(simplified_path.md5_buffer());

	bool exists = data->files.has(pmd5);

	PackedFile pf;
	pf.encrypted = p_encrypted;
	pf.bundle = p_bundle;
	pf.delta = p_delta;
	pf.pack = p_pkg_path;
	pf.salt = p_salt;
	pf.offset = p_ofs;
	pf.size = p_size;
	for (int i = 0; i < 16; i++) {
		pf.md5[i] = p_md5[i];
	}
	pf.src = p_src;

	if (p_delta) {
		data->delta_patches[pmd5].push_back(pf);
	}
	else if (!exists || p_replace_files) {
		data->files[pmd5] = pf;
		data->delta_patches[pmd5].clear();
	}

	if (!exists) {
		// Search for directory.
		PackedDir* cd = data->root;

		if (simplified_path.contains_char('/')) { // In a subdirectory.
			Vector<String> ds = simplified_path.get_base_dir().split("/");

			for (int j = 0; j < ds.size(); j++) {
				if (!cd->subdirs.has(ds[j])) {
					PackedDir* pd = memnew(PackedDir);
					pd->name = ds[j];
					pd->parent = cd;
					cd->subdirs[pd->name] = pd;
					cd = pd;
				}
				else {
					cd = cd->subdirs[ds[j]];
				}
			}
		}
		String filename = simplified_path.get_file();
		// Don't add as a file if the path points to a directory.
		if (!filename.is_empty()) {
			cd->files.insert(filename);
		}
	}
}

void PackedData::remove_path(const String& p_path)
{
	ERR_FAIL_NULL(data);
	String simplified_path = p_path.simplify_path().trim_prefix("res://");
	PathMD5 pmd5(simplified_path.md5_buffer());
	if (!data->files.has(pmd5)) {
		return;
	}

	// Search for directory.
	PackedDir* cd = data->root;

	if (simplified_path.contains_char('/')) { // In a subdirectory.
		Vector<String> ds = simplified_path.get_base_dir().split("/");

		for (int j = 0; j < ds.size(); j++) {
			if (!cd->subdirs.has(ds[j])) {
				return; // Subdirectory does not exist, do not bother creating.
			}
			else {
				cd = cd->subdirs[ds[j]];
			}
		}
	}

	cd->files.erase(simplified_path.get_file());

	data->files.erase(pmd5);
}

void PackedData::add_pack_source(PackSource* p_source)
{
	ERR_FAIL_NULL(data);
	if (p_source != nullptr) {
		data->sources.push_back(p_source);
	}
}

uint8_t* PackedData::get_file_hash(const String& p_path)
{
	ERR_FAIL_NULL_V(data, nullptr);
	String simplified_path = p_path.simplify_path().trim_prefix("res://");
	PathMD5 pmd5(simplified_path.md5_buffer());
	HashMap<PathMD5, PackedFile, PathMD5>::Iterator E = data->files.find(pmd5);
	if (!E) {
		return nullptr;
	}

	return E->value.md5;
}

Vector<PackedData::PackedFile> PackedData::get_delta_patches(const String& p_path)
{
	ERR_FAIL_NULL_V(data, Vector<PackedFile>());
	String simplified_path = p_path.simplify_path().trim_prefix("res://");
	PathMD5 pmd5(simplified_path.md5_buffer());
	HashMap<PathMD5, Vector<PackedFile>, PathMD5>::ConstIterator E =
		data->delta_patches.find(pmd5);
	if (!E) {
		return Vector<PackedFile>();
	}

	return E->value;
}

bool PackedData::has_delta_patches(const String& p_path)
{
	if (data == nullptr) {
		return false;
	}
	String simplified_path = p_path.simplify_path().trim_prefix("res://");
	PathMD5 pmd5(simplified_path.md5_buffer());
	HashMap<PathMD5, Vector<PackedFile>, PathMD5>::ConstIterator E =
		data->delta_patches.find(pmd5);
	if (!E) {
		return false;
	}

	return !E->value.is_empty();
}

HashSet<String> PackedData::get_file_paths()
{
	HashSet<String> file_paths;
	ERR_FAIL_NULL_V(data, file_paths);
	_get_file_paths(data->root, data->root->name, file_paths);
	return file_paths;
}

void PackedData::_get_file_paths(
	PackedDir* p_dir, const String& p_parent_dir, HashSet<String>& r_paths)
{
	for (const String& E : p_dir->files) {
		r_paths.insert(p_parent_dir.path_join(E));
	}

	for (const KeyValue<String, PackedDir*>& E : p_dir->subdirs) {
		_get_file_paths(E.value, p_parent_dir.path_join(E.key), r_paths);
	}
}

void PackedData::clear()
{
	ERR_FAIL_NULL(data);
	data->files.clear();
	data->delta_patches.clear();
	_free_packed_dirs(data->root);
	data->root = memnew(PackedDir);
}

void PackedData::_free_packed_dirs(PackedDir* p_dir)
{
	for (const KeyValue<String, PackedDir*>& E : p_dir->subdirs) {
		_free_packed_dirs(E.value);
	}
	memdelete(p_dir);
}

bool PackedSourcePCK::try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
	const Vector<uint8_t>& p_decryption_key)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return false;
	}

	bool pck_header_found = false;

	// Search for the header at the start offset - standalone PCK file.
	f->seek(p_offset);
	uint32_t magic = f->get_32();
	if (magic == PACK_HEADER_MAGIC) {
		pck_header_found = true;
	}

	// Search for the header in the executable "pck" section - self contained executable.
	if (!pck_header_found) {
		// Loading with offset feature not supported for self contained exe files.
		if (p_offset != 0) {
			ERR_FAIL_V_MSG(false, "Loading self-contained executable with offset not supported.");
		}

		int64_t pck_off = OS::get_embedded_pck_offset();
		if (pck_off != 0) {
			// Search for the header, in case PCK start and section have different alignment.
			for (int i = 0; i < 8; i++) {
				f->seek(pck_off);
				magic = f->get_32();
				if (magic == PACK_HEADER_MAGIC) {
#ifdef DEBUG_ENABLED
					print_verbose(
						"PCK header found in executable pck section, loading from offset 0x" +
						String::num_int64(pck_off - 4, 16));
#endif
					pck_header_found = true;
					break;
				}
				pck_off++;
			}
		}
	}

	// Search for the header at the end of file - self contained executable.
	if (!pck_header_found) {
		// Loading with offset feature not supported for self contained exe files.
		if (p_offset != 0) {
			ERR_FAIL_V_MSG(false, "Loading self-contained executable with offset not supported.");
		}

		f->seek_end();
		f->seek(f->get_position() - 4);
		magic = f->get_32();

		if (magic == PACK_HEADER_MAGIC) {
			f->seek(f->get_position() - 12);
			uint64_t ds = f->get_64();
			f->seek(f->get_position() - ds - 8);
			magic = f->get_32();
			if (magic == PACK_HEADER_MAGIC) {
#ifdef DEBUG_ENABLED
				print_verbose("PCK header found at the end of executable, loading from offset 0x" +
							  String::num_int64(f->get_position() - 4, 16));
#endif
				pck_header_found = true;
			}
		}
	}

	if (!pck_header_found) {
		return false;
	}

	int64_t pck_start_pos = f->get_position() - 4;

	// Read header.
	uint32_t version = f->get_32();
	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	uint32_t ver_patch = f->get_32(); // Not used for validation.

	ERR_FAIL_COND_V_MSG(version != PACK_FORMAT_VERSION_V4 && version != PACK_FORMAT_VERSION_V3 &&
							version != PACK_FORMAT_VERSION_V2,
		false, vformat("Pack version unsupported: %d.", version));
	ERR_FAIL_COND_V_MSG(ver_major > VLTR_VERSION_MAJOR ||
							(ver_major == VLTR_VERSION_MAJOR && ver_minor > VLTR_VERSION_MINOR),
		false,
		vformat("Pack created with a newer version of the engine: %d.%d.%d.", ver_major, ver_minor,
			ver_patch));

	uint32_t pack_flags = f->get_32();
	bool enc_directory = (pack_flags & PACK_DIR_ENCRYPTED);
	bool rel_filebase = (pack_flags & PACK_REL_FILEBASE); // Note: Always enabled for V3.
	bool sparse_bundle = (pack_flags & PACK_SPARSE_BUNDLE);
	String salt;

	uint64_t file_base = f->get_64();
	if ((version == PACK_FORMAT_VERSION_V4) || (version == PACK_FORMAT_VERSION_V3) ||
		(version == PACK_FORMAT_VERSION_V2 && rel_filebase)) {
		file_base += pck_start_pos;
	}

	if (version == PACK_FORMAT_VERSION_V3 || version == PACK_FORMAT_VERSION_V4) {
		// V3/V4: Read directory offset and skip reserved part of the header.
		uint64_t dir_offset = f->get_64() + pck_start_pos;
		if (sparse_bundle && enc_directory && version == PACK_FORMAT_VERSION_V4) {
			// V4: Read encrypted directory salt.
			Vector<uint8_t> salt_data = f->get_buffer(32);
			salt.append_latin1(Span((const char*)salt_data.ptr(), salt_data.size()));
		}
		f->seek(dir_offset);
	}
	else if (version == PACK_FORMAT_VERSION_V2) {
		// V2: Directory directly after the header.
		for (int i = 0; i < 16; i++) {
			f->get_32(); // Reserved.
		}
	}

	// Read directory.
	int file_count = f->get_32();
	if (enc_directory) {
		Ref<FileAccessEncrypted> fae;
		fae.instantiate();
		ERR_FAIL_COND_V_MSG(fae.is_null(), false, "Can't open encrypted pack directory.");

		Vector<uint8_t> key;
#ifdef TOOLS_ENABLED
		if (!p_decryption_key.is_empty()) {
			ERR_FAIL_COND_V_MSG(
				p_decryption_key.size() != 32, false, "Decryption key must be 256-bit.");
		}
		else
#endif
		{
			key.resize(32);
		}

		Error err = fae->open_and_parse(f, key, FileAccessEncrypted::MODE_READ, false);
		ERR_FAIL_COND_V_MSG(err, false, "Can't open encrypted pack directory.");
		f = fae;
	}

	for (int i = 0; i < file_count; i++) {
		uint32_t sl = f->get_32();
		CharString cs;
		cs.resize_uninitialized(sl + 1);
		f->get_buffer((uint8_t*)cs.ptr(), sl);
		cs[sl] = 0;

		String path = String::utf8(cs.ptr(), sl);
		uint64_t ofs = f->get_64();
		uint64_t size = f->get_64();
		uint8_t md5[16];
		f->get_buffer(md5, 16);
		uint32_t flags = f->get_32();

		if (flags & PACK_FILE_REMOVAL) { // The file was removed.
			PackedData::remove_path(path);
		}
		else {
			PackedData::add_path(p_path, path, file_base + ofs, size, md5, this, p_replace_files,
				(flags & PACK_FILE_ENCRYPTED), sparse_bundle, (flags & PACK_FILE_DELTA), salt);
		}
	}

	return true;
}

Ref<FileAccess> PackedSourcePCK::get_file(
	const String& p_path, PackedData::PackedFile* p_file, const Vector<uint8_t>& p_decryption_key)
{
	Ref<FileAccess> file(memnew(FileAccessPack(p_path, *p_file, p_decryption_key)));

	if (PackedData::has_delta_patches(p_path)) {
		Ref<FileAccessPatched> file_patched;
		file_patched.instantiate();
		Error err = file_patched->open_custom(file);
		ERR_FAIL_COND_V(err != OK, Ref<FileAccess>());
		file = file_patched;
	}

	return file;
}

bool PackedSourceDirectory::try_open_pack(const String& p_path, bool p_replace_files,
	uint64_t p_offset, const Vector<uint8_t>& p_decryption_key)
{
	// Load with offset feature only supported for PCK files.
	ERR_FAIL_COND_V_MSG(p_offset != 0, false,
		"Invalid PCK data. Note that loading files with a non-zero offset isn't supported with "
		"directories.");

	if (p_path != "res://") {
		return false;
	}
	add_directory(p_path, p_replace_files);
	return true;
}

Ref<FileAccess> PackedSourceDirectory::get_file(
	const String& p_path, PackedData::PackedFile* p_file, const Vector<uint8_t>& p_decryption_key)
{
	Ref<FileAccess> ret = FileAccess::create_for_path(p_path);
	ret->reopen(p_path, FileAccess::READ);
	return ret;
}

void PackedSourceDirectory::add_directory(const String& p_path, bool p_replace_files)
{
	Ref<DirAccess> da = DirAccess::open(p_path);
	if (da.is_null()) {
		return;
	}
	da->set_include_hidden(true);

	for (const String& file_name : da->get_files()) {
		String file_path = p_path.path_join(file_name);
		uint8_t md5[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		PackedData::add_path(
			p_path, file_path, 0, 0, md5, this, p_replace_files, false, false, false);
	}

	for (const String& sub_dir_name : da->get_directories()) {
		String sub_dir_path = p_path.path_join(sub_dir_name);
		add_directory(sub_dir_path, p_replace_files);
	}
}

Error FileAccessPack::open_internal(const String& p_path, int p_mode_flags)
{
	ERR_PRINT("Can't open pack-referenced file.");
	return ERR_UNAVAILABLE;
}

bool FileAccessPack::is_open() const
{
	if (f.is_valid()) {
		return f->is_open();
	}
	else {
		return false;
	}
}

void FileAccessPack::seek(uint64_t p_position)
{
	ERR_FAIL_COND_MSG(f.is_null(), "File must be opened before use.");

	if (p_position > pf.size) {
		eof = true;
	}
	else {
		eof = false;
	}

	f->seek(off + p_position);
	pos = p_position;
}

void FileAccessPack::seek_end(int64_t p_position) { seek(pf.size + p_position); }

uint64_t FileAccessPack::get_position() const { return pos; }

uint64_t FileAccessPack::get_length() const { return pf.size; }

bool FileAccessPack::eof_reached() const { return eof; }

uint64_t FileAccessPack::get_buffer(uint8_t* p_dst, uint64_t p_length) const
{
	ERR_FAIL_COND_V_MSG(f.is_null(), -1, "File must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	if (eof) {
		return 0;
	}

	int64_t to_read = p_length;
	if (to_read + pos > pf.size) {
		eof = true;
		to_read = (int64_t)pf.size - (int64_t)pos;
	}

	pos += to_read;

	if (to_read <= 0) {
		return 0;
	}
	f->get_buffer(p_dst, to_read);

	return to_read;
}

void FileAccessPack::set_big_endian(bool p_big_endian)
{
	ERR_FAIL_COND_MSG(f.is_null(), "File must be opened before use.");

	FileAccess::set_big_endian(p_big_endian);
	f->set_big_endian(p_big_endian);
}

Error FileAccessPack::get_error() const
{
	if (eof) {
		return ERR_FILE_EOF;
	}
	return OK;
}

void FileAccessPack::flush() { ERR_FAIL(); }

bool FileAccessPack::store_buffer(const uint8_t* p_src, uint64_t p_length) { ERR_FAIL_V(false); }

bool FileAccessPack::file_exists(const String& p_name) { return false; }

void FileAccessPack::close() { f = Ref<FileAccess>(); }

FileAccessPack::FileAccessPack(const String& p_path, const PackedData::PackedFile& p_file,
	const Vector<uint8_t>& p_decryption_key)
{
	path = p_path;
	pf = p_file;
	if (pf.bundle) {
		String simplified_path = p_path.simplify_path();
		String path_to_load = simplified_path;
#ifdef TOOLS_ENABLED
		if (!pf.salt.is_empty()) {
			path_to_load =
				pf.pack.get_base_dir().path_join((simplified_path + pf.salt).sha256_text());
		}
		else {
			path_to_load = pf.pack.get_base_dir().path_join(simplified_path.replace("res://", ""));
		}
#else
		if (!pf.salt.is_empty()) {
			path_to_load = "res://" + (simplified_path + pf.salt).sha256_text();
		}
#endif
		Error err = OK;
		f = FileAccess::open(path_to_load, FileAccess::READ | FileAccess::SKIP_PACK, &err);
		ERR_FAIL_COND_MSG(err != OK,
			vformat(
				R"(Can't open pack-referenced file "%s" from sparse pack "%s" due to error "%s".)",
				simplified_path, pf.pack, error_names[err]));
		off = 0; // For the sparse pack offset is always zero.
	}
	else {
		Error err = OK;
		f = FileAccess::open(pf.pack, FileAccess::READ, &err);
		ERR_FAIL_COND_MSG(err != OK,
			vformat(R"(Can't open pack-referenced file "%s" from pack "%s" due to error "%s".)",
				p_path, pf.pack, error_names[err]));
		f->seek(pf.offset);
		off = pf.offset;
	}

	if (pf.encrypted) {
		Ref<FileAccessEncrypted> fae;
		fae.instantiate();
		ERR_FAIL_COND_MSG(fae.is_null(),
			vformat(R"(Can't open encrypted pack-referenced file "%s" from pack "%s".)", p_path,
				pf.pack));

		Vector<uint8_t> key;
		key.resize(32);

		Error err = fae->open_and_parse(f, key, FileAccessEncrypted::MODE_READ, false);
		ERR_FAIL_COND_MSG(err != OK,
			vformat(
				R"(Can't open encrypted pack-referenced file "%s" from pack "%s" due to error "%s".)",
				p_path, pf.pack, error_names[err]));
		f = fae;
		off = 0;
	}
	pos = 0;
	eof = false;
}

Error DirAccessPack::list_dir_begin()
{
	list_dirs.clear();
	list_files.clear();

	for (const KeyValue<String, PackedData::PackedDir*>& E : current->subdirs) {
		list_dirs.push_back(E.key);
	}

	for (const String& E : current->files) {
		list_files.push_back(E);
	}

	return OK;
}

String DirAccessPack::get_next()
{
	if (list_dirs.size()) {
		cdir = true;
		String d = list_dirs.front()->get();
		list_dirs.pop_front();
		return d;
	}
	else if (list_files.size()) {
		cdir = false;
		String f = list_files.front()->get();
		list_files.pop_front();
		return f;
	}
	else {
		return String();
	}
}

bool DirAccessPack::current_is_dir() const { return cdir; }

bool DirAccessPack::current_is_hidden() const { return false; }

void DirAccessPack::list_dir_end()
{
	list_dirs.clear();
	list_files.clear();
}

int DirAccessPack::get_drive_count() { return 0; }

String DirAccessPack::get_drive(int p_drive) { return ""; }

PackedData::PackedDir* DirAccessPack::_find_dir(const String& p_dir)
{
	String nd = p_dir.replace_char('\\', '/');

	// Special handling since simplify_path() will forbid it
	if (p_dir == "..") {
		return current->parent;
	}

	bool absolute = false;
	if (nd.begins_with("res://")) {
		nd = nd.replace_first("res://", "");
		absolute = true;
	}

	nd = nd.simplify_path();

	if (nd.is_empty()) {
		nd = ".";
	}

	if (nd.begins_with("/")) {
		nd = nd.replace_first("/", "");
		absolute = true;
	}

	Vector<String> paths = nd.split("/");

	PackedData::PackedDir* pd;

	if (absolute) {
		pd = PackedData::data ? PackedData::data->root : nullptr;
	}
	else {
		pd = current;
	}

	if (!pd) {
		return nullptr;
	}

	for (int i = 0; i < paths.size(); i++) {
		const String& p = paths[i];
		if (p == ".") {
			continue;
		}
		else if (p == "..") {
			if (pd->parent) {
				pd = pd->parent;
			}
		}
		else if (pd->subdirs.has(p)) {
			pd = pd->subdirs[p];
		}
		else {
			return nullptr;
		}
	}

	return pd;
}

Error DirAccessPack::change_dir(String p_dir)
{
	PackedData::PackedDir* pd = _find_dir(p_dir);
	if (pd) {
		current = pd;
		return OK;
	}
	else {
		return ERR_INVALID_PARAMETER;
	}
}

String DirAccessPack::get_current_dir(bool p_include_drive) const
{
	PackedData::PackedDir* pd = current;
	String p = current->name;

	while (pd->parent) {
		pd = pd->parent;
		p = pd->name.path_join(p);
	}

	return "res://" + p;
}

bool DirAccessPack::file_exists(String p_file)
{
	PackedData::PackedDir* pd = _find_dir(p_file.get_base_dir());
	if (!pd) {
		return false;
	}
	return pd->files.has(p_file.get_file());
}

bool DirAccessPack::dir_exists(String p_dir) { return _find_dir(p_dir) != nullptr; }

Error DirAccessPack::make_dir(String p_dir) { return ERR_UNAVAILABLE; }

Error DirAccessPack::rename(String p_from, String p_to) { return ERR_UNAVAILABLE; }

Error DirAccessPack::remove(String p_name) { return ERR_UNAVAILABLE; }

uint64_t DirAccessPack::get_space_left() { return 0; }

String DirAccessPack::get_filesystem_type() const { return "PCK"; }

DirAccessPack::DirAccessPack()
{
	current = PackedData::data ? PackedData::data->root : nullptr;
}

// ============================================================================
// ZipArchive / PackedSourceZIP / FileAccessZip
// ============================================================================

#ifdef MINIZIP_ENABLED

extern "C" {

struct ZipData
{
	Ref<FileAccess> f;
};

static void* godot_open(voidpf p_opaque, const char* p_fname, int p_mode)
{
	if (p_mode & ZLIB_FILEFUNC_MODE_WRITE) {
		return nullptr;
	}

	Ref<FileAccess> f = FileAccess::open(String::utf8(p_fname), FileAccess::READ);
	ERR_FAIL_COND_V(f.is_null(), nullptr);

	ZipData* zd = memnew(ZipData);
	zd->f = f;
	return zd;
}

static uLong godot_read(voidpf p_opaque, voidpf p_stream, void* r_buf, uLong p_size)
{
	ZipData* zd = (ZipData*)p_stream;
	zd->f->get_buffer((uint8_t*)r_buf, p_size);
	return p_size;
}

static uLong godot_write(voidpf p_opaque, voidpf p_stream, const void* p_buf, uLong p_size)
{
	return 0;
}

static long godot_tell(voidpf p_opaque, voidpf p_stream)
{
	ZipData* zd = (ZipData*)p_stream;
	return zd->f->get_position();
}

static long godot_seek(voidpf p_opaque, voidpf p_stream, uLong p_offset, int p_origin)
{
	ZipData* zd = (ZipData*)p_stream;

	uint64_t pos = p_offset;
	switch (p_origin) {
	case ZLIB_FILEFUNC_SEEK_CUR:
		pos = zd->f->get_position() + p_offset;
		break;
	case ZLIB_FILEFUNC_SEEK_END:
		pos = zd->f->get_length() + p_offset;
		break;
	default:
		break;
	}

	zd->f->seek(pos);
	return 0;
}

static int godot_close(voidpf p_opaque, voidpf p_stream)
{
	ZipData* zd = (ZipData*)p_stream;
	memdelete(zd);
	return 0;
}

static int godot_testerror(voidpf p_opaque, voidpf p_stream)
{
	ZipData* zd = (ZipData*)p_stream;
	return zd->f->get_error() != OK ? 1 : 0;
}

static voidpf godot_alloc(voidpf p_opaque, uInt p_items, uInt p_size)
{
	return memalloc((size_t)p_items * p_size);
}

static void godot_free(voidpf p_opaque, voidpf r_address) { memfree(r_address); }
} // extern "C"

void ZipArchive::initialize()
{
	if (data == nullptr) {
		data = memnew(Data);
	}
}

void ZipArchive::finalize()
{
	if (data != nullptr) {
		data->packages.clear();
		memdelete(data);
		data = nullptr;
	}
}

void ZipArchive::close_handle(unzFile p_file)
{
	ERR_FAIL_NULL_MSG(p_file, "Cannot close a file if none is open.");
	unzCloseCurrentFile(p_file);
	unzClose(p_file);
}

unzFile ZipArchive::get_file_handle(const String& p_file)
{
	ERR_FAIL_NULL_V(data, nullptr);
	ERR_FAIL_COND_V_MSG(!file_exists(p_file), nullptr, vformat("File '%s' doesn't exist.", p_file));
	File file = data->files[p_file];

	zlib_filefunc_def io;
	memset(&io, 0, sizeof(io));

	io.opaque = nullptr;
	io.zopen_file = godot_open;
	io.zread_file = godot_read;
	io.zwrite_file = godot_write;

	io.ztell_file = godot_tell;
	io.zseek_file = godot_seek;
	io.zclose_file = godot_close;
	io.zerror_file = godot_testerror;

	io.alloc_mem = godot_alloc;
	io.free_mem = godot_free;

	unzFile pkg = unzOpen2(data->packages[file.package].filename.utf8().get_data(), &io);
	ERR_FAIL_NULL_V_MSG(
		pkg, nullptr, vformat("Cannot open file '%s'.", data->packages[file.package].filename));
	int unz_err = unzGoToFilePos(pkg, &file.file_pos);
	if (unz_err != UNZ_OK || unzOpenCurrentFile(pkg) != UNZ_OK) {
		unzClose(pkg);
		ERR_FAIL_V(nullptr);
	}

	return pkg;
}

bool ZipArchive::try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
	PackSource* p_source, const Vector<uint8_t>& p_decryption_key)
{
	(void)p_decryption_key;
	initialize();
	ERR_FAIL_NULL_V(data, false);

	// load with offset feature only supported for PCK files
	ERR_FAIL_COND_V_MSG(p_offset != 0, false,
		"Invalid PCK data. Note that loading files with a non-zero offset isn't supported with ZIP "
		"archives.");

	if (p_path.get_extension().nocasecmp_to("zip") != 0 &&
		p_path.get_extension().nocasecmp_to("pcz") != 0) {
		return false;
	}

	zlib_filefunc_def io;
	memset(&io, 0, sizeof(io));

	io.opaque = nullptr;
	io.zopen_file = godot_open;
	io.zread_file = godot_read;
	io.zwrite_file = godot_write;

	io.ztell_file = godot_tell;
	io.zseek_file = godot_seek;
	io.zclose_file = godot_close;
	io.zerror_file = godot_testerror;

	unzFile zfile = unzOpen2(p_path.utf8().get_data(), &io);
	ERR_FAIL_NULL_V(zfile, false);

	unz_global_info64 gi;
	int err = unzGetGlobalInfo64(zfile, &gi);
	ERR_FAIL_COND_V(err != UNZ_OK, false);

	Package pkg;
	pkg.filename = p_path;
	data->packages.push_back(pkg);
	int pkg_num = data->packages.size() - 1;

	for (uint64_t i = 0; i < gi.number_entry; i++) {
		char filename_inzip[256];

		unz_file_info64 file_info;
		err = unzGetCurrentFileInfo64(
			zfile, &file_info, filename_inzip, sizeof(filename_inzip), nullptr, 0, nullptr, 0);
		ERR_CONTINUE(err != UNZ_OK);

		File f;
		f.package = pkg_num;
		unzGetFilePos(zfile, &f.file_pos);

		String fname = String("res://") + String::utf8(filename_inzip);
		data->files[fname] = f;

		uint8_t md5[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		PackedData::add_path(p_path, fname, 1, 0, md5, p_source, p_replace_files, false);

		if ((i + 1) < gi.number_entry) {
			unzGoToNextFile(zfile);
		}
	}

	unzClose(zfile);

	return true;
}

bool ZipArchive::file_exists(const String& p_name)
{
	return data != nullptr && data->files.has(p_name);
}

Ref<FileAccess> ZipArchive::get_file(
	const String& p_path, PackedData::PackedFile* p_file, const Vector<uint8_t>& p_decryption_key)
{
	(void)p_decryption_key;
	return memnew(FileAccessZip(p_path, *p_file));
}

bool PackedSourceZIP::try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
	const Vector<uint8_t>& p_decryption_key)
{
	return ZipArchive::try_open_pack(p_path, p_replace_files, p_offset, this, p_decryption_key);
}

Ref<FileAccess> PackedSourceZIP::get_file(
	const String& p_path, PackedData::PackedFile* p_file, const Vector<uint8_t>& p_decryption_key)
{
	return ZipArchive::get_file(p_path, p_file, p_decryption_key);
}

Error FileAccessZip::open_internal(const String& p_path, int p_mode_flags)
{
	_close();

	ERR_FAIL_COND_V(p_mode_flags & FileAccess::WRITE, FAILED);
	ERR_FAIL_COND_V(!ZipArchive::is_initialized(), FAILED);
	zfile = ZipArchive::get_file_handle(p_path);
	ERR_FAIL_NULL_V(zfile, FAILED);

	int err = unzGetCurrentFileInfo64(zfile, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
	ERR_FAIL_COND_V(err != UNZ_OK, FAILED);

	return OK;
}

void FileAccessZip::_close()
{
	if (!zfile) {
		return;
	}

	ZipArchive::close_handle(zfile);
	zfile = nullptr;
}

bool FileAccessZip::is_open() const { return zfile != nullptr; }

void FileAccessZip::seek(uint64_t p_position)
{
	ERR_FAIL_NULL(zfile);
	unzSeekCurrentFile(zfile, p_position);
}

void FileAccessZip::seek_end(int64_t p_position)
{
	ERR_FAIL_NULL(zfile);
	unzSeekCurrentFile(zfile, get_length() + p_position);
}

uint64_t FileAccessZip::get_position() const
{
	ERR_FAIL_NULL_V(zfile, 0);
	return unztell64(zfile);
}

uint64_t FileAccessZip::get_length() const
{
	ERR_FAIL_NULL_V(zfile, 0);
	return file_info.uncompressed_size;
}

bool FileAccessZip::eof_reached() const
{
	ERR_FAIL_NULL_V(zfile, true);
	return at_eof;
}

uint64_t FileAccessZip::get_buffer(uint8_t* p_dst, uint64_t p_length) const
{
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);
	ERR_FAIL_NULL_V(zfile, -1);

	at_eof = unzeof(zfile);
	if (at_eof) {
		return 0;
	}
	int64_t read = unzReadCurrentFile(zfile, p_dst, p_length);
	ERR_FAIL_COND_V(read < 0, read);
	if ((uint64_t)read < p_length) {
		at_eof = true;
	}
	return read;
}

Error FileAccessZip::get_error() const
{
	if (!zfile) {
		return ERR_UNCONFIGURED;
	}
	if (eof_reached()) {
		return ERR_FILE_EOF;
	}

	return OK;
}

void FileAccessZip::flush() { ERR_FAIL(); }

bool FileAccessZip::store_buffer(const uint8_t* p_src, uint64_t p_length) { ERR_FAIL_V(false); }

bool FileAccessZip::file_exists(const String& p_name) { return false; }

void FileAccessZip::close() { _close(); }

FileAccessZip::FileAccessZip(const String& p_path, const PackedData::PackedFile& p_file)
{
	open_internal(p_path, FileAccess::READ);
}

FileAccessZip::~FileAccessZip() { _close(); }

#endif // MINIZIP_ENABLED
