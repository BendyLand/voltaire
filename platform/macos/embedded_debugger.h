/**************************************************************************/
/*  embedded_debugger.h                                                   */
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

#include "core/templates/hash_map.h"

class DisplayServerMacOSEmbedded;

class EmbeddedDebugger final
{
#ifdef DEBUG_ENABLED
	typedef Error (*ParseMessageFunc)(const Array& p_args);

	struct Data
	{
		DisplayServerMacOSEmbedded* ds = nullptr;
		HashMap<String, ParseMessageFunc> parse_message_handlers;
	};

	static inline Data* data = nullptr;

	static void _init_parse_message_handlers();

	static Error _msg_window_size(const Array& p_args);
	static Error _msg_mouse_set_mode(const Array& p_args);
	static Error _msg_event(const Array& p_args);
	static Error _msg_win_event(const Array& p_args);
	static Error _msg_notification(const Array& p_args);
	static Error _msg_ime_update(const Array& p_args);
	static Error _msg_ds_state(const Array& p_args);
#endif

public:
	EmbeddedDebugger() = delete;

	static void initialize(DisplayServerMacOSEmbedded* p_ds);
	static void deinitialize();

#ifdef DEBUG_ENABLED
	static Error parse_message(
		void* p_user, const String& p_msg, const Array& p_args, bool& r_captured);
#endif
};
