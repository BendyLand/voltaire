/**************************************************************************/
/*  openxr_user_presence_extension.cpp                                    */
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

#include "../openxr_interface.h"
#include "core/config/project_settings.h"
#include "openxr_user_presence_extension.h"

OpenXRUserPresenceExtension* OpenXRUserPresenceExtension::singleton = nullptr;

OpenXRUserPresenceExtension* OpenXRUserPresenceExtension::get_singleton() { return singleton; }

OpenXRUserPresenceExtension::OpenXRUserPresenceExtension() { singleton = this; }

OpenXRUserPresenceExtension::~OpenXRUserPresenceExtension() { singleton = nullptr; }



void* OpenXRUserPresenceExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
	if (!available) {
		return p_next_pointer;
	}

	properties.type = XR_TYPE_SYSTEM_USER_PRESENCE_PROPERTIES_EXT;
	properties.next = p_next_pointer;
	properties.supportsUserPresence = false;

	return &properties;
}

bool OpenXRUserPresenceExtension::is_active() const
{
	return available && properties.supportsUserPresence;
}

void OpenXRUserPresenceExtension::on_state_ready() { user_present = true; }

void OpenXRUserPresenceExtension::on_state_stopping() { user_present = false; }

bool OpenXRUserPresenceExtension::is_user_present() const { return user_present; }


