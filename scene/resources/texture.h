/**************************************************************************/
/*  texture.h                                                             */
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
#include "core/io/resource.h"
#include "servers/rendering/rendering_server_enums.h"

class Mesh;

using Texture = Resource;

class Texture2D : public Resource
{
	mutable RID placeholder_rid;
	Size2 placeholder_size = Size2(1, 1);
	bool is_placeholder = false;

public:
	virtual Image::Format get_format() const;
	virtual int get_mipmap_count() const;
	virtual int get_width() const;
	virtual int get_height() const;
	virtual Size2 get_size() const;

	virtual bool is_pixel_opaque(int p_x, int p_y) const;

	virtual bool has_alpha() const;
	virtual bool has_mipmaps() const;

	virtual RID get_rid() const override;
	virtual RID get_scaled_rid() const { return get_rid(); }

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const;
	virtual bool get_rect_region(
		const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect, Rect2& r_src_rect) const;

	virtual Ref<Image> get_image() const;

	static Ref<Texture2D> create_placeholder_2d(Size2 p_size = Size2(1, 1));
	virtual Ref<Resource> create_placeholder() const;

	Texture2D();
	virtual ~Texture2D();
};

class CanvasTexture final : public Texture2D
{
public:
	enum TextureFilter
	{
		TEXTURE_FILTER_PARENT_NODE,
		TEXTURE_FILTER_NEAREST,
		TEXTURE_FILTER_LINEAR,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_MAX
	};

	enum TextureRepeat
	{
		TEXTURE_REPEAT_PARENT_NODE,
		TEXTURE_REPEAT_DISABLED,
		TEXTURE_REPEAT_ENABLED,
		TEXTURE_REPEAT_MIRROR,
		TEXTURE_REPEAT_MAX,
	};

private:
	Ref<Texture2D> diffuse_texture;
	Ref<Texture2D> normal_texture;
	Ref<Texture2D> specular_texture;
	Color specular = Color(1, 1, 1, 1);
	real_t shininess = 1.0;

	RID canvas_texture;

	TextureFilter texture_filter = TEXTURE_FILTER_PARENT_NODE;
	TextureRepeat texture_repeat = TEXTURE_REPEAT_PARENT_NODE;

	// Optional 2D mesh drawing mode (folded from MeshTexture).
	Ref<Mesh> mesh;
	Ref<Texture2D> base_texture;
	Size2i image_size;

public:
	void set_diffuse_texture(const Ref<Texture2D>& p_diffuse);
	Ref<Texture2D> get_diffuse_texture() const;

	void set_normal_texture(const Ref<Texture2D>& p_normal);
	Ref<Texture2D> get_normal_texture() const;

	void set_specular_texture(const Ref<Texture2D>& p_specular);
	Ref<Texture2D> get_specular_texture() const;

	void set_specular_color(const Color& p_color);
	Color get_specular_color() const;

	void set_specular_shininess(real_t p_shininess);
	real_t get_specular_shininess() const;

	void set_texture_filter(TextureFilter p_filter);
	TextureFilter get_texture_filter() const;

	void set_texture_repeat(TextureRepeat p_repeat);
	TextureRepeat get_texture_repeat() const;

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_image_size(const Size2& p_size);
	Size2 get_image_size() const;

	void set_base_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_base_texture() const;

	virtual int get_width() const override;
	virtual int get_height() const override;

	virtual bool is_pixel_opaque(int p_x, int p_y) const override;
	virtual bool has_alpha() const override;

	virtual Ref<Image> get_image() const override;

	virtual RID get_rid() const override;

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;
	virtual bool get_rect_region(const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect,
		Rect2& r_src_rect) const override;

	CanvasTexture();
	~CanvasTexture();
};

class TextureLayered : public Resource
{
public:
	enum LayeredType
	{
		LAYERED_TYPE_2D_ARRAY,
		LAYERED_TYPE_CUBEMAP,
		LAYERED_TYPE_CUBEMAP_ARRAY
	};

private:
	mutable RID placeholder_rid;
	Size2i placeholder_size = Size2i(1, 1);
	int placeholder_layers = 1;
	LayeredType placeholder_layered_type = LAYERED_TYPE_2D_ARRAY;
	bool is_placeholder = false;

public:
	virtual Image::Format get_format() const;
	virtual LayeredType get_layered_type() const;
	virtual int get_width() const;
	virtual int get_height() const;
	virtual int get_layers() const;
	virtual bool has_mipmaps() const;
	virtual Ref<Image> get_layer_data(int p_layer) const;
	virtual RID get_rid() const override;

	static Ref<TextureLayered> create_placeholder_layered(
		LayeredType p_type = LAYERED_TYPE_2D_ARRAY, const Size2i& p_size = Size2i(1, 1),
		int p_layers = 1);
	virtual Ref<Resource> create_placeholder() const;

	TextureLayered() = default;
	virtual ~TextureLayered();
};

class Texture3D : public Resource
{
	mutable RID placeholder_rid;
	Vector3i placeholder_size = Vector3i(1, 1, 1);
	bool is_placeholder = false;

public:
	virtual Image::Format get_format() const;
	virtual int get_width() const;
	virtual int get_height() const;
	virtual int get_depth() const;
	virtual bool has_mipmaps() const;
	virtual Vector<Ref<Image>> get_data() const;
	virtual RID get_rid() const override;

	static Ref<Texture3D> create_placeholder_3d(const Vector3i& p_size = Vector3i(1, 1, 1));
	virtual Ref<Resource> create_placeholder() const;

	Texture3D() = default;
	virtual ~Texture3D();
};
