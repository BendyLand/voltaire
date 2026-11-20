/**************************************************************************/
/*  image_texture.h                                                       */
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

#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/texture.h"

class BitMap;

class ImageTexture final : public Texture2D
{
	struct SVGData
	{
		String source;
		float base_scale = 1.0f;
		float saturation = 1.0f;
		bool fix_alpha_border = false;
		bool premult_alpha = false;
		mutable RID base_texture;
		mutable HashMap<double, RID> texture_cache;
		mutable HashMap<Color, Color> cmap;
		mutable Size2 base_size;
		mutable Size2 size;
	};

	struct ScalingLevel
	{
		HashSet<ImageTexture*> textures;
		int32_t refcount = 1;
	};

	static Mutex svg_mutex;
	static HashMap<double, ScalingLevel> scaling_levels;

	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;
	bool mipmaps = false;
	int w = 0;
	int h = 0;
	Size2 size_override;
	mutable Ref<BitMap> alpha_cache;
	bool image_stored = false;
	SVGData* svg_data = nullptr;

	SVGData* _ensure_svg_data();
	void _svg_remove_scale(double p_scale);
	RID _svg_ensure_scale(double p_scale) const;
	RID _svg_load_at_scale(double p_scale, bool p_set_size) const;
	void _svg_update_texture();
	void _svg_clear();
	void _svg_destroy();

protected:
	virtual void reload_from_file() override;

public:
	void set_image(const Ref<Image>& p_image);
	static Ref<ImageTexture> create_from_image(const Ref<Image>& p_image);
	static Ref<ImageTexture> create_from_string(const String& p_source, float p_scale = 1.0f,
		float p_saturation = 1.0f, const HashMap<Color, Color>& p_color_map = HashMap<Color, Color>());

	// SVG / DPI-aware scalable source mode (folded from DPITexture).
	bool is_svg() const { return svg_data != nullptr; }

	void set_source(const String& p_source);
	String get_source() const;

	void set_fix_alpha_border(bool p_enabled);
	bool get_fix_alpha_border() const;

	void set_premult_alpha(bool p_enabled);
	bool get_premult_alpha() const;

	void set_base_scale(float p_scale);
	float get_base_scale() const;

	void set_saturation(float p_saturation);
	float get_saturation() const;

	void set_color_map(const HashMap<Color, Color>& p_color_map);
	HashMap<Color, Color> get_color_map() const;

	static void reference_scaling_level(double p_scale);
	static void unreference_scaling_level(double p_scale);

	virtual Image::Format get_format() const override;

	void update(const Ref<Image>& p_image);
	Ref<Image> get_image() const override;

	int get_width() const override;
	int get_height() const override;

	virtual RID get_rid() const override;
	virtual RID get_scaled_rid() const override;

	bool has_alpha() const override;
	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;

	void draw_msdf_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), int p_outline_size = 0, float p_px_range = 1.0,
		float p_scale = 1.0) const;
	void draw_lcd_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1)) const;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	void set_size_override(const Size2i& p_size);

	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	ImageTexture() = default;
	~ImageTexture();
};

class ImageTextureLayered final : public TextureLayered
{
	LayeredType layered_type = LAYERED_TYPE_2D_ARRAY;

	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;

	int width = 0;
	int height = 0;
	int layers = 0;
	bool mipmaps = false;

public:
	void set_layered_type(LayeredType p_type);

	virtual Image::Format get_format() const override;
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_layers() const override;
	virtual bool has_mipmaps() const override;
	virtual LayeredType get_layered_type() const override;

	Error create_from_images(Vector<Ref<Image>> p_images);
	void update_layer(const Ref<Image>& p_image, int p_layer);
	virtual Ref<Image> get_layer_data(int p_layer) const override;

	virtual RID get_rid() const override;
	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	ImageTextureLayered(LayeredType p_layered_type = LAYERED_TYPE_2D_ARRAY);
	~ImageTextureLayered();
};

class ImageTexture3D final : public Texture3D
{
	mutable RID texture;

	Image::Format format = Image::FORMAT_L8;
	int width = 1;
	int height = 1;
	int depth = 1;
	bool mipmaps = false;
	bool images_stored = false;

public:
	virtual Image::Format get_format() const override;
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_depth() const override;
	virtual bool has_mipmaps() const override;

	Error create(Image::Format p_format, int p_width, int p_height, int p_depth, bool p_mipmaps,
		const Vector<Ref<Image>>& p_data);
	void update(const Vector<Ref<Image>>& p_data);
	virtual Vector<Ref<Image>> get_data() const override;

	virtual RID get_rid() const override;
	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	ImageTexture3D();
	~ImageTexture3D();
};
