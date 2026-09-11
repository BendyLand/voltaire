/**************************************************************************/
/*  editor_debugger_node.cpp                                              */
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

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "editor/debugger/editor_debugger_plugin.h"
#include "editor/debugger/editor_debugger_tree.h"
#include "editor/debugger/script_editor_debugger.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/inspector_dock.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/run/editor_run_bar.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_theme_manager.h"
#include "editor_debugger_node.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/tab_container.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/display_server.h"

EditorDebuggerNode* EditorDebuggerNode::singleton = nullptr;

EditorDebuggerNode::EditorDebuggerNode()
{
	set_name(TTRC("Debugger"));
	set_icon_name("Debug");
	set_layout_key("Debugger");
	set_dock_shortcut(ED_SHORTCUT_AND_COMMAND("bottom_panels/toggle_debugger_bottom_panel",
		TTRC("Toggle Debugger Dock"), KeyModifierMask::ALT | Key::D));
	set_default_slot(EditorDock::DOCK_SLOT_BOTTOM);
	set_available_layouts(EditorDock::DOCK_LAYOUT_HORIZONTAL | EditorDock::DOCK_LAYOUT_FLOATING);

	_update_margins();

	if (!singleton) {
		singleton = this;
	}

	tabs = memnew(TabContainer);
	tabs->set_tabs_visible(false);
	add_child(tabs);

	Ref<StyleBoxEmpty> empty;
	empty.instantiate();
	tabs->add_theme_style_override(SceneStringName(panel), empty.ptr());

	_add_debugger();

	// Remote scene tree
	remote_scene_tree = memnew(EditorDebuggerTree);
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		return;
	}
}

String EditorDebuggerNode::get_server_uri() const
{
	return server.is_valid() ? server->get_uri() : "";
}

Error EditorDebuggerNode::start(const String& p_uri)
{
	if (Engine::get_singleton()->is_recovery_mode_hint()) {
		return ERR_UNAVAILABLE;
	}

	ERR_FAIL_COND_V(!p_uri.contains("://"), ERR_INVALID_PARAMETER);
	if (keep_open && current_uri == p_uri && server.is_valid()) {
		return OK;
	}
	stop(true);
	current_uri = p_uri;

	server = Ref<EditorDebuggerServer>(
		EditorDebuggerServer::create(p_uri.substr(0, p_uri.find("://") + 3)));
	const Error err = server->start(p_uri);
	if (err != OK) {
		return err;
	}
	set_process(true);
	EditorNode::get_log()->add_message(
		"--- Debugging process started ---", EditorLog::MSG_TYPE_EDITOR);
	return OK;
}

void EditorDebuggerNode::stop(bool p_force)
{
	if (keep_open && !p_force) {
		return;
	}

	remote_scene_tree_wait = false;
	inspect_edited_object_wait = false;

	current_uri.clear();
	// Also close all debugging sessions.

	if (server.is_valid()) {
		server->stop();
		EditorNode::get_log()->add_message(
			"--- Debugging process stopped ---", EditorLog::MSG_TYPE_EDITOR);

		if (EditorRunBar::get_singleton()->is_movie_maker_enabled()) {
			// Request attention in case the user was doing something else when movie recording is
			// finished.
			DisplayServer::get_singleton()->window_request_attention();
		}

		server.unref();
	}

	_break_state_changed();
	breakpoints.clear();
	EditorUndoRedoManager::get_singleton()->clear_history(
		EditorUndoRedoManager::REMOTE_HISTORY, false);
	set_process(false);
}

void EditorDebuggerNode::_notification(int p_what)
{
	switch (p_what) {
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (!EditorThemeManager::is_generated_theme_outdated()) {
			return;
		}

		if (tabs->get_tab_count() > 1) {
			tabs->add_theme_style_override(SceneStringName(panel),
				EditorNode::get_singleton()->get_editor_theme()->get_stylebox(
					SNAME("DebuggerPanel"), EditorStringName(EditorStyles)).ptr());
		}
		_update_margins();

		remote_scene_tree->update_icon_max_width();
	} break;

	case NOTIFICATION_READY: {
		_update_debug_options();
		initializing = false;
	} break;

	case NOTIFICATION_PROCESS: {
		if (server.is_null()) {
			return;
		}

		if (!server->is_active()) {
			stop();
			return;
		}
		server->poll();

		_update_errors();

		// Remote scene tree update.
		if (!remote_scene_tree_wait) {
			remote_scene_tree_timeout -= get_process_delta_time();
			if (remote_scene_tree_timeout < 0) {
				if (remote_scene_tree->is_visible_in_tree()) {
					remote_scene_tree_wait = true;
					get_current_debugger()->request_remote_tree();
				}
			}
		}

		// Take connections.
		if (server->is_connection_available()) {
			ScriptEditorDebugger* debugger = nullptr;
			if (debugger == nullptr) {
				if (tabs->get_tab_count() <= 4) { // Max 4 debugging sessions active.
					debugger = _add_debugger();
				}
				else {
					// We already have too many sessions, disconnecting new clients to prevent them
					// from hanging.
					return; // Can't add, stop here.
				}
			}

			EditorRunBar::get_singleton()->get_pause_button()->set_disabled(false);
			// Switch to remote tree view if so desired.
			remote_scene_tree->set_new_session();
			if (auto_switch_remote_scene_tree) {
				SceneTreeDock::get_singleton()->show_remote_tree();
			}
			// Good to go.
			SceneTreeDock::get_singleton()->show_tab_buttons();
			debugger->set_editor_remote_tree(remote_scene_tree);
			// Send breakpoints.
			for (const KeyValue<Breakpoint, bool>& E : breakpoints) {
				const Breakpoint& bp = E.key;
				debugger->set_breakpoint(bp.source, bp.line, E.value);
			} // Will arrive too late, how does the regular run work?

			debugger->update_live_edit_root();
		}
	} break;
	}
}

void EditorDebuggerNode::_update_errors()
{
	int error_count = 0;
	int warning_count = 0;

	if (error_count != last_error_count || warning_count != last_warning_count) {
		last_error_count = error_count;
		last_warning_count = warning_count;

		if (error_count == 0 && warning_count == 0) {
			set_title("");
			set_dock_icon(Ref<Texture2D>());
			set_title_color(Color(0, 0, 0, 0));
			set_force_show_icon(false);
		}
		else {
			set_title(TTR("Debugger") + " (" + itos(error_count + warning_count) + ")");
			if (error_count >= 1 && warning_count >= 1) {
				set_dock_icon(get_editor_theme_icon(SNAME("ErrorWarning")));
				// Use error color to represent the highest level of severity reported.
				set_title_color(get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
			}
			else if (error_count >= 1) {
				set_dock_icon(get_editor_theme_icon(SNAME("Error")));
				set_title_color(get_theme_color(SNAME("error_color"), EditorStringName(Editor)));
			}
			else {
				set_dock_icon(get_editor_theme_icon(SNAME("Warning")));
				set_title_color(get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
			}
			set_force_show_icon(true);
		}
	}
}

void EditorDebuggerNode::_update_margins()
{
	Ref<StyleBox> bottom_panel_margins =
		EditorNode::get_singleton()->get_editor_theme()->get_stylebox(
			SNAME("BottomPanel"), EditorStringName(EditorStyles));
	add_theme_constant_override("margin_top", -bottom_panel_margins->get_margin(SIDE_TOP));
	add_theme_constant_override("margin_left", -bottom_panel_margins->get_margin(SIDE_LEFT));
	add_theme_constant_override("margin_right", -bottom_panel_margins->get_margin(SIDE_RIGHT));
	add_theme_constant_override("margin_bottom", -bottom_panel_margins->get_margin(SIDE_BOTTOM));
}

void EditorDebuggerNode::_debugger_stopped(int p_id)
{
	ScriptEditorDebugger* dbg = get_debugger(p_id);
	ERR_FAIL_NULL(dbg);

	bool found = false;
	if (!found) {
		EditorRunBar::get_singleton()->get_pause_button()->set_pressed(false);
		EditorRunBar::get_singleton()->get_pause_button()->set_disabled(true);
		SceneTreeDock* dock = SceneTreeDock::get_singleton();
		if (dock->is_inside_tree()) {
			dock->hide_remote_tree();
			dock->hide_tab_buttons();
		}
		EditorNode::get_singleton()->notify_all_debug_sessions_exited();
	}
}

void EditorDebuggerNode::set_script_debug_button(MenuButton* p_button)
{
	script_menu = p_button;
	script_menu->set_text(TTRC("Debug"));
	script_menu->set_switch_on_hover(true);
	PopupMenu* p = script_menu->get_popup();
	p->add_shortcut(ED_GET_SHORTCUT("debugger/step_into"), DEBUG_STEP);
	p->add_shortcut(ED_GET_SHORTCUT("debugger/step_over"), DEBUG_NEXT);
	p->add_separator();
	p->add_shortcut(ED_GET_SHORTCUT("debugger/break"), DEBUG_BREAK);
	p->add_shortcut(ED_GET_SHORTCUT("debugger/continue"), DEBUG_CONTINUE);
	p->add_separator();
	p->add_check_shortcut(
		ED_GET_SHORTCUT("debugger/debug_with_external_editor"), DEBUG_WITH_EXTERNAL_EDITOR);
	_break_state_changed();
	script_menu->show();
}

void EditorDebuggerNode::_break_state_changed()
{
	const bool breaked = get_current_debugger()->is_breaked();
	const bool can_debug = get_current_debugger()->is_debuggable();
	if (breaked) { // Show debugger.
		EditorDockManager::get_singleton()->focus_dock(this);
	}

	// Update script menu.
	if (!script_menu) {
		return;
	}
	PopupMenu* p = script_menu->get_popup();
	p->set_item_disabled(p->get_item_index(DEBUG_NEXT), !(breaked && can_debug));
	p->set_item_disabled(p->get_item_index(DEBUG_STEP), !(breaked && can_debug));
	p->set_item_disabled(p->get_item_index(DEBUG_BREAK), breaked);
	p->set_item_disabled(p->get_item_index(DEBUG_CONTINUE), !breaked);
}

void EditorDebuggerNode::_menu_option(int p_id)
{
	switch (p_id) {
	case DEBUG_NEXT: {
		debug_next();
	} break;
	case DEBUG_STEP: {
		debug_step();
	} break;
	case DEBUG_BREAK: {
		debug_break();
	} break;
	case DEBUG_CONTINUE: {
		debug_continue();
	} break;
	case DEBUG_WITH_EXTERNAL_EDITOR: {
		bool ischecked = script_menu->get_popup()->is_item_checked(
			script_menu->get_popup()->get_item_index(DEBUG_WITH_EXTERNAL_EDITOR));
		debug_with_external_editor = !ischecked;
		script_menu->get_popup()->set_item_checked(
			script_menu->get_popup()->get_item_index(DEBUG_WITH_EXTERNAL_EDITOR), !ischecked);
	} break;
	}
}

bool EditorDebuggerNode::is_skip_breakpoints() const
{
	return get_current_debugger()->is_skip_breakpoints();
}

bool EditorDebuggerNode::is_ignore_error_breaks() const
{
	return get_default_debugger()->is_ignore_error_breaks();
}

void EditorDebuggerNode::debug_next() { get_current_debugger()->debug_next(); }

void EditorDebuggerNode::debug_step() { get_current_debugger()->debug_step(); }

void EditorDebuggerNode::debug_break() { get_current_debugger()->debug_break(); }

void EditorDebuggerNode::debug_continue() { get_current_debugger()->debug_continue(); }

String EditorDebuggerNode::get_var_value(const String& p_var) const
{
	return get_current_debugger()->get_var_value(p_var);
}

// LiveEdit/Inspector
void EditorDebuggerNode::request_remote_tree() { get_current_debugger()->request_remote_tree(); }

void EditorDebuggerNode::clear_remote_tree_selection()
{
	remote_scene_tree->clear_selection();
	get_current_debugger()->clear_inspector(remote_scene_tree_clear_msg);
}

void EditorDebuggerNode::_remote_tree_select_requested(
	const TypedArray<int64_t>& p_ids, int p_debugger)
{
	if (p_debugger == tabs->get_current_tab()) {
		remote_scene_tree->select_nodes(p_ids);
	}
}

void EditorDebuggerNode::_remote_tree_clear_selection_requested(int p_debugger)
{
	if (p_debugger != tabs->get_current_tab()) {
		return;
	}
	remote_scene_tree->clear_selection();
	remote_scene_tree_clear_msg = false;
	get_current_debugger()->clear_inspector(false);
	remote_scene_tree_clear_msg = true;
}

void EditorDebuggerNode::_remote_tree_updated(int p_debugger)
{
	if (p_debugger != tabs->get_current_tab()) {
		return;
	}
	remote_scene_tree->clear();
	remote_scene_tree->update_scene_tree(get_current_debugger()->get_remote_tree(), p_debugger);
}

void EditorDebuggerNode::_remote_objects_requested(
	const TypedArray<uint64_t>& p_ids, int p_debugger)
{
	if (p_debugger != tabs->get_current_tab()) {
		return;
	}
	stop_waiting_inspection();
	get_current_debugger()->request_remote_objects(p_ids);
}

void EditorDebuggerNode::_remote_selection_cleared(int p_debugger)
{
	if (p_debugger != tabs->get_current_tab()) {
		return;
	}
	stop_waiting_inspection();
	get_current_debugger()->clear_inspector();
}

bool EditorDebuggerNode::get_debug_mute_audio() const { return debug_mute_audio; }

EditorDebuggerNode::CameraOverride EditorDebuggerNode::get_camera_override()
{
	return camera_override;
}

void EditorDebuggerNode::add_debugger_plugin(const Ref<EditorDebuggerPlugin>& p_plugin)
{
	ERR_FAIL_COND_MSG(p_plugin.is_null(), "Debugger plugin is null.");
	ERR_FAIL_COND_MSG(debugger_plugins.has(p_plugin), "Debugger plugin already exists.");
	debugger_plugins.insert(p_plugin);

	Ref<EditorDebuggerPlugin> plugin = p_plugin;
	for (int i = 0; get_debugger(i); i++) {
		plugin->create_session(get_debugger(i));
	}
}

void EditorDebuggerNode::remove_debugger_plugin(const Ref<EditorDebuggerPlugin>& p_plugin)
{
	ERR_FAIL_COND_MSG(p_plugin.is_null(), "Debugger plugin is null.");
	ERR_FAIL_COND_MSG(!debugger_plugins.has(p_plugin), "Debugger plugin doesn't exists.");
	debugger_plugins.erase(p_plugin);
	Ref<EditorDebuggerPlugin>(p_plugin)->clear();
}


