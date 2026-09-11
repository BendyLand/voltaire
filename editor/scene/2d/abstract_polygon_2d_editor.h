/**************************************************************************/
/*  abstract_polygon_2d_editor.h                                          */
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
#include "scene/2d/node_2d.h"
#include "scene/gui/box_container.h"

class Button;
class CanvasItemEditor;
class ConfirmationDialog;

class AbstractPolygon2DEditor : public HBoxContainer
{
	Button* button_create = nullptr;
	Button* button_edit = nullptr;
	Button* button_delete = nullptr;
	Button* button_center = nullptr;

	struct Vertex
	{
		Vertex() {}

		Vertex(int p_vertex) : vertex(p_vertex) {}

		Vertex(int p_polygon, int p_vertex) : polygon(p_polygon), vertex(p_vertex) {}

		bool operator==(const Vertex& p_vertex) const;
		bool operator!=(const Vertex& p_vertex) const;

		bool valid() const;

		int polygon = -1;
		int vertex = -1;
	};

	struct PosVertex : public Vertex
	{
		PosVertex() {}

		PosVertex(const Vertex& p_vertex, const Vector2& p_pos)
			: Vertex(p_vertex.polygon, p_vertex.vertex), pos(p_pos)
		{
		}

		PosVertex(int p_polygon, int p_vertex, const Vector2& p_pos)
			: Vertex(p_polygon, p_vertex), pos(p_pos)
		{
		}

		Vector2 pos;
	};

	PosVertex edited_point;
	Vertex hover_point;	   // point under mouse cursor
	Vertex selected_point; // currently selected
	PosVertex edge_point;  // adding an edge point?
	Vector2 original_mouse_pos;

	Vector<Vector2> pre_move_edit;
	Vector<Vector2> wip;
	bool wip_active = false;
	bool wip_destructive = false;

	Vector<Vector<Vector2>> pre_center_move_edit;
	bool center_drag = false;
	Vector2 center_drag_origin;
	bool edit_origin_and_center = false;

	bool _polygon_editing_enabled = false;

	CanvasItemEditor* canvas_item_editor = nullptr;
	Panel* panel = nullptr;
	ConfirmationDialog* create_resource = nullptr;

protected:
	enum
	{
		MODE_CREATE,
		MODE_EDIT,
		MODE_DELETE,
		MODE_CONT,
		CENTER_POLY,
	};

	int mode = MODE_EDIT;

	void _wip_cancel();

	void _node_removed(Node* p_node);

	Vertex get_active_point() const;

	virtual Node2D* _get_node() const = 0;
	virtual void _set_node(Node* p_polygon) = 0;

	virtual bool _is_line() const;
	virtual bool _has_uv() const;
	virtual int _get_polygon_count() const;
	virtual Vector2 _get_offset(int p_idx) const;

	virtual void _action_remove_polygon(int p_idx);
	virtual void _commit_action();

	virtual Vector2 _get_geometric_center() const;

	virtual bool _has_resource() const;
	virtual void _create_resource();

public:
	void set_edit_origin_and_center(bool p_enabled);

	void edit(Node* p_polygon);
	AbstractPolygon2DEditor(bool p_wip_destructive = true);
};

class AbstractPolygon2DEditorPlugin : public EditorPlugin
{
	AbstractPolygon2DEditor* polygon_editor = nullptr;
	String klass;

public:
	virtual String get_plugin_name() const override { return klass; }

	AbstractPolygon2DEditorPlugin(AbstractPolygon2DEditor* p_polygon_editor, const String& p_class);
};


