/**************************************************************************/
/*  openxr_hand_tracking_extension.h                                      */
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

#include "../util.h"
#include "core/math/quaternion.h"
#include "servers/xr/xr_hand_tracker.h"

class OpenXRHandTrackingExtension
{
public:
	enum HandTrackedHands
	{
		OPENXR_TRACKED_LEFT_HAND,
		OPENXR_TRACKED_RIGHT_HAND,
		OPENXR_MAX_TRACKED_HANDS
	};

	enum HandTrackedSource
	{
		OPENXR_SOURCE_UNKNOWN,
		OPENXR_SOURCE_UNOBSTRUCTED,
		OPENXR_SOURCE_CONTROLLER,
		OPENXR_SOURCE_NOT_TRACKED,
		OPENXR_SOURCE_MAX
	};

	struct HandTracker
	{
		bool is_initialized = false;
		Ref<XRHandTracker> godot_tracker;
		HandTrackedSource source = OPENXR_SOURCE_UNKNOWN;
	};

	static OpenXRHandTrackingExtension* get_singleton();

	OpenXRHandTrackingExtension();
	virtual ~OpenXRHandTrackingExtension();

	virtual void on_instance_destroyed();
	virtual void on_session_destroyed();

	virtual void* set_system_properties_and_get_next_pointer(void* p_next_pointer);
	virtual void on_state_ready();
	virtual void on_process();
	virtual void on_state_stopping();

	bool get_active();
	const HandTracker* get_hand_tracker(HandTrackedHands p_hand) const;

private:
	static OpenXRHandTrackingExtension* singleton;

	HandTracker hand_trackers[OPENXR_MAX_TRACKED_HANDS]; // Fixed for left and right hand

	// related extensions
	bool hand_tracking_ext = false;
	bool hand_motion_range_ext = false;
	bool hand_tracking_source_ext = false;
	bool unobstructed_data_source = false;
	bool controller_data_source = false;

	// functions
	void cleanup_hand_tracking();
};


