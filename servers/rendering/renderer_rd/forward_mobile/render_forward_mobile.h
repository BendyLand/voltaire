/**************************************************************************/
/*  render_forward_mobile.h                                               */
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

#include "servers/rendering/renderer_rd/shaders/forward_mobile/scene_forward_mobile.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "core/templates/paged_allocator.h"
#include "servers/rendering/multi_uma_buffer.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
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
