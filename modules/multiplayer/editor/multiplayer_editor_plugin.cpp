/**************************************************************************/
/*  multiplayer_editor_plugin.cpp                                         */
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

#include "../multiplayer_synchronizer.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor_network_profiler.h"
#include "multiplayer_editor_plugin.h"
#include "replication_editor.h"
#include "scene/main/scene_tree.h"

void MultiplayerEditorDebugger::_bind_methods() {}

bool MultiplayerEditorDebugger::has_capture(const String& p_capture) const
{
	return p_capture == "multiplayer";
}

/// MultiplayerEditorPlugin

void MultiplayerEditorPlugin::_open_request(const String& p_path)
{
	EditorInterface::get_singleton()->open_scene_from_path(p_path);
}

void MultiplayerEditorPlugin::_node_removed(Node* p_node)
{
	if (p_node && p_node == repl_editor->get_current()) {
		repl_editor->edit(nullptr);
		repl_editor->close();
		repl_editor->get_pin()->set_pressed(false);
	}
}

void MultiplayerEditorPlugin::_pinned()
{
	if (!repl_editor->get_pin()->is_pressed() && repl_editor->get_current() == nullptr) {
		repl_editor->close();
	}
}


