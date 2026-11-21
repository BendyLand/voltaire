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
#include "core/crypto/crypto_core.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/marshalls.h"
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

#include "core/io/zip_io.h"

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
#include "core/error/error_list.h"
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

#include "zip_io.h"

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

#include "core/crypto/crypto_core.h"
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
#include "marshalls.h"
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

#include "core/io/marshalls.h"
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
#include "core/string/string_builder.h"

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
#include "json.h"

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
#include "core/io/json.h"
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

#include "core/crypto/crypto_core.h"
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
#include "xml_parser.h"

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
#include "core/string/translation.h"

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
#include "core/core_globals.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/time.h"
#include "core/templates/rb_set.h"
#include "logger.h"
#include "modules/modules_enabled.gen.h" // For regex.

#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
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
