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

namespace TextureStorage
{

struct API
{
	Color (*get_default_clear_color)() = nullptr;
	/* CANVAS TEXTURE API */
	RID (*canvas_texture_allocate)() = nullptr;
	void (*canvas_texture_initialize)(RID p_rid) = nullptr;
	void (*canvas_texture_free)(RID p_rid) = nullptr;

	void (*canvas_texture_set_channel)(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture) = nullptr;
	void (*canvas_texture_set_shading_parameters)(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess) = nullptr;
	void (*canvas_texture_set_texture_filter)(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter) = nullptr;
	void (*canvas_texture_set_texture_repeat)(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat) = nullptr;
	void (*set_default_clear_color)(const Color& p_color) = nullptr;

	/* TEXTURE 2D / GENERAL API */
	RID (*texture_allocate)() = nullptr;
	void (*texture_free)(RID p_rid) = nullptr;

	void (*texture_2d_initialize)(RID p_texture, const Ref<Image>& p_image) = nullptr;
	void (*texture_2d_update)(RID p_texture, const Ref<Image>& p_image, int p_layer) = nullptr;
	Ref<Image> (*texture_2d_get)(RID p_texture) = nullptr;
	Ref<Image> (*texture_2d_layer_get)(RID p_texture, int p_layer) = nullptr;

	void (*texture_2d_placeholder_initialize)(RID p_texture) = nullptr;

	void (*texture_2d_layered_initialize)(RID p_texture, const Vector<Ref<Image>>& p_layers,
		RSE::TextureLayeredType p_layered_type) = nullptr;
	void (*texture_2d_layered_placeholder_initialize)(
		RID p_texture, RSE::TextureLayeredType p_layered_type) = nullptr;

	void (*texture_3d_initialize)(RID p_texture, Image::Format p_format, int p_width, int p_height,
		int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data) = nullptr;
	void (*texture_3d_update)(RID p_texture, const Vector<Ref<Image>>& p_data) = nullptr;
	Vector<Ref<Image>> (*texture_3d_get)(RID p_texture) = nullptr;
	void (*texture_3d_placeholder_initialize)(RID p_texture) = nullptr;

	void (*texture_proxy_initialize)(RID p_texture, RID p_base) = nullptr;
	void (*texture_proxy_update)(RID p_proxy, RID p_base) = nullptr;

	void (*texture_drawable_initialize)(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps) = nullptr;
	void (*texture_drawable_generate_mipmaps)(RID p_texture) = nullptr;
	RID (*texture_drawable_get_default_material)() = nullptr;

	void (*texture_external_initialize)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;
	void (*texture_external_update)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;

	uint64_t (*texture_get_native_handle)(RID p_texture, bool p_srgb) = nullptr;

	void (*texture_set_format)(RID p_texture, Image::Format p_format) = nullptr;
	Image::Format (*texture_get_format)(RID p_texture) = nullptr;

	void (*texture_set_path)(RID p_texture, const String& p_path) = nullptr;
	String (*texture_get_path)(RID p_texture) = nullptr;
	void (*texture_set_size_override)(RID p_texture, int p_width, int p_height) = nullptr;

	void (*texture_replace)(RID p_texture, RID p_by_texture) = nullptr;
	void (*texture_set_force_redraw_if_visible)(RID p_texture, bool p_enable) = nullptr;
	Size2 (*texture_size_with_proxy)(RID p_proxy) = nullptr;

	/* DECAL API */
	RID (*decal_allocate)() = nullptr;
	void (*decal_initialize)(RID p_rid) = nullptr;
	void (*decal_free)(RID p_rid) = nullptr;

	void (*decal_set_size)(RID p_decal, const Vector3& p_size) = nullptr;
	void (*decal_set_texture)(RID p_decal, RSE::DecalTexture p_type, RID p_texture) = nullptr;
	void (*decal_set_emission_energy)(RID p_decal, float p_energy) = nullptr;
	void (*decal_set_albedo_mix)(RID p_decal, float p_mix) = nullptr;
	void (*decal_set_modulate)(RID p_decal, const Color& p_modulate) = nullptr;
	void (*decal_set_cull_mask)(RID p_decal, uint32_t p_layers) = nullptr;
	void (*decal_set_distance_fade)(
		RID p_decal, bool p_enabled, float p_begin, float p_length) = nullptr;
	void (*decal_set_fade)(RID p_decal, float p_above, float p_below) = nullptr;
	void (*decal_set_normal_fade)(RID p_decal, float p_fade) = nullptr;

	AABB (*decal_get_aabb)(RID p_decal) = nullptr;
	uint32_t (*decal_get_cull_mask)(RID p_decal) = nullptr;

	RID (*decal_instance_create)(RID p_decal) = nullptr;
	void (*decal_instance_free)(RID p_decal_instance) = nullptr;
	void (*decal_instance_set_transform)(
		RID p_decal_instance, const Transform3D& p_transform) = nullptr;
	void (*decal_instance_set_sorting_offset)(
		RID p_decal_instance, float p_sorting_offset) = nullptr;

	/* RENDER TARGET API */
	RID (*render_target_create)() = nullptr;
	void (*render_target_free)(RID p_render_target) = nullptr;

	void (*render_target_set_position)(RID p_render_target, int p_x, int p_y) = nullptr;
	Point2i (*render_target_get_position)(RID p_render_target) = nullptr;

	void (*render_target_set_size)(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count) = nullptr;
	Size2i (*render_target_get_size)(RID p_render_target) = nullptr;

	void (*render_target_set_transparent)(RID p_render_target, bool p_is_transparent) = nullptr;
	void (*render_target_set_direct_to_screen)(
		RID p_render_target, bool p_direct_to_screen) = nullptr;

	RID (*render_target_get_texture)(RID p_render_target) = nullptr;

	void (*render_target_set_msaa)(RID p_render_target, RSE::ViewportMSAA p_msaa) = nullptr;
	RSE::ViewportMSAA (*render_target_get_msaa)(RID p_render_target) = nullptr;
	bool (*render_target_get_msaa_needs_resolve)(RID p_render_target) = nullptr;
	void (*render_target_do_msaa_resolve)(RID p_render_target) = nullptr;

	void (*render_target_set_use_hdr)(RID p_render_target, bool p_use_hdr) = nullptr;
	bool (*render_target_is_using_hdr)(RID p_render_target) = nullptr;

	void (*render_target_set_use_debanding)(RID p_render_target, bool p_use_debanding) = nullptr;

	void (*render_target_set_vrs_mode)(RID p_render_target, RSE::ViewportVRSMode p_mode) = nullptr;
	void (*render_target_set_vrs_update_mode)(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode) = nullptr;
	void (*render_target_set_vrs_texture)(RID p_render_target, RID p_texture) = nullptr;

	void (*render_target_request_clear)(RID p_render_target, const Color& p_clear_color) = nullptr;
	bool (*render_target_is_clear_requested)(RID p_render_target) = nullptr;
	Color (*render_target_get_clear_request_color)(RID p_render_target) = nullptr;
	void (*render_target_do_clear_request)(RID p_render_target) = nullptr;

	void (*render_target_set_sdf_size_and_scale)(RID p_render_target,
		RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale) = nullptr;
	Rect2i (*render_target_get_sdf_rect)(RID p_render_target) = nullptr;
	void (*render_target_mark_sdf_enabled)(RID p_render_target, bool p_enabled) = nullptr;

	RenderTargetSetOverrideFn render_target_set_override;

	RID (*render_target_get_override_color)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_depth)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_velocity)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_velocity_depth)(RID p_render_target) = nullptr;

	void (*render_target_set_render_region)(
		RID p_render_target, const Rect2i& p_render_region) = nullptr;
	Rect2i (*render_target_get_render_region)(RID p_render_target) = nullptr;

	void (*render_target_set_velocity_target_size)(
		RID p_render_target, const Size2i& p_target_size) = nullptr;
	Size2i (*render_target_get_velocity_target_size)(RID p_render_target) = nullptr;

	bool (*render_target_was_used)(RID p_render_target) = nullptr;
	void (*render_target_set_as_unused)(RID p_render_target) = nullptr;
};

}; // namespace TextureStorage

class RendererTextureStorage final
{
private:
	static inline TextureStorage::API api;

public:
	RendererTextureStorage() = delete;
	RendererTextureStorage(const RendererTextureStorage&) = delete;
	RendererTextureStorage& operator=(const RendererTextureStorage&) = delete;
	~RendererTextureStorage() = delete;

	template <typename StorageImpl> static void bind()
	{
		api.set_default_clear_color = &StorageImpl::set_default_clear_color;
		api.get_default_clear_color = &StorageImpl::get_default_clear_color;
		/* CANVAS TEXTURE API */
		api.canvas_texture_allocate = &StorageImpl::canvas_texture_allocate;
		api.canvas_texture_initialize = &StorageImpl::canvas_texture_initialize;
		api.canvas_texture_free = &StorageImpl::canvas_texture_free;
		api.canvas_texture_set_channel = &StorageImpl::canvas_texture_set_channel;
		api.canvas_texture_set_shading_parameters =
			&StorageImpl::canvas_texture_set_shading_parameters;
		api.canvas_texture_set_texture_filter = &StorageImpl::canvas_texture_set_texture_filter;
		api.canvas_texture_set_texture_repeat = &StorageImpl::canvas_texture_set_texture_repeat;

		/* TEXTURE 2D / GENERAL API */
		api.texture_allocate = &StorageImpl::texture_allocate;
		api.texture_free = &StorageImpl::texture_free;
		api.texture_2d_initialize = &StorageImpl::texture_2d_initialize;
		api.texture_2d_update = &StorageImpl::texture_2d_update;
		api.texture_2d_get = &StorageImpl::texture_2d_get;
		api.texture_2d_layer_get = &StorageImpl::texture_2d_layer_get;
		api.texture_2d_placeholder_initialize = &StorageImpl::texture_2d_placeholder_initialize;
		api.texture_2d_layered_initialize = &StorageImpl::texture_2d_layered_initialize;
		api.texture_2d_layered_placeholder_initialize =
			&StorageImpl::texture_2d_layered_placeholder_initialize;
		api.texture_3d_initialize = &StorageImpl::texture_3d_initialize;
		api.texture_3d_update = &StorageImpl::texture_3d_update;
		api.texture_3d_get = &StorageImpl::texture_3d_get;
		api.texture_3d_placeholder_initialize = &StorageImpl::texture_3d_placeholder_initialize;
		api.texture_proxy_initialize = &StorageImpl::texture_proxy_initialize;
		api.texture_proxy_update = &StorageImpl::texture_proxy_update;
		api.texture_drawable_initialize = &StorageImpl::texture_drawable_initialize;
		api.texture_drawable_generate_mipmaps = &StorageImpl::texture_drawable_generate_mipmaps;
		api.texture_drawable_get_default_material =
			&StorageImpl::texture_drawable_get_default_material;
		api.texture_external_initialize = &StorageImpl::texture_external_initialize;
		api.texture_external_update = &StorageImpl::texture_external_update;
		api.texture_get_native_handle = &StorageImpl::texture_get_native_handle;
		api.texture_set_format = &StorageImpl::texture_set_format;
		api.texture_get_format = &StorageImpl::texture_get_format;
		api.texture_set_path = &StorageImpl::texture_set_path;
		api.texture_get_path = &StorageImpl::texture_get_path;
		api.texture_set_size_override = &StorageImpl::texture_set_size_override;
		api.texture_replace = &StorageImpl::texture_replace;
		api.texture_set_force_redraw_if_visible = &StorageImpl::texture_set_force_redraw_if_visible;
		api.texture_size_with_proxy = &StorageImpl::texture_size_with_proxy;

		/* DECAL API */
		api.decal_allocate = &StorageImpl::decal_allocate;
		api.decal_initialize = &StorageImpl::decal_initialize;
		api.decal_free = &StorageImpl::decal_free;
		api.decal_set_size = &StorageImpl::decal_set_size;
		api.decal_set_texture = &StorageImpl::decal_set_texture;
		api.decal_set_emission_energy = &StorageImpl::decal_set_emission_energy;
		api.decal_set_albedo_mix = &StorageImpl::decal_set_albedo_mix;
		api.decal_set_modulate = &StorageImpl::decal_set_modulate;
		api.decal_set_cull_mask = &StorageImpl::decal_set_cull_mask;
		api.decal_set_distance_fade = &StorageImpl::decal_set_distance_fade;
		api.decal_set_fade = &StorageImpl::decal_set_fade;
		api.decal_set_normal_fade = &StorageImpl::decal_set_normal_fade;
		api.decal_get_aabb = &StorageImpl::decal_get_aabb;
		api.decal_get_cull_mask = &StorageImpl::decal_get_cull_mask;
		api.decal_instance_create = &StorageImpl::decal_instance_create;
		api.decal_instance_free = &StorageImpl::decal_instance_free;
		api.decal_instance_set_transform = &StorageImpl::decal_instance_set_transform;
		api.decal_instance_set_sorting_offset = &StorageImpl::decal_instance_set_sorting_offset;

		/* RENDER TARGET API */
		api.render_target_create = &StorageImpl::render_target_create;
		api.render_target_free = &StorageImpl::render_target_free;
		api.render_target_set_position = &StorageImpl::render_target_set_position;
		api.render_target_get_position = &StorageImpl::render_target_get_position;
		api.render_target_set_size = &StorageImpl::render_target_set_size;
		api.render_target_get_size = &StorageImpl::render_target_get_size;
		api.render_target_set_transparent = &StorageImpl::render_target_set_transparent;
		api.render_target_set_direct_to_screen = &StorageImpl::render_target_set_direct_to_screen;
		api.render_target_get_texture = &StorageImpl::render_target_get_texture;
		api.render_target_set_msaa = &StorageImpl::render_target_set_msaa;
		api.render_target_get_msaa = &StorageImpl::render_target_get_msaa;
		api.render_target_get_msaa_needs_resolve =
			&StorageImpl::render_target_get_msaa_needs_resolve;
		api.render_target_do_msaa_resolve = &StorageImpl::render_target_do_msaa_resolve;
		api.render_target_set_use_hdr = &StorageImpl::render_target_set_use_hdr;
		api.render_target_is_using_hdr = &StorageImpl::render_target_is_using_hdr;
		api.render_target_set_use_debanding = &StorageImpl::render_target_set_use_debanding;
		api.render_target_set_vrs_mode = &StorageImpl::render_target_set_vrs_mode;
		api.render_target_set_vrs_update_mode = &StorageImpl::render_target_set_vrs_update_mode;
		api.render_target_set_vrs_texture = &StorageImpl::render_target_set_vrs_texture;
		api.render_target_request_clear = &StorageImpl::render_target_request_clear;
		api.render_target_is_clear_requested = &StorageImpl::render_target_is_clear_requested;
		api.render_target_get_clear_request_color =
			&StorageImpl::render_target_get_clear_request_color;
		api.render_target_do_clear_request = &StorageImpl::render_target_do_clear_request;
		api.render_target_set_sdf_size_and_scale =
			&StorageImpl::render_target_set_sdf_size_and_scale;
		api.render_target_get_sdf_rect = &StorageImpl::render_target_get_sdf_rect;
		api.render_target_mark_sdf_enabled = &StorageImpl::render_target_mark_sdf_enabled;
		api.render_target_set_override = &StorageImpl::render_target_set_override;
		api.render_target_get_override_color = &StorageImpl::render_target_get_override_color;
		api.render_target_get_override_depth = &StorageImpl::render_target_get_override_depth;
		api.render_target_get_override_velocity = &StorageImpl::render_target_get_override_velocity;
		api.render_target_get_override_velocity_depth =
			&StorageImpl::render_target_get_override_velocity_depth;
		api.render_target_set_render_region = &StorageImpl::render_target_set_render_region;
		api.render_target_get_render_region = &StorageImpl::render_target_get_render_region;
		api.render_target_set_velocity_target_size =
			&StorageImpl::render_target_set_velocity_target_size;
		api.render_target_get_velocity_target_size =
			&StorageImpl::render_target_get_velocity_target_size;
		api.render_target_was_used = &StorageImpl::render_target_was_used;
		api.render_target_set_as_unused = &StorageImpl::render_target_set_as_unused;
	}

	/* CANVAS TEXTURE API */
	_FORCE_INLINE_ static RID canvas_texture_allocate() { return api.canvas_texture_allocate(); }

	_FORCE_INLINE_ static void canvas_texture_initialize(RID p_rid)
	{
		api.canvas_texture_initialize(p_rid);
	}

	_FORCE_INLINE_ static void canvas_texture_free(RID p_rid) { api.canvas_texture_free(p_rid); }

	_FORCE_INLINE_ static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
	{
		api.canvas_texture_set_channel(p_canvas_texture, p_channel, p_texture);
	}

	_FORCE_INLINE_ static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess)
	{
		api.canvas_texture_set_shading_parameters(p_canvas_texture, p_base_color, p_shininess);
	}

	_FORCE_INLINE_ static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		api.canvas_texture_set_texture_filter(p_canvas_texture, p_filter);
	}

	_FORCE_INLINE_ static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
	{
		api.canvas_texture_set_texture_repeat(p_canvas_texture, p_repeat);
	}

	/* TEXTURE 2D / GENERAL API */
	_FORCE_INLINE_ static RID texture_allocate() { return api.texture_allocate(); }

	_FORCE_INLINE_ static void texture_free(RID p_rid) { api.texture_free(p_rid); }

	_FORCE_INLINE_ static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
	{
		api.texture_2d_initialize(p_texture, p_image);
	}

	_FORCE_INLINE_ static void texture_2d_update(
		RID p_texture, const Ref<Image>& p_image, int p_layer)
	{
		api.texture_2d_update(p_texture, p_image, p_layer);
	}

	_FORCE_INLINE_ static Ref<Image> texture_2d_get(RID p_texture)
	{
		return api.texture_2d_get(p_texture);
	}

	_FORCE_INLINE_ static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer)
	{
		return api.texture_2d_layer_get(p_texture, p_layer);
	}

	_FORCE_INLINE_ static void texture_2d_placeholder_initialize(RID p_texture)
	{
		api.texture_2d_placeholder_initialize(p_texture);
	}

	_FORCE_INLINE_ static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
	{
		api.texture_2d_layered_initialize(p_texture, p_layers, p_layered_type);
	}

	_FORCE_INLINE_ static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type)
	{
		api.texture_2d_layered_placeholder_initialize(p_texture, p_layered_type);
	}

	_FORCE_INLINE_ static void texture_3d_initialize(RID p_texture, Image::Format p_format,
		int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
	{
		api.texture_3d_initialize(
			p_texture, p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	}

	_FORCE_INLINE_ static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
	{
		api.texture_3d_update(p_texture, p_data);
	}

	_FORCE_INLINE_ static Vector<Ref<Image>> texture_3d_get(RID p_texture)
	{
		return api.texture_3d_get(p_texture);
	}

	_FORCE_INLINE_ static void texture_3d_placeholder_initialize(RID p_texture)
	{
		api.texture_3d_placeholder_initialize(p_texture);
	}

	_FORCE_INLINE_ static void texture_proxy_initialize(RID p_texture, RID p_base)
	{
		api.texture_proxy_initialize(p_texture, p_base);
	}

	_FORCE_INLINE_ static void texture_proxy_update(RID p_proxy, RID p_base)
	{
		api.texture_proxy_update(p_proxy, p_base);
	}

	_FORCE_INLINE_ static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps)
	{
		api.texture_drawable_initialize(
			p_texture, p_width, p_height, p_format, p_color, p_has_mipmaps);
	}

	_FORCE_INLINE_ static void texture_drawable_generate_mipmaps(RID p_texture)
	{
		api.texture_drawable_generate_mipmaps(p_texture);
	}

	_FORCE_INLINE_ static RID texture_drawable_get_default_material()
	{
		return api.texture_drawable_get_default_material();
	}

	_FORCE_INLINE_ static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		api.texture_external_initialize(p_texture, p_width, p_height, p_external_buffer);
	}

	_FORCE_INLINE_ static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		api.texture_external_update(p_texture, p_width, p_height, p_external_buffer);
	}

	_FORCE_INLINE_ static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb)
	{
		return api.texture_get_native_handle(p_texture, p_srgb);
	}

	_FORCE_INLINE_ static void texture_set_format(RID p_texture, Image::Format p_format)
	{
		api.texture_set_format(p_texture, p_format);
	}

	_FORCE_INLINE_ static Image::Format texture_get_format(RID p_texture)
	{
		return api.texture_get_format(p_texture);
	}

	_FORCE_INLINE_ static void texture_set_path(RID p_texture, const String& p_path)
	{
		api.texture_set_path(p_texture, p_path);
	}

	_FORCE_INLINE_ static String texture_get_path(RID p_texture)
	{
		return api.texture_get_path(p_texture);
	}

	_FORCE_INLINE_ static void texture_set_size_override(RID p_texture, int p_width, int p_height)
	{
		api.texture_set_size_override(p_texture, p_width, p_height);
	}

	_FORCE_INLINE_ static void texture_replace(RID p_texture, RID p_by_texture)
	{
		api.texture_replace(p_texture, p_by_texture);
	}

	_FORCE_INLINE_ static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable)
	{
		api.texture_set_force_redraw_if_visible(p_texture, p_enable);
	}

	_FORCE_INLINE_ static Size2 texture_size_with_proxy(RID p_proxy)
	{
		return api.texture_size_with_proxy(p_proxy);
	}

	/* DECAL API */
	_FORCE_INLINE_ static RID decal_allocate() { return api.decal_allocate(); }

	_FORCE_INLINE_ static void decal_initialize(RID p_rid) { api.decal_initialize(p_rid); }

	_FORCE_INLINE_ static void decal_free(RID p_rid) { api.decal_free(p_rid); }

	_FORCE_INLINE_ static void decal_set_size(RID p_decal, const Vector3& p_size)
	{
		api.decal_set_size(p_decal, p_size);
	}

	_FORCE_INLINE_ static void decal_set_texture(
		RID p_decal, RSE::DecalTexture p_type, RID p_texture)
	{
		api.decal_set_texture(p_decal, p_type, p_texture);
	}

	_FORCE_INLINE_ static void decal_set_emission_energy(RID p_decal, float p_energy)
	{
		api.decal_set_emission_energy(p_decal, p_energy);
	}

	_FORCE_INLINE_ static void decal_set_albedo_mix(RID p_decal, float p_mix)
	{
		api.decal_set_albedo_mix(p_decal, p_mix);
	}

	_FORCE_INLINE_ static void decal_set_modulate(RID p_decal, const Color& p_modulate)
	{
		api.decal_set_modulate(p_decal, p_modulate);
	}

	_FORCE_INLINE_ static void decal_set_cull_mask(RID p_decal, uint32_t p_layers)
	{
		api.decal_set_cull_mask(p_decal, p_layers);
	}

	_FORCE_INLINE_ static void decal_set_distance_fade(
		RID p_decal, bool p_enabled, float p_begin, float p_length)
	{
		api.decal_set_distance_fade(p_decal, p_enabled, p_begin, p_length);
	}

	_FORCE_INLINE_ static void decal_set_fade(RID p_decal, float p_above, float p_below)
	{
		api.decal_set_fade(p_decal, p_above, p_below);
	}

	_FORCE_INLINE_ static void decal_set_normal_fade(RID p_decal, float p_fade)
	{
		api.decal_set_normal_fade(p_decal, p_fade);
	}

	_FORCE_INLINE_ static AABB decal_get_aabb(RID p_decal) { return api.decal_get_aabb(p_decal); }

	_FORCE_INLINE_ static uint32_t decal_get_cull_mask(RID p_decal)
	{
		return api.decal_get_cull_mask(p_decal);
	}

	_FORCE_INLINE_ static RID decal_instance_create(RID p_decal)
	{
		return api.decal_instance_create(p_decal);
	}

	_FORCE_INLINE_ static void decal_instance_free(RID p_decal_instance)
	{
		api.decal_instance_free(p_decal_instance);
	}

	_FORCE_INLINE_ static void decal_instance_set_transform(
		RID p_decal_instance, const Transform3D& p_transform)
	{
		api.decal_instance_set_transform(p_decal_instance, p_transform);
	}

	_FORCE_INLINE_ static void decal_instance_set_sorting_offset(
		RID p_decal_instance, float p_sorting_offset)
	{
		api.decal_instance_set_sorting_offset(p_decal_instance, p_sorting_offset);
	}

	/* RENDER TARGET API */
	_FORCE_INLINE_ static RID render_target_create() { return api.render_target_create(); }

	_FORCE_INLINE_ static void render_target_free(RID p_render_target)
	{
		api.render_target_free(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_position(RID p_render_target, int p_x, int p_y)
	{
		api.render_target_set_position(p_render_target, p_x, p_y);
	}

	_FORCE_INLINE_ static Point2i render_target_get_position(RID p_render_target)
	{
		return api.render_target_get_position(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
	{
		api.render_target_set_size(p_render_target, p_width, p_height, p_view_count);
	}

	_FORCE_INLINE_ static Size2i render_target_get_size(RID p_render_target)
	{
		return api.render_target_get_size(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_transparent(
		RID p_render_target, bool p_is_transparent)
	{
		api.render_target_set_transparent(p_render_target, p_is_transparent);
	}

	_FORCE_INLINE_ static void render_target_set_direct_to_screen(
		RID p_render_target, bool p_direct_to_screen)
	{
		api.render_target_set_direct_to_screen(p_render_target, p_direct_to_screen);
	}

	_FORCE_INLINE_ static RID render_target_get_texture(RID p_render_target)
	{
		return api.render_target_get_texture(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
	{
		api.render_target_set_msaa(p_render_target, p_msaa);
	}

	_FORCE_INLINE_ static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target)
	{
		return api.render_target_get_msaa(p_render_target);
	}

	_FORCE_INLINE_ static bool render_target_get_msaa_needs_resolve(RID p_render_target)
	{
		return api.render_target_get_msaa_needs_resolve(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_do_msaa_resolve(RID p_render_target)
	{
		api.render_target_do_msaa_resolve(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
	{
		api.render_target_set_use_hdr(p_render_target, p_use_hdr);
	}

	_FORCE_INLINE_ static bool render_target_is_using_hdr(RID p_render_target)
	{
		return api.render_target_is_using_hdr(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_use_debanding(
		RID p_render_target, bool p_use_debanding)
	{
		api.render_target_set_use_debanding(p_render_target, p_use_debanding);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_mode(
		RID p_render_target, RSE::ViewportVRSMode p_mode)
	{
		api.render_target_set_vrs_mode(p_render_target, p_mode);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
		api.render_target_set_vrs_update_mode(p_render_target, p_mode);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_texture(RID p_render_target, RID p_texture)
	{
		api.render_target_set_vrs_texture(p_render_target, p_texture);
	}

	_FORCE_INLINE_ static void render_target_request_clear(
		RID p_render_target, const Color& p_clear_color)
	{
		api.render_target_request_clear(p_render_target, p_clear_color);
	}

	_FORCE_INLINE_ static bool render_target_is_clear_requested(RID p_render_target)
	{
		return api.render_target_is_clear_requested(p_render_target);
	}

	_FORCE_INLINE_ static Color render_target_get_clear_request_color(RID p_render_target)
	{
		return api.render_target_get_clear_request_color(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_do_clear_request(RID p_render_target)
	{
		api.render_target_do_clear_request(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
	{
		api.render_target_set_sdf_size_and_scale(p_render_target, p_size, p_scale);
	}

	_FORCE_INLINE_ static Rect2i render_target_get_sdf_rect(RID p_render_target)
	{
		return api.render_target_get_sdf_rect(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled)
	{
		api.render_target_mark_sdf_enabled(p_render_target, p_enabled);
	}

	_FORCE_INLINE_ static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity = RID(), RID p_velocity_depth = RID())
	{
		api.render_target_set_override(
			p_render_target, p_color_texture, p_depth_texture, p_velocity, p_velocity_depth);
	}

	_FORCE_INLINE_ static RID render_target_get_override_color(RID p_render_target)
	{
		return api.render_target_get_override_color(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_depth(RID p_render_target)
	{
		return api.render_target_get_override_depth(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_velocity(RID p_render_target)
	{
		return api.render_target_get_override_velocity(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_velocity_depth(RID p_render_target)
	{
		return api.render_target_get_override_velocity_depth(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_render_region(
		RID p_render_target, const Rect2i& p_render_region)
	{
		api.render_target_set_render_region(p_render_target, p_render_region);
	}

	_FORCE_INLINE_ static Rect2i render_target_get_render_region(RID p_render_target)
	{
		return api.render_target_get_render_region(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{
		api.render_target_set_velocity_target_size(p_render_target, p_target_size);
	}

	_FORCE_INLINE_ static Size2i render_target_get_velocity_target_size(RID p_render_target)
	{
		return api.render_target_get_velocity_target_size(p_render_target);
	}

	_FORCE_INLINE_ static bool render_target_was_used(RID p_render_target)
	{
		return api.render_target_was_used(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_as_unused(RID p_render_target)
	{
		api.render_target_set_as_unused(p_render_target);
	}

	_FORCE_INLINE_ static void set_default_clear_color(const Color& p_color)
	{
		if (api.set_default_clear_color) {
			api.set_default_clear_color(p_color);
		}
	}

	_FORCE_INLINE_ static Color get_default_clear_color()
	{
		if (api.get_default_clear_color) {
			return api.get_default_clear_color();
		}
		return Color();

	}
};


