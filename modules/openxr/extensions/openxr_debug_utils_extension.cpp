/**************************************************************************/
/*  openxr_debug_utils_extension.cpp                                      */
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

#include "openxr_debug_utils_extension.h"

#include "../openxr_api.h"

#include "core/config/project_settings.h"
#include "core/string/print_string.h"

#include <openxr/openxr.h>

OpenXRDebugUtilsExtension *OpenXRDebugUtilsExtension::singleton = nullptr;

OpenXRDebugUtilsExtension *OpenXRDebugUtilsExtension::get_singleton() {
	return singleton;
}

OpenXRDebugUtilsExtension::OpenXRDebugUtilsExtension() {
	singleton = this;
}

OpenXRDebugUtilsExtension::~OpenXRDebugUtilsExtension() {
	singleton = nullptr;
}

HashMap<String, bool *> OpenXRDebugUtilsExtension::get_requested_extensions(XrVersion p_version) {
	HashMap<String, bool *> request_extensions;

	request_extensions[XR_EXT_DEBUG_UTILS_EXTENSION_NAME] = &debug_utils_ext;

	return request_extensions;
}



void OpenXRDebugUtilsExtension::on_instance_destroyed() {
	if (default_messenger != XR_NULL_HANDLE) {
		XrResult result = xrDestroyDebugUtilsMessengerEXT(default_messenger);
		if (XR_FAILED(result)) {
			ERR_PRINT("OpenXR: Failed to destroy debug callback [" + OpenXRAPI::get_singleton()->get_error_string(result) + "]");
		}

		default_messenger = XR_NULL_HANDLE;
	}

	xrCreateDebugUtilsMessengerEXT_ptr = nullptr;
	xrDestroyDebugUtilsMessengerEXT_ptr = nullptr;
	xrSetDebugUtilsObjectNameEXT_ptr = nullptr;
	xrSessionBeginDebugUtilsLabelRegionEXT_ptr = nullptr;
	xrSessionEndDebugUtilsLabelRegionEXT_ptr = nullptr;
	xrSessionInsertDebugUtilsLabelEXT_ptr = nullptr;
	debug_utils_ext = false;
}

bool OpenXRDebugUtilsExtension::get_active() {
	return debug_utils_ext;
}

void OpenXRDebugUtilsExtension::set_object_name(XrObjectType p_object_type, uint64_t p_object_handle, const char *p_object_name) {
	ERR_FAIL_COND(!debug_utils_ext);
	ERR_FAIL_NULL(xrSetDebugUtilsObjectNameEXT_ptr);

	const XrDebugUtilsObjectNameInfoEXT space_name_info = {
		XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, // type
		nullptr, // next
		p_object_type, // objectType
		p_object_handle, // objectHandle
		p_object_name, // objectName
	};

	XrResult result = xrSetDebugUtilsObjectNameEXT_ptr(OpenXRAPI::get_singleton()->get_instance(), &space_name_info);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to set object name [" + OpenXRAPI::get_singleton()->get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::begin_debug_label_region(const char *p_label_name) {
	ERR_FAIL_COND(!debug_utils_ext);
	ERR_FAIL_NULL(xrSessionBeginDebugUtilsLabelRegionEXT_ptr);

	const XrDebugUtilsLabelEXT session_active_region_label = {
		XR_TYPE_DEBUG_UTILS_LABEL_EXT, // type
		nullptr, // next
		p_label_name, // labelName
	};

	XrResult result = xrSessionBeginDebugUtilsLabelRegionEXT_ptr(OpenXRAPI::get_singleton()->get_session(), &session_active_region_label);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to begin label region [" + OpenXRAPI::get_singleton()->get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::end_debug_label_region() {
	ERR_FAIL_COND(!debug_utils_ext);
	ERR_FAIL_NULL(xrSessionEndDebugUtilsLabelRegionEXT_ptr);

	XrResult result = xrSessionEndDebugUtilsLabelRegionEXT_ptr(OpenXRAPI::get_singleton()->get_session());
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to end label region [" + OpenXRAPI::get_singleton()->get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::insert_debug_label(const char *p_label_name) {
	ERR_FAIL_COND(!debug_utils_ext);
	ERR_FAIL_NULL(xrSessionInsertDebugUtilsLabelEXT_ptr);

	const XrDebugUtilsLabelEXT session_active_region_label = {
		XR_TYPE_DEBUG_UTILS_LABEL_EXT, // type
		nullptr, // next
		p_label_name, // labelName
	};

	XrResult result = xrSessionInsertDebugUtilsLabelEXT_ptr(OpenXRAPI::get_singleton()->get_session(), &session_active_region_label);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to insert label [" + OpenXRAPI::get_singleton()->get_error_string(result) + "]");
	}
}

XrBool32 XRAPI_PTR OpenXRDebugUtilsExtension::_debug_callback(XrDebugUtilsMessageSeverityFlagsEXT p_message_severity, XrDebugUtilsMessageTypeFlagsEXT p_message_types, const XrDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data) {
	OpenXRDebugUtilsExtension *debug_utils = OpenXRDebugUtilsExtension::get_singleton();

	if (debug_utils) {
		return debug_utils->debug_callback(p_message_severity, p_message_types, p_callback_data, p_user_data);
	}

	return XR_FALSE;
}

XrBool32 OpenXRDebugUtilsExtension::debug_callback(XrDebugUtilsMessageSeverityFlagsEXT p_message_severity, XrDebugUtilsMessageTypeFlagsEXT p_message_types, const XrDebugUtilsMessengerCallbackDataEXT *p_callback_data, void *p_user_data) {
	String msg;

	ERR_FAIL_NULL_V(p_callback_data, XR_FALSE);

	if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		msg = ", type: General";
	} else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		msg = ", type: Validation";
	} else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		msg = ", type: Performance";
	} else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT) {
		msg = ", type: Conformance";
	} else {
		msg = ", type: Unknown (" + String::num_uint64(p_message_types) + ")";
	}

	if (p_callback_data->functionName) {
		msg += ", function Name: " + String(p_callback_data->functionName);
	}
	if (p_callback_data->messageId) {
		msg += "\nMessage ID: " + String(p_callback_data->messageId);
	}
	if (p_callback_data->message) {
		msg += "\nMessage: " + String(p_callback_data->message);
	}

	if (p_callback_data->objectCount > 0) {
		String objects;

		for (uint32_t i = 0; i < p_callback_data->objectCount; i++) {
			if (!objects.is_empty()) {
				objects += ", ";
			}
			objects += p_callback_data->objects[i].objectName;
		}

		msg += "\nObjects: " + objects;
	}

	if (p_callback_data->sessionLabelCount > 0) {
		String labels;

		for (uint32_t i = 0; i < p_callback_data->sessionLabelCount; i++) {
			if (!labels.is_empty()) {
				labels += ", ";
			}
			labels += p_callback_data->sessionLabels[i].labelName;
		}

		msg += "\nLabels: " + labels;
	}

	if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		ERR_PRINT("OpenXR: Severity: Error" + msg);
	} else if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		WARN_PRINT("OpenXR: Severity: Warning" + msg);
	} else if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		// This is a bit double because we won't output this unless verbose messaging in Godot is on.
		print_verbose("OpenXR: Severity: Verbose" + msg);
	}

	return XR_FALSE;
}
