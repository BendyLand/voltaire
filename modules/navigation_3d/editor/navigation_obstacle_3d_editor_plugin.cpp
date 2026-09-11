/**************************************************************************/
/*  navigation_obstacle_3d_editor_plugin.cpp                              */
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

#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "navigation_obstacle_3d_editor_plugin.h"
#include "scene/3d/navigation/navigation_obstacle_3d.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/main/scene_tree.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "servers/rendering/rendering_server.h"

String NavigationObstacle3DGizmoPlugin::get_gizmo_name() const { return "NavigationObstacle3D"; }

bool NavigationObstacle3DGizmoPlugin::can_be_hidden() const { return true; }

int NavigationObstacle3DGizmoPlugin::get_priority() const { return -1; }

NavigationObstacle3DGizmoPlugin::NavigationObstacle3DGizmoPlugin() { current_state = VISIBLE; }

void NavigationObstacle3DEditorPlugin::_node_removed(Node* p_node)
{
	if (obstacle_node == p_node) {
		obstacle_node = nullptr;

		RenderingServer* rs = RenderingServer::get_singleton();
		rs->mesh_clear(point_lines_mesh_rid);
		rs->mesh_clear(point_handle_mesh_rid);

		obstacle_editor->hide();
	}
}

void NavigationObstacle3DEditorPlugin::set_mode(int p_option)
{
	if (p_option == NavigationObstacle3DEditorPlugin::ACTION_FLIP) {
		button_flip->set_pressed(false);
		action_flip_vertices();
		return;
	}

	if (p_option == NavigationObstacle3DEditorPlugin::ACTION_CLEAR) {
		button_clear->set_pressed(false);
		button_clear_dialog->reset_size();
		button_clear_dialog->popup_centered();
		return;
	}

	mode = p_option;

	button_create->set_pressed(p_option == NavigationObstacle3DEditorPlugin::MODE_CREATE);
	button_edit->set_pressed(p_option == NavigationObstacle3DEditorPlugin::MODE_EDIT);
	button_delete->set_pressed(p_option == NavigationObstacle3DEditorPlugin::MODE_DELETE);
	button_flip->set_pressed(false);
	button_clear->set_pressed(false);
}

void NavigationObstacle3DEditorPlugin::_wip_cancel()
{
	wip_vertices.clear();
	wip_active = false;

	edited_point = -1;

	redraw();
}

NavigationObstacle3DEditorPlugin* NavigationObstacle3DEditorPlugin::singleton = nullptr;

NavigationObstacle3DEditorPlugin::~NavigationObstacle3DEditorPlugin()
{
	RenderingServer* rs = RenderingServer::get_singleton();
	ERR_FAIL_NULL(rs);

	if (point_lines_instance_rid.is_valid()) {
		rs->free_rid(point_lines_instance_rid);
		point_lines_instance_rid = RID();
	}
	if (point_lines_mesh_rid.is_valid()) {
		rs->free_rid(point_lines_mesh_rid);
		point_lines_mesh_rid = RID();
	}

	if (point_handles_instance_rid.is_valid()) {
		rs->free_rid(point_handles_instance_rid);
		point_handles_instance_rid = RID();
	}
	if (point_handle_mesh_rid.is_valid()) {
		rs->free_rid(point_handle_mesh_rid);
		point_handle_mesh_rid = RID();
	}
}


