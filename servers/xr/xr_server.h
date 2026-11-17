/**************************************************************************/
/*  xr_server.h                                                           */
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

#include "core/os/thread_safe.h"
#include "core/types.h"

class XRInterface;
class XRTracker;
class XRPositionalTracker;

class XRServer final
{
	static inline Mutex _thread_safe_;

public:
	enum XRMode
	{
		XRMODE_DEFAULT, /* Default behavior, means we check project settings */
		XRMODE_OFF,		/* Ignore project settings, disable OpenXR, disable shaders */
		XRMODE_ON, /* Ignore project settings, enable OpenXR, enable shaders, run editor in VR (if
					  applicable) */
	};

	enum TrackerType
	{
		TRACKER_HEAD = 0x01, /* tracks the position of the players head (or in case of handheld AR,
								location of the phone) */
		TRACKER_CONTROLLER = 0x02,	/* tracks a controller */
		TRACKER_BASESTATION = 0x04, /* tracks location of a base station */
		TRACKER_ANCHOR =
			0x08,			 /* tracks an anchor point, used in AR to track a real live location */
		TRACKER_HAND = 0x10, /* tracks a hand */
		TRACKER_BODY = 0x20, /* tracks a body */
		TRACKER_FACE = 0x40, /* tracks a face */
		TRACKER_UNKNOWN = 0x80, /* unknown tracker */

		TRACKER_ANY_KNOWN = 0x7f, /* all except unknown */
		TRACKER_ANY = 0xff		  /* used by get_connected_trackers to return all types */
	};

	enum RotationMode
	{
		RESET_FULL_ROTATION = 0, /* we reset the full rotation, regardless of how the HMD is
									oriented, we're looking dead ahead */
		RESET_BUT_KEEP_TILT = 1, /* reset rotation but keep tilt. */
		DONT_RESET_ROTATION = 2, /* don't reset the rotation, we will only center on position */
	};

private:
	static XRMode xr_mode;

	// As we may be updating our main state for our next frame while we're still rendering our
	// previous frame, we need to keep copies around.
	struct RenderState
	{
		double world_scale = 1.0; /* scale by which we multiply our tracker positions */
		Transform3D world_origin; /* our world origin point, maps a location in our virtual world to
									 the origin point in our real world tracking volume */
		Transform3D reference_frame; /* our reference frame */
	};

	struct Data
	{
		double world_scale = 1.0; /* scale by which we multiply our tracker positions */
		bool camera_locked_to_origin = false;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void _set_render_world_scale(double p_world_scale);
	static void _set_render_world_origin(const Transform3D& p_world_origin);
	static void _set_render_reference_frame(const Transform3D& p_reference_frame);

	static void set_render_world_scale(double p_world_scale);
	static void set_render_world_origin(const Transform3D& p_world_origin);
	static void set_render_reference_frame(const Transform3D& p_reference_frame);

#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
	static void _add_tracker_bind_compat_90645(const Ref<XRPositionalTracker>& p_tracker);
	static void _remove_tracker_bind_compat_90645(const Ref<XRPositionalTracker>& p_tracker);
	static Ref<XRPositionalTracker> _get_tracker_bind_compat_90645(const String& p_name);
#endif

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static XRMode get_xr_mode();
	static void set_xr_mode(XRMode p_mode);

	static double get_world_scale();
	static void set_world_scale(double p_world_scale);

	static Transform3D get_world_origin();
	static void set_world_origin(const Transform3D& p_world_origin);

	static Transform3D get_reference_frame();
	static void clear_reference_frame();
	static void center_on_hmd(RotationMode p_rotation_mode, bool p_keep_height);

	static Transform3D get_hmd_transform();

	static void set_camera_locked_to_origin(bool p_enable);

	static inline bool is_camera_locked_to_origin()
	{
		return data ? data->camera_locked_to_origin : false;
	}

	static void add_interface(const Ref<XRInterface>& p_interface);
	static void remove_interface(const Ref<XRInterface>& p_interface);
	static int get_interface_count();
	static Ref<XRInterface> get_interface(int p_index);
	static Ref<XRInterface> find_interface(const String& p_name);

	static Ref<XRInterface> get_primary_interface();
	static void set_primary_interface(const Ref<XRInterface>& p_primary_interface);

	static void add_tracker(const Ref<XRTracker>& p_tracker);
	static void remove_tracker(const Ref<XRTracker>& p_tracker);
	static Ref<XRTracker> get_tracker(const String& p_name);

	static PackedStringArray get_suggested_tracker_names();
	static PackedStringArray get_suggested_pose_names(const String& p_tracker_name);

	static void _process();
	static void pre_render();
	static void end_frame();

	XRServer() = delete;
	XRServer(const XRServer&) = delete;
	XRServer& operator=(const XRServer&) = delete;
	~XRServer() = delete;
};

#define XR XRServer
