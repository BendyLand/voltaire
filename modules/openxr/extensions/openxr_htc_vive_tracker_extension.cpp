/**************************************************************************/
/*  openxr_htc_vive_tracker_extension.cpp                                 */
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

#include "../action_map/openxr_interaction_profile_metadata.h"
#include "core/string/print_string.h"
#include "openxr_htc_vive_tracker_extension.h"

HashMap<String, bool*> OpenXRHTCViveTrackerExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME] = &available;

	return request_extensions;
}

PackedStringArray OpenXRHTCViveTrackerExtension::get_suggested_tracker_names()
{
	PackedStringArray arr = {
		"/user/vive_tracker_htcx/role/handheld_object",
		"/user/vive_tracker_htcx/role/left_foot",
		"/user/vive_tracker_htcx/role/right_foot",
		"/user/vive_tracker_htcx/role/left_shoulder",
		"/user/vive_tracker_htcx/role/right_shoulder",
		"/user/vive_tracker_htcx/role/left_elbow",
		"/user/vive_tracker_htcx/role/right_elbow",
		"/user/vive_tracker_htcx/role/left_knee",
		"/user/vive_tracker_htcx/role/right_knee",
		"/user/vive_tracker_htcx/role/waist",
		"/user/vive_tracker_htcx/role/chest",
		"/user/vive_tracker_htcx/role/camera",
		"/user/vive_tracker_htcx/role/keyboard",
		"/user/vive_tracker_htcx/role/left_wrist",
		"/user/vive_tracker_htcx/role/right_wrist",
		"/user/vive_tracker_htcx/role/left_ankle",
		"/user/vive_tracker_htcx/role/right_ankle",
	};
	return arr;
}

bool OpenXRHTCViveTrackerExtension::is_available() { return available; }

bool OpenXRHTCViveTrackerExtension::on_event_polled(const XrEventDataBuffer& event)
{
	switch (event.type) {
	case XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX: {
		// Investigate if we need to do more here
		print_verbose("OpenXR EVENT: VIVE tracker connected");

		return true;
	} break;
	default: {
		return false;
	} break;
	}
}


