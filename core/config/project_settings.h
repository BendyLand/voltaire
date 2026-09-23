/**************************************************************************/
/*  project_settings.h                                                    */
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

#pragma once

#include "core/os/thread_safe.h"
#include "core/templates/rb_map.h"
#include "core/types.h"

template <typename T> class TypedArray;

class ProjectSettings final
{
	static inline BinaryMutex _thread_safe_mutex;
	friend class TestProjectSettingsInternalsAccessor;

public:
	typedef HashMap<String, String> StringMap;
	// This constant is used to make the ".voltaire" folder and paths like "res://.voltaire/editor".
	static inline const String PROJECT_DATA_DIR_NAME_SUFFIX = "voltaire";
	static inline const String EDITOR_SETTING_OVERRIDE_PREFIX =
		PNAME("editor_overrides") + String("/");

	// Properties that are not for built in values begin from this value, so builtin ones are
	// displayed first.
	constexpr static const int32_t NO_BUILTIN_ORDER_BASE = 1 << 16;

#ifdef TOOLS_ENABLED
	const static Vector<String> get_required_features();
	const static Vector<String> get_unsupported_features(const Vector<String>& p_project_features);
#endif // TOOLS_ENABLED

	struct AutoloadInfo
	{
		StringName name;
		String path;
		bool is_singleton = false;
	};

private:
	static bool _property_can_revert(const StringName& p_name);

	static void _queue_changed(const StringName& p_name);
	static void _emit_changed();

	static Error _load_settings_text(const String& p_path);
	static Error _load_settings_binary(const String& p_path);
	static Error _load_settings_text_or_binary(const String& p_text_path, const String& p_bin_path);

	static Error _save_settings_text(const String& p_file, const RBMap<String, List<String>>& p_props,
		const StringMap& p_custom = StringMap(), const String& p_custom_features = String());
	static Error _save_settings_binary(const String& p_file, const RBMap<String, List<String>>& p_props,
		const StringMap& p_custom = StringMap(), const String& p_custom_features = String());

	static Error _save_custom_bnd(const String& p_file);

#ifdef TOOLS_ENABLED
	const static Vector<String> _get_supported_features();
	const static Vector<String> _trim_to_supported_features(
		const Vector<String>& p_project_features);
#endif // TOOLS_ENABLED

	static void _convert_to_last_version(int p_from_version);

	static bool load_resource_pack(const String& p_pack, bool p_replace_files, int p_offset);
	static bool _load_resource_pack(const String& p_pack, bool p_replace_files = true, int p_offset = 0,
		bool p_main_pack = false);

	static Error _setup(const String& p_path, const String& p_main_pack, bool p_upwards = false,
		bool p_ignore_override = false);

	static void _add_builtin_input_map();

private:
	struct Data
	{
		bool is_changed = false;

		// Starting version from 1 ensures that all callers can reset their tested version to 0,
		// and will always detect the initial project settings as a "change".
		uint32_t _version = 1;

		// Track changed settings for get_changed_settings functionality
		HashSet<StringName> changed_settings;

		int last_order = NO_BUILTIN_ORDER_BASE;
		int last_builtin_order = 0;
		uint64_t last_save_time = 0;

		String resource_path = "";
		bool using_datapack = false;
		bool project_loaded = false;
		List<String> input_presets;

		HashSet<String> custom_features;
		HashMap<StringName, LocalVector<Pair<StringName, StringName>>> feature_overrides;
		LocalVector<String> hidden_prefixes;
		HashMap<StringName, AutoloadInfo> autoloads;
		HashMap<StringName, String> global_groups;
		HashMap<StringName, HashSet<StringName>> scene_groups_cache;

		Vector<StringName> global_class_list;
		bool is_global_class_list_loaded = false;
		String project_data_dir_name;
	};


public:
	static const int CONFIG_VERSION = 5;
	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();

	static void refresh_global_class_list();
	static void store_global_class_list(const Vector<String>& p_classes);
	static String get_global_class_list_path();

	static bool is_initialized();
	static bool has_setting(const String& p_var);
	static String localize_path(const String& p_path);
	static String globalize_path(const String& p_path);

	static void set_as_basic(const String& p_name, bool p_basic);
	static void set_as_internal(const String& p_name, bool p_internal);
	static void set_restart_if_changed(const String& p_name, bool p_restart);
	static void set_ignore_value_in_docs(const String& p_name, bool p_ignore);
	static bool get_ignore_value_in_docs(const String& p_name);
	static void add_hidden_prefix(const String& p_prefix);

	static String get_project_data_dir_name();
	static String get_project_data_path();
	static String get_resource_path();
	static String get_imported_files_path();

	static void clear(const String& p_name);
	static int get_order(const String& p_name);
	static void set_order(const String& p_name, int p_order);
	static void set_builtin_order(const String& p_name);
	static bool is_builtin_setting(const String& p_name);

	static Error setup(const String& p_path, const String& p_main_pack, bool p_upwards = false,
		bool p_ignore_override = false);

	static Error load_custom(const String& p_path);
	static Error save_custom(const String& p_path = "", const StringMap& p_custom = StringMap(),
		const Vector<String>& p_custom_features = Vector<String>(),
		bool p_merge_with_current = true);
	static Error save();

	static uint64_t get_last_saved_time() { return data->last_save_time; }

	static List<String> get_input_presets() { return List<String>(data->input_presets); }

	static bool is_using_datapack();
	static bool is_project_loaded();

	static bool has_custom_feature(const String& p_feature);

	// Change tracking methods
	static Vector<String> get_changed_settings();
	static bool check_changed_settings_in_group(const String& p_setting_prefix);

	static const HashMap<StringName, AutoloadInfo>& get_autoload_list();
	static void add_autoload(const AutoloadInfo& p_autoload, bool p_front_insert = false);
	static void remove_autoload(const StringName& p_autoload);
	static bool has_autoload(const StringName& p_autoload);
	static AutoloadInfo get_autoload(const StringName& p_name);
	static void fix_autoload_paths();

	static const HashMap<StringName, String>& get_global_groups_list();
	static void add_global_group(const StringName& p_name, const String& p_description);
	static void remove_global_group(const StringName& p_name);
	static bool has_global_group(const StringName& p_name);

	static const HashMap<StringName, HashSet<StringName>>& get_scene_groups_cache();
	static void add_scene_groups_cache(const StringName& p_path, const HashSet<StringName>& p_cache);
	static void remove_scene_groups_cache(const StringName& p_path);
	static void save_scene_groups_cache();
	static String get_scene_groups_cache_path();
	static void load_scene_groups_cache();

	// Testing a version allows fast cached GET_GLOBAL macros.
	static uint32_t get_version() { return data->_version; }

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const StringName& p_function, int p_idx, List<String>* r_options);
#endif

	static bool has_editor_setting_override(const String& p_setting);

	ProjectSettings() = delete;
	ProjectSettings(const String& p_path) = delete;
	~ProjectSettings() = delete;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Cached versions of GLOBAL_GET.
// Cached but uses a typed variable for storage, this can be more efficient.
// Variables prefixed with _ggc_ to avoid shadowing warnings.
#define GLOBAL_GET_CACHED(m_type, m_setting_name)                                                  \
	([](const char* p_name) -> m_type {                                                            \
		static_assert(std::is_trivially_destructible<m_type>::value,                               \
			"GLOBAL_GET_CACHED must use a trivial type that allows static lifetime.");             \
		static m_type _ggc_local_var;                                                              \
		static uint32_t _ggc_local_version = 0;                                                    \
		static SpinLock _ggc_spin;                                                                 \
		uint32_t _ggc_new_version = ProjectSettings::get_version();                                \
		if (_ggc_local_version != _ggc_new_version) {                                              \
			_ggc_spin.lock();                                                                      \
			_ggc_local_version = _ggc_new_version;                                                 \
			m_type _ggc_temp = _ggc_local_var;                                                     \
			_ggc_spin.unlock();                                                                    \
			return _ggc_temp;                                                                      \
		}                                                                                          \
		_ggc_spin.lock();                                                                          \
		m_type _ggc_temp2 = _ggc_local_var;                                                        \
		_ggc_spin.unlock();                                                                        \
		return _ggc_temp2;                                                                         \
	})(m_setting_name)


