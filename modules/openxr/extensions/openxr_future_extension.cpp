/**************************************************************************/
/*  openxr_future_extension.cpp                                           */
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

#include "../openxr_api.h"
#include "openxr_future_extension.h"

void OpenXRFutureExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRFutureExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}


////////////////////////////////////////////////////////////////////////////
// OpenXRFutureResult

OpenXRFutureResult::ResultStatus OpenXRFutureResult::get_status() const { return status; }

XrFutureEXT OpenXRFutureResult::get_future() const { return future; }

uint64_t OpenXRFutureResult::_get_future() const { return (uint64_t)future; }

void OpenXRFutureResult::cancel_future()
{
	ERR_FAIL_COND(status != RESULT_RUNNING);

	ERR_FAIL_COND(!OpenXRFutureExtension::is_initialized());

	OpenXRFutureExtension::cancel_future(future);
}

////////////////////////////////////////////////////////////////////////////
// OpenXRFutureExtension

HashMap<String, bool*> OpenXRFutureExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_FUTURE_EXTENSION_NAME] = &data->future_ext;

	return request_extensions;
}

void OpenXRFutureExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->future_ext) {
		EXT_INIT_XR_FUNC(xrPollFutureEXT);
		EXT_INIT_XR_FUNC(xrCancelFutureEXT);

		data->future_ext = xrPollFutureEXT_ptr && xrCancelFutureEXT_ptr;
	}
}

void OpenXRFutureExtension::on_instance_destroyed()
{
	xrPollFutureEXT_ptr = nullptr;
	xrCancelFutureEXT_ptr = nullptr;
}

void OpenXRFutureExtension::on_session_destroyed()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	// Cancel any running futures.
	for (const KeyValue<XrFutureEXT, Ref<OpenXRFutureResult>>& element : data->futures) {
		XrFutureCancelInfoEXT cancel_info = {
			XR_TYPE_FUTURE_CANCEL_INFO_EXT, // type
			nullptr,						// next
			element.key						// future
		};
		XrResult result = xrCancelFutureEXT_ptr(OpenXRAPI::get_instance(), &cancel_info);
		if (XR_FAILED(result)) {
			WARN_PRINT(
				"OpenXR: Failed to cancel future [" + OpenXRAPI::get_error_string(result) + "]");
		}

		// Make sure we mark our future result as cancelled
		element.value->_mark_as_cancelled();
	}
	data->futures.clear();
}

bool OpenXRFutureExtension::is_active() { return data != nullptr && data->future_ext; }

void OpenXRFutureExtension::cancel_future(XrFutureEXT p_future)
{
	ERR_FAIL_COND(!data->futures.has(p_future));

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	XrFutureCancelInfoEXT cancel_info = {
		XR_TYPE_FUTURE_CANCEL_INFO_EXT, // type
		nullptr,						// next
		p_future						// future
	};
	XrResult result = xrCancelFutureEXT_ptr(OpenXRAPI::get_instance(), &cancel_info);
	if (XR_FAILED(result)) {
		WARN_PRINT(
			"OpenXR: Failed to cancel future [" + OpenXRAPI::get_error_string(result) + "]");
	}

	// Make sure we mark our future result as cancelled
	data->futures[p_future]->_mark_as_cancelled();

	// And erase it from the futures we track
	data->futures.erase(p_future);
}

void OpenXRFutureExtension::_cancel_future(uint64_t p_future)
{
	cancel_future((XrFutureEXT)p_future);
}

void OpenXRFutureExtension::on_process()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	// Process futures
	Vector<XrFutureEXT> completed;
	for (const KeyValue<XrFutureEXT, Ref<OpenXRFutureResult>>& element : data->futures) {
		XrFuturePollInfoEXT poll_info = {
			XR_TYPE_FUTURE_POLL_INFO_EXT, // type
			nullptr,					  // next
			element.key					  // future
		};
		XrFuturePollResultEXT poll_result = {
			XR_TYPE_FUTURE_POLL_RESULT_EXT, // type
			nullptr,						// next
			XR_FUTURE_STATE_MAX_ENUM_EXT	// state
		};
		XrResult result = xrPollFutureEXT_ptr(OpenXRAPI::get_instance(), &poll_info, &poll_result);
		if (XR_FAILED(result)) {
			ERR_PRINT("OpenXR: Failed to obtain future status [" +
					  OpenXRAPI::get_error_string(result) + "]");
			// Maybe remove this depending on the error?
			continue;
		}
		if (poll_result.state == XR_FUTURE_STATE_READY_EXT) {
			// Mark our future result as finished (this will invoke our callback).
			element.value->_mark_as_finished();

			// Queue removing this
			completed.push_back(element.key);
		}
	}

	// Now clean up completed futures.
	for (const XrFutureEXT& future : completed) {
		data->futures.erase(future);
	}
}



void OpenXRFutureResult::_mark_as_cancelled() {}

void OpenXRFutureResult::_mark_as_finished() {}
