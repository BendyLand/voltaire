/**************************************************************************/
/*  project_dialog.cpp                                                    */
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
#include "core/io/dir_access.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "core/version.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_icons.h"
#include "editor/themes/editor_scale.h"
#include "editor/version_control/editor_vcs_interface.h"
#include "project_dialog.h"
#include "scene/gui/check_box.h"
#include "scene/gui/check_button.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/link_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/texture_rect.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"

static bool is_zip_file(Ref<DirAccess> p_d, const String& p_path)
{
	return p_path.get_extension() == "zip" && p_d->file_exists(p_path);
}

String ProjectDialog::_get_target_path()
{
	if (mode == MODE_NEW || mode == MODE_INSTALL || mode == MODE_DUPLICATE) {
		return project_path->get_text();
	}
	else if (mode == MODE_IMPORT) {
		return install_path->get_text();
	}
	else {
		ERR_FAIL_V("");
	}
}

void ProjectDialog::_set_target_path(const String& p_text)
{
	if (mode == MODE_NEW || mode == MODE_INSTALL || mode == MODE_DUPLICATE) {
		project_path->set_text(p_text);
	}
	else if (mode == MODE_IMPORT) {
		install_path->set_text(p_text);
	}
	else {
		ERR_FAIL();
	}
}

<<<<<<< HEAD
void ProjectDialog::_create_dir_toggled(bool p_pressed)
{
	String target_path = _get_target_path();

	if (create_dir->is_pressed()) {
		// (Re-)append target dir name.
		if (last_custom_target_dir.is_empty()) {
			target_path = target_path.path_join(auto_dir);
		}
		else {
			target_path = target_path.path_join(last_custom_target_dir);
		}
	}
	else {
		// Strip any trailing slash.
		target_path = target_path.rstrip("/\\");
		// Save and remove target dir name.
		if (target_path.get_file() == auto_dir) {
			last_custom_target_dir = "";
		}
		else {
			last_custom_target_dir = target_path.get_file();
		}
		target_path = target_path.get_base_dir();
	}

	_set_target_path(target_path);
	_validate_path();
}

void ProjectDialog::_project_name_changed()
{
	if (mode == MODE_NEW || mode == MODE_INSTALL || mode == MODE_DUPLICATE) {
		_update_target_auto_dir();
	}

	_validate_path();
}

void ProjectDialog::_project_path_changed()
{
	if (mode == MODE_IMPORT) {
		_update_target_auto_dir();
	}

	_validate_path();
}

void ProjectDialog::_install_path_changed() { _validate_path(); }

void ProjectDialog::_project_path_selected(const String& p_path)
{
	show_dialog(false);

	if (create_dir->is_pressed() &&
		(mode == MODE_NEW || mode == MODE_INSTALL || mode == MODE_DUPLICATE)) {
		// Replace parent directory, but keep target dir name.
		project_path->set_text(p_path.path_join(project_path->get_text().get_file()));
	}
	else {
		project_path->set_text(p_path);
	}

	_project_path_changed();

	if (install_path->is_visible_in_tree()) {
		// ZIP is selected; focus install path.
		install_path->grab_focus();
	}
	else {
		get_ok_button()->grab_focus();
	}
}

void ProjectDialog::_install_path_selected(const String& p_path)
{
	ERR_FAIL_COND_MSG(mode != MODE_IMPORT, "Install path is only used for MODE_IMPORT.");

	if (create_dir->is_pressed()) {
		// Replace parent directory, but keep target dir name.
		install_path->set_text(p_path.path_join(install_path->get_text().get_file()));
	}
	else {
		install_path->set_text(p_path);
	}

	_install_path_changed();

	get_ok_button()->grab_focus();
}

=======
>>>>>>> fix/remove-object
void ProjectDialog::_reset_name() { project_name->set_text(TTR("New Game Project")); }

void ProjectDialog::_nonempty_confirmation_ok_pressed()
{
	is_folder_empty = true;
	ok_pressed();
}

void ProjectDialog::set_zip_path(const String& p_path) { zip_path = p_path; }

void ProjectDialog::set_zip_title(const String& p_title) { zip_title = p_title; }

void ProjectDialog::set_original_project_path(const String& p_path)
{
	original_project_path = p_path;
}

void ProjectDialog::set_duplicate_can_edit(bool p_duplicate_can_edit)
{
	duplicate_can_edit = p_duplicate_can_edit;
}

void ProjectDialog::set_mode(Mode p_mode) { mode = p_mode; }

void ProjectDialog::set_project_name(const String& p_name) { project_name->set_text(p_name); }

void ProjectDialog::set_project_path(const String& p_path) { project_path->set_text(p_path); }

void ProjectDialog::ask_for_path_and_show()
{
	_reset_name();
	_browse_project_path();
}

<<<<<<< HEAD
=======
void ProjectDialog::show_dialog(bool p_reset_name, bool p_is_confirmed) {}

>>>>>>> fix/remove-object


void ProjectDialog::_browse_project_path() {}
