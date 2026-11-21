/**************************************************************************/
/*  renderer_scene_render_rd.h                                            */
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

#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/effects/upscale_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
// --- Folded from servers/rendering/renderer_rd/storage_rd/render_data_rd.h ---
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
// --- Folded from servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h ---
#include "servers/rendering/renderer_scene_render.h"

// This is a container for data related to rendering a single frame of a viewport where we load this
// data into a UBO that can be used by the main scene shader but also by various effects.

class RenderSceneDataRD
{
public:
	bool calculate_motion_vectors = false;

	Transform3D cam_transform;
	Projection cam_projection;
	Vector2 taa_jitter;
	float taa_frame_count = 0.0f;
	uint32_t camera_visible_layers;
	bool cam_orthogonal = false;
	bool flip_y = false;

	// For billboards to cast correct shadows.
	Transform3D main_cam_transform;

	// For stereo rendering
	uint32_t view_count = 1;
	Vector3 view_eye_offset[RendererSceneRender::MAX_RENDER_VIEWS];
	Projection view_projection[RendererSceneRender::MAX_RENDER_VIEWS];

	Transform3D prev_cam_transform;
	Projection prev_cam_projection;
	Vector2 prev_taa_jitter;
	Projection prev_view_projection[RendererSceneRender::MAX_RENDER_VIEWS];

	float z_near = 0.0;
	float z_far = 0.0;

	float lod_distance_multiplier = 0.0;
	float screen_mesh_lod_threshold = 0.0;

	uint32_t directional_light_count = 0;
	float dual_paraboloid_side = 0.0;
	float opaque_prepass_threshold = 0.0;
	bool material_uv2_mode = false;
	float emissive_exposure_normalization = 0.0;
	bool shadow_pass = false;

	Size2 shadow_atlas_pixel_size;
	Size2 directional_shadow_pixel_size;

	float radiance_pixel_size;
	float radiance_border_size;
	Size2 reflection_atlas_border_size;

	float time;
	float time_step;

	Transform3D get_cam_transform() const;
	Projection get_cam_projection() const;

	uint32_t get_view_count() const;
	Vector3 get_view_eye_offset(uint32_t p_view) const;
	Projection get_view_projection(uint32_t p_view) const;

	RID create_uniform_buffer();
	void update_ubo(RID p_uniform_buffer, RSE::ViewportDebugDraw p_debug_mode, RID p_env,
		RID p_reflection_probe_instance, RID p_camera_attributes, bool p_pancake_shadows,
		const Size2i& p_screen_size, const Size2& p_viewport_size, const Color& p_default_bg_color,
		float p_luminance_multiplier, bool p_opaque_render_buffers, bool p_apply_alpha_multiplier);
	RID get_uniform_buffer() const;

	static uint32_t get_uniform_buffer_size_bytes() { return sizeof(UBODATA); }

private:
	RID uniform_buffer; // loaded into this uniform buffer (supplied externally)

	enum SceneDataFlags
	{
		SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT = 1 << 0,
		SCENE_DATA_FLAGS_USE_AMBIENT_CUBEMAP = 1 << 1,
		SCENE_DATA_FLAGS_USE_REFLECTION_CUBEMAP = 1 << 2,
		SCENE_DATA_FLAGS_USE_ROUGHNESS_LIMITER = 1 << 3,
		SCENE_DATA_FLAGS_USE_FOG = 1 << 4,
		SCENE_DATA_FLAGS_USE_UV2_MATERIAL = 1 << 5,
		SCENE_DATA_FLAGS_USE_PANCAKE_SHADOWS = 1 << 6,
		SCENE_DATA_FLAGS_IN_SHADOW_PASS = 1 << 7, // Only used by Forward+ renderer.
		SCENE_DATA_FLAGS_MAX
	};

	// This struct is loaded into Set 1 - Binding 0, populated at start of rendering a frame, must
	// match with shader code
	struct UBO
	{
		float projection_matrix[16];
		float inv_projection_matrix[16];
		float inv_view_matrix[12];
		float view_matrix[12];

#ifdef REAL_T_IS_DOUBLE
		float inv_view_precision[4];
#endif

		float projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS][16];
		float inv_projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS][16];
		float eye_offset[RendererSceneRender::MAX_RENDER_VIEWS][4];

		float main_cam_inv_view_matrix[16];

		float viewport_size[2];
		float screen_pixel_size[2];

		float directional_penumbra_shadow_kernel[128]; // 32 vec4s
		float directional_soft_shadow_kernel[128];
		float penumbra_shadow_kernel[128];
		float soft_shadow_kernel[128];

		float shadow_atlas_pixel_size[2];
		float directional_shadow_pixel_size[2];

		float radiance_pixel_size;
		float radiance_border_size;
		float reflection_atlas_border_size[2];

		uint32_t directional_light_count;
		float dual_paraboloid_side;
		float z_far;
		float z_near;

		float roughness_limiter_amount;
		float roughness_limiter_limit;
		float opaque_prepass_threshold;
		uint32_t flags;

		float radiance_inverse_xform[12];

		float ambient_light_color_energy[4];

		float ambient_color_sky_mix;
		float fog_density;
		float fog_height;
		float fog_height_density;

		float fog_depth_curve;
		float fog_depth_begin;
		float fog_depth_end;
		float fog_sun_scatter;

		float fog_light_color[3];
		float fog_aerial_perspective;

		float time;
		float taa_frame_count; // Used to add break up samples over multiple frames. Value is an
							   // integer from 0 to taa_phase_count -1.
		float taa_jitter[2];

		float emissive_exposure_normalization; // Needed to normalize emissive when using physical
											   // units.
		float IBL_exposure_normalization;	   // Adjusts for baked exposure.
		uint32_t camera_visible_layers;
		float pass_alpha_multiplier;
	};

	struct UBODATA
	{
		UBO ubo;
		UBO prev_ubo;
	};
};



// --- End servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h ---
#include "servers/rendering/rendering_server_types.h"

class RenderDataRD
{
public:
	// Access methods to expose data externally
	Ref<RenderSceneBuffers> get_render_scene_buffers() const
	{
		return render_buffers;
	}

	RenderSceneDataRD* get_render_scene_data() const { return scene_data; }

	RID get_environment() const { return environment; }

	RID get_camera_attributes() const { return camera_attributes; }

	// Members are publicly accessible within the render engine.
	Ref<RenderSceneBuffersRD> render_buffers;
	RenderSceneDataRD* scene_data = nullptr;

	const PagedArray<RenderGeometryInstance*>* instances = nullptr;
	const PagedArray<RID>* lights = nullptr;
	const PagedArray<RID>* reflection_probes = nullptr;
	const PagedArray<RID>* voxel_gi_instances = nullptr;
	const PagedArray<RID>* decals = nullptr;
	const PagedArray<RID>* lightmaps = nullptr;
	const PagedArray<RID>* fog_volumes = nullptr;
	RID environment;
	RID camera_attributes;
	RID compositor;
	RID shadow_atlas;
	RID occluder_debug_tex;
	RID reflection_atlas;
	RID reflection_probe;
	int reflection_probe_pass = 0;

	RID cluster_buffer;
	uint32_t cluster_size = 0;
	uint32_t cluster_max_elements = 0;

	uint32_t directional_light_count = 0;
	bool directional_light_soft_shadows = false;

	bool lightmap_bicubic_filter = false;

	float window_output_max_value = 1.0;

	RenderingServerTypes::RenderInfo* render_info = nullptr;

	/* Viewport data */
	bool transparent_bg = false;
	Rect2i render_region;

	/* Shadow data */
	const RendererSceneRender::RenderShadowData* render_shadows = nullptr;
	int render_shadow_count = 0;

	LocalVector<int> cube_shadows;
	LocalVector<int> shadows;
	LocalVector<int> directional_shadows;

	/* GI info */
	const RendererSceneRender::RenderSDFGIData* render_sdfgi_regions = nullptr;
	int render_sdfgi_region_count = 0;
	const RendererSceneRender::RenderSDFGIUpdateData* sdfgi_update_data = nullptr;

	uint32_t voxel_gi_count = 0;
};



// --- End servers/rendering/renderer_rd/storage_rd/render_data_rd.h ---
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/renderer_rd/shaders/cluster_debug.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/cluster_render.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/cluster_store.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/environment/rd_environment.h"

class ClusterBuilderSharedDataRD final {
	friend class ClusterBuilderRD;

	RID sphere_vertex_buffer;
	RID sphere_vertex_array;
	RID sphere_index_buffer;
	RID sphere_index_array;
	float sphere_overfit = 0.0; // Because an icosphere is not a perfect sphere, we need to enlarge it to cover the sphere area.

	RID cone_vertex_buffer;
	RID cone_vertex_array;
	RID cone_index_buffer;
	RID cone_index_array;
	float cone_overfit = 0.0; // Because an cone mesh is not a perfect cone, we need to enlarge it to cover the actual cone area.

	RID box_vertex_buffer;
	RID box_vertex_array;
	RID box_index_buffer;
	RID box_index_array;

	enum Divisor {
		DIVISOR_1,
		DIVISOR_2,
		DIVISOR_4,
	};

	struct ClusterRender {
		struct PushConstant {
			uint32_t base_index;
			uint32_t pad0;
			uint32_t pad1;
			uint32_t pad2;
		};

		ShaderRD cluster_render_shader{ClusterRenderShaderRD()};
		RID shader_version;
		RID shader;

		enum ShaderVariant {
			SHADER_NORMAL,
			SHADER_USE_ATTACHMENT,
		};

		enum PipelineVersion {
			PIPELINE_NORMAL,
			PIPELINE_MSAA,
			PIPELINE_MAX
		};

		RID shader_pipelines[PIPELINE_MAX];
	} cluster_render;

	struct ClusterStore {
		struct PushConstant {
			uint32_t cluster_render_data_size; // how much data for a single cluster takes
			uint32_t max_render_element_count_div_32; // divided by 32
			uint32_t cluster_screen_size[2];
			uint32_t render_element_count_div_32; // divided by 32
			uint32_t max_cluster_element_count_div_32; // divided by 32

			uint32_t pad1;
			uint32_t pad2;
		};

		ShaderRD cluster_store_shader{ClusterStoreShaderRD()};
		RID shader_version;
		RID shader;
		RID shader_pipeline;
	} cluster_store;

	struct ClusterDebug {
		struct PushConstant {
			uint32_t screen_size[2];
			uint32_t cluster_screen_size[2];

			uint32_t cluster_shift;
			uint32_t cluster_type;
			float z_near;
			float z_far;

			uint32_t orthogonal;
			uint32_t max_cluster_element_count_div_32;

			uint32_t pad1;
			uint32_t pad2;
		};

		ShaderRD cluster_debug_shader{ClusterDebugShaderRD()};
		RID shader_version;
		RID shader;
		RID shader_pipeline;
	} cluster_debug;

public:
	ClusterBuilderSharedDataRD();
	~ClusterBuilderSharedDataRD();
};

class ClusterBuilderRD final {
public:
	static constexpr float WIDE_SPOT_ANGLE_THRESHOLD_DEG = 60.0f;

	enum LightType {
		LIGHT_TYPE_OMNI,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_AREA,
	};

	enum BoxType {
		BOX_TYPE_REFLECTION_PROBE,
		BOX_TYPE_DECAL,
	};

	enum ElementType {
		ELEMENT_TYPE_OMNI_LIGHT,
		ELEMENT_TYPE_SPOT_LIGHT,
		ELEMENT_TYPE_AREA_LIGHT,
		ELEMENT_TYPE_DECAL,
		ELEMENT_TYPE_REFLECTION_PROBE,
		ELEMENT_TYPE_MAX,
	};

private:
	ClusterBuilderSharedDataRD *shared = nullptr;

	struct RenderElementData {
		uint32_t type; // 0-4
		uint32_t touches_near;
		uint32_t touches_far;
		uint32_t original_index;
		float transform_inv[12]; // Transposed transform for less space.
		float scale[3];
		uint32_t has_wide_spot_angle;
	}; // Keep aligned to 32 bytes.

	uint32_t cluster_count_by_type[ELEMENT_TYPE_MAX] = {};
	uint32_t max_elements_by_type = 0;

	RenderElementData *render_elements = nullptr;
	uint32_t render_element_count = 0;
	uint32_t render_element_max = 0;

	Transform3D view_xform;
	Projection adjusted_projection;
	Projection projection;
	float z_far = 0;
	float z_near = 0;
	bool camera_orthogonal = false;

	enum Divisor {
		DIVISOR_1,
		DIVISOR_2,
		DIVISOR_4,
	};

	uint32_t cluster_size = 32;
	bool use_msaa = true;
	Divisor divisor = DIVISOR_4;

	Size2i screen_size;
	Size2i cluster_screen_size;

	RID framebuffer;
	RID cluster_render_buffer; // Used for creating.
	RID cluster_buffer; // Used for rendering.
	RID element_buffer; // Used for storing, to hint element touches far plane or near plane.
	uint32_t cluster_render_buffer_size = 0;
	uint32_t cluster_buffer_size = 0;

	RID cluster_render_uniform_set;
	RID cluster_store_uniform_set;

	// Persistent data.

	void _clear();

	struct StateUniform {
		float projection[16];
		float inv_z_far;
		uint32_t screen_to_clusters_shift; // Shift to obtain coordinates in block indices.
		uint32_t cluster_screen_width;
		uint32_t cluster_data_size; // How much data is needed for a single cluster.
		uint32_t cluster_depth_offset;

		uint32_t pad0;
		uint32_t pad1;
		uint32_t pad2;
	};

	RID state_uniform;

	RID debug_uniform_set;

public:
	void setup(Size2i p_screen_size, uint32_t p_max_elements, RID p_depth_buffer, RID p_depth_buffer_sampler, RID p_color_buffer);

	void begin(const Transform3D &p_view_transform, const Projection &p_cam_projection, bool p_flip_y);

	_FORCE_INLINE_ void add_light(LightType p_type, const Transform3D &p_transform, float p_radius, float p_spot_aperture, const Vector2 &p_area_size) {
		if (p_type == LIGHT_TYPE_OMNI && cluster_count_by_type[ELEMENT_TYPE_OMNI_LIGHT] == max_elements_by_type) {
			return; // Max number elements reached.
		}
		if (p_type == LIGHT_TYPE_SPOT && cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT] == max_elements_by_type) {
			return; // Max number elements reached.
		}
		if (p_type == LIGHT_TYPE_AREA && cluster_count_by_type[ELEMENT_TYPE_AREA_LIGHT] == max_elements_by_type) {
			return; // Max number elements reached.
		}

		RenderElementData &e = render_elements[render_element_count];

		Transform3D xform = view_xform * p_transform;

		float radius = xform.basis.get_uniform_scale();
		if (radius < 0.98 || radius > 1.02) {
			xform.basis.orthonormalize();
		}

		radius *= p_radius;

		// Spotlights with wide angle are trated as Omni lights.
		// If the spot angle is above the threshold, we need a sphere instead of a cone for building the clusters
		// since the cone gets too flat/large (spot angle close to 90 degrees) or
		// can't even cover the affected area of the light (spot angle above 90 degrees).
		if (p_type == LIGHT_TYPE_OMNI || (p_type == LIGHT_TYPE_SPOT && p_spot_aperture > WIDE_SPOT_ANGLE_THRESHOLD_DEG)) {
			radius *= shared->sphere_overfit; // Overfit icosphere.

			float depth = -xform.origin.z;
			if (camera_orthogonal) {
				e.touches_near = (depth - radius) < z_near;
			} else {
				// Contains camera inside light.
				float radius2 = radius * shared->sphere_overfit; // Overfit again for outer size (camera may be outside actual sphere but behind an icosphere vertex)
				e.touches_near = xform.origin.length_squared() < radius2 * radius2;
			}

			e.touches_far = (depth + radius) > z_far;
			e.scale[0] = radius;
			e.scale[1] = radius;
			e.scale[2] = radius;
			if (p_type == LIGHT_TYPE_OMNI) {
				e.type = ELEMENT_TYPE_OMNI_LIGHT;
				e.original_index = cluster_count_by_type[ELEMENT_TYPE_OMNI_LIGHT];
				cluster_count_by_type[ELEMENT_TYPE_OMNI_LIGHT]++;
			} else { // LIGHT_TYPE_SPOT with wide angle.
				e.type = ELEMENT_TYPE_SPOT_LIGHT;
				e.has_wide_spot_angle = true;
				e.original_index = cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT];
				cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT]++;
			}

			RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);

		} else if (p_type == LIGHT_TYPE_SPOT) { /*LIGHT_TYPE_SPOT with no wide angle*/
			radius *= shared->cone_overfit; // Overfit cone.

			real_t len = Math::tan(Math::deg_to_rad(p_spot_aperture)) * radius;
			// Approximate, probably better to use a cone support function.
			float max_d = -1e20;
			float min_d = 1e20;
#define CONE_MINMAX(m_x, m_y) \
	{ \
		float d = -xform.xform(Vector3(len * m_x, len * m_y, -radius)).z; \
		min_d = MIN(d, min_d); \
		max_d = MAX(d, max_d); \
	}

			CONE_MINMAX(1, 1);
			CONE_MINMAX(-1, 1);
			CONE_MINMAX(-1, -1);
			CONE_MINMAX(1, -1);
#undef CONE_MINMAX

			if (camera_orthogonal) {
				e.touches_near = min_d < z_near;
			} else {
				Plane base_plane(-xform.basis.get_column(Vector3::AXIS_Z), xform.origin);
				float dist = base_plane.distance_to(Vector3());
				if (dist >= 0 && dist < radius) {
					// Contains camera inside light, check angle.
					float angle = Math::rad_to_deg(Math::acos((-xform.origin.normalized()).dot(-xform.basis.get_column(Vector3::AXIS_Z))));
					e.touches_near = angle < p_spot_aperture * 1.05; //overfit aperture a little due to cone overfit
				} else {
					e.touches_near = false;
				}
			}

			e.touches_far = max_d > z_far;
			e.scale[0] = len * shared->cone_overfit;
			e.scale[1] = len * shared->cone_overfit;
			e.scale[2] = radius;
			e.has_wide_spot_angle = false;
			e.type = ELEMENT_TYPE_SPOT_LIGHT;
			e.original_index = cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT];

			RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);

			cluster_count_by_type[ELEMENT_TYPE_SPOT_LIGHT]++;
		} else { /* LIGHT_TYPE_AREA */
			Vector3 scale = Vector3(p_area_size.x / 2.0 + radius, p_area_size.y / 2.0 + radius, radius / 2.0);

			for (uint32_t i = 0; i < 3; i++) {
				float s = xform.basis.rows[i].length();
				//scale[i] *= s; // lights ignore scale
				xform.basis.rows[i] /= s;
			}
			xform.origin -= xform.basis.get_column(Vector3::AXIS_Z) * scale.z; // translate center to center of box

			float depth = -xform.origin.z;
			float box_depth = Math::abs(xform.basis.xform_inv(Vector3(0, 0, -1)).dot(scale));

			if (camera_orthogonal) {
				e.touches_near = (depth - box_depth) < z_near;
			} else {
				// Contains camera inside box.
				Vector3 inside = xform.xform_inv(Vector3(0, 0, 0)).abs();
				e.touches_near = inside.x < scale.x && inside.y < scale.y && inside.z < scale.z;
			}

			e.touches_far = depth + box_depth > z_far;

			e.scale[0] = scale.x;
			e.scale[1] = scale.y;
			e.scale[2] = scale.z;

			e.type = ELEMENT_TYPE_AREA_LIGHT;
			e.original_index = cluster_count_by_type[ELEMENT_TYPE_AREA_LIGHT];

			RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);

			cluster_count_by_type[ELEMENT_TYPE_AREA_LIGHT]++;
		}

		render_element_count++;
	}

	_FORCE_INLINE_ void add_box(BoxType p_box_type, const Transform3D &p_transform, const Vector3 &p_half_size) {
		if (p_box_type == BOX_TYPE_DECAL && cluster_count_by_type[ELEMENT_TYPE_DECAL] == max_elements_by_type) {
			return; // Max number elements reached.
		}
		if (p_box_type == BOX_TYPE_REFLECTION_PROBE && cluster_count_by_type[ELEMENT_TYPE_REFLECTION_PROBE] == max_elements_by_type) {
			return; // Max number elements reached.
		}

		RenderElementData &e = render_elements[render_element_count];
		Transform3D xform = view_xform * p_transform;

		// Extract scale and scale the matrix by it, makes things simpler.
		Vector3 scale = p_half_size;
		for (uint32_t i = 0; i < 3; i++) {
			float s = xform.basis.rows[i].length();
			scale[i] *= s;
			xform.basis.rows[i] /= s;
		};

		float box_depth = Math::abs(xform.basis.xform_inv(Vector3(0, 0, -1)).dot(scale));
		float depth = -xform.origin.z;

		if (camera_orthogonal) {
			e.touches_near = depth - box_depth < z_near;
		} else {
			// Contains camera inside box.
			Vector3 inside = xform.xform_inv(Vector3(0, 0, 0)).abs();
			e.touches_near = inside.x < scale.x && inside.y < scale.y && inside.z < scale.z;
		}

		e.touches_far = depth + box_depth > z_far;

		e.scale[0] = scale.x;
		e.scale[1] = scale.y;
		e.scale[2] = scale.z;

		e.type = (p_box_type == BOX_TYPE_DECAL) ? ELEMENT_TYPE_DECAL : ELEMENT_TYPE_REFLECTION_PROBE;
		e.original_index = cluster_count_by_type[e.type];

		RendererRD::MaterialStorage::store_transform_transposed_3x4(xform, e.transform_inv);

		cluster_count_by_type[e.type]++;
		render_element_count++;
	}

	_FORCE_INLINE_ uint32_t get_cluster_count_by_type(ElementType p_element_type) const {
		DEV_ASSERT(p_element_type < ELEMENT_TYPE_MAX);
		return cluster_count_by_type[p_element_type];
	}

	void bake_cluster();
	void debug(ElementType p_element);

	RID get_cluster_buffer() const;
	uint32_t get_cluster_size() const;
	uint32_t get_max_cluster_elements() const;

	void set_shared(ClusterBuilderSharedDataRD *p_shared);

	ClusterBuilderRD();
	~ClusterBuilderRD();
};

class RendererSceneRenderRD final
{
	static inline BinaryMutex _thread_safe_mutex;

	friend class RendererRD::SkyRD;
	friend class RendererRD::GI;

public:
	enum FeatureBits
	{
		FEATURE_ADVANCED_BIT = 1U << 0U,
		FEATURE_MULTIVIEW_BIT = 1U << 1U,
		FEATURE_VRS_BIT = 1U << 2U,
		FEATURE_FP16_BIT = 1U << 3U,
		FEATURE_FP32_BIT = 1U << 4U,
	};
	using CameraData = RendererSceneRender::CameraData;
	using RenderShadowData = RendererSceneRender::RenderShadowData;
	using RenderSDFGIData = RendererSceneRender::RenderSDFGIData;
	using RenderSDFGIUpdateData = RendererSceneRender::RenderSDFGIUpdateData;

	struct Data
	{
		RendererRD::BokehDOF* bokeh_dof = nullptr;
		RendererRD::DebugEffects* debug_effects = nullptr;
		RendererRD::Luminance* luminance = nullptr;
		RendererRD::SMAA* smaa = nullptr;
		RendererRD::ToneMapper* tone_mapper = nullptr;
		RendererRD::FSR* fsr = nullptr;
		RendererRD::VRS* vrs = nullptr;
		RendererRD::Resolve* resolve_effects = nullptr;
#ifdef METAL_ENABLED
		RendererRD::MFXSpatialEffect* mfx_spatial = nullptr;
#endif
		double time = 0.0;
		double time_step = 0.0;

		/* ENVIRONMENT */
		bool glow_bicubic_upscale = false;
		bool use_physical_light_units = false;

		// Needed for single argument calls (material and uv2)
		PagedArrayPool<RenderGeometryInstance*> cull_argument_pool;
		PagedArray<RenderGeometryInstance*> cull_argument;

		RendererRD::SkyRD sky;

		RSE::ViewportDebugDraw debug_draw = RSE::VIEWPORT_DEBUG_DRAW_DISABLED;

		/* Shadow atlas */
		RSE::ShadowQuality shadows_quality = RSE::SHADOW_QUALITY_MAX;
		RSE::ShadowQuality directional_shadow_quality = RSE::SHADOW_QUALITY_MAX;
		float shadows_quality_radius = 1.0f;
		float directional_shadow_quality_radius = 1.0f;

		float* directional_penumbra_shadow_kernel = nullptr;
		float* directional_soft_shadow_kernel = nullptr;
		float* penumbra_shadow_kernel = nullptr;
		float* soft_shadow_kernel = nullptr;
		bool lightmap_filter_bicubic = false;
		int directional_penumbra_shadow_samples = 0;
		int directional_soft_shadow_samples = 0;
		int penumbra_shadow_samples = 0;
		int soft_shadow_samples = 0;
		RSE::DecalFilter decals_filter = RSE::DECAL_FILTER_LINEAR_MIPMAPS;
		RSE::LightProjectorFilter light_projectors_filter =
			RSE::LIGHT_PROJECTOR_FILTER_LINEAR_MIPMAPS;
		bool material_use_debanding = false;

		/* GI */
		bool screen_space_roughness_limiter = false;
		float screen_space_roughness_limiter_amount = 0.25f;
		float screen_space_roughness_limiter_limit = 0.18f;

		/* Light data */
		uint64_t scene_pass = 0;
		uint32_t max_cluster_elements = 512;

		/* Fog */
		bool fog_use_legacy_blending = false;

		/* Volumetric Fog */
		uint32_t volumetric_fog_size = 128;
		uint32_t volumetric_fog_depth = 128;
		bool volumetric_fog_filter_active = true;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

public:
	/* CANVAS TEXTURE CALLBACK & INFO */
	typedef void (*CanvasTextureInvalidationCallback)(void* p_userdata);
	static void canvas_texture_set_invalidation_callback(RID p_texture,
		CanvasTextureInvalidationCallback p_callback = nullptr, void* p_userdata = nullptr);

	struct CanvasTextureInfo
	{
		RID diffuse;
		RID normal;
		RID specular;
		Color specular_color;
		float shininess = 0.0;
		bool use_normal = false;
		bool use_specular = false;

		_FORCE_INLINE_ bool is_null() const { return diffuse.is_null(); }
	};

	static CanvasTextureInfo canvas_texture_get_info(RID p_texture,
		RSE::CanvasItemTextureFilter p_base_filter, RSE::CanvasItemTextureRepeat p_base_repeat);

	/* DEFAULT TEXTURES */
	static RID texture_rd_get_default(RendererRD::TextureStorage::DefaultRDTexture p_texture);

	/* ATLAS HELPERS */
	static RID decal_atlas_get_texture();
	static Rect2 decal_atlas_get_texture_rect(RID p_texture);
	static RID area_light_atlas_get_texture();

	/* RENDER TARGET HELPERS */
	static RID render_target_get_rd_texture(RID p_render_target);
	static RID render_target_get_rd_texture_msaa(RID p_render_target);
	static RID render_target_get_rd_texture_slice(RID p_render_target, uint32_t p_layer);
	static RID render_target_get_rd_backbuffer(RID p_render_target);
	static RID render_target_get_rd_framebuffer(RID p_render_target);
	static RID render_target_get_sdf_texture(RID p_render_target);
	static bool render_target_is_sdf_enabled(RID p_render_target);

	static void render_target_set_backbuffer_uniform_set(RID p_render_target, RID p_uniform_set);
	static void render_target_set_framebuffer_uniform_set(RID p_render_target, RID p_uniform_set);

	static void render_target_copy_to_back_buffer(
		RID p_render_target, const Rect2i& p_region, bool p_gen_mipmaps = false);
	static void render_target_clear_back_buffer(
		RID p_render_target, const Rect2i& p_region, const Color& p_color);
	static void render_target_gen_back_buffer_mipmaps(RID p_render_target, const Rect2i& p_region);

	static void _tex_blit_shader_free();
	static void setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers);

	static void _render_scene(RenderDataRD* p_render_data, const Color& p_default_color);
	static void _render_buffers_debug_draw(const RenderDataRD* p_render_data);

	static void _render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region, float p_exposure_normalization);
	static void _render_uv2(const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region);
	static void _render_sdfgi(Ref<RenderSceneBuffersRD> p_render_buffers, const Vector3i& p_from,
		const Vector3i& p_size, const AABB& p_bounds,
		const PagedArray<RenderGeometryInstance*>& p_instances, const RID& p_albedo_texture,
		const RID& p_emission_texture, const RID& p_emission_aniso_texture,
		const RID& p_geom_facing_texture, float p_exposure_normalization);
	static void _render_particle_collider_heightfield(RID p_fb, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, const PagedArray<RenderGeometryInstance*>& p_instances);

	static RID _render_buffers_get_normal_texture(Ref<RenderSceneBuffersRD> p_render_buffers);
	static RID _render_buffers_get_velocity_texture(Ref<RenderSceneBuffersRD> p_render_buffers);

	static void _post_prepass_render(RenderDataRD* p_render_data, bool p_use_gi);

	static bool _has_compositor_effect(
		RSE::CompositorEffectCallbackType p_callback_type, const RenderDataRD* p_render_data);
	static void _render_buffers_ensure_screen_texture(const RenderDataRD* p_render_data);
	static void _render_buffers_copy_screen_texture(const RenderDataRD* p_render_data);
	static void _render_buffers_ensure_depth_texture(const RenderDataRD* p_render_data);
	static void _render_buffers_copy_depth_texture(
		const RenderDataRD* p_render_data, bool p_use_msaa = false);
	static void _render_buffers_post_process_and_tonemap(
		const RenderDataRD* p_render_data, bool p_use_msaa = false);
	static void _post_process_subpass(
		RID p_source_texture, RID p_framebuffer, const RenderDataRD* p_render_data);
	static void _disable_clear_request(const RenderDataRD* p_render_data);

	static void _update_shader_quality_settings() {}

	static bool _debug_draw_can_use_effects(RSE::ViewportDebugDraw p_debug_draw);

	static void _debug_sdfgi_probes(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_framebuffer,
		uint32_t p_view_count, const Projection* p_camera_with_transforms);
	static void _process_compositor_effects(
		RSE::CompositorEffectCallbackType p_callback_type, const RenderDataRD* p_render_data);
	static bool _needs_post_prepass_render(RenderDataRD* p_render_data, bool p_use_gi);
	static void _update_vrs(Ref<RenderSceneBuffersRD> p_render_buffers);
	static bool _compositor_effects_has_flag(const RenderDataRD* p_render_data,
		RSE::CompositorEffectFlags p_flag,
		RSE::CompositorEffectCallbackType p_callback_type =
			RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY);

	/* LIGHTING */

	static void setup_added_reflection_probe(
		const Transform3D& p_transform, const Vector3& p_half_size)
	{
	}

	static void setup_added_light(const RSE::LightType p_type, const Transform3D& p_transform,
		float p_radius, float p_spot_aperture, const Vector2& p_area_size)
	{
	}

	static void setup_added_decal(const Transform3D& p_transform, const Vector3& p_half_size) {}

	/* SKY */

	static RendererRD::SkyRD* get_sky() { return &data->sky; }

	/* SKY API */

	static RID sky_allocate();
	static void sky_initialize(RID p_rid);

	static void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	static void sky_set_mode(RID p_sky, RSE::SkyMode p_mode);
	static void sky_set_material(RID p_sky, RID p_material);
	static Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* ENVIRONMENT API */

	static void environment_glow_set_use_bicubic_upscale(bool p_enable);

	static void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	static void environment_set_volumetric_fog_filter_active(bool p_enable);

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);
	static void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);
	static void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	static Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	static _FORCE_INLINE_ bool is_using_physical_light_units()
	{
		return data->use_physical_light_units;
	}

	/* REFLECTION PROBE */

	static RID reflection_probe_create_framebuffer(RID p_color, RID p_depth);

	/* FOG VOLUMES */

	static uint32_t get_volumetric_fog_size() { return data->volumetric_fog_size; }

	static uint32_t get_volumetric_fog_depth() { return data->volumetric_fog_depth; }

	static bool get_volumetric_fog_filter_active() { return data->volumetric_fog_filter_active; }

	static RID fog_volume_instance_create(RID p_fog_volume);
	static void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform);
	static void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active);
	static RID fog_volume_instance_get_volume(RID p_fog_volume_instance);
	static Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance);

	/* GI LIGHT PROBES */

	static RID voxel_gi_instance_create(RID p_base);
	static void voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform);
	static bool voxel_gi_needs_update(RID p_probe);
	static void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects);

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_quality)
	{
		RendererRD::GI::voxel_gi_quality = p_quality;
	}

	/* RENDER BUFFERS */
	static RDC::DataFormat _render_buffers_get_preferred_color_format();
	static bool _render_buffers_can_be_storage();
	static Ref<RenderSceneBuffers> render_buffers_create();
	static void gi_set_use_half_resolution(bool p_enable);

	static RID render_buffers_get_default_voxel_gi_buffer();

	static void base_uniforms_changed();

	static void render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
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

	static void render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region);

	static void render_particle_collider_heightfield(RID p_collider, const Transform3D& p_transform,
		const PagedArray<RenderGeometryInstance*>& p_instances);

	static void set_scene_pass(uint64_t p_pass) { data->scene_pass = p_pass; }

	static _FORCE_INLINE_ uint64_t get_scene_pass() { return data->scene_pass; }

	static void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);
	static bool screen_space_roughness_limiter_is_active();
	static float screen_space_roughness_limiter_get_amount();
	static float screen_space_roughness_limiter_get_limit();

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	static void decals_set_filter(RSE::DecalFilter p_filter);
	static void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);
	static void lightmaps_set_bicubic_filter(bool p_enable);
	static void material_set_use_debanding(bool p_enable);

	_FORCE_INLINE_ static RSE::ShadowQuality shadows_quality_get() { return data->shadows_quality; }

	_FORCE_INLINE_ static RSE::ShadowQuality directional_shadow_quality_get()
	{
		return data->directional_shadow_quality;
	}

	static _FORCE_INLINE_ float shadows_quality_radius_get()
	{
		if (!data->shadows_quality_radius) {
			data->shadows_quality_radius = 1.0f;
		}
		return data->shadows_quality_radius;
	}

	static _FORCE_INLINE_ float directional_shadow_quality_radius_get()
	{
		if (!data->directional_shadow_quality_radius) {
			data->directional_shadow_quality_radius = 1.0f;
		}
		return data->directional_shadow_quality_radius;
	}

	_FORCE_INLINE_ static float* directional_penumbra_shadow_kernel_get()
	{
		return data->directional_penumbra_shadow_kernel;
	}

	_FORCE_INLINE_ static float* directional_soft_shadow_kernel_get()
	{
		return data->directional_soft_shadow_kernel;
	}

	_FORCE_INLINE_ static float* penumbra_shadow_kernel_get()
	{
		return data->penumbra_shadow_kernel;
	}

	_FORCE_INLINE_ static float* soft_shadow_kernel_get() { return data->soft_shadow_kernel; }

	_FORCE_INLINE_ static int directional_penumbra_shadow_samples_get()
	{
		return data->directional_penumbra_shadow_samples;
	}

	_FORCE_INLINE_ static bool lightmap_filter_bicubic_get()
	{
		return data->lightmap_filter_bicubic;
	}

	_FORCE_INLINE_ static int directional_soft_shadow_samples_get()
	{
		return data->directional_soft_shadow_samples;
	}

	_FORCE_INLINE_ static int penumbra_shadow_samples_get()
	{
		return data->penumbra_shadow_samples;
	}

	_FORCE_INLINE_ static int soft_shadow_samples_get() { return data->soft_shadow_samples; }

	_FORCE_INLINE_ static RSE::LightProjectorFilter light_projectors_get_filter()
	{
		return data->light_projectors_filter;
	}

	_FORCE_INLINE_ static RSE::DecalFilter decals_get_filter() { return data->decals_filter; }

	_FORCE_INLINE_ static bool material_use_debanding_get() { return data->material_use_debanding; }

	static _FORCE_INLINE_ bool fog_use_legacy_blending_get()
	{
		return data->fog_use_legacy_blending;
	}

	static int get_roughness_layers();
	static bool is_using_radiance_octmap_array();

	static bool free(RID p_rid);

	static void update();

	static void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw);

	_FORCE_INLINE_ static RSE::ViewportDebugDraw get_debug_draw_mode() { return data->debug_draw; }

	static void set_time(double p_time, double p_step);

	static void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	static bool is_vrs_supported();
	static bool is_dynamic_gi_supported();
	static bool is_volumetric_supported();
	static uint32_t get_max_elements();

	RendererSceneRenderRD() = delete;
	RendererSceneRenderRD(const RendererSceneRenderRD&) = delete;
	RendererSceneRenderRD& operator=(const RendererSceneRenderRD&) = delete;
	~RendererSceneRenderRD() = delete;
};

// --- Folded from servers/rendering/multi_uma_buffer.h ---
#include "servers/rendering/rendering_device.h"

enum class MultiUmaBufferType : uint8_t
{
	UNIFORM,
	STORAGE,
	VERTEX,
};

/// Interface for making it easier to work with UMA.
///
/// # What is UMA?
///
/// It stands for Unified Memory Architecture. There are two kinds of UMA:
///	 1. HW UMA. This is the case of iGPUs (specially Android, iOS, Apple ARM-based macOS, PS4 & PS5)
///		The CPU and GPU share the same die and same memory. So regular RAM and VRAM are internally
/// the 		same thing. There may be some differences between them in practice due to cache
/// synchronization 		behaviors or the regular BW RAM may be purposely throttled (as is the
/// case of PS4 & PS5).
///  2. "Pretended UMA". On PC Desktop GPUs with ReBAR enabled can pretend VRAM behaves like normal
///		RAM, while internally the data is moved across the PCIe Bus. This can cause differences
///		in execution time of the routines that write to GPU buffers as the region is often uncached
///		(i.e. write-combined) and PCIe latency and BW is vastly different from regular RAM.
///		Without ReBAR, the amount of UMA memory is limited to 256MB (shared by the entire system).
///
/// Since often this type of memory is uncached, it is not well-suited for downloading GPU -> CPU,
/// but rather for uploading CPU -> GPU.
///
/// # When to use UMA buffers?
///
/// UMA buffers have various caveats and improper usage might lead to visual glitches. Therefore
/// they should be used sparingly, where it makes a difference. Does all of the following check?:
///	  1. Data is uploaded from CPU to GPU every (or almost every) frame.
///   2. Data is always uploaded from scratch. Partial uploads are unsupported.
///	  3. If uploading multiple times per frame (e.g. for multiple passes). The amount of times
///      per frame is relatively stable (occasional spikes are fine if using MAX_EXTRA_BUFFERS).
///
/// # Why the caveats?
///
///	This is due to our inability to detect race conditions. If you write to an UMA buffer, submit
///	GPU commands and then write more data to it, we can't guarantee that you won't be writing to a
/// region the GPU is currently reading from. Tools like the validation layers cannot detect this
/// race condition at all, making it very hard to troubleshoot.
///
/// Therefore the safest approach is to use an interface that forces users to upload everything at
/// once. There is one exception for performance: map_raw_for_upload() will return a pointer, and it
/// is your responsibility to make sure you don't use that pointer again after submitting. USE THIS
/// API CALL SPARINGLY AND WITH CARE.
///
/// Since we forbid uploading more data after we've uploaded to it, this Interface will create
/// more buffers. This means users will need more UniformSets (i.e. uniform_set_create).
///
/// # How to use
///
/// Example code 01:
///		MultiUmaBuffer<1> uma_buffer = MultiUmaBuffer<1>("Debug name displayed if run with
///--verbose"); 		uma_buffer.set_uniform_size(0, max_size_bytes);
///
///		for(uint32_t i = 0u; i < num_passes; ++i) {
///			uma_buffer.prepare_for_upload(); // Creates a new buffer (if none exists already)
///											 // of max_size_bytes. Must be called.
///			uma_buffer.upload(0, src_data, size_bytes);
///
///			if(!uniform_set[i]) {
///				RD::Uniform u;
///				u.binding = 1;
///				u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC;
///				u.append_id(uma_buffer._get(0u));
///				uniform_set[i] = rd->uniform_set_create( ... );
///			}
///		}
///
///	  // On shutdown (or if you need to call set_size again).
///	  uma_buffer.uninit();
///
/// Example code 02:
///
///		uma_buffer.prepare_for_upload();
///		RID rid = uma_buffer.get_for_upload(0u);
///		rd->buffer_update(rid, 0, sizeof(BakeParameters), &bake_parameters);
///		RD::Uniform u; // Skipping full initialization of u. See Example 01.
///		u.append_id(rid);
///
/// Example code 03:
///
///		void *dst_data = uma_buffer.map_raw_for_upload(0u);
///		memcpy(dst_data, src_data, size_bytes);
///		rd->buffer_flush(uma_buffer._get(0u));
///		RD::Uniform u; // Skipping full initialization of u. See Example 01.
///		u.append_id(rid);
///
/// # Tricks
///
///	Godot's shadow mapping code calls uma_buffer.uniform_buffers._get(-p_pass_offset) (i.e. a
/// negative value)
/// because for various reasons its shadow mapping code was written like this:
///
///		for( uint32_t i = 0u; i < num_passes; ++i ) {
///			uma_buffer.prepare_for_upload();
///			uma_buffer.upload(0, src_data, size_bytes);
///		}
///		for( uint32_t i = 0u; i < num_passes; ++i ) {
///			RD::Uniform u;
///			u.binding = 1;
///			u.uniform_type = RD::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC;
///			u.append_id(uma_buffer._get(-(num_passes - 1u - i)));
///			uniform_set[i] = rd->uniform_set_create( ... );
///		}
///
/// Every time prepare_for_upload() is called, uma_buffer._get(-idx) will return a different RID(*).
/// Thus with a negative value we can address previous ones. This is fine as long as the value idx
/// doesn't exceed the number of times the user called prepare_for_upload() for this frame.
///
/// (*)This RID will be returned again on the next frame after the same amount of
/// prepare_for_upload() calls; unless the number of times it was called exceeded MAX_EXTRA_BUFFERS.
///
/// # Template parameters
///
///	## NUM_BUFFERS
///
/// How many buffers we should track. e.g. instead of doing this:
///		MultiUmaBuffer<1> omni_lights = /*...*/;
///		MultiUmaBuffer<1> spot_lights = /*...*/;
///		MultiUmaBuffer<1> directional_lights = /*...*/;
///
///		omni_lights.set_uniform_size(0u, omni_size);
///		spot_lights.set_uniform_size(0u, spot_size);
///		directional_lights.set_uniform_size(0u, dir_size);
///
///		omni_lights.prepare_for_upload();
///		spot_lights.prepare_for_upload();
///		directional_lights.prepare_for_upload();
///
/// You can do this:
///
///		MultiUmaBuffer<3> lights = /*...*/;
///
///		lights.set_uniform_size(0u, omni_size);
///		lights.set_uniform_size(1u, spot_size);
///		lights.set_uniform_size(2u, dir_size);
///
///		lights.prepare_for_upload();
///
/// This approach works as long as all buffers would call prepare_for_upload() at the same time.
/// It saves some overhead.
///
/// ## MAX_EXTRA_BUFFERS
///
/// Upper limit on the number of buffers per frame.
///
/// There are times where rendering might spike for exceptional reasons, calling
/// prepare_for_upload() too many times, never to do that again. This will cause an increase in
/// memory usage that will never be reclaimed until shutdown.
///
/// MAX_EXTRA_BUFFERS can be used to handle such spikes, by deallocating the extra buffers.
/// Example:
///		MultiUmaBuffer<1, 6> buffer;
///
///		// Normal frame (assuming up to 6 passes is considered normal):
///		for(uint32_t i = 0u; i < 6u; ++i) {
///			buffer.prepare_for_upload();
///			...
///			buffer.upload(...);
///		}
///
///		// Exceptional frame:
///		for(uint32_t i = 0u; i < 24u; ++i) {
///			buffer.prepare_for_upload();
///			...
///			buffer.upload(...);
///		}
///
///	After the frame is done, those extra 18 buffers will be deleted.
/// Launching godot with --verbose will print diagnostic information.
template <uint32_t NUM_BUFFERS, uint32_t MAX_EXTRA_BUFFERS = UINT32_MAX>
class MultiUmaBuffer
{
	struct BufferInfo
	{
		uint32_t size_bytes = 0;
		MultiUmaBufferType type = MultiUmaBufferType::UNIFORM;
	};

	LocalVector<RID> buffers;
	uint32_t curr_idx = UINT32_MAX;
	uint64_t last_frame_mapped = UINT64_MAX;
	const uint32_t max_extra_buffers = MAX_EXTRA_BUFFERS;
#ifdef DEBUG_ENABLED
	const char* debug_name;
#endif

	BufferInfo buffer_info[NUM_BUFFERS];
#ifdef DEV_ENABLED
	bool can_upload[NUM_BUFFERS] = {};
#endif

	void push()
	{
		for (uint32_t i = 0u; i < NUM_BUFFERS; ++i) {
			const BufferInfo& info = buffer_info[i];
			RID buffer;
			switch (info.type) {
			case MultiUmaBufferType::STORAGE:
				buffer = RD::storage_buffer_create(
					info.size_bytes, Vector<uint8_t>(), RD::BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT);
				break;
			case MultiUmaBufferType::VERTEX:
				buffer = RD::vertex_buffer_create(
					info.size_bytes, Vector<uint8_t>(), RD::BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT);
				break;
			case MultiUmaBufferType::UNIFORM:
			default:
				buffer = RD::uniform_buffer_create(
					info.size_bytes, Vector<uint8_t>(), RD::BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT);
				break;
			}
			buffers.push_back(buffer);
		}
	}

public:
	MultiUmaBuffer(const char* p_debug_name)
		: max_extra_buffers(MAX_EXTRA_BUFFERS)
#ifdef DEBUG_ENABLED
		  ,
		  debug_name(p_debug_name)
#endif
	{
	}

#ifdef DEV_ENABLED
	~MultiUmaBuffer() { DEV_ASSERT(buffers.is_empty() && "Forgot to call uninit()!"); }
#endif

	void uninit()
	{
		if (is_print_verbose_enabled()) {
			// 			print_line("MultiUmaBuffer '"
			// #ifdef DEBUG_ENABLED
			// 					+ String(debug_name) +
			// #else
			// 					   "{DEBUG_ENABLED unavailable}"
			// #endif
			// 					"' used a total of " + itos(buffers.size()) +
			// 					" buffers. A large number may indicate a waste of VRAM and can be brought
			// down by tweaking MAX_EXTRA_BUFFERS for this buffer.");
		}

		for (RID buffer : buffers) {
			if (buffer.is_valid()) {
				RD::free_rid(buffer);
			}
		}

		buffers.clear();
	}

	void shrink_to_max_extra_buffers()
	{
		DEV_ASSERT(curr_idx == 0u && "This function can only be called after reset and before "
									 "being upload_and_advance again!");

		uint32_t elem_count = buffers.size();

		if (elem_count > max_extra_buffers) {
			if (is_print_verbose_enabled()) {
				// 				print_line("MultiUmaBuffer '"
				// #ifdef DEBUG_ENABLED
				// 						+ String(debug_name) +
				// #else
				// 						   "{DEBUG_ENABLED unavailable}"
				// #endif
				// 						"' peaked to " + itos(elem_count) + " elements and shrinking it to "
				// + itos(max_extra_buffers) +
				// 						". If you see this message often, then something is wrong with rendering
				// or MAX_EXTRA_BUFFERS needs to be increased.");
			}
		}

		while (elem_count > max_extra_buffers) {
			--elem_count;
			if (buffers[elem_count].is_valid()) {
				RD::free_rid(buffers[elem_count]);
			}
			buffers.remove_at(elem_count);
		}
	}

	uint32_t get_curr_idx() const { return curr_idx; }

	void set_size(uint32_t p_idx, uint32_t p_size_bytes, MultiUmaBufferType p_type)
	{
		DEV_ASSERT(buffers.is_empty());
		buffer_info[p_idx].size_bytes = p_size_bytes;
		buffer_info[p_idx].type = p_type;
		curr_idx = UINT32_MAX;
		last_frame_mapped = UINT64_MAX;
	}

	void set_size(uint32_t p_idx, uint32_t p_size_bytes, bool p_is_storage)
	{
		set_size(p_idx, p_size_bytes,
			p_is_storage ? MultiUmaBufferType::STORAGE : MultiUmaBufferType::UNIFORM);
	}

	void set_uniform_size(uint32_t p_idx, uint32_t p_size_bytes)
	{
		set_size(p_idx, p_size_bytes, MultiUmaBufferType::UNIFORM);
	}

	void set_storage_size(uint32_t p_idx, uint32_t p_size_bytes)
	{
		set_size(p_idx, p_size_bytes, MultiUmaBufferType::STORAGE);
	}

	void set_vertex_size(uint32_t p_idx, uint32_t p_size_bytes)
	{
		set_size(p_idx, p_size_bytes, MultiUmaBufferType::VERTEX);
	}

	uint32_t get_size(uint32_t p_idx) const { return buffer_info[p_idx].size_bytes; }

	// Gets the raw buffer. Use with care.
	// If you call this function, make sure to have called prepare_for_upload() first.
	// Do not call _get() then prepare_for_upload().
	RID _get(uint32_t p_idx) { return buffers[curr_idx * NUM_BUFFERS + p_idx]; }

	/**
	 * @param p_append	True if you wish to append more data to existing buffer.
	 * @return			False if it's possible to append. True if the internal buffer changed.
	 */
	bool prepare_for_map(bool p_append)
	{
		const uint64_t frames_drawn = RD::get_frames_drawn();

		if (last_frame_mapped == frames_drawn) {
			if (!p_append) {
				++curr_idx;
			}
		}
		else {
			p_append = false;
			curr_idx = 0u;
			if (max_extra_buffers != UINT32_MAX) {
				shrink_to_max_extra_buffers();
			}
		}
		last_frame_mapped = frames_drawn;
		if (curr_idx * NUM_BUFFERS >= buffers.size()) {
			push();
		}

#ifdef DEV_ENABLED
		if (!p_append) {
			for (size_t i = 0u; i < NUM_BUFFERS; ++i) {
				can_upload[i] = true;
			}
		}
#endif
		return !p_append;
	}

	void prepare_for_upload() { prepare_for_map(false); }

	void* map_raw_for_upload(uint32_t p_idx)
	{
#ifdef DEV_ENABLED
		DEV_ASSERT(can_upload[p_idx] &&
				   "Forgot to prepare_for_upload first! Or called get_for_upload/upload() twice.");
		can_upload[p_idx] = false;
#endif
		return RD::buffer_persistent_map_advance(buffers[curr_idx * NUM_BUFFERS + p_idx]);
	}

	RID get_for_upload(uint32_t p_idx)
	{
#ifdef DEV_ENABLED
		DEV_ASSERT(can_upload[p_idx] &&
				   "Forgot to prepare_for_upload first! Or called get_for_upload/upload() twice.");
		can_upload[p_idx] = false;
#endif
		return buffers[curr_idx * NUM_BUFFERS + p_idx];
	}
};



// --- End servers/rendering/multi_uma_buffer.h ---

// --- Folded from servers/rendering/renderer_rd/forward_clustered/render_forward_clustered.h ---
#include "servers/rendering/renderer_rd/shaders/forward_clustered/scene_forward_clustered.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "core/templates/paged_allocator.h"

#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/effects/upscale_effects.h"

#include "servers/rendering/renderer_rd/shaders/forward_clustered/best_fit_normal.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/forward_clustered/integrate_dfg.glsl.gen.h"
#include "servers/rendering/renderer_rd/rd_caches.h"

namespace RendererSceneRenderImplementation
{

class SceneShaderForwardClustered final
{
private:
	static inline Mutex shader_mutex;

public:
	enum ShaderGroup
	{
		SHADER_GROUP_BASE, // Always compiled at the beginning.
		SHADER_GROUP_ADVANCED,
		SHADER_GROUP_MULTIVIEW,
		SHADER_GROUP_ADVANCED_MULTIVIEW,
	};

	// Not an enum because these values are constants that are processed as numbers
	// to arrive at a unique version for a particular shader.
	struct ShaderVersion
	{
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS = 0;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_DP = 1;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS = 2;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS_AND_VOXEL_GI =
			3;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_MULTIVIEW = 4;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS_MULTIVIEW = 5;
		constexpr static uint16_t
			SHADER_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS_AND_VOXEL_GI_MULTIVIEW = 6;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_WITH_MATERIAL = 7;
		constexpr static uint16_t SHADER_VERSION_DEPTH_PASS_WITH_SDF = 8;
		constexpr static uint16_t SHADER_VERSION_COLOR_PASS = 9;
	};

	enum ShaderColorPassFlags
	{
		SHADER_COLOR_PASS_FLAG_UBERSHADER = 1 << 0,
		SHADER_COLOR_PASS_FLAG_SEPARATE_SPECULAR = 1 << 1,
		SHADER_COLOR_PASS_FLAG_LIGHTMAP = 1 << 2,
		SHADER_COLOR_PASS_FLAG_MULTIVIEW = 1 << 3,
		SHADER_COLOR_PASS_FLAG_MOTION_VECTORS = 1 << 4,
		SHADER_COLOR_PASS_FLAG_COUNT = 1 << 5
	};

	enum PipelineVersion
	{
		PIPELINE_VERSION_DEPTH_PASS,
		PIPELINE_VERSION_DEPTH_PASS_DP,
		PIPELINE_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS,
		PIPELINE_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS_AND_VOXEL_GI,
		PIPELINE_VERSION_DEPTH_PASS_WITH_MATERIAL,
		PIPELINE_VERSION_DEPTH_PASS_WITH_SDF,
		PIPELINE_VERSION_DEPTH_PASS_MULTIVIEW,
		PIPELINE_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS_MULTIVIEW,
		PIPELINE_VERSION_DEPTH_PASS_WITH_NORMAL_AND_ROUGHNESS_AND_VOXEL_GI_MULTIVIEW,
		PIPELINE_VERSION_COLOR_PASS,
		PIPELINE_VERSION_MAX
	};

	enum PipelineColorPassFlags
	{
		PIPELINE_COLOR_PASS_FLAG_TRANSPARENT = 1 << 0, // Can't combine with SEPARATE_SPECULAR.
		PIPELINE_COLOR_PASS_FLAG_SEPARATE_SPECULAR = 1 << 1, // Can't combine with TRANSPARENT.
		PIPELINE_COLOR_PASS_FLAG_LIGHTMAP = 1 << 2,
		PIPELINE_COLOR_PASS_FLAG_MULTIVIEW = 1 << 3,
		PIPELINE_COLOR_PASS_FLAG_MOTION_VECTORS = 1 << 4,
		PIPELINE_COLOR_PASS_FLAG_OPTIONS = 5,
		PIPELINE_COLOR_PASS_FLAG_COMBINATIONS = 1 << PIPELINE_COLOR_PASS_FLAG_OPTIONS,
	};

	struct ShaderSpecialization
	{
		union
		{
			uint32_t packed_0;

			struct
			{
				uint32_t use_forward_gi : 1;
				uint32_t use_light_projector : 1;
				uint32_t use_light_soft_shadows : 1;
				uint32_t use_directional_soft_shadows : 1;
				uint32_t decal_use_mipmaps : 1;
				uint32_t projector_use_mipmaps : 1;
				uint32_t use_depth_fog : 1;
				uint32_t use_lightmap_bicubic_filter : 1;
				uint32_t soft_shadow_samples : 6;
				uint32_t penumbra_shadow_samples : 6;
				uint32_t directional_soft_shadow_samples : 6;
				uint32_t directional_penumbra_shadow_samples : 6;
			};
		};

		union
		{
			uint32_t packed_1;

			struct
			{
				uint32_t multimesh : 1;
				uint32_t multimesh_format_2d : 1;
				uint32_t multimesh_has_color : 1;
				uint32_t multimesh_has_custom_data : 1;
				uint32_t fog_use_legacy_blending : 1;
				uint32_t cluster_has_area_light : 1;
			};
		};

		uint32_t packed_2;
	};

	struct UbershaderConstants
	{
		union
		{
			uint32_t packed_0;

			struct
			{
				uint32_t cull_mode : 2;
			};
		};
	};

	struct ShaderData : public RendererRD::MaterialStorage::ShaderData
	{
		enum DepthDraw
		{
			DEPTH_DRAW_DISABLED,
			DEPTH_DRAW_OPAQUE,
			DEPTH_DRAW_ALWAYS
		};

		enum DepthTest
		{
			DEPTH_TEST_DISABLED,
			DEPTH_TEST_ENABLED,
			DEPTH_TEST_ENABLED_INVERTED,
		};

		enum CullVariant
		{
			CULL_VARIANT_NORMAL,
			CULL_VARIANT_REVERSED,
			CULL_VARIANT_DOUBLE_SIDED,
			CULL_VARIANT_MAX

		};

		enum AlphaAntiAliasing
		{
			ALPHA_ANTIALIASING_OFF,
			ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE,
			ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE
		};

		enum StencilFlags
		{
			STENCIL_FLAG_READ = 1,
			STENCIL_FLAG_WRITE = 2,
			STENCIL_FLAG_WRITE_DEPTH_FAIL = 4,
		};

		enum StencilCompare
		{
			STENCIL_COMPARE_LESS,
			STENCIL_COMPARE_EQUAL,
			STENCIL_COMPARE_LESS_OR_EQUAL,
			STENCIL_COMPARE_GREATER,
			STENCIL_COMPARE_NOT_EQUAL,
			STENCIL_COMPARE_GREATER_OR_EQUAL,
			STENCIL_COMPARE_ALWAYS,
			STENCIL_COMPARE_MAX // Not an actual operator, just the amount of operators.
		};

		struct PipelineKey
		{
			RD::VertexFormatID vertex_format_id;
			RD::FramebufferFormatID framebuffer_format_id;
			RDC::PolygonCullMode cull_mode = RDC::POLYGON_CULL_MAX;
			RSE::PrimitiveType primitive_type = RSE::PRIMITIVE_MAX;
			PipelineVersion version = PipelineVersion::PIPELINE_VERSION_MAX;
			uint32_t color_pass_flags = 0;
			ShaderSpecialization shader_specialization = {};
			uint32_t wireframe = false;
			uint32_t ubershader = false;

			uint32_t hash() const
			{
				uint32_t h = hash_murmur3_one_64(vertex_format_id);
				h = hash_murmur3_one_32(framebuffer_format_id, h);
				h = hash_murmur3_one_32(cull_mode, h);
				h = hash_murmur3_one_32(primitive_type, h);
				h = hash_murmur3_one_32(version, h);
				h = hash_murmur3_one_32(color_pass_flags, h);
				h = hash_murmur3_one_32(shader_specialization.packed_0, h);
				h = hash_murmur3_one_32(shader_specialization.packed_1, h);
				h = hash_murmur3_one_32(shader_specialization.packed_2, h);
				h = hash_murmur3_one_32(wireframe, h);
				h = hash_murmur3_one_32(ubershader, h);
				return hash_fmix32(h);
			}
		};

		void _create_pipeline(PipelineKey p_pipeline_key);
		PipelineHashMapRD<PipelineKey, ShaderData, void (ShaderData::*)(PipelineKey)>
			pipeline_hash_map;

		RID version;

		static const uint32_t VERTEX_INPUT_MASKS_SIZE =
			ShaderVersion::SHADER_VERSION_COLOR_PASS * 2 + SHADER_COLOR_PASS_FLAG_COUNT;
		std::atomic<uint64_t> vertex_input_masks[VERTEX_INPUT_MASKS_SIZE] = {};

		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size = 0;

		String code;

		DepthDraw depth_draw = DEPTH_DRAW_OPAQUE;
		DepthTest depth_test = DEPTH_TEST_ENABLED;

		int blend_mode = BLEND_MODE_MIX;
		int depth_test_disabledi = 0;
		int depth_test_invertedi = 0;
		int alpha_antialiasing_mode = ALPHA_ANTIALIASING_OFF;

		bool uses_point_size = false;
		bool uses_alpha = false;
		bool uses_blend_alpha = false;
		bool uses_alpha_clip = false;
		bool uses_alpha_antialiasing = false;
		bool uses_depth_prepass_alpha = false;
		bool uses_discard = false;
		bool uses_roughness = false;
		bool uses_normal = false;
		bool uses_tangent = false;
		bool writes_tangent = false;
		bool uses_particle_trails = false;
		bool uses_normal_map = false;
		bool uses_bent_normal_map = false;
		bool wireframe = false;

		bool unshaded = false;
		bool uses_vertex = false;
		bool uses_position = false;
		bool uses_sss = false;
		bool uses_transmittance = false;
		bool uses_screen_texture = false;
		bool uses_depth_texture = false;
		bool uses_normal_texture = false;
		bool uses_time = false;
		bool uses_vertex_time = false;
		bool uses_fragment_time = false;
		bool writes_modelview_or_projection = false;
		bool uses_world_coordinates = false;
		bool uses_screen_texture_mipmaps = false;
		bool uses_z_clip_scale = false;
		RSE::CullMode cull_mode = RSE::CULL_MODE_DISABLED;

		bool stencil_enabled = false;
		uint32_t stencil_flags = 0;
		StencilCompare stencil_compare = STENCIL_COMPARE_LESS;
		uint32_t stencil_reference = 0;

		uint64_t last_pass = 0;
		uint32_t index = 0;

		_FORCE_INLINE_ bool uses_alpha_pass() const
		{
			bool has_read_screen_alpha =
				uses_screen_texture || uses_depth_texture || uses_normal_texture;
			bool has_base_alpha = (uses_alpha && (!uses_alpha_clip || uses_alpha_antialiasing)) ||
								  has_read_screen_alpha;
			bool has_blend_alpha = uses_blend_alpha;
			bool has_alpha = has_base_alpha || has_blend_alpha;
			bool no_depth_draw = depth_draw == DEPTH_DRAW_DISABLED;
			bool no_depth_test = depth_test != DEPTH_TEST_ENABLED;
			return has_alpha || has_read_screen_alpha || no_depth_draw || no_depth_test;
		}

		_FORCE_INLINE_ bool uses_depth_in_alpha_pass() const
		{
			bool no_depth_draw = depth_draw == DEPTH_DRAW_DISABLED;
			bool no_depth_test = depth_test != DEPTH_TEST_ENABLED;
			return (uses_depth_prepass_alpha || uses_alpha_antialiasing) &&
				   !(no_depth_draw || no_depth_test);
		}

		_FORCE_INLINE_ bool uses_shared_shadow_material() const
		{
			bool backface_culling = cull_mode == RSE::CULL_MODE_BACK;
			return !uses_particle_trails && !writes_modelview_or_projection && !uses_vertex &&
				   !uses_position && !uses_discard && !uses_depth_prepass_alpha &&
				   !uses_alpha_clip && !uses_alpha_antialiasing && backface_culling &&
				   !uses_point_size && !uses_world_coordinates && !wireframe &&
				   !uses_z_clip_scale && !stencil_enabled;
		}

		virtual bool is_animated() const;
		virtual void set_code(const String& p_Code);
		virtual bool casts_shadows() const;
		virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const;
		uint16_t _get_shader_version(PipelineVersion p_pipeline_version,
			uint32_t p_color_pass_flags, bool p_ubershader) const;
		RID _get_shader_variant(uint16_t p_shader_version) const;
		void _clear_vertex_input_mask_cache();
		RID get_shader_variant(PipelineVersion p_pipeline_version, uint32_t p_color_pass_flags,
			bool p_ubershader) const;
		uint64_t get_vertex_input_mask(
			PipelineVersion p_pipeline_version, uint32_t p_color_pass_flags, bool p_ubershader);
		RDC::PolygonCullMode get_cull_mode_from_cull_variant(CullVariant p_cull_variant);
		bool is_valid() const;

		SelfList<ShaderData> shader_list_element;
		ShaderData();
		virtual ~ShaderData() = default;
	};

	static RendererRD::MaterialStorage::ShaderData* _create_shader_func();

	static RendererRD::MaterialStorage::ShaderData* _create_shader_funcs()
	{
		return _create_shader_func();
	}

	struct MaterialData : public RendererRD::MaterialStorage::MaterialData
	{
		ShaderData* shader_data = nullptr;
		RID uniform_set;
		uint64_t last_pass = 0;
		uint32_t index = 0;
		RID next_pass;
		uint8_t priority;
		virtual void set_render_priority(int p_priority);
		virtual void set_next_pass(RID p_pass);
		virtual ~MaterialData();
	};

	static RendererRD::MaterialStorage::MaterialData* _create_material_func(ShaderData* p_shader);

	static RendererRD::MaterialStorage::MaterialData* _create_material_funcs(
		RendererRD::MaterialStorage::ShaderData* p_shader)
	{
		return _create_material_func(static_cast<ShaderData*>(p_shader));
	}

	struct Data
	{
		SelfList<ShaderData>::List shader_list;
		ShaderRD shader{SceneForwardClusteredShaderRD()};
		ShaderCompiler compiler;
		bool emulate_point_size = false;

		RID default_shader;
		RID default_material;
		RID overdraw_material_shader;
		RID overdraw_material;
		RID debug_shadow_splits_material_shader;
		RID debug_shadow_splits_material;
		RID default_shader_rd;
		RID default_multiview_shader_rd; // This is lazily initialized, use "get_default_shader_rd"
										 // instead.
		RID default_shader_sdfgi_rd;

		RID default_vec4_xform_buffer;
		RID default_vec4_xform_uniform_set;

		RID shadow_sampler;

		RID default_material_uniform_set;
		ShaderData* default_material_shader_ptr = nullptr;

		RID overdraw_material_uniform_set;
		ShaderData* overdraw_material_shader_ptr = nullptr;

		RID debug_shadow_splits_material_uniform_set;
		ShaderData* debug_shadow_splits_material_shader_ptr = nullptr;

		ShaderSpecialization default_specialization = {};

		uint32_t pipeline_compilations[RSE::PIPELINE_SOURCE_MAX] = {};
	};

	static inline Data* data = nullptr;

	static void initialize(const String& p_defines = String());
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static void init(const String p_defines);
	static void set_default_specialization(const ShaderSpecialization& p_specialization);
	static void enable_multiview_shader_group();
	static void enable_advanced_shader_group(bool p_needs_multiview = false);
	static bool is_multiview_shader_group_enabled();
	static bool is_advanced_shader_group_enabled(bool p_multiview);
	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);
	static RID get_default_shader_rd(bool p_is_multiview = false);

	SceneShaderForwardClustered() = delete;
	SceneShaderForwardClustered(const SceneShaderForwardClustered&) = delete;
	SceneShaderForwardClustered& operator=(const SceneShaderForwardClustered&) = delete;
	~SceneShaderForwardClustered() = delete;
};

} // namespace RendererSceneRenderImplementation

#define RB_SCOPE_FORWARD_CLUSTERED SNAME("forward_clustered")

#define RB_TEX_SPECULAR SNAME("specular")
#define RB_TEX_SPECULAR_MSAA SNAME("specular_msaa")
#define RB_TEX_NORMAL_ROUGHNESS SNAME("normal_roughness")
#define RB_TEX_NORMAL_ROUGHNESS_MSAA SNAME("normal_roughness_msaa")
#define RB_TEX_VOXEL_GI SNAME("voxel_gi")
#define RB_TEX_VOXEL_GI_MSAA SNAME("voxel_gi_msaa")

namespace RendererSceneRenderImplementation
{

class RenderForwardClustered final
{
	friend SceneShaderForwardClustered;

	enum
	{
		SCENE_UNIFORM_SET = 0,
		RENDER_PASS_UNIFORM_SET = 1,
		TRANSFORMS_UNIFORM_SET = 2,
		MATERIAL_UNIFORM_SET = 3,
	};

	enum
	{
		SDFGI_MAX_CASCADES = 8,
		MAX_VOXEL_GI_INSTANCESS = 8,
		MAX_LIGHTMAPS = 8,
		MAX_VOXEL_GI_INSTANCESS_PER_INSTANCE = 2,
		INSTANCE_DATA_BUFFER_MIN_SIZE = 4096
	};

	enum RenderListType
	{
		RENDER_LIST_OPAQUE,	   // used for opaque objects
		RENDER_LIST_MOTION,	   // used for opaque objects with motion
		RENDER_LIST_ALPHA,	   // used for transparent objects
		RENDER_LIST_SECONDARY, // used for shadows and other objects
		RENDER_LIST_MAX
	};


public:
	/* Framebuffer */

	class RenderBufferDataForwardClustered final : public RefCounted
	{

	private:
		RenderSceneBuffersRD* render_buffers = nullptr;
		RendererRD::FSR2Context* fsr2_context = nullptr;
#ifdef METAL_MFXTEMPORAL_ENABLED
		RendererRD::MFXTemporalContext* mfx_temporal_context = nullptr;
#endif

	public:
		ClusterBuilderRD* cluster_builder = nullptr;

		struct SSEffectsData
		{
			Projection ssil_last_frame_projections[RendererSceneRender::MAX_RENDER_VIEWS];
			Transform3D ssil_last_frame_transform;

			Projection ssr_last_frame_projections[RendererSceneRender::MAX_RENDER_VIEWS];
			Transform3D ssr_last_frame_transform;

			RendererRD::SSEffects::SSILRenderBuffers ssil;
			RendererRD::SSEffects::SSAORenderBuffers ssao;
			RendererRD::SSEffects::SSRRenderBuffers ssr;
		} ss_effects_data;

		enum DepthFrameBufferType
		{
			DEPTH_FB,
			DEPTH_FB_ROUGHNESS,
			DEPTH_FB_ROUGHNESS_VOXELGI
		};

		RID render_sdfgi_uniform_set;

		void ensure_specular();

		bool has_specular() const
		{
			return render_buffers->has_texture(RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_SPECULAR);
		}

		RID get_specular() const
		{
			return render_buffers->get_texture(RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_SPECULAR);
		}

		RID get_specular(uint32_t p_layer)
		{
			return render_buffers->get_texture_slice(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_SPECULAR, p_layer, 0);
		}

		RID get_specular_msaa(uint32_t p_layer)
		{
			return render_buffers->get_texture_slice(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_SPECULAR_MSAA, p_layer, 0);
		}

		void ensure_normal_roughness_texture();

		bool has_normal_roughness() const
		{
			return render_buffers->has_texture(RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_NORMAL_ROUGHNESS);
		}

		RID get_normal_roughness() const
		{
			return render_buffers->get_texture(RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_NORMAL_ROUGHNESS);
		}

		RID get_normal_roughness(uint32_t p_layer)
		{
			return render_buffers->get_texture_slice(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_NORMAL_ROUGHNESS, p_layer, 0);
		}

		RID get_normal_roughness_msaa() const
		{
			return render_buffers->get_texture(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_NORMAL_ROUGHNESS_MSAA);
		}

		RID get_normal_roughness_msaa(uint32_t p_layer)
		{
			return render_buffers->get_texture_slice(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_NORMAL_ROUGHNESS_MSAA, p_layer, 0);
		}

		void ensure_voxelgi();

		bool has_voxelgi() const
		{
			return render_buffers->has_texture(RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_VOXEL_GI);
		}

		RID get_voxelgi() const
		{
			return render_buffers->get_texture(RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_VOXEL_GI);
		}

		RID get_voxelgi(uint32_t p_layer)
		{
			return render_buffers->get_texture_slice(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_VOXEL_GI, p_layer, 0);
		}

		RID get_voxelgi_msaa(uint32_t p_layer)
		{
			return render_buffers->get_texture_slice(
				RB_SCOPE_FORWARD_CLUSTERED, RB_TEX_VOXEL_GI_MSAA, p_layer, 0);
		}

		void ensure_fsr2(RendererRD::FSR2Effect* p_effect);

		RendererRD::FSR2Context* get_fsr2_context() const { return fsr2_context; }

#ifdef METAL_MFXTEMPORAL_ENABLED
		bool ensure_mfx_temporal(RendererRD::MFXTemporalEffect* p_effect);

		RendererRD::MFXTemporalContext* get_mfx_temporal_context() const
		{
			return mfx_temporal_context;
		}
#endif

		RID get_color_only_fb();
		RID get_color_pass_fb(uint32_t p_color_pass_flags);
		RID get_depth_fb(DepthFrameBufferType p_type = DEPTH_FB);
		RID get_specular_only_fb();
		RID get_velocity_only_fb();

		void configure(RenderSceneBuffersRD* p_render_buffers);
		void free_data();

		static RDC::DataFormat get_specular_format();
		static uint32_t get_specular_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
		static RDC::DataFormat get_normal_roughness_format();
		static uint32_t get_normal_roughness_usage_bits(
			bool p_resolve, bool p_msaa, bool p_storage);
		static RDC::DataFormat get_voxelgi_format();
		static uint32_t get_voxelgi_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	};

private:
	static void setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers);


	static void _update_render_base_uniform_set();
	static RID _setup_sdfgi_render_pass_uniform_set(RID p_albedo_texture, RID p_emission_texture,
		RID p_emission_aniso_texture, RID p_geom_facing_texture,
		const RendererRD::Samplers& p_samplers, uint32_t p_uniform_buffer_index);
	static RID _setup_render_pass_uniform_set(RenderListType p_render_list,
		const RenderDataRD* p_render_data, bool p_is_multiview, RID p_radiance_texture,
		const RendererRD::Samplers& p_samplers, uint32_t p_uniform_buffer_index,
		bool p_use_directional_shadow_atlas = false);

	struct BestFitNormal
	{
		ShaderRD shader{BestFitNormalShaderRD()};
		RID shader_version;
		RID pipeline;
		RID texture;
	};

	struct IntegrateDFG
	{
		ShaderRD shader{IntegrateDfgShaderRD()};
		RID shader_version;
		RID pipeline;
		RID texture;
	};

	struct LTC
	{
		RID lut1_texture;
		RID lut2_texture;
	};

	enum PassMode
	{
		PASS_MODE_COLOR,
		PASS_MODE_SHADOW,
		PASS_MODE_SHADOW_DP,
		PASS_MODE_DEPTH,
		PASS_MODE_DEPTH_NORMAL_ROUGHNESS,
		PASS_MODE_DEPTH_NORMAL_ROUGHNESS_VOXEL_GI,
		PASS_MODE_DEPTH_MATERIAL,
		PASS_MODE_SDF,
		PASS_MODE_MAX
	};

	enum ColorPassFlags
	{
		COLOR_PASS_FLAG_TRANSPARENT = 1 << 0,
		COLOR_PASS_FLAG_SEPARATE_SPECULAR = 1 << 1,
		COLOR_PASS_FLAG_MULTIVIEW = 1 << 2,
		COLOR_PASS_FLAG_MOTION_VECTORS = 1 << 3,
	};

	struct GeometryInstanceSurfaceDataCache;
	struct RenderElementInfo;

	struct RenderListParameters
	{
		GeometryInstanceSurfaceDataCache** elements = nullptr;
		RenderElementInfo* element_info = nullptr;
		int element_count = 0;
		bool reverse_cull = false;
		PassMode pass_mode = PASS_MODE_COLOR;
		uint32_t color_pass_flags = 0;
		bool no_gi = false;
		uint32_t view_count = 1;
		RID render_pass_uniform_set;
		bool force_wireframe = false;
		Vector2 uv_offset;
		float lod_distance_multiplier = 0.0;
		float screen_mesh_lod_threshold = 0.0;
		RD::FramebufferFormatID framebuffer_format = 0;
		uint32_t element_offset = 0;
		bool use_directional_soft_shadow = false;
		SceneShaderForwardClustered::ShaderSpecialization base_specialization = {};

		RenderListParameters(GeometryInstanceSurfaceDataCache** p_elements,
			RenderElementInfo* p_element_info, int p_element_count, bool p_reverse_cull,
			PassMode p_pass_mode, uint32_t p_color_pass_flags, bool p_no_gi,
			bool p_use_directional_soft_shadows, RID p_render_pass_uniform_set,
			bool p_force_wireframe = false, const Vector2& p_uv_offset = Vector2(),
			float p_lod_distance_multiplier = 0.0, float p_screen_mesh_lod_threshold = 0.0,
			uint32_t p_view_count = 1, uint32_t p_element_offset = 0,
			SceneShaderForwardClustered::ShaderSpecialization p_base_specialization = {})
		{
			elements = p_elements;
			element_info = p_element_info;
			element_count = p_element_count;
			reverse_cull = p_reverse_cull;
			pass_mode = p_pass_mode;
			color_pass_flags = p_color_pass_flags;
			no_gi = p_no_gi;
			view_count = p_view_count;
			render_pass_uniform_set = p_render_pass_uniform_set;
			force_wireframe = p_force_wireframe;
			uv_offset = p_uv_offset;
			lod_distance_multiplier = p_lod_distance_multiplier;
			screen_mesh_lod_threshold = p_screen_mesh_lod_threshold;
			element_offset = p_element_offset;
			use_directional_soft_shadow = p_use_directional_soft_shadows;
			base_specialization = p_base_specialization;
		}
	};

	struct LightmapData
	{
		float normal_xform[12];
		float texture_size[2];
		float exposure_normalization;
		uint32_t flags;
	};

	struct LightmapCaptureData
	{
		float sh[9 * 4];
	};

	// When changing any of these enums, remember to change the corresponding enums in the shader
	// files as well.
	enum
	{
		INSTANCE_DATA_FLAG_MULTIMESH_INDIRECT = 1 << 2,
		INSTANCE_DATA_FLAGS_DYNAMIC = 1 << 3,
		INSTANCE_DATA_FLAGS_NON_UNIFORM_SCALE = 1 << 4,
		INSTANCE_DATA_FLAG_USE_GI_BUFFERS = 1 << 5,
		INSTANCE_DATA_FLAG_USE_SDFGI = 1 << 6,
		INSTANCE_DATA_FLAG_USE_LIGHTMAP_CAPTURE = 1 << 7,
		INSTANCE_DATA_FLAG_USE_LIGHTMAP = 1 << 8,
		INSTANCE_DATA_FLAG_USE_SH_LIGHTMAP = 1 << 9,
		INSTANCE_DATA_FLAG_USE_VOXEL_GI = 1 << 10,
		INSTANCE_DATA_FLAG_PARTICLES = 1 << 11,
		INSTANCE_DATA_FLAG_MULTIMESH = 1 << 12,
		INSTANCE_DATA_FLAG_MULTIMESH_FORMAT_2D = 1 << 13,
		INSTANCE_DATA_FLAG_MULTIMESH_HAS_COLOR = 1 << 14,
		INSTANCE_DATA_FLAG_MULTIMESH_HAS_CUSTOM_DATA = 1 << 15,
		INSTANCE_DATA_FLAGS_PARTICLE_TRAIL_SHIFT = 16,
		INSTANCE_DATA_FLAGS_PARTICLE_TRAIL_MASK = 0xFF,
		INSTANCE_DATA_FLAGS_FADE_SHIFT = 24,
		INSTANCE_DATA_FLAGS_FADE_MASK = 0xFFUL << INSTANCE_DATA_FLAGS_FADE_SHIFT
	};

	struct SceneState
	{
		// This struct is loaded into Set 1 - Binding 1, populated at start of rendering a frame,
		// must match with shader code
		struct UBO
		{
			uint32_t cluster_shift;
			uint32_t cluster_width;
			uint32_t cluster_type_size;
			uint32_t max_cluster_element_count_div_32;

			uint32_t ss_effects_flags;
			float ssao_light_affect;
			float ssao_ao_affect;
			uint32_t pad1;

			float sdf_to_bounds[16];

			int32_t sdf_offset[3];
			uint32_t pad2;

			int32_t sdf_size[3];
			uint32_t gi_upscale_for_msaa;

			uint32_t volumetric_fog_enabled;
			float volumetric_fog_inv_length;
			float volumetric_fog_detail_spread;
			uint32_t volumetric_fog_pad;
		};

		struct PushConstantUbershader
		{
			SceneShaderForwardClustered::ShaderSpecialization specialization;
			SceneShaderForwardClustered::UbershaderConstants constants;
		};

		struct PushConstant
		{
			uint32_t base_index; //
			uint32_t uv_offset;	 // packed
			uint32_t multimesh_motion_vectors_current_offset;
			uint32_t multimesh_motion_vectors_previous_offset;
			PushConstantUbershader ubershader;
		};

		struct InstanceData
		{
			float transform[12];
			float compressed_aabb_position[4];
			float compressed_aabb_size[4];
			float uv_scale[4];
			uint32_t flags;
			uint32_t instance_uniforms_ofs; // base offset in global buffer for instance variables
			uint32_t gi_offset; // GI information when using lightmapping (VCT or lightmap index)
			uint32_t layer_mask;
			float prev_transform[12];
			float lightmap_uv_scale[4];
#ifdef REAL_T_IS_DOUBLE
			float model_precision[4];
			float prev_model_precision[4];
#endif

			// These setters allow us to copy the data over with operation when using floats.
			inline void set_lightmap_uv_scale(const Rect2& p_rect)
			{
#ifdef REAL_T_IS_DOUBLE
				lightmap_uv_scale[0] = p_rect.position.x;
				lightmap_uv_scale[1] = p_rect.position.y;
				lightmap_uv_scale[2] = p_rect.size.x;
				lightmap_uv_scale[3] = p_rect.size.y;
#else
				Rect2* rect = reinterpret_cast<Rect2*>(lightmap_uv_scale);
				*rect = p_rect;
#endif
			}

			inline void set_compressed_aabb(const AABB& p_aabb)
			{
#ifdef REAL_T_IS_DOUBLE
				compressed_aabb_position[0] = p_aabb.position.x;
				compressed_aabb_position[1] = p_aabb.position.y;
				compressed_aabb_position[2] = p_aabb.position.z;

				compressed_aabb_size[0] = p_aabb.size.x;
				compressed_aabb_size[1] = p_aabb.size.y;
				compressed_aabb_size[2] = p_aabb.size.z;
#else
				Vector3* compressed_aabb_position_vec3 =
					reinterpret_cast<Vector3*>(compressed_aabb_position);
				Vector3* compressed_aabb_size_vec3 =
					reinterpret_cast<Vector3*>(compressed_aabb_size);
				*compressed_aabb_position_vec3 = p_aabb.position;
				*compressed_aabb_size_vec3 = p_aabb.size;
#endif
			}

			inline void set_uv_scale(const Vector4& p_uv_scale)
			{
#ifdef REAL_T_IS_DOUBLE
				uv_scale[0] = p_uv_scale.x;
				uv_scale[1] = p_uv_scale.y;
				uv_scale[2] = p_uv_scale.z;
				uv_scale[3] = p_uv_scale.w;
#else
				Vector4* uv_scale_vec4 = reinterpret_cast<Vector4*>(uv_scale);
				*uv_scale_vec4 = p_uv_scale;
#endif
			}
		};

		static_assert(std::is_trivially_destructible_v<InstanceData>);
		static_assert(std::is_trivially_constructible_v<InstanceData>);

		UBO ubo;

		LocalVector<RID> uniform_buffers;
		LocalVector<RID> implementation_uniform_buffers;
		uint32_t used_uniform_buffer_count = 0;

		LightmapData lightmaps[MAX_LIGHTMAPS];
		RID lightmap_ids[MAX_LIGHTMAPS];
		bool lightmap_has_sh[MAX_LIGHTMAPS];
		uint32_t lightmaps_used = 0;
		uint32_t max_lightmaps;
		RID lightmap_buffer;

		MultiUmaBuffer<1u> instance_buffer[RENDER_LIST_MAX] = {
			MultiUmaBuffer<1u>("RENDER_LIST_OPAQUE"), MultiUmaBuffer<1u>("RENDER_LIST_MOTION"),
			MultiUmaBuffer<1u>("RENDER_LIST_ALPHA"), MultiUmaBuffer<1u>("RENDER_LIST_SECONDARY")};
		InstanceData* curr_gpu_ptr[RENDER_LIST_MAX] = {};

		LightmapCaptureData* lightmap_captures = nullptr;
		uint32_t max_lightmap_captures;
		RID lightmap_capture_buffer;

		RID voxelgi_ids[MAX_VOXEL_GI_INSTANCESS];
		uint32_t voxelgis_used = 0;

		bool used_screen_texture = false;
		bool used_normal_texture = false;
		bool used_depth_texture = false;
		bool used_sss = false;
		bool used_lightmap = false;
		bool used_opaque_stencil = false;

		struct ShadowPass
		{
			uint32_t element_from;
			uint32_t element_count;
			PassMode pass_mode;

			RID rp_uniform_set;
			float lod_distance_multiplier;
			float screen_mesh_lod_threshold;

			RID framebuffer;
			Rect2i rect;
			bool clear_depth;
			bool flip_cull;

			uint32_t uniform_buffer_index;
		};

		LocalVector<ShadowPass> shadow_passes;

		void grow_instance_buffer(
			RenderListType p_render_list, uint32_t p_req_element_count, bool p_append);
	};

	static uint32_t _setup_environment(const RenderDataRD* p_render_data, bool p_no_fog,
		const Size2i& p_screen_size, const Size2& p_viewport_size, const Color& p_default_bg_color,
		bool p_opaque_render_buffers = false, bool p_apply_alpha_multiplier = false,
		bool p_pancake_shadows = false);
	static void _setup_voxelgis(const PagedArray<RID>& p_voxelgis);
	static void _setup_lightmaps(const RenderDataRD* p_render_data, const PagedArray<RID>& p_lightmaps,
		const Transform3D& p_cam_transform);

	struct RenderElementInfo
	{
		enum
		{
			MAX_REPEATS = (1 << 20) - 1
		};

		union
		{
			struct
			{
				uint32_t lod_index : 8;
				uint32_t uses_softshadow : 1;
				uint32_t uses_projector : 1;
				uint32_t uses_forward_gi : 1;
				uint32_t uses_lightmap : 1;
			};

			uint32_t value;
		};

		uint32_t repeat;
	};

	static_assert(std::is_trivially_destructible_v<RenderElementInfo>);
	static_assert(std::is_trivially_constructible_v<RenderElementInfo>);

	template <PassMode p_pass_mode, uint32_t p_color_pass_flags = 0>
	static _FORCE_INLINE_ void _render_list_template(RenderingDevice::DrawListID p_draw_list,
		RenderingDevice::FramebufferFormatID p_framebuffer_Format, RenderListParameters* p_params,
		uint32_t p_from_element, uint32_t p_to_element);
	static void _render_list(RenderingDevice::DrawListID p_draw_list,
		RenderingDevice::FramebufferFormatID p_framebuffer_Format, RenderListParameters* p_params,
		uint32_t p_from_element, uint32_t p_to_element);
	static void _render_list_with_draw_list(RenderListParameters* p_params, RID p_framebuffer,
		uint32_t p_draw_flags = RD::DRAW_DEFAULT_ALL,
		const Vector<Color>& p_clear_color_values = Vector<Color>(),
		float p_clear_depth_value = 0.0, uint32_t p_clear_stencil_value = 0,
		const Rect2& p_region = Rect2());

	static void _fill_instance_data(RenderListType p_render_list, int* p_render_info = nullptr,
		uint32_t p_offset = 0, int32_t p_max_elements = -1, bool p_update_buffer = true);
	static void _fill_render_list(RenderListType p_render_list, const RenderDataRD* p_render_data,
		PassMode p_pass_mode, bool p_using_sdfgi = false, bool p_using_opaque_gi = false,
		bool p_using_motion_pass = false, bool p_append = false);


	struct GeometryInstanceData;
	class GeometryInstanceForwardClustered;

	struct GeometryInstanceLightmapSH
	{
		Color sh[9];
	};

	// Cached data for drawing surfaces
	struct GeometryInstanceSurfaceDataCache
	{
		enum
		{
			FLAG_PASS_DEPTH = 1,
			FLAG_PASS_OPAQUE = 2,
			FLAG_PASS_ALPHA = 4,
			FLAG_PASS_SHADOW = 8,
			FLAG_USES_SHARED_SHADOW_MATERIAL = 128,
			FLAG_USES_SUBSURFACE_SCATTERING = 2048,
			FLAG_USES_SCREEN_TEXTURE = 4096,
			FLAG_USES_DEPTH_TEXTURE = 8192,
			FLAG_USES_NORMAL_TEXTURE = 16384,
			FLAG_USES_DOUBLE_SIDED_SHADOWS = 32768,
			FLAG_USES_PARTICLE_TRAILS = 65536,
			FLAG_USES_MOTION_VECTOR = 131072,
			FLAG_USES_STENCIL = 262144,
		};

		union
		{
			struct
			{
				uint64_t sort_key1;
				uint64_t sort_key2;
			};

			struct
			{
				// Needs to be grouped together to be used in RenderElementInfo, as the value is
				// masked directly.
				uint64_t lod_index : 8;
				uint64_t uses_softshadow : 1;
				uint64_t uses_projector : 1;
				uint64_t uses_forward_gi : 1;
				uint64_t uses_lightmap : 1;

				// Sorted based on optimal order for respecting priority and reducing the amount of
				// rebinding of shaders, materials, and geometry. This current order was found to be
				// the most optimal in large projects. If you wish to measure differences, refer to
				// RenderingDeviceGraph and the methods available to print statistics for draw
				// lists.
				uint64_t depth_layer : 4;
				uint64_t surface_index : 8;
				uint64_t geometry_id : 32;
				uint64_t material_id_hi : 8;

				uint64_t material_id_lo : 24;
				uint64_t shader_id : 32;
				uint64_t priority : 8;
			};
		} sort;

		RSE::PrimitiveType primitive = RSE::PRIMITIVE_MAX;
		uint32_t flags = 0;
		uint32_t surface_index = 0;
		uint32_t color_pass_inclusion_mask = 0;

		void* surface = nullptr;
		RID material_uniform_set;
		SceneShaderForwardClustered::ShaderData* shader = nullptr;
		SceneShaderForwardClustered::MaterialData* material = nullptr;

		void* surface_shadow = nullptr;
		RID material_uniform_set_shadow;
		SceneShaderForwardClustered::ShaderData* shader_shadow = nullptr;

		GeometryInstanceSurfaceDataCache* next = nullptr;
		GeometryInstanceForwardClustered* owner = nullptr;
		SelfList<GeometryInstanceSurfaceDataCache> compilation_dirty_element;
		SelfList<GeometryInstanceSurfaceDataCache> compilation_all_element;

		GeometryInstanceSurfaceDataCache()
			: compilation_dirty_element(this), compilation_all_element(this)
		{
		}
	};

	class GeometryInstanceForwardClustered : public RenderGeometryInstance
	{
	public:
		// lightmap
		RID lightmap_instance;
		Rect2 lightmap_uv_scale;
		uint32_t lightmap_slice_index;
		GeometryInstanceLightmapSH* lightmap_sh = nullptr;

		// used during rendering

		uint32_t gi_offset_cache = 0;
		bool store_transform_cache = true;
		RID transforms_uniform_set;
		uint32_t instance_count = 0;
		uint32_t trail_steps = 1;
		bool can_sdfgi = false;
		bool using_projectors = false;
		bool using_softshadows = false;

		// used during setup
		uint64_t prev_transform_change_frame = 0xFFFFFFFF;

		enum TransformStatus
		{
			NONE,
			MOVED,
			TELEPORTED,
		} transform_status = TransformStatus::MOVED;

		Transform3D prev_transform;
		RID voxel_gi_instances[MAX_VOXEL_GI_INSTANCESS_PER_INSTANCE];
		GeometryInstanceSurfaceDataCache* surface_caches = nullptr;
		SelfList<GeometryInstanceForwardClustered> dirty_list_element;

		GeometryInstanceForwardClustered() : dirty_list_element(this) {}

		virtual void _mark_dirty() override;

		virtual void set_transform(const Transform3D& p_transform, const AABB& p_aabb,
			const AABB& p_transformed_aabb) override;
		virtual void reset_motion_vectors() override;
		virtual void set_use_lightmap(RID p_lightmap_instance, const Rect2& p_lightmap_uv_scale,
			int p_lightmap_slice_index) override;
		virtual void set_lightmap_capture(const Color* p_sh9) override;

		virtual void pair_voxel_gi_instances(
			const RID* p_voxel_gi_instances, uint32_t p_voxel_gi_instance_count) override;

		virtual void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) override;
	};

	// These are not used in the Forward+ path, it has different light clustering tech.
	static uint32_t get_max_lights_total() { return 0; }

	static uint32_t get_max_lights_per_mesh() { return 0; }

	static void _geometry_instance_dependency_changed(
		Dependency::DependencyChangedNotification p_notification, DependencyTracker* p_tracker);
	static void _geometry_instance_dependency_deleted(
		const RID& p_dependency, DependencyTracker* p_tracker);


	struct SurfacePipelineData
	{
		void* mesh_surface = nullptr;
		void* mesh_surface_shadow = nullptr;
		SceneShaderForwardClustered::ShaderData* shader = nullptr;
		SceneShaderForwardClustered::ShaderData* shader_shadow = nullptr;
		bool instanced = false;
		bool uses_opaque = false;
		bool uses_transparent = false;
		bool uses_depth = false;
		bool can_use_lightmap = false;
	};

	struct GlobalPipelineData
	{
		union
		{
			uint32_t key;

			struct
			{
				uint32_t texture_samples : 3;
				uint32_t use_reflection_probes : 1;
				uint32_t use_separate_specular : 1;
				uint32_t use_motion_vectors : 1;
				uint32_t use_normal_and_roughness : 1;
				uint32_t use_lightmaps : 1;
				uint32_t use_voxelgi : 1;
				uint32_t use_sdfgi : 1;
				uint32_t use_multiview : 1;
				uint32_t use_16_bit_shadows : 1;
				uint32_t use_32_bit_shadows : 1;
				uint32_t use_shadow_cubemaps : 1;
				uint32_t use_shadow_dual_paraboloid : 1;
			};
		};
	};


	typedef Pair<SceneShaderForwardClustered::ShaderData*,
		SceneShaderForwardClustered::ShaderData::PipelineKey>
		ShaderPipelinePair;

	static void _update_global_pipeline_data_requirements_from_project();
	static void _update_global_pipeline_data_requirements_from_light_storage();
	static void _geometry_instance_add_surface_with_material(GeometryInstanceForwardClustered* ginstance,
		uint32_t p_surface, SceneShaderForwardClustered::MaterialData* p_material,
		uint32_t p_material_id, uint32_t p_shader_id, RID p_mesh);
	static void _geometry_instance_add_surface_with_material_chain(
		GeometryInstanceForwardClustered* ginstance, uint32_t p_surface,
		SceneShaderForwardClustered::MaterialData* p_material, RID p_mat_src, RID p_mesh);
	static void _geometry_instance_add_surface(GeometryInstanceForwardClustered* ginstance,
		uint32_t p_surface, RID p_material, RID p_mesh);
	static void _geometry_instance_update(RenderGeometryInstance* p_geometry_instance);
	static void _mesh_compile_pipeline_for_surface(SceneShaderForwardClustered::ShaderData* p_shader,
		void* p_mesh_surface, bool p_ubershader, bool p_instanced_surface,
		RSE::PipelineSource p_source,
		SceneShaderForwardClustered::ShaderData::PipelineKey& r_pipeline_key,
		Vector<ShaderPipelinePair>* r_pipeline_pairs = nullptr);
	static void _mesh_compile_pipelines_for_surface(const SurfacePipelineData& p_surface,
		const GlobalPipelineData& p_global, RSE::PipelineSource p_source,
		Vector<ShaderPipelinePair>* r_pipeline_pairs = nullptr);
	static void _mesh_generate_all_pipelines_for_surface_cache(
		GeometryInstanceSurfaceDataCache* p_surface_cache, const GlobalPipelineData& p_global);
	static void _update_dirty_geometry_instances();
	static void _update_dirty_geometry_pipelines();

	// Global data about the scene that can be used to pre-allocate resources without relying on
	// culling.
	struct GlobalSurfaceData
	{
		bool screen_texture_used = false;
		bool normal_texture_used = false;
		bool depth_texture_used = false;
		bool sss_used = false;
	};

	/* Render List */

	struct RenderList
	{
		LocalVector<GeometryInstanceSurfaceDataCache*> elements;
		LocalVector<RenderElementInfo> element_info;

		void clear()
		{
			elements.clear();
			element_info.clear();
		}

		// should eventually be replaced by radix

		struct SortByKey
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				return (A->sort.sort_key2 == B->sort.sort_key2)
						   ? (A->sort.sort_key1 < B->sort.sort_key1)
						   : (A->sort.sort_key2 < B->sort.sort_key2);
			}
		};

		void sort_by_key()
		{
			SortArray<GeometryInstanceSurfaceDataCache*, SortByKey> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		void sort_by_key_range(uint32_t p_from, uint32_t p_size)
		{
			SortArray<GeometryInstanceSurfaceDataCache*, SortByKey> sorter;
			sorter.sort(elements.ptr() + p_from, p_size);
		}

		struct SortByDepth
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				return (A->owner->depth < B->owner->depth);
			}
		};

		void sort_by_depth()
		{ // used for shadows

			SortArray<GeometryInstanceSurfaceDataCache*, SortByDepth> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		struct SortByReverseDepthAndPriority
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				return (A->sort.priority == B->sort.priority)
						   ? (A->owner->depth > B->owner->depth)
						   : (A->sort.priority < B->sort.priority);
			}
		};

		void sort_by_reverse_depth_and_priority()
		{ // used for alpha

			SortArray<GeometryInstanceSurfaceDataCache*, SortByReverseDepthAndPriority> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		_FORCE_INLINE_ void add_element(GeometryInstanceSurfaceDataCache* p_element)
		{
			elements.push_back(p_element);
		}
	};


	static void _update_shader_quality_settings();


	/* SDFGI */
	static void _update_sdfgi(RenderDataRD* p_render_data);

	/* Volumetric fog */

	static void _update_volumetric_fog(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_environment,
		const Projection& p_cam_projection, const Transform3D& p_cam_transform,
		const Transform3D& p_prev_cam_inv_transform, RID p_shadow_atlas,
		int p_directional_light_count, bool p_use_directional_shadows, int p_positional_light_count,
		int p_voxel_gi_count, const PagedArray<RID>& p_fog_volumes);

	/* Render shadows */

	static void _render_shadow_pass(RID p_light, RID p_shadow_atlas, int p_pass,
		const PagedArray<RenderGeometryInstance*>& p_instances, float p_lod_distance_multiplier = 0,
		float p_screen_mesh_lod_threshold = 0.0, bool p_open_pass = true, bool p_close_pass = true,
		bool p_clear_region = true, RenderingServerTypes::RenderInfo* p_render_info = nullptr,
		const Size2i& p_viewport_size = Size2i(1, 1),
		const Transform3D& p_main_cam_transform = Transform3D());
	static void _render_shadow_begin();
	static void _render_shadow_append(RID p_framebuffer,
		const PagedArray<RenderGeometryInstance*>& p_instances, const Projection& p_projection,
		const Transform3D& p_transform, float p_zfar, float p_bias, float p_normal_bias,
		bool p_reverse_cull_face, bool p_use_dp, bool p_use_dp_flip, bool p_use_pancake,
		float p_lod_distance_multiplier = 0.0, float p_screen_mesh_lod_threshold = 0.0,
		const Rect2i& p_rect = Rect2i(), bool p_flip_y = false, bool p_clear_region = true,
		bool p_begin = true, bool p_end = true,
		RenderingServerTypes::RenderInfo* p_render_info = nullptr,
		const Size2i& p_viewport_size = Size2i(1, 1),
		const Transform3D& p_main_cam_transform = Transform3D());
	static void _render_shadow_process();
	static void _render_shadow_end();

	/* Render Scene */
	static void _process_ssao(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_environment,
		const RID* p_normal_buffers, const Projection* p_projections);
	static void _process_ssil(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_environment,
		const RID* p_normal_buffers, const Projection* p_projections,
		const Transform3D& p_transform);
	static void _process_ssr(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_environment,
		const RID* p_normal_slices, const Projection* p_projections, const Vector3* p_eye_offsets,
		const Transform3D& p_transform);
	static void _copy_framebuffer_to_ss_effects(
		Ref<RenderSceneBuffersRD> p_render_buffers, bool p_use_ssil, bool p_use_ssr);
	static void _pre_opaque_render(RenderDataRD* p_render_data, bool p_use_ssao, bool p_use_ssil,
		bool p_use_ssr, bool p_use_gi, const RID* p_normal_roughness_slices, RID p_voxel_gi_buffer);
	static void _process_sss(Ref<RenderSceneBuffersRD> p_render_buffers, const Projection& p_camera);

	/* Debug */
	static void _debug_draw_cluster(Ref<RenderSceneBuffersRD> p_render_buffers);

protected:
	/* setup */

	static RID _render_buffers_get_normal_texture(Ref<RenderSceneBuffersRD> p_render_buffers);
	static RID _render_buffers_get_velocity_texture(Ref<RenderSceneBuffersRD> p_render_buffers);

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);
	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);
	static void environment_set_ssr_half_size(bool p_half_size);
	static void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality);

	static void sub_surface_scattering_set_quality(
		RSE::SubSurfaceScatteringQuality p_quality);
	static void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale);

	/* Rendering */

	static void _render_scene(RenderDataRD* p_render_data, const Color& p_default_bg_color);
	static void _render_buffers_debug_draw(const RenderDataRD* p_render_data);

	static void _render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region, float p_exposure_normalization);
	static void _render_uv2(const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region);
	static void _render_sdfgi(Ref<RenderSceneBuffersRD> p_render_buffers, const Vector3i& p_from,
		const Vector3i& p_size, const AABB& p_bounds,
		const PagedArray<RenderGeometryInstance*>& p_instances, const RID& p_albedo_texture,
		const RID& p_emission_texture, const RID& p_emission_aniso_texture,
		const RID& p_geom_facing_texture, float p_exposure_normalization);
	static void _render_particle_collider_heightfield(RID p_fb, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, const PagedArray<RenderGeometryInstance*>& p_instances);

public:
	struct Data
	{
		RID render_base_uniform_set;
		uint64_t lightmap_texture_array_version = 0xFFFFFFFF;
		BestFitNormal best_fit_normal;
		IntegrateDFG dfg_lut;
		LTC ltc;
		SceneState scene_state;
		HashMap<Size2i, RID> sdfgi_framebuffer_size_cache;

		SelfList<GeometryInstanceForwardClustered>::List geometry_instance_dirty_list;
		SelfList<GeometryInstanceSurfaceDataCache>::List geometry_surface_compilation_dirty_list;
		SelfList<GeometryInstanceSurfaceDataCache>::List geometry_surface_compilation_all_list;

		PagedAllocator<GeometryInstanceForwardClustered> geometry_instance_alloc;
		PagedAllocator<GeometryInstanceSurfaceDataCache> geometry_instance_surface_alloc;
		PagedAllocator<GeometryInstanceLightmapSH> geometry_instance_lightmap_sh;

		GlobalPipelineData global_pipeline_data_compiled = {};
		GlobalPipelineData global_pipeline_data_required = {};
		GlobalSurfaceData global_surface_data;

		RenderList render_list[RENDER_LIST_MAX];

		RendererRD::TAA* taa = nullptr;
		RendererRD::FSR2Effect* fsr2_effect = nullptr;
#ifdef METAL_MFXTEMPORAL_ENABLED
		RendererRD::MFXTemporalEffect* mfx_temporal_effect = nullptr;
#endif
		RendererRD::MotionVectorsStore* motion_vectors_store = nullptr;

		ClusterBuilderSharedDataRD cluster_builder_shared;
		ClusterBuilderRD* current_cluster_builder = nullptr;

		RID shadow_sampler;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static ClusterBuilderSharedDataRD* get_cluster_builder_shared()
	{
		return &data->cluster_builder_shared;
	}

	/* callback from updating our lighting UBOs, used to populate cluster builder */
	static void setup_added_reflection_probe(
		const Transform3D& p_transform, const Vector3& p_half_size);
	static void setup_added_light(const RSE::LightType p_type, const Transform3D& p_transform,
		float p_radius, float p_spot_aperture, const Vector2& p_area_size);
	static void setup_added_decal(const Transform3D& p_transform, const Vector3& p_half_size);

	static void base_uniforms_changed();

	/* SDFGI UPDATE */

	static void sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_environment,
		const Vector3& p_world_position);
	static int sdfgi_get_pending_region_count(
		const Ref<RenderSceneBuffers>& p_render_buffers);
	static AABB sdfgi_get_pending_region_bounds(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region);
	static uint32_t sdfgi_get_pending_region_cascade(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region);

	static RID sdfgi_get_ubo() { return RendererRD::GI::sdfgi_ubo; }

	/* GEOMETRY INSTANCE */

	static RenderGeometryInstance* geometry_instance_create(RID p_base);
	static void geometry_instance_free(RenderGeometryInstance* p_geometry_instance);

	static uint32_t geometry_instance_get_pair_mask();

	/* PIPELINES */

	static void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation);
	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	/* SHADER LIBRARY */

	static void enable_features(uint32_t p_feature_bits);
	static String get_name();

	static bool free(RID p_rid);

	static void update();

	RenderForwardClustered() = delete;
	RenderForwardClustered(const RenderForwardClustered&) = delete;
	RenderForwardClustered& operator=(const RenderForwardClustered&) = delete;
	~RenderForwardClustered() = delete;
};
} // namespace RendererSceneRenderImplementation

// --- End servers/rendering/renderer_rd/forward_clustered/render_forward_clustered.h ---

// --- Folded from servers/rendering/renderer_rd/forward_mobile/render_forward_mobile.h ---
#include "servers/rendering/renderer_rd/shaders/forward_mobile/scene_forward_mobile.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "core/templates/paged_allocator.h"


#include "servers/rendering/renderer_rd/rd_caches.h"

namespace RendererSceneRenderImplementation
{

class SceneShaderForwardMobile final
{
private:
	static inline Mutex shader_mutex;

public:
	enum ShaderVersion
	{
		SHADER_VERSION_COLOR_PASS,
		SHADER_VERSION_LIGHTMAP_COLOR_PASS,
		SHADER_VERSION_SHADOW_PASS,
		SHADER_VERSION_SHADOW_PASS_DP,
		SHADER_VERSION_DEPTH_PASS_WITH_MATERIAL,

		SHADER_VERSION_COLOR_PASS_MULTIVIEW,
		SHADER_VERSION_LIGHTMAP_COLOR_PASS_MULTIVIEW,
		SHADER_VERSION_SHADOW_PASS_MULTIVIEW,
		SHADER_VERSION_MOTION_VECTORS_MULTIVIEW,

		SHADER_VERSION_MAX
	};

	enum ShaderCount
	{
		SHADER_COUNT_NONE,
		SHADER_COUNT_SINGLE,
		SHADER_COUNT_MULTIPLE
	};

	_FORCE_INLINE_ static ShaderCount shader_count_for(uint32_t p_count)
	{
		if (p_count == 0) {
			return SHADER_COUNT_NONE;
		}
		else if (p_count == 1) {
			return SHADER_COUNT_SINGLE;
		}
		else {
			return SHADER_COUNT_MULTIPLE;
		}
	}

	enum ShaderGroup
	{
		SHADER_GROUP_FP32,
		SHADER_GROUP_FP32_MULTIVIEW,
		SHADER_GROUP_FP16,
		SHADER_GROUP_FP16_MULTIVIEW,
	};

	struct ShaderSpecialization
	{
		union
		{
			uint32_t packed_0;

			struct
			{
				uint32_t use_light_projector : 1;
				uint32_t use_light_soft_shadows : 1;
				uint32_t use_directional_soft_shadows : 1;
				uint32_t decal_use_mipmaps : 1;

				uint32_t projector_use_mipmaps : 1;
				uint32_t disable_fog : 1;
				uint32_t use_depth_fog : 1;
				uint32_t use_fog_aerial_perspective : 1;

				uint32_t use_fog_sun_scatter : 1;
				uint32_t use_fog_height_density : 1;
				uint32_t use_lightmap_bicubic_filter : 1;
				uint32_t use_material_debanding : 1;

				uint32_t multimesh : 1;
				uint32_t multimesh_format_2d : 1;
				uint32_t multimesh_has_color : 1;
				uint32_t multimesh_has_custom_data : 1;

				uint32_t scene_use_ambient_cubemap : 1;
				uint32_t scene_use_reflection_cubemap : 1;
				uint32_t scene_roughness_limiter_enabled : 1;
				uint32_t padding_0 : 1;

				uint32_t soft_shadow_samples : 6;
				uint32_t penumbra_shadow_samples : 6;
			};
		};

		union
		{
			uint32_t packed_1;

			struct
			{
				uint32_t directional_soft_shadow_samples : 6;
				uint32_t directional_penumbra_shadow_samples : 6;
				uint32_t omni_lights : 2;
				uint32_t spot_lights : 2;
				uint32_t area_lights : 2;
				uint32_t reflection_probes : 2;
				uint32_t directional_lights : 2;
				uint32_t decals : 1;
				uint32_t directional_light_blend_splits : 8;
				uint32_t padding_1 : 1;
			};
		};

		union
		{
			float packed_2;
			float luminance_multiplier;
		};
	};

	struct UbershaderConstants
	{
		union
		{
			uint32_t packed_0;

			struct
			{
				uint32_t cull_mode : 2;
			};
		};
	};

	struct ShaderData : public RendererRD::MaterialStorage::ShaderData
	{
		enum DepthDraw
		{
			DEPTH_DRAW_DISABLED,
			DEPTH_DRAW_OPAQUE,
			DEPTH_DRAW_ALWAYS
		};

		enum DepthTest
		{
			DEPTH_TEST_DISABLED,
			DEPTH_TEST_ENABLED,
			DEPTH_TEST_ENABLED_INVERTED,
		};

		enum CullVariant
		{
			CULL_VARIANT_NORMAL,
			CULL_VARIANT_REVERSED,
			CULL_VARIANT_DOUBLE_SIDED,
			CULL_VARIANT_MAX

		};

		enum AlphaAntiAliasing
		{
			ALPHA_ANTIALIASING_OFF,
			ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE,
			ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE
		};

		enum StencilFlags
		{
			STENCIL_FLAG_READ = 1,
			STENCIL_FLAG_WRITE = 2,
			STENCIL_FLAG_WRITE_DEPTH_FAIL = 4,
		};

		enum StencilCompare
		{
			STENCIL_COMPARE_LESS,
			STENCIL_COMPARE_EQUAL,
			STENCIL_COMPARE_LESS_OR_EQUAL,
			STENCIL_COMPARE_GREATER,
			STENCIL_COMPARE_NOT_EQUAL,
			STENCIL_COMPARE_GREATER_OR_EQUAL,
			STENCIL_COMPARE_ALWAYS,
			STENCIL_COMPARE_MAX // Not an actual operator, just the amount of operators.
		};

		struct PipelineKey
		{
			RD::VertexFormatID vertex_format_id;
			RD::FramebufferFormatID framebuffer_format_id;
			RDC::PolygonCullMode cull_mode = RDC::POLYGON_CULL_MAX;
			RSE::PrimitiveType primitive_type = RSE::PRIMITIVE_MAX;
			ShaderSpecialization shader_specialization = {};
			ShaderVersion version = SHADER_VERSION_MAX;
			uint32_t render_pass = 0;
			uint32_t wireframe = false;
			uint32_t ubershader = false;

			uint32_t hash() const
			{
				uint32_t h = hash_murmur3_one_32(vertex_format_id);
				h = hash_murmur3_one_32(framebuffer_format_id, h);
				h = hash_murmur3_one_32(cull_mode, h);
				h = hash_murmur3_one_32(primitive_type, h);
				h = hash_murmur3_one_32(shader_specialization.packed_0, h);
				h = hash_murmur3_one_32(shader_specialization.packed_1, h);
				h = hash_murmur3_one_float(shader_specialization.packed_2, h);
				h = hash_murmur3_one_32(version, h);
				h = hash_murmur3_one_32(render_pass, h);
				h = hash_murmur3_one_32(wireframe, h);
				h = hash_murmur3_one_32(ubershader, h);
				return hash_fmix32(h);
			}
		};

		void _create_pipeline(PipelineKey p_pipeline_key);
		PipelineHashMapRD<PipelineKey, ShaderData, void (ShaderData::*)(PipelineKey)>
			pipeline_hash_map;

		RID version;

		static const uint32_t VERTEX_INPUT_MASKS_SIZE = SHADER_VERSION_MAX * 2;
		std::atomic<uint64_t> vertex_input_masks[VERTEX_INPUT_MASKS_SIZE] = {};

		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size = 0;

		String code;

		DepthDraw depth_draw;
		DepthTest depth_test;

		int blend_mode = BLEND_MODE_MIX;
		int depth_test_disabledi = 0;
		int depth_test_invertedi = 0;
		int alpha_antialiasing_mode = ALPHA_ANTIALIASING_OFF;
		int cull_mode = RSE::CULL_MODE_BACK;

		bool uses_point_size = false;
		bool uses_alpha = false;
		bool uses_blend_alpha = false;
		bool uses_alpha_clip = false;
		bool uses_alpha_antialiasing = false;
		bool uses_depth_prepass_alpha = false;
		bool uses_discard = false;
		bool uses_roughness = false;
		bool uses_normal = false;
		bool uses_tangent = false;
		bool writes_tangent = false;
		bool uses_particle_trails = false;
		bool uses_normal_map = false;
		bool uses_bent_normal_map = false;
		bool wireframe = false;

		bool unshaded = false;
		bool uses_vertex = false;
		bool uses_sss = false;
		bool uses_transmittance = false;
		bool uses_screen_texture = false;
		bool uses_depth_texture = false;
		bool uses_normal_texture = false;
		bool uses_screen_texture_mipmaps = false;
		bool uses_time = false;
		bool uses_vertex_time = false;
		bool uses_fragment_time = false;
		bool writes_modelview_or_projection = false;
		bool uses_world_coordinates = false;

		bool stencil_enabled = false;
		uint32_t stencil_flags = 0;
		StencilCompare stencil_compare = STENCIL_COMPARE_LESS;
		uint32_t stencil_reference = 0;

		uint64_t last_pass = 0;
		uint32_t index = 0;

		_FORCE_INLINE_ bool uses_alpha_pass() const
		{
			bool has_read_screen_alpha =
				uses_screen_texture || uses_depth_texture || uses_normal_texture;
			bool has_base_alpha = (uses_alpha && (!uses_alpha_clip || uses_alpha_antialiasing));
			bool has_blend_alpha = uses_blend_alpha;
			bool has_alpha = has_base_alpha || has_blend_alpha;
			bool no_depth_draw = depth_draw == DEPTH_DRAW_DISABLED;
			bool no_depth_test = depth_test != DEPTH_TEST_ENABLED;
			return has_alpha || has_read_screen_alpha || no_depth_draw || no_depth_test;
		}

		_FORCE_INLINE_ bool uses_depth_in_alpha_pass() const
		{
			bool no_depth_draw = depth_draw == DEPTH_DRAW_DISABLED;
			bool no_depth_test = depth_test != DEPTH_TEST_ENABLED;
			return (uses_depth_prepass_alpha || uses_alpha_antialiasing) &&
				   !(no_depth_draw || no_depth_test);
		}

		_FORCE_INLINE_ bool uses_shared_shadow_material() const
		{
			bool backface_culling = cull_mode == RSE::CULL_MODE_BACK;
			return !uses_particle_trails && !writes_modelview_or_projection && !uses_vertex &&
				   !uses_discard && !uses_depth_prepass_alpha && !uses_alpha_clip &&
				   !uses_alpha_antialiasing && !uses_point_size && !uses_world_coordinates &&
				   !wireframe && !stencil_enabled && backface_culling;
		}

		virtual bool is_animated() const;
		virtual void set_code(const String& p_Code);
		virtual bool casts_shadows() const;
		virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const;
		RDC::PolygonCullMode get_cull_mode_from_cull_variant(CullVariant p_cull_variant);
		void _clear_vertex_input_mask_cache();
		RID get_shader_variant(ShaderVersion p_shader_version, bool p_ubershader) const;
		uint64_t get_vertex_input_mask(ShaderVersion p_shader_version, bool p_ubershader);
		bool is_valid() const;

		SelfList<ShaderData> shader_list_element;

		ShaderData();
		virtual ~ShaderData() = default;
	};

	static RendererRD::MaterialStorage::ShaderData* _create_shader_func();

	static RendererRD::MaterialStorage::ShaderData* _create_shader_funcs()
	{
		return _create_shader_func();
	}

	struct MaterialData : public RendererRD::MaterialStorage::MaterialData
	{
		ShaderData* shader_data = nullptr;
		RID uniform_set;
		uint64_t last_pass = 0;
		uint32_t index = 0;
		RID next_pass;
		uint8_t priority;
		virtual void set_render_priority(int p_priority);
		virtual void set_next_pass(RID p_pass);
		virtual ~MaterialData();
	};

	static RendererRD::MaterialStorage::MaterialData* _create_material_func(ShaderData* p_shader);

	static RendererRD::MaterialStorage::MaterialData* _create_material_funcs(
		RendererRD::MaterialStorage::ShaderData* p_shader)
	{
		return _create_material_func(static_cast<ShaderData*>(p_shader));
	}

	struct Data
	{
		SelfList<ShaderData>::List shader_list;
		ShaderRD shader{SceneForwardMobileShaderRD()};
		ShaderCompiler compiler;
		bool use_fp16 = false;
		bool emulate_point_size = false;

		RID default_shader;
		RID default_material;
		RID overdraw_material_shader;
		RID overdraw_material;
		RID debug_shadow_splits_material_shader;
		RID debug_shadow_splits_material;
		RID default_shader_rd;
		RID default_multiview_shader_rd; // This is lazily initialized, use "get_default_shader_rd"
										 // instead.

		RID default_vec4_xform_buffer;
		RID default_vec4_xform_uniform_set;

		RID shadow_sampler;

		RID default_material_uniform_set;
		ShaderData* default_material_shader_ptr = nullptr;

		RID overdraw_material_uniform_set;
		ShaderData* overdraw_material_shader_ptr = nullptr;

		RID debug_shadow_splits_material_uniform_set;
		ShaderData* debug_shadow_splits_material_shader_ptr = nullptr;

		ShaderSpecialization default_specialization = {};

		uint32_t pipeline_compilations[RSE::PIPELINE_SOURCE_MAX] = {};
	};

	static inline Data* data = nullptr;

	static void initialize(const String& p_defines = String());
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static void init(const String p_defines);
	static void set_default_specialization(const ShaderSpecialization& p_specialization);
	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);
	static void enable_fp32_shader_group();
	static void enable_fp16_shader_group();
	static void enable_multiview_shader_group();
	static bool is_multiview_shader_group_enabled();
	static RID get_default_shader_rd(bool p_is_multiview = false);

	SceneShaderForwardMobile() = delete;
	SceneShaderForwardMobile(const SceneShaderForwardMobile&) = delete;
	SceneShaderForwardMobile& operator=(const SceneShaderForwardMobile&) = delete;
	~SceneShaderForwardMobile() = delete;
};

} // namespace RendererSceneRenderImplementation

#define RB_SCOPE_MOBILE SNAME("mobile")

namespace RendererSceneRenderImplementation
{

class RenderForwardMobile final
{
	friend SceneShaderForwardMobile;

protected:
	struct GeometryInstanceSurfaceDataCache;

private:

	/* Scene Shader */

	enum
	{
		SCENE_UNIFORM_SET = 0,
		RENDER_PASS_UNIFORM_SET = 1,
		TRANSFORMS_UNIFORM_SET = 2,
		MATERIAL_UNIFORM_SET = 3,
	};

	enum
	{
		MAX_LIGHTMAPS = 8,
		MAX_RDL_CULL = 8, // maximum number of reflection probes, decals or lights we can cull per
						  // geometry instance
		INSTANCE_DATA_BUFFER_MIN_SIZE = 4096
	};

	enum RenderListType
	{
		RENDER_LIST_OPAQUE,	   // used for opaque objects
		RENDER_LIST_ALPHA,	   // used for transparent objects
		RENDER_LIST_SECONDARY, // used for shadows and other objects
		RENDER_LIST_MAX
	};


	/* Render Buffer */

	class RenderBufferDataForwardMobile final : public RefCounted
	{

	public:
		enum FramebufferConfigType
		{
			FB_CONFIG_RENDER_PASS,			// Single pass framebuffer for normal rendering.
			FB_CONFIG_RENDER_AND_POST_PASS, // Two subpasses, one for normal rendering, one for post
											// processing.
			FB_CONFIG_MAX
		};

		RID get_color_fbs(FramebufferConfigType p_config_type, bool p_resolve_depth = false);
		void free_data();
		void configure(RenderSceneBuffersRD* p_render_buffers);

		RID get_motion_vectors_fb();

	private:
		RenderSceneBuffersRD* render_buffers = nullptr;
	};

	static void setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers);

	struct LTC
	{
		RID lut1_texture;
		RID lut2_texture;
	};

	/* Rendering */

	enum PassMode
	{
		PASS_MODE_COLOR,
		// PASS_MODE_COLOR_SPECULAR,
		PASS_MODE_COLOR_TRANSPARENT,
		PASS_MODE_SHADOW,
		PASS_MODE_SHADOW_DP,
		// PASS_MODE_DEPTH,
		// PASS_MODE_DEPTH_NORMAL_ROUGHNESS,
		// PASS_MODE_DEPTH_NORMAL_ROUGHNESS_VOXEL_GI,
		PASS_MODE_DEPTH_MATERIAL,
		// PASS_MODE_SDF,
		PASS_MODE_MOTION_VECTORS,
	};

	struct RenderElementInfo;

	struct RenderListParameters
	{
		GeometryInstanceSurfaceDataCache** elements = nullptr;
		RenderElementInfo* element_info = nullptr;
		int element_count = 0;
		bool reverse_cull = false;
		PassMode pass_mode = PASS_MODE_COLOR;
		// bool no_gi = false;
		uint32_t view_count = 1;
		RID render_pass_uniform_set;
		bool force_wireframe = false;
		Vector2 uv_offset;
		SceneShaderForwardMobile::ShaderSpecialization base_specialization;
		float lod_distance_multiplier = 0.0;
		float screen_mesh_lod_threshold = 0.0;
		RD::FramebufferFormatID framebuffer_format = 0;
		uint32_t element_offset = 0;
		uint32_t subpass = 0;

		RenderListParameters(GeometryInstanceSurfaceDataCache** p_elements,
			RenderElementInfo* p_element_info, int p_element_count, bool p_reverse_cull,
			PassMode p_pass_mode, RID p_render_pass_uniform_set,
			SceneShaderForwardMobile::ShaderSpecialization p_base_specialization,
			bool p_force_wireframe = false, const Vector2& p_uv_offset = Vector2(),
			float p_lod_distance_multiplier = 0.0, float p_screen_mesh_lod_threshold = 0.0,
			uint32_t p_view_count = 1, uint32_t p_element_offset = 0)
		{
			elements = p_elements;
			element_info = p_element_info;
			element_count = p_element_count;
			reverse_cull = p_reverse_cull;
			pass_mode = p_pass_mode;
			// no_gi = p_no_gi;
			view_count = p_view_count;
			render_pass_uniform_set = p_render_pass_uniform_set;
			force_wireframe = p_force_wireframe;
			uv_offset = p_uv_offset;
			lod_distance_multiplier = p_lod_distance_multiplier;
			screen_mesh_lod_threshold = p_screen_mesh_lod_threshold;
			element_offset = p_element_offset;
			base_specialization = p_base_specialization;
		}
	};

	/* Render shadows */

	static void _render_shadow_pass(RID p_light, RID p_shadow_atlas, int p_pass,
		const PagedArray<RenderGeometryInstance*>& p_instances, float p_lod_distance_multiplier = 0,
		float p_screen_mesh_lod_threshold = 0.0, bool p_open_pass = true, bool p_close_pass = true,
		bool p_clear_region = true, RenderingServerTypes::RenderInfo* p_render_info = nullptr,
		const Transform3D& p_main_cam_transform = Transform3D());
	static void _render_shadow_begin();
	static void _render_shadow_append(RID p_framebuffer,
		const PagedArray<RenderGeometryInstance*>& p_instances, const Projection& p_projection,
		const Transform3D& p_transform, float p_zfar, float p_bias, float p_normal_bias,
		bool p_use_dp, bool p_use_dp_flip, bool p_use_pancake,
		float p_lod_distance_multiplier = 0.0, float p_screen_mesh_lod_threshold = 0.0,
		const Rect2i& p_rect = Rect2i(), bool p_flip_y = false, bool p_clear_region = true,
		bool p_begin = true, bool p_end = true,
		RenderingServerTypes::RenderInfo* p_render_info = nullptr,
		const Transform3D& p_main_cam_transform = Transform3D());
	static void _render_shadow_process();
	static void _render_shadow_end();

	/* Render Scene */

	static RID _setup_render_pass_uniform_set(RenderListType p_render_list,
		const RenderDataRD* p_render_data, bool p_is_multiview, RID p_radiance_texture,
		const RendererRD::Samplers& p_samplers,
		bool p_use_directional_shadow_atlas = false, uint32_t p_pass_offset = 0u);
	static void _pre_opaque_render(RenderDataRD* p_render_data);


	static void _update_render_base_uniform_set();

	static void _fill_instance_data(RenderListType p_render_list, uint32_t p_offset = 0,
		int32_t p_max_elements = -1, bool p_update_buffer = true);
	static void _fill_render_list(RenderListType p_render_list, const RenderDataRD* p_render_data,
		PassMode p_pass_mode, bool p_append = false);

	static void _setup_environment(const RenderDataRD* p_render_data, bool p_no_fog,
		const Size2i& p_screen_size, const Size2& p_viewport_size, const Color& p_default_bg_color,
		bool p_opaque_render_buffers = false, bool p_pancake_shadows = false);
	static void _setup_lightmaps(const RenderDataRD* p_render_data, const PagedArray<RID>& p_lightmaps,
		const Transform3D& p_cam_transform);


	/* Light map */

	struct LightmapData
	{
		float normal_xform[12];
		float texture_size[2];
		float exposure_normalization;
		uint32_t flags;
	};

	struct LightmapCaptureData
	{
		float sh[9 * 4];
	};

	/* Scene state */

	struct SceneState
	{
		MultiUmaBuffer<1u> uniform_buffers = MultiUmaBuffer<1u>("SceneState::uniform_buffers");

		struct PushConstantUbershader
		{
			SceneShaderForwardMobile::ShaderSpecialization specialization;
			SceneShaderForwardMobile::UbershaderConstants constants;
		};

		struct PushConstant
		{
			uint32_t uv_offset;
			uint32_t base_index;
			uint32_t multimesh_motion_vectors_current_offset;
			uint32_t multimesh_motion_vectors_previous_offset;
			PushConstantUbershader ubershader;
		};

		struct InstanceData
		{
			float transform[12];
			float compressed_aabb_position[4];
			float compressed_aabb_size[4];
			float uv_scale[4];
			uint32_t flags;
			uint32_t instance_uniforms_ofs; // Base offset in global buffer for instance variables.
			uint32_t gi_offset; // GI information when using lightmapping (VCT or lightmap index).
			uint32_t layer_mask;
			float prev_transform[12];
			float lightmap_uv_scale[4];	   // Doubles as uv_offset when needed.
			uint32_t reflection_probes[2]; // Packed reflection probes.
			uint32_t omni_lights[2];	   // Packed omni lights.
			uint32_t spot_lights[2];	   // Packed spot lights.
			uint32_t area_lights[2];	   // Packed area lights.
			uint32_t decals[2];			   // Packed spot lights.
			uint32_t padding[2];
#ifdef REAL_T_IS_DOUBLE
			float model_precision[4];
			float prev_model_precision[4];
#endif

			// These setters allow us to copy the data over with operation when using floats.
			inline void set_lightmap_uv_scale(const Rect2& p_rect)
			{
#ifdef REAL_T_IS_DOUBLE
				lightmap_uv_scale[0] = p_rect.position.x;
				lightmap_uv_scale[1] = p_rect.position.y;
				lightmap_uv_scale[2] = p_rect.size.x;
				lightmap_uv_scale[3] = p_rect.size.y;
#else
				Rect2* rect = reinterpret_cast<Rect2*>(lightmap_uv_scale);
				*rect = p_rect;
#endif
			}

			inline void set_compressed_aabb(const AABB& p_aabb)
			{
#ifdef REAL_T_IS_DOUBLE
				compressed_aabb_position[0] = p_aabb.position.x;
				compressed_aabb_position[1] = p_aabb.position.y;
				compressed_aabb_position[2] = p_aabb.position.z;

				compressed_aabb_size[0] = p_aabb.size.x;
				compressed_aabb_size[1] = p_aabb.size.y;
				compressed_aabb_size[2] = p_aabb.size.z;
#else
				Vector3* compressed_aabb_position_vec3 =
					reinterpret_cast<Vector3*>(compressed_aabb_position);
				Vector3* compressed_aabb_size_vec3 =
					reinterpret_cast<Vector3*>(compressed_aabb_size);
				*compressed_aabb_position_vec3 = p_aabb.position;
				*compressed_aabb_size_vec3 = p_aabb.size;
#endif
			}

			inline void set_uv_scale(const Vector4& p_uv_scale)
			{
#ifdef REAL_T_IS_DOUBLE
				uv_scale[0] = p_uv_scale.x;
				uv_scale[1] = p_uv_scale.y;
				uv_scale[2] = p_uv_scale.z;
				uv_scale[3] = p_uv_scale.w;
#else
				Vector4* uv_scale_vec4 = reinterpret_cast<Vector4*>(uv_scale);
				*uv_scale_vec4 = p_uv_scale;
#endif
			}
		};

		static_assert(std::is_trivially_destructible_v<InstanceData>);
		static_assert(std::is_trivially_constructible_v<InstanceData>);

		MultiUmaBuffer<1u> instance_buffer[RENDER_LIST_MAX] = {
			MultiUmaBuffer<1u>("RENDER_LIST_OPAQUE"), MultiUmaBuffer<1u>("RENDER_LIST_ALPHA"),
			MultiUmaBuffer<1u>("RENDER_LIST_SECONDARY")};
		InstanceData* curr_gpu_ptr[RENDER_LIST_MAX] = {};

		// !BAS! We need to change lightmaps, we're not going to do this with a buffer but pushing
		// the used lightmap in
		LightmapData lightmaps[MAX_LIGHTMAPS];
		RID lightmap_ids[MAX_LIGHTMAPS];
		bool lightmap_has_sh[MAX_LIGHTMAPS];
		uint32_t lightmaps_used = 0;
		uint32_t max_lightmaps;
		RID lightmap_buffer;

		LightmapCaptureData* lightmap_captures = nullptr;
		uint32_t max_lightmap_captures;
		RID lightmap_capture_buffer;

		bool used_screen_texture = false;
		bool used_depth_texture = false;
		bool used_lightmap = false;
		bool used_opaque_stencil = false;

		struct ShadowPass
		{
			uint32_t element_from;
			uint32_t element_count;
			bool flip_cull;
			PassMode pass_mode;

			RID rp_uniform_set;
			float lod_distance_multiplier;
			float screen_mesh_lod_threshold;

			RID framebuffer;
			Rect2i rect;
			bool clear_depth;
		};

		LocalVector<ShadowPass> shadow_passes;

		void grow_instance_buffer(
			RenderListType p_render_list, uint32_t p_req_element_count, bool p_append);
	};

	/* Render List */

	// !BAS! Render list can probably be reused between clustered and mobile?
	struct RenderList
	{
		LocalVector<GeometryInstanceSurfaceDataCache*> elements;
		LocalVector<RenderElementInfo> element_info;

		void clear()
		{
			elements.clear();
			element_info.clear();
		}

		// should eventually be replaced by radix

		struct SortByKey
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				return (A->sort.sort_key2 == B->sort.sort_key2)
						   ? (A->sort.sort_key1 < B->sort.sort_key1)
						   : (A->sort.sort_key2 < B->sort.sort_key2);
			}
		};

		void sort_by_key()
		{
			SortArray<GeometryInstanceSurfaceDataCache*, SortByKey> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		void sort_by_key_range(uint32_t p_from, uint32_t p_size)
		{
			SortArray<GeometryInstanceSurfaceDataCache*, SortByKey> sorter;
			sorter.sort(elements.ptr() + p_from, p_size);
		}

		struct SortByKeyAndStencil
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				bool a_stencil = A->flags & GeometryInstanceSurfaceDataCache::FLAG_USES_STENCIL;
				bool b_stencil = B->flags & GeometryInstanceSurfaceDataCache::FLAG_USES_STENCIL;
				if (a_stencil != b_stencil) {
					return a_stencil < b_stencil;
				}
				return (A->sort.sort_key2 == B->sort.sort_key2)
						   ? (A->sort.sort_key1 < B->sort.sort_key1)
						   : (A->sort.sort_key2 < B->sort.sort_key2);
			}
		};

		void sort_by_key_and_stencil()
		{
			SortArray<GeometryInstanceSurfaceDataCache*, SortByKeyAndStencil> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		struct SortByDepth
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				return (A->owner->depth < B->owner->depth);
			}
		};

		void sort_by_depth()
		{ // used for shadows

			SortArray<GeometryInstanceSurfaceDataCache*, SortByDepth> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		struct SortByReverseDepthAndPriority
		{
			_FORCE_INLINE_ bool operator()(const GeometryInstanceSurfaceDataCache* A,
				const GeometryInstanceSurfaceDataCache* B) const
			{
				return (A->sort.priority == B->sort.priority)
						   ? (A->owner->depth > B->owner->depth)
						   : (A->sort.priority < B->sort.priority);
			}
		};

		void sort_by_reverse_depth_and_priority()
		{ // used for alpha

			SortArray<GeometryInstanceSurfaceDataCache*, SortByReverseDepthAndPriority> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		_FORCE_INLINE_ void add_element(GeometryInstanceSurfaceDataCache* p_element)
		{
			elements.push_back(p_element);
		}
	};

	struct RenderElementInfo
	{
		union
		{
			struct
			{
				uint32_t lod_index : 8;
				uint32_t uses_lightmap : 1;
				uint32_t reserved : 23;
			};

			uint32_t value;
		};
	};

	static_assert(std::is_trivially_destructible_v<RenderElementInfo>);
	static_assert(std::is_trivially_constructible_v<RenderElementInfo>);

	static void _render_list(RenderingDevice::DrawListID p_draw_list,
		RenderingDevice::FramebufferFormatID p_framebuffer_Format, RenderListParameters* p_params,
		uint32_t p_from_element, uint32_t p_to_element);
	static void _render_list_with_draw_list(RenderListParameters* p_params, RID p_framebuffer,
		uint32_t p_clear_colors = RD::DRAW_DEFAULT_ALL,
		const Vector<Color>& p_clear_color_values = Vector<Color>(),
		float p_clear_depth_value = 0.0, uint32_t p_clear_stencil_value = 0,
		const Rect2& p_region = Rect2());


protected:
	/* setup */
	static void _update_shader_quality_settings();

	static RDC::DataFormat _render_buffers_get_preferred_color_format();
	static bool _render_buffers_can_be_storage();

	static RID _render_buffers_get_normal_texture(Ref<RenderSceneBuffersRD> p_render_buffers);
	static RID _render_buffers_get_velocity_texture(Ref<RenderSceneBuffersRD> p_render_buffers);

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to)
	{
	}

	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to)
	{
	}

	static void environment_set_ssr_half_size(bool p_half_size) {}

	static void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality)
	{
	}

	static void sub_surface_scattering_set_quality(
		RSE::SubSurfaceScatteringQuality p_quality)
	{
	}

	static void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale) {}

	/* Geometry instance */

	class GeometryInstanceForwardMobile;

	// When changing any of these enums, remember to change the corresponding enums in the shader
	// files as well.
	enum
	{
		INSTANCE_DATA_FLAG_MULTIMESH_INDIRECT = 1 << 2,
		INSTANCE_DATA_FLAGS_DYNAMIC = 1 << 3,
		INSTANCE_DATA_FLAGS_NON_UNIFORM_SCALE = 1 << 4,
		INSTANCE_DATA_FLAG_USE_GI_BUFFERS = 1 << 5,
		INSTANCE_DATA_FLAG_USE_SDFGI = 1 << 6,
		INSTANCE_DATA_FLAG_USE_LIGHTMAP_CAPTURE = 1 << 7,
		INSTANCE_DATA_FLAG_USE_LIGHTMAP = 1 << 8,
		INSTANCE_DATA_FLAG_USE_SH_LIGHTMAP = 1 << 9,
		INSTANCE_DATA_FLAG_USE_VOXEL_GI = 1 << 10,
		INSTANCE_DATA_FLAG_PARTICLES = 1 << 11,
		INSTANCE_DATA_FLAG_MULTIMESH = 1 << 12,
		INSTANCE_DATA_FLAG_MULTIMESH_FORMAT_2D = 1 << 13,
		INSTANCE_DATA_FLAG_MULTIMESH_HAS_COLOR = 1 << 14,
		INSTANCE_DATA_FLAG_MULTIMESH_HAS_CUSTOM_DATA = 1 << 15,
		INSTANCE_DATA_FLAGS_PARTICLE_TRAIL_SHIFT = 16,
		INSTANCE_DATA_FLAGS_PARTICLE_TRAIL_MASK = 0xFF,
	};

	struct GeometryInstanceLightmapSH
	{
		Color sh[9];
	};

	// Cached data for drawing surfaces
	struct GeometryInstanceSurfaceDataCache
	{
		enum
		{
			FLAG_PASS_DEPTH = 1,
			FLAG_PASS_OPAQUE = 2,
			FLAG_PASS_ALPHA = 4,
			FLAG_PASS_SHADOW = 8,
			FLAG_USES_SHARED_SHADOW_MATERIAL = 128,
			FLAG_USES_SUBSURFACE_SCATTERING = 2048,
			FLAG_USES_SCREEN_TEXTURE = 4096,
			FLAG_USES_DEPTH_TEXTURE = 8192,
			FLAG_USES_NORMAL_TEXTURE = 16384,
			FLAG_USES_DOUBLE_SIDED_SHADOWS = 32768,
			FLAG_USES_PARTICLE_TRAILS = 65536,
			FLAG_USES_STENCIL = 131072,
		};

		union
		{
			struct
			{
				uint64_t sort_key1;
				uint64_t sort_key2;
			};

			struct
			{
				// Needs to be grouped together to be used in RenderElementInfo, as the value is
				// masked directly.
				uint64_t lod_index : 8;
				uint64_t uses_lightmap : 1;
				uint64_t pad : 3;

				// Sorted based on optimal order for respecting priority and reducing the amount of
				// rebinding of shaders, materials, and geometry. This current order was found to be
				// the most optimal in large projects. If you wish to measure differences, refer to
				// RenderingDeviceGraph and the methods available to print statistics for draw
				// lists.
				uint64_t depth_layer : 4;
				uint64_t surface_index : 8;
				uint64_t geometry_id : 32;
				uint64_t material_id_hi : 8;

				uint64_t material_id_lo : 24;
				uint64_t shader_id : 32;
				uint64_t priority : 8;
			};
		} sort;

		RSE::PrimitiveType primitive = RSE::PRIMITIVE_MAX;
		uint32_t flags = 0;
		uint32_t surface_index = 0;

		void* surface = nullptr;
		RID material_uniform_set;
		SceneShaderForwardMobile::ShaderData* shader = nullptr;
		SceneShaderForwardMobile::MaterialData* material = nullptr;

		void* surface_shadow = nullptr;
		RID material_uniform_set_shadow;
		SceneShaderForwardMobile::ShaderData* shader_shadow = nullptr;

		GeometryInstanceSurfaceDataCache* next = nullptr;
		GeometryInstanceForwardMobile* owner = nullptr;

		SelfList<GeometryInstanceSurfaceDataCache> compilation_dirty_element;
		SelfList<GeometryInstanceSurfaceDataCache> compilation_all_element;

		GeometryInstanceSurfaceDataCache()
			: compilation_dirty_element(this), compilation_all_element(this)
		{
		}
	};

	class GeometryInstanceForwardMobile : public RenderGeometryInstance
	{
	public:
		// used during rendering
		RID transforms_uniform_set;
		bool use_projector = false;
		bool use_soft_shadow = false;
		bool store_transform_cache =
			true; // If true we copy our transform into our per-draw buffer, if false we use our
				  // transforms UBO and clear our per-draw transform.
		uint32_t instance_count = 0;
		uint32_t trail_steps = 1;

		uint64_t prev_transform_change_frame = UINT_MAX;
		bool prev_transform_dirty = true;
		Transform3D prev_transform;

		// lightmap
		uint32_t gi_offset_cache = 0; // !BAS! Should rename this to lightmap_offset_cache, in
									  // forward clustered this was shared between gi and lightmap
		RID lightmap_instance;
		Rect2 lightmap_uv_scale;
		uint32_t lightmap_slice_index;
		GeometryInstanceLightmapSH* lightmap_sh = nullptr;

		// culled light info
		uint32_t reflection_probe_count = 0;
		RendererRD::ForwardID reflection_probes[MAX_RDL_CULL];
		uint32_t omni_light_count = 0;
		RendererRD::ForwardID omni_lights[MAX_RDL_CULL];
		uint32_t spot_light_count = 0;
		RendererRD::ForwardID spot_lights[MAX_RDL_CULL];
		uint32_t area_light_count = 0;
		RendererRD::ForwardID area_lights[MAX_RDL_CULL];
		uint32_t decals_count = 0;
		RendererRD::ForwardID decals[MAX_RDL_CULL];

		GeometryInstanceSurfaceDataCache* surface_caches = nullptr;

		// do we use this?
		SelfList<GeometryInstanceForwardMobile> dirty_list_element;

		GeometryInstanceForwardMobile() : dirty_list_element(this) {}

		virtual void _mark_dirty() override;

		virtual void set_transform(const Transform3D& p_transform, const AABB& p_aabb,
			const AABB& p_transformed_aabb) override;
		virtual void set_use_lightmap(RID p_lightmap_instance, const Rect2& p_lightmap_uv_scale,
			int p_lightmap_slice_index) override;
		virtual void set_lightmap_capture(const Color* p_sh9) override;

		virtual void clear_light_instances() override;
		virtual void pair_light_instance(
			const RID p_light_instance, RSE::LightType light_type, uint32_t placement_idx) override;
		virtual void pair_reflection_probe_instances(const RID* p_reflection_probe_instances,
			uint32_t p_reflection_probe_instance_count) override;
		virtual void pair_decal_instances(
			const RID* p_decal_instances, uint32_t p_decal_instance_count) override;

		virtual void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) override;
	};

	static uint32_t get_max_lights_total();
	static uint32_t get_max_lights_per_mesh();

	/* Rendering */

	static void _render_scene(RenderDataRD* p_render_data, const Color& p_default_bg_color);

	static void _render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region, float p_exposure_normalization);
	static void _render_uv2(const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region);
	static void _render_sdfgi(Ref<RenderSceneBuffersRD> p_render_buffers, const Vector3i& p_from,
		const Vector3i& p_size, const AABB& p_bounds,
		const PagedArray<RenderGeometryInstance*>& p_instances, const RID& p_albedo_texture,
		const RID& p_emission_texture, const RID& p_emission_aniso_texture,
		const RID& p_geom_facing_texture, float p_exposure_normalization);
	static void _render_particle_collider_heightfield(RID p_fb, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, const PagedArray<RenderGeometryInstance*>& p_instances);

	/* Forward ID */

	class ForwardIDStorageMobile final
	{
	public:
		struct ForwardIDAllocator
		{
			LocalVector<bool> allocations;
			LocalVector<uint8_t> map;
			LocalVector<uint64_t> last_pass;
		};

		struct Data
		{
			ForwardIDAllocator forward_id_allocators[RendererRD::FORWARD_ID_MAX];
		};

		static inline Data* data = nullptr;

		static void initialize();
		static void finalize();
		static bool is_initialized() { return data != nullptr; }

		static RendererRD::ForwardID allocate_forward_id(RendererRD::ForwardIDType p_type);
		static void free_forward_id(RendererRD::ForwardIDType p_type, RendererRD::ForwardID p_id);
		static void map_forward_id(RendererRD::ForwardIDType p_type, RendererRD::ForwardID p_id,
			uint32_t p_index, uint64_t p_last_pass);

		static bool uses_forward_ids() { return true; }

		ForwardIDStorageMobile() = delete;
		ForwardIDStorageMobile(const ForwardIDStorageMobile&) = delete;
		ForwardIDStorageMobile& operator=(const ForwardIDStorageMobile&) = delete;
		~ForwardIDStorageMobile() = delete;
	};

	static void fill_push_constant_instance_indices(
		SceneState::InstanceData* p_instance_data, const GeometryInstanceForwardMobile* p_instance);

	struct ForwardIDByMapSort
	{
		uint8_t map;
		RendererRD::ForwardID forward_id;

		bool operator<(const ForwardIDByMapSort& p_sort) const { return map > p_sort.map; }
	};

public:

	static RID reflection_probe_create_framebuffer(RID p_color, RID p_depth);

	/* SDFGI UPDATE */

	static void sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_environment,
		const Vector3& p_world_position)
	{
	}

	static int sdfgi_get_pending_region_count(
		const Ref<RenderSceneBuffers>& p_render_buffers)
	{
		return 0;
	}

	static AABB sdfgi_get_pending_region_bounds(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region)
	{
		return AABB();
	}

	static uint32_t sdfgi_get_pending_region_cascade(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region)
	{
		return 0;
	}

	/* GEOMETRY INSTANCE */

	static void _geometry_instance_dependency_changed(
		Dependency::DependencyChangedNotification p_notification, DependencyTracker* p_tracker);
	static void _geometry_instance_dependency_deleted(
		const RID& p_dependency, DependencyTracker* p_tracker);


	struct SurfacePipelineData
	{
		void* mesh_surface = nullptr;
		void* mesh_surface_shadow = nullptr;
		SceneShaderForwardMobile::ShaderData* shader = nullptr;
		SceneShaderForwardMobile::ShaderData* shader_shadow = nullptr;
		bool instanced = false;
		bool uses_opaque = false;
		bool uses_transparent = false;
		bool uses_depth = false;
		bool can_use_lightmap = false;
	};

	struct GlobalPipelineData
	{
		union
		{
			uint32_t key;

			struct
			{
				uint32_t texture_samples : 3;
				uint32_t target_samples : 3;
				uint32_t use_reflection_probes : 1;
				uint32_t use_lightmaps : 1;
				uint32_t use_multiview : 1;
				uint32_t use_16_bit_shadows : 1;
				uint32_t use_32_bit_shadows : 1;

	uint32_t use_shadow_cubemaps : 1;
				uint32_t use_shadow_dual_paraboloid : 1;
				uint32_t use_vrs : 1;
				uint32_t use_subpass_post_pass : 1;
				uint32_t use_separate_post_pass : 1;
				uint32_t use_hdr_render_target : 1;
				uint32_t use_ldr_render_target : 1;
			};
		};
	};


	typedef Pair<SceneShaderForwardMobile::ShaderData*,
		SceneShaderForwardMobile::ShaderData::PipelineKey>
		ShaderPipelinePair;

	static void _update_global_pipeline_data_requirements_from_project();
	static void _update_global_pipeline_data_requirements_from_light_storage();
	static void _geometry_instance_add_surface_with_material(GeometryInstanceForwardMobile* ginstance,
		uint32_t p_surface, SceneShaderForwardMobile::MaterialData* p_material,
		uint32_t p_material_id, uint32_t p_shader_id, RID p_mesh);
	static void _geometry_instance_add_surface_with_material_chain(
		GeometryInstanceForwardMobile* ginstance, uint32_t p_surface,
		SceneShaderForwardMobile::MaterialData* p_material, RID p_mat_src, RID p_mesh);
	static void _geometry_instance_add_surface(
		GeometryInstanceForwardMobile* ginstance, uint32_t p_surface, RID p_material, RID p_mesh);
	static void _geometry_instance_update(RenderGeometryInstance* p_geometry_instance);
	static void _mesh_compile_pipeline_for_surface(SceneShaderForwardMobile::ShaderData* p_shader,
		void* p_mesh_surface, bool p_instanced_surface, RSE::PipelineSource p_source,
		SceneShaderForwardMobile::ShaderData::PipelineKey& r_pipeline_key,
		Vector<ShaderPipelinePair>* r_pipeline_pairs = nullptr);
	static void _mesh_compile_pipelines_for_surface(const SurfacePipelineData& p_surface,
		const GlobalPipelineData& p_global, RSE::PipelineSource p_source,
		Vector<ShaderPipelinePair>* r_pipeline_pairs = nullptr);
	static void _mesh_generate_all_pipelines_for_surface_cache(
		GeometryInstanceSurfaceDataCache* p_surface_cache, const GlobalPipelineData& p_global);
	static void _update_dirty_geometry_instances();
	static void _update_dirty_geometry_pipelines();

	// Global data about the scene that can be used to pre-allocate resources without relying on
	// culling.
	struct GlobalSurfaceData
	{
		bool screen_texture_used = false;
		bool depth_texture_used = false;
	};

	struct Data
	{
		bool disable_ubershaders = false;
		LTC ltc;
		uint64_t lightmap_texture_array_version = 0xFFFFFFFF;
		RID render_base_uniform_set;
		SceneState scene_state;
		RenderList render_list[RENDER_LIST_MAX];

		SelfList<GeometryInstanceForwardMobile>::List geometry_instance_dirty_list;
		SelfList<GeometryInstanceSurfaceDataCache>::List geometry_surface_compilation_dirty_list;
		SelfList<GeometryInstanceSurfaceDataCache>::List geometry_surface_compilation_all_list;

		PagedAllocator<GeometryInstanceForwardMobile> geometry_instance_alloc;
		PagedAllocator<GeometryInstanceSurfaceDataCache> geometry_instance_surface_alloc;
		PagedAllocator<GeometryInstanceLightmapSH> geometry_instance_lightmap_sh;

		GlobalPipelineData global_pipeline_data_compiled = {};
		GlobalPipelineData global_pipeline_data_required = {};
		GlobalSurfaceData global_surface_data;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static RenderGeometryInstance* geometry_instance_create(RID p_base);
	static void geometry_instance_free(RenderGeometryInstance* p_geometry_instance);

	static uint32_t geometry_instance_get_pair_mask();

	/* PIPELINES */

	static void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation);
	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	/* SHADER LIBRARY */

	static void enable_features(uint32_t p_feature_bits);
	static String get_name();

	static bool free(RID p_rid);

	static void update();

	static void base_uniforms_changed();

	static bool is_dynamic_gi_supported();
	static bool is_volumetric_supported();
	static uint32_t get_max_elements();

	RenderForwardMobile() = delete;
	RenderForwardMobile(const RenderForwardMobile&) = delete;
	RenderForwardMobile& operator=(const RenderForwardMobile&) = delete;
	~RenderForwardMobile() = delete;
};
} // namespace RendererSceneRenderImplementation

// --- End servers/rendering/renderer_rd/forward_mobile/render_forward_mobile.h ---
