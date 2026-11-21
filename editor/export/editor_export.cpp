/**************************************************************************/
/*  editor_export.cpp                                                     */
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

#include "editor_export.h"

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/timer.h"

// EditorExportPreset

Ref<EditorExportPlatform> EditorExportPreset::get_platform() const { return platform; }

void EditorExportPreset::update_files()
{
	{
		Vector<String> to_remove;
		for (const String& E : selected_files) {
			if (!FileAccess::exists(E)) {
				to_remove.push_back(E);
			}
		}
		for (int i = 0; i < to_remove.size(); ++i) {
			selected_files.erase(to_remove[i]);
		}
	}

	{
		Vector<String> to_remove;
		for (const KeyValue<String, FileExportMode>& E : customized_files) {
			if (!FileAccess::exists(E.key) && !DirAccess::exists(E.key)) {
				to_remove.push_back(E.key);
			}
		}
		for (int i = 0; i < to_remove.size(); ++i) {
			customized_files.erase(to_remove[i]);
		}
	}
}

Vector<String> EditorExportPreset::get_files_to_export() const
{
	Vector<String> files;
	for (const String& E : selected_files) {
		files.push_back(E);
	}
	return files;
}

HashSet<String> EditorExportPreset::get_selected_files() const
{
	return HashSet<String>(selected_files);
}

void EditorExportPreset::set_selected_files(const HashSet<String>& p_files)
{
	selected_files = p_files;
}

int EditorExportPreset::get_customized_files_count() const { return customized_files.size(); }

String EditorExportPreset::get_name() const { return name; }

bool EditorExportPreset::is_runnable() const
{
	return EditorExport::get_runnable_preset_for_platform(platform).ptr() == this;
}

bool EditorExportPreset::is_dedicated_server() const { return dedicated_server; }

EditorExportPreset::ExportFilter EditorExportPreset::get_export_filter() const
{
	return export_filter;
}

String EditorExportPreset::get_include_filter() const { return include_filter; }

String EditorExportPreset::get_export_path() const { return export_path; }

String EditorExportPreset::get_exclude_filter() const { return exclude_filter; }

bool EditorExportPreset::has_export_file(const String& p_path)
{
	return selected_files.has(p_path);
}

EditorExportPreset::FileExportMode EditorExportPreset::get_file_export_mode(
	const String& p_path, EditorExportPreset::FileExportMode p_default) const
{
	HashMap<String, FileExportMode>::ConstIterator i = customized_files.find(p_path);
	if (i) {
		return i->value;
	}
	return p_default;
}

String EditorExportPreset::get_patch(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, patches.size(), String());
	return patches[p_index];
}

void EditorExportPreset::set_patches(const Vector<String>& p_patches) { patches = p_patches; }

Vector<String> EditorExportPreset::get_patches() const { return patches; }

bool EditorExportPreset::is_patch_delta_encoding_enabled() const
{
	return patch_delta_encoding_enabled;
}

int EditorExportPreset::get_patch_delta_zstd_level() const { return patch_delta_zstd_level; }

double EditorExportPreset::get_patch_delta_min_reduction() const
{
	return patch_delta_min_reduction;
}

String EditorExportPreset::get_patch_delta_include_filter() const
{
	return patch_delta_include_filter;
}

String EditorExportPreset::get_patch_delta_exclude_filter() const
{
	return patch_delta_exclude_filter;
}

String EditorExportPreset::get_custom_features() const { return custom_features; }

String EditorExportPreset::get_enc_in_filter() const { return enc_in_filters; }

String EditorExportPreset::get_enc_ex_filter() const { return enc_ex_filters; }

uint64_t EditorExportPreset::get_seed() const { return seed; }

bool EditorExportPreset::get_enc_pck() const { return enc_pck; }

bool EditorExportPreset::get_enc_directory() const { return enc_directory; }

String EditorExportPreset::get_script_encryption_key() const { return script_key; }

EditorExportPreset::ScriptExportMode EditorExportPreset::get_script_export_mode() const
{
	return script_mode;
}

// ProjectZIPPacker

void ProjectZIPPacker::pack_project_zip(const String& p_path)
{
	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);

	String resource_path = ProjectSettings::get_resource_path();
	const String base_path = resource_path.substr(0, resource_path.rfind_char('/')) + "/";

	zipFile zip = zipOpen2(p_path.utf8().get_data(), APPEND_STATUS_CREATE, nullptr, &io);
	_zip_recursive(resource_path, base_path, zip);
	zipClose(zip, nullptr);
}

void ProjectZIPPacker::_zip_recursive(
	const String& p_path, const String& p_base_path, zipFile p_zip)
{
	Ref<DirAccess> dir = DirAccess::open(p_path);
	if (dir.is_null()) {
		WARN_PRINT("Unable to open directory for zipping: " + p_path);
		return;
	}
	dir->list_dir_begin();
	String cur = dir->get_next();
	String project_data_dir_name = ProjectSettings::get_project_data_dir_name();
	while (!cur.is_empty()) {
		String cs = p_path.path_join(cur);
		if (cur == "." || cur == ".." || cur == project_data_dir_name) {
			// Skip
		}
		else if (dir->current_is_dir()) {
			String path = cs.trim_prefix(p_base_path) + "/";
			zipOpenNewFileInZip4(p_zip, path.utf8().get_data(), nullptr, nullptr, 0, nullptr, 0,
				nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0, -MAX_WBITS, DEF_MEM_LEVEL,
				Z_DEFAULT_STRATEGY, nullptr, 0,
				0x0314,
				1 << 11);
			zipCloseFileInZip(p_zip);
			_zip_recursive(cs, p_base_path, p_zip);
		}
		else {
			_zip_file(cs, p_base_path, p_zip);
		}
		cur = dir->get_next();
	}
}

void ProjectZIPPacker::_zip_file(String const&, String const&, void*) {}

// EditorExport

void EditorExport::add_export_platform(const Ref<EditorExportPlatform>& p_platform)
{
	ERR_FAIL_NULL(EditorExport::instance);
	EditorExport::instance->_impl_add_export_platform(p_platform);
}

void EditorExport::_impl_add_export_platform(const Ref<EditorExportPlatform>& p_platform)
{
	p_platform->initialize();
	export_platforms.push_back(p_platform);

	should_update_presets = true;
	should_reload_presets = true;
}

void EditorExport::remove_export_platform(const Ref<EditorExportPlatform>& p_platform)
{
	ERR_FAIL_NULL(EditorExport::instance);
	EditorExport::instance->_impl_remove_export_platform(p_platform);
}

void EditorExport::_impl_remove_export_platform(const Ref<EditorExportPlatform>& p_platform)
{
	export_platforms.erase(p_platform);
	p_platform->cleanup();

	should_update_presets = true;
	should_reload_presets = true;
}

int EditorExport::get_export_platform_count()
{
	return EditorExport::instance->_impl_get_export_platform_count();
}

int EditorExport::_impl_get_export_platform_count() const { return export_platforms.size(); }

int EditorExport::get_export_platform_index_by_name(const String& p_name)
{
	for (int j = 0; j < get_export_platform_count(); j++) {
		Ref<EditorExportPlatform> plat = get_export_platform(j);
		if (!plat.is_null() && plat->get_name().nocasecmp_to(p_name) == 0) {
			return j;
		}
	}
	return -1;
}

bool EditorExport::has_preset_with_name(const String& p_name, int p_exclude_index) const
{
	for (int i = 0; i < export_presets.size(); i++) {
		if (i == p_exclude_index) {
			continue;
		}
		if (export_presets[i]->get_name() == p_name) {
			return true;
		}
	}

	return false;
}

Ref<EditorExportPlatform> EditorExport::get_export_platform(int p_idx)
{
	return EditorExport::instance->_impl_get_export_platform(p_idx);
}

Ref<EditorExportPlatform> EditorExport::_impl_get_export_platform(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, export_platforms.size(), Ref<EditorExportPlatform>());

	return export_platforms[p_idx];
}

void EditorExport::add_export_preset(const Ref<EditorExportPreset>& p_preset, int p_at_pos)
{
	if (p_at_pos < 0) {
		export_presets.push_back(p_preset);
	}
	else {
		export_presets.insert(p_at_pos, p_preset);
	}
	emit_presets_runnable_changed();
}

int EditorExport::get_export_preset_count()
{
	return EditorExport::instance->_impl_get_export_preset_count();
}

int EditorExport::_impl_get_export_preset_count() const { return export_presets.size(); }

Ref<EditorExportPreset> EditorExport::get_export_preset(int p_idx)
{
	return EditorExport::instance->_impl_get_export_preset(p_idx);
}

Ref<EditorExportPreset> EditorExport::_impl_get_export_preset(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, export_presets.size(), Ref<EditorExportPreset>());
	return export_presets[p_idx];
}

void EditorExport::add_export_plugin(const Ref<EditorExportPlugin>& p_plugin)
{
	ERR_FAIL_NULL(EditorExport::instance);
	EditorExport::instance->_impl_add_export_plugin(p_plugin);
}

void EditorExport::_impl_add_export_plugin(const Ref<EditorExportPlugin>& p_plugin)
{
	if (!export_plugins.has(p_plugin)) {
		export_plugins.push_back(p_plugin);
		should_update_presets = true;
	}
}

void EditorExport::remove_export_plugin(const Ref<EditorExportPlugin>& p_plugin)
{
	ERR_FAIL_NULL(EditorExport::instance);
	EditorExport::instance->_impl_remove_export_plugin(p_plugin);
}

void EditorExport::_impl_remove_export_plugin(const Ref<EditorExportPlugin>& p_plugin)
{
	export_plugins.erase(p_plugin);
	should_update_presets = true;
}

Vector<Ref<EditorExportPlugin>> EditorExport::get_export_plugins()
{
	return EditorExport::instance->_impl_get_export_plugins();
}

Vector<Ref<EditorExportPlugin>> EditorExport::_impl_get_export_plugins() { return export_plugins; }

Ref<EditorExportPreset> EditorExport::get_runnable_preset_for_platform(
	const Ref<EditorExportPlatform>& p_for_platform)
{
	return EditorExport::instance->_impl_get_runnable_preset_for_platform(p_for_platform);
}

Ref<EditorExportPreset> EditorExport::_impl_get_runnable_preset_for_platform(
	const Ref<EditorExportPlatform>& p_for_platform) const
{
	const Ref<EditorExportPreset>* preset = runnable_presets.getptr(p_for_platform);
	return preset ? *preset : Ref<EditorExportPreset>();
}

bool EditorExport::poll_export_platforms()
{
	bool changed = false;
	for (int i = 0; i < export_platforms.size(); i++) {
		if (export_platforms.write[i]->poll_export()) {
			changed = true;
		}
	}

	return changed;
}

void EditorExport::emit_presets_runnable_changed() {}
