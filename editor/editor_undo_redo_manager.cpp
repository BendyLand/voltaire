/**************************************************************************/
/*  editor_undo_redo_manager.cpp                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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
#include "editor/debugger/editor_debugger_inspector.h"
#include "editor/debugger/editor_debugger_node.h"
#include "editor/editor_log.h"
#include "editor/editor_node.h"
#include "editor_undo_redo_manager.h"
#include "scene/main/node.h"

void EditorUndoRedoManager::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void EditorUndoRedoManager::finalize()
{
	if (!data) {
		return;
	}
	for (const KeyValue<int, History>& E : data->history_map) {
		discard_history(E.key, false);
	}
	memdelete(data);
	data = nullptr;
}

void EditorUndoRedoManager::force_fixed_history()
{
	initialize();
	ERR_FAIL_COND_MSG(data->pending_action.history_id == INVALID_HISTORY,
		"The current action has no valid history assigned.");
	data->forced_history = true;
}

bool EditorUndoRedoManager::is_committing_action()
{
	return data && data->is_committing;
}

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
	if (!data) {
		return false;
	}
	for (const KeyValue<int, History>& E : data->history_map) {
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
	if (!data) {
		return false;
	}
	for (const KeyValue<int, History>& E : data->history_map) {
		if ((E.key == GLOBAL_HISTORY || E.key == REMOTE_HISTORY ||
				E.key == EditorNode::get_editor_data().get_current_edited_scene_history_id()) &&
			!E.value.redo_stack.is_empty()) {
			return true;
		}
	}
	return false;
}

bool EditorUndoRedoManager::has_history(int p_idx)
{
	return data && data->history_map.has(p_idx);
}

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

EditorUndoRedoManager::History& EditorUndoRedoManager::get_or_create_history(int p_idx)
{
	initialize();
	if (!data->history_map.has(p_idx)) {
		History history;
		history.id = p_idx;
		data->history_map[p_idx] = history;
	}
	return data->history_map[p_idx];
}

bool EditorUndoRedoManager::undo_history(int p_history_id) { return false; }

bool EditorUndoRedoManager::redo_history(int p_history_id) { return false; }

bool EditorUndoRedoManager::is_history_unsaved(int p_history_id) { return false; }

void EditorUndoRedoManager::set_history_as_saved(int p_history_id) {}

void EditorUndoRedoManager::discard_history(int p_history_id, bool p_erase_from_map) {}

void EditorUndoRedoManager::commit_action(bool p_execute) {}


