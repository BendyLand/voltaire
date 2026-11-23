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
#include "core/core_globals.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/time.h"
#include "core/templates/rb_set.h"
#include "core/os/os.h"
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


// --- Merged from core/io/stream_peer.cpp ---
#include "core/io/marshalls.h"
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
#include "core/io/marshalls.h"
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
#include "core/io/resource_saver.h"

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
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/math/random_pcg.h"
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
#include "core/math/random_pcg.h"
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
#include "scene/resources/packed_scene.h"

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
