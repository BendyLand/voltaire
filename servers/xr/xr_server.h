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
	static void _add_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker);
	static void _remove_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker);
	static Ref<XRTracker> _get_tracker_bind_compat_90645(const String& p_name);
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

// --- Folded from servers/xr/xr_tracker.h ---
#include "core/os/thread_safe.h"
#include "core/types.h"

class XRPose final : public RefCounted
{
public:
	// TrackingConfidence gives an indication of how reliable our transform data is.
	enum TrackingConfidence
	{
		XR_TRACKING_CONFIDENCE_NONE, // No tracking information is available for this pose.
		XR_TRACKING_CONFIDENCE_LOW,	 // Tracking information may be inaccurate or estimated.
		XR_TRACKING_CONFIDENCE_HIGH	 // Tracking information is deemed accurate and up to date.
	};

private:
	bool has_tracking_data = false;
	String name;
	Transform3D transform;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	TrackingConfidence tracking_confidence = XR_TRACKING_CONFIDENCE_NONE;

public:
	void set_has_tracking_data(const bool p_has_tracking_data);
	bool get_has_tracking_data() const;

	void set_name(const String& p_name);
	String get_name() const;

	void set_transform(const Transform3D p_transform);
	Transform3D get_transform() const;
	Transform3D get_adjusted_transform() const;

	void set_linear_velocity(const Vector3 p_velocity);
	Vector3 get_linear_velocity() const;

	void set_angular_velocity(const Vector3 p_velocity);
	Vector3 get_angular_velocity() const;

	void set_tracking_confidence(const TrackingConfidence p_tracking_confidence);
	TrackingConfidence get_tracking_confidence() const;
};

/**
	Unified XR tracker representing positional, controller, body, hand, and face trackers,
	discriminated by XRServer::TrackerType.
*/

class XRTracker final : public RefCounted
{
	_THREAD_SAFE_CLASS_

public:
	enum TrackerHand
	{
		TRACKER_HAND_UNKNOWN, /* unknown or not applicable */
		TRACKER_HAND_LEFT,	  /* controller/hand is the left hand */
		TRACKER_HAND_RIGHT,	  /* controller/hand is the right hand */
		TRACKER_HAND_MAX
	};

	enum BodyFlags
	{
		BODY_FLAG_UPPER_BODY_SUPPORTED = 1,
		BODY_FLAG_LOWER_BODY_SUPPORTED = 2,
		BODY_FLAG_HANDS_SUPPORTED = 4,
	};

	enum Joint
	{
		JOINT_ROOT,

		// Upper Body Joints
		JOINT_HIPS,
		JOINT_SPINE,
		JOINT_CHEST,
		JOINT_UPPER_CHEST,
		JOINT_NECK,
		JOINT_HEAD,
		JOINT_HEAD_TIP,
		JOINT_LEFT_SHOULDER,
		JOINT_LEFT_UPPER_ARM,
		JOINT_LEFT_LOWER_ARM,
		JOINT_RIGHT_SHOULDER,
		JOINT_RIGHT_UPPER_ARM,
		JOINT_RIGHT_LOWER_ARM,

		// Lower Body Joints
		JOINT_LEFT_UPPER_LEG,
		JOINT_LEFT_LOWER_LEG,
		JOINT_LEFT_FOOT,
		JOINT_LEFT_TOES,
		JOINT_RIGHT_UPPER_LEG,
		JOINT_RIGHT_LOWER_LEG,
		JOINT_RIGHT_FOOT,
		JOINT_RIGHT_TOES,

		// Left Hand Joints
		JOINT_LEFT_HAND,
		JOINT_LEFT_PALM,
		JOINT_LEFT_WRIST,
		JOINT_LEFT_THUMB_METACARPAL,
		JOINT_LEFT_THUMB_PHALANX_PROXIMAL,
		JOINT_LEFT_THUMB_PHALANX_DISTAL,
		JOINT_LEFT_THUMB_TIP,
		JOINT_LEFT_INDEX_FINGER_METACARPAL,
		JOINT_LEFT_INDEX_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_INDEX_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_INDEX_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_INDEX_FINGER_TIP,
		JOINT_LEFT_MIDDLE_FINGER_METACARPAL,
		JOINT_LEFT_MIDDLE_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_MIDDLE_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_MIDDLE_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_MIDDLE_FINGER_TIP,
		JOINT_LEFT_RING_FINGER_METACARPAL,
		JOINT_LEFT_RING_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_RING_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_RING_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_RING_FINGER_TIP,
		JOINT_LEFT_PINKY_FINGER_METACARPAL,
		JOINT_LEFT_PINKY_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_PINKY_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_PINKY_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_PINKY_FINGER_TIP,

		// Right Hand Joints
		JOINT_RIGHT_HAND,
		JOINT_RIGHT_PALM,
		JOINT_RIGHT_WRIST,
		JOINT_RIGHT_THUMB_METACARPAL,
		JOINT_RIGHT_THUMB_PHALANX_PROXIMAL,
		JOINT_RIGHT_THUMB_PHALANX_DISTAL,
		JOINT_RIGHT_THUMB_TIP,
		JOINT_RIGHT_INDEX_FINGER_METACARPAL,
		JOINT_RIGHT_INDEX_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_INDEX_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_INDEX_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_INDEX_FINGER_TIP,
		JOINT_RIGHT_MIDDLE_FINGER_METACARPAL,
		JOINT_RIGHT_MIDDLE_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_MIDDLE_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_MIDDLE_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_MIDDLE_FINGER_TIP,
		JOINT_RIGHT_RING_FINGER_METACARPAL,
		JOINT_RIGHT_RING_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_RING_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_RING_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_RING_FINGER_TIP,
		JOINT_RIGHT_PINKY_FINGER_METACARPAL,
		JOINT_RIGHT_PINKY_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_PINKY_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_PINKY_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_PINKY_FINGER_TIP,

		// Extra joints that aren't part of the Godot humanoid skeleton, but are commonly used in
		// some VR avatars.
		JOINT_LOWER_CHEST,
		JOINT_LEFT_SCAPULA,
		JOINT_LEFT_WRIST_TWIST,
		JOINT_RIGHT_SCAPULA,
		JOINT_RIGHT_WRIST_TWIST,
		JOINT_LEFT_FOOT_TWIST,
		JOINT_LEFT_HEEL,
		JOINT_LEFT_MIDDLE_FOOT,
		JOINT_RIGHT_FOOT_TWIST,
		JOINT_RIGHT_HEEL,
		JOINT_RIGHT_MIDDLE_FOOT,

		JOINT_MAX,
	};

	enum JointFlags
	{
		JOINT_FLAG_ORIENTATION_VALID = 1,
		JOINT_FLAG_ORIENTATION_TRACKED = 2,
		JOINT_FLAG_POSITION_VALID = 4,
		JOINT_FLAG_POSITION_TRACKED = 8,
	};

	enum HandTrackingSource
	{
		HAND_TRACKING_SOURCE_UNKNOWN,
		HAND_TRACKING_SOURCE_UNOBSTRUCTED,
		HAND_TRACKING_SOURCE_CONTROLLER,
		HAND_TRACKING_SOURCE_NOT_TRACKED,
		HAND_TRACKING_SOURCE_MAX
	};

	enum HandJoint
	{
		HAND_JOINT_PALM,
		HAND_JOINT_WRIST,
		HAND_JOINT_THUMB_METACARPAL,
		HAND_JOINT_THUMB_PHALANX_PROXIMAL,
		HAND_JOINT_THUMB_PHALANX_DISTAL,
		HAND_JOINT_THUMB_TIP,
		HAND_JOINT_INDEX_FINGER_METACARPAL,
		HAND_JOINT_INDEX_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_INDEX_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_INDEX_FINGER_PHALANX_DISTAL,
		HAND_JOINT_INDEX_FINGER_TIP,
		HAND_JOINT_MIDDLE_FINGER_METACARPAL,
		HAND_JOINT_MIDDLE_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_MIDDLE_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_MIDDLE_FINGER_PHALANX_DISTAL,
		HAND_JOINT_MIDDLE_FINGER_TIP,
		HAND_JOINT_RING_FINGER_METACARPAL,
		HAND_JOINT_RING_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_RING_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_RING_FINGER_PHALANX_DISTAL,
		HAND_JOINT_RING_FINGER_TIP,
		HAND_JOINT_PINKY_FINGER_METACARPAL,
		HAND_JOINT_PINKY_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_PINKY_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_PINKY_FINGER_PHALANX_DISTAL,
		HAND_JOINT_PINKY_FINGER_TIP,
		HAND_JOINT_MAX,
	};

	enum HandJointFlags
	{
		HAND_JOINT_FLAG_ORIENTATION_VALID = 1,
		HAND_JOINT_FLAG_ORIENTATION_TRACKED = 2,
		HAND_JOINT_FLAG_POSITION_VALID = 4,
		HAND_JOINT_FLAG_POSITION_TRACKED = 8,
		HAND_JOINT_FLAG_LINEAR_VELOCITY_VALID = 16,
		HAND_JOINT_FLAG_ANGULAR_VELOCITY_VALID = 32,
	};

	enum BlendShapeEntry
	{
		// Base Shapes
		FT_EYE_LOOK_OUT_RIGHT,		 // Right eye looks outwards.
		FT_EYE_LOOK_IN_RIGHT,		 // Right eye looks inwards.
		FT_EYE_LOOK_UP_RIGHT,		 // Right eye looks upwards.
		FT_EYE_LOOK_DOWN_RIGHT,		 // Right eye looks downwards.
		FT_EYE_LOOK_OUT_LEFT,		 // Left eye looks outwards.
		FT_EYE_LOOK_IN_LEFT,		 // Left eye looks inwards.
		FT_EYE_LOOK_UP_LEFT,		 // Left eye looks upwards.
		FT_EYE_LOOK_DOWN_LEFT,		 // Left eye looks downwards.
		FT_EYE_CLOSED_RIGHT,		 // Closes the right eyelid.
		FT_EYE_CLOSED_LEFT,			 // Closes the left eyelid.
		FT_EYE_SQUINT_RIGHT,		 // Squeezes the right eye socket muscles.
		FT_EYE_SQUINT_LEFT,			 // Squeezes the left eye socket muscles.
		FT_EYE_WIDE_RIGHT,			 // Right eyelid widens beyond relaxed.
		FT_EYE_WIDE_LEFT,			 // Left eyelid widens beyond relaxed.
		FT_EYE_DILATION_RIGHT,		 // Dilates the right eye pupil.
		FT_EYE_DILATION_LEFT,		 // Dilates the left eye pupil.
		FT_EYE_CONSTRICT_RIGHT,		 // Constricts the right eye pupil.
		FT_EYE_CONSTRICT_LEFT,		 // Constricts the left eye pupil.
		FT_BROW_PINCH_RIGHT,		 // Right eyebrow pinches in.
		FT_BROW_PINCH_LEFT,			 // Left eyebrow pinches in.
		FT_BROW_LOWERER_RIGHT,		 // Outer right eyebrow pulls down.
		FT_BROW_LOWERER_LEFT,		 // Outer left eyebrow pulls down.
		FT_BROW_INNER_UP_RIGHT,		 // Inner right eyebrow pulls up.
		FT_BROW_INNER_UP_LEFT,		 // Inner left eyebrow pulls up.
		FT_BROW_OUTER_UP_RIGHT,		 // Outer right eyebrow pulls up.
		FT_BROW_OUTER_UP_LEFT,		 // Outer left eyebrow pulls up.
		FT_NOSE_SNEER_RIGHT,		 // Right side face sneers.
		FT_NOSE_SNEER_LEFT,			 // Left side face sneers.
		FT_NASAL_DILATION_RIGHT,	 // Right side nose canal dilates.
		FT_NASAL_DILATION_LEFT,		 // Left side nose canal dilates.
		FT_NASAL_CONSTRICT_RIGHT,	 // Right side nose canal constricts.
		FT_NASAL_CONSTRICT_LEFT,	 // Left side nose canal constricts.
		FT_CHEEK_SQUINT_RIGHT,		 // Raises the right side cheek.
		FT_CHEEK_SQUINT_LEFT,		 // Raises the left side cheek.
		FT_CHEEK_PUFF_RIGHT,		 // Puffs the right side cheek.
		FT_CHEEK_PUFF_LEFT,			 // Puffs the left side cheek.
		FT_CHEEK_SUCK_RIGHT,		 // Sucks in the right side cheek.
		FT_CHEEK_SUCK_LEFT,			 // Sucks in the left side cheek.
		FT_JAW_OPEN,				 // Opens jawbone.
		FT_MOUTH_CLOSED,			 // Closes the mouth.
		FT_JAW_RIGHT,				 // Pushes jawbone right.
		FT_JAW_LEFT,				 // Pushes jawbone left.
		FT_JAW_FORWARD,				 // Pushes jawbone forward.
		FT_JAW_BACKWARD,			 // Pushes jawbone backward.
		FT_JAW_CLENCH,				 // Flexes jaw muscles.
		FT_JAW_MANDIBLE_RAISE,		 // Raises the jawbone.
		FT_LIP_SUCK_UPPER_RIGHT,	 // Upper right lip part tucks in the mouth.
		FT_LIP_SUCK_UPPER_LEFT,		 // Upper left lip part tucks in the mouth.
		FT_LIP_SUCK_LOWER_RIGHT,	 // Lower right lip part tucks in the mouth.
		FT_LIP_SUCK_LOWER_LEFT,		 // Lower left lip part tucks in the mouth.
		FT_LIP_SUCK_CORNER_RIGHT,	 // Right lip corner folds into the mouth.
		FT_LIP_SUCK_CORNER_LEFT,	 // Left lip corner folds into the mouth.
		FT_LIP_FUNNEL_UPPER_RIGHT,	 // Upper right lip part pushes into a funnel.
		FT_LIP_FUNNEL_UPPER_LEFT,	 // Upper left lip part pushes into a funnel.
		FT_LIP_FUNNEL_LOWER_RIGHT,	 // Lower right lip part pushes into a funnel.
		FT_LIP_FUNNEL_LOWER_LEFT,	 // Lower left lip part pushes into a funnel.
		FT_LIP_PUCKER_UPPER_RIGHT,	 // Upper right lip part pushes outwards.
		FT_LIP_PUCKER_UPPER_LEFT,	 // Upper left lip part pushes outwards.
		FT_LIP_PUCKER_LOWER_RIGHT,	 // Lower right lip part pushes outwards.
		FT_LIP_PUCKER_LOWER_LEFT,	 // Lower left lip part pushes outwards.
		FT_MOUTH_UPPER_UP_RIGHT,	 // Upper right part of the lip pulls up.
		FT_MOUTH_UPPER_UP_LEFT,		 // Upper left part of the lip pulls up.
		FT_MOUTH_LOWER_DOWN_RIGHT,	 // Lower right part of the lip pulls up.
		FT_MOUTH_LOWER_DOWN_LEFT,	 // Lower left part of the lip pulls up.
		FT_MOUTH_UPPER_DEEPEN_RIGHT, // Upper right lip part pushes in the cheek.
		FT_MOUTH_UPPER_DEEPEN_LEFT,	 // Upper left lip part pushes in the cheek.
		FT_MOUTH_UPPER_RIGHT,		 // Moves upper lip right.
		FT_MOUTH_UPPER_LEFT,		 // Moves upper lip left.
		FT_MOUTH_LOWER_RIGHT,		 // Moves lower lip right.
		FT_MOUTH_LOWER_LEFT,		 // Moves lower lip left.
		FT_MOUTH_CORNER_PULL_RIGHT,	 // Right lip corner pulls diagonally up and out.
		FT_MOUTH_CORNER_PULL_LEFT,	 // Left lip corner pulls diagonally up and out.
		FT_MOUTH_CORNER_SLANT_RIGHT, // Right corner lip slants up.
		FT_MOUTH_CORNER_SLANT_LEFT,	 // Left corner lip slants up.
		FT_MOUTH_FROWN_RIGHT,		 // Right corner lip pulls down.
		FT_MOUTH_FROWN_LEFT,		 // Left corner lip pulls down.
		FT_MOUTH_STRETCH_RIGHT,		 // Mouth corner lip pulls out and down.
		FT_MOUTH_STRETCH_LEFT,		 // Mouth corner lip pulls out and down.
		FT_MOUTH_DIMPLE_RIGHT,		 // Right lip corner is pushed backwards.
		FT_MOUTH_DIMPLE_LEFT,		 // Left lip corner is pushed backwards.
		FT_MOUTH_RAISER_UPPER,		 // Raises and slightly pushes out the upper mouth.
		FT_MOUTH_RAISER_LOWER,		 // Raises and slightly pushes out the lower mouth.
		FT_MOUTH_PRESS_RIGHT,		 // Right side lips press and flatten together vertically.
		FT_MOUTH_PRESS_LEFT,		 // Left side lips press and flatten together vertically.
		FT_MOUTH_TIGHTENER_RIGHT,	 // Right side lips squeeze together horizontally.
		FT_MOUTH_TIGHTENER_LEFT,	 // Left side lips squeeze together horizontally.
		FT_TONGUE_OUT,				 // Tongue visibly sticks out of the mouth.
		FT_TONGUE_UP,				 // Tongue points upwards.
		FT_TONGUE_DOWN,				 // Tongue points downwards.
		FT_TONGUE_RIGHT,			 // Tongue points right.
		FT_TONGUE_LEFT,				 // Tongue points left.
		FT_TONGUE_ROLL,				 // Sides of the tongue funnel, creating a roll.
		FT_TONGUE_BLEND_DOWN,		 // Tongue arches up then down inside the mouth.
		FT_TONGUE_CURL_UP,			 // Tongue arches down then up inside the mouth.
		FT_TONGUE_SQUISH,			 // Tongue squishes together and thickens.
		FT_TONGUE_FLAT,				 // Tongue flattens and thins out.
		FT_TONGUE_TWIST_RIGHT, // Tongue tip rotates clockwise, with the rest following gradually.
		FT_TONGUE_TWIST_LEFT,  // Tongue tip rotates counter-clockwise, with the rest following
							   // gradually.
		FT_SOFT_PALATE_CLOSE,  // Inner mouth throat closes.
		FT_THROAT_SWALLOW,	   // The Adam's apple visibly swallows.
		FT_NECK_FLEX_RIGHT,	   // Right side neck visibly flexes.
		FT_NECK_FLEX_LEFT,	   // Left side neck visibly flexes.
		// Blended Shapes
		FT_EYE_CLOSED,		  // Closes both eye lids.
		FT_EYE_WIDE,		  // Widens both eye lids.
		FT_EYE_SQUINT,		  // Squints both eye lids.
		FT_EYE_DILATION,	  // Dilates both pupils.
		FT_EYE_CONSTRICT,	  // Constricts both pupils.
		FT_BROW_DOWN_RIGHT,	  // Pulls the right eyebrow down and in.
		FT_BROW_DOWN_LEFT,	  // Pulls the left eyebrow down and in.
		FT_BROW_DOWN,		  // Pulls both eyebrows down and in.
		FT_BROW_UP_RIGHT,	  // Right brow appears worried.
		FT_BROW_UP_LEFT,	  // Left brow appears worried.
		FT_BROW_UP,			  // Both brows appear worried.
		FT_NOSE_SNEER,		  // Entire face sneers.
		FT_NASAL_DILATION,	  // Both nose canals dilate.
		FT_NASAL_CONSTRICT,	  // Both nose canals constrict.
		FT_CHEEK_PUFF,		  // Puffs both cheeks.
		FT_CHEEK_SUCK,		  // Sucks in both cheeks.
		FT_CHEEK_SQUINT,	  // Raises both cheeks.
		FT_LIP_SUCK_UPPER,	  // Tucks in the upper lips.
		FT_LIP_SUCK_LOWER,	  // Tucks in the lower lips.
		FT_LIP_SUCK,		  // Tucks in both lips.
		FT_LIP_FUNNEL_UPPER,  // Funnels in the upper lips.
		FT_LIP_FUNNEL_LOWER,  // Funnels in the lower lips.
		FT_LIP_FUNNEL,		  // Funnels in both lips.
		FT_LIP_PUCKER_UPPER,  // Upper lip part pushes outwards.
		FT_LIP_PUCKER_LOWER,  // Lower lip part pushes outwards.
		FT_LIP_PUCKER,		  // Lips push outwards.
		FT_MOUTH_UPPER_UP,	  // Raises the upper lips.
		FT_MOUTH_LOWER_DOWN,  // Lowers the lower lips.
		FT_MOUTH_OPEN,		  // Mouth opens, revealing teeth.
		FT_MOUTH_RIGHT,		  // Moves mouth right.
		FT_MOUTH_LEFT,		  // Moves mouth left.
		FT_MOUTH_SMILE_RIGHT, // Right side of the mouth smiles.
		FT_MOUTH_SMILE_LEFT,  // Left side of the mouth smiles.
		FT_MOUTH_SMILE,		  // Mouth expresses a smile.
		FT_MOUTH_SAD_RIGHT,	  // Right side of the mouth expresses sadness.
		FT_MOUTH_SAD_LEFT,	  // Left side of the mouth expresses sadness.
		FT_MOUTH_SAD,		  // Mouth expresses sadness.
		FT_MOUTH_STRETCH,	  // Mouth stretches.
		FT_MOUTH_DIMPLE,	  // Lip corners dimple.
		FT_MOUTH_TIGHTENER,	  // Mouth tightens.
		FT_MOUTH_PRESS,		  // Mouth presses together.
		FT_MAX				  // Maximum blend shape.
	};

	struct InputValue
	{
		enum Type
		{
			INPUT_NONE,
			INPUT_BOOL,
			INPUT_FLOAT,
			INPUT_VECTOR2,
		};
		Type type = INPUT_NONE;
		union
		{
			bool bool_val;
			float float_val;
			struct
			{
				real_t x;
				real_t y;
			} vec2_val;
		};

		InputValue() : type(INPUT_NONE), vec2_val{0, 0} {}
	};

protected:
	XRServer::TrackerType type = XRServer::TRACKER_UNKNOWN;
	String name = "Unknown";
	String description;
	String profile;
	TrackerHand tracker_hand = TRACKER_HAND_UNKNOWN;
	bool has_tracking_data = false;

	HashMap<String, Ref<XRPose>> poses;
	HashMap<String, InputValue> inputs;

private:
	struct BodyData
	{
		uint32_t body_flags = 0;
		uint32_t joint_flags[JOINT_MAX] = {};
		Transform3D joint_transforms[JOINT_MAX];
	};

	struct HandData
	{
		HandTrackingSource hand_tracking_source = HAND_TRACKING_SOURCE_UNKNOWN;
		uint32_t hand_joint_flags[HAND_JOINT_MAX] = {};
		Transform3D hand_joint_transforms[HAND_JOINT_MAX];
		float hand_joint_radii[HAND_JOINT_MAX] = {};
		Vector3 hand_joint_linear_velocities[HAND_JOINT_MAX];
		Vector3 hand_joint_angular_velocities[HAND_JOINT_MAX];
	};

	BodyData* body_data = nullptr;
	HandData* hand_data = nullptr;
	PackedFloat32Array blend_shapes;
	Ref<RefCounted> custom_data;

	BodyData* _ensure_body_data();
	HandData* _ensure_hand_data();

public:
	void set_custom_data(const Ref<RefCounted>& p_custom_data) { custom_data = p_custom_data; }
	Ref<RefCounted> get_custom_data() const { return custom_data; }
	void set_tracker_type(XRServer::TrackerType p_type);
	XRServer::TrackerType get_tracker_type() const;
	void set_tracker_name(const String& p_name);
	String get_tracker_name() const;
	void set_tracker_desc(const String& p_desc);
	String get_tracker_desc() const;

	void set_tracker_profile(const String& p_profile);
	String get_tracker_profile() const;

	TrackerHand get_tracker_hand() const;
	void set_tracker_hand(const TrackerHand p_hand);

	void set_has_tracking_data(bool p_has_tracking_data);
	bool get_has_tracking_data() const;

	// Positional poses
	bool has_pose(const String& p_action_name) const;
	Ref<XRPose> get_pose(const String& p_action_name) const;
	void invalidate_pose(const String& p_action_name);
	void set_pose(const String& p_action_name, const Transform3D& p_transform,
		const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
		const XRPose::TrackingConfidence p_tracking_confidence =
			XRPose::XR_TRACKING_CONFIDENCE_HIGH);

	// Controller inputs
	void set_input(const String& p_action_name, bool p_value);
	void set_input(const String& p_action_name, float p_value);
	void set_input(const String& p_action_name, const Vector2& p_value);
	InputValue get_input(const String& p_action_name) const;
	bool is_button_pressed(const String& p_action_name) const;
	float get_input_float(const String& p_action_name) const;
	Vector2 get_input_vector2(const String& p_action_name) const;

	// Body tracking
	void set_body_flags(uint32_t p_body_flags);
	uint32_t get_body_flags() const;

	void set_joint_flags(Joint p_joint, uint32_t p_flags);
	uint32_t get_joint_flags(Joint p_joint) const;

	void set_joint_transform(Joint p_joint, const Transform3D& p_transform);
	Transform3D get_joint_transform(Joint p_joint) const;

	// Hand tracking
	void set_hand_tracking_source(HandTrackingSource p_source);
	HandTrackingSource get_hand_tracking_source() const;

	void set_hand_joint_flags(HandJoint p_joint, uint32_t p_flags);
	uint32_t get_hand_joint_flags(HandJoint p_joint) const;

	void set_hand_joint_transform(HandJoint p_joint, const Transform3D& p_transform);
	Transform3D get_hand_joint_transform(HandJoint p_joint) const;

	void set_hand_joint_radius(HandJoint p_joint, float p_radius);
	float get_hand_joint_radius(HandJoint p_joint) const;

	void set_hand_joint_linear_velocity(HandJoint p_joint, const Vector3& p_velocity);
	Vector3 get_hand_joint_linear_velocity(HandJoint p_joint) const;

	void set_hand_joint_angular_velocity(HandJoint p_joint, const Vector3& p_velocity);
	Vector3 get_hand_joint_angular_velocity(HandJoint p_joint) const;

	// Face tracking
	float get_blend_shape(BlendShapeEntry p_blend_shape) const;
	void set_blend_shape(BlendShapeEntry p_blend_shape, float p_value);

	PackedFloat32Array get_blend_shapes() const;
	void set_blend_shapes(const PackedFloat32Array& p_blend_shapes);

	XRTracker(XRServer::TrackerType p_type = XRServer::TRACKER_UNKNOWN);
	~XRTracker() override;
};
