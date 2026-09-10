/**************************************************************************/
/*  gi.cpp                                                                */
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

#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "gi.h"
#include "servers/rendering/renderer_rd/environment/fog.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_server_globals.h"

using namespace RendererRD;

const Vector3i GI::SDFGI::Cascade::DIRTY_ALL = Vector3i(0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);

GI* GI::singleton = nullptr;

RID GI::voxel_gi_allocate() { return voxel_gi_owner.allocate_rid(); }

void GI::voxel_gi_free(RID p_voxel_gi)
{
	voxel_gi_allocate_data(p_voxel_gi, Transform3D(), AABB(), Vector3i(), Vector<uint8_t>(),
		Vector<uint8_t>(), Vector<uint8_t>(), Vector<int>()); // deallocate
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	voxel_gi->dependency.deleted_notify(p_voxel_gi);
	voxel_gi_owner.free(p_voxel_gi);
}

void GI::voxel_gi_initialize(RID p_voxel_gi)
{
	voxel_gi_owner.initialize_rid(p_voxel_gi, VoxelGI());
}

void GI::voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
	const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	if (voxel_gi->octree_buffer.is_valid()) {
		RD::get_singleton()->free_rid(voxel_gi->octree_buffer);
		RD::get_singleton()->free_rid(voxel_gi->data_buffer);
		if (voxel_gi->sdf_texture.is_valid()) {
			RD::get_singleton()->free_rid(voxel_gi->sdf_texture);
		}

		voxel_gi->sdf_texture = RID();
		voxel_gi->octree_buffer = RID();
		voxel_gi->data_buffer = RID();
		voxel_gi->octree_buffer_size = 0;
		voxel_gi->data_buffer_size = 0;
		voxel_gi->cell_count = 0;
	}

	voxel_gi->to_cell_xform = p_to_cell_xform;
	voxel_gi->bounds = p_aabb;
	voxel_gi->octree_size = p_octree_size;
	voxel_gi->level_counts = p_level_counts;

	if (p_octree_cells.size()) {
		ERR_FAIL_COND(p_octree_cells.size() % 32 != 0); // cells size must be a multiple of 32

		uint32_t cell_count = p_octree_cells.size() / 32;

		ERR_FAIL_COND(p_data_cells.size() != (int)cell_count * 16); // see that data size matches

		voxel_gi->cell_count = cell_count;
		voxel_gi->octree_buffer =
			RD::get_singleton()->storage_buffer_create(p_octree_cells.size(), p_octree_cells);
		voxel_gi->octree_buffer_size = p_octree_cells.size();
		voxel_gi->data_buffer =
			RD::get_singleton()->storage_buffer_create(p_data_cells.size(), p_data_cells);
		voxel_gi->data_buffer_size = p_data_cells.size();

		if (p_distance_field.size()) {
			RD::TextureFormat tf;
			tf.format = RD::DATA_FORMAT_R8_UNORM;
			tf.width = voxel_gi->octree_size.x;
			tf.height = voxel_gi->octree_size.y;
			tf.depth = voxel_gi->octree_size.z;
			tf.texture_type = RD::TEXTURE_TYPE_3D;
			tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_CAN_UPDATE_BIT |
							RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
			Vector<Vector<uint8_t>> s;
			s.push_back(p_distance_field);
			voxel_gi->sdf_texture = RD::get_singleton()->texture_create(tf, RD::TextureView(), s);
			RD::get_singleton()->set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
		}
#if 0
			{
				RD::TextureFormat tf;
				tf.format = RD::DATA_FORMAT_R8_UNORM;
				tf.width = voxel_gi->octree_size.x;
				tf.height = voxel_gi->octree_size.y;
				tf.depth = voxel_gi->octree_size.z;
				tf.type = RD::TEXTURE_TYPE_3D;
				tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT;
				tf.shareable_formats.push_back(RD::DATA_FORMAT_R8_UNORM);
				tf.shareable_formats.push_back(RD::DATA_FORMAT_R8_UINT);
				voxel_gi->sdf_texture = RD::get_singleton()->texture_create(tf, RD::TextureView());
				RD::get_singleton()->set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
			}
			RID shared_tex;
			{
				RD::TextureView tv;
				tv.format_override = RD::DATA_FORMAT_R8_UINT;
				shared_tex = RD::get_singleton()->texture_create_shared(tv, voxel_gi->sdf_texture);
			}
			//update SDF texture
			Vector<RD::Uniform> uniforms;
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 1;
				u.append_id(voxel_gi->octree_buffer);
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 2;
				u.append_id(voxel_gi->data_buffer);
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
				u.binding = 3;
				u.append_id(shared_tex);
				uniforms.push_back(u);
			}

			RID uniform_set = RD::get_singleton()->uniform_set_create(uniforms, voxel_gi_sdf_shader_version_shader, 0);

			{
				uint32_t push_constant[4] = { 0, 0, 0, 0 };

				for (int i = 0; i < voxel_gi->level_counts.size() - 1; i++) {
					push_constant[0] += voxel_gi->level_counts[i];
				}
				push_constant[1] = push_constant[0] + voxel_gi->level_counts[voxel_gi->level_counts.size() - 1];

				print_line("offset: " + itos(push_constant[0]));
				print_line("size: " + itos(push_constant[1]));
				//create SDF
				RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
				RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, voxel_gi_sdf_shader_pipeline);
				RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set, 0);
				RD::get_singleton()->compute_list_set_push_constant(compute_list, push_constant, sizeof(uint32_t) * 4);
				RD::get_singleton()->compute_list_dispatch(compute_list, voxel_gi->octree_size.x / 4, voxel_gi->octree_size.y / 4, voxel_gi->octree_size.z / 4);
				RD::get_singleton()->compute_list_end();
			}

			RD::get_singleton()->free(uniform_set);
			RD::get_singleton()->free(shared_tex);
		}
#endif
	}

	voxel_gi->version++;
	voxel_gi->data_version++;

	voxel_gi->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB GI::voxel_gi_get_bounds(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, AABB());

	return voxel_gi->bounds;
}

Vector3i GI::voxel_gi_get_octree_size(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector3i());
	return voxel_gi->octree_size;
}

Vector<uint8_t> GI::voxel_gi_get_octree_cells(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->octree_buffer.is_valid()) {
		return RD::get_singleton()->buffer_get_data(voxel_gi->octree_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_data_cells(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::get_singleton()->buffer_get_data(voxel_gi->data_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_distance_field(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::get_singleton()->texture_get_data(voxel_gi->sdf_texture, 0);
	}
	return Vector<uint8_t>();
}

Vector<int> GI::voxel_gi_get_level_counts(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<int>());

	return voxel_gi->level_counts;
}

Transform3D GI::voxel_gi_get_to_cell_xform(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Transform3D());

	return voxel_gi->to_cell_xform;
}

void GI::voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->dynamic_range = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_dynamic_range(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);

	return voxel_gi->dynamic_range;
}

void GI::voxel_gi_set_propagation(RID p_voxel_gi, float p_range)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->propagation = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_propagation(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->propagation;
}

void GI::voxel_gi_set_energy(RID p_voxel_gi, float p_energy)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->energy = p_energy;
}

float GI::voxel_gi_get_energy(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->energy;
}

void GI::voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->baked_exposure = p_baked_exposure;
}

float GI::voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->baked_exposure;
}

void GI::voxel_gi_set_bias(RID p_voxel_gi, float p_bias)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->bias = p_bias;
}

float GI::voxel_gi_get_bias(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->bias;
}

void GI::voxel_gi_set_normal_bias(RID p_voxel_gi, float p_normal_bias)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->normal_bias = p_normal_bias;
}

float GI::voxel_gi_get_normal_bias(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->normal_bias;
}

void GI::voxel_gi_set_interior(RID p_voxel_gi, bool p_enable)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->interior = p_enable;
}

void GI::voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->use_two_bounces = p_enable;
	voxel_gi->version++;
}

bool GI::voxel_gi_is_using_two_bounces(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->use_two_bounces;
}

bool GI::voxel_gi_is_interior(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->interior;
}

uint32_t GI::voxel_gi_get_version(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->version;
}

uint32_t GI::voxel_gi_get_data_version(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->data_version;
}

RID GI::voxel_gi_get_octree_buffer(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->octree_buffer;
}

RID GI::voxel_gi_get_data_buffer(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->data_buffer;
}

RID GI::voxel_gi_get_sdf_texture(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());

	return voxel_gi->sdf_texture;
}

Dependency* GI::voxel_gi_get_dependency(RID p_voxel_gi) const
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, nullptr);

	return &voxel_gi->dependency;
}

void GI::sdfgi_reset() { sdfgi_current_version++; }

static RID create_clear_texture(const RD::TextureFormat& p_format, const String& p_name)
{
	RID texture = RD::get_singleton()->texture_create(p_format, RD::TextureView());
	ERR_FAIL_COND_V_MSG(texture.is_null(), RID(), String("Cannot create texture: ") + p_name);

	RD::get_singleton()->set_resource_name(texture, p_name);
	RD::get_singleton()->texture_clear(
		texture, Color(0, 0, 0, 0), 0, p_format.mipmaps, 0, p_format.array_layers);

	return texture;
}

void GI::SDFGI::free_data()
{
	// we don't free things here, we handle SDFGI differently at the moment destructing the object
	// when it needs to change.
}

GI::SDFGI::~SDFGI()
{
	for (const SDFGI::Cascade& c : cascades) {
		RD::get_singleton()->free_rid(c.light_data);
		RD::get_singleton()->free_rid(c.light_aniso_0_tex);
		RD::get_singleton()->free_rid(c.light_aniso_1_tex);
		RD::get_singleton()->free_rid(c.sdf_tex);
		RD::get_singleton()->free_rid(c.solid_cell_dispatch_buffer_storage);
		RD::get_singleton()->free_rid(c.solid_cell_dispatch_buffer_call);
		RD::get_singleton()->free_rid(c.solid_cell_buffer);
		RD::get_singleton()->free_rid(c.lightprobe_history_tex);
		RD::get_singleton()->free_rid(c.lightprobe_average_tex);
		RD::get_singleton()->free_rid(c.lights_buffer);
	}

	RD::get_singleton()->free_rid(render_albedo);
	RD::get_singleton()->free_rid(render_emission);
	RD::get_singleton()->free_rid(render_emission_aniso);

	RD::get_singleton()->free_rid(render_sdf[0]);
	RD::get_singleton()->free_rid(render_sdf[1]);

	RD::get_singleton()->free_rid(render_sdf_half[0]);
	RD::get_singleton()->free_rid(render_sdf_half[1]);

	for (int i = 0; i < 8; i++) {
		RD::get_singleton()->free_rid(render_occlusion[i]);
	}

	RD::get_singleton()->free_rid(render_geom_facing);

	RD::get_singleton()->free_rid(lightprobe_data);
	RD::get_singleton()->free_rid(lightprobe_history_scroll);
	RD::get_singleton()->free_rid(lightprobe_average_scroll);
	RD::get_singleton()->free_rid(occlusion_data);
	RD::get_singleton()->free_rid(ambient_texture);

	RD::get_singleton()->free_rid(cascades_ubo);

	for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
		if (RD::get_singleton()->uniform_set_is_valid(debug_uniform_set[v])) {
			RD::get_singleton()->free_rid(debug_uniform_set[v]);
		}
		debug_uniform_set[v] = RID();
	}

	if (RD::get_singleton()->uniform_set_is_valid(debug_probes_uniform_set)) {
		RD::get_singleton()->free_rid(debug_probes_uniform_set);
	}
	debug_probes_uniform_set = RID();

	if (debug_probes_scene_data_ubo.is_valid()) {
		RD::get_singleton()->free_rid(debug_probes_scene_data_ubo);
		debug_probes_scene_data_ubo = RID();
	}
}

void GI::SDFGI::update(RID p_env, const Vector3& p_world_position)
{
	bounce_feedback =
		RendererSceneRenderRD::get_singleton()->environment_get_sdfgi_bounce_feedback(p_env);
	energy = RendererSceneRenderRD::get_singleton()->environment_get_sdfgi_energy(p_env);
	normal_bias = RendererSceneRenderRD::get_singleton()->environment_get_sdfgi_normal_bias(p_env);
	probe_bias = RendererSceneRenderRD::get_singleton()->environment_get_sdfgi_probe_bias(p_env);
	reads_sky = RendererSceneRenderRD::get_singleton()->environment_get_sdfgi_read_sky_light(p_env);

	int32_t drag_margin = (cascade_size / SDFGI::PROBE_DIVISOR) / 2;

	for (SDFGI::Cascade& cascade : cascades) {
		cascade.dirty_regions = Vector3i();

		Vector3 probe_half_size =
			Vector3(1, 1, 1) * cascade.cell_size * float(cascade_size / SDFGI::PROBE_DIVISOR) * 0.5;
		probe_half_size = Vector3(0, 0, 0);

		Vector3 world_position = p_world_position;
		world_position.y *= y_mult;
		Vector3i pos_in_cascade = Vector3i((world_position + probe_half_size) / cascade.cell_size);

		for (int j = 0; j < 3; j++) {
			if (pos_in_cascade[j] < cascade.position[j]) {
				while (pos_in_cascade[j] < (cascade.position[j] - drag_margin)) {
					cascade.position[j] -= drag_margin * 2;
					cascade.dirty_regions[j] += drag_margin * 2;
				}
			}
			else if (pos_in_cascade[j] > cascade.position[j]) {
				while (pos_in_cascade[j] > (cascade.position[j] + drag_margin)) {
					cascade.position[j] += drag_margin * 2;
					cascade.dirty_regions[j] -= drag_margin * 2;
				}
			}

			if (cascade.dirty_regions[j] == 0) {
				continue; // not dirty
			}
			else if (uint32_t(Math::abs(cascade.dirty_regions[j])) >= cascade_size) {
				// moved too much, just redraw everything (make all dirty)
				cascade.dirty_regions = SDFGI::Cascade::DIRTY_ALL;
				break;
			}
		}

		if (cascade.dirty_regions != Vector3i() &&
			cascade.dirty_regions != SDFGI::Cascade::DIRTY_ALL) {
			// see how much the total dirty volume represents from the total volume
			uint32_t total_volume = cascade_size * cascade_size * cascade_size;
			uint32_t safe_volume = 1;
			for (int j = 0; j < 3; j++) {
				safe_volume *= cascade_size - Math::abs(cascade.dirty_regions[j]);
			}
			uint32_t dirty_volume = total_volume - safe_volume;
			if (dirty_volume > (safe_volume / 2)) {
				// more than half the volume is dirty, make all dirty so its only rendered once
				cascade.dirty_regions = SDFGI::Cascade::DIRTY_ALL;
			}
		}
	}
}

int GI::SDFGI::get_pending_region_data(
	int p_region, Vector3i& r_local_offset, Vector3i& r_local_size, AABB& r_bounds) const
{
	int dirty_count = 0;
	for (uint32_t i = 0; i < cascades.size(); i++) {
		const SDFGI::Cascade& c = cascades[i];

		if (c.dirty_regions == SDFGI::Cascade::DIRTY_ALL) {
			if (dirty_count == p_region) {
				r_local_offset = Vector3i();
				r_local_size = Vector3i(1, 1, 1) * cascade_size;

				r_bounds.position =
					Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + c.position)) *
					c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				r_bounds.size = Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				return i;
			}
			dirty_count++;
		}
		else {
			for (int j = 0; j < 3; j++) {
				if (c.dirty_regions[j] != 0) {
					if (dirty_count == p_region) {
						Vector3i from = Vector3i(0, 0, 0);
						Vector3i to = Vector3i(1, 1, 1) * cascade_size;

						if (c.dirty_regions[j] > 0) {
							// fill from the beginning
							to[j] = c.dirty_regions[j];
						}
						else {
							// fill from the end
							from[j] = to[j] + c.dirty_regions[j];
						}

						for (int k = 0; k < j; k++) {
							// "chip" away previous regions to avoid re-voxelizing the same thing
							if (c.dirty_regions[k] > 0) {
								from[k] += c.dirty_regions[k];
							}
							else if (c.dirty_regions[k] < 0) {
								to[k] += c.dirty_regions[k];
							}
						}

						r_local_offset = from;
						r_local_size = to - from;

						r_bounds.position =
							Vector3(from + Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) +
									c.position) *
							c.cell_size * Vector3(1, 1.0 / y_mult, 1);
						r_bounds.size =
							Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);

						return i;
					}

					dirty_count++;
				}
			}
		}
	}
	return -1;
}

void GI::SDFGI::update_cascades()
{
	// update cascades
	SDFGI::Cascade::UBO cascade_data[SDFGI::MAX_CASCADES];
	int32_t probe_divisor = cascade_size / SDFGI::PROBE_DIVISOR;

	for (uint32_t i = 0; i < cascades.size(); i++) {
		Vector3 pos =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascades[i].position)) *
			cascades[i].cell_size;

		cascade_data[i].offset[0] = pos.x;
		cascade_data[i].offset[1] = pos.y;
		cascade_data[i].offset[2] = pos.z;
		cascade_data[i].to_cell = 1.0 / cascades[i].cell_size;
		cascade_data[i].probe_offset[0] = cascades[i].position.x / probe_divisor;
		cascade_data[i].probe_offset[1] = cascades[i].position.y / probe_divisor;
		cascade_data[i].probe_offset[2] = cascades[i].position.z / probe_divisor;
		cascade_data[i].pad = 0;
	}
}

void GI::SDFGI::pre_process_gi(const Transform3D& p_transform, RenderDataRD* p_render_data)
{
	if (p_render_data->sdfgi_update_data == nullptr) {
		return;
	}

	RendererRD::LightStorage* light_storage = RendererRD::LightStorage::get_singleton();
	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();
	/* Update general SDFGI Buffer */

	SDFGIData sdfgi_data;

	sdfgi_data.grid_size[0] = cascade_size;
	sdfgi_data.grid_size[1] = cascade_size;
	sdfgi_data.grid_size[2] = cascade_size;

	sdfgi_data.max_cascades = cascades.size();
	sdfgi_data.probe_axis_size = probe_axis_count;
	sdfgi_data.cascade_probe_size[0] =
		sdfgi_data.probe_axis_size - 1; // float version for performance
	sdfgi_data.cascade_probe_size[1] = sdfgi_data.probe_axis_size - 1;
	sdfgi_data.cascade_probe_size[2] = sdfgi_data.probe_axis_size - 1;

	float csize = cascade_size;
	sdfgi_data.probe_to_uvw = 1.0 / float(sdfgi_data.cascade_probe_size[0]);
	sdfgi_data.use_occlusion = uses_occlusion;
	// sdfgi_data.energy = energy;

	sdfgi_data.y_mult = y_mult;

	float cascade_voxel_size = (csize / sdfgi_data.cascade_probe_size[0]);
	float occlusion_clamp = (cascade_voxel_size - 0.5) / cascade_voxel_size;
	sdfgi_data.occlusion_clamp[0] = occlusion_clamp;
	sdfgi_data.occlusion_clamp[1] = occlusion_clamp;
	sdfgi_data.occlusion_clamp[2] = occlusion_clamp;
	sdfgi_data.normal_bias = (normal_bias / csize) * sdfgi_data.cascade_probe_size[0];

	// vec2 tex_pixel_size = 1.0 / vec2(ivec2( (OCT_SIZE+2) * params.probe_axis_size *
	// params.probe_axis_size, (OCT_SIZE+2) * params.probe_axis_size ) ); vec3 probe_uv_offset =
	// (ivec3(OCT_SIZE+2,OCT_SIZE+2,(OCT_SIZE+2) * params.probe_axis_size)) * tex_pixel_size.xyx;

	uint32_t oct_size = SDFGI::LIGHTPROBE_OCT_SIZE;

	sdfgi_data.lightprobe_tex_pixel_size[0] =
		1.0 / ((oct_size + 2) * sdfgi_data.probe_axis_size * sdfgi_data.probe_axis_size);
	sdfgi_data.lightprobe_tex_pixel_size[1] = 1.0 / ((oct_size + 2) * sdfgi_data.probe_axis_size);
	sdfgi_data.lightprobe_tex_pixel_size[2] = 1.0;

	sdfgi_data.energy = energy;

	sdfgi_data.lightprobe_uv_offset[0] =
		float(oct_size + 2) * sdfgi_data.lightprobe_tex_pixel_size[0];
	sdfgi_data.lightprobe_uv_offset[1] =
		float(oct_size + 2) * sdfgi_data.lightprobe_tex_pixel_size[1];
	sdfgi_data.lightprobe_uv_offset[2] = float((oct_size + 2) * sdfgi_data.probe_axis_size) *
										 sdfgi_data.lightprobe_tex_pixel_size[0];

	sdfgi_data.occlusion_renormalize[0] = 0.5;
	sdfgi_data.occlusion_renormalize[1] = 1.0;
	sdfgi_data.occlusion_renormalize[2] = 1.0 / float(sdfgi_data.max_cascades);

	int32_t probe_divisor = cascade_size / SDFGI::PROBE_DIVISOR;

	for (uint32_t i = 0; i < sdfgi_data.max_cascades; i++) {
		SDFGIData::ProbeCascadeData& c = sdfgi_data.cascades[i];
		Vector3 pos =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascades[i].position)) *
			cascades[i].cell_size;
		Vector3 cam_origin = p_transform.origin;
		cam_origin.y *= y_mult;
		pos -= cam_origin; // make pos local to camera, to reduce numerical error
		c.position[0] = pos.x;
		c.position[1] = pos.y;
		c.position[2] = pos.z;
		c.to_probe =
			1.0 / (float(cascade_size) * cascades[i].cell_size / float(probe_axis_count - 1));

		Vector3i probe_ofs = cascades[i].position / probe_divisor;
		c.probe_world_offset[0] = probe_ofs.x;
		c.probe_world_offset[1] = probe_ofs.y;
		c.probe_world_offset[2] = probe_ofs.z;

		c.to_cell = 1.0 / cascades[i].cell_size;
		c.exposure_normalization = 1.0;
		if (p_render_data->camera_attributes.is_valid()) {
			float exposure_normalization =
				RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(
					p_render_data->camera_attributes);
			c.exposure_normalization =
				exposure_normalization / cascades[i].baked_exposure_normalization;
		}
	}

	/* Update dynamic lights in SDFGI cascades */

	for (uint32_t i = 0; i < cascades.size(); i++) {
		SDFGI::Cascade& cascade = cascades[i];

		SDFGIShader::Light lights[SDFGI::MAX_DYNAMIC_LIGHTS];
		uint32_t idx = 0;
		for (uint32_t j = 0;
			 j < (uint32_t)p_render_data->sdfgi_update_data->directional_lights->size(); j++) {
			if (idx == SDFGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->sdfgi_update_data->directional_lights->get(j);
			ERR_CONTINUE(!light_storage->owns_light_instance(light_instance));

			RID light = light_storage->light_instance_get_base_light(light_instance);
			Transform3D light_transform =
				light_storage->light_instance_get_base_transform(light_instance);

			if (RSG::light_storage->light_directional_get_sky_mode(light) ==
				RSE::LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			dir.y *= y_mult;
			dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Color color = RSG::light_storage->light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RSE::LIGHT_DIRECTIONAL;
			lights[idx].energy =
				RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_ENERGY) *
				RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
				lights[idx].energy *=
					RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_INTENSITY);
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *=
					RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RSG::light_storage->light_has_shadow(light);

			idx++;
		}

		AABB cascade_aabb;
		cascade_aabb.position =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascade.position)) *
			cascade.cell_size;
		cascade_aabb.size = Vector3(1, 1, 1) * cascade_size * cascade.cell_size;

		for (uint32_t j = 0; j < p_render_data->sdfgi_update_data->positional_light_count; j++) {
			if (idx == SDFGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->sdfgi_update_data->positional_light_instances[j];
			ERR_CONTINUE(!light_storage->owns_light_instance(light_instance));

			RID light = light_storage->light_instance_get_base_light(light_instance);
			AABB light_aabb = light_storage->light_instance_get_base_aabb(light_instance);
			Transform3D light_transform =
				light_storage->light_instance_get_base_transform(light_instance);

			uint32_t max_sdfgi_cascade = RSG::light_storage->light_get_max_sdfgi_cascade(light);
			if (i > max_sdfgi_cascade) {
				continue;
			}

			if (!cascade_aabb.intersects(light_aabb)) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			Vector2 area_size = RSG::light_storage->light_area_get_size(light);
			// faster to not do this here
			// dir.y *= y_mult;
			// dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Vector3 pos = light_transform.origin;
			pos.y *= y_mult;
			lights[idx].position[0] = pos.x;
			lights[idx].position[1] = pos.y;
			lights[idx].position[2] = pos.z;
			Color color = RSG::light_storage->light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RSG::light_storage->light_get_type(light);

			lights[idx].energy =
				RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_ENERGY) *
				RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
				lights[idx].energy *=
					RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_INTENSITY);

				// Convert from Luminous Power to Luminous Intensity
				if (lights[idx].type == RSE::LIGHT_OMNI) {
					lights[idx].energy *= 1.0 / (Math::PI * 4.0);
				}
				else if (lights[idx].type == RSE::LIGHT_SPOT) {
					// Spot Lights are not physically accurate, Luminous Intensity should change in
					// relation to the cone angle. We make this assumption to keep them easy to
					// control.
					lights[idx].energy *= 1.0 / Math::PI;
				}
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *=
					RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RSG::light_storage->light_has_shadow(light);
			lights[idx].attenuation =
				RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_ATTENUATION);
			lights[idx].radius = RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_RANGE);
			lights[idx].cos_spot_angle = Math::cos(Math::deg_to_rad(
				RSG::light_storage->light_get_param(light, RSE::LIGHT_PARAM_SPOT_ANGLE)));
			lights[idx].inv_spot_attenuation = 1.0f / RSG::light_storage->light_get_param(
														  light, RSE::LIGHT_PARAM_SPOT_ATTENUATION);

			if (lights[idx].type == RSE::LIGHT_AREA) {
				Vector3 area_vec_a = light_transform.basis.get_column(0).normalized() * area_size.x;
				Vector3 area_vec_b = light_transform.basis.get_column(1).normalized() * area_size.y;
				Rect2 proj_rect = texture_storage->area_light_atlas_get_texture_rect(
					RSG::light_storage->light_area_get_texture(light));
				lights[idx].area_width[0] = area_vec_a.x;
				lights[idx].area_width[1] = area_vec_a.y;
				lights[idx].area_width[2] = area_vec_a.z;
				lights[idx].area_height[0] = area_vec_b.x;
				lights[idx].area_height[1] = area_vec_b.y;
				lights[idx].area_height[2] = area_vec_b.z;
				lights[idx].area_projector_rect[0] = proj_rect.position.x;
				lights[idx].area_projector_rect[1] = proj_rect.position.y;
				lights[idx].area_projector_rect[2] = proj_rect.size.x;
				lights[idx].area_projector_rect[3] = proj_rect.size.y;

				Size2i texture_size = proj_rect.size * texture_storage->area_light_atlas_get_size();
				lights[idx].cos_spot_angle =
					MIN(Math::floor(Math::log2(MAX(MIN(texture_size.x, texture_size.y), 1.0f))),
						texture_storage->area_light_atlas_get_mipmaps()) -
					1.0f; // max mipmaps
				lights[idx].inv_spot_attenuation =
					1.0f / (lights[idx].radius + area_size.length() / 2.0f); // center range

				if (RSG::light_storage->light_area_get_normalize_energy(light)) {
					// normalization to make larger lights output same amount of light as smaller
					// lights with same energy
					float surface_area = area_size.x * area_size.y;
					lights[idx].energy /= surface_area;
				}
			}

			idx++;
		}

		cascade_dynamic_light_count[i] = idx;
	}
}

void GI::VoxelGIInstance::free_resources()
{
	if (texture.is_valid()) {
		RD::get_singleton()->free_rid(texture);
		RD::get_singleton()->free_rid(write_buffer);

		texture = RID();
		write_buffer = RID();
		mipmaps.clear();
	}

	for (int i = 0; i < dynamic_maps.size(); i++) {
		RD::get_singleton()->free_rid(dynamic_maps[i].texture);
		RD::get_singleton()->free_rid(dynamic_maps[i].depth);

		// these only exist on the first level...
		if (dynamic_maps[i].fb_depth.is_valid()) {
			RD::get_singleton()->free_rid(dynamic_maps[i].fb_depth);
		}
		if (dynamic_maps[i].albedo.is_valid()) {
			RD::get_singleton()->free_rid(dynamic_maps[i].albedo);
		}
		if (dynamic_maps[i].normal.is_valid()) {
			RD::get_singleton()->free_rid(dynamic_maps[i].normal);
		}
		if (dynamic_maps[i].orm.is_valid()) {
			RD::get_singleton()->free_rid(dynamic_maps[i].orm);
		}
	}
	dynamic_maps.clear();
}

void GI::VoxelGIInstance::debug(RD::DrawListID p_draw_list, RID p_framebuffer,
	const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha)
{
	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();

	if (mipmaps.is_empty()) {
		return;
	}

	Projection cam_transform = (p_camera_with_transform * Projection(transform)) *
							   Projection(gi->voxel_gi_get_to_cell_xform(probe).affine_inverse());

	int level = 0;
	Vector3i octree_size = gi->voxel_gi_get_octree_size(probe);

	VoxelGIDebugPushConstant push_constant;
	push_constant.alpha = p_alpha;
	push_constant.dynamic_range = gi->voxel_gi_get_dynamic_range(probe);
	push_constant.cell_offset = mipmaps[level].cell_offset;
	push_constant.level = level;

	push_constant.bounds[0] = octree_size.x >> level;
	push_constant.bounds[1] = octree_size.y >> level;
	push_constant.bounds[2] = octree_size.z >> level;
	push_constant.pad = 0;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			push_constant.projection[i * 4 + j] = cam_transform.columns[i][j];
		}
	}

	if (gi->voxel_gi_debug_uniform_set.is_valid()) {
		RD::get_singleton()->free_rid(gi->voxel_gi_debug_uniform_set);
	}
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 1;
		u.append_id(gi->voxel_gi_get_data_buffer(probe));
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 2;
		u.append_id(texture);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_SAMPLER;
		u.binding = 3;
		u.append_id(material_storage->sampler_rd_get_default(
			RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
		uniforms.push_back(u);
	}

	int cell_count;
	if (!p_emission && p_lighting && has_dynamic_object_data) {
		cell_count = push_constant.bounds[0] * push_constant.bounds[1] * push_constant.bounds[2];
	}
	else {
		cell_count = mipmaps[level].cell_count;
	}

	gi->voxel_gi_debug_uniform_set = RD::get_singleton()->uniform_set_create(
		uniforms, gi->voxel_gi_debug_shader_version_shaders[0], 0);

	int voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_COLOR;
	if (p_emission) {
		voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_EMISSION;
	}
	else if (p_lighting) {
		voxel_gi_debug_pipeline =
			has_dynamic_object_data ? VOXEL_GI_DEBUG_LIGHT_FULL : VOXEL_GI_DEBUG_LIGHT;
	}
	RD::get_singleton()->draw_list_bind_render_pipeline(p_draw_list,
		gi->voxel_gi_debug_shader_version_pipelines[voxel_gi_debug_pipeline].get_render_pipeline(
			RD::INVALID_ID, RD::get_singleton()->framebuffer_get_format(p_framebuffer)));
	RD::get_singleton()->draw_list_bind_uniform_set(p_draw_list, gi->voxel_gi_debug_uniform_set, 0);
	RD::get_singleton()->draw_list_set_push_constant(
		p_draw_list, &push_constant, sizeof(VoxelGIDebugPushConstant));
	RD::get_singleton()->draw_list_draw(p_draw_list, false, cell_count, 36);
}

GI::~GI()
{
	for (int v = 0; v < SHADER_SPECIALIZATION_VARIATIONS; v++) {
		for (int i = 0; i < MODE_MAX; i++) {
			pipelines[v][i].free();
		}
	}

	sdfgi_shader.debug_pipeline.free();

	for (int i = 0; i < SDFGIShader::DIRECT_LIGHT_MODE_MAX; i++) {
		sdfgi_shader.direct_light_pipeline[i].free();
	}

	for (int i = 0; i < SDFGIShader::INTEGRATE_MODE_MAX; i++) {
		sdfgi_shader.integrate_pipeline[i].free();
	}

	for (int i = 0; i < SDFGIShader::PRE_PROCESS_MAX; i++) {
		sdfgi_shader.preprocess_pipeline[i].free();
	}

	for (int i = 0; i < VOXEL_GI_SHADER_VERSION_MAX; i++) {
		voxel_gi_lighting_shader_version_pipelines[i].free();
	}

	if (voxel_gi_debug_shader_version.is_valid()) {
		voxel_gi_debug_shader.version_free(voxel_gi_debug_shader_version);
	}
	if (voxel_gi_lighting_shader_version.is_valid()) {
		voxel_gi_shader.version_free(voxel_gi_lighting_shader_version);
	}
	if (shader_version.is_valid()) {
		shader.version_free(shader_version);
	}
	if (sdfgi_shader.debug_probes_shader.is_valid()) {
		sdfgi_shader.debug_probes.version_free(sdfgi_shader.debug_probes_shader);
	}
	if (sdfgi_shader.debug_shader.is_valid()) {
		sdfgi_shader.debug.version_free(sdfgi_shader.debug_shader);
	}
	if (sdfgi_shader.direct_light_shader.is_valid()) {
		sdfgi_shader.direct_light.version_free(sdfgi_shader.direct_light_shader);
	}
	if (sdfgi_shader.integrate_shader.is_valid()) {
		sdfgi_shader.integrate.version_free(sdfgi_shader.integrate_shader);
	}
	if (sdfgi_shader.preprocess_shader.is_valid()) {
		sdfgi_shader.preprocess.version_free(sdfgi_shader.preprocess_shader);
	}

	singleton = nullptr;
}

void GI::free()
{
	if (default_voxel_gi_buffer.is_valid()) {
		RD::get_singleton()->free_rid(default_voxel_gi_buffer);
	}
	if (voxel_gi_lights_uniform.is_valid()) {
		RD::get_singleton()->free_rid(voxel_gi_lights_uniform);
	}
	if (sdfgi_ubo.is_valid()) {
		RD::get_singleton()->free_rid(sdfgi_ubo);
	}

	if (voxel_gi_lights) {
		memdelete_arr(voxel_gi_lights);
	}
}

Ref<GI::SDFGI> GI::create_sdfgi(
	RID p_env, const Vector3& p_world_position, uint32_t p_requested_history_size)
{
	Ref<SDFGI> sdfgi;
	sdfgi.instantiate();

	sdfgi->create(p_env, p_world_position, p_requested_history_size, this);

	return sdfgi;
}

void GI::setup_voxel_gi_instances(RenderDataRD* p_render_data,
	Ref<RenderSceneBuffersRD> p_render_buffers, const Transform3D& p_transform,
	const PagedArray<RID>& p_voxel_gi_instances, uint32_t& r_voxel_gi_instances_used)
{
	ERR_FAIL_COND(p_render_buffers.is_null());

	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();
	ERR_FAIL_NULL(texture_storage);

	r_voxel_gi_instances_used = 0;

	Ref<RenderBuffersGI> rbgi = p_render_buffers->get_custom_data(RB_SCOPE_GI);
	ERR_FAIL_COND(rbgi.is_null());

	RID voxel_gi_buffer = rbgi->get_voxel_gi_buffer();
	VoxelGIData voxel_gi_data[MAX_VOXEL_GI_INSTANCES];

	bool voxel_gi_instances_changed = false;

	Transform3D to_camera;
	to_camera.origin = p_transform.origin; // only translation, make local

	for (int i = 0; i < MAX_VOXEL_GI_INSTANCES; i++) {
		RID texture;
		if (i < (int)p_voxel_gi_instances.size()) {
			VoxelGIInstance* gipi = voxel_gi_instance_owner.get_or_null(p_voxel_gi_instances[i]);

			if (gipi) {
				texture = gipi->texture;
				VoxelGIData& gipd = voxel_gi_data[i];

				RID base_probe = gipi->probe;

				Transform3D to_cell = voxel_gi_get_to_cell_xform(gipi->probe) *
									  gipi->transform.affine_inverse() * to_camera;

				gipd.xform[0] = to_cell.basis.rows[0][0];
				gipd.xform[1] = to_cell.basis.rows[1][0];
				gipd.xform[2] = to_cell.basis.rows[2][0];
				gipd.xform[3] = 0;
				gipd.xform[4] = to_cell.basis.rows[0][1];
				gipd.xform[5] = to_cell.basis.rows[1][1];
				gipd.xform[6] = to_cell.basis.rows[2][1];
				gipd.xform[7] = 0;
				gipd.xform[8] = to_cell.basis.rows[0][2];
				gipd.xform[9] = to_cell.basis.rows[1][2];
				gipd.xform[10] = to_cell.basis.rows[2][2];
				gipd.xform[11] = 0;
				gipd.xform[12] = to_cell.origin.x;
				gipd.xform[13] = to_cell.origin.y;
				gipd.xform[14] = to_cell.origin.z;
				gipd.xform[15] = 1;

				Vector3 bounds = voxel_gi_get_octree_size(base_probe);

				gipd.bounds[0] = bounds.x;
				gipd.bounds[1] = bounds.y;
				gipd.bounds[2] = bounds.z;

				gipd.dynamic_range =
					voxel_gi_get_dynamic_range(base_probe) * voxel_gi_get_energy(base_probe);
				gipd.bias = voxel_gi_get_bias(base_probe);
				gipd.normal_bias = voxel_gi_get_normal_bias(base_probe);
				gipd.blend_ambient = !voxel_gi_is_interior(base_probe);
				gipd.mipmaps = gipi->mipmaps.size();
				gipd.exposure_normalization = 1.0;
				if (p_render_data->camera_attributes.is_valid()) {
					float exposure_normalization =
						RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(
							p_render_data->camera_attributes);
					gipd.exposure_normalization =
						exposure_normalization /
						voxel_gi_get_baked_exposure_normalization(base_probe);
				}
			}

			r_voxel_gi_instances_used++;
		}

		if (texture == RID()) {
			texture = texture_storage->texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		}

		if (texture != rbgi->voxel_gi_textures[i]) {
			voxel_gi_instances_changed = true;
			rbgi->voxel_gi_textures[i] = texture;
		}
	}

	if (voxel_gi_instances_changed) {
		for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
			if (RD::get_singleton()->uniform_set_is_valid(rbgi->uniform_set[v])) {
				RD::get_singleton()->free_rid(rbgi->uniform_set[v]);
			}
			rbgi->uniform_set[v] = RID();
		}

		if (p_render_buffers->has_custom_data(RB_SCOPE_FOG)) {
			// VoxelGI instances have changed, so we need to update volumetric fog.
			Ref<RendererRD::Fog::VolumetricFog> fog =
				p_render_buffers->get_custom_data(RB_SCOPE_FOG);
			fog->sync_gi_dependent_sets_validity(true);
		}
	}

	if (p_voxel_gi_instances.size() > 0) {
		RD::get_singleton()->draw_command_begin_label("VoxelGIs Setup");
		RD::get_singleton()->draw_command_end_label();
	}
}

void GI::RenderBuffersGI::free_data()
{
	for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
		if (RD::get_singleton()->uniform_set_is_valid(uniform_set[v])) {
			RD::get_singleton()->free_rid(uniform_set[v]);
		}
		uniform_set[v] = RID();
	}

	if (scene_data_ubo.is_valid()) {
		RD::get_singleton()->free_rid(scene_data_ubo);
		scene_data_ubo = RID();
	}

	if (voxel_gi_buffer.is_valid()) {
		RD::get_singleton()->free_rid(voxel_gi_buffer);
		voxel_gi_buffer = RID();
	}
}

RID GI::voxel_gi_instance_create(RID p_base)
{
	VoxelGIInstance voxel_gi;
	voxel_gi.gi = this;
	voxel_gi.probe = p_base;
	RID rid = voxel_gi_instance_owner.make_rid(voxel_gi);
	return rid;
}

void GI::voxel_gi_instance_free(RID p_rid)
{
	GI::VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_rid);
	voxel_gi->free_resources();
	voxel_gi_instance_owner.free(p_rid);
}

void GI::voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->transform = p_xform;
}

bool GI::voxel_gi_needs_update(RID p_probe) const
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(voxel_gi, false);

	return voxel_gi->last_probe_version != voxel_gi_get_version(voxel_gi->probe);
}

void GI::voxel_gi_update(RID p_probe, bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->update(p_update_light_instances, p_light_instances, p_dynamic_objects);
}

void GI::debug_voxel_gi(RID p_voxel_gi, RD::DrawListID p_draw_list, RID p_framebuffer,
	const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->debug(
		p_draw_list, p_framebuffer, p_camera_with_transform, p_lighting, p_emission, p_alpha);
}

void GI::enable_vrs_shader_group() { shader.enable_group(GROUP_VRS); }


