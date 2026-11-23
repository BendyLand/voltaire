/**************************************************************************/
/*  resource_loader.h                                                     */
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

#include "core/io/resource.h"
// --- Folded from core/io/resource_loader_constants.h ---
namespace ResourceLoaderConstants
{

enum CacheMode
{
	CACHE_MODE_IGNORE,
	CACHE_MODE_REUSE,
	CACHE_MODE_REPLACE,
	CACHE_MODE_IGNORE_DEEP,
	CACHE_MODE_REPLACE_DEEP,
};

} // namespace ResourceLoaderConstants



// --- End core/io/resource_loader_constants.h ---
#include "core/os/thread.h"

namespace CoreBind
{
class ResourceLoader;
}

class ConditionVariable;

template <int Tag> class SafeBinaryMutex;

class ResourceFormatLoader : public RefCounted
{
public:
	using CacheMode = ResourceLoaderConstants::CacheMode;
	static constexpr CacheMode CACHE_MODE_IGNORE = ResourceLoaderConstants::CACHE_MODE_IGNORE;
	static constexpr CacheMode CACHE_MODE_REUSE = ResourceLoaderConstants::CACHE_MODE_REUSE;
	static constexpr CacheMode CACHE_MODE_REPLACE = ResourceLoaderConstants::CACHE_MODE_REPLACE;
	static constexpr CacheMode CACHE_MODE_IGNORE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_IGNORE_DEEP;
	static constexpr CacheMode CACHE_MODE_REPLACE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_REPLACE_DEEP;

public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE);
	virtual bool exists(const String& p_path) const;
	virtual void get_recognized_extensions(List<String>* p_extensions) const;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const;
	virtual bool recognize_path(const String& p_path, const String& p_for_type = String()) const;
	virtual bool handles_type(const String& p_type) const;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	virtual String get_resource_type(const String& p_path) const;
	virtual String get_resource_script_class(const String& p_path) const;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const;
	virtual bool has_custom_uid_support() const;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);
	virtual Error rename_dependencies(const String& p_path, const HashMap<String, String>& p_map);

	virtual bool is_import_valid(const String& p_path) const { return true; }

	virtual bool is_imported(const String& p_path) const { return false; }

	virtual int get_import_order(const String& p_path) const { return 0; }

	virtual String get_import_group_file(const String& p_path) const { return ""; } // no group

	virtual ~ResourceFormatLoader() = default;
};

typedef void (*ResourceLoadErrorNotify)(const String& p_text);
typedef void (*DependencyErrorNotify)(
	const String& p_loading, const String& p_which, const String& p_type);

typedef Error (*ResourceLoaderImport)(const String& p_path);
typedef void (*ResourceLoadedCallback)(Ref<Resource> p_resource, const String& p_path);

class ResourceLoader
{
	friend class LoadToken;
	friend class CoreBind::ResourceLoader;

	enum
	{
		MAX_LOADERS = 64
	};

	struct ThreadLoadTask;

public:
	using CacheMode = ResourceLoaderConstants::CacheMode;
	static constexpr CacheMode CACHE_MODE_IGNORE = ResourceLoaderConstants::CACHE_MODE_IGNORE;
	static constexpr CacheMode CACHE_MODE_REUSE = ResourceLoaderConstants::CACHE_MODE_REUSE;
	static constexpr CacheMode CACHE_MODE_REPLACE = ResourceLoaderConstants::CACHE_MODE_REPLACE;
	static constexpr CacheMode CACHE_MODE_IGNORE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_IGNORE_DEEP;
	static constexpr CacheMode CACHE_MODE_REPLACE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_REPLACE_DEEP;

	enum ThreadLoadStatus
	{
		THREAD_LOAD_INVALID_RESOURCE,
		THREAD_LOAD_IN_PROGRESS,
		THREAD_LOAD_FAILED,
		THREAD_LOAD_LOADED
	};

	enum LoadThreadMode
	{
		LOAD_THREAD_FROM_CURRENT,
		LOAD_THREAD_SPAWN_SINGLE,
		LOAD_THREAD_DISTRIBUTE,
	};

	struct LoadToken : public RefCounted
	{
		String local_path;
		String user_path;
		uint32_t user_rc = 0; // Having user RC implies regular RC incremented in one, until the
							  // user RC reaches zero.
		ThreadLoadTask* task_if_unregistered = nullptr;

		void clear();

		virtual ~LoadToken();
	};

	static const int BINARY_MUTEX_TAG = 1;

	static Ref<LoadToken> _load_start(const String& p_path, const String& p_type_hint,
		LoadThreadMode p_thread_mode, CacheMode p_cache_mode, bool p_for_user = false);
	static Ref<Resource> _load_complete(LoadToken& p_load_token, Error* r_error);

private:
	static LoadToken* _load_threaded_request_reuse_user_token(const String& p_path);
	static void _load_threaded_request_setup_user_token(LoadToken* p_token, const String& p_path);

	static Ref<Resource> _load_complete_inner(LoadToken& p_load_token, Error* r_error,
		MutexLock<SafeBinaryMutex<BINARY_MUTEX_TAG>>& p_thread_load_lock);

	static Ref<ResourceFormatLoader> loader[MAX_LOADERS];
	static int loader_count;
	static bool timestamp_on_load;

	static void* err_notify_ud;
	static ResourceLoadErrorNotify err_notify;
	static void* dep_err_notify_ud;
	static DependencyErrorNotify dep_err_notify;
	static bool abort_on_missing_resource;
	static bool create_missing_resources_if_class_unavailable;
	static HashMap<String, Vector<String>> translation_remaps;

	static String _path_remap(const String& p_path, bool* r_translation_remapped = nullptr);
	friend class Resource;

	static SelfList<Resource>::List remapped_list;

	friend class ResourceFormatImporter;

	static Ref<Resource> _load(const String& p_path, const String& p_original_path,
		const String& p_type_hint, CacheMode p_cache_mode, Error* r_error, bool p_use_sub_threads,
		float* r_progress);

	static ResourceLoadedCallback _loaded_callback;

	static Ref<ResourceFormatLoader> _find_custom_resource_format_loader(const String& p_path);

	struct ThreadLoadTask
	{
		Thread::ID thread_id =
			0; // Used if running on an user thread (e.g., simple non-threaded load).
		int thread_index = -1;
		ConditionVariable* cond_var = nullptr; // In not in the worker pool or already awaiting,
											   // this is used as a secondary awaiting mechanism.
		uint32_t awaiters_count = 0;
		LoadToken* load_token = nullptr;
		String local_path;
		String type_hint;
		float progress = 0.0f;
		float max_reported_progress = 0.0f;
		uint64_t last_progress_check_main_thread_frame = UINT64_MAX;
		ThreadLoadStatus status = THREAD_LOAD_IN_PROGRESS;
		CacheMode cache_mode = CACHE_MODE_REUSE;
		Error error = OK;
		Ref<Resource> resource;
		LocalVector<Ref<Resource>> resource_dependencies; // We need to keep these alive for as long
														  // as the task is alive at least.
		ThreadLoadTask* parent_task = nullptr;
		HashSet<String> sub_tasks;

		bool awaited : 1; // If it's in the pool, this helps not awaiting from more than one
						  // dependent thread.
		bool need_wait : 1;
		bool in_progress_check : 1; // Measure against recursion cycles in progress reporting.
									// Cycles are not expected, but can happen due to how it's
									// currently implemented.
		bool use_sub_threads : 1;
		bool started_load : 1;
		bool finished_load : 1;
		bool connections_propagated : 1;

		struct ResourceChangedConnection
		{
			Resource* source = nullptr;
			uint32_t flags = 0;
		};

		LocalVector<ResourceChangedConnection> resource_changed_connections;

		ThreadLoadTask()
			: awaited(false), need_wait(true), in_progress_check(false), use_sub_threads(false),
			  started_load(false), finished_load(false), connections_propagated(false)
		{
		}
	};

	static void _run_load_task(void* p_userdata);

	static thread_local bool import_thread;
	static thread_local int load_nesting;
	static thread_local HashMap<int, HashMap<String, Ref<Resource>>>
		res_ref_overrides; // Outermost key is nesting level.
	static thread_local ThreadLoadTask* curr_load_task;

	static SafeBinaryMutex<BINARY_MUTEX_TAG> thread_load_mutex;
	friend SafeBinaryMutex<BINARY_MUTEX_TAG>& _get_res_loader_mutex();

	static HashMap<String, ThreadLoadTask> thread_load_tasks;
	static HashMap<int, String> thread_waiting_on;
	static LocalVector<int> yielders;
	static bool cleaning_tasks;

	static HashMap<String, LoadToken*> user_load_tokens;

	static float _dependency_get_progress(const String& p_path);

	static bool _ensure_load_progress();

	static String _validate_local_path(const String& p_path);

public:
	static Error load_threaded_request(const String& p_path, const String& p_type_hint = "",
		bool p_use_sub_threads = false, CacheMode p_cache_mode = CACHE_MODE_REUSE);
	static ThreadLoadStatus load_threaded_get_status(
		const String& p_path, float* r_progress = nullptr);
	static Ref<Resource> load_threaded_get(const String& p_path, Error* r_error = nullptr);

	static bool is_within_load() { return load_nesting > 0; }

	static void resource_changed_emit(Resource* p_source);

	static Ref<Resource> load(const String& p_path, const String& p_type_hint = "",
		CacheMode p_cache_mode = CACHE_MODE_REUSE, Error* r_error = nullptr);
	static bool exists(const String& p_path, const String& p_type_hint = "");

	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static void add_resource_format_loader(
		Ref<ResourceFormatLoader> p_format_loader, bool p_at_front = false);
	static void remove_resource_format_loader(Ref<ResourceFormatLoader> p_format_loader);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool should_create_uid_file(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);
	static Error rename_dependencies(const String& p_path, const HashMap<String, String>& p_map);
	static bool is_import_valid(const String& p_path);
	static String get_import_group_file(const String& p_path);
	static bool is_imported(const String& p_path);

	static void set_is_import_thread(bool p_import_thread);

	static void set_timestamp_on_load(bool p_timestamp) { timestamp_on_load = p_timestamp; }

	static bool get_timestamp_on_load() { return timestamp_on_load; }

	// Loaders can safely use this regardless which thread they are running on.
	static void notify_load_error(const String& p_err);

	static void set_error_notify_func(ResourceLoadErrorNotify p_err_notify)
	{
		err_notify = p_err_notify;
	}

	// Loaders can safely use this regardless which thread they are running on.
	static void notify_dependency_error(
		const String& p_path, const String& p_dependency, const String& p_type);

	static void set_dependency_error_notify_func(DependencyErrorNotify p_err_notify)
	{
		dep_err_notify = p_err_notify;
	}

	static void set_abort_on_missing_resources(bool p_abort)
	{
		abort_on_missing_resource = p_abort;
	}

	static bool get_abort_on_missing_resources() { return abort_on_missing_resource; }

	static String path_remap(const String& p_path);
	static String import_remap(const String& p_path);

	static void reload_translation_remaps();
	static void load_translation_remaps();
	static void clear_translation_remaps();

	static void clear_thread_load_tasks();

	static void set_load_callback(ResourceLoadedCallback p_callback);
	static ResourceLoaderImport import;

	static bool add_custom_resource_format_loader(const String& p_script_path);
	static void add_custom_loaders();
	static void remove_custom_loaders();

	static void set_create_missing_resources_if_class_unavailable(bool p_enable);

	_FORCE_INLINE_ static bool is_creating_missing_resources_if_class_unavailable_enabled()
	{
		return create_missing_resources_if_class_unavailable;
	}

	static Ref<Resource> ensure_resource_ref_override_for_outer_load(
		const String& p_path, const String& p_res_type);
	static Ref<Resource> get_resource_ref_override(const String& p_path);

	static bool is_cleaning_tasks();

	static Vector<String> list_directory(const String& p_directory);

	static void initialize();
	static void finalize();
};

// --- Folded from core/io/image_resource_format.h ---
// --- Folded from core/io/image_loader.h ---
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/string/ustring.h"
#include "core/templates/list.h"

class ImageLoader;

class ImageFormatLoader : public RefCounted
{
	friend class ImageLoader;
	friend class ResourceFormatLoaderImage;

public:
	enum LoaderFlags
	{
		FLAG_NONE = 0,
		FLAG_FORCE_LINEAR = 1,
		FLAG_CONVERT_COLORS = 2,
	};

protected:

	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
		uint32_t p_flags = FLAG_NONE, float p_scale = 1.0) = 0;
	virtual void get_recognized_extensions(List<String>* p_extensions) const = 0;
	bool recognize(const String& p_extension) const;

public:
	virtual ~ImageFormatLoader() {}
};

class ImageFormatLoaderExtension : public ImageFormatLoader
{
protected:

public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
		uint32_t p_flags = FLAG_NONE, float p_scale = 1.0) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;

	void add_format_loader();
	void remove_format_loader();
};

class ImageLoader
{
	static inline Vector<Ref<ImageFormatLoader>> loader;
	friend class ResourceFormatLoaderImage;

protected:
public:
	static Error load_image(const String& p_file, Ref<Image> p_image,
		Ref<FileAccess> p_custom = Ref<FileAccess>(),
		uint32_t p_flags = ImageFormatLoader::FLAG_NONE,
		float p_scale = 1.0);
	static void get_recognized_extensions(List<String>* p_extensions);
	static Ref<ImageFormatLoader> recognize(const String& p_extension);

	static void add_image_format_loader(Ref<ImageFormatLoader> p_loader);
	static void remove_image_format_loader(Ref<ImageFormatLoader> p_loader);

	static void cleanup();
};

// --- Folded from modules/svg/image_loader_svg.h ---
class ImageLoaderSVG : public ImageFormatLoader {
	static HashMap<Color, Color> forced_color_map;

	static void _replace_color_property(const HashMap<Color, Color> &p_color_map, const String &p_prefix, String &r_string);

	static Ref<Image> load_mem_svg(const uint8_t *p_svg, int p_size, float p_scale);

public:
	static void set_forced_color_map(const HashMap<Color, Color> &p_color_map);

	static Error create_image_from_utf8_buffer(Ref<Image> p_image, const uint8_t *p_buffer, int p_buffer_size, float p_scale, bool p_upsample);
	static Error create_image_from_utf8_buffer(Ref<Image> p_image, const PackedByteArray &p_buffer, float p_scale, bool p_upsample);

	static Error create_image_from_string(Ref<Image> p_image, String p_string, float p_scale, bool p_upsample, const HashMap<Color, Color> &p_color_map);

	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess, uint32_t p_flags, float p_scale) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;

	ImageLoaderSVG();
};
// --- End core/io/image_loader.h ---


class ResourceFormatLoaderImage : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};



// --- End core/io/image_resource_format.h ---

// --- Folded from core/io/resource_format_binary.h ---
#include "core/io/file_access.h"

// --- Folded from core/io/resource_saver.h ---
#include "core/io/resource.h"

class ResourceFormatSaver : public RefCounted
{
protected:

public:
	virtual Error save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0);
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid);
	virtual bool recognize(const Ref<Resource>& p_resource) const;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const;
	virtual bool recognize_path(const Ref<Resource>& p_resource, const String& p_path) const;

	virtual ~ResourceFormatSaver() {}
};

typedef void (*ResourceSavedCallback)(Ref<Resource> p_resource, const String& p_path);
typedef ResourceUID::ID (*ResourceSaverGetResourceIDForPath)(const String& p_path, bool p_generate);

class ResourceSaver
{
	enum
	{
		MAX_SAVERS = 64
	};

	static Ref<ResourceFormatSaver> saver[MAX_SAVERS];
	static int saver_count;
	static bool timestamp_on_save;
	static ResourceSavedCallback save_callback;
	static ResourceSaverGetResourceIDForPath save_get_id_for_path;

	static Ref<ResourceFormatSaver> _find_custom_resource_format_saver(const String& p_path);

public:
	enum SaverFlags
	{
		FLAG_NONE = 0,
		FLAG_RELATIVE_PATHS = 1,
		FLAG_BUNDLE_RESOURCES = 2,
		FLAG_CHANGE_PATH = 4,
		FLAG_OMIT_EDITOR_PROPERTIES = 8,
		FLAG_SAVE_BIG_ENDIAN = 16,
		FLAG_COMPRESS = 32,
		FLAG_REPLACE_SUBRESOURCE_PATHS = 64,
	};

	static Error save(
		Resource* rp_resource, const String& p_path = "", uint32_t p_flags = (uint32_t)FLAG_NONE);
	static void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions);
	static void add_resource_format_saver(
		Ref<ResourceFormatSaver> p_format_saver, bool p_at_front = false);
	static void remove_resource_format_saver(Ref<ResourceFormatSaver> p_format_saver);

	static Error set_uid(const String& p_path, ResourceUID::ID p_uid);

	static void set_timestamp_on_save(bool p_timestamp) { timestamp_on_save = p_timestamp; }

	static bool get_timestamp_on_save() { return timestamp_on_save; }

	static ResourceUID::ID get_resource_id_for_path(const String& p_path, bool p_generate = false);

	static void set_save_callback(ResourceSavedCallback p_callback);
	static void set_get_resource_id_for_path(ResourceSaverGetResourceIDForPath p_callback);

	static bool add_custom_resource_format_saver(const String& p_script_path);
	static void add_custom_savers();
	static void remove_custom_savers();
};
// --- End core/io/resource_saver.h ---
#include "core/templates/rb_map.h"

class ResourceLoaderBinary
{
	bool translation_remapped = false;
	String local_path;
	String res_path;
	String type;
	Ref<Resource> resource;
	uint32_t ver_format = 0;

	Ref<FileAccess> f;

	uint64_t importmd_ofs = 0;

	ResourceUID::ID uid = ResourceUID::INVALID_ID;

	Vector<char> str_buf;
	List<Ref<Resource>> resource_cache;

	Vector<String> string_map;

	String _get_string();

	struct ExtResource
	{
		String path;
		String type;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		Ref<ResourceLoader::LoadToken> load_token;
	};

	bool using_named_scene_ids = false;
	bool using_uids = false;
	String script_class;
	bool use_sub_threads = false;
	float* progress = nullptr;
	Vector<ExtResource> external_resources;

	struct IntResource
	{
		String path;
		uint64_t offset;
	};

	Vector<IntResource> internal_resources;
	HashMap<String, Ref<Resource>> internal_index_cache;

	String get_unicode_string();
	void _advance_padding(uint32_t p_len);

	HashMap<String, String> remaps;
	Error error = OK;

	ResourceFormatLoader::CacheMode cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
	ResourceFormatLoader::CacheMode cache_mode_for_external =
		ResourceFormatLoader::CACHE_MODE_REUSE;

	friend class ResourceFormatLoaderBinary;

	HashMap<String, Ref<Resource>> dependency_cache;

public:
	Ref<Resource> get_resource();
	Error load();
	void set_translation_remapped(bool p_remapped);

	void set_remaps(const HashMap<String, String>& p_remaps) { remaps = p_remaps; }

	void open(Ref<FileAccess> p_file, bool p_no_resources = false, bool p_keep_uuid_paths = false);
	String recognize(Ref<FileAccess> p_file);
	String recognize_script_class(Ref<FileAccess> p_file);
	void get_dependencies(Ref<FileAccess> p_file, List<String>* p_dependencies, bool p_add_types);
	void get_classes_used(Ref<FileAccess> p_file, HashSet<String>* p_classes);
};

class ResourceFormatLoaderBinary : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
	virtual String get_resource_script_class(const String& p_path) const override;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override;
	virtual bool has_custom_uid_support() const override;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override;
	virtual Error rename_dependencies(
		const String& p_path, const HashMap<String, String>& p_map) override;
};

class ResourceFormatSaverBinaryInstance
{
	String local_path;
	String path;

	bool relative_paths;
	bool bundle_resources;
	bool skip_editor;
	bool big_endian;
	bool takeover_paths;
	String magic;
	HashSet<Ref<Resource>> resource_set;

	struct NonPersistentKey
	{ // for resource properties generated on the fly
		Ref<Resource> base;
		String property;

		bool operator<(const NonPersistentKey& p_key) const
		{
			return base == p_key.base ? property < p_key.property : base < p_key.base;
		}
	};

	HashMap<String, int> string_map;
	Vector<String> strings;

	HashMap<Ref<Resource>, int> external_resources;
	List<Ref<Resource>> saved_resources;

	struct Property
	{
		int name_idx;
	};

	struct ResourceData
	{
		String type;
		List<Property> properties;
	};

	static void _pad_buffer(Ref<FileAccess> r_file, int p_bytes);
	static void save_unicode_string(
		Ref<FileAccess> r_file, const String& p_string, bool p_bit_on_len = false);
	int get_string_index(const String& p_string);

public:
	enum
	{
		FORMAT_FLAG_NAMED_SCENE_IDS = 1,
		FORMAT_FLAG_UIDS = 2,
		FORMAT_FLAG_REAL_T_IS_DOUBLE = 4,
		FORMAT_FLAG_HAS_SCRIPT_CLASS = 8,

		// Amount of reserved 32-bit fields in resource header
		RESERVED_FIELDS = 11
	};

	Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);
	Error set_uid(const String& p_path, ResourceUID::ID p_uid);
};

class ResourceFormatSaverBinary : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceFormatSaverBinary() = default;
};



// --- End core/io/resource_format_binary.h ---

// --- Folded from core/io/missing_resource.h ---
#include "core/io/resource.h"

#define META_PROPERTY_MISSING_RESOURCES "metadata/_missing_resources"
#define META_MISSING_RESOURCES "_missing_resources"

class MissingResource : public Resource
{
	String original_class;
	bool recording_properties = false;

protected:

public:
	void set_original_class(const String& p_class);
	String get_original_class() const;

	void set_recording_properties(bool p_enable);
	bool is_recording_properties() const;

	virtual String get_save_class() const;
};



// --- End core/io/missing_resource.h ---

// --- Folded from drivers/png/png_driver_common.h ---
#include "core/io/image.h"

namespace PNGDriverCommon {

// Attempt to load png from buffer (p_source, p_size) into p_image
Error png_to_image(const uint8_t *p_source, size_t p_size, bool p_force_linear, Ref<Image> p_image);

// Append p_image, as a png, to p_buffer.
// Contents of p_buffer is unspecified if error returned.
Error image_to_png(const Ref<Image> &p_image, Vector<uint8_t> &p_buffer);
} // namespace PNGDriverCommon
// --- End drivers/png/png_driver_common.h ---

// --- Folded from core/io/resource_importer.h ---



class ResourceImporter;
class ResourceFormatImporter;

typedef Ref<Resource> (*ResourceFormatImporterLoadOnStartup)(
	const String& p_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	ResourceFormatLoader::CacheMode p_cache_mode);

class ResourceFormatImporter final
{
public:
	using CacheMode = ResourceFormatLoader::CacheMode;

private:
	struct PathAndType
	{
		String path;
		String type;
		String importer;
		String group_file;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
	};

	static Error _get_path_and_type(const String& p_path, PathAndType& r_path_and_type, bool p_load,
		bool* r_valid = nullptr);

	// need them to stay in order to compute the settings hash
	struct SortImporterByName
	{
		bool operator()(const Ref<ResourceImporter>& p_a, const Ref<ResourceImporter>& p_b) const;
	};

	struct Data
	{
		Vector<Ref<ResourceImporter>> importers;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE);
	static Ref<Resource> load_internal(const String& p_path, Error* r_error, bool p_use_sub_threads,
		float* r_progress, CacheMode p_cache_mode, bool p_silence_errors);
	static void get_recognized_extensions(List<String>* p_extensions);
	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static bool recognize_path(
		const String& p_path, const String& p_for_type = String());
	static bool handles_type(const String& p_type);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool has_custom_uid_support();
	static bool is_import_valid(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);

	static bool is_imported(const String& p_path) { return recognize_path(p_path); }

	static String get_import_group_file(const String& p_path);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static bool exists(const String& p_path);

	static void get_build_dependencies(const String& p_path, HashSet<String>* r_dependencies);

	static int get_import_order(const String& p_path);

	static Error get_import_order_threads_and_importer(
		const String& p_path, int& r_order, bool& r_can_threads, String& r_importer);

	static String get_internal_resource_path(const String& p_path);
	static void get_internal_resource_path_list(const String& p_path, List<String>* r_paths);

	static void add_importer(const Ref<ResourceImporter>& p_importer, bool p_first_priority = false);

	static void remove_importer(const Ref<ResourceImporter>& p_importer)
	{
		if (data) {
			data->importers.erase(p_importer);
		}
	}

	static Ref<ResourceImporter> get_importer_by_name(const String& p_name);
	static Ref<ResourceImporter> get_importer_by_file(const String& p_file);
	static void get_importers_for_file(const String& p_file, List<Ref<ResourceImporter>>* r_importers);
	static void get_importers(List<Ref<ResourceImporter>>* r_importers);

	static bool are_import_settings_valid(const String& p_path);
	static String get_import_settings_hash();

	static String get_import_base_path(const String& p_for_file);
	static Error get_resource_import_info(const String& p_path, String& r_type, ResourceUID::ID& r_uid,
		String& r_import_group_file);

	ResourceFormatImporter() = delete;
	ResourceFormatImporter(const ResourceFormatImporter&) = delete;
	ResourceFormatImporter& operator=(const ResourceFormatImporter&) = delete;
	~ResourceFormatImporter() = delete;
};

class ResourceFormatImporterLoader : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override
	{
		return ResourceFormatImporter::load(
			p_path, p_original_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
	}
	virtual void get_recognized_extensions(List<String>* p_extensions) const override
	{
		ResourceFormatImporter::get_recognized_extensions(p_extensions);
	}
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override
	{
		ResourceFormatImporter::get_recognized_extensions_for_type(p_type, p_extensions);
	}
	virtual bool recognize_path(
		const String& p_path, const String& p_for_type = String()) const override
	{
		return ResourceFormatImporter::recognize_path(p_path, p_for_type);
	}
	virtual bool handles_type(const String& p_type) const override
	{
		return ResourceFormatImporter::handles_type(p_type);
	}
	virtual String get_resource_type(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_type(p_path);
	}
	virtual String get_resource_script_class(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_script_class(p_path);
	}
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_uid(p_path);
	}
	virtual bool has_custom_uid_support() const override
	{
		return ResourceFormatImporter::has_custom_uid_support();
	}
	virtual bool is_import_valid(const String& p_path) const override
	{
		return ResourceFormatImporter::is_import_valid(p_path);
	}
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override
	{
		ResourceFormatImporter::get_dependencies(p_path, p_dependencies, p_add_types);
	}
	virtual bool is_imported(const String& p_path) const override
	{
		return ResourceFormatImporter::is_imported(p_path);
	}
	virtual String get_import_group_file(const String& p_path) const override
	{
		return ResourceFormatImporter::get_import_group_file(p_path);
	}
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override
	{
		ResourceFormatImporter::get_classes_used(p_path, r_classes);
	}
	virtual bool exists(const String& p_path) const override
	{
		return ResourceFormatImporter::exists(p_path);
	}
	virtual int get_import_order(const String& p_path) const override
	{
		return ResourceFormatImporter::get_import_order(p_path);
	}
};

class ResourceImporter : public RefCounted
{
protected:

public:
	static inline ResourceFormatImporterLoadOnStartup load_on_startup = nullptr;

	virtual String get_importer_name() const = 0;
	virtual String get_visible_name() const = 0;
	virtual void get_recognized_extensions(List<String>* p_extensions) const = 0;
	virtual String get_save_extension() const = 0;
	virtual String get_resource_type() const = 0;

	virtual float get_priority() const { return 1.0; }

	virtual int get_import_order() const { return IMPORT_ORDER_DEFAULT; }

	virtual int get_format_version() const { return 0; }

	struct ImportOption
	{
		ImportOption() {}
	};

	enum ImportOrder
	{
		IMPORT_ORDER_DEFAULT = 0,
		IMPORT_ORDER_SCENE = 100,
	};

	virtual bool has_advanced_options() const { return false; }

	virtual void show_advanced_options(const String& p_path) {}

	virtual int get_preset_count() const { return 0; }

	virtual String get_preset_name(int p_idx) const { return String(); }

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const = 0;

	virtual String get_option_group_file() const { return String(); }

	virtual bool can_import_threaded() const { return false; }

	virtual void import_threaded_begin() {}

	virtual void import_threaded_end() {}

	virtual String get_import_settings_string() const { return String(); }

	virtual void get_build_dependencies(
		const String& p_path, HashSet<String>* r_build_dependencies);
};

class ResourceFormatImporterSaver : public ResourceFormatSaver
{
public:
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
};
// --- End core/io/resource_importer.h ---

// --- Folded from core/io/marshalls.h ---
#include "core/math/math_defs.h"
#include "core/typedefs.h"
#include "core/types.h"

// uintr_t is only for pairing with real_t, and we only need it in here.
#ifdef REAL_T_IS_DOUBLE
typedef uint64_t uintr_t;
#else
typedef uint32_t uintr_t;
#endif

/**
 * Miscellaneous helpers for marshaling data types, and encoding
 * in an endian independent way
 */

union MarshallFloat
{
	uint32_t i; ///< int
	float f;	///< float
};

union MarshallDouble
{
	uint64_t l; ///< long long
	double d;	///< double
};

// Behaves like one of the above, depending on compilation setting.
union MarshallReal
{
	uintr_t i;
	real_t r;
};

static inline unsigned int encode_uint16(uint16_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 2; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint16_t);
}

static inline unsigned int encode_uint32(uint32_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 4; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint32_t);
}

static inline unsigned int encode_half(float p_float, uint8_t* p_arr)
{
	encode_uint16(Math::make_half_float(p_float), p_arr);

	return sizeof(uint16_t);
}

static inline unsigned int encode_float(float p_float, uint8_t* p_arr)
{
	MarshallFloat mf;
	mf.f = p_float;
	encode_uint32(mf.i, p_arr);

	return sizeof(uint32_t);
}

static inline unsigned int encode_uint64(uint64_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 8; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint64_t);
}

static inline unsigned int encode_double(double p_double, uint8_t* p_arr)
{
	MarshallDouble md;
	md.d = p_double;
	encode_uint64(md.l, p_arr);

	return sizeof(uint64_t);
}

static inline unsigned int encode_uintr(uintr_t p_uint, uint8_t* p_arr)
{
	for (size_t i = 0; i < sizeof(uintr_t); i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uintr_t);
}

static inline unsigned int encode_real(real_t p_real, uint8_t* p_arr)
{
	MarshallReal mr;
	mr.r = p_real;
	encode_uintr(mr.i, p_arr);

	return sizeof(uintr_t);
}

static inline int encode_cstring(const char* p_string, uint8_t* p_data)
{
	int len = 0;

	while (*p_string) {
		if (p_data) {
			*p_data = (uint8_t)*p_string;
			p_data++;
		}
		p_string++;
		len++;
	}

	if (p_data) {
		*p_data = 0;
	}
	return len + 1;
}

static inline uint16_t decode_uint16(const uint8_t* p_arr)
{
	uint16_t u = 0;

	for (int i = 0; i < 2; i++) {
		uint16_t b = *p_arr;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline uint32_t decode_uint32(const uint8_t* p_arr)
{
	uint32_t u = 0;

	for (int i = 0; i < 4; i++) {
		uint32_t b = *p_arr;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline float decode_half(const uint8_t* p_arr)
{
	return Math::half_to_float(decode_uint16(p_arr));
}

static inline float decode_float(const uint8_t* p_arr)
{
	MarshallFloat mf;
	mf.i = decode_uint32(p_arr);
	return mf.f;
}

static inline uint64_t decode_uint64(const uint8_t* p_arr)
{
	uint64_t u = 0;

	for (int i = 0; i < 8; i++) {
		uint64_t b = (*p_arr) & 0xFF;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline double decode_double(const uint8_t* p_arr)
{
	MarshallDouble md;
	md.l = decode_uint64(p_arr);
	return md.d;
}

class EncodedObjectAsID : public RefCounted
{
protected:
};

Vector<float> vector3_to_float32_array(const Vector3* p_vecs, size_t p_count);
// --- End core/io/marshalls.h ---
