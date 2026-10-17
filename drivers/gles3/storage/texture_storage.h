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

#ifdef GLES3_ENABLED

#include <platform_gl.h>
#include "core/io/image.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rid_owner.h"
#include "drivers/gles3/effects/copy_effects.h"
#include "drivers/gles3/shaders/canvas_sdf.glsl.gen.h"
#include "drivers/gles3/storage/config.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/texture_storage.h"

#ifndef _GL_TEXTURE_EXTERNAL_OES
#define _GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef _EXT_TEXTURE_CUBE_MAP_SEAMLESS
#define _EXT_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#endif

#ifndef _EXT_R16
#define _EXT_R16 0x822A
#endif

#ifndef _EXT_RG16
#define _EXT_RG16 0x822C
#endif

#ifndef _EXT_RGB16
#define _EXT_RGB16 0x8054
#endif

#ifndef _EXT_RGBA16
#define _EXT_RGBA16 0x805B
#endif

#ifndef _EXT_COMPRESSED_RGB_S3TC_DXT1_EXT
#define _EXT_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#endif

#ifndef _EXT_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define _EXT_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define _EXT_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif

#ifndef _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define _EXT_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef _EXT_COMPRESSED_RED_RGTC1_EXT
#define _EXT_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#endif

#ifndef _EXT_COMPRESSED_SIGNED_RED_RGTC1_EXT
#define _EXT_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#endif

#ifndef _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT
#define _EXT_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#endif

#ifndef _EXT_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT
#define _EXT_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#endif

#ifndef _EXT_COMPRESSED_RGBA_BPTC_UNORM
#define _EXT_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif

#ifndef _EXT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#define _EXT_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif

#ifndef _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#define _EXT_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif

#ifndef _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#define _EXT_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif

#ifndef _EXT_COMPRESSED_R11_EAC
#define _EXT_COMPRESSED_R11_EAC 0x9270
#endif

#ifndef _EXT_COMPRESSED_SIGNED_R11_EAC
#define _EXT_COMPRESSED_SIGNED_R11_EAC 0x9271
#endif

#ifndef _EXT_COMPRESSED_RG11_EAC
#define _EXT_COMPRESSED_RG11_EAC 0x9272
#endif

#ifndef _EXT_COMPRESSED_SIGNED_RG11_EAC
#define _EXT_COMPRESSED_SIGNED_RG11_EAC 0x9273
#endif

#ifndef _EXT_COMPRESSED_RGB8_ETC2
#define _EXT_COMPRESSED_RGB8_ETC2 0x9274
#endif

#ifndef _EXT_COMPRESSED_SRGB8_ETC2
#define _EXT_COMPRESSED_SRGB8_ETC2 0x9275
#endif

#ifndef _EXT_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define _EXT_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#endif

#ifndef _EXT_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2
#define _EXT_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#endif

#ifndef _EXT_COMPRESSED_RGBA8_ETC2_EAC
#define _EXT_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#endif

#ifndef _EXT_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
#define _EXT_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#endif

#ifndef _EXT_COMPRESSED_RGBA_ASTC_4x4_KHR
#define _EXT_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#endif

#ifndef _EXT_COMPRESSED_RGBA_ASTC_6x6_KHR
#define _EXT_COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#endif

#ifndef _EXT_COMPRESSED_RGBA_ASTC_8x8_KHR
#define _EXT_COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#endif

namespace GLES3
{

enum DefaultGLTexture
{
	DEFAULT_GL_TEXTURE_WHITE,
	DEFAULT_GL_TEXTURE_BLACK,
	DEFAULT_GL_TEXTURE_TRANSPARENT,
	DEFAULT_GL_TEXTURE_NORMAL,
	DEFAULT_GL_TEXTURE_ANISO,
	DEFAULT_GL_TEXTURE_DEPTH,
	DEFAULT_GL_TEXTURE_CUBEMAP_BLACK,
	DEFAULT_GL_TEXTURE_CUBEMAP_WHITE,
	DEFAULT_GL_TEXTURE_CUBEMAP_TRANSPARENT,
	DEFAULT_GL_TEXTURE_3D_WHITE,
	DEFAULT_GL_TEXTURE_3D_BLACK,
	DEFAULT_GL_TEXTURE_3D_TRANSPARENT,
	DEFAULT_GL_TEXTURE_2D_ARRAY_WHITE,
	DEFAULT_GL_TEXTURE_2D_ARRAY_BLACK,
	DEFAULT_GL_TEXTURE_2D_ARRAY_TRANSPARENT,
	DEFAULT_GL_TEXTURE_EXT,
	DEFAULT_GL_TEXTURE_MAX
};

struct RenderTarget;
struct Texture;
struct CanvasTexture;

struct RenderTarget
{
	RID self;
	Size2i size;
	Point2i position;
	GLuint fbo = 0;
	GLuint color = 0;
	GLuint depth = 0;
	GLenum color_internal_format = GL_RGBA8;
	GLenum color_format = GL_RGBA;
	GLenum color_type = GL_UNSIGNED_BYTE;
	uint32_t color_format_size = 4;
	Image::Format image_format = Image::FORMAT_RGBA8;
	bool direct_to_screen = false;
	bool is_transparent = false;
	bool was_used = false;
	bool clear_requested = false;
	Color clear_color;
	RSE::ViewportMSAA msaa = RSE::VIEWPORT_MSAA_DISABLED;
	bool use_hdr = false;
	bool hdr = false;
	bool used_in_frame = false;

	bool reattach_textures = false;
	bool depth_has_stencil = false;

	RID texture;

	Rect2i render_region;

	GLuint backbuffer_fbo = 0;
	GLuint backbuffer = 0;
	GLuint backbuffer_depth = 0;
	int mipmap_count = 1;

	bool sdf_enabled = false;
	RSE::ViewportSDFOversize sdf_oversize = RSE::VIEWPORT_SDF_OVERSIZE_100_PERCENT;
	RSE::ViewportSDFScale sdf_scale = RSE::VIEWPORT_SDF_SCALE_100_PERCENT;
	Size2i process_size;
	GLuint sdf_texture_read = 0;
	GLuint sdf_texture_write = 0;
	GLuint sdf_texture_process[2] = {0, 0};
	GLuint sdf_texture_write_fb = 0;

	struct RTOverridden
	{
		struct FBOCacheEntry
		{
			GLuint fbo = 0;
			GLuint color = 0;
			GLuint depth = 0;
			bool depth_has_stencil = false;
			Size2i size;
			LocalVector<GLuint> allocated_textures;
		};
	};

	struct Overridden
	{
		RID color;
		RID depth;
		RID velocity;
		RID velocity_depth;
		GLuint velocity_fbo = 0;
		bool is_overridden = false;
		bool depth_has_stencil = false;
		RBMap<uint32_t, RTOverridden::FBOCacheEntry> fbo_cache;
		RBMap<uint32_t, GLuint> velocity_fbo_cache;
	} overridden;

	uint32_t view_count = 1;
	Size2i velocity_target_size;
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
};

struct Texture
{
	enum Type
	{
		TYPE_2D,
		TYPE_LAYERED,
		TYPE_3D,
	};

	Type type = TYPE_2D;

	RID self;
	Image::Format format = Image::FORMAT_L8;
	Image::Format real_format = Image::FORMAT_L8;
	GLenum gl_format_cache = 0;
	GLenum gl_internal_format_cache = 0;
	GLenum gl_type_cache = 0;
	GLenum target = GL_TEXTURE_2D;
	int width = 0;
	int height = 0;
	int depth = 0;
	int layers = 0;
	int alloc_width = 0;
	int alloc_height = 0;
	int mipmaps = 1;
	bool compressed = false;
	bool active = false;
	bool resize_to_po2 = false;
	uint32_t stored_cube_sides = 0;
	size_t total_data_size = 0;
	GLuint tex_id = 0;
	Ref<Image> image;
	Ref<Image> image_cache_2d;
	Vector<Ref<Image>> image_cache_3d;
	String path;
	bool is_render_target = false;
	bool is_from_native_handle = false;
	RenderTarget* render_target = nullptr;
	CanvasTexture* canvas_texture = nullptr;
	bool is_proxy = false;
	RID proxy_to;
	Vector<RID> proxies;

	RSE::TextureLayeredType layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY;
	RSE::TextureDrawableFormat drawable_type = RSE::TEXTURE_DRAWABLE_FORMAT_RGBA8;

	RenderingServerTypes::TextureDetectCallback detect_3d_callback = nullptr;
	void* detect_3d_callback_ud = nullptr;
	RenderingServerTypes::TextureDetectCallback detect_normal_callback = nullptr;
	void* detect_normal_callback_ud = nullptr;
	RenderingServerTypes::TextureDetectRoughnessCallback detect_roughness_callback = nullptr;
	void* detect_roughness_callback_ud = nullptr;
	bool redraw_if_visible = false;

	RSE::CanvasItemTextureFilter filter_cache = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
	RSE::CanvasItemTextureRepeat repeat_cache = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;

	void copy_from(const Texture& p_from)
	{
		type = p_from.type;
		format = p_from.format;
		real_format = p_from.real_format;
		gl_format_cache = p_from.gl_format_cache;
		gl_internal_format_cache = p_from.gl_internal_format_cache;
		gl_type_cache = p_from.gl_type_cache;
		target = p_from.target;
		width = p_from.width;
		height = p_from.height;
		depth = p_from.depth;
		layers = p_from.layers;
		alloc_width = p_from.alloc_width;
		alloc_height = p_from.alloc_height;
		mipmaps = p_from.mipmaps;
		compressed = p_from.compressed;
		active = p_from.active;
		resize_to_po2 = p_from.resize_to_po2;
		stored_cube_sides = p_from.stored_cube_sides;
		total_data_size = p_from.total_data_size;
		tex_id = p_from.tex_id;
		image = p_from.image;
		image_cache_2d = p_from.image_cache_2d;
		image_cache_3d = p_from.image_cache_3d;
		path = p_from.path;
		is_render_target = p_from.is_render_target;
		is_from_native_handle = p_from.is_from_native_handle;
		render_target = p_from.render_target;
		canvas_texture = p_from.canvas_texture;
		is_proxy = true;
		proxy_to = p_from.self;
		layered_type = p_from.layered_type;
		drawable_type = p_from.drawable_type;
		filter_cache = p_from.filter_cache;
		repeat_cache = p_from.repeat_cache;
	}

	void gl_set_filter(RSE::CanvasItemTextureFilter p_filter) { filter_cache = p_filter; }

	void gl_set_repeat(RSE::CanvasItemTextureRepeat p_repeat) { repeat_cache = p_repeat; }
};

struct TextureAtlas
{
	struct Texture
	{
		int users = 0;
		Rect2 uv_rect;
	};

	struct SortItem
	{
		RID texture;
		Size2i pixel_size;
		Size2i size;
		Point2i pos;

		bool operator<(const SortItem& p_item) const { return size.height > p_item.size.height; }
	};

	HashMap<RID, Texture> textures;
	GLuint texture = 0;
	GLuint framebuffer = 0;
	Size2i size;
	bool dirty = false;
};

struct TexBlitShader
{
	CopyShaderGLES3 shader;
	RID default_shader;
	RID default_material;
	RID default_version;
	bool initialized = false;
};

class TextureStorage final
{
private:
	static inline RID_Owner<RenderTarget, true> render_target_owner;
	static inline RID_Owner<Texture, true> texture_owner;
	static inline RID_Owner<CanvasTexture, true> canvas_texture_owner;

	static inline GLuint current_fbo = 0;

	static inline TexBlitShader tex_blit_shader;
	static inline GLuint tex_blit_fbo = 0;
	static inline GLuint tex_blit_quad = 0;
	static inline GLuint tex_blit_quad_array = 0;

	struct CanvasSdfShader
	{
		CanvasSdfShaderGLES3 shader;
		RID shader_version;
	};

	static inline CanvasSdfShader sdf_shader;

	static inline TextureAtlas texture_atlas;
	static inline RID default_gl_textures[DEFAULT_GL_TEXTURE_MAX];
	static inline Ref<Image> texture_2d_placeholder;
	static inline Vector<Ref<Image>> texture_2d_array_placeholder;
	static inline Vector<Ref<Image>> cubemap_placeholder;
	static inline Vector<Ref<Image>> texture_3d_placeholder;

	static void _update_render_target_color(RenderTarget* rt);
	static void _update_render_target_velocity(RenderTarget* rt);
	static void _create_render_target_backbuffer(RenderTarget* rt);
	static void _clear_render_target(RenderTarget* rt);
	static void _render_target_allocate_sdf(RenderTarget* rt);
	static void _render_target_clear_sdf(RenderTarget* rt);
	static Rect2i _render_target_get_sdf_rect(const RenderTarget* rt);

	static void _texture_set_data(
		RID p_texture, const Ref<Image>& p_image, int p_layer = 0, bool p_sync = false);
	static void _texture_set_3d_data(
		RID p_texture, const Vector<Ref<Image>>& p_data, bool p_sync = false);
	static void _texture_set_swizzle(Texture* p_texture, Image::Format p_real_format);
	static Vector<Ref<Image>> _texture_3d_read_framebuffer(Texture* p_texture);
	static Ref<Image> _get_gl_image_and_format(const Ref<Image>& p_image, Image::Format p_format,
		Image::Format& r_real_format, GLenum& r_gl_format, GLenum& r_gl_internal_format,
		GLenum& r_gl_type, bool& r_compressed, bool p_force_decompress = false);

public:
	static inline GLuint system_fbo = 0;

	TextureStorage() = delete;
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;
	~TextureStorage() = delete;

	static void initialize();
	static void finalize();

	/* RENDER TARGET API */

	static RID render_target_create();
	static void render_target_free(RID p_rid);

	static void render_target_set_position(RID p_render_target, int p_x, int p_y);
	static Point2i render_target_get_position(RID p_render_target);

	static void tex_blit_shader_initialize();
	static void tex_blit_shader_free();
	static void _tex_blit_shader_initialize();
	static void _tex_blit_shader_free();

	static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count);
	static Size2i render_target_get_size(RID p_render_target);

	static void render_target_set_transparent(RID p_render_target, bool p_is_transparent);
	static bool render_target_is_transparent(RID p_render_target);
	static bool render_target_get_transparent(RID p_render_target);

	static void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen);
	static bool render_target_is_direct_to_screen(RID p_render_target);
	static bool render_target_get_direct_to_screen(RID p_render_target);

	static RID render_target_get_texture(RID p_render_target);

	static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa);
	static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target);

	static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr);
	static bool render_target_is_using_hdr(RID p_render_target);

	static void render_target_request_clear(RID p_render_target, const Color& p_clear_color);
	static bool render_target_is_clear_requested(RID p_render_target);
	static Color render_target_get_clear_request_color(RID p_render_target);
	static void render_target_do_clear_request(RID p_render_target);
	static void render_target_disable_clear_request(RID p_render_target);

	static GLuint render_target_get_fbo(RID p_render_target);
	static GLuint render_target_get_color(RID p_render_target);
	static GLuint render_target_get_depth(RID p_render_target);

	static bool render_target_is_reattach_textures(RID p_render_target);
	static void render_target_set_reattach_textures(RID p_render_target, bool p_reattach);
	static bool render_target_get_depth_has_stencil(RID p_render_target);

	static GLenum render_target_get_color_internal_format(RID p_render_target);
	static GLenum render_target_get_color_format(RID p_render_target);
	static GLenum render_target_get_color_type(RID p_render_target);
	static uint32_t render_target_get_color_format_size(RID p_render_target);

	static void render_target_copy_to_back_buffer(
		RID p_render_target, const Rect2i& p_region, bool p_gen_mipmaps = false);
	static void render_target_clear_back_buffer(
		RID p_render_target, const Rect2i& p_region, const Color& p_color);
	static void render_target_gen_back_buffer_mipmaps(RID p_render_target, const Rect2i& p_region);

	static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale);
	static Rect2i render_target_get_sdf_rect(RID p_render_target);
	static void render_target_mark_sdf_used(RID p_render_target);
	static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled);
	static GLuint render_target_get_sdf_texture(RID p_render_target);
	static GLuint render_target_get_sdf_framebuffer(RID p_render_target);
	static void render_target_sdf_process(RID p_render_target);
	static bool render_target_is_sdf_enabled(RID p_render_target);
	static void render_target_copy_to_screen(
		RID p_render_target, const Rect2& p_rect, int p_screen);
	static void render_target_clear_sdf(RID p_render_target);
	static RSE::ViewportSDFScale render_target_get_sdf_scale(RID p_render_target);

	static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity = RID(), RID p_velocity_depth = RID());
	static RID render_target_get_override_color(RID p_render_target);
	static RID render_target_get_override_depth(RID p_render_target);
	static RID render_target_get_override_velocity(RID p_render_target);
	static RID render_target_get_override_velocity_depth(RID p_render_target);

	static void render_target_set_render_region(RID p_render_target, const Rect2i& p_region);
	static Rect2i render_target_get_render_region(RID p_render_target);

	static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size);
	static Size2i render_target_get_velocity_target_size(RID p_render_target);

	static bool render_target_was_used(RID p_render_target);
	static void render_target_clear_used(RID p_render_target);
	static void render_target_set_as_unused(RID p_render_target);

	static void render_target_set_as_current(RID p_render_target);

	/* TEXTURE API */

	static RID texture_allocate();
	static void texture_initialize(RID p_rid);
	static void texture_free(RID p_rid);

	static RID texture_2d_create(const Ref<Image>& p_image);
	static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image);
	static void texture_2d_initialize_from_texture(RID p_texture, const Texture& p_from_texture);
	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer);
	static Ref<Image> texture_2d_get(RID p_texture);
	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer);
	static void texture_2d_placeholder_initialize(RID p_texture);

	static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer);
	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer);

	static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type);
	static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type);

	static void texture_3d_initialize(RID p_texture, Image::Format p_format, int p_width,
		int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data);
	static void texture_3d_placeholder_initialize(RID p_texture);

	static void texture_proxy_initialize(RID p_texture, RID p_base);
	static void texture_proxy_update(RID p_texture, RID p_proxy_to);
	static void texture_remap_proxies(RID p_from_texture, RID p_to_texture);
	static Size2 texture_size_with_proxy(RID p_texture);

	static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color = Color(),
		bool p_has_mipmaps = false);
	static void texture_drawable_generate_mipmaps(RID p_texture);
	static RID texture_drawable_get_default_material();

	static RID texture_create_from_native_handle(RSE::TextureType p_type, Image::Format p_format,
		uint64_t p_native_handle, int p_width, int p_height, int p_depth, int p_layers = 1,
		RSE::TextureLayeredType p_layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY);

	static RID texture_3d_create(Image::Format p_format, int p_width, int p_height, int p_depth,
		bool p_mipmaps, const Vector<Ref<Image>>& p_data);
	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data);
	static Vector<Ref<Image>> texture_3d_get(RID p_texture);

	static RID texture_layered_create(
		const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type);
	static void texture_layered_update(RID p_texture, const Vector<Ref<Image>>& p_layers);

	static void texture_set_format(RID p_texture, Image::Format p_format);
	static Image::Format texture_get_format(RID p_texture);
	static void texture_set_width(RID p_texture, int p_width);
	static uint32_t texture_get_width(RID p_texture);
	static void texture_set_height(RID p_texture, int p_height);
	static uint32_t texture_get_height(RID p_texture);
	static void texture_set_depth(RID p_texture, int p_depth);
	static uint32_t texture_get_depth(RID p_texture);
	static void texture_set_layers(RID p_texture, int p_layers);
	static int texture_get_layers(RID p_texture);
	static Vector3i texture_get_size(RID p_texture);
	static uint32_t texture_get_texid(RID p_texture);

	static void texture_set_path(RID p_texture, const String& p_path);
	static String texture_get_path(RID p_texture);
	static void texture_set_size_override(RID p_texture, int p_width, int p_height);

	static void texture_replace(RID p_texture, RID p_by_texture);
	static void texture_set_data(RID p_texture, const Ref<Image>& p_image, int p_layer = 0);

	static void texture_set_detect_3d_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_srgb_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_normal_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_roughness_callback(RID p_texture,
		RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata);

	static void texture_debug_usage(List<RenderingServerTypes::TextureInfo>* r_info);
	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	static void texture_bind(RID p_texture, uint32_t p_index);
	static RID texture_gl_get_default(DefaultGLTexture p_texture);

	static void texture_rd_initialize(
		RID p_texture, const RID& p_rd_texture, RSE::TextureLayeredType p_layered_type);
	static RID texture_get_rd_texture(RID p_texture, bool p_srgb = false);
	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb = false);

	/* ATLAS API */

	static Rect2 texture_atlas_get_texture_rect(RID p_texture);
	static GLuint texture_atlas_get_texture();
	static void texture_add_to_texture_atlas(RID p_texture);
	static void texture_remove_from_texture_atlas(RID p_texture);
	static void texture_atlas_remove_texture(RID p_texture);
	static void texture_atlas_mark_dirty_on_texture(RID p_texture);
	static void update_texture_atlas();

	static void texture_add_to_decal_atlas(RID p_texture, bool p_is_normal = false);
	static void texture_remove_from_decal_atlas(RID p_texture, bool p_is_normal = false);

	/* DECAL API */

	static RID decal_allocate();
	static void decal_initialize(RID p_rid);
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

	/* CANVAS TEXTURE API */

	static RID canvas_texture_allocate();
	static void canvas_texture_initialize(RID p_rid);
	static void canvas_texture_free(RID p_rid);

	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	static void canvas_texture_set_shading_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);
	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat);

	/* GL STATE & LOOKUP HELPERS */

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

	static String get_framebuffer_error(GLenum p_status);
	static Color get_default_clear_color();

	static void bind_texture(RID p_texture, uint32_t p_index);
	static void bind_texture_ext(RID p_texture, uint32_t p_index,
		RSE::CanvasItemTextureFilter p_filter, RSE::CanvasItemTextureRepeat p_repeat);
};

} // namespace GLES3

#endif // GLES3_ENABLED


