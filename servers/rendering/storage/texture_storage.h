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

#include "core/io/image.h"
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/rect2i.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

struct RenderTargetSetOverrideFn
{
	void (*ptr)(RID, RID, RID, RID, RID) = nullptr;

	void operator=(void (*p_ptr)(RID, RID, RID, RID, RID)) { ptr = p_ptr; }

	_FORCE_INLINE_ void operator()(RID p_render_target, RID p_color_texture, RID p_depth_texture,
		RID p_velocity = RID(), RID p_velocity_depth = RID()) const
	{
		if (ptr) {
			ptr(p_render_target, p_color_texture, p_depth_texture, p_velocity, p_velocity_depth);
		}
	}
};

class RendererTextureStorage final
{
public:
	RendererTextureStorage() = delete;
	RendererTextureStorage(const RendererTextureStorage&) = delete;
	RendererTextureStorage& operator=(const RendererTextureStorage&) = delete;
	~RendererTextureStorage() = delete;

	/* CANVAS TEXTURE API */

	inline static RID (*canvas_texture_allocate)() = nullptr;
	inline static void (*canvas_texture_initialize)(RID p_rid) = nullptr;
	inline static void (*canvas_texture_free)(RID p_rid) = nullptr;

	inline static void (*canvas_texture_set_channel)(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture) = nullptr;
	inline static void (*canvas_texture_set_shading_filter)(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter) = nullptr;
	inline static void (*canvas_texture_set_shading_parameters)(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess) = nullptr;
	inline static void (*canvas_texture_set_texture_filter)(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter) = nullptr;
	inline static void (*canvas_texture_set_texture_repeat)(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat) = nullptr;

	/* TEXTURE 2D / GENERAL API */

	inline static RID (*texture_allocate)() = nullptr;
	inline static void (*texture_initialize)(RID p_rid) = nullptr;
	inline static void (*texture_free)(RID p_rid) = nullptr;

	inline static RID (*texture_2d_create)(const Ref<Image>& p_image) = nullptr;
	inline static void (*texture_2d_initialize)(RID p_texture, const Ref<Image>& p_image) = nullptr;
	inline static void (*texture_2d_update)(
		RID p_texture, const Ref<Image>& p_image, int p_layer) = nullptr;
	inline static Ref<Image> (*texture_2d_get)(RID p_texture) = nullptr;
	inline static Ref<Image> (*texture_2d_layer_get)(RID p_texture, int p_layer) = nullptr;

	inline static void (*texture_2d_placeholder_initialize)(RID p_texture) = nullptr;

	inline static void (*texture_2d_layered_initialize)(RID p_texture,
		const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type) = nullptr;
	inline static void (*texture_2d_layered_placeholder_initialize)(
		RID p_texture, RSE::TextureLayeredType p_layered_type) = nullptr;

	inline static void (*texture_3d_initialize)(RID p_texture, Image::Format p_format, int p_width,
		int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data) = nullptr;
	inline static void (*texture_3d_update)(
		RID p_texture, const Vector<Ref<Image>>& p_data) = nullptr;
	inline static Vector<Ref<Image>> (*texture_3d_get)(RID p_texture) = nullptr;
	inline static void (*texture_3d_placeholder_initialize)(RID p_texture) = nullptr;

	inline static void (*texture_proxy_initialize)(RID p_texture, RID p_base) = nullptr;
	inline static void (*texture_proxy_update)(RID p_proxy, RID p_base) = nullptr;

	inline static void (*texture_drawable_initialize)(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps) = nullptr;
	inline static void (*texture_drawable_generate_mipmaps)(RID p_texture) = nullptr;
	inline static RID (*texture_drawable_get_default_material)() = nullptr;

	inline static void (*texture_external_initialize)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;
	inline static void (*texture_external_update)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;

	inline static uint64_t (*texture_get_native_handle)(RID p_texture, bool p_srgb) = nullptr;

	inline static void (*texture_set_format)(RID p_texture, Image::Format p_format) = nullptr;
	inline static Image::Format (*texture_get_format)(RID p_texture) = nullptr;
	inline static void (*texture_set_width)(RID p_texture, int p_width) = nullptr;
	inline static int (*texture_get_width)(RID p_texture) = nullptr;
	inline static void (*texture_set_height)(RID p_texture, int p_height) = nullptr;
	inline static int (*texture_get_height)(RID p_texture) = nullptr;
	inline static void (*texture_set_depth)(RID p_texture, int p_depth) = nullptr;
	inline static int (*texture_get_depth)(RID p_texture) = nullptr;
	inline static void (*texture_set_layers)(RID p_texture, int p_layers) = nullptr;
	inline static int (*texture_get_layers)(RID p_texture) = nullptr;

	inline static void (*texture_set_path)(RID p_texture, const String& p_path) = nullptr;
	inline static String (*texture_get_path)(RID p_texture) = nullptr;
	inline static void (*texture_set_size_override)(
		RID p_texture, int p_width, int p_height) = nullptr;

	inline static void (*texture_replace)(RID p_texture, RID p_by_texture) = nullptr;

	inline static void (*texture_set_force_redraw_if_visible)(
		RID p_texture, bool p_enable) = nullptr;
	inline static Size2 (*texture_size_with_proxy)(RID p_proxy) = nullptr;

	/* DECAL API */

	inline static RID (*decal_allocate)() = nullptr;
	inline static void (*decal_initialize)(RID p_rid) = nullptr;
	inline static void (*decal_free)(RID p_rid) = nullptr;

	inline static void (*decal_set_size)(RID p_decal, const Vector3& p_size) = nullptr;
	inline static void (*decal_set_texture)(
		RID p_decal, RSE::DecalTexture p_type, RID p_texture) = nullptr;
	inline static void (*decal_set_emission_energy)(RID p_decal, float p_energy) = nullptr;
	inline static void (*decal_set_albedo_mix)(RID p_decal, float p_mix) = nullptr;
	inline static void (*decal_set_modulate)(RID p_decal, const Color& p_modulate) = nullptr;
	inline static void (*decal_set_cull_mask)(RID p_decal, uint32_t p_layers) = nullptr;
	inline static void (*decal_set_distance_fade)(
		RID p_decal, bool p_enabled, float p_begin, float p_length) = nullptr;
	inline static void (*decal_set_fade)(RID p_decal, float p_above, float p_below) = nullptr;
	inline static void (*decal_set_normal_fade)(RID p_decal, float p_fade) = nullptr;

	inline static AABB (*decal_get_aabb)(RID p_decal) = nullptr;
	inline static uint32_t (*decal_get_cull_mask)(RID p_decal) = nullptr;

	inline static RID (*decal_instance_create)(RID p_decal) = nullptr;
	inline static void (*decal_instance_free)(RID p_decal_instance) = nullptr;
	inline static void (*decal_instance_set_transform)(
		RID p_decal_instance, const Transform3D& p_transform) = nullptr;
	inline static void (*decal_instance_set_sorting_offset)(
		RID p_decal_instance, float p_sorting_offset) = nullptr;

	/* RENDER TARGET API */

	inline static RID (*render_target_create)() = nullptr;
	inline static void (*render_target_free)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_position)(
		RID p_render_target, int p_x, int p_y) = nullptr;
	inline static Point2i (*render_target_get_position)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_size)(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count) = nullptr;
	inline static Size2i (*render_target_get_size)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_transparent)(
		RID p_render_target, bool p_is_transparent) = nullptr;
	inline static bool (*render_target_is_transparent)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_direct_to_screen)(
		RID p_render_target, bool p_direct_to_screen) = nullptr;
	inline static bool (*render_target_is_direct_to_screen)(RID p_render_target) = nullptr;

	inline static RID (*render_target_get_texture)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_msaa)(
		RID p_render_target, RSE::ViewportMSAA p_msaa) = nullptr;
	inline static RSE::ViewportMSAA (*render_target_get_msaa)(RID p_render_target) = nullptr;
	inline static bool (*render_target_get_msaa_needs_resolve)(RID p_render_target) = nullptr;
	inline static void (*render_target_do_msaa_resolve)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_use_hdr)(RID p_render_target, bool p_use_hdr) = nullptr;
	inline static bool (*render_target_is_using_hdr)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_use_debanding)(
		RID p_render_target, bool p_use_debanding) = nullptr;

	inline static void (*render_target_set_vrs_mode)(
		RID p_render_target, RSE::ViewportVRSMode p_mode) = nullptr;
	inline static void (*render_target_set_vrs_update_mode)(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode) = nullptr;
	inline static void (*render_target_set_vrs_texture)(
		RID p_render_target, RID p_texture) = nullptr;

	inline static void (*render_target_request_clear)(
		RID p_render_target, const Color& p_clear_color) = nullptr;
	inline static bool (*render_target_is_clear_requested)(RID p_render_target) = nullptr;
	inline static Color (*render_target_get_clear_request_color)(RID p_render_target) = nullptr;
	inline static void (*render_target_do_clear_request)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_sdf_size_and_scale)(RID p_render_target,
		RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale) = nullptr;
	inline static Rect2i (*render_target_get_sdf_rect)(RID p_render_target) = nullptr;
	inline static void (*render_target_mark_sdf_enabled)(
		RID p_render_target, bool p_enabled) = nullptr;

	inline static RenderTargetSetOverrideFn render_target_set_override;

	inline static RID (*render_target_get_override_color)(RID p_render_target) = nullptr;
	inline static RID (*render_target_get_override_depth)(RID p_render_target) = nullptr;
	inline static RID (*render_target_get_override_velocity)(RID p_render_target) = nullptr;
	inline static RID (*render_target_get_override_velocity_depth)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_render_region)(
		RID p_render_target, const Rect2i& p_render_region) = nullptr;
	inline static Rect2i (*render_target_get_render_region)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_velocity_target_size)(
		RID p_render_target, const Size2i& p_target_size) = nullptr;
	inline static Size2i (*render_target_get_velocity_target_size)(RID p_render_target) = nullptr;

	inline static bool (*render_target_was_used)(RID p_render_target) = nullptr;
	inline static void (*render_target_set_as_unused)(RID p_render_target) = nullptr;

	inline static void (*render_target_set_as_current)(RID p_render_target) = nullptr;

	inline static void (*set_default_clear_color)(const Color& p_color) = nullptr;
	inline static Color (*get_default_clear_color)() = nullptr;
};


