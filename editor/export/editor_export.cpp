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

#include "editor/export/editor_export_platform.h"

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access.h"
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
#include "core/io/file_access.h"
#include "core/math/random_pcg.h"
#include "core/os/os.h"
#include "editor/export/editor_export_platform.h"
#include "core/string/translation.h"
#include "core/version.h"
#include "editor/export/editor_export_platform.h"
#include "editor/export/editor_export_platform.h"
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
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/export/editor_export_platform.h"
#include "editor/export/editor_export_platform.h"
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


// --- Merged from editor/import/3d/resource_importer_scene.cpp ---
#include "core/error/error_macros.h"
#include "core/io/dir_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "editor/settings/editor_settings.h"
#include "editor/import/3d/resource_importer_scene.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/shape_3d.h"
#include "scene/resources/animation.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/physics_material.h"
#include "scene/resources/packed_scene.h"

void EditorSceneFormatImporter::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

String EditorScenePostImport::get_source_file() const { return source_file; }

void EditorScenePostImport::init(const String& p_source_file) { source_file = p_source_file; }

void EditorScenePostImportPlugin::get_internal_import_options(
	InternalImportCategory p_category, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

void EditorScenePostImportPlugin::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	current_option_list = r_options;
	current_option_list = nullptr;
}

const String ResourceImporterScene::material_extension[3] = {".tres", ".res", ".material"};

String ResourceImporterScene::get_importer_name() const
{
	// For compatibility with 4.2 and earlier we need to keep the "scene" and "animation_library"
	// names. However this is arbitrary so for new import types we can use any string.
	if (_scene_import_type == "PackedScene") {
		return "scene";
	}
	else if (_scene_import_type == "AnimationLibrary") {
		return "animation_library";
	}
	return _scene_import_type;
}

String ResourceImporterScene::get_visible_name() const
{
	// This is displayed on the UI. Friendly names here are nice but not vital, so fall back to the
	// type.
	if (_scene_import_type == "PackedScene") {
		return "Scene";
	}
	else if (_scene_import_type == "ArrayMesh") {
		return "Single Mesh";
	}
	return _scene_import_type.capitalize();
}

void ResourceImporterScene::get_recognized_extensions(List<String>* p_extensions) const
{
	get_scene_importer_extensions(p_extensions);
}

String ResourceImporterScene::get_save_extension() const
{
	if (_scene_import_type == "PackedScene") {
		return "scn";
	}
	return "res";
}

String ResourceImporterScene::get_resource_type() const { return _scene_import_type; }

int ResourceImporterScene::get_format_version() const { return 1; }

int ResourceImporterScene::get_preset_count() const { return 0; }

String ResourceImporterScene::get_preset_name(int p_idx) const { return String(); }

static bool _teststr(const String& p_what, const String& p_str)
{
	String what = p_what;

	// Remove trailing spaces and numbers, some apps like blender add ".number" to duplicates
	// (dot is replaced with _ as invalid character) so also compensate for this.
	while (what.length() && (is_digit(what[what.length() - 1]) || what[what.length() - 1] <= 32 ||
								what[what.length() - 1] == '_')) {
		what = what.substr(0, what.length() - 1);
	}

	if (what.containsn("$" + p_str)) { // Blender and other stuff.
		return true;
	}
	if (what.to_lower().ends_with(
			"-" + p_str)) { // collada only supports "_" and "-" besides letters
		return true;
	}
	if (what.to_lower().ends_with(
			"_" + p_str)) { // collada only supports "_" and "-" besides letters
		return true;
	}
	return false;
}

static String _fixstr(const String& p_what, const String& p_str)
{
	String what = p_what;

	// Remove trailing spaces and numbers, some apps like blender add ".number" to duplicates
	// (dot is replaced with _ as invalid character) so also compensate for this.
	while (what.length() && (is_digit(what[what.length() - 1]) || what[what.length() - 1] <= 32 ||
								what[what.length() - 1] == '_')) {
		what = what.substr(0, what.length() - 1);
	}

	String end = p_what.substr(what.length());

	if (what.containsn("$" + p_str)) { // Blender and other stuff.
		return what.replace("$" + p_str, "") + end;
	}
	if (what.to_lower().ends_with(
			"-" + p_str)) { // collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	if (what.to_lower().ends_with(
			"_" + p_str)) { // collada only supports "_" and "-" besides letters
		return what.substr(0, what.length() - (p_str.length() + 1)) + end;
	}
	return what;
}

static void _pre_gen_shape_list(
	Ref<ImporterMesh>& mesh, Vector<Ref<Shape3D>>& r_shape_list, bool p_convex)
{
	ERR_FAIL_COND_MSG(mesh.is_null(), "Cannot generate shape list with null mesh value.");
	if (!p_convex) {
		Ref<Shape3D> shape = mesh->create_trimesh_shape();
		r_shape_list.push_back(shape);
	}
	else {
		Vector<Ref<Shape3D>> cd;
		cd.push_back(mesh->create_convex_shape(true,
			/*Passing false, otherwise VHACD will be used to simplify (Decompose) the Mesh.*/
			false));
		if (cd.size()) {
			for (int i = 0; i < cd.size(); i++) {
				r_shape_list.push_back(cd[i]);
			}
		}
	}
}

struct ScalableNodeCollection
{
	HashSet<Node3D*> node_3ds;
	HashSet<Ref<ImporterMesh>> importer_meshes;
	HashSet<Ref<Skin>> skins;
	HashSet<Ref<Animation>> animations;
};

void _rescale_skin(Vector3 p_scale, Ref<Skin> p_skin)
{
	// MESH and SKIN data divide, to compensate for object position multiplying.
	for (int i = 0; i < p_skin->get_bind_count(); i++) {
		Transform3D transform = p_skin->get_bind_pose(i);
		p_skin->set_bind_pose(i, Transform3D(transform.basis, p_scale * transform.origin));
	}
}

void ResourceImporterScene::_optimize_animations(
	AnimationPlayer* anim, float p_max_vel_error, float p_max_ang_error, int p_prc_error)
{
	LocalVector<String> anim_names;
	anim->get_animation_list(&anim_names);
	for (const String& E : anim_names) {
		Ref<Animation> a = anim->get_animation(E);
		a->optimize(p_max_vel_error, p_max_ang_error, p_prc_error);
	}
}

void ResourceImporterScene::_compress_animations(AnimationPlayer* anim, int p_page_size_kb)
{
	LocalVector<String> anim_names;
	anim->get_animation_list(&anim_names);
	for (const String& E : anim_names) {
		Ref<Animation> a = anim->get_animation(E);
		a->compress(p_page_size_kb * 1024);
	}
}

void ResourceImporterScene::_replace_owner(Node* p_node, Node* p_scene, Node* p_new_owner)
{
	if (p_node != p_new_owner && p_node->get_owner() == p_scene) {
		p_node->set_owner(p_new_owner);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* n = p_node->get_child(i);
		_replace_owner(n, p_scene, p_new_owner);
	}
}

void ResourceImporterScene::_add_shapes(Node* p_node, const Vector<Ref<Shape3D>>& p_shapes)
{
	for (const Ref<Shape3D>& E : p_shapes) {
		CollisionShape3D* cshape = memnew(CollisionShape3D);
		cshape->set_shape(E);
		p_node->add_child(cshape, true);

		cshape->set_owner(p_node->get_owner());
	}
}

Vector<Ref<EditorSceneFormatImporter>> ResourceImporterScene::scene_importers;
Vector<Ref<EditorScenePostImportPlugin>> ResourceImporterScene::post_importer_plugins;

bool ResourceImporterScene::has_advanced_options() const { return false; }

void ResourceImporterScene::show_advanced_options(const String& p_path) {}

ResourceImporterScene::ResourceImporterScene(const String& p_scene_import_type)
{
	_scene_import_type = p_scene_import_type;
}

void ResourceImporterScene::add_scene_importer(
	Ref<EditorSceneFormatImporter> p_importer, bool p_first_priority)
{
	ERR_FAIL_COND(p_importer.is_null());
	if (p_first_priority) {
		scene_importers.insert(0, p_importer);
	}
	else {
		scene_importers.push_back(p_importer);
	}
}

void ResourceImporterScene::remove_post_importer_plugin(
	const Ref<EditorScenePostImportPlugin>& p_plugin)
{
	post_importer_plugins.erase(p_plugin);
}

void ResourceImporterScene::add_post_importer_plugin(
	const Ref<EditorScenePostImportPlugin>& p_plugin, bool p_first_priority)
{
	ERR_FAIL_COND(p_plugin.is_null());
	if (p_first_priority) {
		post_importer_plugins.insert(0, p_plugin);
	}
	else {
		post_importer_plugins.push_back(p_plugin);
	}
}

void ResourceImporterScene::remove_scene_importer(Ref<EditorSceneFormatImporter> p_importer)
{
	scene_importers.erase(p_importer);
}

void ResourceImporterScene::clean_up_importer_plugins()
{
	scene_importers.clear();
	post_importer_plugins.clear();
}

void ResourceImporterScene::get_scene_importer_extensions(List<String>* p_extensions)
{
	for (Ref<EditorSceneFormatImporter> importer_elem : scene_importers) {
		importer_elem->get_extensions(p_extensions);
	}
}

void EditorSceneFormatImporterESCN::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("escn");
}

Node* EditorScenePostImport::post_import(Node* p_scene) { return p_scene; }

void ResourceImporterScene::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}

void EditorSceneFormatImporter::get_extensions(List<String>* r_extensions) const {}



// ============================================================================
// Merged from editor/import/3d/collada.cpp
// ============================================================================
// --- Inlined from editor/import/3d/collada.h ---
#include "core/io/xml_parser.h"

class Collada
{
public:
	enum ImportFlags
	{
		IMPORT_FLAG_SCENE = 1,
		IMPORT_FLAG_ANIMATION = 2
	};

	struct Image
	{
		String path;
	};

	struct Material
	{
		String name;
		String instance_effect;
	};

	struct Effect
	{
		String name;

		struct Channel
		{
			int uv_idx = 0;
			String texture;
			Color color;
		};

		Channel diffuse, specular, emission, bump;
		float shininess = 40;
		bool found_double_sided = false;
		bool double_sided = true;
		bool unshaded = false;

		String get_texture_path(const String& p_source, Collada& p_state) const;

		Effect() { diffuse.color = Color(1, 1, 1, 1); }
	};

	struct CameraData
	{
		enum Mode
		{
			MODE_PERSPECTIVE,
			MODE_ORTHOGONAL
		};

		Mode mode = MODE_PERSPECTIVE;

		union
		{
			struct
			{
				float x_fov = 0;
				float y_fov = 0;
			} perspective;

			struct
			{
				float x_mag = 0;
				float y_mag = 0;
			} orthogonal;
		};

		float aspect = 1;
		float z_near = 0.05;
		float z_far = 4000;

		CameraData() {}
	};

	struct LightData
	{
		enum Mode
		{
			MODE_AMBIENT,
			MODE_DIRECTIONAL,
			MODE_OMNI,
			MODE_SPOT
		};

		Mode mode = MODE_AMBIENT;

		Color color = Color(1, 1, 1, 1);

		float constant_att = 0;
		float linear_att = 0;
		float quad_att = 0;

		float spot_angle = 45;
		float spot_exp = 1;
	};

	struct MeshData
	{
		String name;

		struct Source
		{
			Vector<float> array;
			int stride = 0;
		};

		HashMap<String, Source> sources;

		struct Vertices
		{
			HashMap<String, String> sources;
		};

		HashMap<String, Vertices> vertices;

		struct Primitives
		{
			struct SourceRef
			{
				String source;
				int offset = 0;
			};

			String material;
			HashMap<String, SourceRef> sources;
			Vector<float> polygons;
			Vector<float> indices;
			int count = 0;
			int vertex_size = 0;
		};

		Vector<Primitives> primitives;

		bool found_double_sided = false;
		bool double_sided = true;
	};

	struct CurveData
	{
		String name;
		bool closed = false;

		struct Source
		{
			Vector<String> sarray;
			Vector<float> array;
			int stride = 0;
		};

		HashMap<String, Source> sources;

		HashMap<String, String> control_vertices;
	};

	struct SkinControllerData
	{
		String base;
		bool use_idrefs = false;

		Transform3D bind_shape;

		struct Source
		{
			Vector<String> sarray; // maybe for names
			Vector<float> array;
			int stride = 1;
		};

		HashMap<String, Source> sources;

		struct Joints
		{
			HashMap<String, String> sources;
		} joints;

		struct Weights
		{
			struct SourceRef
			{
				String source;
				int offset = 0;
			};

			String material;
			HashMap<String, SourceRef> sources;
			Vector<float> sets;
			Vector<float> indices;
			int count = 0;
		} weights;

		HashMap<String, Transform3D> bone_rest_map;
	};

	struct MorphControllerData
	{
		String mesh;
		String mode;

		struct Source
		{
			int stride = 1;
			Vector<String> sarray; // maybe for names
			Vector<float> array;
		};

		HashMap<String, Source> sources;

		HashMap<String, String> targets;
	};

	struct Vertex
	{
		int idx = 0;
		Vector3 vertex;
		Vector3 normal;
		Vector3 uv;
		Vector3 uv2;
		Plane tangent;
		Color color;
		int uid = 0;

		struct Weight
		{
			int bone_idx = 0;
			float weight = 0;

			bool operator<(const Weight w) const { return weight > w.weight; } // heaviest first
		};

		Vector<Weight> weights;

		void fix_weights()
		{
			weights.sort();
			if (weights.size() > 4) {
				// cap to 4 and make weights add up 1
				weights.resize(4);
				float total = 0;
				for (int i = 0; i < 4; i++) {
					total += weights[i].weight;
				}
				if (total) {
					for (int i = 0; i < 4; i++) {
						weights.write[i].weight /= total;
					}
				}
			}
		}

		void fix_unit_scale(const Collada& p_state);

		bool operator<(const Vertex& p_vert) const
		{
			if (uid == p_vert.uid) {
				if (vertex == p_vert.vertex) {
					if (normal == p_vert.normal) {
						if (uv == p_vert.uv) {
							if (uv2 == p_vert.uv2) {
								if (!weights.is_empty() || !p_vert.weights.is_empty()) {
									if (weights.size() == p_vert.weights.size()) {
										for (int i = 0; i < weights.size(); i++) {
											if (weights[i].bone_idx != p_vert.weights[i].bone_idx) {
												return weights[i].bone_idx <
													   p_vert.weights[i].bone_idx;
											}

											if (weights[i].weight != p_vert.weights[i].weight) {
												return weights[i].weight < p_vert.weights[i].weight;
											}
										}
									}
									else {
										return weights.size() < p_vert.weights.size();
									}
								}

								return (color < p_vert.color);
							}
							else {
								return (uv2 < p_vert.uv2);
							}
						}
						else {
							return (uv < p_vert.uv);
						}
					}
					else {
						return (normal < p_vert.normal);
					}
				}
				else {
					return vertex < p_vert.vertex;
				}
			}
			else {
				return uid < p_vert.uid;
			}
		}
	};

	struct Node
	{
		enum Type
		{
			TYPE_NODE,
			TYPE_JOINT,
			TYPE_SKELETON, // this bone is not collada, it's added afterwards as optimization
			TYPE_LIGHT,
			TYPE_CAMERA,
			TYPE_GEOMETRY
		};

		struct XForm
		{
			enum Op
			{
				OP_ROTATE,
				OP_SCALE,
				OP_TRANSLATE,
				OP_MATRIX,
				OP_VISIBILITY
			};

			String id;
			Op op = OP_ROTATE;
			Vector<float> data;
		};

		Type type = TYPE_NODE;

		String name;
		String id;
		String empty_draw_type;
		bool noname = false;
		Vector<XForm> xform_list;
		Transform3D default_transform;
		Transform3D post_transform;
		Vector<Node*> children;

		Node* parent = nullptr;

		Transform3D compute_transform(const Collada& p_state) const;
		Transform3D get_global_transform() const;
		Transform3D get_transform() const;

		bool ignore_anim = false;

		virtual ~Node()
		{
			for (int i = 0; i < children.size(); i++) {
				memdelete(children[i]);
			}
		}
	};

	struct NodeSkeleton : public Node
	{
		NodeSkeleton() { type = TYPE_SKELETON; }
	};

	struct NodeJoint : public Node
	{
		NodeSkeleton* owner = nullptr;
		String sid;

		NodeJoint() { type = TYPE_JOINT; }
	};

	struct NodeGeometry : public Node
	{
		bool controller = false;
		String source;

		struct Material
		{
			String target;
		};

		HashMap<String, Material> material_map;
		Vector<String> skeletons;

		NodeGeometry() { type = TYPE_GEOMETRY; }
	};

	struct NodeCamera : public Node
	{
		String camera;

		NodeCamera() { type = TYPE_CAMERA; }
	};

	struct NodeLight : public Node
	{
		String light;

		NodeLight() { type = TYPE_LIGHT; }
	};

	struct VisualScene
	{
		String name;
		Vector<Node*> root_nodes;

		~VisualScene()
		{
			for (int i = 0; i < root_nodes.size(); i++) {
				memdelete(root_nodes[i]);
			}
		}
	};

	struct AnimationClip
	{
		String name;
		float begin = 0;
		float end = 1;
		Vector<String> tracks;
	};

	struct AnimationTrack
	{
		String id;
		String target;
		String param;
		String component;
		bool property = false;

		enum InterpolationType
		{
			INTERP_LINEAR,
			INTERP_BEZIER
		};

		struct Key
		{
			enum Type
			{
				TYPE_FLOAT,
				TYPE_MATRIX
			};

			float time = 0;
			Vector<float> data;
			Point2 in_tangent;
			Point2 out_tangent;
			InterpolationType interp_type = INTERP_LINEAR;
		};

		Vector<float> get_value_at_time(float p_time) const;

		Vector<Key> keys;
	};

	/****************/
	/* IMPORT STATE */
	/****************/

	struct State
	{
		int import_flags = 0;

		float unit_scale = 1.0;
		Vector3::Axis up_axis = Vector3::AXIS_Y;
		bool z_up = false;

		struct Version
		{
			int major = 0, minor = 0, rev = 0;

			bool operator<(const Version& p_ver) const
			{
				return (major == p_ver.major)
						   ? ((minor == p_ver.minor) ? (rev < p_ver.rev) : minor < p_ver.minor)
						   : major < p_ver.major;
			}

			Version(int p_major = 0, int p_minor = 0, int p_rev = 0)
			{
				major = p_major;
				minor = p_minor;
				rev = p_rev;
			}
		} version;

		HashMap<String, CameraData> camera_data_map;
		HashMap<String, MeshData> mesh_data_map;
		HashMap<String, LightData> light_data_map;
		HashMap<String, CurveData> curve_data_map;

		HashMap<String, String> mesh_name_map;
		HashMap<String, String> morph_name_map;
		HashMap<String, String> morph_ownership_map;
		HashMap<String, SkinControllerData> skin_controller_data_map;
		HashMap<String, MorphControllerData> morph_controller_data_map;

		HashMap<String, Image> image_map;
		HashMap<String, Material> material_map;
		HashMap<String, Effect> effect_map;

		HashMap<String, VisualScene> visual_scene_map;
		HashMap<String, Node*> scene_map;
		HashSet<String> idref_joints;
		HashMap<String, String> sid_to_node_map;
		// RBMap<String,NodeJoint*> bone_map;

		HashMap<String, Transform3D> bone_rest_map;

		String local_path;
		String root_visual_scene;
		String root_physics_scene;

		Vector<AnimationClip> animation_clips;
		Vector<AnimationTrack> animation_tracks;
		HashMap<String, Vector<int>> referenced_tracks;
		HashMap<String, Vector<int>> by_id_tracks;

		float animation_length = 0;
	} state;

	Error load(const String& p_path, int p_flags = 0);

	Transform3D fix_transform(const Transform3D& p_transform);

	Transform3D get_root_transform() const;

	int get_uv_channel(const String& p_name);

private: // private stuff
	HashMap<String, int> channel_map;

	void _parse_asset(XMLParser& p_parser);
	void _parse_image(XMLParser& p_parser);
	void _parse_material(XMLParser& p_parser);
	void _parse_effect_material(XMLParser& p_parser, Effect& p_effect, String& p_id);
	void _parse_effect(XMLParser& p_parser);
	void _parse_camera(XMLParser& p_parser);
	void _parse_light(XMLParser& p_parser);
	void _parse_animation_clip(XMLParser& p_parser);

	void _parse_mesh_geometry(XMLParser& p_parser, const String& p_id, const String& p_name);
	void _parse_curve_geometry(XMLParser& p_parser, const String& p_id, const String& p_name);

	void _parse_skin_controller(XMLParser& p_parser, const String& p_id);
	void _parse_morph_controller(XMLParser& p_parser, const String& p_id);
	void _parse_controller(XMLParser& p_parser);

	Node* _parse_visual_instance_geometry(XMLParser& p_parser);
	Node* _parse_visual_instance_camera(XMLParser& p_parser);
	Node* _parse_visual_instance_light(XMLParser& p_parser);

	Node* _parse_visual_node_instance_data(XMLParser& p_parser);
	Node* _parse_visual_scene_node(XMLParser& p_parser);
	void _parse_visual_scene(XMLParser& p_parser);

	void _parse_animation(XMLParser& p_parser);
	void _parse_scene(XMLParser& p_parser);
	void _parse_library(XMLParser& p_parser);

	Vector<float> _read_float_array(XMLParser& p_parser);
	Vector<String> _read_string_array(XMLParser& p_parser);
	Transform3D _read_transform(XMLParser& p_parser);
	String _read_empty_draw_type(XMLParser& p_parser);

	void _joint_set_owner(Collada::Node* p_node, NodeSkeleton* p_owner);
	void _create_skeletons(Collada::Node** p_node, NodeSkeleton* p_skeleton = nullptr);
	void _find_morph_nodes(VisualScene* p_vscene, Node* p_node);
	bool _remove_node(Node* p_parent, Node* p_node);
	void _remove_node(VisualScene* p_vscene, Node* p_node);
	void _merge_skeletons2(VisualScene* p_vscene);
	void _merge_skeletons(VisualScene* p_vscene, Node* p_node);
	bool _optimize_skeletons(VisualScene* p_vscene, Node* p_node);

	bool _move_geometry_to_skeletons(VisualScene* p_vscene, Node* p_node, List<Node*>* p_mgeom);

	void _optimize();
};



// --- End editor/import/3d/collada.h ---
#include "core/config/project_settings.h"

// #define DEBUG_DEFAULT_ANIMATION
// #define DEBUG_COLLADA
#ifdef DEBUG_COLLADA
#define COLLADA_PRINT(m_what) print_line(m_what)
#else
#define COLLADA_PRINT(m_what)
#endif

#define COLLADA_IMPORT_SCALE_SCENE

/* HELPERS */

String Collada::Effect::get_texture_path(const String& p_source, Collada& p_state) const
{
	const String& image = p_source;
	ERR_FAIL_COND_V(!p_state.state.image_map.has(image), "");
	return p_state.state.image_map[image].path;
}

Transform3D Collada::get_root_transform() const
{
	Transform3D unit_scale_transform;
#ifndef COLLADA_IMPORT_SCALE_SCENE
	unit_scale_transform.scale(Vector3(state.unit_scale, state.unit_scale, state.unit_scale));
#endif
	return unit_scale_transform;
}

void Collada::Vertex::fix_unit_scale(const Collada& p_state)
{
#ifdef COLLADA_IMPORT_SCALE_SCENE
	vertex *= p_state.state.unit_scale;
#endif
}

static String _uri_to_id(const String& p_uri)
{
	if (p_uri.begins_with("#")) {
		return p_uri.substr(1);
	}
	else {
		return p_uri;
	}
}

/** HELPER FUNCTIONS **/

Transform3D Collada::fix_transform(const Transform3D& p_transform)
{
	Transform3D tr = p_transform;

#ifndef NO_UP_AXIS_SWAP

	if (state.up_axis != Vector3::AXIS_Y) {
		for (int i = 0; i < 3; i++) {
			SWAP(tr.basis[1][i], tr.basis[state.up_axis][i]);
		}
		for (int i = 0; i < 3; i++) {
			SWAP(tr.basis[i][1], tr.basis[i][state.up_axis]);
		}

		SWAP(tr.origin[1], tr.origin[state.up_axis]);

		tr.basis[state.up_axis][0] = -tr.basis[state.up_axis][0];
		tr.basis[state.up_axis][1] = -tr.basis[state.up_axis][1];
		tr.basis[0][state.up_axis] = -tr.basis[0][state.up_axis];
		tr.basis[1][state.up_axis] = -tr.basis[1][state.up_axis];
		tr.origin[state.up_axis] = -tr.origin[state.up_axis];
	}
#endif

	// tr.scale(Vector3(state.unit_scale.unit_scale.unit_scale));
	return tr;
	// return state.matrix_fix * p_transform;
}

static Transform3D _read_transform_from_array(const Vector<float>& p_array, int p_ofs = 0)
{
	Transform3D tr;
	// i wonder why collada matrices are transposed, given that's opposed to opengl..
	tr.basis.rows[0][0] = p_array[0 + p_ofs];
	tr.basis.rows[0][1] = p_array[1 + p_ofs];
	tr.basis.rows[0][2] = p_array[2 + p_ofs];
	tr.basis.rows[1][0] = p_array[4 + p_ofs];
	tr.basis.rows[1][1] = p_array[5 + p_ofs];
	tr.basis.rows[1][2] = p_array[6 + p_ofs];
	tr.basis.rows[2][0] = p_array[8 + p_ofs];
	tr.basis.rows[2][1] = p_array[9 + p_ofs];
	tr.basis.rows[2][2] = p_array[10 + p_ofs];
	tr.origin.x = p_array[3 + p_ofs];
	tr.origin.y = p_array[7 + p_ofs];
	tr.origin.z = p_array[11 + p_ofs];
	return tr;
}

/* STRUCTURES */

Transform3D Collada::Node::compute_transform(const Collada& p_state) const
{
	Transform3D xform;

	for (int i = 0; i < xform_list.size(); i++) {
		Transform3D xform_step;
		const XForm& xf = xform_list[i];
		switch (xf.op) {
		case XForm::OP_ROTATE: {
			if (xf.data.size() >= 4) {
				xform_step.rotate(
					Vector3(xf.data[0], xf.data[1], xf.data[2]), Math::deg_to_rad(xf.data[3]));
			}
		} break;
		case XForm::OP_SCALE: {
			if (xf.data.size() >= 3) {
				xform_step.scale(Vector3(xf.data[0], xf.data[1], xf.data[2]));
			}

		} break;
		case XForm::OP_TRANSLATE: {
			if (xf.data.size() >= 3) {
				xform_step.origin = Vector3(xf.data[0], xf.data[1], xf.data[2]);
			}

		} break;
		case XForm::OP_MATRIX: {
			if (xf.data.size() >= 16) {
				xform_step = _read_transform_from_array(xf.data, 0);
			}

		} break;
		default: {
		}
		}

		xform = xform * xform_step;
	}

#ifdef COLLADA_IMPORT_SCALE_SCENE
	xform.origin *= p_state.state.unit_scale;
#endif
	return xform;
}

Transform3D Collada::Node::get_transform() const { return default_transform; }

Transform3D Collada::Node::get_global_transform() const
{
	if (parent) {
		return parent->get_global_transform() * default_transform;
	}
	else {
		return default_transform;
	}
}

Vector<float> Collada::AnimationTrack::get_value_at_time(float p_time) const
{
	ERR_FAIL_COND_V(keys.is_empty(), Vector<float>());
	int i = 0;

	for (i = 0; i < keys.size(); i++) {
		if (keys[i].time > p_time) {
			break;
		}
	}

	if (i == 0) {
		return keys[0].data;
	}
	if (i == keys.size()) {
		return keys[keys.size() - 1].data;
	}

	switch (keys[i].interp_type) {
	case INTERP_BEZIER: // wait for bezier
	case INTERP_LINEAR: {
		float c = (p_time - keys[i - 1].time) / (keys[i].time - keys[i - 1].time);

		if (keys[i].data.size() == 16) {
			// interpolate a matrix
			Transform3D src = _read_transform_from_array(keys[i - 1].data);
			Transform3D dst = _read_transform_from_array(keys[i].data);

			Transform3D interp = c < 0.001 ? src : src.interpolate_with(dst, c);

			Vector<float> ret;
			ret.resize(16);
			// i wonder why collada matrices are transposed, given that's opposed to opengl..
			ret.write[0] = interp.basis.rows[0][0];
			ret.write[1] = interp.basis.rows[0][1];
			ret.write[2] = interp.basis.rows[0][2];
			ret.write[4] = interp.basis.rows[1][0];
			ret.write[5] = interp.basis.rows[1][1];
			ret.write[6] = interp.basis.rows[1][2];
			ret.write[8] = interp.basis.rows[2][0];
			ret.write[9] = interp.basis.rows[2][1];
			ret.write[10] = interp.basis.rows[2][2];
			ret.write[3] = interp.origin.x;
			ret.write[7] = interp.origin.y;
			ret.write[11] = interp.origin.z;
			ret.write[12] = 0;
			ret.write[13] = 0;
			ret.write[14] = 0;
			ret.write[15] = 1;

			return ret;
		}
		else {
			Vector<float> dest;
			dest.resize(keys[i].data.size());
			for (int j = 0; j < dest.size(); j++) {
				dest.write[j] = keys[i].data[j] * c + keys[i - 1].data[j] * (1.0 - c);
			}
			return dest;
			// interpolate one by one
		}
	} break;
	}

	ERR_FAIL_V(Vector<float>());
}

void Collada::_parse_asset(XMLParser& p_parser)
{
	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "up_axis") {
				p_parser.read();
				if (p_parser.get_node_data() == "X_UP") {
					state.up_axis = Vector3::AXIS_X;
				}
				if (p_parser.get_node_data() == "Y_UP") {
					state.up_axis = Vector3::AXIS_Y;
				}
				if (p_parser.get_node_data() == "Z_UP") {
					state.up_axis = Vector3::AXIS_Z;
				}

				COLLADA_PRINT("up axis: " + p_parser.get_node_data());
			}
			else if (name == "unit") {
				state.unit_scale = p_parser.get_named_attribute_value("meter").to_float();
				COLLADA_PRINT("unit scale: " + rtos(state.unit_scale));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "asset") {
			break; // end of <asset>
		}
	}
}

void Collada::_parse_image(XMLParser& p_parser)
{
	String id = p_parser.get_named_attribute_value("id");

	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	Image image;

	if (state.version < State::Version(1, 4, 0)) {
		/* <1.4 */
		String path = p_parser.get_named_attribute_value("source").strip_edges();
		if (!path.contains("://") && path.is_relative_path()) {
			// path is relative to file being loaded, so convert to a resource path
			image.path = ProjectSettings::localize_path(
				state.local_path.get_base_dir().path_join(path.uri_file_decode()));
		}
	}
	else {
		while (p_parser.read() == OK) {
			if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
				String name = p_parser.get_node_name();

				if (name == "init_from") {
					p_parser.read();
					String path = p_parser.get_node_data().strip_edges().uri_file_decode();

					if (!path.contains("://") && path.is_relative_path()) {
						// path is relative to file being loaded, so convert to a resource path
						path = ProjectSettings::localize_path(
							state.local_path.get_base_dir().path_join(path));

					}
					else if (path.find("file:///") == 0) {
						path = path.replace_first("file:///", "");
						path = ProjectSettings::localize_path(path);
					}

					image.path = path;

				}
				else if (name == "data") {
					ERR_PRINT("COLLADA Embedded image data not supported!");

				}
				else if (name == "extra" && !p_parser.is_empty()) {
					p_parser.skip_section();
				}

			}
			else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
					   p_parser.get_node_name() == "image") {
				break; // end of <asset>
			}
		}
	}

	state.image_map[id] = image;
}

void Collada::_parse_material(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	Material material;

	String id = p_parser.get_named_attribute_value("id");
	if (p_parser.has_attribute("name")) {
		material.name = p_parser.get_named_attribute_value("name");
	}

	if (state.version < State::Version(1, 4, 0)) {
		/* <1.4 */
		ERR_PRINT("Collada Materials < 1.4 are not supported (yet)");
	}
	else {
		while (p_parser.read() == OK) {
			if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT &&
				p_parser.get_node_name() == "instance_effect") {
				material.instance_effect = _uri_to_id(p_parser.get_named_attribute_value("url"));
			}
			else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
					   p_parser.get_node_name() == "material") {
				break; // end of <asset>
			}
		}
	}

	state.material_map[id] = material;
}

//! reads floats from inside of xml element until end of xml element
Vector<float> Collada::_read_float_array(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return Vector<float>();
	}

	Vector<String> splitters;
	splitters.push_back(" ");
	splitters.push_back("\n");
	splitters.push_back("\r");
	splitters.push_back("\t");

	Vector<float> array;
	while (p_parser.read() == OK) {
		// TODO: check for comments inside the element
		// and ignore them.

		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			// parse float data
			String str = p_parser.get_node_data();
			array = str.split_floats_mk(splitters, false);
			// array=str.split_floats(" ",false);
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}

	return array;
}

Vector<String> Collada::_read_string_array(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return Vector<String>();
	}

	Vector<String> array;
	while (p_parser.read() == OK) {
		// TODO: check for comments inside the element
		// and ignore them.

		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			// parse String data
			String str = p_parser.get_node_data();
			array = str.split_spaces();
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}

	return array;
}

Transform3D Collada::_read_transform(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return Transform3D();
	}

	Vector<String> array;
	while (p_parser.read() == OK) {
		// TODO: check for comments inside the element
		// and ignore them.

		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			// parse float data
			String str = p_parser.get_node_data();
			array = str.split_spaces();
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}

	ERR_FAIL_COND_V(array.size() != 16, Transform3D());
	Vector<float> farr;
	farr.resize(16);
	for (int i = 0; i < 16; i++) {
		farr.write[i] = array[i].to_float();
	}

	return _read_transform_from_array(farr);
}

String Collada::_read_empty_draw_type(XMLParser& p_parser)
{
	String empty_draw_type = "";

	if (p_parser.is_empty()) {
		return empty_draw_type;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_TEXT) {
			empty_draw_type = p_parser.get_node_data();
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END) {
			break; // end parsing text
		}
	}
	return empty_draw_type;
}

void Collada::_parse_effect(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	String id = p_parser.get_named_attribute_value("id");

	Effect effect;
	if (p_parser.has_attribute("name")) {
		effect.name = p_parser.get_named_attribute_value("name");
	}
	_parse_effect_material(p_parser, effect, id);

	state.effect_map[id] = effect;

	COLLADA_PRINT("Effect ID:" + id);
}

void Collada::_parse_camera(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	String id = p_parser.get_named_attribute_value("id");

	state.camera_data_map[id] = CameraData();
	CameraData& camera = state.camera_data_map[id];

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "perspective") {
				camera.mode = CameraData::MODE_PERSPECTIVE;
			}
			else if (name == "orthographic") {
				camera.mode = CameraData::MODE_ORTHOGONAL;
			}
			else if (name == "xfov") {
				p_parser.read();
				camera.perspective.x_fov = p_parser.get_node_data().to_float();

			}
			else if (name == "yfov") {
				p_parser.read();
				camera.perspective.y_fov = p_parser.get_node_data().to_float();
			}
			else if (name == "xmag") {
				p_parser.read();
				camera.orthogonal.x_mag = p_parser.get_node_data().to_float();

			}
			else if (name == "ymag") {
				p_parser.read();
				camera.orthogonal.y_mag = p_parser.get_node_data().to_float();
			}
			else if (name == "aspect_ratio") {
				p_parser.read();
				camera.aspect = p_parser.get_node_data().to_float();

			}
			else if (name == "znear") {
				p_parser.read();
				camera.z_near = p_parser.get_node_data().to_float();

			}
			else if (name == "zfar") {
				p_parser.read();
				camera.z_far = p_parser.get_node_data().to_float();
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "camera") {
			break; // end of <asset>
		}
	}

	COLLADA_PRINT("Camera ID:" + id);
}

void Collada::_parse_light(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	String id = p_parser.get_named_attribute_value("id");

	state.light_data_map[id] = LightData();
	LightData& light = state.light_data_map[id];

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "ambient") {
				light.mode = LightData::MODE_AMBIENT;
			}
			else if (name == "directional") {
				light.mode = LightData::MODE_DIRECTIONAL;
			}
			else if (name == "point") {
				light.mode = LightData::MODE_OMNI;
			}
			else if (name == "spot") {
				light.mode = LightData::MODE_SPOT;
			}
			else if (name == "color") {
				p_parser.read();
				Vector<float> colorarr = _read_float_array(p_parser);
				COLLADA_PRINT("colorarr size: " + rtos(colorarr.size()));

				if (colorarr.size() >= 4) {
					// alpha strangely not alright? maybe it needs to be multiplied by value as a
					// channel intensity
					Color color(colorarr[0], colorarr[1], colorarr[2], 1.0);
					light.color = color;
				}

			}
			else if (name == "constant_attenuation") {
				p_parser.read();
				light.constant_att = p_parser.get_node_data().to_float();
			}
			else if (name == "linear_attenuation") {
				p_parser.read();
				light.linear_att = p_parser.get_node_data().to_float();
			}
			else if (name == "quadratic_attenuation") {
				p_parser.read();
				light.quad_att = p_parser.get_node_data().to_float();
			}
			else if (name == "falloff_angle") {
				p_parser.read();
				light.spot_angle = p_parser.get_node_data().to_float();

			}
			else if (name == "falloff_exponent") {
				p_parser.read();
				light.spot_exp = p_parser.get_node_data().to_float();
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "light") {
			break; // end of <asset>
		}
	}

	COLLADA_PRINT("Light ID:" + id);
}

void Collada::_parse_curve_geometry(XMLParser& p_parser, const String& p_id, const String& p_name)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	// load everything into a pre dictionary

	state.curve_data_map[p_id] = CurveData();

	CurveData& curvedata = state.curve_data_map[p_id];
	curvedata.name = p_name;
	String closed = p_parser.get_named_attribute_value_safe("closed").to_lower();
	curvedata.closed = closed == "true" || closed == "1";

	COLLADA_PRINT("curve name: " + p_name);

	String current_source;
	// handles geometry node and the curve children in this loop
	// read sources with arrays and accessor for each curve
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				curvedata.sources[id] = CurveData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (curvedata.sources.has(current_source)) {
					curvedata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(curvedata.sources[current_source].array.size()) +
								  " values.");
				}
			}
			else if (section == "Name_array") {
				// create a new array and read it.
				if (curvedata.sources.has(current_source)) {
					curvedata.sources[current_source].sarray = _read_string_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(curvedata.sources[current_source].array.size()) +
								  " values.");
				}

			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (curvedata.sources.has(current_source)) {
					curvedata.sources[current_source].stride =
						p_parser.get_named_attribute_value("stride").to_int();
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(curvedata.sources[current_source].stride));
				}
			}
			else if (section == "control_vertices") {
				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							curvedata.control_vertices[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

			}
			else if (!p_parser.is_empty()) {
				p_parser.skip_section();
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "spline") {
			break;
		}
	}
}

void Collada::_parse_mesh_geometry(XMLParser& p_parser, const String& p_id, const String& p_name)
{
	if (!(state.import_flags & IMPORT_FLAG_SCENE)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}
		return;
	}

	// load everything into a pre dictionary

	state.mesh_data_map[p_id] = MeshData();

	MeshData& meshdata = state.mesh_data_map[p_id];
	meshdata.name = p_name;

	COLLADA_PRINT("mesh name: " + p_name);

	String current_source;
	// handles geometry node and the mesh children in this loop
	// read sources with arrays and accessor for each mesh
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				meshdata.sources[id] = MeshData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (meshdata.sources.has(current_source)) {
					meshdata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(meshdata.sources[current_source].array.size()) + " values.");
				}
			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (meshdata.sources.has(current_source)) {
					meshdata.sources[current_source].stride =
						p_parser.get_named_attribute_value("stride").to_int();
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(meshdata.sources[current_source].stride));
				}
			}
			else if (section == "vertices") {
				MeshData::Vertices vert;
				String id = p_parser.get_named_attribute_value("id");
				int last_ref = 0;

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							if (semantic == "TEXCOORD") {
								semantic = "TEXCOORD" + itos(last_ref++);
							}

							vert.sources[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				meshdata.vertices[id] = vert;

			}
			else if (section == "triangles" || section == "polylist" || section == "polygons") {
				bool polygons = (section == "polygons");
				if (polygons) {
					WARN_PRINT("Primitive type \"polygons\" is not well supported (concave shapes "
							   "may fail). To ensure that the geometry is properly imported, "
							   "please re-export using \"triangles\" or \"polylist\".");
				}
				MeshData::Primitives prim;

				if (p_parser.has_attribute("material")) {
					prim.material = p_parser.get_named_attribute_value("material");
				}
				prim.count = p_parser.get_named_attribute_value("count").to_int();
				prim.vertex_size = 0;
				int last_ref = 0;

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							if (semantic == "TEXCOORD") {
								semantic = "TEXCOORD" + itos(last_ref++);
							}
							int offset = p_parser.get_named_attribute_value("offset").to_int();

							MeshData::Primitives::SourceRef sref;
							sref.source = source;
							sref.offset = offset;
							prim.sources[semantic] = sref;
							prim.vertex_size = MAX(prim.vertex_size, offset + 1);

							COLLADA_PRINT(section + " input semantic: " + semantic +
										  " source: " + source + " offset: " + itos(offset));

						}
						else if (p_parser.get_node_name() == "p") { // indices

							Vector<float> values = _read_float_array(p_parser);
							if (polygons) {
								ERR_CONTINUE(prim.vertex_size == 0);
								prim.polygons.push_back(values.size() / prim.vertex_size);
								int from = prim.indices.size();
								prim.indices.resize(from + values.size());
								for (int i = 0; i < values.size(); i++) {
									prim.indices.write[from + i] = values[i];
								}

							}
							else if (prim.vertex_size > 0) {
								prim.indices = values;
							}

							COLLADA_PRINT("read " + itos(values.size()) + " index values");

						}
						else if (p_parser.get_node_name() == "vcount") { // primitive

							Vector<float> values = _read_float_array(p_parser);
							prim.polygons = values;
							COLLADA_PRINT("read " + itos(values.size()) + " polygon values");
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				meshdata.primitives.push_back(prim);

			}
			else if (p_parser.get_node_name() == "double_sided") {
				p_parser.read();
				meshdata.found_double_sided = true;
				meshdata.double_sided = p_parser.get_node_data().to_int();

			}
			else if (p_parser.get_node_name() == "polygons") {
				ERR_PRINT("Primitive type \"polygons\" not supported, re-export using \"polylist\" "
						  "or \"triangles\".");
			}
			else if (!p_parser.is_empty()) {
				p_parser.skip_section();
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "mesh") {
			break;
		}
	}
}

void Collada::_parse_skin_controller(XMLParser& p_parser, const String& p_id)
{
	state.skin_controller_data_map[p_id] = SkinControllerData();
	SkinControllerData& skindata = state.skin_controller_data_map[p_id];

	skindata.base = _uri_to_id(p_parser.get_named_attribute_value("source"));

	String current_source;

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "bind_shape_matrix") {
				skindata.bind_shape = _read_transform(p_parser);
#ifdef COLLADA_IMPORT_SCALE_SCENE
				skindata.bind_shape.origin *= state.unit_scale;

#endif
				COLLADA_PRINT("skeleton bind shape transform: " + skindata.bind_shape);

			}
			else if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				skindata.sources[id] = SkinControllerData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (skindata.sources.has(current_source)) {
					skindata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(skindata.sources[current_source].array.size()) + " values.");
				}
			}
			else if (section == "Name_array" || section == "IDREF_array") {
				// create a new array and read it.

				if (section == "IDREF_array") {
					skindata.use_idrefs = true;
				}
				if (skindata.sources.has(current_source)) {
					skindata.sources[current_source].sarray = _read_string_array(p_parser);
					if (section == "IDREF_array") {
						Vector<String> sa = skindata.sources[current_source].sarray;
						for (int i = 0; i < sa.size(); i++) {
							state.idref_joints.insert(sa[i]);
						}
					}
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(skindata.sources[current_source].array.size()) + " values.");
				}
			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (skindata.sources.has(current_source)) {
					int stride = 1;
					if (p_parser.has_attribute("stride")) {
						stride = p_parser.get_named_attribute_value("stride").to_int();
					}

					skindata.sources[current_source].stride = stride;
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(skindata.sources[current_source].stride));
				}

			}
			else if (section == "joints") {
				SkinControllerData::Joints joint;

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							joint.sources[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				skindata.joints = joint;

			}
			else if (section == "vertex_weights") {
				SkinControllerData::Weights weights;

				weights.count = p_parser.get_named_attribute_value("count").to_int();

				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							int offset = p_parser.get_named_attribute_value("offset").to_int();

							SkinControllerData::Weights::SourceRef sref;
							sref.source = source;
							sref.offset = offset;
							weights.sources[semantic] = sref;

							COLLADA_PRINT(section + " input semantic: " + semantic +
										  " source: " + source + " offset: " + itos(offset));

						}
						else if (p_parser.get_node_name() == "v") { // indices

							Vector<float> values = _read_float_array(p_parser);
							weights.indices = values;
							COLLADA_PRINT("read " + itos(values.size()) + " index values");

						}
						else if (p_parser.get_node_name() == "vcount") { // weightsitive

							Vector<float> values = _read_float_array(p_parser);
							weights.sets = values;
							COLLADA_PRINT("read " + itos(values.size()) + " polygon values");
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}

				skindata.weights = weights;
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "skin") {
			break;
		}
	}

	/* STORE REST MATRICES */

	ERR_FAIL_COND(!skindata.joints.sources.has("JOINT"));
	ERR_FAIL_COND(!skindata.joints.sources.has("INV_BIND_MATRIX"));

	String joint_arr = skindata.joints.sources["JOINT"];
	String ibm = skindata.joints.sources["INV_BIND_MATRIX"];

	ERR_FAIL_COND(!skindata.sources.has(joint_arr));
	ERR_FAIL_COND(!skindata.sources.has(ibm));

	SkinControllerData::Source& joint_source = skindata.sources[joint_arr];
	SkinControllerData::Source& ibm_source = skindata.sources[ibm];

	ERR_FAIL_COND(joint_source.sarray.size() != ibm_source.array.size() / 16);

	for (int i = 0; i < joint_source.sarray.size(); i++) {
		String name = joint_source.sarray[i];
		Transform3D xform = _read_transform_from_array(
			ibm_source.array, i * 16); //<- this is a mistake, it must be applied to vertices
		xform.affine_invert();		   // inverse for rest, because it's an inverse
#ifdef COLLADA_IMPORT_SCALE_SCENE
		xform.origin *= state.unit_scale;
#endif
		skindata.bone_rest_map[name] = xform;
	}
}

void Collada::_parse_morph_controller(XMLParser& p_parser, const String& p_id)
{
	state.morph_controller_data_map[p_id] = MorphControllerData();
	MorphControllerData& morphdata = state.morph_controller_data_map[p_id];

	morphdata.mesh = _uri_to_id(p_parser.get_named_attribute_value("source"));
	morphdata.mode = p_parser.get_named_attribute_value("method");
	String current_source;

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "source") {
				String id = p_parser.get_named_attribute_value("id");
				morphdata.sources[id] = MorphControllerData::Source();
				current_source = id;
				COLLADA_PRINT("source data: " + id);

			}
			else if (section == "float_array" || section == "array") {
				// create a new array and read it.
				if (morphdata.sources.has(current_source)) {
					morphdata.sources[current_source].array = _read_float_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(morphdata.sources[current_source].array.size()) +
								  " values.");
				}
			}
			else if (section == "Name_array" || section == "IDREF_array") {
				// create a new array and read it.
				if (morphdata.sources.has(current_source)) {
					morphdata.sources[current_source].sarray = _read_string_array(p_parser);
					COLLADA_PRINT("section: " + current_source + " read " +
								  itos(morphdata.sources[current_source].array.size()) +
								  " values.");
				}
			}
			else if (section == "technique_common") {
				// skip it
			}
			else if (section == "accessor") { // child of source (below a technique tag)

				if (morphdata.sources.has(current_source)) {
					int stride = 1;
					if (p_parser.has_attribute("stride")) {
						stride = p_parser.get_named_attribute_value("stride").to_int();
					}

					morphdata.sources[current_source].stride = stride;
					COLLADA_PRINT("section: " + current_source + " stride " +
								  itos(morphdata.sources[current_source].stride));
				}

			}
			else if (section == "targets") {
				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "input") {
							String semantic = p_parser.get_named_attribute_value("semantic");
							String source =
								_uri_to_id(p_parser.get_named_attribute_value("source"));

							morphdata.targets[semantic] = source;

							COLLADA_PRINT(
								section + " input semantic: " + semantic + " source: " + source);
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == section) {
						break;
					}
				}
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "morph") {
			break;
		}
	}

	if (morphdata.targets.has("MORPH_WEIGHT")) {
		state.morph_name_map[morphdata.targets["MORPH_WEIGHT"]] = p_id;
	}
}

void Collada::_parse_controller(XMLParser& p_parser)
{
	String id = p_parser.get_named_attribute_value("id");

	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "skin") {
				_parse_skin_controller(p_parser, id);
			}
			else if (section == "morph") {
				_parse_morph_controller(p_parser, id);
			}
		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "controller") {
			break;
		}
	}
}

Collada::Node* Collada::_parse_visual_instance_geometry(XMLParser& p_parser)
{
	String type = p_parser.get_node_name();
	NodeGeometry* geom = memnew(NodeGeometry);
	geom->controller = type == "instance_controller";
	geom->source = _uri_to_id(p_parser.get_named_attribute_value_safe("url"));

	if (p_parser.is_empty()) { // nothing else to parse...
		return geom;
	}
	// try to find also many materials and skeletons!
	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			if (p_parser.get_node_name() == "instance_material") {
				String symbol = p_parser.get_named_attribute_value("symbol");
				String target = _uri_to_id(p_parser.get_named_attribute_value("target"));

				NodeGeometry::Material mat;
				mat.target = target;
				geom->material_map[symbol] = mat;
				COLLADA_PRINT("uses material: '" + target + "' on primitive'" + symbol + "'");
			}
			else if (p_parser.get_node_name() == "skeleton") {
				p_parser.read();
				String uri = _uri_to_id(p_parser.get_node_data());
				if (!uri.is_empty()) {
					geom->skeletons.push_back(uri);
				}
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == type) {
			break;
		}
	}

	if (geom->controller) {
		if (geom->skeletons.is_empty()) {
			// XSI style

			if (state.skin_controller_data_map.has(geom->source)) {
				SkinControllerData* skin = &state.skin_controller_data_map[geom->source];
				// case where skeletons reference bones with IDREF (XSI)
				ERR_FAIL_COND_V(!skin->joints.sources.has("JOINT"), geom);
				String joint_arr = skin->joints.sources["JOINT"];
				ERR_FAIL_COND_V(!skin->sources.has(joint_arr), geom);
				Collada::SkinControllerData::Source& joint_source = skin->sources[joint_arr];
				geom->skeletons = joint_source.sarray; // quite crazy, but should work.
			}
		}
	}

	return geom;
}

Collada::Node* Collada::_parse_visual_instance_camera(XMLParser& p_parser)
{
	NodeCamera* cam = memnew(NodeCamera);
	cam->camera = _uri_to_id(p_parser.get_named_attribute_value_safe("url"));

	if (state.up_axis == Vector3::AXIS_Z) { // collada weirdness
		cam->post_transform.basis.rotate(Vector3(1, 0, 0), -Math::PI * 0.5);
	}

	if (p_parser.is_empty()) { // nothing else to parse...
		return cam;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
			p_parser.get_node_name() == "instance_camera") {
			break;
		}
	}

	return cam;
}

Collada::Node* Collada::_parse_visual_instance_light(XMLParser& p_parser)
{
	NodeLight* cam = memnew(NodeLight);
	cam->light = _uri_to_id(p_parser.get_named_attribute_value_safe("url"));

	if (state.up_axis == Vector3::AXIS_Z) { // collada weirdness
		cam->post_transform.basis.rotate(Vector3(1, 0, 0), -Math::PI * 0.5);
	}

	if (p_parser.is_empty()) { // nothing else to parse...
		return cam;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
			p_parser.get_node_name() == "instance_light") {
			break;
		}
	}

	return cam;
}

Collada::Node* Collada::_parse_visual_node_instance_data(XMLParser& p_parser)
{
	String instance_type = p_parser.get_node_name();

	if (instance_type == "instance_geometry" || instance_type == "instance_controller") {
		return _parse_visual_instance_geometry(p_parser);
	}
	else if (instance_type == "instance_camera") {
		return _parse_visual_instance_camera(p_parser);
	}
	else if (instance_type == "instance_light") {
		return _parse_visual_instance_light(p_parser);
	}

	if (p_parser.is_empty()) { // nothing else to parse...
		return nullptr;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
			p_parser.get_node_name() == instance_type) {
			break;
		}
	}

	return nullptr;
}

Collada::Node* Collada::_parse_visual_scene_node(XMLParser& p_parser)
{
	String name;

	String id = p_parser.get_named_attribute_value_safe("id");

	bool found_name = false;

	if (id.is_empty()) {
		id = "%NODEID%" + itos(Math::rand());

	}
	else {
		found_name = true;
	}

	Vector<Node::XForm> xform_list;
	Vector<Node*> children;

	String empty_draw_type = "";

	Node* node = nullptr;

	name = p_parser.has_attribute("name") ? p_parser.get_named_attribute_value_safe("name")
										  : p_parser.get_named_attribute_value_safe("id");
	if (name.is_empty()) {
		name = id;
	}
	else {
		found_name = true;
	}

	if ((p_parser.has_attribute("type") && p_parser.get_named_attribute_value("type") == "JOINT") ||
		state.idref_joints.has(name)) {
		// handle a bone

		NodeJoint* joint = memnew(NodeJoint);

		if (p_parser.has_attribute("sid")) { // bones may not have sid
			joint->sid = p_parser.get_named_attribute_value("sid");
			// state.bone_map[joint->sid]=joint;
		}
		else if (state.idref_joints.has(name)) {
			joint->sid = name; // kind of a cheat but..
		}
		else if (p_parser.has_attribute("name")) {
			joint->sid = p_parser.get_named_attribute_value_safe("name");
		}

		if (!joint->sid.is_empty()) {
			state.sid_to_node_map[joint->sid] = id;
		}

		node = joint;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "translate") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_TRANSLATE;

				Vector<float> xlt = _read_float_array(p_parser);
				xf.data = xlt;
				xform_list.push_back(xf);

			}
			else if (section == "rotate") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_ROTATE;

				Vector<float> rot = _read_float_array(p_parser);
				xf.data = rot;

				xform_list.push_back(xf);

			}
			else if (section == "scale") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}

				xf.op = Node::XForm::OP_SCALE;

				Vector<float> scale = _read_float_array(p_parser);

				xf.data = scale;

				xform_list.push_back(xf);

			}
			else if (section == "matrix") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_MATRIX;

				Vector<float> matrix = _read_float_array(p_parser);

				xf.data = matrix;
				String mtx;
				for (int i = 0; i < matrix.size(); i++) {
					mtx += " " + rtos(matrix[i]);
				}

				xform_list.push_back(xf);

			}
			else if (section == "visibility") {
				Node::XForm xf;
				if (p_parser.has_attribute("sid")) {
					xf.id = p_parser.get_named_attribute_value("sid");
				}
				xf.op = Node::XForm::OP_VISIBILITY;

				Vector<float> visible = _read_float_array(p_parser);

				xf.data = visible;

				xform_list.push_back(xf);

			}
			else if (section == "empty_draw_type") {
				empty_draw_type = _read_empty_draw_type(p_parser);
			}
			else if (section == "technique" || section == "extra") {
			}
			else if (section != "node") {
				// usually what defines the type of node
				if (section.begins_with("instance_")) {
					if (!node) {
						node = _parse_visual_node_instance_data(p_parser);

					}
					else {
						ERR_PRINT("Multiple instance_* not supported.");
					}
				}

			}
			else {
				/* Found a child node!! what to do..*/

				Node* child = _parse_visual_scene_node(p_parser);
				children.push_back(child);
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "node") {
			break;
		}
	}

	if (!node) {
		node = memnew(Node); // generic node, nothing of relevance found
	}

	node->noname = !found_name;
	node->xform_list = xform_list;
	node->children = children;
	for (int i = 0; i < children.size(); i++) {
		node->children[i]->parent = node;
	}

	node->name = name;
	node->id = id;
	node->empty_draw_type = empty_draw_type;

	if (node->children.size() == 1) {
		if (node->children[0]->noname && !node->noname) {
			node->children[0]->name = node->name;
			node->name = node->name + "-base";
		}
	}

	node->default_transform = node->compute_transform(*this);
	state.scene_map[id] = node;

	return node;
}

void Collada::_parse_visual_scene(XMLParser& p_parser)
{
	String id = p_parser.get_named_attribute_value("id");

	if (p_parser.is_empty()) {
		return;
	}

	state.visual_scene_map[id] = VisualScene();
	VisualScene& vscene = state.visual_scene_map[id];

	if (p_parser.has_attribute("name")) {
		vscene.name = p_parser.get_named_attribute_value("name");
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String section = p_parser.get_node_name();

			if (section == "node") {
				vscene.root_nodes.push_back(_parse_visual_scene_node(p_parser));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "visual_scene") {
			break;
		}
	}

	COLLADA_PRINT("Scene ID:" + id);
}

void Collada::_parse_animation(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_ANIMATION)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}

		return;
	}

	HashMap<String, Vector<float>> float_sources;
	HashMap<String, Vector<String>> string_sources;
	HashMap<String, int> source_strides;
	HashMap<String, HashMap<String, String>> samplers;
	HashMap<String, Vector<String>> source_param_names;
	HashMap<String, Vector<String>> source_param_types;

	String id = "";
	if (p_parser.has_attribute("id")) {
		id = p_parser.get_named_attribute_value("id");
	}

	String current_source;
	String current_sampler;
	Vector<String> channel_sources;
	Vector<String> channel_targets;

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();
			if (name == "source") {
				current_source = p_parser.get_named_attribute_value("id");
				source_param_names[current_source] = Vector<String>();
				source_param_types[current_source] = Vector<String>();

			}
			else if (name == "float_array") {
				if (!current_source.is_empty()) {
					float_sources[current_source] = _read_float_array(p_parser);
				}

			}
			else if (name == "Name_array") {
				if (!current_source.is_empty()) {
					string_sources[current_source] = _read_string_array(p_parser);
				}
			}
			else if (name == "accessor") {
				if (!current_source.is_empty() && p_parser.has_attribute("stride")) {
					source_strides[current_source] =
						p_parser.get_named_attribute_value("stride").to_int();
				}
			}
			else if (name == "sampler") {
				current_sampler = p_parser.get_named_attribute_value("id");
				samplers[current_sampler] = HashMap<String, String>();
			}
			else if (name == "param") {
				if (p_parser.has_attribute("name")) {
					source_param_names[current_source].push_back(
						p_parser.get_named_attribute_value("name"));
				}
				else {
					source_param_names[current_source].push_back("");
				}

				if (p_parser.has_attribute("type")) {
					source_param_types[current_source].push_back(
						p_parser.get_named_attribute_value("type"));
				}
				else {
					source_param_types[current_source].push_back("");
				}

			}
			else if (name == "input") {
				if (!current_sampler.is_empty()) {
					samplers[current_sampler][p_parser.get_named_attribute_value("semantic")] =
						p_parser.get_named_attribute_value("source");
				}

			}
			else if (name == "channel") {
				channel_sources.push_back(p_parser.get_named_attribute_value("source"));
				channel_targets.push_back(p_parser.get_named_attribute_value("target"));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "animation") {
			break; // end of <asset>
		}
	}

	for (int i = 0; i < channel_sources.size(); i++) {
		String source = _uri_to_id(channel_sources[i]);
		const String& target = channel_targets[i];
		ERR_CONTINUE(!samplers.has(source));
		HashMap<String, String>& sampler = samplers[source];

		ERR_CONTINUE(!sampler.has("INPUT")); // no input semantic? wtf?
		String input_id = _uri_to_id(sampler["INPUT"]);
		COLLADA_PRINT("input id is " + input_id);
		ERR_CONTINUE(!float_sources.has(input_id));

		ERR_CONTINUE(!sampler.has("OUTPUT"));
		String output_id = _uri_to_id(sampler["OUTPUT"]);
		ERR_CONTINUE(!float_sources.has(output_id));

		ERR_CONTINUE(!source_param_names.has(output_id));

		Vector<String>& names = source_param_names[output_id];

		for (int l = 0; l < names.size(); l++) {
			String name = names[l];

			Vector<float>& time_keys = float_sources[input_id];
			int key_count = time_keys.size();

			AnimationTrack track; // begin crating track
			track.id = id;

			track.keys.resize(key_count);

			for (int j = 0; j < key_count; j++) {
				track.keys.write[j].time = time_keys[j];
				state.animation_length = MAX(state.animation_length, time_keys[j]);
			}

			// now read actual values

			int stride = 1;

			if (source_strides.has(output_id)) {
				stride = source_strides[output_id];
			}
			int output_len = stride / names.size();

			ERR_CONTINUE(output_len == 0);
			ERR_CONTINUE(!float_sources.has(output_id));

			Vector<float>& output = float_sources[output_id];

			ERR_CONTINUE_MSG(
				(output.size() / stride) != key_count, "Wrong number of keys in output.");

			for (int j = 0; j < key_count; j++) {
				track.keys.write[j].data.resize(output_len);
				for (int k = 0; k < output_len; k++) {
					track.keys.write[j].data.write[k] =
						output[l + j * stride + k]; // super weird but should work:
				}
			}

			if (sampler.has("INTERPOLATION")) {
				String interp_id = _uri_to_id(sampler["INTERPOLATION"]);
				ERR_CONTINUE(!string_sources.has(interp_id));
				Vector<String>& interps = string_sources[interp_id];
				ERR_CONTINUE(interps.size() != key_count);

				for (int j = 0; j < key_count; j++) {
					if (interps[j] == "BEZIER") {
						track.keys.write[j].interp_type = AnimationTrack::INTERP_BEZIER;
					}
					else {
						track.keys.write[j].interp_type = AnimationTrack::INTERP_LINEAR;
					}
				}
			}

			if (sampler.has("IN_TANGENT") && sampler.has("OUT_TANGENT")) {
				// bezier control points..
				String intangent_id = _uri_to_id(sampler["IN_TANGENT"]);
				ERR_CONTINUE(!float_sources.has(intangent_id));
				Vector<float>& intangents = float_sources[intangent_id];

				ERR_CONTINUE(intangents.size() != key_count * 2 * names.size());

				String outangent_id = _uri_to_id(sampler["OUT_TANGENT"]);
				ERR_CONTINUE(!float_sources.has(outangent_id));
				Vector<float>& outangents = float_sources[outangent_id];
				ERR_CONTINUE(outangents.size() != key_count * 2 * names.size());

				for (int j = 0; j < key_count; j++) {
					track.keys.write[j].in_tangent =
						Vector2(intangents[j * 2 * names.size() + 0 + l * 2],
							intangents[j * 2 * names.size() + 1 + l * 2]);
					track.keys.write[j].out_tangent =
						Vector2(outangents[j * 2 * names.size() + 0 + l * 2],
							outangents[j * 2 * names.size() + 1 + l * 2]);
				}
			}

			if (target.contains_char('/')) { // transform component
				track.target = target.get_slicec('/', 0);
				track.param = target.get_slicec('/', 1);
				if (track.param.contains_char('.')) {
					track.component = track.param.get_slicec('.', 1).to_upper();
				}
				track.param = track.param.get_slicec('.', 0);
				if (names.size() > 1 && track.component.is_empty()) {
					// this is a guess because the collada spec is ambiguous here...
					// i suppose if you have many names (outputs) you can't use a component and i
					// should abide to that.
					track.component = name;
				}
			}
			else {
				track.target = target;
			}

			state.animation_tracks.push_back(track);

			if (!state.referenced_tracks.has(target)) {
				state.referenced_tracks[target] = Vector<int>();
			}

			state.referenced_tracks[target].push_back(state.animation_tracks.size() - 1);

			if (!id.is_empty()) {
				if (!state.by_id_tracks.has(id)) {
					state.by_id_tracks[id] = Vector<int>();
				}

				state.by_id_tracks[id].push_back(state.animation_tracks.size() - 1);
			}

			COLLADA_PRINT("loaded animation with " + itos(key_count) + " keys");
		}
	}
}

void Collada::_parse_animation_clip(XMLParser& p_parser)
{
	if (!(state.import_flags & IMPORT_FLAG_ANIMATION)) {
		if (!p_parser.is_empty()) {
			p_parser.skip_section();
		}

		return;
	}

	AnimationClip clip;

	if (p_parser.has_attribute("name")) {
		clip.name = p_parser.get_named_attribute_value("name");
	}
	else if (p_parser.has_attribute("id")) {
		clip.name = p_parser.get_named_attribute_value("id");
	}
	if (p_parser.has_attribute("start")) {
		clip.begin = p_parser.get_named_attribute_value("start").to_float();
	}
	if (p_parser.has_attribute("end")) {
		clip.end = p_parser.get_named_attribute_value("end").to_float();
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();
			if (name == "instance_animation") {
				String url = _uri_to_id(p_parser.get_named_attribute_value("url"));
				clip.tracks.push_back(url);
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "animation_clip") {
			break; // end of <asset>
		}
	}

	state.animation_clips.push_back(clip);
}

void Collada::_parse_scene(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();

			if (name == "instance_visual_scene") {
				state.root_visual_scene = _uri_to_id(p_parser.get_named_attribute_value("url"));
			}
			else if (name == "instance_physics_scene") {
				state.root_physics_scene = _uri_to_id(p_parser.get_named_attribute_value("url"));
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name() == "scene") {
			break; // end of <asset>
		}
	}
}

void Collada::_parse_library(XMLParser& p_parser)
{
	if (p_parser.is_empty()) {
		return;
	}

	while (p_parser.read() == OK) {
		if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			String name = p_parser.get_node_name();
			COLLADA_PRINT("library name is: " + name);
			if (name == "image") {
				_parse_image(p_parser);
			}
			else if (name == "material") {
				_parse_material(p_parser);
			}
			else if (name == "effect") {
				_parse_effect(p_parser);
			}
			else if (name == "camera") {
				_parse_camera(p_parser);
			}
			else if (name == "light") {
				_parse_light(p_parser);
			}
			else if (name == "geometry") {
				String id = p_parser.get_named_attribute_value("id");
				String name2 = p_parser.get_named_attribute_value_safe("name");
				while (p_parser.read() == OK) {
					if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT) {
						if (p_parser.get_node_name() == "mesh") {
							state.mesh_name_map[id] = (!name2.is_empty()) ? name2 : id;
							_parse_mesh_geometry(p_parser, id, name2);
						}
						else if (p_parser.get_node_name() == "spline") {
							state.mesh_name_map[id] = (!name2.is_empty()) ? name2 : id;
							_parse_curve_geometry(p_parser, id, name2);
						}
						else if (!p_parser.is_empty()) {
							p_parser.skip_section();
						}
					}
					else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
							   p_parser.get_node_name() == "geometry") {
						break;
					}
				}

			}
			else if (name == "controller") {
				_parse_controller(p_parser);
			}
			else if (name == "animation") {
				_parse_animation(p_parser);
			}
			else if (name == "animation_clip") {
				_parse_animation_clip(p_parser);
			}
			else if (name == "visual_scene") {
				COLLADA_PRINT("visual scene");
				_parse_visual_scene(p_parser);
			}
			else if (!p_parser.is_empty()) {
				p_parser.skip_section();
			}

		}
		else if (p_parser.get_node_type() == XMLParser::NODE_ELEMENT_END &&
				   p_parser.get_node_name().begins_with("library_")) {
			break; // end of <asset>
		}
	}
}

void Collada::_joint_set_owner(Collada::Node* p_node, NodeSkeleton* p_owner)
{
	if (p_node->type == Node::TYPE_JOINT) {
		NodeJoint* nj = static_cast<NodeJoint*>(p_node);
		nj->owner = p_owner;

		for (int i = 0; i < nj->children.size(); i++) {
			_joint_set_owner(nj->children.write[i], p_owner);
		}
	}
}

void Collada::_create_skeletons(Collada::Node** p_node, NodeSkeleton* p_skeleton)
{
	Node* node = *p_node;

	if (node->type == Node::TYPE_JOINT) {
		if (!p_skeleton) {
			// ohohohoohoo it's a joint node, time to work!
			NodeSkeleton* sk = memnew(NodeSkeleton);
			*p_node = sk;
			sk->children.push_back(node);
			sk->parent = node->parent;
			node->parent = sk;
			p_skeleton = sk;
		}

		NodeJoint* nj = static_cast<NodeJoint*>(node);
		nj->owner = p_skeleton;
	}
	else {
		p_skeleton = nullptr;
	}

	for (int i = 0; i < node->children.size(); i++) {
		_create_skeletons(&node->children.write[i], p_skeleton);
	}
}

bool Collada::_remove_node(Node* p_parent, Node* p_node)
{
	for (int i = 0; i < p_parent->children.size(); i++) {
		if (p_parent->children[i] == p_node) {
			p_parent->children.remove_at(i);
			return true;
		}
		if (_remove_node(p_parent->children[i], p_node)) {
			return true;
		}
	}

	return false;
}

void Collada::_remove_node(VisualScene* p_vscene, Node* p_node)
{
	for (int i = 0; i < p_vscene->root_nodes.size(); i++) {
		if (p_vscene->root_nodes[i] == p_node) {
			p_vscene->root_nodes.remove_at(i);
			return;
		}
		if (_remove_node(p_vscene->root_nodes[i], p_node)) {
			return;
		}
	}

	ERR_PRINT("Not found node to remove?");
}

void Collada::_merge_skeletons(VisualScene* p_vscene, Node* p_node)
{
	if (p_node->type == Node::TYPE_GEOMETRY) {
		NodeGeometry* gnode = static_cast<NodeGeometry*>(p_node);
		if (gnode->controller) {
			// recount skeletons used
			HashSet<NodeSkeleton*> skeletons;

			for (int i = 0; i < gnode->skeletons.size(); i++) {
				String nodeid = gnode->skeletons[i];

				ERR_CONTINUE(!state.scene_map.has(nodeid)); // weird, it should have it...

				NodeJoint* nj = dynamic_cast<NodeJoint*>(state.scene_map[nodeid]);
				ERR_CONTINUE(!nj);		  // broken collada
				ERR_CONTINUE(!nj->owner); // weird, node should have a skeleton owner

				skeletons.insert(nj->owner);
			}

			if (skeletons.size() > 1) {
				// do the merger!!
				HashSet<NodeSkeleton*>::Iterator E = skeletons.begin();
				NodeSkeleton* base = *E;

				for (++E; E; ++E) {
					NodeSkeleton* merged = *E;
					_remove_node(p_vscene, merged);
					for (int i = 0; i < merged->children.size(); i++) {
						_joint_set_owner(merged->children[i], base);
						base->children.push_back(merged->children[i]);
						merged->children[i]->parent = base;
					}

					merged->children.clear(); // take children from it
					memdelete(merged);
				}
			}
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		_merge_skeletons(p_vscene, p_node->children[i]);
	}
}

void Collada::_merge_skeletons2(VisualScene* p_vscene)
{
	for (KeyValue<String, SkinControllerData>& E : state.skin_controller_data_map) {
		SkinControllerData& cd = E.value;

		NodeSkeleton* skeleton = nullptr;

		for (const KeyValue<String, Transform3D>& F : cd.bone_rest_map) {
			String name;

			if (!state.sid_to_node_map.has(F.key)) {
				continue;
			}

			name = state.sid_to_node_map[F.key];

			ERR_CONTINUE(!state.scene_map.has(name));

			Node* node = state.scene_map[name];
			ERR_CONTINUE(node->type != Node::TYPE_JOINT);

			NodeSkeleton* sk = nullptr;

			while (node && !sk) {
				if (node->type == Node::TYPE_SKELETON) {
					sk = static_cast<NodeSkeleton*>(node);
				}
				node = node->parent;
			}

			ERR_CONTINUE(!sk);

			if (!skeleton) {
				skeleton = sk;
				continue;
			}

			if (skeleton != sk) {
				// whoa.. wtf, merge.
				_remove_node(p_vscene, sk);
				for (int i = 0; i < sk->children.size(); i++) {
					_joint_set_owner(sk->children[i], skeleton);
					skeleton->children.push_back(sk->children[i]);
					sk->children[i]->parent = skeleton;
				}

				sk->children.clear(); // take children from it
				memdelete(sk);
			}
		}
	}
}

bool Collada::_optimize_skeletons(VisualScene* p_vscene, Node* p_node)
{
	Node* node = p_node;

	if (node->type == Node::TYPE_SKELETON && node->parent &&
		node->parent->type == Node::TYPE_NODE && node->parent->children.size() == 1) {
		// replace parent by this...
		Node* parent = node->parent;

		// i wonder if this is alright.. i think it is since created skeleton (first joint) is
		// already animated by bone..
		node->id = parent->id;
		node->name = parent->name;
		node->xform_list = parent->xform_list;
		node->default_transform = parent->default_transform;

		state.scene_map[node->id] = node;
		node->parent = parent->parent;

		if (parent->parent) {
			Node* gp = parent->parent;
			bool found = false;
			for (int i = 0; i < gp->children.size(); i++) {
				if (gp->children[i] == parent) {
					gp->children.write[i] = node;
					found = true;
					break;
				}
			}
			if (!found) {
				ERR_PRINT("BUG");
			}
		}
		else {
			bool found = false;

			for (int i = 0; i < p_vscene->root_nodes.size(); i++) {
				if (p_vscene->root_nodes[i] == parent) {
					p_vscene->root_nodes.write[i] = node;
					found = true;
					break;
				}
			}
			if (!found) {
				ERR_PRINT("BUG");
			}
		}

		parent->children.clear();
		memdelete(parent);
		return true;
	}

	for (int i = 0; i < node->children.size(); i++) {
		if (_optimize_skeletons(p_vscene, node->children[i])) {
			return false; // stop processing, go up
		}
	}

	return false;
}

bool Collada::_move_geometry_to_skeletons(VisualScene* p_vscene, Node* p_node, List<Node*>* p_mgeom)
{
	// Bind Shape Matrix scales the bones and makes them gigantic, so the matrix then shrinks the
	// model? Solution: apply the Bind Shape Matrix to the VERTICES, and if the object comes scaled,
	// it seems to be left alone!

	if (p_node->type == Node::TYPE_GEOMETRY) {
		NodeGeometry* ng = static_cast<NodeGeometry*>(p_node);
		if (ng->ignore_anim) {
			return false; // already made child of skeleton and processeg
		}

		if (ng->controller && ng->skeletons.size()) {
			String nodeid = ng->skeletons[0];

			ERR_FAIL_COND_V(!state.scene_map.has(nodeid), false); // weird, it should have it...
			NodeJoint* nj = dynamic_cast<NodeJoint*>(state.scene_map[nodeid]);
			ERR_FAIL_NULL_V(nj, false);
			ERR_FAIL_NULL_V(nj->owner, false); // Weird, node should have a skeleton owner.

			NodeSkeleton* sk = nj->owner;

			Node* p = sk->parent;
			bool node_is_parent_of_skeleton = false;

			while (p) {
				if (p == p_node) {
					node_is_parent_of_skeleton = true;
					break;
				}
				p = p->parent; // try again
			}

			ERR_FAIL_COND_V(node_is_parent_of_skeleton, false);

			// this should be correct
			ERR_FAIL_COND_V(!state.skin_controller_data_map.has(ng->source), false);
			SkinControllerData& skin = state.skin_controller_data_map[ng->source];
			Transform3D skel_inv = sk->get_global_transform().affine_inverse();
			p_node->default_transform =
				skel_inv *
				(skin.bind_shape /* p_node->get_global_transform()*/); // i honestly have no idea
																	   // what to do with a previous
																	   // model xform.. most
																	   // exporters ignore it

			// make rests relative to the skeleton (they seem to be always relative to world)
			for (KeyValue<String, Transform3D>& E : skin.bone_rest_map) {
				E.value = skel_inv * E.value; // make the bone rest local to the skeleton
				state.bone_rest_map[E.key] =
					E.value; // make it remember where the bone is globally, now that it's relative
			}

			// but most exporters seem to work only if i do this..
			// p_node->default_transform = p_node->get_global_transform();

			// p_node->default_transform=Transform3D(); //this seems to be correct, because bind
			// shape makes the object local to the skeleton
			p_node->ignore_anim =
				true; // collada may animate this later, if it does, then this is not supported
					  // (redo your original asset and don't animate the base mesh)
			p_node->parent = sk;
			// sk->children.push_back(0,p_node); //avoid INFINITE loop
			p_mgeom->push_back(p_node);
			return true;
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		if (_move_geometry_to_skeletons(p_vscene, p_node->children[i], p_mgeom)) {
			p_node->children.remove_at(i);
			i--;
		}
	}

	return false;
}

void Collada::_find_morph_nodes(VisualScene* p_vscene, Node* p_node)
{
	if (p_node->type == Node::TYPE_GEOMETRY) {
		NodeGeometry* nj = static_cast<NodeGeometry*>(p_node);

		if (nj->controller) {
			String base = nj->source;

			while (!base.is_empty() && !state.mesh_data_map.has(base)) {
				if (state.skin_controller_data_map.has(base)) {
					SkinControllerData& sk = state.skin_controller_data_map[base];
					base = sk.base;
				}
				else if (state.morph_controller_data_map.has(base)) {
					state.morph_ownership_map[base] = nj->id;
					break;
				}
				else {
					ERR_FAIL_MSG("Invalid scene.");
				}
			}
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		_find_morph_nodes(p_vscene, p_node->children[i]);
	}
}

void Collada::_optimize()
{
	for (KeyValue<String, VisualScene>& E : state.visual_scene_map) {
		VisualScene& vs = E.value;
		for (int i = 0; i < vs.root_nodes.size(); i++) {
			_create_skeletons(&vs.root_nodes.write[i]);
		}


	for (int i = 0; i < vs.root_nodes.size(); i++) {
			_merge_skeletons(&vs, vs.root_nodes[i]);
		}

		_merge_skeletons2(&vs);

		for (int i = 0; i < vs.root_nodes.size(); i++) {
			_optimize_skeletons(&vs, vs.root_nodes[i]);
		}

		for (int i = 0; i < vs.root_nodes.size(); i++) {
			List<Node*> mgeom;
			if (_move_geometry_to_skeletons(&vs, vs.root_nodes[i], &mgeom)) {
				vs.root_nodes.remove_at(i);
				i--;
			}

			while (!mgeom.is_empty()) {
				Node* n = mgeom.front()->get();
				n->parent->children.push_back(n);
				mgeom.pop_front();
			}
		}

		for (int i = 0; i < vs.root_nodes.size(); i++) {
			_find_morph_nodes(&vs, vs.root_nodes[i]);
		}
	}
}

int Collada::get_uv_channel(const String& p_name)
{
	if (!channel_map.has(p_name)) {
		ERR_FAIL_COND_V(channel_map.size() == 2, 0);

		channel_map[p_name] = channel_map.size();
	}

	return channel_map[p_name];
}

Error Collada::load(const String& p_path, int p_flags)
{
	Ref<XMLParser> parserr = memnew(XMLParser);
	XMLParser& parser = *parserr.ptr();
	Error err = parser.open(p_path);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot open Collada file '" + p_path + "'.");

	state.local_path = ProjectSettings::localize_path(p_path);
	state.import_flags = p_flags;
	/* Skip headers */
	while ((err = parser.read()) == OK) {
		if (parser.get_node_type() == XMLParser::NODE_ELEMENT) {
			if (parser.get_node_name() == "COLLADA") {
				break;
			}
			else if (!parser.is_empty()) {
				parser.skip_section(); // unknown section, likely headers
			}
		}
	}

	ERR_FAIL_COND_V_MSG(err != OK, ERR_FILE_CORRUPT, "Corrupted Collada file '" + p_path + "'.");

	/* Start loading Collada */

	{
		// version
		String version = parser.get_named_attribute_value("version");
		state.version.major = version.get_slicec('.', 0).to_int();
		state.version.minor = version.get_slicec('.', 1).to_int();
		state.version.rev = version.get_slicec('.', 2).to_int();
		COLLADA_PRINT("Collada VERSION: " + version);
	}

	while ((err = parser.read()) == OK) {
		/* Read all the main sections.. */

		if (parser.get_node_type() != XMLParser::NODE_ELEMENT) {
			continue; // no idea what this may be, but skipping anyway
		}

		String section = parser.get_node_name();

		COLLADA_PRINT("section: " + section);

		if (section == "asset") {
			_parse_asset(parser);

		}
		else if (section.begins_with("library_")) {
			_parse_library(parser);
		}
		else if (section == "scene") {
			_parse_scene(parser);
		}
		else if (!parser.is_empty()) {
			parser.skip_section(); // unknown section, likely headers
		}
	}

	_optimize();
	return OK;
}

void Collada::_parse_effect_material(XMLParser& p_parser, Collada::Effect& r_effect, String& r_id) {}

#ifdef COLLADA_PRINT
#undef COLLADA_PRINT
#endif
#ifdef COLLADA_IMPORT_SCALE_SCENE
#undef COLLADA_IMPORT_SCALE_SCENE
#endif

// ============================================================================
// Merged from editor/import/3d/editor_import_collada.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/templates/rb_set.h"

// --- Inlined from editor/import/3d/editor_import_collada.h ---
#include "editor/import/3d/resource_importer_scene.h"

class EditorSceneFormatImporterCollada : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
};



// --- End editor/import/3d/editor_import_collada.h ---
#include "scene/3d/camera_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/animation.h"
#include "scene/resources/surface_tool.h"

struct ColladaImport
{
	Collada collada;
	Node3D* scene = nullptr;

	Vector<Ref<Animation>> animations;

	struct NodeMap
	{
		// String path;
		Node3D* node = nullptr;
		int bone = -1;
		List<int> anim_tracks;
	};

	bool found_ambient = false;
	Color ambient;
	bool found_directional = false;
	bool force_make_tangents = false;
	bool apply_mesh_xform_to_vertices = true;
	bool use_mesh_builtin_materials = false;
	float bake_fps = 30;

	HashMap<String, NodeMap> node_map;	   // map from collada node to engine node
	HashMap<String, String> node_name_map; // map from collada node to engine node
	HashMap<String, Ref<ImporterMesh>> mesh_cache;
	HashMap<String, Ref<Curve3D>> curve_cache;
	HashMap<String, Ref<Material>> material_cache;
	HashMap<Collada::Node*, Skeleton3D*> skeleton_map;

	HashMap<Skeleton3D*, HashMap<String, int>> skeleton_bone_map;

	HashSet<String> valid_animated_nodes;
	Vector<int> valid_animated_properties;
	HashMap<String, bool> bones_with_animation;

	HashSet<String> mesh_unique_names;
	HashSet<String> material_unique_names;

	Error _populate_skeleton(
		Skeleton3D* p_skeleton, Collada::Node* p_node, int& r_bone, int p_parent);
	Error _create_scene_skeletons(Collada::Node* p_node);
	Error _create_scene(Collada::Node* p_node, Node3D* p_parent);
	Error _create_resources(Collada::Node* p_node, bool p_use_compression);
	Error _create_material(const String& p_target);
	Error _create_mesh_surfaces(bool p_optimize, Ref<ImporterMesh>& p_mesh,
		const HashMap<String, Collada::NodeGeometry::Material>& p_material_map,
		const Collada::MeshData& meshdata, const Transform3D& p_local_xform,
		const Vector<int>& bone_remap, const Collada::SkinControllerData* p_skin_controller,
		const Collada::MorphControllerData* p_morph_data,
		const Vector<Ref<ImporterMesh>>& p_morph_meshes = Vector<Ref<ImporterMesh>>(),
		bool p_use_compression = false, bool p_use_mesh_material = false);
	Error load(const String& p_path, int p_flags, bool p_force_make_tangents = false,
		bool p_use_compression = false);
	void _fix_param_animation_tracks();
	void create_animation(int p_clip, bool p_import_value_tracks);
	void create_animations(bool p_import_value_tracks);

	HashSet<String> tracks_in_clips;
	Vector<String> missing_textures;

	void _pre_process_lights(Collada::Node* p_node);
};

Error ColladaImport::_populate_skeleton(
	Skeleton3D* p_skeleton, Collada::Node* p_node, int& r_bone, int p_parent)
{
	if (p_node->type != Collada::Node::TYPE_JOINT) {
		return OK;
	}

	Collada::NodeJoint* joint = static_cast<Collada::NodeJoint*>(p_node);

	p_skeleton->add_bone(p_node->name);
	if (p_parent >= 0) {
		p_skeleton->set_bone_parent(r_bone, p_parent);
	}

	NodeMap nm;
	nm.node = p_skeleton;
	nm.bone = r_bone;
	node_map[p_node->id] = nm;
	node_name_map[p_node->name] = p_node->id;

	skeleton_bone_map[p_skeleton][joint->sid] = r_bone;

	{
		Transform3D xform = joint->compute_transform(collada);
		xform = collada.fix_transform(xform) * joint->post_transform;

		p_skeleton->set_bone_pose_position(r_bone, xform.origin);
		p_skeleton->set_bone_pose_rotation(r_bone, xform.basis.get_rotation_quaternion());
		p_skeleton->set_bone_pose_scale(r_bone, xform.basis.get_scale());
	}

	if (collada.state.bone_rest_map.has(joint->sid)) {
		p_skeleton->set_bone_rest(
			r_bone, collada.fix_transform(collada.state.bone_rest_map[joint->sid]));
		// should map this bone to something for animation?
	}
	else {
		WARN_PRINT("Collada: Joint has no rest.");
	}

	int id = r_bone++;
	for (int i = 0; i < p_node->children.size(); i++) {
		Error err = _populate_skeleton(p_skeleton, p_node->children[i], r_bone, id);
		if (err) {
			return err;
		}
	}

	return OK;
}

void ColladaImport::_pre_process_lights(Collada::Node* p_node)
{
	if (p_node->type == Collada::Node::TYPE_LIGHT) {
		Collada::NodeLight* light = static_cast<Collada::NodeLight*>(p_node);
		if (collada.state.light_data_map.has(light->light)) {
			Collada::LightData& ld = collada.state.light_data_map[light->light];
			if (ld.mode == Collada::LightData::MODE_AMBIENT) {
				found_ambient = true;
				ambient = ld.color;
			}
			if (ld.mode == Collada::LightData::MODE_DIRECTIONAL) {
				found_directional = true;
			}
		}
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		_pre_process_lights(p_node->children[i]);
	}
}

Error ColladaImport::_create_scene_skeletons(Collada::Node* p_node)
{
	if (p_node->type == Collada::Node::TYPE_SKELETON) {
		Skeleton3D* sk = memnew(Skeleton3D);
		int bone = 0;
		for (int i = 0; i < p_node->children.size(); i++) {
			_populate_skeleton(sk, p_node->children[i], bone, -1);
		}
		sk->localize_rests(); // after creating skeleton, rests must be localized...!
		skeleton_map[p_node] = sk;
	}

	for (int i = 0; i < p_node->children.size(); i++) {
		Error err = _create_scene_skeletons(p_node->children[i]);
		if (err) {
			return err;
		}
	}
	return OK;
}

Error ColladaImport::_create_material(const String& p_target)
{
	ERR_FAIL_COND_V(material_cache.has(p_target), ERR_ALREADY_EXISTS);
	ERR_FAIL_COND_V(!collada.state.material_map.has(p_target), ERR_INVALID_PARAMETER);
	Collada::Material& src_mat = collada.state.material_map[p_target];
	ERR_FAIL_COND_V(!collada.state.effect_map.has(src_mat.instance_effect), ERR_INVALID_PARAMETER);
	Collada::Effect& effect = collada.state.effect_map[src_mat.instance_effect];

	Ref<StandardMaterial3D> material = memnew(StandardMaterial3D);

	String base_name;
	if (!src_mat.name.is_empty()) {
		base_name = src_mat.name;
	}
	else if (!effect.name.is_empty()) {
		base_name = effect.name;
	}
	else {
		base_name = "Material";
	}

	String name = base_name;
	int counter = 2;
	while (material_unique_names.has(name)) {
		name = base_name + itos(counter++);
	}

	material_unique_names.insert(name);

	material->set_name(name);

	// DIFFUSE

	if (!effect.diffuse.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.diffuse.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}
			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
				// material->set_parameter(StandardMaterial3D::PARAM_DIFFUSE,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}
	}

	// SPECULAR

	if (!effect.specular.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.specular.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}

			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_texture(StandardMaterial3D::TEXTURE_METALLIC, texture);
				material->set_specular(1.0);

				// material->set_texture(StandardMaterial3D::PARAM_SPECULAR,texture);
				// material->set_parameter(StandardMaterial3D::PARAM_SPECULAR,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}

	}
	else {
		material->set_metallic(effect.specular.color.get_v());
	}

	// EMISSION

	if (!effect.emission.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.emission.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}

			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
				material->set_texture(StandardMaterial3D::TEXTURE_EMISSION, texture);
				material->set_emission(Color(1, 1, 1, 1));

				// material->set_parameter(StandardMaterial3D::PARAM_EMISSION,Color(1,1,1,1));
			}
			else {
				missing_textures.push_back(texfile.get_file());
			}
		}
	}
	else {
		if (effect.emission.color != Color()) {
			material->set_feature(StandardMaterial3D::FEATURE_EMISSION, true);
			material->set_emission(effect.emission.color);
		}
	}

	// NORMAL

	if (!effect.bump.texture.is_empty()) {
		String texfile = effect.get_texture_path(effect.bump.texture, collada);
		if (!texfile.is_empty()) {
			if (texfile.begins_with("/")) {
				texfile = texfile.replace_first("/", "res://");
			}

			Ref<Texture2D> texture = ResourceLoader::load(texfile, "Texture2D");
			if (texture.is_valid()) {
				material->set_feature(StandardMaterial3D::FEATURE_NORMAL_MAPPING, true);
				material->set_texture(StandardMaterial3D::TEXTURE_NORMAL, texture);
				// material->set_emission(Color(1,1,1,1));

				// material->set_texture(StandardMaterial3D::PARAM_NORMAL,texture);
			}
			else {
				// missing_textures.push_back(texfile.get_file());
			}
		}
	}

	float roughness = (effect.shininess - 1.0) / 510;
	material->set_roughness(roughness);

	if (effect.double_sided) {
		material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	}
	if (effect.unshaded) {
		material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	}

	material_cache[p_target] = material;
	return OK;
}

Error ColladaImport::load(
	const String& p_path, int p_flags, bool p_force_make_tangents, bool p_use_compression)
{
	Error err = collada.load(p_path, p_flags);
	ERR_FAIL_COND_V_MSG(err, err, "Cannot load file '" + p_path + "'.");

	force_make_tangents = p_force_make_tangents;
	ERR_FAIL_COND_V(
		!collada.state.visual_scene_map.has(collada.state.root_visual_scene), ERR_INVALID_DATA);
	Collada::VisualScene& vs = collada.state.visual_scene_map[collada.state.root_visual_scene];

	scene = memnew(Node3D); // root

	// determine what's going on with the lights
	for (int i = 0; i < vs.root_nodes.size(); i++) {
		_pre_process_lights(vs.root_nodes[i]);
	}
	// import scene

	for (int i = 0; i < vs.root_nodes.size(); i++) {
		Error err2 = _create_scene_skeletons(vs.root_nodes[i]);
		if (err2 != OK) {
			memdelete(scene);
			ERR_FAIL_COND_V(err2, err2);
		}
	}

	for (int i = 0; i < vs.root_nodes.size(); i++) {
		Error err2 = _create_scene(vs.root_nodes[i], scene);
		if (err2 != OK) {
			memdelete(scene);
			ERR_FAIL_COND_V(err2, err2);
		}

		Error err3 = _create_resources(vs.root_nodes[i], p_use_compression);
		if (err3 != OK) {
			memdelete(scene);
			ERR_FAIL_COND_V(err3, err3);
		}
	}

	// optatively, set unit scale in the root
	scene->set_transform(collada.get_root_transform());

	return OK;
}

void ColladaImport::_fix_param_animation_tracks()
{
	for (KeyValue<String, Collada::Node*>& E : collada.state.scene_map) {
		Collada::Node* n = E.value;
		switch (n->type) {
		case Collada::Node::TYPE_NODE: {
			// ? do nothing
		} break;
		case Collada::Node::TYPE_JOINT: {
		} break;
		case Collada::Node::TYPE_SKELETON: {
		} break;
		case Collada::Node::TYPE_LIGHT: {
		} break;
		case Collada::Node::TYPE_CAMERA: {
		} break;
		case Collada::Node::TYPE_GEOMETRY: {
			Collada::NodeGeometry* ng = static_cast<Collada::NodeGeometry*>(n);
			// test source(s)
			String source = ng->source;

			while (!source.is_empty()) {
				if (collada.state.skin_controller_data_map.has(source)) {
					const Collada::SkinControllerData& skin =
						collada.state.skin_controller_data_map[source];

					// nothing to animate here i think

					source = skin.base;
				}
				else if (collada.state.morph_controller_data_map.has(source)) {
					const Collada::MorphControllerData& morph =
						collada.state.morph_controller_data_map[source];

					if (morph.targets.has("MORPH_WEIGHT") && morph.targets.has("MORPH_TARGET")) {
						String weights = morph.targets["MORPH_WEIGHT"];
						String targets = morph.targets["MORPH_TARGET"];
						// fails here

						if (morph.sources.has(targets) && morph.sources.has(weights)) {
							const Collada::MorphControllerData::Source& weight_src =
								morph.sources[weights];
							const Collada::MorphControllerData::Source& target_src =
								morph.sources[targets];

							ERR_FAIL_COND(weight_src.array.size() != target_src.sarray.size());

							for (int i = 0; i < weight_src.array.size(); i++) {
								String track_name = weights + "(" + itos(i) + ")";
								String mesh_name = target_src.sarray[i];
								if (collada.state.mesh_name_map.has(mesh_name) &&
									collada.state.referenced_tracks.has(track_name)) {
									const Vector<int>& rt =
										collada.state.referenced_tracks[track_name];

									for (int rti = 0; rti < rt.size(); rti++) {
										Collada::AnimationTrack* at =
											&collada.state.animation_tracks.write[rt[rti]];

										at->target = E.key;
										at->param =
											"morph/" + collada.state.mesh_name_map[mesh_name];
										at->property = true;
										// at->param
									}
								}
							}
						}
					}
					source = morph.mesh;
				}
				else {
					source = ""; // for now nothing else supported
				}
			}

		} break;
		}
	}
}

void ColladaImport::create_animations(bool p_import_value_tracks)
{
	_fix_param_animation_tracks();
	for (int i = 0; i < collada.state.animation_clips.size(); i++) {
		for (int j = 0; j < collada.state.animation_clips[i].tracks.size(); j++) {
			tracks_in_clips.insert(collada.state.animation_clips[i].tracks[j]);
		}
	}

	for (int i = 0; i < collada.state.animation_tracks.size(); i++) {
		const Collada::AnimationTrack& at = collada.state.animation_tracks[i];

		String node;

		if (!node_map.has(at.target)) {
			if (node_name_map.has(at.target)) {
				node = node_name_map[at.target];
			}
			else {
				WARN_PRINT("Collada: Couldn't find node: " + at.target);
				continue;
			}
		}
		else {
			node = at.target;
		}

		if (at.property) {
			valid_animated_properties.push_back(i);

		}
		else {
			node_map[node].anim_tracks.push_back(i);
			valid_animated_nodes.insert(node);
		}
	}

	create_animation(-1, p_import_value_tracks);
	for (int i = 0; i < collada.state.animation_clips.size(); i++) {
		create_animation(i, p_import_value_tracks);
	}
}

void EditorSceneFormatImporterCollada::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("dae");
}

void ColladaImport::create_animation(int p_clip, bool p_bake) {}

Error ColladaImport::_create_scene(Collada::Node* p_node, Node3D* p_parent) { return OK; }

Error ColladaImport::_create_resources(Collada::Node* p_node, bool p_is_root) { return OK; }



// ============================================================================
// Merged from editor/import/3d/resource_importer_obj.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
// --- Inlined from editor/import/3d/resource_importer_obj.h ---
#include "editor/import/3d/resource_importer_scene.h"

class EditorOBJImporter : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
};

class ResourceImporterOBJ : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual int get_format_version() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
};



// --- End editor/import/3d/resource_importer_obj.h ---
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/mesh.h"
#include "scene/resources/surface_tool.h"

static Error _parse_material_library(const String& p_path,
	HashMap<String, Ref<StandardMaterial3D>>& material_map, List<String>* r_missing_deps)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN,
		vformat("Couldn't open MTL file '%s', it may not exist or not be readable.", p_path));

	Ref<StandardMaterial3D> current;
	String current_name;
	String base_path = p_path.get_base_dir();
	while (true) {
		String l = f->get_line().strip_edges();

		if (l.begins_with("newmtl ")) {
			// Start of a new material.
			current_name = l.replace("newmtl", "").strip_edges();
			current.instantiate();
			current->set_name(current_name);
			material_map[current_name] = current;
		}
		else if (l.begins_with("Ka ")) {
			// Ambient color.
			WARN_PRINT("OBJ: Ambient light for material '" + current_name + "' is ignored in PBR");

		}
		else if (l.begins_with("Kd ")) {
			// Diffuse color.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() < 4, ERR_INVALID_DATA);
			Color c = current->get_albedo();
			c.r = v[1].to_float();
			c.g = v[2].to_float();
			c.b = v[3].to_float();
		}
		else if (l.begins_with("Ks ")) {
			// Specular color.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() < 4, ERR_INVALID_DATA);
			float r = v[1].to_float();
			float g = v[2].to_float();
			float b = v[3].to_float();
			float metalness = MAX(r, MAX(g, b));
			current->set_metallic(metalness);
		}
		else if (l.begins_with("Ns ")) {
			// Specular exponent.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() != 2, ERR_INVALID_DATA);
			float s = v[1].to_float();
			current->set_metallic((1000.0 - s) / 1000.0);
		}
		else if (l.begins_with("d ")) {
			// Dissolve (1.0 is fully opaque, 0.0 is completely transparent).
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() != 2, ERR_INVALID_DATA);
			float d = v[1].to_float();
			Color c = current->get_albedo();
			c.a = d;
			if (c.a < 0.99) {
				current->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
			}
		}
		else if (l.begins_with("Tr ")) {
			// Transparency (1.0 is completely transparent, 0.0 is fully opaque).
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);
			Vector<String> v = l.split(" ", false);
			ERR_FAIL_COND_V(v.size() != 2, ERR_INVALID_DATA);
			float d = v[1].to_float();
			Color c = current->get_albedo();
			c.a = 1.0 - d;
			if (c.a < 0.99) {
				current->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
			}

		}
		else if (l.begins_with("map_Ka ")) {
			// Ambient texture map.
			WARN_PRINT(
				"OBJ: Ambient light texture for material '" + current_name + "' is ignored in PBR");

		}
		else if (l.begins_with("map_Kd ")) {
			// Diffuse texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			String p = l.replace("map_Kd", "").replace_char('\\', '/').strip_edges();
			String path;
			if (p.is_absolute_path()) {
				path = p;
			}
			else {
				path = base_path.path_join(p);
			}

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, texture);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}

		}
		else if (l.begins_with("map_Ks ")) {
			// Specular color texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			String p = l.replace("map_Ks", "").replace_char('\\', '/').strip_edges();
			String path;
			if (p.is_absolute_path()) {
				path = p;
			}
			else {
				path = base_path.path_join(p);
			}

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_texture(StandardMaterial3D::TEXTURE_METALLIC, texture);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}

		}
		else if (l.begins_with("map_Ns ")) {
			// Specular exponent texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			String p = l.replace("map_Ns", "").replace_char('\\', '/').strip_edges();
			String path;
			if (p.is_absolute_path()) {
				path = p;
			}
			else {
				path = base_path.path_join(p);
			}

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_texture(StandardMaterial3D::TEXTURE_ROUGHNESS, texture);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}
		}
		else if (l.begins_with("map_bump ") || l.begins_with("map_Bump ")) {
			// Bump texture map.
			ERR_FAIL_COND_V(current.is_null(), ERR_FILE_CORRUPT);

			l = l.begins_with("map_bump ") ? l.trim_prefix("map_bump ")
										   : l.trim_prefix("map_Bump ");
			l = l.strip_edges();

			// Read path and optional bump multiplier.
			String p;
			float bm = 1.0;
			int bm_pos = l.find("-bm ");
			if (bm_pos >= 0) {
				int bm_start = bm_pos + 4;
				int bm_end = l.find_char(' ', bm_start);
				if (bm_end >= 0) {
					bm = l.substr(bm_start, bm_end - bm_start).to_float();
					p = l.substr(bm_end + 1);
				}
				else { // Bump multiplier ends at end of line.
					bm = l.substr(bm_start).to_float();
					p = l.substr(0, bm_pos);
				}
			}
			else {
				p = l;
			}

			String path = base_path.path_join(p.replace_char('\\', '/').strip_edges());

			Ref<Texture2D> texture = ResourceLoader::load(path);

			if (texture.is_valid()) {
				current->set_feature(StandardMaterial3D::FEATURE_NORMAL_MAPPING, true);
				current->set_texture(StandardMaterial3D::TEXTURE_NORMAL, texture);
				current->set_normal_scale(bm);
			}
			else if (r_missing_deps) {
				r_missing_deps->push_back(path);
			}
		}
		else if (f->eof_reached()) {
			break;
		}
	}

	return OK;
}

void EditorOBJImporter::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("obj");
}

////////////////////////////////////////////////////

String ResourceImporterOBJ::get_importer_name() const { return "wavefront_obj"; }

String ResourceImporterOBJ::get_visible_name() const { return "OBJ as Mesh"; }

void ResourceImporterOBJ::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("obj");
}

String ResourceImporterOBJ::get_save_extension() const { return "mesh"; }

String ResourceImporterOBJ::get_resource_type() const { return "Mesh"; }

int ResourceImporterOBJ::get_format_version() const { return 1; }

int ResourceImporterOBJ::get_preset_count() const { return 0; }

String ResourceImporterOBJ::get_preset_name(int p_idx) const { return ""; }

void ResourceImporterOBJ::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}



// ============================================================================
// Merged from editor/import/editor_import_plugin.cpp
// ============================================================================

#include "editor/file_system/editor_file_system.h"
#include "editor/import/3d/resource_importer_scene.h"

bool EditorImportPlugin::can_import_threaded() const
{
	return ResourceImporter::can_import_threaded();
}

String EditorImportPlugin::get_importer_name() const { return ""; }

String EditorImportPlugin::get_visible_name() const { return ""; }

String EditorImportPlugin::get_save_extension() const { return ""; }

String EditorImportPlugin::get_resource_type() const { return ""; }

int EditorImportPlugin::get_preset_count() const { return 0; }

float EditorImportPlugin::get_priority() const { return 1.0; }

int EditorImportPlugin::get_import_order() const { return 0; }

int EditorImportPlugin::get_format_version() const { return 0; }

String EditorImportPlugin::get_preset_name(int p_idx) const { return String(); }

void EditorImportPlugin::get_recognized_extensions(List<String>* r_extensions) const {}

void EditorImportPlugin::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}



// ============================================================================
// Merged from editor/import/editor_atlas_packer.cpp
// ============================================================================
// --- Inlined from editor/import/editor_atlas_packer.h ---
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/templates/vector.h"

class EditorAtlasPacker {
public:
	struct Chart {
		Vector<Vector2> vertices;
		struct Face {
			int vertex[3] = { 0 };
		};
		Vector<Face> faces;
		bool can_transpose = false;

		Vector2 final_offset;
		bool transposed = false;
	};

private:
	struct PlottedBitmap {
		int chart_index = 0;
		Vector2i offset;
		int area = 0;
		Vector<int> top_heights;
		Vector<int> bottom_heights;
		bool transposed = false;

		Vector2 final_pos;

		bool operator<(const PlottedBitmap &p_bm) const {
			return area > p_bm.area;
		}
	};

public:
	static void chart_pack(Vector<Chart> &charts, int &r_width, int &r_height, int p_atlas_max_size = 2048, int p_cell_resolution = 4);
};

// --- End editor/import/editor_atlas_packer.h ---

#include "core/math/geometry_2d.h"
#include "core/math/math_funcs_binary.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "scene/resources/bit_map.h"

void EditorAtlasPacker::chart_pack(Vector<Chart> &charts, int &r_width, int &r_height, int p_atlas_max_size, int p_cell_resolution) {
	int divide_by = MIN(64, p_cell_resolution);
	Vector<PlottedBitmap> bitmaps;

	int max_w = 0;

	for (int i = 0; i < charts.size(); i++) {
		const Chart &chart = charts[i];

		//generate aabb

		Rect2i aabb;
		int vertex_count = chart.vertices.size();
		const Vector2 *vertices = chart.vertices.ptr();

		for (int j = 0; j < vertex_count; j++) {
			if (j == 0) {
				aabb.position = vertices[j];
			} else {
				aabb.expand_to(vertices[j]);
			}
		}

		Ref<BitMap> src_bitmap;
		src_bitmap.instantiate();
		src_bitmap->create((aabb.size + Vector2(divide_by - 1, divide_by - 1)) / divide_by);

		int w = src_bitmap->get_size().width;
		int h = src_bitmap->get_size().height;

		//plot triangles, using divisor

		for (int j = 0; j < chart.faces.size(); j++) {
			Vector2i v[3];
			for (int k = 0; k < 3; k++) {
				Vector2 vtx = chart.vertices[chart.faces[j].vertex[k]];
				vtx -= aabb.position;
				vtx /= divide_by;
				vtx = vtx.min(Vector2(w - 1, h - 1));
				v[k] = vtx;
			}

			for (int k = 0; k < 3; k++) {
				int l = k == 0 ? 2 : k - 1;
				Vector<Point2i> points = Geometry2D::bresenham_line(v[k], v[l]);
				for (Point2i point : points) {
					src_bitmap->set_bitv(point, true);
				}
			}
		}

		//src_bitmap->convert_to_image()->save_png("bitmap" + itos(i) + ".png");

		//grow by 1 for each side

		int bmw = src_bitmap->get_size().width + 2;
		int bmh = src_bitmap->get_size().height + 2;

		int heights_size = -1;
		bool transpose = false;
		if (chart.can_transpose && bmh > bmw) {
			heights_size = bmh;
			transpose = true;
		} else {
			heights_size = bmw;
		}

		max_w = MAX(max_w, heights_size);

		Vector<int> top_heights;
		Vector<int> bottom_heights;
		top_heights.resize(heights_size);
		bottom_heights.resize(heights_size);

		for (int x = 0; x < heights_size; x++) {
			top_heights.write[x] = -1;
			bottom_heights.write[x] = 0x7FFFFFFF;
		}

		for (int x = 0; x < bmw; x++) {
			for (int y = 0; y < bmh; y++) {
				bool found_pixel = false;
				for (int lx = x - 1; lx < x + 2 && !found_pixel; lx++) {
					for (int ly = y - 1; ly < y + 2 && !found_pixel; ly++) {
						int px = lx - 1;
						if (px < 0 || px >= w) {
							continue;
						}
						int py = ly - 1;
						if (py < 0 || py >= h) {
							continue;
						}

						if (src_bitmap->get_bit(px, py)) {
							found_pixel = true;
						}
					}
				}
				if (found_pixel) {
					if (transpose) {
						if (x > top_heights[y]) {
							top_heights.write[y] = x;
						}
						if (x < bottom_heights[y]) {
							bottom_heights.write[y] = x;
						}
					} else {
						if (y > top_heights[x]) {
							top_heights.write[x] = y;
						}
						if (y < bottom_heights[x]) {
							bottom_heights.write[x] = y;
						}
					}
				}
			}
		}

		String row;
		for (int j = 0; j < top_heights.size(); j++) {
			row += "(" + itos(top_heights[j]) + "-" + itos(bottom_heights[j]) + "),";
		}

		PlottedBitmap plotted_bitmap;
		plotted_bitmap.offset = aabb.position;
		plotted_bitmap.top_heights = top_heights;
		plotted_bitmap.bottom_heights = bottom_heights;
		plotted_bitmap.chart_index = i;
		plotted_bitmap.transposed = transpose;
		plotted_bitmap.area = bmw * bmh;

		bitmaps.push_back(plotted_bitmap);
	}

	bitmaps.sort();

	int atlas_max_width = Math::nearest_power_of_2_templated(p_atlas_max_size) / divide_by;
	int atlas_w = Math::nearest_power_of_2_templated(max_w);
	int atlas_h;
	while (true) {
		atlas_h = 0;

		//do a tetris
		Vector<int> heights;
		heights.resize(atlas_w);
		for (int i = 0; i < atlas_w; i++) {
			heights.write[i] = 0;
		}

		int *atlas_ptr = heights.ptrw();

		for (int i = 0; i < bitmaps.size(); i++) {
			int best_height = 0x7FFFFFFF;
			int best_height_offset = -1;
			int w = bitmaps[i].top_heights.size();

			const int *top_heights = bitmaps[i].top_heights.ptr();
			const int *bottom_heights = bitmaps[i].bottom_heights.ptr();

			for (int j = 0; j <= atlas_w - w; j++) {
				int height = 0;

				for (int k = 0; k < w; k++) {
					int pixmap_h = bottom_heights[k];
					if (pixmap_h == 0x7FFFFFFF) {
						continue; //no pixel here, anything is fine
					}

					int h = MAX(0, atlas_ptr[j + k] - pixmap_h);
					if (h > height) {
						height = h;
					}
				}

				if (height < best_height) {
					best_height = height;
					best_height_offset = j;
				}
			}

			for (int j = 0; j < w; j++) { //add
				if (top_heights[j] == -1) { //unused
					continue;
				}
				int height = best_height + top_heights[j] + 1;
				atlas_ptr[j + best_height_offset] = height;
				atlas_h = MAX(atlas_h, height);
			}

			// set
			Vector2 offset = bitmaps[i].offset;
			if (bitmaps[i].transposed) {
				SWAP(offset.x, offset.y);
			}

			Vector2 final_pos = Vector2(best_height_offset * divide_by, best_height * divide_by) + Vector2(divide_by, divide_by) - offset;
			charts.write[bitmaps[i].chart_index].final_offset = final_pos;
			charts.write[bitmaps[i].chart_index].transposed = bitmaps[i].transposed;
		}

		if (atlas_h <= atlas_w * 2 || atlas_w >= atlas_max_width) {
			break; //ok this one is enough
		}

		//try again
		atlas_w *= 2;
	}

	r_width = atlas_w * divide_by;
	r_height = atlas_h * divide_by;
}

// ============================================================================
// Merged from editor/import/resource_importer_csv_translation.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "editor/import/3d/resource_importer_scene.h"

String ResourceImporterCSVTranslation::get_importer_name() const { return "csv_translation"; }

String ResourceImporterCSVTranslation::get_visible_name() const { return "CSV Translation"; }

void ResourceImporterCSVTranslation::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("csv");
}

String ResourceImporterCSVTranslation::get_save_extension() const
{
	return ""; // does not save a single resource
}

String ResourceImporterCSVTranslation::get_resource_type() const { return "Translation"; }

int ResourceImporterCSVTranslation::get_preset_count() const { return 0; }

String ResourceImporterCSVTranslation::get_preset_name(int p_idx) const { return ""; }

void ResourceImporterCSVTranslation::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}



// ============================================================================
// Merged from editor/import/resource_importer_font.cpp
// ============================================================================
// --- Inlined from editor/import/resource_importer_font.h ---
#include "core/io/resource_importer.h"

class ResourceImporterFont final : public ResourceImporter
{
public:
	enum Mode
	{
		MODE_DYNAMIC,
		MODE_BMFONT,
		MODE_IMAGEFONT,
	};

private:
	enum Presets
	{
		PRESET_DYNAMIC,
		PRESET_MSDF,
		PRESET_MAX
	};

	Mode mode = MODE_DYNAMIC;

public:
	explicit ResourceImporterFont(Mode p_mode = MODE_DYNAMIC);

	Mode get_mode() const { return mode; }
	void set_mode(Mode p_mode) { mode = p_mode; }

	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual void get_build_dependencies(
		const String& p_path, HashSet<String>* r_build_dependencies) override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;

	virtual bool can_import_threaded() const override { return true; }
};

// --- End editor/import/resource_importer_font.h ---

#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/font.h"
#include "servers/text/text_server.h"

ResourceImporterFont::ResourceImporterFont(Mode p_mode) :
	mode(p_mode)
{
}

String ResourceImporterFont::get_importer_name() const
{
	switch (mode) {
	case MODE_DYNAMIC:
		return "font_data_dynamic";
	case MODE_BMFONT:
		return "font_data_bmfont";
	case MODE_IMAGEFONT:
		return "font_data_image";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterFont::get_visible_name() const
{
	switch (mode) {
	case MODE_DYNAMIC:
		return "Font Data (Dynamic Font)";
	case MODE_BMFONT:
		return "Font Data (AngelCode BMFont)";
	case MODE_IMAGEFONT:
		return "Font Data (Image Font)";
	}
	ERR_FAIL_V(String());
}

void ResourceImporterFont::get_recognized_extensions(List<String>* p_extensions) const
{
	if (!p_extensions) {
		return;
	}
	switch (mode) {
	case MODE_DYNAMIC: {
		p_extensions->push_back("ttf");
		p_extensions->push_back("ttc");
		p_extensions->push_back("otf");
		p_extensions->push_back("otc");
		p_extensions->push_back("woff");
		p_extensions->push_back("woff2");
		p_extensions->push_back("pfb");
		p_extensions->push_back("pfm");
	} break;
	case MODE_BMFONT: {
		p_extensions->push_back("font");
		p_extensions->push_back("fnt");
	} break;
	case MODE_IMAGEFONT: {
		ImageLoader::get_recognized_extensions(p_extensions);
	} break;
	}
}

String ResourceImporterFont::get_save_extension() const { return "fontdata"; }

String ResourceImporterFont::get_resource_type() const { return "FontFile"; }

void ResourceImporterFont::get_build_dependencies(
	const String& p_path, HashSet<String>* r_dependencies)
{
	if (mode == MODE_DYNAMIC) {
		Ref<FontFile> font = ResourceLoader::load(p_path);
		if (font.is_valid() && font->is_multichannel_signed_distance_field()) {
			r_dependencies->insert("module_msdfgen_enabled");
		}
	}
}

int ResourceImporterFont::get_preset_count() const
{
	if (mode == MODE_DYNAMIC) {
		return PRESET_MAX;
	}
	return 0;
}

String ResourceImporterFont::get_preset_name(int p_idx) const
{
	if (mode == MODE_DYNAMIC) {
		switch (p_idx) {
		case PRESET_DYNAMIC:
			return TTR("Dynamically rendered TrueType/OpenType font");
		case PRESET_MSDF:
			return TTR("Prerendered multichannel(+true) signed distance field");
		default:
			return String();
		}
	}
	return String();
}

bool ResourceImporterFont::has_advanced_options() const { return false; }

void ResourceImporterFont::show_advanced_options(const String& p_path) { (void)p_path; }

void ResourceImporterFont::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
	(void)p_path;
	(void)r_options;
	(void)p_preset;
}

// ============================================================================
// Merged from editor/import/resource_importer_shader_file.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
// --- Inlined from editor/import/resource_importer_shader_file.h ---
#include "core/io/resource_importer.h"

class ResourceImporterShaderFile : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
	virtual bool can_import_threaded() const override { return true; }
};



// --- End editor/import/resource_importer_shader_file.h ---
#include "servers/rendering/rendering_device_binds.h"

String ResourceImporterShaderFile::get_importer_name() const { return "glsl"; }

String ResourceImporterShaderFile::get_visible_name() const { return "GLSL Shader File"; }

void ResourceImporterShaderFile::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("glsl");
}

String ResourceImporterShaderFile::get_save_extension() const { return "res"; }

String ResourceImporterShaderFile::get_resource_type() const { return "RDShaderFile"; }

int ResourceImporterShaderFile::get_preset_count() const { return 0; }

String ResourceImporterShaderFile::get_preset_name(int p_idx) const { return String(); }

void ResourceImporterShaderFile::get_import_options(
	const String& p_path, List<ImportOption>* r_options, int p_preset) const
{
}

static String _include_function(const String& p_path, void* userpointer)
{
	Error err;

	String* base_path = (String*)userpointer;

	String include = p_path;
	if (include.is_relative_path()) {
		include = base_path->path_join(include);
	}

	Ref<FileAccess> file_inc = FileAccess::open(include, FileAccess::READ, &err);
	if (err != OK) {
		return String();
	}
	return file_inc->get_as_utf8_string();
}



// ============================================================================
// Merged from editor/import/resource_importer_texture.cpp
// ============================================================================

#include "editor/import/3d/resource_importer_scene.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/config_file.h"
#include "core/io/image_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry_2d.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/import/atlas_import_failed.xpm"

#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/mesh.h"

void ResourceImporterTexture::_texture_reimport_roughness(const Ref<CompressedTexture2D>& p_tex,
	const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_channel)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(mutex);
	String path = p_tex->get_path();

	if (!make_flags.has(path)) {
		make_flags[path] = MakeInfo();
	}

	make_flags[path].flags |= MAKE_ROUGHNESS_FLAG;
	make_flags[path].channel_for_roughness = p_channel;
	make_flags[path].normal_path_for_roughness = p_normal_path;
}

void ResourceImporterTexture::_texture_reimport_3d(const Ref<CompressedTexture2D>& p_tex)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(mutex);
	String path = p_tex->get_path();

	if (!make_flags.has(path)) {
		make_flags[path] = MakeInfo();
	}

	make_flags[path].flags |= MAKE_3D_FLAG;
}

void ResourceImporterTexture::_texture_reimport_normal(const Ref<CompressedTexture2D>& p_tex)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(mutex);
	String path = p_tex->get_path();

	if (!make_flags.has(path)) {
		make_flags[path] = MakeInfo();
	}

	make_flags[path].flags |= MAKE_NORMAL_FLAG;
}

void ResourceImporterTexture::set_mode(Mode p_mode)
{
	switch (p_mode) {
	case MODE_2D_ARRAY:
		target = TARGET_LAYERED_2D_ARRAY;
		break;
	case MODE_CUBEMAP:
		target = TARGET_CUBEMAP;
		break;
	case MODE_CUBEMAP_ARRAY:
		target = TARGET_CUBEMAP_ARRAY;
		break;
	case MODE_3D:
		target = TARGET_TEXTURE_3D;
		break;
	}
}

ResourceImporterTexture::Mode ResourceImporterTexture::_get_layered_mode() const
{
	switch (target) {
	case TARGET_LAYERED_2D_ARRAY:
		return MODE_2D_ARRAY;
	case TARGET_CUBEMAP:
		return MODE_CUBEMAP;
	case TARGET_CUBEMAP_ARRAY:
		return MODE_CUBEMAP_ARRAY;
	case TARGET_TEXTURE_3D:
		return MODE_3D;
	default:
		return MODE_CUBEMAP;
	}
}

String ResourceImporterTexture::get_importer_name() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "texture";
	case TARGET_LAYERED_2D_ARRAY:
		return "2d_array_texture";
	case TARGET_CUBEMAP:
		return "cubemap_texture";
	case TARGET_CUBEMAP_ARRAY:
		return "cubemap_array_texture";
	case TARGET_TEXTURE_3D:
		return "3d_texture";
	case TARGET_TEXTURE_ATLAS:
		return "texture_atlas";
	case TARGET_IMAGE:
		return "image";
	case TARGET_BITMAP:
		return "bitmap";
	case TARGET_SVG:
		return "svg";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterTexture::get_visible_name() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "Texture2D";
	case TARGET_LAYERED_2D_ARRAY:
		return "Texture2DArray";
	case TARGET_CUBEMAP:
		return "Cubemap";
	case TARGET_CUBEMAP_ARRAY:
		return "CubemapArray";
	case TARGET_TEXTURE_3D:
		return "Texture3D";
	case TARGET_TEXTURE_ATLAS:
		return "TextureAtlas";
	case TARGET_IMAGE:
		return "Image";
	case TARGET_BITMAP:
		return "BitMap";
	case TARGET_SVG:
		return "ImageTexture";
	}
	ERR_FAIL_V(String());
}

void ResourceImporterTexture::get_recognized_extensions(List<String>* p_extensions) const
{
	if (!p_extensions) {
		return;
	}
	if (target == TARGET_SVG) {
		p_extensions->push_back("svg");
		return;
	}
	ImageLoader::get_recognized_extensions(p_extensions);
}

String ResourceImporterTexture::get_save_extension() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "ctex";
	case TARGET_LAYERED_2D_ARRAY:
		return "ctexarray";
	case TARGET_CUBEMAP:
		return "ccube";
	case TARGET_CUBEMAP_ARRAY:
		return "ccubearray";
	case TARGET_TEXTURE_3D:
		return "ctex3d";
	case TARGET_TEXTURE_ATLAS:
		return "res";
	case TARGET_IMAGE:
		return "image";
	case TARGET_BITMAP:
		return "res";
	case TARGET_SVG:
		return "dpitex";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterTexture::get_resource_type() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "CompressedTexture2D";
	case TARGET_LAYERED_2D_ARRAY:
	case TARGET_CUBEMAP:
	case TARGET_CUBEMAP_ARRAY:
		return "CompressedTextureLayered";
	case TARGET_TEXTURE_3D:
		return "CompressedTexture3D";
	case TARGET_TEXTURE_ATLAS:
		return "Texture2D";
	case TARGET_IMAGE:
		return "Image";
	case TARGET_BITMAP:
		return "BitMap";
	case TARGET_SVG:
		return "ImageTexture";
	}
	ERR_FAIL_V(String());
}

int ResourceImporterTexture::get_preset_count() const
{
	if (target == TARGET_TEXTURE_2D) {
		return 3;
	}
	return 0;
}

String ResourceImporterTexture::get_preset_name(int p_idx) const
{
	if (target == TARGET_TEXTURE_2D && p_idx >= 0 && p_idx < 3) {
		static const char* preset_names[] = {
			TTRC("2D/3D (Auto-Detect)"),
			TTRC("2D"),
			TTRC("3D"),
		};

		return TTRGET(preset_names[p_idx]);
	}
	return String();
}

String ResourceImporterTexture::get_option_group_file() const
{
	if (target == TARGET_TEXTURE_ATLAS) {
		return "atlas_file";
	}
	return String();
}

void ResourceImporterTexture::_remap_channels(Ref<Image>& r_image, ChannelRemap p_options[4])
{
	ERR_FAIL_COND(r_image->is_compressed());

	// Currently HDR inverted remapping is not allowed.
	bool attempted_hdr_inverted = false;
	if (r_image->get_format() >= Image::FORMAT_RF &&
		r_image->get_format() <= Image::FORMAT_RGBE9995) {
		// Formats which can hold HDR data cannot be inverted the same way as unsigned normalized
		// ones (1.0 - channel).
		for (int i = 0; i < 4; i++) {
			switch (p_options[i]) {
			case REMAP_INV_R:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_R;
				break;
			case REMAP_INV_G:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_G;
				break;
			case REMAP_INV_B:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_B;
				break;
			case REMAP_INV_A:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_A;
				break;
			default:
				break;
			}
		}
	}

	if (attempted_hdr_inverted) {
		WARN_PRINT("Attempted to use an inverted channel remap on an HDR image. The remap has been "
				   "changed to its uninverted equivalent.");
	}

	// Optimization: Set the remap from 'unused' to either 0 or 1 to avoid repeated checks in the
	// conversion loop.
	for (int i = 0; i < 4; i++) {
		if (p_options[i] == REMAP_UNUSED) {
			p_options[i] = i == 3 ? REMAP_1 : REMAP_0;
		}
	}

	// Expand the image's channel count in the event that the current set of channels doesn't allow
	// for the desired remap.
	const Image::Format original_format = r_image->get_format();
	const uint32_t channel_mask = Image::get_format_component_mask(original_format);

	// Whether a channel is supported by the format itself.
	const bool has_channel_r = channel_mask & 0x1;
	const bool has_channel_g = channel_mask & 0x2;
	const bool has_channel_b = channel_mask & 0x4;
	const bool has_channel_a = channel_mask & 0x8;

	// Whether a certain channel needs to be remapped.
	const bool remap_r =
		p_options[0] != REMAP_R ? !(!has_channel_r && p_options[0] == REMAP_0) : false;
	const bool remap_g =
		p_options[1] != REMAP_G ? !(!has_channel_g && p_options[1] == REMAP_0) : false;
	const bool remap_b =
		p_options[2] != REMAP_B ? !(!has_channel_b && p_options[2] == REMAP_0) : false;
	const bool remap_a =
		p_options[3] != REMAP_A ? !(!has_channel_a && p_options[3] == REMAP_1) : false;

	if (!(remap_r || remap_g || remap_b || remap_a)) {
		// Default color map, do nothing.
		return;
	}

	// Whether a certain channel set is needed, either from the source or the remap.
	const bool needs_rg = remap_g || has_channel_g;
	const bool needs_rgb = remap_b || has_channel_b;
	const bool needs_rgba = remap_a || has_channel_a;

	bool could_not_expand = false;
	switch (original_format) {
	case Image::FORMAT_R8:
	case Image::FORMAT_RG8:
	case Image::FORMAT_RGB8: {
		// Convert to either RGBA8, RGB8 or RG8.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBA8);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGB8);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RG8);
		}
	} break;
	case Image::FORMAT_RH:
	case Image::FORMAT_RGH:
	case Image::FORMAT_RGBH: {
		// Convert to either RGBAH, RGBH or RGH.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBAH);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGBH);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RGH);
		}
	} break;
	case Image::FORMAT_RF:
	case Image::FORMAT_RGF:
	case Image::FORMAT_RGBF: {
		// Convert to either RGBAF, RGBF or RGF.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBAF);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGBF);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RGF);
		}
	} break;
	case Image::FORMAT_L8: {
		const bool uniform_rgb = (p_options[0] == p_options[1] && p_options[1] == p_options[2]) ||
								 !(remap_r || remap_g || remap_b);
		if (uniform_rgb) {
			// Uniform RGB.
			if (needs_rgba) {
				r_image->convert(Image::FORMAT_LA8);
			}
		}
		else {
			// Non-uniform RGB.
			if (needs_rgba) {
				r_image->convert(Image::FORMAT_RGBA8);
			}
			else {
				r_image->convert(Image::FORMAT_RGB8);
			}
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_LA8: {
		const bool uniform_rgb = (p_options[0] == p_options[1] && p_options[1] == p_options[2]) ||
								 !(remap_r || remap_g || remap_b);
		if (!uniform_rgb) {
			// Non-uniform RGB.
			r_image->convert(Image::FORMAT_RGBA8);
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_RGB565: {
		if (needs_rgba) {
			// RGB565 doesn't have an alpha expansion, convert to RGBA8.
			r_image->convert(Image::FORMAT_RGBA8);
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_RGBE9995: {
		if (needs_rgba) {
			// RGB9995 doesn't have an alpha expansion, convert to RGBAH.
			r_image->convert(Image::FORMAT_RGBAH);
			could_not_expand = true;
		}
	} break;

	default: {
	} break;
	}

	if (could_not_expand) {
		WARN_PRINT(vformat("Unable to expand image format %s's channels (the target format does "
						   "not exist), converting to %s as a fallback.",
			Image::get_format_name(original_format),
			Image::get_format_name(r_image->get_format())));
	}

	// Remap the channels.
	for (int x = 0; x < r_image->get_width(); x++) {
		for (int y = 0; y < r_image->get_height(); y++) {
			Color src = r_image->get_pixel(x, y);
			Color dst;

			for (int i = 0; i < 4; i++) {
				switch (p_options[i]) {
				case REMAP_R:
					dst[i] = src.r;
					break;
				case REMAP_G:
					dst[i] = src.g;
					break;
				case REMAP_B:
					dst[i] = src.b;
					break;
				case REMAP_A:
					dst[i] = src.a;
					break;

				case REMAP_INV_R:
					dst[i] = 1.0f - src.r;
					break;
				case REMAP_INV_G:
					dst[i] = 1.0f - src.g;
					break;
				case REMAP_INV_B:
					dst[i] = 1.0f - src.b;
					break;
				case REMAP_INV_A:
					dst[i] = 1.0f - src.a;
					break;

				case REMAP_0:
					dst[i] = 0.0f;
					break;
				case REMAP_1:
					dst[i] = 1.0f;
					break;

				default:
					break;
				}
			}

			r_image->set_pixel(x, y, dst);
		}
	}
}

void ResourceImporterTexture::_invert_y_channel(Ref<Image>& r_image)
{
	// Inverting the green channel can be used to flip a normal map's direction.
	// There's no standard when it comes to normal map Y direction, so this is
	// sometimes needed when using a normal map exported from another program.
	// See <http://wiki.polycount.com/wiki/Normal_Map_Technical_Details#Common_Swizzle_Coordinates>.
	const int height = r_image->get_height();
	const int width = r_image->get_width();

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			const Color color = r_image->get_pixel(i, j);
			r_image->set_pixel(i, j, Color(color.r, 1 - color.g, color.b, color.a));
		}
	}
}

void ResourceImporterTexture::_clamp_hdr_exposure(Ref<Image>& r_image)
{
	// Clamp HDR exposure following Filament's tonemapping formula.
	// This can be used to reduce fireflies in environment maps or reduce the influence
	// of the sun from an HDRI panorama on environment lighting (when a DirectionalLight3D is used
	// instead).
	const int height = r_image->get_height();
	const int width = r_image->get_width();

	// These values are chosen arbitrarily and seem to produce good results with 4,096 samples.
	const float linear = 4096.0;
	const float compressed = 16384.0;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			const Color color = r_image->get_pixel(i, j);
			const float luma = color.get_luminance();

			Color clamped_color;
			if (luma <= linear) {
				clamped_color = color;
			}
			else {
				clamped_color =
					(color / luma) * ((linear * linear - compressed * luma) /
										 (2 * linear - compressed - luma));
			}

			r_image->set_pixel(i, j, clamped_color);
		}
	}
}

void ResourceImporterTexture::_save_tex(Vector<Ref<Image>> p_images, const String& p_to_path,
	int p_compress_mode, float p_lossy, const Image::BasisUniversalPackerParams& p_basisu_params,
	Image::CompressMode p_vram_compression, Image::CompressProfile p_vram_compression_profile,
	Image::CompressSource p_csource, Image::UsedChannels used_channels, bool p_mipmaps,
	bool p_force_po2, Image::BPTCFormat p_bptc_format)
{
	Vector<Ref<Image>> mipmap_images; // for 3D
	Mode layered_mode = _get_layered_mode();

	if (layered_mode == MODE_3D) {
		// 3D saves in its own way

		for (int i = 0; i < p_images.size(); i++) {
			if (p_images.write[i]->has_mipmaps()) {
				p_images.write[i]->clear_mipmaps();
			}

			if (p_force_po2) {
				p_images.write[i]->resize_to_po2();
			}
		}

		if (p_mipmaps) {
			Vector<Ref<Image>> parent_images = p_images;
			// create 3D mipmaps, this is horrible, though not used very often
			int w = p_images[0]->get_width();
			int h = p_images[0]->get_height();
			int d = p_images.size();

			while (w > 1 || h > 1 || d > 1) {
				Vector<Ref<Image>> mipmaps;
				int mm_w = MAX(1, w >> 1);
				int mm_h = MAX(1, h >> 1);
				int mm_d = MAX(1, d >> 1);

				for (int i = 0; i < mm_d; i++) {
					Ref<Image> mm =
						Image::create_empty(mm_w, mm_h, false, p_images[0]->get_format());
					Vector3 pos;
					pos.z = float(i) * float(d) / float(mm_d) + 0.5;
					for (int x = 0; x < mm_w; x++) {
						for (int y = 0; y < mm_h; y++) {
							pos.x = float(x) * float(w) / float(mm_w) + 0.5;
							pos.y = float(y) * float(h) / float(mm_h) + 0.5;

							Vector3i posi = Vector3i(pos);
							Vector3 fract = pos - Vector3(posi);
							Vector3i posi_n = posi;
							if (posi_n.x < w - 1) {
								posi_n.x++;
							}
							if (posi_n.y < h - 1) {
								posi_n.y++;
							}
							if (posi_n.z < d - 1) {
								posi_n.z++;
							}

							Color c000 = parent_images[posi.z]->get_pixel(posi.x, posi.y);
							Color c100 = parent_images[posi.z]->get_pixel(posi_n.x, posi.y);
							Color c010 = parent_images[posi.z]->get_pixel(posi.x, posi_n.y);
							Color c110 = parent_images[posi.z]->get_pixel(posi_n.x, posi_n.y);
							Color c001 = parent_images[posi_n.z]->get_pixel(posi.x, posi.y);
							Color c101 = parent_images[posi_n.z]->get_pixel(posi_n.x, posi.y);
							Color c011 = parent_images[posi_n.z]->get_pixel(posi.x, posi_n.y);
							Color c111 = parent_images[posi_n.z]->get_pixel(posi_n.x, posi_n.y);

							Color cx00 = c000.lerp(c100, fract.x);
							Color cx01 = c001.lerp(c101, fract.x);
							Color cx10 = c010.lerp(c110, fract.x);
							Color cx11 = c011.lerp(c111, fract.x);

							Color cy0 = cx00.lerp(cx10, fract.y);
							Color cy1 = cx01.lerp(cx11, fract.y);

							Color cz = cy0.lerp(cy1, fract.z);

							mm->set_pixel(x, y, cz);
						}
					}

					mipmaps.push_back(mm);
				}

				w = mm_w;
				h = mm_h;
				d = mm_d;

				mipmap_images.append_array(mipmaps);
				parent_images = mipmaps;
			}
		}
	}
	else {
		for (int i = 0; i < p_images.size(); i++) {
			if (p_force_po2) {
				p_images.write[i]->resize_to_po2();
			}

			if (p_mipmaps) {
				p_images.write[i]->generate_mipmaps(p_csource == Image::COMPRESS_SOURCE_NORMAL);
			}
			else {
				p_images.write[i]->clear_mipmaps();
			}
		}
	}

	Ref<FileAccess> f = FileAccess::open(p_to_path, FileAccess::WRITE);
	f->store_8('G');
	f->store_8('S');
	f->store_8('T');
	f->store_8('L');

	f->store_32(CompressedTextureLayered::FORMAT_VERSION);
	f->store_32(p_images.size()); // For 2d layers or 3d depth.
	f->store_32(layered_mode);
	f->store_32(0);

	f->store_32(0);
	f->store_32(mipmap_images.size()); // Adjust the amount of mipmaps.
	f->store_32(0);
	f->store_32(0);

	if ((p_compress_mode == COMPRESS_LOSSLESS || p_compress_mode == COMPRESS_LOSSY) &&
		p_images[0]->get_format() >= Image::FORMAT_RF) {
		p_compress_mode = COMPRESS_VRAM_UNCOMPRESSED; // These can't go as lossy.
	}

	for (int i = 0; i < p_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, p_images[i],
			ResourceImporterTexture::CompressMode(p_compress_mode), used_channels,
			p_vram_compression, p_vram_compression_profile, p_lossy, p_basisu_params,
			p_bptc_format);
	}

	for (int i = 0; i < mipmap_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, mipmap_images[i],
			ResourceImporterTexture::CompressMode(p_compress_mode), used_channels,
			p_vram_compression, p_vram_compression_profile, p_lossy, p_basisu_params,
			p_bptc_format);
	}
}

// FIXME: Rasterization has issues, see
// https://github.com/godotengine/godot/issues/68350#issuecomment-1305610290
[[maybe_unused]] static void _plot_triangle(Vector2i* p_vertices, const Vector2i& p_offset,
	bool p_transposed, Ref<Image> p_image, const Ref<Image>& p_src_image)
{
	int width = p_image->get_width();
	int height = p_image->get_height();
	int src_width = p_src_image->get_width();
	int src_height = p_src_image->get_height();

	int x[3];
	int y[3];

	for (int j = 0; j < 3; j++) {
		x[j] = p_vertices[j].x;
		y[j] = p_vertices[j].y;
	}

	// sort the points vertically
	if (y[1] > y[2]) {
		SWAP(x[1], x[2]);
		SWAP(y[1], y[2]);
	}
	if (y[0] > y[1]) {
		SWAP(x[0], x[1]);
		SWAP(y[0], y[1]);
	}
	if (y[1] > y[2]) {
		SWAP(x[1], x[2]);
		SWAP(y[1], y[2]);
	}

	double dx_far = double(x[2] - x[0]) / (y[2] - y[0] + 1);
	double dx_upper = double(x[1] - x[0]) / (y[1] - y[0] + 1);
	double dx_low = double(x[2] - x[1]) / (y[2] - y[1] + 1);
	double xf = x[0];
	double xt = x[0] + dx_upper; // if y[0] == y[1], special case
	int max_y = MIN(y[2], p_transposed ? (width - p_offset.x - 1) : (height - p_offset.y - 1));
	for (int yi = y[0]; yi < max_y; yi++) {
		if (yi >= 0) {
			for (int xi = (xf > 0 ? int(xf) : 0); xi < (xt <= src_width ? xt : src_width); xi++) {
				int px = xi, py = yi;
				int sx = px, sy = py;
				sx = CLAMP(sx, 0, src_width - 1);
				sy = CLAMP(sy, 0, src_height - 1);
				Color color = p_src_image->get_pixel(sx, sy);
				if (p_transposed) {
					SWAP(px, py);
				}
				px += p_offset.x;
				py += p_offset.y;

				// may have been cropped, so don't blit what is not visible?
				if (px < 0 || px >= width) {
					continue;
				}
				if (py < 0 || py >= height) {
					continue;
				}
				p_image->set_pixel(px, py, color);
			}

			for (int xi = (xf < src_width ? int(xf) : src_width - 1); xi >= (xt > 0 ? xt : 0);
				 xi--) {
				int px = xi, py = yi;
				int sx = px, sy = py;
				sx = CLAMP(sx, 0, src_width - 1);
				sy = CLAMP(sy, 0, src_height - 1);
				Color color = p_src_image->get_pixel(sx, sy);
				if (p_transposed) {
					SWAP(px, py);
				}
				px += p_offset.x;
				py += p_offset.y;

				// may have been cropped, so don't blit what is not visible?
				if (px < 0 || px >= width) {
					continue;
				}
				if (py < 0 || py >= height) {
					continue;
				}
				p_image->set_pixel(px, py, color);
			}
		}
		xf += dx_far;
		if (yi < y[1]) {
			xt += dx_upper;
		}
		else {
			xt += dx_low;
		}
	}
}

const char* ResourceImporterTexture::compression_formats[] = {"s3tc_bptc", "etc2_astc", nullptr};

ResourceImporterTexture::ResourceImporterTexture(bool p_singleton) :
	ResourceImporterTexture(TARGET_TEXTURE_2D, p_singleton)
{
}

ResourceImporterTexture::ResourceImporterTexture(ImportTarget p_target, bool p_singleton) :
	target(p_target)
{
	if (p_singleton) {
		singleton = this;
	}
	if (target == TARGET_TEXTURE_2D) {
		CompressedTexture2D::request_3d_callback = _texture_reimport_3d;
		CompressedTexture2D::request_roughness_callback = _texture_reimport_roughness;
		CompressedTexture2D::request_normal_callback = _texture_reimport_normal;
	}
}

ResourceImporterTexture::~ResourceImporterTexture()
{
	if (singleton == this) {
		singleton = nullptr;
	}
}

void ResourceImporterTexture::save_to_ctex_format(Ref<FileAccess> f, const Ref<Image>& p_image,
	CompressMode p_compress_mode, Image::UsedChannels p_channels,
	Image::CompressMode p_compress_format, Image::CompressProfile p_compress_profile,
	float p_lossy_quality, const Image::BasisUniversalPackerParams& p_basisu_params,
	Image::BPTCFormat p_bptc_format)
{
}

void ResourceImporterTexture::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
	(void)p_path;
	(void)r_options;
	(void)p_preset;
}

String ResourceImporterTexture::get_import_settings_string() const { return String(); }

// ============================================================================
// Merged from editor/import/resource_importer_wav.cpp
// ============================================================================

#include "core/io/resource_saver.h"
// --- Inlined from editor/import/resource_importer_wav.h ---
#include "core/io/resource_importer.h"

class ResourceImporterWAV : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	virtual bool can_import_threaded() const override { return true; }
};



// --- End editor/import/resource_importer_wav.h ---
#include "scene/resources/audio_stream_wav.h"

String ResourceImporterWAV::get_importer_name() const { return "wav"; }

String ResourceImporterWAV::get_visible_name() const { return "Microsoft WAV"; }

void ResourceImporterWAV::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("wav");
}

String ResourceImporterWAV::get_save_extension() const { return "sample"; }

String ResourceImporterWAV::get_resource_type() const { return "AudioStreamWAV"; }

int ResourceImporterWAV::get_preset_count() const { return 0; }

String ResourceImporterWAV::get_preset_name(int p_idx) const { return String(); }

void ResourceImporterWAV::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}
