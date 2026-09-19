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

String EditorDebuggerNode::get_server_uri() const
{
	return server.is_valid() ? server->get_uri() : "";
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

ScriptEditorDebugger* EditorDebuggerNode::get_current_debugger() const
{
	ScriptEditorDebugger sed = ScriptEditorDebugger();
	return &sed;
}



void EditorDebuggerNode::stop_waiting_inspection() {}

void EditorDebuggerNode::set_camera_override(EditorDebuggerNode::CameraOverride) {}

ScriptEditorDebugger* EditorDebuggerNode::get_default_debugger() const {}

ScriptEditorDebugger* EditorDebuggerNode::get_debugger(int) const {}

void EditorDebuggerNode::set_debug_mute_audio(bool) {}

void EditorDebuggerNode::reload_scripts(Vector<String> const&) {}
