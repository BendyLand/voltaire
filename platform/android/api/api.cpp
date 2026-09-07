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
#include "java_class_wrapper.h"
#include "jni_singleton.h"

#if !defined(ANDROID_ENABLED)
static JavaClassWrapper* java_class_wrapper = nullptr;
#endif

void unregister_android_api()
{
#if !defined(ANDROID_ENABLED)
	memdelete(java_class_wrapper);
#endif
}

#if !defined(ANDROID_ENABLED)

String JavaClass::get_java_class_name() const { return ""; }

Ref<JavaClass> JavaClass::get_java_parent_class() const { return Ref<JavaClass>(); }

bool JavaClass::has_java_method(const StringName&) const { return false; }

JavaClass::JavaClass() {}

JavaClass::~JavaClass() {}

Ref<JavaClass> JavaObject::get_java_class() const { return Ref<JavaClass>(); }

bool JavaObject::has_java_method(const StringName&) const { return false; }

JavaClassWrapper* JavaClassWrapper::singleton = nullptr;

Ref<JavaClass> JavaClassWrapper::_wrap(const String&, bool) { return Ref<JavaClass>(); }

JavaClassWrapper::JavaClassWrapper() { singleton = this; }

#endif


