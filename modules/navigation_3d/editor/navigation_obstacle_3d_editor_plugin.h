/**************************************************************************/
/*  navigation_obstacle_3d_editor_plugin.h                                */
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

#include "editor/plugins/editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_gizmos.h"
#include "scene/gui/box_container.h"

class Button;
class ConfirmationDialog;
class NavigationObstacle3D;

class NavigationObstacle3DGizmoPlugin : public EditorNode3DGizmoPlugin
{
public:
	virtual String get_gizmo_name() const override;

	bool can_be_hidden() const override;
	int get_priority() const override;

	NavigationObstacle3DGizmoPlugin();
};

class NavigationObstacle3DEditorPlugin : public EditorPlugin
{
	Ref<NavigationObstacle3DGizmoPlugin> obstacle_3d_gizmo_plugin;

	NavigationObstacle3D* obstacle_node = nullptr;

	Ref<StandardMaterial3D> line_material;
	Ref<StandardMaterial3D> handle_material;

	RID point_lines_mesh_rid;
	RID point_lines_instance_rid;
	RID point_handle_mesh_rid;
	RID point_handles_instance_rid;

public:
	enum Mode
	{
		MODE_CREATE = 0,
		MODE_EDIT,
		MODE_DELETE,
		ACTION_FLIP,
		ACTION_CLEAR,
	};

private:
	int mode = MODE_EDIT;

	int edited_point = 0;
	Vector3 edited_point_pos;
	Vector<Vector3> pre_move_edit;
	Vector<Vector3> wip_vertices;
	bool wip_active = false;
	bool snap_ignore = false;

	Button* button_create = nullptr;
	Button* button_edit = nullptr;
	Button* button_delete = nullptr;
	Button* button_flip = nullptr;
	Button* button_clear = nullptr;

	ConfirmationDialog* button_clear_dialog = nullptr;

public:
	HBoxContainer* obstacle_editor = nullptr;
	static NavigationObstacle3DEditorPlugin* singleton;

	int get_mode() { return mode; }

	NavigationObstacle3DEditorPlugin() = default;
	~NavigationObstacle3DEditorPlugin();
};


