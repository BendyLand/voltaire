/**************************************************************************/
/*  collision_shape_2d_editor_plugin.cpp                                  */
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

#include "collision_shape_2d_editor_plugin.h"
#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/2d/capsule_shape_2d.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/2d/concave_polygon_shape_2d.h"
#include "scene/resources/2d/convex_polygon_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/resources/2d/segment_shape_2d.h"
#include "scene/resources/2d/separation_ray_shape_2d.h"
#include "scene/resources/2d/world_boundary_shape_2d.h"

void CollisionShape2DEditor::_node_removed(Node* p_node)
{
	if (p_node == node) {
		node = nullptr;
	}
}

void CollisionShape2DEditor::forward_canvas_draw_over_viewport(Control* p_overlay)
{
	if (!node) {
		return;
	}

	if (!node->is_visible_in_tree()) {
		return;
	}

	Viewport* vp = node->get_viewport();
	if (vp && !vp->is_visible_subviewport()) {
		return;
	}

	if (shape_type == -1) {
		return;
	}

	Transform2D gt = canvas_item_editor->get_canvas_transform() * node->get_screen_transform();

	Ref<Texture2D> h = get_editor_theme_icon(SNAME("EditorHandle"));
	Vector2 size = h->get_size() * 0.5;

	handles.clear();

	switch (shape_type) {
	case CAPSULE_SHAPE: {
		Ref<CapsuleShape2D> shape = current_shape;

		handles.resize(2);
		float radius = shape->get_radius();
		float height = shape->get_height() / 2;

		handles.write[0] = Point2(radius, 0);
		handles.write[1] = Point2(0, height);

		p_overlay->draw_texture(h.ptr(), gt.xform(handles[0]) - size);
		p_overlay->draw_texture(h.ptr(), gt.xform(handles[1]) - size);

	} break;

	case CIRCLE_SHAPE: {
		Ref<CircleShape2D> shape = current_shape;

		handles.resize(1);
		handles.write[0] = Point2(shape->get_radius(), 0);

		p_overlay->draw_texture(h.ptr(), gt.xform(handles[0]) - size);

	} break;

	case CONCAVE_POLYGON_SHAPE: {
		Ref<ConcavePolygonShape2D> shape = current_shape;

		const Vector<Vector2>& segments = shape->get_segments();

		handles.resize(segments.size());
		for (int i = 0; i < handles.size(); i++) {
			handles.write[i] = segments[i];
			p_overlay->draw_texture(h.ptr(), gt.xform(handles[i]) - size);
		}

	} break;

	case CONVEX_POLYGON_SHAPE: {
		Ref<ConvexPolygonShape2D> shape = current_shape;

		const Vector<Vector2>& points = shape->get_points();

		handles.resize(points.size());
		for (int i = 0; i < handles.size(); i++) {
			handles.write[i] = points[i];
			p_overlay->draw_texture(h.ptr(), gt.xform(handles[i]) - size);
		}

	} break;

	case WORLD_BOUNDARY_SHAPE: {
		Ref<WorldBoundaryShape2D> shape = current_shape;

		handles.resize(2);
		handles.write[0] = shape->get_normal() * shape->get_distance();
		handles.write[1] = shape->get_normal() * (shape->get_distance() + 30.0);

		p_overlay->draw_texture(h.ptr(), gt.xform(handles[0]) - size);
		p_overlay->draw_texture(h.ptr(), gt.xform(handles[1]) - size);

	} break;

	case SEPARATION_RAY_SHAPE: {
		Ref<SeparationRayShape2D> shape = current_shape;

		handles.resize(1);
		handles.write[0] = Point2(0, shape->get_length());

		p_overlay->draw_texture(h.ptr(), gt.xform(handles[0]) - size);

	} break;

	case RECTANGLE_SHAPE: {
		Ref<RectangleShape2D> shape = current_shape;

		handles.resize(8);
		Vector2 ext = shape->get_size() / 2;
		for (int i = 0; i < handles.size(); i++) {
			handles.write[i] = RECT_HANDLES[i] * ext;
			p_overlay->draw_texture(h.ptr(), gt.xform(handles[i]) - size);
		}

	} break;

	case SEGMENT_SHAPE: {
		Ref<SegmentShape2D> shape = current_shape;

		handles.resize(2);
		handles.write[0] = shape->get_a();
		handles.write[1] = shape->get_b();

		p_overlay->draw_texture(h.ptr(), gt.xform(handles[0]) - size);
		p_overlay->draw_texture(h.ptr(), gt.xform(handles[1]) - size);

	} break;
	}
}

CollisionShape2DEditorPlugin::CollisionShape2DEditorPlugin()
{
	collision_shape_2d_editor = memnew(CollisionShape2DEditor);
	EditorNode::get_singleton()->get_gui_base()->add_child(collision_shape_2d_editor);
}


