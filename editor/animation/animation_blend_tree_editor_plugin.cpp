/**************************************************************************/
/*  animation_blend_tree_editor_plugin.cpp                                */
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

#include "animation_blend_tree_editor_plugin.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/templates/rb_set.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_properties.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/gui/check_box.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/progress_bar.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/separator.h"
#include "scene/gui/view_panner.h"
#include "scene/main/window.h"

void AnimationNodeBlendTreeEditor::_update_options_menu(bool p_has_input_ports)
{
	add_node->get_popup()->clear();
	add_node->get_popup()->reset_size();
	for (int i = 0; i < add_options.size(); i++) {
		if (p_has_input_ports && add_options[i].input_port_count == 0) {
			continue;
		}
		add_node->get_popup()->add_item(add_options[i].name, i);
	}

	Ref<AnimationNode> clipb = EditorSettings::get_singleton()->get_resource_clipboard();
	if (clipb.is_valid()) {
		add_node->get_popup()->add_separator();
		add_node->get_popup()->add_item(TTR("Paste"), MENU_PASTE);
	}
	add_node->get_popup()->add_separator();
	add_node->get_popup()->add_item(TTR("Load..."), MENU_LOAD_FILE);
	use_position_from_popup_menu = false;
}

Size2 AnimationNodeBlendTreeEditor::get_minimum_size() const { return Size2(10, 200); }

void AnimationNodeBlendTreeEditor::_popup(bool p_has_input_ports, const Vector2& p_node_position)
{
	_update_options_menu(p_has_input_ports);
	use_position_from_popup_menu = true;
	position_from_popup_menu = p_node_position;
	add_node->get_popup()->set_position(
		graph->get_screen_position() + graph->get_local_mouse_position());
	add_node->get_popup()->reset_size();
	add_node->get_popup()->popup();
}

void AnimationNodeBlendTreeEditor::_popup_request(const Vector2& p_position)
{
	if (read_only) {
		return;
	}

	_popup(false, p_position);
}

void AnimationNodeBlendTreeEditor::_connection_to_empty(
	const String& p_from, int p_from_slot, const Vector2& p_release_position)
{
	if (read_only) {
		return;
	}

	Ref<AnimationNode> node = blend_tree->get_node(p_from);
	if (node.is_valid()) {
		from_node = p_from;
		_popup(true, p_release_position);
	}
}

void AnimationNodeBlendTreeEditor::_connection_from_empty(
	const String& p_to, int p_to_slot, const Vector2& p_release_position)
{
	if (read_only) {
		return;
	}

	Ref<AnimationNode> node = blend_tree->get_node(p_to);
	if (node.is_valid()) {
		to_node = p_to;
		to_slot = p_to_slot;
		_popup(false, p_release_position);
	}
}

void AnimationNodeBlendTreeEditor::_popup_hide()
{
	to_node = "";
	to_slot = -1;
}

void AnimationNodeBlendTreeEditor::_scroll_changed(const Vector2& p_scroll)
{
	if (read_only) {
		return;
	}

	if (updating) {
		return;
	}

	if (blend_tree.is_null()) {
		return;
	}

	updating = true;
	blend_tree->set_graph_offset(p_scroll / EDSCALE);
	updating = false;
}


AnimationNodeBlendTreeEditor* AnimationNodeBlendTreeEditor::singleton = nullptr;

// AnimationNode's "node_changed" signal means almost update_input.
void AnimationNodeBlendTreeEditor::_node_changed(const StringName& p_node_name)
{
	// TODO:
	// Here is executed during the commit of EditorNode::undo_redo, it is not possible to create an
	// undo_redo action here. The disconnect when the number of enabled inputs decreases is done in
	// AnimationNodeBlendTree and update_graph(). This means that there is no place to register
	// undo_redo actions. In order to implement undo_redo correctly, we may need to implement
	// AnimationNodeEdit such as AnimationTrackKeyEdit and add it to _node_selected() with
	// EditorNode::get_singleton()->push_item(AnimationNodeEdit).
	update_graph();
}

void AnimationNodeBlendTreeEditor::_node_rename_lineedit_changed(const String& p_text)
{
	current_node_rename_text = p_text;
}

bool AnimationNodeBlendTreeEditor::can_edit(const Ref<AnimationNode>& p_node)
{
	Ref<AnimationNodeBlendTree> bt = p_node;
	return bt.is_valid();
}


