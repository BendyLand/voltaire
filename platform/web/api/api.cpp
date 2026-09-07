/**************************************************************************/
/*  api.cpp                                                               */
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

#include "api.h"
#include "core/config/engine.h"
#include "javascript_bridge_singleton.h"

static JavaScriptBridge* javascript_bridge_singleton;

void unregister_web_api() { memdelete(javascript_bridge_singleton); }

JavaScriptBridge* JavaScriptBridge::singleton = nullptr;

JavaScriptBridge* JavaScriptBridge::get_singleton() { return singleton; }

JavaScriptBridge::JavaScriptBridge()
{
	ERR_FAIL_COND_MSG(singleton != nullptr, "JavaScriptBridge singleton already exists.");
	singleton = this;
}

JavaScriptBridge::~JavaScriptBridge() {}

#if !defined(WEB_ENABLED)

bool JavaScriptBridge::pwa_needs_update() const { return false; }

Error JavaScriptBridge::pwa_update() { return ERR_UNAVAILABLE; }

void JavaScriptBridge::force_fs_sync() {}

void JavaScriptBridge::download_buffer(
	Vector<uint8_t> p_arr, const String& p_name, const String& p_mime)
{
}

#endif


