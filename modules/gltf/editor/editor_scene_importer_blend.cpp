/**************************************************************************/
/*  editor_scene_importer_blend.cpp                                       */
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

#include "../gltf_defines.h"
#include "../gltf_document.h"
#include "core/config/project_settings.h"
#include "core/io/resource_importer.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_import_blend_runner.h"
#include "editor_scene_importer_blend.h"
#include "main/main.h"
#include "scene/gui/line_edit.h"
#include "servers/display/display_server.h"

#ifdef WINDOWS_ENABLED
#include <shlwapi.h>
#endif

static bool _get_blender_version(
	const String& p_path, int& r_major, int& r_minor, String* r_err = nullptr)
{
	if (!FileAccess::exists(p_path)) {
		if (r_err) {
			*r_err = TTR("Path does not point to a valid executable.");
		}
		return false;
	}
	List<String> args;
	args.push_back("--version");
	String pipe;
	Error err = OS::get_singleton()->execute(p_path, args, &pipe);
	if (err != OK) {
		if (r_err) {
			*r_err = TTR("Couldn't run Blender executable.");
		}
		return false;
	}
	int bl = pipe.find("Blender ");
	if (bl == -1) {
		if (r_err) {
			*r_err =
				vformat(TTR("Unexpected --version output from Blender executable at: %s."), p_path);
		}
		return false;
	}
	pipe = pipe.substr(bl);
	pipe = pipe.replace_first("Blender ", "");
	int pp = pipe.find_char('.');
	if (pp == -1) {
		if (r_err) {
			*r_err =
				vformat(TTR("Couldn't extract version information from Blender executable at: %s."),
					p_path);
		}
		return false;
	}
	String v = pipe.substr(0, pp);
	r_major = v.to_int();
	if (r_major < 3) {
		if (r_err) {
			*r_err = vformat(TTR("Found Blender version %d.x, which is too old for this importer "
								 "(3.0+ is required)."),
				r_major);
		}
		return false;
	}

	int pp2 = pipe.find_char('.', pp + 1);
	r_minor = pp2 > pp ? pipe.substr(pp + 1, pp2 - pp - 1).to_int() : 0;

	return true;
}

void EditorSceneFormatImporterBlend::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("blend");
}

static bool _test_blender_path(const String& p_path, String* r_err = nullptr)
{
	int major, minor;
	return _get_blender_version(p_path, major, minor, r_err);
}

Vector<String> EditorFileSystemImportFormatSupportQueryBlend::get_file_extensions() const
{
	Vector<String> ret;
	ret.push_back("blend");
	return ret;
}

void EditorFileSystemImportFormatSupportQueryBlend::_validate_path(String p_path)
{
	String error;
	bool success = false;
	if (p_path == "") {
		error = TTR("Path is empty.");
	}
	else {
		if (_test_blender_path(p_path, &error)) {
			success = true;
			if (auto_detected_path == p_path) {
				error = TTR("Path to Blender executable is valid (Autodetected).");
			}
			else {
				error = TTR("Path to Blender executable is valid.");
			}
		}
	}

	path_status->set_text(error);

	if (success) {
		path_status->add_theme_color_override(SceneStringName(font_color),
			path_status->get_theme_color(SNAME("success_color"), EditorStringName(Editor)));
		configure_blender_dialog->get_ok_button()->set_disabled(false);
	}
	else {
		path_status->add_theme_color_override(SceneStringName(font_color),
			path_status->get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		configure_blender_dialog->get_ok_button()->set_disabled(true);
	}
}

bool EditorFileSystemImportFormatSupportQueryBlend::_autodetect_path()
{
	// Autodetect
	auto_detected_path = "";

#if defined(MACOS_ENABLED)
	Vector<String> find_paths = {
		"/opt/homebrew/bin/blender",
		"/opt/local/bin/blender",
		"/usr/local/bin/blender",
		"/usr/local/opt/blender",
		"/Applications/Blender.app/Contents/MacOS/Blender",
	};
	{
		List<String> mdfind_args;
		mdfind_args.push_back("kMDItemCFBundleIdentifier=org.blenderfoundation.blender");

		String output;
		Error err = OS::get_singleton()->execute("mdfind", mdfind_args, &output);
		if (err == OK) {
			for (const String& find_path : output.split("\n")) {
				find_paths.push_back(find_path.path_join("Contents/MacOS/Blender"));
			}
		}
	}
#elif defined(WINDOWS_ENABLED)
	Vector<String> find_paths = {
		"C:\\Program Files\\Blender Foundation\\blender.exe",
		"C:\\Program Files (x86)\\Blender Foundation\\blender.exe",
	};
	{
		char blender_opener_path[MAX_PATH];
		DWORD path_len = MAX_PATH;
		HRESULT res = AssocQueryString(
			0, ASSOCSTR_EXECUTABLE, ".blend", "open", blender_opener_path, &path_len);
		if (res == S_OK) {
			find_paths.push_back(
				String(blender_opener_path).get_base_dir().path_join("blender.exe"));
		}
	}

#elif defined(UNIX_ENABLED)
	Vector<String> find_paths = {
		"/usr/bin/blender",
		"/usr/local/bin/blender",
		"/opt/blender/bin/blender",
	};
#endif

	for (const String& find_path : find_paths) {
		if (_test_blender_path(find_path)) {
			auto_detected_path = find_path;
			return true;
		}
	}

	return false;
}

void EditorFileSystemImportFormatSupportQueryBlend::_path_confirmed() { confirmed = true; }

void EditorFileSystemImportFormatSupportQueryBlend::_select_install(String p_path)
{
	blender_path->set_text(p_path);
	_validate_path(p_path);
}

void EditorFileSystemImportFormatSupportQueryBlend::_browse_install()
{
	if (blender_path->get_text() != String()) {
		browse_dialog->set_current_file(blender_path->get_text());
	}

	browse_dialog->popup_centered_ratio();
}

void EditorFileSystemImportFormatSupportQueryBlend::_update_icons()
{
	blender_path_browse->set_button_icon(
		blender_path_browse->get_editor_theme_icon(SNAME("FolderBrowse")));
}


