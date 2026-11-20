/**************************************************************************/
/*  canvas_layer.cpp                                                      */
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
