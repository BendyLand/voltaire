/**************************************************************************/
/*  render_forward_clustered.h                                            */
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

#include "servers/rendering/renderer_rd/shaders/forward_clustered/scene_forward_clustered.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "core/templates/paged_allocator.h"
#include "servers/rendering/multi_uma_buffer.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/effects/ss_effects.h"
#include "servers/rendering/renderer_rd/effects/upscale_effects.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
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
