/**************************************************************************/
/*  material_storage.h                                                    */
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

#pragma once

#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "servers/rendering/renderer_rd/pipeline_cache_rd.h"
#include "servers/rendering/renderer_rd/shader_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/material_storage.h"
#include "servers/rendering/storage/utilities.h"

struct Projection;

namespace RendererRD
{

struct GlobalShaderUniforms
{
	enum
	{
		BUFFER_DIRTY_REGION_SIZE = 1024
	};

	struct Variable
	{
		HashSet<RID> texture_materials;

		RSE::GlobalShaderParameterType type;
		int32_t buffer_index;
		int32_t buffer_elements;
	};

	HashMap<String, Variable> variables;

	struct Value
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct ValueInt
	{
		int32_t x;
		int32_t y;
		int32_t z;
		int32_t w;
	};

	struct ValueUInt
	{
		uint32_t x;
		uint32_t y;
		uint32_t z;
		uint32_t w;
	};

	struct ValueUsage
	{
		uint32_t elements = 0;
	};

	List<RID> materials_using_buffer;
	List<RID> materials_using_texture;

	RID buffer;
	Value* buffer_values = nullptr;
	ValueUsage* buffer_usage = nullptr;
	bool* buffer_dirty_regions = nullptr;
	uint32_t buffer_dirty_region_count = 0;

	uint32_t buffer_size = 0;

	bool must_update_texture_materials = false;
	bool must_update_buffer_materials = false;

	HashMap<RID, int32_t> instance_buffer_pos;
};

struct Samplers
{
	RID rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX][RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX];
	float mipmap_bias = 0.0f;
	bool use_nearest_mipmap_filter = false;
	int anisotropic_filtering_level = 2;

	_FORCE_INLINE_ RID get_sampler(
		RSE::CanvasItemTextureFilter p_filter, RSE::CanvasItemTextureRepeat p_repeat) const
	{
		return rids[p_filter][p_repeat];
	}

	template <typename Collection>
	void append_uniforms(Collection& p_uniforms, int p_first_index) const;
	bool is_valid() const;
	bool is_null() const;
};

class MaterialStorage final
{
public:
	enum ShaderType
	{
		SHADER_TYPE_2D,
		SHADER_TYPE_3D,
		SHADER_TYPE_PARTICLES,
		SHADER_TYPE_SKY,
		SHADER_TYPE_FOG,
		SHADER_TYPE_TEXTURE_BLIT,
		SHADER_TYPE_MAX
	};

	struct ShaderData
	{
		enum BlendMode
		{
			BLEND_MODE_MIX,
			BLEND_MODE_ADD,
			BLEND_MODE_SUB,
			BLEND_MODE_MUL,
			BLEND_MODE_ALPHA_TO_COVERAGE,
			BLEND_MODE_PREMULTIPLIED_ALPHA,
			BLEND_MODE_DISABLED
		};

		String path;
		HashMap<String, ShaderLanguage::ShaderNode::Uniform> uniforms;
		HashMap<String, HashMap<int, RID>> default_texture_params;

		virtual void set_path_hint(const String& p_hint);
		virtual void set_default_texture_parameter(
			const String& p_name, RID p_texture, int p_index);
		virtual void get_instance_param_list(
			List<InstanceShaderParam>* p_param_list) const;
		virtual bool is_parameter_texture(const String& p_param) const;

		virtual void set_code(const String& p_Code) = 0;
		virtual bool is_animated() const = 0;
		virtual bool casts_shadows() const = 0;
		virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const = 0;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const = 0;

		virtual ~ShaderData() {}

		static RDC::PipelineColorBlendState::Attachment blend_mode_to_blend_attachment(
			BlendMode p_mode);
		static bool blend_mode_uses_blend_alpha(BlendMode p_mode);
	};

	struct MaterialData
	{
		Vector<RendererRD::TextureStorage::RenderTarget*> render_target_cache;
		void set_as_used();
		RID get_default_texture_id(
			ShaderLanguage::DataType p_type, ShaderLanguage::ShaderNode::Uniform::Hint p_hint);

		virtual void set_render_priority(int p_priority) = 0;
		virtual void set_next_pass(RID p_pass) = 0;
		virtual ~MaterialData();

		void free_parameters_uniform_set(RID p_uniform_set);

	private:
		friend class MaterialStorage;

		RID self;
		List<RID>::Element* global_buffer_E = nullptr;
		List<RID>::Element* global_texture_E = nullptr;
		uint64_t global_textures_pass = 0;
		HashMap<String, uint64_t> used_global_textures;

		Vector<uint8_t> ubo_data[2];
		RID uniform_buffer[2];
		Vector<RID> texture_cache;
	};

	/* Texture Blit Shader */

	struct TexBlitShaderData : public ShaderData
	{
		bool valid;
		RID version;

		PipelineCacheRD pipelines[4];
		Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

		Vector<uint32_t> ubo_offsets;
		uint32_t ubo_size;

		String code;

		BlendMode blend_mode;

		virtual bool is_animated() const;
		virtual void set_code(const String& p_Code);
		virtual bool casts_shadows() const;
		virtual RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const;
		virtual Pair<ShaderRD*, RID> get_native_shader_and_version() const;

		TexBlitShaderData();
		virtual ~TexBlitShaderData();
	};

	static ShaderData* _create_tex_blit_shader_func();

	static MaterialStorage::ShaderData* _create_tex_blit_shader_funcs()
	{
		return _create_tex_blit_shader_func();
	}

	struct TexBlitMaterialData : public MaterialData
	{
		TexBlitShaderData* shader_data = nullptr;
		RID uniform_set;
		bool uniform_set_updated;

		virtual void set_render_priority(int p_priority) {}

		virtual void set_next_pass(RID p_pass) {}

		virtual ~TexBlitMaterialData();
	};

	static MaterialData* _create_tex_blit_material_func(ShaderData* p_shader);

	static MaterialStorage::MaterialData* _create_tex_blit_material_funcs(
		MaterialStorage::ShaderData* p_shader)
	{
		return _create_tex_blit_material_func(static_cast<TexBlitShaderData*>(p_shader));
	}

private:
	/* Samplers */

	static inline Samplers default_samplers;

	/* Buffers */

	static inline RID quad_index_buffer;
	static inline RID quad_index_array;

	/* GLOBAL SHADER UNIFORM API */

	static inline GlobalShaderUniforms global_shader_uniforms;

	static int32_t _global_shader_uniform_allocate(uint32_t p_elements);
	static void _global_shader_uniform_mark_buffer_dirty(int32_t p_index, int32_t p_elements);

	/* SHADER API */

	struct Material;

	struct Shader
	{
		Mutex* mutex = nullptr;
		ShaderData* data = nullptr;
		String code;
		String path_hint;
		ShaderType type;
		HashMap<String, HashMap<int, RID>> default_texture_parameter;
		HashSet<Material*> owners;
		bool embedded = false;
	};

	typedef ShaderData* (*ShaderDataRequestFunction)();
	static inline ShaderDataRequestFunction shader_data_request_func[SHADER_TYPE_MAX] = {};

	static inline RID_Owner<Shader, true> shader_owner;
	static inline HashSet<RID> embedded_set;
	static inline Mutex embedded_set_mutex;

	static Shader* get_shader(RID p_rid) { return shader_owner.get_or_null(p_rid); }

	/* MATERIAL API */

	typedef MaterialData* (*MaterialDataRequestFunction)(ShaderData*);

	struct Material
	{
		RID self;
		MaterialData* data = nullptr;
		Shader* shader = nullptr;
		ShaderType shader_type = SHADER_TYPE_MAX;
		uint32_t shader_id = 0;
		bool uniform_dirty = false;
		bool texture_dirty = false;
		int32_t priority = 0;
		RID next_pass;
		SelfList<Material> update_element;

		Dependency dependency;

		Material() : update_element(this) {}
	};

	static inline MaterialDataRequestFunction material_data_request_func[SHADER_TYPE_MAX] = {};
	static inline RID_Owner<Material, true> material_owner;

	static Material* get_material(RID p_rid) { return material_owner.get_or_null(p_rid); }

	static inline SelfList<Material>::List material_update_list;
	static inline Mutex material_update_list_mutex;

	static void _material_uniform_set_erased(void* p_material);

public:
	// Lifecycle
	static void initialize();
	static void finalize();

	MaterialStorage() = delete;
	MaterialStorage(const MaterialStorage&) = delete;
	MaterialStorage& operator=(const MaterialStorage&) = delete;
	~MaterialStorage() = delete;

	static bool free(RID p_rid);

	/* Helpers */

	static _FORCE_INLINE_ void store_transform(const Transform3D& p_mtx, float* p_array)
	{
		p_array[0] = p_mtx.basis.rows[0][0];
		p_array[1] = p_mtx.basis.rows[1][0];
		p_array[2] = p_mtx.basis.rows[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.basis.rows[0][1];
		p_array[5] = p_mtx.basis.rows[1][1];
		p_array[6] = p_mtx.basis.rows[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.basis.rows[0][2];
		p_array[9] = p_mtx.basis.rows[1][2];
		p_array[10] = p_mtx.basis.rows[2][2];
		p_array[11] = 0;
		p_array[12] = p_mtx.origin.x;
		p_array[13] = p_mtx.origin.y;
		p_array[14] = p_mtx.origin.z;
		p_array[15] = 1;
	}

	static _FORCE_INLINE_ void store_basis_3x4(const Basis& p_mtx, float* p_array)
	{
		p_array[0] = p_mtx.rows[0][0];
		p_array[1] = p_mtx.rows[1][0];
		p_array[2] = p_mtx.rows[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.rows[0][1];
		p_array[5] = p_mtx.rows[1][1];
		p_array[6] = p_mtx.rows[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.rows[0][2];
		p_array[9] = p_mtx.rows[1][2];
		p_array[10] = p_mtx.rows[2][2];
		p_array[11] = 0;
	}

	static _FORCE_INLINE_ void store_transform_3x3(const Basis& p_mtx, float* p_array)
	{
		p_array[0] = p_mtx.rows[0][0];
		p_array[1] = p_mtx.rows[1][0];
		p_array[2] = p_mtx.rows[2][0];
		p_array[3] = 0;
		p_array[4] = p_mtx.rows[0][1];
		p_array[5] = p_mtx.rows[1][1];
		p_array[6] = p_mtx.rows[2][1];
		p_array[7] = 0;
		p_array[8] = p_mtx.rows[0][2];
		p_array[9] = p_mtx.rows[1][2];
		p_array[10] = p_mtx.rows[2][2];
		p_array[11] = 0;
	}

	static _FORCE_INLINE_ void store_transform_transposed_3x4(
		const Transform3D& p_mtx, float* p_array)
	{
		p_array[0] = p_mtx.basis.rows[0][0];
		p_array[1] = p_mtx.basis.rows[0][1];
		p_array[2] = p_mtx.basis.rows[0][2];
		p_array[3] = p_mtx.origin.x;
		p_array[4] = p_mtx.basis.rows[1][0];
		p_array[5] = p_mtx.basis.rows[1][1];
		p_array[6] = p_mtx.basis.rows[1][2];
		p_array[7] = p_mtx.origin.y;
		p_array[8] = p_mtx.basis.rows[2][0];
		p_array[9] = p_mtx.basis.rows[2][1];
		p_array[10] = p_mtx.basis.rows[2][2];
		p_array[11] = p_mtx.origin.z;
	}

	static _FORCE_INLINE_ void store_camera(const Projection& p_mtx, float* p_array)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_array[i * 4 + j] = p_mtx.columns[i][j];
			}
		}
	}

	static _FORCE_INLINE_ void store_soft_shadow_kernel(const float* p_kernel, float* p_array)
	{
		for (int i = 0; i < 128; i++) {
			p_array[i] = p_kernel[i];
		}
	}

#ifdef REAL_T_IS_DOUBLE
	static _FORCE_INLINE_ void split_double(double a, float* a_hi, float* a_lo)
	{
		const double SPLITTER = (1 << 29) + 1;
		double t = a * SPLITTER;
		double t_hi = t - (t - a);
		double t_lo = a - t_hi;
		*a_hi = (float)t_hi;
		*a_lo = (float)t_lo;
	}
#endif

	/* Samplers */

	static Samplers samplers_rd_allocate(
		float p_mipmap_bias = 0.0f, RSE::ViewportAnisotropicFiltering anisotropic_filtering_level =
										RSE::ViewportAnisotropicFiltering::VIEWPORT_ANISOTROPY_4X);
	static void samplers_rd_free(Samplers& p_samplers);

	static _FORCE_INLINE_ RID sampler_rd_get_default(
		RSE::CanvasItemTextureFilter p_filter, RSE::CanvasItemTextureRepeat p_repeat)
	{
		return default_samplers.get_sampler(p_filter, p_repeat);
	}

	static _FORCE_INLINE_ const Samplers& samplers_rd_get_default() { return default_samplers; }

	/* Buffers */

	static RID get_quad_index_array() { return quad_index_array; }

	/* GLOBAL SHADER UNIFORM API */

	static void _update_global_shader_uniforms();

	static void global_shader_parameter_remove(const String& p_name);
	static Vector<String> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
		const String& p_name);
	static RSE::GlobalShaderParameterType global_shader_parameter_get_type_internal(
		const String& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures = true);
	static void global_shader_parameters_clear();

	static int32_t global_shader_parameters_instance_allocate(RID p_instance);
	static void global_shader_parameters_instance_free(RID p_instance);

	static RID global_shader_uniforms_get_storage_buffer();

	/* SHADER API */

	static bool owns_shader(RID p_rid) { return shader_owner.owns(p_rid); }

	static RID shader_allocate();
	static void shader_initialize(RID p_shader, bool p_embedded = true);
	static void shader_free(RID p_rid);

	static void shader_set_code(RID p_shader, const String& p_code);
	static void shader_set_path_hint(RID p_shader, const String& p_path);
	static String shader_get_code(RID p_shader);

	static void shader_set_default_texture_parameter(
		RID p_shader, const String& p_name, RID p_texture, int p_index);
	static RID shader_get_default_texture_parameter(
		RID p_shader, const String& p_name, int p_index);
	static void shader_set_data_request_function(
		ShaderType p_shader_type, ShaderDataRequestFunction p_function);
	static ShaderData* shader_get_data(RID p_shader);

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader);
	static void shader_embedded_set_lock();
	static const HashSet<RID>& shader_embedded_set_get();
	static void shader_embedded_set_unlock();

	/* MATERIAL API */

	static bool owns_material(RID p_rid) { return material_owner.owns(p_rid); }

	static void _material_queue_update(Material* material, bool p_uniform, bool p_texture);
	static void _update_queued_materials();

	static RID material_allocate();
	static void material_initialize(RID p_material);
	static void material_free(RID p_rid);

	static void material_set_shader(RID p_material, RID p_shader);
	static ShaderData* material_get_shader_data(RID p_material);

	static void material_set_next_pass(RID p_material, RID p_next_material);
	static void material_set_render_priority(RID p_material, int priority);

	static bool material_is_animated(RID p_material);
	static bool material_casts_shadows(RID p_material);
	static RSE::CullMode material_get_cull_mode(RID p_material);

	static void material_get_instance_shader_parameters(
		RID p_material, List<InstanceShaderParam>* r_parameters);

	static void material_update_dependency(RID p_material, DependencyTracker* p_instance);

	static void material_set_data_request_function(
		ShaderType p_shader_type, MaterialDataRequestFunction p_function);
	static MaterialDataRequestFunction material_get_data_request_function(ShaderType p_shader_type);

	static _FORCE_INLINE_ uint32_t material_get_shader_id(RID p_material)
	{
		Material* material = material_owner.get_or_null(p_material);
		return material->shader_id;
	}

	static _FORCE_INLINE_ MaterialData* material_get_data(RID p_material, ShaderType p_shader_type)
	{
		Material* material = material_owner.get_or_null(p_material);
		if (!material || material->shader_type != p_shader_type) {
			return nullptr;
		}
		else {
			return material->data;
		}
	}
};

} // namespace RendererRD


