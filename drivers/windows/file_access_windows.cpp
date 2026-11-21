/**************************************************************************/
/*  file_access_windows.cpp                                               */
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

#ifdef WINDOWS_ENABLED

#include "file_access_windows.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/string/print_string.h"

#include <io.h>
#include <share.h> // _SH_DENYNO
#include <shlwapi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tchar.h>

#include <cerrno>
#include <cwchar>

#ifdef _MSC_VER
#define S_ISREG(m) ((m) & _S_IFREG)
#endif

void FileAccessWindows::check_errors(bool p_write) const {
	ERR_FAIL_NULL(f);

	last_error = OK;
	if (ferror(f)) {
		if (p_write) {
			last_error = ERR_FILE_CANT_WRITE;
		} else {
			last_error = ERR_FILE_CANT_READ;
		}
	}
	if (!p_write && feof(f)) {
		last_error = ERR_FILE_EOF;
	}
}

bool FileAccessWindows::is_path_invalid(const String &p_path) {
	// Check for invalid operating system file.
	String fname = p_path.get_file().to_lower();
	if (fname.ends_with(".") || fname.ends_with(" ")) {
		return true;
	}

	int dot = fname.find_char('.');
	if (dot != -1) {
		fname = fname.substr(0, dot);
	}
	return invalid_files.has(fname);
}

String FileAccessWindows::fix_path(const String &p_path) const {
	String r_path = FileAccess::fix_path(p_path);

	if (r_path.is_relative_path()) {
		Char16String current_dir_name;
		size_t str_len = GetCurrentDirectoryW(0, nullptr);
		current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
		r_path = String::utf16((const char16_t *)current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/').path_join(r_path);
	}
	r_path = r_path.simplify_path();
	r_path = r_path.replace_char('/', '\\');
	if (!r_path.is_network_share_path() && !r_path.begins_with(R"(\\?\)")) {
		r_path = R"(\\?\)" + r_path;
	}
	return r_path;
}

Error FileAccessWindows::open_internal(const String &p_path, int p_mode_flags) {
	if (is_path_invalid(p_path)) {
#ifdef DEBUG_ENABLED
		if (p_mode_flags != READ) {
			WARN_PRINT("The path :" + p_path + " is a reserved Windows system pipe, so it can't be used for creating files.");
		}
#endif
		return ERR_INVALID_PARAMETER;
	}

	_close();

	path_src = p_path;
	path = fix_path(p_path);

	const WCHAR *mode_string;

	if (p_mode_flags == READ) {
		mode_string = L"rb";
	} else if (p_mode_flags == WRITE) {
		mode_string = L"wb";
	} else if (p_mode_flags == READ_WRITE) {
		mode_string = L"rb+";
	} else if (p_mode_flags == WRITE_READ) {
		mode_string = L"wb+";
	} else {
		return ERR_INVALID_PARAMETER;
	}

	if (path.ends_with(":\\") || path.ends_with(":")) {
		return ERR_FILE_CANT_OPEN;
	}
	DWORD file_attr = GetFileAttributesW((LPCWSTR)(path.utf16().get_data()));
	if (file_attr != INVALID_FILE_ATTRIBUTES && (file_attr & FILE_ATTRIBUTE_DIRECTORY)) {
		return ERR_FILE_CANT_OPEN;
	}

#ifdef TOOLS_ENABLED
	// Windows is case insensitive in the default configuration, but other platforms can be sensitive to it
	// To ease cross-platform development, we issue a warning if users try to access
	// a file using the wrong case (which *works* on Windows, but won't on other
	// platforms), we only check for relative paths, or paths in res:// or user://,
	// other paths aren't likely to be portable anyway.
	if (p_mode_flags == READ && (p_path.is_relative_path() || get_access_type() != ACCESS_FILESYSTEM)) {
		String base_path = FileAccess::fix_path(p_path);
		String working_path;
		String proper_path;

		if (get_access_type() == ACCESS_RESOURCES) {
			if (ProjectSettings::is_initialized()) {
				working_path = ProjectSettings::get_resource_path();
				if (!working_path.is_empty()) {
					base_path = working_path.path_to_file(base_path);
				}
			}
			proper_path = "res://";
		} else if (get_access_type() == ACCESS_USERDATA) {
			working_path = OS::get_user_data_dir();
			if (!working_path.is_empty()) {
				base_path = working_path.path_to_file(base_path);
			}
			proper_path = "user://";
		}
		working_path = fix_path(working_path);

		WIN32_FIND_DATAW d;
		Vector<String> parts = base_path.simplify_path().split("/");

		bool mismatch = false;

		for (const String &part : parts) {
			working_path = working_path + "\\" + part;

			HANDLE fnd = FindFirstFileW((LPCWSTR)(working_path.utf16().get_data()), &d);
			if (fnd == INVALID_HANDLE_VALUE) {
				mismatch = false;
				break;
			}

			const String fname = String::utf16((const char16_t *)(d.cFileName));

			FindClose(fnd);

			if (!mismatch) {
				mismatch = (part != fname && part.findn(fname) == 0);
			}

			proper_path = proper_path.path_join(fname);
		}

		if (mismatch) {
			WARN_PRINT("Case mismatch opening requested file '" + p_path + "', stored as '" + proper_path + "' in the filesystem. This file will not open when exported to other case-sensitive platforms.");
		}
	}
#endif

	if (is_backup_save_enabled() && p_mode_flags == WRITE) {
		save_path = path;
		// Create a temporary file in the same directory as the target file.
		// Note: do not use GetTempFileNameW, it's not long path aware!
		String tmpfile;
		uint64_t id = OS::get_ticks_usec();
		while (true) {
			tmpfile = path + itos(id++) + ".tmp";
			HANDLE handle = CreateFileW((LPCWSTR)tmpfile.utf16().get_data(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
				break;
			}
			if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION) {
				last_error = ERR_FILE_CANT_WRITE;
				return FAILED;
			}
		}
		path = tmpfile;
	}

	f = _wfsopen((LPCWSTR)(path.utf16().get_data()), mode_string, is_backup_save_enabled() ? ((p_mode_flags == READ) ? _SH_DENYWR : _SH_DENYRW) : _SH_DENYNO);

	if (f == nullptr) {
		switch (errno) {
			case ENOENT: {
				last_error = ERR_FILE_NOT_FOUND;
			} break;
			default: {
				last_error = ERR_FILE_CANT_OPEN;
			} break;
		}
		return last_error;
	} else {
		last_error = OK;
		flags = p_mode_flags;
		return OK;
	}
}

void FileAccessWindows::_close() {
	if (!f) {
		return;
	}

	fclose(f);
	f = nullptr;

	if (!save_path.is_empty()) {
		// This workaround of trying multiple times is added to deal with paranoid Windows
		// antiviruses that love reading just written files even if they are not executable, thus
		// locking the file and preventing renaming from happening.

		bool rename_error = true;
		const Char16String &path_utf16 = path.utf16();
		const Char16String &save_path_utf16 = save_path.utf16();
		for (int i = 0; i < 1000; i++) {
			if (ReplaceFileW((LPCWSTR)(save_path_utf16.get_data()), (LPCWSTR)(path_utf16.get_data()), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, nullptr, nullptr)) {
				rename_error = false;
			} else {
				// Either the target exists and is locked (temporarily, hopefully)
				// or it doesn't exist; let's assume the latter before re-trying.
				rename_error = MoveFileW((LPCWSTR)(path_utf16.get_data()), (LPCWSTR)(save_path_utf16.get_data())) == 0;
			}

			if (!rename_error) {
				break;
			}

			OS::delay_usec(1000);
		}

		if (rename_error) {
			if (close_fail_notify) {
				close_fail_notify(save_path);
			}
		}

		save_path = "";

		ERR_FAIL_COND_MSG(rename_error, "Safe save failed. This may be a permissions problem, but also may happen because you are running a paranoid antivirus. If this is the case, please switch to Windows Defender or disable the 'safe save' option in editor settings. This makes it work, but increases the risk of file corruption in a crash.");
	}
}

String FileAccessWindows::get_path() const {
	return path_src;
}

String FileAccessWindows::get_path_absolute() const {
	return path.trim_prefix(R"(\\?\)").replace_char('\\', '/');
}

bool FileAccessWindows::is_open() const {
	return (f != nullptr);
}

void FileAccessWindows::seek(uint64_t p_position) {
	ERR_FAIL_NULL(f);

	if (_fseeki64(f, p_position, SEEK_SET)) {
		check_errors();
	}
	prev_op = 0;
}

void FileAccessWindows::seek_end(int64_t p_position) {
	ERR_FAIL_NULL(f);

	if (_fseeki64(f, p_position, SEEK_END)) {
		check_errors();
	}
	prev_op = 0;
}

uint64_t FileAccessWindows::get_position() const {
	ERR_FAIL_NULL_V_MSG(f, 0, "File must be opened before use.");

	int64_t aux_position = _ftelli64(f);
	if (aux_position < 0) {
		check_errors();
	}
	return aux_position;
}

uint64_t FileAccessWindows::get_length() const {
	ERR_FAIL_NULL_V(f, 0);

	uint64_t pos = get_position();
	_fseeki64(f, 0, SEEK_END);
	uint64_t size = get_position();
	_fseeki64(f, pos, SEEK_SET);

	return size;
}

bool FileAccessWindows::eof_reached() const {
	return feof(f);
}

uint64_t FileAccessWindows::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_NULL_V(f, -1);
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	if (flags == READ_WRITE || flags == WRITE_READ) {
		if (prev_op == WRITE) {
			fflush(f);
		}
		prev_op = READ;
	}

	uint64_t read = fread(p_dst, 1, p_length, f);
	check_errors();

	return read;
}

Error FileAccessWindows::get_error() const {
	return last_error;
}

Error FileAccessWindows::resize(int64_t p_length) {
	ERR_FAIL_NULL_V_MSG(f, FAILED, "File must be opened before use.");
	errno_t res = _chsize_s(_fileno(f), p_length);
	switch (res) {
		case 0:
			return OK;
		case EACCES:
		case EBADF:
			return ERR_FILE_CANT_OPEN;
		case ENOSPC:
			return ERR_OUT_OF_MEMORY;
		case EINVAL:
			return ERR_INVALID_PARAMETER;
		default:
			return FAILED;
	}
}

void FileAccessWindows::flush() {
	ERR_FAIL_NULL(f);

	fflush(f);
	if (prev_op == WRITE) {
		prev_op = 0;
	}
}

bool FileAccessWindows::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_NULL_V(f, false);
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);

	if (flags == READ_WRITE || flags == WRITE_READ) {
		if (prev_op == READ) {
			if (last_error != ERR_FILE_EOF) {
				fseek(f, 0, SEEK_CUR);
			}
		}
		prev_op = WRITE;
	}

	bool res = fwrite(p_src, 1, p_length, f) == (size_t)p_length;
	check_errors(true);
	return res;
}

bool FileAccessWindows::file_exists(const String &p_name) {
	if (is_path_invalid(p_name)) {
		return false;
	}

	String filename = fix_path(p_name);
	DWORD file_attr = GetFileAttributesW((LPCWSTR)(filename.utf16().get_data()));
	return (file_attr != INVALID_FILE_ATTRIBUTES) && !(file_attr & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t FileAccessWindows::_get_modified_time(const String &p_file) {
	if (is_path_invalid(p_file)) {
		return 0;
	}

	String file = fix_path(p_file);
	if (file.ends_with("\\") && file != "\\") {
		file = file.substr(0, file.length() - 1);
	}

	HANDLE handle = CreateFileW((LPCWSTR)(file.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		FILETIME ft_create, ft_write;

		bool status = GetFileTime(handle, &ft_create, nullptr, &ft_write);

		CloseHandle(handle);

		if (status) {
			uint64_t ret = 0;

			// If write time is invalid, fallback to creation time.
			if (ft_write.dwHighDateTime == 0 && ft_write.dwLowDateTime == 0) {
				ret = ft_create.dwHighDateTime;
				ret <<= 32;
				ret |= ft_create.dwLowDateTime;
			} else {
				ret = ft_write.dwHighDateTime;
				ret <<= 32;
				ret |= ft_write.dwLowDateTime;
			}

			const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
			const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

			if (ret >= TICKS_TO_UNIX_EPOCH) {
				return (ret - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
			}
		}
	}

	return 0;
}

uint64_t FileAccessWindows::_get_access_time(const String &p_file) {
	if (is_path_invalid(p_file)) {
		return 0;
	}

	String file = fix_path(p_file);
	if (file.ends_with("\\") && file != "\\") {
		file = file.substr(0, file.length() - 1);
	}

	HANDLE handle = CreateFileW((LPCWSTR)(file.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		FILETIME ft_create, ft_access;

		bool status = GetFileTime(handle, &ft_create, &ft_access, nullptr);

		CloseHandle(handle);

		if (status) {
			uint64_t ret = 0;

			// If access time is invalid, fallback to creation time.
			if (ft_access.dwHighDateTime == 0 && ft_access.dwLowDateTime == 0) {
				ret = ft_create.dwHighDateTime;
				ret <<= 32;
				ret |= ft_create.dwLowDateTime;
			} else {
				ret = ft_access.dwHighDateTime;
				ret <<= 32;
				ret |= ft_access.dwLowDateTime;
			}

			const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
			const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

			if (ret >= TICKS_TO_UNIX_EPOCH) {
				return (ret - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
			}
		}
	}

	ERR_FAIL_V_MSG(0, "Failed to get access time for: " + p_file + "");
}

int64_t FileAccessWindows::_get_size(const String &p_file) {
	if (is_path_invalid(p_file)) {
		return 0;
	}

	String file = fix_path(p_file);
	if (file.ends_with("\\") && file != "\\") {
		file = file.substr(0, file.length() - 1);
	}

	DWORD file_attr = GetFileAttributesW((LPCWSTR)(file.utf16().get_data()));
	HANDLE handle = CreateFileW((LPCWSTR)(file.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle != INVALID_HANDLE_VALUE && !(file_attr & FILE_ATTRIBUTE_DIRECTORY)) {
		LARGE_INTEGER fsize;

		bool status = GetFileSizeEx(handle, &fsize);

		CloseHandle(handle);

		if (status) {
			return (int64_t)fsize.QuadPart;
		}
	}
	ERR_FAIL_V_MSG(-1, "Failed to get size for: " + p_file + "");
}

uint32_t FileAccessWindows::_get_unix_permissions(const String &p_file) {
	return 0;
}

Error FileAccessWindows::_set_unix_permissions(const String &p_file, uint32_t p_permissions) {
	return ERR_UNAVAILABLE;
}

bool FileAccessWindows::_get_hidden_attribute(const String &p_file) {
	String file = fix_path(p_file);

	DWORD attrib = GetFileAttributesW((LPCWSTR)file.utf16().get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, false, "Failed to get attributes for: " + p_file);
	return (attrib & FILE_ATTRIBUTE_HIDDEN);
}

Error FileAccessWindows::_set_hidden_attribute(const String &p_file, bool p_hidden) {
	String file = fix_path(p_file);
	const Char16String &file_utf16 = file.utf16();

	DWORD attrib = GetFileAttributesW((LPCWSTR)file_utf16.get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, FAILED, "Failed to get attributes for: " + p_file);
	BOOL ok;
	if (p_hidden) {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib | FILE_ATTRIBUTE_HIDDEN);
	} else {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib & ~FILE_ATTRIBUTE_HIDDEN);
	}
	ERR_FAIL_COND_V_MSG(!ok, FAILED, "Failed to set attributes for: " + p_file);

	return OK;
}

bool FileAccessWindows::_get_read_only_attribute(const String &p_file) {
	String file = fix_path(p_file);

	DWORD attrib = GetFileAttributesW((LPCWSTR)file.utf16().get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, false, "Failed to get attributes for: " + p_file);
	return (attrib & FILE_ATTRIBUTE_READONLY);
}

Error FileAccessWindows::_set_read_only_attribute(const String &p_file, bool p_ro) {
	String file = fix_path(p_file);
	const Char16String &file_utf16 = file.utf16();

	DWORD attrib = GetFileAttributesW((LPCWSTR)file_utf16.get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, FAILED, "Failed to get attributes for: " + p_file);
	BOOL ok;
	if (p_ro) {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib | FILE_ATTRIBUTE_READONLY);
	} else {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib & ~FILE_ATTRIBUTE_READONLY);
	}
	ERR_FAIL_COND_V_MSG(!ok, FAILED, "Failed to set attributes for: " + p_file);

	return OK;
}

PackedByteArray FileAccessWindows::_get_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), PackedByteArray());

	String file = fix_path(p_file);
	file += ":" + p_attribute_name;
	const Char16String &file_utf16 = file.utf16();

	PackedByteArray data;
	HANDLE h = CreateFileW((LPCWSTR)file_utf16.get_data(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h != INVALID_HANDLE_VALUE) {
		size_t bytes_in_buffer = 0;
		const int CHUNK_SIZE = 4096;

		DWORD read = 0;
		for (;;) {
			data.resize(bytes_in_buffer + CHUNK_SIZE);
			bool success = ReadFile(h, data.ptrw() + bytes_in_buffer, CHUNK_SIZE, &read, nullptr);
			if (!success || read == 0) {
				break;
			}
			bytes_in_buffer += read;
		}
		data.resize(bytes_in_buffer);
		CloseHandle(h);
	}
	return data;
}

Error FileAccessWindows::_set_extended_attribute(const String &p_file, const String &p_attribute_name, const PackedByteArray &p_data) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

	String file = fix_path(p_file);
	file += ":" + p_attribute_name;
	const Char16String &file_utf16 = file.utf16();

	HANDLE h = CreateFileW((LPCWSTR)file_utf16.get_data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return FAILED;
	}

	DWORD written = 0;
	bool ok = true;
	if (p_data.size() > 0) {
		ok = WriteFile(h, p_data.ptr(), p_data.size(), &written, nullptr);
	}
	CloseHandle(h);

	ERR_FAIL_COND_V_MSG(!ok || written != p_data.size(), FAILED, "Failed to set extended attributes for: " + p_file);

	return OK;
}

Error FileAccessWindows::_remove_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

	String file = fix_path(p_file);
	file += ":" + p_attribute_name;
	const Char16String &file_utf16 = file.utf16();

	return DeleteFileW((LPCWSTR)(file_utf16.get_data())) != 0 ? OK : FAILED;
}

PackedStringArray FileAccessWindows::_get_extended_attributes_list(const String &p_file) {
	PackedStringArray ret;
	String file = fix_path(p_file);

	char info_block[65536] = {};
	PFILE_STREAM_INFO stream_info = (PFILE_STREAM_INFO)info_block;

	HANDLE h = CreateFileW((LPCWSTR)file.utf16().get_data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return ret;
	}
	BOOL out = GetFileInformationByHandleEx(h, FileStreamInfo, info_block, sizeof(info_block));
	CloseHandle(h);
	if (!out) {
		return ret;
	}
	while (true) {
		if (stream_info->StreamNameLength != 0) {
			String name = String::utf16((const char16_t *)stream_info->StreamName, stream_info->StreamNameLength / sizeof(WCHAR));
			if (name.ends_with(":$DATA")) {
				name = name.trim_prefix(":").trim_suffix(":$DATA");
				if (!name.is_empty()) {
					ret.push_back(name);
				}
			}
		}
		if (stream_info->NextEntryOffset == 0) {
			break;
		}
		stream_info = (PFILE_STREAM_INFO)((LPBYTE)stream_info + stream_info->NextEntryOffset);
	}

	return ret;
}

void FileAccessWindows::close() {
	_close();
}

FileAccessWindows::~FileAccessWindows() {
	_close();
}

HashSet<String> FileAccessWindows::invalid_files;

void FileAccessWindows::initialize() {
	static const char *reserved_files[]{
		"con", "prn", "aux", "nul", "com0", "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9", "com¹", "com²", "com³", "lpt0", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9", "lpt¹", "lpt²", "lpt³", nullptr
	};
	int reserved_file_index = 0;
	while (reserved_files[reserved_file_index] != nullptr) {
		invalid_files.insert(String::utf8(reserved_files[reserved_file_index]));
		reserved_file_index++;
	}

	_setmaxstdio(8192);
	print_verbose(vformat("Maximum number of file handles: %d", _getmaxstdio()));
}

void FileAccessWindows::finalize() {
	invalid_files.clear();
}

#endif // WINDOWS_ENABLED

#ifdef S_ISREG
#undef S_ISREG
#endif

// ============================================================================
// Merged from drivers/windows/dir_access_windows.cpp
// ============================================================================

#if defined(WINDOWS_ENABLED)

#include "drivers/windows/file_access_windows.h"

#include "core/os/memory.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "drivers/windows/file_access_windows.h"

#include <windows.h>

#include <cstdio>
#include <cwchar>

typedef struct _NT_IO_STATUS_BLOCK {
	union {
		LONG Status;
		PVOID Pointer;
	} DUMMY;
	ULONG_PTR Information;
} NT_IO_STATUS_BLOCK;

typedef struct _NT_FILE_CASE_SENSITIVE_INFO {
	ULONG Flags;
} NT_FILE_CASE_SENSITIVE_INFO;

typedef enum _NT_FILE_INFORMATION_CLASS {
	FileCaseSensitiveInformation = 71,
} NT_FILE_INFORMATION_CLASS;

#define NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR 0x00000001

extern "C" NTSYSAPI LONG NTAPI NtQueryInformationFile(HANDLE FileHandle, NT_IO_STATUS_BLOCK *IoStatusBlock, PVOID FileInformation, ULONG Length, NT_FILE_INFORMATION_CLASS FileInformationClass);

struct DirAccessWindowsPrivate {
	HANDLE h; // handle for FindFirstFile.
	WIN32_FIND_DATA f;
	WIN32_FIND_DATAW fu; // Unicode version.
};

String DirAccessWindows::fix_path(const String &p_path) const {
	String r_path = DirAccess::fix_path(p_path.trim_prefix(R"(\\?\)").replace_char('\\', '/'));
	if (r_path.ends_with(":")) {
		r_path += "/";
	}
	if (r_path.is_relative_path()) {
		r_path = current_dir.trim_prefix(R"(\\?\)").replace_char('\\', '/').path_join(r_path);
	} else if (r_path == ".") {
		r_path = current_dir.trim_prefix(R"(\\?\)").replace_char('\\', '/');
	}
	r_path = r_path.simplify_path();
	r_path = r_path.replace_char('/', '\\');
	if (!r_path.is_network_share_path() && !r_path.begins_with(R"(\\?\)")) {
		r_path = R"(\\?\)" + r_path;
	}
	return r_path;
}

// CreateFolderAsync

Error DirAccessWindows::list_dir_begin() {
	_cisdir = false;
	_cishidden = false;

	list_dir_end();
	p->h = FindFirstFileExW((LPCWSTR)(String(current_dir + "\\*").utf16().get_data()), FindExInfoStandard, &p->fu, FindExSearchNameMatch, nullptr, 0);

	if (p->h == INVALID_HANDLE_VALUE) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

String DirAccessWindows::get_next() {
	if (p->h == INVALID_HANDLE_VALUE) {
		return "";
	}

	_cisdir = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	_cishidden = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

	String name = String::utf16((const char16_t *)(p->fu.cFileName));

	if (FindNextFileW(p->h, &p->fu) == 0) {
		FindClose(p->h);
		p->h = INVALID_HANDLE_VALUE;
	}

	return name;
}

bool DirAccessWindows::current_is_dir() const {
	return _cisdir;
}

bool DirAccessWindows::current_is_hidden() const {
	return _cishidden;
}

void DirAccessWindows::list_dir_end() {
	if (p->h != INVALID_HANDLE_VALUE) {
		FindClose(p->h);
		p->h = INVALID_HANDLE_VALUE;
	}
}

void DirAccessWindows::_update_drives() {
	drives.clear();
	DWORD mask = GetLogicalDrives();
	for (int i = 0; i < MAX_DRIVES; i++) {
		if (mask & (1 << i)) {
			String drive = String::chr('A' + i) + ':';
			String path = drive + '\\';
			String label;
			Char16String cs = path.utf16();
			UINT type = GetDriveTypeW((LPCWSTR)cs.get_data());
			if (type == DRIVE_REMOTE) {
				drives.push_back(DriveInfo{ drive, "<network>" });
				continue;
			}
			char16_t wlabel[4096];
			if (GetVolumeInformationW((LPCWSTR)cs.get_data(), (LPWSTR)wlabel, 4096, nullptr, nullptr, nullptr, nullptr, 0)) {
				label = String::utf16(wlabel);
			}
			drives.push_back(DriveInfo{ drive, label });
		}
	}
}

int DirAccessWindows::get_drive_count() {
	if (drives.size() == 0) {
		_update_drives();
	}
	return drives.size();
}

String DirAccessWindows::get_drive_label(int p_drive) {
	if (drives.size() == 0) {
		_update_drives();
	}
	if (p_drive < 0 || p_drive >= (int)drives.size()) {
		return String();
	}
	return drives[p_drive].label;
}

String DirAccessWindows::get_drive(int p_drive) {
	if (drives.size() == 0) {
		_update_drives();
	}
	if (p_drive < 0 || p_drive >= (int)drives.size()) {
		return String();
	}
	return drives[p_drive].path;
}

Error DirAccessWindows::change_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	String dir = fix_path(p_dir);

	Char16String real_current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	real_current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
	String prev_dir = String::utf16((const char16_t *)real_current_dir_name.get_data());

	SetCurrentDirectoryW((LPCWSTR)(current_dir.utf16().get_data()));
	bool worked = (SetCurrentDirectoryW((LPCWSTR)(dir.utf16().get_data())) != 0);

	String base = _get_root_path();
	if (!base.is_empty()) {
		str_len = GetCurrentDirectoryW(0, nullptr);
		real_current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
		String new_dir = String::utf16((const char16_t *)real_current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/');
		if (!new_dir.begins_with(base)) {
			worked = false;
		}
	}

	if (worked) {
		str_len = GetCurrentDirectoryW(0, nullptr);
		real_current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
		current_dir = String::utf16((const char16_t *)real_current_dir_name.get_data());
	}

	SetCurrentDirectoryW((LPCWSTR)(prev_dir.utf16().get_data()));

	return worked ? OK : ERR_INVALID_PARAMETER;
}

Error DirAccessWindows::make_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (FileAccessWindows::is_path_invalid(p_dir)) {
#ifdef DEBUG_ENABLED
		WARN_PRINT("The path :" + p_dir + " is a reserved Windows system pipe, so it can't be used for creating directories.");
#endif
		return ERR_INVALID_PARAMETER;
	}

	String dir = fix_path(p_dir);

	bool success;
	int err;

	success = CreateDirectoryW((LPCWSTR)(dir.utf16().get_data()), nullptr);
	err = GetLastError();

	if (success) {
		return OK;
	}

	if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED) {
		return ERR_ALREADY_EXISTS;
	}

	return ERR_CANT_CREATE;
}

String DirAccessWindows::get_current_dir(bool p_include_drive) const {
	String cdir = current_dir.trim_prefix(R"(\\?\)").replace_char('\\', '/');
	String base = _get_root_path();
	if (!base.is_empty()) {
		String bd = cdir.replace_first(base, "");
		if (bd.begins_with("/")) {
			return _get_root_string() + bd.substr(1);
		} else {
			return _get_root_string() + bd;
		}
	}

	if (p_include_drive) {
		return cdir;
	} else {
		if (_get_root_string().is_empty()) {
			int pos = cdir.find_char(':');
			if (pos != -1) {
				return cdir.substr(pos + 1);
			}
		}
		return cdir;
	}
}

bool DirAccessWindows::file_exists(String p_file) {
	GLOBAL_LOCK_FUNCTION

	String file = fix_path(p_file);

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(file.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}

	return !(fileAttr & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirAccessWindows::dir_exists(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	String dir = fix_path(p_dir);

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(dir.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}
	return (fileAttr & FILE_ATTRIBUTE_DIRECTORY);
}

Error DirAccessWindows::rename(String p_path, String p_new_path) {
	String path = fix_path(p_path);
	String new_path = fix_path(p_new_path);

	// If we're only changing file name case we need to do a little juggling
	if (path.to_lower() == new_path.to_lower()) {
		if (dir_exists(path)) {
			// The path is a dir; just rename
			return MoveFileW((LPCWSTR)(path.utf16().get_data()), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;
		}
		// The path is a file; juggle
		// Note: do not use GetTempFileNameW, it's not long path aware!
		Char16String tmpfile_utf16;
		uint64_t id = OS::get_ticks_usec();
		while (true) {
			tmpfile_utf16 = (path + itos(id++) + ".tmp").utf16();
			HANDLE handle = CreateFileW((LPCWSTR)tmpfile_utf16.get_data(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
				break;
			}
			if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION) {
				return FAILED;
			}
		}

		if (!::ReplaceFileW((LPCWSTR)tmpfile_utf16.get_data(), (LPCWSTR)(path.utf16().get_data()), nullptr, 0, nullptr, nullptr)) {
			DeleteFileW((LPCWSTR)tmpfile_utf16.get_data());
			return FAILED;
		}

		return MoveFileW((LPCWSTR)tmpfile_utf16.get_data(), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;

	} else {
		if (file_exists(new_path)) {
			if (remove(new_path) != OK) {
				return FAILED;
			}
		}

		return MoveFileW((LPCWSTR)(path.utf16().get_data()), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;
	}
}

Error DirAccessWindows::remove(String p_path) {
	String path = fix_path(p_path);
	const Char16String &path_utf16 = path.utf16();

	DWORD fileAttr;

	fileAttr = GetFileAttributesW((LPCWSTR)(path_utf16.get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return FAILED;
	}
	if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		return RemoveDirectoryW((LPCWSTR)(path_utf16.get_data())) != 0 ? OK : FAILED;
	} else {
		return DeleteFileW((LPCWSTR)(path_utf16.get_data())) != 0 ? OK : FAILED;
	}
}

uint64_t DirAccessWindows::get_space_left() {
	uint64_t bytes = 0;

	String path = fix_path(current_dir);

	if (!path.ends_with("\\")) {
		path += "\\";
	}

	if (!GetDiskFreeSpaceExW((LPCWSTR)(path.utf16().get_data()), (PULARGE_INTEGER)&bytes, nullptr, nullptr)) {
		return 0;
	}

	// This is either 0 or a value in bytes.
	return bytes;
}

String DirAccessWindows::get_filesystem_type() const {
	String path = current_dir.trim_prefix(R"(\\?\)");

	if (path.is_network_share_path()) {
		return "Network Share";
	}

	int unit_end = path.find_char(':');
	ERR_FAIL_COND_V(unit_end == -1, String());
	String unit = path.substr(0, unit_end + 1) + "\\";

	WCHAR szVolumeName[100];
	WCHAR szFileSystemName[10];
	DWORD dwSerialNumber = 0;
	DWORD dwMaxFileNameLength = 0;
	DWORD dwFileSystemFlags = 0;

	if (::GetVolumeInformationW((LPCWSTR)(unit.utf16().get_data()),
				szVolumeName,
				sizeof(szVolumeName),
				&dwSerialNumber,
				&dwMaxFileNameLength,
				&dwFileSystemFlags,
				szFileSystemName,
				sizeof(szFileSystemName)) == TRUE) {
		return String::utf16((const char16_t *)szFileSystemName).to_upper();
	}

	ERR_FAIL_V("");
}

bool DirAccessWindows::is_case_sensitive(const String &p_path) const {
	String f = fix_path(p_path);

	HANDLE h_file = ::CreateFileW((LPCWSTR)(f.utf16().get_data()), 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (h_file == INVALID_HANDLE_VALUE) {
		return false;
	}

	NT_IO_STATUS_BLOCK io_status_block;
	NT_FILE_CASE_SENSITIVE_INFO file_info;
	LONG out = NtQueryInformationFile(h_file, &io_status_block, &file_info, sizeof(NT_FILE_CASE_SENSITIVE_INFO), FileCaseSensitiveInformation);
	::CloseHandle(h_file);

	if (out >= 0) {
		return file_info.Flags & NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR;
	} else {
		return false;
	}
}

typedef struct {
	ULONGLONG LowPart;
	ULONGLONG HighPart;
} GD_FILE_ID_128;

typedef struct {
	ULONGLONG VolumeSerialNumber;
	GD_FILE_ID_128 FileId;
} GD_FILE_ID_INFO;

bool DirAccessWindows::is_equivalent(const String &p_path_a, const String &p_path_b) const {
	GD_FILE_ID_INFO ex1;
	bool did_fallback1 = false;
	BY_HANDLE_FILE_INFORMATION fallback1 = {};

	String f1 = fix_path(p_path_a);
	HANDLE h1 = ::CreateFileW((LPCWSTR)(f1.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (h1 == INVALID_HANDLE_VALUE) {
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}
	if (!::GetFileInformationByHandleEx(h1, (FILE_INFO_BY_HANDLE_CLASS)0x12 /*FileIdInfo*/, &ex1, sizeof(ex1))) {
		// Try `GetFileInformationByHandle` since the Ex version is not supported by certain USB and network drives.
		did_fallback1 = true;
		if (!::GetFileInformationByHandle(h1, &fallback1)) {
			::CloseHandle(h1);
			return DirAccess::is_equivalent(p_path_a, p_path_b);
		}
	}

	GD_FILE_ID_INFO ex2;
	bool did_fallback2 = false;
	BY_HANDLE_FILE_INFORMATION fallback2 = {};

	String f2 = fix_path(p_path_b);
	HANDLE h2 = ::CreateFileW((LPCWSTR)(f2.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (h2 == INVALID_HANDLE_VALUE) {
		::CloseHandle(h1);
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}
	if (!::GetFileInformationByHandleEx(h2, (FILE_INFO_BY_HANDLE_CLASS)0x12 /*FileIdInfo*/, &ex2, sizeof(ex2))) {
		// Try `GetFileInformationByHandle` since the Ex version is not supported by certain USB and network drives.
		did_fallback2 = true;
		if (!::GetFileInformationByHandle(h2, &fallback2)) {
			::CloseHandle(h1);
			::CloseHandle(h2);
			return DirAccess::is_equivalent(p_path_a, p_path_b);
		}
	}

	// Keep both handles open while retrieving ids, since file ids are only unique for open handles.
	::CloseHandle(h1);
	::CloseHandle(h2);

	if (did_fallback1 != did_fallback2) {
		// Assuming different drives due to different syscall support.
		return false;
	}

	if (did_fallback1) {
		return fallback1.dwVolumeSerialNumber == fallback2.dwVolumeSerialNumber && fallback1.nFileIndexLow == fallback2.nFileIndexLow && fallback1.nFileIndexHigh == fallback2.nFileIndexHigh;
	}

	return ex1.VolumeSerialNumber == ex2.VolumeSerialNumber && ex1.FileId.LowPart == ex2.FileId.LowPart && ex1.FileId.HighPart == ex2.FileId.HighPart;
}

bool DirAccessWindows::is_link(String p_file) {
	String f = fix_path(p_file);

	DWORD attr = GetFileAttributesW((LPCWSTR)(f.utf16().get_data()));
	if (attr == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	return (attr & FILE_ATTRIBUTE_REPARSE_POINT);
}

String DirAccessWindows::read_link(String p_file) {
	String f = fix_path(p_file);

	HANDLE hfile = CreateFileW((LPCWSTR)(f.utf16().get_data()), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (hfile == INVALID_HANDLE_VALUE) {
		return f;
	}

	DWORD ret = GetFinalPathNameByHandleW(hfile, nullptr, 0, VOLUME_NAME_DOS | FILE_NAME_NORMALIZED);
	if (ret == 0) {
		return f;
	}
	Char16String cs;
	cs.resize_uninitialized(ret + 1);
	GetFinalPathNameByHandleW(hfile, (LPWSTR)cs.ptrw(), ret, VOLUME_NAME_DOS | FILE_NAME_NORMALIZED);
	CloseHandle(hfile);

	return String::utf16((const char16_t *)cs.ptr(), ret).trim_prefix(R"(\\?\)").replace_char('\\', '/');
}

Error DirAccessWindows::create_link(String p_source, String p_target) {
	String source = fix_path(p_source);
	String target = fix_path(p_target);

	DWORD file_attr = GetFileAttributesW((LPCWSTR)(source.utf16().get_data()));
	bool is_dir = (file_attr & FILE_ATTRIBUTE_DIRECTORY);

	DWORD flags = ((is_dir) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0) | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
	if (CreateSymbolicLinkW((LPCWSTR)target.utf16().get_data(), (LPCWSTR)source.utf16().get_data(), flags) != 0) {
		return OK;
	} else {
		return FAILED;
	}
}

DirAccessWindows::DirAccessWindows() {
	p = memnew(DirAccessWindowsPrivate);
	p->h = INVALID_HANDLE_VALUE;

	Char16String real_current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	real_current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
	current_dir = String::utf16((const char16_t *)real_current_dir_name.get_data());

	change_dir(".");
}

DirAccessWindows::~DirAccessWindows() {
	list_dir_end();

	memdelete(p);
}

#endif // WINDOWS_ENABLED

#ifdef NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR
#undef NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR
#endif

// ============================================================================
// Merged from drivers/windows/file_access_windows_pipe.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "drivers/windows/file_access_windows.h"

#include "core/string/ustring.h"

#include <winbase.h>

Error FileAccessWindowsPipe::open_existing(HANDLE p_rfd, HANDLE p_wfd, bool p_blocking) {
	// Open pipe using handles created by CreatePipe(rfd, wfd, NULL, 4096) call in the OS.execute_with_pipe.
	_close();

	path_src = String();
	ERR_FAIL_COND_V_MSG(fd[0] != nullptr || fd[1] != nullptr, ERR_ALREADY_IN_USE, "Pipe is already in use.");
	fd[0] = p_rfd;
	fd[1] = p_wfd;

	if (!p_blocking) {
		DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
		SetNamedPipeHandleState(fd[0], &mode, nullptr, nullptr);
		SetNamedPipeHandleState(fd[1], &mode, nullptr, nullptr);
	}

	last_error = OK;
	return OK;
}

Error FileAccessWindowsPipe::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	ERR_FAIL_COND_V_MSG(fd[0] != nullptr || fd[1] != nullptr, ERR_ALREADY_IN_USE, "Pipe is already in use.");

	path = String("\\\\.\\pipe\\LOCAL\\") + p_path.replace("pipe://", "").replace_char('/', '_');

	HANDLE h = CreateFileW((LPCWSTR)path.utf16().get_data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		h = CreateNamedPipeW((LPCWSTR)path.utf16().get_data(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT, 1, 4096, 4096, 0, nullptr);
		if (h == INVALID_HANDLE_VALUE) {
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}
		ConnectNamedPipe(h, nullptr);
	}
	fd[0] = h;
	fd[1] = h;

	last_error = OK;
	return OK;
}

void FileAccessWindowsPipe::_close() {
	if (fd[0] == nullptr) {
		return;
	}
	if (fd[1] != fd[0]) {
		CloseHandle(fd[1]);
	}
	CloseHandle(fd[0]);
	fd[0] = nullptr;
	fd[1] = nullptr;
}

bool FileAccessWindowsPipe::is_open() const {
	return (fd[0] != nullptr || fd[1] != nullptr);
}

String FileAccessWindowsPipe::get_path() const {
	return path_src;
}

String FileAccessWindowsPipe::get_path_absolute() const {
	return path_src;
}

uint64_t FileAccessWindowsPipe::get_length() const {
	ERR_FAIL_COND_V_MSG(fd[0] == nullptr, -1, "Pipe must be opened before use.");

	DWORD buf_rem = 0;
	ERR_FAIL_COND_V(!PeekNamedPipe(fd[0], nullptr, 0, nullptr, &buf_rem, nullptr), 0);
	return buf_rem;
}

uint64_t FileAccessWindowsPipe::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_COND_V_MSG(fd[0] == nullptr, -1, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	DWORD read = 0;
	if (!ReadFile(fd[0], p_dst, p_length, &read, nullptr) || read != p_length) {
		last_error = ERR_FILE_CANT_READ;
	} else {
		last_error = OK;
	}
	return read;
}

Error FileAccessWindowsPipe::get_error() const {
	return last_error;
}

bool FileAccessWindowsPipe::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_COND_V_MSG(fd[1] == nullptr, false, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);

	DWORD read = -1;
	bool ok = WriteFile(fd[1], p_src, p_length, &read, nullptr);
	if (!ok || read != p_length) {
		last_error = ERR_FILE_CANT_WRITE;
		return false;
	} else {
		last_error = OK;
		return true;
	}
}

void FileAccessWindowsPipe::close() {
	_close();
}

FileAccessWindowsPipe::~FileAccessWindowsPipe() {
	_close();
}

#endif // WINDOWS_ENABLED

// ============================================================================
// Merged from drivers/windows/ip_windows.cpp
// ============================================================================

#if defined(WINDOWS_ENABLED)

#include "drivers/windows/file_access_windows.h"

#include <winsock2.h>
#include <ws2tcpip.h>
// Include winsock2 before iphlpapi.
#include <iphlpapi.h>

static IPAddress _sockaddr2ip(struct sockaddr *p_addr) {
	IPAddress ip;

	if (p_addr->sa_family == AF_INET) {
		struct sockaddr_in *addr = (struct sockaddr_in *)p_addr;
		ip.set_ipv4((uint8_t *)&(addr->sin_addr));
	} else if (p_addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		ip.set_ipv6(addr6->sin6_addr.s6_addr);
	}

	return ip;
}

void IPWindows::_resolve_hostname(
	List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type)
{
	struct addrinfo hints;
	struct addrinfo* result = nullptr;

	memset(&hints, 0, sizeof(struct addrinfo));
	if (p_type == IP::TYPE_IPV4) {
		hints.ai_family = AF_INET;
	}
	else if (p_type == IP::TYPE_IPV6) {
		hints.ai_family = AF_INET6;
		hints.ai_flags = 0;
	}
	else {
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_ADDRCONFIG;
	}
	hints.ai_flags &= ~AI_NUMERICHOST;

	int s = getaddrinfo(p_hostname.utf8().get_data(), nullptr, &hints, &result);
	if (s != 0) {
		print_verbose("getaddrinfo failed! Cannot resolve hostname.");
		return;
	}

	if (result == nullptr || result->ai_addr == nullptr) {
		print_verbose("Invalid response from getaddrinfo.");
		if (result) {
			freeaddrinfo(result);
		}
		return;
	}

	struct addrinfo *next = result;

	do {
		if (next->ai_addr == nullptr) {
			next = next->ai_next;
			continue;
		}
		IPAddress ip = _sockaddr2ip(next->ai_addr);
		if (ip.is_valid() && !r_addresses.find(ip)) {
			r_addresses.push_back(ip);
		}
		next = next->ai_next;
	} while (next);

	freeaddrinfo(result);
}

void IPWindows::get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces)
{
	ULONG buf_size = 1024;
	IP_ADAPTER_ADDRESSES* addrs;

	while (true) {
		addrs = (IP_ADAPTER_ADDRESSES*)memalloc(buf_size);
		int err = GetAdaptersAddresses(AF_UNSPEC,
			GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER |
				GAA_FLAG_SKIP_FRIENDLY_NAME,
			nullptr, addrs, &buf_size);
		if (err == NO_ERROR) {
			break;
		}
		memfree(addrs);
		if (err == ERROR_BUFFER_OVERFLOW) {
			continue; // Will go back and alloc the right size.
		}

		ERR_FAIL_MSG("Call to GetAdaptersAddresses failed with error " + itos(err) + ".");
	}

	IP_ADAPTER_ADDRESSES* adapter = addrs;

	while (adapter != nullptr) {
		IP::Interface_Info info;
		info.name = adapter->AdapterName;
		info.name_friendly = adapter->FriendlyName;
		info.index = String::num_uint64(adapter->IfIndex);

		IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress;
		while (address != nullptr) {
			int family = address->Address.lpSockaddr->sa_family;
			if (family != AF_INET && family != AF_INET6) {
				continue;
			}
			info.ip_addresses.push_front(_sockaddr2ip(address->Address.lpSockaddr));
			address = address->Next;
		}
		adapter = adapter->Next;
		// Only add interface if it has at least one IP.
		if (info.ip_addresses.size() > 0) {
			r_interfaces->insert(info.name, info);
		}
	}

	memfree(addrs);
}

void IPWindows::make_default()
{
	IP::bind<IPWindows>();
}

#endif // WINDOWS_ENABLED

// ============================================================================
// Merged from drivers/windows/net_socket_winsock.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "drivers/windows/file_access_windows.h"

#include <mswsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Workaround missing flag in MinGW
#if defined(__MINGW32__) && !defined(SIO_UDP_NETRESET)
#define SIO_UDP_NETRESET _WSAIOW(IOC_VENDOR, 15)
#endif

size_t NetSocketWinSock::_set_addr_storage(struct sockaddr_storage *p_addr, const IPAddress &p_ip, uint16_t p_port, IP::Type p_ip_type) {
	memset(p_addr, 0, sizeof(struct sockaddr_storage));
	if (p_ip_type == IP::TYPE_IPV6 || p_ip_type == IP::TYPE_ANY) { // IPv6 socket.

		// IPv6 only socket with IPv4 address.
		ERR_FAIL_COND_V(!p_ip.is_wildcard() && p_ip_type == IP::TYPE_IPV6 && p_ip.is_ipv4(), 0);

		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(p_port);
		if (p_ip.is_valid()) {
			memcpy(&addr6->sin6_addr.s6_addr, p_ip.get_ipv6(), 16);
		} else {
			addr6->sin6_addr = in6addr_any;
		}
		return sizeof(sockaddr_in6);
	} else { // IPv4 socket.

		// IPv4 socket with IPv6 address.
		ERR_FAIL_COND_V(!p_ip.is_wildcard() && !p_ip.is_ipv4(), 0);

		struct sockaddr_in *addr4 = (struct sockaddr_in *)p_addr;
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(p_port); // Short, network byte order.

		if (p_ip.is_valid()) {
			memcpy(&addr4->sin_addr.s_addr, p_ip.get_ipv4(), 4);
		} else {
			addr4->sin_addr.s_addr = INADDR_ANY;
		}

		return sizeof(sockaddr_in);
	}
}

void NetSocketWinSock::_set_ip_port(struct sockaddr_storage *p_addr, IPAddress *r_ip, uint16_t *r_port) {
	if (p_addr->ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)p_addr;
		if (r_ip) {
			r_ip->set_ipv4((uint8_t *)&(addr4->sin_addr.s_addr));
		}
		if (r_port) {
			*r_port = ntohs(addr4->sin_port);
		}
	} else if (p_addr->ss_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		if (r_ip) {
			r_ip->set_ipv6(addr6->sin6_addr.s6_addr);
		}
		if (r_port) {
			*r_port = ntohs(addr6->sin6_port);
		}
	}
}

Ref<NetSocket> NetSocketWinSock::_create_func() {
	return memnew(NetSocketWinSock);
}

void NetSocketWinSock::make_default() {
	ERR_FAIL_COND(_create != nullptr);

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	_create = _create_func;
}

void NetSocketWinSock::cleanup() {
	ERR_FAIL_COND(_create == nullptr);

	WSACleanup();
	_create = nullptr;
}

NetSocketWinSock::NetSocketWinSock() {
}

NetSocketWinSock::~NetSocketWinSock() {
	close();
}

NetSocketWinSock::NetError NetSocketWinSock::_get_socket_error() const {
	int err = WSAGetLastError();
	if (err == WSAEISCONN) {
		return ERR_NET_IS_CONNECTED;
	}
	if (err == WSAEINPROGRESS || err == WSAEALREADY) {
		return ERR_NET_IN_PROGRESS;
	}
	if (err == WSAEWOULDBLOCK) {
		return ERR_NET_WOULD_BLOCK;
	}
	if (err == WSAEADDRINUSE || err == WSAEADDRNOTAVAIL) {
		return ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE;
	}
	if (err == WSAEACCES) {
		return ERR_NET_UNAUTHORIZED;
	}
	if (err == WSAEMSGSIZE || err == WSAENOBUFS) {
		return ERR_NET_BUFFER_TOO_SMALL;
	}
	print_verbose("Socket error: " + itos(err) + ".");
	return ERR_NET_OTHER;
}

bool NetSocketWinSock::_can_use_ip(const IPAddress &p_ip, const bool p_for_bind) const {
	if (p_for_bind && !(p_ip.is_valid() || p_ip.is_wildcard())) {
		return false;
	} else if (!p_for_bind && !p_ip.is_valid()) {
		return false;
	}
	// Check if socket support this IP type.
	IP::Type type = p_ip.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	return !(_ip_type != IP::TYPE_ANY && !p_ip.is_wildcard() && _ip_type != type);
}

_FORCE_INLINE_ Error NetSocketWinSock::_change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!_can_use_ip(p_ip, false), ERR_INVALID_PARAMETER);

	// Need to force level and af_family to IP(v4) when using dual stacking and provided multicast group is IPv4.
	IP::Type type = _ip_type == IP::TYPE_ANY && p_ip.is_ipv4() ? IP::TYPE_IPV4 : _ip_type;
	// This needs to be the proper level for the multicast group, no matter if the socket is dual stacking.
	int level = type == IP::TYPE_IPV4 ? IPPROTO_IP : IPPROTO_IPV6;
	int ret = -1;

	IPAddress if_ip;
	uint32_t if_v6id = 0;
	HashMap<String, IP::Interface_Info> if_info;
	IP::get_local_interfaces(&if_info);
	for (KeyValue<String, IP::Interface_Info> &E : if_info) {
		IP::Interface_Info &c = E.value;
		if (c.name != p_if_name) {
			continue;
		}

		if_v6id = (uint32_t)c.index.to_int();
		if (type == IP::TYPE_IPV6) {
			break; // IPv6 uses index.
		}

		for (const IPAddress &F : c.ip_addresses) {
			if (!F.is_ipv4()) {
				continue; // Wrong IP type.
			}
			if_ip = F;
			break;
		}
		break;
	}

	if (level == IPPROTO_IP) {
		ERR_FAIL_COND_V(!if_ip.is_valid(), ERR_INVALID_PARAMETER);
		struct ip_mreq greq;
		int sock_opt = p_add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
		memcpy(&greq.imr_multiaddr, p_ip.get_ipv4(), 4);
		memcpy(&greq.imr_interface, if_ip.get_ipv4(), 4);
		ret = setsockopt(_sock, level, sock_opt, (const char *)&greq, sizeof(greq));
	} else {
		struct ipv6_mreq greq;
		int sock_opt = p_add ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
		memcpy(&greq.ipv6mr_multiaddr, p_ip.get_ipv6(), 16);
		greq.ipv6mr_interface = if_v6id;
		ret = setsockopt(_sock, level, sock_opt, (const char *)&greq, sizeof(greq));
	}
	ERR_FAIL_COND_V(ret != 0, FAILED);

	return OK;
}

void NetSocketWinSock::_set_socket(SOCKET p_sock, IP::Type p_ip_type, bool p_is_stream) {
	_sock = p_sock;
	_ip_type = p_ip_type;
	_is_stream = p_is_stream;
}

Error NetSocketWinSock::open(Family p_family, Type p_sock_type, IP::Type &ip_type) {
	ERR_FAIL_COND_V(p_family != Family::INET, ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(ip_type > IP::TYPE_ANY || ip_type < IP::TYPE_NONE, ERR_INVALID_PARAMETER);

	int family = ip_type == IP::TYPE_IPV4 ? AF_INET : AF_INET6;
	int protocol = p_sock_type == TYPE_TCP ? IPPROTO_TCP : IPPROTO_UDP;
	int type = p_sock_type == TYPE_TCP ? SOCK_STREAM : SOCK_DGRAM;
	_sock = socket(family, type, protocol);

	if (_sock == INVALID_SOCKET && ip_type == IP::TYPE_ANY) {
		// Careful here, changing the referenced parameter so the caller knows that we are using an IPv4 socket
		// in place of a dual stack one, and further calls to _set_sock_addr will work as expected.
		ip_type = IP::TYPE_IPV4;
		family = AF_INET;
		_sock = socket(family, type, protocol);
	}

	ERR_FAIL_COND_V(_sock == INVALID_SOCKET, FAILED);
	_ip_type = ip_type;

	if (family == AF_INET6) {
		// Select IPv4 over IPv6 mapping.
		set_ipv6_only_enabled(ip_type != IP::TYPE_ANY);
	}

	if (protocol == IPPROTO_UDP) {
		// Make sure to disable broadcasting for UDP sockets.
		// Depending on the OS, this option might or might not be enabled by default. Let's normalize it.
		set_broadcasting_enabled(false);
	}

	_is_stream = p_sock_type == TYPE_TCP;

	if (!_is_stream) {
		// Disable windows feature/bug reporting WSAECONNRESET/WSAENETRESET when
		// recv/recvfrom and an ICMP reply was received from a previous send/sendto.
		unsigned long disable = 0;
		if (ioctlsocket(_sock, SIO_UDP_CONNRESET, &disable) == SOCKET_ERROR) {
			print_verbose("Unable to turn off UDP WSAECONNRESET behavior on Windows.");
		}
		if (ioctlsocket(_sock, SIO_UDP_NETRESET, &disable) == SOCKET_ERROR) {
			// This feature seems not to be supported on wine.
			print_verbose("Unable to turn off UDP WSAENETRESET behavior on Windows.");
		}
	}
	return OK;
}

void NetSocketWinSock::close() {
	if (_sock != INVALID_SOCKET) {
		closesocket(_sock);
	}

	_sock = INVALID_SOCKET;
	_ip_type = IP::TYPE_NONE;
	_is_stream = false;
}

Error NetSocketWinSock::bind(Address p_addr) {
	ERR_FAIL_COND_V(!p_addr.is_inet(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!_can_use_ip(p_addr.ip(), true), ERR_INVALID_PARAMETER);

	sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_addr.ip(), p_addr.port(), _ip_type);

	if (::bind(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		print_verbose("Failed to bind socket. Error: " + itos(err) + ".");
		close();
		return ERR_UNAVAILABLE;
	}

	return OK;
}

Error NetSocketWinSock::listen(int p_max_pending) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	if (::listen(_sock, p_max_pending) != 0) {
		_get_socket_error();
		print_verbose("Failed to listen from socket.");
		close();
		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::connect_to_host(Address p_addr) {
	ERR_FAIL_COND_V(!p_addr.is_inet(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!_can_use_ip(p_addr.ip(), false), ERR_INVALID_PARAMETER);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_addr.ip(), p_addr.port(), _ip_type);

	if (::WSAConnect(_sock, (struct sockaddr *)&addr, addr_size, nullptr, nullptr, nullptr, nullptr) != 0) {
		NetError err = _get_socket_error();

		switch (err) {
			// We are already connected.
			case ERR_NET_IS_CONNECTED:
				return OK;
			// Still waiting to connect, try again in a while.
			case ERR_NET_WOULD_BLOCK:
			case ERR_NET_IN_PROGRESS:
				return ERR_BUSY;
			default:
				print_verbose("Connection to remote host failed.");
				close();
				return FAILED;
		}
	}

	return OK;
}

Error NetSocketWinSock::poll(PollType p_type, int p_timeout) const {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	bool ready = false;
	fd_set rd, wr, ex;
	fd_set *rdp = nullptr;
	fd_set *wrp = nullptr;
	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_ZERO(&ex);
	FD_SET(_sock, &ex);
	struct timeval timeout = { p_timeout / 1000, (p_timeout % 1000) * 1000 };
	// For blocking operation, pass nullptr timeout pointer to select.
	struct timeval *tp = nullptr;
	if (p_timeout >= 0) {
		// If timeout is non-negative, we want to specify the timeout instead.
		tp = &timeout;
	}

	switch (p_type) {
		case POLL_TYPE_IN:
			FD_SET(_sock, &rd);
			rdp = &rd;
			break;
		case POLL_TYPE_OUT:
			FD_SET(_sock, &wr);
			wrp = &wr;
			break;
		case POLL_TYPE_IN_OUT:
			FD_SET(_sock, &rd);
			FD_SET(_sock, &wr);
			rdp = &rd;
			wrp = &wr;
	}
	// WSAPoll is broken: https://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/.
	int ret = select(1, rdp, wrp, &ex, tp);

	if (ret == SOCKET_ERROR) {
		return FAILED;
	}

	if (ret == 0) {
		return ERR_BUSY;
	}

	if (FD_ISSET(_sock, &ex)) {
		_get_socket_error();
		print_verbose("Exception when polling socket.");
		return FAILED;
	}

	if (rdp && FD_ISSET(_sock, rdp)) {
		ready = true;
	}
	if (wrp && FD_ISSET(_sock, wrp)) {
		ready = true;
	}

	return ready ? OK : ERR_BUSY;
}

Error NetSocketWinSock::recv(uint8_t *p_buffer, int p_len, int &r_read) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	r_read = ::recv(_sock, (char *)p_buffer, p_len, 0);

	if (r_read < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}

		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	struct sockaddr_storage from;
	socklen_t len = sizeof(struct sockaddr_storage);
	memset(&from, 0, len);

	r_read = ::recvfrom(_sock, (char *)p_buffer, p_len, p_peek ? MSG_PEEK : 0, (struct sockaddr *)&from, &len);

	if (r_read < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}

		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	if (from.ss_family == AF_INET) {
		struct sockaddr_in *sin_from = (struct sockaddr_in *)&from;
		r_ip.set_ipv4((uint8_t *)&sin_from->sin_addr);
		r_port = ntohs(sin_from->sin_port);
	} else if (from.ss_family == AF_INET6) {
		struct sockaddr_in6 *s6_from = (struct sockaddr_in6 *)&from;
		r_ip.set_ipv6((uint8_t *)&s6_from->sin6_addr);
		r_port = ntohs(s6_from->sin6_port);
	} else {
		// Unsupported socket family, should never happen.
		ERR_FAIL_V(FAILED);
	}

	return OK;
}

Error NetSocketWinSock::send(const uint8_t *p_buffer, int p_len, int &r_sent) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	int flags = 0;
	r_sent = ::send(_sock, (const char *)p_buffer, p_len, flags);

	if (r_sent < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}
		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_ip, p_port, _ip_type);
	r_sent = ::sendto(_sock, (const char *)p_buffer, p_len, 0, (struct sockaddr *)&addr, addr_size);

	if (r_sent < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}
		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::set_broadcasting_enabled(bool p_enabled) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	// IPv6 has no broadcast support.
	if (_ip_type == IP::TYPE_IPV6) {
		return ERR_UNAVAILABLE;
	}

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, (const char *)&par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change broadcast setting.");
		return FAILED;
	}
	return OK;
}

void NetSocketWinSock::set_blocking_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	int ret = 0;
	unsigned long par = p_enabled ? 0 : 1;
	ret = ioctlsocket(_sock, FIONBIO, &par);
	if (ret != 0) {
		WARN_PRINT("Unable to change non-block mode.");
	}
}

void NetSocketWinSock::set_ipv6_only_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	// This option is only available in IPv6 sockets.
	ERR_FAIL_COND(_ip_type == IP::TYPE_IPV4);

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change IPv4 address mapping over IPv6 option.");
	}
}

void NetSocketWinSock::set_tcp_no_delay_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	ERR_FAIL_COND(!_is_stream); // Not TCP.

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&par, sizeof(int)) < 0) {
		WARN_PRINT("Unable to set TCP no delay option.");
	}
}

void NetSocketWinSock::set_reuse_address_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	// On Windows, enabling SO_REUSEADDR actually would also enable reuse port, very bad on TCP. Denying...
	// Windows does not have this option, SO_REUSEADDR in this magical world means SO_REUSEPORT
}

bool NetSocketWinSock::is_open() const {
	return _sock != INVALID_SOCKET;
}

int NetSocketWinSock::get_available_bytes() const {
	ERR_FAIL_COND_V(!is_open(), -1);

	unsigned long len;
	int ret = ioctlsocket(_sock, FIONREAD, &len);
	if (ret == -1) {
		_get_socket_error();
		print_verbose("Error when checking available bytes on socket.");
		return -1;
	}
	return len;
}

Error NetSocketWinSock::get_socket_address(Address *r_addr) const {
	ERR_FAIL_COND_V(!is_open(), FAILED);

	struct sockaddr_storage saddr;
	socklen_t len = sizeof(saddr);
	if (getsockname(_sock, (struct sockaddr *)&saddr, &len) != 0) {
		_get_socket_error();
		print_verbose("Error when reading local socket address.");
		return FAILED;
	}
	IPAddress ip;
	uint16_t port = 0;
	_set_ip_port(&saddr, &ip, &port);
	if (r_addr) {
		*r_addr = Address(ip, port);
	}
	return OK;
}

Ref<NetSocket> NetSocketWinSock::accept(Address &r_addr) {
	Ref<NetSocket> out;
	ERR_FAIL_COND_V(!is_open(), out);

	struct sockaddr_storage their_addr;
	socklen_t size = sizeof(their_addr);
	SOCKET fd = ::accept(_sock, (struct sockaddr *)&their_addr, &size);
	if (fd == INVALID_SOCKET) {
		_get_socket_error();
		print_verbose("Error when accepting socket connection.");
		return out;
	}

	IPAddress ip;
	uint16_t port = 0;
	_set_ip_port(&their_addr, &ip, &port);
	r_addr = Address(ip, port);

	Ref<NetSocketWinSock> ns = memnew(NetSocketWinSock);
	ns->_set_socket(fd, _ip_type, _is_stream);
	ns->set_blocking_enabled(false);
	return Ref<NetSocket>(ns);
}

Error NetSocketWinSock::join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, true);
}

Error NetSocketWinSock::leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, false);
}

#endif // WINDOWS_ENABLED

#ifdef SIO_UDP_NETRESET
#undef SIO_UDP_NETRESET
#endif

// ============================================================================
// Merged from drivers/windows/thread_windows.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "drivers/windows/file_access_windows.h"

#include "core/os/thread.h"
#include "core/string/ustring.h"

#include <windows.h>

typedef HRESULT(WINAPI *SetThreadDescriptionPtr)(HANDLE p_thread, PCWSTR p_thread_description);
SetThreadDescriptionPtr w10_SetThreadDescription = nullptr;

static Error set_name(const String &p_name) {
	HANDLE hThread = GetCurrentThread();
	HRESULT res = E_FAIL;
	if (w10_SetThreadDescription) {
		res = w10_SetThreadDescription(hThread, (LPCWSTR)p_name.utf16().get_data()); // Windows 10 Redstone (1607) only.
	}
	return SUCCEEDED(res) ? OK : ERR_INVALID_PARAMETER;
}

void init_thread_win() {
	w10_SetThreadDescription = (SetThreadDescriptionPtr)(void *)GetProcAddress(LoadLibraryW(L"kernel32.dll"), "SetThreadDescription");

	Thread::_set_platform_functions({ set_name });
}

#endif // WINDOWS_ENABLED
