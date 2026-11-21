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
#define IN_EDITOR_EXPORT_PLATFORM_H
#include "editor/export/editor_export.h"
#undef IN_EDITOR_EXPORT_PLATFORM_H
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
#include "core/os/shared_object.h"

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
