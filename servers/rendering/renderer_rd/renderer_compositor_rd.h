/**************************************************************************/
/*  renderer_compositor_rd.h                                              */
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
#include "servers/rendering/renderer_compositor.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/environment/rd_environment.h ---
#include "core/templates/rid_owner.h"
#include "servers/rendering/renderer_rd/shaders/environment/sky.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/shaders/environment/gi.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/sdfgi_debug_probes.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/sdfgi_direct_light.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/sdfgi_integrate.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/sdfgi_preprocess.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/voxel_gi.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/voxel_gi_debug.glsl.gen.h"
#include "servers/rendering/storage/utilities.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/shaders/environment/volumetric_fog.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/environment/volumetric_fog_process.glsl.gen.h"
#include "servers/rendering/renderer_rd/rd_caches.h"

// Forward declare RendererSceneRenderRD so we can pass it into some of our methods, these classes
// are pretty tightly bound
class RendererSceneRenderRD;
class RenderDataRD;
class RenderSceneBuffersRD;

namespace RendererRD
{

class SkyRD final
{
public:
	enum SkySet
	{
		SKY_SET_UNIFORMS,
		SKY_SET_MATERIAL,
		SKY_SET_TEXTURES,
		SKY_SET_FOG,
	};

	const int SAMPLERS_BINDING_FIRST_INDEX = 4;

	// Skys need less info from Directional Lights than the normal shaders
	struct SkyDirectionalLightData
	{
		float direction[3];
		float energy;
		float color[3];
		float size;
		uint32_t enabled;
		uint32_t pad[3];
	};

private:
	RDC::DataFormat texture_format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;

	enum SkyTextureSetVersion
	{
		SKY_TEXTURE_SET_BACKGROUND,
		SKY_TEXTURE_SET_HALF_RES,
		SKY_TEXTURE_SET_QUARTER_RES,
		SKY_TEXTURE_SET_OCTMAP,
		SKY_TEXTURE_SET_OCTMAP_HALF_RES,
		SKY_TEXTURE_SET_OCTMAP_QUARTER_RES,
		SKY_TEXTURE_SET_MAX
	};

	enum SkyVersion
	{
		SKY_VERSION_BACKGROUND,
		SKY_VERSION_HALF_RES,
		SKY_VERSION_QUARTER_RES,
		SKY_VERSION_OCTMAP,
		SKY_VERSION_OCTMAP_HALF_RES,
		SKY_VERSION_OCTMAP_QUARTER_RES,

		SKY_VERSION_BACKGROUND_MULTIVIEW,
		SKY_VERSION_HALF_RES_MULTIVIEW,
		SKY_VERSION_QUARTER_RES_MULTIVIEW,

		SKY_VERSION_MAX
	};

	struct SkyPushConstant
	{
		float orientation[12];		 // 48 - 48
		float projection[4];		 // 16 - 64
		float position[3];			 // 12 - 76
		float time;					 // 4 - 80
		float border_size[2];		 // 8 - 88
		float luminance_multiplier;	 // 4 - 92
		float brightness_multiplier; // 4 - 96
		// 128 is the max size of a push constant. We can replace "pad" but we can't add any more.
	};

	struct SkyShaderData : public RendererRD::MaterialStorage::ShaderData
	{
		bool valid = false;
		RID version;

		PipelineCacheRD pipelines[SKY_VERSION_MAX];
		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size = 0;

		String code;

		bool uses_time = false;
		bool uses_position = false;
		bool uses_half_res = false;
		bool uses_quarter_res = false;
		bool uses_light = false;

		virtual bool is_animated() const;
		virtual void set_code(const String& p_Code);
		virtual bool casts_shadows() const;
		virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const;

		SkyShaderData() {}

		virtual ~SkyShaderData();
	};

	void _render_sky(RD::DrawListID p_list, float p_time, RID p_fb, PipelineCacheRD* p_pipeline,
		RID p_uniform_set, RID p_texture_set, const Projection& p_projection,
		const Basis& p_orientation, const Vector3& p_position, float p_luminance_multiplier,
		float p_brightness_modifier, float p_border_size = 0.0);

public:
	struct SkySceneState
	{
		struct UBO
		{
			float combined_reprojection[RendererSceneRender::MAX_RENDER_VIEWS][16]; // 2 x 64 - 128
			float view_inv_projections[RendererSceneRender::MAX_RENDER_VIEWS][16];	// 2 x 64 - 256
			float view_eye_offsets[RendererSceneRender::MAX_RENDER_VIEWS][4];		// 2 x 16 - 288

			uint32_t volumetric_fog_enabled;	// 4 - 292
			float volumetric_fog_inv_length;	// 4 - 296
			float volumetric_fog_detail_spread; // 4 - 300
			float volumetric_fog_sky_affect;	// 4 - 304

			uint32_t fog_enabled;  // 4 - 308
			float fog_sky_affect;  // 4 - 312
			float fog_density;	   // 4 - 316
			float fog_sun_scatter; // 4 - 320

			float fog_light_color[3];	  // 12 - 332
			float fog_aerial_perspective; // 4 - 336

			float z_far;					  // 4 - 340
			uint32_t directional_light_count; // 4 - 344
			uint32_t fog_use_legacy_blending; // 4 - 348
			uint32_t pad1;					  // 4 - 352
		};

		UBO ubo;

		uint32_t view_count = 1;
		Transform3D cam_transform;
		Projection cam_projection;

		SkyDirectionalLightData* directional_lights = nullptr;
		SkyDirectionalLightData* last_frame_directional_lights = nullptr;
		uint32_t max_directional_lights;
		uint32_t last_frame_directional_light_count;
		RID directional_light_buffer;
		RID uniform_set;
		RID uniform_buffer;
		RID fog_uniform_set;
		RID default_fog_uniform_set;

		RID fog_shader;
		RID fog_material;

		// These are lazily initialized, use "get_fog_only_texture_uniform_set" instead.
		RID fog_only_texture_uniform_set;
		RID fog_only_texture_multiview_uniform_set;

		RID get_fog_only_texture_uniform_set(RID p_default_shader_rd, bool p_is_multiview);
	} sky_scene_state;

	struct ReflectionData
	{
		struct Layer
		{
			struct Mipmap
			{
				RID framebuffer;
				RID view;
				Size2i size;
			};

			LocalVector<Mipmap> mipmaps;
		};

		struct DownsampleLayer
		{
			struct Mipmap
			{
				RID view;
				Size2i size;

				// Used only for the raster version.
				RID octmap_view;
				RID framebuffer;
			};

			LocalVector<Mipmap> mipmaps;
		};

		RID radiance_base_octmap;
		RID downsampled_radiance_octmap;
		DownsampleLayer downsampled_layer;
		RID coefficient_buffer;

		bool dirty = true;
		float uv_border_size = 0.0; // Border size in UV space.

		LocalVector<Layer> layers;

		void clear_reflection_data();
		void update_reflection_data(int p_size, int p_mipmaps, bool p_use_array, RID p_base_oct,
			int p_base_layer, bool p_low_quality, int p_roughness_layers,
			RDC::DataFormat p_texture_format, float p_border_size);
		void create_reflection_fast_filter(bool p_use_arrays);
		void create_reflection_importance_sample(
			bool p_use_arrays, int p_base_layer, uint32_t p_sky_ggx_samples_quality);
		void update_reflection_mipmaps(int p_start, int p_end);
	};

	/* Sky shader */

	struct SkyShader
	{
		ShaderRD shader{SkyShaderRD()};
		ShaderCompiler compiler;

		RID default_shader;
		RID default_material;
		RID default_shader_rd;
		RID default_multiview_shader_rd; // This is lazily initialized, use "get_default_shader_rd"
										 // instead.

		RID get_default_shader_rd(bool p_is_multiview = false);
	};

	SkyShader sky_shader;

	struct SkyMaterialData : public RendererRD::MaterialStorage::MaterialData
	{
		SkyShaderData* shader_data = nullptr;
		RID uniform_set;
		bool uniform_set_updated;

		virtual void set_render_priority(int p_priority) {}

		virtual void set_next_pass(RID p_pass) {}

		virtual ~SkyMaterialData();
	};

private:
	SkyMaterialData* _get_sky_material_data(RID p_env);

public:
	struct Sky
	{
		static inline const int REAL_TIME_SIZE = 256;
		static inline const int REAL_TIME_ROUGHNESS_LAYERS = 7;

		RID radiance;
		RID radiance_first_layer_slice;
		RID quarter_res_pass;
		RID quarter_res_framebuffer;
		Size2i screen_size;

		RID uniform_set;

		RID material;
		RID uniform_buffer;

		int radiance_size = REAL_TIME_SIZE;
		float uv_border_size = 0.0; // Border size in UV space.

		RSE::SkyMode mode = RSE::SKY_MODE_AUTOMATIC;
		RSE::SkyMode internal_mode = RSE::SKY_MODE_INCREMENTAL; // When using SKY_MODE_AUTOMATIC,
																// this is the mode used internally.

		ReflectionData reflection;
		bool dirty = false;
		int processing_layer = 0;
		Sky* dirty_list = nullptr;
		float baked_exposure = 1.0;

		// State to track when radiance octmap needs updating.
		SkyMaterialData* prev_material_data = nullptr;
		Vector3 prev_position;
		float prev_time;
		float prev_fog_aerial_perspective = 0.0;
		Color prev_fog_light_color;
		float prev_fog_sun_scatter = 0.0;
		bool prev_fog_enabled = false;
		float prev_fog_density = 0.0;
		float prev_fog_sky_affect = 0.0;
		float prev_fog_light_energy = 0.0;

		void free_radiance();

		void free();

		RID get_textures(SkyTextureSetVersion p_version, RID p_default_shader_rd,
			bool p_is_multiview, Ref<RenderSceneBuffersRD> p_render_buffers);
		bool set_radiance_size(int p_radiance_size);
		int get_radiance_size() const;
		bool set_mode(RSE::SkyMode p_mode);
		bool set_material(RID p_material);
		Ref<Image> bake_panorama(float p_energy, int p_roughness_layers, const Size2i& p_size);
	};

	uint32_t sky_ggx_samples_quality;
	bool sky_use_octmap_array;

	Sky* dirty_sky_list = nullptr;
	mutable RID_Owner<Sky, true> sky_owner;
	int roughness_layers;

	RendererRD::MaterialStorage::ShaderData* _create_sky_shader_func();
	static RendererRD::MaterialStorage::ShaderData* _create_sky_shader_funcs();

	RendererRD::MaterialStorage::MaterialData* _create_sky_material_func(SkyShaderData* p_shader);
	static RendererRD::MaterialStorage::MaterialData* _create_sky_material_funcs(
		RendererRD::MaterialStorage::ShaderData* p_shader);

	SkyRD();
	void init();
	void set_texture_format(RDC::DataFormat p_texture_format);
	~SkyRD();

	void setup_sky(const RenderDataRD* p_render_data, const Size2i p_screen_size);
	void update_radiance_buffers(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_env,
		const Vector3& p_global_pos, double p_time, float p_luminance_multiplier = 1.0,
		float p_brightness_multiplier = 1.0);
	void update_res_buffers(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_env, double p_time,
		float p_luminance_multiplier = 1.0, float p_brightness_multiplier = 1.0);
	void draw_sky(RD::DrawListID p_draw_list, Ref<RenderSceneBuffersRD> p_render_buffers, RID p_env,
		RID p_fb, double p_time, float p_luminance_multiplier = 1.0,
		float p_brightness_multiplier = 1.0);

	void invalidate_sky(Sky* p_sky);
	void update_dirty_skys();

	RID sky_get_material(RID p_sky) const;
	RID sky_get_radiance_texture_rd(RID p_sky) const;
	float sky_get_baked_exposure(RID p_sky) const;

	RID allocate_sky_rid();
	void initialize_sky_rid(RID p_rid);
	Sky* get_sky(RID p_sky) const;
	void free_sky(RID p_sky);
	void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	int sky_get_radiance_size(RID p_sky) const;
	void sky_set_mode(RID p_sky, RSE::SkyMode p_mode);
	void sky_set_material(RID p_sky, RID p_material);
	Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);
	float sky_get_uv_border_size(RID p_sky);
};

} // namespace RendererRD

#define RB_SCOPE_GI SNAME("rbgi")
#define RB_SCOPE_SDFGI SNAME("sdfgi")

#define RB_TEX_AMBIENT SNAME("ambient")
#define RB_TEX_REFLECTION SNAME("reflection")

class RenderDataRD;
class RenderSceneBuffersRD;
class RendererSceneRenderRD;

namespace RendererRD
{

class GI final
{
public:
	GI() = delete;
	~GI() = delete;
	GI(const GI&) = delete;
	GI& operator=(const GI&) = delete;

	/* VOXEL GI STORAGE */

	struct VoxelGI
	{
		RID octree_buffer;
		RID data_buffer;
		RID sdf_texture;

		uint32_t octree_buffer_size = 0;
		uint32_t data_buffer_size = 0;

		Vector<int> level_counts;

		int cell_count = 0;

		Transform3D to_cell_xform;
		AABB bounds;
		Vector3i octree_size;

		float dynamic_range = 2.0;
		float energy = 1.0;
		float baked_exposure = 1.0;
		float bias = 1.4;
		float normal_bias = 0.0;
		float propagation = 0.5;
		bool interior = false;
		bool use_two_bounces = true;

		uint32_t version = 1;
		uint32_t data_version = 1;

		Dependency dependency;
	};

	/* VOXEL_GI INSTANCE */

	struct VoxelGIInstance
	{
		RID probe;
		RID texture;
		RID write_buffer;

		struct Mipmap
		{
			RID texture;
			RID uniform_set;
			RID second_bounce_uniform_set;
			RID write_uniform_set;
			uint32_t level;
			uint32_t cell_offset;
			uint32_t cell_count;
		};

		Vector<Mipmap> mipmaps;

		struct DynamicMap
		{
			RID texture;
			RID fb_depth;
			RID depth;
			RID normal;
			RID albedo;
			RID orm;
			RID fb;
			RID uniform_set;
			uint32_t size;
			int mipmap;
		};

		Vector<DynamicMap> dynamic_maps;

		int slot = -1;
		uint32_t last_probe_version = 0;
		uint32_t last_probe_data_version = 0;

		uint32_t render_index = 0;

		bool has_dynamic_object_data = false;

		Transform3D transform;

		void update(bool p_update_light_instances, const Vector<RID>& p_light_instances,
			const PagedArray<RenderGeometryInstance*>& p_dynamic_objects);
		void debug(RD::DrawListID p_draw_list, RID p_framebuffer,
			const Projection& p_camera_with_transform, bool p_lighting, bool p_emission,
			float p_alpha);
		void free_resources();
	};

private:
	static inline bool _initialized = false;

	/* VOXEL GI STORAGE */

	static inline RID_Owner<VoxelGI, true> voxel_gi_owner;

	/* VOXEL_GI INSTANCE */

	static inline RID_Owner<VoxelGIInstance> voxel_gi_instance_owner;

	struct VoxelGILight
	{
		uint32_t type;
		float energy;
		float radius;
		float attenuation;

		float color[3];
		float cos_spot_angle;

		float position[3];
		float inv_spot_attenuation;

		float direction[3];
		uint32_t has_shadow;

		float area_width[4];
		float area_height[4];
		float area_projector_rect[4];
	};

	struct VoxelGIPushConstant
	{
		int32_t limits[3];
		uint32_t stack_size;

		float emission_scale;
		float propagation;
		float dynamic_range;
		uint32_t light_count;

		uint32_t cell_offset;
		uint32_t cell_count;
		float aniso_strength;
		float cell_size;
	};

	struct VoxelGIDynamicPushConstant
	{
		int32_t limits[3];
		uint32_t light_count;
		int32_t x_dir[3];
		float z_base;
		int32_t y_dir[3];
		float z_sign;
		int32_t z_dir[3];
		float pos_multiplier;
		uint32_t rect_pos[2];
		uint32_t rect_size[2];
		uint32_t prev_rect_ofs[2];
		uint32_t prev_rect_size[2];
		uint32_t flip_x;
		uint32_t flip_y;
		float dynamic_range;
		uint32_t on_mipmap;
		float propagation;
		float cell_size;
		float pad[2];
	};

	static inline VoxelGILight* voxel_gi_lights = nullptr;
	static inline uint32_t voxel_gi_max_lights = 32;
	static inline RID voxel_gi_lights_uniform;

	enum
	{
		VOXEL_GI_SHADER_VERSION_COMPUTE_LIGHT,
		VOXEL_GI_SHADER_VERSION_COMPUTE_SECOND_BOUNCE,
		VOXEL_GI_SHADER_VERSION_COMPUTE_MIPMAP,
		VOXEL_GI_SHADER_VERSION_WRITE_TEXTURE,
		VOXEL_GI_SHADER_VERSION_DYNAMIC_OBJECT_LIGHTING,
		VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_WRITE,
		VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_PLOT,
		VOXEL_GI_SHADER_VERSION_DYNAMIC_SHRINK_WRITE_PLOT,
		VOXEL_GI_SHADER_VERSION_MAX
	};

	static inline ShaderRD voxel_gi_shader{VoxelGiShaderRD()};
	static inline RID voxel_gi_lighting_shader_version;
	static inline RID voxel_gi_lighting_shader_version_shaders[VOXEL_GI_SHADER_VERSION_MAX];
	static inline PipelineDeferredRD
		voxel_gi_lighting_shader_version_pipelines[VOXEL_GI_SHADER_VERSION_MAX];

	enum
	{
		VOXEL_GI_DEBUG_COLOR,
		VOXEL_GI_DEBUG_LIGHT,
		VOXEL_GI_DEBUG_EMISSION,
		VOXEL_GI_DEBUG_LIGHT_FULL,
		VOXEL_GI_DEBUG_MAX
	};

	struct VoxelGIDebugPushConstant
	{
		float projection[16];
		uint32_t cell_offset;
		float dynamic_range;
		float alpha;
		uint32_t level;
		int32_t bounds[3];
		uint32_t pad;
	};

	static inline ShaderRD voxel_gi_debug_shader{VoxelGiDebugShaderRD()};
	static inline RID voxel_gi_debug_shader_version;
	static inline RID voxel_gi_debug_shader_version_shaders[VOXEL_GI_DEBUG_MAX];
	static inline PipelineCacheRD voxel_gi_debug_shader_version_pipelines[VOXEL_GI_DEBUG_MAX];
	static inline RID voxel_gi_debug_uniform_set;

	/* SDFGI */

	struct SDFGIShader
	{
		enum SDFGIPreprocessShaderVersion
		{
			PRE_PROCESS_SCROLL,
			PRE_PROCESS_SCROLL_OCCLUSION,
			PRE_PROCESS_JUMP_FLOOD_INITIALIZE,
			PRE_PROCESS_JUMP_FLOOD_INITIALIZE_HALF,
			PRE_PROCESS_JUMP_FLOOD,
			PRE_PROCESS_JUMP_FLOOD_OPTIMIZED,
			PRE_PROCESS_JUMP_FLOOD_UPSCALE,
			PRE_PROCESS_OCCLUSION,
			PRE_PROCESS_STORE,
			PRE_PROCESS_MAX
		};

		struct PreprocessPushConstant
		{
			int32_t scroll[3];
			int32_t grid_size;

			int32_t probe_offset[3];
			int32_t step_size;

			int32_t half_size;
			uint32_t occlusion_index;
			int32_t cascade;
			uint32_t pad;
		};

		ShaderRD preprocess;
		RID preprocess_shader;
		PipelineDeferredRD preprocess_pipeline[PRE_PROCESS_MAX];

		struct DebugPushConstant
		{
			float grid_size[3];
			uint32_t max_cascades;

			int32_t screen_size[2];
			float y_mult;

			float z_near;

			float inv_projection[3][4];
			float cam_basis[3][3];
			float cam_origin[3];
		};

		ShaderRD debug;
		RID debug_shader;
		RID debug_shader_version;
		PipelineDeferredRD debug_pipeline;

		enum ProbeDebugMode
		{
			PROBE_DEBUG_PROBES,
			PROBE_DEBUG_PROBES_MULTIVIEW,
			PROBE_DEBUG_VISIBILITY,
			PROBE_DEBUG_VISIBILITY_MULTIVIEW,
			PROBE_DEBUG_MAX
		};

		struct DebugProbesSceneData
		{
			float projection[2][16];
		};

		struct DebugProbesPushConstant
		{
			uint32_t band_power;
			uint32_t sections_in_band;
			uint32_t band_mask;
			float section_arc;

			float grid_size[3];
			uint32_t cascade;

			uint32_t pad;
			float y_mult;
			int32_t probe_debug_index;
			int32_t probe_axis_size;
		};

		ShaderRD debug_probes;
		RID debug_probes_shader;
		RID debug_probes_shader_version;

		PipelineCacheRD debug_probes_pipeline[PROBE_DEBUG_MAX];

		struct Light
		{
			float color[3];
			float energy;

			float direction[3];
			uint32_t has_shadow;

			float position[3];
			float attenuation;

			uint32_t type;
			float cos_spot_angle;
			float inv_spot_attenuation;
			float radius;

			float area_width[4];
			float area_height[4];
			float area_projector_rect[4];
		};

		struct DirectLightPushConstant
		{
			float grid_size[3];
			uint32_t max_cascades;

			uint32_t cascade;
			uint32_t light_count;
			uint32_t process_offset;
			uint32_t process_increment;

			int32_t probe_axis_size;
			float bounce_feedback;
			float y_mult;
			uint32_t use_occlusion;
		};

		enum
		{
			DIRECT_LIGHT_MODE_STATIC,
			DIRECT_LIGHT_MODE_DYNAMIC,
			DIRECT_LIGHT_MODE_MAX
		};

		ShaderRD direct_light;
		RID direct_light_shader;
		PipelineDeferredRD direct_light_pipeline[DIRECT_LIGHT_MODE_MAX];

		enum
		{
			INTEGRATE_MODE_PROCESS,
			INTEGRATE_MODE_STORE,
			INTEGRATE_MODE_SCROLL,
			INTEGRATE_MODE_SCROLL_STORE,
			INTEGRATE_MODE_MAX
		};

		struct IntegratePushConstant
		{
			enum
			{
				SKY_FLAGS_MODE_COLOR = 0x01,
				SKY_FLAGS_MODE_SKY = 0x02,
				SKY_FLAGS_ORIENTATION_SIGN = 0x04,
			};

			float grid_size[3];
			uint32_t max_cascades;

			uint32_t probe_axis_size;
			uint32_t cascade;
			uint32_t history_index;
			uint32_t history_size;

			uint32_t ray_count;
			float ray_bias;
			int32_t image_size[2];

			int32_t world_offset[3];
			uint32_t sky_flags;

			int32_t scroll[3];
			float sky_energy;

			float sky_color_or_orientation[3];
			float y_mult;

			float sky_irradiance_border_size[2];
			uint32_t store_ambient_texture;
			uint32_t pad;
		};

		ShaderRD integrate;
		RID integrate_shader;
		PipelineDeferredRD integrate_pipeline[INTEGRATE_MODE_MAX];

		RID integrate_default_sky_uniform_set;

		SDFGIShader()
			: preprocess(SdfgiPreprocessShaderRD()),
			  debug(SdfgiDebugShaderRD()),
			  debug_probes(SdfgiDebugProbesShaderRD()),
			  direct_light(SdfgiDirectLightShaderRD()),
			  integrate(SdfgiIntegrateShaderRD())
		{
		}
	};

	static inline SDFGIShader sdfgi_shader;

public:
	static bool is_initialized() { return _initialized; }

	static bool free(RID p_rid);

	/* GI */

	enum
	{
		MAX_VOXEL_GI_INSTANCES = 8
	};

	class RenderBuffersGI final : public RefCounted
	{
	private:
		RID voxel_gi_buffer;

	public:
		RID voxel_gi_textures[MAX_VOXEL_GI_INSTANCES];

		RID full_buffer;
		RID full_dispatch;
		RID full_mask;

		bool using_half_size_gi = false;

		RID uniform_set[RendererSceneRender::MAX_RENDER_VIEWS];
		RID scene_data_ubo;

		RID get_voxel_gi_buffer();

		void configure(RenderSceneBuffersRD* p_render_buffers) {}

		void free_data();
	};

	/* VOXEL GI API */

	static bool owns_voxel_gi(RID p_rid) { return voxel_gi_owner.owns(p_rid); }

	static RID voxel_gi_allocate();
	static void voxel_gi_free(RID p_voxel_gi);
	static void voxel_gi_initialize(RID p_voxel_gi);

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
	static float voxel_gi_get_dynamic_range(RID p_voxel_gi);

	static void voxel_gi_set_propagation(RID p_voxel_gi, float p_range);
	static float voxel_gi_get_propagation(RID p_voxel_gi);

	static void voxel_gi_set_energy(RID p_voxel_gi, float p_energy);
	static float voxel_gi_get_energy(RID p_voxel_gi);

	static void voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure);
	static float voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi);

	static void voxel_gi_set_bias(RID p_voxel_gi, float p_bias);
	static float voxel_gi_get_bias(RID p_voxel_gi);

	static void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range);
	static float voxel_gi_get_normal_bias(RID p_voxel_gi);

	static void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable);
	static bool voxel_gi_is_interior(RID p_voxel_gi);

	static void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable);
	static bool voxel_gi_is_using_two_bounces(RID p_voxel_gi);

	static uint32_t voxel_gi_get_version(RID p_probe);
	static uint32_t voxel_gi_get_data_version(RID p_probe);

	static RID voxel_gi_get_octree_buffer(RID p_voxel_gi);
	static RID voxel_gi_get_data_buffer(RID p_voxel_gi);

	static RID voxel_gi_get_sdf_texture(RID p_voxel_gi);

	static Dependency* voxel_gi_get_dependency(RID p_voxel_gi);

	/* VOXEL_GI INSTANCE */

	static _FORCE_INLINE_ RID voxel_gi_instance_get_texture(RID p_probe)
	{
		VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
		ERR_FAIL_NULL_V(voxel_gi, RID());
		return voxel_gi->texture;
	}

	static _FORCE_INLINE_ void voxel_gi_instance_set_render_index(RID p_probe, uint32_t p_index)
	{
		VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
		ERR_FAIL_NULL(voxel_gi);

		voxel_gi->render_index = p_index;
	}

	static bool voxel_gi_instance_owns(RID p_rid) { return voxel_gi_instance_owner.owns(p_rid); }

	static void voxel_gi_instance_free(RID p_rid);

	static inline RSE::VoxelGIQuality voxel_gi_quality = RSE::VOXEL_GI_QUALITY_LOW;

	/* SDFGI */

	class SDFGI final : public RefCounted
	{
	public:
		enum
		{
			MAX_CASCADES = 8,
			CASCADE_SIZE = 128,
			PROBE_DIVISOR = 16,
			ANISOTROPY_SIZE = 6,
			MAX_DYNAMIC_LIGHTS = 128,
			MAX_STATIC_LIGHTS = 1024,
			LIGHTPROBE_OCT_SIZE = 6,
			SH_SIZE = 16
		};

		struct Cascade
		{
			struct UBO
			{
				float offset[3];
				float to_cell;
				int32_t probe_offset[3];
				uint32_t pad;
				float pad2[4];
			};

			RID sdf_tex;
			RID light_tex;
			RID light_aniso_0_tex;
			RID light_aniso_1_tex;

			RID light_data;
			RID light_aniso_0_data;
			RID light_aniso_1_data;

			struct SolidCell
			{
				uint32_t position;
				uint32_t albedo;
				uint32_t static_light;
				uint32_t static_light_aniso;
			};

			RID solid_cell_dispatch_buffer_storage;
			RID solid_cell_dispatch_buffer_call;
			RID solid_cell_buffer;

			RID lightprobe_history_tex;
			RID lightprobe_average_tex;

			float cell_size;
			Vector3i position;

			static const Vector3i DIRTY_ALL;
			Vector3i dirty_regions;

			RID sdf_store_uniform_set;
			RID sdf_direct_light_static_uniform_set;
			RID sdf_direct_light_dynamic_uniform_set;
			RID scroll_uniform_set;
			RID scroll_occlusion_uniform_set;
			RID integrate_uniform_set;
			RID lights_buffer;

			float baked_exposure_normalization = 1.0;

			bool all_dynamic_lights_dirty = true;
		};

		RID render_albedo;
		RID render_emission;
		RID render_emission_aniso;
		RID render_occlusion[8];
		RID render_geom_facing;

		RID render_sdf[2];
		RID render_sdf_half[2];

		RID sdf_initialize_uniform_set;
		RID sdf_initialize_half_uniform_set;
		RID jump_flood_uniform_set[2];
		RID jump_flood_half_uniform_set[2];
		RID sdf_upscale_uniform_set;
		int upscale_jfa_uniform_set_index;
		RID occlusion_uniform_set;

		uint32_t cascade_size = 128;

		LocalVector<Cascade> cascades;

		RID lightprobe_texture;
		RID lightprobe_data;
		RID occlusion_texture;
		RID occlusion_data;
		RID ambient_texture;

		RID lightprobe_history_scroll;
		RID lightprobe_average_scroll;

		uint32_t history_size = 0;
		float solid_cell_ratio = 0;
		uint32_t solid_cell_count = 0;

		int num_cascades = 6;
		float min_cell_size = 0;
		uint32_t probe_axis_count = 0;

		RID debug_uniform_set[RendererSceneRender::MAX_RENDER_VIEWS];
		RID debug_probes_scene_data_ubo;
		RID debug_probes_uniform_set;
		RID cascades_ubo;

		bool uses_occlusion = false;
		float bounce_feedback = 0.5;
		bool reads_sky = true;
		float energy = 1.0;
		float normal_bias = 1.1;
		float probe_bias = 1.1;
		RSE::EnvironmentSDFGIYScale y_scale_mode = RSE::ENV_SDFGI_Y_SCALE_75_PERCENT;

		float y_mult = 1.0;

		uint32_t version = 0;
		uint32_t render_pass = 0;

		int32_t cascade_dynamic_light_count[SDFGI::MAX_CASCADES];
		RID integrate_sky_uniform_set;

		void configure(RenderSceneBuffersRD* p_render_buffers) {}

		void free_data();
		~SDFGI();

		void create(RID p_env, const Vector3& p_world_position, uint32_t p_requested_history_size);
		void update(RID p_env, const Vector3& p_world_position);
		void update_light();
		void update_probes(RID p_env, RendererRD::SkyRD::Sky* p_sky);
		void store_probes();
		int get_pending_region_data(
			int p_region, Vector3i& r_local_offset, Vector3i& r_local_size, AABB& r_bounds) const;
		void update_cascades();

		void debug_draw(uint32_t p_view_count, const Projection* p_projections,
			const Transform3D& p_transform, int p_width, int p_height, RID p_render_target,
			RID p_texture, const Vector<RID>& p_texture_views);
		void debug_probes(RID p_framebuffer, const uint32_t p_view_count,
			const Projection* p_camera_with_transforms);

		void pre_process_gi(const Transform3D& p_transform, RenderDataRD* p_render_data);
		void render_region(Ref<RenderSceneBuffersRD> p_render_buffers, int p_region,
			const PagedArray<RenderGeometryInstance*>& p_instances, float p_exposure_normalization);
		void render_static_lights(RenderDataRD* p_render_data,
			Ref<RenderSceneBuffersRD> p_render_buffers, uint32_t p_cascade_count,
			const uint32_t* p_cascade_indices,
			const PagedArray<RID>* p_positional_light_cull_result);
	};

	static inline RSE::EnvironmentSDFGIRayCount sdfgi_ray_count = RSE::ENV_SDFGI_RAY_COUNT_16;
	static inline RSE::EnvironmentSDFGIFramesToConverge sdfgi_frames_to_converge =
		RSE::ENV_SDFGI_CONVERGE_IN_30_FRAMES;
	static inline RSE::EnvironmentSDFGIFramesToUpdateLight sdfgi_frames_to_update_light =
		RSE::ENV_SDFGI_UPDATE_LIGHT_IN_4_FRAMES;

	static inline float sdfgi_solid_cell_ratio = 0.25;
	static inline Vector3 sdfgi_debug_probe_pos;
	static inline Vector3 sdfgi_debug_probe_dir;
	static inline bool sdfgi_debug_probe_enabled = false;
	static inline Vector3i sdfgi_debug_probe_index;
	static inline uint32_t sdfgi_current_version = 0;

	/* SDFGI UPDATE */

	static int sdfgi_get_lightprobe_octahedron_size() { return SDFGI::LIGHTPROBE_OCT_SIZE; }


static void sdfgi_reset();

	struct SDFGIData
	{
		float grid_size[3];
		uint32_t max_cascades;

		uint32_t use_occlusion;
		int32_t probe_axis_size;
		float probe_to_uvw;
		float normal_bias;

		float lightprobe_tex_pixel_size[3];
		float energy;

		float lightprobe_uv_offset[3];
		float y_mult;

		float occlusion_clamp[3];
		uint32_t pad3;

		float occlusion_renormalize[3];
		uint32_t pad4;

		float cascade_probe_size[3];
		uint32_t pad5;

		struct ProbeCascadeData
		{
			float position[3];
			float to_probe;
			int32_t probe_world_offset[3];
			float to_cell;
			float pad[3];
			float exposure_normalization;
		};

		ProbeCascadeData cascades[SDFGI::MAX_CASCADES];
	};

	struct VoxelGIData
	{
		float xform[16];
		float bounds[3];
		float dynamic_range;

		float bias;
		float normal_bias;
		uint32_t blend_ambient;
		uint32_t mipmaps;

		float pad[3];
		float exposure_normalization;
	};

	struct SceneData
	{
		float inv_projection[2][16];
		float cam_transform[16];
		float eye_offset[2][4];

		int32_t screen_size[2];
		float pad1;
		float pad2;
	};

	struct PushConstant
	{
		uint32_t max_voxel_gi_instances;
		uint32_t high_quality_vct;
		uint32_t orthogonal;
		uint32_t view_index;

		float proj_info[4];

		float z_near;
		float z_far;
		float pad2;
		float pad3;
	};

	static inline RID sdfgi_ubo;

	enum Group
	{
		GROUP_NORMAL,
		GROUP_VRS,
	};

	enum Mode
	{
		MODE_VOXEL_GI,
		MODE_VOXEL_GI_WITHOUT_SAMPLER,
		MODE_SDFGI,
		MODE_COMBINED,
		MODE_COMBINED_WITHOUT_SAMPLER,
		MODE_MAX
	};

	enum ShaderSpecializations
	{
		SHADER_SPECIALIZATION_HALF_RES = 1 << 0,
		SHADER_SPECIALIZATION_USE_FULL_PROJECTION_MATRIX = 1 << 1,
		SHADER_SPECIALIZATION_USE_VRS = 1 << 2,
		SHADER_SPECIALIZATION_VARIATIONS = 8,
	};

	static inline RID default_voxel_gi_buffer;

	static inline bool half_resolution = false;
	static inline ShaderRD shader{GiShaderRD()};
	static inline RID shader_version;
	static inline PipelineDeferredRD pipelines[SHADER_SPECIALIZATION_VARIATIONS][MODE_MAX];

	static void initialize(RendererRD::SkyRD* p_sky = nullptr);
	// static void initialize();
	static void finalize();

	static void init(RendererRD::SkyRD* p_sky);
	static void free();

	static Ref<SDFGI> create_sdfgi(
		RID p_env, const Vector3& p_world_position, uint32_t p_requested_history_size);

	static void setup_voxel_gi_instances(RenderDataRD* p_render_data,
		Ref<RenderSceneBuffersRD> p_render_buffers, const Transform3D& p_transform,
		const PagedArray<RID>& p_voxel_gi_instances, uint32_t& r_voxel_gi_instances_used);
	static void process_gi(Ref<RenderSceneBuffersRD> p_render_buffers,
		const RID* p_normal_roughness_slices, RID p_voxel_gi_buffer, RID p_environment,
		uint32_t p_view_count, const Projection* p_projections, const Vector3* p_eye_offsets,
		const Transform3D& p_cam_transform, const PagedArray<RID>& p_voxel_gi_instances);

	static RID voxel_gi_instance_create(RID p_base);
	static void voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform);
	static bool voxel_gi_needs_update(RID p_probe);
	static void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects);
	static void debug_voxel_gi(RID p_voxel_gi, RD::DrawListID p_draw_list, RID p_framebuffer,
		const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha);

	static void enable_vrs_shader_group();
};

} // namespace RendererRD

#define RB_SCOPE_FOG SNAME("Fog")

class ClusterBuilderRD;

namespace RendererRD
{

class Fog final
{
public:
	Fog() = delete;
	~Fog() = delete;
	Fog(const Fog&) = delete;
	Fog& operator=(const Fog&) = delete;

	/* FOG VOLUMES */

	struct FogVolume
	{
		RID material;
		Vector3 size = Vector3(2, 2, 2);

		RSE::FogVolumeShape shape = RSE::FOG_VOLUME_SHAPE_BOX;

		Dependency dependency;
	};

	struct FogVolumeInstance
	{
		RID volume;
		Transform3D transform;
		bool active = false;
	};

	/* Volumetric Fog */
	struct VolumetricFogShader
	{
		enum ShaderGroup
		{
			SHADER_GROUP_BASE,
			SHADER_GROUP_NO_ATOMICS,
			SHADER_GROUP_VULKAN_MEMORY_MODEL,
			SHADER_GROUP_VULKAN_MEMORY_MODEL_NO_ATOMICS,
		};

		enum FogSet
		{
			FOG_SET_BASE,
			FOG_SET_UNIFORMS,
			FOG_SET_MATERIAL,
			FOG_SET_MAX,
		};

		struct FogPushConstant
		{
			float position[3];
			float pad;

			float size[3];
			float pad2;

			int32_t corner[3];
			uint32_t shape;

			float transform[16];
		};

		struct VolumeUBO
		{
			float fog_frustum_size_begin[2];
			float fog_frustum_size_end[2];

			float fog_frustum_end;
			float z_near;
			float z_far;
			float time;

			int32_t fog_volume_size[3];
			uint32_t directional_light_count;

			uint32_t use_temporal_reprojection;
			uint32_t temporal_frame;
			float detail_spread;
			float temporal_blend;

			float to_prev_view[16];
			float transform[16];
		};

		ShaderCompiler compiler;
		ShaderRD shader;
		RID volume_ubo;

		RID default_shader;
		RID default_material;
		RID default_shader_rd;

		RID base_uniform_set;

		RID params_ubo;

		enum
		{
			VOLUMETRIC_FOG_PROCESS_SHADER_DENSITY,
			VOLUMETRIC_FOG_PROCESS_SHADER_DENSITY_WITH_SDFGI,
			VOLUMETRIC_FOG_PROCESS_SHADER_FILTER,
			VOLUMETRIC_FOG_PROCESS_SHADER_FOG,
			VOLUMETRIC_FOG_PROCESS_SHADER_COPY,
			VOLUMETRIC_FOG_PROCESS_SHADER_MAX,
		};

		struct ParamsUBO
		{
			float fog_frustum_size_begin[2];
			float fog_frustum_size_end[2];

			float fog_frustum_end;
			float ambient_inject;
			float z_far;
			uint32_t filter_axis;

			float ambient_color[3];
			float sky_contribution;

			int32_t fog_volume_size[3];
			uint32_t directional_light_count;

			float base_emission[3];
			float base_density;

			float base_scattering[3];
			float phase_g;

			float detail_spread;
			float gi_inject;
			uint32_t max_voxel_gi_instances;
			uint32_t cluster_type_size;

			float screen_size[2];
			uint32_t cluster_shift;
			uint32_t cluster_width;

			uint32_t max_cluster_element_count_div_32;
			uint32_t use_temporal_reprojection;
			uint32_t temporal_frame;
			float temporal_blend;

			float sky_border_size[2];
			float pad[2];

			float cam_rotation[12];
			float to_prev_view[16];
			float radiance_inverse_xform[12];
		};

		ShaderRD process_shader;

		RID process_shader_version;
		PipelineDeferredRD process_pipelines[VOLUMETRIC_FOG_PROCESS_SHADER_MAX];

		VolumetricFogShader()
			: shader(VolumetricFogShaderRD()), process_shader(VolumetricFogProcessShaderRD())
		{
		}
	};

	struct FogShaderData : public RendererRD::MaterialStorage::ShaderData
	{
		bool valid = false;
		RID version;

		PipelineDeferredRD pipeline;
		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size = 0;

		String code;

		bool uses_time = false;

		virtual void set_code(const String& p_Code) override;
		virtual bool is_animated() const override;
		virtual bool casts_shadows() const override;
		virtual RenderingServerTypes::ShaderNativeSourceCode
		get_native_source_code() const override;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const override;

		FogShaderData() {}

		virtual ~FogShaderData();
	};

	struct FogMaterialData : public RendererRD::MaterialStorage::MaterialData
	{
		FogShaderData* shader_data = nullptr;
		RID uniform_set;
		bool uniform_set_updated;

		virtual void set_render_priority(int p_priority) override {}

		virtual void set_next_pass(RID p_pass) override {}

		virtual ~FogMaterialData();
	};

	/* Volumetric FOG Render Buffer Data */
	class VolumetricFog final : public RefCounted
	{
	public:
		enum
		{
			MAX_TEMPORAL_FRAMES = 16
		};

		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;

		float length;
		float spread;

		RID light_density_map;
		RID prev_light_density_map;
		RID fog_map;
		RID density_map;
		RID light_map;
		RID emissive_map;

		RID fog_uniform_set;
		RID copy_uniform_set;

		struct
		{
			RID process_uniform_set_density;
			RID process_uniform_set;
			RID process_uniform_set2;
		} gi_dependent_sets;

		RID sdfgi_uniform_set;
		RID sky_uniform_set;

		int last_shadow_filter = -1;

		RDC::UniformType atomic_type = RDC::UNIFORM_TYPE_IMAGE;

		void configure(RenderSceneBuffersRD* p_render_buffers) {}

		void free_data() {}

		bool sync_gi_dependent_sets_validity(bool p_ensure_freed = false);

		void init(const Vector3i& fog_size, RID p_sky_shader);
		~VolumetricFog();
	};

	struct VolumetricFogSettings
	{
		Vector2i rb_size;
		double time;
		bool is_using_radiance_octmap_array;
		uint32_t max_cluster_elements;
		bool volumetric_fog_filter_active;
		RID shadow_sampler;
		RID voxel_gi_buffer;
		RID shadow_atlas_depth;
		RID omni_light_buffer;
		RID spot_light_buffer;
		RID area_light_buffer;
		RID area_light_atlas;
		RID directional_shadow_depth;
		RID directional_light_buffer;

		Ref<VolumetricFog> vfog;
		ClusterBuilderRD* cluster_builder;
		Ref<GI::SDFGI> sdfgi;
		Ref<GI::RenderBuffersGI> rbgi;
		RID env;
		SkyRD* sky;
	};

private:
	static inline bool _initialized = false;

	static int _get_fog_shader_group();
	static int _get_fog_variant();
	static int _get_fog_process_variant(int p_idx);

	/* FOG VOLUMES */

	static inline RID_Owner<FogVolume, true> fog_volume_owner;
	static inline RID_Owner<FogVolumeInstance> fog_volume_instance_owner;

	static constexpr int SAMPLERS_BINDING_FIRST_INDEX = 3;

	static inline VolumetricFogShader volumetric_fog;

	static Vector3i _point_get_position_in_froxel_volume(const Vector3& p_point, float fog_end,
		const Vector2& fog_near_size, const Vector2& fog_far_size,
		float volumetric_fog_detail_spread, const Vector3& fog_size,
		const Transform3D& p_cam_transform);

	static RendererRD::MaterialStorage::ShaderData* _create_fog_shader_func();
	static RendererRD::MaterialStorage::ShaderData* _create_fog_shader_funcs();

	static RendererRD::MaterialStorage::MaterialData* _create_fog_material_func(
		FogShaderData* p_shader);
	static RendererRD::MaterialStorage::MaterialData* _create_fog_material_funcs(
		RendererRD::MaterialStorage::ShaderData* p_shader);

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static bool free(RID p_rid);

	/* FOG VOLUMES */

	static bool owns_fog_volume(RID p_rid)
	{
		if (!_initialized) {
			return false;
		}
		return fog_volume_owner.owns(p_rid);
	}

	static RID fog_volume_allocate();
	static void fog_volume_initialize(RID p_rid);
	static void fog_volume_free(RID p_rid);
	static Dependency* fog_volume_get_dependency(RID p_fog_volume);

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape);
	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size);
	static void fog_volume_set_material(RID p_fog_volume, RID p_material);
	static RSE::FogVolumeShape fog_volume_get_shape(RID p_fog_volume);
	static RID fog_volume_get_material(RID p_fog_volume);
	static AABB fog_volume_get_aabb(RID p_fog_volume);
	static Vector3 fog_volume_get_size(RID p_fog_volume);

	/* FOG VOLUMES INSTANCE */

	static bool owns_fog_volume_instance(RID p_rid)
	{
		if (!_initialized) {
			return false;
		}
		return fog_volume_instance_owner.owns(p_rid);
	}

	static RID fog_volume_instance_create(RID p_fog_volume);
	static void fog_instance_free(RID p_rid);

	static void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform)
	{
		Fog::FogVolumeInstance* fvi = fog_volume_instance_owner.get_or_null(p_fog_volume_instance);
		ERR_FAIL_NULL(fvi);
		fvi->transform = p_transform;
	}

	static void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active)
	{
		Fog::FogVolumeInstance* fvi = fog_volume_instance_owner.get_or_null(p_fog_volume_instance);
		ERR_FAIL_NULL(fvi);
		fvi->active = p_active;
	}

	static RID fog_volume_instance_get_volume(RID p_fog_volume_instance)
	{
		Fog::FogVolumeInstance* fvi = fog_volume_instance_owner.get_or_null(p_fog_volume_instance);
		ERR_FAIL_NULL_V(fvi, RID());
		return fvi->volume;
	}

	static Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance)
	{
		Fog::FogVolumeInstance* fvi = fog_volume_instance_owner.get_or_null(p_fog_volume_instance);
		ERR_FAIL_NULL_V(fvi, Vector3());
		return fvi->transform.get_origin();
	}

	/* Volumetric FOG API */

	static void init_fog_shader(uint32_t p_max_directional_lights, int p_roughness_layers,
		bool p_is_using_radiance_octmap_array);
	static void free_fog_shader();

	static void volumetric_fog_update(const VolumetricFogSettings& p_settings,
		const Projection& p_cam_projection, const Transform3D& p_cam_transform,
		const Transform3D& p_prev_cam_inv_transform, RID p_shadow_atlas,
		int p_directional_light_count, bool p_use_directional_shadows, int p_positional_light_count,
		int p_voxel_gi_count, const PagedArray<RID>& p_fog_volumes);
};

} // namespace RendererRD
#include "servers/rendering/renderer_rd/rd_caches.h"
// --- Folded from servers/rendering/renderer_rd/renderer_canvas_render_rd.h ---
#include "core/templates/hash_map.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/renderer_scene_render_rd.h ---
#include "servers/rendering/renderer_compositor.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h ---
#include "core/templates/hash_map.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/effects/copy_effects.h ---
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/blur_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/copy.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/copy_to_fb.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/cube_to_dp.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/cube_to_octmap.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/motion_vectors_store.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_filter.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_filter_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_roughness.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_roughness_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/resolve.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/resolve_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/roughness_limiter.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/sort.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/specular_merge.glsl.gen.h"

class RenderSceneBuffersRD;

namespace RendererRD
{

class CopyEffects final
{
public:
	enum RasterEffects
	{
		RASTER_EFFECT_COPY = 1 << 0,
		RASTER_EFFECT_GAUSSIAN_BLUR = 1 << 1,
		RASTER_EFFECT_OCTMAP = 1 << 2,
	};

	CopyEffects() = delete;
	~CopyEffects() = delete;
	CopyEffects(const CopyEffects&) = delete;
	CopyEffects& operator=(const CopyEffects&) = delete;

private:
	// Blur raster shader

	enum BlurRasterMode
	{
		BLUR_MIPMAP,

		BLUR_MODE_GAUSSIAN_BLUR,
		BLUR_MODE_GAUSSIAN_GLOW_GATHER,
		BLUR_MODE_GAUSSIAN_GLOW_DOWNSAMPLE,
		BLUR_MODE_GAUSSIAN_GLOW_UPSAMPLE,
		BLUR_MODE_COPY,

		BLUR_MODE_SET_COLOR,

		BLUR_MODE_MAX
	};

	enum
	{
		BLUR_FLAG_USE_ORTHOGONAL_PROJECTION = (1 << 1),
	};

	struct BlurRasterPushConstant
	{
		float dest_pixel_size[2];
		float source_pixel_size[2];

		float pad[2];
		uint32_t flags;
		float level;

		// glow
		float glow_strength;
		float glow_bloom;
		float glow_hdr_threshold;
		float glow_hdr_scale;

		float glow_exposure;
		float glow_white;
		float glow_luminance_cap;
		float luminance_multiplier;
	};

	struct BlurRaster
	{
		BlurRasterPushConstant push_constant;
		ShaderRD shader{BlurRasterShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[BLUR_MODE_MAX];
		RID glow_sampler;
	};

	// Copy shader

	enum CopyMode
	{
		COPY_MODE_GAUSSIAN_COPY,
		COPY_MODE_GAUSSIAN_COPY_8BIT,
		COPY_MODE_GAUSSIAN_GLOW,
		COPY_MODE_GAUSSIAN_GLOW_AUTO_EXPOSURE,
		COPY_MODE_SIMPLY_COPY,
		COPY_MODE_SIMPLY_COPY_8BIT,
		COPY_MODE_SIMPLY_COPY_DEPTH,
		COPY_MODE_SET_COLOR,
		COPY_MODE_SET_COLOR_8BIT,
		COPY_MODE_MIPMAP,
		COPY_MODE_LINEARIZE_DEPTH,
		COPY_MODE_OCTMAP_TO_PANORAMA,
		COPY_MODE_OCTMAP_ARRAY_TO_PANORAMA,
		COPY_MODE_MAX,

	};

	enum
	{
		COPY_FLAG_HORIZONTAL = (1 << 0),
		COPY_FLAG_USE_COPY_SECTION = (1 << 1),
		COPY_FLAG_USE_ORTHOGONAL_PROJECTION = (1 << 2),
		COPY_FLAG_DOF_NEAR_FIRST_TAP = (1 << 3),
		COPY_FLAG_GLOW_FIRST_PASS = (1 << 4),
		COPY_FLAG_FLIP_Y = (1 << 5),
		COPY_FLAG_FORCE_LUMINANCE = (1 << 6),
		COPY_FLAG_ALL_SOURCE = (1 << 7),
		COPY_FLAG_ALPHA_TO_ONE = (1 << 8),
		COPY_FLAG_SANITIZE_INF_NAN = (1 << 9),
	};

	struct CopyPushConstant
	{
		int32_t section[4];
		int32_t target[2];
		uint32_t flags;
		float luminance_multiplier;
		// Glow.
		float glow_strength;
		float glow_bloom;
		float glow_hdr_threshold;
		float glow_hdr_scale;

		float glow_exposure;
		float glow_white;
		float glow_luminance_cap;
		float glow_auto_exposure_scale;
		// DOF.
		float camera_z_far;
		float camera_z_near;
		// Octmap.
		float octmap_border_size[2];
		// SET color
		float set_color[4];
	};

	struct Copy
	{
		CopyPushConstant push_constant;
		ShaderRD shader{CopyShaderRD()};
		RID shader_version;
		PipelineDeferredRD pipelines[COPY_MODE_MAX];
	};

	// Copy to FB shader

	enum CopyToFBMode
	{
		COPY_TO_FB_COPY,
		COPY_TO_FB_COPY_PANORAMA_TO_DP,
		COPY_TO_FB_COPY2,
		COPY_TO_FB_SET_COLOR,

		// These variants are disabled unless XR shaders are enabled.
		// They should be listed last.
		COPY_TO_FB_MULTIVIEW,
		COPY_TO_FB_MULTIVIEW_WITH_DEPTH,

		COPY_TO_FB_MAX,
	};

	enum CopyToFBFlags
	{
		COPY_TO_FB_FLAG_FLIP_Y = (1 << 0),
		COPY_TO_FB_FLAG_USE_SECTION = (1 << 1),
		COPY_TO_FB_FLAG_FORCE_LUMINANCE = (1 << 2),
		COPY_TO_FB_FLAG_ALPHA_TO_ZERO = (1 << 3),
		COPY_TO_FB_FLAG_SRGB = (1 << 4),
		COPY_TO_FB_FLAG_ALPHA_TO_ONE = (1 << 5),
		COPY_TO_FB_FLAG_LINEAR = (1 << 6),
		COPY_TO_FB_FLAG_NORMAL = (1 << 7),
		COPY_TO_FB_FLAG_USE_SRC_SECTION = (1 << 8),
	};

	struct CopyToFbPushConstant
	{
		float section[4];
		float pixel_size[2];
		float luminance_multiplier;
		uint32_t flags;

		float set_color[4];
	};

	struct CopyToFb
	{
		CopyToFbPushConstant push_constant;
		ShaderRD shader{CopyToFbShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[COPY_TO_FB_MAX];
	};

	// Copy to DP

	struct CopyToDPPushConstant
	{
		float z_far;
		float z_near;
		float texel_size[2];
	};

	struct CopyToDP
	{
		ShaderRD shader{CubeToDpShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
	};

	// Copy to Octmap

	struct CopyToOctmapPushConstant
	{
		float border_size;
		uint32_t pad[3];
	};

	struct CopyToOctmap
	{
		CopyToOctmapPushConstant push_constant;
		ShaderRD shader{CubeToOctmapShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
	};

	// Octmap effects

	struct OctmapDownsamplerPushConstant
	{
		float border_size;
		uint32_t size;
		uint32_t pad[2];
	};

	enum OctmapDownsamplerMode
	{
		DOWNSAMPLER_MODE_FLAG_RGB10_A2 = (1 << 0),

		DOWNSAMPLER_MODE_COMPUTE_MAX = (DOWNSAMPLER_MODE_FLAG_RGB10_A2 + 1),
		DOWNSAMPLER_MODE_RASTER_MAX = 1,
	};

	struct OctmapDownsampler
	{
		OctmapDownsamplerPushConstant push_constant;
		ShaderRD compute_shader{OctmapDownsamplerShaderRD()};
		ShaderRD raster_shader{OctmapDownsamplerRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[DOWNSAMPLER_MODE_COMPUTE_MAX];
		PipelineCacheRD raster_pipeline;
	};

	enum OctmapFilterMode
	{
		FILTER_MODE_FLAG_HIGH_QUALITY = (1 << 0),
		FILTER_MODE_FLAG_ARRAY = (1 << 1),
		FILTER_MODE_FLAG_RGB10_A2 = (1 << 2),

		FILTER_MODE_COMPUTE_MAX =
			((FILTER_MODE_FLAG_HIGH_QUALITY | FILTER_MODE_FLAG_ARRAY | FILTER_MODE_FLAG_RGB10_A2) +
				1),
		FILTER_MODE_RASTER_MAX = (FILTER_MODE_FLAG_HIGH_QUALITY + 1),
	};

	struct OctmapFilterPushConstant
	{
		float border_size[2];
		uint32_t pad1;
		uint32_t pad2;
	};

	struct OctmapFilterRasterPushConstant
	{
		float border_size[2];
		uint32_t mip_level;
		uint32_t pad;
	};

	struct OctmapFilter
	{
		ShaderRD compute_shader{OctmapFilterShaderRD()};
		ShaderRD raster_shader{OctmapFilterRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[FILTER_MODE_COMPUTE_MAX];
		PipelineCacheRD raster_pipelines[FILTER_MODE_RASTER_MAX];

		RID uniform_set;
		RID coefficient_buffer;
		bool use_high_quality = false;
	};

	enum OctmapRoughnessMode
	{
		ROUGHNESS_MODE_RGBA16F,
		ROUGHNESS_MODE_RGB10_A2,
		ROUGHNESS_MODE_MAX,
	};

	struct OctmapRoughnessPushConstant
	{
		uint32_t sample_count;
		float roughness;
		uint32_t source_size;
		uint32_t dest_size;

		float border_size[2];
		uint32_t use_direct_write;
		uint32_t pad;
	};

	struct OctmapRoughness
	{
		OctmapRoughnessPushConstant push_constant;
		ShaderRD compute_shader{OctmapRoughnessShaderRD()};
		ShaderRD raster_shader{OctmapRoughnessRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[ROUGHNESS_MODE_MAX];
		PipelineCacheRD raster_pipeline;
	};

	// Merge specular

	enum SpecularMergeMode
	{
		SPECULAR_MERGE_ADD,
		SPECULAR_MERGE_SSR,
		SPECULAR_MERGE_ADDITIVE_ADD,
		SPECULAR_MERGE_ADDITIVE_SSR,

		SPECULAR_MERGE_ADD_MULTIVIEW,
		SPECULAR_MERGE_SSR_MULTIVIEW,
		SPECULAR_MERGE_ADDITIVE_ADD_MULTIVIEW,
		SPECULAR_MERGE_ADDITIVE_SSR_MULTIVIEW,

		SPECULAR_MERGE_MAX
	};

	/* Specular merge must be done using raster, rather than compute
	 * because it must continue the existing color buffer
	 */

	struct SpecularMerge
	{
		ShaderRD shader{SpecularMergeShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[SPECULAR_MERGE_MAX];
	};

	struct Data
	{
		uint32_t raster_effects = 0;
		BlurRaster blur_raster;
		Copy copy;
		CopyToFb copy_to_fb;
		CopyToDP cube_to_dp;
		CopyToOctmap cube_to_octmap;
		OctmapDownsampler octmap_downsampler;
		OctmapFilter filter;
		OctmapRoughness roughness;
		SpecularMerge specular_merge;
	};

	static inline Data* data = nullptr;

public:
	static void initialize(uint32_t p_raster_effects = 0);
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static uint32_t get_raster_effects() { return data ? data->raster_effects : 0; }

	static void copy_to_rect(RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_rect,
		bool p_flip_y = false, bool p_force_luminance = false, bool p_all_source = false,
		bool p_8_bit_dst = false, bool p_alpha_to_one = false, bool p_sanitize_inf_nan = false);
	static void copy_octmap_to_panorama(RID p_source_octmap, RID p_dest_panorama,
		const Size2i& p_panorama_size, float p_lod, bool p_is_array,
		const Size2& p_source_octmap_border_size);
	static void copy_depth_to_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
		const Rect2i& p_rect, bool p_flip_y = false);
	static void copy_depth_to_rect_and_linearize(RID p_source_rd_texture, RID p_dest_texture,
		const Rect2i& p_rect, bool p_flip_y, float p_z_near, float p_z_far);
	static void copy_to_fb_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
		const Rect2i& p_rect, bool p_flip_y = false, bool p_force_luminance = false,
		bool p_alpha_to_zero = false, bool p_srgb = false, RID p_secondary = RID(),
		bool p_multiview = false, bool alpha_to_one = false, bool p_linear = false,
		bool p_normal = false, const Rect2& p_src_rect = Rect2(),
		float p_linear_luminance_multiplier = 1.0, bool p_bilinear_filtering = true);
	static void copy_to_atlas_fb(RID p_source_rd_texture, RID p_dest_framebuffer,
		const Rect2& p_uv_rect, RD::DrawListID p_draw_list, bool p_flip_y = false,
		bool p_panorama = false);
	static void copy_to_drawlist(RD::DrawListID p_draw_list, RD::FramebufferFormatID p_fb_format,
		RID p_source_rd_texture, bool p_linear = false, float p_linear_luminance_multiplier = 1.0);
	static void copy_raster(RID p_source_texture, RID p_dest_framebuffer);

	static void gaussian_blur(RID p_source_rd_texture, RID p_texture, const Rect2i& p_region,
		const Size2i& p_size, bool p_8bit_dst = false);
	static void gaussian_blur_raster(
		RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_region, const Size2i& p_size);
	static void gaussian_glow(RID p_source_rd_texture, RID p_back_texture, const Size2i& p_size,
		float p_strength = 1.0, bool p_first_pass = false, float p_luminance_cap = 16.0,
		float p_exposure = 1.0, float p_bloom = 0.0, float p_hdr_bleed_threshold = 1.0,
		float p_hdr_bleed_scale = 1.0, RID p_auto_exposure = RID(),
		float p_auto_exposure_scale = 1.0);
	static void gaussian_glow_downsample_raster(RID p_source_rd_texture, RID p_dest_texture,
		float p_luminance_multiplier, const Size2i& p_size, float p_strength = 1.0,
		bool p_first_pass = false, float p_luminance_cap = 16.0, float p_exposure = 1.0,
		float p_bloom = 0.0, float p_hdr_bleed_threshold = 1.0, float p_hdr_bleed_scale = 1.0);
	static void gaussian_glow_upsample_raster(RID p_source_rd_texture, RID p_dest_texture,
		RID p_blend_texture, float p_luminance_multiplier, const Size2i& p_source_size,
		const Size2i& p_dest_size, float p_level, float p_base_strength, bool p_use_debanding);

	static void make_mipmap(RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size);
	static void make_mipmap_raster(
		RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size);

	static void set_color(
		RID p_dest_texture, const Color& p_color, const Rect2i& p_region, bool p_8bit_dst = false);
	static void set_color_raster(RID p_dest_texture, const Color& p_color, const Rect2i& p_region);

	static void copy_cubemap_to_dp(RID p_source_rd_texture, RID p_dst_framebuffer,
		const Rect2& p_rect, const Vector2& p_dst_size, float p_z_near, float p_z_far,
		bool p_dp_flip);
	static void copy_cubemap_to_octmap(
		RID p_source_rd_texture, RID p_dst_framebuffer, float p_border_size);
	static void octmap_downsample(
		RID p_source_octmap, RID p_dest_octmap, const Size2i& p_size, float p_border_size);
	static void octmap_downsample_raster(
		RID p_source_octmap, RID p_dest_framebuffer, const Size2i& p_size, float p_border_size);
	static void octmap_filter(RID p_source_octmap, const Vector<RID>& p_dest_octmap,
		bool p_use_array, float p_border_size);
	static void octmap_filter_raster(
		RID p_source_octmap, RID p_dest_framebuffer, uint32_t p_mip_level, float p_border_size);
	static void octmap_roughness(RID p_source_rd_texture, RID p_dest_texture,
		uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
		float p_border_size);
	static void octmap_roughness_raster(RID p_source_rd_texture, RID p_dest_framebuffer,
		uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
		float p_border_size);

	static void merge_specular(RID p_dest_framebuffer, RID p_specular, RID p_base,
		RID p_reflection, uint32_t p_view_count);
};

class Resolve final
{
private:
	bool prefer_raster_effects;

	struct ResolvePushConstant
	{
		int32_t screen_size[2];
		int32_t samples;
		uint32_t pad;
	};

	enum ResolveMode
	{
		RESOLVE_MODE_GI,
		RESOLVE_MODE_GI_VOXEL_GI,
		RESOLVE_MODE_DEPTH,
		RESOLVE_MODE_MAX
	};

	struct ResolveShader
	{
		ResolvePushConstant push_constant;
		ShaderRD shader{ResolveShaderRD()};
		RID shader_version;
		RID pipelines[RESOLVE_MODE_MAX]; // 3 quality levels
	} resolve;

	struct ResolveRasterShader
	{
		ResolvePushConstant push_constant;
		ShaderRD shader{ResolveRasterShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
	} resolve_raster;

public:
	Resolve(bool p_prefer_raster_effects);
	~Resolve();

	void resolve_gi(RID p_source_depth, RID p_source_normal_roughness, RID p_source_voxel_gi,
		RID p_dest_depth, RID p_dest_normal_roughness, RID p_dest_voxel_gi, Vector2i p_screen_size,
		int p_samples);
	void resolve_depth(RID p_source_depth, RID p_dest_depth, Vector2i p_screen_size, int p_samples);
	void resolve_depth_raster(RID p_source_rd_texture, RID p_dest_framebuffer, int p_samples);
};

// Note, this logic is unused at the time of writing. It should be re-incorporated into the renderer
// at some point.
class RoughnessLimiter final
{
private:
	struct RoughnessLimiterPushConstant
	{
		int32_t screen_size[2];
		float curve;
		uint32_t pad;
	};

	RoughnessLimiterPushConstant push_constant;
	ShaderRD shader{RoughnessLimiterShaderRD()};
	RID shader_version;
	RID pipeline;

public:
	RoughnessLimiter();
	~RoughnessLimiter();

	void roughness_limit(RID p_source_normal, RID p_roughness, const Size2i& p_size, float p_curve);
};

class SortEffects final
{
private:
	enum SortMode
	{
		SORT_MODE_BLOCK,
		SORT_MODE_STEP,
		SORT_MODE_INNER,
		SORT_MODE_MAX
	};

	struct PushConstant
	{
		uint32_t total_elements;
		uint32_t pad[3];
		int32_t job_params[4];
	};

	ShaderRD shader{SortShaderRD()};
	RID shader_version;
	RID pipelines[SORT_MODE_MAX];

public:
	SortEffects();
	~SortEffects();

	void sort_buffer(RID p_uniform_set, int p_size);
};

class MotionVectorsStore final
{
	struct PushConstant
	{
		float reprojection_matrix[16];
		float resolution[2];
		uint32_t pad[2];
	};

	ShaderRD motion_shader{MotionVectorsStoreShaderRD()};
	RID shader_version;
	RID pipeline;

public:
	MotionVectorsStore();
	~MotionVectorsStore();

	void process(Ref<RenderSceneBuffersRD> p_render_buffers, const Projection& p_current_projection,
		const Transform3D& p_current_transform, const Projection& p_previous_projection,
		const Transform3D& p_previous_transform);
};

} // namespace RendererRD

// --- Folded from servers/rendering/renderer_rd/effects/post_effects.h ---
#include "core/types.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/bokeh_dof.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/bokeh_dof_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/luminance_reduce.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/luminance_reduce_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/motion_vectors.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/shadow_frustum.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/smaa_blending.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/smaa_edge_detection.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/smaa_weight_calculation.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/tonemap.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/tonemap_mobile.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/vrs.glsl.gen.h"

#define RB_LUMINANCE_BUFFERS SNAME("luminance_buffers")

#define RB_SCOPE_SMAA SNAME("rb_smaa")

#ifndef RB_EDGES
#define RB_EDGES SNAME("edges")
#endif
#define RB_BLEND SNAME("blend")
#define RB_STENCIL SNAME("stencil")

class RenderSceneBuffersRD;

namespace RendererRD
{

class BokehDOF final
{
private:
	bool prefer_raster_effects;

	struct BokehPushConstant
	{
		uint32_t size[2];
		float z_far;
		float z_near;

		uint32_t orthogonal;
		float blur_size;
		float blur_scale;
		uint32_t steps;

		uint32_t blur_near_active;
		float blur_near_begin;
		float blur_near_end;
		uint32_t blur_far_active;

		float blur_far_begin;
		float blur_far_end;
		uint32_t second_pass;
		uint32_t half_size;

		uint32_t use_jitter;
		float jitter_seed;
		uint32_t use_physical_near;
		uint32_t use_physical_far;

		float blur_size_near;
		float blur_size_far;
		uint32_t pad[2];
	};

	enum BokehMode
	{
		BOKEH_GEN_BLUR_SIZE,
		BOKEH_GEN_BOKEH_BOX,
		BOKEH_GEN_BOKEH_BOX_NOWEIGHT,
		BOKEH_GEN_BOKEH_HEXAGONAL,
		BOKEH_GEN_BOKEH_HEXAGONAL_NOWEIGHT,
		BOKEH_GEN_BOKEH_CIRCULAR,
		BOKEH_COMPOSITE,
		BOKEH_MAX
	};

	struct Bokeh
	{
		BokehPushConstant push_constant;
		ShaderRD compute_shader{BokehDofShaderRD()};
		ShaderRD raster_shader{BokehDofRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[BOKEH_MAX];
		PipelineCacheRD raster_pipelines[BOKEH_MAX];
	} bokeh;

public:
	struct BokehBuffers
	{
		// bokeh buffers

		// textures
		Size2i base_texture_size;
		RID base_texture;
		RID depth_texture;
		RID secondary_texture;
		RID half_texture[2];

		// raster only
		RID base_fb;
		RID secondary_fb; // with weights
		RID half_fb[2];	  // with weights
		RID base_weight_fb;
		RID weight_texture[4];
	};

	BokehDOF(bool p_prefer_raster_effects);
	~BokehDOF();

	void bokeh_dof_compute(const BokehBuffers& p_buffers, RID p_camera_attributes,
		float p_cam_znear, float p_cam_zfar, bool p_cam_orthogonal);
	void bokeh_dof_raster(const BokehBuffers& p_buffers, RID p_camera_attributes, float p_cam_znear,
		float p_cam_zfar, bool p_cam_orthogonal);
};

class ToneMapper final
{
private:
	bool using_mobile_version = false;

	enum TonemapMode
	{
		TONEMAP_MODE_NORMAL,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER,
		TONEMAP_MODE_1D_LUT,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER_1D_LUT,

		TONEMAP_MODE_NORMAL_MULTIVIEW,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER_MULTIVIEW,
		TONEMAP_MODE_1D_LUT_MULTIVIEW,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER_1D_LUT_MULTIVIEW,

		TONEMAP_MODE_MAX
	};

	enum TonemapModeMobile
	{
		TONEMAP_MOBILE_MODE_NORMAL,
		TONEMAP_MOBILE_MODE_1D_LUT,
		TONEMAP_MOBILE_MODE_SUBPASS,
		TONEMAP_MOBILE_MODE_SUBPASS_1D_LUT,

		TONEMAP_MOBILE_MODE_NORMAL_MULTIVIEW,
		TONEMAP_MOBILE_MODE_1D_LUT_MULTIVIEW,
		TONEMAP_MOBILE_MODE_SUBPASS_MULTIVIEW,
		TONEMAP_MOBILE_MODE_SUBPASS_1D_LUT_MULTIVIEW,

		TONEMAP_MOBILE_MODE_MAX
	};

	enum Flags
	{
		TONEMAP_FLAG_USE_BCS = (1 << 0),
		TONEMAP_FLAG_USE_GLOW = (1 << 1),
		TONEMAP_FLAG_USE_AUTO_EXPOSURE = (1 << 2),
		TONEMAP_FLAG_USE_COLOR_CORRECTION = (1 << 3),
		TONEMAP_FLAG_USE_FXAA = (1 << 4),
		TONEMAP_FLAG_USE_8_BIT_DEBANDING = (1 << 5),
		TONEMAP_FLAG_CONVERT_TO_SRGB = (1 << 6),
	};

	enum FlagsMobile
	{
		TONEMAP_MOBILE_FLAG_USE_BCS = (1 << 0),
		TONEMAP_MOBILE_FLAG_USE_GLOW = (1 << 1),
		TONEMAP_MOBILE_FLAG_USE_GLOW_MAP = (1 << 2),
		TONEMAP_MOBILE_FLAG_USE_COLOR_CORRECTION = (1 << 3),
		TONEMAP_MOBILE_FLAG_USE_FXAA = (1 << 4),
		TONEMAP_MOBILE_FLAG_USE_8_BIT_DEBANDING = (1 << 5),
		TONEMAP_MOBILE_FLAG_USE_10_BIT_DEBANDING = (1 << 6),
		TONEMAP_MOBILE_FLAG_CONVERT_TO_SRGB = (1 << 7),

		TONEMAP_MOBILE_FLAG_TONEMAPPER_LINEAR = (1 << 8),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_REINHARD = (1 << 9),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_FILMIC = (1 << 10),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_ACES = (1 << 11),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_AGX = (1 << 12),

		TONEMAP_MOBILE_FLAG_GLOW_MODE_ADD = (1 << 13),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_SCREEN = (1 << 14),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_SOFTLIGHT = (1 << 15),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_REPLACE = (1 << 16),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_MIX = (1 << 17),
		TONEMAP_MOBILE_ADRENO_BUG = (1 << 18), // Needs to be last so we force the pipeline cache to
											   // specify specializations for all variants.
	};

	struct TonemapPushConstant
	{
		float bcs[3];	// 12 - 12
		uint32_t flags; //  4 - 16

		float pixel_size[2];	//  8 - 24
		uint32_t tonemapper;	//  4 - 28
		float output_max_value; //  4 - 32

		uint32_t glow_texture_size[2]; //  8 - 40
		float glow_intensity;		   //  4 - 44
		float glow_map_strength;	   //  4 - 48

		uint32_t glow_mode;	  //  4 - 52
		float glow_levels[7]; // 28 - 80

		float exposure;				//  4 - 84
		float white;				//  4 - 88
		float auto_exposure_scale;	//  4 - 92
		float luminance_multiplier; //  4 - 96

		float tonemapper_params[4]; //  16 - 112
	};

	struct TonemapPushConstantMobile
	{
		float bcs[3];				// 12 - 12
		float luminance_multiplier; //  4 - 16

		float src_pixel_size[2];  //  8 - 24
		float dest_pixel_size[2]; //  8 - 32

		float glow_intensity;	 //  4 - 36
		float glow_map_strength; //  4 - 40
		float exposure;			 //  4 - 44
		float white;			 //  4 - 48

		float tonemapper_params[4]; //  16 - 64
		float output_max_value;		//  4 - 68
		float pad[3];				//  12 - 80
	};

	/* tonemap actually writes to a framebuffer, which is
	 * better to do using the raster pipeline rather than
	 * compute, as that framebuffer might be in different formats
	 */
	struct Tonemap
	{
		TonemapPushConstant push_constant;
		ShaderRD shader{TonemapShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[TONEMAP_MODE_MAX];
	} tonemap;

	struct TonemapMobile
	{
		TonemapPushConstantMobile push_constant;
		ShaderRD shader{TonemapMobileShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[TONEMAP_MOBILE_MODE_MAX];
	} tonemap_mobile;

public:
	ToneMapper(bool p_use_mobile_version);
	~ToneMapper();

	struct TonemapSettings
	{
		bool use_glow = false;
		RSE::EnvironmentGlowBlendMode glow_mode = RSE::ENV_GLOW_BLEND_MODE_SCREEN;
		float glow_intensity = 0.3;
		float glow_map_strength = 0.0f;
		float glow_levels[7] = {1.0, 0.8, 0.4, 0.1, 0.0, 0.0, 0.0};
		Vector2i glow_texture_size;
		bool glow_use_bicubic_upscale = false;
		RID glow_texture;
		RID glow_map;

		RSE::EnvironmentToneMapper tonemap_mode = RSE::ENV_TONE_MAPPER_LINEAR;
		float tonemapper_params[4] = {0.0, 0.0, 0.0, 0.0};
		float exposure = 1.0;
		float white = 1.0;
		float max_value = 1.0;

		bool use_auto_exposure = false;
		float auto_exposure_scale = 0.5;
		RID exposure_texture;
		float luminance_multiplier = 1.0;

		bool use_bcs = false;
		float brightness = 1.0;
		float contrast = 1.0;
		float saturation = 1.0;

		bool use_color_correction = false;
		bool use_1d_color_correction = false;
		RID color_correction_texture;

		bool use_fxaa = false;

		enum DebandingMode
		{
			DEBANDING_MODE_DISABLED,
			DEBANDING_MODE_8_BIT,
			DEBANDING_MODE_10_BIT,
		};

		DebandingMode debanding_mode = DEBANDING_MODE_DISABLED;
		Vector2i texture_size;
		Vector2i dest_texture_size;
		uint32_t view_count = 1;
		bool bilinear_filtering = true;

		bool convert_to_srgb = false;
	};

	void tonemapper(RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings);
	void tonemapper_mobile(
		RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings);
	void tonemapper_subpass(RD::DrawListID p_subpass_draw_list, RID p_source_color,
		RD::FramebufferFormatID p_dst_format_id, const TonemapSettings& p_settings);
};

class Luminance final
{
private:
	bool prefer_raster_effects;

	enum LuminanceReduceMode
	{
		LUMINANCE_REDUCE_READ,
		LUMINANCE_REDUCE,
		LUMINANCE_REDUCE_WRITE,
		LUMINANCE_REDUCE_MAX
	};

	struct LuminanceReducePushConstant
	{
		int32_t source_size[2];
		float max_luminance;
		float min_luminance;
		float exposure_adjust;
		float pad[3];
	};

	struct LuminanceReduce
	{
		ShaderRD shader{LuminanceReduceShaderRD()};
		RID shader_version;
		RID pipelines[LUMINANCE_REDUCE_MAX];
	} luminance_reduce;

	enum LuminanceReduceRasterMode
	{
		LUMINANCE_REDUCE_FRAGMENT_FIRST,
		LUMINANCE_REDUCE_FRAGMENT,
		LUMINANCE_REDUCE_FRAGMENT_FINAL,
		LUMINANCE_REDUCE_FRAGMENT_MAX
	};

	struct LuminanceReduceRasterPushConstant
	{
		int32_t source_size[2];
		int32_t dest_size[2];
		float exposure_adjust;
		float min_luminance;
		float max_luminance;
		uint32_t pad1;
	};

	struct LuminanceReduceFragment
	{
		ShaderRD shader{LuminanceReduceRasterShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[LUMINANCE_REDUCE_FRAGMENT_MAX];
	} luminance_reduce_raster;

public:
	class LuminanceBuffers final : public RefCounted
	{
	private:
		bool prefer_raster_effects;

	public:
		Vector<RID> reduce;
		RID current;

		void configure(RenderSceneBuffersRD* p_render_buffers);
		void free_data();

		void set_prefer_raster_effects(bool p_prefer_raster_effects);
	};

	Ref<LuminanceBuffers> get_luminance_buffers(Ref<RenderSceneBuffersRD> p_render_buffers);
	RID get_current_luminance_buffer(Ref<RenderSceneBuffersRD> p_render_buffers);
	void luminance_reduction(RID p_source_texture, const Size2i p_source_size,
		Ref<LuminanceBuffers> p_luminance_buffers, float p_min_luminance, float p_max_luminance,
		float p_adjust, bool p_set = false);

	Luminance(bool p_prefer_raster_effects);
	~Luminance();
};

class SMAA final
{
private:
	enum SMAAMode
	{
		SMAA_EDGE_DETECTION_COLOR,
		SMAA_WEIGHT_FULL,
		SMAA_BLENDING,
		SMAA_MAX,
	};

	struct SMAAEdgePushConstant
	{
		float inv_size[2];
		float threshold;
		float pad;
	};

	struct SMAAWeightPushConstant
	{
		float inv_size[2];
		uint32_t size[2];

		float subsample_indices[4];
	};

	struct SMAABlendPushConstant
	{
		float inv_size[2];
		uint32_t use_debanding;
		float pad;
	};

	enum SMAABlendFlags
	{
		SMAA_BLEND_FLAG_USE_8_BIT_DEBANDING = (1 << 0),
		SMAA_BLEND_FLAG_USE_10_BIT_DEBANDING = (1 << 1),
	};

	struct SMAAEffect
	{
		SMAAEdgePushConstant edge_push_constant;
		ShaderRD edge_shader{SmaaEdgeDetectionShaderRD()};
		RID edge_shader_version;

		SMAAWeightPushConstant weight_push_constant;
		ShaderRD weight_shader{SmaaWeightCalculationShaderRD()};
		RID weight_shader_version;

		SMAABlendPushConstant blend_push_constant;
		ShaderRD blend_shader{SmaaBlendingShaderRD()};
		RID blend_shader_version;

		RID search_tex;
		RID area_tex;

		RDC::DataFormat stencil_format;

		PipelineCacheRD pipelines[SMAA_MAX];
	} smaa;

	float edge_detection_threshold;

public:
	SMAA();
	~SMAA();

	void allocate_render_targets(Ref<RenderSceneBuffersRD> p_render_buffers);
	void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_color,
		RID p_dst_framebuffer, bool p_use_debanding);
};

class DebugEffects final
{
private:
	struct
	{
		RD::VertexFormatID vertex_format;
		RID vertex_buffer;
		RID vertex_array;

		RID index_buffer;
		RID index_array;

		RID lines_buffer;
		RID lines_array;
	} frustum;

	struct ShadowFrustumPushConstant
	{
		float mvp[16];
		float color[4];
	};

	enum ShadowFrustumPipelines
	{
		SFP_TRANSPARENT,
		SFP_WIREFRAME,
		SFP_MAX
	};

	struct
	{
		ShaderRD shader{ShadowFrustumShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[SFP_MAX];
	} shadow_frustum;

	struct MotionVectorsPushConstant
	{
		float reprojection_matrix[16];
		float resolution[2];
		uint32_t force_derive_from_depth;
		uint32_t pad;
	};

	struct
	{
		ShaderRD shader{MotionVectorsShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
		MotionVectorsPushConstant push_constant;
	} motion_vectors;

	void _create_frustum_arrays();

public:
	DebugEffects();
	~DebugEffects();

	void draw_shadow_frustum(RID p_light, const Projection& p_cam_projection,
		const Transform3D& p_cam_transform, RID p_dest_fb, const Rect2 p_rect);
	void draw_motion_vectors(RID p_velocity, RID p_depth, RID p_dest_fb,
		const Projection& p_current_projection, const Transform3D& p_current_transform,
		const Projection& p_previous_projection, const Transform3D& p_previous_transform,
		Size2i p_resolution);
};

class VRS final
{
private:
	enum VRSMode
	{
		VRS_DEFAULT,
		VRS_MULTIVIEW,
		VRS_RG,
		VRS_RG_MULTIVIEW,
		VRS_MAX,
	};

	struct VRSPushConstant
	{
		float max_texel_factor; // 4x8, 8x4 and 8x8 are only available on some GPUs.
		float res1;
		float res2;
		float res3;
	};

	struct VRSShader
	{
		// VRSPushConstant push_constant;
		ShaderRD shader{VrsShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[VRS_MAX];
	} vrs_shader;

public:
	VRS();
	~VRS();

	void copy_vrs(RID p_source_rd_texture, RID p_dest_framebuffer, bool p_multiview = false);

	Size2i get_vrs_texture_size(const Size2i p_base_size) const;
	void update_vrs_texture(RID p_vrs_fb, RID p_render_target);
};

} // namespace RendererRD

// --- End servers/rendering/renderer_rd/effects/post_effects.h ---

// --- Folded from servers/rendering/renderer_rd/effects/ss_effects.h ---
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_downsample.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_filter.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_hiz.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_resolve.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ss_effects_downsample.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao_blur.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao_importance_map.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao_interleave.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil_blur.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil_importance_map.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil_interleave.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/subsurface_scattering.glsl.gen.h"

#define RB_SCOPE_SSLF SNAME("rb_sslf")
#define RB_SCOPE_SSDS SNAME("rb_ssds")
#define RB_SCOPE_SSIL SNAME("rb_ssil")
#define RB_SCOPE_SSAO SNAME("rb_ssao")
#define RB_SCOPE_SSR SNAME("rb_ssr")

#define RB_LINEAR_DEPTH SNAME("linear_depth")
#define RB_FINAL SNAME("final")
#define RB_LAST_FRAME SNAME("last_frame")
#define RB_DEINTERLEAVED SNAME("deinterleaved")
#define RB_DEINTERLEAVED_PONG SNAME("deinterleaved_pong")
#ifndef RB_EDGES
#define RB_EDGES SNAME("edges")
#endif
#define RB_IMPORTANCE_MAP SNAME("importance_map")
#define RB_IMPORTANCE_PONG SNAME("importance_pong")

#define RB_NORMAL_ROUGHNESS SNAME("normal_roughness")
#define RB_HIZ SNAME("hiz")
#define RB_SSR SNAME("ssr")
#define RB_MIP_LEVEL SNAME("mip_level")

class RenderSceneBuffersRD;

namespace RendererRD
{

class SSEffects final
{
public:
	SSEffects() = delete;
	~SSEffects() = delete;
	SSEffects(const SSEffects&) = delete;
	SSEffects& operator=(const SSEffects&) = delete;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	/* Last Frame */

	static void allocate_last_frame_buffer(
		Ref<RenderSceneBuffersRD> p_render_buffers, bool p_use_ssil, bool p_use_ssr);
	static void copy_internal_texture_to_last_frame(Ref<RenderSceneBuffersRD> p_render_buffers);

	/* SS Downsampler */

	static void downsample_depth(
		Ref<RenderSceneBuffersRD> p_render_buffers, uint32_t p_view, const Projection& p_projection);

	/* SSIL */
	static void ssil_set_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to);

	struct SSILRenderBuffers
	{
		bool half_size = false;
		int buffer_width;
		int buffer_height;
		int half_buffer_width;
		int half_buffer_height;
	};

	struct SSILSettings
	{
		float radius = 1.0;
		float intensity = 2.0;
		float sharpness = 0.98;
		float normal_rejection = 1.0;

		Size2i full_screen_size;
	};

	static void ssil_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSILRenderBuffers& p_ssil_buffers, const SSILSettings& p_settings);
	static void screen_space_indirect_lighting(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSILRenderBuffers& p_ssil_buffers, uint32_t p_view, RID p_normal_buffer,
		const Projection& p_projection, const Projection& p_last_projection,
		const SSILSettings& p_settings);

	/* SSAO */
	static void ssao_set_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to);

	struct SSAORenderBuffers
	{
		bool half_size = false;
		int buffer_width;
		int buffer_height;
		int half_buffer_width;
		int half_buffer_height;
	};

	struct SSAOSettings
	{
		float radius = 1.0;
		float intensity = 2.0;
		float power = 1.5;
		float detail = 0.5;
		float horizon = 0.06;
		float sharpness = 0.98;

		Size2i full_screen_size;
	};

	static void ssao_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSAORenderBuffers& p_ssao_buffers, const SSAOSettings& p_settings);
	static void generate_ssao(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSAORenderBuffers& p_ssao_buffers, uint32_t p_view, RID p_normal_buffer,
		const Projection& p_projection, const SSAOSettings& p_settings);

	/* Screen Space Reflection */
	static void ssr_set_half_size(bool p_half_size);

	struct SSRRenderBuffers
	{
		Size2i size;
		uint32_t mipmaps = 1;
		bool half_size = false;
	};

	static void ssr_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSRRenderBuffers& p_ssr_buffers, const RDC::DataFormat p_color_format);
	static void screen_space_reflection(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSRRenderBuffers& p_ssr_buffers, const RID* p_normal_roughness_slices, int p_max_steps,
		float p_fade_in, float p_fade_out, float p_tolerance, const Projection* p_projections,
		const Projection* p_reprojections, const Vector3* p_eye_offsets);

	/* subsurface scattering */
	static void sss_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	static RSE::SubSurfaceScatteringQuality sss_get_quality();
	static void sss_set_scale(float p_scale, float p_depth_scale);

	static void sub_surface_scattering(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_diffuse,
		RID p_depth, const Projection& p_camera, const Size2i& p_screen_size);

private:
	/* SS Downsampler */

	struct SSEffectsDownsamplePushConstant
	{
		float pixel_size[2];
		float z_far;
		float z_near;
		uint32_t orthogonal;
		float radius_sq;
		uint32_t pad[2];
	};

	enum SSEffectsMode
	{
		SS_EFFECTS_DOWNSAMPLE,
		SS_EFFECTS_DOWNSAMPLE_HALF_RES,
		SS_EFFECTS_DOWNSAMPLE_MIPMAP,
		SS_EFFECTS_DOWNSAMPLE_MIPMAP_HALF_RES,
		SS_EFFECTS_DOWNSAMPLE_HALF,
		SS_EFFECTS_DOWNSAMPLE_HALF_RES_HALF,
		SS_EFFECTS_DOWNSAMPLE_FULL_MIPS,
		SS_EFFECTS_MAX
	};

	struct SSEffectsGatherConstants
	{
		float rotation_matrices[80]; // 5 vec4s * 4
	};

	struct SSEffectsShader
	{
		SSEffectsDownsamplePushConstant downsample_push_constant;
		ShaderRD downsample_shader{SsEffectsDownsampleShaderRD()};
		RID downsample_shader_version;
		bool used_half_size_last_frame = false;
		bool used_mips_last_frame = false;
		bool used_full_mips_last_frame = false;

		RID gather_constants_buffer;

		RID mirror_sampler;

		PipelineDeferredRD pipelines[SS_EFFECTS_MAX];
	};

	/* SSIL */

	enum SSILMode
	{
		SSIL_GATHER,
		SSIL_GATHER_BASE,
		SSIL_GATHER_ADAPTIVE,
		SSIL_GENERATE_IMPORTANCE_MAP,
		SSIL_PROCESS_IMPORTANCE_MAPA,
		SSIL_PROCESS_IMPORTANCE_MAPB,
		SSIL_BLUR_PASS,
		SSIL_BLUR_PASS_SMART,
		SSIL_BLUR_PASS_WIDE,
		SSIL_INTERLEAVE,
		SSIL_INTERLEAVE_SMART,
		SSIL_INTERLEAVE_HALF,
		SSIL_MAX
	};

	struct SSILGatherPushConstant
	{
		int32_t screen_size[2];
		int pass;
		int quality;

		float half_screen_pixel_size[2];
		float half_screen_pixel_size_x025[2];

		float NDC_to_view_mul[2];
		float NDC_to_view_add[2];

		float pad2[2];
		float z_near;
		float z_far;

		float radius;
		float intensity;
		int size_multiplier;
		int pad;

		float fade_out_mul;
		float fade_out_add;
		float normal_rejection_amount;
		float inv_radius_near_limit;

		uint32_t is_orthogonal;
		float neg_inv_radius;
		float load_counter_avg_div;
		float adaptive_sample_limit;

		int32_t pass_coord_offset[2];
		float pass_uv_offset[2];
	};

	struct SSILImportanceMapPushConstant
	{
		float half_screen_pixel_size[2];
		float intensity;
		float pad;
	};

	struct SSILBlurPushConstant
	{
		float edge_sharpness;
		float pad;
		float half_screen_pixel_size[2];
	};

	struct SSILInterleavePushConstant
	{
		float inv_sharpness;
		uint32_t size_modifier;
		float pixel_size[2];
	};

	struct SSILProjectionUniforms
	{
		float inv_last_frame_projection_matrix[16];
	};

	struct SSIL
	{
		SSILGatherPushConstant gather_push_constant;
		ShaderRD gather_shader{SsilShaderRD()};
		RID gather_shader_version;
		RID projection_uniform_buffer;

		SSILImportanceMapPushConstant importance_map_push_constant;
		ShaderRD importance_map_shader{SsilImportanceMapShaderRD()};
		RID importance_map_shader_version;
		RID importance_map_load_counter;
		RID counter_uniform_set;

		SSILBlurPushConstant blur_push_constant;
		ShaderRD blur_shader{SsilBlurShaderRD()};
		RID blur_shader_version;

		SSILInterleavePushConstant interleave_push_constant;
		ShaderRD interleave_shader{SsilInterleaveShaderRD()};
		RID interleave_shader_version;

		PipelineDeferredRD pipelines[SSIL_MAX];
	};

	static void gather_ssil(RD::ComputeListID p_compute_list, const RID* p_ssil_slices,
		const RID* p_edges_slices, const SSILSettings& p_settings, bool p_adaptive_base_pass,
		RID p_gather_uniform_set, RID p_importance_map_uniform_set, RID p_projection_uniform_set);

	/* SSAO */

	enum SSAOMode
	{
		SSAO_GATHER,
		SSAO_GATHER_BASE,
		SSAO_GATHER_ADAPTIVE,
		SSAO_GENERATE_IMPORTANCE_MAP,
		SSAO_PROCESS_IMPORTANCE_MAPA,
		SSAO_PROCESS_IMPORTANCE_MAPB,
		SSAO_BLUR_PASS,
		SSAO_BLUR_PASS_SMART,
		SSAO_BLUR_PASS_WIDE,
		SSAO_INTERLEAVE,
		SSAO_INTERLEAVE_SMART,
		SSAO_INTERLEAVE_HALF,
		SSAO_MAX
	};

	struct SSAOGatherPushConstant
	{
		int32_t screen_size[2];
		int pass;
		int quality;

		float half_screen_pixel_size[2];
		int size_multiplier;
		float detail_intensity;

		float NDC_to_view_mul[2];
		float NDC_to_view_add[2];

		float pad[2];
		float half_screen_pixel_size_x025[2];

		float radius;
		float intensity;
		float shadow_power;
		float shadow_clamp;

		float fade_out_mul;
		float fade_out_add;
		float horizon_angle_threshold;
		float inv_radius_near_limit;

		uint32_t is_orthogonal;
		float neg_inv_radius;
		float load_counter_avg_div;
		float adaptive_sample_limit;

		int32_t pass_coord_offset[2];
		float pass_uv_offset[2];
	};

	struct SSAOImportanceMapPushConstant
	{
		float half_screen_pixel_size[2];
		float intensity;
		float power;
	};

	struct SSAOBlurPushConstant
	{
		float edge_sharpness;
		float pad;
		float half_screen_pixel_size[2];
	};

	struct SSAOInterleavePushConstant
	{
		float inv_sharpness;
		uint32_t size_modifier;
		float pixel_size[2];
	};

	struct SSAO
	{
		SSAOGatherPushConstant gather_push_constant;
		ShaderRD gather_shader{SsaoShaderRD()};
		RID gather_shader_version;

		SSAOImportanceMapPushConstant importance_map_push_constant;
		ShaderRD importance_map_shader{SsaoImportanceMapShaderRD()};
		RID importance_map_shader_version;
		RID importance_map_load_counter;
		RID counter_uniform_set;

		SSAOBlurPushConstant blur_push_constant;
		ShaderRD blur_shader{SsaoBlurShaderRD()};
		RID blur_shader_version;

		SSAOInterleavePushConstant interleave_push_constant;
		ShaderRD interleave_shader{SsaoInterleaveShaderRD()};
		RID interleave_shader_version;

		PipelineDeferredRD pipelines[SSAO_MAX];
	};

	static void gather_ssao(RD::ComputeListID p_compute_list, const RID* p_ao_slices,
		const SSAOSettings& p_settings, bool p_adaptive_base_pass, RID p_gather_uniform_set,
		RID p_importance_map_uniform_set);

	/* Screen Space Reflection */

	enum ScreenSpaceReflectionDownsampleMode
	{
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_DEFAULT,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_ODD_WIDTH,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_ODD_HEIGHT,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_ODD_WIDTH_AND_HEIGHT,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX
	};

	struct ScreenSpaceReflectionDownsamplePushConstant
	{
		int32_t screen_size[2];
		int32_t pad[2];
	};

	enum ScreenSpaceReflectionHizMode
	{
		SCREEN_SPACE_REFLECTION_HIZ_DEFAULT,
		SCREEN_SPACE_REFLECTION_HIZ_ODD_WIDTH,
		SCREEN_SPACE_REFLECTION_HIZ_ODD_HEIGHT,
		SCREEN_SPACE_REFLECTION_HIZ_ODD_WIDTH_AND_HEIGHT,
		SCREEN_SPACE_REFLECTION_HIZ_MAX
	};

	struct ScreenSpaceReflectionHizPushConstant
	{
		int32_t screen_size[2];
		int32_t pad[2];
	};

	struct ScreenSpaceReflectionSceneData
	{
		float projection[2][16];
		float inv_projection[2][16];
		float reprojection[2][16];
		float eye_offset[2][4];
	};

	struct ScreenSpaceReflectionPushConstant
	{
		int32_t screen_size[2];
		int32_t mipmaps;
		int32_t num_steps;
		float distance_fade;
		float curve_fade_in;
		float depth_tolerance;
		int32_t orthogonal;
		uint32_t view_index;
		int32_t pad[3];
	};

	struct ScreenSpaceReflectionFilterPushConstant
	{
		int32_t screen_size[2];
		uint32_t mip_level;
		int32_t pad;
	};

	struct ScreenSpaceReflectionResolvePushConstant
	{
		int32_t screen_size[2];
		int32_t pad[2];
	};

	struct ScreenSpaceReflection
	{
		ShaderRD downsample_shader{ScreenSpaceReflectionDownsampleShaderRD()};
		RID downsample_shader_version;
		PipelineDeferredRD downsample_pipelines[SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX];

		ShaderRD hiz_shader{ScreenSpaceReflectionHizShaderRD()};
		RID hiz_shader_version;
		PipelineDeferredRD hiz_pipelines[SCREEN_SPACE_REFLECTION_HIZ_MAX];

		ShaderRD ssr_shader{ScreenSpaceReflectionShaderRD()};
		RID ssr_shader_version;
		PipelineDeferredRD ssr_pipeline;
		RID ubo;

		ShaderRD filter_shader{ScreenSpaceReflectionFilterShaderRD()};
		RID filter_shader_version;
		PipelineDeferredRD filter_pipeline;

		ShaderRD resolve_shader{ScreenSpaceReflectionResolveShaderRD()};
		RID resolve_shader_version;
		PipelineDeferredRD resolve_pipeline;
	};

	/* Subsurface scattering */

	enum SSSMode
	{
		SUBSURFACE_SCATTERING_MODE_LOW_QUALITY,
		SUBSURFACE_SCATTERING_MODE_MEDIUM_QUALITY,
		SUBSURFACE_SCATTERING_MODE_HIGH_QUALITY,
		SUBSURFACE_SCATTERING_MODE_MAX
	};

	struct SubSurfaceScatteringPushConstant
	{
		int32_t screen_size[2];
		float camera_z_far;
		float camera_z_near;

		uint32_t vertical;
		uint32_t orthogonal;
		float unit_size;
		float scale;

		float depth_scale;
		uint32_t pad[3];
	};

	struct SubSurfaceScattering
	{
		SubSurfaceScatteringPushConstant push_constant;
		ShaderRD shader{SubsurfaceScatteringShaderRD()};
		RID shader_version;
		PipelineDeferredRD pipelines[SUBSURFACE_SCATTERING_MODE_MAX];
	};

	struct Data
	{
		RSE::EnvironmentSSAOQuality ssao_quality = RSE::ENV_SSAO_QUALITY_MEDIUM;
		bool ssao_half_size = false;
		float ssao_adaptive_target = 0.5f;
		int ssao_blur_passes = 2;
		float ssao_fadeout_from = 50.0f;
		float ssao_fadeout_to = 300.0f;

		RSE::EnvironmentSSILQuality ssil_quality = RSE::ENV_SSIL_QUALITY_MEDIUM;
		bool ssil_half_size = false;
		float ssil_adaptive_target = 0.5f;
		int ssil_blur_passes = 4;
		float ssil_fadeout_from = 50.0f;
		float ssil_fadeout_to = 300.0f;

		bool ssr_half_size = false;

		RSE::SubSurfaceScatteringQuality sss_quality = RSE::SUB_SURFACE_SCATTERING_QUALITY_MEDIUM;
		float sss_scale = 0.05f;
		float sss_depth_scale = 0.01f;

		SSEffectsShader ss_effects;
		SSIL ssil;
		SSAO ssao;
		ScreenSpaceReflection ssr;
		SubSurfaceScattering sss;
	};

	static inline Data* data = nullptr;
};

} // namespace RendererRD

// --- End servers/rendering/renderer_rd/effects/ss_effects.h ---
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer_scene_render.h"

// --- Inlined/Folded from servers/rendering/renderer_rd/effects/upscale_effects.h ---
#include "core/types.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_accumulate_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_autogen_reactive_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_compute_luminance_pyramid_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_depth_clip_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_lock_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_rcas_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_reconstruct_previous_depth_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_tcr_autogen_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr_upscale.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/taa_resolve.glsl.gen.h"

#if defined(METAL_ENABLED) && !defined(VISIONOS_ENABLED)
#define METAL_MFXTEMPORAL_ENABLED
#endif

#ifdef METAL_ENABLED
#include "core/templates/paged_allocator.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device_driver.h"

namespace MTLFX
{
class SpatialScalerBase;
class TemporalScalerBase;
} // namespace MTLFX
#endif

// This flag doesn't actually control anything GCC specific in FSR2. It determines
// if symbols should be exported, which is not required for Godot.
#ifndef FFX_GCC
#define FFX_GCC
#endif

#include <thirdparty/amd-fsr2/ffx_fsr2.h>

#define FSR2_MAX_QUEUED_FRAMES (4)
#define FSR2_MAX_UNIFORM_BUFFERS (4)
#define FSR2_MAX_BUFFERED_DESCRIPTORS (FFX_FSR2_PASS_COUNT * FSR2_MAX_QUEUED_FRAMES)
#define FSR2_UBO_RING_BUFFER_SIZE (FSR2_MAX_BUFFERED_DESCRIPTORS * FSR2_MAX_UNIFORM_BUFFERS)

class RenderSceneBuffersRD;

class SpatialUpscaler
{
public:
	virtual const Span<char> get_label() const = 0;
	virtual void ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) = 0;
	virtual void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_rd_texture,
		RID p_destination_texture) = 0;

	SpatialUpscaler() = default;
	virtual ~SpatialUpscaler() = default;
};

namespace RendererRD
{

class FSR final : public SpatialUpscaler
{
public:
	FSR();
	~FSR();

	virtual const Span<char> get_label() const override { return "FSR 1.0 Upscale"; }

	virtual void ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) override;

	virtual void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_rd_texture,
		RID p_destination_texture) override;

private:
	enum FSRShaderVariant
	{
		FSR_SHADER_VARIANT_NORMAL,
		FSR_SHADER_VARIANT_FALLBACK,
	};

	enum FSRUpscalePass
	{
		FSR_UPSCALE_PASS_EASU = 0,
		FSR_UPSCALE_PASS_RCAS = 1
	};

	struct FSRUpscalePushConstant
	{
		float resolution_width;
		float resolution_height;
		float upscaled_width;
		float upscaled_height;
		float sharpness;
		int pass;
		int pad[2];
	};

	ShaderRD fsr_shader{FsrUpscaleShaderRD()};
	RID shader_version;
	PipelineDeferredRD pipeline;
};

class FSR2Context final
{
public:
	enum ResourceID : uint32_t
	{
		RESOURCE_ID_DYNAMIC = 0xFFFFFFFF
	};

	struct Resources
	{
		LocalVector<RID> rids;
		LocalVector<LocalVector<RID>> mip_slice_rids;
		LocalVector<uint32_t> ids;
		LocalVector<FfxResourceDescription> descriptions;
		LocalVector<uint32_t> dynamic_list;
		LocalVector<uint32_t> free_list;

		uint32_t add(
			RID p_rid, bool p_dynamic, uint32_t p_id, FfxResourceDescription p_description)
		{
			uint32_t ret_index;
			if (free_list.is_empty()) {
				ret_index = rids.size();
				uint32_t new_size = ret_index + 1;
				rids.resize(new_size);
				mip_slice_rids.resize(new_size);
				ids.resize(new_size);
				descriptions.resize(new_size);
			}
			else {
				uint32_t end_index = free_list.size() - 1;
				ret_index = free_list[end_index];
				free_list.resize(end_index);
			}

			rids[ret_index] = p_rid;
			mip_slice_rids[ret_index].clear();
			ids[ret_index] = p_id;
			descriptions[ret_index] = p_description;

			if (p_dynamic) {
				dynamic_list.push_back(ret_index);
			}

			return ret_index;
		}

		void remove(uint32_t p_index)
		{
			DEV_ASSERT(p_index < rids.size());
			free_list.push_back(p_index);
			rids[p_index] = RID();
			mip_slice_rids[p_index].clear();
			ids[p_index] = 0;
			descriptions[p_index] = {};
			dynamic_list.erase(p_index);
		}

		uint32_t size() const { return rids.size(); }
	};

	struct Scratch
	{
		Resources resources;
		LocalVector<FfxGpuJobDescription> gpu_jobs;
		RID ubo_ring_buffer[FSR2_UBO_RING_BUFFER_SIZE];
		uint32_t ubo_ring_buffer_index = 0;
		FfxDevice device = nullptr;
	};

	Scratch scratch;
	FfxFsr2Context fsr_context;
	FfxFsr2ContextDescription fsr_desc;

	~FSR2Context();
};

class FSR2Effect final
{
public:
	struct RootSignature
	{
		// Proxy structure to store the shader required by RD that uses the terminology used by the
		// FSR2 API.
		RID shader_rid;
	};

	struct Pipeline
	{
		RID pipeline_rid;
	};

	struct Pass
	{
		ShaderRD* shader;
		RID shader_version;
		RootSignature root_signature;
		uint32_t shader_variant = 0;
		Pipeline pipeline;
		Vector<FfxResourceBinding> sampled_bindings;
		Vector<FfxResourceBinding> storage_bindings;
		Vector<FfxResourceBinding> uniform_bindings;
	};

	struct Device
	{
		Pass passes[FFX_FSR2_PASS_COUNT];
		FfxDeviceCapabilities capabilities;
		RID point_clamp_sampler;
		RID linear_clamp_sampler;
	};

	struct Parameters
	{
		FSR2Context* context;
		Size2i internal_size;
		RID color;
		RID depth;
		RID velocity;
		RID reactive;
		RID exposure;
		RID output;
		float z_near = 0.0f;
		float z_far = 0.0f;
		float fovy = 0.0f;
		Vector2 jitter;
		float delta_time = 0.0f;
		float sharpness = 0.0f;
		bool reset_accumulation = false;
		Projection reprojection;
	};

	FSR2Effect();
	~FSR2Effect();
	FSR2Context* create_context(Size2i p_internal_size, Size2i p_target_size);
	void upscale(const Parameters& p_params);

private:
	struct
	{
		ShaderRD depth_clip{Fsr2DepthClipPassShaderRD()};
		ShaderRD reconstruct_previous_depth{Fsr2ReconstructPreviousDepthPassShaderRD()};
		ShaderRD lock{Fsr2LockPassShaderRD()};
		ShaderRD accumulate{Fsr2AccumulatePassShaderRD()};
		ShaderRD accumulate_sharpen{Fsr2AccumulatePassShaderRD()};
		ShaderRD rcas{Fsr2RcasPassShaderRD()};
		ShaderRD compute_luminance_pyramid{Fsr2ComputeLuminancePyramidPassShaderRD()};
		ShaderRD autogen_reactive{Fsr2AutogenReactivePassShaderRD()};
		ShaderRD tcr_autogen{Fsr2TcrAutogenPassShaderRD()};
	} shaders;

	Device device;
};

class TAA final
{
public:
	TAA();
	~TAA();

	void process(Ref<RenderSceneBuffersRD> p_render_buffers, RDC::DataFormat p_format,
		float p_z_near, float p_z_far);

private:
	struct TAAResolvePushConstant
	{
		float resolution_width;
		float resolution_height;
		float disocclusion_threshold;
		float variance_dynamic;
	};

	ShaderRD taa_shader{TaaResolveShaderRD()};
	RID shader_version;
	RID pipeline;

	void resolve(RID p_frame, RID p_temp, RID p_depth, RID p_velocity, RID p_prev_velocity,
		RID p_history, Size2 p_resolution, float p_z_near, float p_z_far);
};

#ifdef METAL_ENABLED

struct MFXSpatialContext
{
	MTLFX::SpatialScalerBase* scaler = nullptr;
	MFXSpatialContext() = default;
	~MFXSpatialContext();
};

class MFXSpatialEffect final : public SpatialUpscaler
{
	struct CallbackArgs
	{
		MFXSpatialEffect* owner = nullptr;
		MTLFX::SpatialScalerBase* scaler = nullptr;
		RDD::TextureID src;
		RDD::TextureID dst;

		CallbackArgs(MFXSpatialEffect* p_owner, RDD::TextureID p_src, RDD::TextureID p_dst,
			const MFXSpatialContext& p_ctx)
			: owner(p_owner), scaler(p_ctx.scaler), src(p_src), dst(p_dst)
		{
		}

		static void free(CallbackArgs** p_args)
		{
			(*p_args)->owner->args_allocator.free(*p_args);
			*p_args = nullptr;
		}
	};

	PagedAllocator<CallbackArgs, true, 16> args_allocator;
	static void callback(
		RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata);

public:
	virtual const Span<char> get_label() const override { return "MetalFX Spatial Upscale"; }
	virtual void ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) override;
	virtual void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_src, RID p_dst) override;

	struct CreateParams
	{
		Vector2i input_size;
		Vector2i output_size;
		RDD::DataFormat input_format;
		RDD::DataFormat output_format;
	};

	MFXSpatialContext* create_context(CreateParams p_params) const;

	MFXSpatialEffect();
	~MFXSpatialEffect();
};

#ifdef METAL_MFXTEMPORAL_ENABLED

struct MFXTemporalContext
{
	MTLFX::TemporalScalerBase* scaler = nullptr;
	MFXTemporalContext() = default;
	~MFXTemporalContext();
};

class MFXTemporalEffect final
{
	struct CallbackArgs
	{
		MFXTemporalEffect* owner = nullptr;
		MTLFX::TemporalScalerBase* scaler = nullptr;
		RDD::TextureID src;
		RDD::TextureID depth;
		RDD::TextureID motion;
		RDD::TextureID exposure;
		Vector2 jitter_offset;
		RDD::TextureID dst;
		bool reset = false;

		CallbackArgs(MFXTemporalEffect* p_owner, RDD::TextureID p_src, RDD::TextureID p_depth,
			RDD::TextureID p_motion, RDD::TextureID p_exposure, Vector2 p_jitter_offset,
			RDD::TextureID p_dst, const MFXTemporalContext& p_ctx, bool p_reset)
			: owner(p_owner), scaler(p_ctx.scaler), src(p_src), depth(p_depth), motion(p_motion),
			  exposure(p_exposure), jitter_offset(p_jitter_offset), dst(p_dst), reset(p_reset)
		{
		}

		static void free(CallbackArgs** p_args)
		{
			(*p_args)->owner->args_allocator.free(*p_args);
			*p_args = nullptr;
		}
	};

	PagedAllocator<CallbackArgs, true, 16> args_allocator;

	static void callback(
		RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata);

public:
	MFXTemporalEffect();
	~MFXTemporalEffect();

	struct CreateParams
	{
		Vector2i input_size;
		Vector2i output_size;
		RDD::DataFormat input_format;
		RDD::DataFormat depth_format;
		RDD::DataFormat motion_format;
		RDD::DataFormat reactive_format;
		RDD::DataFormat output_format;
		Vector2 motion_vector_scale;
	};

	MFXTemporalContext* create_context(CreateParams p_params) const;

	struct Params
	{
		RID src;
		RID depth;
		RID motion;
		RID exposure;
		RID dst;
		Vector2 jitter_offset;
		bool reset = false;
	};

	void process(MFXTemporalContext* p_ctx, Params p_params);
};

#endif // METAL_MFXTEMPORAL_ENABLED

#endif // METAL_ENABLED

} // namespace RendererRD

#define RB_SCOPE_BUFFERS SNAME("render_buffers")
#define RB_SCOPE_VRS SNAME("VRS")

#define RB_TEXTURE SNAME("texture")
#define RB_TEX_COLOR SNAME("color")
#define RB_TEX_COLOR_MSAA SNAME("color_msaa")
#define RB_TEX_COLOR_UPSCALED SNAME("color_upscaled")
#define RB_TEX_COLOR_SUBSAMPLED SNAME("color_subsampled")
#define RB_TEX_COLOR_MSAA_SUBSAMPLED SNAME("color_msaa_subsampled")
#define RB_TEX_DEPTH SNAME("depth")
#define RB_TEX_DEPTH_MSAA SNAME("depth_msaa")
#define RB_TEX_DEPTH_SUBSAMPLED SNAME("depth_subsampled")
#define RB_TEX_DEPTH_MSAA_SUBSAMPLED SNAME("depth_msaa_subsampled")
#define RB_TEX_VELOCITY SNAME("velocity")
#define RB_TEX_VELOCITY_MSAA SNAME("velocity_msaa")

#define RB_TEX_BLUR_0 SNAME("blur_0")
#define RB_TEX_BLUR_1 SNAME("blur_1")

#define RB_TEX_BACK_COLOR SNAME("back_color")
#define RB_TEX_BACK_DEPTH SNAME("back_depth")

class RenderSceneBuffersRD : public RenderSceneBuffers
{
private:
	bool can_be_storage = true;
	bool force_hdr = false;
	uint32_t max_cluster_elements = 512;
	RDC::DataFormat preferred_data_format = RDC::DATA_FORMAT_MAX;
	RendererRD::VRS* vrs = nullptr;
	uint64_t auto_exposure_version = 1;
	RSE::ViewportVRSMode vrs_mode = RSE::VIEWPORT_VRS_DISABLED;

	// Our render target represents our final destination that we display on screen.
	RID render_target;
	Size2i target_size = Size2i(0, 0);
	uint32_t view_count = 1;

	// The internal size of the textures we render 3D to in case we render at a lower resolution and
	// upscale
	Size2i internal_size = Size2i(0, 0);
	RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	float fsr_sharpness = 0.2f;
	float texture_mipmap_bias = 0.0f;
	RSE::ViewportAnisotropicFiltering anisotropic_filtering_level = RSE::VIEWPORT_ANISOTROPY_4X;

#ifdef METAL_ENABLED
	RendererRD::MFXSpatialContext* mfx_spatial_context = nullptr;
#endif

	// Aliasing settings
	RSE::ViewportMSAA msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
	RSE::ViewportScreenSpaceAA screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
	bool use_taa = false;
	bool use_debanding = false;
	RDC::TextureSamples texture_samples = RDC::TEXTURE_SAMPLES_1;

	// Named Textures

	struct NTKey
	{
		String context;
		String buffer_name;

		bool operator==(const NTKey& p_val) const
		{
			return (context == p_val.context) && (buffer_name == p_val.buffer_name);
		}

		static uint32_t hash(const NTKey& p_val)
		{
			uint32_t h = p_val.context.hash();
			h = hash_murmur3_one_32(p_val.buffer_name.hash(), h);
			return hash_fmix32(h);
		}

		NTKey() {}

		NTKey(const String& p_context, const String& p_texture_name)
		{
			context = p_context;
			buffer_name = p_texture_name;
		}
	};

	struct NTSliceKey
	{
		uint32_t layer;
		uint32_t layers;
		uint32_t mipmap;
		uint32_t mipmaps;
		RD::TextureView texture_view;

		bool operator==(const NTSliceKey& p_val) const
		{
			return (layer == p_val.layer) && (layers == p_val.layers) && (mipmap == p_val.mipmap) &&
				   (mipmaps == p_val.mipmaps) && (texture_view == p_val.texture_view);
		}

		static uint32_t hash(const NTSliceKey& p_val)
		{
			uint32_t h = hash_murmur3_one_32(p_val.layer);
			h = hash_murmur3_one_32(p_val.layers, h);
			h = hash_murmur3_one_32(p_val.mipmap, h);
			h = hash_murmur3_one_32(p_val.mipmaps, h);
			h = hash_murmur3_one_32(p_val.texture_view.format_override, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_r, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_g, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_b, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_a, h);
			return hash_fmix32(h);
		}

		NTSliceKey() {}

		NTSliceKey(uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps,
			RD::TextureView p_texture_view)
		{
			layer = p_layer;
			layers = p_layers;
			mipmap = p_mipmap;
			mipmaps = p_mipmaps;
			texture_view = p_texture_view;
		}
	};

	struct NamedTexture
	{
		// Cache the data used to create our texture
		RDC::TextureFormat format;
		bool is_unique; // If marked as unique, we return it into our pool

		// Our texture objects, slices are lazy (i.e. only created when requested).
		RID texture;
		mutable HashMap<NTSliceKey, RID, NTSliceKey> slices;
		Vector<Size2i> sizes;
	};

	mutable HashMap<NTKey, NamedTexture, NTKey> named_textures;
	void update_sizes(NamedTexture& p_named_texture);
	void free_named_texture(NamedTexture& p_named_texture);

	struct CustomDataEntry
	{
		Ref<RefCounted> data;
		void (*configure_fn)(RefCounted*, RenderSceneBuffersRD*) = nullptr;
		void (*free_fn)(RefCounted*) = nullptr;
	};

	// Data buffers
	mutable HashMap<String, CustomDataEntry> data_buffers;

	// Samplers.
	RendererRD::Samplers samplers;

	void update_samplers();

protected:

public:
	RenderSceneBuffersRD();
	virtual ~RenderSceneBuffersRD();

	// info from our renderer
	void set_can_be_storage(const bool p_can_be_storage) { can_be_storage = p_can_be_storage; }

	bool get_can_be_storage() const { return can_be_storage; }

	void set_max_cluster_elements(const uint32_t p_max_elements)
	{
		max_cluster_elements = p_max_elements;
	}

	uint32_t get_max_cluster_elements() { return max_cluster_elements; }

	void set_preferred_data_format(const RDC::DataFormat p_preferred_data_format)
	{
		preferred_data_format = p_preferred_data_format;
	}

	RDC::DataFormat get_preferred_data_format() const { return preferred_data_format; }

	RDC::DataFormat get_base_data_format() const
	{
		return force_hdr ? RDC::DATA_FORMAT_R16G16B16A16_SFLOAT : preferred_data_format;
	}

	float get_luminance_multiplier() const;

	void set_vrs(RendererRD::VRS* p_vrs) { vrs = p_vrs; }

	RSE::ViewportVRSMode get_vrs_mode() { return vrs_mode; }

	void cleanup();
	virtual void configure(const RenderSceneBuffersConfiguration* p_config) override;
	void configure_for_reflections(const Size2i p_reflection_size);
	virtual void set_fsr_sharpness(float p_fsr_sharpness) override;
	virtual void set_texture_mipmap_bias(float p_texture_mipmap_bias) override;
	virtual void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level) override;
	virtual void set_use_debanding(bool p_use_debanding) override;

#ifdef METAL_ENABLED
	void ensure_mfx(RendererRD::MFXSpatialEffect* p_effect);

	_FORCE_INLINE_ RendererRD::MFXSpatialContext* get_mfx_spatial_context() const
	{
		return mfx_spatial_context;
	}
#endif

	// Named Textures

	bool has_texture(const String& p_context, const String& p_texture_name) const;
	RID create_texture(const String& p_context, const String& p_texture_name,
		const RDC::DataFormat p_data_format, const uint32_t p_usage_bits,
		const RDC::TextureSamples p_texture_samples = RDC::TEXTURE_SAMPLES_1,
		const Size2i p_size = Size2i(0, 0), const uint32_t p_layers = 0,
		const uint32_t p_mipmaps = 1, bool p_unique = true, bool p_discardable = false);
	RID create_texture_from_format(const String& p_context, const String& p_texture_name,
		const RDC::TextureFormat& p_texture_format, RD::TextureView p_view = RD::TextureView(),
		bool p_unique = true);
	RID create_texture_view(const String& p_context, const String& p_texture_name,
		const String& p_view_name, RD::TextureView p_view = RD::TextureView());
	RID get_texture(const String& p_context, const String& p_texture_name) const;
	const RDC::TextureFormat get_texture_format(
		const String& p_context, const String& p_texture_name) const;
	RID get_texture_slice(const String& p_context, const String& p_texture_name,
		const uint32_t p_layer, const uint32_t p_mipmap, const uint32_t p_layers = 1,
		const uint32_t p_mipmaps = 1);
	RID get_texture_slice_view(const String& p_context, const String& p_texture_name,
		const uint32_t p_layer, const uint32_t p_mipmap, const uint32_t p_layers = 1,
		const uint32_t p_mipmaps = 1, RD::TextureView p_view = RD::TextureView());
	Size2i get_texture_slice_size(
		const String& p_context, const String& p_texture_name, const uint32_t p_mipmap);

	void clear_context(const String& p_context);

	// Allocate shared buffers
	void allocate_blur_textures();

	// Custom data
	bool has_custom_data(const String& p_name) const;

	template <typename T>
	void set_custom_data(const String& p_name, const Ref<T>& p_data)
	{
		static_assert(std::is_base_of_v<RefCounted, T>, "Custom data must inherit RefCounted.");
		if (p_data.is_valid()) {
			data_buffers[p_name] = {
				p_data,
				+[](RefCounted* p_obj, RenderSceneBuffersRD* p_rb) {
					static_cast<T*>(p_obj)->configure(p_rb);
				},
				+[](RefCounted* p_obj) { static_cast<T*>(p_obj)->free_data(); }};
		}
		else if (data_buffers.has(p_name)) {
			data_buffers.erase(p_name);
		}
	}

	Ref<RefCounted> get_custom_data(const String& p_name) const;

	// Getters

	_FORCE_INLINE_ RID get_render_target() const { return render_target; }

	_FORCE_INLINE_ uint32_t get_view_count() const { return view_count; }

	_FORCE_INLINE_ Size2i get_internal_size() const { return internal_size; }

	_FORCE_INLINE_ Size2i get_target_size() const { return target_size; }

	_FORCE_INLINE_ RSE::ViewportScaling3DMode get_scaling_3d_mode() const
	{
		return scaling_3d_mode;
	}

	_FORCE_INLINE_ float get_fsr_sharpness() const { return fsr_sharpness; }

	_FORCE_INLINE_ RSE::ViewportMSAA get_msaa_3d() const { return msaa_3d; }

	_FORCE_INLINE_ RDC::TextureSamples get_texture_samples() const { return texture_samples; }

	_FORCE_INLINE_ RSE::ViewportScreenSpaceAA get_screen_space_aa() const
	{
		return screen_space_aa;
	}

	_FORCE_INLINE_ bool get_use_taa() const { return use_taa; }

	_FORCE_INLINE_ bool get_use_debanding() const { return use_debanding; }

	uint64_t get_auto_exposure_version() const { return auto_exposure_version; }

	void set_auto_exposure_version(const uint64_t p_auto_exposure_version)
	{
		auto_exposure_version = p_auto_exposure_version;
	}

	// For our internal textures we provide some easy access methods.

	_FORCE_INLINE_ bool has_internal_texture() const
	{
		return has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR);
	}

	_FORCE_INLINE_ RID get_internal_texture() const
	{
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR);
	}

	_FORCE_INLINE_ RID get_internal_texture(const uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR, p_layer, 0);
	}

	_FORCE_INLINE_ RID get_internal_texture_reactive(const uint32_t p_layer)
	{
		RD::TextureView alpha_only_view;
		alpha_only_view.swizzle_r = RDC::TEXTURE_SWIZZLE_A;
		alpha_only_view.swizzle_g = RDC::TEXTURE_SWIZZLE_A;
		alpha_only_view.swizzle_b = RDC::TEXTURE_SWIZZLE_A;
		alpha_only_view.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
		return get_texture_slice_view(
			RB_SCOPE_BUFFERS, RB_TEX_COLOR, p_layer, 0, 1, 1, alpha_only_view);
	}

	_FORCE_INLINE_ RID get_color_msaa() const
	{
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA);
	}

	_FORCE_INLINE_ RID get_color_msaa(uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA, p_layer, 0);
	}

	bool has_depth_texture();
	RID get_depth_texture();
	RID get_depth_texture(const uint32_t p_layer);

	RID get_depth_msaa() const { return get_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA); }

	RID get_depth_msaa(uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA, p_layer, 0);
	}

	RID get_color_subsampled();
	RID get_color_msaa_subsampled();
	RID get_depth_subsampled();
	RID get_depth_msaa_subsampled();

	// back buffer (color)
	RID get_back_buffer_texture() const
	{
		// Prefer returning the dedicated backbuffer color texture if it was created. Return the
		// reused blur texture otherwise.
		if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_COLOR)) {
			return get_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_COLOR);
		}
		else if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0)) {
			return get_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0);
		}
		else {
			return RID();
		}
	}

	// Upscaled.
	void ensure_upscaled();

	_FORCE_INLINE_ bool has_upscaled_texture() const
	{
		return has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED);
	}

	_FORCE_INLINE_ RID get_upscaled_texture() const
	{
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED);
	}

	_FORCE_INLINE_ RID get_upscaled_texture(const uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED, p_layer, 0);
	}

	// Velocity, currently only used by TAA (Clustered) but we'll be using this in other places soon
	// too.

	void ensure_velocity();
	bool has_velocity_buffer(bool p_has_msaa);
	RID get_velocity_buffer(bool p_get_msaa);
	RID get_velocity_buffer(bool p_get_msaa, uint32_t p_layer);

	RID get_velocity_depth_buffer();

	// Samplers adjusted with the mipmap bias that is best fit for the configuration of these render
	// buffers.

	_FORCE_INLINE_ RendererRD::Samplers get_samplers() const { return samplers; }

	_FORCE_INLINE_ static RDC::TextureSamples msaa_to_samples(RSE::ViewportMSAA p_msaa)
	{
		switch (p_msaa) {
		case RSE::VIEWPORT_MSAA_DISABLED:
			return RDC::TEXTURE_SAMPLES_1;
		case RSE::VIEWPORT_MSAA_2X:
			return RDC::TEXTURE_SAMPLES_2;
		case RSE::VIEWPORT_MSAA_4X:
			return RDC::TEXTURE_SAMPLES_4;
		case RSE::VIEWPORT_MSAA_8X:
			return RDC::TEXTURE_SAMPLES_8;
		default:
			DEV_ASSERT(false && "Unknown MSAA option.");
			return RDC::TEXTURE_SAMPLES_1;
		}
	}

	static uint32_t get_color_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	static RDC::DataFormat get_depth_format(bool p_resolve, bool p_msaa, bool p_storage);
	static uint32_t get_depth_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	static RDC::DataFormat get_velocity_format();
	static uint32_t get_velocity_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	static RDC::DataFormat get_vrs_format();
	static uint32_t get_vrs_usage_bits();

private:
	RID _create_texture_from_format(const String& p_context, const String& p_texture_name,
		const Ref<RDTextureFormat>& p_texture_format,
		const Ref<RDTextureView>& p_view = Ref<RDTextureView>(), bool p_unique = true);
	RID _create_texture_view(const String& p_context, const String& p_texture_name,
		const String& p_view_name, const Ref<RDTextureView> p_view = Ref<RDTextureView>());
	Ref<RDTextureFormat> _get_texture_format(
		const String& p_context, const String& p_texture_name) const;
	RID _get_texture_slice_view(const String& p_context, const String& p_texture_name,
		const uint32_t p_layer, const uint32_t p_mipmap, const uint32_t p_layers = 1,
		const uint32_t p_mipmaps = 1, const Ref<RDTextureView> p_view = Ref<RDTextureView>());

	// For color and depth as exposed to extensions:
	// - we need separately named functions to access the layer,
	// - we don't output an error for missing buffers but just return an empty RID.
	RID _get_color_texture(bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA)) {
				return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA);
			}
			else {
				return RID();
			}
		}
		else if (has_internal_texture()) {
			return get_internal_texture();
		}
		else {
			return RID();
		}
	}

	RID _get_color_layer(const uint32_t p_layer, bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA)) {
				return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA, p_layer, 0);
			}
			else {
				return RID();
			}
		}
		else if (has_internal_texture()) {
			return get_internal_texture(p_layer);
		}
		else {
			return RID();
		}
	}

	RID _get_depth_texture(bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA)) {
				return get_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA);
			}
			else {
				return RID();
			}
		}
		else if (has_depth_texture()) {
			return get_depth_texture();
		}
		else {
			return RID();
		}
	}

	RID _get_depth_layer(const uint32_t p_layer, bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA)) {
				return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA, p_layer, 0);
			}
			else {
				return RID();
			}
		}
		else if (has_depth_texture()) {
			return get_depth_texture(p_layer);
		}
		else {
			return RID();
		}
	}

	RID _get_velocity_texture(bool p_msaa = false)
	{
		if (has_velocity_buffer(p_msaa)) {
			return get_velocity_buffer(p_msaa);
		}
		else {
			return RID();
		}
	}

	RID _get_velocity_layer(const uint32_t p_layer, bool p_msaa = false)
	{
		if (has_velocity_buffer(p_msaa)) {
			return get_velocity_buffer(p_msaa, p_layer);
		}
		else {
			return RID();
		}
	}

public:
	struct WeightBuffers
	{
		RID weight;
		RID fb; // FB with both texture and weight writing into one level lower
	};

	// 2 full size, 2 half size
	WeightBuffers weight_buffers[4]; // Only used in raster
};

// --- Inlined/Folded from servers/rendering/renderer_rd/storage_rd/light_storage.h ---
#include "core/templates/paged_allocator.h"
#include "core/templates/rid_owner.h"

// --- Folded from servers/rendering/renderer_rd/storage_rd/forward_id_storage.h ---
#include <cstdint>

class RendererSceneRenderRD;

namespace RendererRD
{

typedef int32_t ForwardID;

enum ForwardIDType
{
	FORWARD_ID_TYPE_OMNI_LIGHT,
	FORWARD_ID_TYPE_SPOT_LIGHT,
	FORWARD_ID_TYPE_REFLECTION_PROBE,
	FORWARD_ID_TYPE_DECAL,
	FORWARD_ID_TYPE_AREA_LIGHT,
	FORWARD_ID_MAX,
};

class ForwardIDStorage final
{
private:
	static ForwardID _default_allocate_forward_id(ForwardIDType p_type) { return -1; }
	static void _default_free_forward_id(ForwardIDType p_type, ForwardID p_id) {}
	static void _default_map_forward_id(
		ForwardIDType p_type, ForwardID p_id, uint32_t p_index, uint64_t p_last_pass)
	{
	}
	static bool _default_uses_forward_ids() { return false; }

public:
	inline static ForwardID (*allocate_forward_id)(ForwardIDType p_type) =
		&_default_allocate_forward_id;
	inline static void (*free_forward_id)(ForwardIDType p_type, ForwardID p_id) =
		&_default_free_forward_id;
	inline static void (*map_forward_id)(ForwardIDType p_type, ForwardID p_id, uint32_t p_index,
		uint64_t p_last_pass) = &_default_map_forward_id;
	inline static bool (*uses_forward_ids)() = &_default_uses_forward_ids;

	template <typename Impl> static void bind()
	{
		allocate_forward_id = &Impl::allocate_forward_id;
		free_forward_id = &Impl::free_forward_id;
		map_forward_id = &Impl::map_forward_id;
		uses_forward_ids = &Impl::uses_forward_ids;
	}

	static void reset()
	{
		allocate_forward_id = &_default_allocate_forward_id;
		free_forward_id = &_default_free_forward_id;
		map_forward_id = &_default_map_forward_id;
		uses_forward_ids = &_default_uses_forward_ids;
	}

	ForwardIDStorage() = delete;
	ForwardIDStorage(const ForwardIDStorage&) = delete;
	ForwardIDStorage& operator=(const ForwardIDStorage&) = delete;
	~ForwardIDStorage() = delete;
};

} // namespace RendererRD



// --- End servers/rendering/renderer_rd/storage_rd/forward_id_storage.h ---

#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/storage/utilities.h"

class ClusterBuilderRD;
class ClusterBuilderSharedDataRD;
class RenderDataRD;

namespace RendererRD
{

struct DirectionalShadow
{
	RID depth;
	RID fb;
	int light_count = 0;
	int size = 0;
	bool use_16_bits = true;
	int current_light = 0;
};

class LightStorage
{
public:
	enum ShadowAtlastQuadrant : uint32_t
	{
		QUADRANT_SHIFT = 27,
		OMNI_LIGHT_FLAG = 1 << 26,
		SHADOW_INDEX_MASK = OMNI_LIGHT_FLAG - 1,
		SHADOW_INVALID = 0xFFFFFFFF
	};

	struct Light
	{
		RSE::LightType type;
		float param[RSE::LIGHT_PARAM_MAX];
		Color color = Color(1, 1, 1, 1);
		RID projector;
		bool shadow = false;
		bool negative = false;
		bool reverse_cull = false;
		RSE::LightBakeMode bake_mode = RSE::LIGHT_BAKE_DYNAMIC;
		uint32_t max_sdfgi_cascade = 2;
		uint32_t cull_mask = 0xFFFFFFFF;
		uint32_t shadow_caster_mask = 0xFFFFFFFF;
		bool distance_fade = false;
		real_t distance_fade_begin = 40.0;
		real_t distance_fade_shadow = 50.0;
		real_t distance_fade_length = 10.0;
		RSE::LightOmniShadowMode omni_shadow_mode = RSE::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID;
		RSE::LightDirectionalShadowMode directional_shadow_mode =
			RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL;
		bool directional_blend_splits = false;
		RSE::LightDirectionalSkyMode directional_sky_mode =
			RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_AND_SKY;
		Vector2 area_size = Vector2(1, 1);
		bool area_normalize_energy = true;
		RID area_texture;
		uint64_t version = 0;

		Dependency dependency;
	};

	struct LightInstance
	{
		struct ShadowTransform
		{
			Projection camera;
			Transform3D transform;
			float farplane = 0.0;
			float split = 0.0;
			float bias_scale = 0.0;
			float shadow_texel_size = 0.0;
			float range_begin = 0.0;
			Rect2 atlas_rect;
			Vector2 uv_scale;
		};

		RSE::LightType light_type = RSE::LIGHT_DIRECTIONAL;
		ShadowTransform shadow_transform[6];

		AABB aabb;
		RID self;
		RID light;
		Transform3D transform;

		Vector3 light_vector;
		Vector3 spot_vector;
		float linear_att = 0.0;

		uint64_t shadow_pass = 0;
		uint64_t last_scene_pass = 0;
		uint64_t last_scene_shadow_pass = 0;
		uint64_t last_pass = 0;
		uint32_t cull_mask = 0;
		uint32_t light_directional_index = 0;

		Rect2 directional_rect;
		HashSet<RID> shadow_atlases;
		ForwardID forward_id = -1;

		LightInstance() {}
	};

	struct LightData
	{
		float position[3];
		float inv_radius;
		float direction[3];
		float size;
		float color[3];
		float attenuation;
		float area_width[3];
		float inv_spot_attenuation;
		float area_height[3];
		float cos_spot_angle;
		float specular_amount;
		float shadow_opacity;
		float pad[2];
		float atlas_rect[4];
		float shadow_matrix[16];
		float shadow_bias;
		float shadow_normal_bias;
		float transmittance_bias;
		float soft_shadow_size;
		float soft_shadow_scale;
		uint32_t mask;
		float volumetric_fog_energy;
		uint32_t bake_mode;
		float projector_rect[4];
	};

	struct LightInstanceDepthSort
	{
		float depth;
		LightInstance* light_instance;
		Light* light;

		bool operator<(const LightInstanceDepthSort& p_sort) const { return depth < p_sort.depth; }
	};

	struct DirectionalLightData
	{
		float direction[3];
		float energy;
		float color[3];
		float size;
		float specular;
		uint32_t mask;
		float softshadow_angle;
		float soft_shadow_scale;
		uint32_t blend_splits;
		float shadow_opacity;
		float fade_from;
		float fade_to;
		uint32_t pad[2];
		uint32_t bake_mode;
		float volumetric_fog_energy;
		float shadow_bias[4];
		float shadow_normal_bias[4];
		float shadow_transmittance_bias[4];
		float shadow_z_range[4];
		float shadow_range_begin[4];
		float shadow_split_offsets[4];
		float shadow_matrices[4][16];
		float uv_scale1[2];
		float uv_scale2[2];
		float uv_scale3[2];
		float uv_scale4[2];
	};

	struct ReflectionProbe
	{
		RSE::ReflectionProbeUpdateMode update_mode = RSE::REFLECTION_PROBE_UPDATE_ONCE;
		float intensity = 1.0;
		float blend_distance = 1.0;
		RSE::ReflectionProbeAmbientMode ambient_mode = RSE::REFLECTION_PROBE_AMBIENT_ENVIRONMENT;
		Color ambient_color;
		float ambient_color_energy = 1.0;
		float max_distance = 0;
		Vector3 size = Vector3(20, 20, 20);
		Vector3 origin_offset;
		bool interior = false;
		bool box_projection = false;
		bool enable_shadows = false;
		uint32_t cull_mask = (1 << 20) - 1;
		uint32_t reflection_mask = (1 << 20) - 1;
		float mesh_lod_threshold = 0.01;
		float baked_exposure = 1.0;

		Dependency dependency;
	};

	struct ReflectionAtlas
	{
		int count = 0;
		int size = 0;
		int reflection_texture_size = 0;
		float uv_border_size = 0.0;
		bool update_always = false;

		RID reflection;
		RID color_buffer;
		RID color_views[6];
		RID color_fbs[6];
		RID depth_buffer;
		RID depth_fb;

		struct Reflection
		{
			RID owner;
			RendererRD::SkyRD::ReflectionData data;
		};

		Vector<Reflection> reflections;
		Ref<RenderSceneBuffersRD> render_buffers;
		ClusterBuilderRD* cluster_builder = nullptr;
	};

	struct ReflectionProbeInstance
	{
		RID probe;
		int atlas_index = -1;
		RID atlas;
		bool dirty = true;
		bool rendering = false;
		int processing_layer = 1;
		uint64_t last_pass = 0;
		uint32_t cull_mask = 0;
		RendererRD::ForwardID forward_id = -1;
		Transform3D transform;
	};

	enum
	{
		REFLECTION_AMBIENT_DISABLED = 0,
		REFLECTION_AMBIENT_ENVIRONMENT = 1,
		REFLECTION_AMBIENT_COLOR = 2,
	};

	struct ReflectionData
	{
		float box_extents[3];
		float index;
		float box_offset[3];
		uint32_t mask;
		float ambient[3];
		float intensity;
		float blend_distance;
		uint32_t exterior;
		uint32_t box_project;
		uint32_t ambient_mode;
		float exposure_normalization;
		uint32_t pad0;
		uint32_t pad1;
		uint32_t pad2;
		float local_matrix[16];
	};

	struct ReflectionProbeInstanceSort
	{
		float size;
		ReflectionProbeInstance* probe_instance;

		bool operator<(const ReflectionProbeInstanceSort& p_sort) const
		{
			return size < p_sort.size;
		}
	};

	struct Lightmap
	{
		RID light_texture;
		RID shadow_texture;
		RSE::ShadowmaskMode shadowmask_mode = RSE::SHADOWMASK_MODE_NONE;
		bool uses_spherical_harmonics = false;
		bool interior = false;
		AABB bounds = AABB(Vector3(), Vector3(1, 1, 1));
		float baked_exposure = 1.0;
		Vector2i light_texture_size;
		int32_t array_index = -1;
		PackedVector3Array points;
		PackedColorArray point_sh;
		PackedInt32Array tetrahedra;
		PackedInt32Array bsp_tree;

		struct BSP
		{
			static const int32_t EMPTY_LEAF = INT32_MIN;
			float plane[4];
			int32_t over = EMPTY_LEAF, under = EMPTY_LEAF;
		};

		Dependency dependency;
	};

	struct LightmapInstance
	{
		RID lightmap;
		Transform3D transform;
	};

	struct ShadowShrinkStage
	{
		RID texture;
		RID filter_texture;
		uint32_t size = 0;
	};

	struct ShadowAtlas
	{
		struct Quadrant
		{
			uint32_t subdivision = 0;

			struct Shadow
			{
				RID owner;
				uint64_t version = 0;
				uint64_t fog_version = 0;
				uint64_t alloc_tick = 0;

				Shadow() {}
			};

			Vector<Shadow> shadows;

			Quadrant() {}
		} quadrants[4];

		int size_order[4] = {0, 1, 2, 3};
		uint32_t smallest_subdiv = 0;
		int size = 0;
		bool use_16_bits = true;
		RID depth;
		RID fb;
		HashMap<RID, uint32_t> shadow_owners;
	};

	struct ShadowCubemap
	{
		RID cubemap;
		RID side_fb[6];
	};

	/* Static Storage State */
	static inline uint32_t max_cluster_elements = 512;
	static inline RID_Owner<Light, true> light_owner;
	static inline RID_Owner<LightInstance> light_instance_owner;

	static inline uint32_t max_lights = 0;
	static inline uint32_t omni_light_count = 0;
	static inline uint32_t spot_light_count = 0;
	static inline uint32_t area_light_count = 0;
	static inline LightData* omni_lights = nullptr;
	static inline LightData* spot_lights = nullptr;
	static inline LightData* area_lights = nullptr;
	static inline LightInstanceDepthSort* omni_light_sort = nullptr;
	static inline LightInstanceDepthSort* spot_light_sort = nullptr;
	static inline LightInstanceDepthSort* area_light_sort = nullptr;
	static inline RID omni_light_buffer;
	static inline RID spot_light_buffer;
	static inline RID area_light_buffer;

	static inline uint32_t max_directional_lights = 0;
	static inline DirectionalLightData* directional_lights = nullptr;
	static inline RID directional_light_buffer;

	static inline RID_Owner<ReflectionProbe, true> reflection_probe_owner;
	static inline RID_Owner<ReflectionAtlas> reflection_atlas_owner;
	static inline RID_Owner<ReflectionProbeInstance> reflection_probe_instance_owner;

	static inline uint32_t max_reflections = 0;
	static inline uint32_t reflection_count = 0;
	static inline ReflectionData* reflections = nullptr;
	static inline ReflectionProbeInstanceSort* reflection_sort = nullptr;
	static inline RID reflection_buffer;

	static inline bool using_lightmap_array = false;
	static inline Vector<RID> lightmap_textures;
	static inline uint64_t lightmap_array_version = 0;
	static inline float lightmap_probe_capture_update_speed = 4;
	static inline RID_Owner<Lightmap, true> lightmap_owner;
	static inline Vector<RID> shadowmask_textures;
	static inline RID_Owner<LightmapInstance> lightmap_instance_owner;

	static inline uint64_t shadow_atlas_realloc_tolerance_msec = 500;
	static inline RID_Owner<ShadowAtlas> shadow_atlas_owner;
	static inline DirectionalShadow directional_shadow;
	static inline HashMap<int, ShadowCubemap> shadow_cubemaps;

	static inline bool shadow_cubemaps_used = false;
	static inline bool shadow_dual_paraboloid_used = false;

	/* Internal static helper functions */
	static ForwardIDType _light_type_to_forward_id_type(RSE::LightType p_type);
	static void _reflection_atlas_clear(ReflectionAtlas* p_reflection_atlas);
	static void _update_shadow_atlas(ShadowAtlas* shadow_atlas);
	static void _shadow_atlas_invalidate_shadow(ShadowAtlas::Quadrant::Shadow* p_shadow,
		RID p_atlas, ShadowAtlas* p_shadow_atlas, uint32_t p_quadrant, uint32_t p_shadow_idx);
	static bool _shadow_atlas_find_shadow(ShadowAtlas* shadow_atlas, int* p_in_quadrants,
		int p_quadrant_count, int p_current_subdiv, uint64_t p_tick, int& r_quadrant,
		int& r_shadow);
	static bool _shadow_atlas_find_omni_shadows(ShadowAtlas* shadow_atlas, int* p_in_quadrants,
		int p_quadrant_count, int p_current_subdiv, uint64_t p_tick, int& r_quadrant,
		int& r_shadow);
	static ShadowCubemap* _get_shadow_cubemap(int p_size);

public:
	LightStorage() = delete;
	LightStorage(const LightStorage&) = delete;
	LightStorage& operator=(const LightStorage&) = delete;
	~LightStorage() = delete;

	static void initialize();
    static void finalize();
	static bool free(RID p_rid);

	static void set_max_cluster_elements(const uint32_t p_max_cluster_elements)
	{
		max_cluster_elements = p_max_cluster_elements;
		set_max_reflection_probes(p_max_cluster_elements);
		set_max_lights(p_max_cluster_elements);
	}

	static uint32_t get_max_cluster_elements() { return max_cluster_elements; }

	/* Light API */
	static Light* get_light(RID p_rid) { return light_owner.get_or_null(p_rid); }

	static bool owns_light(RID p_rid) { return light_owner.owns(p_rid); }

	static void _light_initialize(RID p_rid, RSE::LightType p_type);

	static RID directional_light_allocate();
	static void directional_light_initialize(RID p_light);
	static RID omni_light_allocate();
	static void omni_light_initialize(RID p_light);
	static RID spot_light_allocate();
	static void spot_light_initialize(RID p_light);
	static RID area_light_allocate();
	static void area_light_initialize(RID p_light);

	static void light_free(RID p_rid);
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
	static uint32_t light_get_shadow_caster_mask(RID p_light);
	static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode);
	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade);

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode);
	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode);
	static void light_directional_set_blend_splits(RID p_light, bool p_enable);
	static bool light_directional_get_blend_splits(RID p_light);
	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode);
	static RSE::LightDirectionalSkyMode light_directional_get_sky_mode(RID p_light);

	static RSE::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light);
	static RSE::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light);
	static void light_area_set_size(RID p_light, const Vector2& p_size);
	static Vector2 light_area_get_size(RID p_light);
	static void light_area_set_normalize_energy(RID p_light, bool p_enabled);
	static bool light_area_get_normalize_energy(RID p_light);
	static void light_area_set_texture(RID p_light, RID p_texture);
	static RID light_area_get_texture(RID p_light);

	static RSE::LightType light_get_type(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, RSE::LIGHT_DIRECTIONAL);
		return light->type;
	}

	static AABB light_get_aabb(RID p_light);

	static float light_get_param(RID p_light, RSE::LightParam p_param)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, 0);
		return light->param[p_param];
	}

	_FORCE_INLINE_ static RID light_get_projector(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, RID());
		return light->projector;
	}

	static Color light_get_color(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, Color());
		return light->color;
	}

	_FORCE_INLINE_ static bool light_is_distance_fade_enabled(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		return light ? light->distance_fade : false;
	}

	_FORCE_INLINE_ static float light_get_distance_fade_begin(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		return light ? light->distance_fade_begin : 0.0f;
	}

	_FORCE_INLINE_ static float light_get_distance_fade_shadow(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		return light ? light->distance_fade_shadow : 0.0f;
	}

	_FORCE_INLINE_ static float light_get_distance_fade_length(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		return light ? light->distance_fade_length : 0.0f;
	}

	static bool light_has_shadow(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, false);
		return light->shadow;
	}

	static bool light_has_projector(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, false);
		return TextureStorage::owns_texture(light->projector);
	}

	_FORCE_INLINE_ static bool light_is_negative(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, false);
		return light->negative;
	}

	_FORCE_INLINE_ static float light_get_transmittance_bias(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, 0.0);
		return light->param[RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS];
	}

	static bool light_get_reverse_cull_face_mode(RID p_light)
	{
		const Light* light = light_owner.get_or_null(p_light);
		ERR_FAIL_NULL_V(light, false);
		return light->reverse_cull;
	}

	static RSE::LightBakeMode light_get_bake_mode(RID p_light);
	static uint32_t light_get_max_sdfgi_cascade(RID p_light);
	static uint64_t light_get_version(RID p_light);
	static uint32_t light_get_cull_mask(RID p_light);
	static Dependency* light_get_dependency(RID p_light);

	/* Light Instance API */
	static LightInstance* get_light_instance(RID p_rid)
	{
		return light_instance_owner.get_or_null(p_rid);
	}

	static bool owns_light_instance(RID p_rid) { return light_instance_owner.owns(p_rid); }

	static RID light_instance_create(RID p_light);
	static void light_instance_free(RID p_light);
	static void light_instance_set_transform(RID p_light_instance, const Transform3D& p_transform);
	static void light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb);
	static void light_instance_set_shadow_transform(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale = 1.0, float p_range_begin = 0,
		const Vector2& p_uv_scale = Vector2());
	static void light_instance_mark_visible(RID p_light_instance);

	static bool light_instance_is_shadow_visible_at_position(
		RID p_light_instance, const Vector3& p_position)
	{
		const LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
		ERR_FAIL_NULL_V(light_instance, false);
		const Light* light = light_owner.get_or_null(light_instance->light);
		ERR_FAIL_NULL_V(light, false);
		if (!light->shadow) {
			return false;
		}
		if (!light->distance_fade) {
			return true;
		}
		real_t distance = p_position.distance_to(light_instance->transform.origin);
		if (distance > light->distance_fade_shadow + light->distance_fade_length) {
			return false;
		}
		return true;
	}

	_FORCE_INLINE_ static RID light_instance_get_base_light(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->light : RID();
	}

	_FORCE_INLINE_ static Transform3D light_instance_get_base_transform(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->transform : Transform3D();
	}

	_FORCE_INLINE_ static AABB light_instance_get_base_aabb(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->aabb : AABB();
	}

	_FORCE_INLINE_ static void light_instance_set_cull_mask(
		RID p_light_instance, uint32_t p_cull_mask)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		if (li) {
			li->cull_mask = p_cull_mask;
		}
	}

	_FORCE_INLINE_ static uint32_t light_instance_get_cull_mask(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->cull_mask : 0;
	}

	_FORCE_INLINE_ static Rect2 light_instance_get_shadow_atlas_rect(
		RID p_light_instance, RID p_shadow_atlas, Vector2i& r_omni_offset)
	{
		ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_shadow_atlas);
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		ERR_FAIL_NULL_V(shadow_atlas, Rect2());
		ERR_FAIL_NULL_V(li, Rect2());
		uint32_t key = shadow_atlas->shadow_owners[li->self];

		uint32_t quadrant = (key >> QUADRANT_SHIFT) & 0x3;
		uint32_t shadow = key & SHADOW_INDEX_MASK;
		ERR_FAIL_COND_V(
			shadow >= (uint32_t)shadow_atlas->quadrants[quadrant].shadows.size(), Rect2());

		uint32_t atlas_size = shadow_atlas->size;
		uint32_t quadrant_size = atlas_size >> 1;
		uint32_t x = (quadrant & 1) * quadrant_size;
		uint32_t y = (quadrant >> 1) * quadrant_size;

		uint32_t shadow_size = (quadrant_size / shadow_atlas->quadrants[quadrant].subdivision);
		x += (shadow % shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;
		y += (shadow / shadow_atlas->quadrants[quadrant].subdivision) * shadow_size;

		if (key & OMNI_LIGHT_FLAG) {
			if (((shadow + 1) % shadow_atlas->quadrants[quadrant].subdivision) == 0) {
				r_omni_offset.x = 1 - int(shadow_atlas->quadrants[quadrant].subdivision);
				r_omni_offset.y = 1;
			}
			else {
				r_omni_offset.x = 1;
				r_omni_offset.y = 0;
			}
		}
		uint32_t width = shadow_size;
		uint32_t height = shadow_size;
		return Rect2(x / float(shadow_atlas->size), y / float(shadow_atlas->size),
			width / float(shadow_atlas->size), height / float(shadow_atlas->size));
	}

	_FORCE_INLINE_ static float light_instance_get_shadow_texel_size(
		RID p_light_instance, RID p_shadow_atlas)
	{
		ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_shadow_atlas);
		ERR_FAIL_NULL_V(shadow_atlas, 0);
		uint32_t key = shadow_atlas->shadow_owners[p_light_instance];
		uint32_t quadrant = (key >> QUADRANT_SHIFT) & 0x3;
		uint32_t quadrant_size = shadow_atlas->size >> 1;
		uint32_t shadow_size = (quadrant_size / shadow_atlas->quadrants[quadrant].subdivision);
		return float(1.0) / shadow_size;
	}

	_FORCE_INLINE_ static Projection light_instance_get_shadow_camera(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].camera : Projection();
	}

	_FORCE_INLINE_ static Transform3D light_instance_get_shadow_transform(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].transform : Transform3D();
	}

	_FORCE_INLINE_ static float light_instance_get_shadow_bias_scale(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].bias_scale : 0.0f;
	}

	_FORCE_INLINE_ static float light_instance_get_shadow_range(RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].farplane : 0.0f;
	}

	_FORCE_INLINE_ static float light_instance_get_shadow_range_begin(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].range_begin : 0.0f;
	}

	_FORCE_INLINE_ static Vector2 light_instance_get_shadow_uv_scale(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].uv_scale : Vector2();
	}

	_FORCE_INLINE_ static void light_instance_set_directional_shadow_atlas_rect(
		RID p_light_instance, int p_index, const Rect2 p_atlas_rect)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		if (li) {
			li->shadow_transform[p_index].atlas_rect = p_atlas_rect;
		}
	}

	_FORCE_INLINE_ static Rect2 light_instance_get_directional_shadow_atlas_rect(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].atlas_rect : Rect2();
	}

	_FORCE_INLINE_ static float light_instance_get_directional_shadow_split(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].split : 0.0f;
	}

	_FORCE_INLINE_ static float light_instance_get_directional_shadow_texel_size(
		RID p_light_instance, int p_index)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_transform[p_index].shadow_texel_size : 0.0f;
	}

	_FORCE_INLINE_ static void light_instance_set_render_pass(RID p_light_instance, uint64_t p_pass)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		if (li) {
			li->last_pass = p_pass;
		}
	}

	_FORCE_INLINE_ static uint64_t light_instance_get_render_pass(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->last_pass : 0;
	}

	_FORCE_INLINE_ static void light_instance_set_shadow_pass(RID p_light_instance, uint64_t p_pass)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		if (li) {
			li->last_scene_shadow_pass = p_pass;
		}
	}

	_FORCE_INLINE_ static uint64_t light_instance_get_shadow_pass(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->last_scene_shadow_pass : 0;
	}

	_FORCE_INLINE_ static ForwardID light_instance_get_forward_id(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->forward_id : -1;
	}

	_FORCE_INLINE_ static RSE::LightType light_instance_get_type(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->light_type : RSE::LIGHT_DIRECTIONAL;
	}

	_FORCE_INLINE_ static void light_instance_set_directional_rect(
		RID p_light_instance, const Rect2& p_directional_rect)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		if (li) {
			li->directional_rect = p_directional_rect;
		}
	}

	_FORCE_INLINE_ static Rect2 light_instance_get_directional_rect(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->directional_rect : Rect2();
	}

	/* Light Data */
	static void free_light_data();
	static void set_max_lights(const uint32_t p_max_lights);

	static RID get_omni_light_buffer() { return omni_light_buffer; }

	static RID get_spot_light_buffer() { return spot_light_buffer; }

	static RID get_area_light_buffer() { return area_light_buffer; }

	static RID get_directional_light_buffer() { return directional_light_buffer; }

	static uint32_t get_max_directional_lights() { return max_directional_lights; }

	static uint32_t get_directional_light_blend_splits(uint32_t p_directional_light_count);
	static bool has_directional_shadows(const uint32_t p_directional_light_count);
	static void update_light_buffers(RenderDataRD* p_render_data, const PagedArray<RID>& p_lights,
		const Transform3D& p_camera_transform, RID p_shadow_atlas, bool p_using_shadows,
		uint32_t& r_directional_light_count, uint32_t& r_positional_light_count,
		bool& r_directional_light_soft_shadows);

	/* Reflection Probe API */
	static ReflectionProbe* get_reflection_probe(RID p_rid)
	{
		return reflection_probe_owner.get_or_null(p_rid);
	}

	static bool owns_reflection_probe(RID p_rid) { return reflection_probe_owner.owns(p_rid); }

	static RID reflection_probe_allocate();
	static void reflection_probe_initialize(RID p_reflection_probe);
	static void reflection_probe_free(RID p_rid);

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
	static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio);
	static void reflection_probe_set_baked_exposure(RID p_probe, float p_exposure);

	static AABB reflection_probe_get_aabb(RID p_probe);
	static RSE::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe);
	static uint32_t reflection_probe_get_cull_mask(RID p_probe);
	static uint32_t reflection_probe_get_reflection_mask(RID p_probe);
	static Vector3 reflection_probe_get_size(RID p_probe);
	static Vector3 reflection_probe_get_origin_offset(RID p_probe);
	static float reflection_probe_get_origin_max_distance(RID p_probe);
	static float reflection_probe_get_mesh_lod_threshold(RID p_probe);
	static float reflection_probe_get_baked_exposure(RID p_probe);
	static bool reflection_probe_renders_shadows(RID p_probe);

	static float reflection_probe_get_intensity(RID p_probe);
	static float reflection_probe_get_blend_distance(RID p_probe);
	static bool reflection_probe_is_interior(RID p_probe);
	static bool reflection_probe_is_box_projection(RID p_probe);
	static RSE::ReflectionProbeAmbientMode reflection_probe_get_ambient_mode(RID p_probe);
	static Color reflection_probe_get_ambient_color(RID p_probe);
	static float reflection_probe_get_ambient_color_energy(RID p_probe);
	static Dependency* reflection_probe_get_dependency(RID p_probe);

	/* Reflection Atlas API */
	static bool owns_reflection_atlas(RID p_rid) { return reflection_atlas_owner.owns(p_rid); }

	static RID reflection_atlas_create();
	static void reflection_atlas_free(RID p_ref_atlas);
	static void reflection_atlas_set_size(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count);
	static int reflection_atlas_get_size(RID p_ref_atlas);

	_FORCE_INLINE_ static RID reflection_atlas_get_texture(RID p_ref_atlas)
	{
		ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_ref_atlas);
		return atlas ? atlas->reflection : RID();
	}

	_FORCE_INLINE_ static int reflection_atlas_get_texture_size(RID p_ref_atlas)
	{
		ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_ref_atlas);
		return atlas ? atlas->reflection_texture_size : 0;
	}

	_FORCE_INLINE_ static float reflection_atlas_get_border_size(RID p_ref_atlas)
	{
		ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_ref_atlas);
		return atlas ? atlas->uv_border_size : 0.0f;
	}

	/* Reflection Probe Instance API */
	static ReflectionProbeInstance* get_reflection_probe_instance(RID p_rid)
	{
		return reflection_probe_instance_owner.get_or_null(p_rid);
	}

	static bool owns_reflection_probe_instance(RID p_rid)
	{
		return reflection_probe_instance_owner.owns(p_rid);
	}

	static RID reflection_probe_instance_create(RID p_probe);
	static void reflection_probe_instance_free(RID p_instance);
	static void reflection_probe_instance_set_transform(
		RID p_instance, const Transform3D& p_transform);
	static bool reflection_probe_has_atlas_index(RID p_instance);
	static void reflection_probe_release_atlas_index(RID p_instance);
	static bool reflection_probe_instance_needs_redraw(RID p_instance);
	static bool reflection_probe_instance_has_reflection(RID p_instance);
	static bool reflection_probe_instance_begin_render(RID p_instance, RID p_reflection_atlas);
	static bool reflection_probe_instance_end_render(RID p_instance, RID p_reflection_atlas);
	static Ref<RenderSceneBuffers> reflection_probe_atlas_get_render_buffers(
		RID p_reflection_atlas);
	static bool reflection_probe_instance_postprocess_step(RID p_instance);

	static uint32_t reflection_probe_instance_get_resolution(RID p_instance);
	static RID reflection_probe_instance_get_framebuffer(RID p_instance, int p_index);
	static RID reflection_probe_instance_get_depth_framebuffer(RID p_instance, int p_index);

	_FORCE_INLINE_ static RID reflection_probe_instance_get_probe(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->probe : RID();
	}

	_FORCE_INLINE_ static ForwardID reflection_probe_instance_get_forward_id(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->forward_id : 0;
	}

	_FORCE_INLINE_ static void reflection_probe_instance_set_cull_mask(
		RID p_instance, uint32_t p_render_pass)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		if (rpi) {
			rpi->cull_mask = p_render_pass;
		}
	}

	_FORCE_INLINE_ static void reflection_probe_instance_set_render_pass(
		RID p_instance, uint32_t p_render_pass)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		if (rpi) {
			rpi->last_pass = p_render_pass;
		}
	}

	_FORCE_INLINE_ static uint32_t reflection_probe_instance_get_render_pass(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->last_pass : 0;
	}

	_FORCE_INLINE_ static Transform3D reflection_probe_instance_get_transform(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->transform : Transform3D();
	}

	_FORCE_INLINE_ static int reflection_probe_instance_get_atlas_index(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->atlas_index : -1;
	}

	static ClusterBuilderRD* reflection_probe_instance_get_cluster_builder(
		RID p_instance, ClusterBuilderSharedDataRD* p_cluster_builder_shared);

	/* Reflection Data Helpers */
	static void free_reflection_data();
	static void set_max_reflection_probes(const uint32_t p_max_reflection_probes);

	static RID get_reflection_probe_buffer() { return reflection_buffer; }

	static void update_reflection_probe_buffer(RenderDataRD* p_render_data,
		const PagedArray<RID>& p_reflections, const Transform3D& p_camera_inverse_transform,
		RID p_environment);
	static RDC::DataFormat get_reflection_probe_color_format();
	static uint32_t get_reflection_probe_color_usage_bits(bool p_storage);
	static RDC::DataFormat get_reflection_probe_depth_format();
	static uint32_t get_reflection_probe_depth_usage_bits();

	/* Lightmap API */
	static Lightmap* get_lightmap(RID p_rid) { return lightmap_owner.get_or_null(p_rid); }

	static bool owns_lightmap(RID p_rid) { return lightmap_owner.owns(p_rid); }

	static RID lightmap_allocate();
	static void lightmap_initialize(RID p_lightmap);
	static void lightmap_free(RID p_rid);

	static void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics);
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
	static AABB lightmap_get_aabb(RID p_lightmap);
	static bool lightmap_is_interior(RID p_lightmap);
	static void lightmap_tap_sh_light(RID p_lightmap, const Vector3& p_point, Color* r_sh);
	static void lightmap_set_probe_capture_update_speed(float p_speed);
	static Dependency* lightmap_get_dependency(RID p_lightmap);

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow);
	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap);
	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode);

	static float lightmap_get_probe_capture_update_speed()
	{
		return lightmap_probe_capture_update_speed;
	}

	_FORCE_INLINE_ static RID lightmap_get_texture(RID p_lightmap)
	{
		const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
		return lm ? lm->light_texture : RID();
	}

	_FORCE_INLINE_ static float lightmap_get_baked_exposure_normalization(RID p_lightmap)
	{
		const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
		return lm ? lm->baked_exposure : 1.0f;
	}

	_FORCE_INLINE_ static int32_t lightmap_get_array_index(RID p_lightmap)
	{
		ERR_FAIL_COND_V(!using_lightmap_array, -1);
		const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
		return lm ? lm->array_index : -1;
	}

	_FORCE_INLINE_ static bool lightmap_uses_spherical_harmonics(RID p_lightmap)
	{
		ERR_FAIL_COND_V(!using_lightmap_array, false);
		const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
		return lm ? lm->uses_spherical_harmonics : false;
	}

	_FORCE_INLINE_ static Vector2i lightmap_get_light_texture_size(RID p_lightmap)
	{
		const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
		return lm ? lm->light_texture_size : Vector2i();
	}

	_FORCE_INLINE_ static uint64_t lightmap_array_get_version()
	{
		ERR_FAIL_COND_V(!using_lightmap_array, 0);
		return lightmap_array_version;
	}

	_FORCE_INLINE_ static int lightmap_array_get_size()
	{
		ERR_FAIL_COND_V(!using_lightmap_array, 0);
		return lightmap_textures.size();
	}

	_FORCE_INLINE_ static const Vector<RID>& lightmap_array_get_textures()
	{
		ERR_FAIL_COND_V(!using_lightmap_array, lightmap_textures);
		return lightmap_textures;
	}

	_FORCE_INLINE_ static RID shadowmask_get_texture(RID p_lightmap)
	{
		const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
		return lm ? lm->shadow_texture : RID();
	}

	/* Lightmap Instance API */
	static LightmapInstance* get_lightmap_instance(RID p_rid)
	{
		return lightmap_instance_owner.get_or_null(p_rid);
	}

	static bool owns_lightmap_instance(RID p_rid) { return lightmap_instance_owner.owns(p_rid); }

	static RID lightmap_instance_create(RID p_lightmap);
	static void lightmap_instance_free(RID p_lightmap);
	static void lightmap_instance_set_transform(RID p_lightmap, const Transform3D& p_transform);

	_FORCE_INLINE_ static bool lightmap_instance_is_valid(RID p_lightmap_instance)
	{
		return lightmap_instance_owner.get_or_null(p_lightmap_instance) != nullptr;
	}

	_FORCE_INLINE_ static RID lightmap_instance_get_lightmap(RID p_lightmap_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_lightmap_instance);
		return li ? li->light : RID();
	}

	_FORCE_INLINE_ static Transform3D lightmap_instance_get_transform(RID p_lightmap_instance)
	{
		LightmapInstance* li = lightmap_instance_owner.get_or_null(p_lightmap_instance);
		return li ? li->transform : Transform3D();
	}

	/* Shadow Atlas API */
	static bool owns_shadow_atlas(RID p_rid) { return shadow_atlas_owner.owns(p_rid); }

	static RID shadow_atlas_create();
	static void shadow_atlas_free(RID p_atlas);
	static void shadow_atlas_set_size(RID p_atlas, int p_size, bool p_16_bits = true);
	static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision);
	static bool shadow_atlas_update_light(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version);

	_FORCE_INLINE_ static bool shadow_atlas_owns_light_instance(RID p_atlas, RID p_light_instance)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, false);
		return atlas->shadow_owners.has(p_light_instance);
	}

	_FORCE_INLINE_ static uint32_t shadow_atlas_get_light_instance_key(
		RID p_atlas, RID p_light_instance)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, -1);
		return atlas->shadow_owners[p_light_instance];
	}

	_FORCE_INLINE_ static RID shadow_atlas_get_texture(RID p_atlas)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		return atlas ? atlas->depth : RID();
	}

	_FORCE_INLINE_ static int shadow_atlas_get_size(RID p_atlas)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		return atlas ? atlas->size : 0;
	}

	_FORCE_INLINE_ static int shadow_atlas_get_quadrant_shadow_size(
		RID p_atlas, uint32_t p_quadrant)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		return atlas->quadrants[p_quadrant].shadows.size();
	}

	_FORCE_INLINE_ static uint32_t shadow_atlas_get_quadrant_subdivision(
		RID p_atlas, uint32_t p_quadrant)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		return atlas->quadrants[p_quadrant].subdivision;
	}

	_FORCE_INLINE_ static RID shadow_atlas_get_fb(RID p_atlas)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		return atlas ? atlas->fb : RID();
	}

	static void shadow_atlas_update(RID p_atlas);
	static RDC::DataFormat get_shadow_atlas_depth_format(bool p_16_bits);
	static uint32_t get_shadow_atlas_depth_usage_bits();

	/* Directional Shadow */
	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true);
	static int get_directional_light_shadow_size(RID p_light_instance);
	static void set_directional_shadow_count(int p_count);
	static Rect2i get_directional_shadow_rect();
	static void update_directional_shadow_atlas();

	_FORCE_INLINE_ static RID directional_shadow_get_texture() { return directional_shadow.depth; }

	_FORCE_INLINE_ static int directional_shadow_get_size() { return directional_shadow.size; }

	_FORCE_INLINE_ static RID direction_shadow_get_fb() { return directional_shadow.fb; }

	_FORCE_INLINE_ static void directional_shadow_increase_current_light()
	{
		directional_shadow.current_light++;
	}

	/* Shadow Cubemaps */
	static RID get_cubemap(int p_size);
	static RID get_cubemap_fb(int p_size, int p_pass);
	static RDC::DataFormat get_cubemap_depth_format();
	static uint32_t get_cubemap_depth_usage_bits();

	static bool get_shadow_cubemaps_used();
	static bool get_shadow_dual_paraboloid_used();
};

} // namespace RendererRD
// --- Folded from servers/rendering/renderer_rd/storage_rd/render_data_rd.h ---

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

#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/renderer_rd/shaders/cluster_debug.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/cluster_render.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/cluster_store.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"


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
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/canvas.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/canvas_occlusion.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"

typedef uint64_t PolygonID;

namespace RendererRD
{

struct LightUniform
{
	float matrix[8];		// light to texture coordinate matrix
	float shadow_matrix[8]; // light to shadow coordinate matrix
	float color[4];

	uint8_t shadow_color[4];
	uint32_t flags; // index to light texture
	float shadow_pixel_size;
	float height;

	float position[2];
	float shadow_z_far_inv;
	float shadow_y_ofs;

	float atlas_rect[4];
};

struct ShaderSpecialization
{
	union
	{
		uint32_t packed_0;

		struct
		{
			uint32_t use_lighting : 1;
			uint32_t use_msdf : 1;
			uint32_t use_lcd : 1;
		};
	};
};

enum ShaderVariant
{
	SHADER_VARIANT_QUAD,
	SHADER_VARIANT_NINEPATCH,
	SHADER_VARIANT_PRIMITIVE,
	SHADER_VARIANT_PRIMITIVE_POINTS,
	SHADER_VARIANT_ATTRIBUTES,
	SHADER_VARIANT_ATTRIBUTES_POINTS,
	SHADER_VARIANT_MAX
};

struct PipelineKey
{
	ShaderVariant variant = SHADER_VARIANT_MAX;
	RD::FramebufferFormatID framebuffer_format_id = RD::INVALID_FORMAT_ID;
	RD::VertexFormatID vertex_format_id = RDC::INVALID_ID;
	RDC::RenderPrimitive render_primitive = RDC::RENDER_PRIMITIVE_MAX;
	ShaderSpecialization shader_specialization = {};
	uint32_t lcd_blend = 0;
	uint32_t ubershader = 0;

	uint32_t hash() const
	{
		uint32_t h = hash_murmur3_one_32(variant);
		h = hash_murmur3_one_32(framebuffer_format_id, h);
		h = hash_murmur3_one_64((uint64_t)vertex_format_id, h);
		h = hash_murmur3_one_32(render_primitive, h);
		h = hash_murmur3_one_32(shader_specialization.packed_0, h);
		h = hash_murmur3_one_32(lcd_blend, h);
		h = hash_murmur3_one_32(ubershader, h);
		return hash_fmix32(h);
	}
};

struct CanvasShaderData : public RendererRD::MaterialStorage::ShaderData
{
	Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;
	int blend_mode = 0;

	Vector<uint32_t> ubo_offsets;
	uint32_t ubo_size = 0;

	String code;
	RID version;
	PipelineHashMapRD<PipelineKey, CanvasShaderData, void (CanvasShaderData::*)(PipelineKey)>
		pipeline_hash_map;

	static inline const uint32_t VERTEX_INPUT_MASKS_SIZE = SHADER_VARIANT_MAX * 2;
	std::atomic<uint64_t> vertex_input_masks[VERTEX_INPUT_MASKS_SIZE] = {};

	bool uses_screen_texture = false;
	bool uses_screen_texture_mipmaps = false;
	bool uses_sdf = false;
	bool uses_time = false;

	void _clear_vertex_input_mask_cache();
	void _create_pipeline(PipelineKey p_pipeline_key);
	virtual bool is_animated() const;
	virtual void set_code(const String& p_Code);
	virtual bool casts_shadows() const;
	virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const;
	virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const;
	RID get_shader(ShaderVariant p_shader_variant, bool p_ubershader) const;
	uint64_t get_vertex_input_mask(ShaderVariant p_shader_variant, bool p_ubershader);
	bool is_valid() const;

	CanvasShaderData();
	virtual ~CanvasShaderData();
};

struct CanvasMaterialData : public RendererRD::MaterialStorage::MaterialData
{
	CanvasShaderData* shader_data = nullptr;
	RID uniform_set;
	RID uniform_set_srgb;

	virtual void set_render_priority(int p_priority) {}

	virtual void set_next_pass(RID p_pass) {}

	virtual ~CanvasMaterialData();
};

struct TextureState
{
	static inline const uint32_t FILTER_SHIFT = 0;
	static inline const uint32_t FILTER_BITS = 3;
	static inline const uint32_t FILTER_MASK = (1 << FILTER_BITS) - 1;
	static inline const uint32_t REPEAT_SHIFT = FILTER_BITS;
	static inline const uint32_t REPEAT_BITS = 2;
	static inline const uint32_t REPEAT_MASK = (1 << REPEAT_BITS) - 1;
	static inline const uint32_t TEXTURE_IS_DATA_SHIFT = REPEAT_SHIFT + REPEAT_BITS;
	static inline const uint32_t TEXTURE_IS_DATA_BITS = 1;
	static inline const uint32_t TEXTURE_IS_DATA_MASK = (1 << TEXTURE_IS_DATA_BITS) - 1;
	static inline const uint32_t LINEAR_COLORS_SHIFT = TEXTURE_IS_DATA_SHIFT + TEXTURE_IS_DATA_BITS;
	static inline const uint32_t LINEAR_COLORS_BITS = 1;
	static inline const uint32_t LINEAR_COLORS_MASK = (1 << LINEAR_COLORS_BITS) - 1;

	RID texture;
	uint32_t other = 0;

	TextureState() {}

	TextureState(RID p_texture, RSE::CanvasItemTextureFilter p_base_filter,
		RSE::CanvasItemTextureRepeat p_base_repeat, bool p_texture_is_data,
		bool p_use_linear_colors)
	{
		texture = p_texture;
		other = (((uint32_t)p_base_filter & FILTER_MASK) << FILTER_SHIFT) |
				(((uint32_t)p_base_repeat & REPEAT_MASK) << REPEAT_SHIFT) |
				(((uint32_t)p_texture_is_data & TEXTURE_IS_DATA_MASK) << TEXTURE_IS_DATA_SHIFT) |
				(((uint32_t)p_use_linear_colors & LINEAR_COLORS_MASK) << LINEAR_COLORS_SHIFT);
	}

	_ALWAYS_INLINE_ RSE::CanvasItemTextureFilter texture_filter() const
	{
		return (RSE::CanvasItemTextureFilter)((other >> FILTER_SHIFT) & FILTER_MASK);
	}

	_ALWAYS_INLINE_ RSE::CanvasItemTextureRepeat texture_repeat() const
	{
		return (RSE::CanvasItemTextureRepeat)((other >> REPEAT_SHIFT) & REPEAT_MASK);
	}

	_ALWAYS_INLINE_ bool linear_colors() const
	{
		return (other >> LINEAR_COLORS_SHIFT) & LINEAR_COLORS_MASK;
	}

	_ALWAYS_INLINE_ bool texture_is_data() const
	{
		return (other >> TEXTURE_IS_DATA_SHIFT) & TEXTURE_IS_DATA_MASK;
	}

	_ALWAYS_INLINE_ bool operator==(const TextureState& p_val) const
	{
		return (texture == p_val.texture) && (other == p_val.other);
	}

	_ALWAYS_INLINE_ bool operator!=(const TextureState& p_val) const
	{
		return (texture != p_val.texture) || (other != p_val.other);
	}

	_ALWAYS_INLINE_ bool is_valid() const { return texture.is_valid(); }

	_ALWAYS_INLINE_ bool is_null() const { return texture.is_null(); }

	uint32_t hash() const
	{
		uint32_t hash = hash_murmur3_one_64(texture.get_id());
		return hash_murmur3_one_32(other, hash);
	}
};

struct TextureInfo
{
	TextureState state;
	RID diffuse;
	RID normal;
	RID specular;
	RID sampler;
	Vector2 texpixel_size;
	uint32_t specular_shininess = 0;
	uint32_t flags = 0;
};

struct InstanceData
{
	float world[6];
	float ninepatch_pixel_size[2];

	union
	{
		// rect
		struct
		{
			float modulation[4];
			float ninepatch_margins[4];
			float dst_rect[4];
			float src_rect[4];
			float pad[2];
		};

		// primitive
		struct
		{
			float points[6];	// vec2 points[3]
			float uvs[6];		// vec2 points[3]
			uint32_t colors[6]; // colors encoded as half
		};
	};

	uint32_t flags;
	uint32_t instance_uniforms_ofs;
	uint32_t lights[4];
};


struct PolygonBuffers
{
	RD::VertexFormatID vertex_format_id;
	RID vertex_buffer;
	RID vertex_array;
	RID index_buffer;
	RID indices;
	uint32_t primitive_count = 0;
};

struct PushConstant
{
	ShaderSpecialization shader_specialization;
	uint32_t specular_shininess;
	uint32_t batch_flags;
	uint32_t pad0;

	float msdf[2];
	float color_texture_pixel_size[2];
};

struct PushConstantAttributes
{
	PushConstant base;

	float world[6];
	uint32_t flags;
	uint32_t instance_uniforms_ofs;
	float modulation[4];
	uint32_t lights[4];

	operator PushConstant&() { return base; }
};

struct Batch
{
	/// First instance index into the instance buffer for this batch.
	uint32_t start = 0;
	/// Number of instances in this batch.
	uint32_t instance_count = 0;
	/// Resource ID of the instance buffer for this batch.
	RID instance_buffer; // UMA
	/// Push-constant payload for non-VAO draws.
	InstanceData push_data = {};

	TextureInfo* tex_info;

	Color modulate = Color(1.0, 1.0, 1.0, 1.0);
	float msdf_pix_range = 0.0;
	float msdf_outline = 0.0;

	RCR::Item* clip = nullptr;

	RID material;
	CanvasMaterialData* material_data = nullptr;

	const RCR::Item::Command* command = nullptr;
	RCR::Item::Command::Type command_type =
		RCR::Item::Command::TYPE_ANIMATION_SLICE; // Can default to any type that doesn't form a
												  // batch.
	ShaderVariant shader_variant = SHADER_VARIANT_QUAD;
	RDC::RenderPrimitive render_primitive = RDC::RENDER_PRIMITIVE_TRIANGLES;
	bool use_lighting = false;
	bool use_msdf = false;
	bool use_lcd = false;
	bool has_blend = false;

	// batch-specific data
	union
	{
		// TYPE_PRIMITIVE
		uint32_t primitive_points = 0;
		// TYPE_PARTICLES
		uint32_t mesh_instance_count;
	};

	uint32_t flags = 0;

	_FORCE_INLINE_ PushConstant push_constant() const
	{
		PushConstant pc;
		pc.specular_shininess = tex_info->specular_shininess;
		pc.batch_flags = tex_info->flags | flags;
		pc.pad0 = 0;

		pc.msdf[0] = msdf_pix_range;
		pc.msdf[1] = msdf_outline;
		pc.color_texture_pixel_size[0] = tex_info->texpixel_size.x;
		pc.color_texture_pixel_size[1] = tex_info->texpixel_size.y;
		return pc;
	}

	_FORCE_INLINE_ PushConstantAttributes push_constant_attributes() const
	{
		PushConstantAttributes pc;
		pc.base = push_constant();
		memcpy(pc.world, push_data.world, sizeof(pc.world));
		memcpy(pc.modulation, push_data.modulation, sizeof(pc.modulation));
		memcpy(pc.lights, push_data.lights, sizeof(pc.lights));
		pc.flags = push_data.flags;
		pc.instance_uniforms_ofs = push_data.instance_uniforms_ofs;
		return pc;
	}
};

static inline struct State
{
	// state buffer
	struct Buffer
	{
		float canvas_transform[16];
		float screen_transform[16];
		float canvas_normal_transform[16];
		float canvas_modulate[4];

		float screen_pixel_size[2];
		float time;
		uint32_t use_pixel_snap;

		float sdf_to_tex[4];
		float sdf_to_screen[2];
		float screen_to_sdf[2];

		uint32_t directional_light_count;
		float tex_to_sdf;
		float shadow_pixel_size;
		uint32_t flags;
	};

	LocalVector<Batch> canvas_instance_batches;
	uint32_t current_batch_index = 0;

	static_assert(std::is_trivially_destructible_v<InstanceData>);
	static_assert(std::is_trivially_constructible_v<InstanceData>);

	MultiUmaBuffer<1u> instance_buffers = MultiUmaBuffer<1u>("CANVAS_INSTANCE_DATA");
	/// A pointer to the current instance buffer retrieved from <c>instance_buffers</c>.
	InstanceData* instance_data = nullptr;
	/// The index of the next instance to be added to <c>instance_data</c>.
	uint32_t instance_data_index = 0;
	/// Save the previous instance data to allow us to append .
	InstanceData* prev_instance_data = nullptr;
	uint32_t prev_instance_data_index = 0;

	InstanceData intermediary_instance_data;

	uint32_t max_instances_per_buffer = 16384;
	uint32_t max_instance_buffer_size = 16384 * sizeof(InstanceData);

	Vector<RD::Uniform> batch_texture_uniforms;
	RID current_batch_uniform_set;

	LightUniform* light_uniforms = nullptr;

	RID lights_storage_buffer;
	RID canvas_state_buffer;
	RID shadow_sampler;
	RID shadow_texture;
	RID shadow_depth_texture;
	RID shadow_fb;
	int shadow_texture_size = 2048;

	RID shadow_occluder_buffer;
	uint32_t shadow_occluder_buffer_size;
	RID shadow_ocluder_uniform_set;

	RID default_transforms_uniform_set;

	double time;

} state;

static inline struct
{
	HashMap<PolygonID, RendererRD::PolygonBuffers> polygons;
	PolygonID last_id;
} polygon_buffers;

static inline struct
{
	// Data must be guaranteed to be erased before the rest on the destructor.
	CanvasShaderData* default_version_data = nullptr;
	ShaderRD canvas_shader{CanvasShaderRD()};
	RID default_version_rd_shader;
	RID quad_index_buffer;
	RID quad_index_array;
	RD::VertexFormatID quad_vertex_format_id;
	RD::VertexFormatID primitive_vertex_format_id;
	ShaderCompiler compiler;
	uint32_t pipeline_compilations[RSE::PIPELINE_SOURCE_MAX] = {};
	Mutex mutex;
} shader;

};

class RendererCanvasRenderRD : public RendererCanvasRender
{
	enum
	{
		BASE_UNIFORM_SET = 0,
		MATERIAL_UNIFORM_SET = 1,
		TRANSFORMS_UNIFORM_SET = 2,
		BATCH_UNIFORM_SET = 3,
	};

	static inline const int SAMPLERS_BINDING_FIRST_INDEX = 10;
	// The size of the ring buffer to store GPU buffers. Triple-buffering the max expected frames in
	// flight.
	static inline const uint32_t BATCH_DATA_BUFFER_COUNT = 3;

	enum
	{
		INSTANCE_FLAGS_LIGHT_COUNT_SHIFT = 0, // 4 bits for light count.

		INSTANCE_FLAGS_CLIP_RECT_UV = (1 << 4),
		INSTANCE_FLAGS_TRANSPOSE_RECT = (1 << 5),

		INSTANCE_FLAGS_NINEPACH_DRAW_CENTER = (1 << 8),
		INSTANCE_FLAGS_NINEPATCH_H_MODE_SHIFT = 9,
		INSTANCE_FLAGS_NINEPATCH_V_MODE_SHIFT = 11,

		INSTANCE_FLAGS_SHADOW_MASKED_SHIFT = 13, // 16 bits.
	};

	enum
	{
		BATCH_FLAGS_INSTANCING_MASK = 0x7F,
		BATCH_FLAGS_INSTANCING_HAS_COLORS = (1 << 7),
		BATCH_FLAGS_INSTANCING_HAS_CUSTOM_DATA = (1 << 8),

		BATCH_FLAGS_DEFAULT_NORMAL_MAP_USED = (1 << 9),
		BATCH_FLAGS_DEFAULT_SPECULAR_MAP_USED = (1 << 10),
	};

	enum
	{
		CANVAS_FLAGS_CONVERT_ATTRIBUTES_TO_LINEAR = (1 << 0),
	};

	enum
	{
		LIGHT_FLAGS_TEXTURE_MASK = 0xFFFF,
		LIGHT_FLAGS_BLEND_SHIFT = 16,
		LIGHT_FLAGS_BLEND_MASK = (3 << 16),
		LIGHT_FLAGS_BLEND_MODE_ADD = (0 << 16),
		LIGHT_FLAGS_BLEND_MODE_SUB = (1 << 16),
		LIGHT_FLAGS_BLEND_MODE_MIX = (2 << 16),
		LIGHT_FLAGS_BLEND_MODE_MASK = (3 << 16),
		LIGHT_FLAGS_HAS_SHADOW = (1 << 20),
		LIGHT_FLAGS_FILTER_SHIFT = 22

	};

	enum
	{
		MAX_RENDER_ITEMS = 256 * 1024,
		MAX_LIGHT_TEXTURES = 1024,
		MAX_LIGHTS_PER_ITEM = 16,
		MAX_LIGHTS_PER_RENDER = 256,
	};

	/****************/
	/**** SHADER ****/
	/****************/

	static RendererRD::MaterialStorage::ShaderData* _create_shader_func();

	static RendererRD::MaterialStorage::ShaderData* _create_shader_funcs()
	{
		return _create_shader_func();
	}

	static RendererRD::MaterialStorage::MaterialData* _create_material_func(
		RendererRD::CanvasShaderData* p_shader);

	static RendererRD::MaterialStorage::MaterialData* _create_material_funcs(
		RendererRD::MaterialStorage::ShaderData* p_shader)
	{
		return _create_material_func(static_cast<RendererRD::CanvasShaderData*>(p_shader));
	}

	/**************************/
	/**** CANVAS TEXTURES *****/
	/**************************/

	struct
	{
		RSE::CanvasItemTextureFilter default_filter;
		RSE::CanvasItemTextureRepeat default_repeat;
	} default_samplers;

	/******************/
	/**** POLYGONS ****/
	/******************/

	static inline struct
	{
		RID index_array[4];
	} primitive_arrays;

	struct CanvasLight
	{
		RID texture;

		struct
		{
			bool enabled = false;
			float z_far;
			float y_offset;
			Transform2D directional_xform;
		} shadow;
	};

	static inline RID_Owner<CanvasLight> canvas_light_owner;

	struct PositionalShadowRenderPushConstant
	{
		float modelview[8];
		float rotation[4];
		float direction[2];
		float z_far;
		uint32_t pad;
		float z_near;
		uint32_t cull_mode;
		float pad2[2];
	};

	struct ShadowRenderPushConstant
	{
		float projection[16];
		float modelview[8];
		float direction[2];
		float z_far;
		uint32_t cull_mode;
	};

	struct OccluderPolygon
	{
		RSE::CanvasOccluderPolygonCullMode cull_mode;
		int line_point_count;
		RID vertex_buffer;
		RID vertex_array;
		RID index_buffer;
		RID index_array;

		int sdf_point_count;
		int sdf_index_count;
		RID sdf_vertex_buffer;
		RID sdf_vertex_array;
		RID sdf_index_buffer;
		RID sdf_index_array;
		bool sdf_is_lines;
	};

	static inline RID_Owner<OccluderPolygon> occluder_polygon_owner;

	enum ShadowRenderMode
	{
		SHADOW_RENDER_MODE_DIRECTIONAL_SHADOW,
		SHADOW_RENDER_MODE_POSITIONAL_SHADOW,
		SHADOW_RENDER_MODE_SDF,
	};

	enum
	{
		SHADOW_RENDER_SDF_TRIANGLES,
		SHADOW_RENDER_SDF_LINES,
	};

	struct ShadowRender
	{
		ShaderRD shader;
		RID shader_version;
		RID render_pipelines[2];
		RID sdf_render_pipelines[2];
		RD::VertexFormatID vertex_format;
		RD::VertexFormatID sdf_vertex_format;
		RD::FramebufferFormatID framebuffer_format;
		RD::FramebufferFormatID sdf_framebuffer_format;

		ShadowRender() : shader(CanvasOcclusionShaderRD()) {}
	};
	static inline ShadowRender shadow_render;

	// state that does not vary across rendering all items

	static_assert(sizeof(RendererRD::InstanceData) == 128, "2D instance data struct size must be 128 bytes");

	// TextureState is used to determine when a new batch is required due to a change of texture
	// state.

	/// A key used to uniquely identify a distinct BATCH_UNIFORM_SET
	struct RIDSetKey
	{
		RendererRD::TextureState state;

		RIDSetKey() {}

		RIDSetKey(RendererRD::TextureState p_state) : state(p_state) {}

		_ALWAYS_INLINE_ bool operator==(const RIDSetKey& p_val) const
		{
			return state == p_val.state;
		}

		_ALWAYS_INLINE_ bool operator!=(const RIDSetKey& p_val) const { return !(*this == p_val); }

		_ALWAYS_INLINE_ uint32_t hash() const { return state.hash(); }
	};

	static void _before_evict(RendererCanvasRenderRD::RIDSetKey& p_key, RID& p_rid);
	static void _uniform_set_invalidation_callback(void* p_userdata);
	static void _canvas_texture_invalidation_callback(bool p_deleted, void* p_userdata);

	typedef LRUCache<RIDSetKey, RID, HashMapHasherDefault, HashMapComparatorDefault<RIDSetKey>,
		_before_evict>
		RIDCache;
	static inline RIDCache rid_set_to_uniform_set;
	/// Maps a CanvasTexture to its associated uniform sets, which must
	/// be invalidated when the CanvasTexture is updated, such as changing the
	/// diffuse texture.
	static inline HashMap<RID, TightLocalVector<RID>> canvas_texture_to_uniform_set;

	static inline constexpr uint32_t PUSH_DATA_INSTANCE_COUNT =
		0x8000'0000; // Use high bit to indicate instance data comes from push_data.
	static inline constexpr uint32_t INSTANCE_COUNT_MASK = 0x7fff'ffff;

	static inline HashMap<RendererRD::TextureState, RendererRD::TextureInfo, HashMapHasherDefault,
		HashMapComparatorDefault<RendererRD::TextureState>,
		PagedAllocator<HashMapElement<RendererRD::TextureState, RendererRD::TextureInfo>>>
		texture_info_map;

	static inline RCR::Item* items[MAX_RENDER_ITEMS];

	static inline RendererRD::TextureInfo default_texture_info;

	static inline bool using_directional_lights = false;
	static inline RID default_canvas_texture;

	static inline RID default_canvas_group_shader;
	static inline RID default_canvas_group_material;
	static inline RID default_clip_children_material;
	static inline RID default_clip_children_shader;

	static inline RSE::CanvasItemTextureFilter default_filter =
		RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR;
	static inline RSE::CanvasItemTextureRepeat default_repeat =
		RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED;

	static RID _create_base_uniform_set(RID p_to_render_target, bool p_backbuffer);

	static inline bool debug_redraw = false;
	static inline Color debug_redraw_color;
	static inline double debug_redraw_time = 1.0;

	// A structure to store cached render target information
	struct RenderTarget
	{
		// Current render target for the canvas.
		RID render_target;
		bool use_linear_colors = false;
	};

	static inline RID _get_pipeline_specialization_or_ubershader(RendererRD::CanvasShaderData* p_shader_data,
		RendererRD::PipelineKey& r_pipeline_key, RendererRD::PushConstant& r_push_constant, RID p_mesh_instance = RID(),
		void* p_surface = nullptr, uint32_t p_surface_index = 0, RID* r_vertex_array = nullptr);
	static void _render_batch_items(RenderTarget p_to_render_target, int p_item_count,
		const Transform2D& p_canvas_transform_inverse, Light* p_lights, bool& r_sdf_used,
		bool p_to_backbuffer = false, RenderingServerTypes::RenderInfo* r_render_info = nullptr);
	static void _record_item_commands(const RCR::Item* p_item, RenderTarget p_render_target,
		const Transform2D& p_base_transform, RCR::Item*& r_current_clip, Light* p_lights,
		bool& r_batch_broken, bool& r_sdf_used, RendererRD::Batch*& r_current_batch);
	static void _render_batch(RD::DrawListID p_draw_list, RendererRD::CanvasShaderData* p_shader_data,
		RenderingDevice::FramebufferFormatID p_framebuffer_format, Light* p_lights,
		const RendererRD::Batch* p_batch, RenderingServerTypes::RenderInfo* r_render_info = nullptr);
	static void _prepare_batch_texture_info(
		RID p_texture, RendererRD::TextureState& p_state, RendererRD::TextureInfo* p_info);

	// non-UMA
	static RendererRD::InstanceData* new_instance_data(RendererRD::Batch& p_current_batch,
		const RendererRD::InstanceData& template_instance, bool p_use_push_data = false);
	[[nodiscard]] static RendererRD::Batch* _new_batch(bool& r_batch_broken);
	static void _add_to_batch(bool& r_batch_broken, RendererRD::Batch*& r_current_batch);
	static void _allocate_instance_buffer();

	static _FORCE_INLINE_ void _update_transform_2d_to_mat2x4(
		const Transform2D& p_transform, float* p_mat2x4);
	static _FORCE_INLINE_ void _update_transform_2d_to_mat2x3(
		const Transform2D& p_transform, float* p_mat2x3);

	static _FORCE_INLINE_ void _update_transform_2d_to_mat4(
		const Transform2D& p_transform, float* p_mat4);
	static _FORCE_INLINE_ void _update_transform_to_mat4(
		const Transform3D& p_transform, float* p_mat4);

	static void _update_shadow_atlas();
	static void _update_occluder_buffer(uint32_t p_size);

public:
	static void initialize();
	static void finalize();

	static PolygonID request_polygon(const Vector<int>& p_indices, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), int p_count = -1);
	static void free_polygon(PolygonID p_polygon);

	static RID light_create();
	static void light_set_texture(RID p_rid, RID p_texture);
	static void light_set_use_shadow(RID p_rid, bool p_enable);
	static void light_update_shadow(RID p_rid, int p_shadow_index, const Transform2D& p_light_xform,
		int p_light_mask, float p_near, float p_far, LightOccluderInstance* p_occluders,
		const Rect2& p_light_rect);
	static void light_update_directional_shadow(RID p_rid, int p_shadow_index,
		const Transform2D& p_light_xform, int p_light_mask, float p_cull_distance,
		const Rect2& p_clip_rect, LightOccluderInstance* p_occluders);

	static void render_sdf(RID p_render_target, LightOccluderInstance* p_occluders);

	static RID occluder_polygon_create();
	static void occluder_polygon_set_shape(
		RID p_occluder, const Vector<Vector2>& p_points, bool p_closed);
	static void occluder_polygon_set_cull_mode(
		RID p_occluder, RSE::CanvasOccluderPolygonCullMode p_mode);

	static void canvas_render_items(RID p_to_render_target, RCR::Item* p_item_list,
		const Color& p_modulate, Light* p_light_list, Light* p_directional_light_list,
		const Transform2D& p_canvas_transform, RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	static void set_shadow_texture_size(int p_size);

	static void set_debug_redraw(bool p_enabled, double p_time, const Color& p_color);

	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	static void set_time(double p_time);
	static void update();
	static bool free(RID p_rid);
	RendererCanvasRenderRD();
	~RendererCanvasRenderRD();
};



// --- End servers/rendering/renderer_rd/renderer_canvas_render_rd.h ---

#include "servers/rendering/renderer_rd/shaders/blit.glsl.gen.h"

#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/storage_rd/mesh_storage.h ---
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/shaders/skeleton.glsl.gen.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererRD
{

class MeshStorage final
{
public:
	enum DefaultRDBuffer
	{
		DEFAULT_RD_BUFFER_VERTEX,
		DEFAULT_RD_BUFFER_NORMAL,
		DEFAULT_RD_BUFFER_TANGENT,
		DEFAULT_RD_BUFFER_COLOR,
		DEFAULT_RD_BUFFER_TEX_UV,
		DEFAULT_RD_BUFFER_TEX_UV2,
		DEFAULT_RD_BUFFER_CUSTOM0,
		DEFAULT_RD_BUFFER_CUSTOM1,
		DEFAULT_RD_BUFFER_CUSTOM2,
		DEFAULT_RD_BUFFER_CUSTOM3,
		DEFAULT_RD_BUFFER_BONES,
		DEFAULT_RD_BUFFER_WEIGHTS,
		DEFAULT_RD_BUFFER_MAX,
	};

	enum IndirectMultiMesh : uint32_t
	{
		INDIRECT_MULTIMESH_COMMAND_STRIDE = 5
	};

private:
	static inline RID default_rd_storage_buffer;

	/* Mesh */

	static inline RID mesh_default_rd_buffers[DEFAULT_RD_BUFFER_MAX];

	struct MeshInstance;

	struct Mesh
	{
		struct Surface
		{
			RSE::PrimitiveType primitive = RSE::PRIMITIVE_POINTS;
			uint64_t format = 0;

			uint32_t vertex_count = 0;
			RID vertex_buffer;
			uint32_t vertex_buffer_size = 0;

			RID attribute_buffer;
			uint32_t attribute_buffer_size = 0;

			RID skin_buffer;
			uint32_t skin_buffer_size = 0;

			struct Version
			{
				uint64_t input_mask = 0;
				uint32_t current_buffer = 0;
				uint32_t previous_buffer = 0;
				bool input_motion_vectors = false;
				bool point_size_emulated = false;
				RD::VertexFormatID vertex_format = 0;
				RID vertex_array;
			};

			SpinLock version_lock;
			Version* versions = nullptr;
			uint32_t version_count = 0;

			RID index_buffer;
			uint32_t index_buffer_size = 0;
			RID index_array;
			uint32_t index_count = 0;

			struct LOD
			{
				float edge_length = 0.0;
				uint32_t index_count = 0;
				RID index_buffer;
				uint32_t index_buffer_size = 0;
				RID index_array;
			};

			LOD* lods = nullptr;
			uint32_t lod_count = 0;

			AABB aabb;

			Vector<AABB> bone_aabbs;

			Transform3D mesh_to_skeleton_xform;

			Vector4 uv_scale;

			RID blend_shape_buffer;
			uint32_t blend_shape_buffer_size = 0;

			RID material;

			uint32_t render_index = 0;
			uint64_t render_pass = 0;

			uint32_t multimesh_render_index = 0;
			uint64_t multimesh_render_pass = 0;

			uint32_t particles_render_index = 0;
			uint64_t particles_render_pass = 0;

			RID uniform_set;
		};

		uint32_t blend_shape_count = 0;
		RSE::BlendShapeMode blend_shape_mode = RSE::BLEND_SHAPE_MODE_NORMALIZED;

		Surface** surfaces = nullptr;
		uint32_t surface_count = 0;

		bool has_bone_weights = false;

		AABB aabb;
		AABB custom_aabb;
		uint64_t skeleton_aabb_version = 0;
		RID skeleton_aabb_rid;

		Vector<RID> material_cache;

		List<MeshInstance*> instances;

		RID shadow_mesh;
		HashSet<Mesh*> shadow_owners;

		String path;

		Dependency dependency;
	};

	static inline RID_Owner<Mesh, true> mesh_owner;

	/* Mesh Instance API */

	struct MeshInstance
	{
		Mesh* mesh = nullptr;
		RID skeleton;

		struct Surface
		{
			RID vertex_buffer[2];
			RID uniform_set[2];
			uint32_t current_buffer = 0;
			uint32_t previous_buffer = 0;
			uint64_t last_change = 0;

			Mesh::Surface::Version* versions = nullptr;
			uint32_t version_count = 0;
		};

		LocalVector<Surface> surfaces;
		LocalVector<float> blend_weights;

		RID blend_weights_buffer;
		List<MeshInstance*>::Element* I = nullptr;
		uint64_t skeleton_version = 0;
		bool dirty = false;
		bool weights_dirty = false;
		SelfList<MeshInstance> weight_update_list;
		SelfList<MeshInstance> array_update_list;
		Transform2D canvas_item_transform_2d;

		MeshInstance() : weight_update_list(this), array_update_list(this) {}
	};

	static RD::VertexFormatID _mesh_surface_generate_vertex_format(uint64_t p_surface_format,
		uint64_t p_input_mask, bool p_instanced_surface, bool p_input_motion_vectors,
		bool p_point_size_emulated, uint32_t& r_position_stride);
	static void _mesh_surface_generate_version_for_input_mask(Mesh::Surface::Version& v, Mesh::Surface* s,
		uint64_t p_input_mask, bool p_input_motion_vectors, bool p_point_size_emulated,
		MeshInstance::Surface* mis = nullptr, uint32_t p_current_buffer = 0,
		uint32_t p_previous_buffer = 0);
	static void _mesh_surface_clear(Mesh* p_mesh, int p_surface);

	static void _mesh_instance_clear(MeshInstance* mi);
	static void _mesh_instance_add_surface(MeshInstance* mi, Mesh* mesh, uint32_t p_surface);
	static void _mesh_instance_add_surface_buffer(MeshInstance* mi, Mesh* mesh, MeshInstance::Surface* s,
		uint32_t p_surface, uint32_t p_buffer_index);
	static void _mesh_instance_remove_surface(MeshInstance* mi, int p_surface);

	static inline RID_Owner<MeshInstance> mesh_instance_owner;

	static inline SelfList<MeshInstance>::List dirty_mesh_instance_weights;
	static inline SelfList<MeshInstance>::List dirty_mesh_instance_arrays;

	/* MultiMesh */

	struct MultiMesh
	{
		RID mesh;
		int instances = 0;
		RSE::MultimeshTransformFormat xform_format = RSE::MULTIMESH_TRANSFORM_3D;
		bool uses_colors = false;
		bool uses_custom_data = false;
		int visible_instances = -1;
		AABB aabb;
		AABB custom_aabb;
		bool aabb_dirty = false;
		bool buffer_set = false;
		bool indirect = false;
		bool motion_vectors_enabled = false;
		uint32_t motion_vectors_current_offset = 0;
		uint32_t motion_vectors_previous_offset = 0;
		uint64_t motion_vectors_last_change = -1;
		uint32_t stride_cache = 0;
		uint32_t color_offset_cache = 0;
		uint32_t custom_data_offset_cache = 0;

		Vector<float> data_cache;
		bool* data_cache_dirty_regions = nullptr;
		uint32_t data_cache_dirty_region_count = 0;
		bool* previous_data_cache_dirty_regions = nullptr;
		uint32_t previous_data_cache_dirty_region_count = 0;

		RID buffer;
		RID uniform_set_3d;
		RID uniform_set_2d;
		RID command_buffer;

		bool dirty = false;
		MultiMesh* dirty_list = nullptr;

		RendererMeshStorage::MultiMeshInterpolator interpolator;

		Dependency dependency;
	};

	static inline RID_Owner<MultiMesh, true> multimesh_owner;

	static inline MultiMesh* multimesh_dirty_list = nullptr;

	static _FORCE_INLINE_ void _multimesh_make_local(MultiMesh* multimesh);
	static _FORCE_INLINE_ void _multimesh_update_motion_vectors_data_cache(MultiMesh* multimesh);
	static _FORCE_INLINE_ bool _multimesh_uses_motion_vectors(MultiMesh* multimesh);
	static _FORCE_INLINE_ void _multimesh_mark_dirty(MultiMesh* multimesh, int p_index, bool p_aabb);
	static _FORCE_INLINE_ void _multimesh_mark_all_dirty(MultiMesh* multimesh, bool p_data, bool p_aabb);
	static _FORCE_INLINE_ void _multimesh_re_create_aabb(
		MultiMesh* multimesh, const float* p_data, int p_instances);

	/* Skeleton */

	struct SkeletonShader
	{
		struct PushConstant
		{
			uint32_t has_normal;
			uint32_t has_tangent;
			uint32_t has_skeleton;
			uint32_t has_blend_shape;

			uint32_t vertex_count;
			uint32_t vertex_stride;
			uint32_t skin_stride;
			uint32_t skin_weight_offset;

			uint32_t blend_shape_count;
			uint32_t normalized_blend_shapes;
			uint32_t normal_tangent_stride;
			uint32_t pad1;
			float skeleton_transform_x[2];
			float skeleton_transform_y[2];

			float skeleton_transform_offset[2];
			float inverse_transform_x[2];

			float inverse_transform_y[2];
			float inverse_transform_offset[2];
		};

		enum
		{
			UNIFORM_SET_INSTANCE = 0,
			UNIFORM_SET_SURFACE = 1,
			UNIFORM_SET_SKELETON = 2,
		};

		enum
		{
			SHADER_MODE_2D,
			SHADER_MODE_3D,
			SHADER_MODE_MAX
		};

		ShaderRD shader{SkeletonShaderRD()};
		RID version;
		RID version_shader[SHADER_MODE_MAX];
		RID pipeline[SHADER_MODE_MAX];

		RID default_skeleton_uniform_set;
	};

	static inline SkeletonShader* skeleton_shader = nullptr;

	struct Skeleton
	{
		bool use_2d = false;
		int size = 0;
		LocalVector<float> data;
		RID buffer;

		bool dirty = false;
		Skeleton* dirty_list = nullptr;
		Transform2D base_transform_2d;

		RID uniform_set_3d;
		RID uniform_set_mi;

		uint64_t version = 1;

		Dependency dependency;
	};

	static inline RID_Owner<Skeleton, true> skeleton_owner;

	static _FORCE_INLINE_ void _skeleton_make_dirty(Skeleton* skeleton);

	static inline Skeleton* skeleton_dirty_list = nullptr;

	enum AttributeLocation
	{
		ATTRIBUTE_LOCATION_PREV_VERTEX = 12,
		ATTRIBUTE_LOCATION_PREV_NORMAL = 13,
		ATTRIBUTE_LOCATION_PREV_TANGENT = 14
	};

public:
	MeshStorage() = delete;
	~MeshStorage() = delete;
	MeshStorage(const MeshStorage&) = delete;
	MeshStorage& operator=(const MeshStorage&) = delete;

	static void initialize();
	static void finalize();
	static bool free(RID p_rid);

	static RID get_default_rd_storage_buffer() { return default_rd_storage_buffer; }

	/* MESH API */

	static bool owns_mesh(RID p_rid) { return mesh_owner.owns(p_rid); }

	static RID mesh_allocate();
	static void mesh_initialize(RID p_mesh);
	static void mesh_free(RID p_rid);

	static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count);

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

	static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface);

	static int mesh_get_surface_count(RID p_mesh);

	static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB mesh_get_custom_aabb(RID p_mesh);

	static AABB mesh_get_aabb(RID p_mesh, RID p_skeleton = RID());
	static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh);

	static void mesh_set_path(RID p_mesh, const String& p_path);
	static String mesh_get_path(RID p_mesh);

	static void mesh_clear(RID p_mesh);
	static void mesh_surface_remove(RID p_mesh, int p_surface);

	static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info);

	static bool mesh_needs_instance(RID p_mesh, bool p_has_skeleton);

	static _FORCE_INLINE_ const RID* mesh_get_surface_count_and_materials(
		RID p_mesh, uint32_t& r_surface_count)
	{
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(mesh, nullptr);
		r_surface_count = mesh->surface_count;
		if (r_surface_count == 0) {
			return nullptr;
		}
		if (mesh->material_cache.is_empty()) {
			mesh->material_cache.resize(mesh->surface_count);
			for (uint32_t i = 0; i < r_surface_count; i++) {
				mesh->material_cache.write[i] = mesh->surfaces[i]->material;
			}
		}

		return mesh->material_cache.ptr();
	}

	static _FORCE_INLINE_ void* mesh_get_surface(RID p_mesh, uint32_t p_surface_index)
	{
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(mesh, nullptr);
		ERR_FAIL_UNSIGNED_INDEX_V(p_surface_index, mesh->surface_count, nullptr);

		return mesh->surfaces[p_surface_index];
	}

	static _FORCE_INLINE_ RID mesh_get_shadow_mesh(RID p_mesh)
	{
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(mesh, RID());

		return mesh->shadow_mesh;
	}

	static _FORCE_INLINE_ RSE::PrimitiveType mesh_surface_get_primitive(void* p_surface)
	{
		Mesh::Surface* surface = reinterpret_cast<Mesh::Surface*>(p_surface);
		return surface->primitive;
	}

	static _FORCE_INLINE_ uint32_t mesh_surface_get_vertex_count(void* p_surface)
	{
		Mesh::Surface* surface = reinterpret_cast<Mesh::Surface*>(p_surface);
		return surface->vertex_count;
	}

	static _FORCE_INLINE_ bool mesh_surface_has_lod(void* p_surface)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);
		return s->lod_count > 0;
	}

	static _FORCE_INLINE_ uint32_t mesh_surface_get_vertices_drawn_count(void* p_surface)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);
		return s->index_count ? s->index_count : s->vertex_count;
	}

	static _FORCE_INLINE_ AABB mesh_surface_get_aabb(void* p_surface)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);
		return s->aabb;
	}

	static _FORCE_INLINE_ uint64_t mesh_surface_get_format(void* p_surface)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);
		return s->format;
	}

	static _FORCE_INLINE_ Vector4 mesh_surface_get_uv_scale(void* p_surface)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);
		return s->uv_scale;
	}

	static _FORCE_INLINE_ uint32_t mesh_surface_get_lod(void* p_surface, float p_model_scale,
		float p_distance_threshold, float p_mesh_lod_threshold, uint32_t& r_index_count)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);

		int32_t current_lod = -1;
		r_index_count = s->index_count;
		for (uint32_t i = 0; i < s->lod_count; i++) {
			float screen_size = s->lods[i].edge_length * p_model_scale / p_distance_threshold;
			if (screen_size > p_mesh_lod_threshold) {
				break;
			}
			current_lod = i;
		}
		if (current_lod == -1) {
			return 0;
		}
		else {
			r_index_count = s->lods[current_lod].index_count;
			return current_lod + 1;
		}
	}

	static _FORCE_INLINE_ RID mesh_surface_get_index_array(void* p_surface, uint32_t p_lod)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);

		if (p_lod == 0) {
			return s->index_array;
		}
		else {
			return s->lods[p_lod - 1].index_array;
		}
	}

	static _FORCE_INLINE_ void mesh_surface_get_vertex_arrays_and_format(void* p_surface,
		uint64_t p_input_mask, bool p_input_motion_vectors, bool p_point_size_emulated,
		RID& r_vertex_array_rd, RD::VertexFormatID& r_vertex_format)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);

		s->version_lock.lock();

		for (uint32_t i = 0; i < s->version_count; i++) {
			if (s->versions[i].input_mask != p_input_mask ||
				s->versions[i].input_motion_vectors != p_input_motion_vectors ||
				s->versions[i].point_size_emulated != p_point_size_emulated) {
				continue;
			}

			r_vertex_format = s->versions[i].vertex_format;
			r_vertex_array_rd = s->versions[i].vertex_array;
			s->version_lock.unlock();
			return;
		}

		uint32_t version = s->version_count;
		s->version_count++;
		s->versions = (Mesh::Surface::Version*)memrealloc(
			s->versions, sizeof(Mesh::Surface::Version) * s->version_count);

		_mesh_surface_generate_version_for_input_mask(
			s->versions[version], s, p_input_mask, p_input_motion_vectors, p_point_size_emulated);

		r_vertex_format = s->versions[version].vertex_format;
		r_vertex_array_rd = s->versions[version].vertex_array;

		s->version_lock.unlock();
	}

	static _FORCE_INLINE_ void mesh_instance_surface_get_vertex_arrays_and_format(RID p_mesh_instance,
		uint64_t p_surface_index, uint64_t p_input_mask, bool p_input_motion_vectors,
		bool p_point_size_emulated, RID& r_vertex_array_rd, RD::VertexFormatID& r_vertex_format)
	{
		MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
		ERR_FAIL_NULL(mi);
		Mesh* mesh = mi->mesh;
		ERR_FAIL_UNSIGNED_INDEX(p_surface_index, mesh->surface_count);

		MeshInstance::Surface* mis = &mi->surfaces[p_surface_index];
		Mesh::Surface* s = mesh->surfaces[p_surface_index];
		uint32_t current_buffer = mis->current_buffer;

		uint32_t previous_buffer =
			p_input_motion_vectors && (RendererCompositor::get_frame_number() == mis->last_change)
				? mis->previous_buffer
				: current_buffer;

		s->version_lock.lock();

		for (uint32_t i = 0; i < mis->version_count; i++) {
			if (mis->versions[i].input_mask != p_input_mask ||
				mis->versions[i].input_motion_vectors != p_input_motion_vectors) {
				continue;
			}

			if (mis->versions[i].current_buffer != current_buffer ||
				mis->versions[i].previous_buffer != previous_buffer) {
				continue;
			}

			r_vertex_format = mis->versions[i].vertex_format;
			r_vertex_array_rd = mis->versions[i].vertex_array;
			s->version_lock.unlock();
			return;
		}

		uint32_t version = mis->version_count;
		mis->version_count++;
		mis->versions = (Mesh::Surface::Version*)memrealloc(
			mis->versions, sizeof(Mesh::Surface::Version) * mis->version_count);

		_mesh_surface_generate_version_for_input_mask(mis->versions[version], s, p_input_mask,
			p_input_motion_vectors, p_point_size_emulated, mis, current_buffer, previous_buffer);

		r_vertex_format = mis->versions[version].vertex_format;
		r_vertex_array_rd = mis->versions[version].vertex_array;

		s->version_lock.unlock();
	}

	static _FORCE_INLINE_ RID mesh_get_default_rd_buffer(DefaultRDBuffer p_buffer)
	{
		ERR_FAIL_INDEX_V(p_buffer, DEFAULT_RD_BUFFER_MAX, RID());
		return mesh_default_rd_buffers[p_buffer];
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_render_pass_index(
		RID p_mesh, uint32_t p_surface_index, uint64_t p_render_pass, uint32_t* r_index)
	{
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		Mesh::Surface* s = mesh->surfaces[p_surface_index];

		if (s->render_pass != p_render_pass) {
			(*r_index)++;
			s->render_pass = p_render_pass;
			s->render_index = *r_index;
		}

		return s->render_index;
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_multimesh_render_pass_index(
		RID p_mesh, uint32_t p_surface_index, uint64_t p_render_pass, uint32_t* r_index)
	{
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		Mesh::Surface* s = mesh->surfaces[p_surface_index];

		if (s->multimesh_render_pass != p_render_pass) {
			(*r_index)++;
			s->multimesh_render_pass = p_render_pass;
			s->multimesh_render_index = *r_index;
		}

		return s->multimesh_render_index;
	}

	_FORCE_INLINE_ uint32_t mesh_surface_get_particles_render_pass_index(
		RID p_mesh, uint32_t p_surface_index, uint64_t p_render_pass, uint32_t* r_index)
	{
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		Mesh::Surface* s = mesh->surfaces[p_surface_index];

		if (s->particles_render_pass != p_render_pass) {
			(*r_index)++;
			s->particles_render_pass = p_render_pass;
			s->particles_render_index = *r_index;
		}

		return s->particles_render_index;
	}

	static _FORCE_INLINE_ RD::VertexFormatID mesh_surface_get_vertex_format(void* p_surface,
		uint64_t p_input_mask, bool p_instanced_surface, bool p_input_motion_vectors,
		bool p_point_size_emulated)
	{
		Mesh::Surface* s = reinterpret_cast<Mesh::Surface*>(p_surface);
		uint32_t position_stride = 0;
		return _mesh_surface_generate_vertex_format(s->format, p_input_mask, p_instanced_surface,
			p_input_motion_vectors, p_point_size_emulated, position_stride);
	}

	static Dependency* mesh_get_dependency(RID p_mesh);

	/* MESH INSTANCE API */

	static bool owns_mesh_instance(RID p_rid) { return mesh_instance_owner.owns(p_rid); }

	static RID mesh_instance_create(RID p_base);
	static void mesh_instance_free(RID p_rid);
	static void mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton);
	static void mesh_instance_set_blend_shape_weight(RID p_mesh_instance, int p_shape, float p_weight);
	static void mesh_instance_check_for_update(RID p_mesh_instance);
	static void mesh_instance_set_canvas_item_transform(
		RID p_mesh_instance, const Transform2D& p_transform);
	static void update_mesh_instances();

	/* MULTIMESH API */

	static bool owns_multimesh(RID p_rid) { return multimesh_owner.owns(p_rid); }

	static RID _multimesh_allocate();
	static void _multimesh_initialize(RID p_multimesh);
	static void _multimesh_free(RID p_rid);

	static void _multimesh_allocate_data(RID p_multimesh, int p_instances,
		RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
		bool p_use_custom_data = false, bool p_use_indirect = false);
	static int _multimesh_get_instance_count(RID p_multimesh);

	static void _multimesh_set_mesh(RID p_multimesh, RID p_mesh);
	static void _multimesh_instance_set_transform(
		RID p_multimesh, int p_index, const Transform3D& p_transform);
	static void _multimesh_instance_set_transform_2d(
		RID p_multimesh, int p_index, const Transform2D& p_transform);
	static void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color);
	static void _multimesh_instance_set_custom_data(RID p_multimesh, int p_index, const Color& p_color);

	static RID _multimesh_get_mesh(RID p_multimesh);

	static Transform3D _multimesh_instance_get_transform(RID p_multimesh, int p_index);
	static Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
	static Color _multimesh_instance_get_color(RID p_multimesh, int p_index);
	static Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

	static void _multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
	static RID _multimesh_get_command_buffer_rd_rid(RID p_multimesh);
	static RID _multimesh_get_buffer_rd_rid(RID p_multimesh);
	static Vector<float> _multimesh_get_buffer(RID p_multimesh);

	static void _multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	static int _multimesh_get_visible_instances(RID p_multimesh);

	static void _multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb);
	static AABB _multimesh_get_custom_aabb(RID p_multimesh);

	static AABB _multimesh_get_aabb(RID p_multimesh);

	static RendererMeshStorage::MultiMeshInterpolator* _multimesh_get_interpolator(RID p_multimesh);

	static void _update_dirty_multimeshes();
	static void _multimesh_get_motion_vectors_offsets(
		RID p_multimesh, uint32_t& r_current_offset, uint32_t& r_prev_offset);
	static bool _multimesh_uses_motion_vectors_offsets(RID p_multimesh);
	static bool _multimesh_uses_motion_vectors(RID p_multimesh);

	static _FORCE_INLINE_ bool multimesh_uses_indirect(RID p_multimesh)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		ERR_FAIL_NULL_V(multimesh, false);
		return multimesh->indirect;
	}

	static _FORCE_INLINE_ RSE::MultimeshTransformFormat multimesh_get_transform_format(
		RID p_multimesh)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		ERR_FAIL_NULL_V(multimesh, RSE::MULTIMESH_TRANSFORM_3D);
		return multimesh->xform_format;
	}

	static _FORCE_INLINE_ bool multimesh_uses_colors(RID p_multimesh)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		ERR_FAIL_NULL_V(multimesh, false);
		return multimesh->uses_colors;
	}

	static _FORCE_INLINE_ bool multimesh_uses_custom_data(RID p_multimesh)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		ERR_FAIL_NULL_V(multimesh, false);
		return multimesh->uses_custom_data;
	}

	static _FORCE_INLINE_ uint32_t multimesh_get_instances_to_draw(RID p_multimesh)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		ERR_FAIL_NULL_V(multimesh, 0);
		if (multimesh->visible_instances >= 0) {
			return multimesh->visible_instances;
		}
		return multimesh->instances;
	}

	static _FORCE_INLINE_ RID multimesh_get_3d_uniform_set(
		RID p_multimesh, RID p_shader, uint32_t p_set)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		if (multimesh == nullptr) {
			return RID();
		}
		if (!multimesh->uniform_set_3d.is_valid()) {
			if (!multimesh->buffer.is_valid()) {
				return RID();
			}
			Vector<RD::Uniform> uniforms;
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(multimesh->buffer);
			uniforms.push_back(u);
			multimesh->uniform_set_3d = RD::uniform_set_create(uniforms, p_shader, p_set);
		}

		return multimesh->uniform_set_3d;
	}

	static _FORCE_INLINE_ RID multimesh_get_2d_uniform_set(
		RID p_multimesh, RID p_shader, uint32_t p_set)
	{
		MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
		if (multimesh == nullptr) {
			return RID();
		}
		if (!multimesh->uniform_set_2d.is_valid()) {
			if (!multimesh->buffer.is_valid()) {
				return RID();
			}
			Vector<RD::Uniform> uniforms;
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(multimesh->buffer);
			uniforms.push_back(u);
			multimesh->uniform_set_2d = RD::uniform_set_create(uniforms, p_shader, p_set);
		}

		return multimesh->uniform_set_2d;
	}

	static Dependency* multimesh_get_dependency(RID p_multimesh);

	/* SKELETON API */

	static bool owns_skeleton(RID p_rid) { return skeleton_owner.owns(p_rid); }

	static RID skeleton_allocate();
	static void skeleton_initialize(RID p_skeleton);
	static void skeleton_free(RID p_rid);

	static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false);
	static void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D& p_base_transform);
	static int skeleton_get_bone_count(RID p_skeleton);
	static void skeleton_bone_set_transform(RID p_skeleton, int p_bone, const Transform3D& p_transform);
	static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone);
	static void skeleton_bone_set_transform_2d(RID p_skeleton, int p_bone, const Transform2D& p_transform);
	static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone);

	static void skeleton_update_dependency(RID p_skeleton, DependencyTracker* p_instance);

	static void _update_dirty_skeletons();

	static _FORCE_INLINE_ bool skeleton_is_valid(RID p_skeleton)
	{
		return skeleton_owner.get_or_null(p_skeleton) != nullptr;
	}

	static _FORCE_INLINE_ RID skeleton_get_3d_uniform_set(
		RID p_skeleton, RID p_shader, uint32_t p_set)
	{
		Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
		ERR_FAIL_NULL_V(skeleton, RID());
		if (skeleton->size == 0) {
			return RID();
		}
		if (skeleton->use_2d) {
			return RID();
		}
		if (!skeleton->uniform_set_3d.is_valid()) {
			Vector<RD::Uniform> uniforms;
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(skeleton->buffer);
			uniforms.push_back(u);
			skeleton->uniform_set_3d = RD::uniform_set_create(uniforms, p_shader, p_set);
		}

		return skeleton->uniform_set_3d;
	}
};

} // namespace RendererRD
// --- Inlined/Folded from servers/rendering/renderer_rd/storage_rd/particles_storage.h ---
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/particles.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/particles_copy.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererRD
{

class SortEffects;

class ParticlesStorage final
{
public:
	enum
	{
		BASE_UNIFORM_SET,
		MATERIAL_UNIFORM_SET,
		COLLISION_TEXTURTES_UNIFORM_SET,
	};

	static inline const int SAMPLERS_BINDING_FIRST_INDEX = 3;

private:
	/* EFFECTS */
	static inline SortEffects* sort_effects = nullptr;

	/* PARTICLES */

	struct ParticleData
	{
		float xform[16];
		float velocity[3];
		uint32_t active;
		float color[4];
		float custom[4];
	};

	struct ParticlesFrameParams
	{
		enum
		{
			MAX_ATTRACTORS = 32,
			MAX_COLLIDERS = 32,
			MAX_3D_TEXTURES = 7
		};

		enum AttractorType
		{
			ATTRACTOR_TYPE_SPHERE,
			ATTRACTOR_TYPE_BOX,
			ATTRACTOR_TYPE_VECTOR_FIELD,
		};

		struct Attractor
		{
			float transform[16];
			float extents[3]; // extents or radius
			uint32_t type;

			uint32_t texture_index; // texture index for vector field
			float strength;
			float attenuation;
			float directionality;
		};

		enum CollisionType
		{
			COLLISION_TYPE_SPHERE,
			COLLISION_TYPE_BOX,
			COLLISION_TYPE_SDF,
			COLLISION_TYPE_HEIGHT_FIELD,
			COLLISION_TYPE_2D_SDF,
		};

		struct Collider
		{
			float transform[16];
			float extents[3]; // extents or radius
			uint32_t type;

			uint32_t texture_index; // texture index for vector field
			float scale;
			uint32_t pad[2];
		};

		uint32_t emitting;
		float system_phase;
		float prev_system_phase;
		uint32_t cycle;

		float explosiveness;
		float randomness;
		float time;
		float delta;

		uint32_t frame;
		float amount_ratio;
		uint32_t pad1;
		uint32_t pad2;

		uint32_t random_seed;
		uint32_t attractor_count;
		uint32_t collider_count;
		float particle_size;

		float emission_transform[16];

		float emitter_velocity[3];
		float interp_to_end;

		Attractor attractors[MAX_ATTRACTORS];
		Collider colliders[MAX_COLLIDERS];
	};

	struct ParticleEmissionBuffer
	{
		struct Data
		{
			float xform[16];
			float velocity[3];
			uint32_t flags;
			float color[4];
			float custom[4];
		};

		int32_t particle_count;
		int32_t particle_max;
		uint32_t pad1;
		uint32_t pad2;
		Data data[1];
	};

	struct Particles
	{
		RSE::ParticlesMode mode = RSE::PARTICLES_MODE_3D;
		bool inactive = true;
		double inactive_time = 0.0;
		bool emitting = false;
		bool one_shot = false;
		int amount = 0;
		double lifetime = 1.0;
		double pre_process_time = 0.0;
		real_t request_process_time = 0.0;
		real_t request_process_time_residual = 0.0;
		real_t explosiveness = 0.0;
		real_t randomness = 0.0;
		bool restart_request = false;
		AABB custom_aabb = AABB(Vector3(-4, -4, -4), Vector3(8, 8, 8));
		bool use_local_coords = false;
		bool has_collision_cache = false;

		bool has_sdf_collision = false;
		Transform2D sdf_collision_transform;
		Rect2 sdf_collision_to_screen;
		RID sdf_collision_texture;

		RID process_material;
		uint32_t frame_counter = 0;

		RSE::ParticlesTransformAlign transform_align = RSE::PARTICLES_TRANSFORM_ALIGN_DISABLED;
		RSE::ParticlesTransformAlignCustomSrc transform_align_src =
			RSE::PARTICLES_ALIGN_CHANNEL_FILTER_X;
		RSE::ParticlesTransformAlignAxis transform_align_axis =
			RSE::PARTICLES_ALIGN_AXIS_Y;

		RSE::ParticlesDrawOrder draw_order = RSE::PARTICLES_DRAW_ORDER_INDEX;

		Vector<RID> draw_passes;
		Vector<Transform3D> trail_bind_poses;
		bool trail_bind_poses_dirty = false;
		RID trail_bind_pose_buffer;
		RID trail_bind_pose_uniform_set;

		RID particle_buffer;
		RID particle_instance_buffer;
		RID frame_params_buffer;

		uint32_t userdata_count = 0;

		RID particles_material_uniform_set;
		RID particles_copy_uniform_set;
		RID particles_transforms_buffer_uniform_set;
		RID collision_textures_uniform_set;

		RID collision_3d_textures[ParticlesFrameParams::MAX_3D_TEXTURES];
		uint32_t collision_3d_textures_used = 0;
		RID collision_heightmap_texture;

		RID particles_sort_buffer;
		RID particles_sort_uniform_set;

		bool dirty = false;
		SelfList<Particles> update_list;

		RID sub_emitter;

		double phase = 0.0;
		double prev_phase = 0.0;
		uint64_t prev_ticks = 0;
		uint32_t random_seed = 0;

		uint32_t cycle_number = 0;

		double speed_scale = 1.0;

		int fixed_fps = 30;
		bool interpolate = true;
		bool fractional_delta = false;
		double frame_remainder = 0;
		real_t collision_base_size = 0.01;

		uint32_t instance_motion_vectors_current_offset = 0;
		uint32_t instance_motion_vectors_previous_offset = 0;
		uint64_t instance_motion_vectors_last_change = -1;
		bool instance_motion_vectors_enabled = false;

		bool clear = true;

		bool force_sub_emit = false;

		Transform3D emission_transform;
		Vector3 emitter_velocity;
		float interp_to_end = 0.0;
		float amount_ratio = 1.0;

		Vector<uint8_t> emission_buffer_data;

		ParticleEmissionBuffer* emission_buffer = nullptr;
		RID emission_storage_buffer;

		RID unused_emission_storage_buffer;
		RID unused_trail_storage_buffer;

		HashSet<RID> collisions;

		Dependency dependency;

		double trail_lifetime = 0.3;
		bool trails_enabled = false;
		LocalVector<ParticlesFrameParams> frame_history;
		LocalVector<ParticlesFrameParams> trail_params;

		Particles() : update_list(this) { random_seed = Math::rand(); }
	};

	static void _particles_process(Particles* p_particles, double p_delta);
	static void _particles_allocate_emission_buffer(Particles* particles);
	static void _particles_ensure_unused_emission_buffer(Particles* particles);
	static void _particles_ensure_unused_trail_buffer(Particles* particles);
	static void _particles_free_data(Particles* particles);
	static void _particles_update_buffers(Particles* particles);

	struct ParticlesShader
	{
		struct PushConstant
		{
			float lifetime;
			uint32_t clear;
			uint32_t total_particles;
			uint32_t trail_size;

			uint32_t use_fractional_delta;
			uint32_t sub_emitter_mode;
			uint32_t can_emit;
			uint32_t trail_pass;
		};

		ShaderRD shader;
		ShaderCompiler compiler;

		RID default_shader;
		RID default_material;
		RID default_shader_rd;

		RID base_uniform_set;

		struct CopyPushConstant
		{
			float sort_direction[3];
			uint32_t total_particles;

			uint32_t trail_size;
			uint32_t trail_total;
			float frame_delta;
			float frame_remainder;

			float align_up[3];
			uint32_t align_mode;

			uint32_t lifetime_split;
			uint32_t lifetime_reverse;
			uint32_t motion_vectors_current_offset;

			struct
			{
				uint32_t order_by_lifetime : 1;
				uint32_t copy_mode_2d : 1;
			};

			float inv_emission_transform[12];

			uint32_t align_src;
			uint32_t align_axis;
			uint32_t pad1;
			uint32_t pad2;
		};

		enum
		{
			MAX_USERDATAS = 6
		};

		enum
		{
			COPY_MODE_FILL_INSTANCES,
			COPY_MODE_FILL_SORT_BUFFER,
			COPY_MODE_FILL_INSTANCES_WITH_SORT_BUFFER,
			COPY_MODE_MAX,
		};

		ShaderRD copy_shader;
		RID copy_shader_version;
		PipelineDeferredRD copy_pipelines[MAX_USERDATAS + 1][COPY_MODE_MAX];

		LocalVector<float> pose_update_buffer;

		ParticlesShader() : shader(ParticlesShaderRD()), copy_shader(ParticlesCopyShaderRD()) {}
	};

	static inline ParticlesShader particles_shader;

	static inline SelfList<Particles>::List particle_update_list;

	static inline RID_Owner<Particles, true> particles_owner;

	/* Particle Shader */

	struct ParticlesShaderData : public MaterialStorage::ShaderData
	{
		bool valid = false;
		RID version;
		bool uses_collision = false;

		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size = 0;

		String code;

		PipelineDeferredRD pipeline;

		bool uses_time = false;

		bool userdatas_used[ParticlesShader::MAX_USERDATAS] = {};
		uint32_t userdata_count = 0;

		virtual bool is_animated() const override;
		virtual void set_code(const String& p_Code) override;
		virtual bool casts_shadows() const override;
		virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const override;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const override;

		ParticlesShaderData() {}
		virtual ~ParticlesShaderData() override;
	};

	static MaterialStorage::ShaderData* _create_particles_shader_func();

	struct ParticleProcessMaterialData : public MaterialStorage::MaterialData
	{
		ParticlesShaderData* shader_data = nullptr;
		RID uniform_set;

		virtual void set_render_priority(int p_priority) override {}
		virtual void set_next_pass(RID p_pass) override {}
		virtual ~ParticleProcessMaterialData() override;
	};

	static MaterialStorage::MaterialData* _create_particles_material_func(ParticlesShaderData* p_shader);

	/* Particles Collision */

	struct ParticlesCollision
	{
		RSE::ParticlesCollisionType type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
		uint32_t cull_mask = 0xFFFFFFFF;
		float radius = 1.0;
		Vector3 extents = Vector3(1, 1, 1);
		float attractor_strength = 1.0;
		float attractor_attenuation = 1.0;
		float attractor_directionality = 0.0;
		RID field_texture;
		RID heightfield_texture;
		RID heightfield_fb;
		Size2i heightfield_fb_size;
		uint32_t heightfield_mask = (1 << 20) - 1;

		RSE::ParticlesCollisionHeightfieldResolution heightfield_resolution =
			RSE::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_1024;

		Dependency dependency;
	};

	struct ParticlesCollisionInstance
	{
		RID collision;
		Transform3D transform;
		bool active = false;
	};

	static inline RID_Owner<ParticlesCollision, true> particles_collision_owner;
	static inline RID_Owner<ParticlesCollisionInstance> particles_collision_instance_owner;

public:
	ParticlesStorage() = delete;
	~ParticlesStorage() = delete;
	ParticlesStorage(const ParticlesStorage&) = delete;
	ParticlesStorage& operator=(const ParticlesStorage&) = delete;

	static void initialize();
	static bool is_initialized() { return particles_shader.default_shader.is_valid(); }
	static void finalize();

	static bool free(RID p_rid);

	/* PARTICLES */

	static bool owns_particles(RID p_rid) { return particles_owner.owns(p_rid); }

	static RID particles_allocate();
	static void particles_initialize(RID p_rid);
	static void particles_free(RID p_rid);

	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode);
	static void particles_set_emitting(RID p_particles, bool p_emitting);
	static void particles_set_amount(RID p_particles, int p_amount);
	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio);
	static void particles_set_lifetime(RID p_particles, double p_lifetime);
	static void particles_set_one_shot(RID p_particles, bool p_one_shot);
	static void particles_set_pre_process_time(RID p_particles, double p_time);
	static void particles_request_process_time(
		RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual);
	static void particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio);
	static void particles_set_randomness_ratio(RID p_particles, real_t p_ratio);
	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb);
	static void particles_set_speed_scale(RID p_particles, double p_scale);
	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable);
	static void particles_set_process_material(RID p_particles, RID p_material);
	static RID particles_get_process_material(RID p_particles);

	static void particles_set_fixed_fps(RID p_particles, int p_fps);
	static void particles_set_interpolate(RID p_particles, bool p_enable);
	static void particles_set_fractional_delta(RID p_particles, bool p_enable);
	static void particles_set_collision_base_size(RID p_particles, real_t p_size);

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align);
	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter);
	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis);

	static void particles_set_seed(RID p_particles, uint32_t p_seed);

	static void particles_set_trails(RID p_particles, bool p_enable, double p_length);
	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses);

	static void particles_restart(RID p_particles);
	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags);

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles);

	static void particles_set_draw_order(
		RID p_particles, RSE::ParticlesDrawOrder p_order);

	static void particles_set_draw_passes(RID p_particles, int p_count);
	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh);

	static void particles_request_process(RID p_particles);
	static AABB particles_get_current_aabb(RID p_particles);
	static AABB particles_get_aabb(RID p_particles);

	static void particles_set_emission_transform(
		RID p_particles, const Transform3D& p_transform);
	static void particles_set_emitter_velocity(
		RID p_particles, const Vector3& p_velocity);
	static void particles_set_interp_to_end(RID p_particles, float p_interp_to_end);

	static bool particles_get_emitting(RID p_particles);
	static int particles_get_draw_passes(RID p_particles);
	static RID particles_get_draw_pass_mesh(RID p_particles, int p_pass);

	static void particles_set_view_axis(
		RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis);

	static bool particles_is_inactive(RID p_particles);

	static _FORCE_INLINE_ RSE::ParticlesMode particles_get_mode(RID p_particles)
	{
		Particles* particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, RSE::PARTICLES_MODE_2D);
		return particles->mode;
	}

	static _FORCE_INLINE_ uint32_t particles_get_frame_counter(RID p_particles)
	{
		Particles* particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, false);
		return particles->frame_counter;
	}

	static _FORCE_INLINE_ uint32_t particles_get_amount(RID p_particles, uint32_t& r_trail_divisor)
	{
		Particles* particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, 0);

		if (particles->trails_enabled && particles->trail_bind_poses.size() > 1) {
			r_trail_divisor = particles->trail_bind_poses.size();
		}
		else {
			r_trail_divisor = 1;
		}

		return particles->amount * r_trail_divisor;
	}

	static _FORCE_INLINE_ bool particles_has_collision(RID p_particles)
	{
		Particles* particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, false);

		return particles->has_collision_cache;
	}

	static _FORCE_INLINE_ uint32_t particles_is_using_local_coords(RID p_particles)
	{
		Particles* particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, false);

		return particles->use_local_coords;
	}

	static _FORCE_INLINE_ RID particles_get_instance_buffer_uniform_set(
		RID p_particles, RID p_shader, uint32_t p_set)
	{
		Particles* particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, RID());
		if (particles->particles_transforms_buffer_uniform_set.is_null() ||
			!RD::uniform_set_is_valid(
				particles->particles_transforms_buffer_uniform_set)) {
			_particles_update_buffers(particles);
			Vector<RD::Uniform> uniforms;

			{
				RD::Uniform u;
				u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 0;
				u.append_id(particles->particle_instance_buffer);
				uniforms.push_back(u);
			}

			particles->particles_transforms_buffer_uniform_set =
				RD::uniform_set_create(uniforms, p_shader, p_set);
		}

		return particles->particles_transforms_buffer_uniform_set;
	}

	static void particles_get_instance_buffer_motion_vectors_offsets(
		RID p_particles, uint32_t& r_current_offset, uint32_t& r_prev_offset);

	static void particles_add_collision(
		RID p_particles, RID p_particles_collision_instance);
	static void particles_remove_collision(
		RID p_particles, RID p_particles_collision_instance);
	static void particles_set_canvas_sdf_collision(RID p_particles, bool p_enable,
		const Transform2D& p_xform, const Rect2& p_to_screen, RID p_texture);

	static void update_particles();

	static void particles_update_dependency(RID p_particles, DependencyTracker* p_instance);
	static Dependency* particles_get_dependency(RID p_particles);

	/* Particles Collision */

	static bool owns_particles_collision(RID p_rid) { return particles_collision_owner.owns(p_rid); }

	static RID particles_collision_allocate();
	static void particles_collision_initialize(RID p_particles_collision);
	static void particles_collision_free(RID p_rid);

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type);
	static void particles_collision_set_cull_mask(
		RID p_particles_collision, uint32_t p_cull_mask);
	static void particles_collision_set_sphere_radius(
		RID p_particles_collision, real_t p_radius);
	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents);
	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength);
	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality);
	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve);
	static void particles_collision_set_field_texture(RID p_particles_collision,
		RID p_texture);
	static void particles_collision_height_field_update(
		RID p_particles_collision);
	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution);
	static AABB particles_collision_get_aabb(RID p_particles_collision);
	static Vector3 particles_collision_get_extents(RID p_particles_collision);
	static bool particles_collision_is_heightfield(RID p_particles_collision);
	static RID particles_collision_get_heightfield_framebuffer(RID p_particles_collision);
	static uint32_t particles_collision_get_height_field_mask(
		RID p_particles_collision);
	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask);
	static uint32_t particles_collision_get_cull_mask(RID p_particles_collision);

	static Dependency* particles_collision_get_dependency(RID p_particles);

	static bool owns_particles_collision_instance(RID p_rid)
	{
		return particles_collision_instance_owner.owns(p_rid);
	}

	static RID particles_collision_instance_create(RID p_collision);
	static void particles_collision_instance_free(RID p_rid);
	static void particles_collision_instance_set_transform(
		RID p_collision_instance, const Transform3D& p_transform);
	static void particles_collision_instance_set_active(
		RID p_collision_instance, bool p_active);
};

} // namespace RendererRD
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
// --- Folded from servers/rendering/renderer_rd/storage_rd/utilities.h ---
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererRD
{

/* VISIBILITY NOTIFIER */

struct VisibilityNotifier
{
    AABB aabb;
    Dependency dependency;
};

class Utilities final
{
private:
    /* VISIBILITY NOTIFIER */

    static inline RID_Owner<VisibilityNotifier> visibility_notifier_owner;

    /* MISC */

    // keep cached since it can be called from any thread
    static inline uint64_t texture_mem_cache = 0;
    static inline uint64_t buffer_mem_cache = 0;
    static inline uint64_t total_mem_cache = 0;

public:
    Utilities() = delete;
    Utilities(const Utilities&) = delete;
    Utilities& operator=(const Utilities&) = delete;
    ~Utilities() = delete;

    /* INSTANCES */

    static RSE::InstanceType get_base_type(RID p_rid);
    static bool free(RID p_rid);

    /* DEPENDENCIES */

    static void base_update_dependency(RID p_base, DependencyTracker* p_instance);

    /* VISIBILITY NOTIFIER */

    _FORCE_INLINE_ static VisibilityNotifier* get_visibility_notifier(RID p_rid)
    {
        return visibility_notifier_owner.get_or_null(p_rid);
    }

    _FORCE_INLINE_ static bool owns_visibility_notifier(RID p_rid)
    {
        return visibility_notifier_owner.owns(p_rid);
    }

    static RID visibility_notifier_allocate();
    static void visibility_notifier_initialize(RID p_notifier);
    static void visibility_notifier_free(RID p_notifier);

    static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb);

    static AABB visibility_notifier_get_aabb(RID p_notifier);
    static void visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred);

    /* TIMING */

    static void capture_timestamps_begin();
    static void capture_timestamp(const String& p_name);
    static uint32_t get_captured_timestamps_count();
    static uint64_t get_captured_timestamps_frame();
    static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index);
    static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index);
    static String get_captured_timestamp_name(uint32_t p_index);

    /* MISC */

    static void update_dirty_resources();

    _FORCE_INLINE_ static void set_debug_generate_wireframes(bool p_generate) {}

    static bool has_os_feature(const String& p_feature);

    static void update_memory_info();

    static uint64_t get_rendering_info(RSE::RenderingInfo p_info);

    static String get_video_adapter_name();
    static String get_video_adapter_vendor();
    static RenderingDeviceEnums::DeviceType get_video_adapter_type();
    static String get_video_adapter_api_version();

    static Size2i get_maximum_viewport_size();
    static uint32_t get_maximum_shader_varyings();
    static uint64_t get_maximum_uniform_buffer_size();
};

} // namespace RendererRD

// --- End servers/rendering/renderer_rd/storage_rd/utilities.h ---
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer.h"

class RendererCompositorRD final
{
public:
	enum BlitMode
	{
		BLIT_MODE_NORMAL,
		BLIT_MODE_USE_LAYER,
		BLIT_MODE_LENS,
		BLIT_MODE_NORMAL_ALPHA,
		BLIT_MODE_MAX
	};

	struct BlitPushConstant
	{
		float src_rect[4];
		float dst_rect[4];

		float rotation_sin;
		float rotation_cos;
		float eye_center[2];

		float k1;
		float k2;
		float upscale;
		float aspect_ratio;

		uint32_t layer;
		uint32_t source_is_srgb;
		uint32_t use_debanding;
		uint32_t target_color_space;

		float reference_multiplier;
		float output_max_value;
		uint32_t pad[2];
	};

	struct BlitPipelines
	{
		RID pipelines[BLIT_MODE_MAX];
	};

	struct Blit
	{
		BlitPushConstant push_constant;
		ShaderRD shader{BlitShaderRD()};
		RID shader_version;
		HashMap<RenderingDevice::FramebufferFormatID, BlitPipelines> pipelines_by_format;
		RID index_buffer;
		RID array;
		RID sampler;
	};

private:
	static inline RendererCanvasRenderRD* canvas = nullptr;
	static inline RendererSceneRenderRD* scene = nullptr;

	static inline RendererRD::Fog* fog = nullptr;
	static inline RendererRD::LightStorage* light_storage = nullptr;
	static inline RendererRD::MaterialStorage* material_storage = nullptr;
	static inline RendererRD::MeshStorage* mesh_storage = nullptr;
	static inline RendererRD::ParticlesStorage* particles_storage = nullptr;
	static inline RendererRD::TextureStorage* texture_storage = nullptr;

	static inline Blit* blit = nullptr;

	static inline HashMap<RID, RID> render_target_descriptors;

	static inline double time = 0.0;
	static inline double delta = 0.0;
	static inline uint64_t frame = 0;

	static BlitPipelines _get_blit_pipelines_for_format(
		RenderingDevice::FramebufferFormatID format);
	static float _compute_reference_multiplier(RDC::ColorSpace p_color_space,
		const float p_reference_luminance, const float p_linear_luminance_scale);

public:
	static void bind_canvas_render();

	static void bind_mesh_storage();

	// Inside class RendererCompositorRD public section:
    static void bind_canvas() { RendererCanvasRender::bind<RendererCanvasRenderRD>(); }

	static void bind_texture_storage()
	{
		RendererTextureStorage::bind<RendererRD::TextureStorage>();
	}

	static void bind_material_storage()
	{
		RendererMaterialStorage::bind<RendererRD::MaterialStorage>();
	}

	static void bind_light_storage() { RendererLightStorage::bind<RendererRD::LightStorage>(); }

	static void bind_particles_storage()
	{
		RendererParticlesStorage::bind<RendererRD::ParticlesStorage>();
	}

	static void bind_utilities();

	static void bind_gi() { RendererGI::bind<RendererRD::GI>(); }

	static void bind_fog() { RendererFog::bind<RendererRD::Fog>(); }

	_ALWAYS_INLINE_ static RendererCanvasRender* get_canvas() { return canvas; }

	_ALWAYS_INLINE_ static RendererSceneRenderRD* get_scene() { return scene; }

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter);

	static void initialize();
	static void begin_frame(double frame_step);
	static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
		const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount);

	_ALWAYS_INLINE_ static bool is_opengl() { return false; }

	_ALWAYS_INLINE_ static void gl_end_frame(bool p_swap_buffers) {}

	static void end_frame(bool p_present);
	static void finalize();

	_ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }

	_ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }

	_ALWAYS_INLINE_ static double get_total_time() { return time; }

	_ALWAYS_INLINE_ static bool can_create_resources_async() { return true; }

	static bool is_xr_enabled() { return RendererCompositor::is_xr_enabled(); }

	_ALWAYS_INLINE_ static Error is_viable() { return OK; }

	static Error _create_current()
	{
		RendererCompositorRD::initialize();
		return OK;
	}

	static void make_current()
	{
		RendererCompositor::low_end = false;
		RendererCompositor::bind_compositor<RendererCompositorRD>();
	}

	RendererCompositorRD() = delete;
	RendererCompositorRD(const RendererCompositorRD&) = delete;
	~RendererCompositorRD() = delete;
};

using RCRD = RendererCompositorRD;
