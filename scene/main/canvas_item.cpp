/**************************************************************************/
/*  canvas_item.cpp                                                       */
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

#include "canvas_item.compat.inc"
#include "canvas_item.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, RenderingServer);

#include "scene/2d/canvas_group.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/dpi_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/multimesh.h"
#include "scene/resources/style_box.h"
#include "scene/resources/world_2d.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/rendering_server.h"

#define ERR_DRAW_GUARD                                                                             \
	ERR_FAIL_COND_MSG(!drawing,                                                                    \
		"Drawing is only allowed inside this node's `_draw()`, functions connected to "            \
		"its `draw` signal, or when it receives NOTIFICATION_DRAW.")

#ifdef DEBUG_ENABLED
bool CanvasItem::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	if (_edit_use_rect()) {
		return _edit_get_rect().has_point(p_point);
	}
	else {
		return p_point.length() < p_tolerance;
	}
}
#endif // DEBUG_ENABLED

#ifdef TOOLS_ENABLED
Transform2D CanvasItem::_edit_get_transform() const
{
	return Transform2D(_edit_get_rotation(), _edit_get_position() + _edit_get_pivot());
}
#endif // TOOLS_ENABLED

bool CanvasItem::is_visible_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible && parent_visible_in_tree;
}

void CanvasItem::_propagate_visibility_changed(bool p_parent_visible_in_tree)
{
	parent_visible_in_tree = p_parent_visible_in_tree;
	if (!visible) {
		return;
	}

	_handle_visibility_change(p_parent_visible_in_tree);
}

void CanvasItem::show()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(true);
}

void CanvasItem::hide()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(false);
}

bool CanvasItem::is_visible() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible;
}

CanvasItem* CanvasItem::current_item_drawn = nullptr;

CanvasItem* CanvasItem::get_current_item_drawn() { return current_item_drawn; }

Transform2D CanvasItem::get_global_transform_with_canvas() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (canvas_layer) {
		return canvas_layer->get_final_transform() * get_global_transform();
	}
	else if (is_inside_tree()) {
		return get_viewport()->get_canvas_transform() * get_global_transform();
	}
	else {
		return get_global_transform();
	}
}

Transform2D CanvasItem::get_screen_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	ERR_FAIL_COND_V(!is_inside_tree(), Transform2D());
	return get_viewport()->get_popup_base_transform() * get_global_transform_with_canvas();
}

Transform2D CanvasItem::get_global_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());

	if (_is_global_invalid()) {
		// This code can enter multiple times from threads if dirty, this is expected.
		const CanvasItem* pi = get_parent_item();
		Transform2D new_global;
		if (pi) {
			new_global = pi->get_global_transform() * get_transform();
		}
		else {
			new_global = get_transform();
		}

		global_transform = new_global;
		_set_global_invalid(false);
	}

	return global_transform;
}

// Same as get_global_transform() but no reset for `global_invalid`.
Transform2D CanvasItem::get_global_transform_const() const
{
	if (_is_global_invalid()) {
		const CanvasItem* pi = get_parent_item();
		if (pi) {
			global_transform = pi->get_global_transform_const() * get_transform();
		}
		else {
			global_transform = get_transform();
		}
	}

	return global_transform;
}

void CanvasItem::_set_global_invalid(bool p_invalid) const
{
	if (is_group_processing()) {
		if (p_invalid) {
			global_invalid.mt.set();
		}
		else {
			global_invalid.mt.clear();
		}
	}
	else {
		global_invalid.st = p_invalid;
	}
}

void CanvasItem::_top_level_raise_self()
{
	if (!is_inside_tree()) {
		return;
	}

	if (canvas_layer) {
		RenderingServer::get_singleton()->canvas_item_set_draw_index(
			canvas_item, canvas_layer->get_sort_index());
	}
	else {
		RenderingServer::get_singleton()->canvas_item_set_draw_index(
			canvas_item, get_viewport()->gui_get_canvas_sort_index());
	}
}

bool CanvasItem::_is_oversampling_with_scale() const
{
	if (oversampling_with_scale == OVERSAMPLING_WITH_SCALE_PARENT_NODE) {
		CanvasItem* ci = get_parent_item();
		if (ci) {
			return ci->_is_oversampling_with_scale();
		}
	}
	return oversampling_with_scale == OVERSAMPLING_WITH_SCALE_ENABLED;
}

CanvasItem::OversamplingWithScale CanvasItem::get_oversampling_with_scale() const
{
	return oversampling_with_scale;
}

void CanvasItem::_update_oversampling(bool p_propagate)
{
	if (p_propagate) {
		for (uint32_t n = 0; n < data.canvas_item_children.size(); n++) {
			CanvasItem* ci = data.canvas_item_children[n];
			if (!ci->top_level &&
				ci->get_oversampling_with_scale() == OVERSAMPLING_WITH_SCALE_PARENT_NODE) {
				ci->_update_oversampling(p_propagate);
			}
		}
	}

	if (parent_visible_in_tree) {
		bool new_oversampling_with_scale = _is_oversampling_with_scale();
		if (new_oversampling_with_scale) {
			double new_os =
				MAX(get_global_transform().get_scale().x, get_global_transform().get_scale().y);
			if (new_os != oversampling_override) {
				oversampling_override = new_os;
				queue_redraw();
			}
		}
		else {
			oversampling_override = -1.0;
		}
		if (is_oversampling_with_scale_cache != new_oversampling_with_scale) {
			is_oversampling_with_scale_cache = new_oversampling_with_scale;
			queue_redraw();
		}
	}
}

void CanvasItem::set_oversampling_with_scale(CanvasItem::OversamplingWithScale p_mode)
{
	if (oversampling_with_scale == p_mode) {
		return;
	}
	oversampling_with_scale = p_mode;
	_update_oversampling(true);
}

void CanvasItem::_window_visibility_changed()
{
	_propagate_visibility_changed(window->is_visible());
}

void CanvasItem::move_to_front()
{
	ERR_MAIN_THREAD_GUARD;
	if (!get_parent()) {
		return;
	}
	get_parent()->move_child(this, -1);
}

void CanvasItem::set_modulate(const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	if (modulate == p_modulate) {
		return;
	}

	modulate = p_modulate;
	RenderingServer::get_singleton()->canvas_item_set_modulate(canvas_item, modulate);
}

Color CanvasItem::get_modulate() const
{
	ERR_READ_THREAD_GUARD_V(Color());
	return modulate;
}

Color CanvasItem::get_modulate_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(Color());
	Color final_modulate = modulate;
	CanvasItem* parent_item = get_parent_item();
	while (parent_item) {
		final_modulate *= parent_item->get_modulate();
		parent_item = parent_item->get_parent_item();
	}
	return final_modulate;
}

void CanvasItem::_top_level_changed_on_parent()
{
	// Inform children that top_level status has changed on a parent.
	_top_level_changed();
}

bool CanvasItem::is_set_as_top_level() const { return top_level; }

void CanvasItem::set_self_modulate(const Color& p_self_modulate)
{
	ERR_THREAD_GUARD;
	if (self_modulate == p_self_modulate) {
		return;
	}

	self_modulate = p_self_modulate;
	RenderingServer::get_singleton()->canvas_item_set_self_modulate(canvas_item, self_modulate);
}

Color CanvasItem::get_self_modulate() const
{
	ERR_READ_THREAD_GUARD_V(Color());
	return self_modulate;
}

void CanvasItem::set_light_mask(int p_light_mask)
{
	ERR_THREAD_GUARD;
	if (light_mask == p_light_mask) {
		return;
	}

	light_mask = p_light_mask;
	RS::get_singleton()->canvas_item_set_light_mask(canvas_item, p_light_mask);
}

int CanvasItem::get_light_mask() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return light_mask;
}

void CanvasItem::set_z_index(int p_z)
{
	ERR_THREAD_GUARD;
	ERR_FAIL_COND_MSG(p_z < RSE::CANVAS_ITEM_Z_MIN || p_z > RSE::CANVAS_ITEM_Z_MAX,
		vformat("Tried to set Z index to an invalid value: %d. Z index must be between %d and %d.",
			p_z, RSE::CANVAS_ITEM_Z_MIN, RSE::CANVAS_ITEM_Z_MAX));
	z_index = p_z;
	RS::get_singleton()->canvas_item_set_z_index(canvas_item, z_index);
	update_configuration_warnings();
}

void CanvasItem::set_z_as_relative(bool p_enabled)
{
	ERR_THREAD_GUARD;
	if (z_relative == p_enabled) {
		return;
	}
	z_relative = p_enabled;
	RS::get_singleton()->canvas_item_set_z_as_relative_to_parent(canvas_item, p_enabled);
}

bool CanvasItem::is_z_relative() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return z_relative;
}

int CanvasItem::get_z_index() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return z_index;
}

int CanvasItem::get_effective_z_index() const
{
	ERR_READ_THREAD_GUARD_V(0);
	int effective_z_index = z_index;
	if (is_z_relative()) {
		CanvasItem* p = get_parent_item();
		if (p) {
			effective_z_index += p->get_effective_z_index();
		}
	}
	return effective_z_index;
}

void CanvasItem::set_y_sort_enabled(bool p_enabled)
{
	ERR_THREAD_GUARD;
	y_sort_enabled = p_enabled;
	RS::get_singleton()->canvas_item_set_sort_children_by_y(canvas_item, y_sort_enabled);
}

bool CanvasItem::is_y_sort_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return y_sort_enabled;
}

void CanvasItem::draw_dashed_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
	real_t p_width, real_t p_dash, bool p_aligned, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	ERR_FAIL_COND(p_dash <= 0.0);

	float length = (p_to - p_from).length();
	Vector2 step = p_dash * (p_to - p_from).normalized();

	if (length < p_dash || step == Vector2()) {
		RenderingServer::get_singleton()->canvas_item_add_line(
			canvas_item, p_from, p_to, p_color, p_width, p_antialiased);
		return;
	}

	int steps = (p_aligned) ? Math::ceil(length / p_dash) : Math::floor(length / p_dash);
	if (steps % 2 == 0) {
		steps--;
	}

	Point2 off = p_from;
	if (p_aligned) {
		off += (p_to - p_from).normalized() * (length - steps * p_dash) / 2.0;
	}

	Vector<Vector2> points;
	points.resize(steps + 1);
	for (int i = 0; i < steps; i += 2) {
		points.write[i] = (i == 0) ? p_from : off;
		points.write[i + 1] = (p_aligned && i == steps - 1) ? p_to : (off + step);
		off += step * 2;
	}

	Vector<Color> colors = {p_color};

	RenderingServer::get_singleton()->canvas_item_add_multiline(
		canvas_item, points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RenderingServer::get_singleton()->canvas_item_add_line(
		canvas_item, p_from, p_to, p_color, p_width, p_antialiased);
}

void CanvasItem::draw_polyline(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Color> colors = {p_color};
	RenderingServer::get_singleton()->canvas_item_add_polyline(
		canvas_item, p_points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_polyline_colors(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RenderingServer::get_singleton()->canvas_item_add_polyline(
		canvas_item, p_points, p_colors, p_width, p_antialiased);
}

void CanvasItem::draw_ellipse_arc(const Vector2& p_center, real_t p_major, real_t p_minor,
	real_t p_start_angle, real_t p_end_angle, int p_point_count, const Color& p_color,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Point2> points;
	points.resize(p_point_count);
	Point2* points_ptr = points.ptrw();

	// Clamp angle difference to full circle so arc won't overlap itself.
	const real_t delta_angle = CLAMP(p_end_angle - p_start_angle, -Math::TAU, Math::TAU);
	for (int i = 0; i < p_point_count; i++) {
		real_t theta = (i / (p_point_count - 1.0f)) * delta_angle + p_start_angle;
		points_ptr[i] = p_center + Vector2(p_major * Math::cos(theta), p_minor * Math::sin(theta));
	}

	draw_polyline(points, p_color, p_width, p_antialiased);
}

void CanvasItem::draw_arc(const Vector2& p_center, real_t p_radius, real_t p_start_angle,
	real_t p_end_angle, int p_point_count, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	draw_ellipse_arc(p_center, p_radius, p_radius, p_start_angle, p_end_angle, p_point_count,
		p_color, p_width, p_antialiased);
}

void CanvasItem::draw_multiline(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Color> colors = {p_color};
	RenderingServer::get_singleton()->canvas_item_add_multiline(
		canvas_item, p_points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_multiline_colors(const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RenderingServer::get_singleton()->canvas_item_add_multiline(
		canvas_item, p_points, p_colors, p_width, p_antialiased);
}

void CanvasItem::draw_rect(
	const Rect2& p_rect, const Color& p_color, bool p_filled, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Rect2 rect = p_rect.abs();

	if (p_filled) {
		if (p_width != -1.0) {
			WARN_PRINT(
				"The draw_rect() \"width\" argument has no effect when \"filled\" is \"true\".");
		}

		RenderingServer::get_singleton()->canvas_item_add_rect(
			canvas_item, rect, p_color, p_antialiased);
	}
	else if (p_width >= rect.size.width || p_width >= rect.size.height) {
		RenderingServer::get_singleton()->canvas_item_add_rect(
			canvas_item, rect.grow(0.5f * p_width), p_color, p_antialiased);
	}
	else {
		Vector<Vector2> points;
		points.resize(5);
		points.write[0] = rect.position;
		points.write[1] = rect.position + Vector2(rect.size.x, 0);
		points.write[2] = rect.position + rect.size;
		points.write[3] = rect.position + Vector2(0, rect.size.y);
		points.write[4] = rect.position;

		Vector<Color> colors = {p_color};

		RenderingServer::get_singleton()->canvas_item_add_polyline(
			canvas_item, points, colors, p_width, p_antialiased);
	}
}

void CanvasItem::draw_ellipse(const Point2& p_pos, real_t p_major, real_t p_minor,
	const Color& p_color, bool p_filled, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	if (p_filled) {
		if (p_width != -1.0) {
			WARN_PRINT("The \"width\" argument has no effect when \"filled\" is \"true\".");
		}

		RenderingServer::get_singleton()->canvas_item_add_ellipse(
			canvas_item, p_pos, p_major, p_minor, p_color, p_antialiased);
	}
	else if (p_width >= 2.0 * MAX(p_major, p_minor)) {
		RenderingServer::get_singleton()->canvas_item_add_ellipse(canvas_item, p_pos,
			p_major + 0.5 * p_width, p_minor + 0.5 * p_width, p_color, p_antialiased);
	}
	else {
		// Tessellation count is hardcoded. Keep in sync with the same variable in
		// `RendererCanvasCull::canvas_item_add_circle()`.
		const int circle_segments = 64;

		Vector<Vector2> points;
		points.resize(circle_segments + 1);

		Vector2* points_ptr = points.ptrw();
		const real_t circle_point_step = Math::TAU / circle_segments;

		for (int i = 0; i < circle_segments; i++) {
			float angle = i * circle_point_step;
			points_ptr[i].x = Math::cos(angle) * p_major;
			points_ptr[i].y = Math::sin(angle) * p_minor;
			points_ptr[i] += p_pos;
		}
		points_ptr[circle_segments] = points_ptr[0];

		Vector<Color> colors = {p_color};

		RenderingServer::get_singleton()->canvas_item_add_polyline(
			canvas_item, points, colors, p_width, p_antialiased);
	}
}

void CanvasItem::draw_circle(const Point2& p_pos, real_t p_radius, const Color& p_color,
	bool p_filled, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	draw_ellipse(p_pos, p_radius, p_radius, p_color, p_filled, p_width, p_antialiased);
}

void CanvasItem::draw_texture(Texture2D* rp_texture, const Point2& p_pos, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_texture->draw(canvas_item, p_pos, p_modulate, false);
}

void CanvasItem::draw_texture_rect(Texture2D* rp_texture, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_texture->draw_rect(canvas_item, p_rect, p_tile, p_modulate, p_transpose);
}

void CanvasItem::draw_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_texture->draw_rect_region(
		canvas_item, p_rect, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

void CanvasItem::draw_msdf_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, double p_outline, double p_pixel_range,
	double p_scale)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	RenderingServer::get_singleton()->canvas_item_add_msdf_texture_rect_region(canvas_item, p_rect,
		rp_texture->get_rid(), p_src_rect, p_modulate, p_outline, p_pixel_range, p_scale);
}

void CanvasItem::draw_lcd_texture_rect_region(
	Texture2D* rp_texture, const Rect2& p_rect, const Rect2& p_src_rect, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	RenderingServer::get_singleton()->canvas_item_add_lcd_texture_rect_region(
		canvas_item, p_rect, rp_texture->get_rid(), p_src_rect, p_modulate);
}

void CanvasItem::draw_style_box(StyleBox* rp_style_box, const Rect2& p_rect)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_style_box->draw(canvas_item, p_rect);
}

void CanvasItem::draw_primitive(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RID rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
	RenderingServer::get_singleton()->canvas_item_add_primitive(
		canvas_item, p_points, p_colors, p_uvs, rid);
}

void CanvasItem::draw_set_transform(const Point2& p_offset, real_t p_rot, const Size2& p_scale)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Transform2D xform(p_rot, p_scale, 0.0, p_offset);
	RenderingServer::get_singleton()->canvas_item_add_set_transform(canvas_item, xform);
}

void CanvasItem::draw_set_transform_matrix(const Transform2D& p_matrix)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RenderingServer::get_singleton()->canvas_item_add_set_transform(canvas_item, p_matrix);
}

void CanvasItem::draw_animation_slice(
	double p_animation_length, double p_slice_begin, double p_slice_end, double p_offset)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RenderingServer::get_singleton()->canvas_item_add_animation_slice(
		canvas_item, p_animation_length, p_slice_begin, p_slice_end, p_offset);
}

void CanvasItem::draw_end_animation()
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RenderingServer::get_singleton()->canvas_item_add_animation_slice(canvas_item, 1, 0, 2, 0);
}

void CanvasItem::draw_polygon(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	const Ref<AtlasTexture> atlas = p_texture;
	if (atlas.is_valid() && atlas->get_atlas().is_valid()) {
		const Ref<Texture2D>& texture = atlas->get_atlas();
		const Vector2 atlas_size = texture->get_size();

		const Vector2 remap_min = atlas->get_region().position / atlas_size;
		const Vector2 remap_max = atlas->get_region().get_end() / atlas_size;

		PackedVector2Array uvs = p_uvs;
		for (Vector2& p : uvs) {
			p.x = Math::remap(p.x, 0, 1, remap_min.x, remap_max.x);
			p.y = Math::remap(p.y, 0, 1, remap_min.y, remap_max.y);
		}
		RenderingServer::get_singleton()->canvas_item_add_polygon(
			canvas_item, p_points, p_colors, uvs, texture->get_rid());
	}
	else {
		RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
		RenderingServer::get_singleton()->canvas_item_add_polygon(
			canvas_item, p_points, p_colors, p_uvs, texture_rid);
	}
}

void CanvasItem::draw_colored_polygon(const Vector<Point2>& p_points, const Color& p_color,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	draw_polygon(p_points, {p_color}, p_uvs, p_texture);
}

void CanvasItem::draw_mesh(Mesh* rp_mesh, const Ref<Texture2D>& p_texture,
	const Transform2D& p_transform, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();

	RenderingServer::get_singleton()->canvas_item_add_mesh(
		canvas_item, rp_mesh->get_rid(), p_transform, p_modulate, texture_rid);
}

void CanvasItem::draw_multimesh(MultiMesh* rp_multimesh, const Ref<Texture2D>& p_texture)
{
	ERR_THREAD_GUARD;
	RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
	RenderingServer::get_singleton()->canvas_item_add_multimesh(
		canvas_item, rp_multimesh->get_rid(), texture_rid);
}

void CanvasItem::draw_string(Font* rp_font, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, const Color& p_modulate,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_font->draw_string(canvas_item, p_pos, p_text, p_alignment, p_width, p_font_size, p_modulate,
		p_jst_flags, p_direction, p_orientation, p_oversampling);
}

void CanvasItem::draw_multiline_string(Font* rp_font, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_max_lines,
	const Color& p_modulate, uint32_t p_brk_flags, uint32_t p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_font->draw_multiline_string(canvas_item, p_pos, p_text, p_alignment, p_width, p_font_size,
		p_max_lines, p_modulate, p_brk_flags, p_jst_flags, p_direction, p_orientation,
		p_oversampling);
}

void CanvasItem::draw_string_outline(Font* rp_font, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_size,
	const Color& p_modulate, uint32_t p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation, float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_font->draw_string_outline(canvas_item, p_pos, p_text, p_alignment, p_width, p_font_size,
		p_size, p_modulate, p_jst_flags, p_direction, p_orientation, p_oversampling);
}

void CanvasItem::draw_multiline_string_outline(Font* rp_font, const Point2& p_pos,
	const String& p_text, HorizontalAlignment p_alignment, float p_width, int p_font_size,
	int p_max_lines, int p_size, const Color& p_modulate, uint32_t p_brk_flags,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_font->draw_multiline_string_outline(canvas_item, p_pos, p_text, p_alignment, p_width,
		p_font_size, p_max_lines, p_size, p_modulate, p_brk_flags, p_jst_flags, p_direction,
		p_orientation, p_oversampling);
}

void CanvasItem::draw_char(Font* rp_font, const Point2& p_pos, const String& p_char,
	int p_font_size, const Color& p_modulate, float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	ERR_FAIL_COND(p_char.length() != 1);

	rp_font->draw_char(canvas_item, p_pos, p_char[0], p_font_size, p_modulate, p_oversampling);
}

void CanvasItem::draw_char_outline(Font* rp_font, const Point2& p_pos, const String& p_char,
	int p_font_size, int p_size, const Color& p_modulate, float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	ERR_FAIL_COND(p_char.length() != 1);

	rp_font->draw_char_outline(
		canvas_item, p_pos, p_char[0], p_font_size, p_size, p_modulate, p_oversampling);
}

void CanvasItem::_notify_transform_deferred()
{
	if (is_inside_tree() && notify_transform && !xform_change.in_list()) {
		get_tree()->xform_change_list.add(&xform_change);
	}
}

void CanvasItem::_physics_interpolated_changed()
{
	RenderingServer::get_singleton()->canvas_item_set_interpolated(
		canvas_item, is_physics_interpolated());
}

void CanvasItem::set_canvas_item_use_identity_transform(bool p_enable)
{
	// Prevent sending item transforms to RenderingServer when using global coords.
	_set_use_identity_transform(p_enable);

	// Let RenderingServer know not to concatenate the parent transform during the render.
	RenderingServer::get_singleton()->canvas_item_set_use_identity_transform(
		get_canvas_item(), p_enable);

	if (is_inside_tree()) {
		if (p_enable) {
			// Make sure item is using identity transform in server.
			RenderingServer::get_singleton()->canvas_item_set_transform(
				get_canvas_item(), Transform2D());
		}
		else {
			// Make sure item transform is up to date in server if switching identity transform off.
			RenderingServer::get_singleton()->canvas_item_set_transform(
				get_canvas_item(), get_transform());
		}
	}
}

Rect2 CanvasItem::get_viewport_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	return get_viewport()->get_visible_rect();
}

RID CanvasItem::get_canvas() const
{
	ERR_READ_THREAD_GUARD_V(RID());
	ERR_FAIL_COND_V(!is_inside_tree(), RID());

	if (canvas_layer) {
		return canvas_layer->get_canvas();
	}
	else {
		return get_viewport()->find_world_2d()->get_canvas();
	}
}

Ref<World2D> CanvasItem::get_world_2d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World2D>());
	ERR_FAIL_COND_V(!is_inside_tree(), Ref<World2D>());

	CanvasItem* tl = get_top_level();

	if (tl->get_viewport()) {
		return tl->get_viewport()->find_world_2d();
	}
	else {
		return Ref<World2D>();
	}
}

RID CanvasItem::get_viewport_rid() const
{
	ERR_READ_THREAD_GUARD_V(RID());
	ERR_FAIL_COND_V(!is_inside_tree(), RID());
	return get_viewport()->get_viewport_rid();
}

void CanvasItem::set_block_transform_notify(bool p_enable)
{
	ERR_THREAD_GUARD;
	block_transform_notify = p_enable;
}

bool CanvasItem::is_block_transform_notify_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return block_transform_notify;
}

void CanvasItem::set_draw_behind_parent(bool p_enable)
{
	ERR_THREAD_GUARD;
	if (behind == p_enable) {
		return;
	}
	behind = p_enable;
	RenderingServer::get_singleton()->canvas_item_set_draw_behind_parent(canvas_item, behind);
}

bool CanvasItem::is_draw_behind_parent_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return behind;
}

void CanvasItem::set_use_parent_material(bool p_use_parent_material)
{
	ERR_THREAD_GUARD;
	use_parent_material = p_use_parent_material;
	RS::get_singleton()->canvas_item_set_use_parent_material(canvas_item, p_use_parent_material);
}

bool CanvasItem::get_use_parent_material() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_parent_material;
}

Ref<Material> CanvasItem::get_material() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Material>());
	return material;
}

Vector2 CanvasItem::make_canvas_position_local(const Vector2& screen_point) const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	ERR_FAIL_COND_V(!is_inside_tree(), screen_point);

	Transform2D local_matrix = (get_canvas_transform() * get_global_transform()).affine_inverse();

	return local_matrix.xform(screen_point);
}

InputEvent* CanvasItem::make_input_local(InputEvent* rp_event) const
{
	ERR_FAIL_COND_V(!is_inside_tree(), rp_event);
	return rp_event->xformed_by((get_canvas_transform() * get_global_transform()).affine_inverse());
}

Vector2 CanvasItem::get_global_mouse_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	ERR_FAIL_NULL_V(get_viewport(), Vector2());
	return get_canvas_transform().affine_inverse().xform(get_viewport()->get_mouse_position());
}

Vector2 CanvasItem::get_local_mouse_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	ERR_FAIL_NULL_V(get_viewport(), Vector2());

	return get_global_transform().affine_inverse().xform(get_global_mouse_position());
}

Transform2D CanvasItem::get_viewport_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	ERR_FAIL_COND_V(!is_inside_tree(), Transform2D());

	if (canvas_layer) {
		return get_viewport()->get_final_transform() * canvas_layer->get_final_transform();
	}
	else {
		return get_viewport()->get_final_transform() * get_viewport()->get_canvas_transform();
	}
}

void CanvasItem::set_notify_local_transform(bool p_enable)
{
	ERR_THREAD_GUARD;
	notify_local_transform = p_enable;
}

bool CanvasItem::is_local_transform_notification_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return notify_local_transform;
}

void CanvasItem::set_notify_transform(bool p_enable)
{
	ERR_THREAD_GUARD;
	if (notify_transform == p_enable) {
		return;
	}

	notify_transform = p_enable;

	if (notify_transform && is_inside_tree()) {
		// This ensures that invalid globals get resolved, so notifications can be received.
		_ALLOW_DISCARD_ get_global_transform();
	}
}

bool CanvasItem::is_transform_notification_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return notify_transform;
}

int CanvasItem::get_canvas_layer() const
{
	ERR_READ_THREAD_GUARD_V(0);
	if (canvas_layer) {
		return canvas_layer->get_layer();
	}
	else {
		return 0;
	}
}

CanvasLayer* CanvasItem::get_canvas_layer_node() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return canvas_layer;
}

void CanvasItem::set_visibility_layer(uint32_t p_visibility_layer)
{
	ERR_THREAD_GUARD;
	visibility_layer = p_visibility_layer;
	RenderingServer::get_singleton()->canvas_item_set_visibility_layer(
		canvas_item, p_visibility_layer);
}

uint32_t CanvasItem::get_visibility_layer() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return visibility_layer;
}

void CanvasItem::set_visibility_layer_bit(uint32_t p_visibility_layer, bool p_enable)
{
	ERR_THREAD_GUARD;
	ERR_FAIL_UNSIGNED_INDEX(p_visibility_layer, 32);
	if (p_enable) {
		set_visibility_layer(visibility_layer | (1 << p_visibility_layer));
	}
	else {
		set_visibility_layer(visibility_layer & (~(1 << p_visibility_layer)));
	}
}

bool CanvasItem::get_visibility_layer_bit(uint32_t p_visibility_layer) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_visibility_layer, 32, false);
	return (visibility_layer & (1 << p_visibility_layer));
}

void CanvasItem::_refresh_texture_filter_cache() const
{
	if (!is_inside_tree()) {
		return;
	}

	if (texture_filter == TEXTURE_FILTER_PARENT_NODE) {
		CanvasItem* parent_item = get_parent_item();
		if (parent_item) {
			texture_filter_cache = parent_item->texture_filter_cache;
		}
		else {
			texture_filter_cache = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
		}
	}
	else {
		texture_filter_cache = RSE::CanvasItemTextureFilter(texture_filter);
	}
}

void CanvasItem::_update_self_texture_filter(RSE::CanvasItemTextureFilter p_texture_filter)
{
	RS::get_singleton()->canvas_item_set_default_texture_filter(
		get_canvas_item(), p_texture_filter);
	queue_redraw();
}

CanvasItem::TextureFilter CanvasItem::get_texture_filter() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_FILTER_NEAREST);
	return texture_filter;
}

void CanvasItem::_refresh_texture_repeat_cache() const
{
	if (!is_inside_tree()) {
		return;
	}

	if (texture_repeat == TEXTURE_REPEAT_PARENT_NODE) {
		CanvasItem* parent_item = get_parent_item();
		if (parent_item) {
			texture_repeat_cache = parent_item->texture_repeat_cache;
		}
		else {
			texture_repeat_cache = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
		}
	}
	else {
		texture_repeat_cache = RSE::CanvasItemTextureRepeat(texture_repeat);
	}
}

void CanvasItem::_update_self_texture_repeat(RSE::CanvasItemTextureRepeat p_texture_repeat)
{
	RS::get_singleton()->canvas_item_set_default_texture_repeat(
		get_canvas_item(), p_texture_repeat);
	queue_redraw();
}

CanvasItem::ClipChildrenMode CanvasItem::get_clip_children_mode() const
{
	ERR_READ_THREAD_GUARD_V(CLIP_CHILDREN_DISABLED);
	return clip_children_mode;
}

CanvasItem::TextureRepeat CanvasItem::get_texture_repeat() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_REPEAT_DISABLED);
	return texture_repeat;
}

CanvasItem::TextureFilter CanvasItem::get_texture_filter_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_FILTER_NEAREST);
	_refresh_texture_filter_cache();
	return (TextureFilter)texture_filter_cache;
}

CanvasItem::TextureRepeat CanvasItem::get_texture_repeat_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_REPEAT_DISABLED);
	_refresh_texture_repeat_cache();
	return (TextureRepeat)texture_repeat_cache;
}

CanvasItem::~CanvasItem()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RenderingServer::get_singleton()->free_rid(canvas_item);
}

Ref<Texture2D> CanvasTexture::get_diffuse_texture() const { return diffuse_texture; }

Ref<Texture2D> CanvasTexture::get_normal_texture() const { return normal_texture; }

Ref<Texture2D> CanvasTexture::get_specular_texture() const { return specular_texture; }

void CanvasTexture::set_specular_color(const Color& p_color)
{
	if (specular == p_color) {
		return;
	}
	specular = p_color;
	RS::get_singleton()->canvas_texture_set_shading_parameters(canvas_texture, specular, shininess);
	emit_changed();
}

Color CanvasTexture::get_specular_color() const { return specular; }

void CanvasTexture::set_specular_shininess(real_t p_shininess)
{
	if (shininess == p_shininess) {
		return;
	}
	shininess = p_shininess;
	RS::get_singleton()->canvas_texture_set_shading_parameters(canvas_texture, specular, shininess);
	emit_changed();
}

real_t CanvasTexture::get_specular_shininess() const { return shininess; }

void CanvasTexture::set_texture_filter(CanvasItem::TextureFilter p_filter)
{
	if (texture_filter == p_filter) {
		return;
	}
	texture_filter = p_filter;
	RS::get_singleton()->canvas_texture_set_texture_filter(
		canvas_texture, RSE::CanvasItemTextureFilter(p_filter));
	emit_changed();
}

CanvasItem::TextureFilter CanvasTexture::get_texture_filter() const { return texture_filter; }

void CanvasTexture::set_texture_repeat(CanvasItem::TextureRepeat p_repeat)
{
	if (texture_repeat == p_repeat) {
		return;
	}
	texture_repeat = p_repeat;
	RS::get_singleton()->canvas_texture_set_texture_repeat(
		canvas_texture, RSE::CanvasItemTextureRepeat(p_repeat));
	emit_changed();
}

CanvasItem::TextureRepeat CanvasTexture::get_texture_repeat() const { return texture_repeat; }

int CanvasTexture::get_width() const
{
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->get_width();
	}
	else {
		return 1;
	}
}

int CanvasTexture::get_height() const
{
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->get_height();
	}
	else {
		return 1;
	}
}

bool CanvasTexture::is_pixel_opaque(int p_x, int p_y) const
{
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->is_pixel_opaque(p_x, p_y);
	}
	else {
		return false;
	}
}

bool CanvasTexture::has_alpha() const
{
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->has_alpha();
	}
	else {
		return false;
	}
}

Ref<Image> CanvasTexture::get_image() const
{
	if (diffuse_texture.is_valid()) {
		return diffuse_texture->get_image();
	}
	else {
		return Ref<Image>();
	}
}

RID CanvasTexture::get_rid() const { return canvas_texture; }

CanvasTexture::CanvasTexture() { canvas_texture = RS::get_singleton()->canvas_texture_create(); }

CanvasTexture::~CanvasTexture()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(canvas_texture);
}


