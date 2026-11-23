/**************************************************************************/
/*  raycast_occlusion_cull.cpp                                            */
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
// --- Inlined from modules/raycast/raycast_occlusion_cull.h ---
#include "servers/rendering/renderer_scene_cull.h"

struct Projection;

class RaycastOcclusionCull final
{
public:
	using HZBuffer = RendererSceneOcclusionCull::HZBuffer;
	struct Data;

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

private:
	static void _init_embree();
	static Vector2 _get_jitter(const Rect2& p_viewport_rect, const Size2i& p_buffer_size);

public:
	static bool is_occluder(RID p_rid);
	static RID occluder_allocate();
	static void occluder_initialize(RID p_occluder);
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);
	static void free_occluder(RID p_occluder);

	static void add_scenario(RID p_scenario);
	static void remove_scenario(RID p_scenario);
	static void scenario_set_instance(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled);
	static void scenario_remove_instance(RID p_scenario, RID p_instance);

	static void add_buffer(RID p_buffer);
	static void remove_buffer(RID p_buffer);
	static HZBuffer* buffer_get_ptr(RID p_buffer);
	static void buffer_set_scenario(RID p_buffer, RID p_scenario);
	static void buffer_set_size(RID p_buffer, const Vector2i& p_size);
	static void buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal);

	static RID buffer_get_debug_texture(RID p_buffer);

	static void set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality);

	RaycastOcclusionCull() = delete;
	RaycastOcclusionCull(const RaycastOcclusionCull&) = delete;
	RaycastOcclusionCull& operator=(const RaycastOcclusionCull&) = delete;
	~RaycastOcclusionCull() = delete;
};

// --- End modules/raycast/raycast_occlusion_cull.h ---
#include "core/io/image.h"
#include "core/math/geometry_3d.h"
#include "register_types.h"
#include "scene/3d/visual_instance_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/projection.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"

#include <embree4/rtcore.h>

#ifdef __SSE2__
#include <pmmintrin.h>
#endif

using CameraRayTile = RTCRayHit16;

struct RendererSceneOcclusionCull::HZBuffer::CameraRayThreadData
{
	int thread_count;
	float z_near;
	float z_far;
	Vector3 camera_dir;
	Vector3 camera_pos;
	Vector3 pixel_corner;
	Vector3 pixel_u_interp;
	Vector3 pixel_v_interp;
	bool camera_orthogonal;
	Size2i buffer_size;
};

namespace
{
struct InstanceID
{
	RID scenario;
	RID instance;

	static uint32_t hash(const InstanceID& p_ins)
	{
		uint32_t h = hash_murmur3_one_64(p_ins.scenario.get_id());
		return hash_fmix32(hash_murmur3_one_64(p_ins.instance.get_id(), h));
	}
	bool operator==(const InstanceID& rhs) const
	{
		return instance == rhs.instance && rhs.scenario == scenario;
	}

	InstanceID() {}
	InstanceID(RID s, RID i) : scenario(s), instance(i) {}
};

struct Occluder
{
	PackedVector3Array vertices;
	PackedInt32Array indices;
	HashSet<InstanceID, InstanceID> users;
};

struct OccluderInstance
{
	RID occluder;
	LocalVector<uint32_t> indices;
	LocalVector<float> xformed_vertices;
	Transform3D xform;
	bool enabled = true;
	bool removed = false;
};

struct Scenario
{
	struct RaycastThreadData
	{
		CameraRayTile* rays = nullptr;
		const uint32_t* masks;
	};

	struct TransformThreadData
	{
		uint32_t thread_count;
		uint32_t vertex_count;
		Transform3D xform;
		const Vector3* read;
		float* write = nullptr;
	};

	Thread* commit_thread = nullptr;
	bool commit_done = true;
	bool dirty = false;

	RTCScene ebr_scene[2] = {nullptr, nullptr};
	int current_scene_idx = 0;

	HashMap<RID, OccluderInstance> instances;
	HashSet<RID> dirty_instances;
	LocalVector<RID> dirty_instances_array;
	LocalVector<RID> removed_instances;

	void _update_dirty_instance(int p_idx, RID* p_instances) {}

	void _update_dirty_instance_thread(int p_idx, RID* p_instances)
	{
		_update_dirty_instance(p_idx, p_instances);
	}

	void _transform_vertices_range(
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

	void _transform_vertices_thread(uint32_t p_thread, TransformThreadData* p_data)
	{
		uint32_t vertex_total = p_data->vertex_count;
		uint32_t total_threads = p_data->thread_count;
		uint32_t from = p_thread * vertex_total / total_threads;
		uint32_t to = (p_thread + 1 == total_threads)
			? vertex_total
			: ((p_thread + 1) * vertex_total / total_threads);
		_transform_vertices_range(p_data->read, p_data->write, p_data->xform, from, to);
	}

	static void _commit_scene(void* p_ud)
	{
		Scenario* scenario = (Scenario*)p_ud;
		int commit_idx = 1 - (scenario->current_scene_idx);
		rtcCommitScene(scenario->ebr_scene[commit_idx]);
		scenario->commit_done = true;
	}

	void free()
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

	void update() {}

	void _raycast(uint32_t p_idx, const RaycastThreadData* p_raycast_data) const
	{
		RTCRayQueryContext context;
		rtcInitRayQueryContext(&context);
		RTCIntersectArguments args;
		rtcInitIntersectArguments(&args);
		args.flags = RTC_RAY_QUERY_FLAG_COHERENT;
		args.context = &context;
		rtcIntersect16((const int*)&p_raycast_data->masks[p_idx * RendererSceneOcclusionCull::HZBuffer::TILE_RAYS],
			ebr_scene[current_scene_idx], &p_raycast_data->rays[p_idx], &args);
	}

	void raycast(CameraRayTile*, unsigned int const*, unsigned int) const {}
};
} // namespace

struct RaycastOcclusionCull::Data
{
	RTCDevice ebr_device = nullptr;
	RID_PtrOwner<Occluder> occluder_owner;
	HashMap<RID, Scenario> scenarios;
	HashMap<RID, HZBuffer> buffers;
	RSE::ViewportOcclusionCullingBuildQuality build_quality =
		RSE::VIEWPORT_OCCLUSION_BUILD_QUALITY_MEDIUM;
	bool _jitter_enabled = false;
};

void RendererSceneOcclusionCull::HZBuffer::clear()
{
	_clear_mips();

	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
		camera_rays_unaligned_buffer = nullptr;
		camera_rays = nullptr;
	}
	camera_ray_masks.clear();
	camera_rays_tile_count = 0;
	tile_grid_size = Size2i();
}

void RendererSceneOcclusionCull::HZBuffer::resize(const Size2i& p_size)
{
	if (p_size == Size2i()) {
		clear();
		return;
	}

	if (!sizes.is_empty() && p_size == sizes[0]) {
		return; // Size didn't change
	}

	_resize_mips(p_size);

	tile_grid_size =
		Size2i(Math::ceil(p_size.x / (float)TILE_SIZE), Math::ceil(p_size.y / (float)TILE_SIZE));
	camera_rays_tile_count = tile_grid_size.x * tile_grid_size.y;

	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
	}

	const int alignment = 64; // Embree requires ray packets to be 64-aligned
	camera_rays_unaligned_buffer =
		(uint8_t*)memalloc(camera_rays_tile_count * sizeof(CameraRayTile) + alignment);
	camera_rays = (void*)(camera_rays_unaligned_buffer + alignment -
						  (((uint64_t)camera_rays_unaligned_buffer) % alignment));

	camera_ray_masks.resize(camera_rays_tile_count * TILE_RAYS);
	memset(camera_ray_masks.ptr(), ~0, camera_rays_tile_count * TILE_RAYS * sizeof(uint32_t));
}

void RendererSceneOcclusionCull::HZBuffer::_camera_rays_threaded(
	uint32_t p_thread, const CameraRayThreadData* p_data)
{
	uint32_t total_tiles = camera_rays_tile_count;
	uint32_t total_threads = p_data->thread_count;
	uint32_t from = p_thread * total_tiles / total_threads;
	uint32_t to = (p_thread + 1 == total_threads) ? total_tiles
												  : ((p_thread + 1) * total_tiles / total_threads);
	_generate_camera_rays(p_data, from, to);
}

void RendererSceneOcclusionCull::HZBuffer::_generate_camera_rays(
	const CameraRayThreadData* p_data, int p_from, int p_to)
{
	const Size2i& buffer_size = sizes[0];
	CameraRayTile* tiles = static_cast<CameraRayTile*>(camera_rays);

	for (int i = p_from; i < p_to; i++) {
		CameraRayTile& tile = tiles[i];
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

void RendererSceneOcclusionCull::HZBuffer::sort_rays(const Vector3& p_camera_dir, bool p_orthogonal)
{
	ERR_FAIL_COND(is_empty());

	const CameraRayTile* tiles = static_cast<const CameraRayTile*>(camera_rays);
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

					mips[0][y * buffer_size.x + x] = tiles[tile_index].ray.tfar[k];
				}
			}
		}
	}
}

void RendererSceneOcclusionCull::HZBuffer::update_camera_rays(
	const Transform3D&, const Vector3&, const Vector2&, real_t, bool)
{
}

RendererSceneOcclusionCull::HZBuffer::~HZBuffer()
{
	if (camera_rays_unaligned_buffer) {
		memfree(camera_rays_unaligned_buffer);
	}
}

bool RaycastOcclusionCull::is_occluder(RID p_rid)
{
	ERR_FAIL_NULL_V(data, false);
	return data->occluder_owner.owns(p_rid);
}

RID RaycastOcclusionCull::occluder_allocate()
{
	ERR_FAIL_NULL_V(data, RID());
	return data->occluder_owner.allocate_rid();
}

void RaycastOcclusionCull::occluder_initialize(RID p_occluder)
{
	ERR_FAIL_NULL(data);
	Occluder* occluder = memnew(Occluder);
	data->occluder_owner.initialize_rid(p_occluder, occluder);
}

void RaycastOcclusionCull::occluder_set_mesh(
	RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
{
	ERR_FAIL_NULL(data);
	Occluder* occluder = data->occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);

	occluder->vertices = p_vertices;
	occluder->indices = p_indices;

	for (const InstanceID& E : occluder->users) {
		RID scenario_rid = E.scenario;
		RID instance_rid = E.instance;
		ERR_CONTINUE(!data->scenarios.has(scenario_rid));
		Scenario& scenario = data->scenarios[scenario_rid];
		ERR_CONTINUE(!scenario.instances.has(instance_rid));

		if (!scenario.dirty_instances.has(instance_rid)) {
			scenario.dirty_instances.insert(instance_rid);
			scenario.dirty_instances_array.push_back(instance_rid);
		}
	}
}

void RaycastOcclusionCull::free_occluder(RID p_occluder)
{
	ERR_FAIL_NULL(data);
	Occluder* occluder = data->occluder_owner.get_or_null(p_occluder);
	ERR_FAIL_NULL(occluder);
	memdelete(occluder);
	data->occluder_owner.free(p_occluder);
}

void RaycastOcclusionCull::add_scenario(RID p_scenario)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(data->scenarios.has(p_scenario));
	data->scenarios[p_scenario] = Scenario();
}

void RaycastOcclusionCull::remove_scenario(RID p_scenario)
{
	ERR_FAIL_NULL(data);
	Scenario* scenario = data->scenarios.getptr(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->free();
	data->scenarios.erase(p_scenario);
}

void RaycastOcclusionCull::scenario_set_instance(
	RID p_scenario, RID p_instance, RID p_occluder, const Transform3D& p_xform, bool p_enabled)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->scenarios.has(p_scenario));
	Scenario& scenario = data->scenarios[p_scenario];

	if (!scenario.instances.has(p_instance)) {
		scenario.instances[p_instance] = OccluderInstance();
	}

	OccluderInstance& instance = scenario.instances[p_instance];

	bool changed = false;

	if (instance.removed) {
		instance.removed = false;
		scenario.removed_instances.erase(p_instance);
		changed = true;
	}

	if (instance.occluder != p_occluder) {
		Occluder* old_occluder = data->occluder_owner.get_or_null(instance.occluder);
		if (old_occluder) {
			old_occluder->users.erase(InstanceID(p_scenario, p_instance));
		}

		instance.occluder = p_occluder;

		if (p_occluder.is_valid()) {
			Occluder* occluder = data->occluder_owner.get_or_null(p_occluder);
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
		scenario.dirty = true;
	}

	if (changed && !scenario.dirty_instances.has(p_instance)) {
		scenario.dirty_instances.insert(p_instance);
		scenario.dirty_instances_array.push_back(p_instance);
		scenario.dirty = true;
	}
}

void RaycastOcclusionCull::scenario_remove_instance(RID p_scenario, RID p_instance)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->scenarios.has(p_scenario));
	Scenario& scenario = data->scenarios[p_scenario];

	if (scenario.instances.has(p_instance)) {
		OccluderInstance& instance = scenario.instances[p_instance];

		if (!instance.removed) {
			Occluder* occluder = data->occluder_owner.get_or_null(instance.occluder);
			if (occluder) {
				occluder->users.erase(InstanceID(p_scenario, p_instance));
			}

			scenario.removed_instances.push_back(p_instance);
			instance.removed = true;
		}
	}
}

void RaycastOcclusionCull::add_buffer(RID p_buffer)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(data->buffers.has(p_buffer));
	data->buffers[p_buffer];
}

void RaycastOcclusionCull::remove_buffer(RID p_buffer)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->buffers.has(p_buffer));
	data->buffers.erase(p_buffer);
}

void RaycastOcclusionCull::buffer_set_scenario(RID p_buffer, RID p_scenario)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->buffers.has(p_buffer));
	ERR_FAIL_COND(p_scenario.is_valid() && !data->scenarios.has(p_scenario));
	data->buffers[p_buffer].scenario_rid = p_scenario;
}

void RaycastOcclusionCull::buffer_set_size(RID p_buffer, const Vector2i& p_size)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(!data->buffers.has(p_buffer));
	data->buffers[p_buffer].resize(p_size);
}

Vector2 RaycastOcclusionCull::_get_jitter(const Rect2& p_viewport_rect, const Size2i& p_buffer_size)
{
	if (!data || !data->_jitter_enabled) {
		return Vector2();
	}

	if ((p_buffer_size.x <= 0) || (p_buffer_size.y <= 0)) {
		return Vector2();
	}

	int32_t frame = Engine::get_frames_drawn();
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
	jitter *= 0.66f;

	return jitter;
}

static Rect2 _get_viewport_rect(const Projection& p_cam_projection)
{
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
	ERR_FAIL_NULL(data);
	if (!data->buffers.has(p_buffer)) {
		return;
	}

	HZBuffer& buffer = data->buffers[p_buffer];

	if (buffer.is_empty() || !data->scenarios.has(buffer.scenario_rid)) {
		return;
	}

	Scenario& scenario = data->scenarios[buffer.scenario_rid];
	scenario.update();

	Rect2 vp_rect = _get_viewport_rect(p_cam_projection);
	Vector2 bottom_left = vp_rect.position;
	bottom_left += _get_jitter(vp_rect, buffer.get_occlusion_buffer_size());
	Vector3 near_bottom_left =
		Vector3(bottom_left.x, bottom_left.y, -p_cam_projection.get_z_near());

	buffer.update_camera_rays(p_cam_transform, near_bottom_left, vp_rect.get_size(),
		p_cam_projection.get_z_far(), p_cam_orthogonal);

	scenario.raycast(static_cast<CameraRayTile*>(buffer.camera_rays),
		buffer.camera_ray_masks.ptr(), buffer.camera_rays_tile_count);
	buffer.sort_rays(-p_cam_transform.basis.get_column(2), p_cam_orthogonal);
	buffer.update_mips();
}

RaycastOcclusionCull::HZBuffer* RaycastOcclusionCull::buffer_get_ptr(RID p_buffer)
{
	ERR_FAIL_NULL_V(data, nullptr);
	if (!data->buffers.has(p_buffer)) {
		return nullptr;
	}
	return &data->buffers[p_buffer];
}

RID RaycastOcclusionCull::buffer_get_debug_texture(RID p_buffer)
{
	ERR_FAIL_NULL_V(data, RID());
	ERR_FAIL_COND_V(!data->buffers.has(p_buffer), RID());
	return data->buffers[p_buffer].get_debug_texture();
}

void RaycastOcclusionCull::set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality)
{
	ERR_FAIL_NULL(data);
	if (data->build_quality == p_quality) {
		return;
	}

	data->build_quality = p_quality;

	for (KeyValue<RID, Scenario>& K : data->scenarios) {
		K.value.dirty = true;
	}
}

void RaycastOcclusionCull::_init_embree()
{
	ERR_FAIL_NULL(data);
#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	String settings = vformat("threads=%d", MAX(1, OS::get_processor_count() - 2));
	data->ebr_device = rtcNewDevice(settings.utf8().ptr());
}

void RaycastOcclusionCull::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
	RendererSceneOcclusionCull::bind<RaycastOcclusionCull>();
}

void RaycastOcclusionCull::finalize()
{
	if (!data) {
		return;
	}

	RendererSceneOcclusionCull::reset();

	for (KeyValue<RID, Scenario>& K : data->scenarios) {
		K.value.free();
	}
	data->scenarios.clear();
	data->buffers.clear();

	if (data->ebr_device != nullptr) {
		rtcReleaseDevice(data->ebr_device);
		data->ebr_device = nullptr;
	}

	memdelete(data);
	data = nullptr;
}

#ifdef TOOLS_ENABLED




#ifdef __SSE2__
#endif

static float blerp(float c00, float c10, float c01, float c11, float tx, float ty)
{
	return Math::lerp(Math::lerp(c00, c10, tx), Math::lerp(c01, c11, tx), ty);
}

struct LightmapRaycaster::EmbreeState
{
	struct AlphaTextureData
	{
		Vector<uint8_t> data;
		Vector2i size;

		uint8_t sample(float u, float v) const
		{
			float x = u * size.x;
			float y = v * size.y;
			int xi = (int)x;
			int yi = (int)y;

			uint8_t texels[4];

			for (int i = 0; i < 4; ++i) {
				int sample_x = CLAMP(xi + i % 2, 0, size.x - 1);
				int sample_y = CLAMP(yi + i / 2, 0, size.y - 1);
				texels[i] = data[sample_y * size.x + sample_x];
			}

			return Math::round(blerp(texels[0], texels[1], texels[2], texels[3], x - xi, y - yi));
		}
	};

	RTCDevice embree_device = nullptr;
	RTCScene embree_scene = nullptr;

	HashMap<unsigned int, AlphaTextureData> alpha_textures;
	HashSet<int> filter_meshes;

	static void filter_function(const struct RTCFilterFunctionNArguments* p_args)
	{
		RTCHit* hit = (RTCHit*)p_args->hit;

		unsigned int geomID = hit->geomID;
		float u = hit->u;
		float v = hit->v;

		EmbreeState* scene = (EmbreeState*)p_args->geometryUserPtr;
		RTCGeometry geom = rtcGetGeometry(scene->embree_scene, geomID);

		rtcInterpolate0(
			geom, hit->primID, hit->u, hit->v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &hit->u, 2);

		if (scene->alpha_textures.has(geomID)) {
			const AlphaTextureData& alpha_texture = scene->alpha_textures[geomID];

			if (alpha_texture.sample(hit->u, hit->v) < 128) {
				p_args->valid[0] = 0;
				return;
			}
		}

		rtcInterpolate0(
			geom, hit->primID, u, v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &hit->Ng_x, 3);
	}
};

Ref<LightmapRaycaster> LightmapRaycaster::create()
{
	return Ref<LightmapRaycaster>(memnew(LightmapRaycaster));
}

bool LightmapRaycaster::intersect(Ray& r_ray)
{
	RTCRayQueryContext context;
	rtcInitRayQueryContext(&context);
	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.context = &context;
	rtcIntersect1(embree_state->embree_scene, (RTCRayHit*)&r_ray, &args);
	return r_ray.geomID != RTC_INVALID_GEOMETRY_ID;
}

void LightmapRaycaster::intersect(Vector<Ray>& r_rays)
{
	Ray* rays = r_rays.ptrw();
	for (int i = 0; i < r_rays.size(); ++i) {
		intersect(rays[i]);
	}
}

void LightmapRaycaster::set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id)
{
	if (p_alpha_texture.is_valid() && p_alpha_texture->get_size() != Vector2i()) {
		EmbreeState::AlphaTextureData tex;
		tex.size = p_alpha_texture->get_size();
		tex.data = p_alpha_texture->get_data();
		embree_state->alpha_textures.insert(p_id, tex);
	}
}

void LightmapRaycaster::add_mesh(const Vector<Vector3>& p_vertices,
	const Vector<Vector3>& p_normals, const Vector<Vector2>& p_uv2s, unsigned int p_id)
{
	RTCGeometry embree_mesh =
		rtcNewGeometry(embree_state->embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	rtcSetGeometryVertexAttributeCount(embree_mesh, 2);

	int vertex_count = p_vertices.size();

	ERR_FAIL_COND(vertex_count % 3 != 0);
	ERR_FAIL_COND(vertex_count != p_uv2s.size());
	ERR_FAIL_COND(!p_normals.is_empty() && vertex_count != p_normals.size());

	Vector3* embree_vertices = (Vector3*)rtcSetNewGeometryBuffer(
		embree_mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3), vertex_count);
	memcpy(embree_vertices, p_vertices.ptr(), sizeof(Vector3) * vertex_count);

	Vector2* embree_light_uvs =
		(Vector2*)rtcSetNewGeometryBuffer(embree_mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
			RTC_FORMAT_FLOAT2, sizeof(Vector2), vertex_count);
	memcpy(embree_light_uvs, p_uv2s.ptr(), sizeof(Vector2) * vertex_count);

	uint32_t* embree_triangles = (uint32_t*)rtcSetNewGeometryBuffer(embree_mesh,
		RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, vertex_count / 3);
	for (int i = 0; i < vertex_count; i++) {
		embree_triangles[i] = i;
	}

	if (!p_normals.is_empty()) {
		Vector3* embree_normals =
			(Vector3*)rtcSetNewGeometryBuffer(embree_mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
				RTC_FORMAT_FLOAT3, sizeof(Vector3), vertex_count);
		memcpy(embree_normals, p_normals.ptr(), sizeof(Vector3) * vertex_count);
	}

	rtcCommitGeometry(embree_mesh);
	rtcSetGeometryIntersectFilterFunction(embree_mesh, EmbreeState::filter_function);
	rtcSetGeometryUserData(embree_mesh, embree_state);
	rtcAttachGeometryByID(embree_state->embree_scene, embree_mesh, p_id);
	rtcReleaseGeometry(embree_mesh);
}

void LightmapRaycaster::commit() { rtcCommitScene(embree_state->embree_scene); }

void LightmapRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids)
{
	for (const int& E : p_mesh_ids) {
		rtcDisableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes = p_mesh_ids;
}

void LightmapRaycaster::clear_mesh_filter()
{
	for (const int& E : embree_state->filter_meshes) {
		rtcEnableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes.clear();
}

static void embree_lm_error_handler(void* p_user_data, RTCError p_code, const char* p_str)
{
	print_error("Embree error: " + String(p_str));
}

LightmapRaycaster::LightmapRaycaster()
{
	embree_state = memnew(EmbreeState);

#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	embree_state->embree_device = rtcNewDevice(nullptr);
	rtcSetDeviceErrorFunction(embree_state->embree_device, &embree_lm_error_handler, nullptr);
	embree_state->embree_scene = rtcNewScene(embree_state->embree_device);
}

LightmapRaycaster::~LightmapRaycaster()
{
	if (embree_state->embree_scene != nullptr) {
		rtcReleaseScene(embree_state->embree_scene);
	}

	if (embree_state->embree_device != nullptr) {
		rtcReleaseDevice(embree_state->embree_device);
	}

	memdelete(embree_state);
	embree_state = nullptr;
}

#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED


#ifdef __SSE2__
#endif

static RTCDevice static_embree_device = nullptr;

struct StaticRaycaster::EmbreeState
{
	RTCScene embree_scene = nullptr;
	HashSet<int> filter_meshes;
};

Ref<StaticRaycaster> StaticRaycaster::create()
{
	return Ref<StaticRaycaster>(memnew(StaticRaycaster));
}

void StaticRaycaster::free()
{
	if (static_embree_device) {
		rtcReleaseDevice(static_embree_device);
		static_embree_device = nullptr;
	}
}

bool StaticRaycaster::intersect(Ray& r_ray)
{
	RTCRayQueryContext context;
	rtcInitRayQueryContext(&context);
	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	args.context = &context;
	rtcIntersect1(embree_state->embree_scene, (RTCRayHit*)&r_ray, &args);
	return r_ray.geomID != RTC_INVALID_GEOMETRY_ID;
}

void StaticRaycaster::intersect(Vector<Ray>& r_rays)
{
	Ray* rays = r_rays.ptrw();
	for (int i = 0; i < r_rays.size(); ++i) {
		intersect(rays[i]);
	}
}

void StaticRaycaster::add_mesh(
	const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices, unsigned int p_id)
{
	RTCGeometry embree_mesh = rtcNewGeometry(static_embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	int vertex_count = p_vertices.size();

	Vector3* embree_vertices = (Vector3*)rtcSetNewGeometryBuffer(
		embree_mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vector3), vertex_count);
	memcpy(embree_vertices, p_vertices.ptr(), sizeof(Vector3) * vertex_count);

	if (p_indices.is_empty()) {
		ERR_FAIL_COND(vertex_count % 3 != 0);
		uint32_t* embree_triangles = (uint32_t*)rtcSetNewGeometryBuffer(embree_mesh,
			RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, vertex_count / 3);
		for (int i = 0; i < vertex_count; i++) {
			embree_triangles[i] = i;
		}
	}
	else {
		uint32_t* embree_triangles =
			(uint32_t*)rtcSetNewGeometryBuffer(embree_mesh, RTC_BUFFER_TYPE_INDEX, 0,
				RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, p_indices.size() / 3);
		memcpy(embree_triangles, p_indices.ptr(), sizeof(uint32_t) * p_indices.size());
	}

	rtcCommitGeometry(embree_mesh);
	rtcAttachGeometryByID(embree_state->embree_scene, embree_mesh, p_id);
	rtcReleaseGeometry(embree_mesh);
}

void StaticRaycaster::commit() { rtcCommitScene(embree_state->embree_scene); }

void StaticRaycaster::set_mesh_filter(const HashSet<int>& p_mesh_ids)
{
	for (const int& E : p_mesh_ids) {
		rtcDisableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes = p_mesh_ids;
}

void StaticRaycaster::clear_mesh_filter()
{
	for (const int& E : embree_state->filter_meshes) {
		rtcEnableGeometry(rtcGetGeometry(embree_state->embree_scene, E));
	}
	rtcCommitScene(embree_state->embree_scene);
	embree_state->filter_meshes.clear();
}

static void embree_error_handler(void* p_user_data, RTCError p_code, const char* p_str)
{
	print_error("Embree error: " + String(p_str));
}

StaticRaycaster::StaticRaycaster()
{
	embree_state = memnew(EmbreeState);

#ifdef __SSE2__
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

	if (!static_embree_device) {
		static_embree_device = rtcNewDevice(nullptr);
		rtcSetDeviceErrorFunction(static_embree_device, &embree_error_handler, nullptr);
	}

	embree_state->embree_scene = rtcNewScene(static_embree_device);
}

StaticRaycaster::~StaticRaycaster()
{
	if (embree_state->embree_scene != nullptr) {
		rtcReleaseScene(embree_state->embree_scene);
	}
	memdelete(embree_state);
	embree_state = nullptr;
}

#endif // TOOLS_ENABLED

void initialize_raycast_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	RaycastOcclusionCull::initialize();
}

void uninitialize_raycast_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	RaycastOcclusionCull::finalize();
#ifdef TOOLS_ENABLED
	StaticRaycaster::free();
#endif
}
