/**************************************************************************/
/*  openxr_api.h                                                          */
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
#include <openxr/openxr_reflection.h>
// --- Folded from modules/openxr/action_map/openxr_action_map.h ---
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

// --- End modules/openxr/action_map/openxr_action_map.h ---
#include "core/error/error_macros.h"
#include "core/math/projection.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "core/types.h"
#include "servers/xr/xr_server.h"

///////////////////////////////////////////////////////////////////////////
// OpenXR Function Pointer & Enum Macros

#define UNPACK(...) __VA_ARGS__

#define INIT_XR_FUNC_V(openxr_api, name) \
	if constexpr (true) { \
		XrResult get_instance_proc_addr_result; \
		get_instance_proc_addr_result = OpenXRAPI::get_instance_proc_addr(#name, (PFN_xrVoidFunction *)&name##_ptr); \
		ERR_FAIL_COND_V(XR_FAILED(get_instance_proc_addr_result), false); \
	} else \
		((void)0)

#define EXT_INIT_XR_FUNC_V(name) INIT_XR_FUNC_V(nullptr, name)
#define OPENXR_API_INIT_XR_FUNC_V(name) INIT_XR_FUNC_V(nullptr, name)

#define INIT_XR_FUNC(openxr_api, name) \
	if constexpr (true) { \
		XrResult get_instance_proc_addr_result; \
		get_instance_proc_addr_result = OpenXRAPI::get_instance_proc_addr(#name, (PFN_xrVoidFunction *)&name##_ptr); \
		ERR_FAIL_COND(XR_FAILED(get_instance_proc_addr_result)); \
	} else \
		((void)0)

#define EXT_INIT_XR_FUNC(name) INIT_XR_FUNC(nullptr, name)
#define OPENXR_API_INIT_XR_FUNC(name) INIT_XR_FUNC(nullptr, name)

#define TRY_INIT_XR_FUNC(openxr_api, name) \
	OpenXRAPI::try_get_instance_proc_addr(#name, (PFN_xrVoidFunction *)&name##_ptr)

#define EXT_TRY_INIT_XR_FUNC(name) TRY_INIT_XR_FUNC(nullptr, name)
#define OPENXR_TRY_API_INIT_XR_FUNC(name) TRY_INIT_XR_FUNC(nullptr, name)
#define GDEXTENSION_INIT_XR_FUNC(name) \
	if constexpr (true) { \
		name##_ptr = reinterpret_cast<PFN_##name>(get_openxr_api()->get_instance_proc_addr(#name)); \
		ERR_FAIL_NULL(name##_ptr); \
	} else \
		((void)0)

#define GDEXTENSION_INIT_XR_FUNC_V(name) \
	if constexpr (true) { \
		name##_ptr = reinterpret_cast<PFN_##name>(get_openxr_api()->get_instance_proc_addr(#name)); \
		ERR_FAIL_NULL_V(name##_ptr, false); \
	} else \
		((void)0)

#define EXT_PROTO_XRRESULT_FUNC1(func_name, arg1_type, arg1) \
	static inline PFN_##func_name func_name##_ptr = nullptr; \
	static XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1) { \
		if (!func_name##_ptr) { \
			return XR_ERROR_HANDLE_INVALID; \
		} \
		return (*func_name##_ptr)(p_##arg1); \
	}

#define EXT_PROTO_XRRESULT_FUNC2(func_name, arg1_type, arg1, arg2_type, arg2) \
	static inline PFN_##func_name func_name##_ptr = nullptr; \
	static XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2) { \
		if (!func_name##_ptr) { \
			return XR_ERROR_HANDLE_INVALID; \
		} \
		return (*func_name##_ptr)(p_##arg1, p_##arg2); \
	}

#define EXT_PROTO_XRRESULT_FUNC3(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3) \
	static inline PFN_##func_name func_name##_ptr = nullptr; \
	static XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3) { \
		if (!func_name##_ptr) { \
			return XR_ERROR_HANDLE_INVALID; \
		} \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3); \
	}

#define EXT_PROTO_XRRESULT_FUNC4(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4) \
	static inline PFN_##func_name func_name##_ptr = nullptr; \
	static XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3, UNPACK arg4_type p_##arg4) { \
		if (!func_name##_ptr) { \
			return XR_ERROR_HANDLE_INVALID; \
		} \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3, p_##arg4); \
	}

#define EXT_PROTO_XRRESULT_FUNC5(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5) \
	static inline PFN_##func_name func_name##_ptr = nullptr; \
	static XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3, UNPACK arg4_type p_##arg4, UNPACK arg5_type p_##arg5) { \
		if (!func_name##_ptr) { \
			return XR_ERROR_HANDLE_INVALID; \
		} \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3, p_##arg4, p_##arg5); \
	}

#define EXT_PROTO_XRRESULT_FUNC6(func_name, arg1_type, arg1, arg2_type, arg2, arg3_type, arg3, arg4_type, arg4, arg5_type, arg5, arg6_type, arg6) \
	static inline PFN_##func_name func_name##_ptr = nullptr; \
	static XRAPI_ATTR XrResult XRAPI_CALL func_name(UNPACK arg1_type p_##arg1, UNPACK arg2_type p_##arg2, UNPACK arg3_type p_##arg3, UNPACK arg4_type p_##arg4, UNPACK arg5_type p_##arg5, UNPACK arg6_type p_##arg6) { \
		if (!func_name##_ptr) { \
			return XR_ERROR_HANDLE_INVALID; \
		} \
		return (*func_name##_ptr)(p_##arg1, p_##arg2, p_##arg3, p_##arg4, p_##arg5, p_##arg6); \
	}

#define XR_ENUM_CASE_STR(name, val) \
	case name: \
		return #name;
#define XR_ENUM_SWITCH(enumType, var) \
	switch (var) { \
		XR_LIST_ENUM_##enumType(XR_ENUM_CASE_STR) default : return "Unknown " #enumType ": " + String::num_int64(int64_t(var)); \
	}

///////////////////////////////////////////////////////////////////////////
// XrUuidEXT HashMap Helpers

struct HashMapHasherXrUuidEXT
{
	static _FORCE_INLINE_ uint32_t hash(const XrUuidEXT& p_uuid)
	{
		return hash_murmur3_buffer(p_uuid.data, XR_UUID_SIZE_EXT);
	}
};

template <>
struct HashMapComparatorDefault<XrUuidEXT>
{
	static bool compare(const XrUuidEXT& p_lhs, const XrUuidEXT& p_rhs)
	{
		for (int i = 0; i < XR_UUID_SIZE_EXT; i++) {
			if (p_lhs.data[i] != p_rhs.data[i]) {
				return false;
			}
		}
		return true;
	}
};

///////////////////////////////////////////////////////////////////////////
// OpenXRUtil

class OpenXRUtil final
{
public:
	static String get_result_string(XrResult p_result);
	static String get_view_configuration_name(XrViewConfigurationType p_view_configuration);
	static String get_reference_space_name(XrReferenceSpaceType p_reference_space);
	static String get_structure_type_name(XrStructureType p_structure_type);
	static String get_session_state_name(XrSessionState p_session_state);
	static String get_action_type_name(XrActionType p_action_type);
	static String get_environment_blend_mode_name(XrEnvironmentBlendMode p_blend_mode);
	static String make_xr_version_string(XrVersion p_version);
	static String get_handle_as_hex_string(void* p_handle);
	static String string_from_xruuid(const XrUuid& xr_uuid);
	static XrUuid xruuid_from_string(const String& p_uuid);

	// Copied from OpenXR xr_linear.h private header, so we can still link against
	// system-provided packages without relying on our `thirdparty` code.

	// Column-major, pre-multiplied. This type does not exist in the OpenXR API and is provided for convenience.
	typedef struct XrMatrix4x4f
	{
		float m[16];
	} XrMatrix4x4f;

	static void XrMatrix4x4f_CreateProjection(XrMatrix4x4f* result, const float tanAngleLeft,
		const float tanAngleRight, const float tanAngleUp, const float tanAngleDown,
		const float nearZ, const float farZ);
	static void XrMatrix4x4f_CreateProjectionFov(
		XrMatrix4x4f* result, const XrFovf fov, const float nearZ, const float farZ);

	OpenXRUtil() = delete;
	OpenXRUtil(const OpenXRUtil&) = delete;
	OpenXRUtil& operator=(const OpenXRUtil&) = delete;
	~OpenXRUtil() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRStructureBase

// Base class for XrStructureType based headers (with persistence config folded in)
class OpenXRStructureBase final : public RefCounted
{
public:
	bool has_valid_configuration() const;
	void* get_header(void* p_next = nullptr);
	XrStructureType get_structure_type();

	void set_next(const Ref<OpenXRStructureBase> p_next);
	Ref<OpenXRStructureBase> get_next() const;

	void add_persistence_context(RID p_persistence_context);
	void remove_persistence_context(RID p_persistence_context);

private:
	Ref<OpenXRStructureBase> next;
	Vector<RID> persistence_contexts;
	Vector<XrSpatialPersistenceContextEXT> context_handles;
	XrSpatialContextPersistenceConfigEXT persistence_config = {
		XR_TYPE_SPATIAL_CONTEXT_PERSISTENCE_CONFIG_EXT, nullptr, 0, nullptr};

	uint64_t _get_structure_type();
};

///////////////////////////////////////////////////////////////////////////
// OpenXRAPI

// Name we add to our extensions if OpenXR 1.1 context is available.
#define XR_OPENXR_1_1_NAME "OPENXR_1_1"

#define XR_API_VERSION_1_1_0 XR_MAKE_VERSION(1, 1, 0)

// forward declarations, we don't want to include these fully
class OpenXRInterface;
class OpenXRExtensionWrapper;
class OpenXRExtensionWrapperExtension;

class OpenXRAPI final
{
public:
	class OpenXRSwapChainInfo final
	{
	private:
		XrSwapchain swapchain = XR_NULL_HANDLE;
		void* swapchain_graphics_data = nullptr;
		uint32_t image_index = 0;
		bool image_acquired = false;
		bool skip_acquire_swapchain = false;

		static Vector<OpenXRSwapChainInfo> free_queue;

	public:
		_FORCE_INLINE_ XrSwapchain get_swapchain() const { return swapchain; }

		_FORCE_INLINE_ bool is_image_acquired() const { return image_acquired; }

		bool create(XrSwapchainCreateFlags p_create_flags, XrSwapchainUsageFlags p_usage_flags,
			int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height,
			uint32_t p_sample_count, uint32_t p_array_size, bool p_next_extensions = false);
		void queue_free();
		static void free_queued();
		void free();

		bool acquire(bool& p_should_render);
		bool release();
		RID get_image();
		RID get_density_map();
	};

private:
	enum OpenXRSwapChainTypes
	{
		OPENXR_SWAPCHAIN_COLOR,
		OPENXR_SWAPCHAIN_DEPTH,
		OPENXR_SWAPCHAIN_MAX
	};

	// When LOCAL_FLOOR isn't supported, we use an approach based on the example code in the
	// OpenXR spec in order to emulate it.
	// See: https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#XR_EXT_local_floor
	struct LocalFloorEmulation
	{
		bool enabled = false;
		XrSpace local_space = XR_NULL_HANDLE;
		XrSpace stage_space = XR_NULL_HANDLE;
		bool should_reset_floor_height = false;
	};

	static bool reset_emulated_floor_height();

	static bool load_layer_properties();
	static bool load_supported_extensions();
	static bool is_extension_supported(const String& p_extension);
	static bool is_any_extension_enabled(const String& p_extensions);

	struct RequestExtension
	{
		String name;
		bool* enabled;
	};

	static XrResult attempt_create_instance(XrVersion p_version);

	static bool openxr_loader_init();
	static bool resolve_instance_openxr_symbols();

	// function pointers
	EXT_PROTO_XRRESULT_FUNC3(xrAcquireSwapchainImage, (XrSwapchain), swapchain,
		(const XrSwapchainImageAcquireInfo*), acquireInfo, (uint32_t*), index)
	EXT_PROTO_XRRESULT_FUNC3(xrApplyHapticFeedback, (XrSession), session,
		(const XrHapticActionInfo*), hapticActionInfo, (const XrHapticBaseHeader*), hapticFeedback)
	EXT_PROTO_XRRESULT_FUNC2(xrAttachSessionActionSets, (XrSession), session,
		(const XrSessionActionSetsAttachInfo*), attachInfo)
	EXT_PROTO_XRRESULT_FUNC2(
		xrBeginFrame, (XrSession), session, (const XrFrameBeginInfo*), frameBeginInfo)
	EXT_PROTO_XRRESULT_FUNC2(
		xrBeginSession, (XrSession), session, (const XrSessionBeginInfo*), beginInfo)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateAction, (XrActionSet), actionSet, (const XrActionCreateInfo*),
		createInfo, (XrAction*), action)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateActionSet, (XrInstance), instance,
		(const XrActionSetCreateInfo*), createInfo, (XrActionSet*), actionSet)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateActionSpace, (XrSession), session,
		(const XrActionSpaceCreateInfo*), createInfo, (XrSpace*), space)
	EXT_PROTO_XRRESULT_FUNC2(
		xrCreateInstance, (const XrInstanceCreateInfo*), createInfo, (XrInstance*), instance)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateReferenceSpace, (XrSession), session,
		(const XrReferenceSpaceCreateInfo*), createInfo, (XrSpace*), space)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSession, (XrInstance), instance, (const XrSessionCreateInfo*),
		createInfo, (XrSession*), session)
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSwapchain, (XrSession), session,
		(const XrSwapchainCreateInfo*), createInfo, (XrSwapchain*), swapchain)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyAction, (XrAction), action)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyActionSet, (XrActionSet), actionSet)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyInstance, (XrInstance), instance)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySession, (XrSession), session)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpace, (XrSpace), space)
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySwapchain, (XrSwapchain), swapchain)
	EXT_PROTO_XRRESULT_FUNC2(
		xrEndFrame, (XrSession), session, (const XrFrameEndInfo*), frameEndInfo)
	EXT_PROTO_XRRESULT_FUNC1(xrEndSession, (XrSession), session)
	EXT_PROTO_XRRESULT_FUNC3(xrEnumerateApiLayerProperties, (uint32_t), propertyCapacityInput,
		(uint32_t*), propertyCountOutput, (XrApiLayerProperties*), properties)
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateEnvironmentBlendModes, (XrInstance), instance, (XrSystemId),
		systemId, (XrViewConfigurationType), viewConfigurationType, (uint32_t),
		environmentBlendModeCapacityInput, (uint32_t*), environmentBlendModeCountOutput,
		(XrEnvironmentBlendMode*), environmentBlendModes)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateInstanceExtensionProperties, (const char*), layerName,
		(uint32_t), propertyCapacityInput, (uint32_t*), propertyCountOutput,
		(XrExtensionProperties*), properties)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateReferenceSpaces, (XrSession), session, (uint32_t),
		spaceCapacityInput, (uint32_t*), spaceCountOutput, (XrReferenceSpaceType*), spaces)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainFormats, (XrSession), session, (uint32_t),
		formatCapacityInput, (uint32_t*), formatCountOutput, (int64_t*), formats)
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateViewConfigurations, (XrInstance), instance, (XrSystemId),
		systemId, (uint32_t), viewConfigurationTypeCapacityInput, (uint32_t*),
		viewConfigurationTypeCountOutput, (XrViewConfigurationType*), viewConfigurationTypes)
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateViewConfigurationViews, (XrInstance), instance,
		(XrSystemId), systemId, (XrViewConfigurationType), viewConfigurationType, (uint32_t),
		viewCapacityInput, (uint32_t*), viewCountOutput, (XrViewConfigurationView*), views)
	EXT_PROTO_XRRESULT_FUNC3(xrGetActionStateBoolean, (XrSession), session,
		(const XrActionStateGetInfo*), getInfo, (XrActionStateBoolean*), state)
	EXT_PROTO_XRRESULT_FUNC3(xrGetActionStateFloat, (XrSession), session,
		(const XrActionStateGetInfo*), getInfo, (XrActionStateFloat*), state)
	EXT_PROTO_XRRESULT_FUNC3(xrGetActionStateVector2f, (XrSession), session,
		(const XrActionStateGetInfo*), getInfo, (XrActionStateVector2f*), state)
	EXT_PROTO_XRRESULT_FUNC3(xrGetCurrentInteractionProfile, (XrSession), session, (XrPath),
		topLevelUserPath, (XrInteractionProfileState*), interactionProfile)
	EXT_PROTO_XRRESULT_FUNC2(xrGetInstanceProperties, (XrInstance), instance,
		(XrInstanceProperties*), instanceProperties)
	EXT_PROTO_XRRESULT_FUNC3(xrGetReferenceSpaceBoundsRect, (XrSession), session,
		(XrReferenceSpaceType), referenceSpaceType, (XrExtent2Df*), bounds)
	EXT_PROTO_XRRESULT_FUNC3(xrGetSystem, (XrInstance), instance, (const XrSystemGetInfo*), getInfo,
		(XrSystemId*), systemId)
	EXT_PROTO_XRRESULT_FUNC3(xrGetSystemProperties, (XrInstance), instance, (XrSystemId), systemId,
		(XrSystemProperties*), properties)
	EXT_PROTO_XRRESULT_FUNC4(xrLocateSpace, (XrSpace), space, (XrSpace), baseSpace, (XrTime), time,
		(XrSpaceLocation*), location)
	EXT_PROTO_XRRESULT_FUNC6(xrLocateViews, (XrSession), session, (const XrViewLocateInfo*),
		viewLocateInfo, (XrViewState*), viewState, (uint32_t), viewCapacityInput, (uint32_t*),
		viewCountOutput, (XrView*), views)
	EXT_PROTO_XRRESULT_FUNC5(xrPathToString, (XrInstance), instance, (XrPath), path, (uint32_t),
		bufferCapacityInput, (uint32_t*), bufferCountOutput, (char*), buffer)
	EXT_PROTO_XRRESULT_FUNC2(xrPollEvent, (XrInstance), instance, (XrEventDataBuffer*), eventData)
	EXT_PROTO_XRRESULT_FUNC2(xrReleaseSwapchainImage, (XrSwapchain), swapchain,
		(const XrSwapchainImageReleaseInfo*), releaseInfo)
	EXT_PROTO_XRRESULT_FUNC3(
		xrResultToString, (XrInstance), instance, (XrResult), value, (char*), buffer)
	EXT_PROTO_XRRESULT_FUNC3(
		xrStringToPath, (XrInstance), instance, (const char*), pathString, (XrPath*), path)
	EXT_PROTO_XRRESULT_FUNC2(xrSuggestInteractionProfileBindings, (XrInstance), instance,
		(const XrInteractionProfileSuggestedBinding*), suggestedBindings)
	EXT_PROTO_XRRESULT_FUNC2(
		xrSyncActions, (XrSession), session, (const XrActionsSyncInfo*), syncInfo)
	EXT_PROTO_XRRESULT_FUNC3(xrWaitFrame, (XrSession), session, (const XrFrameWaitInfo*),
		frameWaitInfo, (XrFrameState*), frameState)
	EXT_PROTO_XRRESULT_FUNC2(
		xrWaitSwapchainImage, (XrSwapchain), swapchain, (const XrSwapchainImageWaitInfo*), waitInfo)

	// instance
	static bool create_instance();
	static bool get_system_info();
	static bool load_supported_view_configuration_types();
	static bool load_supported_environmental_blend_modes();
	static bool is_view_configuration_supported(XrViewConfigurationType p_configuration_type);
	static bool load_supported_view_configuration_views(XrViewConfigurationType p_configuration_type);
	static void destroy_instance();

	// session
	static bool create_session();
	static bool load_supported_reference_spaces();
	static bool is_reference_space_supported(XrReferenceSpaceType p_reference_space);
	static bool setup_play_space();
	static bool setup_view_space();
	static bool load_supported_swapchain_formats();
	static bool is_swapchain_format_supported(int64_t p_swapchain_format);
	static bool obtain_swapchain_formats();
	static bool create_main_swapchains(const Size2i& p_size);
	static void free_main_swapchains();
	static void destroy_session();

	// action map
	struct Tracker
	{				 // Trackers represent tracked physical objects such as controllers, pucks, etc.
		String name; // Name for this tracker (i.e. "/user/hand/left")
		XrPath toplevel_path;	// OpenXR XrPath for this tracker
		RID active_profile_rid; // RID of the active profile for this tracker
	};
	static RID get_tracker_rid(XrPath p_path);

	struct ActionSet
	{						// Action sets define a set of actions that can be enabled together
		String name;		// Name for this action set (i.e. "godot_action_set")
		bool is_attached;	// If true our action set has been attached to the session and can no
							// longer be modified
		XrActionSet handle; // OpenXR handle for this action set
	};

	struct ActionTracker
	{							 // Links and action to a tracker
		RID tracker_rid;		 // RID of the tracker
		XrSpace space;			 // Optional space for pose actions
		bool was_location_valid; // If true the last position we obtained was valid
	};

	struct Action
	{									// Actions define the inputs and outputs in OpenXR
		RID action_set_rid;				// RID of the action set this action belongs to
		String name;					// Name for this action (i.e. "aim_pose")
		XrActionType action_type;		// Type of action (bool, float, etc.)
		Vector<ActionTracker> trackers; // The trackers this action can be used with
		XrAction handle;				// OpenXR handle for this action
	};
	static RID get_action_rid(XrAction p_action);

	struct InteractionProfile
	{ // Interaction profiles define suggested bindings between the physical inputs on controller
	  // types and our actions
		String name; // Name of the interaction profile (i.e.
					 // "/interaction_profiles/valve/index_controller")
		CharString
			internal_name; // Internal name of the interaction profile (translated if required)
		XrPath path;	   // OpenXR path for this profile
		Vector<XrActionSuggestedBinding> bindings; // OpenXR action bindings
		Vector<PackedByteArray>
			modifiers; // Array of modifiers we'll add into XrBindingModificationsKHR
	};
	static RID get_interaction_profile_rid(XrPath p_path);
	static XrPath get_interaction_profile_path(RID p_interaction_profile);

	static CharString get_interaction_profile_internal_name(
		const String& p_interaction_profile_name);
	static const char* check_profile_path(
		const CharString& p_interaction_profile_name, const char* p_path);

	struct OrderedCompositionLayer
	{
		const XrCompositionLayerBaseHeader* composition_layer;
		int sort_order;

		_FORCE_INLINE_ bool operator()(
			const OrderedCompositionLayer& a, const OrderedCompositionLayer& b) const
		{
			return a.sort_order < b.sort_order ||
				   (a.sort_order == b.sort_order &&
					   uint64_t(a.composition_layer) < uint64_t(b.composition_layer));
		}
	};

	// state changes
	static bool poll_events();
	static bool on_state_idle();
	static bool on_state_ready();
	static bool on_state_synchronized();
	static bool on_state_visible();
	static bool on_state_focused();
	static bool on_state_stopping();
	static bool on_state_loss_pending();
	static bool on_state_exiting();

	// convenience
	static void copy_string_to_char_buffer(const String& p_string, char* p_buffer, int p_buffer_len);

	// Render state, Only accessible in rendering thread
	struct RenderState
	{
		bool running = false;
		bool should_render = false;
		bool has_xr_viewport = false;
		XrTime predicted_display_time = 0;
		XrSpace play_space = XR_NULL_HANDLE;
		XrEnvironmentBlendMode environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		double render_target_size_multiplier = 1.0;
		uint64_t frame = 0;
		Rect2i render_region;

		LocalVector<XrView> views;
		LocalVector<XrCompositionLayerProjectionView> projection_views;
		LocalVector<XrCompositionLayerDepthInfoKHR>
			depth_views; // Only used by Composition Layer Depth Extension if available
		bool submit_depth_buffer = false;  // if set to true we submit depth buffers to OpenXR if a
										   // suitable extension is enabled.
		bool use_subsampled_images = true; // We need to default to true for the warning to be shown
										   // if we fallback immediately at startup.
		bool view_pose_valid = false;

		double z_near = 0.0;
		double z_far = 0.0;

		XrCompositionLayerProjection projection_layer = {
			XR_TYPE_COMPOSITION_LAYER_PROJECTION, // type
			nullptr,							  // next
			0,									  // layerFlags
			XR_NULL_HANDLE,						  // space
			0,									  // viewCount
			nullptr								  // views
		};

		Size2i main_swapchain_size;
		OpenXRSwapChainInfo main_swapchains[OPENXR_SWAPCHAIN_MAX];
	};

	static void _allocate_view_buffers_rt(uint32_t p_view_count, bool p_submit_depth_buffer);
	static void _set_render_session_running_rt(bool p_is_running);
	static void _set_render_display_info_rt(XrTime p_predicted_display_time, bool p_should_render);
	static void _set_render_play_space_rt(uint64_t p_play_space);
	static void _set_render_environment_blend_mode_rt(int32_t p_environment_blend_mode);
	static void _set_render_state_multiplier_rt(double p_render_target_size_multiplier);
	static void _set_render_state_render_region_rt(const Rect2i& p_render_region);
	static void _update_main_swapchain_size_rt();

	static void allocate_view_buffers(uint32_t p_view_count, bool p_submit_depth_buffer);
	static void set_render_session_running(bool p_is_running);
	static void set_render_display_info(XrTime p_predicted_display_time, bool p_should_render);
	static void set_render_play_space(XrSpace p_play_space);
	static void set_render_environment_blend_mode(XrEnvironmentBlendMode p_mode);
	static void set_render_state_multiplier(double p_render_target_size_multiplier);
	static void set_render_state_render_region(const Rect2i& p_render_region);

public:
	static void update_main_swapchain_size();

	static XrVersion get_openxr_version() { return data->openxr_version; }

	static XrInstance get_instance() { return data->instance; }

	static XrSystemId get_system_id() { return data->system_id; }

	static XrSession get_session() { return data->session; }

	static XrSessionState get_session_state() { return data->session_state; }

	static String get_runtime_name() { return data->runtime_name; }

	static String get_runtime_version() { return data->runtime_version; }

	static String get_system_name() { return data->system_name; }

	static uint32_t get_vendor_id() { return data->vendor_id; }

	// helper method to convert an XrPosef to a Transform3D
	static Transform3D transform_from_pose(const XrPosef& p_pose);
	static XrPosef pose_from_transform(const Transform3D& p_transform);

	// helper method to get a valid Transform3D from an openxr space location
	static XRPose::TrackingConfidence transform_from_location(
		const XrSpaceLocation& p_location, Transform3D& r_transform);
	static XRPose::TrackingConfidence transform_from_location(
		const XrHandJointLocationEXT& p_location, Transform3D& r_transform);
	static void parse_velocities(
		const XrSpaceVelocity& p_velocity, Vector3& r_linear_velocity, Vector3& r_angular_velocity);
	static XrPath get_xr_path(const String& p_path);
	static String get_xr_path_name(const XrPath& p_path);
	static bool is_top_level_path_supported(const String& p_toplevel_path);
	static bool is_interaction_profile_supported(const String& p_ip_path);
	static bool interaction_profile_supports_io_path(const String& p_ip_path, const String& p_io_path);

	static bool openxr_is_enabled(bool p_check_run_in_editor = true);

	static XrResult try_get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr);
	static XrResult get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr);
	static String get_error_string(XrResult result);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static void set_object_name(
		XrObjectType p_object_type, uint64_t p_object_handle, const String& p_object_name);
	static void begin_debug_label_region(const String& p_label_name);
	static void end_debug_label_region();
	static void insert_debug_label(const String& p_label_name);

	static OpenXRInterface* get_xr_interface() { return data->xr_interface; }

	static void set_xr_interface(OpenXRInterface* p_xr_interface);
	static void cleanup_extension_wrappers();
	static PackedStringArray get_all_requested_extensions(XrVersion p_xr_version);

	static void set_form_factor(XrFormFactor p_form_factor);

	static XrFormFactor get_form_factor() { return data->form_factor; }

	static uint32_t get_view_count();
	static void set_view_configuration(XrViewConfigurationType p_view_configuration);

	static XrViewConfigurationType get_view_configuration() { return data->view_configuration; }

	static bool set_requested_reference_space(XrReferenceSpaceType p_requested_reference_space);

	static XrReferenceSpaceType get_requested_reference_space() { return data->requested_reference_space; }

	static XrReferenceSpaceType get_reference_space() { return data->reference_space; }

	static void set_custom_play_space(XrSpace p_custom_space);

	static void set_submit_depth_buffer(bool p_submit_depth_buffer);

	static bool get_submit_depth_buffer() { return data->submit_depth_buffer; }

	static bool is_initialized();
	static bool is_running();
	static bool initialize(const String& p_rendering_driver);
	static bool initialize_session();
	static void finish();

	static _FORCE_INLINE_ XrSpace get_play_space() { return data->play_space; }

	static _FORCE_INLINE_ XrSpace get_view_space() { return data->view_space; }

	static _FORCE_INLINE_ XrTime get_predicted_display_time() { return data->frame_state.predictedDisplayTime; }

	static _FORCE_INLINE_ XrTime get_next_frame_time()
	{
		return data->frame_state.predictedDisplayTime + data->frame_state.predictedDisplayPeriod;
	}

	static _FORCE_INLINE_ bool can_render()
	{
		return data->instance != XR_NULL_HANDLE && data->session != XR_NULL_HANDLE && data->running &&
			   data->frame_state.shouldRender;
	}

	static XrHandTrackerEXT get_hand_tracker(int p_hand_index);

	static Size2 get_recommended_target_size();
	static XRPose::TrackingConfidence get_head_center(
		Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity);
	static bool get_view_transform(uint32_t p_view, Transform3D& r_transform);
	static bool get_view_projection(
		uint32_t p_view, double p_z_near, double p_z_far, Projection& p_camera_matrix);
	static Vector2 get_eye_focus(uint32_t p_view, float p_aspect);
	static bool process();

	static void pre_render();
	static bool pre_draw_viewport(RID p_render_target);
	static XrSwapchain get_color_swapchain();
	static RID get_color_texture();
	static RID get_depth_texture();
	static RID get_density_map_texture();
	static void set_velocity_texture(RID p_render_target);
	static RID get_velocity_texture();
	static void set_velocity_depth_texture(RID p_render_target);
	static RID get_velocity_depth_texture();
	static void set_velocity_target_size(const Size2i& p_target_size);
	static Size2i get_velocity_target_size();
	static const XrCompositionLayerProjection* get_projection_layer();
	static void post_draw_viewport(RID p_render_target);
	static void end_frame();

	// Display refresh rate
	static float get_display_refresh_rate();
	static void set_display_refresh_rate(float p_refresh_rate);

	// Render Target size multiplier
	static double get_render_target_size_multiplier();
	static void set_render_target_size_multiplier(double multiplier);

	static Rect2i get_render_region();
	static void set_render_region(const Rect2i& p_render_region);

	// Foveation settings
	static bool is_foveation_supported();

	static int get_foveation_level();
	static void set_foveation_level(int p_foveation_level);

	static bool get_foveation_dynamic();
	static void set_foveation_dynamic(bool p_foveation_dynamic);

	static bool get_foveation_with_subsampled_images();
	static void set_foveation_with_subsampled_images(bool p_enabled);

	// Play space.
	static Size2 get_play_space_bounds();

	// swapchains
	static PackedInt64Array get_supported_swapchain_formats();

	static int64_t get_color_swapchain_format() { return data->color_swapchain_format; }

	static int64_t get_depth_swapchain_format() { return data->depth_swapchain_format; }

	static double get_render_state_z_near() { return data->render_state.z_near; }

	static double get_render_state_z_far() { return data->render_state.z_far; }

	// action map
	static String get_default_action_map_resource_name();

	static RID tracker_create(const String& p_name);
	static String tracker_get_name(RID p_tracker);
	static void tracker_check_profile(RID p_tracker, XrSession p_session = XR_NULL_HANDLE);
	static void tracker_free(RID p_tracker);

	static RID action_set_create(
		const String& p_name, const String& p_localized_name, const int p_priority);
	static String action_set_get_name(RID p_action_set);
	static XrActionSet action_set_get_handle(RID p_action_set);
	static bool attach_action_sets(const Vector<RID>& p_action_sets);
	static void action_set_free(RID p_action_set);

	static RID action_create(RID p_action_set, const String& p_name, const String& p_localized_name,
		OpenXRAction::ActionType p_action_type, const Vector<RID>& p_trackers);
	static String action_get_name(RID p_action);
	static XrAction action_get_handle(RID p_action);
	static void action_free(RID p_action);

	static RID interaction_profile_create(const String& p_name);
	static String interaction_profile_get_name(RID p_interaction_profile);
	static void interaction_profile_clear_bindings(RID p_interaction_profile);
	static int interaction_profile_add_binding(
		RID p_interaction_profile, RID p_action, const String& p_path);
	static bool interaction_profile_add_modifier(
		RID p_interaction_profile, const PackedByteArray& p_modifier);
	static bool interaction_profile_suggest_bindings(RID p_interaction_profile);
	static void interaction_profile_free(RID p_interaction_profile);

	static RID find_tracker(const String& p_name);
	static RID find_action_set(const String& p_name);
	static RID find_action(const String& p_name, const RID& p_action_set = RID());

	static bool sync_action_sets(const Vector<RID>& p_active_sets);
	static bool get_action_bool(RID p_action, RID p_tracker);
	static float get_action_float(RID p_action, RID p_tracker);
	static Vector2 get_action_vector2(RID p_action, RID p_tracker);
	static XRPose::TrackingConfidence get_action_pose(RID p_action, RID p_tracker,
		Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity);
	static bool trigger_haptic_pulse(RID p_action, RID p_tracker, float p_frequency, float p_amplitude,
		XrDuration p_duration_ns);

	static const Vector<XrEnvironmentBlendMode> get_supported_environment_blend_modes();

	static bool is_environment_blend_mode_supported(XrEnvironmentBlendMode p_blend_mode);
	static bool set_environment_blend_mode(XrEnvironmentBlendMode p_blend_mode);

	static XrEnvironmentBlendMode get_environment_blend_mode()
	{
		return data->requested_environment_blend_mode;
	}

	enum OpenXRAlphaBlendModeSupport
	{
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE = 0,
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_REAL = 1,
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_EMULATING = 2,
	};

	static void set_emulate_environment_blend_mode_alpha_blend(bool p_enabled);
	static OpenXRAlphaBlendModeSupport is_environment_blend_mode_alpha_blend_supported();

public:
	struct Data
	{
#ifdef ANDROID_ENABLED
		void* openxr_loader_library_handle = nullptr;
		PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr = nullptr;
#endif
		OpenXRInterface* xr_interface = nullptr;
		LocalVector<XrApiLayerProperties> layer_properties;
		LocalVector<XrExtensionProperties> supported_extensions;
		LocalVector<CharString> enabled_extensions;
		LocalVector<XrViewConfigurationType> supported_view_configuration_types;
		LocalVector<XrReferenceSpaceType> supported_reference_spaces;
		PackedInt64Array supported_swapchain_formats;
		XrVersion openxr_version;
		String runtime_name;
		String runtime_version;
		XrFormFactor form_factor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		XrViewConfigurationType view_configuration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		XrReferenceSpaceType requested_reference_space = XR_REFERENCE_SPACE_TYPE_STAGE;
		XrReferenceSpaceType reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
		bool submit_depth_buffer = false;
		XrEnvironmentBlendMode environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		XrEnvironmentBlendMode requested_environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		Vector<XrEnvironmentBlendMode> supported_environment_blend_modes;
		bool emulate_environment_blend_mode_alpha_blend = false;
		XrInstance instance = XR_NULL_HANDLE;
		XrSystemId system_id = 0;
		String system_name;
		uint32_t vendor_id = 0;
		XrSystemTrackingProperties tracking_properties;
		XrSession session = XR_NULL_HANDLE;
		XrSessionState session_state = XR_SESSION_STATE_UNKNOWN;
		bool running = false;
		XrFrameState frame_state = {XR_TYPE_FRAME_STATE, nullptr, 0, 0, false};
		double render_target_size_multiplier = 1.0;
		Rect2i render_region;
		XrSystemGraphicsProperties graphics_properties;
		LocalVector<XrViewConfigurationView> view_configuration_views;
		int64_t color_swapchain_format = 0;
		int64_t depth_swapchain_format = 0;
		bool play_space_is_dirty = true;
		XrSpace play_space = XR_NULL_HANDLE;
		XrSpace custom_play_space = XR_NULL_HANDLE;
		XrSpace view_space = XR_NULL_HANDLE;
		XRPose::TrackingConfidence head_pose_confidence = XRPose::XR_TRACKING_CONFIDENCE_NONE;
		RID velocity_texture;
		RID velocity_depth_texture;
		Size2i velocity_target_size;
		LocalFloorEmulation local_floor_emulation;
		RID_Owner<Tracker, true> tracker_owner;
		bool interaction_profile_changed = true;
		RID_Owner<ActionSet, true> action_set_owner;
		RID_Owner<Action, true> action_owner;
		RID_Owner<InteractionProfile, true> interaction_profile_owner;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void setup();
	static void finalize();
	_FORCE_INLINE_ static bool is_allocated() { return data != nullptr; }

	OpenXRAPI() = delete;
	OpenXRAPI(const OpenXRAPI&) = delete;
	OpenXRAPI& operator=(const OpenXRAPI&) = delete;
	~OpenXRAPI() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRAPIExtension

class OpenXRAPIExtension final : public RefCounted
{
protected:
	_THREAD_SAFE_CLASS_

#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
	void _register_composition_layer_provider_bind_compat_104087(
		OpenXRExtensionWrapperExtension* p_extension);
	void _unregister_composition_layer_provider_bind_compat_104087(
		OpenXRExtensionWrapperExtension* p_extension);
	void _register_projection_views_extension_bind_compat_104087(
		OpenXRExtensionWrapperExtension* p_extension);
	void _unregister_projection_views_extension_bind_compat_104087(
		OpenXRExtensionWrapperExtension* p_extension);
#endif

public:
	uint64_t get_openxr_version();
	uint64_t get_instance();
	uint64_t get_system_id();
	uint64_t get_session();

	static bool openxr_is_enabled(bool p_check_run_in_editor = true);

	uint64_t get_instance_proc_addr(const String& p_name);
	String get_error_string(uint64_t result);
	String get_swapchain_format_name(int64_t p_swapchain_format);
	void set_object_name(
		int64_t p_object_type, uint64_t p_object_handle, const String& p_object_name);
	void begin_debug_label_region(const String& p_label_name);
	void end_debug_label_region();
	void insert_debug_label(const String& p_label_name);

	uint32_t get_view_count() const;
	uint64_t get_view_configuration() const;

	bool is_initialized();
	bool is_running();

	uint64_t get_play_space();
	int64_t get_predicted_display_time();
	int64_t get_next_frame_time();
	bool can_render();

	RID find_action(const String& p_name, const RID& p_action_set = RID());
	uint64_t action_get_handle(RID p_action);

	uint64_t get_hand_tracker(int p_hand_index);

	void register_composition_layer_provider(OpenXRExtensionWrapper* p_extension);
	void unregister_composition_layer_provider(OpenXRExtensionWrapper* p_extension);

	void register_projection_views_extension(OpenXRExtensionWrapper* p_extension);
	void unregister_projection_views_extension(OpenXRExtensionWrapper* p_extension);

	void register_frame_info_extension(OpenXRExtensionWrapper* p_extension);
	void unregister_frame_info_extension(OpenXRExtensionWrapper* p_extension);

	void register_projection_layer_extension(OpenXRExtensionWrapper* p_extension);
	void unregister_projection_layer_extension(OpenXRExtensionWrapper* p_extension);

	double get_render_state_z_near();
	double get_render_state_z_far();

	void set_velocity_texture(RID p_render_target);
	void set_velocity_depth_texture(RID p_render_target);
	void set_velocity_target_size(const Size2i& p_target_size);

	PackedInt64Array get_supported_swapchain_formats();

	uint64_t openxr_swapchain_create(XrSwapchainCreateFlags p_create_flags,
		XrSwapchainUsageFlags p_usage_flags, int64_t p_swapchain_format, uint32_t p_width,
		uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size);
	void openxr_swapchain_free(uint64_t p_swapchain_info);
	uint64_t openxr_swapchain_get_swapchain(uint64_t p_swapchain_info);
	void openxr_swapchain_acquire(uint64_t p_swapchain_info);
	RID openxr_swapchain_get_image(uint64_t p_swapchain_info);
	void openxr_swapchain_release(uint64_t p_swapchain_info);

	uint64_t get_projection_layer();

	void set_render_region(const Rect2i& p_render_region);

	void update_main_swapchain_size();

	enum OpenXRAlphaBlendModeSupport
	{
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE = 0,
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_REAL = 1,
		OPENXR_ALPHA_BLEND_MODE_SUPPORT_EMULATING = 2,
	};

	void set_emulate_environment_blend_mode_alpha_blend(bool p_enabled);
	OpenXRAlphaBlendModeSupport is_environment_blend_mode_alpha_blend_supported();

	OpenXRAPIExtension();
};
