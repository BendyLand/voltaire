/**************************************************************************/
/*  root_motion_view.cpp                                                  */
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

#ifndef _3D_DISABLED

#include "core/config/engine.h"
#include "root_motion_view.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/material.h"

void RootMotionView::set_animation_mixer(const NodePath& p_path)
{
	path = p_path;
	first = true;
}

NodePath RootMotionView::get_animation_mixer() const { return path; }

void RootMotionView::set_color(const Color& p_color)
{
	color = p_color;
	first = true;
}

Color RootMotionView::get_color() const { return color; }

void RootMotionView::set_cell_size(float p_size)
{
	cell_size = p_size;
	first = true;
}

float RootMotionView::get_cell_size() const { return cell_size; }

void RootMotionView::set_radius(float p_radius)
{
	radius = p_radius;
	first = true;
}

float RootMotionView::get_radius() const { return radius; }

void RootMotionView::set_zero_y(bool p_zero_y) { zero_y = p_zero_y; }

bool RootMotionView::get_zero_y() const { return zero_y; }

AABB RootMotionView::get_aabb() const
{
	return AABB(Vector3(-radius, 0, -radius), Vector3(radius * 2, 0.001, radius * 2));
}

void RootMotionView::_bind_methods() {}

RootMotionView::RootMotionView()
{
	if (Engine::get_singleton()->is_editor_hint()) {
		set_process_internal(true);
	}
	immediate.instantiate();
	set_base(immediate->get_rid());
}

RootMotionView::~RootMotionView() { set_base(RID()); }

#endif // _3D_DISABLED


