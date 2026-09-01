/**************************************************************************/
/*  editor_export_platform.cpp                                            */
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

#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/delta_encoding.h"
#include "core/io/dir_access.h"
#include "core/io/file_access_encrypted.h"
#include "core/io/file_access_pack.h" // PACK_HEADER_MAGIC, PACK_FORMAT_VERSION
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
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_plugin.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_export_platform.compat.inc"
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

			Error err = PackedData::get_singleton()->add_pack(
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
	PackedData::get_singleton()->clear();

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

Error EditorExportPlatform::_save_pack_file(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	ERR_FAIL_COND_V_MSG(
		p_total < 1, ERR_PARAMETER_RANGE_ERROR, "Must select at least one file to export.");

	PackData* pd = (PackData*)p_userdata;

	const String simplified_path = simplify_path(p_path);

	Ref<FileAccess> ftmp;
	if (pd->use_sparse_pck) {
		ftmp = FileAccess::open(
			pd->path.get_base_dir().path_join(simplified_path.trim_prefix("res://")),
			FileAccess::WRITE);
	}
	else {
		ftmp = pd->f;
	}

	SavedData sd;
	sd.path_utf8 = simplified_path.trim_prefix("res://").utf8();
	sd.ofs = (pd->use_sparse_pck) ? 0 : pd->f->get_position();
	sd.size = p_data.size();
	sd.delta = p_delta;
	Error err = _encrypt_and_store_data(ftmp, simplified_path, p_data, p_enc_in_filters,
		p_enc_ex_filters, p_key, p_seed, sd.encrypted);
	if (err != OK) {
		return err;
	}
	if (!pd->use_sparse_pck) {
		ERR_FAIL_COND_V(
			pd->f->get_position() - sd.ofs < (uint64_t)p_data.size(), ERR_FILE_CANT_WRITE);
	}

	if (!pd->use_sparse_pck) {
		int pad = _get_pad(PCK_PADDING, pd->f->get_position());
		for (int i = 0; i < pad; i++) {
			pd->f->store_8(0);
		}
	}

	// Store MD5 of original file.
	{
		unsigned char hash[16];
		CryptoCore::md5(p_data.ptr(), p_data.size(), hash);
		sd.md5.resize(16);
		for (int i = 0; i < 16; i++) {
			sd.md5.write[i] = hash[i];
		}
	}

	pd->file_ofs.push_back(sd);

	// TRANSLATORS: This is an editor progress label describing the storing of a file.
	if (pd->ep->step(vformat(TTR("Storing File: %s"), p_path), 2 + p_file * 100 / p_total, false)) {
		return ERR_SKIP;
	}

	return OK;
}

Error EditorExportPlatform::_save_pack_patch_file(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	Ref<FileAccess> old_file = PackedData::get_singleton()->try_open_path(
		p_path, _get_script_encryption_key_bytes(p_preset));
	if (old_file.is_null()) {
		return _save_pack_file(p_preset, p_userdata, p_path, p_data, p_file, p_total,
			p_enc_in_filters, p_enc_ex_filters, p_key, p_seed, false);
	}

	Vector<uint8_t> old_data = old_file->get_buffer(old_file->get_length());

	// We can't rely on the MD5 as stored in the PCKs, since delta patches could have made it stale.
	if (p_data == old_data) {
		return OK; // Do nothing if the file hasn't changed.
	}

	if (!p_preset->is_patch_delta_encoding_enabled()) {
		return _save_pack_file(p_preset, p_userdata, p_path, p_data, p_file, p_total,
			p_enc_in_filters, p_enc_ex_filters, p_key, p_seed, false);
	}

	bool delta = false;

	for (const String& filter : p_preset->get_patch_delta_include_filter().split(",", false)) {
		String filter_stripped = filter.strip_edges();
		if (p_path.matchn(filter_stripped) ||
			p_path.trim_prefix("res://").matchn(filter_stripped)) {
			delta = true;
			break;
		}
	}

	for (const String& filter : p_preset->get_patch_delta_exclude_filter().split(",", false)) {
		String filter_stripped = filter.strip_edges();
		if (p_path.matchn(filter_stripped) ||
			p_path.trim_prefix("res://").matchn(filter_stripped)) {
			delta = false;
			break;
		}
	}

	Vector<uint8_t> patch_data = p_data;

	if (delta) {
		Error err = DeltaEncoding::encode_delta(
			old_data, p_data, patch_data, p_preset->get_patch_delta_zstd_level());
		if (err != OK) {
			return err;
		}

		int64_t reduction_bytes = MAX(0, p_data.size() - patch_data.size());
		double reduction_ratio = reduction_bytes / (double)p_data.size();

		if (reduction_ratio >= p_preset->get_patch_delta_min_reduction()) {
			print_verbose(vformat(
				"Used delta encoding for patch of \"%s\", resulting in a patch of %d bytes, which "
				"reduced the size by %.1f%% (%d bytes) compared to the actual file.",
				p_path, patch_data.size(), reduction_ratio * 100, reduction_bytes));
		}
		else {
			print_verbose(vformat("Skipped delta encoding for patch of \"%s\", as it resulted in a "
								  "patch of %d bytes, which only reduced the size by %.1f%% (%d "
								  "bytes) compared to the actual file.",
				p_path, patch_data.size(), reduction_ratio * 100, reduction_bytes));
			patch_data = p_data;
			delta = false;
		}
	}
	else {
		print_verbose(vformat(
			"Skipped delta encoding for patch of \"%s\", due to include/exclude filters.", p_path));
	}

	return _save_pack_file(p_preset, p_userdata, p_path, patch_data, p_file, p_total,
		p_enc_in_filters, p_enc_ex_filters, p_key, p_seed, delta);
}

Error EditorExportPlatform::_save_zip_file(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	ERR_FAIL_COND_V_MSG(
		p_total < 1, ERR_PARAMETER_RANGE_ERROR, "Must select at least one file to export.");

	const String path = simplify_path(p_path).replace_first("res://", "");

	ZipData* zd = (ZipData*)p_userdata;

	zipFile zip = (zipFile)zd->zip;

	zipOpenNewFileInZip(zip, path.utf8().get_data(), nullptr, nullptr, 0, nullptr, 0, nullptr,
		Z_DEFLATED, Z_DEFAULT_COMPRESSION);

	zipWriteInFileInZip(zip, p_data.ptr(), p_data.size());
	zipCloseFileInZip(zip);

	zd->file_count += 1;

	if (zd->ep->step(TTR("Storing File:") + " " + p_path, 2 + p_file * 100 / p_total, false)) {
		return ERR_SKIP;
	}

	return OK;
}

Error EditorExportPlatform::_save_zip_patch_file(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	Ref<FileAccess> old_file = PackedData::get_singleton()->try_open_path(p_path);
	if (old_file.is_valid()) {
		Vector<uint8_t> old_data = old_file->get_buffer(old_file->get_length());

		// We can't rely on the MD5 as stored in the PCKs, since delta patches could have made it
		// stale.
		if (p_data == old_data) {
			return OK; // Do nothing if the file hasn't changed.
		}
	}

	return _save_zip_file(p_preset, p_userdata, p_path, p_data, p_file, p_total, p_enc_in_filters,
		p_enc_ex_filters, p_key, p_seed, p_delta);
}

Ref<Texture2D> EditorExportPlatform::get_option_icon(int p_index) const
{
	Ref<Theme> theme = EditorNode::get_singleton()->get_editor_theme();
	ERR_FAIL_COND_V(theme.is_null(), Ref<Texture2D>());
	return theme->get_icon(SNAME("Play"), EditorStringName(EditorIcons));
}

String EditorExportPlatform::find_export_template(
	const String& template_file_name, String* err) const
{
	String current_version = VLTR_VERSION_FULL_CONFIG;
	String template_path = EditorPaths::get_singleton()
							   ->get_export_templates_dir()
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
	dir = EditorFileSystem::get_singleton()->find_file(p_path, &file_idx);
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
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path,
	uint32_t p_flags, bool p_enabled)
{
	enabled = p_enabled;
	if (!enabled) {
		return;
	}
	HashSet<String> features = p_platform.get_features(p_preset, p_debug);
	Vector<Ref<EditorExportPlugin>> export_plugins =
		EditorExport::get_singleton()->get_export_plugins();
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
		EditorExport::get_singleton()->get_export_plugins();
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
	const String from_env = OS::get_singleton()->get_environment(ENV_SCRIPT_ENCRYPTION_KEY);
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
			OS::DateTime dt = OS::get_singleton()->get_datetime();

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
				0x0314,	  // "version made by", 0x03 - Unix, 0x14 - ZIP specification version 2.0,
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

			OS::DateTime dt = OS::get_singleton()->get_datetime();

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
				0x0314,	  // "version made by", 0x03 - Unix, 0x14 - ZIP specification version 2.0,
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

Error EditorExportPlatform::save_pack(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, Vector<SharedObject>* p_so_files, EditorExportSaveFunction p_save_func,
	EditorExportRemoveFunction p_remove_func, bool p_embed, int64_t* r_embedded_start,
	int64_t* r_embedded_size)
{
	EditorProgress ep("savepack", TTR("Packing"), 102, true);

	if (p_save_func == nullptr) {
		p_save_func = _save_pack_file;
	}

	// Create the temporary export directory if it doesn't exist.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	da->make_dir_recursive(EditorPaths::get_singleton()->get_temp_dir());

	Ref<FileAccess> f;
	int64_t embed_pos = 0;
	if (!p_embed) {
		// Regular output to separate PCK file.
		f = FileAccess::open(p_path, FileAccess::WRITE);
		if (f.is_null()) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"),
				vformat(TTR("Can't open file for writing at path \"%s\"."), p_path));
			return ERR_CANT_CREATE;
		}
	}
	else {
		// Append to executable.
		f = FileAccess::open(p_path, FileAccess::READ_WRITE);
		if (f.is_null()) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"),
				vformat(TTR("Can't open file for reading-writing at path \"%s\"."), p_path));
			return ERR_FILE_CANT_OPEN;
		}

		f->seek_end();
		embed_pos = f->get_position();

		if (r_embedded_start) {
			*r_embedded_start = embed_pos;
		}

		// Ensure embedded PCK starts at a 64-bit multiple
		int pad = _get_pad(8, f->get_position());
		for (int i = 0; i < pad; i++) {
			f->store_8(0);
		}
	}

	int64_t pck_start_pos = f->get_position();
	uint64_t file_base_ofs = 0;
	uint64_t dir_base_ofs = 0;

	_store_header(f, p_preset->get_enc_pck() && p_preset->get_enc_directory(), false, file_base_ofs,
		dir_base_ofs, String());

	// Align for first file.
	int file_padding = _get_pad(PCK_PADDING, f->get_position());
	for (int i = 0; i < file_padding; i++) {
		f->store_8(0);
	}

	uint64_t file_base = f->get_position();
	f->seek(file_base_ofs);
	f->store_64(file_base - pck_start_pos); // Update files base.
	f->seek(file_base);

	// Write files.
	PackData pd;
	pd.ep = &ep;
	pd.f = f;
	pd.so_files = p_so_files;
	pd.path = p_path;

	Error err = export_project_files(
		p_preset, p_debug, p_save_func, p_remove_func, &pd, _pack_add_shared_object);

	if (err != OK) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"), TTR("Failed to export project files."));
		return err;
	}

	if (pd.file_ofs.is_empty()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"), TTR("No files or changes to export."));
		return FAILED;
	}

	pd.file_ofs.sort(); // Do sort, so we can do binary search later (where ?).

	int dir_padding = _get_pad(PCK_PADDING, f->get_position());
	for (int i = 0; i < dir_padding; i++) {
		f->store_8(0);
	}

	// Write directory.
	uint64_t dir_offset = f->get_position();
	f->seek(dir_base_ofs);
	f->store_64(dir_offset - pck_start_pos);
	f->seek(dir_offset);

	Vector<uint8_t> key;
	if (p_preset->get_enc_pck() && p_preset->get_enc_directory()) {
		key = _get_script_encryption_key_bytes(p_preset);
	}

	if (!_encrypt_and_store_directory(f, pd, key, p_preset->get_seed(), file_base)) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"), TTR("Can't create encrypted file."));
		return ERR_CANT_CREATE;
	}

	if (p_embed) {
		// Ensure embedded data ends at a 64-bit multiple.
		uint64_t embed_end = f->get_position() - embed_pos + 12;
		uint64_t pad = _get_pad(8, embed_end);
		for (uint64_t i = 0; i < pad; i++) {
			f->store_8(0);
		}

		uint64_t pck_size = f->get_position() - pck_start_pos;
		f->store_64(pck_size);
		f->store_32(PACK_HEADER_MAGIC);

		if (r_embedded_size) {
			*r_embedded_size = f->get_position() - embed_pos;
		}
	}
	f->close();

	return OK;
}

Error EditorExportPlatform::save_pack_patch(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, Vector<SharedObject>* p_so_files, bool p_embed, int64_t* r_embedded_start,
	int64_t* r_embedded_size)
{
	return save_pack(p_preset, p_debug, p_path, p_so_files, _save_pack_patch_file,
		_remove_pack_file, p_embed, r_embedded_start, r_embedded_size);
}

Error EditorExportPlatform::save_zip(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, Vector<SharedObject>* p_so_files, EditorExportSaveFunction p_save_func)
{
	EditorProgress ep("savezip", TTR("Packing"), 102, true);

	if (p_save_func == nullptr) {
		p_save_func = _save_zip_file;
	}

	String tmppath = EditorPaths::get_singleton()->get_temp_dir().path_join("packtmp");

	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);
	zipFile zip = zipOpen2(tmppath.utf8().get_data(), APPEND_STATUS_CREATE, nullptr, &io);

	ZipData zd;
	zd.ep = &ep;
	zd.zip = zip;
	zd.so_files = p_so_files;

	Error err =
		export_project_files(p_preset, p_debug, p_save_func, nullptr, &zd, _zip_add_shared_object);
	if (err != OK && err != ERR_SKIP) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save ZIP"), TTR("Failed to export project files."));
		zipClose(zip, nullptr);
		return err;
	}

	zipClose(zip, nullptr);

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	if (zd.file_count == 0) {
		da->remove(tmppath);
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save PCK"), TTR("No files or changes to export."));
		return FAILED;
	}

	err = da->rename(tmppath, p_path);
	if (err != OK) {
		da->remove(tmppath);
		add_message(EXPORT_MESSAGE_ERROR, TTR("Save ZIP"),
			vformat(TTR("Failed to move temporary file \"%s\" to \"%s\"."), tmppath, p_path));
		return err;
	}

	return OK;
}

Error EditorExportPlatform::save_zip_patch(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, Vector<SharedObject>* p_so_files)
{
	return save_zip(p_preset, p_debug, p_path, p_so_files, _save_zip_patch_file);
}

Error EditorExportPlatform::export_pack(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, uint32_t p_flags)
{
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	return save_pack(p_preset, p_debug, p_path);
}

Error EditorExportPlatform::export_zip(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, uint32_t p_flags)
{
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	return save_zip(p_preset, p_debug, p_path);
}

Error EditorExportPlatform::export_pack_patch(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, const Vector<String>& p_patches,
	uint32_t p_flags)
{
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	Error err = _load_patches(p_preset, p_patches.is_empty() ? p_preset->get_patches() : p_patches);
	if (err != OK) {
		return err;
	}
	err = save_pack_patch(p_preset, p_debug, p_path);
	_unload_patches();
	return err;
}

Error EditorExportPlatform::export_zip_patch(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, const Vector<String>& p_patches,
	uint32_t p_flags)
{
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags);
	Error err = _load_patches(p_preset, p_patches.is_empty() ? p_preset->get_patches() : p_patches);
	if (err != OK) {
		return err;
	}
	err = save_zip_patch(p_preset, p_debug, p_path);
	_unload_patches();
	return err;
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



void EditorExportPlatform::_bind_methods() {}

void EditorExportPlatform::get_preset_features(
	const Ref<EditorExportPreset>& p_preset, List<String>* r_features) const
{
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

void EditorExportPlatform::get_platform_features(List<String>* r_features) const {}


