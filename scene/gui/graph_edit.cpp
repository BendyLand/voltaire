/**************************************************************************/
/*  graph_edit.cpp                                                        */
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

#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "core/math/math_funcs.h"
#include "core/os/keyboard.h"
#include "graph_edit.compat.inc"
#include "graph_edit.h"
#include "scene/2d/line_2d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/graph_edit_arranger.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/view_panner.h"
#include "scene/resources/material.h"
#include "scene/resources/style_box_flat.h"
#include "scene/theme/theme_db.h"

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
	const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port)
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

int GraphEdit::get_connection_count(const StringName& p_node, int p_port)
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

void GraphEdit::_graph_element_visibility_changed(GraphElement* p_graph_element)
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

void GraphEdit::_zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event)
{
	// We need to invalidate all connections since we don't know whether
	// the user is zooming/panning at the same time.
	_invalidate_connection_line_cache();

	set_zoom_custom(zoom * p_zoom_factor, p_origin);
}

void GraphEdit::reset_all_connection_activity()
{
	ERR_FAIL_NULL_MSG(connections_layer, "connections_layer is missing.");

	bool changed = false;
	for (Ref<Connection>& conn : connections) {
		if (conn->activity > 0) {
			changed = true;
			conn->_cache.dirty = true;
		}
		conn->activity = 0;
	}
	if (changed) {
		connections_layer->queue_redraw();
	}
}

void GraphEdit::clear_connections()
{
	ERR_FAIL_NULL_MSG(connections_layer, "connections_layer is missing.");

	for (Ref<Connection>& conn : connections) {
		conn->_cache.line->queue_free();
	}

	connections.clear();
	connection_map.clear();

	minimap->queue_redraw();
	queue_redraw();
	connections_layer->queue_redraw();
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

void GraphEdit::set_snapping_enabled(bool p_enable)
{
	if (snapping_enabled == p_enable) {
		return;
	}

	snapping_enabled = p_enable;
	toggle_snapping_button->set_pressed(p_enable);
	queue_redraw();
}

bool GraphEdit::is_snapping_enabled() const { return snapping_enabled; }

void GraphEdit::set_snapping_distance(int p_snapping_distance)
{
	ERR_FAIL_COND_MSG(p_snapping_distance < GRID_MIN_SNAPPING_DISTANCE ||
						  p_snapping_distance > GRID_MAX_SNAPPING_DISTANCE,
		vformat("GraphEdit's snapping distance must be between %d and %d (inclusive)",
			GRID_MIN_SNAPPING_DISTANCE, GRID_MAX_SNAPPING_DISTANCE));
	snapping_distance = p_snapping_distance;
	snapping_distance_spinbox->set_value(p_snapping_distance);
	queue_redraw();
}

int GraphEdit::get_snapping_distance() const { return snapping_distance; }

void GraphEdit::set_show_grid(bool p_show)
{
	if (show_grid == p_show) {
		return;
	}

	show_grid = p_show;
	toggle_grid_button->set_pressed(p_show);
	queue_redraw();
}

bool GraphEdit::is_showing_grid() const { return show_grid; }

void GraphEdit::set_grid_pattern(GridPattern p_pattern)
{
	if (grid_pattern == p_pattern) {
		return;
	}

	grid_pattern = p_pattern;
	queue_redraw();
}

GraphEdit::GridPattern GraphEdit::get_grid_pattern() const { return grid_pattern; }

void GraphEdit::_snapping_toggled() { snapping_enabled = toggle_snapping_button->is_pressed(); }

void GraphEdit::_snapping_distance_changed(double)
{
	snapping_distance = snapping_distance_spinbox->get_value();
	queue_redraw();
}

void GraphEdit::_show_grid_toggled()
{
	show_grid = toggle_grid_button->is_pressed();
	queue_redraw();
}

void GraphEdit::set_minimap_size(Vector2 p_size)
{
	minimap->set_size(p_size);
	Vector2 minimap_size =
		minimap->get_size(); // The size might've been adjusted by the minimum size.

	minimap->set_anchors_preset(Control::PRESET_BOTTOM_RIGHT);
	minimap->set_offset(Side::SIDE_LEFT, -minimap_size.width - MINIMAP_OFFSET);
	minimap->set_offset(Side::SIDE_TOP, -minimap_size.height - MINIMAP_OFFSET);
	minimap->set_offset(Side::SIDE_RIGHT, -MINIMAP_OFFSET);
	minimap->set_offset(Side::SIDE_BOTTOM, -MINIMAP_OFFSET);
	minimap->queue_redraw();
}

Vector2 GraphEdit::get_minimap_size() const { return minimap->get_size(); }

void GraphEdit::set_minimap_opacity(float p_opacity)
{
	if (minimap->get_modulate().a == p_opacity) {
		return;
	}
	minimap->set_modulate(Color(1, 1, 1, p_opacity));
	minimap->queue_redraw();
}

float GraphEdit::get_minimap_opacity() const
{
	Color minimap_modulate = minimap->get_modulate();
	return minimap_modulate.a;
}

void GraphEdit::set_minimap_enabled(bool p_enable)
{
	if (minimap_button->is_pressed() == p_enable) {
		return;
	}
	minimap_button->set_pressed(p_enable);
	_minimap_toggled();
	minimap->queue_redraw();
}

bool GraphEdit::is_minimap_enabled() const { return minimap_button->is_pressed(); }

void GraphEdit::set_show_menu(bool p_hidden)
{
	show_menu = p_hidden;
	menu_panel->set_visible(show_menu);
}

bool GraphEdit::is_showing_menu() const { return show_menu; }

void GraphEdit::set_show_zoom_label(bool p_hidden)
{
	show_zoom_label = p_hidden;
	zoom_label->set_visible(show_zoom_label);
}

bool GraphEdit::is_showing_zoom_label() const { return show_zoom_label; }

void GraphEdit::set_show_zoom_buttons(bool p_hidden)
{
	show_zoom_buttons = p_hidden;

	zoom_minus_button->set_visible(show_zoom_buttons);
	zoom_reset_button->set_visible(show_zoom_buttons);
	zoom_plus_button->set_visible(show_zoom_buttons);
}

bool GraphEdit::is_showing_zoom_buttons() const { return show_zoom_buttons; }

void GraphEdit::set_show_grid_buttons(bool p_hidden)
{
	show_grid_buttons = p_hidden;

	toggle_grid_button->set_visible(show_grid_buttons);
	toggle_snapping_button->set_visible(show_grid_buttons);
	snapping_distance_spinbox->set_visible(show_grid_buttons);
}

bool GraphEdit::is_showing_grid_buttons() const { return show_grid_buttons; }

void GraphEdit::set_show_minimap_button(bool p_hidden)
{
	show_minimap_button = p_hidden;
	minimap_button->set_visible(show_minimap_button);
}

bool GraphEdit::is_showing_minimap_button() const { return show_minimap_button; }

void GraphEdit::set_show_arrange_button(bool p_hidden)
{
	show_arrange_button = p_hidden;
	arrange_button->set_visible(show_arrange_button);
}

bool GraphEdit::is_showing_arrange_button() const { return show_arrange_button; }

void GraphEdit::_minimap_toggled()
{
	if (is_minimap_enabled()) {
		minimap->set_visible(true);
		minimap->queue_redraw();
	}
	else {
		minimap->set_visible(false);
	}
}

void GraphEdit::set_connection_lines_curvature(float p_curvature)
{
	ERR_FAIL_NULL_MSG(connections_layer, "connections_layer is missing.");

	lines_curvature = p_curvature;
	_invalidate_connection_line_cache();
	connections_layer->queue_redraw();
	queue_redraw();
}

float GraphEdit::get_connection_lines_curvature() const { return lines_curvature; }

void GraphEdit::set_connection_lines_thickness(float p_thickness)
{
	ERR_FAIL_NULL_MSG(connections_layer, "connections_layer is missing.");
	ERR_FAIL_COND_MSG
(
		p_thickness < 0, "Connection lines thickness must be greater than or equal to 0.");

	if (lines_thickness == p_thickness) {
		return;
	}
	lines_thickness = p_thickness;
	_invalidate_connection_line_cache();
	connections_layer->queue_redraw();
	queue_redraw();
}

float GraphEdit::get_connection_lines_thickness() const { return lines_thickness; }

void GraphEdit::set_connection_lines_antialiased(bool p_antialiased)
{
	ERR_FAIL_NULL_MSG(connections_layer, "connections_layer is missing.");

	if (lines_antialiased == p_antialiased) {
		return;
	}
	lines_antialiased = p_antialiased;
	_invalidate_connection_line_cache();
	connections_layer->queue_redraw();
	queue_redraw();
}

bool GraphEdit::is_connection_lines_antialiased() const { return lines_antialiased; }

HBoxContainer* GraphEdit::get_menu_hbox() { return menu_hbox; }

Ref<ViewPanner> GraphEdit::get_panner() { return panner; }

void GraphEdit::set_warped_panning(bool p_warped)
{
	warped_panning = p_warped;
	update_warped_panning();
}

void GraphEdit::update_warped_panning() { panner->setup_warped_panning(this, warped_panning); }

void GraphEdit::arrange_nodes() { arranger->arrange_nodes(); }

bool GraphEdit::is_node_hover_valid(
	const StringName& p_from, int p_from_port, const StringName& p_to, int p_to_port)
{
	return true;
}


