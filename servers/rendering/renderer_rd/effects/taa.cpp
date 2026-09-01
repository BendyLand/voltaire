/**************************************************************************/
/*  taa.cpp                                                               */
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

#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "taa.h"

using namespace RendererRD;

TAA::~TAA() { taa_shader.version_free(shader_version); }

void TAA::process(Ref<RenderSceneBuffersRD> p_render_buffers, RD::DataFormat p_format,
	float p_z_near, float p_z_far)
{
	CopyEffects* copy_effects = CopyEffects::get_singleton();

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i internal_size = p_render_buffers->get_internal_size();
	Size2i target_size = p_render_buffers->get_target_size();

	bool just_allocated = false;
	if (!p_render_buffers->has_texture(SNAME("taa"), SNAME("history"))) {
		uint32_t usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT;

		p_render_buffers->create_texture(SNAME("taa"), SNAME("history"), p_format, usage_bits);
		p_render_buffers->create_texture(SNAME("taa"), SNAME("temp"), p_format, usage_bits);

		p_render_buffers->create_texture(
			SNAME("taa"), SNAME("prev_velocity"), RD::DATA_FORMAT_R16G16_SFLOAT, usage_bits);

		just_allocated = true;
	}

	RD::get_singleton()->draw_command_begin_label("TAA");

	for (uint32_t v = 0; v < view_count; v++) {
		// Get our (cached) slices
		RID internal_texture = p_render_buffers->get_internal_texture(v);
		RID velocity_buffer = p_render_buffers->get_velocity_buffer(false, v);
		RID taa_history = p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("history"), v, 0);
		RID taa_prev_velocity =
			p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("prev_velocity"), v, 0);

		if (!just_allocated) {
			RID depth_texture = p_render_buffers->get_depth_texture(v);
			RID taa_temp = p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("temp"), v, 0);
			resolve(internal_texture, taa_temp, depth_texture, velocity_buffer, taa_prev_velocity,
				taa_history, Size2(internal_size.x, internal_size.y), p_z_near, p_z_far);
			copy_effects->copy_to_rect(
				taa_temp, internal_texture, Rect2(0, 0, internal_size.x, internal_size.y));
		}

		copy_effects->copy_to_rect(
			internal_texture, taa_history, Rect2(0, 0, internal_size.x, internal_size.y));
		copy_effects->copy_to_rect(
			velocity_buffer, taa_prev_velocity, Rect2(0, 0, target_size.x, target_size.y));
	}

	RD::get_singleton()->draw_command_end_label();
}


