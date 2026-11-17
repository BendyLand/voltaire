/**************************************************************************/
/*  openxr_eye_gaze_interaction.cpp                                       */
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

#include "../action_map/openxr_interaction_profile_metadata.h"
#include "../openxr_api.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "openxr_eye_gaze_interaction.h"

void OpenXREyeGazeInteractionExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXREyeGazeInteractionExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}


HashMap<String, bool*> OpenXREyeGazeInteractionExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	// Only enable this extension when requested.
	// We still register our meta data or the action map editor will fail.
	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/eye_gaze_interaction") &&
		(!OS::has_feature("mobile") ||
			OS::has_feature(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME))) {
		request_extensions[XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME] = &data->available;
	}

	return request_extensions;
}

void* OpenXREyeGazeInteractionExtension::set_system_properties_and_get_next_pointer(
	void* p_next_pointer)
{
	if (!data->available) {
		return p_next_pointer;
	}

	data->properties.type = XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT;
	data->properties.next = p_next_pointer;
	data->properties.supportsEyeGazeInteraction = false;

	return &data->properties;
}

PackedStringArray OpenXREyeGazeInteractionExtension::get_suggested_tracker_names()
{
	PackedStringArray arr = {"/user/eyes_ext"};
	return arr;
}

bool OpenXREyeGazeInteractionExtension::is_available() { return data != nullptr && data->available; }

bool OpenXREyeGazeInteractionExtension::supports_eye_gaze_interaction()
{
	// The extension being available only means that the OpenXR Runtime supports the extension.
	// The `supportsEyeGazeInteraction` is set to true if the device also supports this.
	// Thus both need to be true.
	// In addition, on mobile runtimes, the proper permission needs to be granted.
	if (data->available && data->properties.supportsEyeGazeInteraction) {
		return !OS::has_feature("mobile") ||
			   OS::has_feature("PERMISSION_XR_EXT_eye_gaze_interaction");
	}

	return false;
}

bool OpenXREyeGazeInteractionExtension::get_eye_gaze_pose(double p_dist, Vector3& r_eye_pose)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	if (!data->init_eye_gaze_pose) {
		data->init_eye_gaze_pose = true;

		data->eye_tracker = OpenXRAPI::find_tracker("/user/eyes_ext");
		if (data->eye_tracker.is_null()) {
			WARN_PRINT("Couldn't obtain eye tracker");
		}

		data->eye_action = OpenXRAPI::find_action("eye_gaze_pose");
		if (data->eye_action.is_null()) {
			WARN_PRINT("Couldn't obtain pose action for `eye_gaze_pose`, make sure to add this to "
					   "your action map.");
		}
	}

	if (data->eye_tracker.is_null() || data->eye_action.is_null()) {
		return false;
	}

	Transform3D eye_transform;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	XRPose::TrackingConfidence confidence = OpenXRAPI::get_action_pose(
		data->eye_action, data->eye_tracker, eye_transform, linear_velocity, angular_velocity);
	if (confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
		return false;
	}

	r_eye_pose = eye_transform.origin + eye_transform.basis[2] * p_dist;

	return true;
}


