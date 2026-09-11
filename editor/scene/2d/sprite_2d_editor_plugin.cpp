/**************************************************************************/
/*  sprite_2d_editor_plugin.cpp                                           */
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

#include <thirdparty/clipper2/include/clipper2/clipper.h>
#include "core/math/geometry_2d.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/light_occluder_2d.h"
#include "scene/2d/mesh_instance_2d.h"
#include "scene/2d/physics/collision_polygon_2d.h"
#include "scene/2d/polygon_2d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/panel.h"
#include "scene/gui/view_panner.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/mesh.h"
#include "sprite_2d_editor_plugin.h"

#define PRECISION 1

void Sprite2DEditor::_node_removed(Node* p_node)
{
	if (p_node == node) {
		node = nullptr;
		options->hide();
	}
}

Vector<Vector2> expand(const Vector<Vector2>& points, const Rect2i& rect, float epsilon = 2.0)
{
	int size = points.size();
	ERR_FAIL_COND_V(size < 2, Vector<Vector2>());

	Clipper2Lib::PathD subj(points.size());
	for (int i = 0; i < points.size(); i++) {
		subj[i] = Clipper2Lib::PointD(points[i].x, points[i].y);
	}

	Clipper2Lib::PathsD solution = Clipper2Lib::InflatePaths({subj}, epsilon,
		Clipper2Lib::JoinType::Miter, Clipper2Lib::EndType::Polygon, 2.0, PRECISION, 0.0);
	// Here the miter_limit = 2.0 and arc_tolerance = 0.0 are Clipper2 defaults,
	// and PRECISION is used to scale points up internally, to attain the desired precision.

	ERR_FAIL_COND_V(solution.size() == 0, points);

	// Clamp into the specified rect.
	Clipper2Lib::RectD clamp(rect.position.x, rect.position.y, rect.position.x + rect.size.width,
		rect.position.y + rect.size.height);
	Clipper2Lib::PathsD out = Clipper2Lib::RectClip(clamp, solution[0], PRECISION);
	// Here PRECISION is used to scale points up internally, to attain the desired precision.

	ERR_FAIL_COND_V(out.size() == 0, points);

	const Clipper2Lib::PathD& p2 = out[0];

	Vector<Vector2> outPoints;

	int lasti = p2.size() - 1;
	Vector2 prev = Vector2(p2[lasti].x, p2[lasti].y);
	for (uint64_t i = 0; i < p2.size(); i++) {
		Vector2 cur = Vector2(p2[i].x, p2[i].y);
		if (cur.distance_to(prev) > 0.5f) {
			outPoints.push_back(cur);
			prev = cur;
		}
	}
	return outPoints;
}

void Sprite2DEditor::_menu_option(int p_option)
{
	if (!node) {
		return;
	}

	selected_menu_item = (Menu)p_option;

	switch (p_option) {
	case MENU_OPTION_CONVERT_TO_MESH_2D: {
		debug_uv_dialog->set_ok_button_text(TTR("Create MeshInstance2D"));
		debug_uv_dialog->set_title(TTR("MeshInstance2D Preview"));

		_popup_debug_uv_dialog();
	} break;
	case MENU_OPTION_CONVERT_TO_POLYGON_2D: {
		debug_uv_dialog->set_ok_button_text(TTR("Create Polygon2D"));
		debug_uv_dialog->set_title(TTR("Polygon2D Preview"));

		_popup_debug_uv_dialog();
	} break;
	case MENU_OPTION_CREATE_COLLISION_POLY_2D: {
		debug_uv_dialog->set_ok_button_text(TTR("Create CollisionPolygon2D"));
		debug_uv_dialog->set_title(TTR("CollisionPolygon2D Preview"));

		_popup_debug_uv_dialog();
	} break;
	case MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D: {
		debug_uv_dialog->set_ok_button_text(TTR("Create LightOccluder2D"));
		debug_uv_dialog->set_title(TTR("LightOccluder2D Preview"));

		_popup_debug_uv_dialog();
	} break;
	}
}

void Sprite2DEditor::_update_mesh_data()
{
	ERR_FAIL_NULL(node);
	Ref<Texture2D> texture = node->get_texture();
	ERR_FAIL_COND(texture.is_null());
	Ref<Image> image = texture->get_image();
	ERR_FAIL_COND(image.is_null());

	if (image->is_compressed()) {
		image->decompress();
	}

	Rect2 rect =
		node->is_region_enabled() ? node->get_region_rect() : Rect2(Point2(), image->get_size());
	rect.size /= Vector2(node->get_hframes(), node->get_vframes());
	rect.position += node->get_frame_coords() * rect.size;

	Ref<BitMap> bm;
	bm.instantiate();
	bm->create_from_image_alpha(image);

	int shrink = shrink_pixels->get_value();
	if (shrink > 0) {
		bm->shrink_mask(shrink, rect);
	}

	int grow = grow_pixels->get_value();
	if (grow > 0) {
		bm->grow_mask(grow, rect);
	}

	float epsilon = simplification->get_value();

	Vector<Vector<Vector2>> lines = bm->clip_opaque_to_polygons(rect, epsilon);

	uv_lines.clear();

	computed_vertices.clear();
	computed_uv.clear();
	computed_indices.clear();

	Size2 img_size = image->get_size();
	for (int i = 0; i < lines.size(); i++) {
		lines.write[i] = expand(lines[i], rect, epsilon);
	}

	if (selected_menu_item == MENU_OPTION_CONVERT_TO_MESH_2D) {
		for (int j = 0; j < lines.size(); j++) {
			int index_ofs = computed_vertices.size();

			for (int i = 0; i < lines[j].size(); i++) {
				Vector2 vtx = lines[j][i];
				computed_uv.push_back((vtx + rect.position) / img_size);

				if (node->is_flipped_h()) {
					vtx.x = rect.size.x - vtx.x;
				}
				if (node->is_flipped_v()) {
					vtx.y = rect.size.y - vtx.y;
				}
				vtx += node->get_offset();
				if (node->is_centered()) {
					vtx -= rect.size / 2.0;
				}

				computed_vertices.push_back(vtx);
			}

			Vector<int> poly = Geometry2D::triangulate_polygon(lines[j]);

			for (int i = 0; i < poly.size(); i += 3) {
				for (int k = 0; k < 3; k++) {
					int idx = i + k;
					int idxn = i + (k + 1) % 3;
					uv_lines.push_back(lines[j][poly[idx]] + rect.position);
					uv_lines.push_back(lines[j][poly[idxn]] + rect.position);

					computed_indices.push_back(poly[idx] + index_ofs);
				}
			}
		}
	}

	outline_lines.clear();
	computed_outline_lines.clear();

	if (selected_menu_item == MENU_OPTION_CONVERT_TO_POLYGON_2D ||
		selected_menu_item == MENU_OPTION_CREATE_COLLISION_POLY_2D ||
		selected_menu_item == MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D) {
		outline_lines.resize(lines.size());
		computed_outline_lines.resize(lines.size());
		for (int pi = 0; pi < lines.size(); pi++) {
			Vector<Vector2> ol;
			Vector<Vector2> col;

			ol.resize(lines[pi].size());
			col.resize(lines[pi].size());

			for (int i = 0; i < lines[pi].size(); i++) {
				Vector2 vtx = lines[pi][i];
				ol.write[i] = vtx + rect.position;

				if (node->is_flipped_h()) {
					vtx.x = rect.size.x - vtx.x;
				}
				if (node->is_flipped_v()) {
					vtx.y = rect.size.y - vtx.y;
				}
				// Don't bake offset to Polygon2D which has offset property.
				if (selected_menu_item != MENU_OPTION_CONVERT_TO_POLYGON_2D) {
					vtx += node->get_offset();
				}
				if (node->is_centered()) {
					vtx -= rect.size / 2.0;
				}

				col.write[i] = vtx;
			}

			outline_lines.write[pi] = ol;
			computed_outline_lines.write[pi] = col;
		}
	}

	debug_uv->queue_redraw();
}

void Sprite2DEditor::_create_node()
{
	switch (selected_menu_item) {
	case MENU_OPTION_CONVERT_TO_MESH_2D: {
		_convert_to_mesh_2d_node();
	} break;
	case MENU_OPTION_CONVERT_TO_POLYGON_2D: {
		_convert_to_polygon_2d_node();
	} break;
	case MENU_OPTION_CREATE_COLLISION_POLY_2D: {
		_create_collision_polygon_2d_node();
	} break;
	case MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D: {
		_create_light_occluder_2d_node();
	} break;
	}
}

void Sprite2DEditor::_debug_uv_input(const Ref<InputEvent>& p_input)
{
	if (panner->gui_input(p_input, debug_uv->get_global_rect())) {
		accept_event();
	}
}

void Sprite2DEditor::_debug_uv_draw()
{
	debug_uv->draw_set_transform(-draw_offset * draw_zoom, 0, Vector2(draw_zoom, draw_zoom));

	Ref<Texture2D> tex = node->get_texture();
	ERR_FAIL_COND(tex.is_null());

	debug_uv->draw_texture(tex.ptr(), Point2());

	Color color = Color(1.0, 0.8, 0.7);

	if (selected_menu_item == MENU_OPTION_CONVERT_TO_MESH_2D && uv_lines.size() > 0) {
		debug_uv->draw_multiline(uv_lines, color);

	}
	else if ((selected_menu_item == MENU_OPTION_CONVERT_TO_POLYGON_2D ||
				   selected_menu_item == MENU_OPTION_CREATE_COLLISION_POLY_2D ||
				   selected_menu_item == MENU_OPTION_CREATE_LIGHT_OCCLUDER_2D) &&
			   outline_lines.size() > 0) {
		for (int i = 0; i < outline_lines.size(); i++) {
			Vector<Vector2> outline = outline_lines[i];

			debug_uv->draw_polyline(outline, color);
			debug_uv->draw_line(outline[0], outline[outline.size() - 1], color);
		}
	}
}

void Sprite2DEditor::_center_view()
{
	Ref<Texture2D> tex = node->get_texture();
	ERR_FAIL_COND(tex.is_null());
	Vector2 zoom_factor = (debug_uv->get_size() - Vector2(1, 1) * 50 * EDSCALE) / tex->get_size();
	zoom_widget->set_zoom(MIN(zoom_factor.x, zoom_factor.y));
	// Recalculate scroll limits.
	_update_zoom_and_pan(false);

	Vector2 offset = (tex->get_size() - debug_uv->get_size() / zoom_widget->get_zoom()) / 2;
	h_scroll->set_value_no_signal(offset.x);
	v_scroll->set_value_no_signal(offset.y);
	_update_zoom_and_pan(false);
}

void Sprite2DEditor::_pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event)
{
	h_scroll->set_value_no_signal(h_scroll->get_value() - p_scroll_vec.x / draw_zoom);
	v_scroll->set_value_no_signal(v_scroll->get_value() - p_scroll_vec.y / draw_zoom);
	_update_zoom_and_pan(false);
}

void Sprite2DEditor::_zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	const real_t prev_zoom = draw_zoom;
	zoom_widget->set_zoom(draw_zoom * p_zoom_factor);
	draw_offset += p_origin / prev_zoom - p_origin / zoom_widget->get_zoom();
	h_scroll->set_value_no_signal(draw_offset.x);
	v_scroll->set_value_no_signal(draw_offset.y);
	_update_zoom_and_pan(false);
}


