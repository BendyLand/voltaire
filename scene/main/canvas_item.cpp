/**************************************************************************/
/*  canvas_item.cpp                                                       */
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

#include "canvas_item.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "scene/2d/canvas_group.h"
#include "scene/main/canvas_layer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/multimesh.h"
#include "scene/resources/style_box.h"
#include "scene/resources/world_2d.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"

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

void CanvasItem::set_visible(bool p_visible) { visible = p_visible; }

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

CanvasItem* CanvasItem::get_parent_item() const
{
	CanvasItem ci = CanvasItem();
	return &ci;
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
		Renderer::canvas_item_set_draw_index(canvas_item, canvas_layer->get_sort_index());
	}
	else {
		Renderer::canvas_item_set_draw_index(
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
	Renderer::canvas_item_set_modulate(canvas_item, modulate);
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

bool CanvasItem::is_set_as_top_level() const { return top_level; }

void CanvasItem::set_self_modulate(const Color& p_self_modulate)
{
	ERR_THREAD_GUARD;
	if (self_modulate == p_self_modulate) {
		return;
	}

	self_modulate = p_self_modulate;
	Renderer::canvas_item_set_self_modulate(canvas_item, self_modulate);
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
	Renderer::canvas_item_set_light_mask(canvas_item, p_light_mask);
}

int CanvasItem::get_light_mask() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return light_mask;
}

void CanvasItem::set_z_as_relative(bool p_enabled)
{
	ERR_THREAD_GUARD;
	if (z_relative == p_enabled) {
		return;
	}
	z_relative = p_enabled;
	Renderer::canvas_item_set_z_as_relative_to_parent(canvas_item, p_enabled);
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
	Renderer::canvas_item_set_sort_children_by_y(canvas_item, y_sort_enabled);
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
		Renderer::canvas_item_add_line(canvas_item, p_from, p_to, p_color, p_width, p_antialiased);
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

	Renderer::canvas_item_add_multiline(canvas_item, points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_line(canvas_item, p_from, p_to, p_color, p_width, p_antialiased);
}

void CanvasItem::draw_polyline(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Color> colors = {p_color};
	Renderer::canvas_item_add_polyline(canvas_item, p_points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_polyline_colors(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_polyline(canvas_item, p_points, p_colors, p_width, p_antialiased);
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
	Renderer::canvas_item_add_multiline(canvas_item, p_points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_multiline_colors(const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_multiline(canvas_item, p_points, p_colors, p_width, p_antialiased);
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

		Renderer::canvas_item_add_rect(canvas_item, rect, p_color, p_antialiased);
	}
	else if (p_width >= rect.size.width || p_width >= rect.size.height) {
		Renderer::canvas_item_add_rect(
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

		Renderer::canvas_item_add_polyline(canvas_item, points, colors, p_width, p_antialiased);
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

		Renderer::canvas_item_add_ellipse(
			canvas_item, p_pos, p_major, p_minor, p_color, p_antialiased);
	}
	else if (p_width >= 2.0 * MAX(p_major, p_minor)) {
		Renderer::canvas_item_add_ellipse(canvas_item, p_pos, p_major + 0.5 * p_width,
			p_minor + 0.5 * p_width, p_color, p_antialiased);
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

		Renderer::canvas_item_add_polyline(canvas_item, points, colors, p_width, p_antialiased);
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
	Renderer::canvas_item_add_msdf_texture_rect_region(canvas_item, p_rect, rp_texture->get_rid(),
		p_src_rect, p_modulate, p_outline, p_pixel_range, p_scale);
}

void CanvasItem::draw_lcd_texture_rect_region(
	Texture2D* rp_texture, const Rect2& p_rect, const Rect2& p_src_rect, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	Renderer::canvas_item_add_lcd_texture_rect_region(
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
	Renderer::canvas_item_add_primitive(canvas_item, p_points, p_colors, p_uvs, rid);
}

void CanvasItem::draw_set_transform(const Point2& p_offset, real_t p_rot, const Size2& p_scale)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Transform2D xform(p_rot, p_scale, 0.0, p_offset);
	Renderer::canvas_item_add_set_transform(canvas_item, xform);
}

void CanvasItem::draw_set_transform_matrix(const Transform2D& p_matrix)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_set_transform(canvas_item, p_matrix);
}

void CanvasItem::draw_animation_slice(
	double p_animation_length, double p_slice_begin, double p_slice_end, double p_offset)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_animation_slice(
		canvas_item, p_animation_length, p_slice_begin, p_slice_end, p_offset);
}

void CanvasItem::draw_end_animation()
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_animation_slice(canvas_item, 1, 0, 2, 0);
}

void CanvasItem::draw_polygon(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	const Ref<AtlasTexture> atlas = p_texture;
	if (atlas.is_valid() && !atlas->is_animated() && atlas->get_atlas().is_valid()) {
		const Ref<Texture2D>& texture = atlas->get_atlas();
		const Vector2 atlas_size = texture->get_size();

		const Vector2 remap_min = atlas->get_region().position / atlas_size;
		const Vector2 remap_max = atlas->get_region().get_end() / atlas_size;

		PackedVector2Array uvs = p_uvs;
		for (Vector2& p : uvs) {
			p.x = Math::remap(p.x, 0, 1, remap_min.x, remap_max.x);
			p.y = Math::remap(p.y, 0, 1, remap_min.y, remap_max.y);
		}
		Renderer::canvas_item_add_polygon(canvas_item, p_points, p_colors, uvs, texture->get_rid());
	}
	else {
		RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
		Renderer::canvas_item_add_polygon(canvas_item, p_points, p_colors, p_uvs, texture_rid);
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

	Renderer::canvas_item_add_mesh(
		canvas_item, rp_mesh->get_rid(), p_transform, p_modulate, texture_rid);
}

void CanvasItem::draw_multimesh(MultiMesh* rp_multimesh, const Ref<Texture2D>& p_texture)
{
	ERR_THREAD_GUARD;
	RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
	Renderer::canvas_item_add_multimesh(canvas_item, rp_multimesh->get_rid(), texture_rid);
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
	if (is_inside_tree() && notify_transform && !xform_change.in_list() && SceneTree::data) {
		SceneTree::data->xform_change_list.add(&xform_change);
	}
}

void CanvasItem::_physics_interpolated_changed()
{
	Renderer::canvas_item_set_interpolated(canvas_item, is_physics_interpolated());
}

void CanvasItem::set_canvas_item_use_identity_transform(bool p_enable)
{
	// Prevent sending item transforms to Renderer when using global coords.
	_set_use_identity_transform(p_enable);

	// Let Renderer know not to concatenate the parent transform during the render.
	Renderer::canvas_item_set_use_identity_transform(get_canvas_item(), p_enable);

	if (is_inside_tree()) {
		if (p_enable) {
			// Make sure item is using identity transform in server.
			Renderer::canvas_item_set_transform(get_canvas_item(), Transform2D());
		}
		else {
			// Make sure item transform is up to date in server if switching identity transform off.
			Renderer::canvas_item_set_transform(get_canvas_item(), get_transform());
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
	Renderer::canvas_item_set_draw_behind_parent(canvas_item, behind);
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
	Renderer::canvas_item_set_use_parent_material(canvas_item, p_use_parent_material);
}

bool CanvasItem::get_use_parent_material() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_parent_material;
}

void CanvasItem::set_material(const Ref<Material>& p_material) {}

void CanvasItem::_update_self_texture_repeat(RSE::CanvasItemTextureRepeat p_texture_repeat) {}

void CanvasItem::_update_self_texture_filter(RSE::CanvasItemTextureFilter p_texture_filter) {}

void CanvasItem::set_z_index(int p_z) {}

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

InputEvent CanvasItem::make_input_local(const InputEvent& p_event) const
{
	ERR_FAIL_COND_V(!is_inside_tree(), p_event);
	return p_event.xformed_by((get_canvas_transform() * get_global_transform()).affine_inverse());
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
	Renderer::canvas_item_set_visibility_layer(canvas_item, p_visibility_layer);
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

PackedStringArray CanvasItem::get_configuration_warnings() const { return PackedStringArray(); }

CanvasItem::CanvasItem() { canvas_item = Renderer::canvas_item_create(); }

CanvasItem::~CanvasItem()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(canvas_item);
}

void CanvasItem::queue_redraw()
{
    if (!is_inside_tree() || !canvas_item.is_valid()) {
        return;
    }
    Renderer::canvas_item_clear(canvas_item);
    if (is_visible_in_tree()) {
        drawing = true;
        notification(NOTIFICATION_DRAW);
        if (draw_callback) {
            draw_callback(this);
        }
        drawing = false;
    }
}

void CanvasItem::_top_level_changed() {}

void CanvasItem::_top_level_changed_on_parent() {}

void CanvasItem::item_rect_changed(bool p_size_changed) {}

Transform2D CanvasItem::get_canvas_transform() const { return Transform2D(); }

void CanvasItem::set_texture_filter(CanvasItem::TextureFilter) {}

void CanvasItem::_handle_visibility_change(bool) {}

void CanvasItem::_update_oversampling(bool) {}

void CanvasItem::set_texture_repeat(CanvasItem::TextureRepeat) {}

void CanvasItem::set_as_top_level(bool) {}

CanvasItem* CanvasItem::get_top_level() const
{
	CanvasItem ci = CanvasItem();
	return &ci;
}

void CanvasItem::_edit_set_scale(const Size2& p_scale) {}

Size2 CanvasItem::_edit_get_scale() const { return Size2(); }

Point2 CanvasItem::_edit_get_position() const { return Point2(); }

void CanvasItem::_edit_set_position(const Point2& p_position) {}

Transform2D CanvasItem::get_transform() const { return Transform2D(); }

void CanvasItem::_notification(int p_what) {}

// --- canvas_item.compat.inc ---

#ifndef DISABLE_DEPRECATED

#include "canvas_item.h"
#include "scene/resources/font.h"

void CanvasItem::_draw_char_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_char, int p_font_size, const Color& p_modulate) const
{
	draw_char(p_font.ptr(), p_pos, p_char, p_font_size, p_modulate, 0.0);
}

void CanvasItem::_draw_char_outline_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_char, int p_font_size, int p_size, const Color& p_modulate) const
{
	draw_char_outline(p_font.ptr(), p_pos, p_char, p_font_size, p_size, p_modulate, 0.0);
}

void CanvasItem::_draw_circle_bind_compat_84472(
	const Point2& p_pos, real_t p_radius, const Color& p_color)
{
	draw_circle(p_pos, p_radius, p_color, true, -1.0, false);
}

void CanvasItem::_draw_rect_bind_compat_84523(
	const Rect2& p_rect, const Color& p_color, bool p_filled, real_t p_width)
{
	draw_rect(p_rect, p_color, p_filled, p_width, false);
}

void CanvasItem::_draw_dashed_line_bind_compat_84523(const Point2& p_from, const Point2& p_to,
	const Color& p_color, real_t p_width, real_t p_dash, bool p_aligned)
{
	draw_dashed_line(p_from, p_to, p_color, p_width, p_dash, p_aligned, false);
}

void CanvasItem::_draw_multiline_bind_compat_84523(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width)
{
	draw_multiline(p_points, p_color, p_width, false);
}

void CanvasItem::_draw_multiline_colors_bind_compat_84523(
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, real_t p_width)
{
	draw_multiline_colors(p_points, p_colors, p_width, false);
}

void CanvasItem::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

#undef ERR_DRAW_GUARD

// ==========================================================================
// Merged from scene/main/canvas_layer.cpp
// ==========================================================================

#include "canvas_layer.h"
#include "scene/2d/parallax_layer.h"
#include "scene/main/canvas_item.h"
#include "scene/main/viewport.h"
#include "scene/resources/world_2d.h"
#include "servers/rendering/renderer.h"

void CanvasLayer::_notification(int p_what)
{
	if (!parallax_enabled) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		parallax.group_name = "__cameras_" + itos(get_viewport().get_id());
		add_to_group(parallax.group_name);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		remove_from_group(parallax.group_name);
	} break;
	}
}

void CanvasLayer::set_layer(int p_xform)
{
	layer = p_xform;
	if (viewport.is_valid()) {
		Renderer::viewport_set_canvas_stacking(
			viewport, canvas, layer, get_index());
		vp->gui_set_root_order_dirty();
	}
}

int CanvasLayer::get_layer() const { return layer; }

void CanvasLayer::show() { set_visible(true); }

void CanvasLayer::hide() { set_visible(false); }

bool CanvasLayer::is_visible() const { return visible; }

void CanvasLayer::set_transform(const Transform2D& p_xform)
{
	transform = p_xform;
	locrotscale_dirty = true;
	if (viewport.is_valid()) {
		Renderer::viewport_set_canvas_transform(
			viewport, canvas, transform);
	}
}

Transform2D CanvasLayer::get_transform() const { return transform; }

Transform2D CanvasLayer::get_final_transform() const
{
	if (is_following_viewport()) {
		Transform2D follow;
		follow.scale(Vector2(get_follow_viewport_scale(), get_follow_viewport_scale()));
		if (vp) {
			follow = vp->get_canvas_transform() * follow;
		}
		return follow * transform;
	}
	return transform;
}

void CanvasLayer::_update_xform()
{
	transform.set_rotation_and_scale(rot, scale);
	transform.set_origin(ofs);
	if (viewport.is_valid()) {
		Renderer::viewport_set_canvas_transform(
			viewport, canvas, transform);
	}
}

void CanvasLayer::_update_locrotscale() const
{
	ofs = transform.columns[2];
	rot = transform.get_rotation();
	scale = transform.get_scale();
	locrotscale_dirty = false;
}

void CanvasLayer::set_offset(const Vector2& p_offset)
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	ofs = p_offset;
	_update_xform();
}

Vector2 CanvasLayer::get_offset() const
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	return ofs;
}

void CanvasLayer::set_rotation(real_t p_radians)
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	rot = p_radians;
	_update_xform();
}

real_t CanvasLayer::get_rotation() const
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	return rot;
}

void CanvasLayer::set_scale(const Vector2& p_scale)
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	scale = p_scale;
	_update_xform();
}

Vector2 CanvasLayer::get_scale() const
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	return scale;
}

void CanvasLayer::update_draw_order()
{
	if (is_inside_tree()) {
		Renderer::viewport_set_canvas_stacking(
			viewport, canvas, layer, get_index());
	}
}

Size2 CanvasLayer::get_viewport_size() const
{
	if (!is_inside_tree()) {
		return Size2(1, 1);
	}

	ERR_FAIL_NULL_V_MSG(vp, Size2(1, 1), "Viewport is not initialized.");

	Rect2 r = vp->get_visible_rect();
	return r.size;
}

RID CanvasLayer::get_viewport() const { return viewport; }

Node* CanvasLayer::get_custom_viewport() const { return custom_viewport; }

void CanvasLayer::reset_sort_index() { sort_index = 0; }

int CanvasLayer::get_sort_index() { return sort_index++; }

RID CanvasLayer::get_canvas() const { return canvas; }

void CanvasLayer::set_follow_viewport(bool p_enable)
{
	if (follow_viewport == p_enable) {
		return;
	}

	follow_viewport = p_enable;
	_update_follow_viewport();
}

bool CanvasLayer::is_following_viewport() const { return follow_viewport; }

void CanvasLayer::set_follow_viewport_scale(float p_ratio)
{
	follow_viewport_scale = p_ratio;
	_update_follow_viewport();
}

float CanvasLayer::get_follow_viewport_scale() const { return follow_viewport_scale; }

void CanvasLayer::_update_follow_viewport(bool p_force_exit)
{
	if (!is_inside_tree()) {
		return;
	}
	if (p_force_exit || !follow_viewport) {
		Renderer::canvas_set_parent(canvas, RID(), 1.0);
	}
	else {
		Renderer::canvas_set_parent(
			canvas, vp->get_world_2d()->get_canvas(), follow_viewport_scale);
	}
}

void CanvasLayer::set_parallax_enabled(bool p_enabled)
{
	if (parallax_enabled == p_enabled) {
		return;
	}

	if (parallax_enabled && is_inside_tree() && !parallax.group_name.is_empty()) {
		remove_from_group(parallax.group_name);
	}

	parallax_enabled = p_enabled;

	if (parallax_enabled && is_inside_tree()) {
		parallax.group_name = "__cameras_" + itos(get_viewport().get_id());
		add_to_group(parallax.group_name);
		_update_scroll();
	}
}

void CanvasLayer::_camera_moved(const Transform2D& p_transform, const Point2& p_screen_offset,
	const Point2& p_adj_screen_offset)
{
	if (!parallax_enabled) {
		return;
	}

	parallax.screen_offset = p_screen_offset;

	set_scroll_scale(p_transform.get_scale().dot(Vector2(0.5, 0.5)));
	set_scroll_offset(p_transform.get_origin());
}

void CanvasLayer::set_scroll_scale(real_t p_scale) { parallax.scale = p_scale; }

real_t CanvasLayer::get_scroll_scale() const { return parallax.scale; }

void CanvasLayer::set_scroll_offset(const Point2& p_ofs)
{
	parallax.offset = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_scroll_offset() const { return parallax.offset; }

void CanvasLayer::set_scroll_base_offset(const Point2& p_ofs)
{
	parallax.base_offset = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_scroll_base_offset() const { return parallax.base_offset; }

void CanvasLayer::set_scroll_base_scale(const Point2& p_ofs)
{
	parallax.base_scale = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_scroll_base_scale() const { return parallax.base_scale; }

void CanvasLayer::set_limit_begin(const Point2& p_ofs)
{
	parallax.limit_begin = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_limit_begin() const { return parallax.limit_begin; }

void CanvasLayer::set_limit_end(const Point2& p_ofs)
{
	parallax.limit_end = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_limit_end() const { return parallax.limit_end; }

void CanvasLayer::set_ignore_camera_zoom(bool p_ignore) { parallax.ignore_camera_zoom = p_ignore; }

bool CanvasLayer::is_ignore_camera_zoom() const { return parallax.ignore_camera_zoom; }

Vector2 CanvasLayer::get_final_offset() const { return parallax.final_offset; }

void CanvasLayer::_update_scroll()
{
	if (!parallax_enabled || !is_inside_tree()) {
		return;
	}

	Vector2 scroll_ofs = parallax.base_offset + parallax.offset * parallax.base_scale;

	Size2 vps = get_viewport_size();
	scroll_ofs = -scroll_ofs;
	if (parallax.limit_begin.x < parallax.limit_end.x) {
		if (scroll_ofs.x < parallax.limit_begin.x) {
			scroll_ofs.x = parallax.limit_begin.x;
		}
		else if (scroll_ofs.x + vps.x > parallax.limit_end.x) {
			scroll_ofs.x = parallax.limit_end.x - vps.x;
		}
	}

	if (parallax.limit_begin.y < parallax.limit_end.y) {
		if (scroll_ofs.y < parallax.limit_begin.y) {
			scroll_ofs.y = parallax.limit_begin.y;
		}
		else if (scroll_ofs.y + vps.y > parallax.limit_end.y) {
			scroll_ofs.y = parallax.limit_end.y - vps.y;
		}
	}
	scroll_ofs = -scroll_ofs;

	parallax.final_offset = scroll_ofs;

	real_t scroll_scale = parallax.scale;
	for (int i = 0; i < get_child_count(); i++) {
		ParallaxLayer* l = dynamic_cast<ParallaxLayer*>(get_child(i));
		if (!l) {
			continue;
		}

		if (parallax.ignore_camera_zoom) {
			l->set_base_offset_and_scale(
				(scroll_ofs + parallax.screen_offset * (scroll_scale - 1.0)) / scroll_scale, 1.0);
		}
		else {
			l->set_base_offset_and_scale(scroll_ofs, scroll_scale);
		}
	}
}

CanvasLayer::CanvasLayer(bool p_parallax)
{
	canvas = Renderer::canvas_create();
	if (p_parallax) {
		parallax_enabled = true;
		set_layer(-100);
	}
}

CanvasLayer::~CanvasLayer()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(canvas);
}

void CanvasLayer::set_visible(bool) {}

void CanvasLayer::set_custom_viewport(Node*) {}
