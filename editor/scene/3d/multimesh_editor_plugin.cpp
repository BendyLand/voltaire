/**************************************************************************/
/*  multimesh_editor_plugin.cpp                                           */
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

#include "core/templates/rb_map.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/scene_tree_editor.h"
#include "multimesh_editor_plugin.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"

void MultiMeshEditor::_node_removed(Node* p_node)
{
	if (p_node == node) {
		node = nullptr;
		hide();
	}
}

void MultiMeshEditor::_browsed(const NodePath& p_path)
{
	NodePath path = node->get_path_to(get_node(p_path));

	if (browsing_source) {
		mesh_source->set_text(String(path));
	}
	else {
		surface_source->set_text(String(path));
	}
}

void MultiMeshEditor::_menu_option(int p_option)
{
	switch (p_option) {
	case MENU_OPTION_POPULATE: {
		if (_last_pp_node != node) {
			surface_source->set_text("..");
			mesh_source->set_text("..");
			populate_axis->select(1);
			populate_rotate_random->set_value(0);
			populate_tilt_random->set_value(0);
			populate_scale_random->set_value(0);
			populate_scale->set_value(1);
			populate_amount->set_value(128);

			_last_pp_node = node;
		}
		populate_dialog->popup_centered(Size2(250, 380));

	} break;
	}
}

void MultiMeshEditor::edit(MultiMeshInstance3D* p_multimesh) { node = p_multimesh; }

void MultiMeshEditor::_browse(bool p_source)
{
	browsing_source = p_source;
	Node* browsed_node = nullptr;
	if (p_source) {
		browsed_node = node->get_node_or_null(mesh_source->get_text());
		std->set_title(TTR("Select a Source Mesh:"));
	}
	else {
		browsed_node = node->get_node_or_null(surface_source->get_text());
		std->set_title(TTR("Select a Target Surface:"));
	}
	std->popup_scenetree_dialog(browsed_node);
}

MultiMeshEditorPlugin::MultiMeshEditorPlugin()
{
	multimesh_editor = memnew(MultiMeshEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(multimesh_editor);

	multimesh_editor->options->hide();
}


