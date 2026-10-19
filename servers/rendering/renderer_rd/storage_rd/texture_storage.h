/**************************************************************************/
/*  texture_storage.h                                                     */
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
/* without limitation the rights to use, copy, modify, merge, publish,   */
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

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/templates/paged_array.h"
#include "core/templates/rb_map.h"
#include "core/templates/rid_owner.h"
#include "core/templates/sort_array.h"
#include "servers/rendering/renderer_rd/shaders/blit.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/canvas_sdf.glsl.gen.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererRD
{

struct DecalAtlas
{
	struct SortItem
	{
		RID texture;
		Vector2i pos;
		Size2i size;
		Vector2i pixel_size;

		bool operator<(const SortItem& p_item) const { return size.width > p_item.size.width; }
	};

	struct MipMap
	{
		RID texture;
		RID fb;
		Size2i size;
	};

	struct Texture
	{
		uint32_t users = 0;
		uint32_t panorama_to_dp_users = 0;
		Rect2 uv_rect;
	};

	HashMap<RID, Texture> textures;
	RID texture;
	RID texture_srgb;
	Vector<MipMap> texture_mipmaps;
	int mipmaps = 1;
	Size2i size;
	bool dirty = false;
};

struct AreaLightAtlas
{
	struct SortItem
	{
		RID texture;
		Vector2i pos;
		Size2i size;
		Vector2i pixel_size;

		bool operator<(const SortItem& p_item) const { return size.width > p_item.size.width; }
	};

	struct MipMap
	{
		RID texture;
		RID fb;
		Size2i size;
	};

	struct Texture
	{
		uint32_t users = 0;
		Rect2 uv_rect;
	};

	HashMap<RID, Texture> textures;
	RID texture;
	Vector<MipMap> texture_mipmaps;
	int mipmaps = 1;
	Size2i size;
	bool dirty = false;
};

struct TexBlitShader
{
	BlitShaderRD shader;
	ShaderCompiler compiler;
	RID default_shader;
	RID default_material;
	bool initialized = false;
};

class TextureStorage final
{
public:
	typedef void (*InvalidationCallback)(bool, void*);
	typedef InvalidationCallback CanvasTextureInvalidationCallback;

	static inline TexBlitShader tex_blit_shader;

	enum
	{
		SAMPLERS_BINDING_FIRST_INDEX = 1,
	};

	struct RenderTargetSDF
	{
		struct PushConstant
		{
			int32_t size[2];
			int32_t stride = 0;
			int32_t shift = 0;
			int32_t base_size[2];
			float pad[2];
		};

		CanvasSdfShaderRD shader;
		ShaderCompiler compiler;
		RID shader_version;
		RID pipeline;
	};

	static inline RenderTargetSDF rt_sdf;

	enum TextureType
	{
		TYPE_2D,
		TYPE_LAYERED,
		TYPE_3D,
	};

	struct CanvasTextureInfo
	{
		RID diffuse;
		RID normal;
		RID specular;
		RID sampler;
		Size2i size;
		Color specular_color;
		float shininess = 0.0;
		bool use_normal = false;
		bool use_specular = false;

		_FORCE_INLINE_ bool is_null() const { return diffuse.is_null(); }
	};

	enum DefaultRDTexture
	{
		DEFAULT_RD_TEXTURE_WHITE,
		DEFAULT_RD_TEXTURE_BLACK,
		DEFAULT_RD_TEXTURE_TRANSPARENT,
		DEFAULT_RD_TEXTURE_NORMAL,
		DEFAULT_RD_TEXTURE_ANISO,
		DEFAULT_RD_TEXTURE_DEPTH,
		DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE,
		DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK,
		DEFAULT_RD_TEXTURE_2D_ARRAY_TRANSPARENT,
		DEFAULT_RD_TEXTURE_2D_ARRAY_NORMAL,
		DEFAULT_RD_TEXTURE_2D_ARRAY_DEPTH,
		DEFAULT_RD_TEXTURE_3D_WHITE,
		DEFAULT_RD_TEXTURE_3D_BLACK,
		DEFAULT_RD_TEXTURE_3D_TRANSPARENT,
		DEFAULT_RD_TEXTURE_CUBEMAP_WHITE,
		DEFAULT_RD_TEXTURE_CUBEMAP_BLACK,
		DEFAULT_RD_TEXTURE_CUBEMAP_TRANSPARENT,
		DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_WHITE,
		DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_BLACK,
		DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_TRANSPARENT,
		DEFAULT_RD_TEXTURE_MAX,
		DEFAULT_RD_TEXTURE_ROUGHNESS,
		DEFAULT_RD_TEXTURE_METALLIC,
		DEFAULT_RD_TEXTURE_FLOWMAP,
	};

	struct CanvasTextureCache
	{
		RID diffuse;
		RID normal;
		RID specular;
		RID sampler;
	};

	struct CanvasTexture
	{
		RID self;
		RID diffuse;
		RID normal_map;
		RID specular;
		Color specular_color = Color(1, 1, 1, 1);
		real_t shininess = 1.0;
		RSE::CanvasItemTextureFilter texture_filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
		RSE::CanvasItemTextureRepeat texture_repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;

		CanvasTextureCache info_cache[2];
		Size2i size_cache = Size2i(1, 1);
		bool use_normal_cache = false;
		bool use_specular_cache = false;

		InvalidationCallback invalidated_callback = nullptr;
		void* invalidated_callback_userdata = nullptr;

		void clear_cache();
		~CanvasTexture();
	};

	struct RenderTarget
	{
		RID self;
		Size2i size;
		Point2i position;
		RID color;
		RID depth;
		RID rd_framebuffer;
		Vector<RID> color_slices;
		RID color_multisample;

		RenderingDeviceCommons::DataFormat color_format =
			RenderingDeviceCommons::DATA_FORMAT_R8G8B8A8_UNORM;
		RenderingDeviceCommons::DataFormat color_format_srgb =
			RenderingDeviceCommons::DATA_FORMAT_R8G8B8A8_SRGB;
		Image::Format image_format = Image::FORMAT_RGBA8;

		RID backbuffer;
		RID backbuffer_mipmap0;
		RID backbuffer_fb;
		Vector<RID> backbuffer_mipmaps;

		RID framebuffer_uniform_set;
		RID backbuffer_uniform_set;

		bool direct_to_screen = false;
		bool is_transparent = false;
		bool was_used = false;
		bool clear_requested = false;
		Color clear_color;
		RSE::ViewportMSAA msaa = RSE::VIEWPORT_MSAA_DISABLED;
		bool msaa_needs_resolve = false;
		bool use_hdr = false;
		bool use_debanding = false;

		Rect2i render_region;

		RID texture;

		RSE::ViewportSDFOversize sdf_oversize = RSE::VIEWPORT_SDF_OVERSIZE_100_PERCENT;
		RSE::ViewportSDFScale sdf_scale = RSE::VIEWPORT_SDF_SCALE_100_PERCENT;
		bool sdf_enabled = false;
		RID sdf_buffer_read;
		RID sdf_buffer_write;
		RID sdf_buffer_write_fb;
		RID sdf_buffer_process[2];
		RID sdf_buffer_process_uniform_sets[2];

		struct RTOverridden
		{
			RID color;
			RID depth;
			RID velocity;
			RID velocity_depth;
			bool is_overridden = false;

			struct SliceKey
			{
				RID texture;
				uint32_t layer = 0;

				SliceKey(RID p_texture = RID(), uint32_t p_layer = 0)
					: texture(p_texture), layer(p_layer)
				{
				}

				bool operator==(const SliceKey& p_key) const
				{
					return texture == p_key.texture && layer == p_key.layer;
				}

				uint32_t hash() const
				{
					uint32_t h = hash_murmur3_one_64(texture.get_id());
					return hash_murmur3_one_32(layer, h);
				}
			};

			HashMap<SliceKey, RID> cached_slices;
		};

		typedef RTOverridden Overridden;
		Overridden overridden;

		uint32_t view_count = 1;
		static inline Size2i velocity_target_size;

		RSE::ViewportVRSMode vrs_mode = RSE::VIEWPORT_VRS_DISABLED;
		RSE::ViewportVRSUpdateMode vrs_update_mode = RSE::VIEWPORT_VRS_UPDATE_ONCE;
		RID vrs_texture;
		bool subsampled_enabled = false;
		bool subsampled_allowed = false;

		RID get_framebuffer();
	};

	struct Texture
	{
		HashSet<RID> lightmap_users;
		TextureType type = TYPE_2D;

		RID self;
		Image::Format format = Image::FORMAT_L8;
		Image::Format real_format = Image::FORMAT_L8;
		Image::Format validated_format = Image::FORMAT_L8;
		int width = 0;
		int height = 0;
		int depth = 0;
		int layers = 0;
		int mipmaps = 1;
		int width_2d = 0;
		int height_2d = 0;

		Ref<Image> image;
		Ref<Image> image_cache_2d;
		String path;
		bool is_render_target = false;
		bool is_proxy = false;
		RID proxy_to;
		Vector<RID> proxies;

		CanvasTexture* canvas_texture = nullptr;
		RenderTarget* render_target = nullptr;

		RenderingDeviceCommons::TextureType rd_type = RenderingDeviceCommons::TEXTURE_TYPE_2D;
		RenderingDeviceCommons::DataFormat rd_format =
			RenderingDeviceCommons::DATA_FORMAT_R8G8B8A8_UNORM;
		RenderingDeviceCommons::DataFormat rd_format_srgb =
			RenderingDeviceCommons::DATA_FORMAT_R8G8B8A8_SRGB;
		RD::TextureView rd_view;
		RID rd_texture;
		RID rd_texture_srgb;

		struct BufferSlice3D
		{
			uint32_t offset = 0;
			uint32_t buffer_size = 0;
			Size2i size;
		};

		uint32_t buffer_size_3d = 0;
		Vector<BufferSlice3D> buffer_slices_3d;

		RSE::TextureLayeredType layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY;
		RSE::TextureDrawableFormat drawable_type = RSE::TEXTURE_DRAWABLE_FORMAT_RGBA8;

		RenderingServerTypes::TextureDetectCallback detect_3d_callback = nullptr;
		void* detect_3d_callback_ud = nullptr;
		RenderingServerTypes::TextureDetectCallback detect_normal_callback = nullptr;
		void* detect_normal_callback_ud = nullptr;
		RenderingServerTypes::TextureDetectRoughnessCallback detect_roughness_callback = nullptr;
		void* detect_roughness_callback_ud = nullptr;
		bool redraw_if_visible = false;

		void cleanup();
	};

	struct Decal
	{
		Dependency dependency;
		Vector3 size = Vector3(2, 2, 2);
		RID textures[RSE::DECAL_TEXTURE_MAX];
		float emission_energy = 1.0;
		float albedo_mix = 1.0;
		Color modulate = Color(1, 1, 1, 1);
		uint32_t cull_mask = 0xFFFFFFFF;
		bool distance_fade = false;
		float distance_fade_begin = 40.0;
		float distance_fade_length = 10.0;
		float upper_fade = 0.3;
		float lower_fade = 0.3;
		float normal_fade = 0.0;
	};

	struct DecalInstance
	{
		RID decal;
		Transform3D transform;
		float sorting_offset = 0.0;
		uint32_t forward_id = 0;
		uint32_t cull_mask = 0xFFFFFFFF;
	};

	struct DecalInstanceSort
	{
		DecalInstance* decal_instance = nullptr;
		Decal* decal = nullptr;
		float depth = 0.0;

		bool operator<(const DecalInstanceSort& p_sort) const { return depth < p_sort.depth; }
	};

	struct DecalData
	{
		float xform[16];
		float inv_xform[16];
		float normal_xform[12];
		float modulate[4];
		float emission[4];
		float emission_energy;
		float albedo_mix;
		float normal_factor;
		float specular;
		float metallic;
		float roughness;
		float normal_scale;
		uint32_t cull_mask;
		float normal[3];
		float normal_fade;
		float albedo_rect[4];
		float normal_rect[4];
		float orm_rect[4];
		float emission_rect[4];
		uint32_t mask;
		float upper_fade;
		float lower_fade;
	};

	struct TextureFromRDFormat
	{
		RenderingDeviceCommons::DataFormat rd_format =
			RenderingDeviceCommons::DATA_FORMAT_R8G8B8A8_UNORM;
		RenderingDeviceCommons::DataFormat rd_format_srgb =
			RenderingDeviceCommons::DATA_FORMAT_R8G8B8A8_SRGB;
		Image::Format image_format = Image::FORMAT_RGBA8;
		RenderingDeviceCommons::TextureSwizzle swizzle_r =
			RenderingDeviceCommons::TEXTURE_SWIZZLE_IDENTITY;
		RenderingDeviceCommons::TextureSwizzle swizzle_g =
			RenderingDeviceCommons::TEXTURE_SWIZZLE_IDENTITY;
		RenderingDeviceCommons::TextureSwizzle swizzle_b =
			RenderingDeviceCommons::TEXTURE_SWIZZLE_IDENTITY;
		RenderingDeviceCommons::TextureSwizzle swizzle_a =
			RenderingDeviceCommons::TEXTURE_SWIZZLE_IDENTITY;
	};

	static inline AreaLightAtlas area_light_atlas;
	static inline DecalAtlas decal_atlas;
	static inline uint32_t decal_count = 0;
	static inline uint32_t max_decals = 0;
	static inline DecalInstanceSort* decal_sort = nullptr;
	static inline DecalData* decals = nullptr;
	static inline RID decal_buffer;

	static inline Ref<Image> texture_2d_placeholder;
	static inline Vector<Ref<Image>> texture_2d_array_placeholder;
	static inline Vector<Ref<Image>> cubemap_placeholder;
	static inline Vector<Ref<Image>> texture_3d_placeholder;

	static inline RID_Owner<RenderTarget, true> render_target_owner;
	static inline RID_Owner<Texture, true> texture_owner;
	static inline RID_Owner<CanvasTexture, true> canvas_texture_owner;
	static inline RID_Owner<Decal, true> decal_owner;
	static inline RID_Owner<DecalInstance> decal_instance_owner;

	static inline RID default_rd_textures[DEFAULT_RD_TEXTURE_MAX];
	static inline Color default_clear_color = Color(0, 0, 0, 1);
	static inline void set_default_clear_color(const Color &p_color) { default_clear_color = p_color; }
	static inline Color get_default_clear_color() { return default_clear_color; }

	TextureStorage() = delete;
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;
	~TextureStorage() = delete;

	static bool free(RID p_rid);

	static void _tex_blit_shader_initialize();
	static void _tex_blit_shader_free();

	static void _texture_format_from_rd(
		RenderingDeviceCommons::DataFormat p_rd_format, TextureFromRDFormat& r_format);
	static Size2i texture_2d_get_size(RID p_texture);

	// Decal atlas & buffers
	static RID decal_atlas_get_texture();
	static RID decal_atlas_get_texture_srgb();

	static Rect2 decal_atlas_get_texture_rect(RID p_texture)
	{
		if (decal_atlas.textures.has(p_texture)) {
			return decal_atlas.textures[p_texture].uv_rect;
		}
		return Rect2();
	}

	static void update_decal_atlas();
	static void decal_atlas_mark_dirty_on_texture(RID p_texture);
	static void decal_atlas_remove_texture(RID p_texture);
	static Dependency* decal_get_dependency(RID p_decal);

	static RID get_decal_buffer() { return decal_buffer; }

	static uint32_t decal_instance_get_forward_id(RID p_decal_instance);
	static void update_decal_buffer(
		const PagedArray<RID>& p_decals, const Transform3D& p_cam_transform);
	static void free_decal_data();
	static void set_max_decals(const uint32_t p_max_decals);

	// Area light atlas queries
	static RID area_light_atlas_get_texture();

	static Rect2 area_light_atlas_get_texture_rect(RID p_texture)
	{
		if (area_light_atlas.textures.has(p_texture)) {
			return area_light_atlas.textures[p_texture].uv_rect;
		}
		return Rect2();
	}

	static Size2i area_light_atlas_get_size() { return area_light_atlas.size; }

	static int area_light_atlas_get_mipmaps() { return area_light_atlas.mipmaps; }

	static void update_area_light_atlas();
	static void area_light_atlas_mark_dirty_on_texture(RID p_texture);
	static void area_light_atlas_remove_texture(RID p_texture);

	// Render target format & slice overrides
	static RenderingDeviceCommons::DataFormat render_target_get_color_format(
		bool p_hdr, bool p_is_multiview = false);
	static uint32_t render_target_get_color_usage_bits(bool p_vrs);
	static RID render_target_get_override_depth_slice(RID p_render_target, uint32_t p_layer);
	static RID render_target_get_override_velocity_slice(RID p_render_target, uint32_t p_layer);

	// Default RD Texture fallback
	static RID texture_rd_get_default(DefaultRDTexture p_texture)
	{
		ERR_FAIL_INDEX_V(p_texture, DEFAULT_RD_TEXTURE_MAX, RID());
		return default_rd_textures[p_texture];
	}

	static CanvasTextureInfo canvas_texture_get_info(RID p_texture,
		RenderingServerEnums::CanvasItemTextureFilter p_filter,
		RenderingServerEnums::CanvasItemTextureRepeat p_repeat, bool p_use_linear_colors,
		bool p_is_data);
	static TextureType texture_get_type(RID p_texture);

	// Canvas texture helpers
	static void canvas_texture_set_invalidation_callback(
		RID p_texture, CanvasTextureInvalidationCallback p_callback, void* p_userdata);

	// Render Target RD textures, backbuffers, and framebuffers
	static RID render_target_get_rd_texture(RID p_render_target);
	static RID render_target_get_rd_texture_msaa(RID p_render_target);
	static RID render_target_get_rd_texture_slice(RID p_render_target, uint32_t p_layer);
	static RID render_target_get_rd_backbuffer(RID p_render_target);
	static RID render_target_get_rd_framebuffer(RID p_render_target);
	static RID render_target_get_sdf_texture(RID p_render_target);
	static bool render_target_is_sdf_enabled(RID p_render_target);

	// Render Target uniform sets and backbuffer operations
	static void render_target_set_backbuffer_uniform_set(RID p_render_target, RID p_uniform_set);
	static void render_target_set_framebuffer_uniform_set(RID p_render_target, RID p_uniform_set);
	static void render_target_copy_to_back_buffer(
		RID p_render_target, const Rect2i& p_region, bool p_gen_mipmaps);
	static void render_target_clear_back_buffer(
		RID p_render_target, const Rect2i& p_region, const Color& p_color);
	static void render_target_gen_back_buffer_mipmaps(RID p_render_target, const Rect2i& p_region);

	// Render Target internal helpers implemented in texture_storage.cpp
	static void _clear_render_target(RenderTarget* rt);
	static void _update_render_target(RenderTarget* rt);
	static void _create_render_target_backbuffer(RenderTarget* rt);
	static Rect2i _render_target_get_sdf_rect(const RenderTarget* rt);
	static void _render_target_clear_sdf(RenderTarget* rt);
	static void _render_target_allocate_sdf(RenderTarget* rt);
	static RID render_target_get_sdf_framebuffer(RID p_render_target);
	static void render_target_sdf_process(RID p_render_target);
	static RID render_target_get_framebuffer_uniform_set(RID p_render_target);
	static RID render_target_get_backbuffer_uniform_set(RID p_render_target);
	static RID render_target_get_rd_backbuffer_framebuffer(RID p_render_target);

	// Texture queries
	static int texture_get_layers(RID p_texture);

	static void initialize();
	static void finalize();

	/* CANVAS TEXTURE API */

	static RID canvas_texture_allocate();
	static void canvas_texture_initialize(RID p_rid);
	static void canvas_texture_free(RID p_rid);

	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);
	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat);

	/* TEXTURE API */

	static RID texture_allocate();
	static void texture_free(RID p_rid);

	static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer);
	static void texture_proxy_initialize(RID p_texture, RID p_base);
	static RID texture_create_from_native_handle(RSE::TextureType p_type, Image::Format p_format,
		uint64_t p_native_handle, int p_width, int p_height, int p_depth, int p_layers = 1,
		RSE::TextureLayeredType p_layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY);

	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer);
	static void texture_proxy_update(RID p_proxy, RID p_base);

	static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image);
	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer = 0);
	static void texture_2d_placeholder_initialize(RID p_texture);
	static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type);
	static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type);

	static void texture_3d_initialize(RID p_texture, Image::Format p_format, int p_width,
		int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data);
	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data);
	static void texture_3d_placeholder_initialize(RID p_texture);

	static Ref<Image> texture_2d_get(RID p_texture);
	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer);
	static Vector<Ref<Image>> texture_3d_get(RID p_texture);

	static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color = Color(),
		bool p_has_mipmaps = false);
	static void texture_drawable_generate_mipmaps(RID p_texture);
	static RID texture_drawable_get_default_material();

	static void texture_replace(RID p_texture, RID p_by_texture);
	static void texture_set_size_override(RID p_texture, int p_width, int p_height);

	static void texture_set_path(RID p_texture, const String& p_path);
	static String texture_get_path(RID p_texture);

	static void texture_set_format(RID p_texture, Image::Format p_format);
	static Image::Format texture_get_format(RID p_texture);

	static void texture_set_detect_3d_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_normal_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_roughness_callback(RID p_texture,
		RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata);

	static void texture_debug_usage(List<RenderingServerTypes::TextureInfo>* r_info);
	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	static Size2 texture_size_with_proxy(RID p_proxy);

	static void texture_rd_initialize(
		RID p_texture, const RID& p_rd_texture, RSE::TextureLayeredType p_layered_type);
	static RID texture_get_rd_texture(RID p_texture, bool p_srgb = false);
	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb = false);

	static void texture_add_to_area_light_atlas(RID p_texture);
	static void texture_remove_from_area_light_atlas(RID p_texture);

	static void texture_add_to_decal_atlas(RID p_texture, bool p_panorama_to_dp = false);
	static void texture_remove_from_decal_atlas(RID p_texture, bool p_panorama_to_dp = false);

	/* DECAL API */

	static RID decal_allocate();
	static void decal_initialize(RID p_decal);
	static void decal_free(RID p_rid);

	static void decal_set_size(RID p_decal, const Vector3& p_size);
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture);
	static void decal_set_emission_energy(RID p_decal, float p_energy);
	static void decal_set_albedo_mix(RID p_decal, float p_mix);
	static void decal_set_modulate(RID p_decal, const Color& p_modulate);
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers);
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length);
	static void decal_set_fade(RID p_decal, float p_above, float p_below);
	static void decal_set_normal_fade(RID p_decal, float p_fade);

	static AABB decal_get_aabb(RID p_decal);
	static uint32_t decal_get_cull_mask(RID p_decal);

	static RID decal_instance_create(RID p_decal);
	static void decal_instance_free(RID p_decal_instance);
	static void decal_instance_set_transform(RID p_decal_instance, const Transform3D& p_transform);
	static void decal_instance_set_sorting_offset(RID p_decal_instance, float p_sorting_offset);

	/* RENDER TARGET API */

	static RID render_target_create();
	static void render_target_free(RID p_rid);

	static void render_target_set_position(RID p_render_target, int p_x, int p_y);
	static Point2i render_target_get_position(RID p_render_target);

	static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count);
	static Size2i render_target_get_size(RID p_render_target);

	static void render_target_set_transparent(RID p_render_target, bool p_is_transparent);
	static bool render_target_get_transparent(RID p_render_target);

	static void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen);
	static bool render_target_get_direct_to_screen(RID p_render_target);

	static bool render_target_was_used(RID p_render_target);
	static void render_target_set_as_unused(RID p_render_target);

	static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa);
	static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target);

	static void render_target_set_msaa_needs_resolve(RID p_render_target, bool p_needs_resolve);
	static bool render_target_get_msaa_needs_resolve(RID p_render_target);
	static void render_target_do_msaa_resolve(RID p_render_target);

	static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr);
	static bool render_target_is_using_hdr(RID p_render_target);

	static void render_target_set_use_debanding(RID p_render_target, bool p_use_debanding);
	static bool render_target_is_using_debanding(RID p_render_target);

	static void render_target_request_clear(RID p_render_target, const Color& p_clear_color);
	static bool render_target_is_clear_requested(RID p_render_target);
	static Color render_target_get_clear_request_color(RID p_render_target);
	static void render_target_disable_clear_request(RID p_render_target);
	static void render_target_do_clear_request(RID p_render_target);

	static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale);
	static Rect2i render_target_get_sdf_rect(RID p_render_target);
	static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled);

	static void render_target_set_vrs_mode(RID p_render_target, RSE::ViewportVRSMode p_mode);
	static RSE::ViewportVRSMode render_target_get_vrs_mode(RID p_render_target);
	static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode);
	static RSE::ViewportVRSUpdateMode render_target_get_vrs_update_mode(RID p_render_target);
	static void render_target_set_vrs_texture(RID p_render_target, RID p_texture);
	static RID render_target_get_vrs_texture(RID p_render_target);

	static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity = RID(), RID p_velocity_depth = RID());
	static RID render_target_get_override_color(RID p_render_target);
	static RID render_target_get_override_depth(RID p_render_target);
	static RID render_target_get_override_velocity(RID p_render_target);
	static RID render_target_get_override_velocity_depth(RID p_render_target);

	static void render_target_set_render_region(RID p_render_target, const Rect2i& p_render_region);
	static Rect2i render_target_get_render_region(RID p_render_target);

	static void render_target_set_subsampled_enabled(RID p_render_target, bool p_enabled);
	static bool render_target_is_subsampled_enabled(RID p_render_target);
	static void render_target_set_subsampled_allowed(RID p_render_target, bool p_allowed);
	static bool render_target_is_subsampled_allowed(RID p_render_target);

	static RID render_target_get_texture(RID p_render_target);

	static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{}

	static Size2i render_target_get_velocity_target_size(RID p_render_target)
	{
		return RenderTarget::velocity_target_size;
	}

	static RenderTarget* get_render_target(RID p_rid)
	{
		return render_target_owner.get_or_null(p_rid);
	}

	static bool owns_render_target(RID p_rid) { return render_target_owner.owns(p_rid); }

	static Texture* get_texture(RID p_rid) { return texture_owner.get_or_null(p_rid); }

	static bool owns_texture(RID p_rid) { return texture_owner.owns(p_rid); }

	static CanvasTexture* get_canvas_texture(RID p_rid)
	{
		return canvas_texture_owner.get_or_null(p_rid);
	}

	static bool owns_canvas_texture(RID p_rid) { return canvas_texture_owner.owns(p_rid); }

	static Decal* get_decal(RID p_rid) { return decal_owner.get_or_null(p_rid); }

	static bool owns_decal(RID p_rid) { return decal_owner.owns(p_rid); }

	static DecalInstance* get_decal_instance(RID p_rid)
	{
		return decal_instance_owner.get_or_null(p_rid);
	}

	static bool owns_decal_instance(RID p_rid) { return decal_instance_owner.owns(p_rid); }
};

} // namespace RendererRD


