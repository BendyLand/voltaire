/**************************************************************************/
/*  csg_gizmos.cpp                                                        */
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

#include "core/math/geometry_3d.h"
#include "csg_gizmos.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/3d/gizmos/gizmo_3d_helper.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/menu_button.h"
#include "scene/main/scene_tree.h"

void CSGShapeEditor::_node_removed(Node* p_node)
{
	if (p_node == node) {
		node = nullptr;
		options->hide();
	}
}

void CSGShapeEditor::edit(CSGShape3D* p_csg_shape)
{
	node = p_csg_shape;
	if (node) {
		options->show();
	}
	else {
		options->hide();
	}
}

void CSGShapeEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		options->set_button_icon(get_editor_theme_icon(SNAME("CSGCombiner3D")));
	} break;
	}
}

String CSGShape3DGizmoPlugin::get_gizmo_name() const { return "CSGShape3D"; }

EditorPluginCSG::EditorPluginCSG()
{
	Ref<CSGShape3DGizmoPlugin> gizmo_plugin =
		Ref<CSGShape3DGizmoPlugin>(memnew(CSGShape3DGizmoPlugin));
	Node3DEditor::get_singleton()->add_gizmo_plugin(gizmo_plugin);

	csg_shape_editor = memnew(CSGShapeEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(csg_shape_editor);
}


