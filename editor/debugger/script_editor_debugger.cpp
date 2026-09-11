/**************************************************************************/
/*  script_editor_debugger.cpp                                            */
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
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/version.h"
#include "editor/debugger/editor_debugger_plugin.h"
#include "editor/debugger/editor_expression_evaluator.h"
#include "editor/debugger/editor_performance_profiler.h"
#include "editor/debugger/editor_profiler.h"
#include "editor/debugger/editor_visual_profiler.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "main/performance.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tab_container.h"
#include "scene/gui/tree.h"
#include "scene/resources/material.h"
#include "script_editor_debugger.h"
#include "servers/display/display_server.h"

using CameraOverride = EditorDebuggerNode::CameraOverride;

void ScriptEditorDebugger::debug_copy()
{
	String msg = reason->get_text();
	if (msg.is_empty()) {
		return;
	}
	DisplayServer::get_singleton()->clipboard_set(msg);
}

void ScriptEditorDebugger::debug_skip_breakpoints()
{
	skip_breakpoints_value = !skip_breakpoints_value;
	if (skip_breakpoints_value) {
		skip_breakpoints->set_button_icon(get_editor_theme_icon(SNAME("DebugSkipBreakpointsOn")));
	}
	else {
		skip_breakpoints->set_button_icon(get_editor_theme_icon(SNAME("DebugSkipBreakpointsOff")));
	}
}

void ScriptEditorDebugger::debug_ignore_error_breaks()
{
	ignore_error_breaks_value = !ignore_error_breaks_value;
	if (ignore_error_breaks_value) {
		ignore_error_breaks->set_button_icon(
			get_theme_icon(SNAME("NotificationDisabled"), SNAME("EditorIcons")));
	}
	else {
		ignore_error_breaks->set_button_icon(
			get_theme_icon(SNAME("Notification"), SNAME("EditorIcons")));
	}
}

void ScriptEditorDebugger::debug_out()
{
	ERR_FAIL_COND(!is_breaked());
	_clear_execution();
}

void ScriptEditorDebugger::debug_next()
{
	ERR_FAIL_COND(!is_breaked());
	_clear_execution();
}

void ScriptEditorDebugger::debug_step()
{
	ERR_FAIL_COND(!is_breaked());
	_clear_execution();
}

void ScriptEditorDebugger::debug_break()
{
	ERR_FAIL_COND(is_breaked());
	_mute_audio_on_break(true);
}

void ScriptEditorDebugger::debug_continue()
{
	ERR_FAIL_COND(!is_breaked());

	// Allow focus stealing only if we actually run this client for security.
	if (remote_pid && EditorNode::get_singleton()->has_child_process(remote_pid)) {
		DisplayServer::get_singleton()->enable_for_stealing_focus(remote_pid);
	}

	_clear_execution();
	_mute_audio_on_break(false);
}

void ScriptEditorDebugger::update_tabs()
{
	if (error_count == 0 && warning_count == 0) {
		errors_tab->set_name(TTRC("Errors"));
		tabs->set_tab_icon(tabs->get_tab_idx_from_control(errors_tab), Ref<Texture2D>());
	}
	else {
		errors_tab->set_name(TTR("Errors") + " (" + itos(error_count + warning_count) + ")");
		if (error_count >= 1 && warning_count >= 1) {
			tabs->set_tab_icon(tabs->get_tab_idx_from_control(errors_tab),
				get_editor_theme_icon(SNAME("ErrorWarning")));
		}
		else if (error_count >= 1) {
			tabs->set_tab_icon(
				tabs->get_tab_idx_from_control(errors_tab), get_editor_theme_icon(SNAME("Error")));
		}
		else {
			tabs->set_tab_icon(tabs->get_tab_idx_from_control(errors_tab),
				get_editor_theme_icon(SNAME("Warning")));
		}
	}
}

void ScriptEditorDebugger::clear_style()
{
	tabs->remove_theme_style_override(SceneStringName(panel));
}

void ScriptEditorDebugger::_file_selected(const String& p_file)
{
	switch (file_dialog_purpose) {
	case SAVE_MONITORS_CSV: {
		Error err;
		Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);

		if (err != OK) {
			ERR_PRINT("Failed to open " + p_file);
			return;
		}
		Vector<String> line;
		line.resize(Performance::MONITOR_MAX);

		// signatures
		for (int i = 0; i < Performance::MONITOR_MAX; i++) {
			line.write[i] = Performance::get_singleton()->get_monitor_name(Performance::Monitor(i));
		}
		file->store_csv_line(line);

		// values
		Vector<List<float>::Element*> iterators;
		iterators.resize(Performance::MONITOR_MAX);
		bool continue_iteration = false;
		for (int i = 0; i < Performance::MONITOR_MAX; i++) {
			iterators.write[i] =
				performance_profiler
					->get_monitor_data(
						Performance::get_singleton()->get_monitor_name(Performance::Monitor(i)))
					->back();
			continue_iteration = continue_iteration || iterators[i];
		}
		while (continue_iteration) {
			continue_iteration = false;
			for (int i = 0; i < Performance::MONITOR_MAX; i++) {
				if (iterators[i]) {
					line.write[i] = String::num_real(iterators[i]->get());
					iterators.write[i] = iterators[i]->prev();
				}
				else {
					line.write[i] = "";
				}
				continue_iteration = continue_iteration || iterators[i];
			}
			file->store_csv_line(line);
		}
		file->store_string("\n");

		Vector<Vector<String>> profiler_data = profiler->get_data_as_csv();
		for (int i = 0; i < profiler_data.size(); i++) {
			file->store_csv_line(profiler_data[i]);
		}
	} break;
	case SAVE_VRAM_CSV: {
		Error err;
		Ref<FileAccess> file = FileAccess::open(p_file, FileAccess::WRITE, &err);

		if (err != OK) {
			ERR_PRINT("Failed to open " + p_file);
			return;
		}

		Vector<String> headers;
		headers.resize(vmem_tree->get_columns());
		for (int i = 0; i < vmem_tree->get_columns(); ++i) {
			headers.write[i] = vmem_tree->get_column_title(i);
		}
		file->store_csv_line(headers);

		if (vmem_tree->get_root()) {
			TreeItem* ti = vmem_tree->get_root()->get_first_child();
			while (ti) {
				Vector<String> values;
				values.resize(vmem_tree->get_columns());
				for (int i = 0; i < vmem_tree->get_columns(); ++i) {
					values.write[i] = ti->get_text(i);
				}
				file->store_csv_line(values);

				ti = ti->get_next();
			}
		}
	} break;
	}
}

const SceneDebuggerTree* ScriptEditorDebugger::get_remote_tree() { return scene_tree; }

void ScriptEditorDebugger::_video_mem_export()
{
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_dialog->clear_filters();
	file_dialog_purpose = SAVE_VRAM_CSV;
	file_dialog->popup_file_dialog();
}

Size2 ScriptEditorDebugger::get_minimum_size() const
{
	Size2 ms = MarginContainer::get_minimum_size();
	ms.y = MAX(ms.y, 250 * EDSCALE);
	return ms;
}

void ScriptEditorDebugger::_set_reason_text(const String& p_reason, MessageType p_type)
{
	switch (p_type) {
	case MESSAGE_ERROR:
		reason->add_theme_color_override(SNAME("default_color"),
			get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		break;
	case MESSAGE_WARNING:
		reason->add_theme_color_override(SNAME("default_color"),
			get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		break;
	default:
		reason->add_theme_color_override(SNAME("default_color"),
			get_theme_color(SNAME("success_color"), EditorStringName(Editor)));
		break;
	}

	reason->set_text(p_reason);

	_update_reason_content_height();

	const PackedInt32Array boundaries = TS->string_get_word_breaks(p_reason, "", 80);
	PackedStringArray lines;
	for (int i = 0; i < boundaries.size(); i += 2) {
		const int start = boundaries[i];
		const int end = boundaries[i + 1];
		lines.append(p_reason.substr(start, end - start));
	}

	reason->set_tooltip_text(String("\n").join(lines));
}

void ScriptEditorDebugger::_update_reason_content_height()
{
	float margin_height = 0;
	const float content_height = margin_height + reason->get_content_height();

	float content_max_height = margin_height;
	for (int i = 0; i < 3; i++) {
		if (i >= reason->get_line_count()) {
			break;
		}
		content_max_height += reason->get_line_height(i);
	}

	reason->set_custom_minimum_size(Size2(0, CLAMP(content_height, 0, content_max_height)));
}

void ScriptEditorDebugger::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (is_ready()) {
			for (TreeItem* file_item = breakpoints_tree->get_root()->get_first_child(); file_item;
				 file_item = file_item->get_next()) {
				for (TreeItem* breakpoint_item = file_item->get_first_child(); breakpoint_item;
					 breakpoint_item = breakpoint_item->get_next()) {
				}
			}
			update_tabs();
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		tabs->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SNAME("DebuggerPanel"), EditorStringName(EditorStyles)).ptr());

		skip_breakpoints->set_button_icon(
			get_editor_theme_icon(skip_breakpoints_value ? SNAME("DebugSkipBreakpointsOn")
														 : SNAME("DebugSkipBreakpointsOff")));
		ignore_error_breaks->set_button_icon(get_editor_theme_icon(
			ignore_error_breaks_value ? SNAME("NotificationDisabled") : SNAME("Notification")));
		ignore_error_breaks->add_theme_color_override(
			"icon_normal_color", get_theme_color(SNAME("error_color"), SNAME("Editor")));
		ignore_error_breaks->add_theme_color_override(
			"icon_hover_color", get_theme_color(SNAME("error_color"), SNAME("Editor")));
		ignore_error_breaks->add_theme_color_override(
			"icon_pressed_color", get_theme_color(SNAME("error_color"), SNAME("Editor")));
		ignore_error_breaks->add_theme_color_override(
			"icon_focus_color", get_theme_color(SNAME("error_color"), SNAME("Editor")));
		copy->set_button_icon(get_editor_theme_icon(SNAME("ActionCopy")));
		step->set_button_icon(get_editor_theme_icon(SNAME("DebugStep")));
		next->set_button_icon(get_editor_theme_icon(SNAME("DebugNext")));
		out->set_button_icon(get_editor_theme_icon(SNAME("DebugOut")));
		dobreak->set_button_icon(get_editor_theme_icon(SNAME("Pause")));
		docontinue->set_button_icon(get_editor_theme_icon(SNAME("DebugContinue")));
		vmem_notice_icon->set_texture(get_editor_theme_icon(SNAME("NodeInfo")));
		vmem_refresh->set_button_icon(get_editor_theme_icon(SNAME("Reload")));
		vmem_export->set_button_icon(get_editor_theme_icon(SNAME("Save")));
		vmem_item_menu->set_item_icon(
			VMEM_MENU_SHOW_IN_FILESYSTEM, get_editor_theme_icon(SNAME("ShowInFileSystem")));
		vmem_item_menu->set_item_icon(
			VMEM_MENU_SHOW_IN_EXPLORER, get_editor_theme_icon(SNAME("Filesystem")));
		search->set_right_icon(get_editor_theme_icon(SNAME("Search")));

		reason->add_theme_color_override(SNAME("default_color"),
			get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
		reason->add_theme_style_override(SNAME("normal"),
			get_theme_stylebox(SNAME("normal"), SNAME("Label")).ptr()); // Empty stylebox.

		const Ref<Font> source_font =
			get_theme_font(SNAME("output_source"), EditorStringName(EditorFonts));
		if (source_font.is_valid()) {
			error_tree->add_theme_font_override("font", source_font.ptr());
		}
		const int font_size =
			get_theme_font_size(SNAME("output_source_size"), EditorStringName(EditorFonts));
		error_tree->add_theme_font_size_override("font_size", font_size);

		TreeItem* error_root = error_tree->get_root();
		if (error_root) {
			TreeItem* error = error_root->get_first_child();
			while (error) {
				error = error->get_next();
			}
		}
	} break;
	}
}

void ScriptEditorDebugger::_update_buttons_state()
{
	const bool has_editor_tree = editor_remote_tree && editor_remote_tree->get_selected();
	step->set_disabled(!is_breaked() || !is_debuggable());
	next->set_disabled(!is_breaked() || !is_debuggable());
	out->set_disabled(!is_breaked() || !is_debuggable());
	copy->set_disabled(!is_breaked());
	docontinue->set_disabled(!is_breaked());
	dobreak->set_disabled(is_breaked());

	thread_list_updating = true;
	LocalVector<ThreadDebugged*> threadss;
	for (KeyValue<uint64_t, ThreadDebugged>& I : threads_debugged) {
		threadss.push_back(&I.value);
	}
	threads->set_disabled(threadss.is_empty());

	threadss.sort_custom<ThreadSort>();
	threads->clear();
	int32_t selected_index = -1;
	for (uint32_t i = 0; i < threadss.size(); i++) {
		if (debugging_thread_id == threadss[i]->thread_id) {
			selected_index = i;
		}
		threads->add_item(threadss[i]->name);
	}
	if (selected_index != -1) {
		threads->select(selected_index);
	}

	thread_list_updating = false;
}

void ScriptEditorDebugger::_stop_and_notify()
{
	stop();
	_set_reason_text(TTRC("Debug session closed."), MESSAGE_WARNING);
}

void ScriptEditorDebugger::stop()
{
	set_process(false);
	threads_debugged.clear();
	debugging_thread_id = Thread::UNASSIGNED_ID;
	remote_pid = 0;
	_clear_execution();

	inspector->clear_cache();

	node_path_cache.clear();
	res_path_cache.clear();
	profiler_signature.clear();

	profiler->set_enabled(false, false);
	profiler->set_profiling(false);

	visual_profiler->set_enabled(false);
	visual_profiler->set_profiling(false);

	audio_muted_on_break = false;

	_update_buttons_state();
}

void ScriptEditorDebugger::_profiler_seeked()
{
	if (is_breaked()) {
		return;
	}
	debug_break();
}

void ScriptEditorDebugger::_export_csv()
{
	file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_SAVE_FILE);
	file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	file_dialog_purpose = SAVE_MONITORS_CSV;
	file_dialog->popup_file_dialog();
}

String ScriptEditorDebugger::get_var_value(const String& p_var) const
{
	if (!is_breaked()) {
		return String();
	}
	return inspector->get_stack_variable(p_var);
}

int ScriptEditorDebugger::_get_node_path_cache(const NodePath& p_path)
{
	const int* r = node_path_cache.getptr(p_path);
	if (r) {
		return *r;
	}

	last_path_id++;

	node_path_cache[p_path] = last_path_id;
	return last_path_id;
}

int ScriptEditorDebugger::_get_res_path_cache(const String& p_path)
{
	HashMap<String, int>::Iterator E = res_path_cache.find(p_path);

	if (E) {
		return E->value;
	}

	last_path_id++;

	res_path_cache[p_path] = last_path_id;

	return last_path_id;
}

bool ScriptEditorDebugger::is_move_to_foreground() const { return move_to_foreground; }

void ScriptEditorDebugger::set_move_to_foreground(const bool& p_move_to_foreground)
{
	move_to_foreground = p_move_to_foreground;
}

void ScriptEditorDebugger::set_live_debugging(bool p_enable) { live_debug = p_enable; }

void ScriptEditorDebugger::_live_edit_set()
{
	TreeItem* ti = editor_remote_tree->get_selected();
	if (!ti) {
		return;
	}

	String path;

	while (ti) {
		String lp = ti->get_text(0);
		path = "/" + lp + path;
		ti = ti->get_parent();
	}

	NodePath np = path;

	EditorNode::get_editor_data().set_edited_scene_live_edit_root(np);

	update_live_edit_root();
}

void ScriptEditorDebugger::_live_edit_clear()
{
	NodePath np = NodePath("/root");
	EditorNode::get_editor_data().set_edited_scene_live_edit_root(np);

	update_live_edit_root();
}

void ScriptEditorDebugger::update_live_edit_root()
{
	NodePath np = EditorNode::get_editor_data().get_edited_scene_live_edit_root();
	live_edit_root->set_text(String(np));
}

bool ScriptEditorDebugger::get_debug_mute_audio() const { return debug_mute_audio; }

void ScriptEditorDebugger::set_debug_mute_audio(bool p_mute)
{
	// Send the message if we want to mute the audio or if it isn't muted already due to a break.
	if (p_mute || !audio_muted_on_break) {
		_send_debug_mute_audio_msg(p_mute);
	}
	debug_mute_audio = p_mute;
}

void ScriptEditorDebugger::_mute_audio_on_break(bool p_mute)
{
	// Send the message if we want to mute the audio on a break or if it isn't muted already.
	if (p_mute || !debug_mute_audio) {
		_send_debug_mute_audio_msg(p_mute);
	}
	audio_muted_on_break = p_mute;
}

CameraOverride ScriptEditorDebugger::get_camera_override() const { return camera_override; }

void ScriptEditorDebugger::set_breakpoint(const String& p_path, int p_line, bool p_enabled)
{
	TreeItem* path_item = breakpoints_tree->search_item_text(p_path);
	if (path_item == nullptr) {
		if (!p_enabled) {
			return;
		}
		path_item = breakpoints_tree->create_item();
		path_item->set_text(0, p_path);
	}

	int idx = 0;
	TreeItem* breakpoint_item;
	for (breakpoint_item = path_item->get_first_child(); breakpoint_item;
		 breakpoint_item = breakpoint_item->get_next()) {
	}

	if (breakpoint_item == nullptr) {
		if (!p_enabled) {
			return;
		}
		breakpoint_item = breakpoints_tree->create_item(path_item, idx);
		breakpoint_item->set_text(0, vformat(TTR("Line %d"), p_line));
		return;
	}

	if (!p_enabled) {
		path_item->remove_child(breakpoint_item);
		if (path_item->get_first_child() == nullptr) {
			breakpoints_tree->get_root()->remove_child(path_item);
		}
	}
}

bool ScriptEditorDebugger::is_skip_breakpoints() const { return skip_breakpoints_value; }

bool ScriptEditorDebugger::is_ignore_error_breaks() const { return ignore_error_breaks_value; }

void ScriptEditorDebugger::_error_activated()
{
	TreeItem* selected = error_tree->get_selected();

	if (!selected) {
		return;
	}

	TreeItem* ci = selected->get_first_child();
	if (ci) {
		selected->set_collapsed(!selected->is_collapsed());
	}
}

void ScriptEditorDebugger::_expand_errors_list()
{
	TreeItem* root = error_tree->get_root();
	if (!root) {
		return;
	}

	TreeItem* item = root->get_first_child();
	while (item) {
		item->set_collapsed(false);
		item = item->get_next();
	}
}

void ScriptEditorDebugger::_collapse_errors_list()
{
	TreeItem* root = error_tree->get_root();
	if (!root) {
		return;
	}

	TreeItem* item = root->get_first_child();
	while (item) {
		item->set_collapsed(true);
		item = item->get_next();
	}
}

void ScriptEditorDebugger::_vmem_item_activated()
{
	TreeItem* selected = vmem_tree->get_selected();
	if (!selected) {
		return;
	}
	const String path = selected->get_text(0);
	if (path.is_empty() || !FileAccess::exists(path)) {
		return;
	}
	FileSystemDock::get_singleton()->navigate_to_path(path);
}

void ScriptEditorDebugger::_vmem_tree_rmb_selected(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem* item = vmem_tree->get_selected();
	if (!item) {
		return;
	}

	String path = item->get_text(0);
	if (path.is_empty() || !FileAccess::exists(path)) {
		return;
	}

	vmem_item_menu->set_position(vmem_tree->get_screen_position() + p_pos);
	vmem_item_menu->popup();
}

void ScriptEditorDebugger::_vmem_item_menu_id_pressed(int p_option)
{
	TreeItem* item = vmem_tree->get_selected();
	if (!item) {
		return;
	}

	String path = item->get_text(0);
	switch (p_option) {
	case VMEM_MENU_SHOW_IN_FILESYSTEM: {
		FileSystemDock::get_singleton()->navigate_to_path(path);
	} break;
	case VMEM_MENU_SHOW_IN_EXPLORER: {
		OS::get_singleton()->shell_show_in_file_manager(
			ProjectSettings::get_singleton()->globalize_path(path), true);
	} break;
	case VMEM_MENU_OWNERS: {
		FileSystemDock::get_owners_dialog()->show(path);
	} break;
	}
}

void ScriptEditorDebugger::_clear_errors_list()
{
	error_tree->clear();
	error_count = 0;
	warning_count = 0;
	update_tabs();

	expand_all_button->set_disabled(true);
	collapse_all_button->set_disabled(true);
	clear_button->set_disabled(true);
}

void ScriptEditorDebugger::_breakpoints_item_rmb_selected(
	const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	breakpoints_menu->clear();
	breakpoints_menu->set_size(Size2(1, 1));

	const TreeItem* selected = breakpoints_tree->get_selected();
	String file = selected->get_text(0);
	file = selected->get_parent()->get_text(0);
}

// Right click on specific file(s) or folder(s).
void ScriptEditorDebugger::_error_tree_item_rmb_selected(const Vector2& p_pos, MouseButton p_button)
{
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	item_menu->clear();
	item_menu->reset_size();

	if (error_tree->is_anything_selected()) {
		item_menu->add_icon_item(
			get_editor_theme_icon(SNAME("ActionCopy")), TTRC("Copy Error"), ACTION_COPY_ERROR);
		item_menu->add_icon_item(get_editor_theme_icon(SNAME("ExternalLink")),
			TTRC("Open C++ Source on GitHub"), ACTION_OPEN_SOURCE);
	}

	if (item_menu->get_item_count() > 0) {
		item_menu->set_position(error_tree->get_screen_position() + p_pos);
		item_menu->popup();
	}
}

void ScriptEditorDebugger::_item_menu_id_pressed(int p_option)
{
	switch (p_option) {
	case ACTION_COPY_ERROR: {
		TreeItem* ti = error_tree->get_selected();
		while (ti->get_parent() != error_tree->get_root()) {
			ti = ti->get_parent();
		}

		String type;

		String text = ti->get_text(0) + "   ";
		int rpad_len = text.length();

		text = type + text + ti->get_text(1) + "\n";
		TreeItem* ci = ti->get_first_child();
		while (ci) {
			text += "  " + ci->get_text(0).rpad(rpad_len) + ci->get_text(1) + "\n";
			ci = ci->get_next();
		}

		DisplayServer::get_singleton()->clipboard_set(text);
	} break;

	case ACTION_OPEN_SOURCE: {
		TreeItem* ti = error_tree->get_selected();
		while (ti->get_parent() != error_tree->get_root()) {
			ti = ti->get_parent();
		}

		// Find the child with the "C++ Source".
		// It's not at a fixed position as "C++ Error" may come first.
		TreeItem* ci = ti->get_first_child();
		const String cpp_source = "<" + TTR("C++ Source") + ">";
		while (ci) {
			if (ci->get_text(0) == cpp_source) {
				break;
			}
			ci = ci->get_next();
		}

		if (!ci) {
			WARN_PRINT_ED("No C++ source reference is available for this error.");
			return;
		}

		// Parse back the `file:line @ method()` string.
		const Vector<String> file_line_number =
			ci->get_text(1).get_slicec('@', 0).strip_edges().split(":");
		ERR_FAIL_COND_MSG(file_line_number.size() < 2,
			"Incorrect C++ source stack trace file:line format (please report).");
		const String& file = file_line_number[0];
		const int line_number = file_line_number[1].to_int();

		// Construct a GitHub repository URL and open it in the user's default web browser.
		// If the commit hash is available, use it for greater accuracy. Otherwise fall back to
		// tagged release.
		String git_ref = String(VLTR_VERSION_HASH).is_empty()
							 ? String(VLTR_VERSION_NUMBER) + "-stable"
							 : String(VLTR_VERSION_HASH);
		OS::get_singleton()->shell_open(vformat(
			"https://github.com/godotengine/godot/blob/%s/%s#L%d", git_ref, file, line_number));
	} break;
	case ACTION_DELETE_BREAKPOINT: {
		const TreeItem* selected = breakpoints_tree->get_selected();
	} break;
	case ACTION_DELETE_BREAKPOINTS_IN_FILE: {
		TreeItem* file_item = breakpoints_tree->get_selected();
		// Store first else we will be removing as we loop.
		List<int> lines;
		for (TreeItem* breakpoint_item = file_item->get_first_child(); breakpoint_item;
			 breakpoint_item = breakpoint_item->get_next()) {
		}

		for (const int& line : lines) {
			_set_breakpoint(file_item->get_text(0), line, false);
		}
	} break;
	case ACTION_DELETE_ALL_BREAKPOINTS: {
		_clear_breakpoints();
	} break;
	}
}

void ScriptEditorDebugger::_tab_changed(int p_tab)
{
	if (tabs->get_tab_title(p_tab) == "Video RAM") {
		// "Video RAM" tab was clicked, refresh the data it's displaying when entering the tab.
		_video_mem_request();
	}
}

void ScriptEditorDebugger::_bind_methods() {}

void ScriptEditorDebugger::add_debugger_tab(Control* p_control) { tabs->add_child(p_control); }

void ScriptEditorDebugger::remove_debugger_tab(Control* p_control)
{
	int idx = tabs->get_tab_idx_from_control(p_control);
	ERR_FAIL_COND(idx < 0);
	p_control->queue_free();
}

int ScriptEditorDebugger::get_current_debugger_tab() const { return tabs->get_current_tab(); }

void ScriptEditorDebugger::switch_to_debugger(int p_debugger_tab_idx)
{
	tabs->set_current_tab(p_debugger_tab_idx);
}

void ScriptEditorDebugger::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	if (p_slot != EditorDock::DOCK_SLOT_BOTTOM) {
		vmem_mc->set_theme_type_variation("NoBorderHorizontalBottom");
		vmem_tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_DISABLED);
	}
	else {
		vmem_mc->set_theme_type_variation("NoBorderHorizontal");
		vmem_tree->set_scroll_hint_mode(Tree::SCROLL_HINT_MODE_BOTTOM);
	}
}

