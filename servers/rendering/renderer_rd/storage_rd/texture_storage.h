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

#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/texture_storage.h"
#include "servers/rendering/storage/utilities.h"

namespace RendererRD
{

class TextureStorage final
{
public:
	enum DefaultRDTexture
	{
		DEFAULT_RD_TEXTURE_WHITE,
		DEFAULT_RD_TEXTURE_BLACK,
		DEFAULT_RD_TEXTURE_TRANSPARENT,
		DEFAULT_RD_TEXTURE_NORMAL,
		DEFAULT_RD_TEXTURE_ANISO,
		DEFAULT_RD_TEXTURE_DEPTH,
		DEFAULT_RD_TEXTURE_ROUGHNESS,
		DEFAULT_RD_TEXTURE_METALLIC,
		DEFAULT_RD_TEXTURE_FLOWMAP,
		DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE,
		DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK,
		DEFAULT_RD_TEXTURE_2D_ARRAY_NORMAL,
		DEFAULT_RD_TEXTURE_3D_WHITE,
		DEFAULT_RD_TEXTURE_3D_BLACK,
		DEFAULT_RD_TEXTURE_CUBEMAP_BLACK,
		DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_BLACK,
		DEFAULT_RD_TEXTURE_MAX
	};

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

	struct RenderTarget
	{
		RID self;
		Size2i size;
		Point2i position;
		RID color;
		RID depth;
		RID rd_framebuffer;
		bool direct_to_screen = false;
		bool is_transparent = false;
		bool was_used = false;
		bool clear_requested = false;
		Color clear_color;
		RSE::ViewportMSAA msaa = RSE::VIEWPORT_MSAA_DISABLED;
		bool use_hdr = false;
		bool use_debanding = false;

		Rect2i render_region;

		RID texture;

		struct Overridden
		{
			RID color;
			RID depth;
			RID velocity;
			RID velocity_depth;
			bool is_overridden = false;
		} overridden;

		uint32_t view_count = 1;
		Size2i velocity_target_size;

		RSE::ViewportVRSMode vrs_mode = RSE::VIEWPORT_VRS_DISABLED;
		RSE::ViewportVRSUpdateMode vrs_update_mode = RSE::VIEWPORT_VRS_UPDATE_ONCE;
		RID vrs_texture;
		bool subsampled_enabled = false;
		bool subsampled_allowed = false;
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
		int width = 0;
		int height = 0;
		int depth = 0;
		int layers = 0;
		Ref<Image> image;
		String path;
		bool is_render_target = false;
		bool is_proxy = false;
		RID proxy_to;
		Vector<RID> proxies;

		RID rd_texture;
		RID rd_texture_srgb;

		RSE::TextureLayeredType layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY;
		RSE::TextureDrawableFormat drawable_type = RSE::TEXTURE_DRAWABLE_FORMAT_RGBA8;

		RenderingServerTypes::TextureDetectCallback detect_3d_callback = nullptr;
		void* detect_3d_callback_ud = nullptr;
		RenderingServerTypes::TextureDetectCallback detect_normal_callback = nullptr;
		void* detect_normal_callback_ud = nullptr;
		RenderingServerTypes::TextureDetectRoughnessCallback detect_roughness_callback = nullptr;
		void* detect_roughness_callback_ud = nullptr;
		bool redraw_if_visible = false;
	};

	struct Decal
	{
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
	};

private:
	static inline RID_Owner<RenderTarget, true> render_target_owner;
	static inline RID_Owner<Texture, true> texture_owner;
	static inline RID_Owner<CanvasTexture, true> canvas_texture_owner;
	static inline RID_Owner<Decal, true> decal_owner;
	static inline RID_Owner<DecalInstance> decal_instance_owner;

	static inline RID default_rd_textures[DEFAULT_RD_TEXTURE_MAX];

public:
	TextureStorage() = delete;
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;
	~TextureStorage() = delete;

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
		RID p_render_target, const Size2i& p_target_size);
	static Size2i render_target_get_velocity_target_size(RID p_render_target);

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


