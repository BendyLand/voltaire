/**************************************************************************/
/*  openxr_action_map.h                                                   */
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
#include "core/io/resource.h"
#include "core/templates/hash_map.h"

class OpenXRActionSet;
class OpenXRActionMap;
class OpenXRInteractionProfile;
class OpenXRIPBinding;

///////////////////////////////////////////////////////////////////////////
// OpenXRAction

class OpenXRAction final : public Resource
{
public:
	enum ActionType
	{
		OPENXR_ACTION_BOOL,
		OPENXR_ACTION_FLOAT,
		OPENXR_ACTION_VECTOR2,
		OPENXR_ACTION_POSE,
		OPENXR_ACTION_HAPTIC,
		OPENXR_ACTION_MAX
	};

private:
	String localized_name;
	ActionType action_type = OPENXR_ACTION_FLOAT;

	PackedStringArray toplevel_paths;

	friend class OpenXRActionSet;
	OpenXRActionSet* action_set = nullptr;

public:
	static Ref<OpenXRAction> new_action(const char* p_name, const char* p_localized_name,
		const ActionType p_action_type, const char* p_toplevel_paths);

	OpenXRActionSet* get_action_set() const { return action_set; }

	String get_name_with_set() const;

	void set_localized_name(const String& p_localized_name);
	String get_localized_name() const;

	void set_action_type(const ActionType p_action_type);
	ActionType get_action_type() const;

	void set_toplevel_paths(const PackedStringArray p_toplevel_paths);
	PackedStringArray get_toplevel_paths() const;

	void add_toplevel_path(const String& p_toplevel_path);
	void rem_toplevel_path(const String& p_toplevel_path);

	void parse_toplevel_paths(const String& p_toplevel_paths);
};

///////////////////////////////////////////////////////////////////////////
// OpenXRActionSet

class OpenXRActionSet final : public Resource
{
private:
	String localized_name;
	int priority = 0;

	void clear_actions();

public:
	static Ref<OpenXRActionSet> new_action_set(
		const char* p_name, const char* p_localized_name, const int p_priority = 0);

	void set_localized_name(const String& p_localized_name);
	String get_localized_name() const;

	void set_priority(const int p_priority);
	int get_priority() const;

	int get_action_count() const;

	Ref<OpenXRAction> get_action(const String& p_name) const;
	void add_action(const Ref<OpenXRAction>& p_action);
	void remove_action(const Ref<OpenXRAction>& p_action);

	Ref<OpenXRAction> add_new_action(const char* p_name, const char* p_localized_name,
		const OpenXRAction::ActionType p_action_type, const char* p_toplevel_paths);

	~OpenXRActionSet();
};

///////////////////////////////////////////////////////////////////////////
// OpenXRHapticBase

class OpenXRHapticBase final : public Resource
{
private:
	XrHapticVibration haptic_vibration;

public:
	void set_duration(int64_t p_duration);
	int64_t get_duration() const;

	void set_frequency(float p_frequency);
	float get_frequency() const;

	void set_amplitude(float p_amplitude);
	float get_amplitude() const;

	const XrHapticBaseHeader* get_xr_structure();

	OpenXRHapticBase();
};

using OpenXRHapticVibration = OpenXRHapticBase;

///////////////////////////////////////////////////////////////////////////
// OpenXRBindingModifier

class OpenXRBindingModifier : public Resource
{
protected:
	friend class OpenXRInteractionProfile;
	friend class OpenXRIPBinding;

	OpenXRInteractionProfile* interaction_profile = nullptr;
	OpenXRIPBinding* ip_binding = nullptr;

public:
	OpenXRInteractionProfile* get_interaction_profile() const { return interaction_profile; }
	OpenXRIPBinding* get_ip_binding() const { return ip_binding; }

	virtual String get_description() const;
	virtual PackedByteArray get_ip_modification();

	OpenXRBindingModifier();
	~OpenXRBindingModifier();
};

using OpenXRIPBindingModifier = OpenXRBindingModifier;
using OpenXRActionBindingModifier = OpenXRBindingModifier;

///////////////////////////////////////////////////////////////////////////
// OpenXRIPBinding & OpenXRInteractionProfile

class OpenXRIPBinding final : public Resource
{
private:
	Ref<OpenXRAction> action;
	String binding_path;
	Vector<Ref<OpenXRBindingModifier>> binding_modifiers;

	friend class OpenXRActionMap;
	OpenXRActionMap* action_map = nullptr;

public:
	static Ref<OpenXRIPBinding> new_binding(
		const Ref<OpenXRAction>& p_action, const String& p_binding_path);

	OpenXRActionMap* get_action_map() { return action_map; }

	void set_action(const Ref<OpenXRAction>& p_action);
	Ref<OpenXRAction> get_action() const;

	void set_binding_path(const String& p_path);
	String get_binding_path() const;

	int get_binding_modifier_count() const;
	Ref<OpenXRBindingModifier> get_binding_modifier(int p_index) const;
	void clear_binding_modifiers();

	void add_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier);
	void remove_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier);

#ifndef DISABLE_DEPRECATED
	void set_paths(const PackedStringArray& p_paths);
	PackedStringArray get_paths() const;
	int get_path_count() const;
	bool has_path(const String& p_path) const;
	void add_path(const String& p_path);
	void remove_path(const String& p_path);
#endif // DISABLE_DEPRECATED

	~OpenXRIPBinding();
};

class OpenXRInteractionProfile final : public Resource
{
private:
	String interaction_profile_path;
	Vector<Ref<OpenXRBindingModifier>> binding_modifiers;

	friend class OpenXRActionMap;
	OpenXRActionMap* action_map = nullptr;

public:
	static Ref<OpenXRInteractionProfile> new_profile(const char* p_input_profile_path);

	OpenXRActionMap* get_action_map() { return action_map; }

	void set_interaction_profile_path(const String& p_input_profile_path);
	String get_interaction_profile_path() const;

	int get_binding_count() const;
	Ref<OpenXRIPBinding> get_binding(int p_index) const;

	Ref<OpenXRIPBinding> find_binding(
		const Ref<OpenXRAction>& p_action, const String& p_binding_path) const;
	Vector<Ref<OpenXRIPBinding>> get_bindings_for_action(const Ref<OpenXRAction>& p_action) const;
	void add_binding(const Ref<OpenXRIPBinding>& p_binding);
	void remove_binding(const Ref<OpenXRIPBinding>& p_binding);

	void add_new_binding(const Ref<OpenXRAction>& p_action, const String& p_paths);
	void remove_binding_for_action(const Ref<OpenXRAction>& p_action);
	bool has_binding_for_action(const Ref<OpenXRAction>& p_action);

	int get_binding_modifier_count() const;
	Ref<OpenXRBindingModifier> get_binding_modifier(int p_index) const;
	void clear_binding_modifiers();

	void add_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier);
	void remove_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier);

	~OpenXRInteractionProfile() = default;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileMetadata

#define XR_PATH_UNSUPPORTED_NAME "unsupported"

class OpenXRInteractionProfileMetadata final
{
public:
	struct TopLevelPath
	{
		String display_name;
		String openxr_path;
		String openxr_extension_names;
	};

	struct IOPath
	{
		String display_name;
		String top_level_path;
		String openxr_path;
		String openxr_extension_names;
		OpenXRAction::ActionType action_type;
	};

	struct InteractionProfile
	{
		String display_name;
		String openxr_path;
		String openxr_extension_names;
		Vector<IOPath> io_paths;

		bool has_io_path(const String& p_io_path) const;
		const IOPath* get_io_path(const String& p_io_path) const;
	};

private:
	static void _register_core_metadata();

public:
	static void register_profile_rename(const String& p_old_name, const String& p_new_name);
	static String check_profile_name(const String& p_name);

	static void register_path_rename(const String& p_old_name, const String& p_new_name);
	static String check_path_name(const String& p_name);

	static void register_top_level_path(const String& p_display_name, const String& p_openxr_path,
		const String& p_openxr_extension_names);
	static bool has_top_level_path(const String& p_openxr_path);
	static String get_top_level_name(const String& p_openxr_path);
	static String get_top_level_extensions(const String& p_openxr_path);

	static void register_interaction_profile(const String& p_display_name,
		const String& p_openxr_path, const String& p_openxr_extension_names);
	static bool has_interaction_profile(const String& p_openxr_path);
	static String get_interaction_profile_extensions(const String& p_openxr_path);
	static const InteractionProfile* get_profile(const String& p_openxr_path);
	static PackedStringArray get_interaction_profile_paths();

	static void register_io_path(const String& p_interaction_profile, const String& p_display_name,
		const String& p_toplevel_path, const String& p_openxr_path,
		const String& p_openxr_extension_names, OpenXRAction::ActionType p_action_type);
	static const IOPath* get_io_path(const String& p_interaction_profile, const String& p_io_path);

	struct Data
	{
		HashMap<String, String> profile_renames;
		HashMap<String, String> path_renames;
		Vector<TopLevelPath> top_level_paths;
		Vector<InteractionProfile> interaction_profiles;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRInteractionProfileMetadata() = delete;
	OpenXRInteractionProfileMetadata(const OpenXRInteractionProfileMetadata&) = delete;
	OpenXRInteractionProfileMetadata& operator=(const OpenXRInteractionProfileMetadata&) = delete;
	~OpenXRInteractionProfileMetadata() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRActionMap

class OpenXRActionMap final : public Resource
{
public:
	int get_action_set_count() const;
	Ref<OpenXRActionSet> find_action_set(const String& p_name) const;
	Ref<OpenXRActionSet> get_action_set(int p_idx) const;
	void add_action_set(const Ref<OpenXRActionSet>& p_action_set);
	void remove_action_set(const Ref<OpenXRActionSet>& p_action_set);

	void clear_interaction_profiles();

	int get_interaction_profile_count() const;
	Ref<OpenXRInteractionProfile> find_interaction_profile(const String& p_path) const;
	Ref<OpenXRInteractionProfile> get_interaction_profile(int p_idx) const;
	void add_interaction_profile(const Ref<OpenXRInteractionProfile>& p_interaction_profile);
	void remove_interaction_profile(const Ref<OpenXRInteractionProfile>& p_interaction_profile);

	void create_default_action_sets();
	void create_editor_action_sets();

	Ref<OpenXRAction> get_action(const String& p_path) const;
	void remove_action(const String& p_path, bool p_remove_interaction_profiles = false);
	PackedStringArray get_top_level_paths(const Ref<OpenXRAction>& p_action);

	~OpenXRActionMap() = default;
};
