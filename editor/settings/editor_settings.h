/**************************************************************************/
/*  editor_settings.h                                                     */
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

#include "core/input/input_event.h"
#include "core/io/config_file.h"
#include "core/io/resource.h"
#include "core/os/os.h"

class EditorSettings final
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	struct Plugin
	{
		String path;
		String name;
		String author;
		String version;
		String description;
		bool installs = false;
		String script;
		Vector<String> install_files;
	};

	enum NetworkMode
	{
		NETWORK_OFFLINE,
		NETWORK_ONLINE,
	};

	// Keep values synced with DisplayServerEnums constants.
	enum InitialScreen
	{
		INITIAL_SCREEN_AUTO = -5, // Remembers last screen position.
		INITIAL_SCREEN_WITH_MOUSE_FOCUS = -4,
		INITIAL_SCREEN_WITH_KEYBOARD_FOCUS = -3,
		INITIAL_SCREEN_PRIMARY = -2,
	};

private:
	struct Data
	{
		HashSet<String> changed_settings;
		String auto_language;

		Ref<ConfigFile> project_metadata;
		bool project_metadata_dirty = false;
		int last_order = 0;

		Ref<Resource> clipboard;
		HashMap<String, Ref<Shortcut>> shortcuts;
		HashMap<String, List<InputEvent>> builtin_action_overrides;

		Vector<String> favorites;
		HashMap<String, PackedStringArray> favorite_properties;
		Vector<String> recent_dirs;

		bool save_changed_setting = true;
		bool optimize_save = true;
		bool initialized = false;
#ifndef DISABLE_DEPRECATED
		HashMap<String, String> compat_map;
#endif
	};

	static bool _property_can_revert(const String& p_name);

	static void _set_initialized();
	static void _load_defaults(Ref<ConfigFile> p_extra_config = Ref<ConfigFile>());
	static void _load_default_visual_shader_editor_theme();
	static String _guess_exec_args_for_extenal_editor(const String& p_value);
	static String _get_project_metadata_path();
#ifndef DISABLE_DEPRECATED
	static void _handle_setting_compatibility();
	static void _rename_setting(const String& p_old_name, const String& p_new_name);
	static void _rename_shortcut(const String& p_old_path, const String& p_new_path);
#endif

	// Bind helpers.
	static Vector<String> _get_shortcut_list();

public:
	enum
	{
		NOTIFICATION_EDITOR_SETTINGS_CHANGED = 10000
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static String get_existing_settings_path();
	static String get_newest_settings_path();

	static void init_shortcuts();
	static void setup_language(bool p_initial_setup);
	static void setup_network();
	static void save();
	static void set_optimize_save(bool p_optimize);

	static bool has_default_value(const String& p_setting);
	static bool has_setting(const String& p_setting);
	static void erase(const String& p_setting);
	static void raise_order(const String& p_setting);
	static void set_restart_if_changed(const String& p_setting, bool p_restart);
	static void set_basic(const String& p_setting, bool p_basic);

	static PackedStringArray get_changed_settings();
	static bool check_changed_settings_in_group(const String& p_setting_prefix);
	static void mark_setting_changed(const String& p_setting);

	static void set_resource_clipboard(const Ref<Resource>& p_resource)
	{
		if (data) {
			data->clipboard = p_resource;
		}
	}

	static Ref<Resource> get_resource_clipboard()
	{
		return data ? data->clipboard : Ref<Resource>();
	}

	static void save_project_metadata();

	static void set_favorites(const Vector<String>& p_favorites, bool p_update_file_dialog = true);
	static void set_favorites_bind(const Vector<String>& p_favorites);
	static Vector<String> get_favorites();
	static Vector<String> get_favorite_folders();
	static void set_favorite_properties(
		const HashMap<String, PackedStringArray>& p_favorite_properties);
	static HashMap<String, PackedStringArray> get_favorite_properties();
	static void set_recent_dirs(
		const Vector<String>& p_recent_dirs, bool p_update_file_dialog = true);
	static void set_recent_dirs_bind(const Vector<String>& p_recent_dirs);
	static Vector<String> get_recent_dirs();
	static void load_favorites_and_recent_dirs();

	static HashMap<String, Color> get_godot2_text_editor_theme();
	static bool is_default_text_editor_theme(const String& p_theme_name);
	static void update_text_editor_themes_list();

	static Vector<String> get_script_templates(
		const String& p_extension, const String& p_custom_path = String());
	static String get_editor_layouts_config();
	static float get_auto_display_scale();
	static String get_language();

	static void _add_shortcut_default(const String& p_path, const Ref<Shortcut>& p_shortcut);
	static void add_shortcut(const String& p_path, const Ref<Shortcut>& p_shortcut);
	static void remove_shortcut(const String& p_path);
	static bool is_shortcut(const String& p_path, const InputEvent& p_event);
	static bool has_shortcut(const String& p_path);
	static Ref<Shortcut> get_shortcut(const String& p_path);
	static void get_shortcut_list(List<String>* r_shortcuts);

	static void notify_changes();

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	EditorSettings() = delete;
	~EditorSettings() = delete;
};

#define EDITOR_DEF(m_var, m_val) _EDITOR_DEF(m_var, Variant(m_val))
#define EDITOR_DEF_RST(m_var, m_val) _EDITOR_DEF(m_var, Variant(m_val), true)
#define EDITOR_DEF_BASIC(m_var, m_val) _EDITOR_DEF(m_var, Variant(m_val), false, true)
#define ED_IS_SHORTCUT(p_name, p_ev) (EditorSettings::is_shortcut(p_name, p_ev))
Ref<Shortcut> ED_SHORTCUT(
	const String& p_path, const String& p_name, Key p_keycode = Key::NONE, bool p_physical = false);
Ref<Shortcut> ED_SHORTCUT_ARRAY(const String& p_path, const String& p_name,
	const PackedInt32Array& p_keycodes, bool p_physical = false);
void ED_SHORTCUT_OVERRIDE(const String& p_path, const String& p_feature, Key p_keycode = Key::NONE,
	bool p_physical = false);
void ED_SHORTCUT_OVERRIDE_ARRAY(const String& p_path, const String& p_feature,
	const PackedInt32Array& p_keycodes, bool p_physical = false);
Ref<Shortcut> ED_GET_SHORTCUT(const String& p_path);

// --- Folded from editor/translations/editor_translation.h ---
#include "core/string/ustring.h"
#include "core/templates/vector.h"

Vector<String> get_editor_locales();
void load_editor_translations(const String &p_locale);
void load_doc_translations(const String &p_locale);
Vector<Vector<String>> get_extractable_message_list();

// --- End editor/translations/editor_translation.h ---

// --- Folded from editor/file_system/editor_paths.h ---
#include "core/string/ustring.h"

class EditorPaths final
{
	struct Data
	{
		bool paths_valid = false; // If any of the paths can't be created, this is false.
		String data_dir;		  // Editor data (templates, shader cache, etc.).
		String config_dir;		  // Editor config (settings, profiles, themes, etc.).
		String cache_dir;		  // Editor cache (thumbnails, tmp generated files).
		String temp_dir;		  // Editor temporary directory.
		String project_data_dir;  // Project-specific data (metadata, shader cache, etc.).
		bool self_contained = false; // Self-contained means everything goes to `editor_data` dir.
		String self_contained_file;  // Self-contained file with configuration.
		String export_templates_folder = "export_templates";
		String text_editor_themes_folder = "text_editor_themes";
		String script_templates_folder = "script_templates";
		String feature_profiles_folder = "feature_profiles";
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static bool are_paths_valid();

	static String get_data_dir();
	static String get_config_dir();
	static String get_cache_dir();
	static String get_temp_dir();
	static String get_project_data_dir();
	static String get_export_templates_dir();
	static String get_debug_keystore_path();
	static String get_project_settings_dir();
	static String get_text_editor_themes_dir();
	static String get_script_templates_dir();
	static String get_project_script_templates_dir();
	static String get_feature_profiles_dir();

	static bool is_self_contained();
	static String get_self_contained_file();

	EditorPaths() = delete;
	EditorPaths(const EditorPaths&) = delete;
	EditorPaths& operator=(const EditorPaths&) = delete;
	~EditorPaths() = delete;
};

// --- Folded from editor/file_system/editor_file_system.h ---
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/os/os.h"
#include "core/templates/hash_set.h"
#include "core/templates/safe_refcount.h"
#include "scene/main/node.h"

class ResourceFormatImporter;
class FileAccess;

class EditorFileSystemDirectory
{
	String name;
	uint64_t modified_time;
	bool verified = false; // used for checking changes

	EditorFileSystemDirectory* parent = nullptr;
	Vector<EditorFileSystemDirectory*> subdirs;

	struct FileInfo
	{
		String file;
		String type;
		String resource_script_class; // If any resource has script with a global class name,
										  // its found here.
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		uint64_t modified_time = 0;
		uint64_t import_modified_time = 0;
		String import_md5;
		Vector<String> import_dest_paths;
		bool import_valid = false;
		String import_group_file;
		Vector<String> deps;
		bool verified = false; // used for checking changes

		// This is for script resources only.
		struct ScriptClassInfo
		{
			String name;
			String extends;
			String icon_path;
			bool is_abstract = false;
			bool is_tool = false;
		};

		ScriptClassInfo class_info;
	};

	Vector<FileInfo*> files;


	friend class EditorFileSystem;

public:
	String get_name();
	String get_path() const;

	int get_subdir_count() const;
	EditorFileSystemDirectory* get_subdir(int p_idx);
	int get_file_count() const;
	String get_file(int p_idx) const;
	String get_file_path(int p_idx) const;
	ResourceUID::ID get_file_uid(int p_idx) const;
	String get_file_type(int p_idx) const;
	String get_file_resource_script_class(int p_idx) const;
	Vector<String> get_file_deps(int p_idx) const;
	bool get_file_import_is_valid(int p_idx) const;
	uint64_t get_file_modified_time(int p_idx) const;
	uint64_t get_file_import_modified_time(int p_idx) const;
	String get_file_script_class_name(int p_idx) const;		 // used for scripts
	String get_file_script_class_extends(int p_idx) const;	 // used for scripts
	String get_file_script_class_icon_path(int p_idx) const; // used for scripts
	String get_file_icon_path(int p_idx) const;				 // used for FileSystemDock

	EditorFileSystemDirectory* get_parent();

	int find_file_index(const String& p_file) const;
	int find_dir_index(const String& p_dir) const;

	void force_update();

	EditorFileSystemDirectory();
	~EditorFileSystemDirectory();
};

class EditorFileSystem : public Node
{
	_THREAD_SAFE_CLASS_

	struct ItemAction
	{
		enum Action
		{
			ACTION_NONE,
			ACTION_DIR_ADD,
			ACTION_DIR_REMOVE,
			ACTION_FILE_ADD,
			ACTION_FILE_REMOVE,
			ACTION_FILE_TEST_REIMPORT,
			ACTION_FILE_RELOAD
		};

		Action action = ACTION_NONE;
		EditorFileSystemDirectory* dir = nullptr;
		String file;
		EditorFileSystemDirectory* new_dir = nullptr;
		EditorFileSystemDirectory::FileInfo* new_file = nullptr;
	};

	struct ScannedDirectory
	{
		String name;
		String full_path;
		Vector<ScannedDirectory*> subdirs;
		List<String> files;

		~ScannedDirectory();
	};

	bool is_case_sensitive = true;
	bool use_threads = false;
	Thread thread;

	EditorFileSystemDirectory* new_filesystem = nullptr;
	static ScannedDirectory* first_scan_root_dir;

	SafeFlag filesystem_changed_queued;
	bool scanning = false;
	bool importing = false;
	bool first_scan = true;
	bool scan_changes_pending = false;
	float scan_total;
	String filesystem_settings_version_for_import;
	bool revalidate_import_files = false;
	static int nb_files_total;

	static void _scan_for_uid_directory(
		const ScannedDirectory* p_scan_dir, const HashSet<String>& p_import_extensions);

	static void _load_first_scan_root_dir();

	HashSet<String> late_update_files;

	void _save_late_updated_files();

	EditorFileSystemDirectory* filesystem = nullptr;



	using ScriptClassInfo = EditorFileSystemDirectory::FileInfo::ScriptClassInfo;

	/* Used for reading the filesystem cache file */
	struct FileCache
	{
		String type;
		String resource_script_class;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		uint64_t modification_time = 0;
		uint64_t import_modification_time = 0;
		String import_md5;
		Vector<String> import_dest_paths;
		Vector<String> deps;
		bool import_valid = false;
		String import_group_file;
		ScriptClassInfo class_info;
	};

	HashMap<String, FileCache> file_cache;
	HashSet<String> dep_update_list;

	struct ScanProgress
	{
		float hi = 0;
		int current = 0;
		void increment();
	};

	struct DirectoryComparator
	{
		bool operator()(
			const EditorFileSystemDirectory* p_a, const EditorFileSystemDirectory* p_b) const
		{
			return p_a->name.filenocasecmp_to(p_b->name) < 0;
		}
	};

	void _save_filesystem_cache();
	void _save_filesystem_cache(EditorFileSystemDirectory* p_dir, Ref<FileAccess> p_file);

	bool _find_file(const String& p_file, EditorFileSystemDirectory** r_d, int& r_file_pos) const;

	void _delete_internal_files(const String& p_file);
	int _insert_actions_delete_files_directory(EditorFileSystemDirectory* p_dir);

	HashSet<String> textfile_extensions;
	HashSet<String> other_file_extensions;
	HashSet<String> valid_extensions;
	HashSet<String> import_extensions;

	static int _scan_new_dir(ScannedDirectory* p_dir, Ref<DirAccess>& da);

	Thread thread_sources;
	bool scanning_changes = false;
	SafeFlag scanning_changes_done;

	List<String> sources_changed;
	List<ItemAction> scan_actions;

	bool _is_test_for_reimport_needed(const String& p_path, uint64_t p_last_modification_time,
		uint64_t p_modification_time, uint64_t p_last_import_modification_time,
		uint64_t p_import_modification_time, const Vector<String>& p_import_dest_paths);
	bool _can_import_file(const String& p_path);

	bool reimport_on_missing_imported_files;

	Vector<String> _get_dependencies(const String& p_path);

	struct ImportFile
	{
		String path;
		String importer;
		bool threaded = false;
		int order = 0;

		bool operator<(const ImportFile& p_if) const
		{
			return order == p_if.order ? (importer < p_if.importer) : (order < p_if.order);
		}
	};

	struct ScriptClassInfoUpdate
	{
		ScriptClassInfo class_info;
		String type;
		ScriptClassInfoUpdate() = default;

		explicit ScriptClassInfoUpdate(const ScriptClassInfo& p_info) : class_info(p_info) {}

		static ScriptClassInfoUpdate from_file_info(const EditorFileSystemDirectory::FileInfo* p_fi)
		{
			ScriptClassInfoUpdate update(p_fi->class_info);
			update.type = p_fi->type;
			return update;
		}
	};

	Mutex update_script_mutex;
	HashMap<String, ScriptClassInfoUpdate> update_script_paths;
	HashSet<String> update_script_paths_documentation;
	void _queue_update_script_class(
		const String& p_path, const ScriptClassInfoUpdate& p_script_update);
	void _process_removed_files(const HashSet<String>& p_processed_files);

	Mutex update_scene_mutex;
	HashSet<String> update_scene_paths;
	void _queue_update_scene_groups(const String& p_path);
	void _get_all_scenes(EditorFileSystemDirectory* p_dir, HashSet<String>& r_list);

	bool using_fat32_or_exfat; // Workaround for projects in FAT32 or exFAT filesystem (pendrives,
							   // most of the time)

	void _find_group_files(EditorFileSystemDirectory* efd,
		HashMap<String, Vector<String>>& group_files, HashSet<String>& groups_to_reimport);

	HashSet<String> group_file_cache;
	HashMap<String, String> file_icon_cache;

	bool refresh_queued = false;

	struct ImportThreadData
	{
		const ImportFile* reimport_files;
		int reimport_from;
		Semaphore* imported_sem = nullptr;
	};

	static ResourceUID::ID _resource_saver_get_resource_id_for_path(
		const String& p_path, bool p_generate);

	bool _scan_extensions();

	String _get_file_by_class_name(EditorFileSystemDirectory* p_dir, const String& p_class_name,
		EditorFileSystemDirectory::FileInfo*& r_file_info);

public:
	static inline EditorFileSystem* instance = nullptr;
	_FORCE_INLINE_ static bool is_initialized() { return instance != nullptr; }

	EditorFileSystemDirectory* _impl_get_filesystem();
	static EditorFileSystemDirectory* get_filesystem();
	bool _impl_is_scanning() const;
	static bool is_scanning();

	bool is_importing() const { return importing; }

	bool _impl_doing_first_scan() const { return first_scan; }
	static bool doing_first_scan() { return instance->_impl_doing_first_scan(); }

	float get_scanning_progress() const;
	HashSet<String> get_valid_extensions() const;

	EditorFileSystemDirectory* _impl_get_filesystem_path(const String& p_path);
	static EditorFileSystemDirectory* get_filesystem_path(const String& p_path);
	String _impl_get_file_type(const String& p_file) const;
	static String get_file_type(const String& p_file);
	EditorFileSystemDirectory* _impl_find_file(const String& p_file, int* r_index) const;
	static EditorFileSystemDirectory* find_file(const String& p_file, int* r_index);
	ResourceUID::ID _impl_get_file_uid(const String& p_path) const;
	static ResourceUID::ID get_file_uid(const String& p_path);

	bool is_group_file(const String& p_path) const;

	static bool _should_skip_directory(const String& p_path);

	static void scan_for_uid();

	EditorFileSystem() = default;
	~EditorFileSystem();
};

// --- Folded from editor/themes/editor_scale.h ---
class EditorScale {
	static float _scale;

public:
	static void set_scale(float p_scale);
	static float get_scale();
};

#define EDSCALE (EditorScale::get_scale())
// --- End editor/file_system/editor_paths.h ---
