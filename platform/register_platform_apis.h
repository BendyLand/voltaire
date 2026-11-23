/**************************************************************************/
/*  register_platform_apis.h                                              */
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

void register_platform_apis();
void unregister_platform_apis();

// --- Folded from platform/android/api/api.h ---
void register_android_api();
void unregister_android_api();
// --- End platform/android/api/api.h ---

// --- Folded from platform/ios/api/api.h ---
#if defined(IOS_ENABLED)
extern void godot_apple_embedded_plugins_initialize();
extern void godot_apple_embedded_plugins_deinitialize();
#endif

void register_ios_api();
void unregister_ios_api();
// --- End platform/ios/api/api.h ---

// --- Folded from platform/visionos/api/api.h ---
#if defined(VISIONOS_ENABLED)
extern void godot_apple_embedded_plugins_initialize();
extern void godot_apple_embedded_plugins_deinitialize();
#endif

void register_visionos_api();
void unregister_visionos_api();
// --- End platform/visionos/api/api.h ---

// --- Folded from platform/web/api/api.h ---
void register_web_api();
void unregister_web_api();
// --- End platform/web/api/api.h ---
