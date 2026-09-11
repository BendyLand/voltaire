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

void VersionControlEditorPlugin::_bind_methods() {}

void VersionControlEditorPlugin::_create_vcs_metadata_files()
{
	String dir = "res://";
	EditorVCSInterface::create_vcs_metadata_files(
		EditorVCSInterface::VCSMetadata(metadata_selection->get_selected_id()), dir);
}

void VersionControlEditorPlugin::_update_theme()
{
	change_type_to_color[EditorVCSInterface::CHANGE_TYPE_NEW] =
		EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("success_color"), EditorStringName(Editor));
	change_type_to_color[EditorVCSInterface::CHANGE_TYPE_MODIFIED] =
		EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("warning_color"), EditorStringName(Editor));
	change_type_to_color[EditorVCSInterface::CHANGE_TYPE_RENAMED] =
		EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("warning_color"), EditorStringName(Editor));
	change_type_to_color[EditorVCSInterface::CHANGE_TYPE_DELETED] =
		EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("error_color"), EditorStringName(Editor));
	change_type_to_color[EditorVCSInterface::CHANGE_TYPE_TYPECHANGE] =
		EditorNode::get_singleton()->get_editor_theme()->get_color(
			SceneStringName(font_color), EditorStringName(Editor));
	change_type_to_color[EditorVCSInterface::CHANGE_TYPE_UNMERGED] =
		EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("warning_color"), EditorStringName(Editor));

	change_type_to_icon[EditorVCSInterface::CHANGE_TYPE_NEW] =
		EditorNode::get_singleton()->get_editor_theme()->get_icon(
			SNAME("StatusSuccess"), EditorStringName(EditorIcons));
	change_type_to_icon[EditorVCSInterface::CHANGE_TYPE_MODIFIED] =
		EditorNode::get_singleton()->get_editor_theme()->get_icon(
			SNAME("StatusWarning"), EditorStringName(EditorIcons));
	change_type_to_icon[EditorVCSInterface::CHANGE_TYPE_RENAMED] =
		EditorNode::get_singleton()->get_editor_theme()->get_icon(
			SNAME("StatusWarning"), EditorStringName(EditorIcons));
	change_type_to_icon[EditorVCSInterface::CHANGE_TYPE_TYPECHANGE] =
		EditorNode::get_singleton()->get_editor_theme()->get_icon(
			SNAME("StatusWarning"), EditorStringName(EditorIcons));
	change_type_to_icon[EditorVCSInterface::CHANGE_TYPE_DELETED] =
		EditorNode::get_singleton()->get_editor_theme()->get_icon(
			SNAME("StatusError"), EditorStringName(EditorIcons));
	change_type_to_icon[EditorVCSInterface::CHANGE_TYPE_UNMERGED] =
		EditorNode::get_singleton()->get_editor_theme()->get_icon(
			SNAME("StatusWarning"), EditorStringName(EditorIcons));

	select_public_path_button->set_button_icon(
		EditorNode::get_singleton()->get_gui_base()->get_editor_theme_icon("Folder"));
	select_private_path_button->set_button_icon(
		EditorNode::get_singleton()->get_gui_base()->get_editor_theme_icon("Folder"));
	refresh_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("Reload"), EditorStringName(EditorIcons)));
	discard_all_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("Close"), EditorStringName(EditorIcons)));
	stage_all_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("MoveDown"), EditorStringName(EditorIcons)));
	unstage_all_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("MoveUp"), EditorStringName(EditorIcons)));
	fetch_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("Reload"), EditorStringName(EditorIcons)));
	pull_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("MoveDown"), EditorStringName(EditorIcons)));
	push_button->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("MoveUp"), EditorStringName(EditorIcons)));
	extra_options->set_button_icon(EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("GuiTabMenuHl"), EditorStringName(EditorIcons)));

	if (EditorVCSInterface::get_singleton()) {
		_refresh_stage_area();
	}
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

void VersionControlEditorPlugin::popup_vcs_metadata_dialog() { metadata_dialog->popup_centered(); }

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

void VersionControlEditorPlugin::_set_vcs_ui_state(bool p_enabled)
{
	set_up_dialog->get_ok_button()->set_disabled(!p_enabled);
	set_up_choice->set_disabled(p_enabled);
	toggle_vcs_choice->set_pressed_no_signal(p_enabled);
}

void VersionControlEditorPlugin::_update_set_up_warning(const String& p_new_text)
{
	bool empty_settings = set_up_username->get_text().strip_edges().is_empty() &&
						  set_up_password->get_text().is_empty() &&
						  set_up_ssh_public_key_path->get_text().strip_edges().is_empty() &&
						  set_up_ssh_private_key_path->get_text().strip_edges().is_empty() &&
						  set_up_ssh_passphrase->get_text().is_empty();

	if (empty_settings) {
		set_up_warning_text->add_theme_color_override(
			SceneStringName(font_color), EditorNode::get_singleton()->get_editor_theme()->get_color(
											 SNAME("warning_color"), EditorStringName(Editor)));
		set_up_warning_text->set_text(
			TTR("Remote settings are empty. VCS features that use the network may not work."));
	}
	else {
		set_up_warning_text->set_text("");
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

void VersionControlEditorPlugin::_commit()
{
	CHECK_PLUGIN_INITIALIZED();

	String msg = commit_message->get_text().strip_edges();

	ERR_FAIL_COND_MSG(msg.is_empty(), "No commit message was provided.");

	EditorVCSInterface::get_singleton()->commit(msg, toggle_amend_commit->is_pressed());

	if (version_control_dock->get_current_layout() == EditorDock::DOCK_LAYOUT_HORIZONTAL) {
		version_control_dock->hide();
	}

	commit_message->release_focus();
	commit_button->release_focus();
	toggle_amend_commit->set_pressed_no_signal(false);
	commit_message->set_text("");
	previous_commit_message = "";

	_refresh_stage_area();
	_refresh_commit_list();
	_refresh_branch_list();
	_clear_diff();
}

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

void VersionControlEditorPlugin::_branch_item_selected(int p_index)
{
	CHECK_PLUGIN_INITIALIZED();

	String branch_name = branch_select->get_item_text(p_index);
	EditorVCSInterface::get_singleton()->checkout_branch(branch_name);

	EditorFileSystem::get_singleton()->scan_changes();
	ScriptEditor::get_singleton()->reload_scripts();

	_refresh_branch_list();
	_refresh_commit_list();
	_refresh_stage_area();
	_clear_diff();

	_update_opened_tabs();
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

void VersionControlEditorPlugin::_update_branch_create_button(const String& p_new_text)
{
	branch_create_ok->set_disabled(p_new_text.strip_edges().is_empty());
}

void VersionControlEditorPlugin::_update_remote_create_button(const String& p_new_text)
{
	remote_create_ok->set_disabled(p_new_text.strip_edges().is_empty());
}

int VersionControlEditorPlugin::_get_item_count(Tree* p_tree)
{
	if (!p_tree->get_root()) {
		return 0;
	}
	return p_tree->get_root()->get_children().size();
}

void VersionControlEditorPlugin::_discard_file(
	const String& p_file_path, EditorVCSInterface::ChangeType p_change)
{
	CHECK_PLUGIN_INITIALIZED();

	if (p_change == EditorVCSInterface::CHANGE_TYPE_NEW) {
		Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		dir->remove(p_file_path);
	}
	else {
		CHECK_PLUGIN_INITIALIZED();
		EditorVCSInterface::get_singleton()->discard_file(p_file_path);
	}
	// FIXIT: The project.godot file shows weird behavior
	EditorFileSystem::get_singleton()->update_file(p_file_path);
}

void VersionControlEditorPlugin::_update_opened_tabs()
{
	Vector<EditorData::EditedScene> open_scenes = EditorNode::get_editor_data().get_edited_scenes();
	for (int i = 0; i < open_scenes.size(); i++) {
		if (open_scenes[i].root == nullptr) {
			continue;
		}
		EditorNode::get_singleton()->reload_scene(open_scenes[i].path);
	}
}

void VersionControlEditorPlugin::_clear_diff()
{
	diff->clear();
	diff_content.clear();
	diff_title->set_text("");
}

void VersionControlEditorPlugin::_display_diff_split_view(
	List<EditorVCSInterface::DiffLine>& p_diff_content)
{
	LocalVector<EditorVCSInterface::DiffLine> parsed_diff;

	for (EditorVCSInterface::DiffLine diff_line : p_diff_content) {
		String line = diff_line.content.strip_edges(false, true);

		if (diff_line.new_line_no >= 0 && diff_line.old_line_no >= 0) {
			diff_line.new_text = line;
			diff_line.old_text = line;
			parsed_diff.push_back(diff_line);
		}
		else if (diff_line.new_line_no == -1) {
			diff_line.new_text = "";
			diff_line.old_text = line;
			parsed_diff.push_back(diff_line);
		}
		else if (diff_line.old_line_no == -1) {
			int32_t j = parsed_diff.size() - 1;
			while (j >= 0 && parsed_diff[j].new_line_no == -1) {
				j--;
			}

			if (j == (int32_t)parsed_diff.size() - 1) {
				// no lines are modified
				diff_line.new_text = line;
				diff_line.old_text = "";
				parsed_diff.push_back(diff_line);
			}
			else {
				// lines are modified
				EditorVCSInterface::DiffLine modified_line = parsed_diff[j + 1];
				modified_line.new_text = line;
				modified_line.new_line_no = diff_line.new_line_no;
				parsed_diff[j + 1] = modified_line;
			}
		}
	}

	diff->push_table(6);
	/*
		[cell]Old Line No[/cell]
		[cell]prefix[/cell]
		[cell]Old Code[/cell]

		[cell]New Line No[/cell]
		[cell]prefix[/cell]
		[cell]New Line[/cell]
	*/

	diff->set_table_column_expand(2, true);
	diff->set_table_column_expand(5, true);

	for (uint32_t i = 0; i < parsed_diff.size(); i++) {
		EditorVCSInterface::DiffLine diff_line = parsed_diff[i];

		bool has_change = diff_line.status != " ";
		static const Color red = EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("error_color"), EditorStringName(Editor));
		static const Color green = EditorNode::get_singleton()->get_editor_theme()->get_color(
			SNAME("success_color"), EditorStringName(Editor));
		static const Color white = EditorNode::get_singleton()->get_editor_theme()->get_color(
									   SceneStringName(font_color), SNAME("Label")) *
								   Color(1, 1, 1, 0.6);

		if (diff_line.old_line_no >= 0) {
			diff->push_cell();
			diff->push_color(has_change ? red : white);
			diff->add_text(String::num_int64(diff_line.old_line_no));
			diff->pop();
			diff->pop();

			diff->push_cell();
			diff->push_color(has_change ? red : white);
			diff->add_text(has_change ? "-|" : " |");
			diff->pop();
			diff->pop();

			diff->push_cell();
			diff->push_color(has_change ? red : white);
			diff->add_text(diff_line.old_text);
			diff->pop();
			diff->pop();

		}
		else {
			diff->push_cell();
			diff->pop();

			diff->push_cell();
			diff->pop();

			diff->push_cell();
			diff->pop();
		}

		if (diff_line.new_line_no >= 0) {
			diff->push_cell();
			diff->push_color(has_change ? green : white);
			diff->add_text(String::num_int64(diff_line.new_line_no));
			diff->pop();
			diff->pop();

			diff->push_cell();
			diff->push_color(has_change ? green : white);
			diff->add_text(has_change ? "+|" : " |");
			diff->pop();
			diff->pop();

			diff->push_cell();
			diff->push_color(has_change ? green : white);
			diff->add_text(diff_line.new_text);
			diff->pop();
			diff->pop();
		}
		else {
			diff->push_cell();
			diff->pop();

			diff->push_cell();
			diff->pop();

			diff->push_cell();
			diff->pop();
		}
	}
	diff->pop();
}

void VersionControlEditorPlugin::_display_diff_unified_view(
	List<EditorVCSInterface::DiffLine>& p_diff_content)
{
	diff->push_table(4);
	diff->set_table_column_expand(3, true);

	/*
		[cell]Old Line No[/cell]
		[cell]New Line No[/cell]
		[cell]status[/cell]
		[cell]code[/cell]
	*/
	for (const EditorVCSInterface::DiffLine& diff_line : p_diff_content) {
		String line = diff_line.content.strip_edges(false, true);

		Color color;
		if (diff_line.status == "+") {
			color = EditorNode::get_singleton()->get_editor_theme()->get_color(
				SNAME("success_color"), EditorStringName(Editor));
		}
		else if (diff_line.status == "-") {
			color = EditorNode::get_singleton()->get_editor_theme()->get_color(
				SNAME("error_color"), EditorStringName(Editor));
		}
		else {
			color = EditorNode::get_singleton()->get_editor_theme()->get_color(
				SceneStringName(font_color), SNAME("Label"));
			color *= Color(1, 1, 1, 0.6);
		}

		diff->push_cell();
		diff->push_color(color);
		diff->push_indent(1);
		diff->add_text(diff_line.old_line_no >= 0 ? String::num_int64(diff_line.old_line_no) : "");
		diff->pop();
		diff->pop();
		diff->pop();

		diff->push_cell();
		diff->push_color(color);
		diff->push_indent(1);
		diff->add_text(diff_line.new_line_no >= 0 ? String::num_int64(diff_line.new_line_no) : "");
		diff->pop();
		diff->pop();
		diff->pop();

		diff->push_cell();
		diff->push_color(color);
		diff->add_text(diff_line.status != "" ? diff_line.status + "|" : " |");
		diff->pop();
		diff->pop();

		diff->push_cell();
		diff->push_color(color);
		diff->add_text(line);
		diff->pop();
		diff->pop();
	}

	diff->pop();
}

void VersionControlEditorPlugin::_update_commit_button()
{
	commit_button->set_disabled(commit_message->get_text().strip_edges().is_empty());
	if (toggle_amend_commit->is_pressed()) {
		commit_button->set_text(TTR("Amend Commit Changes"));
	}
	else {
		commit_button->set_text(TTR("Commit Changes"));
	}
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
	case EXTRA_OPTION_CREATE_BRANCH:
		branch_create_confirm->popup_centered();
		break;
	case EXTRA_OPTION_CREATE_REMOTE:
		remote_create_confirm->popup_centered();
		break;
	}
}

void VersionControlEditorPlugin::_popup_branch_remove_confirm(int p_index)
{
	branch_to_remove = extra_options_remove_branch_list->get_item_text(p_index);

	branch_remove_confirm->set_text(
		vformat(TTR("Do you want to remove the %s branch?"), branch_to_remove));
	branch_remove_confirm->popup_centered();
}

void VersionControlEditorPlugin::_popup_remote_remove_confirm(int p_index)
{
	remote_to_remove = extra_options_remove_remote_list->get_item_text(p_index);

	remote_remove_confirm->set_text(
		vformat(TTR("Do you want to remove the %s remote?"), branch_to_remove));
	remote_remove_confirm->popup_centered();
}

void VersionControlEditorPlugin::_update_extra_options()
{
	extra_options_remove_branch_list->clear();
	for (int i = 0; i < branch_select->get_item_count(); i++) {
		extra_options_remove_branch_list->add_icon_item(
			EditorNode::get_singleton()->get_editor_theme()->get_icon(
				SNAME("VcsBranches"), EditorStringName(EditorIcons)),
			branch_select->get_item_text(branch_select->get_item_id(i)));
	}
	extra_options_remove_branch_list->update_canvas_items();

	extra_options_remove_remote_list->clear();
	for (int i = 0; i < remote_select->get_item_count(); i++) {
		extra_options_remove_remote_list->add_icon_item(
			EditorNode::get_singleton()->get_editor_theme()->get_icon(
				SNAME("ArrowUp"), EditorStringName(EditorIcons)),
			remote_select->get_item_text(remote_select->get_item_id(i)));
	}
	extra_options_remove_remote_list->update_canvas_items();
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


