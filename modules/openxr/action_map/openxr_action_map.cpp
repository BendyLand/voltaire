/**************************************************************************/
/*  openxr_action_map.cpp                                                 */
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
#include "openxr_action_map.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRAction

Ref<OpenXRAction> OpenXRAction::new_action(const char* p_name, const char* p_localized_name,
	const ActionType p_action_type, const char* p_toplevel_paths)
{
	Ref<OpenXRAction> action;
	action.instantiate();
	action->set_name(String(p_name));
	action->set_localized_name(String(p_localized_name));
	action->set_action_type(p_action_type);
	action->parse_toplevel_paths(String(p_toplevel_paths));

	return action;
}

String OpenXRAction::get_name_with_set() const
{
	String action_name = get_name();

	if (action_set != nullptr) {
		action_name = action_set->get_name() + "/" + action_name;
	}

	return action_name;
}

void OpenXRAction::set_localized_name(const String& p_localized_name)
{
	localized_name = p_localized_name;
	emit_changed();
}

String OpenXRAction::get_localized_name() const { return localized_name; }

void OpenXRAction::set_action_type(const OpenXRAction::ActionType p_action_type)
{
	action_type = p_action_type;
	emit_changed();
}

OpenXRAction::ActionType OpenXRAction::get_action_type() const { return action_type; }

void OpenXRAction::set_toplevel_paths(const PackedStringArray p_toplevel_paths)
{
	toplevel_paths = p_toplevel_paths;
	emit_changed();
}

PackedStringArray OpenXRAction::get_toplevel_paths() const { return toplevel_paths; }

void OpenXRAction::add_toplevel_path(const String& p_toplevel_path)
{
	if (!toplevel_paths.has(p_toplevel_path)) {
		toplevel_paths.push_back(p_toplevel_path);
		emit_changed();
	}
}

void OpenXRAction::rem_toplevel_path(const String& p_toplevel_path)
{
	if (toplevel_paths.has(p_toplevel_path)) {
		toplevel_paths.erase(p_toplevel_path);
		emit_changed();
	}
}

void OpenXRAction::parse_toplevel_paths(const String& p_toplevel_paths)
{
	toplevel_paths = p_toplevel_paths.split(",", false);
	emit_changed();
}

////////////////////////////////////////////////////////////////////////////
// OpenXRActionSet

Ref<OpenXRActionSet> OpenXRActionSet::new_action_set(
	const char* p_name, const char* p_localized_name, const int p_priority)
{
	Ref<OpenXRActionSet> action_set;
	action_set.instantiate();
	action_set->set_name(String(p_name));
	action_set->set_localized_name(p_localized_name);
	action_set->set_priority(p_priority);

	return action_set;
}

void OpenXRActionSet::set_localized_name(const String& p_localized_name)
{
	localized_name = p_localized_name;
	emit_changed();
}

String OpenXRActionSet::get_localized_name() const { return localized_name; }

void OpenXRActionSet::set_priority(const int p_priority)
{
	priority = p_priority;
	emit_changed();
}

int OpenXRActionSet::get_priority() const { return priority; }

Ref<OpenXRAction> OpenXRActionSet::add_new_action(const char* p_name, const char* p_localized_name,
	const OpenXRAction::ActionType p_action_type, const char* p_toplevel_paths)
{
	Ref<OpenXRAction> new_action =
		OpenXRAction::new_action(p_name, p_localized_name, p_action_type, p_toplevel_paths);
	add_action(new_action);
	return new_action;
}

OpenXRActionSet::~OpenXRActionSet() { clear_actions(); }

void OpenXRActionSet::clear_actions() {}

void OpenXRActionSet::add_action(Ref<OpenXRAction> const&) {}

void OpenXRActionSet::remove_action(Ref<OpenXRAction> const&) {}

Ref<OpenXRAction> OpenXRActionSet::get_action(String const&) const { return Ref<OpenXRAction>(); }

////////////////////////////////////////////////////////////////////////////
// OpenXRHapticBase

void OpenXRHapticBase::set_duration(int64_t p_duration)
{
	haptic_vibration.duration = p_duration;
	emit_changed();
}

int64_t OpenXRHapticBase::get_duration() const { return haptic_vibration.duration; }

void OpenXRHapticBase::set_frequency(float p_frequency)
{
	haptic_vibration.frequency = p_frequency;
	emit_changed();
}

float OpenXRHapticBase::get_frequency() const { return haptic_vibration.frequency; }

void OpenXRHapticBase::set_amplitude(float p_amplitude)
{
	haptic_vibration.amplitude = p_amplitude;
	emit_changed();
}

float OpenXRHapticBase::get_amplitude() const { return haptic_vibration.amplitude; }

const XrHapticBaseHeader* OpenXRHapticBase::get_xr_structure()
{
	return (XrHapticBaseHeader*)&haptic_vibration;
}

OpenXRHapticBase::OpenXRHapticBase()
{
	haptic_vibration.type = XR_TYPE_HAPTIC_VIBRATION;
	haptic_vibration.next = nullptr;
	haptic_vibration.duration = -1;
	haptic_vibration.frequency = 0.0;
	haptic_vibration.amplitude = 1.0;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRBindingModifier

PackedByteArray OpenXRBindingModifier::get_ip_modification() { return PackedByteArray(); }

String OpenXRBindingModifier::get_description() const { return ""; }

OpenXRBindingModifier::OpenXRBindingModifier() {}

OpenXRBindingModifier::~OpenXRBindingModifier() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRIPBinding & OpenXRInteractionProfile

Ref<OpenXRIPBinding> OpenXRIPBinding::new_binding(
	const Ref<OpenXRAction>& p_action, const String& p_binding_path)
{
	Ref<OpenXRIPBinding> binding;
	binding.instantiate();
	binding->set_action(p_action);
	binding->set_binding_path(p_binding_path);

	return binding;
}

void OpenXRIPBinding::set_action(const Ref<OpenXRAction>& p_action)
{
	action = p_action;
	emit_changed();
}

Ref<OpenXRAction> OpenXRIPBinding::get_action() const { return action; }

void OpenXRIPBinding::set_binding_path(const String& p_path)
{
	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		binding_path = OpenXRInteractionProfileMetadata::check_path_name(p_path);
	}
	else {
		binding_path = p_path;
	}

	emit_changed();
}

String OpenXRIPBinding::get_binding_path() const { return binding_path; }

int OpenXRIPBinding::get_binding_modifier_count() const { return binding_modifiers.size(); }

Ref<OpenXRBindingModifier> OpenXRIPBinding::get_binding_modifier(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, binding_modifiers.size(), nullptr);

	return binding_modifiers[p_index];
}

void OpenXRIPBinding::clear_binding_modifiers()
{
	if (binding_modifiers.is_empty()) {
		return;
	}

	for (int i = 0; i < binding_modifiers.size(); i++) {
		Ref<OpenXRBindingModifier> binding_modifier = binding_modifiers[i];
		binding_modifier->ip_binding = nullptr;
	}
	binding_modifiers.clear();
	emit_changed();
}

void OpenXRIPBinding::add_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	ERR_FAIL_COND(p_binding_modifier.is_null());

	if (!binding_modifiers.has(p_binding_modifier)) {
		if (p_binding_modifier->ip_binding && p_binding_modifier->ip_binding != this) {
			p_binding_modifier->ip_binding->remove_binding_modifier(p_binding_modifier);
		}

		p_binding_modifier->ip_binding = this;
		binding_modifiers.push_back(p_binding_modifier);
		emit_changed();
	}
}

void OpenXRIPBinding::remove_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	int idx = binding_modifiers.find(p_binding_modifier);
	if (idx != -1) {
		binding_modifiers.remove_at(idx);

		ERR_FAIL_COND_MSG(p_binding_modifier->ip_binding != this,
			"Removing binding modifier that belongs to this binding but had incorrect binding "
			"pointer.");
		p_binding_modifier->ip_binding = nullptr;

		emit_changed();
	}
}

#ifndef DISABLE_DEPRECATED

void OpenXRIPBinding::set_paths(const PackedStringArray& p_paths)
{
	binding_path = "";
	for (const String& path : p_paths) {
		if (!binding_path.is_empty()) {
			binding_path += ",";
		}
		binding_path += path;
	}
}

PackedStringArray OpenXRIPBinding::get_paths() const
{
	PackedStringArray paths = binding_path.split(",", false);
	return paths;
}

int OpenXRIPBinding::get_path_count() const
{
	return binding_path.is_empty() ? 0 : 1;
}

bool OpenXRIPBinding::has_path(const String& p_path) const
{
	return binding_path == p_path;
}

void OpenXRIPBinding::add_path(const String& p_path)
{
	if (binding_path != p_path) {
		ERR_FAIL_COND_MSG(!binding_path.is_empty(),
			"Method add_path has been deprecated. A binding path was already set, create separate "
			"binding resources for each path and use set_binding_path instead.");

		binding_path = p_path;
		emit_changed();
	}
}

void OpenXRIPBinding::remove_path(const String& p_path)
{
	ERR_FAIL_COND_MSG(binding_path != p_path,
		"Method remove_path has been deprecated. Attempt at removing a different binding path, "
		"remove the correct binding record from the interaction profile instead.");

	binding_path = p_path;
	emit_changed();
}

#endif // DISABLE_DEPRECATED

OpenXRIPBinding::~OpenXRIPBinding() { action.unref(); }

Ref<OpenXRInteractionProfile> OpenXRInteractionProfile::new_profile(
	const char* p_input_profile_path)
{
	Ref<OpenXRInteractionProfile> profile;
	profile.instantiate();
	profile->set_interaction_profile_path(String(p_input_profile_path));

	return profile;
}

void OpenXRInteractionProfile::set_interaction_profile_path(const String& p_input_profile_path)
{
	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		interaction_profile_path =
			OpenXRInteractionProfileMetadata::check_profile_name(p_input_profile_path);
	}
	else {
		interaction_profile_path = p_input_profile_path;
	}
	emit_changed();
}

String OpenXRInteractionProfile::get_interaction_profile_path() const
{
	return interaction_profile_path;
}

void OpenXRInteractionProfile::add_new_binding(
	const Ref<OpenXRAction>& p_action, const String& p_paths)
{
	PackedStringArray paths = p_paths.split(",", false);

	for (const String& path : paths) {
		Ref<OpenXRIPBinding> binding = OpenXRIPBinding::new_binding(p_action, path);
		add_binding(binding);
	}
}

int OpenXRInteractionProfile::get_binding_modifier_count() const
{
	return binding_modifiers.size();
}

Ref<OpenXRBindingModifier> OpenXRInteractionProfile::get_binding_modifier(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, binding_modifiers.size(), nullptr);

	return binding_modifiers[p_index];
}

void OpenXRInteractionProfile::clear_binding_modifiers()
{
	if (binding_modifiers.is_empty()) {
		return;
	}

	for (int i = 0; i < binding_modifiers.size(); i++) {
		Ref<OpenXRBindingModifier> binding_modifier = binding_modifiers[i];
		binding_modifier->interaction_profile = nullptr;
	}
	binding_modifiers.clear();
	emit_changed();
}

void OpenXRInteractionProfile::add_binding_modifier(
	const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	ERR_FAIL_COND(p_binding_modifier.is_null());

	if (!binding_modifiers.has(p_binding_modifier)) {
		if (p_binding_modifier->interaction_profile &&
			p_binding_modifier->interaction_profile != this) {
			p_binding_modifier->interaction_profile->remove_binding_modifier(p_binding_modifier);
		}

		p_binding_modifier->interaction_profile = this;
		binding_modifiers.push_back(p_binding_modifier);
		emit_changed();
	}
}

void OpenXRInteractionProfile::remove_binding_modifier(
	const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	int idx = binding_modifiers.find(p_binding_modifier);
	if (idx != -1) {
		binding_modifiers.remove_at(idx);

		ERR_FAIL_COND_MSG(p_binding_modifier->interaction_profile != this,
			"Removing binding modifier that belongs to this interaction profile but had incorrect "
			"interaction profile pointer.");
		p_binding_modifier->interaction_profile = nullptr;

		emit_changed();
	}
}

void OpenXRInteractionProfile::add_binding(const Ref<OpenXRIPBinding>& p_binding) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileMetadata

void OpenXRInteractionProfileMetadata::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
	_register_core_metadata();
}

void OpenXRInteractionProfileMetadata::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRInteractionProfileMetadata::register_profile_rename(
	const String& p_old_name, const String& p_new_name)
{
	ERR_FAIL_COND(data->profile_renames.has(p_old_name));

	data->profile_renames[p_old_name] = p_new_name;
}

String OpenXRInteractionProfileMetadata::check_profile_name(const String& p_name)
{
	if (data->profile_renames.has(p_name)) {
		return data->profile_renames[p_name];
	}

	return p_name;
}

void OpenXRInteractionProfileMetadata::register_path_rename(
	const String& p_old_name, const String& p_new_name)
{
	ERR_FAIL_COND(data->path_renames.has(p_old_name));

	data->path_renames[p_old_name] = p_new_name;
}

String OpenXRInteractionProfileMetadata::check_path_name(const String& p_name)
{
	if (data->path_renames.has(p_name)) {
		return data->path_renames[p_name];
	}

	return p_name;
}

void OpenXRInteractionProfileMetadata::register_top_level_path(const String& p_display_name,
	const String& p_openxr_path, const String& p_openxr_extension_names)
{
	ERR_FAIL_COND_MSG(
		has_top_level_path(p_openxr_path), p_openxr_path + " had already been registered");

	TopLevelPath new_toplevel_path = {p_display_name, p_openxr_path, p_openxr_extension_names};

	data->top_level_paths.push_back(new_toplevel_path);
}

void OpenXRInteractionProfileMetadata::register_interaction_profile(const String& p_display_name,
	const String& p_openxr_path, const String& p_openxr_extension_names)
{
	ERR_FAIL_COND_MSG(
		has_interaction_profile(p_openxr_path), p_openxr_path + " has already been registered");

	InteractionProfile new_profile;
	new_profile.display_name = p_display_name;
	new_profile.openxr_path = p_openxr_path;
	new_profile.openxr_extension_names = p_openxr_extension_names;

	data->interaction_profiles.push_back(new_profile);
}

void OpenXRInteractionProfileMetadata::register_io_path(const String& p_interaction_profile,
	const String& p_display_name, const String& p_toplevel_path, const String& p_openxr_path,
	const String& p_openxr_extension_names, OpenXRAction::ActionType p_action_type)
{
	ERR_FAIL_COND_MSG(!has_interaction_profile(p_interaction_profile),
		"Unknown interaction profile " + p_interaction_profile);
	ERR_FAIL_COND_MSG(
		!has_top_level_path(p_toplevel_path), "Unknown top level path " + p_toplevel_path);

	for (InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_interaction_profile) {
			ERR_FAIL_COND_MSG(interaction_profile.has_io_path(p_openxr_path),
				p_interaction_profile + " already has io path " + p_openxr_path + " registered!");

			IOPath new_io_path = {p_display_name, p_toplevel_path, p_openxr_path,
				p_openxr_extension_names, p_action_type};

			interaction_profile.io_paths.push_back(new_io_path);
		}
	}
}

bool OpenXRInteractionProfileMetadata::has_top_level_path(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return true;
		}
	}

	return false;
}

String OpenXRInteractionProfileMetadata::get_top_level_name(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return data->top_level_paths[i].display_name;
		}
	}

	return String();
}

String OpenXRInteractionProfileMetadata::get_top_level_extensions(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return data->top_level_paths[i].openxr_extension_names;
		}
	}

	return XR_PATH_UNSUPPORTED_NAME;
}

bool OpenXRInteractionProfileMetadata::has_interaction_profile(const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return true;
		}
	}

	return false;
}

String OpenXRInteractionProfileMetadata::get_interaction_profile_extensions(
	const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return interaction_profile.openxr_extension_names;
		}
	}

	return XR_PATH_UNSUPPORTED_NAME;
}

const OpenXRInteractionProfileMetadata::InteractionProfile*
OpenXRInteractionProfileMetadata::get_profile(const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return &interaction_profile;
		}
	}

	return nullptr;
}

bool OpenXRInteractionProfileMetadata::InteractionProfile::has_io_path(
	const String& p_io_path) const
{
	for (int i = 0; i < io_paths.size(); i++) {
		if (io_paths[i].openxr_path == p_io_path) {
			return true;
		}
	}

	return false;
}

const OpenXRInteractionProfileMetadata::IOPath*
OpenXRInteractionProfileMetadata::InteractionProfile::get_io_path(const String& p_io_path) const
{
	for (int i = 0; i < io_paths.size(); i++) {
		if (io_paths[i].openxr_path == p_io_path) {
			return &io_paths[i];
		}
	}

	return nullptr;
}

const OpenXRInteractionProfileMetadata::IOPath* OpenXRInteractionProfileMetadata::get_io_path(
	const String& p_interaction_profile, const String& p_io_path)
{
	const OpenXRInteractionProfileMetadata::InteractionProfile* profile =
		get_profile(p_interaction_profile);
	if (profile != nullptr) {
		return profile->get_io_path(p_io_path);
	}

	return nullptr;
}

PackedStringArray OpenXRInteractionProfileMetadata::get_interaction_profile_paths()
{
	PackedStringArray arr;

	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		arr.push_back(interaction_profile.openxr_path);
	}

	return arr;
}

void OpenXRInteractionProfileMetadata::_register_core_metadata()
{
	register_path_rename(
		"/user/hand/left/input/palm_ext/pose", "/user/hand/left/input/grip_surface/pose");
	register_path_rename(
		"/user/hand/right/input/palm_ext/pose", "/user/hand/right/input/grip_surface/pose");

	// Our core toplevel paths
	register_top_level_path("Left hand controller", "/user/hand/left", "");
	register_top_level_path("Right hand controller", "/user/hand/right", "");
	register_top_level_path("Head", "/user/head", "");
	register_top_level_path("Gamepad", "/user/gamepad", "");
	register_top_level_path("Treadmill", "/user/treadmill", "");

	{ // Fallback Khronos simple controller
		const String profile_path = "/interaction_profiles/khr/simple_controller";
		register_interaction_profile("Simple controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Select click", user_path,
				user_path + "/input/select/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Original HTC Vive wands
		const String profile_path = "/interaction_profiles/htc/vive_controller";
		register_interaction_profile("HTC Vive wand", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "System click", user_path,
				user_path + "/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze click", user_path,
				user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad click", user_path,
				user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Microsoft motion controller (original WMR controllers)
		const String profile_path = "/interaction_profiles/microsoft/motion_controller";
		register_interaction_profile("MS Motion controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze click", user_path,
				user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad click", user_path,
				user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Meta touch controller (original touch controllers, Quest 1 and Quest 2 controllers)
		const String profile_path = "/interaction_profiles/oculus/touch_controller";
		register_interaction_profile("Touch controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger touch", user_path,
				user_path + "/input/trigger/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze", user_path, user_path + "/input/squeeze/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick touch", user_path,
				user_path + "/input/thumbstick/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Thumbrest touch", user_path,
				user_path + "/input/thumbrest/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}

		register_io_path(profile_path, "Menu click", "/user/hand/left",
			"/user/hand/left/input/menu/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "System click", "/user/hand/right",
			"/user/hand/right/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

		register_io_path(profile_path, "X click", "/user/hand/left",
			"/user/hand/left/input/x/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "X touch", "/user/hand/left",
			"/user/hand/left/input/x/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "Y click", "/user/hand/left",
			"/user/hand/left/input/y/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "Y touch", "/user/hand/left",
			"/user/hand/left/input/y/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "A click", "/user/hand/right",
			"/user/hand/right/input/a/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "A touch", "/user/hand/right",
			"/user/hand/right/input/a/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "B click", "/user/hand/right",
			"/user/hand/right/input/b/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "B touch", "/user/hand/right",
			"/user/hand/right/input/b/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
	}

	{ // Valve Index controller
		const String profile_path = "/interaction_profiles/valve/index_controller";
		register_interaction_profile("Index controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "System click", user_path,
				user_path + "/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "A click", user_path, user_path + "/input/a/click", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "A touch", user_path, user_path + "/input/a/touch", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "B click", user_path, user_path + "/input/b/click", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "B touch", user_path, user_path + "/input/b/touch", "",
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trigger touch", user_path,
				user_path + "/input/trigger/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze", user_path, user_path + "/input/squeeze/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Squeeze force", user_path,
				user_path + "/input/squeeze/force", "", OpenXRAction::OPENXR_ACTION_FLOAT);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick touch", user_path,
				user_path + "/input/thumbstick/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad force", user_path,
				user_path + "/input/trackpad/force", "", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRActionMap

void OpenXRActionMap::create_default_action_sets()
{
	Ref<OpenXRActionSet> action_set = OpenXRActionSet::new_action_set("godot", "Godot action set");
	add_action_set(action_set);

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
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/primary/click,/user/hand/right/input/primary/click");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/secondary/click,/user/hand/right/input/secondary/click");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
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
	profile->add_new_binding(
		menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/system/click");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click");
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click");
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
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
	profile->add_new_binding(
		select_button, "/user/hand/left/input/system/click,/user/hand/right/input/system/click");
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click");
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click");
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
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
	profile->add_new_binding(
		trigger, "/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(
		grip, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	add_interaction_profile(profile);
}

void OpenXRActionMap::create_editor_action_sets() {}

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

void OpenXRActionMap::add_interaction_profile(const Ref<OpenXRInteractionProfile>&) {}

int OpenXRActionMap::get_interaction_profile_count() const { return 0; }

void OpenXRActionMap::add_action_set(Ref<OpenXRActionSet> const&) {}

void OpenXRActionMap::remove_interaction_profile(Ref<OpenXRInteractionProfile> const&) {}

void OpenXRActionMap::remove_action_set(Ref<OpenXRActionSet> const&) {}

Ref<OpenXRInteractionProfile> OpenXRActionMap::get_interaction_profile(int) const
{
	return Ref<OpenXRInteractionProfile>();
}

Ref<OpenXRActionSet> OpenXRActionMap::find_action_set(String const&) const
{
	return Ref<OpenXRActionSet>();
}
