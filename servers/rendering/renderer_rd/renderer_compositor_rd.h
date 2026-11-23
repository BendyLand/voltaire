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
#include "servers/rendering/renderer_rd/environment/rd_environment.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
// --- Folded from servers/rendering/renderer_rd/renderer_canvas_render_rd.h ---
#include "core/templates/hash_map.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_canvas_render.h"
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
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/shaders/blit.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/storage_rd/mesh_storage.h ---
#include "core/templates/local_vector.h"
#include "core/templates/rid_owner.h"
#include "core/templates/self_list.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/shaders/skeleton.glsl.gen.h"
#include "servers/rendering/storage/mesh_storage.h"
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
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
// --- Folded from servers/rendering/renderer_rd/storage_rd/utilities.h ---
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_device_enums.h"
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
#include "servers/rendering/storage/material_storage.h"
#include "servers/rendering/storage/particles_storage.h"
#include "servers/rendering/storage/texture_storage.h"

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
