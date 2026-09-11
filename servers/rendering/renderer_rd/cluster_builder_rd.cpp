/**************************************************************************/
/*  cluster_builder_rd.cpp                                                */
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

#include "cluster_builder_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_globals.h" // IWYU pragma: keep. RENDER_TIMESTAMP macro uses RSG.

ClusterBuilderSharedDataRD::~ClusterBuilderSharedDataRD()
{
	RD::get_singleton()->free_rid(sphere_vertex_buffer);
	RD::get_singleton()->free_rid(sphere_index_buffer);
	RD::get_singleton()->free_rid(cone_vertex_buffer);
	RD::get_singleton()->free_rid(cone_index_buffer);
	RD::get_singleton()->free_rid(box_vertex_buffer);
	RD::get_singleton()->free_rid(box_index_buffer);

	cluster_render.cluster_render_shader.version_free(cluster_render.shader_version);
	cluster_store.cluster_store_shader.version_free(cluster_store.shader_version);
	cluster_debug.cluster_debug_shader.version_free(cluster_debug.shader_version);
}

/////////////////////////////

void ClusterBuilderRD::_clear()
{
	if (cluster_buffer.is_null()) {
		return;
	}

	RD::get_singleton()->free_rid(cluster_buffer);
	RD::get_singleton()->free_rid(cluster_render_buffer);
	RD::get_singleton()->free_rid(element_buffer);
	cluster_buffer = RID();
	cluster_render_buffer = RID();
	element_buffer = RID();

	memfree(render_elements);

	render_elements = nullptr;
	render_element_max = 0;
	render_element_count = 0;

	RD::get_singleton()->free_rid(framebuffer);
	framebuffer = RID();

	cluster_render_uniform_set = RID();
	cluster_store_uniform_set = RID();
}

void ClusterBuilderRD::setup(Size2i p_screen_size, uint32_t p_max_elements, RID p_depth_buffer,
	RID p_depth_buffer_sampler, RID p_color_buffer)
{
	ERR_FAIL_COND(p_max_elements == 0);
	ERR_FAIL_COND(p_screen_size.x < 1);
	ERR_FAIL_COND(p_screen_size.y < 1);

	_clear();

	screen_size = p_screen_size;

	cluster_screen_size.width =
		Math::division_round_up((uint32_t)p_screen_size.width, cluster_size);
	cluster_screen_size.height =
		Math::division_round_up((uint32_t)p_screen_size.height, cluster_size);

	max_elements_by_type = p_max_elements;
	if (max_elements_by_type % 32) { // Needs to be aligned to 32.
		max_elements_by_type += 32 - (max_elements_by_type % 32);
	}

	cluster_buffer_size = cluster_screen_size.x * cluster_screen_size.y *
						  (max_elements_by_type / 32 + 32) * ELEMENT_TYPE_MAX * 4;

	render_element_max = max_elements_by_type * ELEMENT_TYPE_MAX;

	uint32_t element_tag_bits_size = render_element_max / 32;
	uint32_t element_tag_depth_bits_size = render_element_max;

	cluster_render_buffer_size =
		cluster_screen_size.x * cluster_screen_size.y *
		(element_tag_bits_size + element_tag_depth_bits_size) *
		4; // Tag bits (element was used) and tag depth (depth range in which it was used).

	cluster_render_buffer = RD::get_singleton()->storage_buffer_create(cluster_render_buffer_size);
	cluster_buffer = RD::get_singleton()->storage_buffer_create(cluster_buffer_size);

	render_elements =
		static_cast<RenderElementData*>(memalloc(sizeof(RenderElementData) * render_element_max));
	render_element_count = 0;

	element_buffer =
		RD::get_singleton()->storage_buffer_create(sizeof(RenderElementData) * render_element_max);

	uint32_t div_value = 1 << divisor;
	if (use_msaa) {
		framebuffer = RD::get_singleton()->framebuffer_create_empty(
			p_screen_size / div_value, RD::TEXTURE_SAMPLES_4);
	}
	else {
		framebuffer = RD::get_singleton()->framebuffer_create_empty(p_screen_size / div_value);
	}

	{
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER;
			u.binding = 1;
			u.append_id(state_uniform);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 2;
			u.append_id(element_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 3;
			u.append_id(cluster_render_buffer);
			uniforms.push_back(u);
		}

		cluster_render_uniform_set =
			RD::get_singleton()->uniform_set_create(uniforms, shared->cluster_render.shader, 0);
	}

	{
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 1;
			u.append_id(cluster_render_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 2;
			u.append_id(cluster_buffer);
			uniforms.push_back(u);
		}

		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 3;
			u.append_id(element_buffer);
			uniforms.push_back(u);
		}

		cluster_store_uniform_set =
			RD::get_singleton()->uniform_set_create(uniforms, shared->cluster_store.shader, 0);
	}

	if (p_color_buffer.is_valid()) {
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 1;
			u.append_id(cluster_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
			u.binding = 2;
			u.append_id(p_color_buffer);
			uniforms.push_back(u);
		}

		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
			u.binding = 3;
			u.append_id(p_depth_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
			u.binding = 4;
			u.append_id(p_depth_buffer_sampler);
			uniforms.push_back(u);
		}

		debug_uniform_set =
			RD::get_singleton()->uniform_set_create(uniforms, shared->cluster_debug.shader, 0);
	}
	else {
		debug_uniform_set = RID();
	}
}

void ClusterBuilderRD::begin(
	const Transform3D& p_view_transform, const Projection& p_cam_projection, bool p_flip_y)
{
	view_xform = p_view_transform.affine_inverse();
	projection = p_cam_projection;
	z_near = projection.get_z_near();
	z_far = projection.get_z_far();
	camera_orthogonal = p_cam_projection.is_orthogonal();
	adjusted_projection = projection;
	if (!camera_orthogonal) {
		adjusted_projection.adjust_perspective_znear(0.0001);
	}

	Projection correction;
	correction.set_depth_correction(p_flip_y);
	projection = correction * projection;
	adjusted_projection = correction * adjusted_projection;

	// Reset counts.
	render_element_count = 0;
	for (uint32_t i = 0; i < ELEMENT_TYPE_MAX; i++) {
		cluster_count_by_type[i] = 0;
	}
}

RID ClusterBuilderRD::get_cluster_buffer() const { return cluster_buffer; }

uint32_t ClusterBuilderRD::get_cluster_size() const { return cluster_size; }

uint32_t ClusterBuilderRD::get_max_cluster_elements() const { return max_elements_by_type; }

void ClusterBuilderRD::set_shared(ClusterBuilderSharedDataRD* p_shared) { shared = p_shared; }

ClusterBuilderRD::~ClusterBuilderRD()
{
	_clear();
	RD::get_singleton()->free_rid(state_uniform);
}


