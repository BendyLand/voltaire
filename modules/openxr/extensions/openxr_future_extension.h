/**************************************************************************/
/*  openxr_future_extension.h                                             */
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

#pragma once

/*
	The OpenXR future extension forms the basis of OpenXR's ability to
	execute logic asynchronously.

	Asynchronous functions will return a future object which can be
	polled each frame to determine if the asynchronous function has
	been completed.

	If so the future can be used to obtain final return values.
	The API call for this is often part of the extension that utilizes
	the future.
*/

#include <openxr/openxr.h>
#include "../util.h"
#include "core/types.h"

class OpenXRFutureExtension;

class OpenXRFutureResult : public RefCounted
{
	friend class OpenXRFutureExtension;

protected:
	static void _bind_methods();

	void _mark_as_finished();
	void _mark_as_cancelled();

public:
	enum ResultStatus
	{
		RESULT_RUNNING,
		RESULT_FINISHED,
		RESULT_CANCELLED,
	};

	ResultStatus get_status() const;
	XrFutureEXT get_future() const;

	void cancel_future();

private:
	ResultStatus status = RESULT_RUNNING;
	XrFutureEXT future;

	uint64_t _get_future() const;
};

class OpenXRFutureExtension
{
protected:
	static void _bind_methods();

public:
	static OpenXRFutureExtension* get_singleton();

	OpenXRFutureExtension();
	virtual ~OpenXRFutureExtension();

	virtual HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	virtual void on_instance_created(const XrInstance p_instance);
	virtual void on_instance_destroyed();
	virtual void on_session_destroyed();

	virtual void on_process();

	bool is_active() const;

	void cancel_future(XrFutureEXT p_future);

private:
	static OpenXRFutureExtension* singleton;

	bool future_ext = false;

	HashMap<XrFutureEXT, Ref<OpenXRFutureResult>> futures;

	void _cancel_future(uint64_t p_future);

	// Futures
	EXT_PROTO_XRRESULT_FUNC3(xrPollFutureEXT, (XrInstance), instance, (const XrFuturePollInfoEXT*),
		poll_info, (XrFuturePollResultEXT*), poll_result);
	EXT_PROTO_XRRESULT_FUNC2(
		xrCancelFutureEXT, (XrInstance), instance, (const XrFutureCancelInfoEXT*), cancel_info);
};


