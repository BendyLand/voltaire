/**************************************************************************/
/*  compositor.cpp                                                        */
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

#include "compositor.h"
#include "servers/rendering/rendering_server.h"

void CompositorEffect::set_enabled(bool p_enabled)
{
	enabled = p_enabled;
	if (rid.is_valid()) {
		RenderingServer* rs = RenderingServer::get_singleton();
		ERR_FAIL_NULL(rs);
		rs->compositor_effect_set_enabled(rid, enabled);
	}
}

bool CompositorEffect::get_enabled() const { return enabled; }

CompositorEffect::EffectCallbackType CompositorEffect::get_effect_callback_type() const
{
	return effect_callback_type;
}

void CompositorEffect::set_access_resolved_color(bool p_enabled)
{
	access_resolved_color = p_enabled;
	if (rid.is_valid()) {
		RenderingServer* rs = RenderingServer::get_singleton();
		ERR_FAIL_NULL(rs);
		rs->compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_COLOR,
			access_resolved_color);
	}
}

bool CompositorEffect::get_access_resolved_color() const { return access_resolved_color; }

void CompositorEffect::set_access_resolved_depth(bool p_enabled)
{
	access_resolved_depth = p_enabled;
	if (rid.is_valid()) {
		RenderingServer* rs = RenderingServer::get_singleton();
		ERR_FAIL_NULL(rs);
		rs->compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_DEPTH,
			access_resolved_depth);
	}
}

bool CompositorEffect::get_access_resolved_depth() const { return access_resolved_depth; }

void CompositorEffect::set_needs_motion_vectors(bool p_enabled)
{
	needs_motion_vectors = p_enabled;
	if (rid.is_valid()) {
		RenderingServer* rs = RenderingServer::get_singleton();
		ERR_FAIL_NULL(rs);
		rs->compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_MOTION_VECTORS,
			needs_motion_vectors);
	}
}

bool CompositorEffect::get_needs_motion_vectors() const { return needs_motion_vectors; }

void CompositorEffect::set_needs_normal_roughness(bool p_enabled)
{
	needs_normal_roughness = p_enabled;
	if (rid.is_valid()) {
		RenderingServer* rs = RenderingServer::get_singleton();
		ERR_FAIL_NULL(rs);
		rs->compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_ROUGHNESS,
			needs_normal_roughness);
	}
}

bool CompositorEffect::get_needs_normal_roughness() const { return needs_normal_roughness; }

void CompositorEffect::set_needs_separate_specular(bool p_enabled)
{
	needs_separate_specular = p_enabled;
	if (rid.is_valid()) {
		RenderingServer* rs = RenderingServer::get_singleton();
		ERR_FAIL_NULL(rs);
		rs->compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_SEPARATE_SPECULAR,
			needs_separate_specular);
	}
}

bool CompositorEffect::get_needs_separate_specular() const { return needs_separate_specular; }

CompositorEffect::~CompositorEffect()
{
	RenderingServer* rs = RenderingServer::get_singleton();
	if (rs != nullptr && rid.is_valid()) {
		rs->free_rid(rid);
	}
}

/* Compositor */


Compositor::Compositor()
{
	RenderingServer* rs = RenderingServer::get_singleton();
	if (rs != nullptr) {
		compositor = rs->compositor_create();
	}
}

Compositor::~Compositor()
{
	RenderingServer* rs = RenderingServer::get_singleton();
	if (rs != nullptr && compositor.is_valid()) {
		rs->free_rid(compositor);
	}
}

void CompositorEffect::_call_render_callback(
	int p_effect_callback_type, const RenderData* p_render_data)
{
}


