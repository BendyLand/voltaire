/**************************************************************************/
/*  openxr_action_map.cpp                                                 */
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

#include "openxr_action_map.h"
#include "openxr_interaction_profile_metadata.h"

void OpenXRActionMap::create_default_action_sets()
{
	// Note:
	// - if you make changes here make sure to delete your default_action_map.tres file of it will
	// load an old version.
	// - our palm pose is only available if the relevant extension is supported,
	//   we still want it to be part of our action map as we may deploy the same game to platforms
	//   that do and don't support it.
	// - the same applies for interaction profiles that are only supported if the relevant extension
	// is supported.

	// Create our Godot action set.
	Ref<OpenXRActionSet> action_set = OpenXRActionSet::new_action_set("godot", "Godot action set");
	add_action_set(action_set);

	// Create our actions.
	Ref<OpenXRAction> trigger = action_set->add_new_action("trigger", "Trigger",
		OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_click = action_set->add_new_action("trigger_click", "Trigger click",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_touch = action_set->add_new_action("trigger_touch",
		"Trigger touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip = action_set->add_new_action(
		"grip", "Grip", OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_click = action_set->add_new_action("grip_click", "Grip click",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_force = action_set->add_new_action("grip_force", "Grip force",
		OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary =
		action_set->add_new_action("primary", "Primary joystick/thumbstick/trackpad",
			OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_click =
		action_set->add_new_action("primary_click", "Primary joystick/thumbstick/trackpad click",
			OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_touch =
		action_set->add_new_action("primary_touch", "Primary joystick/thumbstick/trackpad touching",
			OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary =
		action_set->add_new_action("secondary", "Secondary joystick/thumbstick/trackpad",
			OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_click = action_set->add_new_action("secondary_click",
		"Secondary joystick/thumbstick/trackpad click", OpenXRAction::OPENXR_ACTION_BOOL,
		"/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_touch = action_set->add_new_action("secondary_touch",
		"Secondary joystick/thumbstick/trackpad touching", OpenXRAction::OPENXR_ACTION_BOOL,
		"/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> menu_button = action_set->add_new_action("menu_button", "Menu button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> select_button = action_set->add_new_action("select_button", "Select button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_button = action_set->add_new_action("ax_button", "A/X button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_touch = action_set->add_new_action("ax_touch", "A/X touching",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_button = action_set->add_new_action("by_button", "B/Y button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_touch = action_set->add_new_action("by_touch", "B/Y touching",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> default_pose =
		action_set->add_new_action("default_pose", "Default pose", OpenXRAction::OPENXR_ACTION_POSE,
			"/user/hand/left,"
			"/user/hand/right,"
			// "/user/vive_tracker_htcx/role/handheld_object," <-- getting errors on this one.
			"/user/vive_tracker_htcx/role/left_foot,"
			"/user/vive_tracker_htcx/role/right_foot,"
			"/user/vive_tracker_htcx/role/left_shoulder,"
			"/user/vive_tracker_htcx/role/right_shoulder,"
			"/user/vive_tracker_htcx/role/left_elbow,"
			"/user/vive_tracker_htcx/role/right_elbow,"
			"/user/vive_tracker_htcx/role/left_knee,"
			"/user/vive_tracker_htcx/role/right_knee,"
			"/user/vive_tracker_htcx/role/waist,"
			"/user/vive_tracker_htcx/role/chest,"
			"/user/vive_tracker_htcx/role/camera,"
			"/user/vive_tracker_htcx/role/keyboard,"
			"/user/vive_tracker_htcx/role/left_wrist,"
			"/user/vive_tracker_htcx/role/right_wrist,"
			"/user/vive_tracker_htcx/role/left_ankle,"
			"/user/vive_tracker_htcx/role/right_ankle,"
			"/user/eyes_ext");
	Ref<OpenXRAction> aim_pose = action_set->add_new_action("aim_pose", "Aim pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_pose = action_set->add_new_action("grip_pose", "Grip pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> palm_pose = action_set->add_new_action("palm_pose", "Palm pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> haptic =
		action_set->add_new_action("haptic", "Haptic", OpenXRAction::OPENXR_ACTION_HAPTIC,
			"/user/hand/left,"
			"/user/hand/right,"
			// "/user/vive_tracker_htcx/role/handheld_object," <-- getting errors on this one.
			"/user/vive_tracker_htcx/role/left_foot,"
			"/user/vive_tracker_htcx/role/right_foot,"
			"/user/vive_tracker_htcx/role/left_shoulder,"
			"/user/vive_tracker_htcx/role/right_shoulder,"
			"/user/vive_tracker_htcx/role/left_elbow,"
			"/user/vive_tracker_htcx/role/right_elbow,"
			"/user/vive_tracker_htcx/role/left_knee,"
			"/user/vive_tracker_htcx/role/right_knee,"
			"/user/vive_tracker_htcx/role/waist,"
			"/user/vive_tracker_htcx/role/chest,"
			"/user/vive_tracker_htcx/role/camera,"
			"/user/vive_tracker_htcx/role/keyboard,"
			"/user/vive_tracker_htcx/role/left_wrist,"
			"/user/vive_tracker_htcx/role/right_wrist,"
			"/user/vive_tracker_htcx/role/left_ankle,"
			"/user/vive_tracker_htcx/role/right_ankle");

	// Create our interaction profiles.
	Ref<OpenXRInteractionProfile> profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/khr/generic_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	// Generic controller has no menu or select buttons we can use.
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/primary/click,/user/hand/right/input/primary/click");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/secondary/click,/user/hand/right/input/secondary/click");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value"); // OpenXR will
																					 // convert
																					 // float to
																					 // bool.
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(grip_click,
		"/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value"); // OpenXR will
																					 // convert
																					 // float to
																					 // bool.
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Meta touch controller profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/oculus/touch_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	// touch controllers have no select button we can use.
	profile->add_new_binding(menu_button,
		"/user/hand/left/input/menu/click,/user/hand/right/input/system/click"); // right hand
																				 // system click may
																				 // not be
																				 // available.
	profile->add_new_binding(ax_button,
		"/user/hand/left/input/x/click,/user/hand/right/input/a/click"); // x on left hand, a on
																		 // right hand.
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(by_button,
		"/user/hand/left/input/y/click,/user/hand/right/input/b/click"); // y on left hand, b on
																		 // right hand.
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value"); // should be
																					 // converted to
																					 // boolean.
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(grip,
		"/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value"); // should be
																					 // converted to
																					 // boolean.
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	// primary on our touch controller is our thumbstick.
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	// touch controller has no secondary input.
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Pico 4 controller profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/bytedance/pico4_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(select_button,
		"/user/hand/left/input/system/click,/user/hand/right/input/system/click"); // system click
																				   // may not be
																				   // available.
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click");
	profile->add_new_binding(ax_button,
		"/user/hand/left/input/x/click,/user/hand/right/input/a/click"); // x on left hand, a on
																		 // right hand.
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(by_button,
		"/user/hand/left/input/y/click,/user/hand/right/input/b/click"); // y on left hand, b on
																		 // right hand.
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value"); // should be
																					 // converted to
																					 // boolean.
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(grip,
		"/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value"); // should be
																					 // converted to
																					 // boolean.
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	// primary on our pico controller is our thumbstick.
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	// pico controller has no secondary input.
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our hand interaction profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/ext/hand_interaction_ext");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	// Use pinch as trigger.
	profile->add_new_binding(
		trigger, "/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	// Use grasp as grip.
	profile->add_new_binding(
		grip, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	add_interaction_profile(profile);
}

void OpenXRActionMap::create_editor_action_sets()
{
	// TODO implement
}

Ref<OpenXRAction> OpenXRActionMap::get_action(const String& p_path) const
{
	PackedStringArray paths = p_path.split("/", false);
	ERR_FAIL_COND_V(paths.size() != 2, Ref<OpenXRAction>());

	Ref<OpenXRActionSet> action_set = find_action_set(paths[0]);
	if (action_set.is_valid()) {
		return action_set->get_action(paths[1]);
	}

	return Ref<OpenXRAction>();
}


