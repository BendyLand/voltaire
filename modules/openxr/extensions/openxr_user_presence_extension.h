/**************************************************************************/
/*  openxr_user_presence_extension.h                                      */
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

#include "core/types.h"

// When supported, the user presence extension allows an application to detect and respond to change
// of user presence, such as when the user has taken off or put on an XR headset.
//
// See: https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#XR_EXT_user_presence
// for more information.

class OpenXRUserPresenceExtension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);

	static void on_state_ready();
	static void on_state_stopping();

	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_user_present();
	static bool is_active();

private:

public:
	struct Data
	{
		bool available = false;
		XrSystemUserPresencePropertiesEXT properties;
		bool user_present = true;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRUserPresenceExtension() = delete;
	OpenXRUserPresenceExtension(const OpenXRUserPresenceExtension&) = delete;
	OpenXRUserPresenceExtension& operator=(const OpenXRUserPresenceExtension&) = delete;
	~OpenXRUserPresenceExtension() = delete;
};


