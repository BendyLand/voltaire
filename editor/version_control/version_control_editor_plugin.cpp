/**************************************************************************/
/*  version_control_editor_plugin.cpp                                     */
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
#include "core/io/resource_loader.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_bottom_panel.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/version_control/editor_vcs_interface.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/separator.h"
#include "version_control_editor_plugin.h"

#define CHECK_PLUGIN_INITIALIZED()                                                                 \
	ERR_FAIL_NULL_MSG(EditorVCSInterface::get_singleton(),                                         \
		"No VCS plugin is initialized. Select a Version Control Plugin from Project menu.");

VersionControlEditorPlugin* VersionControlEditorPlugin::singleton = nullptr;

void VersionControlEditorPlugin::_create_vcs_metadata_files()
{
	String dir = "res://";
	EditorVCSInterface::create_vcs_metadata_files(
		EditorVCSInterface::VCSMetadata(metadata_selection->get_selected_id()), dir);
}

void VersionControlEditorPlugin::_populate_available_vcs_names()
{
	set_up_choice->clear();
	for (const StringName& available_plugin : available_plugins) {
		set_up_choice->add_item(available_plugin);
	}
}

VersionControlEditorPlugin* VersionControlEditorPlugin::get_singleton()
{
	return singleton ? singleton : memnew(VersionControlEditorPlugin);
}

void VersionControlEditorPlugin::popup_vcs_set_up_dialog(const Control* p_gui_base)
{
	fetch_available_vcs_plugin_names();
	if (!available_plugins.is_empty()) {
		Size2 popup_size = Size2(400, 100);
		Size2 window_size = p_gui_base->get_viewport_rect().size;
		popup_size = popup_size.min(window_size * 0.5);

		_populate_available_vcs_names();

		set_up_dialog->popup_centered_clamped(popup_size * EDSCALE);
	}
	else {
		// TODO: Give info to user on how to fix this error.
		EditorNode::get_singleton()->show_warning(
			TTR("No VCS plugins are available in the project. Install a VCS plugin to use VCS "
				"integration features."),
			TTR("Error"));
	}
}

String VersionControlEditorPlugin::_get_date_string_from(
	int64_t p_unix_timestamp, int64_t p_offset_minutes) const
{
	return vformat("%s %s",
		Time::get_singleton()->get_datetime_string_from_unix_time(
			p_unix_timestamp + p_offset_minutes * 60, true),
		Time::get_singleton()->get_offset_string_from_offset_minutes(p_offset_minutes));
}

void VersionControlEditorPlugin::_set_commit_list_size(int p_index) { _refresh_commit_list(); }

void VersionControlEditorPlugin::_toggle_amend_commit(bool p_toggled)
{
	if (p_toggled) {
		previous_commit_message = commit_message->get_text();
		commit_message->set_text(amend_commit_message);
	}
	else {
		commit_message->set_text(previous_commit_message);
		previous_commit_message = "";
	}
	_update_commit_button();
}

void VersionControlEditorPlugin::_remote_selected(int p_index) { _refresh_remote_list(); }

void VersionControlEditorPlugin::_ssh_public_key_selected(const String& p_path)
{
	set_up_ssh_public_key_path->set_text(p_path);
}

void VersionControlEditorPlugin::_ssh_private_key_selected(const String& p_path)
{
	set_up_ssh_private_key_path->set_text(p_path);
}

void VersionControlEditorPlugin::_create_branch()
{
	CHECK_PLUGIN_INITIALIZED();

	String new_branch_name = branch_create_name_input->get_text().strip_edges();

	EditorVCSInterface::get_singleton()->create_branch(new_branch_name);
	EditorVCSInterface::get_singleton()->checkout_branch(new_branch_name);

	branch_create_name_input->clear();
	_refresh_branch_list();
}

void VersionControlEditorPlugin::_create_remote()
{
	CHECK_PLUGIN_INITIALIZED();

	String new_remote_name = remote_create_name_input->get_text().strip_edges();
	String new_remote_url = remote_create_url_input->get_text().strip_edges();

	EditorVCSInterface::get_singleton()->create_remote(new_remote_name, new_remote_url);

	remote_create_name_input->clear();
	remote_create_url_input->clear();
	_refresh_remote_list();
}

int VersionControlEditorPlugin::_get_item_count(Tree* p_tree)
{
	if (!p_tree->get_root()) {
		return 0;
	}
	return p_tree->get_root()->get_children().size();
}

void VersionControlEditorPlugin::_clear_diff()
{
	diff->clear();
	diff_content.clear();
	diff_title->set_text("");
}

void VersionControlEditorPlugin::_remove_branch()
{
	CHECK_PLUGIN_INITIALIZED();

	EditorVCSInterface::get_singleton()->remove_branch(branch_to_remove);
	branch_to_remove.clear();

	_refresh_branch_list();
}

void VersionControlEditorPlugin::_remove_remote()
{
	CHECK_PLUGIN_INITIALIZED();

	EditorVCSInterface::get_singleton()->remove_remote(remote_to_remove);
	remote_to_remove.clear();

	_refresh_remote_list();
}

void VersionControlEditorPlugin::_extra_option_selected(int p_index)
{
	CHECK_PLUGIN_INITIALIZED();

	switch ((ExtraOption)p_index) {
	case EXTRA_OPTION_FORCE_PUSH:
		_force_push();
		break;
	}
}

bool VersionControlEditorPlugin::_is_staging_area_empty()
{
	return staged_files->get_root()->get_child_count() == 0;
}

void VersionControlEditorPlugin::_toggle_vcs_integration(bool p_toggled)
{
	if (p_toggled) {
		_initialize_vcs();
	}
	else {
		shut_down();
	}
}

void VersionControlEditorPlugin::fetch_available_vcs_plugin_names() { available_plugins.clear(); }

void VersionControlEditorPlugin::register_editor()
{
	EditorDockManager::get_singleton()->add_dock(version_commit_dock);
	EditorDockManager::get_singleton()->add_dock(version_control_dock);

	_set_vcs_ui_state(true);
}

VersionControlEditorPlugin::~VersionControlEditorPlugin()
{
	shut_down();
	memdelete(version_commit_dock);
	memdelete(version_control_dock);
	memdelete(version_control_actions);
}



void VersionControlEditorPlugin::_refresh_remote_list() {}

void VersionControlEditorPlugin::shut_down() {}

void VersionControlEditorPlugin::_refresh_branch_list() {}

VersionControlEditorPlugin::VersionControlEditorPlugin() {}

void VersionControlEditorPlugin::_update_commit_button() {}

void VersionControlEditorPlugin::_set_vcs_ui_state(bool) {}

void VersionControlEditorPlugin::_refresh_commit_list() {}

void VersionControlEditorPlugin::_initialize_vcs() {}

void VersionControlEditorPlugin::_force_push() {}
