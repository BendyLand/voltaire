/**************************************************************************/
/*  fbx_importer_manager.cpp                                              */
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
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "fbx_importer_manager.h"
#include "scene/gui/link_button.h"

void FBXImporterManager::_validate_path(const String& p_path)
{
	String error;
	bool success = false;

	if (p_path == "") {
		error = TTR("Path to FBX2glTF executable is empty.");
	}
	else if (!FileAccess::exists(p_path)) {
		error = TTR("Path to FBX2glTF executable is invalid.");
	}
	else {
		List<String> args;
		args.push_back("--version");
		int exitcode;
		Error err = OS::get_singleton()->execute(p_path, args, nullptr, &exitcode);

		if (err == OK && exitcode == 0) {
			success = true;
		}
		else {
			error = TTR("Error executing this file (wrong version or architecture).");
		}
	}

	if (success) {
		path_status->set_text(TTR("FBX2glTF executable is valid."));
		path_status->add_theme_color_override(SceneStringName(font_color),
			path_status->get_theme_color(SNAME("success_color"), EditorStringName(Editor)));
		get_ok_button()->set_disabled(false);
	}
	else {
		path_status->set_text(error);
		path_status->add_theme_color_override(SceneStringName(font_color),
			path_status->get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		get_ok_button()->set_disabled(true);
	}
}

void FBXImporterManager::_select_file(const String& p_path)
{
	fbx_path->set_text(p_path);
	_validate_path(p_path);
}

void FBXImporterManager::_browse_install()
{
	if (fbx_path->get_text() != String()) {
		browse_dialog->set_current_file(fbx_path->get_text());
	}

	browse_dialog->popup_centered_ratio();
}

FBXImporterManager* FBXImporterManager::singleton = nullptr;


