/**************************************************************************/
/*  file_access.cpp                                                       */
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

#include "core/config/project_settings.h"
#include "core/crypto/crypto.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "file_access.h"

#ifndef DISABLE_DEPRECATED

Ref<FileAccess> FileAccess::_open_encrypted_bind_compat_98918(
	const String& p_path, ModeFlags p_mode_flags, const Vector<uint8_t>& p_key)
{
	return open_encrypted(p_path, p_mode_flags, p_key, Vector<uint8_t>());
}

Ref<FileAccess> FileAccess::_create_temp_bind_compat_114053(
	int p_mode_flags, const String& p_prefix, const String& p_extension, bool p_keep)
{
	return _create_temp((ModeFlags)p_mode_flags, p_prefix, p_extension, p_keep);
}

void FileAccess::store_8_bind_compat_78289(uint8_t p_dest) { store_8(p_dest); }

void FileAccess::store_16_bind_compat_78289(uint16_t p_dest) { store_16(p_dest); }

void FileAccess::store_32_bind_compat_78289(uint32_t p_dest) { store_32(p_dest); }

void FileAccess::store_64_bind_compat_78289(uint64_t p_dest) { store_64(p_dest); }

void FileAccess::store_buffer_bind_compat_78289(const Vector<uint8_t>& p_buffer)
{
	store_buffer(p_buffer);
}

void FileAccess::store_half_bind_compat_78289(float p_dest) { store_half(p_dest); }

void FileAccess::store_float_bind_compat_78289(float p_dest) { store_float(p_dest); }

void FileAccess::store_double_bind_compat_78289(double p_dest) { store_double(p_dest); }

void FileAccess::store_real_bind_compat_78289(real_t p_real) { store_real(p_real); }

void FileAccess::store_string_bind_compat_78289(const String& p_string) { store_string(p_string); }

void FileAccess::store_line_bind_compat_78289(const String& p_line) { store_line(p_line); }

void FileAccess::store_csv_line_bind_compat_78289(
	const Vector<String>& p_values, const String& p_delim)
{
	store_csv_line(p_values, p_delim);
}

void FileAccess::store_pascal_string_bind_compat_78289(const String& p_string)
{
	store_pascal_string(p_string);
}

String FileAccess::get_as_text_bind_compat_110867(bool p_skip_cr) const
{
	String text = get_as_text();
	if (unlikely(p_skip_cr)) {
		text = text.remove_char('\r');
	}
	return text;
}

void FileAccess::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

Ref<FileAccess> FileAccess::create(AccessType p_access)
{
	ERR_FAIL_INDEX_V(p_access, ACCESS_MAX, nullptr);
	ERR_FAIL_NULL_V(create_func[p_access], nullptr);

	Ref<FileAccess> ret = create_func[p_access]();
	ret->_set_access_type(p_access);
	return ret;
}

bool FileAccess::exists(const String& p_name)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		PackedData::has_path(p_name)) {
		return true;
	}

	// Using file_exists because it's faster than trying to open the file.
	Ref<FileAccess> ret = create_for_path(p_name);
	return ret->file_exists(p_name);
}

void FileAccess::_set_access_type(AccessType p_access) { _access_type = p_access; }

Ref<FileAccess> FileAccess::create_for_path(const String& p_path)
{
	Ref<FileAccess> ret;
	if (p_path.begins_with("res://") || p_path.begins_with("uid://")) {
		ret = create(ACCESS_RESOURCES);
	}
	else if (p_path.begins_with("user://")) {
		ret = create(ACCESS_USERDATA);
	}
	else if (p_path.begins_with("pipe://")) {
		ret = create(ACCESS_PIPE);
	}
	else {
		ret = create(ACCESS_FILESYSTEM);
	}

	return ret;
}

Ref<FileAccess> FileAccess::create_temp(ModeFlags p_mode_flags, const String& p_prefix,
	const String& p_extension, bool p_keep, Error* r_error)
{
	const String ERROR_COMMON_PREFIX = "Error while creating temporary file";

	if (!p_prefix.is_empty() && !p_prefix.is_valid_filename()) {
		*r_error = ERR_FILE_BAD_PATH;
		ERR_FAIL_V_MSG(Ref<FileAccess>(),
			vformat(R"(%s: "%s" is not a valid prefix.)", ERROR_COMMON_PREFIX, p_prefix));
	}

	if (!p_extension.is_empty() && !p_extension.is_valid_filename()) {
		*r_error = ERR_FILE_BAD_PATH;
		ERR_FAIL_V_MSG(Ref<FileAccess>(),
			vformat(R"(%s: "%s" is not a valid extension.)", ERROR_COMMON_PREFIX, p_extension));
	}

	const String TEMP_DIR = OS::get_temp_path();
	String extension = p_extension.trim_prefix(".");

	uint32_t suffix_i = 0;
	String path;
	while (true) {
		String datetime = Time::get_datetime_string_from_system().remove_chars("-T:");
		datetime += itos(Time::get_ticks_usec());
		String suffix = datetime + (suffix_i > 0 ? itos(suffix_i) : "");
		path = TEMP_DIR.path_join((p_prefix.is_empty() ? "" : p_prefix + "-") + suffix +
								  (extension.is_empty() ? "" : "." + extension));
		if (!DirAccess::exists(path)) {
			break;
		}
		suffix_i += 1;
	}

	Error err;
	{
		// Create file first with WRITE mode.
		// Otherwise, it would fail to open with a READ mode.
		Ref<FileAccess> ret = FileAccess::open(path, FileAccess::ModeFlags::WRITE, &err);
		if (err != OK) {
			*r_error = err;
			ERR_FAIL_V_MSG(Ref<FileAccess>(),
				vformat(R"(%s: could not create "%s".)", ERROR_COMMON_PREFIX, path));
		}
		ret->flush();
	}

	// Open then the temp file with the correct mode flag.
	Ref<FileAccess> ret = FileAccess::open(path, p_mode_flags, &err);
	if (err != OK) {
		*r_error = err;
		ERR_FAIL_V_MSG(
			Ref<FileAccess>(), vformat(R"(%s: could not open "%s".)", ERROR_COMMON_PREFIX, path));
	}
	if (ret.is_valid()) {
		ret->_is_temp_file = true;
		ret->_temp_keep_after_use = p_keep;
		ret->_temp_path = ret->get_path_absolute();
	}

	*r_error = OK;
	return ret;
}

Ref<FileAccess> FileAccess::_create_temp(
	ModeFlags p_mode_flags, const String& p_prefix, const String& p_extension, bool p_keep)
{
	return create_temp(p_mode_flags, p_prefix, p_extension, p_keep, &last_file_open_error);
}

void FileAccess::_delete_temp()
{
	if (!_is_temp_file || _temp_keep_after_use) {
		return;
	}

	if (!FileAccess::exists(_temp_path)) {
		return;
	}

	DirAccess::remove_absolute(_temp_path);
}

Error FileAccess::reopen(const String& p_path, int p_mode_flags)
{
	return open_internal(p_path, p_mode_flags);
}

Ref<FileAccess> FileAccess::open(const String& p_path, int p_mode_flags, Error* r_error)
{
	// try packed data first

	Ref<FileAccess> ret;
	if (!(p_mode_flags & WRITE) && !(p_mode_flags & SKIP_PACK) && PackedData::is_initialized() &&
		!PackedData::is_disabled()) {
		ret = PackedData::try_open_path(p_path);
		if (ret.is_valid()) {
			if (r_error) {
				*r_error = OK;
			}
			return ret;
		}
	}

	ret = create_for_path(p_path);
	Error err = ret->open_internal(p_path, p_mode_flags & ~SKIP_PACK);

	if (r_error) {
		*r_error = err;
	}
	if (err != OK) {
		ret.unref();
	}

	return ret;
}

Ref<FileAccess> FileAccess::_open(const String& p_path, ModeFlags p_mode_flags)
{
	Error err = OK;
	Ref<FileAccess> fa = open(p_path, p_mode_flags, &err);
	last_file_open_error = err;
	if (err) {
		return Ref<FileAccess>();
	}
	return fa;
}

Ref<FileAccess> FileAccess::open_encrypted(const String& p_path, ModeFlags p_mode_flags,
	const Vector<uint8_t>& p_key, const Vector<uint8_t>& p_iv)
{
	Ref<FileAccess> fa = _open(p_path, p_mode_flags);
	if (fa.is_null()) {
		return fa;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	Error err = fae->open_and_parse(fa, p_key,
		(p_mode_flags == WRITE) ? FileAccessEncrypted::MODE_WRITE_AES256
								: FileAccessEncrypted::MODE_READ,
		true, p_iv);
	last_file_open_error = err;
	if (err) {
		return Ref<FileAccess>();
	}
	return fae;
}

Ref<FileAccess> FileAccess::open_encrypted_pass(
	const String& p_path, ModeFlags p_mode_flags, const String& p_pass)
{
	Ref<FileAccess> fa = _open(p_path, p_mode_flags);
	if (fa.is_null()) {
		return fa;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	Error err = fae->open_and_parse_password(fa, p_pass,
		(p_mode_flags == WRITE) ? FileAccessEncrypted::MODE_WRITE_AES256
								: FileAccessEncrypted::MODE_READ);
	last_file_open_error = err;
	if (err) {
		return Ref<FileAccess>();
	}
	return fae;
}

Ref<FileAccess> FileAccess::open_compressed(
	const String& p_path, ModeFlags p_mode_flags, CompressionMode p_compress_mode)
{
	Ref<FileAccessCompressed> fac;
	fac.instantiate();
	fac->configure("GCPF", (Compression::Mode)p_compress_mode);
	Error err = fac->open_internal(p_path, p_mode_flags);
	last_file_open_error = err;
	if (err) {
		return Ref<FileAccess>();
	}

	return fac;
}

Error FileAccess::get_open_error() { return last_file_open_error; }

FileAccess::CreateFunc FileAccess::get_create_func(AccessType p_access)
{
	return create_func[p_access];
}

FileAccess::AccessType FileAccess::get_access_type() const { return _access_type; }

String FileAccess::fix_path(const String& p_path) const
{
	// Helper used by file accesses that use a single filesystem.

	String r_path = p_path.replace_char('\\', '/');

	switch (_access_type) {
	case ACCESS_RESOURCES: {
		if (ProjectSettings::is_initialized()) {
			if (r_path.begins_with("uid://")) {
				ResourceUID::ID uid = ResourceUID::text_to_id(r_path);
				if (ResourceUID::has_id(uid)) {
					r_path = ResourceUID::get_id_path(uid);
				}
				else {
					r_path.clear();
				}
			}

			if (r_path.begins_with("res://")) {
				String resource_path = ProjectSettings::get_resource_path();
				if (!resource_path.is_empty()) {
					return r_path.replace("res:/", resource_path);
				}
				return r_path.replace("res://", "");
			}
		}

	} break;
	case ACCESS_USERDATA: {
		if (r_path.begins_with("user://")) {
			String data_dir = OS::get_user_data_dir();
			if (!data_dir.is_empty()) {
				return r_path.replace("user:/", data_dir);
			}
			return r_path.replace("user://", "");
		}

	} break;
	case ACCESS_PIPE: {
		return r_path;
	} break;
	case ACCESS_FILESYSTEM: {
		return r_path;
	} break;
	case ACCESS_MAX:
		break; // Can't happen, but silences warning
	}

	return r_path;
}

/* these are all implemented for ease of porting, then can later be optimized */
uint8_t FileAccess::get_8() const
{
	uint8_t data = 0;
	get_buffer(&data, sizeof(uint8_t));

	return data;
}

uint16_t FileAccess::get_16() const
{
	uint16_t data = 0;
	get_buffer(reinterpret_cast<uint8_t*>(&data), sizeof(uint16_t));

	if (big_endian) {
		data = BSWAP16(data);
	}

	return data;
}

uint32_t FileAccess::get_32() const
{
	uint32_t data = 0;
	get_buffer(reinterpret_cast<uint8_t*>(&data), sizeof(uint32_t));

	if (big_endian) {
		data = BSWAP32(data);
	}

	return data;
}

uint64_t FileAccess::get_64() const
{
	uint64_t data = 0;
	get_buffer(reinterpret_cast<uint8_t*>(&data), sizeof(uint64_t));

	if (big_endian) {
		data = BSWAP64(data);
	}

	return data;
}

float FileAccess::get_half() const { return Math::half_to_float(get_16()); }

float FileAccess::get_float() const
{
	MarshallFloat m;
	m.i = get_32();
	return m.f;
}

real_t FileAccess::get_real() const
{
	if (real_is_double) {
		return get_double();
	}
	else {
		return get_float();
	}
}

double FileAccess::get_double() const
{
	MarshallDouble m;
	m.l = get_64();
	return m.d;
}

String FileAccess::get_token() const
{
	CharString token;

	uint8_t c = get_8();

	while (!eof_reached()) {
		if (c <= ' ') {
			if (token.length()) {
				break;
			}
		}
		else {
			token += char(c);
		}
		c = get_8();
	}

	return String::utf8(token.get_data());
}

class CharBuffer
{
	Vector<char> vector;
	char stack_buffer[256];

	char* buffer = nullptr;
	int64_t capacity = 0;
	int64_t written = 0;

	bool grow()
	{
		if (vector.resize(Math::next_power_of_2((uint64_t)1 + (uint64_t)written)) != OK) {
			return false;
		}

		if (buffer == stack_buffer) { // first chunk?

			for (int64_t i = 0; i < written; i++) {
				vector.write[i] = stack_buffer[i];
			}
		}

		buffer = vector.ptrw();
		capacity = vector.size();
		ERR_FAIL_COND_V(written >= capacity, false);

		return true;
	}

public:
	_FORCE_INLINE_ CharBuffer() : buffer(stack_buffer), capacity(std_size(stack_buffer)) {}

	_FORCE_INLINE_ void push_back(char p_char)
	{
		if (written >= capacity) {
			ERR_FAIL_COND(!grow());
		}

		buffer[written++] = p_char;
	}

	_FORCE_INLINE_ const char* get_data() const { return buffer; }
};

String FileAccess::get_line() const
{
	CharBuffer line;

	uint8_t c = get_8();

	while (!eof_reached()) {
		if (c == '\r' || c == '\n' || c == '\0' || get_error() != OK) {
			if (c == '\r') {
				// Check for Windows-style EOL.
				const uint64_t prev_pos = get_position() - 1;
				if (unlikely(get_8() != '\n')) {
					// HACK: We can't simply check the next value in a vacuum, so we risk triggering
					//  an EOL false-positive in the unlikely event that this `\r` was the final
					//  value of the file. Unilaterally work around by re-reading the *previous*
					//  byte (the starting `\r`) to ensure `get_error()` returns `OK`.
					const_cast<FileAccess*>(this)->seek(prev_pos);
					get_8();
				}
			}
			line.push_back(0);
			return String::utf8(line.get_data());
		}
		else {
			line.push_back(char(c));
		}

		c = get_8();
	}
	line.push_back(0);
	return String::utf8(line.get_data());
}

Vector<String> FileAccess::get_csv_line(const String& p_delim) const
{
	ERR_FAIL_COND_V_MSG(p_delim.length() != 1, Vector<String>(),
		"Only single character delimiters are supported to parse CSV lines.");
	ERR_FAIL_COND_V_MSG(p_delim[0] == '"', Vector<String>(),
		"The double quotation mark character (\") is not supported as a delimiter for CSV lines.");

	String line;

	// CSV can support entries with line breaks as long as they are enclosed
	// in double quotes. So our "line" might be more than a single line in the
	// text file.
	int qc = 0;
	do {
		if (eof_reached()) {
			break;
		}
		line += get_line() + "\n";
		qc = 0;
		for (int i = 0; i < line.length(); i++) {
			if (line[i] == '"') {
				qc++;
			}
		}
	} while (qc % 2);

	// Remove the extraneous newline we've added above.
	line = line.substr(0, line.length() - 1);

	Vector<String> strings;

	bool in_quote = false;
	String current;
	for (int i = 0; i < line.length(); i++) {
		char32_t c = line[i];
		// A delimiter ends the current entry, unless it's in a quoted string.
		if (!in_quote && c == p_delim[0]) {
			strings.push_back(current);
			current = String();
		}
		else if (c == '"') {
			// Doubled quotes are escapes for intentional quotes in the string.
			if (line[i + 1] == '"' && in_quote) {
				current += '"';
				i++;
			}
			else {
				in_quote = !in_quote;
			}
		}
		else {
			current += c;
		}
	}

	if (in_quote) {
		WARN_PRINT(
			vformat("Reached end of file before closing '\"' in CSV file '%s'.", get_path()));
	}

	strings.push_back(current);

	return strings;
}

String FileAccess::get_as_text() const
{
	uint64_t original_pos = get_position();
	const_cast<FileAccess*>(this)->seek(0);

	String text = get_as_utf8_string();

	const_cast<FileAccess*>(this)->seek(original_pos);

	return text;
}

Vector<uint8_t> FileAccess::get_buffer(int64_t p_length) const
{
	Vector<uint8_t> data;

	ERR_FAIL_COND_V_MSG(p_length < 0, data, "Length of buffer cannot be smaller than 0.");
	if (p_length == 0) {
		return data;
	}

	data.reserve_exact(p_length);
	Error err = data.resize(p_length);
	ERR_FAIL_COND_V_MSG(err != OK, data, vformat("Can't resize data to %d elements.", p_length));

	uint8_t* w = data.ptrw();
	int64_t len = get_buffer(w, p_length);

	if (len < p_length) {
		data.resize(len);
	}

	return data;
}

String FileAccess::get_as_utf8_string() const
{
	Vector<uint8_t> sourcef;
	uint64_t len = get_length();
	sourcef.resize(len + 1);

	uint8_t* w = sourcef.ptrw();
	uint64_t r = get_buffer(w, len);
	ERR_FAIL_COND_V(r != len, String());
	w[len] = 0;

	String s;
	s.append_utf8((const char*)w, len);
	return s;
}

bool FileAccess::store_8(uint8_t p_dest) { return store_buffer(&p_dest, sizeof(uint8_t)); }

bool FileAccess::store_16(uint16_t p_dest)
{
	if (big_endian) {
		p_dest = BSWAP16(p_dest);
	}

	return store_buffer(reinterpret_cast<uint8_t*>(&p_dest), sizeof(uint16_t));
}

bool FileAccess::store_32(uint32_t p_dest)
{
	if (big_endian) {
		p_dest = BSWAP32(p_dest);
	}

	return store_buffer(reinterpret_cast<uint8_t*>(&p_dest), sizeof(uint32_t));
}

bool FileAccess::store_64(uint64_t p_dest)
{
	if (big_endian) {
		p_dest = BSWAP64(p_dest);
	}

	return store_buffer(reinterpret_cast<uint8_t*>(&p_dest), sizeof(uint64_t));
}

bool FileAccess::store_real(real_t p_real)
{
	if constexpr (sizeof(real_t) == 4) {
		return store_float(p_real);
	}
	else {
		return store_double(p_real);
	}
}

bool FileAccess::store_half(float p_dest) { return store_16(Math::make_half_float(p_dest)); }

bool FileAccess::store_float(float p_dest)
{
	MarshallFloat m;
	m.f = p_dest;
	return store_32(m.i);
}

bool FileAccess::store_double(double p_dest)
{
	MarshallDouble m;
	m.d = p_dest;
	return store_64(m.l);
}

uint64_t FileAccess::get_modified_time(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return 0;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), 0, vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_get_modified_time(p_file);
}

uint64_t FileAccess::get_access_time(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return 0;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "Cannot create FileAccess for path '" + p_file + "'.");

	return fa->_get_access_time(p_file);
}

int64_t FileAccess::get_size(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return PackedData::get_size(p_file);
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(fa.is_null(), -1, "Cannot create FileAccess for path '" + p_file + "'.");

	return fa->_get_size(p_file);
}

uint32_t FileAccess::get_unix_permissions(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return 0;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), 0, vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_get_unix_permissions(p_file);
}

Error FileAccess::set_unix_permissions(const String& p_file, uint32_t p_permissions)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return ERR_UNAVAILABLE;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), ERR_CANT_CREATE, vformat("Cannot create FileAccess for path '%s'.", p_file));

	Error err = fa->_set_unix_permissions(p_file, p_permissions);
	return err;
}

bool FileAccess::get_hidden_attribute(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return false;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), false, vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_get_hidden_attribute(p_file);
}

Error FileAccess::set_hidden_attribute(const String& p_file, bool p_hidden)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return ERR_UNAVAILABLE;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), ERR_CANT_CREATE, vformat("Cannot create FileAccess for path '%s'.", p_file));

	Error err = fa->_set_hidden_attribute(p_file, p_hidden);
	return err;
}

bool FileAccess::get_read_only_attribute(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return false;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), false, vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_get_read_only_attribute(p_file);
}

Error FileAccess::set_read_only_attribute(const String& p_file, bool p_ro)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return ERR_UNAVAILABLE;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), ERR_CANT_CREATE, vformat("Cannot create FileAccess for path '%s'.", p_file));

	Error err = fa->_set_read_only_attribute(p_file, p_ro);
	return err;
}

Vector<uint8_t> FileAccess::get_extended_attribute(
	const String& p_file, const String& p_attribute_name)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return Vector<uint8_t>();
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(fa.is_null(), Vector<uint8_t>(),
		vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_get_extended_attribute(p_file, p_attribute_name);
}

String FileAccess::get_extended_attribute_string(
	const String& p_file, const String& p_attribute_name)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return String();
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), String(), vformat("Cannot create FileAccess for path '%s'.", p_file));

	Vector<uint8_t> data = fa->_get_extended_attribute(p_file, p_attribute_name);
	if (data.is_empty()) {
		return String();
	}
	return String::utf8((const char*)data.ptr(), data.size());
}

Error FileAccess::set_extended_attribute(
	const String& p_file, const String& p_attribute_name, const Vector<uint8_t>& p_data)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return ERR_UNAVAILABLE;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), ERR_CANT_CREATE, vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_set_extended_attribute(p_file, p_attribute_name, p_data);
}

Error FileAccess::set_extended_attribute_string(
	const String& p_file, const String& p_attribute_name, const String& p_data)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return ERR_UNAVAILABLE;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), ERR_CANT_CREATE, vformat("Cannot create FileAccess for path '%s'.", p_file));

	Vector<uint8_t> data;
	CharString cs = p_data.utf8();
	data.resize(cs.size());
	if (cs.size() > 0) {
		memcpy(data.ptrw(), cs.get_data(), cs.size());
	}

	return fa->_set_extended_attribute(p_file, p_attribute_name, data);
}

Error FileAccess::remove_extended_attribute(const String& p_file, const String& p_attribute_name)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return ERR_UNAVAILABLE;
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), ERR_CANT_CREATE, vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_remove_extended_attribute(p_file, p_attribute_name);
}

Vector<String> FileAccess::get_extended_attributes_list(const String& p_file)
{
	if (PackedData::is_initialized() && !PackedData::is_disabled() &&
		(PackedData::has_path(p_file) || PackedData::has_directory(p_file))) {
		return Vector<String>();
	}

	Ref<FileAccess> fa = create_for_path(p_file);
	ERR_FAIL_COND_V_MSG(
		fa.is_null(), Vector<String>(), vformat("Cannot create FileAccess for path '%s'.", p_file));

	return fa->_get_extended_attributes_list(p_file);
}

bool FileAccess::store_string(const String& p_string)
{
	if (p_string.length() == 0) {
		return true;
	}

	CharString cs = p_string.utf8();
	return store_buffer((uint8_t*)&cs[0], cs.length());
}

bool FileAccess::store_pascal_string(const String& p_string)
{
	CharString cs = p_string.utf8();
	return store_32(cs.length()) && store_buffer((uint8_t*)&cs[0], cs.length());
}

String FileAccess::get_pascal_string()
{
	uint32_t sl = get_32();
	CharString cs;
	cs.resize_uninitialized(sl + 1);
	get_buffer((uint8_t*)cs.ptr(), sl);
	cs[sl] = 0;

	return String::utf8(cs.ptr(), sl);
}

bool FileAccess::store_line(const String& p_line) { return store_string(p_line) && store_8('\n'); }

bool FileAccess::store_csv_line(const Vector<String>& p_values, const String& p_delim)
{
	ERR_FAIL_COND_V(p_delim.length() != 1, false);

	String line = "";
	int size = p_values.size();
	for (int i = 0; i < size; ++i) {
		String value = p_values[i];

		if (value.contains_char('"') || value.contains(p_delim) || value.contains_char('\n')) {
			value = "\"" + value.replace("\"", "\"\"") + "\"";
		}
		if (i < size - 1) {
			value += p_delim;
		}

		line += value;
	}

	return store_line(line);
}

bool FileAccess::store_buffer(const Vector<uint8_t>& p_buffer)
{
	uint64_t len = p_buffer.size();
	if (len == 0) {
		return true;
	}

	const uint8_t* r = p_buffer.ptr();

	return store_buffer(r, len);
}

bool FileAccess::store_buffer(const uint8_t* p_src, uint64_t p_length)
{
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);
	for (uint64_t i = 0; i < p_length; i++) {
		if (unlikely(!store_8(p_src[i]))) {
			return false;
		}
	}
	return true;
}

Vector<uint8_t> FileAccess::get_file_as_bytes(const String& p_path, Error* r_error)
{
	Ref<FileAccess> f = FileAccess::open(p_path, READ, r_error);
	if (f.is_null()) {
		if (r_error) { // if error requested, do not throw error
			return Vector<uint8_t>();
		}
		ERR_FAIL_V_MSG(
			Vector<uint8_t>(), vformat("Can't open file from path '%s'.", String(p_path)));
	}
	Vector<uint8_t> data;
	data.reserve_exact(f->get_length());
	data.resize(f->get_length());
	f->get_buffer(data.ptrw(), data.size());
	return data;
}

String FileAccess::get_file_as_string(const String& p_path, Error* r_error)
{
	Error err;
	Vector<uint8_t> array = get_file_as_bytes(p_path, &err);
	if (r_error) {
		*r_error = err;
	}
	if (err != OK) {
		if (r_error) {
			return String();
		}
		ERR_FAIL_V_MSG(
			String(), vformat("Can't get file as string from path '%s'.", String(p_path)));
	}

	String ret;
	ret.append_utf8((const char*)array.ptr(), array.size());
	return ret;
}

String FileAccess::get_md5(const String& p_file)
{
	Ref<FileAccess> f = FileAccess::open(p_file, READ);
	if (f.is_null()) {
		return String();
	}

	CryptoCore::MD5Context ctx;
	ctx.start();

	unsigned char step[32768];

	while (true) {
		uint64_t br = f->get_buffer(step, 32768);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	unsigned char hash[16];
	ctx.finish(hash);

	return String::md5(hash);
}

String FileAccess::get_multiple_md5(const Vector<String>& p_file)
{
	CryptoCore::MD5Context ctx;
	ctx.start();

	for (int i = 0; i < p_file.size(); i++) {
		Ref<FileAccess> f = FileAccess::open(p_file[i], READ);
		ERR_CONTINUE(f.is_null());

		unsigned char step[32768];

		while (true) {
			uint64_t br = f->get_buffer(step, 32768);
			if (br > 0) {
				ctx.update(step, br);
			}
			if (br < 4096) {
				break;
			}
		}
	}

	unsigned char hash[16];
	ctx.finish(hash);

	return String::md5(hash);
}

String FileAccess::get_sha256(const String& p_file)
{
	Ref<FileAccess> f = FileAccess::open(p_file, READ);
	if (f.is_null()) {
		return String();
	}

	CryptoCore::SHA256Context ctx;
	ctx.start();

	unsigned char step[32768];

	while (true) {
		uint64_t br = f->get_buffer(step, 32768);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	unsigned char hash[32];
	ctx.finish(hash);

	return String::hex_encode_buffer(hash, 32);
}

FileAccess::~FileAccess() { _delete_temp(); }



// ============================================================================
// Merged from core/io/compression.cpp
// ============================================================================

#include "compression.h"

#include "core/io/file_access.h"

#include <thirdparty/misc/fastlz.h>

#include <zstd.h>

#ifdef BROTLI_ENABLED
#include <brotli/decode.h>
#endif

namespace {
struct ZstdDecompressorContext {
	ZSTD_DCtx *zstd_d_ctx = nullptr;
	bool zstd_long_distance_matching = false;
	int zstd_window_log_size = 0;

	~ZstdDecompressorContext() {
		if (zstd_d_ctx) {
			ZSTD_freeDCtx(zstd_d_ctx);
		}
	}

	void invalidate(bool p_zstd_long_distance_matching, int p_zstd_window_log_size) {
		if (!zstd_d_ctx || zstd_long_distance_matching != p_zstd_long_distance_matching || zstd_window_log_size != p_zstd_window_log_size) {
			if (zstd_d_ctx) {
				ZSTD_freeDCtx(zstd_d_ctx);
			}

			zstd_d_ctx = ZSTD_createDCtx();
			if (p_zstd_long_distance_matching) {
				ZSTD_DCtx_setParameter(zstd_d_ctx, ZSTD_d_windowLogMax, p_zstd_window_log_size);
			}
			zstd_long_distance_matching = p_zstd_long_distance_matching;
			zstd_window_log_size = p_zstd_window_log_size;
		}
	}
};
} //namespace

int64_t Compression::compress(uint8_t *p_dst, const uint8_t *p_src, int64_t p_src_size, Mode p_mode) {
	switch (p_mode) {
		case MODE_BROTLI: {
			ERR_FAIL_V_MSG(-1, "Only brotli decompression is supported.");
		} break;
		case MODE_FASTLZ: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a FastLZ/LZ77 file 2 GiB or larger. LZ77 supports larger files, but FastLZ's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			if (p_src_size < 16) {
				uint8_t src[16];
				memset(&src[p_src_size], 0, 16 - p_src_size);
				memcpy(src, p_src, p_src_size);
				return fastlz_compress(src, 16, p_dst);
			} else {
				return fastlz_compress(p_src, p_src_size, p_dst);
			}

		} break;
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a Deflate or GZip file 2 GiB or larger. Deflate and GZip are both limited to 4 GiB, and ZLib's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

			z_stream strm;
			strm.zalloc = zipio_alloc;
			strm.zfree = zipio_free;
			strm.opaque = Z_NULL;
			int level = p_mode == MODE_DEFLATE ? zlib_level : gzip_level;
			int err = deflateInit2(&strm, level, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
			if (err != Z_OK) {
				return -1;
			}

			strm.avail_in = p_src_size;
			int aout = deflateBound(&strm, p_src_size);
			strm.avail_out = aout;
			strm.next_in = (Bytef *)p_src;
			strm.next_out = p_dst;
			deflate(&strm, Z_FINISH);
			aout = aout - strm.avail_out;
			deflateEnd(&strm);
			return aout;

		} break;
		case MODE_ZSTD: {
			ZSTD_CCtx *cctx = ZSTD_createCCtx();
			ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, zstd_level);
			if (zstd_long_distance_matching) {
				ZSTD_CCtx_setParameter(cctx, ZSTD_c_enableLongDistanceMatching, 1);
				ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, zstd_window_log_size);
			}
			const int64_t max_dst_size = get_max_compressed_buffer_size(p_src_size, MODE_ZSTD);
			const size_t ret = ZSTD_compressCCtx(cctx, p_dst, max_dst_size, p_src, p_src_size, zstd_level);
			ZSTD_freeCCtx(cctx);
			return (int64_t)ret;
		} break;
	}

	ERR_FAIL_V(-1);
}

int64_t Compression::get_max_compressed_buffer_size(int64_t p_src_size, Mode p_mode) {
	switch (p_mode) {
		case MODE_BROTLI: {
			ERR_FAIL_V_MSG(-1, "Only brotli decompression is supported.");
		} break;
		case MODE_FASTLZ: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a FastLZ/LZ77 file 2 GiB or larger. LZ77 supports larger files, but FastLZ's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int ss = p_src_size + p_src_size * 6 / 100;
			if (ss < 66) {
				ss = 66;
			}
			return ss;

		} break;
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a Deflate or GZip file 2 GiB or larger. Deflate and GZip are both limited to 4 GiB, and ZLib's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

			z_stream strm;
			strm.zalloc = zipio_alloc;
			strm.zfree = zipio_free;
			strm.opaque = Z_NULL;
			int err = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
			if (err != Z_OK) {
				return -1;
			}
			int aout = deflateBound(&strm, p_src_size);
			deflateEnd(&strm);
			return aout;
		} break;
		case MODE_ZSTD: {
			return ZSTD_compressBound(p_src_size);
		} break;
	}

	ERR_FAIL_V(-1);
}

int64_t Compression::decompress(uint8_t *p_dst, int64_t p_dst_max_size, const uint8_t *p_src, int64_t p_src_size, Mode p_mode) {
	switch (p_mode) {
		case MODE_BROTLI: {
#ifdef BROTLI_ENABLED
			size_t ret_size = p_dst_max_size;
			BrotliDecoderResult res = BrotliDecoderDecompress(p_src_size, p_src, &ret_size, p_dst);
			ERR_FAIL_COND_V(res != BROTLI_DECODER_RESULT_SUCCESS, -1);
			return ret_size;
#else
			ERR_FAIL_V_MSG(-1, "Godot was compiled without brotli support.");
#endif
		} break;
		case MODE_FASTLZ: {
			ERR_FAIL_COND_V_MSG(p_dst_max_size > INT32_MAX, -1, "Cannot decompress a FastLZ/LZ77 file 2 GiB or larger. LZ77 supports larger files, but FastLZ's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int ret_size = 0;

			if (p_dst_max_size < 16) {
				uint8_t dst[16];
				fastlz_decompress(p_src, p_src_size, dst, 16);
				memcpy(p_dst, dst, p_dst_max_size);
				ret_size = p_dst_max_size;
			} else {
				ret_size = fastlz_decompress(p_src, p_src_size, p_dst, p_dst_max_size);
			}
			return ret_size;
		} break;
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ERR_FAIL_COND_V_MSG(p_dst_max_size > INT32_MAX, -1, "Cannot decompress a Deflate or GZip file 2 GiB or larger. Deflate and GZip are both limited to 4 GiB, and ZLib's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

			z_stream strm;
			strm.zalloc = zipio_alloc;
			strm.zfree = zipio_free;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;
			int err = inflateInit2(&strm, window_bits);
			ERR_FAIL_COND_V(err != Z_OK, -1);

			strm.avail_in = p_src_size;
			strm.avail_out = p_dst_max_size;
			strm.next_in = (Bytef *)p_src;
			strm.next_out = p_dst;

			err = inflate(&strm, Z_FINISH);
			int total = strm.total_out;
			inflateEnd(&strm);
			ERR_FAIL_COND_V(err != Z_STREAM_END, -1);
			return total;
		} break;
		case MODE_ZSTD: {
			thread_local ZstdDecompressorContext decompressor_ctx;
			decompressor_ctx.invalidate(zstd_long_distance_matching, zstd_window_log_size);

			size_t ret = ZSTD_decompressDCtx(decompressor_ctx.zstd_d_ctx, p_dst, p_dst_max_size, p_src, p_src_size);
			return (int64_t)ret;
		} break;
	}

	ERR_FAIL_V(-1);
}

/**
	This will handle both Gzip and Deflate streams. It will automatically allocate the output buffer into the provided p_dst_vect Vector.
	This is required for compressed data whose final uncompressed size is unknown, as is the case for HTTP response bodies.
	This is much slower however than using Compression::decompress because it may result in multiple full copies of the output buffer.
*/
int Compression::decompress_dynamic(Vector<uint8_t> *p_dst_vect, int64_t p_max_dst_size, const uint8_t *p_src, int64_t p_src_size, Mode p_mode) {
	uint8_t *dst = nullptr;
	int out_mark = 0;

	ERR_FAIL_COND_V(p_src_size <= 0, Z_DATA_ERROR);

	if (p_mode == MODE_BROTLI) {
#ifdef BROTLI_ENABLED
		BrotliDecoderResult ret;
		BrotliDecoderState *state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
		ERR_FAIL_NULL_V(state, Z_DATA_ERROR);

		// Setup the stream inputs.
		const uint8_t *next_in = p_src;
		size_t avail_in = p_src_size;
		uint8_t *next_out = nullptr;
		size_t avail_out = 0;
		size_t total_out = 0;

		// Ensure the destination buffer is empty.
		p_dst_vect->clear();

		// Decompress until stream ends or end of file.
		do {
			// Add another chunk size to the output buffer.
			// This forces a copy of the whole buffer.
			p_dst_vect->resize(p_dst_vect->size() + gzip_chunk);
			// Get pointer to the actual output buffer.
			dst = p_dst_vect->ptrw();

			// Set the stream to the new output stream.
			// Since it was copied, we need to reset the stream to the new buffer.
			next_out = &(dst[out_mark]);
			avail_out += gzip_chunk;

			ret = BrotliDecoderDecompressStream(state, &avail_in, &next_in, &avail_out, &next_out, &total_out);
			if (ret == BROTLI_DECODER_RESULT_ERROR) {
				WARN_PRINT(BrotliDecoderErrorString(BrotliDecoderGetErrorCode(state)));
				BrotliDecoderDestroyInstance(state);
				p_dst_vect->clear();
				return Z_DATA_ERROR;
			}

			out_mark += gzip_chunk - avail_out;

			// Enforce max output size.
			if (p_max_dst_size > -1 && total_out > (uint64_t)p_max_dst_size) {
				BrotliDecoderDestroyInstance(state);
				p_dst_vect->clear();
				return Z_BUF_ERROR;
			}
		} while (ret != BROTLI_DECODER_RESULT_SUCCESS);

		// If all done successfully, resize the output if it's larger than the actual output.
		if ((unsigned long)p_dst_vect->size() > total_out) {
			p_dst_vect->resize(total_out);
		}

		// Clean up and return.
		BrotliDecoderDestroyInstance(state);
		return Z_OK;
#else
		ERR_FAIL_V_MSG(Z_ERRNO, "Godot was compiled without brotli support.");
#endif
	} else {
		// This function only supports GZip and Deflate.
		ERR_FAIL_COND_V_MSG(p_mode != MODE_DEFLATE && p_mode != MODE_GZIP, Z_ERRNO, "Dynamic decompression is only supported with gzip, DEFLATE, and Brotli compression methods.");

		int ret;
		z_stream strm;
		int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

		// Initialize the stream.
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;

		int err = inflateInit2(&strm, window_bits);
		ERR_FAIL_COND_V(err != Z_OK, -1);

		// Setup the stream inputs.
		strm.next_in = (Bytef *)p_src;
		strm.avail_in = p_src_size;

		// Ensure the destination buffer is empty.
		p_dst_vect->clear();

		// Decompress until deflate stream ends or end of file.
		do {
			// Add another chunk size to the output buffer.
			// This forces a copy of the whole buffer.
			p_dst_vect->resize(p_dst_vect->size() + gzip_chunk);
			// Get pointer to the actual output buffer.
			dst = p_dst_vect->ptrw();

			// Set the stream to the new output stream.
			// Since it was copied, we need to reset the stream to the new buffer.
			strm.next_out = &(dst[out_mark]);
			strm.avail_out = gzip_chunk;

			// Run inflate() on input until output buffer is full and needs to be resized or input runs out.
			do {
				ret = inflate(&strm, Z_SYNC_FLUSH);

				switch (ret) {
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;
						[[fallthrough]];
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
					case Z_STREAM_ERROR:
					case Z_BUF_ERROR:
						if (strm.msg) {
							WARN_PRINT(strm.msg);
						}
						(void)inflateEnd(&strm);
						p_dst_vect->clear();
						return ret;
				}
			} while (strm.avail_out > 0 && strm.avail_in > 0);

			out_mark += gzip_chunk;

			// Enforce max output size.
			if (p_max_dst_size > -1 && strm.total_out > (uint64_t)p_max_dst_size) {
				(void)inflateEnd(&strm);
				p_dst_vect->clear();
				return Z_BUF_ERROR;
			}
		} while (ret != Z_STREAM_END);

		// If all done successfully, resize the output if it's larger than the actual output.
		if ((unsigned long)p_dst_vect->size() > strm.total_out) {
			p_dst_vect->resize(strm.total_out);
		}

		// Clean up and return.
		(void)inflateEnd(&strm);
		return Z_OK;
	}
}

// ==========================================================================
// delta_encoding.cpp
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

#include <zstd.h>
#include "core/typedefs.h"
#include "core/templates/vector.h"
#include "core/types.h"
#include "core/io/compression.h"

#define ERR_FAIL_ZSTD_V_MSG(m_result, m_retval, m_msg)                                             \
	ERR_FAIL_COND_V_MSG(ZSTD_isError(m_result), m_retval,                                          \
		vformat("%s Zstandard reported error code %d: \"%s\".", m_msg,                             \
			ZSTD_getErrorCode(m_result), ZSTD_getErrorString(ZSTD_getErrorCode(m_result))))

struct ZstdCompressionContext
{
	ZSTD_CCtx* context = ZSTD_createCCtx();

	~ZstdCompressionContext() { ZSTD_freeCCtx(context); }

	operator ZSTD_CCtx*() { return context; }
};

struct ZstdDecompressionContext
{
	ZSTD_DCtx* context = ZSTD_createDCtx();

	~ZstdDecompressionContext() { ZSTD_freeDCtx(context); }

	operator ZSTD_DCtx*() { return context; }
};

static constexpr uint8_t DELTA_MAGIC[4] = {'G', 'D', 'D', 'L'};
static constexpr uint8_t DELTA_VERSION_NUMBER = 1;
static constexpr size_t DELTA_HEADER_SIZE = 5;

Error DeltaEncoding::encode_delta(Span<uint8_t> p_old_data, Span<uint8_t> p_new_data,
	Vector<uint8_t>& r_delta, int p_compression_level)
{
	size_t zstd_result = ZSTD_compressBound(p_new_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Calculating compression bounds failed.");

	r_delta.reserve_exact(DELTA_HEADER_SIZE + zstd_result);
	r_delta.resize(DELTA_HEADER_SIZE + zstd_result);

	memcpy(r_delta.ptrw(), DELTA_MAGIC, 4);
	r_delta.write[4] = DELTA_VERSION_NUMBER;

	ZstdCompressionContext zstd_context;

	zstd_result =
		ZSTD_CCtx_setParameter(zstd_context, ZSTD_c_compressionLevel, p_compression_level);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting compression level failed.");

	zstd_result = ZSTD_CCtx_setPledgedSrcSize(zstd_context, p_new_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting pledged source size failed.");

	zstd_result = ZSTD_CCtx_refPrefix(zstd_context, p_old_data.ptr(), p_old_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting prefix dictionary failed.");

	zstd_result = ZSTD_CCtx_setParameter(zstd_context, ZSTD_c_contentSizeFlag, 1);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting content size flag failed.");

	zstd_result = ZSTD_CCtx_setParameter(zstd_context, ZSTD_c_checksumFlag, 1);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting checksum flag failed.");

	zstd_result = ZSTD_compress2(zstd_context, r_delta.ptrw() + DELTA_HEADER_SIZE,
		r_delta.size() - DELTA_HEADER_SIZE, p_new_data.ptr(), p_new_data.size());
	ERR_FAIL_ZSTD_V_MSG(zstd_result, FAILED, "Failed to encode delta. Compression failed.");

	r_delta.resize(DELTA_HEADER_SIZE + zstd_result);

	return OK;
}

Error DeltaEncoding::decode_delta(
	Span<uint8_t> p_old_data, Span<uint8_t> p_delta, Vector<uint8_t>& r_new_data)
{
	ERR_FAIL_COND_V_MSG(p_delta.size() < DELTA_HEADER_SIZE, ERR_INVALID_DATA,
		vformat("Failed to decode delta. File size (%d) is too small.", p_delta.size()));

	uint8_t magic[4];
	memcpy(magic, p_delta.ptr(), sizeof(magic));
	uint8_t version = p_delta[4];

	ERR_FAIL_COND_V_MSG(memcmp(magic, DELTA_MAGIC, 4) != 0, ERR_FILE_CORRUPT,
		"Failed to decode delta. Header is invalid.");
	ERR_FAIL_COND_V_MSG(version != DELTA_VERSION_NUMBER, ERR_FILE_UNRECOGNIZED,
		vformat("Failed to decode delta. Expected version %d but found %d.", DELTA_VERSION_NUMBER,
			version));

	size_t zstd_result = ZSTD_getFrameContentSize(
		p_delta.ptr() + DELTA_HEADER_SIZE, p_delta.size() - DELTA_HEADER_SIZE);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to decode delta. Unable to find decompressed size.");

	r_new_data.reserve_exact(zstd_result);
	r_new_data.resize(zstd_result);

	ZstdDecompressionContext zstd_context;

	zstd_result = ZSTD_DCtx_refPrefix(zstd_context, p_old_data.ptr(), p_old_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to decode delta. Setting prefix dictionary failed.");

	zstd_result = ZSTD_decompressDCtx(zstd_context, r_new_data.ptrw(), r_new_data.size(),
		p_delta.ptr() + DELTA_HEADER_SIZE, p_delta.size() - DELTA_HEADER_SIZE);
	ERR_FAIL_ZSTD_V_MSG(zstd_result, FAILED, "Failed to decode delta. Decompression failed.");
	ERR_FAIL_COND_V(zstd_result != (size_t)r_new_data.size(), ERR_FILE_CORRUPT);

	return OK;
}
#ifdef ERR_FAIL_ZSTD_V_MSG
#undef ERR_FAIL_ZSTD_V_MSG
#endif

// ==========================================================================
// zip_io.cpp
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

#include "core/io/file_access.h"

#include "core/templates/local_vector.h"

int godot_unzip_get_current_file_info(unzFile p_zip_file, unz_file_info64 &r_file_info, String &r_filepath) {
	const uLong short_file_path_buffer_size = 16384ul;
	char short_file_path_buffer[short_file_path_buffer_size];

	int err = unzGetCurrentFileInfo64(p_zip_file, &r_file_info, short_file_path_buffer, short_file_path_buffer_size, nullptr, 0, nullptr, 0);
	if (unlikely((err != UNZ_OK) || (r_file_info.size_filename > short_file_path_buffer_size))) {
		LocalVector<char> long_file_path_buffer;
		long_file_path_buffer.resize(r_file_info.size_filename);

		err = unzGetCurrentFileInfo64(p_zip_file, &r_file_info, long_file_path_buffer.ptr(), long_file_path_buffer.size(), nullptr, 0, nullptr, 0);
		if (err != UNZ_OK) {
			return err;
		}
		r_filepath = String::utf8(long_file_path_buffer.ptr(), r_file_info.size_filename);
	} else {
		r_filepath = String::utf8(short_file_path_buffer, r_file_info.size_filename);
	}

	return err;
}

int godot_unzip_locate_file(unzFile p_zip_file, const String &p_filepath, bool p_case_sensitive) {
	int err = unzGoToFirstFile(p_zip_file);
	while (err == UNZ_OK) {
		unz_file_info64 current_file_info;
		String current_filepath;
		err = godot_unzip_get_current_file_info(p_zip_file, current_file_info, current_filepath);
		if (err == UNZ_OK) {
			bool filepaths_are_equal = p_case_sensitive ? (p_filepath == current_filepath) : (p_filepath.nocasecmp_to(current_filepath) == 0);
			if (filepaths_are_equal) {
				return UNZ_OK;
			}
			err = unzGoToNextFile(p_zip_file);
		}
	}
	return err;
}

//

void *zipio_open(voidpf p_opaque, const char *p_fname, int p_mode) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, nullptr);

	String fname = String::utf8(p_fname);

	int file_access_mode = 0;
	if (p_mode & ZLIB_FILEFUNC_MODE_READ) {
		file_access_mode |= FileAccess::READ;
	}
	if (p_mode & ZLIB_FILEFUNC_MODE_WRITE) {
		file_access_mode |= FileAccess::WRITE;
	}
	if (p_mode & ZLIB_FILEFUNC_MODE_CREATE) {
		file_access_mode |= FileAccess::WRITE_READ;
	}

	(*fa) = FileAccess::open(fname, file_access_mode);
	if (fa->is_null()) {
		return nullptr;
	}

	return p_opaque;
}

uLong zipio_read(voidpf p_opaque, voidpf p_stream, void *p_buf, uLong p_size) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return (*fa)->get_buffer((uint8_t *)p_buf, p_size);
}

uLong zipio_write(voidpf p_opaque, voidpf p_stream, const void *p_buf, uLong p_size) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	bool fa_success = (*fa)->store_buffer((uint8_t *)p_buf, p_size);

	if (fa_success) {
		return p_size;
	}

	return 0;
}

long zipio_tell(voidpf p_opaque, voidpf p_stream) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return (*fa)->get_position();
}

long zipio_seek(voidpf p_opaque, voidpf p_stream, uLong p_offset, int p_origin) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	uint64_t pos = p_offset;
	switch (p_origin) {
		case ZLIB_FILEFUNC_SEEK_CUR:
			pos = (*fa)->get_position() + p_offset;
			break;
		case ZLIB_FILEFUNC_SEEK_END:
			pos = (*fa)->get_length() + p_offset;
			break;
		default:
			break;
	}

	(*fa)->seek(pos);
	return 0;
}

int zipio_close(voidpf p_opaque, voidpf p_stream) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	fa->unref();
	return 0;
}

int zipio_testerror(voidpf p_opaque, voidpf p_stream) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 1);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return (fa->is_valid() && (*fa)->get_error() != OK) ? 1 : 0;
}

voidpf zipio_alloc(voidpf p_opaque, uInt p_items, uInt p_size) {
	voidpf ptr = memalloc_zeroed((size_t)p_items * p_size);
	return ptr;
}

void zipio_free(voidpf p_opaque, voidpf r_address) {
	memfree(r_address);
}

zlib_filefunc_def zipio_create_io(Ref<FileAccess> *p_data) {
	zlib_filefunc_def io;
	io.opaque = (void *)p_data;
	io.zopen_file = zipio_open;
	io.zread_file = zipio_read;
	io.zwrite_file = zipio_write;
	io.ztell_file = zipio_tell;
	io.zseek_file = zipio_seek;
	io.zclose_file = zipio_close;
	io.zerror_file = zipio_testerror;
	io.alloc_mem = zipio_alloc;
	io.free_mem = zipio_free;
	return io;
}

// ==========================================================================
// pck_packer.cpp
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
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/version.h"
#include "core/io/compression.h"

static int _get_pad(int p_alignment, int p_n)
{
	int rest = p_n % p_alignment;
	int pad = 0;
	if (rest > 0) {
		pad = p_alignment - rest;
	}

	return pad;
}

Error PCKPacker::pck_start(
	const String& p_pck_path, int p_alignment, const String& p_key, bool p_encrypt_directory)
{
	ERR_FAIL_COND_V_MSG(
		(p_key.is_empty() || !p_key.is_valid_hex_number(false) || p_key.length() != 64),
		ERR_CANT_CREATE, "Invalid Encryption Key (must be 64 characters long).");
	ERR_FAIL_COND_V_MSG(
		p_alignment <= 0, ERR_CANT_CREATE, "Invalid alignment, must be greater then 0.");

	String _key = p_key.to_lower();
	key.resize(32);
	for (int i = 0; i < 32; i++) {
		int v = 0;
		if (i * 2 < _key.length()) {
			char32_t ct = _key[i * 2];
			if (is_digit(ct)) {
				ct = ct - '0';
			}
			else if (ct >= 'a' && ct <= 'f') {
				ct = 10 + ct - 'a';
			}
			v |= ct << 4;
		}

		if (i * 2 + 1 < _key.length()) {
			char32_t ct = _key[i * 2 + 1];
			if (is_digit(ct)) {
				ct = ct - '0';
			}
			else if (ct >= 'a' && ct <= 'f') {
				ct = 10 + ct - 'a';
			}
			v |= ct;
		}
		key.write[i] = v;
	}
	enc_dir = p_encrypt_directory;

	file = FileAccess::open(p_pck_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_CANT_CREATE,
		vformat("Can't open file to write: '%s'.", String(p_pck_path)));

	alignment = p_alignment;

	file->store_32(PACK_HEADER_MAGIC);
	file->store_32(PACK_FORMAT_VERSION);
	file->store_32(VLTR_VERSION_MAJOR);
	file->store_32(VLTR_VERSION_MINOR);
	file->store_32(VLTR_VERSION_PATCH);

	uint32_t pack_flags = PACK_REL_FILEBASE;
	if (enc_dir) {
		pack_flags |= PACK_DIR_ENCRYPTED;
	}
	file->store_32(pack_flags); // flags

	file_base_ofs = file->get_position();
	file->store_64(0); // Files base.

	dir_base_ofs = file->get_position();
	file->store_64(0); // Directory offset.

	for (int i = 0; i < 16; i++) {
		file->store_32(0); // Reserved.
	}

	// Align for first file.
	int pad = _get_pad(alignment, file->get_position());
	for (int i = 0; i < pad; i++) {
		file->store_8(0);
	}

	file_base = file->get_position();
	file->seek(file_base_ofs);
	file->store_64(file_base); // Update files base.
	file->seek(file_base);

	files.clear();

	return OK;
}

Error PCKPacker::add_file_removal(const String& p_target_path)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	File pf;
	// Simplify path here and on every 'files' access so that paths that have extra '/'
	// symbols or 'res://' in them still match the MD5 hash for the saved path.
	pf.path = p_target_path.simplify_path().trim_prefix("res://");
	pf.ofs = file->get_position();
	pf.size = 0;
	pf.removal = true;

	pf.md5.resize_initialized(16);

	files.push_back(pf);

	return OK;
}

Error PCKPacker::add_file(const String& p_target_path, const String& p_source_path, bool p_encrypt)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	Ref<FileAccess> f = FileAccess::open(p_source_path, FileAccess::READ);
	if (f.is_null()) {
		return ERR_FILE_CANT_OPEN;
	}
	Vector<uint8_t> data = FileAccess::get_file_as_bytes(p_source_path);

	return _add_file(p_target_path, p_source_path, data, p_encrypt);
}

Error PCKPacker::add_file_from_buffer(
	const String& p_target_path, const Vector<uint8_t>& p_data, bool p_encrypt)
{
	return _add_file(p_target_path, "<PackedByteArray>", p_data, p_encrypt);
}

Error PCKPacker::_add_file(const String& p_target_path, const String& p_source_path,
	const Vector<uint8_t>& p_data, bool p_encrypt)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	File pf;
	// Simplify path here and on every 'files' access so that paths that have extra '/'
	// symbols or 'res://' in them still match the MD5 hash for the saved path.
	pf.path = p_target_path.simplify_path().trim_prefix("res://");
	pf.src_path = p_source_path;
	pf.ofs = file->get_position();
	pf.size = p_data.size();

	{
		unsigned char hash[16];
		CryptoCore::md5(p_data.ptr(), p_data.size(), hash);
		pf.md5.resize(16);
		for (int i = 0; i < 16; i++) {
			pf.md5.write[i] = hash[i];
		}
	}
	pf.encrypted = p_encrypt;

	Ref<FileAccess> ftmp = file;

	Ref<FileAccessEncrypted> fae;
	if (p_encrypt) {
		fae.instantiate();
		ERR_FAIL_COND_V(fae.is_null(), ERR_CANT_CREATE);

		Error err = fae->open_and_parse(file, key, FileAccessEncrypted::MODE_WRITE_AES256, false);
		ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);
		ftmp = fae;
	}

	ftmp->store_buffer(p_data);

	if (fae.is_valid()) {
		ftmp.unref();
		fae.unref();
	}

	int pad = _get_pad(alignment, file->get_position());
	for (int j = 0; j < pad; j++) {
		file->store_8(0);
	}

	files.push_back(pf);

	return OK;
}

Error PCKPacker::flush(bool p_verbose)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	int dir_padding = _get_pad(alignment, file->get_position());
	for (int i = 0; i < dir_padding; i++) {
		file->store_8(0);
	}

	// Write directory.
	uint64_t dir_offset = file->get_position();
	file->seek(dir_base_ofs);
	file->store_64(dir_offset);
	file->seek(dir_offset);

	file->store_32(uint32_t(files.size()));

	Ref<FileAccessEncrypted> fae;
	Ref<FileAccess> fhead = file;

	if (enc_dir) {
		fae.instantiate();
		ERR_FAIL_COND_V(fae.is_null(), ERR_CANT_CREATE);

		Error err = fae->open_and_parse(file, key, FileAccessEncrypted::MODE_WRITE_AES256, false);
		ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);

		fhead = fae;
	}

	const int file_num = files.size();
	for (int i = 0; i < file_num; i++) {
		CharString utf8_string = files[i].path.utf8();
		int string_len = utf8_string.length();
		int pad = _get_pad(4, string_len);

		fhead->store_32(uint32_t(string_len + pad));
		fhead->store_buffer((const uint8_t*)utf8_string.get_data(), string_len);
		for (int j = 0; j < pad; j++) {
			fhead->store_8(0);
		}

		fhead->store_64(files[i].ofs - file_base);
		fhead->store_64(files[i].size);
		fhead->store_buffer(files[i].md5.ptr(), 16);

		uint32_t flags = 0;
		if (files[i].encrypted) {
			flags |= PACK_FILE_ENCRYPTED;
		}
		if (files[i].removal) {
			flags |= PACK_FILE_REMOVAL;
		}
		fhead->store_32(flags);

		if (p_verbose) {
			__print_line(vformat("[%d/%d - %d%%] PCKPacker flush: %s -> %s", i + 1, file_num,
				float(i + 1) / file_num * 100, files[i].src_path, files[i].path));
		}
	}

	if (fae.is_valid()) {
		fhead.unref();
		fae.unref();
	}

	file.unref();
	return OK;
}

PCKPacker::~PCKPacker()
{
	if (file.is_valid()) {
		flush();
	}
}

// ==========================================================================
// marshalls.cpp
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

#include <climits>
#include <cstdio>
#include "core/io/resource_loader.h"
#include "core/types.h"

#define ERR_FAIL_ADD_OF(a, b, err)                                                                 \
	ERR_FAIL_COND_V(                                                                               \
		((int32_t)(b)) < 0 || ((int32_t)(a)) < 0 || ((int32_t)(a)) > INT_MAX - ((int32_t)(b)),     \
		err)
#define ERR_FAIL_MUL_OF(a, b, err)                                                                 \
	ERR_FAIL_COND_V(                                                                               \
		((int32_t)(a)) < 0 || ((int32_t)(b)) <= 0 || ((int32_t)(a)) > INT_MAX / ((int32_t)(b)),    \
		err)

// Byte 0: `Variant::Type`, byte 1: unused, bytes 2 and 3: additional data.
#define HEADER_TYPE_MASK 0xFF

// For `Variant::INT`, `Variant::FLOAT` and other math types.
#define HEADER_DATA_FLAG_64 (1 << 16)

// For `Variant::OBJECT`.
#define HEADER_DATA_FLAG_OBJECT_AS_ID (1 << 16)

// For `Variant::ARRAY`.
// Occupies bits 16 and 17.
#define HEADER_DATA_FIELD_TYPED_ARRAY_MASK (0b11 << 16)
#define HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT 16

// For `Variant::DICTIONARY`.
// Occupies bits 16 and 17.
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK (0b11 << 16)
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT 16
// Occupies bits 18 and 19.
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK (0b11 << 18)
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT 18

enum ContainerTypeKind
{
	CONTAINER_TYPE_KIND_NONE = 0b00,
	CONTAINER_TYPE_KIND_BUILTIN = 0b01,
	CONTAINER_TYPE_KIND_CLASS_NAME = 0b10,
	CONTAINER_TYPE_KIND_SCRIPT = 0b11,
};

#define GET_CONTAINER_TYPE_KIND(m_header, m_field)                                                 \
	((ContainerTypeKind)(((m_header)&HEADER_DATA_FIELD_##m_field##_MASK) >>                        \
						 HEADER_DATA_FIELD_##m_field##_SHIFT))

static Error _decode_string(const uint8_t*& p_buffer, int& r_left, int* r_len, String& r_string)
{
	ERR_FAIL_COND_V(r_left < 4, ERR_INVALID_DATA);

	int32_t strlen = decode_uint32(p_buffer);
	int32_t pad = 0;

	// Handle padding.
	if (strlen % 4) {
		pad = 4 - strlen % 4;
	}

	p_buffer += 4;
	r_left -= 4;

	// Ensure buffer is big enough.
	ERR_FAIL_ADD_OF(strlen, pad, ERR_FILE_EOF);
	ERR_FAIL_COND_V(strlen < 0 || strlen + pad > r_left, ERR_FILE_EOF);

	String str;
	ERR_FAIL_COND_V(str.append_utf8((const char*)p_buffer, strlen) != OK, ERR_INVALID_DATA);
	r_string = str;

	// Add padding.
	strlen += pad;

	// Update buffer pos, left data count, and return size.
	p_buffer += strlen;
	r_left -= strlen;
	if (r_len) {
		(*r_len) += 4 + strlen;
	}

	return OK;
}

static void _encode_string(const String& p_string, uint8_t*& p_buffer, int& r_len)
{
	CharString utf8 = p_string.utf8();

	if (p_buffer) {
		encode_uint32(utf8.length(), p_buffer);
		p_buffer += 4;
		memcpy(p_buffer, utf8.get_data(), utf8.length());
		p_buffer += utf8.length();
	}

	r_len += 4 + utf8.length();
	while (r_len % 4) {
		r_len++; // Pad.
		if (p_buffer) {
			*(p_buffer++) = 0;
		}
	}
}

Vector<float> vector3_to_float32_array(const Vector3* p_vecs, size_t p_count)
{
	// We always allocate a new array, and we don't `memcpy()`.
	// We also don't consider returning a pointer to the passed vectors when `sizeof(real_t) == 4`.
	// One reason is that we could decide to put a 4th component in `Vector3` for SIMD/mobile
	// performance, which would cause trouble with these optimizations.
	Vector<float> floats;
	if (p_count == 0) {
		return floats;
	}
	floats.resize(p_count * 3);
	float* floats_w = floats.ptrw();
	for (size_t i = 0; i < p_count; ++i) {
		const Vector3 v = p_vecs[i];
		floats_w[0] = v.x;
		floats_w[1] = v.y;
		floats_w[2] = v.z;
		floats_w += 3;
	}
	return floats;
}
#ifdef ERR_FAIL_ADD_OF
#undef ERR_FAIL_ADD_OF
#endif
#ifdef ERR_FAIL_MUL_OF
#undef ERR_FAIL_MUL_OF
#endif
#ifdef HEADER_TYPE_MASK
#undef HEADER_TYPE_MASK
#endif
#ifdef HEADER_DATA_FLAG_64
#undef HEADER_DATA_FLAG_64
#endif
#ifdef HEADER_DATA_FLAG_OBJECT_AS_ID
#undef HEADER_DATA_FLAG_OBJECT_AS_ID
#endif
#ifdef HEADER_DATA_FIELD_TYPED_ARRAY_MASK
#undef HEADER_DATA_FIELD_TYPED_ARRAY_MASK
#endif
#ifdef HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT
#undef HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT
#endif
#ifdef GET_CONTAINER_TYPE_KIND
#undef GET_CONTAINER_TYPE_KIND
#endif

// ==========================================================================
// packed_data_container.cpp
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

#include "core/io/resource_loader.h"
#include "core/io/compression.h"

int PackedDataContainer::size() const { return _size(0); }

uint32_t PackedDataContainer::_type_at_ofs(uint32_t p_ofs) const
{
	ERR_FAIL_COND_V(p_ofs + 4 > (uint32_t)data.size(), 0);
	const uint8_t* rd = data.ptr();
	ERR_FAIL_NULL_V(rd, 0);
	const uint8_t* r = &rd[p_ofs];
	uint32_t type = decode_uint32(r);

	return type;
}

int PackedDataContainer::_size(uint32_t p_ofs) const
{
	ERR_FAIL_COND_V(p_ofs + 4 > (uint32_t)data.size(), 0);
	const uint8_t* rd = data.ptr();
	ERR_FAIL_NULL_V(rd, 0);
	const uint8_t* r = &rd[p_ofs];
	uint32_t type = decode_uint32(r);

	if (type == TYPE_ARRAY) {
		uint32_t len = decode_uint32(r + 4);
		return len;

	}
	else if (type == TYPE_DICT) {
		uint32_t len = decode_uint32(r + 4);
		return len;
	}

	return -1;
}

void PackedDataContainer::_set_data(const Vector<uint8_t>& p_data)
{
	data = p_data;
	datalen = data.size();
}

Vector<uint8_t> PackedDataContainer::_get_data() const { return data; }

int PackedDataContainerRef::size() const { return from->_size(offset); }

// ============================================================================
// Merged from core/io/config_file.cpp
// ============================================================================

#include "config_file.h"
#include "core/io/file_access_wrappers.h"
#include "core/string/ustring.h"

void ConfigFile::erase_section_key(const String& p_section, const String& p_key) {}

bool ConfigFile::has_section_key(const String& p_section, const String& p_key) const
{
	return true;
}

Error ConfigFile::save(const String& p_path)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {
		return err;
	}

	return _internal_save(file);
}

Error ConfigFile::save_encrypted(const String& p_path, const Vector<uint8_t>& p_key)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse(f, p_key, FileAccessEncrypted::MODE_WRITE_AES256);
	if (err) {
		return err;
	}
	return _internal_save(fae);
}

Error ConfigFile::save_encrypted_pass(const String& p_path, const String& p_pass)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse_password(f, p_pass, FileAccessEncrypted::MODE_WRITE_AES256);
	if (err) {
		return err;
	}

	return _internal_save(fae);
}

Error ConfigFile::load(const String& p_path)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (f.is_null()) {
		return err;
	}

	return _internal_load(p_path, f);
}

Error ConfigFile::load_encrypted(const String& p_path, const Vector<uint8_t>& p_key)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse(f, p_key, FileAccessEncrypted::MODE_READ);
	if (err) {
		return err;
	}
	return _internal_load(p_path, fae);
}

Error ConfigFile::load_encrypted_pass(const String& p_path, const String& p_pass)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse_password(f, p_pass, FileAccessEncrypted::MODE_READ);
	if (err) {
		return err;
	}

	return _internal_load(p_path, fae);
}



Error ConfigFile::_internal_save(Ref<FileAccess>) {}

Error ConfigFile::_internal_load(String const&, Ref<FileAccess>) {}

void ConfigFile::erase_section(String const&) {}

bool ConfigFile::has_section(String const&) const {}

// ==========================================================================
// json.cpp
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

#include "core/io/resource_loader.h"
#include "core/io/config_file.h"

const char* JSON::tk_name[TK_MAX] = {
	"'{'",
	"'}'",
	"'['",
	"']'",
	"identifier",
	"string",
	"number",
	"':'",
	"','",
	"EOF",
};

void JSON::_add_indent(String& r_result, const String& p_indent, int p_size)
{
	for (int i = 0; i < p_size; i++) {
		r_result += p_indent;
	}
}

Error JSON::_get_token(
	const char32_t* p_str, int& r_index, int p_len, Token& r_token, int& r_line, String& r_err_str)
{
	while (p_len > 0) {
		switch (p_str[r_index]) {
		case '\n': {
			r_line++;
			r_index++;
			break;
		}
		case 0: {
			r_token.type = TK_EOF;
			return OK;
		} break;
		case '{': {
			r_token.type = TK_CURLY_BRACKET_OPEN;
			r_index++;
			return OK;
		}
		case '}': {
			r_token.type = TK_CURLY_BRACKET_CLOSE;
			r_index++;
			return OK;
		}
		case '[': {
			r_token.type = TK_BRACKET_OPEN;
			r_index++;
			return OK;
		}
		case ']': {
			r_token.type = TK_BRACKET_CLOSE;
			r_index++;
			return OK;
		}
		case ':': {
			r_token.type = TK_COLON;
			r_index++;
			return OK;
		}
		case ',': {
			r_token.type = TK_COMMA;
			r_index++;
			return OK;
		}
		case '"': {
			r_index++;
			String str;
			while (true) {
				if (p_str[r_index] == 0) {
					r_err_str = "Unterminated string";
					return ERR_PARSE_ERROR;
				}
				else if (p_str[r_index] == '"') {
					r_index++;
					break;
				}
				else if (p_str[r_index] == '\\') {
					// escaped characters...
					r_index++;
					char32_t next = p_str[r_index];
					if (next == 0) {
						r_err_str = "Unterminated string";
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
					case 'u': {
						// hex number
						for (int j = 0; j < 4; j++) {
							char32_t c = p_str[r_index + j + 1];
							if (c == 0) {
								r_err_str = "Unterminated string";
								return ERR_PARSE_ERROR;
							}
							if (!is_hex_digit(c)) {
								r_err_str = "Malformed hex constant in string";
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
						r_index += 4; // will add at the end anyway

						if ((res & 0xfffffc00) == 0xd800) {
							if (p_str[r_index + 1] != '\\' || p_str[r_index + 2] != 'u') {
								r_err_str =
									"Invalid UTF-16 sequence in string, unpaired lead surrogate";
								return ERR_PARSE_ERROR;
							}
							r_index += 2;
							char32_t trail = 0;
							for (int j = 0; j < 4; j++) {
								char32_t c = p_str[r_index + j + 1];
								if (c == 0) {
									r_err_str = "Unterminated string";
									return ERR_PARSE_ERROR;
								}
								if (!is_hex_digit(c)) {
									r_err_str = "Malformed hex constant in string";
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

								trail <<= 4;
								trail |= v;
							}
							if ((trail & 0xfffffc00) == 0xdc00) {
								res = (res << 10UL) + trail - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
								r_index += 4; // will add at the end anyway
							}
							else {
								r_err_str =
									"Invalid UTF-16 sequence in string, unpaired lead surrogate";
								return ERR_PARSE_ERROR;
							}
						}
						else if ((res & 0xfffffc00) == 0xdc00) {
							r_err_str =
								"Invalid UTF-16 sequence in string, unpaired trail surrogate";
							return ERR_PARSE_ERROR;
						}

					} break;
					case '"':
					case '\\':
					case '/': {
						res = next;
					} break;
					default: {
						r_err_str = "Invalid escape sequence";
						return ERR_PARSE_ERROR;
					}
					}

					str += res;

				}
				else {
					if (p_str[r_index] == '\n') {
						r_line++;
					}
					str += p_str[r_index];
				}
				r_index++;
			}

			r_token.type = TK_STRING;
			return OK;

		} break;
		default: {
			if (p_str[r_index] <= 32) {
				r_index++;
				break;
			}

			if (p_str[r_index] == '-' || is_digit(p_str[r_index])) {
				// a number
				const char32_t* rptr;
				double number = String::to_float(&p_str[r_index], &rptr);
				r_index += (rptr - &p_str[r_index]);
				r_token.type = TK_NUMBER;
				return OK;

			}
			else if (is_ascii_alphabet_char(p_str[r_index])) {
				String id;

				while (is_ascii_alphabet_char(p_str[r_index])) {
					id += p_str[r_index];
					r_index++;
				}

				r_token.type = TK_IDENTIFIER;
				return OK;
			}
			else {
				r_err_str = "Unexpected character";
				return ERR_PARSE_ERROR;
			}
		}
		}
	}

	r_err_str = "Unknown error getting token";
	return ERR_PARSE_ERROR;
}

String JSON::get_parsed_text() const { return text; }



Error JSON::parse(String const&, bool) {}

// ==========================================================================
// json_resource_format.cpp
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

#include "core/config/engine.h"
#include "core/io/file_access.h"
#include "core/io/config_file.h"

Ref<Resource> ResourceFormatLoaderJSON::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	if (!FileAccess::exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		return Ref<Resource>();
	}

	Ref<JSON> json;
	json.instantiate();

	Error err = json->parse(
		FileAccess::get_file_as_string(p_path), Engine::is_editor_hint());
	if (err != OK) {
		String err_text = "Error parsing JSON file at '" + p_path + "', on line " +
						  itos(json->get_error_line()) + ": " + json->get_error_message();

		if (Engine::is_editor_hint()) {
			// If running on editor, still allow opening the JSON so the code editor can edit it.
			WARN_PRINT(err_text);
		}
		else {
			if (r_error) {
				*r_error = err;
			}
			ERR_PRINT(err_text);
			return Ref<Resource>();
		}
	}

	if (r_error) {
		*r_error = OK;
	}

	return json;
}

void ResourceFormatLoaderJSON::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("json");
}

bool ResourceFormatLoaderJSON::handles_type(const String& p_type) const
{
	return (p_type == "JSON");
}

String ResourceFormatLoaderJSON::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("json")) {
		return "JSON";
	}
	return "";
}

Error ResourceFormatSaverJSON::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Ref<JSON> json = p_resource;
	ERR_FAIL_COND_V(json.is_null(), ERR_INVALID_PARAMETER);

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	ERR_FAIL_COND_V_MSG(err, err, vformat("Cannot save json '%s'.", p_path));

	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void ResourceFormatSaverJSON::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	Ref<JSON> json = p_resource;
	if (json.is_valid()) {
		p_extensions->push_back("json");
	}
}

bool ResourceFormatSaverJSON::recognize(const Ref<Resource>& p_resource) const
{
	return false;
}

// ==========================================================================
// plist.cpp
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
#include "core/os/time.h"
#include "core/io/config_file.h"

PList::PLNodeType PListNode::get_type() const { return data_type; }

Ref<PListNode> PListNode::new_array()
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_ARRAY;
	return node;
}

Ref<PListNode> PListNode::new_dict()
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_DICT;
	return node;
}

Ref<PListNode> PListNode::new_string(const String& p_string)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_STRING;
	node->data_string = p_string.utf8();
	return node;
}

Ref<PListNode> PListNode::new_data(const String& p_string)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_DATA;
	node->data_string = p_string.utf8();
	return node;
}

Ref<PListNode> PListNode::new_date(const String& p_string)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_DATE;
	node->data_string = p_string.utf8();
	node->data_real =
		(double)Time::get_unix_time_from_datetime_string(p_string) - 978307200.0;
	return node;
}

Ref<PListNode> PListNode::new_bool(bool p_bool)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_BOOLEAN;
	node->data_bool = p_bool;
	return node;
}

Ref<PListNode> PListNode::new_int(int64_t p_int)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_INTEGER;
	node->data_int = p_int;
	return node;
}

Ref<PListNode> PListNode::new_real(double p_real)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_REAL;
	node->data_real = p_real;
	return node;
}

bool PListNode::push_subnode(const Ref<PListNode>& p_node, const String& p_key)
{
	ERR_FAIL_COND_V(p_node.is_null(), false);
	if (data_type == PList::PLNodeType::PL_NODE_TYPE_DICT) {
		ERR_FAIL_COND_V(p_key.is_empty(), false);
		ERR_FAIL_COND_V(data_dict.has(p_key), false);
		data_dict[p_key] = p_node;
		return true;
	}
	else if (data_type == PList::PLNodeType::PL_NODE_TYPE_ARRAY) {
		data_array.push_back(p_node);
		return true;
	}
	else {
		ERR_FAIL_V_MSG(false, "PList: Invalid parent node type, should be DICT or ARRAY.");
	}
}

size_t PListNode::get_asn1_size(uint8_t p_len_octets) const
{
	// Get size of all data, excluding type and size information.
	switch (data_type) {
	case PList::PLNodeType::PL_NODE_TYPE_NIL: {
		return 0;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATA:
	case PList::PLNodeType::PL_NODE_TYPE_DATE: {
		ERR_FAIL_V_MSG(0, "PList: DATE and DATA nodes are not supported by ASN.1 serialization.");
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_STRING: {
		return data_string.length();
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_BOOLEAN: {
		return 1;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_INTEGER:
	case PList::PLNodeType::PL_NODE_TYPE_REAL: {
		return 4;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_ARRAY: {
		size_t size = 0;
		for (int i = 0; i < data_array.size(); i++) {
			size += 1 + _asn1_size_len(p_len_octets) + data_array[i]->get_asn1_size(p_len_octets);
		}
		return size;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DICT: {
		size_t size = 0;

		for (const KeyValue<String, Ref<PListNode>>& E : data_dict) {
			size += 1 + _asn1_size_len(p_len_octets);						  // Sequence.
			size += 1 + _asn1_size_len(p_len_octets) + E.key.utf8().length(); // Key.
			size +=
				1 + _asn1_size_len(p_len_octets) + E.value->get_asn1_size(p_len_octets); // Value.
		}
		return size;
	} break;
	default: {
		return 0;
	} break;
	}
}

int PListNode::_asn1_size_len(uint8_t p_len_octets)
{
	if (p_len_octets > 1) {
		return p_len_octets + 1;
	}
	else {
		return 1;
	}
}

void PListNode::store_asn1_size(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const
{
	uint32_t size = get_asn1_size(p_len_octets);
	if (p_len_octets > 1) {
		p_stream.push_back(0x80 + p_len_octets);
	}
	for (int i = p_len_octets - 1; i >= 0; i--) {
		uint8_t x = (size >> i * 8) & 0xFF;
		p_stream.push_back(x);
	}
}

bool PListNode::store_asn1(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const
{
	// Convert to binary ASN1 stream.
	bool valid = true;
	switch (data_type) {
	case PList::PLNodeType::PL_NODE_TYPE_NIL: {
		// Nothing to store.
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATE:
	case PList::PLNodeType::PL_NODE_TYPE_DATA: {
		ERR_FAIL_V_MSG(
			false, "PList: DATE and DATA nodes are not supported by ASN.1 serialization.");
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_STRING: {
		p_stream.push_back(0x0C);
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 0; i < data_string.size(); i++) {
			p_stream.push_back(data_string[i]);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_BOOLEAN: {
		p_stream.push_back(0x01);
		store_asn1_size(p_stream, p_len_octets);
		if (data_bool) {
			p_stream.push_back(0x01);
		}
		else {
			p_stream.push_back(0x00);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_INTEGER: {
		p_stream.push_back(0x02);
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 4; i >= 0; i--) {
			uint8_t x = (data_int >> i * 8) & 0xFF;
			p_stream.push_back(x);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_REAL: {
		p_stream.push_back(0x03);
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 4; i >= 0; i--) {
			uint8_t x = (data_int >> i * 8) & 0xFF;
			p_stream.push_back(x);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_ARRAY: {
		p_stream.push_back(0x30); // Sequence.
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 0; i < data_array.size(); i++) {
			valid = valid && data_array[i]->store_asn1(p_stream, p_len_octets);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DICT: {
		p_stream.push_back(0x31); // Set.
		store_asn1_size(p_stream, p_len_octets);
		for (const KeyValue<String, Ref<PListNode>>& E : data_dict) {
			CharString cs = E.key.utf8();
			uint32_t size = cs.length();

			// Sequence.
			p_stream.push_back(0x30);
			uint32_t seq_size = 2 * (1 + _asn1_size_len(p_len_octets)) + size +
								E.value->get_asn1_size(p_len_octets);
			if (p_len_octets > 1) {
				p_stream.push_back(0x80 + p_len_octets);
			}
			for (int i = p_len_octets - 1; i >= 0; i--) {
				uint8_t x = (seq_size >> i * 8) & 0xFF;
				p_stream.push_back(x);
			}
			// Key.
			p_stream.push_back(0x0C);
			if (p_len_octets > 1) {
				p_stream.push_back(0x80 + p_len_octets);
			}
			for (int i = p_len_octets - 1; i >= 0; i--) {
				uint8_t x = (size >> i * 8) & 0xFF;
				p_stream.push_back(x);
			}
			for (uint32_t i = 0; i < size; i++) {
				p_stream.push_back(cs[i]);
			}
			// Value.
			valid = valid && E.value->store_asn1(p_stream, p_len_octets);
		}
	} break;
	}
	return valid;
}

void PListNode::store_text(String& p_stream, uint8_t p_indent) const
{
	// Convert to text XML stream.
	switch (data_type) {
	case PList::PLNodeType::PL_NODE_TYPE_NIL: {
		// Nothing to store.
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATA: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<data>\n";
		p_stream += String("\t").repeat(p_indent);
		// Data should be Base64 (i.e. ASCII only).
		p_stream += String::ascii(data_string) + "\n";
		p_stream += String("\t").repeat(p_indent);
		p_stream += "</data>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATE: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<date>";
		// Data should be ISO 8601 (i.e. ASCII only).
		p_stream += String::ascii(data_string);
		p_stream += "</date>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_STRING: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<string>";
		p_stream += String::utf8(data_string);
		p_stream += "</string>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_BOOLEAN: {
		p_stream += String("\t").repeat(p_indent);
		if (data_bool) {
			p_stream += "<true/>\n";
		}
		else {
			p_stream += "<false/>\n";
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_INTEGER: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<integer>";
		p_stream += itos(data_int);
		p_stream += "</integer>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_REAL: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<real>";
		p_stream += rtos(data_real);
		p_stream += "</real>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_ARRAY: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<array>\n";
		for (int i = 0; i < data_array.size(); i++) {
			data_array[i]->store_text(p_stream, p_indent + 1);
		}
		p_stream += String("\t").repeat(p_indent);
		p_stream += "</array>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DICT: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<dict>\n";
		for (const KeyValue<String, Ref<PListNode>>& E : data_dict) {
			p_stream += String("\t").repeat(p_indent + 1);
			p_stream += "<key>";
			p_stream += E.key;
			p_stream += "</key>\n";
			E.value->store_text(p_stream, p_indent + 1);
		}
		p_stream += String("\t").repeat(p_indent);
		p_stream += "</dict>\n";
	} break;
	}
}

/*************************************************************************/

PList::PList() { root = PListNode::new_dict(); }

PList::PList(const String& p_string)
{
	String err_str;
	bool ok = load_string(p_string, err_str);
	ERR_FAIL_COND_MSG(!ok, vformat("PList: %s.", err_str));
}

uint64_t PList::read_bplist_var_size_int(Ref<FileAccess> p_file, uint8_t p_size)
{
	uint64_t pos = p_file->get_position();
	uint64_t ret = 0;
	switch (p_size) {
	case 1: {
		ret = p_file->get_8();
	} break;
	case 2: {
		ret = BSWAP16(p_file->get_16());
	} break;
	case 3: {
		ret = BSWAP32(p_file->get_32() & 0x00FFFFFF);
	} break;
	case 4: {
		ret = BSWAP32(p_file->get_32());
	} break;
	case 5: {
		ret = BSWAP64(p_file->get_64() & 0x000000FFFFFFFFFF);
	} break;
	case 6: {
		ret = BSWAP64(p_file->get_64() & 0x0000FFFFFFFFFFFF);
	} break;
	case 7: {
		ret = BSWAP64(p_file->get_64() & 0x00FFFFFFFFFFFFFF);
	} break;
	case 8: {
		ret = BSWAP64(p_file->get_64());
	} break;
	default: {
		ret = 0;
	}
	}
	p_file->seek(pos + p_size);

	return ret;
}

Ref<PListNode> PList::read_bplist_obj(Ref<FileAccess> p_file, uint64_t p_offset_idx)
{
	Ref<PListNode> node;
	node.instantiate();

	uint64_t ot_off = trailer.offset_table_start + p_offset_idx * trailer.offset_size;
	p_file->seek(ot_off);
	uint64_t marker_off = read_bplist_var_size_int(p_file, trailer.offset_size);
	ERR_FAIL_COND_V_MSG(marker_off == 0, Ref<PListNode>(), "Invalid marker size.");

	p_file->seek(marker_off);
	uint8_t marker = p_file->get_8();
	uint8_t marker_type = marker & 0xF0;
	uint64_t marker_size = marker & 0x0F;

	switch (marker_type) {
	case 0x00: {
		if (marker_size == 0x00) {
			node->data_type = PL_NODE_TYPE_NIL;
		}
		else if (marker_size == 0x08) {
			node->data_type = PL_NODE_TYPE_BOOLEAN;
			node->data_bool = false;
		}
		else if (marker_size == 0x09) {
			node->data_type = PL_NODE_TYPE_BOOLEAN;
			node->data_bool = true;
		}
		else {
			ERR_FAIL_V_MSG(Ref<PListNode>(), "Invalid nil/bool marker value.");
		}
	} break;
	case 0x10: {
		node->data_type = PL_NODE_TYPE_INTEGER;
		node->data_int =
			static_cast<int64_t>(read_bplist_var_size_int(p_file, std::pow(2, marker_size)));
	} break;
	case 0x20: {
		node->data_type = PL_NODE_TYPE_REAL;
		node->data_int =
			static_cast<int64_t>(read_bplist_var_size_int(p_file, std::pow(2, marker_size)));
	} break;
	case 0x30: {
		node->data_type = PL_NODE_TYPE_DATE;
		node->data_int = BSWAP64(p_file->get_64());
		node->data_string =
			Time::get_datetime_string_from_unix_time(node->data_real + 978307200.0).utf8();
	} break;
	case 0x40: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		node->data_type = PL_NODE_TYPE_DATA;
		Vector<uint8_t> buf;
		buf.resize(marker_size + 1);
		p_file->get_buffer(reinterpret_cast<uint8_t*>(buf.ptrw()), marker_size);
		node->data_string = CryptoCore::b64_encode_str(buf.ptr(), buf.size()).utf8();
	} break;
	case 0x50: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		node->data_type = PL_NODE_TYPE_STRING;
		node->data_string.resize_uninitialized(marker_size + 1);
		p_file->get_buffer(reinterpret_cast<uint8_t*>(node->data_string.ptrw()), marker_size);
	} break;
	case 0x60: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		Char16String cs16;
		cs16.resize_uninitialized(marker_size + 1);
		for (uint64_t i = 0; i < marker_size; i++) {
			cs16[i] = BSWAP16(p_file->get_16());
		}
		node->data_type = PL_NODE_TYPE_STRING;
		node->data_string = String::utf16(cs16.ptr(), cs16.length()).utf8();
	} break;
	case 0x80: {
		node->data_type = PL_NODE_TYPE_INTEGER;
		node->data_int = static_cast<int64_t>(read_bplist_var_size_int(p_file, marker_size + 1));
	} break;
	case 0xA0:
	case 0xC0: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		uint64_t pos = p_file->get_position();

		node->data_type = PL_NODE_TYPE_ARRAY;
		for (uint64_t i = 0; i < marker_size; i++) {
			p_file->seek(pos + trailer.ref_size * i);
			uint64_t ref = read_bplist_var_size_int(p_file, trailer.ref_size);

			Ref<PListNode> element = read_bplist_obj(p_file, ref);
			ERR_FAIL_COND_V(element.is_null(), Ref<PListNode>());
			node->data_array.push_back(element);
		}
	} break;
	case 0xD0: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		uint64_t pos = p_file->get_position();

		node->data_type = PL_NODE_TYPE_DICT;
		for (uint64_t i = 0; i < marker_size; i++) {
			p_file->seek(pos + trailer.ref_size * i);
			uint64_t key_ref = read_bplist_var_size_int(p_file, trailer.ref_size);

			p_file->seek(pos + trailer.ref_size * (i + marker_size));
			uint64_t obj_ref = read_bplist_var_size_int(p_file, trailer.ref_size);

			Ref<PListNode> element_key = read_bplist_obj(p_file, key_ref);
			ERR_FAIL_COND_V(element_key.is_null() || element_key->data_type != PL_NODE_TYPE_STRING,
				Ref<PListNode>());
			Ref<PListNode> element = read_bplist_obj(p_file, obj_ref);
			ERR_FAIL_COND_V(element.is_null(), Ref<PListNode>());
			node->data_dict[String::utf8(
				element_key->data_string.ptr(), element_key->data_string.length())] = element;
		}
	} break;
	default: {
		ERR_FAIL_V_MSG(Ref<PListNode>(), "Invalid marker type.");
	}
	}
	return node;
}

bool PList::load_file(const String& p_filename)
{
	root = Ref<PListNode>();

	Ref<FileAccess> fb = FileAccess::open(p_filename, FileAccess::READ);
	if (fb.is_null()) {
		return false;
	}

	unsigned char magic[8];
	fb->get_buffer(magic, 8);

	if (String::ascii(Span((const char*)magic, 8)) == "bplist00") {
		fb->seek_end(-26);
		trailer.offset_size = fb->get_8();
		trailer.ref_size = fb->get_8();
		trailer.object_num = BSWAP64(fb->get_64());
		trailer.root_offset_idx = BSWAP64(fb->get_64());
		trailer.offset_table_start = BSWAP64(fb->get_64());
		root = read_bplist_obj(fb, trailer.root_offset_idx);

		return root.is_valid();
	}
	else {
		// Load text plist.
		Error err;
		Vector<uint8_t> array = FileAccess::get_file_as_bytes(p_filename, &err);
		ERR_FAIL_COND_V(err != OK, false);

		String err_str;
		bool ok = load_string(String::utf8((const char*)array.ptr(), array.size()), err_str);
		ERR_FAIL_COND_V_MSG(!ok, false, "PList: " + err_str);

		return true;
	}
}

bool PList::load_string(const String& p_string, String& r_err_out)
{
	root = Ref<PListNode>();

	int pos = 0;
	bool in_plist = false;
	bool done_plist = false;
	List<Ref<PListNode>> stack;
	String key;
	while (pos >= 0) {
		int open_token_s = p_string.find_char('<', pos);
		if (open_token_s == -1) {
			r_err_out = "Unexpected end of data. No tags found.";
			return false;
		}
		int open_token_e = p_string.find_char('>', open_token_s);
		pos = open_token_e;

		String token = p_string.substr(open_token_s + 1, open_token_e - open_token_s - 1);
		if (token.is_empty()) {
			r_err_out = "Invalid token name.";
			return false;
		}
		String value;
		if (token[0] == '?' || token[0] == '!') { // Skip <?xml ... ?> and <!DOCTYPE ... >
			int end_token_e = p_string.find_char('>', open_token_s);
			pos = end_token_e;
			continue;
		}

		if (token.find("plist", 0) == 0) {
			in_plist = true;
			continue;
		}

		if (token == "/plist") {
			done_plist = true;
			break;
		}

		if (!in_plist) {
			r_err_out = "Node outside of <plist> tag.";
			return false;
		}

		if (token == "dict") {
			if (!stack.is_empty()) {
				// Add subnode end enter it.
				Ref<PListNode> dict = PListNode::new_dict();
				dict->data_type = PList::PLNodeType::PL_NODE_TYPE_DICT;
				if (!stack.back()->get()->push_subnode(dict, key)) {
					r_err_out = "Can't push subnode, invalid parent type.";
					return false;
				}
				stack.push_back(dict);
			}
			else {
				// Add root node.
				if (root.is_valid()) {
					r_err_out = "Root node already set.";
					return false;
				}
				Ref<PListNode> dict = PListNode::new_dict();
				stack.push_back(dict);
				root = dict;
			}
			continue;
		}

		if (token == "/dict") {
			// Exit current dict.
			if (stack.is_empty() ||
				stack.back()->get()->data_type != PList::PLNodeType::PL_NODE_TYPE_DICT) {
				r_err_out = "Mismatched </dict> tag.";
				return false;
			}
			stack.pop_back();
			continue;
		}

		if (token == "array") {
			if (!stack.is_empty()) {
				// Add subnode end enter it.
				Ref<PListNode> arr = PListNode::new_array();
				if (!stack.back()->get()->push_subnode(arr, key)) {
					r_err_out
= "Can't push subnode, invalid parent type.";
					return false;
				}
				stack.push_back(arr);
			}
			else {
				// Add root node.
				if (root.is_valid()) {
					r_err_out = "Root node already set.";
					return false;
				}
				Ref<PListNode> arr = PListNode::new_array();
				stack.push_back(arr);
				root = arr;
			}
			continue;
		}

		if (token == "/array") {
			// Exit current array.
			if (stack.is_empty() ||
				stack.back()->get()->data_type != PList::PLNodeType::PL_NODE_TYPE_ARRAY) {
				r_err_out = "Mismatched </array> tag.";
				return false;
			}
			stack.pop_back();
			continue;
		}

		if (token[token.length() - 1] == '/') {
			token = token.substr(0, token.length() - 1);
		}
		else {
			int end_token_s = p_string.find("</", pos);
			if (end_token_s == -1) {
				r_err_out = vformat("Mismatched <%s> tag.", token);
				return false;
			}
			int end_token_e = p_string.find_char('>', end_token_s);
			pos = end_token_e;
			String end_token = p_string.substr(end_token_s + 2, end_token_e - end_token_s - 2);
			if (end_token != token) {
				r_err_out = vformat("Mismatched <%s> and <%s> token pair.", token, end_token);
				return false;
			}
			value = p_string.substr(open_token_e + 1, end_token_s - open_token_e - 1);
		}
		if (token == "key") {
			key = value;
		}
		else {
			Ref<PListNode> var = nullptr;
			if (token == "true") {
				var = PListNode::new_bool(true);
			}
			else if (token == "false") {
				var = PListNode::new_bool(false);
			}
			else if (token == "integer") {
				var = PListNode::new_int(value.to_int());
			}
			else if (token == "real") {
				var = PListNode::new_real(value.to_float());
			}
			else if (token == "string") {
				var = PListNode::new_string(value);
			}
			else if (token == "data") {
				var = PListNode::new_data(value);
			}
			else if (token == "date") {
				var = PListNode::new_date(value);
			}
			else {
				r_err_out = vformat("Invalid value type: %s.", token);
				return false;
			}
			if (stack.is_empty() || !stack.back()->get()->push_subnode(var, key)) {
				r_err_out = "Can't push subnode, invalid parent type.";
				return false;
			}
		}
	}
	if (!stack.is_empty() || !done_plist) {
		r_err_out = "Unexpected end of data. Root node is not closed.";
		return false;
	}
	return true;
}

Vector<uint8_t> PList::save_asn1() const
{
	if (root.is_null()) {
		ERR_FAIL_V_MSG(Vector<uint8_t>(), "PList: Invalid PList, no root node.");
	}
	size_t size = root->get_asn1_size(1);
	uint8_t len_octets = 0;
	if (size < 0x80) {
		len_octets = 1;
	}
	else {
		size = root->get_asn1_size(2);
		if (size < 0xFFFF) {
			len_octets = 2;
		}
		else {
			size = root->get_asn1_size(3);
			if (size < 0xFFFFFF) {
				len_octets = 3;
			}
			else {
				size = root->get_asn1_size(4);
				if (size < 0xFFFFFFFF) {
					len_octets = 4;
				}
				else {
					ERR_FAIL_V_MSG(Vector<uint8_t>(),
						"PList: Data is too big for ASN.1 serializer, should be < 4 GiB.");
				}
			}
		}
	}

	Vector<uint8_t> ret;
	if (!root->store_asn1(ret, len_octets)) {
		ERR_FAIL_V_MSG(Vector<uint8_t>(), "PList: ASN.1 serializer error.");
	}
	return ret;
}

String PList::save_text() const
{
	if (root.is_null()) {
		ERR_FAIL_V_MSG(String(), "PList: Invalid PList, no root node.");
	}

	String ret;
	ret += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	ret += "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
		   "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
	ret += "<plist version=\"1.0\">\n";

	root->store_text(ret, 0);

	ret += "</plist>\n\n";
	return ret;
}

Ref<PListNode> PList::get_root() { return root; }

// ==========================================================================
// xml_parser.cpp
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

#include "core/io/file_access.h"
#include "core/io/config_file.h"

static inline bool _is_white_space(char p_char)
{
	return (p_char == ' ' || p_char == '\t' || p_char == '\n' || p_char == '\r');
}

//! sets the state that text was found. Returns true if set should be set
bool XMLParser::_set_text(const char* p_start, const char* p_end)
{
	// check if text is more than 2 characters, and if not, check if there is
	// only white space, so that this text won't be reported
	if (p_end - p_start < 3) {
		const char* p = p_start;
		for (; p != p_end; ++p) {
			if (!_is_white_space(*p)) {
				break;
			}
		}

		if (p == p_end) {
			return false;
		}
	}

	// set current text to the parsed text, and replace xml special characters
	String s = String::utf8(p_start, (int)(p_end - p_start));
	node_name = s.xml_unescape();

	// current XML node type is text
	node_type = NODE_TEXT;

	return true;
}

void XMLParser::_parse_closing_xml_element()
{
	node_type = NODE_ELEMENT_END;
	node_empty = false;
	attributes.clear();

	next_char();
	const char* pBeginClose = P;

	while (*P && *P != '>') {
		next_char();
	}

	node_name = String::utf8(pBeginClose, (int)(P - pBeginClose));
#ifdef DEBUG_XML
	print_line("XML CLOSE: " + node_name);
#endif

	if (*P) {
		next_char();
	}
}

void XMLParser::_ignore_definition()
{
	node_type = NODE_UNKNOWN;

	const char* F = P;
	// move until end marked with '>' reached
	while (*P && *P != '>') {
		next_char();
	}
	node_name.clear();
	node_name.append_utf8(F, P - F);

	if (*P) {
		next_char();
	}
}

bool XMLParser::_parse_cdata()
{
	if (*(P + 1) != '[') {
		return false;
	}

	node_type = NODE_CDATA;

	// skip '<![CDATA['
	int count = 0;
	while (*P && count < 8) {
		next_char();
		++count;
	}

	if (!*P) {
		node_name = "";
		return true;
	}

	const char* cDataBegin = P;
	const char* cDataEnd = nullptr;

	// find end of CDATA
	while (*P && !cDataEnd) {
		if (*P == '>' && (*(P - 1) == ']') && (*(P - 2) == ']')) {
			cDataEnd = P - 2;
		}

		next_char();
	}

	if (!cDataEnd) {
		cDataEnd = P;
	}
	node_name = String::utf8(cDataBegin, (int)(cDataEnd - cDataBegin));
#ifdef DEBUG_XML
	print_line("XML CDATA: " + node_name);
#endif

	return true;
}

void XMLParser::_parse_comment()
{
	node_type = NODE_COMMENT;
	P += 1;

	const char* pEndOfInput = data + length;
	const char* pCommentBegin;
	const char* pCommentEnd;

	if (P + 1 < pEndOfInput && P[0] == '-' && P[1] == '-') {
		// Comment, use '-->' as end.
		pCommentBegin = P + 2;
		for (pCommentEnd = pCommentBegin; pCommentEnd + 2 < pEndOfInput; pCommentEnd++) {
			if (pCommentEnd[0] == '-' && pCommentEnd[1] == '-' && pCommentEnd[2] == '>') {
				break;
			}
		}
		if (pCommentEnd + 2 < pEndOfInput) {
			P = pCommentEnd + 3;
		}
		else {
			P = pCommentEnd = pEndOfInput;
		}
	}
	else {
		// Like document type definition, match angle brackets.
		pCommentBegin = P;

		int count = 1;
		while (*P && count) {
			if (*P == '>') {
				--count;
			}
			else if (*P == '<') {
				++count;
			}
			next_char();
		}

		if (count) {
			pCommentEnd = P;
		}
		else {
			pCommentEnd = P - 1;
		}
	}

	node_name = String::utf8(pCommentBegin, (int)(pCommentEnd - pCommentBegin));
#ifdef DEBUG_XML
	print_line("XML COMMENT: " + node_name);
#endif
}

void XMLParser::_parse_opening_xml_element()
{
	node_type = NODE_ELEMENT;
	node_empty = false;
	attributes.clear();

	// find name
	const char* startName = P;

	// find end of element
	while (*P && *P != '>' && !_is_white_space(*P)) {
		next_char();
	}

	const char* endName = P;

	// find attributes
	while (*P && *P != '>') {
		if (_is_white_space(*P)) {
			next_char();
		}
		else {
			if (*P != '/') {
				// we've got an attribute

				// read the attribute names
				const char* attributeNameBegin = P;

				while (*P && !_is_white_space(*P) && *P != '=') {
					next_char();
				}

				if (!*P) {
					break;
				}

				const char* attributeNameEnd = P;
				next_char();

				// read the attribute value
				// check for quotes and single quotes, thx to murphy
				while ((*P != '\"') && (*P != '\'') && *P) {
					next_char();
				}

				if (!*P) { // malformatted xml file
					break;
				}

				const char attributeQuoteChar = *P;

				next_char();
				const char* attributeValueBegin = P;

				while (*P != attributeQuoteChar && *P) {
					next_char();
				}

				const char* attributeValueEnd = P;
				if (*P) {
					next_char();
				}

				Attribute attr;
				attr.name =
					String::utf8(attributeNameBegin, (int)(attributeNameEnd - attributeNameBegin));

				String s = String::utf8(
					attributeValueBegin, (int)(attributeValueEnd - attributeValueBegin));

				attr.value = s.xml_unescape();
				attributes.push_back(attr);
			}
			else {
				// tag is closed directly
				next_char();
				node_empty = true;
				break;
			}
		}
	}

	// check if this tag is closing directly
	if (endName > startName && *(endName - 1) == '/') {
		// directly closing tag
		node_empty = true;
		endName--;
	}

	node_name = String::utf8(startName, (int)(endName - startName));
#ifdef DEBUG_XML
	print_line("XML OPEN: " + node_name);
#endif

	if (*P) {
		next_char();
	}
}

void XMLParser::_parse_current_node()
{
	const char* start = P;
	node_offset = P - data;

	// more forward until '<' found
	while (*P != '<' && *P) {
		next_char();
	}

	if (P - start > 0) {
		// we found some text, store it
		if (_set_text(start, P)) {
			return;
		}
	}

	if (!*P) {
		return;
	}

	next_char();

	// based on current token, parse and report next element
	switch (*P) {
	case '/':
		_parse_closing_xml_element();
		break;
	case '?':
		_ignore_definition();
		break;
	case '!':
		if (!_parse_cdata()) {
			_parse_comment();
		}
		break;
	default:
		_parse_opening_xml_element();
		break;
	}
}

uint64_t XMLParser::get_node_offset() const { return node_offset; }

Error XMLParser::seek(uint64_t p_pos)
{
	ERR_FAIL_NULL_V(data, ERR_FILE_EOF);
	ERR_FAIL_COND_V(p_pos >= length, ERR_FILE_EOF);

	P = data + p_pos;

	return read();
}

Error XMLParser::read()
{
	// if end not reached, parse the node
	if (P && (P - data) < (int64_t)length - 1 && *P != 0) {
		_parse_current_node();
		return OK;
	}

	return ERR_FILE_EOF;
}

XMLParser::NodeType XMLParser::get_node_type() { return node_type; }

String XMLParser::get_node_data() const
{
	ERR_FAIL_COND_V(node_type != NODE_TEXT, "");
	return node_name;
}

String XMLParser::get_node_name() const
{
	ERR_FAIL_COND_V(node_type == NODE_TEXT, "");
	return node_name;
}

int XMLParser::get_attribute_count() const { return attributes.size(); }

String XMLParser::get_attribute_name(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, attributes.size(), "");
	return attributes[p_idx].name;
}

String XMLParser::get_attribute_value(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, attributes.size(), "");
	return attributes[p_idx].value;
}

bool XMLParser::has_attribute(const String& p_name) const
{
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == p_name) {
			return true;
		}
	}

	return false;
}

String XMLParser::get_named_attribute_value(const String& p_name) const
{
	int idx = -1;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == p_name) {
			idx = i;
			break;
		}
	}

	ERR_FAIL_COND_V_MSG(idx < 0, "", vformat("Attribute not found: '%s'.", p_name));

	return attributes[idx].value;
}

String XMLParser::get_named_attribute_value_safe(const String& p_name) const
{
	int idx = -1;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == p_name) {
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		return "";
	}
	return attributes[idx].value;
}

bool XMLParser::is_empty() const { return node_empty; }

Error XMLParser::open_buffer(const Vector<uint8_t>& p_buffer)
{
	ERR_FAIL_COND_V(p_buffer.is_empty(), ERR_INVALID_DATA);

	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}

	length = p_buffer.size();
	data_copy = memnew_arr(char, length + 1);
	memcpy(data_copy, p_buffer.ptr(), length);
	data_copy[length] = 0;
	data = data_copy;
	P = data;
	current_line = 0;

	return OK;
}

Error XMLParser::_open_buffer(const uint8_t* p_buffer, size_t p_size)
{
	ERR_FAIL_COND_V(p_size == 0, ERR_INVALID_DATA);
	ERR_FAIL_NULL_V(p_buffer, ERR_INVALID_DATA);

	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}

	length = p_size;
	data = (const char*)p_buffer;
	P = data;
	current_line = 0;

	return OK;
}

Error XMLParser::open(const String& p_path)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot open file '%s'.", p_path));

	length = file->get_length();
	ERR_FAIL_COND_V(length < 1, ERR_FILE_CORRUPT);

	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}

	data_copy = memnew_arr(char, length + 1);
	file->get_buffer((uint8_t*)data_copy, length);
	data_copy[length] = 0;
	data = data_copy;
	P = data;
	current_line = 0;

	return OK;
}

void XMLParser::skip_section()
{
	// skip if this element is empty anyway.
	if (is_empty()) {
		return;
	}

	// read until we've reached the last element in this section
	int tagcount = 1;

	while (tagcount && read() == OK) {
		if (get_node_type() == XMLParser::NODE_ELEMENT && !is_empty()) {
			++tagcount;
		}
		else if (get_node_type() == XMLParser::NODE_ELEMENT_END) {
			--tagcount;
		}
	}
}

void XMLParser::close()
{
	if (data_copy) {
		memdelete_arr(data);
		data_copy = nullptr;
	}
	data = nullptr;
	length = 0;
	P = nullptr;
	node_empty = false;
	node_type = NODE_NONE;
	node_offset = 0;
}

int XMLParser::get_current_line() const { return current_line; }

XMLParser::~XMLParser()
{
	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}
}

// ==========================================================================
// translation_loader_po.cpp
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

#include "core/io/config_file.h"

#include "core/io/file_access.h"
#include "core/string/translation_server.h"

Ref<Resource> TranslationLoaderPO::load_translation(Ref<FileAccess> r_file, Error *r_error) {
	if (r_error) {
		*r_error = ERR_FILE_CORRUPT;
	}

	const String path = r_file->get_path();
	Ref<Translation> translation;
	translation.instantiate();
	String config;

	uint32_t magic = r_file->get_32();
	if (magic == 0x950412de) {
		// Load binary MO file.

		uint16_t version_maj = r_file->get_16();
		uint16_t version_min = r_file->get_16();
		ERR_FAIL_COND_V_MSG(version_maj > 1, Ref<Resource>(), vformat("Unsupported MO file %s, version %d.%d.", path, version_maj, version_min));

		uint32_t num_strings = r_file->get_32();
		uint32_t id_table_offset = r_file->get_32();
		uint32_t trans_table_offset = r_file->get_32();

		// Read string tables.
		for (uint32_t i = 0; i < num_strings; i++) {
			String msg_id;
			String msg_id_plural;
			String msg_context;

			// Read id strings and context.
			{
				Vector<uint8_t> data;
				r_file->seek(id_table_offset + i * 8);
				uint32_t str_start = 0;
				uint32_t str_len = r_file->get_32();
				uint32_t str_offset = r_file->get_32();

				data.resize(str_len + 1);
				r_file->seek(str_offset);
				r_file->get_buffer(data.ptrw(), str_len);
				data.write[str_len] = 0;

				bool is_plural = false;
				for (uint32_t j = 0; j < str_len + 1; j++) {
					if (data[j] == 0x04) {
						msg_context.clear();
						msg_context.append_utf8((const char *)data.ptr(), j);
						str_start = j + 1;
					}
					if (data[j] == 0x00) {
						if (is_plural) {
							msg_id_plural.clear();
							msg_id_plural.append_utf8((const char *)(data.ptr() + str_start), j - str_start);
						} else {
							msg_id.clear();
							msg_id.append_utf8((const char *)(data.ptr() + str_start), j - str_start);
							is_plural = true;
						}
						str_start = j + 1;
					}
				}
			}

			// Read translated strings.
			{
				Vector<uint8_t> data;
				r_file->seek(trans_table_offset + i * 8);
				uint32_t str_len = r_file->get_32();
				uint32_t str_offset = r_file->get_32();

				data.resize(str_len + 1);
				r_file->seek(str_offset);
				r_file->get_buffer(data.ptrw(), str_len);
				data.write[str_len] = 0;

				if (msg_id.is_empty()) {
					config = String::utf8((const char *)data.ptr(), str_len);
					// Record plural rule.
					int p_start = config.find("Plural-Forms");
					if (p_start != -1) {
						int p_end = config.find_char('\n', p_start);
						translation->set_plural_rules_override(config.substr(p_start, p_end - p_start));
					}
				} else {
					uint32_t str_start = 0;
					Vector<String> plural_msg;
					for (uint32_t j = 0; j < str_len + 1; j++) {
						if (data[j] == 0x00) {
							if (msg_id_plural.is_empty()) {
								translation->add_message(msg_id, String::utf8((const char *)(data.ptr() + str_start), j - str_start), msg_context);
							} else {
								plural_msg.push_back(String::utf8((const char *)(data.ptr() + str_start), j - str_start));
							}
							str_start = j + 1;
						}
					}
					if (!plural_msg.is_empty()) {
						translation->add_plural_message(msg_id, plural_msg, msg_context);
					}
				}
			}
		}

	} else {
		// Try to load as text PO file.
		r_file->seek(0);

		enum Status {
			STATUS_NONE,
			STATUS_READING_ID,
			STATUS_READING_STRING,
			STATUS_READING_CONTEXT,
			STATUS_READING_PLURAL,
		};

		Status status = STATUS_NONE;

		String msg_id;
		String msg_str;
		String msg_context;
		Vector<String> msgs_plural;

		if (r_error) {
			*r_error = ERR_FILE_CORRUPT;
		}

		int line = 1;
		int plural_forms = 0;
		int plural_index = -1;
		bool entered_context = false;
		bool skip_this = false;
		bool skip_next = false;
		bool is_eof = false;

		while (!is_eof) {
			String l = r_file->get_line().strip_edges();
			is_eof = r_file->eof_reached();

			// If we reached last line and it's not a content line, break, otherwise let processing that last loop
			if (is_eof && l.is_empty()) {
				if (status == STATUS_READING_ID || status == STATUS_READING_CONTEXT || (status == STATUS_READING_PLURAL && plural_index != plural_forms - 1)) {
					ERR_FAIL_V_MSG(Ref<Resource>(), vformat("Unexpected EOF while reading PO file at: %s:%d.", path, line));
				} else {
					break;
				}
			}

			if (l.begins_with("msgctxt")) {
				ERR_FAIL_COND_V_MSG(status != STATUS_READING_STRING && status != STATUS_READING_PLURAL, Ref<Resource>(), vformat("Unexpected 'msgctxt', was expecting 'msgid_plural' or 'msgstr' before 'msgctxt' while parsing: %s:%d.", path, line));

				// In PO file, "msgctxt" appears before "msgid". If we encounter a "msgctxt", we add what we have read
				// and set "entered_context" to true to prevent adding twice.
				if (!skip_this && !msg_id.is_empty()) {
					if (status == STATUS_READING_STRING) {
						translation->add_message(msg_id, msg_str, msg_context);
					} else if (status == STATUS_READING_PLURAL) {
						ERR_FAIL_COND_V_MSG(plural_index != plural_forms - 1, Ref<Resource>(), vformat("Number of 'msgstr[]' doesn't match with number of plural forms: %s:%d.", path, line));
						translation->add_plural_message(msg_id, msgs_plural, msg_context);
					}
				}
				msg_context = "";
				l = l.substr(7).strip_edges();
				status = STATUS_READING_CONTEXT;
				entered_context = true;
			}

			if (l.begins_with("msgid_plural")) {
				if (plural_forms == 0) {
					ERR_FAIL_V_MSG(Ref<Resource>(), vformat("PO file uses 'msgid_plural' but 'Plural-Forms' is invalid or missing in header: %s:%d.", path, line));
				} else if (status != STATUS_READING_ID) {
					ERR_FAIL_V_MSG(Ref<Resource>(), vformat("Unexpected 'msgid_plural', was expecting 'msgid' before 'msgid_plural' while parsing: %s:%d.", path, line));
				}
				// We don't record the message in "msgid_plural" itself as tr_n(), TTRN(), RTRN() interfaces provide the plural string already.
				// We just have to reset variables related to plurals for "msgstr[]" later on.
				l = l.substr(12).strip_edges();
				plural_index = -1;
				msgs_plural.clear();
				msgs_plural.resize(plural_forms);
				status = STATUS_READING_PLURAL;
			} else if (l.begins_with("msgid")) {
				ERR_FAIL_COND_V_MSG(status == STATUS_READING_ID, Ref<Resource>(), vformat("Unexpected 'msgid', was expecting 'msgstr' while parsing: %s:%d.", path, line));

				if (!msg_id.is_empty()) {
					if (!skip_this && !entered_context) {
						if (status == STATUS_READING_STRING) {
							translation->add_message(msg_id, msg_str, msg_context);
						} else if (status == STATUS_READING_PLURAL) {
							ERR_FAIL_COND_V_MSG(plural_index != plural_forms - 1, Ref<Resource>(), vformat("Number of 'msgstr[]' doesn't match with number of plural forms: %s:%d.", path, line));
							translation->add_plural_message(msg_id, msgs_plural, msg_context);
						}
					}
				} else if (config.is_empty()) {
					config = msg_str;
					// Record plural rule.
					int p_start = config.find("Plural-Forms");
					if (p_start != -1) {
						int p_end = config.find_char('\n', p_start);
						translation->set_plural_rules_override(config.substr(p_start, p_end - p_start));
						plural_forms = translation->get_nplurals();
					}
				}

				l = l.substr(5).strip_edges();
				status = STATUS_READING_ID;
				// If we did not encounter msgctxt, we reset context to empty to reset it.
				if (!entered_context) {
					msg_context = "";
				}
				msg_id = "";
				msg_str = "";
				skip_this = skip_next;
				skip_next = false;
				entered_context = false;
			}

			if (l.begins_with("msgstr[")) {
				ERR_FAIL_COND_V_MSG(status != STATUS_READING_PLURAL, Ref<Resource>(), vformat("Unexpected 'msgstr[]', was expecting 'msgid_plural' before 'msgstr[]' while parsing: %s:%d.", path, line));
				plural_index++; // Increment to add to the next slot in vector msgs_plural.
				l = l.substr(9).strip_edges();
			} else if (l.begins_with("msgstr")) {
				ERR_FAIL_COND_V_MSG(status != STATUS_READING_ID, Ref<Resource>(), vformat("Unexpected 'msgstr', was expecting 'msgid' before 'msgstr' while parsing: %s:%d.", path, line));
				l = l.substr(6).strip_edges();
				status = STATUS_READING_STRING;
			}

			if (l.is_empty() || l.begins_with("#")) {
				if (l.contains("fuzzy")) {
					skip_next = true;
				}
				line++;
				continue; // Nothing to read or comment.
			}

			ERR_FAIL_COND_V_MSG(!l.begins_with("\"") || status == STATUS_NONE, Ref<Resource>(), vformat("Invalid line '%s' while parsing: %s:%d.", l, path, line));

			l = l.substr(1);
			// Find final quote, ignoring escaped ones (\").
			// The escape_next logic is necessary to properly parse things like \\"
			// where the backslash is the one being escaped, not the quote.
			int end_pos = -1;
			bool escape_next = false;
			for (int i = 0; i < l.length(); i++) {
				if (l[i] == '\\' && !escape_next) {
					escape_next = true;
					continue;
				}

				if (l[i] == '"' && !escape_next) {
					end_pos = i;
					break;
				}

				escape_next = false;
			}

			ERR_FAIL_COND_V_MSG(end_pos == -1, Ref<Resource>(), vformat("Expected '\"' at end of message while parsing: %s:%d.", path, line));

			l = l.substr(0, end_pos);
			l = l.c_unescape();

			if (status == STATUS_READING_ID) {
				msg_id += l;
			} else if (status == STATUS_READING_STRING) {
				msg_str += l;
			} else if (status == STATUS_READING_CONTEXT) {
				msg_context += l;
			} else if (status == STATUS_READING_PLURAL && plural_index >= 0) {
				ERR_FAIL_COND_V_MSG(plural_index >= plural_forms, Ref<Resource>(), vformat("Unexpected plural form while parsing: %s:%d.", path, line));
				msgs_plural.write[plural_index] = msgs_plural[plural_index] + l;
			}

			line++;
		}

		// Add the last set of data from last iteration.
		if (status == STATUS_READING_STRING) {
			if (!msg_id.is_empty()) {
				if (!skip_this) {
					translation->add_message(msg_id, msg_str, msg_context);
				}
			} else if (config.is_empty()) {
				config = msg_str;
			}
		} else if (status == STATUS_READING_PLURAL) {
			if (!skip_this && !msg_id.is_empty()) {
				ERR_FAIL_COND_V_MSG(plural_index != plural_forms - 1, Ref<Resource>(), vformat("Number of 'msgstr[]' doesn't match with number of plural forms: %s:%d.", path, line));
				translation->add_plural_message(msg_id, msgs_plural, msg_context);
			}
		}
	}

	ERR_FAIL_COND_V_MSG(config.is_empty(), Ref<Resource>(), vformat("No config found in file: '%s'.", path));

	Vector<String> configs = config.split("\n");
	for (int i = 0; i < configs.size(); i++) {
		String c = configs[i].strip_edges();
		int p = c.find_char(':');
		if (p == -1) {
			continue;
		}
		String prop = c.substr(0, p).strip_edges();
		String value = c.substr(p + 1).strip_edges();

		if (prop == "X-Language" || prop == "Language") {
			translation->set_locale(value);
		}
	}

	if (r_error) {
		*r_error = OK;
	}

	return translation;
}

Ref<Resource> TranslationLoaderPO::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_CANT_OPEN;
	}

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), Ref<Resource>(), vformat("Cannot open file '%s'.", p_path));

	return load_translation(f, r_error);
}

void TranslationLoaderPO::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("po");
	p_extensions->push_back("mo");
}

bool TranslationLoaderPO::handles_type(const String &p_type) const {
	return p_type == "Translation";
}

String TranslationLoaderPO::get_resource_type(const String &p_path) const {
	if (p_path.has_extension("po") || p_path.has_extension("mo")) {
		return "Translation";
	}
	return "";
}

// ============================================================================
// Merged from core/io/dir_access.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/templates/local_vector.h"
#include "dir_access.h"

String DirAccess::_get_root_path() const
{
	switch (_access_type) {
	case ACCESS_RESOURCES:
		return ProjectSettings::get_resource_path();
	case ACCESS_USERDATA:
		return OS::get_user_data_dir();
	default:
		return "";
	}
}

String DirAccess::_get_root_string() const
{
	switch (_access_type) {
	case ACCESS_RESOURCES:
		return "res://";
	case ACCESS_USERDATA:
		return "user://";
	default:
		return "";
	}
}

int DirAccess::get_current_drive()
{
	String path = get_current_dir().to_lower();
	for (int i = 0; i < get_drive_count(); i++) {
		String d = get_drive(i).to_lower();
		if (path.begins_with(d)) {
			return i;
		}
	}

	return 0;
}

bool DirAccess::drives_are_shortcuts() { return false; }

static Error _erase_recursive(DirAccess* p_dir)
{
	List<String> dirs;
	List<String> files;

	p_dir->list_dir_begin();
	String n = p_dir->get_next();
	while (!n.is_empty()) {
		if (n != "." && n != "..") {
			if (p_dir->current_is_dir() && !p_dir->is_link(n)) {
				dirs.push_back(n);
			}
			else {
				files.push_back(n);
			}
		}

		n = p_dir->get_next();
	}

	p_dir->list_dir_end();

	for (const String& E : dirs) {
		Error err = p_dir->change_dir(E);
		if (err == OK) {
			err = _erase_recursive(p_dir);
			if (err) {
				p_dir->change_dir("..");
				return err;
			}
			err = p_dir->change_dir("..");
			if (err) {
				return err;
			}
			err = p_dir->remove(p_dir->get_current_dir().path_join(E));
			if (err) {
				return err;
			}
		}
		else {
			return err;
		}
	}

	for (const String& E : files) {
		Error err = p_dir->remove(p_dir->get_current_dir().path_join(E));
		if (err) {
			return err;
		}
	}

	return OK;
}

Error DirAccess::erase_contents_recursive() { return _erase_recursive(this); }

Error DirAccess::make_dir_recursive(const String& p_dir)
{
	if (p_dir.length() < 1) {
		return OK;
	}

	String full_dir;

	if (p_dir.is_relative_path()) {
		// append current
		full_dir = get_current_dir().path_join(p_dir);

	}
	else {
		full_dir = p_dir;
	}

	full_dir = full_dir.replace_char('\\', '/');

	String base;

	if (full_dir.begins_with("res://")) {
		base = "res://";
	}
	else if (full_dir.begins_with("user://")) {
		base = "user://";
	}
	else if (full_dir.is_network_share_path()) {
		int pos = full_dir.find_char('/', 2);
		ERR_FAIL_COND_V(pos < 0, ERR_INVALID_PARAMETER);
		pos = full_dir.find_char('/', pos + 1);
		ERR_FAIL_COND_V(pos < 0, ERR_INVALID_PARAMETER);
		base = full_dir.substr(0, pos + 1);
	}
	else if (full_dir.begins_with("/")) {
		base = "/";
	}
	else if (full_dir.contains(":/")) {
		base = full_dir.substr(0, full_dir.find(":/") + 2);
	}
	else {
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	}

	full_dir = full_dir.replace_first(base, "").simplify_path();

	Vector<String> subdirs = full_dir.split("/");

	String curpath = base;
	for (int i = 0; i < subdirs.size(); i++) {
		curpath = curpath.path_join(subdirs[i]);
		Error err = make_dir(curpath);
		if (err != OK && err != ERR_ALREADY_EXISTS) {
			ERR_FAIL_V_MSG(err, vformat("Could not create directory: '%s'.", curpath));
		}
	}

	return OK;
}

DirAccess::AccessType DirAccess::get_access_type() const { return _access_type; }

String DirAccess::fix_path(const String& p_path) const
{
	switch (_access_type) {
	case ACCESS_RESOURCES: {
		if (ProjectSettings::is_initialized()) {
			if (p_path.begins_with("res://")) {
				String resource_path = ProjectSettings::get_resource_path();
				if (!resource_path.is_empty()) {
					return p_path.replace_first("res:/", resource_path);
				}
				return p_path.replace_first("res://", "");
			}
		}

	} break;
	case ACCESS_USERDATA: {
		if (p_path.begins_with("user://")) {
			String data_dir = OS::get_user_data_dir();
			if (!data_dir.is_empty()) {
				return p_path.replace_first("user:/", data_dir);
			}
			return p_path.replace_first("user://", "");
		}

	} break;
	case ACCESS_FILESYSTEM: {
		return p_path;
	} break;
	case ACCESS_MAX:
		break; // Can't happen, but silences warning
	}

	return p_path;
}

DirAccess::CreateFunc DirAccess::create_func[ACCESS_MAX] = {nullptr, nullptr, nullptr};

Ref<DirAccess> DirAccess::create_for_path(const String& p_path)
{
	Ref<DirAccess> da;
	if (p_path.begins_with("res://")) {
		da = create(ACCESS_RESOURCES);
	}
	else if (p_path.begins_with("user://")) {
		da = create(ACCESS_USERDATA);
	}
	else {
		da = create(ACCESS_FILESYSTEM);
	}

	return da;
}

Ref<DirAccess> DirAccess::open(const String& p_path, Error* r_error)
{
	Ref<DirAccess> da = create_for_path(p_path);
	ERR_FAIL_COND_V_MSG(
		da.is_null(), nullptr, vformat("Cannot create DirAccess for path '%s'.", p_path));
	Error err = da->change_dir(p_path);
	if (r_error) {
		*r_error = err;
	}
	if (err != OK) {
		return nullptr;
	}

	return da;
}

Ref<DirAccess> DirAccess::_open(const String& p_path)
{
	Error err = OK;
	Ref<DirAccess> da = open(p_path, &err);
	last_dir_open_error = err;
	if (err) {
		return Ref<DirAccess>();
	}
	return da;
}

int DirAccess::_get_drive_count()
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	return d->get_drive_count();
}

String DirAccess::_get_drive_name(int p_idx)
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	return d->get_drive(p_idx);
}

String DirAccess::_get_drive_label(int p_idx)
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	return d->get_drive_label(p_idx);
}

Error DirAccess::make_dir_absolute(const String& p_dir)
{
	Ref<DirAccess> d = DirAccess::create_for_path(p_dir);
	return d->make_dir(p_dir);
}

Error DirAccess::make_dir_recursive_absolute(const String& p_dir)
{
	Ref<DirAccess> d = DirAccess::create_for_path(p_dir);
	return d->make_dir_recursive(p_dir);
}

bool DirAccess::dir_exists_absolute(const String& p_dir)
{
	Ref<DirAccess> d = DirAccess::create_for_path(p_dir);
	return d->dir_exists(p_dir);
}

Error DirAccess::copy_absolute(const String& p_from, const String& p_to, int p_chmod_flags)
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	// Support copying from res:// to user:// etc.
	String from = ProjectSettings::globalize_path(p_from);
	String to = ProjectSettings::globalize_path(p_to);
	return d->copy(from, to, p_chmod_flags);
}

Error DirAccess::rename_absolute(const String& p_from, const String& p_to)
{
	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	String from = ProjectSettings::globalize_path(p_from);
	String to = ProjectSettings::globalize_path(p_to);
	return d->rename(from, to);
}

Error DirAccess::remove_absolute(const String& p_path)
{
	Ref<DirAccess> d = DirAccess::create_for_path(p_path);
	return d->remove(p_path);
}

Ref<DirAccess> DirAccess::create(AccessType p_access)
{
	Ref<DirAccess> da = create_func[p_access] ? create_func[p_access]() : nullptr;
	if (da.is_valid()) {
		da->_access_type = p_access;

		// for ACCESS_RESOURCES and ACCESS_FILESYSTEM, current_dir already defaults to where game
		// was started in case current directory is force changed elsewhere for ACCESS_RESOURCES
		if (p_access == ACCESS_RESOURCES) {
			da->change_dir("res://");
		}
		else if (p_access == ACCESS_USERDATA) {
			da->change_dir("user://");
		}
	}
	return da;
}

Ref<DirAccess> DirAccess::create_temp(const String& p_prefix, bool p_keep, Error* r_error)
{
	const String ERROR_COMMON_PREFIX = "Error while creating temporary directory";

	if (!p_prefix.is_empty() && !p_prefix.is_valid_filename()) {
		*r_error = ERR_FILE_BAD_PATH;
		ERR_FAIL_V_MSG(Ref<DirAccess>(),
			vformat(R"(%s: "%s" is not a valid prefix.)", ERROR_COMMON_PREFIX, p_prefix));
	}

	Ref<DirAccess> dir_access = DirAccess::open(OS::get_temp_path());

	uint32_t suffix_i = 0;
	String path;
	while (true) {
		String datetime = Time::get_datetime_string_from_system().remove_chars("-T:");
		datetime += itos(Time::get_ticks_usec());
		String suffix = datetime + (suffix_i > 0 ? itos(suffix_i) : "");
		path = (p_prefix.is_empty() ? "" : p_prefix + "-") + suffix;
		if (!path.is_valid_filename()) {
			*r_error = ERR_FILE_BAD_PATH;
			return Ref<DirAccess>();
		}
		if (!DirAccess::exists(path)) {
			break;
		}
		suffix_i += 1;
	}

	Error err = dir_access->make_dir(path);
	if (err != OK) {
		*r_error = err;
		ERR_FAIL_V_MSG(Ref<DirAccess>(),
			vformat(R"(%s: "%s" couldn't create directory "%s".)", ERROR_COMMON_PREFIX, path));
	}
	err = dir_access->change_dir(path);
	if (err != OK) {
		*r_error = err;
		return Ref<DirAccess>();
	}

	dir_access->_is_temp = true;
	dir_access->_temp_keep_after_free = p_keep;
	dir_access->_temp_path = dir_access->get_current_dir();

	*r_error = OK;
	return dir_access;
}

Ref<DirAccess> DirAccess::_create_temp(const String& p_prefix, bool p_keep)
{
	return create_temp(p_prefix, p_keep, &last_dir_open_error);
}

void DirAccess::_delete_temp()
{
	if (!_is_temp || _temp_keep_after_free) {
		return;
	}

	if (!DirAccess::exists(_temp_path)) {
		return;
	}

	Error err;
	{
		Ref<DirAccess> dir_access = DirAccess::open(_temp_path, &err);
		if (err != OK) {
			return;
		}
		err = dir_access->erase_contents_recursive();
		if (err != OK) {
			return;
		}
	}

	DirAccess::remove_absolute(_temp_path);
}

Error DirAccess::get_open_error() { return last_dir_open_error; }

String DirAccess::get_full_path(const String& p_path, AccessType p_access)
{
	Ref<DirAccess> d = DirAccess::create(p_access);
	if (d.is_null()) {
		return p_path;
	}

	d->change_dir(p_path);
	String full = d->get_current_dir();
	return full;
}

Error DirAccess::copy(const String& p_from, const String& p_to, int p_chmod_flags)
{
	ERR_FAIL_COND_V_MSG(
		p_from == p_to, ERR_INVALID_PARAMETER, "Source and destination path are equal.");

	// printf("copy %s -> %s\n",p_from.ascii().get_data(),p_to.ascii().get_data());
	Error err;
	{
		Ref<FileAccess> fsrc = FileAccess::open(p_from, FileAccess::READ, &err);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to open '%s'.", p_from));

		Ref<FileAccess> fdst = FileAccess::open(p_to, FileAccess::WRITE, &err);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Failed to open '%s'.", p_to));

		const size_t copy_buffer_limit = 65536; // 64 KB

		fsrc->seek_end(0);
		uint64_t size = fsrc->get_position();
		fsrc->seek(0);
		err = OK;
		size_t buffer_size = MIN(size * sizeof(uint8_t), copy_buffer_limit);
		LocalVector<uint8_t> buffer;
		buffer.resize(buffer_size);
		while (size > 0) {
			if (fsrc->get_error() != OK) {
				err = fsrc->get_error();
				break;
			}
			if (fdst->get_error() != OK) {
				err = fdst->get_error();
				break;
			}

			int bytes_read = fsrc->get_buffer(buffer.ptr(), buffer_size);
			if (bytes_read <= 0) {
				err = FAILED;
				break;
			}
			fdst->store_buffer(buffer.ptr(), bytes_read);

			size -= bytes_read;
		}
	}

	if (err == OK && p_chmod_flags != -1) {
		err = FileAccess::set_unix_permissions(p_to, p_chmod_flags);
		// If running on a platform with no chmod support (i.e., Windows), don't fail
		if (err == ERR_UNAVAILABLE) {
			err = OK;
		}
	}

	return err;
}

// Changes dir for the current scope, returning back to the original dir
// when scope exits
class DirChanger
{
	DirAccess* da;
	String original_dir;

public:
	DirChanger(DirAccess* p_da, const String& p_dir)
		: da(p_da), original_dir(p_da->get_current_dir())
	{
		p_da->change_dir(p_dir);
	}

	~DirChanger() { da->change_dir(original_dir); }
};

Error DirAccess::_copy_dir(
	Ref<DirAccess>& p_target_da, const String& p_to, int p_chmod_flags, bool p_copy_links)
{
	List<String> dirs;

	list_dir_begin();
	String n = get_next();
	while (!n.is_empty()) {
		if (n != "." && n != "..") {
			if (p_copy_links && is_link(get_current_dir().path_join(n))) {
				Error err =
					p_target_da->create_link(read_link(get_current_dir().path_join(n)), p_to + n);
				if (err) {
					ERR_PRINT(vformat("Failed to copy symlink \"%s\".", n));
				}
			}
			else if (current_is_dir()) {
				dirs.push_back(n);
			}
			else {
				const String& rel_path = n;
				if (!n.is_relative_path()) {
					list_dir_end();
					ERR_FAIL_V_MSG(ERR_BUG, vformat("BUG: \"%s\" is not a relative path.", n));
				}
				Error err = copy(get_current_dir().path_join(n), p_to + rel_path, p_chmod_flags);
				if (err) {
					list_dir_end();
					ERR_FAIL_V_MSG(err, vformat("Failed to copy file \"%s\".", n));
				}
			}
		}

		n = get_next();
	}

	list_dir_end();

	for (const String& rel_path : dirs) {
		String target_dir = p_to + rel_path;
		if (!p_target_da->dir_exists(target_dir)) {
			Error err = p_target_da->make_dir(target_dir);
			ERR_FAIL_COND_V_MSG(
				err != OK, err, vformat("Cannot create directory '%s'.", target_dir));
		}

		Error err = change_dir(rel_path);
		ERR_FAIL_COND_V_MSG(
			err != OK, err, vformat("Cannot change current directory to '%s'.", rel_path));

		err = _copy_dir(p_target_da, p_to + rel_path + "/", p_chmod_flags, p_copy_links);
		if (err) {
			change_dir("..");
			ERR_FAIL_V_MSG(err, "Failed to copy recursively.");
		}
		err = change_dir("..");
		ERR_FAIL_COND_V_MSG(err != OK, err, "Failed to go back.");
	}

	return OK;
}

Error DirAccess::copy_dir(const String& p_from, String p_to, int p_chmod_flags, bool p_copy_links)
{
	ERR_FAIL_COND_V_MSG(!dir_exists(p_from), ERR_FILE_NOT_FOUND, "Source directory doesn't exist.");

	Ref<DirAccess> target_da = DirAccess::create_for_path(p_to);
	ERR_FAIL_COND_V_MSG(target_da.is_null(), ERR_CANT_CREATE,
		vformat("Cannot create DirAccess for path '%s'.", p_to));

	if (!target_da->dir_exists(p_to)) {
		Error err = target_da->make_dir_recursive(p_to);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot create directory '%s'.", p_to));
	}

	if (!p_to.ends_with("/")) {
		p_to = p_to + "/";
	}

	DirChanger dir_changer(this, p_from);
	Error err = _copy_dir(target_da, p_to, p_chmod_flags, p_copy_links);

	return err;
}

bool DirAccess::exists(const String& p_dir)
{
	Ref<DirAccess> da = DirAccess::create_for_path(p_dir);
	return
da->change_dir(p_dir) == OK;
}

Vector<String> DirAccess::get_files() { return _get_contents(false); }

Vector<String> DirAccess::get_files_at(const String& p_path)
{
	Ref<DirAccess> da = DirAccess::open(p_path);
	ERR_FAIL_COND_V_MSG(da.is_null(), Vector<String>(),
		vformat("Couldn't open directory at path \"%s\".", p_path));
	return da->get_files();
}

Vector<String> DirAccess::get_directories() { return _get_contents(true); }

Vector<String> DirAccess::get_directories_at(const String& p_path)
{
	Ref<DirAccess> da = DirAccess::open(p_path);
	ERR_FAIL_COND_V_MSG(da.is_null(), Vector<String>(),
		vformat("Couldn't open directory at path \"%s\".", p_path));
	return da->get_directories();
}

Vector<String> DirAccess::_get_contents(bool p_directories)
{
	Vector<String> ret;

	list_dir_begin();
	String s = _get_next();
	while (!s.is_empty()) {
		if (current_is_dir() == p_directories) {
			ret.append(s);
		}
		s = _get_next();
	}

	ret.sort();
	return ret;
}

String DirAccess::_get_next()
{
	String next = get_next();
	while (!next.is_empty() && ((!include_navigational && (next == "." || next == "..")) ||
								   (!include_hidden && current_is_hidden()))) {
		next = get_next();
	}
	return next;
}

void DirAccess::set_include_navigational(bool p_enable) { include_navigational = p_enable; }

bool DirAccess::get_include_navigational() const { return include_navigational; }

void DirAccess::set_include_hidden(bool p_enable) { include_hidden = p_enable; }

bool DirAccess::get_include_hidden() const { return include_hidden; }

bool DirAccess::is_case_sensitive(const String& p_path) const { return true; }

bool DirAccess::is_equivalent(const String& p_path_a, const String& p_path_b) const
{
	return p_path_a == p_path_b;
}

DirAccess::~DirAccess() { _delete_temp(); }

// ==========================================================================
// logger.cpp
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

#include <cstdio>
#include "core/types.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/time.h"
#include "core/templates/rb_set.h"
#include "core/os/os.h"
#include "modules/modules_enabled.gen.h" // For regex.

#ifdef MODULE_REGEX_ENABLED
#include "core/string/translation_server.h"
#endif // MODULE_REGEX_ENABLED

#if defined(MINGW_ENABLED) || defined(_MSC_VER)
#define sprintf sprintf_s
#endif

bool Logger::should_log(bool p_err)
{
	return (!p_err || CoreGlobals::print_error_enabled) &&
		   (p_err || CoreGlobals::print_line_enabled);
}

void Logger::set_flush_stdout_on_print(bool p_value) { _flush_stdout_on_print = p_value; }

void Logger::logf(const char* p_format, ...)
{
	if (!should_log(false)) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	logv(p_format, argp, false);

	va_end(argp);
}

void Logger::logf_error(const char* p_format, ...)
{
	if (!should_log(true)) {
		return;
	}

	va_list argp;
	va_start(argp, p_format);

	logv(p_format, argp, true);

	va_end(argp);
}

void RotatedFileLogger::clear_old_backups()
{
	int max_backups = max_files - 1; // -1 for the current file

	String basename = base_path.get_file().get_basename();
	String extension = base_path.get_extension();

	Ref<DirAccess> da = DirAccess::open(base_path.get_base_dir());
	if (da.is_null()) {
		return;
	}

	da->list_dir_begin();
	String f = da->get_next();
	// backups is a RBSet because it guarantees that iterating on it is done in sorted order.
	// RotatedFileLogger depends on this behavior to delete the oldest log file first.
	RBSet<String> backups;
	while (!f.is_empty()) {
		if (!da->current_is_dir() && f.begins_with(basename) && f.get_extension() == extension &&
			f != base_path.get_file()) {
			backups.insert(f);
		}
		f = da->get_next();
	}
	da->list_dir_end();

	if (backups.size() > max_backups) {
		// since backups are appended with timestamp and Set iterates them in sorted order,
		// first backups are the oldest
		int to_delete = backups.size() - max_backups;
		for (RBSet<String>::Element* E = backups.front(); E && to_delete > 0;
			 E = E->next(), --to_delete) {
			da->remove(E->get());
		}
	}
}

void RotatedFileLogger::rotate_file()
{
	file.unref();

	if (FileAccess::exists(base_path)) {
		if (max_files > 1) {
			String timestamp =
				Time::get_datetime_string_from_system().replace_char(':', '.');
			String backup_name = base_path.get_basename() + timestamp;
			if (!base_path.get_extension().is_empty()) {
				backup_name += "." + base_path.get_extension();
			}

			Ref<DirAccess> da = DirAccess::open(base_path.get_base_dir());
			if (da.is_valid()) {
				da->copy(base_path, backup_name);
			}
			clear_old_backups();
		}
	}
	else {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_USERDATA);
		if (da.is_valid()) {
			da->make_dir_recursive(base_path.get_base_dir());
		}
	}

	file = FileAccess::open(base_path, FileAccess::WRITE);
}

RotatedFileLogger::RotatedFileLogger(const String& p_base_path, int p_max_files)
	: base_path(p_base_path.simplify_path()), max_files(p_max_files > 0 ? p_max_files : 1)
{
	rotate_file();

#ifdef MODULE_REGEX_ENABLED
	strip_ansi_regex.instantiate();
	strip_ansi_regex->compile("\u001b\\[((?:\\d|;)*)([a-zA-Z])");
#endif // MODULE_REGEX_ENABLED
}

void RotatedFileLogger::logv(const char* p_format, va_list p_list, bool p_err)
{
	if (!should_log(p_err)) {
		return;
	}

	if (file.is_valid()) {
		const int static_buf_size = 512;
		char static_buf[static_buf_size];
		char* buf = static_buf;
		va_list list_copy;
		va_copy(list_copy, p_list);
		int len = vsnprintf(buf, static_buf_size, p_format, p_list);
		if (len >= static_buf_size) {
			buf = (char*)Memory::alloc_static(len + 1);
			vsnprintf(buf, len + 1, p_format, list_copy);
		}
		va_end(list_copy);

#ifdef MODULE_REGEX_ENABLED
		// Strip ANSI escape codes (such as those inserted by `print_rich()`)
		// before writing to file, as text editors cannot display those
		// correctly.
		file->store_string(strip_ansi_regex->sub(String::utf8(buf), "", true));
#else
		file->store_buffer((uint8_t*)buf, len);
#endif // MODULE_REGEX_ENABLED

		if (len >= static_buf_size) {
			Memory::free_static(buf);
		}

		if (p_err || _flush_stdout_on_print) {
			// Don't always flush when printing stdout to avoid performance
			// issues when `print()` is spammed in release builds.
			file->flush();
		}
	}
}

void StdLogger::logv(const char* p_format, va_list p_list, bool p_err)
{
	if (!should_log(p_err)) {
		return;
	}

	if (p_err) {
		vfprintf(stderr, p_format, p_list);
	}
	else {
		vprintf(p_format, p_list);
		if (_flush_stdout_on_print) {
			// Don't always flush when printing stdout to avoid performance
			// issues when `print()` is spammed in release builds.
			fflush(stdout);
		}
	}
}

CompositeLogger::CompositeLogger(const Vector<Logger*>& p_loggers) : loggers(p_loggers) {}

void CompositeLogger::logv(const char* p_format, va_list p_list, bool p_err)
{
	if (!should_log(p_err)) {
		return;
	}

	for (int i = 0; i < loggers.size(); ++i) {
		va_list list_copy;
		va_copy(list_copy, p_list);
		loggers[i]->logv(p_format, list_copy, p_err);
		va_end(list_copy);
	}
}

void CompositeLogger::add_logger(Logger* p_logger) { loggers.push_back(p_logger); }

CompositeLogger::~CompositeLogger()
{
	for (int i = 0; i < loggers.size(); ++i) {
		memdelete(loggers[i]);
	}
}
#ifdef sprintf
#undef sprintf
#endif

// ============================================================================
// Merged from core/io/file_access_wrappers.cpp
// ============================================================================

#include "file_access_wrappers.h"

#include "core/config/project_settings.h"
#include "core/io/compression.h"
#include "core/types.h"
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


// --- Merged from core/io/stream_peer.cpp ---
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"

Error StreamPeer::_put_data(const Vector<uint8_t>& p_data)
{
	int len = p_data.size();
	if (len == 0) {
		return OK;
	}
	const uint8_t* r = p_data.ptr();
	return put_data(&r[0], len);
}

void StreamPeer::set_big_endian(bool p_big_endian) { big_endian = p_big_endian; }

bool StreamPeer::is_big_endian_enabled() const { return big_endian; }

void StreamPeer::put_u8(uint8_t p_val) { put_data((const uint8_t*)&p_val, 1); }

void StreamPeer::put_8(int8_t p_val) { put_data((const uint8_t*)&p_val, 1); }

void StreamPeer::put_u16(uint16_t p_val)
{
	if (big_endian) {
		p_val = BSWAP16(p_val);
	}
	uint8_t buf[2];
	encode_uint16(p_val, buf);
	put_data(buf, 2);
}

void StreamPeer::put_16(int16_t p_val)
{
	if (big_endian) {
		p_val = BSWAP16(p_val);
	}
	uint8_t buf[2];
	encode_uint16(p_val, buf);
	put_data(buf, 2);
}

void StreamPeer::put_u32(uint32_t p_val)
{
	if (big_endian) {
		p_val = BSWAP32(p_val);
	}
	uint8_t buf[4];
	encode_uint32(p_val, buf);
	put_data(buf, 4);
}

void StreamPeer::put_32(int32_t p_val)
{
	if (big_endian) {
		p_val = BSWAP32(p_val);
	}
	uint8_t buf[4];
	encode_uint32(p_val, buf);
	put_data(buf, 4);
}

void StreamPeer::put_u64(uint64_t p_val)
{
	if (big_endian) {
		p_val = BSWAP64(p_val);
	}
	uint8_t buf[8];
	encode_uint64(p_val, buf);
	put_data(buf, 8);
}

void StreamPeer::put_64(int64_t p_val)
{
	if (big_endian) {
		p_val = BSWAP64(p_val);
	}
	uint8_t buf[8];
	encode_uint64(p_val, buf);
	put_data(buf, 8);
}

void StreamPeer::put_half(float p_val)
{
	uint8_t buf[2];

	encode_half(p_val, buf);
	uint16_t* p16 = (uint16_t*)buf;
	if (big_endian) {
		*p16 = BSWAP16(*p16);
	}

	put_data(buf, 2);
}

void StreamPeer::put_float(float p_val)
{
	uint8_t buf[4];

	encode_float(p_val, buf);
	if (big_endian) {
		uint32_t* p32 = (uint32_t*)buf;
		*p32 = BSWAP32(*p32);
	}

	put_data(buf, 4);
}

void StreamPeer::put_double(double p_val)
{
	uint8_t buf[8];

	encode_double(p_val, buf);
	if (big_endian) {
		uint64_t* p64 = (uint64_t*)buf;
		*p64 = BSWAP64(*p64);
	}

	put_data(buf, 8);
}

void StreamPeer::put_string(const String& p_string)
{
	CharString cs = p_string.ascii();
	put_u32(cs.length());
	put_data((const uint8_t*)cs.get_data(), cs.length());
}

void StreamPeer::put_utf8_string(const String& p_string)
{
	CharString cs = p_string.utf8();
	put_u32(cs.length());
	put_data((const uint8_t*)cs.get_data(), cs.length());
}

uint8_t StreamPeer::get_u8()
{
	uint8_t buf[1] = {};
	get_data(buf, 1);
	return buf[0];
}

int8_t StreamPeer::get_8()
{
	uint8_t buf[1] = {};
	get_data(buf, 1);
	return int8_t(buf[0]);
}

uint16_t StreamPeer::get_u16()
{
	uint8_t buf[2];
	get_data(buf, 2);

	uint16_t r = decode_uint16(buf);
	if (big_endian) {
		r = BSWAP16(r);
	}

	return r;
}

int16_t StreamPeer::get_16()
{
	uint8_t buf[2];
	get_data(buf, 2);

	uint16_t r = decode_uint16(buf);
	if (big_endian) {
		r = BSWAP16(r);
	}

	return int16_t(r);
}

uint32_t StreamPeer::get_u32()
{
	uint8_t buf[4];
	get_data(buf, 4);

	uint32_t r = decode_uint32(buf);
	if (big_endian) {
		r = BSWAP32(r);
	}

	return r;
}

int32_t StreamPeer::get_32()
{
	uint8_t buf[4];
	get_data(buf, 4);

	uint32_t r = decode_uint32(buf);
	if (big_endian) {
		r = BSWAP32(r);
	}

	return int32_t(r);
}

uint64_t StreamPeer::get_u64()
{
	uint8_t buf[8];
	get_data(buf, 8);

	uint64_t r = decode_uint64(buf);
	if (big_endian) {
		r = BSWAP64(r);
	}

	return r;
}

int64_t StreamPeer::get_64()
{
	uint8_t buf[8];
	get_data(buf, 8);

	uint64_t r = decode_uint64(buf);
	if (big_endian) {
		r = BSWAP64(r);
	}

	return int64_t(r);
}

float StreamPeer::get_half()
{
	uint8_t buf[2];
	get_data(buf, 2);

	if (big_endian) {
		uint16_t* p16 = (uint16_t*)buf;
		*p16 = BSWAP16(*p16);
	}

	return decode_half(buf);
}

float StreamPeer::get_float()
{
	uint8_t buf[4];
	get_data(buf, 4);

	if (big_endian) {
		uint32_t* p32 = (uint32_t*)buf;
		*p32 = BSWAP32(*p32);
	}

	return decode_float(buf);
}

double StreamPeer::get_double()
{
	uint8_t buf[8];
	get_data(buf, 8);

	if (big_endian) {
		uint64_t* p64 = (uint64_t*)buf;
		*p64 = BSWAP64(*p64);
	}

	return decode_double(buf);
}

String StreamPeer::get_string(int p_bytes)
{
	if (p_bytes < 0) {
		p_bytes = get_32();
	}
	ERR_FAIL_COND_V(p_bytes < 0, String());

	Vector<char> buf;
	Error err = buf.resize(p_bytes + 1);
	ERR_FAIL_COND_V(err != OK, String());
	err = get_data((uint8_t*)&buf[0], p_bytes);
	ERR_FAIL_COND_V(err != OK, String());
	buf.write[p_bytes] = 0;
	return buf.ptr();
}

String StreamPeer::get_utf8_string(int p_bytes)
{
	if (p_bytes < 0) {
		p_bytes = get_32();
	}
	ERR_FAIL_COND_V(p_bytes < 0, String());

	Vector<uint8_t> buf;
	Error err = buf.resize(p_bytes);
	ERR_FAIL_COND_V(err != OK, String());
	err = get_data(buf.ptrw(), p_bytes);
	ERR_FAIL_COND_V(err != OK, String());

	return String::utf8((const char*)buf.ptr(), buf.size());
}

Error StreamPeerExtension::get_data(uint8_t* r_buffer, int p_bytes)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_get_data is unimplemented!");
	return FAILED;
}

Error StreamPeerExtension::get_partial_data(uint8_t* r_buffer, int p_bytes, int& r_received)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_get_partial_data is unimplemented!");
	return FAILED;
}

Error StreamPeerExtension::put_data(const uint8_t* p_data, int p_bytes)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_put_data is unimplemented!");
	return FAILED;
}

Error StreamPeerExtension::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	WARN_PRINT_ONCE("StreamPeerExtension::_put_partial_data is unimplemented!");
	return FAILED;
}

Error StreamPeerBuffer::put_data(const uint8_t* p_data, int p_bytes)
{
	if (p_bytes <= 0 || !p_data) {
		return OK;
	}

	if (pointer + p_bytes > data.size()) {
		data.resize(pointer + p_bytes);
	}

	uint8_t* w = data.ptrw();
	memcpy(&w[pointer], p_data, p_bytes);

	pointer += p_bytes;
	return OK;
}

Error StreamPeerBuffer::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	r_sent = p_bytes;
	return put_data(p_data, p_bytes);
}

Error StreamPeerBuffer::get_data(uint8_t* p_buffer, int p_bytes)
{
	int recv;
	get_partial_data(p_buffer, p_bytes, recv);
	if (recv != p_bytes) {
		return ERR_INVALID_PARAMETER;
	}

	return OK;
}

Error StreamPeerBuffer::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	if (!p_bytes) {
		r_received = 0;
		return OK;
	}

	if (pointer + p_bytes > data.size()) {
		r_received = data.size() - pointer;
		if (r_received <= 0) {
			r_received = 0;
			return OK; // you got 0
		}
	}
	else {
		r_received = p_bytes;
	}

	const uint8_t* r = data.ptr();
	memcpy(p_buffer, r + pointer, r_received);

	pointer += r_received;
	// FIXME: return what? OK or ERR_*
	// return OK for now so we don't maybe return garbage
	return OK;
}

int StreamPeerBuffer::get_available_bytes() const { return data.size() - pointer; }

void StreamPeerBuffer::seek(int p_pos)
{
	ERR_FAIL_COND(p_pos < 0);
	ERR_FAIL_COND(p_pos > data.size());
	pointer = p_pos;
}

int StreamPeerBuffer::get_size() const { return data.size(); }

int StreamPeerBuffer::get_position() const { return pointer; }

void StreamPeerBuffer::resize(int p_size) { data.resize(p_size); }

void StreamPeerBuffer::set_data_array(const Vector<uint8_t>& p_data)
{
	data = p_data;
	pointer = 0;
}

const Vector<uint8_t>& StreamPeerBuffer::get_data_array() const { return data; }

void StreamPeerBuffer::clear()
{
	data.clear();
	pointer = 0;
}

Ref<StreamPeerBuffer> StreamPeerBuffer::duplicate() const
{
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->data = data;
	return spb;
}

// ==========================================================================
// stream_peer_socket.cpp
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

#include "core/io/stream_peer.h"

#include "core/os/os.h"

void StreamPeerSocket::accept_socket(Ref<NetSocket> p_sock, const NetSocket::Address& p_addr)
{
	_sock = p_sock;
	_sock->set_blocking_enabled(false);

	timeout = 30;
	status = STATUS_CONNECTED;
	peer_address = p_addr;
	if (p_addr.is_unix()) {
		domain = DOMAIN_UDS;
		peer_path = String(p_addr.get_path().get_data());
	}
	else if (p_addr.is_inet()) {
		domain = DOMAIN_TCP;
	}
}

Error StreamPeerSocket::bind(int p_port, const IPAddress& p_host)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V_MSG(p_port < 0 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The local port number must be between 0 and 65535 (inclusive).");

	domain = DOMAIN_TCP;
	IP::Type ip_type = p_host.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	if (p_host.is_wildcard()) {
		ip_type = IP::TYPE_ANY;
	}
	Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_TCP, ip_type);
	if (err != OK) {
		return err;
	}
	_sock->set_blocking_enabled(false);
	NetSocket::Address addr(p_host, p_port);
	return _sock->bind(addr);
}

Error StreamPeerSocket::connect_to_host(const IPAddress& p_host, int p_port)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(status != STATUS_NONE, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(!p_host.is_valid(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_port < 1 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The remote port number must be between 1 and 65535 (inclusive).");

	domain = DOMAIN_TCP;
	if (!_sock->is_open()) {
		IP::Type ip_type = p_host.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_TCP, ip_type);
		if (err != OK) {
			return err;
		}
		_sock->set_blocking_enabled(false);
	}

	timeout = 30;

	NetSocket::Address addr(p_host, p_port);
	Error err = _sock->connect_to_host(addr);

	if (err == OK) {
		status = STATUS_CONNECTED;
	}
	else if (err == ERR_BUSY) {
		status = STATUS_CONNECTING;
	}
	else {
		ERR_PRINT("Connection to remote host failed!");
		disconnect_from_host();
		return FAILED;
	}

	peer_address = addr;

	return OK;
}

void StreamPeerSocket::set_no_delay(bool p_enabled)
{
	ERR_FAIL_COND(_sock.is_null() || !_sock->is_open());
	_sock->set_tcp_no_delay_enabled(p_enabled);
}

IPAddress StreamPeerSocket::get_connected_host() const { return peer_address.ip(); }

int StreamPeerSocket::get_connected_port() const { return peer_address.port(); }

int StreamPeerSocket::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

Error StreamPeerSocket::bind(const String& p_path)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);

	domain = DOMAIN_UDS;
	IP::Type ip_type = IP::TYPE_NONE;
	Error err = _sock->open(NetSocket::Family::UNIX, NetSocket::TYPE_NONE, ip_type);
	if (err != OK) {
		return err;
	}
	_sock->set_blocking_enabled(false);
	NetSocket::Address addr(p_path);
	return _sock->bind(addr);
}

Error StreamPeerSocket::connect_to_path(const String& p_path)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_INVALID_PARAMETER);

	domain = DOMAIN_UDS;
	if (!_sock->is_open()) {
		IP::Type ip_type = IP::TYPE_NONE;
		Error err = _sock->open(NetSocket::Family::UNIX, NetSocket::TYPE_NONE, ip_type);
		if (err != OK) {
			return err;
		}
		_sock->set_blocking_enabled(false);
	}

	timeout = 30;
	NetSocket::Address addr(p_path);
	Error err = _sock->connect_to_host(addr);

	if (err == OK) {
		status = STATUS_CONNECTED;
	}
	else if (err == ERR_BUSY) {
		status = STATUS_CONNECTING;
	}
	else {
		ERR_PRINT("Connection to remote host failed!");
		disconnect_from_host();
		return FAILED;
	}

	peer_address = addr;
	peer_path = p_path;

	return OK;
}

String StreamPeerSocket::get_connected_path() const
{
	return String(peer_address.get_path().get_data());
}

Error StreamPeerSocket::poll()
{
	if (status == STATUS_CONNECTED) {
		Error err;
		err = _sock->poll(NetSocket::POLL_TYPE_IN, 0);
		if (err == OK) {
			// FIN received
			if (_sock->get_available_bytes() == 0) {
				disconnect_from_host();
				return OK;
			}
		}
		// Also poll write
		err = _sock->poll(NetSocket::POLL_TYPE_IN_OUT, 0);
		if (err != OK && err != ERR_BUSY) {
			// Got an error
			disconnect_from_host();
			status = STATUS_ERROR;
			return err;
		}
		return OK;
	}
	else if (status != STATUS_CONNECTING) {
		return OK;
	}

	Error err = _sock->connect_to_host(peer_address);

	if (err == OK) {
		status = STATUS_CONNECTED;
		return OK;
	}
	else if (err == ERR_BUSY) {
		// Check for connect timeout
		if (OS::get_ticks_msec() > timeout) {
			disconnect_from_host();
			status = STATUS_ERROR;
			return ERR_CONNECTION_ERROR;
		}
		// Still trying to connect
		return OK;
	}

	disconnect_from_host();
	status = STATUS_ERROR;
	return ERR_CONNECTION_ERROR;
}

Error StreamPeerSocket::write(const uint8_t* p_data, int p_bytes, int& r_sent, bool p_block)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);

	if (status != STATUS_CONNECTED) {
		return FAILED;
	}

	Error err;
	int data_to_send = p_bytes;
	const uint8_t* offset = p_data;
	int total_sent = 0;

	while (data_to_send) {
		int sent_amount = 0;
		err = _sock->send(offset, data_to_send, sent_amount);

		if (err != OK) {
			if (err != ERR_BUSY) {
				disconnect_from_host();
				return FAILED;
			}

			if (!p_block) {
				r_sent = total_sent;
				return OK;
			}

			// Block and wait for the socket to accept more data
			err = _sock->poll(NetSocket::POLL_TYPE_OUT, -1);
			if (err != OK) {
				disconnect_from_host();
				return FAILED;
			}
		}
		else {
			data_to_send -= sent_amount;
			offset += sent_amount;
			total_sent += sent_amount;
		}
	}

	r_sent = total_sent;

	return OK;
}

Error StreamPeerSocket::read(uint8_t* p_buffer, int p_bytes, int& r_received, bool p_block)
{
	if (status != STATUS_CONNECTED) {
		return FAILED;
	}

	Error err;
	int to_read = p_bytes;
	int total_read = 0;
	r_received = 0;

	while (to_read) {
		int read = 0;
		err = _sock->recv(p_buffer + total_read, to_read, read);

		if (err != OK) {
			if (err != ERR_BUSY) {
				disconnect_from_host();
				return FAILED;
			}

			if (!p_block) {
				r_received = total_read;
				return OK;
			}

			err = _sock->poll(NetSocket::POLL_TYPE_IN, -1);

			if (err != OK) {
				disconnect_from_host();
				return FAILED;
			}
		}
		else if (read == 0) {
			disconnect_from_host();
			r_received = total_read;
			return ERR_FILE_EOF;
		}
		else {
			to_read -= read;
			total_read += read;

			if (!p_block) {
				r_received = total_read;
				return OK;
			}
		}
	}

	r_received = total_read;

	return OK;
}

StreamPeerSocket::Status StreamPeerSocket::get_status() const { return status; }

void StreamPeerSocket::disconnect_from_host()
{
	if (_sock.is_valid() && _sock->is_open()) {
		_sock->close();
	}

	timeout = 0;
	status = STATUS_NONE;
	peer_address = NetSocket::Address();
	peer_path = String();
}

Error StreamPeerSocket::wait(NetSocket::PollType p_type, int p_timeout)
{
	ERR_FAIL_COND_V(_sock.is_null() || !_sock->is_open(), ERR_UNAVAILABLE);
	return _sock->poll(p_type, p_timeout);
}

Error StreamPeerSocket::put_data(const uint8_t* p_data, int p_bytes)
{
	int total;
	return write(p_data, p_bytes, total, true);
}

Error StreamPeerSocket::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	return write(p_data, p_bytes, r_sent, false);
}

Error StreamPeerSocket::get_data(uint8_t* p_buffer, int p_bytes)
{
	int total;
	return read(p_buffer, p_bytes, total, true);
}

Error StreamPeerSocket::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	return read(p_buffer, p_bytes, r_received, false);
}

int StreamPeerSocket::get_available_bytes() const
{
	ERR_FAIL_COND_V(_sock.is_null(), -1);
	return _sock->get_available_bytes();
}

StreamPeerSocket::StreamPeerSocket(Domain p_domain)
	: domain(p_domain), _sock(NetSocket::create())
{
}

StreamPeerSocket::~StreamPeerSocket() { disconnect_from_host(); }

// ==========================================================================
// stream_peer_gzip.cpp
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

#include <zlib.h>
#include "core/io/stream_peer.h"
#include "core/io/file_access.h"

StreamPeerGZIP::~StreamPeerGZIP() { _close(); }

void StreamPeerGZIP::_close()
{
	if (ctx) {
		z_stream* strm = (z_stream*)ctx;
		if (compressing) {
			deflateEnd(strm);
		}
		else {
			inflateEnd(strm);
		}
		memfree(strm);
		ctx = nullptr;
	}
}

void StreamPeerGZIP::clear()
{
	_close();
	rb.clear();
	buffer.clear();
}

Error StreamPeerGZIP::start_compression(bool p_is_deflate, int p_buffer_size)
{
	return _start(true, p_is_deflate, p_buffer_size);
}

Error StreamPeerGZIP::start_decompression(bool p_is_deflate, int p_buffer_size)
{
	return _start(false, p_is_deflate, p_buffer_size);
}

Error StreamPeerGZIP::_start(bool p_compress, bool p_is_deflate, int p_buffer_size)
{
	ERR_FAIL_COND_V(ctx != nullptr, ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V_MSG(p_buffer_size <= 0, ERR_INVALID_PARAMETER,
		"Invalid buffer size. It should be a positive integer.");
	clear();
	compressing = p_compress;
	rb.resize(Math::nearest_shift(uint32_t(p_buffer_size - 1)));
	buffer.resize(1024);

	// Create ctx.
	ctx = memalloc(sizeof(z_stream));
	z_stream& strm = *(z_stream*)ctx;
	strm.next_in = Z_NULL;
	strm.avail_in = 0;
	strm.zalloc = zipio_alloc;
	strm.zfree = zipio_free;
	strm.opaque = Z_NULL;
	int window_bits = p_is_deflate ? 15 : (15 + 16);
	int err = Z_OK;
	int level = Z_DEFAULT_COMPRESSION;
	if (compressing) {
		err = deflateInit2(&strm, level, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
	}
	else {
		err = inflateInit2(&strm, window_bits);
	}
	ERR_FAIL_COND_V(err != Z_OK, FAILED);
	return OK;
}

Error StreamPeerGZIP::_process(uint8_t* p_dst, int p_dst_size, const uint8_t* p_src, int p_src_size,
	int& r_consumed, int& r_out, bool p_close)
{
	ERR_FAIL_NULL_V(ctx, ERR_UNCONFIGURED);
	z_stream& strm = *(z_stream*)ctx;
	strm.avail_in = p_src_size;
	strm.avail_out = p_dst_size;
	strm.next_in = (Bytef*)p_src;
	strm.next_out = (Bytef*)p_dst;
	int flush = p_close ? Z_FINISH : Z_NO_FLUSH;
	if (compressing) {
		int err = deflate(&strm, flush);
		ERR_FAIL_COND_V(err != (p_close ? Z_STREAM_END : Z_OK), FAILED);
	}
	else {
		int err = inflate(&strm, flush);
		ERR_FAIL_COND_V(err != Z_OK && err != Z_STREAM_END, FAILED);
	}
	r_out = p_dst_size - strm.avail_out;
	r_consumed = p_src_size - strm.avail_in;
	return OK;
}

Error StreamPeerGZIP::put_data(const uint8_t* p_data, int p_bytes)
{
	int wrote = 0;
	Error err = put_partial_data(p_data, p_bytes, wrote);
	if (err != OK) {
		return err;
	}
	ERR_FAIL_COND_V(p_bytes != wrote, ERR_OUT_OF_MEMORY);
	return OK;
}

Error StreamPeerGZIP::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	ERR_FAIL_NULL_V(ctx, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(p_bytes < 0, ERR_INVALID_PARAMETER);

	// Ensure we have enough space in temporary buffer.
	if (buffer.size() < p_bytes) {
		buffer.resize(p_bytes);
	}

	r_sent = 0;
	while (r_sent < p_bytes && rb.space_left() > 1024) { // Keep the ring buffer size meaningful.
		int sent = 0;
		int to_write = 0;
		// Compress or decompress
		Error err = _process(buffer.ptrw(), MIN(buffer.size(), rb.space_left()), p_data + r_sent,
			p_bytes - r_sent, sent, to_write);
		if (err != OK) {
			return err;
		}
		// When decompressing, we might need to do another round.
		r_sent += sent;

		// We can't write more than this buffer is full.
		if (sent == 0 && to_write == 0) {
			return OK;
		}
		if (to_write) {
			// Copy to ring buffer.
			int wrote = rb.write(buffer.ptr(), to_write);
			ERR_FAIL_COND_V(wrote != to_write, ERR_BUG);
		}
	}
	return OK;
}

Error StreamPeerGZIP::get_data(uint8_t* p_buffer, int p_bytes)
{
	int received = 0;
	Error err = get_partial_data(p_buffer, p_bytes, received);
	if (err != OK) {
		return err;
	}
	ERR_FAIL_COND_V(p_bytes != received, ERR_UNAVAILABLE);
	return OK;
}

Error StreamPeerGZIP::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	ERR_FAIL_COND_V(p_bytes < 0, ERR_INVALID_PARAMETER);

	r_received = MIN(p_bytes, rb.data_left());
	if (r_received == 0) {
		return OK;
	}
	int received = rb.read(p_buffer, r_received);
	ERR_FAIL_COND_V(received != r_received, ERR_BUG);
	return OK;
}

int StreamPeerGZIP::get_available_bytes() const { return rb.data_left(); }

Error StreamPeerGZIP::finish()
{
	ERR_FAIL_COND_V(!ctx || !compressing, ERR_UNAVAILABLE);
	// Ensure we have enough space in temporary buffer.
	if (buffer.size() < get_available_bytes()) {
		buffer.resize(
			get_available_bytes()); // get_available_bytes() is what we can store in RingBuffer.
	}
	int consumed = 0;
	int to_write = 0;
	Error err =
		_process(buffer.ptrw(), buffer.size(), nullptr, 0, consumed, to_write, true); // compress
	if (err != OK) {
		return err;
	}
	int wrote = rb.write(buffer.ptr(), to_write);
	ERR_FAIL_COND_V(wrote != to_write, ERR_OUT_OF_MEMORY);
	return OK;
}

// ==========================================================================
// stream_peer_tls.cpp
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

#include "core/io/stream_peer.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_MBEDTLS_ENABLED

void StreamPeerTLS::poll() {}
Error StreamPeerTLS::accept_stream(Ref<StreamPeer> p_base, Ref<TLSOptions> p_options)
{
	return ERR_UNAVAILABLE;
}
Error StreamPeerTLS::connect_to_stream(
	Ref<StreamPeer> p_base, const String& p_common_name, Ref<TLSOptions> p_options)
{
	return ERR_UNAVAILABLE;
}
StreamPeerTLS::Status StreamPeerTLS::get_status() const { return STATUS_DISCONNECTED; }
Ref<StreamPeer> StreamPeerTLS::get_stream() const { return Ref<StreamPeer>(); }
void StreamPeerTLS::disconnect_from_stream() {}

Error StreamPeerTLS::put_data(const uint8_t* p_data, int p_bytes) { return ERR_UNAVAILABLE; }
Error StreamPeerTLS::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	r_sent = 0;
	return ERR_UNAVAILABLE;
}
Error StreamPeerTLS::get_data(uint8_t* p_buffer, int p_bytes) { return ERR_UNAVAILABLE; }
Error StreamPeerTLS::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	r_received = 0;
	return ERR_UNAVAILABLE;
}
int StreamPeerTLS::get_available_bytes() const { return 0; }

StreamPeerTLS* StreamPeerTLS::create(bool p_notify_postinitialize) { return nullptr; }
bool StreamPeerTLS::is_available() { return false; }

StreamPeerTLS::StreamPeerTLS() {}
StreamPeerTLS::~StreamPeerTLS() {}

#endif // !MODULE_MBEDTLS_ENABLED

// ==========================================================================
// socket_server.cpp
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

#include "core/io/stream_peer.h"

Error SocketServer::_listen(const NetSocket::Address& p_addr)
{
	DEV_ASSERT(_sock.is_valid());
	DEV_ASSERT(_sock->is_open());

	_sock->set_blocking_enabled(false);
	Error err = _sock->bind(p_addr);

	if (err != OK) {
		_sock->close();
		return ERR_ALREADY_IN_USE;
	}

	err = _sock->listen(MAX_PENDING_CONNECTIONS);

	if (err != OK) {
		_sock->close();
		return FAILED;
	}
	return OK;
}

Error SocketServer::listen(uint16_t p_port, const IPAddress& p_bind_address)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(
		!p_bind_address.is_valid() && !p_bind_address.is_wildcard(), ERR_INVALID_PARAMETER);

	domain = StreamPeerSocket::DOMAIN_TCP;
	IP::Type ip_type = IP::TYPE_ANY;

	// If the bind address is valid use its type as the socket type
	if (p_bind_address.is_valid()) {
		ip_type = p_bind_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	}

	Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_TCP, ip_type);

	ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);

	_sock->set_reuse_address_enabled(true);

	return _listen(NetSocket::Address(p_bind_address, p_port));
}

Error SocketServer::listen_path(const String& p_path)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_INVALID_PARAMETER);

	domain = StreamPeerSocket::DOMAIN_UDS;
	IP::Type ip_type = IP::TYPE_NONE;
	Error err = _sock->open(NetSocket::Family::UNIX, NetSocket::TYPE_NONE, ip_type);
	ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);

	return _listen(NetSocket::Address(p_path));
}

int SocketServer::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

bool SocketServer::is_listening() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	return _sock->is_open();
}

bool SocketServer::is_connection_available() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	if (!_sock->is_open()) {
		return false;
	}

	Error err = _sock->poll(NetSocket::POLL_TYPE_IN, 0);
	return (err == OK);
}

Ref<StreamPeerSocket> SocketServer::take_connection()
{
	Ref<StreamPeerSocket> conn;
	if (!is_connection_available()) {
		return conn;
	}

	NetSocket::Address addr;
	Ref<NetSocket> ns = _sock->accept(addr);
	if (ns.is_null()) {
		return conn;
	}

	conn.instantiate(domain);
	conn->accept_socket(ns, addr);
	return conn;
}

void SocketServer::stop()
{
	if (_sock.is_valid()) {
		_sock->close();
	}
}

SocketServer::SocketServer() : _sock(NetSocket::create()) {}

SocketServer::~SocketServer() { stop(); }

// ============================================================================
// Merged from core/io/http_client.cpp
// ============================================================================

#include "core/io/stream_peer.h"

#ifndef WEB_ENABLED
#include "core/io/stream_peer.h"
#include "core/os/os.h"
#include "core/version.h"
#endif

const char* HTTPClient::_methods[METHOD_MAX] = {
	"GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE", "CONNECT", "PATCH"};

Ref<HTTPClient> HTTPClient::create(bool p_notify_postinitialize)
{
#ifdef WEB_ENABLED
	if (_create) {
		return Ref<HTTPClient>(_create(p_notify_postinitialize));
	}
	return Ref<HTTPClient>();
#else
	(void)p_notify_postinitialize;
	return Ref<HTTPClient>(memnew(HTTPClient));
#endif
}

Error HTTPClient::_request_raw(Method p_method, const String& p_url,
	const Vector<String>& p_headers, const Vector<uint8_t>& p_body)
{
	int size = p_body.size();
	return request(p_method, p_url, p_headers, size > 0 ? p_body.ptr() : nullptr, size);
}

Error HTTPClient::_request(
	Method p_method, const String& p_url, const Vector<String>& p_headers, const String& p_body)
{
	CharString body_utf8 = p_body.utf8();
	int size = body_utf8.length();
	return request(p_method, p_url, p_headers,
		size > 0 ? (const uint8_t*)body_utf8.get_data() : nullptr, size);
}

Error HTTPClient::verify_headers(const Vector<String>& p_headers)
{
	for (int i = 0; i < p_headers.size(); i++) {
		String sanitized = p_headers[i].strip_edges();
		ERR_FAIL_COND_V_MSG(sanitized.is_empty(), ERR_INVALID_PARAMETER,
			vformat("Invalid HTTP header at index %d: empty.", i));
		ERR_FAIL_COND_V_MSG(sanitized.find_char(':') < 1, ERR_INVALID_PARAMETER,
			vformat("Invalid HTTP header at index %d: String must contain header-value pair, "
					"delimited by ':', but was: '%s'.",
				i, p_headers[i]));
	}

	return OK;
}

Vector<String> HTTPClient::_get_response_headers()
{
	List<String> rh;
	get_response_headers(&rh);
	Vector<String> ret;
	ret.resize(rh.size());
	int idx = 0;
	for (const String& E : rh) {
		ret.set(idx++, E);
	}

	return ret;
}

#ifdef WEB_ENABLED

void HTTPClient::set_http_proxy(const String& p_host, int p_port)
{
	WARN_PRINT("HTTP proxy feature is not available");
}

void HTTPClient::set_https_proxy(const String& p_host, int p_port)
{
	WARN_PRINT("HTTPS proxy feature is not available");
}

#else

Error HTTPClient::connect_to_host(const String& p_host, int p_port, Ref<TLSOptions> p_options)
{
	close();

	conn_port = p_port;
	conn_host = p_host;
	tls_options = p_options;

	String host_lower = conn_host.to_lower();
	if (host_lower.begins_with("http://")) {
		conn_host = conn_host.substr(7);
		tls_options.unref();
	}
	else if (host_lower.begins_with("https://")) {
		if (tls_options.is_null()) {
			tls_options = TLSOptions::client();
		}
		conn_host = conn_host.substr(8);
	}

	ERR_FAIL_COND_V(tls_options.is_valid() && tls_options->is_server(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(tls_options.is_valid() && !StreamPeerTLS::is_available(), ERR_UNAVAILABLE,
		"HTTPS is not available in this build.");
	ERR_FAIL_COND_V(conn_host.length() < HOST_MIN_LEN, ERR_INVALID_PARAMETER);

	if (conn_port < 0) {
		if (tls_options.is_valid()) {
			conn_port = PORT_HTTPS;
		}
		else {
			conn_port = PORT_HTTP;
		}
	}

	connection = tcp_connection;

	if (tls_options.is_valid() && https_proxy_port != -1) {
		proxy_client.instantiate(); // Needs proxy negotiation.
		server_host = https_proxy_host;
		server_port = https_proxy_port;
	}
	else if (tls_options.is_null() && http_proxy_port != -1) {
		server_host = http_proxy_host;
		server_port = http_proxy_port;
	}
	else {
		server_host = conn_host;
		server_port = conn_port;
	}

	if (server_host.is_valid_ip_address()) {
		// Host contains valid IP.
		Error err = tcp_connection->connect_to_host(IPAddress(server_host), server_port);
		if (err) {
			status = STATUS_CANT_CONNECT;
			return err;
		}

		status = STATUS_CONNECTING;
	}
	else {
		// Host contains hostname and needs to be resolved to IP.
		resolving = IP::resolve_hostname_queue_item(server_host);
		if (resolving == IP::RESOLVER_INVALID_ID) {
			status = STATUS_CANT_RESOLVE;
			return ERR_CANT_RESOLVE;
		}
		status = STATUS_RESOLVING;
	}

	return OK;
}

void HTTPClient::set_connection(const Ref<StreamPeer>& p_connection)
{
	ERR_FAIL_COND_MSG(
		p_connection.is_null(), "Connection is not a reference to a valid StreamPeer object.");

	if (connection == p_connection) {
		return;
	}

	close();
	connection = p_connection;
	status = STATUS_CONNECTED;
}

Ref<StreamPeer> HTTPClient::get_connection() const { return connection; }

static bool _check_request_url(HTTPClient::Method p_method, const String& p_url)
{
	switch (p_method) {
	case HTTPClient::METHOD_CONNECT: {
		// Authority in host:port format, as in RFC7231.
		int pos = p_url.find_char(':');
		return 0 < pos && pos < p_url.length() - 1;
	}
	case HTTPClient::METHOD_OPTIONS: {
		if (p_url == "*") {
			return true;
		}
		[[fallthrough]];
	}
	default:
		// Absolute path or absolute URL.
		return p_url.begins_with("/") || p_url.begins_with("http://") ||
			   p_url.begins_with("https://");
	}
}

Error HTTPClient::request(Method p_method, const String& p_url, const Vector<String>& p_headers,
	const uint8_t* p_body, int p_body_size)
{
	ERR_FAIL_INDEX_V(p_method, METHOD_MAX, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(!_check_request_url(p_method, p_url), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(status != STATUS_CONNECTED, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(connection.is_null(), ERR_INVALID_DATA);

	Error err = verify_headers(p_headers);
	if (err) {
		return err;
	}

	String uri = p_url;
	if (tls_options.is_null() && http_proxy_port != -1) {
		uri = vformat("http://%s:%d%s", conn_host, conn_port, p_url);
	}

	String req_str = String(_methods[p_method]) + " " + uri + " HTTP/1.1\r\n";
	bool add_host = true;
	bool add_clen = p_body_size > 0;
	bool add_uagent = true;
	bool add_accept = true;
	for (int i = 0; i < p_headers.size(); i++) {
		req_str += p_headers[i] + "\r\n";
		if (add_host && p_headers[i].findn("Host:") == 0) {
			add_host = false;
		}
		if (add_clen && p_headers[i].findn("Content-Length:") == 0) {
			add_clen = false;
		}
		if (add_uagent && p_headers[i].findn("User-Agent:") == 0) {
			add_uagent = false;
		}
		if (add_accept && p_headers[i].findn("Accept:") == 0) {
			add_accept = false;
		}
	}
	if (add_host) {
		if ((tls_options.is_valid() && conn_port == PORT_HTTPS) ||
			(tls_options.is_null() && conn_port == PORT_HTTP)) {
			// Don't append the standard ports.
			req_str += "Host: " + conn_host + "\r\n";
		}
		else {
			req_str += "Host: " + conn_host + ":" + itos(conn_port) + "\r\n";
		}
	}
	if (add_clen) {
		req_str += "Content-Length: " + itos(p_body_size) + "\r\n";
	}
	if (add_uagent) {
		req_str += "User-Agent: GodotEngine/" + String(VLTR_VERSION_FULL_BUILD) + " (" +
				   OS::get_name() + ")\r\n";
	}
	if (add_accept) {
		req_str += "Accept: */*\r\n";
	}
	req_str += "\r\n";
	CharString cs = req_str.utf8();

	request_buffer->clear();
	request_buffer->put_data((const uint8_t*)cs.get_data(), cs.length());
	if (p_body_size > 0) {
		request_buffer->put_data(p_body, p_body_size);
	}
	request_buffer->seek(0);

	status = STATUS_REQUESTING;
	head_request = p_method == METHOD_HEAD;

	return OK;
}

bool HTTPClient::has_response() const { return response_headers.size() != 0; }

bool HTTPClient::is_response_chunked() const { return chunked; }

int HTTPClient::get_response_code() const { return response_num; }

Error HTTPClient::get_response_headers(List<String>* r_response)
{
	if (!response_headers.size()) {
		return ERR_INVALID_PARAMETER;
	}

	for (int i = 0; i < response_headers.size(); i++) {
		r_response->push_back(response_headers[i]);
	}

	response_headers.clear();

	return OK;
}

void HTTPClient::close()
{
	if (tcp_connection->get_status() != StreamPeerSocket::STATUS_NONE) {
		tcp_connection->disconnect_from_host();
	}

	connection.unref();
	proxy_client.unref();
	status = STATUS_DISCONNECTED;
	head_request = false;
	if (resolving != IP::RESOLVER_INVALID_ID) {
		IP::erase_resolve_item(resolving);
		resolving = IP::RESOLVER_INVALID_ID;
	}
	response_headers.clear();
	response_str.clear();
	request_buffer->clear();
	body_size = -1;
	body_left = 0;
	chunk_left = 0;
	chunk_trailer_part = false;
	read_until_eof = false;
	response_num = 0;
	handshaking = false;
}

Error HTTPClient::poll()
{
	if (tcp_connection.is_valid()) {
		tcp_connection->poll();
	}
	switch (status) {
	case STATUS_RESOLVING: {
		ERR_FAIL_COND_V(resolving == IP::RESOLVER_INVALID_ID, ERR_BUG);

		IP::ResolverStatus rstatus = IP::get_resolve_item_status(resolving);
		switch (rstatus) {
		case IP::RESOLVER_STATUS_WAITING:
			return OK; // Still resolving.

		case IP::RESOLVER_STATUS_DONE: {
			IP::erase_resolve_item(resolving);
			resolving = IP::RESOLVER_INVALID_ID;

			Error err = ERR_BUG; // Should be at least one entry.
			if (err) {
				status = STATUS_CANT_CONNECT;
				return err;
			}

			status = STATUS_CONNECTING;
		} break;
		case IP::RESOLVER_STATUS_NONE:
		case IP::RESOLVER_STATUS_ERROR: {
			IP::erase_resolve_item(resolving);
			resolving = IP::RESOLVER_INVALID_ID;
			close();
			status = STATUS_CANT_RESOLVE;
			return ERR_CANT_RESOLVE;
		} break;
		}
	} break;
	case STATUS_CONNECTING: {
		StreamPeerSocket::Status s = tcp_connection->get_status();
		switch (s) {
		case StreamPeerSocket::STATUS_CONNECTING: {
			return OK;
		} break;
		case StreamPeerSocket::STATUS_CONNECTED: {
			if (tls_options.is_valid() && proxy_client.is_valid()) {
				Error err = proxy_client->poll();
				if (err == ERR_UNCONFIGURED) {
					proxy_client->set_connection(tcp_connection);
					const Vector<String> headers;
					err = proxy_client->request(METHOD_CONNECT,
						vformat("%s:%d", conn_host, conn_port), headers, nullptr, 0);
					if (err != OK) {
						status = STATUS_CANT_CONNECT;
						return err;
					}
				}
				else if (err != OK) {
					status = STATUS_CANT_CONNECT;
					return err;
				}
				switch (proxy_client->get_status()) {
				case STATUS_REQUESTING: {
					return OK;
				} break;
				case STATUS_BODY: {
					proxy_client->read_response_body_chunk();
					return OK;
				} break;
				case STATUS_CONNECTED: {
					if (proxy_client->get_response_code() != RESPONSE_OK) {
						status = STATUS_CANT_CONNECT;
						return ERR_CANT_CONNECT;
					}
					proxy_client.unref();
					return OK;
				}
				case STATUS_DISCONNECTED:
				case STATUS_RESOLVING:
				case STATUS_CONNECTING: {
					status = STATUS_CANT_CONNECT;
					ERR_FAIL_V(ERR_BUG);
				} break;
				default: {
					status = STATUS_CANT_CONNECT;
					return ERR_CANT_CONNECT;
				} break;
				}
			}
			else if (tls_options.is_valid()) {
				Ref<StreamPeerTLS> tls_conn;
				if (!handshaking) {
					// Connect the StreamPeerTLS and start handshaking.
					tls_conn = Ref<StreamPeerTLS>(StreamPeerTLS::create());
					Error err = tls_conn->connect_to_stream(tcp_connection, conn_host, tls_options);
					if (err != OK) {
						close();
						status = STATUS_TLS_HANDSHAKE_ERROR;
						return ERR_CANT_CONNECT;
					}
					connection = tls_conn;
					handshaking = true;
				}
				else {
					// We are already handshaking, which means we can use your already active TLS
					// connection.
					tls_conn = static_cast<Ref<StreamPeerTLS>>(connection);
					if (tls_conn.is_null()) {
						close();
						status = STATUS_TLS_HANDSHAKE_ERROR;
						return ERR_CANT_CONNECT;
					}

					tls_conn->poll(); // Try to finish the handshake.
				}

				if (tls_conn->get_status() == StreamPeerTLS::STATUS_CONNECTED) {
					// Handshake has been successful.
					handshaking = false;
					status = STATUS_CONNECTED;
					return OK;
				}
				else if (tls_conn->get_status() != StreamPeerTLS::STATUS_HANDSHAKING) {
					// Handshake has failed.
					close();
					status = STATUS_TLS_HANDSHAKE_ERROR;
					return ERR_CANT_CONNECT;
				}
				// ... we will need to poll more for handshake to finish.
			}
			else {
				status = STATUS_CONNECTED;
			}
			return OK;
		} break;
		case StreamPeerSocket::STATUS_ERROR:
		case StreamPeerSocket::STATUS_NONE: {
			Error err = ERR_CANT_CONNECT;
			close();
			status = STATUS_CANT_CONNECT;
			return err;
		} break;
		}
	} break;
	case STATUS_BODY:
	case STATUS_CONNECTED: {
		// Check if we are still connected.
		if (tls_options.is_valid()) {
			Ref<StreamPeerTLS> tmp = connection;
			tmp->poll();
			if (tmp->get_status() != StreamPeerTLS::STATUS_CONNECTED) {
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
		}
		else if (tcp_connection->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			status = STATUS_CONNECTION_ERROR;
			return ERR_CONNECTION_ERROR;
		}
		// Connection established, requests can now be made.
		return OK;
	} break;
	case STATUS_REQUESTING: {
		if (request_buffer->get_available_bytes()) {
			int avail = request_buffer->get_available_bytes();
			int pos = request_buffer->get_position();
			const Vector<uint8_t> data = request_buffer->get_data_array();
			int wrote = 0;
			Error err;
			if (blocking) {
				err = connection->put_data(data.ptr() + pos, avail);
				wrote += avail;
			}
			else {
				err = connection->put_partial_data(data.ptr() + pos, avail, wrote);
			}
			if (err != OK) {
				close();
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}
			pos += wrote;
			request_buffer->seek(pos);
			if (avail - wrote > 0) {
				return OK;
			}
			request_buffer->clear();
		}
		while (true) {
			uint8_t byte;
			int rec = 0;
			Error err = _get_http_data(&byte, 1, rec);
			if (err != OK) {
				close();
				status = STATUS_CONNECTION_ERROR;
				return ERR_CONNECTION_ERROR;
			}

			if (rec == 0) {
				return OK; // Still requesting, keep trying!
			}

			response_str.push_back(byte);
			int rs = response_str.size();
			if ((rs >= 2 && response_str[rs - 2] == '\n' && response_str[rs - 1] == '\n') ||
				(rs >= 4 && response_str[rs - 4] == '\r' && response_str[rs - 3] == '\n' &&
					response_str[rs - 2] == '\r' && response_str[rs - 1] == '\n')) {
				// End of response, parse.
				response_str.push_back(0);
				String response =
					String::utf8((const char*)response_str.ptr(), response_str.size());
				Vector<String> responses = response.split("\n");
				body_size = -1;
				chunked = false;
				body_left = 0;
				chunk_left = 0;
				chunk_trailer_part = false;
				read_until_eof = false;
				response_str.clear();
				response_headers.clear();
				response_num = RESPONSE_OK;

				// Per the HTTP 1.1 spec, keep-alive is the default.
				// Not following that specification breaks standard implementations.
				// Broken web servers should be fixed.
				bool keep_alive = true;

				for (int i = 0; i < responses.size(); i++) {
					String header = responses[i].strip_edges();
					String s = header.to_lower();
					if (s.length() == 0) {
						continue;
					}
					if (s.begins_with("content-length:")) {
						body_size = s.substr(s.find_char(':') + 1).strip_edges().to_int();
						body_left = body_size;
					}
					else if (s.begins_with("transfer-encoding:")) {
						String encoding = header.substr(header.find_char(':') + 1).strip_edges();
						if (encoding == "chunked") {
							chunked = true;
						}
					}
					else if (s.begins_with("connection: close")) {
						keep_alive = false;
					}

					if (i == 0 && responses[i].begins_with("HTTP")) {
						String num = responses[i].get_slicec(' ', 1);
						response_num = num.to_int();
					}
					else {
						response_headers.push_back(header);
					}
				}

				// This is a HEAD request, we won't receive anything.
				if (head_request) {
					body_size = 0;
					body_left = 0;
				}

				if (body_size != -1 || chunked) {
					status = STATUS_BODY;
				}
				else if (!keep_alive) {
					read_until_eof = true;
					status = STATUS_BODY;
				}
				else {
					status = STATUS_CONNECTED;
				}
				return OK;
			}
		}
	} break;
	case STATUS_DISCONNECTED: {
		return ERR_UNCONFIGURED;
	} break;
	case STATUS_CONNECTION_ERROR:
	case STATUS_TLS_HANDSHAKE_ERROR: {
		return ERR_CONNECTION_ERROR;
	} break;
	case STATUS_CANT_CONNECT: {
		return ERR_CANT_CONNECT;
	} break;
	case STATUS_CANT_RESOLVE: {
		return ERR_CANT_RESOLVE;
	} break;
	}

	return OK;
}

int64_t HTTPClient::get_response_body_length() const { return body_size; }

Vector<uint8_t> HTTPClient::read_response_body_chunk()
{
	ERR_FAIL_COND_V(status != STATUS_BODY, Vector<uint8_t>());

	Vector<uint8_t> ret;
	Error err = OK;

	if (chunked) {
		while (true) {
			if (chunk_trailer_part) {
				// We need to consume the trailer part too or keep-alive will break.
				uint8_t b;
				int rec = 0;
				err = _get_http_data(&b, 1, rec);

				if (rec == 0) {
					break;
				}

				chunk.push_back(b);
				int cs = chunk.size();
				if ((cs >= 2 && chunk[cs - 2] == '\r' && chunk[cs - 1] == '\n')) {
					if (cs == 2) {
						// Finally over.
						chunk_trailer_part = false;
						status = STATUS_CONNECTED;
						chunk.clear();
						break;
					}
					else {
						// We do not process nor return the trailer data.
						chunk.clear();
					}
				}
			}
			else if (chunk_left == 0) {
				// Reading length.
				uint8_t b;
				int rec = 0;
				err = _get_http_data(&b, 1, rec);

				if (rec == 0) {
					break;
				}

				chunk.push_back(b);

				if (chunk.size() > 32) {
					ERR_PRINT("HTTP Invalid chunk hex len");
					status = STATUS_CONNECTION_ERROR;
					break;
				}

				if (chunk.size() > 2 && chunk[chunk.size() - 2] == '\r' &&
					chunk[chunk.size() - 1] == '\n') {
					int len = 0;
					for (int i = 0; i < chunk.size() - 2; i++) {
						char c = chunk[i];
						int v = 0;
						if (is_digit(c)) {
							v = c - '0';
						}
						else if (c >= 'a' && c <= 'f') {
							v = c - 'a' + 10;
						}
						else if (c >= 'A' && c <= 'F') {
							v = c - 'A' + 10;
						}
						else {
							ERR_PRINT("HTTP Chunk len not in hex!!");
							status = STATUS_CONNECTION_ERROR;
							break;
						}
						len <<= 4;
						len |= v;
						if (len > (1 << 24)) {
							ERR_PRINT("HTTP Chunk too big!! >16mb");
							status = STATUS_CONNECTION_ERROR;
							break;
						}
					}

					if (len == 0) {
						// End reached!
						chunk_trailer_part = true;
						chunk.clear();
						break;
					}

					chunk_left = len + 2;
					chunk.resize(chunk_left);
				}
			}
			else {
				int rec = 0;
				err = _get_http_data(&chunk.write[chunk.size() - chunk_left], chunk_left, rec);
				if (rec == 0) {
					break;
				}
				chunk_left -= rec;

				if (chunk_left == 0) {
					const int chunk_size = chunk.size();
					if (chunk[chunk_size - 2] != '\r' || chunk[chunk_size - 1] != '\n') {
						ERR_PRINT("HTTP Invalid chunk terminator (not \\r\\n)");
						status = STATUS_CONNECTION_ERROR;
						break;
					}

					ret.resize(chunk_size - 2);
					uint8_t* w = ret.ptrw();
					memcpy(w, chunk.ptr(), chunk_size - 2);
					chunk.clear();
				}

				break;
			}
		}
	}
	else {
		int to_read = !read_until_eof ? MIN(body_left, read_chunk_size) : read_chunk_size;
		ret.resize(to_read);
		int _offset = 0;
		while (to_read > 0) {
			int rec = 0;
			{
				uint8_t* w = ret.ptrw();
				err = _get_http_data(w + _offset, to_read, rec);
			}
			if (rec <= 0) { // Ended up reading less.
				ret.resize(_offset);
				break;
			}
			else {
				_offset += rec;
				to_read -= rec;
				if (!read_until_eof) {
					body_left -= rec;
				}
			}
			if (err != OK) {
				ret.resize(_offset);
				break;
			}
		}
	}

	if (err != OK) {
		close();

		if (err == ERR_FILE_EOF) {
			status = STATUS_DISCONNECTED; // Server disconnected.
		}
		else {
			status = STATUS_CONNECTION_ERROR;
		}
	}
	else if (body_left == 0 && !chunked && !read_until_eof) {
		status = STATUS_CONNECTED;
	}

	return ret;
}

HTTPClient::Status HTTPClient::get_status() const { return status; }

void HTTPClient::set_blocking_mode(bool p_enable) { blocking = p_enable; }

bool HTTPClient::is_blocking_mode_enabled() const { return blocking; }

Error HTTPClient::_get_http_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	if (blocking) {
		// We can't use StreamPeer.get_data, since when reaching EOF we will get an
		// error without knowing how many bytes we received.
		Error err = ERR_FILE_EOF;
		int read = 0;
		int left = p_bytes;
		r_received = 0;
		while (left > 0) {
			err = connection->get_partial_data(p_buffer + r_received, left, read);
			if (err == OK) {
				r_received += read;
			}
			else if (err == ERR_FILE_EOF) {
				r_received += read;
				return err;
			}
			else {
				return err;
			}
			left -= read;
		}
		return err;
	}
	else {
		return connection->get_partial_data(p_buffer, p_bytes, r_received);
	}
}

void HTTPClient::set_read_chunk_size(int p_size)
{
	ERR_FAIL_COND(p_size < 256 || p_size > (1 << 24));
	read_chunk_size = p_size;
}

int HTTPClient::get_read_chunk_size() const { return read_chunk_size; }

void HTTPClient::set_http_proxy(const String& p_host, int p_port)
{
	if (p_host.is_empty() || p_port == -1) {
		http_proxy_host = "";
		http_proxy_port = -1;
	}
	else {
		http_proxy_host = p_host;
		http_proxy_port = p_port;
	}
}

void HTTPClient::set_https_proxy(const String& p_host, int p_port)
{
	if (p_host.is_empty() || p_port == -1) {
		https_proxy_host = "";
		https_proxy_port = -1;
	}
	else {
		https_proxy_host = p_host;
		https_proxy_port = p_port;
	}
}

HTTPClient::HTTPClient()
{
	tcp_connection.instantiate();
	request_buffer.instantiate();
}

HTTPClient::~HTTPClient() {}

#endif // WEB_ENABLED

// ============================================================================
// Merged from core/io/ip.cpp
// ============================================================================

#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/templates/hash_map.h"
#include "core/io/ip.h"

/************* RESOLVER ******************/

struct _IP_ResolverPrivate
{
	struct QueueItem
	{
		SafeNumeric<IP::ResolverStatus> status;

		List<IPAddress> response;

		String hostname;
		IP::Type type;

		void clear()
		{
			status.set(IP::RESOLVER_STATUS_NONE);
			response.clear();
			type = IP::TYPE_NONE;
			hostname = "";
		}

		QueueItem() { clear(); }
	};

	QueueItem queue[IP::RESOLVER_MAX_QUERIES];

	IP::ResolverID find_empty_id() const
	{
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() == IP::RESOLVER_STATUS_NONE) {
				return i;
			}
		}
		return IP::RESOLVER_INVALID_ID;
	}

	Mutex mutex;
	Semaphore sem;

	Thread thread;
	SafeFlag thread_abort;

	void resolve_queues()
	{
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}

			MutexLock lock(mutex);
			List<IPAddress> response;
			String hostname = queue[i].hostname;
			IP::Type type = queue[i].type;
			lock.temp_unlock();

			// We should not lock while resolving the hostname,
			// only when modifying the queue.
			IP::_resolve_hostname(response, hostname, type);

			lock.temp_relock();
			// Could have been completed by another function, or deleted.
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}
			// We might be overriding another result, but we don't care as long as the result is
			// valid.
			if (response.size()) {
				String key = get_cache_key(hostname, type);
				cache[key] = response;
			}
			queue[i].response = response;
			queue[i].status.set(
				response.is_empty() ? IP::RESOLVER_STATUS_ERROR : IP::RESOLVER_STATUS_DONE);
		}
	}

	static void _thread_function(void* p_self)
	{
		_IP_ResolverPrivate* ipr = static_cast<_IP_ResolverPrivate*>(p_self);

		while (!ipr->thread_abort.is_set()) {
			ipr->sem.wait();
			ipr->resolve_queues();
		}
	}

	HashMap<String, List<IPAddress>> cache;

	static String get_cache_key(const String& p_hostname, IP::Type p_type)
	{
		return itos(p_type) + p_hostname;
	}
};

IPAddress IP::resolve_hostname(const String& p_hostname, IP::Type p_type)
{
	const Vector<String> addresses = resolve_hostname_addresses(p_hostname, p_type);
	return addresses.size() ? (IPAddress)addresses[0] : IPAddress();
}

Vector<String> IP::resolve_hostname_addresses(const String& p_hostname, Type p_type)
{
	ERR_FAIL_NULL_V(data, Vector<String>());
	_IP_ResolverPrivate* resolver = data->resolver;

	List<IPAddress> res;
	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);

	{
		MutexLock lock(resolver->mutex);
		if (resolver->cache.has(key)) {
			res = resolver->cache[key];
		}
		else {
			// This should be run unlocked so the resolver thread can keep resolving
			// other requests.
			lock.temp_unlock();
			_resolve_hostname(res, p_hostname, p_type);
			lock.temp_relock();
			// We might be overriding another result, but we don't care as long as the result is
			// valid.
			if (res.size()) {
				resolver->cache[key] = res;
			}
		}
	}

	Vector<String> result;
	for (const IPAddress& E : res) {
		result.push_back(String(E));
	}
	return result;
}

IP::ResolverID IP::resolve_hostname_queue_item(const String& p_hostname, IP::Type p_type)
{
	ERR_FAIL_NULL_V(data, RESOLVER_INVALID_ID);
	_IP_ResolverPrivate* resolver = data->resolver;

	MutexLock lock(resolver->mutex);

	ResolverID id = resolver->find_empty_id();

	if (id == RESOLVER_INVALID_ID) {
		WARN_PRINT("Out of resolver queries");
		return id;
	}

	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);
	resolver->queue[id].hostname = p_hostname;
	resolver->queue[id].type = p_type;
	if (resolver->cache.has(key)) {
		resolver->queue[id].response = resolver->cache[key];
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_DONE);
	}
	else {
		resolver->queue[id].response = List<IPAddress>();
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_WAITING);
		if (resolver->thread.is_started()) {
			resolver->sem.post();
		}
		else {
			resolver->resolve_queues();
		}
	}

	return id;
}

IP::ResolverStatus IP::get_resolve_item_status(ResolverID p_id)
{
	ERR_FAIL_NULL_V(data, IP::RESOLVER_STATUS_NONE);
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IP::RESOLVER_STATUS_NONE,
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	IP::ResolverStatus res = data->resolver->queue[p_id].status.get();
	if (res == IP::RESOLVER_STATUS_NONE) {
		ERR_PRINT("Condition status == IP::RESOLVER_STATUS_NONE");
		return IP::RESOLVER_STATUS_NONE;
	}
	return res;
}

IPAddress IP::get_resolve_item_address(ResolverID p_id)
{
	ERR_FAIL_NULL_V(data, IPAddress());
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IPAddress(),
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	_IP_ResolverPrivate* resolver = data->resolver;
	MutexLock lock(resolver->mutex);

	if (resolver->queue[p_id].status.get() != IP::RESOLVER_STATUS_DONE) {
		ERR_PRINT(vformat("Resolve of '%s' didn't complete yet.", resolver->queue[p_id].hostname));
		return IPAddress();
	}

	List<IPAddress> res(resolver->queue[p_id].response);

	for (const IPAddress& E : res) {
		if (E.is_valid()) {
			return E;
		}
	}
	return IPAddress();
}

void IP::erase_resolve_item(ResolverID p_id)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_INDEX_MSG(p_id, IP::RESOLVER_MAX_QUERIES,
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	data->resolver->queue[p_id].status.set(IP::RESOLVER_STATUS_NONE);
}

void IP::clear_cache(const String& p_hostname)
{
	ERR_FAIL_NULL(data);
	_IP_ResolverPrivate* resolver = data->resolver;
	MutexLock lock(resolver->mutex);

	if (p_hostname.is_empty()) {
		resolver->cache.clear();
	}
	else {
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_NONE));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV4));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV6));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_ANY));
	}
}

Vector<String> IP::_get_local_addresses()
{
	Vector<String> addresses;
	List<IPAddress> ip_addresses;
	get_local_addresses(&ip_addresses);
	for (const IPAddress& E : ip_addresses) {
		addresses.push_back(String(E));
	}

	return addresses;
}

void IP::get_local_addresses(List<IPAddress>* r_addresses)
{
	HashMap<String, Interface_Info> interfaces;
	get_local_interfaces(&interfaces);
	for (const KeyValue<String, Interface_Info>& E : interfaces) {
		for (const IPAddress& F : E.value.ip_addresses) {
			r_addresses->push_front(F);
		}
	}
}

void IP::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	data->resolver = memnew(_IP_ResolverPrivate);
	data->resolver->thread_abort.clear();
	data->resolver->thread.start(_IP_ResolverPrivate::_thread_function, data->resolver);
}

void IP::finalize()
{
	if (data == nullptr) {
		return;
	}
	if (data->resolver != nullptr) {
		data->resolver->thread_abort.set();
		data->resolver->sem.post();
		if (data->resolver->thread.is_started()) {
			data->resolver->thread.wait_to_finish();
		}
		memdelete(data->resolver);
		data->resolver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

void IP::reset_backend()
{
	_resolve_hostname = &_default_resolve_hostname;
	get_local_interfaces = &_default_get_local_interfaces;
}

// ==========================================================================
// ip_address.cpp
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

#include "core/io/ip.h"
/*
IPAddress::operator Variant() const {
	return operator String();
}*/

IPAddress::operator String() const {
	if (wildcard) {
		return "*";
	}

	if (!valid) {
		return "";
	}

	if (is_ipv4()) {
		// IPv4 address mapped to IPv6.
		return itos(field8[12]) + "." + itos(field8[13]) + "." + itos(field8[14]) + "." + itos(field8[15]);
	}
	String ret;
	for (int i = 0; i < 8; i++) {
		if (i > 0) {
			ret = ret + ":";
		}
		uint16_t num = (field8[i * 2] << 8) + field8[i * 2 + 1];
		ret = ret + String::num_int64(num, 16);
	}

	return ret;
}

bool IPAddress::_parse_ipv6(const String &p_string, IPAddress &r_ip) {
	int len = p_string.length();
	const char32_t *buf = p_string.ptr();

	int cur = 0;
	int shift = -1;
	for (int i = 0; i < len; i++) {
		for (int j = i; j < len; j++) {
			char32_t c = buf[j];
			if (c == ':') {
				if (j + 1 == len) {
					return false; // Can't end with a column (unless part of shortening).
				}
				if (buf[j + 1] == ':') {
					if (shift > -1) {
						return false; // Only one shortening allowed.
					} else if (j == 0) {
						shift = cur;
					} else {
						shift = cur + 1;
					}
					j++;
				} else if (i == j) {
					return false; // Stray column.
				}
				i = j;
				break;
			}
			if (j - i > 3) {
				return false;
			}
			if (c >= '0' && c <= '9') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= c - '0';
			} else if (c >= 'a' && c <= 'f') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= 10 + (c - 'a');
			} else if (c >= 'A' && c <= 'F') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= 10 + (c - 'A');
			} else if (c == '.') {
				// IPv4 mapped IPv6 (e.g. "::FFFF:127.0.0.1").
				if (cur < 1 || r_ip.field16[cur - 1] != 0xFFFF) {
					return false; // IPv6 part must end with FFFF.
				}
				if (shift < 0 && cur != 6) {
					return false; // Needs 5 zeros, and FFFF "0:0:0:0:0:FFFF:127.0.0.1".
				}
				// Only empty bytes allowed before FFFF.
				r_ip.field16[cur] = 0;
				r_ip.field16[cur - 1] = 0;
				while (cur > 0) {
					cur--;
					if (r_ip.field16[cur] != 0) {
						return false;
					}
				}
				r_ip.field16[5] = 0xFFFF;
				return _parse_ipv4(p_string, i, &r_ip.field8[12]);
			} else {
				return false;
			}
			if (j + 1 == len) {
				i = j;
			}
		}
		r_ip.field16[cur] = BSWAP16(r_ip.field16[cur]);
		cur += 1;
		if (cur > 8 || (cur == 8 && i + 1 != len)) {
			return false;
		}
	}
	if (shift < 0) {
		return cur == 8; // Should have parsed 8 16-bits ints.
	} else if (shift > 7) {
		return false; // Can't shorten more than this.
	} else if (shift == cur || cur == 8) {
		return true; // Nothing to do (fields are assumed zeroized).
	}
	// Shift bytes.
	int pad = 8 - cur;
	int blank_end = shift + pad;
	for (int i = 7; i > shift; i--) {
		if (i < blank_end) {
			r_ip.field16[i] = 0;
		} else {
			r_ip.field16[i] = r_ip.field16[i - pad];
			r_ip.field16[i - pad] = 0;
		}
	}
	return true;
}

bool IPAddress::_parse_ipv4(const String &p_string, int p_start, uint8_t *r_dest) {
	int len = p_string.length();
	const char32_t *buf = p_string.ptr();

	int cur = 0;
	uint16_t next = 0;
	bool parsed = false;
	for (int i = p_start; i < len; i++) {
		char32_t c = buf[i];
		if (c == '.') {
			if (!parsed) {
				return false;
			}
			parsed = false;
			r_dest[cur] = next;
			next = 0;
			cur++;
			if (cur > 3) {
				return false;
			}
		} else if (c >= '0' && c <= '9') {
			parsed = true;
			next *= 10;
			next += c - '0';
			if (next > 255) {
				return false;
			}
		} else {
			return false; // Invalid char.
		}
	}
	if (!parsed) {
		return false;
	}
	r_dest[cur] = next;
	return parsed && cur == 3;
}

void IPAddress::clear() {
	memset(&field8[0], 0, sizeof(field8));
	valid = false;
	wildcard = false;
}

bool IPAddress::is_ipv4() const {
	return (field32[0] == 0 && field32[1] == 0 && field16[4] == 0 && field16[5] == 0xffff);
}

const uint8_t *IPAddress::get_ipv4() const {
	ERR_FAIL_COND_V_MSG(!is_ipv4(), &(field8[12]), "IPv4 requested, but current IP is IPv6."); // Not the correct IPv4 (it's an IPv6), but we don't want to return a null pointer risking an engine crash.
	return &(field8[12]);
}

void IPAddress::set_ipv4(const uint8_t *p_ip) {
	clear();
	valid = true;
	field16[5] = 0xffff;
	field32[3] = *((const uint32_t *)p_ip);
}

const uint8_t *IPAddress::get_ipv6() const {
	return field8;
}

void IPAddress::set_ipv6(const uint8_t *p_buf) {
	clear();
	valid = true;
	for (int i = 0; i < 16; i++) {
		field8[i] = p_buf[i];
	}
}

bool IPAddress::is_valid_ip_address(const String &p_string) {
	IPAddress addr;
	if (p_string.length() < IPV6_MAX_STRING_LENGTH && p_string.contains_char(':')) {
		return _parse_ipv6(p_string, addr);
	} else if (p_string.length() < IPV4_MAX_STRING_LENGTH) { // Try IPv4.
		return _parse_ipv4(p_string, 0, &addr.field8[12]);
	}
	return false;
}

IPAddress::IPAddress(const String &p_string) {
	clear();

	if (p_string == "*") {
		// Wildcard (not a valid IP).
		wildcard = true;

	} else if (p_string.length() < IPV6_MAX_STRING_LENGTH && p_string.contains_char(':')) {
		// IPv6.
		valid = _parse_ipv6(p_string, *this);
		ERR_FAIL_COND_MSG(!valid, "Invalid IPv6 address: " + p_string);

	} else if (p_string.length() < IPV4_MAX_STRING_LENGTH) {
		// IPv4 (mapped to IPv6 internally).
		field16[5] = 0xffff;
		valid = _parse_ipv4(p_string, 0, &field8[12]);
		ERR_FAIL_COND_MSG(!valid, "Invalid IPv4 address: " + p_string);

	} else {
		ERR_PRINT("Invalid IP address.");
	}
}

_FORCE_INLINE_ static void _32_to_buf(uint8_t *p_dst, uint32_t p_n) {
	p_dst[0] = (p_n >> 24) & 0xff;
	p_dst[1] = (p_n >> 16) & 0xff;
	p_dst[2] = (p_n >> 8) & 0xff;
	p_dst[3] = (p_n >> 0) & 0xff;
}

IPAddress::IPAddress(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d, bool p_is_v6) {
	clear();
	valid = true;
	if (!p_is_v6) {
		// Mapped to IPv6.
		field16[5] = 0xffff;
		field8[12] = p_a;
		field8[13] = p_b;
		field8[14] = p_c;
		field8[15] = p_d;
	} else {
		_32_to_buf(&field8[0], p_a);
		_32_to_buf(&field8[4], p_b);
		_32_to_buf(&field8[8], p_c);
		_32_to_buf(&field8[12], p_d);
	}
}

// ==========================================================================
// net_socket.cpp
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

#include "core/io/net_socket.h"

Ref<NetSocket> (*NetSocket::_create)() = nullptr;

Ref<NetSocket> NetSocket::create() {
	if (_create) {
		return _create();
	}

	ERR_PRINT("Unable to create network socket, platform not supported");
	return nullptr;
}

// ==========================================================================
// remote_filesystem_client.cpp
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

#include "core/os/os.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/stream_peer.h"
#include "core/os/os.h"
#include "core/string/ustring.h"

#define FILESYSTEM_CACHE_VERSION 1
#define FILESYSTEM_PROTOCOL_VERSION 1
#define PASSWORD_LENGTH 32

#define FILES_SUBFOLDER "remote_filesystem_files"
#define FILES_CACHE_FILE "remote_filesystem.cache"

Vector<RemoteFilesystemClient::FileCache> RemoteFilesystemClient::_load_cache_file() {
	Ref<FileAccess> fa = FileAccess::open(cache_path.path_join(FILES_CACHE_FILE), FileAccess::READ);
	if (fa.is_null()) {
		return Vector<FileCache>(); // No cache, return empty
	}

	int version = fa->get_line().to_int();
	if (version != FILESYSTEM_CACHE_VERSION) {
		return Vector<FileCache>(); // Version mismatch, ignore everything.
	}

	String file_path = cache_path.path_join(FILES_SUBFOLDER);

	Vector<FileCache> file_cache;

	while (!fa->eof_reached()) {
		String l = fa->get_line();
		Vector<String> fields = l.split("::");
		if (fields.size() != 3) {
			break;
		}
		FileCache fc;
		fc.path = fields[0];
		fc.server_modified_time = fields[1].to_int();
		fc.modified_time = fields[2].to_int();

		String full_path = file_path.path_join(fc.path);
		if (!FileAccess::exists(full_path)) {
			continue; // File is gone.
		}

		if (FileAccess::get_modified_time(full_path) != fc.modified_time) {
			DirAccess::remove_absolute(full_path); // Take the chance to remove this file and assume we no longer have it.
			continue;
		}

		file_cache.push_back(fc);
	}

	return file_cache;
}

Error RemoteFilesystemClient::_store_file(const String &p_path, const LocalVector<uint8_t> &p_file, uint64_t &r_modified_time) {
	r_modified_time = 0;
	String full_path = cache_path.path_join(FILES_SUBFOLDER).path_join(p_path);
	String base_file_dir = full_path.get_base_dir();

	if (!validated_directories.has(base_file_dir)) {
		// Verify that path exists before writing file, but only verify once for performance.
		DirAccess::make_dir_recursive_absolute(base_file_dir);
		validated_directories.insert(base_file_dir);
	}

	Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_OPEN, vformat("Unable to open file for writing to remote filesystem cache: '%s'.", p_path));
	f->store_buffer(p_file.ptr(), p_file.size());
	Error err = f->get_error();
	if (err) {
		return err;
	}
	f.unref(); // Unref to ensure file is not locked and modified time can be obtained.

	r_modified_time = FileAccess::get_modified_time(full_path);
	return OK;
}

Error RemoteFilesystemClient::_remove_file(const String &p_path) {
	return DirAccess::remove_absolute(cache_path.path_join(FILES_SUBFOLDER).path_join(p_path));
}
Error RemoteFilesystemClient::_store_cache_file(const Vector<FileCache> &p_cache) {
	String full_path = cache_path.path_join(FILES_CACHE_FILE);
	String base_file_dir = full_path.get_base_dir();
	Error err = DirAccess::make_dir_recursive_absolute(base_file_dir);
	ERR_FAIL_COND_V_MSG(err != OK && err != ERR_ALREADY_EXISTS, err, vformat("Unable to create base directory to store cache file: '%s'.", base_file_dir));

	Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_OPEN, vformat("Unable to open the remote cache file for writing: '%s'.", full_path));
	f->store_line(itos(FILESYSTEM_CACHE_VERSION));
	for (int i = 0; i < p_cache.size(); i++) {
		String l = p_cache[i].path + "::" + itos(p_cache[i].server_modified_time) + "::" + itos(p_cache[i].modified_time);
		f->store_line(l);
	}
	return OK;
}

Error RemoteFilesystemClient::synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path) {
	Error err = _synchronize_with_server(p_host, p_port, p_password, r_cache_path);
	// Ensure no memory is kept
	validated_directories.reset();
	cache_path = String();
	return err;
}

void RemoteFilesystemClient::_update_cache_path(String &r_cache_path) {
	r_cache_path = cache_path.path_join(FILES_SUBFOLDER);
}

Error RemoteFilesystemClient::_synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path) {
	cache_path = r_cache_path;
	{
		Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		dir->change_dir(cache_path);
		cache_path = dir->get_current_dir();
	}

	Ref<StreamPeerSocket> tcp_client;
	tcp_client.instantiate();

	IPAddress ip = p_host.is_valid_ip_address() ? IPAddress(p_host) : IP::resolve_hostname(p_host);
	ERR_FAIL_COND_V_MSG(!ip.is_valid(), ERR_INVALID_PARAMETER, vformat("Unable to resolve remote filesystem server hostname: '%s'.", p_host));
	print_verbose(vformat("Remote Filesystem: Connecting to host %s, port %d.", ip, p_port));
	Error err = tcp_client->connect_to_host(ip, p_port);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Unable to open connection to remote file server (%s, port %d) failed.", String(p_host), p_port));

	while (tcp_client->get_status() == StreamPeerSocket::STATUS_CONNECTING) {
		tcp_client->poll();
		OS::delay_usec(100);
	}

	if (tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		ERR_FAIL_V_MSG(ERR_CANT_CONNECT, vformat("Connection to remote file server (%s, port %d) failed.", String(p_host), p_port));
	}

	// Connection OK, now send the current file state.
	print_verbose("Remote Filesystem: Connection OK.");

	// Header (GRFS) - Godot Remote File System
	print_verbose("Remote Filesystem: Sending header");
	tcp_client->put_u8('G');
	tcp_client->put_u8('R');
	tcp_client->put_u8('F');
	tcp_client->put_u8('S');
	// Protocol version
	tcp_client->put_32(FILESYSTEM_PROTOCOL_VERSION);
	print_verbose("Remote Filesystem: Sending password");
	uint8_t password[PASSWORD_LENGTH]; // Send fixed size password, since it's easier and safe to validate.
	for (int i = 0; i < PASSWORD_LENGTH; i++) {
		if (i < p_password.length()) {
			password[i] = p_password[i];
		} else {
			password[i] = 0;
		}
	}
	tcp_client->put_data(password, PASSWORD_LENGTH);
	print_verbose("Remote Filesystem: Tags.");
	Vector<String> tags;
	{
		tags.push_back(OS::get_identifier());
		switch (OS::get_preferred_texture_format()) {
			case OS::PREFERRED_TEXTURE_FORMAT_S3TC_BPTC: {
				tags.push_back("bptc");
				tags.push_back("s3tc");
			} break;
			case OS::PREFERRED_TEXTURE_FORMAT_ETC2_ASTC: {
				tags.push_back("etc2");
				tags.push_back("astc");
			} break;
		}
	}

	tcp_client->put_32(tags.size());
	for (int i = 0; i < tags.size(); i++) {
		tcp_client->put_utf8_string(tags[i]);
	}
	// Size of compressed list of files
	print_verbose("Remote Filesystem: Sending file list");

	Vector<FileCache> file_cache = _load_cache_file();

	// Encode file cache to send it via network.
	Vector<uint8_t> file_cache_buffer;
	if (file_cache.size()) {
		StringBuilder sbuild;
		for (int64_t i = 0; i < file_cache.size(); i++) {
			sbuild.append(file_cache[i].path);
			sbuild.append("::");
			sbuild.append(itos(file_cache[i].server_modified_time));
			sbuild.append("\n");
		}
		String s = sbuild.as_string();
		CharString cs = s.utf8();
		file_cache_buffer.resize(Compression::get_max_compressed_buffer_size(cs.length(), Compression::MODE_ZSTD));
		const int64_t res_len = Compression::compress(file_cache_buffer.ptrw(), (const uint8_t *)cs.ptr(), cs.length(), Compression::MODE_ZSTD);
		file_cache_buffer.resize(res_len);

		tcp_client->put_32(cs.length()); // Size of buffer uncompressed
		tcp_client->put_32(file_cache_buffer.size()); // Size of buffer compressed
		tcp_client->put_data(file_cache_buffer.ptr(), file_cache_buffer.size()); // Buffer
	} else {
		tcp_client->put_32(0); // No file cache buffer
	}

	tcp_client->poll();
	ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected after sending header.");

	uint32_t file_count = tcp_client->get_u32();

	ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected while waiting for file list");

	LocalVector<uint8_t> file_buffer;

	Vector<FileCache> temp_file_cache;

	HashSet<String> files_processed;
	for (uint32_t i = 0; i < file_count; i++) {
		String file = tcp_client->get_utf8_string();
		ERR_FAIL_COND_V_MSG(file == String(), ERR_CONNECTION_ERROR, "Invalid file name received from remote filesystem.");
		uint64_t server_modified_time = tcp_client->get_u64();
		ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected while waiting for file info.");

		FileCache fc;
		fc.path = file;
		fc.server_modified_time = server_modified_time;
		temp_file_cache.push_back(fc);

		files_processed.insert(file);
	}

	Vector<FileCache> new_file_cache;

	// Get the actual files. As a robustness measure, if the connection is interrupted here, any file not yet received will be considered removed.
	// Since the file changed anyway, this makes it the easiest way to keep robustness.

	bool server_disconnected = false;
	for (uint32_t i = 0; i < file_count; i++) {
		String file = temp_file_cache[i].path;

		if (temp_file_cache[i].server_modified_time == 0 || server_disconnected) {
			// File was removed, or server disconnected before transferring it. Since it's no longer valid, remove anyway.
			_remove_file(file);
			continue;
		}

		uint64_t file_size = tcp_client->get_u64();
		file_buffer.resize(file_size);

		err = tcp_client->get_data(file_buffer.ptr(), file_size);
		if (err != OK) {
			ERR_PRINT(vformat("Error retrieving file from remote filesystem: '%s'.", file));
			server_disconnected = true;
		}

		if (tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			// Early disconnect, stop accepting files.
			server_disconnected = true;
		}

		if (server_disconnected) {
			// No more server, transfer is invalid, remove this file.
			_remove_file(file);
			continue;
		}

		uint64_t modified_time = 0;
		err = _store_file(file, file_buffer, modified_time);
		if (err != OK) {
			server_disconnected = true;
			continue;
		}
		FileCache fc = temp_file_cache[i];
		fc.modified_time = modified_time;
		new_file_cache.push_back(fc);
	}

	print_verbose("Remote Filesystem: Updating the cache file.");

	// Go through the list of local files read initially (file_cache) and see which ones are
	// unchanged (not sent again from the server).
	// These need to be re-saved in the new list (new_file_cache).

	for (int i = 0; i < file_cache.size(); i++) {
		if (files_processed.has(file_cache[i].path)) {
			continue; // This was either added or removed, so skip.
		}
		new_file_cache.push_back(file_cache[i]);
	}

	err = _store_cache_file(new_file_cache);
	ERR_FAIL_COND_V_MSG(err != OK, ERR_FILE_CANT_OPEN, "Error writing the remote filesystem file cache.");

	print_verbose("Remote Filesystem: Update success.");

	_update_cache_path(r_cache_path);
	return OK;
}
#ifdef FILESYSTEM_CACHE_VERSION
#undef FILESYSTEM_CACHE_VERSION
#endif
#ifdef FILESYSTEM_PROTOCOL_VERSION
#undef FILESYSTEM_PROTOCOL_VERSION
#endif
#ifdef PASSWORD_LENGTH
#undef PASSWORD_LENGTH
#endif
#ifdef FILES_SUBFOLDER
#undef FILES_SUBFOLDER
#endif
#ifdef FILES_CACHE_FILE
#undef FILES_CACHE_FILE
#endif

// ============================================================================
// Merged from core/io/packet_peer.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"

void PacketPeer::set_encode_buffer_max_size(int p_max_size)
{
	ERR_FAIL_COND_MSG(p_max_size < 1024, "Max encode buffer must be at least 1024 bytes");
	ERR_FAIL_COND_MSG(p_max_size > 256 * 1024 * 1024, "Max encode buffer cannot exceed 256 MiB");
	encode_buffer_max_size = Math::next_power_of_2((uint32_t)p_max_size);
	encode_buffer.clear();
}

int PacketPeer::get_encode_buffer_max_size() const { return encode_buffer_max_size; }

Error PacketPeer::get_packet_buffer(Vector<uint8_t>& r_buffer)
{
	const uint8_t* buffer;
	int buffer_size;
	Error err = get_packet(&buffer, buffer_size);
	if (err) {
		return err;
	}

	r_buffer.resize(buffer_size);
	if (buffer_size == 0) {
		return OK;
	}

	uint8_t* w = r_buffer.ptrw();
	for (int i = 0; i < buffer_size; i++) {
		w[i] = buffer[i];
	}

	return OK;
}

Error PacketPeer::put_packet_buffer(const Vector<uint8_t>& p_buffer)
{
	int len = p_buffer.size();
	if (len == 0) {
		return OK;
	}

	const uint8_t* r = p_buffer.ptr();
	return put_packet(&r[0], len);
}

Error PacketPeer::_put_packet(const Vector<uint8_t>& p_buffer)
{
	return put_packet_buffer(p_buffer);
}

Vector<uint8_t> PacketPeer::_get_packet()
{
	Vector<uint8_t> raw;
	last_get_error = get_packet_buffer(raw);
	return raw;
}

Error PacketPeer::_get_packet_error() const { return last_get_error; }

Error PacketPeerExtension::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	WARN_PRINT_ONCE("PacketPeerExtension::_get_packet_native is unimplemented!");
	return FAILED;
}

Error PacketPeerExtension::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	WARN_PRINT_ONCE("PacketPeerExtension::_put_packet_native is unimplemented!");
	return FAILED;
}

Error PacketPeerStream::_poll_buffer() const
{
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);

	int read = 0;
	ERR_FAIL_COND_V(input_buffer.size() < ring_buffer.space_left(), ERR_UNAVAILABLE);
	Error err = peer->get_partial_data(input_buffer.ptrw(), ring_buffer.space_left(), read);
	if (err) {
		return err;
	}
	if (read == 0) {
		return OK;
	}

	int w = ring_buffer.write(&input_buffer[0], read);
	ERR_FAIL_COND_V(w != read, ERR_BUG);

	return OK;
}

int PacketPeerStream::get_available_packet_count() const
{
	_poll_buffer();

	uint32_t remaining = ring_buffer.data_left();

	int ofs = 0;
	int count = 0;

	while (remaining >= 4) {
		uint8_t lbuf[4];
		ring_buffer.copy(lbuf, ofs, 4);
		uint32_t len = decode_uint32(lbuf);
		remaining -= 4;
		ofs += 4;
		if (len > remaining) {
			break;
		}
		remaining -= len;
		ofs += len;
		count++;
	}

	return count;
}

Error PacketPeerStream::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);
	_poll_buffer();

	int remaining = ring_buffer.data_left();
	ERR_FAIL_COND_V(remaining < 4, ERR_UNAVAILABLE);
	uint8_t lbuf[4];
	ring_buffer.copy(lbuf, 0, 4);
	remaining -= 4;
	uint32_t len = decode_uint32(lbuf);
	ERR_FAIL_COND_V(remaining < (int)len, ERR_UNAVAILABLE);

	ERR_FAIL_COND_V(input_buffer.size() < (int)len, ERR_UNAVAILABLE);
	ring_buffer.read(lbuf, 4);					// get rid of first 4 bytes
	ring_buffer.read(input_buffer.ptrw(), len); // read packet

	*r_buffer = &input_buffer[0];
	r_buffer_size = len;
	return OK;
}

Error PacketPeerStream::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(peer.is_null(), ERR_UNCONFIGURED);
	Error err = _poll_buffer(); // won't hurt to poll here too

	if (err) {
		return err;
	}

	if (p_buffer_size == 0) {
		return OK;
	}

	ERR_FAIL_COND_V(p_buffer_size < 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_buffer_size + 4 > output_buffer.size(), ERR_INVALID_PARAMETER);

	encode_uint32(p_buffer_size, output_buffer.ptrw());
	uint8_t* dst = &output_buffer.write[4];
	for (int i = 0; i < p_buffer_size; i++) {
		dst[i] = p_buffer[i];
	}

	return peer->put_data(&output_buffer[0], p_buffer_size + 4);
}

int PacketPeerStream::get_max_packet_size() const { return output_buffer.size(); }

void PacketPeerStream::set_stream_peer(const Ref<StreamPeer>& p_peer)
{
	if (p_peer.ptr() != peer.ptr()) {
		ring_buffer.advance_read(ring_buffer.data_left()); // Reset the ring buffer.
	}

	peer = p_peer;
}

Ref<StreamPeer> PacketPeerStream::get_stream_peer() const { return peer; }

void PacketPeerStream::set_input_buffer_max_size(int p_max_size)
{
	ERR_FAIL_COND_MSG(p_max_size < 0, "Max size of input buffer size cannot be smaller than 0.");
	// WARNING: May lose packets.
	ERR_FAIL_COND_MSG(ring_buffer.data_left(), "Buffer in use, resizing would cause loss of data.");
	ring_buffer.resize(
		Math::nearest_shift(Math::next_power_of_2((uint32_t)p_max_size + (uint32_t)4)) - 1);
	input_buffer.resize(Math::next_power_of_2((uint32_t)p_max_size + (uint32_t)4));
}

int PacketPeerStream::get_input_buffer_max_size() const { return input_buffer.size() - 4; }

void PacketPeerStream::set_output_buffer_max_size(int p_max_size)
{
	output_buffer.resize(Math::next_power_of_2((uint32_t)p_max_size + (uint32_t)4));
}

int PacketPeerStream::get_output_buffer_max_size() const { return output_buffer.size() - 4; }

PacketPeerStream::PacketPeerStream()
{
	// TODO: make a way to select dynamically based on available hardware
	int64_t rbsize = 16;

	ring_buffer.resize(rbsize);
	input_buffer.resize(int64_t(1) << rbsize);
	output_buffer.resize(int64_t(1) << rbsize);
}

// ==========================================================================
// packet_peer_udp.cpp
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

#include "core/io/ip.h"
#include "core/io/packet_peer_udp.h"

void PacketPeerUDP::set_blocking_mode(bool p_enable) { blocking = p_enable; }

void PacketPeerUDP::set_broadcast_enabled(bool p_enabled)
{
	ERR_FAIL_COND(udp_server);
	broadcast = p_enabled;
	if (_sock.is_valid() && _sock->is_open()) {
		_sock->set_broadcasting_enabled(p_enabled);
	}
}

Error PacketPeerUDP::join_multicast_group(IPAddress p_multi_address, const String& p_if_name)
{
	ERR_FAIL_COND_V(udp_server, ERR_LOCKED);
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!p_multi_address.is_valid(), ERR_INVALID_PARAMETER);

	if (!_sock->is_open()) {
		IP::Type ip_type = p_multi_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		Error err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);
		ERR_FAIL_COND_V(err != OK, err);
		_sock->set_blocking_enabled(false);
		_sock->set_broadcasting_enabled(broadcast);
	}
	return _sock->join_multicast_group(p_multi_address, p_if_name);
}

Error PacketPeerUDP::leave_multicast_group(IPAddress p_multi_address, const String& p_if_name)
{
	ERR_FAIL_COND_V(udp_server, ERR_LOCKED);
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!_sock->is_open(), ERR_UNCONFIGURED);
	return _sock->leave_multicast_group(p_multi_address, p_if_name);
}

String PacketPeerUDP::_get_packet_ip() const { return String(get_packet_address()); }

Error PacketPeerUDP::_set_dest_address(const String& p_address, int p_port)
{
	IPAddress ip;
	if (p_address.is_valid_ip_address()) {
		ip = p_address;
	}
	else {
		ip = IP::resolve_hostname(p_address);
		if (!ip.is_valid()) {
			return ERR_CANT_RESOLVE;
		}
	}

	set_dest_address(ip, p_port);
	return OK;
}

int PacketPeerUDP::get_available_packet_count() const
{
	// TODO we should deprecate this, and expose poll instead!
	Error err = const_cast<PacketPeerUDP*>(this)->_poll();
	if (err != OK) {
		return -1;
	}

	return queue_count;
}

Error PacketPeerUDP::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	Error err = _poll();
	if (err != OK) {
		return err;
	}
	if (queue_count == 0) {
		return ERR_UNAVAILABLE;
	}

	/* Bogus GCC warning here:
	 * In member function 'int RingBuffer<T>::read(T*, int, bool) [with T = unsigned char]',
	 *     inlined from 'virtual Error PacketPeerUDP::get_packet(const uint8_t**, int&)' at
	 * core/io/packet_peer_udp.cpp:112:9, inlined from 'virtual Error
	 * PacketPeerUDP::get_packet(const uint8_t**, int&)' at core/io/packet_peer_udp.cpp:99:7: Error:
	 * ./core/ring_buffer.h:68:46: error: writing 1 byte into a region of size 0
	 * [-Werror=stringop-overflow=] 68 |                                 p_buf[dst++] = read[pos +
	 * i]; |                                 ~~~~~~~~~~~~~^~~~~~~
	 */
	VLTR_GCC_WARNING_PUSH
	VLTR_GCC_PRAGMA(
		GCC diagnostic warning "-Wstringop-overflow=0") // Can't "ignore" this for some reason.

	uint32_t size = 0;
	uint8_t ipv6[16] = {};
	rb.read(ipv6, 16, true);
	packet_ip.set_ipv6(ipv6);
	rb.read((uint8_t*)&packet_port, 4, true);
	rb.read((uint8_t*)&size, 4, true);
	rb.read(packet_buffer, size, true);
	--queue_count;
	*r_buffer = packet_buffer;
	r_buffer_size = size;

	VLTR_GCC_WARNING_POP

	return OK;
}

Error PacketPeerUDP::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!peer_addr.is_valid(), ERR_UNCONFIGURED);

	Error err;
	int sent = -1;

	if (!_sock->is_open()) {
		IP::Type ip_type = peer_addr.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);
		ERR_FAIL_COND_V(err != OK, err);
		_sock->set_blocking_enabled(false);
		_sock->set_broadcasting_enabled(broadcast);
	}

	do {
		if (connected && !udp_server) {
			err = _sock->send(p_buffer, p_buffer_size, sent);
		}
		else {
			err = _sock->sendto(p_buffer, p_buffer_size, sent, peer_addr, peer_port);
		}
		if (err != OK) {
			if (err != ERR_BUSY) {
				return FAILED;
			}
			else if (!blocking) {
				return ERR_BUSY;
			}
			// Keep trying to send full packet
			continue;
		}
		return OK;

	} while (sent != p_buffer_size);

	return OK;
}

int PacketPeerUDP::get_max_packet_size() const
{
	return 512; // uhm maybe not
}

Error PacketPeerUDP::bind(int p_port, const IPAddress& p_bind_address, int p_recv_buffer_size)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(
		!p_bind_address.is_valid() && !p_bind_address.is_wildcard(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_port < 0 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The local port number must be between 0 and 65535 (inclusive).");

	Error err;
	IP::Type ip_type = IP::TYPE_ANY;

	if (p_bind_address.is_valid()) {
		ip_type = p_bind_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	}

	err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);

	if (err != OK) {
		return ERR_CANT_CREATE;
	}

	_sock->set_blocking_enabled(false);
	_sock->set_broadcasting_enabled(broadcast);
	NetSocket::Address addr(p_bind_address, p_port);
	err = _sock->bind(addr);

	if (err != OK) {
		_sock->close();
		return err;
	}
	rb.resize(Math::nearest_shift((uint32_t)p_recv_buffer_size));
	return OK;
}

Error PacketPeerUDP::connect_shared_socket(
	Ref<NetSocket> p_sock, IPAddress p_ip, uint16_t p_port, UDPServer* p_server)
{
	udp_server = p_server;
	connected = true;
	_sock = p_sock;
	peer_addr = p_ip;
	peer_port = p_port;
	packet_ip = peer_addr;
	packet_port = peer_port;
	return OK;
}

void PacketPeerUDP::disconnect_shared_socket()
{
	udp_server = nullptr;
	_sock = Ref<NetSocket>(NetSocket::create());
	close();
}

Error PacketPeerUDP::connect_to_host(const IPAddress& p_host, int p_port)
{
	ERR_FAIL_COND_V(udp_server, ERR_LOCKED);
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!p_host.is_valid(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V_MSG(p_port < 1 || p_port > 65535, ERR_INVALID_PARAMETER,
		"The remote port number must be between 1 and 65535 (inclusive).");

	Error err;

	if (!_sock->is_open()) {
		IP::Type ip_type = p_host.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
		err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);
		ERR_FAIL_COND_V(err != OK, ERR_CANT_OPEN);
		_sock->set_blocking_enabled(false);
	}

	NetSocket::Address addr(p_host, p_port);
	err = _sock->connect_to_host(addr);

	// I see no reason why we should get ERR_BUSY (wouldblock/eagain) here.
	// This is UDP, so connect is only used to tell the OS to which socket
	// it should deliver packets when multiple are bound on the same address/port.
	if (err != OK) {
		close();
		ERR_FAIL_V_MSG(FAILED, "Unable to connect");
	}

	connected = true;

	peer_addr = p_host;
	peer_port = p_port;

	// Flush any packet we might still have in queue.
	rb.clear();
	return OK;
}

bool PacketPeerUDP::is_socket_connected() const { return connected; }

void PacketPeerUDP::close()
{
	if (udp_server) {
		udp_server->remove_peer(peer_addr, peer_port);
		udp_server = nullptr;
		_sock = Ref<NetSocket>(NetSocket::create());
	}
	else if (_sock.is_valid()) {
		_sock->close();
	}
	rb.resize(16);
	queue_count = 0;
	connected = false;
}

Error PacketPeerUDP::wait()
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	return _sock->poll(NetSocket::POLL_TYPE_IN, -1);
}

Error PacketPeerUDP::_poll()
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);

	if (!_sock->is_open()) {
		return FAILED;
	}
	if (udp_server) {
		return OK; // Handled by UDPServer.
	}

	Error err;
	int read;
	IPAddress ip;
	uint16_t port;

	while (true) {
		if (connected) {
			err = _sock->recv(recv_buffer, sizeof(recv_buffer), read);
			ip = peer_addr;
			port = peer_port;
		}
		else {
			err = _sock->recvfrom(recv_buffer, sizeof(recv_buffer), read, ip, port);
		}

		if (err != OK) {
			if (err == ERR_BUSY) {
				break;
			}
			return FAILED;
		}

		err = store_packet(ip, port, recv_buffer, read);
#ifdef TOOLS_ENABLED
		if (err != OK) {
			WARN_PRINT("Buffer full, dropping packets!");
		}
#endif
	}

	return OK;
}

Error PacketPeerUDP::store_packet(IPAddress p_ip, uint32_t p_port, uint8_t* p_buf, int p_buf_size)
{
	if (rb.space_left() < p_buf_size + 24) {
		return ERR_OUT_OF_MEMORY;
	}
	rb.write(p_ip.get_ipv6(), 16);
	rb.write((uint8_t*)&p_port, 4);
	rb.write((uint8_t*)&p_buf_size, 4);
	rb.write(p_buf, p_buf_size);
	++queue_count;
	return OK;
}

bool PacketPeerUDP::is_bound() const { return _sock.is_valid() && _sock->is_open(); }

IPAddress PacketPeerUDP::get_packet_address() const { return packet_ip; }

int PacketPeerUDP::get_packet_port() const { return packet_port; }

int PacketPeerUDP::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

void PacketPeerUDP::set_dest_address(const IPAddress& p_address, int p_port)
{
	ERR_FAIL_COND_MSG(connected, "Destination address cannot be set for connected sockets");
	peer_addr = p_address;
	peer_port = p_port;
}

PacketPeerUDP::PacketPeerUDP() : _sock(Ref<NetSocket>(NetSocket::create())) { rb.resize(16); }

PacketPeerUDP::~PacketPeerUDP() { close(); }

// ==========================================================================
// packet_peer_dtls.cpp
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

#include "core/io/packet_peer_dtls.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_MBEDTLS_ENABLED

void PacketPeerDTLS::poll() {}
Error PacketPeerDTLS::connect_to_peer(
	Ref<PacketPeerUDP> p_base, const String& p_hostname, Ref<TLSOptions> p_options)
{
	return ERR_UNAVAILABLE;
}
void PacketPeerDTLS::disconnect_from_peer() {}
PacketPeerDTLS::Status PacketPeerDTLS::get_status() const { return STATUS_DISCONNECTED; }

Error PacketPeerDTLS::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	r_buffer_size = 0;
	return ERR_UNAVAILABLE;
}
Error PacketPeerDTLS::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	return ERR_UNAVAILABLE;
}
int PacketPeerDTLS::get_available_packet_count() const { return 0; }
int PacketPeerDTLS::get_max_packet_size() const { return 0; }

PacketPeerDTLS* PacketPeerDTLS::create(bool p_notify_postinitialize) { return nullptr; }
bool PacketPeerDTLS::is_available() { return false; }

PacketPeerDTLS::PacketPeerDTLS() {}
PacketPeerDTLS::~PacketPeerDTLS() {}

#endif // !MODULE_MBEDTLS_ENABLED

// ==========================================================================
// udp_server.cpp
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

#include "core/io/packet_peer_udp.h"

Error UDPServer::poll()
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	if (!_sock->is_open()) {
		return ERR_UNCONFIGURED;
	}
	Error err;
	int read;
	IPAddress ip;
	uint16_t port;
	while (true) {
		err = _sock->recvfrom(recv_buffer, sizeof(recv_buffer), read, ip, port);
		if (err != OK) {
			if (err == ERR_BUSY) {
				break;
			}
			return FAILED;
		}
		Peer p;
		p.ip = ip;
		p.port = port;
		List<Peer>::Element* E = peers.find(p);
		if (!E) {
			E = pending.find(p);
		}
		if (E) {
			E->get().peer->store_packet(ip, port, recv_buffer, read);
		}
		else {
			if (pending.size() >= max_pending_connections) {
				// Drop connection.
				continue;
			}
			// It's a new peer, add it to the pending list.
			Peer peer;
			peer.ip = ip;
			peer.port = port;
			Ref<PacketPeerUDP> peer_ref = memnew(PacketPeerUDP);
			peer.peer = peer_ref.ptr();
			peer.peer->connect_shared_socket(_sock, ip, port, this);
			peer.peer->store_packet(ip, port, recv_buffer, read);
			pending.push_back(peer);

			// FIXME: peer.peer is intentionally leaked such that it can destruct
			//        when the client no longer needs it.
			//        The current system 'abuses' the refcount_init semantics, such
			//        that the first take_connection call takes ownership. A refactor
			//        with a better "transfer ownership" semantics is warranted to avoid
			//        accidental leaks.
			peer_ref->reference();
		}
	}
	return OK;
}

Error UDPServer::listen(uint16_t p_port, const IPAddress& p_bind_address)
{
	ERR_FAIL_COND_V(_sock.is_null(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(_sock->is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(
		!p_bind_address.is_valid() && !p_bind_address.is_wildcard(), ERR_INVALID_PARAMETER);

	Error err;
	IP::Type ip_type = IP::TYPE_ANY;

	if (p_bind_address.is_valid()) {
		ip_type = p_bind_address.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	}

	err = _sock->open(NetSocket::Family::INET, NetSocket::TYPE_UDP, ip_type);

	if (err != OK) {
		return ERR_CANT_CREATE;
	}

	_sock->set_blocking_enabled(false);
	_sock->set_reuse_address_enabled(true);
	NetSocket::Address addr(p_bind_address, p_port);
	err = _sock->bind(addr);

	if (err != OK) {
		stop();
		return err;
	}
	return OK;
}

int UDPServer::get_local_port() const
{
	NetSocket::Address addr;
	_sock->get_socket_address(&addr);
	return addr.port();
}

bool UDPServer::is_listening() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	return _sock->is_open();
}

bool UDPServer::is_connection_available() const
{
	ERR_FAIL_COND_V(_sock.is_null(), false);

	if (!_sock->is_open()) {
		return false;
	}

	return pending.size() > 0;
}

void UDPServer::set_max_pending_connections(int p_max)
{
	ERR_FAIL_COND_MSG(p_max < 0, "Max pending connections value must be a positive number (0 means "
								 "refuse new connections).");
	max_pending_connections = p_max;
	while (p_max > pending.size()) {
		List<Peer>::Element* E = pending.back();
		if (!E) {
			break;
		}
		memdelete(E->get().peer);
		pending.erase(E);
	}
}

int UDPServer::get_max_pending_connections() const { return max_pending_connections; }

Ref<PacketPeerUDP> UDPServer::take_connection()
{
	Ref<PacketPeerUDP> conn;
	if (!is_connection_available()) {
		return conn;
	}

	Peer peer = pending.front()->get();
	pending.pop_front();
	peers.push_back(peer);
	return peer.peer;
}

void UDPServer::remove_peer(IPAddress p_ip, int p_port)
{
	Peer peer;
	peer.ip = p_ip;
	peer.port = p_port;
	List<Peer>::Element* E = peers.find(peer);
	if (E) {
		peers.erase(E);
	}
}

void UDPServer::stop()
{
	if (_sock.is_valid()) {
		_sock->close();
	}
	List<Peer>::Element* E = peers.front();
	while (E) {
		E->get().peer->disconnect_shared_socket();
		// Don't delete peer; it's intentionally weakly owned.
		E = E->next();
	}
	E = pending.front();
	while (E) {
		E->get().peer->disconnect_shared_socket();
		memdelete(E->get().peer);
		E = E->next();
	}
	peers.clear();
	pending.clear();
}

UDPServer::UDPServer() : _sock(Ref<NetSocket>(NetSocket::create())) {}

UDPServer::~UDPServer() { stop(); }

// ==========================================================================
// dtls_server.cpp
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

#include "core/io/packet_peer_dtls.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_MBEDTLS_ENABLED

bool DTLSServer::is_available() { return false; }
DTLSServer* DTLSServer::create(bool p_notify_postinitialize) { return nullptr; }

Error DTLSServer::setup(Ref<TLSOptions> p_options) { return ERR_UNAVAILABLE; }
void DTLSServer::stop() {}
Ref<PacketPeerDTLS> DTLSServer::take_connection(Ref<PacketPeerUDP> p_peer)
{
	return Ref<PacketPeerDTLS>();
}

DTLSServer::DTLSServer() {}
DTLSServer::~DTLSServer() {}

#endif // !MODULE_MBEDTLS_ENABLED

// === Merged from core/io/resource_loader.cpp ===
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/os/condition_variable.h"
#include "core/os/os.h"
#include "core/os/condition_variable.h"
#include "core/string/print_string.h"
#include "core/string/translation_server.h"
#include "core/templates/rb_set.h"
#include "core/io/resource_loader.h"
#include "servers/rendering/renderer.h"

#ifdef DEBUG_LOAD_THREADED
#define print_lt(m_text) print_line(m_text)
#else
#define print_lt(m_text)
#endif

Ref<ResourceFormatLoader> ResourceLoader::loader[ResourceLoader::MAX_LOADERS];

int ResourceLoader::loader_count = 0;

bool ResourceFormatLoader::recognize_path(const String& p_path, const String& p_for_type) const
{
	List<String> extensions;
	if (p_for_type.is_empty()) {
		get_recognized_extensions(&extensions);
	}
	else {
		get_recognized_extensions_for_type(p_for_type, &extensions);
	}

	for (const String& E : extensions) {
		const String ext = !E.begins_with(".") ? "." + E : E;
		if (p_path.right(ext.length()).nocasecmp_to(ext) == 0) {
			return true;
		}
	}

	return false;
}

void ResourceFormatLoader::get_classes_used(const String& p_path, HashSet<String>* r_classes)
{
	Vector<String> ret;
	for (int i = 0; i < ret.size(); i++) {
		r_classes->insert(ret[i]);
	}

	String res = get_resource_type(p_path);
	if (!res.is_empty()) {
		r_classes->insert(res);
	}
}

ResourceUID::ID ResourceFormatLoader::get_resource_uid(const String& p_path) const
{
	int64_t uid = ResourceUID::INVALID_ID;
	Ref<FileAccess> file = FileAccess::open(p_path + ".uid", FileAccess::READ);
	if (file.is_valid()) {
		uid = ResourceUID::text_to_id(file->get_line());
	}
	return uid;
}

void ResourceFormatLoader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty() || handles_type(p_type)) {
		get_recognized_extensions(p_extensions);
	}
}

void ResourceLoader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions)
{
	for (int i = 0; i < loader_count; i++) {
		loader[i]->get_recognized_extensions_for_type(p_type, p_extensions);
	}
}

bool ResourceFormatLoader::exists(const String& p_path) const
{
	return FileAccess::exists(p_path); // By default just check file.
}

void ResourceFormatLoader::get_recognized_extensions(List<String>* p_extensions) const
{
	PackedStringArray exts;
	const String* r = exts.ptr();
	for (int i = 0; i < exts.size(); ++i) {
		p_extensions->push_back(r[i]);
	}
}

Error ResourceFormatLoader::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	return OK;
}

void ResourceFormatLoader::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	PackedStringArray deps;
	const String* r = deps.ptr();
	for (int i = 0; i < deps.size(); ++i) {
		p_dependencies->push_back(r[i]);
	}
}

void ResourceLoader::LoadToken::clear() {}

ResourceLoader::LoadToken::~LoadToken() { clear(); }

Ref<Resource> ResourceLoader::_load(const String& p_path, const String& p_original_path,
	const String& p_type_hint, CacheMode p_cache_mode, Error* r_error, bool p_use_sub_threads,
	float* r_progress)
{
	const String& original_path = p_original_path.is_empty() ? p_path : p_original_path;
	load_nesting++;

	print_verbose(vformat("Loading resource: %s remapped: %s", p_path, _path_remap(p_path)));

	// Try all loaders and pick the first match for the type hint
	bool found = false;
	Ref<Resource> res;
	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(p_path, p_type_hint)) {
			continue;
		}
		found = true;
		res = loader[i]->load(
			p_path, original_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
		if (res.is_valid()) {
			break;
		}
	}

	res_ref_overrides.erase(load_nesting);
	load_nesting--;

	if (res.is_valid()) {
		return res;
	}
	else {
		print_verbose(vformat("Failed loading resource: %s", p_path));
	}

#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint()) {
		if (ResourceFormatImporter::get_importer_by_file(p_path).is_valid()) {
			// The format is known to the editor, but the file hasn't been imported
			// (otherwise, ResourceFormatImporter would have been found as a suitable loader).
			found = true;
			if (r_error) {
				*r_error = ERR_FILE_NOT_FOUND;
			}
		}
	}
#endif

	ERR_FAIL_COND_V_MSG(found, Ref<Resource>(), vformat("Failed loading resource: %s.", p_path));

#ifdef TOOLS_ENABLED
	Ref<FileAccess> file_check = FileAccess::create(FileAccess::ACCESS_RESOURCES);
	if (!file_check->file_exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		ERR_FAIL_V_MSG(
			Ref<Resource>(), vformat("Resource file not found: %s (expected type: %s)", p_path,
								 !p_type_hint.is_empty() ? p_type_hint : "unknown"));
	}
#endif

	if (r_error) {
		*r_error = ERR_FILE_UNRECOGNIZED;
	}
	ERR_FAIL_V_MSG(Ref<Resource>(), vformat("No loader found for resource: %s (expected type: %s)",
										p_path, !p_type_hint.is_empty() ? p_type_hint : "unknown"));
}

// This implementation must allow re-entrancy for a task that started awaiting in a deeper stack
// frame. The load task token must be manually re-referenced before this is called, which includes
// threaded runs.

String ResourceLoader::_validate_local_path(const String& p_path)
{
	ResourceUID::ID uid = ResourceUID::text_to_id(p_path);
	if (uid != ResourceUID::INVALID_ID) {
		return ResourceUID::get_id_path(uid);
	}
	else if (p_path.is_relative_path()) {
		return ("res://" + p_path).simplify_path();
	}
	else {
		return ProjectSettings::localize_path(p_path);
	}
}

Error ResourceLoader::load_threaded_request(
	const String& p_path, const String& p_type_hint, bool p_use_sub_threads, CacheMode p_cache_mode)
{
	Ref<ResourceLoader::LoadToken> token = _load_start(p_path, p_type_hint,
		p_use_sub_threads ? LOAD_THREAD_DISTRIBUTE : LOAD_THREAD_SPAWN_SINGLE, p_cache_mode, true);
	return token.is_valid() ? OK : FAILED;
}

ResourceLoader::LoadToken* ResourceLoader::_load_threaded_request_reuse_user_token(
	const String& p_path)
{
	HashMap<String, LoadToken*>::Iterator E = user_load_tokens.find(p_path);
	if (E) {
		print_verbose("load_threaded_request(): Another threaded load for resource path '" +
					  p_path + "' has been initiated. Not an error.");
		LoadToken* token = E->value;
		token->user_rc++;
		return token;
	}
	else {
		return nullptr;
	}
}

void ResourceLoader::_load_threaded_request_setup_user_token(
	LoadToken* p_token, const String& p_path)
{
	p_token->user_path = p_path;
	p_token->reference(); // Extra RC until all user requests have been gotten.
	p_token->user_rc = 1;
	user_load_tokens[p_path] = p_token;
	print_lt("REQUEST: user load tokens: " + itos(user_load_tokens.size()));
}

void ResourceLoader::resource_changed_emit(Resource* p_source) {}

Ref<Resource> ResourceLoader::load(
	const String& p_path, const String& p_type_hint, CacheMode p_cache_mode, Error* r_error)
{
	if (r_error) {
		*r_error = OK;
	}

	LoadThreadMode thread_mode = LOAD_THREAD_FROM_CURRENT;
	Ref<LoadToken> load_token = _load_start(p_path, p_type_hint, thread_mode, p_cache_mode);
	if (load_token.is_null()) {
		if (r_error) {
			*r_error = FAILED;
		}
		return Ref<Resource>();
	}

	Ref<Resource> res = _load_complete(*load_token.ptr(), r_error);
	return res;
}

Ref<ResourceLoader::LoadToken> ResourceLoader::_load_start(const String& p_path,
	const String& p_type_hint, LoadThreadMode p_thread_mode, CacheMode p_cache_mode,
	bool p_for_user)
{
	String local_path = _validate_local_path(p_path);
	ERR_FAIL_COND_V(local_path.is_empty(), Ref<ResourceLoader::LoadToken>());

	bool ignoring_cache =
		p_cache_mode == CACHE_MODE_IGNORE || p_cache_mode == CACHE_MODE_IGNORE_DEEP;

	Ref<LoadToken> load_token;
	bool must_not_register = false;
	ThreadLoadTask* load_task_ptr = nullptr;
	{
		MutexLock thread_load_lock(thread_load_mutex);

		if (p_for_user) {
			LoadToken* existing_token = _load_threaded_request_reuse_user_token(p_path);
			if (existing_token) {
				return Ref<LoadToken>(existing_token);
			}
		}

		if (!ignoring_cache && thread_load_tasks.has(local_path)) {
			load_token = Ref<LoadToken>(thread_load_tasks[local_path].load_token);
			if (load_token.is_valid()) {
				if (p_for_user) {
					// Load task exists, with no user tokens at the moment.
					// Let's "attach" to it.
					_load_threaded_request_setup_user_token(load_token.ptr(), p_path);
				}
				return load_token;
			}
			else {
				// The token is dying (reached 0 on another thread).
				// Ensure it's killed now so the path can be safely reused right away.
				thread_load_tasks[local_path].load_token->clear();
			}
		}

		load_token.instantiate();
		load_token->local_path = local_path;
		if (p_for_user) {
			_load_threaded_request_setup_user_token(load_token.ptr(), p_path);
		}

		// create load task
		{
			ThreadLoadTask load_task;

			load_task.load_token = load_token.ptr();
			load_task.local_path = local_path;
			load_task.type_hint = p_type_hint;
			load_task.cache_mode = p_cache_mode;
			load_task.use_sub_threads = p_thread_mode == LOAD_THREAD_DISTRIBUTE;
			if (p_cache_mode == CACHE_MODE_REUSE) {
				Ref<Resource> existing = ResourceCache::get_ref(local_path);
				if (existing.is_valid()) {
					// referencing is fine
					load_task.resource = existing;
					load_task.status = THREAD_LOAD_LOADED;
					load_task.progress = 1.0;
					DEV_ASSERT(!thread_load_tasks.has(local_path));
					thread_load_tasks[local_path] = load_task;
					return load_token;
				}
			}

			// Task hierarchy
			if (curr_load_task) {
				load_task.parent_task = curr_load_task;
				curr_load_task->sub_tasks.insert(load_task.local_path);
			}

			// If we want to ignore cache, but there's another task loading it, we can't add this
			// one to the map.
			must_not_register = ignoring_cache && thread_load_tasks.has(local_path);
			if (must_not_register) {
				load_token->task_if_unregistered = memnew(ThreadLoadTask(load_task));
				load_task_ptr = load_token->task_if_unregistered;
			}
			else {
				DEV_ASSERT(!thread_load_tasks.has(local_path));
				HashMap<String, ResourceLoader::ThreadLoadTask>::Iterator E =
					thread_load_tasks.insert(local_path, load_task);
				load_task_ptr = &E->value;
			}
		}

		// It's important to keep the token alive because until the load completes,
		// which includes before the thread start, it may happen that no one is grabbing
		// the token anymore so it's released.
		load_task_ptr->load_token->reference();

		if (p_thread_mode == LOAD_THREAD_FROM_CURRENT) {
			load_task_ptr->thread_id = Thread::get_caller_id();
		}
	} // MutexLock(thread_load_mutex).

	if (p_thread_mode == LOAD_THREAD_FROM_CURRENT) {
		_run_load_task(load_task_ptr);
	}

	return load_token;
}

float ResourceLoader::_dependency_get_progress(const String& p_path)
{
	if (thread_load_tasks.has(p_path)) {
		ThreadLoadTask& load_task = thread_load_tasks[p_path];
		if (load_task.in_progress_check) {
			// Given the fact that any resource loaded when an outer stack frame is
			// loading another one is considered a dependency of it, for progress
			// tracking purposes, a cycle can happen if even if the original resource
			// graphs involved have none. For instance, preload() can cause this.
			return load_task.max_reported_progress;
		}
		load_task.in_progress_check = true;
		float current_progress = 0.0;
		int dep_count = load_task.sub_tasks.size();
		if (dep_count > 0) {
			for (const String& E : load_task.sub_tasks) {
				current_progress += _dependency_get_progress(E);
			}
			current_progress /= float(dep_count);
			current_progress *= 0.5;
			current_progress += load_task.progress * 0.5;
		}
		else {
			current_progress = load_task.progress;
		}
		load_task.max_reported_progress = MAX(load_task.max_reported_progress, current_progress);
		load_task.in_progress_check = false;
		return load_task.max_reported_progress;
	}
	else {
		return 1.0; // assume finished loading it so it no longer exists
	}
}

ResourceLoader::ThreadLoadStatus ResourceLoader::load_threaded_get_status(
	const String& p_path, float* r_progress)
{
	bool ensure_progress = false;
	ThreadLoadStatus status = THREAD_LOAD_IN_PROGRESS;
	{
		MutexLock thread_load_lock(thread_load_mutex);

		if (!user_load_tokens.has(p_path)) {
			print_verbose("load_threaded_get_status(): No threaded load for resource path '" +
						  p_path +
						  "' has been initiated or its result has already been collected.");
			return THREAD_LOAD_INVALID_RESOURCE;
		}

		String local_path = _validate_local_path(p_path);
		LoadToken* load_token = user_load_tokens[p_path];
		ThreadLoadTask* load_task_ptr;

		if (load_token->task_if_unregistered) {
			load_task_ptr = load_token->task_if_unregistered;
		}
		else {
			ERR_FAIL_COND_V_MSG(!thread_load_tasks.has(local_path), THREAD_LOAD_INVALID_RESOURCE,
				"Bug in ResourceLoader logic, please report.");
			load_task_ptr = &thread_load_tasks[local_path];
		}

		status = load_task_ptr->status;
		if (r_progress) {
			*r_progress = _dependency_get_progress(local_path);
		}

		// Support userland polling in a loop on the main thread.
		if (Thread::is_main_thread() && status == THREAD_LOAD_IN_PROGRESS) {
			uint64_t frame = Engine::get_process_frames();
			if (frame == load_task_ptr->last_progress_check_main_thread_frame) {
				ensure_progress = true;
			}
			else {
				load_task_ptr->last_progress_check_main_thread_frame = frame;
			}
		}
	}

	if (ensure_progress) {
		_ensure_load_progress();
	}

	return status;
}

Ref<Resource> ResourceLoader::load_threaded_get(const String& p_path, Error* r_error)
{
	if (r_error) {
		*r_error = OK;
	}

	Ref<Resource> res;
	{
		MutexLock thread_load_lock(thread_load_mutex);

		if (!user_load_tokens.has(p_path)) {
			print_verbose("load_threaded_get(): No threaded load for resource path '" + p_path +
						  "' has been initiated or its result has already been collected.");
			if (r_error) {
				*r_error = ERR_INVALID_PARAMETER;
			}
			return Ref<Resource>();
		}

		LoadToken* load_token = user_load_tokens[p_path];
		DEV_ASSERT(load_token->user_rc >= 1);

		// Support userland requesting on the main thread before the load is reported to be
		// complete.
		if (Thread::is_main_thread() && !load_token->local_path.is_empty()) {
			ThreadLoadTask* load_task_ptr;

			if (load_token->task_if_unregistered) {
				load_task_ptr = load_token->task_if_unregistered;
			}
			else {
				if (!thread_load_tasks.has(load_token->local_path)) {
					print_error("Bug in ResourceLoader logic, please report.");
					if (r_error) {
						*r_error = ERR_BUG;
					}
					return Ref<Resource>();
				}

				load_task_ptr = &thread_load_tasks[load_token->local_path];
			}

			while (load_task_ptr->status == THREAD_LOAD_IN_PROGRESS) {
				thread_load_lock.temp_unlock();
				bool exit = !_ensure_load_progress();
				OS::delay_usec(1000);

				thread_load_lock.temp_relock();
				if (exit) {
					break;
				}
			}
		}

		res = _load_complete_inner(*load_token, r_error, thread_load_lock);

		load_token->user_rc--;
		if (load_token->user_rc == 0) {
			load_token->user_path.clear();
			user_load_tokens.erase(p_path);
			if (load_token->unreference()) {
				memdelete(load_token);
				load_token = nullptr;
			}
		}
	}

	print_lt("GET: user load tokens: " + itos(user_load_tokens.size()));

	return res;
}

Ref<Resource> ResourceLoader::_load_complete(LoadToken& p_load_token, Error* r_error)
{
	MutexLock thread_load_lock(thread_load_mutex);
	return _load_complete_inner(p_load_token, r_error, thread_load_lock);
}

void ResourceLoader::set_is_import_thread(bool p_import_thread) { import_thread = p_import_thread; }

void ResourceLoader::notify_dependency_error(
	const String& p_path, const String& p_dependency, const String& p_type)
{
	if (dep_err_notify) {
		if (Thread::get_caller_id() == Thread::get_main_id()) {
			dep_err_notify(p_path, p_dependency, p_type);
		}
	}
}

Ref<Resource> ResourceLoader::_load_complete_inner(LoadToken& p_load_token, Error* r_error,
	MutexLock<SafeBinaryMutex<BINARY_MUTEX_TAG>>& p_thread_load_lock)
{
	if (r_error) {
		*r_error = OK;
	}

	ThreadLoadTask* load_task_ptr = nullptr;
	if (p_load_token.task_if_unregistered) {
		load_task_ptr = p_load_token.task_if_unregistered;
	}
	else {
		if (!thread_load_tasks.has(p_load_token.local_path)) {
			if (r_error) {
				*r_error = ERR_BUG;
			}
			ERR_FAIL_V_MSG(Ref<Resource>(), "Bug in ResourceLoader logic, please report.");
		}

		ThreadLoadTask& load_task = thread_load_tasks[p_load_token.local_path];

		if (load_task.status == THREAD_LOAD_IN_PROGRESS) {
			DEV_ASSERT((load_task.task_id == 0) != (load_task.thread_id == 0));
			if (load_task.need_wait) {
				// Loading thread is main or user thread.
				if (!load_task.cond_var) {
					load_task.cond_var = memnew(ConditionVariable);
				}
				load_task.awaiters_count++;
				do {
					load_task.cond_var->wait(p_thread_load_lock);
					DEV_ASSERT(thread_load_tasks.has(p_load_token.local_path) &&
							   p_load_token.get_reference_count());
				} while (load_task.need_wait);
				load_task.awaiters_count--;
				if (load_task.awaiters_count == 0) {
					memdelete(load_task.cond_var);
					load_task.cond_var = nullptr;
				}

				DEV_ASSERT(load_task.status == THREAD_LOAD_FAILED ||
						   load_task.status == THREAD_LOAD_LOADED);
			}
		}

		if (cleaning_tasks) {
			load_task.resource = Ref<Resource>();
			load_task.error = FAILED;
		}

		load_task_ptr = &load_task;
	}

	Ref<Resource> resource = load_task_ptr->resource;
	if (r_error) {
		*r_error = load_task_ptr->error;
	}

	if (resource.is_valid()) {
		if (load_task_ptr->parent_task) {
			if (!load_task_ptr->connections_propagated) {
				// A task awaiting another => Let the awaiter accumulate the resource changed
				// connections.
				DEV_ASSERT(load_task_ptr->parent_task != load_task_ptr);
				for (const ThreadLoadTask::ResourceChangedConnection& rcc :
					load_task_ptr->resource_changed_connections) {
					load_task_ptr->parent_task->resource_changed_connections.push_back(rcc);
				}
				load_task_ptr->connections_propagated = true;
			}
		}
		else {
			p_thread_load_lock.temp_unlock();

			// A leaf task being awaited => Propagate the resource changed connections.
			if (Thread::is_main_thread()) {
				// On the main thread it's safe to migrate the connections to the standard signal
				// mechanism.
				for (const ThreadLoadTask::ResourceChangedConnection& rcc :
					load_task_ptr->resource_changed_connections) {
				}
			}
			else {
				// On non-main threads, we have to queue and call it done when processed.
				if (!load_task_ptr->resource_changed_connections.is_empty()) {
					for (const ThreadLoadTask::ResourceChangedConnection& rcc :
						load_task_ptr->resource_changed_connections) {
					}
				}
			}

			p_thread_load_lock.temp_relock();
		}
	}

	return resource;
}

bool ResourceLoader::_ensure_load_progress()
{
	// Some servers may need a new engine iteration to allow the load to progress.
	// Since the only known one is the rendering server (in single thread mode), let's keep it
	// simple and just sync it. This may be refactored in the future to support other servers and
	// have less coupling.
	if (OS::is_separate_thread_rendering_enabled()) {
		return false; // Not needed.
	}
	Renderer::sync();
	return true;
}

Ref<Resource> ResourceLoader::ensure_resource_ref_override_for_outer_load(
	const String& p_path, const String& p_res_type)
{
	ERR_FAIL_COND_V(
		load_nesting == 0, Ref<Resource>()); // It makes no sense to use this from nesting level 0.
	const String& local_path = _validate_local_path(p_path);
	HashMap<String, Ref<Resource>>& overrides = res_ref_overrides[load_nesting - 1];
	HashMap<String, Ref<Resource>>::Iterator E = overrides.find(local_path);
	if (E) {
		return E->value;
	}
	else {
		return nullptr;
	}
}

Ref<Resource> ResourceLoader::get_resource_ref_override(const String& p_path)
{
	DEV_ASSERT(p_path == _validate_local_path(p_path));
	HashMap<int, HashMap<String, Ref<Resource>>>::Iterator E = res_ref_overrides.find(load_nesting);
	if (!E) {
		return nullptr;
	}
	HashMap<String, Ref<Resource>>::Iterator F = E->value.find(p_path);
	if (!F) {
		return nullptr;
	}

	return F->value;
}

bool ResourceLoader::exists(const String& p_path, const String& p_type_hint)
{
	String local_path = _validate_local_path(p_path);

	if (ResourceCache::has(local_path)) {
		return true; // If cached, it probably exists
	}

	String path = _path_remap(local_path);

	// Try all loaders and pick the first match for the type hint
	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(path, p_type_hint)) {
			continue;
		}

		if (loader[i]->exists(path)) {
			return true;
		}
	}

	return false;
}

void ResourceLoader::add_resource_format_loader(
	Ref<ResourceFormatLoader> p_format_loader, bool p_at_front)
{
	ERR_FAIL_COND(p_format_loader.is_null());
	ERR_FAIL_COND(loader_count >= MAX_LOADERS);

	if (p_at_front) {
		for (int i = loader_count; i > 0; i--) {
			loader[i] = loader[i - 1];
		}
		loader[0] = p_format_loader;
		loader_count++;
	}
	else {
		loader[loader_count++] = p_format_loader;
	}
}

void ResourceLoader::remove_resource_format_loader(Ref<ResourceFormatLoader> p_format_loader)
{
	ERR_FAIL_COND(p_format_loader.is_null());

	// Find loader
	int i = 0;
	for (; i < loader_count; ++i) {
		if (loader[i] == p_format_loader) {
			break;
		}
	}

	ERR_FAIL_COND(i >= loader_count); // Not found

	// Shift next loaders up
	for (; i < loader_count - 1; ++i) {
		loader[i] = loader[i + 1];
	}
	loader[loader_count - 1].unref();
	--loader_count;
}

String ResourceLoader::get_import_group_file(const String& p_path)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->get_import_group_file(p_path);
	}

	return String(); // not found
}

bool ResourceLoader::is_import_valid(const String& p_path)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->is_import_valid(p_path);
	}

	return false; // not found
}

bool ResourceLoader::is_imported(const String& p_path)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->is_imported(p_path);
	}

	return false; // not found
}

void ResourceLoader::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		loader[i]->get_dependencies(local_path, p_dependencies, p_add_types);
	}
}

Error ResourceLoader::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->rename_dependencies(local_path, p_map);
	}

	return OK; // ??
}

void ResourceLoader::get_classes_used(const String& p_path, HashSet<String>* r_classes)
{
	String local_path = _validate_local_path(p_path);

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->get_classes_used(p_path, r_classes);
	}
}

String ResourceLoader::get_resource_type(const String& p_path)
{
	String local_path = _validate_local_path(p_path);

	for (int i = 0; i < loader_count; i++) {
		String result = loader[i]->get_resource_type(local_path);
		if (!result.is_empty()) {
			return result;
		}
	}

	return "";
}

String ResourceLoader::get_resource_script_class(const String& p_path)
{
	String local_path = _validate_local_path(p_path);

	for (int i = 0; i < loader_count; i++) {
		String result = loader[i]->get_resource_script_class(local_path);
		if (!result.is_empty()) {
			return result;
		}
	}

	return "";
}

ResourceUID::ID ResourceLoader::get_resource_uid(const String& p_path)
{
	const String local_path = _validate_local_path(p_path);
	if (!Engine::is_editor_hint()) {
		return ResourceUID::get_path_id(local_path);
	}

	for (int i = 0; i < loader_count; i++) {
		ResourceUID::ID id = loader[i]->get_resource_uid(local_path);
		if (id != ResourceUID::INVALID_ID) {
			return id;
		}
	}

	return ResourceUID::INVALID_ID;
}

bool ResourceLoader::should_create_uid_file(const String& p_path)
{
	const String local_path = _validate_local_path(p_path);
	if (FileAccess::exists(local_path + ".uid")) {
		return false;
	}

	for (int i = 0; i < loader_count; i++) {
		if (loader[i]->recognize_path(local_path)) {
			return !loader[i]->has_custom_uid_support();
		}
	}
	return false;
}

String ResourceLoader::_path_remap(const String& p_path, bool* r_translation_remapped)
{
	String new_path = p_path;

	if (translation_remaps.has(p_path)) {
		// translation_remaps has the following format:
		//   { "res://path.png": PackedStringArray( "res://path-ru.png:ru", "res://path-de.png:de" )
		//   }

		// To find the path of the remapped resource, we extract the locale name after
		// the last ':' to match the project locale.

		// An extra remap may still be necessary afterwards due to the text -> binary converter on
		// export.

		String locale = TranslationServer::get_locale();
		ERR_FAIL_COND_V_MSG(locale.length() < 2, p_path,
			vformat(
				"Could not remap path '%s' for translation as configured locale '%s' is invalid.",
				p_path, locale));

		Vector<String>& res_remaps = *translation_remaps.getptr(new_path);

		int best_score = 0;
		for (int i = 0; i < res_remaps.size(); i++) {
			int split = res_remaps[i].rfind_char(':');
			if (split == -1) {
				continue;
			}
			String l = res_remaps[i].substr(split + 1).strip_edges();
			int score = TranslationServer::compare_locales(locale, l);
			if (score > 0 && score >= best_score) {
				new_path = res_remaps[i].left(split);
				best_score = score;
				if (score == 10) {
					break; // Exact match, skip the rest.
				}
			}
		}

		if (r_translation_remapped) {
			*r_translation_remapped = true;
		}

		// Fallback to p_path if new_path does not exist.
		if (!FileAccess::exists(new_path + ".import") && !FileAccess::exists(new_path + ".remap") &&
			!FileAccess::exists(new_path)) {
			WARN_PRINT(vformat(
				"Translation remap '%s' does not exist. Falling back to '%s'.", new_path, p_path));
			new_path = p_path;
		}
	}

	// Usually, there's no remap file and FileAccess::exists() is faster than FileAccess::open().
	new_path = ResourceUID::ensure_path(new_path);

	return new_path;
}

String ResourceLoader::import_remap(const String& p_path)
{
	if (ResourceFormatImporter::recognize_path(p_path)) {
		return ResourceFormatImporter::get_internal_resource_path(p_path);
	}

	return p_path;
}

String ResourceLoader::path_remap(const String& p_path) { return _path_remap(p_path); }

void ResourceLoader::reload_translation_remaps()
{
	List<Resource*> to_reload;

	{
		MutexLock lock(ResourceCache::lock);
		SelfList<Resource>* E = remapped_list.first();

		while (E) {
			to_reload.push_back(E->self());
			E = E->next();
		}
	}

	// now just make sure to not delete any of these resources while changing locale..
	while (to_reload.front()) {
		to_reload.front()->get()->reload_from_file();
		to_reload.pop_front();
	}
}

void ResourceLoader::clear_translation_remaps()
{
	translation_remaps.clear();
	while (remapped_list.first() != nullptr) {
		remapped_list.remove(remapped_list.first());
	}
}

void ResourceLoader::clear_thread_load_tasks()
{
	// Bring the thing down as quickly as possible without causing deadlocks or leaks.

	MutexLock thread_load_lock(thread_load_mutex);
	cleaning_tasks = true;

	while (true) {
		bool none_running = true;
		if (thread_load_tasks.size()) {
			for (KeyValue<String, ResourceLoader::ThreadLoadTask>& E : thread_load_tasks) {
				if (E.value.status == THREAD_LOAD_IN_PROGRESS) {
					if (E.value.cond_var && E.value.need_wait) {
						E.value.cond_var->notify_all();
					}
					E.value.need_wait = false;
					none_running = false;
				}
			}
		}
		if (none_running) {
			break;
		}

		thread_load_lock.temp_unlock();

		OS::delay_usec(1000);

		thread_load_lock.temp_relock();
	}

	while (user_load_tokens.begin()) {
		LoadToken* user_token = user_load_tokens.begin()->value;
		user_load_tokens.remove(user_load_tokens.begin());
		DEV_ASSERT(user_token->user_rc > 0 && !user_token->user_path.is_empty());
		user_token->user_path.clear();
		user_token->user_rc = 0;
		user_token->unreference();
	}

	thread_load_tasks.clear();
	thread_waiting_on.clear();
	// yielders is already guaranteed to be empty now

	cleaning_tasks = false;
}

void ResourceLoader::set_load_callback(ResourceLoadedCallback p_callback)
{
	_loaded_callback = p_callback;
}

ResourceLoadedCallback ResourceLoader::_loaded_callback = nullptr;

void ResourceLoader::set_create_missing_resources_if_class_unavailable(bool p_enable)
{
	create_missing_resources_if_class_unavailable = p_enable;
}

void ResourceLoader::remove_custom_loaders()
{
	Vector<Ref<ResourceFormatLoader>> custom_loaders;
	for (int i = 0; i < custom_loaders.size(); ++i) {
		remove_resource_format_loader(custom_loaders[i]);
	}
}

bool ResourceLoader::is_cleaning_tasks()
{
	MutexLock lock(thread_load_mutex);
	return cleaning_tasks;
}

Vector<String> ResourceLoader::list_directory(const String& p_directory)
{
	RBSet<String> files_found;
	Ref<DirAccess> dir = DirAccess::open(p_directory);
	if (dir.is_null()) {
		return Vector<String>();
	}

	Error err = dir->list_dir_begin();
	if (err != OK) {
		return Vector<String>();
	}

	String d = dir->get_next();
	while (!d.is_empty()) {
		bool recognized = false;
		if (dir->current_is_dir()) {
			if (d != "." && d != "..") {
				d += "/";
				recognized = true;
			}
		}
		else {
			if (d.ends_with(".import") || d.ends_with(".remap") || d.ends_with(".uid")) {
				d = d.substr(0, d.rfind_char('.'));
			}

			if (d.ends_with(".gdc")) {
				d = d.substr(0, d.rfind_char('.'));
				d += ".gd";
			}

			const String full_path = p_directory.path_join(d);
			// Try all loaders and pick the first match for the type hint.
			for (int i = 0; i < loader_count; i++) {
				if (loader[i]->recognize_path(full_path)) {
					recognized = true;
					break;
				}
			}
		}

		if (recognized) {
			files_found.insert(d);
		}
		d = dir->get_next();
	}

	Vector<String> ret;
	for (const String& f : files_found) {
		ret.push_back(f);
	}

	return ret;
}

void ResourceLoader::initialize() {}

void ResourceLoader::finalize() {}

ResourceLoadErrorNotify ResourceLoader::err_notify = nullptr;
DependencyErrorNotify ResourceLoader::dep_err_notify = nullptr;

bool ResourceLoader::create_missing_resources_if_class_unavailable = false;
bool ResourceLoader::abort_on_missing_resource = true;
bool ResourceLoader::timestamp_on_load = false;

thread_local bool ResourceLoader::import_thread = false;
thread_local int ResourceLoader::load_nesting = 0;
thread_local HashMap<int, HashMap<String, Ref<Resource>>> ResourceLoader::res_ref_overrides;
thread_local ResourceLoader::ThreadLoadTask* ResourceLoader::curr_load_task = nullptr;

SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG>& _get_res_loader_mutex()
{
	return ResourceLoader::thread_load_mutex;
}

template <>
thread_local SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG>::TLSData
	SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG>::tls_data(_get_res_loader_mutex());
SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG> ResourceLoader::thread_load_mutex;
HashMap<String, ResourceLoader::ThreadLoadTask> ResourceLoader::thread_load_tasks;
HashMap<int, String> ResourceLoader::thread_waiting_on;
LocalVector<int> ResourceLoader::yielders;

bool ResourceLoader::cleaning_tasks = false;

HashMap<String, ResourceLoader::LoadToken*> ResourceLoader::user_load_tokens;

SelfList<Resource>::List ResourceLoader::remapped_list;
HashMap<String, Vector<String>> ResourceLoader::translation_remaps;

ResourceLoaderImport ResourceLoader::import = nullptr;

bool ResourceFormatLoader::handles_type(const String& p_type) const { return false; }

String ResourceFormatLoader::get_resource_type(const String& p_path) const { return String(); }

String ResourceFormatLoader::get_resource_script_class(const String& p_path) const
{
	return String();
}

bool ResourceFormatLoader::has_custom_uid_support() const { return false; }

void ResourceLoader::_run_load_task(void* p_userdata) {}

void ResourceLoader::load_translation_remaps() {}


Ref<Resource> ResourceFormatLoader::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress,
	CacheMode p_cache_mode)
{
	Ref<Resource> r = memnew(Resource);
	return r;
}
#ifdef print_lt
#undef print_lt
#endif

// ==========================================================================
// resource_saver.cpp
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
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"

Ref<ResourceFormatSaver> ResourceSaver::saver[MAX_SAVERS];

int ResourceSaver::saver_count = 0;
bool ResourceSaver::timestamp_on_save = false;
ResourceSavedCallback ResourceSaver::save_callback = nullptr;
ResourceSaverGetResourceIDForPath ResourceSaver::save_get_id_for_path = nullptr;

Error ResourceFormatSaver::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Error err = ERR_METHOD_NOT_FOUND;
	return err;
}

Error ResourceFormatSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	Error err = ERR_FILE_UNRECOGNIZED;
	return err;
}

bool ResourceFormatSaver::recognize(const Ref<Resource>& p_resource) const
{
	bool success = false;
	return success;
}

void ResourceFormatSaver::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	PackedStringArray exts;
	const String* r = exts.ptr();
	for (int i = 0; i < exts.size(); ++i) {
		p_extensions->push_back(r[i]);
	}
}

bool ResourceFormatSaver::recognize_path(
	const Ref<Resource>& p_resource, const String& p_path) const
{
	String extension = p_path.get_extension();
	List<String> extensions;
	get_recognized_extensions(p_resource, &extensions);
	for (const String& E : extensions) {
		if (E.nocasecmp_to(extension) == 0) {
			return true;
		}
	}
	return false;
}

Error ResourceSaver::save(Resource* rp_resource, const String& p_path, uint32_t p_flags)
{
	String path = p_path;
	if (path.is_empty()) {
		path = rp_resource->get_path();
	}
	ERR_FAIL_COND_V_MSG(path.is_empty(), ERR_INVALID_PARAMETER,
		"Can't save resource to empty path. Provide non-empty path or a Resource with non-empty "
		"resource_path.");
	Error err = ERR_FILE_UNRECOGNIZED;
	for (int i = 0; i < saver_count; i++) {
		if (!saver[i]->recognize(rp_resource)) {
			continue;
		}

		if (!saver[i]->recognize_path(rp_resource, path)) {
			continue;
		}

		String old_path = rp_resource->get_path();

		String local_path = ProjectSettings::localize_path(path);

		if (p_flags & FLAG_CHANGE_PATH) {
			rp_resource->set_path(local_path);
		}

		err = saver[i]->save(rp_resource, path, p_flags);

		if (err == OK) {
			if (p_flags & FLAG_CHANGE_PATH) {
				rp_resource->set_path(old_path);
			}

			if (save_callback && path.begins_with("res://")) {
				save_callback(rp_resource, path);
			}

			return OK;
		}
	}

	return err;
}

Error ResourceSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String path = p_path;

	ERR_FAIL_COND_V_MSG(path.is_empty(), ERR_INVALID_PARAMETER,
		"Can't update UID to empty path. Provide non-empty path.");

	Error err = ERR_FILE_UNRECOGNIZED;

	for (int i = 0; i < saver_count; i++) {
		err = saver[i]->set_uid(path, p_uid);
		if (err == OK) {
			break;
		}
	}

	return err;
}

void ResourceSaver::set_save_callback(ResourceSavedCallback p_callback)
{
	save_callback = p_callback;
}

void ResourceSaver::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions)
{
	ERR_FAIL_COND_MSG(p_resource.is_null(), "It's not a reference to a valid Resource object.");
	for (int i = 0; i < saver_count; i++) {
		saver[i]->get_recognized_extensions(p_resource, p_extensions);
	}
}

void ResourceSaver::add_resource_format_saver(
	Ref<ResourceFormatSaver> p_format_saver, bool p_at_front)
{
	ERR_FAIL_COND_MSG(
		p_format_saver.is_null(), "It's not a reference to a valid ResourceFormatSaver object.");
	ERR_FAIL_COND(saver_count >= MAX_SAVERS);

	if (p_at_front) {
		for (int i = saver_count; i > 0; i--) {
			saver[i] = saver[i - 1];
		}
		saver[0] = p_format_saver;
		saver_count++;
	}
	else {
		saver[saver_count++] = p_format_saver;
	}
}

void ResourceSaver::remove_resource_format_saver(Ref<ResourceFormatSaver> p_format_saver)
{
	ERR_FAIL_COND_MSG(
		p_format_saver.is_null(), "It's not a reference to a valid ResourceFormatSaver object.");

	// Find saver
	int i = 0;
	for (; i < saver_count; ++i) {
		if (saver[i] == p_format_saver) {
			break;
		}
	}

	ERR_FAIL_COND(i >= saver_count); // Not found

	// Shift next savers up
	for (; i < saver_count - 1; ++i) {
		saver[i] = saver[i + 1];
	}
	saver[saver_count - 1].unref();
	--saver_count;
}

bool ResourceSaver::add_custom_resource_format_saver(const String& p_script_path)
{
	if (_find_custom_resource_format_saver(p_script_path).is_valid()) {
		return false;
	}

	Ref<Resource> res = ResourceLoader::load(p_script_path);
	ERR_FAIL_COND_V(res.is_null(), false);

	return true;
}

void ResourceSaver::remove_custom_savers()
{
	Vector<Ref<ResourceFormatSaver>> custom_savers;
	for (int i = 0; i < custom_savers.size(); ++i) {
		remove_resource_format_saver(custom_savers[i]);
	}
}

ResourceUID::ID ResourceSaver::get_resource_id_for_path(const String& p_path, bool p_generate)
{
	if (save_get_id_for_path) {
		return save_get_id_for_path(p_path, p_generate);
	}
	return ResourceUID::INVALID_ID;
}

void ResourceSaver::set_get_resource_id_for_path(ResourceSaverGetResourceIDForPath p_callback)
{
	save_get_id_for_path = p_callback;
}

Ref<ResourceFormatSaver> ResourceSaver::_find_custom_resource_format_saver(const String& p_path)
{
	Ref<ResourceFormatSaver> rfs = memnew(ResourceFormatSaver);
	return rfs;
}

// ==========================================================================
// resource_uid.cpp
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
#include "core/crypto/crypto.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/types.h"
#include "core/io/resource_uid.h"

// These constants are off by 1, causing the 'z' and '9' characters never to be used.
// This cannot be fixed without breaking compatibility; see GH-83843.
static constexpr uint32_t uid_char_count = ('z' - 'a');
static constexpr uint32_t uid_base = uid_char_count + ('9' - '0');

String ResourceUID::get_cache_file()
{
	return ProjectSettings::get_project_data_path().path_join("uid_cache.bin");
}

static constexpr uint8_t uuid_characters[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
	'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', '0', '1', '2', '3', '4',
	'5', '6', '7', '8'};
static constexpr uint32_t uuid_characters_element_count = std_size(uuid_characters);
static constexpr uint8_t max_uuid_number_length =
	13; // Max 0x7FFFFFFFFFFFFFFF (uid://d4n4ub6itg400) size is 13 characters.

String ResourceUID::id_to_text(ID p_id)
{
	if (p_id < 0) {
		return "uid://<invalid>";
	}

	char32_t tmp[max_uuid_number_length];
	uint32_t tmp_size = 0;
	do {
		uint32_t c = p_id % uuid_characters_element_count;
		tmp[tmp_size] = uuid_characters[c];
		p_id /= uuid_characters_element_count;
		++tmp_size;
	} while (p_id);

	// tmp_size + uid:// (6) + 1 for null.
	String txt;
	txt.resize_uninitialized(tmp_size + 7);

	char32_t* p = txt.ptrw();
	p[0] = 'u';
	p[1] = 'i';
	p[2] = 'd';
	p[3] = ':';
	p[4] = '/';
	p[5] = '/';
	uint32_t size = 6;

	// The above loop give the number backward, recopy it in the string in the correct order.
	for (uint32_t i = 0; i < tmp_size; ++i) {
		p[size++] = tmp[tmp_size - i - 1];
	}

	p[size] = 0;

	return txt;
}

ResourceUID::ID ResourceUID::text_to_id(const String& p_text)
{
	if (!p_text.begins_with("uid://") || p_text == "uid://<invalid>") {
		return INVALID_ID;
	}

	uint32_t l = p_text.length();
	uint64_t uid = 0;
	for (uint32_t i = 6; i < l; i++) {
		uid *= uid_base;
		uint32_t c = p_text[i];
		if (is_ascii_lower_case(c)) {
			uid += c - 'a';
		}
		else if (is_digit(c)) {
			uid += c - '0' + uid_char_count;
		}
		else {
			return INVALID_ID;
		}
	}
	return ID(uid & 0x7FFFFFFFFFFFFFFF);
}

ResourceUID::ID ResourceUID::create_id()
{
	while (true) {
		ID id = INVALID_ID;
		MutexLock lock(data->mutex);
		Error err = CryptoCore::generate_random((uint8_t*)&id, sizeof(id));
		ERR_FAIL_COND_V(err != OK, INVALID_ID);
		id &= 0x7FFFFFFFFFFFFFFF;
		bool exists = data->unique_ids.has(id);
		if (!exists) {
			return id;
		}
	}
}

ResourceUID::ID ResourceUID::create_id_for_path(const String& p_path)
{
	ID id = INVALID_ID;
	RandomPCG rng;
	rng.seed(
		p_path.hash64() * p_path.to_lower().hash64() * FileAccess::get_md5(p_path).hash64());

	while (true) {
		int64_t num1 = rng.rand();
		int64_t num2 = ((int64_t)rng.rand()) << 32;
		id = (num1 | num2) & 0x7FFFFFFFFFFFFFFF;

		MutexLock lock(data->mutex);
		if (!data->unique_ids.has(id)) {
			break;
		}
	}
	return id;
}

bool ResourceUID::has_id(ID p_id)
{
	MutexLock l(data->mutex);
	return data->unique_ids.has(p_id);
}

void ResourceUID::add_id(ID p_id, const String& p_path)
{
	MutexLock l(data->mutex);
	ERR_FAIL_COND(data->unique_ids.has(p_id));
	Cache c;
	c.cs = p_path.utf8();
	data->unique_ids[p_id] = c;
	if (data->use_reverse_cache) {
		data->reverse_cache[c.cs] = p_id;
	}
	data->changed = true;
	// The cache was never loaded (probably does not exist), so assume that first ID initializes it.
	data->cache_initialized = true;
}

void ResourceUID::set_id(ID p_id, const String& p_path)
{
	MutexLock l(data->mutex);
	ERR_FAIL_COND(!data->unique_ids.has(p_id));
	CharString cs = p_path.utf8();
	const char* update_ptr = cs.ptr();
	const char* cached_ptr = data->unique_ids[p_id].cs.ptr();
	if (update_ptr == nullptr && cached_ptr == nullptr) {
		return; // Both are empty strings.
	}
	if ((update_ptr == nullptr) != (cached_ptr == nullptr) || strcmp(update_ptr, cached_ptr) != 0) {
		data->unique_ids[p_id].cs = cs;
		data->unique_ids[p_id].saved_to_cache = false; // changed
		if (data->use_reverse_cache) {
			data->reverse_cache[cs] = p_id;
		}
		data->changed = true;
	}
}

String ResourceUID::get_id_path(ID p_id)
{
	ERR_FAIL_COND_V_MSG(p_id == INVALID_ID, String(), "Invalid UID.");
	MutexLock l(data->mutex);
	const ResourceUID::Cache* cache = data->unique_ids.getptr(p_id);

#if TOOLS_ENABLED
	// On startup, the scan_for_uid_on_startup callback should be set and will
	// execute EditorFileSystem::scan_for_uid, which scans all project files
	// to reload the UID cache before the first scan.
	// Note: EditorFileSystem::scan_for_uid sets scan_for_uid_on_startup to nullptr
	//       once the first scan_for_uid is complete.
	if (!cache && scan_for_uid_on_startup) {
		scan_for_uid_on_startup();
		cache = data->unique_ids.getptr(p_id);
	}
#endif

	if (unlikely(!cache)) {
		if (data->cache_initialized) {
			ERR_PRINT(vformat("Unrecognized UID: \"%s\".", id_to_text(p_id)));
		}
		return String();
	}
	const CharString& cs = cache->cs;
	return String::utf8(cs.ptr());
}

ResourceUID::ID ResourceUID::get_path_id(const String& p_path)
{
	const ID* id = data->reverse_cache.getptr(p_path.utf8());
	if (id) {
		return *id;
	}
	return INVALID_ID;
}

void ResourceUID::remove_id(ID p_id)
{
	MutexLock l(data->mutex);
	ERR_FAIL_COND(!data->unique_ids.has(p_id));
	if (data->use_reverse_cache) {
		data->reverse_cache.erase(data->unique_ids[p_id].cs);
	}
	data->unique_ids.erase(p_id);
}

String ResourceUID::uid_to_path(const String& p_uid)
{
	return get_id_path(text_to_id(p_uid));
}

String ResourceUID::path_to_uid(const String& p_path)
{
	const ID id = ResourceLoader::get_resource_uid(p_path);
	if (id == INVALID_ID) {
		return p_path;
	}
	else {
		return id_to_text(id);
	}
}

String ResourceUID::ensure_path(const String& p_uid_or_path)
{
	if (p_uid_or_path.begins_with("uid://")) {
		return uid_to_path(p_uid_or_path);
	}
	return p_uid_or_path;
}

Vector<uint8_t> ResourceUID::encode_binary_cache(const Vector<Pair<ID, String>>& p_entries)
{
	Ref<StreamPeerBuffer> buffer;
	buffer.instantiate();
	buffer->put_u32(p_entries.size());

	for (const Pair<ID, String>& entry : p_entries) {
		buffer->put_u64(uint64_t(entry.first));
		CharString cs = entry.second.utf8();
		buffer->put_u32(cs.length());
		buffer->put_data((const uint8_t*)cs.ptr(), cs.length());
	}

	return buffer->get_data_array();
}

Error ResourceUID::save_to_cache()
{
	String cache_file = get_cache_file();
	if (!FileAccess::exists(cache_file)) {
		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		d->make_dir_recursive(String(cache_file).get_base_dir()); // ensure base dir exists
	}

	Ref<FileAccess> f = FileAccess::open(cache_file, FileAccess::WRITE);
	if (f.is_null()) {
		return ERR_CANT_OPEN;
	}

	MutexLock l(data->mutex);

	Vector<Pair<ID, String>> entries;
	entries.reserve(data->unique_ids.size());
	data->cache_entries = 0;

	for (KeyValue<ID, Cache>& E : data->unique_ids) {
		entries.push_back(
			Pair<ID, String>(E.key, String::utf8(E.value.cs.ptr(), E.value.cs.length())));
		E.value.saved_to_cache = true;
		data->cache_entries++;
	}

	Vector<uint8_t> encoded_data = encode_binary_cache(entries);
	f->store_buffer(encoded_data.ptr(), encoded_data.size());

	data->changed = false;
	return OK;
}

Error ResourceUID::load_from_cache(bool p_reset)
{
	Ref<FileAccess> f = FileAccess::open(get_cache_file(), FileAccess::READ);
	if (f.is_null()) {
		return ERR_CANT_OPEN;
	}

	MutexLock l(data->mutex);
	if (p_reset) {
		if (data->use_reverse_cache) {
			data->reverse_cache.clear();
		}
		data->unique_ids.clear();
	}

	uint32_t entry_count = f->get_32();
	for (uint32_t i = 0; i < entry_count; i++) {
		int64_t id = f->get_64();
		int32_t len = f->get_32();
		Cache c;
		c.cs.resize_uninitialized(len + 1);
		ERR_FAIL_COND_V(c.cs.size() != len + 1, ERR_FILE_CORRUPT); // Out of memory.
		c.cs[len] = 0;
		int32_t rl = f->get_buffer((uint8_t*)c.cs.ptrw(), len);
		ERR_FAIL_COND_V(rl != len, ERR_FILE_CORRUPT);

		c.saved_to_cache = true;
		data->unique_ids[id] = c;
		if (data->use_reverse_cache) {
			data->reverse_cache[c.cs] = id;
		}
	}

	data->cache_entries = entry_count;
	data->changed = false;
	data->cache_initialized = true;
	return OK;
}

Error ResourceUID::update_cache()
{
	if (!data->changed) {
		return OK;
	}

	if (data->cache_entries == 0) {
		return save_to_cache();
	}
	MutexLock l(data->mutex);

	Ref<FileAccess> f;
	for (KeyValue<ID, Cache>& E : data->unique_ids) {
		if (!E.value.saved_to_cache) {
			if (f.is_null()) {
				f = FileAccess::open(get_cache_file(), FileAccess::READ_WRITE); // Append.
				if (f.is_null()) {
					return ERR_CANT_OPEN;
				}
				f->seek_end();
			}
			f->store_64(uint64_t(E.key));
			uint32_t s = E.value.cs.length();
			f->store_32(s);
			f->store_buffer((const uint8_t*)E.value.cs.ptr(), s);
			E.value.saved_to_cache = true;
			data->cache_entries++;
		}
	}

	if (f.is_valid()) {
		f->seek(0);
		f->store_32(data->cache_entries); // update amount of entries
	}

	data->changed = false;

	return OK;
}

String ResourceUID::get_path_from_cache(Ref<FileAccess>& p_cache_file, const String& p_uid_string)
{
	const int64_t uid_from_string = text_to_id(p_uid_string);
	if (uid_from_string != INVALID_ID) {
		const uint32_t entry_count = p_cache_file->get_32();
		for (uint32_t i = 0; i < entry_count; i++) {
			int64_t id = p_cache_file->get_64();
			int32_t len = p_cache_file->get_32();

			if (id == uid_from_string) {
				CharString cs;
				cs.resize_uninitialized(len + 1);
				ERR_FAIL_COND_V(cs.size() != len + 1, String());
				cs[len] = 0;
				int32_t rl = p_cache_file->get_buffer((uint8_t*)cs.ptrw(), len);
				ERR_FAIL_COND_V(rl != len, String());
				return String::utf8(cs.get_data());
			}
			else {
				p_cache_file->seek(p_cache_file->get_position() + len);
				ERR_FAIL_COND_V(p_cache_file->eof_reached(), String());
			}
		}
	}
	return String();
}

void ResourceUID::clear()
{
	data->cache_entries = 0;
	if (data->use_reverse_cache) {
		data->reverse_cache.clear();
	}
	data->unique_ids.clear();
	data->changed = false;
}

void ResourceUID::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void ResourceUID::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

// ==========================================================================
// missing_resource.cpp
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

#include "core/io/resource_loader.h"

void MissingResource::set_original_class(const String& p_class) { original_class = p_class; }

String MissingResource::get_original_class() const { return original_class; }

void MissingResource::set_recording_properties(bool p_enable) { recording_properties = p_enable; }

bool MissingResource::is_recording_properties() const { return recording_properties; }

String MissingResource::get_save_class() const { return original_class; }

// ==========================================================================
// image_loader.cpp
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

#include "core/io/resource_loader.h"

bool ImageFormatLoader::recognize(const String& p_extension) const
{
	List<String> extensions;
	get_recognized_extensions(&extensions);
	for (const String& E : extensions) {
		if (E.nocasecmp_to(p_extension) == 0) {
			return true;
		}
	}

	return false;
}

Error ImageFormatLoaderExtension::load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
	uint32_t p_flags, float p_scale)
{
	Error err = ERR_UNAVAILABLE;
	return err;
}

void ImageFormatLoaderExtension::add_format_loader() { ImageLoader::add_image_format_loader(this); }

void ImageFormatLoaderExtension::remove_format_loader()
{
	ImageLoader::remove_image_format_loader(this);
}

Error ImageLoader::load_image(const String& p_file, Ref<Image> p_image, Ref<FileAccess> p_custom,
	uint32_t p_flags, float p_scale)
{
	ERR_FAIL_COND_V_MSG(
		p_image.is_null(), ERR_INVALID_PARAMETER, "Can't load an image: invalid Image object.");
	const String file = ResourceUID::ensure_path(p_file);

	Ref<FileAccess> f = p_custom;
	if (f.is_null()) {
		Error err;
		f = FileAccess::open(file, FileAccess::READ, &err);
		ERR_FAIL_COND_V_MSG(f.is_null(), err, vformat("Error opening file '%s'.", file));
	}

	String extension = file.get_extension();

	for (int i = 0; i < loader.size(); i++) {
		if (!loader[i]->recognize(extension)) {
			continue;
		}
		Error err = loader.write[i]->load_image(p_image, f, p_flags, p_scale);
		if (err != OK) {
			ERR_PRINT(vformat("Error loading image: '%s'.", file));
		}

		if (err != ERR_FILE_UNRECOGNIZED) {
			return err;
		}
	}

	return
ERR_FILE_UNRECOGNIZED;
}

void ImageLoader::get_recognized_extensions(List<String>* p_extensions)
{
	for (int i = 0; i < loader.size(); i++) {
		loader[i]->get_recognized_extensions(p_extensions);
	}
}

Ref<ImageFormatLoader> ImageLoader::recognize(const String& p_extension)
{
	for (int i = 0; i < loader.size(); i++) {
		if (loader[i]->recognize(p_extension)) {
			return loader[i];
		}
	}

	return nullptr;
}

void ImageLoader::add_image_format_loader(Ref<ImageFormatLoader> p_loader)
{
	loader.push_back(p_loader);
}

void ImageLoader::remove_image_format_loader(Ref<ImageFormatLoader> p_loader)
{
	loader.erase(p_loader);
}

void ImageLoader::cleanup()
{
	while (loader.size()) {
		remove_image_format_loader(loader[0]);
	}
}

void ImageFormatLoaderExtension::get_recognized_extensions(List<String>* p_extensions) const {}

// ==========================================================================
// image_resource_format.cpp
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

#include "core/io/resource_loader.h"

Ref<Resource> ResourceFormatLoaderImage::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	uint8_t header[4] = { 0, 0, 0, 0 };
	f->get_buffer(header, 4);

	bool unrecognized = header[0] != 'G' || header[1] != 'D' || header[2] != 'I' || header[3] != 'M';
	if (unrecognized) {
		if (r_error) {
			*r_error = ERR_FILE_UNRECOGNIZED;
		}
		ERR_FAIL_V(Ref<Resource>());
	}

	String extension = f->get_pascal_string();

	int idx = -1;

	for (int i = 0; i < ImageLoader::loader.size(); i++) {
		if (ImageLoader::loader[i]->recognize(extension)) {
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		if (r_error) {
			*r_error = ERR_FILE_UNRECOGNIZED;
		}
		ERR_FAIL_V(Ref<Resource>());
	}

	Ref<Image> image;
	image.instantiate();

	Error err = ImageLoader::loader.write[idx]->load_image(image, f);

	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}
		return Ref<Resource>();
	}

	if (r_error) {
		*r_error = OK;
	}

	return image;
}

void ResourceFormatLoaderImage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("image");
}

bool ResourceFormatLoaderImage::handles_type(const String &p_type) const {
	return p_type == "Image";
}

String ResourceFormatLoaderImage::get_resource_type(const String &p_path) const {
	return p_path.get_extension().to_lower() == "image" ? "Image" : String();
}

// ============================================================================
// Merged from core/io/resource.cpp
// ============================================================================

#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/types.h"
#include "core/os/os.h"
#include "core/io/resource.h"
#include "scene/main/node.h" //only so casting works

void Resource::register_custom_data_to_otdb()
{
}

void Resource::_add_resource_base_extension_to_classdb(
	const String& p_extension, const String& p_class)
{
}

void Resource::emit_changed()
{
	if (emit_changed_state != EMIT_CHANGED_UNBLOCKED) {
		emit_changed_state = EMIT_CHANGED_BLOCKED_PENDING_EMIT;
		return;
	}
	if (ResourceLoader::is_within_load() && !Thread::is_main_thread()) {
		ResourceLoader::resource_changed_emit(this);
		return;
	}
}

void Resource::_block_emit_changed()
{
	if (emit_changed_state == EMIT_CHANGED_UNBLOCKED) {
		emit_changed_state = EMIT_CHANGED_BLOCKED;
	}
}

void Resource::_unblock_emit_changed()
{
	bool emit = (emit_changed_state == EMIT_CHANGED_BLOCKED_PENDING_EMIT);
	emit_changed_state = EMIT_CHANGED_UNBLOCKED;
	if (emit) {
		emit_changed();
	}
}

void Resource::_resource_path_changed() {}

void Resource::set_path(const String& p_path, bool p_take_over)
{
	if (path_cache == p_path) {
		return;
	}

	if (p_path.is_empty()) {
		p_take_over = false; // Can't take over an empty path
	}

	{
		MutexLock lock(ResourceCache::lock);

		if (!path_cache.is_empty()) {
			ResourceCache::resources.erase(path_cache);
		}

		path_cache = "";

		Ref<Resource> existing = ResourceCache::get_ref(p_path);

		if (existing.is_valid()) {
			if (p_take_over) {
				existing->path_cache = String();
				ResourceCache::resources.erase(p_path);
			}
			else {
				ERR_FAIL_MSG(vformat("Another resource is loaded from path '%s' (possible cyclic "
									 "resource inclusion).",
					p_path));
			}
		}

		path_cache = p_path;

		if (!path_cache.is_empty()) {
			ResourceCache::resources[path_cache] = this;
		}
	}

	_resource_path_changed();
}

String Resource::get_path() const { return path_cache; }

void Resource::set_path_cache(const String& p_path) { path_cache = p_path; }

static thread_local RandomPCG unique_id_gen = RandomPCG(0);

void Resource::seed_scene_unique_id(uint32_t p_seed) { unique_id_gen.seed(p_seed); }

String Resource::generate_scene_unique_id()
{
	// Generate a unique enough hash, but still user-readable.
	// If it's not unique it does not matter because the saver will try again.
	if (unique_id_gen.get_seed() == 0) {
		OS::DateTime dt = OS::get_datetime();
		uint32_t hash = hash_murmur3_one_32(OS::get_ticks_usec());
		hash = hash_murmur3_one_32(dt.year, hash);
		hash = hash_murmur3_one_32(dt.month, hash);
		hash = hash_murmur3_one_32(dt.day, hash);
		hash = hash_murmur3_one_32(dt.hour, hash);
		hash = hash_murmur3_one_32(dt.minute, hash);
		hash = hash_murmur3_one_32(dt.second, hash);
		hash = hash_murmur3_one_32(Math::rand(), hash);
		unique_id_gen.seed(hash);
	}

	uint32_t random_num = unique_id_gen.rand();

	static constexpr uint32_t characters = 5;
	static constexpr uint32_t char_count = ('z' - 'a');
	static constexpr uint32_t base = char_count + ('9' - '0');
	String id;
	id.resize_uninitialized(characters + 1);
	char32_t* ptr = id.ptrw();
	for (uint32_t i = 0; i < characters; i++) {
		uint32_t c = random_num % base;
		if (c < char_count) {
			ptr[i] = ('a' + c);
		}
		else {
			ptr[i] = ('0' + (c - char_count));
		}
		random_num /= base;
	}
	ptr[characters] = '\0';

	return id;
}

void Resource::set_scene_unique_id(const String& p_id)
{
	bool is_valid = true;
	for (int i = 0; i < p_id.length(); i++) {
		if (!is_ascii_identifier_char(p_id[i])) {
			is_valid = false;
			scene_unique_id = Resource::generate_scene_unique_id();
			break;
		}
	}

	ERR_FAIL_COND_MSG(
		!is_valid, "The scene unique ID must contain only letters, numbers, and underscores.");
	scene_unique_id = p_id;
}

String Resource::get_scene_unique_id() const { return scene_unique_id; }

Ref<Resource> Resource::duplicate(bool p_deep) const
{
	Ref<Resource> res = memnew(Resource);
	return res;
}

void Resource::set_name(const String& p_name)
{
	name = p_name;
	emit_changed();
}

String Resource::get_name() const { return name; }

void Resource::update_configuration_warning()
{
	if (_update_configuration_warning) {
		_update_configuration_warning();
	}
}

bool Resource::editor_can_reload_from_file()
{
	return true; // by default yes
}

Error Resource::copy_from(const Ref<Resource>& p_resource)
{
	ERR_FAIL_COND_V(p_resource.is_null(), ERR_INVALID_PARAMETER);
	_block_emit_changed();
	reset_state(); // May want to reset state.
	_unblock_emit_changed();
	return OK;
}

void Resource::_set_path(const String& p_path) { set_path(p_path, false); }

void Resource::_take_over_path(const String& p_path) { set_path(p_path, true); }

void Resource::set_local_to_scene(bool p_enable) { local_to_scene = p_enable; }

bool Resource::is_local_to_scene() const { return local_to_scene; }

Node* Resource::get_local_scene() const
{
	if (local_scene) {
		return local_scene;
	}

	if (_get_local_scene_func) {
		return _get_local_scene_func();
	}

	return nullptr;
}

void Resource::reset_local_to_scene()
{
	// Restores the state as if setup_local_to_scene() hadn't been called.
}

Ref<Resource> Resource::_duplicate(bool p_subresources) const
{
	Ref<Resource> r = memnew(Resource);
	return r;
}

String Resource::_to_string()
{
	return (name.is_empty() ? "" : String(name) + " ") + "(" + path_cache +
		   ")";
}

Node* (*Resource::_get_local_scene_func)() = nullptr;
void (*Resource::_update_configuration_warning)() = nullptr;

void Resource::set_as_translation_remapped(bool p_remapped)
{
	if (remapped_list.in_list() == p_remapped) {
		return;
	}

	MutexLock lock(ResourceCache::lock);

	if (p_remapped) {
		ResourceLoader::remapped_list.add(&remapped_list);
	}
	else {
		ResourceLoader::remapped_list.remove(&remapped_list);
	}
}

String Resource::get_id_for_path(const String& p_referrer_path) const
{
	return "";
}

Resource::~Resource()
{
	if (unlikely(path_cache.is_empty())) {
		return;
	}

	MutexLock lock(ResourceCache::lock);
	// Only unregister from the cache if this is the actual resource listed there.
	// (Other resources can have the same value in `path_cache` if loaded with `CACHE_IGNORE`.)
	HashMap<String, Resource*>::Iterator E = ResourceCache::resources.find(path_cache);
	if (likely(E && E->value == this)) {
		ResourceCache::resources.remove(E);
	}
}

HashMap<String, Resource*> ResourceCache::resources;
#ifdef TOOLS_ENABLED
HashMap<String, HashMap<String, String>> ResourceCache::resource_path_cache;
#endif

Mutex ResourceCache::lock;

void ResourceCache::clear()
{
	if (!resources.is_empty()) {
		if (OS::is_stdout_verbose()) {
			ERR_PRINT(vformat("%d resources still in use at exit.", resources.size()));
			for (const KeyValue<String, Resource*>& E : resources) {
				__print_line(vformat("Resource still in use: %s", E.key));
			}
		}
		else {
			ERR_PRINT(vformat("%d resources still in use at exit (run with --verbose for details).",
				resources.size()));
		}
	}

	resources.clear();
}

bool ResourceCache::has(const String& p_path)
{
	Resource** res = nullptr;

	{
		MutexLock mutex_lock(lock);

		res = resources.getptr(p_path);

		if (res && (*res)->get_reference_count() == 0) {
			// This resource is in the process of being deleted, ignore its existence.
			(*res)->path_cache = String();
			resources.erase(p_path);
			res = nullptr;
		}
	}

	if (!res) {
		return false;
	}

	return true;
}

Ref<Resource> ResourceCache::get_ref(const String& p_path)
{
	Ref<Resource> ref;
	{
		MutexLock mutex_lock(lock);
		Resource** res = resources.getptr(p_path);

		if (res) {
			ref = Ref<Resource>(*res);
		}

		if (res && ref.is_null()) {
			// This resource is in the process of being deleted, ignore its existence
			(*res)->path_cache = String();
			resources.erase(p_path);
			res = nullptr;
		}
	}

	return ref;
}

void ResourceCache::get_cached_resources(List<Ref<Resource>>* p_resources)
{
	MutexLock mutex_lock(lock);

	LocalVector<String> to_remove;

	for (KeyValue<String, Resource*>& E : resources) {
		Ref<Resource> ref = Ref<Resource>(E.value);

		if (ref.is_null()) {
			// This resource is in the process of being deleted, ignore its existence
			E.value->path_cache = String();
			to_remove.push_back(E.key);
			continue;
		}

		p_resources->push_back(ref);
	}

	for (const String& E : to_remove) {
		resources.erase(E);
	}
}

int ResourceCache::get_cached_resource_count()
{
	MutexLock mutex_lock(lock);
	return resources.size();
}

void Resource::reset_state()
{
	// stub
}

RID Resource::get_rid() const { return RID(); }

void Resource::reload_from_file() {}

void Resource::setup_local_to_scene() {}


// ============================================================================
// Merged from core/io/resource_format_binary.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/resource_loader.h"
#include "core/version.h"
#include "core/io/resource_loader.h"
#include "scene/main/node.h"

// #define print_bl(m_what) print_line(m_what)
#define print_bl(m_what) (void)(m_what)

enum
{
	// numbering must be different from variant, in case new variant types are added (variant must
	// be always contiguous for jumptable optimization)
	VARIANT_NIL = 1,
	VARIANT_BOOL = 2,
	VARIANT_INT = 3,
	VARIANT_FLOAT = 4,
	VARIANT_STRING = 5,
	VARIANT_VECTOR2 = 10,
	VARIANT_RECT2 = 11,
	VARIANT_VECTOR3 = 12,
	VARIANT_PLANE = 13,
	VARIANT_QUATERNION = 14,
	VARIANT_AABB = 15,
	VARIANT_BASIS = 16,
	VARIANT_TRANSFORM3D = 17,
	VARIANT_TRANSFORM2D = 18,
	VARIANT_COLOR = 20,
	VARIANT_NODE_PATH = 22,
	VARIANT_RID = 23,
	VARIANT_OBJECT = 24,
	VARIANT_INPUT_EVENT = 25,
	VARIANT_DICTIONARY = 26,
	VARIANT_ARRAY = 30,
	VARIANT_PACKED_BYTE_ARRAY = 31,
	VARIANT_PACKED_INT32_ARRAY = 32,
	VARIANT_PACKED_FLOAT32_ARRAY = 33,
	VARIANT_PACKED_STRING_ARRAY = 34,
	VARIANT_PACKED_VECTOR3_ARRAY = 35,
	VARIANT_PACKED_COLOR_ARRAY = 36,
	VARIANT_PACKED_VECTOR2_ARRAY = 37,
	VARIANT_INT64 = 40,
	VARIANT_DOUBLE = 41,
	VARIANT_CALLABLE = 42,
	VARIANT_SIGNAL = 43,
	VARIANT_STRING_NAME = 44,
	VARIANT_VECTOR2I = 45,
	VARIANT_RECT2I = 46,
	VARIANT_VECTOR3I = 47,
	VARIANT_PACKED_INT64_ARRAY = 48,
	VARIANT_PACKED_FLOAT64_ARRAY = 49,
	VARIANT_VECTOR4 = 50,
	VARIANT_VECTOR4I = 51,
	VARIANT_PROJECTION = 52,
	VARIANT_PACKED_VECTOR4_ARRAY = 53,
	OBJECT_EMPTY = 0,
	OBJECT_EXTERNAL_RESOURCE = 1,
	OBJECT_INTERNAL_RESOURCE = 2,
	OBJECT_EXTERNAL_RESOURCE_INDEX = 3,
	// Version 2: Added 64-bit support for float and int.
	// Version 3: Changed NodePath encoding.
	// Version 4: New string ID for ext/subresources, breaks forward compat.
	// Version 5: Ability to store script class in the header.
	// Version 6: Added PackedVector4Array Variant type.
	FORMAT_VERSION = 6,
	FORMAT_VERSION_CAN_RENAME_DEPS = 1,
	FORMAT_VERSION_NO_NODEPATH_PROPERTY = 3,
};

void ResourceLoaderBinary::_advance_padding(uint32_t p_len)
{
	uint32_t extra = 4 - (p_len % 4);
	if (extra < 4) {
		for (uint32_t i = 0; i < extra; i++) {
			f->get_8(); // pad to 32
		}
	}
}

static Error read_reals(real_t* r_dst, Ref<FileAccess>& r_file, size_t p_count)
{
	if (r_file->real_is_double) {
		if constexpr (sizeof(real_t) == 8) {
			// Ideal case with double-precision
			r_file->get_buffer((uint8_t*)r_dst, p_count * sizeof(double));
		}
		else if constexpr (sizeof(real_t) == 4) {
			// May be slower, but this is for compatibility. Eventually the data should be
			// converted.
			for (size_t i = 0; i < p_count; ++i) {
				r_dst[i] = r_file->get_double();
			}
		}
		else {
			ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "real_t size is neither 4 nor 8!");
		}
	}
	else {
		if constexpr (sizeof(real_t) == 4) {
			// Ideal case with float-precision
			r_file->get_buffer((uint8_t*)r_dst, p_count * sizeof(float));
		}
		else if constexpr (sizeof(real_t) == 8) {
			for (size_t i = 0; i < p_count; ++i) {
				r_dst[i] = r_file->get_float();
			}
		}
		else {
			ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "real_t size is neither 4 nor 8!");
		}
	}
	return OK;
}

String ResourceLoaderBinary::_get_string()
{
	uint32_t id = f->get_32();
	if (id & 0x80000000) {
		uint32_t len = id & 0x7FFFFFFF;
		if ((int)len > str_buf.size()) {
			str_buf.resize(len);
		}
		if (len == 0) {
			return String();
		}
		f->get_buffer((uint8_t*)&str_buf[0], len);
		return String::utf8(&str_buf[0], len);
	}

	return string_map[id];
}

Ref<Resource> ResourceLoaderBinary::get_resource() { return resource; }

Error ResourceLoaderBinary::load()
{
	if (error != OK) {
		return error;
	}

	for (int i = 0; i < external_resources.size(); i++) {
		String path = external_resources[i].path;

		if (remaps.has(path)) {
			path = remaps[path];
		}

		if (!path.contains("://") && path.is_relative_path()) {
			// path is relative to file being loaded, so convert to a resource path
			path = ProjectSettings::localize_path(
				path.get_base_dir().path_join(external_resources[i].path));
		}

		external_resources.write[i].path =
			path; // remap happens here, not on load because on load it can actually be used for
				  // filesystem dock resource remap
		external_resources.write[i].load_token =
			ResourceLoader::_load_start(path, external_resources[i].type,
				use_sub_threads ? ResourceLoader::LOAD_THREAD_DISTRIBUTE
								: ResourceLoader::LOAD_THREAD_FROM_CURRENT,
				cache_mode_for_external);
		if (external_resources[i].load_token.is_null()) {
			if (!ResourceLoader::get_abort_on_missing_resources()) {
				ResourceLoader::notify_dependency_error(
					local_path, path, external_resources[i].type);
			}
			else {
				error = ERR_FILE_MISSING_DEPENDENCIES;
				ERR_FAIL_V_MSG(error, vformat("Can't load dependency: '%s'.", path));
			}
		}
	}

	for (int i = 0; i < internal_resources.size(); i++) {
		bool main = i == (internal_resources.size() - 1);

		// maybe it is loaded already
		String path;
		String id;

		if (!main) {
			path = internal_resources[i].path;

			if (path.begins_with("local://")) {
				path = path.replace_first("local://", "");
				id = path;
				path = res_path + "::" + path;

				internal_resources.write[i].path = path; // Update path.
			}

			if (cache_mode == ResourceFormatLoader::CACHE_MODE_REUSE && ResourceCache::has(path)) {
				Ref<Resource> cached = ResourceCache::get_ref(path);
				if (cached.is_valid()) {
					// already loaded, don't do anything
					error = OK;
					internal_index_cache[path] = cached;
					continue;
				}
			}
		}
		else {
			if (cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE &&
				!ResourceCache::has(res_path)) {
				path = res_path;
			}
		}

		uint64_t offset = internal_resources[i].offset;

		f->seek(offset);

		String t = get_unicode_string();

		Ref<Resource> res;
		Resource* r = nullptr;

		Ref<MissingResource> missing_resource;

		if (main) {
			res = ResourceLoader::get_resource_ref_override(local_path);
			r = res.ptr();
		}
		if (!r) {
			if (cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE &&
				ResourceCache::has(path)) {
			}
			if (res.is_null()) {
				if (!r) {
					error = ERR_FILE_CORRUPT;
				}
				res = Ref<Resource>(r);
			}
		}

		if (r) {
			if (!path.is_empty()) {
				if (cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE) {
					r->set_path(path,
						cache_mode ==
							ResourceFormatLoader::
								CACHE_MODE_REPLACE); // If got here because the resource with same
													 // path has different type, replace it.
				}
				else {
					r->set_path_cache(path);
				}
			}
			r->set_scene_unique_id(id);
		}

		if (!main) {
			internal_index_cache[path] = res;
		}

		int pc = f->get_32();

		for (int j = 0; j < pc; j++) {
			String name = _get_string();

			if (name == String()) {
				error = ERR_FILE_CORRUPT;
				ERR_FAIL_V(ERR_FILE_CORRUPT);
			}

			if (error) {
				return error;
			}

			bool set_valid = true;
		}

		if (missing_resource.is_valid()) {
			missing_resource->set_recording_properties(false);
		}

		if (progress) {
			*progress = (i + 1) / float(internal_resources.size());
		}

		resource_cache.push_back(res);

		if (main) {
			f.unref();
			resource = res;
			resource->set_as_translation_remapped(translation_remapped);
			error = OK;
			return OK;
		}
	}

	return ERR_FILE_EOF;
}

void ResourceLoaderBinary::set_translation_remapped(bool p_remapped)
{
	translation_remapped = p_remapped;
}

static void save_ustring(Ref<FileAccess> r_file, const String& p_string)
{
	CharString utf8 = p_string.utf8();
	r_file->store_32(uint32_t(utf8.length() + 1));
	r_file->store_buffer((const uint8_t*)utf8.get_data(), utf8.length() + 1);
}

static String get_ustring(Ref<FileAccess> r_file)
{
	int len = r_file->get_32();
	Vector<char> str_buf;
	str_buf.resize(len);
	r_file->get_buffer((uint8_t*)&str_buf[0], len);
	return String::utf8(&str_buf[0], len);
}

String ResourceLoaderBinary::get_unicode_string()
{
	int len = f->get_32();
	if (len > str_buf.size()) {
		str_buf.resize(len);
	}
	if (len == 0) {
		return String();
	}
	f->get_buffer((uint8_t*)&str_buf[0], len);
	return String::utf8(&str_buf[0], len);
}

void ResourceLoaderBinary::get_classes_used(Ref<FileAccess> p_file, HashSet<String>* p_classes)
{
	open(p_file, false, true);
	if (error) {
		return;
	}

	for (const IntResource& res : internal_resources) {
		p_file->seek(res.offset);
		String t = get_unicode_string();
		if (!p_file->get_error() && t != String()) {
			p_classes->insert(t);
		}
	}
}

void ResourceLoaderBinary::get_dependencies(
	Ref<FileAccess> p_file, List<String>* p_dependencies, bool p_add_types)
{
	open(p_file, false, true);
	if (error) {
		return;
	}

	for (int i = 0; i < external_resources.size(); i++) {
		String dep;
		String fallback_path;

		if (external_resources[i].uid != ResourceUID::INVALID_ID) {
			dep = ResourceUID::id_to_text(external_resources[i].uid);
			fallback_path =
				external_resources[i].path; // Used by Dependency Editor, in case uid path fails.
		}
		else {
			dep = external_resources[i].path;
		}

		if (p_add_types && !external_resources[i].type.is_empty()) {
			dep += "::" + external_resources[i].type;
		}
		if (!fallback_path.is_empty()) {
			if (!p_add_types) {
				dep += "::"; // Ensure that path comes third, even if there is no type.
			}
			dep += "::" + fallback_path;
		}

		p_dependencies->push_back(dep);
	}
}

void ResourceLoaderBinary::open(Ref<FileAccess> p_file, bool p_no_resources, bool p_keep_uuid_paths)
{
	error = OK;

	f = p_file;
	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		error = fac->open_after_magic(f);
		if (error != OK) {
			f.unref();
			ERR_FAIL_MSG(vformat("Failed to open binary resource file: '%s'.", local_path));
		}
		f = fac;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		error = ERR_FILE_UNRECOGNIZED;
		f.unref();
		ERR_FAIL_MSG(vformat("Unrecognized binary resource file: '%s'.", local_path));
	}

	bool big_endian = f->get_32();
	bool use_real64 = f->get_32();

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	ver_format = f->get_32();

	print_bl("big endian: " + itos(big_endian));
	print_bl("endian swap: " + itos(big_endian));
	print_bl("real64: " + itos(use_real64));
	print_bl("major: " + itos(ver_major));
	print_bl("minor: " + itos(ver_minor));
	print_bl("format: " + itos(ver_format));

	if (ver_format > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		f.unref();
		ERR_FAIL_MSG(
			vformat("File '%s' can't be loaded, as it uses a format version (%d) or engine version "
					"(%d.%d) which are not supported by your engine version (%s).",
				local_path, ver_format, ver_major, ver_minor, VLTR_VERSION_BRANCH));
	}

	type = get_unicode_string();

	print_bl("type: " + type);

	importmd_ofs = f->get_64();
	uint32_t flags = f->get_32();
	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_NAMED_SCENE_IDS) {
		using_named_scene_ids = true;
	}
	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_UIDS) {
		using_uids = true;
	}
	f->real_is_double =
		(flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_REAL_T_IS_DOUBLE) != 0;

	if (using_uids) {
		uid = ResourceUID::ID(f->get_64());
	}
	else {
		f->get_64(); // skip over uid field
		uid = ResourceUID::INVALID_ID;
	}

	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		script_class = get_unicode_string();
	}

	for (int i = 0; i < ResourceFormatSaverBinaryInstance::RESERVED_FIELDS; i++) {
		f->get_32(); // skip a few reserved fields
	}

	if (p_no_resources) {
		return;
	}

	uint32_t string_table_size = f->get_32();
	string_map.resize(string_table_size);
	for (uint32_t i = 0; i < string_table_size; i++) {
		String s = get_unicode_string();
		string_map.write[i] = s;
	}

	print_bl("strings: " + itos(string_table_size));

	uint32_t ext_resources_size = f->get_32();
	for (uint32_t i = 0; i < ext_resources_size; i++) {
		ExtResource er;
		er.type = get_unicode_string();
		er.path = get_unicode_string();
		if (using_uids) {
			er.uid = ResourceUID::ID(f->get_64());
			if (!p_keep_uuid_paths && er.uid != ResourceUID::INVALID_ID) {
				if (ResourceUID::has_id(er.uid)) {
					// If a UID is found and the path is valid, it will be used, otherwise, it falls
					// back to the path.
					er.path = ResourceUID::get_id_path(er.uid);
				}
				else {
#ifdef TOOLS_ENABLED
					// Silence a warning that can happen during the initial filesystem scan due to
					// cache being regenerated.
					if (ResourceLoader::get_resource_uid(res_path) != er.uid) {
						WARN_PRINT(vformat("'%s': In external resource #%d, invalid UID: '%s' - "
										   "using text path instead: '%s'.",
							res_path, i, ResourceUID::id_to_text(er.uid),
							er.path));
					}
#else
					WARN_PRINT(vformat("'%s': In external resource #%d, invalid UID: '%s' - using "
									   "text path instead: '%s'.",
						res_path, i, ResourceUID::id_to_text(er.uid), er.path));
#endif
				}
			}
		}

		external_resources.push_back(er);
	}

	print_bl("ext resources: " + itos(ext_resources_size));
	uint32_t int_resources_size = f->get_32();

	for (uint32_t i = 0; i < int_resources_size; i++) {
		IntResource ir;
		ir.path = get_unicode_string();
		ir.offset = f->get_64();
		internal_resources.push_back(ir);
	}

	print_bl("int resources: " + itos(int_resources_size));

	if (f->eof_reached()) {
		error = ERR_FILE_CORRUPT;
		f.unref();
		ERR_FAIL_MSG(vformat("Premature end of file (EOF): '%s'.", local_path));
	}
}

String ResourceLoaderBinary::recognize(Ref<FileAccess> p_file)
{
	error = OK;

	f = p_file;
	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		error = fac->open_after_magic(f);
		if (error != OK) {
			f.unref();
			return "";
		}
		f = fac;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		error = ERR_FILE_UNRECOGNIZED;
		f.unref();
		return "";
	}

	bool big_endian = f->get_32();
	f->get_32(); // use_real64

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	uint32_t ver_major = f->get_32();
	f->get_32(); // ver_minor
	uint32_t ver_fmt = f->get_32();

	if (ver_fmt > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		f.unref();
		return "";
	}

	return get_unicode_string();
}

String ResourceLoaderBinary::recognize_script_class(Ref<FileAccess> p_file)
{
	error = OK;

	f = p_file;
	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		error = fac->open_after_magic(f);
		if (error != OK) {
			f.unref();
			return "";
		}
		f = fac;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		error = ERR_FILE_UNRECOGNIZED;
		f.unref();
		return "";
	}

	bool big_endian = f->get_32();
	f->get_32(); // use_real64

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	uint32_t ver_major = f->get_32();
	f->get_32(); // ver_minor
	uint32_t ver_fmt = f->get_32();

	if (ver_fmt > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		f.unref();
		return "";
	}

	_ALLOW_DISCARD_ get_unicode_string(); // type

	f->get_64(); // Metadata offset
	uint32_t flags = f->get_32();
	f->get_64(); // UID

	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		return get_unicode_string();
	}
	else {
		return String();
	}
}

Ref<Resource> ResourceFormatLoaderBinary::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	switch (p_cache_mode) {
	case CACHE_MODE_IGNORE:
	case CACHE_MODE_REUSE:
	case CACHE_MODE_REPLACE:
		loader.cache_mode = p_cache_mode;
		loader.cache_mode_for_external = CACHE_MODE_REUSE;
		break;
	case CACHE_MODE_IGNORE_DEEP:
		loader.cache_mode = CACHE_MODE_IGNORE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	case CACHE_MODE_REPLACE_DEEP:
		loader.cache_mode = CACHE_MODE_REPLACE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	}
	loader.use_sub_threads = p_use_sub_threads;
	loader.progress = r_progress;
	String path = !p_original_path.is_empty() ? p_original_path : p_path;
	loader.local_path = ProjectSettings::localize_path(path);
	loader.res_path = loader.local_path;
	loader.open(f);

	err = loader.load();

	if (r_error) {
		*r_error = err;
	}

	if (err) {
		return Ref<Resource>();
	}
	return loader.resource;
}

void ResourceFormatLoaderBinary::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}

	// res files not supported for GDExtension.
	if (p_type == "GDExtension") {
		return;
	}
}

void ResourceFormatLoaderBinary::get_recognized_extensions(List<String>* p_extensions) const {}

bool ResourceFormatLoaderBinary::handles_type(const String& p_type) const
{
	return true; // handles all
}

void ResourceFormatLoaderBinary::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(f.is_null(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.get_dependencies(f, p_dependencies, p_add_types);
}

Error ResourceFormatLoaderBinary::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Cannot open file '%s'.", p_path));

	Ref<FileAccess> fw;

	String local_path = p_path.get_base_dir();

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		Error err = fac->open_after_magic(f);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot open file '%s'.", p_path));
		f = fac;

		Ref<FileAccessCompressed> facw;
		facw.instantiate();
		facw->configure("RSCC");
		err = facw->open_internal(p_path + ".depren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			err, ERR_FILE_CORRUPT, vformat("Cannot create file '%s.depren'.", p_path));

		fw = facw;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		ERR_FAIL_V_MSG(
			ERR_FILE_UNRECOGNIZED, vformat("Unrecognized binary resource file '%s'.", local_path));
	}
	else {
		fw = FileAccess::open(p_path + ".depren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			fw.is_null(), ERR_CANT_CREATE, vformat("Cannot create file '%s.depren'.", p_path));

		uint8_t magic[4] = {'R', 'S', 'R', 'C'};
		fw->store_buffer(magic, 4);
	}

	bool big_endian = f->get_32();
	bool use_real64 = f->get_32();

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	fw->store_32(big_endian);
	fw->store_32(use_real64); // use real64
	fw->set_big_endian(big_endian != 0);

	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	uint32_t ver_format = f->get_32();

	if (ver_format < FORMAT_VERSION_CAN_RENAME_DEPS) {
		fw.unref();

		{
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			da->remove(p_path + ".depren");
		}

		// Use the old approach.

		WARN_PRINT(vformat(
			"This file is old, so it can't refactor dependencies, opening and resaving '%s'.",
			p_path));

		Error err;
		f = FileAccess::open(p_path, FileAccess::READ, &err);

		ERR_FAIL_COND_V_MSG(
			err != OK, ERR_FILE_CANT_OPEN, vformat("Cannot open file '%s'.", p_path));

		ResourceLoaderBinary loader;
		loader.local_path = ProjectSettings::localize_path(p_path);
		loader.res_path = loader.local_path;
		loader.remaps = p_map;
		loader.open(f);

		err = loader.load();

		ERR_FAIL_COND_V(err != ERR_FILE_EOF, ERR_FILE_CORRUPT);
		Ref<Resource> res = loader.get_resource();
		ERR_FAIL_COND_V(res.is_null(), ERR_FILE_CORRUPT);

		ResourceFormatSaverBinaryInstance saver;
		return saver.save(ProjectSettings::localize_path(p_path), res);
	}

	if (ver_format > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED,
			vformat("File '%s' can't be loaded, as it uses a format version (%d) or engine version "
					"(%d.%d) which are not supported by your engine version (%s).",
				local_path, ver_format, ver_major, ver_minor, VLTR_VERSION_BRANCH));
	}

	// Since we're not actually converting the file contents, leave the version
	// numbers in the file untouched.
	fw->store_32(ver_major);
	fw->store_32(ver_minor);
	fw->store_32(ver_format);

	save_ustring(fw, get_ustring(f)); // type

	uint64_t md_ofs = f->get_position();
	uint64_t importmd_ofs = f->get_64();
	fw->store_64(0); // metadata offset

	uint32_t flags = f->get_32();
	bool using_uids = (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_UIDS);
	uint64_t uid_data = f->get_64();

	fw->store_32(flags);
	fw->store_64(uid_data);
	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		save_ustring(fw, get_ustring(f));
	}

	for (int i = 0; i < ResourceFormatSaverBinaryInstance::RESERVED_FIELDS; i++) {
		fw->store_32(0); // reserved
		f->get_32();
	}

	// string table
	uint32_t string_table_size = f->get_32();

	fw->store_32(string_table_size);

	for (uint32_t i = 0; i < string_table_size; i++) {
		String s = get_ustring(f);
		save_ustring(fw, s);
	}

	// external resources
	uint32_t ext_resources_size = f->get_32();
	fw->store_32(ext_resources_size);
	for (uint32_t i = 0; i < ext_resources_size; i++) {
		String type = get_ustring(f);
		String path = get_ustring(f);

		if (using_uids) {
			ResourceUID::ID uid = f->get_64();
			if (uid != ResourceUID::INVALID_ID) {
				if (ResourceUID::has_id(uid)) {
					// If a UID is found and the path is valid, it will be used, otherwise, it falls
					// back to the path.
					path = ResourceUID::get_id_path(uid);
				}
			}
		}

		bool relative = false;
		if (!path.begins_with("res://")) {
			path = local_path.path_join(path).simplify_path();
			relative = true;
		}

		if (p_map.has(path)) {
			String np = p_map[path];
			path = np;
		}

		String full_path = path;

		if (relative) {
			// restore relative
			path = local_path.path_to_file(path);
		}

		save_ustring(fw, type);
		save_ustring(fw, path);

		if (using_uids) {
			ResourceUID::ID uid = ResourceSaver::get_resource_id_for_path(full_path);
			fw->store_64(uint64_t(uid));
		}
	}

	int64_t size_diff = (int64_t)fw->get_position() - (int64_t)f->get_position();

	// internal resources
	uint32_t int_resources_size = f->get_32();
	fw->store_32(int_resources_size);

	for (uint32_t i = 0; i < int_resources_size; i++) {
		String path = get_ustring(f);
		uint64_t offset = f->get_64();
		save_ustring(fw, path);
		fw->store_64(offset + size_diff);
	}

	// rest of file
	uint8_t b = f->get_8();
	while (!f->eof_reached()) {
		fw->store_8(b);
		b = f->get_8();
	}
	f.unref();

	bool all_ok = fw->get_error() == OK;

	fw->seek(md_ofs);
	fw->store_64(importmd_ofs + size_diff);

	if (!all_ok) {
		return ERR_CANT_CREATE;
	}

	fw.unref();

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (da->exists(p_path + ".depren")) {
		da->remove(p_path);
		da->rename(p_path + ".depren", p_path);
	}
	return OK;
}

void ResourceFormatLoaderBinary::get_classes_used(
	const String& p_path, HashSet<String>* r_classes)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(f.is_null(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.get_classes_used(f, r_classes);

	if (loader.type != "PackedScene") {
		return;
	}

	// Fetch the nodes inside scene files.

	// Reopening is necessary, or errors will occur.
	f->reopen(p_path, FileAccess::READ);
	loader.open(f);
	ERR_FAIL_COND(loader.load() != OK);

	Ref<SceneState> state = Ref<PackedScene>(loader.get_resource())->get_state();
	for (int i = 0; i < state->get_node_count(); i++) {
		const String node_name = state->get_node_type(i);
		// Fetch the values of properties in the node.
	}
}

String ResourceFormatLoaderBinary::get_resource_type(const String& p_path) const
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	String r = loader.recognize(f);
	return r;
}

String ResourceFormatLoaderBinary::get_resource_script_class(const String& p_path) const
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	return loader.recognize_script_class(f);
}

ResourceUID::ID ResourceFormatLoaderBinary::get_resource_uid(const String& p_path) const
{
	String ext = p_path.get_extension().to_lower();
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ResourceUID::INVALID_ID; // could not read
	}

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.open(f, true);
	if (loader.error != OK) {
		return ResourceUID::INVALID_ID; // could not read
	}
	return loader.uid;
}

bool ResourceFormatLoaderBinary::has_custom_uid_support() const { return true; }

void ResourceFormatSaverBinaryInstance::_pad_buffer(Ref<FileAccess> r_file, int p_bytes)
{
	int extra = 4 - (p_bytes % 4);
	if (extra < 4) {
		for (int i = 0; i < extra; i++) {
			r_file->store_8(0); // pad to 32
		}
	}
}

void ResourceFormatSaverBinaryInstance::save_unicode_string(
	Ref<FileAccess> r_file, const String& p_string, bool p_bit_on_len)
{
	CharString utf8 = p_string.utf8();
	if (p_bit_on_len) {
		r_file->store_32(uint32_t((utf8.length() + 1) | 0x80000000));
	}
	else {
		r_file->store_32(uint32_t(utf8.length() + 1));
	}
	r_file->store_buffer((const uint8_t*)utf8.get_data(), utf8.length() + 1);
}

int ResourceFormatSaverBinaryInstance::get_string_index(const String& p_string)
{
	String s = p_string;
	if (string_map.has(s)) {
		return string_map[s];
	}

	string_map[s] = strings.size();
	strings.push_back(s);
	return strings.size() - 1;
}

static String _resource_get_class(Ref<Resource> p_resource)
{
	Ref<MissingResource> missing_resource = p_resource;
	if (missing_resource.is_valid()) {
		return missing_resource->get_original_class();
	}
	return String();
}

Error ResourceFormatSaverBinaryInstance::save(
	const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags)
{
	Resource::seed_scene_unique_id(p_path.hash());

	Error err;
	Ref<FileAccess> f;
	if (p_flags & ResourceSaver::FLAG_COMPRESS) {
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		fac->configure("RSCC");
		f = fac;
		err = fac->open_internal(p_path, FileAccess::WRITE);
	}
	else {
		f = FileAccess::open(p_path, FileAccess::WRITE, &err);
	}

	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot create file '%s'.", p_path));

	relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
	skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
	bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;
	big_endian = p_flags & ResourceSaver::FLAG_SAVE_BIG_ENDIAN;
	takeover_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	if (!p_path.begins_with("res://")) {
		takeover_paths = false;
	}

	local_path = p_path.get_base_dir();
	path = ProjectSettings::localize_path(p_path);

	if (!(p_flags & ResourceSaver::FLAG_COMPRESS)) {
		// save header compressed
		static const uint8_t header[4] = {'R', 'S', 'R', 'C'};
		f->store_buffer(header, 4);
	}

	if (big_endian) {
		f->store_32(1);
	}
	else {
		f->store_32(0);
	}
	f->store_32(0); // 64 bits file, false for now
	f->set_big_endian(big_endian);

	f->store_32(VLTR_VERSION_MAJOR);
	f->store_32(VLTR_VERSION_MINOR);
	f->store_32(FORMAT_VERSION);

	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	save_unicode_string(f, _resource_get_class(p_resource));
	f->store_64(0); // offset to import metadata

	String script_class;
	{
		uint32_t format_flags = FORMAT_FLAG_NAMED_SCENE_IDS | FORMAT_FLAG_UIDS;
#ifdef REAL_T_IS_DOUBLE
		format_flags |= FORMAT_FLAG_REAL_T_IS_DOUBLE;
#endif
		f->store_32(format_flags);
	}
	ResourceUID::ID uid = ResourceSaver::get_resource_id_for_path(p_path, true);
	f->store_64(uint64_t(uid));
	if (!script_class.is_empty()) {
		save_unicode_string(f, script_class);
	}

	for (int i = 0; i < ResourceFormatSaverBinaryInstance::RESERVED_FIELDS; i++) {
		f->store_32(0); // reserved
	}

	List<ResourceData> resources;

	{
		for (const Ref<Resource>& E : saved_resources) {
			ResourceData& rd = resources.push_back(ResourceData())->get();
			rd.type = _resource_get_class(E);
		}
	}

	f->store_32(uint32_t(strings.size())); // string table size
	for (int i = 0; i < strings.size(); i++) {
		save_unicode_string(f, strings[i]);
	}

	// save external resource table
	f->store_32(external_resources.size()); // amount of external resources
	Vector<Ref<Resource>> save_order;
	save_order.resize(external_resources.size());

	for (const KeyValue<Ref<Resource>, int>& E : external_resources) {
		save_order.write[E.value] = E.key;
	}

	for (int i = 0; i < save_order.size(); i++) {
		String res_path = save_order[i]->get_path();
		res_path = relative_paths ? local_path.path_to_file(res_path) : res_path;
		save_unicode_string(f, res_path);
		ResourceUID::ID ruid =
			ResourceSaver::get_resource_id_for_path(save_order[i]->get_path(), false);
		f->store_64(uint64_t(ruid));
	}
	// save internal resource table
	f->store_32(uint32_t(saved_resources.size())); // amount of internal resources
	Vector<uint64_t> ofs_pos;
	HashSet<String> used_unique_ids;

	for (Ref<Resource>& r : saved_resources) {
		if (r->is_built_in()) {
			if (!r->get_scene_unique_id().is_empty()) {
				if (used_unique_ids.has(r->get_scene_unique_id())) {
					r->set_scene_unique_id("");
				}
				else {
					used_unique_ids.insert(r->get_scene_unique_id());
				}
			}
		}
	}

	HashMap<Ref<Resource>, int> resource_map;
	int res_index = 0;
	for (Ref<Resource>& r : saved_resources) {
		if (r->is_built_in()) {
			if (r->get_scene_unique_id().is_empty()) {
				String new_id;

				while (true) {
					new_id = _resource_get_class(r) + "_" + Resource::generate_scene_unique_id();
					if (!used_unique_ids.has(new_id)) {
						break;
					}
				}

				r->set_scene_unique_id(new_id);
				used_unique_ids.insert(new_id);
			}

			save_unicode_string(f, "local://" + r->get_scene_unique_id());
			if (takeover_paths) {
				r->set_path(p_path + "::" + r->get_scene_unique_id(), true);
			}
		}
		else {
			save_unicode_string(f, r->get_path()); // actual external
		}
		ofs_pos.push_back(f->get_position());
		f->store_64(0); // offset in 64 bits
		resource_map[r] = res_index++;
	}

	Vector<uint64_t> ofs_table;

	// now actually save the resources
	for (const ResourceData& rd : resources) {
		ofs_table.push_back(f->get_position());
		save_unicode_string(f, rd.type);
		f->store_32(uint32_t(rd.properties.size()));

		for (const Property& p : rd.properties) {
			f->store_32(uint32_t(p.name_idx));
		}
	}

	for (int i = 0; i < ofs_table.size(); i++) {
		f->seek(ofs_pos[i]);
		f->store_64(ofs_table[i]);
	}

	f->seek_end();

	f->store_buffer((const uint8_t*)"RSRC", 4); // magic at end

	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Error ResourceFormatSaverBinaryInstance::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Cannot open file '%s'.", p_path));

	Ref<FileAccess> fw;

	local_path = p_path.get_base_dir();

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		Error err = fac->open_after_magic(f);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot open file '%s'.", p_path));
		f = fac;

		Ref<FileAccessCompressed> facw;
		facw.instantiate();
		facw->configure("RSCC");
		err = facw->open_internal(p_path + ".uidren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			err, ERR_FILE_CORRUPT, vformat("Cannot create file '%s.uidren'.", p_path));

		fw = facw;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not a binary resource.
		return ERR_FILE_UNRECOGNIZED;
	}
	else {
		fw = FileAccess::open(p_path + ".uidren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			fw.is_null(), ERR_CANT_CREATE, vformat("Cannot create file '%s.uidren'.", p_path));

		uint8_t magich[4] = {'R', 'S', 'R', 'C'};
		fw->store_buffer(magich, 4);
	}

	big_endian = f->get_32();
	bool use_real64 = f->get_32();
	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	fw->store_32(big_endian);
	fw->store_32(use_real64); // use real64
	fw->set_big_endian(big_endian != 0);

	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	uint32_t ver_format = f->get_32();

	if (ver_format < FORMAT_VERSION_CAN_RENAME_DEPS) {
		fw.unref();

		{
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			da->remove(p_path + ".uidren");
		}

		// Use the old approach.

		WARN_PRINT(vformat(
			"This file is old, so it does not support UIDs, opening and resaving '%s'.", p_path));
		return ERR_UNAVAILABLE;
	}

	if (ver_format > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED,
			vformat("File '%s' can't be loaded, as it uses a format version (%d) or engine version "
					"(%d.%d) which are not supported by your engine version (%s).",
				local_path, ver_format, ver_major, ver_minor, VLTR_VERSION_BRANCH));
	}

	// Since we're not actually converting the file contents, leave the version
	// numbers in the file untouched.
	fw->store_32(ver_major);
	fw->store_32(ver_minor);
	fw->store_32(ver_format);

	save_ustring(fw, get_ustring(f)); // type

	fw->store_64(f->get_64()); // metadata offset

	uint32_t flags = f->get_32();
	flags |= ResourceFormatSaverBinaryInstance::FORMAT_FLAG_UIDS;
	f->get_64(); // Skip previous UID

	fw->store_32(flags);
	fw->store_64(uint64_t(p_uid));

	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		save_ustring(fw, get_ustring(f));
	}

	// rest of file
	uint8_t b = f->get_8();
	while (!f->eof_reached()) {
		fw->store_8(b);
		b = f->get_8();
	}

	f.unref();

	bool all_ok = fw->get_error() == OK;

	if (!all_ok) {
		return ERR_CANT_CREATE;
	}

	fw.unref();

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	da->remove(p_path);
	da->rename(p_path + ".uidren", p_path);
	return OK;
}

Error ResourceFormatSaverBinary::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	String local_path = ProjectSettings::localize_path(p_path);
	ResourceFormatSaverBinaryInstance saver;
	return saver.save(local_path, p_resource, p_flags);
}

Error ResourceFormatSaverBinary::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String local_path = ProjectSettings::localize_path(p_path);
	ResourceFormatSaverBinaryInstance saver;
	return saver.set_uid(local_path, p_uid);
}

bool ResourceFormatSaverBinary::recognize(const Ref<Resource>& p_resource) const
{
	return true; // all recognized
}

void ResourceFormatSaverBinary::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	String base = p_resource->get_base_extension().to_lower();
	p_extensions->push_back(base);
	if (base != "res") {
		p_extensions->push_back("res");
	}
}




// ============================================================================
// Merged from core/io/resource_importer.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "core/io/resource_loader.h"

bool ResourceFormatImporter::SortImporterByName::operator()(
	const Ref<ResourceImporter>& p_a, const Ref<ResourceImporter>& p_b) const
{
	return p_a->get_importer_name() < p_b->get_importer_name();
}

Error ResourceFormatImporter::_get_path_and_type(
	const String& p_path, PathAndType& r_path_and_type, bool p_load, bool* r_valid)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path + ".import", FileAccess::READ, &err);

	if (f.is_null()) {
		if (r_valid) {
			*r_valid = false;
		}
		return err;
	}

	if (r_valid) {
		*r_valid = true;
	}

	int lines = 0;
	String error_text;
	bool path_found = false; // First match must have priority.

	String decomp_path;
	bool decomp_path_found = false;
	while (true) {
		if (err == ERR_FILE_EOF) {
			if (p_load && !path_found && decomp_path_found) {
				print_verbose(vformat("No natively supported texture format found for %s, using "
									  "decompressable format %s.",
					p_path, decomp_path));
				r_path_and_type.path = decomp_path;
			}

			return OK;
		}
		else if (err != OK) {
			ERR_PRINT(vformat("ResourceFormatImporter::load - %s.import:%d error: %s.", p_path,
				lines, error_text));
			return err;
		}
	}

	if (p_load && !path_found && decomp_path_found) {
		print_verbose(vformat(
			"No natively supported texture format found for %s, using decompressable format %s.",
			p_path, decomp_path));
		r_path_and_type.path = decomp_path;
		return OK;
	}

	if (r_path_and_type.type.is_empty()) {
		return ERR_FILE_CORRUPT;
	}
	if (r_path_and_type.path.is_empty()) {
		// Some importers may not write files to the .godot folder, so the path can be empty.
		if (r_path_and_type.importer.is_empty()) {
			return ERR_FILE_CORRUPT;
		}

		// It's only invalid if the extension for the importer is not empty.
		Ref<ResourceImporter> importer = get_importer_by_name(r_path_and_type.importer);
		if (importer.is_null() || !importer->get_save_extension().is_empty()) {
			return ERR_FILE_CORRUPT;
		}
	}
	return OK;
}

Ref<Resource> ResourceFormatImporter::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
#ifdef TOOLS_ENABLED
	// When loading a resource on startup, we use the load_on_startup callback,
	// which executes the loading in the EditorFileSystem. It can reimport
	// the resource and retry the load, allowing the resource to be loaded
	// even if it is not yet imported.
	if (ResourceImporter::load_on_startup != nullptr) {
		return ResourceImporter::load_on_startup(
			p_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
	}
#endif

	return load_internal(p_path, r_error, p_use_sub_threads, r_progress, p_cache_mode, false);
}

Ref<Resource> ResourceFormatImporter::load_internal(const String& p_path, Error* r_error,
	bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode, bool p_silence_errors)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, true);

	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}

		return Ref<Resource>();
	}

	if (p_silence_errors) {
		// Note: Some importers do not create files in the .godot folder, so we need to check if the
		// path is empty.
		if (!pat.path.is_empty() && !FileAccess::exists(pat.path)) {
			return Ref<Resource>();
		}
	}

	Ref<Resource> res = ResourceLoader::_load(
		pat.path, p_path, pat.type, p_cache_mode, r_error, p_use_sub_threads, r_progress);

	return res;
}

void ResourceFormatImporter::get_recognized_extensions(List<String>* p_extensions)
{
	HashSet<String> found;

	for (int i = 0; i < data->importers.size(); i++) {
		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (!found.has(F)) {
				p_extensions->push_back(F);
				found.insert(F);
			}
		}
	}
}

void ResourceFormatImporter::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions)
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}

	HashSet<String> found;

	for (int i = 0; i < data->importers.size(); i++) {
		String res_type = data->importers[i]->get_resource_type();
		if (res_type.is_empty()) {
			continue;
		}

		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (!found.has(F)) {
				p_extensions->push_back(F);
				found.insert(F);
			}
		}
	}
}

bool ResourceFormatImporter::exists(const String& p_path)
{
	return FileAccess::exists(p_path + ".import");
}

bool ResourceFormatImporter::recognize_path(const String& p_path, const String& p_for_type)
{
	return FileAccess::exists(p_path + ".import");
}

Error ResourceFormatImporter::get_import_order_threads_and_importer(
	const String& p_path, int& r_order, bool& r_can_threads, String& r_importer)
{
	r_order = 0;
	r_importer = "";

	r_can_threads = false;
	Ref<ResourceImporter> importer;

	if (FileAccess::exists(p_path + ".import")) {
		PathAndType pat;
		Error err = _get_path_and_type(p_path, pat, false);

		if (err == OK) {
			importer = get_importer_by_name(pat.importer);
		}
	}
	else {
		importer = get_importer_by_file(p_path);
	}

	if (importer.is_valid()) {
		r_order = importer->get_import_order();
		r_importer = importer->get_importer_name();
		r_can_threads = importer->can_import_threaded();
		return OK;
	}
	else {
		return ERR_INVALID_PARAMETER;
	}
}

int ResourceFormatImporter::get_import_order(const String& p_path)
{
	Ref<ResourceImporter> importer;

	if (FileAccess::exists(p_path + ".import")) {
		PathAndType pat;
		Error err = _get_path_and_type(p_path, pat, false);

		if (err == OK) {
			importer = get_importer_by_name(pat.importer);
		}
	}
	else {
		importer = get_importer_by_file(p_path);
	}

	if (importer.is_valid()) {
		return importer->get_import_order();
	}

	return 0;
}

bool ResourceFormatImporter::handles_type(const String& p_type)
{
	for (int i = 0; i < data->importers.size(); i++) {
		String res_type = data->importers[i]->get_resource_type();
		if (res_type.is_empty()) {
			continue;
		}
	}

	return true;
}

String ResourceFormatImporter::get_internal_resource_path(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return String();
	}

	return pat.path;
}

String ResourceFormatImporter::get_import_group_file(const String& p_path)
{
	bool valid = true;
	PathAndType pat;
	_get_path_and_type(p_path, pat, false, &valid);
	return valid ? pat.group_file : String();
}

bool ResourceFormatImporter::is_import_valid(const String& p_path)
{
	bool valid = true;
	PathAndType pat;
	_get_path_and_type(p_path, pat, false, &valid);
	return valid;
}

String ResourceFormatImporter::get_resource_type(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return "";
	}

	return pat.type;
}

String ResourceFormatImporter::get_resource_script_class(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return String();
	}

	return ResourceLoader::get_resource_script_class(pat.path);
}

ResourceUID::ID ResourceFormatImporter::get_resource_uid(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return ResourceUID::INVALID_ID;
	}

	return pat.uid;
}

bool ResourceFormatImporter::has_custom_uid_support() { return true; }

Error ResourceFormatImporter::get_resource_import_info(const String& p_path, String& r_type,
	ResourceUID::ID& r_uid, String& r_import_group_file)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err == OK) {
		r_type = pat.type;
		r_uid = pat.uid;
		r_import_group_file = pat.group_file;
	}
	else {
		r_type = "";
		r_uid = ResourceUID::INVALID_ID;
		r_import_group_file = "";
	}

	return err;
}

void ResourceFormatImporter::get_classes_used(const String& p_path, HashSet<String>* r_classes)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return;
	}

	ResourceLoader::get_classes_used(pat.path, r_classes);
}

void ResourceFormatImporter::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return;
	}

	ResourceLoader::get_dependencies(pat.path, p_dependencies, p_add_types);
}

void ResourceFormatImporter::get_build_dependencies(
	const String& p_path, HashSet<String>* r_dependencies)
{
	if (!exists(p_path)) {
		return;
	}

	List<Ref<ResourceImporter>> valid_importers;
	get_importers_for_file(p_path, &valid_importers);
	for (Ref<ResourceImporter> importer : valid_importers) {
		importer->get_build_dependencies(p_path, r_dependencies);
	}
}

Ref<ResourceImporter> ResourceFormatImporter::get_importer_by_name(const String& p_name)
{
	for (int i = 0; i < data->importers.size(); i++) {
		if (data->importers[i]->get_importer_name() == p_name) {
			return data->importers[i];
		}
	}

	return Ref<ResourceImporter>();
}

void ResourceFormatImporter::add_importer(
	const Ref<ResourceImporter>& p_importer, bool p_first_priority)
{
	ERR_FAIL_COND(p_importer.is_null());
	if (p_first_priority) {
		data->importers.insert(0, p_importer);
	}
	else {
		data->importers.push_back(p_importer);
	}
}

void ResourceFormatImporter::get_importers_for_file(
	const String& p_file, List<Ref<ResourceImporter>>* r_importers)
{
	for (int i = 0; i < data->importers.size(); i++) {
		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (p_file.right(F.length()).nocasecmp_to(F) == 0) {
				r_importers->push_back(data->importers[i]);
				break;
			}
		}
	}
}

void ResourceFormatImporter::get_importers(List<Ref<ResourceImporter>>* r_importers)
{
	for (int i = 0; i < data->importers.size(); i++) {
		r_importers->push_back(data->importers[i]);
	}
}

Ref<ResourceImporter> ResourceFormatImporter::get_importer_by_file(const String& p_file)
{
	Ref<ResourceImporter> importer;
	float priority = 0;

	for (int i = 0; i < data->importers.size(); i++) {
		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (p_file.right(F.length()).nocasecmp_to(F) == 0 &&
				data->importers[i]->get_priority() > priority) {
				importer = data->importers[i];
				priority = data->importers[i]->get_priority();
				break;
			}
		}
	}

	return importer;
}

String ResourceFormatImporter::get_import_base_path(const String& p_for_file)
{
	return ProjectSettings::get_imported_files_path().path_join(
		p_for_file.get_file() + "-" + p_for_file.md5_text());
}

bool ResourceFormatImporter::are_import_settings_valid(const String& p_path)
{
	bool valid = true;
	PathAndType pat;
	_get_path_and_type(p_path, pat, false, &valid);

	if (!valid) {
		return false;
	}

	return true;
}

String ResourceFormatImporter::get_import_settings_hash()
{
	Vector<Ref<ResourceImporter>> sorted_importers = data->importers;

	sorted_importers.sort_custom<SortImporterByName>();

	String hash;
	for (int i = 0; i < sorted_importers.size(); i++) {
		hash += ":" + sorted_importers[i]->get_importer_name() + ":" +
				sorted_importers[i]->get_import_settings_string();
	}
	return hash.md5_text();
}

void ResourceFormatImporter::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void ResourceFormatImporter::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void ResourceImporter::get_build_dependencies(const String& p_path, HashSet<String>* r_dependencies)
{
	Vector<String> ret;
	for (int i = 0; i < ret.size(); i++) {
		r_dependencies->insert(ret[i]);
	}
}

Error ResourceFormatImporterSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	Ref<ConfigFile> cf;
	cf.instantiate();
	Error err = cf->load(p_path + ".import");
	if (err != OK) {
		return err;
	}
	cf->save(p_path + ".import");

	return OK;
}



void ResourceFormatImporter::get_internal_resource_path_list(String const&, List<String, DefaultAllocator>*) {}


// --- Merged from core/io/image.cpp ---
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/templates/hash_map.h"
#include "core/io/image.h"

#ifndef DISABLE_DEPRECATED

static Image::CompressProfile _astc_format_to_compress_profile(Image::ASTCFormat p_format)
{
	switch (p_format) {
	case Image::ASTC_FORMAT_4x4:
		return Image::COMPRESS_PROFILE_MAX_QUALITY;
	case Image::ASTC_FORMAT_8x8:
		return Image::COMPRESS_PROFILE_MAX_COMPRESSION;
	default:
		return Image::COMPRESS_PROFILE_AUTOMATIC;
	}
}

Error Image::_compress_bind_compat_115003(
	CompressMode p_mode, CompressSource p_source, ASTCFormat p_format)
{
	return compress(p_mode, p_source, _astc_format_to_compress_profile(p_format));
}

Error Image::_compress_from_channels_bind_compat_115003(
	CompressMode p_mode, UsedChannels p_channels, ASTCFormat p_format)
{
	return compress_from_channels(p_mode, p_channels, _astc_format_to_compress_profile(p_format));
}

Vector<uint8_t> Image::_save_exr_to_buffer_bind_compat_117800(bool p_grayscale) const
{
	return save_exr_to_buffer(p_grayscale);
}

Error Image::_save_exr_bind_compat_117800(const String& p_path, bool p_grayscale) const
{
	return save_exr(p_path, p_grayscale);
}

void Image::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

const char* Image::format_names[Image::FORMAT_MAX] = {
	"Lum8",
	"LumAlpha8",
	"Red8",
	"RedGreen",
	"RGB8",
	"RGBA8",
	"RGBA4444",
	"RGBA5551", // Actually RGB565, kept as RGBA5551 for compatibility.
	"RFloat",
	"RGFloat",
	"RGBFloat",
	"RGBAFloat",
	"RHalf",
	"RGHalf",
	"RGBHalf",
	"RGBAHalf",
	"RGBE9995",
	"DXT1 RGB8",
	"DXT3 RGBA8",
	"DXT5 RGBA8",
	"RGTC Red8",
	"RGTC RedGreen8",
	"BPTC_RGBA",
	"BPTC_RGBF",
	"BPTC_RGBFU",
	"ETC",
	"ETC2_R11",
	"ETC2_R11S",
	"ETC2_RG11",
	"ETC2_RG11S",
	"ETC2_RGB8",
	"ETC2_RGBA8",
	"ETC2_RGB8A1",
	"ETC2_RA_AS_RG",
	"FORMAT_DXT5_RA_AS_RG",
	"ASTC_4x4",
	"ASTC_4x4_HDR",
	"ASTC_8x8",
	"ASTC_8x8_HDR",
	"R16",
	"RG16",
	"RGB16",
	"RGBA16",
	"R16Int",
	"RG16Int",
	"RGB16Int",
	"RGBA16Int",
	"ASTC_6x6",
	"ASTC_6x6_HDR",
};

// External VRAM compression function pointers.

void (*Image::_image_compress_bc_func)(Image*, Image::UsedChannels) = nullptr;
void (*Image::_image_compress_bptc_func)(Image*, Image::UsedChannels, Image::BPTCFormat) = nullptr;
void (*Image::_image_compress_etc1_func)(Image*) = nullptr;
void (*Image::_image_compress_etc2_func)(Image*, Image::UsedChannels) = nullptr;
void (*Image::_image_compress_astc_func)(
	Image*, Image::UsedChannels, Image::CompressProfile) = nullptr;

Error (*Image::_image_compress_bptc_rd_func)(
	Image*, Image::UsedChannels, Image::BPTCFormat) = nullptr;
Error (*Image::_image_compress_bc_rd_func)(Image*, Image::UsedChannels) = nullptr;

// External VRAM decompression function pointers.

void (*Image::_image_decompress_bc)(Image*) = nullptr;
void (*Image::_image_decompress_bptc)(Image*) = nullptr;
void (*Image::_image_decompress_etc1)(Image*) = nullptr;
void (*Image::_image_decompress_etc2)(Image*) = nullptr;
void (*Image::_image_decompress_astc)(Image*) = nullptr;

// External packer function pointers.

Vector<uint8_t> (*Image::webp_lossy_packer)(const Ref<Image>&, float) = nullptr;
Vector<uint8_t> (*Image::webp_lossless_packer)(const Ref<Image>&) = nullptr;
Vector<uint8_t> (*Image::png_packer)(const Ref<Image>&) = nullptr;
Vector<uint8_t> (*Image::basis_universal_packer)(
	const Ref<Image>&, Image::UsedChannels, const BasisUniversalPackerParams&) = nullptr;

Ref<Image> (*Image::webp_unpacker)(const Vector<uint8_t>&) = nullptr;
Ref<Image> (*Image::png_unpacker)(const Vector<uint8_t>&) = nullptr;
Ref<Image> (*Image::basis_universal_unpacker)(const Vector<uint8_t>&) = nullptr;
Ref<Image> (*Image::basis_universal_unpacker_ptr)(const uint8_t*, int) = nullptr;

void Image::_put_pixelb(
	int p_x, int p_y, uint32_t p_pixel_size, uint8_t* p_data, const uint8_t* p_pixel)
{
	uint32_t ofs = (p_y * width + p_x) * p_pixel_size;
	memcpy(p_data + ofs, p_pixel, p_pixel_size);
}

void Image::_get_pixelb(
	int p_x, int p_y, uint32_t p_pixel_size, const uint8_t* p_data, uint8_t* p_pixel)
{
	uint32_t ofs = (p_y * width + p_x) * p_pixel_size;
	memcpy(p_pixel, p_data + ofs, p_pixel_size);
}

int Image::get_format_pixel_size(Format p_format)
{
	switch (p_format) {
	case FORMAT_L8:
		return 1;
	case FORMAT_LA8:
		return 2;
	case FORMAT_R8:
		return 1;
	case FORMAT_RG8:
		return 2;
	case FORMAT_RGB8:
		return 3;
	case FORMAT_RGBA8:
		return 4;
	case FORMAT_RGBA4444:
		return 2;
	case FORMAT_RGB565:
		return 2;
	case FORMAT_RF:
		return 4;
	case FORMAT_RGF:
		return 8;
	case FORMAT_RGBF:
		return 12;
	case FORMAT_RGBAF:
		return 16;
	case FORMAT_RH:
		return 2;
	case FORMAT_RGH:
		return 4;
	case FORMAT_RGBH:
		return 6;
	case FORMAT_RGBAH:
		return 8;
	case FORMAT_RGBE9995:
		return 4;
	case FORMAT_DXT1:
		return 1;
	case FORMAT_DXT3:
		return 1;
	case FORMAT_DXT5:
		return 1;
	case FORMAT_RGTC_R:
		return 1;
	case FORMAT_RGTC_RG:
		return 1;
	case FORMAT_BPTC_RGBA:
		return 1;
	case FORMAT_BPTC_RGBF:
		return 1;
	case FORMAT_BPTC_RGBFU:
		return 1;
	case FORMAT_ETC:
		return 1;
	case FORMAT_ETC2_R11:
		return 1;
	case FORMAT_ETC2_R11S:
		return 1;
	case FORMAT_ETC2_RG11:
		return 1;
	case FORMAT_ETC2_RG11S:
		return 1;
	case FORMAT_ETC2_RGB8:
		return 1;
	case FORMAT_ETC2_RGBA8:
		return 1;
	case FORMAT_ETC2_RGB8A1:
		return 1;
	case FORMAT_ETC2_RA_AS_RG:
		return 1;
	case FORMAT_DXT5_RA_AS_RG:
		return 1;
	case FORMAT_ASTC_4x4:
		return 1;
	case FORMAT_ASTC_4x4_HDR:
		return 1;
	case FORMAT_ASTC_6x6:
		return 1;
	case FORMAT_ASTC_6x6_HDR:
		return 1;
	case FORMAT_ASTC_8x8:
		return 1;
	case FORMAT_ASTC_8x8_HDR:
		return 1;
	case FORMAT_R16:
		return 2;
	case FORMAT_RG16:
		return 4;
	case FORMAT_RGB16:
		return 6;
	case FORMAT_RGBA16:
		return 8;
	case FORMAT_R16I:
		return 2;
	case FORMAT_RG16I:
		return 4;
	case FORMAT_RGB16I:
		return 6;
	case FORMAT_RGBA16I:
		return 8;
	case FORMAT_MAX: {
	}
	}
	return 0;
}

uint64_t Image::get_format_pixels_shifted(Format p_format, uint64_t p_pixels)
{
	switch (p_format) {
	case FORMAT_ASTC_8x8:
	case FORMAT_ASTC_8x8_HDR:
		return p_pixels >> 2;
	case FORMAT_ASTC_6x6:
	case FORMAT_ASTC_6x6_HDR:
		return (p_pixels * 4) / 9;
	case FORMAT_DXT1:
	case FORMAT_RGTC_R:
	case FORMAT_ETC:
	case FORMAT_ETC2_R11:
	case FORMAT_ETC2_R11S:
	case FORMAT_ETC2_RGB8:
	case FORMAT_ETC2_RGB8A1:
		return p_pixels >> 1;
	default:
		return p_pixels;
	}
}

void Image::get_format_min_pixel_size(Format p_format, int& r_w, int& r_h)
{
	switch (p_format) {
	case FORMAT_DXT1:
	case FORMAT_DXT3:
	case FORMAT_DXT5:
	case FORMAT_RGTC_R:
	case FORMAT_RGTC_RG: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ETC: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_BPTC_RGBA:
	case FORMAT_BPTC_RGBF:
	case FORMAT_BPTC_RGBFU: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ETC2_R11:
	case FORMAT_ETC2_R11S:
	case FORMAT_ETC2_RG11:
	case FORMAT_ETC2_RG11S:
	case FORMAT_ETC2_RGB8:
	case FORMAT_ETC2_RGBA8:
	case FORMAT_ETC2_RGB8A1:
	case FORMAT_ETC2_RA_AS_RG:
	case FORMAT_DXT5_RA_AS_RG: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ASTC_4x4:
	case FORMAT_ASTC_4x4_HDR: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ASTC_6x6:
	case FORMAT_ASTC_6x6_HDR: {
		r_w = 6;
		r_h = 6;
	} break;
	case FORMAT_ASTC_8x8:
	case FORMAT_ASTC_8x8_HDR: {
		r_w = 8;
		r_h = 8;
	} break;
	default: {
		r_w = 1;
		r_h = 1;
	} break;
	}
}

int Image::get_format_block_size(Format p_format)
{
	switch (p_format) {
	case FORMAT_DXT1:
	case FORMAT_DXT3:
	case FORMAT_DXT5:
	case FORMAT_RGTC_R:
	case FORMAT_RGTC_RG: {
		return 4;
	}
	case FORMAT_ETC: {
		return 4;
	}
	case FORMAT_BPTC_RGBA:
	case FORMAT_BPTC_RGBF:
	case FORMAT_BPTC_RGBFU: {
		return 4;
	}
	case FORMAT_ETC2_R11:
	case FORMAT_ETC2_R11S:
	case FORMAT_ETC2_RG11:
	case FORMAT_ETC2_RG11S:
	case FORMAT_ETC2_RGB8:
	case FORMAT_ETC2_RGBA8:
	case FORMAT_ETC2_RGB8A1:
	case FORMAT_ETC2_RA_AS_RG:
	case FORMAT_DXT5_RA_AS_RG: {
		return 4;
	}
	case FORMAT_ASTC_4x4:
	case FORMAT_ASTC_4x4_HDR: {
		return 4;
	}
	case FORMAT_ASTC_6x6:
	case FORMAT_ASTC_6x6_HDR: {
		return 6;
	}
	case FORMAT_ASTC_8x8:
	case FORMAT_ASTC_8x8_HDR: {
		return 8;
	}
	default: {
	}
	}

	return 1;
}

void Image::_get_mipmap_offset_and_size(
	int p_mipmap, int64_t& r_offset, int& r_width, int& r_height) const
{
	int w = width;
	int h = height;
	int64_t ofs = 0;

	int pixel_size = get_format_pixel_size(format);
	int block = get_format_block_size(format);
	int minw, minh;
	get_format_min_pixel_size(format, minw, minh);

	for (int i = 0; i < p_mipmap; i++) {
		int bw = w % block != 0 ? w + (block - w % block) : w;
		int bh = h % block != 0 ? h + (block - h % block) : h;

		int64_t s = bw * bh;

		s *= pixel_size;
		s = get_format_pixels_shifted(format, s);
		ofs += s;
		w = MAX(minw, w >> 1);
		h = MAX(minh, h >> 1);
	}

	r_offset = ofs;
	r_width = w;
	r_height = h;
}

int64_t Image::get_mipmap_offset(int p_mipmap) const
{
	ERR_FAIL_INDEX_V(p_mipmap, get_mipmap_count() + 1, -1);

	int64_t ofs;
	int w, h;
	_get_mipmap_offset_and_size(p_mipmap, ofs, w, h);
	return ofs;
}

void Image::get_mipmap_offset_and_size(int p_mipmap, int64_t& r_ofs, int64_t& r_size) const
{
	int64_t ofs;
	int w, h;
	_get_mipmap_offset_and_size(p_mipmap, ofs, w, h);
	int64_t ofs2;
	_get_mipmap_offset_and_size(p_mipmap + 1, ofs2, w, h);
	r_ofs = ofs;
	r_size = ofs2 - ofs;
}

void Image::get_mipmap_offset_size_and_dimensions(
	int p_mipmap, int64_t& r_ofs, int64_t& r_size, int& p_w, int& p_h) const
{
	int64_t ofs;
	_get_mipmap_offset_and_size(p_mipmap, ofs, p_w, p_h);
	int64_t ofs2;
	int w2, h2;
	_get_mipmap_offset_and_size(p_mipmap + 1, ofs2, w2, h2);
	r_ofs = ofs;
	r_size = ofs2 - ofs;
}

Image::Image3DValidateError Image::validate_3d_image(Image::Format p_format, int p_width,
	int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_images)
{
	int w = p_width;
	int h = p_height;
	int d = p_depth;

	int arr_ofs = 0;

	while (true) {
		for (int i = 0; i < d; i++) {
			int idx = i + arr_ofs;
			if (idx >= p_images.size()) {
				return VALIDATE_3D_ERR_MISSING_IMAGES;
			}
			if (p_images[idx].is_null() || p_images[idx]->is_empty()) {
				return VALIDATE_3D_ERR_IMAGE_EMPTY;
			}
			if (p_images[idx]->get_format() != p_format) {
				return VALIDATE_3D_ERR_IMAGE_FORMAT_MISMATCH;
			}
			if (p_images[idx]->get_width() != w || p_images[idx]->get_height() != h) {
				return VALIDATE_3D_ERR_IMAGE_SIZE_MISMATCH;
			}
			if (p_images[idx]->has_mipmaps()) {
				return VALIDATE_3D_ERR_IMAGE_HAS_MIPMAPS;
			}
		}

		arr_ofs += d;

		if (!p_mipmaps) {
			break;
		}

		if (w == 1 && h == 1 && d == 1) {
			break;
		}

		w = MAX(1, w >> 1);
		h = MAX(1, h >> 1);
		d = MAX(1, d >> 1);
	}

	if (arr_ofs != p_images.size()) {
		return VALIDATE_3D_ERR_EXTRA_IMAGES;
	}

	return VALIDATE_3D_OK;
}

String Image::get_3d_image_validation_error_text(Image3DValidateError p_error)
{
	switch (p_error) {
	case VALIDATE_3D_OK: {
		return "Ok";
	} break;
	case VALIDATE_3D_ERR_IMAGE_EMPTY: {
		return "Empty Image found";
	} break;
	case VALIDATE_3D_ERR_MISSING_IMAGES: {
		return "Missing Images";
	} break;
	case VALIDATE_3D_ERR_EXTRA_IMAGES: {
		return "Too many Images";
	} break;
	case VALIDATE_3D_ERR_IMAGE_SIZE_MISMATCH: {
		return "Image size mismatch";
	} break;
	case VALIDATE_3D_ERR_IMAGE_FORMAT_MISMATCH: {
		return "Image format mismatch";
	} break;
	case VALIDATE_3D_ERR_IMAGE_HAS_MIPMAPS: {
		return "Image has included mipmaps";
	} break;
	}
	return String();
}

int Image::get_width() const { return width; }

int Image::get_height() const { return height; }

Size2i Image::get_size() const { return Size2i(width, height); }

bool Image::has_mipmaps() const { return mipmaps; }

int Image::get_mipmap_count() const
{
	if (mipmaps) {
		return get_image_required_mipmaps(width, height, format);
	}
	else {
		return 0;
	}
}

// Using template generates perfectly optimized code due to constant expression reduction and unused
// variable removal present in all compilers.
template <uint32_t read_bytes, bool read_alpha, uint32_t write_bytes, bool write_alpha,
	bool read_gray, bool write_gray>
static void _convert(int p_width, int p_height, const uint8_t* p_src, uint8_t* p_dst)
{
	constexpr uint32_t max_bytes = MAX(read_bytes, write_bytes);

	for (int y = 0; y < p_height; y++) {
		for (int x = 0; x < p_width; x++) {
			const uint8_t* rofs = &p_src[((y * p_width) + x) * (read_bytes + (read_alpha ? 1 : 0))];
			uint8_t* wofs = &p_dst[((y * p_width) + x) * (write_bytes + (write_alpha ? 1 : 0))];

			uint8_t rgba[4] = {0, 0, 0, 255};

			if constexpr (read_gray) {
				rgba[0] = rofs[0];
				rgba[1] = rofs[0];
				rgba[2] = rofs[0];
			}
			else {
				for (uint32_t i = 0; i < max_bytes; i++) {
					rgba[i] = (i < read_bytes) ? rofs[i] : 0;
				}
			}

			if constexpr (read_alpha || write_alpha) {
				rgba[3] = read_alpha ? rofs[read_bytes] : 255;
			}

			if constexpr (write_gray) {
				// REC.709
				const uint8_t luminance =
					(13938U * rgba[0] + 46869U * rgba[1] + 4729U * rgba[2] + 32768U) >> 16U;
				wofs[0] = luminance;
			}
			else {
				for (uint32_t i = 0; i < write_bytes; i++) {
					wofs[i] = rgba[i];
				}
			}

			if constexpr (write_alpha) {
				wofs[write_bytes] = rgba[3];
			}
		}
	}
}

template <typename T, uint32_t read_channels, uint32_t write_channels, T def_zero, T def_one>
static void _convert_fast(int p_width, int p_height, const T* p_src, T* p_dst)
{
	uint32_t dst_count = 0;
	uint32_t src_count = 0;

	const int resolution = p_width * p_height;

	for (int i = 0; i < resolution; i++) {
		memcpy(
			p_dst + dst_count, p_src + src_count, MIN(read_channels, write_channels) * sizeof(T));

		if constexpr (write_channels > read_channels) {
			const T def_value[4] = {def_zero, def_zero, def_zero, def_one};
			memcpy(p_dst + dst_count + read_channels, &def_value[read_channels],
				(write_channels - read_channels) * sizeof(T));
		}

		dst_count += write_channels;
		src_count += read_channels;
	}
}

static bool _are_formats_compatible(Image::Format p_format0, Image::Format p_format1)
{
	if (p_format0 <= Image::FORMAT_RGBA8 && p_format1 <= Image::FORMAT_RGBA8) {
		return true;
	}
	else if (p_format0 <= Image::FORMAT_RGBAH && p_format0 >= Image::FORMAT_RH &&
			   p_format1 <= Image::FORMAT_RGBAH && p_format1 >= Image::FORMAT_RH) {
		return true;
	}
	else if (p_format0 <= Image::FORMAT_RGBAF && p_format0 >= Image::FORMAT_RF &&
			   p_format1 <= Image::FORMAT_RGBAF && p_format1 >= Image::FORMAT_RF) {
		return true;
	}

	return false;
}

void Image::convert(Format p_new_format)
{
	ERR_FAIL_INDEX_MSG(p_new_format, FORMAT_MAX,
		vformat("The Image format specified (%d) is out of range. See Image's Format enum.",
			p_new_format));
	ERR_FAIL_COND_MSG(
		Image::is_format_compressed(format) || Image::is_format_compressed(p_new_format),
		"Cannot convert to (or from) compressed formats. Use compress() and decompress() instead.");

	if (data.is_empty() || p_new_format == format) {
		return;
	}

	// Includes the main image.
	const int mipmap_count = get_mipmap_count() + 1;

	if (!_are_formats_compatible(format, p_new_format)) {
		// Use put/set pixel which is slower but works with non-byte formats.
		Image new_img(width, height, mipmaps, p_new_format);

		for (int mip = 0; mip < mipmap_count; mip++) {
			int64_t src_mip_ofs, dst_mip_ofs;
			int w, h;
			_get_mipmap_offset_and_size(mip, src_mip_ofs, w, h);
			new_img._get_mipmap_offset_and_size(mip, dst_mip_ofs, w, h);

			uint8_t* dst_mip_ptr = new_img.ptrw() + dst_mip_ofs;
			const uint8_t* src_mip_ptr = ptr() + src_mip_ofs;

			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					uint32_t mip_ofs = y * w + x;
					new_img._set_color_at_ofs(
						dst_mip_ptr, mip_ofs, _get_color_at_ofs(src_mip_ptr, mip_ofs));
				}
			}
		}

		_copy_internals_from(new_img);
		return;
	}

	// Convert the formats in an optimized way by removing/adding color channels if necessary.
	Image new_img(width, height, mipmaps, p_new_format);

	const int conversion_type = format | p_new_format << 8;

	for (int mip = 0; mip < mipmap_count; mip++) {
		int64_t mip_offset = 0;
		int64_t mip_size = 0;
		int mip_width = 0;
		int mip_height = 0;
		get_mipmap_offset_size_and_dimensions(mip, mip_offset, mip_size, mip_width, mip_height);

		const uint8_t* rptr = data.ptr() + mip_offset;
		uint8_t* wptr = new_img.data.ptrw() + new_img.get_mipmap_offset(mip);

		switch (conversion_type) {
		case FORMAT_L8 | (FORMAT_LA8 << 8):
			_convert<1, false, 1, true, true, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_R8 << 8):
			_convert<1, false, 1, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_RG8 << 8):
			_convert<1, false, 2, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_RGB8 << 8):
			_convert<1, false, 3, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_RGBA8 << 8):
			_convert<1, false, 3, true, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_L8 << 8):
			_convert<1, true, 1, false, true, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_R8 << 8):
			_convert<1, true, 1, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_RG8 << 8):
			_convert<1, true, 2, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_RGB8 << 8):
			_convert<1, true, 3, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_RGBA8 << 8):
			_convert<1, true, 3, true, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_L8 << 8):
			_convert<1, false, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_LA8 << 8):
			_convert<1, false, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_RG8 << 8):
			_convert<1, false, 2, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_RGB8 << 8):
			_convert<1, false, 3, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_RGBA8 << 8):
			_convert<1, false, 3, true, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_L8 << 8):
			_convert<2, false, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_LA8 << 8):
			_convert<2, false, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_R8 << 8):
			_convert<2, false, 1, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_RGB8 << 8):
			_convert<2, false, 3, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_RGBA8 << 8):
			_convert<2, false, 3, true, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_L8 << 8):
			_convert<3, false, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_LA8 << 8):
			_convert<3, false, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_R8 << 8):
			_convert<3, false, 1, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_RG8 << 8):
			_convert<3, false, 2, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_RGBA8 << 8):
			_convert<3, false, 3, true, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_L8 << 8):
			_convert<3, true, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_LA8 << 8):
			_convert<3, true, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_R8 << 8):
			_convert<3, true, 1, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_RG8 << 8):
			_convert<3, true, 2, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_RGB8 << 8):
			_convert<3, true, 3, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RH | (FORMAT_RGH << 8):
			_convert_fast<uint16_t, 1, 2, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RH | (FORMAT_RGBH << 8):
			_convert_fast<uint16_t, 1, 3, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RH | (FORMAT_RGBAH << 8):
			_convert_fast<uint16_t, 1, 4, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGH | (FORMAT_RH << 8):
			_convert_fast<uint16_t, 2, 1, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGH | (FORMAT_RGBH << 8):
			_convert_fast<uint16_t, 2, 3, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGH | (FORMAT_RGBAH << 8):
			_convert_fast<uint16_t, 2, 4, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBH | (FORMAT_RH << 8):
			_convert_fast<uint16_t, 3, 1, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBH | (FORMAT_RGH << 8):
			_convert_fast<uint16_t, 3, 2, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBH | (FORMAT_RGBAH << 8):
			_convert_fast<uint16_t, 3, 4, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBAH | (FORMAT_RH << 8):
			_convert_fast<uint16_t, 4, 1, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBAH | (FORMAT_RGH << 8):
			_convert_fast<uint16_t, 4, 2, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBAH | (FORMAT_RGBH << 8):
			_convert_fast<uint16_t, 4, 3, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RF | (FORMAT_RGF << 8):
			_convert_fast<uint32_t, 1, 2, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RF | (FORMAT_RGBF << 8):
			_convert_fast<uint32_t, 1, 3, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RF | (FORMAT_RGBAF << 8):
			_convert_fast<uint32_t, 1, 4, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGF | (FORMAT_RF << 8):
			_convert_fast<uint32_t, 2, 1, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGF | (FORMAT_RGBF << 8):
			_convert_fast<uint32_t, 2, 3, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGF | (FORMAT_RGBAF << 8):
			_convert_fast<uint32_t, 2, 4, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBF | (FORMAT_RF << 8):
			_convert_fast<uint32_t, 3, 1, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBF | (FORMAT_RGF << 8):
			_convert_fast<uint32_t, 3, 2, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBF | (FORMAT_RGBAF << 8):
			_convert_fast<uint32_t, 3, 4, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBAF | (FORMAT_RF << 8):
			_convert_fast<uint32_t, 4, 1, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBAF | (FORMAT_RGF << 8):
			_convert_fast<uint32_t, 4, 2, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBAF | (FORMAT_RGBF << 8):
			_convert_fast<uint32_t, 4, 3, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_R16 | (FORMAT_RG16 << 8):
			_convert_fast<uint16_t, 1, 2, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16 | (FORMAT_RGB16 << 8):
			_convert_fast<uint16_t, 1, 3, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16 | (FORMAT_RGBA16 << 8):
			_convert_fast<uint16_t, 1, 4, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16 | (FORMAT_R16 << 8):
			_convert_fast<uint16_t, 2, 1, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16 | (FORMAT_RGB16 << 8):
			_convert_fast<uint16_t, 2, 3, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16 | (FORMAT_RGBA16 << 8):
			_convert_fast<uint16_t, 2, 4, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16 | (FORMAT_R16 << 8):
			_convert_fast<uint16_t, 3, 1, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16 | (FORMAT_RG16 << 8):
			_convert_fast<uint16_t, 3, 2, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16 | (FORMAT_RGBA16 << 8):
			_convert_fast<uint16_t, 3, 4, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16 | (FORMAT_R16 << 8):
			_convert_fast<uint16_t, 4, 1, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16 | (FORMAT_RG16 << 8):
			_convert_fast<uint16_t, 4, 2, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16 | (FORMAT_RGB16 << 8):
			_convert_fast<uint16_t, 4, 3, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16I | (FORMAT_RG16I << 8):
			_convert_fast<uint16_t, 1, 2, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16I | (FORMAT_RGB16I << 8):
			_convert_fast<uint16_t, 1, 3, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16I | (FORMAT_RGBA16I << 8):
			_convert_fast<uint16_t, 1, 4, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16I | (FORMAT_R16I << 8):
			_convert_fast<uint16_t, 2, 1, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16I | (FORMAT_RGB16I << 8):
			_convert_fast<uint16_t, 2, 3, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16I | (FORMAT_RGBA16I << 8):
			_convert_fast<uint16_t, 2, 4, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16I | (FORMAT_R16I << 8):
			_convert_fast<uint16_t, 3, 1, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16I | (FORMAT_RG16I << 8):
			_convert_fast<uint16_t, 3, 2, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16I | (FORMAT_RGBA16I << 8):
			_convert_fast<uint16_t, 3, 4, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16I | (FORMAT_R16I << 8):
			_convert_fast<uint16_t, 4, 1, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16I | (FORMAT_RG16I << 8):
			_convert_fast<uint16_t, 4, 2, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16I | (FORMAT_RGB16I << 8):
			_convert_fast<uint16_t, 4, 3, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		}
	}

	_copy_internals_from(new_img);
}

Image::Format Image::get_format() const { return format; }

enum ImageScaleType
{
	IMAGE_SCALING_INT,
	IMAGE_SCALING_FLOAT,
};

static double _bicubic_interp_kernel(double p_x)
{
	p_x = Math::abs(p_x);

	double bc = 0;

	if (p_x <= 1) {
		bc = (1.5 * p_x - 2.5) * p_x * p_x + 1;
	}
	else if (p_x < 2) {
		bc = ((-0.5 * p_x + 2.5) * p_x - 4) * p_x + 2;
	}

	return bc;
}

template <int CC, typename T, ImageScaleType TYPE>
static void _scale_cubic(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	// get source image size
	int width = p_src_width;
	int height = p_src_height;
	double xfac = (double)width / p_dst_width;
	double yfac = (double)height / p_dst_height;
	// coordinates of source points and coefficients
	double ox, oy, dx, dy;
	int ox1, oy1, ox2, oy2;
	// destination pixel values
	// width and height decreased by 1
	int ymax = height - 1;
	int xmax = width - 1;
	// temporary pointer

	for (uint32_t y = 0; y < p_dst_height; y++) {
		// Y coordinates
		oy = (double)(y + 0.5) * yfac - 0.5;
		oy1 = (int)oy;
		dy = oy - (double)oy1;

		for (uint32_t x = 0; x < p_dst_width; x++) {
			// X coordinates
			ox = (double)(x + 0.5) * xfac - 0.5;
			ox1 = (int)ox;
			dx = ox - (double)ox1;

			// initial pixel value

			T* __restrict dst = ((T*)p_dst) + (y * p_dst_width + x) * CC;

			double color[CC] = {};

			for (int n = -1; n < 3; n++) {
				// get Y coefficient
				[[maybe_unused]] double k1 = _bicubic_interp_kernel(dy - (double)n);

				oy2 = oy1 + n;
				if (oy2 < 0) {
					oy2 = 0;
				}
				if (oy2 > ymax) {
					oy2 = ymax;
				}

				for (int m = -1; m < 3; m++) {
					// get X coefficient
					[[maybe_unused]] double k2 = k1 * _bicubic_interp_kernel((double)m - dx);

					ox2 = ox1 + m;
					if (ox2 < 0) {
						ox2 = 0;
					}
					if (ox2 > xmax) {
						ox2 = xmax;
					}

					// get pixel of original image
					const T* __restrict p = ((T*)p_src) + (oy2 * p_src_width + ox2) * CC;

					for (int i = 0; i < CC; i++) {
						if constexpr (sizeof(T) == 2 && TYPE == IMAGE_SCALING_FLOAT) { // half float
							color[i] = Math::half_to_float(p[i]);
						}
						else {
							color[i] += p[i] * k2;
						}
					}
				}
			}

			for (int i = 0; i < CC; i++) {
				if constexpr (sizeof(T) == 1) { // byte
					dst[i] = CLAMP(Math::fast_ftoi(color[i]), 0, 255);
				}
				else if constexpr (sizeof(T) == 2) {
					if constexpr (TYPE == IMAGE_SCALING_FLOAT) {
						dst[i] = Math::make_half_float(color[i]); // half float
					}
					else {
						dst[i] = CLAMP(Math::fast_ftoi(color[i]), 0, 65535); // uint16
					}
				}
				else {
					dst[i] = color[i];
				}
			}
		}
	}
}

template <int CC, typename T, ImageScaleType TYPE>
static void _scale_bilinear(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	constexpr uint32_t FRAC_BITS = 8;
	constexpr uint32_t FRAC_LEN = (1 << FRAC_BITS);
	constexpr uint32_t FRAC_HALF = (FRAC_LEN >> 1);
	constexpr uint32_t FRAC_MASK = FRAC_LEN - 1;

	for (uint32_t i = 0; i < p_dst_height; i++) {
		// Add 0.5 in order to interpolate based on pixel center
		uint32_t src_yofs_up_fp = (i + 0.5) * p_src_height * FRAC_LEN / p_dst_height;
		// Calculate nearest src pixel center above current, and truncate to get y index
		uint32_t src_yofs_up =
			src_yofs_up_fp >= FRAC_HALF ? (src_yofs_up_fp - FRAC_HALF) >> FRAC_BITS : 0;
		uint32_t src_yofs_down = (src_yofs_up_fp + FRAC_HALF) >> FRAC_BITS;
		if (src_yofs_down >= p_src_height) {
			src_yofs_down = p_src_height - 1;
		}
		// Calculate distance to pixel center of src_yofs_up
		uint32_t src_yofs_frac = src_yofs_up_fp & FRAC_MASK;
		src_yofs_frac =
			src_yofs_frac >= FRAC_HALF ? src_yofs_frac - FRAC_HALF : src_yofs_frac + FRAC_HALF;

		uint32_t y_ofs_up = src_yofs_up * p_src_width * CC;
		uint32_t y_ofs_down = src_yofs_down * p_src_width * CC;

		for (uint32_t j = 0; j < p_dst_width; j++) {
			uint32_t src_xofs_left_fp = (j + 0.5) * p_src_width * FRAC_LEN / p_dst_width;
			uint32_t src_xofs_left =
				src_xofs_left_fp >= FRAC_HALF ? (src_xofs_left_fp - FRAC_HALF) >> FRAC_BITS : 0;
			uint32_t src_xofs_right = (src_xofs_left_fp + FRAC_HALF) >> FRAC_BITS;
			if (src_xofs_right >= p_src_width) {
				src_xofs_right = p_src_width - 1;
			}
			uint32_t src_xofs_frac = src_xofs_left_fp & FRAC_MASK;
			src_xofs_frac =
				src_xofs_frac >= FRAC_HALF ? src_xofs_frac - FRAC_HALF : src_xofs_frac + FRAC_HALF;

			src_xofs_left *= CC;
			src_xofs_right *= CC;

			for (uint32_t l = 0; l < CC; l++) {
				if constexpr (sizeof(T) == 1) { // uint8
					uint32_t p00 = p_src[y_ofs_up + src_xofs_left + l] << FRAC_BITS;
					uint32_t p10 = p_src[y_ofs_up + src_xofs_right + l] << FRAC_BITS;
					uint32_t p01 = p_src[y_ofs_down + src_xofs_left + l] << FRAC_BITS;
					uint32_t p11 = p_src[y_ofs_down + src_xofs_right + l] << FRAC_BITS;

					uint32_t interp_up = p00 + (((p10 - p00) * src_xofs_frac) >> FRAC_BITS);
					uint32_t interp_down = p01 + (((p11 - p01) * src_xofs_frac) >> FRAC_BITS);
					uint32_t interp =
						interp_up + (((interp_down - interp_up) * src_yofs_frac) >> FRAC_BITS);
					interp >>= FRAC_BITS;
					p_dst[i * p_dst_width * CC + j * CC + l] = uint8_t(interp);
				}
				else if constexpr (sizeof(T) == 2) {
					if constexpr (TYPE == IMAGE_SCALING_FLOAT) { // half float
						float xofs_frac = float(src_xofs_frac) / (1 << FRAC_BITS);
						float yofs_frac = float(src_yofs_frac) / (1 << FRAC_BITS);
						const T* src = ((const T*)p_src);
						T* dst = ((T*)p_dst);

						float p00 = Math::half_to_float(src[y_ofs_up + src_xofs_left + l]);
						float p10 = Math::half_to_float(src[y_ofs_up + src_xofs_right + l]);
						float p01 = Math::half_to_float(src[y_ofs_down + src_xofs_left + l]);
						float p11 = Math::half_to_float(src[y_ofs_down + src_xofs_right + l]);

						float interp_up = p00 + (p10 - p00) * xofs_frac;
						float interp_down = p01 + (p11 - p01) * xofs_frac;
						float interp = interp_up + ((interp_down - interp_up) * yofs_frac);

						dst[i * p_dst_width * CC + j * CC + l] = Math::make_half_float(interp);
					}
					else { // uint16
						float xofs_frac = float(src_xofs_frac) / (1 << FRAC_BITS);
						float yofs_frac = float(src_yofs_frac) / (1 << FRAC_BITS);
						const T* src = ((const T*)p_src);
						T* dst = ((T*)p_dst);

						float p00 = src[y_ofs_up + src_xofs_left + l];
						float p10 = src[y_ofs_up + src_xofs_right + l];
						float p01 = src[y_ofs_down + src_xofs_left + l];
						float p11 = src[y_ofs_down + src_xofs_right + l];

						float interp_up = p00 + (p10 - p00) * xofs_frac;
						float interp_down = p01 + (p11 - p01) * xofs_frac;
						float interp = interp_up + ((interp_down - interp_up) * yofs_frac);

						dst[i * p_dst_width * CC + j * CC + l] = uint16_t(interp);
					}
				}
				else if constexpr (sizeof(T) == 4) { // float

					float xofs_frac = float(src_xofs_frac) / (1 << FRAC_BITS);
					float yofs_frac = float(src_yofs_frac) / (1 << FRAC_BITS);
					const T* src = ((const T*)p_src);
					T* dst = ((T*)p_dst);

					float p00 = src[y_ofs_up + src_xofs_left + l];
					float p10 = src[y_ofs_up + src_xofs_right + l];
					float p01 = src[y_ofs_down + src_xofs_left + l];
					float p11 = src[y_ofs_down + src_xofs_right + l];

					float interp_up = p00 + (p10 - p00) * xofs_frac;
					float interp_down = p01 + (p11 - p01) * xofs_frac;
					float interp = interp_up + ((interp_down - interp_up) * yofs_frac);

					dst[i * p_dst_width * CC + j * CC + l] = interp;
				}
			}
		}
	}
}

template <int CC, typename T>
static void _scale_nearest(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	for (uint32_t i = 0; i < p_dst_height; i++) {
		uint32_t src_yofs = (i + 0.5) * p_src_height / p_dst_height;
		uint32_t y_ofs = src_yofs * p_src_width * CC;

		for (uint32_t j = 0; j < p_dst_width; j++) {
			uint32_t src_xofs = (j + 0.5) * p_src_width / p_dst_width;
			src_xofs *= CC;

			for (uint32_t l = 0; l < CC; l++) {
				const T* src = ((const T*)p_src);
				T* dst = ((T*)p_dst);

				T p = src[y_ofs + src_xofs + l];
				dst[i * p_dst_width * CC + j * CC + l] = p;
			}
		}
	}
}

#define LANCZOS_TYPE 3

static float _lanczos(float p_x)
{
	return Math::abs(p_x) >= LANCZOS_TYPE ? 0 : Math::sincn(p_x) * Math::sincn(p_x / LANCZOS_TYPE);
}

template <int CC, typename T, ImageScaleType TYPE>
static void _scale_lanczos(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	int32_t src_width = p_src_width;
	int32_t src_height = p_src_height;
	int32_t dst_height = p_dst_height;
	int32_t dst_width = p_dst_width;

	uint32_t buffer_size = src_height * dst_width * CC;
	float* buffer = memnew_arr(float, buffer_size); // Store the first pass in a buffer

	{ // FIRST PASS (horizontal)

		float x_scale = float(src_width) / float(dst_width);

		float scale_factor = MAX(x_scale, 1); // A larger kernel is required only when downscaling
		int32_t half_kernel = LANCZOS_TYPE * scale_factor;

		float* kernel = memnew_arr(float, half_kernel * 2);

		for (int32_t buffer_x = 0; buffer_x < dst_width; buffer_x++) {
			// The corresponding point on the source image
			float src_x =
				(buffer_x + 0.5f) * x_scale; // Offset by 0.5 so it uses the pixel's center
			int32_t start_x = MAX(0, int32_t(src_x) - half_kernel + 1);
			int32_t end_x = MIN(src_width - 1, int32_t(src_x) + half_kernel);

			// Create the kernel used by all the pixels of the column
			for (int32_t target_x = start_x; target_x <= end_x; target_x++) {
				kernel[target_x - start_x] = _lanczos((target_x + 0.5f - src_x) / scale_factor);
			}

			for (int32_t buffer_y = 0; buffer_y < src_height; buffer_y++) {
				float pixel[CC] = {0};
				float weight = 0;

				for (int32_t target_x = start_x; target_x <= end_x; target_x++) {
					float lanczos_val = kernel[target_x - start_x];
					weight += lanczos_val;

					const T* __restrict src_data =
						((const T*)p_src) + (buffer_y * src_width + target_x) * CC;

					for (uint32_t i = 0; i < CC; i++) {
						if constexpr (sizeof(T) == 2 && TYPE == IMAGE_SCALING_FLOAT) { // half float
							pixel[i] += Math::half_to_float(src_data[i]) * lanczos_val;
						}
						else {
							pixel[i] += src_data[i] * lanczos_val;
						}
					}
				}

				float* dst_data = ((float*)buffer) + (buffer_y * dst_width + buffer_x) * CC;

				for (uint32_t i = 0; i < CC; i++) {
					dst_data[i] = pixel[i] / weight; // Normalize the sum of all the samples
				}
			}
		}

		memdelete_arr(kernel);
	} // End of first pass

	{ // SECOND PASS (vertical + result)

		float y_scale = float(src_height) / float(dst_height);

		float scale_factor = MAX(y_scale, 1);
		int32_t half_kernel = LANCZOS_TYPE * scale_factor;

		float* kernel = memnew_arr(float, half_kernel * 2);

		for (int32_t dst_y = 0; dst_y < dst_height; dst_y++) {
			float buffer_y = (dst_y + 0.5f) * y_scale;
			int32_t start_y = MAX(0, int32_t(buffer_y) - half_kernel + 1);
			int32_t end_y = MIN(src_height - 1, int32_t(buffer_y) + half_kernel);

			for (int32_t target_y = start_y; target_y <= end_y; target_y++) {
				kernel[target_y - start_y] = _lanczos((target_y + 0.5f - buffer_y) / scale_factor);
			}

			for (int32_t dst_x = 0; dst_x < dst_width; dst_x++) {
				float pixel[CC] = {0};
				float weight = 0;

				for (int32_t target_y = start_y; target_y <= end_y; target_y++) {
					float lanczos_val = kernel[target_y - start_y];
					weight += lanczos_val;

					float* buffer_data = ((float*)buffer) + (target_y * dst_width + dst_x) * CC;

					for (uint32_t i = 0; i < CC; i++) {
						pixel[i] += buffer_data[i] * lanczos_val;
					}
				}

				T* dst_data = ((T*)p_dst) + (dst_y * dst_width + dst_x) * CC;

				for (uint32_t i = 0; i < CC; i++) {
					pixel[i] /= weight;

					if constexpr (sizeof(T) == 1) { // byte
						dst_data[i] = CLAMP(Math::fast_ftoi(pixel[i]), 0, 255);
					}
					else if constexpr (sizeof(T) == 2) {
						if constexpr (TYPE == IMAGE_SCALING_FLOAT) { // half float
							dst_data[i] = Math::make_half_float(pixel[i]);
						}
						else { // uint16
							dst_data[i] = CLAMP(Math::fast_ftoi(pixel[i]), 0, 65535);
						}

					}
					else { // float
						dst_data[i] = pixel[i];
					}
				}
			}
		}

		memdelete_arr(kernel);
	} // End of second pass

	memdelete_arr(buffer);
}

static void _overlay(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst, float p_alpha,
	uint32_t p_width, uint32_t p_height, uint32_t p_pixel_size)
{
	uint16_t alpha = MIN((uint16_t)(p_alpha * 256.0f), 256);

	for (uint32_t i = 0; i < p_width * p_height * p_pixel_size; i++) {
		p_dst[i] = (p_dst[i] * (256 - alpha) + p_src[i] * alpha) >> 8;
	}
}

bool Image::is_size_po2() const
{
	return Math::is_power_of_2(width) && Math::is_power_of_2(height);
}

void Image::resize_to_po2(bool p_square, Interpolation p_interpolation)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot resize in compressed image formats.");

	int w = Math::next_power_of_2((uint32_t)width);
	int h = Math::next_power_of_2((uint32_t)height);
	if (p_square) {
		w = h = MAX(w, h);
	}

	if (w == width && h == height) {
		if (!p_square || w == h) {
			return; // nothing to do
		}
	}

	resize(w, h, p_interpolation);
}

void Image::resize(int p_width, int p_height, Interpolation p_interpolation)
{
	ERR_FAIL_COND_MSG(
		data.is_empty(), "Cannot resize image before creating it, use set_data() first.");
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot resize in compressed image formats.");
	ERR_FAIL_COND_MSG(p_width <= 0, "Image width must be greater than 0.");
	ERR_FAIL_COND_MSG(p_height <= 0, "Image height must be greater than 0.");
	ERR_FAIL_COND_MSG(
		p_width > MAX_WIDTH, vformat("Image width cannot be greater than %d pixels.", MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_height > MAX_HEIGHT,
		vformat("Image height cannot be greater than %d pixels.", MAX_HEIGHT));
	ERR_FAIL_COND_MSG(p_width * p_height > MAX_PIXELS,
		vformat("Too many pixels for image, maximum is %d pixels.", MAX_PIXELS));

	if (p_width == width && p_height == height) {
		return;
	}

	// Convert the image to 'standard' RGB(A) formats that may be resized.
	Format original_format = format;
	if (original_format == FORMAT_RGB565 || original_format == FORMAT_RGBA4444) {
		convert(FORMAT_RGBA8);
	}
	else if (original_format == FORMAT_RGBE9995) {
		convert(FORMAT_RGBH);
	}

	Image dst(p_width, p_height, false, format);

	// Setup mipmap-aware scaling
	bool mipmap_aware =
		p_interpolation == INTERPOLATE_TRILINEAR /* || p_interpolation == INTERPOLATE_TRICUBIC */;

	Image dst2;
	int mip1 = 0;
	int mip2 = 0;
	float mip1_weight = 0;
	if (mipmap_aware) {
		float avg_scale = ((float)p_width / width + (float)p_height / height) * 0.5f;
		if (avg_scale >= 1.0f) {
			mipmap_aware = false;
		}
		else {
			float level = Math::log(1.0f / avg_scale) / Math::log(2.0f);
			mip1 = CLAMP((int)Math::floor(level), 0, get_mipmap_count());
			mip2 = CLAMP((int)Math::ceil(level), 0, get_mipmap_count());
			mip1_weight = 1.0f - (level - mip1);
		}
	}
	bool interpolate_mipmaps = mipmap_aware && mip1 != mip2;
	if (interpolate_mipmaps) {
		dst2.initialize_data(p_width, p_height, false, format);
	}

	bool had_mipmaps = mipmaps;
	if (interpolate_mipmaps && !had_mipmaps) {
		generate_mipmaps();
	}
	// --

	const uint8_t* r = data.ptr();
	const unsigned char* r_ptr = r;

	uint8_t* w = dst.data.ptrw();
	unsigned char* w_ptr = w;

	switch (p_interpolation) {
	case INTERPOLATE_NEAREST: {
		if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
			switch (get_format_pixel_size(format)) {
			case 1:
				_scale_nearest<1, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 2:
				_scale_nearest<2, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 3:
				_scale_nearest<3, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_nearest<4, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
			switch (get_format_pixel_size(format)) {
			case 4:
				_scale_nearest<1, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_nearest<2, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 12:
				_scale_nearest<3, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 16:
				_scale_nearest<4, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}

		}
		else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_nearest<1, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_nearest<2, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_nearest<3, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_nearest<4, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_nearest<1, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_nearest<2, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_nearest<3, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_nearest<4, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}

	} break;
	case INTERPOLATE_BILINEAR:
	case INTERPOLATE_TRILINEAR: {
		for (int i = 0; i < 2; ++i) {
			int src_width;
			int src_height;
			const unsigned char* src_ptr;

			if (!mipmap_aware) {
				if (i == 0) {
					// Standard behavior
					src_width = width;
					src_height = height;
					src_ptr = r_ptr;
				}
				else {
					// No need for a second iteration
					break;
				}
			}
			else {
				if (i == 0) {
					// Read from the first mipmap that will be interpolated
					// (if both levels are the same, we will not interpolate, but at least we'll
					// sample from the right level)
					int64_t offs;
					_get_mipmap_offset_and_size(mip1, offs, src_width, src_height);
					src_ptr = r_ptr + offs;
				}
				else if (!interpolate_mipmaps) {
					// No need generate a second image
					break;
				}
				else {
					// Switch to read from the second mipmap that will be interpolated
					int64_t offs;
					_get_mipmap_offset_and_size(mip2, offs, src_width, src_height);
					src_ptr = r_ptr + offs;
					// Switch to write to the second destination image
					w = dst2.data.ptrw();
					w_ptr = w;
				}
			}

			if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
				switch (get_format_pixel_size(format)) {
				case 1:
					_scale_bilinear<1, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 2:
					_scale_bilinear<2, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 3:
					_scale_bilinear<3, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 4:
					_scale_bilinear<4, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
			else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
				switch (get_format_pixel_size(format)) {
				case 4:
					_scale_bilinear<1, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 8:
					_scale_bilinear<2, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 12:
					_scale_bilinear<3, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 16:
					_scale_bilinear<4, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
			else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
				switch (get_format_pixel_size(format)) {
				case 2:
					_scale_bilinear<1, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 4:
					_scale_bilinear<2, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 6:
					_scale_bilinear<3, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 8:
					_scale_bilinear<4, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
			else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
				switch (get_format_pixel_size(format)) {
				case 2:
					_scale_bilinear<1, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 4:
					_scale_bilinear<2, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 6:
					_scale_bilinear<3, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 8:
					_scale_bilinear<4, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
		}

		if (interpolate_mipmaps) {
			// Switch to read again from the first scaled mipmap to overlay it over the second
			r = dst.data.ptr();
			_overlay(r, w, mip1_weight, p_width, p_height, get_format_pixel_size(format));
		}

	} break;
	case INTERPOLATE_CUBIC: {
		if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
			switch (get_format_pixel_size(format)) {
			case 1:
				_scale_cubic<1, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 2:
				_scale_cubic<2, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 3:
				_scale_cubic<3, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_cubic<4, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
			switch (get_format_pixel_size(format)) {
			case 4:
				_scale_cubic<1, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_cubic<2, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 12:
				_scale_cubic<3, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 16:
				_scale_cubic<4, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_cubic<1, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_cubic<2, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_cubic<3, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_cubic<4, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_cubic<1, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_cubic<2, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_cubic<3, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_cubic<4, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
	} break;
	case INTERPOLATE_LANCZOS: {
		if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
			switch (get_format_pixel_size(format)) {
			case 1:
				_scale_lanczos<1, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 2:
				_scale_lanczos<2, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 3:
				_scale_lanczos<3, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_lanczos<4, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
			switch (get_format_pixel_size(format)) {
			case 4:
				_scale_lanczos<1, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_lanczos<2, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 12:
				_scale_lanczos<3, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 16:
				_scale_lanczos<4, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_lanczos<1, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_lanczos<2, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_lanczos<3, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_lanczos<4, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_lanczos<1, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_lanczos<2, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_lanczos<3, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_lanczos<4, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
	} break;
	}

	if (interpolate_mipmaps) {
		dst._copy_internals_from(dst2);
	}

	if (had_mipmaps) {
		dst.generate_mipmaps();
	}

	_copy_internals_from(dst);

	// Reconvert the image to its original format.
	if (original_format != format) {
		convert(original_format);
	}
}

void Image::crop_from_point(int p_x, int p_y, int p_width, int p_height)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot crop in compressed image formats.");
	ERR_FAIL_COND_MSG(p_x < 0, "Start x position cannot be smaller than 0.");
	ERR_FAIL_COND_MSG(p_y < 0, "Start y position cannot be smaller than 0.");
	ERR_FAIL_COND_MSG(p_width <= 0, "Width of image must be greater than 0.");
	ERR_FAIL_COND_MSG(p_height <= 0, "Height of image must be greater than 0.");
	ERR_FAIL_COND_MSG(
		p_x + p_width > MAX_WIDTH, vformat("End x position cannot be greater than %d.", MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_y + p_height > MAX_HEIGHT,
		vformat("End y position cannot be greater than %d.", MAX_HEIGHT));

	/* to save memory, cropping should be done in-place, however, since this function
	   will most likely either not be used much, or in critical areas, for now it won't, because
	   it's a waste of time. */

	if (p_width == width && p_height == height && p_x == 0 && p_y == 0) {
		return;
	}

	uint8_t pdata[16]; // largest is 16
	uint32_t pixel_size = get_format_pixel_size(format);

	Image dst(p_width, p_height, false, format);

	{
		const uint8_t* r = data.ptr();
		uint8_t* w = dst.data.ptrw();

		int m_h = p_y + p_height;
		int m_w = p_x + p_width;
		for (int y = p_y; y < m_h; y++) {
			for (int x = p_x; x < m_w; x++) {
				if ((x >= width || y >= height)) {
					for (uint32_t i = 0; i < pixel_size; i++) {
						pdata[i] = 0;
					}
				}
				else {
					_get_pixelb(x, y, pixel_size, r, pdata);
				}

				dst._put_pixelb(x - p_x, y - p_y, pixel_size, w, pdata);
			}
		}
	}

	if (has_mipmaps()) {
		dst.generate_mipmaps();
	}
	_copy_internals_from(dst);
}

void Image::crop(int p_width, int p_height) { crop_from_point(0, 0, p_width, p_height); }

void Image::rotate_90(ClockDirection p_direction)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot rotate in compressed image formats.");
	ERR_FAIL_COND_MSG(width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", width));
	ERR_FAIL_COND_MSG(height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", height));

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	// In-place 90 degrees rotation by following the permutation cycles.
	{
		// Explanation by example (clockwise):
		//
		//  abc      da
		//  def  ->  eb
		//           fc
		//
		// In memory:
		//  012345    012345
		//  abcdef -> daebfc
		//
		// Permutation cycles:
		//  (0  --a-->  1  --b-->  3  --d-->  0)
		//  (2  --c-->  5  --f-->  4  --e-->  2)
		//
		// Applying cycles (backwards):
		//  0->s  s=a (store)
		//  3->0  abcdef -> dbcdef
		//  1->3  dbcdef -> dbcbef
		//  s->1  dbcbef -> dacbef
		//
		//  2->s  s=c
		//  4->2  dacbef -> daebef
		//  5->4  daebef -> daebff
		//  s->5  daebff -> daebfc

		const int w = width;
		const int h = height;
		const int size = w * h;

		uint8_t* data_ptr = data.ptrw();
		uint32_t pixel_size = get_format_pixel_size(format);

		uint8_t single_pixel_buffer[16];

#define PREV_INDEX_IN_CYCLE(index)                                                                 \
	(p_direction == CLOCKWISE) ? ((h - 1 - (index % h)) * w + (index / h))                         \
							   : ((index % h) * w + (w - 1 - (index / h)))

		if (w == h) { // Square case, 4-length cycles only (plus irrelevant thus skipped 1-length
					  // cycle in the middle for odd-sized squares).
			for (int y = 0; y < h / 2; y++) {
				for (int x = 0; x < (w + 1) / 2; x++) {
					int current = y * w + x;
					memcpy(single_pixel_buffer, data_ptr + current * pixel_size, pixel_size);
					for (int i = 0; i < 3; i++) {
						int prev = PREV_INDEX_IN_CYCLE(current);
						memcpy(data_ptr + current * pixel_size, data_ptr + prev * pixel_size,
							pixel_size);
						current = prev;
					}
					memcpy(data_ptr + current * pixel_size, single_pixel_buffer, pixel_size);
				}
			}
		}
		else { // Rectangular case (w != h), kinda unpredictable cycles.
			int permuted_pixels_count = 0;

			for (int i = 0; i < size; i++) {
				int prev = PREV_INDEX_IN_CYCLE(i);
				if (prev == i) {
					// 1-length cycle, pixel remains at the same index.
					permuted_pixels_count++;
					continue;
				}

				// Check whether we already processed this cycle.
				// We iterate over it and if we'll find an index smaller than `i` then we already
				// processed this cycle because we always start at the smallest index in the cycle.
				// TODO: Improve this naive approach, can be done better.
				while (prev > i) {
					prev = PREV_INDEX_IN_CYCLE(prev);
				}
				if (prev < i) {
					continue;
				}

				// Save the in-cycle pixel with the smallest index (`i`).
				memcpy(single_pixel_buffer, data_ptr + i * pixel_size, pixel_size);

				// Overwrite pixels one by one by the preceding pixel in the cycle.
				int current = i;
				prev = PREV_INDEX_IN_CYCLE(current);
				while (prev != i) {
					memcpy(
						data_ptr + current * pixel_size, data_ptr + prev * pixel_size, pixel_size);
					permuted_pixels_count++;

					current = prev;
					prev = PREV_INDEX_IN_CYCLE(current);
				};

				// Overwrite the remaining pixel in the cycle by the saved pixel with the smallest
				// index.
				memcpy(data_ptr + current * pixel_size, single_pixel_buffer, pixel_size);
				permuted_pixels_count++;

				if (permuted_pixels_count == size) {
					break;
				}
			}

			width = h;
			height = w;
		}

#undef PREV_INDEX_IN_CYCLE
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

void Image::rotate_180()
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot rotate in compressed image formats.");
	ERR_FAIL_COND_MSG(width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", width));
	ERR_FAIL_COND_MSG(height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", height));

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	{
		uint8_t* data_ptr = data.ptrw();
		uint32_t pixel_size = get_format_pixel_size(format);

		uint8_t single_pixel_buffer[16];

		uint8_t* from_begin_ptr = data_ptr;
		uint8_t* from_end_ptr = data_ptr + (width * height - 1) * pixel_size;

		while (from_begin_ptr < from_end_ptr) {
			memcpy(single_pixel_buffer, from_begin_ptr, pixel_size);
			memcpy(from_begin_ptr, from_end_ptr, pixel_size);
			memcpy(from_end_ptr, single_pixel_buffer, pixel_size);

			from_begin_ptr += pixel_size;
			from_end_ptr -= pixel_size;
		}
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

void Image::flip_y()
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot flip_y in compressed image formats.");

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	{
		uint8_t* w = data.ptrw();
		uint8_t up[16];
		uint8_t down[16];
		uint32_t pixel_size = get_format_pixel_size(format);

		for (int y = 0; y < height / 2; y++) {
			for (int x = 0; x < width; x++) {
				_get_pixelb(x, y, pixel_size, w, up);
				_get_pixelb(x, height - y - 1, pixel_size, w, down);

				_put_pixelb(x, height - y - 1, pixel_size, w, up);
				_put_pixelb(x, y, pixel_size, w, down);
			}
		}
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

void Image::flip_x()
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot flip_x in compressed image formats.");

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	{
		uint8_t* w = data.ptrw();
		uint8_t up[16];
		uint8_t down[16];
		uint32_t pixel_size = get_format_pixel_size(format);

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width / 2; x++) {
				_get_pixelb(x, y, pixel_size, w, up);
				_get_pixelb(width - x - 1, y, pixel_size, w, down);

				_put_pixelb(width - x - 1, y, pixel_size, w, up);
				_put_pixelb(x, y, pixel_size, w, down);
			}
		}
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

// Get mipmap size and offset.
int64_t Image::_get_dst_image_size(int p_width, int p_height, Format p_format, int& r_mipmaps,
	int p_mipmaps, int* r_mm_width, int* r_mm_height)
{
	// Data offset in mipmaps (including the original texture).
	int64_t size = 0;

	int w = p_width;
	int h = p_height;

	// Current mipmap index in the loop below. p_mipmaps is the target mipmap index.
	// In this function, mipmap 0 represents the first mipmap instead of the original texture.
	int mm = 0;

	int pixsize = get_format_pixel_size(p_format);
	int block = get_format_block_size(p_format);

	// Technically, you can still compress up to 1 px no matter the format, so commenting this.
	// int minw, minh;
	// get_format_min_pixel_size(p_format, minw, minh);
	int minw = 1, minh = 1;

	while (true) {
		int bw = w % block != 0 ? w + (block - w % block) : w;
		int bh = h % block != 0 ? h + (block - h % block
) : h;

		int64_t s = bw * bh;

		s *= pixsize;
		s = get_format_pixels_shifted(p_format, s);

		size += s;

		if (p_mipmaps >= 0) {
			w = MAX(minw, w >> 1);
			h = MAX(minh, h >> 1);
		}
		else {
			if (w == minw && h == minh) {
				break;
			}
			w = MAX(minw, w >> 1);
			h = MAX(minh, h >> 1);
		}

		// Set mipmap size.
		if (r_mm_width) {
			*r_mm_width = w;
		}
		if (r_mm_height) {
			*r_mm_height = h;
		}

		// Reach target mipmap.
		if (p_mipmaps >= 0 && mm == p_mipmaps) {
			break;
		}

		mm++;
	}

	r_mipmaps = mm;
	return size;
}

template <typename Component, int CC, bool renormalize,
	void (*average_func)(
		Component&, const Component&, const Component&, const Component&, const Component&),
	void (*renormalize_func)(Component*)>
static void _generate_po2_mipmap(
	const Component* p_src, Component* p_dst, uint32_t p_width, uint32_t p_height)
{
	// Fast power of 2 mipmap generation.
	uint32_t dst_w = MAX(p_width >> 1, 1u);
	uint32_t dst_h = MAX(p_height >> 1, 1u);

	int right_step = (p_width == 1) ? 0 : CC;
	int down_step = (p_height == 1) ? 0 : (p_width * CC);

	for (uint32_t i = 0; i < dst_h; i++) {
		const Component* rup_ptr = &p_src[i * 2 * down_step];
		const Component* rdown_ptr = rup_ptr + down_step;
		Component* dst_ptr = &p_dst[i * dst_w * CC];
		uint32_t count = dst_w;

		while (count) {
			count--;
			for (int j = 0; j < CC; j++) {
				average_func(dst_ptr[j], rup_ptr[j], rup_ptr[j + right_step], rdown_ptr[j],
					rdown_ptr[j + right_step]);
			}

			if constexpr (renormalize) {
				renormalize_func(dst_ptr);
			}

			dst_ptr += CC;
			rup_ptr += right_step * 2;
			rdown_ptr += right_step * 2;
		}
	}
}

void Image::_generate_mipmap_from_format(Image::Format p_format, const uint8_t* p_src,
	uint8_t* p_dst, uint32_t p_width, uint32_t p_height, bool p_renormalize)
{
	const float* src_float = reinterpret_cast<const float*>(p_src);
	float* dst_float = reinterpret_cast<float*>(p_dst);

	const uint16_t* src_u16 = reinterpret_cast<const uint16_t*>(p_src);
	uint16_t* dst_u16 = reinterpret_cast<uint16_t*>(p_dst);

	const uint32_t* src_u32 = reinterpret_cast<const uint32_t*>(p_src);
	uint32_t* dst_u32 = reinterpret_cast<uint32_t*>(p_dst);

	switch (p_format) {
	case Image::FORMAT_L8:
	case Image::FORMAT_R8:
		_generate_po2_mipmap<uint8_t, 1, false, Image::average_4_uint8, Image::renormalize_uint8>(
			p_src, p_dst, p_width, p_height);
		break;
	case Image::FORMAT_LA8:
		_generate_po2_mipmap<uint8_t, 2, false, Image::average_4_uint8, Image::renormalize_uint8>(
			p_src, p_dst, p_width, p_height);
		break;
	case Image::FORMAT_RG8:
		_generate_po2_mipmap<uint8_t, 2, false, Image::average_4_uint8, Image::renormalize_uint8>(
			p_src, p_dst, p_width, p_height);
		break;
	case Image::FORMAT_RGB8: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint8_t, 3, true, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint8_t, 3, false, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBA8: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint8_t, 4, true, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint8_t, 4, false, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBA4444: {
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_rgba4444, nullptr>(
			src_u16, dst_u16, p_width, p_height);
	} break;
	case Image::FORMAT_RGB565: {
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_rgb565, nullptr>(
			src_u16, dst_u16, p_width, p_height);
	} break;
	case Image::FORMAT_RF:
		_generate_po2_mipmap<float, 1, false, Image::average_4_float, Image::renormalize_float>(
			src_float, dst_float, p_width, p_height);
		break;
	case Image::FORMAT_RGF:
		_generate_po2_mipmap<float, 2, false, Image::average_4_float, Image::renormalize_float>(
			src_float, dst_float, p_width, p_height);
		break;
	case Image::FORMAT_RGBF: {
		if (p_renormalize) {
			_generate_po2_mipmap<float, 3, true, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<float, 3, false, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBAF: {
		if (p_renormalize) {
			_generate_po2_mipmap<float, 4, true, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<float, 4, false, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RH:
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_half, Image::renormalize_half>(
			src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RGH:
		_generate_po2_mipmap<uint16_t, 2, false, Image::average_4_half, Image::renormalize_half>(
			src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RGBH: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 3, true, Image::average_4_half, Image::renormalize_half>(
				src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 3, false, Image::average_4_half,
				Image::renormalize_half>(src_u16, dst_u16, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBAH: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 4, true, Image::average_4_half, Image::renormalize_half>(
				src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 4, false, Image::average_4_half,
				Image::renormalize_half>(src_u16, dst_u16, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBE9995:
		_generate_po2_mipmap<uint32_t, 1, false, Image::average_4_rgbe9995, nullptr>(
			src_u32, dst_u32, p_width, p_height);
		break;
	case Image::FORMAT_R16:
	case Image::FORMAT_R16I:
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_uint16,
			Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RG16:
	case Image::FORMAT_RG16I:
		_generate_po2_mipmap<uint16_t, 2, false, Image::average_4_uint16,
			Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RGB16:
	case Image::FORMAT_RGB16I: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 3, true, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 3, false, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBA16:
	case Image::FORMAT_RGBA16I: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 4, true, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 4, false, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
	} break;

	default:
		return;
	}
}

void Image::shrink_x2()
{
	ERR_FAIL_COND(data.is_empty());
	Vector<uint8_t> new_data;

	if (mipmaps) {
		// Just use the lower mipmap as base and copy all.
		int64_t ofs = get_mipmap_offset(1);
		int64_t new_size = data.size() - ofs;

		new_data.resize(new_size);
		ERR_FAIL_COND(new_data.is_empty());

		memcpy(new_data.ptrw(), data.ptr() + ofs, new_size);
	}
	else {
		// Generate a mipmap and replace the original.
		ERR_FAIL_COND(is_compressed());

		new_data.resize((width / 2) * (height / 2) * get_format_pixel_size(format));
		ERR_FAIL_COND(data.is_empty() || new_data.is_empty());

		_generate_mipmap_from_format(format, data.ptr(), new_data.ptrw(), width, height, false);
	}

	width = MAX(width / 2, 1);
	height = MAX(height / 2, 1);
	data = new_data;
}

void Image::normalize()
{
	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Color c = get_pixel(x, y);
			Vector3 v(c.r * 2.0 - 1.0, c.g * 2.0 - 1.0, c.b * 2.0 - 1.0);
			v.normalize();
			c.r = v.x * 0.5 + 0.5;
			c.g = v.y * 0.5 + 0.5;
			c.b = v.z * 0.5 + 0.5;
			set_pixel(x, y, c);
		}
	}

	if (used_mipmaps) {
		generate_mipmaps(true);
	}
}

Error Image::generate_mipmaps(bool p_renormalize)
{
	ERR_FAIL_COND_V_MSG(
		is_compressed(), ERR_UNAVAILABLE, "Cannot generate mipmaps from compressed image formats.");
	ERR_FAIL_COND_V_MSG(width == 0 || height == 0, ERR_UNCONFIGURED,
		"Cannot generate mipmaps with width or height equal to 0.");

	int gen_mipmap_count;

	int64_t size = _get_dst_image_size(width, height, format, gen_mipmap_count);
	data.resize(size);
	uint8_t* wp = data.ptrw();

	int prev_ofs = 0;
	int prev_h = height;
	int prev_w = width;

	for (int i = 1; i <= gen_mipmap_count; i++) {
		int64_t ofs;
		int w, h;
		_get_mipmap_offset_and_size(i, ofs, w, h);

		_generate_mipmap_from_format(
			format, wp + prev_ofs, wp + ofs, prev_w, prev_h, p_renormalize);

		prev_ofs = ofs;
		prev_w = w;
		prev_h = h;
	}

	mipmaps = true;

	return OK;
}

void Image::clear_mipmaps()
{
	if (!mipmaps) {
		return;
	}

	if (is_empty()) {
		return;
	}

	int64_t ofs;
	int w, h;
	_get_mipmap_offset_and_size(1, ofs, w, h);
	data.resize(ofs);

	mipmaps = false;
}

bool Image::is_empty() const { return (data.is_empty()); }

const Vector<uint8_t>& Image::get_data() const { return data; }

Ref<Image> Image::create_empty(int p_width, int p_height, bool p_use_mipmaps, Format p_format)
{
	Ref<Image> image;
	image.instantiate();
	image->initialize_data(p_width, p_height, p_use_mipmaps, p_format);
	return image;
}

Ref<Image> Image::create_from_data(
	int p_width, int p_height, bool p_use_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	Ref<Image> image;
	image.instantiate();
	image->initialize_data(p_width, p_height, p_use_mipmaps, p_format, p_data);
	return image;
}

void Image::set_data(
	int p_width, int p_height, bool p_use_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	initialize_data(p_width, p_height, p_use_mipmaps, p_format, p_data);
}

void Image::initialize_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format)
{
	ERR_FAIL_COND_MSG(p_width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", p_width));
	ERR_FAIL_COND_MSG(p_height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", p_height));
	ERR_FAIL_COND_MSG(p_width > MAX_WIDTH,
		vformat("The Image width specified (%d pixels) cannot be greater than %d pixels.", p_width,
			MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_height > MAX_HEIGHT,
		vformat("The Image height specified (%d pixels) cannot be greater than %d pixels.",
			p_height, MAX_HEIGHT));
	ERR_FAIL_COND_MSG(p_width * p_height > MAX_PIXELS,
		vformat("Too many pixels for Image. Maximum is %dx%d = %d pixels.", MAX_WIDTH, MAX_HEIGHT,
			MAX_PIXELS));
	ERR_FAIL_INDEX_MSG(p_format, FORMAT_MAX,
		vformat(
			"The Image format specified (%d) is out of range. See Image's Format enum.", p_format));

	int mm = 0;
	int64_t size = _get_dst_image_size(p_width, p_height, p_format, mm, p_use_mipmaps ? -1 : 0);
	data.resize(size);

	{
		uint8_t* w = data.ptrw();
		memset(w, 0, size);
	}

	width = p_width;
	height = p_height;
	mipmaps = p_use_mipmaps;
	format = p_format;
}

void Image::initialize_data(
	int p_width, int p_height, bool p_use_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND_MSG(p_width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", p_width));
	ERR_FAIL_COND_MSG(p_height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", p_height));
	ERR_FAIL_COND_MSG(p_width > MAX_WIDTH,
		vformat("The Image width specified (%d pixels) cannot be greater than %d pixels.", p_width,
			MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_height > MAX_HEIGHT,
		vformat("The Image height specified (%d pixels) cannot be greater than %d pixels.",
			p_height, MAX_HEIGHT));
	ERR_FAIL_COND_MSG(p_width * p_height > MAX_PIXELS,
		vformat("Too many pixels for Image. Maximum is %dx%d = %d pixels.", MAX_WIDTH, MAX_HEIGHT,
			MAX_PIXELS));
	ERR_FAIL_INDEX_MSG(p_format, FORMAT_MAX,
		vformat(
			"The Image format specified (%d) is out of range. See Image's Format enum.", p_format));

	int mm;
	int64_t size = _get_dst_image_size(p_width, p_height, p_format, mm, p_use_mipmaps ? -1 : 0);

	if (unlikely(p_data.size() != size)) {
		String description_mipmaps = get_format_name(p_format) + " ";
		if (p_use_mipmaps) {
			const int num_mipmaps = get_image_required_mipmaps(p_width, p_height, p_format);
			if (num_mipmaps != 1) {
				description_mipmaps += vformat("with %d mipmaps", num_mipmaps);
			}
			else {
				description_mipmaps += "with 1 mipmap";
			}
		}
		else {
			description_mipmaps += "without mipmaps";
		}
		const String description = vformat("%dx%dx%d (%s)", p_width, p_height,
			get_format_pixel_size(p_format), description_mipmaps);
		ERR_FAIL_MSG(vformat("Expected Image data size of %s = %d bytes, got %d bytes instead.",
			description, size, p_data.size()));
	}

	height = p_height;
	width = p_width;
	format = p_format;
	data = p_data;

	mipmaps = p_use_mipmaps;
}

void Image::initialize_data(const char** p_xpm)
{
	int size_width = 0;
	int size_height = 0;
	int pixelchars = 0;
	mipmaps = false;
	bool has_alpha = false;

	enum Status
	{
		READING_HEADER,
		READING_COLORS,
		READING_PIXELS,
		DONE
	};

	Status status = READING_HEADER;
	int line = 0;

	HashMap<String, Color> colormap;
	int colormap_size = 0;
	uint32_t pixel_size = 0;
	uint8_t* data_write = nullptr;

	while (status != DONE) {
		const char* line_ptr = p_xpm[line];

		switch (status) {
		case READING_HEADER: {
			String line_str = line_ptr;
			line_str = line_str.replace_char('\t', ' ');

			size_width = line_str.get_slicec(' ', 0).to_int();
			size_height = line_str.get_slicec(' ', 1).to_int();
			colormap_size = line_str.get_slicec(' ', 2).to_int();
			pixelchars = line_str.get_slicec(' ', 3).to_int();
			ERR_FAIL_COND(colormap_size > 32766);
			ERR_FAIL_COND(pixelchars > 5);
			ERR_FAIL_COND(size_width > 32767);
			ERR_FAIL_COND(size_height > 32767);
			status = READING_COLORS;
		} break;
		case READING_COLORS: {
			String colorstring;
			for (int i = 0; i < pixelchars; i++) {
				colorstring += *line_ptr;
				line_ptr++;
			}
			// skip spaces
			while (*line_ptr == ' ' || *line_ptr == '\t' || *line_ptr == 0) {
				if (*line_ptr == 0) {
					break;
				}
				line_ptr++;
			}
			if (*line_ptr == 'c') {
				line_ptr++;
				while (*line_ptr == ' ' || *line_ptr == '\t' || *line_ptr == 0) {
					if (*line_ptr == 0) {
						break;
					}
					line_ptr++;
				}

				if (*line_ptr == '#') {
					line_ptr++;
					uint8_t col_r = 0;
					uint8_t col_g = 0;
					uint8_t col_b = 0;
					// uint8_t col_a=255;

					for (int i = 0; i < 6; i++) {
						char v = line_ptr[i];

						if (is_digit(v)) {
							v -= '0';
						}
						else if (v >= 'A' && v <= 'F') {
							v = (v - 'A') + 10;
						}
						else if (v >= 'a' && v <= 'f') {
							v = (v - 'a') + 10;
						}
						else {
							break;
						}

						switch (i) {
						case 0:
							col_r = v << 4;
							break;
						case 1:
							col_r |= v;
							break;
						case 2:
							col_g = v << 4;
							break;
						case 3:
							col_g |= v;
							break;
						case 4:
							col_b = v << 4;
							break;
						case 5:
							col_b |= v;
							break;
						}
					}

					// magenta mask
					if (col_r == 255 && col_g == 0 && col_b == 255) {
						colormap[colorstring] = Color(0, 0, 0, 0);
						has_alpha = true;
					}
					else {
						colormap[colorstring] =
							Color(col_r / 255.0, col_g / 255.0, col_b / 255.0, 1.0);
					}
				}
			}
			if (line == colormap_size) {
				status = READING_PIXELS;
				initialize_data(
					size_width, size_height, false, has_alpha ? FORMAT_RGBA8 : FORMAT_RGB8);
				data_write = data.ptrw();
				pixel_size = has_alpha ? 4 : 3;
			}
		} break;
		case READING_PIXELS: {
			int y = line - colormap_size - 1;
			for (int x = 0; x < size_width; x++) {
				char pixelstr[6] = {0, 0, 0, 0, 0, 0};
				for (int i = 0; i < pixelchars; i++) {
					pixelstr[i] = line_ptr[x * pixelchars + i];
				}

				Color* colorptr = colormap.getptr(pixelstr);
				ERR_FAIL_NULL(colorptr);
				uint8_t pixel[4];
				for (uint32_t i = 0; i < pixel_size; i++) {
					pixel[i] = CLAMP((*colorptr)[i] * 255, 0, 255);
				}
				_put_pixelb(x, y, pixel_size, data_write, pixel);
			}

			if (y == (size_height - 1)) {
				status = DONE;
			}
		} break;
		default: {
		}
		}

		line++;
	}
}

#define DETECT_ALPHA_MAX_THRESHOLD 254
#define DETECT_ALPHA_MIN_THRESHOLD 2

#define DETECT_ALPHA(m_value)                                                                      \
	{                                                                                              \
		uint8_t value = m_value;                                                                   \
		if (value < DETECT_ALPHA_MIN_THRESHOLD)                                                    \
			bit = true;                                                                            \
		else if (value < DETECT_ALPHA_MAX_THRESHOLD) {                                             \
			detected = true;                                                                       \
			break;                                                                                 \
		}                                                                                          \
	}

#define DETECT_NON_ALPHA(m_value)                                                                  \
	{                                                                                              \
		uint8_t value = m_value;                                                                   \
		if (value > 0) {                                                                           \
			detected = true;                                                                       \
			break;                                                                                 \
		}                                                                                          \
	}

bool Image::is_invisible() const
{
	int w, h;
	int64_t len;
	_get_mipmap_offset_and_size(1, len, w, h);

	if (len == 0) {
		return true;
	}

	switch (format) {
	case FORMAT_LA8: {
		const int pixel_count = len / 2;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i] & 0xFF00) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBA8: {
		const int pixel_count = len / 4;
		const uint32_t* pixeldata = reinterpret_cast<const uint32_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i] & 0xFF000000) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBA4444: {
		const int pixel_count = len / 2;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i] & 0x000F) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBAH: {
		// The alpha mask accounts for the sign bit.
		const int pixel_count = len / 8;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i * 4 + 3] & 0x7FFF) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBAF: {
		// The alpha mask accounts for the sign bit.
		const int pixel_count = len / 16;
		const uint32_t* pixeldata = reinterpret_cast<const uint32_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i * 4 + 3] & 0x7FFFFFFF) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBA16:
	case FORMAT_RGBA16I: {
		const int pixel_count = len / 8;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if (pixeldata[i * 4 + 3] != 0) {
				return false;
			}
		}
	} break;
	default: {
		// Formats that are compressed or don't support alpha channels are presumed to be visible.
		return false;
	}
	}

	// Every pixel has been checked, the image is invisible.
	return true;
}

Image::AlphaMode Image::detect_alpha() const
{
	int64_t len = data.size();

	if (len == 0) {
		return ALPHA_NONE;
	}

	int w, h;
	_get_mipmap_offset_and_size(1, len, w, h);

	const uint8_t* r = data.ptr();
	const unsigned char* data_ptr = r;

	bool bit = false;
	bool detected = false;

	switch (format) {
	case FORMAT_LA8: {
		for (int i = 0; i < (len >> 1); i++) {
			DETECT_ALPHA(data_ptr[(i << 1) + 1]);
		}

	} break;
	case FORMAT_RGBA8: {
		for (int i = 0; i < (len >> 2); i++) {
			DETECT_ALPHA(data_ptr[(i << 2) + 3])
		}

	} break;
	case FORMAT_DXT3:
	case FORMAT_DXT5: {
		detected = true;
	} break;
	default: {
	}
	}

	if (detected) {
		return ALPHA_BLEND;
	}
	else if (bit) {
		return ALPHA_BIT;
	}
	else {
		return ALPHA_NONE;
	}
}

Error Image::load(const String& p_path)
{
	String path = ResourceUID::ensure_path(p_path);
#ifdef DEBUG_ENABLED
	if (path.begins_with("res://") && ResourceLoader::exists(path)) {
		WARN_PRINT(vformat(
			"Loaded resource as image file, this will not work on export: '%s'. Instead, import "
			"the image file as an Image resource and load it normally as a resource.",
			path));
	}
#endif
	return ImageLoader::load_image(path, this);
}

Ref<Image> Image::load_from_file(const String& p_path)
{
	String path = ResourceUID::ensure_path(p_path);
#ifdef DEBUG_ENABLED
	if (path.begins_with("res://") && ResourceLoader::exists(path)) {
		WARN_PRINT(vformat(
			"Loaded resource as image file, this will not work on export: '%s'. Instead, import "
			"the image file as an Image resource and load it normally as a resource.",
			path));
	}
#endif
	Ref<Image> image;
	image.instantiate();
	Error err = ImageLoader::load_image(path, image);
	if (err != OK) {
		ERR_FAIL_V_MSG(Ref<Image>(), vformat("Failed to load image. Error %d", err));
	}
	return image;
}

Error Image::save_png(const String& p_path) const
{
	if (save_png_func == nullptr) {
		return ERR_UNAVAILABLE;
	}

	return save_png_func(p_path, Ref<Image>((Image*)this));
}

Error Image::save_jpg(const String& p_path, float p_quality) const
{
	if (save_jpg_func == nullptr) {
		return ERR_UNAVAILABLE;
	}

	return save_jpg_func(p_path, Ref<Image>((Image*)this), p_quality);
}

Vector<uint8_t> Image::save_png_to_buffer() const
{
	if (save_png_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}

	return save_png_buffer_func(Ref<Image>((Image*)this));
}

Vector<uint8_t> Image::save_jpg_to_buffer(float p_quality) const
{
	if (save_jpg_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}

	return save_jpg_buffer_func(Ref<Image>((Image*)this), p_quality);
}

Error Image::save_exr(
	const String& p_path, bool p_grayscale, bool p_color_image, float p_max_value) const
{
	if (save_exr_func == nullptr) {
		return ERR_UNAVAILABLE;
	}
	return save_exr_func(p_path, Ref<Image>((Image*)this), p_grayscale, p_color_image, p_max_value);
}

Vector<uint8_t> Image::save_exr_to_buffer(
	bool p_grayscale, bool p_color_image, float p_max_value) const
{
	if (save_exr_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}
	return save_exr_buffer_func(Ref<Image>((Image*)this), p_grayscale, p_color_image, p_max_value);
}

Error Image::save_dds(const String& p_path) const
{
	if (save_dds_func == nullptr) {
		return ERR_UNAVAILABLE;
	}

	return save_dds_func(p_path, Ref<Image>((Image*)this));
}

Vector<uint8_t> Image::save_dds_to_buffer() const
{
	if (save_dds_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}
	return save_dds_buffer_func(Ref<Image>((Image*)this));
}

Error Image::save_webp(const String& p_path, const bool p_lossy, const float p_quality) const
{
	if (save_webp_func == nullptr) {
		return ERR_UNAVAILABLE;
	}
	ERR_FAIL_COND_V_MSG(p_lossy && !(0.0f <= p_quality && p_quality <= 1.0f), ERR_INVALID_PARAMETER,
		vformat("The WebP lossy quality was set to %f, which is not valid. WebP lossy quality must "
				"be between 0.0 and 1.0 (inclusive).",
			p_quality));

	return save_webp_func(p_path, Ref<Image>((Image*)this), p_lossy, p_quality);
}

Vector<uint8_t> Image::save_webp_to_buffer(const bool p_lossy, const float p_quality) const
{
	if (save_webp_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}
	ERR_FAIL_COND_V_MSG(p_lossy && !(0.0f <= p_quality && p_quality <= 1.0f), Vector<uint8_t>(),
		vformat("The WebP lossy quality was set to %f, which is not valid. WebP lossy quality must "
				"be between 0.0 and 1.0 (inclusive).",
			p_quality));

	return save_webp_buffer_func(Ref<Image>((Image*)this), p_lossy, p_quality);
}

int64_t Image::get_image_data_size(int p_width, int p_height, Format p_format, bool p_mipmaps)
{
	int mm;
	return _get_dst_image_size(p_width, p_height, p_format, mm, p_mipmaps ? -1 : 0);
}

int Image::get_image_required_mipmaps(int p_width, int p_height, Format p_format)
{
	int mm;
	_get_dst_image_size(p_width, p_height, p_format, mm, -1);
	return mm;
}

Size2i Image::get_image_mipmap_size(int p_width, int p_height, Format p_format, int p_mipmap)
{
	int mm;
	Size2i ret;
	_get_dst_image_size(p_width, p_height, p_format, mm, p_mipmap, &ret.x, &ret.y);
	return ret;
}

int64_t Image::get_image_mipmap_offset(int p_width, int p_height, Format p_format, int p_mipmap)
{
	if (p_mipmap <= 0) {
		return 0;
	}
	int mm;
	return _get_dst_image_size(p_width, p_height, p_format, mm, p_mipmap - 1);
}

int64_t Image::get_image_mipmap_offset_and_dimensions(
	int p_width, int p_height, Format p_format, int p_mipmap, int& r_w, int& r_h)
{
	if (p_mipmap <= 0) {
		r_w = p_width;
		r_h = p_height;
		return 0;
	}
	int mm;
	return _get_dst_image_size(p_width, p_height, p_format, mm, p_mipmap - 1, &r_w, &r_h);
}

bool Image::is_compressed() const { return is_format_compressed(format); }

bool Image::is_format_compressed(Format p_format)
{
	return (p_format >= FORMAT_DXT1 && p_format <= FORMAT_ASTC_8x8_HDR) ||
		   (p_format >= FORMAT_ASTC_6x6 && p_format <= FORMAT_ASTC_6x6_HDR);
}

Error Image::decompress()
{
	if (((format >= FORMAT_DXT1 && format <= FORMAT_RGTC_RG) || (format == FORMAT_DXT5_RA_AS_RG)) &&
		_image_decompress_bc) {
		_image_decompress_bc(this);
	}
	else if (format >= FORMAT_BPTC_RGBA && format <= FORMAT_BPTC_RGBFU &&
			   _image_decompress_bptc) {
		_image_decompress_bptc(this);
	}
	else if (format == FORMAT_ETC && _image_decompress_etc1) {
		_image_decompress_etc1(this);
	}
	else if (format >= FORMAT_ETC2_R11 && format <= FORMAT_ETC2_RA_AS_RG &&
			   _image_decompress_etc2) {
		_image_decompress_etc2(this);
	}
	else if (((format >= FORMAT_ASTC_4x4 && format <= FORMAT_ASTC_8x8_HDR) ||
				   (format >= FORMAT_ASTC_6x6 && format <= FORMAT_ASTC_6x6_HDR)) &&
			   _image_decompress_astc) {
		_image_decompress_astc(this);
	}
	else {
		return ERR_UNAVAILABLE;
	}
	return OK;
}

bool Image::can_decompress(const String& p_format_tag)
{
	if (p_format_tag == "astc") {
		return _image_decompress_astc != nullptr;
	}
	else if (p_format_tag == "bptc") {
		return _image_decompress_bptc != nullptr;
	}
	else if (p_format_tag == "etc2") {
		return _image_decompress_etc2 != nullptr;
	}
	else if (p_format_tag == "s3tc") {
		return _image_decompress_bc != nullptr;
	}
	return false;
}

Error Image::compress(CompressMode p_mode, CompressSource p_source, CompressProfile p_profile)
{
	ERR_FAIL_INDEX_V_MSG(p_mode, COMPRESS_MAX, ERR_INVALID_PARAMETER, "Invalid compress mode.");
	ERR_FAIL_INDEX_V_MSG(
		p_source, COMPRESS_SOURCE_MAX, ERR_INVALID_PARAMETER, "Invalid compress source.");
	return compress_from_channels(p_mode, detect_used_channels(p_source), p_profile);
}

Error Image::compress_from_channels(
	CompressMode p_mode, UsedChannels p_channels, CompressProfile p_profile)
{
	return _compress_from_channels(p_mode, p_channels, p_profile, BPTC_DETECT);
}

Error Image::_compress_from_channels(CompressMode p_mode, UsedChannels p_channels,
	CompressProfile p_profile, BPTCFormat p_bptc_format)
{
	ERR_FAIL_COND_V(data.is_empty(), ERR_INVALID_DATA);

	// RenderingDevice only.
	switch (p_mode) {
	case COMPRESS_S3TC: {
		ERR_FAIL_NULL_V(_image_compress_bc_func, ERR_UNAVAILABLE);
		_image_compress_bc_func(this, p_channels);
	} break;
	case COMPRESS_ETC: {
		ERR_FAIL_NULL_V(_image_compress_etc1_func, ERR_UNAVAILABLE);
		_image_compress_etc1_func(this);
	} break;
	case COMPRESS_ETC2: {
		ERR_FAIL_NULL_V(_image_compress_etc2_func, ERR_UNAVAILABLE);
		_image_compress_etc2_func(this, p_channels);
	} break;
	case COMPRESS_BPTC: {
		ERR_FAIL_NULL_V(_image_compress_bptc_func, ERR_UNAVAILABLE);
		_image_compress_bptc_func(this, p_channels, p_bptc_format);
	} break;
	case COMPRESS_ASTC: {
		ERR_FAIL_NULL_V(_image_compress_astc_func, ERR_UNAVAILABLE);
		_image_compress_astc_func(this, p_channels, p_profile);
	} break;
	case COMPRESS_MAX: {
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	} break;
	}

	return OK;
}

Image::Image(const char** p_xpm)
{
	width = 0;
	height = 0;
	mipmaps = false;
	format = FORMAT_L8;

	initialize_data(p_xpm);
}

Image::Image(int p_width, int p_height, bool p_use_mipmaps, Format p_format)
{
	width = 0;
	height = 0;
	mipmaps = p_use_mipmaps;
	format = FORMAT_L8;

	initialize_data(p_width, p_height, p_use_mipmaps, p_format);
}

Image::Image(
	int p_width, int p_height, bool p_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	width = 0;
	height = 0;
	mipmaps = p_mipmaps;
	format = FORMAT_L8;

	initialize_data(p_width, p_height, p_mipmaps, p_format, p_data);
}

Rect2i Image::get_used_rect() const
{
	if (format != FORMAT_LA8 && format != FORMAT_RGBA8 && format != FORMAT_RGBAF &&
		format != FORMAT_RGBAH && format != FORMAT_RGBA4444 && format != FORMAT_RGB565) {
		return Rect2i(0, 0, width, height);
	}

	int len = data.size();

	if (len == 0) {
		return Rect2i();
	}

	int minx = 0xFFFFFF, miny = 0xFFFFFFF;
	int maxx = -1, maxy = -1;
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			if (!(get_pixel(i, j).a > 0)) {
				continue;
			}
			if (i > maxx) {
				maxx = i;
			}
			if (j > maxy) {
				maxy = j;
			}
			if (i < minx) {
				minx = i;
			}
			if (j < miny) {
				miny = j;
			}
		}
	}

	if (maxx == -1) {
		return Rect2i();
	}
	else {
		return Rect2i(minx, miny, maxx - minx + 1, maxy - miny + 1);
	}
}

Ref<Image> Image::get_region(const Rect2i& p_region) const
{
	Ref<Image> img = memnew(Image(p_region.size.x, p_region.size.y, mipmaps, format));
	img->blit_rect(Ref<Image>((Image*)this), p_region, Point2i(0, 0));
	return img;
}

void Image::_get_clipped_src_and_dest_rects(const Ref<Image>& p_src, const Rect2i& p_src_rect,
	const Point2i& p_dest, Rect2i& r_clipped_src_rect, Rect2i& r_clipped_dest_rect) const
{
	r_clipped_dest_rect.position = p_dest;
	r_clipped_src_rect = p_src_rect;

	if (r_clipped_src_rect.position.x < 0) {
		r_clipped_dest_rect.position.x -= r_clipped_src_rect.position.x;
		r_clipped_src_rect.size.x += r_clipped_src_rect.position.x;
		r_clipped_src_rect.position.x = 0;
	}
	if (r_clipped_src_rect.position.y < 0) {
		r_clipped_dest_rect.position.y -= r_clipped_src_rect.position.y;
		r_clipped_src_rect.size.y += r_clipped_src_rect.position.y;
		r_clipped_src_rect.position.y = 0;
	}

	if (r_clipped_dest_rect.position.x < 0) {
		r_clipped_src_rect.position.x -= r_clipped_dest_rect.position.x;
		r_clipped_src_rect.size.x += r_clipped_dest_rect.position.x;
		r_clipped_dest_rect.position.x = 0;
	}
	if (r_clipped_dest_rect.position.y < 0) {
		r_clipped_src_rect.position.y -= r_clipped_dest_rect.position.y;
		r_clipped_src_rect.size.y += r_clipped_dest_rect.position.y;
		r_clipped_dest_rect.position.y = 0;
	}

	r_clipped_src_rect.size.x =
		MAX(0, MIN(r_clipped_src_rect.size.x, MIN(p_src->width - r_clipped_src_rect.position.x,
												  width - r_clipped_dest_rect.position.x)));
	r_clipped_src_rect.size.y =
		MAX(0, MIN(r_clipped_src_rect.size.y, MIN(p_src->height - r_clipped_src_rect.position.y,
												  height - r_clipped_dest_rect.position.y)));

	r_clipped_dest_rect.size.x = r_clipped_src_rect.size.x;
	r_clipped_dest_rect.size.y = r_clipped_src_rect.size.y;
}

void Image::blit_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(p_src.is_null(), "Cannot blit_rect an image: invalid source Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(format != p_src->format);
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot blit_rect in compressed image formats.");

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	uint8_t* wp = data.ptrw();
	uint8_t* dst_data_ptr = wp;

	const uint8_t* rp = p_src->data.ptr();
	const uint8_t* src_data_ptr = rp;

	int pixel_size = get_format_pixel_size(format);

	// If the rect width is equivalent for both src and dst and the x offset is 0, we can blit in a
	// single memcpy. Else, we do a per-line copy.
	if ((dest_rect.size.x == p_src->width) && (p_src->width == width) &&
		(dest_rect.position.x == 0) && (src_rect.position.x == 0)) {
		const uint8_t* src = &src_data_ptr[(src_rect.position.y * p_src->width) * pixel_size];
		uint8_t* dst = &dst_data_ptr[(dest_rect.position.y * width) * pixel_size];
		memcpy(dst, src, width * dest_rect.size.y * pixel_size);
	}
	else {
		for (int i = 0; i < dest_rect.size.y; i++) {
			const uint8_t* src =
				&src_data_ptr[((src_rect.position.y + i) * p_src->width + src_rect.position.x) *
							  pixel_size];
			uint8_t* dst =
				&dst_data_ptr[((dest_rect.position.y + i) * width + dest_rect.position.x) *
							  pixel_size];
			memcpy(dst, src, pixel_size * dest_rect.size.x);
		}
	}
}

void Image::blit_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask,
	const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(
		p_src.is_null(), "Cannot blit_rect_mask an image: invalid source Image object.");
	ERR_FAIL_COND_MSG(
		p_mask.is_null(), "Cannot blit_rect_mask an image: invalid mask Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	int maskdsize = p_mask->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(maskdsize == 0);
	ERR_FAIL_COND_MSG(
		p_src->width != p_mask->width, "Source image width is different from mask width.");
	ERR_FAIL_COND_MSG(
		p_src->height != p_mask->height, "Source image height is different from mask height.");
	ERR_FAIL_COND(format != p_src->format);

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	uint8_t* wp = data.ptrw();
	uint8_t* dst_data_ptr = wp;

	const uint8_t* rp = p_src->data.ptr();
	const uint8_t* src_data_ptr = rp;

	int pixel_size = get_format_pixel_size(format);

	Ref<Image> msk = p_mask;

	for (int i = 0; i < dest_rect.size.y; i++) {
		for (int j = 0; j < dest_rect.size.x; j++) {
			int src_x = src_rect.position.x + j;
			int src_y = src_rect.position.y + i;

			if (msk->get_pixel(src_x, src_y).a != 0) {
				int dst_x = dest_rect.position.x + j;
				int dst_y = dest_rect.position.y + i;

				const uint8_t* src = &src_data_ptr[(src_y * p_src->width + src_x) * pixel_size];
				uint8_t* dst = &dst_data_ptr[(dst_y * width + dst_x) * pixel_size];

				for (int k = 0; k < pixel_size; k++) {
					dst[k] = src[k];
				}
			}
		}
	}
}

void Image::blend_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(p_src.is_null(), "Cannot blend_rect an image: invalid source Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(format != p_src->format);

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	Ref<Image> img = p_src;

	for (int i = 0; i < dest_rect.size.y; i++) {
		for (int j = 0; j < dest_rect.size.x; j++) {
			int src_x = src_rect.position.x + j;
			int src_y = src_rect.position.y + i;

			int dst_x = dest_rect.position.x + j;
			int dst_y = dest_rect.position.y + i;

			Color sc = img->get_pixel(src_x, src_y);
			if (sc.a != 0) {
				Color dc = get_pixel(dst_x, dst_y);
				dc = dc.blend(sc);
				set_pixel(dst_x, dst_y, dc);
			}
		}
	}
}

void Image::blend_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask,
	const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(
		p_src.is_null(), "Cannot blend_rect_mask an image: invalid source Image object.");
	ERR_FAIL_COND_MSG(
		p_mask.is_null(), "Cannot blend_rect_mask an image: invalid mask Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	int maskdsize = p_mask->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(maskdsize == 0);
	ERR_FAIL_COND_MSG(
		p_src->width != p_mask->width, "Source image width is different from mask width.");
	ERR_FAIL_COND_MSG(
		p_src->height != p_mask->height, "Source image height is different from mask height.");
	ERR_FAIL_COND(format != p_src->format);

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	Ref<Image> img = p_src;
	Ref<Image> msk = p_mask;

	for (int i = 0; i < dest_rect.size.y; i++) {
		for (int j = 0; j < dest_rect.size.x; j++) {
			int src_x = src_rect.position.x + j;
			int src_y = src_rect.position.y + i;

			// If the mask's pixel is transparent then we skip it
			// Color c = msk->get_pixel(src_x, src_y);
			// if (c.a == 0) continue;
			if (msk->get_pixel(src_x, src_y).a != 0) {
				int dst_x = dest_rect.position.x + j;
				int dst_y = dest_rect.position.y + i;

				Color sc = img->get_pixel(src_x, src_y);
				if (sc.a != 0) {
					Color dc = get_pixel(dst_x, dst_y);
					dc = dc.blend(sc);
					set_pixel(dst_x, dst_y, dc);
				}
			}
		}
	}
}

// Repeats `p_pixel` `p_count` times in consecutive memory.
// Results in the original pixel and `p_count - 1` subsequent copies of it.
void Image::_repeat_pixel_over_subsequent_memory(uint8_t* p_pixel, int p_pixel_size, int p_count)
{
	int offset = 1;
	for (int stride = 1; offset + stride <= p_count; stride *= 2) {
		memcpy(p_pixel + offset * p_pixel_size, p_pixel, stride * p_pixel_size);
		offset += stride;
	}
	if (offset < p_count) {
		memcpy(p_pixel + offset * p_pixel_size, p_pixel, (p_count - offset) * p_pixel_size);
	}
}

void Image::fill(const Color& p_color)
{
	if (data.is_empty()) {
		return;
	}
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot fill in compressed image formats.");

	uint8_t* dst_data_ptr = data.ptrw();
	int pixel_size = get_format_pixel_size(format);
	int64_t pixel_count = data.size() / pixel_size;

	// Put first pixel with the format-aware API.
	_set_color_at_ofs(dst_data_ptr, 0, p_color);

	_repeat_pixel_over_subsequent_memory(dst_data_ptr, pixel_size, pixel_count);
}

void Image::fill_rect(const Rect2i& p_rect, const Color& p_color)
{
	if (data.is_empty()) {
		return;
	}
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot fill rect in compressed image formats.");

	Rect2i r = Rect2i(0, 0, width, height).intersection(p_rect.abs());
	if (!r.has_area()) {
		return;
	}

	uint8_t* dst_data_ptr = data.ptrw();

	int pixel_size = get_format_pixel_size(format);

	// Put first pixel with the format-aware API.
	uint8_t* rect_first_pixel_ptr =
		&dst_data_ptr[(r.position.y * width + r.position.x) * pixel_size];
	_set_color_at_ofs(rect_first_pixel_ptr, 0, p_color);

	if (r.size.x == width) {
		// No need to fill rows separately.
		_repeat_pixel_over_subsequent_memory(rect_first_pixel_ptr, pixel_size, width * r.size.y);
	}
	else {
		_repeat_pixel_over_subsequent_memory(rect_first_pixel_ptr, pixel_size, r.size.x);
		for (int y = 1; y < r.size.y; y++) {
			memcpy(rect_first_pixel_ptr + y * width * pixel_size, rect_first_pixel_ptr,
				r.size.x * pixel_size);
		}
	}
}

Color Image::get_pixelv(const Point2i& p_point) const { return get_pixel(p_point.x, p_point.y); }

void Image::_copy_internals_from(const Image& p_image)
{
	format = p_image.format;
	width = p_image.width;
	height = p_image.height;
	mipmaps = p_image.mipmaps;
	data = p_image.data;
}

_FORCE_INLINE_ Color color_from_rgba4444(uint16_t p_col)
{
	float r = ((p_col >> 12) & 0xF) / 15.0;
	float g = ((p_col >> 8) & 0xF) / 15.0;
	float b = ((p_col >> 4) & 0xF) / 15.0;
	float a = (p_col & 0xF) / 15.0;
	return Color(r, g, b, a);
}

_FORCE_INLINE_ uint16_t color_to_rgba4444(Color p_col)
{
	uint16_t rgba = 0;

	rgba = uint16_t(CLAMP(p_col.r * 15.0, 0, 15)) << 12;
	rgba |= uint16_t(CLAMP(p_col.g * 15.0, 0, 15)) << 8;
	rgba |= uint16_t(CLAMP(p_col.b * 15.0, 0, 15)) << 4;
	rgba |= uint16_t(CLAMP(p_col.a * 15.0, 0, 15));

	return rgba;
}

_FORCE_INLINE_ Color color_from_rgb565(uint16_t p_col)
{
	float r = ((p_col >> 11) & 0x1F) / 31.0;
	float g = ((p_col >> 5) & 0x3F) / 63.0;
	float b = (p_col & 0x1F) / 31.0;
	return Color(r, g, b, 1.0);
}

_FORCE_INLINE_ uint16_t color_to_rgb565(Color p_col)
{
	uint16_t rgba = 0;

	rgba = uint16_t(CLAMP(p_col.r * 31.0, 0, 31)) << 11;
	rgba |= uint16_t(CLAMP(p_col.g * 63.0, 0, 63)) << 5;
	rgba |= uint16_t(CLAMP(p_col.b * 31.0, 0, 31));

	return rgba;
}

Color Image::_get_color_at_ofs(const uint8_t* p_ptr, uint32_t p_ofs) const
{
	switch (format) {
	case FORMAT_L8: {
		float l = p_ptr[p_ofs] / 255.0;
		return Color(l, l, l, 1);
	}
	case FORMAT_LA8: {
		float l = p_ptr[p_ofs * 2 + 0] / 255.0;
		float a = p_ptr[p_ofs * 2 + 1] / 255.0;
		return Color(l, l, l, a);
	}
	case FORMAT_R8: {
		float r = p_ptr[p_ofs] / 255.0;
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RG8: {
		float r = p_ptr[p_ofs * 2 + 0] / 255.0;
		float g = p_ptr[p_ofs * 2 + 1] / 255.0;
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGB8: {
		float r = p_ptr[p_ofs * 3 + 0] / 255.0;
		float g = p_ptr[p_ofs * 3 + 1] / 255.0;
		float b = p_ptr[p_ofs * 3 + 2] / 255.0;
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBA8: {
		float r = p_ptr[p_ofs * 4 + 0] / 255.0;
		float g = p_ptr[p_ofs * 4 + 1] / 255.0;
		float b = p_ptr[p_ofs * 4 + 2] / 255.0;
		float a = p_ptr[p_ofs * 4 + 3] / 255.0;
		return Color(r, g, b, a);
	}
	case FORMAT_RGBA4444: {
		return color_from_rgba4444(((uint16_t*)p_ptr)[p_ofs]);
	}
	case FORMAT_RGB565: {
		return color_from_rgb565(((uint16_t*)p_ptr)[p_ofs]);
	}
	case FORMAT_RF: {
		float r = ((float*)p_ptr)[p_ofs];
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RGF: {
		float r = ((float*)p_ptr)[p_ofs * 2 + 0];
		float g = ((float*)p_ptr)[p_ofs * 2 + 1];
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGBF: {
		float r = ((float*)p_ptr)[p_ofs * 3 + 0];
		float g = ((float*)p_ptr)[p_ofs * 3 + 1];
		float b = ((float*)p_ptr)[p_ofs * 3 + 2];
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBAF: {
		float r = ((float*)p_ptr)[p_ofs * 4 + 0];
		float g = ((float*)p_ptr)[p_ofs * 4 + 1];
		float b = ((float*)p_ptr)[p_ofs * 4 + 2];
		float a = ((float*)p_ptr)[p_ofs * 4 + 3];
		return Color(r, g, b, a);
	}
	case FORMAT_RH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs];
		return Color(Math::half_to_float(r), 0, 0, 1);
	}
	case FORMAT_RGH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 2 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 2 + 1];
		return Color(Math::half_to_float(r), Math::half_to_float(g), 0, 1);
	}
	case FORMAT_RGBH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 3 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 3 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 3 + 2];
		return Color(Math::half_to_float(r), Math::half_to_float(g), Math::half_to_float(b), 1);
	}
	case FORMAT_RGBAH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 4 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 4 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 4 + 2];
		uint16_t a = ((uint16_t*)p_ptr)[p_ofs * 4 + 3];
		return Color(Math::half_to_float(r), Math::half_to_float(g), Math::half_to_float(b),
			Math::half_to_float(a));
	}
	case FORMAT_RGBE9995: {
		return Color::from_rgbe9995(((uint32_t*)p_ptr)[p_ofs]);
	}
	case FORMAT_R16: {
		float r = ((uint16_t*)p_ptr)[p_ofs] / 65535.0f;
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RG16: {
		float r = ((uint16_t*)p_ptr)[p_ofs * 2 + 0] / 65535.0f;
		float g = ((uint16_t*)p_ptr)[p_ofs * 2 + 1] / 65535.0f;
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGB16: {
		float r = ((uint16_t*)p_ptr)[p_ofs * 3 + 0] / 65535.0f;
		float g = ((uint16_t*)p_ptr)[p_ofs * 3 + 1] / 65535.0f;
		float b = ((uint16_t*)p_ptr)[p_ofs * 3 + 2] / 65535.0f;
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBA16: {
		float r = ((uint16_t*)p_ptr)[p_ofs * 4 + 0] / 65535.0f;
		float g = ((uint16_t*)p_ptr)[p_ofs * 4 + 1] / 65535.0f;
		float b = ((uint16_t*)p_ptr)[p_ofs * 4 + 2] / 65535.0f;
		float a = ((uint16_t*)p_ptr)[p_ofs * 4 + 3] / 65535.0f;
		return Color(r, g, b, a);
	}
	case FORMAT_R16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs];
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RG16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 2 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 2 + 1];
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGB16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 3 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 3 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 3 + 2];
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBA16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 4 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 4 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 4 + 2];
		uint16_t a = ((uint16_t*)p_ptr)[p_ofs * 4 + 3];
		return Color(r, g, b, a);
	}

	default: {
		ERR_FAIL_V_MSG(Color(), "Can't get_pixel() on compressed image, sorry.");
	}
	}
}

void Image::_set_color_at_ofs(uint8_t* r_ptr, uint32_t p_ofs, const Color& p_color)
{
	switch (format) {
	case FORMAT_L8: {
		r_ptr[p_ofs] = uint8_t(CLAMP(p_color.get_v() * 255.0, 0, 255));
	} break;
	case FORMAT_LA8: {
		r_ptr[p_ofs * 2 + 0] = uint8_t(CLAMP(p_color.get_v() * 255.0, 0, 255));
		r_ptr[p_ofs * 2 + 1] = uint8_t(CLAMP(p_color.a * 255.0, 0, 255));
	} break;
	case FORMAT_R8: {
		r_ptr[p_ofs] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
	} break;
	case FORMAT_RG8: {
		r_ptr[p_ofs * 2 + 0] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
		r_ptr[p_ofs * 2 + 1] = uint8_t(CLAMP(p_color.g * 255.0, 0, 255));
	} break;
	case FORMAT_RGB8: {
		r_ptr[p_ofs * 3 + 0] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
		r_ptr[p_ofs * 3 + 1] = uint8_t(CLAMP(p_color.g * 255.0, 0, 255));
		r_ptr[p_ofs * 3 + 2] = uint8_t(CLAMP(p_color.b * 255.0, 0, 255));
	} break;
	case FORMAT_RGBA8: {
		r_ptr[p_ofs * 4 + 0] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
		r_ptr[p_ofs * 4 + 1] = uint8_t(CLAMP(p_color.g * 255.0, 0, 255));
		r_ptr[p_ofs * 4 + 2] = uint8_t(CLAMP(p_color.b * 255.0, 0, 255));
		r_ptr[p_ofs * 4 + 3] = uint8_t(CLAMP(p_color.a * 255.0, 0, 255));
	} break;
	case FORMAT_RGBA4444: {
		((uint16_t*)r_ptr)[p_ofs] = color_to_rgba4444(p_color);
	} break;
	case FORMAT_RGB565: {
		((uint16_t*)r_ptr)[p_ofs] = color_to_rgb565(p_color);
	} break;
	case FORMAT_RF: {
		((float*)r_ptr)[p_ofs] = p_color.r;
	} break;
	case FORMAT_RGF: {
		((float*)r_ptr)[p_ofs * 2 + 0] = p_color.r;
		((float*)r_ptr)[p_ofs * 2 + 1] = p_color.g;
	} break;
	case FORMAT_RGBF: {
		((float*)r_ptr)[p_ofs * 3 + 0] = p_color.r;
		((float*)r_ptr)[p_ofs * 3 + 1] = p_color.g;
		((float*)r_ptr)[p_ofs * 3 + 2] = p_color.b;
	} break;
	case FORMAT_RGBAF: {
		((float*)r_ptr)[p_ofs * 4 + 0] = p_color.r;
		((float*)r_ptr)[p_ofs * 4 + 1] = p_color.g;
		((float*)r_ptr)[p_ofs * 4 + 2] = p_color.b;
		((float*)r_ptr)[p_ofs * 4 + 3] = p_color.a;
	} break;
	case FORMAT_RH: {
		((uint16_t*)r_ptr)[p_ofs] = Math::make_half_float(p_color.r);
	} break;
	case FORMAT_RGH: {
		((uint16_t*)r_ptr)[p_ofs * 2 + 0] = Math::make_half_float(p_color.r);
		((uint16_t*)r_ptr)[p_ofs * 2 + 1] = Math::make_half_float(p_color.g);
	} break;
	case FORMAT_RGBH: {
		((uint16_t*)r_ptr)[p_ofs * 3 + 0] = Math::make_half_float(p_color.r);
		((uint16_t*)r_ptr)[p_ofs * 3 + 1] = Math::make_half_float(p_color.g);
		((uint16_t*)r_ptr)[p_ofs * 3 + 2] = Math::make_half_float(p_color.b);
	} break;
	case FORMAT_RGBAH: {
		((uint16_t*)r_ptr)[p_ofs * 4 + 0] = Math::make_half_float(p_color.r);
		((uint16_t*)r_ptr)[p_ofs * 4 + 1] = Math::make_half_float(p_color.g);
		((uint16_t*)r_ptr)[p_ofs * 4 + 2] = Math::make_half_float(p_color.b);
		((uint16_t*)r_ptr)[p_ofs * 4 + 3] = Math::make_half_float(p_color.a);
	} break;
	case FORMAT_RGBE9995: {
		((uint32_t*)r_ptr)[p_ofs] = p_color.to_rgbe9995();
	} break;
	case FORMAT_R16: {
		((uint16_t*)r_ptr)[p_ofs] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
	} break;
	case FORMAT_RG16: {
		((uint16_t*)r_ptr)[p_ofs * 2 + 0] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 2 + 1] = uint16_t(CLAMP(p_color.g * 65535.0, 0, 65535));
	} break;
	case FORMAT_RGB16: {
		((uint16_t*)r_ptr)[p_ofs * 3 + 0] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 1] = uint16_t(CLAMP(p_color.g * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 2] = uint16_t(CLAMP(p_color.b * 65535.0, 0, 65535));
	} break;
	case FORMAT_RGBA16: {
		((uint16_t*)r_ptr)[p_ofs * 4 + 0] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 1] = uint16_t(CLAMP(p_color.g * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 2] = uint16_t(CLAMP(p_color.b * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 3] = uint16_t(CLAMP(p_color.a * 65535.0, 0, 65535));
	} break;
	case FORMAT_R16I: {
		((uint16_t*)r_ptr)[p_ofs] = uint16_t(CLAMP(p_color.r, 0, 65535));
	} break;
	case FORMAT_RG16I: {
		((uint16_t*)r_ptr)[p_ofs * 2 + 0] = uint16_t(CLAMP(p_color.r, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 2 + 1] = uint16_t(CLAMP(p_color.g, 0, 65535));
	} break;
	case FORMAT_RGB16I: {
		((uint16_t*)r_ptr)[p_ofs * 3 + 0] = uint16_t(CLAMP(p_color.r, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 1] = uint16_t(CLAMP(p_color.g, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 2] = uint16_t(CLAMP(p_color.b, 0, 65535));
	} break;
	case FORMAT_RGBA16I: {
		((uint16_t*)r_ptr)[p_ofs * 4 + 0] = uint16_t(CLAMP(p_color.r, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 1] = uint16_t(CLAMP(p_color.g, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 2] = uint16_t(CLAMP(p_color.b, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 3] = uint16_t(CLAMP(p_color.a, 0, 65535));
	} break;

	default: {
		ERR_FAIL_MSG("Can't set_pixel() on compressed image, sorry.");
	}
	}
}

Color Image::get_pixel(int p_x, int p_y) const
{
#ifdef DEBUG_ENABLED
	ERR_FAIL_INDEX_V(p_x, width, Color());
	ERR_FAIL_INDEX_V(p_y, height, Color());
#endif

	uint32_t ofs = p_y * width + p_x;
	return _get_color_at_ofs(data.ptr(), ofs);
}

void Image::set_pixelv(const Point2i& p_point, const Color& p_color)
{
	set_pixel(p_point.x, p_point.y, p_color);
}

void Image::set_pixel(int p_x, int p_y, const Color& p_color)
{
#ifdef DEBUG_ENABLED
	ERR_FAIL_INDEX(p_x, width);
	ERR_FAIL_INDEX(p_y, height);
#endif

	uint32_t ofs = p_y * width + p_x;
	_set_color_at_ofs(data.ptrw(), ofs, p_color);
}

const uint8_t* Image::ptr() const { return data.ptr(); }

uint8_t* Image::ptrw() { return data.ptrw(); }

int64_t Image::get_data_size() const { return data.size(); }

void Image::adjust_bcs(float p_brightness, float p_contrast, float p_saturation)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot adjust_bcs in compressed image formats.");

	uint8_t* w = data.ptrw();
	uint32_t pixel_size = get_format_pixel_size(format);
	uint32_t pixel_count = data.size() / pixel_size;

	for (uint32_t i = 0; i < pixel_count; i++) {
		Color c = _get_color_at_ofs(w, i);
		Vector3 rgb(c.r, c.g, c.b);

		rgb *= p_brightness;
		rgb = Vector3(0.5, 0.5, 0.5).lerp(rgb, p_contrast);
		float center = (rgb.x + rgb.y + rgb.z) / 3.0;
		rgb = Vector3(center, center, center).lerp(rgb, p_saturation);
		c.r = rgb.x;
		c.g = rgb.y;
		c.b = rgb.z;
		_set_color_at_ofs(w, i, c);
	}
}

Image::UsedChannels Image::detect_used_channels(CompressSource p_source) const
{
	ERR_FAIL_COND_V(data.is_empty(), USED_CHANNELS_RGBA);
	ERR_FAIL_COND_V(is_compressed(), USED_CHANNELS_RGBA);

	if (p_source == COMPRESS_SOURCE_NORMAL) {
		return USED_CHANNELS_RG; // Normal maps only use RG channels.
	}

	if (format == FORMAT_L8) {
		return USED_CHANNELS_L; // Grayscale only cannot have any channel less.
	}
	else if (format == FORMAT_R8 || format == FORMAT_RH || format == FORMAT_RF ||
			   format == FORMAT_R16 || format == FORMAT_R16I) {
		return USED_CHANNELS_R; // Red only cannot have any channel less.
	}

	const bool supports_alpha = format == FORMAT_RGBA8 || format == FORMAT_RGBA4444 ||
								format == FORMAT_RGBAH || format == FORMAT_RGBAF ||
								format == FORMAT_RGBA16 || format == FORMAT_RGBA16I;
	bool r = false, g = false, b = false, a = false, c = false;

	const uint8_t* data_ptr = data.ptr();
	const uint32_t data_total = width * height;

	for (uint32_t i = 0; i < data_total; i++) {
		Color col = _get_color_at_ofs(data_ptr, i);

		if (!r && col.r > 0.001) {
			r = true;
		}
		if (!g && col.g > 0.001) {
			g = true;
		}
		if (!b && col.b > 0.001) {
			b = true;
		}
		if (!a && col.a < 0.999) {
			a = true;
		}

		if (col.r != col.b || col.r != col.g || col.b != col.g) {
			c = true; // The image is not grayscale.
		}

		if (r && g && b && c) {
			// All channels are used, no need to continue.
			if (!supports_alpha) {
				break;
			}
			else if (a) {
				break;
			}
		}
	}

	UsedChannels used_channels;

	if (!c) {
		// Uniform RGB (grayscale).
		if (a) {
			used_channels = USED_CHANNELS_LA;
		}
		else {
			used_channels = USED_CHANNELS_L;
		}
	}
	else {
		// Colored image.
		if (a) {
			used_channels = USED_CHANNELS_RGBA;
		}
		else if (b) {
			used_channels = USED_CHANNELS_RGB;
		}
		else if (g) {
			used_channels = USED_CHANNELS_RG;
		}
		else {
			used_channels = USED_CHANNELS_R;
		}
	}

	if (p_source == COMPRESS_SOURCE_SRGB &&
		(used_channels == USED_CHANNELS_R || used_channels == USED_CHANNELS_RG)) {
		used_channels = USED_CHANNELS_RGB; // R and RG do not support SRGB.
	}

	return used_channels;
}

void Image::optimize_channels()
{
	switch (detect_used_channels()) {
	case USED_CHANNELS_L:
		convert(FORMAT_L8);
		break;
	case USED_CHANNELS_LA:
		convert(FORMAT_LA8);
		break;
	case USED_CHANNELS_R:
		convert(FORMAT_R8);
		break;
	case USED_CHANNELS_RG:
		convert(FORMAT_RG8);
		break;
	case USED_CHANNELS_RGB:
		convert(FORMAT_RGB8);
		break;
	case USED_CHANNELS_RGBA:
		convert(FORMAT_RGBA8);
		break;
	}
}

void Image::normal_map_to_xy()
{
	convert(Image::FORMAT_RGBA8);

	{
		int len = data.size() / 4;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i << 2) + 3] = data_ptr[(i << 2) + 0]; // x to w
			data_ptr[(i << 2) + 0] = data_ptr[(i << 2) + 1]; // y to xz
			data_ptr[(i << 2) + 2] = data_ptr[(i << 2) + 1];
		}
	}

	convert(Image::FORMAT_LA8);
}

Ref<Image> Image::rgbe_to_srgb()
{
	if (data.is_empty()) {
		return Ref<Image>();
	}

	ERR_FAIL_COND_V(format != FORMAT_RGBE9995, Ref<Image>());

	Ref<Image> new_image = create_empty(width, height, false, Image::FORMAT_RGB8);

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			new_image->set_pixel(col, row, get_pixel(col, row).linear_to_srgb());
		}
	}

	if (has_mipmaps()) {
		new_image->generate_mipmaps();
	}

	return new_image;
}

Ref<Image> Image::get_image_from_mipmap(int p_mipmap) const
{
	int64_t ofs, size;
	int w, h;
	get_mipmap_offset_size_and_dimensions(p_mipmap, ofs, size, w, h);

	Vector<uint8_t> new_data;
	new_data.resize(size);

	{
		uint8_t* wr = new_data.ptrw();
		const uint8_t* rd = data.ptr();
		memcpy(wr, rd + ofs, size);
	}

	Ref<Image> image;
	image.instantiate();
	image->width = w;
	image->height = h;
	image->format = format;
	image->data = new_data;

	image->mipmaps = false;
	return image;
}

void Image::bump_map_to_normal_map(float p_bump_scale)
{
	ERR_FAIL_COND(is_compressed());
	clear_mipmaps();
	convert(Image::FORMAT_RF);

	Vector<uint8_t> result_image; // rgba output
	result_image.resize(width * height * 4);

	{
		const uint8_t* rp = data.ptr();
		uint8_t* wp = result_image.ptrw();

		ERR_FAIL_NULL(rp);

		unsigned char* write_ptr = wp;
		float* read_ptr = (float*)rp;

		for (int ty = 0; ty < height; ty++) {
			int py = ty + 1;
			if (py >= height) {
				py -= height;
			}

			for (int tx = 0; tx < width; tx++) {
				int px = tx + 1;
				if (px >= width) {
					px -= width;
				}
				float here = read_ptr[ty * width + tx];
				float to_right = read_ptr[ty * width + px];
				float above = read_ptr[py * width + tx];
				Vector3 up = Vector3(0, 1, (here - above) * p_bump_scale);
				Vector3 across = Vector3(1, 0, (to_right - here) * p_bump_scale);

				Vector3 normal = across.cross(up);
				normal.normalize();

				write_ptr[((ty * width + tx) << 2) + 0] = (127.5 + normal.x * 127.5);
				write_ptr[((ty * width + tx) << 2) + 1] = (127.5 + normal.y * 127.5);
				write_ptr[((ty * width + tx) << 2) + 2] = (127.5 + normal.z * 127.5);
				write_ptr[((ty * width + tx) << 2) + 3] = 255;
			}
		}
	}
	format = FORMAT_RGBA8;
	data = result_image;
}

bool Image::detect_signed(bool p_include_mips) const
{
	ERR_FAIL_COND_V(is_compressed(), false);

	if (format >= Image::FORMAT_RH && format <= Image::FORMAT_RGBAH) {
		const uint16_t* img_data = reinterpret_cast<const uint16_t*>(data.ptr());
		const uint64_t img_size = p_include_mips
									  ? (data.size() / 2)
									  : (width * height * get_format_pixel_size(format) / 2);

		for (uint64_t i = 0; i < img_size; i++) {
			if ((img_data[i] & 0x8000) != 0 && (img_data[i] & 0x7fff) != 0) {
				return true;
			}
		}

	}
	else if (format >= Image::FORMAT_RF && format <= Image::FORMAT_RGBAF) {
		const uint32_t* img_data = reinterpret_cast<const uint32_t*>(data.ptr());
		const uint64_t img_size = p_include_mips
									  ? (data.size() / 4)
									  : (width * height * get_format_pixel_size(format) / 4);

		for (uint64_t i = 0; i < img_size; i++) {
			if ((img_data[i] & 0x80000000) != 0 && (img_data[i] & 0x7fffffff) != 0) {
				return true;
			}
		}
	}

	return false;
}

void Image::srgb_to_linear()
{
	if (data.is_empty()) {
		return;
	}

	static const uint8_t srgb2lin[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7,
		8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 16,
		17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 29, 29,
		30, 31, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47,
		47, 48, 49, 50, 51, 52, 53, 54, 55, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
		70, 71, 72, 73, 74, 75, 76, 77, 78, 80, 81, 82, 83, 84, 85, 87, 88, 89, 90, 92, 93, 94, 95,
		97, 98, 99, 101, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120, 122,
		123, 125, 126, 128, 129, 131, 132, 134, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150,
		152, 153, 155, 157, 159, 160, 162, 164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182,
		184, 186, 188, 190, 192, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 218,
		220, 222, 224, 226, 228, 230, 232, 235, 237, 239, 241, 243, 245, 248, 250, 252, 255};

	ERR_FAIL_COND(format != FORMAT_RGB8 && format != FORMAT_RGBA8);

	if (format == FORMAT_RGBA8) {
		int len = data.size() / 4;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i << 2) + 0] = srgb2lin[data_ptr[(i << 2) + 0]];
			data_ptr[(i << 2) + 1] = srgb2lin[data_ptr[(i << 2) + 1]];
			data_ptr[(i << 2) + 2] = srgb2lin[data_ptr[(i << 2) + 2]];
		}

	}
	else if (format == FORMAT_RGB8) {
		int len = data.size() / 3;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i * 3) + 0] = srgb2lin[data_ptr[(i * 3) + 0]];
			data_ptr[(i * 3) + 1] = srgb2lin[data_ptr[(i * 3) + 1]];
			data_ptr[(i * 3) + 2] = srgb2lin[data_ptr[(i * 3) + 2]];
		}
	}
}

void Image::linear_to_srgb()
{
	if (data.is_empty()) {
		return;
	}

	static const uint8_t lin2srgb[256] = {0, 12, 21, 28, 33, 38, 42, 46, 49, 52, 55, 58, 61, 63, 66,
		68, 70, 73, 75, 77, 79, 81, 82, 84, 86, 88, 89, 91, 93, 94, 96, 97, 99, 100, 102, 103, 104,
		106, 107, 109, 110, 111, 112, 114, 115, 116, 117, 118, 120, 121, 122, 123, 124, 125, 126,
		127, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 142, 143, 144,
		145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 157, 157, 158, 159, 160,
		161, 161, 162, 163, 164, 165, 165, 166, 167, 168, 168, 169, 170, 171, 171, 172, 173, 174,
		174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186,
		187, 187, 188, 189, 189, 190, 191, 191, 192, 193, 193, 194, 194, 195, 196, 196, 197, 197,
		198, 199, 199, 200, 201, 201, 202, 202, 203, 204, 204, 205, 205, 206, 206, 207, 208,
 208,
		209, 209, 210, 210, 211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218,
		219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 224, 224, 225, 226, 226, 227, 227, 228,
		228, 229, 229, 230, 230, 231, 231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237,
		237, 237, 238, 238, 239, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245,
		245, 246, 246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, 253, 253,
		254, 254, 255};

	ERR_FAIL_COND(format != FORMAT_RGB8 && format != FORMAT_RGBA8);

	if (format == FORMAT_RGBA8) {
		int len = data.size() / 4;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i << 2) + 0] = lin2srgb[data_ptr[(i << 2) + 0]];
			data_ptr[(i << 2) + 1] = lin2srgb[data_ptr[(i << 2) + 1]];
			data_ptr[(i << 2) + 2] = lin2srgb[data_ptr[(i << 2) + 2]];
		}

	}
	else if (format == FORMAT_RGB8) {
		int len = data.size() / 3;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i * 3) + 0] = lin2srgb[data_ptr[(i * 3) + 0]];
			data_ptr[(i * 3) + 1] = lin2srgb[data_ptr[(i * 3) + 1]];
			data_ptr[(i * 3) + 2] = lin2srgb[data_ptr[(i * 3) + 2]];
		}
	}
}

void Image::premultiply_alpha()
{
	if (data.is_empty()) {
		return;
	}

	if (format != FORMAT_RGBA8) {
		return; // not needed
	}

	uint8_t* data_ptr = data.ptrw();

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			uint8_t* ptr = &data_ptr[(i * width + j) * 4];

			ptr[0] = (uint16_t(ptr[0]) * uint16_t(ptr[3]) + 255U) >> 8;
			ptr[1] = (uint16_t(ptr[1]) * uint16_t(ptr[3]) + 255U) >> 8;
			ptr[2] = (uint16_t(ptr[2]) * uint16_t(ptr[3]) + 255U) >> 8;
		}
	}
}

void Image::fix_alpha_edges()
{
	if (data.is_empty()) {
		return;
	}

	if (format != FORMAT_RGBA8) {
		return; // not needed
	}

	Vector<uint8_t> dcopy = data;
	const uint8_t* srcptr = dcopy.ptr();

	uint8_t* data_ptr = data.ptrw();

	const int max_radius = 4;
	const int alpha_threshold = 20;
	const int max_dist = 0x7FFFFFFF;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			const uint8_t* rptr = &srcptr[(i * width + j) * 4];
			uint8_t* wptr = &data_ptr[(i * width + j) * 4];

			if (rptr[3] >= alpha_threshold) {
				continue;
			}

			int closest_dist = max_dist;
			uint8_t closest_color[3] = {0};

			int from_x = MAX(0, j - max_radius);
			int to_x = MIN(width - 1, j + max_radius);
			int from_y = MAX(0, i - max_radius);
			int to_y = MIN(height - 1, i + max_radius);

			for (int k = from_y; k <= to_y; k++) {
				for (int l = from_x; l <= to_x; l++) {
					int dy = i - k;
					int dx = j - l;
					int dist = dy * dy + dx * dx;
					if (dist >= closest_dist) {
						continue;
					}

					const uint8_t* rp2 = &srcptr[(k * width + l) << 2];

					if (rp2[3] < alpha_threshold) {
						continue;
					}

					closest_dist = dist;
					closest_color[0] = rp2[0];
					closest_color[1] = rp2[1];
					closest_color[2] = rp2[2];
				}
			}

			if (closest_dist != max_dist) {
				wptr[0] = closest_color[0];
				wptr[1] = closest_color[1];
				wptr[2] = closest_color[2];
			}
		}
	}
}

String Image::get_format_name(Format p_format)
{
	ERR_FAIL_INDEX_V(p_format, FORMAT_MAX, String());
	return format_names[p_format];
}

uint32_t Image::get_format_component_mask(Format p_format)
{
	const uint32_t r = 1;
	const uint32_t rg = 3;
	const uint32_t rgb = 7;
	const uint32_t rgba = 15;

	switch (p_format) {
	case FORMAT_L8:
		return rgb;
	case FORMAT_LA8:
		return rgba;
	case FORMAT_R8:
		return r;
	case FORMAT_RG8:
		return rg;
	case FORMAT_RGB8:
		return rgb;
	case FORMAT_RGBA8:
		return rgba;
	case FORMAT_RGBA4444:
		return rgba;
	case FORMAT_RGB565:
		return rgb;
	case FORMAT_RF:
		return r;
	case FORMAT_RGF:
		return rg;
	case FORMAT_RGBF:
		return rgb;
	case FORMAT_RGBAF:
		return rgba;
	case FORMAT_RH:
		return r;
	case FORMAT_RGH:
		return rg;
	case FORMAT_RGBH:
		return rgb;
	case FORMAT_RGBAH:
		return rgba;
	case FORMAT_RGBE9995:
		return rgb;
	case FORMAT_DXT1:
		return rgb;
	case FORMAT_DXT3:
		return rgba;
	case FORMAT_DXT5:
		return rgba;
	case FORMAT_RGTC_R:
		return r;
	case FORMAT_RGTC_RG:
		return rg;
	case FORMAT_BPTC_RGBA:
		return rgba;
	case FORMAT_BPTC_RGBF:
		return rgb;
	case FORMAT_BPTC_RGBFU:
		return rgb;
	case FORMAT_ETC:
		return rgb;
	case FORMAT_ETC2_R11:
		return r;
	case FORMAT_ETC2_R11S:
		return r;
	case FORMAT_ETC2_RG11:
		return rg;
	case FORMAT_ETC2_RG11S:
		return rg;
	case FORMAT_ETC2_RGB8:
		return rgb;
	case FORMAT_ETC2_RGBA8:
		return rgba;
	case FORMAT_ETC2_RGB8A1:
		return rgba;
	case FORMAT_ETC2_RA_AS_RG:
		return rg;
	case FORMAT_DXT5_RA_AS_RG:
		return rg;
	case FORMAT_ASTC_4x4:
		return rgba;
	case FORMAT_ASTC_4x4_HDR:
		return rgba;
	case FORMAT_ASTC_6x6:
		return rgba;
	case FORMAT_ASTC_6x6_HDR:
		return rgba;
	case FORMAT_ASTC_8x8:
		return rgba;
	case FORMAT_ASTC_8x8_HDR:
		return rgba;
	case FORMAT_R16:
		return r;
	case FORMAT_RG16:
		return rg;
	case FORMAT_RGB16:
		return rgb;
	case FORMAT_RGBA16:
		return rgba;
	case FORMAT_R16I:
		return r;
	case FORMAT_RG16I:
		return rg;
	case FORMAT_RGB16I:
		return rgb;
	case FORMAT_RGBA16I:
		return rgba;
	default:
		ERR_PRINT("Unhandled format.");
		return rgba;
	}
}

Error Image::load_png_from_buffer(const Vector<uint8_t>& p_array)
{
	return _load_from_buffer(p_array, _png_mem_loader_func);
}

Error Image::load_jpg_from_buffer(const Vector<uint8_t>& p_array)
{
	return _load_from_buffer(p_array, _jpg_mem_loader_func);
}

Error Image::load_exr_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_exr_mem_loader_func, ERR_UNAVAILABLE,
		"The TinyEXR module isn't enabled. Recompile the Godot editor or export template binary "
		"with the `tinyexr_export_templates=yes` SCons option.");
	return _load_from_buffer(p_array, _exr_mem_loader_func);
}

Error Image::load_webp_from_buffer(const Vector<uint8_t>& p_array)
{
	return _load_from_buffer(p_array, _webp_mem_loader_func);
}

Error Image::load_tga_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_tga_mem_loader_func, ERR_UNAVAILABLE,
		"The TGA module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_tga_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _tga_mem_loader_func);
}

Error Image::load_bmp_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_bmp_mem_loader_func, ERR_UNAVAILABLE,
		"The BMP module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_bmp_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _bmp_mem_loader_func);
}

Error Image::load_dds_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_dds_mem_loader_func, ERR_UNAVAILABLE,
		"The DDS module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_dds_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _dds_mem_loader_func);
}

Error Image::load_svg_from_buffer(const Vector<uint8_t>& p_array, float p_scale)
{
	ERR_FAIL_NULL_V_MSG(_svg_scalable_mem_loader_func, ERR_UNAVAILABLE,
		"The SVG module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_svg_enabled=yes` SCons option.");

	int buffer_size = p_array.size();

	ERR_FAIL_COND_V(buffer_size == 0, ERR_INVALID_PARAMETER);

	Ref<Image> image = _svg_scalable_mem_loader_func(p_array.ptr(), buffer_size, p_scale);
	ERR_FAIL_COND_V(image.is_null(), ERR_PARSE_ERROR);

	copy_internals_from(image);

	return OK;
}

Error Image::load_svg_from_string(const String& p_svg_str, float p_scale)
{
	return load_svg_from_buffer(p_svg_str.to_utf8_buffer(), p_scale);
}

Error Image::load_ktx_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_ktx_mem_loader_func, ERR_UNAVAILABLE,
		"The KTX module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_ktx_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _ktx_mem_loader_func);
}

void Image::convert_rg_to_ra_rgba8()
{
	ERR_FAIL_COND(format != FORMAT_RGBA8);
	ERR_FAIL_COND(data.is_empty());

	int s = data.size();
	uint8_t* w = data.ptrw();
	for (int i = 0; i < s; i += 4) {
		w[i + 3] = w[i + 1];
		w[i + 1] = 0;
		w[i + 2] = 0;
	}
}

void Image::convert_ra_rgba8_to_rg()
{
	ERR_FAIL_COND(format != FORMAT_RGBA8);
	ERR_FAIL_COND(data.is_empty());

	int s = data.size();
	uint8_t* w = data.ptrw();
	for (int i = 0; i < s; i += 4) {
		w[i + 1] = w[i + 3];
		w[i + 2] = 0;
		w[i + 3] = 255;
	}
}

void Image::convert_rgba8_to_bgra8()
{
	ERR_FAIL_COND(format != FORMAT_RGBA8);
	ERR_FAIL_COND(data.is_empty());

	int s = data.size();
	uint8_t* w = data.ptrw();
	for (int i = 0; i < s; i += 4) {
		uint8_t r = w[i];
		w[i] = w[i + 2]; // Swap R to B
		w[i + 2] = r;	 // Swap B to R
	}
}

Error Image::_load_from_buffer(const Vector<uint8_t>& p_array, ImageMemLoadFunc p_loader)
{
	int buffer_size = p_array.size();

	ERR_FAIL_COND_V(buffer_size == 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_loader, ERR_INVALID_PARAMETER);

	const uint8_t* r = p_array.ptr();

	Ref<Image> image = p_loader(r, buffer_size);
	ERR_FAIL_COND_V(image.is_null(), ERR_PARSE_ERROR);

	copy_internals_from(image);

	return OK;
}

void Image::average_4_uint8(
	uint8_t& p_out, const uint8_t& p_a, const uint8_t& p_b, const uint8_t& p_c, const uint8_t& p_d)
{
	p_out = static_cast<uint8_t>((p_a + p_b + p_c + p_d + 2) >> 2);
}

void Image::average_4_float(
	float& p_out, const float& p_a, const float& p_b, const float& p_c, const float& p_d)
{
	p_out = (p_a + p_b + p_c + p_d) * 0.25f;
}

void Image::average_4_half(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = Math::make_half_float((Math::half_to_float(p_a) + Math::half_to_float(p_b) +
									  Math::half_to_float(p_c) + Math::half_to_float(p_d)) *
								  0.25f);
}

void Image::average_4_rgbe9995(uint32_t& p_out, const uint32_t& p_a, const uint32_t& p_b,
	const uint32_t& p_c, const uint32_t& p_d)
{
	p_out = ((Color::from_rgbe9995(p_a) + Color::from_rgbe9995(p_b) + Color::from_rgbe9995(p_c) +
				 Color::from_rgbe9995(p_d)) *
			 0.25f)
				.to_rgbe9995();
}

void Image::average_4_uint16(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = static_cast<uint16_t>((p_a + p_b + p_c + p_d + 2) >> 2);
}

void Image::average_4_rgba4444(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = color_to_rgba4444((color_from_rgba4444(p_a) + color_from_rgba4444(p_b) +
								  color_from_rgba4444(p_c) + color_from_rgba4444(p_d)) *
							  0.25f);
}

void Image::average_4_rgb565(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = color_to_rgb565((color_from_rgb565(p_a) + color_from_rgb565(p_b) +
								color_from_rgb565(p_c) + color_from_rgb565(p_d)) *
							0.25f);
}

void Image::renormalize_uint8(uint8_t* p_rgb)
{
	Vector3 n(p_rgb[0] / 255.0, p_rgb[1] / 255.0, p_rgb[2] / 255.0);
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	n *= 255;
	p_rgb[0] = CLAMP(int(Math::round(n.x)), 0, 255);
	p_rgb[1] = CLAMP(int(Math::round(n.y)), 0, 255);
	p_rgb[2] = CLAMP(int(Math::round(n.z)), 0, 255);
}

void Image::renormalize_float(float* p_rgb)
{
	Vector3 n(p_rgb[0], p_rgb[1], p_rgb[2]);
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	p_rgb[0] = n.x;
	p_rgb[1] = n.y;
	p_rgb[2] = n.z;
}

void Image::renormalize_half(uint16_t* p_rgb)
{
	Vector3 n(Math::half_to_float(p_rgb[0]), Math::half_to_float(p_rgb[1]),
		Math::half_to_float(p_rgb[2]));
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	p_rgb[0] = Math::make_half_float(n.x);
	p_rgb[1] = Math::make_half_float(n.y);
	p_rgb[2] = Math::make_half_float(n.z);
}

void Image::renormalize_uint16(uint16_t* p_rgb)
{
	Vector3 n(p_rgb[0] / 65535.0, p_rgb[1] / 65535.0, p_rgb[2] / 65535.0);
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	n *= 65535;
	p_rgb[0] = CLAMP(int(Math::round(n.x)), 0, 65535);
	p_rgb[1] = CLAMP(int(Math::round(n.y)), 0, 65535);
	p_rgb[2] = CLAMP(int(Math::round
(n.z)), 0, 65535);
}

Image::Image(const uint8_t* p_mem_png_jpg, int p_len)
{
	width = 0;
	height = 0;
	mipmaps = false;
	format = FORMAT_L8;

	if (_png_mem_loader_func) {
		copy_internals_from(_png_mem_loader_func(p_mem_png_jpg, p_len));
	}

	if (is_empty() && _jpg_mem_loader_func) {
		copy_internals_from(_jpg_mem_loader_func(p_mem_png_jpg, p_len));
	}

	if (is_empty() && _webp_mem_loader_func) {
		copy_internals_from(_webp_mem_loader_func(p_mem_png_jpg, p_len));
	}
}

void Image::set_as_black() { memset(data.ptrw(), 0, data.size()); }

void Image::copy_internals_from(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_MSG(p_image.is_null(), "Cannot copy image internals: invalid Image object.");
	format = p_image->format;
	width = p_image->width;
	height = p_image->height;
	mipmaps = p_image->mipmaps;
	data = p_image->data;
}

// === Merged from core/string/ustring.cpp ===
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

#include <cstdio>
#include <thirdparty/grisu2/grisu2.h>
#include "core/crypto/crypto.h"
#include "core/io/ip.h"
#include "core/templates/hashfuncs.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/string/ustring.h"
#include "core/string/translation_server.h"
// --- Inlined from core/string/ucaps.h ---
// This file was generated using the `misc/scripts/ucaps_fetch.py` script.

#define LTU_LEN 1505
#define UTL_LEN 1488

static const int caps_table[LTU_LEN][2] = {
	{ 0x0061, 0x0041 },
	{ 0x0062, 0x0042 },
	{ 0x0063, 0x0043 },
	{ 0x0064, 0x0044 },
	{ 0x0065, 0x0045 },
	{ 0x0066, 0x0046 },
	{ 0x0067, 0x0047 },
	{ 0x0068, 0x0048 },
	{ 0x0069, 0x0049 },
	{ 0x006A, 0x004A },
	{ 0x006B, 0x004B },
	{ 0x006C, 0x004C },
	{ 0x006D, 0x004D },
	{ 0x006E, 0x004E },
	{ 0x006F, 0x004F },
	{ 0x0070, 0x0050 },
	{ 0x0071, 0x0051 },
	{ 0x0072, 0x0052 },
	{ 0x0073, 0x0053 },
	{ 0x0074, 0x0054 },
	{ 0x0075, 0x0055 },
	{ 0x0076, 0x0056 },
	{ 0x0077, 0x0057 },
	{ 0x0078, 0x0058 },
	{ 0x0079, 0x0059 },
	{ 0x007A, 0x005A },
	{ 0x00B5, 0x039C },
	{ 0x00E0, 0x00C0 },
	{ 0x00E1, 0x00C1 },
	{ 0x00E2, 0x00C2 },
	{ 0x00E3, 0x00C3 },
	{ 0x00E4, 0x00C4 },
	{ 0x00E5, 0x00C5 },
	{ 0x00E6, 0x00C6 },
	{ 0x00E7, 0x00C7 },
	{ 0x00E8, 0x00C8 },
	{ 0x00E9, 0x00C9 },
	{ 0x00EA, 0x00CA },
	{ 0x00EB, 0x00CB },
	{ 0x00EC, 0x00CC },
	{ 0x00ED, 0x00CD },
	{ 0x00EE, 0x00CE },
	{ 0x00EF, 0x00CF },
	{ 0x00F0, 0x00D0 },
	{ 0x00F1, 0x00D1 },
	{ 0x00F2, 0x00D2 },
	{ 0x00F3, 0x00D3 },
	{ 0x00F4, 0x00D4 },
	{ 0x00F5, 0x00D5 },
	{ 0x00F6, 0x00D6 },
	{ 0x00F8, 0x00D8 },
	{ 0x00F9, 0x00D9 },
	{ 0x00FA, 0x00DA },
	{ 0x00FB, 0x00DB },
	{ 0x00FC, 0x00DC },
	{ 0x00FD, 0x00DD },
	{ 0x00FE, 0x00DE },
	{ 0x00FF, 0x0178 },
	{ 0x0101, 0x0100 },
	{ 0x0103, 0x0102 },
	{ 0x0105, 0x0104 },
	{ 0x0107, 0x0106 },
	{ 0x0109, 0x0108 },
	{ 0x010B, 0x010A },
	{ 0x010D, 0x010C },
	{ 0x010F, 0x010E },
	{ 0x0111, 0x0110 },
	{ 0x0113, 0x0112 },
	{ 0x0115, 0x0114 },
	{ 0x0117, 0x0116 },
	{ 0x0119, 0x0118 },
	{ 0x011B, 0x011A },
	{ 0x011D, 0x011C },
	{ 0x011F, 0x011E },
	{ 0x0121, 0x0120 },
	{ 0x0123, 0x0122 },
	{ 0x0125, 0x0124 },
	{ 0x0127, 0x0126 },
	{ 0x0129, 0x0128 },
	{ 0x012B, 0x012A },
	{ 0x012D, 0x012C },
	{ 0x012F, 0x012E },
	{ 0x0131, 0x0049 },
	{ 0x0133, 0x0132 },
	{ 0x0135, 0x0134 },
	{ 0x0137, 0x0136 },
	{ 0x013A, 0x0139 },
	{ 0x013C, 0x013B },
	{ 0x013E, 0x013D },
	{ 0x0140, 0x013F },
	{ 0x0142, 0x0141 },
	{ 0x0144, 0x0143 },
	{ 0x0146, 0x0145 },
	{ 0x0148, 0x0147 },
	{ 0x014B, 0x014A },
	{ 0x014D, 0x014C },
	{ 0x014F, 0x014E },
	{ 0x0151, 0x0150 },
	{ 0x0153, 0x0152 },
	{ 0x0155, 0x0154 },
	{ 0x0157, 0x0156 },
	{ 0x0159, 0x0158 },
	{ 0x015B, 0x015A },
	{ 0x015D, 0x015C },
	{ 0x015F, 0x015E },
	{ 0x0161, 0x0160 },
	{ 0x0163, 0x0162 },
	{ 0x0165, 0x0164 },
	{ 0x0167, 0x0166 },
	{ 0x0169, 0x0168 },
	{ 0x016B, 0x016A },
	{ 0x016D, 0x016C },
	{ 0x016F, 0x016E },
	{ 0x0171, 0x0170 },
	{ 0x0173, 0x0172 },
	{ 0x0175, 0x0174 },
	{ 0x0177, 0x0176 },
	{ 0x017A, 0x0179 },
	{ 0x017C, 0x017B },
	{ 0x017E, 0x017D },
	{ 0x017F, 0x0053 },
	{ 0x0180, 0x0243 },
	{ 0x0183, 0x0182 },
	{ 0x0185, 0x0184 },
	{ 0x0188, 0x0187 },
	{ 0x018C, 0x018B },
	{ 0x0192, 0x0191 },
	{ 0x0195, 0x01F6 },
	{ 0x0199, 0x0198 },
	{ 0x019A, 0x023D },
	{ 0x019B, 0xA7DC },
	{ 0x019E, 0x0220 },
	{ 0x01A1, 0x01A0 },
	{ 0x01A3, 0x01A2 },
	{ 0x01A5, 0x01A4 },
	{ 0x01A8, 0x01A7 },
	{ 0x01AD, 0x01AC },
	{ 0x01B0, 0x01AF },
	{ 0x01B4, 0x01B3 },
	{ 0x01B6, 0x01B5 },
	{ 0x01B9, 0x01B8 },
	{ 0x01BD, 0x01BC },
	{ 0x01BF, 0x01F7 },
	{ 0x01C5, 0x01C4 },
	{ 0x01C6, 0x01C4 },
	{ 0x01C8, 0x01C7 },
	{ 0x01C9, 0x01C7 },
	{ 0x01CB, 0x01CA },
	{ 0x01CC, 0x01CA },
	{ 0x01CE, 0x01CD },
	{ 0x01D0, 0x01CF },
	{ 0x01D2, 0x01D1 },
	{ 0x01D4, 0x01D3 },
	{ 0x01D6, 0x01D5 },
	{ 0x01D8, 0x01D7 },
	{ 0x01DA, 0x01D9 },
	{ 0x01DC, 0x01DB },
	{ 0x01DD, 0x018E },
	{ 0x01DF, 0x01DE },
	{ 0x01E1, 0x01E0 },
	{ 0x01E3, 0x01E2 },
	{ 0x01E5, 0x01E4 },
	{ 0x01E7, 0x01E6 },
	{ 0x01E9, 0x01E8 },
	{ 0x01EB, 0x01EA },
	{ 0x01ED, 0x01EC },
	{ 0x01EF, 0x01EE },
	{ 0x01F2, 0x01F1 },
	{ 0x01F3, 0x01F1 },
	{ 0x01F5, 0x01F4 },
	{ 0x01F9, 0x01F8 },
	{ 0x01FB, 0x01FA },
	{ 0x01FD, 0x01FC },
	{ 0x01FF, 0x01FE },
	{ 0x0201, 0x0200 },
	{ 0x0203, 0x0202 },
	{ 0x0205, 0x0204 },
	{ 0x0207, 0x0206 },
	{ 0x0209, 0x0208 },
	{ 0x020B, 0x020A },
	{ 0x020D, 0x020C },
	{ 0x020F, 0x020E },
	{ 0x0211, 0x0210 },
	{ 0x0213, 0x0212 },
	{ 0x0215, 0x0214 },
	{ 0x0217, 0x0216 },
	{ 0x0219, 0x0218 },
	{ 0x021B, 0x021A },
	{ 0x021D, 0x021C },
	{ 0x021F, 0x021E },
	{ 0x0223, 0x0222 },
	{ 0x0225, 0x0224 },
	{ 0x0227, 0x0226 },
	{ 0x0229, 0x0228 },
	{ 0x022B, 0x022A },
	{ 0x022D, 0x022C },
	{ 0x022F, 0x022E },
	{ 0x0231, 0x0230 },
	{ 0x0233, 0x0232 },
	{ 0x023C, 0x023B },
	{ 0x023F, 0x2C7E },
	{ 0x0240, 0x2C7F },
	{ 0x0242, 0x0241 },
	{ 0x0247, 0x0246 },
	{ 0x0249, 0x0248 },
	{ 0x024B, 0x024A },
	{ 0x024D, 0x024C },
	{ 0x024F, 0x024E },
	{ 0x0250, 0x2C6F },
	{ 0x0251, 0x2C6D },
	{ 0x0252, 0x2C70 },
	{ 0x0253, 0x0181 },
	{ 0x0254, 0x0186 },
	{ 0x0256, 0x0189 },
	{ 0x0257, 0x018A },
	{ 0x0259, 0x018F },
	{ 0x025B, 0x0190 },
	{ 0x025C, 0xA7AB },
	{ 0x0260, 0x0193 },
	{ 0x0261, 0xA7AC },
	{ 0x0263, 0x0194 },
	{ 0x0264, 0xA7CB },
	{ 0x0265, 0xA78D },
	{ 0x0266, 0xA7AA },
	{ 0x0268, 0x0197 },
	{ 0x0269, 0x0196 },
	{ 0x026A, 0xA7AE },
	{ 0x026B, 0x2C62 },
	{ 0x026C, 0xA7AD },
	{ 0x026F, 0x019C },
	{ 0x0271, 0x2C6E },
	{ 0x0272, 0x019D },
	{ 0x0275, 0x019F },
	{ 0x027D, 0x2C64 },
	{ 0x0280, 0x01A6 },
	{ 0x0282, 0xA7C5 },
	{ 0x0283, 0x01A9 },
	{ 0x0287, 0xA7B1 },
	{ 0x0288, 0x01AE },
	{ 0x0289, 0x0244 },
	{ 0x028A, 0x01B1 },
	{ 0x028B, 0x01B2 },
	{ 0x028C, 0x0245 },
	{ 0x0292, 0x01B7 },
	{ 0x029D, 0xA7B2 },
	{ 0x029E, 0xA7B0 },
	{ 0x0345, 0x0399 },
	{ 0x0371, 0x0370 },
	{ 0x0373, 0x0372 },
	{ 0x0377, 0x0376 },
	{ 0x037B, 0x03FD },
	{ 0x037C, 0x03FE },
	{ 0x037D, 0x03FF },
	{ 0x03AC, 0x0386 },
	{ 0x03AD, 0x0388 },
	{ 0x03AE, 0x0389 },
	{ 0x03AF, 0x038A },
	{ 0x03B1, 0x0391 },
	{ 0x03B2, 0x0392 },
	{ 0x03B3, 0x0393 },
	{ 0x03B4, 0x0394 },
	{ 0x03B5, 0x0395 },
	{ 0x03B6, 0x0396 },
	{ 0x03B7, 0x0397 },
	{ 0x03B8, 0x0398 },
	{ 0x03B9, 0x0399 },
	{ 0x03BA, 0x039A },
	{ 0x03BB, 0x039B },
	{ 0x03BC, 0x039C },
	{ 0x03BD, 0x039D },
	{ 0x03BE, 0x039E },
	{ 0x03BF, 0x039F },
	{ 0x03C0, 0x03A0 },
	{ 0x03C1, 0x03A1 },
	{ 0x03C2, 0x03A3 },
	{ 0x03C3, 0x03A3 },
	{ 0x03C4, 0x03A4 },
	{ 0x03C5, 0x03A5 },
	{ 0x03C6, 0x03A6 },
	{ 0x03C7, 0x03A7 },
	{ 0x03C8, 0x03A8 },
	{ 0x03C9, 0x03A9 },
	{ 0x03CA, 0x03AA },
	{ 0x03CB, 0x03AB },
	{ 0x03CC, 0x038C },
	{ 0x03CD, 0x038E },
	{ 0x03CE, 0x038F },
	{ 0x03D0, 0x0392 },
	{ 0x03D1, 0x0398 },
	{ 0x03D5, 0x03A6 },
	{ 0x03D6, 0x03A0 },
	{ 0x03D7, 0x03CF },
	{ 0x03D9, 0x03D8 },
	{ 0x03DB, 0x03DA },
	{ 0x03DD, 0x03DC },
	{ 0x03DF, 0x03DE },
	{ 0x03E1, 0x03E0 },
	{ 0x03E3, 0x03E2 },
	{ 0x03E5, 0x03E4 },
	{ 0x03E7, 0x03E6 },
	{ 0x03E9, 0x03E8 },
	{ 0x03EB, 0x03EA },
	{ 0x03ED, 0x03EC },
	{ 0x03EF, 0x03EE },
	{ 0x03F0, 0x039A },
	{ 0x03F1, 0x03A1 },
	{ 0x03F2, 0x03F9 },
	{ 0x03F3, 0x037F },
	{ 0x03F5, 0x0395 },
	{ 0x03F8, 0x03F7 },
	{ 0x03FB, 0x03FA },
	{ 0x0430, 0x0410 },
	{ 0x0431, 0x0411 },
	{ 0x0432, 0x0412 },
	{ 0x0433, 0x0413 },
	{ 0x0434, 0x0414 },
	{ 0x0435, 0x0415 },
	{ 0x0436, 0x0416 },
	{ 0x0437, 0x0417 },
	{ 0x0438, 0x0418 },
	{ 0x0439, 0x0419 },
	{ 0x043A, 0x041A },
	{ 0x043B, 0x041B },
	{ 0x043C, 0x041C },
	{ 0x043D, 0x041D },
	{ 0x043E, 0x041E },
	{ 0x043F, 0x041F },
	{ 0x0440, 0x0420 },
	{ 0x0441, 0x0421 },
	{ 0x0442, 0x0422 },
	{ 0x0443, 0x0423 },
	{ 0x0444, 0x0424 },
	{ 0x0445, 0x0425 },
	{ 0x0446, 0x0426 },
	{ 0x0447, 0x0427 },
	{ 0x0448, 0x0428 },
	{ 0x0449, 0x0429 },
	{ 0x044A, 0x042A },
	{ 0x044B, 0x042B },
	{ 0x044C, 0x042C },
	{ 0x044D, 0x042D },
	{ 0x044E, 0x042E },
	{ 0x044F, 0x042F },
	{ 0x0450, 0x0400 },
	{ 0x0451, 0x0401 },
	{ 0x0452, 0x0402 },
	{ 0x0453, 0x0403 },
	{ 0x0454, 0x0404 },
	{ 0x0455, 0x0405 },
	{ 0x0456, 0x0406 },
	{ 0x0457, 0x0407 },
	{ 0x0458, 0x0408 },
	{ 0x0459, 0x0409 },
	{ 0x045A, 0x040A },
	{ 0x045B, 0x040B },
	{ 0x045C, 0x040C },
	{ 0x045D, 0x040D },
	{ 0x045E, 0x040E },
	{ 0x045F, 0x040F },
	{ 0x0461, 0x0460 },
	{ 0x0463, 0x0462 },
	{ 0x0465, 0x0464 },
	{ 0x0467, 0x0466 },
	{ 0x0469, 0x0468 },
	{ 0x046B, 0x046A },
	{ 0x046D, 0x046C },
	{ 0x046F, 0x046E },
	{ 0x0471, 0x0470 },
	{ 0x0473, 0x0472 },
	{ 0x0475, 0x0474 },
	{ 0x0477, 0x0476 },
	{ 0x0479, 0x0478 },
	{ 0x047B, 0x047A },
	{ 0x047D, 0x047C },
	{ 0x047F, 0x047E },
	{ 0x0481, 0x0480 },
	{ 0x048B, 0x048A },
	{ 0x048D, 0x048C },
	{ 0x048F, 0x048E },
	{ 0x0491, 0x0490 },
	{ 0x0493, 0x0492 },
	{ 0x0495, 0x0494 },
	{ 0x0497, 0x0496 },
	{ 0x0499, 0x0498 },
	{ 0x049B, 0x049A },
	{ 0x049D, 0x049C },
	{ 0x049F, 0x049E },
	{ 0x04A1, 0x04A0 },
	{ 0x04A3, 0x04A2 },
	{ 0x04A5, 0x04A4 },
	{ 0x04A7, 0x04A6 },
	{ 0x04A9, 0x04A8 },
	{ 0x04AB, 0x04AA },
	{ 0x04AD, 0x04AC },
	{ 0x04AF, 0x04AE },
	{ 0x04B1, 0x04B0 },
	{ 0x04B3, 0x04B2 },
	{ 0x04B5, 0x04B4 },
	{ 0x04B7, 0x04B6 },
	{ 0x04B9, 0x04B8 },
	{ 0x04BB, 0x04BA },
	{ 0x04BD, 0x04BC },
	{ 0x04BF, 0x04BE },
	{ 0x04C2, 0x04C1 },
	{ 0x04C4, 0x04C3 },
	{ 0x04C6, 0x04C5 },
	{ 0x04C8, 0x04C7 },
	{ 0x04CA, 0x04C9 },
	{ 0x04CC, 0x04CB },
	{ 0x04CE, 0x04CD },
	{ 0x04CF, 0x04C0 },
	{ 0x04D1, 0x04D0 },
	{ 0x04D3, 0x04D2 },
	{ 0x04D5, 0x04D4 },
	{ 0x04D7, 0x04D6 },
	{ 0x04D9, 0x04D8 },
	{ 0x04DB, 0x04DA },
	{ 0x04DD, 0x04DC },
	{ 0x04DF, 0x04DE },
	{ 0x04E1, 0x04E0 },
	{ 0x04E3, 0x04E2 },
	{ 0x04E5, 0x04E4 },
	{ 0x04E7, 0x04E6 },
	{ 0x04E9, 0x04E8 },
	{ 0x04EB, 0x04EA },
	{ 0x04ED, 0x04EC },
	{ 0x04EF, 0x04EE },
	{ 0x04F1, 0x04F0 },
	{ 0x04F3, 0x04F2 },
	{ 0x04F5, 0x04F4 },
	{ 0x04F7, 0x04F6 },
	{ 0x04F9, 0x04F8 },
	{ 0x04FB, 0x04FA },
	{ 0x04FD, 0x04FC },
	{ 0x04FF, 0x04FE },
	{ 0x0501, 0x0500 },
	{ 0x0503, 0x0502 },
	{ 0x0505, 0x0504 },
	{ 0x0507, 0x0506 },
	{ 0x0509, 0x0508 },
	{ 0x050B, 0x050A },
	{ 0x050D, 0x050C },
	{ 0x050F, 0x050E },
	{ 0x0511, 0x0510 },
	{ 0x0513, 0x0512 },
	{ 0x0515, 0x0514 },
	{ 0x0517, 0x0516 },
	{ 0x0519, 0x0518 },
	{ 0x051B, 0x051A },
	{ 0x051D, 0x051C },
	{ 0x051F, 0x051E },
	{ 0x0521, 0x0520 },
	{ 0x0523, 0x0522 },
	{ 0x0525, 0x0524 },
	{ 0x0527, 0x0526 },
	{ 0x0529, 0x0528 },
	{ 0x052B, 0x052A },
	{ 0x052D, 0x052C },
	{ 0x052F, 0x052E },
	{ 0x0561, 0x0531 },
	{ 0x0562, 0x0532 },
	{ 0x0563, 0x0533 },
	{ 0x0564, 0x0534 },
	{ 0x0565, 0x0535 },
	{ 0x0566, 0x0536 },
	{ 0x0567, 0x0537 },
	{ 0x0568, 0x0538 },
	{ 0x0569, 0x0539 },
	{ 0x056A, 0x053A },
	{ 0x056B, 0x053B },
	{ 0x056C, 0x053C },
	{ 0x056D, 0x053D },
	{ 0x056E, 0x053E },
	{ 0x056F, 0x053F },
	{ 0x0570, 0x0540 },
	{ 0x0571, 0x0541 },
	{ 0x0572, 0x0542 },
	{ 0x0573, 0x0543 },
	{ 0x0574, 0x0544 },
	{ 0x0575, 0x0545 },
	{ 0x0576, 0x0546 },
	{ 0x0577, 0x0547 },
	{ 0x0578, 0x0548 },
	{ 0x0579, 0x0549 },
	{ 0x057A, 0x054A },
	{ 0x057B, 0x054B },
	{ 0x057C, 0x054C },
	{ 0x057D, 0x054D },
	{ 0x057E, 0x054E },
	{ 0x057F, 0x054F },
	{ 0x0580, 0x0550 },
	{ 0x0581, 0x0551 },
	{ 0x0582, 0x0552 },
	{ 0x0583, 0x0553 },
	{ 0x0584, 0x0554 },
	{ 0x0585, 0x0555 },
	{ 0x0586, 0x0556 },
	{ 0x10D0, 0x1C90 },
	{ 0x10D1, 0x1C91 },
	{ 0x10D2, 0x1C92 },
	{ 0x10D3, 0x1C93 },
	{ 0x10D4, 0x1C94 },
	{ 0x10D5, 0x1C95 },
	{ 0x10D6, 0x1C96 },
	{ 0x10D7, 0x1C97 },
	{ 0x10D8, 0x1C98 },
	{ 0x10D9, 0x1C99 },
	{ 0x10DA, 0x1C9A },
	{ 0x10DB, 0x1C9B },
	{ 0x10DC, 0x1C9C },
	{ 0x10DD, 0x1C9D },
	{ 0x10DE, 0x1C9E },
	{ 0x10DF, 0x1C9F },
	{ 0x10E0, 0x1CA0 },
	{ 0x10E1, 0x1CA1 },
	{ 0x10E2, 0x1CA2 },
	{ 0x10E3, 0x1CA3 },
	{ 0x10E4, 0x1CA4 },
	{ 0x10E5, 0x1CA5 },
	{ 0x10E6, 0x1CA6 },
	{ 0x10E7, 0x1CA7 },
	{ 0x10E8, 0x1CA8 },
	{ 0x10E9, 0x1CA9 },
	{ 0x10EA, 0x1CAA },
	{ 0x10EB, 0x1CAB },
	{ 0x10EC, 0x1CAC },
	{ 0x10ED, 0x1CAD },
	{ 0x10EE, 0x1CAE },
	{ 0x10EF, 0x1CAF },
	{ 0x10F0, 0x1CB0 },
	{ 0x10F1, 0x1CB1 },
	{ 0x10F2, 0x1CB2 },
	{ 0x10F3, 0x1CB3 },
	{ 0x10F4, 0x1CB4 },
	{ 0x10F5, 0x1CB5 },
	{ 0x10F6, 0x1CB6 },
	{ 0x10F7, 0x1CB7 },
	{ 0x10F8, 0x1CB8 },
	{ 0x10F9, 0x1CB9 },
	{ 0x10FA, 0x1CBA },
	{ 0x10FD, 0x1CBD },
	{ 0x10FE, 0x1CBE },
	{ 0x10FF, 0x1CBF },
	{ 0x13F8, 0x13F0 },
	{ 0x13F9, 0x13F1 },
	{ 0x13FA, 0x13F2 },
	{ 0x13FB, 0x13F3 },
	{ 0x13FC, 0x13F4 },
	{ 0x13FD, 0x13F5 },
	{ 0x1C80, 0x0412 },
	{ 0x1C81, 0x0414 },
	{ 0x1C82, 0x041E },
	{ 0x1C83, 0x0421 },
	{ 0x1C84, 0x0422 },
	{ 0x1C85, 0x0422 },
	{ 0x1C86, 0x042A },
	{ 0x1C87, 0x0462 },
	{ 0x1C88, 0xA64A },
	{ 0x1C8A, 0x1C89 },
	{ 0x1D79, 0xA77D },
	{ 0x1D7D, 0x2C63 },
	{ 0x1D8E, 0xA7C6 },
	{ 0x1E01, 0x1E00 },
	{ 0x1E03, 0x1E02 },
	{ 0x1E05, 0x1E04 },
	{ 0x1E07, 0x1E06 },
	{ 0x1E09, 0x1E08 },
	{ 0x1E0B, 0x1E0A },
	{ 0x1E0D, 0x1E0C },
	{ 0x1E0F, 0x1E0E },
	{ 0x1E11, 0x1E10 },
	{ 0x1E13, 0x1E12 },
	{ 0x1E15, 0x1E14 },
	{ 0x1E17, 0x1E16 },
	{ 0x1E19, 0x1E18 },
	{ 0x1E1B, 0x1E1A },
	{ 0x1E1D, 0x1E1C },
	{ 0x1E1F, 0x1E1E },
	{ 0x1E21, 0x1E20 },
	{ 0x1E23, 0x1E22 },
	{ 0x1E25, 0x1E24 },
	{ 0x1E27, 0x1E26 },
	{ 0x1E29, 0x1E28 },
	{ 0x1E2B, 0x1E2A },
	{ 0x1E2D, 0x1E2C },
	{ 0x1E2F, 0x1E2E },
	{ 0x1E31, 0x1E30 },
	{ 0x1E33, 0x1E32 },
	{ 0x1E35, 0x1E34 },
	{ 0x1E37, 0x1E36 },
	{ 0x1E39, 0x1E38 },
	{ 0x1E3B, 0x1E3A },
	{ 0x1E3D, 0x1E3C },
	{ 0x1E3F, 0x1E3E },
	{ 0x1E41, 0x1E40 },
	{ 0x1E43, 0x1E42 },
	{ 0x1E45, 0x1E44 },
	{ 0x1E47, 0x1E46 },
	{ 0x1E49, 0x1E48 },
	{ 0x1E4B, 0x1E4A },
	{ 0x1E4D, 0x1E4C },
	{ 0x1E4F, 0x1E4E },
	{ 0x1E51, 0x1E50 },
	{ 0x1E53, 0x1E52 },
	{ 0x1E55, 0x1E54 },
	{ 0x1E57, 0x1E56 },
	{ 0x1E59, 0x1E58 },
	{ 0x1E5B, 0x1E5A },
	{ 0x1E5D, 0x1E5C },
	{ 0x1E5F, 0x1E5E },
	{ 0x1E61, 0x1E60 },
	{ 0x1E63, 0x1E62 },
	{ 0x1E65, 0x1E64 },
	{ 0x1E67, 0x1E66 },
	{ 0x1E69, 0x1E68 },
	{ 0x1E6B, 0x1E6A },
	{ 0x1E6D, 0x1E6C },
	{ 0x1E6F, 0x1E6E },
	{ 0x1E71, 0x1E70 },
	{ 0x1E73, 0x1E72 },
	{ 0x1E75, 0x1E74 },
	{ 0x1E77, 0x1E76 },
	{ 0x1E79, 0x1E78 },
	{ 0x1E7B, 0x1E7A },
	{ 0x1E7D, 0x1E7C },
	{ 0x1E7F, 0x1E7E },
	{ 0x1E81, 0x1E80 },
	{ 0x1E83, 0x1E82 },
	{ 0x1E85, 0x1E84 },
	{ 0x1E87, 0x1E86 },
	{ 0x1E89, 0x1E88 },
	{ 0x1E8B, 0x1E8A },
	{ 0x1E8D, 0x1E8C },
	{ 0x1E8F, 0x1E8E },
	{ 0x1E91, 0x1E90 },
	{ 0x1E93, 0x1E92 },
	{ 0x1E95, 0x1E94 },
	{ 0x1E9B, 0x1E60 },
	{ 0x1EA1, 0x1EA0 },
	{ 0x1EA3, 0x1EA2 },
	{ 0x1EA5, 0x1EA4 },
	{ 0x1EA7, 0x1EA6 },
	{ 0x1EA9, 0x1EA8 },
	{ 0x1EAB, 0x1EAA },
	{ 0x1EAD, 0x1EAC },
	{ 0x1EAF, 0x1EAE },
	{ 0x1EB1, 0x1EB0 },
	{ 0x1EB3, 0x1EB2 },
	{ 0x1EB5, 0x1EB4 },
	{ 0x1EB7, 0x1EB6 },
	{ 0x1EB9, 0x1EB8 },
	{ 0x1EBB, 0x1EBA },
	{ 0x1EBD, 0x1EBC },
	{ 0x1EBF, 0x1EBE },
	{ 0x1EC1, 0x1EC0 },
	{ 0x1EC3, 0x1EC2 },
	{ 0x1EC5, 0x1EC4 },
	{ 0x1EC7, 0x1EC6 },
	{ 0x1EC9, 0x1EC8 },
	{ 0x1ECB, 0x1ECA },
	{ 0x1ECD, 0x1ECC },
	{ 0x1ECF, 0x1ECE },
	{ 0x1ED1, 0x1ED0 },
	{ 0x1ED3, 0x1ED2 },
	{ 0x1ED5, 0x1ED4 },
	{ 0x1ED7, 0x1ED6 },
	{ 0x1ED9, 0x1ED8 },
	{ 0x1EDB, 0x1EDA },
	{ 0x1EDD, 0x1EDC },
	{ 0x1EDF, 0x1EDE },
	{ 0x1EE1, 0x1EE0 },
	{ 0x1EE3, 0x1EE2 },
	{ 0x1EE5, 0x1EE4 },
	{ 0x1EE7, 0x1EE6 },
	{ 0x1EE9, 0x1EE8 },
	{ 0x1EEB, 0x1EEA },
	{ 0x1EED, 0x1EEC },
	{ 0x1EEF, 0x1EEE },
	{ 0x1EF1, 0x1EF0 },
	{ 0x1EF3, 0x1EF2 },
	{ 0x1EF5, 0x1EF4 },
	{ 0x1EF7, 0x1EF6 },
	{ 0x1EF9, 0x1EF8 },
	{ 0x1EFB, 0x1EFA },
	{ 0x1EFD, 0x1EFC },
	{ 0x1EFF, 0x1EFE },
	{ 0x1F00, 0x1F08 },
	{ 0x1F01, 0x1F09 },
	{ 0x1F02, 0x1F0A },
	{ 0x1F03, 0x1F0B },
	{ 0x1F04, 0x1F0C },
	{ 0x1F05, 0x1F0D },
	{ 0x1F06, 0x1F0E },
	{ 0x1F07, 0x1F0F },
	{ 0x1F10, 0x1F18 },
	{ 0x1F11, 0x1F19 },
	{ 0x1F12, 0x1F1A },
	{ 0x1F13, 0x1F1B },
	{ 0x1F14, 0x1F1C },
	{ 0x1F15, 0x1F1D },
	{ 0x1F20, 0x1F28 },
	{ 0x1F21, 0x1F29 },
	{ 0x1F22, 0x1F2A },
	{ 0x1F23, 0x1F2B },
	{ 0x1F24, 0x1F2C },
	{ 0x1F25, 0x1F2D },
	{ 0x1F26, 0x1F2E },
	{ 0x1F27, 0x1F2F },
	{ 0x1F30, 0x1F38 },
	{ 0x1F31, 0x1F39 },
	{ 0x1F32, 0x1F3A },
	{ 0x1F33, 0x1F3B },
	{ 0x1F34, 0x1F3C },
	{ 0x1F35, 0x1F3D },
	{ 0x1F36, 0x1F3E },
	{ 0x1F37, 0x1F3F },
	{ 0x1F40, 0x1F48 },
	{ 0x1F41, 0x1F49 },
	{ 0x1F42, 0x1F4A },
	{ 0x1F43, 0x1F4B },
	{ 0x1F44, 0x1F4C },
	{ 0x1F45, 0x1F4D },
	{ 0x1F51, 0x1F59 },
	{ 0x1F53, 0x1F5B },
	{ 0x1F55, 0x1F5D },
	{ 0x1F57, 0x1F5F },
	{ 0x1F60, 0x1F68 },
	{ 0x1F61, 0x1F69 },
	{ 0x1F62, 0x1F6A },
	{ 0x1F63, 0x1F6B },
	{ 0x1F64, 0x1F6C },
	{ 0x1F65, 0x1F6D },
	{ 0x1F66, 0x1F6E },
	{ 0x1F67, 0x1F6F },
	{ 0x1F70, 0x1FBA },
	{ 0x1F71, 0x1FBB },
	{ 0x1F72, 0x1FC8 },
	{ 0x1F73, 0x1FC9 },
	{ 0x1F74, 0x1FCA },
	{ 0x1F75, 0x1FCB },
	{ 0x1F76, 0x1FDA },
	{ 0x1F77, 0x1FDB },
	{ 0x1F78, 0x1FF8 },
	{ 0x1F79, 0x1FF9 },
	{ 0x1F7A, 0x1FEA },
	{ 0x1F7B, 0x1FEB },
	{ 0x1F7C, 0x1FFA },
	{ 0x1F7D, 0x1FFB },
	{ 0x1F80, 0x1F88 },
	{ 0x1F81, 0x1F89 },
	{ 0x1F82, 0x1F8A },
	{ 0x1F83, 0x1F8B },
	{ 0x1F84, 0x1F8C },
	{ 0x1F85, 0x1F8D },
	{ 0x1F86, 0x1F8E },
	{ 0x1F87, 0x1F8F },
	{ 0x1F90, 0x1F98 },
	{ 0x1F91, 0x1F99 },
	{ 0x1F92, 0x1F9A },
	{ 0x1F93, 0x1F9B },
	{ 0x1F94, 0x1F9C },
	{ 0x1F95, 0x1F9D },
	{ 0x1F96, 0x1F9E },
	{ 0x1F97, 0x1F9F },
	{ 0x1FA0, 0x1FA8 },
	{ 0x1FA1, 0x1FA9 },
	{ 0x1FA2, 0x1FAA },
	{ 0x1FA3, 0x1FAB },
	{ 0x1FA4, 0x1FAC },
	{ 0x1FA5, 0x1FAD },
	{ 0x1FA6, 0x1FAE },
	{ 0x1FA7, 0x1FAF },
	{ 0x1FB0, 0x1FB8 },
	{ 0x1FB1, 0x1FB9 },
	{ 0x1FB3, 0x1FBC },
	{ 0x1FBE, 0x0399 },
	{ 0x1FC3, 0x1FCC },
	{ 0x1FD0, 0x1FD8 },
	{ 0x1FD1, 0x1FD9 },
	{ 0x1FE0, 0x1FE8 },
	{ 0x1FE1, 0x1FE9 },
	{ 0x1FE5, 0x1FEC },
	{ 0x1FF3, 0x1FFC },
	{ 0x214E, 0x2132 },
	{ 0x2170, 0x2160 },
	{ 0x2171, 0x2161 },
	{ 0x2172, 0x2162 },
	{ 0x2173, 0x2163 },
	{ 0x2174, 0x2164 },
	{ 0x2175, 0x2165 },
	{ 0x2176, 0x2166 },
	{ 0x2177, 0x2167 },
	{ 0x2178, 0x2168 },
	{ 0x2179, 0x2169 },
	{ 0x217A, 0x216A },
	{ 0x217B, 0x216B },
	{ 0x217C, 0x216C },
	{ 0x217D, 0x216D },
	{ 0x217E, 0x216E },
	{ 0x217F, 0x216F },
	{ 0x2184, 0x2183 },
	{ 0x24D0, 0x24B6 },
	{ 0x24D1, 0x24B7 },
	{ 0x24D2, 0x24B8 },
	{ 0x24D3, 0x24B9 },
	{ 0x24D4, 0x24BA },
	{ 0x24D5, 0x24BB },
	{ 0x24D6, 0x24BC },
	{ 0x24D7, 0x24BD },
	{ 0x24D8, 0x24BE },
	{ 0x24D9, 0x24BF },
	{ 0x24DA, 0x24C0 },
	{ 0x24DB, 0x24C1 },
	{ 0x24DC, 0x24C2 },
	{ 0x24DD, 0x24C3 },
	{ 0x24DE, 0x24C4 },
	{ 0x24DF, 0x24C5 },
	{ 0x24E0, 0x24C6 },
	{ 0x24E1, 0x24C7 },
	{ 0x24E2, 0x24C8 },
	{ 0x24E3, 0x24C9 },
	{ 0x24E4, 0x24CA },
	{ 0x24E5, 0x24CB },
	{ 0x24E6, 0x24CC },
	{ 0x24E7, 0x24CD },
	{ 0x24E8, 0x24CE },
	{ 0x24E9, 0x24CF },
	{ 0x2C30, 0x2C00 },
	{ 0x2C31, 0x2C01 },
	{ 0x2C32, 0x2C02 },
	{ 0x2C33, 0x2C03 },
	{ 0x2C34, 0x2C04 },
	{ 0x2C35, 0x2C05 },
	{ 0x2C36, 0x2C06 },
	{ 0x2C37, 0x2C07 },
	{ 0x2C38, 0x2C08 },
	{ 0x2C39, 0x2C09 },
	{ 0x2C3A, 0x2C0A },
	{ 0x2C3B, 0x2C0B },
	{ 0x2C3C, 0x2C0C },
	{ 0x2C3D, 0x2C0D },
	{ 0x2C3E, 0x2C0E },
	{ 0x2C3F, 0x2C0F },
	{ 0x2C40, 0x2C10 },
	{ 0x2C41, 0x2C11 },
	{ 0x2C42, 0x2C12 },
	{ 0x2C43, 0x2C13 },
	{ 0x2C44, 0x2C14 },
	{ 0x2C45, 0x2C15 },
	{ 0x2C46, 0x2C16 },
	{ 0x2C47, 0x2C17 },
	{ 0x2C48, 0x2C18 },
	{ 0x2C49, 0x2C19 },
	{ 0x2C4A, 0x2C1A },
	{ 0x2C4B, 0x2C1B },
	{ 0x2C4C, 0x2C1C },
	{ 0x2C4D, 0x2C1D },
	{ 0x2C4E, 0x2C1E },
	{ 0x2C4F, 0x2C1F },
	{ 0x2C50, 0x2C20 },
	{ 0x2C51, 0x2C21 },
	{ 0x2C52, 0x2C22 },
	{ 0x2C53, 0x2C23 },
	{ 0x2C54, 0x2C24 },
	{ 0x2C55, 0x2C25 },
	{ 0x2C56, 0x2C26 },
	{ 0x2C57, 0x2C27 },
	{ 0x2C58, 0x2C28 },
	{ 0x2C59, 0x2C29 },
	{ 0x2C5A, 0x2C2A },
	{ 0x2C5B, 0x2C2B },
	{ 0x2C5C, 0x2C2C },
	{ 0x2C5D, 0x2C2D },
	{ 0x2C5E, 0x2C2E },
	{ 0x2C5F, 0x2C2F },
	{ 0x2C61, 0x2C60 },
	{ 0x2C65, 0x023A },
	{ 0x2C66, 0x023E },
	{ 0x2C68, 0x2C67 },
	{ 0x2C6A, 0x2C69 },
	{ 0x2C6C, 0x2C6B },
	{ 0x2C73, 0x2C72 },
	{ 0x2C76, 0x2C75 },
	{ 0x2C81, 0x2C80 },
	{ 0x2C83, 0x2C82 },
	{ 0x2C85, 0x2C84 },
	{ 0x2C87, 0x2C86 },
	{ 0x2C89, 0x2C88 },
	{ 0x2C8B, 0x2C8A },
	{ 0x2C8D, 0x2C8C },
	{ 0x2C8F, 0x2C8E },
	{ 0x2C91, 0x2C90 },
	{ 0x2C93, 0x2C92 },
	{ 0x2C95, 0x2C94 },
	{ 0x2C97, 0x2C96 },
	{ 0x2C99, 0x2C98 },
	{ 0x2C9B, 0x2C9A },
	{ 0x2C9D, 0x2C9C },
	{ 0x2C9F, 0x2C9E },
	{ 0x2CA1, 0x2CA0 },
	{ 0x2CA3, 0x2CA2 },
	{ 0x2CA5, 0x2CA4 },
	{ 0x2CA7, 0x2CA6 },
	{ 0x2CA9, 0x2CA8 },
	{ 0x2CAB, 0x2CAA },
	{ 0x2CAD, 0x2CAC },
	{ 0x2CAF, 0x2CAE },
	{ 0x2CB1, 0x2CB0 },
	{ 0x2CB3, 0x2CB2 },
	{ 0x2CB5, 0x2CB4 },
	{ 0x2CB7, 0x2CB6 },
	{ 0x2CB9, 0x2CB8 },
	{ 0x2CBB, 0x2CBA },
	{ 0x2CBD, 0x2CBC },
	{ 0x2CBF, 0x2CBE },
	{ 0x2CC1, 0x2CC0 },
	{ 0x2CC3, 0x2CC2 },
	{ 0x2CC5, 0x2CC4 },
	{ 0x2CC7, 0x2CC6 },
	{ 0x2CC9, 0x2CC8 },
	{ 0x2CCB, 0x2CCA },
	{ 0x2CCD, 0x2CCC },
	{ 0x2CCF, 0x2CCE },
	{ 0x2CD1, 0x2CD0 },
	{ 0x2CD3, 0x2CD2 },
	{ 0x2CD5, 0x2CD4 },
	{ 0x2CD7, 0x2CD6 },
	{ 0x2CD9, 0x2CD8 },
	{ 0x2CDB, 0x2CDA },
	{ 0x2CDD, 0x2CDC },
	{ 0x2CDF, 0x2CDE },
	{ 0x2CE1, 0x2CE0 },
	{ 0x2CE3, 0x2CE2 },
	{ 0x2CEC, 0x2CEB },
	{ 0x2CEE, 0x2CED },
	{ 0x2CF3, 0x2CF2 },
	{ 0x2D00, 0x10A0 },
	{ 0x2D01, 0x10A1 },
	{ 0x2D02, 0x10A2 },
	{ 0x2D03, 0x10A3 },
	{ 0x2D04, 0x10A4 },
	{ 0x2D05, 0x10A5 },
	{ 0x2D06, 0x10A6 },
	{ 0x2D07, 0x10A7 },
	{ 0x2D08, 0x10A8 },
	{ 0x2D09, 0x10A9 },
	{ 0x2D0A, 0x10AA },
	{ 0x2D0B, 0x10AB },
	{ 0x2D0C, 0x10AC },
	{ 0x2D0D, 0x10AD },
	{ 0x2D0E, 0x10AE },
	{ 0x2D0F, 0x10AF },
	{ 0x2D10, 0x10B0 },
	{ 0x2D11, 0x10B1 },
	{ 0x2D12, 0x10B2 },
	{ 0x2D13, 0x10B3 },
	{ 0x2D14, 0x10B4 },
	{ 0x2D15, 0x10B5 },
	{ 0x2D16, 0x10B6 },
	{ 0x2D17, 0x10B7 },
	{ 0x2D18, 0x10B8 },
	{ 0x2D19, 0x10B9 },
	{ 0x2D1A, 0x10BA },
	{ 0x2D1B, 0x10BB },
	{ 0x2D1C, 0x10BC },
	{ 0x2D1D, 0x10BD },
	{ 0x2D1E, 0x10BE },
	{ 0x2D1F, 0x10BF },
	{ 0x2D20, 0x10C0 },
	{ 0x2D21, 0x10C1 },
	{ 0x2D22, 0x10C2 },
	{ 0x2D23, 0x10C3 },
	{ 0x2D24, 0x10C4 },
	{ 0x2D25, 0x10C5 },
	{ 0x2D27, 0x10C7 },
	{ 0x2D2D, 0x10CD },
	{ 0xA641, 0xA640 },
	{ 0xA643, 0xA642 },
	{ 0xA645, 0xA644 },
	{ 0xA647, 0xA646 },
	{ 0xA649, 0xA648 },
	{ 0xA64B, 0xA64A },
	{ 0xA64D, 0xA64C },
	{ 0xA64F, 0xA64E },
	{ 0xA651, 0xA650 },
	{ 0xA653, 0xA652 },
	{ 0xA655, 0xA654 },
	{ 0xA657, 0xA656 },
	{ 0xA659, 0xA658 },
	{ 0xA65B, 0xA65A },
	{ 0xA65D, 0xA65C },
	{ 0xA65F, 0xA65E },
	{ 0xA661, 0xA660 },
	{ 0xA663, 0xA662 },
	{ 0xA665, 0xA664 },
	{ 0xA667, 0xA666 },
	{ 0xA669, 0xA668 },
	{ 0xA66B, 0xA66A },
	{ 0xA66D, 0xA66C },
	{ 0xA681, 0xA680 },
	{ 0xA683, 0xA682 },
	{ 0xA685, 0xA684 },
	{ 0xA687, 0xA686 },
	{ 0xA689, 0xA688 },
	{ 0xA68B, 0xA68A },
	{ 0xA68D, 0xA68C },
	{ 0xA68F, 0xA68E },
	{ 0xA691, 0xA690 },
	{ 0xA693, 0xA692 },
	{ 0xA695, 0xA694 },
	{ 0xA697, 0xA696 },
	{ 0xA699, 0xA698 },
	{ 0xA69B, 0xA69A },
	{ 0xA723, 0xA722 },
	{ 0xA725, 0xA724 },
	{ 0xA727, 0xA726 },
	{ 0xA729, 0xA728 },
	{ 0xA72B, 0xA72A },
	{ 0xA72D, 0xA72C },
	{ 0xA72F, 0xA72E },
	{ 0xA733, 0xA732 },
	{ 0xA735, 0xA734 },
	{ 0xA737, 0xA736 },
	{ 0xA739, 0xA738 },
	{ 0xA73B, 0xA73A },
	{ 0xA73D, 0xA73C },
	{ 0xA73F, 0xA73E },
	{ 0xA741, 0xA740 },
	{ 0xA743, 0xA742 },
	{ 0xA745, 0xA744 },
	{ 0xA747, 0xA746 },
	{ 0xA749, 0xA748 },
	{ 0xA74B, 0xA74A },
	{ 0xA74D, 0xA74C },
	{ 0xA74F, 0xA74E },
	{ 0xA751, 0xA750 },
	{ 0xA753, 0xA752 },
	{ 0xA755, 0xA754 },
	{ 0xA757, 0xA756 },
	{ 0xA759, 0xA758 },
	{ 0xA75B, 0xA75A },
	{ 0xA75D, 0xA75C },
	{ 0xA75F, 0xA75E },
	{ 0xA761, 0xA760 },
	{ 0xA763, 0xA762 },
	{ 0xA765, 0xA764 },
	{ 0xA767, 0xA766 },
	{ 0xA769, 0xA768 },
	{ 0xA76B, 0xA76A },
	{ 0xA76D, 0xA76C },
	{ 0xA76F, 0xA76E },
	{ 0xA77A, 0xA779 },
	{ 0xA77C, 0xA77B },
	{ 0xA77F, 0xA77E },
	{ 0xA781, 0xA780 },
	{ 0xA783, 0xA782 },
	{ 0xA785, 0xA784 },
	{ 0xA787, 0xA786 },
	{ 0xA78C, 0xA78B },
	{ 0xA791, 0xA790 },
	{ 0xA793, 0xA792 },
	{ 0xA794, 0xA7C4 },
	{ 0xA797, 0xA796 },
	{ 0xA799, 0xA798 },
	{ 0xA79B, 0xA79A },
	{ 0xA79D, 0xA79C },
	{ 0xA79F, 0xA79E },
	{ 0xA7A1, 0xA7A0 },
	{ 0xA7A3, 0xA7A2 },
	{ 0xA7A5, 0xA7A4 },
	{ 0xA7A7, 0xA7A6 },
	{ 0xA7A9, 0xA7A8 },
	{ 0xA7B5, 0xA7B4 },
	{ 0xA7B7, 0xA7B6 },
	{ 0xA7B9, 0xA7B8 },
	{ 0xA7BB, 0xA7BA },
	{ 0xA7BD, 0xA7BC },
	{ 0xA7BF, 0xA7BE },
	{ 0xA7C1, 0xA7C0 },
	{ 0xA7C3, 0xA7C2 },
	{ 0xA7C8, 0xA7C7 },
	{ 0xA7CA, 0xA7C9 },
	{ 0xA7CD, 0xA7CC },
	{ 0xA7CF, 0xA7CE },
	{ 0xA7D1, 0xA7D0 },
	{ 0xA7D3, 0xA7D2 },
	{ 0xA7D5, 0xA7D4 },
	{ 0xA7D7, 0xA7D6 },
	{ 0xA7D9, 0xA7D8 },
	{ 0xA7DB, 0xA7DA },
	{ 0xA7F6, 0xA7F5 },
	{ 0xAB53, 0xA7B3 },
	{ 0xAB70, 0x13A0 },
	{ 0xAB71, 0x13A1 },
	{ 0xAB72, 0x13A2 },
	{ 0xAB73, 0x13A3 },
	{ 0xAB74, 0x13A4 },
	{ 0xAB75, 0x13A5 },
	{ 0xAB76, 0x13A6 },
	{ 0xAB77, 0x13A7 },
	{ 0xAB78, 0x13A8 },
	{ 0xAB79, 0x13A9 },
	{ 0xAB7A, 0x13AA },
	{ 0xAB7B, 0x13AB },
	{ 0xAB7C, 0x13AC },
	{ 0xAB7D, 0x13AD },
	{ 0xAB7E, 0x13AE },
	{ 0xAB7F, 0x13AF },
	{ 0xAB80, 0x13B0 },
	{ 0xAB81, 0x13B1 },
	{ 0xAB82, 0x13B2 },
	{ 0xAB83, 0x13B3 },
	{ 0xAB84, 0x13B4 },
	{ 0xAB85, 0x13B5 },
	{ 0xAB86, 0x13B6 },
	{ 0xAB87, 0x13B7 },
	{ 0xAB88, 0x13B8 },
	{ 0xAB89, 0x13B9 },
	{ 0xAB8A, 0x13BA },
	{ 0xAB8B, 0x13BB },
	{ 0xAB8C, 0x13BC },
	{ 0xAB8D, 0x13BD },
	{ 0xAB8E, 0x13BE },
	{ 0xAB8F, 0x13BF },
	{ 0xAB90, 0x13C0 },
	{ 0xAB91, 0x13C1 },
	{ 0xAB92, 0x13C2 },
	{ 0xAB93, 0x13C3 },
	{ 0xAB94, 0x13C4 },
	{ 0xAB95, 0x13C5 },
	{ 0xAB96, 0x13C6 },
	{ 0xAB97, 0x13C7 },
	{ 0xAB98, 0x13C8 },
	{ 0xAB99, 0x13C9 },
	{ 0xAB9A, 0x13CA },
	{ 0xAB9B, 0x13CB },
	{ 0xAB9C, 0x13CC },
	{ 0xAB9D, 0x13CD },
	{ 0xAB9E, 0x13CE },
	{ 0xAB9F, 0x13CF },
	{ 0xABA0, 0x13D0 },
	{ 0xABA1, 0x13D1 },
	{ 0xABA2, 0x13D2 },
	{ 0xABA3, 0x13D3 },
	{ 0xABA4, 0x13D4 },
	{ 0xABA5, 0x13D5 },
	{ 0xABA6, 0x13D6 },
	{ 0xABA7, 0x13D7 },
	{ 0xABA8, 0x13D8 },
	{ 0xABA9, 0x13D9 },
	{ 0xABAA, 0x13DA },
	{ 0xABAB, 0x13DB },
	{ 0xABAC, 0x13DC },
	{ 0xABAD, 0x13DD },
	{ 0xABAE, 0x13DE },
	{ 0xABAF, 0x13DF },
	{ 0xABB0, 0x13E0 },
	{ 0xABB1, 0x13E1 },
	{ 0xABB2, 0x13E2 },
	{ 0xABB3, 0x13E3 },
	{ 0xABB4, 0x13E4 },
	{ 0xABB5, 0x13E5 },
	{ 0xABB6, 0x13E6 },
	{ 0xABB7, 0x13E7 },
	{ 0xABB8, 0x13E8 },
	{ 0xABB9, 0x13E9 },
	{ 0xABBA, 0x13EA },
	{ 0xABBB, 0x13EB },
	{ 0xABBC, 0x13EC },
	{ 0xABBD, 0x13ED },
	{ 0xABBE, 0x13EE },
	{ 0xABBF, 0x13EF },
	{ 0xFF41, 0xFF21 },
	{ 0xFF42, 0xFF22 },
	{ 0xFF43, 0xFF23 },
	{ 0xFF44, 0xFF24 },
	{ 0xFF45, 0xFF25 },
	{ 0xFF46, 0xFF26 },
	{ 0xFF47, 0xFF27 },
	{ 0xFF48, 0xFF28 },
	{ 0xFF49, 0xFF29 },
	{ 0xFF4A, 0xFF2A },
	{ 0xFF4B, 0xFF2B },
	{ 0xFF4C, 0xFF2C },
	{ 0xFF4D, 0xFF2D },
	{ 0xFF4E, 0xFF2E },
	{ 0xFF4F, 0xFF2F },
	{ 0xFF50, 0xFF30 },
	{ 0xFF51, 0xFF31 },
	{ 0xFF52, 0xFF32 },
	{ 0xFF53, 0xFF33 },
	{ 0xFF54, 0xFF34 },
	{ 0xFF55, 0xFF35 },
	{ 0xFF56, 0xFF36 },
	{ 0xFF57, 0xFF37 },
	{ 0xFF58, 0xFF38 },
	{ 0xFF59, 0xFF39 },
	{ 0xFF5A, 0xFF3A },
	{ 0x10428, 0x10400 },
	{ 0x10429, 0x10401 },
	{ 0x1042A, 0x10402 },
	{ 0x1042B, 0x10403 },
	{ 0x1042C, 0x10404 },
	{ 0x1042D, 0x10405 },
	{ 0x1042E, 0x10406 },
	{ 0x1042F, 0x10407 },
	{ 0x10430, 0x10408 },
	{ 0x10431, 0x10409 },
	{ 0x10432, 0x1040A },
	{ 0x10433, 0x1040B },
	{ 0x10434, 0x1040C },
	{ 0x10435, 0x1040D },
	{ 0x10436, 0x1040E },
	{ 0x10437, 0x1040F },
	{ 0x10438, 0x10410 },
	{ 0x10439, 0x10411 },
	{ 0x1043A, 0x10412 },
	{ 0x1043B, 0x10413 },
	{ 0x1043C, 0x10414 },
	{ 0x1043D, 0x10415 },
	{ 0x1043E, 0x10416 },
	{ 0x1043F, 0x10417 },
	{ 0x10440, 0x10418 },
	{ 0x10441, 0x10419 },
	{ 0x10442, 0x1041A },
	{ 0x10443, 0x1041B },
	{ 0x10444, 0x1041C },
	{ 0x10445, 0x1041D },
	{ 0x10446, 0x1041E },
	{ 0x10447, 0x1041F },
	{ 0x10448, 0x10420 },
	{ 0x10449, 0x10421 },
	{ 0x1044A, 0x10422 },
	{ 0x1044B, 0x10423 },
	{ 0x1044C, 0x10424 },
	{ 0x1044D, 0x10425 },
	{ 0x1044E, 0x10426 },
	{ 0x1044F, 0x10427 },
	{ 0x104D8, 0x104B0 },
	{ 0x104D9, 0x104B1 },
	{ 0x104DA, 0x104B2 },
	{ 0x104DB, 0x104B3 },
	{ 0x104DC, 0x104B4 },
	{ 0x104DD, 0x104B5 },
	{ 0x104DE, 0x104B6 },
	{ 0x104DF, 0x104B7 },
	{ 0x104E0, 0x104B8 },
	{ 0x104E1, 0x104B9 },
	{ 0x104E2, 0x104BA },
	{ 0x104E3, 0x104BB },
	{ 0x104E4, 0x104BC },
	{ 0x104E5, 0x104BD },
	{ 0x104E6, 0x104BE },
	{ 0x104E7, 0x104BF },
	{ 0x104E8, 0x104C0 },
	{ 0x104E9, 0x104C1 },
	{ 0x104EA, 0x104C2 },
	{ 0x104EB, 0x104C3 },
	{ 0x104EC, 0x104C4 },
	{ 0x104ED, 0x104C5 },
	{ 0x104EE, 0x104C6 },
	{ 0x104EF, 0x104C7 },
	{ 0x104F0, 0x104C8 },
	{ 0x104F1, 0x104C9 },
	{ 0x104F2, 0x104CA },
	{ 0x104F3, 0x104CB },
	{ 0x104F4, 0x104CC },
	{ 0x104F5, 0x104CD },
	{ 0x104F6, 0x104CE },
	{ 0x104F7, 0x104CF },
	{ 0x104F8, 0x104D0 },
	{ 0x104F9, 0x104D1 },
	{ 0x104FA, 0x104D2 },
	{ 0x104FB, 0x104D3 },
	{ 0x10597, 0x10570 },
	{ 0x10598, 0x10571 },
	{ 0x10599, 0x10572 },
	{ 0x1059A, 0x10573 },
	{ 0x1059B, 0x10574 },
	{ 0x1059C, 0x10575 },
	{ 0x1059D, 0x10576 },
	{ 0x1059E, 0x10577 },
	{ 0x1059F, 0x10578 },
	{ 0x105A0, 0x10579 },
	{ 0x105A1, 0x1057A },
	{ 0x105A3, 0x1057C },
	{ 0x105A4, 0x1057D },
	{ 0x105A5, 0x1057E },
	{ 0x105A6, 0x1057F },
	{ 0x105A7, 0x10580 },
	{ 0x105A8, 0x10581 },
	{ 0x105A9, 0x10582 },
	{ 0x105AA, 0x10583 },
	{ 0x105AB, 0x10584 },
	{ 0x105AC, 0x10585 },
	{ 0x105AD, 0x10586 },
	{ 0x105AE, 0x10587 },
	{ 0x105AF, 0x10588 },
	{ 0x105B0, 0x10589 },
	{ 0x105B1, 0x1058A },
	{ 0x105B3, 0x1058C },
	{ 0x105B4, 0x1058D },
	{ 0x105B5, 0x1058E },
	{ 0x105B6, 0x1058F },
	{ 0x105B7, 0x10590 },
	{ 0x105B8, 0x10591 },
	{ 0x105B9, 0x10592 },
	{ 0x105BB, 0x10594 },
	{ 0x105BC, 0x10595 },
	{ 0x10CC0, 0x10C80 },
	{ 0x10CC1, 0x10C81 },
	{ 0x10CC2, 0x10C82 },
	{ 0x10CC3, 0x10C83 },
	{ 0x10CC4, 0x10C84 },
	{ 0x10CC5, 0x10C85 },
	{ 0x10CC6, 0x10C86 },
	{ 0x10CC7, 0x10C87 },
	{ 0x10CC8, 0x10C88 },
	{ 0x10CC9, 0x10C89 },
	{ 0x10CCA, 0x10C8A },
	{ 0x10CCB, 0x10C8B },
	{ 0x10CCC, 0x10C8C },
	{ 0x10CCD, 0x10C8D },
	{ 0x10CCE, 0x10C8E },
	{ 0x10CCF, 0x10C8F },
	{ 0x10CD0, 0x10C90 },
	{ 0x10CD1, 0x10C91 },
	{ 0x10CD2, 0x10C92 },
	{ 0x10CD3, 0x10C93 },
	{ 0x10CD4, 0x10C94 },
	{ 0x10CD5, 0x10C95 },
	{ 0x10CD6, 0x10C96 },
	{ 0x10CD7, 0x10C97 },
	{ 0x10CD8, 0x10C98 },
	{ 0x10CD9, 0x10C99 },
	{ 0x10CDA, 0x10C9A },
	{ 0x10CDB, 0x10C9B },
	{ 0x10CDC, 0x10C9C },
	{ 0x10CDD, 0x10C9D },
	{ 0x10CDE, 0x10C9E },
	{ 0x10CDF, 0x10C9F },
	{ 0x10CE0, 0x10CA0 },
	{ 0x10CE1, 0x10CA1 },
	{ 0x10CE2, 0x10CA2 },
	{ 0x10CE3, 0x10CA3 },
	{ 0x10CE4, 0x10CA4 },
	{ 0x10CE5, 0x10CA5 },
	{ 0x10CE6, 0x10CA6 },
	{ 0x10CE7, 0x10CA7 },
	{ 0x10CE8, 0x10CA8 },
	{ 0x10CE9, 0x10CA9 },
	{ 0x10CEA, 0x10CAA },
	{ 0x10CEB, 0x10CAB },
	{ 0x10CEC, 0x10CAC },
	{ 0x10CED, 0x10CAD },
	{ 0x10CEE, 0x10CAE },
	{ 0x10CEF, 0x10CAF },
	{ 0x10CF0, 0x10CB0 },
	{ 0x10CF1, 0x10CB1 },
	{ 0x10CF2, 0x10CB2 },
	{ 0x10D70, 0x10D50 },
	{ 0x10D71, 0x10D51 },
	{ 0x10D72, 0x10D52 },
	{ 0x10D73, 0x10D53 },
	{ 0x10D74, 0x10D54 },
	{ 0x10D75, 0x10D55 },
	{ 0x10D76, 0x10D56 },
	{ 0x10D77, 0x10D57 },
	{ 0x10D78, 0x10D58 },
	{ 0x10D79, 0x10D59 },
	{ 0x10D7A, 0x10D5A },
	{ 0x10D7B, 0x10D5B },
	{ 0x10D7C, 0x10D5C },
	{ 0x10D7D, 0x10D5D },
	{ 0x10D7E, 0x10D5E },
	{ 0x10D7F, 0x10D5F },
	{ 0x10D80, 0x10D60 },
	{ 0x10D81, 0x10D61 },
	{ 0x10D82, 0x10D62 },
	{ 0x10D83, 0x10D63 },
	{ 0x10D84, 0x10D64 },
	{ 0x10D85, 0x10D65 },
	{ 0x118C0, 0x118A0 },
	{ 0x118C1, 0x118A1 },
	{ 0x118C2, 0x118A2 },
	{ 0x118C3, 0x118A3 },
	{ 0x118C4, 0x118A4 },
	{ 0x118C5, 0x118A5 },
	{ 0x118C6, 0x118A6 },
	{ 0x118C7, 0x118A7 },
	{ 0x118C8, 0x118A8 },
	{ 0x118C9, 0x118A9 },
	{ 0x118CA, 0x118AA },
	{ 0x118CB, 0x118AB },
	{ 0x118CC, 0x118AC },
	{ 0x118CD, 0x118AD },
	{ 0x118CE, 0x118AE },
	{ 0x118CF, 0x118AF },
	{ 0x118D0, 0x118B0 },
	{ 0x118D1, 0x118B1 },
	{ 0x118D2, 0x118B2 },
	{ 0x118D3, 0x118B3 },
	{ 0x118D4, 0x118B4 },
	{ 0x118D5, 0x118B5 },
	{ 0x118D6, 0x118B6 },
	{ 0x118D7, 0x118B7 },
	{ 0x118D8, 0x118B8 },
	{ 0x118D9, 0x118B9 },
	{ 0x118DA, 0x118BA },
	{ 0x118DB, 0x118BB },
	{ 0x118DC, 0x118BC },
	{ 0x118DD, 0x118BD },
	{ 0x118DE, 0x118BE },
	{ 0x118DF, 0x118BF },
	{ 0x16E60, 0x16E40 },
	{ 0x16E61, 0x16E41 },
	{ 0x16E62, 0x16E42 },
	{ 0x16E63, 0x16E43 },
	{ 0x16E64, 0x16E44 },
	{ 0x16E65, 0x16E45 },
	{ 0x16E66, 0x16E46 },
	{ 0x16E67, 0x16E47 },
	{ 0x16E68, 0x16E48 },
	{ 0x16E69, 0x16E49 },
	{ 0x16E6A, 0x16E4A },
	{ 0x16E6B, 0x16E4B },
	{ 0x16E6C, 0x16E4C },
	{ 0x16E6D, 0x16E4D },
	{ 0x16E6E, 0x16E4E },
	{ 0x16E6F, 0x16E4F },
	{ 0x16E70, 0x16E50 },
	{ 0x16E71, 0x16E51 },
	{ 0x16E72, 0x16E52 },
	{ 0x16E73, 0x16E53 },
	{ 0x16E74, 0x16E54 },
	{ 0x16E75, 0x16E55 },
	{ 0x16E76, 0x16E56 },
	{ 0x16E77, 0x16E57 },
	{ 0x16E78, 0x16E58 },
	{ 0x16E79, 0x16E59 },
	{ 0x16E7A, 0x16E5A },
	{ 0x16E7B, 0x16E5B },
	{ 0x16E7C, 0x16E5C },
	{ 0x16E7D, 0x16E5D },
	{ 0x16E7E, 0x16E5E },
	{ 0x16E7F, 0x16E5F },
	{ 0x16EBB, 0x16EA0 },
	{ 0x16EBC, 0x16EA1 },
	{ 0x16EBD, 0x16EA2 },
	{ 0x16EBE, 0x16EA3 },
	{ 0x16EBF, 0x16EA4 },
	{ 0x16EC0, 0x16EA5 },
	{ 0x16EC1, 0x16EA6 },
	{ 0x16EC2, 0x16EA7 },
	{ 0x16EC3, 0x16EA8 },
	{ 0x16EC4, 0x16EA9 },
	{ 0x16EC5, 0x16EAA },
	{ 0x16EC6, 0x16EAB },
	{ 0x16EC7, 0x16EAC },
	{ 0x16EC8, 0x16EAD },
	{ 0x16EC9, 0x16EAE },
	{ 0x16ECA, 0x16EAF },
	{ 0x16ECB, 0x16EB0 },
	{ 0x16ECC, 0x16EB1 },
	{ 0x16ECD, 0x16EB2 },
	{ 0x16ECE, 0x16EB3 },
	{ 0x16ECF, 0x16EB4 },
	{ 0x16ED0, 0x16EB5 },
	{ 0x16ED1, 0x16EB6 },
	{ 0x16ED2, 0x16EB7 },
	{ 0x16ED3, 0x16EB8 },
	{ 0x1E922, 0x1E900 },
	{ 0x1E923, 0x1E901 },
	{ 0x1E924, 0x1E902 },
	{ 0x1E925, 0x1E903 },
	{ 0x1E926, 0x1E904 },
	{ 0x1E927, 0x1E905 },
	{ 0x1E928, 0x1E906 },
	{ 0x1E929, 0x1E907 },
	{ 0x1E92A, 0x1E908 },
	{ 0x1E92B, 0x1E909 },
	{ 0x1E92C, 0x1E90A },
	{ 0x1E92D, 0x1E90B },
	{ 0x1E92E, 0x1E90C },
	{ 0x1E92F, 0x1E90D },
	{ 0x1E930, 0x1E90E },
	{ 0x1E931, 0x1E90F },
	{ 0x1E932, 0x1E910 },
	{ 0x1E933, 0x1E911 },
	{ 0x1E934, 0x1E912 },
	{ 0x1E935, 0x1E913 },
	{ 0x1E936, 0x1E914 },
	{ 0x1E937, 0x1E915 },
	{ 0x1E938, 0x1E916 },
	{ 0x1E939, 0x1E917 },
	{ 0x1E93A, 0x1E918 },
	{ 0x1E93B, 0x1E919 },
	{ 0x1E93C, 0x1E91A },
	{ 0x1E93D, 0x1E91B },
	{ 0x1E93E, 0x1E91C },
	{ 0x1E93F, 0x1E91D },
	{ 0x1E940, 0x1E91E },
	{ 0x1E941, 0x1E91F },
	{ 0x1E942, 0x1E920 },
	{ 0x1E943, 0x1E921 },
};

static const int reverse_caps_table[UTL_LEN][2] = {
	{ 0x0041, 0x0061 },
	{ 0x0042, 0x0062 },
	{ 0x0043, 0x0063 },
	{ 0x0044, 0x0064 },
	{ 0x0045, 0x0065 },
	{ 0x0046, 0x0066 },
	{ 0x0047, 0x0067 },
	{ 0x0048, 0x0068 },
	{ 0x0049, 0x0069 },
	{ 0x004A, 0x006A },
	{ 0x004B, 0x006B },
	{ 0x004C, 0x006C },
	{ 0x004D, 0x006D },
	{ 0x004E, 0x006E },
	{ 0x004F, 0x006F },
	{ 0x0050, 0x0070 },
	{ 0x0051, 0x0071 },
	{ 0x0052, 0x0072 },
	{ 0x0053, 0x0073 },
	{ 0x0054, 0x0074 },
	{ 0x0055, 0x0075 },
	{ 0x0056, 0x0076 },
	{ 0x0057, 0x0077 },
	{ 0x0058, 0x0078 },
	{ 0x0059, 0x0079 },
	{ 0x005A, 0x007A },
	{ 0x00C0, 0x00E0 },
	{ 0x00C1, 0x00E1 },
	{ 0x00C2, 0x00E2 },
	{ 0x00C3, 0x00E3 },
	{ 0x00C4, 0x00E4 },
	{ 0x00C5, 0x00E5 },
	{ 0x00C6, 0x00E6 },
	{ 0x00C7, 0x00E7 },
	{ 0x00C8, 0x00E8 },
	{ 0x00C9, 0x00E9 },
	{ 0x00CA, 0x00EA },
	{ 0x00CB, 0x00EB },
	{ 0x00CC, 0x00EC },
	{ 0x00CD, 0x00ED },
	{ 0x00CE, 0x00EE },
	{ 0x00CF, 0x00EF },
	{ 0x00D0, 0x00F0 },
	{ 0x00D1, 0x00F1 },
	{ 0x00D2, 0x00F2 },
	{ 0x00D3, 0x00F3 },
	{ 0x00D4, 0x00F4 },
	{ 0x00D5, 0x00F5 },
	{ 0x00D6, 0x00F6 },
	{ 0x00D8, 0x00F8 },
	{ 0x00D9, 0x00F9 },
	{ 0x00DA, 0x00FA },
	{ 0x00DB, 0x00FB },
	{ 0x00DC, 0x00FC },
	{ 0x00DD, 0x00FD },
	{ 0x00DE, 0x00FE },
	{ 0x0100, 0x0101 },
	{ 0x0102, 0x0103 },
	{ 0x0104, 0x0105 },
	{ 0x0106, 0x0107 },
	{ 0x0108, 0x0109 },
	{ 0x010A, 0x010B },
	{ 0x010C, 0x010D },
	{ 0x010E, 0x010F },
	{ 0x0110, 0x0111 },
	{ 0x0112, 0x0113 },
	{ 0x0114, 0x0115 },
	{ 0x0116, 0x0117 },
	{ 0x0118, 0x0119 },
	{ 0x011A, 0x011B },
	{ 0x011C, 0x011D },
	{ 0x011E, 0x011F },
	{ 0x0120, 0x0121 },
	{ 0x0122, 0x0123 },
	{ 0x0124, 0x0125 },
	{ 0x0126, 0x0127 },
	{ 0x0128, 0x0129 },
	{ 0x012A, 0x012B },
	{ 0x012C, 0x012D },
	{ 0x012E, 0x012F },
	{ 0x0130, 0x0069 },
	{ 0x0132, 0x0133 },
	{ 0x0134, 0x0135 },
	{ 0x0136, 0x0137 },
	{ 0x0139, 0x013A },
	{ 0x013B, 0x013C },
	{ 0x013D, 0x013E },
	{ 0x013F, 0x0140 },
	{ 0x0141, 0x0142 },
	{ 0x0143, 0x0144 },
	{ 0x0145, 0x0146 },
	{ 0x0147, 0x0148 },
	{ 0x014A, 0x014B },
	{ 0x014C, 0x014D },
	{ 0x014E, 0x014F },
	{ 0x0150, 0x0151 },
	{ 0x0152, 0x0153 },
	{ 0x0154, 0x0155 },
	{ 0x0156, 0x0157 },
	{ 0x0158, 0x0159 },
	{ 0x015A, 0x015B },
	{ 0x015C, 0x015D },
	{ 0x015E, 0x015F },
	{ 0x0160, 0x0161 },
	{ 0x0162, 0x0163 },
	{ 0x0164, 0x0165 },
	{ 0x0166, 0x0167 },
	{ 0x0168, 0x0169 },
	{ 0x016A, 0x016B },
	{ 0x016C, 0x016D },
	{ 0x016E, 0x016F },
	{ 0x0170, 0x0171 },
	{ 0x0172, 0x0173 },
	{ 0x0174, 0x0175 },
	{ 0x0176, 0x0177 },
	{ 0x0178, 0x00FF },
	{ 0x0179, 0x017A },
	{ 0x017B, 0x017C },
	{ 0x017D, 0x017E },
	{ 0x0181, 0x0253 },
	{ 0x0182, 0x0183 },
	{ 0x0184, 0x0185 },
	{ 0x0186, 0x0254 },
	{ 0x0187, 0x0188 },
	{ 0x0189, 0x0256 },
	{ 0x018A, 0x0257 },
	{ 0x018B, 0x018C },
	{ 0x018E, 0x01DD },
	{ 0x018F, 0x0259 },
	{ 0x0190, 0x025B },
	{ 0x0191, 0x0192 },
	{ 0x0193, 0x0260 },
	{ 0x0194, 0x0263 },
	{ 0x0196, 0x0269 },
	{ 0x0197, 0x0268 },
	{ 0x0198, 0x0199 },
	{ 0x019C, 0x026F },
	{ 0x019D, 0x0272 },
	{ 0x019F, 0x0275 },
	{ 0x01A0, 0x01A1 },
	{ 0x01A2, 0x01A3 },
	{ 0x01A4, 0x01A5 },
	{ 0x01A6, 0x0280 },
	{ 0x01A7, 0x01A8 },
	{ 0x01A9, 0x0283 },
	{ 0x01AC, 0x01AD },
	{ 0x01AE, 0x0288 },
	{ 0x01AF, 0x01B0 },
	{ 0x01B1, 0x028A },
	{ 0x01B2, 0x028B },
	{ 0x01B3, 0x01B4 },
	{ 0x01B5, 0x01B6 },
	{ 0x01B7, 0x0292 },
	{ 0x01B8, 0x01B9 },
	{ 0x01BC, 0x01BD },
	{ 0x01C4, 0x01C6 },
	{ 0x01C5, 0x01C6 },
	{ 0x01C7, 0x01C9 },
	{ 0x01C8, 0x01C9 },
	{ 0x01CA, 0x01CC },
	{ 0x01CB, 0x01CC },
	{ 0x01CD, 0x01CE },
	{ 0x01CF, 0x01D0 },
	{ 0x01D1, 0x01D2 },
	{ 0x01D3, 0x01D4 },
	{ 0x01D5, 0x01D6 },
	{ 0x01D7, 0x01D8 },
	{ 0x01D9, 0x01DA },
	{ 0x01DB, 0x01DC },
	{ 0x01DE, 0x01DF },
	{ 0x01E0, 0x01E1 },
	{ 0x01E2, 0x01E3 },
	{ 0x01E4, 0x01E5 },
	{ 0x01E6, 0x01E7 },
	{ 0x01E8, 0x01E9 },
	{ 0x01EA, 0x01EB },
	{ 0x01EC, 0x01ED },
	{ 0x01EE, 0x01EF },
	{ 0x01F1, 0x01F3 },
	{ 0x01F2, 0x01F3 },
	{ 0x01F4, 0x01F5 },
	{ 0x01F6, 0x0195 },
	{ 0x01F7, 0x01BF },
	{ 0x01F8, 0x01F9 },
	{ 0x01FA, 0x01FB },
	{ 0x01FC, 0x01FD },
	{ 0x01FE, 0x01FF },
	{ 0x0200, 0x0201 },
	{ 0x0202, 0x0203 },
	{ 0x0204, 0x0205 },
	{ 0x0206, 0x0207 },
	{ 0x0208, 0x0209 },
	{ 0x020A, 0x020B },
	{ 0x020C, 0x020D },
	{ 0x020E, 0x020F },
	{ 0x0210, 0x0211 },
	{ 0x0212, 0x0213 },
	{ 0x0214, 0x0215 },
	{ 0x0216, 0x0217 },
	{ 0x0218, 0x0219 },
	{ 0x021A, 0x021B },
	{ 0x021C, 0x021D },
	{ 0x021E, 0x021F },
	{ 0x0220, 0x019E },
	{ 0x0222, 0x0223 },
	{ 0x0224, 0x0225 },
	{ 0x0226, 0x0227 },
	{ 0x0228, 0x0229 },
	{ 0x022A, 0x022B },
	{ 0x022C, 0x022D },
	{ 0x022E, 0x022F },
	{ 0x0230, 0x0231 },
	{ 0x0232, 0x0233 },
	{ 0x023A, 0x2C65 },
	{ 0x023B, 0x023C },
	{ 0x023D, 0x019A },
	{ 0x023E, 0x2C66 },
	{ 0x0241, 0x0242 },
	{ 0x0243, 0x0180 },
	{ 0x0244, 0x0289 },
	{ 0x0245, 0x028C },
	{ 0x0246, 0x0247 },
	{ 0x0248, 0x0249 },
	{ 0x024A, 0x024B },
	{ 0x024C, 0x024D },
	{ 0x024E, 0x024F },
	{ 0x0370, 0x0371 },
	{ 0x0372, 0x0373 },
	{ 0x0376, 0x0377 },
	{ 0x037F, 0x03F3 },
	{ 0x0386, 0x03AC },
	{ 0x0388, 0x03AD },
	{ 0x0389, 0x03AE },
	{ 0x038A, 0x03AF },
	{ 0x038C, 0x03CC },
	{ 0x038E, 0x03CD },
	{ 0x038F, 0x03CE },
	{ 0x0391, 0x03B1 },
	{ 0x0392, 0x03B2 },
	{ 0x0393, 0x03B3 },
	{ 0x0394, 0x03B4 },
	{ 0x0395, 0x03B5 },
	{ 0x0396, 0x03B6 },
	{ 0x0397, 0x03B7 },
	{ 0x0398, 0x03B8 },
	{ 0x0399, 0x03B9 },
	{ 0x039A, 0x03BA },
	{ 0x039B, 0x03BB },
	{ 0x039C, 0x03BC },
	{ 0x039D, 0x03BD },
	{ 0x039E, 0x03BE },
	{ 0x039F, 0x03BF },
	{ 0x03A0, 0x03C0 },
	{ 0x03A1, 0x03C1 },
	{ 0x03A3, 0x03C3 },
	{ 0x03A4, 0x03C4 },
	{ 0x03A5, 0x03C5 },
	{ 0x03A6, 0x03C6 },
	{ 0x03A7, 0x03C7 },
	{ 0x03A8, 0x03C8 },
	{ 0x03A9, 0x03C9 },
	{ 0x03AA, 0x03CA },
	{ 0x03AB, 0x03CB },
	{ 0x03CF, 0x03D7 },
	{ 0x03D8, 0x03D9 },
	{ 0x03DA, 0x03DB },
	{ 0x03DC, 0x03DD },
	{ 0x03DE, 0x03DF },
	{ 0x03E0, 0x03E1 },
	{ 0x03E2, 0x03E3 },
	{ 0x03E4, 0x03E5 },
	{ 0x03E6, 0x03E7 },
	{ 0x03E8, 0x03E9 },
	{ 0x03EA, 0x03EB },
	{ 0x03EC, 0x03ED },
	{ 0x03EE, 0x03EF },
	{ 0x03F4, 0x03B8 },
	{ 0x03F7, 0x03F8 },
	{ 0x03F9, 0x03F2 },
	{ 0x03FA, 0x03FB },
	{ 0x03FD, 0x037B },
	{ 0x03FE, 0x037C },
	{ 0x03FF, 0x037D },
	{ 0x0400, 0x0450 },
	{ 0x0401, 0x0451 },
	{ 0x0402, 0x0452 },
	{ 0x0403, 0x0453 },
	{ 0x0404, 0x0454 },
	{ 0x0405, 0x0455 },
	{ 0x0406, 0x0456 },
	{ 0x0407, 0x0457 },
	{ 0x0408, 0x0458 },
	{ 0x0409, 0x0459 },
	{ 0x040A, 0x045A },
	{ 0x040B, 0x045B },
	{ 0x040C, 0x045C },
	{ 0x040D, 0x045D },
	{ 0x040E, 0x045E },
	{ 0x040F, 0x045F },
	{ 0x0410, 0x0430 },
	{ 0x0411, 0x0431 },
	{ 0x0412, 0x0432 },
	{ 0x0413, 0x0433 },
	{ 0x0414, 0x0434 },
	{ 0x0415, 0x0435 },
	{ 0x0416, 0x0436 },
	{ 0x0417, 0x0437 },
	{ 0x0418, 0x0438 },
	{ 0x0419, 0x0439 },
	{ 0x041A, 0x043A },
	{ 0x041B, 0x043B },
	{ 0x041C, 0x043C },
	{ 0x041D, 0x043D },
	{ 0x041E, 0x043E },
	{ 0x041F, 0x043F },
	{ 0x0420, 0x0440 },
	{ 0x0421, 0x0441 },
	{ 0x0422, 0x0442 },
	{ 0x0423, 0x0443 },
	{ 0x0424, 0x0444 },
	{ 0x0425, 0x0445 },
	{ 0x0426, 0x0446 },
	{ 0x0427, 0x0447 },
	{ 0x0428, 0x0448 },
	{ 0x0429, 0x0449 },
	{ 0x042A, 0x044A },
	{ 0x042B, 0x044B },
	{ 0x042C, 0x044C },
	{ 0x042D, 0x044D },
	{ 0x042E, 0x044E },
	{ 0x042F, 0x044F },
	{ 0x0460, 0x0461 },
	{ 0x0462, 0x0463 },
	{ 0x0464, 0x0465 },
	{ 0x0466, 0x0467 },
	{ 0x0468, 0x0469 },
	{ 0x046A, 0x046B },
	{ 0x046C, 0x046D },
	{ 0x046E, 0x046F },
	{ 0x0470, 0x0471 },
	{ 0x0472, 0x0473 },
	{ 0x0474, 0x0475 },
	{ 0x0476, 0x0477 },
	{ 0x0478, 0x0479 },
	{ 0x047A, 0x047B },
	{ 0x047C, 0x047D },
	{ 0x047E, 0x047F },
	{ 0x0480, 0x0481 },
	{ 0x048A, 0x048B },
	{ 0x048C, 0x048D },
	{ 0x048E, 0x048F },
	{ 0x0490, 0x0491 },
	{ 0x0492, 0x0493 },
	{ 0x0494, 0x0495 },
	{ 0x0496, 0x0497 },
	{ 0x0498, 0x0499 },
	{ 0x049A, 0x049B },
	{ 0x049C, 0x049D },
	{ 0x049E, 0x049F },
	{ 0x04A0, 0x04A1 },
	{ 0x04A2, 0x04A3 },
	{ 0x04A4, 0x04A5 },
	{ 0x04A6, 0x04A7 },
	{ 0x04A8, 0x04A9 },
	{ 0x04AA, 0x04AB },
	{ 0x04AC, 0x04AD },
	{ 0x04AE, 0x04AF },
	{ 0x04B0, 0x04B1 },
	{ 0x04B2, 0x04B3 },
	{ 0x04B4, 0x04B5 },
	{ 0x04B6, 0x04B7 },
	{ 0x04B8, 0x04B9 },
	{ 0x04BA, 0x04BB },
	{ 0x04BC, 0x04BD },
	{ 0x04BE, 0x04BF },
	{ 0x04C0, 0x04CF },
	{ 0x04C1, 0x04C2 },
	{ 0x04C3, 0x04C4 },
	{ 0x04C5, 0x04C6 },
	{ 0x04C7, 0x04C8 },
	{ 0x04C9, 0x04CA },
	{ 0x04CB, 0x04CC },
	{ 0x04CD, 0x04CE },
	{ 0x04D0, 0x04D1 },
	{ 0x04D2, 0x04D3 },
	{ 0x04D4, 0x04D5 },
	{ 0x04D6, 0x04D7 },
	{ 0x04D8, 0x04D9 },
	{ 0x04DA, 0x04DB },
	{ 0x04DC, 0x04DD },
	{ 0x04DE, 0x04DF },
	{ 0x04E0, 0x04E1 },
	{ 0x04E2, 0x04E3 },
	{ 0x04E4, 0x04E5 },
	{ 0x04E6, 0x04E7 },
	{ 0x04E8, 0x04E9 },
	{ 0x04EA, 0x04EB },
	{ 0x04EC, 0x04ED },
	{ 0x04EE, 0x04EF },
	{ 0x04F0, 0x04F1 },
	{ 0x04F2, 0x04F3 },
	{ 0x04F4, 0x04F5 },
	{ 0x04F6, 0x04F7 },
	{ 0x04F8, 0x04F9 },
	{ 0x04FA, 0x04FB },
	{ 0x04FC, 0x04FD },
	{ 0x04FE, 0x04FF },
	{ 0x0500, 0x0501 },
	{ 0x0502, 0x0503 },
	{ 0x0504, 0x0505 },
	{ 0x0506, 0x0507 },
	{ 0x0508, 0x0509 },
	{ 0x050A, 0x050B },
	{ 0x050C, 0x050D },
	{ 0x050E, 0x050F },
	{ 0x0510, 0x0511 },
	{ 0x0512, 0x0513 },
	{ 0x0514, 0x0515 },
	{ 0x0516, 0x0517 },
	{ 0x0518, 0x0519 },
	{ 0x051A, 0x051B },
	{ 0x051C, 0x051D },
	{ 0x051E, 0x051F },
	{ 0x0520, 0x0521 },
	{ 0x0522, 0x0523 },
	{ 0x0524, 0x0525 },
	{ 0x0526, 0x0527 },
	{ 0x0528, 0x0529 },
	{ 0x052A, 0x052B },
	{ 0x052C, 0x052D },
	{ 0x052E, 0x052F },
	{ 0x0531, 0x0561 },
	{ 0x0532, 0x0562 },
	{ 0x0533, 0x0563 },
	{ 0x0534, 0x0564 },
	{ 0x0535, 0x0565 },
	{ 0x0536, 0x0566 },
	{ 0x0537, 0x0567 },
	{ 0x0538, 0x0568 },
	{ 0x0539, 0x0569 },
	{ 0x053A, 0x056A },
	{ 0x053B, 0x056B },
	{ 0x053C, 0x056C },
	{ 0x053D, 0x056D },
	{ 0x053E, 0x056E },
	{ 0x053F, 0x056F },
	{ 0x0540, 0x0570 },
	{ 0x0541, 0x0571 },
	{ 0x0542, 0x0572 },
	{ 0x0543, 0x0573 },
	{ 0x0544, 0x0574 },
	{ 0x0545, 0x0575 },
	{ 0x0546, 0x0576 },
	{ 0x0547, 0x0577 },
	{ 0x0548, 0x0578 },
	{ 0x0549, 0x0579 },
	{ 0x054A, 0x057A },
	{ 0x054B, 0x057B },
	{ 0x054C, 0x057C },
	{ 0x054D, 0x057D },
	{ 0x054E, 0x057E },
	{ 0x054F, 0x057F },
	{ 0x0550, 0x0580 },
	{ 0x0551, 0x0581 },
	{ 0x0552, 0x0582 },
	{ 0x0553, 0x0583 },
	{ 0x0554, 0x0584 },
	{ 0x0555, 0x0585 },
	{ 0x0556, 0x0586 },
	{ 0x10A0, 0x2D00 },
	{ 0x10A1, 0x2D01 },
	{ 0x10A2, 0x2D02 },
	{ 0x10A3, 0x2D03 },
	{ 0x10A4, 0x2D04 },
	{ 0x10A5, 0x2D05 },
	{ 0x10A6, 0x2D06 },
	{ 0x10A7, 0x2D07 },
	{ 0x10A8, 0x2D08 },
	{ 0x10A9, 0x2D09 },
	{ 0x10AA, 0x2D0A },
	{ 0x10AB, 0x2D0B },
	{ 0x10AC, 0x2D0C },
	{ 0x10AD, 0x2D0D },
	{ 0x10AE, 0x2D0E },
	{ 0x10AF, 0x2D0F },
	{ 0x10B0, 0x2D10 },
	{ 0x10B1, 0x2D11 },
	{ 0x10B2, 0x2D12 },
	{ 0x10B3, 0x2D13 },
	{ 0x10B4, 0x2D14 },
	{ 0x10B5, 0x2D15 },
	{ 0x10B6, 0x2D16 },
	{ 0x10B7, 0x2D17 },
	{ 0x10B8, 0x2D18 },
	{ 0x10B9, 0x2D19 },
	{ 0x10BA, 0x2D1A },
	{ 0x10BB, 0x2D1B },
	{ 0x10BC, 0x2D1C },
	{ 0x10BD, 0x2D1D },
	{ 0x10BE, 0x2D1E },
	{ 0x10BF, 0x2D1F },
	{ 0x10C0, 0x2D20 },
	{ 0x10C1, 0x2D21 },
	{ 0x10C2, 0x2D22 },
	{ 0x10C3, 0x2D23 },
	{ 0x10C4, 0x2D24 },
	{ 0x10C5, 0x2D25 },
	{ 0x10C7, 0x2D27 },
	{ 0x10CD, 0x2D2D },
	{ 0x13A0, 0xAB70 },
	{ 0x13A1, 0xAB71 },
	{ 0x13A2, 0xAB72 },
	{ 0x13A3, 0xAB73 },
	{ 0x13A4, 0xAB74 },
	{ 0x13A5, 0xAB75 },
	{ 0x13A6, 0xAB76 },
	{ 0x13A7, 0xAB77 },
	{ 0x13A8, 0xAB78 },
	{ 0x13A9, 0xAB79 },
	{ 0x13AA, 0xAB7A },
	{ 0x13AB, 0xAB7B },
	{ 0x13AC, 0xAB7C },
	{ 0x13AD, 0xAB7D },
	{ 0x13AE, 0xAB7E },
	{ 0x13AF, 0xAB7F },
	{ 0x13B0, 0xAB80 },
	{ 0x13B1, 0xAB81 },
	{ 0x13B2, 0xAB82 },
	{ 0x13B3, 0xAB83 },
	{ 0x13B4, 0xAB84 },
	{ 0x13B5, 0xAB85 },
	{ 0x13B6, 0xAB86 },
	{ 0x13B7, 0xAB87 },
	{ 0x13B8, 0xAB88 },
	{ 0x13B9, 0xAB89 },
	{ 0x13BA, 0xAB8A },
	{ 0x13BB, 0xAB8B },
	{ 0x13BC, 0xAB8C },
	{ 0x13BD, 0xAB8D },
	{ 0x13BE, 0xAB8E },
	{ 0x13BF, 0xAB8F },
	{ 0x13C0, 0xAB90 },
	{ 0x13C1, 0xAB91 },
	{ 0x13C2, 0xAB92 },
	{ 0x13C3, 0xAB93 },
	{ 0x13C4, 0xAB94 },
	{ 0x13C5, 0xAB95 },
	{ 0x13C6, 0xAB96 },
	{ 0x13C7, 0xAB97 },
	{ 0x13C8, 0xAB98 },
	{ 0x13C9, 0xAB99 },
	{ 0x13CA, 0xAB9A },
	{ 0x13CB, 0xAB9B },
	{ 0x13CC, 0xAB9C },
	{ 0x13CD, 0xAB9D },
	{ 0x13CE, 0xAB9E },
	{ 0x13CF, 0xAB9F },
	{ 0x13D0, 0xABA0 },
	{ 0x13D1, 0xABA1 },
	{ 0x13D2, 0xABA2 },
	{ 0x13D3, 0xABA3 },
	{ 0x13D4, 0xABA4 },
	{ 0x13D5, 0xABA5 },
	{ 0x13D6, 0xABA6 },
	{ 0x13D7, 0xABA7 },
	{ 0x13D8, 0xABA8 },
	{ 0x13D9, 0xABA9 },
	{ 0x13DA, 0xABAA },
	{ 0x13DB, 0xABAB },
	{ 0x13DC, 0xABAC },
	{ 0x13DD, 0xABAD },
	{ 0x13DE, 0xABAE },
	{ 0x13DF, 0xABAF },
	{ 0x13E0, 0xABB0 },
	{ 0x13E1, 0xABB1 },
	{ 0x13E2, 0xABB2 },
	{ 0x13E3, 0xABB3 },
	{ 0x13E4, 0xABB4 },
	{ 0x13E5, 0xABB5 },
	{ 0x13E6, 0xABB6 },
	{ 0x13E7, 0xABB7 },
	{ 0x13E8, 0xABB8 },
	{ 0x13E9, 0xABB9 },
	{ 0x13EA, 0xABBA },
	{ 0x13EB, 0xABBB },
	{ 0x13EC, 0xABBC },
	{ 0x13ED, 0xABBD },
	{ 0x13EE, 0xABBE },
	{ 0x13EF, 0xABBF },
	{ 0x13F0, 0x13F8 },
	{ 0x13F1, 0x13F9 },
	{ 0x13F2, 0x13FA },
	{ 0x13F3, 0x13FB },
	{ 0x13F4, 0x13FC },
	{ 0x13F5, 0x13FD },
	{ 0x1C89, 0x1C8A },
	{ 0x1C90, 0x10D0 },
	{ 0x1C91, 0x10D1 },
	{ 0x1C92, 0x10D2 },
	{ 0x1C93, 0x10D3 },
	{ 0x1C94, 0x10D4 },
	{ 0x1C95, 0x10D5 },
	{ 0x1C96, 0x10D6 },
	{ 0x1C97, 0x10D7 },
	{ 0x1C98, 0x10D8 },
	{ 0x1C99, 0x10D9 },
	{ 0x1C9A, 0x10DA },
	{ 0x1C9B, 0x10DB },
	{ 0x1C9C, 0x10DC },
	{ 0x1C9D, 0x10DD },
	{ 0x1C9E, 0x10DE },
	{ 0x1C9F, 0x10DF },
	{ 0x1CA0, 0x10E0 },
	{ 0x1CA1, 0x10E1 },
	{ 0x1CA2, 0x10E2 },
	{ 0x1CA3, 0x10E3 },
	{ 0x1CA4, 0x10E4 },
	{ 0x1CA5, 0x10E5 },
	{ 0x1CA6, 0x10E6 },
	{ 0x1CA7, 0x10E7 },
	{ 0x1CA8, 0x10E8 },
	{ 0x1CA9, 0x10E9 },
	{ 0x1CAA, 0x10EA },
	{ 0x1CAB, 0x10EB },
	{ 0x1CAC, 0x10EC },
	{ 0x1CAD, 0x10ED },
	{ 0x1CAE, 0x10EE },
	{ 0x1CAF, 0x10EF },
	{ 0x1CB0, 0x10F0 },
	{ 0x1CB1, 0x10F1 },
	{ 0x1CB2, 0x10F2 },
	{ 0x1CB3, 0x10F3 },
	{ 0x1CB4, 0x10F4 },
	{ 0x1CB5, 0x10F5 },
	{ 0x1CB6, 0x10F6 },
	{ 0x1CB7, 0x10F7 },
	{ 0x1CB8, 0x10F8 },
	{ 0x1CB9, 0x10F9 },
	{ 0x1CBA, 0x10FA },
	{ 0x1CBD, 0x10FD },
	{ 0x1CBE, 0x10FE },
	{ 0x1CBF, 0x10FF },
	{ 0x1E00, 0x1E01 },
	{ 0x1E02, 0x1E03 },
	{ 0x1E04, 0x1E05 },
	{ 0x1E06, 0x1E07 },
	{ 0x1E08, 0x1E09 },
	{ 0x1E0A, 0x1E0B },
	{ 0x1E0C, 0x1E0D },
	{ 0x1E0E, 0x1E0F },
	{ 0x1E10, 0x1E11 },
	{ 0x1E12, 0x1E13 },
	{ 0x1E14, 0x1E15 },
	{ 0x1E16, 0x1E17 },
	{ 0x1E18, 0x1E19 },
	{ 0x1E1A, 0x1E1B },
	{ 0x1E1C, 0x1E1D },
	{ 0x1E1E, 0x1E1F },
	{ 0x1E20, 0x1E21 },
	{ 0x1E22, 0x1E23 },
	{ 0x1E24, 0x1E25 },
	{ 0x1E26, 0x1E27 },
	{ 0x1E28, 0x1E29 },
	{ 0x1E2A, 0x1E2B },
	{ 0x1E2C, 0x1E2D },
	{ 0x1E2E, 0x1E2F },
	{ 0x1E30, 0x1E31 },
	{ 0x1E32, 0x1E33 },
	{ 0x1E34, 0x1E35 },
	{ 0x1E36, 0x1E37 },
	{ 0x1E38, 0x1E39 },
	{ 0x1E3A, 0x1E3B },
	{ 0x1E3C, 0x1E3D },
	{ 0x1E3E, 0x1E3F },
	{ 0x1E40, 0x1E41 },
	{ 0x1E42, 0x1E43 },
	{ 0x1E44, 0x1E45 },
	{ 0x1E46, 0x1E47 },
	{ 0x1E48, 0x1E49 },
	{ 0x1E4A, 0x1E4B },
	{ 0x1E4C, 0x1E4D },
	{ 0x1E4E, 0x1E4F },
	{ 0x1E50, 0x1E51 },
	{ 0x1E52, 0x1E53 },
	{ 0x1E54, 0x1E55 },
	{ 0x1E56, 0x1E57 },
	{ 0x1E58, 0x1E59 },
	{ 0x1E5A, 0x1E5B },
	{ 0x1E5C, 0x1E5D },
	{ 0x1E5E, 0x1E5F },
	{ 0x1E60, 0x1E61 },
	{ 0x1E62, 0x1E63 },
	{ 0x1E64, 0x1E65 },
	{ 0x1E66, 0x1E67 },
	{ 0x1E68, 0x1E69 },
	{ 0x1E6A, 0x1E6B },
	{ 0x1E6C, 0x1E6D },
	{ 0x1E6E, 0x1E6F },
	{ 0x1E70, 0x1E71 },
	{ 0x1E72, 0x1E73 },
	{ 0x1E74, 0x1E75 },
	{ 0x1E76, 0x1E77 },
	{ 0x1E78, 0x1E79 },
	{ 0x1E7A, 0x1E7B },
	{ 0x1E7C, 0x1E7D },
	{ 0x1E7E, 0x1E7F },
	{ 0x1E80, 0x1E81 },
	{ 0x1E82, 0x1E83 },
	{ 0x1E84, 0x1E85 },
	{ 0x1E86, 0x1E87 },
	{ 0x1E88, 0x1E89 },
	{ 0x1E8A, 0x1E8B },
	{ 0x1E8C, 0x1E8D },
	{ 0x1E8E, 0x1E8F },
	{ 0x1E90, 0x1E91 },
	{ 0x1E92, 0x1E93 },
	{ 0x1E94, 0x1E95 },
	{ 0x1E9E, 0x00DF },
	{ 0x1EA0, 0x1EA1 },
	{ 0x1EA2, 0x1EA3 },
	{ 0x1EA4, 0x1EA5 },
	{ 0x1EA6, 0x1EA7 },
	{ 0x1EA8, 0x1EA9 },
	{ 0x1EAA, 0x1EAB },
	{ 0x1EAC, 0x1EAD },
	{ 0x1EAE, 0x1EAF },
	{ 0x1EB0, 0x1EB1 },
	{ 0x1EB2, 0x1EB3 },
	{ 0x1EB4, 0x1EB5 },
	{ 0x1EB6, 0x1EB7 },
	{ 0x1EB8, 0x1EB9 },
	{ 0x1EBA, 0x1EBB },
	{ 0x1EBC, 0x1EBD },
	{ 0x1EBE, 0x1EBF },
	{ 0x1EC0, 0x1EC1 },
	{ 0x1EC2, 0x1EC3 },
	{ 0x1EC4, 0x1EC5 },
	{ 0x1EC6, 0x1EC7 },
	{ 0x1EC8, 0x1EC9 },
	{ 0x1ECA, 0x1ECB },
	{ 0x1ECC, 0x1ECD },
	{ 0x1ECE, 0x1ECF },
	{ 0x1ED0, 0x1ED1 },
	{ 0x1ED2, 0x1ED3 },
	{ 0x1ED4, 0x1ED5 },
	{ 0x1ED6, 0x1ED7 },
	{ 0x1ED8, 0x1ED9 },
	{ 0x1EDA, 0x1EDB },
	{ 0x1EDC, 0x1EDD },
	{ 0x1EDE, 0x1EDF },
	{ 0x1EE0, 0x1EE1 },
	{ 0x1EE2, 0x1EE3 },
	{ 0x1EE4, 0x1EE5 },
	{ 0x1EE6, 0x1EE7 },
	{ 0x1EE8, 0x1EE9 },
	{ 0x1EEA, 0x1EEB },
	{ 0x1EEC, 0x1EED },
	{ 0x1EEE, 0x1EEF },
	{ 0x1EF0, 0x1EF1 },
	{ 0x1EF2, 0x1EF3 },
	{ 0x1EF4, 0x1EF5 },
	{ 0x1EF6, 0x1EF7 },
	{ 0x1EF8, 0x1EF9 },
	{ 0x1EFA, 0x1EFB },
	{ 0x1EFC, 0x1EFD },
	{ 0x1EFE, 0x1EFF },
	{ 0x1F08, 0x1F00 },
	{ 0x1F09, 0x1F01 },
	{ 0x1F0A, 0x1F02 },
	{ 0x1F0B, 0x1F03 },
	{ 0x1F0C, 0x1F04 },
	{ 0x1F0D, 0x1F05 },
	{ 0x1F0E, 0x1F06 },
	{ 0x1F0F, 0x1F07 },
	{ 0x1F18, 0x1F10 },
	{ 0x1F19, 0x1F11 },
	{ 0x1F1A, 0x1F12 },
	{ 0x1F1B, 0x1F13 },
	{ 0x1F1C, 0x1F14 },
	{ 0x1F1D, 0x1F15 },
	{ 0x1F28, 0x1F20 },
	{ 0x1F29, 0x1F21 },
	{ 0x1F2A, 0x1F22 },
	{ 0x1F2B, 0x1F23 },
	{ 0x1F2C, 0x1F24 },
	{ 0x1F2D, 0x1F25 },
	{ 0x1F2E, 0x1F26 },
	{ 0x1F2F, 0x1F27 },
	{ 0x1F38, 0x1F30 },
	{ 0x1F39, 0x1F31 },
	{ 0x1F3A, 0x1F32 },
	{ 0x1F3B, 0x1F33 },
	{ 0x1F3C, 0x1F34 },
	{ 0x1F3D, 0x1F35 },
	{ 0x1F3E, 0x1F36 },
	{ 0x1F3F, 0x1F37 },
	{ 0x1F48, 0x1F40 },
	{ 0x1F49, 0x1F41 },
	{ 0x1F4A, 0x1F42 },
	{ 0x1F4B, 0x1F43 },
	{ 0x1F4C, 0x1F44 },
	{ 0x1F4D, 0x1F45 },
	{ 0x1F59, 0x1F51 },
	{ 0x1F5B, 0x1F53 },
	{ 0x1F5D, 0x1F55 },
	{ 0x1F5F, 0x1F57 },
	{ 0x1F68, 0x1F60 },
	{ 0x1F69, 0x1F61 },
	{ 0x1F6A, 0x1F62 },
	{ 0x1F6B, 0x1F63 },
	{ 0x1F6C, 0x1F64 },
	{ 0x1F6D, 0x1F65 },
	{ 0x1F6E, 0x1F66 },
	{ 0x1F6F, 0x1F67 },
	{ 0x1F88, 0x1F80 },
	{ 0x1F89, 0x1F81 },
	{ 0x1F8A, 0x1F82 },
	{ 0x1F8B, 0x1F83 },
	{ 0x1F8C, 0x1F84 },
	{ 0x1F8D, 0x1F85 },
	{ 0x1F8E, 0x1F86 },
	{ 0x1F8F, 0x1F87 },
	{ 0x1F98, 0x1F90 },
	{ 0x1F99, 0x1F91 },
	{ 0x1F9A, 0x1F92 },
	{ 0x1F9B, 0x1F93 },
	{ 0x1F9C, 0x1F94 },
	{ 0x1F9D, 0x1F95 },
	{ 0x1F9E, 0x1F96 },
	{ 0x1F9F, 0x1F97 },
	{ 0x1FA8, 0x1FA0 },
	{ 0x1FA9, 0x1FA1 },
	{ 0x1FAA, 0x1FA2 },
	{ 0x1FAB, 0x1FA3 },
	{ 0x1FAC, 0x1FA4 },
	{ 0x1FAD, 0x1FA5 },
	{ 0x1FAE, 0x1FA6 },
	{ 0x1FAF, 0x1FA7 },
	{ 0x1FB8, 0x1FB0 },
	{ 0x1FB9, 0x1FB1 },
	{ 0x1FBA, 0x1F70 },
	{ 0x1FBB, 0x1F71 },
	{ 0x1FBC, 0x1FB3 },
	{ 0x1FC8, 0x1F72 },
	{ 0x1FC9, 0x1F73 },
	{ 0x1FCA, 0x1F74 },
	{ 0x1FCB, 0x1F75 },
	{ 0x1FCC, 0x1FC3 },
	{ 0x1FD8, 0x1FD0 },
	{ 0x1FD9, 0x1FD1 },
	{ 0x1FDA, 0x1F76 },
	{ 0x1FDB, 0x1F77 },
	{ 0x1FE8, 0x1FE0 },
	{ 0x1FE9, 0x1FE1 },
	{ 0x1FEA, 0x1F7A },
	{ 0x1FEB, 0x1F7B },
	{ 0x1FEC, 0x1FE5 },
	{ 0x1FF8, 0x1F78 },
	{ 0x1FF9, 0x1F79 },
	{ 0x1FFA, 0x1F7C },
	{ 0x1FFB, 0x1F7D },
	{ 0x1FFC, 0x1FF3 },
	{ 0x2126, 0x03C9 },
	{ 0x212A, 0x006B },
	{ 0x212B, 0x00E5 },
	{ 0x2132, 0x214E },
	{ 0x2160, 0x2170 },
	{ 0x2161, 0x2171 },
	{ 0x2162, 0x2172 },
	{ 0x2163, 0x2173 },
	{ 0x2164, 0x2174 },
	{ 0x2165, 0x2175 },
	{ 0x2166, 0x2176 },
	{ 0x2167, 0x2177 },
	{ 0x2168, 0x2178 },
	{ 0x2169, 0x2179 },
	{ 0x216A, 0x217A },
	{ 0x216B, 0x217B },
	{ 0x216C, 0x217C },
	{ 0x216D, 0x217D },
	{ 0x216E, 0x217E },
	{ 0x216F, 0x217F },
	{ 0x2183, 0x2184 },
	{ 0x24B6, 0x24D0 },
	{ 0x24B7, 0x24D1 },
	{ 0x24B8, 0x24D2 },
	{ 0x24B9, 0x24D3 },
	{ 0x24BA, 0x24D4 },
	{ 0x24BB, 0x24D5 },
	{ 0x24BC, 0x24D6 },
	{ 0x24BD, 0x24D7 },
	{ 0x24BE, 0x24D8 },
	{ 0x24BF, 0x24D9 },
	{ 0x24C0, 0x24DA },
	{ 0x24C1, 0x24DB },
	{ 0x24C2, 0x24DC },
	{ 0x24C3, 0x24DD },
	{ 0x24C4, 0x24DE },
	{ 0x24C5, 0x24DF },
	{ 0x24C6, 0x24E0 },
	{ 0x24C7, 0x24E1 },
	{ 0x24C8, 0x24E2 },
	{ 0x24C9, 0x24E3 },
	{ 0x24CA, 0x24E4 },
	{ 0x24CB, 0x24E5 },
	{ 0x24CC, 0x24E6 },
	{ 0x24CD, 0x24E7 },
	{ 0x24CE, 0x24E8 },
	{ 0x24CF, 0x24E9 },
	{ 0x2C00, 0x2C30 },
	{ 0x2C01, 0x2C31 },
	{ 0x2C02, 0x2C32 },
	{ 0x2C03, 0x2C33 },
	{ 0x2C04, 0x2C34 },
	{ 0x2C05, 0x2C35 },
	{ 0x2C06, 0x2C36 },
	{ 0x2C07, 0x2C37 },
	{ 0x2C08, 0x2C38 },
	{ 0x2C09, 0x2C39 },
	{ 0x2C0A, 0x2C3A },
	{ 0x2C0B, 0x2C3B },
	{ 0x2C0C, 0x2C3C },
	{ 0x2C0D, 0x2C3D },
	{ 0x2C0E, 0x2C3E },
	{ 0x2C0F, 0x2C3F },
	{ 0x2C10, 0x2C40 },
	{ 0x2C11, 0x2C41 },
	{ 0x2C12, 0x2C42 },
	{ 0x2C13, 0x2C43 },
	{ 0x2C14, 0x2C44 },
	{ 0x2C15, 0x2C45 },
	{ 0x2C16, 0x2C46 },
	{ 0x2C17, 0x2C47 },
	{ 0x2C18, 0x2C48 },
	{ 0x2C19, 0x2C49 },
	{ 0x2C1A, 0x2C4A },
	{ 0x2C1B, 0x2C4B },
	{ 0x2C1C, 0x2C4C },
	{ 0x2C1D, 0x2C4D },
	{ 0x2C1E, 0x2C4E },
	{ 0x2C1F, 0x2C4F },
	{ 0x2C20, 0x2C50 },
	{ 0x2C21, 0x2C51 },
	{ 0x2C22, 0x2C52 },
	{ 0x2C23, 0x2C53 },
	{ 0x2C24, 0x2C54 },
	{ 0x2C25, 0x2C55 },
	{ 0x2C26, 0x2C56 },
	{ 0x2C27, 0x2C57 },
	{ 0x2C28, 0x2C58 },
	{ 0x2C29, 0x2C59 },
	{ 0x2C2A, 0x2C5A },
	{ 0x2C2B, 0x2C5B },
	{ 0x2C2C, 0x2C5C },
	{ 0x2C2D, 0x2C5D },
	{ 0x2C2E, 0x2C5E },
	{ 0x2C2F, 0x2C5F },
	{ 0x2C60, 0x2C61 },
	{ 0x2C62, 0x026B },
	{ 0x2C63, 0x1D7D },
	{ 0x2C64, 0x027D },
	{ 0x2C67, 0x2C68 },
	{ 0x2C69, 0x2C6A },
	{ 0x2C6B, 0x2C6C },
	{ 0x2C6D, 0x0251 },
	{ 0x2C6E, 0x0271 },
	{ 0x2C6F, 0x0250 },
	{ 0x2C70, 0x0252 },
	{ 0x2C72, 0x2C73 },
	{ 0x2C75, 0x2C76 },
	{ 0x2C7E, 0x023F },
	{ 0x2C7F, 0x0240 },
	{ 0x2C80, 0x2C81 },
	{ 0x2C82, 0x2C83 },
	{ 0x2C84, 0x2C85 },
	{ 0x2C86, 0x2C87 },
	{ 0x2C88, 0x2C89 },
	{ 0x2C8A, 0x2C8B },
	{ 0x2C8C, 0x2C8D },
	{ 0x2C8E, 0x2C8F },
	{ 0x2C90, 0x2C91 },
	{ 0x2C92, 0x2C93 },
	{ 0x2C94, 0x2C95 },
	{ 0x2C96, 0x2C97 },
	{ 0x2C98, 0x2C99 },
	{ 0x2C9A, 0x2C9B },
	{ 0x2C9C, 0x2C9D },
	{ 0x2C9E, 0x2C9F },
	{ 0x2CA0, 0x2CA1 },
	{ 0x2CA2, 0x2CA3 },
	{ 0x2CA4, 0x2CA5 },
	{ 0x2CA6, 0x2CA7 },
	{ 0x2CA8, 0x2CA9 },
	{ 0x2CAA, 0x2CAB },
	{ 0x2CAC, 0x2CAD },
	{ 0x2CAE, 0x2CAF },
	{ 0x2CB0, 0x2CB1 },
	{ 0x2CB2, 0x2CB3 },
	{ 0x2CB4, 0x2CB5 },
	{ 0x2CB6, 0x2CB7 },
	{ 0x2CB8, 0x2CB9 },
	{ 0x2CBA, 0x2CBB },
	{ 0x2CBC, 0x2CBD },
	{ 0x2CBE, 0x2CBF },
	{ 0x2CC0, 0x2CC1 },
	{ 0x2CC2, 0x2CC3 },
	{ 0x2CC4, 0x2CC5 },
	{ 0x2CC6, 0x2CC7 },
	{ 0x2CC8, 0x2CC9 },
	{ 0x2CCA, 0x2CCB },
	{ 0x2CCC, 0x2CCD },
	{ 0x2CCE, 0x2CCF },
	{ 0x2CD0, 0x2CD1 },
	{ 0x2CD2, 0x2CD3 },
	{ 0x2CD4, 0x2CD5 },
	{ 0x2CD6, 0x2CD7 },
	{ 0x2CD8, 0x2CD9 },
	{ 0x2CDA, 0x2CDB },
	{ 0x2CDC, 0x2CDD },
	{ 0x2CDE, 0x2CDF },
	{ 0x2CE0, 0x2CE1 },
	{ 0x2CE2, 0x2CE3 },
	{ 0x2CEB, 0x2CEC },
	{ 0x2CED, 0x2CEE },
	{ 0x2CF2, 0x2CF3 },
	{ 0xA640, 0xA641 },
	{ 0xA642, 0xA643 },
	{ 0xA644, 0xA645 },
	{ 0xA646, 0xA647 },
	{ 0xA648, 0xA649 },
	{ 0xA64A, 0xA64B },
	{ 0xA64C, 0xA64D },
	{ 0xA64E, 0xA64F },
	{ 0xA650, 0xA651 },
	{ 0xA652, 0xA653 },
	{ 0xA654, 0xA655 },
	{ 0xA656, 0xA657 },
	{ 0xA658, 0xA659 },
	{ 0xA65A, 0xA65B },
	{ 0xA65C, 0xA65D },
	{ 0xA65E, 0xA65F },
	{ 0xA660, 0xA661 },
	{ 0xA662, 0xA663 },
	{ 0xA664, 0xA665 },
	{ 0xA666, 0xA667 },
	{ 0xA668, 0xA669 },
	{ 0xA66A, 0xA66B },
	{ 0xA66C, 0xA66D },
	{ 0xA680, 0xA681 },
	{ 0xA682, 0xA683 },
	{ 0xA684, 0xA685 },
	{ 0xA686, 0xA687 },
	{ 0xA688, 0xA689 },
	{ 0xA68A, 0xA68B },
	{ 0xA68C, 0xA68D },
	{ 0xA68E, 0xA68F },
	{ 0xA690, 0xA691 },
	{ 0xA692, 0xA693 },
	{ 0xA694, 0xA695 },
	{ 0xA696, 0xA697 },
	{ 0xA698, 0xA699 },
	{ 0xA69A, 0xA69B },
	{ 0xA722, 0xA723 },
	{ 0xA724, 0xA725 },
	{ 0xA726, 0xA727 },
	{ 0xA728, 0xA729 },
	{ 0xA72A, 0xA72B },
	{ 0xA72C, 0xA72D },
	{ 0xA72E, 0xA72F },
	{ 0xA732, 0xA733 },
	{ 0xA734, 0xA735 },
	{ 0xA736, 0xA737 },
	{ 0xA738, 0xA739 },
	{ 0xA73A, 0xA73B },
	{ 0xA73C, 0xA73D },
	{ 0xA73E, 0xA73F },
	{ 0xA740, 0xA741 },
	{ 0xA742, 0xA743 },
	{ 0xA744, 0xA745 },
	{ 0xA746, 0xA747 },
	{ 0xA748, 0xA749 },
	{ 0xA74A, 0xA74B },
	{ 0xA74C, 0xA74D },
	{ 0xA74E, 0xA74F },
	{ 0xA750, 0xA751 },
	{ 0xA752, 0xA753 },
	{ 0xA754, 0xA755 },
	{ 0xA756, 0xA757 },
	{ 0xA758, 0xA759 },
	{ 0xA75A, 0xA75B },
	{ 0xA75C, 0xA75D },
	{ 0xA75E, 0xA75F },
	{ 0xA760, 0xA761 },
	{ 0xA762, 0xA763 },
	{ 0xA764, 0xA765 },
	{ 0xA766, 0xA767 },
	{ 0xA768, 0xA769 },
	{ 0xA76A, 0xA76B },
	{ 0xA76C, 0xA76D },
	{ 0xA76E, 0xA76F },
	{ 0xA779, 0xA77A },
	{ 0xA77B, 0xA77C },
	{ 0xA77D, 0x1D79 },
	{ 0xA77E, 0xA77F },
	{ 0xA780, 0xA781 },
	{ 0xA782, 0xA783 },
	{ 0xA784, 0xA785 },
	{ 0xA786, 0xA787 },
	{ 0xA78B, 0xA78C },
	{ 0xA78D, 0x0265 },
	{ 0xA790, 0xA791 },
	{ 0xA792, 0xA793 },
	{ 0xA796, 0xA797 },
	{ 0xA798, 0xA799 },
	{ 0xA79A, 0xA79B },
	{ 0xA79C, 0xA79D },
	{ 0xA79E, 0xA79F },
	{ 0xA7A0, 0xA7A1 },
	{ 0xA7A2, 0xA7A3 },
	{ 0xA7A4, 0xA7A5 },
	{ 0xA7A6, 0xA7A7 },
	{ 0xA7A8, 0xA7A9 },
	{ 0xA7AA, 0x0266 },
	{ 0xA7AB, 0x025C },
	{ 0xA7AC, 0x0261 },
	{ 0xA7AD, 0x026C },
	{ 0xA7AE, 0x026A },
	{ 0xA7B0, 0x029E },
	{ 0xA7B1, 0x0287 },
	{ 0xA7B2, 0x029D },
	{ 0xA7B3, 0xAB53 },
	{ 0xA7B4, 0xA7B5 },
	{ 0xA7B6, 0xA7B7 },
	{ 0xA7B8, 0xA7B9 },
	{ 0xA7BA, 0xA7BB },
	{ 0xA7BC, 0xA7BD },
	{ 0xA7BE, 0xA7BF },
	{ 0xA7C0, 0xA7C1 },
	{ 0xA7C2, 0xA7C3 },
	{ 0xA7C4, 0xA794 },
	{ 0xA7C5, 0x0282 },
	{ 0xA7C6, 0x1D8E },
	{ 0xA7C7, 0xA7C8 },
	{ 0xA7C9, 0xA7CA },
	{ 0xA7CB, 0x0264 },
	{ 0xA7CC, 0xA7CD },
	{ 0xA7CE, 0xA7CF },
	{ 0xA7D0, 0xA7D1 },
	{ 0xA7D2, 0xA7D3 },
	{ 0xA7D4, 0xA7D5 },
	{ 0xA7D6, 0xA7D7 },
	{ 0xA7D8, 0xA7D9 },
	{ 0xA7DA, 0xA7DB },
	{ 0xA7DC, 0x019B },
	{ 0xA7F5, 0xA7F6 },
	{ 0xFF21, 0xFF41 },
	{ 0xFF22, 0xFF42 },
	{ 0xFF23, 0xFF43 },
	{ 0xFF24, 0xFF44 },
	{ 0xFF25, 0xFF45 },
	{ 0xFF26, 0xFF46 },
	{ 0xFF27, 0xFF47 },
	{ 0xFF28, 0xFF48 },
	{ 0xFF29, 0xFF49 },
	{ 0xFF2A, 0xFF4A },
	{ 0xFF2B, 0xFF4B },
	{ 0xFF2C, 0xFF4C },
	{ 0xFF2D, 0xFF4D },
	{ 0xFF2E, 0xFF4E },
	{ 0xFF2F, 0xFF4F },
	{ 0xFF30, 0xFF50 },
	{ 0xFF31, 0xFF51 },
	{ 0xFF32, 0xFF52 },
	{ 0xFF33, 0xFF53 },
	{ 0xFF34, 0xFF54 },
	{ 0xFF35, 0xFF55 },
	{ 0xFF36, 0xFF56 },
	{ 0xFF37, 0xFF57 },
	{ 0xFF38, 0xFF58 },
	{ 0xFF39, 0xFF59 },
	{ 0xFF3A, 0xFF5A },
	{ 0x10400, 0x10428 },
	{ 0x10401, 0x10429 },
	{ 0x10402, 0x1042A },
	{ 0x10403, 0x1042B },
	{ 0x10404, 0x1042C },
	{ 0x10405, 0x1042D },
	{ 0x10406, 0x1042E },
	{ 0x10407, 0x1042F },
	{ 0x10408, 0x10430 },
	{ 0x10409, 0x10431 },
	{ 0x1040A, 0x10432 },
	{ 0x1040B, 0x10433 },
	{ 0x1040C, 0x10434 },
	{ 0x1040D, 0x10435 },
	{ 0x1040E, 0x10436 },
	{ 0x1040F, 0x10437 },
	{ 0x10410, 0x10438 },
	{ 0x10411, 0x10439 },
	{ 0x10412, 0x1043A },
	{ 0x10413, 0x1043B },
	{ 0x10414, 0x1043C },
	{ 0x10415, 0x1043D },
	{ 0x10416, 0x1043E },
	{ 0x10417, 0x1043F },
	{ 0x10418, 0x10440 },
	{ 0x10419, 0x10441 },
	{ 0x1041A, 0x10442 },
	{ 0x1041B, 0x10443 },
	{ 0x1041C, 0x10444 },
	{ 0x1041D, 0x10445 },
	{ 0x1041E, 0x10446 },
	{ 0x1041F, 0x10447 },
	{ 0x10420, 0x10448 },
	{ 0x10421, 0x10449 },
	{ 0x10422, 0x1044A },
	{ 0x10423, 0x1044B },
	{ 0x10424, 0x1044C },
	{ 0x10425, 0x1044D },
	{ 0x10426, 0x1044E },
	{ 0x10427, 0x1044F },
	{ 0x104B0, 0x104D8 },
	{ 0x104B1, 0x104D9 },
	{ 0x104B2, 0x104DA },
	{ 0x104B3, 0x104DB },
	{ 0x104B4, 0x104DC },
	{ 0x104B5, 0x104DD },
	{ 0x104B6, 0x104DE },
	{ 0x104B7, 0x104DF },
	{ 0x104B8, 0x104E0 },
	{ 0x104B9, 0x104E1 },
	{ 0x104BA, 0x104E2 },
	{ 0x104BB, 0x104E3 },
	{ 0x104BC, 0x104E4 },
	{ 0x104BD, 0x104E5 },
	{ 0x104BE, 0x104E6 },
	{ 0x104BF, 0x104E7 },
	{ 0x104C0, 0x104E8 },
	{ 0x104C1, 0x104E9 },
	{ 0x104C2, 0x104EA },
	{ 0x104C3, 0x104EB },
	{ 0x104C4, 0x104EC },
	{ 0x104C5, 0x104ED },
	{ 0x104C6, 0x104EE },
	{ 0x104C7, 0x104EF },
	{ 0x104C8, 0x104F0 },
	{ 0x104C9, 0x104F1 },
	{ 0x104CA, 0x104F2 },
	{ 0x104CB, 0x104F3 },
	{ 0x104CC, 0x104F4 },
	{ 0x104CD, 0x104F5 },
	{ 0x104CE, 0x104F6 },
	{ 0x104CF, 0x104F7 },
	{ 0x104D0, 0x104F8 },
	{ 0x104D1, 0x104F9 },
	{ 0x104D2, 0x104FA },
	{ 0x104D3, 0x104FB },
	{ 0x10570, 0x10597 },
	{ 0x10571, 0x10598 },
	{ 0x10572, 0x10599 },
	{ 0x10573, 0x1059A },
	{ 0x10574, 0x1059B },
	{ 0x10575, 0x1059C },
	{ 0x10576, 0x1059D },
	{ 0x10577, 0x1059E },
	{ 0x10578, 0x1059F },
	{ 0x10579, 0x105A0 },
	{ 0x1057A, 0x105A1 },
	{ 0x1057C, 0x105A3 },
	{ 0x1057D, 0x105A4 },
	{ 0x1057E, 0x105A5 },
	{ 0x1057F, 0x105A6 },
	{ 0x10580, 0x105A7 },
	{ 0x10581, 0x105A8 },
	{ 0x10582, 0x105A9 },
	{ 0x10583, 0x105AA },
	{ 0x10584, 0x105AB },
	{ 0x10585, 0x105AC },
	{ 0x10586, 0x105AD },
	{ 0x10587, 0x105AE },
	{ 0x10588, 0x105AF },
	{ 0x10589, 0x105B0 },
	{ 0x1058A, 0x105B1 },
	{ 0x1058C, 0x105B3 },
	{ 0x1058D, 0x105B4 },
	{ 0x1058E, 0x105B5 },
	{ 0x1058F, 0x105B6 },
	{ 0x10590, 0x105B7 },
	{ 0x10591, 0x105B8 },
	{ 0x10592, 0x105B9 },
	{ 0x10594, 0x105BB },
	{ 0x10595, 0x105BC },
	{ 0x10C80, 0x10CC0 },
	{ 0x10C81, 0x10CC1 },
	{ 0x10C82, 0x10CC2 },
	{ 0x10C83, 0x10CC3 },
	{ 0x10C84, 0x10CC4 },
	{ 0x10C85, 0x10CC5 },
	{ 0x10C86, 0x10CC6 },
	{ 0x10C87, 0x10CC7 },
	{ 0x10C88, 0x10CC8 },
	{ 0x10C89, 0x10CC9 },
	{ 0x10C8A, 0x10CCA },
	{ 0x10C8B, 0x10CCB },
	{ 0x10C8C, 0x10CCC },
	{ 0x10C8D, 0x10CCD },
	{ 0x10C8E, 0x10CCE },
	{ 0x10C8F, 0x10CCF },
	{ 0x10C90, 0x10CD0 },
	{ 0x10C91, 0x10CD1 },
	{ 0x10C92, 0x10CD2 },
	{ 0x10C93, 0x10CD3 },
	{ 0x10C94, 0x10CD4 },
	{ 0x10C95, 0x10CD5 },
	{ 0x10C96, 0x10CD6 },
	{ 0x10C97, 0x10CD7 },
	{ 0x10C98, 0x10CD8 },
	{ 0x10C99, 0x10CD9 },
	{ 0x10C9A, 0x10CDA },
	{ 0x10C9B, 0x10CDB },
	{ 0x10C9C, 0x10CDC },
	{ 0x10C9D, 0x10CDD },
	{ 0x10C9E, 0x10CDE },
	{ 0x10C9F, 0x10CDF },
	{ 0x10CA0, 0x10CE0 },
	{ 0x10CA1, 0x10CE1 },
	{ 0x10CA2, 0x10CE2 },
	{ 0x10CA3, 0x10CE3 },
	{ 0x10CA4, 0x10CE4 },
	{ 0x10CA5, 0x10CE5 },
	{ 0x10CA6, 0x10CE6 },
	{ 0x10CA7, 0x10CE7 },
	{ 0x10CA8, 0x10CE8 },
	{ 0x10CA9, 0x10CE9 },
	{ 0x10CAA, 0x10CEA },
	{ 0x10CAB, 0x10CEB },
	{ 0x10CAC, 0x10CEC },
	{ 0x10CAD, 0x10CED },
	{ 0x10CAE, 0x10CEE },
	{ 0x10CAF, 0x10CEF },
	{ 0x10CB0, 0x10CF0 },
	{ 0x10CB1, 0x10CF1 },
	{ 0x10CB2, 0x10CF2 },
	{ 0x10D50, 0x10D70 },
	{ 0x10D51, 0x10D71 },
	{ 0x10D52, 0x10D72 },
	{ 0x10D53, 0x10D73 },
	{ 0x10D54, 0x10D74 },
	{ 0x10D55, 0x10D75 },
	{ 0x10D56, 0x10D76 },
	{ 0x10D57, 0x10D77 },
	{ 0x10D58, 0x10D78 },
	{ 0x10D59, 0x10D79 },
	{ 0x10D5A, 0x10D7A },
	{ 0x10D5B, 0x10D7B },
	{ 0x10D5C, 0x10D7C },
	{ 0x10D5D, 0x10D7D },
	{ 0x10D5E, 0x10D7E },
	{ 0x10D5F, 0x10D7F },
	{ 0x10D60, 0x10D80 },
	{ 0x10D61, 0x10D81 },
	{ 0x10D62, 0x10D82 },
	{ 0x10D63, 0x10D83 },
	{ 0x10D64, 0x10D84 },
	{ 0x10D65, 0x10D85 },
	{ 0x118A0, 0x118C0 },
	{ 0x118A1, 0x118C1 },
	{ 0x118A2, 0x118C2 },
	{ 0x118A3, 0x118C3 },
	{ 0x118A4, 0x118C4 },
	{ 0x118A5, 0x118C5 },
	{ 0x118A6, 0x118C6 },
	{ 0x118A7, 0x118C7 },
	{ 0x118A8, 0x118C8 },
	{ 0x118A9, 0x118C9 },
	{ 0x118AA, 0x118CA },
	{ 0x118AB, 0x118CB },
	{ 0x118AC, 0x118CC },
	{ 0x118AD, 0x118CD },
	{ 0x118AE, 0x118CE },
	{ 0x118AF, 0x118CF },
	{ 0x118B0, 0x118D0 },
	{ 0x118B1, 0x118D1 },
	{ 0x118B2, 0x118D2 },
	{ 0x118B3, 0x118D3 },
	{ 0x118B4, 0x118D4 },
	{ 0x118B5, 0x118D5 },
	{ 0x118B6, 0x118D6 },
	{ 0x118B7, 0x118D7 },
	{ 0x118B8, 0x118D8 },
	{ 0x118B9, 0x118D9 },
	{ 0x118BA, 0x118DA },
	{ 0x118BB, 0x118DB },
	{ 0x118BC, 0x118DC },
	{ 0x118BD, 0x118DD },
	{ 0x118BE, 0x118DE },
	{ 0x118BF, 0x118DF },
	{ 0x16E40, 0x16E60 },
	{ 0x16E41, 0x16E61 },
	{ 0x16E42, 0x16E62 },
	{ 0x16E43, 0x16E63 },
	{ 0x16E44, 0x16E64 },
	{ 0x16E45, 0x16E65 },
	{ 0x16E46, 0x16E66 },
	{ 0x16E47, 0x16E67 },
	{ 0x16E48, 0x16E68 },
	{ 0x16E49, 0x16E69 },
	{ 0x16E4A, 0x16E6A },
	{ 0x16E4B, 0x16E6B },
	{ 0x16E4C, 0x16E6C },
	{ 0x16E4D, 0x16E6D },
	{ 0x16E4E, 0x16E6E },
	{ 0x16E4F, 0x16E6F },
	{ 0x16E50, 0x16E70 },
	{ 0x16E51, 0x16E71 },
	{ 0x16E52, 0x16E72 },
	{ 0x16E53, 0x16E73 },
	{ 0x16E54, 0x16E74 },
	{ 0x16E55, 0x16E75 },
	{ 0x16E56, 0x16E76 },
	{ 0x16E57, 0x16E77 },
	{ 0x16E58, 0x16E78 },
	{ 0x16E59, 0x16E79 },
	{ 0x16E5A, 0x16E7A },
	{ 0x16E5B, 0x16E7B },
	{ 0x16E5C, 0x16E7C },
	{ 0x16E5D, 0x16E7D },
	{ 0x16E5E, 0x16E7E },
	{ 0x16E5F, 0x16E7F },
	{ 0x16EA0, 0x16EBB },
	{ 0x16EA1, 0x16EBC },
	{ 0x16EA2, 0x16EBD },
	{ 0x16EA3, 0x16EBE },
	{ 0x16EA4, 0x16EBF },
	{ 0x16EA5, 0x16EC0 },
	{ 0x16EA6, 0x16EC1 },
	{ 0x16EA7, 0x16EC2 },
	{ 0x16EA8, 0x16EC3 },
	{ 0x16EA9, 0x16EC4 },
	{ 0x16EAA, 0x16EC5 },
	{ 0x16EAB, 0x16EC6 },
	{ 0x16EAC, 0x16EC7 },
	{ 0x16EAD, 0x16EC8 },
	{ 0x16EAE, 0x16EC9 },
	{ 0x16EAF, 0x16ECA },
	{ 0x16EB0, 0x16ECB },
	{ 0x16EB1, 0x16ECC },
	{ 0x16EB2, 0x16ECD },
	{ 0x16EB3, 0x16ECE },
	{ 0x16EB4, 0x16ECF },
	{ 0x16EB5, 0x16ED0 },
	{ 0x16EB6, 0x16ED1 },
	{ 0x16EB7, 0x16ED2 },
	{ 0x16EB8, 0x16ED3 },
	{ 0x1E900, 0x1E922 },
	{ 0x1E901, 0x1E923 },
	{ 0x1E902, 0x1E924 },
	{ 0x1E903, 0x1E925 },
	{ 0x1E904, 0x1E926 },
	{ 0x1E905, 0x1E927 },
	{ 0x1E906, 0x1E928 },
	{ 0x1E907, 0x1E929 },
	{ 0x1E908, 0x1E92A },
	{ 0x1E909, 0x1E92B },
	{ 0x1E90A, 0x1E92C },
	{ 0x1E90B, 0x1E92D },
	{ 0x1E90C, 0x1E92E },
	{ 0x1E90D, 0x1E92F },
	{ 0x1E90E, 0x1E930 },
	{ 0x1E90F, 0x1E931 },
	{ 0x1E910, 0x1E932 },
	{ 0x1E911, 0x1E933 },
	{ 0x1E912, 0x1E934 },
	{ 0x1E913, 0x1E935 },
	{ 0x1E914, 0x1E936 },
	{ 0x1E915, 0x1E937 },
	{ 0x1E916, 0x1E938 },
	{ 0x1E917, 0x1E939 },
	{ 0x1E918, 0x1E93A },
	{ 0x1E919, 0x1E93B },
	{ 0x1E91A, 0x1E93C },
	{ 0x1E91B, 0x1E93D },
	{ 0x1E91C, 0x1E93E },
	{ 0x1E91D, 0x1E93F },
	{ 0x1E91E, 0x1E940 },
	{ 0x1E91F, 0x1E941 },
	{ 0x1E920, 0x1E942 },
	{ 0x1E921, 0x1E943 },
};

inline int _find_upper(int p_ch) {
	int low = 0;
	int high = LTU_LEN - 1;
	int middle;

	while (low <= high) {
		middle = (low + high) / 2;

		if (p_ch < caps_table[middle][0]) {
			high = middle - 1; // Search low end of array.
		} else if (caps_table[middle][0] < p_ch) {
			low = middle + 1; // Search high end of array.
		} else {
			return caps_table[middle][1];
		}
	}

	return p_ch;
}

inline int _find_lower(int p_ch) {
	int low = 0;
	int high = UTL_LEN - 1;
	int middle;

	while (low <= high) {
		middle = (low + high) / 2;

		if (p_ch < reverse_caps_table[middle][0]) {
			high = middle - 1; // Search low end of array.
		} else if (reverse_caps_table[middle][0] < p_ch) {
			low = middle + 1; // Search high end of array.
		} else {
			return reverse_caps_table[middle][1];
		}
	}

	return p_ch;
}

// --- End core/string/ucaps.h ---
#include "core/version_generated.gen.h"
#include "core/string/ustring.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // to disable build-time warning which suggested to use strcpy_s
								// instead strcpy
#endif

#if defined(MINGW_ENABLED) || defined(_MSC_VER)
#define snprintf _snprintf_s
#endif

static const int MAX_DECIMALS = 32;

static _FORCE_INLINE_ char32_t lower_case(char32_t p_char)
{
	return (is_ascii_upper_case(p_char) ? (p_char + ('a' - 'A')) : p_char);
}

// Case-insensitive version of are_spans_equal
template <typename T1, typename T2>
static bool strings_equal_lower(const T1* p_lhs_begin, const T2* p_rhs_begin, size_t p_len)
{
	for (size_t i = 0; i < p_len; ++i) {
		if (_find_lower(p_lhs_begin[i]) != _find_lower(p_rhs_begin[i])) {
			return false;
		}
	}
	return true;
}

Error String::parse_url(
	String& r_scheme, String& r_host, int& r_port, String& r_path, String& r_fragment) const
{
	// Splits the URL into scheme, host, port, path, fragment. Strip credentials when present.
	String base = *this;
	r_scheme = "";
	r_host = "";
	r_port = 0;
	r_path = "";
	r_fragment = "";

	int pos = base.find("://");
	// Scheme
	if (pos != -1) {
		bool is_scheme_valid = true;
		for (int i = 0; i < pos; i++) {
			if (!is_ascii_alphanumeric_char(base[i]) && base[i] != '+' && base[i] != '-' &&
				base[i] != '.') {
				is_scheme_valid = false;
				break;
			}
		}
		if (is_scheme_valid) {
			r_scheme = base.substr(0, pos + 3).to_lower();
			base = base.substr(pos + 3);
		}
	}
	pos = base.find_char('#');
	// Fragment
	if (pos != -1) {
		r_fragment = base.substr(pos + 1);
		base = base.substr(0, pos);
	}
	pos = base.find_char('/');
	// Path
	if (pos != -1) {
		r_path = base.substr(pos);
		base = base.substr(0, pos);
	}
	// Host
	pos = base.find_char('@');
	if (pos != -1) {
		// Strip credentials
		base = base.substr(pos + 1);
	}
	if (base.begins_with("[")) {
		// Literal IPv6
		pos = base.rfind_char(']');
		if (pos == -1) {
			return ERR_INVALID_PARAMETER;
		}
		r_host = base.substr(1, pos - 1);
		base = base.substr(pos + 1);
	}
	else {
		// Anything else
		if (base.get_slice_count(":") > 2) {
			return ERR_INVALID_PARAMETER;
		}
		pos = base.rfind_char(':');
		if (pos == -1) {
			r_host = base;
			base = "";
		}
		else {
			r_host = base.substr(0, pos);
			base = base.substr(pos);
		}
	}
	if (r_host.is_empty()) {
		return ERR_INVALID_PARAMETER;
	}
	r_host = r_host.to_lower();
	// Port
	if (base.begins_with(":")) {
		base = base.substr(1);
		if (!base.is_valid_int()) {
			return ERR_INVALID_PARAMETER;
		}
		r_port = base.to_int();
		if (r_port < 1 || r_port > 65535) {
			return ERR_INVALID_PARAMETER;
		}
	}
	return OK;
}

void String::append_latin1(const Span<char>& p_cstr)
{
	if (p_cstr.is_empty()) {
		return;
	}

	const int prev_length = length();
	resize_uninitialized(prev_length + p_cstr.size() + 1); // include 0

	const char* src = p_cstr.ptr();
	const char* end = src + p_cstr.size();
	char32_t* dst = ptrw() + prev_length;

	for (; src < end; ++src, ++dst) {
		// If char is int8_t, a set sign bit will be reinterpreted as 256 - val implicitly.
		if (unlikely(*src == '\0')) {
			// NUL in string is allowed by the unicode standard, but unsupported in our
			// implementation right now.
			print_unicode_error("Unexpected NUL character", true);
			*dst = _replacement_char;
		}
		else {
			*dst = static_cast<uint8_t>(*src);
		}
	}
	*dst = 0;
}

Error String::append_utf32(const Span<char32_t>& p_cstr)
{
	if (p_cstr.is_empty()) {
		return OK;
	}

	Error error = OK;

	const int prev_length = length();
	resize_uninitialized(prev_length + p_cstr.size() + 1);
	const char32_t* src = p_cstr.ptr();
	const char32_t* end = p_cstr.ptr() + p_cstr.size();
	char32_t* dst = ptrw() + prev_length;

	// Copy the string, and check for UTF-32 problems.
	for (; src < end; ++src, ++dst) {
		const char32_t chr = *src;
		if (unlikely(chr == U'\0')) {
			// NUL in string is allowed by the unicode standard, but unsupported in our
			// implementation right now.
			print_unicode_error("Unexpected NUL character", true);
			*dst = _replacement_char;
			error = ERR_PARSE_ERROR;
		}
		else if (unlikely((chr & 0xfffff800) == 0xd800)) {
			print_unicode_error(vformat("Unpaired surrogate (%x)", (uint32_t)chr), true);
			*dst = _replacement_char;
			error = ERR_PARSE_ERROR;
		}
		else if (unlikely(chr > 0x10ffff)) {
			print_unicode_error(vformat("Invalid unicode codepoint (%x)", (uint32_t)chr), true);
			*dst = _replacement_char;
			error = ERR_PARSE_ERROR;
		}
		else {
			*dst = chr;
		}
	}
	*dst = 0;
	return error;
}

void String::append_utf32_unchecked(const Span<char32_t>& p_span)
{
	const int prev_length = length();
	resize_uninitialized(prev_length + p_span.size() + 1); // + 1 for \0
	char32_t* dst = ptrw() + prev_length;
	memcpy(dst, p_span.ptr(), p_span.size() * sizeof(char32_t));
	*(dst + p_span.size()) = _null;
}

String String::operator+(const String& p_str) const
{
	String res = *this;
	res += p_str;
	return res;
}

String String::operator+(const char* p_str) const
{
	String res = *this;
	res += p_str;
	return res;
}

String String::operator+(const wchar_t* p_str) const
{
	String res = *this;
	res += p_str;
	return res;
}

String String::operator+(const char32_t* p_str) const
{
	String res = *this;
	res += p_str;
	return res;
}

String String::operator+(char32_t p_char) const
{
	String res = *this;
	res += p_char;
	return res;
}

String operator+(const char* p_chr, const String& p_str)
{
	String tmp = p_chr;
	tmp += p_str;
	return tmp;
}

String operator+(const wchar_t* p_chr, const String& p_str)
{
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	String tmp = String::utf16((const char16_t*)p_chr);
#else
	// wchar_t is 32-bit
	String tmp = (const char32_t*)p_chr;
#endif
	tmp += p_str;
	return tmp;
}

String operator+(char32_t p_chr, const String& p_str) { return (String::chr(p_chr) + p_str); }

String& String::operator+=(const String& p_str)
{
	if (is_empty()) {
		*this = p_str;
		return *this;
	}
	append_utf32_unchecked(p_str);
	return *this;
}

String& String::operator+=(const char* p_str)
{
	append_latin1(p_str);
	return *this;
}

String& String::operator+=(const wchar_t* p_str)
{
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	*this += String::utf16((const char16_t*)p_str);
#else
	// wchar_t is 32-bit
	*this += String((const char32_t*)p_str);
#endif
	return *this;
}

String& String::operator+=(const char32_t* p_str)
{
	append_utf32(Span(p_str, strlen(p_str)));
	return *this;
}

String& String::operator+=(char32_t p_char)
{
	append_utf32(Span(&p_char, 1));
	return *this;
}

bool String::operator==(const char* p_str) const
{
	// Compare Latin-1 encoded c-string.
	return span() == Span(p_str, strlen(p_str)).reinterpret<uint8_t>();
}

bool String::operator==(const wchar_t* p_str) const
{
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit, parse as UTF-16
	return *this == String::utf16((const char16_t*)p_str);
#else
	// wchar_t is 32-bit, compare char by char
	return *this == (const char32_t*)p_str;
#endif
}

bool String::operator==(const char32_t* p_str) const
{
	// Compare UTF-32 encoded c-string.
	return span() == Span(p_str, strlen(p_str));
}

bool String::operator==(const String& p_str) const { return span() == p_str.span(); }

bool String::operator==(const Span<char32_t>& p_str_range) const { return span() == p_str_range; }

bool operator==(const char* p_chr, const String& p_str) { return p_str == p_chr; }

bool operator==(const wchar_t* p_chr, const String& p_str)
{
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	return p_str == String::utf16((const char16_t*)p_chr);
#else
	// wchar_t is 32-bit
	return p_str == (const char32_t*)p_chr;
#endif
}

bool operator!=(const char* p_chr, const String& p_str) { return !(p_str == p_chr); }

bool operator!=(const wchar_t* p_chr, const String& p_str)
{
#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	return !(p_str == String::utf16((const char16_t*)p_chr));
#else
	// wchar_t is 32-bit
	return !(p_str == String((const char32_t*)p_chr));
#endif
}

bool String::operator!=(const char* p_str) const { return (!(*this == p_str)); }

bool String::operator!=(const wchar_t* p_str) const { return (!(*this == p_str)); }

bool String::operator!=(const char32_t* p_str) const { return (!(*this == p_str)); }

bool String::operator!=(const String& p_str) const { return !((*this == p_str)); }

bool String::operator<=(const String& p_str) const { return !(p_str < *this); }

bool String::operator>(const String& p_str) const { return p_str < *this; }

bool String::operator>=(const String& p_str) const { return !(*this < p_str); }

bool String::operator<(const char* p_str) const
{
	if (is_empty() && p_str[0] == 0) {
		return false;
	}
	if (is_empty()) {
		return true;
	}
	return str_compare(get_data(), p_str) < 0;
}

bool String::operator<(const wchar_t* p_str) const
{
	if (is_empty() && p_str[0] == 0) {
		return false;
	}
	if (is_empty()) {
		return true;
	}

#ifdef WINDOWS_ENABLED
	// wchar_t is 16-bit
	return str_compare(get_data(), String::utf16((const char16_t*)p_str).get_data()) < 0;
#else
	// wchar_t is 32-bit
	return str_compare(get_data(), (const char32_t*)p_str) < 0;
#endif
}

bool String::operator<(const char32_t* p_str) const
{
	if (is_empty() && p_str[0] == 0) {
		return false;
	}
	if (is_empty()) {
		return true;
	}

	return str_compare(get_data(), p_str) < 0;
}

bool String::operator<(const String& p_str) const { return operator<(p_str.get_data()); }

signed char String::nocasecmp_to(const String& p_str) const
{
	if (is_empty() && p_str.is_empty()) {
		return 0;
	}
	if (is_empty()) {
		return -1;
	}
	if (p_str.is_empty()) {
		return 1;
	}

	const char32_t* that_str = p_str.get_data();
	const char32_t* this_str = get_data();

	while (true) {
		if (*that_str == 0 && *this_str == 0) { // If both strings are at the end, they are equal.
			return 0;
		}
		else if (*this_str == 0) { // If at the end of this, and not of other, we are less.
			return -1;
		}
		else if (*that_str == 0) { // If at end of other, and not of this, we are greater.
			return 1;
		}
		else if (_find_upper(*this_str) <
				   _find_upper(*that_str)) { // If current character in this is less, we are less.
			return -1;
		}
		else if (_find_upper(*this_str) >
				   _find_upper(
					   *that_str)) { // If current character in this is greater, we are greater.
			return 1;
		}

		this_str++;
		that_str++;
	}
}

signed char String::casecmp_to(const String& p_str) const
{
	if (is_empty() && p_str.is_empty()) {
		return 0;
	}
	if (is_empty()) {
		return -1;
	}
	if (p_str.is_empty()) {
		return 1;
	}

	const char32_t* that_str = p_str.get_data();
	const char32_t* this_str = get_data();

	while (true) {
		if (*that_str == 0 && *this_str == 0) { // If both strings are at the end, they are equal.
			return 0;
		}
		else if (*this_str == 0) { // If at the end of this, and not of other, we are less.
			return -1;
		}
		else if (*that_str == 0) { // If at end of other, and not of this, we are greater.
			return 1;
		}
		else if (*this_str < *that_str) { // If current character in this is less, we are less.
			return -1;
		}
		else if (*this_str >
				   *that_str) { // If current character in this is greater, we are greater.
			return 1;
		}

		this_str++;
		that_str++;
	}
}

static _FORCE_INLINE_ signed char natural_cmp_common(
	const char32_t*& r_this_str, const char32_t*& r_that_str)
{
	// Keep ptrs to start of numerical sequences.
	const char32_t* this_substr = r_this_str;
	const char32_t* that_substr = r_that_str;

	// Compare lengths of both numerical sequences, ignoring leading zeros.
	while (is_digit(*r_this_str)) {
		r_this_str++;
	}
	while (is_digit(*r_that_str)) {
		r_that_str++;
	}
	while (*this_substr == '0') {
		this_substr++;
	}
	while (*that_substr == '0') {
		that_substr++;
	}
	int this_len = r_this_str - this_substr;
	int that_len = r_that_str - that_substr;

	if (this_len < that_len) {
		return -1;
	}
	else if (this_len > that_len) {
		return 1;
	}

	// If lengths equal, compare lexicographically.
	while (this_substr != r_this_str && that_substr != r_that_str) {
		if (*this_substr < *that_substr) {
			return -1;
		}
		else if (*this_substr > *that_substr) {
			return 1;
		}
		this_substr++;
		that_substr++;
	}

	return 0;
}

static _FORCE_INLINE_ signed char naturalcasecmp_to_base(
	const char32_t* p_this_str, const char32_t* p_that_str)
{
	if (p_this_str && p_that_str) {
		while (*p_this_str == '.' || *p_that_str == '.') {
			if (*p_this_str++ != '.') {
				return 1;
			}
			if (*p_that_str++ != '.') {
				return -1;
			}
			if (!*p_that_str) {
				return 1;
			}
			if (!*p_this_str) {
				return -1;
			}
		}

		while (*p_this_str) {
			if (!*p_that_str) {
				return 1;
			}
			else if (is_digit(*p_this_str)) {
				if (!is_digit(*p_that_str)) {
					return -1;
				}

				signed char ret = natural_cmp_common(p_this_str, p_that_str);
				if (ret) {
					return ret;
				}
			}
			else if (is_digit(*p_that_str)) {
				return 1;
			}
			else {
				if (*p_this_str <
					*p_that_str) { // If current character in this is less, we are less.
					return -1;
				}
				else if (*p_this_str > *p_that_str) { // If current character in this is greater,
														// we are greater.
					return 1;
				}

				p_this_str++;
				p_that_str++;
			}
		}
		if (*p_that_str) {
			return -1;
		}
	}

	return 0;
}

signed char String::naturalcasecmp_to(const String& p_str) const
{
	const char32_t* this_str = get_data();
	const char32_t* that_str = p_str.get_data();

	return naturalcasecmp_to_base(this_str, that_str);
}

static _FORCE_INLINE_ signed char naturalnocasecmp_to_base(
	const char32_t* p_this_str, const char32_t* p_that_str)
{
	if (p_this_str && p_that_str) {
		while (*p_this_str == '.' || *p_that_str == '.') {
			if (*p_this_str++ != '.') {
				return 1;
			}
			if (*p_that_str++ != '.') {
				return -1;
			}
			if (!*p_that_str) {
				return 1;
			}
			if (!*p_this_str) {
				return -1;
			}
		}

		while (*p_this_str) {
			if (!*p_that_str) {
				return 1;
			}
			else if (is_digit(*p_this_str)) {
				if (!is_digit(*p_that_str)) {
					return -1;
				}

				signed char ret = natural_cmp_common(p_this_str, p_that_str);
				if (ret) {
					return ret;
				}
			}
			else if (is_digit(*p_that_str)) {
				return 1;
			}
			else {
				if (_find_upper(*p_this_str) <
					_find_upper(
						*p_that_str)) { // If current character in this is less, we are less.
					return -1;
				}
				else if (_find_upper(*p_this_str) >
						   _find_upper(*p_that_str)) { // If current character in this is greater,
													   // we are greater.
					return 1;
				}

				p_this_str++;
				p_that_str++;
			}
		}
		if (*p_that_str) {
			return -1;
		}
	}

	return 0;
}

signed char String::naturalnocasecmp_to(const String& p_str) const
{
	const char32_t* this_str = get_data();
	const char32_t* that_str = p_str.get_data();

	return naturalnocasecmp_to_base(this_str, that_str);
}

static _FORCE_INLINE_ signed char file_cmp_common(
	const char32_t*& r_this_str, const char32_t*& r_that_str)
{
	// Compare leading `_` sequences.
	while ((*r_this_str == '_' && *r_that_str) || (*r_this_str && *r_that_str == '_')) {
		// Sort `_` lower than everything except `.`
		if (*r_this_str != '_') {
			return *r_this_str == '.' ? -1 : 1;
		}
		else if (*r_that_str != '_') {
			return *r_that_str == '.' ? 1 : -1;
		}
		r_this_str++;
		r_that_str++;
	}

	return 0;
}

signed char String::filecasecmp_to(const String& p_str) const
{
	const char32_t* this_str = get_data();
	const char32_t* that_str = p_str.get_data();

	signed char ret = file_cmp_common(this_str, that_str);
	if (ret) {
		return ret;
	}

	return naturalcasecmp_to_base(this_str, that_str);
}

signed char String::filenocasecmp_to(const String& p_str) const
{
	const char32_t* this_str = get_data();
	const char32_t* that_str = p_str.get_data();

	signed char ret = file_cmp_common(this_str, that_str);
	if (ret) {
		return ret;
	}

	return naturalnocasecmp_to_base(this_str, that_str);
}

String String::_separate_compound_words() const
{
	if (length() == 0) {
		return *this;
	}

	const char32_t* cstr = get_data();
	int start_index = 0;
	String new_string;

	bool is_prev_upper = is_unicode_upper_case(cstr[0]);
	bool is_prev_lower = is_unicode_lower_case(cstr[0]);
	bool is_prev_digit = is_digit(cstr[0]);

	for (int i = 1; i < length(); i++) {
		const bool is_curr_upper = is_unicode_upper_case(cstr[i]);
		const bool is_curr_lower = is_unicode_lower_case(cstr[i]);
		const bool is_curr_digit = is_digit(cstr[i]);

		bool is_next_lower = false;
		if (i + 1 < length()) {
			is_next_lower = is_unicode_lower_case(cstr[i + 1]);
		}

		const bool cond_a = is_prev_lower && is_curr_upper; // aA
		const bool cond_b =
			(is_prev_upper || is_prev_digit) && is_curr_upper && is_next_lower; // AAa, 2Aa
		const bool cond_c = is_prev_digit && is_curr_lower && is_next_lower;	// 2aa
		const bool cond_d = (is_prev_upper || is_prev_lower) && is_curr_digit;	// A2, a2

		if (cond_a || cond_b || cond_c || cond_d) {
			new_string += substr(start_index, i - start_index) + " ";
			start_index = i;
		}

		is_prev_upper = is_curr_upper;
		is_prev_lower = is_curr_lower;
		is_prev_digit = is_curr_digit;
	}

	new_string += substr(start_index, size() - start_index);

	for (int i = 0; i < new_string.size(); i++) {
		const bool whitespace = is_whitespace(new_string[i]);
		const bool underscore = is_underscore(new_string[i]);
		const bool hyphen = is_hyphen(new_string[i]);

		if (whitespace || underscore || hyphen) {
			new_string[i] = ' ';
		}
	}

	return new_string.to_lower();
}

String String::capitalize() const
{
	String words = _separate_compound_words().strip_edges();
	String ret;
	for (int i = 0; i < words.get_slice_count(" "); i++) {
		String slice = words.get_slicec(' ', i);
		if (slice.length() > 0) {
			slice[0] = _find_upper(slice[0]);
			if (i > 0) {
				ret += " ";
			}
			ret += slice;
		}
	}
	return ret;
}

String String::to_camel_case() const
{
	String words = _separate_compound_words().strip_edges();
	String ret;
	for (int i = 0; i < words.get_slice_count(" "); i++) {
		String slice = words.get_slicec(' ', i);
		if (slice.length() > 0) {
			if (i == 0) {
				slice[0] = _find_lower(slice[0]);
			}
			else {
				slice[0] = _find_upper(slice[0]);
			}
			ret += slice;
		}
	}
	return ret;
}

String String::to_pascal_case() const
{
	String words = _separate_compound_words().strip_edges();
	String ret;
	for (int i = 0; i < words.get_slice_count(" "); i++) {
		String slice = words.get_slicec(' ', i);
		if (slice.length() > 0) {
			slice[0] = _find_upper(slice[0]);
			ret += slice;
		}
	}
	return ret;
}

String String::to_snake_case() const { return _separate_compound_words().replace_char(' ', '_'); }

String String::to_kebab_case() const { return _separate_compound_words().replace_char(' ', '-'); }

String String::get_with_code_lines() const
{
	const Vector<String> lines = split("\n");
	String ret;
	for (int i = 0; i < lines.size(); i++) {
		if (i > 0) {
			ret += "\n";
		}
		ret += vformat("%4d | %s", i + 1, lines[i]);
	}
	return ret;
}

int String::get_slice_count(const String& p_splitter) const
{
	if (is_empty()) {
		return 0;
	}
	if (p_splitter.is_empty()) {
		return 0;
	}

	int pos = 0;
	int slices = 1;

	while ((pos = find(p_splitter, pos)) >= 0) {
		slices++;
		pos += p_splitter.length();
	}

	return slices;
}

int String::get_slice_count(const char* p_splitter) const
{
	if (is_empty()) {
		return 0;
	}
	if (p_splitter == nullptr || *p_splitter == '\0') {
		return 0;
	}

	int pos = 0;
	int slices = 1;
	int splitter_length = strlen(p_splitter);

	while ((pos = find(p_splitter, pos)) >= 0) {
		slices++;
		pos += splitter_length;
	}

	return slices;
}

String String::get_slice(const String& p_splitter, int p_slice) const
{
	if (is_empty() || p_splitter.is_empty()) {
		return "";
	}

	int pos = 0;
	int prev_pos = 0;
	// int slices=1;
	if (p_slice < 0) {
		return "";
	}
	if (find(p_splitter) == -1) {
		return *this;
	}

	int i = 0;
	while (true) {
		pos = find(p_splitter, pos);
		if (pos == -1) {
			pos = length(); // reached end
		}

		int from = prev_pos;
		// int to=pos;

		if (p_slice == i) {
			return substr(from, pos - from);
		}

		if (pos == length()) { // reached end and no find
			break;
		}
		pos += p_splitter.length();
		prev_pos = pos;
		i++;
	}

	return ""; // no find!
}

String String::get_slice(const char* p_splitter, int p_slice) const
{
	if (is_empty() || p_splitter == nullptr || *p_splitter == '\0') {
		return "";
	}

	int pos = 0;
	int prev_pos = 0;
	// int slices=1;
	if (p_slice < 0) {
		return "";
	}
	if (find(p_splitter) == -1) {
		return *this;
	}

	int i = 0;
	const int splitter_length = strlen(p_splitter);
	while (true) {
		pos = find(p_splitter, pos);
		if (pos == -1) {
			pos = length(); // reached end
		}

		int from = prev_pos;
		// int to=pos;

		if (p_slice == i) {
			return substr(from, pos - from);
		}

		if (pos == length()) { // reached end and no find
			break;
		}
		pos += splitter_length;
		prev_pos = pos;
		i++;
	}

	return ""; // no find!
}

String String::get_slicec(char32_t p_splitter, int p_slice) const
{
	if (is_empty()) {
		return String();
	}

	if (p_slice < 0) {
		return String();
	}

	const char32_t* c = ptr();
	int i = 0;
	int prev = 0;
	int count = 0;
	while (true) {
		if (c[i] == 0 || c[i] == p_splitter) {
			if (p_slice == count) {
				return substr(prev, i - prev);
			}
			else if (c[i] == 0) {
				return String();
			}
			else {
				count++;
				prev = i + 1;
			}
		}

		i++;
	}
}

Vector<String> String::split_spaces(int p_maxsplit) const
{
	Vector<String> ret;
	int from = 0;
	int i = 0;
	int len = length();
	if (len == 0) {
		return ret;
	}

	bool inside = false;

	while (true) {
		bool empty = operator[](i) < 33;

		if (i == 0) {
			inside = !empty;
		}

		if (!empty && !inside) {
			inside = true;
			from = i;
		}

		if (empty && inside) {
			if (p_maxsplit > 0 && p_maxsplit == ret.size()) {
				// Put rest of the string and leave cycle.
				ret.push_back(substr(from));
				break;
			}
			ret.push_back(substr(from, i - from));
			inside = false;
		}

		if (i == len) {
			break;
		}
		i++;
	}

	return ret;
}

Vector<String> String::split(const String& p_splitter, bool p_allow_empty, int p_maxsplit) const
{
	Vector<String> ret;

	if (is_empty()) {
		if (p_allow_empty) {
			ret.push_back("");
		}
		return ret;
	}

	int from = 0;
	int len = length();

	while (true) {
		int end;
		if (p_splitter.is_empty()) {
			end = from + 1;
		}
		else {
			end = find(p_splitter, from);
			if (end < 0) {
				end = len;
			}
		}
		if (p_allow_empty || (end > from)) {
			if (p_maxsplit <= 0) {
				ret.push_back(substr(from, end - from));
			}
			else {
				// Put rest of the string and leave cycle.
				if (p_maxsplit == ret.size()) {
					ret.push_back(substr(from, len));
					break;
				}

				// Otherwise, push items until positive limit is reached.
				ret.push_back(substr(from, end - from));
			}
		}

		if (end == len) {
			break;
		}

		from = end + p_splitter.length();
	}

	return ret;
}

Vector<String> String::split(const char* p_splitter, bool p_allow_empty, int p_maxsplit) const
{
	Vector<String> ret;

	if (is_empty()) {
		if (p_allow_empty) {
			ret.push_back("");
		}
		return ret;
	}

	int from = 0;
	int len = length();
	const int splitter_length = strlen(p_splitter);

	while (true) {
		int end;
		if (p_splitter == nullptr || *p_splitter == '\0') {
			end = from + 1;
		}
		else {
			end = find(p_splitter, from);
			if (end < 0) {
				end = len;
			}
		}
		if (p_allow_empty || (end > from)) {
			if (p_maxsplit <= 0) {
				ret.push_back(substr(from, end - from));
			}
			else {
				// Put rest of the string and leave cycle.
				if (p_maxsplit == ret.size()) {
					ret.push_back(substr(from, len));
					break;
				}

				// Otherwise, push items until positive limit is reached.
				ret.push_back(substr(from, end - from));
			}
		}

		if (end == len) {
			break;
		}

		from = end + splitter_length;
	}

	return ret;
}

Vector<String> String::rsplit(const String& p_splitter, bool p_allow_empty, int p_maxsplit) const
{
	Vector<String> ret;
	const int len = length();
	int remaining_len = len;

	while (true) {
		if (remaining_len < p_splitter.length() || (p_maxsplit > 0 && p_maxsplit == ret.size())) {
			// no room for another splitter or hit max splits, push what's left and we're done
			if (p_allow_empty || remaining_len > 0) {
				ret.push_back(substr(0, remaining_len));
			}
			break;
		}

		int left_edge;
		if (p_splitter.is_empty()) {
			left_edge = remaining_len - 1;
			if (left_edge == 0) {
				left_edge--; // Skip to the < 0 condition.
			}
		}
		else {
			left_edge = rfind(p_splitter, remaining_len - p_splitter.length());
		}

		if (left_edge < 0) {
			// no more splitters, we're done
			ret.push_back(substr(0, remaining_len));
			break;
		}

		int substr_start = left_edge + p_splitter.length();
		if (p_allow_empty || substr_start < remaining_len) {
			ret.push_back(substr(substr_start, remaining_len - substr_start));
		}

		remaining_len = left_edge;
	}

	ret.reverse();
	return ret;
}

Vector<String> String::rsplit(const char* p_splitter, bool p_allow_empty, int p_maxsplit) const
{
	Vector<String> ret;
	const int len = length();
	const int splitter_length = strlen(p_splitter);
	int remaining_len = len;

	while (true) {
		if (remaining_len < splitter_length || (p_maxsplit > 0 && p_maxsplit == ret.size())) {
			// no room for another splitter or hit max splits, push what's left and we're done
			if (p_allow_empty || remaining_len > 0) {
				ret.push_back(substr(0, remaining_len));
			}
			break;
		}

		int left_edge;
		if (p_splitter == nullptr || *p_splitter == '\0') {
			left_edge = remaining_len - 1;
			if (left_edge == 0) {
				left_edge--; // Skip to the < 0 condition.
			}
		}
		else {
			left_edge = rfind(p_splitter, remaining_len - splitter_length);
		}

		if (left_edge < 0) {
			// no more splitters, we're done
			ret.push_back(substr(0, remaining_len));
			break;
		}

		int substr_start = left_edge + splitter_length;
		if (p_allow_empty || substr_start < remaining_len) {
			ret.push_back(substr(substr_start, remaining_len - substr_start));
		}

		remaining_len = left_edge;
	}

	ret.reverse();
	return ret;
}

Vector<double> String::split_floats(const String& p_splitter, bool p_allow_empty) const
{
	Vector<double> ret;
	int from = 0;
	int len = length();
	if (len == 0) {
		return ret;
	}

	String buffer = *this;
	while (true) {
		int end = find(p_splitter, from);
		if (end < 0) {
			end = len;
		}
		if (p_allow_empty || (end > from)) {
			buffer[end] = 0;
			ret.push_back(String::to_float(&buffer.get_data()[from]));
			buffer[end] = _cowdata.get(end);
		}

		if (end == len) {
			break;
		}

		from = end + p_splitter.length();
	}

	return ret;
}

Vector<float> String::split_floats_mk(const Vector<String>& p_splitters, bool p_allow_empty) const
{
	Vector<float> ret;
	int from = 0;
	int len = length();
	if (len == 0) {
		return ret;
	}

	String buffer = *this;
	while (true) {
		int idx = 0;
		int end = findmk(p_splitters, from, &idx);
		int spl_len = 1;
		if (end < 0) {
			end = len;
		}
		else {
			spl_len = p_splitters[idx].length();
		}

		if (p_allow_empty || (end > from)) {
			buffer[end] = 0;
			ret.push_back(String::to_float(&buffer.get_data()[from]));
			buffer[end] = _cowdata.get(end);
		}

		if (end == len) {
			break;
		}

		from = end + spl_len;
	}

	return ret;
}

Vector<int> String::split_ints(const String& p_splitter, bool p_allow_empty) const
{
	Vector<int> ret;
	int from = 0;
	int len = length();
	if (len == 0) {
		return ret;
	}

	while (true) {
		int end = find(p_splitter, from);
		if (end < 0) {
			end = len;
		}
		if (p_allow_empty || (end > from)) {
			ret.push_back(String::to_int(&get_data()[from], end - from));
		}

		if (end == len) {
			break;
		}

		from = end + p_splitter.length();
	}

	return ret;
}

Vector<int> String::split_ints_mk(const Vector<String>& p_splitters, bool p_allow_empty) const
{
	Vector<int> ret;
	int from = 0;
	int len = length();
	if (len == 0) {
		return ret;
	}

	while (true) {
		int idx = 0;
		int end = findmk(p_splitters, from, &idx);
		int spl_len = 1;
		if (end < 0) {
			end = len;
		}
		else {
			spl_len = p_splitters[idx].length();
		}

		if (p_allow_empty || (end > from)) {
			ret.push_back(String::to_int(&get_data()[from], end - from));
		}

		if (end == len) {
			break;
		}

		from = end + spl_len;
	}

	return ret;
}

String String::join(const Vector<String>& p_parts) const
{
	if (p_parts.is_empty()) {
		return String();
	}
	else if (p_parts.size() == 1) {
		return p_parts[0];
	}

	const int this_length = length();

	int new_size = (p_parts.size() - 1) * this_length;
	for (const String& part : p_parts) {
		new_size += part.length();
	}
	new_size += 1;

	String ret;
	ret.resize_uninitialized(new_size);
	char32_t* ret_ptrw = ret.ptrw();
	const char32_t* this_ptr = ptr();

	bool first = true;
	for (const String& part : p_parts) {
		if (first) {
			first = false;
		}
		else if (this_length) {
			memcpy(ret_ptrw, this_ptr, this_length * sizeof(char32_t));
			ret_ptrw += this_length;
		}

		const int part_length = part.length();
		if (part_length) {
			memcpy(ret_ptrw, part.ptr(), part_length * sizeof(char32_t));
			ret_ptrw += part_length;
		}
	}

	*ret_ptrw = 0;

	return ret;
}

char32_t String::char_uppercase(char32_t p_char) { return _find_upper(p_char); }

char32_t String::char_lowercase(char32_t p_char) { return _find_lower(p_char); }

String String::to_upper() const
{
	if (is_empty()) {
		return *this;
	}

	String upper;
	upper.resize_uninitialized(size());
	const char32_t* old_ptr = ptr();
	char32_t* upper_ptrw = upper.ptrw();

	while (*old_ptr) {
		*upper_ptrw++ = _find_upper(*old_ptr++);
	}

	*upper_ptrw = 0;

	return upper;
}

String String::to_lower() const
{
	if (is_empty()) {
		return *this;
	}

	String lower;
	lower.resize_uninitialized(size());
	const char32_t* old_ptr = ptr();
	char32_t* lower_ptrw = lower.ptrw();

	while (*old_ptr) {
		*lower_ptrw++ = _find_lower(*old_ptr++);
	}

	*lower_ptrw = 0;

	return lower;
}

String String::num(double p_num, int p_decimals)
{
	if (Math::is_nan(p_num)) {
		return "nan";
	}

	if (Math::is_inf(p_num)) {
		if (std::signbit(p_num)) {
			return "-inf";
		}
		else {
			return "inf";
		}
	}

	if (p_decimals < 0) {
		p_decimals = 14;
		const double abs_num = Math::abs(p_num);
		if (abs_num > 10) {
			// We want to align the digits to the above reasonable default, so we only
			// need to subtract log10 for numbers with a positive power of ten.
			p_decimals -= (int)std::floor(std::log10(abs_num));
		}
	}
	if (p_decimals > MAX_DECIMALS) {
		p_decimals = MAX_DECIMALS;
	}

	char fmt[7];
	fmt[0] = '%';
	fmt[1] = '.';

	if (p_decimals < 0) {
		fmt[1] = 'l';
		fmt[2] = 'f';
		fmt[3] = 0;
	}
	else if (p_decimals < 10) {
		fmt[2] = '0' + p_decimals;
		fmt[3] = 'l';
		fmt[4] = 'f';
		fmt[5] = 0;
	}
	else {
		fmt[2] = '0' + (p_decimals / 10);
		fmt[3] = '0' + (p_decimals % 10);
		fmt[4] = 'l';
		fmt[5] = 'f';
		fmt[6] = 0;
	}
	// if we want to convert a double with as much decimal places as
	// DBL_MAX or DBL_MIN then we would theoretically need a buffer of at least
	// DBL_MAX_10_EXP + 2 for DBL_MAX and DBL_MAX_10_EXP + 4 for DBL_MIN.
	// BUT those values where still giving me exceptions, so I tested from
	// DBL_MAX_10_EXP + 10 incrementing one by one and DBL_MAX_10_EXP + 17 (325)
	// was the first buffer size not to throw an exception
	char buf[325];

#if defined(__GNUC__) || defined(_MSC_VER)
	// PLEASE NOTE that, albeit vcrt online reference states that snprintf
	// should safely truncate the output to the given buffer size, we have
	// found a case where this is not true, so we should create a buffer
	// as big as needed
	snprintf(buf, 325, fmt, p_num);
#else
	sprintf(buf, fmt, p_num);
#endif

	buf[324] = 0;
	// Destroy trailing zeroes, except one after period.
	{
		bool period = false;
		int z = 0;
		while (buf[z]) {
			if (buf[z] == '.') {
				period = true;
			}
			z++;
		}

		if (period) {
			z--;
			while (z > 0) {
				if (buf[z] == '0') {
					buf[z] = 0;
				}
				else if (buf[z] == '.') {
					buf[z + 1] = '0';
					break;
				}
				else {
					break;
				}

				z--;
			}
		}
	}

	return buf;
}

String String::num_int64(int64_t p_num, int p_base, bool p_capitalize_hex)
{
	ERR_FAIL_COND_V_MSG(p_base < 2 || p_base > 36, "",
		"Cannot convert to base " + itos(p_base) + ", since the value is " +
			(p_base < 2 ? "less than 2." : "greater than 36."));

	bool sign = p_num < 0;

	int64_t n = p_num;

	int chars = 0;
	do {
		n /= p_base;
		chars++;
	} while (n);

	if (sign) {
		chars++;
	}
	String s;
	s.resize_uninitialized(chars + 1);
	char32_t* c = s.ptrw();
	c[chars] = 0;
	n = p_num;
	do {
		int mod = Math::abs(n % p_base);
		if (mod >= 10) {
			char a = (p_capitalize_hex ? 'A' : 'a');
			c[--chars] = a + (mod - 10);
		}
		else {
			c[--chars] = '0' + mod;
		}

		n /= p_base;
	} while (n);

	if (sign) {
		c[0] = '-';
	}

	return s;
}

String String::num_uint64(uint64_t p_num, int p_base, bool p_capitalize_hex)
{
	ERR_FAIL_COND_V_MSG(p_base < 2 || p_base > 36, "",
		"Cannot convert to base " + itos(p_base) + ", since the value is " +
			(p_base < 2 ? "less than 2." : "greater than 36."));

	uint64_t n = p_num;

	int chars = 0;
	do {
		n /= p_base;
		chars++;
	} while (n);

	String s;
	s.resize_uninitialized(chars + 1);
	char32_t* c = s.ptrw();
	c[chars] = 0;
	n = p_num;
	do {
		int mod = n % p_base;
		if (mod >= 10) {
			char a = (p_capitalize_hex ? 'A' : 'a');
			c[--chars] = a + (mod - 10);
		}
		else {
			c[--chars] = '0' + mod;
		}

		n /= p_base;
	} while (n);

	return s;
}

String String::num_real(double p_num, bool p_trailing)
{
	if (Math::is_nan(p_num) || Math::is_inf(p_num)) {
		return num(p_num, 0);
	}

	if (p_num == (double)(int64_t)p_num) {
		if (p_trailing) {
			return num_int64((int64_t)p_num) + ".0";
		}
		else {
			return num_int64((int64_t)p_num);
		}
	}

	int decimals = 14;
	// We want to align the digits to the above sane default, so we only need
	// to subtract log10 for numbers with a positive power of ten magnitude.
	const double abs_num = Math::abs(p_num);
	if (abs_num > 10) {
		decimals -= (int)std::floor(std::log10(abs_num));
	}

	return num(p_num, decimals);
}

String String::num_real(float p_num, bool p_trailing)
{
	if (Math::is_nan(p_num) || Math::is_inf(p_num)) {
		return num(p_num, 0);
	}

	if (p_num == (float)(int64_t)p_num) {
		if (p_trailing) {
			return num_int64((int64_t)p_num) + ".0";
		}
		else {
			return num_int64((int64_t)p_num);
		}
	}
	int decimals = 6;
	// We want to align the digits to the above sane default, so we only need
	// to subtract log10 for numbers with a positive power of ten magnitude.
	const float abs_num = Math::abs(p_num);
	if (abs_num > 10) {
		decimals -= (int)std::floor(std::log10(abs_num));
	}
	return num(p_num, decimals);
}

String String::num_scientific(double p_num)
{
	if (Math::is_nan(p_num) || Math::is_inf(p_num)) {
		return num(p_num, 0);
	}
	char buffer[256];
	char* last = grisu2::to_chars(buffer, p_num);
	return String::ascii(Span(buffer, last - buffer));
}

String String::num_scientific(float p_num)
{
	if (Math::is_nan(p_num) || Math::is_inf(p_num)) {
		return num(p_num, 0);
	}
	char buffer[256];
	char* last = grisu2::to_chars(buffer, p_num);
	return String::ascii(Span(buffer, last - buffer));
}

String String::md5(const uint8_t* p_md5) { return String::hex_encode_buffer(p_md5, 16); }

String String::hex_encode_buffer(const uint8_t* p_buffer, int p_len)
{
	static const char hex[16] = {
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	String ret;
	ret.resize_uninitialized(p_len * 2 + 1);
	char32_t* ret_ptrw = ret.ptrw();

	for (int i = 0; i < p_len; i++) {
		*ret_ptrw++ = hex[p_buffer[i] >> 4];
		*ret_ptrw++ = hex[p_buffer[i] & 0xF];
	}

	*ret_ptrw = 0;

	return ret;
}

Vector<uint8_t> String::hex_decode() const
{
	ERR_FAIL_COND_V_MSG(
		length() % 2 != 0, Vector<uint8_t>(), "Hexadecimal string of uneven length.");

#define HEX_TO_BYTE(m_output, m_index)                                                             \
	uint8_t m_output;                                                                              \
	c = operator[](m_index);                                                                       \
	if (is_digit(c)) {                                                                             \
		m_output = c - '0';                                                                        \
	} \
	else if (c >= 'a' && c <= 'f') {                                                             \
		m_output = c - 'a' + 10;                                                                   \
	} \
	else if (c >= 'A' && c <= 'F') {                                                             \
		m_output = c - 'A' + 10;                                                                   \
	} \
	else {                                                                                       \
		ERR_FAIL_V_MSG(Vector<uint8_t>(),                                                          \
			"Invalid hexadecimal character \"" + chr(c) + "\" at index " + m_index + ".");         \
	}

	Vector<uint8_t> out;
	int len = length() / 2;
	out.resize_uninitialized(len);
	uint8_t* out_ptrw = out.ptrw();
	for (int i = 0; i < len; i++) {
		char32_t c;
		HEX_TO_BYTE(first, i * 2);
		HEX_TO_BYTE(second, i * 2 + 1);
		out_ptrw[i] = first * 16 + second;
	}
	return out;
#undef HEX_TO_BYTE
}

void String::print_unicode_error(const String& p_message, bool p_critical) const
{
	if (p_critical) {
		print_error(
			vformat(U"Unicode parsing error, some characters were replaced with � (U+FFFD): %s",
				p_message));
	}
	else {
		print_error(vformat("Unicode parsing error: %s", p_message));
	}
}

CharString String::ascii(bool p_allow_extended) const
{
	if (!length()) {
		return CharString();
	}

	CharString cs;
	cs.resize_uninitialized(size());
	char* cs_ptrw = cs.ptrw();
	const char32_t* this_ptr = ptr();

	for (int i = 0; i < size(); i++) {
		char32_t c = this_ptr[i];
		if ((c <= 0x7f) || (c <= 0xff && p_allow_extended)) {
			cs_ptrw[i] = char(c);
		}
		else {
			print_unicode_error(vformat(
				"Invalid unicode codepoint (%x), cannot represent as ASCII/Latin-1", (uint32_t)c));
			cs_ptrw[i] = 0x20; // ASCII doesn't have a replacement character like unicode, 0x1a is
							   // sometimes used but is kinda arcane.
		}
	}

	return cs;
}

Error String::append_ascii(const Span<char>& p_range)
{
	if (p_range.is_empty()) {
		return OK;
	}

	const int prev_length = length();
	resize_uninitialized(prev_length + p_range.size() + 1); // Include \0

	const char* src = p_range.ptr();
	const char* end = src + p_range.size();
	char32_t* dst = ptrw() + prev_length;
	bool decode_failed = false;

	for (; src < end; ++src, ++dst) {
		// If char is int8_t, a set sign bit will be reinterpreted as 256 - val implicitly.
		const uint8_t chr = *src;
		if (unlikely(chr == '\0')) {
			// NUL in string is allowed by the unicode standard, but unsupported in our
			// implementation right now.
			print_unicode_error("Unexpected NUL character", true);
			*dst = _replacement_char;
		}
		else if (unlikely(chr > 127)) {
			print_unicode_error(vformat("Invalid ASCII codepoint (%x)", (uint32_t)chr), true);
			decode_failed = true;
			*dst = _replacement_char;
		}
		else {
			*dst = chr;
		}
	}
	*dst = _null;
	return decode_failed ? ERR_INVALID_DATA : OK;
}

Error String::append_utf8(const char* p_utf8, int p_len)
{
	if (!p_utf8) {
		return ERR_INVALID_DATA;
	}

	/* HANDLE BOM (Byte Order Mark) */
	if (p_len < 0 || p_len >= 3) {
		bool has_bom =
			uint8_t(p_utf8[0]) == 0xef && uint8_t(p_utf8[1]) == 0xbb && uint8_t(p_utf8[2]) == 0xbf;
		if (has_bom) {
			// 8-bit encoding, byte order has no meaning in UTF-8, just skip it
			if (p_len >= 0) {
				p_len -= 3;
			}
			p_utf8 += 3;
		}
	}

	if (p_len < 0) {
		p_len = strlen(p_utf8);
	}

	const int prev_length = length();
	// If all utf8 characters maps to ASCII, then the max size will be p_len, and we add +1 for the
	// null termination.
	resize_uninitialized(prev_length + p_len + 1);
	char32_t* dst = ptrw() + prev_length;

	Error result = Error::OK;

	const uint8_t* ptrtmp = (uint8_t*)p_utf8;
	const uint8_t* ptr_limit = (uint8_t*)p_utf8 + p_len;

	while (ptrtmp < ptr_limit && *ptrtmp) {
		uint8_t c = *ptrtmp;
		uint32_t unicode = _replacement_char;
		uint32_t size = 1;

		if ((c & 0b10000000) == 0) {
			unicode = c;
			if (unicode > 0x7F) {
				unicode = _replacement_char;
				print_unicode_error(vformat("Invalid unicode codepoint (%d)", unicode), true);
				result = Error::ERR_INVALID_DATA;
			}
		}
		else if ((c & 0b11100000) == 0b11000000) {
			if (ptrtmp + 1 >= ptr_limit) {
				print_unicode_error(vformat("Missing %x UTF-8 continuation byte", c), true);
				result = Error::ERR_INVALID_DATA;
			}
			else {
				uint8_t c2 = *(ptrtmp + 1);

				if ((c2 & 0b11000000) == 0b10000000) {
					unicode = (uint32_t)((c & 0b00011111) << 6) | (uint32_t)(c2 & 0b00111111);

					if (unicode < 0x80) {
						unicode = _replacement_char;
						print_unicode_error(vformat("Overlong encoding (%x %x)", c, c2));
						result = Error::ERR_INVALID_DATA;
					}
					else if (unicode > 0x7FF) {
						unicode = _replacement_char;
						print_unicode_error(
							vformat("Invalid unicode codepoint (%d)", unicode), true);
						result = Error::ERR_INVALID_DATA;
					}
					else {
						size = 2;
					}
				}
				else {
					print_unicode_error(
						vformat("Byte %x is not a correct continuation byte after %x", c2, c));
					result = Error::ERR_INVALID_DATA;
				}
			}
		}
		else if ((c & 0b11110000) == 0b11100000) {
			uint32_t range_min = (c == 0xE0) ? 0xA0 : 0x80;
			uint32_t range_max = (c == 0xED) ? 0x9F : 0xBF;
			uint8_t c2 = (ptrtmp + 1) < ptr_limit ? *(ptrtmp + 1) : 0;
			uint8_t c3 = (ptrtmp + 2) < ptr_limit ? *(ptrtmp + 2) : 0;
			bool c2_valid = c2 && (c2 >= range_min) && (c2 <= range_max);
			bool c3_valid = c3 && ((c3 & 0b11000000) == 0b10000000);

			if (c2_valid && c3_valid) {
				unicode = (uint32_t)((c & 0b00001111) << 12) | (uint32_t)((c2 & 0b00111111) << 6) |
						  (uint32_t)(c3 & 0b00111111);

				if (unicode < 0x800) {
					unicode = _replacement_char;
					print_unicode_error(vformat("Overlong encoding (%x %x %x)", c, c2, c3));
					result = Error::ERR_INVALID_DATA;
				}
				else if (unicode > 0xFFFF) {
					unicode = _replacement_char;
					print_unicode_error(vformat("Invalid unicode codepoint (%d)", unicode), true);
					result = Error::ERR_INVALID_DATA;
				}
				else {
					size = 3;
				}
			}
			else {
				if (c2 == 0) {
					print_unicode_error(vformat("Missing %x UTF-8 continuation byte", c), true);
				}
				else if (c2_valid == false) {
					print_unicode_error(
						vformat("Byte %x is not a correct continuation byte after %x", c2, c));
				}
				else if (c3 == 0) {
					print_unicode_error(
						vformat("Missing %x %x UTF-8 continuation byte", c, c2), true);
				}
				else {
					print_unicode_error(vformat(
						"Byte %x is not a correct continuation byte after %x %x", c3, c, c2));
					// The unicode specification, in paragraphe 3.9 "Unicode Encoding Forms"
					// Conformance state : "Only when a sequence of two or three bytes is a
					// truncated version of a sequence which is otherwise well-formed to that point,
					// is more than one byte replaced with a single U+FFFD" So here we replace the
					// first 2 bytes with one single replacement_char.
					size = 2;
				}

				result = Error::ERR_INVALID_DATA;
			}
		}
		else if ((c & 0b11111000) == 0b11110000) {
			uint32_t range_min = (c == 0xF0) ? 0x90 : 0x80;
			uint32_t range_max = (c == 0xF4) ? 0x8F : 0xBF;

			uint8_t c2 = ((ptrtmp + 1) < ptr_limit) ? *(ptrtmp + 1) : 0;
			uint8_t c3 = ((ptrtmp + 2) < ptr_limit) ? *(ptrtmp + 2) : 0;
			uint8_t c4 = ((ptrtmp + 3) < ptr_limit) ? *(ptrtmp + 3) : 0;

			bool c2_valid = c2 && (c2 >= range_min) && (c2 <= range_max);
			bool c3_valid = c3 && ((c3 & 0b11000000) == 0b10000000);
			bool c4_valid = c4 && ((c4 & 0b11000000) == 0b10000000);

			if (c2_valid && c3_valid && c4_valid) {
				unicode = (uint32_t)((c & 0b00000111) << 18) | (uint32_t)((c2 & 0b00111111) << 12) |
						  (uint32_t)((c3 & 0b00111111) << 6) | (uint32_t)(c4 & 0b00111111);

				if (unicode < 0x10000) {
					unicode = _replacement_char;
					print_unicode_error(vformat("Overlong encoding (%x %x %x %x)", c, c2, c3, c4));
					result = Error::ERR_INVALID_DATA;
				}
				else if (unicode > 0x10FFFF) {
					unicode = _replacement_char;
					print_unicode_error(vformat("Invalid unicode codepoint (%d)", unicode), true);
					result = Error::ERR_INVALID_DATA;
				}
				else {
					size = 4;
				}
			}
			else {
				if (c2 == 0) {
					print_unicode_error(vformat("Missing %x UTF-8 continuation byte", c), true);
				}
				else if (c2_valid == false) {
					print_unicode_error(
						vformat("Byte %x is not a correct continuation byte after %x", c2, c));
				}
				else if (c3 == 0) {
					print_unicode_error(
						vformat("Missing %x %x UTF-8 continuation byte", c, c2), true);
				}
				else if (c3_valid == false) {
					print_unicode_error(vformat(
						"Byte %x is not a correct continuation byte after %x %x", c3, c, c2));
					size = 2;
				}
				else if (c4 == 0) {
					print_unicode_error(
						vformat("Missing %x %x %x UTF-8 continuation byte", c, c2, c3), true);
				}
				else {
					print_unicode_error(
						vformat("Byte %x is not a correct continuation byte after %x %x %x", c4, c,
							c2, c3));
					size = 3;
				}

				result = Error::ERR_INVALID_DATA;
			}
		}
		else {
			print_unicode_error(vformat("Invalid UTF-8 leading byte (%x)", c), true);
			result = Error::ERR_INVALID_DATA;
		}

		(*dst++) = unicode;
		ptrtmp += size;
	}

	(*dst++) = 0;
	resize_uninitialized(dst - ptr());

	return result;
}

CharString String::utf8(Vector<uint8_t>* r_ch_length_map) const
{
	int l = length();
	if (!l) {
		return CharString();
	}

	uint8_t* map_ptr = nullptr;
	if (r_ch_length_map) {
		r_ch_length_map->resize_uninitialized(l);
		map_ptr = r_ch_length_map->ptrw();
	}

	const char32_t* d = &operator[](0);
	int fl = 0;
	for (int i = 0; i < l; i++) {
		uint32_t c = d[i];
		int ch_w = 1;
		if (c <= 0x7f) { // 7 bits.
			ch_w = 1;
		}
		else if (c <= 0x7ff) { // 11 bits
			ch_w = 2;
		}
		else if (c <= 0xffff) { // 16 bits
			ch_w = 3;
		}
		else if (c <= 0x001fffff) { // 21 bits
			ch_w = 4;
		}
		else if (c <= 0x03ffffff) { // 26 bits
			ch_w = 5;
			print_unicode_error(vformat("Invalid unicode codepoint (%x)", c));
		}
		else if (c <= 0x7fffffff) { // 31 bits
			ch_w = 6;
			print_unicode_error(vformat("Invalid unicode codepoint (%x)", c));
		}
		else {
			ch_w = 1;
			print_unicode_error(
				vformat("Invalid unicode codepoint (%x), cannot represent as UTF-8", c), true);
		}
		fl += ch_w;
		if (map_ptr) {
			map_ptr[i] = ch_w;
		}
	}

	CharString utf8s;
	if (fl == 0) {
		return utf8s;
	}

	utf8s.resize_uninitialized(fl + 1);
	uint8_t* cdst = (uint8_t*)utf8s.get_data();

#define APPEND_CHAR(m_c) *(cdst++) = m_c

	for (int i = 0; i < l; i++) {
		uint32_t c = d[i];

		if (c <= 0x7f) { // 7 bits.
			APPEND_CHAR(c);
		}
		else if (c <= 0x7ff) {							  // 11 bits
			APPEND_CHAR(uint32_t(0xc0 | ((c >> 6) & 0x1f)));  // Top 5 bits.
			APPEND_CHAR(uint32_t(0x80 | (c & 0x3f)));		  // Bottom 6 bits.
		}
		else if (c <= 0xffff) {							  // 16 bits
			APPEND_CHAR(uint32_t(0xe0 | ((c >> 12) & 0x0f))); // Top 4 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 6) & 0x3f)));  // Middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | (c & 0x3f)));		  // Bottom 6 bits.
		}
		else if (c <= 0x001fffff) {						  // 21 bits
			APPEND_CHAR(uint32_t(0xf0 | ((c >> 18) & 0x07))); // Top 3 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 12) & 0x3f))); // Upper middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 6) & 0x3f)));  // Lower middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | (c & 0x3f)));		  // Bottom 6 bits.
		}
		else if (c <= 0x03ffffff) {						  // 26 bits
			APPEND_CHAR(uint32_t(0xf8 | ((c >> 24) & 0x03))); // Top 2 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 18) & 0x3f))); // Upper middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 12) & 0x3f))); // middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 6) & 0x3f)));  // Lower middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | (c & 0x3f)));		  // Bottom 6 bits.
		}
		else if (c <= 0x7fffffff) {						  // 31 bits
			APPEND_CHAR(uint32_t(0xfc | ((c >> 30) & 0x01))); // Top 1 bit.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 24) & 0x3f))); // Upper upper middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 18) & 0x3f))); // Lower upper middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 12) & 0x3f))); // Upper lower middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | ((c >> 6) & 0x3f)));  // Lower lower middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | (c & 0x3f)));		  // Bottom 6 bits.
		}
		else {
			// the string is a valid UTF32, so it should never happen ...
			print_unicode_error(vformat("Non scalar value (%x)", c), true);
			APPEND_CHAR(uint32_t(0xe0 | ((_replacement_char >> 12) & 0x0f))); // Top 4 bits.
			APPEND_CHAR(uint32_t(0x80 | ((_replacement_char >> 6) & 0x3f)));  // Middle 6 bits.
			APPEND_CHAR(uint32_t(0x80 | (_replacement_char & 0x3f)));		  // Bottom 6 bits.
		}
	}
#undef APPEND_CHAR
	*cdst = 0; // trailing zero

	return utf8s;
}

Error String::append_utf16(const char16_t* p_utf16, int p_len, bool p_default_little_endian)
{
	if (!p_utf16) {
		return ERR_INVALID_DATA;
	}

	int cstr_size = 0;
	int str_size = 0;

	bool byteswap = !p_default_little_endian;
	/* HANDLE BOM (Byte Order Mark) */
	if (p_len < 0 || p_len >= 1) {
		bool has_bom = false;
		if (uint16_t(p_utf16[0]) == 0xfeff) { // correct BOM, read as is
			has_bom = true;
			byteswap = false;
		}
		else if (uint16_t(p_utf16[0]) == 0xfffe) { // backwards BOM, swap bytes
			has_bom = true;
			byteswap = true;
		}
		if (has_bom) {
			if (p_len >= 0) {
				p_len -= 1;
			}
			p_utf16 += 1;
		}
	}

	bool decode_error = false;
	{
		const char16_t* ptrtmp = p_utf16;
		const char16_t* ptrtmp_limit = p_len >= 0 ? &p_utf16[p_len] : nullptr;
		uint32_t c_prev = 0;
		bool skip = false;
		while (ptrtmp != ptrtmp_limit && *ptrtmp) {
			uint32_t c = (byteswap) ? BSWAP16(*ptrtmp) : *ptrtmp;

			if ((c & 0xfffffc00) == 0xd800) { // lead surrogate
				if (skip) {
					print_unicode_error(
						vformat("Unpaired lead surrogate (%x [trail?] %x)", c_prev, c));
					decode_error = true;
				}
				skip = true;
			}
			else if ((c & 0xfffffc00) == 0xdc00) { // trail surrogate
				if (skip) {
					str_size--;
				}
				else {
					print_unicode_error(
						vformat("Unpaired trail surrogate (%x [lead?] %x)", c_prev, c));
					decode_error = true;
				}
				skip = false;
			}
			else {
				skip = false;
			}

			c_prev = c;
			str_size++;
			cstr_size++;
			ptrtmp++;
		}

		if (skip) {
			print_unicode_error(vformat("Unpaired lead surrogate (%x [eol])", c_prev));
			decode_error = true;
		}
	}

	if (str_size == 0) {
		clear();
		return OK; // empty string
	}

	const int prev_length = length();
	resize_uninitialized(prev_length + str_size + 1);
	char32_t* dst = ptrw() + prev_length;
	dst[str_size] = 0;

	bool skip = false;
	uint32_t c_prev = 0;
	while (cstr_size) {
		uint32_t c = (byteswap) ? BSWAP16(*p_utf16) : *p_utf16;

		if ((c & 0xfffffc00) == 0xd800) { // lead surrogate
			if (skip) {
				*(dst++) = c_prev; // unpaired, store as is
			}
			skip = true;
		}
		else if ((c & 0xfffffc00) == 0xdc00) { // trail surrogate
			if (skip) {
				*(dst++) =
					(c_prev << 10UL) + c - ((0xd800 << 10UL) + 0xdc00 - 0x10000); // decode pair
			}
			else {
				*(dst++) = c; // unpaired, store as is
			}
			skip = false;
		}
		else {
			*(dst++) = c;
			skip = false;
		}

		cstr_size--;
		p_utf16++;
		c_prev = c;
	}

	if (skip) {
		*(dst++) = c_prev;
	}

	if (decode_error) {
		return ERR_PARSE_ERROR;
	}
	else {
		return OK;
	}
}

Char16String String::utf16() const
{
	int l = length();
	if (!l) {
		return Char16String();
	}

	const char32_t* d = &operator[](0);
	int fl = 0;
	for (int i = 0; i < l; i++) {
		uint32_t c = d[i];
		if (c <= 0xffff) { // 16 bits.
			fl += 1;
			if ((c & 0xfffff800) == 0xd800) {
				print_unicode_error(vformat("Unpaired surrogate (%x)", c));
			}
		}
		else if (c <= 0x10ffff) { // 32 bits.
			fl += 2;
		}
		else {
			print_unicode_error(
				vformat("Invalid unicode codepoint (%x), cannot represent as UTF-16", c), true);
			fl += 1;
		}
	}

	Char16String utf16s;
	if (fl == 0) {
		return utf16s;
	}

	utf16s.resize_uninitialized(fl + 1);
	uint16_t* cdst = (uint16_t*)utf16s.get_data();

#define APPEND_CHAR(m_c) *(cdst++) = m_c

	for (int i = 0; i < l; i++) {
		uint32_t c = d[i];

		if (c <= 0xffff) { // 16 bits.
			APPEND_CHAR(c);
		}
		else if (c <= 0x10ffff) {						 // 32 bits.
			APPEND_CHAR(uint32_t((c >> 10) + 0xd7c0));	 // lead surrogate.
			APPEND_CHAR(uint32_t((c & 0x3ff) | 0xdc00)); // trail surrogate.
		}
		else {
			// the string is a valid UTF32, so it should never happen ...
			APPEND_CHAR(uint32_t((_replacement_char >> 10) + 0xd7c0));
			APPEND_CHAR(uint32_t((_replacement_char & 0x3ff) | 0xdc00));
		}
	}
#undef APPEND_CHAR
	*cdst = 0; // trailing zero

	return utf16s;
}

int64_t String::hex_to_int() const
{
	int len = length();
	if (len == 0) {
		return 0;
	}

	const char32_t* s = ptr();

	int64_t sign = s[0] == '-' ? -1 : 1;

	if (sign < 0) {
		s++;
	}

	if (len > 2 && s[0] == '0' && lower_case(s[1]) == 'x') {
		s += 2;
	}

	int64_t hex = 0;

	while (*s) {
		char32_t c = lower_case(*s);
		int64_t n;
		if (is_digit(c)) {
			n = c - '0';
		}
		else if (c >= 'a' && c <= 'f') {
			n = (c - 'a') + 10;
		}
		else {
			ERR_FAIL_V_MSG(0,
				vformat(R"(Invalid hexadecimal notation character "%c" (U+%04X) in string "%s".)",
					*s, static_cast<int32_t>(*s), *this));
		}
		// Check for overflow/underflow, with special case to ensure INT64_MIN does not result in
		// error
		bool overflow = ((hex > INT64_MAX / 16) &&
							(sign == 1 || (sign == -1 && hex != (INT64_MAX >> 4) + 1))) ||
						(sign == -1 && hex == (INT64_MAX >> 4) + 1 && c > '0');
		ERR_FAIL_COND_V_MSG(overflow, sign == 1 ? INT64_MAX : INT64_MIN,
			"Cannot represent " + *this + " as a 64-bit signed integer, since the value is " +
				(sign == 1 ? "too large." : "too small."));
		hex *= 16;
		hex += n;
		s++;
	}

	return hex * sign;
}

int64_t String::bin_to_int() const
{
	int len = length();
	if (len == 0) {
		return 0;
	}

	const char32_t* s = ptr();

	int64_t sign = s[0] == '-' ? -1 : 1;

	if (sign < 0) {
		s++;
	}

	if (len > 2 && s[0] == '0' && lower_case(s[1]) == 'b') {
		s += 2;
	}

	int64_t binary = 0;

	while (*s) {
		char32_t c = lower_case(*s);
		int64_t n;
		if (c == '0' || c == '1') {
			n = c - '0';
		}
		else {
			return 0;
		}
		// Check for overflow/underflow, with special case to ensure INT64_MIN does not result in
		// error
		bool overflow = ((binary > INT64_MAX / 2) &&
							(sign == 1 || (sign == -1 && binary != (INT64_MAX >> 1) + 1))) ||
						(sign == -1 && binary == (INT64_MAX >> 1) + 1 && c > '0');
		ERR_FAIL_COND_V_MSG(overflow, sign == 1 ? INT64_MAX : INT64_MIN,
			"Cannot represent " + *this + " as a 64-bit signed integer, since the value is " +
				(sign == 1 ? "too large." : "too small."));
		binary *= 2;
		binary += n;
		s++;
	}

	return binary * sign;
}

template <typename C, typename T> _ALWAYS_INLINE_ int64_t _to_int(const T& p_in, int p_to)
{
	// Accumulate the total number in an unsigned integer as the range is:
	// +9223372036854775807 to -9223372036854775808 and the smallest negative
	// number does not fit inside an int64_t. So we accumulate the positive
	// number in an unsigned, and then at the very end convert to its signed
	// form.
	uint64_t integer = 0;
	uint8_t digits = 0;
	bool positive = true;

	for (int i = 0; i < p_to; i++) {
		C c = p_in[i];
		if (is_digit(c)) {
			// No need to do expensive checks unless we're approaching INT64_MAX / INT64_MIN.
			if (unlikely(digits > 18)) {
				bool overflow = (integer > INT64_MAX / 10) ||
								(integer == INT64_MAX / 10 &&
									((positive && c > '7') || (!positive && c > '8')));
				ERR_FAIL_COND_V_MSG(overflow, positive ? INT64_MAX : INT64_MIN,
					"Cannot represent " + String(p_in) +
						" as a 64-bit signed integer, since the value is " +
						(positive ? "too large." : "too small."));
			}

			integer *= 10;
			integer += c - '0';
			++digits;

		}
		else if (integer == 0 && c == '-') {
			positive = !positive;
		}
	}

	if (positive) {
		return int64_t(integer);
	}
	else {
		return int64_t(integer * uint64_t(-1));
	}
}

int64_t String::to_int() const
{
	if (length() == 0) {
		return 0;
	}

	int to = (find_char('.') >= 0) ? find_char('.') : length();

	return _to_int<char32_t>(*this, to);
}

int64_t String::to_int(const char* p_str, int p_len)
{
	int to = 0;
	if (p_len >= 0) {
		to = p_len;
	}
	else {
		while (p_str[to] != 0 && p_str[to] != '.') {
			to++;
		}
	}

	return _to_int<char>(p_str, to);
}

int64_t String::to_int(const wchar_t* p_str, int p_len)
{
	int to = 0;
	if (p_len >= 0) {
		to = p_len;
	}
	else {
		while (p_str[to] != 0 && p_str[to] != '.') {
			to++;
		}
	}

	return _to_int<wchar_t>(p_str, to);
}

bool String::is_numeric() const
{
	if (length() == 0) {
		return false;
	}

	int s = 0;
	if (operator[](0) == '-') {
		++s;
	}
	bool dot = false;
	for (int i = s; i < length(); i++) {
		char32_t c = operator[](i);
		if (c == '.') {
			if (dot) {
				return false;
			}
			dot = true;
		}
		else if (!is_digit(c)) {
			return false;
		}
	}

	return true; // TODO: Use the parser below for this instead
}

template <typename C>
static double built_in_strtod(
	/* A decimal ASCII floating-point number,
	 * optionally preceded by white space. Must
	 * have form "-I.FE-X", where I is the integer
	 * part of the mantissa, F is the fractional
	 * part of the mantissa, and X is the
	 * exponent. Either of the signs may be "+",
	 * "-", or omitted. Either I or F may be
	 * omitted, or both. The decimal point isn't
	 * necessary unless F is present. The "E" may
	 * actually be an "e". E and X may both be
	 * omitted (but not just one). */
	const C* p_string,
	/* If non-nullptr, store terminating Cacter's
	 * address here. */
	C** r_end = nullptr)
{
	/* Largest possible base 10 exponent. Any
	 * exponent larger than this will already
	 * produce underflow or overflow, so there's
	 * no need to worry about additional digits. */
	static const int maxExponent = 511;
	/* Table giving binary powers of 10. Entry
	 * is 10^2^i. Used to convert decimal
	 * exponents into floating-point numbers. */
	static const double powersOf10[] = {
		10., 100., 1.0e4, 1.0e8, 1.0e16, 1.0e32, 1.0e64, 1.0e128, 1.0e256};

	bool sign, expSign = false;
	double fraction, dblExp;
	const double* d;
	const C* p;
	int c;
	/* Exponent read from "EX" field. */
	int exp = 0;
	/* Exponent that derives from the fractional
	 * part. Under normal circumstances, it is
	 * the negative of the number of digits in F.
	 * However, if I is very long, the last digits
	 * of I get dropped (otherwise a long I with a
	 * large negative exponent could cause an
	 * unnecessary overflow on I alone). In this
	 * case, fracExp is incremented one for each
	 * dropped digit. */
	int fracExp = 0;
	/* Number of digits in mantissa. */
	int mantSize;
	/* Number of mantissa digits BEFORE decimal point. */
	int decPt;
	/* Temporarily holds location of exponent in string. */
	const C* pExp;

	/*
	 * Strip off leading blanks and check for a sign.
	 */

	p = p_string;
	while (*p == ' ' || *p == '\t' || *p == '\n') {
		p += 1;
	}
	if (*p == '-') {
		sign = true;
		p += 1;
	}
	else {
		if (*p == '+') {
			p += 1;
		}
		sign = false;
	}

	/*
	 * Count the number of digits in the mantissa (including the decimal
	 * point), and also locate the decimal point.
	 */

	decPt = -1;
	for (mantSize = 0;; mantSize += 1) {
		c = *p;
		if (!is_digit(c)) {
			if ((c != '.') || (decPt >= 0)) {
				break;
			}
			decPt = mantSize;
		}
		p += 1;
	}

	/*
	 * Now suck up the digits in the mantissa. Use two integers to collect 9
	 * digits each (this is faster than using floating-point). If the mantissa
	 * has more than 18 digits, ignore the extras, since they can't affect the
	 * value anyway.
	 */

	pExp = p;
	p -= mantSize;
	if (decPt < 0) {
		decPt = mantSize;
	}
	else {
		mantSize -= 1; /* One of the digits was the point. */
	}
	if (mantSize > 18) {
		fracExp = decPt - 18;
		mantSize = 18;
	}
	else {
		fracExp = decPt - mantSize;
	}
	if (mantSize == 0) {
		fraction = 0.0;
		p = p_string;
		goto done;
	}
	else {
		int frac1, frac2;

		frac1 = 0;
		for (; mantSize > 9; mantSize -= 1) {
			c = *p;
			p += 1;
			if (c == '.') {
				c = *p;
				p += 1;
			}
			frac1 = 10 * frac1 + (c - '0');
		}
		frac2 = 0;
		for (; mantSize > 0; mantSize -= 1) {
			c = *p;
			p += 1;
			if (c == '.') {
				c = *p;
				p += 1;
			}
			frac2 = 10 * frac2 + (c - '0');
		}
		fraction = (1.0e9 * frac1) + frac2;
	}

	/*
	 * Skim off the exponent.
	 */

	p = pExp;
	if ((*p == 'E') || (*p == 'e')) {
		p += 1;
		if (*p == '-') {
			expSign = true;
			p += 1;
		}
		else {
			if (*p == '+') {
				p += 1;
			}
			expSign = false;
		}
		if (!is_digit(char32_t(*p))) {
			p = pExp;
			goto done;
		}
		while (is_digit(char32_t(*p))) {
			exp = exp * 10 + (*p - '0');
			p += 1;
		}
	}
	if (expSign) {
		exp = fracExp - exp;
	}
	else {
		exp = fracExp + exp;
	}

	/*
	 * Generate a floating-point number that represents the exponent. Do this
	 * by processing the exponent one bit at a time to combine many powers of
	 * 2 of 10. Then combine the exponent with the fraction.
	 */

	if (exp < 0) {
		expSign = true;
		exp = -exp;
	}
	else {
		expSign = false;
	}

	if (exp > maxExponent) {
		exp = maxExponent;
		WARN_PRINT("Exponent too high");
	}
	dblExp = 1.0;
	for (d = powersOf10; exp != 0; exp >>= 1, ++d) {
		if (exp & 01) {
			dblExp *= *d;
		}
	}
	if (expSign) {
		fraction /= dblExp;
	}
	else {
		fraction *= dblExp;
	}

done:
	if (r_end != nullptr) {
		*r_end = (C*)p;
	}

	if (sign) {
		return -fraction;
	}
	return fraction;
}

#define READING_SIGN 0
#define READING_INT 1
#define READING_DEC 2
#define READING_EXP 3
#define READING_DONE 4

double String::to_float(const char* p_str) { return built_in_strtod<char>(p_str); }

double String::to_float(const char32_t* p_str, const char32_t** r_end)
{
	return built_in_strtod<char32_t>(p_str, (char32_t**)r_end);
}

double String::to_float(const wchar_t* p_str, const wchar_t** r_end)
{
	return built_in_strtod<wchar_t>(p_str, (wchar_t**)r_end);
}

uint32_t String::num_characters(int64_t p_int)
{
	int r = 1;
	if (p_int < 0) {
		r += 1;
		if (p_int == INT64_MIN) {
			p_int = INT64_MAX;
		}
		else {
			p_int = -p_int;
		}
	}
	while (p_int >= 10) {
		p_int /= 10;
		r++;
	}
	return r;
}

int64_t String::to_int(const char32_t* p_str, int p_len, bool p_clamp)
{
	if (p_len == 0 || !p_str[0]) {
		return 0;
	}
	///@todo make more exact so saving and loading does not lose precision

	int64_t integer = 0;
	int64_t sign = 1;
	int reading = READING_SIGN;

	const char32_t* str = p_str;
	const char32_t* limit = &p_str[p_len];

	while (*str && reading != READING_DONE && str != limit) {
		char32_t c = *(str++);
		switch (reading) {
		case READING_SIGN: {
			if (is_digit(c)) {
				reading = READING_INT;
				// let it fallthrough
			}
			else if (c == '-') {
				sign = -1;
				reading = READING_INT;
				break;
			}
			else if (c == '+') {
				sign = 1;
				reading = READING_INT;
				break;
			}
			else {
				break;
			}
			[[fallthrough]];
		}
		case READING_INT: {
			if (is_digit(c)) {
				if (integer > INT64_MAX / 10) {
					String number("");
					str = p_str;
					while (*str && str != limit) {
						number += *(str++);
					}
					if (p_clamp) {
						if (sign == 1) {
							return INT64_MAX;
						}
						else {
							return INT64_MIN;
						}
					}
					else {
						ERR_FAIL_V_MSG(sign == 1 ? INT64_MAX : INT64_MIN,
							"Cannot represent " + number +
								" as a 64-bit signed integer, since the value is " +
								(sign == 1 ? "too large." : "too small."));
					}
				}
				integer *= 10;
				integer += c - '0';
			}
			else {
				reading = READING_DONE;
			}

		} break;
		}
	}

	return sign * integer;
}

double String::to_float() const
{
	if (is_empty()) {
		return 0;
	}
	return built_in_strtod<char32_t>(get_data());
}

uint32_t String::hash(const char* p_cstr)
{
	// static_cast: avoid negative values on platforms where char is signed.
	uint32_t hashv = 5381;
	uint32_t c = static_cast<uint8_t>(*p_cstr++);

	while (c) {
		hashv = ((hashv << 5) + hashv) + c; /* hash * 33 + c */
		c = static_cast<uint8_t>(*p_cstr++);
	}

	return hashv;
}

uint32_t String::hash(const char* p_cstr, int p_len)
{
	uint32_t hashv = 5381;
	for (int i = 0; i < p_len; i++) {
		// static_cast: avoid negative values on platforms where char is signed.
		hashv = ((hashv << 5) + hashv) + static_cast<uint8_t>(p_cstr[i]); /* hash * 33 + c */
	}

	return hashv;
}

uint32_t String::hash(const wchar_t* p_cstr, int p_len)
{
	// Avoid negative values on platforms where wchar_t is signed. Account for different sizes.
	using wide_unsigned = std::conditional<sizeof(wchar_t) == 2, uint16_t, uint32_t>::type;

	uint32_t hashv = 5381;
	for (int i = 0; i < p_len; i++) {
		hashv = ((hashv << 5) + hashv) + static_cast<wide_unsigned>(p_cstr[i]); /* hash * 33 + c */
	}

	return hashv;
}

uint32_t String::hash(const wchar_t* p_cstr)
{
	// Avoid negative values on platforms where wchar_t is signed. Account for different sizes.
	using wide_unsigned = std::conditional<sizeof(wchar_t) == 2, uint16_t, uint32_t>::type;

	uint32_t hashv = 5381;
	uint32_t c = static_cast<wide_unsigned>(*p_cstr++);

	while (c) {
		hashv = ((hashv << 5) + hashv) + c; /* hash * 33 + c */
		c = static_cast<wide_unsigned>(*p_cstr++);
	}

	return hashv;
}

uint32_t String::hash(const char32_t* p_cstr, int p_len)
{
	uint32_t hashv = 5381;
	for (int i = 0; i < p_len; i++) {
		hashv = ((hashv << 5) + hashv) + p_cstr[i]; /* hash * 33 + c */
	}

	return hashv;
}

uint32_t String::hash(const char32_t* p_cstr)
{
	uint32_t hashv = 5381;
	uint32_t c = *p_cstr++;

	while (c) {
		hashv = ((hashv << 5) + hashv) + c; /* hash * 33 + c */
		c = *p_cstr++;
	}

	return hashv;
}

uint32_t String::hash() const
{
	/* simple djb2 hashing */

	const char32_t* chr = get_data();
	uint32_t hashv = 5381;
	uint32_t c = *chr++;

	while (c) {
		hashv = ((hashv << 5) + hashv) + c; /* hash * 33 + c */
		c = *chr++;
	}

	return hashv;
}

uint64_t String::hash64() const
{
	/* simple djb2 hashing */

	const char32_t* chr = get_data();
	uint64_t hashv = 5381;
	uint64_t c = *chr++;

	while (c) {
		hashv = ((hashv << 5) + hashv) + c; /* hash * 33 + c */
		c = *chr++;
	}

	return hashv;
}

String String::md5_text() const
{
	CharString cs = utf8();
	unsigned char hash[16];
	CryptoCore::md5((unsigned char*)cs.ptr(), cs.length(), hash);
	return String::hex_encode_buffer(hash, 16);
}

String String::sha1_text() const
{
	CharString cs = utf8();
	unsigned char hash[20];
	CryptoCore::sha1((unsigned char*)cs.ptr(), cs.length(), hash);
	return String::hex_encode_buffer(hash, 20);
}

String String::sha256_text() const
{
	CharString cs = utf8();
	unsigned char hash[32];
	CryptoCore::sha256((unsigned char*)cs.ptr(), cs.length(), hash);
	return String::hex_encode_buffer(hash, 32);
}

Vector<uint8_t> String::md5_buffer() const
{
	CharString cs = utf8();
	unsigned char hash[16];
	CryptoCore::md5((unsigned char*)cs.ptr(), cs.length(), hash);

	Vector<uint8_t> ret;
	ret.resize_uninitialized(16);
	uint8_t* ret_ptrw = ret.ptrw();
	for (int i = 0; i < 16; i++) {
		ret_ptrw[i] = hash[i];
	}
	return ret;
}

Vector<uint8_t> String::sha1_buffer() const
{
	CharString cs = utf8();
	unsigned char hash[20];
	CryptoCore::sha1((unsigned char*)cs.ptr(), cs.length(), hash);

	Vector<uint8_t> ret;
	ret.resize_uninitialized(20);
	uint8_t* ret_ptrw = ret.ptrw();
	for (int i = 0; i < 20; i++) {
		ret_ptrw[i] = hash[i];
	}

	return ret;
}

Vector<uint8_t> String::sha256_buffer() const
{
	CharString cs = utf8();
	unsigned char hash[32];
	CryptoCore::sha256((unsigned char*)cs.ptr(), cs.length(), hash);

	Vector<uint8_t> ret;
	ret.resize_uninitialized(32);
	uint8_t* ret_ptrw = ret.ptrw();
	for (int i = 0; i < 32; i++) {
		ret_ptrw[i] = hash[i];
	}
	return ret;
}

String String::insert(int p_at_pos, const String& p_string) const
{
	if (p_string.is_empty() || p_at_pos < 0) {
		return *this;
	}

	if (p_at_pos > length()) {
		p_at_pos = length();
	}

	String ret;
	ret.resize_uninitialized(length() + p_string.length() + 1);
	char32_t* ret_ptrw = ret.ptrw();
	const char32_t* this_ptr = ptr();

	if (p_at_pos > 0) {
		memcpy(ret_ptrw, this_ptr, p_at_pos * sizeof(char32_t));
		ret_ptrw += p_at_pos;
	}

	memcpy(ret_ptrw, p_string.ptr(), p_string.length() * sizeof(char32_t));
	ret_ptrw += p_string.length();

	if (p_at_pos < length()) {
		memcpy(ret_ptrw, this_ptr + p_at_pos, (length() - p_at_pos) * sizeof(char32_t));
		ret_ptrw += length() - p_at_pos;
	}

	*ret_ptrw = 0;

	return ret;
}

String String::erase(int p_pos, int p_chars) const
{
	ERR_FAIL_COND_V_MSG(p_pos < 0, "",
		vformat("Invalid starting position for `String.erase()`: %d. Starting position must be "
				"positive or zero.",
			p_pos));
	ERR_FAIL_COND_V_MSG(p_chars < 0, "",
		vformat("Invalid character count for `String.erase()`: %d. Character count must be "
				"positive or zero.",
			p_chars));
	return left(p_pos) + substr(p_pos + p_chars);
}

template <class T> static bool _contains_char(char32_t p_c, const T* p_chars, int p_chars_len)
{
	for (int i = 0; i < p_chars_len; ++i) {
		if (p_c == (char32_t)p_chars[i]) {
			return true;
		}
	}

	return false;
}

String String::remove_char(char32_t p_char) const
{
	if (p_char == 0) {
		return *this;
	}

	int len = length();
	if (len == 0) {
		return *this;
	}

	int index = 0;
	const char32_t* old_ptr = ptr();
	for (; index < len; ++index) {
		if (old_ptr[index] == p_char) {
			break;
		}
	}

	// If no occurrence of `char` was found, return this.
	if (index == len) {
		return *this;
	}

	// If we found at least one occurrence of `char`, create new string, allocating enough space for
	// the current length minus one.
	String new_string;
	new_string.resize_uninitialized(len);
	char32_t* new_ptr = new_string.ptrw();

	// Copy part of input before `char`.
	memcpy(new_ptr, old_ptr, index * sizeof(char32_t));

	int new_size = index;

	// Copy rest, skipping `char`.
	for (++index; index < len; ++index) {
		const char32_t old_char = old_ptr[index];
		if (old_char != p_char) {
			new_ptr[new_size] = old_char;
			++new_size;
		}
	}

	new_ptr[new_size] = _null;

	// Shrink new string to fit.
	new_string.resize_uninitialized(new_size + 1);

	return new_string;
}

template <class T>
static String _remove_chars_common(const String& p_this, const T* p_chars, int p_chars_len)
{
	// Delegate if p_chars has a single element.
	if (p_chars_len == 1) {
		return p_this.remove_char(*p_chars);
	}
	else if (p_chars_len == 0) {
		return p_this;
	}

	int len = p_this.length();

	if (len == 0) {
		return p_this;
	}

	int index = 0;
	const char32_t* old_ptr = p_this.ptr();
	for (; index < len; ++index) {
		if (_contains_char(old_ptr[index], p_chars, p_chars_len)) {
			break;
		}
	}

	// If no occurrence of `chars` was found, return this.
	if (index == len) {
		return p_this;
	}

	// If we found at least one occurrence of `chars`, create new string, allocating enough space
	// for the current length minus one.
	String new_string;
	new_string.resize_uninitialized(len);
	char32_t* new_ptr = new_string.ptrw();

	// Copy part of input before `char`.
	memcpy(new_ptr, old_ptr, index * sizeof(char32_t));

	int new_size = index;

	// Copy rest, skipping `chars`.
	for (++index; index < len; ++index) {
		const char32_t old_char = old_ptr[index];
		if (!_contains_char(old_char, p_chars, p_chars_len)) {
			new_ptr[new_size] = old_char;
			++new_size;
		}
	}

	new_ptr[new_size] = 0;

	// Shrink new string to fit.
	new_string.resize_uninitialized(new_size + 1);

	return new_string;
}

String String::remove_chars(const String& p_chars) const
{
	return _remove_chars_common(*this, p_chars.ptr(), p_chars.length());
}

String String::remove_chars(const char* p_chars) const
{
	return _remove_chars_common(*this, p_chars, strlen(p_chars));
}

String String::substr(int p_from, int p_chars) const
{
	if (p_chars == -1) {
		p_chars = length() - p_from;
	}

	if (is_empty() || p_from < 0 || p_from >= length() || p_chars <= 0) {
		return "";
	}

	if ((p_from + p_chars) > length()) {
		p_chars = length() - p_from;
	}

	if (p_from == 0 && p_chars >= length()) {
		return String(*this);
	}

	String s;
	s.append_utf32_unchecked(Span(ptr() + p_from, p_chars));
	return s;
}

int String::find(const String& p_str, int p_from) const
{
	const int str_len = p_str.length();
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || p_str.is_empty()) {
		return -1; // Still out of bounds
	}

	if (p_str.length() == 1) {
		// Optimize with single-char implementation.
		return span().find(p_str[0], p_from);
	}

	return span().find_sequence(p_str.span(), p_from);
}

int String::find(const char* p_str, int p_from) const
{
	const int str_len = strlen(p_str);
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || str_len == 0) {
		return -1; // Still out of bounds
	}

	if (str_len == 1) {
		return find_char(*p_str, p_from); // Optimize with single-char find.
	}

	return span().find_sequence(Span((const unsigned char*)p_str, str_len), p_from);
}

int String::find_char(char32_t p_char, int p_from) const
{
	if (p_from < 0) {
		p_from = length() + p_from;
	}
	if (p_from < 0 || p_from >= length()) {
		return -1;
	}
	return span().find(p_char, p_from);
}

int String::findmk(const Vector<String>& p_keys, int p_from, int* r_key) const
{
	if (p_from < 0) {
		return -1;
	}
	if (p_keys.is_empty()) {
		return -1;
	}

	// int str_len=p_str.length();
	const String* keys = &p_keys[0];
	int key_count = p_keys.size();
	int len = length();

	if (len == 0) {
		return -1; // won't find anything!
	}

	const char32_t* src = get_data();

	for (int i = p_from; i < len; i++) {
		for (int k = 0; k < key_count; k++) {
			const int str_len = keys[k].length();

			if (i + str_len > len) {
				continue; // Can't find this key here.
			}

			const char32_t* str = keys[k].get_data();
			if (are_spans_equal(src + i, str, str_len)) {
				if (r_key) {
					*r_key = k;
				}
				return i;
			}
		}
	}

	return -1;
}

int String::findn(const String& p_str, int p_from) const
{
	const int str_len = p_str.length();
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || p_str.is_empty()) {
		return -1; // Still out of bounds
	}

	const char32_t* src = get_data();
	const char32_t* str = p_str.get_data();

	for (int i = p_from; i <= (len - str_len); i++) {
		if (strings_equal_lower(src + i, str, str_len)) {
			return i;
		}
	}

	return -1;
}

int String::findn(const char* p_str, int p_from) const
{
	const int str_len = strlen(p_str);
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || str_len == 0) {
		return -1; // Still out of bounds
	}

	const char32_t* src = get_data();

	for (int i = p_from; i <= (len - str_len); i++) {
		if (strings_equal_lower(src + i, p_str, str_len)) {
			return i;
		}
	}

	return -1;
}

int String::rfind(const String& p_str, int p_from) const
{
	const int str_len = p_str.length();
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || p_str.is_empty()) {
		return -1; // Still out of bounds
	}

	if (p_str.length() == 1) {
		// Optimize with single-char implementation.
		return span().rfind(p_str[0], p_from);
	}

	return span().rfind_sequence(p_str.span(), p_from);
}

int String::rfind(const char* p_str, int p_from) const
{
	const int str_len = strlen(p_str);
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || str_len == 0) {
		return -1; // Still out of bounds
	}

	if (str_len == 1) {
		// Optimize with single-char implementation.
		return span().rfind(p_str[0], p_from);
	}

	return span().rfind_sequence(Span((const unsigned char*)p_str, str_len), p_from);
}

int String::rfind_char(char32_t p_char, int p_from) const
{
	if (p_from < 0) {
		p_from = length() + p_from;
	}
	if (p_from < 0 || p_from >= length()) {
		return -1;
	}
	return span().rfind(p_char, p_from);
}

int String::rfindn(const String& p_str, int p_from) const
{
	const int str_len = p_str.length();
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || p_str.is_empty()) {
		return -1; // Still out of bounds
	}

	const char32_t* src = get_data();
	const char32_t* str = p_str.get_data();

	for (int i = p_from; i >= 0; i--) {
		if (strings_equal_lower(src + i, str, str_len)) {
			return i;
		}
	}

	return -1;
}

int String::rfindn(const char* p_str, int p_from) const
{
	const int str_len = strlen(p_str);
	const int len = length();

	if (p_from < 0) {
		p_from = len - str_len + p_from + 1;
	}
	if (p_from < 0 || p_from > len - str_len || str_len == 0) {
		return -1; // Still out of bounds
	}

	const char32_t* src = get_data();

	for (int i = p_from; i >= 0; i--) {
		if (strings_equal_lower(src + i, p_str, str_len)) {
			return i;
		}
	}

	return -1;
}

bool String::ends_with(const String& p_string) const
{
	const int l = p_string.length();
	if (l > length()) {
		return false;
	}
	if (l == 0) {
		return true;
	}

	return memcmp(ptr() + (length() - l), p_string.ptr(), l * sizeof(char32_t)) == 0;
}

bool String::ends_with(const char* p_string) const
{
	if (!p_string) {
		return false;
	}

	int l = strlen(p_string);
	if (l > length()) {
		return false;
	}

	if (l == 0) {
		return true;
	}

	const char32_t* s = &operator[](length() - l);

	for (int i = 0; i < l; i++) {
		if (static_cast<char32_t>(p_string[i]) != s[i]) {
			return false;
		}
	}

	return true;
}

bool String::begins_with(const String& p_string) const
{
	const int l = p_string.length();
	if (l > length()) {
		return false;
	}
	if (l == 0) {
		return true;
	}

	return memcmp(ptr(), p_string.ptr(), l * sizeof(char32_t)) == 0;
}

bool String::begins_with(const char* p_string) const
{
	if (!p_string) {
		return false;
	}

	int l = length();
	if (l == 0) {
		return *p_string == 0;
	}

	const char32_t* str = &operator[](0);
	int i = 0;

	while (*p_string && i < l) {
		if ((char32_t)*p_string != str[i]) {
			return false;
		}
		i++;
		p_string++;
	}

	return *p_string == 0;
}

bool String::is_enclosed_in(const String& p_string) const
{
	return begins_with(p_string) && ends_with(p_string);
}

bool String::is_subsequence_of(const String& p_string) const
{
	return _base_is_subsequence_of(p_string, false);
}

bool String::is_subsequence_ofn(const String& p_string) const
{
	return _base_is_subsequence_of(p_string, true);
}

bool String::is_quoted() const { return is_enclosed_in("\"") || is_enclosed_in("'"); }

bool String::is_lowercase() const
{
	for (const char32_t* str = &operator[](0); *str; str++) {
		if (is_unicode_upper_case(*str)) {
			return false;
		}
	}
	return true;
}

int String::_count(const String& p_string, int p_from, int p_to, bool p_case_insensitive) const
{
	if (p_string.is_empty()) {
		return 0;
	}
	int len = length();
	int slen = p_string.length();
	if (len < slen) {
		return 0;
	}
	String str;
	if (p_from >= 0 && p_to >= 0) {
		if (p_to == 0) {
			p_to = len;
		}
		else if (p_from >= p_to) {
			return 0;
		}
		if (p_from == 0 && p_to == len) {
			str = *this;
		}
		else {
			str = substr(p_from, p_to - p_from);
		}
	}
	else {
		return 0;
	}
	int c = 0;
	int idx = 0;
	while ((idx = p_case_insensitive ? str.findn(p_string, idx) : str.find(p_string, idx)) != -1) {
		// Skip the occurrence itself.
		idx += slen;
		++c;
	}
	return c;
}

int String::_count(const char* p_string, int p_from, int p_to, bool p_case_insensitive) const
{
	int substring_length = strlen(p_string);
	if (substring_length == 0) {
		return 0;
	}
	const int source_length = length();

	if (source_length < substring_length) {
		return 0;
	}
	String str;
	int search_limit = p_to;
	if (p_from >= 0 && p_to >= 0) {
		if (p_to == 0) {
			search_limit = source_length;
		}
		else if (p_from >= p_to) {
			return 0;
		}
		if (p_from == 0 && search_limit == source_length) {
			str = *this;
		}
		else {
			str = substr(p_from, search_limit - p_from);
		}
	}
	else {
		return 0;
	}
	int c = 0;
	int idx = 0;
	while ((idx = p_case_insensitive ? str.findn(p_string, idx) : str.find(p_string, idx)) != -1) {
		// Skip the occurrence itself.
		idx += substring_length;
		++c;
	}
	return c;
}

int String::count(const String& p_string, int p_from, int p_to) const
{
	return _count(p_string, p_from, p_to, false);
}

int String::count(const char* p_string, int p_from, int p_to) const
{
	return _count(p_string, p_from, p_to, false);
}

int String::countn(const String& p_string, int p_from, int p_to) const
{
	return _count(p_string, p_from, p_to, true);
}

int String::countn(const char* p_string, int p_from, int p_to) const
{
	return _count(p_string, p_from, p_to, true);
}

bool String::_base_is_subsequence_of(const String& p_string, bool p_case_insensitive) const
{
	int len = length();
	if (len == 0) {
		// Technically an empty string is subsequence of any string
		return true;
	}

	if (len > p_string.length()) {
		return false;
	}

	const char32_t* src = &operator[](0);
	const char32_t* tgt = &p_string[0];

	for (; *src && *tgt; tgt++) {
		bool match = false;
		if (p_case_insensitive) {
			char32_t srcc = _find_lower(*src);
			char32_t tgtc = _find_lower(*tgt);
			match = srcc == tgtc;
		}
		else {
			match = *src == *tgt;
		}
		if (match) {
			src++;
			if (!*src) {
				return true;
			}
		}
	}

	return false;
}

Vector<String> String::bigrams() const
{
	int n_pairs = length() - 1;
	Vector<String> b;
	if (n_pairs <= 0) {
		return b;
	}
	b.resize_initialized(n_pairs);
	String* b_ptrw = b.ptrw();
	for (int i = 0; i < n_pairs; i++) {
		b_ptrw[i] = substr(i, 2);
	}
	return b;
}

// Similarity according to Sorensen-Dice coefficient
float String::similarity(const String& p_string) const
{
	if (operator==(p_string)) {
		// Equal strings are totally similar
		return 1.0f;
	}
	if (length() < 2 || p_string.length() < 2) {
		// No way to calculate similarity without a single bigram
		return 0.0f;
	}

	const int src_size = length() - 1;
	const int tgt_size = p_string.length() - 1;

	const int sum = src_size + tgt_size;
	int inter = 0;
	for (int i = 0; i < src_size; i++) {
		const char32_t i0 = get(i);
		const char32_t i1 = get(i + 1);

		for (int j = 0; j < tgt_size; j++) {
			if (i0 == p_string.get(j) && i1 == p_string.get(j + 1)) {
				inter++;
				break;
			}
		}
	}

	return (2.0f * inter) / sum;
}

static bool _wildcard_match(
	const char32_t* p_pattern, const char32_t* p_string, bool p_case_sensitive)
{
	switch (*p_pattern) {
	case '\0':
		return !*p_string;
	case '*':
		return _wildcard_match(p_pattern + 1, p_string, p_case_sensitive) ||
			   (*p_string && _wildcard_match(p_pattern, p_string + 1, p_case_sensitive));
	case '?':
		return *p_string && (*p_string != '.') &&
			   _wildcard_match(p_pattern + 1, p_string + 1, p_case_sensitive);
	default:

		return (p_case_sensitive ? (*p_string == *p_pattern)
								 : (_find_upper(*p_string) == _find_upper(*p_pattern))) &&
			   _wildcard_match(p_pattern + 1, p_string + 1, p_case_sensitive);
	}
}

bool String::match(const String& p_wildcard) const
{
	if (!p_wildcard.length() || !length()) {
		return false;
	}

	return _wildcard_match(p_wildcard.get_data(), get_data(), true);
}

bool String::matchn(const String& p_wildcard) const
{
	if (!p_wildcard.length() || !length()) {
		return false;
	}
	return _wildcard_match(p_wildcard.get_data(), get_data(), false);
}

static String _replace_common(
	const String& p_this, const String& p_key, const String& p_with, bool p_case_insensitive)
{
	if (p_key.is_empty() || p_this.is_empty()) {
		return p_this;
	}

	const size_t key_length = p_key.length();

	int search_from = 0;
	int result = 0;

	LocalVector<int> found;

	while ((result = (p_case_insensitive ? p_this.findn(p_key, search_from)
										 : p_this.find(p_key, search_from))) >= 0) {
		found.push_back(result);
		ERR_FAIL_COND_V_MSG((result + key_length) > INT32_MAX, p_this, "Key length too long");
		search_from = result + key_length;
	}

	if (found.is_empty()) {
		return p_this;
	}

	String new_string;

	const int with_length = p_with.length();
	const int old_length = p_this.length();

	new_string.resize_uninitialized(
		old_length + int(found.size()) * (with_length - key_length) + 1);

	char32_t* new_ptrw = new_string.ptrw();
	const char32_t* old_ptr = p_this.ptr();
	const char32_t* with_ptr = p_with.ptr();

	int last_pos = 0;

	for (const int& pos : found) {
		if (last_pos != pos) {
			memcpy(new_ptrw, old_ptr + last_pos, (pos - last_pos) * sizeof(char32_t));
			new_ptrw += (pos - last_pos);
		}
		if (with_length) {
			memcpy(new_ptrw, with_ptr, with_length * sizeof(char32_t));
			new_ptrw += with_length;
		}
		last_pos = pos + key_length;
	}

	if (last_pos != old_length) {
		memcpy(new_ptrw, old_ptr + last_pos, (old_length - last_pos) * sizeof(char32_t));
		new_ptrw += old_length - last_pos;
	}

	*new_ptrw = 0;

	return new_string;
}

static String _replace_common(
	const String& p_this, const char* p_key, const char* p_with, bool p_case_insensitive)
{
	size_t key_length = strlen(p_key);

	if (key_length == 0 || p_this.is_empty()) {
		return p_this;
	}

	int search_from = 0;
	int result = 0;

	LocalVector<int> found;

	while ((result = (p_case_insensitive ? p_this.findn(p_key, search_from)
										 : p_this.find(p_key, search_from))) >= 0) {
		found.push_back(result);
		ERR_FAIL_COND_V_MSG((result + key_length) > INT32_MAX, p_this, "Key length too long");
		search_from = result + key_length;
	}

	if (found.is_empty()) {
		return p_this;
	}

	String new_string;

	// Create string to speed up copying as we can't do `memcopy` between `char32_t` and `char`.
	const String with_string(p_with);
	const int with_length = with_string.length();
	const int old_length = p_this.length();

	new_string.resize_uninitialized(
		old_length + int(found.size()) * (with_length - key_length) + 1);

	char32_t* new_ptrw = new_string.ptrw();
	const char32_t* old_ptr = p_this.ptr();
	const char32_t* with_ptr = with_string.ptr();

	int last_pos = 0;

	for (const int& pos : found) {
		if (last_pos != pos) {
			memcpy(new_ptrw, old_ptr + last_pos, (pos - last_pos) * sizeof(char32_t));
			new_ptrw += (pos - last_pos);
		}
		if (with_length) {
			memcpy(new_ptrw, with_ptr, with_length * sizeof(char32_t));
			new_ptrw += with_length;
		}
		last_pos = pos + key_length;
	}

	if (last_pos != old_length) {
		memcpy(new_ptrw, old_ptr + last_pos, (old_length - last_pos) * sizeof(char32_t));
		new_ptrw += old_length - last_pos;
	}

	*new_ptrw = 0;

	return new_string;
}

String String::replace(const String& p_key, const String& p_with) const
{
	return _replace_common(*this, p_key, p_with, false);
}

String String::replace(const char* p_key, const char* p_with) const
{
	return _replace_common(*this, p_key, p_with, false);
}

String String::replace_first(const String& p_key, const String& p_with) const
{
	int pos = find(p_key);
	if (pos >= 0) {
		const int old_length = length();
		const int key_length = p_key.length();
		const int with_length = p_with.length();

		String new_string;
		new_string.resize_uninitialized(old_length + (with_length - key_length) + 1);

		char32_t* new_ptrw = new_string.ptrw();
		const char32_t* old_ptr = ptr();
		const char32_t* with_ptr = p_with.ptr();

		if (pos > 0) {
			memcpy(new_ptrw, old_ptr, pos * sizeof(char32_t));
			new_ptrw += pos;
		}

		if (with_length) {
			memcpy(new_ptrw, with_ptr, with_length * sizeof(char32_t));
			new_ptrw += with_length;
		}
		pos += key_length;

		if (pos != old_length) {
			memcpy(new_ptrw, old_ptr + pos, (old_length - pos) * sizeof(char32_t));
			new_ptrw += (old_length - pos);
		}

		*new_ptrw = 0;

		return new_string;
	}

	return *this;
}

String String::replace_first(const char* p_key, const char* p_with) const
{
	int pos = find(p_key);
	if (pos >= 0) {
		const int old_length = length();
		const int key_length = strlen(p_key);
		const int with_length = strlen(p_with);

		String new_string;
		new_string.resize_uninitialized(old_length + (with_length - key_length) + 1);

		char32_t* new_ptrw = new_string.ptrw();
		const char32_t* old_ptr = ptr();

		if (pos > 0) {
			memcpy(new_ptrw, old_ptr, pos * sizeof(char32_t));
			new_ptrw += pos;
		}

		for (int i = 0; i < with_length; ++i) {
			*new_ptrw++ = p_with[i];
		}
		pos += key_length;

		if (pos != old_length) {
			memcpy(new_ptrw, old_ptr + pos, (old_length - pos) * sizeof(char32_t));
			new_ptrw += (old_length - pos);
		}

		*new_ptrw = 0;

		return new_string;
	}

	return *this;
}

String String::replace_char(char32_t p_key, char32_t p_with) const
{
	ERR_FAIL_COND_V_MSG(p_with == 0, *this, "`with` must not be the NUL character.");

	if (p_key == 0) {
		return *this;
	}

	int len = length();
	if (len == 0) {
		return *this;
	}

	int index = 0;
	const char32_t* old_ptr = ptr();
	for (; index < len; ++index) {
		if (old_ptr[index] == p_key) {
			break;
		}
	}

	// If no occurrence of `key` was found, return this.
	if (index == len) {
		return *this;
	}

	// If we found at least one occurrence of `key`, create new string.
	String new_string;
	new_string.resize_uninitialized(len + 1);
	char32_t* new_ptr = new_string.ptrw();

	// Copy part of input before `key`.
	memcpy(new_ptr, old_ptr, index * sizeof(char32_t));

	new_ptr[index] = p_with;

	// Copy or replace rest of input.
	for (++index; index < len; ++index) {
		if (old_ptr[index] == p_key) {
			new_ptr[index] = p_with;
		}
		else {
			new_ptr[index] = old_ptr[index];
		}
	}

	new_ptr[index] = _null;

	return new_string;
}

template <class T>
static String _replace_chars_common(
	const String& p_this, const T* p_keys, int p_keys_len, char32_t p_with)
{
	ERR_FAIL_COND_V_MSG(p_with == 0, p_this, "`with` must not be the NUL character.");

	// Delegate if p_keys is a single element.
	if (p_keys_len == 1) {
		return p_this.replace_char(*p_keys, p_with);
	}
	else if (p_keys_len == 0) {
		return p_this;
	}

	int len = p_this.length();
	if (len == 0) {
		return p_this;
	}

	int index = 0;
	const char32_t* old_ptr = p_this.ptr();
	for (; index < len; ++index) {
		if (_contains_char(old_ptr[index], p_keys, p_keys_len)) {
			break;
		}
	}

	// If no occurrence of `keys` was found, return this.
	if (index == len) {
		return p_this;
	}

	// If we found at least one occurrence of `keys`, create new string.
	String new_string;
	new_string.resize_uninitialized(len + 1);
	char32_t* new_ptr = new_string.ptrw();

	// Copy part of input before `key`.
	memcpy(new_ptr, old_ptr, index * sizeof(char32_t));

	new_ptr[index] = p_with;

	// Copy or replace rest of input.
	for (++index; index < len; ++index) {
		const char32_t old_char = old_ptr[index];
		if (_contains_char(old_char, p_keys, p_keys_len)) {
			new_ptr[index] = p_with;
		}
		else {
			new_ptr[index] = old_char;
		}
	}

	new_ptr[index] = 0;

	return new_string;
}

String String::replace_chars(const String& p_keys, char32_t p_with) const
{
	return _replace_chars_common(*this, p_keys.ptr(), p_keys.length(), p_with);
}

String String::replace_chars(const char* p_keys, char32_t p_with) const
{
	return _replace_chars_common(*this, p_keys, strlen(p_keys), p_with);
}

String String::replacen(const String& p_key, const String& p_with) const
{
	return _replace_common(*this, p_key, p_with, true);
}

String String::replacen(const char* p_key, const char* p_with) const
{
	return _replace_common(*this, p_key, p_with, true);
}

String String::repeat(int p_count) const
{
	ERR_FAIL_COND_V_MSG(p_count < 0, "", "Parameter count should be a positive number.");

	if (p_count == 0) {
		return "";
	}

	if (p_count == 1) {
		return *this;
	}

	int len = length();
	String new_string = *this;
	new_string.resize_uninitialized(p_count * len + 1);

	char32_t* dst = new_string.ptrw();
	int offset = 1;
	int stride = 1;
	while (offset < p_count) {
		memcpy(dst + offset * len, dst, stride * len * sizeof(char32_t));
		offset += stride;
		stride = MIN(stride * 2, p_count - offset);
	}
	dst[p_count * len] = _null;
	return new_string;
}

String String::reverse() const
{
	int len = length();
	if (len <= 1) {
		return *this;
	}
	String new_string;
	new_string.resize_uninitialized(len + 1);

	const char32_t* src = ptr();
	char32_t* dst = new_string.ptrw();
	for (int i = 0; i < len; i++) {
		dst[i] = src[len - i - 1];
	}
	dst[len] = _null;
	return new_string;
}

String String::left(int p_len) const
{
	if (p_len < 0) {
		p_len = length() + p_len;
	}

	if (p_len <= 0) {
		return "";
	}

	if (p_len >= length()) {
		return *this;
	}

	String s;
	s.append_utf32_unchecked(Span(ptr(), p_len));
	return s;
}

String String::right(int p_len) const
{
	if (p_len < 0) {
		p_len = length() + p_len;
	}

	if (p_len <= 0) {
		return "";
	}

	if (p_len >= length()) {
		return *this;
	}

	String s;
	s.append_utf32_unchecked(Span(ptr() + length() - p_len, p_len));
	return s;
}

char32_t String::unicode_at(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, length(), 0);
	return operator[](p_idx);
}

String String::indent(const String& p_prefix) const
{
	String new_string;
	int line_start = 0;

	for (int i = 0; i < length(); i++) {
		const char32_t c = operator[](i);
		if (c == '\n') {
			if (i == line_start) {
				new_string += c; // Leave empty lines empty.
			}
			else {
				new_string += p_prefix + substr(line_start, i - line_start + 1);
			}
			line_start = i + 1;
		}
	}
	if (line_start != length()) {
		new_string += p_prefix + substr(line_start);
	}
	return new_string;
}

String String::dedent() const
{
	String new_string;
	String indent;
	bool has_indent = false;
	bool has_text = false;
	int line_start = 0;
	int indent_stop = -1;

	for (int i = 0; i < length(); i++) {
		char32_t c = operator[](i);
		if (c == '\n') {
			if (has_text) {
				new_string += substr(indent_stop, i - indent_stop);
			}
			new_string += "\n";
			has_text = false;
			line_start = i + 1;
			indent_stop = -1;
		}
		else if (!has_text) {
			if (c > 32) {
				has_text = true;
				if (!has_indent) {
					has_indent = true;
					indent = substr(line_start, i - line_start);
					indent_stop = i;
				}
			}
			if (has_indent && indent_stop < 0) {
				int j = i - line_start;
				if (j >= indent.length() || c != indent[j]) {
					indent_stop = i;
				}
			}
		}
	}

	if (has_text) {
		new_string += substr(indent_stop, length() - indent_stop);
	}

	return new_string;
}

String String::strip_edges(bool p_left, bool p_right) const
{
	int len = length();
	int beg = 0, end = len;

	if (p_left) {
		for (int i = 0; i < len; i++) {
			if (operator[](i) <= 32) {
				beg++;
			}
			else {
				break;
			}
		}
	}

	if (p_right) {
		for (int i = len - 1; i >= 0; i--) {
			if (operator[](i) <= 32) {
				end--;
			}
			else {
				break;
			}
		}
	}

	if (beg == 0 && end == len) {
		return *this;
	}

	return substr(beg, end - beg);
}

String String::strip_escapes() const
{
	String new_string;
	for (int i = 0; i < length(); i++) {
		// Escape characters on first page of the ASCII table, before 32 (Space).
		if (operator[](i) < 32) {
			continue;
		}
		new_string += operator[](i);
	}

	return new_string;
}

String String::lstrip(const String& p_chars) const
{
	int len = length();
	int beg;

	for (beg = 0; beg < len; beg++) {
		if (p_chars.find_char(get(beg)) == -1) {
			break;
		}
	}

	if (beg == 0) {
		return *this;
	}

	return substr(beg, len - beg);
}

String String::rstrip(const String& p_chars) const
{
	int len = length();
	int end;

	for (end = len - 1; end >= 0; end--) {
		if (p_chars.find_char(get(end)) == -1) {
			break;
		}
	}

	if (end == len - 1) {
		return *this;
	}

	return substr(0, end + 1);
}

bool String::is_network_share_path() const { return begins_with("//") || begins_with("\\\\"); }

String String::simplify_path() const
{
	String s = *this;
	String drive;

	// Check if we have a special path (like res://) or a protocol identifier.
	int p = s.find("://");
	bool found = false;
	if (p > 0) {
		bool only_chars = true;
		for (int i = 0; i < p; i++) {
			if (!is_ascii_alphanumeric_char(s[i])) {
				only_chars = false;
				break;
			}
		}
		if (only_chars) {
			found = true;
			drive = s.substr(0, p + 3);
			s = s.substr(p + 3);
		}
	}
	if (!found) {
		if (is_network_share_path()) {
			// Network path, beginning with // or \\.
			drive = s.substr(0, 2);
			s = s.substr(2);
		}
		else if (s.begins_with("/") || s.begins_with("\\")) {
			// Absolute path.
			drive = s.substr(0, 1);
			s = s.substr(1);
		}
		else {
			// Windows-style drive path, like C:/ or C:\.
			p = s.find(":/");
			if (p == -1) {
				p = s.find(":\\");
			}
			if (p != -1 && p < s.find_char('/')) {
				drive = s.substr(0, p + 2);
				s = s.substr(p + 2);
			}
		}
	}

	s = s.replace_char('\\', '/');
	while (true) { // in case of using 2 or more slash
		String compare = s.replace("//", "/");
		if (s == compare) {
			break;
		}
		else {
			s = compare;
		}
	}
	Vector<String> dirs = s.split("/", false);
	bool absolute_path = is_absolute_path();

	absolute_path =
		absolute_path &&
		!begins_with("res://"); // FIXME: Some code (GLTF importer) rely on accessing files up from
								// `res://`, this probably should be disabled in the future.

	for (int i = 0; i < dirs.size(); i++) {
		String d = dirs[i];
		if (d == ".") {
			dirs.remove_at(i);
			i--;
		}
		else if (d == "..") {
			if (i != 0 && dirs[i - 1] != "..") {
				dirs.remove_at(i);
				dirs.remove_at(i - 1);
				i -= 2;
			}
			else if (absolute_path && i == 0) {
				dirs.remove_at(i);
				i--;
			}
		}
	}

	s = "";

	for (int i = 0; i < dirs.size(); i++) {
		if (i > 0) {
			s += "/";
		}
		s += dirs[i];
	}

	return drive + s;
}

static int _humanize_digits(int p_num)
{
	if (p_num < 100) {
		return 2;
	}
	else if (p_num < 1024) {
		return 1;
	}
	else {
		return 0;
	}
}

String String::humanize_size(uint64_t p_size)
{
	int magnitude = 0;
	uint64_t _div = 1;
	while (p_size > _div * 1024 && magnitude < 6) {
		_div *= 1024;
		magnitude++;
	}

	if (magnitude == 0) {
		return String::num_uint64(p_size) + " " + RTR("B");
	}
	else {
		String suffix;
		switch (magnitude) {
		case 1:
			suffix = RTR("KiB");
			break;
		case 2:
			suffix = RTR("MiB");
			break;
		case 3:
			suffix = RTR("GiB");
			break;
		case 4:
			suffix = RTR("TiB");
			break;
		case 5:
			suffix = RTR("PiB");
			break;
		case 6:
			suffix = RTR("EiB");
			break;
		}

		const double divisor = _div;
		const int digits = _humanize_digits(p_size / _div);
		return String::num(p_size / divisor).pad_decimals(digits) + " " + suffix;
	}
}

bool String::is_absolute_path() const
{
	if (length() > 1) {
		return (operator[](0) == '/' || operator[](0) == '\\' || find(":/") != -1 ||
				find(":\\") != -1);
	}
	else if ((length()) == 1) {
		return (operator[](0) == '/' || operator[](0) == '\\');
	}
	else {
		return false;
	}
}

String String::validate_ascii_identifier() const
{
	if (is_empty()) {
		return "_"; // Empty string is not a valid identifier.
	}

	String result;
	if (is_digit(operator[](0))) {
		result = "_" + *this;
	}
	else {
		result = *this;
	}

	int len = result.length();
	char32_t* buffer = result.ptrw();
	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(buffer[i])) {
			buffer[i] = '_';
		}
	}

	return result;
}

String String::validate_unicode_identifier() const
{
	if (is_empty()) {
		return "_"; // Empty string is not a valid identifier.
	}

	String result;
	if (is_unicode_identifier_start(operator[](0))) {
		result = *this;
	}
	else {
		result = "_" + *this;
	}

	int len = result.length();
	char32_t* buffer = result.ptrw();
	for (int i = 0; i < len; i++) {
		if (!is_unicode_identifier_continue(buffer[i])) {
			buffer[i] = '_';
		}
	}

	return result;
}

bool String::is_valid_ascii_identifier() const
{
	int len = length();

	if (len == 0) {
		return false;
	}

	if (is_digit(operator[](0))) {
		return false;
	}

	const char32_t* str = &operator[](0);

	for (int i = 0; i < len; i++) {
		if (!is_ascii_identifier_char(str[i])) {
			return false;
		}
	}

	return true;
}

bool String::is_valid_unicode_identifier() const
{
	const char32_t* str = ptr();
	int len = length();

	if (len == 0) {
		return false; // Empty string.
	}

	if (!is_unicode_identifier_start(str[0])) {
		return false;
	}

	for (int i = 1; i < len; i++) {
		if (!is_unicode_identifier_continue(str[i])) {
			return false;
		}
	}
	return true;
}

bool String::is_valid_string() const
{
	int l = length();
	const char32_t* src = get_data();
	bool valid = true;
	for (int i = 0; i < l; i++) {
		valid = valid && (src[i] < 0xd800 || (src[i] > 0xdfff && src[i] <= 0x10ffff));
	}
	return valid;
}

String String::uri_encode() const
{
	const CharString temp = utf8();
	String res;

	for (int i = 0; i < temp.length(); ++i) {
		uint8_t ord = uint8_t(temp[i]);
		if (ord == '.' || ord == '-' || ord == '~' || is_ascii_identifier_char(ord)) {
			res += ord;
		}
		else {
			char p[4] = {'%', 0, 0, 0};
			p[1] = hex_char_table_upper[ord >> 4];
			p[2] = hex_char_table_upper[ord & 0xF];
			res += p;
		}
	}
	return res;
}

String String::uri_decode() const
{
	CharString src = utf8();
	CharString res;
	for (int i = 0; i < src.length(); ++i) {
		if (src[i] == '%' && i + 2 < src.length()) {
			char ord1 = src[i + 1];
			if (is_digit(ord1) || is_ascii_upper_case(ord1)) {
				char ord2 = src[i + 2];
				if (is_digit(ord2) || is_ascii_upper_case(ord2)) {
					char bytes[3] = {(char)ord1, (char)ord2, 0};
					res += (char)strtol(bytes, nullptr, 16);
					i += 2;
				}
			}
			else {
				res += src[i];
			}
		}
		else if (src[i] == '+') {
			res += ' ';
		}
		else {
			res += src[i];
		}
	}
	return String::utf8(res);
}

String String::uri_file_decode() const
{
	CharString src = utf8();
	CharString res;
	for (int i = 0; i < src.length(); ++i) {
		if (src[i] == '%' && i + 2 < src.length()) {
			char ord1 = src[i + 1];
			if (is_digit(ord1) || is_ascii_upper_case(ord1)) {
				char ord2 = src[i + 2];
				if (is_digit(ord2) || is_ascii_upper_case(ord2)) {
					char bytes[3] = {(char)ord1, (char)ord2, 0};
					res += (char)strtol(bytes, nullptr, 16);
					i += 2;
				}
			}
			else {
				res += src[i];
			}
		}
		else {
			res += src[i];
		}
	}
	return String::utf8(res);
}

String String::c_unescape() const
{
	String escaped = *this;
	escaped = escaped.replace("\\a", "\a");
	escaped = escaped.replace("\\b", "\b");
	escaped = escaped.replace("\\f", "\f");
	escaped = escaped.replace("\\n", "\n");
	escaped = escaped.replace("\\r", "\r");
	escaped = escaped.replace("\\t", "\t");
	escaped = escaped.replace("\\v", "\v");
	escaped = escaped.replace("\\'", "\'");
	escaped = escaped.replace("\\\"", "\"");
	escaped = escaped.replace("\\\\", "\\");

	return escaped;
}

String String::c_escape() const
{
	String escaped = *this;
	escaped = escaped.replace("\\", "\\\\");
	escaped = escaped.replace("\a", "\\a");
	escaped = escaped.replace("\b", "\\b");
	escaped = escaped.replace("\f", "\\f");
	escaped = escaped.replace("\n", "\\n");
	escaped = escaped.replace("\r", "\\r");
	escaped = escaped.replace("\t", "\\t");
	escaped = escaped.replace("\v", "\\v");
	escaped = escaped.replace("\'", "\\'");
	escaped = escaped.replace("\"", "\\\"");

	return escaped;
}

String String::c_escape_multiline() const
{
	String escaped = *this;
	escaped = escaped.replace("\\", "\\\\");
	escaped = escaped.replace("\"", "\\\"");

	return escaped;
}

String String::json_escape() const
{
	String escaped = *this;
	escaped = escaped.replace("\\", "\\\\");
	escaped = escaped.replace("\b", "\\b");
	escaped = escaped.replace("\f", "\\f");
	escaped = escaped.replace("\n", "\\n");
	escaped = escaped.replace("\r", "\\r");
	escaped = escaped.replace("\t", "\\t");
	escaped = escaped.replace("\v", "\\v");
	escaped = escaped.replace("\"", "\\\"");

	return escaped;
}

String String::xml_escape(bool p_escape_quotes) const
{
	String str = *this;
	str = str.replace("&", "&amp;");
	str = str.replace("<", "&lt;");
	str = str.replace(">", "&gt;");
	if (p_escape_quotes) {
		str = str.replace("'", "&apos;");
		str = str.replace("\"", "&quot;");
	}
	/*
for (int i=1;i<32;i++) {
	char chr[2]={i,0};
	str=str.replace(chr,"&#"+String::num(i)+";");
}*/
	return str;
}

static _FORCE_INLINE_ int _xml_unescape(const char32_t* p_src, int p_src_len, char32_t* p_dst)
{
	int len = 0;
	while (p_src_len) {
		if (*p_src == '&') {
			int eat = 0;

			if (p_src_len >= 4 && p_src[1] == '#') {
				char32_t c = 0;
				bool overflow = false;
				if (p_src[2] == 'x') {
					// Hex entity &#x<num>;
					for (int i = 3; i < p_src_len; i++) {
						eat = i + 1;
						char32_t ct = p_src[i];
						if (ct == ';') {
							break;
						}
						else if (is_digit(ct)) {
							ct = ct - '0';
						}
						else if (ct >= 'a' && ct <= 'f') {
							ct = (ct - 'a') + 10;
						}
						else if (ct >= 'A' && ct <= 'F') {
							ct = (ct - 'A') + 10;
						}
						else {
							break;
						}
						if (c > (UINT32_MAX >> 4)) {
							overflow = true;
							break;
						}
						c <<= 4;
						c |= ct;
					}
				}
				else {
					// Decimal entity &#<num>;
					for (int i = 2; i < p_src_len; i++) {
						eat = i + 1;
						char32_t ct = p_src[i];
						if (ct == ';' || !is_digit(ct)) {
							break;
						}
					}
					if (p_src[eat - 1] == ';') {
						int64_t val = String::to_int(p_src + 2, eat - 3);
						if (val > 0 && val <= UINT32_MAX) {
							c = (char32_t)val;
						}
						else {
							overflow = true;
						}
					}
				}

				// Value must be non-zero, in the range of char32_t,
				// actually end with ';'. If invalid, leave the entity as-is
				if (c == '\0' || overflow || p_src[eat - 1] != ';') {
					eat = 1;
					c = *p_src;
				}
				if (p_dst) {
					*p_dst = c;
				}

			}
			else if (p_src_len >= 4 && p_src[1] == 'g' && p_src[2] == 't' && p_src[3] == ';') {
				if (p_dst) {
					*p_dst = '>';
				}
				eat = 4;
			}
			else if (p_src_len >= 4 && p_src[1] == 'l' && p_src[2] == 't' && p_src[3] == ';') {
				if (p_dst) {
					*p_dst = '<';
				}
				eat = 4;
			}
			else if (p_src_len >= 5 && p_src[1] == 'a' && p_src[2] == 'm' && p_src[3] == 'p' &&
					   p_src[4] == ';') {
				if (p_dst) {
					*p_dst = '&';
				}
				eat = 5;
			}
			else if (p_src_len >= 6 && p_src[1] == 'q' && p_src[2] == 'u' && p_src[3] == 'o' &&
					   p_src[4] == 't' && p_src[5] == ';') {
				if (p_dst) {
					*p_dst = '"';
				}
				eat = 6;
			}
			else if (p_src_len >= 6 && p_src[1] == 'a' && p_src[2] == 'p' && p_src[3] == 'o' &&
					   p_src[4] == 's' && p_src[5] == ';') {
				if (p_dst) {
					*p_dst = '\'';
				}
				eat = 6;
			}
			else {
				if (p_dst) {
					*p_dst = *p_src;
				}
				eat = 1;
			}

			if (p_dst) {
				p_dst++;
			}

			len++;
			p_src += eat;
			p_src_len -= eat;
		}
		else {
			if (p_dst) {
				*p_dst = *p_src;
				p_dst++;
			}
			len++;
			p_src++;
			p_src_len--;
		}
	}

	return len;
}

String String::xml_unescape() const
{
	String str;
	int l = length();
	int len = _xml_unescape(get_data(), l, nullptr);
	if (len == 0) {
		return String();
	}
	str.resize_uninitialized(len + 1);
	char32_t* str_ptrw = str.ptrw();
	_xml_unescape(get_data(), l, str_ptrw);
	str_ptrw[len] = 0;
	return str;
}

String String::pad_decimals(int p_digits) const
{
	String s = *this;
	int c = s.find_char('.');

	if (c == -1) {
		if (p_digits <= 0) {
			return s;
		}
		s += ".";
		c = s.length() - 1;
	}
	else {
		if (p_digits <= 0) {
			return s.substr(0, c);
		}
	}

	if (s.length() - (c + 1) > p_digits) {
		return s.substr(0, c + p_digits + 1);
	}
	else {
		int zeros_to_add = p_digits - s.length() + (c + 1);
		return s + String("0").repeat(zeros_to_add);
	}
}

String String::pad_zeros(int p_digits) const
{
	String s = *this;
	int end = s.find_char('.');

	if (end == -1) {
		end = s.length();
	}

	if (end == 0) {
		return s;
	}

	int begin = 0;

	while (begin < end && !is_digit(s[begin])) {
		begin++;
	}

	int zeros_to_add = p_digits - (end - begin);

	if (zeros_to_add <= 0) {
		return s;
	}
	else {
		return s.insert(begin, String("0").repeat(zeros_to_add));
	}
}

String String::trim_prefix(const String& p_prefix) const
{
	String s = *this;
	if (s.begins_with(p_prefix)) {
		return s.substr(p_prefix.length());
	}
	return s;
}

String String::trim_prefix(const char* p_prefix) const
{
	String s = *this;
	if (s.begins_with(p_prefix)) {
		int prefix_length = strlen(p_prefix);
		return s.substr(prefix_length);
	}
	return s;
}

String String::trim_suffix(const String& p_suffix) const
{
	String s = *this;
	if (s.ends_with(p_suffix)) {
		return s.substr(0, s.length() - p_suffix.length());
	}
	return s;
}

String String::trim_suffix(const char* p_suffix) const
{
	String s = *this;
	if (s.ends_with(p_suffix)) {
		return s.substr(0, s.length() - strlen(p_suffix));
	}
	return s;
}

bool String::is_valid_int() const
{
	int len = length();

	if (len == 0) {
		return false;
	}

	int from = 0;
	if (len != 1 && (operator[](0) == '+' || operator[](0) == '-')) {
		from++;
	}

	for (int i = from; i < len; i++) {
		if (!is_digit(operator[](i))) {
			return false; // no start with number plz
		}
	}

	return true;
}

bool String::is_valid_hex_number(bool p_with_prefix) const
{
	int len = length();

	if (len == 0) {
		return false;
	}

	int from = 0;
	if (len != 1 && (operator[](0) == '+' || operator[](0) == '-')) {
		from++;
	}

	if (p_with_prefix) {
		if (len < 3) {
			return false;
		}
		if (operator[](from) != '0' || operator[](from + 1) != 'x') {
			return false;
		}
		from += 2;
	}

	if (from == len) {
		return false;
	}

	for (int i = from; i < len; i++) {
		char32_t c = operator[](i);
		if (is_hex_digit(c)) {
			continue;
		}
		return false;
	}

	return true;
}

bool String::is_valid_float() const
{
	int len = length();

	if (len == 0) {
		return false;
	}

	int from = 0;
	if (operator[](0) == '+' || operator[](0) == '-') {
		from++;
	}

	bool exponent_found = false;
	bool period_found = false;
	bool sign_found = false;
	bool exponent_values_found = false;
	bool numbers_found = false;

	for (int i = from; i < len; i++) {
		const char32_t c = operator[](i);
		if (is_digit(c)) {
			if (exponent_found) {
				exponent_values_found = true;
			}
			else {
				numbers_found = true;
			}
		}
		else if (numbers_found && !exponent_found && (c == 'e' || c == 'E')) {
			exponent_found = true;
		}
		else if (!period_found && !exponent_found && c == '.') {
			period_found = true;
		}
		else if ((c == '-' || c == '+') && exponent_found && !exponent_values_found &&
				   !sign_found) {
			sign_found = true;
		}
		else {
			return false; // no start with number plz
		}
	}

	return numbers_found;
}

String String::path_to_file(const String& p_path) const
{
	// Don't get base dir for src, this is expected to be a dir already.
	String src = replace_char('\\', '/');
	String dst = p_path.replace_char('\\', '/').get_base_dir();
	String rel = src.path_to(dst);
	if (rel == dst) { // failed
		return p_path;
	}
	else {
		return rel + p_path.get_file();
	}
}

String String::path_to(const String& p_path) const
{
	String src = replace_char('\\', '/');
	String dst = p_path.replace_char('\\', '/');
	if (!src.ends_with("/")) {
		src += "/";
	}
	if (!dst.ends_with("/")) {
		dst += "/";
	}

	if (src.begins_with("res://") && dst.begins_with("res://")) {
		src = src.replace("res://", "/");
		dst = dst.replace("res://", "/");

	}
	else if (src.begins_with("user://") && dst.begins_with("user://")) {
		src = src.replace("user://", "/");
		dst = dst.replace("user://", "/");

	}
	else if (src.begins_with("/") && dst.begins_with("/")) {
		// nothing
	}
	else {
		// dos style
		String src_begin = src.get_slicec('/', 0);
		String dst_begin = dst.get_slicec('/', 0);

		if (src_begin != dst_begin) {
			return p_path; // impossible to do this
		}

		src = src.substr(src_begin.length());
		dst = dst.substr(dst_begin.length());
	}

	// remove leading and trailing slash and split
	Vector<String> src_dirs = src.substr(1, src.length() - 2).split("/");
	Vector<String> dst_dirs = dst.substr(1, dst.length() - 2).split("/");

	// find common parent
	int common_parent = 0;

	while (true) {
		if (src_dirs.size() == common_parent) {
			break;
		}
		if (dst_dirs.size() == common_parent) {
			break;
		}
		if (src_dirs[common_parent] != dst_dirs[common_parent]) {
			break;
		}
		common_parent++;
	}

	common_parent--;

	int dirs_to_backtrack = (src_dirs.size() - 1) - common_parent;
	String dir = String("../").repeat(dirs_to_backtrack);

	for (int i = common_parent + 1; i < dst_dirs.size(); i++) {
		dir += dst_dirs[i] + "/";
	}

	if (dir.length() == 0) {
		dir = "./";
	}
	return dir;
}

bool String::is_valid_html_color() const { return Color::html_is_valid(*this); }

// Changes made to the set of invalid filename characters must also be reflected in the String
// documentation for is_valid_filename.
static const char* invalid_filename_characters[] = {
	":", "/", "\\", "?", "*", "\"", "|", "%", "<", ">"};

bool String::is_valid_filename() const
{
	String stripped = strip_edges();
	if (*this != stripped) {
		return false;
	}

	if (stripped.is_empty()) {
		return false;
	}

	for (const char* ch : invalid_filename_characters) {
		if (contains(ch)) {
			return false;
		}
	}
	return true;
}

String String::validate_filename() const
{
	String name = strip_edges();
	for (const char* ch : invalid_filename_characters) {
		name = name.replace(ch, "_");
	}
	return name;
}

bool String::is_valid_ip_address() const { return IPAddress::is_valid_ip_address(*this); }

bool String::is_resource_file() const { return begins_with("res://") && find("::") == -1; }

bool String::is_relative_path() const { return !is_absolute_path(); }

String String::get_base_dir() const
{
	int end = 0;

	// URL scheme style base.
	int basepos = find("://");
	if (basepos != -1) {
		end = basepos + 3;
	}

	// Windows top level directory base.
	if (end == 0) {
		basepos = find(":/");
		if (basepos == -1) {
			basepos = find(":\\");
		}
		if (basepos != -1) {
			end = basepos + 2;
		}
	}

	// Windows UNC network share path.
	if (end == 0) {
		if (is_network_share_path()) {
			basepos = find_char('/', 2);
			if (basepos == -1) {
				basepos = find_char('\\', 2);
			}
			int servpos = find_char('/', basepos + 1);
			if (servpos == -1) {
				servpos = find_char('\\', basepos + 1);
			}
			if (servpos != -1) {
				end = servpos + 1;
			}
		}
	}

	// Unix root directory base.
	if (end == 0) {
		if (begins_with("/")) {
			end = 1;
		}
	}

	String rs;
	String base;
	if (end != 0) {
		rs = substr(end, length());
		base = substr(0, end);
	}
	else {
		rs = *this;
	}

	int sep = MAX(rs.rfind_char('/'), rs.rfind_char('\\'));
	if (sep == -1) {
		return base;
	}

	return base + rs.substr(0, sep);
}

String String::get_file() const
{
	int sep = MAX(rfind_char('/'), rfind_char('\\'));
	if (sep == -1) {
		return *this;
	}

	return substr(sep + 1, length());
}

String String::get_extension() const
{
	int pos = rfind_char('.');
	if (pos < 0 || pos < MAX(rfind_char('/'), rfind_char('\\'))) {
		return "";
	}

	return substr(pos + 1, length());
}

String String::path_join(const String& p_file) const
{
	if (is_empty()) {
		return p_file;
	}
	if (operator[](length() - 1) == '/' || (p_file.size() > 0 && p_file.operator[](0) == '/')) {
		return *this + p_file;
	}
	return *this + "/" + p_file;
}

String String::property_name_encode() const
{
	// Escape and quote strings with extended ASCII or further Unicode characters
	// as well as '"', '=' or ' ' (32)
	const char32_t* cstr = get_data();
	for (int i = 0; cstr[i]; i++) {
		if (cstr[i] == '=' || cstr[i] == '"' || cstr[i] == ';' || cstr[i] == '[' ||
			cstr[i] == ']' || cstr[i] < 33 || cstr[i] > 126) {
			return "\"" + c_escape_multiline() + "\"";
		}
	}
	// Keep as is
	return *this;
}

// Changes made to the set of invalid characters must also be reflected in the String documentation.

static const char32_t invalid_node_name_characters[] = {
	'.', ':', '@', '/', '\"', UNIQUE_NODE_PREFIX[0], 0};

String String::get_invalid_node_name_characters(bool p_allow_internal)
{
	// Do not use this function for critical validation.
	String r;
	const char32_t* c = invalid_node_name_characters;
	while (*c) {
		if (p_allow_internal && *c == '@') {
			c++;
			continue;
		}

		if (c != invalid_node_name_characters) {
			r += " ";
		}
		r += String::chr(*c);
		c++;
	}
	return r;
}

String String::validate_node_name() const
{
	// This is a critical validation in node addition, so it must be optimized.
	const char32_t* cn = ptr();
	if (cn == nullptr) {
		return String();
	}
	bool valid = true;
	uint32_t idx = 0;
	while (cn[idx]) {
		const char32_t* c = invalid_node_name_characters;
		while (*c) {
			if (cn[idx] == *c) {
				valid = false;
				break;
			}
			c++;
		}
		if (!valid) {
			break;
		}
		idx++;
	}

	if (valid) {
		return *this;
	}

	String validated = *this;
	char32_t* nn = validated.ptrw();
	while (nn[idx]) {
		const char32_t* c = invalid_node_name_characters;
		while (*c) {
			if (nn[idx] == *c) {
				nn[idx] = '_';
				break;
			}
			c++;
		}
		idx++;
	}

	return validated;
}

String String::get_basename() const
{
	int pos = rfind_char('.');
	if (pos < 0 || pos < MAX(rfind_char('/'), rfind_char('\\'))) {
		return *this;
	}

	return substr(0, pos);
}

String itos(int64_t p_val) { return String::num_int64(p_val); }

String uitos(uint64_t p_val) { return String::num_uint64(p_val); }

String rtos(double p_val) { return String::num(p_val); }

String rtoss(double p_val) { return String::num_scientific(p_val); }

// Right-pad with a character.
String String::rpad(int p_min_length, const String& p_character) const
{
	String s = *this;
	int padding = p_min_length - s.length();
	if (padding > 0) {
		s += p_character.repeat(padding);
	}
	return s;
}

// Left-pad with a character.
String String::lpad(int p_min_length, const String& p_character) const
{
	String s = *this;
	int padding = p_min_length - s.length();
	if (padding > 0) {
		s = p_character.repeat(padding) + s;
	}
	return s;
}

String String::quote(const String& p_quotechar) const { return p_quotechar + *this + p_quotechar; }

String String::unquote() const
{
	if (!is_quoted()) {
		return *this;
	}

	return substr(1, length() - 2);
}

Vector<uint8_t> String::to_ascii_buffer() const
{
	const String* s = this;
	if (s->is_empty()) {
		return Vector<uint8_t>();
	}
	CharString charstr = s->ascii();

	Vector<uint8_t> retval;
	size_t len = charstr.length();
	retval.resize_uninitialized(len);
	uint8_t* w = retval.ptrw();
	memcpy(w, charstr.ptr(), len);

	return retval;
}

Vector<uint8_t> String::to_utf8_buffer() const
{
	const String* s = this;
	if (s->is_empty()) {
		return Vector<uint8_t>();
	}
	CharString charstr = s->utf8();

	Vector<uint8_t> retval;
	size_t len = charstr.length();
	retval.resize_uninitialized(len);
	uint8_t* w = retval.ptrw();
	memcpy(w, charstr.ptr(), len);

	return retval;
}

Vector<uint8_t> String::to_utf16_buffer() const
{
	const String* s = this;
	if (s->is_empty()) {
		return Vector<uint8_t>();
	}
	Char16String charstr = s->utf16();

	Vector<uint8_t> retval;
	size_t len = charstr.length() * sizeof(char16_t);
	retval.resize_uninitialized(len);
	uint8_t* w = retval.ptrw();
	memcpy(w, (const void*)charstr.ptr(), len);

	return retval;
}

Vector<uint8_t> String::to_utf32_buffer() const
{
	const String* s = this;
	if (s->is_empty()) {
		return Vector<uint8_t>();
	}

	Vector<uint8_t> retval;
	size_t len = s->length() * sizeof(char32_t);
	retval.resize_uninitialized(len);
	uint8_t* w = retval.ptrw();
	memcpy(w, (const void*)s->ptr(), len);

	return retval;
}

Vector<uint8_t> String::to_wchar_buffer() const
{
#ifdef WINDOWS_ENABLED
	return to_utf16_buffer();
#else
	return to_utf32_buffer();
#endif
}

Vector<uint8_t> String::to_multibyte_char_buffer(const String& p_encoding) const
{
	return OS::string_to_multibyte(p_encoding, *this);
}

#ifdef TOOLS_ENABLED
/**
 * "Tools TRanslate". Performs string replacement for internationalization
 * within the editor. A translation context can optionally be specified to
 * disambiguate between identical source strings in translations. When
 * placeholders are desired, use `vformat(TTR("Example: %s"), some_string)`.
 * If a string mentions a quantity (and may therefore need a dynamic plural form),
 * use `TTRN()` instead of `TTR()`.
 *
 * NOTE: Only use `TTR()` in editor-only code (typically within the `editor/` folder).
 * For translations that can be supplied by exported projects, use `RTR()` instead.
 */
String TTR(const String& p_text, const String& p_context)
{
	if (TranslationServer::is_initialized()) {
		return TranslationServer::get_editor_domain()->translate(
			p_text, p_context);
	}

	return p_text;
}

/**
 * "Tools TRanslate for N items". Performs string replacement for
 * internationalization within the editor. A translation context can optionally
 * be specified to disambiguate between identical source strings in
 * translations. Use `TTR()` if the string doesn't need dynamic plural form.
 * When placeholders are desired, use
 * `vformat(TTRN("%d item", "%d items", some_integer), some_integer)`.
 * The placeholder must be present in both strings to avoid run-time warnings in `vformat()`.
 *
 * NOTE: Only use `TTRN()` in editor-only code (typically within the `editor/` folder).
 * For translations that can be supplied by exported projects, use `RTRN()` instead.
 */
String TTRN(const String& p_text, const String& p_text_plural, int p_n, const String& p_context)
{
	if (TranslationServer::is_initialized()) {
		return TranslationServer::get_editor_domain()->translate_plural(
			p_text, p_text_plural, p_n, p_context);
	}

	// Return message based on English plural rule if translation is not possible.
	if (p_n == 1) {
		return p_text;
	}
	return p_text_plural;
}

/**
 * "Docs TRanslate". Used for the editor class reference documentation,
 * handling descriptions extracted from the XML.
 * It also replaces `$DOCS_URL` with the actual URL to the documentation's branch,
 * to allow dehardcoding it in the XML and doing proper substitutions everywhere.
 */
String DTR(const String& p_text, const String& p_context)
{
	// Comes straight from the XML, so remove indentation and any trailing whitespace.
	const String text = p_text.dedent().strip_edges();

	if (TranslationServer::is_initialized()) {
		return String(
			TranslationServer::get_doc_domain()->translate(text, p_context))
			.replace("$DOCS_URL", VLTR_VERSION_DOCS_URL);
	}

	return text.replace("$DOCS_URL", VLTR_VERSION_DOCS_URL);
}

/**
 * "Docs TRanslate for N items". Used for the editor class reference documentation
 * (with support for plurals), handling descriptions extracted from the XML.
 * It also replaces `$DOCS_URL` with the actual URL to the documentation's branch,
 * to allow dehardcoding it in the XML and doing proper substitutions everywhere.
 */
String DTRN(const String& p_text, const String& p_text_plural, int p_n, const String& p_context)
{
	const String text = p_text.dedent().strip_edges();
	const String text_plural = p_text_plural.dedent().strip_edges();

	if (TranslationServer::is_initialized()) {
		return String(TranslationServer::get_doc_domain()->translate_plural(
						  text, text_plural, p_n, p_context))
			.replace("$DOCS_URL", VLTR_VERSION_DOCS_URL);
	}

	// Return message based on English plural rule if translation is not possible.
	if (p_n == 1) {
		return text.replace("$DOCS_URL", VLTR_VERSION_DOCS_URL);
	}
	return text_plural.replace("$DOCS_URL", VLTR_VERSION_DOCS_URL);
}
#endif

/**
 * "Run-time TRanslate". Performs string replacement f
or internationalization
 * without the editor. A translation context can optionally be specified to
 * disambiguate between identical source strings in translations. When
 * placeholders are desired, use `vformat(RTR("Example: %s"), some_string)`.
 * If a string mentions a quantity (and may therefore need a dynamic plural form),
 * use `RTRN()` instead of `RTR()`.
 *
 * NOTE: Do not use `RTR()` in editor-only code (typically within the `editor/`
 * folder). For editor translations, use `TTR()` instead.
 */
String RTR(const String& p_text, const String& p_context)
{
	if (TranslationServer::is_initialized()) {
#ifdef TOOLS_ENABLED
		String rtr =
			TranslationServer::get_editor_domain()->translate(p_text, p_context);
		if (!rtr.is_empty() && rtr != p_text) {
			return rtr;
		}
#endif // TOOLS_ENABLED
		return TranslationServer::translate(p_text, p_context);
	}

	return p_text;
}

/**
 * "Run-time TRanslate for N items". Performs string replacement for
 * internationalization without the editor. A translation context can optionally
 * be specified to disambiguate between identical source strings in translations.
 * Use `RTR()` if the string doesn't need dynamic plural form. When placeholders
 * are desired, use `vformat(RTRN("%d item", "%d items", some_integer), some_integer)`.
 * The placeholder must be present in both strings to avoid run-time warnings in `vformat()`.
 *
 * NOTE: Do not use `RTRN()` in editor-only code (typically within the `editor/`
 * folder). For editor translations, use `TTRN()` instead.
 */
String RTRN(const String& p_text, const String& p_text_plural, int p_n, const String& p_context)
{
	if (TranslationServer::is_initialized()) {
#ifdef TOOLS_ENABLED
		String rtr = TranslationServer::get_editor_domain()->translate_plural(
			p_text, p_text_plural, p_n, p_context);
		if (!rtr.is_empty() && rtr != p_text && rtr != p_text_plural) {
			return rtr;
		}
#endif // TOOLS_ENABLED
		return TranslationServer::translate_plural(
			p_text, p_text_plural, p_n, p_context);
	}

	// Return message based on English plural rule if translation is not possible.
	if (p_n == 1) {
		return p_text;
	}
	return p_text_plural;
}



// ============================================================================
// Merged from core/string/char_range.cpp
// ============================================================================

// This file was generated using the `misc/scripts/char_range_fetch.py` script.

#include "core/string/ustring.h"

// Unicode Derived Core Properties
// Source: https://www.unicode.org/Public/17.0.0/ucd/DerivedCoreProperties.txt

const int xid_start_size = 692;
const CharRange xid_start[xid_start_size] = {
	{ 0x41, 0x5a },
	{ 0x5f, 0x5f },
	{ 0x61, 0x7a },
	{ 0xaa, 0xaa },
	{ 0xb5, 0xb5 },
	{ 0xba, 0xba },
	{ 0xc0, 0xd6 },
	{ 0xd8, 0xf6 },
	{ 0xf8, 0x2c1 },
	{ 0x2c6, 0x2d1 },
	{ 0x2e0, 0x2e4 },
	{ 0x2ec, 0x2ec },
	{ 0x2ee, 0x2ee },
	{ 0x370, 0x374 },
	{ 0x376, 0x377 },
	{ 0x37b, 0x37d },
	{ 0x37f, 0x37f },
	{ 0x386, 0x386 },
	{ 0x388, 0x38a },
	{ 0x38c, 0x38c },
	{ 0x38e, 0x3a1 },
	{ 0x3a3, 0x3f5 },
	{ 0x3f7, 0x481 },
	{ 0x48a, 0x52f },
	{ 0x531, 0x556 },
	{ 0x559, 0x559 },
	{ 0x560, 0x588 },
	{ 0x5d0, 0x5ea },
	{ 0x5ef, 0x5f2 },
	{ 0x620, 0x64a },
	{ 0x66e, 0x66f },
	{ 0x671, 0x6d3 },
	{ 0x6d5, 0x6d5 },
	{ 0x6e5, 0x6e6 },
	{ 0x6ee, 0x6ef },
	{ 0x6fa, 0x6fc },
	{ 0x6ff, 0x6ff },
	{ 0x710, 0x710 },
	{ 0x712, 0x72f },
	{ 0x74d, 0x7a5 },
	{ 0x7b1, 0x7b1 },
	{ 0x7ca, 0x7ea },
	{ 0x7f4, 0x7f5 },
	{ 0x7fa, 0x7fa },
	{ 0x800, 0x815 },
	{ 0x81a, 0x81a },
	{ 0x824, 0x824 },
	{ 0x828, 0x828 },
	{ 0x840, 0x858 },
	{ 0x860, 0x86a },
	{ 0x870, 0x887 },
	{ 0x889, 0x88f },
	{ 0x8a0, 0x8c9 },
	{ 0x904, 0x939 },
	{ 0x93d, 0x93d },
	{ 0x950, 0x950 },
	{ 0x958, 0x961 },
	{ 0x971, 0x980 },
	{ 0x985, 0x98c },
	{ 0x98f, 0x990 },
	{ 0x993, 0x9a8 },
	{ 0x9aa, 0x9b0 },
	{ 0x9b2, 0x9b2 },
	{ 0x9b6, 0x9b9 },
	{ 0x9bd, 0x9bd },
	{ 0x9ce, 0x9ce },
	{ 0x9dc, 0x9dd },
	{ 0x9df, 0x9e1 },
	{ 0x9f0, 0x9f1 },
	{ 0x9fc, 0x9fc },
	{ 0xa05, 0xa0a },
	{ 0xa0f, 0xa10 },
	{ 0xa13, 0xa28 },
	{ 0xa2a, 0xa30 },
	{ 0xa32, 0xa33 },
	{ 0xa35, 0xa36 },
	{ 0xa38, 0xa39 },
	{ 0xa59, 0xa5c },
	{ 0xa5e, 0xa5e },
	{ 0xa72, 0xa74 },
	{ 0xa85, 0xa8d },
	{ 0xa8f, 0xa91 },
	{ 0xa93, 0xaa8 },
	{ 0xaaa, 0xab0 },
	{ 0xab2, 0xab3 },
	{ 0xab5, 0xab9 },
	{ 0xabd, 0xabd },
	{ 0xad0, 0xad0 },
	{ 0xae0, 0xae1 },
	{ 0xaf9, 0xaf9 },
	{ 0xb05, 0xb0c },
	{ 0xb0f, 0xb10 },
	{ 0xb13, 0xb28 },
	{ 0xb2a, 0xb30 },
	{ 0xb32, 0xb33 },
	{ 0xb35, 0xb39 },
	{ 0xb3d, 0xb3d },
	{ 0xb5c, 0xb5d },
	{ 0xb5f, 0xb61 },
	{ 0xb71, 0xb71 },
	{ 0xb83, 0xb83 },
	{ 0xb85, 0xb8a },
	{ 0xb8e, 0xb90 },
	{ 0xb92, 0xb95 },
	{ 0xb99, 0xb9a },
	{ 0xb9c, 0xb9c },
	{ 0xb9e, 0xb9f },
	{ 0xba3, 0xba4 },
	{ 0xba8, 0xbaa },
	{ 0xbae, 0xbb9 },
	{ 0xbd0, 0xbd0 },
	{ 0xc05, 0xc0c },
	{ 0xc0e, 0xc10 },
	{ 0xc12, 0xc28 },
	{ 0xc2a, 0xc39 },
	{ 0xc3d, 0xc3d },
	{ 0xc58, 0xc5a },
	{ 0xc5c, 0xc5d },
	{ 0xc60, 0xc61 },
	{ 0xc80, 0xc80 },
	{ 0xc85, 0xc8c },
	{ 0xc8e, 0xc90 },
	{ 0xc92, 0xca8 },
	{ 0xcaa, 0xcb3 },
	{ 0xcb5, 0xcb9 },
	{ 0xcbd, 0xcbd },
	{ 0xcdc, 0xcde },
	{ 0xce0, 0xce1 },
	{ 0xcf1, 0xcf2 },
	{ 0xd04, 0xd0c },
	{ 0xd0e, 0xd10 },
	{ 0xd12, 0xd3a },
	{ 0xd3d, 0xd3d },
	{ 0xd4e, 0xd4e },
	{ 0xd54, 0xd56 },
	{ 0xd5f, 0xd61 },
	{ 0xd7a, 0xd7f },
	{ 0xd85, 0xd96 },
	{ 0xd9a, 0xdb1 },
	{ 0xdb3, 0xdbb },
	{ 0xdbd, 0xdbd },
	{ 0xdc0, 0xdc6 },
	{ 0xe01, 0xe30 },
	{ 0xe32, 0xe32 },
	{ 0xe40, 0xe46 },
	{ 0xe81, 0xe82 },
	{ 0xe84, 0xe84 },
	{ 0xe86, 0xe8a },
	{ 0xe8c, 0xea3 },
	{ 0xea5, 0xea5 },
	{ 0xea7, 0xeb0 },
	{ 0xeb2, 0xeb2 },
	{ 0xebd, 0xebd },
	{ 0xec0, 0xec4 },
	{ 0xec6, 0xec6 },
	{ 0xedc, 0xedf },
	{ 0xf00, 0xf00 },
	{ 0xf40, 0xf47 },
	{ 0xf49, 0xf6c },
	{ 0xf88, 0xf8c },
	{ 0x1000, 0x102a },
	{ 0x103f, 0x103f },
	{ 0x1050, 0x1055 },
	{ 0x105a, 0x105d },
	{ 0x1061, 0x1061 },
	{ 0x1065, 0x1066 },
	{ 0x106e, 0x1070 },
	{ 0x1075, 0x1081 },
	{ 0x108e, 0x108e },
	{ 0x10a0, 0x10c5 },
	{ 0x10c7, 0x10c7 },
	{ 0x10cd, 0x10cd },
	{ 0x10d0, 0x10fa },
	{ 0x10fc, 0x1248 },
	{ 0x124a, 0x124d },
	{ 0x1250, 0x1256 },
	{ 0x1258, 0x1258 },
	{ 0x125a, 0x125d },
	{ 0x1260, 0x1288 },
	{ 0x128a, 0x128d },
	{ 0x1290, 0x12b0 },
	{ 0x12b2, 0x12b5 },
	{ 0x12b8, 0x12be },
	{ 0x12c0, 0x12c0 },
	{ 0x12c2, 0x12c5 },
	{ 0x12c8, 0x12d6 },
	{ 0x12d8, 0x1310 },
	{ 0x1312, 0x1315 },
	{ 0x1318, 0x135a },
	{ 0x1380, 0x138f },
	{ 0x13a0, 0x13f5 },
	{ 0x13f8, 0x13fd },
	{ 0x1401, 0x166c },
	{ 0x166f, 0x167f },
	{ 0x1681, 0x169a },
	{ 0x16a0, 0x16ea },
	{ 0x16ee, 0x16f8 },
	{ 0x1700, 0x1711 },
	{ 0x171f, 0x1731 },
	{ 0x1740, 0x1751 },
	{ 0x1760, 0x176c },
	{ 0x176e, 0x1770 },
	{ 0x1780, 0x17b3 },
	{ 0x17d7, 0x17d7 },
	{ 0x17dc, 0x17dc },
	{ 0x1820, 0x1878 },
	{ 0x1880, 0x18a8 },
	{ 0x18aa, 0x18aa },
	{ 0x18b0, 0x18f5 },
	{ 0x1900, 0x191e },
	{ 0x1950, 0x196d },
	{ 0x1970, 0x1974 },
	{ 0x1980, 0x19ab },
	{ 0x19b0, 0x19c9 },
	{ 0x1a00, 0x1a16 },
	{ 0x1a20, 0x1a54 },
	{ 0x1aa7, 0x1aa7 },
	{ 0x1b05, 0x1b33 },
	{ 0x1b45, 0x1b4c },
	{ 0x1b83, 0x1ba0 },
	{ 0x1bae, 0x1baf },
	{ 0x1bba, 0x1be5 },
	{ 0x1c00, 0x1c23 },
	{ 0x1c4d, 0x1c4f },
	{ 0x1c5a, 0x1c7d },
	{ 0x1c80, 0x1c8a },
	{ 0x1c90, 0x1cba },
	{ 0x1cbd, 0x1cbf },
	{ 0x1ce9, 0x1cec },
	{ 0x1cee, 0x1cf3 },
	{ 0x1cf5, 0x1cf6 },
	{ 0x1cfa, 0x1cfa },
	{ 0x1d00, 0x1dbf },
	{ 0x1e00, 0x1f15 },
	{ 0x1f18, 0x1f1d },
	{ 0x1f20, 0x1f45 },
	{ 0x1f48, 0x1f4d },
	{ 0x1f50, 0x1f57 },
	{ 0x1f59, 0x1f59 },
	{ 0x1f5b, 0x1f5b },
	{ 0x1f5d, 0x1f5d },
	{ 0x1f5f, 0x1f7d },
	{ 0x1f80, 0x1fb4 },
	{ 0x1fb6, 0x1fbc },
	{ 0x1fbe, 0x1fbe },
	{ 0x1fc2, 0x1fc4 },
	{ 0x1fc6, 0x1fcc },
	{ 0x1fd0, 0x1fd3 },
	{ 0x1fd6, 0x1fdb },
	{ 0x1fe0, 0x1fec },
	{ 0x1ff2, 0x1ff4 },
	{ 0x1ff6, 0x1ffc },
	{ 0x2071, 0x2071 },
	{ 0x207f, 0x207f },
	{ 0x2090, 0x209c },
	{ 0x2102, 0x2102 },
	{ 0x2107, 0x2107 },
	{ 0x210a, 0x2113 },
	{ 0x2115, 0x2115 },
	{ 0x2118, 0x211d },
	{ 0x2124, 0x2124 },
	{ 0x2126, 0x2126 },
	{ 0x2128, 0x2128 },
	{ 0x212a, 0x2139 },
	{ 0x213c, 0x213f },
	{ 0x2145, 0x2149 },
	{ 0x214e, 0x214e },
	{ 0x2160, 0x2188 },
	{ 0x2c00, 0x2ce4 },
	{ 0x2ceb, 0x2cee },
	{ 0x2cf2, 0x2cf3 },
	{ 0x2d00, 0x2d25 },
	{ 0x2d27, 0x2d27 },
	{ 0x2d2d, 0x2d2d },
	{ 0x2d30, 0x2d67 },
	{ 0x2d6f, 0x2d6f },
	{ 0x2d80, 0x2d96 },
	{ 0x2da0, 0x2da6 },
	{ 0x2da8, 0x2dae },
	{ 0x2db0, 0x2db6 },
	{ 0x2db8, 0x2dbe },
	{ 0x2dc0, 0x2dc6 },
	{ 0x2dc8, 0x2dce },
	{ 0x2dd0, 0x2dd6 },
	{ 0x2dd8, 0x2dde },
	{ 0x3005, 0x3007 },
	{ 0x3021, 0x3029 },
	{ 0x3031, 0x3035 },
	{ 0x3038, 0x303c },
	{ 0x3041, 0x3096 },
	{ 0x309d, 0x309f },
	{ 0x30a1, 0x30fa },
	{ 0x30fc, 0x30ff },
	{ 0x3105, 0x312f },
	{ 0x3131, 0x318e },
	{ 0x31a0, 0x31bf },
	{ 0x31f0, 0x31ff },
	{ 0x3400, 0x4dbf },
	{ 0x4e00, 0xa48c },
	{ 0xa4d0, 0xa4fd },
	{ 0xa500, 0xa60c },
	{ 0xa610, 0xa61f },
	{ 0xa62a, 0xa62b },
	{ 0xa640, 0xa66e },
	{ 0xa67f, 0xa69d },
	{ 0xa6a0, 0xa6ef },
	{ 0xa717, 0xa71f },
	{ 0xa722, 0xa788 },
	{ 0xa78b, 0xa7dc },
	{ 0xa7f1, 0xa801 },
	{ 0xa803, 0xa805 },
	{ 0xa807, 0xa80a },
	{ 0xa80c, 0xa822 },
	{ 0xa840, 0xa873 },
	{ 0xa882, 0xa8b3 },
	{ 0xa8f2, 0xa8f7 },
	{ 0xa8fb, 0xa8fb },
	{ 0xa8fd, 0xa8fe },
	{ 0xa90a, 0xa925 },
	{ 0xa930, 0xa946 },
	{ 0xa960, 0xa97c },
	{ 0xa984, 0xa9b2 },
	{ 0xa9cf, 0xa9cf },
	{ 0xa9e0, 0xa9e4 },
	{ 0xa9e6, 0xa9ef },
	{ 0xa9fa, 0xa9fe },
	{ 0xaa00, 0xaa28 },
	{ 0xaa40, 0xaa42 },
	{ 0xaa44, 0xaa4b },
	{ 0xaa60, 0xaa76 },
	{ 0xaa7a, 0xaa7a },
	{ 0xaa7e, 0xaaaf },
	{ 0xaab1, 0xaab1 },
	{ 0xaab5, 0xaab6 },
	{ 0xaab9, 0xaabd },
	{ 0xaac0, 0xaac0 },
	{ 0xaac2, 0xaac2 },
	{ 0xaadb, 0xaadd },
	{ 0xaae0, 0xaaea },
	{ 0xaaf2, 0xaaf4 },
	{ 0xab01, 0xab06 },
	{ 0xab09, 0xab0e },
	{ 0xab11, 0xab16 },
	{ 0xab20, 0xab26 },
	{ 0xab28, 0xab2e },
	{ 0xab30, 0xab5a },
	{ 0xab5c, 0xab69 },
	{ 0xab70, 0xabe2 },
	{ 0xac00, 0xd7a3 },
	{ 0xd7b0, 0xd7c6 },
	{ 0xd7cb, 0xd7fb },
	{ 0xf900, 0xfa6d },
	{ 0xfa70, 0xfad9 },
	{ 0xfb00, 0xfb06 },
	{ 0xfb13, 0xfb17 },
	{ 0xfb1d, 0xfb1d },
	{ 0xfb1f, 0xfb28 },
	{ 0xfb2a, 0xfb36 },
	{ 0xfb38, 0xfb3c },
	{ 0xfb3e, 0xfb3e },
	{ 0xfb40, 0xfb41 },
	{ 0xfb43, 0xfb44 },
	{ 0xfb46, 0xfbb1 },
	{ 0xfbd3, 0xfc5d },
	{ 0xfc64, 0xfd3d },
	{ 0xfd50, 0xfd8f },
	{ 0xfd92, 0xfdc7 },
	{ 0xfdf0, 0xfdf9 },
	{ 0xfe71, 0xfe71 },
	{ 0xfe73, 0xfe73 },
	{ 0xfe77, 0xfe77 },
	{ 0xfe79, 0xfe79 },
	{ 0xfe7b, 0xfe7b },
	{ 0xfe7d, 0xfe7d },
	{ 0xfe7f, 0xfefc },
	{ 0xff21, 0xff3a },
	{ 0xff41, 0xff5a },
	{ 0xff66, 0xff9d },
	{ 0xffa0, 0xffbe },
	{ 0xffc2, 0xffc7 },
	{ 0xffca, 0xffcf },
	{ 0xffd2, 0xffd7 },
	{ 0xffda, 0xffdc },
	{ 0x10000, 0x1000b },
	{ 0x1000d, 0x10026 },
	{ 0x10028, 0x1003a },
	{ 0x1003c, 0x1003d },
	{ 0x1003f, 0x1004d },
	{ 0x10050, 0x1005d },
	{ 0x10080, 0x100fa },
	{ 0x10140, 0x10174 },
	{ 0x10280, 0x1029c },
	{ 0x102a0, 0x102d0 },
	{ 0x10300, 0x1031f },
	{ 0x1032d, 0x1034a },
	{ 0x10350, 0x10375 },
	{ 0x10380, 0x1039d },
	{ 0x103a0, 0x103c3 },
	{ 0x103c8, 0x103cf },
	{ 0x103d1, 0x103d5 },
	{ 0x10400, 0x1049d },
	{ 0x104b0, 0x104d3 },
	{ 0x104d8, 0x104fb },
	{ 0x10500, 0x10527 },
	{ 0x10530, 0x10563 },
	{ 0x10570, 0x1057a },
	{ 0x1057c, 0x1058a },
	{ 0x1058c, 0x10592 },
	{ 0x10594, 0x10595 },
	{ 0x10597, 0x105a1 },
	{ 0x105a3, 0x105b1 },
	{ 0x105b3, 0x105b9 },
	{ 0x105bb, 0x105bc },
	{ 0x105c0, 0x105f3 },
	{ 0x10600, 0x10736 },
	{ 0x10740, 0x10755 },
	{ 0x10760, 0x10767 },
	{ 0x10780, 0x10785 },
	{ 0x10787, 0x107b0 },
	{ 0x107b2, 0x107ba },
	{ 0x10800, 0x10805 },
	{ 0x10808, 0x10808 },
	{ 0x1080a, 0x10835 },
	{ 0x10837, 0x10838 },
	{ 0x1083c, 0x1083c },
	{ 0x1083f, 0x10855 },
	{ 0x10860, 0x10876 },
	{ 0x10880, 0x1089e },
	{ 0x108e0, 0x108f2 },
	{ 0x108f4, 0x108f5 },
	{ 0x10900, 0x10915 },
	{ 0x10920, 0x10939 },
	{ 0x10940, 0x10959 },
	{ 0x10980, 0x109b7 },
	{ 0x109be, 0x109bf },
	{ 0x10a00, 0x10a00 },
	{ 0x10a10, 0x10a13 },
	{ 0x10a15, 0x10a17 },
	{ 0x10a19, 0x10a35 },
	{ 0x10a60, 0x10a7c },
	{ 0x10a80, 0x10a9c },
	{ 0x10ac0, 0x10ac7 },
	{ 0x10ac9, 0x10ae4 },
	{ 0x10b00, 0x10b35 },
	{ 0x10b40, 0x10b55 },
	{ 0x10b60, 0x10b72 },
	{ 0x10b80, 0x10b91 },
	{ 0x10c00, 0x10c48 },
	{ 0x10c80, 0x10cb2 },
	{ 0x10cc0, 0x10cf2 },
	{ 0x10d00, 0x10d23 },
	{ 0x10d4a, 0x10d65 },
	{ 0x10d6f, 0x10d85 },
	{ 0x10e80, 0x10ea9 },
	{ 0x10eb0, 0x10eb1 },
	{ 0x10ec2, 0x10ec7 },
	{ 0x10f00, 0x10f1c },
	{ 0x10f27, 0x10f27 },
	{ 0x10f30, 0x10f45 },
	{ 0x10f70, 0x10f81 },
	{ 0x10fb0, 0x10fc4 },
	{ 0x10fe0, 0x10ff6 },
	{ 0x11003, 0x11037 },
	{ 0x11071, 0x11072 },
	{ 0x11075, 0x11075 },
	{ 0x11083, 0x110af },
	{ 0x110d0, 0x110e8 },
	{ 0x11103, 0x11126 },
	{ 0x11144, 0x11144 },
	{ 0x11147, 0x11147 },
	{ 0x11150, 0x11172 },
	{ 0x11176, 0x11176 },
	{ 0x11183, 0x111b2 },
	{ 0x111c1, 0x111c4 },
	{ 0x111da, 0x111da },
	{ 0x111dc, 0x111dc },
	{ 0x11200, 0x11211 },
	{ 0x11213, 0x1122b },
	{ 0x1123f, 0x11240 },
	{ 0x11280, 0x11286 },
	{ 0x11288, 0x11288 },
	{ 0x1128a, 0x1128d },
	{ 0x1128f, 0x1129d },
	{ 0x1129f, 0x112a8 },
	{ 0x112b0, 0x112de },
	{ 0x11305, 0x1130c },
	{ 0x1130f, 0x11310 },
	{ 0x11313, 0x11328 },
	{ 0x1132a, 0x11330 },
	{ 0x11332, 0x11333 },
	{ 0x11335, 0x11339 },
	{ 0x1133d, 0x1133d },
	{ 0x11350, 0x11350 },
	{ 0x1135d, 0x11361 },
	{ 0x11380, 0x11389 },
	{ 0x1138b, 0x1138b },
	{ 0x1138e, 0x1138e },
	{ 0x11390, 0x113b5 },
	{ 0x113b7, 0x113b7 },
	{ 0x113d1, 0x113d1 },
	{ 0x113d3, 0x113d3 },
	{ 0x11400, 0x11434 },
	{ 0x11447, 0x1144a },
	{ 0x1145f, 0x11461 },
	{ 0x11480, 0x114af },
	{ 0x114c4, 0x114c5 },
	{ 0x114c7, 0x114c7 },
	{ 0x11580, 0x115ae },
	{ 0x115d8, 0x115db },
	{ 0x11600, 0x1162f },
	{ 0x11644, 0x11644 },
	{ 0x11680, 0x116aa },
	{ 0x116b8, 0x116b8 },
	{ 0x11700, 0x1171a },
	{ 0x11740, 0x11746 },
	{ 0x11800, 0x1182b },
	{ 0x118a0, 0x118df },
	{ 0x118ff, 0x11906 },
	{ 0x11909, 0x11909 },
	{ 0x1190c, 0x11913 },
	{ 0x11915, 0x11916 },
	{ 0x11918, 0x1192f },
	{ 0x1193f, 0x1193f },
	{ 0x11941, 0x11941 },
	{ 0x119a0, 0x119a7 },
	{ 0x119aa, 0x119d0 },
	{ 0x119e1, 0x119e1 },
	{ 0x119e3, 0x119e3 },
	{ 0x11a00, 0x11a00 },
	{ 0x11a0b, 0x11a32 },
	{ 0x11a3a, 0x11a3a },
	{ 0x11a50, 0x11a50 },
	{ 0x11a5c, 0x11a89 },
	{ 0x11a9d, 0x11a9d },
	{ 0x11ab0, 0x11af8 },
	{ 0x11bc0, 0x11be0 },
	{ 0x11c00, 0x11c08 },
	{ 0x11c0a, 0x11c2e },
	{ 0x11c40, 0x11c40 },
	{ 0x11c72, 0x11c8f },
	{ 0x11d00, 0x11d06 },
	{ 0x11d08, 0x11d09 },
	{ 0x11d0b, 0x11d30 },
	{ 0x11d46, 0x11d46 },
	{ 0x11d60, 0x11d65 },
	{ 0x11d67, 0x11d68 },
	{ 0x11d6a, 0x11d89 },
	{ 0x11d98, 0x11d98 },
	{ 0x11db0, 0x11ddb },
	{ 0x11ee0, 0x11ef2 },
	{ 0x11f02, 0x11f02 },
	{ 0x11f04, 0x11f10 },
	{ 0x11f12, 0x11f33 },
	{ 0x11fb0, 0x11fb0 },
	{ 0x12000, 0x12399 },
	{ 0x12400, 0x1246e },
	{ 0x12480, 0x12543 },
	{ 0x12f90, 0x12ff0 },
	{ 0x13000, 0x1342f },
	{ 0x13441, 0x13446 },
	{ 0x13460, 0x143fa },
	{ 0x14400, 0x14646 },
	{ 0x16100, 0x1611d },
	{ 0x16800, 0x16a38 },
	{ 0x16a40, 0x16a5e },
	{ 0x16a70, 0x16abe },
	{ 0x16ad0, 0x16aed },
	{ 0x16b00, 0x16b2f },
	{ 0x16b40, 0x16b43 },
	{ 0x16b63, 0x16b77 },
	{ 0x16b7d, 0x16b8f },
	{ 0x16d40, 0x16d6c },
	{ 0x16e40, 0x16e7f },
	{ 0x16ea0, 0x16eb8 },
	{ 0x16ebb, 0x16ed3 },
	{ 0x16f00, 0x16f4a },
	{ 0x16f50, 0x16f50 },
	{ 0x16f93, 0x16f9f },
	{ 0x16fe0, 0x16fe1 },
	{ 0x16fe3, 0x16fe3 },
	{ 0x16ff2, 0x16ff6 },
	{ 0x17000, 0x18cd5 },
	{ 0x18cff, 0x18d1e },
	{ 0x18d80, 0x18df2 },
	{ 0x1aff0, 0x1aff3 },
	{ 0x1aff5, 0x1affb },
	{ 0x1affd, 0x1affe },
	{ 0x1b000, 0x1b122 },
	{ 0x1b132, 0x1b132 },
	{ 0x1b150, 0x1b152 },
	{ 0x1b155, 0x1b155 },
	{ 0x1b164, 0x1b167 },
	{ 0x1b170, 0x1b2fb },
	{ 0x1bc00, 0x1bc6a },
	{ 0x1bc70, 0x1bc7c },
	{ 0x1bc80, 0x1bc88 },
	{ 0x1bc90, 0x1bc99 },
	{ 0x1d400, 0x1d454 },
	{ 0x1d456, 0x1d49c },
	{ 0x1d49e, 0x1d49f },
	{ 0x1d4a2, 0x1d4a2 },
	{ 0x1d4a5, 0x1d4a6 },
	{ 0x1d4a9, 0x1d4ac },
	{ 0x1d4ae, 0x1d4b9 },
	{ 0x1d4bb, 0x1d4bb },
	{ 0x1d4bd, 0x1d4c3 },
	{ 0x1d4c5, 0x1d505 },
	{ 0x1d507, 0x1d50a },
	{ 0x1d50d, 0x1d514 },
	{ 0x1d516, 0x1d51c },
	{ 0x1d51e, 0x1d539 },
	{ 0x1d53b, 0x1d53e },
	{ 0x1d540, 0x1d544 },
	{ 0x1d546, 0x1d546 },
	{ 0x1d54a, 0x1d550 },
	{ 0x1d552, 0x1d6a5 },
	{ 0x1d6a8, 0x1d6c0 },
	{ 0x1d6c2, 0x1d6da },
	{ 0x1d6dc, 0x1d6fa },
	{ 0x1d6fc, 0x1d714 },
	{ 0x1d716, 0x1d734 },
	{ 0x1d736, 0x1d74e },
	{ 0x1d750, 0x1d76e },
	{ 0x1d770, 0x1d788 },
	{ 0x1d78a, 0x1d7a8 },
	{ 0x1d7aa, 0x1d7c2 },
	{ 0x1d7c4, 0x1d7cb },
	{ 0x1df00, 0x1df1e },
	{ 0x1df25, 0x1df2a },
	{ 0x1e030, 0x1e06d },
	{ 0x1e100, 0x1e12c },
	{ 0x1e137, 0x1e13d },
	{ 0x1e14e, 0x1e14e },
	{ 0x1e290, 0x1e2ad },
	{ 0x1e2c0, 0x1e2eb },
	{ 0x1e4d0, 0x1e4eb },
	{ 0x1e5d0, 0x1e5ed },
	{ 0x1e5f0, 0x1e5f0 },
	{ 0x1e6c0, 0x1e6de },
	{ 0x1e6e0, 0x1e6e2 },
	{ 0x1e6e4, 0x1e6e5 },
	{ 0x1e6e7, 0x1e6ed },
	{ 0x1e6f0, 0x1e6f4 },
	{ 0x1e6fe, 0x1e6ff },
	{ 0x1e7e0, 0x1e7e6 },
	{ 0x1e7e8, 0x1e7eb },
	{ 0x1e7ed, 0x1e7ee },
	{ 0x1e7f0, 0x1e7fe },
	{ 0x1e800, 0x1e8c4 },
	{ 0x1e900, 0x1e943 },
	{ 0x1e94b, 0x1e94b },
	{ 0x1ee00, 0x1ee03 },
	{ 0x1ee05, 0x1ee1f },
	{ 0x1ee21, 0x1ee22 },
	{ 0x1ee24, 0x1ee24 },
	{ 0x1ee27, 0x1ee27 },
	{ 0x1ee29, 0x1ee32 },
	{ 0x1ee34, 0x1ee37 },
	{ 0x1ee39, 0x1ee39 },
	{ 0x1ee3b, 0x1ee3b },
	{ 0x1ee42, 0x1ee42 },
	{ 0x1ee47, 0x1ee47 },
	{ 0x1ee49, 0x1ee49 },
	{ 0x1ee4b, 0x1ee4b },
	{ 0x1ee4d, 0x1ee4f },
	{ 0x1ee51, 0x1ee52 },
	{ 0x1ee54, 0x1ee54 },
	{ 0x1ee57, 0x1ee57 },
	{ 0x1ee59, 0x1ee59 },
	{ 0x1ee5b, 0x1ee5b },
	{ 0x1ee5d, 0x1ee5d },
	{ 0x1ee5f, 0x1ee5f },
	{ 0x1ee61, 0x1ee62 },
	{ 0x1ee64, 0x1ee64 },
	{ 0x1ee67, 0x1ee6a },
	{ 0x1ee6c, 0x1ee72 },
	{ 0x1ee74, 0x1ee77 },
	{ 0x1ee79, 0x1ee7c },
	{ 0x1ee7e, 0x1ee7e },
	{ 0x1ee80, 0x1ee89 },
	{ 0x1ee8b, 0x1ee9b },
	{ 0x1eea1, 0x1eea3 },
	{ 0x1eea5, 0x1eea9 },
	{ 0x1eeab, 0x1eebb },
	{ 0x20000, 0x2a6df },
	{ 0x2a700, 0x2b81d },
	{ 0x2b820, 0x2cead },
	{ 0x2ceb0, 0x2ebe0 },
	{ 0x2ebf0, 0x2ee5d },
	{ 0x2f800, 0x2fa1d },
	{ 0x30000, 0x3134a },
	{ 0x31350, 0x33479 },
};

const int xid_continue_size = 806;
const CharRange xid_continue[xid_continue_size] = {
	{ 0x30, 0x39 },
	{ 0x41, 0x5a },
	{ 0x5f, 0x5f },
	{ 0x61, 0x7a },
	{ 0xaa, 0xaa },
	{ 0xb5, 0xb5 },
	{ 0xb7, 0xb7 },
	{ 0xba, 0xba },
	{ 0xc0, 0xd6 },
	{ 0xd8, 0xf6 },
	{ 0xf8, 0x2c1 },
	{ 0x2c6, 0x2d1 },
	{ 0x2e0, 0x2e4 },
	{ 0x2ec, 0x2ec },
	{ 0x2ee, 0x2ee },
	{ 0x300, 0x374 },
	{ 0x376, 0x377 },
	{ 0x37b, 0x37d },
	{ 0x37f, 0x37f },
	{ 0x386, 0x38a },
	{ 0x38c, 0x38c },
	{ 0x38e, 0x3a1 },
	{ 0x3a3, 0x3f5 },
	{ 0x3f7, 0x481 },
	{ 0x483, 0x487 },
	{ 0x48a, 0x52f },
	{ 0x531, 0x556 },
	{ 0x559, 0x559 },
	{ 0x560, 0x588 },
	{ 0x591, 0x5bd },
	{ 0x5bf, 0x5bf },
	{ 0x5c1, 0x5c2 },
	{ 0x5c4, 0x5c5 },
	{ 0x5c7, 0x5c7 },
	{ 0x5d0, 0x5ea },
	{ 0x5ef, 0x5f2 },
	{ 0x610, 0x61a },
	{ 0x620, 0x669 },
	{ 0x66e, 0x6d3 },
	{ 0x6d5, 0x6dc },
	{ 0x6df, 0x6e8 },
	{ 0x6ea, 0x6fc },
	{ 0x6ff, 0x6ff },
	{ 0x710, 0x74a },
	{ 0x74d, 0x7b1 },
	{ 0x7c0, 0x7f5 },
	{ 0x7fa, 0x7fa },
	{ 0x7fd, 0x7fd },
	{ 0x800, 0x82d },
	{ 0x840, 0x85b },
	{ 0x860, 0x86a },
	{ 0x870, 0x887 },
	{ 0x889, 0x88f },
	{ 0x897, 0x8e1 },
	{ 0x8e3, 0x963 },
	{ 0x966, 0x96f },
	{ 0x971, 0x983 },
	{ 0x985, 0x98c },
	{ 0x98f, 0x990 },
	{ 0x993, 0x9a8 },
	{ 0x9aa, 0x9b0 },
	{ 0x9b2, 0x9b2 },
	{ 0x9b6, 0x9b9 },
	{ 0x9bc, 0x9c4 },
	{ 0x9c7, 0x9c8 },
	{ 0x9cb, 0x9ce },
	{ 0x9d7, 0x9d7 },
	{ 0x9dc, 0x9dd },
	{ 0x9df, 0x9e3 },
	{ 0x9e6, 0x9f1 },
	{ 0x9fc, 0x9fc },
	{ 0x9fe, 0x9fe },
	{ 0xa01, 0xa03 },
	{ 0xa05, 0xa0a },
	{ 0xa0f, 0xa10 },
	{ 0xa13, 0xa28 },
	{ 0xa2a, 0xa30 },
	{ 0xa32, 0xa33 },
	{ 0xa35, 0xa36 },
	{ 0xa38, 0xa39 },
	{ 0xa3c, 0xa3c },
	{ 0xa3e, 0xa42 },
	{ 0xa47, 0xa48 },
	{ 0xa4b, 0xa4d },
	{ 0xa51, 0xa51 },
	{ 0xa59, 0xa5c },
	{ 0xa5e, 0xa5e },
	{ 0xa66, 0xa75 },
	{ 0xa81, 0xa83 },
	{ 0xa85, 0xa8d },
	{ 0xa8f, 0xa91 },
	{ 0xa93, 0xaa8 },
	{ 0xaaa, 0xab0 },
	{ 0xab2, 0xab3 },
	{ 0xab5, 0xab9 },
	{ 0xabc, 0xac5 },
	{ 0xac7, 0xac9 },
	{ 0xacb, 0xacd },
	{ 0xad0, 0xad0 },
	{ 0xae0, 0xae3 },
	{ 0xae6, 0xaef },
	{ 0xaf9, 0xaff },
	{ 0xb01, 0xb03 },
	{ 0xb05, 0xb0c },
	{ 0xb0f, 0xb10 },
	{ 0xb13, 0xb28 },
	{ 0xb2a, 0xb30 },
	{ 0xb32, 0xb33 },
	{ 0xb35, 0xb39 },
	{ 0xb3c, 0xb44 },
	{ 0xb47, 0xb48 },
	{ 0xb4b, 0xb4d },
	{ 0xb55, 0xb57 },
	{ 0xb5c, 0xb5d },
	{ 0xb5f, 0xb63 },
	{ 0xb66, 0xb6f },
	{ 0xb71, 0xb71 },
	{ 0xb82, 0xb83 },
	{ 0xb85, 0xb8a },
	{ 0xb8e, 0xb90 },
	{ 0xb92, 0xb95 },
	{ 0xb99, 0xb9a },
	{ 0xb9c, 0xb9c },
	{ 0xb9e, 0xb9f },
	{ 0xba3, 0xba4 },
	{ 0xba8, 0xbaa },
	{ 0xbae, 0xbb9 },
	{ 0xbbe, 0xbc2 },
	{ 0xbc6, 0xbc8 },
	{ 0xbca, 0xbcd },
	{ 0xbd0, 0xbd0 },
	{ 0xbd7, 0xbd7 },
	{ 0xbe6, 0xbef },
	{ 0xc00, 0xc0c },
	{ 0xc0e, 0xc10 },
	{ 0xc12, 0xc28 },
	{ 0xc2a, 0xc39 },
	{ 0xc3c, 0xc44 },
	{ 0xc46, 0xc48 },
	{ 0xc4a, 0xc4d },
	{ 0xc55, 0xc56 },
	{ 0xc58, 0xc5a },
	{ 0xc5c, 0xc5d },
	{ 0xc60, 0xc63 },
	{ 0xc66, 0xc6f },
	{ 0xc80, 0xc83 },
	{ 0xc85, 0xc8c },
	{ 0xc8e, 0xc90 },
	{ 0xc92, 0xca8 },
	{ 0xcaa, 0xcb3 },
	{ 0xcb5, 0xcb9 },
	{ 0xcbc, 0xcc4 },
	{ 0xcc6, 0xcc8 },
	{ 0xcca, 0xccd },
	{ 0xcd5, 0xcd6 },
	{ 0xcdc, 0xcde },
	{ 0xce0, 0xce3 },
	{ 0xce6, 0xcef },
	{ 0xcf1, 0xcf3 },
	{ 0xd00, 0xd0c },
	{ 0xd0e, 0xd10 },
	{ 0xd12, 0xd44 },
	{ 0xd46, 0xd48 },
	{ 0xd4a, 0xd4e },
	{ 0xd54, 0xd57 },
	{ 0xd5f, 0xd63 },
	{ 0xd66, 0xd6f },
	{ 0xd7a, 0xd7f },
	{ 0xd81, 0xd83 },
	{ 0xd85, 0xd96 },
	{ 0xd9a, 0xdb1 },
	{ 0xdb3, 0xdbb },
	{ 0xdbd, 0xdbd },
	{ 0xdc0, 0xdc6 },
	{ 0xdca, 0xdca },
	{ 0xdcf, 0xdd4 },
	{ 0xdd6, 0xdd6 },
	{ 0xdd8, 0xddf },
	{ 0xde6, 0xdef },
	{ 0xdf2, 0xdf3 },
	{ 0xe01, 0xe3a },
	{ 0xe40, 0xe4e },
	{ 0xe50, 0xe59 },
	{ 0xe81, 0xe82 },
	{ 0xe84, 0xe84 },
	{ 0xe86, 0xe8a },
	{ 0xe8c, 0xea3 },
	{ 0xea5, 0xea5 },
	{ 0xea7, 0xebd },
	{ 0xec0, 0xec4 },
	{ 0xec6, 0xec6 },
	{ 0xec8, 0xece },
	{ 0xed0, 0xed9 },
	{ 0xedc, 0xedf },
	{ 0xf00, 0xf00 },
	{ 0xf18, 0xf19 },
	{ 0xf20, 0xf29 },
	{ 0xf35, 0xf35 },
	{ 0xf37, 0xf37 },
	{ 0xf39, 0xf39 },
	{ 0xf3e, 0xf47 },
	{ 0xf49, 0xf6c },
	{ 0xf71, 0xf84 },
	{ 0xf86, 0xf97 },
	{ 0xf99, 0xfbc },
	{ 0xfc6, 0xfc6 },
	{ 0x1000, 0x1049 },
	{ 0x1050, 0x109d },
	{ 0x10a0, 0x10c5 },
	{ 0x10c7, 0x10c7 },
	{ 0x10cd, 0x10cd },
	{ 0x10d0, 0x10fa },
	{ 0x10fc, 0x1248 },
	{ 0x124a, 0x124d },
	{ 0x1250, 0x1256 },
	{ 0x1258, 0x1258 },
	{ 0x125a, 0x125d },
	{ 0x1260, 0x1288 },
	{ 0x128a, 0x128d },
	{ 0x1290, 0x12b0 },
	{ 0x12b2, 0x12b5 },
	{ 0x12b8, 0x12be },
	{ 0x12c0, 0x12c0 },
	{ 0x12c2, 0x12c5 },
	{ 0x12c8, 0x12d6 },
	{ 0x12d8, 0x1310 },
	{ 0x1312, 0x1315 },
	{ 0x1318, 0x135a },
	{ 0x135d, 0x135f },
	{ 0x1369, 0x1371 },
	{ 0x1380, 0x138f },
	{ 0x13a0, 0x13f5 },
	{ 0x13f8, 0x13fd },
	{ 0x1401, 0x166c },
	{ 0x166f, 0x167f },
	{ 0x1681, 0x169a },
	{ 0x16a0, 0x16ea },
	{ 0x16ee, 0x16f8 },
	{ 0x1700, 0x1715 },
	{ 0x171f, 0x1734 },
	{ 0x1740, 0x1753 },
	{ 0x1760, 0x176c },
	{ 0x176e, 0x1770 },
	{ 0x1772, 0x1773 },
	{ 0x1780, 0x17d3 },
	{ 0x17d7, 0x17d7 },
	{ 0x17dc, 0x17dd },
	{ 0x17e0, 0x17e9 },
	{ 0x180b, 0x180d },
	{ 0x180f, 0x1819 },
	{ 0x1820, 0x1878 },
	{ 0x1880, 0x18aa },
	{ 0x18b0, 0x18f5 },
	{ 0x1900, 0x191e },
	{ 0x1920, 0x192b },
	{ 0x1930, 0x193b },
	{ 0x1946, 0x196d },
	{ 0x1970, 0x1974 },
	{ 0x1980, 0x19ab },
	{ 0x19b0, 0x19c9 },
	{ 0x19d0, 0x19da },
	{ 0x1a00, 0x1a1b },
	{ 0x1a20, 0x1a5e },
	{ 0x1a60, 0x1a7c },
	{ 0x1a7f, 0x1a89 },
	{ 0x1a90, 0x1a99 },
	{ 0x1aa7, 0x1aa7 },
	{ 0x1ab0, 0x1abd },
	{ 0x1abf, 0x1add },
	{ 0x1ae0, 0x1aeb },
	{ 0x1b00, 0x1b4c },
	{ 0x1b50, 0x1b59 },
	{ 0x1b6b, 0x1b73 },
	{ 0x1b80, 0x1bf3 },
	{ 0x1c00, 0x1c37 },
	{ 0x1c40, 0x1c49 },
	{ 0x1c4d, 0x1c7d },
	{ 0x1c80, 0x1c8a },
	{ 0x1c90, 0x1cba },
	{ 0x1cbd, 0x1cbf },
	{ 0x1cd0, 0x1cd2 },
	{ 0x1cd4, 0x1cfa },
	{ 0x1d00, 0x1f15 },
	{ 0x1f18, 0x1f1d },
	{ 0x1f20, 0x1f45 },
	{ 0x1f48, 0x1f4d },
	{ 0x1f50, 0x1f57 },
	{ 0x1f59, 0x1f59 },
	{ 0x1f5b, 0x1f5b },
	{ 0x1f5d, 0x1f5d },
	{ 0x1f5f, 0x1f7d },
	{ 0x1f80, 0x1fb4 },
	{ 0x1fb6, 0x1fbc },
	{ 0x1fbe, 0x1fbe },
	{ 0x1fc2, 0x1fc4 },
	{ 0x1fc6, 0x1fcc },
	{ 0x1fd0, 0x1fd3 },
	{ 0x1fd6, 0x1fdb },
	{ 0x1fe0, 0x1fec },
	{ 0x1ff2, 0x1ff4 },
	{ 0x1ff6, 0x1ffc },
	{ 0x200c, 0x200d },
	{ 0x203f, 0x2040 },
	{ 0x2054, 0x2054 },
	{ 0x2071, 0x2071 },
	{ 0x207f, 0x207f },
	{ 0x2090, 0x209c },
	{ 0x20d0, 0x20dc },
	{ 0x20e1, 0x20e1 },
	{ 0x20e5, 0x20f0 },
	{ 0x2102, 0x2102 },
	{ 0x2107, 0x2107 },
	{ 0x210a, 0x2113 },
	{ 0x2115, 0x2115 },
	{ 0x2118, 0x211d },
	{ 0x2124, 0x2124 },
	{ 0x2126, 0x2126 },
	{ 0x2128, 0x2128 },
	{ 0x212a, 0x2139 },
	{ 0x213c, 0x213f },
	{ 0x2145, 0x2149 },
	{ 0x214e, 0x214e },
	{ 0x2160, 0x2188 },
	{ 0x2c00, 0x2ce4 },
	{ 0x2ceb, 0x2cf3 },
	{ 0x2d00, 0x2d25 },
	{ 0x2d27, 0x2d27 },
	{ 0x2d2d, 0x2d2d },
	{ 0x2d30, 0x2d67 },
	{ 0x2d6f, 0x2d6f },
	{ 0x2d7f, 0x2d96 },
	{ 0x2da0, 0x2da6 },
	{ 0x2da8, 0x2dae },
	{ 0x2db0, 0x2db6 },
	{ 0x2db8, 0x2dbe },
	{ 0x2dc0, 0x2dc6 },
	{ 0x2dc8, 0x2dce },
	{ 0x2dd0, 0x2dd6 },
	{ 0x2dd8, 0x2dde },
	{ 0x2de0, 0x2dff },
	{ 0x3005, 0x3007 },
	{ 0x3021, 0x302f },
	{ 0x3031, 0x3035 },
	{ 0x3038, 0x303c },
	{ 0x3041, 0x3096 },
	{ 0x3099, 0x309a },
	{ 0x309d, 0x309f },
	{ 0x30a1, 0x30ff },
	{ 0x3105, 0x312f },
	{ 0x3131, 0x318e },
	{ 0x31a0, 0x31bf },
	{ 0x31f0, 0x31ff },
	{ 0x3400, 0x4dbf },
	{ 0x4e00, 0xa48c },
	{ 0xa4d0, 0xa4fd },
	{ 0xa500, 0xa60c },
	{ 0xa610, 0xa62b },
	{ 0xa640, 0xa66f },
	{ 0xa674, 0xa67d },
	{ 0xa67f, 0xa6f1 },
	{ 0xa717, 0xa71f },
	{ 0xa722, 0xa788 },
	{ 0xa78b, 0xa7dc },
	{ 0xa7f1, 0xa827 },
	{ 0xa82c, 0xa82c },
	{ 0xa840, 0xa873 },
	{ 0xa880, 0xa8c5 },
	{ 0xa8d0, 0xa8d9 },
	{ 0xa8e0, 0xa8f7 },
	{ 0xa8fb, 0xa8fb },
	{ 0xa8fd, 0xa92d },
	{ 0xa930, 0xa953 },
	{ 0xa960, 0xa97c },
	{ 0xa980, 0xa9c0 },
	{ 0xa9cf, 0xa9d9 },
	{ 0xa9e0, 0xa9fe },
	{ 0xaa00, 0xaa36 },
	{ 0xaa40, 0xaa4d },
	{ 0xaa50, 0xaa59 },
	{ 0xaa60, 0xaa76 },
	{ 0xaa7a, 0xaac2 },
	{ 0xaadb, 0xaadd },
	{ 0xaae0, 0xaaef },
	{ 0xaaf2, 0xaaf6 },
	{ 0xab01, 0xab06 },
	{ 0xab09, 0xab0e },
	{ 0xab11, 0xab16 },
	{ 0xab20, 0xab26 },
	{ 0xab28, 0xab2e },
	{ 0xab30, 0xab5a },
	{ 0xab5c, 0xab69 },
	{ 0xab70, 0xabea },
	{ 0xabec, 0xabed },
	{ 0xabf0, 0xabf9 },
	{ 0xac00, 0xd7a3 },
	{ 0xd7b0, 0xd7c6 },
	{ 0xd7cb, 0xd7fb },
	{ 0xf900, 0xfa6d },
	{ 0xfa70, 0xfad9 },
	{ 0xfb00, 0xfb06 },
	{ 0xfb13, 0xfb17 },
	{ 0xfb1d, 0xfb28 },
	{ 0xfb2a, 0xfb36 },
	{ 0xfb38, 0xfb3c },
	{ 0xfb3e, 0xfb3e },
	{ 0xfb40, 0xfb41 },
	{ 0xfb43, 0xfb44 },
	{ 0xfb46, 0xfbb1 },
	{ 0xfbd3, 0xfc5d },
	{ 0xfc64, 0xfd3d },
	{ 0xfd50, 0xfd8f },
	{ 0xfd92, 0xfdc7 },
	{ 0xfdf0, 0xfdf9 },
	{ 0xfe00, 0xfe0f },
	{ 0xfe20, 0xfe2f },
	{ 0xfe33, 0xfe34 },
	{ 0xfe4d, 0xfe4f },
	{ 0xfe71, 0xfe71 },
	{ 0xfe73, 0xfe73 },
	{ 0xfe77, 0xfe77 },
	{ 0xfe79, 0xfe79 },
	{ 0xfe7b, 0xfe7b },
	{ 0xfe7d, 0xfe7d },
	{ 0xfe7f, 0xfefc },
	{ 0xff10, 0xff19 },
	{ 0xff21, 0xff3a },
	{ 0xff3f, 0xff3f },
	{ 0xff41, 0xff5a },
	{ 0xff65, 0xffbe },
	{ 0xffc2, 0xffc7 },
	{ 0xffca, 0xffcf },
	{ 0xffd2, 0xffd7 },
	{ 0xffda, 0xffdc },
	{ 0x10000, 0x1000b },
	{ 0x1000d, 0x10026 },
	{ 0x10028, 0x1003a },
	{ 0x1003c, 0x1003d },
	{ 0x1003f, 0x1004d },
	{ 0x10050, 0x1005d },
	{ 0x10080, 0x100fa },
	{ 0x10140, 0x10174 },
	{ 0x101fd, 0x101fd },
	{ 0x10280, 0x1029c },
	{ 0x102a0, 0x102d0 },
	{ 0x102e0, 0x102e0 },
	{ 0x10300, 0x1031f },
	{ 0x1032d, 0x1034a },
	{ 0x10350, 0x1037a },
	{ 0x10380, 0x1039d },
	{ 0x103a0, 0x103c3 },
	{ 0x103c8, 0x103cf },
	{ 0x103d1, 0x103d5 },
	{ 0x10400, 0x1049d },
	{ 0x104a0, 0x104a9 },
	{ 0x104b0, 0x104d3 },
	{ 0x104d8, 0x104fb },
	{ 0x10500, 0x10527 },
	{ 0x10530, 0x10563 },
	{ 0x10570, 0x1057a },
	{ 0x1057c, 0x1058a },
	{ 0x1058c, 0x10592 },
	{ 0x10594, 0x10595 },
	{ 0x10597, 0x105a1 },
	{ 0x105a3, 0x105b1 },
	{ 0x105b3, 0x105b9 },
	{ 0x105bb, 0x105bc },
	{ 0x105c0, 0x105f3 },
	{ 0x10600, 0x10736 },
	{ 0x10740, 0x10755 },
	{ 0x10760, 0x10767 },
	{ 0x10780, 0x10785 },
	{ 0x10787, 0x107b0 },
	{ 0x107b2, 0x107ba },
	{ 0x10800, 0x10805 },
	{ 0x10808, 0x10808 },
	{ 0x1080a, 0x10835 },
	{ 0x10837, 0x10838 },
	{ 0x1083c, 0x1083c },
	{ 0x1083f, 0x10855 },
	{ 0x10860, 0x10876 },
	{ 0x10880, 0x1089e },
	{ 0x108e0, 0x108f2 },
	{ 0x108f4, 0x108f5 },
	{ 0x10900, 0x10915 },
	{ 0x10920, 0x10939 },
	{ 0x10940, 0x10959 },
	{ 0x10980, 0x109b7 },
	{ 0x109be, 0x109bf },
	{ 0x10a00, 0x10a03 },
	{ 0x10a05, 0x10a06 },
	{ 0x10a0c, 0x10a13 },
	{ 0x10a15, 0x10a17 },
	{ 0x10a19, 0x10a35 },
	{ 0x10a38, 0x10a3a },
	{ 0x10a3f, 0x10a3f },
	{ 0x10a60, 0x10a7c },
	{ 0x10a80, 0x10a9c },
	{ 0x10ac0, 0x10ac7 },
	{ 0x10ac9, 0x10ae6 },
	{ 0x10b00, 0x10b35 },
	{ 0x10b40, 0x10b55 },
	{ 0x10b60, 0x10b72 },
	{ 0x10b80, 0x10b91 },
	{ 0x10c00, 0x10c48 },
	{ 0x10c80, 0x10cb2 },
	{ 0x10cc0, 0x10cf2 },
	{ 0x10d00, 0x10d27 },
	{ 0x10d30, 0x10d39 },
	{ 0x10d40, 0x10d65 },
	{ 0x10d69, 0x10d6d },
	{ 0x10d6f, 0x10d85 },
	{ 0x10e80, 0x10ea9 },
	{ 0x10eab, 0x10eac },
	{ 0x10eb0, 0x10eb1 },
	{ 0x10ec2, 0x10ec7 },
	{ 0x10efa, 0x10f1c },
	{ 0x10f27, 0x10f27 },
	{ 0x10f30, 0x10f50 },
	{ 0x10f70, 0x10f85 },
	{ 0x10fb0, 0x10fc4 },
	{ 0x10fe0, 0x10ff6 },
	{ 0x11000, 0x11046 },
	{ 0x11066, 0x11075 },
	{ 0x1107f, 0x110ba },
	{ 0x110c2, 0x110c2 },
	{ 0x110d0, 0x110e8 },
	{ 0x110f0, 0x110f9 },
	{ 0x11100, 0x11134 },
	{ 0x11136, 0x1113f },
	{ 0x11144, 0x11147 },
	{ 0x11150, 0x11173 },
	{ 0x11176, 0x11176 },
	{ 0x11180, 0x111c4 },
	{ 0x111c9, 0x111cc },
	{ 0x111ce, 0x111da },
	{ 0x111dc, 0x111dc },
	{ 0x11200, 0x11211 },
	{ 0x11213, 0x11237 },
	{ 0x1123e, 0x11241 },
	{ 0x11280, 0x11286 },
	{ 0x11288, 0x11288 },
	{ 0x1128a, 0x1128d },
	{ 0x1128f, 0x1129d },
	{ 0x1129f, 0x112a8 },
	{ 0x112b0, 0x112ea },
	{ 0x112f0, 0x112f9 },
	{ 0x11300, 0x11303 },
	{ 0x11305, 0x1130c },
	{ 0x1130f, 0x11310 },
	{ 0x11313, 0x11328 },
	{ 0x1132a, 0x11330 },
	{ 0x11332, 0x11333 },
	{ 0x11335, 0x11339 },
	{ 0x1133b, 0x11344 },
	{ 0x11347, 0x11348 },
	{ 0x1134b, 0x1134d },
	{ 0x11350, 0x11350 },
	{ 0x11357, 0x11357 },
	{ 0x1135d, 0x11363 },
	{ 0x11366, 0x1136c },
	{ 0x11370, 0x11374 },
	{ 0x11380, 0x11389 },
	{ 0x1138b, 0x1138b },
	{ 0x1138e, 0x1138e },
	{ 0x11390, 0x113b5 },
	{ 0x113b7, 0x113c0 },
	{ 0x113c2, 0x113c2 },
	{ 0x113c5, 0x113c5 },
	{ 0x113c7, 0x113ca },
	{ 0x113cc, 0x113d3 },
	{ 0x113e1, 0x113e2 },
	{ 0x11400, 0x1144a },
	{ 0x11450, 0x11459 },
	{ 0x1145e, 0x11461 },
	{ 0x11480, 0x114c5 },
	{ 0x114c7, 0x114c7 },
	{ 0x114d0, 0x114d9 },
	{ 0x11580, 0x115b5 },
	{ 0x115b8, 0x115c0 },
	{ 0x115d8, 0x115dd },
	{ 0x11600, 0x11640 },
	{ 0x11644, 0x11644 },
	{ 0x11650, 0x11659 },
	{ 0x11680, 0x116b8 },
	{ 0x116c0, 0x116c9 },
	{ 0x116d0, 0x116e3 },
	{ 0x11700, 0x1171a },
	{ 0x1171d, 0x1172b },
	{ 0x11730, 0x11739 },
	{ 0x11740, 0x11746 },
	{ 0x11800, 0x1183a },
	{ 0x118a0, 0x118e9 },
	{ 0x118ff, 0x11906 },
	{ 0x11909, 0x11909 },
	{ 0x1190c, 0x11913 },
	{ 0x11915, 0x11916 },
	{ 0x11918, 0x11935 },
	{ 0x11937, 0x11938 },
	{ 0x1193b, 0x11943 },
	{ 0x11950, 0x11959 },
	{ 0x119a0, 0x119a7 },
	{ 0x119aa, 0x119d7 },
	{ 0x119da, 0x119e1 },
	{ 0x119e3, 0x119e4 },
	{ 0x11a00, 0x11a3e },
	{ 0x11a47, 0x11a47 },
	{ 0x11a50, 0x11a99 },
	{ 0x11a9d, 0x11a9d },
	{ 0x11ab0, 0x11af8 },
	{ 0x11b60, 0x11b67 },
	{ 0x11bc0, 0x11be0 },
	{ 0x11bf0, 0x11bf9 },
	{ 0x11c00, 0x11c08 },
	{ 0x11c0a, 0x11c36 },
	{ 0x11c38, 0x11c40 },
	{ 0x11c50, 0x11c59 },
	{ 0x11c72, 0x11c8f },
	{ 0x11c92, 0x11ca7 },
	{ 0x11ca9, 0x11cb6 },
	{ 0x11d00, 0x11d06 },
	{ 0x11d08, 0x11d09 },
	{ 0x11d0b, 0x11d36 },
	{ 0x11d3a, 0x11d3a },
	{ 0x11d3c, 0x11d3d },
	{ 0x11d3f, 0x11d47 },
	{ 0x11d50, 0x11d59 },
	{ 0x11d60, 0x11d65 },
	{ 0x11d67, 0x11d68 },
	{ 0x11d6a, 0x11d8e },
	{ 0x11d90, 0x11d91 },
	{ 0x11d93, 0x11d98 },
	{ 0x11da0, 0x11da9 },
	{ 0x11db0, 0x11ddb },
	{ 0x11de0, 0x11de9 },
	{ 0x11ee0, 0x11ef6 },
	{ 0x11f00, 0x11f10 },
	{ 0x11f12, 0x11f3a },
	{ 0x11f3e, 0x11f42 },
	{ 0x11f50, 0x11f5a },
	{ 0x11fb0, 0x11fb0 },
	{ 0x12000, 0x12399 },
	{ 0x12400, 0x1246e },
	{ 0x12480, 0x12543 },
	{ 0x12f90, 0x12ff0 },
	{ 0x13000, 0x1342f },
	{ 0x13440, 0x13455 },
	{ 0x13460, 0x143fa },
	{ 0x14400, 0x14646 },
	{ 0x16100, 0x16139 },
	{ 0x16800, 0x16a38 },
	{ 0x16a40, 0x16a5e },
	{ 0x16a60, 0x16a69 },
	{ 0x16a70, 0x16abe },
	{ 0x16ac0, 0x16ac9 },
	{ 0x16ad0, 0x16aed },
	{ 0x16af0, 0x16af4 },
	{ 0x16b00, 0x16b36 },
	{ 0x16b40, 0x16b43 },
	{ 0x16b50, 0x16b59 },
	{ 0x16b63, 0x16b77 },
	{ 0x16b7d, 0x16b8f },
	{ 0x16d40, 0x16d6c },
	{ 0x16d70, 0x16d79 },
	{ 0x16e40, 0x16e7f },
	{ 0x16ea0, 0x16eb8 },
	{ 0x16ebb, 0x16ed3 },
	{ 0x16f00, 0x16f4a },
	{ 0x16f4f, 0x16f87 },
	{ 0x16f8f, 0x16f9f },
	{ 0x16fe0, 0x16fe1 },
	{ 0x16fe3, 0x16fe4 },
	{ 0x16ff0, 0x16ff6 },
	{ 0x17000, 0x18cd5 },
	{ 0x18cff, 0x18d1e },
	{ 0x18d80, 0x18df2 },
	{ 0x1aff0, 0x1aff3 },
	{ 0x1aff5, 0x1affb },
	{ 0x1affd, 0x1affe },
	{ 0x1b000, 0x1b122 },
	{ 0x1b132, 0x1b132 },
	{ 0x1b150, 0x1b152 },
	{ 0x1b155, 0x1b155 },
	{ 0x1b164, 0x1b167 },
	{ 0x1b170, 0x1b2fb },
	{ 0x1bc00, 0x1bc6a },
	{ 0x1bc70, 0x1bc7c },
	{ 0x1bc80, 0x1bc88 },
	{ 0x1bc90, 0x1bc99 },
	{ 0x1bc9d, 0x1bc9e },
	{ 0x1ccf0, 0x1ccf9 },
	{ 0x1cf00, 0x1cf2d },
	{ 0x1cf30, 0x1cf46 },
	{ 0x1d165, 0x1d169 },
	{ 0x1d16d, 0x1d172 },
	{ 0x1d17b, 0x1d182 },
	{ 0x1d185, 0x1d18b },
	{ 0x1d1aa, 0x1d1ad },
	{ 0x1d242, 0x1d244 },
	{ 0x1d400, 0x1d454 },
	{ 0x1d456, 0x1d49c },
	{ 0x1d49e, 0x1d49f },
	{ 0x1d4a2, 0x1d4a2 },
	{ 0x1d4a5, 0x1d4a6 },
	{ 0x1d4a9, 0x1d4ac },
	{ 0x1d4ae, 0x1d4b9 },
	{ 0x1d4bb, 0x1d4bb },
	{ 0x1d4bd, 0x1d4c3 },
	{ 0x1d4c5, 0x1d505 },
	{ 0x1d507, 0x1d50a },
	{ 0x1d50d, 0x1d514 },
	{ 0x1d516, 0x1d51c },
	{ 0x1d51e, 0x1d539 },
	{ 0x1d53b, 0x1d53e },
	{ 0x1d540, 0x1d544 },
	{ 0x1d546, 0x1d546 },
	{ 0x1d54a, 0x1d550 },
	{ 0x1d552, 0x1d6a5 },
	{ 0x1d6a8, 0x1d6c0 },
	{ 0x1d6c2, 0x1d6da },
	{ 0x1d6dc, 0x1d6fa },
	{ 0x1d6fc, 0x1d714 },
	{ 0x1d716, 0x1d734 },
	{ 0x1d736, 0x1d74e },
	{ 0x1d750, 0x1d76e },
	{ 0x1d770, 0x1d788 },
	{ 0x1d78a, 0x1d7a8 },
	{ 0x1d7aa, 0x1d7c2 },
	{ 0x1d7c4, 0x1d7cb },
	{ 0x1d7ce, 0x1d7ff },
	{ 0x1da00, 0x1da36 },
	{ 0x1da3b, 0x1da6c },
	{ 0x1da75, 0x1da75 },
	{ 0x1da84, 0x1da84 },
	{ 0x1da9b, 0x1da9f },
	{ 0x1daa1, 0x1daaf },
	{ 0x1df00, 0x1df1e },
	{ 0x1df25, 0x1df2a },
	{ 0x1e000, 0x1e006 },
	{ 0x1e008, 0x1e018 },
	{ 0x1e01b, 0x1e021 },
	{ 0x1e023, 0x1e024 },
	{ 0x1e026, 0x1e02a },
	{ 0x1e030, 0x1e06d },
	{ 0x1e08f, 0x1e08f },
	{ 0x1e100, 0x1e12c },
	{ 0x1e130, 0x1e13d },
	{ 0x1e140, 0x1e149 },
	{ 0x1e14e, 0x1e14e },
	{ 0x1e290, 0x1e2ae },
	{ 0x1e2c0, 0x1e2f9 },
	{ 0x1e4d0, 0x1e4f9 },
	{ 0x1e5d0, 0x1e5fa },
	{ 0x1e6c0, 0x1e6de },
	{ 0x1e6e0, 0x1e6f5 },
	{ 0x1e6fe, 0x1e6ff },
	{ 0x1e7e0, 0x1e7e6 },
	{ 0x1e7e8, 0x1e7eb },
	{ 0x1e7ed, 0x1e7ee },
	{ 0x1e7f0, 0x1e7fe },
	{ 0x1e800, 0x1e8c4 },
	{ 0x1e8d0, 0x1e8d6 },
	{ 0x1e900, 0x1e94b },
	{ 0x1e950, 0x1e959 },
	{ 0x1ee00, 0x1ee03 },
	{ 0x1ee05, 0x1ee1f },
	{ 0x1ee21, 0x1ee22 },
	{ 0x1ee24, 0x1ee24 },
	{ 0x1ee27, 0x1ee27 },
	{ 0x1ee29, 0x1ee32 },
	{ 0x1ee34, 0x1ee37 },
	{ 0x1ee39, 0x1ee39 },
	{ 0x1ee3b, 0x1ee3b },
	{ 0x1ee42, 0x1ee42 },
	{ 0x1ee47, 0x1ee47 },
	{ 0x1ee49, 0x1ee49 },
	{ 0x1ee4b, 0x1ee4b },
	{ 0x1ee4d, 0x1ee4f },
	{ 0x1ee51, 0x1ee52 },
	{ 0x1ee54, 0x1ee54 },
	{ 0x1ee57, 0x1ee57 },
	{ 0x1ee59, 0x1ee59 },
	{ 0x1ee5b, 0x1ee5b },
	{ 0x1ee5d, 0x1ee5d },
	{ 0x1ee5f, 0x1ee5f },
	{ 0x1ee61, 0x1ee62 },
	{ 0x1ee64, 0x1ee64 },
	{ 0x1ee67, 0x1ee6a },
	{ 0x1ee6c, 0x1ee72 },
	{ 0x1ee74, 0x1ee77 },
	{ 0x1ee79, 0x1ee7c },
	{ 0x1ee7e, 0x1ee7e },
	{ 0x1ee80, 0x1ee89 },
	{ 0x1ee8b, 0x1ee9b },
	{ 0x1eea1, 0x1eea3 },
	{ 0x1eea5, 0x1eea9 },
	{ 0x1eeab, 0x1eebb },
	{ 0x1fbf0, 0x1fbf9 },
	{ 0x20000, 0x2a6df },
	{ 0x2a700, 0x2b81d },
	{ 0x2b820, 0x2cead },
	{ 0x2ceb0, 0x2ebe0 },
	{ 0x2ebf0, 0x2ee5d },
	{ 0x2f800, 0x2fa1d },
	{ 0x30000, 0x3134a },
	{ 0x31350, 0x33479 },
	{ 0xe0100, 0xe01ef },
};

const int uppercase_letter_size = 660;
const CharRange uppercase_letter[uppercase_letter_size] = {
	{ 0x41, 0x5a },
	{ 0xc0, 0xd6 },
	{ 0xd8, 0xde },
	{ 0x100, 0x100 },
	{ 0x102, 0x102 },
	{ 0x104, 0x104 },
	{ 0x106, 0x106 },
	{ 0x108, 0x108 },
	{ 0x10a, 0x10a },
	{ 0x10c, 0x10c },
	{ 0x10e, 0x10e },
	{ 0x110, 0x110 },
	{ 0x112, 0x112 },
	{ 0x114, 0x114 },
	{ 0x116, 0x116 },
	{ 0x118, 0x118 },
	{ 0x11a, 0x11a },
	{ 0x11c, 0x11c },
	{ 0x11e, 0x11e },
	{ 0x120, 0x120 },
	{ 0x122, 0x122 },
	{ 0x124, 0x124 },
	{ 0x126, 0x126 },
	{ 0x128, 0x128 },
	{ 0x12a, 0x12a },
	{ 0x12c, 0x12c },
	{ 0x12e, 0x12e },
	{ 0x130, 0x130 },
	{ 0x132, 0x132 },
	{ 0x134, 0x134 },
	{ 0x136, 0x136 },
	{ 0x139, 0x139 },
	{ 0x13b, 0x13b },
	{ 0x13d, 0x13d },
	{ 0x13f, 0x13f },
	{ 0x141, 0x141 },
	{ 0x143, 0x143 },
	{ 0x145, 0x145 },
	{ 0x147, 0x147 },
	{ 0x14a, 0x14a },
	{ 0x14c, 0x14c },
	{ 0x14e, 0x14e },
	{ 0x150, 0x150 },
	{ 0x152, 0x152 },
	{ 0x154, 0x154 },
	{ 0x156, 0x156 },
	{ 0x158, 0x158 },
	{ 0x15a, 0x15a },
	{ 0x15c, 0x15c },
	{ 0x15e, 0x15e },
	{ 0x160, 0x160 },
	{ 0x162, 0x162 },
	{ 0x164, 0x164 },
	{ 0x166, 0x166 },
	{ 0x168, 0x168 },
	{ 0x16a, 0x16a },
	{ 0x16c, 0x16c },
	{ 0x16e, 0x16e },
	{ 0x170, 0x170 },
	{ 0x172, 0x172 },
	{ 0x174, 0x174 },
	{ 0x176, 0x176 },
	{ 0x178, 0x179 },
	{ 0x17b, 0x17b },
	{ 0x17d, 0x17d },
	{ 0x181, 0x182 },
	{ 0x184, 0x184 },
	{ 0x186, 0x187 },
	{ 0x189, 0x18b },
	{ 0x18e, 0x191 },
	{ 0x193, 0x194 },
	{ 0x196, 0x198 },
	{ 0x19c, 0x19d },
	{ 0x19f, 0x1a0 },
	{ 0x1a2, 0x1a2 },
	{ 0x1a4, 0x1a4 },
	{ 0x1a6, 0x1a7 },
	{ 0x1a9, 0x1a9 },
	{ 0x1ac, 0x1ac },
	{ 0x1ae, 0x1af },
	{ 0x1b1, 0x1b3 },
	{ 0x1b5, 0x1b5 },
	{ 0x1b7, 0x1b8 },
	{ 0x1bc, 0x1bc },
	{ 0x1c4, 0x1c4 },
	{ 0x1c7, 0x1c7 },
	{ 0x1ca, 0x1ca },
	{ 0x1cd, 0x1cd },
	{ 0x1cf, 0x1cf },
	{ 0x1d1, 0x1d1 },
	{ 0x1d3, 0x1d3 },
	{ 0x1d5, 0x1d5 },
	{ 0x1d7, 0x1d7 },
	{ 0x1d9, 0x1d9 },
	{ 0x1db, 0x1db },
	{ 0x1de, 0x1de },
	{ 0x1e0, 0x1e0 },
	{ 0x1e2, 0x1e2 },
	{ 0x1e4, 0x1e4 },
	{ 0x1e6, 0x1e6 },
	{ 0x1e8, 0x1e8 },
	{ 0x1ea, 0x1ea },
	{ 0x1ec, 0x1ec },
	{ 0x1ee, 0x1ee },
	{ 0x1f1, 0x1f1 },
	{ 0x1f4, 0x1f4 },
	{ 0x1f6, 0x1f8 },
	{ 0x1fa, 0x1fa },
	{ 0x1fc, 0x1fc },
	{ 0x1fe, 0x1fe },
	{ 0x200, 0x200 },
	{ 0x202, 0x202 },
	{ 0x204, 0x204 },
	{ 0x206, 0x206 },
	{ 0x208, 0x208 },
	{ 0x20a, 0x20a },
	{ 0x20c, 0x20c },
	{ 0x20e, 0x20e },
	{ 0x210, 0x210 },
	{ 0x212, 0x212 },
	{ 0x214, 0x214 },
	{ 0x216, 0x216 },
	{ 0x218, 0x218 },
	{ 0x21a, 0x21a },
	{ 0x21c, 0x21c },
	{ 0x21e, 0x21e },
	{ 0x220, 0x220 },
	{ 0x222, 0x222 },
	{ 0x224, 0x224 },
	{ 0x226, 0x226 },
	{ 0x228, 0x228 },
	{ 0x22a, 0x22a },
	{ 0x22c, 0x22c },
	{ 0x22e, 0x22e },
	{ 0x230, 0x230 },
	{ 0x232, 0x232 },
	{ 0x23a, 0x23b },
	{ 0x23d, 0x23e },
	{ 0x241, 0x241 },
	{ 0x243, 0x246 },
	{ 0x248, 0x248 },
	{ 0x24a, 0x24a },
	{ 0x24c, 0x24c },
	{ 0x24e, 0x24e },
	{ 0x370, 0x370 },
	{ 0x372, 0x372 },
	{ 0x376, 0x376 },
	{ 0x37f, 0x37f },
	{ 0x386, 0x386 },
	{ 0x388, 0x38a },
	{ 0x38c, 0x38c },
	{ 0x38e, 0x38f },
	{ 0x391, 0x3a1 },
	{ 0x3a3, 0x3ab },
	{ 0x3cf, 0x3cf },
	{ 0x3d2, 0x3d4 },
	{ 0x3d8, 0x3d8 },
	{ 0x3da, 0x3da },
	{ 0x3dc, 0x3dc },
	{ 0x3de, 0x3de },
	{ 0x3e0, 0x3e0 },
	{ 0x3e2, 0x3e2 },
	{ 0x3e4, 0x3e4 },
	{ 0x3e6, 0x3e6 },
	{ 0x3e8, 0x3e8 },
	{ 0x3ea, 0x3ea },
	{ 0x3ec, 0x3ec },
	{ 0x3ee, 0x3ee },
	{ 0x3f4, 0x3f4 },
	{ 0x3f7, 0x3f7 },
	{ 0x3f9, 0x3fa },
	{ 0x3fd, 0x42f },
	{ 0x460, 0x460 },
	{ 0x462, 0x462 },
	{ 0x464, 0x464 },
	{ 0x466, 0x466 },
	{ 0x468, 0x468 },
	{ 0x46a, 0x46a },
	{ 0x46c, 0x46c },
	{ 0x46e, 0x46e },
	{ 0x470, 0x470 },
	{ 0x472, 0x472 },
	{ 0x474, 0x474 },
	{ 0x476, 0x476 },
	{ 0x478, 0x478 },
	{ 0x47a, 0x47a },
	{ 0x47c, 0x47c },
	{ 0x47e, 0x47e },
	{ 0x480, 0x480 },
	{ 0x48a, 0x48a },
	{ 0x48c, 0x48c },
	{ 0x48e, 0x48e },
	{ 0x490, 0x490 },
	{ 0x492, 0x492 },
	{ 0x494, 0x494 },
	{ 0x496, 0x496 },
	{ 0x498, 0x498 },
	{ 0x49a, 0x49a },
	{ 0x49c, 0x49c },
	{ 0x49e, 0x49e },
	{ 0x4a0, 0x4a0 },
	{ 0x4a2, 0x4a2 },
	{ 0x4a4, 0x4a4 },
	{ 0x4a6, 0x4a6 },
	{ 0x4a8, 0x4a8 },
	{ 0x4aa, 0x4aa },
	{ 0x4ac, 0x4ac },
	{ 0x4ae, 0x4ae },
	{ 0x4b0, 0x4b0 },
	{ 0x4b2, 0x4b2 },
	{ 0x4b4, 0x4b4 },
	{ 0x4b6, 0x4b6 },
	{ 0x4b8, 0x4b8 },
	{ 0x4ba, 0x4ba },
	{ 0x4bc, 0x4bc },
	{ 0x4be, 0x4be },
	{ 0x4c0, 0x4c1 },
	{ 0x4c3, 0x4c3 },
	{ 0x4c5, 0x4c5 },
	{ 0x4c7, 0x4c7 },
	{ 0x4c9, 0x4c9 },
	{ 0x4cb, 0x4cb },
	{ 0x4cd, 0x4cd },
	{ 0x4d0, 0x4d0 },
	{ 0x4d2, 0x4d2 },
	{ 0x4d4, 0x4d4 },
	{ 0x4d6, 0x4d6 },
	{ 0x4d8, 0x4d8 },
	{ 0x4da, 0x4da },
	{ 0x4dc, 0x4dc },
	{ 0x4de, 0x4de },
	{ 0x4e0, 0x4e0 },
	{ 0x4e2, 0x4e2 },
	{ 0x4e4, 0x4e4 },
	{ 0x4e6, 0x4e6 },
	{ 0x4e8, 0x4e8 },
	{ 0x4ea, 0x4ea },
	{ 0x4ec, 0x4ec },
	{ 0x4ee, 0x4ee },
	{ 0x4f0, 0x4f0 },
	{ 0x4f2, 0x4f2 },
	{ 0x4f4, 0x4f4 },
	{ 0x4f6, 0x4f6 },
	{ 0x4f8, 0x4f8 },
	{ 0x4fa, 0x4fa },
	{ 0x4fc, 0x4fc },
	{ 0x4fe, 0x4fe },
	{ 0x500, 0x500 },
	{ 0x502, 0x502 },
	{ 0x504, 0x504 },
	{ 0x506, 0x506 },
	{ 0x508, 0x508 },
	{ 0x50a, 0x50a },
	{ 0x50c, 0x50c },
	{ 0x50e, 0x50e },
	{ 0x510, 0x510 },
	{ 0x512, 0x512 },
	{ 0x514, 0x514 },
	{ 0x516, 0x516 },
	{ 0x518, 0x518 },
	{ 0x51a, 0x51a },
	{ 0x51c, 0x51c },
	{ 0x51e, 0x51e },
	{ 0x520, 0x520 },
	{ 0x522, 0x522 },
	{ 0x524, 0x524 },
	{ 0x526, 0x526 },
	{ 0x528, 0x528 },
	{ 0x52a, 0x52a },
	{ 0x52c, 0x52c },
	{ 0x52e, 0x52e },
	{ 0x531, 0x556 },
	{ 0x10a0, 0x10c5 },
	{ 0x10c7, 0x10c7 },
	{ 0x10cd, 0x10cd },
	{ 0x13a0, 0x13f5 },
	{ 0x1c89, 0x1c89 },
	{ 0x1c90, 0x1cba },
	{ 0x1cbd, 0x1cbf },
	{ 0x1e00, 0x1e00 },
	{ 0x1e02, 0x1e02 },
	{ 0x1e04, 0x1e04 },
	{ 0x1e06, 0x1e06 },
	{ 0x1e08, 0x1e08 },
	{ 0x1e0a, 0x1e0a },
	{ 0x1e0c, 0x1e0c },
	{ 0x1e0e, 0x1e0e },
	{ 0x1e10, 0x1e10 },
	{ 0x1e12, 0x1e12 },
	{ 0x1e14, 0x1e14 },
	{ 0x1e16, 0x1e16 },
	{ 0x1e18, 0x1e18 },
	{ 0x1e1a, 0x1e1a },
	{ 0x1e1c, 0x1e1c },
	{ 0x1e1e, 0x1e1e },
	{ 0x1e20, 0x1e20 },
	{ 0x1e22, 0x1e22 },
	{ 0x1e24, 0x1e24 },
	{ 0x1e26, 0x1e26 },
	{ 0x1e28, 0x1e28 },
	{ 0x1e2a, 0x1e2a },
	{ 0x1e2c, 0x1e2c },
	{ 0x1e2e, 0x1e2e },
	{ 0x1e30, 0x1e30 },
	{ 0x1e32, 0x1e32 },
	{ 0x1e34, 0x1e34 },
	{ 0x1e36, 0x1e36 },
	{ 0x1e38, 0x1e38 },
	{ 0x1e3a, 0x1e3a },
	{ 0x1e3c, 0x1e3c },
	{ 0x1e3e, 0x1e3e },
	{ 0x1e40, 0x1e40 },
	{ 0x1e42, 0x1e42 },
	{ 0x1e44, 0x1e44 },
	{ 0x1e46, 0x1e46 },
	{ 0x1e48, 0x1e48 },
	{ 0x1e4a, 0x1e4a },
	{ 0x1e4c, 0x1e4c },
	{ 0x1e4e, 0x1e4e },
	{ 0x1e50, 0x1e50 },
	{ 0x1e52, 0x1e52 },
	{ 0x1e54, 0x1e54 },
	{ 0x1e56, 0x1e56 },
	{ 0x1e58, 0x1e58 },
	{ 0x1e5a, 0x1e5a },
	{ 0x1e5c, 0x1e5c },
	{ 0x1e5e, 0x1e5e },
	{ 0x1e60, 0x1e60 },
	{ 0x1e62, 0x1e62 },
	{ 0x1e64, 0x1e64 },
	{ 0x1e66, 0x1e66 },
	{ 0x1e68, 0x1e68 },
	{ 0x1e6a, 0x1e6a },
	{ 0x1e6c, 0x1e6c },
	{ 0x1e6e, 0x1e6e },
	{ 0x1e70, 0x1e70 },
	{ 0x1e72, 0x1e72 },
	{ 0x1e74, 0x1e74 },
	{ 0x1e76, 0x1e76 },
	{ 0x1e78, 0x1e78 },
	{ 0x1e7a, 0x1e7a },
	{ 0x1e7c, 0x1e7c },
	{ 0x1e7e, 0x1e7e },
	{ 0x1e80, 0x1e80 },
	{ 0x1e82, 0x1e82 },
	{ 0x1e84, 0x1e84 },
	{ 0x1e86, 0x1e86 },
	{ 0x1e88, 0x1e88 },
	{ 0x1e8a, 0x1e8a },
	{ 0x1e8c, 0x1e8c },
	{ 0x1e8e, 0x1e8e },
	{ 0x1e90, 0x1e90 },
	{ 0x1e92, 0x1e92 },
	{ 0x1e94, 0x1e94 },
	{ 0x1e9e, 0x1e9e },
	{ 0x1ea0, 0x1ea0 },
	{ 0x1ea2, 0x1ea2 },
	{ 0x1ea4, 0x1ea4 },
	{ 0x1ea6, 0x1ea6 },
	{ 0x1ea8, 0x1ea8 },
	{ 0x1eaa, 0x1eaa },
	{ 0x1eac, 0x1eac },
	{ 0x1eae, 0x1eae },
	{ 0x1eb0, 0x1eb0 },
	{ 0x1eb2, 0x1eb2 },
	{ 0x1eb4, 0x1eb4 },
	{ 0x1eb6, 0x1eb6 },
	{ 0x1eb8, 0x1eb8 },
	{ 0x1eba, 0x1eba },
	{ 0x1ebc, 0x1ebc },
	{ 0x1ebe, 0x1ebe },
	{ 0x1ec0, 0x1ec0 },
	{ 0x1ec2, 0x1ec2 },
	{ 0x1ec4, 0x1ec4 },
	{ 0x1ec6, 0x1ec6 },
	{ 0x1ec8, 0x1ec8 },
	{ 0x1eca, 0x1eca },
	{ 0x1ecc, 0x1ecc },
	{ 0x1ece, 0x1ece },
	{ 0x1ed0, 0x1ed0 },
	{ 0x1ed2, 0x1ed2 },
	{ 0x1ed4, 0x1ed4 },
	{ 0x1ed6, 0x1ed6 },
	{ 0x1ed8, 0x1ed8 },
	{ 0x1eda, 0x1eda },
	{ 0x1edc, 0x1edc },
	{ 0x1ede, 0x1ede },
	{ 0x1ee0, 0x1ee0 },
	{ 0x1ee2, 0x1ee2 },
	{ 0x1ee4, 0x1ee4 },
	{ 0x1ee6, 0x1ee6 },
	{ 0x1ee8, 0x1ee8 },
	{ 0x1eea, 0x1eea },
	{ 0x1eec, 0x1eec },
	{ 0x1eee, 0x1eee },
	{ 0x1ef0, 0x1ef0 },
	{ 0x1ef2, 0x1ef2 },
	{ 0x1ef4, 0x1ef4 },
	{ 0x1ef6, 0x1ef6 },
	{ 0x1ef8, 0x1ef8 },
	{ 0x1efa, 0x1efa },
	{ 0x1efc, 0x1efc },
	{ 0x1efe, 0x1efe },
	{ 0x1f08, 0x1f0f },
	{ 0x1f18, 0x1f1d },
	{ 0x1f28, 0x1f2f },
	{ 0x1f38, 0x1f3f },
	{ 0x1f48, 0x1f4d },
	{ 0x1f59, 0x1f59 },
	{ 0x1f5b, 0x1f5b },
	{ 0x1f5d, 0x1f5d },
	{ 0x1f5f, 0x1f5f },
	{ 0x1f68, 0x1f6f },
	{ 0x1fb8, 0x1fbb },
	{ 0x1fc8, 0x1fcb },
	{ 0x1fd8, 0x1fdb },
	{ 0x1fe8, 0x1fec },
	{ 0x1ff8, 0x1ffb },
	{ 0x2102, 0x2102 },
	{ 0x2107, 0x2107 },
	{ 0x210b, 0x210d },
	{ 0x2110, 0x2112 },
	{ 0x2115, 0x2115 },
	{ 0x2119, 0x211d },
	{ 0x2124, 0x2124 },
	{ 0x2126, 0x2126 },
	{ 0x2128, 0x2128 },
	{ 0x212a, 0x212d },
	{ 0x2130, 0x2133 },
	{ 0x213e, 0x213f },
	{ 0x2145, 0x2145 },
	{ 0x2160, 0x216f },
	{ 0x2183, 0x2183 },
	{ 0x24b6, 0x24cf },
	{ 0x2c00, 0x2c2f },
	{ 0x2c60, 0x2c60 },
	{ 0x2c62, 0x2c64 },
	{ 0x2c67, 0x2c67 },
	{ 0x2c69, 0x2c69 },
	{ 0x2c6b, 0x2c6b },
	{ 0x2c6d, 0x2c70 },
	{ 0x2c72, 0x2c72 },
	{ 0x2c75, 0x2c75 },
	{ 0x2c7e, 0x2c80 },
	{ 0x2c82, 0x2c82 },
	{ 0x2c84, 0x2c84 },
	{ 0x2c86, 0x2c86 },
	{ 0x2c88, 0x2c88 },
	{ 0x2c8a, 0x2c8a },
	{ 0x2c8c, 0x2c8c },
	{ 0x2c8e, 0x2c8e },
	{ 0x2c90, 0x2c90 },
	{ 0x2c92, 0x2c92 },
	{ 0x2c94, 0x2c94 },
	{ 0x2c96, 0x2c96 },
	{ 0x2c98, 0x2c98 },
	{ 0x2c9a, 0x2c9a },
	{ 0x2c9c, 0x2c9c },
	{ 0x2c9e, 0x2c9e },
	{ 0x2ca0, 0x2ca0 },
	{ 0x2ca2, 0x2ca2 },
	{ 0x2ca4, 0x2ca4 },
	{ 0x2ca6, 0x2ca6 },
	{ 0x2ca8, 0x2ca8 },
	{ 0x2caa, 0x2caa },
	{ 0x2cac, 0x2cac },
	{ 0x2cae, 0x2cae },
	{ 0x2cb0, 0x2cb0 },
	{ 0x2cb2, 0x2cb2 },
	{ 0x2cb4, 0x2cb4 },
	{ 0x2cb6, 0x2cb6 },
	{ 0x2cb8, 0x2cb8 },
	{ 0x2cba, 0x2cba },
	{ 0x2cbc, 0x2cbc },
	{ 0x2cbe, 0x2cbe },
	{ 0x2cc0, 0x2cc0 },
	{ 0x2cc2, 0x2cc2 },
	{ 0x2cc4, 0x2cc4 },
	{ 0x2cc6, 0x2cc6 },
	{ 0x2cc8, 0x2cc8 },
	{ 0x2cca, 0x2cca },
	{ 0x2ccc, 0x2ccc },
	{ 0x2cce, 0x2cce },
	{ 0x2cd0, 0x2cd0 },
	{ 0x2cd2, 0x2cd2 },
	{ 0x2cd4, 0x2cd4 },
	{ 0x2cd6, 0x2cd6 },
	{ 0x2cd8, 0x2cd8 },
	{ 0x2cda, 0x2cda },
	{ 0x2cdc, 0x2cdc },
	{ 0x2cde, 0x2cde },
	{ 0x2ce0, 0x2ce0 },
	{ 0x2ce2, 0x2ce2 },
	{ 0x2ceb, 0x2ceb },
	{ 0x2ced, 0x2ced },
	{ 0x2cf2, 0x2cf2 },
	{ 0xa640, 0xa640 },
	{ 0xa642, 0xa642 },
	{ 0xa644, 0xa644 },
	{ 0xa646, 0xa646 },
	{ 0xa648, 0xa648 },
	{ 0xa64a, 0xa64a },
	{ 0xa64c, 0xa64c },
	{ 0xa64e, 0xa64e },
	{ 0xa650, 0xa650 },
	{ 0xa652, 0xa652 },
	{ 0xa654, 0xa654 },
	{ 0xa656, 0xa656 },
	{ 0xa658, 0xa658 },
	{ 0xa65a, 0xa65a },
	{ 0xa65c, 0xa65c },
	{ 0xa65e, 0xa65e },
	{ 0xa660, 0xa660 },
	{ 0xa662, 0xa662 },
	{ 0xa664, 0xa664 },
	{ 0xa666, 0xa666 },
	{ 0xa668, 0xa668 },
	{ 0xa66a, 0xa66a },
	{ 0xa66c, 0xa66c },
	{ 0xa680, 0xa680 },
	{ 0xa682, 0xa682 },
	{ 0xa684, 0xa684 },
	{ 0xa686, 0xa686 },
	{ 0xa688, 0xa688 },
	{ 0xa68a, 0xa68a },
	{ 0xa68c, 0xa68c },
	{ 0xa68e, 0xa68e },
	{ 0xa690, 0xa690 },
	{ 0xa692, 0xa692 },
	{ 0xa694, 0xa694 },
	{ 0xa696, 0xa696 },
	{ 0xa698, 0xa698 },
	{ 0xa69a, 0xa69a },
	{ 0xa722, 0xa722 },
	{ 0xa724, 0xa724 },
	{ 0xa726, 0xa726 },
	{ 0xa728, 0xa728 },
	{ 0xa72a, 0xa72a },
	{ 0xa72c, 0xa72c },
	{ 0xa72e, 0xa72e },
	{ 0xa732, 0xa732 },
	{ 0xa734, 0xa734 },
	{ 0xa736, 0xa736 },
	{ 0xa738, 0xa738 },
	{ 0xa73a, 0xa73a },
	{ 0xa73c, 0xa73c },
	{ 0xa73e, 0xa73e },
	{ 0xa740, 0xa740 },
	{ 0xa742, 0xa742 },
	{ 0xa744, 0xa744 },
	{ 0xa746, 0xa746 },
	{ 0xa748, 0xa748 },
	{ 0xa74a, 0xa74a },
	{ 0xa74c, 0xa74c },
	{ 0xa74e, 0xa74e },
	{ 0xa750, 0xa750 },
	{ 0xa752, 0xa752 },
	{ 0xa754, 0xa754 },
	{ 0xa756, 0xa756 },
	{ 0xa758, 0xa758 },
	{ 0xa75a, 0xa75a },
	{ 0xa75c, 0xa75c },
	{ 0xa75e, 0xa75e },
	{ 0xa760, 0xa760 },
	{ 0xa762, 0xa762 },
	{ 0xa764, 0xa764 },
	{ 0xa766, 0xa766 },
	{ 0xa768, 0xa768 },
	{ 0xa76a, 0xa76a },
	{ 0xa76c, 0xa76c },
	{ 0xa76e, 0xa76e },
	{ 0xa779, 0xa779 },
	{ 0xa77b, 0xa77b },
	{ 0xa77d, 0xa77e },
	{ 0xa780, 0xa780 },
	{ 0xa782, 0xa782 },
	{ 0xa784, 0xa784 },
	{ 0xa786, 0xa786 },
	{ 0xa78b, 0xa78b },
	{ 0xa78d, 0xa78d },
	{ 0xa790, 0xa790 },
	{ 0xa792, 0xa792 },
	{ 0xa796, 0xa796 },
	{ 0xa798, 0xa798 },
	{ 0xa79a, 0xa79a },
	{ 0xa79c, 0xa79c },
	{ 0xa79e, 0xa79e },
	{ 0xa7a0, 0xa7a0 },
	{ 0xa7a2, 0xa7a2 },
	{ 0xa7a4, 0xa7a4 },
	{ 0xa7a6, 0xa7a6 },
	{ 0xa7a8, 0xa7a8 },
	{ 0xa7aa, 0xa7ae },
	{ 0xa7b0, 0xa7b4 },
	{ 0xa7b6, 0xa7b6 },
	{ 0xa7b8, 0xa7b8 },
	{ 0xa7ba, 0xa7ba },
	{ 0xa7bc, 0xa7bc },
	{ 0xa7be, 0xa7be },
	{ 0xa7c0, 0xa7c0 },
	{ 0xa7c2, 0xa7c2 },
	{ 0xa7c4, 0xa7c7 },
	{ 0xa7c9, 0xa7c9 },
	{ 0xa7cb, 0xa7cc },
	{ 0xa7ce, 0xa7ce },
	{ 0xa7d0, 0xa7d0 },
	{ 0xa7d2, 0xa7d2 },
	{ 0xa7d4, 0xa7d4 },
	{ 0xa7d6, 0xa7d6 },
	{ 0xa7d8, 0xa7d8 },
	{ 0xa7da, 0xa7da },
	{ 0xa7dc, 0xa7dc },
	{ 0xa7f5, 0xa7f5 },
	{ 0xff21, 0xff3a },
	{ 0x10400, 0x10427 },
	{ 0x104b0, 0x104d3 },
	{ 0x10570, 0x1057a },
	{ 0x1057c, 0x1058a },
	{ 0x1058c, 0x10592 },
	{ 0x10594, 0x10595 },
	{ 0x10c80, 0x10cb2 },
	{ 0x10d50, 0x10d65 },
	{ 0x118a0, 0x118bf },
	{ 0x16e40, 0x16e5f },
	{ 0x16ea0, 0x16eb8 },
	{ 0x1d400, 0x1d419 },
	{ 0x1d434, 0x1d44d },
	{ 0x1d468, 0x1d481 },
	{ 0x1d49c, 0x1d49c },
	{ 0x1d49e, 0x1d49f },
	{ 0x1d4a2, 0x1d4a2 },
	{ 0x1d4a5, 0x1d4a6 },
	{ 0x1d4a9, 0x1d4ac },
	{ 0x1d4ae, 0x1d4b5 },
	{ 0x1d4d0, 0x1d4e9 },
	{ 0x1d504, 0x1d505 },
	{ 0x1d507, 0x1d50a },
	{ 0x1d50d, 0x1d514 },
	{ 0x1d516, 0x1d51c },
	{ 0x1d538, 0x1d539 },
	{ 0x1d53b, 0x1d53e },
	{ 0x1d540, 0x1d544 },
	{ 0x1d546, 0x1d546 },
	{ 0x1d54a, 0x1d550 },
	{ 0x1d56c, 0x1d585 },
	{ 0x1d5a0, 0x1d5b9 },
	{ 0x1d5d4, 0x1d5ed },
	{ 0x1d608, 0x1d621 },
	{ 0x1d63c, 0x1d655 },
	{ 0x1d670, 0x1d689 },
	{ 0x1d6a8, 0x1d6c0 },
	{ 0x1d6e2, 0x1d6fa },
	{ 0x1d71c, 0x1d734 },
	{ 0x1d756, 0x1d76e },
	{ 0x1d790, 0x1d7a8 },
	{ 0x1d7ca, 0x1d7ca },
	{ 0x1e900, 0x1e921 },
	{ 0x1f130, 0x1f149 },
	{ 0x1f150, 0x1f169 },
	{ 0x1f170, 0x1f189 },
};

const int lowercase_letter_size = 677;
const CharRange lowercase_letter[lowercase_letter_size] = {
	{ 0x61, 0x7a },
	{ 0xaa, 0xaa },
	{ 0xb5, 0xb5 },
	{ 0xba, 0xba },
	{ 0xdf, 0xf6 },
	{ 0xf8, 0xff },
	{ 0x101, 0x101 },
	{ 0x103, 0x103 },
	{ 0x105, 0x105 },
	{ 0x107, 0x107 },
	{ 0x109, 0x109 },
	{ 0x10b, 0x10b },
	{ 0x10d, 0x10d },
	{ 0x10f, 0x10f },
	{ 0x111, 0x111 },
	{ 0x113, 0x113 },
	{ 0x115, 0x115 },
	{ 0x117, 0x117 },
	{ 0x119, 0x119 },
	{ 0x11b, 0x11b },
	{ 0x11d, 0x11d },
	{ 0x11f, 0x11f },
	{ 0x121, 0x121 },
	{ 0x123, 0x123 },
	{ 0x125, 0x125 },
	{ 0x127, 0x127 },
	{ 0x129, 0x129 },
	{ 0x12b, 0x12b },
	{ 0x12d, 0x12d },
	{ 0x12f, 0x12f },
	{ 0x131, 0x131 },
	{ 0x133, 0x133 },
	{ 0x135, 0x135 },
	{ 0x137, 0x138 },
	{ 0x13a, 0x13a },
	{ 0x13c, 0x13c },
	{ 0x13e, 0x13e },
	{ 0x140, 0x140 },
	{ 0x142, 0x142 },
	{ 0x144, 0x144 },
	{ 0x146, 0x146 },
	{ 0x148, 0x149 },
	{ 0x14b, 0x14b },
	{ 0x14d, 0x14d },
	{ 0x14f, 0x14f },
	{ 0x151, 0x151 },
	{ 0x153, 0x153 },
	{ 0x155, 0x155 },
	{ 0x157, 0x157 },
	{ 0x159, 0x159 },
	{ 0x15b, 0x15b },
	{ 0x15d, 0x15d },
	{ 0x15f, 0x15f },
	{ 0x161, 0x161 },
	{ 0x163, 0x163 },
	{ 0x165, 0x165 },
	{ 0x167, 0x167 },
	{ 0x169, 0x169 },
	{ 0x16b, 0x16b },
	{ 0x16d, 0x16d },
	{ 0x16f, 0x16f },
	{ 0x171, 0x171 },
	{ 0x173, 0x173 },
	{ 0x175, 0x175 },
	{ 0x177, 0x177 },
	{ 0x17a, 0x17a },
	{ 0x17c, 0x17c },
	{ 0x17e, 0x180 },
	{ 0x183, 0x183 },
	{ 0x185, 0x185 },
	{ 0x188, 0x188 },
	{ 0x18c, 0x18d },
	{ 0x192, 0x192 },
	{ 0x195, 0x195 },
	{ 0x199, 0x19b },
	{ 0x19e, 0x19e },
	{ 0x1a1, 0x1a1 },
	{ 0x1a3, 0x1a3 },
	{ 0x1a5, 0x1a5 },
	{ 0x1a8, 0x1a8 },
	{ 0x1aa, 0x1ab },
	{ 0x1ad, 0x1ad },
	{ 0x1b0, 0x1b0 },
	{ 0x1b4, 0x1b4 },
	{ 0x1b6, 0x1b6 },
	{ 0x1b9, 0x1ba },
	{ 0x1bd, 0x1bf },
	{ 0x1c6, 0x1c6 },
	{ 0x1c9, 0x1c9 },
	{ 0x1cc, 0x1cc },
	{ 0x1ce, 0x1ce },
	{ 0x1d0, 0x1d0 },
	{ 0x1d2, 0x1d2 },
	{ 0x1d4, 0x1d4 },
	{ 0x1d6, 0x1d6 },
	{ 0x1d8, 0x1d8 },
	{ 0x1da, 0x1da },
	{ 0x1dc, 0x1dd },
	{ 0x1df, 0x1df },
	{ 0x1e1, 0x1e1 },
	{ 0x1e3, 0x1e3 },
	{ 0x1e5, 0x1e5 },
	{ 0x1e7, 0x1e7 },
	{ 0x1e9, 0x1e9 },
	{ 0x1eb, 0x1eb },
	{ 0x1ed, 0x1ed },
	{ 0x1ef, 0x1f0 },
	{ 0x1f3, 0x1f3 },
	{ 0x1f5, 0x1f5 },
	{ 0x1f9, 0x1f9 },
	{ 0x1fb, 0x1fb },
	{ 0x1fd, 0x1fd },
	{ 0x1ff, 0x1ff },
	{ 0x201, 0x201 },
	{ 0x203, 0x203 },
	{ 0x205, 0x205 },
	{ 0x207, 0x207 },
	{ 0x209, 0x209 },
	{ 0x20b, 0x20b },
	{ 0x20d, 0x20d },
	{ 0x20f, 0x20f },
	{ 0x211, 0x211 },
	{ 0x213, 0x213 },
	{ 0x215, 0x215 },
	{ 0x217, 0x217 },
	{ 0x219, 0x219 },
	{ 0x21b, 0x21b },
	{ 0x21d, 0x21d },
	{ 0x21f, 0x21f },
	{ 0x221, 0x221 },
	{ 0x223, 0x223 },
	{ 0x225, 0x225 },
	{ 0x227, 0x227 },
	{ 0x229, 0x229 },
	{ 0x22b, 0x22b },
	{ 0x22d, 0x22d },
	{ 0x22f, 0x22f },
	{ 0x231, 0x231 },
	{ 0x233, 0x239 },
	{ 0x23c, 0x23c },
	{ 0x23f, 0x240 },
	{ 0x242, 0x242 },
	{ 0x247, 0x247 },
	{ 0x249, 0x249 },
	{ 0x24b, 0x24b },
	{ 0x24d, 0x24d },
	{ 0x24f, 0x293 },
	{ 0x296, 0x2b8 },
	{ 0x2c0, 0x2c1 },
	{ 0x2e0, 0x2e4 },
	{ 0x345, 0x345 },
	{ 0x371, 0x371 },
	{ 0x373, 0x373 },
	{ 0x377, 0x377 },
	{ 0x37a, 0x37d },
	{ 0x390, 0x390 },
	{ 0x3ac, 0x3ce },
	{ 0x3d0, 0x3d1 },
	{ 0x3d5, 0x3d7 },
	{ 0x3d9, 0x3d9 },
	{ 0x3db, 0x3db },
	{ 0x3dd, 0x3dd },
	{ 0x3df, 0x3df },
	{ 0x3e1, 0x3e1 },
	{ 0x3e3, 0x3e3 },
	{ 0x3e5, 0x3e5 },
	{ 0x3e7, 0x3e7 },
	{ 0x3e9, 0x3e9 },
	{ 0x3eb, 0x3eb },
	{ 0x3ed, 0x3ed },
	{ 0x3ef, 0x3f3 },
	{ 0x3f5, 0x3f5 },
	{ 0x3f8, 0x3f8 },
	{ 0x3fb, 0x3fc },
	{ 0x430, 0x45f },
	{ 0x461, 0x461 },
	{ 0x463, 0x463 },
	{ 0x465, 0x465 },
	{ 0x467, 0x467 },
	{ 0x469, 0x469 },
	{ 0x46b, 0x46b },
	{ 0x46d, 0x46d },
	{ 0x46f, 0x46f },
	{ 0x471, 0x471 },
	{ 0x473, 0x473 },
	{ 0x475, 0x475 },
	{ 0x477, 0x477 },
	{ 0x479, 0x479 },
	{ 0x47b, 0x47b },
	{ 0x47d, 0x47d },
	{ 0x47f, 0x47f },
	{ 0x481, 0x481 },
	{ 0x48b, 0x48b },
	{ 0x48d, 0x48d },
	{ 0x48f, 0x48f },
	{ 0x491, 0x491 },
	{ 0x493, 0x493 },
	{ 0x495, 0x495 },
	{ 0x497, 0x497 },
	{ 0x499, 0x499 },
	{ 0x49b, 0x49b },
	{ 0x49d, 0x49d },
	{ 0x49f, 0x49f },
	{ 0x4a1, 0x4a1 },
	{ 0x4a3, 0x4a3 },
	{ 0x4a5, 0x4a5 },
	{ 0x4a7, 0x4a7 },
	{ 0x4a9, 0x4a9 },
	{ 0x4ab, 0x4ab },
	{ 0x4ad, 0x4ad },
	{ 0x4af, 0x4af },
	{ 0x4b1, 0x4b1 },
	{ 0x4b3, 0x4b3 },
	{ 0x4b5, 0x4b5 },
	{ 0x4b7, 0x4b7 },
	{ 0x4b9, 0x4b9 },
	{ 0x4bb, 0x4bb },
	{ 0x4bd, 0x4bd },
	{ 0x4bf, 0x4bf },
	{ 0x4c2, 0x4c2 },
	{ 0x4c4, 0x4c4 },
	{ 0x4c6, 0x4c6 },
	{ 0x4c8, 0x4c8 },
	{ 0x4ca, 0x4ca },
	{ 0x4cc, 0x4cc },
	{ 0x4ce, 0x4cf },
	{ 0x4d1, 0x4d1 },
	{ 0x4d3, 0x4d3 },
	{ 0x4d5, 0x4d5 },
	{ 0x4d7, 0x4d7 },
	{ 0x4d9, 0x4d9 },
	{ 0x4db, 0x4db },
	{ 0x4dd, 0x4dd },
	{ 0x4df, 0x4df },
	{ 0x4e1, 0x4e1 },
	{ 0x4e3, 0x4e3 },
	{ 0x4e5, 0x4e5 },
	{ 0x4e7, 0x4e7 },
	{ 0x4e9, 0x4e9 },
	{ 0x4eb, 0x4eb },
	{ 0x4ed, 0x4ed },
	{ 0x4ef, 0x4ef },
	{ 0x4f1, 0x4f1 },
	{ 0x4f3, 0x4f3 },
	{ 0x4f5, 0x4f5 },
	{ 0x4f7, 0x4f7 },
	{ 0x4f9, 0x4f9 },
	{ 0x4fb, 0x4fb },
	{ 0x4fd, 0x4fd },
	{ 0x4ff, 0x4ff },
	{ 0x501, 0x501 },
	{ 0x503, 0x503 },
	{ 0x505, 0x505 },
	{ 0x507, 0x507 },
	{ 0x509, 0x509 },
	{ 0x50b, 0x50b },
	{ 0x50d, 0x50d },
	{ 0x50f, 0x50f },
	{ 0x511, 0x511 },
	{ 0x513, 0x513 },
	{ 0x515, 0x515 },
	{ 0x517, 0x517 },
	{ 0x519, 0x519 },
	{ 0x51b, 0x51b },
	{ 0x51d, 0x51d },
	{ 0x51f, 0x51f },
	{ 0x521, 0x521 },
	{ 0x523, 0x523 },
	{ 0x525, 0x525 },
	{ 0x527, 0x527 },
	{ 0x529, 0x529 },
	{ 0x52b, 0x52b },
	{ 0x52d, 0x52d },
	{ 0x52f, 0x52f },
	{ 0x560, 0x588 },
	{ 0x10d0, 0x10fa },
	{ 0x10fc, 0x10ff },
	{ 0x13f8, 0x13fd },
	{ 0x1c80, 0x1c88 },
	{ 0x1c8a, 0x1c8a },
	{ 0x1d00, 0x1dbf },
	{ 0x1e01, 0x1e01 },
	{ 0x1e03, 0x1e03 },
	{ 0x1e05, 0x1e05 },
	{ 0x1e07, 0x1e07 },
	{ 0x1e09, 0x1e09 },
	{ 0x1e0b, 0x1e0b },
	{ 0x1e0d, 0x1e0d },
	{ 0x1e0f, 0x1e0f },
	{ 0x1e11, 0x1e11 },
	{ 0x1e13, 0x1e13 },
	{ 0x1e15, 0x1e15 },
	{ 0x1e17, 0x1e17 },
	{ 0x1e19, 0x1e19 },
	{ 0x1e1b, 0x1e1b },
	{ 0x1e1d, 0x1e1d },
	{ 0x1e1f, 0x1e1f },
	{ 0x1e21, 0x1e21 },
	{ 0x1e23, 0x1e23 },
	{ 0x1e25, 0x1e25 },
	{ 0x1e27, 0x1e27 },
	{ 0x1e29, 0x1e29 },
	{ 0x1e2b, 0x1e2b },
	{ 0x1e2d, 0x1e2d },
	{ 0x1e2f, 0x1e2f },
	{ 0x1e31, 0x1e31 },
	{ 0x1e33, 0x1e33 },
	{ 0x1e35, 0x1e35 },
	{ 0x1e37, 0x1e37 },
	{ 0x1e39, 0x1e39 },
	{ 0x1e3b, 0x1e3b },
	{ 0x1e3d, 0x1e3d },
	{ 0x1e3f, 0x1e3f },
	{ 0x1e41, 0x1e41 },
	{ 0x1e43, 0x1e43 },
	{ 0x1e45, 0x1e45 },
	{ 0x1e47, 0x1e47 },
	{ 0x1e49, 0x1e49 },
	{ 0x1e4b, 0x1e4b },
	{ 0x1e4d, 0x1e4d },
	{ 0x1e4f, 0x1e4f },
	{ 0x1e51, 0x1e51 },
	{ 0x1e53, 0x1e53 },
	{ 0x1e55, 0x1e55 },
	{ 0x1e57, 0x1e57 },
	{ 0x1e59, 0x1e59 },
	{ 0x1e5b, 0x1e5b },
	{ 0x1e5d, 0x1e5d },
	{ 0x1e5f, 0x1e5f },
	{ 0x1e61, 0x1e61 },
	{ 0x1e63, 0x1e63 },
	{ 0x1e65, 0x1e65 },
	{ 0x1e67, 0x1e67 },
	{ 0x1e69, 0x1e69 },
	{ 0x1e6b, 0x1e6b },
	{ 0x1e6d, 0x1e6d },
	{ 0x1e6f, 0x1e6f },
	{ 0x1e71, 0x1e71 },
	{ 0x1e73, 0x1e73 },
	{ 0x1e75, 0x1e75 },
	{ 0x1e77, 0x1e77 },
	{ 0x1e79, 0x1e79 },
	{ 0x1e7b, 0x1e7b },
	{ 0x1e7d, 0x1e7d },
	{ 0x1e7f, 0x1e7f },
	{ 0x1e81, 0x1e81 },
	{ 0x1e83, 0x1e83 },
	{ 0x1e85, 0x1e85 },
	{ 0x1e87, 0x1e87 },
	{ 0x1e89, 0x1e89 },
	{ 0x1e8b, 0x1e8b },
	{ 0x1e8d, 0x1e8d },
	{ 0x1e8f, 0x1e8f },
	{ 0x1e91, 0x1e91 },
	{ 0x1e93, 0x1e93 },
	{ 0x1e95, 0x1e9d },
	{ 0x1e9f, 0x1e9f },
	{ 0x1ea1, 0x1ea1 },
	{ 0x1ea3, 0x1ea3 },
	{ 0x1ea5, 0x1ea5 },
	{ 0x1ea7, 0x1ea7 },
	{ 0x1ea9, 0x1ea9 },
	{ 0x1eab, 0x1eab },
	{ 0x1ead, 0x1ead },
	{ 0x1eaf, 0x1eaf },
	{ 0x1eb1, 0x1eb1 },
	{ 0x1eb3, 0x1eb3 },
	{ 0x1eb5, 0x1eb5 },
	{ 0x1eb7, 0x1eb7 },
	{ 0x1eb9, 0x1eb9 },
	{ 0x1ebb, 0x1ebb },
	{ 0x1ebd, 0x1ebd },
	{ 0x1ebf, 0x1ebf },
	{ 0x1ec1, 0x1ec1 },
	{ 0x1ec3, 0x1ec3 },
	{ 0x1ec5, 0x1ec5 },
	{ 0x1ec7, 0x1ec7 },
	{ 0x1ec9, 0x1ec9 },
	{ 0x1ecb, 0x1ecb },
	{ 0x1ecd, 0x1ecd },
	{ 0x1ecf, 0x1ecf },
	{ 0x1ed1, 0x1ed1 },
	{ 0x1ed3, 0x1ed3 },
	{ 0x1ed5, 0x1ed5 },
	{ 0x1ed7, 0x1ed7 },
	{ 0x1ed9, 0x1ed9 },
	{ 0x1edb, 0x1edb },
	{ 0x1edd, 0x1edd },
	{ 0x1edf, 0x1edf },
	{ 0x1ee1, 0x1ee1 },
	{ 0x1ee3, 0x1ee3 },
	{ 0x1ee5, 0x1ee5 },
	{ 0x1ee7, 0x1ee7 },
	{ 0x1ee9, 0x1ee9 },
	{ 0x1eeb, 0x1eeb },
	{ 0x1eed, 0x1eed },
	{ 0x1eef, 0x1eef },
	{ 0x1ef1, 0x1ef1 },
	{ 0x1ef3, 0x1ef3 },
	{ 0x1ef5, 0x1ef5 },
	{ 0x1ef7, 0x1ef7 },
	{ 0x1ef9, 0x1ef9 },
	{ 0x1efb, 0x1efb },
	{ 0x1efd, 0x1efd },
	{ 0x1eff, 0x1f07 },
	{ 0x1f10, 0x1f15 },
	{ 0x1f20, 0x1f27 },
	{ 0x1f30, 0x1f37 },
	{ 0x1f40, 0x1f45 },
	{ 0x1f50, 0x1f57 },
	{ 0x1f60, 0x1f67 },
	{ 0x1f70, 0x1f7d },
	{ 0x1f80, 0x1f87 },
	{ 0x1f90, 0x1f97 },
	{ 0x1fa0, 0x1fa7 },
	{ 0x1fb0, 0x1fb4 },
	{ 0x1fb6, 0x1fb7 },
	{ 0x1fbe, 0x1fbe },
	{ 0x1fc2, 0x1fc4 },
	{ 0x1fc6, 0x1fc7 },
	{ 0x1fd0, 0x1fd3 },
	{ 0x1fd6, 0x1fd7 },
	{ 0x1fe0, 0x1fe7 },
	{ 0x1ff2, 0x1ff4 },
	{ 0x1ff6, 0x1ff7 },
	{ 0x2071, 0x2071 },
	{ 0x207f, 0x207f },
	{ 0x2090, 0x209c },
	{ 0x210a, 0x210a },
	{ 0x210e, 0x210f },
	{ 0x2113, 0x2113 },
	{ 0x212f, 0x212f },
	{ 0x2134, 0x2134 },
	{ 0x2139, 0x2139 },
	{ 0x213c, 0x213d },
	{ 0x2146, 0x2149 },
	{ 0x214e, 0x214e },
	{ 0x2170, 0x217f },
	{ 0x2184, 0x2184 },
	{ 0x24d0, 0x24e9 },
	{ 0x2c30, 0x2c5f },
	{ 0x2c61, 0x2c61 },
	{ 0x2c65, 0x2c66 },
	{ 0x2c68, 0x2c68 },
	{ 0x2c6a, 0x2c6a },
	{ 0x2c6c, 0x2c6c },
	{ 0x2c71, 0x2c71 },
	{ 0x2c73, 0x2c74 },
	{ 0x2c76, 0x2c7d },
	{ 0x2c81, 0x2c81 },
	{ 0x2c83, 0x2c83 },
	{ 0x2c85, 0x2c85 },
	{ 0x2c87, 0x2c87 },
	{ 0x2c89, 0x2c89 },
	{ 0x2c8b, 0x2c8b },
	{ 0x2c8d, 0x2c8d },
	{ 0x2c8f, 0x2c8f },
	{ 0x2c91, 0x2c91 },
	{ 0x2c93, 0x2c93 },
	{ 0x2c95, 0x2c95 },
	{ 0x2c97, 0x2c97 },
	{ 0x2c99, 0x2c99 },
	{ 0x2c9b, 0x2c9b },
	{ 0x2c9d, 0x2c9d },
	{ 0x2c9f, 0x2c9f },
	{ 0x2ca1, 0x2ca1 },
	{ 0x2ca3, 0x2ca3 },
	{ 0x2ca5, 0x2ca5 },
	{ 0x2ca7, 0x2ca7 },
	{ 0x2ca9, 0x2ca9 },
	{ 0x2cab, 0x2cab },
	{ 0x2cad, 0x2cad },
	{ 0x2caf, 0x2caf },
	{ 0x2cb1, 0x2cb1 },
	{ 0x2cb3, 0x2cb3 },
	{ 0x2cb5, 0x2cb5 },
	{ 0x2cb7, 0x2cb7 },
	{ 0x2cb9, 0x2cb9 },
	{ 0x2cbb, 0x2cbb },
	{ 0x2cbd, 0x2cbd },
	{ 0x2cbf, 0x2cbf },
	{ 0x2cc1, 0x2cc1 },
	{ 0x2cc3, 0x2cc3 },
	{ 0x2cc5, 0x2cc5 },
	{ 0x2cc7, 0x2cc7 },
	{ 0x2cc9, 0x2cc9 },
	{ 0x2ccb, 0x2ccb },
	{ 0x2ccd, 0x2ccd },
	{ 0x2ccf, 0x2ccf },
	{ 0x2cd1, 0x2cd1 },
	{ 0x2cd3, 0x2cd3 },
	{ 0x2cd5, 0x2cd5 },
	{ 0x2cd7, 0x2cd7 },
	{ 0x2cd9, 0x2cd9 },
	{ 0x2cdb, 0x2cdb },
	{ 0x2cdd, 0x2cdd },
	{ 0x2cdf, 0x2cdf },
	{ 0x2ce1, 0x2ce1 },
	{ 0x2ce3, 0x2ce4 },
	{ 0x2cec, 0x2cec },
	{ 0x2cee, 0x2cee },
	{ 0x2cf3, 0x2cf3 },
	{ 0x2d00, 0x2d25 },
	{ 0x2d27, 0x2d27 },
	{ 0x2d2d, 0x2d2d },
	{ 0xa641, 0xa641 },
	{ 0xa643, 0xa643 },
	{ 0xa645, 0xa645 },
	{ 0xa647, 0xa647 },
	{ 0xa649, 0xa649 },
	{ 0xa64b, 0xa64b },
	{ 0xa64d, 0xa64d },
	{ 0xa64f, 0xa64f },
	{ 0xa651, 0xa651 },
	{ 0xa653, 0xa653 },
	{ 0xa655, 0xa655 },
	{ 0xa657, 0xa657 },
	{ 0xa659, 0xa659 },
	{ 0xa65b, 0xa65b },
	{ 0xa65d, 0xa65d },
	{ 0xa65f, 0xa65f },
	{ 0xa661, 0xa661 },
	{ 0xa663, 0xa663 },
	{ 0xa665, 0xa665 },
	{ 0xa667, 0xa667 },
	{ 0xa669, 0xa669 },
	{ 0xa66b, 0xa66b },
	{ 0xa66d, 0xa66d },
	{ 0xa681, 0xa681 },
	{ 0xa683, 0xa683 },
	{ 0xa685, 0xa685 },
	{ 0xa687, 0xa687 },
	{ 0xa689, 0xa689 },
	{ 0xa68b, 0xa68b },
	{ 0xa68d, 0xa68d },
	{ 0xa68f, 0xa68f },
	{ 0xa691, 0xa691 },
	{ 0xa693, 0xa693 },
	{ 0xa695, 0xa695 },
	{ 0xa697, 0xa697 },
	{ 0xa699, 0xa699 },
	{ 0xa69b, 0xa69d },
	{ 0xa723, 0xa723 },
	{ 0xa725, 0xa725 },
	{ 0xa727, 0xa727 },
	{ 0xa729, 0xa729 },
	{ 0xa72b, 0xa72b },
	{ 0xa72d, 0xa72d },
	{ 0xa72f, 0xa731 },
	{ 0xa733, 0xa733 },
	{ 0xa735, 0xa735 },
	{ 0xa737, 0xa737 },
	{ 0xa739, 0xa739 },
	{ 0xa73b, 0xa73b },
	{ 0xa73d, 0xa73d },
	{ 0xa73f, 0xa73f },
	{ 0xa741, 0xa741 },
	{ 0xa743, 0xa743 },
	{ 0xa745, 0xa745 },
	{ 0xa747, 0xa747 },
	{ 0xa749, 0xa749 },
	{ 0xa74b, 0xa74b },
	{ 0xa74d, 0xa74d },
	{ 0xa74f, 0xa74f },
	{ 0xa751, 0xa751 },
	{ 0xa753, 0xa753 },
	{ 0xa755, 0xa755 },
	{ 0xa757, 0xa757 },
	{ 0xa759, 0xa759 },
	{ 0xa75b, 0xa75b },
	{ 0xa75d, 0xa75d },
	{ 0xa75f, 0xa75f },
	{ 0xa761, 0xa761 },
	{ 0xa763, 0xa763 },
	{ 0xa765, 0xa765 },
	{ 0xa767, 0xa767 },
	{ 0xa769, 0xa769 },
	{ 0xa76b, 0xa76b },
	{ 0xa76d, 0xa76d },
	{ 0xa76f, 0xa778 },
	{ 0xa77a, 0xa77a },
	{ 0xa77c, 0xa77c },
	{ 0xa77f, 0xa77f },
	{ 0xa781, 0xa781 },
	{ 0xa783, 0xa783 },
	{ 0xa785, 0xa785 },
	{ 0xa787, 0xa787 },
	{ 0xa78c, 0xa78c },
	{ 0xa78e, 0xa78e },
	{ 0xa791, 0xa791 },
	{ 0xa793, 0xa795 },
	{ 0xa797, 0xa797 },
	{ 0xa799, 0xa799 },
	{ 0xa79b, 0xa79b },
	{ 0xa79d, 0xa79d },
	{ 0xa79f, 0xa79f },
	{ 0xa7a1, 0xa7a1 },
	{ 0xa7a3, 0xa7a3 },
	{ 0xa7a5, 0xa7a5 },
	{ 0xa7a7, 0xa7a7 },
	{ 0xa7a9, 0xa7a9 },
	{ 0xa7af, 0xa7af },
	{ 0xa7b5, 0xa7b5 },
	{ 0xa7b7, 0xa7b7 },
	{ 0xa7b9, 0xa7b9 },
	{ 0xa7bb, 0xa7bb },
	{ 0xa7bd, 0xa7bd },
	{ 0xa7bf, 0xa7bf },
	{ 0xa7c1, 0xa7c1 },
	{ 0xa7c3, 0xa7c3 },
	{ 0xa7c8, 0xa7c8 },
	{ 0xa7ca, 0xa7ca },
	{ 0xa7cd, 0xa7cd },
	{ 0xa7cf, 0xa7cf },
	{ 0xa7d1, 0xa7d1 },
	{ 0xa7d3, 0xa7d3 },
	{ 0xa7d5, 0xa7d5 },
	{ 0xa7d7, 0xa7d7 },
	{ 0xa7d9, 0xa7d9 },
	{ 0xa7db, 0xa7db },
	{ 0xa7f1, 0xa7f4 },
	{ 0xa7f6, 0xa7f6 },
	{ 0xa7f8, 0xa7fa },
	{ 0xab30, 0xab5a },
	{ 0xab5c, 0xab69 },
	{ 0xab70, 0xabbf },
	{ 0xfb00, 0xfb06 },
	{ 0xfb13, 0xfb17 },
	{ 0xff41, 0xff5a },
	{ 0x10428, 0x1044f },
	{ 0x104d8, 0x104fb },
	{ 0x10597, 0x105a1 },
	{ 0x105a3, 0x105b1 },
	{ 0x105b3, 0x105b9 },
	{ 0x105bb, 0x105bc },
	{ 0x10780, 0x10780 },
	{ 0x10783, 0x10785 },
	{ 0x10787, 0x107b0 },
	{ 0x107b2, 0x107ba },
	{ 0x10cc0, 0x10cf2 },
	{ 0x10d70, 0x10d85 },
	{ 0x118c0, 0x118df },
	{ 0x16e60, 0x16e7f },
	{ 0x16ebb, 0x16ed3 },
	{ 0x1d41a, 0x1d433 },
	{ 0x1d44e, 0x1d454 },
	{ 0x1d456, 0x1d467 },
	{ 0x1d482, 0x1d49b },
	{ 0x1d4b6, 0x1d4b9 },
	{ 0x1d4bb, 0x1d4bb },
	{ 0x1d4bd, 0x1d4c3 },
	{ 0x1d4c5, 0x1d4cf },
	{ 0x1d4ea, 0x1d503 },
	{ 0x1d51e, 0x1d537 },
	{ 0x1d552, 0x1d56b },
	{ 0x1d586, 0x1d59f },
	{ 0x1d5ba, 0x1d5d3 },
	{ 0x1d5ee, 0x1d607 },
	{ 0x1d622, 0x1d63b },
	{ 0x1d656, 0x1d66f },
	{ 0x1d68a, 0x1d6a5 },
	{ 0x1d6c2, 0x1d6da },
	{ 0x1d6dc, 0x1d6e1 },
	{ 0x1d6fc, 0x1d714 },
	{ 0x1d716, 0x1d71b },
	{ 0x1d736, 0x1d74e },
	{ 0x1d750, 0x1d755 },
	{ 0x1d770, 0x1d788 },
	{ 0x1d78a, 0x1d78f },
	{ 0x1d7aa, 0x1d7c2 },
	{ 0x1d7c4, 0x1d7c9 },
	{ 0x1d7cb, 0x1d7cb },
	{ 0x1df00, 0x1df09 },
	{ 0x1df0b, 0x1df1e },
	{ 0x1df25, 0x1df2a },
	{ 0x1e030, 0x1e06d },
	{ 0x1e922, 0x1e943 },
};

const int unicode_letter_size = 761;
const CharRange unicode_letter[unicode_letter_size] = {
	{ 0x41, 0x5a },
	{ 0x61, 0x7a },
	{ 0xaa, 0xaa },
	{ 0xb5, 0xb5 },
	{ 0xba, 0xba },
	{ 0xc0, 0xd6 },
	{ 0xd8, 0xf6 },
	{ 0xf8, 0x2c1 },
	{ 0x2c6, 0x2d1 },
	{ 0x2e0, 0x2e4 },
	{ 0x2ec, 0x2ec },
	{ 0x2ee, 0x2ee },
	{ 0x345, 0x345 },
	{ 0x363, 0x374 },
	{ 0x376, 0x377 },
	{ 0x37a, 0x37d },
	{ 0x37f, 0x37f },
	{ 0x386, 0x386 },
	{ 0x388, 0x38a },
	{ 0x38c, 0x38c },
	{ 0x38e, 0x3a1 },
	{ 0x3a3, 0x3f5 },
	{ 0x3f7, 0x481 },
	{ 0x48a, 0x52f },
	{ 0x531, 0x556 },
	{ 0x559, 0x559 },
	{ 0x560, 0x588 },
	{ 0x5b0, 0x5bd },
	{ 0x5bf, 0x5bf },
	{ 0x5c1, 0x5c2 },
	{ 0x5c4, 0x5c5 },
	{ 0x5c7, 0x5c7 },
	{ 0x5d0, 0x5ea },
	{ 0x5ef, 0x5f2 },
	{ 0x610, 0x61a },
	{ 0x620, 0x657 },
	{ 0x659, 0x65f },
	{ 0x66e, 0x6d3 },
	{ 0x6d5, 0x6dc },
	{ 0x6e1, 0x6e8 },
	{ 0x6ed, 0x6ef },
	{ 0x6fa, 0x6fc },
	{ 0x6ff, 0x6ff },
	{ 0x710, 0x73f },
	{ 0x74d, 0x7b1 },
	{ 0x7ca, 0x7ea },
	{ 0x7f4, 0x7f5 },
	{ 0x7fa, 0x7fa },
	{ 0x800, 0x817 },
	{ 0x81a, 0x82c },
	{ 0x840, 0x858 },
	{ 0x860, 0x86a },
	{ 0x870, 0x887 },
	{ 0x889, 0x88f },
	{ 0x897, 0x897 },
	{ 0x8a0, 0x8c9 },
	{ 0x8d4, 0x8df },
	{ 0x8e3, 0x8e9 },
	{ 0x8f0, 0x93b },
	{ 0x93d, 0x94c },
	{ 0x94e, 0x950 },
	{ 0x955, 0x963 },
	{ 0x971, 0x983 },
	{ 0x985, 0x98c },
	{ 0x98f, 0x990 },
	{ 0x993, 0x9a8 },
	{ 0x9aa, 0x9b0 },
	{ 0x9b2, 0x9b2 },
	{ 0x9b6, 0x9b9 },
	{ 0x9bd, 0x9c4 },
	{ 0x9c7, 0x9c8 },
	{ 0x9cb, 0x9cc },
	{ 0x9ce, 0x9ce },
	{ 0x9d7, 0x9d7 },
	{ 0x9dc, 0x9dd },
	{ 0x9df, 0x9e3 },
	{ 0x9f0, 0x9f1 },
	{ 0x9fc, 0x9fc },
	{ 0xa01, 0xa03 },
	{ 0xa05, 0xa0a },
	{ 0xa0f, 0xa10 },
	{ 0xa13, 0xa28 },
	{ 0xa2a, 0xa30 },
	{ 0xa32, 0xa33 },
	{ 0xa35, 0xa36 },
	{ 0xa38, 0xa39 },
	{ 0xa3e, 0xa42 },
	{ 0xa47, 0xa48 },
	{ 0xa4b, 0xa4c },
	{ 0xa51, 0xa51 },
	{ 0xa59, 0xa5c },
	{ 0xa5e, 0xa5e },
	{ 0xa70, 0xa75 },
	{ 0xa81, 0xa83 },
	{ 0xa85, 0xa8d },
	{ 0xa8f, 0xa91 },
	{ 0xa93, 0xaa8 },
	{ 0xaaa, 0xab0 },
	{ 0xab2, 0xab3 },
	{ 0xab5, 0xab9 },
	{ 0xabd, 0xac5 },
	{ 0xac7, 0xac9 },
	{ 0xacb, 0xacc },
	{ 0xad0, 0xad0 },
	{ 0xae0, 0xae3 },
	{ 0xaf9, 0xafc },
	{ 0xb01, 0xb03 },
	{ 0xb05, 0xb0c },
	{ 0xb0f, 0xb10 },
	{ 0xb13, 0xb28 },
	{ 0xb2a, 0xb30 },
	{ 0xb32, 0xb33 },
	{ 0xb35, 0xb39 },
	{ 0xb3d, 0xb44 },
	{ 0xb47, 0xb48 },
	{ 0xb4b, 0xb4c },
	{ 0xb56, 0xb57 },
	{ 0xb5c, 0xb5d },
	{ 0xb5f, 0xb63 },
	{ 0xb71, 0xb71 },
	{ 0xb82, 0xb83 },
	{ 0xb85, 0xb8a },
	{ 0xb8e, 0xb90 },
	{ 0xb92, 0xb95 },
	{ 0xb99, 0xb9a },
	{ 0xb9c, 0xb9c },
	{ 0xb9e, 0xb9f },
	{ 0xba3, 0xba4 },
	{ 0xba8, 0xbaa },
	{ 0xbae, 0xbb9 },
	{ 0xbbe, 0xbc2 },
	{ 0xbc6, 0xbc8 },
	{ 0xbca, 0xbcc },
	{ 0xbd0, 0xbd0 },
	{ 0xbd7, 0xbd7 },
	{ 0xc00, 0xc0c },
	{ 0xc0e, 0xc10 },
	{ 0xc12, 0xc28 },
	{ 0xc2a, 0xc39 },
	{ 0xc3d, 0xc44 },
	{ 0xc46, 0xc48 },
	{ 0xc4a, 0xc4c },
	{ 0xc55, 0xc56 },
	{ 0xc58, 0xc5a },
	{ 0xc5c, 0xc5d },
	{ 0xc60, 0xc63 },
	{ 0xc80, 0xc83 },
	{ 0xc85, 0xc8c },
	{ 0xc8e, 0xc90 },
	{ 0xc92, 0xca8 },
	{ 0xcaa, 0xcb3 },
	{ 0xcb5, 0xcb9 },
	{ 0xcbd, 0xcc4 },
	{ 0xcc6, 0xcc8 },
	{ 0xcca, 0xccc },
	{ 0xcd5, 0xcd6 },
	{ 0xcdc, 0xcde },
	{ 0xce0, 0xce3 },
	{ 0xcf1, 0xcf3 },
	{ 0xd00, 0xd0c },
	{ 0xd0e, 0xd10 },
	{ 0xd12, 0xd3a },
	{ 0xd3d, 0xd44 },
	{ 0xd46, 0xd48 },
	{ 0xd4a, 0xd4c },
	{ 0xd4e, 0xd4e },
	{ 0xd54, 0xd57 },
	{ 0xd5f, 0xd63 },
	{ 0xd7a, 0xd7f },
	{ 0xd81, 0xd83 },
	{ 0xd85, 0xd96 },
	{ 0xd9a, 0xdb1 },
	{ 0xdb3, 0xdbb },
	{ 0xdbd, 0xdbd },
	{ 0xdc0, 0xdc6 },
	{ 0xdcf, 0xdd4 },
	{ 0xdd6, 0xdd6 },
	{ 0xdd8, 0xddf },
	{ 0xdf2, 0xdf3 },
	{ 0xe01, 0xe3a },
	{ 0xe40, 0xe46 },
	{ 0xe4d, 0xe4d },
	{ 0xe81, 0xe82 },
	{ 0xe84, 0xe84 },
	{ 0xe86, 0xe8a },
	{ 0xe8c, 0xea3 },
	{ 0xea5, 0xea5 },
	{ 0xea7, 0xeb9 },
	{ 0xebb, 0xebd },
	{ 0xec0, 0xec4 },
	{ 0xec6, 0xec6 },
	{ 0xecd, 0xecd },
	{ 0xedc, 0xedf },
	{ 0xf00, 0xf00 },
	{ 0xf40, 0xf47 },
	{ 0xf49, 0xf6c },
	{ 0xf71, 0xf83 },
	{ 0xf88, 0xf97 },
	{ 0xf99, 0xfbc },
	{ 0x1000, 0x1036 },
	{ 0x1038, 0x1038 },
	{ 0x103b, 0x103f },
	{ 0x1050, 0x108f },
	{ 0x109a, 0x109d },
	{ 0x10a0, 0x10c5 },
	{ 0x10c7, 0x10c7 },
	{ 0x10cd, 0x10cd },
	{ 0x10d0, 0x10fa },
	{ 0x10fc, 0x1248 },
	{ 0x124a, 0x124d },
	{ 0x1250, 0x1256 },
	{ 0x1258, 0x1258 },
	{ 0x125a, 0x125d },
	{ 0x1260, 0x1288 },
	{ 0x128a, 0x128d },
	{ 0x1290, 0x12b0 },
	{ 0x12b2, 0x12b5 },
	{ 0x12b8, 0x12be },
	{ 0x12c0, 0x12c0 },
	{ 0x12c2, 0x12c5 },
	{ 0x12c8, 0x12d6 },
	{ 0x12d8, 0x1310 },
	{ 0x1312, 0x1315 },
	{ 0x1318, 0x135a },
	{ 0x1380, 0x138f },
	{ 0x13a0, 0x13f5 },
	{ 0x13f8, 0x13fd },
	{ 0x1401, 0x166c },
	{ 0x166f, 0x167f },
	{ 0x1681, 0x169a },
	{ 0x16a0, 0x16ea },
	{ 0x16ee, 0x16f8 },
	{ 0x1700, 0x1713 },
	{ 0x171f, 0x1733 },
	{ 0x1740, 0x1753 },
	{ 0x1760, 0x176c },
	{ 0x176e, 0x1770 },
	{ 0x1772, 0x1773 },
	{ 0x1780, 0x17b3 },
	{ 0x17b6, 0x17c8 },
	{ 0x17d7, 0x17d7 },
	{ 0x17dc, 0x17dc },
	{ 0x1820, 0x1878 },
	{ 0x1880, 0x18aa },
	{ 0x18b0, 0x18f5 },
	{ 0x1900, 0x191e },
	{ 0x1920, 0x192b },
	{ 0x1930, 0x1938 },
	{ 0x1950, 0x196d },
	{ 0x1970, 0x1974 },
	{ 0x1980, 0x19ab },
	{ 0x19b0, 0x19c9 },
	{ 0x1a00, 0x1a1b },
	{ 0x1a20, 0x1a5e },
	{ 0x1a61, 0x1a74 },
	{ 0x1aa7, 0x1aa7 },
	{ 0x1abf, 0x1ac0 },
	{ 0x1acc, 0x1ace },
	{ 0x1b00, 0x1b33 },
	{ 0x1b35, 0x1b43 },
	{ 0x1b45, 0x1b4c },
	{ 0x1b80, 0x1ba9 },
	{ 0x1bac, 0x1baf },
	{ 0x1bba, 0x1be5 },
	{ 0x1be7, 0x1bf1 },
	{ 0x1c00, 0x1c36 },
	{ 0x1c4d, 0x1c4f },
	{ 0x1c5a, 0x1c7d },
	{ 0x1c80, 0x1c8a },
	{ 0x1c90, 0x1cba },
	{ 0x1cbd, 0x1cbf },
	{ 0x1ce9, 0x1cec },
	{ 0x1cee, 0x1cf3 },
	{ 0x1cf5, 0x1cf6 },
	{ 0x1cfa, 0x1cfa },
	{ 0x1d00, 0x1dbf },
	{ 0x1dd3, 0x1df4 },
	{ 0x1e00, 0x1f15 },
	{ 0x1f18, 0x1f1d },
	{ 0x1f20, 0x1f45 },
	{ 0x1f48, 0x1f4d },
	{ 0x1f50, 0x1f57 },
	{ 0x1f59, 0x1f59 },
	{ 0x1f5b, 0x1f5b },
	{ 0x1f5d, 0x1f5d },
	{ 0x1f5f, 0x1f7d },
	{ 0x1f80, 0x1fb4 },
	{ 0x1fb6, 0x1fbc },
	{ 0x1fbe, 0x1fbe },
	{ 0x1fc2, 0x1fc4 },
	{ 0x1fc6, 0x1fcc },
	{ 0x1fd0, 0x1fd3 },
	{ 0x1fd6, 0x1fdb },
	{ 0x1fe0, 0x1fec },
	{ 0x1ff2, 0x1ff4 },
	{ 0x1ff6, 0x1ffc },
	{ 0x2071, 0x2071 },
	{ 0x207f, 0x207f },
	{ 0x2090, 0x209c },
	{ 0x2102, 0x2102 },
	{ 0x2107, 0x2107 },
	{ 0x210a, 0x2113 },
	{ 0x2115, 0x2115 },
	{ 0x2119, 0x211d },
	{ 0x2124, 0x2124 },
	{ 0x2126, 0x2126 },
	{ 0x2128, 0x2128 },
	{ 0x212a, 0x212d },
	{ 0x212f, 0x2139 },
	{ 0x213c, 0x213f },
	{ 0x2145, 0x2149 },
	{ 0x214e, 0x214e },
	{ 0x2160, 0x2188 },
	{ 0x24b6, 0x24e9 },
	{ 0x2c00, 0x2ce4 },
	{ 0x2ceb, 0x2cee },
	{ 0x2cf2, 0x2cf3 },
	{ 0x2d00, 0x2d25 },
	{ 0x2d27, 0x2d27 },
	{ 0x2d2d, 0x2d2d },
	{ 0x2d30, 0x2d67 },
	{ 0x2d6f, 0x2d6f },
	{ 0x2d80, 0x2d96 },
	{ 0x2da0, 0x2da6 },
	{ 0x2da8, 0x2dae },
	{ 0x2db0, 0x2db6 },
	{ 0x2db8, 0x2dbe },
	{ 0x2dc0, 0x2dc6 },
	{ 0x2dc8, 0x2dce },
	{ 0x2dd0, 0x2dd6 },
	{ 0x2dd8, 0x2dde },
	{ 0x2de0, 0x2dff },
	{ 0x2e2f, 0x2e2f },
	{ 0x3005, 0x3007 },
	{ 0x3021, 0x3029 },
	{ 0x3031, 0x3035 },
	{ 0x3038, 0x303c },
	{ 0x3041, 0x3096 },
	{ 0x309d, 0x309f },
	{ 0x30a1, 0x30fa },
	{ 0x30fc, 0x30ff },
	{ 0x3105, 0x312f },
	{ 0x3131, 0x318e },
	{ 0x31a0, 0x31bf },
	{ 0x31f0, 0x31ff },
	{ 0x3400, 0x4dbf },
	{ 0x4e00, 0xa48c },
	{ 0xa4d0, 0xa4fd },
	{ 0xa500, 0xa60c },
	{ 0xa610, 0xa61f },
	{ 0xa62a, 0xa62b },
	{ 0xa640, 0xa66e },
	{ 0xa674, 0xa67b },
	{ 0xa67f, 0xa6ef },
	{ 0xa717, 0xa71f },
	{ 0xa722, 0xa788 },
	{ 0xa78b, 0xa7dc },
	{ 0xa7f1, 0xa805 },
	{ 0xa807, 0xa827 },
	{ 0xa840, 0xa873 },
	{ 0xa880, 0xa8c3 },
	{ 0xa8c5, 0xa8c5 },
	{ 0xa8f2, 0xa8f7 },
	{ 0xa8fb, 0xa8fb },
	{ 0xa8fd, 0xa8ff },
	{ 0xa90a, 0xa92a },
	{ 0xa930, 0xa952 },
	{ 0xa960, 0xa97c },
	{ 0xa980, 0xa9b2 },
	{ 0xa9b4, 0xa9bf },
	{ 0xa9cf, 0xa9cf },
	{ 0xa9e0, 0xa9ef },
	{ 0xa9fa, 0xa9fe },
	{ 0xaa00, 0xaa36 },
	{ 0xaa40, 0xaa4d },
	{ 0xaa60, 0xaa76 },
	{ 0xaa7a, 0xaabe },
	{ 0xaac0, 0xaac0 },
	{ 0xaac2, 0xaac2 },
	{ 0xaadb, 0xaadd },
	{ 0xaae0, 0xaaef },
	{ 0xaaf2, 0xaaf5 },
	{ 0xab01, 0xab06 },
	{ 0xab09, 0xab0e },
	{ 0xab11, 0xab16 },
	{ 0xab20, 0xab26 },
	{ 0xab28, 0xab2e },
	{ 0xab30, 0xab5a },
	{ 0xab5c, 0xab69 },
	{ 0xab70, 0xabea },
	{ 0xac00, 0xd7a3 },
	{ 0xd7b0, 0xd7c6 },
	{ 0xd7cb, 0xd7fb },
	{ 0xf900, 0xfa6d },
	{ 0xfa70, 0xfad9 },
	{ 0xfb00, 0xfb06 },
	{ 0xfb13, 0xfb17 },
	{ 0xfb1d, 0xfb28 },
	{ 0xfb2a, 0xfb36 },
	{ 0xfb38, 0xfb3c },
	{ 0xfb3e, 0xfb3e },
	{ 0xfb40, 0xfb41 },
	{ 0xfb43, 0xfb44 },
	{ 0xfb46, 0xfbb1 },
	{ 0xfbd3, 0xfd3d },
	{ 0xfd50, 0xfd8f },
	{ 0xfd92, 0xfdc7 },
	{ 0xfdf0, 0xfdfb },
	{ 0xfe70, 0xfe74 },
	{ 0xfe76, 0xfefc },
	{ 0xff21, 0xff3a },
	{ 0xff41, 0xff5a },
	{ 0xff66, 0xffbe },
	{ 0xffc2, 0xffc7 },
	{ 0xffca, 0xffcf },
	{ 0xffd2, 0xffd7 },
	{ 0xffda, 0xffdc },
	{ 0x10000, 0x1000b },
	{ 0x1000d, 0x10026 },
	{ 0x10028, 0x1003a },
	{ 0x1003c, 0x1003d },
	{ 0x1003f, 0x1004d },
	{ 0x10050, 0x1005d },
	{ 0x10080, 0x100fa },
	{ 0x10140, 0x10174 },
	{ 0x10280, 0x1029c },
	{ 0x102a0, 0x102d0 },
	{ 0x10300, 0x1031f },
	{ 0x1032d, 0x1034a },
	{ 0x10350, 0x1037a },
	{ 0x10380, 0x1039d },
	{ 0x103a0, 0x103c3 },
	{ 0x103c8, 0x103cf },
	{ 0x103d1, 0x103d5 },
	{ 0x10400, 0x1049d },
	{ 0x104b0, 0x104d3 },
	{ 0x104d8, 0x104fb },
	{ 0x10500, 0x10527 },
	{ 0x10530, 0x10563 },
	{ 0x10570, 0x1057a },
	{ 0x1057c, 0x1058a },
	{ 0x1058c, 0x10592 },
	{ 0x10594, 0x10595 },
	{ 0x10597, 0x105a1 },
	{ 0x105a3, 0x105b1 },
	{ 0x105b3, 0x105b9 },
	{ 0x105bb, 0x105bc },
	{ 0x105c0, 0x105f3 },
	{ 0x10600, 0x10736 },
	{ 0x10740, 0x10755 },
	{ 0x10760, 0x10767 },
	{ 0x10780, 0x10785 },
	{ 0x10787, 0x107b0 },
	{ 0x107b2, 0x107ba },
	{ 0x10800, 0x10805 },
	{ 0x10808, 0x10808 },
	{ 0x1080a, 0x10835 },
	{ 0x10837, 0x10838 },
	{ 0x1083c, 0x1083c },
	{ 0x1083f, 0x10855 },
	{ 0x10860, 0x10876 },
	{ 0x10880, 0x1089e },
	{ 0x108e0, 0x108f2 },
	{ 0x108f4, 0x108f5 },
	{ 0x10900, 0x10915 },
	{ 0x10920, 0x10939 },
	{ 0x10940, 0x10959 },
	{ 0x10980, 0x109b7 },
	{ 0x109be, 0x109bf },
	{ 0x10a00, 0x10a03 },
	{ 0x10a05, 0x10a06 },
	{ 0x10a0c, 0x10a13 },
	{ 0x10a15, 0x10a17 },
	{ 0x10a19, 0x10a35 },
	{ 0x10a60, 0x10a7c },
	{ 0x10a80, 0x10a9c },
	{ 0x10ac0, 0x10ac7 },
	{ 0x10ac9, 0x10ae4 },
	{ 0x10b00, 0x10b35 },
	{ 0x10b40, 0x10b55 },
	{ 0x10b60, 0x10b72 },
	{ 0x10b80, 0x10b91 },
	{ 0x10c00, 0x10c48 },
	{ 0x10c80, 0x10cb2 },
	{ 0x10cc0, 0x10cf2 },
	{ 0x10d00, 0x10d27 },
	{ 0x10d4a, 0x10d65 },
	{ 0x10d69, 0x10d69 },
	{ 0x10d6f, 0x10d85 },
	{ 0x10e80, 0x10ea9 },
	{ 0x10eab, 0x10eac },
	{ 0x10eb0, 0x10eb1 },
	{ 0x10ec2, 0x10ec7 },
	{ 0x10efa, 0x10efc },
	{ 0x10f00, 0x10f1c },
	{ 0x10f27, 0x10f27 },
	{ 0x10f30, 0x10f45 },
	{ 0x10f70, 0x10f81 },
	{ 0x10fb0, 0x10fc4 },
	{ 0x10fe0, 0x10ff6 },
	{ 0x11000, 0x11045 },
	{ 0x11071, 0x11075 },
	{ 0x11080, 0x110b8 },
	{ 0x110c2, 0x110c2 },
	{ 0x110d0, 0x110e8 },
	{ 0x11100, 0x11132 },
	{ 0x11144, 0x11147 },
	{ 0x11150, 0x11172 },
	{ 0x11176, 0x11176 },
	{ 0x11180, 0x111bf },
	{ 0x111c1, 0x111c4 },
	{ 0x111ce, 0x111cf },
	{ 0x111da, 0x111da },
	{ 0x111dc, 0x111dc },
	{ 0x11200, 0x11211 },
	{ 0x11213, 0x11234 },
	{ 0x11237, 0x11237 },
	{ 0x1123e, 0x11241 },
	{ 0x11280, 0x11286 },
	{ 0x11288, 0x11288 },
	{ 0x1128a, 0x1128d },
	{ 0x1128f, 0x1129d },
	{ 0x1129f, 0x112a8 },
	{ 0x112b0, 0x112e8 },
	{ 0x11300, 0x11303 },
	{ 0x11305, 0x1130c },
	{ 0x1130f, 0x11310 },
	{ 0x11313, 0x11328 },
	{ 0x1132a, 0x11330 },
	{ 0x11332, 0x11333 },
	{ 0x11335, 0x11339 },
	{ 0x1133d, 0x11344 },
	{ 0x11347, 0x11348 },
	{ 0x1134b, 0x1134c },
	{ 0x11350, 0x11350 },
	{ 0x11357, 0x11357 },
	{ 0x1135d, 0x11363 },
	{ 0x11380, 0x11389 },
	{ 0x1138b, 0x1138b },
	{ 0x1138e, 0x1138e },
	{ 0x11390, 0x113b5 },
	{ 0x113b7, 0x113c0 },
	{ 0x113c2, 0x113c2 },
	{ 0x113c5, 0x113c5 },
	{ 0x113c7, 0x113ca },
	{ 0x113cc, 0x113cd },
	{ 0x113d1, 0x113d1 },
	{ 0x113d3, 0x113d3 },
	{ 0x11400, 0x11441 },
	{ 0x11443, 0x11445 },
	{ 0x11447, 0x1144a },
	{ 0x1145f, 0x11461 },
	{ 0x11480, 0x114c1 },
	{ 0x114c4, 0x114c5 },
	{ 0x114c7, 0x114c7 },
	{ 0x11580, 0x115b5 },
	{ 0x115b8, 0x115be },
	{ 0x115d8, 0x115dd },
	{ 0x11600, 0x1163e },
	{ 0x11640, 0x11640 },
	{ 0x11644, 0x11644 },
	{ 0x11680, 0x116b5 },
	{ 0x116b8, 0x116b8 },
	{ 0x11700, 0x1171a },
	{ 0x1171d, 0x1172a },
	{ 0x11740, 0x11746 },
	{ 0x11800, 0x11838 },
	{ 0x118a0, 0x118df },
	{ 0x118ff, 0x11906 },
	{ 0x11909, 0x11909 },
	{ 0x1190c, 0x11913 },
	{ 0x11915, 0x11916 },
	{ 0x11918, 0x11935 },
	{ 0x11937, 0x11938 },
	{ 0x1193b, 0x1193c },
	{ 0x1193f, 0x11942 },
	{ 0x119a0, 0x119a7 },
	{ 0x119aa, 0x119d7 },
	{ 0x119da, 0x119df },
	{ 0x119e1, 0x119e1 },
	{ 0x119e3, 0x119e4 },
	{ 0x11a00, 0x11a32 },
	{ 0x11a35, 0x11a3e },
	{ 0x11a50, 0x11a97 },
	{ 0x11a9d, 0x11a9d },
	{ 0x11ab0, 0x11af8 },
	{ 0x11b60, 0x11b67 },
	{ 0x11bc0, 0x11be0 },
	{ 0x11c00, 0x11c08 },
	{ 0x11c0a, 0x11c36 },
	{ 0x11c38, 0x11c3e },
	{ 0x11c40, 0x11c40 },
	{ 0x11c72, 0x11c8f },
	{ 0x11c92, 0x11ca7 },
	{ 0x11ca9, 0x11cb6 },
	{ 0x11d00, 0x11d06 },
	{ 0x11d08, 0x11d09 },
	{ 0x11d0b, 0x11d36 },
	{ 0x11d3a, 0x11d3a },
	{ 0x11d3c, 0x11d3d },
	{ 0x11d3f, 0x11d41 },
	{ 0x11d43, 0x11d43 },
	{ 0x11d46, 0x11d47 },
	{ 0x11d60, 0x11d65 },
	{ 0x11d67, 0x11d68 },
	{ 0x11d6a, 0x11d8e },
	{ 0x11d90, 0x11d91 },
	{ 0x11d93, 0x11d96 },
	{ 0x11d98, 0x11d98 },
	{ 0x11db0, 0x11ddb },
	{ 0x11ee0, 0x11ef6 },
	{ 0x11f00, 0x11f10 },
	{ 0x11f12, 0x11f3a },
	{ 0x11f3e, 0x11f40 },
	{ 0x11fb0, 0x11fb0 },
	{ 0x12000, 0x12399 },
	{ 0x12400, 0x1246e },
	{ 0x12480, 0x12543 },
	{ 0x12f90, 0x12ff0 },
	{ 0x13000, 0x1342f },
	{ 0x13441, 0x13446 },
	{ 0x13460, 0x143fa },
	{ 0x14400, 0x14646 },
	{ 0x16100, 0x1612e },
	{ 0x16800, 0x16a38 },
	{ 0x16a40, 0x16a5e },
	{ 0x16a70, 0x16abe },
	{ 0x16ad0, 0x16aed },
	{ 0x16b00, 0x16b2f },
	{ 0x16b40, 0x16b43 },
	{ 0x16b63, 0x16b77 },
	{ 0x16b7d, 0x16b8f },
	{ 0x16d40, 0x16d6c },
	{ 0x16e40, 0x16e7f },
	{ 0x16ea0, 0x16eb8 },
	{ 0x16ebb, 0x16ed3 },
	{ 0x16f00, 0x16f4a },
	{ 0x16f4f, 0x16f87 },
	{ 0x16f8f, 0x16f9f },
	{ 0x16fe0, 0x16fe1 },
	{ 0x16fe3, 0x16fe3 },
	{ 0x16ff0, 0x16ff6 },
	{ 0x17000, 0x18cd5 },
	{ 0x18cff, 0x18d1e },
	{ 0x18d80, 0x18df2 },
	{ 0x1aff0, 0x1aff3 },
	{ 0x1aff5, 0x1affb },
	{ 0x1affd, 0x1affe },
	{ 0x1b000, 0x1b122 },
	{ 0x1b132, 0x1b132 },
	{ 0x1b150, 0x1b152 },
	{ 0x1b155, 0x1b155 },
	{ 0x1b164, 0x1b167 },
	{ 0x1b170, 0x1b2fb },
	{ 0x1bc00, 0x1bc6a },
	{ 0x1bc70, 0x1bc7c },
	{ 0x1bc80, 0x1bc88 },
	{ 0x1bc90, 0x1bc99 },
	{ 0x1bc9e, 0x1bc9e },
	{ 0x1d400, 0x1d454 },
	{ 0x1d456, 0x1d49c },
	{ 0x1d49e, 0x1d49f },
	{ 0x1d4a2, 0x1d4a2 },
	{ 0x1d4a5, 0x1d4a6 },
	{ 0x1d4a9, 0x1d4ac },
	{ 0x1d4ae, 0x1d4b9 },
	{ 0x1d4bb, 0x1d4bb },
	{ 0x1d4bd, 0x1d4c3 },
	{ 0x1d4c5, 0x1d505 },
	{ 0x1d507, 0x1d50a },
	{ 0x1d50d, 0x1d514 },
	{ 0x1d516, 0x1d51c },
	{ 0x1d51e, 0x1d539 },
	{ 0x1d53b, 0x1d53e },
	{ 0x1d540, 0x1d544 },
	{ 0x1d546, 0x1d546 },
	{ 0x1d54a, 0x1d550 },
	{ 0x1d552, 0x1d6a5 },
	{ 0x1d6a8, 0x1d6c0 },
	{ 0x1d6c2, 0x1d6da },
	{ 0x1d6dc, 0x1d6fa },
	{ 0x1d6fc, 0x1d714 },
	{ 0x1d716, 0x1d734 },
	{ 0x1d736, 0x1d74e },
	{ 0x1d750, 0x1d76e },
	{ 0x1d770, 0x1d788 },
	{ 0x1d78a, 0x1d7a8 },
	{ 0x1d7aa, 0x1d7c2 },
	{ 0x1d7c4, 0x1d7cb },
	{ 0x1df00, 0x1df1e },
	{ 0x1df25, 0x1df2a },
	{ 0x1e000, 0x1e006 },
	{ 0x1e008, 0x1e018 },
	{ 0x1e01b, 0x1e021 },
	{ 0x1e023, 0x1e024 },
	{ 0x1e026, 0x1e02a },
	{ 0x1e030, 0x1e06d },
	{ 0x1e08f, 0x1e08f },
	{ 0x1e100, 0x1e12c },
	{ 0x1e137, 0x1e13d },
	{ 0x1e14e, 0x1e14e },
	{ 0x1e290, 0x1e2ad },
	{ 0x1e2c0, 0x1e2eb },
	{ 0x1e4d0, 0x1e4eb },
	{ 0x1e5d0, 0x1e5ed },
	{ 0x1e5f0, 0x1e5f0 },
	{ 0x1e6c0, 0x1e6de },
	{ 0x1e6e0, 0x1e6f5 },
	{ 0x1e6fe, 0x1e6ff },
	{ 0x1e7e0, 0x1e7e6 },
	{ 0x1e7e8, 0x1e7eb },
	{ 0x1e7ed, 0x1e7ee },
	{ 0x1e7f0, 0x1e7fe },
	{ 0x1e800, 0x1e8c4 },
	{ 0x1e900, 0x1e943 },
	{ 0x1e947, 0x1e947 },
	{ 0x1e94b, 0x1e94b },
	{ 0x1ee00, 0x1ee03 },
	{ 0x1ee05, 0x1ee1f },
	{ 0x1ee21, 0x1ee22 },
	{ 0x1ee24, 0x1ee24 },
	{ 0x1ee27, 0x1ee27 },
	{ 0x1ee29, 0x1ee32 },
	{ 0x1ee34, 0x1ee37 },
	{ 0x1ee39, 0x1ee39 },
	{ 0x1ee3b, 0x1ee3b },
	{ 0x1ee42, 0x1ee42 },
	{ 0x1ee47, 0x1ee47 },
	{ 0x1ee49, 0x1ee49 },
	{ 0x1ee4b, 0x1ee4b },
	{ 0x1ee4d, 0x1ee4f },
	{ 0x1ee51, 0x1ee52 },
	{ 0x1ee54, 0x1ee54 },
	{ 0x1ee57, 0x1ee57 },
	{ 0x1ee59, 0x1ee59 },
	{ 0x1ee5b, 0x1ee5b },
	{ 0x1ee5d, 0x1ee5d },
	{ 0x1ee5f, 0x1ee5f },
	{ 0x1ee61, 0x1ee62 },
	{ 0x1ee64, 0x1ee64 },
	{ 0x1ee67, 0x1ee6a },
	{ 0x1ee6c, 0x1ee72 },
	{ 0x1ee74, 0x1ee77 },
	{ 0x1ee79, 0x1ee7c },
	{ 0x1ee7e, 0x1ee7e },
	{ 0x1ee80, 0x1ee89 },
	{ 0x1ee8b, 0x1ee9b },
	{ 0x1eea1, 0x1eea3 },
	{ 0x1eea5, 0x1eea9 },
	{ 0x1eeab, 0x1eebb },
	{ 0x1f130, 0x1f149 },
	{ 0x1f150, 0x1f169 },
	{ 0x1f170, 0x1f189 },
	{ 0x20000, 0x2a6df },
	{ 0x2a700, 0x2b81d },
	{ 0x2b820, 0x2cead },
	{ 0x2ceb0, 0x2ebe0 },
	{ 0x2ebf0, 0x2ee5d },
	{ 0x2f800, 0x2fa1d },
	{ 0x30000, 0x3134a },
	{ 0x31350, 0x33479 },
};


// --- Merged from core/string/translation.cpp ---
#include "core/os/thread.h"
#include "core/string/translation_server.h"
#include "core/templates/pair.h"
#include "core/types.h"
#include "core/string/translation_server.h"

extern "C"
{
#include <thirdparty/misc/smaz.h>
}

struct CompressedString
{
	int orig_len = 0;
	CharString compressed;
	int offset = 0;
};

void _check_for_incompatibility(const String& p_msgctxt, const String& p_msgid)
{
	// Gettext PO and MO files use an empty untranslated string without context
	// to store metadata.
	if (p_msgctxt.is_empty() && p_msgid.is_empty()) {
		WARN_PRINT("Both context and the untranslated string are empty. This may cause issues with "
				   "the translation system and external tools.");
	}

	// The EOT character (0x04) is used as a separator between context and
	// untranslated string in the MO file format. This convention is also used
	// by `get_message_list()`.
	//
	// It's unusual to have this character in the context or untranslated
	// string. But it doesn't do any harm as long as you are aware of this when
	// using the relevant APIs and tools.
	if (p_msgctxt.contains_char(0x04)) {
		WARN_PRINT(vformat("Found EOT character (0x04) within context '%s'. This may cause issues "
						   "with the translation system and external tools.",
			p_msgctxt));
	}
	if (p_msgid.contains_char(0x04)) {
		WARN_PRINT(vformat("Found EOT character (0x04) within untranslated string '%s'. This may "
						   "cause issues with the translation system and external tools.",
			p_msgid));
	}
}

bool Translation::generate(const Ref<Translation>& p_from)
{
	// This method compresses a Translation instance.
	// Right now, it doesn't handle context or plurals.
#ifdef TOOLS_ENABLED
	ERR_FAIL_COND_V(p_from.is_null(), false);

	List<String> keys;
	{
		List<String> raw_keys;
		p_from->get_message_list(&raw_keys);

		for (const String& key : raw_keys) {
			const String key_str = key;
			int p = key_str.find_char(0x04);
			if (p == -1) {
				keys.push_back(key);
			}
			else {
				const String& msgctxt = key_str.substr(0, p);
				const String& msgid = key_str.substr(p + 1);
				WARN_PRINT(vformat("Optimized translation does not support context, ignoring "
								   "message '%s' with context '%s'.",
					msgid, msgctxt));
			}
		}
	}

	int size = Math::larger_prime(keys.size());

	Vector<Vector<Pair<int, CharString>>> buckets;
	Vector<HashMap<uint32_t, int>> table;
	Vector<uint32_t> hfunc_table;
	Vector<CompressedString> compressed;

	table.resize(size);
	hfunc_table.resize(size);
	buckets.resize(size);
	compressed.resize(keys.size());

	int idx = 0;
	int total_compression_size = 0;

	for (const String& E : keys) {
		// hash string
		CharString cs = E.utf8();
		uint32_t h = hash(0, cs.get_data());
		Pair<int, CharString> p;
		p.first = idx;
		p.second = cs;
		buckets.write[h % size].push_back(p);

		// compress string
		CharString src_s = p_from->get_message(E).utf8();
		CompressedString ps;
		ps.orig_len = src_s.size();
		ps.offset = total_compression_size;

		if (ps.orig_len != 0) {
			CharString dst_s;
			dst_s.resize_uninitialized(src_s.size());
			int ret = smaz_compress(src_s.get_data(), src_s.size(), dst_s.ptrw(), src_s.size());
			if (ret >= src_s.size()) {
				// if compressed is larger than original, just use original
				ps.orig_len = src_s.size();
				ps.compressed = src_s;
			}
			else {
				dst_s.resize_uninitialized(ret);
				ps.compressed = dst_s;
			}
		}
		else {
			ps.orig_len = 1;
			ps.compressed.resize_uninitialized(1);
			ps.compressed[0] = 0;
		}

		compressed.write[idx] = ps;
		total_compression_size += ps.compressed.size();
		idx++;
	}

	int bucket_table_size = 0;

	for (int i = 0; i < size; i++) {
		const Vector<Pair<int, CharString>>& b = buckets[i];
		HashMap<uint32_t, int>& t = table.write[i];

		if (b.is_empty()) {
			continue;
		}

		int d = 1;
		int item = 0;

		while (item < b.size()) {
			uint32_t slot = hash(d, b[item].second.get_data());
			if (t.has(slot)) {
				item = 0;
				d++;
				t.clear();
			}
			else {
				t[slot] = b[item].first;
				item++;
			}
		}

		hfunc_table.write[i] = d;
		bucket_table_size += 2 + b.size() * 4;
	}

	ERR_FAIL_COND_V(bucket_table_size == 0, false);

	String from_locale = p_from->get_locale();

	hash_table.resize(size);
	bucket_table.resize(bucket_table_size);

	int* htwb = hash_table.ptrw();
	int* btwb = bucket_table.ptrw();

	uint32_t* htw = (uint32_t*)&htwb[0];
	uint32_t* btw = (uint32_t*)&btwb[0];

	int btindex = 0;

	for (int i = 0; i < size; i++) {
		const HashMap<uint32_t, int>& t = table[i];
		if (t.is_empty()) {
			htw[i] = 0xFFFFFFFF; // nothing
			continue;
		}

		htw[i] = btindex;
		btw[btindex++] = t.size();
		btw[btindex++] = hfunc_table[i];

		for (const KeyValue<uint32_t, int>& E : t) {
			btw[btindex++] = E.key;
			btw[btindex++] = compressed[E.value].offset;
			btw[btindex++] = compressed[E.value].compressed.size();
			btw[btindex++] = compressed[E.value].orig_len;
		}
	}

	strings.resize(total_compression_size);
	uint8_t* cw = strings.ptrw();

	for (int i = 0; i < compressed.size(); i++) {
		memcpy(&cw[compressed[i].offset], compressed[i].compressed.get_data(),
			compressed[i].compressed.size());
	}

	ERR_FAIL_COND_V(btindex != bucket_table_size, false);
	translation_map.clear();
	set_locale(from_locale);

	return true;
#else
	return false;
#endif
}

Vector<String> Translation::_get_message_list() const
{
	if (bucket_table.size() > 0) {
		WARN_PRINT_ONCE("Optimized translation does not store the message texts to be translated.");
		return {};
	}

	List<String> msgstrs;
	get_message_list(&msgstrs);

	Vector<String> keys;
	keys.resize(msgstrs.size());
	int idx = 0;
	for (const String& msgstr : msgstrs) {
		keys.write[idx++] = msgstr;
	}
	return keys;
}

Vector<String> Translation::get_translated_message_list() const
{
	if (bucket_table.size() > 0) {
		Vector<String> msgs;

		const int* htr = hash_table.ptr();
		const uint32_t* htptr = (const uint32_t*)&htr[0];
		const int* btr = bucket_table.ptr();
		const uint32_t* btptr = (const uint32_t*)&btr[0];
		const uint8_t* sr = strings.ptr();
		const char* sptr = (const char*)&sr[0];

		for (int i = 0; i < hash_table.size(); i++) {
			uint32_t p = htptr[i];
			if (p != 0xFFFFFFFF) {
				const Bucket& bucket = *(const Bucket*)&btptr[p];
				for (int j = 0; j < bucket.size; j++) {
					if (bucket.elem[j].comp_size == bucket.elem[j].uncomp_size) {
						String rstr = String::utf8(
							&sptr[bucket.elem[j].str_offset], bucket.elem[j].uncomp_size);
						msgs.push_back(rstr);
					}
					else {
						CharString uncomp;
						uncomp.resize_uninitialized(bucket.elem[j].uncomp_size + 1);
						smaz_decompress(&sptr[bucket.elem[j].str_offset], bucket.elem[j].comp_size,
							uncomp.ptrw(), bucket.elem[j].uncomp_size);
						String rstr = String::utf8(uncomp.get_data());
						msgs.push_back(rstr);
					}
				}
			}
		}
		return msgs;
	}

	Vector<String> msgstrs;
	for (const KeyValue<MessageKey, Vector<String>>& E : translation_map) {
		for (const String& msgstr : E.value) {
			msgstrs.push_back(msgstr);
		}
	}
	return msgstrs;
}

void Translation::set_locale(const String& p_locale)
{
	locale = TranslationServer::standardize_locale(p_locale);

	if (plural_rules_cache && plural_rules_override.is_empty()) {
		memdelete(plural_rules_cache);
		plural_rules_cache = nullptr;
	}
}

void Translation::add_message(
	const String& p_src_text, const String& p_xlated_text, const String& p_context)
{
	_check_for_incompatibility(p_context, p_src_text);
	translation_map[{p_context, p_src_text}] = {p_xlated_text};
}

void Translation::add_plural_message(const String& p_src_text,
	const Vector<String>& p_plural_xlated_texts, const String& p_context)
{
	ERR_FAIL_COND_MSG(p_plural_xlated_texts.is_empty(),
		"Parameter vector p_plural_xlated_texts passed in is empty.");

	Vector<String> msgstrs;
	msgstrs.resize(p_plural_xlated_texts.size());
	for (int i = 0; i < p_plural_xlated_texts.size(); i++) {
		msgstrs.write[i] = p_plural_xlated_texts[i];
	}

	_check_for_incompatibility(p_context, p_src_text);
	translation_map[{p_context, p_src_text}] = msgstrs;
}

String Translation::get_message(const String& p_src_text, const String& p_context) const
{
	if (bucket_table.size() > 0) {
		// p_context passed in is ignored in compressed mode.
		int htsize = hash_table.size();

		if (htsize == 0) {
			return String();
		}

		CharString str = p_src_text.utf8();
		uint32_t h = hash(0, str.get_data());

		const int* htr = hash_table.ptr();
		const uint32_t* htptr = (const uint32_t*)&htr[0];
		const int* btr = bucket_table.ptr();
		const uint32_t* btptr = (const uint32_t*)&btr[0];
		const uint8_t* sr = strings.ptr();
		const char* sptr = (const char*)&sr[0];

		uint32_t p = htptr[h % htsize];

		if (p == 0xFFFFFFFF) {
			return String(); // nothing
		}

		const Bucket& bucket = *(const Bucket*)&btptr[p];

		h = hash(bucket.func, str.get_data());

		int idx = -1;

		for (int i = 0; i < bucket.size; i++) {
			if (bucket.elem[i].key == h) {
				idx = i;
				break;
			}
		}

		if (idx == -1) {
			return String();
		}

		if (bucket.elem[idx].comp_size == bucket.elem[idx].uncomp_size) {
			return String::utf8(&sptr[bucket.elem[idx].str_offset], bucket.elem[idx].uncomp_size);
		}
		else {
			CharString uncomp;
			uncomp.resize_uninitialized(bucket.elem[idx].uncomp_size + 1);
			smaz_decompress(&sptr[bucket.elem[idx].str_offset], bucket.elem[idx].comp_size,
				uncomp.ptrw(), bucket.elem[idx].uncomp_size);
			return String::utf8(uncomp.get_data());
		}
	}

	const Vector<String>* msgstrs = translation_map.getptr({p_context, p_src_text});
	if (msgstrs == nullptr) {
		return String();
	}

	DEV_ASSERT(!msgstrs->is_empty()); // Should be prevented when adding messages.
	return msgstrs->get(0);
}

String Translation::get_plural_message(const String& p_src_text,
	const String& p_plural_text, int p_n, const String& p_context) const
{
	if (bucket_table.size() > 0) {
		// The use of plurals translation is not yet supported in compressed mode.
		return get_message(p_src_text, p_context);
	}

	ERR_FAIL_COND_V_MSG(p_n < 0, String(),
		"N passed into translation to get a plural message should not be negative. For negative "
		"numbers, use singular translation please. Search \"gettext PO Plural Forms\" online for "
		"details on translating negative numbers.");

	const Vector<String>* msgstrs = translation_map.getptr({p_context, p_src_text});
	if (msgstrs == nullptr) {
		return String();
	}

	const int index = _get_plural_rules()->evaluate(p_n);
	ERR_FAIL_INDEX_V_MSG(index, msgstrs->size(), String(),
		"Plural index returned or number of plural translations is not valid.");
	return msgstrs->get(index);
}

void Translation::erase_message(const String& p_src_text, const String& p_context)
{
	translation_map.erase({p_context, p_src_text});
}

void Translation::get_message_list(List<String>* r_messages) const
{
	if (bucket_table.size() > 0) {
		WARN_PRINT_ONCE("Optimized translation does not store the message texts to be translated.");
		return;
	}

	for (const KeyValue<MessageKey, Vector<String>>& E : translation_map) {
		if (E.key.msgctxt.is_empty()) {
			r_messages->push_back(E.key.msgid);
		}
		else {
			// Separated by the EOT character. Compatible with the MO file format.
			r_messages->push_back(vformat("%s\x04%s", E.key.msgctxt, E.key.msgid));
		}
	}
}

int Translation::get_message_count() const
{
	if (bucket_table.size() > 0) {
		WARN_PRINT_ONCE("Optimized translation does not store the message texts to be translated.");
		return 0;
	}

	return translation_map.size();
}

PluralRules* Translation::_get_plural_rules() const
{
	if (plural_rules_cache) {
		return plural_rules_cache;
	}

	if (!plural_rules_override.is_empty()) {
		plural_rules_cache = PluralRules::parse(plural_rules_override);
	}

	if (!plural_rules_cache) {
		// Locale's default plural rules.
		const String& default_rule = TranslationServer::get_plural_rules(locale);
		if (!default_rule.is_empty()) {
			plural_rules_cache = PluralRules::parse(default_rule);
		}

		// Use English plural rules as a fallback.
		if (!plural_rules_cache) {
			plural_rules_cache = PluralRules::parse("nplurals=2; plural=(n != 1);");
		}
	}

	DEV_ASSERT(plural_rules_cache != nullptr);
	return plural_rules_cache;
}

void Translation::set_plural_rules_override(const String& p_rules)
{
	plural_rules_override = p_rules;
	if (plural_rules_cache) {
		memdelete(plural_rules_cache);
		plural_rules_cache = nullptr;
	}
}

String Translation::get_plural_rules_override() const { return plural_rules_override; }

int Translation::get_nplurals() const { return _get_plural_rules()->get_nplurals(); }

Translation::~Translation()
{
	if (plural_rules_cache) {
		memdelete(plural_rules_cache);
		plural_rules_cache = nullptr;
	}
}

// ==========================================================================
// plural_rules.cpp
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

#include "core/types.h"
#include "core/string/translation_server.h"

int PluralRules::_find_unquoted(const String& p_src, char32_t p_chr) const
{
	const int len = p_src.length();
	if (len == 0) {
		return -1;
	}

	const char32_t* src = p_src.get_data();
	bool in_quote = false;
	for (int i = 0; i < len; i++) {
		if (in_quote) {
			if (src[i] == ')') {
				in_quote = false;
			}
		}
		else {
			if (src[i] == '(') {
				in_quote = true;
			}
			else if (src[i] == p_chr) {
				return i;
			}
		}
	}

	return -1;
}

void PluralRules::_cache_plural_tests(const String& p_plural_rule, Ref<EQNode>& p_node)
{
	// Some examples of p_plural_rule passed in can have the form:
	// "n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5"
	// (Arabic) "n >= 2" (French) // When evaluating the last, especially careful with this one. "n
	// != 1" (English)

	String rule = p_plural_rule;
	if (rule.begins_with("(") && rule.ends_with(")")) {
		int bcount = 0;
		for (int i = 1; i < rule.length() - 1 && bcount >= 0; i++) {
			if (rule[i] == '(') {
				bcount++;
			}
			else if (rule[i] == ')') {
				bcount--;
			}
		}
		if (bcount == 0) {
			rule = rule.substr(1, rule.length() - 2);
		}
	}

	int first_ques_mark = _find_unquoted(rule, '?');
	int first_colon = _find_unquoted(rule, ':');

	if (first_ques_mark == -1) {
		p_node->regex = rule.strip_edges();
		return;
	}

	p_node->regex = rule.substr(0, first_ques_mark).strip_edges();

	p_node->left.instantiate();
	_cache_plural_tests(
		rule.substr(first_ques_mark + 1, first_colon - first_ques_mark - 1).strip_edges(),
		p_node->left);
	p_node->right.instantiate();
	_cache_plural_tests(rule.substr(first_colon + 1).strip_edges(), p_node->right);
}

PluralRules::PluralRules(int p_nplurals, const String& p_plural)
	: nplurals(p_nplurals), plural(p_plural)
{
	equi_tests.instantiate();
	_cache_plural_tests(plural, equi_tests);

	expr.instantiate();
}

int PluralRules::evaluate(int p_n) const { return 0; }

PluralRules* PluralRules::parse(const String& p_rules)
{
	// `p_rules` should be in the format "nplurals=<N>; plural=<Expression>;".

	const int nplurals_eq = p_rules.find_char('=');
	ERR_FAIL_COND_V_MSG(nplurals_eq == -1, nullptr,
		"Invalid plural rules format. Missing equal sign for `nplurals`.");

	const int nplurals_semi_col = p_rules.find_char(';', nplurals_eq);
	ERR_FAIL_COND_V_MSG(nplurals_semi_col == -1, nullptr,
		"Invalid plural rules format. Missing semicolon for `nplurals`.");

	const String nplurals_str =
		p_rules.substr(nplurals_eq + 1, nplurals_semi_col - (nplurals_eq + 1)).strip_edges();
	ERR_FAIL_COND_V_MSG(!nplurals_str.is_valid_int(), nullptr,
		"Invalid plural rules format. `nplurals` should be an integer.");

	const int nplurals = nplurals_str.to_int();
	ERR_FAIL_COND_V_MSG(
		nplurals < 1, nullptr, "Invalid plural rules format. `nplurals` should be at least 1.");

	const int expression_eq = p_rules.find_char('=', nplurals_semi_col + 1);
	ERR_FAIL_COND_V_MSG(expression_eq == -1, nullptr,
		"Invalid plural rules format. Missing equal sign for `plural`.");

	int expression_end = p_rules.rfind_char(';');
	if (expression_end == -1) {
		WARN_PRINT("Invalid plural rules format. Missing semicolon at the end of `plural` "
				   "expression. Assuming ends at the end of the string.");
		expression_end = p_rules.length();
	}

	const int expression_start = expression_eq + 1;
	ERR_FAIL_COND_V_MSG(expression_end <= expression_start, nullptr,
		"Invalid plural rules format. `plural` expression is empty.");

	const String& plural =
		p_rules.substr(expression_start, expression_end - expression_start).strip_edges();
	return memnew(PluralRules(nplurals, plural));
}

// ==========================================================================
// translation_domain.cpp
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

#include "core/string/translation_server.h"

struct _character_accent_pair
{
	const char32_t character;
	const char32_t* accented_character;
};

static _character_accent_pair _character_to_accented[] = {
	{'A', U"Å"},
	{'B', U"ß"},
	{'C', U"Ç"},
	{'D', U"Ð"},
	{'E', U"É"},
	{'F', U"F́"},
	{'G', U"Ĝ"},
	{'H', U"Ĥ"},
	{'I', U"Ĩ"},
	{'J', U"Ĵ"},
	{'K', U"ĸ"},
	{'L', U"Ł"},
	{'M', U"Ḿ"},
	{'N', U"й"},
	{'O', U"Ö"},
	{'P', U"Ṕ"},
	{'Q', U"Q́"},
	{'R', U"Ř"},
	{'S', U"Ŝ"},
	{'T', U"Ŧ"},
	{'U', U"Ũ"},
	{'V', U"Ṽ"},
	{'W', U"Ŵ"},
	{'X', U"X́"},
	{'Y', U"Ÿ"},
	{'Z', U"Ž"},
	{'a', U"á"},
	{'b', U"ḅ"},
	{'c', U"ć"},
	{'d', U"d́"},
	{'e', U"é"},
	{'f', U"f́"},
	{'g', U"ǵ"},
	{'h', U"h̀"},
	{'i', U"í"},
	{'j', U"ǰ"},
	{'k', U"ḱ"},
	{'l', U"ł"},
	{'m', U"m̀"},
	{'n', U"ή"},
	{'o', U"ô"},
	{'p', U"ṕ"},
	{'q', U"q́"},
	{'r', U"ŕ"},
	{'s', U"š"},
	{'t', U"ŧ"},
	{'u', U"ü"},
	{'v', U"ṽ"},
	{'w', U"ŵ"},
	{'x', U"x́"},
	{'y', U"ý"},
	{'z', U"ź"},
};

String TranslationDomain::_get_override_string(const String& p_message) const
{
	String res;
	for (int i = 0; i < p_message.length(); i++) {
		if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += p_message[i];
			res += p_message[i + 1];
			i++;
			continue;
		}
		res += '*';
	}
	return res;
}

String TranslationDomain::_double_vowels(const String& p_message) const
{
	String res;
	for (int i = 0; i < p_message.length(); i++) {
		if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += p_message[i];
			res += p_message[i + 1];
			i++;
			continue;
		}
		res += p_message[i];
		if (p_message[i] == 'a' || p_message[i] == 'e' || p_message[i] == 'i' ||
			p_message[i] == 'o' || p_message[i] == 'u' || p_message[i] == 'A' ||
			p_message[i] == 'E' || p_message[i] == 'I' || p_message[i] == 'O' ||
			p_message[i] == 'U') {
			res += p_message[i];
		}
	}
	return res;
}

String TranslationDomain::_replace_with_accented_string(const String& p_message) const
{
	String res;
	for (int i = 0; i < p_message.length(); i++) {
		if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += p_message[i];
			res += p_message[i + 1];
			i++;
			continue;
		}
		const char32_t* accented = _get_accented_version(p_message[i]);
		if (accented) {
			res += accented;
		}
		else {
			res += p_message[i];
		}
	}
	return res;
}

String TranslationDomain::_wrap_with_fakebidi_characters(const String& p_message) const
{
	String res;
	char32_t fakebidiprefix = U'\u202e';
	char32_t fakebidisuffix = U'\u202c';
	res += fakebidiprefix;
	// The fake bidi unicode gets popped at every newline so pushing it back at every newline.
	for (int i = 0; i < p_message.length(); i++) {
		if (p_message[i] == '\n') {
			res += fakebidisuffix;
			res += p_message[i];
			res += fakebidiprefix;
		}
		else if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += fakebidisuffix;
			res += p_message[i];
			res += p_message[i + 1];
			res += fakebidiprefix;
			i++;
		}
		else {
			res += p_message[i];
		}
	}
	res += fakebidisuffix;
	return res;
}

String TranslationDomain::_add_padding(const String& p_message, int p_length) const
{
	String underscores = String("_").repeat(p_length * pseudolocalization.expansion_ratio / 2);
	String prefix = pseudolocalization.prefix + underscores;
	String suffix = underscores + pseudolocalization.suffix;

	return prefix + p_message + suffix;
}

const char32_t* TranslationDomain::_get_accented_version(char32_t p_character) const
{
	if (!is_ascii_alphabet_char(p_character)) {
		return nullptr;
	}

	for (unsigned int i = 0; i < std_size(_character_to_accented); i++) {
		if (_character_to_accented[i].character == p_character) {
			return _character_to_accented[i].accented_character;
		}
	}

	return nullptr;
}

bool TranslationDomain::_is_placeholder(const String& p_message, int p_index) const
{
	return p_index < p_message.length() - 1 && p_message[p_index] == '%' &&
		   (p_message[p_index + 1] == 's' || p_message[p_index + 1] == 'c' ||
			   p_message[p_index + 1] == 'd' || p_message[p_index + 1] == 'o' ||
			   p_message[p_index + 1] == 'x' || p_message[p_index + 1] == 'X' ||
			   p_message[p_index + 1] == 'f');
}

String TranslationDomain::get_message_from_translations(
	const String& p_locale, const String& p_message, const String& p_context) const
{
	String res;
	int best_score = 0;

	for (const Ref<Translation>& E : translations) {
		int score = TranslationServer::compare_locales(p_locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			const String r = E->get_message(p_message, p_context);
			if (r.is_empty()) {
				continue;
			}
			res = r;
			best_score = score;
			if (score == 10) {
				break; // Exact match, skip the rest.
			}
		}
	}

	return res;
}

String TranslationDomain::get_message_from_translations(const String& p_locale,
	const String& p_message, const String& p_message_plural, int p_n,
	const String& p_context) const
{
	String res;
	int best_score = 0;

	for (const Ref<Translation>& E : translations) {
		int score = TranslationServer::compare_locales(p_locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			const String r = E->get_plural_message(p_message, p_message_plural, p_n, p_context);
			if (r.is_empty()) {
				continue;
			}
			res = r;
			best_score = score;
			if (score == 10) {
				break; // Exact match, skip the rest.
			}
		}
	}

	return res;
}

PackedStringArray TranslationDomain::get_loaded_locales() const
{
	PackedStringArray locales;
	for (const Ref<Translation>& E : translations) {
		const String& locale = E->get_locale();
		if (!locales.has(locale)) {
			locales.push_back(locale);
		}
	}
	return locales;
}

#ifndef DISABLE_DEPRECATED
Ref<Translation> TranslationDomain::get_translation_object(const String& p_locale) const
{
	Ref<Translation> res;
	int best_score = 0;

	for (const Ref<Translation>& E : translations) {
		int score = TranslationServer::compare_locales(p_locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			res = E;
			best_score = score;
			if (score == 10) {
				break; // Exact match, skip the rest.
			}
		}
	}
	return res;
}
#endif

void TranslationDomain::add_translation(const Ref<Translation>& p_translation)
{
	ERR_FAIL_COND_MSG(p_translation.is_null(), "Invalid translation provided.");
	translations.insert(p_translation);
}

void TranslationDomain::remove_translation(const Ref<Translation>& p_translation)
{
	translations.erase(p_translation);
}

void TranslationDomain::clear() { translations.clear(); }

const HashSet<Ref<Translation>> TranslationDomain::get_translations() const
{
	return HashSet<Ref<Translation>>(translations);
}

HashSet<Ref<Translation>> TranslationDomain::find_translations(
	const String& p_locale, bool p_exact) const
{
	HashSet<Ref<Translation>> res;
	if (p_exact) {
		for (const Ref<Translation>& E : translations) {
			if (E->get_locale() == p_locale) {
				res.insert(E);
			}
		}
	}
	else {
		for (const Ref<Translation>& E : translations) {
			if (TranslationServer::compare_locales(p_locale, E->get_locale()) >
				0) {
				res.insert(E);
			}
		}
	}
	return res;
}

bool TranslationDomain::has_translation(const Ref<Translation>& p_translation) const
{
	return translations.has(p_translation);
}

bool TranslationDomain::has_translation_for_locale(const String& p_locale, bool p_exact) const
{
	if (p_exact) {
		for (const Ref<Translation>& E : translations) {
			if (E->get_locale() == p_locale) {
				return true;
			}
		}
	}
	else {
		for (const Ref<Translation>& E : translations) {
			if (TranslationServer::compare_locales(p_locale, E->get_locale()) >
				0) {
				return true;
			}
		}
	}
	return false;
}

String TranslationDomain::translate(
	const String& p_message, const String& p_context) const
{
	if (!enabled) {
		return p_message;
	}

	const String& locale = locale_override.is_empty()
							   ? TranslationServer::get_locale()
							   : locale_override;
	String res = get_message_from_translations(locale, p_message, p_context);

	const bool& allow_fallback = TranslationServer::is_fallback_allowed();
	const String& fallback = TranslationServer::get_fallback_locale();
	if (res.is_empty() && allow_fallback && fallback.length() >= 2) {
		res = get_message_from_translations(fallback, p_message, p_context);
	}

	if (res.is_empty()) {
		return pseudolocalization.enabled ? pseudolocalize(p_message) : p_message;
	}
	return pseudolocalization.enabled ? pseudolocalize(res) : res;
}

String TranslationDomain::translate_plural(const String& p_message,
	const String& p_message_plural, int p_n, const String& p_context) const
{
	if (!enabled) {
		return p_n == 1 ? p_message : p_message_plural;
	}

	const String& locale = locale_override.is_empty()
							   ? TranslationServer::get_locale()
							   : locale_override;
	String res =
		get_message_from_translations(locale, p_message, p_message_plural, p_n, p_context);

	const bool& allow_fallback = TranslationServer::is_fallback_allowed();
	const String& fallback = TranslationServer::get_fallback_locale();
	if (res.is_empty() && allow_fallback && fallback.length() >= 2) {
		res = get_message_from_translations(fallback, p_message, p_message_plural, p_n, p_context);
	}

	if (res.is_empty()) {
		if (p_n == 1) {
			return p_message;
		}
		return p_message_plural;
	}
	return res;
}

String TranslationDomain::get_locale_override() const { return locale_override; }

void TranslationDomain::set_locale_override(const String& p_locale)
{
	locale_override = p_locale.is_empty()
						  ? p_locale
						  : TranslationServer::standardize_locale(p_locale);
}

bool TranslationDomain::is_enabled() const { return enabled; }

void TranslationDomain::set_enabled(bool p_enabled) { enabled = p_enabled; }

bool TranslationDomain::is_pseudolocalization_enabled() const { return pseudolocalization.enabled; }

void TranslationDomain::set_pseudolocalization_enabled(bool p_enabled)
{
	pseudolocalization.enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_accents_enabled() const
{
	return pseudolocalization.accents_enabled;
}

void TranslationDomain::set_pseudolocalization_accents_enabled(bool p_enabled)
{
	pseudolocalization.accents_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_double_vowels_enabled() const
{
	return pseudolocalization.double_vowels_enabled;
}

void TranslationDomain::set_pseudolocalization_double_vowels_enabled(bool p_enabled)
{
	pseudolocalization.double_vowels_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_fake_bidi_enabled() const
{
	return pseudolocalization.fake_bidi_enabled;
}

void TranslationDomain::set_pseudolocalization_fake_bidi_enabled(bool p_enabled)
{
	pseudolocalization.fake_bidi_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_override_enabled() const
{
	return pseudolocalization.override_enabled;
}

void TranslationDomain::set_pseudolocalization_override_enabled(bool p_enabled)
{
	pseudolocalization.override_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_skip_placeholders_enabled() const
{
	return pseudolocalization.skip_placeholders_enabled;
}

void TranslationDomain::set_pseudolocalization_skip_placeholders_enabled(bool p_enabled)
{
	pseudolocalization.skip_placeholders_enabled = p_enabled;
}

float TranslationDomain::get_pseudolocalization_expansion_ratio() const
{
	return pseudolocalization.expansion_ratio;
}

void TranslationDomain::set_pseudolocalization_expansion_ratio(float p_ratio)
{
	pseudolocalization.expansion_ratio = p_ratio;
}

String TranslationDomain::get_pseudolocalization_prefix() const
{
	return pseudolocalization.prefix;
}

void TranslationDomain::set_pseudolocalization_prefix(const String& p_prefix)
{
	pseudolocalization.prefix = p_prefix;
}

String TranslationDomain::get_pseudolocalization_suffix() const
{
	return pseudolocalization.suffix;
}

void TranslationDomain::set_pseudolocalization_suffix(const String& p_suffix)
{
	pseudolocalization.suffix = p_suffix;
}

String TranslationDomain::pseudolocalize(const String& p_message) const
{
	if (p_message.is_empty()) {
		return p_message;
	}

	String message = p_message;
	int length = message.length();
	if (pseudolocalization.override_enabled) {
		message = _get_override_string(message);
	}

	if (pseudolocalization.double_vowels_enabled) {
		message = _double_vowels(message);
	}

	if (pseudolocalization.accents_enabled) {
		message = _replace_with_accented_string(message);
	}

	if (pseudolocalization.fake_bidi_enabled) {
		message = _wrap_with_fakebidi_characters(message);
	}

	return _add_padding(message, length);
}

// ==========================================================================
// translation_server.cpp
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
// --- Inlined from core/string/locales.h ---
// Windows has some weird locale identifiers which do not honor the ISO 639-1
// standardized nomenclature. Whenever those don't conflict with existing ISO
// identifiers, we override them.
//
// Reference:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms693062(v=vs.85).aspx

static const char *locale_renames[][2] = {
	{ "in", "id" }, //  Indonesian
	{ "iw", "he" }, //  Hebrew
	{ "no", "nb" }, //  Norwegian Bokmål
	{ "C", "en" }, // Locale is not set, fallback to English.
	{ nullptr, nullptr }
};

// Additional script information to preferred scripts.
// Language code, script code, default country, supported countries.
// Reference:
// - https://lh.2xlibre.net/locales/
// - https://www.localeplanet.com/icu/index.html
// - https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f

static const char *locale_scripts[][4] = {
	{ "az", "Latn", "", "AZ" },
	{ "az", "Arab", "", "IR" },
	{ "bs", "Latn", "", "BA" },
	{ "ff", "Latn", "", "BF,CM,GH,GM,GN,GW,LR,MR,NE,NG,SL,SN" },
	{ "pa", "Arab", "PK", "PK" },
	{ "pa", "Guru", "IN", "IN" },
	{ "sd", "Arab", "PK", "PK" },
	{ "sd", "Deva", "IN", "IN" },
	{ "shi", "Tfng", "", "MA" },
	{ "sr", "Cyrl", "", "BA,RS,XK" },
	{ "sr", "Latn", "", "ME" },
	{ "uz", "Latn", "", "UZ" },
	{ "uz", "Arab", "AF", "AF" },
	{ "vai", "Vaii", "", "LR" },
	{ "yue", "Hans", "CN", "CN" },
	{ "yue", "Hant", "HK", "HK" },
	{ "zh", "Hans", "CN", "CN,SG" },
	{ "zh", "Hant", "TW", "HK,MO,TW" },
	{ nullptr, nullptr, nullptr, nullptr }
};

// Additional mapping for outdated, temporary or exceptionally reserved country codes.
// Reference:
// - https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
// - https://www.iso.org/obp/ui/#search/code/

static const char *country_renames[][2] = {
	{ "BU", "MM" }, // Burma, name changed to Myanmar.
	{ "KV", "XK" }, // Kosovo (temporary FIPS code to European Commission code), no official ISO code assigned.
	{ "TP", "TL" }, // East Timor, name changed to Timor-Leste.
	{ "UK", "GB" }, // United Kingdom, exceptionally reserved code.
	{ nullptr, nullptr }
};

// Country code, country name.
// Reference:
// - https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
// - https://www.iso.org/obp/ui/#search/code/

static const char *country_names[][2] = {
	{ "AC", "Ascension Island" }, // Exceptionally reserved.
	{ "AD", "Andorra" },
	{ "AE", "United Arab Emirates" },
	{ "AF", "Afghanistan" },
	{ "AG", "Antigua and Barbuda" },
	{ "AI", "Anguilla" },
	{ "AL", "Albania" },
	{ "AM", "Armenia" },
	{ "AN", "Netherlands Antilles" }, // Transitionally reserved, divided into BQ, CW and SX.
	{ "AO", "Angola" },
	{ "AQ", "Antarctica" },
	{ "AR", "Argentina" },
	{ "AS", "American Samoa" },
	{ "AT", "Austria" },
	{ "AU", "Australia" },
	{ "AW", "Aruba" },
	{ "AX", "Åland Islands" },
	{ "AZ", "Azerbaijan" },
	{ "BA", "Bosnia and Herzegovina" },
	{ "BB", "Barbados" },
	{ "BD", "Bangladesh" },
	{ "BE", "Belgium" },
	{ "BF", "Burkina Faso" },
	{ "BG", "Bulgaria" },
	{ "BH", "Bahrain" },
	{ "BI", "Burundi" },
	{ "BJ", "Benin" },
	{ "BL", "St. Barthélemy" },
	{ "BM", "Bermuda" },
	{ "BN", "Brunei" },
	{ "BO", "Bolivia" },
	{ "BQ", "Caribbean Netherlands" },
	{ "BR", "Brazil" },
	{ "BS", "Bahamas" },
	{ "BT", "Bhutan" },
	{ "BV", "Bouvet Island" },
	{ "BW", "Botswana" },
	{ "BY", "Belarus" },
	{ "BZ", "Belize" },
	{ "CA", "Canada" },
	{ "CC", "Cocos (Keeling) Islands" },
	{ "CD", "Congo - Kinshasa" },
	{ "CF", "Central African Republic" },
	{ "CG", "Congo - Brazzaville" },
	{ "CH", "Switzerland" },
	{ "CI", "Côte d'Ivoire" },
	{ "CK", "Cook Islands" },
	{ "CL", "Chile" },
	{ "CM", "Cameroon" },
	{ "CN", "China" },
	{ "CO", "Colombia" },
	{ "CP", "Clipperton Island" }, // Exceptionally reserved.
	{ "CR", "Costa Rica" },
	{ "CQ", "Island of Sark" }, // Exceptionally reserved.
	{ "CU", "Cuba" },
	{ "CV", "Cabo Verde" },
	{ "CW", "Curaçao" },
	{ "CX", "Christmas Island" },
	{ "CY", "Cyprus" },
	{ "CZ", "Czechia" },
	{ "DE", "Germany" },
	{ "DG", "Diego Garcia" }, // Exceptionally reserved.
	{ "DJ", "Djibouti" },
	{ "DK", "Denmark" },
	{ "DM", "Dominica" },
	{ "DO", "Dominican Republic" },
	{ "DZ", "Algeria" },
	{ "EA", "Ceuta and Melilla" }, // Exceptionally reserved.
	{ "EC", "Ecuador" },
	{ "EE", "Estonia" },
	{ "EG", "Egypt" },
	{ "EH", "Western Sahara" },
	{ "ER", "Eritrea" },
	{ "ES", "Spain" },
	{ "ET", "Ethiopia" },
	{ "EU", "European Union" }, // Exceptionally reserved.
	{ "EZ", "Eurozone" }, // Exceptionally reserved.
	{ "FI", "Finland" },
	{ "FJ", "Fiji" },
	{ "FK", "Falkland Islands" },
	{ "FM", "Micronesia" },
	{ "FO", "Faroe Islands" },
	{ "FR", "France" },
	{ "FX", "France, Metropolitan" }, // Exceptionally reserved.
	{ "GA", "Gabon" },
	{ "GB", "United Kingdom" },
	{ "GD", "Grenada" },
	{ "GE", "Georgia" },
	{ "GF", "French Guiana" },
	{ "GG", "Guernsey" },
	{ "GH", "Ghana" },
	{ "GI", "Gibraltar" },
	{ "GL", "Greenland" },
	{ "GM", "Gambia" },
	{ "GN", "Guinea" },
	{ "GP", "Guadeloupe" },
	{ "GQ", "Equatorial Guinea" },
	{ "GR", "Greece" },
	{ "GS", "South Georgia and South Sandwich Islands" },
	{ "GT", "Guatemala" },
	{ "GU", "Guam" },
	{ "GW", "Guinea-Bissau" },
	{ "GY", "Guyana" },
	{ "HK", "Hong Kong" },
	{ "HM", "Heard Island and McDonald Islands" },
	{ "HN", "Honduras" },
	{ "HR", "Croatia" },
	{ "HT", "Haiti" },
	{ "HU", "Hungary" },
	{ "IC", "Canary Islands" }, // Exceptionally reserved.
	{ "ID", "Indonesia" },
	{ "IE", "Ireland" },
	{ "IL", "Israel" },
	{ "IM", "Isle of Man" },
	{ "IN", "India" },
	{ "IO", "British Indian Ocean Territory" },
	{ "IQ", "Iraq" },
	{ "IR", "Iran" },
	{ "IS", "Iceland" },
	{ "IT", "Italy" },
	{ "JE", "Jersey" },
	{ "JM", "Jamaica" },
	{ "JO", "Jordan" },
	{ "JP", "Japan" },
	{ "KE", "Kenya" },
	{ "KG", "Kyrgyzstan" },
	{ "KH", "Cambodia" },
	{ "KI", "Kiribati" },
	{ "KM", "Comoros" },
	{ "KN", "St. Kitts and Nevis" },
	{ "KP", "North Korea" },
	{ "KR", "South Korea" },
	{ "KW", "Kuwait" },
	{ "KY", "Cayman Islands" },
	{ "KZ", "Kazakhstan" },
	{ "LA", "Laos" },
	{ "LB", "Lebanon" },
	{ "LC", "St. Lucia" },
	{ "LI", "Liechtenstein" },
	{ "LK", "Sri Lanka" },
	{ "LR", "Liberia" },
	{ "LS", "Lesotho" },
	{ "LT", "Lithuania" },
	{ "LU", "Luxembourg" },
	{ "LV", "Latvia" },
	{ "LY", "Libya" },
	{ "MA", "Morocco" },
	{ "MC", "Monaco" },
	{ "MD", "Moldova" },
	{ "ME", "Montenegro" },
	{ "MF", "St. Martin" },
	{ "MG", "Madagascar" },
	{ "MH", "Marshall Islands" },
	{ "MK", "North Macedonia" },
	{ "ML", "Mali" },
	{ "MM", "Myanmar" },
	{ "MN", "Mongolia" },
	{ "MO", "Macao" },
	{ "MP", "Northern Mariana Islands" },
	{ "MQ", "Martinique" },
	{ "MR", "Mauritania" },
	{ "MS", "Montserrat" },
	{ "MT", "Malta" },
	{ "MU", "Mauritius" },
	{ "MV", "Maldives" },
	{ "MW", "Malawi" },
	{ "MX", "Mexico" },
	{ "MY", "Malaysia" },
	{ "MZ", "Mozambique" },
	{ "NA", "Namibia" },
	{ "NC", "New Caledonia" },
	{ "NE", "Niger" },
	{ "NF", "Norfolk Island" },
	{ "NG", "Nigeria" },
	{ "NI", "Nicaragua" },
	{ "NL", "Netherlands" },
	{ "NO", "Norway" },
	{ "NP", "Nepal" },
	{ "NR", "Nauru" },
	{ "NU", "Niue" },
	{ "NZ", "New Zealand" },
	{ "OM", "Oman" },
	{ "PA", "Panama" },
	{ "PE", "Peru" },
	{ "PF", "French Polynesia" },
	{ "PG", "Papua New Guinea" },
	{ "PH", "Philippines" },
	{ "PK", "Pakistan" },
	{ "PL", "Poland" },
	{ "PM", "St. Pierre and Miquelon" },
	{ "PN", "Pitcairn Islands" },
	{ "PR", "Puerto Rico" },
	{ "PS", "Palestine" },
	{ "PT", "Portugal" },
	{ "PW", "Palau" },
	{ "PY", "Paraguay" },
	{ "QA", "Qatar" },
	{ "RE", "Réunion" },
	{ "RO", "Romania" },
	{ "RS", "Serbia" },
	{ "RU", "Russia" },
	{ "RW", "Rwanda" },
	{ "SA", "Saudi Arabia" },
	{ "SB", "Solomon Islands" },
	{ "SC", "Seychelles" },
	{ "SD", "Sudan" },
	{ "SE", "Sweden" },
	{ "SG", "Singapore" },
	{ "SH", "St. Helena, Ascension and Tristan da Cunha" },
	{ "SI", "Slovenia" },
	{ "SJ", "Svalbard and Jan Mayen" },
	{ "SK", "Slovakia" },
	{ "SL", "Sierra Leone" },
	{ "SM", "San Marino" },
	{ "SN", "Senegal" },
	{ "SO", "Somalia" },
	{ "SR", "Suriname" },
	{ "SS", "South Sudan" },
	{ "ST", "Sao Tome and Principe" },
	{ "SV", "El Salvador" },
	{ "SX", "Sint Maarten" },
	{ "SY", "Syria" },
	{ "SZ", "Eswatini" },
	{ "TA", "Tristan da Cunha" }, // Exceptionally reserved.
	{ "TC", "Turks and Caicos Islands" },
	{ "TD", "Chad" },
	{ "TF", "French Southern Territories" },
	{ "TG", "Togo" },
	{ "TH", "Thailand" },
	{ "TJ", "Tajikistan" },
	{ "TK", "Tokelau" },
	{ "TL", "Timor-Leste" },
	{ "TM", "Turkmenistan" },
	{ "TN", "Tunisia" },
	{ "TO", "Tonga" },
	{ "TR", "Turkey" },
	{ "TT", "Trinidad and Tobago" },
	{ "TV", "Tuvalu" },
	{ "TW", "Taiwan" },
	{ "TZ", "Tanzania" },
	{ "UA", "Ukraine" },
	{ "UG", "Uganda" },
	{ "UM", "U.S. Outlying Islands" },
	{ "US", "United States of America" },
	{ "UY", "Uruguay" },
	{ "UZ", "Uzbekistan" },
	{ "VA", "Holy See" },
	{ "VC", "St. Vincent and the Grenadines" },
	{ "VE", "Venezuela" },
	{ "VG", "British Virgin Islands" },
	{ "VI", "U.S. Virgin Islands" },
	{ "VN", "Viet Nam" },
	{ "VU", "Vanuatu" },
	{ "WF", "Wallis and Futuna" },
	{ "WS", "Samoa" },
	{ "XK", "Kosovo" }, // Temporary code, no official ISO code assigned.
	{ "YE", "Yemen" },
	{ "YT", "Mayotte" },
	{ "ZA", "South Africa" },
	{ "ZM", "Zambia" },
	{ "ZW", "Zimbabwe" },
	{ nullptr, nullptr }
};

// Languages code, language name.
// Reference:
// - https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
// - https://www.localeplanet.com/icu/index.html
// - https://lh.2xlibre.net/locales/

static const char *language_list[][2] = {
	{ "aa", "Afar" },
	{ "ab", "Abkhazian" },
	{ "ace", "Achinese" },
	{ "ach", "Acoli" },
	{ "ada", "Adangme" },
	{ "ady", "Adyghe" },
	{ "ae", "Avestan" },
	{ "aeb", "Tunisian Arabic" },
	{ "af", "Afrikaans" },
	{ "afh", "Afrihili" },
	{ "agq", "Aghem" },
	{ "ain", "Ainu" },
	{ "agr", "Aguaruna" },
	{ "ak", "Akan" },
	{ "akk", "Akkadian" },
	{ "akz", "Alabama" },
	{ "ale", "Aleut" },
	{ "aln", "Gheg Albanian" },
	{ "alt", "Southern Altai" },
	{ "am", "Amharic" },
	{ "an", "Aragonese" },
	{ "ang", "Old English" },
	{ "anp", "Angika" },
	{ "ar", "Arabic" },
	{ "arc", "Aramaic" },
	{ "arn", "Mapudungun" },
	{ "aro", "Araona" },
	{ "arp", "Arapaho" },
	{ "arq", "Algerian Arabic" },
	{ "ars", "Najdi Arabic" },
	{ "arw", "Arawak" },
	{ "ary", "Moroccan Arabic" },
	{ "arz", "Egyptian Arabic" },
	{ "as", "Assamese" },
	{ "asa", "Asu" },
	{ "ase", "American Sign Language" },
	{ "ast", "Asturian" },
	{ "av", "Avaric" },
	{ "avk", "Kotava" },
	{ "awa", "Awadhi" },
	{ "ayc", "Southern Aymara" },
	{ "ay", "Aymara" },
	{ "az", "Azerbaijani" },
	{ "ba", "Bashkir" },
	{ "bal", "Baluchi" },
	{ "ban", "Balinese" },
	{ "bar", "Bavarian" },
	{ "bas", "Bassa" },
	{ "bax", "Bamun" },
	{ "bbc", "Batak Toba" },
	{ "bbj", "Ghomala" },
	{ "be", "Belarusian" },
	{ "bej", "Beja" },
	{ "bem", "Bemba" },
	{ "ber", "Berber" },
	{ "bew", "Betawi" },
	{ "bez", "Bena" },
	{ "bfd", "Bafut" },
	{ "bfq", "Badaga" },
	{ "bg", "Bulgarian" },
	{ "bhb", "Bhili" },
	{ "bgn", "Western Balochi" },
	{ "bho", "Bhojpuri" },
	{ "bi", "Bislama" },
	{ "bik", "Bikol" },
	{ "bin", "Bini" },
	{ "bjn", "Banjar" },
	{ "bkm", "Kom" },
	{ "bla", "Siksika" },
	{ "bm", "Bambara" },
	{ "bn", "Bengali" },
	{ "bo", "Tibetan" },
	{ "bpy", "Bishnupriya" },
	{ "bqi", "Bakhtiari" },
	{ "br", "Breton" },
	{ "brh", "Brahui" },
	{ "brx", "Bodo" },
	{ "bs", "Bosnian" },
	{ "bss", "Akoose" },
	{ "bua", "Buriat" },
	{ "bug", "Buginese" },
	{ "bum", "Bulu" },
	{ "byn", "Bilin" },
	{ "byv", "Medumba" },
	{ "ca", "Catalan" },
	{ "cad", "Caddo" },
	{ "car", "Carib" },
	{ "cay", "Cayuga" },
	{ "cch", "Atsam" },
	{ "ccp", "Chakma" },
	{ "ce", "Chechen" },
	{ "ceb", "Cebuano" },
	{ "cgg", "Chiga" },
	{ "ch", "Chamorro" },
	{ "chb", "Chibcha" },
	{ "chg", "Chagatai" },
	{ "chk", "Chuukese" },
	{ "chm", "Mari" },
	{ "chn", "Chinook Jargon" },
	{ "cho", "Choctaw" },
	{ "chp", "Chipewyan" },
	{ "chr", "Cherokee" },
	{ "chy", "Cheyenne" },
	{ "cic", "Chickasaw" },
	{ "ckb", "Central Kurdish" },
	{ "cmn", "Mandarin Chinese" },
	{ "co", "Corsican" },
	{ "cop", "Coptic" },
	{ "cps", "Capiznon" },
	{ "cr", "Cree" },
	{ "crh", "Crimean Tatar" },
	{ "crs", "Seselwa Creole French" },
	{ "cs", "Czech" },
	{ "csb", "Kashubian" },
	{ "cu", "Church Slavic" },
	{ "cv", "Chuvash" },
	{ "cy", "Welsh" },
	{ "da", "Danish" },
	{ "dak", "Dakota" },
	{ "dar", "Dargwa" },
	{ "dav", "Taita" },
	{ "de", "German" },
	{ "del", "Delaware" },
	{ "den", "Slave" },
	{ "dgr", "Dogrib" },
	{ "din", "Dinka" },
	{ "dje", "Zarma" },
	{ "doi", "Dogri" },
	{ "dsb", "Lower Sorbian" },
	{ "dtp", "Central Dusun" },
	{ "dua", "Duala" },
	{ "dum", "Middle Dutch" },
	{ "dv", "Dhivehi" },
	{ "dyo", "Jola-Fonyi" },
	{ "dyu", "Dyula" },
	{ "dz", "Dzongkha" },
	{ "dzg", "Dazaga" },
	{ "ebu", "Embu" },
	{ "ee", "Ewe" },
	{ "efi", "Efik" },
	{ "egl", "Emilian" },
	{ "egy", "Ancient Egyptian" },
	{ "eka", "Ekajuk" },
	{ "el", "Greek" },
	{ "elx", "Elamite" },
	{ "en", "English" },
	{ "enm", "Middle English" },
	{ "eo", "Esperanto" },
	{ "es", "Spanish" },
	{ "esu", "Central Yupik" },
	{ "et", "Estonian" },
	{ "eu", "Basque" },
	{ "ewo", "Ewondo" },
	{ "ext", "Extremaduran" },
	{ "fa", "Persian" },
	{ "fan", "Fang" },
	{ "fat", "Fanti" },
	{ "ff", "Fulah" },
	{ "fi", "Finnish" },
	{ "fil", "Filipino" },
	{ "fit", "Tornedalen Finnish" },
	{ "fj", "Fijian" },
	{ "fo", "Faroese" },
	{ "fon", "Fon" },
	{ "fr", "French" },
	{ "frc", "Cajun French" },
	{ "frm", "Middle French" },
	{ "fro", "Old French" },
	{ "frp", "Arpitan" },
	{ "frr", "Northern Frisian" },
	{ "frs", "Eastern Frisian" },
	{ "fur", "Friulian" },
	{ "fy", "Western Frisian" },
	{ "ga", "Irish" },
	{ "gaa", "Ga" },
	{ "gag", "Gagauz" },
	{ "gan", "Gan Chinese" },
	{ "gay", "Gayo" },
	{ "gba", "Gbaya" },
	{ "gbz", "Zoroastrian Dari" },
	{ "gd", "Scottish Gaelic" },
	{ "gez", "Geez" },
	{ "gil", "Gilbertese" },
	{ "gl", "Galician" },
	{ "glk", "Gilaki" },
	{ "gmh", "Middle High German" },
	{ "gn", "Guarani" },
	{ "goh", "Old High German" },
	{ "gom", "Goan Konkani" },
	{ "gon", "Gondi" },
	{ "gor", "Gorontalo" },
	{ "got", "Gothic" },
	{ "grb", "Grebo" },
	{ "grc", "Ancient Greek" },
	{ "gsw", "Swiss German" },
	{ "gu", "Gujarati" },
	{ "guc", "Wayuu" },
	{ "gur", "Frafra" },
	{ "guz", "Gusii" },
	{ "gv", "Manx" },
	{ "gwi", "Gwichʼin" },
	{ "ha", "Hausa" },
	{ "hai", "Haida" },
	{ "hak", "Hakka Chinese" },
	{ "haw", "Hawaiian" },
	{ "he", "Hebrew" },
	{ "hi", "Hindi" },
	{ "hif", "Fiji Hindi" },
	{ "hil", "Hiligaynon" },
	{ "hit", "Hittite" },
	{ "hmn", "Hmong" },
	{ "ho", "Hiri Motu" },
	{ "hne", "Chhattisgarhi" },
	{ "hr", "Croatian" },
	{ "hsb", "Upper Sorbian" },
	{ "hsn", "Xiang Chinese" },
	{ "ht", "Haitian" },
	{ "hu", "Hungarian" },
	{ "hup", "Hupa" },
	{ "hus", "Huastec" },
	{ "hy", "Armenian" },
	{ "hz", "Herero" },
	{ "ia", "Interlingua" },
	{ "iba", "Iban" },
	{ "ibb", "Ibibio" },
	{ "id", "Indonesian" },
	{ "ie", "Interlingue" },
	{ "ig", "Igbo" },
	{ "ii", "Sichuan Yi" },
	{ "ik", "Inupiaq" },
	{ "ilo", "Iloko" },
	{ "inh", "Ingush" },
	{ "io", "Ido" },
	{ "is", "Icelandic" },
	{ "it", "Italian" },
	{ "iu", "Inuktitut" },
	{ "izh", "Ingrian" },
	{ "ja", "Japanese" },
	{ "jam", "Jamaican Creole English" },
	{ "jbo", "Lojban" },
	{ "jgo", "Ngomba" },
	{ "jmc", "Machame" },
	{ "jpr", "Judeo-Persian" },
	{ "jrb", "Judeo-Arabic" },
	{ "jut", "Jutish" },
	{ "jv", "Javanese" },
	{ "ka", "Georgian" },
	{ "kaa", "Kara-Kalpak" },
	{ "kab", "Kabyle" },
	{ "kac", "Kachin" },
	{ "kaj", "Jju" },
	{ "kam", "Kamba" },
	{ "kaw", "Kawi" },
	{ "kbd", "Kabardian" },
	{ "kbl", "Kanembu" },
	{ "kcg", "Tyap" },
	{ "kde", "Makonde" },
	{ "kea", "Kabuverdianu" },
	{ "ken", "Kenyang" },
	{ "kfo", "Koro" },
	{ "kg", "Kongo" },
	{ "kgp", "Kaingang" },
	{ "kha", "Khasi" },
	{ "kho", "Khotanese" },
	{ "khq", "Koyra Chiini" },
	{ "khw", "Khowar" },
	{ "ki", "Kikuyu" },
	{ "kiu", "Kirmanjki" },
	{ "kj", "Kuanyama" },
	{ "kk", "Kazakh" },
	{ "kkj", "Kako" },
	{ "kl", "Kalaallisut" },
	{ "kln", "Kalenjin" },
	{ "km", "Central Khmer" },
	{ "kmb", "Kimbundu" },
	{ "kn", "Kannada" },
	{ "ko", "Korean" },
	{ "koi", "Komi-Permyak" },
	{ "kok", "Konkani" },
	{ "kos", "Kosraean" },
	{ "kpe", "Kpelle" },
	{ "kr", "Kanuri" },
	{ "krc", "Karachay-Balkar" },
	{ "kri", "Krio" },
	{ "krj", "Kinaray-a" },
	{ "krl", "Karelian" },
	{ "kru", "Kurukh" },
	{ "ks", "Kashmiri" },
	{ "ksb", "Shambala" },
	{ "ksf", "Bafia" },
	{ "ksh", "Colognian" },
	{ "ku", "Kurdish" },
	{ "kum", "Kumyk" },
	{ "kut", "Kutenai" },
	{ "kv", "Komi" },
	{ "kw", "Cornish" },
	{ "ky", "Kirghiz" },
	{ "la", "Latin" },
	{ "lad", "Ladino" },
	{ "lag", "Langi" },
	{ "lah", "Lahnda" },
	{ "lam", "Lamba" },
	{ "lb", "Luxembourgish" },
	{ "lez", "Lezghian" },
	{ "lfn", "Lingua Franca Nova" },
	{ "lg", "Ganda" },
	{ "li", "Limburgan" },
	{ "lij", "Ligurian" },
	{ "liv", "Livonian" },
	{ "lkt", "Lakota" },
	{ "lmo", "Lombard" },
	{ "ln", "Lingala" },
	{ "lo", "Lao" },
	{ "lol", "Mongo" },
	{ "lou", "Louisiana Creole" },
	{ "loz", "Lozi" },
	{ "lrc", "Northern Luri" },
	{ "lt", "Lithuanian" },
	{ "ltg", "Latgalian" },
	{ "lu", "Luba-Katanga" },
	{ "lua", "Luba-Lulua" },
	{ "lui", "Luiseno" },
	{ "lun", "Lunda" },
	{ "luo", "Luo" },
	{ "lus", "Mizo" },
	{ "luy", "Luyia" },
	{ "lv", "Latvian" },
	{ "lzh", "Literary Chinese" },
	{ "lzz", "Laz" },
	{ "mad", "Madurese" },
	{ "maf", "Mafa" },
	{ "mag", "Magahi" },
	{ "mai", "Maithili" },
	{ "mak", "Makasar" },
	{ "man", "Mandingo" },
	{ "mas", "Masai" },
	{ "mde", "Maba" },
	{ "mdf", "Moksha" },
	{ "mdr", "Mandar" },
	{ "men", "Mende" },
	{ "mer", "Meru" },
	{ "mfe", "Morisyen" },
	{ "mg", "Malagasy" },
	{ "mga", "Middle Irish" },
	{ "mgh", "Makhuwa-Meetto" },
	{ "mgo", "Metaʼ" },
	{ "mh", "Marshallese" },
	{ "mhr", "Eastern Mari" },
	{ "mi", "Māori" },
	{ "mic", "Mi'kmaq" },
	{ "min", "Minangkabau" },
	{ "miq", "Mískito" },
	{ "mjw", "Karbi" },
	{ "mk", "Macedonian" },
	{ "ml", "Malayalam" },
	{ "mn", "Mongolian" },
	{ "mnc", "Manchu" },
	{ "mni", "Manipuri" },
	{ "mnw", "Mon" },
	{ "mos", "Mossi" },
	{ "moh", "Mohawk" },
	{ "mr", "Marathi" },
	{ "mrj", "Western Mari" },
	{ "ms", "Malay" },
	{ "mt", "Maltese" },
	{ "mua", "Mundang" },
	{ "mus", "Muscogee" },
	{ "mwl", "Mirandese" },
	{ "mwr", "Marwari" },
	{ "mwv", "Mentawai" },
	{ "my", "Burmese" },
	{ "mye", "Myene" },
	{ "myv", "Erzya" },
	{ "mzn", "Mazanderani" },
	{ "na", "Nauru" },
	{ "nah", "Nahuatl" },
	{ "nan", "Min Nan Chinese" },
	{ "nap", "Neapolitan" },
	{ "naq", "Nama" },
	{ "nan", "Min Nan Chinese" },
	{ "nb", "Norwegian Bokmål" },
	{ "nd", "North Ndebele" },
	{ "nds", "Low German" },
	{ "ne", "Nepali" },
	{ "new", "Newari" },
	{ "nhn", "Central Nahuatl" },
	{ "ng", "Ndonga" },
	{ "nia", "Nias" },
	{ "niu", "Niuean" },
	{ "njo", "Ao Naga" },
	{ "nl", "Dutch" },
	{ "nmg", "Kwasio" },
	{ "nn", "Norwegian Nynorsk" },
	{ "nnh", "Ngiemboon" },
	{ "nog", "Nogai" },
	{ "non", "Old Norse" },
	{ "nov", "Novial" },
	{ "nqo", "N'ko" },
	{ "nr", "South Ndebele" },
	{ "nso", "Pedi" },
	{ "nus", "Nuer" },
	{ "nv", "Navajo" },
	{ "nwc", "Classical Newari" },
	{ "ny", "Nyanja" },
	{ "nym", "Nyamwezi" },
	{ "nyn", "Nyankole" },
	{ "nyo", "Nyoro" },
	{ "nzi", "Nzima" },
	{ "oc", "Occitan" },
	{ "oj", "Ojibwa" },
	{ "om", "Oromo" },
	{ "or", "Odia" },
	{ "os", "Ossetic" },
	{ "osa", "Osage" },
	{ "ota", "Ottoman Turkish" },
	{ "pa", "Panjabi" },
	{ "pag", "Pangasinan" },
	{ "pal", "Pahlavi" },
	{ "pam", "Pampanga" },
	{ "pap", "Papiamento" },
	{ "pau", "Palauan" },
	{ "pcd", "Picard" },
	{ "pcm", "Nigerian Pidgin" },
	{ "pdc", "Pennsylvania German" },
	{ "pdt", "Plautdietsch" },
	{ "peo", "Old Persian" },
	{ "pfl", "Palatine German" },
	{ "phn", "Phoenician" },
	{ "pi", "Pali" },
	{ "pl", "Polish" },
	{ "pms", "Piedmontese" },
	{ "pnt", "Pontic" },
	{ "pon", "Pohnpeian" },
	{ "pr", "Pirate" },
	{ "prg", "Prussian" },
	{ "pro", "Old Provençal" },
	{ "prs", "Dari" },
	{ "ps", "Pushto" },
	{ "pt", "Portuguese" },
	{ "qu", "Quechua" },
	{ "quc", "K'iche" },
	{ "qug", "Chimborazo Highland Quichua" },
	{ "quy", "Ayacucho Quechua" },
	{ "quz", "Cusco Quechua" },
	{ "raj", "Rajasthani" },
	{ "rap", "Rapanui" },
	{ "rar", "Rarotongan" },
	{ "rgn", "Romagnol" },
	{ "rif", "Riffian" },
	{ "rm", "Romansh" },
	{ "rn", "Rundi" },
	{ "ro", "Romanian" },
	{ "rof", "Rombo" },
	{ "rom", "Romany" },
	{ "rtm", "Rotuman" },
	{ "ru", "Russian" },
	{ "rue", "Rusyn" },
	{ "rug", "Roviana" },
	{ "rup", "Aromanian" },
	{ "rw", "Kinyarwanda" },
	{ "rwk", "Rwa" },
	{ "sa", "Sanskrit" },
	{ "sad", "Sandawe" },
	{ "sah", "Sakha" },
	{ "sam", "Samaritan Aramaic" },
	{ "saq", "Samburu" },
	{ "sas", "Sasak" },
	{ "sat", "Santali" },
	{ "saz", "Saurashtra" },
	{ "sba", "Ngambay" },
	{ "sbp", "Sangu" },
	{ "sc", "Sardinian" },
	{ "scn", "Sicilian" },
	{ "sco", "Scots" },
	{ "sd", "Sindhi" },
	{ "sdc", "Sassarese Sardinian" },
	{ "sdh", "Southern Kurdish" },
	{ "se", "Northern Sami" },
	{ "see", "Seneca" },
	{ "seh", "Sena" },
	{ "sei", "Seri" },
	{ "sel", "Selkup" },
	{ "ses", "Koyraboro Senni" },
	{ "sg", "Sango" },
	{ "sga", "Old Irish" },
	{ "sgs", "Samogitian" },
	{ "sh", "Serbo-Croatian" },
	{ "shi", "Tachelhit" },
	{ "shn", "Shan" },
	{ "shs", "Shuswap" },
	{ "shu", "Chadian Arabic" },
	{ "si", "Sinhala" },
	{ "sid", "Sidamo" },
	{ "sk", "Slovak" },
	{ "sl", "Slovenian" },
	{ "sli", "Lower Silesian" },
	{ "sly", "Selayar" },
	{ "sm", "Samoan" },
	{ "sma", "Southern Sami" },
	{ "smj", "Lule Sami" },
	{ "smn", "Inari Sami" },
	{ "sms", "Skolt Sami" },
	{ "sn", "Shona" },
	{ "snk", "Soninke" },
	{ "so", "Somali" },
	{ "sog", "Sogdien" },
	{ "son", "Songhai" },
	{ "sq", "Albanian" },
	{ "sr", "Serbian" },
	{ "srn", "Sranan Tongo" },
	{ "srr", "Serer" },
	{ "ss", "Swati" },
	{ "ssy", "Saho" },
	{ "st", "Southern Sotho" },
	{ "stq", "Saterland Frisian" },
	{ "su", "Sundanese" },
	{ "suk", "Sukuma" },
	{ "sus", "Susu" },
	{ "sux", "Sumerian" },
	{ "sv", "Swedish" },
	{ "sw", "Swahili" },
	{ "swb", "Comorian" },
	{ "swc", "Congo Swahili" },
	{ "syc", "Classical Syriac" },
	{ "syr", "Syriac" },
	{ "szl", "Silesian" },
	{ "ta", "Tamil" },
	{ "tcy", "Tulu" },
	{ "te", "Telugu" },
	{ "tem", "Timne" },
	{ "teo", "Teso" },
	{ "ter", "Tereno" },
	{ "tet", "Tetum" },
	{ "tg", "Tajik" },
	{ "th", "Thai" },
	{ "the", "Chitwania Tharu" },
	{ "ti", "Tigrinya" },
	{ "tig", "Tigre" },
	{ "tiv", "Tiv" },
	{ "tk", "Turkmen" },
	{ "tkl", "Tokelau" },
	{ "tkr", "Tsakhur" },
	{ "tl", "Tagalog" },
	{ "tlh", "Klingon" },
	{ "tli", "Tlingit" },
	{ "tly", "Talysh" },
	{ "tmh", "Tamashek" },
	{ "tn", "Tswana" },
	{ "to", "Tongan" },
	{ "tok", "Toki Pona" },
	{ "tog", "Nyasa Tonga" },
	{ "tpi", "Tok Pisin" },
	{ "tr", "Turkish" },
	{ "tru", "Turoyo" },
	{ "trv", "Taroko" },
	{ "ts", "Tsonga" },
	{ "tsd", "Tsakonian" },
	{ "tsi", "Tsimshian" },
	{ "tt", "Tatar" },
	{ "ttt", "Muslim Tat" },
	{ "tum", "Tumbuka" },
	{ "tvl", "Tuvalu" },
	{ "tw", "Twi" },
	{ "twq", "Tasawaq" },
	{ "ty", "Tahitian" },
	{ "tyv", "Tuvinian" },
	{ "tzm", "Central Atlas Tamazight" },
	{ "udm", "Udmurt" },
	{ "ug", "Uyghur" },
	{ "uga", "Ugaritic" },
	{ "uk", "Ukrainian" },
	{ "umb", "Umbundu" },
	{ "unm", "Unami" },
	{ "ur", "Urdu" },
	{ "uz", "Uzbek" },
	{ "vai", "Vai" },
	{ "ve", "Venda" },
	{ "vec", "Venetian" },
	{ "vep", "Veps" },
	{ "vi", "Vietnamese" },
	{ "vls", "West Flemish" },
	{ "vmf", "Main-Franconian" },
	{ "vo", "Volapük" },
	{ "vot", "Votic" },
	{ "vro", "Võro" },
	{ "vun", "Vunjo" },
	{ "wa", "Walloon" },
	{ "wae", "Walser" },
	{ "wal", "Wolaytta" },
	{ "war", "Waray" },
	{ "was", "Washo" },
	{ "wbp", "Warlpiri" },
	{ "wo", "Wolof" },
	{ "wuu", "Wu Chinese" },
	{ "xal", "Kalmyk" },
	{ "xh", "Xhosa" },
	{ "xmf", "Mingrelian" },
	{ "xog", "Soga" },
	{ "yao", "Yao" },
	{ "yap", "Yapese" },
	{ "yav", "Yangben" },
	{ "ybb", "Yemba" },
	{ "yi", "Yiddish" },
	{ "yo", "Yoruba" },
	{ "yrl", "Nheengatu" },
	{ "yue", "Yue Chinese" },
	{ "yuw", "Papua New Guinea" },
	{ "za", "Zhuang" },
	{ "zap", "Zapotec" },
	{ "zbl", "Blissymbols" },
	{ "zea", "Zeelandic" },
	{ "zen", "Zenaga" },
	{ "zgh", "Standard Moroccan Tamazight" },
	{ "zh", "Chinese" },
	{ "zu", "Zulu" },
	{ "zun", "Zuni" },
	{ "zza", "Zaza" },
	{ nullptr, nullptr },
};

// Scripts used by languages.
// Reference:
// - https://www.unicode.org/cldr/charts/48/supplemental/languages_and_scripts.html

static const char *language_script_list[][2] = {
	{ "aa", "Latn" },
	{ "ab", "Cyrl" },
	{ "ace", "Latn" },
	{ "ach", "Latn" },
	{ "ada", "Latn" },
	{ "ady", "Cyrl" },
	{ "ae", "Avst" },
	{ "aeb", "Arab" },
	{ "af", "Latn" },
	{ "afh", "Latn" },
	{ "agq", "Latn" },
	{ "ain", "Kana Latn" },
	{ "agr", "Latn" },
	{ "ak", "Latn" },
	{ "akk", "Xsux" },
	{ "akz", "Latn" },
	{ "ale", "Latn" },
	{ "aln", "Latn" },
	{ "alt", "Cyrl" },
	{ "am", "Ethi" },
	{ "an", "Latn" },
	{ "ang", "Latn" },
	{ "anp", "Deva" },
	{ "ar", "Arab Syrc" },
	{ "arc", "Armi Nbat Palm" },
	{ "arn", "Latn" },
	{ "aro", "Latn" },
	{ "arp", "Latn" },
	{ "arq", "Arab" },
	{ "ars", "Arab" },
	{ "arw", "Latn" },
	{ "ary", "Arab" },
	{ "arz", "Arab" },
	{ "as", "Beng" },
	{ "asa", "Latn" },
	{ "ast", "Latn" },
	{ "av", "Cyrl" },
	{ "avk", "Latn" },
	{ "awa", "Deva" },
	{ "ayc", "Latn" },
	{ "ay", "Latn" },
	{ "az", "Latn Arab Cyrl" },
	{ "ba", "Cyrl" },
	{ "bal", "Arab Latn" },
	{ "ban", "Latn Bali" },
	{ "bar", "Latn" },
	{ "bas", "Latn" },
	{ "bax", "Bamu" },
	{ "bbc", "Latn Batk" },
	{ "bbj", "Latn" },
	{ "be", "Cyrl" },
	{ "bej", "Arab" },
	{ "bem", "Latn" },
	{ "ber", "Latn Arab Tfng" },
	{ "bew", "Latn" },
	{ "bez", "Latn" },
	{ "bfd", "Latn" },
	{ "bfq", "Taml" },
	{ "bg", "Cyrl" },
	{ "bhb", "Deva" },
	{ "bgn", "Arab" },
	{ "bho", "Deva" },
	{ "bi", "Latn" },
	{ "bik", "Latn" },
	{ "bin", "Latn" },
	{ "bjn", "Latn" },
	{ "bkm", "Latn" },
	{ "bla", "Latn" },
	{ "bm", "Latn Nkoo" },
	{ "bn", "Beng" },
	{ "bo", "Tibt" },
	{ "bpy", "Beng" },
	{ "bqi", "Arab" },
	{ "br", "Latn" },
	{ "brh", "Arab Latn" },
	{ "brx", "Deva" },
	{ "bs", "Latn Cyrl" },
	{ "bss", "Latn" },
	{ "bua", "Cyrl" },
	{ "bug", "Latn Bug" },
	{ "bum", "Latn" },
	{ "byn", "Ethi" },
	{ "byv", "Latn" },
	{ "ca", "Latn" },
	{ "cad", "Latn" },
	{ "car", "Latn" },
	{ "cay", "Latn" },
	{ "cch", "Latn" },
	{ "ccp", "Cakm Beng" },
	{ "ce", "Cyrl" },
	{ "ceb", "Latn" },
	{ "cgg", "Latn" },
	{ "ch", "Latn" },
	{ "chb", "Latn" },
	{ "chg", "Arab" },
	{ "chk", "Latn" },
	{ "chm", "Cyrl" },
	{ "chn", "Latn" },
	{ "cho", "Latn" },
	{ "chp", "Latn Cans" },
	{ "chr", "Cher" },
	{ "chy", "Latn" },
	{ "cic", "Latn" },
	{ "ckb", "Arab" },
	{ "cmn", "Hant Hans Hani" },
	{ "co", "Latn" },
	{ "cop", "Copt Arab Grek" },
	{ "cps", "Latn" },
	{ "cr", "Cans Latn" },
	{ "crh", "Cyrl" },
	{ "crs", "Latn" },
	{ "cs", "Latn" },
	{ "csb", "Latn" },
	{ "cu", "Cyrl" },
	{ "cv", "Cyrl" },
	{ "cy", "Latn" },
	{ "da", "Latn" },
	{ "dak", "Latn" },
	{ "dar", "Cyrl" },
	{ "dav", "Latn" },
	{ "de", "Latn Runr" },
	{ "del", "Latn" },
	{ "den", "Latn Cans" },
	{ "dgr", "Latn" },
	{ "din", "Latn" },
	{ "dje", "Latn" },
	{ "doi", "Deva Arab Takr" },
	{ "dsb", "Latn" },
	{ "dtp", "Latn" },
	{ "dua", "Latn" },
	{ "dum", "Latn" },
	{ "dv", "Thaa" },
	{ "dyo", "Latn Arab" },
	{ "dyu", "Latn" },
	{ "dz", "Tibt" },
	{ "dzg", "Latn" },
	{ "ebu", "Latn" },
	{ "ee", "Latn" },
	{ "efi", "Latn" },
	{ "egl", "Latn" },
	{ "egy", "Egyp" },
	{ "eka", "Latn" },
	{ "el", "Grek" },
	{ "elx", "Xsux" },
	{ "en", "Latn Dsrt Shaw" },
	{ "enm", "Latn" },
	{ "eo", "Latn" },
	{ "es", "Latn" },
	{ "esu", "Latn" },
	{ "et", "Latn" },
	{ "eu", "Latn" },
	{ "ewo", "Latn" },
	{ "ext", "Latn" },
	{ "fa", "Arab" },
	{ "fan", "Latn" },
	{ "fat", "Latn" },
	{ "ff", "Latn Adlm" },
	{ "fi", "Latn" },
	{ "fil", "Latn Tglg" },
	{ "fit", "Latn" },
	{ "fj", "Latn" },
	{ "fo", "Latn" },
	{ "fon", "Latn" },
	{ "fr", "Latn Dupl" },
	{ "frc", "Latn" },
	{ "frm", "Latn" },
	{ "fro", "Latn" },
	{ "frp", "Latn" },
	{ "frr", "Latn" },
	{ "frs", "Latn" },
	{ "fur", "Latn" },
	{ "fy", "Latn" },
	{ "ga", "Latn" },
	{ "gaa", "Latn" },
	{ "gag", "Latn Cyrl" },
	{ "gan", "Hans Hani" },
	{ "gay", "Latn" },
	{ "gba", "Latn" },
	{ "gbz", "Arab" },
	{ "gd", "Latn" },
	{ "gez", "Ethi" },
	{ "gil", "Latn" },
	{ "gl", "Latn" },
	{ "glk", "Arab" },
	{ "gmh", "Latn" },
	{ "gn", "Latn" },
	{ "goh", "Latn" },
	{ "gom", "Deav Latn Knda Mlym Arab" },
	{ "gon", "Deva Telu" },
	{ "gor", "Latn" },
	{ "got", "Goth" },
	{ "grb", "Latn" },
	{ "grc", "Grek" },
	{ "gsw", "Latn" },
	{ "gu", "Gujr" },
	{ "guc", "Latn" },
	{ "gur", "Latn" },
	{ "guz", "Latn" },
	{ "gv", "Latn" },
	{ "gwi", "Latn" },
	{ "ha", "Latn Arab" },
	{ "hai", "Latn" },
	{ "hak", "Hans Hant Hani" },
	{ "haw", "Latn" },
	{ "he", "Hebr" },
	{ "hi", "Deva Latn Mahj" },
	{ "hif", "Deva Latn" },
	{ "hil", "Latn" },
	{ "hit", "Xsux" },
	{ "hmn", "Latn Hmng" },
	{ "ho", "Latn" },
	{ "hne", "Deva" },
	{ "hr", "Latn" },
	{ "hsb", "Latn" },
	{ "hsn", "Hans Hani" },
	{ "ht", "Latn" },
	{ "hu", "Latn" },
	{ "hup", "Latn" },
	{ "hus", "Latn" },
	{ "hy", "Armn" },
	{ "hz", "Latn" },
	{ "ia", "Latn" },
	{ "iba", "Latn" },
	{ "ibb", "Latn" },
	{ "id", "Latn Arab" },
	{ "ie", "Latn" },
	{ "ig", "Latn" },
	{ "ii", "Yiii Latn" },
	{ "ik", "Latn" },
	{ "ilo", "Latn" },
	{ "inh", "Cyrl" },
	{ "io", "Latn" },
	{ "is", "Latn" },
	{ "it", "Latn" },
	{ "iu", "Cans Latn" },
	{ "izh", "Latn" },
	{ "ja", "Jpan Hani Hira Kana" },
	{ "jam", "Latn" },
	{ "jbo", "Latn" },
	{ "jgo", "Latn" },
	{ "jmc", "Latn" },
	{ "jpr", "Hebr" },
	{ "jrb", "Hebr" },
	{ "jut", "Latn" },
	{ "jv", "Latn Java" },
	{ "ka", "Geor" },
	{ "kaa", "Cyrl Latn" },
	{ "kab", "Latn" },
	{ "kac", "Latn" },
	{ "kaj", "Latn" },
	{ "kam", "Latn" },
	{ "kaw", "Bali Java Kawi" },
	{ "kbd", "Cyrl" },
	{ "kbl", "Arab" },
	{ "kcg", "Latn" },
	{ "kde", "Latn" },
	{ "kea", "Latn" },
	{ "ken", "Latn" },
	{ "kfo", "Latn" },
	{ "kg", "Latn" },
	{ "kgp", "Latn" },
	{ "kha", "Latn Beng" },
	{ "kho", "Brah Khar" },
	{ "khq", "Latn" },
	{ "khw", "Arab" },
	{ "ki", "Latn" },
	{ "kiu", "Latn" },
	{ "kj", "Latn" },
	{ "kk", "Cyrl Arab" },
	{ "kkj", "Latn" },
	{ "kl", "Latn" },
	{ "kln", "Latn" },
	{ "km", "Khmr" },
	{ "kmb", "Latn" },
	{ "kn", "Knda" },
	{ "ko", "Kore Hang Hani" },
	{ "koi", "Cyrl" },
	{ "kok", "Deva Latn" },
	{ "kos", "Latn" },
	{ "kpe", "Latn" },
	{ "kr", "Latn" },
	{ "krc", "Cyrl" },
	{ "kri", "Latn" },
	{ "krj", "Latn" },
	{ "krl", "Latn" },
	{ "kru", "Deva" },
	{ "ks", "Arab Deva" },
	{ "ksb", "Latn" },
	{ "ksf", "Latn" },
	{ "ksh", "Latn" },
	{ "ku", "Latn Arab Cyrl" },
	{ "kum", "Cyrl" },
	{ "kut", "Latn" },
	{ "kv", "Cyrl Perm" },
	{ "kw", "Latn" },
	{ "ky", "Cyrl Arab Latn" },
	{ "la", "Latn" },
	{ "lad", "Hebr" },
	{ "lag", "Latn" },
	{ "lah", "Arab" },
	{ "lam", "Latn" },
	{ "lb", "Latn" },
	{ "lez", "Cyrl Aghb" },
	{ "lfn", "Latn Cyrl" },
	{ "lg", "Latn" },
	{ "li", "Latn" },
	{ "lij", "Latn" },
	{ "liv", "Latn" },
	{ "lkt", "Latn" },
	{ "lmo", "Latn" },
	{ "ln", "Latn" },
	{ "lo", "Laoo" },
	{ "lol", "Latn" },
	{ "lou", "Latn" },
	{ "loz", "Latn" },
	{ "lrc", "Arab" },
	{ "lt", "Latn" },
	{ "ltg", "Latn" },
	{ "lu", "Latn" },
	{ "lua", "Latn" },
	{ "lui", "Latn" },
	{ "lun", "Latn" },
	{ "luo", "Latn" },
	{ "lus", "Beng" },
	{ "luy", "Latn" },
	{ "lv", "Latn" },
	{ "lzh", "Hant" },
	{ "lzz", "Latn Geor" },
	{ "mad", "Latn" },
	{ "maf", "Latn" },
	{ "mag", "Deva" },
	{ "mai", "Deva Tirh" },
	{ "mak", "Latn Bugi" },
	{ "man", "Latn Nkoo" },
	{ "mas", "Latn" },
	{ "mde", "Arab Latn" },
	{ "mdf", "Cyrl" },
	{ "mdr", "Latn Bugi" },
	{ "men", "Latn Mend" },
	{ "mer", "Latn" },
	{ "mfe", "Latn" },
	{ "mg", "Latn" },
	{ "mga", "Latn" },
	{ "mgh", "Latn" },
	{ "mgo", "Latn" },
	{ "mh", "Latn" },
	{ "mhr", "Cyrl" },
	{ "mi", "Latn" },
	{ "mic", "Latn" },
	{ "min", "Latn" },
	{ "miq", "Latn" },
	{ "mjw", "Latn" },
	{ "mk", "Cyrl" },
	{ "ml", "Mlym" },
	{ "mn", "Cyrl Mong Phag" },
	{ "mnc", "Mong" },
	{ "mni", "Beng Mtei" },
	{ "mnw", "Mymr" },
	{ "mos", "Latn" },
	{ "moh", "Latn" },
	{ "mr", "Deva Modi" },
	{ "mrj", "Cyrl" },
	{ "ms", "Latn Arab" },
	{ "mt", "Latn" },
	{ "mua", "Latn" },
	{ "mus", "Latn" },
	{ "mwl", "Latn" },
	{ "mwr", "Deva" },
	{ "mwv", "Latn" },
	{ "my", "Mymr" },
	{ "mye", "Latn" },
	{ "myv", "Cyrl" },
	{ "mzn", "Arab" },
	{ "na", "Latn" },
	{ "nah", "Latn" },
	{ "nan", "Hans Hant Hani" },
	{ "nap", "Latn" },
	{ "naq", "Latn" },
	{ "nan", "Latn Hans Hant Hani" },
	{ "nb", "Latn" },
	{ "nd", "Latn" },
	{ "nds", "Latn" },
	{ "ne", "Deva" },
	{ "new", "Deva" },
	{ "nhn", "Latn" },
	{ "ng", "Latn" },
	{ "nia", "Latn" },
	{ "niu", "Latn" },
	{ "njo", "Latn" },
	{ "nl", "Latn" },
	{ "nmg", "Latn" },
	{ "nn", "Latn" },
	{ "nnh", "Latn" },
	{ "nog", "Cyrl" },
	{ "non", "Runr" },
	{ "nov", "Latn" },
	{ "nqo", "Nkoo" },
	{ "nr", "Latn" },
	{ "nso", "Latn" },
	{ "nus", "Latn" },
	{ "nv", "Latn" },
	{ "nwc", "Ranj Newa Deva Sidd Brah" },
	{ "ny", "Latn" },
	{ "nym", "Latn" },
	{ "nyn", "Latn" },
	{ "nyo", "Latn" },
	{ "nzi", "Latn" },
	{ "oc", "Latn" },
	{ "oj", "Cans Latn" },
	{ "om", "Latn Ethi" },
	{ "or", "Orya" },
	{ "os", "Cyrl" },
	{ "osa", "Osge Latn" },
	{ "ota", "Arab" },
	{ "pa", "Guru Arab" },
	{ "pag", "Latn" },
	{ "pal", "Phli Phlp" },
	{ "pam", "Latn" },
	{ "pap", "Latn" },
	{ "pau", "Latn" },
	{ "pcd", "Latn" },
	{ "pcm", "Latn" },
	{ "pdc", "Latn" },
	{ "pdt", "Latn" },
	{ "peo", "Xpeo" },
	{ "pfl", "Latn" },
	{ "phn", "Phnx" },
	{ "pi", "Latn Deva Mymr Sinh Thai" },
	{ "pl", "Latn" },
	{ "pms", "Latn" },
	{ "pnt", "Grek Cyrl Latn" },
	{ "pon", "Latn" },
	{ "pr", "Latn" },
	{ "prg", "Latn" },
	{ "pro", "Latn" },
	{ "prs", "Arab" },
	{ "ps", "Arab" },
	{ "pt", "Latn" },
	{ "qu", "Latn" },
	{ "quc", "Latn" },
	{ "qug", "Latn" },
	{ "quy", "Latn" },
	{ "quz", "Latn" },
	{ "raj", "Deva" },
	{ "rap", "Latn" },
	{ "rar", "Latn" },
	{ "rgn", "Latn" },
	{ "rif", "Latn Tfng" },
	{ "rm", "Latn" },
	{ "rn", "Latn" },
	{ "ro", "Latn Cyrl" },
	{ "rof", "Latn" },
	{ "rom", "Latn Cyrl" },
	{ "rtm", "Latn" },
	{ "ru", "Cyrl" },
	{ "rue", "Cyrl" },
	{ "rug", "Latn" },
	{ "rup", "Latn" },
	{ "rw", "Latn" },
	{ "rwk", "Latn" },
	{ "sa", "Deva Gran Shrd Sidd Sinh" },
	{ "sad", "Latn" },
	{ "sah", "Cyrl" },
	{ "sam", "Samr Hebr" },
	{ "saq", "Latn" },
	{ "sas", "Latn" },
	{ "sat", "Olck Beng Deva Latn Orya" },
	{ "saz", "Saur" },
	{ "sba", "Latn" },
	{ "sbp", "Latn" },
	{ "sc", "Latn" },
	{ "scn", "Latn" },
	{ "sco", "Latn" },
	{ "sd", "Arab Deva Khoj Sind" },
	{ "sdc", "Latn" },
	{ "sdh", "Arab" },
	{ "se", "Latn Cyrl Arab" },
	{ "see", "Latn" },
	{ "seh", "Latn" },
	{ "sei", "Latn" },
	{ "sel", "Cyrl" },
	{ "ses", "Latn" },
	{ "sg", "Latn" },
	{ "sga", "Latn Ogam" },
	{ "sgs", "Latn" },
	{ "sh", "Latn Cyrl" },
	{ "shi", "Tfng Latn Arab" },
	{ "shn", "Mymr" },
	{ "shs", "Latn Dupl" },
	{ "shu", "Arab" },
	{ "si", "Sinh" },
	{ "sid", "Latn" },
	{ "sk", "Latn" },
	{ "sl", "Latn" },
	{ "sli", "Latn" },
	{ "sly", "Latn" },
	{ "sm", "Latn" },
	{ "sma", "Latn" },
	{ "smj", "Latn" },
	{ "smn", "Latn" },
	{ "sms", "Latn" },
	{ "sn", "Latn" },
	{ "snk", "Latn" },
	{ "so", "Latn Arab Osma" },
	{ "sog", "Sogd Syrc Mani" },
	{ "son", "Latn Arab" },
	{ "sq", "Latn Elba" },
	{ "sr", "Cyrl Latn" },
	{ "srn", "Latn" },
	{ "srr", "Latn" },
	{ "ss", "Latn" },
	{ "ssy", "Latn" },
	{ "st", "Latn" },
	{ "stq", "Latn" },
	{ "su", "Latn Sund" },
	{ "suk", "Latn" },
	{ "sus", "Latn Arab" },
	{ "sux", "Xsux" },
	{ "sv", "Latn" },
	{ "sw", "Latn" },
	{ "swb", "Arab Latn" },
	{ "swc", "Arab Latn" },
	{ "syc", "Syrc" },
	{ "syr", "Syrc" },
	{ "szl", "Latn" },
	{ "ta", "Taml" },
	{ "tcy", "Knda" },
	{ "te", "Telu" },
	{ "tem", "Latn" },
	{ "teo", "Latn" },
	{ "ter", "Latn" },
	{ "tet", "Latn" },
	{ "tg", "Cyrl Arab Latn" },
	{ "th", "Thai" },
	{ "the", "Deva" },
	{ "ti", "Ethi" },
	{ "tig", "Ethi" },
	{ "tiv", "Latn" },
	{ "tk", "Latn Arab Cyrl" },
	{ "tkl", "Latn" },
	{ "tkr", "Latn Cyrl" },
	{ "tl", "Latn Arab Blis" },
	{ "tlh", "Piqd Latn" },
	{ "tli", "Latn" },
	{ "tly", "Latn Arab Cyrl" },
	{ "tmh", "Latn" },
	{ "tn", "Latn" },
	{ "to", "Latn" },
	{ "tog", "Latn" },
	{ "tok", "Latn" },
	{ "tpi", "Latn" },
	{ "tr", "Latn Arab" },
	{ "tru", "Latn Syrc" },
	{ "trv", "Latn" },
	{ "ts", "Latn" },
	{ "tsd", "Grek" },
	{ "tsi", "Latn" },
	{ "tt", "Cyrl" },
	{ "ttt", "Latn Arab Cyrl" },
	{ "tum", "Latn" },
	{ "tvl", "Latn" },
	{ "tw", "Latn" },
	{ "twq", "Latn" },
	{ "ty", "Latn" },
	{ "tyv", "Cyrl" },
	{ "tzm", "Latn Tfng" },
	{ "udm", "Cyrl Latn" },
	{ "ug", "Arab Cyrl Latn" },
	{ "uga", "Ugar" },
	{ "uk", "Cyrl" },
	{ "umb", "Latn" },
	{ "unm", "Latn" },
	{ "ur", "Arab" },
	{ "uz", "Latn Cyrl Arab" },
	{ "vai", "Vaii Latn" },
	{ "ve", "Latn" },
	{ "vec", "Latn" },
	{ "vep", "Latn" },
	{ "vi", "Latn Hani" },
	{ "vls", "Latn" },
	{ "vmf", "Latn" },
	{ "vo", "Latn" },
	{ "vot", "Latn" },
	{ "vro", "Latn" },
	{ "vun", "Latn" },
	{ "wa", "Latn" },
	{ "wae", "Latn" },
	{ "wal", "Ethi" },
	{ "war", "Latn" },
	{ "was", "Latn" },
	{ "wbp", "Latn" },
	{ "wo", "Latn Arab" },
	{ "wuu", "Hans Hani" },
	{ "xal", "Cyrl" },
	{ "xh", "Latn" },
	{ "xmf", "Geor" },
	{ "xog", "Latn" },
	{ "yao", "Latn" },
	{ "yap", "Latn" },
	{ "yav", "Latn" },
	{ "ybb", "Latn" },
	{ "yi", "Hebr" },
	{ "yo", "Latn" },
	{ "yrl", "Latn" },
	{ "yue", "Hant Hans Hani" },
	{ "yuw", "Latn" },
	{ "za", "Latn Hans Hani" },
	{ "zap", "Latn" },
	{ "zbl", "Blis" },
	{ "zea", "Latn" },
	{ "zen", "Tfng" },
	{ "zgh", "Tfng" },
	{ "zh", "Hans Hant Hani Bopo Latn Phag" },
	{ "zu", "Latn" },
	{ "zun", "Latn" },
	{ "zza", "Latn" },
	{ nullptr, nullptr },
};

// Additional regional variants.
// Variant name, supported languages.

static const char *locale_variants[][2] = {
	{ "valencia", "ca" },
	{ "iqtelif", "tt" },
	{ "saaho", "aa" },
	{ "tradnl", "es" },
	{ nullptr, nullptr },
};

// Script names and codes (excludes typographic variants, special codes, reserved codes and aliases for combined scripts).
// Reference:
// - https://en.wikipedia.org/wiki/ISO_15924

static const char *script_list[][2] = {
	{ "Adlam", "Adlm" },
	{ "Afaka", "Afak" },
	{ "Caucasian Albanian", "Aghb" },
	{ "Ahom", "Ahom" },
	{ "Arabic", "Arab" },
	{ "Imperial Aramaic", "Armi" },
	{ "Armenian", "Armn" },
	{ "Avestan", "Avst" },
	{ "Balinese", "Bali" },
	{ "Bamum", "Bamu" },
	{ "Bassa Vah", "Bass" },
	{ "Batak", "Batk" },
	{ "Bengali", "Beng" },
	{ "Bhaiksuki", "Bhks" },
	{ "Blissymbols", "Blis" },
	{ "Bopomofo", "Bopo" },
	{ "Brahmi", "Brah" },
	{ "Braille", "Brai" },
	{ "Buginese", "Bugi" },
	{ "Buhid", "Buhd" },
	{ "Chakma", "Cakm" },
	{ "Unified Canadian Aboriginal", "Cans" },
	{ "Carian", "Cari" },
	{ "Cham", "Cham" },
	{ "Cherokee", "Cher" },
	{ "Chorasmian", "Chrs" },
	{ "Cirth", "Cirt" },
	{ "Coptic", "Copt" },
	{ "Cypro-Minoan", "Cpmn" },
	{ "Cypriot", "Cprt" },
	{ "Cyrillic", "Cyrl" },
	{ "Devanagari", "Deva" },
	{ "Dives Akuru", "Diak" },
	{ "Dogra", "Dogr" },
	{ "Deseret", "Dsrt" },
	{ "Duployan", "Dupl" },
	{ "Egyptian demotic", "Egyd" },
	{ "Egyptian hieratic", "Egyh" },
	{ "Egyptian hieroglyphs", "Egyp" },
	{ "Elbasan", "Elba" },
	{ "Elymaic", "Elym" },
	{ "Ethiopic", "Ethi" },
	{ "Khutsuri", "Geok" },
	{ "Georgian", "Geor" },
	{ "Glagolitic", "Glag" },
	{ "Gunjala Gondi", "Gong" },
	{ "Masaram Gondi", "Gonm" },
	{ "Gothic", "Goth" },
	{ "Grantha", "Gran" },
	{ "Greek", "Grek" },
	{ "Gujarati", "Gujr" },
	{ "Gurmukhi", "Guru" },
	{ "Hangul", "Hang" },
	{ "Han", "Hani" },
	{ "Hanunoo", "Hano" },
	{ "Simplified Han", "Hans" },
	{ "Traditional Han", "Hant" },
	{ "Hatran", "Hatr" },
	{ "Hebrew", "Hebr" },
	{ "Hiragana", "Hira" },
	{ "Anatolian Hieroglyphs", "Hluw" },
	{ "Pahawh Hmong", "Hmng" },
	{ "Nyiakeng Puachue Hmong", "Hmnp" },
	{ "Old Hungarian", "Hung" },
	{ "Indus", "Inds" },
	{ "Old Italic", "Ital" },
	{ "Javanese", "Java" },
	{ "Jurchen", "Jurc" },
	{ "Kayah Li", "Kali" },
	{ "Katakana", "Kana" },
	{ "Kawi", "Kawi" },
	{ "Kharoshthi", "Khar" },
	{ "Khmer", "Khmr" },
	{ "Khojki", "Khoj" },
	{ "Khitan large script", "Kitl" },
	{ "Khitan small script", "Kits" },
	{ "Kannada", "Knda" },
	{ "Kpelle", "Kpel" },
	{ "Kaithi", "Kthi" },
	{ "Tai Tham", "Lana" },
	{ "Lao", "Laoo" },
	{ "Latin", "Latn" },
	{ "Leke", "Leke" },
	{ "Lepcha", "Lepc" },
	{ "Limbu", "Limb" },
	{ "Linear A", "Lina" },
	{ "Linear B", "Linb" },
	{ "Lisu", "Lisu" },
	{ "Loma", "Loma" },
	{ "Lycian", "Lyci" },
	{ "Lydian", "Lydi" },
	{ "Mahajani", "Mahj" },
	{ "Makasar", "Maka" },
	{ "Mandaic", "Mand" },
	{ "Manichaean", "Mani" },
	{ "Marchen", "Marc" },
	{ "Mayan Hieroglyphs", "Maya" },
	{ "Medefaidrin", "Medf" },
	{ "Mende Kikakui", "Mend" },
	{ "Meroitic Cursive", "Merc" },
	{ "Meroitic Hieroglyphs", "Mero" },
	{ "Malayalam", "Mlym" },
	{ "Modi", "Modi" },
	{ "Mongolian", "Mong" },
	{ "Moon", "Moon" },
	{ "Mro", "Mroo" },
	{ "Meitei Mayek", "Mtei" },
	{ "Multani", "Mult" },
	{ "Myanmar / Burmese", "Mymr" },
	{ "Nag Mundari", "Nagm" },
	{ "Nandinagari", "Nand" },
	{ "Old North Arabian", "Narb" },
	{ "Nabataean", "Nbat" },
	{ "Newa", "Newa" },
	{ "Naxi Dongba", "Nkdb" },
	{ "Nakhi Geba", "Nkgb" },
	{ "N'ko", "Nkoo" },
	{ "Nüshu", "Nshu" },
	{ "Ogham", "Ogam" },
	{ "Ol Chiki", "Olck" },
	{ "Old Turkic", "Orkh" },
	{ "Oriya", "Orya" },
	{ "Osage", "Osge" },
	{ "Osmanya", "Osma" },
	{ "Old Uyghur", "Ougr" },
	{ "Palmyrene", "Palm" },
	{ "Pau Cin Hau", "Pauc" },
	{ "Proto-Cuneiform", "Pcun" },
	{ "Proto-Elamite", "Pelm" },
	{ "Old Permic", "Perm" },
	{ "Phags-pa", "Phag" },
	{ "Inscriptional Pahlavi", "Phli" },
	{ "Psalter Pahlavi", "Phlp" },
	{ "Book Pahlavi", "Phlv" },
	{ "Phoenician", "Phnx" },
	{ "Klingon", "Piqd" },
	{ "Miao", "Plrd" },
	{ "Inscriptional Parthian", "Prti" },
	{ "Proto-Sinaitic", "Psin" },
	{ "Ranjana", "Ranj" },
	{ "Rejang", "Rjng" },
	{ "Hanifi Rohingya", "Rohg" },
	{ "Rongorongo", "Roro" },
	{ "Runic", "Runr" },
	{ "Samaritan", "Samr" },
	{ "Sarati", "Sara" },
	{ "Old South Arabian", "Sarb" },
	{ "Saurashtra", "Saur" },
	{ "SignWriting", "Sgnw" },
	{ "Shavian", "Shaw" },
	{ "Sharada", "Shrd" },
	{ "Shuishu", "Shui" },
	{ "Siddham", "Sidd" },
	{ "Khudawadi", "Sind" },
	{ "Sinhala", "Sinh" },
	{ "Sogdian", "Sogd" },
	{ "Old Sogdian", "Sogo" },
	{ "Sora Sompeng", "Sora" },
	{ "Soyombo", "Soyo" },
	{ "Sundanese", "Sund" },
	{ "Syloti Nagri", "Sylo" },
	{ "Syriac", "Syrc" },
	{ "Tagbanwa", "Tagb" },
	{ "Takri", "Takr" },
	{ "Tai Le", "Tale" },
	{ "New Tai Lue", "Talu" },
	{ "Tamil", "Taml" },
	{ "Tangut", "Tang" },
	{ "Tai Viet", "Tavt" },
	{ "Telugu", "Telu" },
	{ "Tengwar", "Teng" },
	{ "Tifinagh", "Tfng" },
	{ "Tagalog", "Tglg" },
	{ "Thaana", "Thaa" },
	{ "Thai", "Thai" },
	{ "Tibetan", "Tibt" },
	{ "Tirhuta", "Tirh" },
	{ "Tangsa", "Tnsa" },
	{ "Toto", "Toto" },
	{ "Ugaritic", "Ugar" },
	{ "Vai", "Vaii" },
	{ "Visible Speech", "Visp" },
	{ "Vithkuqi", "Vith" },
	{ "Warang Citi", "Wara" },
	{ "Wancho", "Wcho" },
	{ "Woleai", "Wole" },
	{ "Old Persian", "Xpeo" },
	{ "Cuneiform", "Xsux" },
	{ "Yezidi", "Yezi" },
	{ "Yi", "Yiii" },
	{ "Zanabazar Square", "Zanb" },
	{ nullptr, nullptr }
};

// Plural rules.
// Reference:
// - https://cgit.git.savannah.gnu.org/cgit/gettext.git/tree/gettext-tools/src/plural-table.c
// - https://github.com/unicode-org/cldr/blob/release-47/common/supplemental/plurals.xml
static const char *plural_rules[][2] = {
	{ "bm bo dz hnj id ig ii in ja jbo jv jw kde kea km ko lkt lo ms my nqo osa root sah ses sg su th to tpi vi wo yo yue zh", "nplurals=1; plural=0;" },
	{ "af an asa ast az bal bem bez bg brx ca ce cgg chr ckb da de dv ee el en eo es et eu fi fo fur fy gl gsw ha haw he hu ia io it jgo ji jmc ka kaj kcg kk kkj kl ks ksb ku ky lb lg lij mas mgo ml mn mr nah nb nd ne nl nn nnh no nr ny nyn om or os pap ps pt rm rof rwk saq sc sd sdh seh sn so sq ss ssy st sv sw syr ta te teo tig tk tn tr ts ug ur uz ve vo vun wae xh xog yi", "nplurals=2; plural=(n != 1);" },
	{ "ak bho csw ff fr guw hy kab ln mg nso pa pt_BR si ti wa", "nplurals=2; plural=(n > 1);" },
	{ "is mk", "nplurals=2; plural=(n%10==1 && n%100!=11);" },
	{ "tzm", "nplurals=2; plural=(n<=1 || (n>=11 && n<=99));" },
	{ "am as bn doi fa gu hi kn pcm zu", "nplurals=2; plural=(n==0 || n==1);" },
	{ "ceb fil tl", "nplurals=2; plural=(n==1 || n==2 || n==3 || (n%10!=4 && n%10!=6 && n%10!=9));" },
	{ "prg", "nplurals=3; plural=(n%10==0 || (n%100>=11 && n%100<=19) ? 0 : n%10==1 && n%100!=11 ? 1 : 2);" },
	{ "lv", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2);" },
	{ "lt", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2);" },
	{ "be hr ru sr uk", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
	{ "bs sh", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<12 || n%100>14) ? 1 : 2);" },
	{ "lag", "nplurals=3; plural=(n==0 ? 0 : (n==0 || n==1) && n!=0 ? 1 : 2);" },
	{ "blo ksh", "nplurals=3; plural=(n==0 ? 0 : n==1 ? 1 : 2);" },
	{ "shi", "nplurals=3; plural=(n==0 || n==1 ? 0 : n>=2 && n<=10 ? 1 : 2);" },
	{ "pl", "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
	{ "mo", "nplurals=3; plural=(n==1 ? 0 : n==0 || (n!=1 && n%100>=1 && n%100<=19) ? 1 : 2);" },
	{ "iu iw naq sat se sma smi smj smn sms", "nplurals=3; plural=(n==1 ? 0 : n==2 ? 1 : 2);" },
	{ "cs sk", "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" },
	{ "ro", "nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;" },
	{ "ga", "nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;" },
	{ "sl", "nplurals=4; plural=(n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3);" },
	{ "dsb hsb", "nplurals=4; plural=(n%100==1 ? 0 : n%100==2 ? 1 : n%100>=3 && n%100<=4 ? 2 : 3);" },
	{ "gv", "nplurals=4; plural=(n%10==1 ? 0 : n%10==2 ? 1 : n%100==0 || n%100==20 || n%100==40 || n%100==60 || n%100==80 ? 2 : 3);" },
	{ "gd", "nplurals=4; plural=(n==1 || n==11 ? 0 : n==2 || n==12 ? 1 : (n>=3 && n<=10) || (n>=13 && n<=19) ? 2 : 3);" },
	{ "br", "nplurals=5; plural=(n%10==1 && n%100!=11 && n%100!=71 && n%100!=91 ? 0 : n%10==2 && n%100!=12 && n%100!=72 && n%100!=92 ? 1 : ((n%10>=3 && n%10<=4) || n%10==9) && (n%100<10 || n%100>19) && (n%100<70 || n%100>79) && (n%100<90 || n%100>99) ? 2 : n!=0 && n%1000000==0 ? 3 : 4);" },
	{ "mt", "nplurals=5; plural=(n==1 ? 0 : n==2 ? 1 : n==0 || (n%100>=3 && n%100<=10) ? 2 : n%100>=11 && n%100<=19 ? 3 : 4);" },
	{ "kw", "nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n%100==2 || n%100==22 || n%100==42 || n%100==62 || n%100==82 || (n%1000==0 && ((n%100000>=1000 && n%100000<=20000) || n%100000==40000 || n%100000==60000 || n%100000==80000)) || (n!=0 && n%1000000==100000) ? 2 : n%100==3 || n%100==23 || n%100==43 || n%100==63 || n%100==83 ? 3 : n!=1 && (n%100==1 || n%100==21 || n%100==41 || n%100==61 || n%100==81) ? 4 : 5);" },
	{ "ar ars", "nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5);" },
	{ "cy", "nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n==3 ? 3 : n==6 ? 4 : 5);" },
	{ nullptr, nullptr }
};

static const struct NumSystemData {
	const char *locales;
	const char32_t *digits;
	const char32_t *percent_sign;
	const char32_t *exp_l;
	const char32_t *exp_u;
} num_system_data[] = {
	// Eastern Arabic numerals.
	{ "ar ar_AE ar_BH ar_DJ ar_EG ar_ER ar_IL ar_IQ ar_JO ar_KM ar_KW ar_LB ar_MR ar_OM ar_PS ar_QA ar_SA ar_SD ar_SO ar_SS ar_SY ar_TD ar_YE ckb ckb_IQ ckb_IR sd sd_PK sd_Arab sd_Arab_PK", U"٠١٢٣٤٥٦٧٨٩٫", U"٪", U"اس", U"اس" },
	// Persian and Urdu numerals.
	{ "fa fa_AF fa_IR ks ks_IN ks_Arab ks_Arab_IN lrc lrc_IQ lrc_IR mzn mzn_IR pa_PK pa_Arab pa_Arab_PK ps ps_AF ps_PK ur_IN uz_AF uz_Arab uz_Arab_AF", U"۰۱۲۳۴۵۶۷۸۹٫", U"٪", U"اس", U"اس" },
	// Bengali numerals.
	{ "as as_IN bn bn_BD bn_IN mni mni_IN mni_Beng mni_Beng_IN", U"০১২৩৪৫৬৭৮৯.", U"%", U"e", U"E" },
	// Devanagari numerals.
	{ "mr mr_IN ne ne_IN ne_NP sa sa_IN", U"०१२३४५६७८९.", U"%", U"e", U"E" },
	// Dzongkha numerals.
	{ "dz dz_BT", U"༠༡༢༣༤༥༦༧༨༩.", U"%", U"e", U"E" },
	// Santali numerals.
	{ "sat sat_IN sat_Olck sat_Olck_IN", U"᱐᱑᱒᱓᱔᱕᱖᱗᱘᱙.", U"%", U"e", U"E" },
	// Burmese numerals.
	{ "my my_MM", U"၀၁၂၃၄၅၆၇၈၉.", U"%", U"e", U"E" },
	// Chakma numerals.
	{ "ccp ccp_BD ccp_IN", U"𑄶𑄷𑄸𑄹𑄺𑄻𑄼𑄽𑄾𑄿.", U"%", U"e", U"E" },
	// Adlam numerals.
	{ "ff ff_Adlm_BF ff_Adlm_CM ff_Adlm_GH ff_Adlm_GM ff_Adlm_GN ff_Adlm_GW ff_Adlm_LR ff_Adlm_MR ff_Adlm_NE ff_Adlm_NG ff_Adlm_SL ff_Adlm_SN", U"𞥐𞥑𞥒𞥓𞥔𞥕𞥖𞥗𞥘𞥙.", U"%", U"𞤉", U"𞤉" },
	// End sentinel.
	{ nullptr, nullptr, nullptr, nullptr, nullptr }
};

// --- End core/string/locales.h ---
#include "core/string/translation_server.h"

// --- translation_server.compat.inc ---
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


String TranslationServer::_standardize_locale_bind_compat_98972(const String& p_locale)
{
	return standardize_locale(p_locale, false);
}

void TranslationServer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


void TranslationServer::init_locale_info()
{
	// Init locale info.
	language_map.clear();
	int idx = 0;
	while (language_list[idx][0] != nullptr) {
		language_map[language_list[idx][0]] = String::utf8(language_list[idx][1]);
		idx++;
	}

	// Init locale-script map.
	locale_script_info.clear();
	idx = 0;
	while (locale_scripts[idx][0] != nullptr) {
		LocaleScriptInfo info;
		info.name = locale_scripts[idx][0];
		info.script = locale_scripts[idx][1];
		info.default_country = locale_scripts[idx][2];
		Vector<String> supported_countries = String(locale_scripts[idx][3]).split(",", false);
		for (int i = 0; i < supported_countries.size(); i++) {
			info.supported_countries.insert(supported_countries[i]);
		}
		locale_script_info.push_back(info);
		idx++;
	}

	// Init supported script list.
	script_map.clear();
	idx = 0;
	while (script_list[idx][0] != nullptr) {
		script_map[script_list[idx][1]] = String::utf8(script_list[idx][0]);
		idx++;
	}

	// Init regional variant map.
	variant_map.clear();
	idx = 0;
	while (locale_variants[idx][0] != nullptr) {
		variant_map[locale_variants[idx][0]] = locale_variants[idx][1];
		idx++;
	}

	// Init locale renames.
	locale_rename_map.clear();
	idx = 0;
	while (locale_renames[idx][0] != nullptr) {
		if (!String(locale_renames[idx][1]).is_empty()) {
			locale_rename_map[locale_renames[idx][0]] = locale_renames[idx][1];
		}
		idx++;
	}

	// Init locale scripts.
	language_script_map.clear();
	idx = 0;
	while (language_script_list[idx][0] != nullptr) {
		HashSet<String>& set = language_script_map[language_script_list[idx][0]];
		Vector<String> scripts = String(language_script_list[idx][1]).split(" ");
		for (const String& s : scripts) {
			set.insert(s);
		}
		idx++;
	}

	// Init country names.
	country_name_map.clear();
	idx = 0;
	while (country_names[idx][0] != nullptr) {
		country_name_map[String(country_names[idx][0])] = String::utf8(country_names[idx][1]);
		idx++;
	}

	// Init country renames.
	country_rename_map.clear();
	idx = 0;
	while (country_renames[idx][0] != nullptr) {
		if (!String(country_renames[idx][1]).is_empty()) {
			country_rename_map[country_renames[idx][0]] = country_renames[idx][1];
		}
		idx++;
	}

	// Init plural rules.
	plural_rules_map.clear();
	idx = 0;
	while (plural_rules[idx][0] != nullptr) {
		const Vector<String> rule_locs = String(plural_rules[idx][0]).split(" ");
		const String rule = String(plural_rules[idx][1]);
		for (const String& l : rule_locs) {
			plural_rules_map[l] = rule;
		}
		idx++;
	}

	// Init number systems.
	num_system_map.clear();
	idx = 0;
	while (num_system_data[idx].locales != nullptr) {
		const NumSystemData& nsd = num_system_data[idx];

		// These fields must not be empty.
		DEV_ASSERT(nsd.percent_sign && nsd.percent_sign[0] != '\0');
		DEV_ASSERT(nsd.digits && nsd.digits[0] != '\0');
		DEV_ASSERT(nsd.exp_l && nsd.exp_l[0] != '\0');
		DEV_ASSERT(nsd.exp_u && nsd.exp_u[0] != '\0');
		DEV_ASSERT(strlen(nsd.digits) == 11);

		const Vector<String> locales = String(nsd.locales).split(" ");
		for (const String& l : locales) {
			num_system_map[l] = idx;
		}
		idx++;
	}
}

TranslationServer::Locale::operator String() const
{
	String out = language;
	if (!script.is_empty()) {
		out = out + "_" + script;
	}
	if (!country.is_empty()) {
		out = out + "_" + country;
	}
	if (!variant.is_empty()) {
		out = out + "_" + variant;
	}
	return out;
}

TranslationServer::Locale::Locale(
	const String& p_locale, bool p_add_defaults)
{
	// Replaces '-' with '_' for macOS style locales.
	String univ_locale = p_locale.replace_char('-', '_');

	// Extract locale elements.
	Vector<String> locale_elements = univ_locale.get_slicec('@', 0).split("_");
	language = locale_elements[0];
	if (locale_elements.size() >= 2) {
		if (locale_elements[1].length() == 4 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_lower_case(locale_elements[1][1]) &&
			is_ascii_lower_case(locale_elements[1][2]) &&
			is_ascii_lower_case(locale_elements[1][3])) {
			script = locale_elements[1];
		}
		if (locale_elements[1].length() == 2 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_upper_case(locale_elements[1][1])) {
			country = locale_elements[1];
		}
	}
	if (locale_elements.size() >= 3) {
		if (locale_elements[2].length() == 2 && is_ascii_upper_case(locale_elements[2][0]) &&
			is_ascii_upper_case(locale_elements[2][1])) {
			country = locale_elements[2];
		}
		else if (TranslationServer::variant_map.has(locale_elements[2].to_lower()) &&
				   TranslationServer::variant_map[locale_elements[2].to_lower()] == language) {
			variant = locale_elements[2].to_lower();
		}
	}
	if (locale_elements.size() >= 4) {
		if (TranslationServer::variant_map.has(locale_elements[3].to_lower()) &&
			TranslationServer::variant_map[locale_elements[3].to_lower()] == language) {
			variant = locale_elements[3].to_lower();
		}
	}

	// Try extract script and variant from the extra part.
	Vector<String> script_extra = univ_locale.get_slicec('@', 1).split(";");
	for (int i = 0; i < script_extra.size(); i++) {
		if (script_extra[i].to_lower() == "cyrillic") {
			script = "Cyrl";
			break;
		}
		else if (script_extra[i].to_lower() == "latin") {
			script = "Latn";
			break;
		}
		else if (script_extra[i].to_lower() == "devanagari") {
			script = "Deva";
			break;
		}
		else if (TranslationServer::variant_map.has(script_extra[i].to_lower()) &&
				   TranslationServer::variant_map[script_extra[i].to_lower()] == language) {
			variant = script_extra[i].to_lower();
		}
	}

	// Handles known non-ISO language names used e.g. on Windows.
	if (TranslationServer::locale_rename_map.has(language)) {
		language = TranslationServer::locale_rename_map[language];
	}

	// Handle country renames.
	if (TranslationServer::country_rename_map.has(country)) {
		country = TranslationServer::country_rename_map[country];
	}

	// Remove unsupported script codes.
	if (!TranslationServer::script_map.has(script)) {
		script = "";
	}

	// Add script code base on language and country codes for some ambiguous cases.
	if (p_add_defaults) {
		if (script.is_empty()) {
			for (int i = 0; i < TranslationServer::locale_script_info.size(); i++) {
				const LocaleScriptInfo& info = TranslationServer::locale_script_info[i];
				if (info.name == language) {
					if (country.is_empty() || info.supported_countries.has(country)) {
						script = info.script;
						break;
					}
				}
			}
		}
		if (!script.is_empty() && country.is_empty()) {
			// Add conntry code based on script for some ambiguous cases.
			for (int i = 0; i < TranslationServer::locale_script_info.size(); i++) {
				const LocaleScriptInfo& info = TranslationServer::locale_script_info[i];
				if (info.name == language && info.script == script) {
					country = info.default_country;
					break;
				}
			}
		}
	}
}

String TranslationServer::format_number(const String& p_string, const String& p_locale)
{
	ERR_FAIL_COND_V(p_locale.is_empty(), p_string);
	if (!num_system_map.has(p_locale)) {
		return p_string;
	}

	int index = num_system_map[p_locale];
	const NumSystemData& nsd = num_system_data[index];

	String res = p_string;
	res = res.replace("e", nsd.exp_l);
	res = res.replace("E", nsd.exp_u);
	char32_t* res_data = res.ptrw();
	for (int j = 0; j < res.length(); j++) {
		if (res_data[j] >= 0x30 && res_data[j] <= 0x39) {
			res_data[j] = nsd.digits[res_data[j] - 0x30];
		}
		else if (res_data[j] == '.' || res_data[j] == ',') {
			res_data[j] = nsd.digits[10];
		}
	}
	return res;
}

String TranslationServer::parse_number(const String& p_string, const String& p_locale)
{
	ERR_FAIL_COND_V(p_locale.is_empty(), p_string);
	if (!num_system_map.has(p_locale)) {
		return p_string;
	}

	int index = num_system_map[p_locale];
	const NumSystemData& nsd = num_system_data[index];

	String res = p_string;
	res = res.replace(nsd.exp_l, "e");
	res = res.replace(nsd.exp_u, "E");
	char32_t* res_data = res.ptrw();
	for (int j = 0; j < res.length(); j++) {
		if (res_data[j] == nsd.digits[10]) {
			res_data[j] = '.';
		}
		else {
			for (int k = 0; k < 10; k++) {
				if (res_data[j] == nsd.digits[k]) {
					res_data[j] = 0x30 + k;
				}
			}
		}
	}
	return res;
}

String TranslationServer::get_percent_sign(const String& p_locale)
{
	ERR_FAIL_COND_V(p_locale.is_empty(), "%");
	if (!num_system_map.has(p_locale)) {
		return "%";
	}

	int index = num_system_map[p_locale];
	return num_system_data[index].percent_sign;
}

String TranslationServer::standardize_locale(const String& p_locale, bool p_add_defaults)
{
	return Locale(p_locale, p_add_defaults).operator String();
}

int TranslationServer::compare_locales(const String& p_locale_a, const String& p_locale_b)
{
	if (p_locale_a == p_locale_b) {
		// Exact match.
		return 10;
	}

	const String cache_key = p_locale_a + "|" + p_locale_b;
	const int* cached_result = data->locale_compare_cache.getptr(cache_key);
	if (cached_result) {
		return *cached_result;
	}

	Locale locale_a = Locale(p_locale_a, true);
	Locale locale_b = Locale(p_locale_b, true);

	if (locale_a == locale_b) {
		// Exact match.
		data->locale_compare_cache.insert(cache_key, 10);
		return 10;
	}

	if (locale_a.language != locale_b.language) {
		// No match.
		data->locale_compare_cache.insert(cache_key, 0);
		return 0;
	}

	// Matching language, both locales have extra parts. Compare the
	// remaining elements. If both elements are non-empty, check the
	// match to increase or decrease the score. If either element or
	// both are empty, leave the score as is.
	int score = 5;
	if (!locale_a.script.is_empty() && !locale_b.script.is_empty()) {
		if (locale_a.script == locale_b.script) {
			score++;
		}
		else {
			score--;
		}
	}
	if (!locale_a.country.is_empty() && !locale_b.country.is_empty()) {
		if (locale_a.country == locale_b.country) {
			score++;
		}
		else {
			score--;
		}
	}
	if (!locale_a.variant.is_empty() && !locale_b.variant.is_empty()) {
		if (locale_a.variant == locale_b.variant) {
			score++;
		}
		else {
			score--;
		}
	}

	data->locale_compare_cache.insert(cache_key, score);
	return score;
}

String TranslationServer::get_locale_name(const String& p_locale)
{
	String lang_name, script_name, country_name;
	Vector<String> locale_elements = standardize_locale(p_locale).split("_");
	lang_name = locale_elements[0];
	if (locale_elements.size() >= 2) {
		if (locale_elements[1].length() == 4 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_lower_case(locale_elements[1][1]) &&
			is_ascii_lower_case(locale_elements[1][2]) &&
			is_ascii_lower_case(locale_elements[1][3])) {
			script_name = locale_elements[1];
		}
		if (locale_elements[1].length() == 2 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_upper_case(locale_elements[1][1])) {
			country_name = locale_elements[1];
		}
	}
	if (locale_elements.size() >= 3) {
		if (locale_elements[2].length() == 2 && is_ascii_upper_case(locale_elements[2][0]) &&
			is_ascii_upper_case(locale_elements[2][1])) {
			country_name = locale_elements[2];
		}
	}

	String name = get_language_name(lang_name);
	if (!script_name.is_empty()) {
		name = name + " (" + get_script_name(script_name) + ")";
	}
	if (!country_name.is_empty()) {
		name = name + ", " + get_country_name(country_name);
	}
	return name;
}

String TranslationServer::get_plural_rules(const String& p_locale)
{
	const String* rule = plural_rules_map.getptr(p_locale);
	if (rule) {
		return *rule;
	}

	Locale l = Locale(p_locale, false);
	if (!l.country.is_empty()) {
		rule = plural_rules_map.getptr(l.language + "_" + l.country);
		if (rule) {
			return *rule;
		}
	}
	rule = plural_rules_map.getptr(l.language);
	if (rule) {
		return *rule;
	}
	return String();
}

Vector<String> TranslationServer::get_all_languages()
{
	Vector<String> languages;

	for (const KeyValue<String, String>& E : language_map) {
		languages.push_back(E.key);
	}

	return languages;
}

String TranslationServer::get_language_name(const String& p_language)
{
	if (language_map.has(p_language)) {
		return language_map[p_language];
	}
	else {
		return p_language;
	}
}

Vector<String> TranslationServer::get_all_scripts()
{
	Vector<String> scripts;

	for (const KeyValue<String, String>& E : script_map) {
		scripts.push_back(E.key);
	}

	return scripts;
}

String TranslationServer::get_script_name(const String& p_script)
{
	if (script_map.has(p_script)) {
		return script_map[p_script];
	}
	else {
		return p_script;
	}
}

Vector<String> TranslationServer::get_all_countries()
{
	Vector<String> countries;

	for (const KeyValue<String, String>& E : country_name_map) {
		countries.push_back(E.key);
	}

	return countries;
}

String TranslationServer::get_country_name(const String& p_country)
{
	if (country_name_map.has(p_country)) {
		return country_name_map[p_country];
	}
	else {
		return p_country;
	}
}

void TranslationServer::set_locale(const String& p_locale)
{
	ERR_FAIL_COND_MSG(p_locale.is_empty(), "Locale cannot be an empty string.");

	String new_locale = standardize_locale(p_locale);
	if (data->locale == new_locale) {
		return;
	}

	data->locale = new_locale;
	ResourceLoader::reload_translation_remaps();
}

String TranslationServer::get_locale() { return data->locale; }

void TranslationServer::set_fallback_allowed(const bool& p_allow) { data->allow_fallback = p_allow; }

bool TranslationServer::is_fallback_allowed() { return data->allow_fallback; }

void TranslationServer::set_fallback_locale(const String& p_locale) { data->fallback = p_locale; }

String TranslationServer::get_fallback_locale() { return data->fallback; }

bool TranslationServer::is_script_suppored_by_locale(
	const String& p_locale, const String& p_script)
{
	Locale l = Locale(p_locale, true);
	if (l.script == p_script) {
		return true;
	}
	if (!language_script_map.has(l.language)) {
		return false;
	}
	return language_script_map[l.language].has(p_script);
}

PackedStringArray TranslationServer::get_loaded_locales()
{
	return data->main_domain->get_loaded_locales();
}

void TranslationServer::add_translation(const Ref<Translation>& p_translation)
{
	data->main_domain->add_translation(p_translation);
}

void TranslationServer::remove_translation(const Ref<Translation>& p_translation)
{
	data->main_domain->remove_translation(p_translation);
}

#ifndef DISABLE_DEPRECATED
Ref<Translation> TranslationServer::get_translation_object(const String& p_locale)
{
	return data->main_domain->get_translation_object(p_locale);
}
#endif

bool TranslationServer::has_translation(const Ref<Translation>& p_translation)
{
	return data->main_domain->has_translation(p_translation);
}

bool TranslationServer::has_translation_for_locale(const String& p_locale, bool p_exact)
{
	return data->main_domain->has_translation_for_locale(p_locale, p_exact);
}

void TranslationServer::clear() { data->main_domain->clear(); }

String TranslationServer::translate(
	const String& p_message, const String& p_context)
{
	return data->main_domain->translate(p_message, p_context);
}

String TranslationServer::translate_plural(const String& p_message,
	const String& p_message_plural, int p_n, const String& p_context)
{
	return data->main_domain->translate_plural(p_message, p_message_plural, p_n, p_context);
}

bool TranslationServer::has_domain(const String& p_domain)
{
	if (p_domain == String()) {
		return true;
	}
	return data->custom_domains.has(p_domain);
}

Ref<TranslationDomain> TranslationServer::get_or_add_domain(const String& p_domain)
{
	if (p_domain == String()) {
		return data->main_domain;
	}
	const Ref<TranslationDomain>* domain = data->custom_domains.getptr(p_domain);
	if (domain) {
		if (domain->is_valid()) {
			return *domain;
		}
		ERR_PRINT("Bug (please report): Found invalid translation domain.");
	}
	Ref<TranslationDomain> new_domain = memnew(TranslationDomain);
	data->custom_domains[p_domain] = new_domain;
	return new_domain;
}

void TranslationServer::remove_domain(const String& p_domain)
{
	ERR_FAIL_COND_MSG(p_domain == String(), "Cannot remove main translation domain.");
	data->custom_domains.erase(p_domain);
}

String TranslationServer::get_tool_locale()
{
#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint() ||
		Engine::is_project_manager_hint()) {
		if (data->editor_domain->has_translation_for_locale(data->locale, true)) {
			return data->locale;
		}
		return "en";
	}
#endif

	Ref<Translation> res;
	int best_score = 0;

	for (const Ref<Translation>& E : data->main_domain->get_translations()) {
		int score = TranslationServer::compare_locales(data->locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			res = E;
			best_score = score;
			if (score == 10) {
				return data->locale; // Exact match.
			}
		}
	}
	return res.is_valid() ? res->get_locale() : data->fallback;
}

bool TranslationServer::is_pseudolocalization_enabled()
{
	return data->main_domain->is_pseudolocalization_enabled();
}

void TranslationServer::set_pseudolocalization_enabled(bool p_enabled)
{
	data->main_domain->set_pseudolocalization_enabled(p_enabled);

	ResourceLoader::reload_translation_remaps();
}

String TranslationServer::pseudolocalize(const String& p_message)
{
	return data->main_domain->pseudolocalize(p_message);
}


void TranslationServer::load_project_translations(Ref<TranslationDomain> p_domain)
{
	DEV_ASSERT(p_domain.is_valid());

	p_domain->clear();
	const String prop = "internationalization/locale/translations";
	if (!ProjectSettings::has_setting(prop)) {
		return;
	}
}
void TranslationServer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) {}

void TranslationServer::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->main_domain.instantiate();

#ifdef TOOLS_ENABLED
	data->editor_domain = get_or_add_domain("godot.editor");
	data->property_domain = get_or_add_domain("godot.properties");
	data->doc_domain = get_or_add_domain("godot.documentation");
#endif // TOOLS_ENABLED

	init_locale_info();
}

void TranslationServer::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void TranslationServer::setup() {}

// ============================================================================
// Merged from core/string/node_path.cpp
// ============================================================================

#include "core/types.h"

void NodePath::_update_hash_cache() const
{
	String path = get_concatenated_names();
	String subpath = get_concatenated_subnames();
	uint32_t hash =
		HashMapHasherDefault::hash(Pair<const String&, const String&>(path, subpath));
	data->hash_cache = is_absolute() ? hash : ~hash;
	data->hash_cache_valid = true;
}

void NodePath::prepend_period()
{
	if (data->path.size() && data->path[0] != ".") {
		_copy_on_write();
		data->path.insert(0, ".");
		data->concatenated_path = String();
		data->hash_cache_valid = false;
	}
}

bool NodePath::is_absolute() const
{
	if (!data) {
		return false;
	}

	return data->absolute;
}

int NodePath::get_name_count() const
{
	if (!data) {
		return 0;
	}

	return data->path.size();
}

String NodePath::get_name(int p_idx) const
{
	ERR_FAIL_NULL_V(data, String());
	ERR_FAIL_INDEX_V(p_idx, data->path.size(), String());
	return data->path[p_idx];
}

int NodePath::get_subname_count() const
{
	if (!data) {
		return 0;
	}

	return data->subpath.size();
}

String NodePath::get_subname(int p_idx) const
{
	ERR_FAIL_NULL_V(data, String());
	ERR_FAIL_INDEX_V(p_idx, data->subpath.size(), String());
	return data->subpath[p_idx];
}

int NodePath::get_total_name_count() const
{
	if (!data) {
		return 0;
	}

	return data->path.size() + data->subpath.size();
}

void NodePath::unref()
{
	if (data && data->refcount.unref()) {
		memdelete(data);
	}
	data = nullptr;
}

bool NodePath::operator==(const NodePath& p_path) const
{
	if (data == p_path.data) {
		return true;
	}

	if (!data || !p_path.data) {
		return false;
	}

	if (data->hash_cache_valid && p_path.data->hash_cache_valid) {
		if (data->hash_cache != p_path.data->hash_cache) {
			return false;
		}
	}

	if (data->absolute != p_path.data->absolute) {
		return false;
	}

	int path_size = data->path.size();

	if (path_size != p_path.data->path.size()) {
		return false;
	}

	int subpath_size = data->subpath.size();

	if (subpath_size != p_path.data->subpath.size()) {
		return false;
	}

	const String* l_path_ptr = data->path.ptr();
	const String* r_path_ptr = p_path.data->path.ptr();

	for (int i = 0; i < path_size; i++) {
		if (l_path_ptr[i] != r_path_ptr[i]) {
			return false;
		}
	}

	const String* l_subpath_ptr = data->subpath.ptr();
	const String* r_subpath_ptr = p_path.data->subpath.ptr();

	for (int i = 0; i < subpath_size; i++) {
		if (l_subpath_ptr[i] != r_subpath_ptr[i]) {
			return false;
		}
	}

	return true;
}

bool NodePath::operator!=(const NodePath& p_path) const { return (!(*this == p_path)); }

void NodePath::operator=(const NodePath& p_path)
{
	if (this == &p_path) {
		return;
	}

	unref();

	if (p_path.data && p_path.data->refcount.ref()) {
		data = p_path.data;
	}
}

void NodePath::_copy_on_write()
{
	if (!data) {
		return; // No data; nothing to do.
	}

	if (data->refcount.get() == 1) {
		return; // Already the only owner of data.
	}

	// Make a copy.
	Data* new_data = memnew(Data);
	new_data->refcount.init();
	new_data->path = data->path;
	new_data->subpath = data->subpath;
	new_data->concatenated_path = data->concatenated_path;
	new_data->concatenated_subpath = data->concatenated_subpath;
	new_data->absolute = data->absolute;
	new_data->hash_cache_valid = data->hash_cache_valid;
	new_data->hash_cache = data->hash_cache;
	unref();
	data = new_data;
}

NodePath::operator String() const
{
	if (!data) {
		return String();
	}

	String ret;
	if (data->absolute) {
		ret = "/";
	}

	ret += get_concatenated_names();

	String subpath = get_concatenated_subnames();
	if (!subpath.is_empty()) {
		ret += ":" + subpath;
	}

	return ret;
}

Vector<String> NodePath::get_names() const
{
	if (data) {
		return data->path;
	}
	return Vector<String>();
}

Vector<String> NodePath::get_subnames() const
{
	if (data) {
		return data->subpath;
	}
	return Vector<String>();
}

String NodePath::get_concatenated_names() const
{
	ERR_FAIL_NULL_V(data, String());

	if (data->concatenated_path.is_empty()) {
		int pc = data->path.size();
		String concatenated;
		const String* sn = data->path.ptr();
		for (int i = 0; i < pc; i++) {
			if (i > 0) {
				concatenated += "/";
			}
			concatenated += sn[i];
		}
		data->concatenated_path = concatenated;
	}
	return data->concatenated_path;
}

String NodePath::get_concatenated_subnames() const
{
	ERR_FAIL_NULL_V(data, String());

	if (data->concatenated_subpath.is_empty()) {
		int spc = data->subpath.size();
		String concatenated;
		const String* ssn = data->subpath.ptr();
		for (int i = 0; i < spc; i++) {
			if (i > 0) {
				concatenated += ":";
			}
			concatenated += ssn[i];
		}
		data->concatenated_subpath = concatenated;
	}
	return data->concatenated_subpath;
}

NodePath NodePath::slice(int p_begin, int p_end) const
{
	const int name_count = get_name_count();
	const int total_count = get_total_name_count();

	int begin = CLAMP(p_begin, -total_count, total_count);
	if (begin < 0) {
		begin += total_count;
	}
	int end = CLAMP(p_end, -total_count, total_count);
	if (end < 0) {
		end += total_count;
	}
	const int sub_begin = MAX(begin - name_count, 0);
	const int sub_end = MAX(end - name_count, 0);

	const Vector<String> names = get_names().slice(begin, end);
	const Vector<String> sub_names = get_subnames().slice(sub_begin, sub_end);
	const bool absolute = is_absolute() && (begin == 0);
	return NodePath(names, sub_names, absolute);
}

NodePath NodePath::rel_path_to(const NodePath& p_np) const
{
	ERR_FAIL_COND_V(!is_absolute(), NodePath());
	ERR_FAIL_COND_V(!p_np.is_absolute(), NodePath());

	Vector<String> src_dirs = get_names();
	Vector<String> dst_dirs = p_np.get_names();

	// find common parent
	int common_parent = 0;

	while (true) {
		if (src_dirs.size() == common_parent) {
			break;
		}
		if (dst_dirs.size() == common_parent) {
			break;
		}
		if (src_dirs[common_parent] != dst_dirs[common_parent]) {
			break;
		}
		common_parent++;
	}

	common_parent--;

	Vector<String> relpath;
	relpath.resize(src_dirs.size() + dst_dirs.size() + 1);

	String* relpath_ptr = relpath.ptrw();

	int path_size = 0;
	String back_str("..");
	for (int i = common_parent + 1; i < src_dirs.size(); i++) {
		relpath_ptr[path_size++] = back_str;
	}

	for (int i = common_parent + 1; i < dst_dirs.size(); i++) {
		relpath_ptr[path_size++] = dst_dirs[i];
	}

	if (path_size == 0) {
		relpath_ptr[path_size++] = ".";
	}

	relpath.resize(path_size);

	return NodePath(relpath, p_np.get_subnames(), false);
}

NodePath NodePath::get_as_property_path() const
{
	if (!data || !data->path.size()) {
		return *this;
	}
	else {
		Vector<String> new_path = data->subpath;

		String initial_subname = data->path[0];

		for (int i = 1; i < data->path.size(); i++) {
			initial_subname += "/" + data->path[i];
		}
		new_path.insert(0, initial_subname);

		return NodePath(Vector<String>(), new_path, false);
	}
}

bool NodePath::is_empty() const { return !data; }

void NodePath::simplify()
{
	if (!data) {
		return;
	}
	_copy_on_write();
	for (int i = 0; i < data->path.size(); i++) {
		if (data->path.size() == 1) {
			break;
		}
		if (data->path[i] == ".") {
			data->path.remove_at(i);
			i--;
		}
		else if (i > 0 && data->path[i] == ".." && data->path[i - 1] != "." &&
				   data->path[i - 1] != "..") {
			// remove both
			data->path.remove_at(i - 1);
			data->path.remove_at(i - 1);
			i -= 2;
			if (data->path.is_empty()) {
				data->path.push_back(".");
				break;
			}
		}
	}
	data->concatenated_path = String();
	data->hash_cache_valid = false;
}

NodePath NodePath::simplified() const
{
	NodePath np = *this;
	np.simplify();
	return np;
}

NodePath::NodePath(const Vector<String>& p_path, bool p_absolute)
{
	if (p_path.is_empty() && !p_absolute) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = p_absolute;
	data->path = p_path;
	data->hash_cache_valid = false;
}

NodePath::NodePath(
	const Vector<String>& p_path, const Vector<String>& p_subpath, bool p_absolute)
{
	if (p_path.is_empty() && p_subpath.is_empty() && !p_absolute) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = p_absolute;
	data->path = p_path;
	data->subpath = p_subpath;
	data->hash_cache_valid = false;
}

NodePath::NodePath(const NodePath& p_path)
{
	if (p_path.data && p_path.data->refcount.ref()) {
		data = p_path.data;
	}
}

NodePath::NodePath(const String& p_path)
{
	if (p_path.length() == 0) {
		return;
	}

	String path = p_path;
	Vector<String> subpath;

	bool absolute = (path[0] == '/');
	bool last_is_slash = true;
	int slices = 0;
	int subpath_pos = path.find_char(':');

	if (subpath_pos != -1) {
		int from = subpath_pos + 1;

		for (int i = from; i <= path.length(); i++) {
			if (path[i] == ':' || path[i] == 0) {
				String str = path.substr(from, i - from);
				if (str.is_empty()) {
					if (path[i] == 0) {
						continue; // Allow end-of-path :
					}

					ERR_FAIL_MSG(vformat("Invalid NodePath '%s'.", p_path));
				}
				subpath.push_back(str);

				from = i + 1;
			}
		}

		path = path.substr(0, subpath_pos);
	}

	for (int i = (int)absolute; i < path.length(); i++) {
		if (path[i] == '/') {
			last_is_slash = true;
		}
		else {
			if (last_is_slash) {
				slices++;
			}

			last_is_slash = false;
		}
	}

	if (slices == 0 && !absolute && !subpath.size()) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = absolute;
	data->subpath = subpath;
	data->hash_cache_valid = false;

	if (slices == 0) {
		return;
	}
	data->path.resize(slices);
	last_is_slash = true;
	int from = (int)absolute;
	int slice = 0;

	for (int i = (int)absolute; i < path.length() + 1; i++) {
		if (path[i] == '/' || path[i] == 0) {
			if (!last_is_slash) {
				String name = path.substr(from, i - from);
				ERR_FAIL_INDEX(slice, data->path.size());
				data->path.write[slice++] = name;
			}
			from = i + 1;
			last_is_slash = true;
		}
		else {
			last_is_slash = false;
		}
	}
}

NodePath::~NodePath() { unref(); }

// ==========================================================================
// print_string.cpp
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

#include <cstdio>
#include "core/types.h"
#include "core/os/os.h"
#include "core/string/print_string.h"

static PrintHandlerList* print_handler_list = nullptr;
static thread_local bool is_printing = false;

static void __print_fallback(const String& p_string, bool p_err, bool p_reentrance)
{
	if (p_reentrance) {
		fprintf(p_err ? stderr : stdout,
			"While attempting to print an error, another error was printed:\n");
	}

	fprintf(p_err ? stderr : stdout, "%s\n", p_string.utf8().get_data());
}

void add_print_handler(PrintHandlerList* p_handler)
{
	_global_lock();
	p_handler->next = print_handler_list;
	print_handler_list = p_handler;
	_global_unlock();
}

void remove_print_handler(const PrintHandlerList* p_handler)
{
	_global_lock();

	PrintHandlerList* prev = nullptr;
	PrintHandlerList* l = print_handler_list;

	while (l) {
		if (l == p_handler) {
			if (prev) {
				prev->next = l->next;
			}
			else {
				print_handler_list = l->next;
			}
			break;
		}
		prev = l;
		l = l->next;
	}
	// OS::print("print handler list is %p\n",print_handler_list);

	_global_unlock();
	ERR_FAIL_NULL(l);
}

void __print_line(const String& p_string)
{
	if (!CoreGlobals::print_line_enabled) {
		return;
	}

	if (!CoreGlobals::print_ready) {
		__print_fallback(p_string, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(p_string, false, true);
		return;
	}

	is_printing = true;

	OS::print("%s\n", p_string.utf8().get_data());

	_global_lock();
	PrintHandlerList* l = print_handler_list;
	while (l) {
		l->printfunc(l->userdata, p_string, false, false);
		l = l->next;
	}

	_global_unlock();

	is_printing = false;
}

void __print_line_rich(const String& p_string)
{
	if (!CoreGlobals::print_line_enabled) {
		return;
	}

	// Convert a subset of BBCode tags to ANSI escape codes for correct display in the terminal.
	// Support of those ANSI escape codes varies across terminal emulators,
	// especially for italic and strikethrough.

	String output;
	int pos = 0;
	bool in_named_url = false;
	while (pos <= p_string.length()) {
		int brk_pos = p_string.find_char('[', pos);

		if (brk_pos < 0) {
			brk_pos = p_string.length();
		}

		String txt = brk_pos > pos ? p_string.substr(pos, brk_pos - pos) : "";
		if (brk_pos == p_string.length()) {
			output += txt;
			break;
		}

		int brk_end = p_string.find_char(']', brk_pos + 1);

		if (brk_end == -1) {
			txt += p_string.substr(brk_pos);
			output += txt;
			break;
		}
		pos = brk_end + 1;
		output += txt;

		String tag = p_string.substr(brk_pos + 1, brk_end - brk_pos - 1);
		if (tag == "b") {
			output += "\u001b[1m";
		}
		else if (tag == "/b") {
			output += "\u001b[22m";
		}
		else if (tag == "i") {
			output += "\u001b[3m";
		}
		else if (tag == "/i") {
			output += "\u001b[23m";
		}
		else if (tag == "u") {
			output += "\u001b[4m";
		}
		else if (tag == "/u") {
			output += "\u001b[24m";
		}
		else if (tag == "s") {
			output += "\u001b[9m";
		}
		else if (tag == "/s") {
			output += "\u001b[29m";
		}
		else if (tag == "indent") {
			output += "    ";
		}
		else if (tag == "/indent") {
			output += "";
		}
		else if (tag == "code") {
			output += "\u001b[2m";
		}
		else if (tag == "/code") {
			output += "\u001b[22m";
		}
		else if (tag.begins_with("url=")) {
			// Support named URLs using OSC 8 escape codes:
			// <https://gist.github.com/egmontkob/eb114294efbcd5adb1944c9f3cb5feda>
			in_named_url = true;
			const String url_link = tag.substr(strlen("url="));
			output += vformat("\u001b]8;;%s\u001b\\", url_link);
		}
		else if (tag == "url") {
			output += "";
		}
		else if (tag == "/url") {
			if (in_named_url) {
				output += "\u001b]8;;\u001b\\";
				in_named_url = false;
			}
			else {
				// While it's legal to close an URL that was never opened using OSC 8 escape codes,
				// it would result in the code being printed to unsupported terminal emulators
				// when using unnamed URLs.
				output += "";
			}
		}
		else if (tag == "center") {
			output += "\n\t\t\t";
		}
		else if (tag == "/center") {
			output += "";
		}
		else if (tag == "right") {
			output += "\n\t\t\t\t\t\t";
		}
		else if (tag == "/right") {
			output += "";
		}
		else if (tag.begins_with("color=")) {
			String color_name = tag.trim_prefix("color=");
			if (color_name == "black") {
				output += "\u001b[30m";
			}
			else if (color_name == "red") {
				output += "\u001b[91m";
			}
			else if (color_name == "green") {
				output += "\u001b[92m";
			}
			else if (color_name == "lime") {
				output += "\u001b[92m";
			}
			else if (color_name == "yellow") {
				output += "\u001b[93m";
			}
			else if (color_name == "blue") {
				output += "\u001b[94m";
			}
			else if (color_name == "magenta") {
				output += "\u001b[95m";
			}
			else if (color_name == "pink") {
				output += "\u001b[38;5;218m";
			}
			else if (color_name == "purple") {
				output += "\u001b[38;5;98m";
			}
			else if (color_name == "cyan") {
				output += "\u001b[96m";
			}
			else if (color_name == "white") {
				output += "\u001b[97m";
			}
			else if (color_name == "orange") {
				output += "\u001b[38;5;208m";
			}
			else if (color_name == "gray") {
				output += "\u001b[90m";
			}
			else {
				Color c = Color::from_string(color_name, Color());
				output += vformat("\u001b[38;2;%d;%d;%dm", c.r * 255, c.g * 255, c.b * 255);
			}
		}
		else if (tag == "/color") {
			output += "\u001b[39m";
		}
		else if (tag.begins_with("bgcolor=")) {
			String color_name = tag.trim_prefix("bgcolor=");
			if (color_name == "black") {
				output += "\u001b[40m";
			}
			else if (color_name == "red") {
				output += "\u001b[101m";
			}
			else if (color_name == "green") {
				output += "\u001b[102m";
			}
			else if (color_name == "lime") {
				output += "\u001b[102m";
			}
			else if (color_name == "yellow") {
				output += "\u001b[103m";
			}
			else if (color_name == "blue") {
				output += "\u001b[104m";
			}
			else if (color_name == "magenta") {
				output += "\u001b[105m";
			}
			else if (color_name == "pink") {
				output += "\u001b[48;5;218m";
			}
			else if (color_name == "purple") {
				output += "\u001b[48;5;98m";
			}
			else if (color_name == "cyan") {
				output += "\u001b[106m";
			}
			else if (color_name == "white") {
				output += "\u001b[107m";
			}
			else if (color_name == "orange") {
				output += "\u001b[48;5;208m";
			}
			else if (color_name == "gray") {
				output += "\u001b[100m";
			}
			else {
				Color c = Color::from_string(color_name, Color());
				output += vformat("\u001b[48;2;%d;%d;%dm", c.r * 255, c.g * 255, c.b * 255);
			}
		}
		else if (tag == "/bgcolor") {
			output += "\u001b[49m";
		}
		else if (tag.begins_with("fgcolor=")) {
			String color_name = tag.trim_prefix("fgcolor=");
			if (color_name == "black") {
				output += "\u001b[30;40m";
			}
			else if (color_name == "red") {
				output += "\u001b[91;101m";
			}
			else if (color_name == "green") {
				output += "\u001b[92;102m";
			}
			else if (color_name == "lime") {
				output += "\u001b[92;102m";
			}
			else if (color_name == "yellow") {
				output += "\u001b[93;103m";
			}
			else if (color_name == "blue") {
				output += "\u001b[94;104m";
			}
			else if (color_name == "magenta") {
				output += "\u001b[95;105m";
			}
			else if (color_name == "pink") {
				output += "\u001b[38;5;218;48;5;218m";
			}
			else if (color_name == "purple") {
				output += "\u001b[38;5;98;48;5;98m";
			}
			else if (color_name == "cyan") {
				output += "\u001b[96;106m";
			}
			else if (color_name == "white") {
				output += "\u001b[97;107m";
			}
			else if (color_name == "orange") {
				output += "\u001b[38;5;208;48;5;208m";
			}
			else if (color_name == "gray") {
				output += "\u001b[90;100m";
			}
			else {
				Color c = Color::from_string(color_name, Color());
				output += vformat("\u001b[38;2;%d;%d;%d;48;2;%d;%d;%dm", c.r * 255, c.g * 255,
					c.b * 255, c.r * 255, c.g * 255, c.b * 255);
			}
		}
		else if (tag == "/fgcolor") {
			output += "\u001b[39;49m";
		}
		else {
			output += "[";
			pos = brk_pos + 1;
		}
	}
	output += "\u001b[0m"; // Reset.

	if (!CoreGlobals::print_ready) {
		__print_fallback(output, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(output, false, true);
		return;
	}

	is_printing = true;

	OS::print_rich("%s\n", output.utf8().get_data());

	_global_lock();
	PrintHandlerList* l = print_handler_list;
	while (l) {
		l->printfunc(l->userdata, p_string, false, true);
		l = l->next;
	}

	_global_unlock();

	is_printing = false;
}

void print_raw(const String& p_string)
{
	if (!CoreGlobals::print_ready) {
		__print_fallback(p_string, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(p_string, true, true);
		return;
	}

	is_printing = true;

	OS::print("%s", p_string.utf8().get_data());

	is_printing = false;
}

void print_error(const String& p_string)
{
	if (!CoreGlobals::print_error_enabled) {
		return;
	}

	if (!CoreGlobals::print_ready) {
		__print_fallback(p_string, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(p_string, true, true);
		return;
	}

	is_printing = true;

	OS::printerr("%s\n", p_string.utf8().get_data());

	_global_lock();
	PrintHandlerList* l = print_handler_list;
	while (l) {
		l->printfunc(l->userdata, p_string, true, false);
		l = l->next;
	}

	_global_unlock();

	is_printing = false;
}

bool is_print_verbose_enabled() { return OS::is_stdout_verbose(); }

// ==========================================================================
// string_builder.cpp
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

#include "core/string/ustring.h"

StringBuilder &StringBuilder::append(const String &p_string) {
	if (p_string.is_empty()) {
		return *this;
	}

	strings.push_back(p_string);
	appended_strings.push_back(-1);

	string_length += p_string.length();

	return *this;
}

StringBuilder &StringBuilder::append(const char *p_cstring) {
	int32_t len = strlen(p_cstring);

	c_strings.push_back(p_cstring);
	appended_strings.push_back(len);

	string_length += len;

	return *this;
}

String StringBuilder::as_string() const {
	if (string_length == 0) {
		return "";
	}

	String string;
	string.resize_uninitialized(string_length + 1);
	char32_t *buffer = string.ptrw();

	int current_position = 0;

	int godot_string_elem = 0;
	int c_string_elem = 0;

	for (uint32_t i = 0; i < appended_strings.size(); i++) {
		const int32_t str_len = appended_strings[i];

		if (str_len == -1) {
			// Godot string
			const String &s = strings[godot_string_elem];

			memcpy(buffer + current_position, s.ptr(), s.length() * sizeof(char32_t));

			current_position += s.length();

			godot_string_elem++;
		} else {
			const char *s = c_strings[c_string_elem];

			for (int32_t j = 0; j < str_len; j++) {
				buffer[current_position + j] = s[j];
			}

			current_position += str_len;

			c_string_elem++;
		}
	}
	buffer[current_position] = 0;

	return string;
}

// ==========================================================================
// fuzzy_search.cpp
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

#include "core/string/translation_server.h"

static const String boundary_chars = "/\\-_. ";

static bool _is_valid_interval(const Vector2i& p_interval)
{
	// Empty intervals are represented as (-1, -1).
	return p_interval.x >= 0 && p_interval.y >= p_interval.x;
}

static Vector2i _extend_interval(const Vector2i& p_a, const Vector2i& p_b)
{
	if (!_is_valid_interval(p_a)) {
		return p_b;
	}
	if (!_is_valid_interval(p_b)) {
		return p_a;
	}
	return Vector2i(MIN(p_a.x, p_b.x), MAX(p_a.y, p_b.y));
}

static bool _is_word_boundary(const String& p_str, int p_index)
{
	if (p_index == -1 || p_index == p_str.size()) {
		return true;
	}
	return boundary_chars.find_char(p_str[p_index]) != -1;
}

bool FuzzySearchToken::try_exact_match(
	FuzzyTokenMatch& p_match, const String& p_target, int p_offset) const
{
	p_match.token_idx = idx;
	p_match.token_length = string.length();
	int match_idx = p_target.find(string, p_offset);
	if (match_idx == -1) {
		return false;
	}
	p_match.add_substring(match_idx, string.length());
	return true;
}

bool FuzzySearchToken::try_fuzzy_match(
	FuzzyTokenMatch& p_match, const String& p_target, int p_offset, int p_miss_budget) const
{
	p_match.token_idx = idx;
	p_match.token_length = string.length();
	int run_start = -1;
	int run_len = 0;

	// Search for the subsequence p_token in p_target starting from p_offset, recording each
	// substring for later scoring and display.
	for (int i = 0; i < string.length(); i++) {
		int new_offset = p_target.find_char(string[i], p_offset);
		if (new_offset < 0) {
			p_miss_budget--;
			if (p_miss_budget < 0) {
				return false;
			}
		}
		else {
			if (run_start == -1 || p_offset != new_offset) {
				if (run_start != -1) {
					p_match.add_substring(run_start, run_len);
				}
				run_start = new_offset;
				run_len = 1;
			}
			else {
				run_len += 1;
			}
			p_offset = new_offset + 1;
		}
	}

	if (run_start != -1) {
		p_match.add_substring(run_start, run_len);
	}

	return true;
}

void FuzzyTokenMatch::add_substring(int p_substring_start, int p_substring_length)
{
	substrings.append(Vector2i(p_substring_start, p_substring_length));
	matched_length += p_substring_length;
	Vector2i substring_interval = {p_substring_start, p_substring_start + p_substring_length - 1};
	interval = _extend_interval(interval, substring_interval);
}

bool FuzzyTokenMatch::intersects(const Vector2i& p_other_interval) const
{
	if (!_is_valid_interval(interval) || !_is_valid_interval(p_other_interval)) {
		return false;
	}
	return interval.y >= p_other_interval.x && interval.x <= p_other_interval.y;
}

bool FuzzySearchMatch::_can_add_token_match(const FuzzyTokenMatch& p_match) const
{
	if (p_match.get_miss_count() > miss_budget) {
		return false;
	}

	if (p_match.intersects(match_interval)) {
		if (token_matches.size() == 1) {
			return false;
		}
		for (const FuzzyTokenMatch& existing_match : token_matches) {
			if (existing_match.intersects(p_match.interval)) {
				return false;
			}
		}
	}

	return true;
}

bool FuzzyTokenMatch::is_case_insensitive(const String& p_original, const String& p_adjusted) const
{
	for (const Vector2i& substr : substrings) {
		const int end = substr.x + substr.y;
		for (int i = substr.x; i < end; i++) {
			if (p_original[i] != p_adjusted[i]) {
				return true;
			}
		}
	}
	return false;
}

void FuzzySearchMatch::_score_token_match(FuzzyTokenMatch& p_match, bool p_case_insensitive) const
{
	// This can always be tweaked more. The intuition is that exact matches should almost always
	// be prioritized over broken up matches, and other criteria more or less act as tie breakers.

	p_match.score = -20 * p_match.get_miss_count() - (p_case_insensitive ? 3 : 0);

	for (const Vector2i& substring : p_match.substrings) {
		// Score longer substrings higher than short substrings.
		int substring_score = substring.y * substring.y;
		// Score matches deeper in path higher than shallower matches
		if (substring.x > dir_index) {
			substring_score *= 2;
		}
		// Score matches on a word boundary higher than matches within a word
		if (_is_word_boundary(target, substring.x - 1) ||
			_is_word_boundary(target, substring.x + substring.y)) {
			substring_score += 4;
		}
		// Score exact query matches higher than non-compact subsequence matches
		if (substring.y == p_match.token_length) {
			substring_score += 100;
		}
		p_match.score += substring_score;
	}
}

void FuzzySearchMatch::_maybe_apply_token_order_score_bonus()
{
	// This adds a small bonus to results which match tokens in the same order they appear in the
	// query.
	if (token_matches.is_empty()) {
		return;
	}

	int* token_range_starts = (int*)alloca(sizeof(int) * token_matches.size());

	for (const FuzzyTokenMatch& match : token_matches) {
		token_range_starts[match.token_idx] = match.interval.x;
	}

	for (int i = 1; i < token_matches.size(); i++) {
		// Individual tokens can match without a range if the missed-character budget allows for it.
		// If the i'th token matches in this manner, skip ahead so we check neither (i-1, i) nor (i,
		// i+1). It's safe that this skips i=0 since any valid start will be > -1.
		if (token_range_starts[i] == -1) {
			i++;
			continue;
		}
		if (token_range_starts[i - 1] > token_range_starts[i]) {
			return;
		}
	}

	score += 1;
}

void FuzzySearchMatch::_add_token_match(const FuzzyTokenMatch& p_match)
{
	score += p_match.score;
	match_interval = _extend_interval(match_interval, p_match.interval);
	miss_budget -= p_match.get_miss_count();
	token_matches.append(p_match);
}

static void remove_low_scores(Vector<Ref<FuzzySearchMatch>>& p_results, float p_cull_score)
{
	// Removes all results with score < p_cull_score in-place.
	int i = 0;
	int j = p_results.size() - 1;
	Ref<FuzzySearchMatch>* results = p_results.ptrw();

	while (true) {
		// Advances i to an element to remove and j to an element to keep.
		while (j >= i && results[j]->get_score() < p_cull_score) {
			j--;
		}
		while (i < j && results[i]->get_score() >= p_cull_score) {
			i++;
		}
		if (i >= j) {
			break;
		}
		results[i++] = results[j--];
	}

	p_results.resize(j + 1);
}

Vector<FuzzySearchToken> FuzzySearch::_get_tokens(const String& p_query) const
{
	Vector<FuzzySearchToken> tokens;

	for (const String& string : p_query.split(" ", false)) {
		tokens.append({
			static_cast<int>(tokens.size()),
			case_sensitive ? string : string.to_lower(),
		});
	}

	struct TokenComparator
	{
		bool operator()(const FuzzySearchToken& p_left, const FuzzySearchToken& p_right) const
		{
			if (p_left.string.length() == p_right.string.length()) {
				return p_left.idx < p_right.idx;
			}
			return p_left.string.length() > p_right.string.length();
		}
	};

	// Prioritize matching longer tokens before shorter ones since match overlaps are not accepted.
	tokens.sort_custom<TokenComparator>();
	return tokens;
}

void FuzzySearch::_sort_and_filter(Vector<Ref<FuzzySearchMatch>>& p_results) const
{
	if (p_results.is_empty()) {
		return;
	}

	if (filter_low_scores) {
		float avg_score = 0;
		float max_score = 0;

		for (const Ref<FuzzySearchMatch>& result : p_results) {
			avg_score += result->get_score();
			max_score = MAX(max_score, result->get_score());
		}

		avg_score /= p_results.size();
		float cull_score = MIN(filter_cutoff, Math::lerp(avg_score, max_score, filter_factor));
		remove_low_scores(p_results, cull_score);
	}

	struct FuzzySearchResultComparator
	{
		bool operator()(
			const Ref<FuzzySearchMatch>& p_lhs, const Ref<FuzzySearchMatch>& p_rhs) const
		{
			// Sort on (score, length, alphanumeric) to ensure consistent ordering.
			if (p_lhs->score == p_rhs->score) {
				if (p_lhs->target.length() == p_rhs->target.length()) {
					return p_lhs->target < p_rhs->target;
				}
				return p_lhs->target.length() < p_rhs->target.length();
			}
			return p_lhs->score > p_rhs->score;
		}
	};

	SortArray<Ref<FuzzySearchMatch>, FuzzySearchResultComparator> sorter;

	if (p_results.size() > max_results) {
		sorter.partial_sort(0, p_results.size(), max_results, p_results.ptrw());
		p_results.resize(max_results);
	}
	else {
		sorter.sort(p_results.ptrw(), p_results.size());
	}
}

void FuzzySearch::set_case_sensitive(bool p_case_sensitive) { case_sensitive = p_case_sensitive; }

bool FuzzySearch::_search_tokens(const Vector<FuzzySearchToken>& p_tokens, const String& p_target,
	Ref<FuzzySearchMatch>& r_result) const
{
	r_result->target = p_target;
	r_result->dir_index = p_target.rfind_char('/');
	r_result->miss_budget = max_misses;
	r_result->token_matches.reserve(p_tokens.size());

	String adjusted_target = case_sensitive ? p_target : p_target.to_lower();

	// For each token, eagerly generate subsequences starting from index 0 and keep the best scoring
	// one which does not conflict with prior token matches. This is not ensured to find the highest
	// scoring combination of matches, or necessarily the highest scoring single subsequence, as it
	// only considers eager subsequences for a given index, and likewise eagerly finds matches for
	// each token in sequence.
	for (const FuzzySearchToken& token : p_tokens) {
		FuzzyTokenMatch best_match;
		int offset = start_offset;

		while (true) {
			FuzzyTokenMatch match;
			if (exact_token_matches) {
				if (!token.try_exact_match(match, adjusted_target, offset)) {
					break;
				}
			}
			else {
				if (!token.try_fuzzy_match(match, adjusted_target, offset, r_result->miss_budget)) {
					break;
				}
			}
			if (r_result->_can_add_token_match(match)) {
				r_result->_score_token_match(
					match, match.is_case_insensitive(p_target, adjusted_target));
				if (best_match.token_idx == -1 || best_match.score < match.score) {
					best_match = match;
				}
			}
			if (_is_valid_interval(match.interval)) {
				offset = match.interval.x + 1;
			}
			else {
				break;
			}
		}

		if (best_match.token_idx == -1) {
			return false;
		}

		r_result->_add_token_match(best_match);
	}

	if (r_result->match_interval.x == -1) {
		// Reject matches which rely entirely on misses.
		return false;
	}

	r_result->_maybe_apply_token_order_score_bonus();
	return true;
}

Ref<FuzzySearchMatch> FuzzySearch::search(const String& p_query, const String& p_target) const
{
	Ref<FuzzySearchMatch> result;
	result.instantiate();
	if (_search_tokens(_get_tokens(p_query), p_target, result)) {
		return result;
	}
	return nullptr;
}

Vector<Ref<FuzzySearchMatch>> FuzzySearch::search_all(
	const String& p_query, const PackedStringArray& p_targets) const
{
	Vector<Ref<FuzzySearchMatch>> results;
	const Vector<FuzzySearchToken> tokens = _get_tokens(p_query);

	for (int i = 0; i < p_targets.size(); i++) {
		Ref<FuzzySearchMatch> result;
		result.instantiate();
		result->original_index = i;
		if (_search_tokens(tokens, p_targets[i], result)) {
			results.append(result);
		}
	}

	_sort_and_filter(results);
	return results;
}
