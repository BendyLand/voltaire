/**************************************************************************/
/*  rasterizer_gles3.h                                                    */
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

#include "servers/rendering/renderer.h"
#ifdef GLES3_ENABLED
#include "drivers/gles3/storage/utilities.h"
// --- Folded from drivers/gles3/rasterizer_canvas_gles3.h ---
#include "drivers/gles3/storage/texture_storage.h"
#ifdef GLES3_ENABLED

#include "drivers/gles3/shaders/canvas.glsl.gen.h"
#include "drivers/gles3/shaders/canvas_occlusion.glsl.gen.h"
// --- Inlined/Folded from drivers/gles3/storage/material_storage.h ---
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
#include "servers/rendering/renderer.h"
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
#include "drivers/gles3/storage/mesh_storage.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/renderer_viewport.h"

namespace GLES3
{

struct PolygonBuffers
{
	GLuint vertex_buffer = 0;
	GLuint vertex_array = 0;
	GLuint index_buffer = 0;
	int count = 0;
	bool color_disabled = false;
	Color color = Color(1.0, 1.0, 1.0, 1.0);
	HashMap<PolygonID, PolygonBuffers> polygons;
	PolygonID last_id = 0;
};


struct Data
{
	GLuint canvas_quad_vertices;
	GLuint canvas_quad_array;

	GLuint indexed_quad_buffer;
	GLuint indexed_quad_array;

	GLuint particle_quad_vertices;
	GLuint particle_quad_array;

	GLuint ninepatch_vertices;
	GLuint ninepatch_elements;

	RID canvas_shader_default_version;

	uint32_t max_lights_per_render = 256;
	uint32_t max_lights_per_item = 16;
	uint32_t max_instances_per_buffer = 16384;
	uint32_t max_instance_buffer_size = 16384 * 128;
};

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

struct InstanceData
{
	float world[6];
	float color_texture_pixel_size[2];

	union
	{
		// rect
		struct
		{
			float modulation[4];

			union
			{
				float msdf[4];
				float ninepatch_margins[4];
			};

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

struct Batch
{
	// Position in the UBO measured in bytes
	uint32_t start = 0;
	uint32_t instance_count = 0;
	uint32_t instance_buffer_index = 0;

	RID tex;
	RSE::CanvasItemTextureFilter filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX;
	RSE::CanvasItemTextureRepeat repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX;

	GLES3::ShaderData::BlendMode blend_mode = GLES3::ShaderData::BLEND_MODE_MIX;
	Color blend_color = Color(1.0, 1.0, 1.0, 1.0);

	RCR::Item* clip = nullptr;

	RID material;
	GLES3::MaterialData* material_data = nullptr;
	uint64_t vertex_input_mask =
		RSE::ARRAY_FORMAT_VERTEX | RSE::ARRAY_FORMAT_COLOR | RSE::ARRAY_FORMAT_TEX_UV;
	uint64_t specialization = 0;

	const RCR::Item::Command* command = nullptr;
	RCR::Item::Command::Type command_type =
		RCR::Item::Command::TYPE_ANIMATION_SLICE; // Can default to any type that doesn't form a
											 // batch.
	uint32_t primitive_points = 0;

	uint32_t flags = 0;
	uint32_t specular_shininess = 0.0;

	bool lights_disabled = false;
};

struct DataBuffer
{
	Vector<GLuint> instance_buffers;
	GLuint light_ubo = 0;
	GLuint state_ubo = 0;
	uint64_t last_frame_used = -3;
	GLsync fence = GLsync();
};

struct State
{
	LocalVector<DataBuffer> canvas_instance_data_buffers;
	LocalVector<Batch> canvas_instance_batches;
	uint32_t current_data_buffer_index = 0;
	uint32_t current_instance_buffer_index = 0;
	uint32_t current_batch_index = 0;
	uint32_t last_item_index = 0;

	InstanceData* instance_data_array = nullptr;

	LightUniform* light_uniforms = nullptr;

	GLuint shadow_texture = 0;
	GLuint shadow_depth_buffer = 0;
	GLuint shadow_fb = 0;
	int shadow_texture_size = 2048;

	bool using_directional_lights = false;

	RID current_tex;
	RSE::CanvasItemTextureFilter current_filter_mode = RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX;
	RSE::CanvasItemTextureRepeat current_repeat_mode = RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX;

	bool transparent_render_target = false;

	double time = 0.0;

	RSE::CanvasItemTextureFilter default_filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
	RSE::CanvasItemTextureRepeat default_repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
};

};

class RasterizerCanvasGLES3 : public RendererCanvasRender
{
	static _FORCE_INLINE_ void _update_transform_2d_to_mat2x4(
		const Transform2D& p_transform, float* p_mat2x4);
	static _FORCE_INLINE_ void _update_transform_2d_to_mat2x3(
		const Transform2D& p_transform, float* p_mat2x3);

	static _FORCE_INLINE_ void _update_transform_2d_to_mat4(const Transform2D& p_transform, float* p_mat4);
	static _FORCE_INLINE_ void _update_transform_to_mat4(const Transform3D& p_transform, float* p_mat4);

	enum
	{
		INSTANCE_FLAGS_LIGHT_COUNT_SHIFT = 0, // 4 bits for light count.

		INSTANCE_FLAGS_CLIP_RECT_UV = (1 << 4),
		INSTANCE_FLAGS_TRANSPOSE_RECT = (1 << 5),
		INSTANCE_FLAGS_USE_MSDF = (1 << 6),
		INSTANCE_FLAGS_USE_LCD = (1 << 7),

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
		DEFAULT_MAX_LIGHTS_PER_RENDER = 256,
	};

	/******************/
	/**** LIGHTING ****/
	/******************/

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

	struct OccluderPolygon
	{
		RSE::CanvasOccluderPolygonCullMode cull_mode = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;
		int line_point_count = 0;
		GLuint vertex_buffer = 0;
		GLuint vertex_array = 0;
		GLuint index_buffer = 0;

		int sdf_point_count = 0;
		int sdf_index_count = 0;
		GLuint sdf_vertex_buffer = 0;
		GLuint sdf_vertex_array = 0;
		GLuint sdf_index_buffer = 0;
		bool sdf_is_lines = false;
	};

	static inline RID_Owner<OccluderPolygon> occluder_polygon_owner;

	static void _update_shadow_atlas();

	static inline struct ShadowRender
	{
		CanvasOcclusionShaderGLES3 shader;
		RID shader_version;
	} shadow_render;


	static_assert(
		sizeof(GLES3::LightUniform) % 16 == 0, "2D light UBO size must be a multiple of 16 bytes");

public:
	enum
	{
		BASE_UNIFORM_LOCATION = 0,
		GLOBAL_UNIFORM_LOCATION = 1,
		LIGHT_UNIFORM_LOCATION = 2,
		INSTANCE_UNIFORM_LOCATION = 3,
		MATERIAL_UNIFORM_LOCATION = 4,
	};

	struct StateBuffer
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
		uint32_t pad1;
		uint32_t pad2;
	};

	static_assert(
		sizeof(StateBuffer) % 16 == 0, "2D state UBO size must be a multiple of 16 bytes");

	static inline GLES3::PolygonBuffers polygon_buffers;

	static PolygonID request_polygon(const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(), const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), int p_count = -1);
	static void free_polygon(PolygonID p_polygon);

	static_assert(sizeof(GLES3::InstanceData) == 128, "2D instance data struct size must be 128 bytes");

	static inline GLES3::Data data;


	// DataBuffer contains our per-frame data. I.e. the resources that are updated each frame.
	// We track them and ensure that they don't get reused until at least 2 frames have passed
	// to avoid the GPU stalling to wait for a resource to become available.

	static inline GLES3::State state;

	static inline RCR::Item* items[MAX_RENDER_ITEMS];

	static inline RID default_canvas_texture;
	static inline RID default_canvas_group_material;
	static inline RID default_canvas_group_shader;
	static inline RID default_clip_children_material;
	static inline RID default_clip_children_shader;

	typedef void Texture;

	static void canvas_begin(RID p_to_render_target, bool p_to_backbuffer, bool p_backbuffer_has_mipmaps);

	// virtual void draw_window_margins(int *black_margin, RID *black_image);
	static void draw_lens_distortion_rect(const Rect2& p_rect, float p_k1, float p_k2,
		const Vector2& p_eye_center, float p_oversample);

	static void reset_canvas();

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
	static void set_shadow_texture_size(int p_size);

	static bool free(RID p_rid);
	static void update();

	static void _bind_canvas_texture(RID p_texture, RSE::CanvasItemTextureFilter p_base_filter,
		RSE::CanvasItemTextureRepeat p_base_repeat);
	static void _prepare_canvas_texture(RID p_texture, RSE::CanvasItemTextureFilter p_base_filter,
		RSE::CanvasItemTextureRepeat p_base_repeat, uint32_t& r_index, Size2& r_texpixel_size);

	static void canvas_render_items(RID p_to_render_target, RCR::Item* p_item_list, const Color& p_modulate,
		Light* p_light_list, Light* p_directional_list, const Transform2D& p_canvas_transform,
		RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info = nullptr);
	static void _render_items(RID p_to_render_target, int p_item_count,
		const Transform2D& p_canvas_transform_inverse, Light* p_lights, bool& r_sdf_used,
		bool p_to_backbuffer = false, RenderingServerTypes::RenderInfo* r_render_info = nullptr,
		bool p_backbuffer_has_mipmaps = false);
	static void _record_item_commands(const RCR::Item* p_item, RID p_render_target,
		const Transform2D& p_canvas_transform_inverse, RCR::Item*& current_clip,
		GLES3::ShaderData::BlendMode p_blend_mode, Light* p_lights, uint32_t& r_index,
		bool& r_break_batch, bool& r_sdf_used, const Point2& p_repeat_offset);
	static void _render_batch(Light* p_lights, uint32_t p_index,
		RenderingServerTypes::RenderInfo* r_render_info = nullptr);
	static bool _bind_material(GLES3::MaterialData* p_material_data,
		CanvasShaderGLES3::ShaderVariant p_variant, uint64_t p_specialization);
	static void _new_batch(bool& r_batch_broken);
	static void _add_to_batch(uint32_t& r_index, bool& r_batch_broken);
	static void _allocate_instance_data_buffer();
	static void _allocate_instance_buffer();
	static void _enable_attributes(uint32_t p_start, bool p_primitive, uint32_t p_rate = 1);

	static void set_time(double p_time);

	static void finalize();
	static void initialize();

	static void set_debug_redraw(bool p_enabled, double p_time, const Color& p_color)
	{
		if (p_enabled) {
			WARN_PRINT_ONCE("Debug CanvasItem Redraw is not available yet when using the "
							"Compatibility renderer.");
		}
	}

	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source) { return 0; }

	RasterizerCanvasGLES3(GLES3::TextureStorage *p_texture_storage, GLES3::MaterialStorage *p_material_storage) = delete;
    RasterizerCanvasGLES3() = delete;
    RasterizerCanvasGLES3(const RasterizerCanvasGLES3 &) = delete;
    ~RasterizerCanvasGLES3() = delete;
};

#endif // GLES3_ENABLED



// --- End drivers/gles3/rasterizer_canvas_gles3.h ---
// --- Folded from drivers/gles3/rasterizer_scene_gles3.h ---
#ifdef GLES3_ENABLED

#include "core/math/aabb.h"
#include "core/math/projection.h"
#include "core/math/vector3.h"
#include "core/math/vector3.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
// --- Inlined/Folded from drivers/gles3/storage/light_storage.h ---
#ifdef GLES3_ENABLED

#include <platform_gl.h>
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/storage/utilities.h"

namespace GLES3
{

struct DirectionalShadow
{
	GLuint depth = 0;
	GLuint fbo = 0;

	int light_count = 0;
	int size = 0;
	bool use_16_bits = true;
	int current_light = 0;
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
		float farplane;
		float split;
		float bias_scale;
		float shadow_texel_size;
		float range_begin;
		Rect2 atlas_rect;
		Vector2 uv_scale;
	};

	ShadowTransform shadow_transform[6];
	RSE::LightType light_type = RSE::LIGHT_DIRECTIONAL;

	AABB aabb;
	RID self;
	RID light;
	Transform3D transform;

	uint64_t shadow_pass = 0;
	uint64_t last_scene_pass = 0;
	uint64_t last_scene_shadow_pass = 0;
	uint64_t last_pass = 0;
	uint32_t cull_mask = 0;
	uint32_t light_directional_index = 0;

	Rect2 directional_rect;

	HashSet<RID> shadow_atlases;

	int32_t gl_id = -1;
	int32_t shadow_id = -1;

	LightInstance() {}
};

/* REFLECTION PROBE */

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

/* REFLECTION ATLAS */

struct ReflectionAtlas
{
	int count = 0;
	int size = 0;

	int mipmap_count = 1;
	int mipmap_size[8];
	GLuint depth = 0;

	struct Reflection
	{
		RID owner;
		GLuint color = 0;
		GLuint radiance = 0;
		GLuint fbos[7];
	};

	Vector<Reflection> reflections;

	Ref<RenderSceneBuffersGLES3> render_buffers;
};

/* REFLECTION PROBE INSTANCE */

struct ReflectionProbeInstance
{
	RID probe;
	int atlas_index = -1;
	RID atlas;

	bool dirty = true;
	bool rendering = false;
	int processing_layer = 0;

	uint64_t last_pass = 0;
	uint32_t cull_mask = 0;

	Transform3D transform;
};

/* LIGHTMAP */

struct Lightmap
{
	RID light_texture;
	RID shadow_texture;
	bool uses_spherical_harmonics = false;
	bool interior = false;
	AABB bounds = AABB(Vector3(), Vector3(1, 1, 1));
	float baked_exposure = 1.0;
	Vector2i light_texture_size;
	int32_t array_index = -1;
	RSE::ShadowmaskMode shadowmask_mode = RSE::SHADOWMASK_MODE_NONE;
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

	struct ShadowAtlas
	{
		struct Quadrant
		{
			uint32_t subdivision = 0;

			struct Shadow
			{
				RID owner;
				bool owner_is_omni = false;
				uint64_t version = 0;
				uint64_t alloc_tick = 0;

				Shadow() {}
			};

			Vector<Shadow> shadows;
			LocalVector<GLuint> textures;
			LocalVector<GLuint> fbos;

			Quadrant() {}
		} quadrants[4];

		int size_order[4] = {0, 1, 2, 3};
		uint32_t smallest_subdiv = 0;

		int size = 0;
		bool use_16_bits = true;

		GLuint debug_texture = 0;
		GLuint debug_fbo = 0;

		HashMap<RID, uint32_t> shadow_owners;
	};

	/* Static Storage State */
	static inline RID_Owner<Light, true> light_owner;
	static inline RID_Owner<LightInstance> light_instance_owner;
	static inline RID_Owner<ReflectionProbe, true> reflection_probe_owner;
	static inline RID_Owner<ReflectionAtlas> reflection_atlas_owner;
	static inline RID_Owner<ReflectionProbeInstance> reflection_probe_instance_owner;

	static inline float lightmap_probe_capture_update_speed = 4;
	static inline RID_Owner<Lightmap, true> lightmap_owner;
	static inline RID_Owner<LightmapInstance> lightmap_instance_owner;

	static inline uint64_t shadow_atlas_realloc_tolerance_msec = 500;
	static inline RID_Owner<ShadowAtlas> shadow_atlas_owner;
	static inline DirectionalShadow directional_shadow;

	/* Internal static helpers */
	static void _shadow_atlas_invalidate_shadow(ShadowAtlas::Quadrant::Shadow* p_shadow,
		RID p_atlas, ShadowAtlas* p_shadow_atlas, uint32_t p_quadrant, uint32_t p_shadow_idx);
	static bool _shadow_atlas_find_shadow(ShadowAtlas* shadow_atlas, int* p_in_quadrants,
		int p_quadrant_count, int p_current_subdiv, uint64_t p_tick, bool p_omni, int& r_quadrant,
		int& r_shadow);

public:
	LightStorage() = delete;
	LightStorage(const LightStorage&) = delete;
	LightStorage& operator=(const LightStorage&) = delete;
	~LightStorage() = delete;

	static void initialize();

	/* Light API */
	static Light* get_light(RID p_rid) { return light_owner.get_or_null(p_rid); }

	static bool owns_light(RID p_rid) { return light_owner.owns(p_rid); }

	static void _light_initialize(RID p_rid, RSE::LightType p_type);

	static RID directional_light_allocate();
	static void directional_light_initialize(RID p_rid);
	static RID omni_light_allocate();
	static void omni_light_initialize(RID p_rid);
	static RID spot_light_allocate();
	static void spot_light_initialize(RID p_rid);
	static RID area_light_allocate();
	static void area_light_initialize(RID p_rid);

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

	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade) {}

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode);

	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode);
	static void light_directional_set_blend_splits(RID p_light, bool p_enable);
	static bool light_directional_get_blend_splits(RID p_light);
	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode);
	static RSE::LightDirectionalSkyMode light_directional_get_sky_mode(RID p_light);

	static void light_area_set_size(RID p_light, const Vector2& p_size);
	static Vector2 light_area_get_size(RID p_light);

	static void light_area_set_normalize_energy(RID p_light, bool p_enabled);
	static bool light_area_get_normalize_energy(RID p_light);
	static void light_area_set_texture(RID p_light, RID p_texture);
	static RID light_area_get_texture(RID p_light);

	static RSE::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light);
	static RSE::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light);

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

	static uint32_t light_get_max_sdfgi_cascade(RID p_light) { return 0; }

	static uint64_t light_get_version(RID p_light);
	static uint32_t light_get_cull_mask(RID p_light);

	/* LIGHT INSTANCE API */

	static LightInstance* get_light_instance(RID p_rid)
	{
		return light_instance_owner.get_or_null(p_rid);
	}

	static bool owns_light_instance(RID p_rid) { return light_instance_owner.owns(p_rid); }

	static RID light_instance_create(RID p_light);
	static void light_instance_free(RID p_light_instance);

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

	_FORCE_INLINE_ static GLuint light_instance_get_shadow_texture(
		RID p_light_instance, RID p_shadow_atlas)
	{
#ifdef DEBUG_ENABLED
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		ERR_FAIL_COND_V(!li->shadow_atlases.has(p_shadow_atlas), 0);
#endif
		ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_shadow_atlas);
		ERR_FAIL_NULL_V(shadow_atlas, 0);
#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_V(!shadow_atlas->shadow_owners.has(p_light_instance), 0);
#endif
		uint32_t key = shadow_atlas->shadow_owners[p_light_instance];

		uint32_t quadrant = (key >> QUADRANT_SHIFT) & 0x3;
		uint32_t shadow = key & SHADOW_INDEX_MASK;

		ERR_FAIL_COND_V(shadow >= (uint32_t)shadow_atlas->quadrants[quadrant].shadows.size(), 0);

		return shadow_atlas_get_quadrant_shadow_texture(p_shadow_atlas, quadrant, shadow);
	}

	_FORCE_INLINE_ static bool light_instance_has_shadow_atlas(
		RID p_light_instance, RID p_shadow_atlas)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_atlases.has(p_shadow_atlas) : false;
	}

	_FORCE_INLINE_ static float light_instance_get_shadow_texel_size(
		RID p_light_instance, RID p_shadow_atlas)
	{
#ifdef DEBUG_ENABLED
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		ERR_FAIL_COND_V(!li->shadow_atlases.has(p_shadow_atlas), 0);
#endif
		ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_shadow_atlas);
		ERR_FAIL_NULL_V(shadow_atlas, 0);
#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_V(!shadow_atlas->shadow_owners.has(p_light_instance), 0);
#endif
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

	_FORCE_INLINE_ static RSE::LightType light_instance_get_type(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->light_type : RSE::LIGHT_DIRECTIONAL;
	}

	_FORCE_INLINE_ static int32_t light_instance_get_gl_id(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->gl_id : -1;
	}

	_FORCE_INLINE_ static int32_t light_instance_get_shadow_id(RID p_light_instance)
	{
		LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
		return li ? li->shadow_id : -1;
	}

	/* PROBE API */

	static ReflectionProbe* get_reflection_probe(RID p_rid)
	{
		return reflection_probe_owner.get_or_null(p_rid);
	}

	static bool owns_reflection_probe(RID p_rid) { return reflection_probe_owner.owns(p_rid); }

	static RID reflection_probe_allocate();
	static void reflection_probe_initialize(RID p_rid);
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
	static float reflection_probe_get_mesh_lod_threshold(RID p_probe);

	static AABB reflection_probe_get_aabb(RID p_probe);
	static RSE::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe);
	static uint32_t reflection_probe_get_cull_mask(RID p_probe);
	static uint32_t reflection_probe_get_reflection_mask(RID p_probe);
	static Vector3 reflection_probe_get_size(RID p_probe);
	static Vector3 reflection_probe_get_origin_offset(RID p_probe);
	static float reflection_probe_get_origin_max_distance(RID p_probe);
	static bool reflection_probe_renders_shadows(RID p_probe);

	static Dependency* reflection_probe_get_dependency(RID p_probe);

	/* REFLECTION ATLAS */

	static bool owns_reflection_atlas(RID p_rid) { return reflection_atlas_owner.owns(p_rid); }

	static RID reflection_atlas_create();
	static void reflection_atlas_free(RID p_ref_atlas);
	static int reflection_atlas_get_size(RID p_ref_atlas);
	static void reflection_atlas_set_size(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count);

	/* REFLECTION PROBE INSTANCE */

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

	_FORCE_INLINE_ static RID reflection_probe_instance_get_probe(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->probe : RID();
	}

	_FORCE_INLINE_ static RID reflection_probe_instance_get_atlas(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->atlas : RID();
	}

	static Transform3D reflection_probe_instance_get_transform(RID p_instance)
	{
		ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
		return rpi ? rpi->transform : Transform3D();
	}

	static GLuint reflection_probe_instance_get_texture(RID p_instance);
	static GLuint reflection_probe_instance_get_framebuffer(RID p_instance, int p_index);

	/* LIGHTMAP CAPTURE */

	static Lightmap* get_lightmap(RID p_rid) { return lightmap_owner.get_or_null(p_rid); }

	static bool owns_lightmap(RID p_rid) { return lightmap_owner.owns(p_rid); }

	static RID lightmap_allocate();
	static void lightmap_initialize(RID p_rid);
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
	static void lightmap_tap_sh_light(RID p_lightmap, const Vector3& p_point, Color* r_sh);
	static bool lightmap_is_interior(RID p_lightmap);
	static void lightmap_set_probe_capture_update_speed(float p_speed);
	static float lightmap_get_probe_capture_update_speed();

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow);
	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap);
	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode);

	/* LIGHTMAP INSTANCE */

	static LightmapInstance* get_lightmap_instance(RID p_rid)
	{
		return lightmap_instance_owner.get_or_null(p_rid);
	}

	static bool owns_lightmap_instance(RID p_rid) { return lightmap_instance_owner.owns(p_rid); }

	static RID lightmap_instance_create(RID p_lightmap);
	static void lightmap_instance_free(RID p_lightmap);
	static void lightmap_instance_set_transform(RID p_lightmap, const Transform3D& p_transform);

	/* SHADOW ATLAS API */
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

	_FORCE_INLINE_ static int shadow_atlas_get_size(RID p_atlas)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		return atlas ? atlas->size : 0;
	}

	_FORCE_INLINE_ static GLuint shadow_atlas_get_debug_fb(RID p_atlas)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);

		if (atlas->debug_fbo != 0) {
			return atlas->debug_fbo;
		}
		glGenFramebuffers(1, &atlas->debug_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, atlas->debug_fbo);

		if (atlas->debug_texture == 0) {
			atlas->debug_texture = shadow_atlas_get_debug_texture(p_atlas);
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas->debug_texture);

		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, atlas->debug_texture, 0);

		glBindFramebuffer(GL_FRAMEBUFFER, GLES3::TextureStorage::system_fbo);

		return atlas->debug_fbo;
	}

	_FORCE_INLINE_ static GLuint shadow_atlas_get_debug_texture(RID p_atlas)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);

		if (atlas->debug_texture != 0) {
			return atlas->debug_texture;
		}

		glGenTextures(1, &atlas->debug_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, atlas->debug_texture);

		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_R8, atlas->size, atlas->size, 0, GL_RED, GL_UNSIGNED_INT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE);

		glBindTexture(GL_TEXTURE_2D, 0);

		return atlas->debug_texture;
	}

	_FORCE_INLINE_ static int shadow_atlas_get_quadrant_shadows_length(
		RID p_atlas, uint32_t p_quadrant)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		return atlas->quadrants[p_quadrant].shadows.size();
	}

	_FORCE_INLINE_ static uint32_t shadow_atlas_get_quadrant_shadows_allocated(
		RID p_atlas, uint32_t p_quadrant)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		return atlas->quadrants[p_quadrant].textures.size();
	}

	_FORCE_INLINE_ static uint32_t shadow_atlas_get_quadrant_subdivision(
		RID p_atlas, uint32_t p_quadrant)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		return atlas->quadrants[p_quadrant].subdivision;
	}

	_FORCE_INLINE_ static GLuint shadow_atlas_get_quadrant_shadow_texture(
		RID p_atlas, uint32_t p_quadrant, uint32_t p_shadow)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_shadow, atlas->quadrants[p_quadrant].textures.size(), 0);
		return atlas->quadrants[p_quadrant].textures[p_shadow];
	}

	_FORCE_INLINE_ static GLuint shadow_atlas_get_quadrant_shadow_fb(
		RID p_atlas, uint32_t p_quadrant, uint32_t p_shadow)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_shadow, atlas->quadrants[p_quadrant].fbos.size(), 0);
		return atlas->quadrants[p_quadrant].fbos[p_shadow];
	}

	_FORCE_INLINE_ static int shadow_atlas_get_quadrant_shadow_size(
		RID p_atlas, uint32_t p_quadrant)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, 0);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, 0);
		return (atlas->size >> 1) / atlas->quadrants[p_quadrant].subdivision;
	}

	_FORCE_INLINE_ static bool shadow_atlas_get_quadrant_shadow_is_omni(
		RID p_atlas, uint32_t p_quadrant, uint32_t p_shadow)
	{
		ShadowAtlas* atlas = shadow_atlas_owner.get_or_null(p_atlas);
		ERR_FAIL_NULL_V(atlas, false);
		ERR_FAIL_UNSIGNED_INDEX_V(p_quadrant, 4, false);
		ERR_FAIL_UNSIGNED_INDEX_V(
			p_shadow, (uint32_t)atlas->quadrants[p_quadrant].shadows.size(), false);
		return atlas->quadrants[p_quadrant].shadows[p_shadow].owner_is_omni;
	}

	static void shadow_atlas_update(RID p_atlas);

	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true);
	static int get_directional_light_shadow_size(RID p_light_instance);
	static void set_directional_shadow_count(int p_count);

	static Rect2i get_directional_shadow_rect();
	static void update_directional_shadow_atlas();

	_FORCE_INLINE_ static GLuint directional_shadow_get_texture()
	{
		return directional_shadow.depth;
	}

	_FORCE_INLINE_ static int directional_shadow_get_size() { return directional_shadow.size; }

	_FORCE_INLINE_ static GLuint direction_shadow_get_fb() { return directional_shadow.fbo; }

	_FORCE_INLINE_ static void directional_shadow_increase_current_light()
	{
		directional_shadow.current_light++;
	}
};

} // namespace GLES3

#endif // GLES3_ENABLED

#include "drivers/gles3/storage/mesh_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

namespace GLES3
{

class Fog final
{
public:
	Fog() = delete;
	~Fog() = delete;
	Fog(const Fog&) = delete;
	Fog& operator=(const Fog&) = delete;

	static void initialize();
	static void finalize();
	static bool is_initialized();
	static bool free(RID p_rid);

	/* FOG VOLUMES */

	static bool owns_fog_volume(RID p_rid);
	static RID fog_volume_allocate();
	static void fog_volume_initialize(RID p_rid);
	static void fog_volume_free(RID p_rid);
	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape);
	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size);
	static void fog_volume_set_material(RID p_fog_volume, RID p_material);
	static AABB fog_volume_get_aabb(RID p_fog_volume);
	static RSE::FogVolumeShape fog_volume_get_shape(RID p_fog_volume);
};

class GI final
{
public:
	GI() = delete;
	~GI() = delete;
	GI(const GI&) = delete;
	GI& operator=(const GI&) = delete;

	static void initialize();
	static void finalize();
	static bool is_initialized();
	static bool free(RID p_rid);

	/* VOXEL GI STUBS */
	static bool owns_voxel_gi(RID p_rid);
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

	/* SDFGI STUBS */
	static void sdfgi_reset();
};

} // namespace GLES3

class RenderSceneBuffersGLES3;

enum RenderListType
{
	RENDER_LIST_OPAQUE,	   // used for opaque objects
	RENDER_LIST_ALPHA,	   // used for transparent objects
	RENDER_LIST_SECONDARY, // used for shadows and other objects
	RENDER_LIST_MAX
};

enum PassMode
{
	PASS_MODE_COLOR,
	PASS_MODE_COLOR_TRANSPARENT,
	PASS_MODE_SHADOW,
	PASS_MODE_DEPTH,
	PASS_MODE_MATERIAL,
	PASS_MODE_MOTION_VECTORS,
};

// These should share as much as possible with SkyUniform Location
enum SceneUniformLocation
{
	SCENE_TONEMAP_UNIFORM_LOCATION,
	SCENE_GLOBALS_UNIFORM_LOCATION,
	SCENE_DATA_UNIFORM_LOCATION,
	SCENE_MATERIAL_UNIFORM_LOCATION,
	SCENE_EMPTY1, // Unused, put here to avoid conflicts with
				  // SKY_DIRECTIONAL_LIGHT_UNIFORM_LOCATION.
	SCENE_OMNILIGHT_UNIFORM_LOCATION,
	SCENE_SPOTLIGHT_UNIFORM_LOCATION,
	SCENE_AREALIGHT_UNIFORM_LOCATION,
	SCENE_DIRECTIONAL_LIGHT_UNIFORM_LOCATION,
	SCENE_MULTIVIEW_UNIFORM_LOCATION,
	SCENE_POSITIONAL_SHADOW_UNIFORM_LOCATION,
	SCENE_DIRECTIONAL_SHADOW_UNIFORM_LOCATION,
	SCENE_EMPTY2, // Unused, put here to avoid conflicts with SKY_MULTIVIEW_UNIFORM_LOCATION.
	SCENE_PREV_DATA_UNIFORM_LOCATION,
	SCENE_PREV_MULTIVIEW_UNIFORM_LOCATION,
};

enum SkyUniformLocation
{
	SKY_TONEMAP_UNIFORM_LOCATION,
	SKY_GLOBALS_UNIFORM_LOCATION,
	SKY_EMPTY1, // Unused, put here to avoid conflicts with SCENE_DATA_UNIFORM_LOCATION.
	SKY_MATERIAL_UNIFORM_LOCATION,
	SKY_DIRECTIONAL_LIGHT_UNIFORM_LOCATION,
	SKY_EMPTY2, // Unused, put here to avoid conflicts with SCENE_OMNILIGHT_UNIFORM_LOCATION.
	SKY_EMPTY3, // Unused, put here to avoid conflicts with SCENE_SPOTLIGHT_UNIFORM_LOCATION.
	SKY_EMPTY4, // Unused, put here to avoid conflicts with SCENE_AREALIGHT_UNIFORM_LOCATION.
	SKY_EMPTY5, // Unused, put here to avoid conflicts with
				// SCENE_DIRECTIONAL_LIGHT_UNIFORM_LOCATION.
	SKY_EMPTY6, // Unused, put here to avoid conflicts with SCENE_MULTIVIEW_UNIFORM_LOCATION.
	SKY_EMPTY7, // Unused, put here to avoid conflicts with
				// SCENE_POSITIONAL_SHADOW_UNIFORM_LOCATION.
	SKY_EMPTY8, // Unused, put here to avoid conflicts with
				// SCENE_DIRECTIONAL_SHADOW_UNIFORM_LOCATION.
	SKY_MULTIVIEW_UNIFORM_LOCATION,
	SKY_EMPTY9,	 // Unused, put here to avoid conflicts with SCENE_PREV_DATA_UNIFORM_LOCATION.
	SKY_EMPTY10, // Unused, put here to avoid conflicts with SCENE_PREV_MULTIVIEW_UNIFORM_LOCATION.
};

struct RenderDataGLES3
{
	Ref<RenderSceneBuffersGLES3> render_buffers;
	bool transparent_bg = false;
	Rect2i render_region;

	Transform3D cam_transform;
	Transform3D inv_cam_transform;
	Projection cam_projection;
	bool cam_orthogonal = false;
	uint32_t camera_visible_layers = 0xFFFFFFFF;

	// For billboards to cast correct shadows.
	Transform3D main_cam_transform;

	// For stereo rendering
	uint32_t view_count = 1;
	Vector3 view_eye_offset[RendererSceneRender::MAX_RENDER_VIEWS];
	Projection view_projection[RendererSceneRender::MAX_RENDER_VIEWS];

	float z_near = 0.0;
	float z_far = 0.0;

	const PagedArray<RenderGeometryInstance*>* instances = nullptr;
	const PagedArray<RID>* lights = nullptr;
	const PagedArray<RID>* reflection_probes = nullptr;
	RID environment;
	RID camera_attributes;
	RID shadow_atlas;
	RID reflection_probe;
	int reflection_probe_pass = 0;

	float lod_distance_multiplier = 0.0;
	float screen_mesh_lod_threshold = 0.0;

	uint32_t directional_light_count = 0;
	uint32_t directional_shadow_count = 0;

	uint32_t spot_light_count = 0;
	uint32_t omni_light_count = 0;
	uint32_t area_light_count = 0;

	float luminance_multiplier = 1.0;

	RenderingServerTypes::RenderInfo* render_info = nullptr;

	/* Shadow data */
	const RendererSceneRender::RenderShadowData* render_shadows = nullptr;
	int render_shadow_count = 0;
};

class RasterizerCanvasGLES3;

class RasterizerSceneGLES3 : public RendererSceneRender
{
private:
	RSE::ViewportDebugDraw debug_draw = RSE::VIEWPORT_DEBUG_DRAW_DISABLED;
	static inline uint64_t scene_pass = 0;

	template <typename T> struct InstanceSort
	{
		float depth;
		T* instance = nullptr;

		bool operator<(const InstanceSort& p_sort) const { return depth < p_sort.depth; }
	};

	struct SceneGlobals
	{
		RID shader_default_version;
		RID default_material;
		RID default_shader;
		RID overdraw_material;
		RID overdraw_shader;
	} scene_globals;

	GLES3::MaterialData* default_material_data_ptr = nullptr;
	GLES3::MaterialData* overdraw_material_data_ptr = nullptr;

	struct LTC
	{
		RID lut1_texture;
		RID lut2_texture;
	} ltc;

	/* LIGHT INSTANCE */

	struct LightData
	{
		float position[3];
		float inv_radius;

		float direction[3]; // Only used by SpotLight
		float size;

		float color[3];
		float attenuation;

		float inv_spot_attenuation;
		float cos_spot_angle;
		float specular_amount;
		float shadow_opacity;

		float pad[3];
		uint32_t bake_mode;

		float area_width[4]; // 4th is padding
		float area_height[4];
	};

	static_assert(sizeof(LightData) % 16 == 0, "LightData size must be a multiple of 16 bytes");

	struct DirectionalLightData
	{
		float direction[3];
		float energy;

		float color[3];
		float size;

		uint32_t enabled : 1; // For use by SkyShaders
		uint32_t bake_mode : 2;
		float shadow_opacity;
		float specular;
		uint32_t mask;
	};

	static_assert(sizeof(DirectionalLightData) % 16 == 0,
		"DirectionalLightData size must be a multiple of 16 bytes");

	struct ShadowData
	{
		float shadow_matrix[16];

		float light_position[3];
		float shadow_normal_bias;

		float pad[3];
		float shadow_atlas_pixel_size;
	};

	static_assert(sizeof(ShadowData) % 16 == 0, "ShadowData size must be a multiple of 16 bytes");

	struct DirectionalShadowData
	{
		float direction[3];
		float shadow_atlas_pixel_size;
		float shadow_normal_bias[4];
		float shadow_split_offsets[4];
		float shadow_matrices[4][16];
		float fade_from;
		float fade_to;
		uint32_t blend_splits; // Not exposed to the shader.
		uint32_t pad;
	};

	static_assert(sizeof(DirectionalShadowData) % 16 == 0,
		"DirectionalShadowData size must be a multiple of 16 bytes");

	class GeometryInstanceGLES3;

	// Cached data for drawing surfaces
	struct GeometryInstanceSurface
	{
		enum
		{
			FLAG_PASS_DEPTH = 1,
			FLAG_PASS_OPAQUE = 2,
			FLAG_PASS_ALPHA = 4,
			FLAG_PASS_SHADOW = 8,
			FLAG_USES_SHARED_SHADOW_MATERIAL = 128,
			FLAG_USES_SCREEN_TEXTURE = 2048,
			FLAG_USES_DEPTH_TEXTURE = 4096,
			FLAG_USES_NORMAL_TEXTURE = 8192,
			FLAG_USES_DOUBLE_SIDED_SHADOWS = 16384,
			FLAG_USES_STENCIL = 32768,
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
				uint64_t lod_index : 8;
				uint64_t surface_index : 8;
				uint64_t geometry_id : 32;
				uint64_t material_id_low : 16;

				uint64_t material_id_hi : 16;
				uint64_t shader_id : 32;
				uint64_t uses_softshadow : 1;
				uint64_t uses_projector : 1;
				uint64_t uses_forward_gi : 1;
				uint64_t uses_lightmap : 1;
				uint64_t depth_layer : 4;
				uint64_t priority : 8;
			};
		} sort;

		RSE::PrimitiveType primitive = RSE::PRIMITIVE_MAX;
		uint32_t flags = 0;
		uint32_t surface_index = 0;
		uint32_t lod_index = 0;
		uint32_t index_count = 0;
		int32_t light_pass_index = -1;
		bool finished_base_pass = false;

		void* surface = nullptr;
		GLES3::ShaderData* shader = nullptr;
		GLES3::MaterialData* material = nullptr;

		void* surface_shadow = nullptr;
		GLES3::ShaderData* shader_shadow = nullptr;
		GLES3::MaterialData* material_shadow = nullptr;

		GeometryInstanceSurface* next = nullptr;
		GeometryInstanceGLES3* owner = nullptr;
	};

	struct GeometryInstanceLightmapSH
	{
		Color sh[9];
	};

	class GeometryInstanceGLES3 : public RenderGeometryInstance
	{
	public:
		// used during rendering
		bool store_transform_cache = true;

		// Used for generating motion vectors.
		Transform3D prev_transform;
		bool is_prev_transform_stored = false;

		int32_t instance_count = 0;

		bool can_sdfgi = false;
		bool using_projectors = false;
		bool using_softshadows = false;

		struct LightPass
		{
			int32_t light_id = -1;	// Position in the light uniform buffer.
			int32_t shadow_id = -1; // Position in the shadow uniform buffer.
			RID light_instance_rid;
			bool is_omni = false;
		};

		LocalVector<LightPass> light_passes;

		uint32_t paired_omni_light_count = 0;
		uint32_t paired_spot_light_count = 0;
		uint32_t paired_area_light_count = 0;
		LocalVector<RID> paired_omni_lights;
		LocalVector<RID> paired_spot_lights;
		LocalVector<RID> paired_area_lights;
		LocalVector<uint32_t> omni_light_gl_cache;
		LocalVector<uint32_t> spot_light_gl_cache;
		LocalVector<uint32_t> area_light_gl_cache;

		LocalVector<RID> paired_reflection_probes;
		LocalVector<RID> reflection_probe_rid_cache;
		LocalVector<Transform3D> reflection_probes_local_transform_cache;

		RID lightmap_instance;
		Rect2 lightmap_uv_scale;
		uint32_t lightmap_slice_index;
		GeometryInstanceLightmapSH* lightmap_sh = nullptr;

		// Used during setup.
		RasterizerSceneGLES3* scene_owner = nullptr;
		GeometryInstanceSurface* surface_caches = nullptr;
		SelfList<GeometryInstanceGLES3> dirty_list_element;

		GeometryInstanceGLES3() : dirty_list_element(this) {}

		virtual void _mark_dirty() override;
		virtual void set_use_lightmap(RID p_lightmap_instance, const Rect2& p_lightmap_uv_scale,
			int p_lightmap_slice_index) override;
		virtual void set_lightmap_capture(const Color* p_sh9) override;

		virtual void clear_light_instances() override;
		virtual void pair_light_instance(
			const RID p_light_instance, RSE::LightType light_type, uint32_t placement_idx) override;
		virtual void pair_reflection_probe_instances(const RID* p_reflection_probe_instances,
			uint32_t p_reflection_probe_instance_count) override;

		virtual void pair_decal_instances(
			const RID* p_decal_instances, uint32_t p_decal_instance_count) override
		{
		}

		virtual void pair_voxel_gi_instances(
			const RID* p_voxel_gi_instances, uint32_t p_voxel_gi_instance_count) override
		{
		}

		virtual void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) override
		{
		}
	};

	virtual uint32_t get_max_lights_total() override;
	virtual uint32_t get_max_lights_per_mesh() override;

	enum
	{
		INSTANCE_DATA_FLAGS_DYNAMIC = 1 << 3,
		INSTANCE_DATA_FLAGS_NON_UNIFORM_SCALE = 1 << 4,
		INSTANCE_DATA_FLAG_USE_GI_BUFFERS = 1 << 5,
		INSTANCE_DATA_FLAG_USE_LIGHTMAP_CAPTURE = 1 << 7,
		INSTANCE_DATA_FLAG_USE_LIGHTMAP = 1 << 8,
		INSTANCE_DATA_FLAG_USE_SH_LIGHTMAP = 1 << 9,
		INSTANCE_DATA_FLAG_USE_VOXEL_GI = 1 << 10,
		INSTANCE_DATA_FLAG_PARTICLES = 1 << 11,
		INSTANCE_DATA_FLAG_MULTIMESH = 1 << 12,
		INSTANCE_DATA_FLAG_MULTIMESH_FORMAT_2D = 1 << 13,
		INSTANCE_DATA_FLAG_MULTIMESH_HAS_COLOR = 1 << 14,
		INSTANCE_DATA_FLAG_MULTIMESH_HAS_CUSTOM_DATA = 1 << 15,
	};

	static void _geometry_instance_dependency_changed(
		Dependency::DependencyChangedNotification p_notification, DependencyTracker* p_tracker);
	static void _geometry_instance_dependency_deleted(
		const RID& p_dependency, DependencyTracker* p_tracker);

	SelfList<GeometryInstanceGLES3>::List geometry_instance_dirty_list;

	// Use PagedAllocator instead of RID to maximize performance
	PagedAllocator<GeometryInstanceGLES3> geometry_instance_alloc;
	PagedAllocator<GeometryInstanceSurface> geometry_instance_surface_alloc;

	void _geometry_instance_add_surface_with_material(GeometryInstanceGLES3* ginstance,
		uint32_t p_surface, GLES3::MaterialData* p_material, uint32_t p_material_id,
		uint32_t p_shader_id, RID p_mesh);
	void _geometry_instance_add_surface_with_material_chain(GeometryInstanceGLES3* ginstance,
		uint32_t p_surface, GLES3::MaterialData* p_material, RID p_mat_src, RID p_mesh);
	void _geometry_instance_add_surface(
		GeometryInstanceGLES3* ginstance, uint32_t p_surface, RID p_material, RID p_mesh);
	void _geometry_instance_update(RenderGeometryInstance* p_geometry_instance);
	void _update_dirty_geometry_instances();

	struct SceneState
	{
		struct UBO
		{
			float projection_matrix[16];
			float inv_projection_matrix[16];
			float inv_view_matrix[16];
			float view_matrix[16];

			float main_cam_inv_view_matrix[16];

			float viewport_size[2];
			float screen_pixel_size[2];

			float ambient_light_color_energy[4];

			float ambient_color_sky_mix;
			uint32_t directional_shadow_count;
			float emissive_exposure_normalization;
			uint32_t use_ambient_light = 0;

			uint32_t use_ambient_cubemap = 0;
			uint32_t use_reflection_cubemap = 0;
			float fog_aerial_perspective;
			float time;

			float radiance_inverse_xform[12];

			uint32_t directional_light_count;
			float z_far;
			float z_near;
			float IBL_exposure_normalization;

			uint32_t fog_enabled;
			uint32_t fog_mode;
			float fog_density;
			float fog_height;

			float fog_height_density;
			float fog_depth_curve;
			float fog_sun_scatter;
			float fog_depth_begin;

			float fog_light_color[3];
			float fog_depth_end;

			float shadow_bias;
			float luminance_multiplier;
			uint32_t camera_visible_layers;
			bool pancake_shadows;
		};

		static_assert(sizeof(UBO) % 16 == 0, "Scene UBO size must be a multiple of 16 bytes");
		static_assert(sizeof(UBO) < 16384, "Scene UBO size must be 16384 bytes or smaller");

		struct MultiviewUBO
		{
			float projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS][16];
			float inv_projection_matrix_view[RendererSceneRender::MAX_RENDER_VIEWS][16];
			float eye_offset[RendererSceneRender::MAX_RENDER_VIEWS][4];
		};

		static_assert(
			sizeof(MultiviewUBO) % 16 == 0, "Multiview UBO size must be a multiple of 16 bytes");
		static_assert(
			sizeof(MultiviewUBO) < 16384, "MultiviewUBO size must be 16384 bytes or smaller");

		struct TonemapUBO
		{
			float exposure = 1.0;
			int32_t tonemapper = 0;
			int32_t pad = 0;
			int32_t pad2 = 0;
			float tonemapper_params[4] = {0.0, 0.0, 0.0, 0.0};
			float brightness = 1.0;
			float contrast = 1.0;
			float saturation = 1.0;
			int32_t pad3 = 0;
		};

		static_assert(
			sizeof(TonemapUBO) % 16 == 0, "Tonemap UBO size must be a multiple of 16 bytes");

		UBO data;
		UBO prev_data;
		GLuint ubo_buffer = 0;
		GLuint prev_ubo_buffer = 0;
		MultiviewUBO multiview_data;
		MultiviewUBO prev_multiview_data;
		GLuint multiview_buffer = 0;
		GLuint prev_multiview_buffer = 0;
		GLuint tonemap_buffer = 0;

		int prev_data_state = 0; // 0 = Motion vectors not used, 1 = use data (first frame only), 2
								 // = use previous data

		bool used_depth_prepass = false;

		GLES3::ShaderData::BlendMode current_blend_mode =
			GLES3::ShaderData::BLEND_MODE_MIX;
		RSE::CullMode cull_mode = RSE::CULL_MODE_BACK;
		GLenum current_depth_function = GL_GEQUAL;

		bool current_blend_enabled = false;
		bool current_depth_draw_enabled = false;
		bool current_depth_test_enabled = false;
		bool current_scissor_test_enabled = false;

		void reset_gl_state()
		{
			glDisable(GL_BLEND);
			current_blend_enabled = false;

			glDisable(GL_SCISSOR_TEST);
			current_scissor_test_enabled = false;

			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
			cull_mode = RSE::CULL_MODE_BACK;

			glDepthMask(GL_FALSE);
			current_depth_draw_enabled = false;
			glDisable(GL_DEPTH_TEST);
			current_depth_test_enabled = false;

			glDepthFunc(GL_GEQUAL);
			current_depth_function = GL_GEQUAL;

			glDisable(GL_STENCIL_TEST);
			current_stencil_test_enabled = false;
			glStencilMask(255);
			current_stencil_write_mask = 255;
			glStencilFunc(GL_ALWAYS, 0, 255);
			current_stencil_compare = GL_ALWAYS;
			current_stencil_reference = 0;
			current_stencil_compare_mask = 255;
		}

		void set_gl_cull_mode(RSE::CullMode p_mode)
		{
			if (cull_mode != p_mode) {
				if (p_mode == RSE::CULL_MODE_DISABLED) {
					glDisable(GL_CULL_FACE);
				}
				else {
					if (cull_mode == RSE::CULL_MODE_DISABLED) {
						// Last time was disabled, so enable and set proper face.
						glEnable(GL_CULL_FACE);
					}
					glCullFace(p_mode == RSE::CULL_MODE_FRONT ? GL_FRONT : GL_BACK);
				}
				cull_mode = p_mode;
			}
		}

		void enable_gl_blend(bool p_enabled)
		{
			if (current_blend_enabled != p_enabled) {
				if (p_enabled) {
					glEnable(GL_BLEND);
				}
				else {
					glDisable(GL_BLEND);
				}
				current_blend_enabled = p_enabled;
			}
		}

		void enable_gl_scissor_test(bool p_enabled)
		{
			if (current_scissor_test_enabled != p_enabled) {
				if (p_enabled) {
					glEnable(GL_SCISSOR_TEST);
				}
				else {
					glDisable(GL_SCISSOR_TEST);
				}
				current_scissor_test_enabled = p_enabled;
			}
		}

		void enable_gl_depth_draw(bool p_enabled)
		{
			if (current_depth_draw_enabled != p_enabled) {
				glDepthMask(p_enabled ? GL_TRUE : GL_FALSE);
				current_depth_draw_enabled = p_enabled;
			}
		}

		void enable_gl_depth_test(bool p_enabled)
		{
			if (current_depth_test_enabled != p_enabled) {
				if (p_enabled) {
					glEnable(GL_DEPTH_TEST);
				}
				else {
					glDisable(GL_DEPTH_TEST);
				}
				current_depth_test_enabled = p_enabled;
			}
		}

		void set_gl_depth_func(GLenum p_depth_func)
		{
			if (current_depth_function != p_depth_func) {
				glDepthFunc(p_depth_func);
				current_depth_function = p_depth_func;
			}
		}

		void enable_gl_stencil_test(bool p_enabled)
		{
			if (current_stencil_test_enabled != p_enabled) {
				if (p_enabled) {
					glEnable(GL_STENCIL_TEST);
				}
				else {
					glDisable(GL_STENCIL_TEST);
				}
				current_stencil_test_enabled = p_enabled;
			}
		}

		void set_gl_stencil_func(GLenum p_compare, GLint p_reference, GLenum p_compare_mask)
		{
			if (current_stencil_compare != p_compare || current_stencil_reference != p_reference ||
				current_stencil_compare_mask != p_compare_mask) {
				glStencilFunc(p_compare, p_reference, p_compare_mask);
				current_stencil_compare = p_compare;
				current_stencil_reference = p_reference;
				current_stencil_compare_mask = p_compare_mask;
			}
		}

		void set_gl_stencil_write_mask(GLuint p_mask)
		{
			if (current_stencil_write_mask != p_mask) {
				glStencilMask(p_mask);
				current_stencil_write_mask = p_mask;
			}
		}

		void set_gl_stencil_op(GLenum p_op_fail, GLenum p_op_dpfail, GLenum p_op_dppass)
		{
			if (current_stencil_op_fail != p_op_fail || current_stencil_op_dpfail != p_op_dpfail ||
				current_stencil_op_dppass != p_op_dppass) {
				glStencilOp(p_op_fail, p_op_dpfail, p_op_dppass);
				current_stencil_op_fail = p_op_fail;
				current_stencil_op_dpfail = p_op_dpfail;
				current_stencil_op_dppass = p_op_dppass;
			}
		}

		GLenum current_stencil_compare = GL_ALWAYS;
		GLuint current_stencil_compare_mask = 255;
		GLuint current_stencil_write_mask = 255;
		GLint current_stencil_reference = 0;
		GLenum current_stencil_op_fail = GL_KEEP;
		GLenum current_stencil_op_dpfail = GL_KEEP;
		GLenum current_stencil_op_dppass = GL_KEEP;
		bool current_stencil_test_enabled = false;

		bool texscreen_copied = false;
		bool used_screen_texture = false;
		bool used_normal_texture = false;
		bool used_depth_texture = false;
		bool used_opaque_stencil = false;

		LightData* omni_lights = nullptr;
		LightData* spot_lights = nullptr;
		LightData* area_lights = nullptr;
		ShadowData* positional_shadows = nullptr;

		InstanceSort<GLES3::LightInstance>* omni_light_sort;
		InstanceSort<GLES3::LightInstance>* spot_light_sort;
		InstanceSort<GLES3::LightInstance>* area_light_sort;
		GLuint omni_light_buffer = 0;
		GLuint spot_light_buffer = 0;
		GLuint area_light_buffer = 0;
		GLuint positional_shadow_buffer = 0;
		uint32_t omni_light_count = 0;
		uint32_t spot_light_count = 0;
		uint32_t area_light_count = 0;
		RSE::ShadowQuality positional_shadow_quality = RSE::ShadowQuality::SHADOW_QUALITY_SOFT_LOW;

		DirectionalLightData* directional_lights = nullptr;
		GLuint directional_light_buffer = 0;
		DirectionalShadowData* directional_shadows = nullptr;
		GLuint directional_shadow_buffer = 0;
		RSE::ShadowQuality directional_shadow_quality = RSE::ShadowQuality::SHADOW_QUALITY_SOFT_LOW;
	} scene_state;

	struct RenderListParameters
	{
		GeometryInstanceSurface** elements = nullptr;
		int element_count = 0;
		bool reverse_cull = false;
		uint64_t spec_constant_base_flags = 0;
		bool force_wireframe = false;
		Vector2 uv_offset = Vector2(0, 0);

		RenderListParameters(GeometryInstanceSurface** p_elements, int p_element_count,
			bool p_reverse_cull, uint64_t p_spec_constant_base_flags,
			bool p_force_wireframe = false, Vector2 p_uv_offset = Vector2())
		{
			elements = p_elements;
			element_count = p_element_count;
			reverse_cull = p_reverse_cull;
			spec_constant_base_flags = p_spec_constant_base_flags;
			force_wireframe = p_force_wireframe;
			uv_offset = p_uv_offset;
		}
	};

	struct RenderList
	{
		LocalVector<GeometryInstanceSurface*> elements;

		void clear() { elements.clear(); }

		// should eventually be replaced by radix

		struct SortByKey
		{
			_FORCE_INLINE_ bool operator()(
				const GeometryInstanceSurface* A, const GeometryInstanceSurface* B) const
			{
				return (A->sort.sort_key2 == B->sort.sort_key2)
						   ? (A->sort.sort_key1 < B->sort.sort_key1)
						   : (A->sort.sort_key2 < B->sort.sort_key2);
			}
		};

		void sort_by_key()
		{
			SortArray<GeometryInstanceSurface*, SortByKey> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		void sort_by_key_range(uint32_t p_from, uint32_t p_size)
		{
			SortArray<GeometryInstanceSurface*, SortByKey> sorter;
			sorter.sort(elements.ptr() + p_from, p_size);
		}

		struct SortByDepth
		{
			_FORCE_INLINE_ bool operator()(
				const GeometryInstanceSurface* A, const GeometryInstanceSurface* B) const
			{
				return (A->owner->depth < B->owner->depth);
			}
		};

		void sort_by_depth()
		{ // used for shadows

			SortArray<GeometryInstanceSurface*, SortByDepth> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		struct SortByReverseDepthAndPriority
		{
			_FORCE_INLINE_ bool operator()(
				const GeometryInstanceSurface* A, const GeometryInstanceSurface* B) const
			{
				return (A->sort.priority == B->sort.priority)
						   ? (A->owner->depth > B->owner->depth)
						   : (A->sort.priority < B->sort.priority);
			}
		};

		void sort_by_reverse_depth_and_priority()
		{ // used for alpha

			SortArray<GeometryInstanceSurface*, SortByReverseDepthAndPriority> sorter;
			sorter.sort(elements.ptr(), elements.size());
		}

		_FORCE_INLINE_ void add_element(GeometryInstanceSurface* p_element)
		{
			elements.push_back(p_element);
		}
	};

	RenderList render_list[RENDER_LIST_MAX];

	void _update_scene_ubo(GLuint& p_ubo_buffer, GLuint p_index, uint32_t p_size,
		const void* p_source_data, String p_name = "");

	void _setup_lights(const RenderDataGLES3* p_render_data, bool p_using_shadows,
		uint32_t& r_directional_light_count, uint32_t& r_omni_light_count,
		uint32_t& r_spot_light_count, uint32_t& r_area_light_count,
		uint32_t& r_directional_shadow_count);
	void _setup_environment(const RenderDataGLES3* p_render_data, bool p_no_fog,
		const Size2i& p_screen_size, bool p_flip_y, const Color& p_default_bg_color,
		bool p_pancake_shadows, float p_shadow_bias = 0.0);
	void _fill_render_list(RenderListType p_render_list, const RenderDataGLES3* p_render_data,
		PassMode p_pass_mode, bool p_append = false);
	void _render_shadows(
		const RenderDataGLES3* p_render_data, const Size2i& p_viewport_size = Size2i(1, 1));
	void _render_shadow_pass(RID p_light, RID p_shadow_atlas, int p_pass,
		const PagedArray<RenderGeometryInstance*>& p_instances, float p_lod_distance_multiplier = 0,
		float p_screen_mesh_lod_threshold = 0.0,
		RenderingServerTypes::RenderInfo* p_render_info = nullptr,
		const Size2i& p_viewport_size = Size2i(1, 1),
		const Transform3D& p_main_cam_transform = Transform3D());
	void _render_post_processing(const RenderDataGLES3* p_render_data);

	template <PassMode p_pass_mode>
	_FORCE_INLINE_ void _render_list_template(RenderListParameters* p_params,
		const RenderDataGLES3* p_render_data, uint32_t p_from_element, uint32_t p_to_element,
		bool p_alpha_pass = false);

	_FORCE_INLINE_ uint32_t _indices_to_primitives(
		RSE::PrimitiveType p_primitive, uint32_t p_indices) const;

protected:
	double time;
	double time_step = 0;

	bool screen_space_roughness_limiter = false;
	float screen_space_roughness_limiter_amount = 0.25;
	float screen_space_roughness_limiter_limit = 0.18;

	void _render_buffers_debug_draw(
		Ref<RenderSceneBuffersGLES3> p_render_buffers, RID p_shadow_atlas, GLuint p_fbo);

	/* Camera Attributes */

	struct CameraAttributes
	{
		float exposure_multiplier = 1.0;
		float exposure_normalization = 1.0;
	};

	bool use_physical_light_units = false;
	mutable RID_Owner<CameraAttributes, true> camera_attributes_owner;

	/* Environment */

	RSE::EnvironmentSSAOQuality ssao_quality = RSE::ENV_SSAO_QUALITY_MEDIUM;
	bool ssao_half_size = false;
	float ssao_adaptive_target = 0.5;
	int ssao_blur_passes = 2;
	float ssao_fadeout_from = 50.0;
	float ssao_fadeout_to = 300.0;

	bool glow_bicubic_upscale = false;

	bool lightmap_bicubic_upscale = false;

	/* Sky */

	struct SkyGlobals
	{
		float fog_aerial_perspective = 0.0;
		Color fog_light_color;
		float fog_sun_scatter = 0.0;
		bool fog_enabled = false;
		float fog_density = 0.0;
		float z_far = 0.0;
		uint32_t directional_light_count = 0;

		DirectionalLightData* directional_lights = nullptr;
		DirectionalLightData* last_frame_directional_lights = nullptr;
		uint32_t last_frame_directional_light_count = 0;
		GLuint directional_light_buffer = 0;

		RID shader_default_version;
		RID default_material;
		RID default_shader;
		RID fog_material;
		RID fog_shader;
		GLuint screen_triangle = 0;
		GLuint screen_triangle_array = 0;
		uint32_t max_directional_lights = 4;
		uint32_t roughness_layers = 8;
	} sky_globals;

	struct Sky
	{
		// Screen Buffers
		GLuint half_res_pass = 0;
		GLuint half_res_framebuffer = 0;
		GLuint quarter_res_pass = 0;
		GLuint quarter_res_framebuffer = 0;
		Size2i screen_size = Size2i(0, 0);

		// Radiance Cubemap
		GLuint radiance = 0;
		GLuint radiance_framebuffer = 0;
		GLuint raw_radiance = 0;

		RID material;
		GLuint uniform_buffer;

		int radiance_size = 256;
		int mipmap_count = 1;

		RSE::SkyMode mode = RSE::SKY_MODE_AUTOMATIC;
		RSE::SkyMode internal_mode = RSE::SKY_MODE_INCREMENTAL; // When using SKY_MODE_AUTOMATIC,
																// this is the mode used internally.

		// ReflectionData reflection;
		bool reflection_dirty = false;
		bool dirty = false;
		int processing_layer = 0;
		Sky* dirty_list = nullptr;
		float baked_exposure = 1.0;

		// State to track when radiance cubemap needs updating
		GLES3::MaterialData* prev_material = nullptr;
		Vector3 prev_position = Vector3(0.0, 0.0, 0.0);
		float prev_time = 0.0f;
		float prev_fog_aerial_perspective = 0.0;
		Color prev_fog_light_color;
		float prev_fog_sun_scatter = 0.0;
		bool prev_fog_enabled = false;
		float prev_fog_density = 0.0;
		float prev_fog_sky_affect = 0.0;
		float prev_fog_light_energy = 0.0;
	};

	Sky* dirty_sky_list = nullptr;
	mutable RID_Owner<Sky, true> sky_owner;

	GLES3::MaterialData* _get_sky_material_data(RID p_env);
	void _setup_sky(const RenderDataGLES3* p_render_data, const PagedArray<RID>& p_lights,
		const Projection& p_projection, const Transform3D& p_transform, const Size2i p_screen_size);
	void _invalidate_sky(Sky* p_sky);
	void _update_dirty_skys();
	void _update_sky_radiance(RID p_env, const Projection& p_projection,
		const Transform3D& p_transform, float p_sky_energy_multiplier);
	void _draw_sky(RID p_env, const Projection& p_projection, const Transform3D& p_transform,
		float p_sky_energy_multiplier, float p_luminance_multiplier, bool p_use_multiview,
		bool p_flip_y, bool p_apply_color_adjustments_in_post);
	void _free_sky_data(Sky* p_sky);

	// Needed for a single argument calls (material and uv2).
	PagedArrayPool<RenderGeometryInstance*> cull_argument_pool;
	PagedArray<RenderGeometryInstance*> cull_argument;

public:
	RasterizerCanvasGLES3* canvas = nullptr;

	RenderGeometryInstance* geometry_instance_create(RID p_base) override;
	void geometry_instance_free(RenderGeometryInstance* p_geometry_instance) override;

	uint32_t geometry_instance_get_pair_mask() override;

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

	RID sky_allocate() override;
	void sky_initialize(RID p_rid) override;
	void sky_set_radiance_size(RID p_sky, int p_radiance_size) override;
	void sky_set_mode(RID p_sky, RSE::SkyMode p_mode) override;
	void sky_set_material(RID p_sky, RID p_material) override;
	Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size) override;
	float sky_get_baked_exposure(RID p_sky) const;

	/* ENVIRONMENT API */

	void environment_glow_set_use_bicubic_upscale(bool p_enable) override;

	void environment_set_ssr_half_size(bool p_half_size) override;
	void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality) override;

	void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to) override;

	void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to) override;

	void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count) override;
	void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames) override;
	void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update) override;

	void environment_set_volumetric_fog_volume_size(int p_size, int p_depth) override;
	void environment_set_volumetric_fog_filter_active(bool p_enable) override;

	Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size) override;

	_FORCE_INLINE_ bool is_using_physical_light_units() { return use_physical_light_units; }

	void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality) override;
	void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality) override;

	RID fog_volume_instance_create(RID p_fog_volume) override;
	void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform) override;
	void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active) override;
	RID fog_volume_instance_get_volume(RID p_fog_volume_instance) const override;
	Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance) const override;

	RID voxel_gi_instance_create(RID p_voxel_gi) override;
	void voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform) override;
	bool voxel_gi_needs_update(RID p_probe) const override;
	void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects) override;

	void voxel_gi_set_quality(RSE::VoxelGIQuality) override;

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
		RenderingServerTypes::RenderInfo* r_render_info = nullptr) override;
	void render_material(const Transform3D& p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region) override;
	void render_particle_collider_heightfield(RID p_collider, const Transform3D& p_transform,
		const PagedArray<RenderGeometryInstance*>& p_instances) override;

	void set_scene_pass(uint64_t p_pass) override { scene_pass = p_pass; }

	static _FORCE_INLINE_ uint64_t get_scene_pass() { return scene_pass; }

	void set_time(double p_time, double p_step) override;
	void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw) override;

	_FORCE_INLINE_ RSE::ViewportDebugDraw get_debug_draw_mode() const { return debug_draw; }

	Ref<RenderSceneBuffers> render_buffers_create() override;
	void gi_set_use_half_resolution(bool p_enable) override;

	void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_curve) override;
	bool screen_space_roughness_limiter_is_active() const;

	void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality) override;
	void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale) override;

	void _render_uv2(const PagedArray<RenderGeometryInstance*>& p_instances, GLuint p_framebuffer,
		const Rect2i& p_region);

	bool free(RID p_rid) override;
	void update() override;
	void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir) override;

	void decals_set_filter(RSE::DecalFilter p_filter) override;
	void light_projectors_set_filter(RSE::LightProjectorFilter p_filter) override;
	virtual void lightmaps_set_bicubic_filter(bool p_enable) override;
	virtual void material_set_use_debanding(bool p_enable) override;

	RasterizerSceneGLES3(GLES3::TextureStorage* p_texture_storage,
		GLES3::MaterialStorage* p_material_storage, GLES3::LightStorage* p_light_storage);
	~RasterizerSceneGLES3();
};

#endif // GLES3_ENABLED



// --- End drivers/gles3/rasterizer_scene_gles3.h ---


#include "drivers/gles3/storage/mesh_storage.h"
// --- Inlined/Folded from drivers/gles3/storage/particles_storage.h ---
#ifdef GLES3_ENABLED

#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "drivers/gles3/shaders/particles_copy.glsl.gen.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/storage/utilities.h"

#include <platform_gl.h>

namespace GLES3 {

enum ParticlesUniformLocation {
	PARTICLES_FRAME_UNIFORM_LOCATION,
	PARTICLES_GLOBALS_UNIFORM_LOCATION,
	PARTICLES_MATERIAL_UNIFORM_LOCATION,
};

class ParticlesStorage final {
private:
	static inline bool _initialized = false;

	/* PARTICLES */

	struct ParticleInstanceData3D {
		float xform[12];
		float color[2]; // Color and custom are packed together into one vec4;
		float custom[2];
	};

	struct ParticleInstanceData2D {
		float xform[8];
		float color[2]; // Color and custom are packed together into one vec4;
		float custom[2];
	};

	struct ParticlesViewSort {
		Vector3 z_dir;
		bool operator()(const ParticleInstanceData3D &p_a, const ParticleInstanceData3D &p_b) const {
			return z_dir.dot(Vector3(p_a.xform[3], p_a.xform[7], p_a.xform[11])) < z_dir.dot(Vector3(p_b.xform[3], p_b.xform[7], p_b.xform[11]));
		}
	};

	struct ParticlesFrameParams {
		enum {
			MAX_ATTRACTORS = 32,
			MAX_COLLIDERS = 32,
			MAX_3D_TEXTURES = 0 // GLES3 renderer doesn't support using 3D textures for flow field or collisions.
		};

		enum AttractorType {
			ATTRACTOR_TYPE_SPHERE,
			ATTRACTOR_TYPE_BOX,
			ATTRACTOR_TYPE_VECTOR_FIELD,
		};

		struct Attractor {
			float transform[16];
			float extents[4]; // Extents or radius. w-channel is padding.

			uint32_t type;
			float strength;
			float attenuation;
			float directionality;
		};

		enum CollisionType {
			COLLISION_TYPE_SPHERE,
			COLLISION_TYPE_BOX,
			COLLISION_TYPE_SDF,
			COLLISION_TYPE_HEIGHT_FIELD,
			COLLISION_TYPE_2D_SDF,
		};

		struct Collider {
			float transform[16];
			float extents[4]; // Extents or radius. w-channel is padding.

			uint32_t type;
			float scale;
			float pad0;
			float pad1;
		};

		uint32_t emitting;
		uint32_t cycle;
		float system_phase;
		float prev_system_phase;

		float explosiveness;
		float randomness;
		float time;
		float delta;

		float particle_size;
		float amount_ratio;
		float pad1;
		float pad2;

		uint32_t random_seed;
		uint32_t attractor_count;
		uint32_t collider_count;
		uint32_t frame;

		float emission_transform[16];
		float emitter_velocity[3];
		float interp_to_end;

		Attractor attractors[MAX_ATTRACTORS];
		Collider colliders[MAX_COLLIDERS];
	};

	static_assert(sizeof(ParticlesFrameParams) % 16 == 0, "ParticlesFrameParams size must be a multiple of 16 bytes");
	static_assert(sizeof(ParticlesFrameParams) < 16384, "ParticlesFrameParams must be 16384 bytes or smaller");

	struct Particles {
		RSE::ParticlesMode mode = RSE::PARTICLES_MODE_3D;
		bool inactive = true;
		double inactive_time = 0.0;
		bool emitting = false;
		bool one_shot = false;
		float amount_ratio = 1.0;
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
		GLuint sdf_collision_texture = 0;

		RID process_material;
		uint32_t frame_counter = 0;

		RSE::ParticlesTransformAlign transform_align = RSE::PARTICLES_TRANSFORM_ALIGN_DISABLED;
		RSE::ParticlesTransformAlignAxis transform_align_axis = RSE::PARTICLES_ALIGN_AXIS_Y;
		RSE::ParticlesTransformAlignCustomSrc transform_align_channel_filter = RSE::PARTICLES_ALIGN_CHANNEL_FILTER_X;
		RSE::ParticlesDrawOrder draw_order = RSE::PARTICLES_DRAW_ORDER_INDEX;

		Vector<RID> draw_passes;

		GLuint frame_params_ubo = 0;

		// We may process particles multiple times each frame (if they have a fixed FPS higher than the game FPS).
		// Unfortunately, this means we can't just use a round-robin system of 3 buffers.
		// To ensure the sort buffer is accurate, we copy the last frame instance buffer just before processing.

		// Transform Feedback buffer and VAO for rendering.
		// Each frame we render to this one.
		GLuint front_vertex_array = 0; // Binds process buffer. Used for processing.
		GLuint front_process_buffer = 0; // Transform + color + custom data + userdata + velocity + flags. Only needed for processing.
		GLuint front_instance_buffer = 0; // Transform + color + custom data. In packed format needed for rendering.

		// VAO for transform feedback, contains last frame's data.
		// Read from this one for particles process and then copy to last frame buffer.
		GLuint back_vertex_array = 0; // Binds process buffer. Used for processing.
		GLuint back_process_buffer = 0; // Transform + color + custom data + userdata + velocity + flags. Only needed for processing.
		GLuint back_instance_buffer = 0; // Transform + color + custom data. In packed format needed for rendering.

		uint64_t last_change = 0;

		uint32_t instance_buffer_size_cache = 0;
		uint32_t instance_buffer_stride_cache = 0;
		uint32_t num_attrib_arrays_cache = 0;
		uint32_t process_buffer_stride_cache = 0;

		// Only ever copied to, holds last frame's instance data, then swaps with sort_buffer.
		GLuint last_frame_buffer = 0;
		bool last_frame_buffer_filled = false;
		float last_frame_phase = 0.0;

		// The frame-before-last's instance buffer.
		// Use this to copy data back for sorting or computing AABB.
		GLuint sort_buffer = 0;
		bool sort_buffer_filled = false;
		float sort_buffer_phase = 0.0;

		uint32_t userdata_count = 0;

		bool dirty = false;
		SelfList<Particles> update_list;

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

		bool clear = true;

		Transform3D emission_transform;
		Vector3 emitter_velocity;
		float interp_to_end = 0.0;

		HashSet<RID> collisions;

		Dependency dependency;

		double trail_length = 1.0;
		bool trails_enabled = false;

		Particles() :
				update_list(this) {
			random_seed = Math::rand();
		}
	};

	static void _particles_process(Particles *p_particles, double p_delta);
	static void _particles_free_data(Particles *particles);
	static void _particles_update_buffers(Particles *particles);
	static void _particles_allocate_history_buffers(Particles *particles);
	static void _particles_update_instance_buffer(Particles *particles, const Vector3 &p_axis, const Vector3 &p_up_axis);

	template <typename T>
	static void _particles_reverse_lifetime_sort(Particles *particles);

	struct ParticlesShader {
		RID default_shader;
		RID default_material;
		RID default_shader_version;

		ParticlesCopyShaderGLES3 copy_shader;
		RID copy_shader_version;
	};

	static inline ParticlesShader particles_shader;

	static inline SelfList<Particles>::List particle_update_list;

	static inline RID_Owner<Particles, true> particles_owner;

	/* Particles Collision */

	struct ParticlesCollision {
		RSE::ParticlesCollisionType type = RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT;
		uint32_t cull_mask = 0xFFFFFFFF;
		float radius = 1.0;
		Vector3 extents = Vector3(1, 1, 1);
		float attractor_strength = 1.0;
		float attractor_attenuation = 1.0;
		float attractor_directionality = 0.0;
		GLuint field_texture = 0;
		GLuint heightfield_texture = 0;
		GLuint heightfield_fb = 0;
		Size2i heightfield_fb_size;
		uint32_t heightfield_mask = (1 << 20) - 1;

		RSE::ParticlesCollisionHeightfieldResolution heightfield_resolution = RSE::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_1024;

		Dependency dependency;
	};

	struct ParticlesCollisionInstance {
		RID collision;
		Transform3D transform;
		bool active = false;
	};

	static inline RID_Owner<ParticlesCollision, true> particles_collision_owner;
	static inline RID_Owner<ParticlesCollisionInstance> particles_collision_instance_owner;

public:
	ParticlesStorage() = delete;
	~ParticlesStorage() = delete;
	ParticlesStorage(const ParticlesStorage &) = delete;
	ParticlesStorage &operator=(const ParticlesStorage &) = delete;

	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static bool free(RID p_rid);

	/* PARTICLES */

	static bool owns_particles(RID p_rid) { return particles_owner.owns(p_rid); }

	static RID particles_allocate();
	static void particles_initialize(RID p_rid);
	static void particles_free(RID p_rid);

	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode);
	static void particles_emit(RID p_particles, const Transform3D &p_transform, const Vector3 &p_velocity, const Color &p_color, const Color &p_custom, uint32_t p_emit_flags);
	static void particles_set_emitting(RID p_particles, bool p_emitting);
	static void particles_set_amount(RID p_particles, int p_amount);
	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio);
	static void particles_set_lifetime(RID p_particles, double p_lifetime);
	static void particles_set_one_shot(RID p_particles, bool p_one_shot);
	static void particles_set_pre_process_time(RID p_particles, double p_time);
	static void particles_request_process_time(RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual);
	static void particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio);
	static void particles_set_randomness_ratio(RID p_particles, real_t p_ratio);
	static void particles_set_custom_aabb(RID p_particles, const AABB &p_aabb);
	static void particles_set_speed_scale(RID p_particles, double p_scale);
	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable);
	static void particles_set_process_material(RID p_particles, RID p_material);
	static RID particles_get_process_material(RID p_particles);
	static void particles_set_fixed_fps(RID p_particles, int p_fps);
	static void particles_set_interpolate(RID p_particles, bool p_enable);
	static void particles_set_fractional_delta(RID p_particles, bool p_enable);
	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles);
	static void particles_set_view_axis(RID p_particles, const Vector3 &p_axis, const Vector3 &p_up_axis);
	static void particles_set_collision_base_size(RID p_particles, real_t p_size);

	static void particles_set_transform_align(RID p_particles, RSE::ParticlesTransformAlign p_transform_align);
	static void particles_set_transform_align_channel_filter(RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel_filter);
	static void particles_set_transform_align_axis(RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis);

	static void particles_set_seed(RID p_particles, uint32_t p_seed);

	static void particles_set_trails(RID p_particles, bool p_enable, double p_length);
	static void particles_set_trail_bind_poses(RID p_particles, const Vector<Transform3D> &p_bind_poses);

	static void particles_restart(RID p_particles);

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order);

	static void particles_set_draw_passes(RID p_particles, int p_count);
	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh);

	static void particles_request_process(RID p_particles);
	static AABB particles_get_current_aabb(RID p_particles);
	static AABB particles_get_aabb(RID p_particles);

	static void particles_set_emission_transform(RID p_particles, const Transform3D &p_transform);
	static void particles_set_emitter_velocity(RID p_particles, const Vector3 &p_velocity);
	static void particles_set_interp_to_end(RID p_particles, float p_interp);

	static bool particles_get_emitting(RID p_particles);
	static int particles_get_draw_passes(RID p_particles);
	static RID particles_get_draw_pass_mesh(RID p_particles, int p_pass);

	static void particles_add_collision(RID p_particles, RID p_instance);
	static void particles_remove_collision(RID p_particles, RID p_instance);

	static void particles_set_canvas_sdf_collision(RID p_particles, bool p_enable, const Transform2D &p_xform, const Rect2 &p_to_screen, GLuint p_texture);

	static void update_particles();
	static bool particles_is_inactive(RID p_particles);

	static _FORCE_INLINE_ RSE::ParticlesMode particles_get_mode(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, RSE::PARTICLES_MODE_2D);
		return particles->mode;
	}

	static _FORCE_INLINE_ uint32_t particles_get_amount(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, 0);

		return particles->amount;
	}

	static _FORCE_INLINE_ GLuint particles_get_gl_buffer(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		if (particles == nullptr) {
			return 0;
		}

		if ((particles->draw_order == RSE::PARTICLES_DRAW_ORDER_VIEW_DEPTH || particles->draw_order == RSE::PARTICLES_DRAW_ORDER_REVERSE_LIFETIME) && particles->sort_buffer_filled) {
			return particles->sort_buffer;
		}
		return particles->back_instance_buffer;
	}

	static _FORCE_INLINE_ GLuint particles_get_prev_gl_buffer(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, 0);

		return particles->front_instance_buffer;
	}

	static _FORCE_INLINE_ uint64_t particles_get_last_change(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, 0);

		return particles->last_change;
	}

	static _FORCE_INLINE_ bool particles_has_collision(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, false);

		return particles->has_collision_cache;
	}

	static _FORCE_INLINE_ uint32_t particles_is_using_local_coords(RID p_particles) {
		Particles *particles = particles_owner.get_or_null(p_particles);
		ERR_FAIL_NULL_V(particles, false);

		return particles->use_local_coords;
	}

	static Dependency *particles_get_dependency(RID p_particles);

	/* PARTICLES COLLISION */
	static bool owns_particles_collision(RID p_rid) { return particles_collision_owner.owns(p_rid); }

	static RID particles_collision_allocate();
	static void particles_collision_initialize(RID p_rid);
	static void particles_collision_free(RID p_rid);

	static void particles_collision_set_collision_type(RID p_particles_collision, RSE::ParticlesCollisionType p_type);
	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask);
	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius);
	static void particles_collision_set_box_extents(RID p_particles_collision, const Vector3 &p_extents);
	static void particles_collision_set_attractor_strength(RID p_particles_collision, real_t p_strength);
	static void particles_collision_set_attractor_directionality(RID p_particles_collision, real_t p_directionality);
	static void particles_collision_set_attractor_attenuation(RID p_particles_collision, real_t p_curve);
	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture);
	static void particles_collision_height_field_update(RID p_particles_collision);
	static void particles_collision_set_height_field_resolution(RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution);
	static AABB particles_collision_get_aabb(RID p_particles_collision);
	static Vector3 particles_collision_get_extents(RID p_particles_collision);
	static bool particles_collision_is_heightfield(RID p_particles_collision);
	static GLuint particles_collision_get_heightfield_framebuffer(RID p_particles_collision);
	static uint32_t particles_collision_get_height_field_mask(RID p_particles_collision);
	static void particles_collision_set_height_field_mask(RID p_particles_collision, uint32_t p_heightfield_mask);
	static uint32_t particles_collision_get_cull_mask(RID p_particles_collision);

	static _FORCE_INLINE_ Size2i particles_collision_get_heightfield_size(RID p_particles_collision) {
		ParticlesCollision *particles_collision = particles_collision_owner.get_or_null(p_particles_collision);
		ERR_FAIL_NULL_V(particles_collision, Size2i());
		ERR_FAIL_COND_V(particles_collision->type != RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE, Size2i());

		return particles_collision->heightfield_fb_size;
	}

	static Dependency *particles_collision_get_dependency(RID p_particles);

	/* PARTICLES COLLISION INSTANCE */
	static bool owns_particles_collision_instance(RID p_rid) { return particles_collision_instance_owner.owns(p_rid); }

	static RID particles_collision_instance_create(RID p_collision);
	static void particles_collision_instance_free(RID p_rid);
	static void particles_collision_instance_set_transform(RID p_collision_instance, const Transform3D &p_transform);
	static void particles_collision_instance_set_active(RID p_collision_instance, bool p_active);
};

} // namespace GLES3

#endif // GLES3_ENABLED
#include "drivers/gles3/storage/texture_storage.h"
#include "drivers/gles3/storage/utilities.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer.h"

class RasterizerGLES3 final
{
private:
	static inline uint64_t frame = 1;
	static inline double delta = 0.0;
	static inline double time_total = 0.0;

#ifdef WINDOWS_ENABLED
	static inline bool screen_flipped_y = false;
#endif

	static inline GLES3::TextureStorage* texture_storage = nullptr;
	static inline GLES3::MaterialStorage* material_storage = nullptr;
	static inline GLES3::ParticlesStorage* particles_storage = nullptr;
	static inline GLES3::LightStorage* light_storage = nullptr;
	static inline RasterizerCanvasGLES3* canvas = nullptr;
	static inline RasterizerSceneGLES3* scene = nullptr;

	static void _blit_render_target_to_screen(DisplayServerEnums::WindowID p_screen,
		const RenderingServerTypes::BlitToScreen& p_blit, bool p_first = true);

public:
	static inline void bind_utilities();
	static inline void bind_mesh_storage();

    static inline void bind_canvas() { RendererCanvasRender::bind<RasterizerCanvasGLES3>(); }
	static void bind_gi() { RendererGI::bind<GLES3::GI>(); }
	static void bind_fog() { RendererFog::bind<GLES3::Fog>(); }

	static _ALWAYS_INLINE_ RendererSceneRender* get_scene() { return scene; }

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true);

	static void initialize();
	static void begin_frame(double frame_step);

	static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
		const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount);

	static _ALWAYS_INLINE_ bool is_opengl() { return true; }

	static void gl_end_frame(bool p_swap_buffers);
	static void end_frame(bool p_swap_buffers);

	static void finalize();

	static void make_current(bool p_gles_over_gl);

#ifdef WINDOWS_ENABLED
	static _ALWAYS_INLINE_ void set_screen_flipped_y(bool p_flipped)
	{
		screen_flipped_y = p_flipped;
	}
#endif

	static _ALWAYS_INLINE_ uint64_t get_frame_number() { return frame; }

	static _ALWAYS_INLINE_ double get_frame_delta_time() { return delta; }

	static _ALWAYS_INLINE_ double get_total_time() { return time_total; }

	static _ALWAYS_INLINE_ bool can_create_resources_async() { return false; }

	static void bind_texture_storage() { RendererTextureStorage::bind<GLES3::TextureStorage>(); }

	static void bind_material_storage() { RendererMaterialStorage::bind<GLES3::MaterialStorage>(); }

	static void bind_light_storage() { RendererLightStorage::bind<GLES3::LightStorage>(); }

	static void bind_particles_storage()
	{
		// GLES3 does not implement GPU particles storage; leave empty or stub if unused
	}

	RasterizerGLES3() = delete;
	RasterizerGLES3(const RasterizerGLES3&) = delete;
	~RasterizerGLES3() = delete;
};

using RGLES3 = RasterizerGLES3;

#endif // GLES3_ENABLED
