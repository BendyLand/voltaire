/**************************************************************************/
/*  canvas_layer.cpp                                                      */
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

#include "canvas_layer.h"
#include "scene/main/canvas_item.h"
#include "scene/main/viewport.h"
#include "scene/resources/world_2d.h"
#include "servers/rendering/rendering_server.h"

void CanvasLayer::set_layer(int p_xform)
{
	layer = p_xform;
	if (viewport.is_valid()) {
		RenderingServer::get_singleton()->viewport_set_canvas_stacking(
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
		RenderingServer::get_singleton()->viewport_set_canvas_transform(
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
		RenderingServer::get_singleton()->viewport_set_canvas_transform(
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
		RenderingServer::get_singleton()->viewport_set_canvas_stacking(
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
		RS::get_singleton()->canvas_set_parent(canvas, RID(), 1.0);
	}
	else {
		RS::get_singleton()->canvas_set_parent(
			canvas, vp->get_world_2d()->get_canvas(), follow_viewport_scale);
	}
}


CanvasLayer::CanvasLayer() { canvas = RS::get_singleton()->canvas_create(); }

CanvasLayer::~CanvasLayer()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(canvas);
}


