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

void ProjectDialog::_validate_path()
{
	_set_message("", MESSAGE_SUCCESS, PROJECT_PATH);
	_set_message("", MESSAGE_SUCCESS, INSTALL_PATH);

	if (project_name->get_text().strip_edges().is_empty()) {
		_set_message(TTRC("It would be a good idea to name your project."), MESSAGE_ERROR);
		return;
	}

	Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	String path = project_path->get_text().simplify_path();

	String target_path = path;
	InputType target_path_input_type = PROJECT_PATH;

	if (mode == MODE_IMPORT) {
		if (path.get_file().strip_edges() == "project.godot") {
			path = path.get_base_dir();
			project_path->set_text(path);
		}

		if (is_zip_file(d, path)) {
			zip_path = path;
		}
		else if (is_zip_file(d, path.strip_edges())) {
			zip_path = path.strip_edges();
		}
		else {
			zip_path = "";
		}

		if (!zip_path.is_empty()) {
			target_path = install_path->get_text().simplify_path();
			target_path_input_type = INSTALL_PATH;

			create_dir->show();
			install_path_container->show();

			Ref<FileAccess> io_fa;
			zlib_filefunc_def io = zipio_create_io(&io_fa);

			unzFile pkg = unzOpen2(zip_path.utf8().get_data(), &io);
			if (!pkg) {
				_set_message(
					TTRC("Invalid \".zip\" project file; it is not in ZIP format."), MESSAGE_ERROR);
				unzClose(pkg);
				return;
			}

			int ret = unzGoToFirstFile(pkg);
			while (ret == UNZ_OK) {
				unz_file_info info;
				char fname[16384];
				ret = unzGetCurrentFileInfo(pkg, &info, fname, 16384, nullptr, 0, nullptr, 0);
				ERR_FAIL_COND_MSG(ret != UNZ_OK, "Failed to get current file info.");

				String name = String::utf8(fname);

				// Skip the __MACOSX directory created by macOS's built-in file zipper.
				if (name.begins_with("__MACOSX")) {
					ret = unzGoToNextFile(pkg);
					continue;
				}

				if (name.get_file() == "project.godot") {
					break; // ret == UNZ_OK.
				}

				ret = unzGoToNextFile(pkg);
			}

			if (ret == UNZ_END_OF_LIST_OF_FILE) {
				_set_message(TTRC("Invalid \".zip\" project file; it doesn't contain a "
								  "\"project.godot\" file."),
					MESSAGE_ERROR);
				unzClose(pkg);
				return;
			}

			unzClose(pkg);
		}
		else if (d->dir_exists(path) && d->file_exists(path.path_join("project.godot"))) {
			zip_path = "";

			create_dir->hide();
			install_path_container->hide();

			_set_message(TTRC("Valid project found at path."), MESSAGE_SUCCESS);
		}
		else {
			create_dir->hide();
			install_path_container->hide();

			_set_message(
				TTRC(
					"Please choose a \"project.godot\", a directory with one, or a \".zip\" file."),
				MESSAGE_ERROR);
			return;
		}
	}

	if (target_path.is_relative_path()) {
		_set_message(TTRC("The path specified is invalid."), MESSAGE_ERROR, target_path_input_type);
		return;
	}

	if (target_path.get_file() != OS::get_singleton()->get_safe_dir_name(target_path.get_file())) {
		_set_message(
			TTRC(
				"The directory name specified contains invalid characters or trailing whitespace."),
			MESSAGE_ERROR, target_path_input_type);
		return;
	}

	String working_dir = d->get_current_dir();
	String executable_dir = OS::get_singleton()->get_executable_path().get_base_dir();
	if (target_path == working_dir || target_path == executable_dir) {
		_set_message(
			TTRC("Creating a project at the engine's working directory or executable directory is "
				 "not allowed, as it would prevent the project manager from starting."),
			MESSAGE_ERROR, target_path_input_type);
		return;
	}

	// TODO: The following 5 lines could be simplified if OS.get_user_home_dir() or SYSTEM_DIR_HOME
	// is implemented. See: https://github.com/godotengine/godot-proposals/issues/4851.
#ifdef WINDOWS_ENABLED
	String home_dir = OS::get_singleton()->get_environment("USERPROFILE");
#else
	String home_dir = OS::get_singleton()->get_environment("HOME");
#endif
	String documents_dir = OS::get_singleton()->get_system_dir(OS::SYSTEM_DIR_DOCUMENTS);
	if (target_path == home_dir || target_path == documents_dir) {
		_set_message(TTRC("You cannot save a project at the selected path. Please create a "
						  "subfolder or choose a new path."),
			MESSAGE_ERROR, target_path_input_type);
		return;
	}

	is_folder_empty = true;
	if (mode == MODE_NEW || mode == MODE_INSTALL || mode == MODE_DUPLICATE ||
		(mode == MODE_IMPORT && target_path_input_type == InputType::INSTALL_PATH)) {
		if (create_dir->is_pressed()) {
			if (!d->dir_exists(target_path.get_base_dir())) {
				_set_message(TTRC("The parent directory of the path specified doesn't exist."),
					MESSAGE_ERROR, target_path_input_type);
				return;
			}

			if (d->dir_exists(target_path)) {
				// The path is not necessarily empty here, but we will update the message later if
				// it isn't.
				_set_message(TTRC("The project folder already exists and is empty."),
					MESSAGE_SUCCESS, target_path_input_type);
			}
			else {
				_set_message(TTRC("The project folder will be automatically created."),
					MESSAGE_SUCCESS, target_path_input_type);
			}
		}
		else {
			if (!d->dir_exists(target_path)) {
				_set_message(TTRC("The path specified doesn't exist."), MESSAGE_ERROR,
					target_path_input_type);
				return;
			}

			// The path is not necessarily empty here, but we will update the message later if it
			// isn't.
			_set_message(TTRC("The project folder exists and is empty."), MESSAGE_SUCCESS,
				target_path_input_type);
		}

		// Check if the directory is empty. Not an error, but we want to warn the user.
		if (d->change_dir(target_path) == OK) {
			d->list_dir_begin();
			String n = d->get_next();
			while (!n.is_empty()) {
				if (n[0] != '.') {
					// Allow `.`, `..` (reserved current/parent folder names)
					// and hidden files/folders to be present.
					// For instance, this lets users initialize a Git repository
					// and still be able to create a project in the directory afterwards.
					is_folder_empty = false;
					break;
				}
				n = d->get_next();
			}
			d->list_dir_end();

			if (!is_folder_empty) {
				_set_message(TTRC("The selected path is not empty. Choosing an empty folder is "
								  "highly recommended."),
					MESSAGE_WARNING, target_path_input_type);
			}
		}
	}

	// Check if the target path is a subdirectory of original when duplicating
	if (mode == MODE_DUPLICATE) {
		String base_path = original_project_path;
		String duplicate_target = target_path;

		// Ensure the paths end with a slash
		if (!base_path.ends_with("/")) {
			base_path += "/";
		}

		if (!duplicate_target.ends_with("/")) {
			duplicate_target += "/";
		}

		bool is_subdirectory_or_equal;

		if (d->is_case_sensitive(base_path) || d->is_case_sensitive(duplicate_target)) {
			is_subdirectory_or_equal = duplicate_target.begins_with(base_path);
		}
		else {
			base_path = base_path.to_lower();
			String target_lower = duplicate_target.to_lower();
			is_subdirectory_or_equal = target_lower.begins_with(base_path);
		}

		if (is_subdirectory_or_equal) {
			_set_message(TTRC("Cannot duplicate a project into itself."), MESSAGE_ERROR,
				target_path_input_type);
		}
	}
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


