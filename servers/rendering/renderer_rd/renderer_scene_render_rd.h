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
#include "servers/rendering/renderer_rd/effects/post_effects.h"
#include "servers/rendering/renderer_rd/effects/upscale_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
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
