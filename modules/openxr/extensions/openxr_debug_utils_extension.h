/**************************************************************************/
/*  openxr_debug_utils_extension.h                                        */
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

#include "../util.h"
#include "core/types.h"
#include "thirdparty/openxr/include/openxr/openxr.h"


class OpenXRDebugUtilsExtension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static bool get_active();

	static void set_object_name(
		XrObjectType p_object_type, uint64_t p_object_handle, const char* p_object_name);
	static void begin_debug_label_region(const char* p_label_name);
	static void end_debug_label_region();
	static void insert_debug_label(const char* p_label_name);

private:

	static XrBool32 XRAPI_PTR _debug_callback(
		XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
		XrDebugUtilsMessageTypeFlagsEXT p_message_types,
		const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);
	static XrBool32 debug_callback(XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
		XrDebugUtilsMessageTypeFlagsEXT p_message_types,
		const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);

	// OpenXR API call wrappers
	EXT_PROTO_XRRESULT_FUNC3(xrCreateDebugUtilsMessengerEXT, (XrInstance), p_instance,
		(const XrDebugUtilsMessengerCreateInfoEXT*), p_create_info, (XrDebugUtilsMessengerEXT*),
		p_messenger)
	EXT_PROTO_XRRESULT_FUNC1(
		xrDestroyDebugUtilsMessengerEXT, (XrDebugUtilsMessengerEXT), p_messenger)
	EXT_PROTO_XRRESULT_FUNC2(xrSetDebugUtilsObjectNameEXT, (XrInstance), p_instance,
		(const XrDebugUtilsObjectNameInfoEXT*), p_name_info)
	EXT_PROTO_XRRESULT_FUNC2(xrSessionBeginDebugUtilsLabelRegionEXT, (XrSession), p_session,
		(const XrDebugUtilsLabelEXT*), p_label_info)
	EXT_PROTO_XRRESULT_FUNC1(xrSessionEndDebugUtilsLabelRegionEXT, (XrSession), p_session)
	EXT_PROTO_XRRESULT_FUNC2(xrSessionInsertDebugUtilsLabelEXT, (XrSession), p_session,
		(const XrDebugUtilsLabelEXT*), p_label_info)

public:
	struct Data
	{
		bool debug_utils_ext = false;
		XrDebugUtilsMessengerEXT default_messenger = XR_NULL_HANDLE;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRDebugUtilsExtension() = delete;
	OpenXRDebugUtilsExtension(const OpenXRDebugUtilsExtension&) = delete;
	OpenXRDebugUtilsExtension& operator=(const OpenXRDebugUtilsExtension&) = delete;
	~OpenXRDebugUtilsExtension() = delete;
};


