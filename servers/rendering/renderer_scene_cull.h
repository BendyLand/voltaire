/**************************************************************************/
/*  renderer_scene_cull.h                                                 */
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

#pragma once

#include "core/math/dynamic_bvh.h"
#include "core/math/transform_interpolator.h"
#include "core/templates/bin_sorted_array.h"
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/paged_array.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "servers/rendering/instance_uniforms.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_occlusion_cull.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"
#include "servers/xr/xr_interface.h"

class RenderingLightCuller;

class RendererSceneCull final
{
public:
	enum
	{
		SDFGI_MAX_CASCADES = 8,
		SDFGI_MAX_REGIONS_PER_CASCADE = 3,
		MAX_INSTANCE_PAIRS = 32,
		MAX_UPDATE_SHADOWS = 512
	};

	/* EVENT QUEUING */

	static void tick();
	static void pre_draw(bool p_will_draw);

	/* CAMERA API */

	struct Camera
	{
		enum Type
		{
			PERSPECTIVE,
			ORTHOGONAL,
			FRUSTUM
		};

		Type type;
		float fov;
		float znear, zfar;
		float size;
		Vector2 offset;
		uint32_t visible_layers;
		bool vaspect;
		RID env;
		RID attributes;
		RID compositor;

		Transform3D transform;

		Camera()
		{
			visible_layers = 0xFFFFFFFF;
			fov = 75;
			type = PERSPECTIVE;
			znear = 0.05;
			zfar = 4000;
			size = 1.0;
			offset = Vector2();
			vaspect = false;
		}
	};


	static RID camera_allocate();
	static void camera_initialize(RID p_rid);

	static void camera_set_perspective(RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	static void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	static void camera_set_frustum(
		RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	static void camera_set_transform(RID p_camera, const Transform3D& p_transform);
	static void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	static void camera_set_environment(RID p_camera, RID p_env);
	static void camera_set_camera_attributes(RID p_camera, RID p_attributes);
	static void camera_set_compositor(RID p_camera, RID p_compositor);
	static void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);
	static bool is_camera(RID p_camera);

	/* OCCLUDER API */

	static RID occluder_allocate();
	static void occluder_initialize(RID p_occluder);
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);

	/* SCENARIO API */

	struct Instance;

	struct PlaneSign
	{
		_ALWAYS_INLINE_ PlaneSign() {}

		_ALWAYS_INLINE_ PlaneSign(const Plane& p_plane)
		{
			if (p_plane.normal.x > 0) {
				signs[0] = 0;
			}
			else {
				signs[0] = 3;
			}
			if (p_plane.normal.y > 0) {
				signs[1] = 1;
			}
			else {
				signs[1] = 4;
			}
			if (p_plane.normal.z > 0) {
				signs[2] = 2;
			}
			else {
				signs[2] = 5;
			}
		}

		uint32_t signs[3];
	};

	struct Frustum
	{
		Vector<Plane> planes;
		Vector<PlaneSign> plane_signs;
		const Plane* planes_ptr;
		const PlaneSign* plane_signs_ptr;
		uint32_t plane_count;

		_ALWAYS_INLINE_ Frustum() {}

		_ALWAYS_INLINE_ Frustum(const Frustum& p_frustum)
		{
			planes = p_frustum.planes;
			plane_signs = p_frustum.plane_signs;

			planes_ptr = planes.ptr();
			plane_signs_ptr = plane_signs.ptr();
			plane_count = p_frustum.plane_count;
		}

		_ALWAYS_INLINE_ void operator=(const Frustum& p_frustum)
		{
			planes = p_frustum.planes;
			plane_signs = p_frustum.plane_signs;

			planes_ptr = planes.ptr();
			plane_signs_ptr = plane_signs.ptr();
			plane_count = p_frustum.plane_count;
		}

		_ALWAYS_INLINE_ Frustum(const Vector<Plane>& p_planes)
		{
			planes = p_planes;
			planes_ptr = planes.ptrw();
			plane_count = planes.size();
			for (int i = 0; i < planes.size(); i++) {
				PlaneSign ps(p_planes[i]);
				plane_signs.push_back(ps);
			}

			plane_signs_ptr = plane_signs.ptr();
		}
	};

	struct InstanceBounds
	{
		// Efficiently store instance bounds.
		// Because bounds checking is performed first,
		// keep it separated from data.

		real_t bounds[6];

		_ALWAYS_INLINE_ InstanceBounds() {}

		_ALWAYS_INLINE_ InstanceBounds(const AABB& p_aabb)
		{
			bounds[0] = p_aabb.position.x;
			bounds[1] = p_aabb.position.y;
			bounds[2] = p_aabb.position.z;
			bounds[3] = p_aabb.position.x + p_aabb.size.x;
			bounds[4] = p_aabb.position.y + p_aabb.size.y;
			bounds[5] = p_aabb.position.z + p_aabb.size.z;
		}

		_ALWAYS_INLINE_ bool in_frustum(const Frustum& p_frustum) const
		{
			// This is not a full SAT check and the possibility of false positives exist,
			// but the tradeoff vs performance is still very good.

			for (uint32_t i = 0; i < p_frustum.plane_count; i++) {
				Vector3 min(bounds[p_frustum.plane_signs_ptr[i].signs[0]],
					bounds[p_frustum.plane_signs_ptr[i].signs[1]],
					bounds[p_frustum.plane_signs_ptr[i].signs[2]]);

				if (p_frustum.planes_ptr[i].distance_to(min) >= 0.0) {
					return false;
				}
			}

			return true;
		}

		_ALWAYS_INLINE_ bool in_aabb(const AABB& p_aabb) const
		{
			Vector3 end = p_aabb.position + p_aabb.size;

			if (bounds[0] >= end.x) {
				return false;
			}
			if (bounds[3] <= p_aabb.position.x) {
				return false;
			}
			if (bounds[1] >= end.y) {
				return false;
			}
			if (bounds[4] <= p_aabb.position.y) {
				return false;
			}
			if (bounds[2] >= end.z) {
				return false;
			}
			if (bounds[5] <= p_aabb.position.z) {
				return false;
			}

			return true;
		}
	};

	struct InstanceVisibilityNotifierData;

	struct InstanceData
	{
		// Store instance pointer as well as common instance processing information,
		// to make processing more cache friendly.
		enum Flags : uint32_t
		{
			FLAG_BASE_TYPE_MASK = 0xFF,
			FLAG_CAST_SHADOWS = (1 << 8),
			FLAG_CAST_SHADOWS_ONLY = (1 << 9),
			FLAG_REDRAW_IF_VISIBLE = (1 << 10),
			FLAG_GEOM_LIGHTING_DIRTY = (1 << 11),
			FLAG_GEOM_REFLECTION_DIRTY = (1 << 12),
			FLAG_GEOM_DECAL_DIRTY = (1 << 13),
			FLAG_GEOM_VOXEL_GI_DIRTY = (1 << 14),
			FLAG_LIGHTMAP_CAPTURE = (1 << 15),
			FLAG_USES_BAKED_LIGHT = (1 << 16),
			FLAG_USES_MESH_INSTANCE = (1 << 17),
			FLAG_REFLECTION_PROBE_DIRTY = (1 << 18),
			FLAG_IGNORE_OCCLUSION_CULLING = (1 << 19),
			FLAG_VISIBILITY_DEPENDENCY_NEEDS_CHECK =
				(3 << 20), // 2 bits, overlaps with the other vis. dependency flags
			FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE = (1 << 20),
			FLAG_VISIBILITY_DEPENDENCY_HIDDEN = (1 << 21),
			FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN = (1 << 22),
			FLAG_GEOM_PROJECTOR_SOFTSHADOW_DIRTY = (1 << 23),
			FLAG_IGNORE_ALL_CULLING = (1 << 24),
		};

		uint32_t flags = 0;
		uint32_t layer_mask = 0; // for fast layer-mask discard
		RID base_rid;

		union
		{
			uint64_t instance_data_rid;
			RenderGeometryInstance* instance_geometry;
			InstanceVisibilityNotifierData* visibility_notifier = nullptr;
		};

		Instance* instance = nullptr;
		int32_t parent_array_index = -1;
		int32_t visibility_index = -1;

		// Each time occlusion culling determines an instance is visible,
		// set this to occlusion_frame plus some delay.
		// Once the timeout is reached, allow the instance to be occlusion culled.
		// This creates a delay for occlusion culling, which prevents flickering
		// when jittering the raster occlusion projection.
		uint64_t occlusion_timeout = 0;
	};

	struct InstanceVisibilityData
	{
		uint64_t viewport_state = 0;
		int32_t array_index = -1;
		RSE::VisibilityRangeFadeMode fade_mode = RSE::VISIBILITY_RANGE_FADE_DISABLED;
		Vector3 position;
		Instance* instance = nullptr;
		float range_begin = 0.0f;
		float range_end = 0.0f;
		float range_begin_margin = 0.0f;
		float range_end_margin = 0.0f;
		float children_fade_alpha = 1.0f;
	};

	class VisibilityArray : public BinSortedArray<InstanceVisibilityData>
	{
		_FORCE_INLINE_ void _update_idx(InstanceVisibilityData& r_element, uint64_t p_idx)
		{
			r_element.instance->visibility_index = p_idx;
			if (r_element.instance->scenario && r_element.instance->array_index != -1) {
				r_element.instance->scenario->instance_data[r_element.instance->array_index]
					.visibility_index = p_idx;
			}
		}
	};


	struct Scenario
	{
		enum IndexerType
		{
			INDEXER_GEOMETRY, // for geometry
			INDEXER_VOLUMES,  // for everything else
			INDEXER_MAX
		};

		DynamicBVH indexers[INDEXER_MAX];

		RID self;

		List<Instance*> directional_lights;
		RID environment;
		RID fallback_environment;
		RID camera_attributes;
		RID compositor;
		RID reflection_probe_shadow_atlas;
		RID reflection_atlas;
		uint64_t used_viewport_visibility_bits;
		HashMap<RID, uint64_t> viewport_visibility_masks;

		SelfList<Instance>::List instances;

		LocalVector<RID> dynamic_lights;

		PagedArray<InstanceBounds> instance_aabbs;
		PagedArray<InstanceData> instance_data;
		VisibilityArray instance_visibility;

		Scenario()
		{
			indexers[INDEXER_GEOMETRY].set_index(INDEXER_GEOMETRY);
			indexers[INDEXER_VOLUMES].set_index(INDEXER_VOLUMES);
			used_viewport_visibility_bits = 0;
		}
	};


	static void _instance_pair(Instance* p_A, Instance* p_B);
	static void _instance_unpair(Instance* p_A, Instance* p_B);

	static void _instance_update_mesh_instance(Instance* p_instance);

	static RID scenario_allocate();
	static void scenario_initialize(RID p_rid);

	static void scenario_set_environment(RID p_scenario, RID p_environment);
	static void scenario_set_camera_attributes(RID p_scenario, RID p_attributes);
	static void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	static void scenario_set_compositor(RID p_scenario, RID p_compositor);
	static void scenario_set_reflection_atlas_size(
		RID p_scenario, int p_reflection_size, int p_reflection_count);
	static bool is_scenario(RID p_scenario);
	static RID scenario_get_environment(RID p_scenario);
	static void scenario_add_viewport_visibility_mask(RID p_scenario, RID p_viewport);
	static void scenario_remove_viewport_visibility_mask(RID p_scenario, RID p_viewport);

	/* INSTANCING API */

	struct InstancePair
	{
		Instance* a = nullptr;
		Instance* b = nullptr;
		SelfList<InstancePair> list_a;
		SelfList<InstancePair> list_b;

		InstancePair() : list_a(this), list_b(this) {}
	};


	struct InstanceBaseData
	{
		~InstanceBaseData() = default;
	};

	struct Instance
	{
		RSE::InstanceType base_type;
		RID base;

		RID skeleton;
		RID material_override;
		RID material_overlay;

		RID mesh_instance; // only used for meshes and when skeleton/blendshapes exist

		Transform3D transform;
		bool teleported = false;

		float lod_bias;

		bool ignore_occlusion_culling;
		bool ignore_all_culling;

		Vector<RID> materials;

		RSE::ShadowCastingSetting cast_shadows;

		uint32_t layer_mask;
		// Fit in 32 bits.
		bool mirror : 1;
		bool receive_shadows : 1;
		bool visible : 1;
		bool baked_light : 1; // This flag is only to know if it actually did use baked light.
		bool dynamic_gi : 1;  // Same as above for dynamic objects.
		bool redraw_if_visible : 1;

		Instance* lightmap = nullptr;
		Rect2 lightmap_uv_scale;
		int lightmap_slice_index;
		uint32_t lightmap_cull_index;
		Vector<Color> lightmap_sh; // spherical harmonic

		AABB aabb;
		AABB transformed_aabb;
		AABB prev_transformed_aabb;

		InstanceUniforms instance_uniforms;

		//

		RID self;
		// scenario stuff
		DynamicBVH::ID indexer_id;
		int32_t array_index = -1;
		int32_t visibility_index = -1;
		float visibility_range_begin = 0.0f;
		float visibility_range_end = 0.0f;
		float visibility_range_begin_margin = 0.0f;
		float visibility_range_end_margin = 0.0f;
		RSE::VisibilityRangeFadeMode visibility_range_fade_mode =
			RSE::VISIBILITY_RANGE_FADE_DISABLED;
		Instance* visibility_parent = nullptr;
		HashSet<Instance*> visibility_dependencies;
		uint32_t visibility_dependencies_depth = 0;
		float transparency = 0.0f;
		Scenario* scenario = nullptr;
		SelfList<Instance> scenario_item;

		// aabb stuff
		bool update_aabb;
		bool update_dependencies;

		SelfList<Instance> update_item;

		AABB* custom_aabb = nullptr; // <Zylann> would using aabb directly with a bool be better?
		float extra_margin;

		// sorting
		float sorting_offset = 0.0;
		bool use_aabb_center = true;

		Vector<Color> lightmap_target_sh; // target is used for incrementally changing the SH over
										  // time, this avoids pops in some corner cases and when
										  // going interior <-> exterior

		uint64_t last_frame_pass;

		uint64_t version; // changes to this, and changes to base increase version

		InstanceBaseData* base_data = nullptr;

		SelfList<InstancePair>::List pairs;
		uint64_t pair_check;

		DependencyTracker dependency_tracker;

		static void dependency_changed(
			Dependency::DependencyChangedNotification p_notification, DependencyTracker* tracker)
		{
			Instance* instance = (Instance*)tracker->userdata;
			switch (p_notification) {
			case Dependency::DEPENDENCY_CHANGED_SKELETON_DATA:
			case Dependency::DEPENDENCY_CHANGED_SKELETON_BONES:
			case Dependency::DEPENDENCY_CHANGED_AABB: {
				RendererSceneCull::_instance_queue_update(instance, true, false);

			} break;
			case Dependency::DEPENDENCY_CHANGED_MULTIMESH_VISIBLE_INSTANCES:
			case Dependency::DEPENDENCY_CHANGED_MATERIAL: {
				RendererSceneCull::_instance_queue_update(instance, false, true);
			} break;
			case Dependency::DEPENDENCY_CHANGED_MESH:
			case Dependency::DEPENDENCY_CHANGED_PARTICLES:
			case Dependency::DEPENDENCY_CHANGED_MULTIMESH:
			case Dependency::DEPENDENCY_CHANGED_DECAL:
			case Dependency::DEPENDENCY_CHANGED_LIGHT: {
				RendererSceneCull::_instance_queue_update(instance, true, true);
			} break;
			case Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE:
			case Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR:
			case Dependency::DEPENDENCY_CHANGED_CULL_MASK: {
				// requires repairing
				if (instance->indexer_id.is_valid()) {
					RendererSceneCull::_unpair_instance(instance);
					RendererSceneCull::_instance_queue_update(instance, true, true);
				}

			} break;
			default: {
				// Ignored notifications.
			} break;
			}
		}

		static void dependency_deleted(const RID& p_dependency, DependencyTracker* tracker)
		{
			Instance* instance = (Instance*)tracker->userdata;

			if (p_dependency == instance->base) {
				RendererSceneCull::instance_set_base(instance->self, RID());
			}
			else if (p_dependency == instance->skeleton) {
				RendererSceneCull::instance_attach_skeleton(instance->self, RID());
			}
			else {
				// It's possible the same material is used in multiple slots,
				// so we check whether we need to clear them all.
				if (p_dependency == instance->material_override) {
					RendererSceneCull::instance_geometry_set_material_override(instance->self, RID());
				}
				if (p_dependency == instance->material_overlay) {
					RendererSceneCull::instance_geometry_set_material_overlay(instance->self, RID());
				}
				for (int i = 0; i < instance->materials.size(); i++) {
					if (p_dependency == instance->materials[i]) {
						RendererSceneCull::instance_set_surface_override_material(instance->self, i, RID());
					}
				}
				if (instance->base_type == RSE::INSTANCE_PARTICLES) {
					RID particle_material =
						Renderer::particles_storage->particles_get_process_material(instance->base);
					if (p_dependency == particle_material) {
						Renderer::particles_storage->particles_set_process_material(
							instance->base, RID());
					}
				}

				// Even if no change is made we still need to call `_instance_queue_update`.
				// This dependency could also be a result of the freed material being used
				// by the mesh this mesh instance uses.
				RendererSceneCull::_instance_queue_update(instance, false, true);
			}
		}

		Instance() : scenario_item(this), update_item(this)
		{
			base_type = RSE::INSTANCE_NONE;
			cast_shadows = RSE::SHADOW_CASTING_SETTING_ON;
			receive_shadows = true;
			visible = true;
			layer_mask = 1;
			baked_light = true;
			dynamic_gi = false;
			redraw_if_visible = false;

			lightmap_slice_index = 0;
			lightmap = nullptr;
			lightmap_cull_index = 0;
			lod_bias = 1.0;
			ignore_occlusion_culling = false;
			ignore_all_culling = false;

			scenario = nullptr;

			update_aabb = false;
			update_dependencies = false;

			extra_margin = 0;

			visible = true;

			visibility_range_begin = 0;
			visibility_range_end = 0;
			visibility_range_begin_margin = 0;
			visibility_range_end_margin = 0;

			last_frame_pass = 0;
			version = 1;
			base_data = nullptr;

			custom_aabb = nullptr;

			pair_check = 0;
			array_index = -1;

			dependency_tracker.userdata = this;
			dependency_tracker.changed_callback = dependency_changed;
			dependency_tracker.deleted_callback = dependency_deleted;
		}

		~Instance()
		{
			memdelete(base_data);
			memdelete(custom_aabb);
		}
	};

	static void _instance_queue_update(
		Instance* p_instance, bool p_update_aabb, bool p_update_dependencies = false);

	struct InstanceGeometryData : public InstanceBaseData
	{
		RenderGeometryInstance* geometry_instance = nullptr;
		HashSet<Instance*> lights;
		bool can_cast_shadows;
		bool material_is_animated;
		uint32_t projector_count = 0;
		uint32_t softshadow_count = 0;

		HashSet<Instance*> decals;
		HashSet<Instance*> reflection_probes;
		HashSet<Instance*> voxel_gi_instances;
		HashSet<Instance*> lightmap_captures;

		InstanceGeometryData()
		{
			can_cast_shadows = true;
			material_is_animated = true;
		}
	};

	struct InstanceReflectionProbeData : public InstanceBaseData
	{
		Instance* owner = nullptr;

		HashSet<Instance*> geometries;

		RID instance;
		SelfList<InstanceReflectionProbeData> update_list;

		int render_step;

		InstanceReflectionProbeData() : update_list(this) { render_step = -1; }
	};

	struct InstanceDecalData : public InstanceBaseData
	{
		Instance* owner = nullptr;
		RID instance;
		uint32_t cull_mask = 0xFFFFFFFF;

		HashSet<Instance*> geometries;

		InstanceDecalData() {}
	};


	struct InstanceParticlesCollisionData : public InstanceBaseData
	{
		RID instance;
		uint32_t cull_mask = 0xFFFFFFFF;
	};

	struct InstanceFogVolumeData : public InstanceBaseData
	{
		RID instance;
		bool is_global;
	};

	struct InstanceVisibilityNotifierData : public InstanceBaseData
	{
		bool just_visible = false;
		uint64_t visible_in_frame = 0;
		RID base;
		SelfList<InstanceVisibilityNotifierData> list_element;

		InstanceVisibilityNotifierData() : list_element(this) {}
	};


	struct InstanceLightData : public InstanceBaseData
	{
		RID instance;
		uint64_t last_version;
		List<Instance*>::Element* D; // directional light in scenario

		bool uses_projector = false;
		bool uses_softshadow = false;

		HashSet<Instance*> geometries;

		Instance* baked_light = nullptr;

		RSE::LightBakeMode bake_mode;
		uint32_t max_sdfgi_cascade = 2;
		uint32_t cull_mask = 0xFFFFFFFF;

	private:
		// Instead of a single dirty flag, we maintain a count
		// so that we can detect lights that are being made dirty
		// each frame, and switch on tighter caster culling.
		int32_t shadow_dirty_count;

		uint32_t light_update_frame_id;
		bool light_intersects_multiple_cameras;
		uint32_t light_intersects_multiple_cameras_timeout_frame_id;

	public:
		bool is_shadow_dirty() const { return shadow_dirty_count != 0; }

		void make_shadow_dirty() { shadow_dirty_count = light_intersects_multiple_cameras ? 1 : 2; }

		void detect_light_intersects_multiple_cameras(uint32_t p_frame_id)
		{
			// We need to detect the case where shadow updates are occurring
			// more than once per frame. In this case, we need to turn off
			// tighter caster culling, so situation reverts to one full shadow update
			// per frame (light_intersects_multiple_cameras is set).
			if (p_frame_id == light_update_frame_id) {
				light_intersects_multiple_cameras = true;
				light_intersects_multiple_cameras_timeout_frame_id = p_frame_id + 60;
			}
			else {
				// When shadow_volume_intersects_multiple_cameras is set, we
				// want to detect the situation this is no longer the case, via a timeout.
				// The system can go back to tighter caster culling in this situation.
				// Having a long-ish timeout prevents rapid cycling.
				if (light_intersects_multiple_cameras &&
					(p_frame_id >= light_intersects_multiple_cameras_timeout_frame_id)) {
					light_intersects_multiple_cameras = false;
					light_intersects_multiple_cameras_timeout_frame_id = UINT32_MAX;
				}
			}
			light_update_frame_id = p_frame_id;
		}

		void decrement_shadow_dirty()
		{
			shadow_dirty_count--;
			DEV_ASSERT(shadow_dirty_count >= 0);
		}

		// Shadow updates can either full (everything in the shadow volume)
		// or closely culled to the camera frustum.
		bool is_shadow_update_full() const { return shadow_dirty_count == 0; }

		InstanceLightData()
		{
			bake_mode = RSE::LIGHT_BAKE_DISABLED;
			D = nullptr;
			last_version = 0;
			baked_light = nullptr;

			shadow_dirty_count = 1;
			light_update_frame_id = UINT32_MAX;
			light_intersects_multiple_cameras_timeout_frame_id = UINT32_MAX;
			light_intersects_multiple_cameras = false;
		}
	};

	struct InstanceVoxelGIData : public InstanceBaseData
	{
		Instance* owner = nullptr;

		HashSet<Instance*> geometries;
		HashSet<Instance*> dynamic_geometries;

		HashSet<Instance*> lights;

		struct LightCache
		{
			RSE::LightType type;
			Transform3D transform;
			Color color;
			float energy;
			float intensity;
			float bake_energy;
			float radius;
			float attenuation;
			float spot_angle;
			float spot_attenuation;
			bool has_shadow;
			RSE::LightDirectionalSkyMode sky_mode;
			Vector2 area_size;
			bool area_normalize_energy;
			RID area_texture;
		};

		Vector<LightCache> light_cache;
		Vector<RID> light_instances;

		RID probe_instance;

		bool invalid;
		uint32_t base_version;

		SelfList<InstanceVoxelGIData> update_element;

		InstanceVoxelGIData() : update_element(this)
		{
			invalid = true;
			base_version = 0;
		}
	};


	struct InstanceLightmapData : public InstanceBaseData
	{
		RID instance;
		HashSet<Instance*> geometries;
		HashSet<Instance*> users;

		InstanceLightmapData() {}
	};


	struct PairInstances
	{
		Instance* instance = nullptr;
		PagedAllocator<InstancePair>* pair_allocator = nullptr;
		SelfList<InstancePair>::List pairs_found;
		DynamicBVH* bvh = nullptr;
		DynamicBVH* bvh2 = nullptr; // some may need to cull in two
		uint32_t pair_mask;
		uint64_t pair_pass;

		_FORCE_INLINE_ bool operator()(void* p_data)
		{
			Instance* p_instance = (Instance*)p_data;

			if (instance != p_instance &&
				instance->transformed_aabb.intersects(p_instance->transformed_aabb) &&
				(pair_mask & (1 << p_instance->base_type))) {
				// test is more coarse in indexer
				p_instance->pair_check = pair_pass;
				InstancePair* pair = pair_allocator->alloc();
				pair->a = instance;
				pair->b = p_instance;
				pairs_found.add(&pair->list_a);
			}
			return false;
		}

		void pair()
		{
			if (bvh) {
				bvh->aabb_query(instance->transformed_aabb, *this);
			}
			if (bvh2) {
				bvh2->aabb_query(instance->transformed_aabb, *this);
			}
			while (instance->pairs.first()) {
				InstancePair* pair = instance->pairs.first()->self();
				Instance* other_instance = instance == pair->a ? pair->b : pair->a;
				if (other_instance->pair_check != pair_pass) {
					// unpaired
					_instance_unpair(instance, other_instance);
				}
				else {
					// kept
					other_instance->pair_check = 0; // if kept, then put pair check to zero, so we
													// can distinguish with the newly added ones
				}

				pair_allocator->free(pair);
			}
			while (pairs_found.first()) {
				InstancePair* pair = pairs_found.first()->self();
				pairs_found.remove(pairs_found.first());

				if (pair->b->pair_check == pair_pass) {
					// paired
					_instance_pair(instance, pair->b);
				}
				pair->a->pairs.add(&pair->list_a);
				pair->b->pairs.add(&pair->list_b);
			}
		}
	};


	struct InstanceCullResult
	{
		PagedArray<RenderGeometryInstance*> geometry_instances;
		PagedArray<Instance*> lights;
		PagedArray<RID> light_instances;
		PagedArray<RID> lightmaps;
		PagedArray<RID> reflections;
		PagedArray<RID> decals;
		PagedArray<RID> voxel_gi_instances;
		PagedArray<RID> mesh_instances;
		PagedArray<RID> fog_volumes;

		struct DirectionalShadow
		{
			PagedArray<RenderGeometryInstance*>
				cascade_geometry_instances[RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES];
		} directional_shadows[RendererSceneRender::MAX_DIRECTIONAL_LIGHTS];

		PagedArray<RenderGeometryInstance*>
			sdfgi_region_geometry_instances[SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE];
		PagedArray<RID> sdfgi_cascade_lights[SDFGI_MAX_CASCADES];

		void clear()
		{
			geometry_instances.clear();
			lights.clear();
			light_instances.clear();
			lightmaps.clear();
			reflections.clear();
			decals.clear();
			voxel_gi_instances.clear();
			mesh_instances.clear();
			fog_volumes.clear();
			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].clear();
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].clear();
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].clear();
			}
		}

		void reset()
		{
			geometry_instances.reset();
			lights.reset();
			light_instances.reset();
			lightmaps.reset();
			reflections.reset();
			decals.reset();
			voxel_gi_instances.reset();
			mesh_instances.reset();
			fog_volumes.reset();
			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].reset();
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].reset();
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].reset();
			}
		}

		void append_from(InstanceCullResult& p_cull_result)
		{
			geometry_instances.merge_unordered(p_cull_result.geometry_instances);
			lights.merge_unordered(p_cull_result.lights);
			light_instances.merge_unordered(p_cull_result.light_instances);
			lightmaps.merge_unordered(p_cull_result.lightmaps);
			reflections.merge_unordered(p_cull_result.reflections);
			decals.merge_unordered(p_cull_result.decals);
			voxel_gi_instances.merge_unordered(p_cull_result.voxel_gi_instances);
			mesh_instances.merge_unordered(p_cull_result.mesh_instances);
			fog_volumes.merge_unordered(p_cull_result.fog_volumes);

			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].merge_unordered(
						p_cull_result.directional_shadows[i].cascade_geometry_instances[j]);
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].merge_unordered(
					p_cull_result.sdfgi_region_geometry_instances[i]);
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].merge_unordered(p_cull_result.sdfgi_cascade_lights[i]);
			}
		}

		void init(PagedArrayPool<RID>* p_rid_pool,
			PagedArrayPool<RenderGeometryInstance*>* p_geometry_instance_pool,
			PagedArrayPool<Instance*>* p_instance_pool)
		{
			geometry_instances.set_page_pool(p_geometry_instance_pool);
			light_instances.set_page_pool(p_rid_pool);
			lights.set_page_pool(p_instance_pool);
			lightmaps.set_page_pool(p_rid_pool);
			reflections.set_page_pool(p_rid_pool);
			decals.set_page_pool(p_rid_pool);
			voxel_gi_instances.set_page_pool(p_rid_pool);
			mesh_instances.set_page_pool(p_rid_pool);
			fog_volumes.set_page_pool(p_rid_pool);
			for (int i = 0; i < RendererSceneRender::MAX_DIRECTIONAL_LIGHTS; i++) {
				for (int j = 0; j < RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES; j++) {
					directional_shadows[i].cascade_geometry_instances[j].set_page_pool(
						p_geometry_instance_pool);
				}
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
				sdfgi_region_geometry_instances[i].set_page_pool(p_geometry_instance_pool);
			}

			for (int i = 0; i < SDFGI_MAX_CASCADES; i++) {
				sdfgi_cascade_lights[i].set_page_pool(p_rid_pool);
			}
		}
	};


	static RID instance_allocate();
	static void instance_initialize(RID p_rid);

	static void instance_set_base(RID p_instance, RID p_base);
	static void instance_set_scenario(RID p_instance, RID p_scenario);
	static void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	static void instance_set_pivot_data(RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	static void instance_set_transform(RID p_instance, const Transform3D& p_transform);
	static void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	static void instance_set_surface_override_material(RID p_instance, int p_surface, RID p_material);
	static void instance_set_visible(RID p_instance, bool p_visible);
	static void instance_geometry_set_transparency(RID p_instance, float p_transparency);

	static void instance_teleport(RID p_instance);

	static void instance_set_custom_aabb(RID p_instance, AABB p_aabb);

	static void instance_attach_skeleton(RID p_instance, RID p_skeleton);

	static void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);

	static void instance_set_visibility_parent(RID p_instance, RID p_parent_instance);

	static void instance_set_ignore_culling(RID p_instance, bool p_enabled);

	static bool _update_instance_visibility_depth(Instance* p_instance);
	static void _update_instance_visibility_dependencies(Instance* p_instance);

	static void instance_geometry_set_flag(RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled);
	static void instance_geometry_set_cast_shadows_setting(
		RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting);
	static void instance_geometry_set_material_override(RID p_instance, RID p_material);
	static void instance_geometry_set_material_overlay(RID p_instance, RID p_material);

	static void instance_geometry_set_visibility_range(RID p_instance, float p_min, float p_max,
		float p_min_margin, float p_max_margin, RSE::VisibilityRangeFadeMode p_fade_mode);

	static void instance_geometry_set_lightmap(
		RID p_instance, RID p_lightmap, const Rect2& p_lightmap_uv_scale, int p_slice_index);
	static void instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias);

	static void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation);
	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	static _FORCE_INLINE_ void _update_instance(Instance* p_instance);
	static _FORCE_INLINE_ void _update_instance_aabb(Instance* p_instance);
	static _FORCE_INLINE_ void _update_instance_lightmap_captures(Instance* p_instance);
	static void _unpair_instance(Instance* p_instance);

	static void _light_instance_setup_directional_shadow(int p_shadow_index, Instance* p_instance,
		const Transform3D p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, bool p_cam_vaspect);

	static _FORCE_INLINE_ bool _light_instance_update_shadow(Instance* p_instance,
		const Transform3D p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, bool p_cam_vaspect, RID p_shadow_atlas, Scenario* p_scenario,
		float p_screen_mesh_lod_threshold, uint32_t p_visible_layers = 0xFFFFFF);

	static RID _render_get_environment(RID p_camera, RID p_scenario);
	static RID _render_get_compositor(RID p_camera, RID p_scenario);

	struct Cull
	{
		struct Shadow
		{
			RID light_instance;
			uint32_t caster_mask;

			struct Cascade
			{
				Frustum frustum;

				Projection projection;
				Transform3D transform;
				real_t zfar;
				real_t split;
				real_t shadow_texel_size;
				real_t bias_scale;
				real_t range_begin;
				Vector2 uv_scale;

			} cascades[RendererSceneRender::MAX_DIRECTIONAL_LIGHT_CASCADES]; // max 4 cascades

			uint32_t cascade_count;

		} shadows[RendererSceneRender::MAX_DIRECTIONAL_LIGHTS];

		uint32_t shadow_count;

		struct SDFGI
		{
			// have arrays here because SDFGI functions expects this, plus regions can have areas
			AABB region_aabb[SDFGI_MAX_CASCADES *
							 SDFGI_MAX_REGIONS_PER_CASCADE]; // max 3 regions per cascade
			uint32_t region_cascade[SDFGI_MAX_CASCADES *
									SDFGI_MAX_REGIONS_PER_CASCADE]; // max 3 regions per cascade
			uint32_t region_count = 0;

			uint32_t cascade_light_index[SDFGI_MAX_CASCADES];
			uint32_t cascade_light_count = 0;

		} sdfgi;

		SpinLock lock;

		Frustum frustum;
	};

	struct VisibilityCullData
	{
		uint64_t viewport_mask;
		Scenario* scenario = nullptr;
		Vector3 camera_position;
		uint32_t cull_offset;
		uint32_t cull_count;
	};

	static void _visibility_cull_threaded(uint32_t p_thread, VisibilityCullData* cull_data);
	static void _visibility_cull(const VisibilityCullData& cull_data, uint64_t p_from, uint64_t p_to);
	template <bool p_fade_check>
	static _FORCE_INLINE_ int _visibility_range_check(
		InstanceVisibilityData& r_vis_data, const Vector3& p_camera_pos, uint64_t p_viewport_mask);

	struct CullData
	{
		Cull* cull = nullptr;
		Scenario* scenario = nullptr;
		RID shadow_atlas;
		Transform3D cam_transform;
		uint32_t visible_layers;
		Instance* render_reflection_probe = nullptr;
		const RendererSceneOcclusionCull::HZBuffer* occlusion_buffer;
		const Projection* camera_matrix;
		uint64_t visibility_viewport_mask;
	};

	static void _scene_cull_threaded(uint32_t p_thread, CullData* cull_data);
	static void _scene_cull(
		CullData& cull_data, InstanceCullResult& cull_result, uint64_t p_from, uint64_t p_to);
	static void _scene_particles_set_view_axis(
		RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis);
	static _FORCE_INLINE_ bool _visibility_parent_check(
		const CullData& p_cull_data, const InstanceData& p_instance_data);

	static bool _render_reflection_probe_step(Instance* p_instance, int p_step);

	static void render_empty_scene(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_scenario,
		RID p_shadow_atlas, float p_window_output_max_value);

	static void render_camera(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_camera,
		RID p_scenario, RID p_viewport, Size2 p_viewport_size, uint32_t p_jitter_phase_count,
		float p_screen_mesh_lod_threshold, RID p_shadow_atlas, Ref<XRInterface>& p_xr_interface,
		float p_window_output_max_value, RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	static void render_particle_colliders();
	static void render_probes();

	struct InterpolationData
	{
		bool interpolation_enabled = false;
	};

	struct Data
	{
		RendererSceneRender* scene_render = nullptr;
		uint64_t render_pass = 0;

		RID_Owner<Camera, true> camera_owner;

		PagedArrayPool<InstanceBounds> instance_aabb_page_pool;
		PagedArrayPool<InstanceData> instance_data_page_pool;
		PagedArrayPool<InstanceVisibilityData> instance_visibility_data_page_pool;

		int indexer_update_iterations = 0;
		RID_Owner<Scenario, true> scenario_owner;

		PagedAllocator<InstancePair> pair_allocator;
		SelfList<Instance>::List _instance_update_list;
		SelfList<InstanceReflectionProbeData>::List reflection_probe_render_list;

		SpinLock visible_notifier_list_lock;
		SelfList<InstanceVisibilityNotifierData>::List visible_notifier_list;
		SelfList<InstanceVoxelGIData>::List voxel_gi_update_list;

		uint64_t pair_pass = 1;
		HashSet<Instance*> heightfield_particle_colliders_update_list;

		PagedArrayPool<Instance*> instance_cull_page_pool;
		PagedArrayPool<RenderGeometryInstance*> geometry_instance_cull_page_pool;
		PagedArrayPool<RID> rid_cull_page_pool;

		PagedArray<Instance*> instance_cull_result;
		PagedArray<Instance*> instance_shadow_cull_result;

		InstanceCullResult scene_cull_result;
		LocalVector<InstanceCullResult> scene_cull_result_threads;

		RendererSceneRender::RenderShadowData render_shadow_data[MAX_UPDATE_SHADOWS];
		uint32_t max_shadows_used = 0;

		RendererSceneRender::RenderSDFGIData
			render_sdfgi_data[SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE];
		RendererSceneRender::RenderSDFGIUpdateData sdfgi_update_data;

		uint32_t thread_cull_threshold = 200;

		RID_Owner<Instance, true> instance_owner{65536, 4194304};

		uint32_t geometry_instance_pair_mask = 0;

		LocalVector<Vector2> camera_jitter_array;
		RenderingLightCuller* light_culler = nullptr;

		Cull cull;
		InterpolationData _interpolation_data;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }
	static _FORCE_INLINE_ RendererSceneRender* get_scene_render()
	{
		return data ? data->scene_render : nullptr;
	}

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_arg1) { if (data && data->scene_render) { data->scene_render->voxel_gi_set_quality(p_arg1); } }

	/* SKY API */

	static RID sky_allocate() { ERR_FAIL_COND_V(!data || !data->scene_render, RID()); return data->scene_render->sky_allocate(); }
	static void sky_initialize(RID p_arg1) { if (data && data->scene_render) { data->scene_render->sky_initialize(p_arg1); } }

	static void sky_set_radiance_size(RID p_arg1, int p_arg2) { if (data && data->scene_render) { data->scene_render->sky_set_radiance_size(p_arg1, p_arg2); } }
	static void sky_set_mode(RID p_arg1, RSE::SkyMode p_arg2) { if (data && data->scene_render) { data->scene_render->sky_set_mode(p_arg1, p_arg2); } }
	static void sky_set_material(RID p_arg1, RID p_arg2) { if (data && data->scene_render) { data->scene_render->sky_set_material(p_arg1, p_arg2); } }
	static Ref<Image> sky_bake_panorama(RID p_arg1, float p_arg2, bool p_arg3, const Size2i& p_arg4) { ERR_FAIL_COND_V(!data || !data->scene_render, Ref<Image>()); return data->scene_render->sky_bake_panorama(p_arg1, p_arg2, p_arg3, p_arg4); }

	// Compositor effect

	static RID compositor_effect_allocate() { return RendererSceneRender::compositor_effect_allocate(); }
	static void compositor_effect_initialize(RID p_arg1) { RendererSceneRender::compositor_effect_initialize(p_arg1); }

	static bool is_compositor_effect(RID p_arg1) { return RendererSceneRender::is_compositor_effect(p_arg1); }

	static void compositor_effect_set_enabled(RID p_arg1, bool p_arg2) { RendererSceneRender::compositor_effect_set_enabled(p_arg1, p_arg2); }
	static void compositor_effect_set_flag(RID p_arg1, RSE::CompositorEffectFlags p_arg2, bool p_arg3) { RendererSceneRender::compositor_effect_set_flag(p_arg1, p_arg2, p_arg3); }

	// Compositor

	static RID compositor_allocate() { return RendererSceneRender::compositor_allocate(); }
	static void compositor_initialize(RID p_arg1) { RendererSceneRender::compositor_initialize(p_arg1); }

	static bool is_compositor(RID p_arg1) { return RendererSceneRender::is_compositor(p_arg1); }

	// Environment

	static RID environment_allocate() { return RendererSceneRender::environment_allocate(); }
	static void environment_initialize(RID p_arg1) { RendererSceneRender::environment_initialize(p_arg1); }

	static bool is_environment(RID p_arg1) { return RendererSceneRender::is_environment(p_arg1); }

	// Background
	static void environment_set_background(RID p_arg1, RSE::EnvironmentBG p_arg2) { RendererSceneRender::environment_set_background(p_arg1, p_arg2); }
	static void environment_set_sky(RID p_arg1, RID p_arg2) { RendererSceneRender::environment_set_sky(p_arg1, p_arg2); }
	static void environment_set_sky_custom_fov(RID p_arg1, float p_arg2) { RendererSceneRender::environment_set_sky_custom_fov(p_arg1, p_arg2); }
	static void environment_set_sky_orientation(RID p_arg1, const Basis& p_arg2) { RendererSceneRender::environment_set_sky_orientation(p_arg1, p_arg2); }
	static void environment_set_bg_color(RID p_arg1, const Color& p_arg2) { RendererSceneRender::environment_set_bg_color(p_arg1, p_arg2); }
	static void environment_set_bg_energy(RID p_arg1, float p_arg2, float p_arg3) { RendererSceneRender::environment_set_bg_energy(p_arg1, p_arg2, p_arg3); }
	static void environment_set_canvas_max_layer(RID p_arg1, int p_arg2) { RendererSceneRender::environment_set_canvas_max_layer(p_arg1, p_arg2); }
	static void environment_set_ambient_light(RID p_arg1, const Color& p_arg2, RSE::EnvironmentAmbientSource p_arg3, float p_arg4, float p_arg5, RSE::EnvironmentReflectionSource p_arg6) { RendererSceneRender::environment_set_ambient_light(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); }
	static void environment_set_camera_feed_id(RID p_arg1, int p_arg2) { RendererSceneRender::environment_set_camera_feed_id(p_arg1, p_arg2); }

	static RSE::EnvironmentBG environment_get_background(RID p_arg1) { return RendererSceneRender::environment_get_background(p_arg1); }
	static RID environment_get_sky(RID p_arg1) { return RendererSceneRender::environment_get_sky(p_arg1); }
	static float environment_get_sky_custom_fov(RID p_arg1) { return RendererSceneRender::environment_get_sky_custom_fov(p_arg1); }
	static Basis environment_get_sky_orientation(RID p_arg1) { return RendererSceneRender::environment_get_sky_orientation(p_arg1); }
	static Color environment_get_bg_color(RID p_arg1) { return RendererSceneRender::environment_get_bg_color(p_arg1); }
	static float environment_get_bg_energy_multiplier(RID p_arg1) { return RendererSceneRender::environment_get_bg_energy_multiplier(p_arg1); }
	static float environment_get_bg_intensity(RID p_arg1) { return RendererSceneRender::environment_get_bg_intensity(p_arg1); }
	static int environment_get_canvas_max_layer(RID p_arg1) { return RendererSceneRender::environment_get_canvas_max_layer(p_arg1); }
	static RSE::EnvironmentAmbientSource environment_get_ambient_source(RID p_arg1) { return RendererSceneRender::environment_get_ambient_source(p_arg1); }
	static Color environment_get_ambient_light(RID p_arg1) { return RendererSceneRender::environment_get_ambient_light(p_arg1); }
	static float environment_get_ambient_light_energy(RID p_arg1) { return RendererSceneRender::environment_get_ambient_light_energy(p_arg1); }
	static float environment_get_ambient_sky_contribution(RID p_arg1) { return RendererSceneRender::environment_get_ambient_sky_contribution(p_arg1); }
	static RSE::EnvironmentReflectionSource environment_get_reflection_source(RID p_arg1) { return RendererSceneRender::environment_get_reflection_source(p_arg1); }

	// Tonemap
	static void environment_set_tonemap(RID p_arg1, RSE::EnvironmentToneMapper p_arg2, float p_arg3, float p_arg4) { RendererSceneRender::environment_set_tonemap(p_arg1, p_arg2, p_arg3, p_arg4); }
	static void environment_set_tonemap_agx_contrast(RID p_arg1, float p_arg2) { RendererSceneRender::environment_set_tonemap_agx_contrast(p_arg1, p_arg2); }
	static RSE::EnvironmentToneMapper environment_get_tone_mapper(RID p_arg1) { return RendererSceneRender::environment_get_tone_mapper(p_arg1); }
	static float environment_get_exposure(RID p_arg1) { return RendererSceneRender::environment_get_exposure(p_arg1); }
	static float environment_get_white(RID p_arg1, bool p_arg2, float p_arg3) { return RendererSceneRender::environment_get_white(p_arg1, p_arg2, p_arg3); }

	// Fog
	static void environment_set_fog(RID p_arg1, bool p_arg2, const Color& p_arg3, float p_arg4, float p_arg5, float p_arg6, float p_arg7, float p_arg8, float p_arg9, float p_arg10, RSE::EnvironmentFogMode p_arg11) { RendererSceneRender::environment_set_fog(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11); }

	static bool environment_get_fog_enabled(RID p_arg1) { return RendererSceneRender::environment_get_fog_enabled(p_arg1); }
	static Color environment_get_fog_light_color(RID p_arg1) { return RendererSceneRender::environment_get_fog_light_color(p_arg1); }
	static float environment_get_fog_light_energy(RID p_arg1) { return RendererSceneRender::environment_get_fog_light_energy(p_arg1); }
	static float environment_get_fog_sun_scatter(RID p_arg1) { return RendererSceneRender::environment_get_fog_sun_scatter(p_arg1); }
	static float environment_get_fog_density(RID p_arg1) { return RendererSceneRender::environment_get_fog_density(p_arg1); }
	static float environment_get_fog_sky_affect(RID p_arg1) { return RendererSceneRender::environment_get_fog_sky_affect(p_arg1); }
	static float environment_get_fog_height(RID p_arg1) { return RendererSceneRender::environment_get_fog_height(p_arg1); }
	static float environment_get_fog_height_density(RID p_arg1) { return RendererSceneRender::environment_get_fog_height_density(p_arg1); }
	static float environment_get_fog_aerial_perspective(RID p_arg1) { return RendererSceneRender::environment_get_fog_aerial_perspective(p_arg1); }
	static RSE::EnvironmentFogMode environment_get_fog_mode(RID p_arg1) { return RendererSceneRender::environment_get_fog_mode(p_arg1); }

	static void environment_set_volumetric_fog_volume_size(int p_arg1, int p_arg2) { if (data && data->scene_render) { data->scene_render->environment_set_volumetric_fog_volume_size(p_arg1, p_arg2); } }
	static void environment_set_volumetric_fog_filter_active(bool p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_volumetric_fog_filter_active(p_arg1); } }

	// Depth Fog
	static void environment_set_fog_depth(RID p_arg1, float p_arg2, float p_arg3, float p_arg4) { RendererSceneRender::environment_set_fog_depth(p_arg1, p_arg2, p_arg3, p_arg4); }
	static float environment_get_fog_depth_curve(RID p_arg1) { return RendererSceneRender::environment_get_fog_depth_curve(p_arg1); }
	static float environment_get_fog_depth_begin(RID p_arg1) { return RendererSceneRender::environment_get_fog_depth_begin(p_arg1); }
	static float environment_get_fog_depth_end(RID p_arg1) { return RendererSceneRender::environment_get_fog_depth_end(p_arg1); }

	// Volumentric Fog
	static void environment_set_volumetric_fog(RID p_arg1, bool p_arg2, float p_arg3, const Color& p_arg4, const Color& p_arg5, float p_arg6, float p_arg7, float p_arg8, float p_arg9, float p_arg10, bool p_arg11, float p_arg12, float p_arg13, float p_arg14) { RendererSceneRender::environment_set_volumetric_fog(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11, p_arg12, p_arg13, p_arg14); }

	static bool environment_get_volumetric_fog_enabled(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_enabled(p_arg1); }
	static float environment_get_volumetric_fog_density(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_density(p_arg1); }
	static Color environment_get_volumetric_fog_scattering(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_scattering(p_arg1); }
	static Color environment_get_volumetric_fog_emission(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_emission(p_arg1); }
	static float environment_get_volumetric_fog_emission_energy(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_emission_energy(p_arg1); }
	static float environment_get_volumetric_fog_anisotropy(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_anisotropy(p_arg1); }
	static float environment_get_volumetric_fog_length(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_length(p_arg1); }
	static float environment_get_volumetric_fog_detail_spread(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_detail_spread(p_arg1); }
	static float environment_get_volumetric_fog_gi_inject(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_gi_inject(p_arg1); }
	static float environment_get_volumetric_fog_sky_affect(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_sky_affect(p_arg1); }
	static bool environment_get_volumetric_fog_temporal_reprojection(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection(p_arg1); }
	static float environment_get_volumetric_fog_temporal_reprojection_amount(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_temporal_reprojection_amount(p_arg1); }
	static float environment_get_volumetric_fog_ambient_inject(RID p_arg1) { return RendererSceneRender::environment_get_volumetric_fog_ambient_inject(p_arg1); }

	// Glow
	static void environment_set_glow(RID p_arg1, bool p_arg2, Vector<float> p_arg3, float p_arg4, float p_arg5, float p_arg6, float p_arg7, RSE::EnvironmentGlowBlendMode p_arg8, float p_arg9, float p_arg10, float p_arg11, float p_arg12, RID p_arg13) { RendererSceneRender::environment_set_glow(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11, p_arg12, p_arg13); }

	static bool environment_get_glow_enabled(RID p_arg1) { return RendererSceneRender::environment_get_glow_enabled(p_arg1); }
	static Vector<float> environment_get_glow_levels(RID p_arg1) { return RendererSceneRender::environment_get_glow_levels(p_arg1); }
	static float environment_get_glow_intensity(RID p_arg1) { return RendererSceneRender::environment_get_glow_intensity(p_arg1); }
	static float environment_get_glow_strength(RID p_arg1) { return RendererSceneRender::environment_get_glow_strength(p_arg1); }
	static float environment_get_glow_bloom(RID p_arg1) { return RendererSceneRender::environment_get_glow_bloom(p_arg1); }
	static float environment_get_glow_mix(RID p_arg1) { return RendererSceneRender::environment_get_glow_mix(p_arg1); }
	static RSE::EnvironmentGlowBlendMode environment_get_glow_blend_mode(RID p_arg1) { return RendererSceneRender::environment_get_glow_blend_mode(p_arg1); }
	static float environment_get_glow_hdr_bleed_threshold(RID p_arg1) { return RendererSceneRender::environment_get_glow_hdr_bleed_threshold(p_arg1); }
	static float environment_get_glow_hdr_luminance_cap(RID p_arg1) { return RendererSceneRender::environment_get_glow_hdr_luminance_cap(p_arg1); }
	static float environment_get_glow_hdr_bleed_scale(RID p_arg1) { return RendererSceneRender::environment_get_glow_hdr_bleed_scale(p_arg1); }
	static float environment_get_glow_map_strength(RID p_arg1) { return RendererSceneRender::environment_get_glow_map_strength(p_arg1); }
	static RID environment_get_glow_map(RID p_arg1) { return RendererSceneRender::environment_get_glow_map(p_arg1); }

	static void environment_glow_set_use_bicubic_upscale(bool p_arg1) { if (data && data->scene_render) { data->scene_render->environment_glow_set_use_bicubic_upscale(p_arg1); } }

	// SSR
	static void environment_set_ssr(RID p_arg1, bool p_arg2, int p_arg3, float p_arg4, float p_arg5, float p_arg6) { RendererSceneRender::environment_set_ssr(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); }

	static bool environment_get_ssr_enabled(RID p_arg1) { return RendererSceneRender::environment_get_ssr_enabled(p_arg1); }
	static int environment_get_ssr_max_steps(RID p_arg1) { return RendererSceneRender::environment_get_ssr_max_steps(p_arg1); }
	static float environment_get_ssr_fade_in(RID p_arg1) { return RendererSceneRender::environment_get_ssr_fade_in(p_arg1); }
	static float environment_get_ssr_fade_out(RID p_arg1) { return RendererSceneRender::environment_get_ssr_fade_out(p_arg1); }
	static float environment_get_ssr_depth_tolerance(RID p_arg1) { return RendererSceneRender::environment_get_ssr_depth_tolerance(p_arg1); }

	static void environment_set_ssr_half_size(bool p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_ssr_half_size(p_arg1); } }
	static void environment_set_ssr_roughness_quality(RSE::EnvironmentSSRRoughnessQuality p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_ssr_roughness_quality(p_arg1); } }

	// SSAO
	static void environment_set_ssao(RID p_arg1, bool p_arg2, float p_arg3, float p_arg4, float p_arg5, float p_arg6, float p_arg7, float p_arg8, float p_arg9, float p_arg10) { RendererSceneRender::environment_set_ssao(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10); }

	static bool environment_get_ssao_enabled(RID p_arg1) { return RendererSceneRender::environment_get_ssao_enabled(p_arg1); }
	static float environment_get_ssao_radius(RID p_arg1) { return RendererSceneRender::environment_get_ssao_radius(p_arg1); }
	static float environment_get_ssao_intensity(RID p_arg1) { return RendererSceneRender::environment_get_ssao_intensity(p_arg1); }
	static float environment_get_ssao_power(RID p_arg1) { return RendererSceneRender::environment_get_ssao_power(p_arg1); }
	static float environment_get_ssao_detail(RID p_arg1) { return RendererSceneRender::environment_get_ssao_detail(p_arg1); }
	static float environment_get_ssao_horizon(RID p_arg1) { return RendererSceneRender::environment_get_ssao_horizon(p_arg1); }
	static float environment_get_ssao_sharpness(RID p_arg1) { return RendererSceneRender::environment_get_ssao_sharpness(p_arg1); }
	static float environment_get_ssao_direct_light_affect(RID p_arg1) { return RendererSceneRender::environment_get_ssao_direct_light_affect(p_arg1); }
	static float environment_get_ssao_ao_channel_affect(RID p_arg1) { return RendererSceneRender::environment_get_ssao_ao_channel_affect(p_arg1); }

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_arg1, bool p_arg2, float p_arg3, int p_arg4, float p_arg5, float p_arg6) { if (data && data->scene_render) { data->scene_render->environment_set_ssao_quality(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); } }

	// SSIL
	static void environment_set_ssil(RID p_arg1, bool p_arg2, float p_arg3, float p_arg4, float p_arg5, float p_arg6) { RendererSceneRender::environment_set_ssil(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); }

	static bool environment_get_ssil_enabled(RID p_arg1) { return RendererSceneRender::environment_get_ssil_enabled(p_arg1); }
	static float environment_get_ssil_radius(RID p_arg1) { return RendererSceneRender::environment_get_ssil_radius(p_arg1); }
	static float environment_get_ssil_intensity(RID p_arg1) { return RendererSceneRender::environment_get_ssil_intensity(p_arg1); }
	static float environment_get_ssil_sharpness(RID p_arg1) { return RendererSceneRender::environment_get_ssil_sharpness(p_arg1); }
	static float environment_get_ssil_normal_rejection(RID p_arg1) { return RendererSceneRender::environment_get_ssil_normal_rejection(p_arg1); }

	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_arg1, bool p_arg2, float p_arg3, int p_arg4, float p_arg5, float p_arg6) { if (data && data->scene_render) { data->scene_render->environment_set_ssil_quality(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6); } }

	// SDFGI

	static void environment_set_sdfgi(RID p_arg1, bool p_arg2, int p_arg3, float p_arg4, RSE::EnvironmentSDFGIYScale p_arg5, bool p_arg6, float p_arg7, bool p_arg8, float p_arg9, float p_arg10, float p_arg11) { RendererSceneRender::environment_set_sdfgi(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7, p_arg8, p_arg9, p_arg10, p_arg11); }

	static bool environment_get_sdfgi_enabled(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_enabled(p_arg1); }
	static int environment_get_sdfgi_cascades(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_cascades(p_arg1); }
	static float environment_get_sdfgi_min_cell_size(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_min_cell_size(p_arg1); }
	static bool environment_get_sdfgi_use_occlusion(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_use_occlusion(p_arg1); }
	static float environment_get_sdfgi_bounce_feedback(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_bounce_feedback(p_arg1); }
	static bool environment_get_sdfgi_read_sky_light(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_read_sky_light(p_arg1); }
	static float environment_get_sdfgi_energy(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_energy(p_arg1); }
	static float environment_get_sdfgi_normal_bias(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_normal_bias(p_arg1); }
	static float environment_get_sdfgi_probe_bias(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_probe_bias(p_arg1); }
	static RSE::EnvironmentSDFGIYScale environment_get_sdfgi_y_scale(RID p_arg1) { return RendererSceneRender::environment_get_sdfgi_y_scale(p_arg1); }

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_sdfgi_ray_count(p_arg1); } }
	static void environment_set_sdfgi_frames_to_converge(RSE::EnvironmentSDFGIFramesToConverge p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_sdfgi_frames_to_converge(p_arg1); } }
	static void environment_set_sdfgi_frames_to_update_light(RSE::EnvironmentSDFGIFramesToUpdateLight p_arg1) { if (data && data->scene_render) { data->scene_render->environment_set_sdfgi_frames_to_update_light(p_arg1); } }

	// Adjustment
	static void environment_set_adjustment(RID p_arg1, bool p_arg2, float p_arg3, float p_arg4, float p_arg5, bool p_arg6, RID p_arg7) { RendererSceneRender::environment_set_adjustment(p_arg1, p_arg2, p_arg3, p_arg4, p_arg5, p_arg6, p_arg7); }

	static bool environment_get_adjustments_enabled(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_enabled(p_arg1); }
	static float environment_get_adjustments_brightness(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_brightness(p_arg1); }

	static float environment_get_adjustments_contrast(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_contrast(p_arg1); }
	static float environment_get_adjustments_saturation(RID p_arg1) { return RendererSceneRender::environment_get_adjustments_saturation(p_arg1); }
	static bool environment_get_use_1d_color_correction(RID p_arg1) { return RendererSceneRender::environment_get_use_1d_color_correction(p_arg1); }
	static RID environment_get_color_correction(RID p_arg1) { return RendererSceneRender::environment_get_color_correction(p_arg1); }

	static Ref<Image> environment_bake_panorama(RID p_arg1, bool p_arg2, const Size2i& p_arg3) { ERR_FAIL_COND_V(!data || !data->scene_render, Ref<Image>()); return data->scene_render->environment_bake_panorama(p_arg1, p_arg2, p_arg3); }

	static void screen_space_roughness_limiter_set_active(bool p_arg1, float p_arg2, float p_arg3) { if (data && data->scene_render) { data->scene_render->screen_space_roughness_limiter_set_active(p_arg1, p_arg2, p_arg3); } }
	static void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_arg1) { if (data && data->scene_render) { data->scene_render->sub_surface_scattering_set_quality(p_arg1); } }
	static void sub_surface_scattering_set_scale(float p_arg1, float p_arg2) { if (data && data->scene_render) { data->scene_render->sub_surface_scattering_set_scale(p_arg1, p_arg2); } }

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_arg1) { if (data && data->scene_render) { data->scene_render->positional_soft_shadow_filter_set_quality(p_arg1); } }
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_arg1) { if (data && data->scene_render) { data->scene_render->directional_soft_shadow_filter_set_quality(p_arg1); } }

	static void sdfgi_set_debug_probe_select(const Vector3& p_arg1, const Vector3& p_arg2) { if (data && data->scene_render) { data->scene_render->sdfgi_set_debug_probe_select(p_arg1, p_arg2); } }

	/* Render Buffers */

	static Ref<RenderSceneBuffers> render_buffers_create() { ERR_FAIL_COND_V(!data || !data->scene_render, Ref<RenderSceneBuffers>()); return data->scene_render->render_buffers_create(); }
	static void gi_set_use_half_resolution(bool p_arg1) { if (data && data->scene_render) { data->scene_render->gi_set_use_half_resolution(p_arg1); } }

	/* Misc */
	static void set_debug_draw_mode(RSE::ViewportDebugDraw p_arg1) { if (data && data->scene_render) { data->scene_render->set_debug_draw_mode(p_arg1); } }

	static void decals_set_filter(RSE::DecalFilter p_arg1) { if (data && data->scene_render) { data->scene_render->decals_set_filter(p_arg1); } }
	static void light_projectors_set_filter(RSE::LightProjectorFilter p_arg1) { if (data && data->scene_render) { data->scene_render->light_projectors_set_filter(p_arg1); } }
	static void lightmaps_set_bicubic_filter(bool p_arg1) { if (data && data->scene_render) { data->scene_render->lightmaps_set_bicubic_filter(p_arg1); } }
	static void material_set_use_debanding(bool p_arg1) { if (data && data->scene_render) { data->scene_render->material_set_use_debanding(p_arg1); } }

	static void update();

	static bool free(RID p_rid);

	static void set_scene_render(RendererSceneRender* p_scene_render);

	static void update_visibility_notifiers();

	/* INTERPOLATION */

	static void update_interpolation_tick(bool p_process = true);
	static void update_interpolation_frame(bool p_process = true);
	static void set_physics_interpolation_enabled(bool p_enabled);

	RendererSceneCull() = delete;
	RendererSceneCull(const RendererSceneCull&) = delete;
	RendererSceneCull& operator=(const RendererSceneCull&) = delete;
	~RendererSceneCull() = delete;
};


