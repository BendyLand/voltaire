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

// ============================================================================
// Merged from editor/export/editor_export_platform.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/compression.h"
#include "core/io/dir_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/image.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/io/zip_io.h"
#include "core/math/random_pcg.h"
#include "core/os/os.h"
#include "editor/export/editor_export_platform.h"
#include "core/string/translation.h"
#include "core/version.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_platform.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
// Inlined editor/export/editor_export_platform.compat.inc
#ifndef DISABLE_DEPRECATED

#include "editor_export_platform.h"

Vector<String> EditorExportPlatform::_get_forced_export_files_bind_compat_71542()
{
	return get_forced_export_files(Ref<EditorExportPreset>());
}

Error EditorExportPlatform::_export_project_bind_compat_118787(
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path,
	uint32_t p_flags)
{
	return export_project(p_preset, p_debug, p_path, p_flags, true);
}

void EditorExportPlatform::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED



#include "editor_export_platform.h"
#include "scene/gui/dialogs.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/texture.h"

class EditorExportSaveProxy
{
	HashSet<String> saved_paths;
	EditorExportPlatform::EditorExportSaveFunction save_func;
	bool tracking_saves = false;

public:
	bool has_saved(const String& p_path) const { return saved_paths.has(p_path); }

	Error save_file(const Ref<EditorExportPreset>& p_preset, void* p_userdata, const String& p_path,
		const Vector<uint8_t>& p_data, int p_file, int p_total,
		const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
		const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
	{
		if (tracking_saves) {
			saved_paths.insert(p_path.simplify_path().trim_prefix("res://"));
		}

		return save_func(p_preset, p_userdata, p_path, p_data, p_file, p_total, p_enc_in_filters,
			p_enc_ex_filters, p_key, p_seed, p_delta);
	}

	EditorExportSaveProxy(
		EditorExportPlatform::EditorExportSaveFunction p_save_func, bool p_track_saves)
		: save_func(p_save_func), tracking_saves(p_track_saves)
	{
	}
};

static int _get_pad(int p_alignment, int p_n)
{
	int rest = p_n % p_alignment;
	int pad = 0;
	if (rest > 0) {
		pad = p_alignment - rest;
	};

	return pad;
}

static constexpr int PCK_PADDING = 16;

Ref<Image> EditorExportPlatform::_load_icon_or_splash_image(
	const String& p_path, Error* r_error) const
{
	Ref<Image> image;

	if (!p_path.is_empty() && ResourceLoader::exists(p_path) &&
		!ResourceLoader::get_resource_type(p_path).is_empty()) {
		Ref<Texture2D> texture =
			ResourceLoader::load(p_path, "", ResourceFormatLoader::CACHE_MODE_IGNORE, r_error);
		if (texture.is_valid()) {
			image = texture->get_image();
			if (image.is_valid() && image->is_compressed()) {
				image->decompress();
			}
		}
	}
	if (image.is_null()) {
		image.instantiate();
		Error err = ImageLoader::load_image(p_path, image);
		if (r_error) {
			*r_error = err;
		}
	}
	return image;
}

Error EditorExportPlatform::_extract_android_assets(
	const String& p_bundle_path, String& r_pck_path, String& r_temp_dir)
{
	Error err = OK;

	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);
	unzFile zip_file = unzOpen2(p_bundle_path.utf8().get_data(), &io);
	if (!zip_file) {
		return ERR_FILE_CANT_OPEN;
	}

	const char* pck_name = "assets.sparsepck";
	String pck_base_dir;

	int ret = unzGoToFirstFile(zip_file);
	while (ret == UNZ_OK) {
		unz_file_info64 file_info = {};
		char file_name_buf[16384];
		ret = unzGetCurrentFileInfo64(
			zip_file, &file_info, file_name_buf, sizeof(file_name_buf), nullptr, 0, nullptr, 0);
		if (ret != UNZ_OK) {
			break;
		}

		String file_name = String::utf8(file_name_buf);
		if (file_name.ends_with(pck_name)) {
			pck_base_dir = file_name.trim_suffix(pck_name);
			break;
		}

		ret = unzGoToNextFile(zip_file);
	}

	if (ret != UNZ_OK || pck_base_dir.is_empty()) {
		unzClose(zip_file);
		return ERR_FILE_UNRECOGNIZED;
	}

	Ref<DirAccess> temp_dir = DirAccess::create_temp("export_patch_base", true, &err);
	if (err != OK) {
		unzClose(zip_file);
		return err;
	}

	String temp_dir_path = temp_dir->get_current_dir();

	ret = unzGoToFirstFile(zip_file);
	while (ret == UNZ_OK) {
		unz_file_info64 zip_file_info = {};
		char file_name_buf[16384];
		if (unzGetCurrentFileInfo64(zip_file, &zip_file_info, file_name_buf, sizeof(file_name_buf),
				nullptr, 0, nullptr, 0) != UNZ_OK) {
			err = ERR_FILE_CORRUPT;
			break;
		}

		String file_name = String::utf8(file_name_buf);
		if (!file_name.begins_with(pck_base_dir)) {
			ret = unzGoToNextFile(zip_file);
			continue;
		}

		String file_path_relative = file_name.trim_prefix(pck_base_dir).simplify_path();
		if (file_path_relative.is_empty()) {
			ret = unzGoToNextFile(zip_file);
			continue;
		}

		String file_output_path = temp_dir_path.path_join(file_path_relative);
		err = DirAccess::make_dir_recursive_absolute(file_output_path.get_base_dir());
		if (err != OK) {
			break;
		}

		if (unzOpenCurrentFile(zip_file) != UNZ_OK) {
			err = ERR_FILE_CANT_OPEN;
			break;
		}

		LocalVector<uint8_t> uncomp_data;
		uncomp_data.resize(zip_file_info.uncompressed_size);
		int read_bytes = unzReadCurrentFile(zip_file, uncomp_data.ptr(), uncomp_data.size());
		unzCloseCurrentFile(zip_file);

		if (read_bytes < 0 || read_bytes != (int)uncomp_data.size()) {
			err = ERR_FILE_CANT_READ;
			break;
		}

		Ref<FileAccess> temp_file = FileAccess::open(file_output_path, FileAccess::WRITE, &err);
		if (err != OK) {
			break;
		}

		if (!temp_file->store_buffer(uncomp_data.ptr(), uncomp_data.size())) {
			err = ERR_FILE_CANT_WRITE;
			break;
		}

		ret = unzGoToNextFile(zip_file);
	}

	unzClose(zip_file);

	r_pck_path = temp_dir_path.path_join(pck_name);
	r_temp_dir = temp_dir_path;

	return err;
}

Error EditorExportPlatform::_load_patches(
	const Ref<EditorExportPreset>& p_preset, const Vector<String>& p_patches)
{
	if (!p_patches.is_empty()) {
		for (const String& path : p_patches) {
			String pck_path = path;

			if (path.ends_with(".apk") || path.ends_with(".aab")) {
				String temp_dir;
				Error err = _extract_android_assets(path, pck_path, temp_dir);
				if (err != OK) {
					_unload_patches();
					add_message(EXPORT_MESSAGE_ERROR, TTR("Patch Creation"),
						vformat(TTR("Could not extract assets from Android bundle \"%s\", due to "
									"error \"%s\"."),
							path, error_names[err]));
					return err;
				}

				patch_temp_dirs.push_back(temp_dir);
			}

			Error err = PackedData::add_pack(
				pck_path, true, 0, _get_script_encryption_key_bytes(p_preset));
			if (err != OK) {
				_unload_patches();
				add_message(EXPORT_MESSAGE_ERROR, TTR("Patch Creation"),
					vformat(TTR("Could not load patch pack with path \"%s\"."), pck_path));
				return err;
			}
		}
	}

	return OK;
}

void EditorExportPlatform::_unload_patches()
{
	PackedData::clear();

	for (const String& temp_dir : patch_temp_dirs) {
		Ref<DirAccess> temp_dir_da = DirAccess::open(temp_dir);
		if (temp_dir_da.is_valid()) {
			temp_dir_da->erase_contents_recursive();
			temp_dir_da->remove(temp_dir);
		}
	}

	patch_temp_dirs.clear();
}

Error EditorExportPlatform::_encrypt_and_store_data(Ref<FileAccess> p_fd, const String& p_path,
	const Vector<uint8_t>& p_data, const Vector<String>& p_enc_in_filters,
	const Vector<String>& p_enc_ex_filters, const Vector<uint8_t>& p_key, uint64_t p_seed,
	bool& r_encrypt)
{
	r_encrypt = false;
	for (int i = 0; i < p_enc_in_filters.size(); ++i) {
		if (p_path.matchn(p_enc_in_filters[i]) ||
			p_path.trim_prefix("res://").matchn(p_enc_in_filters[i])) {
			r_encrypt = true;
			break;
		}
	}

	for (int i = 0; i < p_enc_ex_filters.size(); ++i) {
		if (p_path.matchn(p_enc_ex_filters[i]) ||
			p_path.trim_prefix("res://").matchn(p_enc_ex_filters[i])) {
			r_encrypt = false;
			break;
		}
	}

	Ref<FileAccessEncrypted> fae;
	Ref<FileAccess> ftmp = p_fd;
	if (r_encrypt) {
		Vector<uint8_t> iv;
		if (p_seed != 0) {
			uint64_t seed = p_seed;

			const uint8_t* ptr = p_data.ptr();
			int64_t len = p_data.size();
			for (int64_t i = 0; i < len; i++) {
				seed = ((seed << 5) + seed) ^ ptr[i];
			}

			RandomPCG rng = RandomPCG(seed);
			iv.resize(16);
			for (int i = 0; i < 16; i++) {
				iv.write[i] = rng.rand() % 256;
			}
		}

		fae.instantiate();
		ERR_FAIL_COND_V(fae.is_null(), ERR_FILE_CANT_OPEN);

		Error err =
			fae->open_and_parse(ftmp, p_key, FileAccessEncrypted::MODE_WRITE_AES256, false, iv);
		ERR_FAIL_COND_V(err != OK, ERR_FILE_CANT_OPEN);
		ftmp = fae;
	}

	// Store file content.
	ftmp->store_buffer(p_data.ptr(), p_data.size());

	if (fae.is_valid()) {
		ftmp.unref();
		fae.unref();
	}
	return OK;
}

String EditorExportPlatform::find_export_template(
	const String& template_file_name, String* err) const
{
	String current_version = VLTR_VERSION_FULL_CONFIG;
	String template_path = EditorPaths::get_export_templates_dir()
							   .path_join(current_version)
							   .path_join(template_file_name);

	if (FileAccess::exists(template_path)) {
		return template_path;
	}

	// Not found
	if (err) {
		*err += TTR("No export template found at the expected path:") + "\n" + template_path + "\n";
	}
	return String();
}

bool EditorExportPlatform::exists_export_template(
	const String& template_file_name, String* err) const
{
	return find_export_template(template_file_name, err) != "";
}

void EditorExportPlatform::_export_find_resources(
	EditorFileSystemDirectory* p_dir, HashSet<String>& p_paths)
{
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_export_find_resources(p_dir->get_subdir(i), p_paths);
	}

	for (int i = 0; i < p_dir->get_file_count(); i++) {
		if (p_dir->get_file_type(i) == "TextFile") {
			continue;
		}
		p_paths.insert(p_dir->get_file_path(i));
	}
}

void EditorExportPlatform::_export_find_customized_resources(
	const Ref<EditorExportPreset>& p_preset, EditorFileSystemDirectory* p_dir,
	EditorExportPreset::FileExportMode p_mode, HashSet<String>& p_paths)
{
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		EditorFileSystemDirectory* subdir = p_dir->get_subdir(i);
		_export_find_customized_resources(
			p_preset, subdir, p_preset->get_file_export_mode(subdir->get_path(), p_mode), p_paths);
	}

	for (int i = 0; i < p_dir->get_file_count(); i++) {
		if (p_dir->get_file_type(i) == "TextFile") {
			continue;
		}
		String path = p_dir->get_file_path(i);
		EditorExportPreset::FileExportMode file_mode = p_preset->get_file_export_mode(path, p_mode);
		if (file_mode != EditorExportPreset::MODE_FILE_REMOVE) {
			p_paths.insert(path);
		}
	}
}

void EditorExportPlatform::_export_find_dependencies(const String& p_path, HashSet<String>& p_paths)
{
	if (p_paths.has(p_path)) {
		return;
	}

	p_paths.insert(p_path);

	EditorFileSystemDirectory* dir;
	int file_idx;
	dir = EditorFileSystem::find_file(p_path, &file_idx);
	if (!dir) {
		return;
	}

	Vector<String> deps = dir->get_file_deps(file_idx);

	for (int i = 0; i < deps.size(); i++) {
		_export_find_dependencies(deps[i], p_paths);
	}
}

void EditorExportPlatform::_edit_files_with_filter(
	Ref<DirAccess>& da, const Vector<String>& p_filters, HashSet<String>& r_list, bool exclude)
{
	da->list_dir_begin();
	String cur_dir = da->get_current_dir().replace_char('\\', '/');
	if (!cur_dir.ends_with("/")) {
		cur_dir += "/";
	}
	String cur_dir_no_prefix = cur_dir.replace("res://", "");

	Vector<String> dirs;
	String f = da->get_next();
	while (!f.is_empty()) {
		if (da->current_is_dir()) {
			dirs.push_back(f);
		}
		else {
			String fullpath = cur_dir + f;
			// Test also against path without res:// so that filters like `file.txt` can work.
			String fullpath_no_prefix = cur_dir_no_prefix + f;
			for (int i = 0; i < p_filters.size(); ++i) {
				if (fullpath.matchn(p_filters[i]) || fullpath_no_prefix.matchn(p_filters[i])) {
					if (!exclude) {
						r_list.insert(fullpath);
					}
					else {
						r_list.erase(fullpath);
					}
				}
			}
		}
		f = da->get_next();
	}

	da->list_dir_end();

	for (int i = 0; i < dirs.size(); ++i) {
		const String& dir = dirs[i];
		if (dir.begins_with(".")) {
			continue;
		}

		if (EditorFileSystem::_should_skip_directory(cur_dir + dir)) {
			continue;
		}

		da->change_dir(dir);
		_edit_files_with_filter(da, p_filters, r_list, exclude);
		da->change_dir("..");
	}
}

void EditorExportPlatform::_edit_filter_list(
	HashSet<String>& r_list, const String& p_filter, bool exclude)
{
	if (p_filter.is_empty()) {
		return;
	}
	Vector<String> split = p_filter.split(",");
	Vector<String> filters;
	for (int i = 0; i < split.size(); i++) {
		String f = split[i].strip_edges();
		if (f.is_empty()) {
			continue;
		}
		filters.push_back(f);
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	ERR_FAIL_COND(da.is_null());
	_edit_files_with_filter(da, filters, r_list, exclude);
}

HashSet<String> EditorExportPlatform::get_features(
	const Ref<EditorExportPreset>& p_preset, bool p_debug) const
{
	Ref<EditorExportPlatform> platform = p_preset->get_platform();
	List<String> feature_list;
	platform->get_platform_features(&feature_list);
	platform->get_preset_features(p_preset, &feature_list);

	HashSet<String> result;
	for (const String& E : feature_list) {
		result.insert(E);
	}

	result.insert("template");
	if (p_debug) {
		result.insert("debug");
		result.insert("template_debug");
	}
	else {
		result.insert("release");
		result.insert("template_release");
	}

#ifdef REAL_T_IS_DOUBLE
	result.insert("double");
#else
	result.insert("single");
#endif // REAL_T_IS_DOUBLE

	if (!p_preset->get_custom_features().is_empty()) {
		Vector<String> tmp_custom_list = p_preset->get_custom_features().split(",");

		for (int i = 0; i < tmp_custom_list.size(); i++) {
			String f = tmp_custom_list[i].strip_edges();
			if (!f.is_empty()) {
				result.insert(f);
			}
		}
	}

	return result;
}

EditorExportPlatform::ExportNotifier::ExportNotifier(EditorExportPlatform& p_platform,
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path, uint32_t p_flags,
	bool p_enabled)
{
	enabled = p_enabled;
	if (!enabled) {
		return;
	}
	HashSet<String> features = p_platform.get_features(p_preset, p_debug);
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_export_plugins();
	// initial export plugin callback
	for (int i = 0; i < export_plugins.size(); i++) {
		export_plugins.write[i]->set_export_preset(p_preset);
		export_plugins.write[i]->_export_begin(features, p_debug, p_path, p_flags);
	}
}

EditorExportPlatform::ExportNotifier::~ExportNotifier()
{
	if (!enabled) {
		return;
	}
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_export_plugins();
	for (int i = 0; i < export_plugins.size(); i++) {
		export_plugins.write[i]->_export_end();
		export_plugins.write[i]->_export_end_clear();
		export_plugins.write[i]->set_export_preset(Ref<EditorExportPreset>());
	}
}

bool EditorExportPlatform::_is_editable_ancestor(Node* p_root, Node* p_node)
{
	while (p_node != nullptr && p_node != p_root) {
		if (p_root->is_editable_instance(p_node)) {
			return true;
		}
		p_node = p_node->get_owner();
	}
	return false;
}

String EditorExportPlatform::_get_script_encryption_key(const Ref<EditorExportPreset>& p_preset)
{
	const String from_env = OS::get_environment(ENV_SCRIPT_ENCRYPTION_KEY);
	if (!from_env.is_empty()) {
		return from_env.to_lower();
	}
	return p_preset->get_script_encryption_key().to_lower();
}

Vector<uint8_t> EditorExportPlatform::_get_script_encryption_key_bytes(
	const Ref<EditorExportPreset>& p_preset)
{
	Vector<uint8_t> key;
	String script_key = _get_script_encryption_key(p_preset);
	if (script_key.length() == 64) {
		key.resize(32);
		for (int i = 0; i < 32; i++) {
			int v = 0;
			if (i * 2 < script_key.length()) {
				char32_t ct = script_key[i * 2];
				if (is_digit(ct)) {
					ct = ct - '0';
				}
				else if (ct >= 'a' && ct <= 'f') {
					ct = 10 + ct - 'a';
				}
				v |= ct << 4;
			}

			if (i * 2 + 1 < script_key.length()) {
				char32_t ct = script_key[i * 2 + 1];
				if (is_digit(ct)) {
					ct = ct - '0';
				}
				else if (ct >= 'a' && ct <= 'f') {
					ct = 10 + ct - 'a';
				}
				v |= ct;
			}
			key.write[i] = v;
		}
	}

	return key;
}

// Used by the main export function to filter excluded global classes, extensions
// and UIDs based on excluded resources configured in the export preset.

Error EditorExportPlatform::_pack_add_shared_object(
	const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so)
{
	PackData* pack_data = (PackData*)p_userdata;
	if (pack_data->so_files) {
		pack_data->so_files->push_back(p_so);
	}

	return OK;
}

Error EditorExportPlatform::_remove_pack_file(
	const Ref<EditorExportPreset>& p_preset, void* p_userdata, const String& p_path)
{
	PackData* pd = (PackData*)p_userdata;

	SavedData sd;
	sd.path_utf8 = p_path.utf8();
	sd.ofs = pd->f->get_position();
	sd.size = 0;
	sd.removal = true;

	// This padding will likely never be added, as we should already be aligned when removals are
	// added.
	int pad = _get_pad(PCK_PADDING, pd->f->get_position());
	for (int i = 0; i < pad; i++) {
		pd->f->store_8(0);
	}

	sd.md5.resize_initialized(16);

	pd->file_ofs.push_back(sd);

	return OK;
}

Error EditorExportPlatform::_zip_add_shared_object(
	const Ref<EditorExportPreset>& p_preset, void* p_userdata, const SharedObject& p_so)
{
	ZipData* zip_data = (ZipData*)p_userdata;
	if (zip_data->so_files) {
		zip_data->so_files->push_back(p_so);
	}

	return OK;
}

void EditorExportPlatform::zip_folder_recursive(
	zipFile& p_zip, const String& p_root_path, const String& p_folder, const String& p_pkg_name)
{
	String dir = p_folder.is_empty() ? p_root_path : p_root_path.path_join(p_folder);

	Ref<DirAccess> da = DirAccess::open(dir);
	ERR_FAIL_COND(da.is_null());

	da->list_dir_begin();
	String f = da->get_next();
	while (!f.is_empty()) {
		if (f == "." || f == "..") {
			f = da->get_next();
			continue;
		}
		if (da->is_link(f)) {
			OS::DateTime dt = OS::get_datetime();

			zip_fileinfo zipfi;
			zipfi.tmz_date.tm_year = dt.year;
			zipfi.tmz_date.tm_mon =
				dt.month - 1; // Note: "tm" month range - 0..11, Godot month range - 1..12,
							  // https://www.cplusplus.com/reference/ctime/tm/
			zipfi.tmz_date.tm_mday = dt.day;
			zipfi.tmz_date.tm_hour = dt.hour;
			zipfi.tmz_date.tm_min = dt.minute;
			zipfi.tmz_date.tm_sec = dt.second;
			zipfi.dosDate = 0;
			// 0120000: symbolic link type
			// 0000755: permissions rwxr-xr-x
			// 0000644: permissions rw-r--r--
			uint32_t _mode = 0120644;
			zipfi.external_fa =
				(_mode << 16L) |
				((_mode & 0200) ? 0 : 1); // UUUUUUUUUUUUUUUU0000000000ADVSHR: Unix permissions (U)
										  // + DOS read-only flag (R).
			zipfi.internal_fa = 0;

			zipOpenNewFileInZip4(p_zip, p_folder.path_join(f).utf8().get_data(), &zipfi, nullptr, 0,
				nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0, -MAX_WBITS,
				DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, nullptr, 0,
				0x0314, // "version made by", 0x03 - Unix, 0x14 - ZIP specification version 2.0,
						// required to store Unix file permissions
				1 << 11); // Bit 11 is the language encoding flag. When set, filename and comment
						  // fields must be encoded using UTF-8.

			const CharString target_utf8 = da->read_link(f).utf8();
			zipWriteInFileInZip(p_zip, target_utf8.get_data(), target_utf8.size());
			zipCloseFileInZip(p_zip);
		}
		else if (da->current_is_dir()) {
			zip_folder_recursive(p_zip, p_root_path, p_folder.path_join(f), p_pkg_name);
		}
		else {
			bool _is_executable = is_executable(dir.path_join(f));

			OS::DateTime dt = OS::get_datetime();

			zip_fileinfo zipfi;
			zipfi.tmz_date.tm_year = dt.year;
			zipfi.tmz_date.tm_mon =
				dt.month - 1; // Note: "tm" month range - 0..11, Godot month range - 1..12,
							  // https://www.cplusplus.com/reference/ctime/tm/
			zipfi.tmz_date.tm_mday = dt.day;
			zipfi.tmz_date.tm_hour = dt.hour;
			zipfi.tmz_date.tm_min = dt.minute;
			zipfi.tmz_date.tm_sec = dt.second;
			zipfi.dosDate = 0;
			// 0100000: regular file type
			// 0000755: permissions rwxr-xr-x
			// 0000644: permissions rw-r--r--
			uint32_t _mode = (_is_executable ? 0100755 : 0100644);
			zipfi.external_fa =
				(_mode << 16L) |
				((_mode & 0200) ? 0 : 1); // UUUUUUUUUUUUUUUU0000000000ADVSHR: Unix permissions (U)
										  // + DOS read-only flag (R).
			zipfi.internal_fa = 0;

			zipOpenNewFileInZip4(p_zip, p_folder.path_join(f).utf8().get_data(), &zipfi, nullptr, 0,
				nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0, -MAX_WBITS,
				DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, nullptr, 0,
				0x0314, // "version made by", 0x03 - Unix, 0x14 - ZIP specification version 2.0,
						// required to store Unix file permissions
				1 << 11); // Bit 11 is the language encoding flag. When set, filename and comment
						  // fields must be encoded using UTF-8.

			Ref<FileAccess> fa = FileAccess::open(dir.path_join(f), FileAccess::READ);
			if (fa.is_null()) {
				add_message(EXPORT_MESSAGE_ERROR, TTR("ZIP Creation"),
					vformat(
						TTR("Could not open file to read from path \"%s\"."), dir.path_join(f)));
				return;
			}
			const int bufsize = 16384;
			uint8_t buf[bufsize];

			while (true) {
				uint64_t got = fa->get_buffer(buf, bufsize);
				if (got == 0) {
					break;
				}
				zipWriteInFileInZip(p_zip, buf, got);
			}

			zipCloseFileInZip(p_zip);
		}
		f = da->get_next();
	}
	da->list_dir_end();
}

bool EditorExportPlatform::_store_header(Ref<FileAccess> p_fd, bool p_enc, bool p_sparse,
	uint64_t& r_file_base_ofs, uint64_t& r_dir_base_ofs, const String& p_salt)
{
	p_fd->store_32(PACK_HEADER_MAGIC);
	p_fd->store_32(PACK_FORMAT_VERSION);
	p_fd->store_32(VLTR_VERSION_MAJOR);
	p_fd->store_32(VLTR_VERSION_MINOR);
	p_fd->store_32(VLTR_VERSION_PATCH);

	uint32_t pack_flags = PACK_REL_FILEBASE;
	if (p_enc) {
		pack_flags |= PACK_DIR_ENCRYPTED;
	}
	if (p_sparse) {
		pack_flags |= PACK_SPARSE_BUNDLE;
	}
	p_fd->store_32(pack_flags); // Flags.

	r_file_base_ofs = p_fd->get_position();
	p_fd->store_64(0); // Files base offset.

	r_dir_base_ofs = p_fd->get_position();
	p_fd->store_64(0); // Directory offset.

	if (p_enc && p_sparse && p_salt.length() == 32) {
		CharString cs = p_salt.latin1();
		p_fd->store_buffer((const uint8_t*)cs.ptr(), 32);
	}
	else {
		for (int i = 0; i < 8; i++) {
			// Reserved.
			p_fd->store_32(0);
		}
	}

	for (int i = 0; i < 8; i++) {
		// Reserved.
		p_fd->store_32(0);
	}
	return true;
}

bool EditorExportPlatform::_encrypt_and_store_directory(Ref<FileAccess> p_fd, PackData& p_pack_data,
	const Vector<uint8_t>& p_key, uint64_t p_seed, uint64_t p_file_base)
{
	Ref<FileAccessEncrypted> fae;
	Ref<FileAccess> fhead = p_fd;

	fhead->store_32(p_pack_data.file_ofs.size()); // amount of files

	if (!p_key.is_empty()) {
		uint64_t seed = p_seed;
		fae.instantiate();
		if (fae.is_null()) {
			return false;
		}

		Vector<uint8_t> iv;
		if (seed != 0) {
			for (int i = 0; i < p_pack_data.file_ofs.size(); i++) {
				for (int64_t j = 0; j < p_pack_data.file_ofs[i].path_utf8.length(); j++) {
					seed = ((seed << 5) + seed) ^ p_pack_data.file_ofs[i].path_utf8.get_data()[j];
				}
				for (int64_t j = 0; j < p_pack_data.file_ofs[i].md5.size(); j++) {
					seed = ((seed << 5) + seed) ^ p_pack_data.file_ofs[i].md5[j];
				}
				seed = ((seed << 5) + seed) ^ (p_pack_data.file_ofs[i].ofs - p_file_base);
				seed = ((seed << 5) + seed) ^ p_pack_data.file_ofs[i].size;
			}

			RandomPCG rng = RandomPCG(seed);
			iv.resize(16);
			for (int i = 0; i < 16; i++) {
				iv.write[i] = rng.rand() % 256;
			}
		}

		Error err =
			fae->open_and_parse(fhead, p_key, FileAccessEncrypted::MODE_WRITE_AES256, false, iv);
		if (err != OK) {
			return false;
		}

		fhead = fae;
	}
	for (int i = 0; i < p_pack_data.file_ofs.size(); i++) {
		uint32_t string_len = p_pack_data.file_ofs[i].path_utf8.length();
		uint32_t pad = _get_pad(4, string_len);

		fhead->store_32(string_len + pad);
		fhead->store_buffer(
			(const uint8_t*)p_pack_data.file_ofs[i].path_utf8.get_data(), string_len);
		for (uint32_t j = 0; j < pad; j++) {
			fhead->store_8(0);
		}

		fhead->store_64(p_pack_data.file_ofs[i].ofs - p_file_base);
		fhead->store_64(p_pack_data.file_ofs[i].size); // pay attention here, this is where file is
		fhead->store_buffer(p_pack_data.file_ofs[i].md5.ptr(), 16); // also save md5 for file
		uint32_t flags = 0;
		if (p_pack_data.file_ofs[i].encrypted) {
			flags |= PACK_FILE_ENCRYPTED;
		}
		if (p_pack_data.file_ofs[i].removal) {
			flags |= PACK_FILE_REMOVAL;
		}
		if (p_pack_data.file_ofs[i].delta) {
			flags |= PACK_FILE_DELTA;
		}
		fhead->store_32(flags);
	}

	if (fae.is_valid()) {
		fhead.unref();
		fae.unref();
	}
	return true;
}

String EditorExportPlatform::simplify_path(const String& p_path)
{
	if (p_path.begins_with("uid://")) {
		const String path = ResourceUID::uid_to_path(p_path);
		print_verbose(vformat(
			R"(UID-referenced exported file name "%s" was replaced with "%s".)", p_path, path));
		return path.simplify_path();
	}
	else {
		return p_path.simplify_path();
	}
}

void EditorExportPlatform::get_preset_features(
	const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const
{
}

bool EditorExportPlatform::fill_log_messages(RichTextLabel* p_log, Error p_err) { return true; }

Ref<EditorExportPreset> EditorExportPlatform::create_preset()
{
	Ref<EditorExportPreset> eep = memnew(EditorExportPreset);
	return eep;
}

bool EditorExportPlatform::can_export(const Ref<EditorExportPreset>& p_preset, String& r_error,
	bool& r_missing_templates, bool p_debug) const
{
	return true;
}

void EditorExportPlatform::get_export_options(List<ExportOption>* r_options) const {}

String EditorExportPlatform::get_os_name() const { return "CustomOS"; }

String EditorExportPlatform::get_name() const { return "Custom Platform"; }

Ref<Texture2D> EditorExportPlatform::get_logo() const { return Ref<Texture2D>(); }

bool EditorExportPlatform::has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
	String& r_error, bool& r_missing_templates, bool p_debug) const
{
	r_missing_templates = false;
	return true;
}

bool EditorExportPlatform::has_valid_project_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error) const
{
	return true;
}

List<String> EditorExportPlatform::get_binary_extensions(
	const Ref<EditorExportPreset>& p_preset) const
{
	return List<String>();
}

Error EditorExportPlatform::export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, uint32_t p_flags, bool p_notify)
{
	return OK;
}

Error EditorExportPlatform::ssh_run_on_remote_no_wait(const String& p_host, const String& p_port,
	const Vector<String>& p_ssh_args, const String& p_cmd_args, ProcessID* r_pid,
	int p_port_fwd) const
{
	return OK;
}

void EditorExportPlatform::get_platform_features(List<String>* r_features) const {}

Vector<String> EditorExportPlatform::get_forced_export_files(
	const Ref<EditorExportPreset>& p_preset)
{
	return Vector<String>();
}

Error EditorExportPlatform::ssh_run_on_remote(const String& p_host, const String& p_port,
	const Vector<String>& p_ssh_args, const String& p_cmd_args, String* r_out, int p_port_fwd) const
{
	return OK;
}



// ============================================================================
// Merged from editor/export/editor_export_platform_pc.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/export/editor_export_platform.h"
#include "scene/resources/image_texture.h" // IWYU pragma: keep. Misdetection of `logo`.

String EditorExportPlatformPC::get_name() const { return name; }

String EditorExportPlatformPC::get_os_name() const { return os_name; }

Ref<Texture2D> EditorExportPlatformPC::get_logo() const { return logo; }

bool EditorExportPlatformPC::has_valid_project_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error) const
{
	return true;
}

Error EditorExportPlatformPC::export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, uint32_t p_flags, bool p_notify)
{
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags, p_notify);

	Error err = prepare_template(p_preset, p_debug, p_path, p_flags);
	if (err == OK) {
		err = modify_template(p_preset, p_debug, p_path, p_flags);
	}
	if (err == OK) {
		err = export_project_data(p_preset, p_debug, p_path, p_flags);
	}

	return err;
}

Error EditorExportPlatformPC::sign_shared_object(
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path)
{
	return OK;
}

void EditorExportPlatformPC::set_name(const String& p_name) { name = p_name; }

void EditorExportPlatformPC::set_os_name(const String& p_name) { os_name = p_name; }

void EditorExportPlatformPC::set_logo(const Ref<Texture2D>& p_logo) { logo = p_logo; }

void EditorExportPlatformPC::get_platform_features(List<String>* r_features) const
{
	r_features->push_back("pc"); // Identify PC platforms as such.
}

void EditorExportPlatformPC::resolve_platform_feature_priorities(
	const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features)
{
}

int EditorExportPlatformPC::get_chmod_flags() const { return chmod_flags; }

void EditorExportPlatformPC::set_chmod_flags(int p_flags) { chmod_flags = p_flags; }

Error EditorExportPlatformPC::prepare_template(
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path, uint32_t p_flags)
{
	return OK;
}

void EditorExportPlatformPC::get_preset_features(
	const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const
{
}

String EditorExportPlatformPC::get_export_option_warning(
	const EditorExportPreset* p_preset, const String& p_name) const
{
	return String();
}

Error EditorExportPlatformPC::export_project_data(
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path, uint32_t p_flags)
{
	return OK;
}

void EditorExportPlatformPC::get_export_options(
	List<EditorExportPlatform::ExportOption>* r_options) const
{
}

bool EditorExportPlatformPC::has_valid_export_configuration(const Ref<EditorExportPreset>& p_preset,
	String& r_error, bool& r_missing_templates, bool p_debug) const
{
	return false;
}



// ============================================================================
// Merged from editor/export/editor_export_plugin.cpp
// ============================================================================

#include "editor/export/editor_export_platform.h"

#include "core/config/project_settings.h"
#include "editor/export/editor_export_platform.h"

void EditorExportPlugin::set_export_base_path(const String& p_export_base_path)
{
	export_base_path = p_export_base_path;
}

const String& EditorExportPlugin::get_export_base_path() const { return export_base_path; }

void EditorExportPlugin::set_export_preset(const Ref<EditorExportPreset>& p_preset)
{
	if (p_preset.is_valid()) {
		export_preset = p_preset;
	}
}

Ref<EditorExportPreset> EditorExportPlugin::get_export_preset() const { return export_preset; }

Ref<EditorExportPlatform> EditorExportPlugin::get_export_platform() const
{
	if (export_preset.is_valid()) {
		return export_preset->get_platform();
	}
	else {
		return Ref<EditorExportPlatform>();
	}
}

void EditorExportPlugin::add_file(const String& p_path, const Vector<uint8_t>& p_file, bool p_remap)
{
	ExtraFile ef;
	ef.data = p_file;
	ef.path = p_path;
	ef.remap = p_remap;
	extra_files.push_back(ef);
}

void EditorExportPlugin::add_shared_object(
	const String& p_path, const Vector<String>& p_tags, const String& p_target)
{
	shared_objects.push_back(SharedObject(p_path, p_tags, p_target));
}

void EditorExportPlugin::_add_shared_object(const SharedObject& p_shared_object)
{
	shared_objects.push_back(p_shared_object);
}

void EditorExportPlugin::add_apple_embedded_platform_framework(const String& p_path)
{
	apple_embedded_platform_frameworks.push_back(p_path);
}

void EditorExportPlugin::add_apple_embedded_platform_embedded_framework(const String& p_path)
{
	apple_embedded_platform_embedded_frameworks.push_back(p_path);
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_frameworks() const
{
	return apple_embedded_platform_frameworks;
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_embedded_frameworks() const
{
	return apple_embedded_platform_embedded_frameworks;
}

void EditorExportPlugin::add_apple_embedded_platform_plist_content(const String& p_plist_content)
{
	apple_embedded_platform_plist_content += p_plist_content + "\n";
}

String EditorExportPlugin::get_apple_embedded_platform_plist_content() const
{
	return apple_embedded_platform_plist_content;
}

void EditorExportPlugin::add_apple_embedded_platform_linker_flags(const String& p_flags)
{
	if (apple_embedded_platform_linker_flags.length() > 0) {
		apple_embedded_platform_linker_flags += ' ';
	}
	apple_embedded_platform_linker_flags += p_flags;
}

String EditorExportPlugin::get_apple_embedded_platform_linker_flags() const
{
	return apple_embedded_platform_linker_flags;
}

void EditorExportPlugin::add_apple_embedded_platform_bundle_file(const String& p_path)
{
	apple_embedded_platform_bundle_files.push_back(p_path);
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_bundle_files() const
{
	return apple_embedded_platform_bundle_files;
}

void EditorExportPlugin::add_apple_embedded_platform_cpp_code(const String& p_code)
{
	apple_embedded_platform_cpp_code += p_code;
}

String EditorExportPlugin::get_apple_embedded_platform_cpp_code() const
{
	return apple_embedded_platform_cpp_code;
}

void EditorExportPlugin::add_macos_plugin_file(const String& p_path)
{
	macos_plugin_files.push_back(p_path);
}

const Vector<String>& EditorExportPlugin::get_macos_plugin_files() const
{
	return macos_plugin_files;
}

void EditorExportPlugin::add_apple_embedded_platform_project_static_lib(const String& p_path)
{
	apple_embedded_platform_project_static_libs.push_back(p_path);
}

Vector<String> EditorExportPlugin::get_apple_embedded_platform_project_static_libs() const
{
	return apple_embedded_platform_project_static_libs;
}

// Customization

PackedStringArray EditorExportPlugin::get_export_features(
	const Ref<EditorExportPlatform>& p_export_platform, bool p_debug) const
{
	return _get_export_features(p_export_platform, p_debug);
}

void EditorExportPlugin::_export_file(
	const String& p_path, const String& p_type, const HashSet<String>& p_features)
{
}

void EditorExportPlugin::_export_begin(
	const HashSet<String>& p_features, bool p_debug, const String& p_path, int p_flags)
{
}

void EditorExportPlugin::_export_end() {}

void EditorExportPlugin::_end_generate_apple_embedded_project(
	const String& p_path, bool p_will_build_archive)
{
}

void EditorExportPlugin::end_generate_apple_embedded_project(
	const String& p_path, bool p_will_build_archive)
{
	_end_generate_apple_embedded_project(p_path, p_will_build_archive);
}

void EditorExportPlugin::skip() { skipped = true; }

bool EditorExportPlugin::_begin_customize_resources(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_targets)
{
	return true;
}

Ref<Resource> EditorExportPlugin::_customize_resource(
	const Ref<Resource>& p_resource, const String& p_path)
{
	return nullptr;
}

bool EditorExportPlugin::_begin_customize_scenes(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_targets)
{
	return true;
}

Node* EditorExportPlugin::_customize_scene(Node* p_scene, const String& p_path) { return nullptr; }

uint64_t EditorExportPlugin::_get_customization_configuration_hash() const { return 0; }

void EditorExportPlugin::_end_customize_scenes() {}

void EditorExportPlugin::_end_customize_resources() {}

PackedStringArray EditorExportPlugin::_get_export_features(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return PackedStringArray();
}

void EditorExportPlugin::_get_export_options(const Ref<EditorExportPlatform>& p_export_platform,
	List<EditorExportPlatform::ExportOption>* r_options) const
{
}

bool EditorExportPlugin::_should_update_export_options(
	const Ref<EditorExportPlatform>& p_platform) const
{
	return false;
}

bool EditorExportPlugin::_get_export_option_visibility(
	const Ref<EditorExportPlatform>& p_platform, const String& p_option) const
{
	return true;
}

String EditorExportPlugin::_get_export_option_warning(
	const Ref<EditorExportPlatform>& p_platform, const String& p_option) const
{
	return String();
}

String EditorExportPlugin::get_name() const { return String(); }

bool EditorExportPlugin::supports_platform(const Ref<EditorExportPlatform>& p_platform) const
{
	return true;
}

Vector<String> EditorExportPlugin::get_android_dependencies(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return Vector<String>();
}

Vector<String> EditorExportPlugin::get_android_dependencies_maven_repos(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return Vector<String>();
}

Vector<String> EditorExportPlugin::get_android_libraries(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return Vector<String>();
}

String EditorExportPlugin::get_android_manifest_activity_element_contents(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return String();
}

String EditorExportPlugin::get_android_manifest_application_element_contents(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return String();
}

String EditorExportPlugin::get_android_manifest_element_contents(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	return String();
}

PackedByteArray EditorExportPlugin::update_android_prebuilt_manifest(
	const Ref<EditorExportPlatform>& p_export_platform,
	const PackedByteArray& p_manifest_data) const
{
	return PackedByteArray();
}

// DedicatedServerExportPlugin

EditorExportPreset::FileExportMode DedicatedServerExportPlugin::_get_export_mode_for_path(
	const String& p_path)
{
	Ref<EditorExportPreset> preset = get_export_preset();
	ERR_FAIL_COND_V(preset.is_null(), EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED);

	EditorExportPreset::FileExportMode mode = preset->get_file_export_mode(p_path);
	if (mode != EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED) {
		return mode;
	}

	String path = p_path;
	if (path.begins_with("res://")) {
		path = path.substr(6);
	}

	Vector<String> parts = path.split("/");

	while (parts.size() > 0) {
		parts.resize(parts.size() - 1);

		String test_path = "res://";
		if (parts.size() > 0) {
			test_path += String("/").join(parts) + "/";
		}

		mode = preset->get_file_export_mode(test_path);
		if (mode != EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED) {
			break;
		}
	}

	return mode;
}

PackedStringArray DedicatedServerExportPlugin::_get_export_features(
	const Ref<EditorExportPlatform>& p_platform, bool p_debug) const
{
	PackedStringArray ret;

	Ref<EditorExportPreset> preset = get_export_preset();
	ERR_FAIL_COND_V(preset.is_null(), ret);

	if (preset->is_dedicated_server()) {
		ret.append("dedicated_server");
	}
	return ret;
}

bool DedicatedServerExportPlugin::_begin_customize_scenes(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features)
{
	Ref<EditorExportPreset> preset = get_export_preset();
	ERR_FAIL_COND_V(preset.is_null(), false);

	current_export_mode = EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED;

	return preset->get_export_filter() == EditorExportPreset::EXPORT_CUSTOMIZED;
}

bool DedicatedServerExportPlugin::_begin_customize_resources(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features)
{
	Ref<EditorExportPreset> preset = get_export_preset();
	ERR_FAIL_COND_V(preset.is_null(), false);

	current_export_mode = EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED;

	return preset->get_export_filter() == EditorExportPreset::EXPORT_CUSTOMIZED;
}

Node* DedicatedServerExportPlugin::_customize_scene(Node* p_root, const String& p_path)
{
	current_export_mode = _get_export_mode_for_path(p_path);
	return nullptr;
}

void DedicatedServerExportPlugin::_end_customize_scenes()
{
	current_export_mode = EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED;
}

void DedicatedServerExportPlugin::_end_customize_resources()
{
	current_export_mode = EditorExportPreset::MODE_FILE_NOT_CUSTOMIZED;
}

Ref<Resource> DedicatedServerExportPlugin::_customize_resource(
	const Ref<Resource>& p_resource, const String& p_path)
{
	return p_resource;
}

uint64_t DedicatedServerExportPlugin::_get_customization_configuration_hash() const { return 0; }

// ============================================================================
// Merged from editor/export/shader_baker_export_plugin.cpp
// ============================================================================
// --- Inlined from editor/export/shader_baker_export_plugin.h ---
#include "core/os/condition_variable.h"
#include "core/templates/rb_set.h"
#include "editor/export/editor_export_platform.h"

class ShaderRD;
class RenderingShaderContainerFormat;

class ShaderBakerExportPluginPlatform final : public RefCounted
{
public:
	enum Driver
	{
		DRIVER_VULKAN,
		DRIVER_D3D12,
		DRIVER_METAL,
	};

private:
	Driver driver;
#ifdef D3D12_ENABLED
	void* lib_d3d12 = nullptr;
#endif

public:
	explicit ShaderBakerExportPluginPlatform(Driver p_driver) : driver(p_driver) {}
	~ShaderBakerExportPluginPlatform() override;

	Driver get_driver() const { return driver; }
	RenderingShaderContainerFormat* create_shader_container_format(
		const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset);
	bool matches_driver(const String& p_driver) const;
};

class ShaderBakerExportPlugin final : public EditorExportPlugin
{
protected:
	struct WorkItem
	{
		String cache_path;
		String shader_name;
		Vector<String> stage_sources;
		Vector<uint64_t> dynamic_buffers;
		int64_t variant = 0;
	};

	struct WorkResult
	{
		Vector<PackedByteArray> variant_data;
	};

	struct ShaderGroupItem
	{
		String cache_path;
		LocalVector<int> variants;
	};

	String shader_cache_platform_name;
	String shader_cache_renderer_name;
	String shader_cache_export_path;
	RBSet<String> shader_paths_processed;
	HashMap<String, WorkResult> shader_work_results;
	Mutex shader_work_results_mutex;
	LocalVector<ShaderGroupItem> shader_group_items;
	RenderingShaderContainerFormat* shader_container_format = nullptr;
	String shader_container_driver;
	Vector<Ref<ShaderBakerExportPluginPlatform>> platforms;
	uint64_t customization_configuration_hash = 0;
	uint32_t tasks_processed = 0;
	uint32_t tasks_total = 0;
	std::atomic<bool> tasks_cancelled;
	BinaryMutex tasks_mutex;
	ConditionVariable tasks_condition;

	String get_name() const override;
	bool _is_active(const Vector<String>& p_features) const;
	bool _initialize_container_format(
		const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset);
	void _cleanup_container_format();
	bool _initialize_cache_directory();
	bool _begin_customize_resources(
		const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features) override;
	bool _begin_customize_scenes(
		const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features) override;
	void _end_customize_resources() override;
	Ref<Resource> _customize_resource(
		const Ref<Resource>& p_resource, const String& p_path) override;
	Node* _customize_scene(Node* p_root, const String& p_path) override;
	uint64_t _get_customization_configuration_hash() const override;
	void _customize_shader_version(ShaderRD* p_shader, RID p_version);
	void _process_work_item(WorkItem p_work_item);

public:
	ShaderBakerExportPlugin();
	~ShaderBakerExportPlugin() override;
	void add_platform(Ref<ShaderBakerExportPluginPlatform> p_platform);
	void remove_platform(Ref<ShaderBakerExportPluginPlatform> p_platform);
};

// --- End editor/export/shader_baker_export_plugin.h ---

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/string/string_builder.h"
#include "core/version.h"
#include "scene/3d/visual_instance_3d.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_shader_container.h"

#ifdef VULKAN_ENABLED
#define Window VulkanX11Window
#define Font VulkanX11Font
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#undef Font
#undef Window
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Always
#undef Always
#endif
#endif

#ifdef D3D12_ENABLED
#include "drivers/d3d12/rendering_context_driver_d3d12.h"
#include <windows.h>
#endif

#ifdef METAL_ENABLED
#include "drivers/metal/rendering_shader_container_metal.h"
#endif

// Ensure that AlphaCut is the same between the two classes so we can share the code to detect
// transparency.
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_DISABLED, Label3D::ALPHA_CUT_DISABLED));
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_DISCARD, Label3D::ALPHA_CUT_DISCARD));
static_assert(
	ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_OPAQUE_PREPASS, Label3D::ALPHA_CUT_OPAQUE_PREPASS));
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_HASH, Label3D::ALPHA_CUT_HASH));
static_assert(ENUM_MEMBERS_EQUAL(Sprite3D::ALPHA_CUT_MAX, Label3D::ALPHA_CUT_MAX));

ShaderBakerExportPluginPlatform::~ShaderBakerExportPluginPlatform()
{
#ifdef D3D12_ENABLED
	if (lib_d3d12 != nullptr) {
		FreeLibrary((HMODULE)(lib_d3d12));
	}
#endif
}

RenderingShaderContainerFormat* ShaderBakerExportPluginPlatform::create_shader_container_format(
	const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset)
{
	switch (driver) {
	case DRIVER_VULKAN: {
#ifdef VULKAN_ENABLED
		return memnew(RenderingShaderContainerFormatVulkan);
#else
		ERR_FAIL_V_MSG(nullptr, "Vulkan shader baker is not enabled in this build.");
#endif
	}
	case DRIVER_D3D12: {
#ifdef D3D12_ENABLED
		if (lib_d3d12 == nullptr) {
			lib_d3d12 = LoadLibraryW(L"D3D12.dll");
			ERR_FAIL_NULL_V_MSG(lib_d3d12, nullptr, "Unable to load D3D12.dll.");
		}
		RenderingShaderContainerFormatD3D12* shader_container_format_d3d12 =
			memnew(RenderingShaderContainerFormatD3D12);
		shader_container_format_d3d12->set_lib_d3d12(lib_d3d12);
		return shader_container_format_d3d12;
#else
		ERR_FAIL_V_MSG(nullptr, "D3D12 shader baker is not enabled in this build.");
#endif
	}
	case DRIVER_METAL: {
#ifdef METAL_ENABLED
		const String& os_name = p_platform->get_os_name();
		const MetalDeviceProfile* profile = nullptr;
		MinOsVersion min_os_version;

		if (os_name == U"macOS") {
			profile = MetalDeviceProfile::get_profile(
				MetalDeviceProfile::Platform::macOS, MetalDeviceProfile::GPU::Apple7, min_os_version);
		}
		else if (os_name == U"iOS") {
			profile = MetalDeviceProfile::get_profile(
				MetalDeviceProfile::Platform::iOS, MetalDeviceProfile::GPU::Apple7, min_os_version);
		}
		else if (os_name == U"visionOS") {
			profile = MetalDeviceProfile::get_profile(
				MetalDeviceProfile::Platform::visionOS, MetalDeviceProfile::GPU::Apple8, min_os_version);
		}
		else {
			ERR_FAIL_V_MSG(nullptr, vformat("Unsupported platform: %s", os_name));
		}
		ERR_FAIL_NULL_V(profile, nullptr);
		return memnew(RenderingShaderContainerFormatMetal(profile, true));
#else
		ERR_FAIL_V_MSG(nullptr, "Metal shader baker is not enabled in this build.");
#endif
	}
	}
	return nullptr;
}

bool ShaderBakerExportPluginPlatform::matches_driver(const String& p_driver) const
{
	switch (driver) {
	case DRIVER_VULKAN:
		return p_driver == "vulkan";
	case DRIVER_D3D12:
		return p_driver == "d3d12";
	case DRIVER_METAL:
		return p_driver == "metal";
	}
	return false;
}

String ShaderBakerExportPlugin::get_name() const { return "ShaderBaker"; }

bool ShaderBakerExportPlugin::_is_active(const Vector<String>& p_features) const
{
	// Shader baker should only work when a RendererRD driver is active, as the embedded shaders
	// won't be found otherwise.
	return RendererSceneRenderRD::data != nullptr && p_features.has("shader_baker");
}

void ShaderBakerExportPlugin::_cleanup_container_format()
{
	if (shader_container_format != nullptr) {
		memdelete(shader_container_format);
		shader_container_format = nullptr;
	}
}

bool ShaderBakerExportPlugin::_initialize_cache_directory()
{
	shader_cache_export_path = get_export_base_path()
								   .path_join("shader_baker")
								   .path_join(shader_cache_platform_name)
								   .path_join(shader_container_driver);

	if (!DirAccess::dir_exists_absolute(shader_cache_export_path)) {
		Error err = DirAccess::make_dir_recursive_absolute(shader_cache_export_path);
		ERR_FAIL_COND_V_MSG(err != OK, false, "Can't create shader cache folder for exporting.");
	}

	return true;
}

bool ShaderBakerExportPlugin::_begin_customize_scenes(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features)
{
	if (!_is_active(p_features)) {
		return false;
	}

	if (shader_container_format == nullptr) {
		// Resource customization failed to initialize.
		return false;
	}

	return true;
}

Ref<Resource> ShaderBakerExportPlugin::_customize_resource(
	const Ref<Resource>& p_resource, const String& p_path)
{
	DEV_ASSERT(singleton != nullptr);

	Ref<Material> material = p_resource;
	if (material.is_valid()) {
		RID material_rid = material->get_rid();
		if (material_rid.is_valid()) {
			RendererRD::MaterialStorage::ShaderData* shader_data =
				RendererRD::MaterialStorage::material_get_shader_data(material_rid);
			if (shader_data != nullptr) {
				Pair<ShaderRD*, RID> shader_version_pair =
					shader_data->get_native_shader_and_version();
				if (shader_version_pair.first != nullptr) {
					_customize_shader_version(
						shader_version_pair.first, shader_version_pair.second);
				}
			}
		}
	}

	return Ref<Resource>();
}

uint64_t ShaderBakerExportPlugin::_get_customization_configuration_hash() const
{
	return customization_configuration_hash;
}

void ShaderBakerExportPlugin::_process_work_item(WorkItem p_work_item)
{
	if (!tasks_cancelled) {
		Vector<RDC::ShaderStageSPIRVData> spirv_data =
			ShaderRD::compile_stages(p_work_item.stage_sources, p_work_item.dynamic_buffers);
		if (unlikely(spirv_data.is_empty())) {
			ERR_PRINT("Unable to retrieve SPIR-V data for shader.");
		}
		else {
			Ref<RenderingShaderContainer> shader_container =
				shader_container_format->create_container();

			bool code_compiled =
				shader_container->set_code_from_spirv(p_work_item.shader_name, spirv_data);
			if (unlikely(!code_compiled)) {
				ERR_PRINT("Failed to compile code to native for SPIR-V.");
			}
			else {
				PackedByteArray shader_bytes = shader_container->to_bytes();
				{
					MutexLock lock(shader_work_results_mutex);
					shader_work_results[p_work_item.cache_path]
						.variant_data.ptrw()[p_work_item.variant] = shader_bytes;
				}
			}
		}
	}

	{
		MutexLock lock(tasks_mutex);
		tasks_processed++;
	}

	tasks_condition.notify_one();
}

ShaderBakerExportPlugin::ShaderBakerExportPlugin()
{
#ifdef VULKAN_ENABLED
	add_platform(memnew(ShaderBakerExportPluginPlatform(ShaderBakerExportPluginPlatform::DRIVER_VULKAN)));
#endif
#ifdef D3D12_ENABLED
	add_platform(memnew(ShaderBakerExportPluginPlatform(ShaderBakerExportPluginPlatform::DRIVER_D3D12)));
#endif
#ifdef METAL_ENABLED
	add_platform(memnew(ShaderBakerExportPluginPlatform(ShaderBakerExportPluginPlatform::DRIVER_METAL)));
#endif
}

ShaderBakerExportPlugin::~ShaderBakerExportPlugin() {}

void ShaderBakerExportPlugin::add_platform(Ref<ShaderBakerExportPluginPlatform> p_platform)
{
	platforms.push_back(p_platform);
}

void ShaderBakerExportPlugin::remove_platform(Ref<ShaderBakerExportPluginPlatform> p_platform)
{
	platforms.erase(p_platform);
}

bool ShaderBakerExportPlugin::_begin_customize_resources(
	const Ref<EditorExportPlatform>& p_platform, const Vector<String>& p_features)
{
	return false;
}

Node* ShaderBakerExportPlugin::_customize_scene(Node* p_scene, const String& p_path)
{
	return p_scene;
}

void ShaderBakerExportPlugin::_end_customize_resources() {}

bool ShaderBakerExportPlugin::_initialize_container_format(
	const Ref<EditorExportPlatform>& p_platform, const Ref<EditorExportPreset>& p_preset)
{
	return false;
}

void ShaderBakerExportPlugin::_customize_shader_version(ShaderRD* p_shader, RID p_version) {}

// ============================================================================
// Merged from editor/export/apple_export_utils.cpp
// ============================================================================

#include "editor/export/editor_export_platform.h"

#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/config_file.h"
#include "editor/file_system/editor_paths.h"
#include "modules/regex/regex.h"

#include <ctime>


/*************************************************************************/
/* MachO                                                                 */
/*************************************************************************/

uint32_t MachO::seg_align(uint64_t p_vmaddr, uint32_t p_min, uint32_t p_max) {
	uint32_t salign = p_max;
	if (p_vmaddr != 0) {
		uint64_t seg_align = 1;
		salign = 0;
		while ((seg_align & p_vmaddr) == 0) {
			seg_align = seg_align << 1;
			salign++;
		}
		salign = CLAMP(salign, p_min, p_max);
	}
	return salign;
}

bool MachO::alloc_signature(uint64_t p_size) {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "MachO: File not opened.");
	if (signature_offset != 0) {
		// Nothing to do, already have signature load command.
		return true;
	}
	if (lc_limit == 0 || lc_limit + 16 > exe_base) {
		ERR_FAIL_V_MSG(false, "MachO: Can't allocate signature load command, please use \"codesign_allocate\" utility first.");
	} else {
		// Add signature load command.
		signature_offset = lc_limit;

		fa->seek(lc_limit);
		LoadCommandHeader lc;
		lc.cmd = LC_CODE_SIGNATURE;
		lc.cmdsize = 16;
		if (swap) {
			lc.cmdsize = BSWAP32(lc.cmdsize);
		}
		fa->store_buffer((const uint8_t *)&lc, sizeof(LoadCommandHeader));

		uint32_t lc_offset = fa->get_length() + PAD(fa->get_length(), 16);
		uint32_t lc_size = 0;
		if (swap) {
			lc_offset = BSWAP32(lc_offset);
			lc_size = BSWAP32(lc_size);
		}
		fa->store_32(lc_offset);
		fa->store_32(lc_size);

		// Write new command number.
		fa->seek(0x10);
		uint32_t ncmds = fa->get_32();
		uint32_t cmdssize = fa->get_32();
		if (swap) {
			ncmds = BSWAP32(ncmds);
			cmdssize = BSWAP32(cmdssize);
		}
		ncmds += 1;
		cmdssize += 16;
		if (swap) {
			ncmds = BSWAP32(ncmds);
			cmdssize = BSWAP32(cmdssize);
		}
		fa->seek(0x10);
		fa->store_32(ncmds);
		fa->store_32(cmdssize);

		lc_limit = lc_limit + sizeof(LoadCommandHeader) + 8;

		return true;
	}
}

bool MachO::is_macho(const String &p_path) {
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("MachO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fb->get_32();
	return (magic == 0xcefaedfe || magic == 0xfeedface || magic == 0xcffaedfe || magic == 0xfeedfacf);
}

uint32_t MachO::get_filetype(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, vformat("MachO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fa->get_32();
	MachHeader mach_header;

	// Read MachO header.
	if (magic == 0xcefaedfe || magic == 0xfeedface) {
		// Thin 32-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
	} else if (magic == 0xcffaedfe || magic == 0xfeedfacf) {
		// Thin 64-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
		fa->get_32(); // Skip extra reserved field.
	} else {
		ERR_FAIL_V_MSG(0, vformat("MachO: File is not a valid MachO binary: \"%s\".", p_path));
	}
	return mach_header.filetype;
}

bool MachO::open_file(const String &p_path) {
	fa = FileAccess::open(p_path, FileAccess::READ_WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("MachO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fa->get_32();
	MachHeader mach_header;

	// Read MachO header.
	swap = (magic == 0xcffaedfe || magic == 0xcefaedfe);
	if (magic == 0xcefaedfe || magic == 0xfeedface) {
		// Thin 32-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
	} else if (magic == 0xcffaedfe || magic == 0xfeedfacf) {
		// Thin 64-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
		fa->get_32(); // Skip extra reserved field.
	} else {
		ERR_FAIL_V_MSG(false, vformat("MachO: File is not a valid MachO binary: \"%s\".", p_path));
	}

	if (swap) {
		mach_header.ncmds = BSWAP32(mach_header.ncmds);
		mach_header.cpusubtype = BSWAP32(mach_header.cpusubtype);
		mach_header.cputype = BSWAP32(mach_header.cputype);
	}
	cpusubtype = mach_header.cpusubtype;
	cputype = mach_header.cputype;
	align = 0;
	exe_base = std::numeric_limits<uint64_t>::max();
	exe_limit = 0;
	lc_limit = 0;
	link_edit_offset = 0;
	signature_offset = 0;

	// Read load commands.
	for (uint32_t i = 0; i < mach_header.ncmds; i++) {
		LoadCommandHeader lc;
		fa->get_buffer((uint8_t *)&lc, sizeof(LoadCommandHeader));
		if (swap) {
			lc.cmd = BSWAP32(lc.cmd);
			lc.cmdsize = BSWAP32(lc.cmdsize);
		}
		uint64_t ps = fa->get_position();
		switch (lc.cmd) {
			case LC_SEGMENT: {
				LoadCommandSegment lc_seg;
				fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment));
				if (swap) {
					lc_seg.nsects = BSWAP32(lc_seg.nsects);
					lc_seg.vmaddr = BSWAP32(lc_seg.vmaddr);
					lc_seg.vmsize = BSWAP32(lc_seg.vmsize);
				}
				align = MAX(align, seg_align(lc_seg.vmaddr, 2, 15));
				if (String(lc_seg.segname) == "__TEXT") {
					exe_limit = MAX(exe_limit, lc_seg.vmsize);
					for (uint32_t j = 0; j < lc_seg.nsects; j++) {
						Section lc_sect;
						fa->get_buffer((uint8_t *)&lc_sect, sizeof(Section));
						if (String(lc_sect.sectname) == "__text") {
							if (swap) {
								exe_base = MIN(exe_base, BSWAP32(lc_sect.offset));
							} else {
								exe_base = MIN(exe_base, lc_sect.offset);
							}
						}
						if (swap) {
							align = MAX(align, BSWAP32(lc_sect.align));
						} else {
							align = MAX(align, lc_sect.align);
						}
					}
				} else if (String(lc_seg.segname) == "__LINKEDIT") {
					link_edit_offset = ps - 8;
				}
			} break;
			case LC_SEGMENT_64: {
				LoadCommandSegment64 lc_seg;
				fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment64));
				if (swap) {
					lc_seg.nsects = BSWAP32(lc_seg.nsects);
					lc_seg.vmaddr = BSWAP64(lc_seg.vmaddr);
					lc_seg.vmsize = BSWAP64(lc_seg.vmsize);
				}
				align = MAX(align, seg_align(lc_seg.vmaddr, 3, 15));
				if (String(lc_seg.segname) == "__TEXT") {
					exe_limit = MAX(exe_limit, lc_seg.vmsize);
					for (uint32_t j = 0; j < lc_seg.nsects; j++) {
						Section64 lc_sect;
						fa->get_buffer((uint8_t *)&lc_sect, sizeof(Section64));
						if (String(lc_sect.sectname) == "__text") {
							if (swap) {
								exe_base = MIN(exe_base, BSWAP32(lc_sect.offset));
							} else {
								exe_base = MIN(exe_base, lc_sect.offset);
							}
							if (swap) {
								align = MAX(align, BSWAP32(lc_sect.align));
							} else {
								align = MAX(align, lc_sect.align);
							}
						}
					}
				} else if (String(lc_seg.segname) == "__LINKEDIT") {
					link_edit_offset = ps - 8;
				}
			} break;
			case LC_CODE_SIGNATURE: {
				signature_offset = ps - 8;
			} break;
			default: {
			} break;
		}
		fa->seek(ps + lc.cmdsize - 8);
		lc_limit = ps + lc.cmdsize - 8;
	}

	if (exe_limit == 0 || lc_limit == 0) {
		ERR_FAIL_V_MSG(false, vformat("MachO: No load commands or executable code found: \"%s\".", p_path));
	}

	return true;
}

uint64_t MachO::get_exe_base() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return exe_base;
}

uint64_t MachO::get_exe_limit() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return exe_limit;
}

int32_t MachO::get_align() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return align;
}

uint32_t MachO::get_cputype() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return cputype;
}

uint32_t MachO::get_cpusubtype() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return cpusubtype;
}

uint64_t MachO::get_size() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return fa->get_length();
}

uint64_t MachO::get_signature_offset() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	ERR_FAIL_COND_V_MSG(signature_offset == 0, 0, "MachO: No signature load command.");

	fa->seek(signature_offset + 8);
	if (swap) {
		return BSWAP32(fa->get_32());
	} else {
		return fa->get_32();
	}
}

uint64_t MachO::get_code_limit() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");

	if (signature_offset == 0) {
		return fa->get_length() + PAD(fa->get_length(), 16);
	} else {
		return get_signature_offset();
	}
}

uint64_t MachO::get_signature_size() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	ERR_FAIL_COND_V_MSG(signature_offset == 0, 0, "MachO: No signature load command.");

	fa->seek(signature_offset + 12);
	if (swap) {
		return BSWAP32(fa->get_32());
	} else {
		return fa->get_32();
	}
}

bool MachO::is_signed() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "MachO: File not opened.");
	if (signature_offset == 0) {
		return false;
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return false; // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		uint32_t index_type = BSWAP32(fa->get_32());
		uint32_t offset = BSWAP32(fa->get_32());
		if (index_type == 0x00000000) { // CodeDirectory index type.
			fa->seek(get_signature_offset() + offset + 12);
			uint32_t flags = BSWAP32(fa->get_32());
			if (flags & 0x20000) {
				return false; // Found CD, linker-signed.
			} else {
				return true; // Found CD, not linker-signed.
			}
		}
	}
	return false; // No CD found.
}

PackedByteArray MachO::get_cdhash_sha1() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "MachO: File not opened.");
	if (signature_offset == 0) {
		return PackedByteArray();
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return PackedByteArray(); // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		fa->get_32(); // Index type, skip.
		uint32_t offset = BSWAP32(fa->get_32());
		uint64_t pos = fa->get_position();

		fa->seek(get_signature_offset() + offset);
		uint32_t cdmagic = BSWAP32(fa->get_32());
		uint32_t cdsize = BSWAP32(fa->get_32());
		if (cdmagic == 0xfade0c02) { // CodeDirectory.
			fa->seek(get_signature_offset() + offset + 36);
			uint8_t hash_size = fa->get_8();
			uint8_t hash_type = fa->get_8();
			if (hash_size == 0x14 && hash_type == 0x01) { /* SHA-1 */
				PackedByteArray hash;
				hash.resize(0x14);

				fa->seek(get_signature_offset() + offset);
				PackedByteArray blob;
				blob.resize(cdsize);
				fa->get_buffer(blob.ptrw(), cdsize);

				CryptoCore::SHA1Context ctx;
				ctx.start();
				ctx.update(blob.ptr(), blob.size());
				ctx.finish(hash.ptrw());

				return hash;
			}
		}
		fa->seek(pos);
	}
	return PackedByteArray();
}

PackedByteArray MachO::get_cdhash_sha256() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "MachO: File not opened.");
	if (signature_offset == 0) {
		return PackedByteArray();
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return PackedByteArray(); // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		fa->get_32(); // Index type, skip.
		uint32_t offset = BSWAP32(fa->get_32());
		uint64_t pos = fa->get_position();

		fa->seek(get_signature_offset() + offset);
		uint32_t cdmagic = BSWAP32(fa->get_32());
		uint32_t cdsize = BSWAP32(fa->get_32());
		if (cdmagic == 0xfade0c02) { // CodeDirectory.
			fa->seek(get_signature_offset() + offset + 36);
			uint8_t hash_size = fa->get_8();
			uint8_t hash_type = fa->get_8();
			if (hash_size == 0x20 && hash_type == 0x02) { /* SHA-256 */
				PackedByteArray hash;
				hash.resize(0x20);

				fa->seek(get_signature_offset() + offset);
				PackedByteArray blob;
				blob.resize(cdsize);
				fa->get_buffer(blob.ptrw(), cdsize);

				CryptoCore::SHA256Context ctx;
				ctx.start();
				ctx.update(blob.ptr(), blob.size());
				ctx.finish(hash.ptrw());

				return hash;
			}
		}
		fa->seek(pos);
	}
	return PackedByteArray();
}

PackedByteArray MachO::get_requirements() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "MachO: File not opened.");
	if (signature_offset == 0) {
		return PackedByteArray();
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return PackedByteArray(); // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		fa->get_32(); // Index type, skip.
		uint32_t offset = BSWAP32(fa->get_32());
		uint64_t pos = fa->get_position();

		fa->seek(get_signature_offset() + offset);
		uint32_t rqmagic = BSWAP32(fa->get_32());
		uint32_t rqsize = BSWAP32(fa->get_32());
		if (rqmagic == 0xfade0c01) { // Requirements.
			PackedByteArray blob;
			fa->seek(get_signature_offset() + offset);
			blob.resize(rqsize);
			fa->get_buffer(blob.ptrw(), rqsize);
			return blob;
		}
		fa->seek(pos);
	}
	return PackedByteArray();
}

const Ref<FileAccess> MachO::get_file() const {
	return fa;
}

Ref<FileAccess> MachO::get_file() {
	return fa;
}

bool MachO::set_signature_size(uint64_t p_size) {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "MachO: File not opened.");

	// Ensure signature load command exists.
	ERR_FAIL_COND_V_MSG(link_edit_offset == 0, false, "MachO: No __LINKEDIT segment found.");
	ERR_FAIL_COND_V_MSG(!alloc_signature(p_size), false, "MachO: Can't allocate signature load command.");

	// Update signature load command.
	uint64_t old_size = get_signature_size();
	uint64_t new_size = p_size + PAD(p_size, 16384);

	if (new_size <= old_size) {
		fa->seek(get_signature_offset());
		for (uint64_t i = 0; i < old_size; i++) {
			fa->store_8(0x00);
		}
		return true;
	}

	fa->seek(signature_offset + 12);
	if (swap) {
		fa->store_32(BSWAP32(new_size));
	} else {
		fa->store_32(new_size);
	}

	uint64_t end = get_signature_offset() + new_size;

	// Update "__LINKEDIT" segment.
	LoadCommandHeader lc;
	fa->seek(link_edit_offset);
	fa->get_buffer((uint8_t *)&lc, sizeof(LoadCommandHeader));
	if (swap) {
		lc.cmd = BSWAP32(lc.cmd);
		lc.cmdsize = BSWAP32(lc.cmdsize);
	}
	switch (lc.cmd) {
		case LC_SEGMENT: {
			LoadCommandSegment lc_seg;
			fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment));
			if (swap) {
				lc_seg.vmsize = BSWAP32(lc_seg.vmsize);
				lc_seg.filesize = BSWAP32(lc_seg.filesize);
				lc_seg.fileoff = BSWAP32(lc_seg.fileoff);
			}

			lc_seg.vmsize = end - lc_seg.fileoff;
			lc_seg.vmsize += PAD(lc_seg.vmsize, 4096);
			lc_seg.filesize = end - lc_seg.fileoff;

			if (swap) {
				lc_seg.vmsize = BSWAP32(lc_seg.vmsize);
				lc_seg.filesize = BSWAP32(lc_seg.filesize);
			}
			fa->seek(link_edit_offset + 8);
			fa->store_buffer((const uint8_t *)&lc_seg, sizeof(LoadCommandSegment));
		} break;
		case LC_SEGMENT_64: {
			LoadCommandSegment64 lc_seg;
			fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment64));
			if (swap) {
				lc_seg.vmsize = BSWAP64(lc_seg.vmsize);
				lc_seg.filesize = BSWAP64(lc_seg.filesize);
				lc_seg.fileoff = BSWAP64(lc_seg.fileoff);
			}
			lc_seg.vmsize = end - lc_seg.fileoff;
			lc_seg.vmsize += PAD(lc_seg.vmsize, 4096);
			lc_seg.filesize = end - lc_seg.fileoff;
			if (swap) {
				lc_seg.vmsize = BSWAP64(lc_seg.vmsize);
				lc_seg.filesize = BSWAP64(lc_seg.filesize);
			}
			fa->seek(link_edit_offset + 8);
			fa->store_buffer((const uint8_t *)&lc_seg, sizeof(LoadCommandSegment64));
		} break;
		default: {
			ERR_FAIL_V_MSG(false, "MachO: Invalid __LINKEDIT segment type.");
		} break;
	}
	fa->seek(get_signature_offset());
	for (uint64_t i = 0; i < new_size; i++) {
		fa->store_8(0x00);
	}
	return true;
}

/*************************************************************************/
/* LipO                                                                  */
/*************************************************************************/

bool LipO::is_lipo(const String &p_path) {
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fb->get_32();
	return (magic == 0xbebafeca || magic == 0xcafebabe || magic == 0xbfbafeca || magic == 0xcafebabf);
}

bool LipO::create_file(const String &p_output_path, const Vector<String> &p_files) {
	close();

	fa = FileAccess::open(p_output_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_output_path));

	uint64_t max_size = 0;
	for (int i = 0; i < p_files.size(); i++) {
		{
			MachO mh;
			if (!mh.open_file(p_files[i])) {
				ERR_FAIL_V_MSG(false, vformat("LipO: Invalid MachO file: \"%s\".", p_files[i]));
			}

			FatArch arch;
			arch.cputype = mh.get_cputype();
			arch.cpusubtype = mh.get_cpusubtype();
			arch.offset = 0;
			arch.size = mh.get_size();
			arch.align = mh.get_align();
			max_size += arch.size;

			archs.push_back(arch);
		}

		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}
	}

	// Write header.
	bool is_64 = (max_size >= std::numeric_limits<uint32_t>::max());
	if (is_64) {
		fa->store_32(0xbfbafeca);
	} else {
		fa->store_32(0xbebafeca);
	}
	fa->store_32(BSWAP32(archs.size()));
	uint64_t offset = archs.size() * (is_64 ? 32 : 20) + 8;
	for (int i = 0; i < archs.size(); i++) {
		archs.write[i].offset = offset + PAD(offset, uint64_t(1) << archs[i].align);
		if (is_64) {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_64(BSWAP64(archs[i].offset));
			fa->store_64(BSWAP64(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
			fa->store_32(0);
		} else {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_32(BSWAP32(archs[i].offset));
			fa->store_32(BSWAP32(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
		}
		offset = archs[i].offset + archs[i].size;
	}

	// Write files and padding.
	for (int i = 0; i < archs.size(); i++) {
		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}
		uint64_t cur = fa->get_position();
		for (uint64_t j = cur; j < archs[i].offset; j++) {
			fa->store_8(0);
		}
		int pages = archs[i].size / 4096;
		int remain = archs[i].size % 4096;
		unsigned char step[4096];
		for (int j = 0; j < pages; j++) {
			uint64_t br = fb->get_buffer(step, 4096);
			if (br > 0) {
				fa->store_buffer(step, br);
			}
		}
		uint64_t br = fb->get_buffer(step, remain);
		if (br > 0) {
			fa->store_buffer(step, br);
		}
	}
	return true;
}

bool LipO::create_file(const String &p_output_path, const Vector<String> &p_files, const Vector<Vector2i> &p_cputypes) {
	close();

	fa = FileAccess::open(p_output_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_output_path));
	ERR_FAIL_COND_V(p_files.size() != p_cputypes.size(), false);

	uint64_t max_size = 0;
	for (int i = 0; i < p_files.size(); i++) {
		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}

		{
			FatArch arch;
			MachO mh;
			if (MachO::is_macho(p_files[i]) && mh.open_file(p_files[i])) {
				arch.cputype = mh.get_cputype();
				arch.cpusubtype = mh.get_cpusubtype();
				arch.offset = 0;
				arch.size = mh.get_size();
				arch.align = mh.get_align();
				ERR_FAIL_V_MSG(arch.cputype != (uint32_t)p_cputypes[i].x || arch.cpusubtype != (uint32_t)p_cputypes[i].y, vformat("Mismatching MachO architecture: \"%s\".", p_files[i]));
			} else {
				arch.cputype = (uint32_t)p_cputypes[i].x;
				arch.cpusubtype = (uint32_t)p_cputypes[i].y;
				arch.offset = 0;
				arch.size = fb->get_length();
				arch.align = 0x03;
			}
			max_size += arch.size;

			archs.push_back(arch);
		}
	}

	// Write header.
	bool is_64 = (max_size >= std::numeric_limits<uint32_t>::max());
	if (is_64) {
		fa->store_32(0xbfbafeca);
	} else {
		fa->store_32(0xbebafeca);
	}
	fa->store_32(BSWAP32(archs.size()));
	uint64_t offset = archs.size() * (is_64 ? 32 : 20) + 8;
	for (int i = 0; i < archs.size(); i++) {
		archs.write[i].offset = offset + PAD(offset, uint64_t(1) << archs[i].align);
		if (is_64) {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_64(BSWAP64(archs[i].offset));
			fa->store_64(BSWAP64(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
			fa->store_32(0);
		} else {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_32(BSWAP32(archs[i].offset));
			fa->store_32(BSWAP32(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
		}
		offset = archs[i].offset + archs[i].size;
	}

	// Write files and padding.
	for (int i = 0; i < archs.size(); i++) {
		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}
		uint64_t cur = fa->get_position();
		for (uint64_t j = cur; j < archs[i].offset; j++) {
			fa->store_8(0);
		}
		int pages = archs[i].size / 4096;
		int remain = archs[i].size % 4096;
		unsigned char step[4096];
		for (int j = 0; j < pages; j++) {
			uint64_t br = fb->get_buffer(step, 4096);
			if (br > 0) {
				fa->store_buffer(step, br);
			}
		}
		uint64_t br = fb->get_buffer(step, remain);
		if (br > 0) {
			fa->store_buffer(step, br);
		}
	}
	return true;
}

bool LipO::open_file(const String &p_path) {
	close();

	fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_path));

	uint32_t magic = fa->get_32();
	if (magic == 0xbebafeca) {
		// 32-bit fat binary, bswap.
		uint32_t nfat_arch = BSWAP32(fa->get_32());
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = BSWAP32(fa->get_32());
			arch.cpusubtype = BSWAP32(fa->get_32());
			arch.offset = BSWAP32(fa->get_32());
			arch.size = BSWAP32(fa->get_32());
			arch.align = BSWAP32(fa->get_32());

			archs.push_back(arch);
		}
	} else if (magic == 0xcafebabe) {
		// 32-bit fat binary.
		uint32_t nfat_arch = fa->get_32();
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = fa->get_32();
			arch.cpusubtype = fa->get_32();
			arch.offset = fa->get_32();
			arch.size = fa->get_32();
			arch.align = fa->get_32();

			archs.push_back(arch);
		}
	} else if (magic == 0xbfbafeca) {
		// 64-bit fat binary, bswap.
		uint32_t nfat_arch = BSWAP32(fa->get_32());
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = BSWAP32(fa->get_32());
			arch.cpusubtype = BSWAP32(fa->get_32());
			arch.offset = BSWAP64(fa->get_64());
			arch.size = BSWAP64(fa->get_64());
			arch.align = BSWAP32(fa->get_32());
			fa->get_32(); // Skip, reserved.

			archs.push_back(arch);
		}
	} else if (magic == 0xcafebabf) {
		// 64-bit fat binary.
		uint32_t nfat_arch = fa->get_32();
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = fa->get_32();
			arch.cpusubtype = fa->get_32();
			arch.offset = fa->get_64();
			arch.size = fa->get_64();
			arch.align = fa->get_32();
			fa->get_32(); // Skip, reserved.

			archs.push_back(arch);
		}
	} else {
		close();
		ERR_FAIL_V_MSG(false, vformat("LipO: Invalid fat binary: \"%s\".", p_path));
	}
	return true;
}

int LipO::get_arch_count() const {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "LipO: File not opened.");
	return archs.size();
}

uint32_t LipO::get_arch_cputype(int p_index) const {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "LipO: File not opened.");
	ERR_FAIL_INDEX_V(p_index, archs.size(), 0);
	return archs[p_index].cputype;
}

uint32_t LipO::get_arch_cpusubtype(int p_index) const {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "LipO: File not opened.");
	ERR_FAIL_INDEX_V(p_index, archs.size(), 0);
	return archs[p_index].cpusubtype;
}

bool LipO::extract_arch(int p_index, const String &p_path) {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "LipO: File not opened.");
	ERR_FAIL_INDEX_V(p_index, archs.size(), false);

	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_path));

	fa->seek(archs[p_index].offset);

	int pages = archs[p_index].size / 4096;
	int remain = archs[p_index].size % 4096;
	unsigned char step[4096];
	for (int i = 0; i < pages; i++) {
		uint64_t br = fa->get_buffer(step, 4096);
		if (br > 0) {
			fb->store_buffer(step, br);
		}
	}
	uint64_t br = fa->get_buffer(step, remain);
	if (br > 0) {
		fb->store_buffer(step, br);
	}
	return true;
}

void LipO::close() {
	archs.clear();
}

LipO::~LipO() {
	close();
}

/*************************************************************************/
/* CodeSignCodeResources                                                 */
/*************************************************************************/

String CodeSignCodeResources::hash_sha1_base64(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), String(), vformat("CodeSign/CodeResources: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA1Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = fa->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	unsigned char hash[0x14];
	ctx.finish(hash);

	return CryptoCore::b64_encode_str(hash, 0x14);
}

String CodeSignCodeResources::hash_sha256_base64(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), String(), vformat("CodeSign/CodeResources: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA256Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = fa->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	unsigned char hash[0x20];
	ctx.finish(hash);

	return CryptoCore::b64_encode_str(hash, 0x20);
}

void CodeSignCodeResources::add_rule1(const String &p_rule, const String &p_key, int p_weight, bool p_store) {
	rules1.push_back(CRRule(p_rule, p_key, p_weight, p_store));
}

void CodeSignCodeResources::add_rule2(const String &p_rule, const String &p_key, int p_weight, bool p_store) {
	rules2.push_back(CRRule(p_rule, p_key, p_weight, p_store));
}

CodeSignCodeResources::CRMatch CodeSignCodeResources::match_rules1(const String &p_path) const {
	CRMatch found = CRMatch::CR_MATCH_NO;
	int weight = 0;
	for (int i = 0; i < rules1.size(); i++) {
		RegEx regex = RegEx(rules1[i].file_pattern);
		if (regex.search(p_path).is_valid()) {
			if (rules1[i].key == "omit") {
				return CRMatch::CR_MATCH_NO;
			} else if (rules1[i].key == "nested") {
				if (weight <= rules1[i].weight) {
					found = CRMatch::CR_MATCH_NESTED;
					weight = rules1[i].weight;
				}
			} else if (rules1[i].key == "optional") {
				if (weight <= rules1[i].weight) {
					found = CRMatch::CR_MATCH_OPTIONAL;
					weight = rules1[i].weight;
				}
			} else {
				if (weight <= rules1[i].weight) {
					found = CRMatch::CR_MATCH_YES;
					weight = rules1[i].weight;
				}
			}
		}
	}
	return found;
}

CodeSignCodeResources::CRMatch CodeSignCodeResources::match_rules2(const String &p_path) const {
	CRMatch found = CRMatch::CR_MATCH_NO;
	int weight = 0;
	for (int i = 0; i < rules2.size(); i++) {
		RegEx regex = RegEx(rules2[i].file_pattern);
		if (regex.search(p_path).is_valid()) {
			if (rules2[i].key == "omit") {
				return CRMatch::CR_MATCH_NO;
			} else if (rules2[i].key == "nested") {
				if (weight <= rules2[i].weight) {
					found = CRMatch::CR_MATCH_NESTED;
					weight = rules2[i].weight;
				}
			} else if (rules2[i].key == "optional") {
				if (weight <= rules2[i].weight) {
					found = CRMatch::CR_MATCH_OPTIONAL;
					weight = rules2[i].weight;
				}
			} else {
				if (weight <= rules2[i].weight) {
					found = CRMatch::CR_MATCH_YES;
					weight = rules2[i].weight;
				}
			}
		}
	}
	return found;
}

bool CodeSignCodeResources::add_file1(const String &p_root, const String &p_path) {
	CRMatch found = match_rules1(p_path);
	if (found != CRMatch::CR_MATCH_YES && found != CRMatch::CR_MATCH_OPTIONAL) {
		return true; // No match.
	}

	CRFile f;
	f.name = p_path;
	f.optional = (found == CRMatch::CR_MATCH_OPTIONAL);
	f.nested = false;
	f.hash = hash_sha1_base64(p_root.path_join(p_path));
	print_verbose(vformat("CodeSign/CodeResources: File(V1) %s hash1:%s", f.name, f.hash));

	files1.push_back(f);
	return true;
}

bool CodeSignCodeResources::add_file2(const String &p_root, const String &p_path) {
	CRMatch found = match_rules2(p_path);
	if (found == CRMatch::CR_MATCH_NESTED) {
		return add_nested_file(p_root, p_path, p_root.path_join(p_path));
	}
	if (found != CRMatch::CR_MATCH_YES && found != CRMatch::CR_MATCH_OPTIONAL) {
		return true; // No match.
	}

	CRFile f;
	f.name = p_path;
	f.optional = (found == CRMatch::CR_MATCH_OPTIONAL);
	f.nested = false;
	f.hash = hash_sha1_base64(p_root.path_join(p_path));
	f.hash2 = hash_sha256_base64(p_root.path_join(p_path));

	print_verbose(vformat("CodeSign/CodeResources: File(V2) %s hash1:%s hash2:%s", f.name, f.hash, f.hash2));

	files2.push_back(f);
	return true;
}

bool CodeSignCodeResources::add_nested_file(const String &p_root, const String &p_path, const String &p_exepath) {
#define CLEANUP() \
	if (files_to_add.size() > 1) { \
		for (int j = 0; j < files_to_add.size(); j++) { \
			da->remove(files_to_add[j]); \
		} \
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND_V(da.is_null(), false);

	Vector<String> files_to_add;
	if (LipO::is_lipo(p_exepath)) {
		String tmp_path_name = EditorPaths::get_temp_dir().path_join("_lipo");
		Error err = da->make_dir_recursive(tmp_path_name);
		ERR_FAIL_COND_V_MSG(err != OK, false, vformat("CodeSign/CodeResources: Failed to create \"%s\" subfolder.", tmp_path_name));
		LipO lip;
		if (lip.open_file(p_exepath)) {
			for (int i = 0; i < lip.get_arch_count(); i++) {
				if (!lip.extract_arch(i, tmp_path_name.path_join("_rqexe_" + itos(i)))) {
					CLEANUP();
					ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Failed to extract thin binary.");
				}
				files_to_add.push_back(tmp_path_name.path_join("_rqexe_" + itos(i)));
			}
		}
	} else if (MachO::is_macho(p_exepath)) {
		files_to_add.push_back(p_exepath);
	}

	CRFile f;
	f.name = p_path;
	f.optional = false;
	f.nested = true;
	for (int i = 0; i < files_to_add.size(); i++) {
		MachO mh;
		if (!mh.open_file(files_to_add[i])) {
			CLEANUP();
			ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Invalid executable file.");
		}
		PackedByteArray hash = mh.get_cdhash_sha256(); // Use SHA-256 variant, if available.
		if (hash.size() != 0x20) {
			hash = mh.get_cdhash_sha1(); // Use SHA-1 instead.
			if (hash.size() != 0x14) {
				CLEANUP();
				ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Unsigned nested executable file.");
			}
		}
		hash.resize(0x14); // Always clamp to 0x14 size.
		f.hash = CryptoCore::b64_encode_str(hash.ptr(), hash.size());

		PackedByteArray rq_blob = mh.get_requirements();
		String req_string;
		if (rq_blob.size() > 8) {
			CodeSignBlob rq = CodeSignBlob::make_requirements(rq_blob);
			Vector<String> rqs = rq.parse_requirements();
			for (int j = 0; j < rqs.size(); j++) {
				if (rqs[j].begins_with("designated => ")) {
					req_string = rqs[j].replace("designated => ", "");
				}
			}
		}
		if (req_string.is_empty()) {
			req_string = "cdhash H\"" + String::hex_encode_buffer(hash.ptr(), hash.size()) + "\"";
		}
		print_verbose(vformat("CodeSign/CodeResources: Nested object %s (cputype: %d) cdhash:%s designated rq:%s", f.name, mh.get_cputype(), f.hash, req_string));
		if (f.requirements != req_string) {
			if (i != 0) {
				f.requirements += " or ";
			}
			f.requirements += req_string;
		}
	}
	files2.push_back(f);

	CLEANUP();
	return true;

#undef CLEANUP
}

bool CodeSignCodeResources::add_folder_recursive(const String &p_root, const String &p_path, const String &p_main_exe_path) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND_V(da.is_null(), false);
	Error err = da->change_dir(p_root.path_join(p_path));
	ERR_FAIL_COND_V(err != OK, false);

	bool ret = true;
	da->list_dir_begin();
	String n = da->get_next();
	while (n != String()) {
		if (n != "." && n != "..") {
			String path = p_root.path_join(p_path).path_join(n);
			if (path == p_main_exe_path) {
				n = da->get_next();
				continue; // Skip main executable.
			}
			if (da->current_is_dir()) {
				CRMatch found = match_rules2(p_path.path_join(n));
				String fmw_ver = "Current"; // Framework version (default).
				String info_path;
				String main_exe;
				bool bundle = false;
				if (da->file_exists(path.path_join("Contents/Info.plist"))) {
					info_path = path.path_join("Contents/Info.plist");
					main_exe = path.path_join("Contents/MacOS");
					bundle = true;
				} else if (da->file_exists(path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver)))) {
					info_path = path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver));
					main_exe = path.path_join(vformat("Versions/%s", fmw_ver));
					bundle = true;
				} else if (da->file_exists(path.path_join("Resources/Info.plist"))) {
					info_path = path.path_join("Resources/Info.plist");
					main_exe = path;
					bundle = true;
				} else if (da->file_exists(path.path_join("Info.plist"))) {
					info_path = path.path_join("Info.plist");
					main_exe = path;
					bundle = true;
				}
				if (bundle && found == CRMatch::CR_MATCH_NESTED && !info_path.is_empty()) {
					// Read Info.plist.
					PList info_plist;
					if (info_plist.load_file(info_path)) {
						if (info_plist.get_root()->data_type == PList::PLNodeType::PL_NODE_TYPE_DICT && info_plist.get_root()->data_dict.has("CFBundleExecutable")) {
							main_exe = main_exe.path_join(String::utf8(info_plist.get_root()->data_dict["CFBundleExecutable"]->data_string.get_data()));
						} else {
							ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Invalid Info.plist, no exe name.");
						}
					} else {
						ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Invalid Info.plist, can't load.");
					}
					ret = ret && add_nested_file(p_root, p_path.path_join(n), main_exe);
				} else {
					ret = ret && add_folder_recursive(p_root, p_path.path_join(n), p_main_exe_path);
				}
			} else {
				ret = ret && add_file1(p_root, p_path.path_join(n));
				ret = ret && add_file2(p_root, p_path.path_join(n));
			}
		}

		n = da->get_next();
	}

	da->list_dir_end();
	return ret;
}

bool CodeSignCodeResources::save_to_file(const String &p_path) {
	PList pl;

	print_verbose(vformat("CodeSign/CodeResources: Writing to file: %s", p_path));

	// Write version 1 hashes.
	Ref<PListNode> files1_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(files1_dict, "files");
	for (int i = 0; i < files1.size(); i++) {
		if (files1[i].optional) {
			Ref<PListNode> file_dict = PListNode::new_dict();
			files1_dict->push_subnode(file_dict, files1[i].name);

			file_dict->push_subnode(PListNode::new_data(files1[i].hash), "hash");
			file_dict->push_subnode(PListNode::new_bool(true), "optional");
		} else {
			files1_dict->push_subnode(PListNode::new_data(files1[i].hash), files1[i].name);
		}
	}

	// Write version 2 hashes.
	Ref<PListNode> files2_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(files2_dict, "files2");
	for (int i = 0; i < files2.size(); i++) {
		Ref<PListNode> file_dict = PListNode::new_dict();
		files2_dict->push_subnode(file_dict, files2[i].name);

		if (files2[i].nested) {
			file_dict->push_subnode(PListNode::new_data(files2[i].hash), "cdhash");
			file_dict->push_subnode(PListNode::new_string(files2[i].requirements), "requirement");
		} else {
			file_dict->push_subnode(PListNode::new_data(files2[i].hash), "hash");
			file_dict->push_subnode(PListNode::new_data(files2[i].hash2), "hash2");
			if (files2[i].optional) {
				file_dict->push_subnode(PListNode::new_bool(true), "optional");
			}
		}
	}

	// Write version 1 rules.
	Ref<PListNode> rules1_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(rules1_dict, "rules");
	for (int i = 0; i < rules1.size(); i++) {
		if (rules1[i].store) {
			if (rules1[i].key.is_empty() && rules1[i].weight <= 0) {
				rules1_dict->push_subnode(PListNode::new_bool(true), rules1[i].file_pattern);
			} else {
				Ref<PListNode> rule_dict = PListNode::new_dict();
				rules1_dict->push_subnode(rule_dict, rules1[i].file_pattern);
				if (!rules1[i].key.is_empty()) {
					rule_dict->push_subnode(PListNode::new_bool(true), rules1[i].key);
				}
				if (rules1[i].weight != 1) {
					rule_dict->push_subnode(PListNode::new_real(rules1[i].weight), "weight");
				}
			}
		}
	}

	// Write version 2 rules.
	Ref<PListNode> rules2_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(rules2_dict, "rules2");
	for (int i = 0; i < rules2.size(); i++) {
		if (rules2[i].store) {
			if (rules2[i].key.is_empty() && rules2[i].weight <= 0) {
				rules2_dict->push_subnode(PListNode::new_bool(true), rules2[i].file_pattern);
			} else {
				Ref<PListNode> rule_dict = PListNode::new_dict();
				rules2_dict->push_subnode(rule_dict, rules2[i].file_pattern);
				if (!rules2[i].key.is_empty()) {
					rule_dict->push_subnode(PListNode::new_bool(true), rules2[i].key);
				}
				if (rules2[i].weight != 1) {
					rule_dict->push_subnode(PListNode::new_real(rules2[i].weight), "weight");
				}
			}
		}
	}
	String text = pl.save_text();
	ERR_FAIL_COND_V_MSG(text.is_empty(), false, "CodeSign/CodeResources: Generating resources PList failed.");

	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("CodeSign/CodeResources: Can't open file: \"%s\".", p_path));

	CharString cs = text.utf8();
	fa->store_buffer((const uint8_t *)cs.ptr(), cs.length());
	return true;
}



/*************************************************************************/
/* CodeSignBlob                                                          */
/*************************************************************************/

CodeSignBlob CodeSignBlob::make_requirements() {
	CodeSignBlob b;
	b.index_type = INDEX_REQUIREMENTS;
	b.blob.append_array({ 0xFA, 0xDE, 0x0C, 0x01 }); // Requirement set magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x0C }); // Length of requirements set (12 bytes).
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x00 }); // Empty.
	return b;
}

CodeSignBlob CodeSignBlob::make_requirements(const PackedByteArray &p_data) {
	CodeSignBlob b;
	b.index_type = INDEX_REQUIREMENTS;
	b.blob = p_data;
	return b;
}

_FORCE_INLINE_ void CodeSignBlob::_parse_certificate_slot(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_out += "certificate ";
	uint32_t tag_slot = _R(r_pos);
	if (tag_slot == 0x00000000) {
		r_out += "leaf";
	} else if (tag_slot == 0xffffffff) {
		r_out += "root";
	} else {
		r_out += itos((int32_t)tag_slot);
	}
	r_pos += 4;
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_key(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	const uint32_t key_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + key_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_pos += 4 + key_size + PAD(key_size, 4);
	r_out += "[" + String::utf8((const char *)blob.ptr() + r_pos + 4, key_size) + "]";
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_oid_key(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	uint32_t key_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + key_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_out += "[field.";
	r_out += itos(blob[r_pos + 4] / 40) + ".";
	r_out += itos(blob[r_pos + 4] % 40);
	uint32_t spos = r_pos + 5;
	while (spos < r_pos + 4 + key_size) {
		r_out += ".";
		if (blob[spos] <= 127) {
			r_out += itos(blob[spos]);
			spos += 1;
		} else {
			uint32_t x = (0x7F & blob[spos]) << 7;
			spos += 1;
			while (blob[spos] > 127) {
				x = (x + (0x7F & blob[spos])) << 7;
				spos += 1;
			}
			x = (x + (0x7F & blob[spos]));
			r_out += itos(x);
			spos += 1;
		}
	}
	r_out += "]";
	r_pos += 4 + key_size + PAD(key_size, 4);
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_hash_string(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	uint32_t tag_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + tag_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_out += "H\"" + String::hex_encode_buffer(blob.ptr() + r_pos + 4, tag_size) + "\"";
	r_pos += 4 + tag_size + PAD(tag_size, 4);
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_value(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	const uint32_t key_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + key_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_pos += 4 + key_size + PAD(key_size, 4);
	r_out += "\"" + String::utf8((const char *)blob.ptr() + r_pos + 4, key_size) + "\"";
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_date(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	uint32_t date = _R(r_pos);
	time_t t = 978307200 + date;
	struct tm lt;
#ifdef WINDOWS_ENABLED
	gmtime_s(&lt, &t);
#else
	gmtime_r(&t, &lt);
#endif
	r_out += vformat("<%04d-%02d-%02d ", (int)(1900 + lt.tm_year), (int)(lt.tm_mon + 1), (int)(lt.tm_mday)) + vformat("%02d:%02d:%02d +0000>", (int)(lt.tm_hour), (int)(lt.tm_min), (int)(lt.tm_sec));
#undef _R
}

_FORCE_INLINE_ bool CodeSignBlob::_parse_match(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_V_MSG(r_pos >= p_rq_size, false, "CodeSign/Requirements: Out of bounds.");
	uint32_t match = _R(r_pos);
	r_pos += 4;
	switch (match) {
		case 0x00000000: {
			r_out += "exists";
		} break;
		case 0x00000001: {
			r_out += "= ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000002: {
			r_out += "~ ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000003: {
			r_out += "= *";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000004: {
			r_out += "= ";
			_parse_value(r_pos, r_out, p_rq_size);
			r_out += "*";
		} break;
		case 0x00000005: {
			r_out += "< ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000006: {
			r_out += "> ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000007: {
			r_out += "<= ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000008: {
			r_out += ">= ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000009: {
			r_out += "= ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000A: {
			r_out += "< ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000B: {
			r_out += "> ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000C: {
			r_out += "<= ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000D: {
			r_out += ">= ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000E: {
			r_out += "absent";
		} break;
		default: {
			return false;
		}
	}
	return true;
#undef _R
}

Vector<String> CodeSignBlob::parse_requirements() const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	Vector<String> list;

	// Read requirements set header.
	ERR_FAIL_COND_V_MSG(blob.size() < 12, list, "CodeSign/Requirements: Blob is too small.");
	uint32_t magic = _R(0);
	ERR_FAIL_COND_V_MSG(magic != 0xfade0c01, list, "CodeSign/Requirements: Invalid set magic.");
	uint32_t size = _R(4);
	ERR_FAIL_COND_V_MSG(size != (uint32_t)blob.size(), list, "CodeSign/Requirements: Invalid set size.");
	uint32_t count = _R(8);

	for (uint32_t i = 0; i < count; i++) {
		String out;

		// Read requirement header.
		uint32_t rq_type = _R(12 + i * 8);
		uint32_t rq_offset = _R(12 + i * 8 + 4);
		ERR_FAIL_COND_V_MSG(rq_offset + 12 >= (uint32_t)blob.size(), list, "CodeSign/Requirements: Invalid requirement offset.");
		switch (rq_type) {
			case 0x00000001: {
				out += "host => ";
			} break;
			case 0x00000002: {
				out += "guest => ";
			} break;
			case 0x00000003: {
				out += "designated => ";
			} break;
			case 0x00000004: {
				out += "library => ";
			} break;
			case 0x00000005: {
				out += "plugin => ";
			} break;
			default: {
				ERR_FAIL_V_MSG(list, "CodeSign/Requirements: Invalid requirement type.");
			}
		}
		uint32_t rq_magic = _R(rq_offset);
		uint32_t rq_size = _R(rq_offset + 4);
		uint32_t rq_ver = _R(rq_offset + 8);
		uint32_t pos = rq_offset + 12;
		ERR_FAIL_COND_V_MSG(rq_magic != 0xfade0c00, list, "CodeSign/Requirements: Invalid requirement magic.");
		ERR_FAIL_COND_V_MSG(rq_ver != 0x00000001, list, "CodeSign/Requirements: Invalid requirement version.");

		// Read requirement tokens.
		List<String> tokens;
		while (pos < rq_offset + rq_size) {
			uint32_t rq_tag = _R(pos);
			pos += 4;
			String token;
			switch (rq_tag) {
				case 0x00000000: {
					token = "false";
				} break;
				case 0x00000001: {
					token = "true";
				} break;
				case 0x00000002: {
					token = "identifier ";
					_parse_value(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000003: {
					token = "anchor apple";
				} break;
				case 0x00000004: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					token += " ";
					_parse_hash_string(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000005: {
					token = "info";
					_parse_key(pos, token, rq_offset + rq_size);
					token += " = ";
					_parse_value(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000006: {
					token = "and";
				} break;
				case 0x00000007: {
					token = "or";
				} break;
				case 0x00000008: {
					token = "cdhash ";
					_parse_hash_string(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000009: {
					token = "!";
				} break;
				case 0x0000000A: {
					token = "info";
					_parse_key(pos, token, rq_offset + rq_size);
					token += " ";
					ERR_FAIL_COND_V_MSG(!_parse_match(pos, token, rq_offset + rq_size), list, "CodeSign/Requirements: Unsupported match suffix.");
				} break;
				case 0x0000000B: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					_parse_key(pos, token, rq_offset + rq_size);
					token += " ";
					ERR_FAIL_COND_V_MSG(!_parse_match(pos, token, rq_offset + rq_size), list, "CodeSign/Requirements: Unsupported match suffix.");
				} break;
				case 0x0000000C: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					token += " trusted";
				} break;
				case 0x0000000D: {
					token = "anchor trusted";
				} break;
				case 0x0000000E: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					_parse_oid_key(pos, token, rq_offset + rq_size);
					token += " ";
					ERR_FAIL_COND_V_MSG(!_parse_match(pos, token, rq_offset + rq_size), list, "CodeSign/Requirements: Unsupported match suffix.");
				} break;
				case 0x0000000F: {
					token = "anchor apple generic";
				} break;
				default: {
					ERR_FAIL_V_MSG(list, "CodeSign/Requirements: Invalid requirement token.");
				} break;
			}
			tokens.push_back(token);
		}

		// Polish to infix notation (w/o bracket optimization).
		for (List<String>::Element *E = tokens.back(); E; E = E->prev()) {
			if (E->get() == "and") {
				ERR_FAIL_COND_V_MSG(!E->next() || !E->next()->next(), list, "CodeSign/Requirements: Invalid token sequence.");
				String token = "(" + E->next()->get() + " and " + E->next()->next()->get() + ")";
				tokens.erase(E->next()->next());
				tokens.erase(E->next());
				E->get() = token;
			} else if (E->get() == "or") {
				ERR_FAIL_COND_V_MSG(!E->next() || !E->next()->next(), list, "CodeSign/Requirements: Invalid token sequence.");
				String token = "(" + E->next()->get() + " or " + E->next()->next()->get() + ")";
				tokens.erase(E->next()->next());
				tokens.erase(E->next());
				E->get() = token;
			}
		}

		if (tokens.size() == 1) {
			list.push_back(out + tokens.front()->get());
		} else {
			ERR_FAIL_V_MSG(list, "CodeSign/Requirements: Invalid token sequence.");
		}
	}

	return list;
#undef _R
}

CodeSignBlob CodeSignBlob::make_entitlements_text() {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_TEXT;
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x71 }); // Text Entitlements set magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x08 }); // Length (8 bytes).
	return b;
}

CodeSignBlob CodeSignBlob::make_entitlements_text(const String &p_string) {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_TEXT;
	CharString utf8 = p_string.utf8();
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x71 }); // Text Entitlements set magic.
	for (int i = 3; i >= 0; i--) {
		uint8_t x = ((utf8.length() + 8) >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	for (int i = 0; i < utf8.length(); i++) { // Write data.
		b.blob.push_back(utf8[i]);
	}
	return b;
}

CodeSignBlob CodeSignBlob::make_entitlements_binary() {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_BINARY;
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x72 }); // Binary Entitlements magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x08 }); // Length (8 bytes).
	return b;
}

CodeSignBlob CodeSignBlob::make_entitlements_binary(const String &p_string) {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_BINARY;
	PList pl = PList(p_string);

	PackedByteArray asn1 = pl.save_asn1();
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x72 }); // Binary Entitlements magic.
	uint32_t size = asn1.size() + 8;
	for (int i = 3; i >= 0; i--) {
		uint8_t x = (size >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	b.blob.append_array(asn1); // Write data.
	return b;
}

CodeSignBlob CodeSignBlob::make_code_directory() {
	CodeSignBlob b;
	b.index_type = INDEX_CODE_DIRECTORY;
	b.blob.append_array({ 0xFA, 0xDE, 0x0C, 0x02 }); // Code Directory magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x00 }); // Size (8 bytes).
	return b;
}

CodeSignBlob CodeSignBlob::make_code_directory(uint8_t p_hash_size, uint8_t p_hash_type, bool p_main, const CharString &p_id, const CharString &p_team_id, uint32_t p_page_size, uint64_t p_exe_limit, uint64_t p_code_limit) {
	CodeSignBlob b;
	b.index_type = INDEX_CODE_DIRECTORY;
	b.pages = p_code_limit / (uint64_t(1) << p_page_size);
	b.remain = p_code_limit % (uint64_t(1) << p_page_size);
	b.code_slots = b.pages + (b.remain > 0 ? 1 : 0);
	b.special_slots = 7;

	int cd_size = 8 + sizeof(CodeDirectoryHeader) + (b.code_slots + b.special_slots) * p_hash_size + p_id.size() + p_team_id.size();
	int cd_off = 8 + sizeof(CodeDirectoryHeader);
	b.blob.append_array({ 0xFA, 0xDE, 0x0C, 0x02 }); // Code Directory magic.
	for (int i = 3; i >= 0; i--) {
		uint8_t x = (cd_size >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	b.blob.resize(cd_size);
	memset(b.blob.ptrw() + 8, 0x00, cd_size - 8);
	CodeDirectoryHeader *cd = reinterpret_cast<CodeDirectoryHeader *>(b.blob.ptrw() + 8);

	bool is_64_cl = (p_code_limit >= std::numeric_limits<uint32_t>::max());

	// Version and options.
	cd->version = BSWAP32(0x20500);
	cd->flags = BSWAP32(SIGNATURE_ADHOC | SIGNATURE_RUNTIME);
	cd->special_slots = BSWAP32(b.special_slots);
	cd->code_slots = BSWAP32(b.code_slots);
	if (is_64_cl) {
		cd->code_limit_64 = BSWAP64(p_code_limit);
	} else {
		cd->code_limit = BSWAP32(p_code_limit);
	}
	cd->hash_size = p_hash_size;
	cd->hash_type = p_hash_type;
	cd->page_size = p_page_size;
	cd->exec_seg_base = 0x00;
	cd->exec_seg_limit = BSWAP64(p_exe_limit);
	cd->exec_seg_flags = 0;
	if (p_main) {
		cd->exec_seg_flags |= EXECSEG_MAIN_BINARY;
	}
	cd->exec_seg_flags = BSWAP64(cd->exec_seg_flags);
	uint32_t version = (11 << 16) + (3 << 8) + 0; // Version 11.3.0
	cd->runtime = BSWAP32(version);

	// Copy ID.
	cd->ident_offset = BSWAP32(cd_off);
	memcpy(b.blob.ptrw() + cd_off, p_id.get_data(), p_id.size());
	cd_off += p_id.size();

	// Copy Team ID.
	if (p_team_id.length() > 0) {
		cd->team_offset = BSWAP32(cd_off);
		memcpy(b.blob.ptrw() + cd_off, p_team_id.get_data(), p_team_id.size());
		cd_off += p_team_id.size();
	} else {
		cd->team_offset = 0;
	}

	// Scatter vector.
	cd->scatter_vector_offset = 0; // Not used.

	// Executable hashes offset.
	cd->hash_offset = BSWAP32(cd_off + b.special_slots * cd->hash_size);
	return b;
}

bool CodeSignBlob::set_hash_in_slot(const PackedByteArray &p_hash, int p_slot) {
	ERR_FAIL_COND_V_MSG((p_slot < -special_slots) || (p_slot >= code_slots), false, vformat("CodeSign/CodeDirectory: Invalid hash slot index: %d.", p_slot));
	CodeDirectoryHeader *cd = reinterpret_cast<CodeDirectoryHeader *>(blob.ptrw() + 8);
	for (int i = 0; i < cd->hash_size; i++) {
		blob.write[BSWAP32(cd->hash_offset) + p_slot * cd->hash_size + i] = p_hash[i];
	}
	return true;
}

CodeSignBlob CodeSignBlob::make_signature() {
	CodeSignBlob b;
	b.index_type = INDEX_SIGNATURE;
	b.blob.append_array({ 0xFA, 0xDE, 0x0B, 0x01 }); // Signature magic.
	uint32_t sign_size = 8; // Ad-hoc signature is empty.
	for (int i = 3; i >= 0; i--) {
		uint8_t x = (sign_size >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	return b;
}

PackedByteArray CodeSignBlob::get_hash_sha1() const {
	PackedByteArray hash;
	hash.resize(0x14);

	CryptoCore::SHA1Context ctx;
	ctx.start();
	ctx.update(blob.ptr(), blob.size());
	ctx.finish(hash.ptrw());

	return hash;
}

PackedByteArray CodeSignBlob::get_hash_sha256() const {
	PackedByteArray hash;
	hash.resize(0x20);

	CryptoCore::SHA256Context ctx;
	ctx.start();
	ctx.update(blob.ptr(), blob.size());
	ctx.finish(hash.ptrw());

	return hash;
}

void CodeSignBlob::write_to_file(Ref<FileAccess> p_file) const {
	ERR_FAIL_COND_MSG(p_file.is_null(), "CodeSignBlob: Invalid file handle.");
	p_file->store_buffer(blob.ptr(), blob.size());
}

/*************************************************************************/
/* CodeSignSuperBlob                                                     */
/*************************************************************************/

bool CodeSignSuperBlob::add_blob(const CodeSignBlob &p_blob) {
	if (!p_blob.is_empty()) {
		blobs.push_back(p_blob);
		return true;
	} else {
		return false;
	}
}

int CodeSignSuperBlob::get_size() const {
	int size = 12 + blobs.size() * 8;
	for (int i = 0; i < blobs.size(); i++) {
		if (blobs[i].is_empty()) {
			return 0;
		}
		size += blobs[i].get_size();
	}
	return size;
}

void CodeSignSuperBlob::write_to_file(Ref<FileAccess> p_file) const {
	ERR_FAIL_COND_MSG(p_file.is_null(), "CodeSign/SuperBlob: Invalid file handle.");
	uint32_t size = get_size();
	uint32_t data_offset = 12 + blobs.size() * 8;

	// Write header.
	p_file->store_32(BSWAP32(0xfade0cc0));
	p_file->store_32(BSWAP32(size));
	p_file->store_32(BSWAP32(blobs.size()));

	// Write index.
	for (int i = 0; i < blobs.size(); i++) {
		if (blobs[i].is_empty()) {
			return;
		}
		p_file->store_32(BSWAP32(blobs[i].get_index_type()));
		p_file->store_32(BSWAP32(data_offset));
		data_offset += blobs[i].get_size();
	}

	// Write blobs.
	for (int i = 0; i < blobs.size(); i++) {
		blobs[i].write_to_file(p_file);
	}
}


/*************************************************************************/
/* CodeSign                                                              */
/*************************************************************************/

PackedByteArray CodeSign::file_hash_sha1(const String &p_path) {
	PackedByteArray file_hash;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), PackedByteArray(), vformat("CodeSign: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA1Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = f->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	file_hash.resize(0x14);
	ctx.finish(file_hash.ptrw());
	return file_hash;
}

PackedByteArray CodeSign::file_hash_sha256(const String &p_path) {
	PackedByteArray file_hash;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), PackedByteArray(), vformat("CodeSign: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA256Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = f->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	file_hash.resize(0x20);
	ctx.finish(file_hash.ptrw());
	return file_hash;
}

Error CodeSign::_codesign_file(bool p_use_hardened_runtime, bool p_force, const String &p_info, const String &p_exe_path, const String &p_bundle_path, const String &p_ent_path, bool p_ios_bundle, String &r_error_msg) {
#define CLEANUP() \
	if (files_to_sign.size() > 1) { \
		for (int j = 0; j < files_to_sign.size(); j++) { \
			da->remove(files_to_sign[j]); \
		} \
	}

	print_verbose(vformat("CodeSign: Signing executable: %s, bundle: %s with entitlements %s", p_exe_path, p_bundle_path, p_ent_path));

	PackedByteArray info_hash1, info_hash2;
	PackedByteArray res_hash1, res_hash2;
	String id;
	String main_exe = p_exe_path;

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		r_error_msg = TTR("Can't get filesystem access.");
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "CodeSign: Can't get filesystem access.");
	}

	// Read Info.plist.
	if (!p_info.is_empty()) {
		print_verbose("CodeSign: Reading bundle info...");
		PList info_plist;
		if (info_plist.load_file(p_info)) {
			info_hash1 = file_hash_sha1(p_info);
			info_hash2 = file_hash_sha256(p_info);
			if (info_hash1.is_empty() || info_hash2.is_empty()) {
				r_error_msg = TTR("Failed to get Info.plist hash.");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to get Info.plist hash.");
			}

			if (info_plist.get_root()->data_type == PList::PLNodeType::PL_NODE_TYPE_DICT && info_plist.get_root()->data_dict.has("CFBundleExecutable")) {
				main_exe = p_exe_path.path_join(String::utf8(info_plist.get_root()->data_dict["CFBundleExecutable"]->data_string.get_data()));
			} else {
				r_error_msg = TTR("Invalid Info.plist, no exe name.");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid Info.plist, no exe name.");
			}

			if (info_plist.get_root()->data_type == PList::PLNodeType::PL_NODE_TYPE_DICT && info_plist.get_root()->data_dict.has("CFBundleIdentifier")) {
				id = info_plist.get_root()->data_dict["CFBundleIdentifier"]->data_string.get_data();
			} else {
				r_error_msg = TTR("Invalid Info.plist, no bundle id.");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid Info.plist, no bundle id.");
			}
		} else {
			r_error_msg = TTR("Invalid Info.plist, can't load.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid Info.plist, can't load.");
		}
	}

	// Extract fat binary.
	Vector<String> files_to_sign;
	if (LipO::is_lipo(main_exe)) {
		print_verbose(vformat("CodeSign: Executable is fat, extracting..."));
		String tmp_path_name = EditorPaths::get_temp_dir().path_join("_lipo");
		Error err = da->make_dir_recursive(tmp_path_name);
		if (err != OK) {
			r_error_msg = vformat(TTR("Failed to create \"%s\" subfolder."), tmp_path_name);
			ERR_FAIL_V_MSG(FAILED, vformat("CodeSign: Failed to create \"%s\" subfolder.", tmp_path_name));
		}
		LipO lip;
		if (lip.open_file(main_exe)) {
			for (int i = 0; i < lip.get_arch_count(); i++) {
				if (!lip.extract_arch(i, tmp_path_name.path_join("_exe_" + itos(i)))) {
					CLEANUP();
					r_error_msg = TTR("Failed to extract thin binary.");
					ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to extract thin binary.");
				}
				files_to_sign.push_back(tmp_path_name.path_join("_exe_" + itos(i)));
			}
		}
	} else if (MachO::is_macho(main_exe)) {
		print_verbose("CodeSign: Executable is thin...");
		files_to_sign.push_back(main_exe);
	} else {
		r_error_msg = TTR("Invalid binary format.");
		ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid binary format.");
	}

	// Check if it's already signed.
	if (!p_force) {
		for (int i = 0; i < files_to_sign.size(); i++) {
			MachO mh;
			mh.open_file(files_to_sign[i]);
			if (mh.is_signed()) {
				CLEANUP();
				r_error_msg = TTR("Already signed!");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Already signed!");
			}
		}
	}

	// Generate core resources.
	if (!p_bundle_path.is_empty()) {
		print_verbose("CodeSign: Generating bundle CodeResources...");
		CodeSignCodeResources cr;

		if (p_ios_bundle) {
			cr.add_rule1("^.*");
			cr.add_rule1("^.*\\.lproj/", "optional", 100);
			cr.add_rule1("^.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule1("^Base\\.lproj/", "", 1010);
			cr.add_rule1("^version.plist$");

			cr.add_rule2(".*\\.dSYM($|/)", "", 11);
			cr.add_rule2("^(.*/)?\\.DS_Store$", "omit", 2000);
			cr.add_rule2("^.*");
			cr.add_rule2("^.*\\.lproj/", "optional", 1000);
			cr.add_rule2("^.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule2("^Base\\.lproj/", "", 1010);
			cr.add_rule2("^Info\\.plist$", "omit", 20);
			cr.add_rule2("^PkgInfo$", "omit", 20);
			cr.add_rule2("^embedded\\.provisionprofile$", "", 10);
			cr.add_rule2("^version\\.plist$", "", 20);

			cr.add_rule2("^_MASReceipt", "omit", 2000, false);
			cr.add_rule2("^_CodeSignature", "omit", 2000, false);
			cr.add_rule2("^CodeResources", "omit", 2000, false);
		} else {
			cr.add_rule1("^Resources($|/)");
			cr.add_rule1("^Resources/.*\\.lproj/", "optional", 1000);
			cr.add_rule1("^Resources/.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule1("^Resources/Base\\.lproj/", "", 1010);
			cr.add_rule1("^version.plist$");

			cr.add_rule2(".*\\.dSYM($|/)", "", 11);
			cr.add_rule2("^(.*/)?\\.DS_Store$", "omit", 2000);
			cr.add_rule2("^(Frameworks|SharedFrameworks|PlugIns|Plug-ins|XPCServices|Helpers|MacOS|Library/(Automator|Spotlight|LoginItems))/", "nested", 10);
			cr.add_rule2("^.*");
			cr.add_rule2("^Info\\.plist$", "omit", 20);
			cr.add_rule2("^PkgInfo$", "omit", 20);
			cr.add_rule2("^Resources($|/)", "", 20);
			cr.add_rule2("^Resources/.*\\.lproj/", "optional", 1000);
			cr.add_rule2("^Resources/.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule2("^Resources/Base\\.lproj/", "", 1010);
			cr.add_rule2("^[^/]+$", "nested", 10);
			cr.add_rule2("^embedded\\.provisionprofile$", "", 10);
			cr.add_rule2("^version\\.plist$", "", 20);
			cr.add_rule2("^_MASReceipt", "omit", 2000, false);
			cr.add_rule2("^_CodeSignature", "omit", 2000, false);
			cr.add_rule2("^CodeResources", "omit", 2000, false);
		}

		if (!cr.add_folder_recursive(p_bundle_path, "", main_exe)) {
			CLEANUP();
			r_error_msg = TTR("Failed to process nested resources.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to process nested resources.");
		}
		Error err = da->make_dir_recursive(p_bundle_path.path_join("_CodeSignature"));
		if (err != OK) {
			CLEANUP();
			r_error_msg = TTR("Failed to create _CodeSignature subfolder.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to create _CodeSignature subfolder.");
		}
		cr.save_to_file(p_bundle_path.path_join("_CodeSignature").path_join("CodeResources"));
		res_hash1 = file_hash_sha1(p_bundle_path.path_join("_CodeSignature").path_join("CodeResources"));
		res_hash2 = file_hash_sha256(p_bundle_path.path_join("_CodeSignature").path_join("CodeResources"));
		if (res_hash1.is_empty() || res_hash2.is_empty()) {
			CLEANUP();
			r_error_msg = TTR("Failed to get CodeResources hash.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to get CodeResources hash.");
		}
	}

	// Generate common signature structures.
	if (id.is_empty()) {
		uint8_t uuid[16];
		Error err = CryptoCore::generate_random(uuid, 16);
		ERR_FAIL_COND_V_MSG(err, err, "Failed to generate UUID.");
		id = (String("a-55554944") /*a-UUID*/ + String::hex_encode_buffer(uuid, 16));
	}
	CharString uuid_str = id.utf8();
	print_verbose(vformat("CodeSign: Used bundle ID: %s", id));

	print_verbose("CodeSign: Processing entitlements...");

	CodeSignBlob cet;
	CodeSignBlob ceb;
	if (!p_ent_path.is_empty()) {
		String entitlements = FileAccess::get_file_as_string(p_ent_path);
		if (entitlements.is_empty()) {
			CLEANUP();
			r_error_msg = TTR("Invalid entitlements file.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid entitlements file.");
		}
		cet = CodeSignBlob::make_entitlements_text(entitlements);
		ceb = CodeSignBlob::make_entitlements_binary(entitlements);
	}

	print_verbose("CodeSign: Generating requirements...");
	String team_id = "";
	CodeSignBlob rq = CodeSignBlob::make_requirements();

	// Sign executables.
	for (int i = 0; i < files_to_sign.size(); i++) {
		MachO mh;
		if (!mh.open_file(files_to_sign[i])) {
			CLEANUP();
			r_error_msg = TTR("Invalid executable file.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid executable file.");
		}
		print_verbose(vformat("CodeSign: Signing executable for cputype: %d ...", mh.get_cputype()));

		print_verbose("CodeSign: Generating CodeDirectory...");
		CodeSignBlob cd1 = CodeSignBlob::make_code_directory(0x14, 0x01, true, uuid_str, team_id.utf8(), 12, mh.get_exe_limit(), mh.get_code_limit());
		CodeSignBlob cd2 = CodeSignBlob::make_code_directory(0x20, 0x02, true, uuid_str, team_id.utf8(), 12, mh.get_exe_limit(), mh.get_code_limit());
		print_verbose("CodeSign: Calculating special slot hashes...");
		if (info_hash2.size() == 0x20) {
			cd2.set_hash_in_slot(info_hash2, CodeSignBlob::SLOT_INFO_PLIST);
		}
		if (info_hash1.size() == 0x14) {
			cd1.set_hash_in_slot(info_hash1, CodeSignBlob::SLOT_INFO_PLIST);
		}
		cd1.set_hash_in_slot(rq.get_hash_sha1(), CodeSignBlob::SLOT_REQUIREMENTS);
		cd2.set_hash_in_slot(rq.get_hash_sha256(), CodeSignBlob::SLOT_REQUIREMENTS);
		if (res_hash2.size() == 0x20) {
			cd2.set_hash_in_slot(res_hash2, CodeSignBlob::SLOT_RESOURCES);
		}
		if (res_hash1.size() == 0x14) {
			cd1.set_hash_in_slot(res_hash1, CodeSignBlob::SLOT_RESOURCES);
		}
		if (!cet.is_empty()) {
			cd1.set_hash_in_slot(cet.get_hash_sha1(), CodeSignBlob::SLOT_ENTITLEMENTS); //Text variant.
			cd2.set_hash_in_slot(cet.get_hash_sha256(), CodeSignBlob::SLOT_ENTITLEMENTS);
		}
		if (!ceb.is_empty()) {
			cd1.set_hash_in_slot(ceb.get_hash_sha1(), CodeSignBlob::SLOT_DER_ENTITLEMENTS); //ASN.1 variant.
			cd2.set_hash_in_slot(ceb.get_hash_sha256(), CodeSignBlob::SLOT_DER_ENTITLEMENTS);
		}

		// Calculate signature size.
		int sign_size = 12; // SuperBlob header.
		sign_size += cd1.get_size() + 8;
		sign_size += cd2.get_size() + 8;
		sign_size += rq.get_size() + 8;
		if (!cet.is_empty()) {
			sign_size += cet.get_size() + 8;
		}
		if (!ceb.is_empty()) {
			sign_size += ceb.get_size() + 8;
		}
		sign_size += 16; // Empty signature size.

		// Alloc/resize signature load command.
		print_verbose(vformat("CodeSign: Reallocating space for the signature superblob (%d)...", sign_size));
		if (!mh.set_signature_size(sign_size)) {
			CLEANUP();
			r_error_msg = TTR("Can't resize signature load command.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Can't resize signature load command.");
		}

		print_verbose("CodeSign: Calculating executable code hashes...");
		// Calculate executable code hashes.
		PackedByteArray buffer;
		PackedByteArray hash1, hash2;
		hash1.resize(0x14);
		hash2.resize(0x20);
		buffer.resize(1 << 12);
		mh.get_file()->seek(0);
		for (int32_t j = 0; j < cd2.get_page_count(); j++) {
			mh.get_file()->get_buffer(buffer.ptrw(), (1 << 12));
			CryptoCore::SHA256Context ctx2;
			ctx2.start();
			ctx2.update(buffer.ptr(), (1 << 12));
			ctx2.finish(hash2.ptrw());
			cd2.set_hash_in_slot(hash2, j);

			CryptoCore::SHA1Context ctx1;
			ctx1.start();
			ctx1.update(buffer.ptr(), (1 << 12));
			ctx1.finish(hash1.ptrw());
			cd1.set_hash_in_slot(hash1, j);
		}
		if (cd2.get_page_remainder() > 0) {
			mh.get_file()->get_buffer(buffer.ptrw(), cd2.get_page_remainder());
			CryptoCore::SHA256Context ctx2;
			ctx2.start();
			ctx2.update(buffer.ptr(), cd2.get_page_remainder());
			ctx2.finish(hash2.ptrw());
			cd2.set_hash_in_slot(hash2, cd2.get_page_count());

			CryptoCore::SHA1Context ctx1;
			ctx1.start();
			ctx1.update(buffer.ptr(), cd1.get_page_remainder());
			ctx1.finish(hash1.ptrw());
			cd1.set_hash_in_slot(hash1, cd1.get_page_count());
		}

		print_verbose("CodeSign: Generating signature...");
		CodeSignBlob cs = CodeSignBlob::make_signature();

		print_verbose("CodeSign: Writing signature superblob...");
		// Write signature data to the executable.
		CodeSignSuperBlob sb = CodeSignSuperBlob();
		sb.add_blob(cd2);
		sb.add_blob(cd1);
		sb.add_blob(rq);
		if (!cet.is_empty()) {
			sb.add_blob(cet);
		}
		if (!ceb.is_empty()) {
			sb.add_blob(ceb);
		}
		sb.add_blob(cs);
		mh.get_file()->seek(mh.get_signature_offset());
		sb.write_to_file(mh.get_file());
	}
	if (files_to_sign.size() > 1) {
		print_verbose("CodeSign: Rebuilding fat executable...");
		LipO lip;
		if (!lip.create_file(main_exe, files_to_sign)) {
			CLEANUP();
			r_error_msg = TTR("Failed to create fat binary.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to create fat binary.");
		}
		CLEANUP();
	}
	FileAccess::set_unix_permissions(main_exe, 0755); // Restore unix permissions.
	return OK;
#undef CLEANUP
}

Error CodeSign::codesign(bool p_use_hardened_runtime, bool p_force, const String &p_path, const String &p_ent_path, String &r_error_msg) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		r_error_msg = TTR("Can't get filesystem access.");
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "CodeSign: Can't get filesystem access.");
	}

	if (da->dir_exists(p_path)) {
		String fmw_ver = "Current"; // Framework version (default).
		String info_path;
		String main_exe;
		String bundle_path;
		bool bundle = false;
		bool ios_bundle = false;
		if (da->file_exists(p_path.path_join("Contents/Info.plist"))) {
			info_path = p_path.path_join("Contents/Info.plist");
			main_exe = p_path.path_join("Contents/MacOS");
			bundle_path = p_path.path_join("Contents");
			bundle = true;
		} else if (da->file_exists(p_path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver)))) {
			info_path = p_path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver));
			main_exe = p_path.path_join(vformat("Versions/%s", fmw_ver));
			bundle_path = p_path.path_join(vformat("Versions/%s", fmw_ver));
			bundle = true;
		} else if (da->file_exists(p_path.path_join("Resources/Info.plist"))) {
			info_path = p_path.path_join("Resources/Info.plist");
			main_exe = p_path;
			bundle_path = p_path;
			bundle = true;
		} else if (da->file_exists(p_path.path_join("Info.plist"))) {
			info_path = p_path.path_join("Info.plist");
			main_exe = p_path;
			bundle_path = p_path;
			bundle = true;
			ios_bundle = true;
		}
		if (bundle) {
			return _codesign_file(p_use_hardened_runtime, p_force, info_path, main_exe, bundle_path, p_ent_path, ios_bundle, r_error_msg);
		} else {
			r_error_msg = TTR("Unknown bundle type.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Unknown bundle type.");
		}
	} else if (da->file_exists(p_path)) {
		return _codesign_file(p_use_hardened_runtime, p_force, "", p_path, "", p_ent_path, false, r_error_msg);
	} else {
		r_error_msg = TTR("Unknown object type.");
		ERR_FAIL_V_MSG(FAILED, "CodeSign: Unknown object type.");
	}
}

/*************************************************************************/
/* PluginConfigAppleEmbedded                                             */
/*************************************************************************/

String PluginConfigAppleEmbedded::resolve_local_dependency_path(
	String plugin_config_dir, String dependency_path)
{
	String absolute_path;

	if (dependency_path.is_empty()) {
		return absolute_path;
	}

	if (dependency_path.is_absolute_path()) {
		return dependency_path;
	}

	String res_path = ProjectSettings::globalize_path("res://");
	absolute_path = plugin_config_dir.path_join(dependency_path);

	return absolute_path.replace(res_path, "res://");
}

String PluginConfigAppleEmbedded::resolve_system_dependency_path(String dependency_path)
{
	String absolute_path;

	if (dependency_path.is_empty()) {
		return absolute_path;
	}

	if (dependency_path.is_absolute_path()) {
		return dependency_path;
	}

	String system_path = "/System/Library/Frameworks";

	return system_path.path_join(dependency_path);
}

Vector<String> PluginConfigAppleEmbedded::resolve_local_dependencies(
	String plugin_config_dir, Vector<String> p_paths)
{
	Vector<String> paths;

	for (int i = 0; i < p_paths.size(); i++) {
		String path = resolve_local_dependency_path(plugin_config_dir, p_paths[i]);

		if (path.is_empty()) {
			continue;
		}

		paths.push_back(path);
	}

	return paths;
}

Vector<String> PluginConfigAppleEmbedded::resolve_system_dependencies(Vector<String> p_paths)
{
	Vector<String> paths;

	for (int i = 0; i < p_paths.size(); i++) {
		String path = resolve_system_dependency_path(p_paths[i]);

		if (path.is_empty()) {
			continue;
		}

		paths.push_back(path);
	}

	return paths;
}

bool PluginConfigAppleEmbedded::validate_plugin(PluginConfigAppleEmbedded& plugin_config)
{
	bool valid_name = !plugin_config.name.is_empty();
	bool valid_binary_name = !plugin_config.binary.is_empty();
	bool valid_initialize = !plugin_config.initialization_method.is_empty();
	bool valid_deinitialize = !plugin_config.deinitialization_method.is_empty();

	bool fields_value = valid_name && valid_binary_name && valid_initialize && valid_deinitialize;

	if (!fields_value) {
		return false;
	}

	String plugin_extension = plugin_config.binary.get_extension().to_lower();

	if ((plugin_extension == "a" && FileAccess::exists(plugin_config.binary)) ||
		(plugin_extension == "xcframework" && DirAccess::exists(plugin_config.binary))) {
		plugin_config.valid_config = true;
		plugin_config.supports_targets = false;
	}
	else {
		String file_path = plugin_config.binary.get_base_dir();
		String file_name = plugin_config.binary.get_basename().get_file();
		String file_extension = plugin_config.binary.get_extension();
		String release_file_name = file_path.path_join(file_name + ".release." + file_extension);
		String debug_file_name = file_path.path_join(file_name + ".debug." + file_extension);

		if ((plugin_extension == "a" && FileAccess::exists(release_file_name) &&
				FileAccess::exists(debug_file_name)) ||
			(plugin_extension == "xcframework" && DirAccess::exists(release_file_name) &&
				DirAccess::exists(debug_file_name))) {
			plugin_config.valid_config = true;
			plugin_config.supports_targets = true;
		}
	}

	return plugin_config.valid_config;
}

String PluginConfigAppleEmbedded::get_plugin_main_binary(
	PluginConfigAppleEmbedded& plugin_config, bool p_debug)
{
	if (!plugin_config.supports_targets) {
		return plugin_config.binary;
	}

	String plugin_binary_dir = plugin_config.binary.get_base_dir();
	String plugin_name_prefix = plugin_config.binary.get_basename().get_file();
	String plugin_extension = plugin_config.binary.get_extension();
	String plugin_file =
		plugin_name_prefix + "." + (p_debug ? "debug" : "release") + "." + plugin_extension;

	return plugin_binary_dir.path_join(plugin_file);
}

uint64_t PluginConfigAppleEmbedded::get_plugin_modification_time(
	const PluginConfigAppleEmbedded& plugin_config, const String& config_path)
{
	uint64_t last_updated = FileAccess::get_modified_time(config_path);

	if (!plugin_config.supports_targets) {
		last_updated = MAX(last_updated, FileAccess::get_modified_time(plugin_config.binary));
	}
	else {
		String file_path = plugin_config.binary.get_base_dir();
		String file_name = plugin_config.binary.get_basename().get_file();
		String plugin_extension = plugin_config.binary.get_extension();
		String release_file_name = file_path.path_join(file_name + ".release." + plugin_extension);
		String debug_file_name = file_path.path_join(file_name + ".debug." + plugin_extension);

		last_updated = MAX(last_updated, FileAccess::get_modified_time(release_file_name));
		last_updated = MAX(last_updated, FileAccess::get_modified_time(debug_file_name));
	}

	return last_updated;
}

#ifdef CLEANUP
#undef CLEANUP
#endif
#ifdef _R
#undef _R
#endif

// ============================================================================
// Merged from editor/export/editor_export_platform_apple_embedded.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/themes/editor_scale.h"
#include "editor/export/editor_export_platform.h"
#include "main/main.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#include "modules/svg/image_loader_svg.h"
#include "servers/display/display_server.h"

#ifdef MACOS_ENABLED
#include "core/io/json.h"
#include "core/os/process_id.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#endif

Vector<EditorExportPlatformAppleEmbedded::ExportArchitecture>
EditorExportPlatformAppleEmbedded::_get_supported_architectures() const
{
	Vector<ExportArchitecture> archs;
	archs.push_back(ExportArchitecture("arm64", true));
	return archs;
}

struct APIAccessInfo
{
	String prop_name;
	String type_name;
	Vector<String> prop_flag_value;
	Vector<String> prop_flag_name;
	int default_value;
};

static const APIAccessInfo api_info[] = {
	{"file_timestamp", "NSPrivacyAccessedAPICategoryFileTimestamp", {"DDA9.1", "C617.1", "3B52.1"},
		{"Display to user on-device:", "Inside app or group container",
			"Files provided to app by user"},
		3},
	{"system_boot_time", "NSPrivacyAccessedAPICategorySystemBootTime",
		{"35F9.1", "8FFB.1", "3D61.1"},
		{"Measure time on-device", "Calculate absolute event timestamps",
			"User-initiated bug report"},
		1},
	{"disk_space", "NSPrivacyAccessedAPICategoryDiskSpace",
		{"E174.1", "85F4.1", "7D9E.1", "B728.1"},
		{"Write or delete file on-device", "Display to user on-device", "User-initiated bug report",
			"Health research app"},
		3},
	{"active_keyboard", "NSPrivacyAccessedAPICategoryActiveKeyboards", {"3EC4.1", "54BD.1"},
		{"Custom keyboard app on-device", "Customize UI on-device:2"}, 0},
	{"user_defaults", "NSPrivacyAccessedAPICategoryUserDefaults", {"1C8F.1", "AC6B.1", "CA92.1"},
		{"Access info from same App Group", "Access managed app configuration",
			"Access info from same app"},
		0},
};

struct DataCollectionInfo
{
	String prop_name;
	String type_name;
};

static const DataCollectionInfo data_collect_type_info[] = {
	{"name", "NSPrivacyCollectedDataTypeName"},
	{"email_address", "NSPrivacyCollectedDataTypeEmailAddress"},
	{"phone_number", "NSPrivacyCollectedDataTypePhoneNumber"},
	{"physical_address", "NSPrivacyCollectedDataTypePhysicalAddress"},
	{"other_contact_info", "NSPrivacyCollectedDataTypeOtherUserContactInfo"},
	{"health", "NSPrivacyCollectedDataTypeHealth"},
	{"fitness", "NSPrivacyCollectedDataTypeFitness"},
	{"payment_info", "NSPrivacyCollectedDataTypePaymentInfo"},
	{"credit_info", "NSPrivacyCollectedDataTypeCreditInfo"},
	{"other_financial_info", "NSPrivacyCollectedDataTypeOtherFinancialInfo"},
	{"precise_location", "NSPrivacyCollectedDataTypePreciseLocation"},
	{"coarse_location", "NSPrivacyCollectedDataTypeCoarseLocation"},
	{"sensitive_info", "NSPrivacyCollectedDataTypeSensitiveInfo"},
	{"contacts", "NSPrivacyCollectedDataTypeContacts"},
	{"emails_or_text_messages", "NSPrivacyCollectedDataTypeEmailsOrTextMessages"},
	{"photos_or_videos", "NSPrivacyCollectedDataTypePhotosorVideos"},
	{"audio_data", "NSPrivacyCollectedDataTypeAudioData"},
	{"gameplay_content", "NSPrivacyCollectedDataTypeGameplayContent"},
	{"customer_support", "NSPrivacyCollectedDataTypeCustomerSupport"},
	{"other_user_content", "NSPrivacyCollectedDataTypeOtherUserContent"},
	{"browsing_history", "NSPrivacyCollectedDataTypeBrowsingHistory"},
	{"search_history", "NSPrivacyCollectedDataTypeSearchHistory"},
	{"user_id", "NSPrivacyCollectedDataTypeUserID"},
	{"device_id", "NSPrivacyCollectedDataTypeDeviceID"},
	{"purchase_history", "NSPrivacyCollectedDataTypePurchaseHistory"},
	{"product_interaction", "NSPrivacyCollectedDataTypeProductInteraction"},
	{"advertising_data", "NSPrivacyCollectedDataTypeAdvertisingData"},
	{"other_usage_data", "NSPrivacyCollectedDataTypeOtherUsageData"},
	{"crash_data", "NSPrivacyCollectedDataTypeCrashData"},
	{"performance_data", "NSPrivacyCollectedDataTypePerformanceData"},
	{"other_diagnostic_data", "NSPrivacyCollectedDataTypeOtherDiagnosticData"},
	{"environment_scanning", "NSPrivacyCollectedDataTypeEnvironmentScanning"},
	{"hands", "NSPrivacyCollectedDataTypeHands"},
	{"head", "NSPrivacyCollectedDataTypeHead"},
	{"other_data_types", "NSPrivacyCollectedDataTypeOtherDataTypes"},
};

static const DataCollectionInfo data_collect_purpose_info[] = {
	{"Analytics", "NSPrivacyCollectedDataTypePurposeAnalytics"},
	{"App Functionality", "NSPrivacyCollectedDataTypePurposeAppFunctionality"},
	{"Developer Advertising", "NSPrivacyCollectedDataTypePurposeDeveloperAdvertising"},
	{"Third-party Advertising", "NSPrivacyCollectedDataTypePurposeThirdPartyAdvertising"},
	{"Product Personalization", "NSPrivacyCollectedDataTypePurposeProductPersonalization"},
	{"Other", "NSPrivacyCollectedDataTypePurposeOther"},
};

static const String export_method_string[] = {
	"app-store",
	"development",
	"ad-hoc",
	"enterprise",
};

void EditorExportPlatformAppleEmbedded::_notification(int p_what)
{
}

String EditorExportPlatformAppleEmbedded::_get_additional_plist_content()
{
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_export_plugins();
	String result;
	for (int i = 0; i < export_plugins.size(); ++i) {
		result += export_plugins[i]->get_apple_embedded_platform_plist_content();
	}
	return result;
}

String EditorExportPlatformAppleEmbedded::_get_linker_flags()
{
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_export_plugins();
	String result;
	for (int i = 0; i < export_plugins.size(); ++i) {
		String flags = export_plugins[i]->get_apple_embedded_platform_linker_flags();
		if (flags.length() == 0) {
			continue;
		}
		if (result.length() > 0) {
			result += ' ';
		}
		result += flags;
	}
	// the flags will be enclosed in quotes, so need to escape them
	return result.replace("\"", "\\\"");
}

String EditorExportPlatformAppleEmbedded::_get_cpp_code()
{
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_export_plugins();
	String result;
	for (int i = 0; i < export_plugins.size(); ++i) {
		result += export_plugins[i]->get_apple_embedded_platform_cpp_code();
	}
	return result;
}

void EditorExportPlatformAppleEmbedded::_blend_and_rotate(
	Ref<Image>& p_dst, Ref<Image>& p_src, bool p_rot)
{
	ERR_FAIL_COND(p_dst.is_null());
	ERR_FAIL_COND(p_src.is_null());

	int sw = p_rot ? p_src->get_height() : p_src->get_width();
	int sh = p_rot ? p_src->get_width() : p_src->get_height();

	int x_pos = (p_dst->get_width() - sw) / 2;
	int y_pos = (p_dst->get_height() - sh) / 2;

	int xs = (x_pos >= 0) ? 0 : -x_pos;
	int ys = (y_pos >= 0) ? 0 : -y_pos;

	if (sw + x_pos > p_dst->get_width()) {
		sw = p_dst->get_width() - x_pos;
	}
	if (sh + y_pos > p_dst->get_height()) {
		sh = p_dst->get_height() - y_pos;
	}

	for (int y = ys; y < sh; y++) {
		for (int x = xs; x < sw; x++) {
			Color sc =
				p_rot ? p_src->get_pixel(p_src->get_width() - y - 1, x) : p_src->get_pixel(x, y);
			Color dc = p_dst->get_pixel(x_pos + x, y_pos + y);
			dc.r = (double)(sc.a * sc.r + dc.a * (1.0 - sc.a) * dc.r);
			dc.g = (double)(sc.a * sc.g + dc.a * (1.0 - sc.a) * dc.g);
			dc.b = (double)(sc.a * sc.b + dc.a * (1.0 - sc.a) * dc.b);
			dc.a = (double)(sc.a + dc.a * (1.0 - sc.a));
			p_dst->set_pixel(x_pos + x, y_pos + y, dc);
		}
	}
}

Error EditorExportPlatformAppleEmbedded::_walk_dir_recursive(
	Ref<DirAccess>& p_da, FileHandler p_handler, void* p_userdata)
{
	Vector<String> dirs;
	String current_dir = p_da->get_current_dir();
	p_da->list_dir_begin();
	String path = p_da->get_next();
	while (!path.is_empty()) {
		if (p_da->current_is_dir()) {
			if (path != "." && path != "..") {
				dirs.push_back(path);
			}
		}
		else {
			Error err = p_handler(current_dir.path_join(path), p_userdata);
			if (err) {
				p_da->list_dir_end();
				return err;
			}
		}
		path = p_da->get_next();
	}
	p_da->list_dir_end();

	for (int i = 0; i < dirs.size(); ++i) {
		p_da->change_dir(dirs[i]);
		Error err = _walk_dir_recursive(p_da, p_handler, p_userdata);
		p_da->change_dir("..");
		if (err) {
			return err;
		}
	}

	return OK;
}

struct CodesignData
{
	const Ref<EditorExportPreset>& preset;
	bool debug = false;

	CodesignData(const Ref<EditorExportPreset>& p_preset, bool p_debug)
		: preset(p_preset), debug(p_debug)
	{
	}
};

struct PbxId
{
private:
	static char _hex_char(uint8_t p_four_bits)
	{
		if (p_four_bits < 10) {
			return ('0' + p_four_bits);
		}
		return 'A' + (p_four_bits - 10);
	}

	static String _hex_pad(uint32_t p_num)
	{
		Vector<char> ret;
		ret.resize(sizeof(p_num) * 2);
		for (uint64_t i = 0; i < sizeof(p_num) * 2; ++i) {
			uint8_t four_bits = (p_num >> (sizeof(p_num) * 8 - (i + 1) * 4)) & 0xF;
			ret.write[i] = _hex_char(four_bits);
		}
		return String::utf8(ret.ptr(), ret.size());
	}

public:
	uint32_t high_bits;
	uint32_t mid_bits;
	uint32_t low_bits;

	String str() const { return _hex_pad(high_bits) + _hex_pad(mid_bits) + _hex_pad(low_bits); }

	PbxId& operator++()
	{
		low_bits++;
		if (!low_bits) {
			mid_bits++;
			if (!mid_bits) {
				high_bits++;
			}
		}

		return *this;
	}
};

struct ExportLibsData
{
	Vector<String> lib_paths;
	String dest_dir;
};

Error EditorExportPlatformAppleEmbedded::_export_additional_assets(
	const Ref<EditorExportPreset>& p_preset, const String& p_out_dir,
	const Vector<String>& p_assets, bool p_is_framework, bool p_should_embed,
	Vector<AppleEmbeddedExportAsset>& r_exported_assets)
{
	for (int f_idx = 0; f_idx < p_assets.size(); ++f_idx) {
		const String& asset = p_assets[f_idx];
		if (asset.begins_with("res://")) {
			Error err = _copy_asset(p_preset, p_out_dir, asset, nullptr, p_is_framework,
				p_should_embed, r_exported_assets);
			ERR_FAIL_COND_V(err != OK, err);
		}
		else if (asset.is_absolute_path() &&
				   ProjectSettings::localize_path(asset).begins_with("res://")) {
			Error err = _copy_asset(p_preset, p_out_dir,
				ProjectSettings::localize_path(asset), nullptr, p_is_framework,
				p_should_embed, r_exported_assets);
			ERR_FAIL_COND_V(err != OK, err);
		}
		else {
			// either SDK-builtin or already a part of the export template
			AppleEmbeddedExportAsset exported_asset = {asset, p_is_framework, p_should_embed};
			r_exported_assets.push_back(exported_asset);
		}
	}

	return OK;
}

Error EditorExportPlatformAppleEmbedded::_export_additional_assets(
	const Ref<EditorExportPreset>& p_preset, const String& p_out_dir,
	const Vector<SharedObject>& p_libraries, Vector<AppleEmbeddedExportAsset>& r_exported_assets)
{
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_export_plugins();
	for (int i = 0; i < export_plugins.size(); i++) {
		Vector<String> linked_frameworks =
			export_plugins[i]->get_apple_embedded_platform_frameworks();
		Error err = _export_additional_assets(
			p_preset, p_out_dir, linked_frameworks, true, false, r_exported_assets);
		ERR_FAIL_COND_V(err, err);

		Vector<String> embedded_frameworks =
			export_plugins[i]->get_apple_embedded_platform_embedded_frameworks();
		err = _export_additional_assets(
			p_preset, p_out_dir, embedded_frameworks, true, true, r_exported_assets);
		ERR_FAIL_COND_V(err, err);

		Vector<String> project_static_libs =
			export_plugins[i]->get_apple_embedded_platform_project_static_libs();
		for (int j = 0; j < project_static_libs.size(); j++) {
			project_static_libs.write[j] =
				project_static_libs[j]
					.get_file(); // Only the file name as it's copied to the project
		}
		err = _export_additional_assets(
			p_preset, p_out_dir, project_static_libs, true, false, r_exported_assets);
		ERR_FAIL_COND_V(err, err);

		Vector<String> apple_embedded_platform_bundle_files =
			export_plugins[i]->get_apple_embedded_platform_bundle_files();
		err = _export_additional_assets(p_preset, p_out_dir, apple_embedded_platform_bundle_files,
			false, false, r_exported_assets);
		ERR_FAIL_COND_V(err, err);
	}

	Vector<String> library_paths;
	for (int i = 0; i < p_libraries.size(); ++i) {
		library_paths.push_back(p_libraries[i].path);
	}
	Error err = _export_additional_assets(
		p_preset, p_out_dir, library_paths, true, true, r_exported_assets);
	ERR_FAIL_COND_V(err, err);

	return OK;
}

Error EditorExportPlatformAppleEmbedded::export_project(const Ref<EditorExportPreset>& p_preset,
	bool p_debug, const String& p_path, uint32_t p_flags, bool p_notify)
{
	return _export_project_helper(p_preset, p_debug, p_path, p_flags, p_notify, false);
}

int EditorExportPlatformAppleEmbedded::get_options_count() const
{
	MutexLock lock(device_lock);
	return devices.size();
}

String EditorExportPlatformAppleEmbedded::get_options_tooltip() const
{
	return TTR("Select device from the list");
}

String EditorExportPlatformAppleEmbedded::get_option_label(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, devices.size(), "");
	MutexLock lock(device_lock);
	return devices[p_index].name;
}

String EditorExportPlatformAppleEmbedded::get_option_tooltip(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, devices.size(), "");
	MutexLock lock(device_lock);
	return "UUID: " + devices[p_index].id;
}

bool EditorExportPlatformAppleEmbedded::is_package_name_valid(
	const String& p_package, String* r_error) const
{
	String pname = p_package;

	if (pname.length() == 0) {
		if (r_error) {
			*r_error = TTR("Identifier is missing.");
		}
		return false;
	}

	for (int i = 0; i < pname.length(); i++) {
		char32_t c = pname[i];
		if (!(is_ascii_alphanumeric_char(c) || c == '-' || c == '.')) {
			if (r_error) {
				*r_error = vformat(
					TTR("The character '%s' is not allowed in Identifier."), String::chr(c));
			}
			return false;
		}
	}

	return true;
}

#ifdef MACOS_ENABLED
bool EditorExportPlatformAppleEmbedded::_check_xcode_install()
{
	static bool xcode_found = false;
	if (!xcode_found) {
		Vector<String> mdfind_paths;
		List<String> mdfind_args;
		mdfind_args.push_back("kMDItemCFBundleIdentifier=com.apple.dt.Xcode");

		String output;
		Error err = OS::execute("mdfind", mdfind_args, &output);
		if (err == OK) {
			mdfind_paths = output.split("\n");
		}
		for (const String& found_path : mdfind_paths) {
			xcode_found =
				!found_path.is_empty() && DirAccess::dir_exists_absolute(found_path.strip_edges());
			if (xcode_found) {
				break;
			}
		}
	}
	return xcode_found;
}

void EditorExportPlatformAppleEmbedded::_check_for_changes_poll_thread(void* ud)
{
	EditorExportPlatformAppleEmbedded* ea = static_cast<EditorExportPlatformAppleEmbedded*>(ud);

	String device_types;
	bool first = true;
	for (const String& d : ea->get_device_types()) {
		if (first) {
			first = false;
		}
		else {
			device_types += "|";
		}
		device_types += d;
	}

	while (!ea->quit_request.is_set()) {
		// Nothing to do if we already know the plugins have changed.
		if (!ea->plugins_changed.is_set()) {
			MutexLock lock(ea->plugins_lock);

			Vector<PluginConfigAppleEmbedded> loaded_plugins = get_plugins(ea->get_platform_name());

			if (ea->plugins.size() != loaded_plugins.size()) {
				ea->plugins_changed.set();
			}
			else {
				for (int i = 0; i < ea->plugins.size(); i++) {
					if (ea->plugins[i].name != loaded_plugins[i].name ||
						ea->plugins[i].last_updated != loaded_plugins[i].last_updated) {
						ea->plugins_changed.set();
						break;
					}
				}
			}
		}

		// Check for devices updates.
		Vector<Device> ldevices;

		// Enum real devices (via ios_deploy, pre Xcode 15).
		String ios_deploy_setting = "export/" + ea->get_platform_name() + "/ios_deploy";
		if (EditorSettings::is_initialized() &&
			EditorSettings::has_setting(ios_deploy_setting)) {
			String idepl = EDITOR_GET(ios_deploy_setting);
			if (ea->has_runnable_preset.is_set() && !idepl.is_empty()) {
				String devices_json;
				List<String> args;
				args.push_back("-c");
				args.push_back("-timeout");
				args.push_back("1");
				args.push_back("-j");
				args.push_back("-u");
				args.push_back("-I");

				int ec = 0;
				Error err = OS::execute(idepl, args, &devices_json, &ec, true);
				if (err == OK && ec == 0) {
					Ref<JSON> json;
					json.instantiate();
					devices_json = "{ \"devices\":[" + devices_json.replace("}{", "},{") + "]}";
					err = json->parse(devices_json);
					if (err == OK) {
						Dictionary data = json->get_data();
						Array devices = data["devices"];
						for (int i = 0; i < devices.size(); i++) {
							Dictionary device_event = devices[i];
							if (device_event["Event"] == "DeviceDetected") {
								Dictionary device_info = device_event["Device"];
								Device nd;
								nd.id = device_info["DeviceIdentifier"];
								nd.name =
									device_info["DeviceName"].operator String() + " (ios_deploy, " +
									((device_event["Interface"] == "WIFI") ? "network" : "wired") +
									")";
								nd.wifi = device_event["Interface"] == "WIFI";
								nd.use_ios_deploy = true;
								ldevices.push_back(nd);
							}
						}
					}
				}
			}
		}
		// Enum devices (via Xcode).
		if (ea->has_runnable_preset.is_set() && _check_xcode_install() &&
			(FileAccess::exists("/usr/bin/xcrun") || FileAccess::exists("/bin/xcrun"))) {
			String devices_json;
			List<String> args;
			args.push_back("devicectl");
			args.push_back("list");
			args.push_back("devices");
			args.push_back("-j");
			args.push_back("-");
			args.push_back("-q");
			// Add a timeout, so the process doesn't hang indefinitely, which can prevent Godot
			// shutting down.
			args.push_back("--timeout");
			args.push_back("5");
			args.push_back("--filter");
			args.push_back(vformat("hardwareProperties.deviceType MATCHES '%s'", device_types));

			int ec = 0;
			Error err = OS::execute("xcrun", args, &devices_json, &ec, false);
			if (err == OK && ec == 0) {
				Ref<JSON> json;
				json.instantiate();
				err = json->parse(devices_json);
				if (err == OK) {
					const Dictionary& data = json->get_data();
					const Dictionary& result = data["result"];
					const Array& devices = result["devices"];
					for (int i = 0; i < devices.size(); i++) {
						const Dictionary& device_info = devices[i];
						const Dictionary& conn_props = device_info["connectionProperties"];
						const Dictionary& dev_props = device_info["deviceProperties"];
						if (dev_props.has("developerModeStatus") &&
							conn_props.has("pairingState") && conn_props.has("transportType") &&
							conn_props["pairingState"] == "paired" &&
							dev_props["developerModeStatus"] == "enabled") {
							Device nd;
							nd.id = device_info["identifier"];
							nd.name = dev_props["name"].operator String() + " (devicectl, " +
									  ((conn_props["transportType"] == "localNetwork") ? "network"
																					   : "wired") +
									  ")";
							nd.wifi = conn_props["transportType"] == "localNetwork";
							ldevices.push_back(nd);
						}
					}
				}
			}
		}

		// Update device list.
		{
			MutexLock lock(ea->device_lock);

			bool different = false;

			if (ea->devices.size() != ldevices.size()) {
				different = true;
			}
			else {
				for (int i = 0; i < ea->devices.size(); i++) {
					if (ea->devices[i].id != ldevices[i].id) {
						different = true;
						break;
					}
				}
			}

			if (different) {
				ea->devices = ldevices;
				ea->devices_changed.set();
			}
		}

		uint64_t sleep = 200;
		uint64_t wait = 3000000;
		uint64_t time = OS::get_ticks_usec();
		while (OS::get_ticks_usec() - time < wait) {
			OS::delay_usec(1000 * sleep);
			if (ea->quit_request.is_set()) {
				break;
			}
		}
	}
}

void EditorExportPlatformAppleEmbedded::_update_preset_status()
{
	bool has_runnable =
		EditorExport::get_runnable_preset_for_platform(this).is_valid();
	if (has_runnable) {
		has_runnable_preset.set();
	}
	else {
		has_runnable_preset.clear();
	}
	devices_changed.set();
}

class FileReader
{
	Ref<FileAccess> f;
	LocalVector<char> buf;

	void append_span(Span<char> p_span, String& p_data)
	{
		uint32_t old_size = p_data.size();
		if (p_data.append_utf8(p_span) != OK) {
			p_data.resize_uninitialized(old_size); // Back up to original size.
			if (old_size > 0) {
				p_data[old_size - 1] = '\0';
			}
			p_data.append_latin1(p_span);
		}
	}

public:
	uint32_t get_lines(String& p_data)
	{
		uint64_t available = f->get_length() - f->get_position();
		if (available == 0) {
			return 0;
		}

		uint32_t start = buf.size();
		buf.resize_uninitialized(buf.size() + available);
		f->get_buffer((uint8_t*)buf.ptr() + start, available);
		const char* end = &buf[buf.size() - 1];
		const char* p = end;
		uint32_t n = available;
		bool found = false;
		// Scan for a newline starting from the end of the appended bytes.
		while (n--) {
			if (*p == '\n') {
				found = true;
				break;
			}
			p--;
		}
		if (found) {
			size_t len = static_cast<size_t>(p - buf.ptr()) + 1;
			Span<char> new_data(buf.ptr(), len);
			append_span(new_data, p_data);
			size_t remain = static_cast<size_t>(end - p);
			// If there is unprocessed data in the buffer, move it to the front.
			if (remain > 0) {
				// Move to next char after '\n'.
				p++;
				memmove(buf.ptr(), p, remain);
			}
			buf.resize_uninitialized(remain);
		}
		return available;
	}

	// Flush any remaining data.
	uint32_t flush(String& p_data)
	{
		Span<char> new_data = buf.span();
		if (new_data.size() > 0) {
			append_span(new_data, p_data);
		}
		return new_data.size();
	}

	FileReader(Ref<FileAccess> p_f) : f(p_f) {}
};

int EditorExportPlatformAppleEmbedded::_execute(const String& p_path,
	const List<String>& p_arguments, std::function<void(const String&)> p_on_data)
{
	Dictionary pipe_info = OS::execute_with_pipe(p_path, p_arguments, false);
	ERR_FAIL_COND_V_MSG(pipe_info.is_empty(), 1, "execute_with_pipe failed");

	Ref<FileAccess> fa_stdout = pipe_info["stdio"];
	Ref<FileAccess> fa_stderr = pipe_info["stderr"];
	ProcessID pid = pipe_info["pid"];

	FileReader stdout_r(fa_stdout);
	FileReader stderr_r(fa_stderr);

	while (true) {
		String output;
		stdout_r.get_lines(output);
		stderr_r.get_lines(output);
		if (!output.is_empty()) {
			p_on_data(output);
		}

		// If the process is no longer running and no new data arrived, we're done.
		if (output.is_empty() && !OS::is_process_running(pid)) {
			break;
		}

		OS::delay_usec(1000);
	}

	// Flush any remaining content
	String output;
	stdout_r.flush(output);
	stderr_r.flush(output);
	if (!output.is_empty()) {
		p_on_data(output);
	}

	fa_stdout->close();
	fa_stderr->close();

	return OS::get_process_exit_code(pid);
}

#endif

Error EditorExportPlatformAppleEmbedded::run(
	const Ref<EditorExportPreset>& p_preset, int p_device, uint32_t p_debug_flags)
{
#ifdef MACOS_ENABLED
	ERR_FAIL_INDEX_V(p_device, devices.size(), ERR_INVALID_PARAMETER);

	String can_export_error;
	bool can_export_missing_templates;
	if (!can_export(p_preset, can_export_error, can_export_missing_templates)) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Run"), can_export_error);
		return ERR_UNCONFIGURED;
	}

	MutexLock lock(device_lock);

	String id = "tmpexport." + uitos(OS::get_unix_time());

	Ref<DirAccess> filesystem_da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND_V_MSG(filesystem_da.is_null(), ERR_CANT_CREATE,
		"Cannot create DirAccess for path '" + EditorPaths::get_temp_dir() + "'.");
	filesystem_da->make_dir_recursive(EditorPaths::get_temp_dir().path_join(id));
	String tmp_export_path =
		EditorPaths::get_temp_dir().path_join(id).path_join("export.ipa");

#define CLEANUP_AND_RETURN(m_err)                                                                  \
	{                                                                                              \
		if (filesystem_da->change_dir(                                                             \
				EditorPaths::get_temp_dir().path_join(id)) == OK) {               \
			filesystem_da->erase_contents_recursive();                                             \
			filesystem_da->change_dir("..");                                                       \
			filesystem_da->remove(id);                                                             \
		}                                                                                          \
		return m_err;                                                                              \
	}                                                                                              \
	((void)0)

	Device dev = devices[p_device];

	// Export before sending to device.
	Error err = _export_project_helper(p_preset, true, tmp_export_path, p_debug_flags, true, true);

	if (err != OK) {
		CLEANUP_AND_RETURN(err);
	}

	Vector<String> cmd_args_list;
	String host = EDITOR_GET("network/debug/remote_host");
	int remote_port = (int)EDITOR_GET("network/debug/remote_port");

	if (p_debug_flags.has_flag(DEBUG_FLAG_REMOTE_DEBUG_LOCALHOST)) {
		host = "localhost";
	}

	if (p_debug_flags.has_flag(DEBUG_FLAG_DUMB_CLIENT)) {
		int port = EDITOR_GET("filesystem/file_server/port");
		String passwd = EDITOR_GET("filesystem/file_server/password");
		cmd_args_list.push_back("--remote-fs");
		cmd_args_list.push_back(host + ":" + itos(port));
		if (!passwd.is_empty()) {
			cmd_args_list.push_back("--remote-fs-password");
			cmd_args_list.push_back(passwd);
		}
	}

	if (p_debug_flags.has_flag(DEBUG_FLAG_REMOTE_DEBUG)) {
		cmd_args_list.push_back("--remote-debug");
		cmd_args_list.push_back(get_debug_protocol() + host + ":" + String::num_int64(remote_port));
	}

	if (p_debug_flags.has_flag(DEBUG_FLAG_VIEW_COLLISIONS)) {
		cmd_args_list.push_back("--debug-collisions");
	}

	if (p_debug_flags.has_flag(DEBUG_FLAG_VIEW_NAVIGATION)) {
		cmd_args_list.push_back("--debug-navigation");
	}

	if (dev.use_ios_deploy) {
		// Deploy and run on real device (via ios-deploy).
		{
			List<String> args;
			args.push_back("-u");
			args.push_back("-I");
			args.push_back("--id");
			args.push_back(dev.id);
			args.push_back("--justlaunch");
			args.push_back("--bundle");
			args.push_back(EditorPaths::get_temp_dir().path_join(id).path_join(
				"export.xcarchive/Products/Applications/export.app"));
			String app_args;
			for (const String& E : cmd_args_list) {
				app_args += E + " ";
			}
			if (!app_args.is_empty()) {
				args.push_back("--args");
				args.push_back(app_args);
			}

			String idepl = EDITOR_GET("export/" + get_platform_name() + "/ios_deploy");
			if (idepl.is_empty()) {
				idepl = "ios-deploy";
			}
			String log;
			int ec;
			err = OS::execute(idepl, args, &log, &ec, true);
			if (err != OK) {
				add_message(EXPORT_MESSAGE_WARNING, TTR("Run"),
					TTR("Could not start ios-deploy executable."));
				CLEANUP_AND_RETURN(err);
			}
			if (ec != 0) {
				print_line("ios-deploy:\n" + log);
				add_message(EXPORT_MESSAGE_ERROR, TTR("Run"),
					TTR("Installation/running failed, see editor log for details."));
				CLEANUP_AND_RETURN(ERR_UNCONFIGURED);
			}
		}
	}
	else {
		// Deploy and run on real device (via Xcode).
		{
			List<String> args;
			args.push_back("devicectl");
			args.push_back("device");
			args.push_back("install");
			args.push_back("app");
			args.push_back("-d");
			args.push_back(dev.id);
			args.push_back(EditorPaths::get_temp_dir().path_join(id).path_join(
				"export.xcarchive/Products/Applications/export.app"));

			String log;
			int ec = _execute(
				"xcrun", args, [&log](const String& p_data) { log.append_utf32(p_data.span()); });
			if (ec != 0) {
				print_line("device install:\n" + log);
				add_message(EXPORT_MESSAGE_ERROR, TTR("Run"),
					TTR("Installation failed, see editor log for details."));
				CLEANUP_AND_RETURN(ERR_UNCONFIGURED);
			}
		}

		{
			List<String> args;
			args.push_back("devicectl");
			args.push_back("device");
			args.push_back("process");
			args.push_back("launch");
			args.push_back("--terminate-existing");
			args.push_back("-d");
			args.push_back(dev.id);
			args.push_back(p_preset->get("application/bundle_identifier"));
			for (const String& E : cmd_args_list) {
				args.push_back(E);
			}

			String log;
			int ec = _execute(
				"xcrun", args, [&log](const String& p_data) { log.append_utf32(p_data.span()); });
			if (ec != 0) {
				print_line("devicectl launch:\n" + log);
				add_message(EXPORT_MESSAGE_ERR OR, TTR("Run"),
					TTR("Running failed, see editor log for details."));
			}
		}
	}

	CLEANUP_AND_RETURN(OK);

#undef CLEANUP_AND_RETURN
#else
	return ERR_UNCONFIGURED;
#endif
}

void EditorExportPlatformAppleEmbedded::_initialize(
	const char* p_platform_logo_svg, const char* p_run_icon_svg)
{
	Ref<Image> img = memnew(Image);
	const bool upsample = !Math::is_equal_approx(Math::round(EDSCALE), EDSCALE);

	ImageLoaderSVG::create_image_from_string(img, p_platform_logo_svg, EDSCALE, upsample, false);
	logo = ImageTexture::create_from_image(img);

	ImageLoaderSVG::create_image_from_string(img, p_run_icon_svg, EDSCALE, upsample, false);
	run_icon = ImageTexture::create_from_image(img);

	plugins_changed.set();


	devices_changed.set();
#ifdef MACOS_ENABLED
	_update_preset_status();
#endif
}

EditorExportPlatformAppleEmbedded::~EditorExportPlatformAppleEmbedded() {}

Error EditorExportPlatformAppleEmbedded::_copy_asset(Ref<EditorExportPreset> const&, String const&,
	String const&, String const*, bool, bool,
	Vector<EditorExportPlatformAppleEmbedded::AppleEmbeddedExportAsset>&)
{
	return OK;
}

String EditorExportPlatformAppleEmbedded::_process_config_file_line(Ref<EditorExportPreset> const&,
	String const&, EditorExportPlatformAppleEmbedded::AppleEmbeddedConfigData const&, bool,
	EditorExportPlatformAppleEmbedded::CodeSigningDetails const&)
{
	return String();
}

Error EditorExportPlatformAppleEmbedded::_export_project_helper(
	Ref<EditorExportPreset> const&, bool, String const&, unsigned int, bool, bool)
{
	return OK;
}

bool EditorExportPlatformAppleEmbedded::has_valid_project_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error) const
{
	return false;
}

void EditorExportPlatformAppleEmbedded::get_preset_features(
	const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const
{
}

String EditorExportPlatformAppleEmbedded::get_export_option_warning(
	const EditorExportPreset* p_preset, const String& p_name) const
{
	return String();
}

bool EditorExportPlatformAppleEmbedded::get_export_option_visibility(
	const EditorExportPreset* p_preset, const String& p_option) const
{
	return false;
}

bool EditorExportPlatformAppleEmbedded::has_valid_export_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error, bool& r_missing_templates,
	bool p_debug) const
{
	return false;
}

void EditorExportPlatformAppleEmbedded::get_export_options(
	List<EditorExportPlatform::ExportOption>* r_options) const
{
}

#ifdef CLEANUP_AND_RETURN
#undef CLEANUP_AND_RETURN
#endif
