/**************************************************************************/
/*  multi_node_edit.h                                                     */
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

#pragma once

#include "core/types.h"

class MultiNodeEdit : public RefCounted
{
	friend class EditorQuickOpenDialog;

	LocalVector<NodePath> nodes;
	bool notify_property_list_changed_pending = false;

	struct PLData
	{
		int uses = 0;
	};

	void _queue_notify_property_list_changed();
	void _notify_property_list_changed();

protected:
	static void _bind_methods();

public:
	bool _hide_script_from_inspector() { return true; }

	bool _hide_metadata_from_inspector() { return true; }

	bool _property_can_revert(const StringName& p_name) const;
	String _get_editor_name() const;

	void add_node(const NodePath& p_node);

	int get_node_count() const;
	NodePath get_node(int p_index) const;
	StringName get_edited_class_name() const;

	// If the nodes selected are the same independently of order then return true.
	bool is_same_selection(const MultiNodeEdit* p_other) const
	{
		if (get_node_count() != p_other->get_node_count()) {
			return false;
		}
		HashSet<NodePath> nodes_in_selection;
		for (const NodePath& node : p_other->nodes) {
			nodes_in_selection.insert(node);
		}
		for (const NodePath& node : nodes) {
			if (!nodes_in_selection.has(node)) {
				return false;
			}
		}

		return true;
	}
};


