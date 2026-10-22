/**************************************************************************/
/*  texture_storage_common.h                                              */
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

#include "core/io/image.h"
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

class TextureStorageCommon
{
public:
    struct CanvasTexture
    {
        RID diffuse;
        RID normal_map;
        RID specular;
        Color specular_color = Color(1, 1, 1, 1);
        float shininess = 1.0f;

        RSE::CanvasItemTextureFilter texture_filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
        RSE::CanvasItemTextureRepeat texture_repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
    };

protected:
    static inline Color default_clear_color;
    static inline RID_Owner<CanvasTexture, true> canvas_texture_owner;

    static inline Ref<Image> texture_2d_placeholder;
    static inline Vector<Ref<Image>> texture_2d_array_placeholder;
    static inline Vector<Ref<Image>> cubemap_placeholder;
    static inline Vector<Ref<Image>> texture_3d_placeholder;

public:
	static void render_target_free(RID p_rid) {}

	static void render_target_set_vrs_texture(RID p_render_target, RID p_texture) {}

	static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
	}

	static void render_target_set_vrs_mode(RID p_render_target, RSE::ViewportVRSMode p_mode) {}

	static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale) {}

	static void render_target_set_use_debanding(RID p_render_target, bool p_use_debanding) {}

	static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr_2d);

	static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa) {}

	static void render_target_set_transparent(RID p_render_target, bool p_is_transparent) {}

	static RID render_target_get_texture(RID p_render_target) { return RID(); }

	static void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen) {}

	static void render_target_set_position(RID p_render_target, int p_x, int p_y) {}

	static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count) {}

	static RID render_target_create() { return RID(); }

	static void render_target_do_msaa_resolve(RID p_render_target) {}

	static bool render_target_get_msaa_needs_resolve(RID p_render_target) { return false; }

	static bool render_target_is_clear_requested(RID p_render_target) { return false; }

	static Size2 texture_size_with_proxy(RID p_proxy) { return Size2(); }

	static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled) {}

	static Rect2i render_target_get_sdf_rect(RID p_render_target) { return Rect2i(); }

	static void render_target_do_clear_request(RID p_render_target) {}

	static void render_target_request_clear(RID p_render_target, const Color& p_clear_color) {}

	static AABB decal_get_aabb(RID p_decal) { return AABB(); }

	static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image) {}

	static RID texture_allocate() { return RID(); }

	static uint32_t decal_get_cull_mask(RID p_decal) { return 0; }

	static void decal_instance_set_transform(RID p_decal, const Transform3D& p_transform) {}

	static void decal_instance_set_sorting_offset(RID p_decal_instance, float p_sorting_offset) {}

	static RID decal_instance_create(RID p_decal) { return RID(); }

	static void decal_instance_free(RID p_decal_instance) {}

	static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity_texture, RID p_velocity_depth_texture) {}

    /* Clear Color */

    static void set_default_clear_color(const Color& p_color) { default_clear_color = p_color; }
    static _FORCE_INLINE_ Color get_default_clear_color() { return default_clear_color; }

    /* Canvas Texture API */

    static _FORCE_INLINE_ bool owns_canvas_texture(RID p_rid) { return canvas_texture_owner.owns(p_rid); }
    static _FORCE_INLINE_ CanvasTexture* get_canvas_texture(RID p_rid) { return canvas_texture_owner.get_or_null(p_rid); }

    static RID canvas_texture_allocate();
    static void canvas_texture_initialize(RID p_rid);
    static void canvas_texture_free(RID p_rid);

    static void canvas_texture_set_channel(
        RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
    static void canvas_texture_set_shading_parameters(
        RID p_canvas_texture, const Color& p_base_color, float p_shininess);

    static void canvas_texture_set_texture_filter(
        RID p_item, RSE::CanvasItemTextureFilter p_filter);
    static void canvas_texture_set_texture_repeat(
        RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

    /* Placeholders */

    static void texture_2d_placeholder_initialize(RID p_texture);
    static void texture_2d_layered_placeholder_initialize(
        RID p_texture, RSE::TextureLayeredType p_layered_type);
    static void texture_3d_placeholder_initialize(RID p_texture);

    static _FORCE_INLINE_ Ref<Image> placeholder_2d_get() { return texture_2d_placeholder; }
    static _FORCE_INLINE_ Vector<Ref<Image>> placeholder_2d_array_get() { return texture_2d_array_placeholder; }
    static _FORCE_INLINE_ Vector<Ref<Image>> placeholder_cubemap_get() { return cubemap_placeholder; }
    static _FORCE_INLINE_ Vector<Ref<Image>> placeholder_3d_get() { return texture_3d_placeholder; }

    TextureStorageCommon() = delete;
    TextureStorageCommon(const TextureStorageCommon&) = delete;
    ~TextureStorageCommon() = delete;
};
