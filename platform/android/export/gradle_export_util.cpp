/**************************************************************************/
/*  gradle_export_util.cpp                                                */
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

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_plugin.h"
#include "gradle_export_util.h"
#include "modules/regex/regex.h"

int _get_android_orientation_value(DisplayServerEnums::ScreenOrientation screen_orientation)
{
	switch (screen_orientation) {
	case DisplayServerEnums::SCREEN_PORTRAIT:
		return 1;
	case DisplayServerEnums::SCREEN_REVERSE_LANDSCAPE:
		return 8;
	case DisplayServerEnums::SCREEN_REVERSE_PORTRAIT:
		return 9;
	case DisplayServerEnums::SCREEN_SENSOR_LANDSCAPE:
		return 11;
	case DisplayServerEnums::SCREEN_SENSOR_PORTRAIT:
		return 12;
	case DisplayServerEnums::SCREEN_SENSOR:
		return 13;
	case DisplayServerEnums::SCREEN_LANDSCAPE:
	default:
		return 0;
	}
}

String _get_android_orientation_label(DisplayServerEnums::ScreenOrientation screen_orientation)
{
	switch (screen_orientation) {
	case DisplayServerEnums::SCREEN_PORTRAIT:
		return "portrait";
	case DisplayServerEnums::SCREEN_REVERSE_LANDSCAPE:
		return "reverseLandscape";
	case DisplayServerEnums::SCREEN_REVERSE_PORTRAIT:
		return "reversePortrait";
	case DisplayServerEnums::SCREEN_SENSOR_LANDSCAPE:
		return "userLandscape";
	case DisplayServerEnums::SCREEN_SENSOR_PORTRAIT:
		return "userPortrait";
	case DisplayServerEnums::SCREEN_SENSOR:
		return "fullUser";
	case DisplayServerEnums::SCREEN_LANDSCAPE:
	default:
		return "landscape";
	}
}

int _get_app_category_value(int category_index)
{
	switch (category_index) {
	case APP_CATEGORY_ACCESSIBILITY:
		return 8;
	case APP_CATEGORY_AUDIO:
		return 1;
	case APP_CATEGORY_IMAGE:
		return 3;
	case APP_CATEGORY_MAPS:
		return 6;
	case APP_CATEGORY_NEWS:
		return 5;
	case APP_CATEGORY_PRODUCTIVITY:
		return 7;
	case APP_CATEGORY_SOCIAL:
		return 4;
	case APP_CATEGORY_UNDEFINED:
		return -1;
	case APP_CATEGORY_VIDEO:
		return 2;
	case APP_CATEGORY_GAME:
	default:
		return 0;
	}
}

String _get_app_category_label(int category_index)
{
	switch (category_index) {
	case APP_CATEGORY_ACCESSIBILITY:
		return "accessibility";
	case APP_CATEGORY_AUDIO:
		return "audio";
	case APP_CATEGORY_IMAGE:
		return "image";
	case APP_CATEGORY_MAPS:
		return "maps";
	case APP_CATEGORY_NEWS:
		return "news";
	case APP_CATEGORY_PRODUCTIVITY:
		return "productivity";
	case APP_CATEGORY_SOCIAL:
		return "social";
	case APP_CATEGORY_VIDEO:
		return "video";
	case APP_CATEGORY_GAME:
	default:
		return "game";
	}
}

// Utility method used to create a directory.
Error create_directory(const String& p_dir)
{
	if (!DirAccess::exists(p_dir)) {
		Ref<DirAccess> filesystem_da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		ERR_FAIL_COND_V_MSG(
			filesystem_da.is_null(), ERR_CANT_CREATE, "Cannot create directory '" + p_dir + "'.");
		Error err = filesystem_da->make_dir_recursive(p_dir);
		ERR_FAIL_COND_V_MSG(err, ERR_CANT_CREATE, "Cannot create directory '" + p_dir + "'.");
	}
	return OK;
}

// Writes p_data into a file at p_path, creating directories if necessary.
// Note: this will overwrite the file at p_path if it already exists.
Error store_file_at_path(const String& p_path, const Vector<uint8_t>& p_data)
{
	String dir = p_path.get_base_dir();
	Error err = create_directory(dir);
	if (err != OK) {
		return err;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), ERR_CANT_CREATE, "Cannot create file '" + p_path + "'.");
	fa->store_buffer(p_data.ptr(), p_data.size());
	return OK;
}

// Writes string p_data into a file at p_path, creating directories if necessary.
// Note: this will overwrite the file at p_path if it already exists.
Error store_string_at_path(const String& p_path, const String& p_data)
{
	String dir = p_path.get_base_dir();
	Error err = create_directory(dir);
	if (err != OK) {
		if (OS::get_singleton()->is_stdout_verbose()) {
			print_error("Unable to write data into " + p_path);
		}
		return err;
	}
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), ERR_CANT_CREATE, "Cannot create file '" + p_path + "'.");
	fa->store_string(p_data);
	return OK;
}

// Implementation of EditorExportSaveFunction.
// This method will only be called as an input to export_project_files.
// It is used by the export_project_files method to save all the asset files into the gradle
// project. It's functionality mirrors that of the method save_apk_file. This method will be called
// ONLY when gradle build is enabled.
Error rename_and_store_file_in_gradle_project(const Ref<EditorExportPreset>& p_preset,
	void* p_userdata, const String& p_path, const Vector<uint8_t>& p_data, int p_file, int p_total,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta)
{
	CustomExportData* export_data = static_cast<CustomExportData*>(p_userdata);

	const String simplified_path = EditorExportPlatform::simplify_path(p_path);

	Vector<uint8_t> enc_data;
	EditorExportPlatform::SavedData sd;
	Error err = _store_temp_file(simplified_path, p_data, p_enc_in_filters, p_enc_ex_filters, p_key,
		p_seed, p_delta, enc_data, sd);
	if (err != OK) {
		return err;
	}

	String dst_path;
	if (export_data->pd.salt.length() == 32) {
		dst_path = export_data->assets_directory + String("/") +
				   (simplified_path + export_data->pd.salt).sha256_text();
	}
	else {
		dst_path =
			export_data->assets_directory + String("/") + simplified_path.trim_prefix("res://");
	}
	print_verbose("Saving project files from " + simplified_path + " into " + dst_path);
	err = store_file_at_path(dst_path, enc_data);

	export_data->pd.file_ofs.push_back(sd);
	return err;
}

String _android_xml_escape(const String& p_string)
{
	// Android XML requires strings to be both valid XML (`xml_escape()`) but also
	// to escape characters which are valid XML but have special meaning in Android XML.
	// https://developer.android.com/guide/topics/resources/string-resource.html#FormattingAndStyling
	// Note: Didn't handle U+XXXX unicode chars, could be done if needed.
	return p_string.replace("@", "\\@")
		.replace("?", "\\?")
		.replace("'", "\\'")
		.replace("\"", "\\\"")
		.replace("\n", "\\n")
		.replace("\t", "\\t")
		.xml_escape(false);
}

String bool_to_string(bool v) { return v ? "true" : "false"; }

String _get_gles_tag()
{
	return "    <uses-feature android:glEsVersion=\"0x00030000\" android:required=\"true\" />\n";
}

Error _store_temp_file(const String& p_simplified_path, const Vector<uint8_t>& p_data,
	const Vector<String>& p_enc_in_filters, const Vector<String>& p_enc_ex_filters,
	const Vector<uint8_t>& p_key, uint64_t p_seed, bool p_delta, Vector<uint8_t>& r_enc_data,
	EditorExportPlatform::SavedData& r_sd)
{
	Error err = OK;
	Ref<FileAccess> ftmp =
		FileAccess::create_temp(FileAccess::WRITE_READ, "export", "tmp", false, &err);
	if (err != OK) {
		return err;
	}
	r_sd.path_utf8 = p_simplified_path.trim_prefix("res://").utf8();
	r_sd.ofs = 0;
	r_sd.size = p_data.size();
	r_sd.delta = p_delta;
	err = EditorExportPlatform::_encrypt_and_store_data(ftmp, p_simplified_path, p_data,
		p_enc_in_filters, p_enc_ex_filters, p_key, p_seed, r_sd.encrypted);
	if (err != OK) {
		return err;
	}

	r_enc_data.resize(ftmp->get_length());
	ftmp->seek(0);
	ftmp->get_buffer(r_enc_data.ptrw(), r_enc_data.size());
	ftmp.unref();

	// Store MD5 of original file.
	{
		unsigned char hash[16];
		CryptoCore::md5(p_data.ptr(), p_data.size(), hash);
		r_sd.md5.resize(16);
		for (int i = 0; i < 16; i++) {
			r_sd.md5.write[i] = hash[i];
		}
	}
	return OK;
}


