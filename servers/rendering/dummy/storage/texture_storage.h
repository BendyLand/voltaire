/**************************************************************************/
/*  texture_storage.h                                                     */
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

#include "core/templates/rid_owner.h"
#include "servers/rendering/storage/texture_storage.h"

namespace RendererDummy
{

struct DummyTexture
{
	String path;
	int width = 0;
	int height = 0;
	int depth = 0;
	int layers = 0;
	Image::Format format = Image::FORMAT_L8;
	Ref<Image> image;
};

struct DummyRenderTarget
{
	Point2i position;
	Size2i size;
	RID texture;
	bool direct_to_screen = false;
	bool is_transparent = false;
	bool was_used = false;
	bool clear_requested = false;
	Color clear_color;
	RSE::ViewportMSAA msaa = RSE::VIEWPORT_MSAA_DISABLED;
	bool use_hdr = false;
};

struct DummyCanvasTexture
{
	RID diffuse;
	RID normal_map;
	RID specular;
	Color specular_color = Color(1, 1, 1, 1);
	real_t shininess = 1.0;
	RSE::CanvasItemTextureFilter filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
	RSE::CanvasItemTextureRepeat repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
};

class TextureStorage final
{
private:
	struct Data
	{
		RID_Owner<DummyTexture> texture_owner;
		RID_Owner<DummyRenderTarget> render_target_owner;
		RID_Owner<DummyCanvasTexture> canvas_texture_owner;
		Color default_clear_color = Color(0, 0, 0, 1);
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			data = memnew(Data);
		}
		return data;
	}

public:
	TextureStorage() = delete;
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;
	~TextureStorage() = delete;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	static bool owns_texture(RID p_rid) { return data && data->texture_owner.owns(p_rid); }
	static bool owns_render_target(RID p_rid)
	{
		return data && data->render_target_owner.owns(p_rid);
	}
	static bool owns_canvas_texture(RID p_rid)
	{
		return data && data->canvas_texture_owner.owns(p_rid);
	}

	/* CANVAS TEXTURE API */

	static RID canvas_texture_allocate() { return _get_data()->canvas_texture_owner.allocate_rid(); }

	static void canvas_texture_initialize(RID p_rid)
	{
		_get_data()->canvas_texture_owner.initialize_rid(p_rid, DummyCanvasTexture());
	}

	static void canvas_texture_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->canvas_texture_owner.free(p_rid);
	}

	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		switch (p_channel) {
		case RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE:
			ct->diffuse = p_texture;
			break;
		case RSE::CANVAS_TEXTURE_CHANNEL_NORMAL:
			ct->normal_map = p_texture;
			break;
		case RSE::CANVAS_TEXTURE_CHANNEL_SPECULAR:
			ct->specular = p_texture;
			break;
		}
	}

	static void canvas_texture_set_shading_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->filter = p_filter;
	}

	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->specular_color = p_base_color;
		ct->shininess = p_shininess;
	}

	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->filter = p_filter;
	}

	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->repeat = p_repeat;
	}

	/* TEXTURE 2D / GENERAL API */

	static RID texture_allocate() { return _get_data()->texture_owner.allocate_rid(); }

	static void texture_initialize(RID p_rid)
	{
		_get_data()->texture_owner.initialize_rid(p_rid, DummyTexture());
	}

	static void texture_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->texture_owner.free(p_rid);
	}

	static RID texture_2d_create(const Ref<Image>& p_image)
	{
		RID rid = _get_data()->texture_owner.allocate_rid();
		texture_2d_initialize(rid, p_image);
		return rid;
	}

	static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
	{
		DummyTexture t;
		if (p_image.is_valid()) {
			t.image = p_image;
			t.width = p_image->get_width();
			t.height = p_image->get_height();
			t.format = p_image->get_format();
		}
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->image = p_image;
	}

	static Ref<Image> texture_2d_get(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, Ref<Image>());
		return t->image;
	}

	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer)
	{
		return texture_2d_get(p_texture);
	}

	static void texture_2d_placeholder_initialize(RID p_texture) { texture_initialize(p_texture); }

	static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
	{
		DummyTexture t;
		t.layers = p_layers.size();
		if (!p_layers.is_empty() && p_layers[0].is_valid()) {
			t.image = p_layers[0];
			t.width = p_layers[0]->get_width();
			t.height = p_layers[0]->get_height();
			t.format = p_layers[0]->get_format();
		}
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type)
	{
		texture_initialize(p_texture);
	}

	static void texture_3d_initialize(RID p_texture, Image::Format p_format, int p_width,
		int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		t.depth = p_depth;
		t.format = p_format;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data) {}

	static Vector<Ref<Image>> texture_3d_get(RID p_texture) { return Vector<Ref<Image>>(); }

	static void texture_3d_placeholder_initialize(RID p_texture) { texture_initialize(p_texture); }

	static void texture_proxy_initialize(RID p_texture, RID p_base)
	{
		texture_initialize(p_texture);
	}

	static void texture_proxy_update(RID p_proxy, RID p_base) {}

	static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_drawable_generate_mipmaps(RID p_texture) {}

	static RID texture_drawable_get_default_material() { return RID(); }

	static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
	}

	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb) { return 0; }

	static void texture_set_format(RID p_texture, Image::Format p_format)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->format = p_format;
	}

	static Image::Format texture_get_format(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, Image::FORMAT_L8);
		return t->format;
	}

	static void texture_set_width(RID p_texture, int p_width)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->width = p_width;
	}

	static int texture_get_width(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->width;
	}

	static void texture_set_height(RID p_texture, int p_height)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->height = p_height;
	}

	static int texture_get_height(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->height;
	}

	static void texture_set_depth(RID p_texture, int p_depth)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->depth = p_depth;
	}

	static int texture_get_depth(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->depth;
	}

	static void texture_set_layers(RID p_texture, int p_layers)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->layers = p_layers;
	}

	static int texture_get_layers(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->layers;
	}

	static void texture_set_path(RID p_texture, const String& p_path)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->path = p_path;
	}

	static String texture_get_path(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, String());
		return t->path;
	}

	static void texture_set_size_override(RID p_texture, int p_width, int p_height) {}

	static void texture_replace(RID p_texture, RID p_by_texture) {}

	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {}

	static Size2 texture_size_with_proxy(RID p_proxy)
	{
		return Size2(texture_get_width(p_proxy), texture_get_height(p_proxy));
	}

	/* DECAL API */

	static RID decal_allocate() { return RID(); }
	static void decal_initialize(RID p_rid) {}
	static void decal_free(RID p_rid) {}
	static void decal_set_size(RID p_decal, const Vector3& p_size) {}
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture) {}
	static void decal_set_emission_energy(RID p_decal, float p_energy) {}
	static void decal_set_albedo_mix(RID p_decal, float p_mix) {}
	static void decal_set_modulate(RID p_decal, const Color& p_modulate) {}
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers) {}
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length)
	{
	}
	static void decal_set_fade(RID p_decal, float p_above, float p_below) {}
	static void decal_set_normal_fade(RID p_decal, float p_fade) {}
	static AABB decal_get_aabb(RID p_decal) { return AABB(); }
	static uint32_t decal_get_cull_mask(RID p_decal) { return 0; }

	static RID decal_instance_create(RID p_decal) { return RID(); }
	static void decal_instance_free(RID p_decal_instance) {}
	static void decal_instance_set_transform(RID p_decal_instance, const Transform3D& p_transform)
	{
	}
	static void decal_instance_set_sorting_offset(RID p_decal_instance, float p_sorting_offset) {}

	/* RENDER TARGET API */

	static RID render_target_create()
	{
		Data* d = _get_data();
		RID rid = d->render_target_owner.allocate_rid();
		DummyRenderTarget rt;
		rt.texture = texture_allocate();
		texture_initialize(rt.texture);
		d->render_target_owner.initialize_rid(rid, rt);
		return rid;
	}

	static void render_target_free(RID p_render_target)
	{
		if (!data) {
			return;
		}
		DummyRenderTarget* rt = data->render_target_owner.get_or_null(p_render_target);
		if (rt) {
			if (rt->texture.is_valid()) {
				texture_free(rt->texture);
			}
			data->render_target_owner.free(p_render_target);
		}
	}

	static void render_target_set_position(RID p_render_target, int p_x, int p_y)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->position = Point2i(p_x, p_y);
	}

	static Point2i render_target_get_position(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Point2i());
		return rt->position;
	}

	static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->size = Size2i(p_width, p_height);
	}

	static Size2i render_target_get_size(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Size2i());
		return rt->size;
	}

	static void render_target_set_transparent(RID p_render_target, bool p_is_transparent)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->is_transparent = p_is_transparent;
	}

	static bool render_target_is_transparent(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->is_transparent;
	}

	static void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->direct_to_screen = p_direct_to_screen;
	}

	static bool render_target_is_direct_to_screen(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->direct_to_screen;
	}

	static RID render_target_get_texture(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, RID());
		return rt->texture;
	}

	static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->msaa = p_msaa;
	}

	static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_MSAA_DISABLED);
		return rt->msaa;
	}

	static bool render_target_get_msaa_needs_resolve(RID p_render_target) { return false; }

	static void render_target_do_msaa_resolve(RID p_render_target) {}

	static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->use_hdr = p_use_hdr;
	}

	static bool render_target_is_using_hdr(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->use_hdr;
	}

	static void render_target_set_use_debanding(RID p_render_target, bool p_use_debanding) {}

	static void render_target_set_vrs_mode(RID p_render_target, RSE::ViewportVRSMode p_mode) {}

	static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
	}

	static void render_target_set_vrs_texture(RID p_render_target, RID p_texture) {}

	static void render_target_request_clear(RID p_render_target, const Color& p_clear_color)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->clear_requested = true;
		rt->clear_color = p_clear_color;
	}

	static bool render_target_is_clear_requested(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->clear_requested;
	}

	static Color render_target_get_clear_request_color(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Color());
		return rt->clear_color;
	}

	static void render_target_do_clear_request(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->clear_requested = false;
	}

	static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
	{
	}

	static Rect2i render_target_get_sdf_rect(RID p_render_target) { return Rect2i(); }

	static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled) {}

	static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity, RID p_velocity_depth)
	{
	}

	static RID render_target_get_override_color(RID p_render_target) { return RID(); }
	static RID render_target_get_override_depth(RID p_render_target) { return RID(); }
	static RID render_target_get_override_velocity(RID p_render_target) { return RID(); }
	static RID render_target_get_override_velocity_depth(RID p_render_target) { return RID(); }

	static void render_target_set_render_region(
		RID p_render_target, const Rect2i& p_render_region)
	{
	}

	static Rect2i render_target_get_render_region(RID p_render_target) { return Rect2i(); }

	static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{
	}

	static Size2i render_target_get_velocity_target_size(RID p_render_target) { return Size2i(); }

	static bool render_target_was_used(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->was_used;
	}

	static void render_target_set_as_unused(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->was_used = false;
	}

	static void render_target_set_as_current(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->was_used = true;
	}

	static void set_default_clear_color(const Color& p_color)
	{
		_get_data()->default_clear_color = p_color;
	}

	static Color get_default_clear_color() { return _get_data()->default_clear_color; }
};

} // namespace RendererDummy


