/**************************************************************************/
/*  openxr_hand_tracking_extension.cpp                                    */
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
#include "../openxr_api.h"
#include "core/config/project_settings.h"
#include "core/string/print_string.h"
#include "openxr_hand_tracking_extension.h"
#include "servers/xr/xr_server.h"

OpenXRHandTrackingExtension* OpenXRHandTrackingExtension::singleton = nullptr;

OpenXRHandTrackingExtension* OpenXRHandTrackingExtension::get_singleton() { return singleton; }

OpenXRHandTrackingExtension::~OpenXRHandTrackingExtension() { singleton = nullptr; }

void OpenXRHandTrackingExtension::on_session_destroyed() { cleanup_hand_tracking(); }

void OpenXRHandTrackingExtension::on_state_stopping()
{
	// cleanup
	cleanup_hand_tracking();
}

const OpenXRHandTrackingExtension::HandTracker* OpenXRHandTrackingExtension::get_hand_tracker(
	HandTrackedHands p_hand) const
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_hand, OPENXR_MAX_TRACKED_HANDS, nullptr);

	return &hand_trackers[p_hand];
}


