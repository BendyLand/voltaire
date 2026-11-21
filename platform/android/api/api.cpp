/**************************************************************************/
/*  api.cpp                                                               */
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

#include "api.h"
#include "java_class_wrapper.h"
#include "platform/android/os_android.h"

void unregister_android_api()
{
#if !defined(ANDROID_ENABLED)
	JavaClassWrapper::finalize();
#endif
}

#if !defined(ANDROID_ENABLED)

String JavaClass::get_java_class_name() const { return ""; }

Ref<JavaClass> JavaClass::get_java_parent_class() const { return Ref<JavaClass>(); }

bool JavaClass::has_java_method(const String&) const { return false; }

JavaClass::JavaClass() {}

JavaClass::~JavaClass() {}

Ref<JavaClass> JavaObject::get_java_class() const { return Ref<JavaClass>(); }

bool JavaObject::has_java_method(const String&) const { return false; }

void JavaClassWrapper::initialize() {}

void JavaClassWrapper::finalize() {}

Ref<JavaClass> JavaClassWrapper::_wrap(const String&, bool) { return Ref<JavaClass>(); }

void register_android_api() {}

#endif

// ============================================================================
// Merged from platform/ios/api/api.cpp
// ============================================================================

#include "platform/ios/api/api.h"

#if defined(IOS_ENABLED)

void register_ios_api() {
	godot_apple_embedded_plugins_initialize();
}

void unregister_ios_api() {
	godot_apple_embedded_plugins_deinitialize();
}

#else

void register_ios_api() {}
void unregister_ios_api() {}

#endif // IOS_ENABLED

// ============================================================================
// Merged from platform/visionos/api/api.cpp
// ============================================================================

#include "platform/visionos/api/api.h"

#if defined(VISIONOS_ENABLED)

void register_visionos_api() {
	godot_apple_embedded_plugins_initialize();
}

void unregister_visionos_api() {
	godot_apple_embedded_plugins_deinitialize();
}

#else

void register_visionos_api() {}
void unregister_visionos_api() {}

#endif // VISIONOS_ENABLED

// ============================================================================
// Merged from platform/web/api/api.cpp
// ============================================================================

#include "platform/web/api/api.h"
#include "platform/web/os_web.h"

void unregister_web_api() {}

void register_web_api() {}

#if !defined(WEB_ENABLED)

bool JavaScriptBridge::pwa_needs_update() { return false; }

Error JavaScriptBridge::pwa_update() { return ERR_UNAVAILABLE; }

void JavaScriptBridge::force_fs_sync() {}

void JavaScriptBridge::download_buffer(
	Vector<uint8_t> p_arr, const String& p_name, const String& p_mime)
{
}

#endif
