/**************************************************************************/
/*  core_string_names.h                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/string/string_name.h"

class CoreStringNames final
{
public:
	static inline const StringName free_ = "free"; // free would conflict with C++ keyword.
	static inline const StringName changed = "changed";
	static inline const StringName script = "script";
	static inline const StringName script_changed = "script_changed";
	static inline const StringName _iter_init = "_iter_init";
	static inline const StringName _iter_next = "_iter_next";
	static inline const StringName _iter_get = "_iter_get";
	static inline const StringName get_rid = "get_rid";
	static inline const StringName _to_string = "_to_string";
	static inline const StringName _custom_features = "_custom_features";

	static inline const StringName x = "x";
	static inline const StringName y = "y";
	static inline const StringName z = "z";
	static inline const StringName w = "w";
	static inline const StringName r = "r";
	static inline const StringName g = "g";
	static inline const StringName b = "b";
	static inline const StringName a = "a";
	static inline const StringName position = "position";
	static inline const StringName size = "size";
	static inline const StringName end = "end";
	static inline const StringName basis = "basis";
	static inline const StringName origin = "origin";
	static inline const StringName normal = "normal";
	static inline const StringName d = "d";
	static inline const StringName h = "h";
	static inline const StringName s = "s";
	static inline const StringName v = "v";
	static inline const StringName r8 = "r8";
	static inline const StringName g8 = "g8";
	static inline const StringName b8 = "b8";
	static inline const StringName a8 = "a8";

	static inline const StringName call = "call";
	static inline const StringName call_deferred = "call_deferred";
	static inline const StringName bind = "bind";
	static inline const StringName notification = "notification";
	static inline const StringName property_list_changed = "property_list_changed";
};


