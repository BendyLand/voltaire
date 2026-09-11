/**************************************************************************/
/*  openxr_meta_controller_extension.cpp                                  */
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
#include "../openxr_api.h"
#include "openxr_meta_controller_extension.h"

const char* touch_controller = "/interaction_profiles/oculus/touch_controller";
const char* touch_pro_controller = "/interaction_profiles/meta/touch_pro_controller";
const char* touch_plus_controller = "/interaction_profiles/meta/touch_plus_controller";
const char* rift_cv1_controller = "/interaction_profiles/meta/touch_controller_rift_cv1";
const char* quest1_rift_s_controller = "/interaction_profiles/meta/touch_controller_quest_1_rift_s";
const char* quest2_controller = "/interaction_profiles/meta/touch_controller_quest_2";

// Vendor extension paths (promoted)
const char* touch_controller_pro_fb = "/interaction_profiles/facebook/touch_controller_pro";
const char* touch_controller_plus_fb = "/interaction_profiles/facebook/touch_controller_plus";
const char* touch_controller_plus_meta = "/interaction_profiles/meta/touch_controller_plus";

HashMap<String, bool*> OpenXRMetaControllerExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (p_version < XR_API_VERSION_1_1_0) {
		// Extensions where promoted in OpenXR 1.1, only include it in OpenXR 1.0.
		request_extensions[XR_FB_TOUCH_CONTROLLER_PRO_EXTENSION_NAME] = &available[META_TOUCH_PRO];
		request_extensions[XR_META_TOUCH_CONTROLLER_PLUS_EXTENSION_NAME] =
			&available[META_TOUCH_PLUS];
	}

	request_extensions[XR_FB_TOUCH_CONTROLLER_PROXIMITY_EXTENSION_NAME] =
		&available[META_TOUCH_PROXIMITY];

	return request_extensions;
}

bool OpenXRMetaControllerExtension::is_available(MetaControllers p_type)
{
	return available[p_type];
}


