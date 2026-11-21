/**************************************************************************/
/*  editor_export.h                                                       */
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

#if !defined(EDITOR_EXPORT_MAIN_SECTION_H) && !defined(IN_EDITOR_EXPORT_PLATFORM_H)
#define EDITOR_EXPORT_MAIN_SECTION_H

#include "editor/export/editor_export_platform.h"

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

#endif // EDITOR_EXPORT_MAIN_SECTION_H
