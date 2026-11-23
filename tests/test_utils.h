/**************************************************************************/
/*  test_utils.h                                                          */
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

class String;

namespace TestUtils {

String get_data_path(const String &p_file);
String get_executable_dir();
String get_temp_path(const String &p_suffix);
} // namespace TestUtils

// FIXME: This was originally constrained to `tests/core/config/test_project_settings.h`, but that
//  file is no longer a header. Some other tests relied on the ability to override the resource
//  path, so relocating the accessor is the least-intrusive workaround.
class TestProjectSettingsInternalsAccessor {
public:
	static String &resource_path();
};

// --- Folded from tests/test_tools.h ---
#include "core/error/error_macros.h"

struct ErrorDetector {
	ErrorDetector() {
		eh.errfunc = _detect_error;
		eh.userdata = this;

		add_error_handler(&eh);
	}

	~ErrorDetector() {
		remove_error_handler(&eh);
	}

	void clear() {
		has_error = false;
	}

	static void _detect_error(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, bool p_editor_notify, ErrorHandlerType p_type) {
		ErrorDetector *self = (ErrorDetector *)p_self;
		self->has_error = true;
	}

	ErrorHandlerList eh;
	bool has_error = false;
};
// --- End tests/test_tools.h ---
