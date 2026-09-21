/**************************************************************************/
/*  editor_export_preset.h                                                */
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

class EditorExportPlatform;

#include "core/types.h"

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

	HashMap<StringName, bool> update_visibility;

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
	String _get_property_warning(const StringName& p_name) const;
<<<<<<< HEAD

	static void _bind_methods();
=======
>>>>>>> fix/remove-object


#ifndef DISABLE_DEPRECATED
	int _get_script_export_mode_bind_compat_107167() const;
	static void _bind_compatibility_methods();
#endif

public:
	Ref<EditorExportPlatform> get_platform() const;

	void update_files();
	void update_value_overrides();

	Vector<String> get_files_to_export() const;
	HashSet<String> get_selected_files() const;
	void set_selected_files(const HashSet<String>& p_files);
	int get_customized_files_count() const;

<<<<<<< HEAD
	void add_export_file(const String& p_path);
	void remove_export_file(const String& p_path);
	bool has_export_file(const String& p_path);

	void set_file_export_mode(const String& p_path, FileExportMode p_mode);
	FileExportMode get_file_export_mode(
		const String& p_path, FileExportMode p_default = MODE_FILE_NOT_CUSTOMIZED) const;

	void set_name(const String& p_name);
=======
	bool has_export_file(const String& p_path);

	FileExportMode get_file_export_mode(
		const String& p_path, FileExportMode p_default = MODE_FILE_NOT_CUSTOMIZED) const;

>>>>>>> fix/remove-object
	String get_name() const;

	bool is_runnable() const;

	bool are_advanced_options_enabled() const;
	void set_options_search_active(bool p_active);

	bool is_dedicated_server() const;

	ExportFilter get_export_filter() const;

<<<<<<< HEAD
	void set_include_filter(const String& p_include);
	String get_include_filter() const;

	void set_exclude_filter(const String& p_exclude);
	String get_exclude_filter() const;

	void add_patch(const String& p_path, int p_at_pos = -1);
	void set_patch(int p_index, const String& p_path);

=======
	String get_include_filter() const;

	String get_exclude_filter() const;

>>>>>>> fix/remove-object
	String get_patch(int p_index);

	void set_patches(const Vector<String>& p_patches);
	Vector<String> get_patches() const;

	bool is_patch_delta_encoding_enabled() const;

	int get_patch_delta_zstd_level() const;

	double get_patch_delta_min_reduction() const;

<<<<<<< HEAD
	void set_patch_delta_include_filter(const String& p_filter);
	String get_patch_delta_include_filter() const;

	void set_patch_delta_exclude_filter(const String& p_filter);
	String get_patch_delta_exclude_filter() const;

	void set_custom_features(const String& p_custom_features);
	String get_custom_features() const;

	void set_export_path(const String& p_path);
	String get_export_path() const;

	void set_enc_in_filter(const String& p_filter);
	String get_enc_in_filter() const;

	void set_enc_ex_filter(const String& p_filter);
=======
	String get_patch_delta_include_filter() const;

	String get_patch_delta_exclude_filter() const;

	String get_custom_features() const;

	String get_export_path() const;

	String get_enc_in_filter() const;

>>>>>>> fix/remove-object
	String get_enc_ex_filter() const;

	uint64_t get_seed() const;

	bool get_enc_pck() const;

	bool get_enc_directory() const;

<<<<<<< HEAD
	void set_script_encryption_key(const String& p_key);
=======
>>>>>>> fix/remove-object
	String get_script_encryption_key() const;

	ScriptExportMode get_script_export_mode() const;

	// Return the preset's version number, or fall back to the
	// `application/config/version` project setting if set to an empty string.
	// If `p_windows_version` is `true`, formats the returned version number to
	// be compatible with Windows executable metadata (which requires a
	// 4-component format).
	String get_version(const StringName& p_name, bool p_windows_version = false) const;
};


