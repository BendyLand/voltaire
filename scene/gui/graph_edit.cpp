/**************************************************************************/
/*  graph_edit.cpp                                                        */
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

#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/os/keyboard.h"
#include "graph_edit.h"
#include "scene/2d/line_2d.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/material.h"
#include "scene/resources/style_box.h"
#include "scene/theme/theme_db.h"
#include <cfloat>
#include "core/input/shortcut.h"
#include "scene/main/viewport.h"
#include "graph_node.h"
#include "scene/main/scene_tree.h"
#include "servers/display/accessibility_server.h"

constexpr int MINIMAP_OFFSET = 12;
constexpr int MINIMAP_PADDING = 5;
constexpr int MIN_DRAG_DISTANCE_FOR_VALID_CONNECTION = 20;
constexpr int MAX_CONNECTION_LINE_CURVE_TESSELATION_STAGES = 5;
constexpr int GRID_MINOR_STEPS_PER_MAJOR_LINE = 10;
constexpr int GRID_MINOR_STEPS_PER_MAJOR_DOT = 5;
constexpr int GRID_MIN_SNAPPING_DISTANCE = 2;
constexpr int GRID_MAX_SNAPPING_DISTANCE = 100;

bool GraphEditFilter::has_point(const Point2& p_point) const { return ge->_filter_input(p_point); }

GraphEditFilter::GraphEditFilter(GraphEdit* p_edit) { ge = p_edit; }

Control::CursorShape GraphEditMinimap::get_cursor_shape(const Point2& p_pos) const
{
	if (is_resizing || (p_pos.x < theme_cache.resizer->get_width() &&
						   p_pos.y < theme_cache.resizer->get_height())) {
		return CURSOR_FDIAGSIZE;
	}

	return Control::get_cursor_shape(p_pos);
}

void GraphEditMinimap::update_minimap()
{
	Vector2 graph_offset = _get_graph_offset();
	Vector2 graph_size = _get_graph_size();

	camera_position = ge->get_scroll_offset() - graph_offset;
	camera_size = ge->get_size();

	Vector2 render_size = _get_render_size();
	float target_ratio = render_size.width / render_size.height;
	float graph_ratio = graph_size.width / graph_size.height;

	graph_proportions = graph_size;
	graph_padding = Vector2(0, 0);
	if (graph_ratio > target_ratio) {
		graph_proportions.width = graph_size.width;
		graph_proportions.height = graph_size.width / target_ratio;
		graph_padding.y = Math::abs(graph_size.height - graph_proportions.y) / 2;
	}
	else {
		graph_proportions.width = graph_size.height * target_ratio;
		graph_proportions.height = graph_size.height;
		graph_padding.x = Math::abs(graph_size.width - graph_proportions.x) / 2;
	}

	// This centers minimap inside the minimap rectangle.
	minimap_offset = minimap_padding + _convert_from_graph_position(graph_padding);
}

Rect2 GraphEditMinimap::get_camera_rect()
{
	Vector2 camera_center =
		_convert_from_graph_position(camera_position + camera_size / 2) + minimap_offset;
	Vector2 camera_viewport = _convert_from_graph_position(camera_size);
	Vector2 camera_pos = (camera_center - camera_viewport / 2);
	return Rect2(camera_pos, camera_viewport);
}

Vector2 GraphEditMinimap::_get_render_size()
{
	if (!is_inside_tree()) {
		return Vector2(0, 0);
	}

	return get_size() - 2 * minimap_padding;
}

Vector2 GraphEditMinimap::_get_graph_offset() { return ge->min_scroll_offset; }

Vector2 GraphEditMinimap::_get_graph_size()
{
	Vector2 graph_size = ge->max_scroll_offset - ge->min_scroll_offset;

	if (graph_size.width == 0) {
		graph_size.width = 1;
	}
	if (graph_size.height == 0) {
		graph_size.height = 1;
	}

	return graph_size;
}

Vector2 GraphEditMinimap::_convert_from_graph_position(const Vector2& p_position)
{
	Vector2 map_position = Vector2(0, 0);
	Vector2 render_size = _get_render_size();

	map_position.x = p_position.x * render_size.width / graph_proportions.x;
	map_position.y = p_position.y * render_size.height / graph_proportions.y;

	return map_position;
}

Vector2 GraphEditMinimap::_convert_to_graph_position(const Vector2& p_position)
{
	Vector2 graph_position = Vector2(0, 0);
	Vector2 render_size = _get_render_size();

	graph_position.x = p_position.x * graph_proportions.x / render_size.width;
	graph_position.y = p_position.y * graph_proportions.y / render_size.height;

	return graph_position;
}

void GraphEditMinimap::_adjust_graph_scroll(const Vector2& p_offset)
{
	Vector2 graph_offset = _get_graph_offset();
	ge->set_scroll_offset(p_offset + graph_offset - camera_size / 2);
}

GraphEditMinimap::GraphEditMinimap(GraphEdit* p_edit)
{
	ge = p_edit;

	minimap_padding = Vector2(MINIMAP_PADDING, MINIMAP_PADDING);
	minimap_offset = minimap_padding + _convert_from_graph_position(graph_padding);
}

Ref<Shader> GraphEdit::default_connections_shader;

void GraphEdit::init_shaders()
{
	default_connections_shader.instantiate();
	default_connections_shader->set_code(R"(
// Connection lines shader.
shader_type canvas_item;
render_mode blend_mix;

uniform vec4 rim_color : source_color;
uniform int from_type;
uniform int to_type;
uniform float line_width;

void fragment(){
	float fake_aa_width = 1.5/line_width;
	float rim_width = 1.5/line_width;

	float dist = abs(UV.y - 0.5);
	float alpha = smoothstep(0.5, 0.5-fake_aa_width, dist);
	vec4 final_color = mix(rim_color, COLOR, smoothstep(0.5-rim_width, 0.5-fake_aa_width-rim_width, dist));
	COLOR = vec4(final_color.rgb, final_color.a*alpha);
}
)");
}

void GraphEdit::finish_shaders() { default_connections_shader.unref(); }

Control::CursorShape GraphEdit::get_cursor_shape(const Point2& p_pos) const
{
	if (moving_selection) {
		return CURSOR_MOVE;
	}

	return Control::get_cursor_shape(p_pos);
}

bool GraphEdit::is_node_connected(
	const String& p_from, int p_from_port, const String& p_to, int p_to_port)
{
	for (const Ref<Connection>& conn : connection_map[p_from]) {
		if (conn->from_node == p_from && conn->from_port == p_from_port && conn->to_node == p_to &&
			conn->to_port == p_to_port) {
			return true;
		}
	}

	return false;
}

const Vector<Ref<GraphEdit::Connection>>& GraphEdit::get_connections() const { return connections; }

int GraphEdit::get_connection_count(const String& p_node, int p_port)
{
	int count = 0;
	for (const Ref<Connection>& conn : connections) {
		if ((conn->from_node == p_node && conn->from_port == p_port) ||
			(conn->to_node == p_node && conn->to_port == p_port)) {
			count += 1;
		}
	}
	return count;
}

Vector2 GraphEdit::get_scroll_offset() const { return scroll_offset; }

void GraphEdit::_graph_element_visibility_changed(GraphNode* p_graph_element)
{
	if (p_graph_element->is_selected() && !p_graph_element->is_visible()) {
		p_graph_element->set_selected(false);
	}
}

void GraphEdit::_update_theme_item_cache()
{
	Control::_update_theme_item_cache();

	theme_cache.base_scale = get_theme_default_base_scale();
}

bool GraphEdit::is_in_input_hotzone(GraphNode* p_graph_node, int p_port_idx,
	const Vector2& p_mouse_pos, const Vector2i& p_port_size)
{
	Vector2 pos =
		p_graph_node->get_input_port_position(p_port_idx) * zoom + p_graph_node->get_position();
	return is_in_port_hotzone(pos / zoom, p_mouse_pos, p_port_size, true);
}

bool GraphEdit::is_in_output_hotzone(GraphNode* p_graph_node, int p_port_idx,
	const Vector2& p_mouse_pos, const Vector2i& p_port_size)
{
	if (p_graph_node->is_resizable()) {
		Ref<Texture2D> resizer = p_graph_node->theme_cache.resizer;
		Rect2 resizer_rect = Rect2(
			p_graph_node->get_position() / zoom + p_graph_node->get_size() - resizer->get_size(),
			resizer->get_size());
		if (resizer_rect.has_point(p_mouse_pos)) {
			return false;
		}
	}
	Vector2 pos =
		p_graph_node->get_output_port_position(p_port_idx) * zoom + p_graph_node->get_position();
	return is_in_port_hotzone(pos / zoom, p_mouse_pos, p_port_size, false);
}

PackedVector2Array GraphEdit::get_connection_line(const Vector2& p_from, const Vector2& p_to) const
{
	float x_diff = (p_to.x - p_from.x);
	float cp_offset = x_diff * lines_curvature;
	if (x_diff < 0) {
		cp_offset *= -1;
	}

	Curve2D curve;
	curve.add_point(p_from);
	curve.set_point_out(0, Vector2(cp_offset, 0));
	curve.add_point(p_to);
	curve.set_point_in(1, Vector2(-cp_offset, 0));

	if (lines_curvature > 0) {
		return curve.tessellate(MAX_CONNECTION_LINE_CURVE_TESSELATION_STAGES, 2.0);
	}
	else {
		return curve.tessellate(1);
	}
}

Ref<GraphEdit::Connection> GraphEdit::get_closest_connection_at_point(
	const Vector2& p_point, float p_max_distance) const
{
	Vector2 transformed_point = p_point + get_scroll_offset();

	Ref<GraphEdit::Connection> closest_connection;
	float closest_distance = p_max_distance;
	for (const Ref<Connection>& conn : connections) {
		if (conn->_cache.aabb.distance_to(transformed_point) > p_max_distance) {
			continue;
		}

		Vector<Vector2> points =
			get_connection_line(conn->_cache.from_pos * zoom, conn->_cache.to_pos * zoom);
		for (int i = 0; i < points.size() - 1; i++) {
			const real_t distance =
				Geometry2D::get_distance_to_segment(transformed_point, points[i], points[i + 1]);
			if (distance <= lines_thickness * 0.5 + p_max_distance && distance < closest_distance) {
				closest_connection = conn;
				closest_distance = distance;
			}
		}
	}

	return closest_connection;
}

List<Ref<GraphEdit::Connection>> GraphEdit::get_connections_intersecting_with_rect(
	const Rect2& p_rect) const
{
	Rect2 transformed_rect = p_rect;
	transformed_rect.position += get_scroll_offset();

	List<Ref<Connection>> intersecting_connections;
	for (const Ref<Connection>& conn : connections) {
		if (!conn->_cache.aabb.intersects(transformed_rect)) {
			continue;
		}

		Vector<Vector2> points =
			get_connection_line(conn->_cache.from_pos * zoom, conn->_cache.to_pos * zoom);
		for (int i = 0; i < points.size() - 1; i++) {
			if (Geometry2D::segment_intersects_rect(points[i], points[i + 1], transformed_rect)) {
				intersecting_connections.push_back(conn);
				break;
			}
		}
	}
	return intersecting_connections;
}

void GraphEdit::_draw_minimap_connection_line(const Vector2& p_from_graph_position,
	const Vector2& p_to_graph_position, const Color& p_from_color, const Color& p_to_color)
{
	Vector<Vector2> points = get_connection_line(p_from_graph_position, p_to_graph_position);
	ERR_FAIL_COND_MSG(points.size() < 2, "\"_get_connection_line()\" returned an invalid line.");
	// Convert to minimap points.
	for (Vector2& point : points) {
		point = minimap->_convert_from_graph_position(point) + minimap->minimap_offset;
	}

	// Setup polyline colors.
	LocalVector<Color> colors;
	colors.reserve(points.size());
	const Vector2& from = points[0];
	const Vector2& to = points[points.size() - 1];
	float length_inv = 1.0 / (from).distance_to(to);
	for (const Vector2& point : points) {
		float normalized_curve_position = from.distance_to(point) * length_inv;
		colors.push_back(p_from_color.lerp(p_to_color, normalized_curve_position));
	}

	minimap->draw_polyline_colors(points, Vector<Color>(colors), 0.5, lines_antialiased);
}

void GraphEdit::_top_layer_draw()
{
	if (!box_selecting) {
		return;
	}

	top_layer->draw_rect(box_selecting_rect, theme_cache.selection_fill);
	top_layer->draw_rect(box_selecting_rect, theme_cache.selection_stroke, false);
}

void GraphEdit::_draw_grid()
{
	Vector2 offset = get_scroll_offset() / zoom;
	Size2 size = get_size() / zoom;

	Point2i from_pos = (offset / float(snapping_distance)).floor();
	Point2i len = (size / float(snapping_distance)).floor() + Vector2(1, 1);

	switch (grid_pattern) {
	case GRID_PATTERN_LINES: {
		for (int i = from_pos.x; i < from_pos.x + len.x; i++) {
			Color color;

			if (Math::abs(i) % GRID_MINOR_STEPS_PER_MAJOR_LINE == 0) {
				color = theme_cache.grid_major;
			}
			else {
				color = theme_cache.grid_minor;
			}

			float base_offset = i * snapping_distance * zoom - offset.x * zoom;
			draw_line(Vector2(base_offset, 0), Vector2(base_offset, get_size().height), color);
		}

		for (int i = from_pos.y; i < from_pos.y + len.y; i++) {
			Color color;

			if (Math::abs(i) % GRID_MINOR_STEPS_PER_MAJOR_LINE == 0) {
				color = theme_cache.grid_major;
			}
			else {
				color = theme_cache.grid_minor;
			}

			float base_offset = i * snapping_distance * zoom - offset.y * zoom;
			draw_line(Vector2(0, base_offset), Vector2(get_size().width, base_offset), color);
		}
	} break;
	case GRID_PATTERN_DOTS: {
		Color transparent_grid_minor = theme_cache.grid_minor;
		transparent_grid_minor.a *= CLAMP(1.0 * (zoom - 0.4), 0, 1);

		// Minor dots.
		if (transparent_grid_minor.a != 0) {
			for (int i = from_pos.x; i < from_pos.x + len.x; i++) {
				for (int j = from_pos.y; j < from_pos.y + len.y; j++) {
					if (Math::abs(i) % GRID_MINOR_STEPS_PER_MAJOR_DOT == 0 &&
						Math::abs(j) % GRID_MINOR_STEPS_PER_MAJOR_DOT == 0) {
						continue;
					}

					float base_offset_x = i * snapping_distance * zoom - offset.x * zoom;
					float base_offset_y = j * snapping_distance * zoom - offset.y * zoom;

					draw_rect(
						Rect2(base_offset_x - 1, base_offset_y - 1, 3, 3), transparent_grid_minor);
				}
			}
		}

		// Major dots.
		if (theme_cache.grid_major.a != 0) {
			for (int i = from_pos.x - from_pos.x % GRID_MINOR_STEPS_PER_MAJOR_DOT;
				 i < from_pos.x + len.x; i += GRID_MINOR_STEPS_PER_MAJOR_DOT) {
				for (int j = from_pos.y - from_pos.y % GRID_MINOR_STEPS_PER_MAJOR_DOT;
					 j < from_pos.y + len.y; j += GRID_MINOR_STEPS_PER_MAJOR_DOT) {
					float base_offset_x = i * snapping_distance * zoom - offset.x * zoom;
					float base_offset_y = j * snapping_distance * zoom - offset.y * zoom;

					draw_rect(
						Rect2(base_offset_x - 1, base_offset_y - 1, 3, 3), theme_cache.grid_major);
				}
			}
		}

	} break;
	}
}

void GraphEdit::_zoom_callback(float p_zoom_factor, Vector2 p_origin, const InputEvent& p_event)
{
	// We need to invalidate all connections since we don't know whether
	// the user is zooming/panning at the same time.
	_invalidate_connection_line_cache();

	set_zoom_custom(zoom * p_zoom_factor, p_origin);
}

void GraphEdit::set_panning_scheme(PanningScheme p_scheme)
{
	panning_scheme = p_scheme;
	panner->set_control_scheme((ViewPanner::ControlScheme)p_scheme);
}

GraphEdit::PanningScheme GraphEdit::get_panning_scheme() const { return panning_scheme; }

void GraphEdit::set_zoom(float p_zoom) { set_zoom_custom(p_zoom, get_size() / 2); }

float GraphEdit::get_zoom() const { return zoom; }

void GraphEdit::set_zoom_step(float p_zoom_step)
{
	p_zoom_step = std::abs(p_zoom_step);
	ERR_FAIL_COND(!std::isfinite(p_zoom_step));
	if (zoom_step == p_zoom_step) {
		return;
	}

	zoom_step = p_zoom_step;
	panner->set_scroll_zoom_factor(zoom_step);
}

float GraphEdit::get_zoom_step() const { return zoom_step; }

void GraphEdit::set_zoom_min(float p_zoom_min)
{
	ERR_FAIL_COND_MSG(
		p_zoom_min > zoom_max, "Cannot set min zoom level greater than max zoom level.");

	if (zoom_min == p_zoom_min) {
		return;
	}

	zoom_min = p_zoom_min;
	set_zoom(zoom);
}

float GraphEdit::get_zoom_min() const { return zoom_min; }

void GraphEdit::set_zoom_max(float p_zoom_max)
{
	ERR_FAIL_COND_MSG(
		p_zoom_max < zoom_min, "Cannot set max zoom level lesser than min zoom level.");

	if (zoom_max == p_zoom_max) {
		return;
	}

	zoom_max = p_zoom_max;
	set_zoom(zoom);
}

float GraphEdit::get_zoom_max() const { return zoom_max; }

void GraphEdit::set_right_disconnects(bool p_enable) { right_disconnects = p_enable; }

bool GraphEdit::is_right_disconnects_enabled() const { return right_disconnects; }

void GraphEdit::add_valid_right_disconnect_type(int p_type)
{
	valid_right_disconnect_types.insert(p_type);
}

void GraphEdit::remove_valid_right_disconnect_type(int p_type)
{
	valid_right_disconnect_types.erase(p_type);
}

void GraphEdit::add_valid_left_disconnect_type(int p_type)
{
	valid_left_disconnect_types.insert(p_type);
}

void GraphEdit::remove_valid_left_disconnect_type(int p_type)
{
	valid_left_disconnect_types.erase(p_type);
}

void GraphEdit::_zoom_minus() { set_zoom(zoom / zoom_step); }

void GraphEdit::_zoom_reset() { set_zoom(1); }

void GraphEdit::_zoom_plus() { set_zoom(zoom * zoom_step); }

void GraphEdit::_update_zoom_label()
{
	int zoom_percent = static_cast<int>(Math::round(zoom * 100));
	String zoom_text = itos(zoom_percent) + "%";
	zoom_label->set_text(zoom_text);
}

void GraphEdit::_invalidate_connection_line_cache()
{
	for (Ref<Connection>& conn : connections) {
		conn->_cache.dirty = true;
	}
}

float GraphEdit::_get_shader_line_width() { return lines_thickness * theme_cache.base_scale + 4.0; }

void GraphEdit::add_valid_connection_type(int p_type, int p_with_type)
{
	ConnectionType ct(p_type, p_with_type);
	valid_connection_types.insert(ct);
}

void GraphEdit::remove_valid_connection_type(int p_type, int p_with_type)
{
	ConnectionType ct(p_type, p_with_type);
	valid_connection_types.erase(ct);
}

bool GraphEdit::is_valid_connection_type(int p_type, int p_with_type) const
{
	ConnectionType ct(p_type, p_with_type);
	return valid_connection_types.has(ct);
}

bool GraphEdit::is_snapping_enabled() const { return snapping_enabled; }

int GraphEdit::get_snapping_distance() const { return snapping_distance; }

bool GraphEdit::is_showing_grid() const { return show_grid; }

GraphEdit::GridPattern GraphEdit::get_grid_pattern() const { return grid_pattern; }

void GraphEdit::_snapping_toggled() { snapping_enabled = toggle_snapping_button->is_pressed(); }

Vector2 GraphEdit::get_minimap_size() const { return minimap->get_size(); }

float GraphEdit::get_minimap_opacity() const
{
	Color minimap_modulate = minimap->get_modulate();
	return minimap_modulate.a;
}

bool GraphEdit::is_minimap_enabled() const { return minimap_button->is_pressed(); }

bool GraphEdit::is_showing_menu() const { return show_menu; }

bool GraphEdit::is_showing_zoom_label() const { return show_zoom_label; }

bool GraphEdit::is_showing_zoom_buttons() const { return show_zoom_buttons; }

bool GraphEdit::is_showing_grid_buttons() const { return show_grid_buttons; }

bool GraphEdit::is_showing_minimap_button() const { return show_minimap_button; }

bool GraphEdit::is_showing_arrange_button() const { return show_arrange_button; }

float GraphEdit::get_connection_lines_curvature() const { return lines_curvature; }

float GraphEdit::get_connection_lines_thickness() const { return lines_thickness; }

bool GraphEdit::is_connection_lines_antialiased() const { return lines_antialiased; }

Control* GraphEdit::get_menu_hbox() { return menu_hbox; }

Ref<ViewPanner> GraphEdit::get_panner() { return panner; }

void GraphEdit::set_warped_panning(bool p_warped)
{
	warped_panning = p_warped;

	update_warped_panning();
}

void GraphEdit::update_warped_panning() { panner->setup_warped_panning(this, warped_panning); }

void GraphEdit::arrange_nodes() { arranger->arrange_nodes(); }

bool GraphEdit::is_node_hover_valid(
	const String& p_from, int p_from_port, const String& p_to, int p_to_port)
{
	return true;
}

void GraphEdit::set_zoom_custom(float p_zoom, const Vector2& p_center) {}

bool GraphEdit::is_in_port_hotzone(Vector2 const&, Vector2 const&, Vector2i const&, bool) { return true; }

void GraphEdit::set_scroll_offset(Vector2 const&) {}

bool GraphEdit::_filter_input(Vector2 const&) { return false; }

Error GraphEdit::connect_node(String const&, int, String const&, int, bool) { return OK; }

void GraphEdit::add_child_notify(Node* p_child) {}

void GraphEdit::remove_child_notify(Node* p_child) {}

GraphNode* GraphEdit::get_element_frame(const String& p_attached_graph_element)
{
	if (!linked_parent_map.has(p_attached_graph_element)) {
		return nullptr;
	}
	Node* parent = get_node_or_null(NodePath(linked_parent_map[p_attached_graph_element]));
	GraphNode* frame = dynamic_cast<GraphNode*>(parent);
	return (frame && frame->is_frame()) ? frame : nullptr;
}

Vector<String> GraphEdit::get_attached_nodes_of_frame(const String& p_graph_frame) const
{
	Vector<String> attached_nodes;
	if (!frame_attached_nodes.has(p_graph_frame)) {
		return attached_nodes;
	}
	for (const String& elem : frame_attached_nodes[p_graph_frame]) {
		attached_nodes.push_back(elem);
	}
	return attached_nodes;
}

// --- graph_edit.compat.inc ---

#ifndef DISABLE_DEPRECATED


bool GraphEdit::_is_arrange_nodes_button_hidden_bind_compat_81582() const
{
	return !is_showing_arrange_button();
}

PackedVector2Array GraphEdit::_get_connection_line_bind_compat_86158(
	const Vector2& p_from, const Vector2& p_to)
{
	return get_connection_line(p_from, p_to);
}

Error GraphEdit::_connect_node_bind_compat_97449(
	const String& p_from, int p_from_port, const String& p_to, int p_to_port)
{
	return connect_node(p_from, p_from_port, p_to, p_to_port);
}

void GraphEdit::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- graph_edit_arranger.cpp ---

int GraphEditArranger::_set_operations(
	SET_OPERATIONS p_operation, HashSet<String>& r_u, const HashSet<String>& r_v)
{
	switch (p_operation) {
	case GraphEditArranger::IS_EQUAL: {
		for (const String& E : r_u) {
			if (!r_v.has(E)) {
				return 0;
			}
		}
		return r_u.size() == r_v.size();
	} break;
	case GraphEditArranger::IS_SUBSET: {
		if (r_u.size() == r_v.size() && !r_u.size()) {
			return 1;
		}
		for (const String& E : r_u) {
			if (!r_v.has(E)) {
				return 0;
			}
		}
		return 1;
	} break;
	case GraphEditArranger::DIFFERENCE: {
		Vector<String> common;
		for (const String& E : r_u) {
			if (r_v.has(E)) {
				common.append(E);
			}
		}
		for (const String& E : common) {
			r_u.erase(E);
		}
		return r_u.size();
	} break;
	case GraphEditArranger::UNION: {
		for (const String& E : r_v) {
			if (!r_u.has(E)) {
				r_u.insert(E);
			}
		}
		return r_u.size();
	} break;
	default:
		break;
	}
	return -1;
}

HashMap<int, Vector<String>> GraphEditArranger::_layering(
	const HashSet<String>& r_selected_nodes,
	const HashMap<String, HashSet<String>>& r_upper_neighbours)
{
	HashMap<int, Vector<String>> l;

	HashSet<String> p(r_selected_nodes);
	HashSet<String> q(r_selected_nodes);
	HashSet<String> u;
	HashSet<String> z;
	int current_layer = 0;
	bool selected = false;

	while (!_set_operations(GraphEditArranger::IS_EQUAL, q, u)) {
		_set_operations(GraphEditArranger::DIFFERENCE, p, u);
		for (const String& E : p) {
			HashSet<String> n(r_upper_neighbours[E]);
			if (_set_operations(GraphEditArranger::IS_SUBSET, n, z)) {
				Vector<String> t;
				t.push_back(E);
				if (!l.has(current_layer)) {
					l.insert(current_layer, Vector<String>{});
				}
				selected = true;
				t.append_array(l[current_layer]);
				l.insert(current_layer, t);
				u.insert(E);
			}
		}
		if (!selected) {
			current_layer++;
			uint32_t previous_size_z = z.size();
			_set_operations(GraphEditArranger::UNION, z, u);
			if (z.size() == previous_size_z) {
				WARN_PRINT(
					"Graph contains cycle(s). The cycle(s) will not be rearranged accurately.");
				Vector<String> t;
				if (l.has(0)) {
					t.append_array(l[0]);
				}
				for (const String& E : p) {
					t.push_back(E);
				}
				l.insert(0, t);
				break;
			}
		}
		selected = false;
	}

	return l;
}



void GraphEditArranger::arrange_nodes() {}

// --- view_panner.cpp ---

void ViewPanner::release_pan_key()
{
	pan_key_pressed = false;
	if (drag_type == DragType::DRAG_TYPE_PAN) {
		drag_type = DragType::DRAG_TYPE_NONE;
	}
}

void ViewPanner::set_control_scheme(ControlScheme p_scheme) { control_scheme = p_scheme; }

void ViewPanner::set_enable_rmb(bool p_enable) { enable_rmb = p_enable; }

void ViewPanner::set_pan_shortcut(Ref<Shortcut> p_shortcut)
{
	pan_view_shortcut = p_shortcut;
	pan_key_pressed = false;
}

void ViewPanner::set_simple_panning_enabled(bool p_enabled) { simple_panning_enabled = p_enabled; }

void ViewPanner::set_scroll_speed(int p_scroll_speed)
{
	ERR_FAIL_COND(p_scroll_speed <= 0);
	scroll_speed = p_scroll_speed;
}

void ViewPanner::set_scroll_zoom_factor(float p_scroll_zoom_factor)
{
	ERR_FAIL_COND(p_scroll_zoom_factor <= 1.0);
	scroll_zoom_factor = p_scroll_zoom_factor;
}

void ViewPanner::set_pan_axis(PanAxis p_pan_axis) { pan_axis = p_pan_axis; }

void ViewPanner::set_zoom_style(ZoomStyle p_zoom_style) { zoom_style = p_zoom_style; }

void ViewPanner::setup(ControlScheme p_scheme, Ref<Shortcut> p_shortcut, bool p_simple_panning)
{
	set_control_scheme(p_scheme);
	set_pan_shortcut(p_shortcut);
	set_simple_panning_enabled(p_simple_panning);
}

void ViewPanner::setup_warped_panning(Node* p_owner, bool p_allowed)
{
	warped_panning_owner = p_allowed ? p_owner : nullptr;
}

bool ViewPanner::is_panning() const
{
	return (drag_type == DragType::DRAG_TYPE_PAN) || pan_key_pressed;
}

void ViewPanner::set_force_drag(bool p_force) { force_drag = p_force; }



bool ViewPanner::gui_input(const InputEvent&, Rect2) { return false; }

// --- graph_node.cpp ---

void GraphNode::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;

	if (mode == MODE_ELEMENT) {
		if (titlebar_hbox) {
			remove_child(titlebar_hbox);
			memdelete(titlebar_hbox);
			titlebar_hbox = nullptr;
			title_label = nullptr;
		}
		set_focus_mode(FOCUS_NONE);
	}
	else {
		if (!titlebar_hbox) {
			titlebar_hbox = Control::create_hbox();
			titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
			titlebar_hbox->set_use_parent_material(true);
			add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

			title_label = memnew(Label);
			title_label->set_h_size_flags(SIZE_EXPAND_FILL);
			titlebar_hbox->add_child(title_label);
		}
		if (title_label) {
			title_label->set_theme_type_variation(
				mode == MODE_FRAME ? "GraphFrameTitleLabel" : "GraphNodeTitleLabel");
			title_label->set_horizontal_alignment(
				mode == MODE_FRAME ? HORIZONTAL_ALIGNMENT_CENTER : HORIZONTAL_ALIGNMENT_LEFT);
			title_label->set_use_parent_material(mode == MODE_FRAME);
			title_label->set_text(title);
		}
		set_focus_mode(mode == MODE_NODE ? FOCUS_ACCESSIBILITY : FOCUS_NONE);
	}
	queue_redraw();
}

#ifdef TOOLS_ENABLED
void GraphNode::_edit_set_position(const Point2& p_position) {}
#endif

void GraphNode::set_position_offset(const Vector2& p_offset)
{
	if (position_offset == p_offset) {
		return;
	}
	position_offset = p_offset;
	queue_redraw();
}

void GraphNode::set_selected(bool p_selected)
{
	if (!is_selectable() && p_selected) {
		return;
	}
	if (selected == p_selected) {
		return;
	}
	selected = p_selected;
	queue_redraw();
}

void GraphNode::set_drag(bool p_drag)
{
	if (p_drag) {
		drag_from = get_position_offset();
	}
}

void GraphNode::set_resizable(bool p_enable)
{
	if (resizable == p_enable) {
		return;
	}
	resizable = p_enable;
	queue_redraw();
}

void GraphNode::set_selectable(bool p_selectable)
{
	if (!p_selectable) {
		set_selected(false);
	}
	selectable = p_selectable;
}

void GraphNode::set_title(const String& p_title)
{
	if (title == p_title) {
		return;
	}
	title = p_title;
	if (title_label) {
		title_label->set_text(title);
	}
	update_minimum_size();
}

Size2 GraphNode::get_titlebar_size() const
{
	if (!titlebar_hbox) {
		return Size2();
	}
	Size2 min_size = titlebar_hbox->get_bound_minimum_size();
	if (theme_cache.titlebar.is_valid()) {
		min_size += theme_cache.titlebar->get_minimum_size();
	}
	return min_size;
}

void GraphNode::set_autoshrink_enabled(bool p_enable)
{
	if (autoshrink_enabled == p_enable) {
		return;
	}
	autoshrink_enabled = p_enable;
	update_minimum_size();
	queue_redraw();
}

void GraphNode::set_autoshrink_margin(const int& p_margin)
{
	if (autoshrink_margin == p_margin) {
		return;
	}
	autoshrink_margin = p_margin;
	update_minimum_size();
	queue_redraw();
}

void GraphNode::set_tint_color_enabled(bool p_enable)
{
	if (tint_color_enabled == p_enable) {
		return;
	}
	tint_color_enabled = p_enable;
	queue_redraw();
}

void GraphNode::set_tint_color(const Color& p_tint_color)
{
	if (tint_color == p_tint_color) {
		return;
	}
	tint_color = p_tint_color;
	queue_redraw();
}

void GraphNode::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_SORT_CHILDREN: {
		_resort();
	} break;

	case NOTIFICATION_DRAW: {
		if (mode == MODE_FRAME) {
			if (theme_cache.panel.is_null() || theme_cache.titlebar.is_null() || !titlebar_hbox) {
				break;
			}
			// Used for layout calculations.
			Ref<StyleBox> sb_panel = theme_cache.panel;
			Ref<StyleBox> sb_titlebar = theme_cache.titlebar;

			// Used for drawing.
			Ref<StyleBox> sb_to_draw_panel =
				(selected && theme_cache.panel_selected.is_valid()) ? theme_cache.panel_selected
																	: sb_panel;
			Ref<StyleBox> sb_to_draw_titlebar =
				(selected && theme_cache.titlebar_selected.is_valid())
					? theme_cache.titlebar_selected
					: sb_titlebar;

			Rect2 titlebar_rect(
				Point2(), titlebar_hbox->get_bound_minimum_size() + sb_titlebar->get_minimum_size());
			Size2 body_size = get_size();
			titlebar_rect.size.width = body_size.width;
			body_size.y -= titlebar_rect.size.height;
			Rect2 body_rect(Point2(0, titlebar_rect.size.height), body_size);

			// Draw body stylebox.
			draw_style_box(sb_to_draw_panel.ptr(), body_rect);

			// Draw title bar stylebox above.
			draw_style_box(sb_to_draw_titlebar.ptr(), titlebar_rect);

			// Only draw the resize handle if the frame is not auto-resizing.
			if (resizable && !autoshrink_enabled && theme_cache.resizer.is_valid()) {
				Ref<Texture2D> resizer = theme_cache.resizer;
				draw_texture(
					resizer.ptr(), get_size() - resizer->get_size(), theme_cache.resizer_color);
			}
		}
	} break;
	}
}

void GraphNode::draw_port(int p_slot_index, Point2i p_pos, bool p_left, const Color& p_color)
{
	if (mode != MODE_NODE) {
		return;
	}
	Slot slot = slot_table[p_slot_index];
	Ref<Texture2D> port_icon = p_left ? slot.custom_port_icon_left : slot.custom_port_icon_right;

	if (port_icon.is_null()) {
		port_icon = theme_cache.port;
	}
	if (port_icon.is_null()) {
		return;
	}

	Point2 icon_offset = -port_icon->get_size() * 0.5;
	port_icon->draw(get_canvas_item(), p_pos + icon_offset, p_color);
}

void GraphNode::set_slot(int p_slot_index, bool p_enable_left, int p_type_left,
	const Color& p_color_left, bool p_enable_right, int p_type_right, const Color& p_color_right,
	const Ref<Texture2D>& p_custom_left, const Ref<Texture2D>& p_custom_right, bool p_draw_stylebox)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set slot with index (%d) lesser than zero.", p_slot_index));

	if (!p_enable_left && p_type_left == 0 && p_color_left == Color(1, 1, 1, 1) &&
		!p_enable_right && p_type_right == 0 && p_color_right == Color(1, 1, 1, 1) &&
		p_custom_left.is_null() && p_custom_right.is_null()) {
		slot_table.erase(p_slot_index);
		return;
	}

	Slot s;
	s.enable_left = p_enable_left;
	s.type_left = p_type_left;
	s.color_left = p_color_left;
	s.custom_port_icon_left = p_custom_left;
	s.enable_right = p_enable_right;
	s.type_right = p_type_right;
	s.color_right = p_color_right;
	s.custom_port_icon_right = p_custom_right;
	s.draw_stylebox = p_draw_stylebox;
	slot_table[p_slot_index] = s;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::clear_slot(int p_slot_index)
{
	slot_table.erase(p_slot_index);
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::clear_all_slots()
{
	slot_table.clear();
	port_pos_dirty = true;
	queue_redraw();
}

bool GraphNode::is_slot_enabled_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_left;
}

void GraphNode::set_slot_enabled_left(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set enable_left for the slot (%d).", p_slot_index));
	slot_table[p_slot_index].enable_left = p_enable;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::set_slot_type_left(int p_slot_index, int p_type)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].type_left = p_type;
	port_pos_dirty = true;
	queue_redraw();
}

int GraphNode::get_slot_type_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_left;
}

void GraphNode::set_slot_color_left(int p_slot_index, const Color& p_color)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].color_left = p_color;
	port_pos_dirty = true;
	queue_redraw();
}

Color GraphNode::get_slot_color_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_left;
}

void GraphNode::set_slot_custom_icon_left(int p_slot_index, const Ref<Texture2D>& p_custom_icon)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].custom_port_icon_left = p_custom_icon;
	port_pos_dirty = true;
	queue_redraw();
}

Ref<Texture2D> GraphNode::get_slot_custom_icon_left(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Ref<Texture2D>();
	}
	return slot_table[p_slot_index].custom_port_icon_left;
}

bool GraphNode::is_slot_enabled_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].enable_right;
}

void GraphNode::set_slot_enabled_right(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND_MSG(
		p_slot_index < 0, vformat("Cannot set enable_right for the slot (%d).", p_slot_index));
	slot_table[p_slot_index].enable_right = p_enable;
	port_pos_dirty = true;
	queue_redraw();
}

void GraphNode::set_slot_type_right(int p_slot_index, int p_type)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].type_right = p_type;
	port_pos_dirty = true;
	queue_redraw();
}

int GraphNode::get_slot_type_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return 0;
	}
	return slot_table[p_slot_index].type_right;
}

void GraphNode::set_slot_color_right(int p_slot_index, const Color& p_color)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].color_right = p_color;
	port_pos_dirty = true;
	queue_redraw();
}

Color GraphNode::get_slot_color_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Color(1, 1, 1, 1);
	}
	return slot_table[p_slot_index].color_right;
}

void GraphNode::set_slot_custom_icon_right(int p_slot_index, const Ref<Texture2D>& p_custom_icon)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].custom_port_icon_right = p_custom_icon;
	port_pos_dirty = true;
	queue_redraw();
}

Ref<Texture2D> GraphNode::get_slot_custom_icon_right(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return Ref<Texture2D>();
	}
	return slot_table[p_slot_index].custom_port_icon_right;
}

bool GraphNode::is_slot_draw_stylebox(int p_slot_index) const
{
	if (!slot_table.has(p_slot_index)) {
		return false;
	}
	return slot_table[p_slot_index].draw_stylebox;
}

void GraphNode::set_slot_draw_stylebox(int p_slot_index, bool p_enable)
{
	ERR_FAIL_COND(!slot_table.has(p_slot_index));
	slot_table[p_slot_index].draw_stylebox = p_enable;
	queue_redraw();
}

void GraphNode::set_ignore_invalid_connection_type(bool p_ignore)
{
	ignore_invalid_connection_type = p_ignore;
}

bool GraphNode::is_ignoring_valid_connection_type() const { return ignore_invalid_connection_type; }

bool GraphNode::has_point(const Point2& p_point) const
{
	if (mode == MODE_FRAME) {
		if (theme_cache.resizer.is_valid()) {
			Ref<Texture2D> resizer = theme_cache.resizer;
			if (Rect2(get_size() - resizer->get_size(), resizer->get_size()).has_point(p_point)) {
				return true;
			}
		}

		// For grabbing on the titlebar.
		int titlebar_height = get_titlebar_size().height;
		if (Rect2(0, 0, get_size().width, titlebar_height).has_point(p_point)) {
			return true;
		}

		// Allow grabbing on all sides of the frame.
		Rect2 frame_rect = Rect2(0, 0, get_size().width, get_size().height);
		Rect2 no_drag_rect = frame_rect.grow(-drag_margin);

		if (frame_rect.has_point(p_point) && !no_drag_rect.has_point(p_point)) {
			return true;
		}

		return false;
	}

	return Control::has_point(p_point);
}

Size2 GraphNode::get_minimum_size() const { return _get_minimum_size(false); }

Size2 GraphNode::get_desired_size() const { return _get_minimum_size(true); }

int GraphNode::get_input_port_count()
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return left_port_cache.size();
}

int GraphNode::get_output_port_count()
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	return right_port_cache.size();
}

Vector2 GraphNode::get_input_port_position(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Vector2();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Vector2());
	return left_port_cache[p_port_idx].pos;
}

int GraphNode::get_input_port_type(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), 0);
	return left_port_cache[p_port_idx].type;
}

Color GraphNode::get_input_port_color(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Color();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), Color());
	return left_port_cache[p_port_idx].color;
}

int GraphNode::get_input_port_slot(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return -1;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, left_port_cache.size(), -1);
	return left_port_cache[p_port_idx].slot_index;
}

Vector2 GraphNode::get_output_port_position(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Vector2();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Vector2());
	return right_port_cache[p_port_idx].pos;
}

int GraphNode::get_output_port_type(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return 0;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), 0);
	return right_port_cache[p_port_idx].type;
}

Color GraphNode::get_output_port_color(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return Color();
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), Color());
	return right_port_cache[p_port_idx].color;
}

int GraphNode::get_output_port_slot(int p_port_idx)
{
	if (mode != MODE_NODE) {
		return -1;
	}
	if (port_pos_dirty) {
		_port_pos_update();
	}

	ERR_FAIL_INDEX_V(p_port_idx, right_port_cache.size(), -1);
	return right_port_cache[p_port_idx].slot_index;
}

Control::CursorShape GraphNode::get_cursor_shape(const Point2& p_pos) const
{
	const bool can_resize = (mode == MODE_FRAME) ? (resizable && !autoshrink_enabled) : resizable;
	if (can_resize && theme_cache.resizer.is_valid()) {
		if (resizing || (p_pos.x > get_size().x - theme_cache.resizer->get_width() &&
							p_pos.y > get_size().y - theme_cache.resizer->get_height())) {
			return CURSOR_FDIAGSIZE;
		}
	}

	return Control::get_cursor_shape(p_pos);
}

Vector<int> GraphNode::get_allowed_size_flags_horizontal() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> GraphNode::get_allowed_size_flags_vertical() const
{
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_EXPAND);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

void GraphNode::set_slots_focus_mode(Control::FocusMode p_focus_mode)
{
	slots_focus_mode = p_focus_mode;
}

Control::FocusMode GraphNode::get_slots_focus_mode() const { return slots_focus_mode; }

GraphNode::GraphNode(Mode p_mode) : mode(p_mode)
{
	if (mode != MODE_ELEMENT) {
		titlebar_hbox = Control::create_hbox();
		titlebar_hbox->set_h_size_flags(SIZE_EXPAND_FILL);
		titlebar_hbox->set_use_parent_material(true);
		add_child(titlebar_hbox, false, INTERNAL_MODE_FRONT);

		title_label = memnew(Label);
		title_label->set_theme_type_variation(
			mode == MODE_FRAME ? "GraphFrameTitleLabel" : "GraphNodeTitleLabel");
		title_label->set_h_size_flags(SIZE_EXPAND_FILL);
		if (mode == MODE_FRAME) {
			title_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
			title_label->set_use_parent_material(true);
		}
		titlebar_hbox->add_child(title_label);
	}

	set_mouse_filter(MOUSE_FILTER_STOP);
	if (mode == MODE_NODE) {
		set_focus_mode(FOCUS_ACCESSIBILITY);
	}
}

void GraphNode::_port_pos_update()
{
	port_pos_dirty = false;
}

Size2 GraphNode::_get_minimum_size(bool) const { return Size2(); }

String GraphNode::get_accessibility_container_name(const Node* p_child) const { return String(); }

String GraphNode::_get_accessibility_name() const { return String(); }

void GraphNode::_resort()
{
	if (mode == MODE_FRAME && titlebar_hbox && theme_cache.titlebar.is_valid()) {
		Ref<StyleBox> sb_titlebar = theme_cache.titlebar;
		Size2 titlebar_size = Size2(get_size().width, titlebar_hbox->get_size().height);
		titlebar_size -= sb_titlebar->get_minimum_size();
		Rect2 titlebar_rect = Rect2(sb_titlebar->get_offset(), titlebar_size);
		fit_child_in_rect(titlebar_hbox, titlebar_rect);
	}
}
