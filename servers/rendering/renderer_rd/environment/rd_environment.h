/**************************************************************************/
/*  rd_environment.h                                                      */
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

#include "core/templates/rid_owner.h"
#include "servers/rendering/renderer_rd/shaders/environment/sky.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/environment/renderer_gi.h"
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
#include "servers/rendering/environment/renderer_fog.h"
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
