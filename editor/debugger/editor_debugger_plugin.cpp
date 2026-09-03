/**************************************************************************/
/*  editor_debugger_plugin.cpp                                            */
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

#include "editor/debugger/script_editor_debugger.h"
#include "editor_debugger_plugin.h"

void EditorDebuggerSession::_bind_methods() {}

void EditorDebuggerSession::add_session_tab(Control* p_tab)
{
	ERR_FAIL_COND(!p_tab || !debugger);
	debugger->add_debugger_tab(p_tab);
	tabs.insert(p_tab);
}

void EditorDebuggerSession::remove_session_tab(Control* p_tab)
{
	ERR_FAIL_COND(!p_tab || !debugger);
	debugger->remove_debugger_tab(p_tab);
	tabs.erase(p_tab);
}

bool EditorDebuggerSession::is_breaked()
{
	ERR_FAIL_NULL_V_MSG(debugger, false, "Plugin is not attached to debugger.");
	return debugger->is_breaked();
}

bool EditorDebuggerSession::is_debuggable()
{
	ERR_FAIL_NULL_V_MSG(debugger, false, "Plugin is not attached to debugger.");
	return debugger->is_debuggable();
}

void EditorDebuggerSession::set_breakpoint(const String& p_path, int p_line, bool p_enabled)
{
	ERR_FAIL_NULL_MSG(debugger, "Plugin is not attached to debugger.");
	debugger->set_breakpoint(p_path, p_line, p_enabled);
}

EditorDebuggerSession::~EditorDebuggerSession() { detach_debugger(); }

/// EditorDebuggerPlugin

EditorDebuggerPlugin::~EditorDebuggerPlugin() { clear(); }

void EditorDebuggerPlugin::clear()
{
	for (Ref<EditorDebuggerSession>& session : sessions) {
		session->detach_debugger();
	}
	sessions.clear();
}

void EditorDebuggerPlugin::create_session(ScriptEditorDebugger* p_debugger)
{
	sessions.push_back(Ref<EditorDebuggerSession>(memnew(EditorDebuggerSession(p_debugger))));
	setup_session(sessions.size() - 1);
}

Ref<EditorDebuggerSession> EditorDebuggerPlugin::get_session(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, sessions.size(), nullptr);
	return sessions.get(p_idx);
}

void EditorDebuggerPlugin::breakpoints_cleared_in_tree() {}

void EditorDebuggerPlugin::setup_session(int p_idx) {}

bool EditorDebuggerPlugin::has_capture(const String& p_capture) const { return false; }


