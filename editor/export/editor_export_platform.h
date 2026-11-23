/**************************************************************************/
/*  editor_export_platform.h                                              */
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

#include "core/os/process_id.h"
// --- Folded preset section from editor/export/editor_export.h ---
#ifndef EDITOR_EXPORT_PRESET_SECTION_H
#define EDITOR_EXPORT_PRESET_SECTION_H

#include "core/types.h"

class EditorExportPlatform;

class EditorExportPreset : public RefCounted
{
public:
	enum ExportFilter
	{
		EXPORT_ALL_RESOURCES,
		EXPORT_SELECTED_SCENES,
		EXPORT_SELECTED_RESOURCES,
		EXCLUDE_SELECTED_RESOURCES,
		EXPORT_CUSTOMIZED,
	};

	enum FileExportMode
	{
		MODE_FILE_NOT_CUSTOMIZED,
		MODE_FILE_STRIP,
		MODE_FILE_KEEP,
		MODE_FILE_REMOVE,
	};

	enum ScriptExportMode
	{
		MODE_SCRIPT_TEXT,
		MODE_SCRIPT_BINARY_TOKENS,
		MODE_SCRIPT_BINARY_TOKENS_COMPRESSED,
	};

private:
	Ref<EditorExportPlatform> platform;
	ExportFilter export_filter = EXPORT_ALL_RESOURCES;
	String include_filter;
	String exclude_filter;
	String export_path;

	String exporter;
	HashSet<String> selected_files;
	HashMap<String, FileExportMode> customized_files;
	bool dedicated_server = false;

	Vector<String> patches;
	bool patch_delta_encoding_enabled = false;
	int patch_delta_zstd_level = 19;
	double patch_delta_min_reduction = 0.1;
	String patch_delta_include_filter = "*";
	String patch_delta_exclude_filter;

	friend class EditorExport;
	friend class EditorExportPlatform;

	HashMap<String, bool> update_visibility;

	String name;
	bool options_search_active = false;

	String custom_features;

	String enc_in_filters;
	String enc_ex_filters;
	bool enc_pck = false;
	bool enc_directory = false;
	uint64_t seed = 0;

	String script_key;
	ScriptExportMode script_mode = MODE_SCRIPT_BINARY_TOKENS_COMPRESSED;

protected:
	String _get_property_warning(const String& p_name) const;

public:
	Ref<EditorExportPlatform> get_platform() const;

	void update_files();
	void update_value_overrides();

	Vector<String> get_files_to_export() const;
	HashSet<String> get_selected_files() const;
	void set_selected_files(const HashSet<String>& p_files);
	int get_customized_files_count() const;

	bool has_export_file(const String& p_path);

	FileExportMode get_file_export_mode(
		const String& p_path, FileExportMode p_default = MODE_FILE_NOT_CUSTOMIZED) const;

	String get_name() const;

	bool is_runnable() const;

	bool are_advanced_options_enabled() const;
	void set_options_search_active(bool p_active);

	bool is_dedicated_server() const;

	ExportFilter get_export_filter() const;

	String get_include_filter() const;

	String get_exclude_filter() const;

	String get_patch(int p_index);

	void set_patches(const Vector<String>& p_patches);
	Vector<String> get_patches() const;

	bool is_patch_delta_encoding_enabled() const;

	int get_patch_delta_zstd_level() const;

	double get_patch_delta_min_reduction() const;

	String get_patch_delta_include_filter() const;

	String get_patch_delta_exclude_filter() const;

	String get_custom_features() const;

	String get_export_path() const;

	String get_enc_in_filter() const;

	String get_enc_ex_filter() const;

	uint64_t get_seed() const;

	bool get_enc_pck() const;

	bool get_enc_directory() const;

	String get_script_encryption_key() const;

	ScriptExportMode get_script_export_mode() const;

	String get_version(const String& p_name, bool p_windows_version = false) const;
};

#endif // EDITOR_EXPORT_PRESET_SECTION_H
#include "scene/resources/texture.h"

class DirAccess;
class EditorExportPlugin;
class EditorFileSystemDirectory;
class FileAccess;
class Image;
class Node;
class RichTextLabel;
class Texture2D;
struct EditorProgress;
struct SharedObject;

typedef void* zipFile;

const String ENV_SCRIPT_ENCRYPTION_KEY = "GODOT_SCRIPT_ENCRYPTION_KEY";

class EditorExportPlatform : public RefCounted
{
protected:

public:

	EditorExportPlatform() {}
	typedef Error (*EditorExportSaveFunction)(const Ref<EditorExportPreset>& p_preset,
		void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file,
		int p_total, const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
		const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta);
	typedef Error (*EditorExportRemoveFunction)(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const String& p_path);
	typedef Error (*EditorExportSaveSharedObject)(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so);

	enum DebugFlags
	{
		DEBUG_FLAG_DUMB_CLIENT = 1,
		DEBUG_FLAG_REMOTE_DEBUG = 2,
		DEBUG_FLAG_REMOTE_DEBUG_LOCALHOST = 4,
		DEBUG_FLAG_VIEW_COLLISIONS = 8,
		DEBUG_FLAG_VIEW_NAVIGATION = 16,
	};

	enum ExportMessageType
	{
		EXPORT_MESSAGE_NONE,
		EXPORT_MESSAGE_INFO,
		EXPORT_MESSAGE_WARNING,
		EXPORT_MESSAGE_ERROR,
	};

	struct ExportMessage
	{
		ExportMessageType msg_type;
		String category;
		String text;
	};

	struct SavedData
	{
		uint64_t ofs = 0;
		uint64_t size = 0;
		bool encrypted = false;
		bool removal = false;
		bool delta = false;
		Vector<uint8_t> md5;
		CharString path_utf8;

		bool operator<(const SavedData& p_data) const { return path_utf8 < p_data.path_utf8; }
	};

	struct PackData
	{
		String path;
		String salt;
		Ref<FileAccess> f;
		Vector<SavedData> file_ofs;
		EditorProgress* ep = nullptr;
		Vector<SharedObject>* so_files = nullptr;
		bool use_sparse_pck = false;
	};

	static bool _store_header(Ref<FileAccess> p_fd, bool p_enc, bool p_sparse,
		uint64_t& r_file_base_ofs, uint64_t& r_dir_base_ofs, const String& p_salt);
	static bool _encrypt_and_store_directory(Ref<FileAccess> p_fd, PackData& p_pack_data,
		const Vector<uint8_t>& p_key, uint64_t p_seed, uint64_t p_file_base);
	static Error _encrypt_and_store_data(Ref<FileAccess> p_fd, const String& p_path,
		const Vector<uint8_t>& p_data, const Vector<String>& p_enc_in_filters,
		const Vector<String>& p_enc_ex_filters, const Vector<uint8_t>& p_key, uint64_t p_seed,
		bool& r_encrypt);
	static String _get_script_encryption_key(const Ref<EditorExportPreset>& p_preset);
	static Vector<uint8_t> _get_script_encryption_key_bytes(
		const Ref<EditorExportPreset>& p_preset);

private:
	struct ZipData
	{
		void* zip = nullptr;
		EditorProgress* ep = nullptr;
		Vector<SharedObject>* so_files = nullptr;
		int file_count = 0;
	};

	Vector<ExportMessage> messages;
	Vector<String> patch_temp_dirs;

	void _export_find_resources(EditorFileSystemDirectory* p_dir, HashSet<String>& p_paths);
	void _export_find_customized_resources(const Ref<EditorExportPreset>& p_preset,
		EditorFileSystemDirectory* p_dir, EditorExportPreset::FileExportMode p_mode,
		HashSet<String>& p_paths);
	void _export_find_dependencies(const String& p_path, HashSet<String>& p_paths);

	static Error _pack_add_shared_object(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so);

	static Error _remove_pack_file(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const String& p_path);

	static Error _zip_add_shared_object(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so);

	static Error _script_save_file(const Ref<EditorExportPreset>& p_preset, void* p_userdata,
		const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
		const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
		const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta);
	static Error _script_add_shared_object(
		const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so);

	void _edit_files_with_filter(
		Ref<DirAccess>& da, const Vector<String>& p_filters, HashSet<String>& r_list, bool exclude);
	void _edit_filter_list(HashSet<String>& r_list, const String& p_filter, bool exclude);

	struct FilteredCache
	{
		Vector<uint8_t> extension_list;
		Vector<uint8_t> global_class_list;
		Vector<uint8_t> uids;
	};

	static FilteredCache _get_filtered_cache(const HashSet<String>& p_paths);

	struct FileExportCache
	{
		uint64_t source_modified_time = 0;
		String source_md5;
		String saved_path;
		bool used = false;
	};

	bool _export_customize_scene_resources(Node* p_root, Node* p_node,
		LocalVector<Ref<EditorExportPlugin>>& customize_resources_plugins);
	bool _is_editable_ancestor(Node* p_root, Node* p_node);

	String _export_customize(const String& p_path,
		LocalVector<Ref<EditorExportPlugin>>& customize_resources_plugins,
		LocalVector<Ref<EditorExportPlugin>>& customize_scenes_plugins,
		HashMap<String, FileExportCache>& export_cache, const String& export_base_path,
		bool p_force_save);

protected:
	struct ExportNotifier
	{
		bool enabled = true;

		ExportNotifier(EditorExportPlatform& p_platform, const Ref<EditorExportPreset>& p_preset,
			bool p_debug, const String& p_path, uint32_t p_flags,
			bool p_enabled = true);
		~ExportNotifier();
	};

	HashSet<String> get_features(const Ref<EditorExportPreset>& p_preset, bool p_debug) const;

	bool exists_export_template(const String& p_template_file_name, String* r_err) const;
	String find_export_template(const String& p_template_file_name, String* r_err = nullptr) const;
	Vector<String> gen_export_flags(uint32_t p_flags);

	virtual void zip_folder_recursive(zipFile& p_zip, const String& p_root_path,
		const String& p_folder, const String& p_pkg_name);

	ProcessID _ssh_run_on_remote_no_wait(const String& p_host, const String& p_port,
		const Vector<String>& p_ssh_args, const String& p_cmd_args, int p_port_fwd = -1) const
	{
		ProcessID pid = 0;
		Error err =
			ssh_run_on_remote_no_wait(p_host, p_port, p_ssh_args, p_cmd_args, &pid, p_port_fwd);
		if (err != OK) {
			return -1;
		}
		else {
			return pid;
		}
	}

	Error ssh_run_on_remote(const String& p_host, const String& p_port,
		const Vector<String>& p_ssh_args, const String& p_cmd_args, String* r_out = nullptr,
		int p_port_fwd = -1) const;
	Error ssh_run_on_remote_no_wait(const String& p_host, const String& p_port,
		const Vector<String>& p_ssh_args, const String& p_cmd_args, ProcessID* r_pid = nullptr,
		int p_port_fwd = -1) const;
	Error ssh_push_to_remote(const String& p_host, const String& p_port,
		const Vector<String>& p_scp_args, const String& p_src_file, const String& p_dst_file) const;

	Error _extract_android_assets(
		const String& p_bundle_path, String& r_pck_path, String& r_temp_dir);
	Error _load_patches(const Ref<EditorExportPreset>& p_preset, const Vector<String>& p_patches);
	void _unload_patches();

	Ref<Image> _load_icon_or_splash_image(const String& p_path, Error* r_error) const;

#ifndef DISABLE_DEPRECATED
	Error _export_project_bind_compat_118787(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0);
	static Vector<String> _get_forced_export_files_bind_compat_71542();
	static void _bind_compatibility_methods();
#endif

public:
	static String simplify_path(const String& p_path);
	virtual void get_preset_features(
		const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const;

	struct ExportOption
	{
		bool update_visibility = false;
		bool required = false;

		ExportOption() {}
	};

	virtual Ref<EditorExportPreset> create_preset();

	virtual bool is_executable(const String& p_path) const { return false; }

	virtual void clear_messages() { messages.clear(); }

	virtual void add_message(
		ExportMessageType p_type, const String& p_category, const String& p_message)
	{
		ExportMessage msg;
		msg.category = p_category;
		msg.text = p_message;
		msg.msg_type = p_type;
		messages.push_back(msg);
		switch (p_type) {
		case EXPORT_MESSAGE_INFO: {
			__print_line(vformat("%s: %s", msg.category, msg.text));
		} break;
		case EXPORT_MESSAGE_WARNING: {
			WARN_PRINT(vformat("%s: %s", msg.category, msg.text));
		} break;
		case EXPORT_MESSAGE_ERROR: {
			ERR_PRINT(vformat("%s: %s", msg.category, msg.text));
		} break;
		default:
			break;
		}
	}

	virtual int get_message_count() const { return messages.size(); }

	virtual ExportMessage get_message(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, messages.size(), ExportMessage());
		return messages[p_index];
	}

	virtual ExportMessageType _get_message_type(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, messages.size(), EXPORT_MESSAGE_NONE);
		return messages[p_index].msg_type;
	}

	virtual String _get_message_category(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, messages.size(), String());
		return messages[p_index].category;
	}

	virtual String _get_message_text(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, messages.size(), String());
		return messages[p_index].text;
	}

	virtual ExportMessageType get_worst_message_type() const
	{
		ExportMessageType worst_type = EXPORT_MESSAGE_NONE;
		for (int i = 0; i < messages.size(); i++) {
			worst_type = MAX(worst_type, messages[i].msg_type);
		}
		return worst_type;
	}

	static Vector<String> get_forced_export_files(const Ref<EditorExportPreset>& p_preset);

	virtual bool fill_log_messages(RichTextLabel* p_log, Error p_err);

	virtual void get_export_options(List<ExportOption>* r_options) const;

	virtual bool should_update_export_options() { return false; }

	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const
	{
		return true;
	}

	virtual String get_export_option_warning(
		const EditorExportPreset* p_preset, const String& p_name) const
	{
		return String();
	}

	virtual String get_os_name() const;
	virtual String get_name() const;
	virtual Ref<Texture2D> get_logo() const;

	Error export_project_files(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		EditorExportSaveFunction p_save_func, EditorExportRemoveFunction p_remove_func,
		void* p_udata, EditorExportSaveSharedObject p_so_func = nullptr);

	virtual bool poll_export() { return false; }

	virtual int get_options_count() const { return 0; }

	virtual bool is_option_runnable(int p_index) const { return true; }

	virtual String get_options_tooltip() const { return ""; }

	virtual String get_option_label(int p_device) const { return ""; }

	virtual String get_option_tooltip(int p_device) const { return ""; }

	virtual String get_device_architecture(int p_device) const { return ""; }

	virtual void cleanup() {}

	virtual Error run(const Ref<EditorExportPreset>& p_preset, int p_device,
		uint32_t p_debug_flags)
	{
		return OK;
	}

	virtual Ref<Texture2D> get_run_icon() const { return get_logo(); }

	virtual bool can_export(const Ref<EditorExportPreset>& p_preset, String& r_error,
		bool& r_missing_templates, bool p_debug = false) const;
	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const;

	virtual List<String> get_binary_extensions(const Ref<EditorExportPreset>& p_preset) const;
	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0,
		bool p_notify = true);
	virtual void get_platform_features(List<String>* r_features) const;

	virtual void resolve_platform_feature_priorities(
		const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features)
	{
	}

	virtual String get_debug_protocol() const { return "tcp://"; }

	virtual void initialize() {}
};

// --- Folded from editor/export/editor_export_platform_pc.h ---


class ImageTexture;

class EditorExportPlatformPC : public EditorExportPlatform
{
private:
	Ref<ImageTexture> logo;
	String name;
	String os_name;

	int chmod_flags = -1;

public:
	virtual String get_name() const override;
	virtual void get_preset_features(
		const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const override;
	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual String get_export_option_warning(
		const EditorExportPreset* p_preset, const String& p_name) const override;

	virtual String get_os_name() const override;
	virtual Ref<Texture2D> get_logo() const override;

	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const override;
	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;
	virtual Error sign_shared_object(
		const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path);
	virtual String get_template_file_name(const String& p_target, const String& p_arch) const = 0;

	virtual Error prepare_template(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags);

	virtual Error modify_template(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags)
	{
		return OK;
	}

	virtual Error export_project_data(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags);

	void set_name(const String& p_name);
	void set_os_name(const String& p_name);

	void set_logo(const Ref<Texture2D>& p_logo);

	void add_platform_feature(const String& p_feature);
	virtual void get_platform_features(List<String>* r_features) const override;
	virtual void resolve_platform_feature_priorities(
		const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features) override;

	int get_chmod_flags() const;
	void set_chmod_flags(int p_flags);

	virtual Error fixup_embedded_pck(
		const String& p_path, int64_t p_embedded_start, int64_t p_embedded_size)
	{
		return Error::OK;
	}
};



// --- End editor/export/editor_export_platform_pc.h ---

// --- Folded from editor/export/editor_export_plugin.h ---
// --- Folded from core/os/shared_object.h ---
#include "core/string/ustring.h"
#include "core/templates/vector.h"

struct SharedObject {
	String path;
	Vector<String> tags;
	String target;

	SharedObject(const String &p_path, const Vector<String> &p_tags, const String &p_target) :
			path(p_path),
			tags(p_tags),
			target(p_target) {
	}

	SharedObject() {}
};

// --- End core/os/shared_object.h ---

#include "scene/main/node.h"

class EditorExportPlugin : public RefCounted
{
	friend class EditorExport;
	friend class EditorExportPlatform;
	friend class EditorExportPreset;

	String export_base_path;
	Ref<EditorExportPreset> export_preset;

	Vector<SharedObject> shared_objects;

	struct ExtraFile
	{
		String path;
		Vector<uint8_t> data;
		bool remap = false;
	};

	Vector<ExtraFile> extra_files;
	bool skipped = false;

	Vector<String> apple_embedded_platform_frameworks;
	Vector<String> apple_embedded_platform_embedded_frameworks;
	Vector<String> apple_embedded_platform_project_static_libs;
	String apple_embedded_platform_plist_content;
	String apple_embedded_platform_linker_flags;
	Vector<String> apple_embedded_platform_bundle_files;
	String apple_embedded_platform_cpp_code;

	Vector<String> macos_plugin_files;

	_FORCE_INLINE_ void _clear()
	{
		shared_objects.clear();
		extra_files.clear();
		skipped = false;
	}

	_FORCE_INLINE_ void _export_end_clear()
	{
		apple_embedded_platform_frameworks.clear();
		apple_embedded_platform_embedded_frameworks.clear();
		apple_embedded_platform_bundle_files.clear();
		apple_embedded_platform_plist_content = "";
		apple_embedded_platform_linker_flags = "";
		apple_embedded_platform_cpp_code = "";
		macos_plugin_files.clear();
	}

	// Export
	void _export_file_script(
		const String& p_path, const String& p_type, const Vector<String>& p_features);
	void _export_begin_script(
		const Vector<String>& p_features, bool p_debug, const String& p_path, int p_flags);
	void _export_end_script();

	String _has_valid_export_configuration(const Ref<EditorExportPlatform>& p_export_platform,
		const Ref<EditorExportPreset>& p_preset);

protected:
	void set_export_base_path(const String& p_export_base_path);
	const String& get_export_base_path() const;
	void set_export_preset(const Ref<EditorExportPreset>& p_preset);
	Ref<EditorExportPreset> get_export_preset() const;
	Ref<EditorExportPlatform> get_export_platform() const;

	void add_file(const String& p_path, const Vector<uint8_t>& p_file, bool p_remap);
	void add_shared_object(
		const String& p_path, const Vector<String>& tags, const String& p_target = String());
	void _add_shared_object(const SharedObject& p_shared_object);

	void add_apple_embedded_platform_framework(const String& p_path);
	void add_apple_embedded_platform_embedded_framework(const String& p_path);
	void add_apple_embedded_platform_project_static_lib(const String& p_path);
	void add_apple_embedded_platform_plist_content(const String& p_plist_content);
	void add_apple_embedded_platform_linker_flags(const String& p_flags);
	void add_apple_embedded_platform_bundle_file(const String& p_path);
	void add_apple_embedded_platform_cpp_code(const String& p_code);
	void add_macos_plugin_file(const String& p_path);

	void skip();

	virtual void _export_file(
		const String& p_path, const String& p_type, const HashSet<String>& p_features);
	virtual void _export_begin(
		const HashSet<String>& p_features, bool p_debug, const String& p_path, int p_flags);
	virtual void _export_end();
	virtual void _end_generate_apple_embedded_project(
		const String& p_path, bool p_will_build_archive);

	virtual bool _begin_customize_resources(const Ref<EditorExportPlatform>& p_platform,
		const Vector<String>& p_features);
	virtual Ref<Resource> _customize_resource(const Ref<Resource>& p_resource,
		const String& p_path);

	virtual bool _begin_customize_scenes(const Ref<EditorExportPlatform>& p_platform,
		const Vector<String>& p_features);
	virtual Node* _customize_scene(Node* p_root, const String& p_path);

	virtual uint64_t _get_customization_configuration_hash() const;

	virtual void _end_customize_scenes();
	virtual void _end_customize_resources();

	virtual PackedStringArray _get_export_features(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual void _get_export_options(const Ref<EditorExportPlatform>& p_export_platform,
		List<EditorExportPlatform::ExportOption>* r_options) const;
	virtual bool _should_update_export_options(
		const Ref<EditorExportPlatform>& p_export_platform) const;
	virtual bool _get_export_option_visibility(
		const Ref<EditorExportPlatform>& p_export_platform, const String& p_option_name) const;
	virtual String _get_export_option_warning(
		const Ref<EditorExportPlatform>& p_export_platform, const String& p_option_name) const;

public:
	virtual String get_name() const;

	virtual bool supports_platform(const Ref<EditorExportPlatform>& p_export_platform) const;
	PackedStringArray get_export_features(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;

	void end_generate_apple_embedded_project(const String& p_path, bool p_will_build_archive);

	virtual PackedStringArray get_android_dependencies(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual PackedStringArray get_android_dependencies_maven_repos(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual PackedStringArray get_android_libraries(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual String get_android_manifest_activity_element_contents(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual String get_android_manifest_application_element_contents(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual String get_android_manifest_element_contents(
		const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const;
	virtual PackedByteArray update_android_prebuilt_manifest(
		const Ref<EditorExportPlatform>& p_export_platform,
		const PackedByteArray& p_manifest_data) const;

	Vector<String> get_apple_embedded_platform_frameworks() const;
	Vector<String> get_apple_embedded_platform_embedded_frameworks() const;
	Vector<String> get_apple_embedded_platform_project_static_libs() const;
	String get_apple_embedded_platform_plist_content() const;
	String get_apple_embedded_platform_linker_flags() const;
	Vector<String> get_apple_embedded_platform_bundle_files() const;
	String get_apple_embedded_platform_cpp_code() const;
	const Vector<String>& get_macos_plugin_files() const;
};

class DedicatedServerExportPlugin final : public EditorExportPlugin
{
private:
	EditorExportPreset::FileExportMode current_export_mode =
		EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED;

	EditorExportPreset::FileExportMode _get_export_mode_for_path(const String& p_path);

protected:
	String get_name() const override { return "DedicatedServer"; }

	PackedStringArray _get_export_features(
		const Ref<EditorExportPlatform>& p_platform, bool p_debug) const override;
	uint64_t _get_customization_configuration_hash() const override;

	bool _begin_customize_scenes(
		const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features) override;
	bool _begin_customize_resources(
		const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features) override;

	Node* _customize_scene(Node* p_root, const String& p_path) override;
	Ref<Resource> _customize_resource(
		const Ref<Resource>& p_resource, const String& p_path) override;

	void _end_customize_scenes() override;
	void _end_customize_resources() override;
};

// --- End editor/export/editor_export_plugin.h ---

// --- Folded from editor/export/apple_export_utils.h ---
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/string/ustring.h"

/*************************************************************************/
/* MachO                                                                 */
/*************************************************************************/

class MachO final : public RefCounted
{
public:
	struct MachHeader
	{
		uint32_t cputype;
		uint32_t cpusubtype;
		uint32_t filetype;
		uint32_t ncmds;
		uint32_t sizeofcmds;
		uint32_t flags;
	};

	enum LoadCommandID
	{
		LC_SEGMENT = 0x00000001,
		LC_SYMTAB = 0x00000002,
		LC_SYMSEG = 0x00000003,
		LC_THREAD = 0x00000004,
		LC_UNIXTHREAD = 0x00000005,
		LC_LOADFVMLIB = 0x00000006,
		LC_IDFVMLIB = 0x00000007,
		LC_IDENT = 0x00000008,
		LC_FVMFILE = 0x00000009,
		LC_PREPAGE = 0x0000000a,
		LC_DYSYMTAB = 0x0000000b,
		LC_LOAD_DYLIB = 0x0000000c,
		LC_ID_DYLIB = 0x0000000d,
		LC_LOAD_DYLINKER = 0x0000000e,
		LC_ID_DYLINKER = 0x0000000f,
		LC_PREBOUND_DYLIB = 0x00000010,
		LC_ROUTINES = 0x00000011,
		LC_SUB_FRAMEWORK = 0x00000012,
		LC_SUB_UMBRELLA = 0x00000013,
		LC_SUB_CLIENT = 0x00000014,
		LC_SUB_LIBRARY = 0x00000015,
		LC_TWOLEVEL_HINTS = 0x00000016,
		LC_PREBIND_CKSUM = 0x00000017,
		LC_LOAD_WEAK_DYLIB = 0x80000018,
		LC_SEGMENT_64 = 0x00000019,
		LC_ROUTINES_64 = 0x0000001a,
		LC_UUID = 0x0000001b,
		LC_RPATH = 0x8000001c,
		LC_CODE_SIGNATURE = 0x0000001d,
		LC_SEGMENT_SPLIT_INFO = 0x0000001e,
		LC_REEXPORT_DYLIB = 0x8000001f,
		LC_LAZY_LOAD_DYLIB = 0x00000020,
		LC_ENCRYPTION_INFO = 0x00000021,
		LC_DYLD_INFO = 0x00000022,
		LC_DYLD_INFO_ONLY = 0x80000022,
		LC_LOAD_UPWARD_DYLIB = 0x80000023,
		LC_VERSION_MIN_MACOSX = 0x00000024,
		LC_VERSION_MIN_IPHONEOS = 0x00000025,
		LC_FUNCTION_STARTS = 0x00000026,
		LC_DYLD_ENVIRONMENT = 0x00000027,
		LC_MAIN = 0x80000028,
		LC_DATA_IN_CODE = 0x00000029,
		LC_SOURCE_VERSION = 0x0000002a,
		LC_DYLIB_CODE_SIGN_DRS = 0x0000002b,
		LC_ENCRYPTION_INFO_64 = 0x0000002c,
		LC_LINKER_OPTION = 0x0000002d,
		LC_LINKER_OPTIMIZATION_HINT = 0x0000002e,
		LC_VERSION_MIN_TVOS = 0x0000002f,
		LC_VERSION_MIN_WATCHOS = 0x00000030,
		LC_BUILD_VERSION = 0x00000032,
	};

	enum PlatformID
	{
		PLATFORM_UNKNOWN = 0,
		PLATFORM_MACOS = 1,
		PLATFORM_IOS = 2,
		PLATFORM_TVOS = 3,
		PLATFORM_WATCHOS = 4,
		PLATFORM_BRIDGEOS = 5,
		PLATFORM_MACCATALYST = 6,
		PLATFORM_IOSSIMULATOR = 7,
		PLATFORM_TVOSSIMULATOR = 8,
		PLATFORM_WATCHOSSIMULATOR = 9,
		PLATFORM_DRIVERKIT = 10,
		PLATFORM_VISIONOS = 11,
		PLATFORM_VISIONOSSIMULATOR = 12,
	};

	struct LoadCommandHeader
	{
		uint32_t cmd;
		uint32_t cmdsize;
	};

	struct LoadCommandSegment
	{
		char segname[16];
		uint32_t vmaddr;
		uint32_t vmsize;
		uint32_t fileoff;
		uint32_t filesize;
		uint32_t maxprot;
		uint32_t initprot;
		uint32_t nsects;
		uint32_t flags;
	};

	struct LoadCommandSegment64
	{
		char segname[16];
		uint64_t vmaddr;
		uint64_t vmsize;
		uint64_t fileoff;
		uint64_t filesize;
		uint32_t maxprot;
		uint32_t initprot;
		uint32_t nsects;
		uint32_t flags;
	};

	struct Section
	{
		char sectname[16];
		char segname[16];
		uint32_t addr;
		uint32_t size;
		uint32_t offset;
		uint32_t align;
		uint32_t reloff;
		uint32_t nreloc;
		uint32_t flags;
		uint32_t reserved1;
		uint32_t reserved2;
	};

	struct Section64
	{
		char sectname[16];
		char segname[16];
		uint64_t addr;
		uint64_t size;
		uint32_t offset;
		uint32_t align;
		uint32_t reloff;
		uint32_t nreloc;
		uint32_t flags;
		uint32_t reserved1;
		uint32_t reserved2;
		uint32_t reserved3;
	};

private:
	Ref<FileAccess> fa;
	bool swap = false;

	uint64_t lc_limit = 0;

	uint64_t exe_limit = 0;
	uint64_t exe_base = std::numeric_limits<uint64_t>::max();
	uint32_t align = 0;
	uint32_t cputype = 0;
	uint32_t cpusubtype = 0;

	uint64_t link_edit_offset = 0;
	uint64_t signature_offset = 0;

	uint32_t seg_align(uint64_t p_vmaddr, uint32_t p_min, uint32_t p_max);
	bool alloc_signature(uint64_t p_size);

	static inline size_t PAD(size_t s, size_t a) { return (a - s % a); }

public:
	static bool is_macho(const String& p_path);
	static uint32_t get_filetype(const String& p_path);

	bool open_file(const String& p_path);

	uint64_t get_exe_base();
	uint64_t get_exe_limit();
	int32_t get_align();
	uint32_t get_cputype();
	uint32_t get_cpusubtype();
	uint64_t get_size();
	uint64_t get_code_limit();

	uint64_t get_signature_offset();
	bool is_signed();

	PackedByteArray get_cdhash_sha1();
	PackedByteArray get_cdhash_sha256();

	PackedByteArray get_requirements();

	const Ref<FileAccess> get_file() const;
	Ref<FileAccess> get_file();

	uint64_t get_signature_size();
	bool set_signature_size(uint64_t p_size);
};

/*************************************************************************/
/* LipO                                                                  */
/*************************************************************************/

class LipO final : public RefCounted
{
	struct FatArch
	{
		uint32_t cputype;
		uint32_t cpusubtype;
		uint64_t offset;
		uint64_t size;
		uint32_t align;
	};

	Ref<FileAccess> fa;
	Vector<FatArch> archs;

	static inline size_t PAD(size_t s, size_t a) { return (a - s % a); }

public:
	static bool is_lipo(const String& p_path);

	bool create_file(const String& p_output_path, const Vector<String>& p_files);
	bool create_file(const String& p_output_path, const Vector<String>& p_files,
		const Vector<Vector2i>& p_cputypes);

	bool open_file(const String& p_path);
	int get_arch_count() const;
	uint32_t get_arch_cputype(int p_index) const;
	uint32_t get_arch_cpusubtype(int p_index) const;
	bool extract_arch(int p_index, const String& p_path);

	void close();

	~LipO() override;
};

/*************************************************************************/
/* CodeSignCodeResources                                                 */
/*************************************************************************/

class CodeSignCodeResources
{
public:
	enum class CRMatch
	{
		CR_MATCH_NO,
		CR_MATCH_YES,
		CR_MATCH_NESTED,
		CR_MATCH_OPTIONAL,
	};

private:
	struct CRFile
	{
		String name;
		String hash;
		String hash2;
		bool optional;
		bool nested;
		String requirements;
	};

	struct CRRule
	{
		String file_pattern;
		String key;
		int weight;
		bool store;

		CRRule()
		{
			weight = 1;
			store = true;
		}

		CRRule(const String& p_file_pattern, const String& p_key, int p_weight, bool p_store)
		{
			file_pattern = p_file_pattern;
			key = p_key;
			weight = p_weight;
			store = p_store;
		}
	};

	Vector<CRRule> rules1;
	Vector<CRRule> rules2;

	Vector<CRFile> files1;
	Vector<CRFile> files2;

	String hash_sha1_base64(const String& p_path);
	String hash_sha256_base64(const String& p_path);

public:
	void add_rule1(
		const String& p_rule, const String& p_key = "", int p_weight = 0, bool p_store = true);
	void add_rule2(
		const String& p_rule, const String& p_key = "", int p_weight = 0, bool p_store = true);

	CRMatch match_rules1(const String& p_path) const;
	CRMatch match_rules2(const String& p_path) const;

	bool add_file1(const String& p_root, const String& p_path);
	bool add_file2(const String& p_root, const String& p_path);
	bool add_nested_file(const String& p_root, const String& p_path, const String& p_exepath);

	bool add_folder_recursive(
		const String& p_root, const String& p_path = "", const String& p_main_exe_path = "");

	bool save_to_file(const String& p_path);
};

/*************************************************************************/
/* CodeSignBlob                                                          */
/*************************************************************************/

class CodeSignBlob
{
public:
	enum IndexType : uint32_t
	{
		INDEX_CODE_DIRECTORY = 0x00000000,
		INDEX_REQUIREMENTS = 0x00000002,
		INDEX_ENTITLEMENTS_TEXT = 0x00000005,
		INDEX_ENTITLEMENTS_BINARY = 0x00000007,
		INDEX_SIGNATURE = 0x00010000,
	};

	enum Slot
	{
		SLOT_INFO_PLIST = -1,
		SLOT_REQUIREMENTS = -2,
		SLOT_RESOURCES = -3,
		SLOT_APP_SPECIFIC = -4,
		SLOT_ENTITLEMENTS = -5,
		SLOT_RESERVER1 = -6,
		SLOT_DER_ENTITLEMENTS = -7,
	};

	enum CodeSignExecSegFlags
	{
		EXECSEG_MAIN_BINARY = 0x1,
		EXECSEG_ALLOW_UNSIGNED = 0x10,
		EXECSEG_DEBUGGER = 0x20,
		EXECSEG_JIT = 0x40,
		EXECSEG_SKIP_LV = 0x80,
		EXECSEG_CAN_LOAD_CDHASH = 0x100,
		EXECSEG_CAN_EXEC_CDHASH = 0x200,
	};

	enum CodeSignatureFlags
	{
		SIGNATURE_HOST = 0x0001,
		SIGNATURE_ADHOC = 0x0002,
		SIGNATURE_TASK_ALLOW = 0x0004,
		SIGNATURE_INSTALLER = 0x0008,
		SIGNATURE_FORCED_LV = 0x0010,
		SIGNATURE_INVALID_ALLOWED = 0x0020,
		SIGNATURE_FORCE_HARD = 0x0100,
		SIGNATURE_FORCE_KILL = 0x0200,
		SIGNATURE_FORCE_EXPIRATION = 0x0400,
		SIGNATURE_RESTRICT = 0x0800,
		SIGNATURE_ENFORCEMENT = 0x1000,
		SIGNATURE_LIBRARY_VALIDATION = 0x2000,
		SIGNATURE_ENTITLEMENTS_VALIDATED = 0x4000,
		SIGNATURE_NVRAM_UNRESTRICTED = 0x8000,
		SIGNATURE_RUNTIME = 0x10000,
		SIGNATURE_LINKER_SIGNED = 0x20000,
	};

private:
	struct CodeDirectoryHeader
	{
		uint32_t version;
		uint32_t flags;
		uint32_t hash_offset;
		uint32_t ident_offset;
		uint32_t special_slots;
		uint32_t code_slots;
		uint32_t code_limit;
		uint8_t hash_size;
		uint8_t hash_type;
		uint8_t platform;
		uint8_t page_size;
		uint32_t spare2;
		uint32_t scatter_vector_offset;
		uint32_t team_offset;
		uint32_t spare3;
		uint64_t code_limit_64;
		uint64_t exec_seg_base;
		uint64_t exec_seg_limit;
		uint64_t exec_seg_flags;
		uint32_t runtime;
		uint32_t pre_encrypt_offset;
	};

	PackedByteArray blob;
	uint32_t index_type = INDEX_CODE_DIRECTORY;
	int32_t pages = 0;
	int32_t remain = 0;
	int32_t code_slots = 0;
	int32_t special_slots = 0;

	static inline size_t PAD(size_t s, size_t a) { return (s % a == 0) ? 0 : (a - s % a); }

	_FORCE_INLINE_ void _parse_certificate_slot(
		uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_key(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_oid_key(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_hash_string(
		uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_value(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ void _parse_date(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;
	_FORCE_INLINE_ bool _parse_match(uint32_t& r_pos, String& r_out, uint32_t p_rq_size) const;

public:
	CodeSignBlob() = default;

	static CodeSignBlob make_requirements();
	static CodeSignBlob make_requirements(const PackedByteArray& p_data);
	static CodeSignBlob make_entitlements_text();
	static CodeSignBlob make_entitlements_text(const String& p_string);
	static CodeSignBlob make_entitlements_binary();
	static CodeSignBlob make_entitlements_binary(const String& p_string);
	static CodeSignBlob make_code_directory();
	static CodeSignBlob make_code_directory(uint8_t p_hash_size, uint8_t p_hash_type, bool p_main,
		const CharString& p_id, const CharString& p_team_id, uint32_t p_page_size,
		uint64_t p_exe_limit, uint64_t p_code_limit);
	static CodeSignBlob make_signature();

	Vector<String> parse_requirements() const;

	int32_t get_page_count() const { return pages; }
	int32_t get_page_remainder() const { return remain; }
	bool set_hash_in_slot(const PackedByteArray& p_hash, int p_slot);

	bool is_empty() const { return blob.is_empty(); }
	PackedByteArray get_hash_sha1() const;
	PackedByteArray get_hash_sha256() const;
	int get_size() const { return blob.size(); }
	uint32_t get_index_type() const { return index_type; }
	void write_to_file(Ref<FileAccess> p_file) const;
};

/*************************************************************************/
/* CodeSignSuperBlob                                                     */
/*************************************************************************/

class CodeSignSuperBlob
{
	Vector<CodeSignBlob> blobs;

public:
	bool add_blob(const CodeSignBlob& p_blob);

	int get_size() const;
	void write_to_file(Ref<FileAccess> p_file) const;
};

/*************************************************************************/
/* CodeSign                                                              */
/*************************************************************************/

class CodeSign
{
	static PackedByteArray file_hash_sha1(const String& p_path);
	static PackedByteArray file_hash_sha256(const String& p_path);
	static Error _codesign_file(bool p_use_hardened_runtime, bool p_force, const String& p_info,
		const String& p_exe_path, const String& p_bundle_path, const String& p_ent_path,
		bool p_ios_bundle, String& r_error_msg);

public:
	static Error codesign(bool p_use_hardened_runtime, bool p_force, const String& p_path,
		const String& p_ent_path, String& r_error_msg);
};

/*************************************************************************/
/* PluginConfigAppleEmbedded                                             */
/*************************************************************************/

struct PluginConfigAppleEmbedded
{
	inline static const char* PLUGIN_CONFIG_EXT = ".gdip";

	inline static const char* CONFIG_SECTION = "config";
	inline static const char* CONFIG_NAME_KEY = "name";
	inline static const char* CONFIG_BINARY_KEY = "binary";
	inline static const char* CONFIG_INITIALIZE_KEY = "initialization";
	inline static const char* CONFIG_DEINITIALIZE_KEY = "deinitialization";

	inline static const char* DEPENDENCIES_SECTION = "dependencies";
	inline static const char* DEPENDENCIES_LINKED_KEY = "linked";
	inline static const char* DEPENDENCIES_EMBEDDED_KEY = "embedded";
	inline static const char* DEPENDENCIES_SYSTEM_KEY = "system";
	inline static const char* DEPENDENCIES_CAPABILITIES_KEY = "capabilities";
	inline static const char* DEPENDENCIES_FILES_KEY = "files";
	inline static const char* DEPENDENCIES_LINKER_FLAGS = "linker_flags";
	inline static const char* DEPENDENCIES_SPM_PACKAGES_KEY = "spm_packages";

	inline static const char* PLIST_SECTION = "plist";

	enum PlistItemType
	{
		UNKNOWN,
		STRING,
		INTEGER,
		BOOLEAN,
		RAW,
		STRING_INPUT,
	};

	struct PlistItem
	{
		PlistItemType type;
		String value;
	};

	struct SPMPackage
	{
		String url;
		String version;
		Vector<String> products;
	};

	bool valid_config = false;
	bool supports_targets = false;
	uint64_t last_updated = 0;

	String name;
	String binary;
	String initialization_method;
	String deinitialization_method;

	Vector<String> linked_dependencies;
	Vector<String> embedded_dependencies;
	Vector<String> system_dependencies;

	Vector<String> files_to_copy;
	Vector<String> capabilities;

	Vector<String> linker_flags;

	Vector<SPMPackage> spm_packages;

	HashMap<String, PlistItem> plist;

	static String resolve_local_dependency_path(String plugin_config_dir, String dependency_path);
	static String resolve_system_dependency_path(String dependency_path);
	static Vector<String> resolve_local_dependencies(
		String plugin_config_dir, Vector<String> p_paths);
	static Vector<String> resolve_system_dependencies(Vector<String> p_paths);
	static bool validate_plugin(PluginConfigAppleEmbedded& plugin_config);
	static String get_plugin_main_binary(PluginConfigAppleEmbedded& plugin_config, bool p_debug);
	static uint64_t get_plugin_modification_time(
		const PluginConfigAppleEmbedded& plugin_config, const String& config_path);
	static PluginConfigAppleEmbedded load_plugin_config(
		Ref<ConfigFile> config_file, const String& path);
};

// --- Folded from editor/export/editor_export_platform_apple_embedded.h ---
#include <sys/stat.h>
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/templates/safe_refcount.h"

#include "scene/resources/image_texture.h"

#ifdef MACOS_ENABLED
#include <functional>
#endif

// Optional environment variables for defining confidential information. If any
// of these is set, they will override the values set in the credentials file.
const String ENV_APPLE_PLATFORM_PROFILE_UUID_DEBUG =
	"GODOT_APPLE_PLATFORM_PROVISIONING_PROFILE_UUID_DEBUG";
const String ENV_APPLE_PLATFORM_PROFILE_UUID_RELEASE =
	"GODOT_APPLE_PLATFORM_PROVISIONING_PROFILE_UUID_RELEASE";
const String ENV_APPLE_PLATFORM_PROFILE_SPECIFIER_DEBUG =
	"GODOT_APPLE_PLATFORM_PROFILE_SPECIFIER_DEBUG";
const String ENV_APPLE_PLATFORM_PROFILE_SPECIFIER_RELEASE =
	"GODOT_APPLE_PLATFORM_PROFILE_SPECIFIER_RELEASE";

static const String storyboard_image_scale_mode[] = {
	"center",
	"scaleAspectFit",
	"scaleAspectFill",
	"scaleToFill",
};

class EditorExportPlatformAppleEmbedded : public EditorExportPlatform
{
	Ref<ImageTexture> logo;
	Ref<ImageTexture> run_icon;

	// Plugins
	mutable SafeFlag plugins_changed;
	SafeFlag devices_changed;

	struct Device
	{
		String id;
		String name;
		bool wifi = false;
		bool use_ios_deploy = false;
	};

	Vector<Device> devices;
	Mutex device_lock;

	Mutex plugins_lock;
	mutable Vector<PluginConfigAppleEmbedded> plugins;
#ifdef MACOS_ENABLED
	Thread check_for_changes_thread;
	SafeFlag quit_request;
	SafeFlag has_runnable_preset;

	static bool _check_xcode_install();
	static void _check_for_changes_poll_thread(void* ud);
	void _update_preset_status();

protected:
	void _start_remote_device_poller_thread()
	{
		check_for_changes_thread.start(_check_for_changes_poll_thread, this);
	}

	void _stop_remote_device_poller_thread()
	{
		quit_request.set();
		if (check_for_changes_thread.is_started()) {
			check_for_changes_thread.wait_to_finish();
		}
	}

	int _execute(const String& p_path, const List<String>& p_arguments,
		std::function<void(const String&)> p_on_data);

private:
#endif

	typedef Error (*FileHandler)(String p_file, void* p_userdata);
	static Error _walk_dir_recursive(Ref<DirAccess>& p_da, FileHandler p_handler, void* p_userdata);
	static Error _codesign(String p_file, void* p_userdata);

	struct ExportArchitecture
	{
		String name;
		bool is_default = false;

		ExportArchitecture() {}

		ExportArchitecture(String p_name, bool p_is_default)
		{
			name = p_name;
			is_default = p_is_default;
		}
	};

	struct AppleEmbeddedExportAsset
	{
		String exported_path;
		bool is_framework =
			false; // framework is anything linked to the binary, otherwise it's a resource
		bool should_embed = false;
	};

	String _get_additional_plist_content();
	String _get_linker_flags();
	String _get_cpp_code();

protected:
	struct AppleEmbeddedConfigData
	{
		String pkg_name;
		String binary_name;
		String plist_content;
		String architectures;
		String linker_flags;
		String cpp_code;
		String modules_buildfile;
		String modules_fileref;
		String modules_buildphase;
		String modules_buildgrp;
		String spm_packages;
		String spm_package_refs;
		String spm_package_products;
		Vector<String> capabilities;
	};

	struct CodeSigningDetails
	{
		String debug_signing_identity;
		String release_signing_identity;
		String debug_provisioning_profile_uuid;
		String release_provisioning_profile_uuid;
		String debug_provisioning_profile_specifier;
		String release_provisioning_profile_specifier;
		bool debug_manual_signing = false;
		bool release_manual_signing = false;
	};

	struct IconInfo
	{
		const char* preset_key;
		const char* idiom;
		const char* export_name;
		const char* actual_size_side;
		const char* scale;
		const char* unscaled_size;
		bool force_opaque;
	};

private:
	void _fix_config_file(const Ref<EditorExportPreset>& p_preset, Vector<uint8_t>& pfile,
		const AppleEmbeddedConfigData& p_config, bool p_debug);

	Vector<ExportArchitecture> _get_supported_architectures() const;
	Vector<String> _get_preset_architectures(const Ref<EditorExportPreset>& p_preset) const;

	void _check_xcframework_content(const String& p_path, int& r_total_libs, int& r_static_libs,
		int& r_dylibs, int& r_frameworks) const;
	Error _convert_to_framework(
		const String& p_source, const String& p_destination, const String& p_id) const;

	void _add_assets_to_project(const String& p_out_dir, const Ref<EditorExportPreset>& p_preset,
		Vector<uint8_t>& p_project_data,
		const Vector<AppleEmbeddedExportAsset>& p_additional_assets);
	Error _export_additional_assets(const Ref<EditorExportPreset>& p_preset,
		const String& p_out_dir, const Vector<String>& p_assets, bool p_is_framework,
		bool p_should_embed, Vector<AppleEmbeddedExportAsset>& r_exported_assets);
	Error _copy_asset(const Ref<EditorExportPreset>& p_preset, const String& p_out_dir,
		const String& p_asset, const String* p_custom_file_name, bool p_is_framework,
		bool p_should_embed, Vector<AppleEmbeddedExportAsset>& r_exported_assets);
	Error _export_additional_assets(const Ref<EditorExportPreset>& p_preset,
		const String& p_out_dir, const Vector<SharedObject>& p_libraries,
		Vector<AppleEmbeddedExportAsset>& r_exported_assets);
	Error _export_apple_embedded_plugins(const Ref<EditorExportPreset>& p_preset,
		AppleEmbeddedConfigData& p_config_data, const String& p_dest_dir,
		const Vector<String>& p_module_libs, Vector<AppleEmbeddedExportAsset>& r_exported_assets,
		bool p_debug);

	Error _export_project_helper(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags, bool p_notify, bool p_oneclick);

	bool is_package_name_valid(const String& p_package, String* r_error = nullptr) const;

protected:
	virtual String _process_config_file_line(const Ref<EditorExportPreset>& p_preset,
		const String& p_line, const AppleEmbeddedConfigData& p_config, bool p_debug,
		const CodeSigningDetails& p_code_signing);

	String launch_screen_image_file_name;

	void _blend_and_rotate(Ref<Image>& p_dst, Ref<Image>& p_src, bool p_rot);

	virtual Error _export_loading_screen_file(
		const Ref<EditorExportPreset>& p_preset, const String& p_dest_dir)
	{
		return OK;
	}

	virtual Error _export_icons(
		const Ref<EditorExportPreset>& p_preset, const String& p_iconset_dir)
	{
		return OK;
	}

	virtual String get_platform_name() const = 0;
	virtual String get_sdk_name() const = 0;
	virtual const Vector<String> get_device_types() const = 0;
	virtual String get_minimum_deployment_target() const = 0;

	virtual void get_preset_features(
		const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const override;
	virtual void get_export_options(List<ExportOption>* r_options) const override;
	virtual bool get_export_option_visibility(
		const EditorExportPreset* p_preset, const String& p_option) const override;
	virtual String get_export_option_warning(
		const EditorExportPreset* p_preset, const String& p_name) const override;

	virtual Vector<IconInfo> get_icon_infos() const = 0;

	void _notification(int p_what);

	virtual void get_platform_features(List<String>* r_features) const override
	{
		r_features->push_back("mobile");
		r_features->push_back("apple_embedded");
	}

	void _initialize(const char* p_platform_logo_svg, const char* p_run_icon_svg);

public:
	virtual Ref<Texture2D> get_logo() const override { return logo; }

	virtual Ref<Texture2D> get_run_icon() const override { return run_icon; }

	virtual int get_options_count() const override;
	virtual String get_options_tooltip() const override;
	virtual String get_option_label(int p_index) const override;
	virtual String get_option_tooltip(int p_index) const override;
	virtual Error run(
		const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags) override;

	virtual bool poll_export() override
	{
		bool dc = devices_changed.is_set();
		if (dc) {
			// don't clear unless we're reporting true, to avoid race
			devices_changed.clear();
		}
		return dc;
	}

	virtual bool should_update_export_options() override
	{
		bool export_options_changed = plugins_changed.is_set();
		if (export_options_changed) {
			// don't clear unless we're reporting true, to avoid race
			plugins_changed.clear();
		}
		return export_options_changed;
	}

	virtual Error export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
		const String& p_path, uint32_t p_flags = 0, bool p_notify = true) override;

	virtual bool has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
		String& r_error, bool& r_missing_templates, bool p_debug = false) const override;
	virtual bool has_valid_project_configuration(
		const Ref<EditorExportPreset>& p_preset, String& r_error) const override;

	virtual void resolve_platform_feature_priorities(
		const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features) override
	{
	}

	~EditorExportPlatformAppleEmbedded();

	/// List the gdip files in the directory specified by the p_path parameter.
	static Vector<String> list_plugin_config_files(const String& p_path, bool p_check_directories)
	{
		Vector<String> dir_files;
		Ref<DirAccess> da = DirAccess::open(p_path);
		if (da.is_valid()) {
			da->list_dir_begin();
			while (true) {
				String file = da->get_next();
				if (file.is_empty()) {
					break;
				}

				if (file == "." || file == "..") {
					continue;
				}

				if (da->current_is_hidden()) {
					continue;
				}

				if (da->current_is_dir()) {
					if (p_check_directories) {
						Vector<String> directory_files =
							list_plugin_config_files(p_path.path_join(file), false);
						for (int i = 0; i < directory_files.size(); ++i) {
							dir_files.push_back(file.path_join(directory_files[i]));
						}
					}

					continue;
				}

				if (file.ends_with(PluginConfigAppleEmbedded::PLUGIN_CONFIG_EXT)) {
					dir_files.push_back(file);
				}
			}
			da->list_dir_end();
		}

		return dir_files;
	}

	static Vector<PluginConfigAppleEmbedded> get_plugins(const String& p_platform_name)
	{
		Vector<PluginConfigAppleEmbedded> loaded_plugins;

		String plugins_dir = ProjectSettings::get_resource_path().path_join(
			p_platform_name + "/plugins");

		if (DirAccess::exists(plugins_dir)) {
			Vector<String> plugins_filenames = list_plugin_config_files(plugins_dir, true);

			if (!plugins_filenames.is_empty()) {
				Ref<ConfigFile> config_file;
				config_file.instantiate();
				for (int i = 0; i < plugins_filenames.size(); i++) {
					PluginConfigAppleEmbedded config =
						PluginConfigAppleEmbedded::load_plugin_config(
							config_file, plugins_dir.path_join(plugins_filenames[i]));
					if (config.valid_config) {
						loaded_plugins.push_back(config);
					}
					else {
						print_error("Invalid plugin config file " + plugins_filenames[i]);
					}
				}
			}
		}

		return loaded_plugins;
	}
};



// --- End editor/export/editor_export_platform_apple_embedded.h ---

// --- End editor/export/apple_export_utils.h ---

// --- Folded main section from editor/export/editor_export.h ---
#include "scene/main/node.h"
class Timer;

typedef void* zipFile;

class ProjectZIPPacker
{
	static void _zip_file(const String& p_path, const String& p_base_path, zipFile p_zip);
	static void _zip_recursive(const String& p_path, const String& p_base_path, zipFile p_zip);

public:
	static String get_project_zip_safe_name();
	static void pack_project_zip(const String& p_path);
};

class EditorExport : public Node
{
	Vector<Ref<EditorExportPlatform>> export_platforms;
	Vector<Ref<EditorExportPreset>> export_presets;
	Vector<Ref<EditorExportPlugin>> export_plugins;
	HashMap<Ref<EditorExportPlatform>, Ref<EditorExportPreset>> runnable_presets;

	static inline const String RUNNABLE_SECTION_NAME = "runnable_presets";

	static inline String _export_presets_updated;
	static inline String _export_presets_runnable_updated;

	Timer* save_timer = nullptr;
	bool block_save = false;
	bool should_update_presets = false;
	bool should_reload_presets = false;

	void _save();

protected:
	friend class EditorExportPreset;
	void emit_presets_runnable_changed();

	void _notification(int p_what);

public:
	static inline EditorExport* instance = nullptr;
	_FORCE_INLINE_ static bool is_initialized() { return instance != nullptr; }

	// Encodes a platform/device pair into a single menu/device id and decodes it back.
	static int encode_platform_device_id(int p_platform_idx, int p_device_idx)
	{
		return p_platform_idx * 10000 + p_device_idx;
	}

	static int decode_platform_from_id(int p_id) { return p_id / 10000; }

	static int decode_device_from_id(int p_id) { return p_id % 10000; }

	void _impl_add_export_platform(const Ref<EditorExportPlatform>& p_platform);
	static void add_export_platform(const Ref<EditorExportPlatform>& p_platform);
	int _impl_get_export_platform_count() const;
	static int get_export_platform_count();
	int get_export_platform_index_by_name(const String& p_name);
	Ref<EditorExportPlatform> _impl_get_export_platform(int p_idx);
	static Ref<EditorExportPlatform> get_export_platform(int p_idx);
	void _impl_remove_export_platform(const Ref<EditorExportPlatform>& p_platform);
	static void remove_export_platform(const Ref<EditorExportPlatform>& p_platform);

	void add_export_preset(const Ref<EditorExportPreset>& p_preset, int p_at_pos = -1);
	int _impl_get_export_preset_count() const;
	static int get_export_preset_count();
	Ref<EditorExportPreset> _impl_get_export_preset(int p_idx);
	static Ref<EditorExportPreset> get_export_preset(int p_idx);

	void _impl_add_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	static void add_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	void _impl_remove_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	static void remove_export_plugin(const Ref<EditorExportPlugin>& p_plugin);
	Vector<Ref<EditorExportPlugin>> _impl_get_export_plugins();
	static Vector<Ref<EditorExportPlugin>> get_export_plugins();

	Ref<EditorExportPreset> _impl_get_runnable_preset_for_platform(
		const Ref<EditorExportPlatform>& p_for_platform) const;
	static Ref<EditorExportPreset> get_runnable_preset_for_platform(
		const Ref<EditorExportPlatform>& p_for_platform);

	void load_config();
	void update_export_presets();
	bool poll_export_platforms();
	bool has_preset_with_name(const String& p_name, int p_exclude_index = -1) const;

	EditorExport();
};

// --- Folded from editor/editor_string_names.h ---
#include "core/os/memory.h"
#include "core/string/ustring.h"

class EditorStrings final
{
public:
	struct Data
	{
		const String Editor = String("Editor");
		const String EditorFonts = String("EditorFonts");
		const String EditorIcons = String("EditorIcons");
		const String EditorStyles = String("EditorStyles");
	};

	static inline Data* data = nullptr;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	static bool is_initialized() { return data != nullptr; }

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (!data) {
			initialize();
		}
		return data;
	}

	EditorStrings() = delete;
	~EditorStrings() = delete;
};

#define EditorString(m_name) EditorStrings::_get_data()->m_name
