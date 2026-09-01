/**************************************************************************/
/*  openxr_editor_plugin.cpp                                              */
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

#include <openxr/openxr.h>
#include "../action_map/openxr_action_map.h"
#include "../openxr_api.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_node.h"
#include "openxr_editor_plugin.h"
#include "platform/android/export/export_plugin.h"

PackedStringArray OpenXRExportPlugin::get_android_dependencies(
	const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const
{
	PackedStringArray ret;

	if (!supports_platform(p_export_platform)) {
		return ret;
	}

	if (is_openxr_mode()) {
		// Loader is always identified by the full API version even if we're initializing for
		// OpenXR 1.0.
		int major = XR_VERSION_MAJOR(XR_CURRENT_API_VERSION);
		int minor = XR_VERSION_MINOR(XR_CURRENT_API_VERSION);
		int patch = XR_VERSION_PATCH(XR_CURRENT_API_VERSION);
		String openxr_loader =
			"org.khronos.openxr:openxr_loader_for_android:" + String::num_int64(major) + "." +
			String::num_int64(minor) + "." + String::num_int64(patch);

		ret.push_back(openxr_loader);
	}

	return ret;
}

PackedStringArray OpenXRExportPlugin::_get_export_features(
	const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const
{
	PackedStringArray features;

	if (!supports_platform(p_export_platform) || !is_openxr_mode()) {
		return features;
	}

	// Placeholder for now

	return features;
}

String OpenXRExportPlugin::get_android_manifest_activity_element_contents(
	const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const
{
	String contents;

	if (!supports_platform(p_export_platform) || !is_openxr_mode()) {
		return contents;
	}

	contents += R"n(
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />

				<category android:name="android.intent.category.DEFAULT" />

                <category android:name="org.khronos.openxr.intent.category.IMMERSIVE_HMD" />
            </intent-filter>
)n";

	return contents;
}

void OpenXREditorPlugin::make_visible(bool p_visible) {}

void OpenXREditorPlugin::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		// Initialize our export plugin
		openxr_export_plugin.instantiate();
		add_export_plugin(openxr_export_plugin);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		// Clean up our export plugin
		remove_export_plugin(openxr_export_plugin);

		openxr_export_plugin.unref();
	}
	}
}


