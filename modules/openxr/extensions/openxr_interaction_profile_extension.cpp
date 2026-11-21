/**************************************************************************/
/*  openxr_interaction_profile_extension.cpp                              */
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
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"
#include "openxr_interaction_profile_extension.h"

#ifndef XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME
#define XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME "XR_LOGITECH_mx_ink_stylus_interaction"
#endif

#ifndef XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME
#define XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME "XR_VALVE_frame_controller_interaction"
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileExtension

OpenXRInteractionProfileExtension::OpenXRInteractionProfileExtension(ProfileKind p_kind)
	: kind(p_kind)
{
}

void OpenXRInteractionProfileExtension::initialize()
{
	if (data) {
		return;
	}
	data = memnew_arr(OpenXRInteractionProfileExtension, PROFILE_MAX);
	for (int i = 0; i < PROFILE_MAX; i++) {
		data[i] = OpenXRInteractionProfileExtension(static_cast<ProfileKind>(i));
	}
}

void OpenXRInteractionProfileExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete_arr(data);
	data = nullptr;
}

OpenXRInteractionProfileExtension* OpenXRInteractionProfileExtension::get_profile(
	ProfileKind p_kind)
{
	ERR_FAIL_INDEX_V(p_kind, PROFILE_MAX, nullptr);
	return data ? &data[p_kind] : nullptr;
}

bool OpenXRInteractionProfileExtension::is_profile_available(ProfileKind p_kind, int p_subindex)
{
	ERR_FAIL_INDEX_V(p_kind, PROFILE_MAX, false);
	return data != nullptr && data[p_kind].is_available(p_subindex);
}

HashMap<String, bool*> OpenXRInteractionProfileExtension::get_all_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> result;
	if (!data) {
		return result;
	}
	for (int i = 0; i < PROFILE_MAX; i++) {
		HashMap<String, bool*> ext = data[i].get_requested_extensions(p_version);
		for (const KeyValue<String, bool*>& kv : ext) {
			result[kv.key] = kv.value;
		}
	}
	return result;
}

HashMap<String, bool*> OpenXRInteractionProfileExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	switch (kind) {
	case PROFILE_HTC_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME] =
				&flags[HTC_VIVE_COSMOS];
			request_extensions[XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME] =
				&flags[HTC_VIVE_FOCUS3];
		}
		request_extensions[XR_HTC_HAND_INTERACTION_EXTENSION_NAME] = &flags[HTC_HAND_INTERACTION];
	} break;
	case PROFILE_HTC_VIVE_TRACKER: {
		request_extensions[XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_HUAWEI_CONTROLLER: {
		request_extensions[XR_HUAWEI_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_KHR_GENERIC_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_KHR_GENERIC_CONTROLLER_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_META_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_FB_TOUCH_CONTROLLER_PRO_EXTENSION_NAME] = &flags[META_TOUCH_PRO];
			request_extensions[XR_META_TOUCH_CONTROLLER_PLUS_EXTENSION_NAME] =
				&flags[META_TOUCH_PLUS];
		}
		request_extensions[XR_FB_TOUCH_CONTROLLER_PROXIMITY_EXTENSION_NAME] =
			&flags[META_TOUCH_PROXIMITY];
	} break;
	case PROFILE_ML2_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_ML_ML2_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_MXINK: {
		request_extensions[XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_PALM_POSE: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_PALM_POSE_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_PICO_CONTROLLER: {
		request_extensions[XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_VALVE_CONTROLLER: {
		request_extensions[XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_WMR_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME] =
				&flags[WMR_HPMR];
			request_extensions[XR_EXT_SAMSUNG_ODYSSEY_CONTROLLER_EXTENSION_NAME] =
				&flags[WMR_SAMSUNG_ODESSY];
		}
		request_extensions[XR_MSFT_HAND_INTERACTION_EXTENSION_NAME] = &flags[WMR_HAND_INTERACTION];
	} break;
	case PROFILE_HAND_INTERACTION: {
		request_extensions[XR_EXT_HAND_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_LOCAL_FLOOR: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_LOCAL_FLOOR_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_DPAD_BINDING: {
		request_extensions[XR_KHR_BINDING_MODIFICATION_EXTENSION_NAME] = &flags[0];
		request_extensions[XR_EXT_DPAD_BINDING_EXTENSION_NAME] = &flags[1];
	} break;
	case PROFILE_VALVE_ANALOG_THRESHOLD: {
		request_extensions[XR_KHR_BINDING_MODIFICATION_EXTENSION_NAME] = &flags[0];
		request_extensions[XR_VALVE_ANALOG_THRESHOLD_EXTENSION_NAME] = &flags[1];
	} break;
	case PROFILE_COMPOSITION_LAYER_DEPTH: {
		request_extensions[XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_MAX:
		break;
	}

	return request_extensions;
}

PackedStringArray OpenXRInteractionProfileExtension::get_suggested_tracker_names() const
{
	switch (kind) {
	case PROFILE_HTC_CONTROLLER: {
		return PackedStringArray{
			"/user/hand_htc/left",
			"/user/hand_htc/right",
		};
	}
	case PROFILE_HTC_VIVE_TRACKER: {
		return PackedStringArray{
			"/user/vive_tracker_htcx/role/handheld_object",
			"/user/vive_tracker_htcx/role/left_foot",
			"/user/vive_tracker_htcx/role/right_foot",
			"/user/vive_tracker_htcx/role/left_shoulder",
			"/user/vive_tracker_htcx/role/right_shoulder",
			"/user/vive_tracker_htcx/role/left_elbow",
			"/user/vive_tracker_htcx/role/right_elbow",
			"/user/vive_tracker_htcx/role/left_knee",
			"/user/vive_tracker_htcx/role/right_knee",
			"/user/vive_tracker_htcx/role/waist",
			"/user/vive_tracker_htcx/role/chest",
			"/user/vive_tracker_htcx/role/camera",
			"/user/vive_tracker_htcx/role/keyboard",
			"/user/vive_tracker_htcx/role/left_wrist",
			"/user/vive_tracker_htcx/role/right_wrist",
			"/user/vive_tracker_htcx/role/left_ankle",
			"/user/vive_tracker_htcx/role/right_ankle",
		};
	}
	default:
		return PackedStringArray();
	}
}

bool OpenXRInteractionProfileExtension::on_event_polled(const XrEventDataBuffer& p_event)
{
	if (kind == PROFILE_HTC_VIVE_TRACKER &&
		p_event.type == XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX) {
		print_verbose("OpenXR EVENT: VIVE tracker connected");
		return true;
	}
	return false;
}

void OpenXRInteractionProfileExtension::on_register_metadata()
{
	if (kind != PROFILE_HAND_INTERACTION) {
		return;
	}

	const String profile_path = "/interaction_profiles/ext/hand_interaction_ext";
	OpenXRInteractionProfileMetadata::register_interaction_profile(
		"Hand interaction", profile_path, XR_EXT_HAND_INTERACTION_EXTENSION_NAME);
	for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grip pose", user_path,
			user_path + "/input/grip/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim pose", user_path,
			user_path + "/input/aim/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch pose", user_path,
			user_path + "/input/pinch_ext/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Poke pose", user_path,
			user_path + "/input/poke_ext/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grip surface pose",
			user_path, user_path + "/input/grip_surface/pose",
			XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
											"," XR_OPENXR_1_1_NAME,
			OpenXRAction::OPENXR_ACTION_POSE);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch", user_path,
			user_path + "/input/pinch_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch ready", user_path,
			user_path + "/input/pinch_ext/ready_ext", "", OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim activate", user_path,
			user_path + "/input/aim_activate_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim activate ready",
			user_path, user_path + "/input/aim_activate_ext/ready_ext", "",
			OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grasp", user_path,
			user_path + "/input/grasp_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grasp ready", user_path,
			user_path + "/input/grasp_ext/ready_ext", "", OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe left", user_path,
			user_path + "/input/swipe_left_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe right", user_path,
			user_path + "/input/swipe_right_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe forward", user_path,
			user_path + "/input/swipe_forward_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe backward", user_path,
			user_path + "/input/swipe_backward_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Tap thumb", user_path,
			user_path + "/input/tap_thumb_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
	}
}

bool OpenXRInteractionProfileExtension::is_available(int p_subindex) const
{
	if (kind == PROFILE_DPAD_BINDING || kind == PROFILE_VALVE_ANALOG_THRESHOLD) {
		return flags[0] && flags[1];
	}
	ERR_FAIL_INDEX_V(p_subindex, 3, false);
	return flags[p_subindex];
}

////////////////////////////////////////////////////////////////////////////
// OpenXRDpadBindingModifier

OpenXRDpadBindingModifier::OpenXRDpadBindingModifier()
{
	ERR_FAIL_COND(
		dpad_bindings_data.resize_initialized(sizeof(XrInteractionProfileDpadBindingEXT)) != OK);
	dpad_bindings = (XrInteractionProfileDpadBindingEXT*)dpad_bindings_data.ptrw();

	dpad_bindings->type = XR_TYPE_INTERACTION_PROFILE_DPAD_BINDING_EXT;
	dpad_bindings->next = nullptr;

	dpad_bindings->forceThreshold = 0.6;
	dpad_bindings->forceThresholdReleased = 0.4;
	dpad_bindings->centerRegion = 0.1;
	dpad_bindings->wedgeAngle = Math::deg_to_rad(90.0);
	dpad_bindings->isSticky = false;
}

void OpenXRDpadBindingModifier::set_action_set(const Ref<OpenXRActionSet>& p_action_set)
{
	action_set = p_action_set;
}

Ref<OpenXRActionSet> OpenXRDpadBindingModifier::get_action_set() const { return action_set; }

void OpenXRDpadBindingModifier::set_input_path(const String& p_input_path)
{
	input_path = p_input_path;
	emit_changed();
}

String OpenXRDpadBindingModifier::get_input_path() const { return input_path; }

void OpenXRDpadBindingModifier::set_threshold(float p_threshold)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	dpad_bindings->forceThreshold = p_threshold;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_threshold() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->forceThreshold;
}

void OpenXRDpadBindingModifier::set_threshold_released(float p_threshold)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	dpad_bindings->forceThresholdReleased = p_threshold;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_threshold_released() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->forceThresholdReleased;
}

void OpenXRDpadBindingModifier::set_center_region(float p_center_region)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_center_region < 0.0 || p_center_region > 1.0);

	dpad_bindings->centerRegion = p_center_region;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_center_region() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->centerRegion;
}

void OpenXRDpadBindingModifier::set_wedge_angle(float p_wedge_angle)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->wedgeAngle = p_wedge_angle;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_wedge_angle() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->wedgeAngle;
}

void OpenXRDpadBindingModifier::set_wedge_angle_deg(float p_wedge_angle)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->wedgeAngle = Math::deg_to_rad(p_wedge_angle);
	emit_changed();
}

float OpenXRDpadBindingModifier::get_wedge_angle_deg() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return Math::rad_to_deg(dpad_bindings->wedgeAngle);
}

void OpenXRDpadBindingModifier::set_is_sticky(bool p_sticky)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->isSticky = p_sticky;
	emit_changed();
}

bool OpenXRDpadBindingModifier::get_is_sticky() const
{
	ERR_FAIL_NULL_V(dpad_bindings, false);
	return dpad_bindings->isSticky;
}

void OpenXRDpadBindingModifier::set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	on_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRDpadBindingModifier::get_on_haptic() const { return on_haptic; }

void OpenXRDpadBindingModifier::set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	off_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRDpadBindingModifier::get_off_haptic() const { return off_haptic; }

PackedByteArray OpenXRDpadBindingModifier::get_ip_modification()
{
	ERR_FAIL_NULL_V(dpad_bindings, PackedByteArray());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedByteArray());

	if (!OpenXRInteractionProfileExtension::is_initialized() ||
		!OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_DPAD_BINDING)) {
		WARN_PRINT("DPad binding extension is not enabled or available.");
		return PackedByteArray();
	}

	dpad_bindings->binding = OpenXRAPI::get_xr_path(input_path);
	ERR_FAIL_COND_V(dpad_bindings->binding == XR_NULL_PATH, PackedByteArray());

	ERR_FAIL_COND_V(action_set.is_null(), PackedByteArray());
	RID action_set_rid = OpenXRAPI::find_action_set(action_set->get_name());
	ERR_FAIL_COND_V(!action_set_rid.is_valid(), PackedByteArray());
	dpad_bindings->actionSet = OpenXRAPI::action_set_get_handle(action_set_rid);

	dpad_bindings->onHaptic = on_haptic.is_valid() ? on_haptic->get_xr_structure() : nullptr;
	dpad_bindings->offHaptic = off_haptic.is_valid() ? off_haptic->get_xr_structure() : nullptr;

	return dpad_bindings_data;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRAnalogThresholdModifier

OpenXRAnalogThresholdModifier::OpenXRAnalogThresholdModifier()
{
	analog_threshold.type = XR_TYPE_INTERACTION_PROFILE_ANALOG_THRESHOLD_VALVE;
	analog_threshold.next = nullptr;

	analog_threshold.onThreshold = 0.6;
	analog_threshold.offThreshold = 0.4;
}

void OpenXRAnalogThresholdModifier::set_on_threshold(float p_threshold)
{
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	analog_threshold.onThreshold = p_threshold;
	emit_changed();
}

float OpenXRAnalogThresholdModifier::get_on_threshold() const
{
	return analog_threshold.onThreshold;
}

void OpenXRAnalogThresholdModifier::set_off_threshold(float p_threshold)
{
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	analog_threshold.offThreshold = p_threshold;
	emit_changed();
}

float OpenXRAnalogThresholdModifier::get_off_threshold() const
{
	return analog_threshold.offThreshold;
}

void OpenXRAnalogThresholdModifier::set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	on_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRAnalogThresholdModifier::get_on_haptic() const { return on_haptic; }

void OpenXRAnalogThresholdModifier::set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	off_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRAnalogThresholdModifier::get_off_haptic() const { return off_haptic; }

PackedByteArray OpenXRAnalogThresholdModifier::get_ip_modification()
{
	PackedByteArray ret;

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	if (!OpenXRInteractionProfileExtension::is_initialized() ||
		!OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_VALVE_ANALOG_THRESHOLD)) {
		WARN_PRINT("Analog threshold extension is not enabled or available.");
		return ret;
	}

	ERR_FAIL_NULL_V(ip_binding, ret);

	Ref<OpenXRAction> action = ip_binding->get_action();
	ERR_FAIL_COND_V(action.is_null(), ret);

	Ref<OpenXRActionSet> action_set = action->get_action_set();
	ERR_FAIL_COND_V(action_set.is_null(), ret);
	RID action_set_rid = OpenXRAPI::find_action_set(action_set->get_name());
	ERR_FAIL_COND_V(!action_set_rid.is_valid(), ret);

	RID action_rid = OpenXRAPI::find_action(action->get_name(), action_set_rid);
	ERR_FAIL_COND_V(!action_rid.is_valid(), ret);

	analog_threshold.action = OpenXRAPI::action_get_handle(action_rid);

	analog_threshold.binding = OpenXRAPI::get_xr_path(ip_binding->get_binding_path());
	ERR_FAIL_COND_V(analog_threshold.binding == XR_NULL_PATH, ret);

	analog_threshold.onHaptic = on_haptic.is_valid() ? on_haptic->get_xr_structure() : nullptr;
	analog_threshold.offHaptic = off_haptic.is_valid() ? off_haptic->get_xr_structure() : nullptr;

	ERR_FAIL_COND_V(ret.resize(sizeof(XrInteractionProfileAnalogThresholdVALVE)) != OK, ret);
	memcpy(ret.ptrw(), &analog_threshold, sizeof(XrInteractionProfileAnalogThresholdVALVE));

	return ret;
}
