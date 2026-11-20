/**************************************************************************/
/*  image_texture.cpp                                                     */
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

#include "image_texture.h"

#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "modules/modules_enabled.gen.h" // For svg.
#include "scene/resources/bit_map.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#ifdef MODULE_SVG_ENABLED
#include "modules/svg/image_loader_svg.h"
#endif

Mutex ImageTexture::svg_mutex;
HashMap<double, ImageTexture::ScalingLevel> ImageTexture::scaling_levels;

void ImageTexture::reference_scaling_level(double p_scale)
{
	uint32_t oversampling = CLAMP(p_scale, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	double scale = double(oversampling) / 64.0;

	MutexLock lock(svg_mutex);
	ScalingLevel* sl = scaling_levels.getptr(scale);
	if (sl) {
		sl->refcount++;
	}
	else {
		ScalingLevel new_sl;
		scaling_levels.insert(scale, new_sl);
	}
}

void ImageTexture::unreference_scaling_level(double p_scale)
{
	uint32_t oversampling = CLAMP(p_scale, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	double scale = double(oversampling) / 64.0;

	MutexLock lock(svg_mutex);
	ScalingLevel* sl = scaling_levels.getptr(scale);
	if (sl) {
		sl->refcount--;
		if (sl->refcount == 0) {
			for (ImageTexture* tx : sl->textures) {
				tx->_svg_remove_scale(scale);
			}
			sl->textures.clear();
			scaling_levels.erase(scale);
		}
	}
}

ImageTexture::SVGData* ImageTexture::_ensure_svg_data()
{
	if (!svg_data) {
		svg_data = memnew(SVGData);
	}
	return svg_data;
}

void ImageTexture::_svg_remove_scale(double p_scale)
{
	if (!svg_data || Math::is_equal_approx(p_scale, 1.0)) {
		return;
	}

	RID* rid = svg_data->texture_cache.getptr(p_scale);
	if (rid) {
		if (rid->is_valid()) {
			Renderer::free_rid(*rid);
		}
		svg_data->texture_cache.erase(p_scale);
	}
}

RID ImageTexture::_svg_ensure_scale(double p_scale) const
{
	if (!svg_data) {
		return RID();
	}

	uint32_t oversampling = CLAMP(p_scale, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		if (svg_data->base_texture.is_null()) {
			svg_data->base_texture = _svg_load_at_scale(p_scale, true);
		}
		return svg_data->base_texture;
	}
	double scale = double(oversampling) / 64.0;

	RID* rid = svg_data->texture_cache.getptr(scale);
	if (rid) {
		return *rid;
	}

	MutexLock lock(svg_mutex);
	ScalingLevel* sl = scaling_levels.getptr(scale);
	ERR_FAIL_NULL_V_MSG(sl, RID(), "Invalid scaling level");
	sl->textures.insert(const_cast<ImageTexture*>(this));

	RID new_rid = _svg_load_at_scale(scale, false);
	svg_data->texture_cache[scale] = new_rid;
	return new_rid;
}

RID ImageTexture::_svg_load_at_scale(double p_scale, bool p_set_size) const
{
	if (!svg_data) {
		return RID();
	}

	Ref<Image> img;
	img.instantiate();
#ifdef MODULE_SVG_ENABLED
	const bool upsample = !Math::is_equal_approx(
		Math::round(p_scale * svg_data->base_scale), p_scale * svg_data->base_scale);

	Error err = ImageLoaderSVG::create_image_from_string(
		img, svg_data->source, p_scale * svg_data->base_scale, upsample, svg_data->cmap);
	if (err != OK) {
		return RID();
	}
#else
	img = Image::create_empty(Math::round(16 * p_scale * svg_data->base_scale),
		Math::round(16 * p_scale * svg_data->base_scale), false, Image::FORMAT_RGBA8);
#endif
	if (svg_data->saturation != 1.0f) {
		img->adjust_bcs(1.0, 1.0, svg_data->saturation);
	}

	if (svg_data->fix_alpha_border) {
		img->fix_alpha_edges();
	}

	if (svg_data->premult_alpha) {
		img->premultiply_alpha();
	}

	Size2 current_size = svg_data->size;
	if (p_set_size || svg_data->size.is_zero_approx()) {
		svg_data->size.x = img->get_width() / p_scale;
		svg_data->base_size.x = svg_data->size.x;
		if (size_override.x != 0) {
			svg_data->size.x = size_override.x;
		}
		svg_data->size.y = img->get_height() / p_scale;
		svg_data->base_size.y = svg_data->size.y;
		if (size_override.y != 0) {
			svg_data->size.y = size_override.y;
		}
		current_size = svg_data->size;
	}
	if (current_size.is_zero_approx()) {
		current_size.x = img->get_width();
		current_size.y = img->get_height();
	}

	RID rid = Renderer::texture_2d_create(img);
	Renderer::texture_set_size_override(rid, current_size.x, current_size.y);
	return rid;
}

void ImageTexture::_svg_clear()
{
	if (!svg_data) {
		return;
	}
	for (KeyValue<double, RID>& tx : svg_data->texture_cache) {
		if (tx.value.is_valid()) {
			Renderer::free_rid(tx.value);
		}
	}
	svg_data->texture_cache.clear();
	if (svg_data->base_texture.is_valid()) {
		Renderer::free_rid(svg_data->base_texture);
	}
	svg_data->base_texture = RID();
	alpha_cache.unref();
}

void ImageTexture::_svg_destroy()
{
	if (!svg_data) {
		return;
	}
	_svg_clear();
	{
		MutexLock lock(svg_mutex);
		for (KeyValue<double, ScalingLevel>& sl : scaling_levels) {
			sl.value.textures.erase(this);
		}
	}
	memdelete(svg_data);
	svg_data = nullptr;
}

void ImageTexture::_svg_update_texture()
{
	_svg_clear();
	emit_changed();
}

void ImageTexture::set_source(const String& p_source)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->source == p_source) {
		return;
	}
	sd->source = p_source;
	_svg_update_texture();
}

String ImageTexture::get_source() const { return svg_data ? svg_data->source : String(); }

void ImageTexture::set_fix_alpha_border(bool p_enabled)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->fix_alpha_border == p_enabled) {
		return;
	}
	sd->fix_alpha_border = p_enabled;
	_svg_update_texture();
}

bool ImageTexture::get_fix_alpha_border() const
{
	return svg_data ? svg_data->fix_alpha_border : false;
}

void ImageTexture::set_premult_alpha(bool p_enabled)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->premult_alpha == p_enabled) {
		return;
	}
	sd->premult_alpha = p_enabled;
	_svg_update_texture();
}

bool ImageTexture::get_premult_alpha() const { return svg_data ? svg_data->premult_alpha : false; }

void ImageTexture::set_base_scale(float p_scale)
{
	ERR_FAIL_COND(p_scale <= 0.0f);
	SVGData* sd = _ensure_svg_data();
	if (sd->base_scale == p_scale) {
		return;
	}
	sd->base_scale = p_scale;
	_svg_update_texture();
}

float ImageTexture::get_base_scale() const { return svg_data ? svg_data->base_scale : 1.0f; }

void ImageTexture::set_saturation(float p_saturation)
{
	SVGData* sd = _ensure_svg_data();
	if (sd->saturation == p_saturation) {
		return;
	}
	sd->saturation = p_saturation;
	_svg_update_texture();
}

float ImageTexture::get_saturation() const { return svg_data ? svg_data->saturation : 1.0f; }

void ImageTexture::set_color_map(const HashMap<Color, Color>& p_color_map)
{
	SVGData* sd = _ensure_svg_data();
	sd->cmap = p_color_map;
	_svg_update_texture();
}

HashMap<Color, Color> ImageTexture::get_color_map() const
{
	return svg_data ? svg_data->cmap : HashMap<Color, Color>();
}

void ImageTexture::reload_from_file() {}

Ref<ImageTexture> ImageTexture::create_from_image(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_V_MSG(p_image.is_null(), Ref<ImageTexture>(), "Invalid image: null");
	ERR_FAIL_COND_V_MSG(p_image->is_empty(), Ref<ImageTexture>(), "Invalid image: image is empty");

	Ref<ImageTexture> image_texture;
	image_texture.instantiate();
	image_texture->set_image(p_image);
	return image_texture;
}

Ref<ImageTexture> ImageTexture::create_from_string(const String& p_source, float p_scale,
	float p_saturation, const HashMap<Color, Color>& p_color_map)
{
	ERR_FAIL_COND_V_MSG(p_source.is_empty(), Ref<ImageTexture>(), "SVG source is empty.");
	ERR_FAIL_COND_V_MSG(p_scale <= 0.0f, Ref<ImageTexture>(), "Scale must be positive.");

	Ref<ImageTexture> image_texture;
	image_texture.instantiate();
	SVGData* sd = image_texture->_ensure_svg_data();
	sd->source = p_source;
	sd->base_scale = p_scale;
	sd->saturation = p_saturation;
	sd->cmap = p_color_map;
	image_texture->_svg_update_texture();
	return image_texture;
}

Image::Format ImageTexture::get_format() const
{
	if (svg_data) {
		return Image::FORMAT_RGBA8;
	}
	return format;
}

void ImageTexture::update(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_MSG(p_image.is_null(), "Invalid image");
	ERR_FAIL_COND_MSG(texture.is_null(), "Texture is not initialized.");
	ERR_FAIL_COND_MSG(p_image->get_width() != w || p_image->get_height() != h,
		"The new image dimensions must match the texture size.");
	ERR_FAIL_COND_MSG(p_image->get_format() != format,
		"The new image format must match the texture's image format.");
	ERR_FAIL_COND_MSG(mipmaps != p_image->has_mipmaps(),
		"The new image mipmaps configuration must match the texture's image mipmaps configuration");

	Renderer::texture_2d_update(texture, p_image);

	emit_changed();

	alpha_cache.unref();
	image_stored = true;
}

Ref<Image> ImageTexture::get_image() const
{
	if (svg_data) {
		RID rid = _svg_ensure_scale(1.0);
		if (rid.is_valid()) {
			return Renderer::texture_2d_get(rid);
		}
		return Ref<Image>();
	}

	if (image_stored) {
		return Renderer::texture_2d_get(texture);
	}
	else {
		return Ref<Image>();
	}
}

int ImageTexture::get_width() const
{
	if (svg_data) {
		_svg_ensure_scale(1.0);
		return svg_data->size.x;
	}
	return w;
}

int ImageTexture::get_height() const
{
	if (svg_data) {
		_svg_ensure_scale(1.0);
		return svg_data->size.y;
	}
	return h;
}

RID ImageTexture::get_rid() const
{
	if (svg_data) {
		return _svg_ensure_scale(1.0);
	}

	if (texture.is_null()) {
		// We are in trouble, create something temporary.
		texture = Renderer::texture_2d_placeholder_create();
	}
	return texture;
}

RID ImageTexture::get_scaled_rid() const
{
	if (svg_data) {
		double scale = TextServer::get_current_drawn_item_oversampling();
		if (scale == 0.0) {
			scale = 1.0;
		}
		return _svg_ensure_scale(scale);
	}
	return get_rid();
}

bool ImageTexture::has_alpha() const
{
	if (svg_data) {
		return true;
	}
	return (format == Image::FORMAT_LA8 || format == Image::FORMAT_RGBA8);
}

void ImageTexture::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if (svg_data) {
		RID rid = get_scaled_rid();
		Renderer::canvas_item_add_texture_rect(
			p_canvas_item, Rect2(p_pos, svg_data->size), rid, false, p_modulate, p_transpose);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, Size2(w, h)), texture, false, p_modulate, p_transpose);
}

void ImageTexture::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if (svg_data) {
		Renderer::canvas_item_add_texture_rect(
			p_canvas_item, p_rect, get_scaled_rid(), p_tile, p_modulate, p_transpose);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void ImageTexture::draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
	const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if (svg_data) {
		Renderer::canvas_item_add_texture_rect_region(
			p_canvas_item, p_rect, get_scaled_rid(), p_src_rect, p_modulate, p_transpose, p_clip_uv);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

void ImageTexture::draw_msdf_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, int p_outline_size, float p_px_range,
	float p_scale) const
{
	if (svg_data) {
		RID rid = get_scaled_rid();
		Renderer::canvas_item_add_msdf_texture_rect_region(
			p_canvas_item, p_rect, rid, p_src_rect, p_modulate, p_outline_size, p_px_range, p_scale);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_msdf_texture_rect_region(p_canvas_item,
		p_rect, texture, p_src_rect, p_modulate, p_outline_size, p_px_range, p_scale);
}

void ImageTexture::draw_lcd_rect_region(
	RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect, const Color& p_modulate) const
{
	if (svg_data) {
		RID rid = get_scaled_rid();
		Renderer::canvas_item_add_lcd_texture_rect_region(
			p_canvas_item, p_rect, rid, p_src_rect, p_modulate);
		return;
	}

	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_lcd_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate);
}

bool ImageTexture::is_pixel_opaque(int p_x, int p_y) const
{
	if (alpha_cache.is_valid()) {
		int aw = int(alpha_cache->get_size().width);
		int ah = int(alpha_cache->get_size().height);
		if (aw == 0 || ah == 0) {
			return true;
		}

		int tw = svg_data ? int(svg_data->size.x) : w;
		int th = svg_data ? int(svg_data->size.y) : h;
		if (tw == 0 || th == 0) {
			return true;
		}

		int x = p_x * aw / tw;
		int y = p_y * ah / th;

		x = CLAMP(x, 0, aw - 1);
		y = CLAMP(y, 0, ah - 1);

		return alpha_cache->get_bit(x, y);
	}

	return true;
}

void ImageTexture::set_size_override(const Size2i& p_size)
{
	if (svg_data) {
		if (size_override == p_size) {
			return;
		}
		size_override = p_size;
		if (size_override.x == 0 || size_override.y == 0) {
			_svg_ensure_scale(1.0);
			svg_data->size = svg_data->base_size;
		}
		if (size_override.x != 0) {
			svg_data->size.x = size_override.x;
		}
		if (size_override.y != 0) {
			svg_data->size.y = size_override.y;
		}
		for (KeyValue<double, RID>& tx : svg_data->texture_cache) {
			if (tx.value.is_valid()) {
				Renderer::texture_set_size_override(tx.value, svg_data->size.x, svg_data->size.y);
			}
		}
		if (svg_data->base_texture.is_valid()) {
			Renderer::texture_set_size_override(
				svg_data->base_texture, svg_data->size.x, svg_data->size.y);
		}
		emit_changed();
		return;
	}

	Size2i s = p_size;
	if (s.x != 0) {
		w = s.x;
	}
	if (s.y != 0) {
		h = s.y;
	}
	Renderer::texture_set_size_override(texture, w, h);
}

void ImageTexture::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

ImageTexture::~ImageTexture()
{
	if (svg_data) {
		_svg_destroy();
	}
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void ImageTexture::set_image(const Ref<Image>& p_image)
{
	if (p_image.is_null() || p_image->is_empty()) {
		if (image_stored) {
			ERR_PRINT("Invalid image");
		}
		return;
	}

	if (svg_data) {
		_svg_destroy();
	}

	w = p_image->get_width();
	h = p_image->get_height();
	format = p_image->get_format();
	mipmaps = p_image->has_mipmaps();

	if (texture.is_null()) {
		texture = Renderer::texture_2d_create(p_image);
	}
	else {
		RID new_texture = Renderer::texture_2d_create(p_image);
		Renderer::texture_replace(texture, new_texture);
	}
	emit_changed();

	alpha_cache.unref();
	image_stored = true;
}

Image::Format ImageTextureLayered::get_format() const { return format; }

int ImageTextureLayered::get_width() const { return width; }

int ImageTextureLayered::get_height() const { return height; }

int ImageTextureLayered::get_layers() const { return layers; }

bool ImageTextureLayered::has_mipmaps() const { return mipmaps; }

void ImageTextureLayered::set_layered_type(LayeredType p_type)
{
	layered_type = p_type;
}

ImageTextureLayered::LayeredType ImageTextureLayered::get_layered_type() const
{
	return layered_type;
}

Error ImageTextureLayered::create_from_images(Vector<Ref<Image>> p_images)
{
	int new_layers = p_images.size();
	ERR_FAIL_COND_V(new_layers == 0, ERR_INVALID_PARAMETER);
	if (layered_type == LAYERED_TYPE_CUBEMAP) {
		ERR_FAIL_COND_V_MSG(
			new_layers != 6, ERR_INVALID_PARAMETER, "Cubemaps require exactly 6 layers");
	}
	else if (layered_type == LAYERED_TYPE_CUBEMAP_ARRAY) {
		ERR_FAIL_COND_V_MSG((new_layers % 6) != 0, ERR_INVALID_PARAMETER,
			"Cubemap array layers must be a multiple of 6");
	}

	ERR_FAIL_COND_V(p_images[0].is_null() || p_images[0]->is_empty(), ERR_INVALID_PARAMETER);

	Image::Format new_format = p_images[0]->get_format();
	int new_width = p_images[0]->get_width();
	int new_height = p_images[0]->get_height();
	bool new_mipmaps = p_images[0]->has_mipmaps();

	for (int i = 1; i < p_images.size(); i++) {
		ERR_FAIL_COND_V_MSG(p_images[i]->get_format() != new_format, ERR_INVALID_PARAMETER,
			"All images must share the same format");
		ERR_FAIL_COND_V_MSG(
			p_images[i]->get_width() != new_width || p_images[i]->get_height() != new_height,
			ERR_INVALID_PARAMETER, "All images must share the same dimensions");
		ERR_FAIL_COND_V_MSG(p_images[i]->has_mipmaps() != new_mipmaps, ERR_INVALID_PARAMETER,
			"All images must share the usage of mipmaps");
	}

	if (texture.is_valid()) {
		RID new_texture = Renderer::texture_2d_layered_create(
			p_images, RSE::TextureLayeredType(layered_type));
		ERR_FAIL_COND_V(!new_texture.is_valid(), ERR_CANT_CREATE);
		Renderer::texture_replace(texture, new_texture);
	}
	else {
		texture = Renderer::texture_2d_layered_create(
			p_images, RSE::TextureLayeredType(layered_type));
		ERR_FAIL_COND_V(!texture.is_valid(), ERR_CANT_CREATE);
	}

	format = new_format;
	width = new_width;
	height = new_height;
	layers = new_layers;
	mipmaps = new_mipmaps;
	return OK;
}

void ImageTextureLayered::update_layer(const Ref<Image>& p_image, int p_layer)
{
	ERR_FAIL_COND_MSG(texture.is_null(), "Texture is not initialized.");
	ERR_FAIL_COND_MSG(p_image.is_null(), "Invalid image.");
	ERR_FAIL_COND_MSG(
		p_image->get_format() != format, "Image format must match texture's image format.");
	ERR_FAIL_COND_MSG(p_image->get_width() != width || p_image->get_height() != height,
		"Image size must match texture's image size.");
	ERR_FAIL_COND_MSG(p_image->has_mipmaps() != mipmaps,
		"Image mipmap configuration must match texture's image mipmap configuration.");
	ERR_FAIL_INDEX_MSG(p_layer, layers, "Layer index is out of bounds.");
	Renderer::texture_2d_update(texture, p_image, p_layer);
}

Ref<Image> ImageTextureLayered::get_layer_data(int p_layer) const
{
	ERR_FAIL_INDEX_V(p_layer, layers, Ref<Image>());
	return Renderer::texture_2d_layer_get(texture, p_layer);
}

RID ImageTextureLayered::get_rid() const
{
	if (texture.is_null()) {
		texture = Renderer::texture_2d_layered_placeholder_create(
			RSE::TextureLayeredType(layered_type));
	}
	return texture;
}

void ImageTextureLayered::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

ImageTextureLayered::ImageTextureLayered(LayeredType p_layered_type)
{
	layered_type = p_layered_type;
}

ImageTextureLayered::~ImageTextureLayered()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

Image::Format ImageTexture3D::get_format() const { return format; }

int ImageTexture3D::get_width() const { return width; }

int ImageTexture3D::get_height() const { return height; }

int ImageTexture3D::get_depth() const { return depth; }

bool ImageTexture3D::has_mipmaps() const { return mipmaps; }

Error ImageTexture3D::create(Image::Format p_format, int p_width, int p_height, int p_depth,
	bool p_mipmaps, const Vector<Ref<Image>>& p_data)
{
	RID tex = Renderer::texture_3d_create(
		p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	ERR_FAIL_COND_V(tex.is_null(), ERR_CANT_CREATE);

	if (texture.is_valid()) {
		Renderer::texture_replace(texture, tex);
	}
	else {
		texture = tex;
	}

	format = p_format;
	width = p_width;
	height = p_height;
	depth = p_depth;
	mipmaps = p_mipmaps;

	return OK;
}

void ImageTexture3D::update(const Vector<Ref<Image>>& p_data)
{
	ERR_FAIL_COND(!texture.is_valid());
	Renderer::texture_3d_update(texture, p_data);
}

Vector<Ref<Image>> ImageTexture3D::get_data() const
{
	ERR_FAIL_COND_V(!texture.is_valid(), Vector<Ref<Image>>());
	return Renderer::texture_3d_get(texture);
}

RID ImageTexture3D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_3d_placeholder_create();
	}
	return texture;
}

void ImageTexture3D::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

ImageTexture3D::ImageTexture3D() {}

ImageTexture3D::~ImageTexture3D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}
