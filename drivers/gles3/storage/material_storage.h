/**************************************************************************/
/*  material_storage.h                                                    */
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

#ifdef GLES3_ENABLED

#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "drivers/gles3/shaders/canvas.glsl.gen.h"
#include "drivers/gles3/shaders/particles.glsl.gen.h"
#include "drivers/gles3/shaders/scene.glsl.gen.h"
#include "drivers/gles3/shaders/sky.glsl.gen.h"
#include "drivers/gles3/shaders/tex_blit.glsl.gen.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/material_storage.h"
#include "servers/rendering/storage/utilities.h"

namespace GLES3
{

enum
{
	PARTICLES_MAX_USERDATAS = 6
};

struct ShaderData
{
	enum BlendMode
	{ // Used internally.
		BLEND_MODE_MIX,
		BLEND_MODE_ADD,
		BLEND_MODE_SUB,
		BLEND_MODE_MUL,
		BLEND_MODE_PREMULT_ALPHA,
		BLEND_MODE_PMALPHA = BLEND_MODE_PREMULT_ALPHA,
		BLEND_MODE_ALPHA_TO_COVERAGE,
		BLEND_MODE_DISABLED,
		BLEND_MODE_LCD,
	};

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

	enum StencilCompare
	{
		STENCIL_COMPARE_LESS,
		STENCIL_COMPARE_EQUAL,
		STENCIL_COMPARE_LESS_OR_EQUAL,
		STENCIL_COMPARE_GREATER,
		STENCIL_COMPARE_NOT_EQUAL,
		STENCIL_COMPARE_GREATER_OR_EQUAL,
		STENCIL_COMPARE_ALWAYS,
		STENCIL_COMPARE_MAX
	};

	enum StencilFlags
	{
		STENCIL_FLAG_READ = 1,
		STENCIL_FLAG_WRITE = 2,
		STENCIL_FLAG_WRITE_DEPTH_FAIL = 4,
	};

	enum AlphaAntiAliasing
	{
		ALPHA_ANTIALIASING_OFF,
		ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE,
		ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE
	};

	RSE::ShaderMode mode = RSE::SHADER_MAX;
	String path;
	HashMap<String, ShaderLanguage::ShaderNode::Uniform> uniforms;
	HashMap<String, HashMap<int, RID>> default_texture_params;

	bool valid = false;
	RID version;

	Vector<ShaderCompiler::GeneratedCode::Texture> texture_uniforms;

	Vector<uint32_t> ubo_offsets;
	uint32_t ubo_size = 0;

	String code;

	BlendMode blend_mode = BLEND_MODE_MIX;
	AlphaAntiAliasing alpha_antialiasing_mode = ALPHA_ANTIALIASING_OFF;
	DepthDraw depth_draw = DEPTH_DRAW_OPAQUE;
	DepthTest depth_test = DEPTH_TEST_ENABLED;
	RSE::CullMode cull_mode = RSE::CULL_MODE_BACK;

	StencilCompare stencil_compare = STENCIL_COMPARE_ALWAYS;
	uint32_t stencil_flags = 0;
	int32_t stencil_reference = -1;
	bool stencil_enabled = false;

	bool uses_point_size = false;
	bool uses_alpha = false;
	bool compiler_alpha_clip = false;
	bool uses_blend_alpha = false;
	bool uses_depth_prepass_alpha = false;
	bool uses_discard = false;
	bool uses_roughness = false;
	bool uses_normal = false;
	bool uses_particle_trails = false;
	bool wireframe = false;

	bool unshaded = false;
	bool uses_vertex = false;
	bool uses_position = false;
	bool uses_sss = false;
	bool uses_transmittance = false;
	bool uses_screen_texture = false;
	bool uses_screen_texture_mipmaps = false;
	bool uses_depth_texture = false;
	bool uses_normal_texture = false;
	bool uses_bent_normal_texture = false;
	bool uses_sdf = false;
	bool uses_time = false;
	bool uses_vertex_time = false;
	bool uses_fragment_time = false;
	bool uses_half_res = false;
	bool uses_quarter_res = false;
	bool uses_light = false;
	bool uses_collision = false;
	bool writes_modelview_or_projection = false;
	bool uses_world_coordinates = false;
	bool uses_tangent = false;
	bool writes_tangent = false;
	bool uses_color = false;
	bool uses_uv = false;
	bool uses_uv2 = false;
	bool uses_custom0 = false;
	bool uses_custom1 = false;
	bool uses_custom2 = false;
	bool uses_custom3 = false;
	bool uses_bones = false;
	bool uses_weights = false;

	uint64_t vertex_input_mask = 0;

	bool userdatas_used[PARTICLES_MAX_USERDATAS] = {};
	uint32_t userdata_count = 0;

	explicit ShaderData(RSE::ShaderMode p_mode) : mode(p_mode) {}
	~ShaderData();

	void set_path_hint(const String& p_hint);
	void set_default_texture_parameter(const String& p_name, RID p_texture, int p_index);
	void get_instance_param_list(List<InstanceShaderParam>* p_param_list) const;
	bool is_parameter_texture(const String& p_param) const;

	void set_code(const String& p_code);
	bool is_animated() const;
	bool casts_shadows() const;
	RenderingServerTypes::ShaderNativeSourceCode get_native_source_code() const;

private:
	void _set_code_canvas(const String& p_code);
	void _set_code_sky(const String& p_code);
	void _set_code_scene(const String& p_code);
	void _set_code_particles(const String& p_code);
	void _set_code_tex_blit(const String& p_code);
};

struct Material;

struct Shader
{
	ShaderData* data = nullptr;
	String code;
	String path_hint;
	RSE::ShaderMode mode;
	HashMap<String, HashMap<int, RID>> default_texture_parameter;
	HashSet<Material*> owners;
};

struct MaterialData
{
	ShaderData* shader_data = nullptr;
	uint64_t last_pass = 0;
	uint32_t index = 0;
	RID next_pass;
	RID uniform_set;
	uint8_t priority = 0;
	bool uniform_set_updated = false;

	explicit MaterialData(ShaderData* p_shader_data = nullptr) : shader_data(p_shader_data) {}
	~MaterialData();

	RID get_default_texture_id(
		ShaderLanguage::DataType p_type, ShaderLanguage::ShaderNode::Uniform::Hint p_hint);

	void set_render_priority(int p_priority);
	void set_next_pass(RID p_pass);
	void bind_uniforms();

	// Used internally by all Materials
protected:
	Vector<uint8_t> ubo_data;
	GLuint uniform_buffer = GLuint(0);
	Vector<RID> texture_cache;

private:
	friend class MaterialStorage;
	RID self;
	List<RID>::Element* global_buffer_E = nullptr;
	List<RID>::Element* global_texture_E = nullptr;
	uint64_t global_textures_pass = 0;
	HashMap<String, uint64_t> used_global_textures;
};

struct Material
{
	RID self;
	MaterialData* data = nullptr;
	Shader* shader = nullptr;
	// shortcut to shader data and type
	RSE::ShaderMode shader_mode = RSE::SHADER_MAX;
	uint32_t shader_id = 0;
	bool uniform_dirty = false;
	bool texture_dirty = false;
	int32_t priority = 0;
	RID next_pass;
	SelfList<Material> update_element;

	Dependency dependency;

	Material() : update_element(this) {}
};

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

	GLuint buffer = GLuint(0);
	Value* buffer_values = nullptr;
	ValueUsage* buffer_usage = nullptr;
	bool* buffer_dirty_regions = nullptr;
	uint32_t buffer_dirty_region_count = 0;

	uint32_t buffer_size = 0;

	bool must_update_texture_materials = false;
	bool must_update_buffer_materials = false;

	HashMap<RID, int32_t> instance_buffer_pos;
};

class MaterialStorage final
{
private:
	friend struct MaterialData;

	/* GLOBAL SHADER UNIFORM API */

	static inline GlobalShaderUniforms global_shader_uniforms;

	static int32_t _global_shader_uniform_allocate(uint32_t p_elements);
	static void _global_shader_uniform_mark_buffer_dirty(int32_t p_index, int32_t p_elements);

	/* SHADER API */

	static inline RID_Owner<Shader, true> shader_owner;

	/* MATERIAL API */
	static inline RID_Owner<Material, true> material_owner;

	static inline SelfList<Material>::List material_update_list;
	static inline HashSet<RID> dummy_embedded_set;

public:
	struct Shaders
	{
		CanvasShaderGLES3 canvas_shader;
		SkyShaderGLES3 sky_shader;
		SceneShaderGLES3 scene_shader;
		ParticlesShaderGLES3 particles_process_shader;
		TexBlitShaderGLES3 tex_blit_shader;

		ShaderCompiler compiler_canvas;
		ShaderCompiler compiler_scene;
		ShaderCompiler compiler_particles;
		ShaderCompiler compiler_sky;
		ShaderCompiler compiler_tex_blit;
	};

	static inline Shaders* shaders = nullptr;

	MaterialStorage() = delete;
	MaterialStorage(const MaterialStorage&) = delete;
	MaterialStorage& operator=(const MaterialStorage&) = delete;
	~MaterialStorage() = delete;

	static void initialize();
	static void finalize();

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

	static _FORCE_INLINE_ void store_camera(const Projection& p_mtx, float* p_array)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_array[i * 4 + j] = p_mtx.columns[i][j];
			}
		}
	}

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

	static GLuint global_shader_parameters_get_uniform_buffer();

	/* SHADER API */

	static Shader* get_shader(RID p_rid) { return shader_owner.get_or_null(p_rid); }

	static bool owns_shader(RID p_rid) { return shader_owner.owns(p_rid); }

	static void _shader_make_dirty(Shader* p_shader);

	static RID shader_allocate();
	static void shader_initialize(RID p_rid, bool p_embedded = true);
	static void shader_free(RID p_rid);

	static void shader_set_code(RID p_shader, const String& p_code);
	static void shader_set_path_hint(RID p_shader, const String& p_path);
	static String shader_get_code(RID p_shader);

	static void shader_set_default_texture_parameter(
		RID p_shader, const String& p_name, RID p_texture, int p_index);
	static RID shader_get_default_texture_parameter(
		RID p_shader, const String& p_name, int p_index);

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(
		RID p_shader);

	static void shader_embedded_set_lock() {}

	static const HashSet<RID>& shader_embedded_set_get()
	{
		return dummy_embedded_set;
	}

	static void shader_embedded_set_unlock() {}

	/* MATERIAL API */

	static Material* get_material(RID p_rid) { return material_owner.get_or_null(p_rid); }

	static bool owns_material(RID p_rid) { return material_owner.owns(p_rid); }

	static void _material_queue_update(Material* material, bool p_uniform, bool p_texture);
	static void _update_queued_materials();

	static RID material_allocate();
	static void material_initialize(RID p_rid);
	static void material_free(RID p_rid);

	static void material_set_shader(RID p_material, RID p_shader);

	static void material_set_next_pass(RID p_material, RID p_next_material);
	static void material_set_render_priority(RID p_material, int priority);

	static bool material_is_animated(RID p_material);
	static bool material_casts_shadows(RID p_material);
	static RSE::CullMode material_get_cull_mode(RID p_material);

	static void material_get_instance_shader_parameters(
		RID p_material, List<InstanceShaderParam>* r_parameters);

	static void material_update_dependency(RID p_material, DependencyTracker* p_instance);

	static _FORCE_INLINE_ uint32_t material_get_shader_id(RID p_material)
	{
		Material* material = material_owner.get_or_null(p_material);
		return material->shader_id;
	}

	static _FORCE_INLINE_ MaterialData* material_get_data(RID p_material, RSE::ShaderMode p_shader_mode)
	{
		Material* material = material_owner.get_or_null(p_material);
		if (!material || material->shader_mode != p_shader_mode) {
			return nullptr;
		}
		else {
			return material->data;
		}
	}
};

} // namespace GLES3

#endif // GLES3_ENABLED
