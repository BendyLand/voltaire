/**************************************************************************/
/*  openxr_spatial_plane_tracking.h                                       */
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

#include "../openxr_future_extension.h"
#include "openxr_spatial_entities.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif

// Plane tracker (folded into OpenXRSpatialEntityTracker)
using OpenXRPlaneTracker = OpenXRSpatialEntityTracker;

// Plane tracking logic
class OpenXRSpatialPlaneTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_supported();

private:
	// Discovery logic
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);

	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_plane_tracking_ext = false;
		bool spatial_plane_tracking_supported = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> plane_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRPlaneTracker>>> plane_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialPlaneTrackingCapability() = delete;
	OpenXRSpatialPlaneTrackingCapability(const OpenXRSpatialPlaneTrackingCapability&) = delete;
	OpenXRSpatialPlaneTrackingCapability& operator=(const OpenXRSpatialPlaneTrackingCapability&) = delete;
	~OpenXRSpatialPlaneTrackingCapability() = delete;
};
