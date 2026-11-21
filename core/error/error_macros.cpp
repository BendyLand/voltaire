/**************************************************************************/
/*  error_macros.cpp                                                      */
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

#include "error_macros.h"

#include "core/core_globals.h"
#include "core/io/logger.h"
#include "core/os/os.h"
#include "core/string/ustring.h"

#include <cstdio>

// Optional physics interpolation warnings try to include the path to the relevant node.
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
#include "core/config/project_settings.h"
#include "scene/main/node.h"
#endif

static ErrorHandlerList *error_handler_list = nullptr;
static thread_local bool is_printing_error = false;

static void _err_print_fallback(const char *p_function, const char *p_file, int p_line, const char *p_error_details, ErrorHandlerType p_type, bool p_reentrance) {
	if (p_reentrance) {
		fprintf(stderr, "While attempting to print an error, another error was printed:\n");
	}

	fprintf(stderr, "%s: %s\n", _error_handler_type_string(p_type), p_error_details);

	if (p_function && p_file) {
		fprintf(stderr, "   at: %s (%s:%i)\n", p_function, p_file, p_line);
	}
}

void add_error_handler(ErrorHandlerList *p_handler) {
	// If p_handler is already in error_handler_list
	// we'd better remove it first then we can add it.
	// This prevent cyclic redundancy.
	remove_error_handler(p_handler);

	_global_lock();

	p_handler->next = error_handler_list;
	error_handler_list = p_handler;

	_global_unlock();
}

void remove_error_handler(const ErrorHandlerList *p_handler) {
	_global_lock();

	ErrorHandlerList *prev = nullptr;
	ErrorHandlerList *l = error_handler_list;

	while (l) {
		if (l == p_handler) {
			if (prev) {
				prev->next = l->next;
			} else {
				error_handler_list = l->next;
			}
			break;
		}
		prev = l;
		l = l->next;
	}

	_global_unlock();
}

// Errors without messages.
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, "", p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), "", p_editor_notify, p_type);
}

// Main error printing function.
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	if (!CoreGlobals::print_ready) {
		const char *err_details = (p_message && *p_message) ? p_message : p_error;
		_err_print_fallback(p_function, p_file, p_line, err_details, p_type, false);
		return;
	}

	if (is_printing_error) {
		// Fallback if we're already printing an error, to prevent infinite recursion.
		const char *err_details = (p_message && *p_message) ? p_message : p_error;
		_err_print_fallback(p_function, p_file, p_line, err_details, p_type, true);
		return;
	}

	is_printing_error = true;

	// Fallback if errors happen before OS init or after it's destroyed.
	const char *err_details = (p_message && *p_message) ? p_message : p_error;
	_err_print_fallback(p_function, p_file, p_line, err_details, p_type, false);

	_global_lock();

	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, p_function, p_file, p_line, p_error, p_message, p_editor_notify, p_type);
		l = l->next;
	}

	_global_unlock();

	is_printing_error = false;
}

// For printing errors when we may crash at any point, so we must flush ASAP a lot of lines
// but we don't want to make it noisy by printing lots of file & line info (because it's already
// been printing by a preceding _err_print_error).
void _err_print_error_asap(const String &p_error, ErrorHandlerType p_type) {
	const CharString err_details_str = p_error.utf8();
	const char *err_details = err_details_str.get_data();

	if (!CoreGlobals::print_ready) {
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, false);
		return;
	}

	if (is_printing_error) {
		// Fallback if we're already printing an error, to prevent infinite recursion.
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, true);
		return;
	}

	is_printing_error = true;
	if (OS::is_initialized()) {
		OS::printerr("%s: %s\n", _error_handler_type_string(p_type), err_details);
	} else {
		// Fallback if errors happen before OS init or after it's destroyed.
		_err_print_fallback(nullptr, nullptr, 0, err_details, p_type, false);
	}

	_global_lock();

	ErrorHandlerList *l = error_handler_list;
	while (l) {
		l->errfunc(l->userdata, "", "", 0, err_details, "", false, p_type);
		l = l->next;
	}

	_global_unlock();

	is_printing_error = false;
}

// Errors with message. (All combinations of p_error and p_message as String or char*.)
void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const char *p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message, p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const String &p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error, p_message.utf8().get_data(), p_editor_notify, p_type);
}

void _err_print_error(const char *p_function, const char *p_file, int p_line, const String &p_error, const String &p_message, bool p_editor_notify, ErrorHandlerType p_type) {
	_err_print_error(p_function, p_file, p_line, p_error.utf8().get_data(), p_message.utf8().get_data(), p_editor_notify, p_type);
}

// Index errors. (All combinations of p_message as String or char*.)
void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const char *p_message, bool p_editor_notify, bool p_fatal) {
	String fstr(p_fatal ? "FATAL: " : "");
	String err(fstr + "Index " + p_index_str + " = " + itos(p_index) + " is out of bounds (" + p_size_str + " = " + itos(p_size) + ").");
	_err_print_error(p_function, p_file, p_line, err.utf8().get_data(), p_message, p_editor_notify, ERR_HANDLER_ERROR);
}

void _err_print_index_error(const char *p_function, const char *p_file, int p_line, int64_t p_index, int64_t p_size, const char *p_index_str, const char *p_size_str, const String &p_message, bool p_editor_notify, bool p_fatal) {
	_err_print_index_error(p_function, p_file, p_line, p_index, p_size, p_index_str, p_size_str, p_message.utf8().get_data(), p_editor_notify, p_fatal);
}

void _err_flush_stdout() {
	fflush(stdout);
}

// Prevent error spam by limiting the warnings to a certain frequency.
void _physics_interpolation_warning(const char *p_function, const char *p_file, int p_line, const char *p_warn_string) {
#if defined(DEBUG_ENABLED) && defined(TOOLS_ENABLED)
	const uint32_t warn_max = 2048;
	const uint32_t warn_timeout_seconds = 15;

	static uint32_t warn_count = warn_max;
	static uint32_t warn_timeout = warn_timeout_seconds;

	uint32_t time_now = UINT32_MAX;

	if (warn_count) {
		warn_count--;
	}

	if (!warn_count) {
		time_now = OS::get_ticks_msec() / 1000;
	}

	if ((warn_count == 0) && (time_now >= warn_timeout)) {
		warn_count = warn_max;
		warn_timeout = time_now + warn_timeout_seconds;
	}
#endif
}

// ============================================================================
// Merged from core/error/error_list.cpp
// ============================================================================

#include "error_list.h"

#include "core/string/ustring.h"
#include "core/typedefs.h"

const char *error_names[] = {
	TTRC("OK"), // OK
	TTRC("Failed"), // FAILED
	TTRC("Unavailable"), // ERR_UNAVAILABLE
	TTRC("Unconfigured"), // ERR_UNCONFIGURED
	TTRC("Unauthorized"), // ERR_UNAUTHORIZED
	TTRC("Parameter out of range"), // ERR_PARAMETER_RANGE_ERROR
	TTRC("Out of memory"), // ERR_OUT_OF_MEMORY
	TTRC("File not found"), // ERR_FILE_NOT_FOUND
	TTRC("File: Bad drive"), // ERR_FILE_BAD_DRIVE
	TTRC("File: Bad path"), // ERR_FILE_BAD_PATH
	TTRC("File: Permission denied"), // ERR_FILE_NO_PERMISSION
	TTRC("File already in use"), // ERR_FILE_ALREADY_IN_USE
	TTRC("Can't open file"), // ERR_FILE_CANT_OPEN
	TTRC("Can't write file"), // ERR_FILE_CANT_WRITE
	TTRC("Can't read file"), // ERR_FILE_CANT_READ
	TTRC("File unrecognized"), // ERR_FILE_UNRECOGNIZED
	TTRC("File corrupt"), // ERR_FILE_CORRUPT
	TTRC("Missing dependencies for file"), // ERR_FILE_MISSING_DEPENDENCIES
	TTRC("End of file"), // ERR_FILE_EOF
	TTRC("Can't open"), // ERR_CANT_OPEN
	TTRC("Can't create"), // ERR_CANT_CREATE
	TTRC("Query failed"), // ERR_QUERY_FAILED
	TTRC("Already in use"), // ERR_ALREADY_IN_USE
	TTRC("Locked"), // ERR_LOCKED
	TTRC("Timeout"), // ERR_TIMEOUT
	TTRC("Can't connect"), // ERR_CANT_CONNECT
	TTRC("Can't resolve"), // ERR_CANT_RESOLVE
	TTRC("Connection error"), // ERR_CONNECTION_ERROR
	TTRC("Can't acquire resource"), // ERR_CANT_ACQUIRE_RESOURCE
	TTRC("Can't fork"), // ERR_CANT_FORK
	TTRC("Invalid data"), // ERR_INVALID_DATA
	TTRC("Invalid parameter"), // ERR_INVALID_PARAMETER
	TTRC("Already exists"), // ERR_ALREADY_EXISTS
	TTRC("Does not exist"), // ERR_DOES_NOT_EXIST
	TTRC("Can't read database"), // ERR_DATABASE_CANT_READ
	TTRC("Can't write database"), // ERR_DATABASE_CANT_WRITE
	TTRC("Compilation failed"), // ERR_COMPILATION_FAILED
	TTRC("Method not found"), // ERR_METHOD_NOT_FOUND
	TTRC("Link failed"), // ERR_LINK_FAILED
	TTRC("Script failed"), // ERR_SCRIPT_FAILED
	TTRC("Cyclic link detected"), // ERR_CYCLIC_LINK
	TTRC("Invalid declaration"), // ERR_INVALID_DECLARATION
	TTRC("Duplicate symbol"), // ERR_DUPLICATE_SYMBOL
	TTRC("Parse error"), // ERR_PARSE_ERROR
	TTRC("Busy"), // ERR_BUSY
	TTRC("Skip"), // ERR_SKIP
	TTRC("Help"), // ERR_HELP
	TTRC("Bug"), // ERR_BUG
	TTRC("Printer on fire"), // ERR_PRINTER_ON_FIRE
};

static_assert(std_size(error_names) == ERR_MAX);
