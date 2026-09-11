/**************************************************************************/
/*  editor_undo_redo_manager.cpp                                          */
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

#include "core/io/resource.h"
#include "core/os/os.h"
#include "core/templates/mem_unique_ptr.h"
#include "editor/debugger/editor_debugger_inspector.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor_undo_redo_manager.h"
#include "scene/main/node.h"

EditorUndoRedoManager* EditorUndoRedoManager::singleton = nullptr;

void EditorUndoRedoManager::force_fixed_history()
{
	ERR_FAIL_COND_MSG(pending_action.history_id == INVALID_HISTORY,
		"The current action has no valid history assigned.");
	forced_history = true;
}

bool EditorUndoRedoManager::is_committing_action() const { return is_committing; }

bool EditorUndoRedoManager::undo()
{
	if (!has_undo()) {
		return false;
	}

	History* selected_history = _get_newest_undo();
	if (selected_history) {
		return undo_history(selected_history->id);
	}
	return false;
}

bool EditorUndoRedoManager::redo()
{
	if (!has_redo()) {
		return false;
	}

	int selected_history = INVALID_HISTORY;
	double global_timestamp = Math::INF;

	// Pick the history with lowest last action timestamp (either global or current scene).
	{
		History& history = get_or_create_history(GLOBAL_HISTORY);
		if (!history.redo_stack.is_empty()) {
			selected_history = history.id;
			global_timestamp = history.redo_stack.back()->get().timestamp;
		}
	}

	{
		History& history = get_or_create_history(REMOTE_HISTORY);
		if (!history.redo_stack.is_empty() &&
			history.redo_stack.back()->get().timestamp < global_timestamp) {
			selected_history = history.id;
			global_timestamp = history.redo_stack.back()->get().timestamp;
		}
	}

	{
		History& history = get_or_create_history(
			EditorNode::get_editor_data().get_current_edited_scene_history_id());
		if (!history.redo_stack.is_empty() &&
			history.redo_stack.back()->get().timestamp < global_timestamp) {
			selected_history = history.id;
		}
	}

	if (selected_history != INVALID_HISTORY) {
		return redo_history(selected_history);
	}
	return false;
}

void EditorUndoRedoManager::set_history_as_unsaved(int p_id)
{
	History& history = get_or_create_history(p_id);
	history.saved_version = UNSAVED_VERSION;
}

bool EditorUndoRedoManager::has_undo()
{
	for (const KeyValue<int, History>& E : history_map) {
		if ((E.key == GLOBAL_HISTORY || E.key == REMOTE_HISTORY ||
				E.key == EditorNode::get_editor_data().get_current_edited_scene_history_id()) &&
			!E.value.undo_stack.is_empty()) {
			return true;
		}
	}
	return false;
}

bool EditorUndoRedoManager::has_redo()
{
	for (const KeyValue<int, History>& E : history_map) {
		if ((E.key == GLOBAL_HISTORY || E.key == REMOTE_HISTORY ||
				E.key == EditorNode::get_editor_data().get_current_edited_scene_history_id()) &&
			!E.value.redo_stack.is_empty()) {
			return true;
		}
	}
	return false;
}

bool EditorUndoRedoManager::has_history(int p_idx) const { return history_map.has(p_idx); }

int EditorUndoRedoManager::get_current_action_history_id()
{
	if (has_undo()) {
		History* selected_history = _get_newest_undo();
		if (selected_history) {
			return selected_history->id;
		}
	}
	return INVALID_HISTORY;
}

EditorUndoRedoManager::History* EditorUndoRedoManager::_get_newest_undo()
{
	History* selected_history = nullptr;
	double global_timestamp = 0;

	// Pick the history with greatest last action timestamp (either global or current scene).
	{
		History& history = get_or_create_history(GLOBAL_HISTORY);
		if (!history.undo_stack.is_empty()) {
			selected_history = &history;
			global_timestamp = history.undo_stack.back()->get().timestamp;
		}
	}

	{
		History& history = get_or_create_history(REMOTE_HISTORY);
		if (!history.undo_stack.is_empty() &&
			history.undo_stack.back()->get().timestamp > global_timestamp) {
			selected_history = &history;
			global_timestamp = history.undo_stack.back()->get().timestamp;
		}
	}

	{
		History& history = get_or_create_history(
			EditorNode::get_editor_data().get_current_edited_scene_history_id());
		if (!history.undo_stack.is_empty() &&
			history.undo_stack.back()->get().timestamp > global_timestamp) {
			selected_history = &history;
		}
	}

	return selected_history;
}

// In editor/editor_undo_redo_manager.cpp

// 1. Define static helper wrappers at the top of the file or above _bind_methods()
static void _bind_commit_action(bool p_execute)
{
	if (EditorUndoRedoManager::get_singleton()) {
		EditorUndoRedoManager::get_singleton()->commit_action(p_execute);
	}
}

static bool _bind_is_committing_action()
{
	if (EditorUndoRedoManager::get_singleton()) {
		return EditorUndoRedoManager::get_singleton()->is_committing_action();
	}
	return false;
}

void EditorUndoRedoManager::_bind_methods() {}

EditorUndoRedoManager* EditorUndoRedoManager::get_singleton() { return singleton; }

EditorUndoRedoManager::~EditorUndoRedoManager()
{
	for (const KeyValue<int, History>& E : history_map) {
		discard_history(E.key, false);
	}
}


