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
#include "core/io/delta_encoding.h"
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
#include "core/os/shared_object.h"
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
#include "scene/gui/rich_text_label.h"
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
#include "core/os/shared_object.h"
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
#include "scene/3d/label_3d.h"
#include "scene/3d/sprite_3d.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_shader_container.h"

#ifdef VULKAN_ENABLED
#define Window VulkanX11Window
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#undef Window
#endif

#ifdef D3D12_ENABLED
#include "drivers/d3d12/rendering_shader_container_d3d12.h"
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
