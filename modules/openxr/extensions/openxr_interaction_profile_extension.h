/**************************************************************************/
/*  openxr_interaction_profile_extension.h                                */
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

#include <openxr/openxr.h>
#include "../action_map/openxr_action_map.h"
#include "core/templates/hash_map.h"
#include "core/types.h"

class OpenXRInteractionProfileExtension final
{
public:
	enum ProfileKind
	{
		PROFILE_HTC_CONTROLLER = 0,
		PROFILE_HTC_VIVE_TRACKER,
		PROFILE_HUAWEI_CONTROLLER,
		PROFILE_KHR_GENERIC_CONTROLLER,
		PROFILE_META_CONTROLLER,
		PROFILE_ML2_CONTROLLER,
		PROFILE_MXINK,
		PROFILE_PALM_POSE,
		PROFILE_PICO_CONTROLLER,
		PROFILE_VALVE_CONTROLLER,
		PROFILE_WMR_CONTROLLER,
		PROFILE_HAND_INTERACTION,
		PROFILE_LOCAL_FLOOR,
		PROFILE_DPAD_BINDING,
		PROFILE_VALVE_ANALOG_THRESHOLD,
		PROFILE_COMPOSITION_LAYER_DEPTH,
		PROFILE_MAX
	};

	enum HTCControllers
	{
		HTC_VIVE_COSMOS = 0,
		HTC_VIVE_FOCUS3 = 1,
		HTC_HAND_INTERACTION = 2,
		HTC_MAX_CONTROLLERS = 3
	};

	enum MetaControllers
	{
		META_TOUCH_PROXIMITY = 0,
		META_TOUCH_PRO = 1,
		META_TOUCH_PLUS = 2,
		META_MAX_CONTROLLERS = 3
	};

	enum WMRControllers
	{
		WMR_HPMR = 0,
		WMR_SAMSUNG_ODESSY = 1,
		WMR_HAND_INTERACTION = 2,
		WMR_MAX_CONTROLLERS = 3
	};

private:
	ProfileKind kind = PROFILE_KHR_GENERIC_CONTROLLER;
	bool flags[3] = {false, false, false};

public:
	explicit OpenXRInteractionProfileExtension(
		ProfileKind p_kind = PROFILE_KHR_GENERIC_CONTROLLER);

	ProfileKind get_kind() const { return kind; }

	HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	PackedStringArray get_suggested_tracker_names() const;
	bool on_event_polled(const XrEventDataBuffer& p_event);
	void on_register_metadata();

	bool is_available(int p_subindex = 0) const;

	static inline OpenXRInteractionProfileExtension* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static OpenXRInteractionProfileExtension* get_profile(ProfileKind p_kind);
	static bool is_profile_available(ProfileKind p_kind, int p_subindex = 0);
	static HashMap<String, bool*> get_all_requested_extensions(XrVersion p_version);
};

class OpenXRDpadBindingModifier final : public OpenXRBindingModifier
{
private:
	PackedByteArray dpad_bindings_data;
	XrInteractionProfileDpadBindingEXT* dpad_bindings = nullptr;
	String input_path;
	Ref<OpenXRActionSet> action_set;
	Ref<OpenXRHapticBase> on_haptic;
	Ref<OpenXRHapticBase> off_haptic;

public:
	OpenXRDpadBindingModifier();

	void set_action_set(const Ref<OpenXRActionSet>& p_action_set);
	Ref<OpenXRActionSet> get_action_set() const;

	void set_input_path(const String& p_input_path);
	String get_input_path() const;

	void set_threshold(float p_threshold);
	float get_threshold() const;

	void set_threshold_released(float p_threshold);
	float get_threshold_released() const;

	void set_center_region(float p_center_region);
	float get_center_region() const;

	void set_wedge_angle(float p_wedge_angle);
	float get_wedge_angle() const;

	void set_wedge_angle_deg(float p_wedge_angle);
	float get_wedge_angle_deg() const;

	void set_is_sticky(bool p_sticky);
	bool get_is_sticky() const;

	void set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_on_haptic() const;

	void set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_off_haptic() const;

	virtual String get_description() const override { return "DPad modifier"; }
	virtual PackedByteArray get_ip_modification() override;
};

class OpenXRAnalogThresholdModifier final : public OpenXRBindingModifier
{
private:
	XrInteractionProfileAnalogThresholdVALVE analog_threshold;
	Ref<OpenXRHapticBase> on_haptic;
	Ref<OpenXRHapticBase> off_haptic;

public:
	OpenXRAnalogThresholdModifier();

	void set_on_threshold(float p_threshold);
	float get_on_threshold() const;

	void set_off_threshold(float p_threshold);
	float get_off_threshold() const;

	void set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_on_haptic() const;

	void set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_off_haptic() const;

	virtual String get_description() const override { return "Analog threshold modifier"; }
	virtual PackedByteArray get_ip_modification() override;
};
