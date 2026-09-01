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

void ClusterBuilderRD::bake_cluster()
{
	RENDER_TIMESTAMP("> Bake 3D Cluster");

	RD::get_singleton()->draw_command_begin_label("Bake Light Cluster");

	// Clear cluster buffer.
	RD::get_singleton()->buffer_clear(cluster_buffer, 0, cluster_buffer_size);

	if (render_element_count > 0) {
		// Clear render buffer.
		RD::get_singleton()->buffer_clear(cluster_render_buffer, 0, cluster_render_buffer_size);

		{ // Fill state uniform.

			StateUniform state;

			RendererRD::MaterialStorage::store_camera(adjusted_projection, state.projection);
			state.inv_z_far = 1.0 / z_far;
			state.screen_to_clusters_shift = Math::get_shift_from_power_of_2(cluster_size);
			state.screen_to_clusters_shift -= divisor; // screen is smaller, shift one less

			state.cluster_screen_width = cluster_screen_size.x;
			state.cluster_depth_offset = (render_element_max / 32);
			state.cluster_data_size = state.cluster_depth_offset + render_element_max;

			RD::get_singleton()->buffer_update(state_uniform, 0, sizeof(StateUniform), &state);
		}

		// Update instances.

		RD::get_singleton()->buffer_update(
			element_buffer, 0, sizeof(RenderElementData) * render_element_count, render_elements);

		RENDER_TIMESTAMP("Render 3D Cluster Elements");

		// Render elements.
		{
			RD::DrawListID draw_list = RD::get_singleton()->draw_list_begin(framebuffer);
			ClusterBuilderSharedDataRD::ClusterRender::PushConstant push_constant = {};

			RD::get_singleton()->draw_list_bind_render_pipeline(draw_list,
				shared->cluster_render.shader_pipelines
					[use_msaa ? ClusterBuilderSharedDataRD::ClusterRender::PIPELINE_MSAA
							  : ClusterBuilderSharedDataRD::ClusterRender::PIPELINE_NORMAL]);
			RD::get_singleton()->draw_list_bind_uniform_set(
				draw_list, cluster_render_uniform_set, 0);

			for (uint32_t i = 0; i < render_element_count;) {
				push_constant.base_index = i;
				switch (render_elements[i].type) {
				case ELEMENT_TYPE_OMNI_LIGHT: {
					RD::get_singleton()->draw_list_bind_vertex_array(
						draw_list, shared->sphere_vertex_array);
					RD::get_singleton()->draw_list_bind_index_array(
						draw_list, shared->sphere_index_array);
				} break;
				case ELEMENT_TYPE_SPOT_LIGHT: {
					// If the spot angle is above a certain threshold, use a sphere instead of a
					// cone for building the clusters since the cone gets too flat/large (spot angle
					// close to 90 degrees) or can't even cover the affected area of the light (spot
					// angle above 90 degrees).
					if (render_elements[i].has_wide_spot_angle) {
						RD::get_singleton()->draw_list_bind_vertex_array(
							draw_list, shared->sphere_vertex_array);
						RD::get_singleton()->draw_list_bind_index_array(
							draw_list, shared->sphere_index_array);
					}
					else {
						RD::get_singleton()->draw_list_bind_vertex_array(
							draw_list, shared->cone_vertex_array);
						RD::get_singleton()->draw_list_bind_index_array(
							draw_list, shared->cone_index_array);
					}
				} break;
				case ELEMENT_TYPE_AREA_LIGHT: {
					RD::get_singleton()->draw_list_bind_vertex_array(
						draw_list, shared->box_vertex_array);
					RD::get_singleton()->draw_list_bind_index_array(
						draw_list, shared->box_index_array);
				} break;
				case ELEMENT_TYPE_DECAL:
				case ELEMENT_TYPE_REFLECTION_PROBE: {
					RD::get_singleton()->draw_list_bind_vertex_array(
						draw_list, shared->box_vertex_array);
					RD::get_singleton()->draw_list_bind_index_array(
						draw_list, shared->box_index_array);
				} break;
				}

				RD::get_singleton()->draw_list_set_push_constant(draw_list, &push_constant,
					sizeof(ClusterBuilderSharedDataRD::ClusterRender::PushConstant));

				uint32_t instances = 1;
				RD::get_singleton()->draw_list_draw(draw_list, true, instances);
				i += instances;
			}
			RD::get_singleton()->draw_list_end();
		}
		// Store elements.
		RENDER_TIMESTAMP("Pack 3D Cluster Elements");

		{
			RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
			RD::get_singleton()->compute_list_bind_compute_pipeline(
				compute_list, shared->cluster_store.shader_pipeline);
			RD::get_singleton()->compute_list_bind_uniform_set(
				compute_list, cluster_store_uniform_set, 0);

			ClusterBuilderSharedDataRD::ClusterStore::PushConstant push_constant;
			push_constant.cluster_render_data_size = render_element_max / 32 + render_element_max;
			push_constant.max_render_element_count_div_32 = render_element_max / 32;
			push_constant.cluster_screen_size[0] = cluster_screen_size.x;
			push_constant.cluster_screen_size[1] = cluster_screen_size.y;

			push_constant.render_element_count_div_32 =
				Math::division_round_up(render_element_count, 32U);
			push_constant.max_cluster_element_count_div_32 = max_elements_by_type / 32;
			push_constant.pad1 = 0;
			push_constant.pad2 = 0;

			RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant,
				sizeof(ClusterBuilderSharedDataRD::ClusterStore::PushConstant));

			RD::get_singleton()->compute_list_dispatch_threads(
				compute_list, cluster_screen_size.x, cluster_screen_size.y, 1);

			RD::get_singleton()->compute_list_end();
		}
	}
	RENDER_TIMESTAMP("< Bake 3D Cluster");
	RD::get_singleton()->draw_command_end_label();
}

void ClusterBuilderRD::debug(ElementType p_element)
{
	ERR_FAIL_COND(debug_uniform_set.is_null());
	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
	RD::get_singleton()->compute_list_bind_compute_pipeline(
		compute_list, shared->cluster_debug.shader_pipeline);
	RD::get_singleton()->compute_list_bind_uniform_set(compute_list, debug_uniform_set, 0);

	ClusterBuilderSharedDataRD::ClusterDebug::PushConstant push_constant;
	push_constant.screen_size[0] = screen_size.x;
	push_constant.screen_size[1] = screen_size.y;
	push_constant.cluster_screen_size[0] = cluster_screen_size.x;
	push_constant.cluster_screen_size[1] = cluster_screen_size.y;
	push_constant.cluster_shift = Math::get_shift_from_power_of_2(cluster_size);
	push_constant.cluster_type = p_element;
	push_constant.orthogonal = camera_orthogonal;
	push_constant.z_far = z_far;
	push_constant.z_near = z_near;
	push_constant.max_cluster_element_count_div_32 = max_elements_by_type / 32;

	RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant,
		sizeof(ClusterBuilderSharedDataRD::ClusterDebug::PushConstant));

	RD::get_singleton()->compute_list_dispatch_threads(
		compute_list, screen_size.x, screen_size.y, 1);

	RD::get_singleton()->compute_list_end();
}

RID ClusterBuilderRD::get_cluster_buffer() const { return cluster_buffer; }

uint32_t ClusterBuilderRD::get_cluster_size() const { return cluster_size; }

uint32_t ClusterBuilderRD::get_max_cluster_elements() const { return max_elements_by_type; }

void ClusterBuilderRD::set_shared(ClusterBuilderSharedDataRD* p_shared) { shared = p_shared; }

ClusterBuilderRD::ClusterBuilderRD()
{
	state_uniform = RD::get_singleton()->uniform_buffer_create(sizeof(StateUniform));
}

ClusterBuilderRD::~ClusterBuilderRD()
{
	_clear();
	RD::get_singleton()->free_rid(state_uniform);
}


