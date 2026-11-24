/**************************************************************************/
/*  renderer.h                                                            */
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

#include "core/io/image.h"
#include "core/os/os.h"
#include "core/templates/local_vector.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "servers/display/display_server.h"
// --- Inlined/Folded from servers/rendering/environment/renderer_fog.h ---
#include "core/types.h"
#include "core/math/plane.h"
#include "core/templates/hashfuncs.h"
#include "scene/resources/texture.h"

class RendererFog
{
public:
	/* FOG VOLUMES FUNCTION POINTERS */
	inline static bool (*owns_fog_volume)(RID p_rid) = nullptr;

	inline static RID (*fog_volume_allocate)() = nullptr;
	inline static void (*fog_volume_initialize)(RID p_rid) = nullptr;
	inline static void (*fog_volume_free)(RID p_rid) = nullptr;

	inline static void (*fog_volume_set_shape)(RID p_fog_volume, RSE::FogVolumeShape p_shape) = nullptr;
	inline static void (*fog_volume_set_size)(RID p_fog_volume, const Vector3 &p_size) = nullptr;
	inline static void (*fog_volume_set_material)(RID p_fog_volume, RID p_material) = nullptr;
	inline static AABB (*fog_volume_get_aabb)(RID p_fog_volume) = nullptr;
	inline static RSE::FogVolumeShape (*fog_volume_get_shape)(RID p_fog_volume) = nullptr;

	/* BIND TEMPLATE */
	template <typename T>
	static void bind()
	{
		owns_fog_volume = T::owns_fog_volume;

		fog_volume_allocate = T::fog_volume_allocate;
		fog_volume_initialize = T::fog_volume_initialize;
		fog_volume_free = T::fog_volume_free;

		fog_volume_set_shape = T::fog_volume_set_shape;
		fog_volume_set_size = T::fog_volume_set_size;
		fog_volume_set_material = T::fog_volume_set_material;
		fog_volume_get_aabb = T::fog_volume_get_aabb;
		fog_volume_get_shape = T::fog_volume_get_shape;
	}
};
// --- Folded from servers/rendering/environment/renderer_gi.h ---
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "scene/resources/texture.h"

class RendererGI
{
public:
	/* VOXEL GI FUNCTION POINTERS */
	inline static bool (*owns_voxel_gi)(RID p_rid) = nullptr;
	inline static RID (*voxel_gi_allocate)() = nullptr;
	inline static void (*voxel_gi_free)(RID p_voxel_gi) = nullptr;
	inline static void (*voxel_gi_initialize)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_allocate_data)(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts) = nullptr;

	inline static AABB (*voxel_gi_get_bounds)(RID p_voxel_gi) = nullptr;
	inline static Vector3i (*voxel_gi_get_octree_size)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_octree_cells)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_data_cells)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_distance_field)(RID p_voxel_gi) = nullptr;
	inline static Vector<int> (*voxel_gi_get_level_counts)(RID p_voxel_gi) = nullptr;
	inline static Transform3D (*voxel_gi_get_to_cell_xform)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_dynamic_range)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_dynamic_range)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_propagation)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_propagation)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_energy)(RID p_voxel_gi, float p_energy) = nullptr;
	inline static float (*voxel_gi_get_energy)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_baked_exposure_normalization)(
		RID p_voxel_gi, float p_baked_exposure) = nullptr;
	inline static float (*voxel_gi_get_baked_exposure_normalization)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_bias)(RID p_voxel_gi, float p_bias) = nullptr;
	inline static float (*voxel_gi_get_bias)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_normal_bias)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_normal_bias)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_interior)(RID p_voxel_gi, bool p_enable) = nullptr;
	inline static bool (*voxel_gi_is_interior)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_use_two_bounces)(RID p_voxel_gi, bool p_enable) = nullptr;
	inline static bool (*voxel_gi_is_using_two_bounces)(RID p_voxel_gi) = nullptr;

	inline static uint32_t (*voxel_gi_get_version)(RID p_probe) = nullptr;

	/* SDFGI FUNCTION POINTERS */
	inline static void (*sdfgi_reset)() = nullptr;

	/* BIND TEMPLATE */
	template <typename T> static void bind()
	{
		owns_voxel_gi = T::owns_voxel_gi;
		voxel_gi_allocate = T::voxel_gi_allocate;
		voxel_gi_free = T::voxel_gi_free;
		voxel_gi_initialize = T::voxel_gi_initialize;
		voxel_gi_allocate_data = T::voxel_gi_allocate_data;
		voxel_gi_get_bounds = T::voxel_gi_get_bounds;
		voxel_gi_get_octree_size = T::voxel_gi_get_octree_size;
		voxel_gi_get_octree_cells = T::voxel_gi_get_octree_cells;
		voxel_gi_get_data_cells = T::voxel_gi_get_data_cells;
		voxel_gi_get_distance_field = T::voxel_gi_get_distance_field;
		voxel_gi_get_level_counts = T::voxel_gi_get_level_counts;
		voxel_gi_get_to_cell_xform = T::voxel_gi_get_to_cell_xform;
		voxel_gi_set_dynamic_range = T::voxel_gi_set_dynamic_range;
		voxel_gi_get_dynamic_range = T::voxel_gi_get_dynamic_range;
		voxel_gi_set_propagation = T::voxel_gi_set_propagation;
		voxel_gi_get_propagation = T::voxel_gi_get_propagation;
		voxel_gi_set_energy = T::voxel_gi_set_energy;
		voxel_gi_get_energy = T::voxel_gi_get_energy;
		voxel_gi_set_baked_exposure_normalization = T::voxel_gi_set_baked_exposure_normalization;
		voxel_gi_get_baked_exposure_normalization = T::voxel_gi_get_baked_exposure_normalization;
		voxel_gi_set_bias = T::voxel_gi_set_bias;
		voxel_gi_get_bias = T::voxel_gi_get_bias;
		voxel_gi_set_normal_bias = T::voxel_gi_set_normal_bias;
		voxel_gi_get_normal_bias = T::voxel_gi_get_normal_bias;
		voxel_gi_set_interior = T::voxel_gi_set_interior;
		voxel_gi_is_interior = T::voxel_gi_is_interior;
		voxel_gi_set_use_two_bounces = T::voxel_gi_set_use_two_bounces;
		voxel_gi_is_using_two_bounces = T::voxel_gi_is_using_two_bounces;
		voxel_gi_get_version = T::voxel_gi_get_version;
		sdfgi_reset = T::sdfgi_reset;
	}
};

// --- End servers/rendering/environment/renderer_gi.h ---
// --- Folded from servers/rendering/renderer_compositor.h ---
#include "core/io/image.h"
#include "servers/display/display_server.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"
// --- Folded from servers/rendering/storage/utilities.h ---
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/list.h"
#include "core/templates/hashfuncs.h"
#include "servers/rendering/rendering_device.h"
#include "scene/resources/texture.h"

class DependencyTracker;

class Dependency
{
public:
	enum DependencyChangedNotification
	{
		DEPENDENCY_CHANGED_AABB,
		DEPENDENCY_CHANGED_MATERIAL,
		DEPENDENCY_CHANGED_MESH,
		DEPENDENCY_CHANGED_MULTIMESH,
		DEPENDENCY_CHANGED_MULTIMESH_VISIBLE_INSTANCES,
		DEPENDENCY_CHANGED_PARTICLES,
		DEPENDENCY_CHANGED_PARTICLES_INSTANCES,
		DEPENDENCY_CHANGED_DECAL,
		DEPENDENCY_CHANGED_SKELETON_DATA,
		DEPENDENCY_CHANGED_SKELETON_BONES,
		DEPENDENCY_CHANGED_LIGHT,
		DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR,
		DEPENDENCY_CHANGED_REFLECTION_PROBE,
		DEPENDENCY_CHANGED_CULL_MASK,
	};

	void changed_notify(DependencyChangedNotification p_notification);
	void deleted_notify(const RID& p_rid);

	~Dependency();

private:
	friend class DependencyTracker;
	HashMap<DependencyTracker*, uint32_t> instances;
};

class DependencyTracker
{
public:
	void* userdata = nullptr;
	typedef void (*ChangedCallback)(Dependency::DependencyChangedNotification, DependencyTracker*);
	typedef void (*DeletedCallback)(const RID&, DependencyTracker*);

	ChangedCallback changed_callback = nullptr;
	DeletedCallback deleted_callback = nullptr;

	void update_begin()
	{ // call before updating dependencies
		instance_version++;
	}

	void update_dependency(Dependency* p_dependency)
	{ // called internally, can't be used directly, use update functions in Storage
		dependencies.insert(p_dependency);
		p_dependency->instances[this] = instance_version;
	}

	void update_end()
	{ // call after updating dependencies
		List<Pair<Dependency*, DependencyTracker*>> to_clean_up;

		for (Dependency* E : dependencies) {
			Dependency* dep = E;
			HashMap<DependencyTracker*, uint32_t>::Iterator F = dep->instances.find(this);
			ERR_CONTINUE(!F);
			if (F->value != instance_version) {
				Pair<Dependency*, DependencyTracker*> p;
				p.first = dep;
				p.second = F->key;
				to_clean_up.push_back(p);
			}
		}

		while (to_clean_up.size()) {
			to_clean_up.front()->get().first->instances.erase(to_clean_up.front()->get().second);
			dependencies.erase(to_clean_up.front()->get().first);
			to_clean_up.pop_front();
		}
	}

	void clear()
	{ // clear all dependencies
		for (Dependency* E : dependencies) {
			Dependency* dep = E;
			dep->instances.erase(this);
		}
		dependencies.clear();
	}

	~DependencyTracker() { clear(); }

private:
	friend class Dependency;
	uint32_t instance_version = 0;
	HashSet<Dependency*> dependencies;
};

class DependencyTracker;

class RendererUtilities
{
public:
	inline static bool capturing_timestamps = false;

	inline static RID (*visibility_notifier_allocate)() = nullptr;
	inline static void (*visibility_notifier_initialize)(RID) = nullptr;
	inline static void (*visibility_notifier_free)(RID) = nullptr;
	inline static void (*visibility_notifier_set_aabb)(RID, const AABB&) = nullptr;
	inline static AABB (*visibility_notifier_get_aabb)(RID) = nullptr;
	inline static void (*visibility_notifier_call)(RID, bool, bool) = nullptr;

	inline static bool (*free)(RID) = nullptr;
	inline static RSE::InstanceType (*get_base_type)(RID) = nullptr;
	inline static void (*base_update_dependency)(RID, DependencyTracker*) = nullptr;

	inline static void (*capture_timestamps_begin)() = nullptr;
	inline static void (*capture_timestamp)(const String&) = nullptr;
	inline static uint32_t (*get_captured_timestamps_count)() = nullptr;
	inline static uint64_t (*get_captured_timestamps_frame)() = nullptr;
	inline static uint64_t (*get_captured_timestamp_gpu_time)(uint32_t) = nullptr;
	inline static uint64_t (*get_captured_timestamp_cpu_time)(uint32_t) = nullptr;
	inline static String (*get_captured_timestamp_name)(uint32_t) = nullptr;

	inline static void (*update_dirty_resources)() = nullptr;
	inline static void (*set_debug_generate_wireframes)(bool) = nullptr;
	inline static bool (*has_os_feature)(const String&) = nullptr;
	inline static void (*update_memory_info)() = nullptr;

	inline static uint64_t (*get_rendering_info)(RSE::RenderingInfo) = nullptr;
	inline static String (*get_video_adapter_name)() = nullptr;
	inline static String (*get_video_adapter_vendor)() = nullptr;
	inline static RenderingDeviceEnums::DeviceType (*get_video_adapter_type)() = nullptr;
	inline static String (*get_video_adapter_api_version)() = nullptr;

	inline static Size2i (*get_maximum_viewport_size)() = nullptr;
	inline static uint32_t (*get_maximum_shader_varyings)() = nullptr;
	inline static uint64_t (*get_maximum_uniform_buffer_size)() = nullptr;

	RendererUtilities() = delete;
	RendererUtilities(const RendererUtilities&) = delete;
	RendererUtilities& operator=(const RendererUtilities&) = delete;
	~RendererUtilities() = delete;
};

#define RENDER_TIMESTAMP(m_text)                                                                   \
	if (RendererUtilities::capture_timestamp) {                                                    \
		RendererUtilities::capture_timestamp(m_text);                                              \
	}
// --- End servers/rendering/storage/utilities.h ---

class RendererCanvasRender;
class RendererSceneRender;

class RendererFog;
class RendererGI;

class RendererCompositor final
{
private:
    static inline bool xr_enabled = false;

public:
    static inline bool back_end = false;
    static inline bool low_end = false;

    static Error create();
    static bool is_xr_enabled();

    _ALWAYS_INLINE_ static bool is_low_end() { return low_end; }

    /* Static Function Pointer Dispatch Interface */

    static inline RendererCanvasRender* (*get_canvas)() = nullptr;

    static inline void (*set_boot_image_with_stretch)(
        const Ref<Image>&, const Color&, RSE::SplashStretchMode, bool) = nullptr;
    static inline void (*initialize)() = nullptr;
    static inline bool (*is_initialized)() = nullptr;
    static inline void (*begin_frame)(double) = nullptr;
    static inline void (*blit_render_targets_to_screen)(
        DisplayServerEnums::WindowID, const RenderingServerTypes::BlitToScreen*, int) = nullptr;

    static inline bool (*is_opengl)() = nullptr;
    static inline void (*gl_end_frame)(bool) = nullptr;
    static inline void (*end_frame)(bool) = nullptr;
    static inline void (*finalize)() = nullptr;

    static inline uint64_t (*get_frame_number)() = nullptr;
    static inline double (*get_frame_delta_time)() = nullptr;
    static inline double (*get_total_time)() = nullptr;
    static inline bool (*can_create_resources_async)() = nullptr;

    template <typename Backend> static void bind_compositor()
    {
        Backend::bind_utilities();
        Backend::bind_texture_storage();
        Backend::bind_material_storage();
        Backend::bind_mesh_storage();
        Backend::bind_light_storage();
        Backend::bind_particles_storage();
		Backend::bind_gi();
		Backend::bind_fog();
		Backend::bind_canvas();

        set_boot_image_with_stretch = &Backend::set_boot_image_with_stretch;
        initialize = &Backend::initialize;
        begin_frame = &Backend::begin_frame;
        blit_render_targets_to_screen = &Backend::blit_render_targets_to_screen;

        is_opengl = &Backend::is_opengl;
        gl_end_frame = &Backend::gl_end_frame;
        end_frame = &Backend::end_frame;
        finalize = &Backend::finalize;

        get_frame_number = &Backend::get_frame_number;
        get_frame_delta_time = &Backend::get_frame_delta_time;
        get_total_time = &Backend::get_total_time;
        can_create_resources_async = &Backend::can_create_resources_async;
    }

    RendererCompositor() = delete;
    RendererCompositor(const RendererCompositor&) = delete;
    RendererCompositor& operator=(const RendererCompositor&) = delete;
    ~RendererCompositor() = delete;
};

using RC = RendererCompositor;
// --- End servers/rendering/renderer_compositor.h ---
#include "servers/rendering/rendering_device.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"
// --- Folded from servers/rendering/storage/camera_attributes_storage.h ---
#include "core/types.h"
#include "scene/resources/texture.h"

class RendererCameraAttributes final
{
public:
	RendererCameraAttributes() = delete;
	~RendererCameraAttributes() = delete;
	RendererCameraAttributes(const RendererCameraAttributes&) = delete;
	RendererCameraAttributes& operator=(const RendererCameraAttributes&) = delete;

	struct CameraAttributes
	{
		float exposure_multiplier = 1.0;
		float exposure_normalization = 1.0;
		float exposure_sensitivity = 100.0; // In ISO.

		bool use_auto_exposure = false;
		float auto_exposure_min_sensitivity = 50.0;
		float auto_exposure_max_sensitivity = 800.0;
		float auto_exposure_adjust_speed = 1.0;
		float auto_exposure_scale = 1.0;
		uint64_t auto_exposure_version = 0;

		bool dof_blur_far_enabled = false;
		float dof_blur_far_distance = 10;
		float dof_blur_far_transition = 5;
		bool dof_blur_near_enabled = false;
		float dof_blur_near_distance = 2;
		float dof_blur_near_transition = 1;
		float dof_blur_amount = 0.1;
	};

private:
	static inline bool _initialized = false;

	static inline RSE::DOFBlurQuality dof_blur_quality = RSE::DOF_BLUR_QUALITY_MEDIUM;
	static inline RSE::DOFBokehShape dof_blur_bokeh_shape = RSE::DOF_BOKEH_HEXAGON;
	static inline bool dof_blur_use_jitter = false;
	static inline uint64_t auto_exposure_counter = 2;

	static inline RID_Owner<CameraAttributes, true> camera_attributes_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static CameraAttributes* get_camera_attributes(RID p_rid)
	{
		return camera_attributes_owner.get_or_null(p_rid);
	}
	static bool owns_camera_attributes(RID p_rid) { return camera_attributes_owner.owns(p_rid); }

	static RID camera_attributes_allocate();
	static void camera_attributes_initialize(RID p_rid);
	static void camera_attributes_free(RID p_rid);

	static void camera_attributes_set_dof_blur_quality(
		RSE::DOFBlurQuality p_quality, bool p_use_jitter);
	static void camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape);

	static void camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
		float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
		float p_near_transition, float p_amount);
	static bool camera_attributes_get_dof_far_enabled(RID p_camera_attributes);
	static float camera_attributes_get_dof_far_distance(RID p_camera_attributes);
	static float camera_attributes_get_dof_far_transition(RID p_camera_attributes);
	static bool camera_attributes_get_dof_near_enabled(RID p_camera_attributes);
	static float camera_attributes_get_dof_near_distance(RID p_camera_attributes);
	static float camera_attributes_get_dof_near_transition(RID p_camera_attributes);
	static float camera_attributes_get_dof_blur_amount(RID p_camera_attributes);

	_FORCE_INLINE_ static bool camera_attributes_uses_dof(RID p_camera_attributes)
	{
		CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);

		return cam_attributes &&
			   (cam_attributes->dof_blur_near_enabled || cam_attributes->dof_blur_far_enabled) &&
			   cam_attributes->dof_blur_amount > 0.0;
	}

	static void camera_attributes_set_exposure(
		RID p_camera_attributes, float p_multiplier, float p_exposure_normalization);
	static float camera_attributes_get_exposure_normalization_factor(RID p_camera_attributes);

	static void camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
		float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale);
	static float camera_attributes_get_auto_exposure_min_sensitivity(RID p_camera_attributes);
	static float camera_attributes_get_auto_exposure_max_sensitivity(RID p_camera_attributes);
	static float camera_attributes_get_auto_exposure_adjust_speed(RID p_camera_attributes);
	static float camera_attributes_get_auto_exposure_scale(RID p_camera_attributes);
	static uint64_t camera_attributes_get_auto_exposure_version(RID p_camera_attributes);

	_FORCE_INLINE_ static bool camera_attributes_uses_auto_exposure(RID p_camera_attributes)
	{
		CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);

		return cam_attributes && cam_attributes->use_auto_exposure;
	}

	_FORCE_INLINE_ static RSE::DOFBlurQuality camera_attributes_get_dof_blur_quality()
	{
		return dof_blur_quality;
	}

	_FORCE_INLINE_ static RSE::DOFBokehShape camera_attributes_get_dof_blur_bokeh_shape()
	{
		return dof_blur_bokeh_shape;
	}

	_FORCE_INLINE_ static bool camera_attributes_get_dof_blur_use_jitter()
	{
		return dof_blur_use_jitter;
	}
};

// --- End servers/rendering/storage/camera_attributes_storage.h ---
// --- Inlined/Folded from servers/rendering/storage/light_storage.h ---
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/math/plane.h"
#include "core/templates/hashfuncs.h"
#include "scene/resources/texture.h"
// --- Folded from servers/rendering/renderer_scene_render.h ---
#include "core/types.h"
// --- Folded from servers/rendering/renderer_geometry_instance.h ---
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/templates/hashfuncs.h"


// Base implementation of RenderGeometryInstance shared by internal renderers.
class RenderGeometryInstance {
public:
	// setup
	uint32_t base_flags = 0;
	uint32_t flags_cache = 0;

	// used during rendering
	float depth = 0;

	RID mesh_instance;

	Transform3D transform;
	bool mirror = false;
	AABB transformed_aabb;
	bool non_uniform_scale = false;
	float lod_model_scale = 1.0;
	float lod_bias = 0.0;
	float sorting_offset = 0.0;
	bool use_aabb_center = true;

	uint32_t layer_mask = 1;

	bool fade_near = false;
	float fade_near_begin = 0;
	float fade_near_end = 0;
	bool fade_far = false;
	float fade_far_begin = 0;
	float fade_far_end = 0;

	float parent_fade_alpha = 1.0;
	float force_alpha = 1.0;

	int32_t shader_uniforms_offset = -1;

	struct Data {
		//data used less often goes into regular heap
		RID base;
		RSE::InstanceType base_type;

		RID skeleton;
		Vector<RID> surface_materials;
		RID material_override;
		RID material_overlay;
		AABB aabb;

		bool use_baked_light = false;
		bool use_dynamic_gi = false;
		bool cast_double_sided_shadows = false;
		bool dirty_dependencies = false;

		DependencyTracker dependency_tracker;
	};

	Data *data = nullptr;

	virtual ~RenderGeometryInstance() {}

	virtual void _mark_dirty() {}

	void set_skeleton(RID p_skeleton);
	void set_material_override(RID p_override);
	void set_material_overlay(RID p_overlay);
	void set_surface_materials(const Vector<RID> &p_materials);
	void set_mesh_instance(RID p_mesh_instance);
	virtual void set_transform(const Transform3D &p_transform, const AABB &p_aabb, const AABB &p_transformed_aabb);
	void set_pivot_data(float p_sorting_offset, bool p_use_aabb_center);
	void set_lod_bias(float p_lod_bias);
	void set_layer_mask(uint32_t p_layer_mask);
	void set_fade_range(bool p_enable_near, float p_near_begin, float p_near_end, bool p_enable_far, float p_far_begin, float p_far_end);
	void set_parent_fade_alpha(float p_alpha);
	void set_transparency(float p_transparency);
	void set_use_baked_light(bool p_enable);
	void set_use_dynamic_gi(bool p_enable);
	virtual void set_use_lightmap(RID p_lightmap_instance, const Rect2 &p_lightmap_uv_scale, int p_lightmap_slice_index) {}
	virtual void set_lightmap_capture(const Color *p_sh9) {}
	void set_instance_shader_uniforms_offset(int32_t p_offset);
	void set_cast_double_sided_shadows(bool p_enable);

	virtual void reset_motion_vectors();

	Transform3D get_transform();
	AABB get_aabb();

	virtual void clear_light_instances() {}
	virtual void pair_light_instance(const RID p_light_instance, RSE::LightType light_type, uint32_t placement_idx) {}
	virtual void pair_reflection_probe_instances(const RID *p_reflection_probe_instances, uint32_t p_reflection_probe_instance_count) {}
	virtual void pair_decal_instances(const RID *p_decal_instances, uint32_t p_decal_instance_count) {}
	virtual void pair_voxel_gi_instances(const RID *p_voxel_gi_instances, uint32_t p_voxel_gi_instance_count) {}

	virtual void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) {}
};

// --- End servers/rendering/renderer_geometry_instance.h ---
#include "servers/rendering/rendering_device.h"
// --- Folded from servers/rendering/storage/compositor_storage.h ---
#include "core/types.h"
#include "scene/resources/texture.h"

class RendererCompositorStorage final
{
public:
	RendererCompositorStorage() = delete;
	~RendererCompositorStorage() = delete;
	RendererCompositorStorage(const RendererCompositorStorage&) = delete;
	RendererCompositorStorage& operator=(const RendererCompositorStorage&) = delete;

private:
	static inline bool _initialized = false;
	static inline int num_compositor_effects_with_motion_vectors = 0;

	// Compositor effect
	struct CompositorEffect
	{
		bool is_enabled = true;
		RSE::CompositorEffectCallbackType callback_type;

		uint32_t flags = {};
	};

	static inline RID_Owner<CompositorEffect, true> compositor_effects_owner;

	// Compositor
	struct Compositor
	{
		// Compositor effects
		Vector<RID> compositor_effects;
	};

	static inline RID_Owner<Compositor, true> compositor_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static int get_num_compositor_effects_with_motion_vectors()
	{
		return num_compositor_effects_with_motion_vectors;
	}

	// Compositor effect
	static RID compositor_effect_allocate();
	static void compositor_effect_initialize(RID p_rid);
	static void compositor_effect_free(RID p_rid);

	static bool is_compositor_effect(RID p_effect)
	{
		return compositor_effects_owner.owns(p_effect);
	}

	static void compositor_effect_set_enabled(RID p_effect, bool p_enabled);
	static bool compositor_effect_get_enabled(RID p_effect);

	static RSE::CompositorEffectCallbackType compositor_effect_get_callback_type(RID p_effect);

	static void compositor_effect_set_flag(
		RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set);
	static bool compositor_effect_get_flag(RID p_effect, RSE::CompositorEffectFlags p_flag);

	// Compositor
	static RID compositor_allocate();
	static void compositor_initialize(RID p_rid);
	static void compositor_free(RID p_rid);

	static bool is_compositor(RID p_compositor) { return compositor_owner.owns(p_compositor); }

	static void compositor_set_compositor_effects(RID p_compositor, const Vector<RID>& p_effects);
	static Vector<RID> compositor_get_compositor_effects(RID p_compositor,
		RSE::CompositorEffectCallbackType p_callback_type =
			RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY,
		bool p_enabled_only = true);
};

// --- End servers/rendering/storage/compositor_storage.h ---
// --- Folded from servers/rendering/storage/environment_storage.h ---
#include "core/types.h"
#include "scene/resources/texture.h"

class RendererEnvironmentStorage final
{
public:
	RendererEnvironmentStorage() = delete;
	~RendererEnvironmentStorage() = delete;
	RendererEnvironmentStorage(const RendererEnvironmentStorage&) = delete;
	RendererEnvironmentStorage& operator=(const RendererEnvironmentStorage&) = delete;

	union TonemapParameters
	{
		// Shader vec4:
		float tonemapper_params[4];

		// Reinhard:
		struct
		{
			float white_squared;
		};

		// Filmic and ACES:
		struct
		{
			float white_tonemapped;
		};

		// AgX:
		struct
		{
			float awp_contrast;
			float awp_toe_a;
			float awp_slope;
			float awp_w;
		};
	};

private:
	static inline bool _initialized = false;

	// Environment
	struct Environment
	{
		// Note, we capture and store all environment parameters received from Godot here.
		// Not all renderers support all effects and should just ignore the bits they don't support.

		// Background
		RSE::EnvironmentBG background = RSE::ENV_BG_CLEAR_COLOR;
		RID sky;
		float sky_custom_fov = 0.0;
		Basis sky_orientation;
		Color bg_color;
		float bg_energy_multiplier = 1.0;
		float bg_intensity = 1.0; // Measured in nits or candela/m^2. Default to 1.0 so this doesn't impact rendering when Physical Light Units disabled.
		int canvas_max_layer = 0;
		RSE::EnvironmentAmbientSource ambient_source = RSE::ENV_AMBIENT_SOURCE_BG;
		Color ambient_light;
		float ambient_light_energy = 1.0;
		float ambient_sky_contribution = 1.0;
		RSE::EnvironmentReflectionSource reflection_source = RSE::ENV_REFLECTION_SOURCE_BG;
		int camera_feed_id = 0;

		// Tonemap
		RSE::EnvironmentToneMapper tone_mapper;
		float exposure = 1.0;
		float white = 1.0;
		float tonemap_agx_contrast = 1.25; // Default to approximately Blender's AgX contrast
		float max_value = 1.0;

		// Fog
		bool fog_enabled = false;
		RSE::EnvironmentFogMode fog_mode = RSE::EnvironmentFogMode::ENV_FOG_MODE_EXPONENTIAL;
		Color fog_light_color = Color(0.518, 0.553, 0.608);
		float fog_light_energy = 1.0;
		float fog_sun_scatter = 0.0;
		float fog_density = 0.01;
		float fog_sky_affect = 1.0;
		float fog_height = 0.0;
		float fog_height_density = 0.0; //can be negative to invert effect
		float fog_aerial_perspective = 0.0;

		// Depth Fog
		float fog_depth_curve = 1.0;
		float fog_depth_begin = 10.0;
		float fog_depth_end = 100.0;

		// Volumetric Fog
		bool volumetric_fog_enabled = false;
		float volumetric_fog_density = 0.01;
		Color volumetric_fog_scattering = Color(1, 1, 1);
		Color volumetric_fog_emission = Color(0, 0, 0);
		float volumetric_fog_emission_energy = 0.0;
		float volumetric_fog_anisotropy = 0.2;
		float volumetric_fog_length = 64.0;
		float volumetric_fog_detail_spread = 2.0;
		float volumetric_fog_gi_inject = 1.0;
		float volumetric_fog_ambient_inject = 0.0;
		float volumetric_fog_sky_affect = 1.0;
		bool volumetric_fog_temporal_reprojection = true;
		float volumetric_fog_temporal_reprojection_amount = 0.9;

		// Glow
		bool glow_enabled = false;
		Vector<float> glow_levels;
		float glow_intensity = 0.3;
		float glow_strength = 1.0;
		float glow_bloom = 0.0;
		float glow_mix = 0.01;
		RSE::EnvironmentGlowBlendMode glow_blend_mode = RSE::ENV_GLOW_BLEND_MODE_SCREEN;
		float glow_hdr_bleed_threshold = 1.0;
		float glow_hdr_luminance_cap = 12.0;
		float glow_hdr_bleed_scale = 2.0;
		float glow_map_strength = 0.0f; // 1.0f in GLES3 ??
		RID glow_map;

		// SSR
		bool ssr_enabled = false;
		int ssr_max_steps = 64;
		float ssr_fade_in = 0.15;
		float ssr_fade_out = 2.0;
		float ssr_depth_tolerance = 0.5;

		// SSAO
		bool ssao_enabled = false;
		float ssao_radius = 1.0;
		float ssao_intensity = 2.0;
		float ssao_power = 1.5;
		float ssao_detail = 0.5;
		float ssao_horizon = 0.06;
		float ssao_sharpness = 0.98;
		float ssao_direct_light_affect = 0.0;
		float ssao_ao_channel_affect = 0.0;

		// SSIL
		bool ssil_enabled = false;
		float ssil_radius = 5.0;
		float ssil_intensity = 1.0;
		float ssil_sharpness = 0.98;
		float ssil_normal_rejection = 1.0;

		// SDFGI
		bool sdfgi_enabled = false;
		int sdfgi_cascades = 4;
		float sdfgi_min_cell_size = 0.2;
		bool sdfgi_use_occlusion = false;
		float sdfgi_bounce_feedback = 0.5;
		bool sdfgi_read_sky_light = true;
		float sdfgi_energy = 1.0;
		float sdfgi_normal_bias = 1.1;
		float sdfgi_probe_bias = 1.1;
		RSE::EnvironmentSDFGIYScale sdfgi_y_scale = RSE::ENV_SDFGI_Y_SCALE_75_PERCENT;

		// Adjustments
		bool adjustments_enabled = false;
		float adjustments_brightness = 1.0f;
		float adjustments_contrast = 1.0f;
		float adjustments_saturation = 1.0f;
		bool use_1d_color_correction = false;
		RID color_correction;
	};

	static inline RID_Owner<Environment, true> environment_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	// Environment
	static RID environment_allocate();
	static void environment_initialize(RID p_rid);
	static void environment_free(RID p_rid);

	static bool is_environment(RID p_environment) { return environment_owner.owns(p_environment); }

	// Background
	static void environment_set_background(RID p_env, RSE::EnvironmentBG p_bg);
	static void environment_set_sky(RID p_env, RID p_sky);
	static void environment_set_sky_custom_fov(RID p_env, float p_scale);
	static void environment_set_sky_orientation(RID p_env, const Basis& p_orientation);
	static void environment_set_bg_color(RID p_env, const Color& p_color);
	static void environment_set_bg_energy(RID p_env, float p_multiplier, float p_exposure_value);
	static void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	static void environment_set_ambient_light(RID p_env, const Color& p_color,
		RSE::EnvironmentAmbientSource p_ambient = RSE::ENV_AMBIENT_SOURCE_BG, float p_energy = 1.0,
		float p_sky_contribution = 0.0,
		RSE::EnvironmentReflectionSource p_reflection_source = RSE::ENV_REFLECTION_SOURCE_BG);
	static void environment_set_camera_feed_id(RID p_env, int p_camera_feed_id);
	static int environment_get_camera_feed_id(RID p_env);

	static RSE::EnvironmentBG environment_get_background(RID p_env);
	static RID environment_get_sky(RID p_env);
	static float environment_get_sky_custom_fov(RID p_env);
	static Basis environment_get_sky_orientation(RID p_env);
	static Color environment_get_bg_color(RID p_env);
	static float environment_get_bg_energy_multiplier(RID p_env);
	static float environment_get_bg_intensity(RID p_env);
	static int environment_get_canvas_max_layer(RID p_env);
	static RSE::EnvironmentAmbientSource environment_get_ambient_source(RID p_env);
	static Color environment_get_ambient_light(RID p_env);
	static float environment_get_ambient_light_energy(RID p_env);
	static float environment_get_ambient_sky_contribution(RID p_env);
	static RSE::EnvironmentReflectionSource environment_get_reflection_source(RID p_env);

	// Tonemap
	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static RSE::EnvironmentToneMapper environment_get_tone_mapper(RID p_env);
	static float environment_get_exposure(RID p_env);
	static float environment_get_white(RID p_env, bool p_limit_agx_white, float p_output_max_value);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static float environment_get_tonemap_agx_contrast(RID p_env);
	static TonemapParameters environment_get_tonemap_parameters(
		RID p_env, bool p_limit_agx_white, float p_output_max_value);

	// Fog
	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode);
	static bool environment_get_fog_enabled(RID p_env);
	static RSE::EnvironmentFogMode environment_get_fog_mode(RID p_env);
	static Color environment_get_fog_light_color(RID p_env);
	static float environment_get_fog_light_energy(RID p_env);
	static float environment_get_fog_sun_scatter(RID p_env);
	static float environment_get_fog_density(RID p_env);
	static float environment_get_fog_sky_affect(RID p_env);
	static float environment_get_fog_height(RID p_env);
	static float environment_get_fog_height_density(RID p_env);
	static float environment_get_fog_aerial_perspective(RID p_env);

	// Depth Fog
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);
	static float environment_get_fog_depth_curve(RID p_env);
	static float environment_get_fog_depth_begin(RID p_env);
	static float environment_get_fog_depth_end(RID p_env);

	// Volumetric Fog
	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static bool environment_get_volumetric_fog_enabled(RID p_env);
	static float environment_get_volumetric_fog_density(RID p_env);
	static Color environment_get_volumetric_fog_scattering(RID p_env);
	static Color environment_get_volumetric_fog_emission(RID p_env);
	static float environment_get_volumetric_fog_emission_energy(RID p_env);
	static float environment_get_volumetric_fog_anisotropy(RID p_env);
	static float environment_get_volumetric_fog_length(RID p_env);
	static float environment_get_volumetric_fog_detail_spread(RID p_env);
	static float environment_get_volumetric_fog_gi_inject(RID p_env);
	static float environment_get_volumetric_fog_sky_affect(RID p_env);
	static bool environment_get_volumetric_fog_temporal_reprojection(RID p_env);
	static float environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env);
	static float environment_get_volumetric_fog_ambient_inject(RID p_env);

	// GLOW
	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
		float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);
	static bool environment_get_glow_enabled(RID p_env);
	static Vector<float> environment_get_glow_levels(RID p_env);
	static float environment_get_glow_intensity(RID p_env);
	static float environment_get_glow_strength(RID p_env);
	static float environment_get_glow_bloom(RID p_env);
	static float environment_get_glow_mix(RID p_env);
	static RSE::EnvironmentGlowBlendMode environment_get_glow_blend_mode(RID p_env);
	static float environment_get_glow_hdr_bleed_threshold(RID p_env);
	static float environment_get_glow_hdr_luminance_cap(RID p_env);
	static float environment_get_glow_hdr_bleed_scale(RID p_env);
	static float environment_get_glow_map_strength(RID p_env);
	static RID environment_get_glow_map(RID p_env);

	// SSR
	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int,
		float p_fade_out, float p_depth_tolerance);
	static bool environment_get_ssr_enabled(RID p_env);
	static int environment_get_ssr_max_steps(RID p_env);
	static float environment_get_ssr_fade_in(RID p_env);
	static float environment_get_ssr_fade_out(RID p_env);
	static float environment_get_ssr_depth_tolerance(RID p_env);

	// SSAO
	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);
	static bool environment_get_ssao_enabled(RID p_env);
	static float environment_get_ssao_radius(RID p_env);
	static float environment_get_ssao_intensity(RID p_env);
	static float environment_get_ssao_power(RID p_env);
	static float environment_get_ssao_detail(RID p_env);
	static float environment_get_ssao_horizon(RID p_env);
	static float environment_get_ssao_sharpness(RID p_env);
	static float environment_get_ssao_direct_light_affect(RID p_env);
	static float environment_get_ssao_ao_channel_affect(RID p_env);

	// SSIL
	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);
	static bool environment_get_ssil_enabled(RID p_env);
	static float environment_get_ssil_radius(RID p_env);
	static float environment_get_ssil_intensity(RID p_env);
	static float environment_get_ssil_sharpness(RID p_env);
	static float environment_get_ssil_normal_rejection(RID p_env);

	// SDFGI
	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
		float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
		float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
		float p_probe_bias);
	static bool environment_get_sdfgi_enabled(RID p_env);
	static int environment_get_sdfgi_cascades(RID p_env);
	static float environment_get_sdfgi_min_cell_size(RID p_env);
	static bool environment_get_sdfgi_use_occlusion(RID p_env);
	static float environment_get_sdfgi_bounce_feedback(RID p_env);
	static bool environment_get_sdfgi_read_sky_light(RID p_env);
	static float environment_get_sdfgi_energy(RID p_env);
	static float environment_get_sdfgi_normal_bias(RID p_env);
	static float environment_get_sdfgi_probe_bias(RID p_env);
	static RSE::EnvironmentSDFGIYScale environment_get_sdfgi_y_scale(RID p_env);

	// Adjustment
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
		float p_contrast, float p_saturation, bool p_use_1d_color_correction,
		RID p_color_correction);
	static bool environment_get_adjustments_enabled(RID p_env);
	static float environment_get_adjustments_brightness(RID p_env);
	static float environment_get_adjustments_contrast(RID p_env);
	static float environment_get_adjustments_saturation(RID p_env);
	static bool environment_get_use_1d_color_correction(RID p_env);
	static RID environment_get_color_correction(RID p_env);
};

// --- End servers/rendering/storage/environment_storage.h ---
// --- Inlined/Folded from servers/rendering/storage/render_scene_buffers.h ---
#include "core/types.h"
#include "scene/resources/texture.h"

class RenderSceneBuffersConfiguration : public RefCounted
{
private:
	RID render_target;

	Size2i internal_size;
	Size2i target_size;
	uint32_t view_count = 1;

	RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	RSE::ViewportMSAA msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
	RSE::ViewportScreenSpaceAA screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
	RSE::ViewportAnisotropicFiltering anisotropic_filtering_level = RSE::VIEWPORT_ANISOTROPY_4X;

	float fsr_sharpness = 0.0;
	float texture_mipmap_bias = 0.0;
	bool use_taa = false;
	bool use_debanding = false;

protected:

public:
	RID get_render_target() const { return render_target; }

	void set_render_target(RID p_render_target) { render_target = p_render_target; }

	Size2i get_internal_size() const { return internal_size; }

	void set_internal_size(Size2i p_internal_size) { internal_size = p_internal_size; }

	Size2i get_target_size() const { return target_size; }

	void set_target_size(Size2i p_target_size) { target_size = p_target_size; }

	uint32_t get_view_count() const { return view_count; }

	void set_view_count(uint32_t p_view_count) { view_count = p_view_count; }

	RSE::ViewportScaling3DMode get_scaling_3d_mode() const { return scaling_3d_mode; }

	void set_scaling_3d_mode(RSE::ViewportScaling3DMode p_scaling_3d_mode)
	{
		scaling_3d_mode = p_scaling_3d_mode;
	}

	RSE::ViewportMSAA get_msaa_3d() const { return msaa_3d; }

	void set_msaa_3d(RSE::ViewportMSAA p_msaa_3d) { msaa_3d = p_msaa_3d; }

	RSE::ViewportScreenSpaceAA get_screen_space_aa() const { return screen_space_aa; }

	void set_screen_space_aa(RSE::ViewportScreenSpaceAA p_screen_space_aa)
	{
		screen_space_aa = p_screen_space_aa;
	}

	float get_fsr_sharpness() const { return fsr_sharpness; }

	void set_fsr_sharpness(float p_fsr_sharpness) { fsr_sharpness = p_fsr_sharpness; }

	float get_texture_mipmap_bias() const { return texture_mipmap_bias; }

	void set_texture_mipmap_bias(float p_texture_mipmap_bias)
	{
		texture_mipmap_bias = p_texture_mipmap_bias;
	}

	RSE::ViewportAnisotropicFiltering get_anisotropic_filtering_level() const
	{
		return anisotropic_filtering_level;
	}

	void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level)
	{
		anisotropic_filtering_level = p_anisotropic_filtering_level;
	}

	bool get_use_taa() const { return use_taa; }

	void set_use_taa(bool p_use_taa) { use_taa = p_use_taa; }

	bool get_use_debanding() const { return use_debanding; }

	void set_use_debanding(bool p_use_debanding) { use_debanding = p_use_debanding; }

	RenderSceneBuffersConfiguration() {}

	virtual ~RenderSceneBuffersConfiguration() {}
};

class RenderSceneBuffers : public RefCounted
{
protected:

public:
	RenderSceneBuffers() {}

	virtual ~RenderSceneBuffers() {}

	virtual void configure(const RenderSceneBuffersConfiguration* p_config) = 0;

	// for those settings that are unlikely to require buffers to be recreated, we'll add setters
	virtual void set_fsr_sharpness(float p_fsr_sharpness) = 0;
	virtual void set_texture_mipmap_bias(float p_texture_mipmap_bias) = 0;
	virtual void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level) = 0;
	virtual void set_use_debanding(bool p_use_debanding) = 0;
};

class RendererSceneRender
{
public:
	enum
	{
		MAX_DIRECTIONAL_LIGHTS = 8,
		MAX_DIRECTIONAL_LIGHT_CASCADES = 4,
		MAX_RENDER_VIEWS = 2
	};

	/* Geometry Instance */

	virtual RenderGeometryInstance* geometry_instance_create(RID p_base);
	virtual void geometry_instance_free(RenderGeometryInstance* p_geometry_instance);
	virtual uint32_t geometry_instance_get_pair_mask();

	/* Lights matched up to Geometry Instances */
	virtual uint32_t get_max_lights_total();
	virtual uint32_t get_max_lights_per_mesh();

	/* PIPELINES */

	virtual void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation);
	virtual uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	/* SDFGI UPDATE */

	virtual void sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_environment,
		const Vector3& p_world_position);
	virtual int sdfgi_get_pending_region_count(
		const Ref<RenderSceneBuffers>& p_render_buffers) const;
	virtual AABB sdfgi_get_pending_region_bounds(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const;
	virtual uint32_t sdfgi_get_pending_region_cascade(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const;

	/* SKY API */

	virtual RID sky_allocate();
	virtual void sky_initialize(RID p_rid);

	virtual void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	virtual void sky_set_mode(RID p_sky, RSE::SkyMode p_samples);
	virtual void sky_set_material(RID p_sky, RID p_material);
	virtual Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* COMPOSITOR EFFECT API */

	static RID compositor_effect_allocate();
	static void compositor_effect_initialize(RID p_rid);
	static void compositor_effect_free(RID p_rid);

	static bool is_compositor_effect(RID p_compositor);
	static void compositor_effect_set_enabled(RID p_compositor, bool p_enabled);
	static void compositor_effect_set_flag(
		RID p_compositor, RSE::CompositorEffectFlags p_flag, bool p_set);

	/* COMPOSITOR API */

	static RID compositor_allocate();
	static void compositor_initialize(RID p_rid);
	static void compositor_free(RID p_rid);

	static bool is_compositor(RID p_compositor);

	/* ENVIRONMENT API */

	static RID environment_allocate();
	static void environment_initialize(RID p_rid);
	static void environment_free(RID p_rid);

	static bool is_environment(RID p_env);

	// Background
	static void environment_set_background(RID p_env, RSE::EnvironmentBG p_bg);
	static void environment_set_sky(RID p_env, RID p_sky);
	static void environment_set_sky_custom_fov(RID p_env, float p_scale);
	static void environment_set_sky_orientation(RID p_env, const Basis& p_orientation);
	static void environment_set_bg_color(RID p_env, const Color& p_color);
	static void environment_set_bg_energy(RID p_env, float p_multiplier, float p_exposure_value);
	static void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	static void environment_set_ambient_light(RID p_env, const Color& p_color,
		RSE::EnvironmentAmbientSource p_ambient = RSE::ENV_AMBIENT_SOURCE_BG, float p_energy = 1.0,
		float p_sky_contribution = 0.0,
		RSE::EnvironmentReflectionSource p_reflection_source = RSE::ENV_REFLECTION_SOURCE_BG);
	static void environment_set_camera_feed_id(RID p_env, int p_camera_feed_id);

	static RSE::EnvironmentBG environment_get_background(RID p_env);
	static RID environment_get_sky(RID p_env);
	static float environment_get_sky_custom_fov(RID p_env);
	static Basis environment_get_sky_orientation(RID p_env);
	static Color environment_get_bg_color(RID p_env);
	static float environment_get_bg_energy_multiplier(RID p_env);
	static float environment_get_bg_intensity(RID p_env);
	static int environment_get_canvas_max_layer(RID p_env);
	static RSE::EnvironmentAmbientSource environment_get_ambient_source(RID p_env);
	static Color environment_get_ambient_light(RID p_env);
	static float environment_get_ambient_light_energy(RID p_env);
	static float environment_get_ambient_sky_contribution(RID p_env);
	static RSE::EnvironmentReflectionSource environment_get_reflection_source(RID p_env);
	static int environment_get_camera_feed_id(RID p_env);

	// Tonemap
	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static RSE::EnvironmentToneMapper environment_get_tone_mapper(RID p_env);
	static float environment_get_exposure(RID p_env);
	static float environment_get_white(RID p_env, bool p_limit_agx_white, float p_output_max_value);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static float environment_get_tonemap_agx_contrast(RID p_env);
	static RendererEnvironmentStorage::TonemapParameters environment_get_tonemap_parameters(
		RID p_env, bool p_limit_agx_white, float p_output_max_value);

	// Fog
	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode);
	static bool environment_get_fog_enabled(RID p_env);
	static RSE::EnvironmentFogMode environment_get_fog_mode(RID p_env);
	static Color environment_get_fog_light_color(RID p_env);
	static float environment_get_fog_light_energy(RID p_env);
	static float environment_get_fog_sun_scatter(RID p_env);
	static float environment_get_fog_density(RID p_env);
	static float environment_get_fog_sky_affect(RID p_env);
	static float environment_get_fog_height(RID p_env);
	static float environment_get_fog_height_density(RID p_env);
	static float environment_get_fog_aerial_perspective(RID p_env);

	// Depth Fog
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);
	static float environment_get_fog_depth_curve(RID p_env);
	static float environment_get_fog_depth_begin(RID p_env);
	static float environment_get_fog_depth_end(RID p_env);

	// Volumetric Fog
	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static bool environment_get_volumetric_fog_enabled(RID p_env);
	static float environment_get_volumetric_fog_density(RID p_env);
	static Color environment_get_volumetric_fog_scattering(RID p_env);
	static Color environment_get_volumetric_fog_emission(RID p_env);
	static float environment_get_volumetric_fog_emission_energy(RID p_env);
	static float environment_get_volumetric_fog_anisotropy(RID p_env);
	static float environment_get_volumetric_fog_length(RID p_env);
	static float environment_get_volumetric_fog_detail_spread(RID p_env);
	static float environment_get_volumetric_fog_gi_inject(RID p_env);
	static float environment_get_volumetric_fog_sky_affect(RID p_env);
	static bool environment_get_volumetric_fog_temporal_reprojection(RID p_env);
	static float environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env);
	static float environment_get_volumetric_fog_ambient_inject(RID p_env);

	virtual void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	virtual void environment_set_volumetric_fog_filter_active(bool p_enable);

	// GLOW
	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels, float p_intensity,
		float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);
	static bool environment_get_glow_enabled(RID p_env);
	static Vector<float> environment_get_glow_levels(RID p_env);
	static float environment_get_glow_intensity(RID p_env);
	static float environment_get_glow_strength(RID p_env);
	static float environment_get_glow_bloom(RID p_env);
	static float environment_get_glow_mix(RID p_env);
	static RSE::EnvironmentGlowBlendMode environment_get_glow_blend_mode(RID p_env);
	static float environment_get_glow_hdr_bleed_threshold(RID p_env);
	static float environment_get_glow_hdr_luminance_cap(RID p_env);
	static float environment_get_glow_hdr_bleed_scale(RID p_env);
	static float environment_get_glow_map_strength(RID p_env);
	static RID environment_get_glow_map(RID p_env);

	virtual void environment_glow_set_use_bicubic_upscale(bool p_enable);

	// SSR
	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int,
		float p_fade_out, float p_depth_tolerance);
	static bool environment_get_ssr_enabled(RID p_env);
	static int environment_get_ssr_max_steps(RID p_env);
	static float environment_get_ssr_fade_in(RID p_env);
	static float environment_get_ssr_fade_out(RID p_env);
	static float environment_get_ssr_depth_tolerance(RID p_env);

	virtual void environment_set_ssr_half_size(bool p_half_size);
	virtual void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality);

	// SSAO
	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);
	static bool environment_get_ssao_enabled(RID p_env);
	static float environment_get_ssao_radius(RID p_env);
	static float environment_get_ssao_intensity(RID p_env);
	static float environment_get_ssao_power(RID p_env);
	static float environment_get_ssao_detail(RID p_env);
	static float environment_get_ssao_horizon(RID p_env);
	static float environment_get_ssao_sharpness(RID p_env);
	static float environment_get_ssao_direct_light_affect(RID p_env);
	static float environment_get_ssao_ao_channel_affect(RID p_env);

	virtual void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	// SSIL
	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);
	static bool environment_get_ssil_enabled(RID p_env);
	static float environment_get_ssil_radius(RID p_env);
	static float environment_get_ssil_intensity(RID p_env);
	static float environment_get_ssil_sharpness(RID p_env);
	static float environment_get_ssil_normal_rejection(RID p_env);

	virtual void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	// SDFGI
	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades, float p_min_cell_size,
		RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion, float p_bounce_feedback,
		bool p_read_sky, float p_energy, float p_normal_bias, float p_probe_bias);
	static bool environment_get_sdfgi_enabled(RID p_env);
	static int environment_get_sdfgi_cascades(RID p_env);
	static float environment_get_sdfgi_min_cell_size(RID p_env);
	static bool environment_get_sdfgi_use_occlusion(RID p_env);
	static float environment_get_sdfgi_bounce_feedback(RID p_env);
	static bool environment_get_sdfgi_read_sky_light(RID p_env);
	static float environment_get_sdfgi_energy(RID p_env);
	static float environment_get_sdfgi_normal_bias(RID p_env);
	static float environment_get_sdfgi_probe_bias(RID p_env);
	static RSE::EnvironmentSDFGIYScale environment_get_sdfgi_y_scale(RID p_env);

	virtual void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);
	virtual void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);
	virtual void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	// Adjustment
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness, float p_contrast,
		float p_saturation, bool p_use_1d_color_correction, RID p_color_correction);
	static bool environment_get_adjustments_enabled(RID p_env);
	static float environment_get_adjustments_brightness(RID p_env);
	static float environment_get_adjustments_contrast(RID p_env);
	static float environment_get_adjustments_saturation(RID p_env);
	static bool environment_get_use_1d_color_correction(RID p_env);
	static RID environment_get_color_correction(RID p_env);

	virtual Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	virtual void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	virtual void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	virtual RID fog_volume_instance_create(RID p_fog_volume);
	virtual void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform);
	virtual void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active);
	virtual RID fog_volume_instance_get_volume(RID p_fog_volume_instance) const;
	virtual Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance) const;

	virtual RID voxel_gi_instance_create(RID p_voxel_gi);
	virtual void voxel_gi_instance_set_transform_to_data(
		RID p_probe, const Transform3D& p_xform);
	virtual bool voxel_gi_needs_update(RID p_probe) const;
	virtual void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects);

	virtual void voxel_gi_set_quality(RSE::VoxelGIQuality);

	struct RenderShadowData
	{
		RID light;
		int pass = 0;
		PagedArray<RenderGeometryInstance*> instances;
	};

	struct RenderSDFGIData
	{
		int region = 0;
		PagedArray<RenderGeometryInstance*> instances;
	};

	struct RenderSDFGIUpdateData
	{
		bool update_static = false;
		uint32_t static_cascade_count;
		uint32_t* static_cascade_indices = nullptr;
		PagedArray<RID>* static_positional_lights;

		const Vector<RID>* directional_lights;
		const RID* positional_light_instances;
		uint32_t positional_light_count;
	};

	struct CameraData
	{
		// flags
		uint32_t view_count;
		bool is_orthogonal;
		uint32_t visible_layers;
		bool vaspect;

		// Main/center projection
		Transform3D main_transform;
		Projection main_projection;

		Transform3D view_offset[RendererSceneRender::MAX_RENDER_VIEWS];
		Projection view_projection[RendererSceneRender::MAX_RENDER_VIEWS];
		Vector2 taa_jitter;
		float taa_frame_count = 0.0f;

		void set_camera(const Transform3D p_transform, const Projection p_projection,
			bool p_is_orthogonal, bool p_vaspect, const Vector2& p_taa_jitter = Vector2(),
			float p_taa_frame_count = 0.0f, uint32_t p_visible_layers = 0xFFFFFFFF);
		void set_multiview_camera(uint32_t p_view_count, const Transform3D* p_transforms,
			const Projection* p_projections, bool p_is_orthogonal, bool p_vaspect,
			uint32_t p_visible_layers = 0xFFFFFFFF);
	};

	virtual void render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
		const CameraData* p_camera_data, const CameraData* p_prev_camera_data,
		const PagedArray<RenderGeometryInstance*>& p_instances, const PagedArray<RID>& p_lights,
		const PagedArray<RID>& p_reflection_probes, const PagedArray<RID>& p_voxel_gi_instances,
		const PagedArray<RID>& p_decals, const PagedArray<RID>& p_lightmaps,
		const PagedArray<RID>& p_fog_volumes, RID p_environment, RID p_camera_attributes,
		RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas,
		RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold,
		const RenderShadowData* p_render_shadows, int p_render_shadow_count,
		const RenderSDFGIData* p_render_sdfgi_regions, int p_render_sdfgi_region_count,
		float p_window_output_max_value, const RenderSDFGIUpdateData* p_sdfgi_update_data = nullptr,
		RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	virtual void render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region);
	virtual void render_particle_collider_heightfield(RID p_collider,
		const Transform3D& p_transform, const PagedArray<RenderGeometryInstance*>& p_instances);

	virtual void set_scene_pass(uint64_t p_pass);
	virtual void set_time(double p_time, double p_step);
	virtual void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw);

	virtual Ref<RenderSceneBuffers> render_buffers_create();
	virtual void gi_set_use_half_resolution(bool p_enable);

	virtual void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);
	static bool screen_space_roughness_limiter_is_active();

	virtual void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	virtual void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale);

	virtual bool free(RID p_rid);

	virtual void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	virtual void decals_set_filter(RSE::DecalFilter p_filter);
	virtual void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);
	virtual void lightmaps_set_bicubic_filter(bool p_enable);
	virtual void material_set_use_debanding(bool p_enable);

	virtual void update();

	virtual ~RendererSceneRender() {
	    printf("[LIFECYCLE 2] ~RendererSceneRender running on %p, vptr=%p\n", (void*)this, *(void**)this);
	    fflush(stdout);
	}
};

// --- Folded from servers/rendering/storage/variant_converters.h ---
#include <initializer_list>
#include <type_traits>
#include "core/typedefs.h"
#include "core/types.h"

template <typename T> struct VariantConverterStd140
{
	// Generic base template for all Vector2/3/4(i) classes.
	static constexpr int Elements = T::AXIS_COUNT;

	template <typename P> static void convert(const T& p_v, P* p_write, bool p_compact)
	{
		for (int i = 0; i < Elements; i++) {
			p_write[i] = p_v[i];
		}
	}
};

template <> struct VariantConverterStd140<float>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(float p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<int32_t>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(int32_t p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<uint32_t>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(uint32_t p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<Basis>
{
	static constexpr int Elements = 9;

	template <typename P> static void convert(const Basis& p_v, P* p_write, bool p_compact)
	{
		// Basis can have compact 9 floats or std140 layout 12 floats.
		int i = 0;

		p_write[i++] = p_v.rows[0][0];
		p_write[i++] = p_v.rows[1][0];
		p_write[i++] = p_v.rows[2][0];
		if (!p_compact) {
			p_write[i++] = 0;
		}

		p_write[i++] = p_v.rows[0][1];
		p_write[i++] = p_v.rows[1][1];
		p_write[i++] = p_v.rows[2][1];
		if (!p_compact) {
			p_write[i++] = 0;
		}

		p_write[i++] = p_v.rows[0][2];
		p_write[i++] = p_v.rows[1][2];
		p_write[i++] = p_v.rows[2][2];
		if (!p_compact) {
			p_write[i++] = 0;
		}
	}
};

template <> struct VariantConverterStd140<Transform2D>
{
	static constexpr int Elements = 12;

	template <typename P> static void convert(const Transform2D& p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v.columns[0][0];
		p_write[1] = p_v.columns[0][1];
		p_write[2] = 0;
		p_write[3] = 0;

		p_write[4] = p_v.columns[1][0];
		p_write[5] = p_v.columns[1][1];
		p_write[6] = 0;
		p_write[7] = 0;

		p_write[8] = p_v.columns[2][0];
		p_write[9] = p_v.columns[2][1];
		p_write[10] = 1;
		p_write[11] = 0;
	}
};

template <> struct VariantConverterStd140<Transform3D>
{
	static constexpr int Elements = 16;

	template <typename P> static void convert(const Transform3D& p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v.basis.rows[0][0];
		p_write[1] = p_v.basis.rows[1][0];
		p_write[2] = p_v.basis.rows[2][0];
		p_write[3] = 0;

		p_write[4] = p_v.basis.rows[0][1];
		p_write[5] = p_v.basis.rows[1][1];
		p_write[6] = p_v.basis.rows[2][1];
		p_write[7] = 0;

		p_write[8] = p_v.basis.rows[0][2];
		p_write[9] = p_v.basis.rows[1][2];
		p_write[10] = p_v.basis.rows[2][2];
		p_write[11] = 0;

		p_write[12] = p_v.origin.x;
		p_write[13] = p_v.origin.y;
		p_write[14] = p_v.origin.z;
		p_write[15] = 1;
	}
};

template <> struct VariantConverterStd140<Projection>
{
	static constexpr int Elements = 16;

	template <typename P> static void convert(const Projection& p_v, P* p_write, bool p_compact)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_write[i * 4 + j] = p_v.columns[i][j];
			}
		}
	}
};

template <typename T, typename P> T construct_vector(const std::initializer_list<P>& values)
{
	T vector{};
	int index = 0;
	for (P v : values) {
		vector[index++] = v;
		if (index >= T::AXIS_COUNT) {
			break;
		}
	}
	return vector;
}

template <typename, typename = void> inline constexpr bool is_vector_type_v = false;

template <typename T>
inline constexpr bool is_vector_type_v<T, std::void_t<decltype(T::AXIS_COUNT)>> = true;

template <typename T, typename P>
void convert_item_std140(const T& p_item, P* p_write, bool p_compact = false)
{
	VariantConverterStd140<T>::template convert<P>(p_item, p_write, p_compact);
}

template <typename T, typename From, typename To>
void write_array_std140(const Vector<From>& p_values, To* p_write, int p_array_size, int p_stride)
{
	constexpr int elements = VariantConverterStd140<T>::Elements;
	const int src_count = p_values.size();
	const int dst_count = elements * p_array_size;
	const int stride_count = p_stride * p_array_size;
	const From* read = p_values.ptr();
	const T default_value{};

	memset(p_write, 0, sizeof(To) * stride_count);

	for (int i = 0, j = 0; i < dst_count; i += elements, j += p_stride) {
		if (i + elements - 1 < src_count) {
			// Only copy full items with all elements, no partial or missing data.
			for (int e = 0; e < elements; e++) {
				DEV_ASSERT(j + e < stride_count && i + e < src_count);
				p_write[j + e] = read[i + e];
			}
		}
		else {
			// If not enough source data was passed in, write default values.
			convert_item_std140(default_value, p_write + j);
		}
	}
}
// --- End servers/rendering/renderer_scene_render.h ---

namespace LightStorage
{

struct API
{
	/* Light API */
	RID (*directional_light_allocate)() = nullptr;
	void (*directional_light_initialize)(RID p_rid) = nullptr;

	RID (*omni_light_allocate)() = nullptr;
	void (*omni_light_initialize)(RID p_rid) = nullptr;

	RID (*spot_light_allocate)() = nullptr;
	void (*spot_light_initialize)(RID p_rid) = nullptr;

	RID (*area_light_allocate)() = nullptr;
	void (*area_light_initialize)(RID p_rid) = nullptr;

	void (*light_free)(RID p_rid) = nullptr;

	void (*light_set_color)(RID p_light, const Color& p_color) = nullptr;
	void (*light_set_param)(RID p_light, RSE::LightParam p_param, float p_value) = nullptr;
	void (*light_set_shadow)(RID p_light, bool p_enabled) = nullptr;
	void (*light_set_projector)(RID p_light, RID p_texture) = nullptr;
	void (*light_set_negative)(RID p_light, bool p_enable) = nullptr;
	void (*light_set_cull_mask)(RID p_light, uint32_t p_mask) = nullptr;
	void (*light_set_distance_fade)(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length) = nullptr;
	void (*light_set_reverse_cull_face_mode)(RID p_light, bool p_enabled) = nullptr;
	void (*light_set_shadow_caster_mask)(RID p_light, uint32_t p_caster_mask) = nullptr;
	uint32_t (*light_get_shadow_caster_mask)(RID p_light) = nullptr;
	void (*light_set_bake_mode)(RID p_light, RSE::LightBakeMode p_bake_mode) = nullptr;
	void (*light_set_max_sdfgi_cascade)(RID p_light, uint32_t p_cascade) = nullptr;

	void (*light_omni_set_shadow_mode)(RID p_light, RSE::LightOmniShadowMode p_mode) = nullptr;

	void (*light_directional_set_shadow_mode)(
		RID p_light, RSE::LightDirectionalShadowMode p_mode) = nullptr;
	void (*light_directional_set_blend_splits)(RID p_light, bool p_enable) = nullptr;
	bool (*light_directional_get_blend_splits)(RID p_light) = nullptr;
	void (*light_directional_set_sky_mode)(
		RID p_light, RSE::LightDirectionalSkyMode p_mode) = nullptr;
	RSE::LightDirectionalSkyMode (*light_directional_get_sky_mode)(RID p_light) = nullptr;

	RSE::LightDirectionalShadowMode (*light_directional_get_shadow_mode)(RID p_light) = nullptr;
	RSE::LightOmniShadowMode (*light_omni_get_shadow_mode)(RID p_light) = nullptr;

	void (*light_area_set_size)(RID p_light, const Vector2& p_size) = nullptr;
	Vector2 (*light_area_get_size)(RID p_light) = nullptr;
	void (*light_area_set_normalize_energy)(RID p_light, bool p_enabled) = nullptr;
	bool (*light_area_get_normalize_energy)(RID p_light) = nullptr;
	void (*light_area_set_texture)(RID p_light, RID p_texture) = nullptr;
	RID (*light_area_get_texture)(RID p_light) = nullptr;

	bool (*light_has_shadow)(RID p_light) = nullptr;
	bool (*light_has_projector)(RID p_light) = nullptr;

	RSE::LightType (*light_get_type)(RID p_light) = nullptr;
	AABB (*light_get_aabb)(RID p_light) = nullptr;
	float (*light_get_param)(RID p_light, RSE::LightParam p_param) = nullptr;
	Color (*light_get_color)(RID p_light) = nullptr;
	bool (*light_get_reverse_cull_face_mode)(RID p_light) = nullptr;
	RSE::LightBakeMode (*light_get_bake_mode)(RID p_light) = nullptr;
	uint32_t (*light_get_max_sdfgi_cascade)(RID p_light) = nullptr;
	uint64_t (*light_get_version)(RID p_light) = nullptr;
	uint32_t (*light_get_cull_mask)(RID p_light) = nullptr;

	/* Light Instance API */
	RID (*light_instance_create)(RID p_light) = nullptr;
	void (*light_instance_free)(RID p_light_instance) = nullptr;
	void (*light_instance_set_transform)(
		RID p_light_instance, const Transform3D& p_transform) = nullptr;
	void (*light_instance_set_aabb)(RID p_light_instance, const AABB& p_aabb) = nullptr;
	void (*light_instance_set_shadow_transform)(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale, float p_range_begin,
		const Vector2& p_uv_scale) = nullptr;
	void (*light_instance_mark_visible)(RID p_light_instance) = nullptr;
	bool (*light_instance_is_shadow_visible_at_position)(
		RID p_light, const Vector3& p_position) = nullptr;

	/* Probe API */
	RID (*reflection_probe_allocate)() = nullptr;
	void (*reflection_probe_initialize)(RID p_rid) = nullptr;
	void (*reflection_probe_free)(RID p_rid) = nullptr;

	void (*reflection_probe_set_update_mode)(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode) = nullptr;
	void (*reflection_probe_set_resolution)(RID p_probe, int p_resolution) = nullptr;
	void (*reflection_probe_set_intensity)(RID p_probe, float p_intensity) = nullptr;
	void (*reflection_probe_set_blend_distance)(RID p_probe, float p_blend_distance) = nullptr;
	void (*reflection_probe_set_ambient_mode)(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode) = nullptr;
	void (*reflection_probe_set_ambient_color)(RID p_probe, const Color& p_color) = nullptr;
	void (*reflection_probe_set_ambient_energy)(RID p_probe, float p_energy) = nullptr;
	void (*reflection_probe_set_max_distance)(RID p_probe, float p_distance) = nullptr;
	void (*reflection_probe_set_size)(RID p_probe, const Vector3& p_size) = nullptr;
	void (*reflection_probe_set_origin_offset)(RID p_probe, const Vector3& p_offset) = nullptr;
	void (*reflection_probe_set_as_interior)(RID p_probe, bool p_enable) = nullptr;
	void (*reflection_probe_set_enable_box_projection)(RID p_probe, bool p_enable) = nullptr;
	void (*reflection_probe_set_enable_shadows)(RID p_probe, bool p_enable) = nullptr;
	void (*reflection_probe_set_cull_mask)(RID p_probe, uint32_t p_layers) = nullptr;
	void (*reflection_probe_set_reflection_mask)(RID p_probe, uint32_t p_layers) = nullptr;
	void (*reflection_probe_set_mesh_lod_threshold)(RID p_probe, float p_ratio) = nullptr;

	AABB (*reflection_probe_get_aabb)(RID p_probe) = nullptr;
	RSE::ReflectionProbeUpdateMode (*reflection_probe_get_update_mode)(RID p_probe) = nullptr;
	uint32_t (*reflection_probe_get_cull_mask)(RID p_probe) = nullptr;
	uint32_t (*reflection_probe_get_reflection_mask)(RID p_probe) = nullptr;
	Vector3 (*reflection_probe_get_size)(RID p_probe) = nullptr;
	Vector3 (*reflection_probe_get_origin_offset)(RID p_probe) = nullptr;
	float (*reflection_probe_get_origin_max_distance)(RID p_probe) = nullptr;
	bool (*reflection_probe_renders_shadows)(RID p_probe) = nullptr;
	float (*reflection_probe_get_mesh_lod_threshold)(RID p_probe) = nullptr;

	/* Reflection Atlas */
	RID (*reflection_atlas_create)() = nullptr;
	void (*reflection_atlas_free)(RID p_ref_atlas) = nullptr;
	void (*reflection_atlas_set_size)(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count) = nullptr;
	int (*reflection_atlas_get_size)(RID p_ref_atlas) = nullptr;

	/* Reflection Probe Instance */
	RID (*reflection_probe_instance_create)(RID p_probe) = nullptr;
	void (*reflection_probe_instance_free)(RID p_instance) = nullptr;
	void (*reflection_probe_instance_set_transform)(
		RID p_instance, const Transform3D& p_transform) = nullptr;
	bool (*reflection_probe_has_atlas_index)(RID p_instance) = nullptr;
	void (*reflection_probe_release_atlas_index)(RID p_instance) = nullptr;
	bool (*reflection_probe_instance_needs_redraw)(RID p_instance) = nullptr;
	bool (*reflection_probe_instance_has_reflection)(RID p_instance) = nullptr;
	bool (*reflection_probe_instance_begin_render)(
		RID p_instance, RID p_reflection_atlas) = nullptr;
	bool (*reflection_probe_instance_end_render)(RID p_instance, RID p_reflection_atlas) = nullptr;
	Ref<RenderSceneBuffers> (*reflection_probe_atlas_get_render_buffers)(
		RID p_reflection_atlas) = nullptr;
	bool (*reflection_probe_instance_postprocess_step)(RID p_instance) = nullptr;

	/* Lightmap */
	RID (*lightmap_allocate)() = nullptr;
	void (*lightmap_initialize)(RID p_rid) = nullptr;
	void (*lightmap_free)(RID p_rid) = nullptr;

	void (*lightmap_set_textures)(
		RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics) = nullptr;
	void (*lightmap_set_probe_bounds)(RID p_lightmap, const AABB& p_bounds) = nullptr;
	void (*lightmap_set_probe_interior)(RID p_lightmap, bool p_interior) = nullptr;
	void (*lightmap_set_probe_capture_data)(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree) = nullptr;
	void (*lightmap_set_baked_exposure_normalization)(RID p_lightmap, float p_exposure) = nullptr;
	PackedVector3Array (*lightmap_get_probe_capture_points)(RID p_lightmap) = nullptr;
	PackedColorArray (*lightmap_get_probe_capture_sh)(RID p_lightmap) = nullptr;
	PackedInt32Array (*lightmap_get_probe_capture_tetrahedra)(RID p_lightmap) = nullptr;
	PackedInt32Array (*lightmap_get_probe_capture_bsp_tree)(RID p_lightmap) = nullptr;
	AABB (*lightmap_get_aabb)(RID p_lightmap) = nullptr;
	void (*lightmap_tap_sh_light)(RID p_lightmap, const Vector3& p_point, Color* r_sh) = nullptr;
	bool (*lightmap_is_interior)(RID p_lightmap) = nullptr;
	void (*lightmap_set_probe_capture_update_speed)(float p_speed) = nullptr;
	float (*lightmap_get_probe_capture_update_speed)() = nullptr;

	void (*lightmap_set_shadowmask_textures)(RID p_lightmap, RID p_shadow) = nullptr;
	RSE::ShadowmaskMode (*lightmap_get_shadowmask_mode)(RID p_lightmap) = nullptr;
	void (*lightmap_set_shadowmask_mode)(RID p_lightmap, RSE::ShadowmaskMode p_mode) = nullptr;

	/* Lightmap Instance */
	RID (*lightmap_instance_create)(RID p_lightmap) = nullptr;
	void (*lightmap_instance_free)(RID p_lightmap) = nullptr;
	void (*lightmap_instance_set_transform)(
		RID p_lightmap, const Transform3D& p_transform) = nullptr;

	/* Shadow Atlas */
	RID (*shadow_atlas_create)() = nullptr;
	void (*shadow_atlas_free)(RID p_atlas) = nullptr;
	void (*shadow_atlas_set_size)(RID p_atlas, int p_size, bool p_use_16_bits) = nullptr;
	void (*shadow_atlas_set_quadrant_subdivision)(
		RID p_atlas, int p_quadrant, int p_subdivision) = nullptr;
	bool (*shadow_atlas_update_light)(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version) = nullptr;
	void (*shadow_atlas_update)(RID p_atlas) = nullptr;

	void (*directional_shadow_atlas_set_size)(int p_size, bool p_16_bits) = nullptr;
	int (*get_directional_light_shadow_size)(RID p_light_instance) = nullptr;
	void (*set_directional_shadow_count)(int p_count) = nullptr;
}; // struct API

}; // namespace LightStorage

class RendererLightStorage final
{
private:
	static inline LightStorage::API api;

public:
	RendererLightStorage() = delete;
	RendererLightStorage(const RendererLightStorage&) = delete;
	RendererLightStorage& operator=(const RendererLightStorage&) = delete;
	~RendererLightStorage() = delete;

	template <typename StorageImpl> static void bind()
	{
		/* Light API */
		api.directional_light_allocate = &StorageImpl::directional_light_allocate;
		api.directional_light_initialize = &StorageImpl::directional_light_initialize;
		api.omni_light_allocate = &StorageImpl::omni_light_allocate;
		api.omni_light_initialize = &StorageImpl::omni_light_initialize;
		api.spot_light_allocate = &StorageImpl::spot_light_allocate;
		api.spot_light_initialize = &StorageImpl::spot_light_initialize;
		api.area_light_allocate = &StorageImpl::area_light_allocate;
		api.area_light_initialize = &StorageImpl::area_light_initialize;
		api.light_free = &StorageImpl::light_free;

		api.light_set_color = &StorageImpl::light_set_color;
		api.light_set_param = &StorageImpl::light_set_param;
		api.light_set_shadow = &StorageImpl::light_set_shadow;
		api.light_set_projector = &StorageImpl::light_set_projector;
		api.light_set_negative = &StorageImpl::light_set_negative;
		api.light_set_cull_mask = &StorageImpl::light_set_cull_mask;
		api.light_set_distance_fade = &StorageImpl::light_set_distance_fade;
		api.light_set_reverse_cull_face_mode = &StorageImpl::light_set_reverse_cull_face_mode;
		api.light_set_shadow_caster_mask = &StorageImpl::light_set_shadow_caster_mask;
		api.light_get_shadow_caster_mask = &StorageImpl::light_get_shadow_caster_mask;
		api.light_set_bake_mode = &StorageImpl::light_set_bake_mode;
		api.light_set_max_sdfgi_cascade = &StorageImpl::light_set_max_sdfgi_cascade;

		api.light_omni_set_shadow_mode = &StorageImpl::light_omni_set_shadow_mode;

		api.light_directional_set_shadow_mode = &StorageImpl::light_directional_set_shadow_mode;
		api.light_directional_set_blend_splits = &StorageImpl::light_directional_set_blend_splits;
		api.light_directional_get_blend_splits = &StorageImpl::light_directional_get_blend_splits;
		api.light_directional_set_sky_mode = &StorageImpl::light_directional_set_sky_mode;
		api.light_directional_get_sky_mode = &StorageImpl::light_directional_get_sky_mode;

		api.light_directional_get_shadow_mode = &StorageImpl::light_directional_get_shadow_mode;
		api.light_omni_get_shadow_mode = &StorageImpl::light_omni_get_shadow_mode;

		api.light_area_set_size = &StorageImpl::light_area_set_size;
		api.light_area_get_size = &StorageImpl::light_area_get_size;
		api.light_area_set_normalize_energy = &StorageImpl::light_area_set_normalize_energy;
		api.light_area_get_normalize_energy = &StorageImpl::light_area_get_normalize_energy;
		api.light_area_set_texture = &StorageImpl::light_area_set_texture;
		api.light_area_get_texture = &StorageImpl::light_area_get_texture;

		api.light_has_shadow = &StorageImpl::light_has_shadow;
		api.light_has_projector = &StorageImpl::light_has_projector;

		api.light_get_type = &StorageImpl::light_get_type;
		api.light_get_aabb = &StorageImpl::light_get_aabb;
		api.light_get_param = &StorageImpl::light_get_param;
		api.light_get_color = &StorageImpl::light_get_color;
		api.light_get_reverse_cull_face_mode = &StorageImpl::light_get_reverse_cull_face_mode;
		api.light_get_bake_mode = &StorageImpl::light_get_bake_mode;
		api.light_get_max_sdfgi_cascade = &StorageImpl::light_get_max_sdfgi_cascade;
		api.light_get_version = &StorageImpl::light_get_version;
		api.light_get_cull_mask = &StorageImpl::light_get_cull_mask;

		/* Light Instance API */
		api.light_instance_create = &StorageImpl::light_instance_create;
		api.light_instance_free = &StorageImpl::light_instance_free;
		api.light_instance_set_transform = &StorageImpl::light_instance_set_transform;
		api.light_instance_set_aabb = &StorageImpl::light_instance_set_aabb;
		api.light_instance_set_shadow_transform = &StorageImpl::light_instance_set_shadow_transform;
		api.light_instance_mark_visible = &StorageImpl::light_instance_mark_visible;
		api.light_instance_is_shadow_visible_at_position =
			&StorageImpl::light_instance_is_shadow_visible_at_position;

		/* Probe API */
		api.reflection_probe_allocate = &StorageImpl::reflection_probe_allocate;
		api.reflection_probe_initialize = &StorageImpl::reflection_probe_initialize;
		api.reflection_probe_free = &StorageImpl::reflection_probe_free;

		api.reflection_probe_set_update_mode = &StorageImpl::reflection_probe_set_update_mode;
		api.reflection_probe_set_resolution = &StorageImpl::reflection_probe_set_resolution;
		api.reflection_probe_set_intensity = &StorageImpl::reflection_probe_set_intensity;
		api.reflection_probe_set_blend_distance = &StorageImpl::reflection_probe_set_blend_distance;
		api.reflection_probe_set_ambient_mode = &StorageImpl::reflection_probe_set_ambient_mode;
		api.reflection_probe_set_ambient_color = &StorageImpl::reflection_probe_set_ambient_color;
		api.reflection_probe_set_ambient_energy = &StorageImpl::reflection_probe_set_ambient_energy;
		api.reflection_probe_set_max_distance = &StorageImpl::reflection_probe_set_max_distance;
		api.reflection_probe_set_size = &StorageImpl::reflection_probe_set_size;
		api.reflection_probe_set_origin_offset = &StorageImpl::reflection_probe_set_origin_offset;
		api.reflection_probe_set_as_interior = &StorageImpl::reflection_probe_set_as_interior;
		api.reflection_probe_set_enable_box_projection =
			&StorageImpl::reflection_probe_set_enable_box_projection;
		api.reflection_probe_set_enable_shadows = &StorageImpl::reflection_probe_set_enable_shadows;
		api.reflection_probe_set_cull_mask = &StorageImpl::reflection_probe_set_cull_mask;
		api.reflection_probe_set_reflection_mask =
			&StorageImpl::reflection_probe_set_reflection_mask;
		api.reflection_probe_set_mesh_lod_threshold =
			&StorageImpl::reflection_probe_set_mesh_lod_threshold;

		api.reflection_probe_get_aabb = &StorageImpl::reflection_probe_get_aabb;
		api.reflection_probe_get_update_mode = &StorageImpl::reflection_probe_get_update_mode;
		api.reflection_probe_get_cull_mask = &StorageImpl::reflection_probe_get_cull_mask;
		api.reflection_probe_get_reflection_mask =
			&StorageImpl::reflection_probe_get_reflection_mask;
		api.reflection_probe_get_size = &StorageImpl::reflection_probe_get_size;
		api.reflection_probe_get_origin_offset = &StorageImpl::reflection_probe_get_origin_offset;
		api.reflection_probe_get_origin_max_distance =
			&StorageImpl::reflection_probe_get_origin_max_distance;
		api.reflection_probe_renders_shadows = &StorageImpl::reflection_probe_renders_shadows;
		api.reflection_probe_get_mesh_lod_threshold =
			&StorageImpl::reflection_probe_get_mesh_lod_threshold;

		/* Reflection Atlas */
		api.reflection_atlas_create = &StorageImpl::reflection_atlas_create;
		api.reflection_atlas_free = &StorageImpl::reflection_atlas_free;
		api.reflection_atlas_set_size = &StorageImpl::reflection_atlas_set_size;
		api.reflection_atlas_get_size = &StorageImpl::reflection_atlas_get_size;

		/* Reflection Probe Instance */
		api.reflection_probe_instance_create = &StorageImpl::reflection_probe_instance_create;
		api.reflection_probe_instance_free = &StorageImpl::reflection_probe_instance_free;
		api.reflection_probe_instance_set_transform =
			&StorageImpl::reflection_probe_instance_set_transform;
		api.reflection_probe_has_atlas_index = &StorageImpl::reflection_probe_has_atlas_index;
		api.reflection_probe_release_atlas_index =
			&StorageImpl::reflection_probe_release_atlas_index;
		api.reflection_probe_instance_needs_redraw =
			&StorageImpl::reflection_probe_instance_needs_redraw;
		api.reflection_probe_instance_has_reflection =
			&StorageImpl::reflection_probe_instance_has_reflection;
		api.reflection_probe_instance_begin_render =
			&StorageImpl::reflection_probe_instance_begin_render;
		api.reflection_probe_instance_end_render =
			&StorageImpl::reflection_probe_instance_end_render;
		api.reflection_probe_atlas_get_render_buffers =
			&StorageImpl::reflection_probe_atlas_get_render_buffers;
		api.reflection_probe_instance_postprocess_step =
			&StorageImpl::reflection_probe_instance_postprocess_step;

		/* Lightmap */
		api.lightmap_allocate = &StorageImpl::lightmap_allocate;
		api.lightmap_initialize = &StorageImpl::lightmap_initialize;
		api.lightmap_free = &StorageImpl::lightmap_free;

		api.lightmap_set_textures = &StorageImpl::lightmap_set_textures;
		api.lightmap_set_probe_bounds = &StorageImpl::lightmap_set_probe_bounds;
		api.lightmap_set_probe_interior = &StorageImpl::lightmap_set_probe_interior;
		api.lightmap_set_probe_capture_data = &StorageImpl::lightmap_set_probe_capture_data;
		api.lightmap_set_baked_exposure_normalization =
			&StorageImpl::lightmap_set_baked_exposure_normalization;
		api.lightmap_get_probe_capture_points = &StorageImpl::lightmap_get_probe_capture_points;
		api.lightmap_get_probe_capture_sh = &StorageImpl::lightmap_get_probe_capture_sh;
		api.lightmap_get_probe_capture_tetrahedra =
			&StorageImpl::lightmap_get_probe_capture_tetrahedra;
		api.lightmap_get_probe_capture_bsp_tree = &StorageImpl::lightmap_get_probe_capture_bsp_tree;
		api.lightmap_get_aabb = &StorageImpl::lightmap_get_aabb;
		api.lightmap_tap_sh_light = &StorageImpl::lightmap_tap_sh_light;
		api.lightmap_is_interior = &StorageImpl::lightmap_is_interior;
		api.lightmap_set_probe_capture_update_speed =
			&StorageImpl::lightmap_set_probe_capture_update_speed;
		api.lightmap_get_probe_capture_update_speed =
			&StorageImpl::lightmap_get_probe_capture_update_speed;

		api.lightmap_set_shadowmask_textures = &StorageImpl::lightmap_set_shadowmask_textures;
		api.lightmap_get_shadowmask_mode = &StorageImpl::lightmap_get_shadowmask_mode;
		api.lightmap_set_shadowmask_mode = &StorageImpl::lightmap_set_shadowmask_mode;

		/* Lightmap Instance */
		api.lightmap_instance_create = &StorageImpl::lightmap_instance_create;
		api.lightmap_instance_free = &StorageImpl::lightmap_instance_free;
		api.lightmap_instance_set_transform = &StorageImpl::lightmap_instance_set_transform;

		/* Shadow Atlas */
		api.shadow_atlas_create = &StorageImpl::shadow_atlas_create;
		api.shadow_atlas_free = &StorageImpl::shadow_atlas_free;
		api.shadow_atlas_set_size = &StorageImpl::shadow_atlas_set_size;
		api.shadow_atlas_set_quadrant_subdivision =
			&StorageImpl::shadow_atlas_set_quadrant_subdivision;
		api.shadow_atlas_update_light = &StorageImpl::shadow_atlas_update_light;
		api.shadow_atlas_update = &StorageImpl::shadow_atlas_update;

		api.directional_shadow_atlas_set_size = &StorageImpl::directional_shadow_atlas_set_size;
		api.get_directional_light_shadow_size = &StorageImpl::get_directional_light_shadow_size;
		api.set_directional_shadow_count = &StorageImpl::set_directional_shadow_count;
	}

	/* Light API */
	_FORCE_INLINE_ static RID directional_light_allocate()
	{
		return api.directional_light_allocate();
	}

	_FORCE_INLINE_ static void directional_light_initialize(RID p_rid)
	{
		api.directional_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static RID omni_light_allocate() { return api.omni_light_allocate(); }

	_FORCE_INLINE_ static void omni_light_initialize(RID p_rid)
	{
		api.omni_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static RID spot_light_allocate() { return api.spot_light_allocate(); }

	_FORCE_INLINE_ static void spot_light_initialize(RID p_rid)
	{
		api.spot_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static RID area_light_allocate() { return api.area_light_allocate(); }

	_FORCE_INLINE_ static void area_light_initialize(RID p_rid)
	{
		api.area_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static void light_free(RID p_rid) { api.light_free(p_rid); }

	_FORCE_INLINE_ static void light_set_color(RID p_light, const Color& p_color)
	{
		api.light_set_color(p_light, p_color);
	}

	_FORCE_INLINE_ static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value)
	{
		api.light_set_param(p_light, p_param, p_value);
	}

	_FORCE_INLINE_ static void light_set_shadow(RID p_light, bool p_enabled)
	{
		api.light_set_shadow(p_light, p_enabled);
	}

	_FORCE_INLINE_ static void light_set_projector(RID p_light, RID p_texture)
	{
		api.light_set_projector(p_light, p_texture);
	}

	_FORCE_INLINE_ static void light_set_negative(RID p_light, bool p_enable)
	{
		api.light_set_negative(p_light, p_enable);
	}

	_FORCE_INLINE_ static void light_set_cull_mask(RID p_light, uint32_t p_mask)
	{
		api.light_set_cull_mask(p_light, p_mask);
	}

	_FORCE_INLINE_ static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
	{
		api.light_set_distance_fade(p_light, p_enabled, p_begin, p_shadow, p_length);
	}

	_FORCE_INLINE_ static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled)
	{
		api.light_set_reverse_cull_face_mode(p_light, p_enabled);
	}

	_FORCE_INLINE_ static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask)
	{
		api.light_set_shadow_caster_mask(p_light, p_caster_mask);
	}

	_FORCE_INLINE_ static uint32_t light_get_shadow_caster_mask(RID p_light)
	{
		return api.light_get_shadow_caster_mask(p_light);
	}

	_FORCE_INLINE_ static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode)
	{
		api.light_set_bake_mode(p_light, p_bake_mode);
	}

	_FORCE_INLINE_ static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade)
	{
		api.light_set_max_sdfgi_cascade(p_light, p_cascade);
	}

	_FORCE_INLINE_ static void light_omni_set_shadow_mode(
		RID p_light, RSE::LightOmniShadowMode p_mode)
	{
		api.light_omni_set_shadow_mode(p_light, p_mode);
	}

	_FORCE_INLINE_ static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode)
	{
		api.light_directional_set_shadow_mode(p_light, p_mode);
	}

	_FORCE_INLINE_ static void light_directional_set_blend_splits(RID p_light, bool p_enable)
	{
		api.light_directional_set_blend_splits(p_light, p_enable);
	}

	_FORCE_INLINE_ static bool light_directional_get_blend_splits(RID p_light)
	{
		return api.light_directional_get_blend_splits(p_light);
	}

	_FORCE_INLINE_ static void light_directional_set_sky_mode(
		RID p_light, RSE::LightDirectionalSkyMode p_mode)
	{
		api.light_directional_set_sky_mode(p_light, p_mode);
	}

	_FORCE_INLINE_ static RSE::LightDirectionalSkyMode light_directional_get_sky_mode(RID p_light)
	{
		return api.light_directional_get_sky_mode(p_light);
	}

	_FORCE_INLINE_ static RSE::LightDirectionalShadowMode light_directional_get_shadow_mode(
		RID p_light)
	{
		return api.light_directional_get_shadow_mode(p_light);
	}

	_FORCE_INLINE_ static RSE::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light)
	{
		return api.light_omni_get_shadow_mode(p_light);
	}

	_FORCE_INLINE_ static void light_area_set_size(RID p_light, const Vector2& p_size)
	{
		api.light_area_set_size(p_light, p_size);
	}

	_FORCE_INLINE_ static Vector2 light_area_get_size(RID p_light)
	{
		return api.light_area_get_size(p_light);
	}

	_FORCE_INLINE_ static void light_area_set_normalize_energy(RID p_light, bool p_enabled)
	{
		api.light_area_set_normalize_energy(p_light, p_enabled);
	}

	_FORCE_INLINE_ static bool light_area_get_normalize_energy(RID p_light)
	{
		return api.light_area_get_normalize_energy(p_light);
	}

	_FORCE_INLINE_ static void light_area_set_texture(RID p_light, RID p_texture)
	{
		api.light_area_set_texture(p_light, p_texture);
	}

	_FORCE_INLINE_ static RID light_area_get_texture(RID p_light)
	{
		return api.light_area_get_texture(p_light);
	}

	_FORCE_INLINE_ static bool light_has_shadow(RID p_light)
	{
		return api.light_has_shadow(p_light);
	}

	_FORCE_INLINE_ static bool light_has_projector(RID p_light)
	{
		return api.light_has_projector(p_light);
	}

	_FORCE_INLINE_ static RSE::LightType light_get_type(RID p_light)
	{
		return api.light_get_type(p_light);
	}

	_FORCE_INLINE_ static AABB light_get_aabb(RID p_light) { return api.light_get_aabb(p_light); }

	_FORCE_INLINE_ static float light_get_param(RID p_light, RSE::LightParam p_param)
	{
		return api.light_get_param(p_light, p_param);
	}

	_FORCE_INLINE_ static Color light_get_color(RID p_light)
	{
		return api.light_get_color(p_light);
	}

	_FORCE_INLINE_ static bool light_get_reverse_cull_face_mode(RID p_light)
	{
		return api.light_get_reverse_cull_face_mode(p_light);
	}

	_FORCE_INLINE_ static RSE::LightBakeMode light_get_bake_mode(RID p_light)
	{
		return api.light_get_bake_mode(p_light);
	}

	_FORCE_INLINE_ static uint32_t light_get_max_sdfgi_cascade(RID p_light)
	{
		return api.light_get_max_sdfgi_cascade(p_light);
	}

	_FORCE_INLINE_ static uint64_t light_get_version(RID p_light)
	{
		return api.light_get_version(p_light);
	}

	_FORCE_INLINE_ static uint32_t light_get_cull_mask(RID p_light)
	{
		return api.light_get_cull_mask(p_light);
	}

	/* Light Instance API */
	_FORCE_INLINE_ static RID light_instance_create(RID p_light)
	{
		return api.light_instance_create(p_light);
	}

	_FORCE_INLINE_ static void light_instance_free(RID p_light_instance)
	{
		api.light_instance_free(p_light_instance);
	}

	_FORCE_INLINE_ static void light_instance_set_transform(
		RID p_light_instance, const Transform3D& p_transform)
	{
		api.light_instance_set_transform(p_light_instance, p_transform);
	}

	_FORCE_INLINE_ static void light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb)
	{
		api.light_instance_set_aabb(p_light_instance, p_aabb);
	}

	_FORCE_INLINE_ static void light_instance_set_shadow_transform(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale = 1.0, float p_range_begin = 0,
		const Vector2& p_uv_scale = Vector2())
	{
		api.light_instance_set_shadow_transform(p_light_instance, p_projection, p_transform, p_far,
			p_split, p_pass, p_shadow_texel_size, p_bias_scale, p_range_begin, p_uv_scale);
	}

	_FORCE_INLINE_ static void light_instance_mark_visible(RID p_light_instance)
	{
		api.light_instance_mark_visible(p_light_instance);
	}

	_FORCE_INLINE_ static bool light_instances_can_render_shadow_cube() { return true; }

	_FORCE_INLINE_ static bool light_instance_is_shadow_visible_at_position(
		RID p_light, const Vector3& p_position)
	{
		return api.light_instance_is_shadow_visible_at_position(p_light, p_position);
	}

	/* Probe API */
	_FORCE_INLINE_ static RID reflection_probe_allocate()
	{
		return api.reflection_probe_allocate();
	}

	_FORCE_INLINE_ static void reflection_probe_initialize(RID p_rid)
	{
		api.reflection_probe_initialize(p_rid);
	}

	_FORCE_INLINE_ static void reflection_probe_free(RID p_rid)
	{
		api.reflection_probe_free(p_rid);
	}

	_FORCE_INLINE_ static void reflection_probe_set_update_mode(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
	{
		api.reflection_probe_set_update_mode(p_probe, p_mode);
	}

	_FORCE_INLINE_ static void reflection_probe_set_resolution(RID p_probe, int p_resolution)
	{
		api.reflection_probe_set_resolution(p_probe, p_resolution);
	}

	_FORCE_INLINE_ static void reflection_probe_set_intensity(RID p_probe, float p_intensity)
	{
		api.reflection_probe_set_intensity(p_probe, p_intensity);
	}

	_FORCE_INLINE_ static void reflection_probe_set_blend_distance(
		RID p_probe, float p_blend_distance)
	{
		api.reflection_probe_set_blend_distance(p_probe, p_blend_distance);
	}

	_FORCE_INLINE_ static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode)
	{
		api.reflection_probe_set_ambient_mode(p_probe, p_mode);
	}

	_FORCE_INLINE_ static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color)
	{
		api.reflection_probe_set_ambient_color(p_probe, p_color);
	}

	_FORCE_INLINE_ static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy)
	{
		api.reflection_probe_set_ambient_energy(p_probe, p_energy);
	}

	_FORCE_INLINE_ static void reflection_probe_set_max_distance(RID p_probe, float p_distance)
	{
		api.reflection_probe_set_max_distance(p_probe, p_distance);
	}

	_FORCE_INLINE_ static void reflection_probe_set_size(RID p_probe, const Vector3& p_size)
	{
		api.reflection_probe_set_size(p_probe, p_size);
	}

	_FORCE_INLINE_ static void reflection_probe_set_origin_offset(
		RID p_probe, const Vector3& p_offset)
	{
		api.reflection_probe_set_origin_offset(p_probe, p_offset);
	}

	_FORCE_INLINE_ static void reflection_probe_set_as_interior(RID p_probe, bool p_enable)
	{
		api.reflection_probe_set_as_interior(p_probe, p_enable);
	}

	_FORCE_INLINE_ static void reflection_probe_set_enable_box_projection(
		RID p_probe, bool p_enable)
	{
		api.reflection_probe_set_enable_box_projection(p_probe, p_enable);
	}

	_FORCE_INLINE_ static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable)
	{
		api.reflection_probe_set_enable_shadows(p_probe, p_enable);
	}

	_FORCE_INLINE_ static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers)
	{
		api.reflection_probe_set_cull_mask(p_probe, p_layers);
	}

	_FORCE_INLINE_ static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers)
	{
		api.reflection_probe_set_reflection_mask(p_probe, p_layers);
	}

	_FORCE_INLINE_ static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio)
	{
		api.reflection_probe_set_mesh_lod_threshold(p_probe, p_ratio);
	}

	_FORCE_INLINE_ static AABB reflection_probe_get_aabb(RID p_probe)
	{
		return api.reflection_probe_get_aabb(p_probe);
	}

	_FORCE_INLINE_ static RSE::ReflectionProbeUpdateMode reflection_probe_get_update_mode(
		RID p_probe)
	{
		return api.reflection_probe_get_update_mode(p_probe);
	}

	_FORCE_INLINE_ static uint32_t reflection_probe_get_cull_mask(RID p_probe)
	{
		return api.reflection_probe_get_cull_mask(p_probe);
	}

	_FORCE_INLINE_ static uint32_t reflection_probe_get_reflection_mask(RID p_probe)
	{
		return api.reflection_probe_get_reflection_mask(p_probe);
	}

	_FORCE_INLINE_ static Vector3 reflection_probe_get_size(RID p_probe)
	{
		return api.reflection_probe_get_size(p_probe);
	}

	_FORCE_INLINE_ static Vector3 reflection_probe_get_origin_offset(RID p_probe)
	{
		return api.reflection_probe_get_origin_offset(p_probe);
	}

	_FORCE_INLINE_ static float reflection_probe_get_origin_max_distance(RID p_probe)
	{
		return api.reflection_probe_get_origin_max_distance(p_probe);
	}

	_FORCE_INLINE_ static bool reflection_probe_renders_shadows(RID p_probe)
	{
		return api.reflection_probe_renders_shadows(p_probe);
	}

	_FORCE_INLINE_ static float reflection_probe_get_mesh_lod_threshold(RID p_probe)
	{
		return api.reflection_probe_get_mesh_lod_threshold(p_probe);
	}

	/* Reflection Atlas */
	_FORCE_INLINE_ static RID reflection_atlas_create() { return api.reflection_atlas_create(); }

	_FORCE_INLINE_ static void reflection_atlas_free(RID p_ref_atlas)
	{
		api.reflection_atlas_free(p_ref_atlas);
	}

	_FORCE_INLINE_ static void reflection_atlas_set_size(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count)
	{
		api.reflection_atlas_set_size(p_ref_atlas, p_reflection_size, p_reflection_count);
	}

	_FORCE_INLINE_ static int reflection_atlas_get_size(RID p_ref_atlas)
	{
		return api.reflection_atlas_get_size(p_ref_atlas);
	}

	/* Reflection Probe Instance */
	_FORCE_INLINE_ static RID reflection_probe_instance_create(RID p_probe)
	{
		return api.reflection_probe_instance_create(p_probe);
	}

	_FORCE_INLINE_ static void reflection_probe_instance_free(RID p_instance)
	{
		api.reflection_probe_instance_free(p_instance);
	}

	_FORCE_INLINE_ static void reflection_probe_instance_set_transform(
		RID p_instance, const Transform3D& p_transform)
	{
		api.reflection_probe_instance_set_transform(p_instance, p_transform);
	}

	_FORCE_INLINE_ static bool reflection_probe_has_atlas_index(RID p_instance)
	{
		return api.reflection_probe_has_atlas_index(p_instance);
	}

	_FORCE_INLINE_ static void reflection_probe_release_atlas_index(RID p_instance)
	{
		api.reflection_probe_release_atlas_index(p_instance);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_needs_redraw(RID p_instance)
	{
		return api.reflection_probe_instance_needs_redraw(p_instance);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_has_reflection(RID p_instance)
	{
		return api.reflection_probe_instance_has_reflection(p_instance);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_begin_render(
		RID p_instance, RID p_reflection_atlas)
	{
		return api.reflection_probe_instance_begin_render(p_instance, p_reflection_atlas);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_end_render(
		RID p_instance, RID p_reflection_atlas)
	{
		return api.reflection_probe_instance_end_render(p_instance, p_reflection_atlas);
	}

	_FORCE_INLINE_ static Ref<RenderSceneBuffers> reflection_probe_atlas_get_render_buffers(
		RID p_reflection_atlas)
	{
		return api.reflection_probe_atlas_get_render_buffers(p_reflection_atlas);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_postprocess_step(RID p_instance)
	{
		return api.reflection_probe_instance_postprocess_step(p_instance);
	}

	/* Lightmap */
	_FORCE_INLINE_ static RID lightmap_allocate() { return api.lightmap_allocate(); }

	_FORCE_INLINE_ static void lightmap_initialize(RID p_rid) { api.lightmap_initialize(p_rid); }

	_FORCE_INLINE_ static void lightmap_free(RID p_rid) { api.lightmap_free(p_rid); }

	_FORCE_INLINE_ static void lightmap_set_textures(
		RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics)
	{
		api.lightmap_set_textures(p_lightmap, p_light, p_uses_spherical_harmonics);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds)
	{
		api.lightmap_set_probe_bounds(p_lightmap, p_bounds);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior)
	{
		api.lightmap_set_probe_interior(p_lightmap, p_interior);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_capture_data(RID p_lightmap,
		const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
		const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree)
	{
		api.lightmap_set_probe_capture_data(
			p_lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
	}

	_FORCE_INLINE_ static void lightmap_set_baked_exposure_normalization(
		RID p_lightmap, float p_exposure)
	{
		api.lightmap_set_baked_exposure_normalization(p_lightmap, p_exposure);
	}

	_FORCE_INLINE_ static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_points(p_lightmap);
	}

	_FORCE_INLINE_ static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_sh(p_lightmap);
	}

	_FORCE_INLINE_ static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_tetrahedra(p_lightmap);
	}

	_FORCE_INLINE_ static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_bsp_tree(p_lightmap);
	}

	_FORCE_INLINE_ static AABB lightmap_get_aabb(RID p_lightmap)
	{
		return api.lightmap_get_aabb(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_tap_sh_light(
		RID p_lightmap, const Vector3& p_point, Color* r_sh)
	{
		api.lightmap_tap_sh_light(p_lightmap, p_point, r_sh);
	}

	_FORCE_INLINE_ static bool lightmap_is_interior(RID p_lightmap)
	{
		return api.lightmap_is_interior(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_capture_update_speed(float p_speed)
	{
		api.lightmap_set_probe_capture_update_speed(p_speed);
	}

	_FORCE_INLINE_ static float lightmap_get_probe_capture_update_speed()
	{
		return api.lightmap_get_probe_capture_update_speed();
	}

	_FORCE_INLINE_ static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow)
	{
		api.lightmap_set_shadowmask_textures(p_lightmap, p_shadow);
	}

	_FORCE_INLINE_ static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap)
	{
		return api.lightmap_get_shadowmask_mode(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_set_shadowmask_mode(
		RID p_lightmap, RSE::ShadowmaskMode p_mode)
	{
		api.lightmap_set_shadowmask_mode(p_lightmap, p_mode);
	}

	/* Lightmap Instance */

	_FORCE_INLINE_ static RID lightmap_instance_create(RID p_lightmap)
	{
		return api.lightmap_instance_create(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_instance_free(RID p_lightmap)
	{
		api.lightmap_instance_free(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_instance_set_transform(
		RID p_lightmap, const Transform3D& p_transform)
	{
		api.lightmap_instance_set_transform(p_lightmap, p_transform);
	}

	/* Shadow Atlas */
	_FORCE_INLINE_ static RID shadow_atlas_create() { return api.shadow_atlas_create(); }

	_FORCE_INLINE_ static void shadow_atlas_free(RID p_atlas) { api.shadow_atlas_free(p_atlas); }

	_FORCE_INLINE_ static void shadow_atlas_set_size(
		RID p_atlas, int p_size, bool p_use_16_bits = true)
	{
		api.shadow_atlas_set_size(p_atlas, p_size, p_use_16_bits);
	}

	_FORCE_INLINE_ static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision)
	{
		api.shadow_atlas_set_quadrant_subdivision(p_atlas, p_quadrant, p_subdivision);
	}

	_FORCE_INLINE_ static bool shadow_atlas_update_light(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version)
	{
		return api.shadow_atlas_update_light(
			p_atlas, p_light_instance, p_coverage, p_light_version);
	}

	_FORCE_INLINE_ static void shadow_atlas_update(RID p_atlas)
	{
		api.shadow_atlas_update(p_atlas);
	}

	_FORCE_INLINE_ static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true)
	{
		api.directional_shadow_atlas_set_size(p_size, p_16_bits);
	}

	_FORCE_INLINE_ static int get_directional_light_shadow_size(RID p_light_instance)
	{
		return api.get_directional_light_shadow_size(p_light_instance);
	}

	_FORCE_INLINE_ static void set_directional_shadow_count(int p_count)
	{
		api.set_directional_shadow_count(p_count);
	}
};
// --- Inlined/Folded from servers/rendering/storage/material_storage.h ---
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"


struct InstanceShaderParam
{
	int index;
};

namespace MaterialStorage
{

struct API
{
	/* GLOBAL SHADER UNIFORM API */
	void (*global_shader_parameter_remove)(const String& p_name) = nullptr;
	Vector<String> (*global_shader_parameter_get_list)() = nullptr;
	RSE::GlobalShaderParameterType (*global_shader_parameter_get_type)(
		const String& p_name) = nullptr;
	void (*global_shader_parameters_load_settings)(bool p_load_textures) = nullptr;
	void (*global_shader_parameters_clear)() = nullptr;
	int32_t (*global_shader_parameters_instance_allocate)(RID p_instance) = nullptr;
	void (*global_shader_parameters_instance_free)(RID p_instance) = nullptr;
	void (*finalize)() = nullptr;

	/* SHADER API */
	RID (*shader_allocate)() = nullptr;
	void (*shader_initialize)(RID p_rid, bool p_embedded) = nullptr;
	void (*shader_free)(RID p_rid) = nullptr;
	void (*shader_set_code)(RID p_shader, const String& p_code) = nullptr;
	void (*shader_set_path_hint)(RID p_shader, const String& p_path) = nullptr;
	String (*shader_get_code)(RID p_shader) = nullptr;
	void (*shader_set_default_texture_parameter)(
		RID p_shader, const String& p_name, RID p_texture, int p_index) = nullptr;
	RID(*shader_get_default_texture_parameter)
	(RID p_shader, const String& p_name, int p_index) = nullptr;
	RenderingServerTypes::ShaderNativeSourceCode (*shader_get_native_source_code)(
		RID p_shader) = nullptr;
	void (*shader_embedded_set_lock)() = nullptr;
	const HashSet<RID>& (*shader_embedded_set_get)() = nullptr;
	void (*shader_embedded_set_unlock)() = nullptr;

	/* MATERIAL API */
	RID (*material_allocate)() = nullptr;
	void (*material_initialize)(RID p_rid) = nullptr;
	void (*material_free)(RID p_rid) = nullptr;
	void (*material_set_render_priority)(RID p_material, int priority) = nullptr;
	void (*material_set_shader)(RID p_shader_material, RID p_shader) = nullptr;
	void (*material_set_next_pass)(RID p_material, RID p_next_material) = nullptr;
	bool (*material_is_animated)(RID p_material) = nullptr;
	bool (*material_casts_shadows)(RID p_material) = nullptr;
	RSE::CullMode (*material_get_cull_mode)(RID p_material) = nullptr;
	void (*material_get_instance_shader_parameters)(
		RID p_material, List<InstanceShaderParam>* r_parameters) = nullptr;
	void (*material_update_dependency)(RID p_material, DependencyTracker* p_instance) = nullptr;
};

}; // namespace MaterialStorage

class RendererMaterialStorage final
{
private:
    static inline MaterialStorage::API api;

public:
    RendererMaterialStorage() = delete;
    RendererMaterialStorage(const RendererMaterialStorage&) = delete;
    RendererMaterialStorage& operator=(const RendererMaterialStorage&) = delete;
    ~RendererMaterialStorage() = delete;

    template <typename StorageImpl>
    static void bind()
    {
        /* GLOBAL SHADER UNIFORM API */
        api.global_shader_parameter_remove = &StorageImpl::global_shader_parameter_remove;
        api.global_shader_parameter_get_list = &StorageImpl::global_shader_parameter_get_list;
        api.global_shader_parameter_get_type = &StorageImpl::global_shader_parameter_get_type;
        api.global_shader_parameters_load_settings = &StorageImpl::global_shader_parameters_load_settings;
        api.global_shader_parameters_clear = &StorageImpl::global_shader_parameters_clear;
        api.global_shader_parameters_instance_allocate = &StorageImpl::global_shader_parameters_instance_allocate;
        api.global_shader_parameters_instance_free = &StorageImpl::global_shader_parameters_instance_free;
        api.finalize = &StorageImpl::finalize;

        /* SHADER API */
        api.shader_allocate = &StorageImpl::shader_allocate;
        api.shader_initialize = &StorageImpl::shader_initialize;
        api.shader_free = &StorageImpl::shader_free;
        api.shader_set_code = &StorageImpl::shader_set_code;
        api.shader_set_path_hint = &StorageImpl::shader_set_path_hint;
        api.shader_get_code = &StorageImpl::shader_get_code;
        api.shader_set_default_texture_parameter = &StorageImpl::shader_set_default_texture_parameter;
        api.shader_get_default_texture_parameter = &StorageImpl::shader_get_default_texture_parameter;
        api.shader_get_native_source_code = &StorageImpl::shader_get_native_source_code;
        api.shader_embedded_set_lock = &StorageImpl::shader_embedded_set_lock;
        api.shader_embedded_set_get = &StorageImpl::shader_embedded_set_get;
        api.shader_embedded_set_unlock = &StorageImpl::shader_embedded_set_unlock;

        /* MATERIAL API */
        api.material_allocate = &StorageImpl::material_allocate;
        api.material_initialize = &StorageImpl::material_initialize;
        api.material_free = &StorageImpl::material_free;
        api.material_set_render_priority = &StorageImpl::material_set_render_priority;
        api.material_set_shader = &StorageImpl::material_set_shader;
        api.material_set_next_pass = &StorageImpl::material_set_next_pass;
        api.material_is_animated = &StorageImpl::material_is_animated;
        api.material_casts_shadows = &StorageImpl::material_casts_shadows;
        api.material_get_cull_mode = &StorageImpl::material_get_cull_mode;
        api.material_get_instance_shader_parameters = &StorageImpl::material_get_instance_shader_parameters;
        api.material_update_dependency = &StorageImpl::material_update_dependency;
    }

    /* GLOBAL SHADER UNIFORM API */
    _FORCE_INLINE_ static void global_shader_parameter_remove(const String& p_name)
    {
        api.global_shader_parameter_remove(p_name);
    }

    _FORCE_INLINE_ static Vector<String> global_shader_parameter_get_list()
    {
        return api.global_shader_parameter_get_list();
    }

    _FORCE_INLINE_ static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
        const String& p_name)
    {
        return api.global_shader_parameter_get_type(p_name);
    }

    _FORCE_INLINE_ static void global_shader_parameters_load_settings(bool p_load_textures = true)
    {
        api.global_shader_parameters_load_settings(p_load_textures);
    }

    _FORCE_INLINE_ static void global_shader_parameters_clear()
    {
        api.global_shader_parameters_clear();
    }

    _FORCE_INLINE_ static int32_t global_shader_parameters_instance_allocate(RID p_instance)
    {
        return api.global_shader_parameters_instance_allocate(p_instance);
    }

    _FORCE_INLINE_ static void global_shader_parameters_instance_free(RID p_instance)
    {
        api.global_shader_parameters_instance_free(p_instance);
    }

    /* SHADER API */
    _FORCE_INLINE_ static RID shader_allocate()
    {
        return api.shader_allocate();
    }

    _FORCE_INLINE_ static void shader_initialize(RID p_rid, bool p_embedded = true)
    {
        api.shader_initialize(p_rid, p_embedded);
    }

    _FORCE_INLINE_ static void shader_free(RID p_rid)
    {
        api.shader_free(p_rid);
    }

    _FORCE_INLINE_ static void shader_set_code(RID p_shader, const String& p_code)
    {
        api.shader_set_code(p_shader, p_code);
    }

    _FORCE_INLINE_ static void shader_set_path_hint(RID p_shader, const String& p_path)
    {
        api.shader_set_path_hint(p_shader, p_path);
    }

    _FORCE_INLINE_ static String shader_get_code(RID p_shader)
    {
        return api.shader_get_code(p_shader);
    }

    _FORCE_INLINE_ static void shader_set_default_texture_parameter(
        RID p_shader, const String& p_name, RID p_texture, int p_index)
    {
        api.shader_set_default_texture_parameter(p_shader, p_name, p_texture, p_index);
    }

    _FORCE_INLINE_ static RID shader_get_default_texture_parameter(
        RID p_shader, const String& p_name, int p_index)
    {
        return api.shader_get_default_texture_parameter(p_shader, p_name, p_index);
    }

    _FORCE_INLINE_ static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(
        RID p_shader)
    {
        return api.shader_get_native_source_code(p_shader);
    }

    _FORCE_INLINE_ static void shader_embedded_set_lock()
    {
        api.shader_embedded_set_lock();
    }

    _FORCE_INLINE_ static const HashSet<RID>& shader_embedded_set_get()
    {
        return api.shader_embedded_set_get();
    }

    _FORCE_INLINE_ static void shader_embedded_set_unlock()
    {
        api.shader_embedded_set_unlock();
    }

    /* MATERIAL API */
    _FORCE_INLINE_ static RID material_allocate()
    {
        return api.material_allocate();
    }

    _FORCE_INLINE_ static void material_initialize(RID p_rid)
    {
        api.material_initialize(p_rid);
    }

    _FORCE_INLINE_ static void material_free(RID p_rid)
    {
        api.material_free(p_rid);
    }

    _FORCE_INLINE_ static void material_set_render_priority(RID p_material, int priority)
    {
        api.material_set_render_priority(p_material, priority);
    }

    _FORCE_INLINE_ static void material_set_shader(RID p_shader_material, RID p_shader)
    {
        api.material_set_shader(p_shader_material, p_shader);
    }

    _FORCE_INLINE_ static void material_set_next_pass(RID p_material, RID p_next_material)
    {
        api.material_set_next_pass(p_material, p_next_material);
    }

    _FORCE_INLINE_ static bool material_is_animated(RID p_material)
    {
        return api.material_is_animated(p_material);
    }

    _FORCE_INLINE_ static bool material_casts_shadows(RID p_material)
    {
        return api.material_casts_shadows(p_material);
    }

    _FORCE_INLINE_ static RSE::CullMode material_get_cull_mode(RID p_material)
    {
        return api.material_get_cull_mode(p_material);
    }

    _FORCE_INLINE_ static void material_get_instance_shader_parameters(
        RID p_material, List<InstanceShaderParam>* r_parameters)
    {
        api.material_get_instance_shader_parameters(p_material, r_parameters);
    }

    _FORCE_INLINE_ static void material_update_dependency(RID p_material, DependencyTracker* p_instance)
    {
        api.material_update_dependency(p_material, p_instance);
    }

    _FORCE_INLINE_ static void finalize()
    {
        if (api.finalize) {
            api.finalize();
        }
    }
};
// --- Inlined/Folded from servers/rendering/storage/mesh_storage.h ---
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/templates/local_vector.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"


struct InterpolationData
{
    void notify_free_multimesh(RID p_rid);
    LocalVector<RID> multimesh_interpolate_update_list;
    LocalVector<RID> multimesh_transform_update_lists[2];
    LocalVector<RID>* multimesh_transform_update_list_curr =
        &multimesh_transform_update_lists[0];
    LocalVector<RID>* multimesh_transform_update_list_prev =
        &multimesh_transform_update_lists[1];
};

class RendererMeshStorage final
{
public:
    RendererMeshStorage() = delete;
    RendererMeshStorage(const RendererMeshStorage&) = delete;
    RendererMeshStorage& operator=(const RendererMeshStorage&) = delete;
    ~RendererMeshStorage() = delete;

    /* MESH API */

    inline static RID (*mesh_allocate)() = nullptr;
    inline static void (*mesh_initialize)(RID p_rid) = nullptr;
    inline static void (*mesh_free)(RID p_rid) = nullptr;

    inline static void (*mesh_set_blend_shape_count)(RID p_mesh, int p_blend_shape_count) = nullptr;
    inline static bool (*mesh_needs_instance)(RID p_mesh, bool p_has_skeleton) = nullptr;

    inline static void (*mesh_add_surface)(
        RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface) = nullptr;

    inline static int (*mesh_get_blend_shape_count)(RID p_mesh) = nullptr;

    inline static void (*mesh_set_blend_shape_mode)(RID p_mesh, RSE::BlendShapeMode p_mode) = nullptr;
    inline static RSE::BlendShapeMode (*mesh_get_blend_shape_mode)(RID p_mesh) = nullptr;

    inline static void (*mesh_surface_update_vertex_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_attribute_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_skin_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_index_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;

    inline static void (*mesh_surface_set_material)(RID p_mesh, int p_surface, RID p_material) = nullptr;
    inline static RID (*mesh_surface_get_material)(RID p_mesh, int p_surface) = nullptr;

    inline static RenderingServerTypes::SurfaceData (*mesh_get_surface)(RID p_mesh, int p_surface) = nullptr;

    inline static RID (*mesh_surface_get_vertex_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_attribute_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_skin_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_index_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;

    inline static int (*mesh_get_surface_count)(RID p_mesh) = nullptr;

    inline static void (*mesh_set_custom_aabb)(RID p_mesh, const AABB& p_aabb) = nullptr;
    inline static AABB (*mesh_get_custom_aabb)(RID p_mesh) = nullptr;
    inline static AABB (*mesh_get_aabb)(RID p_mesh, RID p_skeleton) = nullptr;

    inline static void (*mesh_set_path)(RID p_mesh, const String& p_path) = nullptr;
    inline static String (*mesh_get_path)(RID p_mesh) = nullptr;
    inline static void (*mesh_set_shadow_mesh)(RID p_mesh, RID p_shadow_mesh) = nullptr;

    inline static void (*mesh_clear)(RID p_mesh) = nullptr;
    inline static void (*mesh_surface_remove)(RID p_mesh, int p_surface) = nullptr;
    inline static void (*mesh_debug_usage)(List<RenderingServerTypes::MeshInfo>* r_info) = nullptr;

    /* MESH INSTANCE API */

    inline static RID (*mesh_instance_create)(RID p_base) = nullptr;
    inline static void (*mesh_instance_free)(RID p_mesh_instance) = nullptr;
    inline static void (*mesh_instance_set_skeleton)(RID p_mesh_instance, RID p_skeleton) = nullptr;
    inline static void (*mesh_instance_set_blend_shape_weight)(
        RID p_mesh_instance, int p_shape, float p_weight) = nullptr;
    inline static void (*mesh_instance_check_for_update)(RID p_mesh_instance) = nullptr;
    inline static void (*mesh_instance_set_canvas_item_transform)(
        RID p_mesh_instance, const Transform2D& p_transform) = nullptr;
    inline static void (*update_mesh_instances)() = nullptr;

    /* MULTIMESH STRUCTS & BACKEND DISPATCH HOOKS */

    struct MultiMeshInterpolator
    {
        RSE::MultimeshTransformFormat _transform_format = RSE::MULTIMESH_TRANSFORM_3D;
        bool _use_colors = false;
        bool _use_custom_data = false;

        int _stride = 0;
        int _vf_size_xform = 0;
        int _vf_size_color = 0;
        int _vf_size_data = 0;
        int _num_instances = 0;

        int quality = 0;
        bool interpolated = false;
        bool on_interpolate_update_list = false;
        bool on_transform_update_list = false;

        Vector<float> _data_prev;
        Vector<float> _data_curr;
        Vector<float> _data_interpolated;
    };

    inline static RID (*_multimesh_allocate)() = nullptr;
    inline static void (*_multimesh_initialize)(RID p_rid) = nullptr;
    inline static void (*_multimesh_free)(RID p_rid) = nullptr;

    inline static void (*_multimesh_allocate_data)(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors,
        bool p_use_custom_data, bool p_use_indirect) = nullptr;

    inline static int (*_multimesh_get_instance_count)(RID p_multimesh) = nullptr;

    inline static void (*_multimesh_set_mesh)(RID p_multimesh, RID p_mesh) = nullptr;
    inline static void (*_multimesh_instance_set_transform)(
        RID p_multimesh, int p_index, const Transform3D& p_transform) = nullptr;
    inline static void (*_multimesh_instance_set_transform_2d)(
        RID p_multimesh, int p_index, const Transform2D& p_transform) = nullptr;
    inline static void (*_multimesh_instance_set_color)(
        RID p_multimesh, int p_index, const Color& p_color) = nullptr;
    inline static void (*_multimesh_instance_set_custom_data)(
        RID p_multimesh, int p_index, const Color& p_color) = nullptr;

    inline static void (*_multimesh_set_custom_aabb)(RID p_multimesh, const AABB& p_aabb) = nullptr;
    inline static AABB (*_multimesh_get_custom_aabb)(RID p_multimesh) = nullptr;

    inline static RID (*_multimesh_get_mesh)(RID p_multimesh) = nullptr;

    inline static Transform3D (*_multimesh_instance_get_transform)(RID p_multimesh, int p_index) = nullptr;
    inline static Transform2D (*_multimesh_instance_get_transform_2d)(RID p_multimesh, int p_index) = nullptr;
    inline static Color (*_multimesh_instance_get_color)(RID p_multimesh, int p_index) = nullptr;
    inline static Color (*_multimesh_instance_get_custom_data)(RID p_multimesh, int p_index) = nullptr;

    inline static void (*_multimesh_set_buffer)(RID p_multimesh, const Vector<float>& p_buffer) = nullptr;
    inline static RID (*_multimesh_get_command_buffer_rd_rid)(RID p_multimesh) = nullptr;
    inline static RID (*_multimesh_get_buffer_rd_rid)(RID p_multimesh) = nullptr;
    inline static Vector<float> (*_multimesh_get_buffer)(RID p_multimesh) = nullptr;

    inline static void (*_multimesh_set_visible_instances)(RID p_multimesh, int p_visible) = nullptr;
    inline static int (*_multimesh_get_visible_instances)(RID p_multimesh) = nullptr;

    inline static AABB (*_multimesh_get_aabb)(RID p_multimesh) = nullptr;

    inline static MultiMeshInterpolator* (*_multimesh_get_interpolator)(RID p_multimesh) = nullptr;

    /* MULTIMESH PUBLIC FRONTEND API */

    static _FORCE_INLINE_ RID multimesh_allocate() { return _multimesh_allocate(); }
    static _FORCE_INLINE_ void multimesh_initialize(RID p_rid) { _multimesh_initialize(p_rid); }
    static _FORCE_INLINE_ void multimesh_free(RID p_rid) { _multimesh_free(p_rid); }

    static void multimesh_allocate_data(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
        bool p_use_custom_data = false, bool p_use_indirect = false);

    static int multimesh_get_instance_count(RID p_multimesh);

    static void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
    static void multimesh_instance_set_transform(
        RID p_multimesh, int p_index, const Transform3D& p_transform);
    static void multimesh_instance_set_transform_2d(
        RID p_multimesh, int p_index, const Transform2D& p_transform);
    static void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color);
    static void multimesh_instance_set_custom_data(
        RID p_multimesh, int p_index, const Color& p_color);

    static void multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb);
    static AABB multimesh_get_custom_aabb(RID p_multimesh);

    static RID multimesh_get_mesh(RID p_multimesh);

    static Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index);
    static Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
    static Color multimesh_instance_get_color(RID p_multimesh, int p_index);
    static Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

    static void multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
    static RID multimesh_get_command_buffer_rd_rid(RID p_multimesh);
    static RID multimesh_get_buffer_rd_rid(RID p_multimesh);
    static Vector<float> multimesh_get_buffer(RID p_multimesh);

    static void multimesh_set_buffer_interpolated(
        RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev);
    static void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
    static void multimesh_set_physics_interpolation_quality(
        RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality);
    static void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);
    static void multimesh_instances_reset_physics_interpolation(RID p_multimesh);

    static void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
    static int multimesh_get_visible_instances(RID p_multimesh);

    static AABB multimesh_get_aabb(RID p_multimesh);

private:
    static void _multimesh_add_to_interpolation_lists(RID p_multimesh, MultiMeshInterpolator& r_mmi);

public:
    /* SKELETON API */

    inline static RID (*skeleton_allocate)() = nullptr;
    inline static void (*skeleton_initialize)(RID p_rid) = nullptr;
    inline static void (*skeleton_free)(RID p_rid) = nullptr;

    inline static void (*skeleton_allocate_data)(
        RID p_skeleton, int p_bones, bool p_2d_skeleton) = nullptr;
    inline static void (*skeleton_set_base_transform_2d)(
        RID p_skeleton, const Transform2D& p_base_transform) = nullptr;
    inline static int (*skeleton_get_bone_count)(RID p_skeleton) = nullptr;
    inline static void (*skeleton_bone_set_transform)(
        RID p_skeleton, int p_bone, const Transform3D& p_transform) = nullptr;
    inline static Transform3D (*skeleton_bone_get_transform)(RID p_skeleton, int p_bone) = nullptr;
    inline static void (*skeleton_bone_set_transform_2d)(
        RID p_skeleton, int p_bone, const Transform2D& p_transform) = nullptr;
    inline static Transform2D (*skeleton_bone_get_transform_2d)(RID p_skeleton, int p_bone) = nullptr;

    inline static void (*skeleton_update_dependency)(RID p_base, DependencyTracker* p_instance) = nullptr;

    /* INTERPOLATION */

    inline static InterpolationData _interpolation_data;

    static void update_interpolation_tick(bool p_process = true);
    static void update_interpolation_frame(bool p_process = true);
};
// --- Inlined/Folded from servers/rendering/storage/particles_storage.h ---
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/typedefs.h"
#include "core/types.h"
#include "core/math/plane.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/vector.h"
#include "scene/resources/texture.h"

class RendererParticlesStorage
{
public:
	/* PARTICLES FUNCTION POINTERS */
	inline static RID (*particles_allocate)() = nullptr;
	inline static void (*particles_initialize)(RID p_rid) = nullptr;
	inline static void (*particles_free)(RID p_rid) = nullptr;

	inline static void (*particles_set_mode)(RID p_particles, RSE::ParticlesMode p_mode) = nullptr;

	inline static void (*particles_set_emitting)(RID p_particles, bool p_emitting) = nullptr;
	inline static bool (*particles_get_emitting)(RID p_particles) = nullptr;

	inline static void (*particles_set_amount)(RID p_particles, int p_amount) = nullptr;
	inline static void (*particles_set_amount_ratio)(RID p_particles, float p_amount_ratio) = nullptr;
	inline static void (*particles_set_lifetime)(RID p_particles, double p_lifetime) = nullptr;
	inline static void (*particles_set_one_shot)(RID p_particles, bool p_one_shot) = nullptr;
	inline static void (*particles_set_pre_process_time)(RID p_particles, double p_time) = nullptr;
	inline static void (*particles_request_process_time)(
		RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual) = nullptr;
	inline static void (*particles_set_explosiveness_ratio)(RID p_particles, real_t p_ratio) = nullptr;
	inline static void (*particles_set_randomness_ratio)(RID p_particles, real_t p_ratio) = nullptr;
	inline static void (*particles_set_custom_aabb)(RID p_particles, const AABB &p_aabb) = nullptr;
	inline static void (*particles_set_speed_scale)(RID p_particles, double p_scale) = nullptr;
	inline static void (*particles_set_use_local_coordinates)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_process_material)(RID p_particles, RID p_material) = nullptr;
	inline static RID (*particles_get_process_material)(RID p_particles) = nullptr;
	inline static void (*particles_set_fixed_fps)(RID p_particles, int p_fps) = nullptr;
	inline static void (*particles_set_interpolate)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_fractional_delta)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_collision_base_size)(RID p_particles, real_t p_size) = nullptr;

	inline static void (*particles_set_transform_align)(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align) = nullptr;
	inline static void (*particles_set_transform_align_channel_filter)(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel_filter) = nullptr;
	inline static void (*particles_set_transform_align_axis)(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis) = nullptr;

	inline static void (*particles_set_seed)(RID p_particles, uint32_t p_seed) = nullptr;

	inline static void (*particles_set_trails)(RID p_particles, bool p_enable, double p_length) = nullptr;
	inline static void (*particles_set_trail_bind_poses)(
		RID p_particles, const Vector<Transform3D> &p_bind_poses) = nullptr;

	inline static void (*particles_restart)(RID p_particles) = nullptr;
	inline static void (*particles_emit)(
		RID p_particles, const Transform3D &p_transform, const Vector3 &p_velocity,
		const Color &p_color, const Color &p_custom, uint32_t p_emit_flags) = nullptr;
	inline static void (*particles_set_subemitter)(RID p_particles, RID p_subemitter_particles) = nullptr;

	inline static bool (*particles_is_inactive)(RID p_particles) = nullptr;

	inline static void (*particles_set_draw_order)(RID p_particles, RSE::ParticlesDrawOrder p_order) = nullptr;

	inline static void (*particles_set_draw_passes)(RID p_particles, int p_count) = nullptr;
	inline static void (*particles_set_draw_pass_mesh)(RID p_particles, int p_pass, RID p_mesh) = nullptr;

	inline static void (*particles_request_process)(RID p_particles) = nullptr;
	inline static AABB (*particles_get_current_aabb)(RID p_particles) = nullptr;
	inline static AABB (*particles_get_aabb)(RID p_particles) = nullptr;

	inline static void (*particles_set_emission_transform)(
		RID p_particles, const Transform3D &p_transform) = nullptr;
	inline static void (*particles_set_emitter_velocity)(RID p_particles, const Vector3 &p_velocity) = nullptr;
	inline static void (*particles_set_interp_to_end)(RID p_particles, float p_interp_to_end) = nullptr;

	inline static int (*particles_get_draw_passes)(RID p_particles) = nullptr;
	inline static RID (*particles_get_draw_pass_mesh)(RID p_particles, int p_pass) = nullptr;

	inline static void (*particles_set_view_axis)(
		RID p_particles, const Vector3 &p_axis, const Vector3 &p_up_axis) = nullptr;

	inline static void (*particles_add_collision)(RID p_particles, RID p_particles_collision_instance) = nullptr;
	inline static void (*particles_remove_collision)(RID p_particles, RID p_particles_collision_instance) = nullptr;

	inline static void (*update_particles)() = nullptr;

	/* PARTICLES COLLISION FUNCTION POINTERS */
	inline static RID (*particles_collision_allocate)() = nullptr;
	inline static void (*particles_collision_initialize)(RID p_rid) = nullptr;
	inline static void (*particles_collision_free)(RID p_rid) = nullptr;

	inline static void (*particles_collision_set_collision_type)(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type) = nullptr;
	inline static void (*particles_collision_set_cull_mask)(
		RID p_particles_collision, uint32_t p_cull_mask) = nullptr;
	inline static void (*particles_collision_set_sphere_radius)(
		RID p_particles_collision, real_t p_radius) = nullptr;
	inline static void (*particles_collision_set_box_extents)(
		RID p_particles_collision, const Vector3 &p_extents) = nullptr;
	inline static void (*particles_collision_set_attractor_strength)(
		RID p_particles_collision, real_t p_strength) = nullptr;
	inline static void (*particles_collision_set_attractor_directionality)(
		RID p_particles_collision, real_t p_directionality) = nullptr;
	inline static void (*particles_collision_set_attractor_attenuation)(
		RID p_particles_collision, real_t p_curve) = nullptr;
	inline static void (*particles_collision_set_field_texture)(
		RID p_particles_collision, RID p_texture) = nullptr;
	inline static void (*particles_collision_height_field_update)(RID p_particles_collision) = nullptr;
	inline static void (*particles_collision_set_height_field_resolution)(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution) = nullptr;
	inline static AABB (*particles_collision_get_aabb)(RID p_particles_collision) = nullptr;
	inline static bool (*particles_collision_is_heightfield)(RID p_particles_collision) = nullptr;
	inline static uint32_t (*particles_collision_get_height_field_mask)(RID p_particles_collision) = nullptr;
	inline static void (*particles_collision_set_height_field_mask)(
		RID p_particles_collision, uint32_t p_heightfield_mask) = nullptr;
	inline static uint32_t (*particles_collision_get_cull_mask)(RID p_particles_collision) = nullptr;

	inline static RID (*particles_collision_instance_create)(RID p_collision) = nullptr;
	inline static void (*particles_collision_instance_free)(RID p_rid) = nullptr;
	inline static void (*particles_collision_instance_set_transform)(
		RID p_collision_instance, const Transform3D &p_transform) = nullptr;
	inline static void (*particles_collision_instance_set_active)(
		RID p_collision_instance, bool p_active) = nullptr;

	/* BIND TEMPLATE */
	template <typename T>
	static void bind()
	{
		particles_allocate = T::particles_allocate;
		particles_initialize = T::particles_initialize;
		particles_free = T::particles_free;

		particles_set_mode = T::particles_set_mode;

		particles_set_emitting = T::particles_set_emitting;
		particles_get_emitting = T::particles_get_emitting;

		particles_set_amount = T::particles_set_amount;
		particles_set_amount_ratio = T::particles_set_amount_ratio;
		particles_set_lifetime = T::particles_set_lifetime;
		particles_set_one_shot = T::particles_set_one_shot;
		particles_set_pre_process_time = T::particles_set_pre_process_time;
		particles_request_process_time = T::particles_request_process_time;
		particles_set_explosiveness_ratio = T::particles_set_explosiveness_ratio;
		particles_set_randomness_ratio = T::particles_set_randomness_ratio;
		particles_set_custom_aabb = T::particles_set_custom_aabb;
		particles_set_speed_scale = T::particles_set_speed_scale;
		particles_set_use_local_coordinates = T::particles_set_use_local_coordinates;
		particles_set_process_material = T::particles_set_process_material;
		particles_get_process_material = T::particles_get_process_material;
		particles_set_fixed_fps = T::particles_set_fixed_fps;
		particles_set_interpolate = T::particles_set_interpolate;
		particles_set_fractional_delta = T::particles_set_fractional_delta;
		particles_set_collision_base_size = T::particles_set_collision_base_size;

		particles_set_transform_align = T::particles_set_transform_align;
		particles_set_transform_align_channel_filter = T::particles_set_transform_align_channel_filter;
		particles_set_transform_align_axis = T::particles_set_transform_align_axis;

		particles_set_seed = T::particles_set_seed;

		particles_set_trails = T::particles_set_trails;
		particles_set_trail_bind_poses = T::particles_set_trail_bind_poses;

		particles_restart = T::particles_restart;
		particles_emit = T::particles_emit;
		particles_set_subemitter = T::particles_set_subemitter;

		particles_is_inactive = T::particles_is_inactive;

		particles_set_draw_order = T::particles_set_draw_order;

		particles_set_draw_passes = T::particles_set_draw_passes;
		particles_set_draw_pass_mesh = T::particles_set_draw_pass_mesh;

		particles_request_process = T::particles_request_process;
		particles_get_current_aabb = T::particles_get_current_aabb;
		particles_get_aabb = T::particles_get_aabb;

		particles_set_emission_transform = T::particles_set_emission_transform;
		particles_set_emitter_velocity = T::particles_set_emitter_velocity;
		particles_set_interp_to_end = T::particles_set_interp_to_end;

		particles_get_draw_passes = T::particles_get_draw_passes;
		particles_get_draw_pass_mesh = T::particles_get_draw_pass_mesh;

		particles_set_view_axis = T::particles_set_view_axis;

		particles_add_collision = T::particles_add_collision;
		particles_remove_collision = T::particles_remove_collision;

		update_particles = T::update_particles;

		particles_collision_allocate = T::particles_collision_allocate;
		particles_collision_initialize = T::particles_collision_initialize;
		particles_collision_free = T::particles_collision_free;

		particles_collision_set_collision_type = T::particles_collision_set_collision_type;
		particles_collision_set_cull_mask = T::particles_collision_set_cull_mask;
		particles_collision_set_sphere_radius = T::particles_collision_set_sphere_radius;
		particles_collision_set_box_extents = T::particles_collision_set_box_extents;
		particles_collision_set_attractor_strength = T::particles_collision_set_attractor_strength;
		particles_collision_set_attractor_directionality = T::particles_collision_set_attractor_directionality;
		particles_collision_set_attractor_attenuation = T::particles_collision_set_attractor_attenuation;
		particles_collision_set_field_texture = T::particles_collision_set_field_texture;
		particles_collision_height_field_update = T::particles_collision_height_field_update;
		particles_collision_set_height_field_resolution = T::particles_collision_set_height_field_resolution;
		particles_collision_get_aabb = T::particles_collision_get_aabb;
		particles_collision_is_heightfield = T::particles_collision_is_heightfield;
		particles_collision_get_height_field_mask = T::particles_collision_get_height_field_mask;
		particles_collision_set_height_field_mask = T::particles_collision_set_height_field_mask;
		particles_collision_get_cull_mask = T::particles_collision_get_cull_mask;

		particles_collision_instance_create = T::particles_collision_instance_create;
		particles_collision_instance_free = T::particles_collision_instance_free;
		particles_collision_instance_set_transform = T::particles_collision_instance_set_transform;
		particles_collision_instance_set_active = T::particles_collision_instance_set_active;
	}
};
// --- Inlined/Folded from servers/rendering/storage/texture_storage.h ---
#include "core/io/image.h"
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"
#include "core/math/plane.h"
#include "core/templates/hashfuncs.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"

struct RenderTargetSetOverrideFn
{
	void (*ptr)(RID, RID, RID, RID, RID) = nullptr;

	void operator=(void (*p_ptr)(RID, RID, RID, RID, RID)) { ptr = p_ptr; }

	_FORCE_INLINE_ void operator()(RID p_render_target, RID p_color_texture, RID p_depth_texture,
		RID p_velocity = RID(), RID p_velocity_depth = RID()) const
	{
		if (ptr) {
			ptr(p_render_target, p_color_texture, p_depth_texture, p_velocity, p_velocity_depth);
		}
	}
};

namespace TextureStorage
{

struct API
{
	Color (*get_default_clear_color)() = nullptr;
	/* CANVAS TEXTURE API */
	RID (*canvas_texture_allocate)() = nullptr;
	void (*canvas_texture_initialize)(RID p_rid) = nullptr;
	void (*canvas_texture_free)(RID p_rid) = nullptr;

	void (*canvas_texture_set_channel)(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture) = nullptr;
	void (*canvas_texture_set_shading_parameters)(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess) = nullptr;
	void (*canvas_texture_set_texture_filter)(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter) = nullptr;
	void (*canvas_texture_set_texture_repeat)(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat) = nullptr;
	void (*set_default_clear_color)(const Color& p_color) = nullptr;

	/* TEXTURE 2D / GENERAL API */
	RID (*texture_allocate)() = nullptr;
	void (*texture_free)(RID p_rid) = nullptr;

	void (*texture_2d_initialize)(RID p_texture, const Ref<Image>& p_image) = nullptr;
	void (*texture_2d_update)(RID p_texture, const Ref<Image>& p_image, int p_layer) = nullptr;
	Ref<Image> (*texture_2d_get)(RID p_texture) = nullptr;
	Ref<Image> (*texture_2d_layer_get)(RID p_texture, int p_layer) = nullptr;

	void (*texture_2d_placeholder_initialize)(RID p_texture) = nullptr;

	void (*texture_2d_layered_initialize)(RID p_texture, const Vector<Ref<Image>>& p_layers,
		RSE::TextureLayeredType p_layered_type) = nullptr;
	void (*texture_2d_layered_placeholder_initialize)(
		RID p_texture, RSE::TextureLayeredType p_layered_type) = nullptr;

	void (*texture_3d_initialize)(RID p_texture, Image::Format p_format, int p_width, int p_height,
		int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data) = nullptr;
	void (*texture_3d_update)(RID p_texture, const Vector<Ref<Image>>& p_data) = nullptr;
	Vector<Ref<Image>> (*texture_3d_get)(RID p_texture) = nullptr;
	void (*texture_3d_placeholder_initialize)(RID p_texture) = nullptr;

	void (*texture_proxy_initialize)(RID p_texture, RID p_base) = nullptr;
	void (*texture_proxy_update)(RID p_proxy, RID p_base) = nullptr;

	void (*texture_drawable_initialize)(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps) = nullptr;
	void (*texture_drawable_generate_mipmaps)(RID p_texture) = nullptr;
	RID (*texture_drawable_get_default_material)() = nullptr;

	void (*texture_external_initialize)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;
	void (*texture_external_update)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;

	uint64_t (*texture_get_native_handle)(RID p_texture, bool p_srgb) = nullptr;

	void (*texture_set_format)(RID p_texture, Image::Format p_format) = nullptr;
	Image::Format (*texture_get_format)(RID p_texture) = nullptr;

	void (*texture_set_path)(RID p_texture, const String& p_path) = nullptr;
	String (*texture_get_path)(RID p_texture) = nullptr;
	void (*texture_set_size_override)(RID p_texture, int p_width, int p_height) = nullptr;

	void (*texture_replace)(RID p_texture, RID p_by_texture) = nullptr;
	void (*texture_set_force_redraw_if_visible)(RID p_texture, bool p_enable) = nullptr;
	Size2 (*texture_size_with_proxy)(RID p_proxy) = nullptr;

	/* DECAL API */
	RID (*decal_allocate)() = nullptr;
	void (*decal_initialize)(RID p_rid) = nullptr;
	void (*decal_free)(RID p_rid) = nullptr;

	void (*decal_set_size)(RID p_decal, const Vector3& p_size) = nullptr;
	void (*decal_set_texture)(RID p_decal, RSE::DecalTexture p_type, RID p_texture) = nullptr;
	void (*decal_set_emission_energy)(RID p_decal, float p_energy) = nullptr;
	void (*decal_set_albedo_mix)(RID p_decal, float p_mix) = nullptr;
	void (*decal_set_modulate)(RID p_decal, const Color& p_modulate) = nullptr;
	void (*decal_set_cull_mask)(RID p_decal, uint32_t p_layers) = nullptr;
	void (*decal_set_distance_fade)(
		RID p_decal, bool p_enabled, float p_begin, float p_length) = nullptr;
	void (*decal_set_fade)(RID p_decal, float p_above, float p_below) = nullptr;
	void (*decal_set_normal_fade)(RID p_decal, float p_fade) = nullptr;

	AABB (*decal_get_aabb)(RID p_decal) = nullptr;
	uint32_t (*decal_get_cull_mask)(RID p_decal) = nullptr;

	RID (*decal_instance_create)(RID p_decal) = nullptr;
	void (*decal_instance_free)(RID p_decal_instance) = nullptr;
	void (*decal_instance_set_transform)(
		RID p_decal_instance, const Transform3D& p_transform) = nullptr;
	void (*decal_instance_set_sorting_offset)(
		RID p_decal_instance, float p_sorting_offset) = nullptr;

	/* RENDER TARGET API */
	RID (*render_target_create)() = nullptr;
	void (*render_target_free)(RID p_render_target) = nullptr;

	void (*render_target_set_position)(RID p_render_target, int p_x, int p_y) = nullptr;
	Point2i (*render_target_get_position)(RID p_render_target) = nullptr;

	void (*render_target_set_size)(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count) = nullptr;
	Size2i (*render_target_get_size)(RID p_render_target) = nullptr;

	void (*render_target_set_transparent)(RID p_render_target, bool p_is_transparent) = nullptr;
	void (*render_target_set_direct_to_screen)(
		RID p_render_target, bool p_direct_to_screen) = nullptr;

	RID (*render_target_get_texture)(RID p_render_target) = nullptr;

	void (*render_target_set_msaa)(RID p_render_target, RSE::ViewportMSAA p_msaa) = nullptr;
	RSE::ViewportMSAA (*render_target_get_msaa)(RID p_render_target) = nullptr;
	bool (*render_target_get_msaa_needs_resolve)(RID p_render_target) = nullptr;
	void (*render_target_do_msaa_resolve)(RID p_render_target) = nullptr;

	void (*render_target_set_use_hdr)(RID p_render_target, bool p_use_hdr) = nullptr;
	bool (*render_target_is_using_hdr)(RID p_render_target) = nullptr;

	void (*render_target_set_use_debanding)(RID p_render_target, bool p_use_debanding) = nullptr;

	void (*render_target_set_vrs_mode)(RID p_render_target, RSE::ViewportVRSMode p_mode) = nullptr;
	void (*render_target_set_vrs_update_mode)(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode) = nullptr;
	void (*render_target_set_vrs_texture)(RID p_render_target, RID p_texture) = nullptr;

	void (*render_target_request_clear)(RID p_render_target, const Color& p_clear_color) = nullptr;
	bool (*render_target_is_clear_requested)(RID p_render_target) = nullptr;
	Color (*render_target_get_clear_request_color)(RID p_render_target) = nullptr;
	void (*render_target_do_clear_request)(RID p_render_target) = nullptr;

	void (*render_target_set_sdf_size_and_scale)(RID p_render_target,
		RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale) = nullptr;
	Rect2i (*render_target_get_sdf_rect)(RID p_render_target) = nullptr;
	void (*render_target_mark_sdf_enabled)(RID p_render_target, bool p_enabled) = nullptr;

	RenderTargetSetOverrideFn render_target_set_override;

	RID (*render_target_get_override_color)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_depth)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_velocity)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_velocity_depth)(RID p_render_target) = nullptr;

	void (*render_target_set_render_region)(
		RID p_render_target, const Rect2i& p_render_region) = nullptr;
	Rect2i (*render_target_get_render_region)(RID p_render_target) = nullptr;

	void (*render_target_set_velocity_target_size)(
		RID p_render_target, const Size2i& p_target_size) = nullptr;
	Size2i (*render_target_get_velocity_target_size)(RID p_render_target) = nullptr;

	bool (*render_target_was_used)(RID p_render_target) = nullptr;
	void (*render_target_set_as_unused)(RID p_render_target) = nullptr;
};

}; // namespace TextureStorage

class RendererTextureStorage final
{
private:
	static inline TextureStorage::API api;

public:
	RendererTextureStorage() = delete;
	RendererTextureStorage(const RendererTextureStorage&) = delete;
	RendererTextureStorage& operator=(const RendererTextureStorage&) = delete;
	~RendererTextureStorage() = delete;

	template <typename StorageImpl> static void bind()
	{
		api.set_default_clear_color = &StorageImpl::set_default_clear_color;
		api.get_default_clear_color = &StorageImpl::get_default_clear_color;
		/* CANVAS TEXTURE API */
		api.canvas_texture_allocate = &StorageImpl::canvas_texture_allocate;
		api.canvas_texture_initialize = &StorageImpl::canvas_texture_initialize;
		api.canvas_texture_free = &StorageImpl::canvas_texture_free;
		api.canvas_texture_set_channel = &StorageImpl::canvas_texture_set_channel;
		api.canvas_texture_set_shading_parameters =
			&StorageImpl::canvas_texture_set_shading_parameters;
		api.canvas_texture_set_texture_filter = &StorageImpl::canvas_texture_set_texture_filter;
		api.canvas_texture_set_texture_repeat = &StorageImpl::canvas_texture_set_texture_repeat;

		/* TEXTURE 2D / GENERAL API */
		api.texture_allocate = &StorageImpl::texture_allocate;
		api.texture_free = &StorageImpl::texture_free;
		api.texture_2d_initialize = &StorageImpl::texture_2d_initialize;
		api.texture_2d_update = &StorageImpl::texture_2d_update;
		api.texture_2d_get = &StorageImpl::texture_2d_get;
		api.texture_2d_layer_get = &StorageImpl::texture_2d_layer_get;
		api.texture_2d_placeholder_initialize = &StorageImpl::texture_2d_placeholder_initialize;
		api.texture_2d_layered_initialize = &StorageImpl::texture_2d_layered_initialize;
		api.texture_2d_layered_placeholder_initialize =
			&StorageImpl::texture_2d_layered_placeholder_initialize;
		api.texture_3d_initialize = &StorageImpl::texture_3d_initialize;
		api.texture_3d_update = &StorageImpl::texture_3d_update;
		api.texture_3d_get = &StorageImpl::texture_3d_get;
		api.texture_3d_placeholder_initialize = &StorageImpl::texture_3d_placeholder_initialize;
		api.texture_proxy_initialize = &StorageImpl::texture_proxy_initialize;
		api.texture_proxy_update = &StorageImpl::texture_proxy_update;
		api.texture_drawable_initialize = &StorageImpl::texture_drawable_initialize;
		api.texture_drawable_generate_mipmaps = &StorageImpl::texture_drawable_generate_mipmaps;
		api.texture_drawable_get_default_material =
			&StorageImpl::texture_drawable_get_default_material;
		api.texture_external_initialize = &StorageImpl::texture_external_initialize;
		api.texture_external_update = &StorageImpl::texture_external_update;
		api.texture_get_native_handle = &StorageImpl::texture_get_native_handle;
		api.texture_set_format = &StorageImpl::texture_set_format;
		api.texture_get_format = &StorageImpl::texture_get_format;
		api.texture_set_path = &StorageImpl::texture_set_path;
		api.texture_get_path = &StorageImpl::texture_get_path;
		api.texture_set_size_override = &StorageImpl::texture_set_size_override;
		api.texture_replace = &StorageImpl::texture_replace;
		api.texture_set_force_redraw_if_visible = &StorageImpl::texture_set_force_redraw_if_visible;
		api.texture_size_with_proxy = &StorageImpl::texture_size_with_proxy;

		/* DECAL API */
		api.decal_allocate = &StorageImpl::decal_allocate;
		api.decal_initialize = &StorageImpl::decal_initialize;
		api.decal_free = &StorageImpl::decal_free;
		api.decal_set_size = &StorageImpl::decal_set_size;
		api.decal_set_texture = &StorageImpl::decal_set_texture;
		api.decal_set_emission_energy = &StorageImpl::decal_set_emission_energy;
		api.decal_set_albedo_mix = &StorageImpl::decal_set_albedo_mix;
		api.decal_set_modulate = &StorageImpl::decal_set_modulate;
		api.decal_set_cull_mask = &StorageImpl::decal_set_cull_mask;
		api.decal_set_distance_fade = &StorageImpl::decal_set_distance_fade;
		api.decal_set_fade = &StorageImpl::decal_set_fade;
		api.decal_set_normal_fade = &StorageImpl::decal_set_normal_fade;
		api.decal_get_aabb = &StorageImpl::decal_get_aabb;
		api.decal_get_cull_mask = &StorageImpl::decal_get_cull_mask;
		api.decal_instance_create = &StorageImpl::decal_instance_create;
		api.decal_instance_free = &StorageImpl::decal_instance_free;
		api.decal_instance_set_transform = &StorageImpl::decal_instance_set_transform;
		api.decal_instance_set_sorting_offset = &StorageImpl::decal_instance_set_sorting_offset;

		/* RENDER TARGET API */
		api.render_target_create = &StorageImpl::render_target_create;
		api.render_target_free = &StorageImpl::render_target_free;
		api.render_target_set_position = &StorageImpl::render_target_set_position;
		api.render_target_get_position = &StorageImpl::render_target_get_position;
		api.render_target_set_size = &StorageImpl::render_target_set_size;
		api.render_target_get_size = &StorageImpl::render_target_get_size;
		api.render_target_set_transparent = &StorageImpl::render_target_set_transparent;
		api.render_target_set_direct_to_screen = &StorageImpl::render_target_set_direct_to_screen;
		api.render_target_get_texture = &StorageImpl::render_target_get_texture;
		api.render_target_set_msaa = &StorageImpl::render_target_set_msaa;
		api.render_target_get_msaa = &StorageImpl::render_target_get_msaa;
		api.render_target_get_msaa_needs_resolve =
			&StorageImpl::render_target_get_msaa_needs_resolve;
		api.render_target_do_msaa_resolve = &StorageImpl::render_target_do_msaa_resolve;
		api.render_target_set_use_hdr = &StorageImpl::render_target_set_use_hdr;
		api.render_target_is_using_hdr = &StorageImpl::render_target_is_using_hdr;
		api.render_target_set_use_debanding = &StorageImpl::render_target_set_use_debanding;
		api.render_target_set_vrs_mode = &StorageImpl::render_target_set_vrs_mode;
		api.render_target_set_vrs_update_mode = &StorageImpl::render_target_set_vrs_update_mode;
		api.render_target_set_vrs_texture = &StorageImpl::render_target_set_vrs_texture;
		api.render_target_request_clear = &StorageImpl::render_target_request_clear;
		api.render_target_is_clear_requested = &StorageImpl::render_target_is_clear_requested;
		api.render_target_get_clear_request_color =
			&StorageImpl::render_target_get_clear_request_color;
		api.render_target_do_clear_request = &StorageImpl::render_target_do_clear_request;
		api.render_target_set_sdf_size_and_scale =
			&StorageImpl::render_target_set_sdf_size_and_scale;
		api.render_target_get_sdf_rect = &StorageImpl::render_target_get_sdf_rect;
		api.render_target_mark_sdf_enabled = &StorageImpl::render_target_mark_sdf_enabled;
		api.render_target_set_override = &StorageImpl::render_target_set_override;
		api.render_target_get_override_color = &StorageImpl::render_target_get_override_color;
		api.render_target_get_override_depth = &StorageImpl::render_target_get_override_depth;
		api.render_target_get_override_velocity = &StorageImpl::render_target_get_override_velocity;
		api.render_target_get_override_velocity_depth =
			&StorageImpl::render_target_get_override_velocity_depth;
		api.render_target_set_render_region = &StorageImpl::render_target_set_render_region;
		api.render_target_get_render_region = &StorageImpl::render_target_get_render_region;
		api.render_target_set_velocity_target_size =
			&StorageImpl::render_target_set_velocity_target_size;
		api.render_target_get_velocity_target_size =
			&StorageImpl::render_target_get_velocity_target_size;
		api.render_target_was_used = &StorageImpl::render_target_was_used;
		api.render_target_set_as_unused = &StorageImpl::render_target_set_as_unused;
	}

	/* CANVAS TEXTURE API */
	_FORCE_INLINE_ static RID canvas_texture_allocate() { return api.canvas_texture_allocate(); }

	_FORCE_INLINE_ static void canvas_texture_initialize(RID p_rid)
	{
		api.canvas_texture_initialize(p_rid);
	}

	_FORCE_INLINE_ static void canvas_texture_free(RID p_rid) { api.canvas_texture_free(p_rid); }

	_FORCE_INLINE_ static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
	{
		api.canvas_texture_set_channel(p_canvas_texture, p_channel, p_texture);
	}

	_FORCE_INLINE_ static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess)
	{
		api.canvas_texture_set_shading_parameters(p_canvas_texture, p_base_color, p_shininess);
	}

	_FORCE_INLINE_ static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		api.canvas_texture_set_texture_filter(p_canvas_texture, p_filter);
	}

	_FORCE_INLINE_ static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
	{
		api.canvas_texture_set_texture_repeat(p_canvas_texture, p_repeat);
	}

	/* TEXTURE 2D / GENERAL API */
	_FORCE_INLINE_ static RID texture_allocate() { return api.texture_allocate(); }

	_FORCE_INLINE_ static void texture_free(RID p_rid) { api.texture_free(p_rid); }

	_FORCE_INLINE_ static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
	{
		api.texture_2d_initialize(p_texture, p_image);
	}

	_FORCE_INLINE_ static void texture_2d_update(
		RID p_texture, const Ref<Image>& p_image, int p_layer)
	{
		api.texture_2d_update(p_texture, p_image, p_layer);
	}

	_FORCE_INLINE_ static Ref<Image> texture_2d_get(RID p_texture)
	{
		return api.texture_2d_get(p_texture);
	}

	_FORCE_INLINE_ static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer)
	{
		return api.texture_2d_layer_get(p_texture, p_layer);
	}

	_FORCE_INLINE_ static void texture_2d_placeholder_initialize(RID p_texture)
	{
		api.texture_2d_placeholder_initialize(p_texture);
	}

	_FORCE_INLINE_ static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
	{
		api.texture_2d_layered_initialize(p_texture, p_layers, p_layered_type);
	}

	_FORCE_INLINE_ static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type)
	{
		api.texture_2d_layered_placeholder_initialize(p_texture, p_layered_type);
	}

	_FORCE_INLINE_ static void texture_3d_initialize(RID p_texture, Image::Format p_format,
		int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
	{
		api.texture_3d_initialize(
			p_texture, p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	}

	_FORCE_INLINE_ static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
	{
		api.texture_3d_update(p_texture, p_data);
	}

	_FORCE_INLINE_ static Vector<Ref<Image>> texture_3d_get(RID p_texture)
	{
		return api.texture_3d_get(p_texture);
	}

	_FORCE_INLINE_ static void texture_3d_placeholder_initialize(RID p_texture)
	{
		api.texture_3d_placeholder_initialize(p_texture);
	}

	_FORCE_INLINE_ static void texture_proxy_initialize(RID p_texture, RID p_base)
	{
		api.texture_proxy_initialize(p_texture, p_base);
	}

	_FORCE_INLINE_ static void texture_proxy_update(RID p_proxy, RID p_base)
	{
		api.texture_proxy_update(p_proxy, p_base);
	}

	_FORCE_INLINE_ static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps)
	{
		api.texture_drawable_initialize(
			p_texture, p_width, p_height, p_format, p_color, p_has_mipmaps);
	}

	_FORCE_INLINE_ static void texture_drawable_generate_mipmaps(RID p_texture)
	{
		api.texture_drawable_generate_mipmaps(p_texture);
	}

	_FORCE_INLINE_ static RID texture_drawable_get_default_material()
	{
		return api.texture_drawable_get_default_material();
	}

	_FORCE_INLINE_ static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		api.texture_external_initialize(p_texture, p_width, p_height, p_external_buffer);
	}

	_FORCE_INLINE_ static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		api.texture_external_update(p_texture, p_width, p_height, p_external_buffer);
	}

	_FORCE_INLINE_ static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb)
	{
		return api.texture_get_native_handle(p_texture, p_srgb);
	}

	_FORCE_INLINE_ static void texture_set_format(RID p_texture, Image::Format p_format)
	{
		api.texture_set_format(p_texture, p_format);
	}

	_FORCE_INLINE_ static Image::Format texture_get_format(RID p_texture)
	{
		return api.texture_get_format(p_texture);
	}

	_FORCE_INLINE_ static void texture_set_path(RID p_texture, const String& p_path)
	{
		api.texture_set_path(p_texture, p_path);
	}

	_FORCE_INLINE_ static String texture_get_path(RID p_texture)
	{
		return api.texture_get_path(p_texture);
	}

	_FORCE_INLINE_ static void texture_set_size_override(RID p_texture, int p_width, int p_height)
	{
		api.texture_set_size_override(p_texture, p_width, p_height);
	}

	_FORCE_INLINE_ static void texture_replace(RID p_texture, RID p_by_texture)
	{
		api.texture_replace(p_texture, p_by_texture);
	}

	_FORCE_INLINE_ static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable)
	{
		api.texture_set_force_redraw_if_visible(p_texture, p_enable);
	}

	_FORCE_INLINE_ static Size2 texture_size_with_proxy(RID p_proxy)
	{
		return api.texture_size_with_proxy(p_proxy);
	}

	/* DECAL API */
	_FORCE_INLINE_ static RID decal_allocate() { return api.decal_allocate(); }

	_FORCE_INLINE_ static void decal_initialize(RID p_rid) { api.decal_initialize(p_rid); }

	_FORCE_INLINE_ static void decal_free(RID p_rid) { api.decal_free(p_rid); }

	_FORCE_INLINE_ static void decal_set_size(RID p_decal, const Vector3& p_size)
	{
		api.decal_set_size(p_decal, p_size);
	}

	_FORCE_INLINE_ static void decal_set_texture(
		RID p_decal, RSE::DecalTexture p_type, RID p_texture)
	{
		api.decal_set_texture(p_decal, p_type, p_texture);
	}

	_FORCE_INLINE_ static void decal_set_emission_energy(RID p_decal, float p_energy)
	{
		api.decal_set_emission_energy(p_decal, p_energy);
	}

	_FORCE_INLINE_ static void decal_set_albedo_mix(RID p_decal, float p_mix)
	{
		api.decal_set_albedo_mix(p_decal, p_mix);
	}

	_FORCE_INLINE_ static void decal_set_modulate(RID p_decal, const Color& p_modulate)
	{
		api.decal_set_modulate(p_decal, p_modulate);
	}

	_FORCE_INLINE_ static void decal_set_cull_mask(RID p_decal, uint32_t p_layers)
	{
		api.decal_set_cull_mask(p_decal, p_layers);
	}

	_FORCE_INLINE_ static void decal_set_distance_fade(
		RID p_decal, bool p_enabled, float p_begin, float p_length)
	{
		api.decal_set_distance_fade(p_decal, p_enabled, p_begin, p_length);
	}

	_FORCE_INLINE_ static void decal_set_fade(RID p_decal, float p_above, float p_below)
	{
		api.decal_set_fade(p_decal, p_above, p_below);
	}

	_FORCE_INLINE_ static void decal_set_normal_fade(RID p_decal, float p_fade)
	{
		api.decal_set_normal_fade(p_decal, p_fade);
	}

	_FORCE_INLINE_ static AABB decal_get_aabb(RID p_decal) { return api.decal_get_aabb(p_decal); }

	_FORCE_INLINE_ static uint32_t decal_get_cull_mask(RID p_decal)
	{
		return api.decal_get_cull_mask(p_decal);
	}

	_FORCE_INLINE_ static RID decal_instance_create(RID p_decal)
	{
		return api.decal_instance_create(p_decal);
	}

	_FORCE_INLINE_ static void decal_instance_free(RID p_decal_instance)
	{
		api.decal_instance_free(p_decal_instance);
	}

	_FORCE_INLINE_ static void decal_instance_set_transform(
		RID p_decal_instance, const Transform3D& p_transform)
	{
		api.decal_instance_set_transform(p_decal_instance, p_transform);
	}

	_FORCE_INLINE_ static void decal_instance_set_sorting_offset(
		RID p_decal_instance, float p_sorting_offset)
	{
		api.decal_instance_set_sorting_offset(p_decal_instance, p_sorting_offset);
	}

	/* RENDER TARGET API */
	_FORCE_INLINE_ static RID render_target_create() { return api.render_target_create(); }

	_FORCE_INLINE_ static void render_target_free(RID p_render_target)
	{
		api.render_target_free(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_position(RID p_render_target, int p_x, int p_y)
	{
		api.render_target_set_position(p_render_target, p_x, p_y);
	}

	_FORCE_INLINE_ static Point2i render_target_get_position(RID p_render_target)
	{
		return api.render_target_get_position(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
	{
		api.render_target_set_size(p_render_target, p_width, p_height, p_view_count);
	}

	_FORCE_INLINE_ static Size2i render_target_get_size(RID p_render_target)
	{
		return api.render_target_get_size(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_transparent(
		RID p_render_target, bool p_is_transparent)
	{
		api.render_target_set_transparent(p_render_target, p_is_transparent);
	}

	_FORCE_INLINE_ static void render_target_set_direct_to_screen(
		RID p_render_target, bool p_direct_to_screen)
	{
		api.render_target_set_direct_to_screen(p_render_target, p_direct_to_screen);
	}

	_FORCE_INLINE_ static RID render_target_get_texture(RID p_render_target)
	{
		return api.render_target_get_texture(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
	{
		api.render_target_set_msaa(p_render_target, p_msaa);
	}

	_FORCE_INLINE_ static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target)
	{
		return api.render_target_get_msaa(p_render_target);
	}

	_FORCE_INLINE_ static bool render_target_get_msaa_needs_resolve(RID p_render_target)
	{
		return api.render_target_get_msaa_needs_resolve(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_do_msaa_resolve(RID p_render_target)
	{
		api.render_target_do_msaa_resolve(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
	{
		api.render_target_set_use_hdr(p_render_target, p_use_hdr);
	}

	_FORCE_INLINE_ static bool render_target_is_using_hdr(RID p_render_target)
	{
		return api.render_target_is_using_hdr(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_use_debanding(
		RID p_render_target, bool p_use_debanding)
	{
		api.render_target_set_use_debanding(p_render_target, p_use_debanding);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_mode(
		RID p_render_target, RSE::ViewportVRSMode p_mode)
	{
		api.render_target_set_vrs_mode(p_render_target, p_mode);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
		api.render_target_set_vrs_update_mode(p_render_target, p_mode);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_texture(RID p_render_target, RID p_texture)
	{
		api.render_target_set_vrs_texture(p_render_target, p_texture);
	}

	_FORCE_INLINE_ static void render_target_request_clear(
		RID p_render_target, const Color& p_clear_color)
	{
		api.render_target_request_clear(p_render_target, p_clear_color);
	}

	_FORCE_INLINE_ static bool render_target_is_clear_requested(RID p_render_target)
	{
		return api.render_target_is_clear_requested(p_render_target);
	}

	_FORCE_INLINE_ static Color render_target_get_clear_request_color(RID p_render_target)
	{
		return api.render_target_get_clear_request_color(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_do_clear_request(RID p_render_target)
	{
		api.render_target_do_clear_request(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
	{
		api.render_target_set_sdf_size_and_scale(p_render_target, p_size, p_scale);
	}

	_FORCE_INLINE_ static Rect2i render_target_get_sdf_rect(RID p_render_target)
	{
		return api.render_target_get_sdf_rect(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled)
	{
		api.render_target_mark_sdf_enabled(p_render_target, p_enabled);
	}

	_FORCE_INLINE_ static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity = RID(), RID p_velocity_depth = RID())
	{
		api.render_target_set_override(
			p_render_target, p_color_texture, p_depth_texture, p_velocity, p_velocity_depth);
	}

	_FORCE_INLINE_ static RID render_target_get_override_color(RID p_render_target)
	{
		return api.render_target_get_override_color(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_depth(RID p_render_target)
	{
		return api.render_target_get_override_depth(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_velocity(RID p_render_target)
	{
		return api.render_target_get_override_velocity(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_velocity_depth(RID p_render_target)
	{
		return api.render_target_get_override_velocity_depth(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_render_region(
		RID p_render_target, const Rect2i& p_render_region)
	{
		api.render_target_set_render_region(p_render_target, p_render_region);
	}

	_FORCE_INLINE_ static Rect2i render_target_get_render_region(RID p_render_target)
	{
		return api.render_target_get_render_region(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{
		api.render_target_set_velocity_target_size(p_render_target, p_target_size);
	}

	_FORCE_INLINE_ static Size2i render_target_get_velocity_target_size(RID p_render_target)
	{
		return api.render_target_get_velocity_target_size(p_render_target);
	}

	_FORCE_INLINE_ static bool render_target_was_used(RID p_render_target)
	{
		return api.render_target_was_used(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_as_unused(RID p_render_target)
	{
		api.render_target_set_as_unused(p_render_target);
	}

	_FORCE_INLINE_ static void set_default_clear_color(const Color& p_color)
	{
		if (api.set_default_clear_color) {
			api.set_default_clear_color(p_color);
		}
	}

	_FORCE_INLINE_ static Color get_default_clear_color()
	{
		if (api.get_default_clear_color) {
			return api.get_default_clear_color();
		}
		return Color();

	}
};

namespace Geometry3D
{
struct MeshData;
}

#ifdef DEBUG_ENABLED
#define ERR_NOT_ON_RENDER_THREAD ERR_FAIL_COND(!Renderer::is_on_render_thread());
#define ERR_NOT_ON_RENDER_THREAD_V(m_ret) ERR_FAIL_COND_V(!Renderer::is_on_render_thread(), m_ret);
#else
#define ERR_NOT_ON_RENDER_THREAD
#define ERR_NOT_ON_RENDER_THREAD_V(m_ret)
#endif

class RenderingDevice;
class RendererCanvasRender;
class RendererCanvasCull;
class RendererViewport;
class RenderingMethod;

class Renderer final
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	static inline const Vector2 SMALL_VEC2 = Vector2(CMP_EPSILON, CMP_EPSILON);
	static inline const Vector3 SMALL_VEC3 = Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON);

private:
	struct Data
	{
		struct Backend
		{
			enum
			{
				MAX_INSTANCE_CULL = 8192,
				MAX_INSTANCE_LIGHTS = 4,
				LIGHT_CACHE_DIRTY = -1,
				MAX_LIGHTS_CULLED = 256,
				MAX_ROOM_CULL = 32,
				MAX_EXTERIOR_PORTALS = 128,
				MAX_LIGHT_SAMPLERS = 256,
				INSTANCE_ROOMLESS_MASK = (1 << 20)
			};

			// Active subsystem driver
			RendererCompositor* compositor = nullptr;

			// Frame profiling & debug tracking
			uint64_t frame_profile_frame = 0;
			Vector<RenderingServerTypes::FrameProfileArea> frame_profile;
			double frame_setup_time = 0.0;
			bool print_gpu_profile = false;
			HashMap<String, float> print_gpu_profile_task_time;
			uint64_t print_frame_profile_ticks_from = 0;
			uint32_t print_frame_profile_frame_count = 0;

			int changes = 0;
			RID test_cube;
		};

		int mm_policy = 0;
		bool render_loop_enabled = true;

		RID test_texture;
		RID white_texture;
		RID test_material;

		Backend backend;
	};

	static RID _make_test_cube();
	static void _free_internal_rids();

public:
	static inline Data* data = nullptr;

	/* SUBSYSTEM & STORAGE POINTERS */
	static inline bool threaded = false;

	static inline RendererParticlesStorage* particles_storage = nullptr;
	static inline RendererFog* fog = nullptr;
	static inline RendererCanvasRender* canvas_render = nullptr;

	static inline RendererCanvasCull* canvas = nullptr;
	static inline RendererViewport* viewport = nullptr;

	static void fix_surface_compatibility(
		RenderingServerTypes::SurfaceData& p_surface, const String& p_path = String());
	static void set_boot_image(
		const Ref<Image>& p_image, const Color& p_color, bool p_scale, bool p_use_filter);

	/* SERVER LIFECYCLE */

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	/* TEXTURE API */

	static RID texture_2d_create(const Ref<Image>& p_image);
	static RID texture_2d_layered_create(
		const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type);
	static RID texture_3d_create(Image::Format p_format, int p_width, int p_height, int p_depth,
		bool p_mipmaps, const Vector<Ref<Image>>& p_data);
	static RID texture_external_create(int p_width, int p_height, uint64_t p_external_buffer = 0);
	static RID texture_proxy_create(RID p_base);
	static RID texture_drawable_create(int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color = Color(1, 1, 1, 1),
		bool p_with_mipmaps = false);

	static RID texture_create_from_native_handle(RSE::TextureType p_type, Image::Format p_format,
		uint64_t p_native_handle, int p_width, int p_height, int p_depth, int p_layers = 1,
		RSE::TextureLayeredType p_layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY);

	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer = 0);
	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data);
	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer = 0);
	static void texture_proxy_update(RID p_texture, RID p_proxy_to);

	static RID texture_2d_placeholder_create();
	static RID texture_2d_layered_placeholder_create(RSE::TextureLayeredType p_layered_type);
	static RID texture_3d_placeholder_create();

	static Ref<Image> texture_2d_get(RID p_texture);
	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer);
	static Vector<Ref<Image>> texture_3d_get(RID p_texture);

	static void texture_replace(RID p_texture, RID p_by_texture);
	static void texture_set_size_override(RID p_texture, int p_width, int p_height);

	static void texture_set_path(RID p_texture, const String& p_path);
	static String texture_get_path(RID p_texture);

	static void texture_drawable_generate_mipmaps(RID p_texture);
	static RID texture_drawable_get_default_material();

	static Image::Format texture_get_format(RID p_texture);

	static void texture_set_detect_3d_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_normal_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_roughness_callback(RID p_texture,
		RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata);

	static void texture_debug_usage(List<RenderingServerTypes::TextureInfo>* r_info);

	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	static RID texture_rd_create(const RID& p_rd_texture,
		const RSE::TextureLayeredType p_layer_type = RSE::TEXTURE_LAYERED_2D_ARRAY);
	static RID texture_get_rd_texture(RID p_texture, bool p_srgb = false);
	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb = false);

	/* SHADER API */

	static RID shader_create();
	static RID shader_create_from_code(const String& p_code, const String& p_path_hint = String());

	static void shader_set_code(RID p_shader, const String& p_code);
	static void shader_set_path_hint(RID p_shader, const String& p_path);
	static String shader_get_code(RID p_shader);

	static void shader_set_default_texture_parameter(
		RID p_shader, const String& p_name, RID p_texture, int p_index = 0);
	static RID shader_get_default_texture_parameter(
		RID p_shader, const String& p_name, int p_index = 0);

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader);

	/* COMMON MATERIAL API */

	static RID material_create();
	static RID material_create_from_shader(RID p_next_pass, int p_render_priority, RID p_shader);

	static void material_set_shader(RID p_shader_material, RID p_shader);
	static void material_set_render_priority(RID p_material, int priority);
	static void material_set_next_pass(RID p_material, RID p_next_material);
	static void material_set_use_debanding(bool p_enable);

	/* MESH API */

	static RID mesh_create_from_surfaces(
		const Vector<RenderingServerTypes::SurfaceData>& p_surfaces, int p_blend_shape_count = 0);
	static RID mesh_create();

	static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count);

	static uint32_t mesh_surface_get_format_offset(
		uint32_t p_format, int p_vertex_len, int p_array_index);
	static uint32_t mesh_surface_get_format_vertex_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_normal_tangent_stride(
		uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_attribute_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_skin_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_index_stride(uint32_t p_format, int p_vertex_len);

	static void mesh_surface_make_offsets_from_format(uint64_t p_format, int p_vertex_len,
		int p_index_len, uint32_t* r_offsets, uint32_t& r_vertex_element_size,
		uint32_t& r_normal_element_size, uint32_t& r_attrib_element_size,
		uint32_t& r_skin_element_size);

	static void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface);

	static int mesh_get_blend_shape_count(RID p_mesh);

	static void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode);
	static RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh);

	static void mesh_surface_update_vertex_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_attribute_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_skin_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_index_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);

	static void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	static RID mesh_surface_get_material(RID p_mesh, int p_surface);

	static RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface);

	static int mesh_get_surface_count(RID p_mesh);

	static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB mesh_get_custom_aabb(RID p_mesh);

	static void mesh_set_path(RID p_mesh, const String& p_path);
	static String mesh_get_path(RID p_mesh);

	static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh);

	static void mesh_surface_remove(RID p_mesh, int p_surface);
	static void mesh_clear(RID p_mesh);

	static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface);

	static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info);

	/* MULTIMESH API */

	static RID multimesh_create();

	static void multimesh_allocate_data(RID p_multimesh, int p_instances,
		RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
		bool p_use_custom_data = false, bool p_use_indirect = false);
	static int multimesh_get_instance_count(RID p_multimesh);

	static void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
	static void multimesh_instance_set_transform(
		RID p_multimesh, int p_index, const Transform3D& p_transform);
	static void multimesh_instance_set_transform_2d(
		RID p_multimesh, int p_index, const Transform2D& p_transform);
	static void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color);
	static void multimesh_instance_set_custom_data(
		RID p_multimesh, int p_index, const Color& p_color);

	static RID multimesh_get_mesh(RID p_multimesh);
	static AABB multimesh_get_aabb(RID p_multimesh);

	static void multimesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB multimesh_get_custom_aabb(RID p_mesh);

	static Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index);
	static Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
	static Color multimesh_instance_get_color(RID p_multimesh, int p_index);
	static Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

	static void multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
	static RID multimesh_get_command_buffer_rd_rid(RID p_multimesh);
	static RID multimesh_get_buffer_rd_rid(RID p_multimesh);
	static Vector<float> multimesh_get_buffer(RID p_multimesh);

	static void multimesh_set_buffer_interpolated(
		RID p_multimesh, const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev);
	static void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
	static void multimesh_set_physics_interpolation_quality(
		RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality);
	static void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);
	static void multimesh_instances_reset_physics_interpolation(RID p_multimesh);

	static void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	static int multimesh_get_visible_instances(RID p_multimesh);

	/* SKELETON API */

	static RID skeleton_create();
	static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false);
	static int skeleton_get_bone_count(RID p_skeleton);
	static void skeleton_bone_set_transform(
		RID p_skeleton, int p_bone, const Transform3D& p_transform);
	static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone);
	static void skeleton_bone_set_transform_2d(
		RID p_skeleton, int p_bone, const Transform2D& p_transform);
	static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone);
	static void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D& p_base_transform);

	/* LIGHT API */

	static RID directional_light_create();
	static RID omni_light_create();
	static RID spot_light_create();
	static RID area_light_create();

	static void light_set_color(RID p_light, const Color& p_color);
	static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value);
	static void light_set_shadow(RID p_light, bool p_enabled);
	static void light_set_projector(RID p_light, RID p_texture);
	static void light_set_negative(RID p_light, bool p_enable);
	static void light_set_cull_mask(RID p_light, uint32_t p_mask);
	static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length);
	static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled);
	static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask);

	static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode);
	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade);

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode);

	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode);
	static void light_directional_set_blend_splits(RID p_light, bool p_enable);
	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode);

	static void light_area_set_size(RID p_light, const Vector2& p_size);
	static void light_area_set_normalize_energy(RID p_light, bool p_enabled);
	static void light_area_set_texture(RID p_light, RID p_texture);

	static RID shadow_atlas_create();
	static void shadow_atlas_set_size(RID p_atlas, int p_size, bool p_use_16_bits = true);
	static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision);

	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true);

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	static void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);

	/* REFLECTION PROBE API */

	static RID reflection_probe_create();

	static void reflection_probe_set_update_mode(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode);
	static void reflection_probe_set_intensity(RID p_probe, float p_intensity);
	static void reflection_probe_set_blend_distance(RID p_probe, float p_blend_distance);

	static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode);
	static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color);
	static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy);
	static void reflection_probe_set_max_distance(RID p_probe, float p_distance);
	static void reflection_probe_set_size(RID p_probe, const Vector3& p_size);
	static void reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset);
	static void reflection_probe_set_as_interior(RID p_probe, bool p_enable);
	static void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable);
	static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable);
	static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers);
	static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers);
	static void reflection_probe_set_resolution(RID p_probe, int p_resolution);
	static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_pixels);

	/* DECAL API */

	static RID decal_create();
	static void decal_set_size(RID p_decal, const Vector3& p_size);
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture);
	static void decal_set_emission_energy(RID p_decal, float p_energy);
	static void decal_set_albedo_mix(RID p_decal, float p_mix);
	static void decal_set_modulate(RID p_decal, const Color& p_modulate);
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers);
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length);
	static void decal_set_fade(RID p_decal, float p_above, float p_below);
	static void decal_set_normal_fade(RID p_decal, float p_fade);

	static void decals_set_filter(RSE::DecalFilter p_quality);

	/* VOXEL GI API */

	static RID voxel_gi_create();

	static void voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts);

	static AABB voxel_gi_get_bounds(RID p_voxel_gi);
	static Vector3i voxel_gi_get_octree_size(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_octree_cells(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_data_cells(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_distance_field(RID p_voxel_gi);
	static Vector<int> voxel_gi_get_level_counts(RID p_voxel_gi);
	static Transform3D voxel_gi_get_to_cell_xform(RID p_voxel_gi);

	static void voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_propagation(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_energy(RID p_voxel_gi, float p_energy);
	static void voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure);
	static void voxel_gi_set_bias(RID p_voxel_gi, float p_bias);
	static void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable);
	static void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable);

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_quality);

	static void sdfgi_reset();

	/* LIGHTMAP API */

	static RID lightmap_create();

	static void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_haromics);
	static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds);
	static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior);
	static void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree);
	static void lightmap_set_baked_exposure_normalization(RID p_lightmap, float p_exposure);
	static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap);
	static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap);
	static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap);
	static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap);

	static void lightmap_set_probe_capture_update_speed(float p_speed);
	static void lightmaps_set_bicubic_filter(bool p_enable);

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow);
	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap);
	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode);

	/* PARTICLES API */

	static RID particles_create();
	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode);

	static void particles_set_emitting(RID p_particles, bool p_enable);
	static bool particles_get_emitting(RID p_particles);
	static void particles_set_amount(RID p_particles, int p_amount);
	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio);
	static void particles_set_lifetime(RID p_particles, double p_lifetime);
	static void particles_set_one_shot(RID p_particles, bool p_one_shot);
	static void particles_set_pre_process_time(RID p_particles, double p_time);
	static void particles_request_process_time(RID p_particles, real_t p_request_process_time,
		real_t p_request_process_time_residual = 0.0);
	static void particles_set_explosiveness_ratio(RID p_particles, float p_ratio);
	static void particles_set_randomness_ratio(RID p_particles, float p_ratio);
	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb);
	static void particles_set_speed_scale(RID p_particles, double p_scale);
	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable);
	static void particles_set_process_material(RID p_particles, RID p_material);
	static void particles_set_fixed_fps(RID p_particles, int p_fps);
	static void particles_set_interpolate(RID p_particles, bool p_enable);
	static void particles_set_fractional_delta(RID p_particles, bool p_enable);
	static void particles_set_collision_base_size(RID p_particles, float p_size);
	static void particles_set_seed(RID p_particles, uint32_t p_seed);

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align);
	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter);
	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis);

	static void particles_set_trails(RID p_particles, bool p_enable, float p_length_sec);
	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses);

	static bool particles_is_inactive(RID p_particles);
	static void particles_request_process(RID p_particles);
	static void particles_restart(RID p_particles);

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles);

	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags);

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order);

	static void particles_set_draw_passes(RID p_particles, int p_count);
	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh);

	static AABB particles_get_current_aabb(RID p_particles);

	static void particles_set_emission_transform(RID p_particles, const Transform3D& p_transform);
	static void particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity);
	static void particles_set_interp_to_end(RID p_particles, float p_interp);

	/* PARTICLES COLLISION API */

	static RID particles_collision_create();

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type);
	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask);
	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius);
	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents);
	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength);
	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality);
	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve);
	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture);

	static void particles_collision_height_field_update(RID p_particles_collision);

	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution);
	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask);

	/* FOG VOLUME API */

	static RID fog_volume_create();

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape);
	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size);
	static void fog_volume_set_material(RID p_fog_volume, RID p_material);

	/* VISIBILITY NOTIFIER API */

	static RID visibility_notifier_create();
	static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb);

	/* OCCLUDER API */

	static RID occluder_create();
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);

	/* CAMERA API */

	static RID camera_create();
	static void camera_set_perspective(
		RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	static void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	static void camera_set_frustum(
		RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	static void camera_set_transform(RID p_camera, const Transform3D& p_transform);
	static void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	static void camera_set_environment(RID p_camera, RID p_env);
	static void camera_set_camera_attributes(RID p_camera, RID p_camera_attributes);
	static void camera_set_compositor(RID p_camera, RID p_compositor);
	static void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);

	/* VIEWPORT API */

	static RID viewport_create();

#ifndef XR_DISABLED
	static void viewport_set_use_xr(RID p_viewport, bool p_use_xr);
#endif // !XR_DISABLED

	static void viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count = 1);
	static void viewport_set_active(RID p_viewport, bool p_active);
	static void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);
	static void viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask);

	static void viewport_attach_to_screen(RID p_viewport, const Rect2& p_rect = Rect2(),
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);

	static void viewport_set_scaling_3d_mode(
		RID p_viewport, RSE::ViewportScaling3DMode p_scaling_3d_mode);
	static void viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale);
	static void viewport_set_fsr_sharpness(RID p_viewport, float p_fsr_sharpness);
	static void viewport_set_texture_mipmap_bias(RID p_viewport, float p_texture_mipmap_bias);
	static void viewport_set_anisotropic_filtering_level(
		RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level);

	static void viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode);
	static RSE::ViewportUpdateMode viewport_get_update_mode(RID p_viewport);

	static void viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode);

	static RID viewport_get_render_target(RID p_viewport);
	static RID viewport_get_texture(RID p_viewport);

	static void viewport_set_environment_mode(RID p_viewport, RSE::ViewportEnvironmentMode p_mode);
	static void viewport_set_disable_3d(RID p_viewport, bool p_disable);
	static void viewport_set_disable_2d(RID p_viewport, bool p_disable);

	static void viewport_attach_camera(RID p_viewport, RID p_camera);
	static void viewport_set_scenario(RID p_viewport, RID p_scenario);
	static void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	static void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	static void viewport_set_canvas_transform(
		RID p_viewport, RID p_canvas, const Transform2D& p_offset);
	static void viewport_set_transparent_background(RID p_viewport, bool p_enabled);
	static void viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr);
	static bool viewport_is_using_hdr_2d(RID p_viewport);
	static void viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled);
	static void viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled);

	static void viewport_set_default_canvas_item_texture_filter(
		RID p_viewport, RSE::CanvasItemTextureFilter p_filter);
	static void viewport_set_default_canvas_item_texture_repeat(
		RID p_viewport, RSE::CanvasItemTextureRepeat p_repeat);

	static void viewport_set_global_canvas_transform(
		RID p_viewport, const Transform2D& p_transform);
	static void viewport_set_canvas_stacking(
		RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	static void viewport_set_sdf_oversize_and_scale(
		RID p_viewport, RSE::ViewportSDFOversize p_oversize, RSE::ViewportSDFScale p_scale);

	static void viewport_set_positional_shadow_atlas_size(
		RID p_viewport, int p_size, bool p_16_bits = true);
	static void viewport_set_positional_shadow_atlas_quadrant_subdivision(
		RID p_viewport, int p_quadrant, int p_subdiv);

	static void viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	static void viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa);

	static void viewport_set_screen_space_aa(RID p_viewport, RSE::ViewportScreenSpaceAA p_mode);

	static void viewport_set_use_taa(RID p_viewport, bool p_use_taa);
	static void viewport_set_use_debanding(RID p_viewport, bool p_use_debanding);
	static void viewport_set_force_motion_vectors(RID p_viewport, bool p_force_motion_vectors);

	static void viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels);

	static void viewport_set_use_occlusion_culling(RID p_viewport, bool p_use_occlusion_culling);
	static void viewport_set_occlusion_rays_per_thread(int p_rays_per_thread);

	static void viewport_set_occlusion_culling_build_quality(
		RSE::ViewportOcclusionCullingBuildQuality p_quality);

	static int viewport_get_render_info(
		RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info);

	static void viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw);

	static void viewport_set_measure_render_time(RID p_viewport, bool p_enable);
	static double viewport_get_measured_render_time_cpu(RID p_viewport);
	static double viewport_get_measured_render_time_gpu(RID p_viewport);

	static RID viewport_find_from_screen_attachment(
		DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID);

	static void viewport_set_vrs_mode(RID p_viewport, RSE::ViewportVRSMode p_mode);
	static void viewport_set_vrs_update_mode(RID p_viewport, RSE::ViewportVRSUpdateMode p_mode);
	static void viewport_set_vrs_texture(RID p_viewport, RID p_texture);

	/* SKY API */

	static RID sky_create();
	static void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	static void sky_set_mode(RID p_sky, RSE::SkyMode p_mode);
	static void sky_set_material(RID p_sky, RID p_material);
	static Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* COMPOSITOR EFFECTS API */

	static RID compositor_effect_create();
	static void compositor_effect_set_enabled(RID p_effect, bool p_enabled);
	static void compositor_effect_set_flag(
		RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set);

	/* COMPOSITOR API */

	static RID compositor_create();

	/* ENVIRONMENT API */

	static RID environment_create();

	static void environment_set_background(RID p_env, RSE::EnvironmentBG p_bg);
	static void environment_set_sky(RID p_env, RID p_sky);
	static void environment_set_sky_custom_fov(RID p_env, float p_scale);
	static void environment_set_sky_orientation(RID p_env, const Basis& p_orientation);
	static void environment_set_bg_color(RID p_env, const Color& p_color);
	static void environment_set_bg_energy(RID p_env, float p_multiplier, float p_exposure_value);
	static void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	static void environment_set_ambient_light(RID p_env, const Color& p_color,
		RSE::EnvironmentAmbientSource p_ambient = RSE::ENV_AMBIENT_SOURCE_BG, float p_energy = 1.0,
		float p_sky_contribution = 0.0,
		RSE::EnvironmentReflectionSource p_reflection_source = RSE::ENV_REFLECTION_SOURCE_BG);
	static void environment_set_camera_feed_id(RID p_env, int p_camera_feed_id);

	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
		float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);

	static void environment_glow_set_use_bicubic_upscale(bool p_enable);

	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
		float p_contrast, float p_saturation, bool p_use_1d_color_correction,
		RID p_color_correction);

	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_in,
		float p_fade_out, float p_depth_tolerance);

	static void environment_set_ssr_half_size(bool p_half_size);

	static void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality);

	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);

	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
		float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
		float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
		float p_probe_bias);

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);

	static void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);

	static void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode = RSE::EnvironmentFogMode::ENV_FOG_MODE_EXPONENTIAL);
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);

	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	static void environment_set_volumetric_fog_filter_active(bool p_enable);

	static Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	static void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);

	static void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	static void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale);

	/* CAMERA ATTRIBUTES API */

	static RID camera_attributes_create();

	static void camera_attributes_set_dof_blur_quality(
		RSE::DOFBlurQuality p_quality, bool p_use_jitter);

	static void camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape);

	static void camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
		float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
		float p_near_transition, float p_amount);
	static void camera_attributes_set_exposure(
		RID p_camera_attributes, float p_multiplier, float p_exposure_normalization);
	static void camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
		float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale);

	/* SCENARIO API */

	static RID scenario_create();

	static void scenario_set_environment(RID p_scenario, RID p_environment);
	static void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	static void scenario_set_camera_attributes(RID p_scenario, RID p_camera_attributes);
	static void scenario_set_compositor(RID p_scenario, RID p_compositor);

	/* INSTANCING API */

	static RID instance_create2(RID p_base, RID p_scenario);
	static RID instance_create();

	static void instance_set_base(RID p_instance, RID p_base);
	static void instance_set_scenario(RID p_instance, RID p_scenario);
	static void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	static void instance_set_pivot_data(
		RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	static void instance_set_transform(RID p_instance, const Transform3D& p_transform);
	static void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	static void instance_set_surface_override_material(
		RID p_instance, int p_surface, RID p_material);
	static void instance_set_visible(RID p_instance, bool p_visible);

	static void instance_teleport(RID p_instance);
	static void instance_set_custom_aabb(RID p_instance, AABB p_aabb);
	static void instance_attach_skeleton(RID p_instance, RID p_skeleton);
	static void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);
	static void instance_set_visibility_parent(RID p_instance, RID p_parent_instance);
	static void instance_set_ignore_culling(RID p_instance, bool p_enabled);

	static PackedInt64Array _instances_cull_aabb_bind(const AABB& p_aabb, RID p_scenario = RID());
	static PackedInt64Array _instances_cull_ray_bind(
		const Vector3& p_from, const Vector3& p_to, RID p_scenario = RID());

	static void instance_geometry_set_flag(
		RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled);
	static void instance_geometry_set_cast_shadows_setting(
		RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting);
	static void instance_geometry_set_material_override(RID p_instance, RID p_material);
	static void instance_geometry_set_material_overlay(RID p_instance, RID p_material);
	static void instance_geometry_set_visibility_range(RID p_instance, float p_min, float p_max,
		float p_min_margin, float p_max_margin, RSE::VisibilityRangeFadeMode p_fade_mode);
	static void instance_geometry_set_lightmap(
		RID p_instance, RID p_lightmap, const Rect2& p_lightmap_uv_scale, int p_lightmap_slice);
	static void instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias);
	static void instance_geometry_set_transparency(RID p_instance, float p_transparency);

	/* CANVAS API (2D) */

	static RID canvas_create();
	static void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2& p_mirroring);
	static void canvas_set_item_repeat(RID p_item, const Point2& p_repeat_size, int p_repeat_times);
	static void canvas_set_modulate(RID p_canvas, const Color& p_color);
	static void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale);
	static void canvas_set_disable_scale(bool p_disable);

	/* CANVAS TEXTURE API */

	static RID canvas_texture_create();
	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);

	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat);

	/* CANVAS ITEM API */

	static RID canvas_item_create();
	static void canvas_item_set_parent(RID p_item, RID p_parent);

	static void canvas_item_set_default_texture_filter(
		RID p_item, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_item_set_default_texture_repeat(
		RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	static void canvas_item_set_visible(RID p_item, bool p_visible);
	static void canvas_item_set_light_mask(RID p_item, int p_mask);

	static void canvas_item_set_update_when_visible(RID p_item, bool p_update);

	static void canvas_item_set_transform(RID p_item, const Transform2D& p_transform);
	static void canvas_item_set_clip(RID p_item, bool p_clip);
	static void canvas_item_set_distance_field_mode(RID p_item, bool p_enable);
	static void canvas_item_set_custom_rect(
		RID p_item, bool p_custom_rect, const Rect2& p_rect = Rect2());
	static void canvas_item_set_modulate(RID p_item, const Color& p_color);
	static void canvas_item_set_self_modulate(RID p_item, const Color& p_color);
	static void canvas_item_set_visibility_layer(RID p_item, uint32_t p_visibility_layer);

	static void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable);
	static void canvas_item_set_use_identity_transform(RID p_item, bool p_enabled);

	static void canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
		const Color& p_color, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_rect(
		RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major,
		float p_minor, const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_circle(RID p_item, const Point2& p_pos, float p_radius,
		const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
		bool p_tile = false, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false);
	static void canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = false);
	static void canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect,
		RID p_texture, const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1),
		int p_outline_size = 0, float p_px_range = 1.0, float p_scale = 1.0);
	static void canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect,
		RID p_texture, const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1));
	static void canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect, const Rect2& p_source,
		RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
		RSE::NinePatchAxisMode p_x_axis_mode = RSE::NINE_PATCH_STRETCH,
		RSE::NinePatchAxisMode p_y_axis_mode = RSE::NINE_PATCH_STRETCH, bool p_draw_center = true,
		const Color& p_modulate = Color(1, 1, 1));
	static void canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture);
	static void canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		RID p_texture = RID());
	static void canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(), const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1);
	static void canvas_item_add_mesh(RID p_item, const RID& p_mesh,
		const Transform2D& p_transform = Transform2D(), const Color& p_modulate = Color(1, 1, 1),
		RID p_texture = RID());
	static void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID());
	static void canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture);
	static void canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform);
	static void canvas_item_add_clip_ignore(RID p_item, bool p_ignore);
	static void canvas_item_add_animation_slice(RID p_item, double p_animation_length,
		double p_slice_begin, double p_slice_end, double p_offset);

	static void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable);
	static void canvas_item_set_z_index(RID p_item, int p_z);
	static void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable);
	static void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2& p_rect);

	static void canvas_item_attach_skeleton(RID p_item, RID p_skeleton);

	static void canvas_item_clear(RID p_item);
	static void canvas_item_set_draw_index(RID p_item, int p_index);

	static void canvas_item_set_material(RID p_item, RID p_material);
	static void canvas_item_set_use_parent_material(RID p_item, bool p_enable);

	static void canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
		float p_clear_margin = 5.0, bool p_fit_empty = false, float p_fit_margin = 0.0,
		bool p_blur_mipmaps = false);

	static void canvas_item_set_debug_redraw(bool p_enabled);
	static bool canvas_item_get_debug_redraw();

	static void canvas_item_set_interpolated(RID p_item, bool p_interpolated);
	static void canvas_item_reset_physics_interpolation(RID p_item);
	static void canvas_item_transform_physics_interpolation(
		RID p_item, const Transform2D& p_transform);

	/* CANVAS LIGHT */

	static RID canvas_light_create();
	static void canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode);
	static void canvas_light_attach_to_canvas(RID p_light, RID p_canvas);
	static void canvas_light_set_enabled(RID p_light, bool p_enabled);
	static void canvas_light_set_transform(RID p_light, const Transform2D& p_transform);
	static void canvas_light_set_color(RID p_light, const Color& p_color);
	static void canvas_light_set_height(RID p_light, float p_height);
	static void canvas_light_set_energy(RID p_light, float p_energy);
	static void canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z);
	static void canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer);
	static void canvas_light_set_item_cull_mask(RID p_light, int p_mask);
	static void canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask);

	static void canvas_light_set_directional_distance(RID p_light, float p_distance);

	static void canvas_light_set_texture_scale(RID p_light, float p_scale);
	static void canvas_light_set_texture(RID p_light, RID p_texture);
	static void canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset);

	static void canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode);

	static void canvas_light_set_shadow_enabled(RID p_light, bool p_enabled);
	static void canvas_light_set_shadow_filter(RID p_light, RSE::CanvasLightShadowFilter p_filter);
	static void canvas_light_set_shadow_color(RID p_light, const Color& p_color);
	static void canvas_light_set_shadow_smooth(RID p_light, float p_smooth);

	static void canvas_light_set_interpolated(RID p_light, bool p_interpolated);
	static void canvas_light_reset_physics_interpolation(RID p_light);
	static void canvas_light_transform_physics_interpolation(
		RID p_light, const Transform2D& p_transform);

	/* CANVAS LIGHT OCCLUDER API */

	static RID canvas_light_occluder_create();
	static void canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas);
	static void canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled);
	static void canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon);
	static void canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable);
	static void canvas_light_occluder_set_transform(RID p_occluder, const Transform2D& p_xform);
	static void canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask);

	static void canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated);
	static void canvas_light_occluder_reset_physics_interpolation(RID p_occluder);
	static void canvas_light_occluder_transform_physics_interpolation(
		RID p_occluder, const Transform2D& p_transform);

	/* CANVAS OCCLUDER POLYGON API */

	static RID canvas_occluder_polygon_create();
	static void canvas_occluder_polygon_set_shape(
		RID p_occluder_polygon, const Vector<Vector2>& p_shape, bool p_closed);

	static void canvas_occluder_polygon_set_cull_mode(
		RID p_occluder_polygon, RSE::CanvasOccluderPolygonCullMode p_mode);

	static void canvas_set_shadow_texture_size(int p_size);

	static Rect2 debug_canvas_item_get_rect(RID p_item);
	static Rect2 _debug_canvas_item_get_rect(RID p_item);

	/* GLOBAL SHADER PARAMETERS API */

	static void global_shader_parameter_remove(const String& p_name);
	static Vector<String> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
		const String& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures);
	static void global_shader_parameters_clear();

	static int global_shader_uniform_type_get_shader_datatype(
		RSE::GlobalShaderParameterType p_type);

	/* FREE */

	static void free_rid(RID p_rid);

	/* INTERPOLATION */

	static void set_physics_interpolation_enabled(bool p_enabled);

	/* EVENT QUEUING */

	static void draw(bool p_swap_buffers = true, double p_frame_step = 0.0);
	static void sync();
	static bool has_changed();
	static void tick();
	static void pre_draw(bool p_will_draw);

	/* STATUS INFORMATION */

	static uint64_t get_rendering_info(RSE::RenderingInfo p_info);
	static String get_video_adapter_name();
	static String get_video_adapter_vendor();
	static RenderingDeviceEnums::DeviceType get_video_adapter_type();
	static String get_video_adapter_api_version();

	static void set_frame_profiling_enabled(bool p_enable);
	static Vector<RenderingServerTypes::FrameProfileArea> get_frame_profile();
	static uint64_t get_frame_profile_frame();

	static double get_frame_setup_time_cpu();

	static void gi_set_use_half_resolution(bool p_enable);

	/* TESTING */

	static RID get_test_cube();
	static RID get_test_texture();
	static RID get_white_texture();

	static void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	static RID make_sphere_mesh(int p_lats, int p_lons, real_t p_radius);

	static void mesh_add_surface_from_mesh_data(
		RID p_mesh, const Geometry3D::MeshData& p_mesh_data);
	static void mesh_add_surface_from_planes(RID p_mesh, const Vector<Plane>& p_planes);

	/* BACKGROUND */

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true);

	static Color get_default_clear_color();
	static void set_default_clear_color(const Color& p_color);

	/* MISC */

	static bool has_os_feature(const String& p_feature);

	static void set_debug_generate_wireframes(bool p_generate);

	static void call_set_vsync_mode(
		DisplayServerEnums::VSyncMode p_mode, DisplayServerEnums::WindowID p_window);

	static bool is_low_end();

	static void set_print_gpu_profile(bool p_enable);

	static Size2i get_maximum_viewport_size();

	static bool is_render_loop_enabled();
	static void set_render_loop_enabled(bool p_enabled);

	static bool is_on_render_thread();

	static String get_current_rendering_driver_name();
	static String get_current_rendering_method();

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);

	typedef void (*SurfaceUpgradeCallback)();
	static void set_surface_upgrade_callback(SurfaceUpgradeCallback p_callback);
	static void set_warn_on_surface_upgrade(bool p_warn);
#endif

	static void redraw_request()
	{
		if (data) {
			data->backend.changes++;
		}
	}

	/* LIFECYCLE DISALLOWANCE */

	Renderer() = delete;
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	~Renderer() = delete;
};

// --- Folded from servers/rendering/renderer_rd/shader_rd.h ---
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/types.h"
#include "servers/rendering/rendering_device.h"

class StringBuilder;

class ShaderRD
{
public:
	struct VariantDefine
	{
		int group = 0;
		CharString text;
		bool default_enabled = true;

		VariantDefine() {}

		VariantDefine(int p_group, const String& p_text, bool p_default_enabled)
		{
			group = p_group;
			default_enabled = p_default_enabled;
			text = p_text.utf8();
		}
	};

	typedef Pair<ShaderRD*, RID> ShaderVersionPair;
	typedef HashSet<ShaderVersionPair> ShaderVersionPairSet;

	struct StageData
	{
		const char* vertex_code = nullptr;
		const char* fragment_code = nullptr;
		const char* compute_code = nullptr;
		const char* raygen_code = nullptr;
		const char* any_hit_code = nullptr;
		const char* closest_hit_code = nullptr;
		const char* miss_code = nullptr;
		const char* intersection_code = nullptr;
		const char* name = nullptr;
		bool is_raytracing = false;
	};

private:
	// versions
	CharString general_defines;
	Vector<VariantDefine> variant_defines;
	Vector<bool> variants_enabled;
	Vector<uint32_t> variant_to_group;
	HashMap<int, LocalVector<int>> group_to_variant_map;
	Vector<bool> group_enabled;

	Vector<RD::PipelineImmutableSampler> immutable_samplers;
	Vector<uint64_t> dynamic_buffers;

	struct Version
	{
		Mutex* mutex = nullptr;
		CharString uniforms;
		CharString vertex_globals;
		CharString compute_globals;
		CharString fragment_globals;
		CharString raygen_globals;
		CharString any_hit_globals;
		CharString closest_hit_globals;
		CharString miss_globals;
		CharString intersection_globals;
		HashMap<String, CharString> code_sections;
		Vector<CharString> custom_defines;
		Vector<bool> group_loaded_from_cache;

		Vector<Vector<uint8_t>> variant_data;
		Vector<RID> variants;

		bool valid;
		bool dirty;
		bool initialize_needed;
		bool embedded;
	};

	struct CompileData
	{
		Version* version;
		int group = 0;
	};

	// Vector will have the size of SHADER_STAGE_MAX and unused stages will have empty strings.
	void _compile_variant(uint32_t p_variant, CompileData p_data);

	void _initialize_version(Version* p_version);
	void _clear_version(Version* p_version, bool p_skip_compilation = false);
	void _compile_version_start(Version* p_version, int p_group);
	void _compile_version_end(Version* p_version, int p_group);
	void _compile_ensure_finished(Version* p_version);
	void _allocate_placeholders(Version* p_version, int p_group);

	RID_Owner<Version, true> version_owner;
	Mutex versions_mutex;
	HashMap<RID, Mutex*> version_mutexes;

	struct StageTemplate
	{
		struct Chunk
		{
			enum Type
			{
				TYPE_VERSION_DEFINES,
				TYPE_MATERIAL_UNIFORMS,
				TYPE_VERTEX_GLOBALS,
				TYPE_FRAGMENT_GLOBALS,
				TYPE_COMPUTE_GLOBALS,
				TYPE_RAYGEN_GLOBALS,
				TYPE_ANY_HIT_GLOBALS,
				TYPE_CLOSEST_HIT_GLOBALS,
				TYPE_MISS_GLOBALS,
				TYPE_INTERSECTION_GLOBALS,
				TYPE_CODE,
				TYPE_TEXT
			};

			Type type;
			String code;
			CharString text;
		};

		LocalVector<Chunk> chunks;
	};

	RDC::PipelineType pipeline_type = RDC::PIPELINE_TYPE_RASTERIZATION;

	String name;

	CharString base_compute_defines;

	String base_sha256;
	LocalVector<String> group_sha256;

	static inline ShaderVersionPairSet shader_versions_embedded_set;
	static inline Mutex shader_versions_embedded_set_mutex;

	static String shader_cache_user_dir;
	static String shader_cache_res_dir;
	static bool shader_cache_cleanup_on_start;
	static bool shader_cache_save_compressed;
	static bool shader_cache_save_compressed_zstd;
	static bool shader_cache_save_debug;
	bool shader_cache_user_dir_valid = false;
	bool shader_cache_res_dir_valid = false;

	enum StageType
	{
		STAGE_TYPE_VERTEX,
		STAGE_TYPE_FRAGMENT,
		STAGE_TYPE_COMPUTE,
		STAGE_TYPE_RAYGEN,
		STAGE_TYPE_ANY_HIT,
		STAGE_TYPE_CLOSEST_HIT,
		STAGE_TYPE_MISS,
		STAGE_TYPE_INTERSECTION,
		STAGE_TYPE_MAX,
	};

	StageTemplate stage_templates[STAGE_TYPE_MAX];

	void _build_variant_code(StringBuilder& p_builder, uint32_t p_variant, const Version* p_version,
		const StageTemplate& p_template);
	Vector<String> _build_variant_stage_sources(uint32_t p_variant, CompileData p_data);

	void _add_stage(const char* p_code, StageType p_stage_type);

	String _version_get_sha1(Version* p_version) const;
	String _get_cache_file_relative_path(Version* p_version, int p_group, const String& p_api_name);
	String _get_cache_file_path(
		Version* p_version, int p_group, const String& p_api_name, bool p_user_dir);
	void _load_variant_from_cache(uint32_t p_variant, CompileData p_data);
	bool _load_from_cache(Version* p_version, int p_group);
	void _save_to_cache(Version* p_version, int p_group);
	void _initialize_cache();
	void _version_set(Version* p_version, const HashMap<String, String>& p_code,
		const Vector<String>& p_custom_defines);

public:
	ShaderRD();
	explicit ShaderRD(const StageData& p_data);
	void setup(const StageData& p_data);
	void setup(const char* p_vertex_code, const char* p_fragment_code, const char* p_compute_code,
		const char* p_name);
	void setup_raytracing(const char* p_raygen_code, const char* p_any_hit_code,
		const char* p_closest_hit_code, const char* p_miss_code, const char* p_intersection_code,
		const char* p_name);

	_FORCE_INLINE_ RID version_get_shader(RID p_version, int p_variant)
	{
		ERR_FAIL_INDEX_V(p_variant, variant_defines.size(), RID());
		ERR_FAIL_COND_V(!variants_enabled[p_variant], RID());

		Version* version = version_owner.get_or_null(p_version);
		ERR_FAIL_NULL_V(version, RID());

		MutexLock lock(*version->mutex);

		if (version->dirty) {
			_initialize_version(version);
			for (int i = 0; i < group_enabled.size(); i++) {
				if (!group_enabled[i]) {
					_allocate_placeholders(version, i);
					continue;
				}
				_compile_version_start(version, i);
			}
			// Force the compile to finish for every group that was started
			for (int i = 0; i < group_enabled.size(); i++) {
				if (group_enabled[i]) {
					_compile_version_end(version, i);
				}
			}
		}

		if (!version->valid) {
			return RID();
		}

		return version->variants[p_variant];
	}

	RID version_create(bool p_embedded = true);

	void version_set_code(RID p_version, const HashMap<String, String>& p_code,
		const String& p_uniforms, const String& p_vertex_globals, const String& p_fragment_globals,
		const Vector<String>& p_custom_defines);

	void version_set_compute_code(RID p_version, const HashMap<String, String>& p_code,
		const String& p_uniforms, const String& p_compute_globals,
		const Vector<String>& p_custom_defines);
	void version_set_raytracing_code(RID p_version, const HashMap<String, String>& p_code,
		const String& p_uniforms, const String& p_raygen_globals, const String& p_any_hit_globals,
		const String& p_closest_hit_globals, const String& p_miss_globals,
		const String& p_intersection_globals, const Vector<String>& p_custom_defines);

	bool version_is_valid(RID p_version);

	bool version_free(RID p_version);

	// Enable/disable variants for things that you know won't be used at engine initialization time
	// .
	void set_variant_enabled(int p_variant, bool p_enabled);
	bool is_variant_enabled(int p_variant) const;
	int64_t get_variant_count() const;
	int get_variant_to_group(int p_variant) const;

	// Enable/disable groups for things that might be enabled at run time.
	void enable_group(int p_group);
	bool is_group_enabled(int p_group) const;
	int64_t get_group_count() const;
	const LocalVector<int>& get_group_to_variants(int p_group) const;

	const String& get_name() const;

	const Vector<uint64_t>& get_dynamic_buffers() const;

	static void shaders_embedded_set_lock();
	static const ShaderVersionPairSet& shaders_embedded_set_get();
	static void shaders_embedded_set_unlock();

	static void set_shader_cache_user_dir(const String& p_dir);
	static const String& get_shader_cache_user_dir();
	static void set_shader_cache_res_dir(const String& p_dir);
	static const String& get_shader_cache_res_dir();
	static void set_shader_cache_save_compressed(bool p_enable);
	static void set_shader_cache_save_compressed_zstd(bool p_enable);
	static void set_shader_cache_save_debug(bool p_enable);

	static Vector<RDC::ShaderStageSPIRVData> compile_stages(
		const Vector<String>& p_stage_sources, const Vector<uint64_t>& p_dynamic_buffers);
	static PackedByteArray save_shader_cache_bytes(
		const LocalVector<int>& p_variants, const Vector<Vector<uint8_t>>& p_variant_data);

	Vector<String> version_build_variant_stage_sources(RID p_version, int p_variant);
	RenderingServerTypes::ShaderNativeSourceCode version_get_native_source_code(RID p_version);
	String version_get_cache_file_relative_path(
		RID p_version, int p_group, const String& p_api_name);

	struct DynamicBuffer
	{
		static uint64_t encode(uint32_t p_set_id, uint32_t p_binding)
		{
			return uint64_t(p_set_id) << 32ul | uint64_t(p_binding);
		}
	};

	// Dynamic Buffers specifies Which buffers will be persistent/dynamic when used.
	// See DynamicBuffer::encode. We need this argument because SPIR-V does not distinguish between
	// a uniform buffer and a dynamic uniform buffer. At shader level they're the same thing, but
	// the PSO is created slightly differently and they're bound differently. On D3D12 the Root
	// Layout is also different.
	void initialize(const Vector<String>& p_variant_defines, const String& p_general_defines = "",
		const Vector<RD::PipelineImmutableSampler>& p_immutable_samplers =
			Vector<RD::PipelineImmutableSampler>(),
		const Vector<uint64_t>& p_dynamic_buffers = Vector<uint64_t>());
	void initialize(const Vector<VariantDefine>& p_variant_defines,
		const String& p_general_defines = "",
		const Vector<RD::PipelineImmutableSampler>& p_immutable_samplers =
			Vector<RD::PipelineImmutableSampler>(),
		const Vector<uint64_t>& p_dynamic_buffers = Vector<uint64_t>());

	~ShaderRD();
};
// --- End servers/rendering/renderer_rd/shader_rd.h ---

// --- Folded from servers/rendering/renderer_viewport.h ---
#include "core/types.h"
#include "servers/display/display_server.h"

#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"


#pragma once

#include "core/types.h"
#include "servers/display/display_server.h"

#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"


class RendererViewport final
{
public:
	struct CanvasBase
	{
	};

	struct Viewport
	{
		RID self;
		RID parent;

		bool use_xr = false;

		Size2i internal_size;
		Size2i size;
		uint32_t view_count = 1;
		RID camera;
		RID scenario;

		RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
		float scaling_3d_scale = 1.0f;
		float fsr_sharpness = 0.2f;
		float texture_mipmap_bias = 0.0f;
		RSE::ViewportAnisotropicFiltering anisotropic_filtering_level = RSE::VIEWPORT_ANISOTROPY_4X;
		bool fsr_enabled = false;
		uint32_t jitter_phase_count = 0;
		RSE::ViewportUpdateMode update_mode = RSE::VIEWPORT_UPDATE_WHEN_VISIBLE;
		RID render_target;
		RID render_target_texture;
		Ref<RenderSceneBuffers> render_buffers;

		RSE::ViewportMSAA msaa_2d = RSE::VIEWPORT_MSAA_DISABLED;
		RSE::ViewportMSAA msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
		RSE::ViewportScreenSpaceAA screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
		bool use_taa = false;
		bool use_debanding = false;
		bool force_motion_vectors = false;

		RendererSceneRender::CameraData prev_camera_data;
		uint64_t prev_camera_data_frame = 0;

		bool use_occlusion_culling = false;
		bool occlusion_buffer_dirty = true;

		DisplayServerEnums::WindowID viewport_to_screen = DisplayServerEnums::INVALID_WINDOW_ID;
		Rect2 viewport_to_screen_rect;
		bool viewport_render_direct_to_screen = false;

		bool disable_2d = false;
		RSE::ViewportEnvironmentMode disable_environment = RSE::VIEWPORT_ENVIRONMENT_INHERIT;
		bool disable_3d = false;
		bool measure_render_time = false;

		bool snap_2d_transforms_to_pixel = false;
		bool snap_2d_vertices_to_pixel = false;

		uint64_t time_cpu_begin = 0;
		uint64_t time_cpu_end = 0;

		uint64_t time_gpu_begin = 0;
		uint64_t time_gpu_end = 0;

		RID shadow_atlas;
		int shadow_atlas_size = 0;
		bool shadow_atlas_16_bits = true;

		bool sdf_active = false;

		float mesh_lod_threshold = 1.0f;

		uint64_t last_pass = 0;

		RSE::ViewportDebugDraw debug_draw = RSE::VIEWPORT_DEBUG_DRAW_DISABLED;
		RSE::ViewportClearMode clear_mode = RSE::VIEWPORT_CLEAR_ALWAYS;

		RSE::CanvasItemTextureFilter texture_filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR;
		RSE::CanvasItemTextureRepeat texture_repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED;

		bool transparent_bg = false;
		bool use_hdr_2d = false;
		float window_output_max_value = 1.0f;

		uint32_t canvas_cull_mask = 0xffffffff;

		struct CanvasKey
		{
			int64_t stacking = 0;
			RID canvas;

			bool operator<(const CanvasKey& p_canvas) const
			{
				if (stacking == p_canvas.stacking) {
					return canvas < p_canvas.canvas;
				}
				return stacking < p_canvas.stacking;
			}

			CanvasKey() = default;

			CanvasKey(const RID& p_canvas, int p_layer, int p_sublayer)
			{
				canvas = p_canvas;
				int64_t sign = p_layer < 0 ? -1 : 1;
				stacking = sign * (((int64_t)Math::abs(p_layer)) << 32) + p_sublayer;
			}

			int get_layer() const { return stacking >> 32; }
		};


		struct CanvasData
		{
			CanvasBase* canvas = nullptr;
			Transform2D transform;
			int layer = 0;
			int sublayer = 0;
		};

		Transform2D global_transform;
		HashMap<RID, CanvasData> canvas_map;
		RenderingServerTypes::RenderInfo render_info;

		Viewport() = default;
	};

	struct ViewportSort
	{
		_FORCE_INLINE_ bool operator()(const RendererViewport::Viewport* p_left,
			const RendererViewport::Viewport* p_right) const
		{
			bool left_is_parent_of_right = p_right->parent == p_left->self;
			if (left_is_parent_of_right) {
				return true;
			}
			bool right_is_parent_of_left = p_left->parent == p_right->self;
			if (right_is_parent_of_left) {
				return false;
			}
			return p_left->parent < p_right->parent;
		}
	};

private:
	HashMap<String, RID> timestamp_vp_map;
	uint64_t draw_viewports_pass = 0;

	mutable RID_Owner<Viewport, true> viewport_owner;

	Vector<Viewport*> active_viewports;
	Vector<Viewport*> sorted_active_viewports;
	bool sorted_active_viewports_dirty = false;

	int total_objects_drawn = 0;
	int total_vertices_drawn = 0;
	int total_draw_calls_used = 0;
	int num_viewports_with_motion_vectors = 0;
	int occlusion_rays_per_thread = 512;

	Vector<Viewport*> _sort_active_viewports();
	void _viewport_set_size(Viewport* p_viewport, int p_width, int p_height, uint32_t p_view_count);
	bool _viewport_requires_motion_vectors(Viewport* p_viewport);
	void _viewport_set_force_motion_vectors(Viewport* p_viewport, bool p_force_motion_vectors);
	void _configure_3d_render_buffers(Viewport* p_viewport);
	void _draw_3d(Viewport* p_viewport);
	void _draw_viewport(Viewport* p_viewport);
	DisplayServerEnums::WindowID _get_containing_window(Viewport* p_viewport);
	void _resize_occlusion_culling_buffer(const Size2i& p_size);

public:
	RendererViewport() = default;
	~RendererViewport() = default;

	// Prevent accidental copying; this coordinator owns unique resource tables
	RendererViewport(const RendererViewport&) = delete;
	RendererViewport& operator=(const RendererViewport&) = delete;

	RendererViewport(RendererViewport&&) noexcept = default;
	RendererViewport& operator=(RendererViewport&&) noexcept = default;

	RID viewport_allocate();
	void viewport_initialize(RID p_rid);

#ifndef XR_DISABLED
	void viewport_set_use_xr(RID p_viewport, bool p_use_xr);
#endif

	void viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count = 1);
	void viewport_attach_to_screen(RID p_viewport, const Rect2& p_rect = Rect2(),
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);

	void viewport_set_active(RID p_viewport, bool p_active);
	void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);

	void viewport_set_scaling_3d_mode(RID p_viewport, RSE::ViewportScaling3DMode p_mode);
	void viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale);
	void viewport_set_fsr_sharpness(RID p_viewport, float p_sharpness);
	void viewport_set_texture_mipmap_bias(RID p_viewport, float p_mipmap_bias);
	void viewport_set_anisotropic_filtering_level(
		RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level);

	void viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode);
	RSE::ViewportUpdateMode viewport_get_update_mode(RID p_viewport) const;
	void viewport_set_vflip(RID p_viewport, bool p_enable);

	void viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode);

	RID viewport_get_render_target(RID p_viewport) const;
	RID viewport_get_texture(RID p_viewport) const;
	RID viewport_get_occluder_debug_texture(RID p_viewport) const;

	void viewport_set_prev_camera_data(
		RID p_viewport, const RendererSceneRender::CameraData* p_camera_data);
	const RendererSceneRender::CameraData* viewport_get_prev_camera_data(RID p_viewport);

	void viewport_set_disable_2d(RID p_viewport, bool p_disable);
	void viewport_set_environment_mode(RID p_viewport, RSE::ViewportEnvironmentMode p_mode);
	void viewport_set_disable_3d(RID p_viewport, bool p_disable);

	bool viewport_is_environment_disabled(Viewport* viewport);

	void viewport_attach_camera(RID p_viewport, RID p_camera);
	void viewport_set_scenario(RID p_viewport, RID p_scenario);
	void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	void viewport_set_canvas_transform(RID p_viewport, RID p_canvas, const Transform2D& p_offset);
	void viewport_set_transparent_background(RID p_viewport, bool p_enabled);
	void viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr_2d);

	bool viewport_is_using_hdr_2d(RID p_viewport) const;

	void viewport_set_global_canvas_transform(RID p_viewport, const Transform2D& p_transform);
	void viewport_set_canvas_stacking(RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	void viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask);

	void viewport_set_positional_shadow_atlas_size(
		RID p_viewport, int p_size, bool p_16_bits = true);
	void viewport_set_positional_shadow_atlas_quadrant_subdivision(
		RID p_viewport, int p_quadrant, int p_subdiv);

	void viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	void viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	void viewport_set_screen_space_aa(RID p_viewport, RSE::ViewportScreenSpaceAA p_mode);
	void viewport_set_use_taa(RID p_viewport, bool p_use_taa);
	void viewport_set_use_debanding(RID p_viewport, bool p_use_debanding);
	void viewport_set_force_motion_vectors(RID p_viewport, bool p_force_motion_vectors);
	void viewport_set_use_occlusion_culling(RID p_viewport, bool p_use_occlusion_culling);
	void viewport_set_occlusion_rays_per_thread(int p_rays_per_thread);
	void viewport_set_occlusion_culling_build_quality(
		RSE::ViewportOcclusionCullingBuildQuality p_quality);
	void viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels);

	int viewport_get_render_info(
		RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info);
	void viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw);

	void viewport_set_measure_render_time(RID p_viewport, bool p_enable);
	float viewport_get_measured_render_time_cpu(RID p_viewport) const;
	float viewport_get_measured_render_time_gpu(RID p_viewport) const;

	void viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled);

	void viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled);

	void viewport_set_default_canvas_item_texture_filter(
		RID p_viewport, RSE::CanvasItemTextureFilter p_filter);
	void viewport_set_default_canvas_item_texture_repeat(
		RID p_viewport, RSE::CanvasItemTextureRepeat p_repeat);

	void viewport_set_sdf_oversize_and_scale(
		RID p_viewport, RSE::ViewportSDFOversize p_over_size, RSE::ViewportSDFScale p_scale);

	RID viewport_find_from_screen_attachment(
		DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const;

	void viewport_set_vrs_mode(RID p_viewport, RSE::ViewportVRSMode p_mode);
	void viewport_set_vrs_update_mode(RID p_viewport, RSE::ViewportVRSUpdateMode p_mode);
	void viewport_set_vrs_texture(RID p_viewport, RID p_texture);

	void handle_timestamp(String p_timestamp, uint64_t p_cpu_time, uint64_t p_gpu_time);

	void draw_viewports(bool p_swap_buffers);

	bool free(RID p_rid);

	int get_total_objects_drawn() const;
	int get_total_primitives_drawn() const;
	int get_total_draw_calls_used() const;
	int get_num_viewports_with_motion_vectors() const;

	void call_set_vsync_mode(
		DisplayServerEnums::VSyncMode p_mode, DisplayServerEnums::WindowID p_window);
};

// --- Folded from servers/rendering/renderer_canvas_cull.h ---
#include "core/types.h"
// --- Inlined/Folded from servers/rendering/renderer_scene_cull.h ---
// --- Folded from core/math/dynamic_bvh.h ---
#include "core/types.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/types.h"
#include "core/typedefs.h"

// Based on bullet Dbvh

/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2013 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

///DynamicBVH implementation by Nathanael Presson
// The DynamicBVH class implements a fast dynamic bounding volume tree based on axis aligned bounding boxes (aabb tree).

class DynamicBVH {
	struct Node;

public:
	struct ID {
		Node *node = nullptr;

	public:
		_FORCE_INLINE_ bool is_valid() const { return node != nullptr; }
	};

private:
	struct Volume {
		Vector3 min, max;

		_FORCE_INLINE_ Vector3 get_center() const { return ((min + max) / 2); }
		_FORCE_INLINE_ Vector3 get_length() const { return (max - min); }

		_FORCE_INLINE_ bool contains(const Volume &p_a) const {
			return ((min.x <= p_a.min.x) &&
					(min.y <= p_a.min.y) &&
					(min.z <= p_a.min.z) &&
					(max.x >= p_a.max.x) &&
					(max.y >= p_a.max.y) &&
					(max.z >= p_a.max.z));
		}

		_FORCE_INLINE_ Volume merge(const Volume &p_b) const {
			Volume r;
			for (int i = 0; i < 3; ++i) {
				if (min[i] < p_b.min[i]) {
					r.min[i] = min[i];
				} else {
					r.min[i] = p_b.min[i];
				}
				if (max[i] > p_b.max[i]) {
					r.max[i] = max[i];
				} else {
					r.max[i] = p_b.max[i];
				}
			}
			return r;
		}

		_FORCE_INLINE_ real_t get_size() const {
			const Vector3 edges = get_length();
			return (edges.x * edges.y * edges.z +
					edges.x + edges.y + edges.z);
		}

		_FORCE_INLINE_ bool is_not_equal_to(const Volume &p_b) const {
			return ((min.x != p_b.min.x) ||
					(min.y != p_b.min.y) ||
					(min.z != p_b.min.z) ||
					(max.x != p_b.max.x) ||
					(max.y != p_b.max.y) ||
					(max.z != p_b.max.z));
		}

		_FORCE_INLINE_ real_t get_proximity_to(const Volume &p_b) const {
			const Vector3 d = (min + max) - (p_b.min + p_b.max);
			return (Math::abs(d.x) + Math::abs(d.y) + Math::abs(d.z));
		}

		_FORCE_INLINE_ int select_by_proximity(const Volume &p_a, const Volume &p_b) const {
			return (get_proximity_to(p_a) < get_proximity_to(p_b) ? 0 : 1);
		}

		//
		_FORCE_INLINE_ bool intersects(const Volume &p_b) const {
			return ((min.x <= p_b.max.x) &&
					(max.x >= p_b.min.x) &&
					(min.y <= p_b.max.y) &&
					(max.y >= p_b.min.y) &&
					(min.z <= p_b.max.z) &&
					(max.z >= p_b.min.z));
		}

		_FORCE_INLINE_ bool intersects_convex(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count) const {
			Vector3 half_extents = (max - min) * 0.5;
			Vector3 ofs = min + half_extents;

			for (int i = 0; i < p_plane_count; i++) {
				const Plane &p = p_planes[i];
				Vector3 point(
						(p.normal.x > 0) ? -half_extents.x : half_extents.x,
						(p.normal.y > 0) ? -half_extents.y : half_extents.y,
						(p.normal.z > 0) ? -half_extents.z : half_extents.z);
				point += ofs;
				if (p.is_point_over(point)) {
					return false;
				}
			}

			// Make sure all points in the shape aren't fully separated from the AABB on
			// each axis.
			int bad_point_counts_positive[3] = { 0 };
			int bad_point_counts_negative[3] = { 0 };

			for (int k = 0; k < 3; k++) {
				for (int i = 0; i < p_point_count; i++) {
					if (p_points[i][k] > ofs[k] + half_extents[k]) {
						bad_point_counts_positive[k]++;
					}
					if (p_points[i][k] < ofs[k] - half_extents[k]) {
						bad_point_counts_negative[k]++;
					}
				}

				if (bad_point_counts_negative[k] == p_point_count) {
					return false;
				}
				if (bad_point_counts_positive[k] == p_point_count) {
					return false;
				}
			}

			return true;
		}
	};

	struct Node {
		Volume volume;
		Node *parent = nullptr;
		union {
			Node *children[2];
			void *data;
		};

		_FORCE_INLINE_ bool is_leaf() const { return children[1] == nullptr; }
		_FORCE_INLINE_ bool is_internal() const { return (!is_leaf()); }

		_FORCE_INLINE_ int get_index_in_parent() const {
			ERR_FAIL_NULL_V(parent, 0);
			return (parent->children[1] == this) ? 1 : 0;
		}
		void get_max_depth(int p_depth, int &r_maxdepth) {
			if (is_internal()) {
				children[0]->get_max_depth(p_depth + 1, r_maxdepth);
				children[1]->get_max_depth(p_depth + 1, r_maxdepth);
			} else {
				r_maxdepth = MAX(r_maxdepth, p_depth);
			}
		}

		//
		int count_leaves() const {
			if (is_internal()) {
				return children[0]->count_leaves() + children[1]->count_leaves();
			} else {
				return (1);
			}
		}

		bool is_left_of_axis(const Vector3 &p_org, const Vector3 &p_axis) const {
			return p_axis.dot(volume.get_center() - p_org) <= 0;
		}

		Node() {
			children[0] = nullptr;
			children[1] = nullptr;
		}
	};

	PagedAllocator<Node> node_allocator;
	// Fields
	Node *bvh_root = nullptr;
	int lkhd = -1;
	int total_leaves = 0;
	uint32_t opath = 0;
	uint32_t index = 0;

	enum {
		ALLOCA_STACK_SIZE = 128
	};

	_FORCE_INLINE_ void _delete_node(Node *p_node);
	void _recurse_delete_node(Node *p_node);
	_FORCE_INLINE_ Node *_create_node(Node *p_parent, void *p_data);
	_FORCE_INLINE_ DynamicBVH::Node *_create_node_with_volume(Node *p_parent, const Volume &p_volume, void *p_data);
	_FORCE_INLINE_ void _insert_leaf(Node *p_root, Node *p_leaf);
	_FORCE_INLINE_ Node *_remove_leaf(Node *r_leaf);
	void _fetch_leaves(Node *p_root, LocalVector<Node *> &r_leaves, int p_depth = -1);
	static int _split(Node **r_leaves, int p_count, const Vector3 &p_org, const Vector3 &p_axis);
	static Volume _bounds(Node **r_leaves, int p_count);
	void _bottom_up(Node **r_leaves, int p_count);
	Node *_top_down(Node **r_leaves, int p_count, int p_bu_threshold);
	Node *_node_sort(Node *r_node, Node *&r_root);

	_FORCE_INLINE_ void _update(Node *r_leaf, int p_lookahead = -1);

	void _extract_leaves(Node *p_node, List<ID> *r_elements);

	_FORCE_INLINE_ bool _ray_aabb(const Vector3 &p_ray_from, const Vector3 &p_ray_inv_direction, const unsigned int p_ray_sign[3], const Vector3 p_bounds[2], real_t &r_tmin, real_t p_lambda_min, real_t p_lambda_max) {
		real_t tmax, tymin, tymax, tzmin, tzmax;
		r_tmin = (p_bounds[p_ray_sign[0]].x - p_ray_from.x) * p_ray_inv_direction.x;
		tmax = (p_bounds[1 - p_ray_sign[0]].x - p_ray_from.x) * p_ray_inv_direction.x;
		tymin = (p_bounds[p_ray_sign[1]].y - p_ray_from.y) * p_ray_inv_direction.y;
		tymax = (p_bounds[1 - p_ray_sign[1]].y - p_ray_from.y) * p_ray_inv_direction.y;

		if ((r_tmin > tymax) || (tymin > tmax)) {
			return false;
		}

		if (tymin > r_tmin) {
			r_tmin = tymin;
		}

		if (tymax < tmax) {
			tmax = tymax;
		}

		tzmin = (p_bounds[p_ray_sign[2]].z - p_ray_from.z) * p_ray_inv_direction.z;
		tzmax = (p_bounds[1 - p_ray_sign[2]].z - p_ray_from.z) * p_ray_inv_direction.z;

		if ((r_tmin > tzmax) || (tzmin > tmax)) {
			return false;
		}
		if (tzmin > r_tmin) {
			r_tmin = tzmin;
		}
		if (tzmax < tmax) {
			tmax = tzmax;
		}
		return ((r_tmin < p_lambda_max) && (tmax > p_lambda_min));
	}

public:
	// Methods
	void clear();
	bool is_empty() const { return (nullptr == bvh_root); }
	void optimize_bottom_up();
	void optimize_top_down(int p_bu_threshold = 128);
	void optimize_incremental(int p_passes);
	ID insert(const AABB &p_box, void *p_userdata);
	bool update(const ID &p_id, const AABB &p_box);
	void remove(const ID &p_id);
	void get_elements(List<ID> *r_elements);

	int get_leaf_count() const;
	int get_max_depth() const;

	/* Discouraged, but works as a reference on how it must be used */
	struct DefaultQueryResult {
		virtual bool operator()(void *p_data) = 0; //return true whether you want to continue the query
		virtual ~DefaultQueryResult() {}
	};

	template <typename QueryResult>
	_FORCE_INLINE_ void aabb_query(const AABB &p_aabb, QueryResult &r_result);
	template <typename QueryResult>
	_FORCE_INLINE_ void convex_query(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count, QueryResult &r_result);
	template <typename QueryResult>
	_FORCE_INLINE_ void ray_query(const Vector3 &p_from, const Vector3 &p_to, QueryResult &r_result);

	void set_index(uint32_t p_index);
	uint32_t get_index() const;

	~DynamicBVH();
};

template <typename QueryResult>
void DynamicBVH::aabb_query(const AABB &p_box, QueryResult &r_result) {
	if (!bvh_root) {
		return;
	}

	Volume volume;
	volume.min = p_box.position;
	volume.max = p_box.position + p_box.size;

	const Node **alloca_stack = (const Node **)alloca(ALLOCA_STACK_SIZE * sizeof(const Node *));
	const Node **stack = alloca_stack;
	stack[0] = bvh_root;
	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;

	LocalVector<const Node *> aux_stack; //only used in rare occasions when you run out of alloca memory because tree is too unbalanced. Should correct itself over time.

	do {
		depth--;
		const Node *n = stack[depth];
		if (n->volume.intersects(volume)) {
			if (n->is_internal()) {
				if (depth > threshold) {
					if (aux_stack.is_empty()) {
						aux_stack.resize(ALLOCA_STACK_SIZE * 2);
						memcpy(aux_stack.ptr(), alloca_stack, ALLOCA_STACK_SIZE * sizeof(const Node *));
						alloca_stack = nullptr;
					} else {
						aux_stack.resize(aux_stack.size() * 2);
					}
					stack = aux_stack.ptr();
					threshold = aux_stack.size() - 2;
				}
				stack[depth++] = n->children[0];
				stack[depth++] = n->children[1];
			} else {
				if (r_result(n->data)) {
					return;
				}
			}
		}
	} while (depth > 0);
}

template <typename QueryResult>
void DynamicBVH::convex_query(const Plane *p_planes, int p_plane_count, const Vector3 *p_points, int p_point_count, QueryResult &r_result) {
	if (!bvh_root) {
		return;
	}

	//generate a volume anyway to improve pre-testing
	Volume volume;
	for (int i = 0; i < p_point_count; i++) {
		if (i == 0) {
			volume.min = p_points[0];
			volume.max = p_points[0];
		} else {
			volume.min = volume.min.min(p_points[i]);
			volume.max = volume.max.max(p_points[i]);
		}
	}

	const Node **alloca_stack = (const Node **)alloca(ALLOCA_STACK_SIZE * sizeof(const Node *));
	const Node **stack = alloca_stack;
	stack[0] = bvh_root;
	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;

	LocalVector<const Node *> aux_stack; //only used in rare occasions when you run out of alloca memory because tree is too unbalanced. Should correct itself over time.

	do {
		depth--;
		const Node *n = stack[depth];
		if (n->volume.intersects(volume) && n->volume.intersects_convex(p_planes, p_plane_count, p_points, p_point_count)) {
			if (n->is_internal()) {
				if (depth > threshold) {
					if (aux_stack.is_empty()) {
						aux_stack.resize(ALLOCA_STACK_SIZE * 2);
						memcpy(aux_stack.ptr(), alloca_stack, ALLOCA_STACK_SIZE * sizeof(const Node *));
						alloca_stack = nullptr;
					} else {
						aux_stack.resize(aux_stack.size() * 2);
					}
					stack = aux_stack.ptr();
					threshold = aux_stack.size() - 2;
				}
				stack[depth++] = n->children[0];
				stack[depth++] = n->children[1];
			} else {
				if (r_result(n->data)) {
					return;
				}
			}
		}
	} while (depth > 0);
}
template <typename QueryResult>
void DynamicBVH::ray_query(const Vector3 &p_from, const Vector3 &p_to, QueryResult &r_result) {
	if (!bvh_root) {
		return;
	}

	Vector3 ray_dir = (p_to - p_from);
	ray_dir.normalize();

	///what about division by zero? --> just set rayDirection[i] to INF/B3_LARGE_FLOAT
	Vector3 inv_dir;
	inv_dir[0] = ray_dir[0] == real_t(0.0) ? real_t(1e20) : real_t(1.0) / ray_dir[0];
	inv_dir[1] = ray_dir[1] == real_t(0.0) ? real_t(1e20) : real_t(1.0) / ray_dir[1];
	inv_dir[2] = ray_dir[2] == real_t(0.0) ? real_t(1e20) : real_t(1.0) / ray_dir[2];
	unsigned int signs[3] = { inv_dir[0] < 0.0, inv_dir[1] < 0.0, inv_dir[2] < 0.0 };

	real_t lambda_max = ray_dir.dot(p_to - p_from);

	Vector3 bounds[2];

	const Node **alloca_stack = (const Node **)alloca(ALLOCA_STACK_SIZE * sizeof(const Node *));
	const Node **stack = alloca_stack;
	stack[0] = bvh_root;
	int32_t depth = 1;
	int32_t threshold = ALLOCA_STACK_SIZE - 2;

	LocalVector<const Node *> aux_stack; //only used in rare occasions when you run out of alloca memory because tree is too unbalanced. Should correct itself over time.

	do {
		depth--;
		const Node *node = stack[depth];
		bounds[0] = node->volume.min;
		bounds[1] = node->volume.max;
		real_t tmin = 1.f, lambda_min = 0.f;
		unsigned int result1 = false;
		result1 = _ray_aabb(p_from, inv_dir, signs, bounds, tmin, lambda_min, lambda_max);
		if (result1) {
			if (node->is_internal()) {
				if (depth > threshold) {
					if (aux_stack.is_empty()) {
						aux_stack.resize(ALLOCA_STACK_SIZE * 2);
						memcpy(aux_stack.ptr(), alloca_stack, ALLOCA_STACK_SIZE * sizeof(const Node *));
						alloca_stack = nullptr;
					} else {
						aux_stack.resize(aux_stack.size() * 2);
					}
					stack = aux_stack.ptr();
					threshold = aux_stack.size() - 2;
				}
				stack[depth++] = node->children[0];
				stack[depth++] = node->children[1];
			} else {
				if (r_result(node->data)) {
					return;
				}
			}
		}
	} while (depth > 0);
}

// --- End core/math/dynamic_bvh.h ---
#include "core/math/geometry_3d.h"
// --- Folded from core/templates/bin_sorted_array.h ---
#include "core/templates/local_vector.h"
#include "core/types.h"

template <typename T>
class _WARN_UNUSED_ BinSortedArray {
	PagedArray<T> array;
	LocalVector<uint64_t> bin_limits;

	// Implement if elements need to keep track of their own index in the array.
	_FORCE_INLINE_ virtual void _update_idx(T &r_element, uint64_t p_idx) {}

	_FORCE_INLINE_ void _swap(uint64_t p_a, uint64_t p_b) {
		SWAP(array[p_a], array[p_b]);
		_update_idx(array[p_a], p_a);
		_update_idx(array[p_b], p_b);
	}

public:
	uint64_t insert(T &p_element, uint64_t p_bin) {
		array.push_back(p_element);
		uint64_t new_idx = array.size() - 1;
		_update_idx(p_element, new_idx);
		bin_limits[0] = new_idx;
		if (p_bin != 0) {
			new_idx = move(new_idx, p_bin);
		}
		return new_idx;
	}

	uint64_t move(uint64_t p_idx, uint64_t p_bin) {
		ERR_FAIL_UNSIGNED_INDEX_V(p_idx, array.size(), -1);

		uint64_t current_bin = bin_limits.size() - 1;
		while (p_idx > bin_limits[current_bin]) {
			current_bin--;
		}

		if (p_bin == current_bin) {
			return p_idx;
		}

		uint64_t current_idx = p_idx;
		if (p_bin > current_bin) {
			while (p_bin > current_bin) {
				uint64_t swap_idx = 0;

				if (current_bin == bin_limits.size() - 1) {
					bin_limits.push_back(0);
				} else {
					bin_limits[current_bin + 1]++;
					swap_idx = bin_limits[current_bin + 1];
				}

				if (current_idx != swap_idx) {
					_swap(current_idx, swap_idx);
					current_idx = swap_idx;
				}

				current_bin++;
			}
		} else {
			while (p_bin < current_bin) {
				uint64_t swap_idx = bin_limits[current_bin];

				if (current_idx != swap_idx) {
					_swap(current_idx, swap_idx);
				}

				if (current_bin == bin_limits.size() - 1 && bin_limits[current_bin] == 0) {
					bin_limits.resize(bin_limits.size() - 1);
				} else {
					bin_limits[current_bin]--;
				}
				current_idx = swap_idx;
				current_bin--;
			}
		}

		return current_idx;
	}

	void remove_at(uint64_t p_idx) {
		ERR_FAIL_UNSIGNED_INDEX(p_idx, array.size());
		uint64_t new_idx = move(p_idx, 0);
		uint64_t swap_idx = array.size() - 1;

		if (new_idx != swap_idx) {
			_swap(new_idx, swap_idx);
		}

		if (bin_limits[0] > 0) {
			bin_limits[0]--;
		}

		array.pop_back();
	}

	void set_page_pool(PagedArrayPool<T> *p_page_pool) {
		array.set_page_pool(p_page_pool);
	}

	_FORCE_INLINE_ const T &operator[](uint64_t p_index) const {
		return array[p_index];
	}

	_FORCE_INLINE_ T &operator[](uint64_t p_index) {
		return array[p_index];
	}

	int get_bin_count() {
		if (array.size() == 0) {
			return 0;
		}
		return bin_limits.size();
	}

	int get_bin_start(int p_bin) {
		ERR_FAIL_COND_V(p_bin >= get_bin_count(), ~0U);
		if ((unsigned int)p_bin == bin_limits.size() - 1) {
			return 0;
		}
		return bin_limits[p_bin + 1] + 1;
	}

	int get_bin_size(int p_bin) {
		ERR_FAIL_COND_V(p_bin >= get_bin_count(), 0);
		if ((unsigned int)p_bin == bin_limits.size() - 1) {
			return bin_limits[p_bin] + 1;
		}
		return bin_limits[p_bin] - bin_limits[p_bin + 1];
	}

	void reset() {
		array.reset();
		bin_limits.clear();
		bin_limits.push_back(0);
	}

	BinSortedArray() {
		bin_limits.push_back(0);
	}

	virtual ~BinSortedArray() {
		reset();
	}
};

// --- End core/templates/bin_sorted_array.h ---
#include "core/templates/local_vector.h"
#include "core/types.h"
#include "core/math/plane.h"

// --- Folded from servers/rendering/renderer_scene_occlusion_cull.h ---
#include "core/io/image.h"
#include "core/types.h"
#include "core/templates/local_vector.h"
#include "scene/resources/texture.h"

#include <cfloat> // FLT_MIN, FLT_MAX

class RendererSceneOcclusionCull final
{
public:
	class HZBuffer
	{
	public:
		static const int TILE_SIZE = 4;
		static const int TILE_RAYS = TILE_SIZE * TILE_SIZE;
		struct CameraRayThreadData;

	protected:
		LocalVector<float> data;
		LocalVector<Size2i> sizes;
		LocalVector<float *> mips;

		RID debug_texture;
		Ref<Image> debug_image;
		PackedByteArray debug_data;
		float debug_tex_range = 0.0f;

		uint64_t occlusion_frame = 0;
		Size2i occlusion_buffer_size;

		Size2i tile_grid_size;

		void _clear_mips();
		void _resize_mips(const Size2i &p_size);
		void _camera_rays_threaded(uint32_t p_thread, const CameraRayThreadData *p_data);
		void _generate_camera_rays(const CameraRayThreadData *p_data, int p_from, int p_to);

		_FORCE_INLINE_ bool _is_occluded(const real_t p_bounds[6], const Vector3 &p_cam_position, const Transform3D &p_cam_inv_transform, const Projection &p_cam_projection, real_t p_near, bool p_is_orthogonal) const {
			if (is_empty()) {
				return false;
			}

			Vector3 closest_point = p_cam_position.clamp(Vector3(p_bounds[0], p_bounds[1], p_bounds[2]), Vector3(p_bounds[3], p_bounds[4], p_bounds[5]));

			if (closest_point == p_cam_position) {
				return false;
			}

			Vector3 closest_point_view = p_cam_inv_transform.xform(closest_point);
			if (closest_point_view.z > -p_near) {
				return false;
			}

			// Force distance calculation to use double precision to avoid floating-point overflow for distant objects.
			closest_point = closest_point - p_cam_position;
			float min_depth = Math::sqrt((double)closest_point.x * (double)closest_point.x + (double)closest_point.y * (double)closest_point.y + (double)closest_point.z * (double)closest_point.z);

			Vector2 rect_min = Vector2(FLT_MAX, FLT_MAX);
			Vector2 rect_max = Vector2(FLT_MIN, FLT_MIN);

			for (int j = 0; j < 8; j++) {
				// Bitmask to cycle through the corners of the AABB.
				Vector3 corner = Vector3(
						j & 4 ? p_bounds[0] : p_bounds[3],
						j & 2 ? p_bounds[1] : p_bounds[4],
						j & 1 ? p_bounds[2] : p_bounds[5]);
				Vector3 view = p_cam_inv_transform.xform(corner);

				// When using an orthogonal camera, the closest point of an AABB to the camera is guaranteed to be a corner.
				if (p_is_orthogonal) {
					min_depth = MIN(min_depth, -view.z);
				}

				Vector3 projected = p_cam_projection.xform(view);

				if (-view.z < 0.0) {
					rect_min = Vector2(0.0f, 0.0f);
					rect_max = Vector2(1.0f, 1.0f);
					break;
				}

				Vector2 normalized = Vector2(projected.x * 0.5f + 0.5f, projected.y * 0.5f + 0.5f);
				rect_min = rect_min.min(normalized);
				rect_max = rect_max.max(normalized);
			}

			rect_max = rect_max.minf(1);
			rect_min = rect_min.maxf(0);

			int mip_count = mips.size();

			Vector2 screen_diagonal = (rect_max - rect_min) * sizes[0];
			float size = MAX(screen_diagonal.x, screen_diagonal.y);
			float l = Math::ceil(Math::log2(size));
			int lod = CLAMP(l, 0, mip_count - 1);

			const int max_samples = 512;
			int sample_count = 0;
			bool visible = true;

			for (; lod >= 0; lod--) {
				int w = sizes[lod].x;
				int h = sizes[lod].y;

				int minx = CLAMP(rect_min.x * w - 1, 0, w - 1);
				int maxx = CLAMP(rect_max.x * w + 1, 0, w - 1);

				int miny = CLAMP(rect_min.y * h - 1, 0, h - 1);
				int maxy = CLAMP(rect_max.y * h + 1, 0, h - 1);

				sample_count += (maxx - minx + 1) * (maxy - miny + 1);

				if (sample_count > max_samples) {
					return false;
				}

				visible = false;
				for (int y = miny; y <= maxy; y++) {
					for (int x = minx; x <= maxx; x++) {
						float depth = mips[lod][y * w + x];
						if (depth > min_depth) {
							visible = true;
							break;
						}
					}
					if (visible) {
						break;
					}
				}

				if (!visible) {
					return true;
				}
			}

			return !visible;
		}

	public:
		static bool occlusion_jitter_enabled;

		unsigned int camera_rays_tile_count = 0;
		uint8_t *camera_rays_unaligned_buffer = nullptr;
		void *camera_rays = nullptr;
		LocalVector<uint32_t> camera_ray_masks;
		RID scenario_rid;

		_FORCE_INLINE_ bool is_empty() const {
			return sizes.is_empty();
		}

		void clear();
		void resize(const Size2i &p_size);
		void sort_rays(const Vector3 &p_camera_dir, bool p_orthogonal);
		void update_camera_rays(const Transform3D &p_cam_transform, const Vector3 &p_near_bottom_left, const Vector2 &p_near_extents, real_t p_z_far, bool p_cam_orthogonal);

		void update_mips();

		// Thin wrapper around _is_occluded(),
		// allowing occlusion timers to delay the disappearance
		// of objects to prevent flickering when using jittering.
		_FORCE_INLINE_ bool is_occluded(const real_t p_bounds[6], const Vector3 &p_cam_position, const Transform3D &p_cam_inv_transform, const Projection &p_cam_projection, real_t p_near, bool p_is_orthogonal, uint64_t &r_occlusion_timeout) const {
			bool occluded = _is_occluded(p_bounds, p_cam_position, p_cam_inv_transform, p_cam_projection, p_near, p_is_orthogonal);

			// Special case, temporal jitter disabled,
			// so we don't use occlusion timers.
			if (!occlusion_jitter_enabled) {
				return occluded;
			}

			if (!occluded) {
//#define DEBUG_RASTER_OCCLUSION_JITTER
#ifdef DEBUG_RASTER_OCCLUSION_JITTER
				r_occlusion_timeout = occlusion_frame + 1;
#else
				r_occlusion_timeout = occlusion_frame + 9;
#endif
			} else if (r_occlusion_timeout) {
				// Regular timeout, allow occlusion culling
				// to proceed as normal after the delay.
				if (occlusion_frame >= r_occlusion_timeout) {
					r_occlusion_timeout = 0;
				}
			}

			return occluded && !r_occlusion_timeout;
		}

		RID get_debug_texture();
		const Size2i &get_occlusion_buffer_size() const { return occlusion_buffer_size; }

		~HZBuffer();
	};

private:
	static void _print_warning()
	{
		WARN_PRINT_ONCE("Occlusion culling is disabled at build-time.");
	}

	static bool _default_is_occluder(RID p_rid) { return false; }
	static RID _default_occluder_allocate() { return RID(); }
	static void _default_occluder_initialize(RID p_occluder) {}
	static void _default_free_occluder(RID p_occluder) { _print_warning(); }
	static void _default_occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
	{
		_print_warning();
	}

	static void _default_add_scenario(RID p_scenario) {}
	static void _default_remove_scenario(RID p_scenario) {}
	static void _default_scenario_set_instance(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled)
	{
		_print_warning();
	}
	static void _default_scenario_remove_instance(RID p_scenario, RID p_instance)
	{
		_print_warning();
	}

	static void _default_add_buffer(RID p_buffer) { _print_warning(); }
	static void _default_remove_buffer(RID p_buffer) { _print_warning(); }
	static HZBuffer* _default_buffer_get_ptr(RID p_buffer) { return nullptr; }
	static void _default_buffer_set_scenario(RID p_buffer, RID p_scenario) { _print_warning(); }
	static void _default_buffer_set_size(RID p_buffer, const Vector2i& p_size) { _print_warning(); }
	static void _default_buffer_update(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal)
	{
	}

	static RID _default_buffer_get_debug_texture(RID p_buffer)
	{
		_print_warning();
		return RID();
	}

	static void _default_set_build_quality(RSE::ViewportOcclusionCullingBuildQuality p_quality) {}

public:
	inline static bool (*is_occluder)(RID p_rid) = &_default_is_occluder;
	inline static RID (*occluder_allocate)() = &_default_occluder_allocate;
	inline static void (*occluder_initialize)(RID p_occluder) = &_default_occluder_initialize;
	inline static void (*free_occluder)(RID p_occluder) = &_default_free_occluder;
	inline static void (*occluder_set_mesh)(RID p_occluder, const PackedVector3Array& p_vertices,
		const PackedInt32Array& p_indices) = &_default_occluder_set_mesh;

	inline static void (*add_scenario)(RID p_scenario) = &_default_add_scenario;
	inline static void (*remove_scenario)(RID p_scenario) = &_default_remove_scenario;
	inline static void (*scenario_set_instance)(RID p_scenario, RID p_instance, RID p_occluder,
		const Transform3D& p_xform, bool p_enabled) = &_default_scenario_set_instance;
	inline static void (*scenario_remove_instance)(
		RID p_scenario, RID p_instance) = &_default_scenario_remove_instance;

	inline static void (*add_buffer)(RID p_buffer) = &_default_add_buffer;
	inline static void (*remove_buffer)(RID p_buffer) = &_default_remove_buffer;
	inline static HZBuffer* (*buffer_get_ptr)(RID p_buffer) = &_default_buffer_get_ptr;
	inline static void (*buffer_set_scenario)(
		RID p_buffer, RID p_scenario) = &_default_buffer_set_scenario;
	inline static void (*buffer_set_size)(
		RID p_buffer, const Vector2i& p_size) = &_default_buffer_set_size;
	inline static void (*buffer_update)(RID p_buffer, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal) = &_default_buffer_update;

	inline static RID (*buffer_get_debug_texture)(RID p_buffer) = &_default_buffer_get_debug_texture;

	inline static void (*set_build_quality)(
		RSE::ViewportOcclusionCullingBuildQuality p_quality) = &_default_set_build_quality;

	template <typename Impl> static void bind()
	{
		is_occluder = &Impl::is_occluder;
		occluder_allocate = &Impl::occluder_allocate;
		occluder_initialize = &Impl::occluder_initialize;
		free_occluder = &Impl::free_occluder;
		occluder_set_mesh = &Impl::occluder_set_mesh;

		add_scenario = &Impl::add_scenario;
		remove_scenario = &Impl::remove_scenario;
		scenario_set_instance = &Impl::scenario_set_instance;
		scenario_remove_instance = &Impl::scenario_remove_instance;

		add_buffer = &Impl::add_buffer;
		remove_buffer = &Impl::remove_buffer;
		buffer_get_ptr = &Impl::buffer_get_ptr;
		buffer_set_scenario = &Impl::buffer_set_scenario;
		buffer_set_size = &Impl::buffer_set_size;
		buffer_update = &Impl::buffer_update;

		buffer_get_debug_texture = &Impl::buffer_get_debug_texture;

		set_build_quality = &Impl::set_build_quality;
	}

	static void reset()
	{
		is_occluder = &_default_is_occluder;
		occluder_allocate = &_default_occluder_allocate;
		occluder_initialize = &_default_occluder_initialize;
		free_occluder = &_default_free_occluder;
		occluder_set_mesh = &_default_occluder_set_mesh;

		add_scenario = &_default_add_scenario;
		remove_scenario = &_default_remove_scenario;
		scenario_set_instance = &_default_scenario_set_instance;
		scenario_remove_instance = &_default_scenario_remove_instance;

		add_buffer = &_default_add_buffer;
		remove_buffer = &_default_remove_buffer;
		buffer_get_ptr = &_default_buffer_get_ptr;
		buffer_set_scenario = &_default_buffer_set_scenario;
		buffer_set_size = &_default_buffer_set_size;
		buffer_update = &_default_buffer_update;

		buffer_get_debug_texture = &_default_buffer_get_debug_texture;

		set_build_quality = &_default_set_build_quality;
	}

	RendererSceneOcclusionCull() = delete;
	RendererSceneOcclusionCull(const RendererSceneOcclusionCull&) = delete;
	RendererSceneOcclusionCull& operator=(const RendererSceneOcclusionCull&) = delete;
	~RendererSceneOcclusionCull() = delete;
};

// --- End servers/rendering/renderer_scene_occlusion_cull.h ---

#include "servers/rendering/rendering_device.h"

// --- Folded from servers/xr/xr_server.h ---
#include "core/os/os.h"
#include "core/types.h"

class XRInterface;
class XRTracker;

class XRServer final
{
	static inline Mutex _thread_safe_;

public:
	enum XRMode
	{
		XRMODE_DEFAULT, /* Default behavior, means we check project settings */
		XRMODE_OFF,		/* Ignore project settings, disable OpenXR, disable shaders */
		XRMODE_ON, /* Ignore project settings, enable OpenXR, enable shaders, run editor in VR (if
					  applicable) */
	};

	enum TrackerType
	{
		TRACKER_HEAD = 0x01, /* tracks the position of the players head (or in case of handheld AR,
								location of the phone) */
		TRACKER_CONTROLLER = 0x02,	/* tracks a controller */
		TRACKER_BASESTATION = 0x04, /* tracks location of a base station */
		TRACKER_ANCHOR =
			0x08,			 /* tracks an anchor point, used in AR to track a real live location */
		TRACKER_HAND = 0x10, /* tracks a hand */
		TRACKER_BODY = 0x20, /* tracks a body */
		TRACKER_FACE = 0x40, /* tracks a face */
		TRACKER_UNKNOWN = 0x80, /* unknown tracker */

		TRACKER_ANY_KNOWN = 0x7f, /* all except unknown */
		TRACKER_ANY = 0xff		  /* used by get_connected_trackers to return all types */
	};

	enum RotationMode
	{
		RESET_FULL_ROTATION = 0, /* we reset the full rotation, regardless of how the HMD is
									oriented, we're looking dead ahead */
		RESET_BUT_KEEP_TILT = 1, /* reset rotation but keep tilt. */
		DONT_RESET_ROTATION = 2, /* don't reset the rotation, we will only center on position */
	};

private:
	static XRMode xr_mode;

	// As we may be updating our main state for our next frame while we're still rendering our
	// previous frame, we need to keep copies around.
	struct RenderState
	{
		double world_scale = 1.0; /* scale by which we multiply our tracker positions */
		Transform3D world_origin; /* our world origin point, maps a location in our virtual world to
									 the origin point in our real world tracking volume */
		Transform3D reference_frame; /* our reference frame */
	};

	struct Data
	{
		double world_scale = 1.0; /* scale by which we multiply our tracker positions */
		bool camera_locked_to_origin = false;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void _set_render_world_scale(double p_world_scale);
	static void _set_render_world_origin(const Transform3D& p_world_origin);
	static void _set_render_reference_frame(const Transform3D& p_reference_frame);

	static void set_render_world_scale(double p_world_scale);
	static void set_render_world_origin(const Transform3D& p_world_origin);
	static void set_render_reference_frame(const Transform3D& p_reference_frame);

#ifndef DISABLE_DEPRECATED
	static void _bind_compatibility_methods();
	static void _add_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker);
	static void _remove_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker);
	static Ref<XRTracker> _get_tracker_bind_compat_90645(const String& p_name);
#endif

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static XRMode get_xr_mode();
	static void set_xr_mode(XRMode p_mode);

	static double get_world_scale();
	static void set_world_scale(double p_world_scale);

	static Transform3D get_world_origin();
	static void set_world_origin(const Transform3D& p_world_origin);

	static Transform3D get_reference_frame();
	static void clear_reference_frame();
	static void center_on_hmd(RotationMode p_rotation_mode, bool p_keep_height);

	static Transform3D get_hmd_transform();

	static void set_camera_locked_to_origin(bool p_enable);

	static inline bool is_camera_locked_to_origin()
	{
		return data ? data->camera_locked_to_origin : false;
	}

	static void add_interface(const Ref<XRInterface>& p_interface);
	static void remove_interface(const Ref<XRInterface>& p_interface);
	static int get_interface_count();
	static Ref<XRInterface> get_interface(int p_index);
	static Ref<XRInterface> find_interface(const String& p_name);

	static Ref<XRInterface> get_primary_interface();
	static void set_primary_interface(const Ref<XRInterface>& p_primary_interface);

	static void add_tracker(const Ref<XRTracker>& p_tracker);
	static void remove_tracker(const Ref<XRTracker>& p_tracker);
	static Ref<XRTracker> get_tracker(const String& p_name);

	static PackedStringArray get_suggested_tracker_names();
	static PackedStringArray get_suggested_pose_names(const String& p_tracker_name);

	static void _process();
	static void pre_render();
	static void end_frame();

	XRServer() = delete;
	XRServer(const XRServer&) = delete;
	XRServer& operator=(const XRServer&) = delete;
	~XRServer() = delete;
};

#define XR XRServer

// --- Folded from servers/xr/xr_tracker.h ---
#include "core/os/os.h"
#include "core/types.h"

class XRPose final : public RefCounted
{
public:
	// TrackingConfidence gives an indication of how reliable our transform data is.
	enum TrackingConfidence
	{
		XR_TRACKING_CONFIDENCE_NONE, // No tracking information is available for this pose.
		XR_TRACKING_CONFIDENCE_LOW,	 // Tracking information may be inaccurate or estimated.
		XR_TRACKING_CONFIDENCE_HIGH	 // Tracking information is deemed accurate and up to date.
	};

private:
	bool has_tracking_data = false;
	String name;
	Transform3D transform;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	TrackingConfidence tracking_confidence = XR_TRACKING_CONFIDENCE_NONE;

public:
	void set_has_tracking_data(const bool p_has_tracking_data);
	bool get_has_tracking_data() const;

	void set_name(const String& p_name);
	String get_name() const;

	void set_transform(const Transform3D p_transform);
	Transform3D get_transform() const;
	Transform3D get_adjusted_transform() const;

	void set_linear_velocity(const Vector3 p_velocity);
	Vector3 get_linear_velocity() const;

	void set_angular_velocity(const Vector3 p_velocity);
	Vector3 get_angular_velocity() const;

	void set_tracking_confidence(const TrackingConfidence p_tracking_confidence);
	TrackingConfidence get_tracking_confidence() const;
};

/**
	Unified XR tracker representing positional, controller, body, hand, and face trackers,
	discriminated by XRServer::TrackerType.
*/

class XRTracker final : public RefCounted
{
	_THREAD_SAFE_CLASS_

public:
	enum TrackerHand
	{
		TRACKER_HAND_UNKNOWN, /* unknown or not applicable */
		TRACKER_HAND_LEFT,	  /* controller/hand is the left hand */
		TRACKER_HAND_RIGHT,	  /* controller/hand is the right hand */
		TRACKER_HAND_MAX
	};

	enum BodyFlags
	{
		BODY_FLAG_UPPER_BODY_SUPPORTED = 1,
		BODY_FLAG_LOWER_BODY_SUPPORTED = 2,
		BODY_FLAG_HANDS_SUPPORTED = 4,
	};

	enum Joint
	{
		JOINT_ROOT,

		// Upper Body Joints
		JOINT_HIPS,
		JOINT_SPINE,
		JOINT_CHEST,
		JOINT_UPPER_CHEST,
		JOINT_NECK,
		JOINT_HEAD,
		JOINT_HEAD_TIP,
		JOINT_LEFT_SHOULDER,
		JOINT_LEFT_UPPER_ARM,
		JOINT_LEFT_LOWER_ARM,
		JOINT_RIGHT_SHOULDER,
		JOINT_RIGHT_UPPER_ARM,
		JOINT_RIGHT_LOWER_ARM,

		// Lower Body Joints
		JOINT_LEFT_UPPER_LEG,
		JOINT_LEFT_LOWER_LEG,
		JOINT_LEFT_FOOT,
		JOINT_LEFT_TOES,
		JOINT_RIGHT_UPPER_LEG,
		JOINT_RIGHT_LOWER_LEG,
		JOINT_RIGHT_FOOT,
		JOINT_RIGHT_TOES,

		// Left Hand Joints
		JOINT_LEFT_HAND,
		JOINT_LEFT_PALM,
		JOINT_LEFT_WRIST,
		JOINT_LEFT_THUMB_METACARPAL,
		JOINT_LEFT_THUMB_PHALANX_PROXIMAL,
		JOINT_LEFT_THUMB_PHALANX_DISTAL,
		JOINT_LEFT_THUMB_TIP,
		JOINT_LEFT_INDEX_FINGER_METACARPAL,
		JOINT_LEFT_INDEX_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_INDEX_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_INDEX_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_INDEX_FINGER_TIP,
		JOINT_LEFT_MIDDLE_FINGER_METACARPAL,
		JOINT_LEFT_MIDDLE_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_MIDDLE_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_MIDDLE_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_MIDDLE_FINGER_TIP,
		JOINT_LEFT_RING_FINGER_METACARPAL,
		JOINT_LEFT_RING_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_RING_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_RING_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_RING_FINGER_TIP,
		JOINT_LEFT_PINKY_FINGER_METACARPAL,
		JOINT_LEFT_PINKY_FINGER_PHALANX_PROXIMAL,
		JOINT_LEFT_PINKY_FINGER_PHALANX_INTERMEDIATE,
		JOINT_LEFT_PINKY_FINGER_PHALANX_DISTAL,
		JOINT_LEFT_PINKY_FINGER_TIP,

		// Right Hand Joints
		JOINT_RIGHT_HAND,
		JOINT_RIGHT_PALM,
		JOINT_RIGHT_WRIST,
		JOINT_RIGHT_THUMB_METACARPAL,
		JOINT_RIGHT_THUMB_PHALANX_PROXIMAL,
		JOINT_RIGHT_THUMB_PHALANX_DISTAL,
		JOINT_RIGHT_THUMB_TIP,
		JOINT_RIGHT_INDEX_FINGER_METACARPAL,
		JOINT_RIGHT_INDEX_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_INDEX_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_INDEX_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_INDEX_FINGER_TIP,
		JOINT_RIGHT_MIDDLE_FINGER_METACARPAL,
		JOINT_RIGHT_MIDDLE_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_MIDDLE_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_MIDDLE_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_MIDDLE_FINGER_TIP,
		JOINT_RIGHT_RING_FINGER_METACARPAL,
		JOINT_RIGHT_RING_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_RING_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_RING_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_RING_FINGER_TIP,
		JOINT_RIGHT_PINKY_FINGER_METACARPAL,
		JOINT_RIGHT_PINKY_FINGER_PHALANX_PROXIMAL,
		JOINT_RIGHT_PINKY_FINGER_PHALANX_INTERMEDIATE,
		JOINT_RIGHT_PINKY_FINGER_PHALANX_DISTAL,
		JOINT_RIGHT_PINKY_FINGER_TIP,

		// Extra joints that aren't part of the Godot humanoid skeleton, but are commonly used in
		// some VR avatars.
		JOINT_LOWER_CHEST,
		JOINT_LEFT_SCAPULA,
		JOINT_LEFT_WRIST_TWIST,
		JOINT_RIGHT_SCAPULA,
		JOINT_RIGHT_WRIST_TWIST,
		JOINT_LEFT_FOOT_TWIST,
		JOINT_LEFT_HEEL,
		JOINT_LEFT_MIDDLE_FOOT,
		JOINT_RIGHT_FOOT_TWIST,
		JOINT_RIGHT_HEEL,
		JOINT_RIGHT_MIDDLE_FOOT,

		JOINT_MAX,
	};

	enum JointFlags
	{
		JOINT_FLAG_ORIENTATION_VALID = 1,
		JOINT_FLAG_ORIENTATION_TRACKED = 2,
		JOINT_FLAG_POSITION_VALID = 4,
		JOINT_FLAG_POSITION_TRACKED = 8,
	};

	enum HandTrackingSource
	{
		HAND_TRACKING_SOURCE_UNKNOWN,
		HAND_TRACKING_SOURCE_UNOBSTRUCTED,
		HAND_TRACKING_SOURCE_CONTROLLER,
		HAND_TRACKING_SOURCE_NOT_TRACKED,
		HAND_TRACKING_SOURCE_MAX
	};

	enum HandJoint
	{
		HAND_JOINT_PALM,
		HAND_JOINT_WRIST,
		HAND_JOINT_THUMB_METACARPAL,
		HAND_JOINT_THUMB_PHALANX_PROXIMAL,
		HAND_JOINT_THUMB_PHALANX_DISTAL,
		HAND_JOINT_THUMB_TIP,
		HAND_JOINT_INDEX_FINGER_METACARPAL,
		HAND_JOINT_INDEX_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_INDEX_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_INDEX_FINGER_PHALANX_DISTAL,
		HAND_JOINT_INDEX_FINGER_TIP,
		HAND_JOINT_MIDDLE_FINGER_METACARPAL,
		HAND_JOINT_MIDDLE_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_MIDDLE_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_MIDDLE_FINGER_PHALANX_DISTAL,
		HAND_JOINT_MIDDLE_FINGER_TIP,
		HAND_JOINT_RING_FINGER_METACARPAL,
		HAND_JOINT_RING_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_RING_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_RING_FINGER_PHALANX_DISTAL,
		HAND_JOINT_RING_FINGER_TIP,
		HAND_JOINT_PINKY_FINGER_METACARPAL,
		HAND_JOINT_PINKY_FINGER_PHALANX_PROXIMAL,
		HAND_JOINT_PINKY_FINGER_PHALANX_INTERMEDIATE,
		HAND_JOINT_PINKY_FINGER_PHALANX_DISTAL,
		HAND_JOINT_PINKY_FINGER_TIP,
		HAND_JOINT_MAX,
	};

	enum HandJointFlags
	{
		HAND_JOINT_FLAG_ORIENTATION_VALID = 1,
		HAND_JOINT_FLAG_ORIENTATION_TRACKED = 2,
		HAND_JOINT_FLAG_POSITION_VALID = 4,
		HAND_JOINT_FLAG_POSITION_TRACKED = 8,
		HAND_JOINT_FLAG_LINEAR_VELOCITY_VALID = 16,
		HAND_JOINT_FLAG_ANGULAR_VELOCITY_VALID = 32,
	};

	enum BlendShapeEntry
	{
		// Base Shapes
		FT_EYE_LOOK_OUT_RIGHT,		 // Right eye looks outwards.
		FT_EYE_LOOK_IN_RIGHT,		 // Right eye looks inwards.
		FT_EYE_LOOK_UP_RIGHT,		 // Right eye looks upwards.
		FT_EYE_LOOK_DOWN_RIGHT,		 // Right eye looks downwards.
		FT_EYE_LOOK_OUT_LEFT,		 // Left eye looks outwards.
		FT_EYE_LOOK_IN_LEFT,		 // Left eye looks inwards.
		FT_EYE_LOOK_UP_LEFT,		 // Left eye looks upwards.
		FT_EYE_LOOK_DOWN_LEFT,		 // Left eye looks downwards.
		FT_EYE_CLOSED_RIGHT,		 // Closes the right eyelid.
		FT_EYE_CLOSED_LEFT,			 // Closes the left eyelid.
		FT_EYE_SQUINT_RIGHT,		 // Squeezes the right eye socket muscles.
		FT_EYE_SQUINT_LEFT,			 // Squeezes the left eye socket muscles.
		FT_EYE_WIDE_RIGHT,			 // Right eyelid widens beyond relaxed.
		FT_EYE_WIDE_LEFT,			 // Left eyelid widens beyond relaxed.
		FT_EYE_DILATION_RIGHT,		 // Dilates the right eye pupil.
		FT_EYE_DILATION_LEFT,		 // Dilates the left eye pupil.
		FT_EYE_CONSTRICT_RIGHT,		 // Constricts the right eye pupil.
		FT_EYE_CONSTRICT_LEFT,		 // Constricts the left eye pupil.
		FT_BROW_PINCH_RIGHT,		 // Right eyebrow pinches in.
		FT_BROW_PINCH_LEFT,			 // Left eyebrow pinches in.
		FT_BROW_LOWERER_RIGHT,		 // Outer right eyebrow pulls down.
		FT_BROW_LOWERER_LEFT,		 // Outer left eyebrow pulls down.
		FT_BROW_INNER_UP_RIGHT,		 // Inner right eyebrow pulls up.
		FT_BROW_INNER_UP_LEFT,		 // Inner left eyebrow pulls up.
		FT_BROW_OUTER_UP_RIGHT,		 // Outer right eyebrow pulls up.
		FT_BROW_OUTER_UP_LEFT,		 // Outer left eyebrow pulls up.
		FT_NOSE_SNEER_RIGHT,		 // Right side face sneers.
		FT_NOSE_SNEER_LEFT,			 // Left side face sneers.
		FT_NASAL_DILATION_RIGHT,	 // Right side nose canal dilates.
		FT_NASAL_DILATION_LEFT,		 // Left side nose canal dilates.
		FT_NASAL_CONSTRICT_RIGHT,	 // Right side nose canal constricts.
		FT_NASAL_CONSTRICT_LEFT,	 // Left side nose canal constricts.
		FT_CHEEK_SQUINT_RIGHT,		 // Raises the right side cheek.
		FT_CHEEK_SQUINT_LEFT,		 // Raises the left side cheek.
		FT_CHEEK_PUFF_RIGHT,		 // Puffs the right side cheek.
		FT_CHEEK_PUFF_LEFT,			 // Puffs the left side cheek.
		FT_CHEEK_SUCK_RIGHT,		 // Sucks in the right side cheek.
		FT_CHEEK_SUCK_LEFT,			 // Sucks in the left side cheek.
		FT_JAW_OPEN,				 // Opens jawbone.
		FT_MOUTH_CLOSED,			 // Closes the mouth.
		FT_JAW_RIGHT,				 // Pushes jawbone right.
		FT_JAW_LEFT,				 // Pushes jawbone left.
		FT_JAW_FORWARD,				 // Pushes jawbone forward.
		FT_JAW_BACKWARD,			 // Pushes jawbone backward.
		FT_JAW_CLENCH,				 // Flexes jaw muscles.
		FT_JAW_MANDIBLE_RAISE,		 // Raises the jawbone.
		FT_LIP_SUCK_UPPER_RIGHT,	 // Upper right lip part tucks in the mouth.
		FT_LIP_SUCK_UPPER_LEFT,		 // Upper left lip part tucks in the mouth.
		FT_LIP_SUCK_LOWER_RIGHT,	 // Lower right lip part tucks in the mouth.
		FT_LIP_SUCK_LOWER_LEFT,		 // Lower left lip part tucks in the mouth.
		FT_LIP_SUCK_CORNER_RIGHT,	 // Right lip corner folds into the mouth.
		FT_LIP_SUCK_CORNER_LEFT,	 // Left lip corner folds into the mouth.
		FT_LIP_FUNNEL_UPPER_RIGHT,	 // Upper right lip part pushes into a funnel.
		FT_LIP_FUNNEL_UPPER_LEFT,	 // Upper left lip part pushes into a funnel.
		FT_LIP_FUNNEL_LOWER_RIGHT,	 // Lower right lip part pushes into a funnel.
		FT_LIP_FUNNEL_LOWER_LEFT,	 // Lower left lip part pushes into a funnel.
		FT_LIP_PUCKER_UPPER_RIGHT,	 // Upper right lip part pushes outwards.
		FT_LIP_PUCKER_UPPER_LEFT,	 // Upper left lip part pushes outwards.
		FT_LIP_PUCKER_LOWER_RIGHT,	 // Lower right lip part pushes outwards.
		FT_LIP_PUCKER_LOWER_LEFT,	 // Lower left lip part pushes outwards.
		FT_MOUTH_UPPER_UP_RIGHT,	 // Upper right part of the lip pulls up.
		FT_MOUTH_UPPER_UP_LEFT,		 // Upper left part of the lip pulls up.
		FT_MOUTH_LOWER_DOWN_RIGHT,	 // Lower right part of the lip pulls up.
		FT_MOUTH_LOWER_DOWN_LEFT,	 // Lower left part of the lip pulls up.
		FT_MOUTH_UPPER_DEEPEN_RIGHT, // Upper right lip part pushes in the cheek.
		FT_MOUTH_UPPER_DEEPEN_LEFT,	 // Upper left lip part pushes in the cheek.
		FT_MOUTH_UPPER_RIGHT,		 // Moves upper lip right.
		FT_MOUTH_UPPER_LEFT,		 // Moves upper lip left.
		FT_MOUTH_LOWER_RIGHT,		 // Moves lower lip right.
		FT_MOUTH_LOWER_LEFT,		 // Moves lower lip left.
		FT_MOUTH_CORNER_PULL_RIGHT,	 // Right lip corner pulls diagonally up and out.
		FT_MOUTH_CORNER_PULL_LEFT,	 // Left lip corner pulls diagonally up and out.
		FT_MOUTH_CORNER_SLANT_RIGHT, // Right corner lip slants up.
		FT_MOUTH_CORNER_SLANT_LEFT,	 // Left corner lip slants up.
		FT_MOUTH_FROWN_RIGHT,		 // Right corner lip pulls down.
		FT_MOUTH_FROWN_LEFT,		 // Left corner lip pulls down.
		FT_MOUTH_STRETCH_RIGHT,		 // Mouth corner lip pulls out and down.
		FT_MOUTH_STRETCH_LEFT,		 // Mouth corner lip pulls out and down.
		FT_MOUTH_DIMPLE_RIGHT,		 // Right lip corner is pushed backwards.
		FT_MOUTH_DIMPLE_LEFT,		 // Left lip corner is pushed backwards.
		FT_MOUTH_RAISER_UPPER,		 // Raises and slightly pushes out the upper mouth.
		FT_MOUTH_RAISER_LOWER,		 // Raises and slightly pushes out the lower mouth.
		FT_MOUTH_PRESS_RIGHT,		 // Right side lips press and flatten together vertically.
		FT_MOUTH_PRESS_LEFT,		 // Left side lips press and flatten together vertically.
		FT_MOUTH_TIGHTENER_RIGHT,	 // Right side lips squeeze together horizontally.
		FT_MOUTH_TIGHTENER_LEFT,	 // Left side lips squeeze together horizontally.
		FT_TONGUE_OUT,				 // Tongue visibly sticks out of the mouth.
		FT_TONGUE_UP,				 // Tongue points upwards.
		FT_TONGUE_DOWN,				 // Tongue points downwards.
		FT_TONGUE_RIGHT,			 // Tongue points right.
		FT_TONGUE_LEFT,				 // Tongue points left.
		FT_TONGUE_ROLL,				 // Sides of the tongue funnel, creating a roll.
		FT_TONGUE_BLEND_DOWN,		 // Tongue arches up then down inside the mouth.
		FT_TONGUE_CURL_UP,			 // Tongue arches down then up inside the mouth.
		FT_TONGUE_SQUISH,			 // Tongue squishes together and thickens.
		FT_TONGUE_FLAT,				 // Tongue flattens and thins out.
		FT_TONGUE_TWIST_RIGHT, // Tongue tip rotates clockwise, with the rest following gradually.
		FT_TONGUE_TWIST_LEFT,  // Tongue tip rotates counter-clockwise, with the rest following
							   // gradually.
		FT_SOFT_PALATE_CLOSE,  // Inner mouth throat closes.
		FT_THROAT_SWALLOW,	   // The Adam's apple visibly swallows.
		FT_NECK_FLEX_RIGHT,	   // Right side neck visibly flexes.
		FT_NECK_FLEX_LEFT,	   // Left side neck visibly flexes.
		// Blended Shapes
		FT_EYE_CLOSED,		  // Closes both eye lids.
		FT_EYE_WIDE,		  // Widens both eye lids.
		FT_EYE_SQUINT,		  // Squints both eye lids.
		FT_EYE_DILATION,	  // Dilates both pupils.
		FT_EYE_CONSTRICT,	  // Constricts both pupils.
		FT_BROW_DOWN_RIGHT,	  // Pulls the right eyebrow down and in.
		FT_BROW_DOWN_LEFT,	  // Pulls the left eyebrow down and in.
		FT_BROW_DOWN,		  // Pulls both eyebrows down and in.
		FT_BROW_UP_RIGHT,	  // Right brow appears worried.
		FT_BROW_UP_LEFT,	  // Left brow appears worried.
		FT_BROW_UP,			  // Both brows appear worried.
		FT_NOSE_SNEER,		  // Entire face sneers.
		FT_NASAL_DILATION,	  // Both nose canals dilate.
		FT_NASAL_CONSTRICT,	  // Both nose canals constrict.
		FT_CHEEK_PUFF,		  // Puffs both cheeks.
		FT_CHEEK_SUCK,		  // Sucks in both cheeks.
		FT_CHEEK_SQUINT,	  // Raises both cheeks.
		FT_LIP_SUCK_UPPER,	  // Tucks in the upper lips.
		FT_LIP_SUCK_LOWER,	  // Tucks in the lower lips.
		FT_LIP_SUCK,		  // Tucks in both lips.
		FT_LIP_FUNNEL_UPPER,  // Funnels in the upper lips.
		FT_LIP_FUNNEL_LOWER,  // Funnels in the lower lips.
		FT_LIP_FUNNEL,		  // Funnels in both lips.
		FT_LIP_PUCKER_UPPER,  // Upper lip part pushes outwards.
		FT_LIP_PUCKER_LOWER,  // Lower lip part pushes outwards.
		FT_LIP_PUCKER,		  // Lips push outwards.
		FT_MOUTH_UPPER_UP,	  // Raises the upper lips.
		FT_MOUTH_LOWER_DOWN,  // Lowers the lower lips.
		FT_MOUTH_OPEN,		  // Mouth opens, revealing teeth.
		FT_MOUTH_RIGHT,		  // Moves mouth right.
		FT_MOUTH_LEFT,		  // Moves mouth left.
		FT_MOUTH_SMILE_RIGHT, // Right side of the mouth smiles.
		FT_MOUTH_SMILE_LEFT,  // Left side of the mouth smiles.
		FT_MOUTH_SMILE,		  // Mouth expresses a smile.
		FT_MOUTH_SAD_RIGHT,	  // Right side of the mouth expresses sadness.
		FT_MOUTH_SAD_LEFT,	  // Left side of the mouth expresses sadness.
		FT_MOUTH_SAD,		  // Mouth expresses sadness.
		FT_MOUTH_STRETCH,	  // Mouth stretches.
		FT_MOUTH_DIMPLE,	  // Lip corners dimple.
		FT_MOUTH_TIGHTENER,	  // Mouth tightens.
		FT_MOUTH_PRESS,		  // Mouth presses together.
		FT_MAX				  // Maximum blend shape.
	};

	struct InputValue
	{
		enum Type
		{
			INPUT_NONE,
			INPUT_BOOL,
			INPUT_FLOAT,
			INPUT_VECTOR2,
		};
		Type type = INPUT_NONE;
		union
		{
			bool bool_val;
			float float_val;
			struct
			{
				real_t x;
				real_t y;
			} vec2_val;
		};

		InputValue() : type(INPUT_NONE), vec2_val{0, 0} {}
	};

protected:
	XRServer::TrackerType type = XRServer::TRACKER_UNKNOWN;
	String name = "Unknown";
	String description;
	String profile;
	TrackerHand tracker_hand = TRACKER_HAND_UNKNOWN;
	bool has_tracking_data = false;

	HashMap<String, Ref<XRPose>> poses;
	HashMap<String, InputValue> inputs;

private:
	struct BodyData
	{
		uint32_t body_flags = 0;
		uint32_t joint_flags[JOINT_MAX] = {};
		Transform3D joint_transforms[JOINT_MAX];
	};

	struct HandData
	{
		HandTrackingSource hand_tracking_source = HAND_TRACKING_SOURCE_UNKNOWN;
		uint32_t hand_joint_flags[HAND_JOINT_MAX] = {};
		Transform3D hand_joint_transforms[HAND_JOINT_MAX];
		float hand_joint_radii[HAND_JOINT_MAX] = {};
		Vector3 hand_joint_linear_velocities[HAND_JOINT_MAX];
		Vector3 hand_joint_angular_velocities[HAND_JOINT_MAX];
	};

	BodyData* body_data = nullptr;
	HandData* hand_data = nullptr;
	PackedFloat32Array blend_shapes;
	Ref<RefCounted> custom_data;

	BodyData* _ensure_body_data();
	HandData* _ensure_hand_data();

public:
	void set_custom_data(const Ref<RefCounted>& p_custom_data) { custom_data = p_custom_data; }
	Ref<RefCounted> get_custom_data() const { return custom_data; }
	void set_tracker_type(XRServer::TrackerType p_type);
	XRServer::TrackerType get_tracker_type() const;
	void set_tracker_name(const String& p_name);
	String get_tracker_name() const;
	void set_tracker_desc(const String& p_desc);
	String get_tracker_desc() const;

	void set_tracker_profile(const String& p_profile);
	String get_tracker_profile() const;

	TrackerHand get_tracker_hand() const;
	void set_tracker_hand(const TrackerHand p_hand);

	void set_has_tracking_data(bool p_has_tracking_data);
	bool get_has_tracking_data() const;

	// Positional poses
	bool has_pose(const String& p_action_name) const;
	Ref<XRPose> get_pose(const String& p_action_name) const;
	void invalidate_pose(const String& p_action_name);
	void set_pose(const String& p_action_name, const Transform3D& p_transform,
		const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
		const XRPose::TrackingConfidence p_tracking_confidence =
			XRPose::XR_TRACKING_CONFIDENCE_HIGH);

	// Controller inputs
	void set_input(const String& p_action_name, bool p_value);
	void set_input(const String& p_action_name, float p_value);
	void set_input(const String& p_action_name, const Vector2& p_value);
	InputValue get_input(const String& p_action_name) const;
	bool is_button_pressed(const String& p_action_name) const;
	float get_input_float(const String& p_action_name) const;
	Vector2 get_input_vector2(const String& p_action_name) const;

	// Body tracking
	void set_body_flags(uint32_t p_body_flags);
	uint32_t get_body_flags() const;

	void set_joint_flags(Joint p_joint, uint32_t p_flags);
	uint32_t get_joint_flags(Joint p_joint) const;

	void set_joint_transform(Joint p_joint, const Transform3D& p_transform);
	Transform3D get_joint_transform(Joint p_joint) const;

	// Hand tracking
	void set_hand_tracking_source(HandTrackingSource p_source);
	HandTrackingSource get_hand_tracking_source() const;

	void set_hand_joint_flags(HandJoint p_joint, uint32_t p_flags);
	uint32_t get_hand_joint_flags(HandJoint p_joint) const;

	void set_hand_joint_transform(HandJoint p_joint, const Transform3D& p_transform);
	Transform3D get_hand_joint_transform(HandJoint p_joint) const;

	void set_hand_joint_radius(HandJoint p_joint, float p_radius);
	float get_hand_joint_radius(HandJoint p_joint) const;

	void set_hand_joint_linear_velocity(HandJoint p_joint, const Vector3& p_velocity);
	Vector3 get_hand_joint_linear_velocity(HandJoint p_joint) const;

	void set_hand_joint_angular_velocity(HandJoint p_joint, const Vector3& p_velocity);
	Vector3 get_hand_joint_angular_velocity(HandJoint p_joint) const;

	// Face tracking
	float get_blend_shape(BlendShapeEntry p_blend_shape) const;
	void set_blend_shape(BlendShapeEntry p_blend_shape, float p_value);

	PackedFloat32Array get_blend_shapes() const;
	void set_blend_shapes(const PackedFloat32Array& p_blend_shapes);

	XRTracker(XRServer::TrackerType p_type = XRServer::TRACKER_UNKNOWN);
	~XRTracker() override;
};

// --- Folded from servers/xr/xr_interface.h ---
#include "core/os/os.h"
#include "core/types.h"
#include "servers/rendering/rendering_device.h"
/**
	The XR interface is a template class on top of which we build interface to different AR, VR and
   tracking SDKs. The idea is that we subclass this class, implement the logic, and then instantiate
   a singleton of each interface when Godot starts. These instances do not initialize themselves but
   register themselves with the AR/VR server.

	If the user wants to enable AR/VR, they can choose the interface they want to use and initialize
   it.

	Note that we may make this into a fully instantiable class for GDExtension support.
*/

struct Projection;

namespace RenderingServerTypes
{
struct BlitToScreen;
}

class XRInterface : public RefCounted
{
public:
	enum Capabilities
	{				   /* purely metadata, provides some info about what this interface supports */
		XR_NONE = 0,   /* no capabilities */
		XR_MONO = 1,   /* can be used with mono output */
		XR_STEREO = 2, /* can be used with stereo output */
		XR_QUAD = 4,   /* can be used with quad output (not currently supported) */
		XR_VR = 8,	   /* offers VR support */
		XR_AR = 16,	   /* offers AR support */
		XR_EXTERNAL = 32 /* renders to external device */
	};

	enum TrackingStatus
	{ /* tracking status currently based on AR but we can start doing more with this for VR as well
	   */
		XR_NORMAL_TRACKING,
		XR_EXCESSIVE_MOTION,
		XR_INSUFFICIENT_FEATURES,
		XR_UNKNOWN_TRACKING,
		XR_NOT_TRACKING
	};

	enum PlayAreaMode
	{						  /* defines the mode used by the XR interface for tracking */
		XR_PLAY_AREA_UNKNOWN, /* Area mode not set or not available */
		XR_PLAY_AREA_3DOF, /* Only support orientation tracking, no positional tracking, area will
							  center around player */
		XR_PLAY_AREA_SITTING,	/* Player is in seated position, limited positional tracking, fixed
								   guardian around player */
		XR_PLAY_AREA_ROOMSCALE, /* Player is free to move around, full positional tracking */
		XR_PLAY_AREA_STAGE,		/* Same as roomscale but origin point is fixed to the center of the
								   physical space */
		XR_PLAY_AREA_CUSTOM = 0x7FFFFFFF, /* Used to denote that a custom, possibly non-standard,
											 play area is being used */
	};

	enum EnvironmentBlendMode
	{
		XR_ENV_BLEND_MODE_OPAQUE,	   /* You cannot see the real world, VR like */
		XR_ENV_BLEND_MODE_ADDITIVE,	   /* You can see the real world, AR like */
		XR_ENV_BLEND_MODE_ALPHA_BLEND, /* Real world is passed through where alpha channel is 0.0
										  and gradually blends to opaque for value 1.0. */
	};

	enum VRSTextureFormat
	{
		XR_VRS_TEXTURE_FORMAT_UNIFIED,
		XR_VRS_TEXTURE_FORMAT_FRAGMENT_SHADING_RATE,
		XR_VRS_TEXTURE_FORMAT_FRAGMENT_DENSITY_MAP,
	};

protected:
	_THREAD_SAFE_CLASS_

public:
	virtual PackedStringArray get_suggested_tracker_names() const; /* return a list of likely/suggested tracker names */
	virtual PackedStringArray get_suggested_pose_names(const String& p_tracker_name)
		const; /* return a list of likely/suggested action names for this tracker */
	virtual TrackingStatus get_tracking_status() const; /* get the status of our current tracking */
	virtual void trigger_haptic_pulse(const String& p_action_name, const String& p_tracker_name,
		double p_frequency, double p_amplitude, double p_duration_sec,
		double p_delay_sec = 0); /* trigger a haptic pulse */

	/** general interface information **/
	virtual String get_name() const;
	virtual uint32_t get_capabilities() const;

	bool is_primary();
	void set_primary(bool p_is_primary);

	virtual bool is_initialized() const; /* returns true if we've initialized this interface */
	virtual bool initialize();	 /* initialize this interface, if this has an HMD it becomes the
										primary interface */
	virtual void uninitialize(); /* deinitialize this interface */

	/** specific to VR **/
	virtual bool supports_play_area_mode(XRInterface::PlayAreaMode
			p_mode); /* query if this interface supports this play area mode */
	virtual XRInterface::PlayAreaMode get_play_area_mode() const; /* get the current play area mode */
	virtual bool set_play_area_mode(
		XRInterface::PlayAreaMode p_mode); /* change the play area mode, note that this should
											  return false if the mode is not available */
	virtual PackedVector3Array get_play_area() const; /* if available, returns an array of vectors denoting the play area the
							  player can move around in */

	/** specific to AR **/
	virtual bool get_anchor_detection_is_enabled() const;
	virtual void set_anchor_detection_is_enabled(bool p_enable);
	virtual int get_camera_feed_id();

	/** rendering and internal **/

	// These methods are called from the main thread.
	virtual Transform3D get_camera_transform(); /* returns the position of our camera, only used for updating
								   reference frame. For monoscopic this is equal to the views
								   transform, for stereoscopic this should be an average */
	virtual void process();

	// These methods can be called from both main and render thread.
	virtual Size2 get_render_target_size(); /* returns the recommended render target size per
												   eye for this device */
	virtual uint32_t get_view_count();		/* returns the view count we need (1 is monoscopic, 2 is
												   stereoscopic but can be more) */

	// These methods are called from the rendering thread.
	virtual Transform3D get_transform_for_view(
		uint32_t p_view, const Transform3D& p_cam_transform); /* get each views transform */
	virtual Projection get_projection_for_view(uint32_t p_view, double p_aspect, double p_z_near,
		double p_z_far);			 /* get each view projection matrix */
	virtual RID get_color_texture(); /* obtain color output texture (if applicable) */
	virtual RID get_depth_texture(); /* obtain depth output texture (if applicable, used for reprojection) */
	virtual RID get_velocity_texture(); /* obtain velocity output texture (if applicable, used for spacewarp) */
	virtual RID get_velocity_depth_texture();
	virtual Size2i get_velocity_target_size();
	virtual Rect2i get_render_region();

	virtual void pre_render() {}

	virtual bool pre_draw_viewport(RID p_render_target)
	{
		return true;
	} /* inform XR interface we are about to start our viewport draw process */

	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(RID p_render_target,
		const Rect2& p_screen_rect); /* inform XR interface we finished our viewport draw process */

	virtual void end_frame() {}

	/** passthrough **/

	virtual bool is_passthrough_supported() { return false; }

	virtual bool is_passthrough_enabled() { return false; }

	virtual bool start_passthrough() { return false; }

	virtual void stop_passthrough() {}

	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const
	{
		return XR_ENV_BLEND_MODE_OPAQUE;
	}

	virtual bool set_environment_blend_mode(EnvironmentBlendMode mode) { return false; }

	/** VRS **/
	virtual RID get_vrs_texture(); /* obtain VRS texture */

	virtual VRSTextureFormat get_vrs_texture_format() { return XR_VRS_TEXTURE_FORMAT_UNIFIED; }

	XRInterface() = default;
	~XRInterface() = default;
};

/* Helper class for generating stereoscopic VRS images */

class XRVRS final
{
private:
	float vrs_min_radius = 20.0f;
	float vrs_strength = 1.0f;
	Rect2i vrs_render_region;
	bool vrs_dirty = true;

	RID vrs_texture;
	Size2i target_size;
	Vector<Vector2> eye_foci;

public:
	~XRVRS();

	float get_vrs_min_radius() const;
	void set_vrs_min_radius(float p_vrs_min_radius);
	float get_vrs_strength() const;
	void set_vrs_strength(float p_vrs_strength);
	Rect2i get_vrs_render_region() const;
	void set_vrs_render_region(const Rect2i& p_vrs_render_region);

	RID make_vrs_texture(const Size2& p_target_size, const Vector<Vector2>& p_eye_foci);
};
// --- End servers/xr/xr_interface.h ---

// --- Folded from servers/camera/camera_server.h ---
#include "core/os/os.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"

/**
	The camera server is a singleton object that gives access to the various
	camera feeds that can be used as the background for our environment.
**/

class CameraFeed;
template <typename T> class TypedArray;

class CameraServer final
{
	static inline Mutex _thread_safe_;

public:
	enum FeedImage
	{
		FEED_RGBA_IMAGE = 0,
		FEED_YCBCR_IMAGE = 0,
		FEED_Y_IMAGE = 0,
		FEED_CBCR_IMAGE = 1,
		FEED_IMAGES = 2
	};

	static inline constexpr const char feeds_updated_signal_name[] = "camera_feeds_updated";

	struct Data
	{
		bool monitoring_feeds = false;
		Vector<Ref<CameraFeed>> feeds;
	};

	static inline Data* data = nullptr;

private:
	static void _default_initialize_backend() {}
	static void _default_finalize_backend() {}
	static void _default_set_monitoring_feeds(bool p_monitoring_feeds);
	static void _default_handle_application_pause() {}
	static void _default_handle_application_resume() {}
	static void _default_handle_display_rotation_change(int p_orientation) { (void)p_orientation; }

	inline static void (*_initialize_backend)() = &_default_initialize_backend;
	inline static void (*_finalize_backend)() = &_default_finalize_backend;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void set_monitoring_feeds_state(bool p_monitoring_feeds);
	_FORCE_INLINE_ static bool is_monitoring_feeds()
	{
		return data ? data->monitoring_feeds : false;
	}

	inline static void (*set_monitoring_feeds)(
		bool p_monitoring_feeds) = &_default_set_monitoring_feeds;
	inline static void (*handle_application_pause)() = &_default_handle_application_pause;
	inline static void (*handle_application_resume)() = &_default_handle_application_resume;
	inline static void (*handle_display_rotation_change)(
		int p_orientation) = &_default_handle_display_rotation_change;

	template <typename Impl> static void bind()
	{
		_initialize_backend = &Impl::initialize;
		_finalize_backend = &Impl::finalize;
		set_monitoring_feeds = &Impl::set_monitoring_feeds;
		handle_application_pause = &Impl::handle_application_pause;
		handle_application_resume = &Impl::handle_application_resume;
		handle_display_rotation_change = &Impl::handle_display_rotation_change;
	}

	static void reset_backend()
	{
		_initialize_backend = &_default_initialize_backend;
		_finalize_backend = &_default_finalize_backend;
		set_monitoring_feeds = &_default_set_monitoring_feeds;
		handle_application_pause = &_default_handle_application_pause;
		handle_application_resume = &_default_handle_application_resume;
		handle_display_rotation_change = &_default_handle_display_rotation_change;
	}

	// Right now we identify our feed by it's ID when it's used in the background.
	// May see if we can change this to purely relying on CameraFeed objects or by name.
	static int get_free_id();
	static int get_feed_index(int p_id);
	static Ref<CameraFeed> get_feed_by_id(int p_id);

	// Add and remove feeds.
	static void add_feed(const Ref<CameraFeed>& p_feed);
	static void remove_feed(const Ref<CameraFeed>& p_feed);

	// Get our feeds.
	static Ref<CameraFeed> get_feed(int p_index);
	static int get_feed_count();

	// Intended for use with custom CameraServer implementation.
	static RID feed_texture(int p_id, FeedImage p_texture);

	CameraServer() = delete;
	CameraServer(const CameraServer&) = delete;
	CameraServer& operator=(const CameraServer&) = delete;
	~CameraServer() = delete;
};

// --- Folded from servers/camera/camera_feed.h ---
#include "core/io/image.h"
#include "core/types.h"

/**
	The camera server is a singleton object that gives access to the various
	camera feeds that can be used as the background for our environment.
**/

class CameraFeed : public RefCounted
{
public:
	enum FeedDataType
	{
		FEED_NOIMAGE, // we don't have an image yet
		FEED_RGB,	  // our texture will contain a normal RGB texture that can be used directly
		FEED_YCBCR,	  // our texture will contain a YCbCr texture that needs to be converted to RGB
					  // before output
		FEED_YCBCR_SEP, // our camera is split into two textures, first plane contains Y data,
						// second plane contains CbCr data
		FEED_EXTERNAL,	// specific for android atm, camera feed is managed externally, assumed RGB
						// for now
	};

	enum FeedPosition
	{
		FEED_UNSPECIFIED, // we have no idea
		FEED_FRONT,		  // this is a camera on the front of the device
		FEED_BACK		  // this is a camera on the back of the device
	};

private:
	int id; // unique id for this, for internal use in case feeds are removed
	const String format_changed_signal_name = "format_changed";
	const String frame_changed_signal_name = "frame_changed";

protected:
	struct FeedFormat
	{
		int width = 0;
		int height = 0;
		String format;
		int frame_numerator = 0;
		int frame_denominator = 0;
		uint32_t pixel_format = 0;
	};

	String name;		   // name of our camera feed
	FeedDataType datatype; // type of texture data stored
	FeedPosition position; // position of camera on the device
	Transform2D transform; // display transform
	int base_width = 0;
	int base_height = 0;
	Vector<FeedFormat> formats;
	int selected_format = -1;

	bool active; // only when active do we actually update the camera texture each frame
	RID texture[CameraServer::FEED_IMAGES]; // texture images needed for this


public:
	int get_id() const;
	bool is_active() const;
	void set_active(bool p_is_active);

	String get_name() const;
	void set_name(String p_name);

	int get_base_width() const;
	int get_base_height() const;

	FeedPosition get_position() const;
	void set_position(FeedPosition p_position);

	Transform2D get_transform() const;
	void set_transform(const Transform2D& p_transform);

	RID get_texture(CameraServer::FeedImage p_which);
	uint64_t get_texture_tex_id(CameraServer::FeedImage p_which);

	CameraFeed();
	CameraFeed(String p_name, FeedPosition p_position = CameraFeed::FEED_UNSPECIFIED);
	virtual ~CameraFeed();

	FeedDataType get_datatype() const;
	void set_rgb_image(const Ref<Image>& p_rgb_img);
	void set_ycbcr_image(const Ref<Image>& p_ycbcr_img);
	void set_ycbcr_images(const Ref<Image>& p_y_img, const Ref<Image>& p_cbcr_img);
	void set_external(int p_width, int p_height);

	virtual FeedFormat get_format() const;

	virtual bool activate_feed();
	virtual void deactivate_feed();
};

// --- Folded from modules/camera/camera_apple.h ---
#if defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
///@TODO this is a near duplicate of CameraIOS, we should find a way to combine those to minimize code duplication!!!!
// If you fix something here, make sure you fix it there as well!



class CameraApple final
{
public:
	static void initialize() {}
	static void finalize();

	static void update_feeds();
	static void set_monitoring_feeds(bool p_monitoring_feeds);
	static void handle_display_rotation_change(int p_orientation);
	static void handle_application_pause();
	static void handle_application_resume();

	CameraApple() = delete;
	CameraApple(const CameraApple&) = delete;
	CameraApple& operator=(const CameraApple&) = delete;
	~CameraApple() = delete;
};
#endif // defined(MACOS_ENABLED) || defined(APPLE_EMBEDDED_ENABLED)
// --- End modules/camera/camera_apple.h ---
// --- End servers/camera/camera_server.h ---
// --- End servers/xr/xr_server.h ---

class InstanceUniforms final
{
public:
	void free(RID p_self);

	void materials_start();

	inline int32_t location() const { return _location; }

	inline bool is_allocated() const { return _location != -1; }

private:
	struct Item
	{
		int32_t index = -1;
		int32_t flags = 0;
		inline bool is_valid() const { return index != -1; }
	};

	int32_t _location = -1;
	HashMap<String, Item> _parameters;

	void _init_param(
		Item& r_item, const InstanceShaderParam& p_param) const;
	void _invalidate_items();
};

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

	struct InstanceBaseData;

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
			InstanceBaseData* visibility_notifier = nullptr;
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

		// Shared fields across instance base types
		Instance* owner = nullptr;
		RID instance;
		uint32_t cull_mask = 0xFFFFFFFF;
		HashSet<Instance*> geometries;
		HashSet<Instance*> lights;

		// Geometry data
		RenderGeometryInstance* geometry_instance = nullptr;
		bool can_cast_shadows = true;
		bool material_is_animated = true;
		uint32_t projector_count = 0;
		uint32_t softshadow_count = 0;
		HashSet<Instance*> decals;
		HashSet<Instance*> reflection_probes;
		HashSet<Instance*> voxel_gi_instances;
		HashSet<Instance*> lightmap_captures;

		// Reflection probe data
		SelfList<InstanceBaseData> update_list;
		int render_step = -1;

		// Fog volume data
		bool is_global = false;

		// Visibility notifier data
		bool just_visible = false;
		uint64_t visible_in_frame = 0;
		RID base;
		SelfList<InstanceBaseData> list_element;

		// Light data
		uint64_t last_version = 0;
		List<Instance*>::Element* D = nullptr; // directional light in scenario
		bool uses_projector = false;
		bool uses_softshadow = false;
		Instance* baked_light = nullptr;
		RSE::LightBakeMode bake_mode = RSE::LIGHT_BAKE_DISABLED;
		uint32_t max_sdfgi_cascade = 2;

		// VoxelGI data
		HashSet<Instance*> dynamic_geometries;
		Vector<LightCache> light_cache;
		Vector<RID> light_instances;
		RID probe_instance;
		bool invalid = true;
		uint32_t base_version = 0;
		SelfList<InstanceBaseData> update_element;

		// Lightmap data
		HashSet<Instance*> users;

	private:
		// Instead of a single dirty flag, we maintain a count
		// so that we can detect lights that are being made dirty
		// each frame, and switch on tighter caster culling.
		int32_t shadow_dirty_count = 1;

		uint32_t light_update_frame_id = UINT32_MAX;
		bool light_intersects_multiple_cameras = false;
		uint32_t light_intersects_multiple_cameras_timeout_frame_id = UINT32_MAX;

	public:
		bool is_shadow_dirty() const { return shadow_dirty_count != 0; }

		void make_shadow_dirty() { shadow_dirty_count = light_intersects_multiple_cameras ? 1 : 2; }

		void detect_light_intersects_multiple_cameras(uint32_t p_frame_id)
		{
			if (p_frame_id == light_update_frame_id) {
				light_intersects_multiple_cameras = true;
				light_intersects_multiple_cameras_timeout_frame_id = p_frame_id + 60;
			}
			else {
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

		InstanceBaseData() : update_list(this), list_element(this), update_element(this) {}
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
			if (base_data) {
				memdelete(base_data);
			}
			if (custom_aabb) {
				memdelete(custom_aabb);
			}
		}
	};

	static void _instance_queue_update(
		Instance* p_instance, bool p_update_aabb, bool p_update_dependencies = false);


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
		SelfList<InstanceBaseData>::List reflection_probe_render_list;

		SpinLock visible_notifier_list_lock;
		SelfList<InstanceBaseData>::List visible_notifier_list;
		SelfList<InstanceBaseData>::List voxel_gi_update_list;

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

struct Projection;
struct Transform3D;

// For testing performance improvements from the LightCuller:
// Uncomment LIGHT_CULLER_DEBUG_FLASH and it will turn the culler
// on and off every LIGHT_CULLER_DEBUG_FLASH_FREQUENCY camera prepares.
// Uncomment LIGHT_CULLER_DEBUG_LOGGING to get periodic print of the number of casters culled before / after.
// Uncomment LIGHT_CULLER_DEBUG_DIRECTIONAL_LIGHT to get periodic print of the number of casters culled for the directional light..

// #define LIGHT_CULLER_DEBUG_LOGGING
// #define LIGHT_CULLER_DEBUG_DIRECTIONAL_LIGHT
// #define LIGHT_CULLER_DEBUG_REGULAR_LIGHT
// #define LIGHT_CULLER_DEBUG_FLASH
#define LIGHT_CULLER_DEBUG_FLASH_FREQUENCY 1024
////////////////////////////////////////////////////////////////////////////////////////////////

// The code to generate the lookup table is included but commented out.
// This may be useful for debugging / regenerating the LUT in the future,
// especially if the order of planes changes.
// When this define is set, the generated lookup table will be printed to debug output.
// The generated lookup table can be copy pasted
// straight to LUT_entry_sizes and LUT_entries.
// See the referenced article for explanation.
// #define RENDERING_LIGHT_CULLER_CALCULATE_LUT

////////////////////////////////////////////////////////////////////////////////////////////////
// This define will be set automatically depending on earlier defines, you can leave this as is.
#if defined(LIGHT_CULLER_DEBUG_LOGGING) || defined(RENDERING_LIGHT_CULLER_CALCULATE_LUT)
#define RENDERING_LIGHT_CULLER_DEBUG_STRINGS
#endif

// Culls shadow casters that can't cast shadows into the camera frustum.
class RenderingLightCuller final {
public:
	RenderingLightCuller();

private:
	class LightSource {
	public:
		enum SourceType {
			ST_UNKNOWN,
			ST_DIRECTIONAL,
			ST_SPOTLIGHT,
			ST_OMNI,
			ST_AREA,
		};

		LightSource() {
			type = ST_UNKNOWN;
			angle = 0.0f;
			range = FLT_MAX;
			cascade_count = 0;
			cascade_splits[0] = cascade_splits[1] = cascade_splits[2] = 0;
			blend_splits = false;
		}

		// All in world space, culling done in world space.
		Vector3 pos;
		Vector3 dir;
		SourceType type;

		float angle; // For spotlight.
		float range;
		float width; // For area light.
		float height;
		Vector2 area_size; // For area light.

		int cascade_count;
		float cascade_splits[3]; // Max 4 cascades, which only has 3 splits.
		bool blend_splits;
	};

	// Directional lights have separate cull frustums for each cascade, so this struct is needed to specify which one to use for each cull step.
	struct CullFrustumData {
		// Functions expect this to store a frustum, so it must be ALWAYS at least Plane[6]. Undefined behavior otherwise.
		const Plane *frustum_planes = nullptr;
		// Functions expect this to store frustum corners, so it must be ALWAYS at least Vector3[8]. UB otherwise.
		const Vector3 *frustum_points = nullptr;
	};

	// Same order as godot.
	enum PlaneOrder {
		PLANE_NEAR,
		PLANE_FAR,
		PLANE_LEFT,
		PLANE_TOP,
		PLANE_RIGHT,
		PLANE_BOTTOM,
		PLANE_TOTAL,
	};

	// Same order as godot.
	enum PointOrder {
		PT_FAR_LEFT_TOP,
		PT_FAR_LEFT_BOTTOM,
		PT_FAR_RIGHT_TOP,
		PT_FAR_RIGHT_BOTTOM,
		PT_NEAR_LEFT_TOP,
		PT_NEAR_LEFT_BOTTOM,
		PT_NEAR_RIGHT_TOP,
		PT_NEAR_RIGHT_BOTTOM,
	};

	// 6 bits, 6 planes.
	enum {
		NUM_CAM_PLANES = 6,
		NUM_CAM_POINTS = 8,
		MAX_CULL_PLANES = 17,
		LUT_SIZE = 64,
	};

public:
	// Before each pass with a different camera, you must call this so the culler can pre-create
	// the camera frustum planes and corner points in world space which are used for the culling.
	bool prepare_camera(const Transform3D &p_cam_transform, const Projection &p_cam_matrix);

	// REGULAR LIGHTS (SPOT, OMNI).
	// These are prepared then used for culling one by one, single threaded.
	// prepare_regular_light() returns false if the entire light is culled (i.e. there is no intersection between the light and the view frustum).
	bool prepare_regular_light(const RendererSceneCull::Instance &p_instance) { return _prepare_light(p_instance, -1); }

	// Cull according to the regular light planes that were setup in the previous call to prepare_regular_light.
	void cull_regular_light(PagedArray<RendererSceneCull::Instance *> &r_instance_shadow_cull_result);

	// Directional lights are prepared in advance, and can be culled multithreaded chopping and changing between
	// different directional_light_id.
	void prepare_directional_light(const RendererSceneCull::Instance *p_instance, int32_t p_directional_light_id);

	// Return false if the instance is to be culled.
	bool cull_directional_light(const RendererSceneCull::InstanceBounds &p_bound, int32_t p_directional_light_id, int32_t p_cascade);

	// Can turn on and off from the engine if desired.
	void set_caster_culling_active(bool p_active) { data.caster_culling_active = p_active; }
	void set_light_culling_active(bool p_active) { data.light_culling_active = p_active; }

private:
	struct LightCullPlanes {
		void add_cull_plane(const Plane &p);
		Plane cull_planes[MAX_CULL_PLANES];
		int num_cull_planes = 0;
#ifdef LIGHT_CULLER_DEBUG_DIRECTIONAL_LIGHT
		uint32_t rejected_count = 0;
#endif
	};

	struct DirectionalCullPlanes {
		LightCullPlanes planes[4]; // One set of cull planes per cascade
	};

	bool _prepare_light(const RendererSceneCull::Instance &p_instance, int32_t p_directional_light_id = -1);

	// Avoid adding extra culling planes derived from near colinear triangles.
	// The normals derived from these will be inaccurate, and can lead to false
	// culling of objects that should be within the light volume.
	// See:
	// - issue GH-89702 "Tighter Shadow Caster Culling causes some object shadows to not render for a Frame"
	// - issue GH-89560 "Directional Shadows disappear with large Camera Z Far values at some angles"
	// - issue GH-91976 "SpotLight3D shadows exhibit flickering when moved around."
	// - PR GH-92078 which gave it the current value.
	bool _is_colinear_tri(const Vector3 &p_a, const Vector3 &p_b, const Vector3 &p_c) const {
		// Lengths of sides a, b and c.
		float la = (p_b - p_a).length();
		float lb = (p_c - p_b).length();
		float lc = (p_c - p_a).length();

		// Get longest side into lc.
		if (lb < la) {
			SWAP(la, lb);
		}
		if (lc < lb) {
			SWAP(lb, lc);
		}

		// Prevent divide by zero.
		if (lc > 0.001f) {
			// If the summed length of the smaller two
			// sides is close to the length of the longest side,
			// the points are colinear, and the triangle is near degenerate.
			float ld = ((la + lb) - lc) / lc;

			// ld will be close to zero for colinear tris.

			// Long frustums are made out of significantly stretched-out triangles,
			// so large threshold will produce large amounts of cullable but unculled meshes.
			// For example: 0.001 fails to cull cullable meshes with camera FOV of 70 and ortho shadows at ~50m at certain view angles.
			// 0.0001 fails to cull cullable meshes with camera FOV of 70 and ortho shadows at ~500m at certain view angles.
			// ...These apply less to cascades since they have large near planes in comparison to far planes, unlike ortho lights.
			// If you're reading this and the value for directional lights is still 0.001f,
			// that is fine as is and it only means GH-115176 didn't make it.
			return ld < 0.001f;
		}

		// Don't create planes from tiny triangles,
		// they won't be accurate.
		return true;
	}

	// Internal version uses LightSource.
	bool _add_light_camera_planes(LightCullPlanes &r_cull_planes, const LightSource &p_light_source, const CullFrustumData &p_cull_frustum);

	// Directional light gives parallel culling planes (as opposed to point lights).
	bool add_light_camera_planes_directional(LightCullPlanes &r_cull_planes, const LightSource &p_light_source, const CullFrustumData &p_cull_frustum);

	// Is the light culler active? maybe not in the editor...
	bool is_caster_culling_active() const { return data.caster_culling_active; }
	bool is_light_culling_active() const { return data.light_culling_active; }

	// Ensure result is at least a Plane[6] for the frustum input and Vector3[8] to store result before calling. If not, undefined behavior.
	bool create_frustum_points(const Plane *p_frustum_planes, Vector3 *r_result) const;

	// Do we want to log some debug output?
	bool is_logging() const { return data.debug_count == 0; }

	struct Data {
		// Camera frustum planes (world space) - order ePlane.
		Vector<Plane> frustum_planes;

		// Camera frustum corners (world space) - order ePoint.
		Vector3 frustum_points[NUM_CAM_POINTS];

		// Master can have multiple directional lights.
		// These need to store their own cull planes individually, as master
		// chops and changes between culling different lights
		// instead of doing one by one, and we don't want to prepare
		// lights multiple times per frame.
		LocalVector<DirectionalCullPlanes> directional_cull_planes;

		Transform3D camera_transform;
		Projection camera_projection;

		// Single threaded cull planes for regular lights
		// (OMNI, SPOT). These lights reuse the same set of cull plane data.
		LightCullPlanes regular_cull_planes;

#ifdef LIGHT_CULLER_DEBUG_REGULAR_LIGHT
		uint32_t regular_rejected_count = 0;
#endif
		// The whole regular light can be out of range of the view frustum, in which case all casters should be culled.
		bool out_of_range = false;

#ifdef RENDERING_LIGHT_CULLER_DEBUG_STRINGS
		static String plane_bitfield_to_string(unsigned int BF);
		// Names of the plane and point enums, useful for debugging.
		static const char *string_planes[];
		static const char *string_points[];
#endif

		// Precalculated look up table.
		static uint8_t LUT_entry_sizes[LUT_SIZE];
		static uint8_t LUT_entries[LUT_SIZE][8];

		bool caster_culling_active = true;
		bool light_culling_active = true;

		// Light culling is a basic on / off switch.
		// Caster culling only works if light culling is also on.
		bool is_active() const { return light_culling_active; }

		// Ideally a frame counter, but for ease of implementation
		// this is just incremented on each prepare_camera.
		// used to turn on and off debugging features.
		int debug_count = -1;
	} data;

	// This functionality is not required in general use (and is compiled out),
	// as the lookup table can normally be hard coded
	// (provided order of planes etc does not change).
	// It is provided for debugging / future maintenance.
#ifdef RENDERING_LIGHT_CULLER_CALCULATE_LUT
	void get_neighbouring_planes(PlaneOrder p_plane, PlaneOrder r_neigh_planes[4]) const;
	void get_corners_of_planes(PlaneOrder p_plane_a, PlaneOrder p_plane_b, PointOrder r_points[2]) const;
	void create_LUT();
	void compact_LUT_entry(uint32_t p_entry_id);
	void debug_print_LUT();
	void debug_print_LUT_as_table();
	void add_LUT(int p_plane_0, int p_plane_1, PointOrder p_pts[2]);
	void add_LUT_entry(uint32_t p_entry_id, PointOrder p_pts[2]);
	String debug_string_LUT_entry(const LocalVector<uint8_t> &p_entry, bool p_pair = false);
	String string_LUT_entry(const LocalVector<uint8_t> &p_entry);

	// Contains a list of points for each combination of plane facing directions.
	LocalVector<uint8_t> _calculated_LUT[LUT_SIZE];
#endif
};
// --- Inlined/Folded from servers/rendering/renderer_canvas_render.h ---
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"

class Renderer;

typedef uint64_t PolygonID;

struct Polygon
{
	PolygonID polygon_id;
	Rect2 rect_cache;

	void create(const Vector<int>& p_indices, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), int p_count = -1);

	_FORCE_INLINE_ Polygon() { polygon_id = 0; }

	~Polygon();
};

namespace RCR // RendererCanvasRender
{

struct Item
{
	struct CommandBlock
	{
		enum
		{
			MAX_SIZE = 4096
		};

		uint32_t usage;
		uint8_t* memory = nullptr;
	};

	struct Command
	{
		enum Type
		{
			TYPE_RECT,
			TYPE_NINEPATCH,
			TYPE_POLYGON,
			TYPE_PRIMITIVE,
			TYPE_MESH,
			TYPE_MULTIMESH,
			TYPE_PARTICLES,
			TYPE_TRANSFORM,
			TYPE_CLIP_IGNORE,
			TYPE_ANIMATION_SLICE,
		};

		Command* next = nullptr;
		Type type;
	};

	struct CommandRect
	{
		Command header{nullptr, Command::TYPE_RECT};
		Rect2 rect;
		Color modulate;
		Rect2 source;
		uint16_t flags = 0;
		float outline = 0;
		float px_range = 1;

		RID texture;
	};

	struct CommandNinePatch
	{
		Command header{nullptr, Command::TYPE_NINEPATCH};
		Rect2 rect;
		Rect2 source;
		float margin[4];
		bool draw_center = true;
		Color color;
		RSE::NinePatchAxisMode axis_x;
		RSE::NinePatchAxisMode axis_y;

		RID texture;
	};

	struct CommandPolygon
	{
		Command header{nullptr, Command::TYPE_POLYGON};
		RSE::PrimitiveType primitive;
		Polygon polygon;

		RID texture;
	};

	struct CommandPrimitive
	{
		Command header{nullptr, Command::TYPE_PRIMITIVE};
		uint32_t point_count;
		Vector2 points[4];
		Vector2 uvs[4];
		Color colors[4];

		RID texture;
	};

	struct CommandMesh
	{
		Command header{nullptr, Command::TYPE_MESH};
		RID mesh;
		Transform2D transform;
		Color modulate;
		RID mesh_instance;

		RID texture;

		~CommandMesh();
	};

	struct CommandMultiMesh
	{
		Command header{nullptr, Command::TYPE_MULTIMESH};
		RID multimesh;

		RID texture;
	};

	struct CommandParticles
	{
		Command header{nullptr, Command::TYPE_PARTICLES};
		RID particles;
		RID texture;
	};

	struct CommandTransform
	{
		Command header{nullptr, Command::TYPE_TRANSFORM};
		Transform2D xform;
	};

	struct CommandClipIgnore
	{
		Command header{nullptr, Command::TYPE_CLIP_IGNORE};
		bool ignore = false;
	};

	struct CommandAnimationSlice
	{
		Command header{nullptr, Command::TYPE_ANIMATION_SLICE};
		double animation_length = 0;
		double slice_begin = 0;
		double slice_end = 0;
		double offset = 0;
	};

	struct ViewportRender
	{
		Renderer* owner = nullptr;
		void* udata = nullptr;
		Rect2 rect;
	};

	Transform2D xform_curr;
	Transform2D xform_prev;

	bool clip : 1;
	bool visible : 1;
	bool behind : 1;
	bool update_when_visible : 1;
	bool on_interpolate_transform_list : 1;
	bool interpolated : 1;
	bool use_identity_transform : 1;

	struct CanvasGroup
	{
		RSE::CanvasGroupMode mode;
		bool fit_empty;
		float fit_margin;
		bool blur_mipmaps;
		float clear_margin;
	};

	CanvasGroup* canvas_group = nullptr;
	bool use_canvas_group = false;
	int light_mask;
	int z_final;

	mutable bool custom_rect;
	mutable bool rect_dirty;
	mutable Rect2 rect;
	RID material;
	RID skeleton;

	int32_t instance_allocated_shader_uniforms_offset = -1;

	Item* next = nullptr;

	struct CopyBackBuffer
	{
		Rect2 rect;
		Rect2 screen_rect;
		bool full;
	};

	CopyBackBuffer* copy_back_buffer = nullptr;

	Color final_modulate;
	Transform2D final_transform;
	Rect2 final_clip_rect;
	Item* final_clip_owner = nullptr;
	Item* material_owner = nullptr;
	Item* canvas_group_owner = nullptr;
	ViewportRender* vp_render = nullptr;
	bool distance_field;
	bool light_masked;
	bool repeat_source;
	Point2 repeat_size;
	int repeat_times = 1;
	Item* repeat_source_item = nullptr;

	Rect2 global_rect_cache;

	const Rect2& get_rect() const;

	Command* commands = nullptr;
	Command* last_command = nullptr;
	Vector<CommandBlock> blocks;
	uint32_t current_block;
#ifdef DEBUG_ENABLED
	mutable double debug_redraw_time = 0;
#endif

	template <typename T> T* alloc_command()
	{
		static_assert(offsetof(T, header) == 0, "Command header must be at offset 0.");
		T* command = nullptr;
		if (commands == nullptr) {
			command = memnew_placement(memalloc(sizeof(T)), T);
			command->header.next = nullptr;
			commands = &command->header;
			last_command = &command->header;
		}
		else {
			while (true) {
				if (unlikely(current_block == (uint32_t)blocks.size())) {
					CommandBlock cb;
					cb.memory = (uint8_t*)memalloc(CommandBlock::MAX_SIZE);
					cb.usage = 0;
					blocks.push_back(cb);
				}

				CommandBlock* c = &blocks.write[current_block];
				size_t space_left = CommandBlock::MAX_SIZE - c->usage;
				if (space_left < sizeof(T)) {
					current_block++;
					continue;
				}

				void* memory = c->memory + c->usage;
				command = memnew_placement(memory, T);
				command->header.next = nullptr;
				last_command->next = &command->header;
				last_command = &command->header;
				c->usage += sizeof(T);
				break;
			}
		}

		rect_dirty = true;
		return command;
	}

	void clear()
	{
		static_assert(std::is_trivially_destructible_v<CommandRect>);
		static_assert(std::is_trivially_destructible_v<CommandNinePatch>);
		static_assert(std::is_trivially_destructible_v<CommandPrimitive>);
		static_assert(std::is_trivially_destructible_v<CommandMultiMesh>);
		static_assert(std::is_trivially_destructible_v<CommandParticles>);
		static_assert(std::is_trivially_destructible_v<CommandTransform>);
		static_assert(std::is_trivially_destructible_v<CommandClipIgnore>);
		static_assert(std::is_trivially_destructible_v<CommandAnimationSlice>);

		Command* c = commands;
		while (c) {
			Command* n = c->next;
			switch (c->type) {
			case Command::TYPE_POLYGON:
				reinterpret_cast<CommandPolygon*>(c)->~CommandPolygon();
				break;
			case Command::TYPE_MESH:
				reinterpret_cast<CommandMesh*>(c)->~CommandMesh();
				break;
			default:
				break;
			}
			if (c == commands) {
				memfree(commands);
				commands = nullptr;
			}
			c = n;
		}
		{
			uint32_t cbc = MIN((current_block + 1), (uint32_t)blocks.size());
			CommandBlock* blockptr = blocks.ptrw();
			for (uint32_t i = 0; i < cbc; i++) {
				blockptr[i].usage = 0;
			}
		}

		last_command = nullptr;
		commands = nullptr;
		current_block = 0;
		clip = false;
		rect_dirty = true;
		final_clip_owner = nullptr;
		material_owner = nullptr;
		light_masked = false;
	}

	RSE::CanvasItemTextureFilter texture_filter;
	RSE::CanvasItemTextureRepeat texture_repeat;

	Item()
	{
		commands = nullptr;
		last_command = nullptr;
		current_block = 0;
		light_mask = 1;
		vp_render = nullptr;
		next = nullptr;
		final_clip_owner = nullptr;
		canvas_group_owner = nullptr;
		clip = false;
		final_modulate = Color(1, 1, 1, 1);
		visible = true;
		rect_dirty = true;
		custom_rect = false;
		behind = false;
		material_owner = nullptr;
		copy_back_buffer = nullptr;
		distance_field = false;
		light_masked = false;
		update_when_visible = false;
		z_final = 0;
		texture_filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
		texture_repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
		repeat_source = false;
		on_interpolate_transform_list = false;
		interpolated = true;
		use_identity_transform = false;
	}

	~Item()
	{
		clear();
		for (int i = 0; i < blocks.size(); i++) {
			memfree(blocks[i].memory);
		}
		memdelete(copy_back_buffer);
	}
};

}; // namespace RCR

class RendererCanvasRender
{
public:
	RendererCanvasRender() = delete;
	RendererCanvasRender(const RendererCanvasRender&) = delete;
	~RendererCanvasRender() = delete;

	enum CanvasRectFlags
	{
		CANVAS_RECT_REGION = 1,
		CANVAS_RECT_TILE = 2,
		CANVAS_RECT_FLIP_H = 4,
		CANVAS_RECT_FLIP_V = 8,
		CANVAS_RECT_TRANSPOSE = 16,
		CANVAS_RECT_CLIP_UV = 32,
		CANVAS_RECT_IS_GROUP = 64,
		CANVAS_RECT_MSDF = 128,
		CANVAS_RECT_LCD = 256,
	};

	struct Light
	{
		bool enabled : 1;
		bool on_interpolate_transform_list : 1;
		bool interpolated : 1;
		Color color;
		Transform2D xform_curr;
		Transform2D xform_prev;
		float height;
		float energy;
		float scale;
		int z_min;
		int z_max;
		int layer_min;
		int layer_max;
		int item_mask;
		int item_shadow_mask;
		float directional_distance;
		RSE::CanvasLightMode mode;
		RSE::CanvasLightBlendMode blend_mode;
		RID texture;
		Vector2 texture_offset;
		RID canvas;
		bool use_shadow;
		int shadow_buffer_size;
		RSE::CanvasLightShadowFilter shadow_filter;
		Color shadow_color;
		float shadow_smooth;

		Rect2 rect_cache;
		Transform2D xform_cache;
		float radius_cache;

		Transform2D light_shader_xform;

		Light* shadows_next_ptr = nullptr;
		Light* filter_next_ptr = nullptr;
		Light* next_ptr = nullptr;
		Light* directional_next_ptr = nullptr;

		RID light_internal;
		uint64_t version;

		int32_t render_index_cache;

		Light()
		{
			version = 0;
			enabled = true;
			on_interpolate_transform_list = false;
			interpolated = true;
			color = Color(1, 1, 1);
			shadow_color = Color(0, 0, 0, 0);
			height = 0;
			z_min = -1024;
			z_max = 1024;
			layer_min = 0;
			layer_max = 0;
			item_mask = 1;
			scale = 1.0;
			energy = 1.0;
			item_shadow_mask = 1;
			mode = RSE::CANVAS_LIGHT_MODE_POINT;
			blend_mode = RSE::CANVAS_LIGHT_BLEND_MODE_ADD;
			next_ptr = nullptr;
			directional_next_ptr = nullptr;
			filter_next_ptr = nullptr;
			use_shadow = false;
			shadow_buffer_size = 2048;
			shadow_filter = RSE::CANVAS_LIGHT_FILTER_NONE;
			shadow_smooth = 0.0;
			render_index_cache = -1;
			directional_distance = 10000.0;
		}
	};

	static inline PolygonID (*request_polygon)(const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors, const Vector<Point2>& p_uvs,
		const Vector<int>& p_bones, const Vector<float>& p_weights, int p_count) = nullptr;
	static inline void (*free_polygon)(PolygonID p_polygon) = nullptr;

	struct LightOccluderInstance
	{
		bool enabled : 1;
		bool on_interpolate_transform_list : 1;
		bool sdf_collision = false;
		bool interpolated : 1;
		RID canvas;
		RID polygon;
		RID occluder;
		Rect2 aabb_cache;
		Transform2D xform_curr;
		Transform2D xform_prev;
		Transform2D xform_cache;
		int light_mask;
		RSE::CanvasOccluderPolygonCullMode cull_cache;

		LightOccluderInstance* next = nullptr;

		LightOccluderInstance()
		{
			enabled = true;
			on_interpolate_transform_list = false;
			interpolated = false;
			sdf_collision = false;
			next = nullptr;
			light_mask = 1;
			cull_cache = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;
		}
	};

	// Lifecycle dispatch
	static inline void (*initialize)() = nullptr;
	static inline void (*finalize)() = nullptr;

	// Draw and update dispatch
	static inline void (*canvas_render_items)(RID p_to_render_target, RCR::Item* p_item_list,
		const Color& p_modulate, Light* p_light_list, Light* p_directional_list,
		const Transform2D& p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info) = nullptr;

	static inline void (*set_time)(double p_time) = nullptr;
	static inline void (*update)() = nullptr;

	// Lighting and occluder dispatch
	static inline RID (*light_create)() = nullptr;
	static inline void (*light_set_texture)(RID p_rid, RID p_texture) = nullptr;
	static inline void (*light_set_use_shadow)(RID p_rid, bool p_enable) = nullptr;
	static inline void (*light_update_shadow)(RID p_rid, int p_shadow_index,
		const Transform2D& p_light_xform, int p_light_mask, float p_near, float p_far,
		LightOccluderInstance* p_occluders, const Rect2& p_light_rect) = nullptr;
	static inline void (*light_update_directional_shadow)(RID p_rid, int p_shadow_index,
		const Transform2D& p_light_xform, int p_light_mask, float p_cull_distance,
		const Rect2& p_clip_rect, LightOccluderInstance* p_occluders) = nullptr;

	static inline void (*render_sdf)(
		RID p_render_target, LightOccluderInstance* p_occluders) = nullptr;

	static inline RID (*occluder_polygon_create)() = nullptr;
	static inline void (*occluder_polygon_set_shape)(
		RID p_occluder, const Vector<Vector2>& p_points, bool p_closed) = nullptr;
	static inline void (*occluder_polygon_set_cull_mode)(
		RID p_occluder, RSE::CanvasOccluderPolygonCullMode p_mode) = nullptr;
	static inline void (*set_shadow_texture_size)(int p_size) = nullptr;

	// Resource cleanup and diagnostics
	static inline bool (*free)(RID p_rid) = nullptr;
	static inline void (*set_debug_redraw)(
		bool p_enabled, double p_time, const Color& p_color) = nullptr;
	static inline uint32_t (*get_pipeline_compilations)(RSE::PipelineSource p_source) = nullptr;

    template <typename T>
    static void bind()
    {
        initialize = &T::initialize;
        finalize = &T::finalize;

        request_polygon = &T::request_polygon;
        free_polygon = &T::free_polygon;

        canvas_render_items = &T::canvas_render_items;
        set_time = &T::set_time;
        update = &T::update;
        free = &T::free;
        set_debug_redraw = &T::set_debug_redraw;
        get_pipeline_compilations = &T::get_pipeline_compilations;

        light_create = &T::light_create;
        light_set_texture = &T::light_set_texture;
        light_set_use_shadow = &T::light_set_use_shadow;
        light_update_shadow = &T::light_update_shadow;
        light_update_directional_shadow = &T::light_update_directional_shadow;

        render_sdf = &T::render_sdf;

        occluder_polygon_create = &T::occluder_polygon_create;
        occluder_polygon_set_shape = &T::occluder_polygon_set_shape;
        occluder_polygon_set_cull_mode = &T::occluder_polygon_set_cull_mode;
        set_shadow_texture_size = &T::set_shadow_texture_size;
    }
};

#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"

class RendererCanvasCull
{
	static void _dependency_changed(
		Dependency::DependencyChangedNotification p_notification, DependencyTracker* p_tracker);
	static void _dependency_deleted(const RID& p_dependency, DependencyTracker* p_tracker);

public:
	struct Item : public RCR::Item
	{
		RID parent; // canvas it belongs to
		RID self;
		List<Item*>::Element* E;
		int z_index;
		bool z_relative;
		bool sort_y;
		Color modulate;
		Color self_modulate;
		bool use_parent_material;
		int index;
		bool children_order_dirty;
		int ysort_children_count;
		Color ysort_modulate;
		Transform2D ysort_xform; // Relative to y-sorted subtree's root item (identity for such
								 // root). Its `origin.y` is used for sorting.
		int ysort_index;
		int ysort_parent_abs_z_index; // Absolute Z index of parent. Only populated and used when
									  // y-sorting.
		uint32_t visibility_layer = 0xffffffff;

		Vector<Item*> child_items;

		struct VisibilityNotifierData
		{
			Rect2 area;
			bool just_visible = false;
			uint64_t visible_in_frame = 0;
			SelfList<VisibilityNotifierData> visible_element;

			VisibilityNotifierData() : visible_element(this) {}
		};

		VisibilityNotifierData* visibility_notifier = nullptr;

		DependencyTracker dependency_tracker;
		InstanceUniforms instance_uniforms;
		SelfList<Item> update_item;

		bool update_dependencies = false;

		Item() : update_item(this)
		{
			children_order_dirty = true;
			E = nullptr;
			z_index = 0;
			modulate = Color(1, 1, 1, 1);
			self_modulate = Color(1, 1, 1, 1);
			sort_y = false;
			use_parent_material = false;
			z_relative = true;
			index = 0;
			ysort_children_count = -1;
			ysort_xform = Transform2D();
			ysort_index = 0;
			ysort_parent_abs_z_index = 0;

			dependency_tracker.userdata = this;
			dependency_tracker.changed_callback = &RendererCanvasCull::_dependency_changed;
			dependency_tracker.deleted_callback = &RendererCanvasCull::_dependency_deleted;
		}
	};

	void _item_queue_update(Item* p_item, bool p_update_dependencies);
	SelfList<Item>::List _item_update_list;

	struct ItemIndexSort
	{
		_FORCE_INLINE_ bool operator()(const Item* p_left, const Item* p_right) const
		{
			return p_left->index < p_right->index;
		}
	};

	struct ItemYSort
	{
		_FORCE_INLINE_ bool operator()(const Item* p_left, const Item* p_right) const
		{
			const real_t left_y = p_left->ysort_xform.columns[2].y;
			const real_t right_y = p_right->ysort_xform.columns[2].y;
			if (Math::is_equal_approx(left_y, right_y)) {
				return p_left->ysort_index < p_right->ysort_index;
			}

			return left_y < right_y;
		}
	};

	struct LightOccluderPolygon
	{
		bool active;
		Rect2 aabb;
		RSE::CanvasOccluderPolygonCullMode cull_mode;
		RID occluder;
		HashSet<RendererCanvasRender::LightOccluderInstance*> owners;

		LightOccluderPolygon()
		{
			active = false;
			cull_mode = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;
		}
	};

	RID_Owner<LightOccluderPolygon, true> canvas_light_occluder_polygon_owner;

	RID_Owner<RendererCanvasRender::LightOccluderInstance, true> canvas_light_occluder_owner;

	struct Canvas : public RendererViewport::CanvasBase
	{
		HashSet<RID> viewports;

		struct ChildItem
		{
			Item* item = nullptr;

			bool operator<(const ChildItem& p_item) const
			{
				return item->index < p_item.item->index;
			}
		};

		HashSet<RendererCanvasRender::Light*> lights;
		HashSet<RendererCanvasRender::Light*> directional_lights;

		HashSet<RendererCanvasRender::LightOccluderInstance*> occluders;

		bool children_order_dirty;
		Vector<ChildItem> child_items;
		Color modulate;
		RID parent;
		float parent_scale;

		int find_item(Item* p_item)
		{
			for (int i = 0; i < child_items.size(); i++) {
				if (child_items[i].item == p_item) {
					return i;
				}
			}
			return -1;
		}

		void erase_item(Item* p_item)
		{
			int idx = find_item(p_item);
			if (idx >= 0) {
				child_items.remove_at(idx);
			}
		}

		Canvas()
		{
			modulate = Color(1, 1, 1, 1);
			children_order_dirty = true;
			parent_scale = 1.0;
		}
	};

	mutable RID_Owner<Canvas, true> canvas_owner;
	RID_Owner<Item, true> canvas_item_owner{65536, 4194304};
	RID_Owner<RendererCanvasRender::Light, true> canvas_light_owner;

	bool disable_scale;
	bool sdf_used = false;
	bool snapping_2d_transforms_to_pixel = false;

	bool debug_redraw = false;
	double debug_redraw_time = 0;
	Color debug_redraw_color;

	PagedAllocator<Item::VisibilityNotifierData> visibility_notifier_allocator;
	SelfList<Item::VisibilityNotifierData>::List visibility_notifier_list;

	_FORCE_INLINE_ void _attach_canvas_item_for_draw(Item* ci, Item* p_canvas_clip,
		RCR::Item** r_z_list, RCR::Item** r_z_last_list,
		const Transform2D& p_transform, const Rect2& p_clip_rect, Rect2 p_global_rect,
		const Color& modulate, int p_z, RendererCanvasCull::Item* p_material_owner,
		bool p_use_canvas_group, RCR::Item* r_canvas_group_from);

private:
	void _render_canvas_item_tree(RID p_to_render_target, Canvas::ChildItem* p_child_items,
		int p_child_item_count, const Transform2D& p_transform, const Rect2& p_clip_rect,
		const Color& p_modulate, RendererCanvasRender::Light* p_lights,
		RendererCanvasRender::Light* p_directional_lights,
		RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		uint32_t p_canvas_cull_mask, RenderingServerTypes::RenderInfo* r_render_info = nullptr);
	void _cull_canvas_item(Item* p_canvas_item, const Transform2D& p_parent_xform,
		const Rect2& p_clip_rect, const Color& p_modulate, int p_z,
		RCR::Item** r_z_list, RCR::Item** r_z_last_list,
		Item* p_canvas_clip, Item* p_material_owner, bool p_is_already_y_sorted,
		uint32_t p_canvas_cull_mask, const Point2& p_repeat_size, int p_repeat_times,
		RCR::Item* p_repeat_source_item);

	void _collect_ysort_children(RendererCanvasCull::Item* p_canvas_item,
		RendererCanvasCull::Item* p_material_owner, const Color& p_modulate,
		RendererCanvasCull::Item** r_items, int& r_index, int& r_ysort_children_count, int p_z,
		uint32_t p_canvas_cull_mask);
	int _count_ysort_children(RendererCanvasCull::Item* p_canvas_item);
	void _mark_ysort_dirty(RendererCanvasCull::Item* ysort_owner);

	static constexpr int z_range = RSE::CANVAS_ITEM_Z_MAX - RSE::CANVAS_ITEM_Z_MIN + 1;

	RCR::Item** z_list = nullptr;
	RCR::Item** z_last_list = nullptr;

	Transform2D _current_camera_transform;

public:
	void render_canvas(RID p_render_target, Canvas* p_canvas, const Transform2D& p_transform,
		RendererCanvasRender::Light* p_lights, RendererCanvasRender::Light* p_directional_lights,
		const Rect2& p_clip_rect, RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_transforms_to_pixel,
		bool p_snap_2d_vertices_to_pixel, uint32_t p_canvas_cull_mask,
		RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	bool was_sdf_used();

	RID canvas_allocate();
	void canvas_initialize(RID p_rid);

	void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2& p_mirroring);
	void canvas_set_item_repeat(RID p_item, const Point2& p_repeat_size, int p_repeat_times);
	void canvas_set_modulate(RID p_canvas, const Color& p_color);
	void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale);
	void canvas_set_disable_scale(bool p_disable);

	RID canvas_item_allocate();
	void canvas_item_initialize(RID p_rid);

	void canvas_item_set_parent(RID p_item, RID p_parent);

	void canvas_item_set_visible(RID p_item, bool p_visible);
	void canvas_item_set_light_mask(RID p_item, int p_mask);

	void canvas_item_set_visibility_layer(RID p_item, uint32_t p_layer);
	uint32_t canvas_item_get_visibility_layer(RID p_item);

	void canvas_item_set_transform(RID p_item, const Transform2D& p_transform);
	void canvas_item_set_clip(RID p_item, bool p_clip);
	void canvas_item_set_distance_field_mode(RID p_item, bool p_enable);
	void canvas_item_set_custom_rect(RID p_item, bool p_custom_rect, const Rect2& p_rect = Rect2());
	void canvas_item_set_modulate(RID p_item, const Color& p_color);
	void canvas_item_set_self_modulate(RID p_item, const Color& p_color);

	void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable);
	void canvas_item_set_use_identity_transform(RID p_item, bool p_enable);

	void canvas_item_set_update_when_visible(RID p_item, bool p_update);

	float canvas_item_get_compensated_antialiasing_width(float p_width) const;

	void canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
		const Color& p_color, float p_width = -1.0, bool p_antialiased = false);
	void canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	void canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	void canvas_item_add_rect(
		RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased);
	void canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major, float p_minor,
		const Color& p_color, bool p_antialiased = false);
	void canvas_item_add_circle(
		RID p_item, const Point2& p_pos, float p_radius, const Color& p_color, bool p_antialiased);
	void canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
		bool p_tile = false, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false);
	void canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = false);
	void canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), int p_outline_size = 0,
		float p_px_range = 1.0, float p_scale = 1.0);
	void canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1));
	void canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect, const Rect2& p_source,
		RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
		RSE::NinePatchAxisMode p_x_axis_mode = RSE::NINE_PATCH_STRETCH,
		RSE::NinePatchAxisMode p_y_axis_mode = RSE::NINE_PATCH_STRETCH, bool p_draw_center = true,
		const Color& p_modulate = Color(1, 1, 1));
	void canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture);
	void canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		RID p_texture = RID());
	void canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(), const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1);
	void canvas_item_add_mesh(RID p_item, const RID& p_mesh,
		const Transform2D& p_transform = Transform2D(), const Color& p_modulate = Color(1, 1, 1),
		RID p_texture = RID());
	void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID());
	void canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture);
	void canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform);
	void canvas_item_add_clip_ignore(RID p_item, bool p_ignore);
	void canvas_item_add_animation_slice(RID p_item, double p_animation_length,
		double p_slice_begin, double p_slice_end, double p_offset);

	void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable);
	void canvas_item_set_z_index(RID p_item, int p_z);
	void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable);
	void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2& p_rect);
	void canvas_item_attach_skeleton(RID p_item, RID p_skeleton);

	void canvas_item_clear(RID p_item);
	void canvas_item_set_draw_index(RID p_item, int p_index);

	void canvas_item_set_material(RID p_item, RID p_material);

	void canvas_item_set_use_parent_material(RID p_item, bool p_enable);

	void canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
		float p_clear_margin = 5.0, bool p_fit_empty = false, float p_fit_margin = 0.0,
		bool p_blur_mipmaps = false);

	void canvas_item_set_debug_redraw(bool p_enabled);
	bool canvas_item_get_debug_redraw() const;

	void canvas_item_set_interpolated(RID p_item, bool p_interpolated);
	void canvas_item_reset_physics_interpolation(RID p_item);
	void canvas_item_transform_physics_interpolation(RID p_item, const Transform2D& p_transform);

	RID canvas_light_allocate();
	void canvas_light_initialize(RID p_rid);

	void update();

	void canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode);
	void canvas_light_attach_to_canvas(RID p_light, RID p_canvas);
	void canvas_light_set_enabled(RID p_light, bool p_enabled);
	void canvas_light_set_texture_scale(RID p_light, float p_scale);
	void canvas_light_set_transform(RID p_light, const Transform2D& p_transform);
	void canvas_light_set_texture(RID p_light, RID p_texture);
	void canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset);
	void canvas_light_set_color(RID p_light, const Color& p_color);
	void canvas_light_set_height(RID p_light, float p_height);
	void canvas_light_set_energy(RID p_light, float p_energy);
	void canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z);
	void canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer);
	void canvas_light_set_item_cull_mask(RID p_light, int p_mask);
	void canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask);
	void canvas_light_set_directional_distance(RID p_light, float p_distance);

	void canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode);

	void canvas_light_set_shadow_enabled(RID p_light, bool p_enabled);
	void canvas_light_set_shadow_filter(RID p_light, RSE::CanvasLightShadowFilter p_filter);
	void canvas_light_set_shadow_color(RID p_light, const Color& p_color);
	void canvas_light_set_shadow_smooth(RID p_light, float p_smooth);

	void canvas_light_set_interpolated(RID p_light, bool p_interpolated);
	void canvas_light_reset_physics_interpolation(RID p_light);
	void canvas_light_transform_physics_interpolation(RID p_light, const Transform2D& p_transform);

	RID canvas_light_occluder_allocate();
	void canvas_light_occluder_initialize(RID p_rid);

	void canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas);
	void canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled);
	void canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon);
	void canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable);
	void canvas_light_occluder_set_transform(RID p_occluder, const Transform2D& p_xform);
	void canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask);

	void canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated);
	void canvas_light_occluder_reset_physics_interpolation(RID p_occluder);
	void canvas_light_occluder_transform_physics_interpolation(
		RID p_occluder, const Transform2D& p_transform);

	RID canvas_occluder_polygon_allocate();
	void canvas_occluder_polygon_initialize(RID p_rid);

	void canvas_occluder_polygon_set_shape(
		RID p_occluder_polygon, const Vector<Vector2>& p_shape, bool p_closed);

	void canvas_occluder_polygon_set_cull_mode(
		RID p_occluder_polygon, RSE::CanvasOccluderPolygonCullMode p_mode);

	void canvas_set_shadow_texture_size(int p_size);

	RID canvas_texture_allocate();
	void canvas_texture_initialize(RID p_rid);

	void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);

	void canvas_texture_set_texture_filter(RID p_item, RSE::CanvasItemTextureFilter p_filter);
	void canvas_texture_set_texture_repeat(RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	void canvas_item_set_default_texture_filter(RID p_item, RSE::CanvasItemTextureFilter p_filter);
	void canvas_item_set_default_texture_repeat(RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	void update_visibility_notifiers();

	void _update_dirty_item(Item* p_item);

	Rect2 _debug_canvas_item_get_rect(RID p_item);

	void finalize();

	/* INTERPOLATION */

	void tick();
	void update_interpolation_tick(bool p_process = true);

	void set_physics_interpolation_enabled(bool p_enabled)
	{
		_interpolation_data.interpolation_enabled = p_enabled;
	}

	struct InterpolationData
	{
		void notify_free_canvas_item(RID p_rid, RendererCanvasCull::Item& r_canvas_item);
		void notify_free_canvas_light(RID p_rid, RendererCanvasRender::Light& r_canvas_light);
		void notify_free_canvas_light_occluder(
			RID p_rid, RendererCanvasRender::LightOccluderInstance& r_canvas_light_occluder);

		LocalVector<RID> canvas_item_transform_update_lists[2];
		LocalVector<RID>* canvas_item_transform_update_list_curr =
			&canvas_item_transform_update_lists[0];
		LocalVector<RID>* canvas_item_transform_update_list_prev =
			&canvas_item_transform_update_lists[1];

		LocalVector<RID> canvas_light_transform_update_lists[2];
		LocalVector<RID>* canvas_light_transform_update_list_curr =
			&canvas_light_transform_update_lists[0];
		LocalVector<RID>* canvas_light_transform_update_list_prev =
			&canvas_light_transform_update_lists[1];

		LocalVector<RID> canvas_light_occluder_transform_update_lists[2];
		LocalVector<RID>* canvas_light_occluder_transform_update_list_curr =
			&canvas_light_occluder_transform_update_lists[0];
		LocalVector<RID>* canvas_light_occluder_transform_update_list_prev =
			&canvas_light_occluder_transform_update_lists[1];

		bool interpolation_enabled = false;
	} _interpolation_data;

	RendererCanvasCull();
	~RendererCanvasCull();
};



// --- End servers/rendering/renderer_canvas_cull.h ---
// --- End servers/rendering/renderer_viewport.h ---

// --- Folded from servers/rendering/shader_language.h ---
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"
#include "core/templates/hash_map.h"
#include "core/os/memory.h"
#include "core/typedefs.h"
#include "core/types.h"

#ifdef DEBUG_ENABLED

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"

class ShaderWarning final
{
public:
	enum Code
	{
		FLOAT_COMPARISON,
		UNUSED_CONSTANT,
		UNUSED_FUNCTION,
		UNUSED_STRUCT,
		UNUSED_UNIFORM,
		UNUSED_VARYING,
		UNUSED_LOCAL_VARIABLE,
		FORMATTING_ERROR,
		DEVICE_LIMIT_EXCEEDED,
		MAGIC_POSITION_WRITE,
		WARNING_MAX,
	};

	enum CodeFlags : uint32_t
	{
		NONE_FLAG = 0U,
		FLOAT_COMPARISON_FLAG = 1U,
		UNUSED_CONSTANT_FLAG = 2U,
		UNUSED_FUNCTION_FLAG = 4U,
		UNUSED_STRUCT_FLAG = 8U,
		UNUSED_UNIFORM_FLAG = 16U,
		UNUSED_VARYING_FLAG = 32U,
		UNUSED_LOCAL_VARIABLE_FLAG = 64U,
		FORMATTING_ERROR_FLAG = 128U,
		DEVICE_LIMIT_EXCEEDED_FLAG = 256U,
		MAGIC_POSITION_WRITE_FLAG = 512U,
	};

private:
	Code code;
	int line;
	String subject;

public:
	Code get_code() const;
	int get_line() const;
	const String& get_subject() const;
	String get_message() const;
	String get_name() const;

	static String get_name_from_code(Code p_code);
	static Code get_code_from_name(const String& p_name);
	static CodeFlags get_flags_from_codemap(const HashMap<Code, bool>& p_map);
};

#endif // DEBUG_ENABLED

class ShaderLanguage
{
public:
	struct TkPos
	{
		int char_idx;
		int tk_line;
	};

	enum TokenType
	{
		TK_EMPTY,
		TK_IDENTIFIER,
		TK_TRUE,
		TK_FALSE,
		TK_FLOAT_CONSTANT,
		TK_INT_CONSTANT,
		TK_UINT_CONSTANT,
		TK_STRING_CONSTANT,
		TK_TYPE_VOID,
		TK_TYPE_BOOL,
		TK_TYPE_BVEC2,
		TK_TYPE_BVEC3,
		TK_TYPE_BVEC4,
		TK_TYPE_INT,
		TK_TYPE_IVEC2,
		TK_TYPE_IVEC3,
		TK_TYPE_IVEC4,
		TK_TYPE_UINT,
		TK_TYPE_UVEC2,
		TK_TYPE_UVEC3,
		TK_TYPE_UVEC4,
		TK_TYPE_FLOAT,
		TK_TYPE_VEC2,
		TK_TYPE_VEC3,
		TK_TYPE_VEC4,
		TK_TYPE_MAT2,
		TK_TYPE_MAT3,
		TK_TYPE_MAT4,
		TK_TYPE_SAMPLER2D,
		TK_TYPE_ISAMPLER2D,
		TK_TYPE_USAMPLER2D,
		TK_TYPE_SAMPLER2DARRAY,
		TK_TYPE_ISAMPLER2DARRAY,
		TK_TYPE_USAMPLER2DARRAY,
		TK_TYPE_SAMPLER3D,
		TK_TYPE_ISAMPLER3D,
		TK_TYPE_USAMPLER3D,
		TK_TYPE_SAMPLERCUBE,
		TK_TYPE_SAMPLERCUBEARRAY,
		TK_TYPE_SAMPLEREXT,
		TK_INTERPOLATION_FLAT,
		TK_INTERPOLATION_SMOOTH,
		TK_CONST,
		TK_STRUCT,
		TK_PRECISION_LOW,
		TK_PRECISION_MID,
		TK_PRECISION_HIGH,
		TK_OP_EQUAL,
		TK_OP_NOT_EQUAL,
		TK_OP_LESS,
		TK_OP_LESS_EQUAL,
		TK_OP_GREATER,
		TK_OP_GREATER_EQUAL,
		TK_OP_AND,
		TK_OP_OR,
		TK_OP_NOT,
		TK_OP_ADD,
		TK_OP_SUB,
		TK_OP_MUL,
		TK_OP_DIV,
		TK_OP_MOD,
		TK_OP_SHIFT_LEFT,
		TK_OP_SHIFT_RIGHT,
		TK_OP_ASSIGN,
		TK_OP_ASSIGN_ADD,
		TK_OP_ASSIGN_SUB,
		TK_OP_ASSIGN_MUL,
		TK_OP_ASSIGN_DIV,
		TK_OP_ASSIGN_MOD,
		TK_OP_ASSIGN_SHIFT_LEFT,
		TK_OP_ASSIGN_SHIFT_RIGHT,
		TK_OP_ASSIGN_BIT_AND,
		TK_OP_ASSIGN_BIT_OR,
		TK_OP_ASSIGN_BIT_XOR,
		TK_OP_BIT_AND,
		TK_OP_BIT_OR,
		TK_OP_BIT_XOR,
		TK_OP_BIT_INVERT,
		TK_OP_INCREMENT,
		TK_OP_DECREMENT,
		TK_CF_IF,
		TK_CF_ELSE,
		TK_CF_FOR,
		TK_CF_WHILE,
		TK_CF_DO,
		TK_CF_SWITCH,
		TK_CF_CASE,
		TK_CF_DEFAULT,
		TK_CF_BREAK,
		TK_CF_CONTINUE,
		TK_CF_RETURN,
		TK_CF_DISCARD,
		TK_BRACKET_OPEN,
		TK_BRACKET_CLOSE,
		TK_CURLY_BRACKET_OPEN,
		TK_CURLY_BRACKET_CLOSE,
		TK_PARENTHESIS_OPEN,
		TK_PARENTHESIS_CLOSE,
		TK_QUESTION,
		TK_COMMA,
		TK_COLON,
		TK_SEMICOLON,
		TK_PERIOD,
		TK_UNIFORM,
		TK_UNIFORM_GROUP,
		TK_INSTANCE,
		TK_GLOBAL,
		TK_VARYING,
		TK_ARG_IN,
		TK_ARG_OUT,
		TK_ARG_INOUT,
		TK_RENDER_MODE,
		TK_STENCIL_MODE,
		TK_HINT_DEFAULT_WHITE_TEXTURE,
		TK_HINT_DEFAULT_BLACK_TEXTURE,
		TK_HINT_DEFAULT_TRANSPARENT_TEXTURE,
		TK_HINT_NORMAL_TEXTURE,
		TK_HINT_ROUGHNESS_NORMAL_TEXTURE,
		TK_HINT_ROUGHNESS_R,
		TK_HINT_ROUGHNESS_G,
		TK_HINT_ROUGHNESS_B,
		TK_HINT_ROUGHNESS_A,
		TK_HINT_ROUGHNESS_GRAY,
		TK_HINT_ANISOTROPY_TEXTURE,
		TK_HINT_SOURCE_COLOR,
		TK_HINT_COLOR_CONVERSION_DISABLED,
		TK_HINT_RANGE,
		TK_HINT_ENUM,
		TK_HINT_INSTANCE_INDEX,
		TK_HINT_SCREEN_TEXTURE,
		TK_HINT_NORMAL_ROUGHNESS_TEXTURE,
		TK_HINT_DEPTH_TEXTURE,
		TK_HINT_BLIT_SOURCE0,
		TK_HINT_BLIT_SOURCE1,
		TK_HINT_BLIT_SOURCE2,
		TK_HINT_BLIT_SOURCE3,
		TK_FILTER_NEAREST,
		TK_FILTER_LINEAR,
		TK_FILTER_NEAREST_MIPMAP,
		TK_FILTER_LINEAR_MIPMAP,
		TK_FILTER_NEAREST_MIPMAP_ANISOTROPIC,
		TK_FILTER_LINEAR_MIPMAP_ANISOTROPIC,
		TK_REPEAT_ENABLE,
		TK_REPEAT_DISABLE,
		TK_SHADER_TYPE,
		TK_CURSOR,
		TK_ERROR,
		TK_EOF,
		TK_MAX
	};

/* COMPILER */

// lame work around to Apple defining this as a macro in 10.12 SDK
#ifdef TYPE_BOOL
#undef TYPE_BOOL
#endif

	enum DataType
	{
		TYPE_VOID,
		TYPE_BOOL,
		TYPE_BVEC2,
		TYPE_BVEC3,
		TYPE_BVEC4,
		TYPE_INT,
		TYPE_IVEC2,
		TYPE_IVEC3,
		TYPE_IVEC4,
		TYPE_UINT,
		TYPE_UVEC2,
		TYPE_UVEC3,
		TYPE_UVEC4,
		TYPE_FLOAT,
		TYPE_VEC2,
		TYPE_VEC3,
		TYPE_VEC4,
		TYPE_MAT2,
		TYPE_MAT3,
		TYPE_MAT4,
		TYPE_SAMPLER2D,
		TYPE_ISAMPLER2D,
		TYPE_USAMPLER2D,
		TYPE_SAMPLER2DARRAY,
		TYPE_ISAMPLER2DARRAY,
		TYPE_USAMPLER2DARRAY,
		TYPE_SAMPLER3D,
		TYPE_ISAMPLER3D,
		TYPE_USAMPLER3D,
		TYPE_SAMPLERCUBE,
		TYPE_SAMPLERCUBEARRAY,
		TYPE_SAMPLEREXT,
		TYPE_STRUCT,
		TYPE_MAX
	};

	enum DataPrecision
	{
		PRECISION_LOWP,
		PRECISION_MEDIUMP,
		PRECISION_HIGHP,
		PRECISION_DEFAULT,
	};

	enum DataInterpolation
	{
		INTERPOLATION_FLAT,
		INTERPOLATION_SMOOTH,
		INTERPOLATION_DEFAULT,
	};

	enum Operator
	{
		OP_EQUAL,
		OP_NOT_EQUAL,
		OP_LESS,
		OP_LESS_EQUAL,
		OP_GREATER,
		OP_GREATER_EQUAL,
		OP_AND,
		OP_OR,
		OP_NOT,
		OP_NEGATE,
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV,
		OP_MOD,
		OP_SHIFT_LEFT,
		OP_SHIFT_RIGHT,
		OP_ASSIGN,
		OP_ASSIGN_ADD,
		OP_ASSIGN_SUB,
		OP_ASSIGN_MUL,
		OP_ASSIGN_DIV,
		OP_ASSIGN_MOD,
		OP_ASSIGN_SHIFT_LEFT,
		OP_ASSIGN_SHIFT_RIGHT,
		OP_ASSIGN_BIT_AND,
		OP_ASSIGN_BIT_OR,
		OP_ASSIGN_BIT_XOR,
		OP_BIT_AND,
		OP_BIT_OR,
		OP_BIT_XOR,
		OP_BIT_INVERT,
		OP_INCREMENT,
		OP_DECREMENT,
		OP_SELECT_IF,
		OP_SELECT_ELSE, // used only internally, then only IF appears with 3 arguments
		OP_POST_INCREMENT,
		OP_POST_DECREMENT,
		OP_CALL,
		OP_CONSTRUCT,
		OP_STRUCT,
		OP_INDEX,
		OP_EMPTY,
		OP_MAX
	};

	enum FlowOperation
	{
		FLOW_OP_IF,
		FLOW_OP_RETURN,
		FLOW_OP_FOR,
		FLOW_OP_WHILE,
		FLOW_OP_DO,
		FLOW_OP_BREAK,
		FLOW_OP_SWITCH,
		FLOW_OP_CASE,
		FLOW_OP_DEFAULT,
		FLOW_OP_CONTINUE,
		FLOW_OP_DISCARD
	};

	enum ArgumentQualifier
	{
		ARGUMENT_QUALIFIER_IN,
		ARGUMENT_QUALIFIER_OUT,
		ARGUMENT_QUALIFIER_INOUT,
	};

	enum SubClassTag
	{
		TAG_GLOBAL,
		TAG_ARRAY,
	};

	enum TextureFilter
	{
		FILTER_NEAREST,
		FILTER_LINEAR,
		FILTER_NEAREST_MIPMAP,
		FILTER_LINEAR_MIPMAP,
		FILTER_NEAREST_MIPMAP_ANISOTROPIC,
		FILTER_LINEAR_MIPMAP_ANISOTROPIC,
		FILTER_DEFAULT,
	};

	enum TextureRepeat
	{
		REPEAT_DISABLE,
		REPEAT_ENABLE,
		REPEAT_DEFAULT,
	};

	enum
	{
		MAX_INSTANCE_UNIFORM_INDICES = 16
	};

	struct VaryingFunctionNames
	{
		String fragment;
		String vertex;
		String light;

		VaryingFunctionNames()
		{
			fragment = "fragment";
			vertex = "vertex";
			light = "light";
		}
	};

	union Scalar
	{
		bool boolean = false;
		float real;
		int32_t sint;
		uint32_t uint;
	};

	struct Node
	{
		Node* next = nullptr;

		enum Type
		{
			NODE_TYPE_SHADER,
			NODE_TYPE_FUNCTION,
			NODE_TYPE_BLOCK,
			NODE_TYPE_VARIABLE,
			NODE_TYPE_VARIABLE_DECLARATION,
			NODE_TYPE_CONSTANT,
			NODE_TYPE_OPERATOR,
			NODE_TYPE_CONTROL_FLOW,
			NODE_TYPE_MEMBER,
			NODE_TYPE_ARRAY,
			NODE_TYPE_ARRAY_CONSTRUCT,
			NODE_TYPE_STRUCT,
		};

		Type type;

		virtual DataType get_datatype() const { return TYPE_VOID; }

		virtual String get_datatype_name() const { return ""; }

		virtual int get_array_size() const { return 0; }

		virtual bool is_indexed() const { return false; }

		virtual Vector<Scalar> get_values() const { return Vector<Scalar>(); }

		Node(Type t) : type(t) {}

		virtual ~Node() {}
	};

	template <typename T> T* alloc_node()
	{
		T* node = memnew(T);
		node->next = nodes;
		nodes = node;
		return node;
	}

	Node* nodes = nullptr;

	struct OperatorNode : public Node
	{
		DataType return_cache = TYPE_VOID;
		DataPrecision return_precision_cache = PRECISION_DEFAULT;
		int return_array_size = 0;
		Operator op = OP_EQUAL;
		String struct_name;
		Vector<Node*> arguments;
		Vector<Scalar> values;

		virtual DataType get_datatype() const override { return return_cache; }

		virtual String get_datatype_name() const override { return String(struct_name); }

		virtual int get_array_size() const override { return return_array_size; }

		virtual bool is_indexed() const override { return op == OP_INDEX; }

		virtual Vector<Scalar> get_values() const override { return values; }

		OperatorNode() : Node(NODE_TYPE_OPERATOR) {}
	};

	struct VariableNode : public Node
	{
		DataType datatype_cache = TYPE_VOID;
		String name;
		String rname;
		String struct_name;
		bool is_const = false;
		bool is_local = false;

		virtual DataType get_datatype() const override { return datatype_cache; }

		virtual String get_datatype_name() const override { return String(struct_name); }

		VariableNode() : Node(NODE_TYPE_VARIABLE) {}
	};

	struct VariableDeclarationNode : public Node
	{
		DataPrecision precision = PRECISION_DEFAULT;
		DataType datatype = TYPE_VOID;
		String struct_name;
		bool is_const = false;

		struct Declaration
		{
			String name;
			uint32_t size = 0U;
			Node* size_expression = nullptr;
			Vector<Node*> initializer;
			bool single_expression = false;
		};

		Vector<Declaration> declarations;

		virtual DataType get_datatype() const override { return datatype; }

		VariableDeclarationNode() : Node(NODE_TYPE_VARIABLE_DECLARATION) {}
	};

	struct ArrayNode : public Node
	{
		DataType datatype_cache = TYPE_VOID;
		String struct_name;
		String name;
		Node* index_expression = nullptr;
		Node* call_expression = nullptr;
		Node* assign_expression = nullptr;
		bool is_const = false;
		int array_size = 0;
		bool is_local = false;

		virtual DataType get_datatype() const override
		{
			return call_expression ? call_expression->get_datatype() : datatype_cache;
		}

		virtual String get_datatype_name() const override
		{
			return call_expression ? call_expression->get_datatype_name() : String(struct_name);
		}

		virtual int get_array_size() const override
		{
			return (index_expression || call_expression) ? 0 : array_size;
		}

		virtual bool is_indexed() const override { return index_expression != nullptr; }

		ArrayNode() : Node(NODE_TYPE_ARRAY) {}
	};

	struct ArrayConstructNode : public Node
	{
		DataType datatype = TYPE_VOID;
		String struct_name;
		Vector<Node*> initializer;

		virtual DataType get_datatype() const override { return datatype; }

		virtual String get_datatype_name() const override { return struct_name; }

		virtual int get_array_size() const override { return initializer.size(); }

		ArrayConstructNode() : Node(NODE_TYPE_ARRAY_CONSTRUCT) {}
	};

	struct ConstantNode : public Node
	{
		DataType datatype = TYPE_VOID;
		String struct_name = "";
		int array_size = 0;

		Vector<Scalar> values;
		Vector<VariableDeclarationNode::Declaration> array_declarations;

		virtual DataType get_datatype() const override { return datatype; }

		virtual String get_datatype_name() const override { return struct_name; }

		virtual int get_array_size() const override { return array_size; }

		virtual Vector<Scalar> get_values() const override { return values; }

		ConstantNode() : Node(NODE_TYPE_CONSTANT) {}
	};

	struct FunctionNode;

	struct BlockNode : public Node
	{
		FunctionNode* parent_function = nullptr;
		BlockNode* parent_block = nullptr;

		enum BlockType
		{
			BLOCK_TYPE_STANDARD,
			BLOCK_TYPE_FOR_INIT,
			BLOCK_TYPE_FOR_CONDITION,
			BLOCK_TYPE_FOR_EXPRESSION,
			BLOCK_TYPE_SWITCH,
			BLOCK_TYPE_CASE,
			BLOCK_TYPE_DEFAULT,
		};

		int block_type = BLOCK_TYPE_STANDARD;
		SubClassTag block_tag = SubClassTag::TAG_GLOBAL;

		struct Variable
		{
			DataType type;
			String struct_name;
			DataPrecision precision;
			int line; // for completion
			int array_size;
			bool is_const;
			Vector<Scalar> values;
		};

		HashMap<String, Variable> variables;
		List<Node*> statements;
		bool single_statement = false;
		bool use_comma_between_statements = false;
		bool use_op_eval = true;

		DataType expected_type = TYPE_VOID;
		HashSet<int> constants;

		BlockNode() : Node(NODE_TYPE_BLOCK) {}
	};

	struct ControlFlowNode : public Node
	{
		FlowOperation flow_op = FLOW_OP_IF;
		Vector<Node*> expressions;
		Vector<BlockNode*> blocks;

		ControlFlowNode() : Node(NODE_TYPE_CONTROL_FLOW) {}
	};

	struct MemberNode : public Node
	{
		DataType basetype = TYPE_VOID;
		bool basetype_const = false;
		String base_struct_name;
		DataPrecision precision = PRECISION_DEFAULT;
		DataType datatype = TYPE_VOID;
		int array_size = 0;
		String struct_name;
		String name;
		Node* owner = nullptr;
		Node* index_expression = nullptr;
		Node* assign_expression = nullptr;
		Node* call_expression = nullptr;
		bool has_swizzling_duplicates = false;

		virtual DataType get_datatype() const override
		{
			return call_expression ? call_expression->get_datatype() : datatype;
		}

		virtual String get_datatype_name() const override
		{
			return call_expression ? call_expression->get_datatype_name() : String(struct_name);
		}

		virtual int get_array_size() const override
		{
			return (index_expression || call_expression) ? 0 : array_size;
		}

		virtual bool is_indexed() const override
		{
			return index_expression != nullptr || call_expression != nullptr;
		}

		MemberNode() : Node(NODE_TYPE_MEMBER) {}
	};

	struct StructNode : public Node
	{
		List<MemberNode*> members;

		StructNode() : Node(NODE_TYPE_STRUCT) {}
	};

	struct ShaderNode : public Node
	{
		struct Constant
		{
			String name;
			DataType type;
			String struct_name;
			DataPrecision precision;
			Node* initializer = nullptr;
			int array_size;
		};

		struct Function
		{
			String name;
			String rname;
			FunctionNode* function = nullptr;
			HashSet<String> uses_function;
			bool callable;
		};

		struct Struct
		{
			String name;
			StructNode* shader_struct = nullptr;
		};

		struct Varying
		{
			enum Stage
			{
				STAGE_UNKNOWN,
				STAGE_VERTEX,
				STAGE_FRAGMENT,
			};

			Stage stage = STAGE_UNKNOWN;
			DataType type = TYPE_VOID;
			DataInterpolation interpolation = INTERPOLATION_FLAT;
			DataPrecision precision = PRECISION_DEFAULT;
			int array_size = 0;
			TkPos tkpos;

			uint32_t get_size() const
			{
				uint32_t size = 1;
				if (array_size > 0) {
					size = (uint32_t)array_size;
				}

				switch (type) {
				case TYPE_MAT2:
					size *= 2;
					break;
				case TYPE_MAT3:
					size *= 3;
					break;
				case TYPE_MAT4:
					size *= 4;
					break;
				default:
					break;
				}
				return size;
			}

			Varying() {}
		};

		struct Uniform
		{
			enum Hint
			{
				HINT_NONE,
				HINT_RANGE,
				HINT_ENUM,
				HINT_SOURCE_COLOR,
				HINT_COLOR_CONVERSION_DISABLED,
				HINT_NORMAL,
				HINT_ROUGHNESS_NORMAL,
				HINT_ROUGHNESS_R,
				HINT_ROUGHNESS_G,
				HINT_ROUGHNESS_B,
				HINT_ROUGHNESS_A,
				HINT_ROUGHNESS_GRAY,
				HINT_DEFAULT_BLACK,
				HINT_DEFAULT_WHITE,
				HINT_DEFAULT_TRANSPARENT,
				HINT_ANISOTROPY,
				HINT_SCREEN_TEXTURE,
				HINT_BLIT_SOURCE0,
				HINT_BLIT_SOURCE1,
				HINT_BLIT_SOURCE2,
				HINT_BLIT_SOURCE3,
				HINT_NORMAL_ROUGHNESS_TEXTURE,
				HINT_DEPTH_TEXTURE,
				HINT_MAX
			};

			enum Scope
			{
				SCOPE_LOCAL,
				SCOPE_INSTANCE,
				SCOPE_GLOBAL,
			};

			int order = 0;
			int prop_order = 0;
			int texture_order = 0;
			int texture_binding = 0;
			DataType type = TYPE_VOID;
			DataPrecision precision = PRECISION_DEFAULT;
			int array_size = 0;
			Vector<Scalar> default_value;
			Scope scope = SCOPE_LOCAL;
			Hint hint = HINT_NONE;
			bool use_color = false;
			TextureFilter filter = FILTER_DEFAULT;
			TextureRepeat repeat = REPEAT_DEFAULT;
			float hint_range[3];
			PackedStringArray hint_enum_names;
			int instance_index = 0;
			String group;

			_FORCE_INLINE_ bool is_texture() const
			{
				// Order is assigned to -1 for texture uniforms.
				return order < 0;
			}

			Uniform()
			{
				hint_range[0] = 0.0f;
				hint_range[1] = 1.0f;
				hint_range[2] = 0.001f;
			}
		};

		HashMap<String, Constant> constants;
		HashMap<String, Varying> varyings;
		HashMap<String, Uniform> uniforms;
		HashMap<String, Struct> structs;
		HashMap<String, Function> functions;
		Vector<String> render_modes;
		Vector<String> stencil_modes;
		int stencil_reference = -1;

		Vector<Function> vfunctions;
		Vector<Constant> vconstants;
		Vector<Struct> vstructs;

		ShaderNode() : Node(NODE_TYPE_SHADER) {}
	};

	struct FunctionNode : public Node
	{
		struct Argument
		{
			ArgumentQualifier qualifier;
			String name;
			DataType type;
			String struct_name;
			DataPrecision precision;
			// for passing textures as arguments
			bool tex_argument_check;
			TextureFilter tex_argument_filter;
			TextureRepeat tex_argument_repeat;
			bool tex_builtin_check;
			String tex_builtin;
			ShaderNode::Uniform::Hint tex_hint;
			bool is_const;
			int array_size;

			HashMap<String, HashSet<int>> tex_argument_connect;
		};

		String name;
		String rname;
		DataType return_type = TYPE_VOID;
		String return_struct_name;
		DataPrecision return_precision = PRECISION_DEFAULT;
		int return_array_size = 0;
		Vector<Argument> arguments;
		BlockNode* body = nullptr;
		bool can_discard = false;

		virtual DataType get_datatype() const override { return return_type; }

		virtual String get_datatype_name() const override { return String(return_struct_name); }

		virtual int get_array_size() const override { return return_array_size; }

		FunctionNode() : Node(NODE_TYPE_FUNCTION) {}
	};

	struct UniformOrderComparator
	{
		_FORCE_INLINE_ bool operator()(
			const Pair<String, int>& A, const Pair<String, int>& B) const
		{
			return A.second < B.second;
		}
	};

	struct Expression
	{
		bool is_op;

		union
		{
			Operator op;
			Node* node = nullptr;
		};
	};

	struct ExpressionInfo
	{
		Vector<Expression>* expression = nullptr;
		TokenType tt_break = TK_EMPTY;
		bool is_last_expr = false;
	};

	struct VarInfo
	{
		String name;
		DataType type;
	};

	enum CompletionType
	{
		COMPLETION_NONE,
		COMPLETION_SHADER_TYPE,
		COMPLETION_RENDER_MODE,
		COMPLETION_STENCIL_MODE,
		COMPLETION_MAIN_FUNCTION,
		COMPLETION_IDENTIFIER,
		COMPLETION_FUNCTION_CALL,
		COMPLETION_CALL_ARGUMENTS,
		COMPLETION_INDEX,
		COMPLETION_STRUCT,
		COMPLETION_HINT,
	};

	struct Token
	{
		TokenType type;
		String text;
		double constant;
		uint16_t line;

		bool is_integer_constant() const
		{
			return type == TK_INT_CONSTANT || type == TK_UINT_CONSTANT;
		}
	};

	static String get_operator_text(Operator p_op);
	static String get_token_text(Token p_token);

	static bool is_token_datatype(TokenType p_type);
	static bool is_token_variable_datatype(TokenType p_type);
	static DataType get_token_datatype(TokenType p_type);
	static bool is_token_interpolation(TokenType p_type);
	static DataInterpolation get_token_interpolation(TokenType p_type);
	static bool is_token_precision(TokenType p_type);
	static bool is_token_arg_qual(TokenType p_type);
	static DataPrecision get_token_precision(TokenType p_type);
	static String get_precision_name(DataPrecision p_type);
	static String get_interpolation_name(DataInterpolation p_interpolation);
	static String get_datatype_name(DataType p_type);
	static String get_uniform_hint_name(ShaderNode::Uniform::Hint p_hint);
	static String get_texture_filter_name(TextureFilter p_filter);
	static String get_texture_repeat_name(TextureRepeat p_repeat);
	static bool is_token_nonvoid_datatype(TokenType p_type);
	static bool is_token_operator(TokenType p_type);
	static bool is_token_operator_assign(TokenType p_type);
	static bool is_token_hint(TokenType p_type);

	static bool convert_constant(
		ConstantNode* p_constant, DataType p_to_type, Scalar* p_value = nullptr);
	static DataType get_scalar_type(DataType p_type);
	static int get_cardinality(DataType p_type);
	static bool is_scalar_type(DataType p_type);
	static bool is_float_type(DataType p_type);
	static bool is_sampler_type(DataType p_type);
	static bool is_hint_color(ShaderNode::Uniform::Hint p_hint);
	static uint32_t get_datatype_size(DataType p_type);
	static uint32_t get_datatype_component_count(DataType p_type);

	static void get_keyword_list(List<String>* r_keywords);
	static bool is_control_flow_keyword(String p_keyword);
	static void get_builtin_funcs(List<String>* r_keywords);

	static SafeNumeric<int> instance_counter;

	struct BuiltInInfo
	{
		DataType type = TYPE_VOID;
		bool constant = false;
		Vector<Scalar> values;

		BuiltInInfo() {}

		BuiltInInfo(DataType p_type, bool p_constant = false, const Vector<Scalar>& p_values = {})
			: type(p_type), constant(p_constant), values(p_values)
		{
		}
	};

	struct StageFunctionInfo
	{
		struct Argument
		{
			String name;
			DataType type;

			Argument(const String& p_name = String(), DataType p_type = TYPE_VOID)
			{
				name = p_name;
				type = p_type;
			}
		};

		Vector<Argument> arguments;
		DataType return_type = TYPE_VOID;
		String skip_function;
	};

	struct ModeInfo
	{
		String name;
		Vector<String> options;

		ModeInfo() {}

		ModeInfo(const String& p_name) : name(p_name) {}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3, const String& p_arg4)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
			options.push_back(p_arg4);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3, const String& p_arg4, const String& p_arg5)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
			options.push_back(p_arg4);
			options.push_back(p_arg5);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3, const String& p_arg4, const String& p_arg5,
			const String& p_arg6)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
			options.push_back(p_arg4);
			options.push_back(p_arg5);
			options.push_back(p_arg6);
		}

		ModeInfo(const String& p_name, std::initializer_list<String> p_args) : name(p_name)
		{
			for (const String& arg : p_args) {
				options.push_back(arg);
			}
		}
	};

	struct FunctionInfo
	{
		HashMap<String, BuiltInInfo> built_ins;
		HashMap<String, StageFunctionInfo> stage_functions;

		bool can_discard = false;
		bool main_function = false;
	};

	static bool has_builtin(const HashMap<String, ShaderLanguage::FunctionInfo>& p_functions,
		const String& p_name, bool p_check_global_funcs = false);

	typedef DataType (*GlobalShaderUniformGetTypeFunc)(const String& p_name);

	struct FilePosition
	{
		String file;
		int line = 0;
	};

private:
	struct KeyWord
	{
		TokenType token;
		const char* text;
		uint32_t flags;
		const Vector<String> excluded_shader_types;
		const Vector<String> excluded_functions;
	};

	static const KeyWord keyword_list[];

	GlobalShaderUniformGetTypeFunc global_shader_uniform_get_type_func = nullptr;

	bool error_set = false;
	String error_str;
	int error_line = 0;

	Vector<FilePosition> include_positions;
	HashSet<String> include_markers_handled;
	HashMap<String, int> function_overload_count;

	// Additional function information (eg. call hierarchy). No need to expose it to compiler.
	struct CallInfo
	{
		struct Item
		{
			enum ItemType
			{
				ITEM_TYPE_BUILTIN,
				ITEM_TYPE_VARYING,
			} type;

			TkPos pos;

			Item() {}

			Item(ItemType p_type, TkPos p_pos) : type(p_type), pos(p_pos) {}
		};

		String name;
		List<Pair<String, Item>> uses_restricted_items;
		List<CallInfo*> calls;
	};

	RBMap<String, CallInfo> calls_info;

#ifdef DEBUG_ENABLED
	struct Usage
	{
		int decl_line;
		bool used = false;

		Usage(int p_decl_line = -1) { decl_line = p_decl_line; }
	};

	HashMap<String, Usage> used_constants;
	HashMap<String, Usage> used_varyings;
	HashMap<String, Usage> used_uniforms;
	HashMap<String, Usage> used_functions;
	HashMap<String, Usage> used_structs;
	HashMap<ShaderWarning::Code, HashMap<String, Usage>*> warnings_check_map;

	HashMap<String, HashMap<String, Usage>> used_local_vars;
	HashMap<ShaderWarning::Code, HashMap<String, HashMap<String, Usage>>*>
		warnings_check_map2;

	List<ShaderWarning> warnings;

	bool check_warnings = false;
	uint32_t warning_flags = 0;
	void _check_warning_accums();
#endif // DEBUG_ENABLED

	String code;
	int char_idx = 0;
	int tk_line = 0;

	String shader_type_identifier;
	String current_function;
	bool is_const_decl = false;
	String last_name;
	bool is_shader_inc = false;

	String current_uniform_group_name;

	VaryingFunctionNames varying_function_names;
	uint32_t base_varying_index = 0;

	TkPos _get_tkpos()
	{
		TkPos tkp;
		tkp.char_idx = char_idx;
		tkp.tk_line = tk_line;
		return tkp;
	}

	void _set_tkpos(TkPos p_pos)
	{
		char_idx = p_pos.char_idx;
		tk_line = p_pos.tk_line;
	}

	void _set_error(const String& p_str)
	{
		if (error_set) {
			return;
		}

		error_line = tk_line;
		error_set = true;
		error_str = p_str;
		include_positions.write[include_positions.size() - 1].line = tk_line;
	}

	void _set_expected_error(const String& p_what)
	{
		_set_error(vformat(RTR("Expected a '%s'."), p_what));
	}

	void _set_expected_error(const String& p_first, const String p_second)
	{
		_set_error(vformat(RTR("Expected a '%s' or '%s'."), p_first, p_second));
	}

	void _set_expected_after_error(const String& p_what, const String& p_after)
	{
		_set_error(vformat(RTR("Expected a '%s' after '%s'."), p_what, p_after));
	}

	void _set_redefinition_error(const String& p_what)
	{
		_set_error(vformat(RTR("Redefinition of '%s'."), p_what));
	}

	void _set_parsing_error() { _set_error("Parser bug."); }

	static const char* token_names[TK_MAX];

	Token _make_token(TokenType p_type, const String& p_text = String());
	Token _get_token();
	bool _lookup_next(Token& r_tk);
	Token _peek();

	ShaderNode* shader = nullptr;

	enum IdentifierType
	{
		IDENTIFIER_FUNCTION,
		IDENTIFIER_UNIFORM,
		IDENTIFIER_VARYING,
		IDENTIFIER_FUNCTION_ARGUMENT,
		IDENTIFIER_LOCAL_VAR,
		IDENTIFIER_BUILTIN_VAR,
		IDENTIFIER_CONSTANT,
		IDENTIFIER_MAX,
	};

	IdentifierType last_type = IDENTIFIER_MAX;

	bool _find_identifier(const BlockNode* p_block, bool p_allow_reassign,
		const FunctionInfo& p_function_info, const String& p_identifier,
		DataType* r_data_type = nullptr, IdentifierType* r_type = nullptr,
		bool* r_is_const = nullptr, int* r_array_size = nullptr,
		String* r_struct_name = nullptr, Vector<Scalar>* r_constant_values = nullptr);
#ifdef DEBUG_ENABLED
	void _parse_used_identifier(
		const String& p_identifier, IdentifierType p_type, const String& p_function);
#endif // DEBUG_ENABLED
	bool _is_operator_assign(Operator p_op) const;
	bool _validate_assign(
		Node* p_node, const FunctionInfo& p_function_info, String* r_message = nullptr);
	bool _validate_operator(const BlockNode* p_block, const FunctionInfo& p_function_info,
		OperatorNode* p_op, DataType* r_ret_type = nullptr, int* r_ret_size = nullptr,
		String* r_ret_struct_name = nullptr);

	Vector<Scalar> _get_node_values(
		const BlockNode* p_block, const FunctionInfo& p_function_info, Node* p_node);
	bool _eval_operator(
		const BlockNode* p_block, const FunctionInfo& p_function_info, OperatorNode* p_op);
	Scalar _eval_unary_scalar(const Scalar& p_a, Operator p_op, DataType p_ret_type);
	Scalar _eval_scalar(
		const Scalar& p_a, const Scalar& p_b, Operator p_op, DataType p_ret_type, bool& r_is_valid);
	Vector<Scalar> _eval_unary_vector(
		const Vector<Scalar>& p_va, DataType p_ret_type, Operator p_op);
	Vector<Scalar> _eval_vector(const Vector<Scalar>& p_va, const Vector<Scalar>& p_vb,
		DataType p_left_type, DataType p_right_type, DataType p_ret_type, Operator p_op,
		bool& r_is_valid);
	Vector<Scalar> _eval_vector_transform(const Vector<Scalar>& p_va, const Vector<Scalar>& p_vb,
		DataType p_left_type, DataType p_right_type, DataType p_ret_type);

	struct BuiltinEntry
	{
		const char* name;
	};

	struct BuiltinFuncDef
	{
		enum
		{
			MAX_ARGS = 5
		};

		const char* name;
		DataType rettype;
		const DataType args[MAX_ARGS];
		const char* args_names[MAX_ARGS];
		SubClassTag tag;
		bool high_end;
	};

	struct BuiltinFuncOutArgs
	{ // arguments used as out in built in functions

		enum
		{
			MAX_ARGS = 2
		};

		const char* name;
		const int arguments[MAX_ARGS];
	};

	struct BuiltinFuncConstArgs
	{
		const char* name;
		int arg;
		int min;
		int max;
	};

	CompletionType completion_type;
	ShaderNode::Uniform::Hint current_uniform_hint = ShaderNode::Uniform::HINT_NONE;
	TextureFilter current_uniform_filter = FILTER_DEFAULT;
	TextureRepeat current_uniform_repeat = REPEAT_DEFAULT;
	bool current_uniform_instance_index_defined = false;
	int completion_line = 0;
	BlockNode* completion_block = nullptr;
	DataType completion_base;
	bool completion_base_array = false;
	SubClassTag completion_class;
	String completion_function;
	String completion_struct;
	int completion_argument = 0;

#ifdef DEBUG_ENABLED
	uint32_t keyword_completion_context;
#endif // DEBUG_ENABLED

	const HashMap<String, FunctionInfo>* stages = nullptr;
	bool is_supported_frag_only_funcs = false;
	bool is_discard_supported = false;

	bool _get_completable_identifier(
		BlockNode* p_block, CompletionType p_type, String& identifier);
	static const BuiltinFuncDef builtin_func_defs[];
	static const BuiltinFuncOutArgs builtin_func_out_args[];
	static const BuiltinFuncConstArgs builtin_func_const_args[];
	static const BuiltinEntry frag_only_func_defs[];

	static bool is_const_suffix_lut_initialized;

	Error _validate_precision(DataType p_type, DataPrecision p_precision);
	bool _compare_datatypes(DataType p_datatype_a, String p_datatype_name_a, int p_array_size_a,
		DataType p_datatype_b, String p_datatype_name_b, int p_array_size_b);
	bool _compare_datatypes_in_nodes(Node* a, Node* b);

	bool _validate_function_call(BlockNode* p_block, const FunctionInfo& p_function_info,
		OperatorNode* p_func, DataType* r_ret_type, String* r_ret_type_str,
		bool* r_is_custom_function = nullptr);
	bool _parse_function_arguments(BlockNode* p_block, const FunctionInfo& p_function_info,
		OperatorNode* p_func, int* r_complete_arg = nullptr);
	ShaderNode::Uniform::Hint _sanitize_hint(ShaderNode::Uniform::Hint p_hint);
	bool _propagate_function_call_sampler_uniform_settings(const String& p_name, int p_argument,
		TextureFilter p_filter, TextureRepeat p_repeat, ShaderNode::Uniform::Hint p_hint);
	bool _propagate_function_call_sampler_builtin_reference(
		const String& p_name, int p_argument, const String& p_builtin);
	bool _validate_varying_assign(ShaderNode::Varying& p_varying, String* r_message);
	bool _check_node_constness(const Node* p_node) const;

	bool _check_restricted_func(
		const String& p_name, const String& p_current_function) const;
	bool _validate_restricted_func(
		const String& p_call_name, const CallInfo* p_func_info, bool p_is_builtin_hint = false);

	Node* _parse_expression(BlockNode* p_block, const FunctionInfo& p_function_info,
		const ExpressionInfo* p_previous_expression_info = nullptr);
	Error _parse_array_size(BlockNode* p_block, const FunctionInfo& p_function_info,
		bool p_forbid_unknown_size, Node** r_size_expression, int* r_array_size,
		bool* r_unknown_size);
	Node* _parse_array_constructor(BlockNode* p_block, const FunctionInfo& p_function_info);
	Node* _parse_array_constructor(BlockNode* p_block, const FunctionInfo& p_function_info,
		DataType p_type, const String& p_struct_name, int p_array_size);
	ShaderLanguage::Node* _reduce_expression(BlockNode* p_block, ShaderLanguage::Node* p_node);

	Node* _parse_and_reduce_expression(BlockNode* p_block, const FunctionInfo& p_function_info,
		const ExpressionInfo* p_previous_expression_info = nullptr);
	Error _parse_block(BlockNode* p_block, const FunctionInfo& p_function_info,
		bool p_just_one = false, bool p_can_break = false, bool p_can_continue = false);
	String _get_shader_type_list(const HashSet<String>& p_shader_types) const;
	String _get_qualifier_str(ArgumentQualifier p_qualifier) const;

	bool _parse_numeric_constant_expression(const FunctionInfo& p_function_info, float& r_constant);
	Error _parse_shader(const HashMap<String, FunctionInfo>& p_functions,
		const Vector<ModeInfo>& p_render_modes, const Vector<ModeInfo>& p_stencil_modes,
		const HashSet<String>& p_shader_types);

	Error _find_last_flow_op_in_block(BlockNode* p_block, FlowOperation p_op);
	Error _find_last_flow_op_in_op(ControlFlowNode* p_flow, FlowOperation p_op);

	Error _parse_shader_mode(bool p_is_stencil, const Vector<ModeInfo>& p_modes,
		HashMap<String, String>& r_defined_modes);

public:
#ifdef DEBUG_ENABLED
	List<ShaderWarning>::Element* get_warnings_ptr();

	void enable_warning_checking(bool p_enabled);
	bool is_warning_checking_enabled() const;

	void set_warning_flags(uint32_t p_flags);
	uint32_t get_warning_flags() const;
#endif // DEBUG_ENABLED

	// static void get_keyword_list(ShaderType p_type,List<String> *p_keywords);

	static void finish();
	void clear();

	static String get_shader_type(const String& p_code);
	static bool is_builtin_func_out_parameter(const String& p_name, int p_param);

	struct ShaderCompileInfo
	{
		HashMap<String, FunctionInfo> functions;
		Vector<ModeInfo> render_modes;
		Vector<ModeInfo> stencil_modes;
		VaryingFunctionNames varying_function_names;
		HashSet<String> shader_types;
		GlobalShaderUniformGetTypeFunc global_shader_uniform_type_func = nullptr;
		bool is_include = false;
		uint32_t base_varying_index = 0;
	};

	Error compile(const String& p_code, const ShaderCompileInfo& p_info);

	String get_error_text();
	Vector<FilePosition> get_include_positions();

	CompletionType get_completion_type() const { return completion_type; }

	int get_error_line();

	ShaderNode* get_shader();

	String token_debug(const String& p_code);

	ShaderLanguage();
	~ShaderLanguage();
};

// --- Folded from servers/rendering/shader_compiler.h ---
#include "core/templates/hashfuncs.h"
#include "scene/resources/texture.h"


class ShaderCompiler final
{
public:
	enum Stage
	{
		STAGE_VERTEX,
		STAGE_FRAGMENT,
		STAGE_COMPUTE,
		STAGE_RAYGEN,
		STAGE_ANY_HIT,
		STAGE_CLOSEST_HIT,
		STAGE_MISS,
		STAGE_INTERSECTION,
		STAGE_MAX
	};

	struct IdentifierActions
	{
		HashMap<String, Stage> entry_point_stages;

		HashMap<String, Pair<int*, int>> render_mode_values;
		HashMap<String, bool*> render_mode_flags;
		HashMap<String, bool*> usage_flag_pointers;
		HashMap<String, bool*> write_flag_pointers;
		HashMap<String, Pair<int*, int>> stencil_mode_values;
		int* stencil_reference = nullptr;

		HashMap<String, ShaderLanguage::ShaderNode::Uniform>* uniforms = nullptr;
	};

	struct GeneratedCode
	{
		Vector<String> defines;

		struct Texture
		{
			String name;
			ShaderLanguage::DataType type = ShaderLanguage::DataType::TYPE_VOID;
			ShaderLanguage::ShaderNode::Uniform::Hint hint =
				ShaderLanguage::ShaderNode::Uniform::Hint::HINT_NONE;
			bool use_color = false;
			ShaderLanguage::TextureFilter filter = ShaderLanguage::TextureFilter::FILTER_DEFAULT;
			ShaderLanguage::TextureRepeat repeat = ShaderLanguage::TextureRepeat::REPEAT_DEFAULT;
			bool global = false;
			int array_size = 0;
		};

		Vector<Texture> texture_uniforms;

		Vector<uint32_t> uniform_offsets;
		uint32_t uniform_total_size = 0;
		String uniforms;
		String stage_globals[STAGE_MAX];

		HashMap<String, String> code;

		bool uses_global_textures = false;
		bool uses_fragment_time = false;
		bool uses_vertex_time = false;
		bool uses_screen_texture_mipmaps = false;
		bool uses_screen_texture = false;
		bool uses_depth_texture = false;
		bool uses_normal_roughness_texture = false;
	};

	struct DefaultIdentifierActions
	{
		HashMap<String, String> renames;
		HashMap<String, String> render_mode_defines;
		HashMap<String, String> usage_defines;
		HashMap<String, String> custom_samplers;
		ShaderLanguage::TextureFilter default_filter =
			ShaderLanguage::TextureFilter::FILTER_NEAREST;
		ShaderLanguage::TextureRepeat default_repeat =
			ShaderLanguage::TextureRepeat::REPEAT_DISABLE;
		int base_texture_binding_index = 0;
		int texture_layout_set = 0;
		String base_uniform_string;
		String global_buffer_array_variable;
		String instance_uniform_index_variable;
		uint32_t base_varying_index = 0;
		bool apply_luminance_multiplier = false;
		bool check_multiview_samplers = false;
	};

private:
	ShaderLanguage parser;

	String _get_sampler_name(
		ShaderLanguage::TextureFilter p_filter, ShaderLanguage::TextureRepeat p_repeat);

	void _dump_function_deps(const ShaderLanguage::ShaderNode* p_node, const String& p_for_func,
		const HashMap<String, String>& p_func_code, String& r_to_add,
		HashSet<String>& added);
	String _dump_node_code(const ShaderLanguage::Node* p_node, int p_level,
		GeneratedCode& r_gen_code, IdentifierActions& p_actions,
		const DefaultIdentifierActions& p_default_actions, bool p_assigning, bool p_scope = true);

	const ShaderLanguage::ShaderNode* shader = nullptr;
	const ShaderLanguage::FunctionNode* function = nullptr;
	String current_func_name;
	String time_name;
	HashSet<String> texture_functions;

	HashSet<String> used_name_defines;
	HashSet<String> used_flag_pointers;
	HashSet<String> used_rmode_defines;
	HashSet<String> internal_functions;
	HashSet<String> fragment_varyings;

	DefaultIdentifierActions actions;

	static ShaderLanguage::DataType _get_global_shader_uniform_type(const String& p_name);

public:
	Error compile(RSE::ShaderMode p_mode, const String& p_code, IdentifierActions* p_actions,
		const String& p_path, GeneratedCode& r_gen_code);

	void initialize(DefaultIdentifierActions p_actions);
	ShaderCompiler();
};

// --- Folded from servers/rendering/shader_preprocessor.h ---
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/hash_map.h"
#include "core/templates/rb_set.h"
#include "scene/resources/material.h"

class ShaderPreprocessor
{
public:
	enum CompletionType
	{
		COMPLETION_TYPE_NONE,
		COMPLETION_TYPE_DIRECTIVE,
		COMPLETION_TYPE_PRAGMA_DIRECTIVE,
		COMPLETION_TYPE_PRAGMA,
		COMPLETION_TYPE_CONDITION,
		COMPLETION_TYPE_INCLUDE_PATH,
	};

	struct FilePosition
	{
		String file;
		int line = 0;
	};

	struct Region
	{
		String file;
		int from_line = -1;
		int to_line = -1;
		bool enabled = false;
		Region* parent = nullptr;
	};

private:
	struct Token
	{
		char32_t text;
		int line;

		Token();
		Token(char32_t p_text, int p_line);
	};

	// The real preprocessor that understands basic shader and preprocessor language syntax.
	class Tokenizer
	{
	public:
		String code;
		int line;
		int index;
		int size;
		LocalVector<Token> generated;

	private:
		void add_generated(const Token& p_t);
		char32_t next();

	public:
		int get_line() const;
		int get_index() const;
		char32_t peek();
		int consume_line_continuations(int p_offset);

		void get_and_clear_generated(LocalVector<char32_t>* r_out);
		void backtrack(char32_t p_what);
		LocalVector<Token> advance(char32_t p_what);
		void skip_whitespace();
		bool consume_empty_line();
		String get_identifier(bool* r_is_cursor = nullptr, bool p_started = false);
		String peek_identifier();
		Token get_token();

		Tokenizer(const String& p_code);
	};

	class CommentRemover
	{
	private:
		LocalVector<char32_t> stripped;
		String code;
		int index;
		int line;
		int comment_line_open;
		int comments_open;
		int strings_open;

	public:
		String get_error() const;
		int get_error_line() const;
		char32_t peek() const;

		bool advance(char32_t p_what);
		String strip();

		CommentRemover(const String& p_code);
	};

	struct Define
	{
		Vector<String> arguments;
		String body;
		bool is_builtin = false;
	};

	struct Branch
	{
		Vector<bool> conditions;
		Branch* parent = nullptr;
		bool else_defined = false;

		Branch() {}

		Branch(bool p_condition, Branch* p_parent) : parent(p_parent)
		{
			conditions.push_back(p_condition);
		}
	};

	struct State
	{
		RBMap<String, Define*> defines;
		List<Branch> branches;
		Branch* current_branch = nullptr;
		int condition_depth = 0;
		RBSet<String> includes;
		List<uint64_t> cyclic_include_hashes; // Holds code hash of includes.
		int include_depth = 0;
		String current_filename;
		String current_shader_type;
		String error;
		List<FilePosition> include_positions;
		bool save_regions = false;
		RBMap<String, List<Region>> regions;
		Region* previous_region = nullptr;
		bool disabled = false;
		CompletionType completion_type = COMPLETION_TYPE_NONE;
		HashSet<Ref<ShaderInclude>> shader_includes;
		bool completion_show_defines = false;
	};

private:
	LocalVector<char32_t> output;
	State* state = nullptr;

private:
	static bool is_char_word(char32_t p_char);
	static bool is_char_space(char32_t p_char);
	static bool is_char_end(char32_t p_char);
	static String vector_to_string(
		const LocalVector<char32_t>& p_v, int p_start = 0, int p_end = -1);
	static String tokens_to_string(const LocalVector<Token>& p_tokens);

	void _set_expected_error(const String& p_what, int p_line)
	{
		set_error(vformat(RTR("Expected a '%s'."), p_what), p_line);
	}

	void _set_unexpected_token_error(const String& p_what, int p_line)
	{
		set_error(vformat(RTR("Unexpected token: '%s'."), p_what), p_line);
	}

	void process_directive(Tokenizer* p_tokenizer);
	void process_define(Tokenizer* p_tokenizer);
	void process_elif(Tokenizer* p_tokenizer);
	void process_else(Tokenizer* p_tokenizer);
	void process_endif(Tokenizer* p_tokenizer);
	void process_error(Tokenizer* p_tokenizer);
	void process_if(Tokenizer* p_tokenizer);
	void process_ifdef(Tokenizer* p_tokenizer);
	void process_ifndef(Tokenizer* p_tokenizer);
	void process_include(Tokenizer* p_tokenizer);
	void process_pragma(Tokenizer* p_tokenizer);
	void process_undef(Tokenizer* p_tokenizer);

	void add_region(int p_line, bool p_enabled, Region* p_parent_region);
	void start_branch_condition(Tokenizer* p_tokenizer, bool p_success, bool p_continue = false);

	Error expand_condition(const String& p_string, int p_line, String& r_result);
	void expand_output_macros(int p_start, int p_line);
	Error expand_macros(const String& p_string, int p_line, String& r_result);
	bool expand_macros_once(const String& p_line, int p_line_number,
		const RBMap<String, Define*>::Element* p_define_pair, String& r_expanded);
	bool find_match(
		const String& p_string, const String& p_value, int& r_index, int& r_index_start);
	void concatenate_macro_body(String& r_body);

	String next_directive(Tokenizer* p_tokenizer, const Vector<String>& p_directives);
	void add_to_output(const String& p_str);
	void set_error(const String& p_error, int p_line);

	static Define* create_define(const String& p_body);
	void insert_builtin_define(String p_name, String p_value, State& p_state);

	void clear_state();

	Error preprocess(State* p_state, const String& p_code, String& r_result);

public:
	static void get_keyword_list(List<String>* r_keywords, bool p_include_shader_keywords,
		bool p_ignore_context_keywords = false);
	static void get_pragma_list(List<String>* r_pragmas);

	ShaderPreprocessor() = default;
	~ShaderPreprocessor() = default;
};



// --- End servers/rendering/shader_preprocessor.h ---

// --- Folded from servers/rendering/shader_types.h ---
#include "scene/resources/texture.h"


class ShaderTypes
{
public:
    ShaderTypes() = delete;
    ~ShaderTypes() = delete;

    struct Type
    {
        HashMap<String, ShaderLanguage::FunctionInfo> functions;
        Vector<ShaderLanguage::ModeInfo> modes;
        Vector<ShaderLanguage::ModeInfo> stencil_modes;
    };

    static inline HashMap<RSE::ShaderMode, Type> shader_modes;
    static inline HashSet<String> shader_types;
    static inline List<String> shader_types_list;

    static const HashMap<String, ShaderLanguage::FunctionInfo>& get_functions(
        RSE::ShaderMode p_mode);
    static const Vector<ShaderLanguage::ModeInfo>& get_modes(RSE::ShaderMode p_mode);
    static const Vector<ShaderLanguage::ModeInfo>& get_stencil_modes(RSE::ShaderMode p_mode);
    static const HashSet<String>& get_types();
    static const List<String>& get_types_list();

    static void initialize();
    static void finalize();
};


// --- End servers/rendering/shader_types.h ---

// --- Folded from servers/rendering/shader_include_db.h ---
#include "core/types.h"

class ShaderIncludeDB
{
private:
	static HashMap<String, String> built_in_includes;

protected:

public:
	static void register_built_in_include_file(
		const String& p_filename, const String& p_shader_code);
	static PackedStringArray list_built_in_include_files();
	static bool has_built_in_include_file(const String& p_filename);
	static String get_built_in_include_file(const String& p_filename);
	static String parse_include_files(const String& p_code);
};



// --- End servers/rendering/shader_include_db.h ---
// --- End servers/rendering/shader_compiler.h ---
// --- End servers/rendering/shader_language.h ---

// --- Folded from servers/rendering/dummy/rasterizer_dummy.h ---
#include "core/types.h"
#include "core/math/plane.h"
#include "core/types.h"
#include "core/templates/hashfuncs.h"
#include "core/types.h"

#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"



class DependencyTracker;
class Image;

namespace RendererDummy
{

class Fog final
{
public:
	Fog() = delete;
	~Fog() = delete;
	Fog(const Fog&) = delete;
	Fog& operator=(const Fog&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* FOG VOLUMES */

	static bool owns_fog_volume(RID p_rid) { return false; }

	static RID fog_volume_allocate() { return RID(); }

	static void fog_volume_initialize(RID p_rid) {}

	static void fog_volume_free(RID p_rid) {}

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape) {}

	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size) {}

	static void fog_volume_set_material(RID p_fog_volume, RID p_material) {}

	static AABB fog_volume_get_aabb(RID p_fog_volume) { return AABB(); }

	static RSE::FogVolumeShape fog_volume_get_shape(RID p_fog_volume)
	{
		return RSE::FOG_VOLUME_SHAPE_ELLIPSOID;
	}
};

class GI final
{
public:
	GI() = delete;
	~GI() = delete;
	GI(const GI&) = delete;
	GI& operator=(const GI&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* VOXEL GI API */

	static bool owns_voxel_gi(RID p_rid) { return false; }

	static RID voxel_gi_allocate() { return RID(); }

	static void voxel_gi_free(RID p_rid) {}

	static void voxel_gi_initialize(RID p_rid) {}

	static void voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts)
	{
	}

	static AABB voxel_gi_get_bounds(RID p_voxel_gi) { return AABB(); }

	static Vector3i voxel_gi_get_octree_size(RID p_voxel_gi) { return Vector3i(); }

	static Vector<uint8_t> voxel_gi_get_octree_cells(RID p_voxel_gi) { return Vector<uint8_t>(); }

	static Vector<uint8_t> voxel_gi_get_data_cells(RID p_voxel_gi) { return Vector<uint8_t>(); }

	static Vector<uint8_t> voxel_gi_get_distance_field(RID p_voxel_gi) { return Vector<uint8_t>(); }

	static Vector<int> voxel_gi_get_level_counts(RID p_voxel_gi) { return Vector<int>(); }

	static Transform3D voxel_gi_get_to_cell_xform(RID p_voxel_gi) { return Transform3D(); }

	static void voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_dynamic_range(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_propagation(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_propagation(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_energy(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_energy(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure) {}

	static float voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi) { return 1.0f; }

	static void voxel_gi_set_bias(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_bias(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_normal_bias(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable) {}

	static bool voxel_gi_is_interior(RID p_voxel_gi) { return false; }

	static void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable) {}

	static bool voxel_gi_is_using_two_bounces(RID p_voxel_gi) { return false; }

	static uint32_t voxel_gi_get_version(RID p_voxel_gi) { return 0; }

	static void sdfgi_reset() {}
};

class LightStorage final
{
public:
	struct Lightmap
	{
		// dummy lightmap, no data
	};

	struct LightmapInstance
	{
		RID lightmap;
	};

private:
	struct Data
	{
		RID_Owner<Lightmap, true> lightmap_owner;
		RID_Owner<LightmapInstance> lightmap_instance_owner;
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			initialize();
		}
		return data;
	}

public:
	LightStorage() = delete;
	LightStorage(const LightStorage&) = delete;
	LightStorage& operator=(const LightStorage&) = delete;
	~LightStorage() = delete;

	static void initialize();
	static void finalize();
	static bool free(RID p_rid);

	/* Light API */
	static RID directional_light_allocate() { return RID(); }

	static void directional_light_initialize(RID p_rid) {}

	static RID omni_light_allocate() { return RID(); }

	static void omni_light_initialize(RID p_rid) {}

	static RID spot_light_allocate() { return RID(); }

	static void spot_light_initialize(RID p_rid) {}

	static RID area_light_allocate() { return RID(); }

	static void area_light_initialize(RID p_rid) {}

	static void light_free(RID p_rid) {}

	static void light_set_color(RID p_light, const Color& p_color) {}

	static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value) {}

	static void light_set_shadow(RID p_light, bool p_enabled) {}

	static void light_set_projector(RID p_light, RID p_texture) {}

	static void light_set_negative(RID p_light, bool p_enable) {}

	static void light_set_cull_mask(RID p_light, uint32_t p_mask) {}

	static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
	{
	}

	static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled) {}

	static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask) {}

	static uint32_t light_get_shadow_caster_mask(RID p_light) { return 0xFFFFFFFF; }

	static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode) {}

	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade) {}

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode) {}

	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode)
	{
	}

	static void light_directional_set_blend_splits(RID p_light, bool p_enable) {}

	static bool light_directional_get_blend_splits(RID p_light) { return false; }

	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode) {}

	static RSE::LightDirectionalSkyMode light_directional_get_sky_mode(RID p_light)
	{
		return RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_AND_SKY;
	}

	static RSE::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light)
	{
		return RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL;
	}

	static RSE::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light)
	{
		return RSE::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID;
	}

	static void light_area_set_size(RID p_light, const Vector2& p_size) {}

	static Vector2 light_area_get_size(RID p_light) { return Vector2(); }

	static void light_area_set_normalize_energy(RID p_light, bool p_enabled) {}

	static bool light_area_get_normalize_energy(RID p_light) { return true; }

	static void light_area_set_texture(RID p_light, RID p_texture) {}

	static RID light_area_get_texture(RID p_light) { return RID(); }

	static bool light_has_shadow(RID p_light) { return false; }

	static bool light_has_projector(RID p_light) { return false; }

	static RSE::LightType light_get_type(RID p_light) { return RSE::LIGHT_OMNI; }

	static AABB light_get_aabb(RID p_light) { return AABB(); }

	static float light_get_param(RID p_light, RSE::LightParam p_param) { return 0.0f; }

	static Color light_get_color(RID p_light) { return Color(); }

	static bool light_get_reverse_cull_face_mode(RID p_light) { return false; }

	static RSE::LightBakeMode light_get_bake_mode(RID p_light) { return RSE::LIGHT_BAKE_DISABLED; }

	static uint32_t light_get_max_sdfgi_cascade(RID p_light) { return 0; }

	static uint64_t light_get_version(RID p_light) { return 0; }

	static uint32_t light_get_cull_mask(RID p_light) { return 0; }

	/* LIGHT INSTANCE API */
	static RID light_instance_create(RID p_light) { return RID(); }

	static void light_instance_free(RID p_light) {}

	static void light_instance_set_transform(RID p_light_instance, const Transform3D& p_transform)
	{
	}

	static void light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb) {}

	static void light_instance_set_shadow_transform(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale = 1.0, float p_range_begin = 0,
		const Vector2& p_uv_scale = Vector2())
	{
	}

	static void light_instance_mark_visible(RID p_light_instance) {}

	static bool light_instance_is_shadow_visible_at_position(
		RID p_light_instance, const Vector3& p_position)
	{
		return false;
	}

	/* PROBE API */
	static RID reflection_probe_allocate() { return RID(); }

	static void reflection_probe_initialize(RID p_rid) {}

	static void reflection_probe_free(RID p_rid) {}

	static void reflection_probe_set_update_mode(RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
	{
	}

	static void reflection_probe_set_intensity(RID p_probe, float p_intensity) {}

	static void reflection_probe_set_blend_distance(RID p_probe, float p_blend_distance) {}

	static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode)
	{
	}

	static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color) {}

	static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy) {}

	static void reflection_probe_set_max_distance(RID p_probe, float p_distance) {}

	static void reflection_probe_set_size(RID p_probe, const Vector3& p_size) {}

	static void reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset) {}

	static void reflection_probe_set_as_interior(RID p_probe, bool p_enable) {}

	static void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable) {}

	static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable) {}

	static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers) {}

	static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers) {}

	static void reflection_probe_set_resolution(RID p_probe, int p_resolution) {}

	static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio) {}

	static float reflection_probe_get_mesh_lod_threshold(RID p_probe) { return 0.0f; }

	static AABB reflection_probe_get_aabb(RID p_probe) { return AABB(); }

	static RSE::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe)
	{
		return RSE::REFLECTION_PROBE_UPDATE_ONCE;
	}

	static uint32_t reflection_probe_get_cull_mask(RID p_probe) { return 0; }

	static uint32_t reflection_probe_get_reflection_mask(RID p_probe) { return 0; }

	static Vector3 reflection_probe_get_size(RID p_probe) { return Vector3(); }

	static Vector3 reflection_probe_get_origin_offset(RID p_probe) { return Vector3(); }

	static float reflection_probe_get_origin_max_distance(RID p_probe) { return 0.0f; }

	static bool reflection_probe_renders_shadows(RID p_probe) { return false; }

	/* REFLECTION ATLAS */
	static RID reflection_atlas_create() { return RID(); }

	static void reflection_atlas_free(RID p_ref_atlas) {}

	static int reflection_atlas_get_size(RID p_ref_atlas) { return 0; }

	static void reflection_atlas_set_size(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count)
	{
	}

	/* REFLECTION PROBE INSTANCE */
	static RID reflection_probe_instance_create(RID p_probe) { return RID(); }

	static void reflection_probe_instance_free(RID p_instance) {}

	static void reflection_probe_instance_set_transform(
		RID p_instance, const Transform3D& p_transform)
	{
	}

	static bool reflection_probe_has_atlas_index(RID p_instance) { return false; }

	static void reflection_probe_release_atlas_index(RID p_instance) {}

	static bool reflection_probe_instance_needs_redraw(RID p_instance) { return false; }

	static bool reflection_probe_instance_has_reflection(RID p_instance) { return false; }

	static bool reflection_probe_instance_begin_render(RID p_instance, RID p_reflection_atlas)
	{
		return false;
	}

	static bool reflection_probe_instance_end_render(RID p_instance, RID p_reflection_atlas)
	{
		return false;
	}

	static Ref<RenderSceneBuffers> reflection_probe_atlas_get_render_buffers(RID p_reflection_atlas)
	{
		return Ref<RenderSceneBuffers>();
	}

	static bool reflection_probe_instance_postprocess_step(RID p_instance) { return true; }

	/* LIGHTMAP CAPTURE */
	static bool owns_lightmap(RID p_rid) { return data && data->lightmap_owner.owns(p_rid); }

	static RID lightmap_allocate();
	static void lightmap_initialize(RID p_rid);
	static void lightmap_free(RID p_rid);

	static void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics)
	{
	}

	static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds) {}

	static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior) {}

	static void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree)
	{
	}

	static void lightmap_set_baked_exposure_normalization(RID p_lightmap, float p_exposure) {}

	static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap)
	{
		return PackedVector3Array();
	}

	static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap)
	{
		return PackedColorArray();
	}

	static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
	{
		return PackedInt32Array();
	}

	static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
	{
		return PackedInt32Array();
	}

	static AABB lightmap_get_aabb(RID p_lightmap) { return AABB(); }

	static void lightmap_tap_sh_light(RID p_lightmap, const Vector3& p_point, Color* r_sh) {}

	static bool lightmap_is_interior(RID p_lightmap) { return false; }

	static void lightmap_set_probe_capture_update_speed(float p_speed) {}

	static float lightmap_get_probe_capture_update_speed() { return 0.0f; }

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow) {}

	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap)
	{
		return RSE::SHADOWMASK_MODE_NONE;
	}

	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode) {}

	/* LIGHTMAP INSTANCE */
	static bool owns_lightmap_instance(RID p_rid)
	{
		return data && data->lightmap_instance_owner.owns(p_rid);
	}

	static RID lightmap_instance_create(RID p_lightmap);
	static void lightmap_instance_free(RID p_lightmap);

	static void lightmap_instance_set_transform(RID p_lightmap, const Transform3D& p_transform) {}

	/* SHADOW ATLAS API */
	static RID shadow_atlas_create() { return RID(); }

	static void shadow_atlas_free(RID p_atlas) {}

	static void shadow_atlas_set_size(RID p_atlas, int p_size, bool p_16_bits = true) {}

	static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision)
	{
	}

	static bool shadow_atlas_update_light(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version)
	{
		return false;
	}

	static void shadow_atlas_update(RID p_atlas) {}

	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true) {}

	static int get_directional_light_shadow_size(RID p_light_instance) { return 0; }

	static void set_directional_shadow_count(int p_count) {}
};

class MaterialStorage final
{
private:
	struct DummyShader
	{
		HashMap<String, ShaderLanguage::ShaderNode::Uniform> uniforms;
	};

	struct DummyMaterial
	{
		RID shader;
		RID next_pass;
	};

	struct Data
	{
		HashMap<String, RSE::GlobalShaderParameterType> global_shader_variables;
		RID_Owner<DummyShader> shader_owner;
		ShaderCompiler* dummy_compiler = nullptr;
		HashSet<RID> dummy_embedded_set;
		RID_Owner<DummyMaterial> material_owner;
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			initialize();
		}
		return data;
	}

public:
	MaterialStorage() = delete;
	MaterialStorage(const MaterialStorage&) = delete;
	MaterialStorage& operator=(const MaterialStorage&) = delete;
	~MaterialStorage() = delete;

	static void initialize();
	static void finalize();

	/* GLOBAL SHADER UNIFORM API */
	static void global_shader_parameter_remove(const String& p_name);
	static Vector<String> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(const String& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures = true);

	static void global_shader_parameters_clear() {}

	static int32_t global_shader_parameters_instance_allocate(RID p_instance) { return 0; }

	static void global_shader_parameters_instance_free(RID p_instance) {}

	/* SHADER API */
	static bool owns_shader(RID p_rid) { return data && data->shader_owner.owns(p_rid); }

	static RID shader_allocate();
	static void shader_initialize(RID p_rid, bool p_embedded = true);
	static void shader_free(RID p_rid);

	static void shader_set_code(RID p_shader, const String& p_code);

	static void shader_set_path_hint(RID p_shader, const String& p_code) {}

	static String shader_get_code(RID p_shader) { return ""; }

	static void shader_set_default_texture_parameter(
		RID p_shader, const String& p_name, RID p_texture, int p_index)
	{
	}

	static RID shader_get_default_texture_parameter(
		RID p_shader, const String& p_name, int p_index)
	{
		return RID();
	}

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader)
	{
		return RenderingServerTypes::ShaderNativeSourceCode();
	}

	static void shader_embedded_set_lock() {}

	static const HashSet<RID>& shader_embedded_set_get()
	{
		return _get_data()->dummy_embedded_set;
	}

	static void shader_embedded_set_unlock() {}

	/* MATERIAL API */
	static bool owns_material(RID p_rid) { return data && data->material_owner.owns(p_rid); }

	static RID material_allocate();
	static void material_initialize(RID p_rid);
	static void material_free(RID p_rid);

	static void material_set_render_priority(RID p_material, int priority) {}

	static void material_set_shader(RID p_shader_material, RID p_shader);

	static void material_set_next_pass(RID p_material, RID p_next_material);

	static bool material_is_animated(RID p_material) { return false; }

	static bool material_casts_shadows(RID p_material) { return false; }

	static RSE::CullMode material_get_cull_mode(RID p_material) { return RSE::CULL_MODE_DISABLED; }

	static void material_get_instance_shader_parameters(
		RID p_material, List<InstanceShaderParam>* r_parameters);

	static void material_update_dependency(RID p_material, DependencyTracker* p_instance) {}
};

struct DummyMesh
{
	Vector<RenderingServerTypes::SurfaceData> surfaces;
	int blend_shape_count = 0;
	RSE::BlendShapeMode blend_shape_mode = RSE::BLEND_SHAPE_MODE_NORMALIZED;
	PackedFloat32Array blend_shape_values;
	Dependency dependency;
};

class MeshStorage final
{
private:
	struct DummyMultiMesh
	{
		PackedFloat32Array buffer;
	};

	struct Data
	{
		RID_Owner<DummyMesh> mesh_owner;
		RID_Owner<DummyMultiMesh> multimesh_owner;
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			data = memnew(Data);
		}
		return data;
	}

public:
	MeshStorage() = delete;
	MeshStorage(const MeshStorage&) = delete;
	MeshStorage& operator=(const MeshStorage&) = delete;
	~MeshStorage() = delete;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	/* MESH API */
	static _FORCE_INLINE_ DummyMesh* get_mesh(RID p_rid)
	{
		return data ? data->mesh_owner.get_or_null(p_rid) : nullptr;
	}
	static bool owns_mesh(RID p_rid) { return data && data->mesh_owner.owns(p_rid); }

	static RID mesh_allocate() { return _get_data()->mesh_owner.allocate_rid(); }
	static void mesh_initialize(RID p_rid)
	{
		_get_data()->mesh_owner.initialize_rid(p_rid, DummyMesh());
	}
	static void mesh_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->mesh_owner.free(p_rid);
	}

	static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->blend_shape_count = p_blend_shape_count;
	}

	static bool mesh_needs_instance(RID p_mesh, bool p_has_skeleton) { return false; }

	static void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->surfaces.push_back(p_surface);
	}

	static int mesh_get_blend_shape_count(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, 0);
		return m->blend_shape_count;
	}

	static void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->blend_shape_mode = p_mode;
	}

	static RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, RSE::BLEND_SHAPE_MODE_NORMALIZED);
		return m->blend_shape_mode;
	}

	static void mesh_surface_update_vertex_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}
	static void mesh_surface_update_attribute_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}
	static void mesh_surface_update_skin_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}
	static void mesh_surface_update_index_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}

	static void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		ERR_FAIL_INDEX(p_surface, m->surfaces.size());
		m->surfaces.write[p_surface].material = p_material;
	}

	static RID mesh_surface_get_material(RID p_mesh, int p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, RID());
		ERR_FAIL_INDEX_V(p_surface, m->surfaces.size(), RID());
		return m->surfaces[p_surface].material;
	}

	static RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, RenderingServerTypes::SurfaceData());
		ERR_FAIL_INDEX_V(p_surface, m->surfaces.size(), RenderingServerTypes::SurfaceData());
		return m->surfaces[p_surface];
	}

	static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
	static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
	static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
	static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }

	static int mesh_get_surface_count(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, 0);
		return m->surfaces.size();
	}

	static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb) {}
	static AABB mesh_get_custom_aabb(RID p_mesh) { return AABB(); }
	static AABB mesh_get_aabb(RID p_mesh, RID p_skeleton) { return AABB(); }

	static void mesh_set_path(RID p_mesh, const String& p_path) {}
	static String mesh_get_path(RID p_mesh) { return String(); }
	static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh) {}

	static void mesh_clear(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->surfaces.clear();
	}

	static void mesh_surface_remove(RID p_mesh, int p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		ERR_FAIL_INDEX(p_surface, m->surfaces.size());
		m->surfaces.remove_at(p_surface);
	}

	static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info) {}

	/* MESH INSTANCE API */

	static RID mesh_instance_create(RID p_base) { return RID(); }
	static void mesh_instance_free(RID p_rid) {}
	static void mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton) {}
	static void mesh_instance_set_blend_shape_weight(
		RID p_mesh_instance, int p_shape, float p_weight)
	{
	}
	static void mesh_instance_check_for_update(RID p_mesh_instance) {}
	static void mesh_instance_set_canvas_item_transform(
		RID p_mesh_instance, const Transform2D& p_transform)
	{
	}
	static void update_mesh_instances() {}

	/* MULTIMESH API */
	static _FORCE_INLINE_ void multimesh_free(RID p_rid) { _multimesh_free(p_rid); }
	static bool owns_multimesh(RID p_rid) { return data && data->multimesh_owner.owns(p_rid); }

	static RID _multimesh_allocate() { return _get_data()->multimesh_owner.allocate_rid(); }
	static void _multimesh_initialize(RID p_rid)
	{
		_get_data()->multimesh_owner.initialize_rid(p_rid, DummyMultiMesh());
	}
	static void _multimesh_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->multimesh_owner.free(p_rid);
	}

	static void _multimesh_allocate_data(RID p_multimesh, int p_instances,
		RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
		bool p_use_custom_data = false, bool p_use_indirect = false)
	{
	}
	static int _multimesh_get_instance_count(RID p_multimesh) { return 0; }

	static void _multimesh_set_mesh(RID p_multimesh, RID p_mesh) {}
	static void _multimesh_instance_set_transform(
		RID p_multimesh, int p_index, const Transform3D& p_transform)
	{
	}
	static void _multimesh_instance_set_transform_2d(
		RID p_multimesh, int p_index, const Transform2D& p_transform)
	{
	}
	static void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color) {}
	static void _multimesh_instance_set_custom_data(
		RID p_multimesh, int p_index, const Color& p_color)
	{
	}

	static void _multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb) {}
	static AABB _multimesh_get_custom_aabb(RID p_multimesh) { return AABB(); }
	static RID _multimesh_get_mesh(RID p_multimesh) { return RID(); }

	static Transform3D _multimesh_instance_get_transform(RID p_multimesh, int p_index)
	{
		return Transform3D();
	}
	static Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index)
	{
		return Transform2D();
	}
	static Color _multimesh_instance_get_color(RID p_multimesh, int p_index) { return Color(); }
	static Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
	{
		return Color();
	}

	static void _multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer) {}
	static RID _multimesh_get_command_buffer_rd_rid(RID p_multimesh) { return RID(); }
	static RID _multimesh_get_buffer_rd_rid(RID p_multimesh) { return RID(); }
	static Vector<float> _multimesh_get_buffer(RID p_multimesh) { return Vector<float>(); }

	static void _multimesh_set_visible_instances(RID p_multimesh, int p_visible) {}
	static int _multimesh_get_visible_instances(RID p_multimesh) { return 0; }

	static AABB _multimesh_get_aabb(RID p_multimesh) { return AABB(); }

	static RendererMeshStorage::MultiMeshInterpolator* _multimesh_get_interpolator(RID p_multimesh)
	{
		return nullptr;
	}

	/* SKELETON API */

	static RID skeleton_allocate() { return RID(); }
	static void skeleton_initialize(RID p_rid) {}
	static void skeleton_free(RID p_rid) {}

	static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false) {}
	static void skeleton_set_base_transform_2d(
		RID p_skeleton, const Transform2D& p_base_transform)
	{
	}
	static int skeleton_get_bone_count(RID p_skeleton) { return 0; }
	static void skeleton_bone_set_transform(
		RID p_skeleton, int p_bone, const Transform3D& p_transform)
	{
	}
	static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone)
	{
		return Transform3D();
	}
	static void skeleton_bone_set_transform_2d(
		RID p_skeleton, int p_bone, const Transform2D& p_transform)
	{
	}
	static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone)
	{
		return Transform2D();
	}

	static void skeleton_update_dependency(RID p_base, DependencyTracker* p_instance) {}
};

class ParticlesStorage final
{
public:
	ParticlesStorage() = delete;
	~ParticlesStorage() = delete;
	ParticlesStorage(const ParticlesStorage&) = delete;
	ParticlesStorage& operator=(const ParticlesStorage&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* PARTICLES */

	static bool owns_particles(RID p_rid) { return false; }

	static RID particles_allocate() { return RID(); }

	static void particles_initialize(RID p_rid) {}

	static void particles_free(RID p_rid) {}

	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode) {}

	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags)
	{
	}

	static void particles_set_emitting(RID p_particles, bool p_emitting) {}

	static void particles_set_amount(RID p_particles, int p_amount) {}

	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio) {}

	static void particles_set_lifetime(RID p_particles, double p_lifetime) {}

	static void particles_set_one_shot(RID p_particles, bool p_one_shot) {}

	static void particles_set_pre_process_time(RID p_particles, double p_time) {}

	static void particles_request_process_time(
		RID p_particles, real_t p_request_process_time, real_t p_particles_request_process_time)
	{
	}

	static void particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio) {}

	static void particles_set_randomness_ratio(RID p_particles, real_t p_ratio) {}

	static void particles_set_seed(RID p_particles, uint32_t p_seed) {}

	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb) {}

	static void particles_set_speed_scale(RID p_particles, double p_scale) {}

	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable) {}

	static void particles_set_process_material(RID p_particles, RID p_material) {}

	static RID particles_get_process_material(RID p_particles) { return RID(); }

	static void particles_set_fixed_fps(RID p_particles, int p_fps) {}

	static void particles_set_interpolate(RID p_particles, bool p_enable) {}

	static void particles_set_fractional_delta(RID p_particles, bool p_enable) {}

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles) {}

	static void particles_set_view_axis(
		RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis)
	{
	}

	static void particles_set_collision_base_size(RID p_particles, real_t p_size) {}

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align)
	{
	}

	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter)
	{
	}

	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis)
	{
	}

	static void particles_set_trails(RID p_particles, bool p_enable, double p_length) {}

	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses)
	{
	}

	static void particles_restart(RID p_particles) {}

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order) {}

	static void particles_set_draw_passes(RID p_particles, int p_count) {}

	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh) {}

	static void particles_request_process(RID p_particles) {}

	static AABB particles_get_current_aabb(RID p_particles) { return AABB(); }

	static AABB particles_get_aabb(RID p_particles) { return AABB(); }

	static void particles_set_emission_transform(RID p_particles, const Transform3D& p_transform) {}

	static void particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity) {}

	static void particles_set_interp_to_end(RID p_particles, float p_interp) {}

	static bool particles_get_emitting(RID p_particles) { return false; }

	static int particles_get_draw_passes(RID p_particles) { return 0; }

	static RID particles_get_draw_pass_mesh(RID p_particles, int p_pass) { return RID(); }

	static void particles_add_collision(RID p_particles, RID p_instance) {}

	static void particles_remove_collision(RID p_particles, RID p_instance) {}

	static void update_particles() {}

	/* PARTICLES COLLISION */

	static bool owns_particles_collision(RID p_rid) { return false; }

	static RID particles_collision_allocate() { return RID(); }

	static void particles_collision_initialize(RID p_rid) {}

	static void particles_collision_free(RID p_rid) {}

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type)
	{
	}

	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask)
	{
	}

	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius) {}

	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents)
	{
	}

	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength)
	{
	}

	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality)
	{
	}

	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve)
	{
	}

	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture) {}

	static void particles_collision_height_field_update(RID p_particles_collision) {}

	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution)
	{
	}

	static AABB particles_collision_get_aabb(RID p_particles_collision) { return AABB(); }

	static bool particles_collision_is_heightfield(RID p_particles_collision) { return false; }

	static uint32_t particles_collision_get_height_field_mask(RID p_particles_collision)
	{
		return 0;
	}

	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask)
	{
	}

	static uint32_t particles_collision_get_cull_mask(RID p_particles_collision) { return 0; }

	static bool owns_particles_collision_instance(RID p_rid) { return false; }

	static RID particles_collision_instance_create(RID p_collision) { return RID(); }

	static void particles_collision_instance_free(RID p_rid) {}

	static void particles_collision_instance_set_transform(
		RID p_collision_instance, const Transform3D& p_transform)
	{
	}

	static void particles_collision_instance_set_active(RID p_collision_instance, bool p_active) {}

	static bool particles_is_inactive(RID p_particles) { return false; }
};

struct DummyTexture
{
	String path;
	int width = 0;
	int height = 0;
	int depth = 0;
	int layers = 0;
	Image::Format format = Image::FORMAT_L8;
	Ref<Image> image;
};

struct DummyRenderTarget
{
	Point2i position;
	Size2i size;
	RID texture;
	bool direct_to_screen = false;
	bool is_transparent = false;
	bool was_used = false;
	bool clear_requested = false;
	Color clear_color;
	RSE::ViewportMSAA msaa = RSE::VIEWPORT_MSAA_DISABLED;
	bool use_hdr = false;
};

struct DummyCanvasTexture
{
	RID diffuse;
	RID normal_map;
	RID specular;
	Color specular_color = Color(1, 1, 1, 1);
	real_t shininess = 1.0;
	RSE::CanvasItemTextureFilter filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
	RSE::CanvasItemTextureRepeat repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
};

class TextureStorage final
{
private:
	struct Data
	{
		RID_Owner<DummyTexture> texture_owner;
		RID_Owner<DummyRenderTarget> render_target_owner;
		RID_Owner<DummyCanvasTexture> canvas_texture_owner;
		Color default_clear_color = Color(0, 0, 0, 1);
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			data = memnew(Data);
		}
		return data;
	}

public:
	TextureStorage() = delete;
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;
	~TextureStorage() = delete;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	static bool owns_texture(RID p_rid) { return data && data->texture_owner.owns(p_rid); }
	static bool owns_render_target(RID p_rid)
	{
		return data && data->render_target_owner.owns(p_rid);
	}
	static bool owns_canvas_texture(RID p_rid)
	{
		return data && data->canvas_texture_owner.owns(p_rid);
	}

	/* CANVAS TEXTURE API */

	static RID canvas_texture_allocate() { return _get_data()->canvas_texture_owner.allocate_rid(); }

	static void canvas_texture_initialize(RID p_rid)
	{
		_get_data()->canvas_texture_owner.initialize_rid(p_rid, DummyCanvasTexture());
	}

	static void canvas_texture_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->canvas_texture_owner.free(p_rid);
	}

	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		switch (p_channel) {
		case RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE:
			ct->diffuse = p_texture;
			break;
		case RSE::CANVAS_TEXTURE_CHANNEL_NORMAL:
			ct->normal_map = p_texture;
			break;
		case RSE::CANVAS_TEXTURE_CHANNEL_SPECULAR:
			ct->specular = p_texture;
			break;
		}
	}

	static void canvas_texture_set_shading_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->filter = p_filter;
	}

	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->specular_color = p_base_color;
		ct->shininess = p_shininess;
	}

	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->filter = p_filter;
	}

	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->repeat = p_repeat;
	}

	/* TEXTURE 2D / GENERAL API */

	static RID texture_allocate() { return _get_data()->texture_owner.allocate_rid(); }

	static void texture_initialize(RID p_rid)
	{
		_get_data()->texture_owner.initialize_rid(p_rid, DummyTexture());
	}

	static void texture_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->texture_owner.free(p_rid);
	}

	static RID texture_2d_create(const Ref<Image>& p_image)
	{
		RID rid = _get_data()->texture_owner.allocate_rid();
		texture_2d_initialize(rid, p_image);
		return rid;
	}

	static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
	{
		DummyTexture t;
		if (p_image.is_valid()) {
			t.image = p_image;
			t.width = p_image->get_width();
			t.height = p_image->get_height();
			t.format = p_image->get_format();
		}
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->image = p_image;
	}

	static Ref<Image> texture_2d_get(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, Ref<Image>());
		return t->image;
	}

	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer)
	{
		return texture_2d_get(p_texture);
	}

	static void texture_2d_placeholder_initialize(RID p_texture) { texture_initialize(p_texture); }

	static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
	{
		DummyTexture t;
		t.layers = p_layers.size();
		if (!p_layers.is_empty() && p_layers[0].is_valid()) {
			t.image = p_layers[0];
			t.width = p_layers[0]->get_width();
			t.height = p_layers[0]->get_height();
			t.format = p_layers[0]->get_format();
		}
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type)
	{
		texture_initialize(p_texture);
	}

	static void texture_3d_initialize(RID p_texture, Image::Format p_format, int p_width,
		int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		t.depth = p_depth;
		t.format = p_format;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data) {}

	static Vector<Ref<Image>> texture_3d_get(RID p_texture) { return Vector<Ref<Image>>(); }

	static void texture_3d_placeholder_initialize(RID p_texture) { texture_initialize(p_texture); }

	static void texture_proxy_initialize(RID p_texture, RID p_base)
	{
		texture_initialize(p_texture);
	}

	static void texture_proxy_update(RID p_proxy, RID p_base) {}

	static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_drawable_generate_mipmaps(RID p_texture) {}

	static RID texture_drawable_get_default_material() { return RID(); }

	static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
	}

	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb) { return 0; }

	static void texture_set_format(RID p_texture, Image::Format p_format)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->format = p_format;
	}

	static Image::Format texture_get_format(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, Image::FORMAT_L8);
		return t->format;
	}

	static void texture_set_width(RID p_texture, int p_width)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->width = p_width;
	}

	static int texture_get_width(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->width;
	}

	static void texture_set_height(RID p_texture, int p_height)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->height = p_height;
	}

	static int texture_get_height(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->height;
	}

	static void texture_set_depth(RID p_texture, int p_depth)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->depth = p_depth;
	}

	static int texture_get_depth(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->depth;
	}

	static void texture_set_layers(RID p_texture, int p_layers)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->layers = p_layers;
	}

	static int texture_get_layers(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->layers;
	}

	static void texture_set_path(RID p_texture, const String& p_path)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->path = p_path;
	}

	static String texture_get_path(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, String());
		return t->path;
	}

	static void texture_set_size_override(RID p_texture, int p_width, int p_height) {}

	static void texture_replace(RID p_texture, RID p_by_texture) {}

	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {}

	static Size2 texture_size_with_proxy(RID p_proxy)
	{
		return Size2(texture_get_width(p_proxy), texture_get_height(p_proxy));
	}

	/* DECAL API */

	static RID decal_allocate() { return RID(); }
	static void decal_initialize(RID p_rid) {}
	static void decal_free(RID p_rid) {}
	static void decal_set_size(RID p_decal, const Vector3& p_size) {}
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture) {}
	static void decal_set_emission_energy(RID p_decal, float p_energy) {}
	static void decal_set_albedo_mix(RID p_decal, float p_mix) {}
	static void decal_set_modulate(RID p_decal, const Color& p_modulate) {}
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers) {}
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length)
	{
	}
	static void decal_set_fade(RID p_decal, float p_above, float p_below) {}
	static void decal_set_normal_fade(RID p_decal, float p_fade) {}
	static AABB decal_get_aabb(RID p_decal) { return AABB(); }
	static uint32_t decal_get_cull_mask(RID p_decal) { return 0; }

	static RID decal_instance_create(RID p_decal) { return RID(); }
	static void decal_instance_free(RID p_decal_instance) {}
	static void decal_instance_set_transform(RID p_decal_instance, const Transform3D& p_transform)
	{
	}
	static void decal_instance_set_sorting_offset(RID p_decal_instance, float p_sorting_offset) {}

	/* RENDER TARGET API */

	static RID render_target_create()
	{
		Data* d = _get_data();
		RID rid = d->render_target_owner.allocate_rid();
		DummyRenderTarget rt;
		rt.texture = texture_allocate();
		texture_initialize(rt.texture);
		d->render_target_owner.initialize_rid(rid, rt);
		return rid;
	}

	static void render_target_free(RID p_render_target)
	{
		if (!data) {
			return;
		}
		DummyRenderTarget* rt = data->render_target_owner.get_or_null(p_render_target);
		if (rt) {
			if (rt->texture.is_valid()) {
				texture_free(rt->texture);
			}
			data->render_target_owner.free(p_render_target);
		}
	}

	static void render_target_set_position(RID p_render_target, int p_x, int p_y)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->position = Point2i(p_x, p_y);
	}

	static Point2i render_target_get_position(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Point2i());
		return rt->position;
	}

	static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->size = Size2i(p_width, p_height);
	}

	static Size2i render_target_get_size(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Size2i());
		return rt->size;
	}

	static void render_target_set_transparent(RID p_render_target, bool p_is_transparent)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->is_transparent = p_is_transparent;
	}

	static bool render_target_is_transparent(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->is_transparent;
	}

	static void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->direct_to_screen = p_direct_to_screen;
	}

	static bool render_target_is_direct_to_screen(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->direct_to_screen;
	}

	static RID render_target_get_texture(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, RID());
		return rt->texture;
	}

	static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->msaa = p_msaa;
	}

	static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_MSAA_DISABLED);
		return rt->msaa;
	}

	static bool render_target_get_msaa_needs_resolve(RID p_render_target) { return false; }

	static void render_target_do_msaa_resolve(RID p_render_target) {}

	static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->use_hdr = p_use_hdr;
	}

	static bool render_target_is_using_hdr(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->use_hdr;
	}

	static void render_target_set_use_debanding(RID p_render_target, bool p_use_debanding) {}

	static void render_target_set_vrs_mode(RID p_render_target, RSE::ViewportVRSMode p_mode) {}

	static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
	}

	static void render_target_set_vrs_texture(RID p_render_target, RID p_texture) {}

	static void render_target_request_clear(RID p_render_target, const Color& p_clear_color)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->clear_requested = true;
		rt->clear_color = p_clear_color;
	}

	static bool render_target_is_clear_requested(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->clear_requested;
	}

	static Color render_target_get_clear_request_color(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Color());
		return rt->clear_color;
	}

	static void render_target_do_clear_request(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->clear_requested = false;
	}

	static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
	{
	}

	static Rect2i render_target_get_sdf_rect(RID p_render_target) { return Rect2i(); }

	static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled) {}

	static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity, RID p_velocity_depth)
	{
	}

	static RID render_target_get_override_color(RID p_render_target) { return RID(); }
	static RID render_target_get_override_depth(RID p_render_target) { return RID(); }
	static RID render_target_get_override_velocity(RID p_render_target) { return RID(); }
	static RID render_target_get_override_velocity_depth(RID p_render_target) { return RID(); }

	static void render_target_set_render_region(RID p_render_target, const Rect2i& p_render_region)
	{
	}

	static Rect2i render_target_get_render_region(RID p_render_target) { return Rect2i(); }

	static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{
	}

	static Size2i render_target_get_velocity_target_size(RID p_render_target) { return Size2i(); }

	static bool render_target_was_used(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->was_used;
	}

	static void render_target_set_as_unused(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->was_used = false;
	}

	static void render_target_set_as_current(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->was_used = true;
	}

	static void set_default_clear_color(const Color& p_color)
	{
		_get_data()->default_clear_color = p_color;
	}

	static Color get_default_clear_color() { return _get_data()->default_clear_color; }
};

class Utilities final
{
public:
	Utilities() = delete;
	Utilities(const Utilities&) = delete;
	Utilities& operator=(const Utilities&) = delete;
	~Utilities() = delete;

	static void initialize() {}
	static void finalize() {}

	static RSE::InstanceType get_base_type(RID p_rid);
	static bool free(RID p_rid);

	static void base_update_dependency(RID p_base, DependencyTracker* p_instance);

	static RID visibility_notifier_allocate() { return RID(); }
	static void visibility_notifier_initialize(RID p_notifier) {}
	static void visibility_notifier_free(RID p_notifier) {}
	static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb) {}
	static AABB visibility_notifier_get_aabb(RID p_notifier) { return AABB(); }
	static void visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred) {}

	static void capture_timestamps_begin() {}
	static void capture_timestamp(const String& p_name) {}
	static uint32_t get_captured_timestamps_count() { return 0; }
	static uint64_t get_captured_timestamps_frame() { return 0; }
	static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index) { return 0; }
	static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index) { return 0; }
	static String get_captured_timestamp_name(uint32_t p_index) { return String(); }

	static void update_dirty_resources() {}
	static void set_debug_generate_wireframes(bool p_generate) {}

	static bool has_os_feature(const String& p_feature)
	{
		return p_feature == "rgtc" || p_feature == "bptc" || p_feature == "s3tc" ||
			   p_feature == "etc2";
	}

	static void update_memory_info() {}

	static uint64_t get_rendering_info(RSE::RenderingInfo p_info) { return 0; }

	static String get_video_adapter_name() { return String(); }
	static String get_video_adapter_vendor() { return String(); }
	static RenderingDeviceEnums::DeviceType get_video_adapter_type()
	{
		return RenderingDeviceEnums::DeviceType::DEVICE_TYPE_OTHER;
	}
	static String get_video_adapter_api_version() { return String(); }

	static Size2i get_maximum_viewport_size() { return Size2i(); }

	static uint32_t get_maximum_shader_varyings() { return 31; }

	static uint64_t get_maximum_uniform_buffer_size() { return 65536; }
};

} // namespace RendererDummy

class RasterizerCanvasDummy final
{
public:
	static void initialize() {}

	static void finalize() {}

	static void set_time(double p_time) {}

	static void update() {}

	static bool free(RID p_rid) { return false; }

	static PolygonID request_polygon(const Vector<int>& p_indices, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), int p_count = -1)
	{
		return 0;
	}

	static void free_polygon(PolygonID p_polygon) {}

	static void canvas_render_items(RID p_to_render_target, RCR::Item* p_item_list,
		const Color& p_modulate, RendererCanvasRender::Light* p_light_list,
		RendererCanvasRender::Light* p_directional_list, const Transform2D& p_canvas_transform,
		RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info = nullptr)
	{
	}

	static void set_debug_redraw(bool p_enabled, double p_time, const Color& p_color) {}

	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source) { return 0; }

	static RID light_create() { return RID(); }

	static void light_set_texture(RID p_rid, RID p_texture) {}

	static void light_set_use_shadow(RID p_rid, bool p_enable) {}

	static void light_update_shadow(RID p_rid, int p_shadow_index, const Transform2D& p_light_xform,
		int p_light_mask, float p_near, float p_far,
		RendererCanvasRender::LightOccluderInstance* p_occluders, const Rect2& p_light_rect)
	{
	}

	static void light_update_directional_shadow(RID p_rid, int p_shadow_index,
		const Transform2D& p_light_xform, int p_light_mask, float p_cull_distance,
		const Rect2& p_clip_rect, RendererCanvasRender::LightOccluderInstance* p_occluders)
	{
	}

	static void render_sdf(
		RID p_render_target, RendererCanvasRender::LightOccluderInstance* p_occluders)
	{
	}

	static RID occluder_polygon_create() { return RID(); }

	static void occluder_polygon_set_shape(
		RID p_occluder, const Vector<Vector2>& p_points, bool p_closed)
	{
	}

	static void occluder_polygon_set_cull_mode(
		RID p_occluder, RSE::CanvasOccluderPolygonCullMode p_mode)
	{
	}

	static void set_shadow_texture_size(int p_size) {}

	RasterizerCanvasDummy() = delete;
	~RasterizerCanvasDummy() = delete;
};

class RasterizerSceneDummy final : public RendererSceneRender
{
public:
	class GeometryInstanceDummy final : public RenderGeometryInstance
	{
	public:
		GeometryInstanceDummy() {}

		virtual void set_transform(const Transform3D& p_transform, const AABB& p_aabb,
			const AABB& p_transformed_aabb) override
		{
		}
	};

	PagedAllocator<GeometryInstanceDummy> geometry_instance_alloc;

public:
	RenderGeometryInstance* geometry_instance_create(RID p_base) override
	{
		RSE::InstanceType type = RendererDummy::Utilities::get_base_type(p_base);
		ERR_FAIL_COND_V(!((1 << type) & RSE::INSTANCE_GEOMETRY_MASK), nullptr);

		GeometryInstanceDummy* ginstance = geometry_instance_alloc.alloc();

		return ginstance;
	}

	void geometry_instance_free(RenderGeometryInstance* p_geometry_instance) override
	{
		GeometryInstanceDummy* ginstance = static_cast<GeometryInstanceDummy*>(p_geometry_instance);
		ERR_FAIL_NULL(ginstance);

		geometry_instance_alloc.free(ginstance);
	}

	uint32_t geometry_instance_get_pair_mask() override { return 0; }

	virtual uint32_t get_max_lights_total() override { return 0; }

	virtual uint32_t get_max_lights_per_mesh() override { return 0; }

	/* PIPELINES */

	virtual void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation) override {}

	virtual uint32_t get_pipeline_compilations(RSE::PipelineSource p_source) override { return 0; }

	/* SDFGI UPDATE */

	void sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_environment,
		const Vector3& p_world_position) override
	{
	}

	int sdfgi_get_pending_region_count(
		const Ref<RenderSceneBuffers>& p_render_buffers) const override
	{
		return 0;
	}

	AABB sdfgi_get_pending_region_bounds(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const override
	{
		return AABB();
	}

	uint32_t sdfgi_get_pending_region_cascade(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const override
	{
		return 0;
	}

	/* SKY API */

	RID sky_allocate() override { return RID(); }

	void sky_initialize(RID p_rid) override {}

	void sky_set_radiance_size(RID p_sky, int p_radiance_size) override {}

	void sky_set_mode(RID p_sky, RSE::SkyMode p_samples) override {}

	void sky_set_material(RID p_sky, RID p_material) override {}

	Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size) override
	{
		return Ref<Image>();
	}

	/* ENVIRONMENT API */

	void environment_glow_set_use_bicubic_upscale(bool p_enable) override {}

	void environment_set_ssr_half_size(bool p_half_size) override {}

	void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality) override
	{
	}

	void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to) override
	{
	}

	void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to) override
	{
	}

	void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count) override {}

	void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames) override
	{
	}

	void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update) override
	{
	}

	void environment_set_volumetric_fog_volume_size(int p_size, int p_depth) override {}

	void environment_set_volumetric_fog_filter_active(bool p_enable) override {}

	Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size) override
	{
		return Ref<Image>();
	}

	void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality) override {}

	void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality) override {}

	RID fog_volume_instance_create(RID p_fog_volume) override { return RID(); }

	void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform) override
	{
	}

	void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active) override {}

	RID fog_volume_instance_get_volume(RID p_fog_volume_instance) const override { return RID(); }

	Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance) const override
	{
		return Vector3();
	}

	RID voxel_gi_instance_create(RID p_voxel_gi) override { return RID(); }

	void voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform) override
	{
	}

	bool voxel_gi_needs_update(RID p_probe) const override { return false; }

	void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects) override
	{
	}

	void voxel_gi_set_quality(RSE::VoxelGIQuality) override {}

	void render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
		const CameraData* p_camera_data, const CameraData* p_prev_camera_data,
		const PagedArray<RenderGeometryInstance*>& p_instances, const PagedArray<RID>& p_lights,
		const PagedArray<RID>& p_reflection_probes, const PagedArray<RID>& p_voxel_gi_instances,
		const PagedArray<RID>& p_decals, const PagedArray<RID>& p_lightmaps,
		const PagedArray<RID>& p_fog_volumes, RID p_environment, RID p_camera_attributes,
		RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas,
		RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold,
		const RenderShadowData* p_render_shadows, int p_render_shadow_count,
		const RenderSDFGIData* p_render_sdfgi_regions, int p_render_sdfgi_region_count,
		float p_window_output_max_value, const RenderSDFGIUpdateData* p_sdfgi_update_data = nullptr,
		RenderingServerTypes::RenderInfo* r_info = nullptr) override
	{
	}

	void render_material(const Transform3D& p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region) override
	{
	}

	void render_particle_collider_heightfield(RID p_collider, const Transform3D& p_transform,
		const PagedArray<RenderGeometryInstance*>& p_instances) override
	{
	}

	void set_scene_pass(uint64_t p_pass) override {}

	void set_time(double p_time, double p_step) override {}

	void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw) override {}

	Ref<RenderSceneBuffers> render_buffers_create() override { return Ref<RenderSceneBuffers>(); }

	void gi_set_use_half_resolution(bool p_enable) override {}

	void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_curve) override
	{
	}

	bool screen_space_roughness_limiter_is_active() const { return false; }

	void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality) override {}

	void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale) override {}

	bool free(RID p_rid) override;

	void update() override {}

	void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir) override {}

	virtual void decals_set_filter(RSE::DecalFilter p_filter) override {}

	virtual void light_projectors_set_filter(RSE::LightProjectorFilter p_filter) override {}

	virtual void lightmaps_set_bicubic_filter(bool p_enable) override {}

	virtual void material_set_use_debanding(bool p_enable) override {}

	RasterizerSceneDummy() {}

	~RasterizerSceneDummy() {}
};

class RasterizerDummy final
{
private:
	static inline uint64_t frame = 1;
	static inline double delta = 0.0;
	static inline double time = 0.0;

	static inline RasterizerCanvasDummy* canvas = nullptr;
	static inline RasterizerSceneDummy* scene = nullptr;

	static inline RendererDummy::Fog* fog = nullptr;
	static inline RendererDummy::GI* gi = nullptr;
	static inline RendererDummy::LightStorage* light_storage = nullptr;
	static inline RendererDummy::MaterialStorage* material_storage = nullptr;
	static inline RendererDummy::MeshStorage* mesh_storage = nullptr;
	static inline RendererDummy::ParticlesStorage* particles_storage = nullptr;
	static inline RendererDummy::TextureStorage* texture_storage = nullptr;
	static inline RendererDummy::Utilities* utilities = nullptr;

public:
	static RendererSceneRender* get_scene();

	static RendererFog* get_fog();

	static RendererDummy::Utilities* get_utilities();

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true)
	{
	}

	static void initialize();

	static void begin_frame(double frame_step);

	static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
		const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
	{
	}

	_ALWAYS_INLINE_ static bool is_opengl() { return false; }

	_ALWAYS_INLINE_ static void gl_end_frame(bool p_swap_buffers) {}

	static void end_frame(bool p_present);

	static void finalize();

	static Error _create_current()
	{
		RasterizerDummy::initialize();
		return OK;
	}

	static void make_current();

	_ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }

	_ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }

	_ALWAYS_INLINE_ static double get_total_time() { return time; }

	_ALWAYS_INLINE_ static bool can_create_resources_async() { return false; }

public:
	static void bind_mesh_storage()
	{
		RendererMeshStorage::mesh_allocate = &RendererDummy::MeshStorage::mesh_allocate;
		RendererMeshStorage::mesh_initialize = &RendererDummy::MeshStorage::mesh_initialize;
		RendererMeshStorage::mesh_free = &RendererDummy::MeshStorage::mesh_free;
		RendererMeshStorage::mesh_set_blend_shape_count =
			&RendererDummy::MeshStorage::mesh_set_blend_shape_count;
		RendererMeshStorage::mesh_needs_instance = &RendererDummy::MeshStorage::mesh_needs_instance;
		RendererMeshStorage::mesh_add_surface = &RendererDummy::MeshStorage::mesh_add_surface;
		RendererMeshStorage::mesh_get_blend_shape_count =
			&RendererDummy::MeshStorage::mesh_get_blend_shape_count;
		RendererMeshStorage::mesh_set_blend_shape_mode =
			&RendererDummy::MeshStorage::mesh_set_blend_shape_mode;
		RendererMeshStorage::mesh_get_blend_shape_mode =
			&RendererDummy::MeshStorage::mesh_get_blend_shape_mode;
		RendererMeshStorage::mesh_surface_update_vertex_region =
			&RendererDummy::MeshStorage::mesh_surface_update_vertex_region;
		RendererMeshStorage::mesh_surface_update_attribute_region =
			&RendererDummy::MeshStorage::mesh_surface_update_attribute_region;
		RendererMeshStorage::mesh_surface_update_skin_region =
			&RendererDummy::MeshStorage::mesh_surface_update_skin_region;
		RendererMeshStorage::mesh_surface_update_index_region =
			&RendererDummy::MeshStorage::mesh_surface_update_index_region;
		RendererMeshStorage::mesh_surface_set_material =
			&RendererDummy::MeshStorage::mesh_surface_set_material;
		RendererMeshStorage::mesh_surface_get_material =
			&RendererDummy::MeshStorage::mesh_surface_get_material;
		RendererMeshStorage::mesh_get_surface = &RendererDummy::MeshStorage::mesh_get_surface;
		RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_vertex_buffer_rd_rid;
		RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_attribute_buffer_rd_rid;
		RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_skin_buffer_rd_rid;
		RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_index_buffer_rd_rid;
		RendererMeshStorage::mesh_get_surface_count =
			&RendererDummy::MeshStorage::mesh_get_surface_count;
		RendererMeshStorage::mesh_set_custom_aabb =
			&RendererDummy::MeshStorage::mesh_set_custom_aabb;
		RendererMeshStorage::mesh_get_custom_aabb =
			&RendererDummy::MeshStorage::mesh_get_custom_aabb;
		RendererMeshStorage::mesh_get_aabb = &RendererDummy::MeshStorage::mesh_get_aabb;
		RendererMeshStorage::mesh_set_path = &RendererDummy::MeshStorage::mesh_set_path;
		RendererMeshStorage::mesh_get_path = &RendererDummy::MeshStorage::mesh_get_path;
		RendererMeshStorage::mesh_set_shadow_mesh =
			&RendererDummy::MeshStorage::mesh_set_shadow_mesh;
		RendererMeshStorage::mesh_clear = &RendererDummy::MeshStorage::mesh_clear;
		RendererMeshStorage::mesh_surface_remove = &RendererDummy::MeshStorage::mesh_surface_remove;
		RendererMeshStorage::mesh_debug_usage = &RendererDummy::MeshStorage::mesh_debug_usage;

		RendererMeshStorage::mesh_instance_create =
			&RendererDummy::MeshStorage::mesh_instance_create;
		RendererMeshStorage::mesh_instance_free = &RendererDummy::MeshStorage::mesh_instance_free;
		RendererMeshStorage::mesh_instance_set_skeleton =
			&RendererDummy::MeshStorage::mesh_instance_set_skeleton;
		RendererMeshStorage::mesh_instance_set_blend_shape_weight =
			&RendererDummy::MeshStorage::mesh_instance_set_blend_shape_weight;
		RendererMeshStorage::mesh_instance_check_for_update =
			&RendererDummy::MeshStorage::mesh_instance_check_for_update;
		RendererMeshStorage::mesh_instance_set_canvas_item_transform =
			&RendererDummy::MeshStorage::mesh_instance_set_canvas_item_transform;
		RendererMeshStorage::update_mesh_instances =
			&RendererDummy::MeshStorage::update_mesh_instances;

		RendererMeshStorage::_multimesh_allocate = &RendererDummy::MeshStorage::_multimesh_allocate;
		RendererMeshStorage::_multimesh_initialize =
			&RendererDummy::MeshStorage::_multimesh_initialize;
		RendererMeshStorage::_multimesh_free = &RendererDummy::MeshStorage::_multimesh_free;
		RendererMeshStorage::_multimesh_allocate_data =
			&RendererDummy::MeshStorage::_multimesh_allocate_data;
		RendererMeshStorage::_multimesh_get_instance_count =
			&RendererDummy::MeshStorage::_multimesh_get_instance_count;
		RendererMeshStorage::_multimesh_set_mesh = &RendererDummy::MeshStorage::_multimesh_set_mesh;
		RendererMeshStorage::_multimesh_instance_set_transform =
			&RendererDummy::MeshStorage::_multimesh_instance_set_transform;
		RendererMeshStorage::_multimesh_instance_set_transform_2d =
			&RendererDummy::MeshStorage::_multimesh_instance_set_transform_2d;
		RendererMeshStorage::_multimesh_instance_set_color =
			&RendererDummy::MeshStorage::_multimesh_instance_set_color;
		RendererMeshStorage::_multimesh_instance_set_custom_data =
			&RendererDummy::MeshStorage::_multimesh_instance_set_custom_data;
		RendererMeshStorage::_multimesh_set_custom_aabb =
			&RendererDummy::MeshStorage::_multimesh_set_custom_aabb;
		RendererMeshStorage::_multimesh_get_custom_aabb =
			&RendererDummy::MeshStorage::_multimesh_get_custom_aabb;
		RendererMeshStorage::_multimesh_get_mesh = &RendererDummy::MeshStorage::_multimesh_get_mesh;
		RendererMeshStorage::_multimesh_instance_get_transform =
			&RendererDummy::MeshStorage::_multimesh_instance_get_transform;
		RendererMeshStorage::_multimesh_instance_get_transform_2d =
			&RendererDummy::MeshStorage::_multimesh_instance_get_transform_2d;
		RendererMeshStorage::_multimesh_instance_get_color =
			&RendererDummy::MeshStorage::_multimesh_instance_get_color;
		RendererMeshStorage::_multimesh_instance_get_custom_data =
			&RendererDummy::MeshStorage::_multimesh_instance_get_custom_data;
		RendererMeshStorage::_multimesh_set_buffer =
			&RendererDummy::MeshStorage::_multimesh_set_buffer;
		RendererMeshStorage::_multimesh_get_command_buffer_rd_rid =
			&RendererDummy::MeshStorage::_multimesh_get_command_buffer_rd_rid;
		RendererMeshStorage::_multimesh_get_buffer_rd_rid =
			&RendererDummy::MeshStorage::_multimesh_get_buffer_rd_rid;
		RendererMeshStorage::_multimesh_get_buffer =
			&RendererDummy::MeshStorage::_multimesh_get_buffer;
		RendererMeshStorage::_multimesh_set_visible_instances =
			&RendererDummy::MeshStorage::_multimesh_set_visible_instances;
		RendererMeshStorage::_multimesh_get_visible_instances =
			&RendererDummy::MeshStorage::_multimesh_get_visible_instances;
		RendererMeshStorage::_multimesh_get_aabb = &RendererDummy::MeshStorage::_multimesh_get_aabb;
		RendererMeshStorage::_multimesh_get_interpolator =
			&RendererDummy::MeshStorage::_multimesh_get_interpolator;

		RendererMeshStorage::skeleton_allocate = &RendererDummy::MeshStorage::skeleton_allocate;
		RendererMeshStorage::skeleton_initialize = &RendererDummy::MeshStorage::skeleton_initialize;
		RendererMeshStorage::skeleton_free = &RendererDummy::MeshStorage::skeleton_free;
		RendererMeshStorage::skeleton_allocate_data =
			&RendererDummy::MeshStorage::skeleton_allocate_data;
		RendererMeshStorage::skeleton_set_base_transform_2d =
			&RendererDummy::MeshStorage::skeleton_set_base_transform_2d;
		RendererMeshStorage::skeleton_get_bone_count =
			&RendererDummy::MeshStorage::skeleton_get_bone_count;
		RendererMeshStorage::skeleton_bone_set_transform =
			&RendererDummy::MeshStorage::skeleton_bone_set_transform;
		RendererMeshStorage::skeleton_bone_get_transform =
			&RendererDummy::MeshStorage::skeleton_bone_get_transform;
		RendererMeshStorage::skeleton_bone_set_transform_2d =
			&RendererDummy::MeshStorage::skeleton_bone_set_transform_2d;
		RendererMeshStorage::skeleton_bone_get_transform_2d =
			&RendererDummy::MeshStorage::skeleton_bone_get_transform_2d;
		RendererMeshStorage::skeleton_update_dependency =
			&RendererDummy::MeshStorage::skeleton_update_dependency;
	}

	static void bind_texture_storage();

	static void bind_material_storage();

	static void bind_canvas();

	static void bind_light_storage() { RendererLightStorage::bind<RendererDummy::LightStorage>(); }

	static void bind_particles_storage();

	static void bind_utilities();

	static void bind_gi() { RendererGI::bind<RendererDummy::GI>(); }

	static void bind_fog() { RendererFog::bind<RendererDummy::Fog>(); }

	RasterizerDummy() = delete;
	RasterizerDummy(const RasterizerDummy&) = delete;
	~RasterizerDummy() = delete;
};

using RDummy = RasterizerDummy;
// --- End servers/rendering/dummy/rasterizer_dummy.h ---
