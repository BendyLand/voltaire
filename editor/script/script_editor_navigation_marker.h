/**************************************************************************/
/*  script_editor_navigation_marker.h                                     */
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

#include <cstdint>

class ScriptEditorNavigationMarker final
{
	struct Data
	{
		bool init_in_progress = false;

		bool locate_in_progress = false;
		uint64_t locate_end_physics_frame = 0;
		uint64_t locate_end_process_frame = 0;

		bool traverse_in_progress = false;
		uint64_t traverse_end_physics_frame = 0;
		uint64_t traverse_end_process_frame = 0;
	};

public:
	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static void init_begin();
	static void init_end();
	static void locate_begin();
	static void locate_end();
	static void traverse_begin();
	static void traverse_end();

	static bool is_initializing();
	static bool is_locating();
	static bool is_traversing();

	static bool is_locate_just_occured();
	static bool is_traverse_just_occured();

	ScriptEditorNavigationMarker() = delete;
	~ScriptEditorNavigationMarker() = delete;
};
