/**************************************************************************/
/*  raycast_occlusion_cull.cpp                                            */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/projection.h"
#include "core/os/os.h"
#include "raycast_occlusion_cull.h"

#ifdef __SSE2__
#include <pmmintrin.h>
#endif

RaycastOcclusionCull* RaycastOcclusionCull::raycast_singleton = nullptr;

void RaycastOcclusionCull::RaycastHZBuffer::clear()
{
	HZBuffer::clear();

	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
		camera_rays_unaligned_buffer = nullptr;
		camera_rays = nullptr;
	}
	camera_ray_masks.clear();
	camera_rays_tile_count = 0;
	tile_grid_size = Size2i();
}

void RaycastOcclusionCull::RaycastHZBuffer::resize(const Size2i& p_size)
{
	if (p_size == Size2i()) {
		clear();
		return;
	}

	if (!sizes.is_empty() && p_size == sizes[0]) {
		return; // Size didn't change
	}

	HZBuffer::resize(p_size);

	tile_grid_size =
		Size2i(Math::ceil(p_size.x / (float)TILE_SIZE), Math::ceil(p_size.y / (float)TILE_SIZE));
	camera_rays_tile_count = tile_grid_size.x * tile_grid_size.y;

	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
	}

	const int alignment = 64; // Embree requires ray packets to be 64-aligned
	camera_rays_unaligned_buffer =
		(uint8_t*)memalloc(camera_rays_tile_count * sizeof(CameraRayTile) + alignment);
	camera_rays = (CameraRayTile*)(camera_rays_unaligned_buffer + alignment -
								   (((uint64_t)camera_rays_unaligned_buffer) % alignment));

	camera_ray_masks.resize(camera_rays_tile_count * TILE_RAYS);
	memset(camera_ray_masks.ptr(), ~0, camera_rays_tile_count * TILE_RAYS * sizeof(uint32_t));
}

void RaycastOcclusionCull::RaycastHZBuffer::_camera_rays_threaded(
	uint32_t p_thread, const CameraRayThreadData* p_data)
{
	uint32_t total_tiles = camera_rays_tile_count;
	uint32_t total_threads = p_data->thread_count;
	uint32_t from = p_thread * total_tiles / total_threads;
	uint32_t to = (p_thread + 1 == total_threads) ? total_tiles
												  : ((p_thread + 1) * total_tiles / total_threads);
	_generate_camera_rays(p_data, from, to);
}

void RaycastOcclusionCull::RaycastHZBuffer::_generate_camera_rays(
	const CameraRayThreadData* p_data, int p_from, int p_to)
{
	const Size2i& buffer_size = sizes[0];

	for (int i = p_from; i < p_to; i++) {
		CameraRayTile& tile = camera_rays[i];
		int tile_x = (i % tile_grid_size.x) * TILE_SIZE;
		int tile_y = (i / tile_grid_size.x) * TILE_SIZE;

		for (int j = 0; j < TILE_RAYS; j++) {
			int x = tile_x + j % TILE_SIZE;
			int y = tile_y + j / TILE_SIZE;

			float u = (float(x) + 0.5f) / buffer_size.x;
			float v = (float(y) + 0.5f) / buffer_size.y;
			Vector3 pixel_pos =
				p_data->pixel_corner + u * p_data->pixel_u_interp + v * p_data->pixel_v_interp;

			tile.ray.tnear[j] = p_data->z_near;

			Vector3 dir;
			if (p_data->camera_orthogonal) {
				dir = p_data->camera_dir;
				tile.ray.org_x[j] = pixel_pos.x - dir.x * p_data->z_near;
				tile.ray.org_y[j] = pixel_pos.y - dir.y * p_data->z_near;
				tile.ray.org_z[j] = pixel_pos.z - dir.z * p_data->z_near;
			}
			else {
				dir = (pixel_pos - p_data->camera_pos).normalized();
				tile.ray.org_x[j] = p_data->camera_pos.x;
				tile.ray.org_y[j] = p_data->camera_pos.y;
				tile.ray.org_z[j] = p_data->camera_pos.z;
				tile.ray.tnear[j] /= dir.dot(p_data->camera_dir);
			}

			tile.ray.dir_x[j] = dir.x;
			tile.ray.dir_y[j] = dir.y;
			tile.ray.dir_z[j] = dir.z;

			tile.ray.tfar[j] = p_data->z_far;
			tile.ray.time[j] = 0.0f;

			tile.ray.flags[j] = 0;
			tile.ray.mask[j] = ~0U;
			tile.hit.geomID[j] = RTC_INVALID_GEOMETRY_ID;
		}
	}
}

void RaycastOcclusionCull::RaycastHZBuffer::sort_rays(
	const Vector3& p_camera_dir, bool p_orthogonal)
{
	ERR_FAIL_COND(is_empty());

	Size2i buffer_size = sizes[0];
	for (int i = 0; i < tile_grid_size.y; i++) {
		for (int j = 0; j < tile_grid_size.x; j++) {
			for (int tile_i = 0; tile_i < TILE_SIZE; tile_i++) {
				for (int tile_j = 0; tile_j < TILE_SIZE; tile_j++) {
					int x = j * TILE_SIZE + tile_j;
					int y = i * TILE_SIZE + tile_i;
					if (x >= buffer_size.x || y >= buffer_size.y) {
						continue;
					}
					int k = tile_i * TILE_SIZE + tile_j;
					int tile_index = i * tile_grid_size.x + j;

					mips[0][y * buffer_size.x + x] = camera_rays[tile_index].ray.tfar[k];
				}
			}
		}
	}
}

RaycastOcclusionCull::RaycastHZBuffer::~RaycastHZBuffer()
{
	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
	}
}

bool RaycastOcclusionCull::is_occluder(RID p_rid) { return occluder_owner.owns(p_rid); }

RID RaycastOcclusionCull::occluder_allocate() { return occluder_owner.allocate_rid(); }

void RaycastOcclusionCull::occluder_initialize(RID p_occluder)
{
	Occluder* occluder = memnew(Occluder);
	occluder_owner.initialize_rid(p_occluder, occluder);
}

void RaycastOcclusionCull::occluder_set_mesh(
	RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
{
	Occluder* occluder = occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	occluder->vertices = p_vertices;
	occluder->indices = p_indices;

	for (const InstanceID& E : occluder->users) {
		RID scenario_rid = E.scenario;
		RID instance_rid = E.instance;
		ERR_CONTINUE(!scenarios.has(scenario_rid));
		Scenario& scenario = scenarios[scenario_rid];
		ERR_CONTINUE(!scenario.instances.has(instance_rid));

		if (!scenario.dirty_instances.has(instance_rid)) {
			scenario.dirty_instances.insert(instance_rid);
			scenario.dirty_instances_array.push_back(instance_rid);
		}
	}
}

void RaycastOcclusionCull::free_occluder(RID p_occluder)
{
	Occluder* occluder = occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);
	memdelete(occluder);
	occluder_owner.free(p_occluder);
}

void RaycastOcclusionCull::add_scenario(RID p_scenario)
{
	ERR_FAIL_COND(scenarios.has(p_scenario));
	scenarios[p_scenario] = Scenario();
}

void RaycastOcclusionCull::remove_scenario(RID p_scenario)
{
	Scenario* scenario = scenarios.getptr(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->free();
	scenarios.erase(p_scenario);
}

void RaycastOcclusionCull::scenario_set_instance(
	RID p_scenario, RID p_instance, RID p_occluder, const Transform3D& p_xform, bool p_enabled)
{
	ERR_FAIL_COND(!scenarios.has(p_scenario));
	Scenario& scenario = scenarios[p_scenario];

	if (!scenario.instances.has(p_instance)) {
		scenario.instances[p_instance] = OccluderInstance();
	}

	OccluderInstance& instance = scenario.instances[p_instance];

	bool changed = false;

	if (instance.removed) {
		instance.removed = false;
		scenario.removed_instances.erase(p_instance);
		changed = true; // It was removed and re-added, we might have missed some changes
	}

	if (instance.occluder != p_occluder) {
		Occluder* old_occluder = occluder_owner.get_or_null(instance.occluder);
		if (old_occluder) {
			old_occluder->users.erase(InstanceID(p_scenario, p_instance));
		}

		instance.occluder = p_occluder;

		if (p_occluder.is_valid()) {
			Occluder* occluder = occluder_owner.get_or_null(p_occluder);
			ERR_FAIL_NULL(occluder);
			occluder->users.insert(InstanceID(p_scenario, p_instance));
		}
		changed = true;
	}

	if (instance.xform != p_xform) {
		scenario.instances[p_instance].xform = p_xform;
		changed = true;
	}

	if (instance.enabled != p_enabled) {
		instance.enabled = p_enabled;
		scenario.dirty =
			true; // The scenario needs a scene re-build, but the instance doesn't need update
	}

	if (changed && !scenario.dirty_instances.has(p_instance)) {
		scenario.dirty_instances.insert(p_instance);
		scenario.dirty_instances_array.push_back(p_instance);
		scenario.dirty = true;
	}
}

void RaycastOcclusionCull::scenario_remove_instance(RID p_scenario, RID p_instance)
{
	ERR_FAIL_COND(!scenarios.has(p_scenario));
	Scenario& scenario = scenarios[p_scenario];

	if (scenario.instances.has(p_instance)) {
		OccluderInstance& instance = scenario.instances[p_instance];

		if (!instance.removed) {
			Occluder* occluder = occluder_owner.get_or_null(instance.occluder);
			if (occluder) {
				occluder->users.erase(InstanceID(p_scenario, p_instance));
			}

			scenario.removed_instances.push_back(p_instance);
			instance.removed = true;
		}
	}
}

void RaycastOcclusionCull::Scenario::_update_dirty_instance_thread(int p_idx, RID* p_instances)
{
	_update_dirty_instance(p_idx, p_instances);
}

void RaycastOcclusionCull::Scenario::_transform_vertices_thread(
	uint32_t p_thread, TransformThreadData* p_data)
{
	uint32_t vertex_total = p_data->vertex_count;
	uint32_t total_threads = p_data->thread_count;
	uint32_t from = p_thread * vertex_total / total_threads;
	uint32_t to = (p_thread + 1 == total_threads) ? vertex_total
												  : ((p_thread + 1) * vertex_total / total_threads);
	_transform_vertices_range(p_data->read, p_data->write, p_data->xform, from, to);
}

void RaycastOcclusionCull::Scenario::_transform_vertices_range(
	const Vector3* p_read, float* p_write, const Transform3D& p_xform, int p_from, int p_to)
{
	float* floats_w = p_write + 3 * p_from;
	for (int i = p_from; i < p_to; i++) {
		const Vector3 p = p_xform.xform(p_read[i]);
		floats_w[0] = p.x;
		floats_w[1] = p.y;
		floats_w[2] = p.z;
		floats_w += 3;
	}
}

void RaycastOcclusionCull::Scenario::free()
{
	if (commit_thread) {
		if (commit_thread->is_started()) {
			commit_thread->wait_to_finish();
		}
		memdelete(commit_thread);
		commit_thread = nullptr;
	}

	for (int i = 0; i < 2; i++) {
		if (ebr_scene[i]) {
			rtcReleaseScene(ebr_scene[i]);
			ebr_scene[i] = nullptr;
		}
	}
}

void RaycastOcclusionCull::Scenario::_commit_scene(void* p_ud)
{
	Scenario* scenario = (Scenario*)p_ud;
	int commit_idx = 1 - (scenario->current_scene_idx);
	rtcCommitScene(scenario->ebr_scene[commit_idx]);
	scenario->commit_done = true;
}

void RaycastOcclusionCull::Scenario::_raycast(
	uint32_t p_idx, const RaycastThreadData* p_raycast_data) const
{
	RTCRayQueryContext context;
	rtcInitRayQueryContext(&context);
	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.flags = RTC_RAY_QUERY_FLAG_COHERENT;
	args.context = &context;
	rtcIntersect16((const int*)&p_raycast_data->masks[p_idx * TILE_RAYS],
		ebr_scene[current_scene_idx], &p_raycast_data->rays[p_idx], &args);
}

void RaycastOcclusionCull::add_buffer(RID p_buffer)
{
	ERR_FAIL_COND(buffers.has(p_buffer));
	buffers[p_buffer] = RaycastHZBuffer();
}

void RaycastOcclusionCull::remove_buffer(RID p_buffer)
{
	ERR_FAIL_COND(!buffers.has(p_buffer));
	buffers.erase(p_buffer);
}

void RaycastOcclusionCull::buffer_set_scenario(RID p_buffer, RID p_scenario)
{
	ERR_FAIL_COND(!buffers.has(p_buffer));
	ERR_FAIL_COND(p_scenario.is_valid() && !scenarios.has(p_scenario));
	buffers[p_buffer].scenario_rid = p_scenario;
}

void RaycastOcclusionCull::buffer_set_size(RID p_buffer, const Vector2i& p_size)
{
	ERR_FAIL_COND(!buffers.has(p_buffer));
	buffers[p_buffer].resize(p_size);
}

Vector2 RaycastOcclusionCull::_get_jitter(const Rect2& p_viewport_rect, const Size2i& p_buffer_size)
{
	if (!_jitter_enabled) {
		return Vector2();
	}

	// Prevent divide by zero when using NULL viewport.
	if ((p_buffer_size.x <= 0) || (p_buffer_size.y <= 0)) {
		return Vector2();
	}

	int32_t frame = Engine::get_singleton()->get_frames_drawn();
	frame %= 9;

	Vector2 jitter;

	switch (frame) {
	default:
		break;
	case 1: {
		jitter = Vector2(-1, -1);
	} break;
	case 2: {
		jitter = Vector2(1, -1);
	} break;
	case 3: {
		jitter = Vector2(-1, 1);
	} break;
	case 4: {
		jitter = Vector2(1, 1);
	} break;
	case 5: {
		jitter = Vector2(-0.5f, -0.5f);
	} break;
	case 6: {
		jitter = Vector2(0.5f, -0.5f);
	} break;
	case 7: {
		jitter = Vector2(-0.5f, 0.5f);
	} break;
	case 8: {
		jitter = Vector2(0.5f, 0.5f);
	} break;
	}
	Vector2 half_extents = p_viewport_rect.get_size() * 0.5;
	jitter *=
		Vector2(half_extents.x / (float)p_buffer_size.x, half_extents.y / (float)p_buffer_size.y);

	// The multiplier here determines the jitter magnitude in pixels.
	// It seems like a value of 0.66 matches well the above jittering pattern as it generates
	// subpixel samples at 0, 1/3 and 2/3 Higher magnitude gives fewer false hidden, but more false
	// shown. False hidden is obvious to viewer, false shown is not. False shown can lower
	// percentage that are occluded, and therefore performance.
	jitter *= 0.66f;

	return jitter;
}

Rect2 _get_viewport_rect(const Projection& p_cam_projection)
{
	// NOTE: This assumes a rectangular projection plane, i.e. that:
	// - the matrix is a projection across z-axis (i.e. is invertible and columns[0][1], [0][3],
	// [1][0] and [1][3] == 0)
	// - the projection plane is rectangular (i.e. columns[0][2] and [1][2] == 0 if columns[2][3] !=
	// 0)
	Size2 half_extents = p_cam_projection.get_viewport_half_extents();
	Point2 bottom_left =
		-half_extents *
		Vector2(p_cam_projection.columns[3][0] * p_cam_projection.columns[3][3] +
					p_cam_projection.columns[2][0] * p_cam_projection.columns[2][3] + 1,
			p_cam_projection.columns[3][1] * p_cam_projection.columns[3][3] +
				p_cam_projection.columns[2][1] * p_cam_projection.columns[2][3] + 1);
	return Rect2(bottom_left, 2 * half_extents);
}

void RaycastOcclusionCull::buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
	const Projection& p_cam_projection, bool p_cam_orthogonal)
{
	if (!buffers.has(p_buffer)) {
		return;
	}

	RaycastHZBuffer& buffer = buffers[p_buffer];

	if (buffer.is_empty() || !scenarios.has(buffer.scenario_rid)) {
		return;
	}

	Scenario& scenario = scenarios[buffer.scenario_rid];
	scenario.update();

	Rect2 vp_rect = _get_viewport_rect(p_cam_projection);
	Vector2 bottom_left = vp_rect.position;
	bottom_left += _get_jitter(vp_rect, buffer.get_occlusion_buffer_size());
	Vector3 near_bottom_left =
		Vector3(bottom_left.x, bottom_left.y, -p_cam_projection.get_z_near());

	buffer.update_camera_rays(p_cam_transform, near_bottom_left, vp_rect.get_size(),
		p_cam_projection.get_z_far(), p_cam_orthogonal);

	scenario.raycast(
		buffer.camera_rays, buffer.camera_ray_masks.ptr(), buffer.camera_rays_tile_count);
	buffer.sort_rays(-p_cam_transform.basis.get_column(2), p_cam_orthogonal);
	buffer.update_mips();
}

RaycastOcclusionCull::HZBuffer* RaycastOcclusionCull::buffer_get_ptr(RID p_buffer)
{
	if (!buffers.has(p_buffer)) {
		return nullptr;
	}
	return &buffers[p_buffer];
}

RID RaycastOcclusionCull::buffer_get_debug_texture(RID p_buffer)
{
	ERR_FAIL_COND_V(!buffers.has(p_buffer), RID());
	return buffers[p_buffer].get_debug_texture();
}

void RaycastOcclusionCull::set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality)
{
	if (build_quality == p_quality) {
		return;
	}

	build_quality = p_quality;

	for (KeyValue<RID, Scenario>& K : scenarios) {
		K.value.dirty = true;
	}
}

void RaycastOcclusionCull::_init_embree()
{
#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	String settings = vformat("threads=%d", MAX(1, OS::get_singleton()->get_processor_count() - 2));
	ebr_device = rtcNewDevice(settings.utf8().ptr());
}

RaycastOcclusionCull::~RaycastOcclusionCull()
{
	for (KeyValue<RID, Scenario>& K : scenarios) {
		K.value.free();
	}

	if (ebr_device != nullptr) {
		rtcReleaseDevice(ebr_device);
	}

	raycast_singleton = nullptr;
}


